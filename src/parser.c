#ifndef PARSER_H
#include "parser.h"
#endif

#include <string.h>
#include <stdio.h>

#include "tables.c"

void parser_init(Parser *p, Lexer *l) {
	p->lexer = l;
	p->src = l->src;
	p->current = lexer_next(l);
}

void parser_advance(Parser *p) {
	p->current = lexer_next(p->lexer);	
}

int parser_expect(Parser *p, TokenType tt) {
		TokenType cur = p->current.type;
		parser_advance(p);	
		return (cur == tt);	
}

int parser_tteq(Parser *p, char *str) {
	return startswith(&p->src[p->current.span.start], str) && (strlen(str) == (p->current.span.end - p->current.span.start));
}

int parse_register(Parser *p) {
	for (int i = 0; i < 64; i++) {
		if (parser_tteq(p, reg_table[i].key)) {
			parser_advance(p);
			return reg_table[i].value;
		}
	}
}

int parse_number(Parser *p) {
    char *end;
    int n;
    switch (p->current.type) {
    case TOK_DECNUM:
        n = strtol(&p->src[p->current.span.start], &end, 10);
        parser_advance(p);
        return n;
    case TOK_HEXNUM:
        n = strtol(&p->src[p->current.span.start], &end, 16);
        parser_advance(p);
        return n;
	case TOK_OCTNUM:
        n = strtol(&p->src[p->current.span.start], &end, 8);
        parser_advance(p);
        return n;
    case TOK_BINNUM:
        n = strtol(&p->src[p->current.span.start], &end, 2);
        parser_advance(p);
        return n;
    }
}

NumOrLabel parse_number_or_label(Parser *p) {
    if (p->current.type == TOK_IDENT) {
        char *label = malloc(100 * sizeof(char));
        int n = p->current.span.end - p->current.span.start;
        strncpy(label, &p->src[p->current.span.start], n);
        label[n] = '\0';
        NumOrLabel res = {
            1,
            { .l = {label} }
        };
        parser_advance(p);
        return res;
    } else {
        int n = parse_number(p);
        NumOrLabel res = {
            0,
            { .n = n }
        };
        return res;
    }
}

RIns parse_r_ins(Parser *p, RInsTableEntry *entry) {
	int rd = parse_register(p);
	parser_expect(p, TOK_COMMA);
	int rs1 = parse_register(p);
	parser_expect(p, TOK_COMMA);
	int rs2 = parse_register(p);
	RIns ins = {
		entry, rd, rs1, rs2
	};
	return ins;
} 

IIns parse_i_ins(Parser *p, IInsTableEntry *entry) {
	int rd = parse_register(p);
	parser_expect(p, TOK_COMMA);
	int rs1 = parse_register(p);
	parser_expect(p, TOK_COMMA);
	NumOrLabel imm = parse_number_or_label(p);
    IIns ins = {
        entry, rd, rs1, imm
    };
    return ins;
}

IIns parse_i_ins_2(Parser *p, IInsTableEntry *entry) {
    int rd = parse_register(p);
    parser_expect(p, TOK_COMMA);
    NumOrLabel imm = parse_number_or_label(p);
    parser_expect(p, TOK_LPAREN);
    int rs1 = parse_register(p);
    parser_expect(p, TOK_RPAREN);
    IIns ins = {
        entry, rd, rs1, imm
    };
    return ins;    
}

SIns parse_s_ins(Parser *p, SInsTableEntry *entry) {
    int rs2 = parse_register(p);
    parser_expect(p, TOK_COMMA);
    NumOrLabel imm = parse_number_or_label(p);
    parser_expect(p, TOK_LPAREN);
    int rs1 = parse_register(p);
    parser_expect(p, TOK_RPAREN);
    SIns ins = {
        entry, rs1, rs2, imm
    };
    return ins;
}

BIns parse_b_ins(Parser *p, BInsTableEntry *entry) {
	int rs1 = parse_register(p);
	parser_expect(p, TOK_COMMA);
	int rs2 = parse_register(p);
	parser_expect(p, TOK_COMMA);
    NumOrLabel imm = parse_number_or_label(p);
    BIns ins = {
        entry, rs1, rs2, imm
    };
    return ins;
}

UIns parse_u_ins(Parser *p, UInsTableEntry *entry) {
	int rd = parse_register(p);
	parser_expect(p, TOK_COMMA);
    NumOrLabel imm = parse_number_or_label(p);
    UIns ins = {
        entry, rd, imm
    };
    return ins;
}

JIns parse_j_ins(Parser *p, JInsTableEntry *entry) {
	int rd = parse_register(p);
	parser_expect(p, TOK_COMMA);
    NumOrLabel imm = parse_number_or_label(p);
    JIns ins = {
        entry, rd, imm
    };
    return ins;
}

ParseNode parser_next(Parser *p) {	
	for (int i = 0; i < sizeof(r_ins_table)/sizeof(RInsTableEntry); i++) {
		if (parser_tteq(p, r_ins_table[i].key)) {
			parser_advance(p);
			ParseNode node = {
				R_INS,
				{ .r = parse_r_ins(p, &r_ins_table[i]) }
			};
			return node;
		}
	}
	for (int i = 0; i < sizeof(i_ins_table)/sizeof(IInsTableEntry); i++) {
		if (parser_tteq(p, i_ins_table[i].key)) {
			parser_advance(p);
			ParseNode node = {
				I_INS,
				{ .i = parse_i_ins(p, &i_ins_table[i]) }
			};
			return node;
		}
	}
    for (int i = 0; i < sizeof(i_ins_table_2)/sizeof(IInsTableEntry); i++) {
		if (parser_tteq(p, i_ins_table_2[i].key)) {
			parser_advance(p);
			ParseNode node = {
				I_INS,
				{ .i = parse_i_ins_2(p, &i_ins_table_2[i]) }
			};
			return node;
		}
	}
	for (int i = 0; i < sizeof(s_ins_table)/sizeof(SInsTableEntry); i++) {
		if (parser_tteq(p, s_ins_table[i].key)) {
			parser_advance(p);
			ParseNode node = {
				S_INS,
				{ .s = parse_s_ins(p, &s_ins_table[i]) }
			};
			return node;
		}
	}
	for (int i = 0; i < sizeof(b_ins_table)/sizeof(BInsTableEntry); i++) {
		if (parser_tteq(p, b_ins_table[i].key)) {
			parser_advance(p);
			ParseNode node = {
				B_INS,
				{ .b = parse_b_ins(p, &b_ins_table[i]) }
			};
			return node;
		}
	}
	for (int i = 0; i < sizeof(u_ins_table)/sizeof(UInsTableEntry); i++) {
		if (parser_tteq(p, i_ins_table[i].key)) {
			parser_advance(p);
			ParseNode node = {
				U_INS,
				{ .u = parse_u_ins(p, &u_ins_table[i]) }
			};
			return node;
		}
	}
	for (int i = 0; i < sizeof(j_ins_table)/sizeof(JInsTableEntry); i++) {
		if (parser_tteq(p, j_ins_table[i].key)) {
			parser_advance(p);
			ParseNode node = {
				J_INS,
				{ .j = parse_j_ins(p, &j_ins_table[i]) }
			};
			return node;
		}
	}
    
    char *label = malloc(100 * sizeof(char));
    int n = p->current.span.end - p->current.span.start;
    strncpy(label, &p->src[p->current.span.start], n);
    label[n] = '\0';
    parser_advance(p);

    parser_expect(p, TOK_COLON);
    ParseNode node = {
        LABEL,
        { .l = {label} }
    };
    return node;
}
