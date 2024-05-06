#include "common.h"

/*
:testpattern ( a  _( case 0 ) ) a big dog
:testpattern ( [ _big _small ] _* _( dog ) ) small cat dog
:testpattern ( _* _(dog) ) cat dog
:testpattern ( _* _one _* ( _two )  _* ( _three ) ) first one two  and three
:testpattern ( _* ( _one _* ( _two ) ) _* ( _three ) ) first one two three
:testpattern ( _* _( _one _two _three ) _* _( _four _* _six ) _* ) one two three middle four five six last
:testpattern ( _* ( _one _* ( _two _* ( _omega ) ) _three _* _four ) ) one two omega three four
:testpattern ( _{ optional } _* _( dog ) ) optional cat dog
:testpattern ( { _optional } _* _( dog ) ) optional cat dog
:testpattern ( _{ _optional } _* _( dog ) ) optional cat dog
:testpattern ( [ _big small ] _* _( dog ) ) big cat dog
:testpattern ( _one _two _( three _four ) five six @_3- _three ) one two three four five six
:testpattern ( _{ _( one _two three ) } _[ _(four five six) ] ) one two three four five six
:testpattern ( _{ _( one _two three ) } _[ _(four five _six) ] ) one two three four five six
:testpattern ( _three @_0- _* _two @_0+ _* _four ) one two three four five six
:testpattern ( _three @_0- _* _one @_0+ _* _five ) one two three four five six

sample retry: 
 concept: ~iwe ('I we us)
 # I do not wear nor do I need glasses
 u:  (  [do will might]  ~iwe [need have] ) 

*/

#define INFINITE_MATCH (-(200 << 8)) // allowed to match anywhere

#define NOT_BIT					0X00010000
#define FREEMODE_BIT			0X00020000
#define QUOTE_BIT				0X00080000
#define NOTNOT_BIT				0X00400000

#define GAPSTART                0X000000FF
#define GAPLIMIT                0X0000FF00
#define GAPLIMITSHIFT 8
#define GAP_SHIFT 16
#define GAP_SLOT                0x001F0000 // std wildcard index
#define SPECIFIC_SLOT           0x1F000000 // words or bracketed items
#define SPECIFIC_SHIFT 24

#define WILDGAP					0X20000000  // start of gap is 0x000000ff, limit of gap is 0x0000ff00  
#define WILDMEMORIZEGAP			0X40000000  // start of gap is 0x000000ff, limit of gap is 0x0000ff00  
#define WILDMEMORIZESPECIFIC	0X80000000  //   while 0x1f0000 is wildcard index to use
// bottom 16 bits hold start and range of a gap (if there is one) and bits WILDGAP or MEMORIZEWILDGAP will be on
// and slot to use is 5 bits below
// BUT it we are memorizing specific, it must use separate slot index because _*~4 _() can exist
HEAPREF heapPatternThread = NULL;
char xword[MAX_WORD_SIZE]; // used by Match, saves on stack space
int patternDepth = 0;
int indentBasis = 1;
bool nopatterndata = false; // speedup by not saving matching info
bool patternRetry = false;
static char kindprior[100];
static int bilimit = 0;
bool deeptrace = false;
static char* returnPtr = NULL;
char* patternchoice = NULL;
// pattern macro  calling data
static unsigned int functionNest = 0;	// recursive depth of macro calling
#define MAX_PAREN_NEST 50
static char* ptrStack[MAX_PAREN_NEST];
#define MATCHEDDEPTH 40
static uint64 matchedBits[MATCHEDDEPTH][4];	 // nesting level zone of bit matches
static uint64 retryBits[4];     // last set of match bits before retry
unsigned int patternEvaluationCount = 0;    // number of patterns evaluated in this volley
int bicounter = 0;
HEAPREF matchedWordsList = NULL;
unsigned char keywordStarter[256] = // non special characters that will use default MATCHIT
{
    0,0,0,0,0,0,0,0,0,0,			0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,			0,0,0,0,0,0,0,0,0,'\'',
    0,0,0,0,0,0,0,0,  '0','1',	'2','3','4','5','6','7','8','9',0,0,
    0,0,0,0,0,'A','B','C','D','E', 	'F','G','H','I','J','K','L','M','N','O',
    'P','Q','R','S','T','U','V','W','X','Y', 	'Z',0,0,0,0,0,0,'A','B','C',
    'D','E','F','G','H','I','J','K','L','M',  'N','O','P','Q','R','S','T','U','V','W',
    'X','Y','Z',0,0,0,0,  1,1,1,     1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,             1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,       1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,
};

void ShowMatchResult(FunctionResult result, char* rule, char* label,int id)
{
    if (trace & TRACE_LABEL && label && *label && !(trace & TRACE_PATTERN) && CheckTopicTrace())
    {
        if (result == NOPROBLEM_BIT) Log(FORCETABUSERLOG, "**  Match: %s %d.%d %c: \r\n", label, TOPLEVELID(id), REJOINDERID(id),  *rule); //  \\  blocks linefeed on next Log call
        else Log(FORCETABUSERLOG, "**  Fail %s: %d.%d %c: \r\n", label, TOPLEVELID(id), REJOINDERID(id),  *rule); //  \\  blocks linefeed on next Log call
    }
    if (result == NOPROBLEM_BIT && trace & (TRACE_PATTERN | TRACE_MATCH | TRACE_SAMPLE) && CheckTopicTrace()) //   display the entire matching responder and maybe wildcard bindings
    {

        if (label && *label) Log(USERLOG, "**  Match: %s %s %d.%d %c: ", label, GetTopicName(currentTopicID), TOPLEVELID(id), REJOINDERID(id), *rule); //  \\  blocks linefeed on next Log call
        else  Log(USERLOG, "**  Match: %s %d.%d %c:", GetTopicName(currentTopicID), TOPLEVELID(id), REJOINDERID(id), * rule); //  \\  blocks linefeed on next Log call
        if (wildcardIndex)
        {
            Log(USERLOG,"  matchvar: ");
            for (int i = 0; i < wildcardIndex; ++i)
            {
                if (i > MAX_WILDCARDS) break;
                if (*wildcardOriginalText[i])
                {
                    if (!strcmp(wildcardOriginalText[i], wildcardCanonicalText[i])) Log(USERLOG, "_%d=%s (%d-%d)  ", i, wildcardOriginalText[i],wildcardPosition[i] & 0x0000ffff, wildcardPosition[i] >> 16);
                    else Log(USERLOG, "_%d=%s / %s (%d-%d)  ", i, wildcardOriginalText[i], wildcardCanonicalText[i], wildcardPosition[i] & 0x0000ffff, wildcardPosition[i] >> 16);
                }

                else Log(USERLOG, "_%d=null (%d-%d)  ", i, wildcardPosition[i] & 0x0000ffff, wildcardPosition[i] >> 16);
            }
        }
        Log(USERLOG,"\r\n");
    }
}

static void MarkMatchLocation(unsigned int start, unsigned int end, int depth)
{
    for (unsigned int i = start; i <= end; ++i)
    {
        int offset = i / 64; // which unit
        int index = i % 64;  // which bit
        uint64 mask = ((uint64)1) << index;
        if (depth < MATCHEDDEPTH) matchedBits[depth][offset] |= mask;
    }
}

static char* BitIndex(uint64 bits, char* buffer, int offset)
{
    uint64 mask = 0X0000000000000001ULL;
    for (int index = 0; index <= 63; ++index)
    {
        if (mask & bits)
        {
            sprintf(buffer, "%d ", offset + index);
            buffer += strlen(buffer);
        }
        mask <<= 1;
    }
    return buffer;
}

void GetPatternData(char* buffer)
{
    char* original = buffer;
    buffer = BitIndex(matchedBits[0][0], buffer, 0);
    buffer = BitIndex(matchedBits[0][1], buffer, 64);
    buffer = BitIndex(matchedBits[0][2], buffer, 128);
    BitIndex(matchedBits[0][3], buffer, 192);
    if (strlen(original) == 0)
    {
        // no final matching data, so return last retried set
        buffer = BitIndex(retryBits[0], buffer, 0);
        buffer = BitIndex(retryBits[1], buffer, 64);
        buffer = BitIndex(retryBits[2], buffer, 128);
        BitIndex(retryBits[3], buffer, 192);
    }
}

void GetPatternMatchedWords(char* buffer)
{
    char* original = buffer;
    HEAPREF list = matchedWordsList;
    while (list)
    {
        uint64 val1, val2, val3;
        list = UnpackHeapval(list, val1, val2, val3);

        WORDP D = (WORDP)val1;
        if (buffer != original) strcat(buffer++, " ");
        strcpy(buffer, D->word);
        buffer += strlen(buffer);
        strcpy(buffer, " 1");
        buffer += 2;
    }
}

static void DecodeFNRef(char* side)
{
    char* at = "";
    if (side[1] == USERVAR_PREFIX) at = GetUserVariable(side + 1, false);
    else if (IsDigit(side[1])) at = FNVAR(side );
    at = SkipWhitespace(at);
    strcpy(side, at);
}

static void DecodeAssignment(char* word, char* lhs, char* op, char* rhs)
{
    // get the operator
    char* assign = word + Decode(word + 1, 1); // use accelerator to point to op in the middle
    strncpy(lhs, word + 2, assign - word - 2);
    lhs[assign - word - 2] = 0;
    assign++; // :
    op[0] = *assign++;
	op[1] = 0;
	op[2] = 0;
	if (*assign == '=') op[1] = *assign++;
    strcpy(rhs, assign);
}

static void DecodeComparison(char* word, char* lhs, char* op, char* rhs)
{
    // get the operator
    char* compare = word + Decode(word + 1, 1); // use accelerator to point to op in the middle
    strncpy(lhs, word + 2, compare - word - 2);
    lhs[compare - word - 2] = 0;
    *op = *compare++;
    op[1] = 0;
    if (*compare == '=') // was == or >= or <= or &= 
    {
        op[1] = '=';
        op[2] = 0;
        ++compare;
    }
    strcpy(rhs, compare);
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
        if (*pattern == GetLowercaseData(*word) && MatchesPattern(word + 1, pattern + 1)) return true;
    }
    return false; // failed to resynch
}

static bool SysVarExists(char* ptr) //   %system variable
{
    char* sysvar = SystemVariable(ptr, NULL);
    if (!*sysvar) return false;
    return (*sysvar) ? true : false;	// value != null
}

