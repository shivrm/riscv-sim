#include "lexer.h"
#include <stdio.h>
#include <string.h>


// Used to construct single character tokens
#define TOK(kind) { Span s = {start, l->pos}; Token t = {kind, s}; return t; }

/*
   Advances the lexer while the current character (c)
   meets a certain predicate.

   Implementing this as a macro allowed direct substitution
   of the predicate expression instead of having to call
   a function for every character. This led to a 25%
   performance increase. 
*/
#define LEXER_TAKE(predicate) { \
	size_t start = l->pos;        \
	char c;                       \
	while ((c=lexer_current(l)))  \
	if(predicate)                 \
	lexer_advance(l);             \
	else                          \
	break;                        \
	s.end = l->pos;               \
}

// Initializes the lexer
void lexer_init(Lexer* l, char* src) {
	l->src = src;
	l->pos = 0;
	l->line = 1;
	l->lastline = -1;
}

// Returns the current charater.
// Inlining improves performance (~20%)
char lexer_current(Lexer* l) {
	return l->src[l->pos];
}

// Advances the lexer to the next character
// Inlining improves performance (~20%)
void lexer_advance(Lexer* l) {
	l->pos++;
}

// Checks if the current character falls into some character classes
// To be used with LEXER_TAKE
#define IS_WHITESPACE(c) ((c) == ' ' || (c) == '\t')
#define IS_DECDIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_BINDIGIT(c) ((c) == '0' || (c) == '1')
#define IS_HEXDIGIT(c) ((c) >= '0' && (c) <= '9' || (c) >= 'a' && (c) <= 'f')
#define IS_OCTDIGIT(c) ((c) >= '0' && (c) <= '7')
#define IS_ALPHA(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#define IS_IDENTCHAR(c) (IS_DECDIGIT(c) || IS_ALPHA(c) || (c) == '_')

// Returns the next token
Token lexer_next(Lexer *l) {
	size_t start = l->pos;
	char c = lexer_current(l);
	Span s = { start, start };

	// Newlines
	if (c == '\n') {
		l->line++;
		l->lastline = l->pos;
		lexer_advance(l);
		return lexer_next(l);
	}	

	// Whitespace
	if (IS_WHITESPACE(c)) {
		LEXER_TAKE(IS_WHITESPACE(c));
		return lexer_next(l);
	}

	// Identitifiers
	if (IS_ALPHA(c) || c == '_') {
		LEXER_TAKE(IS_IDENTCHAR(c));
		Token t = { TOK_IDENT, s };
		return t;
	}

	// Numeric literals
	// First check for leading minus sign
	int flag = 0;
	if (c == '-') {
		lexer_advance(l);
		flag = 1;
	}

	// Check for base specifiers like 0x, 0b, etc
	c = lexer_current(l);
	if (c == '0') {
		lexer_advance(l);
		switch(lexer_current(l)) {
		// Hexadecimal
		case 'x': {
			lexer_advance(l);
			
			c = lexer_current(l);
			if (!IS_HEXDIGIT(c)) {
				TOK(TOK_ERR);	
			}
			
			LEXER_TAKE(IS_HEXDIGIT(c));
			Token t = { TOK_HEXNUM, s };
			return t;
		}
		// Binary
		case 'b': {
			lexer_advance(l);
			
			c = lexer_current(l);
			if (!IS_BINDIGIT(c)) {
				TOK(TOK_ERR);	
			}
			
			LEXER_TAKE(IS_BINDIGIT(c));
			Token t = { TOK_BINNUM, s };
			return t;
		}
		// Octal (also handles the literal '0')
		default:
			LEXER_TAKE(IS_OCTDIGIT(c));
			Token t = { TOK_OCTNUM, s };
			return t;
		}
	} else if (IS_DECDIGIT(c)) {
		// Decimal numbers
		LEXER_TAKE(IS_DECDIGIT(c));
		Token t = { TOK_DECNUM, s };
		return t;
	} else if (flag) {
		// Minus sign without any digits after it
		TOK(TOK_ERR);
	}

	// Single character tokens
	lexer_advance(l);
	switch (c) {
		case '\0':
			TOK(TOK_EOF);
		case ',':
			TOK(TOK_COMMA);
		case ':':
			TOK(TOK_COLON);
		case '(':
			TOK(TOK_LPAREN);
		case ')':
			TOK(TOK_RPAREN);	
		case ';':
			// If there is a comment, ignore all characters until end of line
			// and return the token after thar
			LEXER_TAKE((c != '\n'));			
			return lexer_next(l);
		default:
			// Syntax error for invalid characters
			TOK(TOK_ERR);
	}
}


