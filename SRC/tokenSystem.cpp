#include "common.h"

#ifdef INFORMATION
SPACES		 space \t \r \n 
PUNCTUATIONS  , | -  (see also ENDERS)
ENDERS		 . ; : ? ! -
BRACKETS 	 () [ ] { } < >
ARITHMETICS	  % * + - ^ = / .  
SYMBOLS 	  $ # @ ~  
CONVERTERS	  & `
//NORMALS 	  A-Z a-z 0-9 _  and sometimes / 
#endif

int inputNest = 0;

#define MAX_BURST 400
static char burstWords[MAX_BURST][MAX_WORD_SIZE];	// each token burst from a text string
static unsigned int burstLimit = 0;					// index of burst  words

uint64 tokenFlags;										// what tokenization saw
char* wordStarts[MAX_SENTENCE_LENGTH];				// current sentence tokenization (always points to D->word values or allocated values)
int wordCount;								// how many words/tokens in sentence
bool capState[MAX_SENTENCE_LENGTH];					
bool originalCapState[MAX_SENTENCE_LENGTH];			// was input word capitalized by user

void ResetTokenSystem()
{
	tokenFlags = 0;
	wordStarts[0] = AllocateHeap((char*)"");    
    wordCount = 0;
	memset(wordStarts,0,sizeof(char*)*MAX_SENTENCE_LENGTH); // reinit for new volley - sharing of word space can occur throughout this volley
	wordStarts[0] = ""; // underflow protection
	ClearWhereInSentence();
	memset(concepts, 0, sizeof(concepts));  // concept chains per word
	memset(topics, 0, sizeof(concepts));  // concept chains per word
}

void DumpResponseControls(uint64 val)
{
	if (val & RESPONSE_UPPERSTART) Log(STDTRACELOG,(char*)"RESPONSE_UPPERSTART ");
	if (val & RESPONSE_REMOVESPACEBEFORECOMMA) Log(STDTRACELOG,(char*)"RESPONSE_REMOVESPACEBEFORECOMMA ");
	if (val & RESPONSE_ALTERUNDERSCORES) Log(STDTRACELOG,(char*)"RESPONSE_ALTERUNDERSCORES ");
	if (val & RESPONSE_REMOVETILDE) Log(STDTRACELOG, (char*)"RESPONSE_REMOVETILDE ");
	if (val & RESPONSE_NOCONVERTSPECIAL) Log(STDTRACELOG, (char*)"RESPONSE_NOCONVERTSPECIAL ");
	if (val & RESPONSE_CURLYQUOTES) Log(STDTRACELOG, (char*)"RESPONSE_CURLYQUOTES ");
}

void DumpTokenControls(uint64 val)
{
	if ((val & DO_SUBSTITUTE_SYSTEM) == DO_SUBSTITUTE_SYSTEM) Log(STDTRACELOG,(char*)"DO_SUBSTITUTE_SYSTEM ");
	else // partials
	{
		if (val & DO_ESSENTIALS) Log(STDTRACELOG,(char*)"DO_ESSENTIALS ");
		if (val & DO_SUBSTITUTES) Log(STDTRACELOG,(char*)"DO_SUBSTITUTES ");
		if (val & DO_CONTRACTIONS) Log(STDTRACELOG,(char*)"DO_CONTRACTIONS ");
		if (val & DO_INTERJECTIONS) Log(STDTRACELOG,(char*)"DO_INTERJECTIONS ");
		if (val & DO_BRITISH) Log(STDTRACELOG,(char*)"DO_BRITISH ");
		if (val & DO_SPELLING) Log(STDTRACELOG,(char*)"DO_SPELLING ");
		if (val & DO_TEXTING) Log(STDTRACELOG,(char*)"DO_TEXTING ");
		if (val & DO_NOISE) Log(STDTRACELOG,(char*)"DO_NOISE ");
	}
	if (val & DO_PRIVATE) Log(STDTRACELOG,(char*)"DO_PRIVATE ");
	// reserved
	if (val & DO_NUMBER_MERGE) Log(STDTRACELOG,(char*)"DO_NUMBER_MERGE ");
	if (val & DO_PROPERNAME_MERGE) Log(STDTRACELOG,(char*)"DO_PROPERNAME_MERGE ");
	if (val & DO_DATE_MERGE) Log(STDTRACELOG,(char*)"DO_DATE_MERGE ");
	if (val & NO_PROPER_SPELLCHECK) Log(STDTRACELOG,(char*)"NO_PROPER_SPELLCHECK ");
	if (val & NO_LOWERCASE_PROPER_MERGE) Log(STDTRACELOG,(char*)"NO_LOWERCASE_PROPER_MERGE ");
	if (val & DO_SPELLCHECK) Log(STDTRACELOG,(char*)"DO_SPELLCHECK ");
	if (val & DO_INTERJECTION_SPLITTING) Log(STDTRACELOG,(char*)"DO_INTERJECTION_SPLITTING ");
	if (val & DO_SPLIT_UNDERSCORE) Log(STDTRACELOG,(char*)"DO_SPLIT_UNDERSCORE ");
	if (val & MARK_LOWER) Log(STDTRACELOG,(char*)"MARK_LOWER ");

	if ((val & DO_PARSE) == DO_PARSE) Log(STDTRACELOG,(char*)"DO_PARSE ");
	else if (val & DO_POSTAG) Log(STDTRACELOG,(char*)"DO_POSTAG ");
	
	if (val & NO_IMPERATIVE) Log(STDTRACELOG,(char*)"NO_IMPERATIVE ");
	if (val & NO_WITHIN) Log(STDTRACELOG,(char*)"NO_WITHIN ");
	if (val & NO_SENTENCE_END) Log(STDTRACELOG,(char*)"NO_SENTENCE_END ");
	if (val & NO_HYPHEN_END) Log(STDTRACELOG,(char*)"NO_HYPHEN_END ");
	if (val & NO_COLON_END) Log(STDTRACELOG,(char*)"NO_COLON_END ");
	if (val & NO_SEMICOLON_END) Log(STDTRACELOG,(char*)"NO_SEMICOLON_END ");
	if (val & STRICT_CASING) Log(STDTRACELOG,(char*)"STRICT_CASING ");
	if (val & ONLY_LOWERCASE) Log(STDTRACELOG,(char*)"ONLY_LOWERCASE ");
	if (val & TOKEN_AS_IS) Log(STDTRACELOG,(char*)"TOKEN_AS_IS ");
	if (val & SPLIT_QUOTE) Log(STDTRACELOG,(char*)"SPLIT_QUOTE ");
	if (val & LEAVE_QUOTE) Log(STDTRACELOG,(char*)"LEAVE_QUOTE ");
	if (val & UNTOUCHED_INPUT) Log(STDTRACELOG,(char*)"UNTOUCHED_INPUT ");
	if (val & NO_FIX_UTF) Log(STDTRACELOG,(char*)"NO_FIX_UTF ");
	if (val & NO_CONDITIONAL_IDIOM) Log(STDTRACELOG, (char*)"NO_CONDITIONAL_IDIOM ");
}

void DumpTokenFlags(char* msg)
{
	Log(STDTRACELOG,(char*)"%s TokenFlags: ",msg);
	// DID THESE
	if (tokenFlags & DO_ESSENTIALS) Log(STDTRACELOG,(char*)"DO_ESSENTIALS ");
	if (tokenFlags & DO_SUBSTITUTES) Log(STDTRACELOG,(char*)"DO_SUBSTITUTES ");
	if (tokenFlags & DO_CONTRACTIONS) Log(STDTRACELOG,(char*)"DO_CONTRACTIONS ");
	if (tokenFlags & DO_INTERJECTIONS) Log(STDTRACELOG,(char*)"DO_INTERJECTIONS ");
	if (tokenFlags & DO_BRITISH) Log(STDTRACELOG,(char*)"DO_BRITISH ");
	if (tokenFlags & DO_SPELLING) Log(STDTRACELOG,(char*)"DO_SPELLING ");
	if (tokenFlags & DO_TEXTING) Log(STDTRACELOG,(char*)"DO_TEXTING ");
	if (tokenFlags & DO_PRIVATE) Log(STDTRACELOG,(char*)"DO_PRIVATE ");
	// reserved
	if (tokenFlags & DO_NUMBER_MERGE) Log(STDTRACELOG,(char*)"NUMBER_MERGE ");
	if (tokenFlags & DO_PROPERNAME_MERGE) Log(STDTRACELOG,(char*)"PROPERNAME_MERGE ");
	if (tokenFlags & DO_DATE_MERGE) Log(STDTRACELOG,(char*)"DATE_MERGE ");
	if (tokenFlags & DO_SPELLCHECK) Log(STDTRACELOG,(char*)"SPELLCHECK ");
	// FOUND THESE
	if (tokenFlags & NO_HYPHEN_END) Log(STDTRACELOG,(char*)"HYPHEN_END ");
	if (tokenFlags & NO_COLON_END) Log(STDTRACELOG,(char*)"COLON_END ");
	if (tokenFlags & PRESENT) Log(STDTRACELOG,(char*)"PRESENT ");
	if (tokenFlags & PAST) Log(STDTRACELOG,(char*)"PAST ");
	if (tokenFlags & FUTURE) Log(STDTRACELOG,(char*)"FUTURE ");
	if (tokenFlags & PERFECT) Log(STDTRACELOG,(char*)"PERFECT ");
	if (tokenFlags & PRESENT_PERFECT) Log(STDTRACELOG,(char*)"PRESENT_PERFECT ");
	if (tokenFlags & CONTINUOUS) Log(STDTRACELOG,(char*)"CONTINUOUS ");
	if (tokenFlags & PASSIVE) Log(STDTRACELOG,(char*)"PASSIVE ");

	if (tokenFlags & QUESTIONMARK) Log(STDTRACELOG,(char*)"QUESTIONMARK ");
	if (tokenFlags & EXCLAMATIONMARK) Log(STDTRACELOG,(char*)"EXCLAMATIONMARK ");
	if (tokenFlags & PERIODMARK) Log(STDTRACELOG,(char*)"PERIODMARK ");
	if (tokenFlags & USERINPUT) Log(STDTRACELOG,(char*)"USERINPUT ");
	if (tokenFlags & FAULTY_PARSE) Log(STDTRACELOG,(char*)"FAULTY_PARSE ");
	if (tokenFlags & COMMANDMARK) Log(STDTRACELOG,(char*)"COMMANDMARK ");
	if (tokenFlags & QUOTATION) Log(STDTRACELOG,(char*)"QUOTATION ");
	if (tokenFlags & IMPLIED_YOU) Log(STDTRACELOG,(char*)"IMPLIED_YOU ");
	if (tokenFlags & NOT_SENTENCE) Log(STDTRACELOG,(char*)"NOT_SENTENCE ");
	if (inputNest) Log(STDTRACELOG,(char*)" ^input ");
	if (tokenFlags & NO_CONDITIONAL_IDIOM) Log(STDTRACELOG, (char*)"CONDITIONAL_IDIOM ");
	Log(STDTRACELOG,(char*)"\r\n");
}

// BUG see if . allowed in word 

int ValidPeriodToken(char* start, char* end, char next,char next2) // token with period in it - classify it
{ //  TOKEN_INCLUSIVE means completes word   TOKEN_EXCLUSIVE not part of word.    TOKEN_INCOMPLETE  means embedded in word but word not yet done
	size_t len = end - start;
	if (IsAlphaUTF8(next) && tokenControl & TOKEN_AS_IS) return TOKEN_INCOMPLETE;
	if (IsDigit(next)) return TOKEN_INCOMPLETE;
	if (len > 100) return TOKEN_EXCLUSIVE; // makes no sense
	if (len == 2) // letter period combo like H.
	{
		char* next = SkipWhitespace(start + 2);
		if (IsUpperCase(*next) || !*next) return TOKEN_INCLUSIVE;	// Letter period like E. before a name
	}
	if (IsWhiteSpace(next) && IsDigit(*start)) return TOKEN_EXCLUSIVE;	// assume no one uses double period without a digit after it.
	if (FindWord(start,len)) return TOKEN_INCLUSIVE;	// nov.  recognized by system for later use
	if (IsMadeOfInitials(start,end) == ABBREVIATION) return TOKEN_INCLUSIVE; //   word of initials is ok
	if (IsUrl(start,end)) 
	{
		if (!IsAlphaUTF8(*(end-1))) return TOKEN_INCOMPLETE; // bruce@job.net]
		return TOKEN_INCLUSIVE; //   swallow URL as a whole
	}
	if (!strnicmp((char*)"no.",start,3) && IsDigit(next)) return TOKEN_INCLUSIVE; //   no.8  
	if (!strnicmp((char*)"no.",start,3)) return TOKEN_INCLUSIVE; //   sentence: No.
	if (!IsDigit(*start) && len > 3 && *(end-3) == '.') return TOKEN_INCLUSIVE; // p.a._system
	if (FindWord(start,len-1)) return TOKEN_EXCLUSIVE; // word exists independent of it

	// is part of a word but word not yet done
    if (IsFloat(start,end,numberStyle) && IsDigit(next)) return TOKEN_INCOMPLETE; //   decimal number9
	if (*start == '$' && IsFloat(start+1,end,numberStyle) && IsDigit(next)) return TOKEN_INCOMPLETE; //   decimal number9 or money
	if (IsNumericDate(start,end)) return TOKEN_INCOMPLETE;	//   swallow period date as a whole - bug . after it?
	if ( next == '-') return TOKEN_INCOMPLETE;	// like N.J.-based
	if (IsAlphaUTF8(next)) return TOKEN_INCOMPLETE;  // "file.txt" 

	//  not part of word, will be stand alone token.
	return TOKEN_EXCLUSIVE;
}


////////////////////////////////////////////////////////////////////////
// BURSTING CODE
////////////////////////////////////////////////////////////////////////

int BurstWord(char* word, int contractionStyle) 
{
#ifdef INFORMATION
	BurstWord, at a minimum, separates the argument into words based on internal whitespace and internal sentence punctuation.
	This is done for storing "sentences" as fact callArgumentList.
	Movie titles extend this to split off possessive endings of nouns. Bob's becomes Bob_'s. 
	Movie titles may contain contractions. These are not split, but two forms of the title have to be stored, the 
	original and one spot contractions have be expanded, which refines to the original.
	And in full burst mode it splits off contractions as well (why- who uses it).

#endif
	//   concept and class names do not burst, regular or quoted, nor do we waste time if word is 1-2 characters, or if quoted string and NOBURST requested
	if (!word[1] || !word[2] || *word == '~' || (*word == '\'' && word[1] == '~' ) || (contractionStyle & NOBURST && *word == '"')) 
	{
		strcpy(burstWords[0],word);
		return 1;
	}

	//   make it safe to write on the data while separating things
	char* copy = AllocateBuffer();
	strcpy(copy, word);
	word = copy;
	unsigned int base = 0;

	//   eliminate quote kind of things around it
    if (*word == '"' || *word == '\'' || *word == '*' || *word == '.')
    {
       size_t len = strlen(word);
       if (len > 2 &&  word[len-1] == *word) // start and end same and has something between
       {
           word[len-1] = 0;  // remove trailing quote
           ++word;
       }
   }

	bool underscoreSeen = false;
	char* start = word;
	while (*++word) // locate spaces of words, and 's 'd 'll 
	{
        if (*word == ' ' || *word == '_' || *word == '`' ||  (*word == '-' && contractionStyle == HYPHENS)) //   these bound words for sure
        {
			if (*word == '_' || *word == '`') underscoreSeen = true;
            if (!word[1]) break;  // end of coming up.

			char* end = word;  
			int len = end-start;
			char* prior = (end-1); // ptr to last char of word
			char priorchar = *prior;

			// separate punctuation from token except if it is initials or abbrev of some kind
			if (priorchar == ',' || IsPunctuation(priorchar) & ENDERS) //   - : ; ? !     ,
			{
				char next = *end;
				char next2 = (next) ? *SkipWhitespace(end+1) : 0;
				if (len <= 1){;}
				else if (priorchar == '.' && ValidPeriodToken(start,end,next,next2) != TOKEN_EXCLUSIVE){;} //   dont want to burst titles or abbreviations period from them
				else  // punctuation not a part of token
				{
					*prior = 0; // not a singleton character, remove it
					--len; // better not be here with -fore  (len = 0)
				}
			}

			// copy off the word we burst
            strncpy(burstWords[base],start,len);
			burstWords[base++][len] = 0;
			if (base > (MAX_BURST - 5)) break; //   protect excess

			//   add trailing punctuation if any was removed
			if (!*prior)
			{
				 *burstWords[base] = priorchar;
				 burstWords[base++][1] = 0;
			}

			//   now resume after
            start = word + 1;
            while (*start == ' ' || *start == '_' || *start == '`') ++start;  // skip any excess blanks of either kind
            word = start - 1;
        }
		else if (*word == '\'' && contractionStyle & (POSSESSIVES|CONTRACTIONS)) //  possible word boundary by split of contraction or possession
        {
            int split = 0;
            if (word[1] == 0 || word[1] == ' '  || word[1] == '_') split = 1;  //   ' at end of word
            else if (word[1] == 's' && (word[2] == 0 || word[2] == ' ' || word[2] == '_')) split = 2;  //   's at end of word
			else if (!(contractionStyle & CONTRACTIONS)) {;} // only accepting possessives
            else if (word[1] == 'm' && (word[2] == 0 || word[2] == ' '  || word[2] == '_')) split = 2;  //   'm at end of word
            else if (word[1] == 't' && (word[2] == 0 || word[2] == ' '  || word[2] == '_')) split = 2;  //   't at end of word
            else if ((word[1] == 'r' || word[1] == 'v') && word[2] == 'e' && (word[3] == 0 || word[3] == ' '  || word[3] == '_')) split = 3; //    're 've
            else if (word[1] == 'l'  && word[2] == 'l' && (word[3] == 0 || word[3] == ' '  || word[3] == '_')) split = 3; //    'll
            if (split) 
            {
				//  swallow any word before
				if (*start != '\'')
				{
					int len = word - start;
					strncpy(burstWords[base],start,len);
					burstWords[base++][len] = 0;
					start = word;
				}
	
				// swallow apostrophe chunk as unique word, aim at the blank after it
				word += split;
				int len = word - start;
				strncpy(burstWords[base],start,len);
				burstWords[base++][len] = 0;

				start = word;
				if (!*word) break;	//   we are done, show we are at end of line
				if (base > MAX_BURST - 5) break; //   protect excess
                ++start; // set start to go for next word+
            }
       }
    }

	// now handle end of last piece
    if (start && *start && *start != ' ' && *start != '_') strcpy(burstWords[base++],start); // a trailing 's or '  won't have any followup word left
	if (!base && underscoreSeen) strcpy(burstWords[base++],(char*)"_");
	else if (!base) strcpy(burstWords[base++],start);
	FreeBuffer();
	burstLimit = base;	// note legality of burst word accessor GetBurstWord
    return base;
}

