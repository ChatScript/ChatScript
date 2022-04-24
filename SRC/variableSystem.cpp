// variableSystem.cpp - manage user variables ($variables)

#include "common.h"
int bbb = 0;
#ifdef INFORMATION
There are 5 kinds of variables.
1. User variables beginning wih $(regular and transient which begin with $$)
2. Wildcard variables beginning with _
3. Fact sets beginning with @
4. Function variables beginning with ^
5. System variables beginning with %
#endif
HEAPREF userVariableThreadList = NULL;
STACKREF stackedUserVariableThreadList = NULL;
int impliedSet = ALREADY_HANDLED;	// what fact set is involved in operation
int impliedWild = ALREADY_HANDLED;	// what wildcard is involved in operation
char impliedOp = 0;					// for impliedSet, what op is in effect += = 
HEAPREF variableChangedThreadlist = NULL;
char* nullLocal = "``";
char* nullGlobal = "";
int wildcardIndex = 0;
char wildcardOriginalText[MAX_WILDCARDS + 1][MAX_MATCHVAR_SIZE + 1];  // spot wild cards can be stored
char wildcardCanonicalText[MAX_WILDCARDS + 1][MAX_MATCHVAR_SIZE + 1];  // spot wild cards can be stored
unsigned int wildcardPosition[MAX_WILDCARDS + 1]; // spot it started and ended in sentence
char wildcardSeparator[2];
bool wildcardSeparatorGiven = false;

//   list of active variables needing saving

WORDP tracedFunctionsList[MAX_TRACED_FUNCTIONS];
HEAPREF kernelVariableThreadList = NULL;
HEAPREF botVariableThreadList = NULL;
unsigned int tracedFunctionsIndex;
unsigned int modifiedTraceVal = 0;
bool modifiedTrace = false;
unsigned int modifiedTimingVal = 0;
bool modifiedTiming = false;

void InitVariableSystem()
{
	wildcardSeparatorGiven = false;
    *wildcardSeparator = ' ';
    wildcardSeparator[1] = 0;
    kernelVariableThreadList = botVariableThreadList = userVariableThreadList = NULL;
    tracedFunctionsIndex = 0;
}

int GetWildcardID(char* x) // wildcard id is "_10" or "_3"
{
    if (!IsDigit(x[1])) return ILLEGAL_MATCHVARIABLE;
    unsigned int n = x[1] - '0';
    char c = x[2];
    if (IsDigit(c)) n = (n * 10) + (c - '0');
    return (n > MAX_WILDCARDS) ? ILLEGAL_MATCHVARIABLE : n;
}

static void CompleteWildcard()
{
    WORDP D = FindWord(wildcardCanonicalText[wildcardIndex]);
    if (D && D->properties & D->internalBits & UPPERCASE_HASH)  // but may not be found if original has plural or such or if uses _
    {
        strcpy(wildcardCanonicalText[wildcardIndex], D->word);
    }

    ++wildcardIndex;
    if (wildcardIndex > MAX_WILDCARDS) wildcardIndex = 0;
}

static void FlipSeparator(char* word, char* buffer)
{
	char oppositeSeparator = 0;
	if (*wildcardSeparator == ' ') oppositeSeparator = '_';
	else if (*wildcardSeparator == '_') oppositeSeparator = ' ';
	if (strchr(word, *wildcardSeparator)) // need to change wanted notation to what we are supposed to return
	{
		strcpy(buffer, word);
		char* at = buffer;
		while ((at = strchr(at, *wildcardSeparator))) *at = oppositeSeparator;
	}
}

void JoinMatch(int start, int end, int index, bool inpattern)
{
    // ignore invalid index
    if (index > MAX_WILDCARDS) return;
    // concatenate the match value
    bool started = false;
    bool proper = false;
    int realend = end & REMOVE_SUBJECT;
	// generate the std expected memorizations
    for (int i = start; i <= realend; ++i)
    {
        if (unmarked[i] || i < 1 || i > wordCount) continue;	// ignore words masked or off end
        char* word = wordStarts[i];
        if (!word) continue;
        if (started)
        {
            // no separator on japanese words in japanese
            unsigned char japanletter[8];
            int kind = 0;
            bool japanese = false;
            if (!japanese) japanese = csapicall == TEST_PATTERN && IsJapanese(word, (unsigned char*)&japanletter, kind);
            if (!kind)
            {
                strcat(wildcardOriginalText[index], wildcardSeparator);
                strcat(wildcardCanonicalText[index], wildcardSeparator);
            }
        }
        else started = true;
        if (IsUpperCase(*word) && word[1]) proper = true; // may be proper name
        strcat(wildcardOriginalText[index], word);
		strcat(wildcardCanonicalText[index], wordCanonical[i] ? wordCanonical[i] : word);
    }
	// we have found phrases using appropriate wildcard separator to be used

	if (start == realend || proper) {} // single word or proper name
	// if not a proper name and any are unknown, the composite is unknown unless it is an entire sentence grab, then preserve pieces
	else if (strstr(wildcardCanonicalText[index], "unknown-word") && (start != 1 || realend != wordCount))
		strcpy(wildcardCanonicalText[index], "unknown-word");  
	// proper names canonical is same, but merely having 1 uppercase is not proper if multiword "I live here" _*
    
	WORDP D = NULL;
	WORDP upperfoundword = NULL;
	if  (uppercaseFind > EXACTNOTSET) upperfoundword = Meaning2Word(uppercaseFind );
	// specific uppercase form requested, but we will not use it if it was not from user input but merely ^mark value
	int len = strlen(wildcardOriginalText[index]);
	int lenc = strlen(wildcardCanonicalText[index]);
	char* word = AllocateBuffer();

	if (upperfoundword) 
	{
		if (!stricmp(upperfoundword->word, wildcardCanonicalText[index]) || !stricmp(upperfoundword->word, wildcardOriginalText[index]))
		{ // we have a match for what we want in the way we want it
				D = upperfoundword; // match was to be capitalized this way
		}
		else
		{
			FlipSeparator(wildcardOriginalText[index], word);
			if (!stricmp(word, upperfoundword->word)) D = upperfoundword; // alternate form matches
			else
			{
				FlipSeparator(wildcardCanonicalText[index], word);
				if (!stricmp(word, upperfoundword->word)) D = upperfoundword;// alternate form matches
			}
		}
	}

	if (D) // we match an uppercase request, it becomes original and canonical (we ignore original being plural)
	{
		strcpy(wildcardOriginalText[index], upperfoundword->word);
		strcpy(wildcardCanonicalText[index], upperfoundword->word);
	}
	else  // we do what we can with the match
	{
		D = FindWord(wildcardOriginalText[index], len);
		if (D) strcpy(wildcardOriginalText[index], D->word); // use capitalization from dictionary
		else if (start != end) // consider if alternate is in dictionary (we used a separator)
		{
				FlipSeparator(wildcardOriginalText[index], word);
				D = FindWord(word, len);
				if (D) strcpy(wildcardOriginalText[index], D->word);
		}
		D = FindWord(wildcardCanonicalText[index], lenc);
		if (D) strcpy(wildcardCanonicalText[index], D->word); // use capitalization from dictionary
		else if (start != realend) // consider if alternate is in dictionary (we used a separator)
		{
			FlipSeparator(wildcardCanonicalText[index], word);
			D = FindWord(word, lenc);
			if (D) strcpy(wildcardCanonicalText[index], D->word);
		}
	}
	FreeBuffer();
   uppercaseFind = EXACTUSEDUP; // use it up if set

    if (trace & TRACE_OUTPUT && !inpattern && CheckTopicTrace()) Log(USERLOG,"_%d=%s/%s ", index, wildcardOriginalText[index], wildcardCanonicalText[index]);
}

