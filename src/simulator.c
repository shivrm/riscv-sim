#include <stdio.h>
#include <stdint.h>
#include <string.h>

#ifndef SIMULATOR_H
#include "simulator.h"
#endif

#define BUF_CHUNK_SIZE 4096
#define PN_CHUNK_SIZE 4096

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
	printf("%.*s\n", end - start, start);
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
	printf("%.*s\n", end - start, start);
}


void sim_init(Simulator *s) {
   // TODO 
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

	// Dynamic array for storing arbitrary number of nodes
	size_t len = 0, cap = 0;
	ParseNode *nodes = NULL;

	while(1) {
		ParseNode pn = parser_next(&p, &err);

		// This error occurs when we already reached the EOF after
		// the last node, but not when we are in the middle of parsing
		// an instruction.
		// So it's used as a signal to indicate that parsing completed
		// successfully.
		if (err.is_err) {
			if (strcmp(err.msg, "EOF while parsing") == 0) break;
			print_parse_error(src, &err);
			return 1;
		}

		// If capacity is reached, then grow the array
		if (len == cap) {
			nodes = realloc(nodes, (cap + PN_CHUNK_SIZE) * sizeof(ParseNode));
			cap += PN_CHUNK_SIZE;
		}

		nodes[len++] = pn;	
	}

	EmitErr err2 = {0, "", 0};
    LabelVec labels = find_labels(nodes, len, &err2);
	if (err2.is_err) {
		print_emit_error(src, &err2);
		return -1;
	}

	emit_all(s->mem, nodes, len, labels, &err2);
	if (err2.is_err) {
		print_emit_error(src, &err2);
		return -1;
	}

    s->src = src;
    s->nodes = nodes;  
}

void sim_run(Simulator *s) {
    // Read 32-bits from index `pc` of memory
    uint32_t ins = *(uint32_t*)(&s->mem[s->pc]);

    int opcode = ins & 0b1111111;

    switch (opcode) {
        case 0b0110011:{// R format
            int rd_idx = (ins >> 7) & 0b11111;
            int funct3 = (ins >> 12) & 0b111;
            int rs1_idx = (ins >> 15) & 0b11111;
            int rs2_idx = (ins >> 20) & 0b11111;
            int funct7 = (ins >> 25) & 0b1111111;

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
					switch(funct7){
                    	case 0x20: rd = rs1 >> rs2; break; // sra                   
                    	case 0x00: //srl
							if (rs1>>63 == -1){ // if the first bit is -1
								rd = (int64_t) ((u_int64_t)(rs1) >> (u_int64_t)(rs2));
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
					rd = ((int64_t)((u_int64_t)rs1 < (u_int64_t)rs2))?1:0;
					break;

            }
	
            s->regs[rd_idx] = rd;
            break;
		}

		case 0b0010011:{ // I format arithmetic instructions
            int rd_idx = (ins >> 7) & 0b11111,
                funct3 = (ins >> 12) & 0b111,
                rs1_idx = (ins >> 15) & 0b11111,
                imm = (ins >> 20) & 0b111111111111; // to make it a 12-bit number, shouldn't make a difference

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
								rd = (int64_t) ((u_int64_t)(rs1) >> imm_second6);
								break;
							}
							else {	// if the leading digit is 0
								rd = rs1 >> imm_second6; 
								break;
							}
					}
				}					
				case 0x2: // slti
					rd = (rs1 < imm)?1:0; // rs1 and imm are signed integers by default
					break;
				case 0x3: // sltiu
					rd = ((u_int64_t)rs1 < imm)?1:0;
					break;

            }

            s->regs[rd_idx] = rd;
            break;
		}
		case 0b0000011:{ // I format load instructions
            int rd_idx = (ins >> 7) & 0b11111,
                funct3 = (ins >> 12) & 0b111,
                rs1_idx = (ins >> 15) & 0b11111,
                imm = (ins >> 20) & 0b111111111111;

            int64_t rs1 = s->regs[rs1_idx], rd;
			int64_t address = rs1 + imm;
			int64_t mem_value = *(int64_t*)(&s->mem[address]); // take out the entire 64 bit value
			u_int64_t unsigned_mem_value = (u_int64_t) mem_value;
            
            switch(funct3) {
                case 0x0:{ //lb
					int8_t eight_bit_num = (mem_value << 56) >> 56;
					rd = (int64_t) eight_bit_num;       
                    break;
				}
                case 0x1:{ // lh
                    int16_t sixteen_bit_num = (mem_value << 48) >> 48; 
					rd = (int64_t) sixteen_bit_num;
                    break;
				}
				case 0x2: // lw
                    int32_t thirtytwo_bit_num = (mem_value << 32) >> 32; 
					rd = (int64_t) thirtytwo_bit_num;
                    break;
				case 0x3: // ld
					rd = mem_value;
					break;
				case 0x4:{ // lbu
					u_int8_t eight_bit_num = (unsigned_mem_value << 56) >> 56;
					rd = (int64_t) eight_bit_num;       
                    break;
				}
				case 0x5: { // lhu
					u_int16_t sixteen_bit_num = (unsigned_mem_value << 48) >> 48;
					rd = (int64_t) sixteen_bit_num;       
                    break;
				}					
				case 0x6:{ // lwu
                    u_int32_t thirtytwo_bit_num = (unsigned_mem_value << 32) >> 32; 
					rd = (int64_t) thirtytwo_bit_num;
                    break;
				}
            }
			
            s->regs[rd_idx] = rd;
            break;
		}
    }

    s->pc += 4;
}