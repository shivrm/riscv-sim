#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef SIMULATOR_H
#include "simulator.h"
#endif

#define BUF_CHUNK_SIZE 4096
#define PN_CHUNK_SIZE 4096
#define DATA_SEGMENT_START 0x10000 

void sim_stack_push(Simulator *s, char *label, int line);
void sim_stack_pop(Simulator *s);

// Reads an entire file into a buffer
char *read_file(FILE *f) {
	size_t len = 0, cap = 0;
	char *buf = NULL;
	while(1) {
		size_t available = cap - len;
		// Reallocate buffer if no space is available
		if (available <= 1) {
			buf = realloc(buf, cap + BUF_CHUNK_SIZE);
			for (int i = cap; i < cap + BUF_CHUNK_SIZE; i++) {
				buf[i] = 0;
			}
			cap += BUF_CHUNK_SIZE;
		}

		fgets(&buf[len], available, f);
		// fgets doesn't return the number of characters read, so strlen is used.
		size_t n = strlen(&buf[len]);
		len += n; 
		
		// If the number of bytes read is less than n, that means we either hit
		// a newline or the EOF.
		if (n < available && feof(f)) {
			return buf; 
		}	
	}
}

void print_line(char *src, int line) {
	// Moves to the starting line of the error
	char *ptr = src;
	for (int i = 1; i < line; i++) {
		while ((*ptr != '\n') && (*ptr != '\0')) {
			ptr++;
		}	
		ptr++;
	}	

	char *start = ptr;

	// Move until end of line. If colon detected, then start at the
	// character after colon
	while (*ptr != '\n' && *ptr != '\0') {
		if (*ptr == ':') {
			start = ptr + 1;
		}
		ptr++;
	}

	char *end = ptr;

	// Trim leading whitespace
	while ((*start == ' ') || (*start == '\t')) start++;


	printf("%.*s", (int)(end - start), start);
}

// Prints a parse error
void print_parse_error(char *src, ParseErr *err) {	

	// Moves to the starting line of the error
	char *ptr = src;
	for (int i = 1; i < err->line; i++) {
		while ((*ptr != '\n') && (*ptr != '\0')) {
			ptr++;
		}	
		ptr++;
	}	
	char *start = ptr;		
	while (*ptr != '\n' && *ptr != '\0') ptr++;	
	char *end = ptr;


	printf("Error on line %d: %s\n", err->line, err->msg);



	// Add arrows underneath to point to error position
	printf("%.*s\n", (int)(end - start), start);
	for (int i = 1; i < err->scol; i++) {
		if (start[i] == '\t') {
			printf("\t");
		} else {
			printf(" ");
		}
	}
	for (int i = err->scol; i < err->ecol; i++) {
		printf("^");
	}
	printf("\n");
}

// Prints an emit error
void print_emit_error(char *src, EmitErr *err) {
	// Moves to the starting line`of the error
	char *ptr = src;
	for (int i = 1; i < err->line; i++) {
		while ((*ptr != '\n') && (*ptr != '\0')) {
			ptr++;
		}	
		ptr++;
	}	
	char *start = ptr;		
	while (*ptr != '\n' && *ptr != '\0') ptr++;	
	char *end = ptr;

	printf("Error on line %d: %s\n", err->line, err->msg);	
	printf("%.*s\n", (int)(end - start), start);
}


void sim_init(Simulator *s) {
	s->pc = 0;

	s->breaks = malloc(sizeof(BreakPointVec));
	s->breaks->len = 0;
	s->breaks->cap = 0;
	s->breaks->data = NULL;

	s->stack = malloc(sizeof(StackVec));
	s->stack->len = 0;
	s->stack->cap = 0;
	s->stack->data = NULL;

	s->labels = malloc(sizeof(LabelVec));
	s->labels->len = 0;
	s->labels->cap = 0;
	s->labels->data = NULL;

	sim_stack_push(s, "main", 0);

	for (int i = 0; i < 32; i++) {
		s->regs[i] = 0;
	}

	for (int i = 0; i < MEM_SIZE; i++) {
		s->mem[i] = 0;
	} 

    CacheConfig cfg = {256, 16, 1, FIFO, WRITETHROUGH};

	// If cache is enabled, create a cache and initialize it	
	if (s->cache_enabled) {
		s->cache = malloc(sizeof(Cache));
		cache_init(s->cache, &cfg);
		s->cache->mem = s->mem;
	}
}

