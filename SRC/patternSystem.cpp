#include "common.h"

/*
:testpattern ( a  _( case 0 ) ) a big dog
:testpattern ( [ _alternate_1 _alternate_2 ] _* _( dog ) ) alternate_2 cat dog
:testpattern ( _* _(dog) ) cat dog
:testpattern ( _* _one _* ( _two )  _* ( _three ) ) first one two  and three
:testpattern ( _* ( _one _* ( _two ) ) _* ( _three ) ) first one two three
:testpattern ( _* _( _one _two _three ) _* _( _four _* _six ) _* ) one two three middle four five six last
:testpattern ( _* ( _one _* ( _two _* ( _omega ) ) _three _* _four ) ) one two omega three four
:testpattern ( _{ optional } _* _( dog ) ) optional cat dog
:testpattern ( { _optional } _* _( dog ) ) optional cat dog
:testpattern ( _{ _optional } _* _( dog ) ) optional cat dog
:testpattern ( [ _alternate_1 alternate_2 ] _* _( dog ) ) alternate_1 cat dog
:testpattern ( _one _two _( three _four ) five six @_3- _three ) one two three four five six
:testpattern ( _{ _( one _two three ) } _[ _(four five six) ] ) one two three four five six
:testpattern ( _{ _( one _two three ) } _[ _(four five _six) ] ) one two three four five six
*/

#define INFINITE_MATCH (-(200 << 8)) // allowed to match anywhere

#define NOT_BIT					0X00010000
#define FREEMODE_BIT			0X00020000
#define QUOTE_BIT				0X00080000
#define NOTNOT_BIT				0X00400000
#define WILDGAP					0X20000000  // start of gap is 0x000000ff, limit of gap is 0x0000ff00  
#define WILDMEMORIZEGAP			0X40000000  // start of gap is 0x000000ff, limit of gap is 0x0000ff00  
#define WILDMEMORIZESPECIFIC	0X80000000  //   while 0x1f0000 is wildcard index to use
#define GAP_SHIFT 16
#define SPECIFIC_SHIFT 24
#define GAPLIMITSHIFT 8

bool matching = false;
bool deeptrace = false;
static char* returnPtr = NULL;

// pattern macro  calling data
static unsigned int functionNest = 0;	// recursive depth of macro calling
#define MAX_PAREN_NEST 50
static char* ptrStack[MAX_PAREN_NEST];
static int argStack[MAX_PAREN_NEST];
static int baseStack[MAX_PAREN_NEST];
static uint64 matchedBits[20][4];	 // nesting level zone of bit matches
void ShowMatchResult(FunctionResult result, char* rule,char* label)
{
	if (trace & TRACE_LABEL && label && *label && !(trace & TRACE_PATTERN)  && CheckTopicTrace())
	{
		if (result == NOPROBLEM_BIT) Log(STDTRACETABLOG,(char*)"  **  Match: %s\r\n",ShowRule(rule));
		else Log(STDTRACETABLOG,(char*)"  **fail: %s\r\n",ShowRule(rule));
	}
	if (result == NOPROBLEM_BIT && trace & (TRACE_PATTERN|TRACE_MATCH|TRACE_SAMPLE)  && CheckTopicTrace() ) //   display the entire matching responder and maybe wildcard bindings
	{
		if (!(trace & (TRACE_PATTERN|TRACE_SAMPLE)) && label) Log(STDTRACETABLOG, "Try %s",ShowRule(rule)); 
		Log(STDTRACETABLOG,(char*)"  **  Match: %s",(label) ? ShowRule(rule) : ""); //   show abstract result we will apply
		if (wildcardIndex)
		{
			Log(STDTRACETABLOG,(char*)"  Wildcards: ");
			for (int i = 0; i < wildcardIndex; ++i)
			{
				if (*wildcardOriginalText[i]) Log(STDTRACELOG,(char*)"_%d=%s / %s (%d-%d)  ",i,wildcardOriginalText[i],wildcardCanonicalText[i],wildcardPosition[i] & 0x0000ffff,wildcardPosition[i]>>16);
				else Log(STDTRACELOG,(char*)"_%d=null (%d-%d)  ",i,wildcardPosition[i] & 0x0000ffff,wildcardPosition[i]>>16);
			}
		}
		Log(STDTRACELOG,(char*)"\r\n");
	}
}

static void MarkMatchLocation(int start, int end, int depth)
{
	for (int i = start; i <= end; ++i)
	{
		int offset = i / 64; // which unit
		int index = i % 64;  // which bit
		uint64 mask = ((uint64)1) << index;
		matchedBits[depth][offset] |= mask;
	}
}

static char* BitIndex(uint64 bits, char* buffer, int offset)
{
	uint64 mask = 0X0000000000000001ULL;
	for (int index = 0; index <= 63; ++index)
	{
		if (mask & bits) 
		{
			sprintf(buffer,"%d ",offset + index);
			buffer += strlen(buffer);
		}
		mask <<= 1;
	}
	return buffer;
}

void GetPatternData(char* buffer)
{
	char* xxoriginal = buffer;
	buffer = BitIndex(matchedBits[0][0],buffer,0);
	buffer = BitIndex(matchedBits[0][1],buffer,64);
	buffer = BitIndex(matchedBits[0][2],buffer,128);
	buffer = BitIndex(matchedBits[0][3],buffer,192);
}

static void DecodeFNRef(char* side)
{
	char* at = "";
	if (side[1] == USERVAR_PREFIX) at = GetUserVariable(side+1); 
	else if (IsDigit(side[1])) at = FNVAR(side+1);
	at = SkipWhitespace(at);
	strcpy(side,at);
}

static void DecodeComparison(char* word, char* lhs, char* op, char* rhs)
{
	// get the operator
	char* compare = word + Decode(word+1,1); // use accelerator to point to op in the middle
	strncpy(lhs,word+2,compare-word-2);
	lhs[compare-word-2] = 0;
	*op = *compare++;
	op[1] = 0;
	if (*compare == '=') // was == or >= or <= or &= 
	{
		op[1] = '=';
		op[2] = 0;
		++compare;
	}
	strcpy(rhs,compare);
}

bool MatchesPattern(char* word, char* pattern) //   does word match pattern of characters and *
{
	if (!*pattern && *word) return false;	// no more pattern but have more word so fails 
	size_t len = 0;
	while (IsDigit(*pattern)) len = (len * 10) + *pattern++ - '0'; //   length test leading characters can be length of word
	if (len && strlen(word) != len) return false; // length failed
	char* start = pattern;

	--pattern;
	while (*++pattern && *pattern != '*' && *word) //   must match leading non-wild exactly
	{
		if (*pattern != '.' &&  *pattern != GetLowercaseData(*word)) return false; // accept a single letter either correctly OR as 1 character wildcard
		++word;
	}
	if (pattern == start && len) return true;	// just a length test, no real pattern
	if (!*word) return !*pattern || (*pattern == '*' && !pattern[1]);	// the word is done. If pattern is done or is just a trailing wild then we are good, otherwise we are bad.
	if (*word && !*pattern) return false;		// pattern ran out w/o wild and word still has more

	// Otherwise we have a * in the pattern now and have to match it against more word
	
	//   wildcard until does match
	char find = *++pattern; //   the characters AFTER wildcard
	if (!find) return true; // pattern ended on wildcard - matches all the rest of the word including NO rest of word

	// now resynch
	--word;
	while (*++word)
	{
		if (*pattern == GetLowercaseData(*word) && MatchesPattern(word + 1,pattern + 1)) return true;
	}
	return false; // failed to resynch
}

static bool SysVarExists(char* ptr) //   %system variable
{
	char* sysvar = SystemVariable(ptr,NULL);
	if (!*sysvar) return false;
	return (*sysvar) ? true : false;	// value != null
}

