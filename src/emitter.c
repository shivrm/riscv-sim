#include <stdio.h>
#include <string.h>

#ifndef EMITTER_H
#include "emitter.h"
#endif

#define LE_CHUNK_SIZE 4096

// Returns the address that a label points to
// Throws an error if label is not found.
int get_label_pos(LabelVec labels, Label label, EmitErr *err) {
    for (int i = 0; i < labels.len; i++) {
        if (strcmp(labels.data[i].lbl_name, label.name) == 0) {
            return labels.data[i].offset;
        }
    }

    err->is_err = 1;
    err->msg = malloc(100 * sizeof(char));
    sprintf(err->msg, "Label not found: %s", label.name);
    return -1;
}

// Emits the hex code corresponding to an instruction
// to a file.
void emit_ins(FILE *f, ParseNode *p, LabelVec labels, int pc, EmitErr *err) {
    int hex, imm;
    switch (p->type) {
        case R_INS:
            hex = (p->data.r.entry->funct7 << 25)
                        | (p->data.r.rs2 << 20)
                        | (p->data.r.rs1 << 15)
                        | (p->data.r.entry->funct3 << 12)
                        | (p->data.r.rd << 7)
                        | (p->data.r.entry->opcode);

            break;
        
        case I_INS:
            // If immediate is a label, resolve its address
            if (p->data.i.imm.is_label) {
                imm = get_label_pos(labels, p->data.i.imm.data.l, err);
                if (err->is_err) {
                    err->line = p->line;    
                    return;
                }
            } else {
                imm = p->data.i.imm.data.n;
            }

            char *ins_name = p->data.i.entry->key;

            // Immediate bounds checking 
            if (imm < -2048 || imm > 2047) {
                err->is_err = 1;
                err->msg = "Immediate does not fit in 12 bits";
                err->line = p->line;
                return;
            }

            // Immediates for shift instructions should fit in 6 bits
            int is_shift = (strcmp(ins_name, "slli")==0) || (strcmp(ins_name, "srli")==0) || (strcmp(ins_name, "srai")==0);
            if (is_shift && (imm < 0 || imm > 63)) {
                err->is_err = 1;
                err->msg = "Immediate does not fit in 6 bits";
                err->line = p->line;
                return;
            }

            // funct6 value for srai
            if (strcmp(ins_name, "srai") == 0) {
                imm |= 0x400;
            }

            hex = (imm << 20)
                        | (p->data.i.rs1 << 15)
                        | (p->data.i.entry->funct3 << 12)
                        | (p->data.i.rd << 7)
                        | (p->data.i.entry->opcode);

            break;

        case S_INS:
            // If immediate is a label, resolve its address
            if (p->data.s.imm.is_label) {
                imm = get_label_pos(labels, p->data.s.imm.data.l, err);
                if (err->is_err) {
                    err->line = p->line;    
                    return;
                }
            } else {
                imm = p->data.s.imm.data.n;
            }

            // Bounds checking for immediate 
            if (imm < -2048 || imm > 2047) {
                err->is_err = 1;
                err->msg = "Immediate does not fit in 12 bits";
                err->line = p->line;
                return;
            }
            
            hex =  ((imm >> 5 & 0b1111111) << 25)
                    | (p->data.s.rs2 << 20)
                    | (p->data.s.rs1 << 15)
                    | (p->data.s.entry->funct3 << 12)
                    | ((imm & 0b11111) << 7)
                    | (p->data.s.entry->opcode);
            break;

        case B_INS:
            // If immediate is a label, resolve its address
            if (p->data.b.imm.is_label) {
                imm = get_label_pos(labels, p->data.b.imm.data.l, err) - pc;
                if (err->is_err) {
                    err->line = p->line;    
                    return;
                }
            } else {
                imm = p->data.b.imm.data.n;
            }

            // Immediates for B-format instructions are
            // assumed to be even.
            if (imm % 2) {
                err->is_err = 1;
                err->msg = "Immediate is not an even number";
                err->line = p->line;
                return;
            }

            // Bounds checking for immediate 
            if (imm < -4096 || imm > 4094) {
                err->is_err = 1;
                err->msg = "Immediate does not fit in 12 bits";
                err->line = p->line;
                return;
            }
            
            hex = ((imm >> 12 & 1) << 31)
                    | ((imm >> 5 & 0b111111) << 25)
                    | (p->data.b.rs2 << 20)
                    | (p->data.b.rs1 << 15)
                    | (p->data.b.entry->funct3 << 12)
                    | ((imm >> 1 & 0b1111) << 8)
                    | ((imm >> 11 & 1) << 7)
                    | (p->data.b.entry->opcode);
            break;

        case U_INS:
            // If immediate is a label, resolve its address
            if (p->data.u.imm.is_label) {
                imm = get_label_pos(labels, p->data.u.imm.data.l, err);
                if (err->is_err) {
                    err->line = p->line;    
                    return;
                }
            } else {
                imm = p->data.u.imm.data.n;
            }

            // Bounds checking for immediate 
            if (imm < 0 || imm > (1<<20)-1) {
                err->is_err = 1;
                err->msg = "Immediate does not fit in 20 bits";
                err->line = p->line;
                return;
            }

            hex = (imm << 12)
                    | (p->data.u.rd << 7)
                    | (p->data.u.entry->opcode);
            break;
        
        case J_INS:
            // If immediate is a label, resolve its address
            if (p->data.s.imm.is_label) {
                imm = get_label_pos(labels, p->data.b.imm.data.l, err) - pc;
                if (err->is_err) {
                    err->line = p->line;    
                    return;
                }
            } else {
                imm = p->data.b.imm.data.n;
            }
            
            // Bounds checking for immediate 
            if (imm < -(1<<21) || imm > (1<<21)-2) {
                err->is_err = 1;
                err->msg = "Immediate does not fit in 20 bits";
                err->line = p->line;
                return;
            }

            hex = ((imm >> 20 & 1) << 31)
                    | ((imm >> 1 & 0x3ff) << 21)
                    | ((imm >> 11 & 1) << 20)
                    | ((imm >> 12 & 0xff) << 12)
                    | (p->data.u.rd << 7)
                    | (p->data.u.entry->opcode);
            break;
    }

    // Prints the hex representation to the file
    fprintf(f, "%08x\n", hex);
}