char* GetBurstWord(unsigned int n) //   0-based
{
	if (n >= burstLimit) 
	{
		ReportBug((char*)"Bad burst n %d",n)
		return "";
	}
	return burstWords[n];
}

char* JoinWords(unsigned int n,bool output) // 
{
	char* limit;
	char* joinBuffer = InfiniteStack(limit,"JoinWords"); // transient
    *joinBuffer = 0;
	char* at = joinBuffer;
    for (unsigned int i = 0; i < n; ++i)
    {
		char* hold = burstWords[i];
		if (!hold) break;
        if (!output && (*hold == ',' || *hold == '?' || *hold == '!' || *hold == ':')) // for output, dont space before punctuation
        {
            if (joinBuffer != at) *--at = 0; //   remove the understore before it
        } 
		size_t len = strlen(hold);
		if ((len + 4 + (at - joinBuffer)) >= maxBufferSize) break;	 // avoid overflow
        strcpy(at,hold);
		at += len;
        if (i != (n-1)) strcpy(at++,(char*)"_");
    }
	if (strlen(joinBuffer) >= (MAX_WORD_SIZE-1)) 
	{
		ReportBug("Joinwords was too big %d %s",strlen(joinBuffer),joinBuffer);
		joinBuffer[MAX_WORD_SIZE-1] = 0; // safety truncation
	}
	CompleteBindStack(); //  we'd like to leave this infinite but string copy by caller may be into infinite as well
    return joinBuffer;
}

////////////////////////////////////////////////////////////////////////
// BASIC TOKENIZING CODE
////////////////////////////////////////////////////////////////////////

static char* HandleQuoter(char* ptr,char** words, int& count)
{
	char c = *ptr; // kind of quoter
	char* end = ptr;
	while (1)
	{
		end = strchr(end + 1, c); // find matching end?
		if (!end) return NULL;
		if (end[1] == '"') end++;	// skip over "" in quote
		else break;
	}
	if (tokenControl & LEAVE_QUOTE) return end+1;

	char pastEnd = IsPunctuation(end[1]); // what comes AFTER quote
	if (!(pastEnd & (SPACES|PUNCTUATIONS|ENDERS))) return NULL;	// doesnt end cleanly

	// if quote has a tailing comma or period, move it outside of the end - "Pirates of the Caribbean,(char*)"  -- violates NOMODIFY clause if any
	char priorc = *(end-1);
	if (priorc == ',' || priorc == '.')
	{
		*(end-1) = *end;
		*end-- = priorc;
	}

	if (c == '*') // stage direction notation, erase it and return to normal processing
	{
		*ptr = ' ';
		*end = ' ';		// erase the closing * of a stage direction -- but violates a nomodify clause
		return ptr;		// skip opening *
	}
	
	// strip off the quotes if quoted words are only alphanumeric single words (emphasis quoting)
	char* at = ptr;
	while (++at < end)
	{
		if (!IsAlphaUTF8OrDigit(*at) ) // worth quoting, unless it is final char and an ender
		{
			if (at == (end-1) && IsPunctuation(*at) & ENDERS);
			else // store string as properly tokenized, NOT as a string.
			{
				char* limit;
				char* buf = InfiniteStack(limit,"HandleQuoter"); // transient
				++end; // subsume the closing marker
				strncpy(buf,ptr,end-ptr);
				buf[end-ptr] = 0;
				buf[MAX_WORD_SIZE - 25] = 0; // force safe limit
				++count;
				words[count] = AllocateHeap(buf); 
				ReleaseInfiniteStack();
				if (!words[count]) words[count] = AllocateHeap((char*)"a"); // safe replacement
				return end;
			}
		}
	}
	++count;
	words[count] = AllocateHeap(ptr+1,end-ptr-1); // stripped quotes off simple word
	if (!words[count]) words[count] = AllocateHeap((char*)"a"); // safe replacement
	return  end + 1;
}

static WORDP UnitSubstitution(char* buffer)
{
	char value[MAX_WORD_SIZE];
	char* at = buffer - 1;
	while (IsDigit(*++at) || *at == '.' || *at == ','); // skip past number
	strcpy(value, "?`");
	strcat(value + 2, at); // presume word after number is not big
	while ((at = strchr(value, '.'))) memmove(at, at + 1, strlen(at)); // remove abbreviation periods
	WORDP D = FindWord(value, 0, STANDARD_LOOKUP);
	if (!D)
	{
		size_t len = strlen(value);
		if (value[len-1] == 's')  D = FindWord(value, len-1, STANDARD_LOOKUP);
	}
	uint64 allowed = tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE);
	return (D && allowed & D->internalBits) ? D : NULL; // allowed transform
}