static bool FindPartialInSentenceTest(char* test, int start,int originalstart,bool reverse,
	int& actualStart, int& actualEnd)
{
	if (!test || !*test) return false;
	char* word = AllocateStack(NULL, MAX_WORD_SIZE);
	if (reverse)
	{
		for ( int i = originalstart-1; i >= 1; --i) // can this be found in sentence backwards
		{
			MakeLowerCopy(word,wordStarts[i]);
			if (unmarked[i] || !MatchesPattern(word,test)) continue;	// if universally unmarked, skip it. Or if they dont match
			// we have a match of a word
			actualStart = i;
			actualEnd = i;
			ReleaseStack(word);
			return true;
		}
	}
	else 
	{
		for (int i = start+1; i <= wordCount; ++i) // can this be found in sentence
		{
            if (!wordStarts[i]) continue;
			MakeLowerCopy(word,wordStarts[i]);
			if (unmarked[i] || !MatchesPattern(word,test)) continue;	// if universally unmarked, skip it. Or if they dont match
			// we have a match of a word
			actualStart = i;
			actualEnd = i;
			ReleaseStack(word);
			return true;
		}
	}
	ReleaseStack(word);
	return false;
}

static bool MatchTest(bool reverse,WORDP D, int start,char* op, char* compare,int quote,
	int& actualStart, int& actualEnd, bool exact,int legalgap) // is token found somewhere after start?
{
	if (start == INFINITE_MATCH) start = (reverse) ? (wordCount + 1) : 0;
	uppercaseFind = 0;
	if (deeptrace) Log(STDTRACELOG,(char*)" matchtesting:%s ",D->word);
	while (GetNextSpot(D,start,actualStart,actualEnd,reverse,legalgap)) // find a spot later where token is in sentence
    {
		if (deeptrace) Log(STDTRACELOG,(char*)" matchtest:%s %d-%d ",D->word,actualStart,actualEnd);
		if (exact && (start+1) != actualStart) return false;	// we are doing _0?~hello or whatever. Must be on the mark
 		if (deeptrace) Log(STDTRACELOG,(char*)" matchtest:ok ");
        start = actualStart; // where to try next if fail on test
        if (op) // we have a test to perform
        {
			char* word;
			if (D->word && (IsAlphaUTF8(*D->word) || D->internalBits & UTF8)) word = D->word; //   implicitly all normal words are relation tested as given
			else word = quote ? wordStarts[actualStart] : wordCanonical[actualStart];
			int id;
			if (deeptrace) Log(STDTRACELOG,(char*)" matchtest:optest ");
			char* word1val = AllocateStack(NULL,MAX_WORD_SIZE);
			char* word2val = AllocateStack(NULL, MAX_WORD_SIZE);
			FunctionResult res = HandleRelation(word, op, compare, false, id, word1val, word2val);
			ReleaseStack(word1val);
			if (res & ENDCODES) continue; // failed 
        }
 		if (*D->word == '~')  return true; // we CANNOT tell whether original or canon led to set...
		if (!quote) return true; // can match canonical or original
		
        //   we have a match, but prove it is a original match, not a canonical one
		if (actualEnd < actualStart) continue;	// trying to match within a composite. 
		if (actualStart == actualEnd && !stricmp(D->word,wordStarts[actualStart])) return true;   // literal word match
		else // match a phrase literally
		{
			char word[MAX_WORD_SIZE];
			char* at = word;
			for (int i = actualStart; i <= actualEnd; ++i)
			{
				strcpy(at,wordStarts[i]);
				at += strlen(wordStarts[i]);
				if (i != actualEnd) *at++ = '_';
			}
			*at = 0;
			if (!stricmp(D->word,word)) return true;
		}
    } 
	 if (deeptrace) Log(STDTRACELOG,(char*)" matchtest:%s failed ",D->word);
     return false;
}

static bool FindPhrase(char* word, int start,bool reverse, int & actualStart, int& actualEnd)
{   // Phrases are dynamic, might not be marked, so have to check each word separately. -- faulty in not respecting ignored(unmarked) words
	if (start > wordCount) return false;
	bool matched = false;
	actualEnd = start;
	int oldend;
	oldend = start = 0; // allowed to match anywhere or only next

	int n = BurstWord(word);
	for (int i = 0; i < n; ++i) // use the set of burst words - but "Andy Warhol" might be a SINGLE word.
	{
		WORDP D = FindWord(GetBurstWord(i));
		matched = MatchTest(reverse,D,actualEnd,NULL,NULL,0,actualStart,actualEnd,false,0);
		if (matched)
		{
			if (oldend > 0 && actualStart != (oldend + 1)) // do our words match in sequence NO. retry later in sentence
			{
				++start;
				actualStart = actualEnd = start;
				i = -1;
				oldend = start = 0;
				matched = false;
				continue;
			}
			if (i == 0) start = actualStart; // where we matched INITIALLY
			oldend = actualEnd;
		}
		else break;
	}
	if (matched) actualStart = start;
	return matched;
}

static char* PushMatch(int used)
{
	char* limit;
	char* base = InfiniteStack64(limit,"PushMatch");
	int* vals = (int*)base;
	for (int i = 0; i < used; ++i) *vals++ = wildcardPosition[i];
	char* rest = (char*) vals;
	for (int i = 0; i < used; ++i)
	{
		strcpy(rest, wildcardOriginalText[i]);
		rest += strlen(rest) + 1;
		strcpy(rest, wildcardCanonicalText[i]);
		rest += strlen(rest) + 1;
	}
	CompleteBindStack64(rest - base, base);
	return base;
}

static void PopMatch(char* base,int used)
{
	int* vals = (int*)base;
	for (int i = 0; i < used; ++i) wildcardPosition[i] = *vals++;
	char* rest = (char*)vals;
	for (int i = 0; i < used; ++i)
	{
		strcpy(wildcardOriginalText[i],rest);
		rest += strlen(rest) + 1;
		strcpy(wildcardCanonicalText[i],rest);
		rest += strlen(rest) + 1;
	}
	ReleaseStack(base);
}
#ifdef INFORMATION

We keep a positional range reference within the sentence where we are(positionStart and positionEnd).
Before we attempt the next match we make a backup copy(oldStart and oldEnd)
so that if the match fails, we can revert back to where we were(under some circumstances).

We keep a variable firstMatched to track the first real word we have matched so far.
If the whole match is declared a failure eventually, we may be allowed to go back and
retry matching starting immediately after that location. That is, we do not do all possible backtracking
as Prolog might, but we do a cheaper form where we simply try again farther in the sentence.
Also, firstMatched is returned from a subcall, so the caller can know where to end a wildcard memorization
started before the subcall.

Some tokens create a wildcard effect, where the next thing is allowed to be some distance away.
This is tracked by wildcardSelector, and the token after the wildcard, when found, is checked to see
if its position is allowed.When we enter a choice construct like[] and {}, when a choice fails,
we reset the wildcardSelector back to originalWildcardSelector so the next choice sees the same environment.

In reverse mode, the range of positionStart and positionEnd continue to be earlier and later in the sentence,
but validation treats positionStart as the basis of measuring distance.

Rebindable refers to ability to relocate firstmatched on failure (1 means we can shift from here, 3 means we enforce spacing and cannot rebind)
Some operations like < or @_0+ force a specific position, and if no firstMatch has yet happened, then you cannot change
the start location.

returnStart and returnEnd are the range of the match that happened when making a subcall.
Startposition is where we start matching from.
#endif

