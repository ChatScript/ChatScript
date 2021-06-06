/*
©2012–2015 · Serge Zaitsev · zaitsev DOT serge AT gmail DOT com
Released under the MIT license.  For further information, please visit this page
http://zserge.com/jsmn.html
Revised by Bruce Wilcox


*/

#include <stdlib.h>
#include "jsmn.h"

/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmntok_t *jsmn_alloc_token(jsmn_parser *parser,  jsmntok_t *tokens) {
	jsmntok_t *tok;
	tok = &tokens[parser->toknext++];
	tok->start = tok->end = -1;
	tok->size = 0;
	tok->parent = -1;
	return tok;
}

/**
 * Fills token type and boundaries.
 */
static void jsmn_fill_token(jsmntok_t *token, jsmntype_t type,  int start, int end) {
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
}

/**
 * Fills next available token with JSON primitive (number, boolean, null)
 */
static jsmnerr_t jsmn_parse_primitive(jsmn_parser *parser, const char *js, size_t len, jsmntok_t *tokens) {
	jsmntok_t *token;
	int start;

	start = parser->pos;
	int nest = 0;
	char c;
	for (; parser->pos < len && (c = js[parser->pos]) ; parser->pos++) {
		switch (c) 
		{
			case '[': // we allow [ ] in jsonpath access tokens
				++nest;
				continue;
			case ']':
				--nest;
				if (nest < 0) goto found; // was actual close of an array
				continue;
			case ':':
			case '\t' : case '\r' : case '\n' : case ' ' : // marks end of thingys
			case ','  :  case '}' :
				goto found;
		}
		if (c < 32 || c >= 127) { // only ascii blank and on, no UTF markers
			parser->pos = start;
			return JSMN_ERROR_INVAL;
		}
	}

found:
	token = jsmn_alloc_token(parser, tokens);
	jsmn_fill_token(token, JSMN_PRIMITIVE, start, parser->pos);
	token->parent = parser->toksuper;
	parser->pos--;
	return (jsmnerr_t) 0;
}

/**
 * Filsl next token with JSON string.
 */
static jsmnerr_t jsmn_parse_string(jsmn_parser *parser, const char *js, size_t len, jsmntok_t *tokens) {
	jsmntok_t *token;

	int start = parser->pos;

	parser->pos++;

	/* Skip starting quote */
	char c;
	for (; parser->pos < len && (c = js[parser->pos]); parser->pos++) {
		/* Quote: end of string */
		if (c == '\"') {
			token = jsmn_alloc_token(parser, tokens);
			jsmn_fill_token(token, JSMN_STRING, start+1, parser->pos);
			token->parent = parser->toksuper;
			return (jsmnerr_t) 0;
		}

		/* Backslash: Quoted symbol expected */
		if (c == '\\') {
			parser->pos++;
			switch (js[parser->pos]) {
			case 'u': /* Allows escaped symbol \uXXXX */
				{
					parser->pos++;
					for (int i = 0; i < 4 && (c = js[parser->pos]); i++) 
					{
						if ((c >= '0' && c <= '9') ||  	(c >= 'A' && c <= 'F') ||  (c >= 'a' && c <= 'f'))  
						{
							parser->pos++;
						}
						else /* If it isn't a hex character we have an error */
						{
							parser->pos = start;
							return JSMN_ERROR_INVAL;
						}
					}
					parser->pos--;
					break;
				}
				/*  ANY character can be escaped according to standard */
				default: break;
			}
		}
	}
	parser->pos = start;
	return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, size_t len, jsmntok_t *tokens) { // if tokens == null, just count them
	jsmnerr_t r;
	int i;
	jsmntok_t *token;
	int count = 0;
	char c;
	for (; parser->pos < len && (c = js[parser->pos]); parser->pos++) 
	{
		jsmntype_t type;
		switch (c) {
			case '{': case '[':
				count++;
				token = jsmn_alloc_token(parser, tokens);
				if (parser->toksuper != -1) {
					tokens[parser->toksuper].size++;
					token->parent = parser->toksuper;
				}
				token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
				token->start = parser->pos;
				parser->toksuper = parser->toknext - 1;
				break;
			case '}': case ']':
				type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
				if (parser->toknext < 1) 
					return JSMN_ERROR_INVAL;
				token = &tokens[parser->toknext - 1];
				for (;;) {
					if (token->start != -1 && token->end == -1) {
						if (token->type != type)  
							return JSMN_ERROR_INVAL;
						token->end = parser->pos + 1;
						parser->toksuper = token->parent;
						break;
					}
					if (token->parent == -1)  break;
					token = &tokens[token->parent];
				}
				break;
			case '\t' : case '\r' : case '\n' : case ':' : case ',': case ' ':  // ignore all these
				break;
			case '"':
				r = jsmn_parse_string(parser, js, len, tokens);
				if (r < 0) 
					return r;
				count++;
				if (parser->toksuper != -1 ) tokens[parser->toksuper].size++;
				break;
			default:
				r = jsmn_parse_primitive(parser, js, len, tokens);
				if (r < 0) return r;
				count++;
				if (parser->toksuper != -1 ) tokens[parser->toksuper].size++;
				break;
		}
	}

	for (i = parser->toknext - 1; i >= 0; i--) 
	{
		/* Unmatched opened object or array */
		if (tokens[i].start != -1 && tokens[i].end == -1)  return JSMN_ERROR_PART;
	}

	return (jsmnerr_t) count;
}

