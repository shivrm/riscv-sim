#ifndef PARSER_H
#include "parser.h"
#endif

#include <string.h>
#include <stdio.h>

#include "tables.c"

// Does `a` start with `b`?
// Assumes that a is longer than b
int startswith(char *a, char *b) {
	int i = 0;
	char c1 = a[i], c2 = b[i];
	while(c2) {
		if (c1 != c2) return 0;
		i++;
		c1 = a[i];
		c2 = b[i];
	}
	return 1;
}

// Converts a token type to a human readable string
// Used for error reporting
char *token_type_to_str(TokenType tt) {
	switch (tt) {
		case TOK_EOF:    return "EOF";
		case TOK_LPAREN: return "left paren `(`";
		case TOK_RPAREN: return "right paren `)`";
		case TOK_COMMA:  return "comma `,`";
		case TOK_COLON:  return "colon `:`";
		case TOK_DECNUM: return "decimal number";
		case TOK_BINNUM: return "binary number";
		case TOK_HEXNUM: return "hex number";
		case TOK_OCTNUM: return "octal number";
		case TOK_IDENT:  return "identifier";
		case TOK_DIRECTIVE: return "directive";
		default: return "token";
	}
}

// Initializes the parser
void parser_init(Parser *p, Lexer *l) {
	p->lexer = l;
	p->src = l->src;
	p->current = lexer_next(l);
	p->text_section = 1;
}

// Moves the parser to the next token
void parser_advance(Parser *p, ParseErr *err) {
	if (p->current.type == TOK_EOF) {
		err->is_err = 1;
		err->msg = "EOF while parsing";
		err->line = p->lexer->line;
		err->scol = p->current.span.start - p->lexer->lastline;
		err->ecol = p->current.span.end - p->lexer->lastline;
		return;
	}

	p->current = lexer_next(p->lexer);	

	// Handles syntax errors from the lexer
	if (p->current.type == TOK_ERR) {
		err->is_err = 1;
		err->msg = "Invalid syntax";	
		err->line = p->lexer->line;
		err->scol = p->current.span.start - p->lexer->lastline;
		err->ecol = p->current.span.end - p->lexer->lastline;
	}
}

// Advances the parser if (current token type) == tt
// Otherwise it throws an error
void parser_expect(Parser *p, TokenType tt, ParseErr *err) {
		TokenType cur = p->current.type;
		if (cur != tt) {
			err->is_err = 1; 
			err->msg = malloc(50 * sizeof(char));
			sprintf(err->msg, "Expected %s, got %s", token_type_to_str(tt), token_type_to_str(p->current.type));
			err->line = p->lexer->line;
			err->scol = p->current.span.start - p->lexer->lastline;
			err->ecol = p->current.span.end - p->lexer->lastline;
			return;
		}
        parser_advance(p, err);	
}

// Checks if the token test of the current token is equal to `str`
int parser_tteq(Parser *p, char *str) {
	return startswith(&p->src[p->current.span.start], str)
		&& (strlen(str) == (p->current.span.end - p->current.span.start));
}

// Parses a register into its register number (0-31)
int parse_register(Parser *p, ParseErr *err) {
	for (int i = 0; i < sizeof(reg_table)/sizeof(RegTableEntry); i++) {
		if (parser_tteq(p, reg_table[i].key)) {
			parser_advance(p, err);	
			return reg_table[i].value;
		}
	}

	// THrow an error if none of the register names match
	err->is_err = 1;
	err->msg = "Invalid register name";
	err->line = p->lexer->line;
	err->scol = p->current.span.start - p->lexer->lastline;
	err->ecol = p->current.span.end - p->lexer->lastline;
	return -1;
}