static bool FindPartialInSentenceTest(char* test, int start, int originalstart, bool reverse,
    int& actualStart, int& actualEnd) // this does actuals, not canonicals
{
    if (!test || !*test) return false;
    char* word = AllocateStack(NULL, MAX_WORD_SIZE);
    if (reverse)
    {
        for (int i = originalstart - 1; i >= 1; --i) // can this be found in sentence backwards
        {
            MakeLowerCopy(word, wordStarts[i]);
            if (unmarked[i] || !MatchesPattern(word, test)) continue;	// if universally unmarked, skip it. Or if they dont match
                                                                        // we have a match of a word
			if (!HasMarks(i)) continue; // used ^unmark(@ _x) , making this invisible to mark system
			actualStart = i;
            actualEnd = i;
            ReleaseStack(word);
            return true;
        }
    }
    else
    {
        for (int i = start + 1; i <= wordCount; ++i) // can this be found in sentence
        {
            if (!wordStarts[i]) continue;
            MakeLowerCopy(word, wordStarts[i]);
            if (unmarked[i] || !MatchesPattern(word, test)) continue;	// if universally unmarked, skip it. Or if they dont match
					                                                   // we have a match of a word
			if (!HasMarks(i)) continue; // used ^unmark(@ _x) , making this invisible to mark system
            actualStart = i;
            actualEnd = i;
            ReleaseStack(word);
            return true;
        }
    }
    ReleaseStack(word);
    return false;
}

static bool MatchTest(bool reverse, WORDP D, int start, char* op, char* compare, int quote,
    bool exact, int legalgap, MARKDATA* hitdata) // is token found somewhere after start?
{
    if (start == INFINITE_MATCH) start = (reverse) ? (wordCount + 1) : 0;
    if (deeptrace && D) Log(USERLOG," matchtesting:%s ", D->word);
    while (GetNextSpot(D, start, reverse, legalgap, hitdata)) // find a spot later where token is in sentence
    {
        if (hitdata->start == 0xff)  hitdata->start = 0;
        start = hitdata->start; // where to try next if fail on test
      
        if (deeptrace) Log(USERLOG," matchtest:%s %d-%d ", D->word, hitdata->start, hitdata->end);
        if (exact && (unsigned int)(start + 1) != hitdata->start) return false;	// we are doing _0?~hello or whatever. Must be on the mark
        if (deeptrace) Log(USERLOG," matchtest:ok ");
        if (op) // we have a test to perform
        {
            char* word;
            if (D->word && (IsAlphaUTF8(*D->word) || D->internalBits & UTF8)) word = D->word; //   implicitly all normal words are relation tested as given
            else word = quote ? wordStarts[hitdata->start] : wordCanonical[hitdata->start];
            int id;
            if (deeptrace) Log(USERLOG," matchtest:optest ");
            char word1val[MAX_WORD_SIZE];
            char word2val [MAX_WORD_SIZE];
            FunctionResult res = HandleRelation(word, op, compare, false, id, word1val, word2val);
            if (res & ENDCODES) continue; // failed 
        }
        if (*D->word == '~')  return true; // we CANNOT tell whether original or canon led to set...
        if (!quote) return true; // can match canonical or original

                                 //   we have a match, but prove it is a original match, not a canonical one
        unsigned int end = hitdata->end & REMOVE_SUBJECT; // remove hidden subject field if there from fundamental meaning match
        if ((int)end < hitdata->start) continue;	// trying to match within a composite. 
        if (hitdata->start == (int)end && !stricmp(D->word, wordStarts[hitdata->start])) return true;   // literal word match
        else // match a phrase literally
        {
            char word[MAX_WORD_SIZE];
            char* at = word;
            for (int i = hitdata->start; i <= (int)end; ++i)
            {
                strcpy(at, wordStarts[i]);
                at += strlen(wordStarts[i]);
                if (i != (int)end) *at++ = '_';
            }
            *at = 0;
            if (!stricmp(D->word, word)) return true;
        }
    }
    if (deeptrace && D) Log(USERLOG," matchtest:%s failed ", D->word);
    return false;
}

static bool FindPhrase(char* word, int start, bool reverse, int & actualStart, int& actualEnd,MARKDATA* hitdata)
{   // Phrases are dynamic, might not be marked, so have to check each word separately. -- faulty in not respecting ignored(unmarked) words
    if (start > wordCount) return false;
    bool matched = false;
    actualEnd = start;
    unsigned int oldend;
    oldend = start = 0; // allowed to match anywhere or only next
    unsigned int n = BurstWord(word);
    for (int i = 0; i < n; ++i) // use the set of burst words - but "Andy Warhol" might be a SINGLE word.
    {
        WORDP D = FindWord(GetBurstWord(i));
        matched = MatchTest(reverse, D, actualEnd, NULL, NULL, 0,false, 0,hitdata);
        if (matched)
        {
            actualStart = hitdata->start;
            actualEnd = hitdata->end;

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
            oldend = actualEnd & REMOVE_SUBJECT; // remove hidden subject field
        }
        else break;
    }
    if (matched) actualStart = start;
    return matched;
}