int sim_load(Simulator *s, char *file) {
	FILE *fin = fopen(file, "r");	
	if (!fin) {
		printf("Could not open input file\n");
		return -1;
	}

	char* src = read_file(fin);
	fclose(fin);

	Lexer l;
	lexer_init(&l, src);
	Parser p;
	ParseErr err = {0, "", 0, 0, 0};
	parser_init(&p, &l);

	ParseNodeVec pn = {0};
	DataVec d = {0};

	parse_all(&p, &pn, &d, &err);
	if (err.is_err) {
		print_parse_error(src, &err);
		return 1;
	}

	EmitErr err2 = {0, "", 0};
    find_labels(pn.data, pn.len, s->labels, &err2);
	if (err2.is_err) {
		print_emit_error(src, &err2);
		return -1;
	}

	emit_all(s->mem, pn.data, pn.len, s->labels, &err2);
	if (err2.is_err) {
		print_emit_error(src, &err2);
		return -1;
	}

    s->src = src;
    s->nodes = pn.data;  

	// Copy d into data segment
	for (int i = 0; i < d.len; i++) {
		s->mem[DATA_SEGMENT_START + i] = d.data[i];	
	}

	return 0;
}

void sim_run_one(Simulator *s) {
    // Read 32-bits from index `pc` of memory
    uint32_t ins = *(uint32_t*)(&s->mem[s->pc]);

    int opcode = ins & 0b1111111;

    switch (opcode) {
        case 0b0110011:{// R format
            int rd_idx = (ins >> 7) & 0b11111,
        		funct3 = (ins >> 12) & 0b111,
            	rs1_idx = (ins >> 15) & 0b11111,
            	rs2_idx = (ins >> 20) & 0b11111,
            	funct7 = (ins >> 25) & 0b1111111;

            int64_t rs1 = s->regs[rs1_idx], rs2 = s->regs[rs2_idx], rd;
            
            switch(funct3) {
                case 0x0:
                    switch (funct7) {
                        case 0x00: rd = rs1 + rs2; break; // add                    
                        case 0x20: rd = rs1 - rs2; break; // sub
                    }
                    break;
                case 0x4: // xor
                    rd = rs1 ^ rs2;
                    break;
				case 0x6: // or
					rd = rs1 | rs2;
					break;
				case 0x7: // and
					rd = rs1 & rs2;
					break;
				case 0x1: // sll
					rd = rs1 << rs2;
					break;
				case 0x5: 
					switch(funct7) {
                    	case 0x20: rd = rs1 >> rs2; break; // sra                   
                    	case 0x00: //srl
							if (rs1>>63 == -1){ // if the first bit is -1
								rd = (int64_t) ((uint64_t)(rs1) >> (uint64_t)(rs2));
								break;
							}
							else {	
								rd = rs1 >> rs2; 
								break;
							}
					}					
				case 0x2: // slt
					rd = (rs1 < rs2)?1:0;
					break;
				case 0x3: // sltu
					rd = ((int64_t)((uint64_t)rs1 < (uint64_t)rs2))?1:0;
					break;

            }
	
            s->regs[rd_idx] = rd;
            break;
		}

		case 0b0010011:{ // I format arithmetic instructions
            int rd_idx = (ins >> 7) & 0b11111,
                funct3 = (ins >> 12) & 0b111,
                rs1_idx = (ins >> 15) & 0b11111,
                imm = ins >> 20;

			// Sign extension
			imm = imm | (0xfffff000 * (imm >> 11));

            int64_t rs1 = s->regs[rs1_idx], rd;
            
            switch(funct3) {
                case 0x0:
					rd = rs1 + imm;  // addi           
                    break;
                case 0x4: 
                    rd = rs1 ^ imm; //xori
                    break;
				case 0x6: // ori
					rd = rs1 | imm;
					break;
				case 0x7: // andi
					rd = rs1 & imm;
					break;
				case 0x1:{ // slli
					int imm_second6 = imm & 0b000000111111;
					rd = rs1 << (imm_second6);
					break;
				}
				case 0x5: {
					int imm_first6 = imm>>6;
					int imm_second6 = imm & 0b000000111111;
					switch(imm_first6){
                    	case 0x10: rd = rs1 >> imm_second6; break; // srai             
                    	case 0x00: //srli
							if (rs1>>63 == -1){ // if the leading digit is 1
								rd = (uint64_t) ((int64_t)(rs1) >> imm_second6);
								break;
							} else {	// if the leading digit is 0
								rd = rs1 >> imm_second6; 
								break;
							}
					}
					break;
				}					
				case 0x2: // slti
					rd = (rs1 < imm)?1:0; // rs1 and imm are signed integers by default
					break;
				case 0x3: // sltiu
					rd = ((uint64_t)rs1 < (uint32_t)imm)?1:0;
					break;

            }

            s->regs[rd_idx] = rd;
            break;
		}
		case 0b0000011:{ // I format load instructions
			ins = (int32_t) ins;
            int rd_idx = (ins >> 7) & 0b11111,
                funct3 = (ins >> 12) & 0b111,
                rs1_idx = (ins >> 15) & 0b11111,
                imm = (ins >> 20);

			// Sign extension
			imm = imm | (0xfffff000 * (imm >> 11));
   
            int64_t rs1 = s->regs[rs1_idx], rd;
			int64_t address = rs1 + imm;

            switch(funct3) {
                case 0x0:{ //lb
					int64_t mem_value = cache_read(s->cache, address, 1);
					int8_t eight_bit_num = (mem_value << 56) >> 56;
					rd = (int64_t) eight_bit_num;       
                    break;
				}
                case 0x1:{ // lh
					int64_t mem_value = cache_read(s->cache, address, 2);
                    int16_t sixteen_bit_num = (mem_value << 48) >> 48; 
					rd = (int64_t) sixteen_bit_num;
                    break;
				}
				case 0x2:{ // lw
					int64_t mem_value = cache_read(s->cache, address, 4);
                    int32_t thirtytwo_bit_num = (mem_value << 32) >> 32; 
					rd = (int64_t) thirtytwo_bit_num;
                    break;
				}
				case 0x3: // ld
					uint64_t mem_value = cache_read(s->cache, address, 8);
					rd = mem_value;
					break;
				case 0x4:{ // lbu
					uint64_t unsigned_mem_value = cache_read(s->cache, address, 1);
					uint8_t eight_bit_num = (unsigned_mem_value << 56) >> 56;
					rd = (int64_t) eight_bit_num;       
                    break;
				}
				case 0x5: { // lhu
					uint64_t unsigned_mem_value = cache_read(s->cache, address, 2);
					uint16_t sixteen_bit_num = (unsigned_mem_value << 48) >> 48;
					rd = (int64_t) sixteen_bit_num;       
                    break;
				}					
				case 0x6:{ // lwu
					uint64_t unsigned_mem_value = cache_read(s->cache, address, 4);
                    uint32_t thirtytwo_bit_num = (unsigned_mem_value << 32) >> 32; 
					rd = (int64_t) thirtytwo_bit_num;
                    break;
				}
            }
			
            s->regs[rd_idx] = rd;
            break;
		}
		case 0b0100011:{ // S format store instructions
			ins = (int32_t) ins;
            int funct3 = (ins >> 12) & 0b111,
                rs1_idx = (ins >> 15) & 0b11111,
				rs2_idx = (ins >> 20) & 0b11111,
                imm = (ins>>7 & 0b11111) + ((ins>>25)<<5); // making sure that the sign remains.

            int64_t rs1 = s->regs[rs1_idx], rs2 = s->regs[rs2_idx], rd;
			int64_t address = rs1 + imm;
            
            switch(funct3) {
                case 0x0:{ //sb
					*(uint8_t*)(&s->mem[address]) = (rs2 << 56)>>56; // i think this works just fine without the pointer typecasting
                    break;
				}
                case 0x1:{ // sh
					*(uint16_t*)(&s->mem[address]) = (rs2 << 48)>>48;
                    break;
				}
				case 0x2: // sw
					*(uint32_t*)(&s->mem[address]) = (rs2 << 32)>>32;
                    break;
				case 0x3: // sd
					*(uint64_t*)(&s->mem[address]) = rs2 ;
					break;
			}
            break;
		}
		case 0b1100011:{ // B format branch instructions
			ins = (int32_t) ins;
            int funct3 = (ins >> 12) & 0b111,
                rs1_idx = (ins >> 15) & 0b11111,
				rs2_idx = (ins >> 20) & 0b11111,
                imm;
			imm = ((ins>>31)<< 11) + // 12
            	(((ins>>7)& 0b1) << 10) + // 11
            	(((ins >>25) & 0b111111) << 4) + //10:5
            	((ins>>8) & 0b1111); // 4:1
			imm = imm << 1;
			imm = imm | (0xffffe000 * (imm >> 12)); // sign extension to 32 bits
            int64_t rs1 = s->regs[rs1_idx], rs2 = s->regs[rs2_idx], rd;
			int64_t address = rs1 + imm;

            switch(funct3) {
                case 0x0:{ //beq
					if (rs1 == rs2) s->pc += imm-4;
                    break;
				}
                case 0x1:{ // bne
					if (rs1 != rs2) s->pc += imm-4;
                    break;
				}
				case 0x4: // blt
					if (rs1 < rs2) s->pc += imm-4;
                    break;
				case 0x5: // bge
					if (rs1 >= rs2) s->pc += imm-4;
					break;
				case 0x6: // bltu
					if ((uint64_t)rs1 < (uint64_t)rs2) s->pc += imm-4;
					break;
				case 0x7: // bgeu
					if ((uint64_t)rs1 >= (uint64_t)rs2) s->pc += imm-4;
					break;
			}
            break;
		}

		case 0b0110111:{ // lui
			int rd_idx = (ins >> 7) & 0b11111;
			long int imm = (ins>>12);


			int64_t rd = ((int64_t)imm << 44) >> 32; // we have to do msb extend
			s->regs[rd_idx] = rd;
			break;
		}
		case 0b0010111:{ //auipc
			int rd_idx = (ins >> 7) & 0b11111,
				imm = (ins>>12);


			int64_t rd = (((int64_t)imm << 44) >> 32) + s->pc; // we have to do msb extend
			s->regs[rd_idx] = rd;
			break;
		}
		case 0b01101111:{ //jal
			int rd_idx = (ins >> 7) & 0b11111,
				imm;
			imm = ((ins>>31)<< 19) + // 20
            	(((ins>>12)& 0b11111111) << 11) + // 19:12
            	(((ins >>20) & 0b1) << 10) + //11
            	((ins>>21) & 0b1111111111); // 10:1
			imm = imm <<1; // note that imm is a signed immediate

			// Sign extension
			imm = imm | 0xffe00000 * (imm >> 20);

			int64_t rd = s->pc + 4;
			s->pc += imm - 4;
			s->regs[rd_idx] = rd;

			char *label;
			for (int i = 0; i < s->labels->len; i++) {
				if (s->labels->data[i].offset == s->pc+4) {
					label = s->labels->data[i].lbl_name;	
				}
			}

			sim_stack_push(s, label, 0);

			break;
		}
		case 0b1100111:{ //jalr
            int rd_idx = (ins >> 7) & 0b11111,
                funct3 = (ins >> 12) & 0b111,
                rs1_idx = (ins >> 15) & 0b11111,
                imm = ins >> 20;

            int64_t rs1 = s->regs[rs1_idx], rd;

			rd = s->pc + 4;
			s->pc = rs1 + imm - 4; // return address + immediate;

			s->regs[rd_idx] = rd;
			
			sim_stack_pop(s);
			
			break;
		}
    }
	
	// Force x0 to 0
	s->regs[0] = 0;
    s->pc += 4;
}