void SetWildCard(int start, int end, bool inpattern)
{
    if (end < start) end = start;				// matched within a token
    if (end > wordCount)
    {
        if (start != end) end = wordCount; // for start==end we allow being off end, eg _>
        else start = end = wordCount;
    }
    while (unmarked[start]) ++start; // skip over unmarked words at start
    while (unmarked[end]) --end; // skip over unmarked words at end
    wildcardPosition[wildcardIndex] = start | WILDENDSHIFT(end);
    *wildcardOriginalText[wildcardIndex] = 0;
    *wildcardCanonicalText[wildcardIndex] = 0;
    if (start == 0 || wordCount == 0 || (end == 0 && start != 1)) // null match, like _{ .. }
    {
        ++wildcardIndex;
        if (wildcardIndex > MAX_WILDCARDS) wildcardIndex = 0;
    }
    else // did match
    {
        JoinMatch(start, end, wildcardIndex, inpattern);
        CompleteWildcard();
    }
}

void SetWildCardGiven(int start, int end, bool inpattern, int index)
{
    // ignore invalid index
    if (index > MAX_WILDCARDS) return;
    if (end < start) end = start;				// matched within a token
    if (end > wordCount)
    {
        if (start != end) end = wordCount + 1; // for start==end we allow being off end, eg _>
        else start = end = wordCount + 1;
    }
    *wildcardOriginalText[index] = 0;
    *wildcardCanonicalText[index] = 0;
    while (unmarked[start]) ++start; // skip over unmarked words at start
    while (unmarked[end]) --end; // skip over unmarked words at end
    wildcardPosition[index] = start | WILDENDSHIFT(end);
    if (start == 0 || wordCount == 0 || (end == 0 && start != 1)) // null match, like _{ .. }
    {
    }
    else JoinMatch(start, end, index, inpattern); // did match
}

void SetWildCardNull()
{
    SetWildCardGivenValue((char*)"", (char*)"", -1, -1, wildcardIndex);
    CompleteWildcard();
}

void SetWildCardGivenValue(char* original, char* canonical, int start, int end, int index)
{
    // ignore invalid index
    if (index > MAX_WILDCARDS) return;
    int realend = end & REMOVE_SUBJECT;
    if (realend < start) realend = end = start;				// matched within a token
    if (realend > wordCount && start != realend) realend = wordCount; // for start==end we allow being off end, eg _>
    *wildcardOriginalText[index] = 0;
    *wildcardCanonicalText[index] = 0;
    if (start <= 0 || wordCount == 0 || (realend <= 0 && start != 1)) // null match, like _{ .. }
    {
    }
    else JoinMatch(start, end, index, false); // did match
    if (start == 0) start = end = 1;
    if (start == -1) start = end = 0;
	if (!*original) end = start = 0; // no match found, indicate no position
    wildcardPosition[index] = start | WILDENDSHIFT(end);
}

void SetWildCardIndexStart(int index)
{
    wildcardIndex = index;
}

void SetWildCard(char* value, char* canonicalValue, const char* index, unsigned int position)
{
    // adjust values to assign
    if (!value) value = "";
    if (!canonicalValue) canonicalValue = "";
    if (strlen(value) > MAX_MATCHVAR_SIZE)
    {
        value[MAX_MATCHVAR_SIZE] = 0;
        ReportBug((char*)"Too long matchvariable original value %s", value)
    }
    if (strlen(canonicalValue) > MAX_MATCHVAR_SIZE)
    {
        canonicalValue[MAX_MATCHVAR_SIZE] = 0;
        ReportBug((char*)"Too long matchvariable canonical value %s", value)
    }
    while (value[0] == ' ') ++value;
    while (canonicalValue && canonicalValue[0] == ' ') ++canonicalValue;

    // store the values
    if (index) wildcardIndex = GetWildcardID((char*)index);
    strcpy(wildcardOriginalText[wildcardIndex], value);
    strcpy(wildcardCanonicalText[wildcardIndex], (canonicalValue) ? canonicalValue : value);
    wildcardPosition[wildcardIndex] = position | WILDENDSHIFT(position);

    CompleteWildcard();
}

char* GetwildcardText(unsigned int i, bool canon)
{
    if (i > MAX_WILDCARDS) return "";
    return canon ? wildcardCanonicalText[i] : wildcardOriginalText[i];
}

