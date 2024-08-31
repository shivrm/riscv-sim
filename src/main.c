#include <stdio.h>
#include <string.h>

#ifndef EMITTER_H
#include "emitter.h"
#endif

char *repr_token_type(TokenType tt) {
	switch (tt) {
		case TOK_EOF:    return "EOF   ";
		case TOK_LPAREN: return "LPAREN";
		case TOK_RPAREN: return "RPAREN";
		case TOK_COMMA:  return "COMMA ";
		case TOK_COLON:  return "COLON ";
		case TOK_DECNUM: return "DECNUM";
		case TOK_BINNUM: return "BINNUM";
		case TOK_HEXNUM: return "HEXNUM";
		case TOK_OCTNUM: return "OCTNUM";
		case TOK_IDENT:  return "IDENT ";
	}
}


void str_slice(char* str, char* result, size_t start, size_t end) {
	strncpy(result, str + start, end - start);
	result[end - start] = '\0';
}

int main(void) {
	char* src = " \
	Label1: \
	sd x0, 0(x0) \
	add x11, x3, x4 \
    addi x12, x3, 0x200 \
    ld x5, 02(x3) \
	ld x5 \
";

	FILE *f = fopen("out.hex", "wb");


	Lexer l;
	lexer_init(&l, src);

	printf("Source string: %s\n", src);

	Parser p;
	ParseErr err = {0, ""};
	parser_init(&p, &l);

	ParseNode nodes[100];
	int n = 0;

	for (int i = 0; i < 6; i++) {
		ParseNode pn = parser_next(&p, &err);
		nodes[n++] = pn;
		if (err.is_err) {
			printf("ERROR: %s\n", err.msg);
			return 1;
		}
	}

	emit_all(f, nodes, 5);
	fclose(f);

/*
	ParseNode pn = parser_next(&p);
	
	printf("%d\n", pn.type);
	printf("%d %d %d\n", pn.data.r.rd, pn.data.r.rs1, pn.data.r.rs2);

	pn = parser_next(&p);

	printf("%d\n", pn.type);
	printf("%d %d %d\n", pn.data.i.rd, pn.data.i.rs1, pn.data.i.imm.data.n);

	pn = parser_next(&p);

	printf("%d\n", pn.type);
	printf("%s\n", pn.data.l.name);

	pn = parser_next(&p);

	printf("%d\n", pn.type);
	printf("%s\n", pn.data.l.name);

	pn = parser_next(&p);

	printf("%d\n", pn.type);
	printf("%d %d %d\n", pn.data.r.rd, pn.data.r.rs1, pn.data.r.rs2);
*/

	/*
	Token t;
	while ((t=lexer_next(&l)).type != TOK_EOF) {
		size_t start = t.span.start, end = t.span.end;
		char* slice = malloc(end - start + 1);
		str_slice(src, slice, start, end);
		printf("%s\t%s\n", repr_token_type(t.type), slice);
		free(slice);
	}
	*/

	return 0;
}