// Prints the values of rhe registers
void sim_regs(Simulator *s) {
    for (int i = 0; i <= 31; i++) {
        printf("x%d = 0x%lX \n", i, s->regs[i]);
    }
}

// Prints <count> bytes of memory, starting at address <start>
void sim_mem(Simulator *s, int start, int count){

	for (int i = 0; i < count; i++) {
		int addr = start + i;
		printf("Memory[0x%x] = 0x%X \n", addr, s->mem[addr]);
	}
}

// Get the line number of the nth instruction
int get_ins_line(Simulator *s, int n) {
	ParseNode *p = s->nodes;
	int i = 0;
	while (i < n) {
		if (p->type == LABEL) {p++; continue;}
		i++;
		p++;	
	}
	return (--p)->line;

}

// Executes one instruction
void sim_step(Simulator *s) {
	uint32_t ins = *(uint32_t*)(&s->mem[s->pc]);
	if (!ins) {
		printf("Nothing to step\n");
		return;
	};
	
	uint64_t pc = s->pc;
	int len = s->stack->len;
	sim_run_one(s);
	
	printf("Executed: ");
	int line = get_ins_line(s, pc/4+1);
	print_line(s->src, line);
	printf("; PC = 0x%lX\n", pc);

	// Update call stack
	s->stack->data[len-1].line = line;

	// Remove `main` from stack at end of code
	ins = *(uint32_t*)(&s->mem[s->pc]);
	if (!ins) {
		s->stack->len--;
	}
}