// Parses a number
int parse_number(Parser *p, ParseErr *err) {
    char *end;
	int n;

    switch (p->current.type) {
    case TOK_DECNUM:
        n = strtol(&p->src[p->current.span.start], &end, 10);
        parser_advance(p, err);
		return n;
    case TOK_HEXNUM:
        n = strtol(&p->src[p->current.span.start], &end, 16);
        parser_advance(p, err);
		return n;
	case TOK_OCTNUM:
        n = strtol(&p->src[p->current.span.start], &end, 8);
        parser_advance(p, err);
		return n;
	case TOK_BINNUM: ;
		// strtol doesn't handle `0b` syntax, so we parse the minus
		// sign ourselves and use strtol for everything after `0b`. 
		int negative = (p->src[p->current.span.start] == '-');	
        n = strtol(&p->src[p->current.span.start + 2 + negative], &end, 2);
        parser_advance(p, err);
        return negative? -n: n;
	default:
		err->is_err = 1;
		err->msg = "Invalid number";
		err->line = p->lexer->line;
		err->scol = p->current.span.start - p->lexer->lastline;
		err->ecol = p->current.span.end - p->lexer->lastline;
		return -1;
    }

}

// Parses a number or a label
NumOrLabel parse_number_or_label(Parser *p, ParseErr *err) {
    if (p->current.type == TOK_IDENT) {
		// Copy label text into a string
        int n = p->current.span.end - p->current.span.start;
        char *label = malloc((n+1) * sizeof(char));
        strncpy(label, &p->src[p->current.span.start], n);
        label[n] = '\0';

        NumOrLabel res = {
            1,
            { .l = {label} }
        };
        parser_advance(p, err);
        return res;
    } else {
        int n = parse_number(p, err);
		if (err->is_err) {
			err->msg = "Invalid number or label";
		}
        NumOrLabel res = {
            0,
            { .n = n }
        };
        return res;
    }
}

// Parses R-format instruction
RIns parse_r_ins(Parser *p, RInsTableEntry *entry, ParseErr *err) {
	RIns ins = { entry, 0, 0, 0 };

	ins.rd = parse_register(p, err);
	if (err->is_err) return ins;
	
	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;
	
	ins.rs1 = parse_register(p, err);
	if (err->is_err) return ins;
	
	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;
	
	ins.rs2 = parse_register(p, err);
	if (err->is_err) return ins;
	return ins;
}

// Parses I-format arithmetic instruction
IIns parse_i_ins(Parser *p, IInsTableEntry *entry, ParseErr *err) {
	IIns ins = { entry, 0, 0, 0 };
	
	ins.rd = parse_register(p, err);
	if (err->is_err) return ins;

	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;

	ins.rs1 = parse_register(p, err);
	if (err->is_err) return ins;

	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;

	ins.imm = parse_number_or_label(p, err);
	if (err->is_err) return ins;
 
    return ins;
}

// Parses I-format load/jump instruction
IIns parse_i_ins_2(Parser *p, IInsTableEntry *entry, ParseErr *err) {
	IIns ins = { entry, 0, 0, 0 };
    ins.rd = parse_register(p, err);
	if (err->is_err) return ins;

    parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;

    ins.imm = parse_number_or_label(p, err);
	if (err->is_err) return ins;

    parser_expect(p, TOK_LPAREN, err);
	if (err->is_err) return ins;

    ins.rs1 = parse_register(p, err);
	if (err->is_err) return ins;

    parser_expect(p, TOK_RPAREN, err);
	if (err->is_err) return ins;
    
	return ins;    
}

// Parses S-format instruction
SIns parse_s_ins(Parser *p, SInsTableEntry *entry, ParseErr *err) {
	SIns ins = { entry, 0, 0, 0 };

	ins.rs2 = parse_register(p, err);
	if (err->is_err) return ins;
    
    parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;
    
    ins.imm = parse_number_or_label(p, err);
	if (err->is_err) return ins;
    
    parser_expect(p, TOK_LPAREN, err);
	if (err->is_err) return ins;
    
    ins.rs1 = parse_register(p, err);
	if (err->is_err) return ins;
    
    parser_expect(p, TOK_RPAREN, err);
	if (err->is_err) return ins;
    
    return ins;
}

