/*
©2012–2015 · Serge Zaitsev · zaitsev DOT serge AT gmail DOT com
Released under the MIT license.  For further information, please visit this page
http://zserge.com/jsmn.html
©2016–2022 Revised by Bruce Wilcox
*/
#define VALIDNONDIGIT 1
#define VALIDDIGIT 2
#define VALIDUPPER 3
#define VALIDLOWER 4
#define VALIDUTF8 6

#include "common.h"
#include "jsmn.h"
extern char detectedlanguage[100];
extern bool HasUTF8BOM(char* t);
extern unsigned char isAlphabeticDigitData[];
/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmntok_t* jsmn_alloc_token(jsmn_parser* parser, jsmntok_t* tokens) {
	jsmntok_t* tok = &tokens[parser->toknext++];
	tok->parent = tok->start = tok->end = -1;
	tok->size = 0;
	return tok;
}

/**
 * Fills token type and boundaries.
 */
static jsmntok_t* jsmn_fill_token(jsmn_parser* parser, jsmntok_t* tokens, jsmntype_t type, int start, int end) {
	jsmntok_t* token = &tokens[parser->toknext++];
	token->type = type;
	token->start = start;
	token->end = end;
	token->size = 0;
	token->parent = parser->toksuper;
	return token;
}

/**
 * Fills next available token with JSON primitive (number, boolean, null)
 * CS does not require quotes around field names or strings of simple word, so they come here also
 * Note no errror checking. If it starts with a digit, it's a number and it ends
 * when : } or such json happens, so dates, spaces, they all swallow as a text string
 * masquerading as a number.
 */
static jsmnerr_t jsmn_parse_primitive(jsmn_parser* parser, const char* js, size_t len, jsmntok_t* tokens,bool fieldvalue) {
	int start; // field names and values

	start = parser->pos;
	int nest = 0;
	char c;
	for (; parser->pos < len && js[parser->pos]; parser->pos++) {
		c = js[parser->pos];
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
		case '\t': case '\r': case '\n': case ' ': // marks end of thingys
		case ',':  case '}':
			goto found;
		}
	}

found:
	int lenx = parser->pos - start;
	if (lenx > 2500) // no primitive or field name will be this big, even as variable accessor. Strings can be but they are not primitives
	{
		parser->pos = start;
		char msg[100];
		strncpy(msg, js + start, 90);
		msg[90] = 0;
		ReportBug("Jsmn err2 %s", msg);
		return JSMN_ERROR_INVAL;
	}
	jsmn_fill_token(parser, tokens, JSMN_PRIMITIVE, start, parser->pos);
	parser->pos--;
	return (jsmnerr_t)0;
}

static jsmnerr_t jsmn_parse_string(jsmn_parser* parser, const char* js, jsmntok_t* tokens)
{
	int start = parser->pos;
	char* dq = (char*)(js + start++);
	if (!strncmp(dq, "\"language\":", 8))
	{
		char* lang =dq + 12;
		if (*lang == '\\') ++lang;
		if (*lang == '"')  ++lang;
		char* end = strchr(lang, '\\');
		char* end1 = strchr(lang, '"');
		char* end2 = strchr(lang, ',');
		if (!end) end = end1;
		if (!end) end = end2;
		if (end1 && end1 < end) end = end1;
		if (end2 && end2 < end) end = end2;
		char c = *end;
		*end = 0;
		strcpy(detectedlanguage, lang);
		*end = c;
	}
	
	while ((dq = strchr(++dq, '"'))) // leap to potential closing dquote
	{
		if (*(dq - 1) != '\\')
		{
			jsmn_fill_token(parser, tokens, JSMN_STRING, start, dq - js);
			parser->pos = dq - js;
			return (jsmnerr_t)0;
		}
		char* escape = dq;
		while (*--escape == '\\');
		if ((dq - escape) & 0x01) // even prior escapes will cancel each other out
		{
			jsmn_fill_token(parser, tokens, JSMN_STRING, start, dq - js);
			parser->pos = dq - js;
			return (jsmnerr_t)0;
		}
	}
	return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
jsmnerr_t jsmn_parse(jsmn_parser* parser, const char* js, size_t len, jsmntok_t* tokens)
{
	jsmnerr_t r;
	int i;
	jsmntok_t* token;
	char c;
	bool fieldvalue = false;
	if (HasUTF8BOM((char*)js)) parser->pos += 3; // UTF8 BOM
	for (; parser->pos < len; parser->pos++)
	{
		c = js[parser->pos];
		jsmntype_t type;
		switch (c) {
		case '{': case '[':
			fieldvalue = false;
			token = jsmn_alloc_token(parser, tokens);
			if (parser->toksuper != -1)
			{
				tokens[parser->toksuper].size++;
				token->parent = parser->toksuper;
			}
			token->type = (c == '{') ? JSMN_OBJECT : JSMN_ARRAY;
			token->start = parser->pos;
			parser->toksuper = parser->toknext - 1;
			break;
		case '}': case ']':
			type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
			if (parser->toknext < 1)  return JSMN_ERROR_INVAL;
			token = &tokens[parser->toknext - 1];
			for (;;) {
				if (token->start != -1 && token->end == -1)
				{
					if (token->type != type)   return JSMN_ERROR_INVAL;
					token->end = parser->pos + 1;
					parser->toksuper = token->parent;
					break;
				}
				if (token->parent == -1)  break;
				token = &tokens[token->parent];
			}
			break;
		case ':': 	
			fieldvalue = true; // next thing parsed will be a field value
			break;
		case ',': case '\t': case '\r': case '\n': case ' ':  // ignore all these
			break;
		case '"':
			r = jsmn_parse_string(parser, js, tokens);
			if (r < 0)  return r;
			if (parser->toksuper != -1) tokens[parser->toksuper].size++;
			break;
		default:
			r = jsmn_parse_primitive(parser, js, len, tokens,fieldvalue); // applies to cs  field names and array and field values
			if (r < 0) return r;
			if (parser->toksuper != -1) tokens[parser->toksuper].size++;
			fieldvalue = false; 
			break;
		}
	}

	for (i = parser->toknext - 1; i >= 0; i--)
	{
		/* Unmatched opened object or array */
		if (tokens[i].start != -1 && tokens[i].end == -1)  return JSMN_ERROR_PART;
	}

	return (jsmnerr_t)(parser->toknext + 1); // +1 is probably not needed safety margin
}