char* GetUserVariable(const char* word, bool nojson, bool fortrace)
{
    if (!dictionaryLocked && !compiling && !rebooting && currentBeforeLayer != LAYER_BOOT) return "";
    int len = 0;
    const char* separator = (nojson) ? (const char*)NULL : strchr(word, '.');
    const char* bracket = (nojson) ? (const char*)NULL : strchr(word, '[');
    if (!separator && bracket) separator = bracket;
    if (bracket && bracket < separator) separator = bracket;	// this happens first
    if (separator) len = separator - word;
    bool localvar = strstr(word, "$_") != NULL; // local var anywhere in the chain?
    char* item = NULL;
    char* answer;
    bool   showpath = (trace & TRACE_VARIABLE && !fortrace);
    char path[MAX_WORD_SIZE];
     *path = 0;
    if (showpath) strcpy(path, word);
    char flagsValue[20];
    *flagsValue = 0;

    WORDP D = FindWord(word, len, LOWERCASE_LOOKUP);
    if (!D)  goto NULLVALUE; 	//   no such variable

    item = D->w.userValue;
    if (localvar) // is $_local variable whose value we get
    {
        if (!item)  goto NULLVALUE;
        if (*item == LCLVARDATA_PREFIX && item[1] == LCLVARDATA_PREFIX) // should be true
        {
            item += 2; // skip `` marker
        }
    }
    else if (!item) goto NULLVALUE; // null value normal

    if (separator) // json object or array follows this
    {
        if (showpath) sprintf(path, "%s'%s", word,D->word);
    LOOPDEEPER:
        FACT* factvalue = NULL;
        if (IsDigitWord(item, AMERICAN_NUMBERS) && (!strnicmp(separator, ".subject", 8) || !strnicmp(separator, ".verb", 5) || !strnicmp(separator, ".object", 7) || !strnicmp(separator, ".flags", 6))) // fact id?
        {
            int val = atoi(item);
            factvalue = Index2Fact(val);
            if (!factvalue) goto NULLVALUE;
        }
        else D = FindWord(item); // the basic item
        if (!D) goto NULLVALUE;
        if (showpath)
        {
            strcat(path, "=");
            strcat(path,D->word);
        }

        if (*separator == '.' && !strnicmp(item, "ja-", 3)) // dot into array means find value as object
        {
            WORDP key = FindWord(separator + 1); // only can go 1 level here
            FACT* F = GetSubjectNondeadHead(D);
            while (F)
            {
                    if (Meaning2Word(F->object) == key) break; 
                    F = GetSubjectNondeadNext(F);
            }
            if (!F) goto NULLVALUE;
            answer = Meaning2Word(F->verb)->word;
            goto ANSWER;
        }
        if (*separator == '.' && strncmp(item, "jo-", 3) && !factvalue) goto NULLVALUE; // cannot be dotted
        //else if (*separator == '[' && strncmp(item, "ja-", 3)) goto NULLVALUE; // cannot be indexed

                                                                          // is there more later
        char* separator1 = (char*)strchr(separator + 1, '.');	// more dot like $x.y.z?
        char* bracket1 = (char*)strchr(separator + 1, '[');	// more [ like $x[y][z]?
        if (!bracket1) bracket1 = (char*)strchr(separator + 1, ']'); // is there a closer as final
        if (bracket1 && !separator1) separator1 = bracket1;
        if (bracket1 && bracket1 < separator1) separator1 = bracket1;

        // get the label after this current separator
        char* label = (char*)separator + 1;
        if (separator1)
        {
            len = (separator1 - label);
            if (*(separator1 - 1) == ']') --len;	// prior close of array ref
        }
        else len = 0;
        WORDP key = NULL;
        char originalkeyname[SMALL_WORD_SIZE];
        if (factvalue)
        {
            if (separator[1] == 's' || separator[1] == 'S') label = "subject";
            else if (separator[1] == 'v' || separator[1] == 'V') label = "verb";
            else if (separator[1] == 'f' || separator[1] == 'F') label = "flags";
            else label = "object";
            strcpy(originalkeyname, label);
        }
        else if (*separator == '.') // it is a field
        {
            if ((*label != '_' && *label != '\'') || (separator[1] == '_' && !IsDigit(separator[2])) || (separator[1] == '\'' && separator[2] == '_' && !IsDigit(separator[3]))) // any variable ref will be in dictionary as will field name
            {
                if (*label == '\\') ++label; // escaped $, not indirection
                key = FindWord(label, len, PRIMARY_CASE_ALLOWED); // case sensitive find
                if (!key) goto NULLVALUE; // dont recognize such a name
                label = key->word; // actual key name was given
                strcpy(originalkeyname, label);
            }
            if (separator[1] == '$') // indirection
            {
                label = GetUserVariable(key->word, false, true); // key is a variable name , go get it to use real key
                strcpy(originalkeyname, key->word);
                key = FindWord(label);
                if (!key) goto NULLVALUE;	// cannot find
            }
            else if ((separator[1] == '_' && IsDigit(separator[2])) || (separator[1] == '\'' && separator[2] == '_' && IsDigit(separator[3]))) // indirection key match variable
            {
                int index = (separator[1] == '_') ? atoi(separator + 2) : atoi(separator + 3);
                sprintf(originalkeyname, "_%d", index);
                if (index >= 0 && index <= MAX_WILDCARDS)
                {
                    if (separator[1] == '_') strcpy(label, wildcardCanonicalText[index]);
                    else strcpy(label, wildcardOriginalText[index]);
                    key = FindWord(label);
                }
                if (!key) goto NULLVALUE;	// cannot find
            }
        }
        else // it is an index - of either array OR object
        {
            if (IsDigit(*label) || (*label == '-' && label[1] == '1'))
            {
                char* end = strchr(label, ']');
                key = FindWord(label, end - label);
                if (!key)
                {
                    if (*label != '-' || !IsDigit(label[1])) goto NULLVALUE;
                    key = StoreWord("-1", 0, AS_IS);
                }
                label = key->word;
                sprintf(originalkeyname, "%s", label);
            }
            else if (*label == '$') // indirect via user variable
            {
                char val[MAX_WORD_SIZE];
                strncpy(val, label, len);
                val[len] = 0;
                label = GetUserVariable(val, false, true); // key is a variable name or index, go get it to use real key
                if (IsDigit(*label)) key = FindWord(label);
                else if (!IsDigit(*label))  goto NULLVALUE;
                sprintf(originalkeyname, "%s", val);
            }
            else if ((*label == '_' && IsDigit(label[1])) || (*label == '\'' && label[1] == '_' && IsDigit(label[2]))) // indirection key match variable
            {
                int index = (*label == '_') ? atoi(label + 1) : atoi(label + 2);
                sprintf(originalkeyname, "_%d", index);
                if (index >= 0 && index <= MAX_WILDCARDS)
                {
                    if (separator[1] == '_') strcpy(label, wildcardCanonicalText[index]);
                    else strcpy(label, wildcardOriginalText[index]);
                    if (IsDigit(*label)) key = FindWord(label);
                }
                if (!IsDigit(*label))  goto NULLVALUE;
            }
            else  goto NULLVALUE; // not a number or indirect
        }
        if (showpath)
        {
            if (*separator == '.') strcat(path, ".");
            else strcat(path, "[");
            if (strcmp(originalkeyname, key->word))
            {
                strcat(path, originalkeyname);
                strcat(path, "=");
            }
            strcat(path, key->word);
            if (*separator != '.') strcat(path, "]");
        }
        if (factvalue) // $x.subject
        {
            if (separator[1] == 's' || separator[1] == 'S') answer = Meaning2Word(factvalue->subject)->word;
            else if (separator[1] == 'v' || separator[1] == 'V') answer = Meaning2Word(factvalue->verb)->word;
            else if (separator[1] == 'f' || separator[1] == 'F')
            {
                sprintf(flagsValue, "%u", factvalue->flags);
                answer = flagsValue;
            }
            else answer = Meaning2Word(factvalue->object)->word;
            separator = separator1;
            if (!separator) goto ANSWER;
            item = answer;
            goto LOOPDEEPER;
        }
        MEANING verb = MakeMeaning(key);
        int selected = atoi(label);

        FACT* F = GetSubjectNondeadHead(D);
        if (!strnicmp(D->word,"jo-",3) && bracket1 && *bracket1 == ']' && !bracket1[1]) // index into json object
        {
            int count = 0;
            while (F)
            {
                if (count++ == selected) break;
                if (selected == -1 && GetSubjectNondeadNext(F) == NULL) break; // first
                F = GetSubjectNondeadNext(F);
            }
            if (!F) goto NULLVALUE;
            answer = Meaning2Word(F->verb)->word;
            goto ANSWER;
        }

        while (F)
        {
            if (F->verb == verb || selected == -1 )// newest fact
            {
                answer = Meaning2Word(F->object)->word;
                if (!strcmp(answer, "null"))
                {
                    item = "``";
                    return item + 2; // null value for locals
                }
                // does it continue?
                if (separator1 && *separator1 == ']') ++separator1; // after current key/index there is another
                if (separator1 && *separator1) // after current key/index there is another
                {
                    item = answer;
                    separator = separator1;
                    goto LOOPDEEPER;
                }
                goto ANSWER;
            }
            F = GetSubjectNondeadNext(F);
        }
        goto NULLVALUE;
    }
    answer = item;
    // OLD NO LONGER VALID? if item is in fact & there are problems   return (*item == '&') ? (item + 1) : item; //   value is quoted or not

ANSWER:
    if (localvar)
    {
        char* limit;
        char* ans = InfiniteStack(limit, "GetUserVariable"); // has complete
        strcpy(ans, "``");
        strcpy(ans + 2, answer);
        CompleteBindStack();
        if (*path && showpath) Log(USERLOG, "`%s->%s`", path, ans);
        return ans + 2;
    }
    if (*path && showpath) Log(USERLOG, "`%s->%s`", path, answer);
    return answer;

NULLVALUE:
    return (localvar) ? (nullLocal + 2) : nullGlobal; // null value for locals
}