// Parses B-format instruction
BIns parse_b_ins(Parser *p, BInsTableEntry *entry, ParseErr *err) {
	BIns ins = { entry, 0, 0, 0 };

	ins.rs1 = parse_register(p, err);
	if (err->is_err) return ins;
    
	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;
    
	ins.rs2 = parse_register(p, err);
	if (err->is_err) return ins;
    
	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;
    
    ins.imm = parse_number_or_label(p, err);
	if (err->is_err) return ins;
    
    return ins;
}

// Parses U-format instruction
UIns parse_u_ins(Parser *p, UInsTableEntry *entry, ParseErr *err) {
	UIns ins = { entry, 0, 0 };
	
	ins.rd = parse_register(p, err);
	if (err->is_err) return ins;
    
	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;
    
    ins.imm = parse_number_or_label(p, err);
	if (err->is_err) return ins;
    
    return ins;
}

// Parses J-format instruction
JIns parse_j_ins(Parser *p, JInsTableEntry *entry, ParseErr *err) {
	JIns ins = { entry, 0, 0 };
	
	ins.rd = parse_register(p, err);
	if (err->is_err) return ins;
	
	parser_expect(p, TOK_COMMA, err);
	if (err->is_err) return ins;
 
	ins.imm = parse_number_or_label(p, err);
	if (err->is_err) return ins;
 
    return ins;
}

ParseNode parse_text_element(Parser *p, ParseErr *err) {	
	ParseNode sentinel = {0}; // Zero object to be returned in case of error

	// Checks if instruction is R-format and parses
	for (int i = 0; i < sizeof(r_ins_table)/sizeof(RInsTableEntry); i++) {
		if (parser_tteq(p, r_ins_table[i].key)) {
			parser_advance(p, err);
			if (err->is_err) return sentinel;
			ParseNode node = {
				R_INS,
				{ .r = parse_r_ins(p, &r_ins_table[i], err) },
				p->lexer->line - 1,
			};
			return node;
		}
	}
	
	// Checks if instruction is I-format arithmetic and parses
	for (int i = 0; i < sizeof(i_ins_table)/sizeof(IInsTableEntry); i++) {
		if (parser_tteq(p, i_ins_table[i].key)) {
			parser_advance(p, err);
			if (err->is_err) return sentinel;
			ParseNode node = {
				I_INS,
				{ .i = parse_i_ins(p, &i_ins_table[i], err) },
				p->lexer->line - 1,
			};
			return node;
		}
	}

	// Checks if instruction is I-format memory/jump and parses.
    for (int i = 0; i < sizeof(i_ins_table_2)/sizeof(IInsTableEntry); i++) {
		if (parser_tteq(p, i_ins_table_2[i].key)) {
			parser_advance(p, err);
			if (err->is_err) return sentinel;
			ParseNode node = {
				I_INS,
				{ .i = parse_i_ins_2(p, &i_ins_table_2[i], err) },
				p->lexer->line - 1,
			};
			return node;
		}
	}

	// Checks if instruction is S-format and parses
	for (int i = 0; i < sizeof(s_ins_table)/sizeof(SInsTableEntry); i++) {
		if (parser_tteq(p, s_ins_table[i].key)) {
			parser_advance(p, err);
			if (err->is_err) return sentinel;
			ParseNode node = {
				S_INS,
				{ .s = parse_s_ins(p, &s_ins_table[i], err) },
				p->lexer->line - 1,
			};
			return node;
		}
	}

	// Checks if instruction is B-format and parses
	for (int i = 0; i < sizeof(b_ins_table)/sizeof(BInsTableEntry); i++) {
		if (parser_tteq(p, b_ins_table[i].key)) {
			parser_advance(p, err);
			if (err->is_err) return sentinel;
			ParseNode node = {
				B_INS,
				{ .b = parse_b_ins(p, &b_ins_table[i], err) },
				p->lexer->line - 1,
			};
			return node;
		}
	}

	// Checks if instruction is U-format and parses
	for (int i = 0; i < sizeof(u_ins_table)/sizeof(UInsTableEntry); i++) {
		if (parser_tteq(p, u_ins_table[i].key)) {
			parser_advance(p, err);
			if (err->is_err) return sentinel;
			ParseNode node = {
				U_INS,
				{ .u = parse_u_ins(p, &u_ins_table[i], err) },
				p->lexer->line - 1,
			};
			return node;
		}
	}

	// Checks if instruction is J-format and parses
	for (int i = 0; i < sizeof(j_ins_table)/sizeof(JInsTableEntry); i++) {
		if (parser_tteq(p, j_ins_table[i].key)) {
			parser_advance(p, err);
			if (err->is_err) return sentinel;
			ParseNode node = {
				J_INS,
				{ .j = parse_j_ins(p, &j_ins_table[i], err) },
				p->lexer->line - 1,
			};
			return node;
		}
	}

	// Token might be a label. Copies text into a string
    int n = p->current.span.end - p->current.span.start;
    char *label = malloc((n+1) * sizeof(char));
    strncpy(label, &p->src[p->current.span.start], n);
    label[n] = '\0';
 
    parser_advance(p, err);
	if (err->is_err) return sentinel;

	// Checks for colon after label
    parser_expect(p, TOK_COLON, err);

	// No colon means the earlier token did not belong 
	// to a label. Raise an unknown instruction error.
	if (err->is_err) {
		err->msg = "Unknown instruction";
		return sentinel;
	}
 
	// Otherwise return the label
    ParseNode node = {
        LABEL,
        { .l = {label} },
		p->lexer->line - 1,
    };
    return node;
}