static char* FindWordEnd(char* ptr, char* priorToken, char** words, int &count, bool nomodify, bool oobStart, bool oobJson)
{
	char* start = ptr;
	char c = *ptr;
	unsigned char kind = IsPunctuation(c);
	char* end = NULL;
	static bool quotepending = false;

	// OOB which has { or [ inside starter, must swallow all as one string lest reading JSON blow token limit on sentence. And we can do jsonparse.
	if (oobStart && oobJson) // support JSON parsing
	{
		if (count == 0 && *ptr == '[') return ptr + 1;	// start of oob
		int level = 0;
		char* jsonStart = ptr;
		--ptr;
		bool quote = false;
		while (*++ptr)
		{
			if (*ptr == '"' && *(ptr - 1) != '\\') quote = !quote;
			if (quote) {} // ignore content for level counting
			else if (*ptr == '{' || *ptr == '[')
			{
				if (*(ptr - 1) != '\\') ++level;
			}
			else if (*ptr == '}' || *ptr == ']')
			{
				if (*(ptr - 1) != '\\') --level;
				if (level == 0)
				{
					if (tokenControl & JSON_DIRECT_FROM_OOB) // allow full json no tokenlimit
					{
						ARGUMENT(1) = "TRANSIENT SAFE";
						ARGUMENT(2) = jsonStart;
						char word[MAX_WORD_SIZE];
						FunctionResult result = JSONParseCode(word);
						++count;
						if (result == NOPROBLEM_BIT) words[count] = AllocateHeap(word); // insert json object
						else words[count] = AllocateHeap((char*)"bad json");
					}
					return ptr + 1;
				}
			}
		}

		return ptr;
	}
	// OOB only separates ( [ { ) ] }   - the rest remain joined as given
	if (oobStart)
	{
		if (*ptr == '(' || *ptr == ')' || *ptr == '[' || *ptr == ']' || *ptr == '{' || *ptr == '}' || *ptr == ',') return ptr + 1;
		bool quote = false;
		--ptr;
		while (*++ptr)
		{
			if (*ptr == '"' && *(ptr - 1) != '\\') quote = !quote;
			if (quote) continue;
			if (*ptr != ' ' &&  *ptr != '(' && *ptr != ')' && *ptr != '[' && *ptr != ']'  && *ptr != '{' && *ptr != '}') continue;
			break;
		}
		return ptr;
	}

	if (kind & QUOTERS) // quoted strings 
	{
		if (c == '\'' && ptr[1] == 's' && !IsAlphaUTF8(ptr[2])) return ptr + 2;	// 's directly
		if (c == '"')
		{
            if (tokenControl & SPLIT_QUOTE)
            {
                char* end = strchr(ptr + 1, '"');
                if (end) // strip the quotes
                {
                    *ptr = ' ';
                    *end = ' ';
                }
                else return ptr + 1; // split up quote marks
            }
			else // see if merely highlighting a word
			{
				char* word = AllocateStack(NULL,INPUT_BUFFER_SIZE,false,false);
				char* tail = ReadCompiledWord(ptr, word);
				char* close = strchr(word + 1, '"');
				ReleaseStack(word);
				if (close && !strchr(word, ' ')) // we dont need quotes
				{
					if (tokenControl & LEAVE_QUOTE) return tail;
					*ptr = ' ';			// kill off starting dq
					ptr[close - word] = ' ';	// kill off closing dq
					return ptr;
				}
			}
		}
		if (c == '\'' && tokenControl & SPLIT_QUOTE) // 'enemies of the state'
		{
			if (quotepending) quotepending = false;
			else if (strchr(ptr + 1, '\'')) quotepending = true;
            if (quotepending) return ptr + 1;
            else if (ptr[1] == ' ' || ptr[1] == '.' || ptr[1] == ',') return ptr + 1;
		}
		if (c == '\'' && !(tokenControl & TOKEN_AS_IS) && !IsAlphaUTF8(ptr[1]) && !IsDigit(ptr[1])) 	return ptr + 1; // is this quote or apostrophe - for penntag dont touch it - for 've  leave it alone also leave '82 alone
		else if (c == '\''  && tokenControl & TOKEN_AS_IS) { ; } // for penntag dont touch it - for 've  leave it alone also leave '82 alone
		else if (c == '"' && tokenControl & TOKEN_AS_IS) return ptr + 1;
		else
		{
			char* end = HandleQuoter(ptr, words, count);
			if (end)  return end;
		}
        if (!IsDigit(ptr[1])) return ptr + 1; // just return isolated quote
	}
    char token[MAX_WORD_SIZE];
    ReadCompiledWord(ptr, token);
    size_t l = strlen(token);

	if (*ptr == '?') return ptr + 1; // we dont have anything that should join after ?    but  ) might start emoticon
	if (*ptr == 0xc2 && ptr[1] == 0xbf) return ptr + 2; // inverted spanish ?
	if (*ptr == 0xc2 && ptr[1] == 0xa1) return ptr + 2; // inverted spanish !

	if (IsAlphaUTF8(*ptr) && ptr[1] == '.' && ptr[2] == ' ' && IsUpperCase(*ptr)) return ptr + 2; // single letter abbreviaion period like H.
	if (*ptr == '.' && ptr[1] == '.' && ptr[2] == '.' && ptr[3] != '.') return ptr + 3; // ...
	if (*ptr == '-' && ptr[1] == '-' && ptr[2] == '-') ptr[2] = ' ';	// change excess --- to space
	if (*ptr == '-' && ptr[1] == '-' && (ptr[2] == ' ' || IsAlphaUTF8(ptr[2]))) return ptr + 2; // the -- break
	if (*ptr == ';' && ptr[1] != ')' && ptr[1] != '(') return ptr + 1; // semicolon not emoticon
	if (*ptr == ',' && ptr[1] != ':') return ptr + 1; // comma not emoticon
	if (*ptr == '|') return ptr + 1;
	if (*ptr == '(' || *ptr == '[' || *ptr == '{') return ptr + 1;

	// if we actually have this token in dictionary, accept it. (eg abbreviations, etc)
	WORDP Z = FindWord(token); // either case
	if (Z && !IS_NEW_WORD(Z) && token[l-1] != '?' && token[l - 1] != '!') return ptr + l; // not generated by user input
	// if token ends in period and does not start with digit (not float) and word we know,
	// return prior

	if (*token == '.' && !IsInteger(token + 1, false, numberStyle) && FindWord(token + 1)) return ptr + 1; // sentence end then word we know
	
	if (token[l - 1] == '.' && FindWord(token, l - 1)) return ptr + l - 1;
  
	// if this was 93302-42345 then we need to keep - separate, not as minus
	if (*token == '-' && IsInteger(token + 1, false, numberStyle) && IsInteger(priorToken, false, numberStyle))
	{
		return ptr + 1;
	}

	// find current token which has | after it and separate it, like myba,atat,joha
	char* pipe = strchr(token + 1, '|');
	if (pipe)
	{
		*pipe = 0; // break apart token
	}

	// check for float
	if (strchr(token, numberPeriod) || strchr(token, 'e') || strchr(token, 'E'))
	{
		char* at = token;

		// check for a currency symbol
		char* number = token;
		char* currency = 0;
		if ((currency = (char*)GetCurrency((unsigned char*)at, number))) at = number; // if currency, find number start of it

		bool seenExponent = false;
		bool seenPeriod = false;
		while (*++at && (IsDigit(*at) || *at == ',' || *at == '.' || (!seenExponent && (*at == 'e' || *at == 'E')) || *at == '-' || *at == '+'))
		{
			if (currency && at == currency) break; // seen enough if reached a currency suffix
			if (*at == 'e' || *at == 'E') seenExponent = true; // exponent can only appear once, 10e4euros
			// period AFTER float like 1.0. w space or end
			if (*at == numberPeriod && IsDigit(*(at-1)) && seenPeriod && !at[1])
			{
				return ptr + (at - token);
			}
			if (*at == numberPeriod) seenPeriod = true;
		}

		// may be units or currency attached, so dont split that apart
		if (IsFloat(number, at, numberStyle) && !UnitSubstitution(at)) // $50. is not a float, its end of sentene
		{
			if (currency && at == currency) at += strlen(currency);
			if (*at == '%') ++at;
			if (*at == 'k' || *at == 'K' || *at == 'm' || *at == 'M' || *at == 'B' || *at == 'b')
			{
				if (!at[1]) ++at;
			}
			return ptr + (at - token);
		}
	}

	// check for negative number
	if (*token == '-' && IsDigit(token[1]))
	{
		char* at = token;
		while (*++at && (IsDigit(*at) || *at == '.' || *at == ',')) { ; }
		if (!*at) {
			// might be at the year part of a date 10-1-1992
			if (count > 2 && IsDigit(*priorToken) && *words[count - 1] == '-' && IsDigit(*words[count - 2])) { ; }
			else return ptr + strlen(token);
		}
	}

	// check for ordinary integers whose commas may be confusing
	if ((IsDigit(token[0])|| IsDigit(token[1])) && IsDigitWord(token, numberStyle, true)) return ptr + strlen(token);

	// check for date
	if (IsDate(token)) return ptr + strlen(token);

	// check for two numbers separated by a hyphen
	char* hyp = strchr(token, '-');
	if (hyp && IsDigit(*token)) 
	{
		char* at = hyp;
		while (*++at && IsDigit(*at)) { ; }
		char* at1 = hyp;
		while (--at1 != token && IsDigit(*--at1)) { ; }
		if (at1 == token && *at == 0) return ptr + (hyp - token);
	}
	if (hyp && !strchr(hyp+1,'-')) // - used as measure separator 
	{
		if ((hyp[1] == 'x' || hyp[1] == 'X') && hyp[1] == '-') // measure like 2ft-x-5ft
		{
			ptr[hyp - token] = ' ';
			if (hyp[2] == '-') ptr[hyp + 2 - token] = ' ';
			return ptr + (hyp - token);
		}
		else if ((IsDigit(*token) || (*token == '.' && IsDigit(token[1]))) && IsAlphaUTF8(hyp[1]) && !(tokenControl & TOKEN_AS_IS)) // break apart measures like 4-ft except when penntag strict casing
		{
			ptr[hyp - token] = ' ';
			return ptr + (hyp - token);	//   treat as space
		}
		else if (hyp[1] == '-') return ptr + (hyp - token); // the anyways-- break 
	}

	// find current token which has comma after it and separate it, like myba,atat,joha
	char* comma = strchr(token + 1, ',');
	if (comma)
	{
		*comma = 0; // break apart token
		comma = ptr + (comma - token);
	}

	// Things that are normally separated as single character tokens
	char next = ptr[1];
	if (c == '=' && next == '=') // swallow headers == ==== ===== etc
	{
		while (*++ptr == '='){;}
		return ptr;
	}
	else if (c == '\'' && next == '\'' && ptr[2] == '\'' && ptr[3] == '\'') return ptr + 4;	// '''' marker
	else if (c == '\'' && next == '\'' && ptr[2] == '\'') return ptr + 3;	// ''' marker
	else if (c == '\'' && next == '\'') return ptr + 2;	// '' marker
	//   arithmetic operator between numbers -  . won't be seen because would have been swallowed already if part of a float, 
	else if ((kind & ARITHMETICS || c == 'x' || c == 'X' || c == '/') && IsDigit(*priorToken) && IsDigit(next)) 
	{
		return ptr+1;  // separate operators from number
	}
	//   normal punctuation separation
	else if (c == '.' && IsDigit(ptr[1]));	// double start like .24
	else if (c == '.' && (ptr[1] == '"' || ptr[1] == '\'')) return ptr + 1;	// let it end after closing quote
	if (c == '.' && ptr[1] == '.' && ptr[2] == '.')  // stop at .. or ...   stand alone punctuation 
	{
		if (tokenControl & TOKEN_AS_IS) 
			return ptr + 3;
		return ptr+1;
	}
	else if (*ptr == numberComma)
	{
		if (IsDigit(ptr[1]) && IsDigit(ptr[2]) && IsDigit(ptr[3]) && ptr != start && IsDigit(ptr[-1])) { ; } // 1,000  is legal
		else return ptr + 1;
	}
 	else if (kind & (ENDERS|PUNCTUATIONS) && ((unsigned char)IsPunctuation(ptr[1]) == SPACES || ptr[1] == 0)) return ptr+1; 
	
	// read an emoticon
	char emote[MAX_WORD_SIZE];
	int index = 0;
	int letters = 0;
	char* at = ptr-1;
	if (!IsAlphaUTF8OrDigit(at[1]))  while (*++at && *at != ' ') // dont check on T?
	{
		emote[index++] = *at;
		if (IsAlphaUTF8(*at) || IsDigit(*at)) ++letters;
		if (letters > 1) break; // to many to be emoticon
		if (*at == '?' || *at == '!' || *at == '.' || *at == ',')
		{
			letters = 5;
			break;	// punctuation we dont want to lose
		}
	}
	if (letters < 2 && (at-ptr) >= 2 && emote[0] != '.' && emote[0] != ',' && emote[0] != '?' && emote[0] != '!' ) // presumed emoticon
	{
		return at;
	}

	if (comma && IsDigit(*(comma-1)) && !IsDigit(comma[1])) return comma; // $7 99
	if (comma && IsDigit(comma[1]) && IsDigit(comma[2]) && IsDigit(comma[3]) && IsDigit(comma[4])) return comma; // 25,2019 
	if (comma && !IsCommaNumberSegment(comma+1,NULL)) return comma; // 25,2 rest of word is not valid comma segments

	if (kind & BRACKETS && ( (c != '>' && c != '<') || next != '=') ) 
	{
		if (c == '<' && next == '/') return ptr + 2; // keep html together  </
		if (c == '[' && next == '[') return ptr + 2; // keep html together  [[
		if (c == ']' && next == ']') return ptr + 2; // keep html together  ]]
		if (c == '{' && next == '{') return ptr + 2; // keep html together  {{
		if (c == '}' && next == '}') return ptr + 2; // keep html together  }}
		return ptr+1; // keep all brackets () [] {} <> separate but  <= and >= are operations
	}

	//   find "normal" word end, including all touching nonwhitespace, keeping periods (since can be part of word) but not ? or ! which cant
 	end = ptr;
	char* stopper = NULL;
	char* fullstopper = NULL;
	if (*ptr != ':' && *ptr != ';') while (*++end && !IsWhiteSpace(*end) && *end != '!' && *end != '?')
	{
		if (*end == numberComma) 
		{
			if (!IsDigit(end[1]) || !IsDigit(* (end-1))) // not comma within a number
			{
				if (!fullstopper) fullstopper = end;
				if (!stopper) stopper = end;
			}
			continue;
		}
		if (*end == ';' && !stopper) stopper = end;
		if (*end == '-' && !(tokenControl & TOKEN_AS_IS) && !stopper) stopper = end; // alternate possible end  (e.g. 8.4-ounce)
		if (*end == ';' && !fullstopper) fullstopper = end; // alternate possible end  (e.g. 8.4-ounce)
		if (end[0] == '.' && end[1] == '.' && end[2] == '.') break; // ...
	}
	if (comma && end > comma && (!IsDigit(comma[1]) ||!IsDigit(comma[-1]))) end = comma;

	if (end == ptr) ++end;	// must shift at least 1

	// possessive ending? swallow whole token like "K-9's"
	if (*(end - 1) == 's' && (end - ptr) > 2 && *(end - 2) == '\'') return end - 2;

	WORDP X = FindWord(ptr,end-ptr,PRIMARY_CASE_ALLOWED);
	// avoid punctuation so we can detect emoticons
	if (X && !(X->properties & PUNCTUATION) && (X->properties & PART_OF_SPEECH || X->systemFlags & PATTERN_WORD || X->internalBits & HAS_SUBSTITUTE)) // we know this word (with exceptions)
	{
		// if ' follows a number, make it feet
		if (*ptr == '\'' && (end-ptr) == 1)
		{
			if (IsDigit(*priorToken))
			{
				++count;
				words[count] = AllocateHeap((char*)"foot"); 
				return end;
			}
		}

		// but No. must not be recognized unless followed by a digit
		else if (!strnicmp(ptr,(char*)"no.",end-ptr))
		{
			char* at = end;
			if (*at) while (*++at && *at == ' ');
			if (IsDigit(*at)) return end;
		}

		else return end;
	}
	if (IsUpperCase(*ptr)) 
	{
		X = FindWord(ptr,end-ptr,LOWERCASE_LOOKUP);
		// avoid punctuation so we can detect emoticons
		if (X && !(X->properties & PUNCTUATION) && (X->properties & PART_OF_SPEECH || X->systemFlags & PATTERN_WORD || X->internalBits & HAS_SUBSTITUTE)) // we know this word (with exceptions)
		{
			//  No. must not be recognized unless followed by a digit
			if (!strnicmp(ptr,(char*)"no.",end-ptr))
			{
				char* at = end;
				if (*at) while (*++at && *at == ' ');
				if (IsDigit(*at)) return end;
			}

			else return end;
		}
	}

	// could it be email or web address?
	char* atsign = strchr(ptr,'@'); // possible email?
	if (atsign && atsign < end)
	{
		char* period = strchr(atsign+1,'.');
		if (period && period < end && IsAlphaUTF8(ptr[end-ptr-1]) &&  IsAlphaUTF8(ptr[end-ptr-2])) // can be domain data
		{
			// find end of email text word
			while (*++period)
			{
				if (!IsAlphaUTF8(*period)) return period;
			}
			return end;
		}
	}

	//  e-mail, needs to not see - as a stopper.
	WORDP W = (fullstopper) ? FindWord(ptr,fullstopper-ptr) : NULL;
	if (*end && IsDigit(end[1]) && IsDigit(*(end-1))) W = NULL; // if , separating digits, DONT break at it  4,000 even though we recognize subpiece
	if (W && (W->properties & PART_OF_SPEECH || W->systemFlags & PATTERN_WORD))  return fullstopper; // recognize word at more splits

	// recognize subword? now in case - is a stopper
	if (stopper)
	{
		W = ((stopper-ptr) > 1 && ((*stopper != '-' && *stopper != '/') || !IsAlphaUTF8(stopper[1]))) ? FindWord(ptr,stopper-ptr) : NULL;
		if (*stopper == '-' && (IsAlphaUTF8(end[1]) || IsDigit(end[1]))) W = NULL; // but don't split -  in a name or word or think like jo-5
		else if (*stopper && IsDigit(stopper[1]) && IsDigit(*(stopper-1))) W = NULL; // if , separating digits, DONT break at it  4,000 even though we recognize subpiece
		if (W && (W->properties & PART_OF_SPEECH || W->systemFlags & PATTERN_WORD))  return stopper; // recognize word at more splits
	}
	int lsize = strlen(token);
	while (IsPunctuation(token[lsize-1])) token[--lsize] = 0; // remove trailing punctuation
	char* after = start + lsize;
	// see if we have 25,2015
	size_t tokenlen = strlen(token);
	if (tokenlen == 7 && IsDigit(token[0]) && IsDigit(token[1]) && token[2] == numberComma && IsDigit(token[3]))
		return ptr + 2;
	if (tokenlen == 6 && IsDigit(token[0])  && token[1] == numberComma && IsDigit(token[2])) // 2,2015
		return ptr + 1;
	if (!strnicmp(token,"https://",8) || !strnicmp(token,"http://",7)) return after;
	if (*priorToken != '/' && IsFraction(token)) return after; // fraction?

															   // check for place number
	char* place = ptr;
	while (IsDigit(*place)) ++place;
	if (!stricmp(language, "english") && (!stricmp(place,"st") || !stricmp(place,"nd") || !stricmp(place,"rd"))) return end;
	else if (!stricmp(language, "french") && (!stricmp(place, "er") || !stricmp(place, "ere") || !stricmp(place, "ère") || !stricmp(place, "nd") || !stricmp(place, "nde") || !stricmp(place, "eme") || !stricmp(place, "ème"))) return end;
	int len = end - ptr;
	char next2;
	if (*ptr == '/') return ptr+1; // split of things separated

	while (++ptr && !IsWordTerminator(*ptr)) // now scan to find end of token one by one, stopping where appropriate
    {
		c = *ptr;
        if (c == '|') break;
		kind = IsPunctuation(c);
		next = ptr[1];
		if (c == numberComma) 
		{
			if (!IsDigit(ptr[1]) || !IsDigit(*(ptr-1))) break; // comma obviously not in a number
			// must have 3 digits after comma
			if (!IsDigit(ptr[2]) || !IsDigit(ptr[3])) break;
		}
		if (c == '\'' && next == '\'') break;	// '' marker or ''' or ''''
		else if (c == '=' && next == '=') break; // swallow headers == ==== ===== etc
		next2 = (next) ? *SkipWhitespace(ptr+2) : 0; // start of next token
		if (c == '-' && next == '-') break; // -- in middle is a break regardless
		if (tokenControl & TOKEN_AS_IS) {;}
		else
		{
			if (c == '\'') //   possessive ' or 's  - we separate ' or 's into own word
			{
				if (next == ',' || IsWhiteSpace(next) || next == ';' || next == '.' || next == '!' || next == '?') // trailing plural?
				{
					break; 
				}
				if (!IsAlphaUTF8OrDigit(next)) break;					//   ' not within a word, ending it
				if (((next == 's') || ( next == 'S')) && !IsAlphaUTF8OrDigit(ptr[2]))  //   's becomes separate - can be WRONG when used as contraction like speaker's but we cant know
				{
					ptr[1] = 's'; // in case uppercase flaw
					break;	
				}
				// ' as particle ellision 
				if ((ptr - start) == 1 && (*start == 'd' || *start == 'c' || *start == 'j' || *start == 'l' || *start == 's' || *start == 't' || *start == 'm' || *start == 'n')) return ptr + 1;  // break off d' argent and other foreign particles
				else if (!stricmp(language, "french"))
				{
					if ((ptr - start) == 1 && (*start == 'D' || *start == 'C' || *start == 'J' || *start == 'L' || *start == 'S' || *start == 'T' || *start == 'M' || *start == 'N')) return ptr + 1;  // break off french particles in upper case
					else if ((ptr - start) == 2 && (*start == 'q' || *start == 'Q') && *(start + 1) == 'u') return ptr + 1;  // break off qu'
					else if ((ptr - start) == 5 && (*start == 'j' || *start == 'J') && *(start + 1) == 'u' && *(start + 2) == 's' && *(start + 3) == 'q' && *(start + 4) == 'u') return ptr + 1;  // break off jusqu'
					else if ((ptr - start) == 6 && (*start == 'l' || *start == 'L') && *(start + 1) == 'o' && *(start + 2) == 'r' && *(start + 3) == 's' && *(start + 4) == 'q' && *(start + 5) == 'u') return ptr + 1;  // break off lorsqu'
					else if ((ptr - start) == 6 && (*start == 'p' || *start == 'P') && *(start + 1) == 'u' && *(start + 2) == 'i' && *(start + 3) == 's' && *(start + 4) == 'q' && *(start + 5) == 'u') return ptr + 1;  // break off puisqu'
				}
	
				//   12'6" or 12'. or 12' 
				if (IsDigit(*start) && !IsAlphaUTF8(next)) return ptr + 1;  //   12' swallow ' into number word
			}
			else if (ptr != start && c == ':' && IsDigit(next) && IsDigit(*(ptr-1)) && len > 1) //   time 10:30 or odds 1:3
			{
				if (!strnicmp(end-2,(char*)"am",2)) return end-2;
				else if (!strnicmp(end-2,(char*)"pm",2)) return end-2;
				else if (len > 2 && !strnicmp(end-3,(char*)"a.m",3)) return end-3;
				else if (len > 2 && !strnicmp(end-3,(char*)"p.m",3)) return end-3;
				else if (len > 3 && !strnicmp(end-4,(char*)"a.m.",4)) return end-4;
				else if (len > 3 && !strnicmp(end-4,(char*)"p.m.",4)) return end-4;
				else if (ptr[2] == ' ' || !ptr[2]) return ptr+2;
				else if ((ptr[3] == ' ' || !ptr[3]) && IsDigit(ptr[2])) return ptr+3;
			} 
			
			// number before things? 8months but not 24%  And dont split 1.23 or time words 10:30 and 30:20:20. dont break 6E
			if (IsDigit(*start) && IsDigit(*(ptr-1)) && !IsDigit(c) && c != '%' && c != '.' && c != ':' && ptr[1] && ptr[2] && ptr[1] != ' ' && ptr[2] != ' ')
			{
				if (c == 's' && ptr[1] == 't'){;} // 1st
				else if (c == 'n' && ptr[1] == 'd'){;} // 2nd
				else if (c == 'r' && ptr[1] == 'd'){;} // 3rd
				else if (c == 't' && ptr[1] == 'h'){;} // 5th
				else // break apart known word but not single value or non-word
				{
					char word[MAX_WORD_SIZE];
					ReadCompiledWord(ptr-1,word); // what is the word
					if (FindWord(word,0)) return ptr; // we know this second word after the digit
				}
			}
			if ( c == ']' || c == ')') break; //closers
			if ((c == 'x' || c== 'X') && IsDigit(*start) && IsDigit(next)) break; // break  4x4
		}

		if (kind & BRACKETS) break; // separate brackets

		if (kind & (PUNCTUATIONS|ENDERS|QUOTERS) && IsWordTerminator(next)) 
		{
			if (c == '-' && *ptr == '-' && next == ' ') return ptr + 1;
			if (tokenControl & TOKEN_AS_IS && next == ' ' && ptr[1] && !IsWhiteSpace(ptr[2])) return ptr + 1; // our token ends and there is more text to come
			if (!(tokenControl & TOKEN_AS_IS)) break; // funny things at end of word
		}
		if (c == '/') return ptr; //  separate out / items like john/bob or 12/21/45  or 1/2
		if (c == ';') return ptr; // separate semicolons

		// special interpretations of period
		if (c == '.') 
        {
			int x = ValidPeriodToken(start,end,next,next2);
			if (x == TOKEN_INCLUSIVE) return end;
			else if (x == TOKEN_INCOMPLETE) continue;
			else break;
        }
	}
	if (*(ptr-1) == '"' && start != (ptr-1)) --ptr;// trailing double quote stuck on something else
    return ptr;
}

