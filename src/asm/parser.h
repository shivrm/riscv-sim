#include <stdint.h>

#define PARSER_H

#ifndef LEXER_H
#include "lexer.h"
#endif

#ifndef TABLES_H
#include "tables.h"
#endif

typedef struct Parser {
	Token current;
	Lexer *lexer;
	char *src;
	int text_section;
} Parser;

typedef struct Label {
    char *name;
} Label;

typedef struct NumOrLabel {
	int is_label;
	union {
		int n;
		Label l;
	} data;
} NumOrLabel;

typedef struct RIns {
	RInsTableEntry *entry;
	int rd, rs1, rs2;
} RIns;

typedef struct IIns {
	IInsTableEntry *entry;
	int rd, rs1;
	NumOrLabel imm;
} IIns;

typedef struct SIns {
	SInsTableEntry *entry;
	int rs1, rs2;
	NumOrLabel imm;	
} SIns;

typedef struct BIns {
	BInsTableEntry *entry;
	int rs1, rs2;
	NumOrLabel imm;
} BIns;

typedef struct UIns {
	UInsTableEntry *entry;
	int rd;
	NumOrLabel imm;	
} UIns;

typedef struct JIns {
	JInsTableEntry *entry;
	int rd;
	NumOrLabel imm;
} JIns;

typedef enum NodeType {
	LABEL, R_INS, I_INS, S_INS, B_INS, U_INS, J_INS
} NodeType;

typedef struct ParseErr {
	int is_err;
	char *msg;
	int line, scol, ecol;	
} ParseErr;

typedef struct ParseNode {	
	NodeType type;
	union {
        Label l;
		RIns r;
		IIns i;
		SIns s;
		BIns b;
		UIns u;
		JIns j;
	} data;
	int line;
} ParseNode;

typedef struct ParseNodeVec {
	size_t len, cap;
	ParseNode *data;
} ParseNodeVec;

typedef struct DataVec {
	size_t len, cap;
	uint8_t *data;
} DataVec;

void parser_init(Parser *p, Lexer *l);
ParseNode parser_next(Parser *p, ParseErr *err);
void parse_all(Parser *p, ParseNodeVec *pn, DataVec *d, ParseErr *err);