void ClearUserVariableSetFlags()
{
    HEAPREF varthread = userVariableThreadList;
    while (varthread)
    {
        uint64 D;
        varthread = UnpackHeapval(varthread, D, discard, discard);
        RemoveInternalFlag((WORDP)D, VAR_CHANGED);
    }
}

void ShowChangedVariables()
{
    HEAPREF varthread = userVariableThreadList;
    while (varthread)
    {
        uint64 Dx;
        varthread = UnpackHeapval(varthread, Dx, discard, discard);
        WORDP D = (WORDP)Dx;
        char* value = D->w.userValue;
        if (value && *value) Log(1, (char*)"%s = %s\r\n", D->word, value);
        else Log(1, (char*)"%s = null\r\n", D->word);
    }
}

void PrepareVariableChange(WORDP D, char* word, bool init)
{
    if (D->word[1] == '_') // tmp var
    {
        if (init) D->w.userValue = NULL;
    }
	else if (D->internalBits & BOTVAR) {}  // system vars have already been inited
    else if (!(D->internalBits & VAR_CHANGED))	// not changed already this volley
    {
		userVariableThreadList = AllocateHeapval(userVariableThreadList, (uint64)D, (uint64)0, (uint64)0);
		D->internalBits |= VAR_CHANGED; // bypasses even locked preexisting variables
 		if (init) D->w.userValue = NULL;
	}
    else if (memoryMarkThreadList)
    {
        // There has been a memory mark, track changes to existing variables
        uint64 memory;
        UnpackHeapval(memoryMarkThreadList, memory, discard);
        if ((uint64)D->w.userValue >= memory)
        {
            memoryVariableChangesThreadList = AllocateHeapval(memoryVariableChangesThreadList,
                (uint64)D, (uint64)D->w.userValue, (uint64)0);// save name
        }
    }
}

void SetVariable(WORDP D, char* value)
{
    // check for json assign
    if (strchr(D->word, '.') || strchr(D->word, '['))
    {
        JSONVariableAssign(D->word, value);
    }
    else // normal var
    {
        if (D->w.userValue == value) return;

        if (D->word[1] != '_' && D->word[1] != '$' && (!D->w.userValue || !value || strcmp(D->w.userValue, value))) // only permanent variables get tracked
        {
            variableChangedThreadlist = AllocateHeapval(variableChangedThreadlist,
                (uint64)D, (uint64)D->w.userValue, (uint64)D->internalBits);// save name
        }
        D->w.userValue = value;
        if (D->word[1] == '^') // function definition assignment
        {
            variableChangedThreadlist = MakeFunctionDefinition(value); // change internal bits AFTER save of value
        }
    }
}

void SetUserVariable(const char* var, char* word, bool assignment,bool reuse)
{
    char varname[MAX_WORD_SIZE];
    MakeLowerCopy(varname, (char*)var);
    WORDP D = StoreWord(varname);				// find or create the var.
    if (!D) return; // ran out of memory
#ifndef DISCARDTESTING
    if (debugVar) (*debugVar)(varname, word);
#endif

    // adjust value
    if (word) // has a nonnull value?
    {
        if (!*word || !stricmp(word, (char*)"null")) word = NULL; // really is null
        else //   some value 
        {
            bool purelocal = (D->word[1] == LOCALVAR_PREFIX);
            if (purelocal) word = AllocateStack(word, 0, true); // trying to save on permanent space
			else
			{
				if (D->w.userValue && !strcmp(word, D->w.userValue))
					return; // no change is happening in heap
				if (!reuse) word = AllocateHeap(word, 0, 1, false, purelocal); // we may be restoring old value which doesnt need allocation
			}
			if (!word) return;
        }
    }
	else if (!D->w.userValue)  return; // clear to null already

    PrepareVariableChange(D, word, !(csapicall == TEST_PATTERN || csapicall == TEST_OUTPUT));
    if (planning && !documentMode) // handle undoable assignment (cannot use text sharing as done in document mode)
    {
        if (D->w.userValue == NULL) SpecialFact(MakeMeaning(D), (MEANING)1, 0);
        else SpecialFact(MakeMeaning(D), (MEANING)(D->w.userValue - heapBase), 0);
    }
    if (csapicall == TEST_PATTERN || csapicall == TEST_OUTPUT)  SetVariable(D, word);
    else D->w.userValue = word;

	// monitored engine variables

	if (var[1] == 'c' && var[2] == 's' && var[3] == '_')
	{
		if (!stricmp(var, (char*)"$cs_json_array_defaults"))
		{
			int64 val = 0;
			if (word && *word) ReadInt64(word, val);
			jsonDefaults = (int)val;
		}
        // tokencontrol changes are noticed by the engine
        else if (!stricmp(var, (char*)"$cs_language"))
        {
            SetLanguage(word);
        }
		// tokencontrol changes are noticed by the engine
		else if (!stricmp(var, (char*)"$cs_token"))
		{
			int64 val = 0;
			if (word && *word) ReadInt64(word, val);
			else
			{
				val = (DO_INTERJECTION_SPLITTING | DO_SUBSTITUTE_SYSTEM | DO_NUMBER_MERGE | DO_PROPERNAME_MERGE | DO_SPELLCHECK);
				if (!stricmp(language, "english")) val |= DO_PARSE;
			}
			tokenControl = val;
		}

		// cs_float changes are noticed by the engine
		else if (!stricmp(var, (char*)"$cs_fullfloat"))
			fullfloat = (word && *word) ? true : false;
		
		// cs_numbers changes are noticed by the engine (india, french, other)
		else if (!stricmp(var, (char*)"$cs_numbers"))
		{
			if (!word) numberStyle = AMERICAN_NUMBERS;
			else if (!stricmp(word, "indian")) numberStyle = INDIAN_NUMBERS;
			else if (!stricmp(word, "french")) numberStyle = FRENCH_NUMBERS;
			else numberStyle = AMERICAN_NUMBERS;

			if (numberStyle == FRENCH_NUMBERS)
			{
				numberComma = '.';
				numberPeriod = ',';
			}
			else
			{
				numberComma = ',';
				numberPeriod = '.';
			}
		}
		// trace
		else if (!stricmp(var, (char*)"$cs_trace"))
		{
			int64 val = 0;
			if (word && *word) ReadInt64(word, val);
			trace = (unsigned int)val;
			if (assignment) // remember script changed it
			{
				modifiedTraceVal = trace;
				modifiedTrace = true;
			}
		}
		// time
		else if (!stricmp(var, (char*)"$cs_time"))
		{
			int64 val = 0;
			if (word && *word) ReadInt64(word, val);
			timing = (unsigned int)val;
			if (assignment) // remember script changed it
			{
				modifiedTimingVal = timing;
				modifiedTiming = true;
			}
		}
		// output random choice selection
		else if (!stricmp(var, (char*)"$cs_outputchoice"))
		{
			int64 val = -1;
			if (word && *word) ReadInt64(word, val);
			outputchoice = (unsigned int)val;
		}
		// responsecontrol changes are noticed by the engine
		else if (!stricmp(var, (char*)"$cs_response"))
		{
			int64 val = 0;
			if (word && *word) ReadInt64(word, val);
			else val = ALL_RESPONSES;
			responseControl = (unsigned int)val;
		}
		// cs_botid changes are noticed by the engine
		else if (!stricmp(var, (char*)"$cs_botid"))
		{
			int64 val = 0;
			if (word && *word) ReadInt64(word, val);
			myBot = (uint64)val;
		}
		// wildcardseparator changes are noticed by the engine
		else if (!stricmp(var, (char*)"$cs_wildcardSeparator"))
		{
			wildcardSeparatorGiven = true;
			if (!word) *wildcardSeparator = 0;
			else if (*word == '\\') *wildcardSeparator = word[2];
			else *wildcardSeparator = (*word == '"') ? word[1] : *word; // 1st char in string if need be
		}
        // random index
        else if (!stricmp(var, (char*)"$cs_randIndex"))
        {
            int64 val = 0;
            if (word && *word) ReadInt64(word, val);
            randIndex = (unsigned int)val;
        }
	}

    if (trace && D->internalBits & MACRO_TRACE)
    {
        char pattern[110];
        char label[MAX_LABEL_SIZE];
        GetPattern(currentRule, label, pattern, true,100);  // go to output
        Log(ECHOUSERLOG, "%s -> %s at %s.%d.%d %s %s\r\n", D->word, word, GetTopicName(currentTopicID), TOPLEVELID(currentRuleID), REJOINDERID(currentRuleID), label, pattern);
    }
}