char* Tokenize(char* input,int &mycount,char** words,bool all,bool nomodify,bool oobStart) //   return ptr to stuff to continue analyzing later
{	// all is true if to pay no attention to end of sentence -- eg for a quoted string
	// nomodify is true on analyzing outputs into sentences, because user format may be fixed
    char* ptr = SkipWhitespace(input);
	int count = 0;

    char* html = input;
    bool oobJson = false;
    unsigned int quoteCount = 0;
    char priorToken[MAX_WORD_SIZE] = {0};
    int nest = 0;
    unsigned int paren = 0;
    
	//AdjustUTF8(ptr, ptr - 1);

	if (tokenControl == UNTOUCHED_INPUT)
	{
		while (ALWAYS) {
			input = SkipWhitespace(input);
			char* space = strchr(input,' '); // find separator
			if (space) {
				++count;
				words[count] = AllocateHeap(input,space-input); // the token
				input = space;
			}
			else if (*input) {
				++count;
				words[count] = AllocateHeap(input); // the token
				input += strlen(input);
				break;
			}
			else break;
		}
		mycount = count;
		ptr = input;
		goto SAFETY;
	}

	// convert html data
	while ((html = strstr(html,(char*)"&#")) != 0) // &#32;
	{
		if (IsDigit(html[2]) && IsDigit(html[3]) && html[4] == ';')
		{
			*html = (char)atoi(html+2);
			memmove(html+1,html+5,strlen(html+4));
		}
		else ++html;
	}
	html = input;
	while ((html = strstr(html,(char*)"&quot;")) != 0) // &quot;
	{
		*html = '"';
		memmove(html+1,html+6,strlen(html+5));
	}
	html = input;
	while ((html = strchr(html,'\\')) != 0) // \"  remove this
	{
		if (html[1] == '"')	memmove(html, html + 1, strlen(html));
		++html;
	}
    *priorToken = 0;
    while (ptr) // find tokens til end of sentence or end of tokens
	{
		ptr = SkipWhitespace(ptr);
		//test input added markers
		if (*ptr == INPUTMARKER) // double opens single closes
		{
			++ptr;
			if (*ptr != INPUTMARKER) break;	// closer
			++ptr;
			continue; // start or end of a sentence from ^input, do not include markers
		}
		if (!*ptr) break; 
		if (!(tokenControl & TOKEN_AS_IS)) while (*ptr == ptr[1] && !IsAlphaUTF8OrDigit(*ptr)  && *ptr != '-' && *ptr != '.' && *ptr != '[' && *ptr != ']' && *ptr != '(' && 
			*ptr != ')' && *ptr != '{' && *ptr != '}') 
			++ptr; // ignore repeated non-alpha non-digit characters -   - but NOT -- and not ...
		if (count == 0)
		{
			if (*ptr != '[' ) oobStart = false;
			else // is this oob json?
			{
				char* at = SkipWhitespace(ptr+1);
				if (*at == '[' || *at == '{') oobJson = true;
			}
		}
		else if (oobStart && *ptr == ']') oobJson = false; // end of oob
		if (*ptr == '"' && !strchr(ptr+1,'"') && !(tokenControl & TOKEN_AS_IS) && ptr[1] && !quoteCount && !(tokenControl & SPLIT_QUOTE))  ptr = SkipWhitespace(++ptr); // ignore single starting quote?  "hi   -- but if it next sentence line was part like POS tagging, would be a problem  and beware of 5' 11"

		// find end of word 
		int oldCount = count;
		if (!*ptr) break; 
		char* end = FindWordEnd(ptr,priorToken,words,count,nomodify,oobStart,oobJson);
		if (count != oldCount)	// FindWordEnd performed allocation already 
		{
			if (count > 0) strcpy(priorToken, words[count]);
			ptr = SkipWhitespace(end);
			continue;
		}
		else if ((end - ptr) > (MAX_WORD_SIZE - 3)) // too big to handle, suppress it.
		{
			char word[MAX_WORD_SIZE];
			strncpy(word, ptr, MAX_WORD_SIZE - 25);
			word[MAX_WORD_SIZE - 25] = 0;
			ReportBug("Token too big: %s size %d limited to %d\r\n", word, (end - ptr), MAX_WORD_SIZE - 25);
			end = ptr + MAX_WORD_SIZE - 25; // abort, too much jammed together. no token to reach MAX_WORD_SIZE
		}
		if (*ptr == ' ')	// FindWordEnd removed stage direction start
		{
			if (count > 0) strcpy(priorToken,words[count]);
			ptr = SkipWhitespace(end);
			continue;
		}

		// get the token
		size_t len = end - ptr;
		strncpy(priorToken,ptr,len);
		priorToken[len] = 0;
		
		char lastc = *(end-1);
		if (*priorToken == '(') ++paren;
		else if (*priorToken && paren) --paren;

		char startc = *priorToken;

		//   reserve next word, unless we have too many
		if (++count > REAL_SENTENCE_LIMIT ) 
		{
			mycount = REAL_SENTENCE_LIMIT;
			goto SAFETY;
		}

		//   if the word is a quoted expression, see if we KNOW it already as a noun, if so, remove quotes
		if (*priorToken == '"' && len > 2)
		{
			char buffer[MAX_WORD_SIZE];
			strcpy(buffer,priorToken);
			ForceUnderscores(buffer);
			WORDP E = FindWord(buffer+1,len-2); // do we know this unquoted?
			if (E && E->properties & PART_OF_SPEECH) strcpy(priorToken,E->word);
		}

		// assign token
 		char* token = words[count] = AllocateHeap(priorToken,len);   
		if (!token) token = words[count] = AllocateHeap((char*)"a"); 
		else if (len == 1 && startc == 'i') *token = 'I'; // force upper case on I
		if (count == 1 && *token == '[' && !token[1]) oobStart = true; // special tokenizing rules

		//   set up for next token or ending tokenization
		ptr = SkipWhitespace(end);

		if (*token == '"' && !(tokenControl & SPLIT_QUOTE) && (count == 1 || !IsDigit(*words[count-1] ))) ++quoteCount;
		if (*token == '"' && !(tokenControl & SPLIT_QUOTE) && count > 1 && quoteCount && !(quoteCount & 1)) // does end of this quote end the sentence?
		{
			char c = words[count-1][0];
			if (*ptr == ',' || c == ',') {;} // comma after or inside means not at end
			else if (*ptr && IsLowerCase(*ptr)){;} // sentence continues
			else if (c == '!' || c == '?' || c == '.') break;	 // internal punctuation ends the sentence
		}
		
		if (*token == '(' && !token[1]) ++nest;
		else if (*token == ')' && !token[1]) --nest;
		else if (*token == '[' && !token[1]) ++nest;
		else if (*token == ']' && !token[1]) --nest;

		if (oobStart && *token == ']' && nest == 0) break;	// ending oob

		if (*ptr == ')' && nest == 1){;}
		else if (*ptr == ']' && nest == 1){;}
		else if (tokenControl & TOKEN_AS_IS) {;} // penn bank input already broken up as sentences
		else if (all || tokenControl & NO_SENTENCE_END || startc == ',' || token[1]){continue;}	// keep going - ) for closing whatever
		else if ( (count > 1 && *token == '\'' && ( (*words[count-1] == '.' && !words[count-1][1]) || *words[count-1] == '!' || *words[count-1] == '?'))) break; // end here
		else if (IsPunctuation(startc) & ENDERS || (startc == ']' && *words[1] == '[' && !nest)) //   done a sentence or oob fragment
		{
			if ((quoteCount & 1) && !(tokenControl & SPLIT_QUOTE)) continue;	// cannot end quotation w/o quote mark at end
			// each punctuation ender can be separately controlled
			if (startc == '-')
			{
				if (IsDigit(*ptr)) {;} // is minus 
				else if (!(tokenControl & NO_HYPHEN_END)) // we dont want hypen to end it anyway
				{
					*token = '.';
					tokenFlags |= NO_HYPHEN_END;
					break;
				}
			}
			else if (startc == ':' && !paren)
			{
				if (strstr(ptr,(char*)" and ") || strchr(ptr,',')) {;}	// guess : is a list - could be wrong guess
				else if (!(tokenControl & NO_COLON_END))			// we dont want colon to end it anyway
				{
					tokenFlags |= NO_COLON_END; 
					break;
				}
			}
			else if (startc == ';'  && !paren)
			{
				if (!(tokenControl & NO_SEMICOLON_END))
				{
					tokenFlags |= NO_SEMICOLON_END;// we dont want semicolon to end it anyway
					 break;
				}
			}
			else if (*ptr == '"' || *ptr == '\'') continue;
			else break;	// []  ? and !  and  .  are mandatory unless NO_SENTENCE_END used
		}
	}
	words[count+1] = AllocateHeap((char*)"");	// mark as empty

	// if all is a quote, remove quotes if it is just around a single word
	if (count == 3 && *words[1] == '"' && *words[count] == '"')
	{
		memmove(words,words+1,count * sizeof(char*)); // move all down
		count -= 2;
	}
	// if all is a quote, remove quotes if it is just around a single word
	else if (count  == 3 && *words[1] == '\'' && *words[count] == '\'')
	{
		memmove(words,words+1,count * sizeof(char*)); // move all down
		count -= 2;
	}
	mycount = count;
SAFETY:
	// cant check sooner because we also use ` for internal marking of sentences
	for (int i = 1; i < mycount; ++i)
	{
		char* fix = words[i];
		while ((fix = strchr(fix, '`')))  *fix = '\'';
	}
	return ptr;
}