bool Match(char* buffer,char* ptr, unsigned int depth, int startposition, char* kind, int rebindable,unsigned int wildcardSelector,
	int &returnstart,int& returnend,int &uppercasem,int& firstMatched,int positionStart,int positionEnd, bool reverse)
{//   always STARTS past initial opening thing ( [ {  and ends with closing matching thing
	int startdepth = globalDepth;
    int wildgap = 0;
    if (wildcardSelector &  WILDGAP && *kind == '{')
        wildgap = ((wildcardSelector >> GAPLIMITSHIFT) & 0x000000ff) + 1;
	memset(&matchedBits[depth],0,sizeof(uint64) * 4);  // nesting level zone of bit matches
    char word[MAX_WORD_SIZE];
	char* orig = ptr;
	int statusBits = (*kind == '<') ? FREEMODE_BIT : 0; //   turns off: not, quote, startedgap, freemode ,wildselectorpassback
    if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACETABLOG, "%s ",kind); //   start on new indented line
    bool matched;
	unsigned int startNest = functionNest;
    int startfnvarbase = fnVarbase;
	int wildcardBase = wildcardIndex;
	unsigned int result;
	int bidirectional = 0;
	int bidirectionalSelector = 0;
	int bidirectionalWildcardIndex = 0;
	WORDP D;
	unsigned int oldtrace = trace;
	int beginmatch = -1; // for ( ) where did we actually start matching
	bool success = false;
	char* priorPiece = NULL;
	int at;
	int slidingStart = startposition;
    firstMatched = -1; //   ()  should return spot it started (firstMatched) so caller has ability to bind any wild card before it
    if (rebindable == 1)  slidingStart = positionStart = INFINITE_MATCH; //   INFINITE_MATCH means we are in initial startup, allows us to match ANYWHERE forward to start
	int originalWildcardSelector = wildcardSelector;
	positionEnd = startposition; //   we scan starting 1 after this
 	int basicStart = startposition;	//   we must not match real stuff any earlier than here
    char* argumentText = NULL; //   pushed original text from a function arg -- function arg never decodes to name another function arg, we would have expanded it instead
    uppercaseFind = -1;
	while (ALWAYS) //   we have a list of things, either () or { } or [ ].  We check each item until one fails in () or one succeeds in  [ ] or { }
    {
        int oldStart = positionStart; //  allows us to restore if we fail, and confirm legality of position advance.
        int oldEnd = positionEnd;
		int id;
		char* nextTokenStart = SkipWhitespace(ptr);
		returnPtr = nextTokenStart; // where we last were before token... we cant fail on _ and that's what we care about
		ptr = ReadCompiledWord(nextTokenStart,word);
		if (*word == '<' && word[1] == '<')  ++nextTokenStart; // skip the 1st < of <<  form
		if (*word == '>' && word[1] == '>')  ++nextTokenStart; // skip the 1st > of >>  form
		nextTokenStart = SkipWhitespace(nextTokenStart+1);	// ignore blanks after if token is a simple single thing like !
		char c = *word;
		bool foundaword = false;
		if (deeptrace) Log(STDTRACELOG,(char*)" token:%s ",word);
        switch(c) 
        {
			// prefixs on tokens
            case '!': //   NOT condition - not a stand-alone token, attached to another token
				ptr = nextTokenStart;
				statusBits |= NOT_BIT;
				if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"!");
				if (*ptr == '!') 
				{
					ptr = SkipWhitespace(nextTokenStart+1);
					statusBits |= NOTNOT_BIT;
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"!");
				}
				continue;
			case '\'': //   single quoted item    
				if (!stricmp(word,(char*)"'s"))
				{
					matched = MatchTest(reverse,FindWord(word),(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd,NULL,NULL,
						statusBits & QUOTE_BIT,positionStart,positionEnd,false,0);
					if (!matched) uppercaseFind = -1;
					else foundaword = true;
					if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart;
					break;
				}
				else
				{
					statusBits |= QUOTE_BIT;
					ptr = nextTokenStart;
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"'");
					continue;
				}
			case '_':
				// a wildcard id?  names a memorized value like _8
				if (IsDigit(word[1]))
				{
					matched = GetwildcardText(GetWildcardID(word),false)[0] != 0; // simple _2  means is it defined
					break;
				}
				// memorization coming - there can be up-to-two memorizations in progress: _* (wildmemorizegap) and _xxx (wildmemorizespecific)
				// it will be gap first and specific second (either token or smear of () [] {} )
				ptr = nextTokenStart;
				uppercaseFind = -1;
				
				// if we are going to memorize something AND we previously matched inside a phrase, we need to move to after phrase...
				if ((positionStart - positionEnd) == 1 && !reverse) positionEnd = positionStart; // If currently matched a phrase, move to end. 
				else if ((positionEnd - positionStart) == 1 && reverse) positionStart = positionEnd; // If currently matched a phrase, move to end. 
				
				//  aba or ~dat or **ar*
				if (ptr[0] != '*' || ptr[1] == '*') // wildcard word
				{
					wildcardSelector |= (WILDMEMORIZESPECIFIC + (wildcardIndex << SPECIFIC_SHIFT)); 
				}
				// *1 or *-2 or *elle (single wild token pattern match)
				else if (IsDigit(ptr[1]) ||  ptr[1] == '-' || IsAlphaUTF8(ptr[1])) 
				{
					wildcardSelector |= (WILDMEMORIZESPECIFIC + (wildcardIndex << SPECIFIC_SHIFT)); 
				}
				else // *~ or *
				{
					wildcardSelector |=  (WILDMEMORIZEGAP + (wildcardIndex << GAP_SHIFT)); // variable gap
				}
				SetWildCardNull(); // dummy match to reserve place
				if (trace & TRACE_PATTERN  && CheckTopicTrace() && bidirectional != 2) Log(STDTRACELOG,(char*)"_");
				continue;
			case '@': // factset ref
				if (word[1] == '_') // set positional reference  @_20+ or @_0-   or anchor @_20
				{
					if (firstMatched < 0) firstMatched = NORETRY; // cannot retry this match locally
					char* end = word+3;  // skip @_2
					if (IsDigit(*end)) ++end; // point to proper + or - ending
					unsigned int wild = wildcardPosition[GetWildcardID(word+1)];
					int index = (wildcardSelector >> GAP_SHIFT) & 0x0000001f;

					// memorize gap to end based on direction...
					if (*end && (wildcardSelector & WILDMEMORIZEGAP) && !reverse) // close to end of sentence 
					{
						positionStart = wordCount; // pretend to match at end of sentence
						int start = wildcardSelector & 0x000000ff;
						int limit = (wildcardSelector >> GAPLIMITSHIFT) & 0x000000ff;
  						if ((positionStart + 1 - start) > limit) //   too long til end
						{
							matched = false;
 							wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
							break;
						}
						if (wildcardSelector & WILDMEMORIZEGAP) 
						{
							if ((wordCount - start) == 0) SetWildCardGivenValue((char*)"",(char*)"",true,start,index); // empty gap
							else SetWildCardGiven(start,wordCount,true,index );  //   wildcard legal swallow between elements
 							wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
						}
					}
					
					// memorize gap to end based on direction...
					if (*end && (wildcardSelector & WILDMEMORIZEGAP) && reverse) // close to start of sentence 
					{
						positionEnd = 1; // pretend to match at end of sentence
						int start = wildcardSelector & 0x000000ff;
						int limit = (wildcardSelector >> GAPLIMITSHIFT) & 0x000000ff;
  						if ((start - positionEnd) > limit) //   too long til end
						{
							matched = false;
 							wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
							break;
						}
						if (wildcardSelector & WILDMEMORIZEGAP) 
						{
							if ((start - positionEnd + 1) == 0) SetWildCardGivenValue((char*)"",(char*)"",true,start,index); // empty gap
							else SetWildCardGiven(positionEnd,start,true,index );  //   wildcard legal swallow between elements
 							wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
						}
					}

					if (*end == '-') 
					{
						reverse = true;
						oldEnd = positionEnd = positionStart = WILDCARD_START(wild);
					}
					else // + and nothing both move forward. 
					{
						int oldend = positionEnd; // will be 0 if we are starting out with no required match
						positionStart = WILDCARD_START(wild);
						positionEnd = WILDCARD_END(wild);
						if (positionStart < oldend && !reverse) // we are trying to jump backwards
						{
							matched = false;
							break;
						}
						else if (positionStart > oldend && reverse)
						{
							matched = false;
							break;
						}
						reverse = false;
						if (!wildcardSelector && oldend && *end != '+' && positionStart != (oldend + 1) && positionStart != INFINITE_MATCH) // this is an anchor that does not match
						{
							matched = false;
							break;
						}
					}
					if (!positionEnd && positionStart) 
					{
						matched = false;
						break;
					}
					if (*end)
					{
						oldEnd = positionEnd; // forced match ok
						oldStart = positionStart;
					}
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) 
					{
						if (positionStart <= 0 || positionStart > wordCount || positionEnd <= 0 || positionEnd > wordCount) Log(STDTRACELOG, "(index:%d)",positionEnd);
						else if (positionStart == positionEnd) Log(STDTRACELOG,(char*)"(word:%s index:%d)",wordStarts[positionEnd],positionEnd);
						else Log(STDTRACELOG,(char*)"(word:%s-%s index:%d-%d)",wordStarts[positionStart],wordStarts[positionEnd],positionStart,positionEnd);
					}
					if (beginmatch == -1) beginmatch = positionStart; // treat this as a real match
					matched = true;
					if (rebindable) rebindable = 3;	// not allowed to rebind from this, it is a fixed location
				}
				else
				{
					int set = GetSetID(word);
					if (set == ILLEGAL_FACTSET) matched = false;
					else matched = FACTSET_COUNT(set) != 0;
				}
				break;
   			case '<': //   sentence start marker OR << >> set
				if (firstMatched < 0) firstMatched = NORETRY; // cannot retry this match
				if (word[1] == '<')  goto DOUBLELEFT; //   << 
				at = 0;
				while (unmarked[++at] && at <= wordCount){;} // skip over hidden data
				--at; // we perceive the start to be here
				ptr = nextTokenStart;
				if ((wildcardSelector & WILDGAP) && !reverse) // cannot memorize going forward to  start of sentence
				{
					matched = false;
 					wildcardSelector &= -1 ^ (WILDMEMORIZEGAP|WILDGAP);
				}
				else { // match can FORCE it to go to start from any direction
					positionStart = positionEnd = at; //   idiom < * and < _* handled under *
					matched = true;
				}
				if (matched && rebindable) rebindable = 3;	// not allowed to rebind from this, it is a fixed location
                break;
            case '>': //   sentence end marker
				if (word[1] == '>')  goto DOUBLERIGHT; //   >> closer, and reset to start of sentence wild again...
				at = positionEnd;
				while (unmarked[++at] && at <= wordCount){;} // skip over hidden data
				if (at > wordCount) at = positionEnd;	// he was the end
				else at = wordCount; // the presumed real end
				uppercaseFind = -1;
				ptr = nextTokenStart;
				if (*kind == '[') rebindable = 2; // let outer level decide if it is right  
				// # get 3 days forecast in Orlando, New York | London and San Francisco
				//	u: (_and) _10 = _0
				//	u: TEST (@_10+ * _[ ~arrayseparator > ])

				if ((wildcardSelector & WILDGAP) && reverse) // cannot memorize going backward to  end of sentence
				{
					matched = false;
 					wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
				}
				else if (reverse)
				{				
					// match can FORCE it to go to end from any direction
					positionStart = positionEnd = wordCount + 1; 
					matched = true;
				}
				else if ((wildcardSelector & WILDGAP) || positionEnd == at)// you can go to end from anywhere if you have a gap OR you are there
				{
					matched =  true;
					positionStart = positionEnd = at+1; //   pretend to match a word off end of sentence
				}
				else if (*kind == '[' || *kind == '{') // nested unit will figure out if legal 
				{
					matched =  true;
					positionStart = positionEnd = at+1; //   pretend to match a word at end of sentence
				}
				else if (positionStart == INFINITE_MATCH) 
				{
					positionStart = positionEnd = wordCount + 1; 
					matched = true;
				}
				else matched = false;
				if (matched && rebindable && rebindable != 2) rebindable = 3;	// not allowed to rebind from this, it is a fixed location
 				break;
             case '*':
				 uppercaseFind = -1;
				if (beginmatch == -1) beginmatch = startposition + 1;
				if (word[1] == '-') //   backward grab, -1 is word before now -- BUG does not respect unmark system
				{
					int at;
					if (!reverse) at = positionEnd - (word[2] - '0') - 1; // limited to 9 back
					else  at = positionEnd + (word[2] - '0') - 1; 
					if (reverse && at > wordCount)
					{
						oldEnd = at; //   set last match AFTER our word
						positionStart = positionEnd = at - 1; //   cover the word now
						matched = true; 
					}
					else if (!reverse && at >= 0) //   no earlier than pre sentence start
					{
						oldEnd = at; //   set last match BEFORE our word
						positionStart = positionEnd = at + 1; //   cover the word now
						matched = true; 
					}
					else matched = false;
				}
				else if (IsDigit(word[1]))  // fixed length gap
                {
					int at;
					int count = word[1] - '0';	// how many to swallow
					if (reverse)
					{
						int begin = positionStart -1;
						at = positionStart; // start here
						while (count-- && --at >= 1) // can we swallow this (not an ignored word)
						{
							if (unmarked[at]) 
							{
								++count;	// ignore this word
								if (at == begin) --begin;	// ignore this as starter
							}
						}
						if (at >= 1 ) // pretend match
						{ 
							positionEnd = begin ; // pretend match here -  wildcard covers the gap
							positionStart = at; 
							matched = true; 
						}
						else  matched = false;
					}
					else
					{
						at = positionEnd; // start here
						int begin = positionEnd + 1;
						while (count-- && ++at <= wordCount) // can we swallow this (not an ignored word)
						{
							if (unmarked[at]) 
							{
								++count;	// ignore this word
								if (at == begin) ++begin;	// ignore this as starter
							}
						}
						if (at <= wordCount ) // pretend match
						{ 
							positionStart = begin; // pretend match here -  wildcard covers the gap
 							positionEnd = at; 
							matched = true; 
						}
						else  matched = false;
					}
                }
				else if (IsAlphaUTF8(word[1]) || word[1] == '*') 
					matched = FindPartialInSentenceTest(word+1,(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd,positionStart,reverse,
					positionStart,positionEnd); // wildword match like st*m* or *team* matches steamroller
                else // variable gap
                {
					int start = (reverse) ? (positionStart-1) : (positionEnd+1);
					wildcardSelector |= start | WILDGAP; // cannot conflict, two wilds in a row change no position
					if (word[1] == '~') 
					{
						wildcardSelector |= (word[2]-'0') << GAPLIMITSHIFT; // *~3 - limit is 9 back
						if (word[strlen(word) - 1] == 'b')
						{
							bidirectional = 1; // now aiming backwards
							priorPiece = ptr;
							bidirectionalSelector = wildcardSelector; // where to start forward direction if backward fails
							reverse = !reverse; // run inverted first (presumably backward)
							start = (reverse) ? (positionStart - 1) : (positionEnd + 1); 
							wildcardSelector &= -1 ^ 0x000000ff;
							wildcardSelector |= start;
							bidirectionalWildcardIndex = wildcardIndex;
						}
					}
                    else // I * meat
					{
						wildcardSelector |= 200 << GAPLIMITSHIFT;  // 200 is a safe infinity // I * meat
						if (positionStart == 0) positionStart = INFINITE_MATCH; // < * resets to allow match anywhere
					}
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"%s ",word);
					continue;
                }
                break;
            case USERVAR_PREFIX: // is user variable defined
				if (IsAlphaUTF8(word[1]) || word[1] == '_' || word[1] == USERVAR_PREFIX) // legal variable, not $ or $100
				{
					char* val = GetUserVariable(word);
					matched = *val ? true : false;
				}
				else goto matchit;
                break;
            case '^': //   function call, function argument  or indirect function variable assign ref like ^$$tmp = null
                if  (IsDigit(word[1]) || word[1] == USERVAR_PREFIX || word[1] == '_') //   macro argument substitution or indirect function variable
                {
                    argumentText = ptr; //   transient substitution of text

					if (IsDigit(word[1]))  ptr = FNVAR(word+1);  
					else if (word[1] == USERVAR_PREFIX) ptr = GetUserVariable(word+1); // get value of variable and continue in place
					else ptr = wildcardCanonicalText[GetWildcardID(word+1)]; // ordinary wildcard substituted in place (bug)?
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"%s=>",word);
					continue;
                }
                
				D = FindWord(word,0); // find the function
				if (!D || !(D->internalBits & FUNCTION_NAME)) matched = false; // shouldnt fail
				else if (D->x.codeIndex || D->internalBits & IS_OUTPUT_MACRO) // system function or output macro- execute it
                {
					char* base = PushMatch(wildcardIndex);
					int baseindex = wildcardIndex;
					AllocateOutputBuffer();
					FunctionResult result;
					if (!stricmp(D->word,"^match") || !stricmp(D->word, "^mark") || !stricmp(D->word, "^unmark")) matching = true;
					ptr = DoFunction(word,ptr,currentOutputBase,result);
					matching = false;
					PopMatch(base, baseindex);
					matched = !(result & ENDCODES);

					// allowed to do comparisons on answers from system functions but cannot have space before them, but not from user macros
					if (*ptr == '!' && ptr[1] == ' ' )// simple not operator or no operator
					{
						if (!stricmp(currentOutputBase,(char*)"0") || !stricmp(currentOutputBase,(char*)"false")) result = FAILRULE_BIT;	// treat 0 and false as failure along with actual failures
					}
					else if (ptr[1] == '<' || ptr[1] == '>'){;} // << and >> are not comparison operators in a pattern
					else if (IsComparison(*ptr) && *(ptr-1) != ' ' && (*ptr != '!' || ptr[1] == '='))  // ! w/o = is not a comparison
					{
						char op[10];
						char* opptr = ptr;
						*op = *opptr;
						op[1] = 0;
						char* rhs = ++opptr; 
						if (*opptr == '=') // was == or >= or <= or &= 
						{
							op[1] = '=';
							op[2] = 0;
							++rhs;
						}
						char copy[MAX_WORD_SIZE];
						ptr = ReadCompiledWord(rhs,copy);
						rhs = copy;

						if (*rhs == '^') // local function argument or indirect ^$ var  is LHS. copy across real argument
						{
							char* at = "";
							if (rhs[1] == USERVAR_PREFIX) at = GetUserVariable(rhs+1); 
							else if (IsDigit(rhs[1])) at = FNVAR(rhs+1);
							at = SkipWhitespace(at);
							strcpy(rhs,at);
						}
				
						if (*op == '?' && opptr[0] != '~')
						{
							matched = MatchTest(reverse,FindWord(currentOutputBase),
								(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd,NULL,NULL,false,
								positionStart,positionEnd,false,0); 
							uppercaseFind = -1;
							if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
						}
						else
						{
							int id;
							char* word1val = AllocateStack(NULL, MAX_WORD_SIZE);
							char* word2val = AllocateStack(NULL, MAX_WORD_SIZE);
 							result = HandleRelation(currentOutputBase,op,rhs,false,id,word1val,word2val); 
							ReleaseStack(word1val);
							matched = (result & ENDCODES) ? 0 : 1;
						}
					}
					FreeOutputBuffer();
                }
				else // user patternmacro function - execute it in pattern context as continuation of current code
				{ 
					if (functionNest >= MAX_PAREN_NEST) // fail, too deep nesting
					{
						matched = false;
						break; 
					}

					//   save old base data
					baseStack[functionNest] = callArgumentBase; 
					argStack[functionNest] = callArgumentIndex; 

					if ((trace & TRACE_PATTERN || D->internalBits & MACRO_TRACE)  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"("); 
					ptr += 2; // skip ( and space
					FunctionResult result = NOPROBLEM_BIT;
					// read arguments
					while (*ptr && *ptr != ')' ) 
					{
						ptr = ReadArgument(ptr,buffer,result);  // gets the unevealed arg
						callArgumentList[callArgumentIndex++] = AllocateStack(buffer);
						if ((trace & TRACE_PATTERN || D->internalBits & MACRO_TRACE)  && CheckTopicTrace()) Log(STDTRACELOG,(char*)" %s, ",buffer); 
						*buffer = 0;
						if (result != NOPROBLEM_BIT) 
						{
							matched = false;
							break;
						}
					}
					if ((trace & TRACE_PATTERN || D->internalBits & MACRO_TRACE)  && CheckTopicTrace()) Log(STDTRACELOG,(char*)")\r\n"); 
                    callArgumentBase =  fnVarbase = argStack[functionNest] - 1;
                    ptrStack[functionNest++] = ptr + 2; // skip closing paren and space
					ptr = (char*)FindAppropriateDefinition(D, result); 
					if (ptr)
					{
						char* word = AllocateStack(NULL,MAX_WORD_SIZE);
						ptr = ReadCompiledWord(ptr, word) + 2; // eat flags and count and (
						while (*ptr) // skip over locals list (which should be null)
						{
							ptr = ReadCompiledWord(ptr, word);
							if (*word == ')') break;
						}
						ReleaseStack(word);
					}
					else ptr = ""; // null function
					oldtrace = trace;
					if (D->internalBits & MACRO_TRACE  && CheckTopicTrace()) 
					{
						trace = (unsigned int)-1;
						if (oldtrace && !(oldtrace & TRACE_ECHO)) trace ^= TRACE_ECHO;
					}
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACELOG,(char*)"%s=> ",word);
					if (result == NOPROBLEM_BIT) continue;
				}
				if (result == FAILRULE_BIT) matched = false;
				break;
          case 0: case '`': // end of data (argument or function - never a real rule)
	           if (argumentText) // return to normal from argument substitution
                {
                    ptr = argumentText;
                    argumentText = NULL;
                    continue;
                }
                else if (functionNest > startNest) // function call end
                {
 					if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(STDTRACETABLOG,(char*)""); 
					--functionNest;
                    callArgumentIndex = argStack[functionNest]; //   end of argument list (for next argument set)
                    fnVarbase = callArgumentBase = baseStack[functionNest]; //   base of callArgumentList
                    ptr = ptrStack[functionNest]; // continue using prior code
					trace = (modifiedTrace) ? modifiedTraceVal : oldtrace;
					continue;
                }
                else 
				{
					globalDepth = startdepth;
					uppercaseFind = -1;
 					return false; // shouldn't happen
				}
                break;
