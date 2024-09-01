#include <stdio.h>
#include <string.h>

#ifndef EMITTER_H
#include "emitter.h"
#endif

#define BUF_CHUNK_SIZE 4096

typedef struct buffer {
	size_t cap, len;
	char *data;
} buffer;


void read_file(FILE *f, buffer *buf) {
	while(1) {
		size_t available = buf->cap - buf->len;
		// Reallocate buffer if no space is available
		if (available <= 1) {
			buf->data = realloc(buf->data, buf->cap + BUF_CHUNK_SIZE);
			for (int i = buf->cap; i < buf->cap + BUF_CHUNK_SIZE; i++) {
				buf->data[i] = 0;
			}
			buf->cap += BUF_CHUNK_SIZE;
		}

		fgets(&buf->data[buf->len], available, f);
		// fgets doesn't return the number of characters read, so strlen is used.
		size_t n = strlen(&buf->data[buf->len]);
		buf->len += n; 
		
		// If the number of bytes read is less than n, that means we either hit
		// a newline or the EOF.
		if (n < available && feof(f)) {
			return; 
		}	
	}
}


void print_parse_error(char *src, ParseErr *err) {	
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

void print_emit_error(char *src, EmitErr *err) {
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

int main(void) {
	buffer buf = {0, 0, NULL};
	FILE *fin = fopen("input.s", "r");	
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
			print_parse_error(src, &err);
			return 1;
		}
		nodes[n++] = pn;	
	}

	EmitErr err2 = {0, "", 0};
	FILE *fout = fopen("output.hex", "wb");
	emit_all(fout, nodes, n, &err2);
	fclose(fout);

	if (err2.is_err) {
		print_emit_error(src, &err2);
		return 1;
	}

	printf("OK\n");	
	return 0;
}
