#include <stdio.h>
#include <string.h>
#ifndef EMITTER_H
#include "emitter.h"
#endif

int get_label_pos(LabelVec labels, Label label) {
    for (int i = 0; i < labels.len; i++) {
        if (strcmp(labels.data[i].lbl_name, label.name) == 0) {
            return labels.data[i].offset;
        }
    }
    printf("Label not found: %s\n", label.name);
    return -1;
}

void emit_ins(FILE *f, ParseNode *p, LabelVec labels, int pc) {
    int hex, imm;
    switch (p->type) {
        case R_INS:
            hex = (p->data.r.entry->funct7 << 25)
                        | (p->data.r.rs2 << 20)
                        | (p->data.r.rs1 << 15)
                        | (p->data.r.entry->funct3 << 12)
                        | (p->data.r.rd << 7)
                        | (p->data.r.entry->opcode);

            fputc(hex & 0xff, f);
            fputc((hex >> 8) & 0xff, f);
            fputc((hex >> 16) & 0xff, f);
            fputc((hex >> 24) & 0xff, f);
            break;
        
        case I_INS:
            imm = 0;
            if (p->data.i.imm.is_label) {
                imm = get_label_pos(labels, p->data.i.imm.data.l);
            } else {
                imm = p->data.i.imm.data.n;
            }

            hex = (imm << 20)
                        | (p->data.i.rs1 << 15)
                        | (p->data.i.entry->funct3 << 12)
                        | (p->data.i.rd << 7)
                        | (p->data.i.entry->opcode);


            fputc(hex & 0xff, f);
            fputc((hex >> 8) & 0xff, f);
            fputc((hex >> 16) & 0xff, f);
            fputc((hex >> 24) & 0xff, f);
            break;

        case S_INS:
            imm = 0;
            if (p->data.s.imm.is_label) {
                imm = get_label_pos(labels, p->data.s.imm.data.l);
            } else {
                imm = p->data.s.imm.data.n;
            }
            
            hex =  ((imm >> 5 & 0b1111111) << 25)
                    | (p->data.s.rs2 << 20)
                    | (p->data.s.rs1 << 15)
                    | (p->data.s.entry->funct3 << 12)
                    | ((imm & 0b11111) << 8)
                    | (p->data.s.entry->opcode);
    
            fputc(hex & 0xff, f);
            fputc((hex >> 8) & 0xff, f);
            fputc((hex >> 16) & 0xff, f);
            fputc((hex >> 24) & 0xff, f);
            break;

        case B_INS:
            if (p->data.b.imm.is_label) {
                imm = get_label_pos(labels, p->data.b.imm.data.l) - pc;
            } else {
                imm = p->data.b.imm.data.n;
            }
            
            hex = ((imm >> 12 & 1) << 31)
                    | ((imm >> 5 & 0b111111) << 25)
                    | (p->data.b.rs2 << 20)
                    | (p->data.b.rs1 << 15)
                    | (p->data.b.entry->funct3 << 12)
                    | ((imm >> 1 & 0b1111) << 8)
                    | ((imm >> 11 & 1) << 7)
                    | (p->data.b.entry->opcode);
    
            fputc(hex & 0xff, f);
            fputc((hex >> 8) & 0xff, f);
            fputc((hex >> 16) & 0xff, f);
            fputc((hex >> 24) & 0xff, f);
            break;

        case U_INS:
            if (p->data.u.imm.is_label) {
                imm = get_label_pos(labels, p->data.u.imm.data.l);
            } else {
                imm = p->data.u.imm.data.n;
            }

            hex = (imm << 12)
                    | (p->data.u.rd << 7)
                    | (p->data.u.entry->opcode);

            fputc(hex & 0xff, f);
            fputc((hex >> 8) & 0xff, f);
            fputc((hex >> 16) & 0xff, f);
            fputc((hex >> 24) & 0xff, f);
            break;
        
        case J_INS:
            if (p->data.s.imm.is_label) {
                imm = get_label_pos(labels, p->data.b.imm.data.l) - pc;
            } else {
                imm = p->data.b.imm.data.n;
            }

            hex = ((imm >> 20 & 1) << 31)
                    | ((imm >> 1 & 0x3ff) << 21)
                    | ((imm >> 11 & 1) << 20)
                    | ((imm >> 12 & 0xff) << 12)
                    | (p->data.u.rd << 7)
                    | (p->data.u.entry->opcode);

            fputc(hex & 0xff, f);
            fputc((hex >> 8) & 0xff, f);
            fputc((hex >> 16) & 0xff, f);
            fputc((hex >> 24) & 0xff, f);
            break;
    }
}

void emit_all(FILE *f, ParseNode p[], int num_nodes) {
    LabelEntry le[100] = {};
    LabelVec labels = {0, 100, le}; 

    int offset = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (p[i].type == LABEL) {
            labels.data[labels.len].lbl_name = p[i].data.l.name;
            labels.data[labels.len++].offset = offset;
        } else {
            offset += 4;
        }
    }

    int pc = 0;
    for (int i = 0; i < num_nodes; i++) {
        if (p[i].type != LABEL) {
            emit_ins(f, &p[i], labels, pc);
            pc += 4;
        }
    }
}