// Enumerates 
void emit_all(FILE *f, ParseNode p[], int num_nodes, EmitErr *err) {

    // Enumerates labels
    LabelVec labels = {0, 0, NULL}; 
    labels.data = malloc(LE_CHUNK_SIZE * sizeof(LabelEntry));
    labels.cap = LE_CHUNK_SIZE;
    
    int offset = 0;
    for (int i = 0; i < num_nodes; i++) {
        // Resize label vec if out of space
        if (labels.cap == labels.len) {
            labels.data = realloc(labels.data, (labels.cap + LE_CHUNK_SIZE) * sizeof(LabelEntry));
            labels.cap += LE_CHUNK_SIZE;
        }

        if (p[i].type == LABEL) {
            for (int j = 0; j < labels.len; j++) {
                if (strcmp(labels.data[j].lbl_name, p[i].data.l.name) == 0) {
                    err->is_err = 1;
                    err->msg = "Duplicate definition of label";
                    err->line = p[i].line;
                    return;
                }
            }

            labels.data[labels.len].lbl_name = p[i].data.l.name;
            labels.data[labels.len++].offset = offset;
        } else {
            offset += 4;
        }
    }

    // Check for label at end of file, not followed by an instruction
    if (labels.data[labels.len-1].offset == offset) {
        err->is_err = 1;
        err->msg = "Label without instruction";
        err->line = p[num_nodes-1].line;
        return;
    }

    // Emit instructions
    int pc = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (p[i].type != LABEL) {
            emit_ins(f, &p[i], labels, pc, err);
            pc += 4;
        }
    }
}