////////////////////////////////////////////////////////////////////////
// POST PROCESSING CODE
////////////////////////////////////////////////////////////////////////

static WORDP MergeProperNoun(int& start, int end,bool upperStart) 
{ // end is inclusive
	WORDP D;
	uint64 gender = 0;
	char buffer[MAX_WORD_SIZE];
	*buffer = 0;

	// build composite name
	char* ptr = buffer;
	bool uppercase = false;
	bool name = false;
	if (IsUpperCase(*wordStarts[start]) && IsUpperCase(*wordStarts[end])) uppercase = true;	// MUST BE UPPER
    for (int i = start; i <= end; ++i)
    {
		char* word = wordStarts[i];
        size_t len = strlen(word);
        if (*word == ',' ||*word == '?' ||*word == '!' ||*word == ':')  
		{
			if (i != start) *--ptr = 0;  //   remove the understore before it 
		}
		else
		{
			// locate known sex of word if any, composite will inherit it
			D = FindWord(word,len,LOWERCASE_LOOKUP);
			if (D) gender |= D->properties & (NOUN_HE|NOUN_SHE|NOUN_HUMAN|NOUN_PROPER_SINGULAR);
 			D = FindWord(word,len,UPPERCASE_LOOKUP);
			if (D) 
			{
				gender |= D->properties & (NOUN_HE|NOUN_SHE|NOUN_HUMAN|NOUN_PROPER_SINGULAR);
				if (D->properties & NOUN_FIRSTNAME) name = true;
			}
		}
		if ( (ptr-buffer+len) >= (MAX_WORD_SIZE -3)) break; // overflow
        strcpy(ptr,word);
        ptr += len;
        if (i < end) *ptr++ = '_'; // more to go
    }
	*buffer = GetUppercaseData(*buffer); // start it as uppercase

	D = FindWord(buffer,0,UPPERCASE_LOOKUP); // if we know the word in upper case
	// see if adding in determiner or title to name
	if (start > 1) // see if determiner before is known, like The Fray or Title like Mr.
	{
		WORDP E = FindWord(wordStarts[start-1],0,UPPERCASE_LOOKUP); // the word before
		if (E && !(E->properties & NOUN_TITLE_OF_ADDRESS)) E = NULL; 
		// if not a title of address is it a determiner?  "The" is most common
		if (!E) 
		{
			E = FindWord(wordStarts[start-1],0,LOWERCASE_LOOKUP);
			if (E && !(E->properties & DETERMINER)) E = NULL;
		}
		if (E) // known title of address or determiner? See if we know the composite word includes it - like the Rolling Stones is actually The_Rolling_Stones
		{
			char buffer1[MAX_WORD_SIZE];
			strcpy(buffer1,E->word);
			*buffer1 = GetUppercaseData(*buffer1); 
			strcat(buffer1,(char*)"_");
			strcat(buffer1,buffer);
			if (E->properties & DETERMINER) // if determine is part of name, revise to include it
			{
				WORDP F = FindWord(buffer1);
				if (F) 
				{
					--start;
					D = F; 
				}
			}
			else if (tokenControl & STRICT_CASING && IsUpperCase(*buffer) && IsLowerCase(*wordStarts[start-1])){;} // cannot mix lower title in
			else //   accept title as part of unknown name automatically
			{
				strcpy(buffer,buffer1);
				D = FindWord(buffer);
				--start;
			}
		}
	}
	if ((end - start)  == 0) return NULL;	// dont bother, we already have this word in the sentence
	if (!D && upperStart) 
	{
		WORDP X = FindWord(buffer,0,LOWERCASE_LOOKUP);
		if (X) D = X; // if we know it in lower case, use that since we dont know the uppercase one - eg "Artificial Intelligence"
		else 
		{
			D = FindWord(buffer,0,UPPERCASE_LOOKUP);
			if (D && D->systemFlags & LOCATIONWORD) gender = 0; // a place, not a name
			else D = StoreWord(buffer,gender|NOUN_PROPER_SINGULAR|NOUN);
		}
	}
	if (D && (D->properties & gender) != gender) AddProperty(D,gender); // wont work when dictionary is locked
	if (!D && !upperStart) return NULL; // neither known in upper case nor does he try to create it
	if (D && D->systemFlags & ALWAYS_PROPER_NAME_MERGE) return D;
	if (name) return D; // use known capitalization  - it has a first name
	if (uppercase) return D;
	return NULL; // let SetSequenceStamp find it instead
}

static bool HasCaps(char* word)
{
    if (IsMadeOfInitials(word,word+strlen(word)) == ABBREVIATION) return true;
    if (!IsUpperCase(*word) || strlen(word) == 1) return false;
    while (*++word)
    {
        if (!IsUpperCase(*word)) return true; // do not allow all caps as such a word. at best its an acronym
    }
    return false;
}

static int FinishName(int& start, int& end, bool& upperStart,uint64 kind,WORDP name)
{ // start is beginning of sequence, end is on the sequence last word. i is where to continue outside after having done this one
	
    if (end == UNINIT) end = start;

	if ((end - start) > 6) // improbable, probably all caps input
	{
		int more = end;
		start = end = UNINIT;
		upperStart = false;
		return more; // continue AFTER here
	}

	if (upperStart == false && start == 1 && end == (int)wordCount && IsUpperCase(*wordStarts[start])) upperStart = true; // assume he meant it if only literally that as sentence (eg header)
	
    //   a 1-word title gets no change. also
	if (end == (int)wordCount && start == 1 && (!IsUpperCase(*wordStarts[end]) || !IsUpperCase(*wordStarts[start]) ) && end < 5 && (!name || !(name->systemFlags & ALWAYS_PROPER_NAME_MERGE))) {;} //  entire short sentence gets ignored
	else if ( (end-start) < 1 ){;}  
	else //   make title
	{
		WORDP E = MergeProperNoun(start,end,upperStart);
		if (E) 
		{
			AddSystemFlag(E,kind); // if timeword 
			char* tokens[2];
			tokens[1] = E->word;
			ReplaceWords("Merge name",start,end-start + 1,1,tokens);  //   replace multiple words with single word
			tokenFlags |= DO_PROPERNAME_MERGE;
		}
	}
	int result = start + 1;
	start = end = UNINIT;
	upperStart = false;
	return result; // continue AFTER here
}

static void HandleFirstWord() // Handle capitalization of starting word of sentence
{
	if (*wordStarts[1] == '"') return; // dont touch a quoted thing

	// look at it in upper case first
    WORDP D = FindWord(wordStarts[1],0,UPPERCASE_LOOKUP); // Known in upper case?
	if (D && D->properties & (NOUN|PRONOUN_BITS)) return;	// upper case is fine for nouns and pronoun I

	// look at it in lower case
	WORDP E = FindWord(wordStarts[1],0,LOWERCASE_LOOKUP); 
	WORDP N;
	char word[MAX_WORD_SIZE];
	MakeLowerCopy(word,wordStarts[1]);
	char* noun = GetSingularNoun(word,true,true);
	
	if (D && !E && !IsUpperCase(*wordStarts[1]) && D->properties & NOUN_PROPER_SINGULAR)  wordStarts[1] = D->word; // have upper but not lower, use upper if not plural
	else if (!IsUpperCase(*wordStarts[1])) return; // dont change what is already ok, dont want unnecessary trace output
	else if (noun && !stricmp(word,noun)) wordStarts[1] = StoreWord(noun)->word; // lower case form is the singular form already - use that whether he gave us upper or lower
	else if (E && E->properties & (CONJUNCTION|PRONOUN_BITS|PREPOSITION)) wordStarts[1] = AllocateHeap(E->word); // simple word lower case, use it
	else if (E && E->properties & AUX_VERB && (N = FindWord(wordStarts[2])) && (N->properties & (PRONOUN_BITS | NOUN_BITS) || GetSingularNoun(wordStarts[2],true,false))) wordStarts[1] = AllocateHeap(E->word); // potential aux before obvious noun/pronoun, use lower case of it

	// see if multiple word (like composite name)
	char* multi = strchr(wordStarts[1],'_');
	if (!D && !E && !multi) return;  // UNKNOWN word in any case (probably a name)
	if (E && E->internalBits & HAS_SUBSTITUTE){;}
	else if (!multi || !IsUpperCase(multi[1])) // remove sentence start uppercase if known in lower case unless its a multi-word title or substitute
	{ // or special case word
		WORDP set[20];
		int n = GetWords(wordStarts[1],set,true);	// strict case upper case
		int i;
		for (i = 0; i < n; ++i)
		{
			if (!strcmp(set[i]->word,wordStarts[1]))  // perfect match 
			{
				if (IsLowerCase(wordStarts[1][0])) break;		// starts lower, has upper elsewhere, like eBay.
				if (IsUpperCase(wordStarts[1][1])) break;	// has uppercase more than once
			}
		}
		if (i >= n) // there is nothing special about his word (like eBay or TED)
		{
			char word[MAX_WORD_SIZE];
			MakeLowerCopy(word,wordStarts[1]);
			if (FindWord(word,0,LOWERCASE_LOOKUP))
			{
				char* tokens[2];
				tokens[1] = word;
				ReplaceWords("lowercase",1,1,1,tokens);
			}
		}
	}
	else if (multi) 
	{
		char* tokens[2];
		tokens[1] = word;
		ReplaceWords("multiword",1,1,1,tokens);
		WORDP D = FindWord(wordStarts[1]);
		if (D) AddProperty(D,NOUN_PROPER_SINGULAR);
	}
}