static FunctionResult DoMath(char* oldValue, char* moreValue, char* result, char op, char* fullop)
{
    if (*oldValue == '_') oldValue = GetwildcardText(GetWildcardID(oldValue), true); // onto a wildcard
    else if (*oldValue == USERVAR_PREFIX) oldValue = GetUserVariable(oldValue,false,true); // onto user variable
    else if (*oldValue == '^') oldValue = FNVAR(oldValue + 1); // onto function argument
    else if (*oldValue && !IsNumberStarter(*oldValue)) return FAILRULE_BIT; // illegal

    if (*moreValue == '_') moreValue = GetwildcardText(GetWildcardID(moreValue), true);
    else if (*moreValue == USERVAR_PREFIX) moreValue = GetUserVariable(moreValue, false, true);
    else if (*moreValue == '^') moreValue = FNVAR(moreValue + 1);
    else if (*oldValue && !IsNumberStarter(*moreValue)) return FAILRULE_BIT; // illegal

                                                        // perform numeric op
    bool floating = false;
    if (strchr(oldValue, '.') || strchr(moreValue, '.') || op == '/') floating = true;

    if (floating)
    {
        double newval = Convert2Double(oldValue);
        double more = Convert2Double(moreValue);
        if (op == '-') newval -= more;
        else if (op == '*') newval *= more;
        else if (op == '/') {
            if (more == 0) return FAILRULE_BIT; // cannot divide by 0
            newval /= more;
        }
        else if (op == '%')
        {
            if (more == 0) return FAILRULE_BIT;
            int64 ivalue = (int64)newval;
            int64 morval = (int64)more;
            newval = (double)(ivalue % morval);
        }
        else newval += more;
        WriteFloat(result, newval);
        if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG," %s   ", result);
    }
    else
    {
        int64 newval;
        ReadInt64(oldValue, newval);
        int64 more;
        ReadInt64(moreValue, more);
        if (op == '-') newval -= more;
        else if (op == '*') newval *= more;
        else if (op == '/')
        {
            if (more == 0)  return  FAILRULE_BIT; // cannot divide by 0
            newval /= more;
        }
        else if (op == '%')
        {
            if (more == 0) return  FAILRULE_BIT;
            newval %= more;
        }
        else if (op == '|')
        {
            newval |= more;
            if (fullop[1] == '^') newval ^= more;
        }
        else if (op == '&') newval &= more;
        else if (op == '^') newval ^= more;
        else if (op == '<') newval <<= more;
        else if (op == '>') newval >>= more;
        else newval += more;
        char tracex[MAX_WORD_SIZE];
#ifdef WIN32
        sprintf(result, (char*)"%I64d", newval);
#else
        sprintf(result, (char*)"%lld", newval);
#endif        
        if (trace & TRACE_OUTPUT && CheckTopicTrace())
        {
            sprintf(tracex, "0x%016llx", newval);
            Log(USERLOG," %s/%s   ", result, tracex);
        }
    }
    return NOPROBLEM_BIT;
}

FunctionResult Add2UserVariable(char* var, char* moreValue, char* op, char* originalArg)
{
    // get original value
    char  minusflag = *op;
    char* oldValue;
    if (*var == '_') oldValue = GetwildcardText(GetWildcardID(var), true); // onto a wildcard
    else if (*var == USERVAR_PREFIX) oldValue = GetUserVariable(var, false, true); // onto user variable
    else if (*var == '^') oldValue = FNVAR(var + 1); // onto function argument
    else if (*var == SYSVAR_PREFIX) oldValue =  SystemVariable(var, NULL); 
    else return FAILRULE_BIT; // illegal

                              // get augment value
    if (*moreValue == '_') moreValue = GetwildcardText(GetWildcardID(moreValue), true);
    else if (*moreValue == USERVAR_PREFIX) moreValue = GetUserVariable(moreValue, false, true);
    else if (*moreValue == '^') moreValue = FNVAR(moreValue + 1);

    // try json array set op?
    if (IsValidJSONName(oldValue, 'a'))
    {
        if (*op != '+') return FAILRULE_BIT;
        FunctionResult result = NOPROBLEM_BIT;
        WORDP old = FindWord(oldValue);
        char junk[10];
        if (IsValidJSONName(moreValue, 'a'))
        {
            char* limit;
            FACT* F = GetSubjectNondeadHead(FindWord(moreValue));
            FACT** stack = (FACT**)InfiniteStack(limit, "array merge");
            int index = 0;
            while (F) // stack object key data
            {
                if (F->flags & JSON_ARRAY_FACT)  stack[index++] = F;
                F = GetSubjectNondeadNext(F);
            }
            CompleteBindStack64(index, (char*)stack);
            for (int i = index - 1; i >= 0; --i)
            {
                F = stack[i];
                unsigned int flags = JSON_ARRAY_FACT | (F->flags & JSON_OBJECT_FLAGS);
                if (oldValue[3] == 't') flags |= FACTTRANSIENT;
                MEANING value = F->object;
                result = DoJSONArrayInsert(true, old, value, flags, junk);
                if (result == FAILRULE_BIT) break;
            }
            ReleaseStack((char*)stack);

        }
        else
        {
            unsigned int flags = JSON_ARRAY_FACT;
            if (oldValue[3] == 't') flags |= FACTTRANSIENT;
            MEANING value = jsonValue(moreValue, flags);
            result = DoJSONArrayInsert(true, old, value, flags, junk);
        }
        return result;
    }

    char result[MAX_WORD_SIZE];
    FunctionResult fnresult = DoMath(oldValue, moreValue, result, minusflag, op);
    if (fnresult != NOPROBLEM_BIT) return fnresult;

    // store result back
    if (*var == '_')
        SetWildCard(result, result, var, 0);
    else if (*var == USERVAR_PREFIX)
    {
        char* dot = strchr(var, '.');
        if (!dot) SetUserVariable(var, result, true);
        else
        {
            if (csapicall == TEST_PATTERN || csapicall == TEST_OUTPUT)
            {
                *dot = 0;
                WORDP D = StoreWord(var);
                *dot = '.';
                SetVariable(D, D->w.userValue); // force change of internal content detect
            }
            JSONVariableAssign(var, result);// json object insert
        }
    }
    else if (*var == '^') strcpy(FNVAR(var + 1), result);
    else if (*var == SYSVAR_PREFIX)  SystemVariable(var, result);
    
    return NOPROBLEM_BIT;
}

