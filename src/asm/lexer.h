#define LEXER_H

#include <stdlib.h>

typedef struct Span {
	size_t start, end;
} Span;

typedef enum TokenType {
	TOK_EOF, TOK_ERR, // Tokens for end-of-file and syntax errors

	TOK_DIRECTIVE,
	TOK_IDENT, TOK_DECNUM, TOK_BINNUM, TOK_HEXNUM, TOK_OCTNUM,
	TOK_COMMA, TOK_COLON, TOK_LPAREN, TOK_RPAREN,
} TokenType;

typedef struct Token {
	TokenType type;
	Span span;	
} Token;

typedef struct Lexer {
	char *src;
	size_t pos, line, lastline;
} Lexer;

void lexer_init(Lexer* l, char *src);
Token lexer_next(Lexer *l);