bool DateZone(int i, int& start, int& end)
{
	WORDP D = FindWord(wordStarts[i],0,UPPERCASE_LOOKUP);
	if (!D || !(D->systemFlags & MONTH)) return false;
	start = i;
	end = i;
	if (i > 1 && IsDigit(*wordStarts[i-1]) && atoi(wordStarts[i-1]) < 32) start = i-1;
	else if (i < wordCount && IsDigit(*wordStarts[i+1]) && atoi(wordStarts[i+1]) < 32) end = i+1;
	else if (i > 2 && !stricmp(wordStarts[i-1],(char*)"of") && IsDigit(*wordStarts[i-2])) start = i-2;
	// dont merge "*the 2nd of april" because it might be "the 2nd of April meeting"
	if (end < (int)wordCount)
	{
		char* next = wordStarts[end+1];
		if (IsDigit(*next++) && IsDigit(*next++) &&IsDigit(*next++) && IsDigit(*next++) && !*next) ++end;	// swallow year
		else if (*wordStarts[end+1] == ',')
		{
			char* next = wordStarts[end+2];
			if (IsDigit(*next++) && IsDigit(*next++) &&IsDigit(*next++) && IsDigit(*next++) && !*next) end += 2;	// swallow comma year
		}
	}
	return (start != end); // there is something there
}

void ProcessCompositeDate()
{
    for (int i = FindOOBEnd(1); i <= wordCount; ++i) 
	{
		int start,end;
		if (DateZone(i,start,end))
		{
			char word[MAX_WORD_SIZE];
			strcpy(word,wordStarts[i]); // force month first
			word[0] = toUppercaseData[*word]; // insure upper case
			int at = start - 1;
			while (++at <= end)
			{
				if (at != i && stricmp(wordStarts[at],(char*)"of") && *wordStarts[at] != ',')
				{
					strcat(word,(char*)"_");
					strcat(word,wordStarts[at]);
					if (IsDigit(*wordStarts[at]))
					{
						size_t len = strlen(word);
						if (!IsDigit(word[len-1]) && IsDigit(word[len-3])) word[len-2] = 0; // 1st, 2nd, etc
					}
				}
			}
			WORDP D = StoreWord(word,NOUN|NOUN_PROPER_SINGULAR);
			AddSystemFlag(D,TIMEWORD|MONTH);
			char* tokens[2];
			tokens[1] = D->word;
			ReplaceWords("Date",start,end-start+1,1,tokens);
			tokenFlags |= DO_DATE_MERGE;
		}
  
	}
}

void ProperNameMerge() 
{
	if (tokenControl & ONLY_LOWERCASE) return;

    int start = UNINIT;
    int end = UNINIT;
    uint64 kind = 0;
	bool upperStart = false;
	wordStarts[wordCount+1] = "";
	wordStarts[wordCount+2] = "";

    for (int i = FindOOBEnd(1); i <= wordCount; ++i) 
    {
		char* word = wordStarts[i];
		if (*word == '"' || (strchr(word,'_') && !IsUpperCase(word[0])) || strchr(word,':')) // we never join composite words onto proper names unless the composite is proper already
		{
			if (start != UNINIT)  i = FinishName(start,end,upperStart,kind,NULL); // we have a name started, finish it off
			continue;
		}
		WORDP Z = FindWord(word,0,UPPERCASE_LOOKUP);
		if (IsUpperCase(*word) && Z && Z->systemFlags & NO_PROPER_MERGE)
		{
			if (start != UNINIT) i = FinishName(start,end,upperStart,kind,Z);
			continue;
		}
		if (*word != ',' && !IsUpperCase(*word) && FindWord(word) && tokenControl & NO_LOWERCASE_PROPER_MERGE) // dont allow lowercase words to merge into a title
		{
			int localend = i-1;
			if (start != UNINIT) i = FinishName(start,localend,upperStart,kind,Z);
			continue;
		}

		if (IsUpperCase(*word) && start != UNINIT && i == wordCount) // composite at end of sentence
		{
			int end = i;
			i = FinishName(start,end,upperStart,kind,Z);
			continue;
		}
			
		// check for easy cases of 2 words in a row being a known uppercase word
		if (start == UNINIT && i != (int)wordCount && wordStarts[i+1] && *wordStarts[i+1] != '"')
		{
			char composite[MAX_WORD_SIZE * 5];
			strcpy(composite,wordStarts[i]);
			strcat(composite,(char*)"_");
			strcat(composite,wordStarts[i+1]);
			Z = FindWord(composite,0,UPPERCASE_LOOKUP);
			if (Z && Z->systemFlags & NO_PROPER_MERGE) Z = NULL;
			if (tokenControl & (ONLY_LOWERCASE|STRICT_CASING) && IsLowerCase(*composite)) Z = NULL;	// refuse to see word
			if (Z && Z->properties & NOUN) 
			{
				end = i + 1;
				if (Z->properties & NOUN_TITLE_OF_WORK && i != end && !IsUpperCase(*wordStarts[i+1])) // dont automerge title names the "The Cat", let sequences find them and keep words separate when not intended
				{
					start = end = UNINIT;
					continue;
				}
				else
				{
					bool fakeupper = false;
					i = FinishName(i,end,fakeupper,0,Z);
					continue;
				}
			}
			// now add easy triple
			if ((i + 2) <= wordCount&& *wordStarts[i+2] != '"')
			{
				strcat(composite,(char*)"_");
				strcat(composite,wordStarts[i+2]);
				Z = FindWord(composite,0,UPPERCASE_LOOKUP);
				if (tokenControl & STRICT_CASING && IsLowerCase(*composite)) Z = NULL;	// refuse to see word
				if (Z && Z->systemFlags & NO_PROPER_MERGE) Z = NULL;
				if (Z && (Z->properties & NOUN || Z->systemFlags & PATTERN_WORD)) 
				{
					int count = i + 2;
					bool fakeupper = false;
					i = FinishName(i,count,fakeupper,0,Z);
					continue;
				}
			}
		}
        size_t len = strlen(word);

		WORDP nextWord  = (i < wordCount) ? FindWord(wordStarts[i+1],0,UPPERCASE_LOOKUP) : NULL; // grab next word
		if (tokenControl & (ONLY_LOWERCASE|STRICT_CASING) &&  i < wordCount && wordStarts[i+1] && IsLowerCase(*wordStarts[i+1])) nextWord = NULL;	// refuse to see word
		if (nextWord && nextWord->systemFlags & NO_PROPER_MERGE) nextWord = NULL;

  		WORDP U = FindWord(word,len,UPPERCASE_LOOKUP);
		if (tokenControl & (ONLY_LOWERCASE|STRICT_CASING) && IsLowerCase(*word)) U = NULL;	// refuse to see word
		if (U && U->systemFlags & NO_PROPER_MERGE) U = NULL;
		if (U && !(U->properties & ESSENTIAL_FLAGS)) U = NULL;	//  not a real word
		WORDP D = U; // the default word to use

		WORDP L = FindWord(word,len,LOWERCASE_LOOKUP);
		if (tokenControl & STRICT_CASING && IsUpperCase(*word)) L = NULL;	// refuse to see word
		if (L && L->systemFlags & NO_PROPER_MERGE) L = NULL;
	
		if (L && !IsUpperCase(*word)) D = L;			// has lower case meaning, he didnt cap it, assume its lower case
		else if (L && i == 1 && L->properties & (PREPOSITION | PRONOUN_BITS | CONJUNCTION) ) D = L; // start of sentence, assume these word kinds are NOT in name
		if (i == 1 && L &&  L->properties & AUX_VERB && nextWord && nextWord->properties & (PRONOUN_BITS)) continue;	// obviously its not Will You but its will they
		else if (start == UNINIT && IsLowerCase(*word) && L && L->properties & (ESSENTIAL_FLAGS|QWORD)) continue; //   he didnt capitalize it himself and its a useful word, not a proper name
		
		if (!D && L && L->properties) D = L; //   ever heard of this word? 

		//   given human first name as starter or a title
        if (start == UNINIT && D && D->properties & (NOUN_FIRSTNAME|NOUN_TITLE_OF_ADDRESS))
        {
			upperStart = (i != 1 &&  D->internalBits & UPPERCASE_HASH) ? true : false;  // the word is upper case, so it begins a potential naming
			start = i; 
			kind = 0;
            end = UNINIT;    //   have no potential end yet
            if (i < wordCount) //   have a last name? or followed by a preposition? 
            {
				size_t len1 = strlen(wordStarts[i+1]);
				WORDP F = FindWord(wordStarts[i+1],len1,LOWERCASE_LOOKUP);
				if (tokenControl & STRICT_CASING && IsUpperCase(*wordStarts[i+1])) F = NULL;	// refuse to see word
				if (F && F->properties & (CONJUNCTION | PREPOSITION | PRONOUN_BITS)) //   dont want river in the to become River in the or Paris and Rome to become Paris_and_rome
				{
					start = UNINIT;
					++i;
					continue;
				}
				
				if (nextWord && !(nextWord->properties & ESSENTIAL_FLAGS)) nextWord = NULL;		//   not real
				if (nextWord && nextWord->properties & NOUN_TITLE_OF_ADDRESS) nextWord = NULL;	//  a title of address cannot be here
				if (nextWord && nextWord->systemFlags & NO_PROPER_MERGE) nextWord = NULL;
	
                if (IsUpperCase(*wordStarts[i+1])) //   it's  capitalized --but not just capitalizabile else "Alex lent" would match
                {
					upperStart = true;	//   must be valid
					if (IsLowerCase(*wordStarts[i])) //   make current word upper case, do not overwrite its shared ptr
					{
						if (!wordStarts[i]) wordStarts[i] = AllocateHeap((char*)"a");
						else *wordStarts[i] = GetUppercaseData(*wordStarts[i]);  // safe to overwrite, since it was a fresh allocation
					}
                    ++i; 
                    continue;
                }
           }
        }

		// so much for known human name pairs. Now the general issue.
        bool intended = (HasCaps(word) || IsUpperCase(*word)) && i != 1;
		if ((HasCaps(word) || IsUpperCase(*word)) && !D) intended = true;	// unknown word which had caps. He must have meant it  - GE is an abbrev, but allow it to pass
        uint64 type = (D) ? (D->systemFlags & TIMEWORD) : 0; // type of word if we know it
		if (!kind) kind = type;
        else if (kind && type && kind != type) intended = false;   // cant intermix time and space words

		// National Education Association, education is a known word that should be merged but Mary, George, and Larry, shouldnt merge
		if (D && D->internalBits & UPPERCASE_HASH && GetMeanings(D)) // we KNOW this word by itself, dont try to merge it
		{
			if (start == (int)i)
			{
				end = i;
				i = FinishName(start,end,upperStart,kind,D);
			}
			if (start == UNINIT)  
			{
				upperStart = true;
				start = i;
				end = UNINIT;
			}
			continue;
		}
		
		if (i == 1 && wordCount > 1) // pay close attention to sentence starter
        {
			WORDP N = FindWord(wordStarts[2]);
			if (N && N->properties & PRONOUN_BITS) continue;	// 2nd word is a pronoun, not likely a title word
 			if (D && D->properties & (DETERMINER|QWORD)) continue;   //   ignore starting with a determiner or question word(but might have to back up later to accept it)
		}

        //   Indian food is not intended
		if (intended || (D && D->properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_TITLE_OF_ADDRESS))) // cap word or proper name can start
        {
			if (D && D->properties & POSSESSIVE); // not Taiwanese President
			else if (L && L->properties & QWORD); // ignore WHO for who
            else if (start == UNINIT) //   havent started yet, start now
            {
	 			upperStart = (intended && i != 1);  //   he started it properly or not
                start = i; 
				kind = (D) ? (D->systemFlags & TIMEWORD) : 0;
            }
            if (end != UNINIT) end = UNINIT;  //   swallow a word along the way that is allowed to be lower case
        }
        else if (start != UNINIT) // lowercase may end name, unless turns out to be followed by uppercase after comma and being special
        {
			if (*word == ',' && wordStarts[i+1]) // obvious names of companies
			{
				if (!strcmp(wordStarts[i+1],"Inc.") || !strcmp(wordStarts[i+1],"Ltd.")) continue;
				else if (!strcmp(wordStarts[i+1],"Incorporated") || !strcmp(wordStarts[i+1],"Corporation")) continue;
			}
			if (!stricmp(word,"of") && wordStarts[i+1])
			{
				WORDP X = FindWord(wordStarts[i+1]);
				if (X && D->parseBits & OF_PROPER) continue; // allow Bank of America
			}

			// dont merge comma and lowercase names. Do those via script or recognition
			end = i - 1;	// possessive is not part of it
			i = FinishName(start,end,upperStart,kind,NULL);

            //   Hammer, Howell, & Houton, Inc. 
       }
    }

    if (start != UNINIT ) // proper noun is pending 
    {
        if (end == UNINIT) end = wordCount;
		FinishName(start,end,upperStart,kind,NULL);
    }

	HandleFirstWord();
}