DOUBLELEFT:  case '(': case '[':  case '{': // nested condition (required or optional) (= consecutive  [ = choice   { = optional   << all of
				// we make << also a depth token
				uppercaseFind = -1;
				ptr = nextTokenStart;
				{
					int returnStart = positionStart; // default return for range start
					int returnEnd = positionEnd;  // default return for range end
					int rStart = positionStart; // refresh values of start and end for failure to match
					int rEnd = positionEnd;
					unsigned int oldselect = wildcardSelector;
					int uppercasemat = 0;
					// nest inherits gaps leading to it. memorization requests withheld til he returns
					int whenmatched = 0;
					char* type = "[";
					if (*word == '(') type = "(";
					else if (*word == '{') type = "{";
					else if (*word == '<') 
					{
						type = "<<";
						positionEnd = startposition;  //   allowed to pick up after here - oldStart/oldEnd synch automatically works
						positionStart = INFINITE_MATCH;
						rEnd = 0;
						rStart = INFINITE_MATCH; 
					}
					int localRebindable = 0; // not allowed to try rebinding start again by default
					if (positionStart == INFINITE_MATCH) localRebindable = 1; // we can move the start
					if (oldselect & WILDGAP) localRebindable = 2; // allowed to gap in 
					int select = wildcardSelector;
					if (select & WILDMEMORIZEGAP) // dont memorize within, do it out here.
					{
						select ^= WILDMEMORIZEGAP;
						select |= WILDGAP;
					}
					else if (select & WILDMEMORIZESPECIFIC) select ^= WILDMEMORIZESPECIFIC;

                    matched = Match(buffer,ptr,depth+1,positionEnd,type, localRebindable,select,returnStart,
						returnEnd,uppercasemat,whenmatched,positionStart,positionEnd,reverse); //   subsection ok - it is allowed to set position vars, if ! get used, they dont matter because we fail
					wildcardSelector = oldselect; // restore outer environment
                    if (matched)
					{
                        // return positions are always returned forward looking
                        if (reverse && returnStart > returnEnd)
                        {
                            int x = returnStart;
                            returnStart = returnEnd;
                            returnEnd = x;
                        }

						positionStart = returnStart;
						if (positionStart == INFINITE_MATCH && returnStart > 0 &&  returnStart != INFINITE_MATCH) positionStart = returnEnd;
						positionEnd = returnEnd;
		
						// copy back marking bits on match
						if (!(statusBits & NOT_BIT)) // wanted match to happen
						{
							int olddepth = depth + 1;
							matchedBits[depth][0] |= matchedBits[olddepth][0];
							matchedBits[depth][1] |= matchedBits[olddepth][1];
							matchedBits[depth][2] |= matchedBits[olddepth][2];
							matchedBits[depth][3] |= matchedBits[olddepth][3];
							if (beginmatch == -1) beginmatch = positionStart; // first match in this level
							if (firstMatched < 0) firstMatched = whenmatched;
						}
						// allow single matches to propogate back thru
						if (positionStart != positionEnd) uppercaseFind = uppercasemat;
						if (*word == '<') // allows thereafter to be anywhere
						{
							positionStart = INFINITE_MATCH;
							oldEnd = oldStart = positionEnd = 0;
						}
						// The whole thing matched but if @_ was used, which way it ran and what to consider the resulting zone is completely confused.
						// So force a tolerable meaning so it acts like it is contiguous to caller.  If we are memorizing it may be silly but its what we can do.
						if (*word == '(' && positionStart == NORETRY) 
						{
							positionEnd = positionStart = (reverse) ? (oldStart - 1) : (oldEnd + 1) ;  // claim we only moved 1 unit
						}
						else if (positionEnd) oldEnd = (reverse) ? (positionEnd + 1) : (positionEnd - 1); //   nested checked continuity, so we allow match whatever it found - but not if never set it (match didnt have words)
					}
					else if (*word == '{')  // we didnt match, but we are not required to
					{
						if (wildcardSelector & WILDMEMORIZESPECIFIC) // was already inited to null when slot allocated
						{ // what happens with ( *~3 {boy} bottle) ? // not legal
							wildcardSelector ^= WILDMEMORIZESPECIFIC; // do not memorize it further
						}
                        originalWildcardSelector = wildcardSelector;
                        statusBits |= NOT_BIT; //   we didnt match and pretend we didnt want to
					}
					else // no match for ( or [ or << means we have to restore old positions regardless of what happened inside
					{ // but we should check why the positions were not properly restored from the match call...BUG
						positionStart = rStart;
						positionEnd = rEnd;
						wildcardSelector = 0; // failure of [ and ( and << loses all memorization
					}
				} // just a data block
				ptr = BalanceParen(ptr,true,false); // reserve wildcards as we skip over the material including closer 
                break;

 DOUBLERIGHT: case ')': case ']': case '}' :  //   end sequence/choice/optional
				ptr = nextTokenStart;
				matched = (*kind == '(' || *kind == '<'); //   [] and {} must be failures if we are here while ( ) and << >> are success
				if (wildcardSelector & WILDGAP) //   pending gap  -  [ foo fum * ] and { foo fot * } are  illegal but [*3 *2] is not 
                {
					if (depth != 0) //  don't end with a gap, illegal
					{
						wildcardSelector = 0;
						matched = false; //   force match failure
					}
					else if (reverse) 
					{
						positionEnd = positionStart - 1; 
						positionStart = 1;
					}
					else positionStart = wordCount + 1; //   at top level a close implies > )
				}
				if (matched && depth > 0 && *word == ')') // matched all at this level? wont be true if it uses < inside it
				{
					if (slidingStart && slidingStart != INFINITE_MATCH) positionStart = (reverse) ? (startposition  - 1) : (startposition  + 1);
					else
					{
						// if ( ) started wild like (* xxx) we need to start from beginning
						// if ()  started matchable like (tom is) we need to start from starting match

						if (beginmatch != -1) positionStart =  beginmatch; // handle ((* test)) and ((test))
						else positionStart = (reverse) ? (startposition  - 1) : (startposition  + 1); // probably wrong on this one
					}
				}
                break; 
            case '"':  //   double quoted string
				matched = FindPhrase(word,(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, reverse,
					positionStart,positionEnd);
				uppercaseFind = -1;
				if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
				break;
            case SYSVAR_PREFIX: //   system variable
				if (!word[1]) // simple % 
				{
					matched = MatchTest(reverse,FindWord(word),(positionEnd < basicStart && firstMatched < 0) ? basicStart: positionEnd,NULL,NULL,
						statusBits & QUOTE_BIT,positionStart,positionEnd,false,0); //   possessive 's
					uppercaseFind = -1;
					if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
				}
                else matched = SysVarExists(word);
                break;
            case '?': //  question sentence? or variable search for 
				if (word[1] == '$')
				{
					strcpy(word, GetUserVariable(word+1));
					goto matchit;
				}
				else
				{
					ptr = nextTokenStart;
					if (!word[1]) matched = (tokenFlags & QUESTIONMARK) ? true : false;
					else matched = false;
				}
	            break;
            case '=': //   a comparison test - never quotes the left side. Right side could be quoted
				//   format is:  = 1-bytejumpcodeToComparator leftside comparator rightside
				if (!word[1]) //   the simple = being present
				{
					matched = MatchTest(reverse,FindWord(word),(positionEnd < basicStart && firstMatched < 0)  ? basicStart : positionEnd,NULL,NULL,
						statusBits & QUOTE_BIT,positionStart,positionEnd,false,0); //   possessive 's
					uppercaseFind = -1;
					if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
				}
				//   if left side is anything but a variable $ or _ or @, it must be found in sentence and that is what we compare against
				else 
				{
					char lhsside[MAX_WORD_SIZE];
					char* lhs = lhsside;
					char op[10];
					char rhsside[MAX_WORD_SIZE];
					char* rhs = rhsside;
					DecodeComparison(word, lhs, op, rhs);
					if (trace & TRACE_PATTERN) sprintf(word,(char*)"%s%s%s",lhs,op,rhs);
					if (*lhs == '^') DecodeFNRef(lhs); // local function arg indirect ^$ var or _ as LHS
					if (*rhs == '^') DecodeFNRef(rhs);// local function argument or indirect ^$ var  is LHS. copy across real argument
				
					bool quoted = false;
					if (*lhs == '\'') // left side is quoted
					{
						++lhs; 
						quoted = true;
					}
			
					if (*op == '?' && *rhs != '~') // NOT a ? into a set test - means does this thing exist in sentence
					{
						char* val = "";
						if (*lhs == USERVAR_PREFIX) val = GetUserVariable(lhs);
						else if (*lhs == '_') val = (quoted) ? wildcardOriginalText[GetWildcardID(lhs)] : wildcardCanonicalText[GetWildcardID(lhs)];
						else if (*lhs == '^' && IsDigit(lhs[1])) val = FNVAR(lhs+1);  
						else if (*lhs == SYSVAR_PREFIX) val = SystemVariable(lhs,NULL);
						else val = lhs; // direct word

						if (*val == '"') // phrase requires dynamic matching
						{
							matched = FindPhrase(val,(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, reverse,
								positionStart,positionEnd);
							if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
							if (trace & TRACE_PATTERN) sprintf(word,(char*)"%s(%s)%s",lhs,val,op);
							break;
						}

						matched = MatchTest(reverse,FindWord(val),(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd,NULL,NULL,
							quoted,positionStart,positionEnd,false,0); 
						uppercaseFind = -1;
						if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
						if (trace & TRACE_PATTERN) sprintf(word,(char*)"%s(%s)%s",lhs,val,op);
						break;
					}
	
					result = *lhs;
					if (IsComparison(*op)) // otherwise for words and concepts, look up in sentence and check relation there
					{
						if (result == '_' && quoted) --lhs; // include the quote
						char* word1val = AllocateStack(NULL, MAX_WORD_SIZE);
						char* word2val = AllocateStack(NULL, MAX_WORD_SIZE);
						FunctionResult answer = HandleRelation(lhs,op,rhs,false,id,word1val,word2val);
						matched = (answer & ENDCODES) ? 0 : 1;
						if (trace  & TRACE_PATTERN) 
						{
							if (!stricmp(lhs,word1val)) *word1val = 0; // dont need redundant constants in trace
							if (!stricmp(rhs,word2val)) *word2val = 0; // dont need redundant constants in trace
							if (*word1val && *word2val) sprintf(word,(char*)"%s(%s)%s%s(%s)",lhs,word1val,op,rhs,word2val);
							else if (*word1val) sprintf(word,(char*)"%s(%s)%s%s",lhs,word1val,op,rhs);
							else if (*word2val) sprintf(word,(char*)"%s%s%s(%s)",lhs,op,rhs,word2val);
							else sprintf(word,(char*)"%s%s%s",lhs,op,rhs);
						}
						ReleaseStack(word1val);
					}
					else // find and test
					{
						matched = MatchTest(reverse,FindWord(lhs),(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd,op,rhs,
							quoted,positionStart,positionEnd,false,0); //   MUST match later 
						uppercaseFind = -1;
						if (!matched) break;
					}
 				}
				break;
            case '\\': //   escape to allow [ ] () < > ' {  } ! as words and 's possessive And anything else for that matter
				{
					matched =  MatchTest(reverse,FindWord(word+1),(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd,NULL,NULL,
						statusBits & QUOTE_BIT,positionStart,positionEnd,false,0);
					uppercaseFind = -1;
					if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart;
					if (matched) {}
					else if (word[1] == '!' ) matched =  (wordCount && (tokenFlags & EXCLAMATIONMARK)); //   exclamatory sentence
  					else if (word[1] == '?') matched =  (tokenFlags & QUESTIONMARK) ? true : false; //   question sentence
					break;
				}
			case '~': // current topic ~ and named topic
				if (word[1] == 0) // current topic
				{
					matched = IsCurrentTopic(currentTopicID); // clearly we are executing rules from it but is the current topic interesting
					break;
				}
				// drop thru for all other ~
			default: //   ordinary words, concept/topic, numbers, : and ~ and | and & accelerator
			matchit:
				matched = MatchTest(reverse,FindWord(word),(positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd,NULL,NULL,
					statusBits & QUOTE_BIT,positionStart,positionEnd,false,wildgap);
				
				if (!matched) uppercaseFind = -1;
				else foundaword = true;
				if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart;
         } 

		statusBits &= -1 ^ QUOTE_BIT; // turn off any pending quote
		bool inverted = false;
        if (statusBits & NOT_BIT) // flip success to failure maybe
        {
			inverted = true;
			if (matched)
			{
				if (statusBits & NOTNOT_BIT) // is match immediately after or not
				{
					if (!reverse && positionStart == (oldEnd + 1))
					{
						matched = false;
						uppercaseFind = -1;
					}
					else if (reverse && positionEnd == (oldStart - 1))
					{
						matched = false;
						uppercaseFind = -1;
					}
				}
				else
				{
					matched = false;
					uppercaseFind = -1;
				}
				statusBits &= -1 ^ (NOT_BIT|NOTNOT_BIT);
				positionStart = oldStart; //   restore any changed position values (if we succeed we would and if we fail it doesnt harm us)
				positionEnd = oldEnd;
			}
        }

		//   prove any wild gap was legal, accounting for ignored words if needed
 		unsigned int started;
		if (!reverse) started = (positionStart < REAL_SENTENCE_LIMIT) ? positionStart : 0; // position start may be the unlimited access value
		else 
		{
			if (positionEnd > wordCount) positionEnd = wordCount;
			started = (positionStart < REAL_SENTENCE_LIMIT) ? positionEnd : wordCount; // position start may be the unlimited access value
		}
		if (started == INFINITE_MATCH) started = 1;
		bool legalgap = false;
		unsigned int memorizationStart = positionStart;
        if ((wildcardSelector & WILDGAP) && matched) // test for legality of gap
        {
			unsigned int begin = started; // where we think we are now at match
			memorizationStart = started = (wildcardSelector & 0x000000ff); // actual word we started at
			unsigned int ignore = started;
			int x;
			int limit = (wildcardSelector >> GAPLIMITSHIFT) & 0x000000ff;
			if (reverse)
			{
				x = started - begin; // *~2 debug() something will generate a -1 started... this is safe here
				while (ignore > begin) // no charge for ignored words in gap
				{
					if (unmarked[ignore--]) --x; 
				}
			}
			else
			{
				x = begin - started; // *~2 debug() something will generate a -1 started... this is safe here
				while (ignore < begin) // no charge for ignored words in gap
				{
					if (unmarked[ignore++]) --x; 
				}
			}
			if (x < 0) legalgap = false; // if searched _@10- *~4 > 
  			else if (x <= limit) legalgap = true;   //   we know this was legal, so allow advancement test not to fail- matched gap is started...oldEnd-1
			else  
			{
				matched = false;  // more words than limit
				wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP); //   turn off any save flag
			}
		}
		if (matched) // perform any memorization
		{
			if (matched && !inverted && foundaword) // we accept this positive match
			{
				MarkMatchLocation(positionStart, positionEnd, depth);
				if (beginmatch == -1) beginmatch = positionStart; // first match in this level
			}
			if (oldEnd == positionEnd && oldStart == positionStart) // something like function call or variable existence, didnt change position
			{
				if (wildcardSelector == WILDMEMORIZESPECIFIC)
				{
					if (*word == USERVAR_PREFIX)
					{
						int hold = uppercaseFind; // we dont know the content, just what we hunted for
						uppercaseFind = -1;
						char* value = GetUserVariable(word);
						SetWildCard(value,value, 0,0);  // specific swallow
						uppercaseFind = hold;
					}
				}
			}
			else if (wildcardSelector & (WILDMEMORIZEGAP |WILDMEMORIZESPECIFIC )) //   memorize ONE -  will be gap or specific not in front of () {} []
			{
				if (started == INFINITE_MATCH) started = 1;
				if (positionStart == INFINITE_MATCH) positionStart = 1;
				if (wildcardSelector & WILDMEMORIZEGAP) //   would be first if both
				{
					int hold = uppercaseFind;
					uppercaseFind = -1;
					int index = (wildcardSelector >> GAP_SHIFT) & 0x0000001f;
					if (reverse)
					{
						if ((started - positionStart) == 0) SetWildCardGivenValue((char*)"",(char*)"",0,positionEnd+1,index); // empty gap
						else SetWildCardGiven(positionStart +1, memorizationStart,true,index );  //   wildcard legal swallow between elements
					}	
					else if ((positionStart - memorizationStart) == 0) SetWildCardGivenValue((char*)"",(char*)"",0,oldEnd+1, index); // empty gap
					else 
					{
						SetWildCardGiven(memorizationStart,positionStart-1,true, index);  //   wildcard legal swallow between elements
						if (positionEnd < (positionStart - 1)) positionEnd = positionStart - 1;	// gap closes out 
					}
					uppercaseFind = hold;
				}
				if (wildcardSelector & WILDMEMORIZESPECIFIC) 
				{
					int index = (wildcardSelector >> SPECIFIC_SHIFT) & 0x0000001f;
					SetWildCardGiven(positionStart,positionEnd,true,index);  // specific swallow 
					if (strchr(wildcardCanonicalText[index],' ')) // is lower case canonical a dictionary word with content?
					{
						char* word = AllocateStack(NULL,MAX_WORD_SIZE);
						strcpy(word,wildcardCanonicalText[index]);
						char* at = word;
						while ((at = strchr(at,' '))) *at = '_';
						WORDP D = FindWord(word,0); // find without underscores..
						if (D && D->properties & PART_OF_SPEECH)  strcpy(wildcardCanonicalText[index],D->word);
						ReleaseStack(word);
					}
				}
			}
			wildcardSelector = 0; // completes all memorization/gaps at this level
		}
		else //   fix side effects of anything that failed to match by reverting
        {
            positionStart = oldStart;
            positionEnd = oldEnd;
            if (*word == '{')
                wildcardSelector = originalWildcardSelector; // failed optional match after return from calling it, need to restore any wildcard eg:  test *~1 {me} [ready]
            else if (*kind == '(' || *kind == '<') wildcardSelector = 0; /// should NOT clear this inside a [] or a {} on failure since they must try again
			else // [] and {} get alternate attemps so reset context
			{
				wildcardSelector = originalWildcardSelector;
				beginmatch = -1;
				firstMatched = -1;
			}
		}

		// manage bidirectional failure or success
		if (bidirectional)
		{
			if (matched)
			{
				if (bidirectional == 1) reverse = !reverse;
				bidirectional = 0;
			}
			else if (bidirectional == 1)
			{
				reverse = !reverse;
				bidirectional = 2;
				ptr = priorPiece;
				wildcardSelector = bidirectionalSelector;
				wildcardIndex = bidirectionalWildcardIndex;
				continue;
			}
			else bidirectional = 0; // give up fully
		}

        //   end sequence/choice/optional/random
        if (*word == ')' || *word ==  ']' || *word == '}' || (*word == '>' && word[1] == '>')) 
        {
			success = matched != 0; 
			if (success && argumentText) //   we are ok, but we need to resume old data
			{
				ptr = argumentText;
				argumentText = NULL;
				continue;
			}

			break;
        }

		//   postprocess match of single word or paren expression
		if (statusBits & NOT_BIT) //   flip failure result to success now (after wildcardsetting doesnt happen because formally match failed first)
        {
            matched = true; 
			statusBits &= -1 ^ (NOT_BIT|NOTNOT_BIT);
         }

		//   word ptr may not advance more than 1 at a time (allowed to advance 0 - like a string match or test) unless global unmarks in progress
        //   But if initial start was INFINITE_MATCH, allowed to match anywhere to start with
		if (legalgap || !matched  || positionStart == INFINITE_MATCH || oldStart == INFINITE_MATCH) {;}
		else if (reverse)
		{
			if (oldStart < oldEnd && positionEnd >= (oldStart - 1) ){;} // legal move ahead given matched WITHIN last time
			else if (positionEnd < (oldStart - 1 ))  // failed to match position advance
			{
				int ignored = oldStart - 1;
				if (oldStart && unmarked[ignored]) while (--ignored > positionEnd && unmarked[ignored]); // dont have to account for these
				if (ignored != positionStart) // position track failed
				{
					if (firstMatched == positionStart) firstMatched = 0; // drop recog of it
					matched = false;
					positionStart = oldStart;
					positionEnd = oldEnd;
				}
			}
		}
		else if (!legalgap && rebindable != 2) // forward requirement must be tested (1 means we are allowed to rebind)
		{
			if (oldEnd < oldStart && positionStart <= (oldStart + 1)){;} // legal move ahead given matched WITHIN last time -- what does match within mean?
			else if (positionStart > (oldEnd + 1))  // failed to match position advance of one
			{
				int ignored = oldEnd+1;
				if (unmarked[ignored]) while (++ignored < positionStart && unmarked[ignored]); // dont have to account for these
				if (ignored != positionStart) // position track failed
				{
					if (firstMatched == positionStart) firstMatched = 0; // drop recog of it
					matched = false;
					positionStart = oldStart;
					positionEnd = oldEnd;
				}
			}
		}
		
		if (trace & TRACE_PATTERN  && CheckTopicTrace()) 
		{
			bool success = matched;
			if (statusBits & NOT_BIT) success = !success;
			if (*word == '[' || *word == '{' || *word == '(' || (*word == '<' && word[1] == '<')) {} // seen on RETURN from a matching pair
			else if (*word == ']' || *word == '}' || *word == ')' || (*word == '>' && word[1] == '>')) {} 
			else
			{
				Log(STDTRACELOG,(char*)"%s",word);
				if (*word == '~' && matched) 
				{
					if (positionStart <= 0 || positionStart > wordCount || positionEnd <= 0 || positionEnd > wordCount) {;} // still in init startup?
					else if (positionStart != positionEnd) Log(STDTRACELOG,(char*)"(%s...%s)",wordStarts[positionStart],wordStarts[positionEnd]);
					else Log(STDTRACELOG,(char*)"(%s)",wordStarts[positionStart]);
				}
				else if (*word == USERVAR_PREFIX && matched) 
				{
					Log(STDTRACELOG,(char*)"(%s)",GetUserVariable(word));
				}
				else if (*word == '*' && matched && positionStart > 0 && positionStart <= wordCount && positionEnd <= wordCount) 
				{
					*word = 0;
					for (int i = positionStart; i <= positionEnd; ++i) 
					{
						if (*word) strcat(word,(char*)" ");
						strcat(word,wordStarts[i]);
					}
					Log(STDTRACELOG,(char*)"(%s)",word);
				}

				Log(STDTRACELOG,(success) ? (char*)"+ " : (char*)"- ");
			}
		}
	
        //   now verify position of match, NEXT is default for (type, not matter for others
        if (*kind == '(' || *kind == '<') //   ALL must match in sequence ( or jumbled <
        {
			//   we failed, retry shifting the start if we can
			if (!matched)
			{
				if (*kind == '<') break;	// we failed << >>

				if (rebindable == 1 && firstMatched > 0 && firstMatched < NORETRY) //   we are top level and have a first matcher, we can try to shift it
				{
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) 
					{
						Log(STDTRACETABLOG,(char*)"------ Try pattern matching again, after word %d (%s) ------ ",firstMatched,wordStarts[firstMatched]);
						Log(STDTRACETABLOG,(char*)"");
					}
					//   reset to initial conditions, mostly 
					reverse = false;
					ptr = orig;
					wildcardIndex = wildcardBase; 
					basicStart = positionEnd = firstMatched;  //   THIS is different from inital conditions
					firstMatched = -1; 
					beginmatch = -1; // RETRY PATTERN
					positionStart = INFINITE_MATCH; 
					wildcardSelector = 0;
					statusBits &= -1 ^ (NOT_BIT | FREEMODE_BIT | NOTNOT_BIT);
					argumentText = NULL; 
					// clear bit markers
					matchedBits[depth][0] = 0;
					matchedBits[depth][1] = 0;
					matchedBits[depth][2] = 0;
					matchedBits[depth][3] = 0;
					continue;
				}
				break; //   default fail
			}
			if (statusBits & FREEMODE_BIT) 
			{
				positionEnd = startposition;  //   allowed to pick up after here
				positionStart = INFINITE_MATCH; //   re-allow anywhere
			}
		}
        else if (matched) // was could not be END of sentence marker, why not???  
        {
			if (argumentText) //   we are ok, but we need to resume old data
			{
				ptr = argumentText;
				argumentText = NULL;
			}
			else if (*kind == '{' || *kind == '[')
			{
				success = true;
				break;	// we matched all needed in this
			}
		}
    } 

	//   begin the return sequence
	
	if (functionNest > startNest)//   since we are failing, we need to close out all function calls in progress at this level
    {
        callArgumentIndex = argStack[startNest];
        callArgumentBase = baseStack[startNest];
        fnVarbase = callArgumentIndex - 1;
		functionNest = startNest;
    }
	
	if (success)
	{
        returnend = positionEnd;
        if (depth > 0 && *word == ')' && !reverse) returnstart = positionStart;		// where we began this level
        else if (depth > 0 && reverse && positionStart > positionEnd) // our data is in reverse, flip it around
        {
            returnend = positionStart;
            returnstart = positionEnd;
        }
        else if (depth > 0 && *word == ')') returnstart = positionStart;		// where we began this level
        else returnstart = (firstMatched > 0) ? firstMatched : positionStart; // if never matched a real token, report 0 as start
	}

	//   if we leave this level w/o seeing the close, show it by elipsis 
	//   can only happen on [ and { via success and on ) by failure
	globalDepth = startdepth; // insures even if we skip >> closes, we get correct depth
	if (trace & TRACE_PATTERN && depth  && CheckTopicTrace())
	{
		if (*word != ')' && *word != '}' && *word !=  ']' && (*word != '>' || word[1] != '>'))
		{
			if (*ptr != '}' && *ptr != ']' && *ptr != ')' && (*ptr != '>' || ptr[1] != '>')) Log(STDTRACELOG,(char*)"...");	// there is more in the pattern still
			if (success) 
			{
				if (*kind == '<') Log(STDTRACELOG,(char*)">>");
				else Log(STDTRACELOG,(char*)"%c",(*kind == '{') ? '}' : ']');
			}
			else if (*kind == '<') Log(STDTRACELOG,(char*)">>");
			else Log(STDTRACELOG,(char*)")");
		}
		else Log(STDTRACELOG,(char*)"%s",word); // we read to end of pattern 
		if (*word == '}') Log(STDTRACELOG,(char*)"+"); // optional always matches, by definition
		else Log(STDTRACELOG,(char*)"%c",matched ? '+' : '-');
		Log(STDTRACETABLOG,(char*)""); // next level resumed
	}
	if (trace & TRACE_PATTERN && !depth)
	{
		if (!matched)
		{
			char* copy = AllocateStack(NULL,MAX_WORD_SIZE);
			strncpy(copy,ptr,80);
			strcpy(copy+75,(char*)"...");
			char* at = strchr(copy,')');
			if (at) at[1] = 0;
			CleanOutput(copy);
			Log(STDTRACELOG,(char*)"        Remaining pattern: %s\r\n",copy);
			ReleaseStack(copy);
		}
		else Log(STDTRACELOG,(char*)")+\r\n");
	}
	return success;
}