// Executes instructions intil EOF or until breakpoint
void sim_run(Simulator *s) {
    uint32_t ins = *(uint32_t*)(&s->mem[s->pc]);

	while (ins) {
		sim_step(s);
		ins = *(uint32_t*)(&s->mem[s->pc]);

		if (!ins) break;

		// Check if current line is a breakpoint
		int line = get_ins_line(s, s->pc/4+1);
		for (int i = 0; i < s->breaks->len; i++) {
			if (s->breaks->data[i] == line) {
				printf("Execution stopped at breakpoint\n");
				return;
			}
		}
	}

	print_cache_stats(s->cache);
} 

// Adds a breakpoint
void sim_add_breakpoint(Simulator *s, int line) {
	// If there is no space, then grow the breakpoint array
	if (s->breaks->len >= s->breaks->cap) {
		s->breaks->data = realloc(s->breaks->data, (s->breaks->cap + 1024) * sizeof(int));
		s->breaks->cap += 1024;
	}
	s->breaks->data[s->breaks->len++] = line;
	printf("Breakpoint set at line %d\n", line);
}

// Removes a breakpoint
void sim_remove_breakpoint(Simulator *s, int line) {
	int idx = 0, found = 0;;
	for (idx = 0; idx < s->breaks->len; idx++) {
		if (s->breaks->data[idx] == line) {
			found = 1;
			break;
		}
	}
	if (found) {
		s->breaks->data[idx] = s->breaks->data[--s->breaks->len];
		printf("Deleted breakpoint at line %d\n", line);
	} else {
		printf("No breakpoint at line %d\n", line);
	}
}

// Pushes a label and line onto the stack
void sim_stack_push(Simulator *s, char *label, int line) {
	if (s->stack->len >= s->stack->cap) {
		s->stack->data = realloc(s->stack->data, (s->stack->cap + 1024) * sizeof(StackEntry));
		s->stack->cap += 1024;
	}

	s->stack->data[s->stack->len].label = label;	
	s->stack->data[s->stack->len].line = line;
	s->stack->len++;
}

// Pops one entry from the top of the stack
void sim_stack_pop(Simulator *s) {
	s->stack->len--;
}

// Shows the stack
void sim_show_stack(Simulator *s) {
	if (!s->stack->len) {
		printf("Empty Call Stack: Execution complete\n");
	} else {
		printf("Call Stack:\n");
	}

	for (int i = 0; i < s->stack->len; i++) {
		printf("%s:%d\n", s->stack->data[i].label, s->stack->data[i].line);
	}	
}