static char* PushMatch(int used)
{
    char* limit;
    char* base = InfiniteStack(limit, "PushMatch");
    int* vals = (int*)base;
    for (int i = 0; i < used; ++i) *vals++ = wildcardPosition[i];
    char* rest = (char*)vals;
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

static void PopMatch(char* base, int used)
{
    int* vals = (int*)base;
    for (int i = 0; i < used; ++i) wildcardPosition[i] = *vals++;
    char* rest = (char*)vals;
    for (int i = 0; i < used; ++i)
    {
        strcpy(wildcardOriginalText[i], rest);
        rest += strlen(rest) + 1;
        strcpy(wildcardCanonicalText[i], rest);
        rest += strlen(rest) + 1;
    }
    ReleaseStack(base);
}

static char* PatternMacroCall(WORDP D, char* ptr,bool& matched)
{
    if ((trace & TRACE_PATTERN || D->internalBits & MACRO_TRACE) && CheckTopicTrace()) Log(USERLOG, "%s(", D->word);
    ptr += 2; // skip ( and space
    FunctionResult result = NOPROBLEM_BIT;
    int callArgumentIndex = 0;
    char* defn = (char*)FindAppropriateDefinition(D, result, true);
    int argcount = 0;
    if (defn)
    {
        char* argp = strchr(defn, ' ') + 1;
        unsigned char c = (unsigned char)*argp;
        argcount = (c >= 'a') ? (c - 'a' + 15) : (c - 'A'); // expected args, leaving us at ( args ...
    }
    
    CALLFRAME* frame = ChangeDepth(1, D->word);
    frame->n_arguments = argcount;
    currentFNVarFrame = frame; // used for function variable accesses

    // read arguments
    int currentcount = 0;
    char* buffer = AllocateBuffer("match");
    while (*ptr && *ptr != ')')
    {
        ptr = ReadArgument(ptr, buffer, result);  // gets the unevealed arg
        if (++currentcount > argcount)
        {
            matched = false;
            break;
        }
        if (*buffer == '(' && buffer[1] == '  ') // MAYNOT do  ( ... ) - thats not 1 arg but multiple
        {
            matched = false;
            break;
        }
        frame->arguments[++callArgumentIndex] = AllocateStack(buffer);
        if ((trace & TRACE_PATTERN || D->internalBits & MACRO_TRACE) && CheckTopicTrace()) Log(USERLOG, " %s, ", buffer);
        *buffer = 0;
        if (result != NOPROBLEM_BIT)
        {
            matched = false;
            break;
        }
    }
    FreeBuffer("Match");

    if (currentcount != argcount) // error!
    {
        matched = false;
        ChangeDepth(-1, D->word);
        return ptr;
    }

    if ((trace & TRACE_PATTERN || D->internalBits & MACRO_TRACE) && CheckTopicTrace()) Log(USERLOG, ")\r\n");
    ptrStack[functionNest++] = ptr + 2; // skip closing paren and space
    ptr = defn;
    if (ptr)
    {
        char* word = AllocateStack(NULL, MAX_WORD_SIZE);
        ptr = ReadCompiledWord(ptr, word) + 2; // eat flags and count and (
        while (*ptr) // skip over locals list (which should be null)
        {
            ptr = ReadCompiledWord(ptr, word);
            if (*word == ')') break;
        }
        ReleaseStack(word);
    }
    else ptr = ""; // null function
    if (result == NOPROBLEM_BIT) matched = true; // direct resumption
    else matched = false;
    return ptr;
}

static char* PatternOutputmacroCall(WORDP D, char* ptr, FunctionResult &result,MARKDATA* hitdata)
{
    char* base = PushMatch(wildcardIndex);
    int baseindex = wildcardIndex;
    if (trace & TRACE_PATTERN) Log(USERLOG, "\r\n");
    ptr = DoFunction(D->word, ptr, currentOutputBase, result);
    PopMatch(base, baseindex); // return to index before we called function
    return ptr;
}

#ifdef INFORMATION

We keep a positional range reference within the sentence where we are(positionStartand positionEnd).
Before we attempt the next match we make a backup copy(oldStartand oldEnd)
so that if the match fails, we can revert back to where we were(under some circumstances).

We keep a variable firstMatched to track the first real word we have matched so far.
If the whole match is declared a failure eventually, we may be allowed to go backand
retry matching starting immediately after that location.That is, we do not do all possible backtracking
as Prolog might, but we do a cheaper form where we simply try again farther in the sentence.
Also, firstMatched is returned from a subcall, so the caller can know where to end a wildcard memorization
started before the subcall.

Some tokens create a wildcard effect, where the next thing is allowed to be some distance away.
This is tracked by wildcardSelector, and the token after the wildcard, when found, is checked to see
if its position is allowed.When we enter a choice construct like[] and {}, when a choice fails,
we reset the wildcardSelector back to originalWildcardSelector so the next choice sees the same environment.

In reverse mode, the range of positionStartand positionEnd continue to be earlierand later in the sentence,
but validation treats positionStart as the basis of measuring distance.

Rebindable refers to ability to relocate firstmatched on failure(1 means we can shift from here, 3 means we enforce spacing and cannot rebind)
Some operations like < or @_0 + force a specific position, and if no firstMatch has yet happened, then you cannot change
    the start location.

    returnStartand returnEnd are the range of the match that happened when making a subcall.
    Startposition is where we start matching from.
#endif

bool Match(char* ptr, int depth,MARKDATA& hitdata, int rebindable, unsigned int wildcardSelector,
       int& firstMatched, char kind, bool reverse)
{//   always STARTS past initial opening thing ( [ {  and ends with closing matching thing
    int startposition = hitdata.start;
    int positionStart = (rebindable == 1)  ? INFINITE_MATCH : hitdata.end; // passed thru in recursive call
    // INFINITE_MATCH means we are in initial startup, allows us to match ANYWHERE forward to start
    int rebindlimit = 20; // prevent large rebind attempts
    int positionEnd = startposition; //   we scan starting 1 after this
    int startdepth = globalDepth;
    patternDepth = depth;
    int wildgap = 0;
    hitdata.word = 0;
    if (depth == 0)
    {
        ++patternEvaluationCount;
        bilimit = 0;
    }
    if (wildcardSelector &  WILDGAP && kind == '{')
        wildgap = ((wildcardSelector & GAPLIMIT) >> GAPLIMITSHIFT) + 1;
    if (!nopatterndata)
    {
        if (depth < MATCHEDDEPTH) memset(&matchedBits[depth], 0, sizeof(uint64) * 4);  // nesting level zone of bit matches
        if (!nopatterndata && depth == 0)
        {
            memset(&retryBits, 0, sizeof(uint64) * 4);
            matchedWordsList = NULL;
        }
    }
    char* word = AllocateBuffer(); // users may type big, but pattern words wont be, except data string assignment
    char* orig = ptr;
    int statusBits = (kind == '<') ? FREEMODE_BIT : 0; //   turns off: not, quote, startedgap, freemode ,wildselectorpassback
    kindprior[depth] = kind;
    bool matched = false;
    unsigned int startNest = functionNest;
    int wildcardBase = wildcardIndex;
    int bidirectional = 0;
    int bidirectionalSelector = 0;
    int bidirectionalWildcardIndex = 0;
    WORDP D;
    int beginmatch = -1; // for ( ) where did we actually start matching
    bool success = false;
    char* priorPiece = NULL;
    if (!depth) patternRetry = false;
    int at;
    int slidingStart = startposition;
    firstMatched = -1; //   ()  should return spot it started (firstMatched) so caller has ability to bind any wild card before it
    if (rebindable == 1)  slidingStart =  INFINITE_MATCH; //   INFINITE_MATCH means we are in initial startup, allows us to match ANYWHERE forward to start
    int originalWildcardSelector = wildcardSelector;
    int basicStart = startposition;	//   we must not match real stuff any earlier than here
    char* argumentText = NULL; //   pushed original text from a function arg -- function arg never decodes to name another function arg, we would have expanded it instead
	if (trace & TRACE_PATTERN && CheckTopicTrace())
	{
		if (detailpattern )
		{
			char s[10];
			if (positionStart == INFINITE_MATCH) strcpy(s, "?");
			else sprintf(s, "%d", positionStart);
			Log(USERLOG, "%s-%d-", s, positionEnd);
		}
		Log((depth == 0 && !csapicall && !debugcommand) ? USERLOG : USERLOG, "%c ", kind); //   start on new indented line
	}
	while (ALWAYS) //   we have a list of things, either () or { } or [ ].  We check each item until one fails in () or one succeeds in  [ ] or { }
    {
        int oldStart = positionStart; //  allows us to restore if we fail, and confirm legality of position advance.
        int oldEnd = positionEnd;
        int id;
        char* nextTokenStart = SkipWhitespace(ptr);
        returnPtr = nextTokenStart; // where we last were before token... we cant fail on _ and that's what we care about
        char* starttoken = nextTokenStart;
        ptr = ReadCompiledWord(nextTokenStart, word);
        if (*word == '<' && word[1] == '<')  ++nextTokenStart; // skip the 1st < of <<  form
        if (*word == '>' && word[1] == '>')  ++nextTokenStart; // skip the 1st > of >>  form
        nextTokenStart = SkipWhitespace(nextTokenStart + 1);    // ignore blanks after if token is a simple single thing like !
        char c = *word;
        WORDP foundaword = NULL;
        if (c != ')' && c != '}' && c != ']' && (c != '>' || word[1] != '>'))
        {// if returning to above, leave these values alone
            hitdata.word = 0; 
            hitdata.disjoint = 0;
        }
        hitdata.start = 0;
		switch (c) // c is prefix on tokens, declares type of token
		{
        DOUBLELEFT:  
            case '(': case '[':  case '{': // nesting condition    (= consecutive  [ = choice   { = optional   << = all of unordered
            if (trace & TRACE_PATTERN && *word != '{' && indentBasis == -1) Log(USERLOG, "\r\n");

            ptr = nextTokenStart;
            {
                int returnStart = positionStart; // default return for range start
                int returnEnd = positionEnd;  // default return for range end
                int rStart = positionStart; // refresh values of start and end for failure to match
                int rEnd = positionEnd;
                unsigned int oldselect = wildcardSelector;
                int uppercaserturn= 0;
                // nest inherits gaps leading to it. 
                // memorization requests withheld til he returns.
                // may have 2 memorizations, gap before and the () item
                // Only know close of before gap upon return
                int whenmatched = 0;
                char type = '[';
                if (*word == '(') type = '(';
                else if (*word == '{') type = '{';
                else if (*word == '<')
                {
                    type = '<';
                    positionEnd = startposition;  //   allowed to pick up after here - oldStart/oldEnd synch automatically works
                    positionStart = INFINITE_MATCH;
                    rEnd = 0;
                    rStart = INFINITE_MATCH;
                }
                int localRebindable = 0; // not allowed to try rebinding start again by default
                if (positionStart == INFINITE_MATCH) localRebindable = 1; // we can move the start
                if (oldselect & (WILDMEMORIZEGAP | WILDGAP)) localRebindable = 2; // allowed to gap in 
                int select = wildcardSelector;
                if (select & WILDMEMORIZEGAP) // dont memorize within, do it out here but pass gap in
                {
                    select ^= WILDMEMORIZEGAP;
                    select |= WILDGAP;
                }
                else if (select & WILDMEMORIZESPECIFIC) select ^= WILDMEMORIZESPECIFIC;
                int bracketstart = reverse ? positionStart - 1 : positionEnd + 1;
                hitdata.start = positionEnd;
                hitdata.end = positionStart; // passing this thru to nested call
                matched = Match(ptr, depth + 1, hitdata, localRebindable, select, 
                    whenmatched, type,  reverse); //   subsection ok - it is allowed to set position vars, if ! get used, they dont matter because we fail
                returnStart = hitdata.start;
                returnEnd = hitdata.end;
                foundaword = Meaning2Word(hitdata.word);
                wildcardSelector = oldselect; // restore outer environment
                if (matched) // position and whenmatched may not have changed
                {
                    // monitor which pattern in multiple matched
                    // return positions are always returned forward looking
                    if (reverse && returnStart > returnEnd)
                    {
                        int x = returnStart;
                        returnStart = returnEnd;
                        returnEnd = x;
                    }

                    if (wildcardSelector & WILDMEMORIZEGAP) // the wildcard for a gap BEFORE us
                    {
                        int index = (wildcardSelector >> GAP_SHIFT) & 0x0000001f;
                        wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP); // remove the before marker
                        int brackend = whenmatched; // gap starts here
                        if (whenmatched > 0)
                        {
                            if (reverse) returnEnd = whenmatched;
                            else returnStart = whenmatched;
                        }
                        if ((brackend - bracketstart) == 0) SetWildCardGivenValue((char*)"", (char*)"", 0, oldEnd + 1, index,&hitdata); // empty gap
                        else if (reverse) SetWildCardGiven(brackend + 1, bracketstart, true, index);  //   wildcard legal swallow between elements
                        else SetWildCardGiven(bracketstart, brackend - 1, true, index);  //   wildcard legal swallow between elements
                    }
                    else if (wildcardSelector & WILDGAP) // the wildcard for a gap BEFORE us
                    {
                        if (whenmatched > 0)
                        {
                            if (reverse) returnEnd = whenmatched;
                            else returnStart = whenmatched;
                        }
                    }
                    if (returnStart > 0) positionStart = returnStart;
                    if (positionStart == INFINITE_MATCH && returnStart > 0 && returnStart != INFINITE_MATCH) positionStart = returnEnd;
                    if (returnEnd > 0) positionEnd = returnEnd;

                    // copy back marking bits on match
                    if (!(statusBits & NOT_BIT)) // wanted match to happen
                    {
                        int olddepth = depth + 1;
                        if (!nopatterndata && olddepth < MATCHEDDEPTH)
                        {
                            matchedBits[depth][0] |= matchedBits[olddepth][0];
                            matchedBits[depth][1] |= matchedBits[olddepth][1];
                            matchedBits[depth][2] |= matchedBits[olddepth][2];
                            matchedBits[depth][3] |= matchedBits[olddepth][3];
                        }
                        if (beginmatch == -1) beginmatch = positionStart; // first match in this level
                        if (firstMatched < 0) firstMatched = whenmatched;
                    }
                    // allow single matches to propogate back thru
                    if (*word == '<') // allows thereafter to be anywhere
                    {
                        positionStart = INFINITE_MATCH;
                        oldEnd = oldStart = positionEnd = 0;
                    }
                    // The whole thing matched but if @_ was used, which way it ran and what to consider the resulting zone is completely confused.
                    // So force a tolerable meaning so it acts like it is contiguous to caller.  If we are memorizing it may be silly but its what we can do.
                    if (*word == '(' && positionStart == NORETRY)
                    {
                        positionEnd = positionStart = (reverse) ? (oldStart - 1) : (oldEnd + 1);  // claim we only moved 1 unit
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
            ptr = BalanceParen(ptr, true, false); // reserve wildcards as we skip over the material including closer 
            break;

        DOUBLERIGHT: 
        case ')': case ']': case '}':  //   end sequence/choice/optional
            ptr = nextTokenStart;
            matched = (kind == '(' || kind == '<'); //   [] and {} must be failures if we are here while ( ) and << >> are success
            if (wildcardSelector & WILDGAP) //   pending gap  -  [ foo fum * ] and { foo fot * } are  illegal but [*3 *2] is not 
            {
                if (*word == ']' || *word == '}') { ; }
                else if (depth != 0) //  don't end () with a gap, illegal
                {
                    wildcardSelector = 0;
                    matched = false; //   force match failure
                }
                else if (reverse) // depth 0 main sentence
                {
                    positionEnd = 1; // nominally matched to here
                    positionStart -= 1; // began here
                }
                else positionStart = wordCount + 1; //   at top level a close implies > )
            }
            if (matched && depth > 0 && *word == ')') // matched all at this level? wont be true if it uses < inside it
            {
                if (slidingStart && slidingStart != INFINITE_MATCH)
                {
                    if (positionEnd == startposition) {} // we found nothing in the sentence, just matching other stuff
                    else positionStart = (reverse) ? (startposition - 1) : (startposition + 1);
                }
                else
                {
                    // if ( ) started wild like (* xxx) we need to start from beginning
                    // if ()  started matchable like (tom is) we need to start from starting match

                    if (beginmatch != -1) positionStart = beginmatch; // handle ((* test)) and ((test))
                    else if (positionStart == INFINITE_MATCH) {} // no change
                    else positionStart = (reverse) ? (startposition - 1) : (startposition + 1); // probably wrong on this one
                }
            }
        break;
        case '!': //   NOT condition - not a stand-alone token, attached to another token
			ptr = nextTokenStart;
			if (*ptr == '-') // prove not found anywhere before here
			{
				++ptr;
				ptr = ReadCompiledWord(ptr, xword);
				WORDP D = FindWord(xword, 0, PRIMARY_CASE_ALLOWED); // word only, not ( ) stuff
				if (!GetNextSpot(D, positionStart,  true )) // found. too bad
				{
					if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(USERLOG,"%s+", word);
					continue;
				}
				matched = false;
				break;
			}
			if (*ptr == '+') ++ptr; // normal forward (alternate notation to mere !word)

			if (*ptr == '!') // !!
			{
				if (ptr[1] == '+') ++ptr; // ignore notation
				else if (ptr[1] == '-')
				{
					++ptr;
					ptr = ReadCompiledWord(ptr+1, xword);
					WORDP D = FindWord(xword, 0, PRIMARY_CASE_ALLOWED); // word only, not ( ) stuff
					if (!GetNextSpot(D, positionStart, true,0,&hitdata) || hitdata.start != (unsigned int)(positionStart - 1))
					{
						if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(USERLOG,"%s+",word);
						continue;
					}
					matched = false;
					break;
				}
                ptr = SkipWhitespace(ptr + 1);
                statusBits |= NOTNOT_BIT;
            }
			if (trace & TRACE_PATTERN && CheckTopicTrace())
			{
				Log(USERLOG,"!");
			}
			statusBits |= NOT_BIT;
			continue;
        case '\'': //   single quoted item    -- direct word
            if (!stricmp(word, (char*)"'s")) // possessive singular 
            {
                WORDP D = FindWord(word);
                matched = MatchTest(reverse, D, (positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, NULL, NULL,
                    statusBits & QUOTE_BIT,  false, 0,&hitdata);
                if (!matched) hitdata.word = 0;
                else
                {
                    foundaword = D;
                    positionStart = hitdata.start;
                    positionEnd = hitdata.end;
                }
                if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart;
                break;
            }
            else
            {
                statusBits |= QUOTE_BIT;
                ptr = nextTokenStart;
                if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(USERLOG,"'");
                continue;
            }
        case '_':

            // a simple wildcard id?  names a memorized value like _8
            if (IsDigit(word[1]) && ( !word[2] || (IsDigit(word[2]) && !word[3] )))
            {
                matched = GetwildcardText(GetWildcardID(word), false)[0] != 0; // simple _2  means is it defined
                break;
            }

            // a wildcard id?  names a memorized value like _8
            ptr = nextTokenStart;
            if (IsDigit(*ptr)) // _0~ and _10~ or _5[ or _6{ or _7(
            {
                wildcardIndex = atoi(ptr); // start saves here now
                if (IsDigit(*++ptr)) ++ptr; // swallow 2nd digit
                if (trace & TRACE_PATTERN && CheckTopicTrace() && bidirectional != 2)  Log(USERLOG, "%s", word);
            }
            else if (trace & TRACE_PATTERN && CheckTopicTrace() && bidirectional != 2)
            {
               Log(USERLOG, "_");
            }

            // memorization coming - there can be up-to-two memorizations in progress: _* (wildmemorizegap) and _xxx (wildmemorizespecific)
            // it will be gap first and specific second (either token or smear of () [] {} )

            // if we are going to memorize something AND we previously matched inside a phrase, we need to move to after phrase...
            /*
            // this code is very suspicious...
            // - a forward match positionEnd will always be greater that positionStart, so first test is never true
            // - checking for a difference of 1 ignores the fact that the range could be more so doesn't seem wholly indicative of a match
            if ((positionStart - positionEnd) == 1 && !reverse) positionEnd = positionStart; // If currently matched a phrase, move to end.
            else if ((positionEnd - positionStart) == 1 && reverse) positionStart = positionEnd; // If currently matched a phrase, move to end.
            */
                                                                                                     //  aba or ~dat or **ar*
            if (ptr[0] != '*' || ptr[1] == '*') // wildcard word or () [] {}, not gap
            {
                wildcardSelector &= -1 ^ SPECIFIC_SLOT;
                wildcardSelector |= (WILDMEMORIZESPECIFIC + (wildcardIndex << SPECIFIC_SHIFT));
            }
            // *1 or *-2 or *elle (single wild token pattern match)
            else if (IsDigit(ptr[1]) || ptr[1] == '-' || IsAlphaUTF8(ptr[1]))
            {
                wildcardSelector &= -1 ^ SPECIFIC_SLOT;
                wildcardSelector |= (WILDMEMORIZESPECIFIC + (wildcardIndex << SPECIFIC_SHIFT));
            }
            else // *~ or *
            {
                wildcardSelector &= -1 ^ GAP_SLOT;
                wildcardSelector |= (WILDMEMORIZEGAP + (wildcardIndex << GAP_SHIFT)); // variable gap
            }
            SetWildCardNull(); // dummy match to reserve place
            continue;
        case '@': // factset ref or @_2+ or @retry  or @_2 anchor
            if (!stricmp(word, "@retry")) // when done, retry if match worked
            {
                patternRetry = true;
                matched = true; 
                break;
            }
            if (word[1] == '_') // set positional reference  @_20+ or @_0-   or anchor @_20
            {
                if (firstMatched < 0) firstMatched = NORETRY; // cannot retry this match locally
                char* end = word + 3;  // skip @_2
                if (IsDigit(*end)) ++end; // point to proper + or - ending
                unsigned int wild = wildcardPosition[GetWildcardID(word + 1)];
                int index = (wildcardSelector >> GAP_SHIFT) & 0x0000001f;

                // memorize gap to end based on direction...
                if (*end && (wildcardSelector & WILDMEMORIZEGAP) && !reverse) // close to end of sentence 
                {
                    positionStart = wordCount; // pretend to match at end of sentence
                    int start = wildcardSelector & GAPSTART;
                    int limit = (wildcardSelector & GAPLIMIT) >> GAPLIMITSHIFT ;
                    if ((positionStart + 1 - start) > limit) //   too long til end
                    {
                        matched = false;
                        wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
                        break;
                    }
                    if (wildcardSelector & WILDMEMORIZEGAP)
                    {
                        if ((wordCount - start) == 0) SetWildCardGivenValue((char*)"", (char*)"", start, start, index,&hitdata); // empty gap
                        else SetWildCardGiven(start, wordCount, true, index);  //   wildcard legal swallow between elements
                        wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
                    }
                }

                // memorize gap to start based on direction...
                if (*end && (wildcardSelector & WILDMEMORIZEGAP) && reverse) // close to start of sentence 
                {
                    positionEnd = 1; // pretend to match here (looking backwards)
                    int start = wildcardSelector & GAPSTART;
                    int limit = (wildcardSelector & GAPLIMIT) >> GAPLIMITSHIFT;
                    if ((start - positionEnd) > limit) //   too long til end
                    {
                        matched = false;
                        wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
                        break;
                    }
                    if (wildcardSelector & WILDMEMORIZEGAP)
                    {
                        if ((start - positionEnd + 1) == 0 || start == 0) SetWildCardGivenValue((char*)"", (char*)"",  start, start,index,&hitdata); // empty gap
                        else SetWildCardGiven(positionEnd, start, true, index);  //   wildcard legal swallow between elements
                        wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
                    }
                }

                if (*end == '-')
                {
                    reverse = true;
					positionEnd = positionStart = WILDCARD_START(wild);
                    // These are where the "old" match was
                }
                else // + and nothing both move forward. 
                {
                    int oldstart = positionStart;
                    int oldend = positionEnd; // will be 0 if we are starting out with no required match
                    positionStart = WILDCARD_START(wild);
                    positionEnd = WILDCARD_END(wild) & REMOVE_SUBJECT;
                    if (oldend && *end != '+' && positionStart != INFINITE_MATCH) // this is an anchor that might not match
                    {
                        if (wildcardSelector)
                        {
                            int start = wildcardSelector & GAPSTART;
                            int limit = (wildcardSelector & GAPLIMIT) >> GAPLIMITSHIFT;
                            if (!reverse && (positionStart < (oldend + 1) || positionStart > (start + limit)))
                            {
                                matched = false;
                                break;
                            }
                            else if (reverse && (positionEnd > (oldstart - 1) || positionEnd < (start - limit)))
                            {
                                matched = false;
                                break;
                            }
                        }
                        else
                        {
                            if (!reverse && positionStart != (oldend + 1))
                            {
                                matched = false;
                                break;
                            }
                            else if (reverse && positionEnd != (oldstart - 1))
                            {
                                matched = false;
                                break;
                            }
                        }
                    }
                    reverse = false;
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
                    char data[MAX_WORD_SIZE];

                    if (positionStart <= 0 || positionStart > wordCount || positionEnd <= 0 || positionEnd > wordCount) sprintf(data, "`index:%d`", positionEnd);
                    else if (positionStart == positionEnd) sprintf(data,"`%s index:%d`", wordStarts[positionEnd], positionEnd);
                    else sprintf(data,"`%s-%s index:%d-%d`", wordStarts[positionStart], wordStarts[positionEnd], positionStart, positionEnd);
                    strcat(word, data);
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
            if (word[1] == '<')
            {
                if (firstMatched <= 0 && firstMatched != NORETRY) rebindable = NORETRY; // since this matches anywhere, we dont want to retry if nothing found so far
                goto DOUBLELEFT; //   << 
            }
            at = 0;
            while (unmarked[++at] ) { ; } // skip over hidden data
            --at; // we perceive the start to be here
            ptr = nextTokenStart;
            if ((wildcardSelector & WILDGAP) && !reverse) // cannot memorize going forward to  start of sentence
            {
                matched = false;
                wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP);
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
            while (unmarked[++at] ) { ; } // skip over hidden data
            if (at > wordCount) at = positionEnd;	// he was the end
            else at = wordCount; // the presumed real end
            ptr = nextTokenStart;
            if (kind == '[') rebindable = 2; // let outer level decide if it is right  
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
                matched = true;
                positionStart = positionEnd = at + 1; //   pretend to match a word off end of sentence
            }
            else if (kind == '[' || kind == '{') // nested unit will figure out if legal 
            {
                matched = true;
                positionStart = positionEnd = at + 1; //   pretend to match a word at end of sentence
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
            if (beginmatch == -1) beginmatch = startposition + 1;
            if (word[1] == '-') //   backward grab, -1 is word before now -- BUG does not respect unmark system
            {
                int atx;
                if (!reverse) atx = positionEnd - (word[2] - '0') - 1; // limited to 9 back
                else  atx = positionEnd + (word[2] - '0') - 1;
                if (reverse && atx > wordCount)
                {
                    oldEnd = atx; //   set last match AFTER our word
                    positionStart = positionEnd = atx - 1; //   cover the word now
                    matched = true;
                }
                else if (!reverse && atx >= 0) //   no earlier than pre sentence start
                {
                    oldEnd = atx; //   set last match BEFORE our word
                    positionStart = positionEnd = atx + 1; //   cover the word now
                    matched = true;
                }
                else matched = false;
            }
            else if (IsDigit(word[1]))  // fixed length gap
            {
                unsigned int atx;
                unsigned int count = word[1] - '0';	// how many to swallow
                if (reverse)
                {
                    unsigned int begin = positionStart - 1;
                    atx = positionStart; // start here
                    while (count-- && --atx >= 1) // can we swallow this (not an ignored word)
                    {
                        if (unmarked[atx])
                        {
                            ++count;	// ignore this word
                            if (atx == begin) --begin;	// ignore this as starter
                        }
                    }
                    if (atx >= 1) // pretend match
                    {
                        positionEnd = begin; // pretend match here -  wildcard covers the gap
                        positionStart = atx;
                        matched = true;
                    }
                    else  matched = false;
                }
                else
                {
                    atx = positionEnd; // start here
                    int begin = positionEnd + 1;
                    while (count-- && ++atx <= wordCount) // can we swallow this (not an ignored word)
                    {
                        if (unmarked[atx])
                        {
                            ++count;	// ignore this word
                            if (atx == begin) ++begin;	// ignore this as starter
                        }
                    }
                    if (atx <= wordCount) // pretend match
                    {
                        positionStart = begin; // pretend match here -  wildcard covers the gap
                        positionEnd = atx;
                        matched = true;
                    }
                    else  matched = false;
                }
            }
            else if (IsAlphaUTF8(word[1]) || word[1] == '*')
            {
                matched = FindPartialInSentenceTest(word + 1, (positionEnd < basicStart&& firstMatched < 0) ? basicStart : positionEnd, positionStart, reverse,
                    positionStart, positionEnd); // wildword match like st*m* or *team* matches steamroller
                if (matched)
                {
                    foundaword = StoreWord(word, AS_IS); // this is not a real word, but a pattern but we dont care
                    hitdata.start = positionStart;
                    hitdata.end = positionEnd;
                    hitdata.word = Word2Index(foundaword);
                }
            }
            else // variable gap
            {
                int start = (reverse) ? (positionStart - 1) : (positionEnd + 1);
                wildcardSelector |= start | WILDGAP; // cannot conflict, two wilds in a row change no position
                if (word[1] == '~')
                {
                    wildcardSelector |= (word[2] - '0') << GAPLIMITSHIFT; // *~3 - limit is 9 back
                    if (word[strlen(word) - 1] == 'b')
                    {
                        priorPiece = ptr;
                        bidirectional = 1; // now aiming backwards on 1st leg of bidirect
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
                    wildcardSelector |= REAL_SENTENCE_WORD_LIMIT << GAPLIMITSHIFT;  
                    if (positionStart == 0) positionStart = INFINITE_MATCH; // < * resets to allow match anywhere
                }
                if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(USERLOG,"%s ", word);
                continue;
            }
            break;
        case USERVAR_PREFIX: // is user variable defined  $x vs  $x?
            if (IsAlphaUTF8(word[1]) || word[1] == '_' || word[1] == USERVAR_PREFIX) // legal variable, not $ or $100
            {
                 char* val = GetUserVariable(word, false);
                 // val may be empty string either becuase value is NULL
                 // OR because it is the empty string (which is a value).
                 if (*val) matched = true;
                 //else if (val == nullGlobal) matched = false;
                 //else if (val == (nullLocal + 2)) matched = false;
                 else matched = false; // empty string IS a value
             }
            else goto MATCHIT; // more like a word, not a variable
            break;
        case '^': //   function call, function argument  or indirect function variable assign ref like ^$$tmp = null
            if (word[1] == '_' && IsDigit(word[2])) // ^_1 = original
            {
                ptr = wildcardCanonicalText[GetWildcardID(word + 1)]; // ordinary wildcard substituted in place (bug)?
                if (trace & TRACE_PATTERN && CheckTopicTrace()) Log(USERLOG, "%s=>", word);
                continue;
            }
            else if (IsDigit(word[1]) || word[1] == USERVAR_PREFIX) //   macro argument substitution or indirect function variable
            {
                argumentText = ptr; //   transient substitution of text
                if (IsDigit(word[1]))  ptr = FNVAR(word );
                else if (word[1] == USERVAR_PREFIX) ptr = GetUserVariable(word + 1, false); // get value of variable and continue in place
                if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(USERLOG,"%s=>", word);
                continue;
            }

            D = FindWord(word, 0); // find the function
            if (!D || !(D->internalBits & FUNCTION_NAME))
            {
                matched = false; // shouldnt fail normally, dont disrupt api calls etc, fail quietly
            }
            else if (D->x.codeIndex || D->internalBits & IS_OUTPUT_MACRO) // system function or output macro- execute it
            {
                AllocateOutputBuffer();
                FunctionResult result;
                ptr = PatternOutputmacroCall(D, ptr,result,&hitdata);
                matched = !(result & ENDCODES);
                if (!stricmp(word, "^debug") && (kind == '[' || kind == '{')) matched = false; // be innocuous

                // allowed to do comparisons on answers from system functions but cannot have space before them, but not from user macros
                if (result != NOPROBLEM_BIT) { ; }
                else if (*ptr == '!' && ptr[1] == ' ')// simple not operator or no operator
                {
                    if (!stricmp(currentOutputBase, (char*)"0") || !stricmp(currentOutputBase, (char*)"false")) result = FAILRULE_BIT;	// treat 0 and false as failure along with actual failures
                }
                else if (ptr[1] == '<' || ptr[1] == '>') { ; } // << and >> are not comparison operators in a pattern
                else if (IsComparison(*ptr) && *(ptr - 1) != ' ' && (*ptr != '!' || ptr[1] == '='))  // ! w/o = is not a comparison
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
                    ptr = ReadCompiledWord(rhs, copy);
                    rhs = copy;

                    if (*rhs == '^') // local function argument or indirect ^$ var  is LHS. copy across real argument
                    {
                        char* atx = "";
                        if (rhs[1] == USERVAR_PREFIX) atx = GetUserVariable(rhs + 1, false);
                        else if (IsDigit(rhs[1])) atx = FNVAR(rhs );
                        atx = SkipWhitespace(atx);
                        strcpy(rhs, atx);
                    }

                    if (*op == '?' && opptr[0] != '~')
                    {
                        matched = MatchTest(reverse, FindWord(currentOutputBase),
                            (positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, NULL, NULL, false,
                            false, 0,&hitdata);
                        if (matched)
                        {
                            positionStart = hitdata.start;
                            positionEnd = hitdata.end;
                        }
                        if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
                    }
                    else
                    {
                        int id;
                        char* word1val = AllocateStack(NULL, MAX_WORD_SIZE);
                        char* word2val = AllocateStack(NULL, MAX_WORD_SIZE);
                        result = HandleRelation(currentOutputBase, op, rhs, false, id, word1val, word2val);
                        ReleaseStack(word1val);
                        matched = (result & ENDCODES) ? 0 : 1;
                    }
                }
                FreeOutputBuffer(); 
            }
            else // user patternmacro function - execute it in pattern context as continuation of current code
            {
                if (functionNest >= MAX_PAREN_NEST) matched = false; // fail, too deep nesting
                else
                {
                    ptr = PatternMacroCall(D, ptr, matched);
                    if (matched) continue; // call was set up ok
                }
            }
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
                if (trace & TRACE_PATTERN  && CheckTopicTrace()) Log(USERLOG,"");
                ptr = ptrStack[--functionNest]; // continue using prior code
                ChangeDepth(-1, "match end function call");
                continue;
            }
            else
            {
                globalDepth = startdepth;
                if (patternDepth-- == 0) patternDepth = 0;
                FreeBuffer();
                return false; // shouldn't happen
            }
            break;
        case '"':  //   double quoted string
            matched = FindPhrase(word, (positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, reverse,
                positionStart, positionEnd,&hitdata);
            if (matched)
            {
                positionStart = hitdata.start;
                positionEnd = hitdata.end;
            }
            if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
            break;
        case SYSVAR_PREFIX: //   system variable
            if (!word[1]) // simple % 
            {
                matched = MatchTest(reverse, FindWord(word), (positionEnd < basicStart&& firstMatched < 0) ? basicStart : positionEnd, NULL, NULL,
                    statusBits & QUOTE_BIT, false, 0,&hitdata); //   possessive 's
                if (matched)
                {
                    positionStart = hitdata.start;
                    positionEnd = hitdata.end;
                }
                if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
            }
            else if (!stricmp(word, "%trace_on"))
            {
                if (!blockapitrace)
                {
                    trace = TRACE_PATTERN;
                    if (!strnicmp(ptr, "all", 3))
                    {
                        trace = (unsigned int)-1;
                        ptr += 4;
                    }
                }
                continue;
            }
            else if (!stricmp(word, "%trace_off"))
            {
                trace = 0;
                continue;
            }
            else if (!stricmp(word, "%testpattern-prescan") || !stricmp(word, "%testpattern-nosave")) continue;
            else matched = SysVarExists(word);
            break;
        case '?': //  question sentence? or variable search for 
            if (word[1] == '$')
            {
                strcpy(word, GetUserVariable(word + 1, false));
                goto MATCHIT;
            }
            else
            {
                ptr = nextTokenStart;
                if (!word[1]) matched = (tokenFlags & QUESTIONMARK) ? true : false;
                else matched = false;
            }
            break;
        case ':': // assignment
		{
			if (strchr(word, '=' )) //  assignment within
			{
				char lhsside[MAX_WORD_SIZE];
				char* lhs = lhsside;
				char op[10];
				char rhsside[MAX_WORD_SIZE];
				char* rhs = rhsside;
				DecodeAssignment(word, lhs, op, rhs);
				FunctionResult result;
				char* ptr = AllocateBuffer("patternmatchassign");
				sprintf(ptr, " %s ", op);
				strcat(ptr, rhs);
                char* buffer = AllocateBuffer("calling performassign");
				PerformAssignment(lhs, ptr, buffer, result);
                FreeBuffer("calling performassign");
				FreeBuffer("patternmatchassign");

				if (result == NOPROBLEM_BIT) matched = true;
			}
			else goto MATCHIT;
		}
		break;
        case '=': //   a comparison test - never quotes the left side. Right side could be quoted
                  //   format is:  = 1-bytejumpcodeToComparator leftside comparator rightside
            if (!word[1] || word[1] == '=') //   the simple = being present or == (probably mistake in pattern)
            {
                matched = MatchTest(reverse, FindWord(word), (positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, NULL, NULL,
                    statusBits & QUOTE_BIT, false, 0,&hitdata); //   possessive 's
                if (matched)
                {
                    positionStart = hitdata.start;
                    positionEnd = hitdata.end;
                }
                if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
            }
            //   if left side is anything but a variable $ or _ or @, it must be found in sentence and that is what we compare against
            else
            {
                char copy[MAX_WORD_SIZE];
                strcpy(copy, word);
                strcpy(word, copy); // refresh for retry debugging
                char lhsside[MAX_WORD_SIZE];
                char* lhs = lhsside;
                char op[10];
                char rhsside[MAX_WORD_SIZE];
                char* rhs = rhsside;
                DecodeComparison(word, lhs, op, rhs);
                if (trace & TRACE_PATTERN)
                {
                    char lhscopy[MAX_WORD_SIZE];
                    strcpy(lhscopy, lhs);
                    if (*lhs == '_') // show value
                    {
                        char val[MAX_WORD_SIZE];
                        strcpy(val, "`");
                        strncpy(val + 1, wildcardOriginalText[GetWildcardID(lhs)], 15);
                        strcpy(val + 16, "...");
                        strcat(val, "`");
                        strcat(lhscopy, val);
                    }
                    else if (*lhs == '$') // show value
                    {
                        char val[MAX_WORD_SIZE];
                        strcpy(val, "`");
                        strncpy(val + 1, GetUserVariable(lhs), 15);
                        strcpy(val + 16, "...");
                        strcat(val, "`");
                        strcat(lhscopy, val);
                    }
                    else if (*lhs == '%') // show value
                    {
                        char val[MAX_WORD_SIZE];
                        strcpy(val, "`");
                        strcpy(val + 1, SystemVariable(lhs, NULL));
                        strcpy(val + 16, "...");
                        strcat(val, "`");
                        strcat(lhscopy, val);
                    }

                    sprintf(word, (char*)"%s%s%s", lhscopy, op, rhs); //rephrase for trace later
                }
                if (*lhs == '^') DecodeFNRef(lhs); // local function arg indirect ^$ var or _ as LHS
                if (*rhs == '^') DecodeFNRef(rhs);// local function argument or indirect ^$ var  is LHS. copy across real argument

                bool quoted = false;
                if (*lhs == '\'') // left side is quoted
                {
                    ++lhs;
                    quoted = true;
                }
                if (*op == '?' && *rhs == USERVAR_PREFIX)
                {
                    rhs = GetUserVariable(rhs, false);
                }
                if (*op == '?' && *rhs != '~') // NOT a ? into a set test - means does this thing exist in sentence
                {
                    char* val = "";
                    if (*lhs == USERVAR_PREFIX) val = GetUserVariable(lhs, false);
                    else if (*lhs == '_') val = (quoted) ? wildcardOriginalText[GetWildcardID(lhs)] : wildcardCanonicalText[GetWildcardID(lhs)];
                    else if (*lhs == '^' && IsDigit(lhs[1])) val = FNVAR(lhs );
                    else if (*lhs == SYSVAR_PREFIX) val = SystemVariable(lhs, NULL);
                    else val = lhs; // direct word

                    if (*val == '"') // phrase requires dynamic matching
                    {
                        matched = FindPhrase(val, (positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, reverse,
                            positionStart, positionEnd,&hitdata);
                        if (matched)
                        {
                            positionStart = hitdata.start;
                            positionEnd = hitdata.end;
                        }
                        if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
                        if (trace & TRACE_PATTERN) sprintf(word, (char*)"%s`%s`%s", lhs, val, op);
                        break;
                    }

                    matched = MatchTest(reverse, FindWord(val), (positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, NULL, NULL,
                        quoted, false, 0,&hitdata);
                    if (matched)
                    {
                        positionStart = hitdata.start;
                        positionEnd = hitdata.end;
                    }
                    else  hitdata.word = 0;
                    if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart; //   first SOLID match
                    if (trace & TRACE_PATTERN) sprintf(word, (char*)"%s`%s`%s", lhs, val, op);
                    break;
                }

                int lhschar = *lhs;
                if (IsComparison(*op)) // otherwise for words and concepts, look up in sentence and check relation there
                {
                    if (lhschar == '_' && quoted) --lhs; // include the quote
                    char* word1val = AllocateStack(NULL, MAX_WORD_SIZE);
                    char* word2val = AllocateStack(NULL, MAX_WORD_SIZE);
                    FunctionResult answer = HandleRelation(lhs, op, rhs, false, id, word1val, word2val);
                    matched = (answer & ENDCODES) ? 0 : 1;
                    if (trace  & TRACE_PATTERN)
                    {
                        if (!stricmp(lhs, word1val)) *word1val = 0; // dont need redundant constants in trace
                        if (!stricmp(rhs, word2val)) *word2val = 0; // dont need redundant constants in trace
                        if (*word1val && *word2val) sprintf(word, (char*)"%s`%s`%s%s`%s`", lhs, word1val, op, rhs, word2val);
                        else if (*word1val) sprintf(word, (char*)"%s`%s`%s%s", lhs, word1val, op, rhs);
                        else if (*word2val) sprintf(word, (char*)"%s%s%s`%s`", lhs, op, rhs, word2val);
                        else sprintf(word, (char*)"%s%s%s", lhs, op, rhs);
                    }
                    ReleaseStack(word1val);
                }
                else // find and test
                {
                    matched = MatchTest(reverse, FindWord(lhs), (positionEnd < basicStart&& firstMatched < 0) ? basicStart : positionEnd, op, rhs,
                        quoted,  false, 0, &hitdata); //   MUST match later 
                    if (matched)
                    {
                        positionStart = hitdata.start;
                        positionEnd = hitdata.end;
                    }
                    else hitdata.word = 0;
                    if (!matched) break;
                }
            }
            break;
        case '\\': //   escape to allow [ ] () < > ' {  } ! as words and 's possessive And anything else for that matter
        {
            matched = MatchTest(reverse, FindWord(word + 1), (positionEnd < basicStart && firstMatched < 0) ? basicStart : positionEnd, NULL, NULL,
                statusBits & QUOTE_BIT,  false, 0,&hitdata);
            if (!(statusBits & NOT_BIT) && matched && firstMatched < 0) firstMatched = positionStart;
            if (matched) 
            {
                positionStart = hitdata.start;
                positionEnd = hitdata.end;
            }
            else if (word[1] == '!') matched = (wordCount && (tokenFlags & EXCLAMATIONMARK)); //   exclamatory sentence
            else if (word[1] == '?') matched = (tokenFlags & QUESTIONMARK) ? true : false; //   question sentence
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
        MATCHIT:
            int teststart = (positionEnd < basicStart && firstMatched < 0) ? basicStart : (reverse ? positionStart : positionEnd);
            foundaword = FindWord(word);
            if (!foundaword ) foundaword = StoreWord(word,AS_IS); // may be composite concept requiring on fly merger, or replaceword pattern word
            matched = MatchTest(reverse, foundaword, teststart, NULL, NULL,
                statusBits & QUOTE_BIT,  false, wildgap,&hitdata);
            if (!matched) foundaword = NULL;
            else
            {
                positionStart = hitdata.start;
                positionEnd = hitdata.end;
                if (hitdata.word) foundaword = Meaning2Word(hitdata.word);
                if (!(statusBits & NOT_BIT) && firstMatched <= 0)  firstMatched = positionStart;
            }
        }
        // NOTE: coming out of using Matchtest, positionEnd may be composite of subject and object

        statusBits &= -1 ^ QUOTE_BIT; // turn off any pending quote
        if (statusBits & NOT_BIT) // flip success to failure maybe
        {
            foundaword = NULL; // not a real word to pay attention to
            hitdata.word = 0; 
            if (matched)
            {
                if (statusBits & NOTNOT_BIT) // is match immediately after or not
                {
                    if (!reverse)
                    {
                        if (positionStart == (oldEnd + 1)) matched = false;
                    }
                    else if ((positionEnd & REMOVE_SUBJECT) == (oldStart - 1)) matched = false;
                }
                else matched = false;
                statusBits &= -1 ^ (NOT_BIT | NOTNOT_BIT);
                positionStart = oldStart; //   restore any changed position values (if we succeed we would and if we fail it doesnt harm us)
                positionEnd = oldEnd;
            }
        }

        //   prove any wild gap was legal, accounting for ignored words if needed
        int matchStarted; // actual place included in match from direction of match (start of prior word if reverse)
        if (!reverse) matchStarted = (positionStart < REAL_SENTENCE_WORD_LIMIT) ? positionStart : 0; // position start may be the unlimited access value
        else // reverse
        {
            if ((positionEnd & REMOVE_SUBJECT) > wordCount) positionEnd = wordCount;
            matchStarted = (positionStart < REAL_SENTENCE_WORD_LIMIT) ? positionEnd  : wordCount; // position start may be the unlimited access value
        }
        if (matchStarted == INFINITE_MATCH) matchStarted = 1;
        bool legalgap = false;
        unsigned int memorizationStart = positionStart;
        if ((wildcardSelector & WILDGAP) && matched) // test for legality of gap
        {
            int endOfGap = matchStarted; // where we think we are now at current match
            memorizationStart = matchStarted = (wildcardSelector & GAPSTART); // actual word we started at
            
            int ignore = matchStarted;
            int gapSize;
            if (!reverse)
            {
                // gap counting must skip ignored slots
                gapSize = endOfGap - matchStarted; // *~2 debug() something will generate a -1 started... this is safe here
                while (ignore < endOfGap) { if (unmarked[ignore++]) --gapSize; } // no charge for ignored words in gap
            }
            else // reverse
            {
                gapSize = matchStarted - endOfGap; // *~2 debug() something will generate a -1 started... this is safe here
                while (ignore > endOfGap) {if (unmarked[ignore--]) --gapSize;}// no charge for ignored words in gap
            }

            int limit = (wildcardSelector & GAPLIMIT) >> GAPLIMITSHIFT;
            if (gapSize < 0) legalgap = false; // if searched _@10- *~4 >
            else if (gapSize <= limit)
            {
                legalgap = true;   //   we know this was legal, so allow advancement test not to fail- matched gap is started...oldEnd-1
                // need to know (< *) pattern matcher 
                if (!depth && *word == ')' && firstMatched < 0) firstMatched = memorizationStart;
            }
			else if (depth == 0 && *word == ')')
			{
				legalgap = true; // variable gap to end of top level ) can match, just cant match *~2 > )
			}
            else
            {
                matched = false;  // more words than limit
                wildcardSelector &= -1 ^ (WILDMEMORIZEGAP | WILDGAP); //   turn off any save flag
            }
        }
        if (matched) // perform any memorization
        {
            if (foundaword) // we accept this positive match
            {
                if (!nopatterndata)
                {
                    MarkMatchLocation(positionStart, positionEnd & REMOVE_SUBJECT, depth);
                    matchedWordsList = AllocateHeapval(HV1_WORDP | HV2_INT, matchedWordsList, (uint64)foundaword, depth);
                }
                if (beginmatch == -1) beginmatch = positionStart; // first match in this level
            }
            if (oldEnd == (positionEnd & REMOVE_SUBJECT) && oldStart == positionStart) // something like function call or variable existence, didnt change position
            {
                if (wildcardSelector == WILDMEMORIZESPECIFIC && *word == USERVAR_PREFIX)
                {
                    char* value = GetUserVariable(word, false);
                    SetWildCard(value, value, 0, 0);  // specific swallow
                }
            }
            else if (wildcardSelector & (WILDMEMORIZEGAP | WILDMEMORIZESPECIFIC)) //   memorize ONE -  will be gap or specific not in front of () {} []
            {
                if (matchStarted == INFINITE_MATCH) matchStarted = 1;
                if (positionStart == INFINITE_MATCH) positionStart = 1; // always is moving forward
                if (wildcardSelector & WILDMEMORIZEGAP) //   would be first if both
                {
                    int index = (wildcardSelector >> GAP_SHIFT) & 0x0000001f;
                    if (reverse)
                    {
                        // if starter was at beginning, there is no data to match
                        if (positionStart == 0 || (memorizationStart - (positionEnd & REMOVE_SUBJECT)) == 0) SetWildCardGivenValue((char*)"", (char*)"", 0, positionEnd , index,&hitdata); // empty gap
                        else if (positionStart < (positionEnd & REMOVE_SUBJECT)) SetWildCardGiven(positionStart, positionEnd, true, index);  //   wildcard legal swallow between elements
                        else SetWildCardGiven((positionEnd & REMOVE_SUBJECT) + 1, memorizationStart, true, index);  //   wildcard legal swallow between elements
                    }
                    else if ((positionStart - memorizationStart) == 0) SetWildCardGivenValue((char*)"", (char*)"", 0, oldEnd + 1, index,&hitdata); // empty gap
                    else // normal gap
                    {
                        SetWildCardGiven(memorizationStart, positionStart - 1, true, index);  //   wildcard legal swallow between elements
                        if ((positionEnd & REMOVE_SUBJECT) < (positionStart - 1)) positionEnd = positionStart - 1;	// gap closes out 
                    }
                }
                if (wildcardSelector & WILDMEMORIZESPECIFIC)
                {
                    int index = (wildcardSelector >> SPECIFIC_SHIFT) & 0x0000001f;
                    SetWildCardGiven(positionStart, positionEnd, true, index,&hitdata);  // specific swallow 
                    if (strchr(wildcardCanonicalText[index], ' ')) // is lower case canonical a dictionary word with content?
                    {
                        char* word = AllocateStack(NULL, MAX_WORD_SIZE);
                        strcpy(word, wildcardCanonicalText[index]);
                        char* at = word;
                        while ((at = strchr(at, ' '))) *at = '_';
                        WORDP D = FindWord(word, 0); // find without underscores..
                        if (D && D->properties & PART_OF_SPEECH)  strcpy(wildcardCanonicalText[index], D->word);
                        ReleaseStack(word);
                    }
                }
            }
            wildcardSelector = 0; // completes all memorization/gaps at this level
        }
        else // (  fix side effects of anything that failed to match by reverting
        {
            positionStart = oldStart;
            positionEnd = oldEnd;
            if (*word == '{')
                wildcardSelector = originalWildcardSelector; // failed optional match after return from calling it, need to restore any wildcard eg:  test *~1 {me} [ready]
            else if (kind == '(' || kind == '<') wildcardSelector = 0; // should NOT clear this inside a [] or a {} on failure since they must try again
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
                if (bidirectional == 1) reverse = !reverse; // we were looking backwards, resume normal
                bidirectional = 0;
            }
            else if (bidirectional == 1) // looking back failed
            {
                reverse = !reverse; // look other way
                bidirectional = 2; // now on 2nd stage of bidirectional
                ptr = priorPiece;
                wildcardSelector = bidirectionalSelector;
                wildcardIndex = bidirectionalWildcardIndex;
                if (trace & TRACE_PATTERN  && CheckTopicTrace())
                {
                    Log(USERLOG, "%s- *~nb-flip %d ", word, firstMatched);
                }
                continue;
            }
            else if (bidirectional == 2) // failed on 2nd part of bidirectional
            {
                if (++bilimit > 100) // we are pointed at initial match, which can unbind
                {
                    rebindable = 0; // dont allow it to retry find
                }
                bidirectional = 0; // we end this bidirect, but can rebind maybe
            }
            else bidirectional = 0; // give up fully
        }

        positionEnd &= REMOVE_SUBJECT; 

        //   end sequence/choice/optional/random
        if (*word == ')' || *word == ']' || *word == '}' || (*word == '>' && word[1] == '>'))
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
            statusBits &= -1 ^ (NOT_BIT | NOTNOT_BIT);
        }

        //   word ptr may not advance more than 1 at a time (allowed to advance 0 - like a string match or test) unless global unmarks in progress
        //   But if initial start was INFINITE_MATCH, allowed to match anywhere to start with
        if (legalgap || !matched || positionStart == INFINITE_MATCH || oldStart == INFINITE_MATCH) { ; }
        else if (reverse)
        {
            if (oldStart < oldEnd && positionEnd >= (oldStart - 1)) { ; } // legal move ahead given matched WITHIN last time
            else if (positionEnd < (oldEnd - 1))  // failed to match position advance
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
        else if (!legalgap) //  && rebindable != 2  forward requirement must be tested (1 means we are allowed to rebind)
        {
            if (oldEnd < oldStart && positionStart <= (oldStart + 1)) { ; } // legal move ahead given matched WITHIN last time -- what does match within mean?
            else if (positionStart > (oldEnd + 1))  // failed to match position advance of one
            {
                int ignored = oldEnd + 1;
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
        else // simple advance failed, not rebindable
        {
            matched = false;
            positionStart = oldStart;
            positionEnd = oldEnd;
        }

        if (trace & TRACE_PATTERN  && CheckTopicTrace())
        {
            bool success = matched;
            if (statusBits & NOT_BIT) success = !success;
            if (*word == '[' || *word == '{' || *word == '(' || (*word == '<' && word[1] == '<')) 
            {// seen on RETURN from a matching pair
               } 
            else if (*word == ']' || *word == '}' || *word == ')' || (*word == '>' && word[1] == '>')) {}
            else
            {
                if (*word == '~' && IsDigit(word[1])) // internal concept
                {
                    WORDP D = FindWord(word);
                    FACT* F = GetObjectNondeadHead(D);
                    int count = 0;
                    FACT** factlist = (FACT**)AllocateBuffer();
                    while (F)
                    {
                        factlist[count++] = F;
                        F = GetObjectNondeadNext(F);
                    }
                    while (count--)
                    {
                        F = factlist[count];
                        char* key = Meaning2Word(F->subject)->word;
                        if (!matched) Log(USERLOG,"%s- ", key);
                        else if (matched && positionStart > 0 && positionStart == positionEnd &&
                            (!stricmp(key, wordStarts[positionStart]) || !stricmp(key, wordCanonical[positionStart])))
                        {
                            Log(USERLOG,"%s+ ", key);
                            break; // dont need whole list
                        }
                        else if (matched && positionStart != positionEnd)
                        { 
                            strcpy(xword, key);
                            char* x = strchr(xword, '_');
                            if (!x) x = strchr(xword, ' ');
                            if (x) *x = 0; // does first word of phrase match?
                            if (positionStart > 0 && (!stricmp(xword, wordStarts[positionStart]) || !stricmp(xword, wordCanonical[positionStart])))
                            {
                                Log(USERLOG,"%s+ ", key);
                                break; // dont need whole list
                            }
                            else Log(USERLOG,"%s- ", key);
                        }
                        else Log(USERLOG,"%s- ", key);
                    }
                    FreeBuffer();
                }
				else
				{
					// remove comparison and assigned flagging
					if (*word == ':' && strchr(word, '=')) // assignment
					{
						char lhsside[MAX_WORD_SIZE];
						char* lhs = lhsside;
						char op[10];
						char rhsside[MAX_WORD_SIZE];
						char* rhs = rhsside;
						DecodeAssignment(word, lhs, op, rhs);
                        if (*lhs == '$')
                        {
                            char* val = GetUserVariable(lhs, false);
                            if (stricmp(rhs,val))Log(USERLOG, "%s:%s%s`%s`", lhs, op, rhs, val);
                            else Log(USERLOG, "%s:%s%s", lhs, op, rhs);
                        }
						else if (*lhs == '_') Log(USERLOG,"%s:%s%s `%s`", lhs, op, rhs, wildcardOriginalText[GetWildcardID(lhs)]);
						else Log(USERLOG,"%s:%s%s", lhs, op, rhs);
					}
                    else if (*word == '=' && word[1]) // comparison
                    {
                        char lhsside[MAX_WORD_SIZE];
                        char* lhs = lhsside;
                        char op[10];
                        char rhsside[MAX_WORD_SIZE];
                        char* rhs = rhsside;
                        DecodeComparison(word, lhs, op, rhs);
                        if (*lhs == '_')
                        {
                            char val[MAX_WORD_SIZE];
                            strcpy(val, "('");
                            strncpy(val+2, wildcardOriginalText[GetWildcardID(lhs)], 15);
                            strcpy(val + 17, "...");
                            strcat(val, "`)");
                            strcat(lhs, val);
                        }
						Log(USERLOG,"%s%s%s", lhs, op, rhs);
					}
					else if (*word == '^') Log(USERLOG,"%s(...)", word);
                    else if (*word == '\\') Log(USERLOG, "`%s`", word+1);
                    else Log(USERLOG,"`%s`", word);
				}
                if (*word == '~' && matched && IsDigit(word[1])) { ; }
                else if (*word == '~' && matched)
                {
                    if (positionStart <= 0 || positionStart > wordCount || positionEnd <= 0 || positionEnd > wordCount) { ; } // still in init startup?
                    else if (positionStart != positionEnd) Log(USERLOG,"`%s...%s`", wordStarts[positionStart], wordStarts[positionEnd]);
                    else Log(USERLOG,"`%s`", wordStarts[positionStart]);
                }
                else if (*word == USERVAR_PREFIX && matched)
                {
                    Log(USERLOG,"`%s`", GetUserVariable(word, false));
                }
                else if (*word == '*' && matched && positionStart > 0 && positionStart <= wordCount && positionEnd <= wordCount)
                {
                    *word = 0;
                    for (int i = positionStart; i <= positionEnd; ++i)
                    {
                        if (*word) strcat(word, (char*)" ");
                        strcat(word, wordStarts[i]);
                    }
                    Log(USERLOG,"(%s)", word);
                }
                if (*word == '~' && IsDigit(word[1]) ){ ; } // internal concept
                else Log(USERLOG, (success) ? (char*)"+ " : (char*)"- ");
                if (*word == '[' || *word == '{' || *word == '(' || (*word == '<' && word[1] == '<'))
                {
                    ;
                }
            }
        }

        //   now verify position of match, NEXT is default for (type, not matter for others
        if (kind == '(' || kind == '<') //   ALL must match in sequence ( or jumbled <
        {
            //   we failed, retry shifting the start if we can
            if (!matched)
            {
                if (kind == '<') break;	// we failed << >>

                if (rebindable == 1 && firstMatched > 0 && firstMatched < NORETRY && --rebindlimit > 0) //   we are top level and have a first matcher, we can try to shift it
                {
                    if (trace & TRACE_PATTERN  && CheckTopicTrace())
                    {
                        Log(USERLOG, "\r\n");
                        Log(FORCETABUSERLOG, (char*)"------ Try pattern matching again, after word %d`%s`\r\n", firstMatched, wordStarts[firstMatched]);
                       // Log(USERLOG,"");
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
                    if (!nopatterndata)
                    {
                        if (depth < MATCHEDDEPTH)
                        {
                            // save bits first
                            retryBits[0] = matchedBits[depth][0];
                            retryBits[1] = matchedBits[depth][1];
                            retryBits[2] = matchedBits[depth][2];
                            retryBits[3] = matchedBits[depth][3];
                            // clear bit markers
                            matchedBits[depth][0] = 0;
                            matchedBits[depth][1] = 0;
                            matchedBits[depth][2] = 0;
                            matchedBits[depth][3] = 0;
                        }
                        while (matchedWordsList)
                        {
                            uint64 val1;
                            uint64 val2;
                            HEAPREF list = UnpackHeapval(matchedWordsList, val1, val2);
                            if (val2 != depth) break;
                            matchedWordsList = list;
                        }
                    }
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
            else if (kind == '{' || kind == '[')
            {
                if (kind == '[' && depth == 1)
                {
                    patternchoice = starttoken;
                }
                success = true;
                break;	// we matched all needed in this
            }
        }
    }

    //   begin the return sequence
    while (functionNest > startNest)//   since we are failing, we need to close out all function calls in progress at this level
    {
        --functionNest;
        ChangeDepth(-1, "match return sequence");
    }

    if (success)
    {
        hitdata.end = positionEnd;
        // note we are not returning disjoint value if any
        if (depth > 0 && *word == ')' && !reverse) hitdata.start = positionStart;		// where we began this level
        else if (depth > 0 && reverse && positionStart > positionEnd) // our data is in reverse, flip it around
        {
            hitdata.end = positionStart;
            hitdata.start = positionEnd;
        }
        else if (depth > 0 && *word == ')')  hitdata.start =  positionStart;		// where we began this level
        else
        {
            if (firstMatched == NORETRY)  hitdata.start = 1; // non-rebindable < start of pattern
            else  hitdata.start = (firstMatched > 0) ? firstMatched : positionStart; // if never matched a real token, report 0 as start
        }
    }

    //   if we leave this level w/o seeing the close, show it by elipsis 
    //   can only happen on [ and { via success and on ) by failure
    globalDepth = startdepth; // insures even if we skip >> closes, we get correct depth
    if (trace & TRACE_PATTERN && (depth || detailpattern) && CheckTopicTrace())
    {
          if (*word != ')' && *word != '}' && *word != ']' && (*word != '>' || word[1] != '>'))
        {
            if (*ptr != '}' && *ptr != ']' && *ptr != ')' && (*ptr != '>' || ptr[1] != '>')) 
                Log(USERLOG,"...");	// there is more in the pattern still
            if (success)
            {
                if (kind == '<') Log(USERLOG,">>");
                else Log(USERLOG,"%c", (kind == '{') ? '}' : ']');
            }
            else if (kind == '<') Log(USERLOG,">>");
            else Log(USERLOG,")");
        }
		else
		{
			Log(USERLOG,"%s", word); // we read to end of pattern 
		}
		if (detailpattern)
		{
			char s[10];
			if (positionStart == INFINITE_MATCH) strcpy(s, "?");
			else sprintf(s, "%d", positionStart);
			Log(USERLOG, "-%s-%d", s, positionEnd);
		}
		if (*word == '}') Log(USERLOG,"+ "); // optional always matches, by definition
        else Log(USERLOG,"%c ", matched ? '+' : '-');

        // A classic pattern is:
            //       ( !x !y [ item item item])  indentBasis == 1
            // or  ([  (!y [item item item]) ( ) ] ) indentBasis== 3
        if (depth <= indentBasis)
        {
            if (depth != 1) Log(USERLOG,"\r\n"); 
        }
        else if (depth == (indentBasis + 1) && kind == '(')
        {
            if (kindprior[depth] == '(' || kindprior[depth] == '[')
            {
                char* x = SkipWhitespace(BalanceParen(ptr, true, false));
                if (*x != ']' && *x != ')')  Log(USERLOG, "\r\n");
            }
        }
    }
    if (trace & TRACE_PATTERN && !depth)
    {
        if (matched)  Log(USERLOG,")+\r\n");
        else if (*ptr == ')') Log(USERLOG, ")-\r\n");
        else Log(USERLOG, "...)-\r\n");
    }
    if (patternDepth-- == 0) patternDepth = 0;
    FreeBuffer();
    return success;
}

void ExecuteConceptPatterns(FACT* specificPattern)
{
    WORDP D = FindWord("conceptPattern");
    if (!D) return; // none
   
    bool hasConceptPattern = false;
    uint64 start_time = ElapsedMilliseconds();

    SAVESYSTEMSTATE()

    char* buffer = AllocateBuffer();
    FACT* F = (specificPattern ? specificPattern : GetVerbNondeadHead(D));
    WORDP lastMatchedConcept = NULL;
    while (F)
    {
        MEANING conceptMeaning = F->object;
        WORDP concept = Meaning2Word(F->object);
        char* pattern = Meaning2Word(F->subject)->word;
        F = (specificPattern ? NULL : GetVerbNondeadNext(F));
        if (concept == lastMatchedConcept) continue; // one match per

        // all patterns here have been compiled, skip compilation mark
        ++pattern;

        if (!*pattern) continue;     // fails if no pattern compiled

        int start = 0;
        int end = 0;
        bool retried = false;
        MARKDATA hitdata;
    retry:
        int matched = 0;
        wildcardIndex = 0;  //   reset wildcard allocation on top-level pattern match
        char oldmark[MAX_SENTENCE_LENGTH];
        memcpy(oldmark, unmarked, MAX_SENTENCE_LENGTH);
        hitdata.start = start;
        bool match = Match(pattern+2, 0, hitdata, 1, 0, matched, '(') != 0;  //   skip paren and treat as NOT at start depth, dont reset wildcards- if it does match, wildcards are bound
        start = hitdata.start;
        end = hitdata.end;
        memcpy(unmarked, oldmark, MAX_SENTENCE_LENGTH);
        ShowMatchResult(!match ? FAILRULE_BIT : NOPROBLEM_BIT, pattern, NULL, 0);
        if (match)
        {
            lastMatchedConcept = concept;
            // Mark specific thing else using returned start
            if (wildcardIndex) // wildcard assigned, use that
            {
                start = WILDCARD_START(wildcardPosition[0]);
                end = WILDCARD_END_ONLY(wildcardPosition[0]);
            }
            else if (end == 0) start = end = 1;  // didnt match a word
            if ((trace & TRACE_PATTERN) || showMark || (trace & TRACE_PREPARE))
            {
                if (!hasConceptPattern)
                {
                    Log(USERLOG,"\r\n");
                    Log(USERLOG,"Concept Patterns:\r\n");
                    hasConceptPattern = true;
                }
                Log(USERLOG,"mark  @word %s %d-%d ", concept->word, start, end);
            }
            MarkMeaningAndImplications(0, 0, conceptMeaning, start, end, FIXED, false, false);
            if (*concept->word != '~') Add2ConceptTopicList(concepts, concept, start, end, true); // add ordinary word to concept list directly as WordHit will not store anything but concepts

            if (patternRetry)
            {
                if (end > start) start = end;    // continue from last match location
                if (start > MAX_SENTENCE_LENGTH || start < 0)
                {
                    start = 0; // never matched internal words - is at infinite start -- WHY allow this?
                }
                else
                {
                    retried = true;
                    goto retry;
                }
            }
        }
    }
    RESTORESYSTEMSTATE()
    
    if (hasConceptPattern) Log(USERLOG,"\r\n");
    int diff = (int)(ElapsedMilliseconds() - start_time);
    TrackTime((char*)"ConceptPatterns",diff);
}