static void SetBotVars(HEAPREF varthread)
{
    while (varthread)
    {
        uint64 D;
        uint64 value;
        varthread = UnpackHeapval(varthread, D, value, discard);
        if (!releaseBotVar || stricmp(releaseBotVar, ((WORDP)D)->word))
            ((WORDP)D)->w.userValue = (char*) value;
        else releaseBotVar = NULL; // we have allowed it to not restore
    }
}

void ReestablishBotVariables() // refresh bot variables in case user overwrote them
{
    SetBotVars(botVariableThreadList);
    SetBotVars(kernelVariableThreadList);
}

void NoteBotVariables() // system defined variables
{
    while (userVariableThreadList)
    {
        uint64 Dx;
        userVariableThreadList = UnpackHeapval(userVariableThreadList, Dx, discard, discard);
        WORDP D = (WORDP)Dx;
        if (D->word[1] != LOCALVAR_PREFIX && D->word[1] != TRANSIENTVAR_PREFIX) // not a transient var
        {
            if (!strnicmp(D->word, "$cs_", 4)) continue; // dont force these, they are for user
			D->internalBits |= (unsigned int)BOTVAR; // mark it
            botVariableThreadList = AllocateHeapval(botVariableThreadList, (uint64)D, (uint64)D->w.userValue, (uint64)0);
        }
        RemoveInternalFlag(D, VAR_CHANGED);
		if (!stricmp(D->word, "$verb")) 
			verbwordx = atoi(D->w.userValue);
    }
}

void MigrateUserVariables()
{
    while (userVariableThreadList)
    {
        uint64 Dx;
        userVariableThreadList = UnpackHeapval(userVariableThreadList, Dx, discard, discard);
        WORDP D = (WORDP)Dx;
        D->w.userValue = AllocateStack(D->w.userValue, 0);
        D->word = AllocateStack(D->word, 0); // reallocate name
        stackedUserVariableThreadList = AllocateStackval(stackedUserVariableThreadList, (uint64)D);
    }
}

void RecoverUserVariables()
{// transfer stack variables to heap variables
    while (stackedUserVariableThreadList)
    {
        uint64 Dx;
        stackedUserVariableThreadList = UnpackStackval(stackedUserVariableThreadList, Dx);
        WORDP D = (WORDP)Dx;
        D->w.userValue = AllocateHeap(D->w.userValue, 0);
        D->word = AllocateHeap(D->word, 0);
        userVariableThreadList = AllocateHeapval(userVariableThreadList,(uint64)D, (uint64)0, (uint64)0);
    }
}

void ClearUserVariables(char* above)
{
    HEAPREF varthread = userVariableThreadList;
    unsigned int* prevcell = 0;
    while (varthread) // variable may be below mark but has value above
    {
        uint64 Dx;
        varthread = UnpackHeapval(varthread, Dx,discard);
        WORDP D = (WORDP)Dx;
		RemoveInternalFlag(D, MACRO_TRACE);

		if (!above) // removing ALL variables
        {
			D->w.userValue = NULL;
            RemoveInternalFlag(D, VAR_CHANGED);
        }
        else  if (D->w.userValue && D->w.userValue < above) // heap is down, so this is recent and must be lost.
        {
            D->w.userValue = NULL; // lose value in new area
        }
    }
    if (!above) userVariableThreadList = 0;
}

// This is the comparison function for the sort operation.
static int compareVariables(const void *var1, const void *var2)
{
    return strcmp((*(WORDP *)var1)->word, (*(WORDP *)var2)->word);
}

static void ListVariables(char* header, HEAPREF varthread)
{
    char* value;
    while (varthread)
    {
        uint64 Dx;
        varthread = UnpackHeapval(varthread, Dx, discard, discard);
        WORDP D = (WORDP)Dx;
        value = D->w.userValue;
        if (value && *value)  Log(USERLOG,"  %s variable: %s = %s\r\n", header, D->word, value);
    }
}

void DumpUserVariables(bool doall)
{
    char* value;
    ListVariables("preboot", kernelVariableThreadList);
    ListVariables("boot", botVariableThreadList);
    if (!doall) return;

    // count entries
    int counter = 0;
    HEAPREF varthread = userVariableThreadList;
    while (varthread)
    {
        varthread = UnpackHeapval(varthread, discard, discard, discard);
        ++counter;
    }

    // Show the user variables in alphabetically sorted order.
    WORDP *arySortVariablesHelper = (WORDP*)AllocateStack(NULL, ((counter) ? counter : 1) * sizeof(WORDP));

    // Load the array.
    varthread = userVariableThreadList;
    int i = 0;
    while (varthread)
    {
        uint64 D;
        varthread = UnpackHeapval(varthread, D, discard, discard);
        arySortVariablesHelper[i++] = (WORDP)D;
    }

    // Sort it.
    qsort(arySortVariablesHelper, counter, sizeof(WORDP), compareVariables);

    // Display the variables in sorted order.
    for (i = 0; i < counter; ++i)
    {
        WORDP D = arySortVariablesHelper[i];
        value = D->w.userValue;
        if (value && *value)
        {
            if (!stricmp(D->word, "$cs_token"))
            {
                Log(USERLOG, "  variable: $cs_token decoded = ");
                int64 val;
                ReadInt64(value, val);
                DumpTokenControls(val);
                Log(USERLOG, "\r\n");
            }
            else Log(USERLOG, "  variable: %s = %s\r\n", D->word, value);
        }
    }
    ReleaseStack((char*)arySortVariablesHelper); // short term
}

char* ProcessMath(char* ptr, char* oldValue, FunctionResult result) // paren expression returns answer on oldValue
{
    char word[MAX_WORD_SIZE];
    char op[MAX_WORD_SIZE];
    int level = 0;
    char moreValue[MAX_WORD_SIZE];
    char answer[MAX_WORD_SIZE];
    *op = 0;
    *oldValue = 0;
    *moreValue = 0;
    while (*ptr)
    {
        ptr = ReadCompiledWord(ptr, word); // will be (, ), op, value
        if (*word == '(')
        {
            ++level;
            continue;
        }
        else if (*word == ')')
        {
            --level;
            if (level == 0) return ptr; // done
            continue;
        }
        if (IsArithmeticOp(word)) strcpy(op, word);
        else
        {
            strcpy(oldValue, word); // must have been value
            ptr = ReadCompiledWord(ptr, op);
        }
        if (*ptr == '(') // nest
        {
            ptr = ProcessMath(ptr, moreValue, result);
            strcpy(answer, moreValue);
        }
        else  ptr = ReadCompiledWord(ptr, moreValue);

        FunctionResult fnresult = DoMath(oldValue, moreValue, answer, *op, op);  //  (x + y ) or (x + y + z)
        *op = 0;
        strcpy(oldValue, answer);
    }

    return ptr;
}