void parse_data_element(Parser *p, DataVec *d, ParseErr *err) {
	if (parser_tteq(p, ".word")) {
		parser_advance(p, err);
		if (err->is_err) return;

		while (1) {
			switch (p->current.type) {
			
			case TOK_BINNUM:
			case TOK_HEXNUM:
			case TOK_OCTNUM:
			case TOK_DECNUM: ;
				int n = parse_number(p, err);
				if (err->is_err) return;

				if (d->len - d->cap < 4) {
					d->data = realloc(d->data, (d->cap + 1024) * sizeof(uint8_t));
					d->cap += 1024;			
				}

				*(uint32_t*)(&d->data[d->len]) = n;
				d->len += 4;
				return;
			case TOK_COMMA:
				parser_advance(p, err);
				if (err->is_err) return;
			default: 
				err->is_err = 1;
				err->line = p->lexer->line;
				err->msg = "Unknown token"; 
				return;	
			}
		}
	} else {
		err->is_err = 1;
		err->line = p->lexer->line;
		err->msg = "Unknown token"; 
		return;
	}
}

void parse_one(Parser *p, ParseNodeVec *pn, DataVec *d, ParseErr *err) {
	if (p->current.type == TOK_DIRECTIVE) {
		if (parser_tteq(p, ".data")) {
			parser_advance(p, err);
			p->text_section = 0;
			parse_one(p, pn, d, err);
			return;
		} else if (parser_tteq(p, ".text")) {
			parser_advance(p, err);
			p->text_section = 1;
			parse_one(p, pn, d, err);
			return;
		}
	}

	if (p->text_section) {
		if (pn->cap == pn->len) {
            pn->data = realloc(pn->data, (pn->cap + 1024) * sizeof(ParseNode));
            pn->cap += 1024;
		}

		pn->data[pn->len++] = parse_text_element(p, err);
		return;
	} else {
		parse_data_element(p, d, err);
		return;
	}
}

void parse_all(Parser *p, ParseNodeVec *pn, DataVec *d, ParseErr *err) {
	while (1) {
		parse_one(p, pn, d, err);
		if (err->is_err) {
			if (strcmp(err->msg, "EOF while parsing") == 0) {
				pn->len--;
				err->is_err = 0;
			}
			return;
		}
	}
}