static void MergeNumbers(int& start,int& end) //   four score and twenty = four-score-twenty
{//   start thru end exclusive of end, but put in number power order if out of order (four and twenty becomes twenty-four)
    char word[MAX_WORD_SIZE];
    char* ptr = word;
    for (int i = start; i < end; ++i)
    {
		char* item = wordStarts[i];
        if (*item == numberComma) continue; //   ignore commas
		if (i > start && *item == '-') ++item; // skip leading -
		if (i > start && IsDigit(*wordStarts[i-1]) && !IsDigit(*item)) // digit followed by word
		{
			end = start = (unsigned int)UNINIT;
			return; 
		}
		if (i > start && !IsDigit(*wordStarts[i-1]) && IsDigit(*item) && *wordStarts[i-1] != '-' && *wordStarts[i-1] != '+' ) // word followed by digit
		{
			end = start = (unsigned int)UNINIT;
			return; 
		}

        size_t len = strlen(wordStarts[i]);
		//   one thousand one hundred and twenty three
		//   OR  one and twenty 
        if (i > 1 && i < wordCount && (*item == 'a' || *item == 'A')) //   and,  maybe flip order if first, like one and twenty, then ignore
        {
			int64 power1 = NumberPower(wordStarts[i-1], numberStyle);
			int64 power2 = NumberPower(wordStarts[i+1], numberStyle);
			if (power1 < power2) //   latter is bigger than former --- assume nothing before and just overwrite
			{
				strcpy(word,wordStarts[i+1]);
                ptr = word + strlen(word);
                *ptr++ = '-';
                strcpy(ptr,wordStarts[i-1]);
                ptr += strlen(ptr);
                break;
			}
			if (power1 == power2) // same granularity, don't merge, like "what is two and two"
			{
				  end = start = (unsigned int)UNINIT;
				  return;
			}
            continue;
        }

        strcpy(ptr,item);
        ptr += len;
        *ptr = 0;
        if (i > 1 && i != start) //   prove not mixing types digits and words
        {
  			int64 power1 = NumberPower(wordStarts[i-1], numberStyle);
			int64 power2 = NumberPower(wordStarts[i], numberStyle);
			if (power1 == power2 && power1 != 1) // allow one two three
			{
				end = start = (unsigned int)UNINIT; 
				return;
			}
	        if (*word == '-' && !IsDigit(*item)) 
			{
				end = start = (unsigned int)UNINIT;
				return; //   - not a sign? CANCEL MERGE
			}
        }
        if (i < (end-1) && *item != '-') *ptr++ = '-'; //   hypenate words (not digits )
        else if (i < (end-1) && strchr(wordStarts[i+1],'/')) *ptr++ = '-'; //   is a fraction? BUG
    }
    *ptr = 0;

	//   change any _ to - (substitutions or wordnet might have merged using _
	while ((ptr = strchr(word,'_'))) *ptr = '-';

	//   create the single word and replace all the tokens
    WORDP D = StoreWord(word,ADJECTIVE|NOUN|ADJECTIVE_NUMBER|NOUN_NUMBER); 
	char* tokens[2];
	tokens[1] = D->word;
	ReplaceWords("Merge number",start,end-start,1,tokens);
	tokenFlags |= DO_NUMBER_MERGE;
    end = start = (unsigned int)UNINIT;
}

void ProcessSplitUnderscores()
{
	char* tokens[10];
	for (int i = FindOOBEnd(1); i <= wordCount; ++i) 
    {
		char* original = wordStarts[i];
		if (*original == '\'' || *original == '"') continue;	// quoted expression, do not split
		char* at = original;
		char* under = strchr(original,'_');
		if (!under) continue;

		// dont split if email or url
		if (strchr(original, '@') || strchr(original, '.')) continue;

		int index = 1;
		while (under)
		{
			*under = 0;
			tokens[index++] = StoreWord(at)->word;
			*under = '_';
			at = ++under;
			under = strchr(at,'_');
			if (index > 9) return;	// give up, bad data
		}
		tokens[index] = StoreWord(at)->word;
		ReplaceWords("Split underscore",i,1,index,tokens);
		i += index - 1; // skip over what we did
	}
}

void ProcessCompositeNumber() 
{
    //  convert a series of numbers into one hypenated one and remove commas from a comma-digited string.
	// merge all numbers into one, even if not interpretable.  9  1 1 become such a number as does twenty forty sixty-five
    int start = UNINIT;
	int end = UNINIT;
	char* number;

    for (int i = FindOOBEnd(1); i <= wordCount; ++i) 
    {
		char* word = wordStarts[i];
        bool isNumber = IsNumber(word,numberStyle) != NOT_A_NUMBER && !IsPlaceNumber(word,numberStyle) && !GetCurrency((unsigned char*) word,number);
		size_t len = strlen(word);
        if (isNumber || (start == UNINIT && *word == '-' && i < wordCount && IsDigit(*wordStarts[i+1]))) // is this a number or part of one
        {
            if (start == UNINIT) start = i;
            if (end != UNINIT) end = (unsigned int)UNINIT;
        }
        else if (start == UNINIT) continue; // nothing started
		else
        {
            if (i != wordCount && i != 1) // middle words AND and , 
			{
				// AND between words 
				if (!strnicmp((char*)"and",word,len) || !strnicmp((char*)"&", word, len))
				{
					end = i;
					if (!IsDigit(*wordStarts[i-1]) && !IsDigit(*wordStarts[i+1])) // potential word number 
					{
						int64 before = Convert2Integer(wordStarts[i-1],numberStyle);  //  non numbers return NOT_A_NUMBER   
						int64 after = Convert2Integer(wordStarts[i+1],numberStyle);
						if (after > before){;} // want them ordered--- ignore four score and twenty
						else if (before == 100 || before == 1000 || before == 1000000) continue; // one thousand and five - ten thousand and fifty
					}
				}
				// comma between digit tokens
				else if (*wordStarts[i] == numberComma ) 
				{
					if (IsDigit(*wordStarts[i-1]) && IsDigit(*wordStarts[i+1])) // a numeric comma
					{
						if (strlen(wordStarts[i+1]) == 3) // after comma must be exactly 3 digits 
						{
							end = i; // potential stop
							continue;
						}
					}
				}
			}
            //   this definitely breaks the sequence
            if (end  == UNINIT) end = i;
            if ((end-start) == 1) // no change if its a 1-length item
            {
                start = end = (unsigned int)UNINIT;
                continue; 
            }

			//  numbers in  series cannot merge unless triples after the first (international like 1 222 233) or all single digits
			if (IsDigit(*wordStarts[start]))
			{
				bool multidigit = true;
				for ( int j = start + 1; j < end; ++j) 
				{
					if (wordStarts[j][1] || !IsDigit(wordStarts[j][0])) multidigit = false; 

					if (strlen(wordStarts[j]) != 3 && IsDigit(*wordStarts[j]) && !multidigit) 
					{
						start = end = UNINIT;
						break;
					}
				}
			}

            if (end != UNINIT) 
			{
				i = start; // all merge, just continue to next word now
				MergeNumbers(start,end);
			}
        }
    }

    if (start != UNINIT) //   merge is pending
    {
        if (end  == UNINIT) end = wordCount+1; // drops off the end
		int count = end-start;
        if (count > 1) 
		{
			//   dont merge a date-- number followed by comma 4 digit number - January 1, 1940
			//   and 3 , 3455 or   3 , 12   makes no sense either. Must be 3 digits past the comma
			if (IsDigit(*wordStarts[start]))
			{
				bool multidigit = true;
				for (int j = start + 1; j < end; ++j) 
				{
					if (wordStarts[j][1] || !IsDigit(wordStarts[j][0])) multidigit = false; 
					//  cannot merge numbers like 1 2 3  instead numbers after the 1st digit number must be triples (international)
					if (strlen(wordStarts[j]) != 3 && IsDigit(*wordStarts[j]) && !multidigit) return;
				}
			}

			size_t nextLen = strlen(wordStarts[start+1]);
			if (count != 2 || !IsDigit(*wordStarts[start+1]) || nextLen == 3) MergeNumbers(start,end); 
		}
    }
}

void ReplaceWords(char* why,int i, int oldlength,int newlength,char** tokens) 
{
	if ((wordCount + (newlength-oldlength)) > 251) return; // sentence limitation

	// protect old values after our patch area
	int afterCount = wordCount - i - oldlength + 1;
	char* backupTokens[MAX_SENTENCE_LENGTH];	// place to copy the old tokens
	unsigned short int backupDerivations[MAX_SENTENCE_LENGTH];	// place to copy the old derivations
	memcpy(backupTokens,wordStarts + i + oldlength,sizeof(char*) * afterCount); // save old tokens
	memcpy(backupDerivations,derivationIndex + i + oldlength,sizeof(short int) * afterCount); // save old derivations

	// move in new tokens which are insured to be in dictionary.
	for (int j = 1; j <= newlength; ++j) wordStarts[i + j - 1] = StoreWord(tokens[j])->word;

	// the derivations of each new token is from the range of derviations of the old
	unsigned int start = derivationIndex[i] >> 8;
	unsigned int end = derivationIndex[i+oldlength-1] & 0x0ff;
	unsigned int derivation = (start << 8) | end;
	int endAt = (i + newlength);
	for (int at = i; at <= endAt; ++at) derivationIndex[at] = (unsigned short)derivation;

	// now restore the trailing data.
	memcpy(wordStarts+i+newlength,backupTokens,sizeof(char*) * afterCount);
	memcpy(derivationIndex+i+newlength,backupDerivations,sizeof(short int) * afterCount);

	wordCount += newlength - oldlength;
	wordStarts[wordCount+1] = NULL;
	if (trace & TRACE_INPUT || spellTrace)
	{
		char* limit;
		char* buffer = InfiniteStack(limit,"ReplaceWords");
		char* original = buffer;
		for (int i = 1; i <= wordCount; ++i)
		{
			strcpy(buffer,wordStarts[i]);
			buffer += strlen(buffer);
			*buffer++ = ' ';
		}
		*buffer = 0;
		Log(STDUSERLOG,"%s revised input: %s\r\n",why,original);

		ReleaseInfiniteStack();
	}
}

static bool Substitute(WORDP found,char* sub, int i,int erasing)
{ //   erasing is 1 less than the number of words involved
	// see if we have test condition to process (starts with !) and has [ ] with list of words to NOT match after
	if (sub && *sub == '!')
	{
		if (*++sub != '[') // not a list, a bug
		{
			if (!stricmp(sub,(char*)"tense")) // 'd depends on tense
			{
				WORDP X = (i < wordCount) ? FindWord(wordStarts[i+1]) : 0;
				WORDP Y = (i < (wordCount-1)) ? FindWord(wordStarts[i+2]) : 0;
				if (X && X->properties & VERB_INFINITIVE)
				{
					sub = "would";
				}
				else if (X && X->properties & VERB_PAST_PARTICIPLE)
				{
					sub = "had";
				}
				else if (Y && Y->properties & VERB_INFINITIVE)
				{
					sub = "would";
				}
				else // assume pastparticple "had"
				{
					sub = "had";
				}
			}
			else
			{
				ReportBug((char*)"bad substitute %s",sub)
				return false;
			}
		}
		else
		{
			char word[MAX_WORD_SIZE];
			bool match = false;
			char* ptr = sub+1;
			while (ALWAYS)
			{
				ptr = ReadSystemToken(ptr,word);
				if (*word == ']') break;
				if ( *word == '>')
				{
					if ( i == wordCount) match = true;
				}
				else if (i < wordCount && !stricmp(wordStarts[i+1],word)) match = true;
			}
			if (match) return false;	// not to do
			sub = ptr;	// here is the thing to sub
			if (!*sub) sub = 0;
		}
	}
	else if (*found->word == '?' && found->word[1] == '`') // unit substitution
	{
		char* tokens[3];
		char* at = wordStarts[i];
		while (IsDigit(*++at) || *at == '.');
		char c = *at;
		*at = 0;	// closes out units
		tokens[1] = wordStarts[i]; // the word after the erase zone
		tokens[2] = sub;
		ReplaceWords("Number units", i, 1, 2, tokens); // remove 1, add 2
		*at = c;
		return true;
	}

	int erase = 1 + erasing;
	if (!sub || *sub == '%') // just delete the word or note tokenbit and then delete
	{
		if (tokenControl & TOKEN_AS_IS && *found->word != '.' &&  *found->word != '?' && *found->word != '!') // cannot tamper with word count (pennbank pretokenied stuff) except trail punctuation
		{
			return false;
		}

		if (sub && *sub == '%') 
		{
			if (trace & TRACE_SUBSTITUTE && CheckTopicTrace()) Log(STDTRACELOG,(char*)"substitute flag:  %s\r\n",sub+1);
			tokenFlags |= (int)FindMiscValueByName(sub+1);
		}
		else if (trace & TRACE_SUBSTITUTE && CheckTopicTrace()) 
		{
			Log(STDTRACELOG,(char*)"  substitute erase:  ");
			for (int j = i; j < i+erasing+1; ++j) Log(STDTRACELOG,(char*)"%s ",wordStarts[j]);
			Log(STDTRACELOG,(char*)"\r\n");
		}	
		char* tokens[15];
		tokens[1] = wordStarts[i+erasing+1]; // the word after the erase zone
		int extra = (tokens[1] && *tokens[1]) ? 1 : 0;

		int newWordCount =  wordCount - (erasing+1);
		if (newWordCount == 0) return false;	// dont erase sentence completely

		if (i != wordCount)	ReplaceWords("Deleting",i,erasing+1 + extra,extra,tokens); // remove the removals + the one after if there is one. replace with just the one
		else 	ReplaceWords("Deleting",i,erasing+1,erasing,tokens); // remove 1, add 0
		return true;
	}

	//   substitution match
	if (!strchr(sub,'+') && erasing == 0 && !strcmp(sub,wordStarts[i])) return false; // changing single word case to what it already is?
	
    char wordlist[MAX_WORD_SIZE];
    strcpy(wordlist,sub);
    char* ptr = wordlist;
    while ((ptr= strchr(ptr,'+'))) *ptr = ' '; // change + separators to spaces but leave _ alone

	char* tokens[MAX_SENTENCE_LENGTH];			// the new tokens we will substitute
	memset(tokens,0,sizeof(char*) * MAX_SENTENCE_LENGTH);
	int count;
	if (*sub == '"') // use the content internally literally - like "a_lot"  meaning want it as a single word
	{
		count = 1;
		size_t len = strlen(wordlist);
		tokens[1] = AllocateHeap(wordlist+1,len-2); // remove quotes from it now
		if (!tokens[1]) tokens[1] = AllocateHeap((char*)"a"); 
	}
    else Tokenize(wordlist,count,tokens); // get the tokenization of the substitution

	if (count == 1 && !erasing) //   simple replacement
	{
		if (trace & TRACE_SUBSTITUTE && CheckTopicTrace()) Log(STDTRACELOG,(char*)"  substitute simple replace: \"%s\" with %s\r\n",wordStarts[i],tokens[1]);
		ReplaceWords("Replacement",i,1,1,tokens);	
	}
	else // multi replacement
	{
		if (tokenControl & TOKEN_AS_IS && !(tokenControl & DO_SUBSTITUTES) && (DO_CONTRACTIONS & (uint64)found->internalBits) && count != erase) // cannot tamper with word count (pennbank pretokenied stuff)
		{
			return false;
		}
		if ((wordCount + (count - erase)) >= REAL_SENTENCE_LIMIT) return false;	// cant fit

		if (trace & TRACE_SUBSTITUTE && CheckTopicTrace()) Log(STDTRACELOG,(char*)"  substitute replace: \"%s\" with \"%s\"\r\n",found->word,wordlist);
		ReplaceWords("Multireplace",i, erase, count,tokens);
	}
	return true;
}

