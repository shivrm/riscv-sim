#include <stdio.h>
#include <string.h>

#ifndef EMITTER_H
#include "emitter.h"
#endif

#include "readfile.c"

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

void print_error(char *src, ParseErr *err) {	
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

int main(void) {
	buffer buf = {0, 0, NULL};
	FILE *fin = fopen("src.s", "r");	
	read_file(fin, &buf);
	fclose(fin);


	char* src = buf.data;

	Lexer l;
	lexer_init(&l, src);

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

	Parser p;
	ParseErr err = {0, "", 0, 0, 0};
	parser_init(&p, &l);
	ParseNode nodes[100];
	
	int n = 0;
	while(1) {
		ParseNode pn = parser_next(&p, &err);
		if (err.is_err) {
			if (strcmp(err.msg, "EOF while parsing") == 0) break;
			print_error(src, &err);
			return 1;
		}
		nodes[n++] = pn;	
	}

	FILE *fout = fopen("out.hex", "wb");
	emit_all(fout, nodes, n);
	fclose(fout);

	printf("OK\n");	
	return 0;
}