char* PerformAssignment(char* word, char* ptr, char* buffer, FunctionResult &result, bool nojson)
{// assign to and from  $var, _var, ^var, @set, and %sysvar
    char op[MAX_WORD_SIZE];
    currentFact = NULL;					// No assignment can start with a fact lying around
    int oldImpliedSet = impliedSet;		// in case nested calls happen
    int oldImpliedWild = impliedWild;	// in case nested calls happen
    int assignFromWild = ALREADY_HANDLED;
    result = NOPROBLEM_BIT;

    if (*word == '^' && word[1] == '^' && IsDigit(word[2])) // indirect function variable assign
    {
        char* value = FNVAR(word + 2);
        if (*value == LCLVARDATA_PREFIX && value[1] == LCLVARDATA_PREFIX)//  not allowed to write indirect to caller arg
        {
            result = FAILRULE_BIT;
            return ptr;
        }
        strcpy(word, value); // change over to indirect to assign onto
        strcpy(word, GetUserVariable(word)); // now indirect thru him if we can
        if (!*word)
        {
            result = FAILRULE_BIT;
            return ptr;
        }
    }
    else if (*word == '^' && IsDigit(word[1])) // indirect function variable assign
    {
        char* value = FNVAR(word + 1);
        if (*value == LCLVARDATA_PREFIX && value[1] == LCLVARDATA_PREFIX)//  not allowed to write indirect to caller arg
        {
            result = FAILRULE_BIT;
            return ptr;
        }
        strcpy(word, value); // change over to assign onto caller var
    }

    impliedSet = ALREADY_HANDLED;
    impliedWild = ALREADY_HANDLED;
    if (*word == '@')
    {
        impliedSet = GetSetID(word);
        if (impliedSet == ILLEGAL_FACTSET)
        {
            result = FAILRULE_BIT;
            return ptr;
        }
        char* at = word + 1;
        while (*++at && IsDigit(*at)) { ; } // find end
        if (*at) // not allowed to assign onto annotated factset
        {
            result = FAILRULE_BIT;
            return ptr;
        }
    }
    else if (*word == '_')
    {
        impliedWild = GetWildcardID(word);	// if a wildcard save location
        if (impliedWild == ILLEGAL_MATCHVARIABLE)
        {
            result = FAILRULE_BIT;
            return ptr;
        }
    }
    int setToImply = impliedSet; // what he originally requested
    int setToWild = impliedWild; // what he originally requested
    bool otherassign = (*word != '@') && (*word != '_');

    // Get assignment operator
    ptr = ReadCompiledWord(ptr, op); // assignment operator = += -= /= *= %= ^= |= &=
    impliedOp = *op;
    if (*op == '=' && impliedSet >= 0) SET_FACTSET_COUNT(impliedSet, 0); // force to be empty for @0 = ^first(...)
    char originalWord1[MAX_WORD_SIZE];
    ReadCompiledWord(ptr, originalWord1);
    
    assignFromWild = (*ptr == '_' && IsDigit(ptr[1])) ? GetWildcardID(ptr) : -1;
    
    // get the from value when assigning to _ var from a function var wildcard
    bool wildwild = false;
    if (*word == '_' && *originalWord1 == '^' && IsDigit(originalWord1[1])) // function argument
    {
        char* value = FNVAR(originalWord1 + 1);
        if (*value == '_'  && IsDigit(value[1]))
        {
            assignFromWild = GetWildcardID(value);
            ptr = ReadCompiledWord(ptr, buffer); // assigning from wild to wild. Just copy across
            strcpy(buffer, value);
            wildwild = true;
            if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"%s => %s ", word, value );
        }
    }
    if (*word == '_' && *ptr == '\'' && ptr[1] == '_' && IsDigit(ptr[2]))
    {
        assignFromWild = GetWildcardID(ptr + 1); // allow quoted assign across
        ptr = ReadCompiledWord(ptr + 1, buffer);
    }
    else if (assignFromWild >= 0 && *word == '_')
    {
        if (!wildwild) ptr = ReadCompiledWord(ptr, buffer); // assigning from wild to wild. Just copy across
    }
    else if (*ptr == '(')
    {
        ptr = ProcessMath(ptr, buffer, result);
    }
    else if (*word == '$' && word[1] == '^') // function definition assignment
    {
        strcpy(buffer, ptr+2);
        ptr = "";
    }
    else
    {
        ptr = GetCommandArg(ptr, buffer, result, OUTPUT_NOCOMMANUMBER | ASSIGNMENT); // need to see null assigned -- store raw numbers, not with commas, lest relations break
        if (*buffer == '#' && !strchr(buffer,' ')) // substitute a constant? user type-in :set command for example
        {
            uint64 n = FindPropertyValueByName(buffer + 1);
            if (!n) n = FindSystemValueByName(buffer + 1);
            if (n)
            {
#ifdef WIN32
                sprintf(buffer, (char*)"%I64d", (long long int) n);
#else
                sprintf(buffer, (char*)"%lld", (long long int) n);
#endif	
            }
        }
        if (result & ENDCODES) goto exit;
        // impliedwild will be used up when spreading a fact by assigned. impliedset will get used up if assigning to a factset.
        // assigning to a var is simple
        // A fact was created but not used up by retrieving some field of it. Convert to a reference to fact.
        // if we already did a conversion into a set or wildcard, dont do it here. Otherwise do it now into those or $uservars.
        // DO NOT CHANGE TO add: if settowild != IMPLIED_WILD. that introduces a bug assigning to $vars.

        // if he original requested to assign to and the assignment has been done, then we dont need to do anything
        if ((setToImply != impliedSet && setToImply != ALREADY_HANDLED) || (setToWild != impliedWild  && setToWild != ALREADY_HANDLED)) currentFact = NULL; // used up
                                                                                                                                                            // A fact was created but not used up by retrieving some field of it. Convert to a reference to fact. -- eg $$f = createfact()
        else if (currentFact && setToImply == impliedSet && setToWild == impliedWild && (setToWild != ALREADY_HANDLED || setToImply != ALREADY_HANDLED || otherassign)) sprintf(buffer, (char*)"%u", currentFactIndex());
    }
    // normally null is empty string, but for assignment allow the word, which means null as in removal
    if (!stricmp(buffer, (char*)"null") && (*word != USERVAR_PREFIX || !strchr(word, '.'))) *buffer = 0;

    //   now sort out who we are assigning into and if its arithmetic or simple assign
    if (*word == '@')
    {
        if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"%s(%s) %s %s => ", word, GetUserVariable(word, false, true), op, originalWord1);
        if (impliedSet == ALREADY_HANDLED) { ; }
        else if (!*buffer && *op == '=') // null assign to set as a whole
        {
            if (*originalWord1 == '^') { ; } // presume its a factset function and it has done its job - @0 = next(fact @1fact)
            else SET_FACTSET_COUNT(impliedSet, 0);
        }
        else if (*buffer == '@') // set to set operator
        {
            FACT* F;
            int rightSet = GetSetID(buffer);
            if (rightSet == ILLEGAL_FACTSET)
            {
                result = FAILRULE_BIT;
                impliedOp = 0;
                goto exit;
            }
            unsigned int rightCount = FACTSET_COUNT(rightSet);
            unsigned int impliedCount = FACTSET_COUNT(impliedSet);

            if (*op == '+') while (rightCount) AddFact(impliedSet, factSet[rightSet][rightCount--]); // add set to set preserving order
            else if (*op == '-') // remove from set
            {
                for (unsigned int i = 1; i <= rightCount; ++i) factSet[rightSet][i]->flags |= MARKED_FACT; // mark right facts
                for (unsigned int i = 1; i <= impliedCount; ++i) // erase from the left side
                {
                    F = factSet[impliedSet][i];
                    if (F->flags & MARKED_FACT)
                    {
                        memmove(&factSet[impliedSet][i], &factSet[impliedSet][i + 1], (impliedCount - i) * sizeof(FACT*));
                        --impliedCount; // new end count
                        --i; // redo loop at this point
                    }
                }
                for (unsigned int i = 1; i <= rightCount; ++i) factSet[rightSet][i]->flags ^= MARKED_FACT; // erase marks
                SET_FACTSET_COUNT(impliedSet, impliedCount);
            }
            else if (*op == '=') memmove(&factSet[impliedSet][0], &factSet[rightSet][0], (rightCount + 1) * sizeof(FACT*)); // assigned from set
            else result = FAILRULE_BIT;
        }
        else if (IsDigit(*buffer) || !*buffer) // fact index (or null fact) to set operators 
        {
            int index;
            ReadInt(buffer, index);
            FACT* F = Index2Fact(index);
            unsigned int impliedCount = FACTSET_COUNT(impliedSet);
            if (*op == '+' && op[1] == '=' && !stricmp(originalWord1, (char*)"^query")) { ; } // add to set done directly @2 += ^query()
            else if (*op == '+' && F) AddFact(impliedSet, F); // add to set
            else if (*op == '-' ) // remove from set (can even remove null facts in a set)
            {
                if (F) F->flags |= MARKED_FACT;
                for (unsigned int i = 1; i <= impliedCount; ++i) // erase from the left side
                {
                    FACT* G = factSet[impliedSet][i];
                    if ((G && G->flags & MARKED_FACT) || (!G && !F))
                    {
                        memmove(&factSet[impliedSet][i], &factSet[impliedSet][i + 1], (impliedCount - i) * sizeof(FACT*));
                        --impliedCount;
                        --i;
                    }
                }
                SET_FACTSET_COUNT(impliedSet, impliedCount);
                if (F) F->flags ^= MARKED_FACT;
            }
            else if (*op == '=' && F) // assign to set (cant do this with null because it erases the set)
            {
                SET_FACTSET_COUNT(impliedSet, 0);
                AddFact(impliedSet, F);
            }
            else result = FAILRULE_BIT;
        }
        if (impliedSet != ALREADY_HANDLED)
        {
            factSetNext[impliedSet] = 0; // all changes requires a reset of next ptr
            impliedSet = ALREADY_HANDLED;
        }
    }
    else if (IsArithmeticOperator(op))
    {
        if (trace & TRACE_OUTPUT && CheckTopicTrace())
        {
            if (*op == '=') Log(USERLOG,"%s %s %s(%s) => ", word, op, originalWord1, GetUserVariable(originalWord1, false, true));
            else Log(USERLOG,"%s(%s) %s %s(%s) => ", word, GetUserVariable(word, false, true), op, originalWord1, GetUserVariable(originalWord1, false, true));
        }
        if (*word == '^') result = FAILRULE_BIT;	// not allowed to increment locally at present OR json array ref...
        else if (*buffer) result = Add2UserVariable(word, buffer, op, originalWord1);
    }
    else if (*word == '_') //   assign to wild card
    {
        if (impliedWild != ALREADY_HANDLED) // no one has actually done the assignnment yet
        {
            if (assignFromWild >= 0 && assignFromWild <= MAX_WILDCARDS) // full tranfer of data
            {
                SetWildCard(wildcardOriginalText[assignFromWild], wildcardCanonicalText[assignFromWild], word, 0);
                wildcardPosition[GetWildcardID(word)] = wildcardPosition[assignFromWild];
                strcpy(buffer, wildcardOriginalText[assignFromWild]); // for tracing
            }
            else SetWildCard(buffer, buffer, word, 0);
        }
        if (trace & TRACE_OUTPUT && CheckTopicTrace())
        {
            if (!*buffer && !stricmp(originalWord1,"null")) Log(USERLOG, "%s=%s", word, originalWord1);
            else Log(USERLOG, "%s=%s`%s`", word, originalWord1, buffer);
        }
    }
    else if (*word == USERVAR_PREFIX)
    {
        if (trace & TRACE_OUTPUT && CheckTopicTrace())
        {
            if (!*buffer && !stricmp(originalWord1, "null")) Log(USERLOG, "%s=%s ", word, originalWord1);
            else if (!stricmp(originalWord1,buffer)) Log(USERLOG, "%s = %s ", word, originalWord1);
            else if (*originalWord1 == '^') Log(USERLOG, "%s = %s(...) `%s` ", word, originalWord1, buffer);
            else Log(USERLOG, "%s = %s `%s` ", word, originalWord1, buffer);
        }
        // assume that if a variable explicitly has quotes that they are important and therefore don't need to strip them 
        // BUG that we dont do that for _n vars but not likely
        bool stripQuotes = (*originalWord1 == USERVAR_PREFIX) ? false : true; // literals get quotes stripped, variables already did that
        char* dot = strchr(word, '.');
        if (!dot)
        {
            dot = strchr(word, '['); // array assign?
            if (dot) dot = strchr(dot, ']');
        }
        if (!dot || nojson) SetUserVariable(word, buffer, true);
        else result = JSONVariableAssign(word, buffer, stripQuotes);// json object insert
    }
    else if (*word == '\'' && word[1] == USERVAR_PREFIX)
    {
        if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"%s = %s `%s`\r\n", word, originalWord1, buffer);
        SetUserVariable(word + 1, buffer, true); // '$xx = value  -- like passed thru as argument
    }
    else if (*word == SYSVAR_PREFIX && !strchr(buffer, ' '))
    {
        if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"%s = %s `%s`\r\n", word, originalWord1, buffer);
        SystemVariable(word, buffer); // assignment onto system var
    }
    else if (*word == '^') // overwrite function arg
    {
        if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"%s = %s\r\n", word, buffer);
        FNVAR(word + 1) = AllocateStack(buffer);
    }
    else // cannot touch a  word, or number
    {
        if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"illegal assignment to %s\r\n", word);
        result = FAILRULE_BIT;
        goto exit;
    }

    //  followup arithmetic operators?
    while (ptr && IsArithmeticOperator(ptr))
    {
        ptr = ReadCompiledWord(ptr, op);
        ReadCompiledWord(ptr, originalWord1);
        ptr = GetCommandArg(ptr, buffer, result, ASSIGNMENT);
        if (!stricmp(buffer, word))
        {
            result = FAILRULE_BIT;
            Log(USERLOG,"variable assign %s has itself as a term\r\n", word);
        }
        if (result & ENDCODES) goto exit; // failed next value
        if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG,"    %s(%s) %s %s(%s) =>", word, GetUserVariable(word, false, true), op, originalWord1, buffer);
        if (*buffer) result = Add2UserVariable(word, buffer, op, originalWord1);
    }

    // debug
    if (trace & TRACE_OUTPUT && CheckTopicTrace())
    {
        logUpdated = false;
        if (*word == '@')
        {
            int set = GetSetID(word);
            int count = FACTSET_COUNT(set);
            FACT* F = factSet[set][count];
            unsigned int id = Fact2Index(F);
            char* fact = AllocateBuffer();
            WriteFact(F, false, fact, false, false,true);
            Log(USERLOG,"last value @%d[%d] is %d %s", set, count, id, fact); // show last item in set
            FreeBuffer();
        }
    }

exit:
    currentFact = NULL; // any assignment uses up current fact by definition
    *buffer = 0;
    impliedSet = oldImpliedSet;
    impliedWild = oldImpliedWild;
    impliedOp = 0;
    if (trace & TRACE_OUTPUT && CheckTopicTrace()) Log(USERLOG, "\r\n");

    return ptr;
}