static WORDP Viability(WORDP word, int i, unsigned int n)
{
	if (!word) return NULL;
	if (word->systemFlags & ALWAYS_PROPER_NAME_MERGE) return word;
	if (word->systemFlags & CONDITIONAL_IDIOM) //  dare not unless there are no conditions
    {
        char* script = word->w.conditionalIdiom;
        if (script[1] != '=') return 0; // no conditions listed
		if (tokenControl & NO_CONDITIONAL_IDIOM) return 0;
	}
    if (word->internalBits & HAS_SUBSTITUTE)
    {
        WORDP X = GetSubstitute(word); //uh - but we would, uh, , buy, .. lollipops
		if (X && !strcmp(X->word, word->word)) return NULL; // avoid infinite substitute
        uint64 allowed = tokenControl & (DO_SUBSTITUTE_SYSTEM | DO_PRIVATE);
        return (allowed & word->internalBits) ? word : 0; // allowed transform
    }
    if (!(tokenControl & DO_SUBSTITUTES)) return 0; // no dictionary word merge

    if (word->properties & NOUN_TITLE_OF_WORK) return 0;

    //   dont swallow - before a number
    if (i < wordCount && IsDigit(*wordStarts[i + 1]))
    {
        char* name = word->word;
        if (*name == '-' && name[1] == 0) return 0;
        if (*name == '<' && name[1] == '-' && name[2] == 0) return 0;
    }

    if (word->properties & (PUNCTUATION | COMMA | PREPOSITION | AUX_VERB) && n) return word; //   multiword prep is legal as is "used_to" helper
    if (GETMULTIWORDHEADER(word) && !(word->systemFlags & PATTERN_WORD)) return 0; // if it is not a name or interjection or preposition, we dont want to use the wordnet composite word list, UNLESS it is a pattern word (like nautical_mile)
                                                                                   // exclude "going to" if not followed by a potential verb 
    if (!stricmp(word->word, (char*)"going_to") && i < wordCount)
    {
        WORDP D = FindWord(wordStarts[i + 2]); // +1 will be "to"
        return (D && !(D->properties & VERB_INFINITIVE)) ? word : 0;
    }
    if (!n) return 0;

    // how to handle proper nouns for merging here
	if (word->systemFlags & NO_PROPER_MERGE) return 0;
	if (n && word->systemFlags & ALWAYS_PROPER_NAME_MERGE) return word;
	if (!(word->internalBits & UPPERCASE_HASH)) { ; }
    else if (!(tokenControl & DO_PROPERNAME_MERGE)) return 0; // do not merge any proper name
    else if (n  && word->properties & PART_OF_SPEECH && !IS_NEW_WORD(word))
        return word;// Merge dictionary names.  We  merge other proper names later.  words declared ONLY as interjections wont convert in other slots
    else if (n  && word->properties & word->systemFlags & PATTERN_WORD) return word;//  Merge any proper name which is a keyword. 
	
    char* part = strchr(word->word, '_');
    if (word->properties & (NOUN | ADJECTIVE | ADVERB | VERB) && part && !(word->systemFlags & PATTERN_WORD))
    {
        char* part1 = strchr(part + 1, '_');
        WORDP P2 = FindWord(part + 1, 0, LOWERCASE_LOOKUP);
        WORDP P1 = FindWord(word->word, (part - word->word), LOWERCASE_LOOKUP);
        if (!part1 && P1 && P2 && P1->properties & PART_OF_SPEECH && P2->properties & PART_OF_SPEECH)
        {
            // if there a noun this is plural of? like "square feet" where "square_foot" is the keyword
            char* noun = GetSingularNoun(word->word, false, true);
            if (noun)
            {
                WORDP D1 = FindWord(noun);
                if (D1->systemFlags & PATTERN_WORD) { ; }
                else return 0; // we dont merge non-pattern words?
            }
            else return 0;
        }
    }

    if (word->properties & (NOUN | ADJECTIVE | ADVERB | CONJUNCTION_SUBORDINATE) && !IS_NEW_WORD(word)) return word; // merge dictionary found normal word but not if we created it as a sequence ourselves
    return 0;
}

static WORDP ViableIdiom(char* text,int i,unsigned int n)
{ // n is words merged into "word"

	WORDP word = FindWord(text,0, STANDARD_LOOKUP);
	if (!word)
	{
		size_t len = strlen(text);
		if (text[len - 1] == 's' && text[0] != '<') word = FindWord(text, len - 1, STANDARD_LOOKUP);
		if (!word) return 0;
	}
    bool again = primaryLookupSucceeded;
    WORDP X = Viability(word, i, n);
    if (X || !again) return X;
    
	// allowed to try other case
    word = FindWord(text, 0, SECONDARY_CASE_ALLOWED);
	X = Viability(word, i, n);
	if (X) return X;

	return NULL;
}

static bool ProcessMyIdiom(int i,unsigned int max,char* buffer,char* ptr)
{//   buffer is 1st word, ptr is end of it
    WORDP word;
    WORDP found = NULL;
    unsigned int idiomMatch = 0;

	unsigned int n = 0;
    for ( int j = i; j <= wordCount; ++j)
    {
		if (j != i) // add next word onto original starter
		{
			if (!stricmp(loginID,wordStarts[j])) break; // user name should not be part of idiom
			*ptr++ = '`'; // separator between words
			++n; // appended word count
			size_t len = strlen(wordStarts[j]);
			if ( ((ptr - buffer) + len) >= (MAX_WORD_SIZE-40)) return false; // avoid buffer overflow
			strcpy(ptr,wordStarts[j]);
			ptr += strlen(ptr);
		}
    
		//   we have to check both cases, because idiomheaders might accidently match a substitute
		WORDP localfound = found; //   we want the longest match, but do not expect multiple matches at a particular distance
		if (i == 1 && j == wordCount)  //   try for matching at end AND start
        {
			word = NULL;
			*ptr++ = '>'; //   end marker
			*ptr-- = 0;
			word = ViableIdiom(buffer,1,n);
			if (word) 
			{
				found = word;  
				idiomMatch = n;     //   n words ADDED to 1st word
			}
			*ptr = 0; //   remove tail end
		}
		if (found == localfound && i == 1 && (word = ViableIdiom(buffer,1,n))) // match at start
		{
			found = word;   
			idiomMatch = n;   
		}
        if (found == localfound && (word = ViableIdiom(buffer+1,i,n))) // match normal
        {
			found = word; 
			idiomMatch = n; 
		}

		if (!found && i == j && IsDigit(buffer[1])) found = UnitSubstitution(buffer + 1);// generic digits + unit

        if (found == localfound && j == wordCount)  //   sentence ender
		{
			*ptr++ = '>'; //   end of sentence marker
			*ptr-- = 0;  
			word = ViableIdiom(buffer+1,0,n);
			if (word)
            {
				found = word; 
				idiomMatch = n; 
			}
			*ptr= 0; //   back to normal
        }
		if (found == localfound && *(ptr-1) == 's' && j != i) // try singularlizing a noun
		{
			size_t len = strlen(buffer+1);
			word = FindWord(buffer+1,len-1); // remove s
			if (len > 3 && !word && *(ptr-2) == 'e') 
				word = FindWord(buffer+1,len-2); // remove es
			if (len > 3 && !word && *(ptr-2) == 'e' && *(ptr-3) == 'i') // change ies to y
			{
				char noun[MAX_WORD_SIZE];
				strcpy(noun,buffer);
				strcpy(noun+len-3,(char*)"y");
				word = FindWord(noun,0, STANDARD_LOOKUP);
			}
			if (word && (word = ViableIdiom(word->word,i,n))) // was composite
			{
				found = word; // tolerate the singular
				idiomMatch = n; 
			}
		}

        if (n == max) break; //   peeked ahead to max length so we are done
	} //   end J loop

	if (!found) return false;

	WORDP D = GetSubstitute(found);
    if (D == found)  return false;

	bool result = false;
	
	//   dictionary match to multiple word entry
	if (found->internalBits & HAS_SUBSTITUTE) // a special substitution
	{
		result = Substitute(found,D ? D->word : NULL,i,idiomMatch);//   do substitution
		if (result) tokenFlags |= found->internalBits & (DO_SUBSTITUTE_SYSTEM|DO_PRIVATE); // we did this kind of substitution
	}
	else if (found->systemFlags & CONDITIONAL_IDIOM)  // must be a composite word, not a substitute
	{

		if (trace & TRACE_SUBSTITUTE && CheckTopicTrace()) 
		{
			Log(STDTRACELOG,(char*)"use multiword: %s instead of ",found->word);
			for (unsigned int j = i;  j < i + idiomMatch+1; ++j) Log(STDTRACELOG,(char*)"%s ",wordStarts[j]);
			Log(STDTRACELOG,(char*)"\r\n");
		}
		char* tokens[2];
		tokens[1] = found->word;
		ReplaceWords("Idiom",i,idiomMatch + 1,1,tokens);
		result =  true;
		tokenFlags |= NO_CONDITIONAL_IDIOM;
	}

	return result;
}

void ProcessSubstitutes() // revise contiguous words based on LIVEDATA files
{
    char buffer[MAX_WORD_SIZE];
    *buffer = '<';	// sentence start marker

    unsigned int cycles = 0;
    for (int i = FindOOBEnd(1); i <= wordCount; ++i)
    {
		if (!stricmp(loginID,wordStarts[i])) continue; // dont match user's name

		//   put word into buffer to start with
        size_t len = strlen(wordStarts[i]);
		if (len > (MAX_WORD_SIZE-40)) continue;	// too big
		char* ptr = buffer+1;
        strcpy(ptr,wordStarts[i]);
        ptr += len;

        //   can this start a substition?  It must have an idiom count != ZERO_IDIOM_COUNT
 
        unsigned int count = 0;
		WORDP D = FindWord(buffer+1,0,PRIMARY_CASE_ALLOWED); // main word a header?
  		if (D) count = GETMULTIWORDHEADER(D);
		else if (wordStarts[i][len-1] == 's') // consider singular?
		{
			D = FindWord(wordStarts[i], len-1, PRIMARY_CASE_ALLOWED);
			if (D) count = GETMULTIWORDHEADER(D);
		}
        
		//   does secondary form longer phrases?
        WORDP E  = FindWord(buffer+1,0,SECONDARY_CASE_ALLOWED);
		if (E && GETMULTIWORDHEADER(E) > count) count = GETMULTIWORDHEADER(E);

        //   now see if start-bounded word does better
        if (i == 1) 
        {
			D = FindWord(buffer,0,PRIMARY_CASE_ALLOWED); // with < header
            if (D && GETMULTIWORDHEADER(D) > count) count = GETMULTIWORDHEADER(D);
			D = FindWord(buffer,0,SECONDARY_CASE_ALLOWED);
			if (D && GETMULTIWORDHEADER(D) > count) count = GETMULTIWORDHEADER(D);
 		}

		//   now see if end-bounded word does better
		if (i == wordCount)
        {
            *ptr++ = '>'; //   append boundary
            *ptr-- = 0;
            D = FindWord(buffer+1,0,PRIMARY_CASE_ALLOWED);
            if (D && GETMULTIWORDHEADER(D) > count) count = GETMULTIWORDHEADER(D);
			D = FindWord(buffer+1,0,SECONDARY_CASE_ALLOWED);
			if (D &&  GETMULTIWORDHEADER(D) > count) count = GETMULTIWORDHEADER(D);

			if (i == 1) //   can use start and end simultaneously
			{
				D = FindWord(buffer,0,PRIMARY_CASE_ALLOWED); 
				if (D && GETMULTIWORDHEADER(D) > count) count = GETMULTIWORDHEADER(D);
				D = FindWord(buffer,0,SECONDARY_CASE_ALLOWED);
				if (D && GETMULTIWORDHEADER(D) > count) count = GETMULTIWORDHEADER(D);
			}
			*ptr = 0;	// remove tail
		}

		if (!count && IsDigit(*wordStarts[i])) count = 1; // numeric units
        
		//   use max count
        if (count && ProcessMyIdiom(i,count-1,buffer,ptr)) 
		{
			if (cycles > 20) // something is probably wrong
			{
				Log(STDTRACELOG,(char*)"Substitute cycle overflow %s\r\n",buffer);
				break;
			}

			i = 0;  //   restart since we modified sentence
			++cycles;
		}
	}
}
