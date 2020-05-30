#include "common.h"

static MEANING lengthLists[100];		// lists of valid words by length
bool fixedSpell = false;
bool spellTrace = false;
char spellCheckWord[MAX_WORD_SIZE];
int badspellcount = 0;

typedef struct SUFFIX
{
    char* word;
	uint64 flags;
} SUFFIX;


static SUFFIX stems[] = 
{
	{ (char*)"less",NOUN},
	{ (char*)"ness",ADJECTIVE|NOUN},
	{ (char*)"est",ADJECTIVE},
    { (char*)"er",ADJECTIVE},
	{ (char*)"ly",ADJECTIVE},
	{0},
};

static SUFFIX stems_french[] = 
{
	{ (char*)"âtre",ADJECTIVE},
	{ (char*)"able",ADJECTIVE},
	{ (char*)"ade",NOUN},
	{ (char*)"age",NOUN},
	{ (char*)"aille",NOUN},
	{ (char*)"ain",NOUN|ADJECTIVE},
	{ (char*)"ais",NOUN|ADJECTIVE},
	{ (char*)"al",ADJECTIVE},
	{ (char*)"ance",NOUN},
	{ (char*)"ant",ADJECTIVE},
	{ (char*)"ard",ADJECTIVE},
	{ (char*)"aud",ADJECTIVE},
	{ (char*)"ère",NOUN},
	{ (char*)"ée",NOUN},
	{ (char*)"el",ADJECTIVE},
	{ (char*)"et",ADJECTIVE},
	{ (char*)"esse",NOUN},
	{ (char*)"eur",ADJECTIVE|NOUN},
	{ (char*)"euse",NOUN},
	{ (char*)"eux",ADJECTIVE},
	{ (char*)"ible",ADJECTIVE},
	{ (char*)"isme",NOUN},
	{ (char*)"iste",NOUN|ADJECTIVE},
	{ (char*)"ien",NOUN|ADJECTIVE},
	{ (char*)"ier",NOUN},
	{ (char*)"ie",NOUN},
	{ (char*)"if",ADJECTIVE},
	{ (char*)"in",ADJECTIVE},
	{ (char*)"ir",VERB},
	{ (char*)"asser",VERB},
	{ (char*)"ater",VERB},
	{ (char*)"ailler",VERB},
	{ (char*)"ifier",VERB},
	{ (char*)"iner",VERB},
	{ (char*)"iser",VERB},
	{ (char*)"oter",VERB},
	{ (char*)"ot",ADJECTIVE},
	{ (char*)"oyer",VERB},
	{ (char*)"er",VERB|NOUN},
	{ (char*)"ment",ADVERB|NOUN},
	{ (char*)"ois",NOUN},
	{ (char*)"son",NOUN},
	{ (char*)"tion",NOUN},
	{ (char*)"ure",NOUN},
	{ (char*)"logue",NOUN},
	{ (char*)"logie",NOUN},
	{ (char*)"gène",NOUN},
	{ (char*)"gramme",NOUN},
	{ (char*)"manie",NOUN},
	{ (char*)"phobe",NOUN},
	{ (char*)"phobie",NOUN},
	{ (char*)"ose",NOUN},
	{0},
};

bool multichoice = false;

void InitSpellCheck()
{
	memset(lengthLists,0,sizeof(MEANING) * 100);
	WORDP D = dictionaryBase;
	while (++D <= dictionaryFree)
	{
		if (!D->word || !IsAlphaUTF8(*D->word) || D->length >= 100 || strchr(D->word,'~') || strchr(D->word,USERVAR_PREFIX) || strchr(D->word,'^') || strchr(D->word,' ')  || strchr(D->word,'_')) continue; 
		if (D->properties & PART_OF_SPEECH || D->systemFlags & PATTERN_WORD)
		{
            WORDINFO wordData;
            ComputeWordData(D->word, &wordData);
			D->spellNode = lengthLists[wordData.charlen];
			lengthLists[wordData.charlen] = MakeMeaning(D);
		}
	}
}

static bool SameUTF(char* word, char* utfstring)
{
	size_t len = strlen(utfstring);
	return (!strncmp(word, utfstring, len));
}

static int SplitWord(char* word,int i)
{
	size_t len1 = strlen(word);
	// Do not split acronyms (all uppercase) unless word before or after is lowercase
    size_t j;
	for (j = 0; j < len1; ++j)
	{
		if (!IsUpperCase(word[j])) break;
	}
	if (j == len1) // looks like acronym but is any neighbor non caps
	{
		if (wordStarts[i - 1] && IsUpperCase(wordStarts[i - 1][0])) j = 0;
		if (wordStarts[i + 1] && IsUpperCase(wordStarts[i + 1][0])) j = 0;
	}
	if (j == len1) return 0;

	WORDP D2;
	bool good;
	int breakAt = 0;
	if (IsDigit(*word))
    {
		while (IsDigit(word[++breakAt]) || word[breakAt] == '.' || word[breakAt] == ','){;} //   find end of number
        if (word[breakAt]) // found end of number
		{
			D2 = FindWord(word+breakAt,0,PRIMARY_CASE_ALLOWED);
			if (D2)
			{
				good = (D2->properties & (PART_OF_SPEECH|FOREIGN_WORD)) != 0 || (D2->internalBits & HAS_SUBSTITUTE) != 0; 
				if (good && (D2->systemFlags & AGE_LEARNED))// must be common words we find
				{
					char number[MAX_WORD_SIZE];
					strncpy(number,word,breakAt);
					number[breakAt] = 0;
					StoreWord(number,ADJECTIVE|NOUN|ADJECTIVE_NUMBER|NOUN_NUMBER); 
					return breakAt; // split here
				}
			}
		}
    }

	// dont split acronyms

	//  try all combinations of breaking the word into two known words
    bool isEnglish = (!stricmp(language, "english") ? true : false);
    bool isFrench = (!stricmp(language, "french") ? true : false);
	breakAt = 0;
	size_t len = strlen(word);
    for (unsigned int k = 1; k < len-1; ++k)
    {
        if (isEnglish && k == 1 && *word != 'a' && *word != 'A' && *word != 'i' && *word != 'I') continue; //   only a and i are allowed single-letter words
        else if (isFrench && k == 1 && *word != 'y' && *word != 'a' && *word != 'A' && !SameUTF(word,"à") && !SameUTF(word, "À") && !SameUTF(word, "ô") && !SameUTF(word,"Ô")) continue; //   in french only y, a and ô are allowed single-letter words
		WORDP D1 = FindWord(word,k,PRIMARY_CASE_ALLOWED);
        if (!D1) continue;
		good = (D1->properties & (PART_OF_SPEECH|FOREIGN_WORD)) != 0 || (D1->internalBits & HAS_SUBSTITUTE) != 0; 
		if (!good || !(D1->systemFlags & AGE_LEARNED)) continue; // must be normal common words we find

        D2 = FindWord(word+k,len-k,PRIMARY_CASE_ALLOWED);
        if (!D2) continue;
        good = (D2->properties & (PART_OF_SPEECH|FOREIGN_WORD)) != 0 || (D2->internalBits & HAS_SUBSTITUTE) != 0;
		if (!good || !(D2->systemFlags & AGE_LEARNED) ) continue; // must be normal common words we find

        if (!breakAt) breakAt = k; // found a split
		else // found multiple places to split... dont know what to do
        {
           breakAt = -1; 
           break;
		}
    }
	return breakAt;
}

static char* SpellCheck( int i)
{
    char* tokens[6];
    //   on entry we will have passed over words which are KnownWord (including bases) or isInitialWord (all initials)
    //   wordstarts from 1 ... wordCount is the incoming sentence words (original). We are processing the ith word here.
    char* word = wordStarts[i];
	if (!*word) return NULL;
	if (!stricmp(word,loginID) || !stricmp(word,computerID)) return word; //   dont change his/our name ever
    int start = derivationIndex[i] >> 8;
    int end = derivationIndex[i] & 0x00ff;
    if (start != end || stricmp(wordStarts[i],derivationSentence[start])) // changing capitalization doesnt count as a change
        return word; // got here by a spellcheck so trust it.
	size_t len = strlen(word);
	if (len > 2 && word[len-2] == '\'') return word;	// dont do anything with ' words

    //   test for run togetherness like "talkabout fingers"
    int breakAt = SplitWord(word,i);
    if (breakAt > 0)//   we found a split, insert 2nd word into word stream
    {
		WORDP D = FindWord(word,breakAt,PRIMARY_CASE_ALLOWED);
		tokens[1] = D->word;
		tokens[2] = word+breakAt;
		fixedSpell = ReplaceWords("Splitword",i,1,2,tokens);
		return NULL;
    }

	// now imagine partial runtogetherness, like "talkab out fingers"
	if (i < wordCount)
	{
		char tmpword[MAX_WORD_SIZE*2];
		strcpy(tmpword,word);
		strcat(tmpword,wordStarts[i+1]);
		breakAt = SplitWord(tmpword,i);
		if (breakAt > 0  && stricmp(tmpword+breakAt,wordStarts[i+1])) // replace words with the dual pair
		{
			WORDP D = FindWord(tmpword,breakAt,PRIMARY_CASE_ALLOWED);
			tokens[1] = D->word;
			tokens[2] = tmpword +breakAt;
			fixedSpell = ReplaceWords("SplitWords",i,2,2,tokens);
			return NULL;
		}
	}

    //   remove any nondigit characters repeated more than once. Dont do this earlier, we want substitutions to have a chance at it first.  ammmmmmazing
	static char word1[MAX_WORD_SIZE];
    char* ptr = word-1; 
	char* ptr1 = word1;
    while (*++ptr)
    {
	   *ptr1 = *ptr;
	   while (ptr[1] == *ptr1 && ptr[2] == *ptr1 && (*ptr1 < '0' || *ptr1 > '9')) ++ptr; // skip double repeats
	   ++ptr1;
    }
	*ptr1 = 0;
	if (FindCanonical(word1,0,true) && !IsUpperCase(*word1)) return word1; // this is a different form of a canonical word so its ok

	//   now use word spell checker 
	size_t lenx = strlen(word); // try for reduced form
	if (word[lenx - 1] == 's') // noun plural and verb status
	{
		word[lenx - 1] = 0;
		char* d = SpellFix(word, i, VERB|NOUN);
		word[lenx - 1] = 's';
		if (d)
		{
			char plural[MAX_WORD_SIZE];
			strcpy(plural, d);
			strcat(plural, "s");
			WORDP X = StoreWord(plural);
			return X->word;
		}
	}
	if (!stricmp(&word[lenx - 3],"ing")) //  verb participle present
	{
		word[lenx - 3] = 0;
		char* d = SpellFix(word, i, VERB);
		word[lenx - 3] = 'i';
		if (d)
		{
			char plural[MAX_WORD_SIZE];
			strcpy(plural, d);
			strcat(plural, "ing");
			WORDP X = StoreWord(plural);
			return X->word;
		}
	}
    char* d = SpellFix(word,i,PART_OF_SPEECH); 
	if (d) return d;

	// if is is a misspelled plural?
	char plural[MAX_WORD_SIZE];
	if (word[len-1] == 's')
	{
		strcpy(plural,word);
		plural[len-1] = 0;
		d = SpellFix(plural,i,PART_OF_SPEECH); 
		if (d) return d; // dont care that it is plural
	}

    return NULL;
}

char* ProbableKnownWord(char* word)
{
	if (strchr(word,' ') || strchr(word,'_')) return word; // not user input, is synthesized
	size_t len = strlen(word);
	char lower[MAX_WORD_SIZE];
	MakeLowerCopy(lower,word);

	// do we know the word in lower case?
	WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	if (D) // direct recognition
	{
		if (D->properties & FOREIGN_WORD || *D->word == '~' || D->systemFlags & PATTERN_WORD) return D->word;	// we know this word clearly or its a concept set ref emotion
		if (D->properties & PART_OF_SPEECH && !IS_NEW_WORD(D)) return D->word; // old word we know
		if (D <= dictionaryPreBuild[LAYER_0]) return D->word; // in dictionary
		if (stricmp(language,"English") && !IS_NEW_WORD(D)) return D->word; // foreign word we know
		if (IsConceptMember(D)) return D->word;

		// are there facts using this word?
//		if (GetSubjectNondeadHead(D) || GetObjectNondeadHead(D) || GetVerbNondeadHead(D)) return D->word;
	}

	// do we know the word in upper case?
	char upper[MAX_WORD_SIZE];
	MakeLowerCopy(upper,word);
	upper[0] = GetUppercaseData(upper[0]);
	D = FindWord(upper,0,UPPERCASE_LOOKUP);
	if (D) // direct recognition
	{
		if (D->properties & FOREIGN_WORD || *D->word == '~' || D->systemFlags & PATTERN_WORD) return D->word;	// we know this word clearly or its a concept set ref emotion
		if (D->properties & PART_OF_SPEECH && !IS_NEW_WORD(D)) return D->word; // old word we know
		if (D <= dictionaryPreBuild[LAYER_0]) return D->word; // in dictionary
		if (stricmp(language,"English") && !IS_NEW_WORD(D)) return D->word; // foreign word we know
		if (IsConceptMember(D)) return D->word;

	// are there facts using this word?
//		if (GetSubjectNondeadHead(D) || GetObjectNondeadHead(D) || GetVerbNondeadHead(D)) return D->word;
	}

	// interpolate to lower case words 
    if (!stricmp(language, "english"))
    {
        uint64 expectedBase = 0;
        if (ProbableAdjective(word, len, expectedBase) && expectedBase) return word;
        expectedBase = 0;
        if (ProbableAdverb(word, len, expectedBase) && expectedBase) return word;

        // is it a verb form
        char* verb = GetInfinitive(lower, true); // no new verbs
        if (verb)  return  StoreWord(lower, 0)->word; // verb form recognized

        // is it simple plural of a noun?
        if (word[len - 1] == 's')
        {
            WORDP E = FindWord(lower, len - 1, LOWERCASE_LOOKUP);
            if (E && E->properties & NOUN)
            {
                E = StoreWord(word, NOUN | NOUN_PLURAL);
                return E->word;
            }
            E = FindWord(lower, len - 1, UPPERCASE_LOOKUP);
            if (E && E->properties & NOUN)
            {
                *word = toUppercaseData[*word];
                E = StoreWord(word, NOUN | NOUN_PROPER_PLURAL);
                return E->word;
            }
        }
    }
	return NULL;
}

static bool UsefulKnownWord(WORDP D)
{
	if (IS_NEW_WORD(D)) return false;
	if (D->properties & TAG_TEST) return true;
	if (D->systemFlags & PATTERN_WORD) return true;
	return false;
}

bool SpellCheckSentence()
{
	if (!stricmp(language, "ideographic")) return false; // no spell check on them
	char retry[256];
	memset(retry, 0, sizeof(retry));
    char* tokens[6];
	fixedSpell = false;
	bool isEnglish = (!stricmp(language, "english") ? true : false);
    bool isGerman = (!stricmp(language, "german") ? true : false);
	int startWord = FindOOBEnd(1);
	int i;
	int badspelllimit = 0;
	int badspellsize = 0;
	float badspellratio = 1.0;
	char* baddata = GetUserVariable("$cs_badspellLimit"); // 10-20 (50%)
	if (*baddata)
	{
		badspelllimit = atoi(baddata);
		baddata = strchr(baddata, '-');
		if (baddata)
		{
			badspellsize = atoi(baddata + 1);
			badspellratio = (float) badspelllimit / badspellsize;
		}
	}
	bool delayedspell = false;
	int spellbad = 0;
	for (i = startWord; i <= wordCount; ++i)
	{
		if (retry[i-1]) // prior was a problem needing full spellcheck
		{
			if (++spellbad >= badspelllimit && badspelllimit) // crossed threshhold of number of spellchecks
			{
				float ratio = (float)spellbad / (i + 1 - startWord); // we never hit last word
				if (ratio >= badspellratio)
				{
					if (trace & TRACE_SPELLING) Log(STDUSERLOG, (char*)"BadSpell abort NL\r\n");
					char junk[100];
					sprintf(junk, "%d-%d", spellbad, (i + 1 - startWord));
					SetUserVariable("$$cs_badspell", junk);
					break;
				}
			}
		}

		char* word = wordStarts[i];
		int size = strlen(word);
		if (size > (MAX_WORD_SIZE - 100)) continue;

		char bigword[3 * MAX_WORD_SIZE]; // allows join of 2 words
		if (i != wordCount) // merge 2 adj words w hyphen if can, even though one or both are legal words
		{
			WORDP X = FindWord(word);
			WORDP Y = FindWord(wordStarts[i + 1]);
			if (!(X && Y && UsefulKnownWord(X) && UsefulKnownWord(Y))) // has-been is a word we dont want merge,but model numbers we do.
			{
				strcpy(bigword, word);
				size_t len = strlen(bigword);
				strcpy(bigword + len++, "-");
				strcpy(bigword + len, wordStarts[i + 1]);
				WORDP XX = FindWord(bigword);
				if (XX && UsefulKnownWord(XX))
				{
					tokens[1] = XX->word;
					fixedSpell = ReplaceWords("merge to hyphenword", i, 2, 1, tokens);
					continue;
				}
			}
		}
		if (spellTrace)
		{
			strcpy(spellCheckWord, word);
			echo = true;
		}

		// change any \ to /
		char newword[MAX_WORD_SIZE];
		bool altered = false;
		strcpy(newword, word);
		char* at = newword;
		while ((at = strchr(at, '\\')))
		{
			*at = '/';
			altered = true;
		}
		if (altered) word = wordStarts[i] = StoreWord(newword, AS_IS)->word;

		// do we know the word meaningfully as is?
		WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (D && !IS_NEW_WORD(D))
		{
			bool good = false;
			if (D->properties & TAG_TEST || *D->word == '~' || D->systemFlags & PATTERN_WORD) good = true;	// we know this word clearly or its a concept set ref emotion
			else if (D <= dictionaryPreBuild[LAYER_0]) good = true; // in dictionary - if a substitute would have happend by now
			else if (!isEnglish) good = true; // foreign word we know
			else if (IsConceptMember(D)) good = true;
			if (good) continue;
		}
		// handle lower case forms of upper case nouns
		if (isGerman && !D)
		{
			WORDP E = FindWord(word, 0, SECONDARY_CASE_ALLOWED);
			if (E && E->internalBits & UPPERCASE_HASH && E->properties & NOUN_SINGULAR) // we had lower case, we find upper case
			{
				tokens[1] = E->word;
				fixedSpell = ReplaceWords("' German lowercased noun", i, 1, 1, tokens);
				continue;
			}
		}
		// he gave upper case, try easy lower case
		WORDP LD = FindWord(word, 0, LOWERCASE_LOOKUP);
		if (LD && !IS_NEW_WORD(LD) && IsUpperCase(*wordStarts[i]))
		{
			bool good = false;
			if (LD->properties & TAG_TEST || *LD->word == '~' || LD->systemFlags & PATTERN_WORD) good = true;	// we know this word clearly or its a concept set ref emotion
			else if (LD <= dictionaryPreBuild[LAYER_0]) good = true; // in dictionary - if a substitute would have happend by now
			else if (!isEnglish) good = true; // foreign word we know
			else if (IsConceptMember(LD)) good = true;
			if (good)
			{
				tokens[1] = LD->word;
				fixedSpell = ReplaceWords("' lowercase", i, 1, 1, tokens);
				continue;
			}
		}

		if (IsDate(word)) continue; // allow 1970/10/5 or similar
		// dont spell check  numbers
		size_t l = strlen(word);
		char* end = word + l;
		if (IsFloat(word, end, numberStyle)) continue;
		if (IsFractionNumber(word)) continue;
		if (IsDigitWord(word)) continue;

		if (IsUrl(word, word + l)) continue;

		// appended &quot?
		if (*word != '&' && !stricmp(word + l - 5, "&quot"))
		{
			word[l - 5] = 0;
			WORDP X = StoreWord(word);
			tokens[1] = X->word;
			tokens[2] = "\"";
			fixedSpell = ReplaceWords("split &quot", i, 1, 2, tokens);
			--i; // retry spellcheck
			continue;
		}

		if (*word == '\'')
		{
			WORDP X = ApostropheBreak(word);
			if (X)
			{
				tokens[1] = X->word;
				fixedSpell = ReplaceWords("' replace", i, 1, 1, tokens);
				continue;
			}
		}
		// degrees
		if ((unsigned char)*word == 0xc2 && (unsigned char)word[1] == 0xb0 && !word[3]) continue; // leave degreeC,F,K, etc alone

		if (isEnglish && *word == '\'' && !word[1] && i != startWord && IsDigit(*wordStarts[i - 1])) // fails if not digit bug
		{
			tokens[1] = (char*)"foot";
			fixedSpell = ReplaceWords("' as feet", i, 1, 1, tokens);
			continue;
		}
		if (isEnglish && *word == '"' && !word[1] && i != startWord && IsDigit(*wordStarts[i - 1])) // fails if not digit bug
		{
			tokens[1] = (char*)"inch";
			fixedSpell = ReplaceWords("' as feet", i, 1, 1, tokens);
			continue;
		}
		if (!word || !word[1] || *word == '"') continue; // illegal or single char or quoted thingy 

		//  dont spell check email or other things with @ or . in them
		if (strchr(word, '@') || strchr(word, '&') || strchr(word, '$')) continue;

		//  dont spell check hashtags
		if (word[0] == '#' && !IsDigit(word[1])) {
			bool validHash = true;
			for (int j = 1; j < size; ++j) {
				if (!IsAlphaUTF8OrDigit(word[j]) && word[j] != '_') {
					validHash = false;
					break;
				}
			}
			if (validHash) continue;
		}

		// dont spell check names of json objects or arrays
		if (!strnicmp(word, "ja-", 3) || !strnicmp(word, "jo-", 3)) continue;

		// dont spell check web addresses
		if (!strnicmp(word, "http", 4) || !strnicmp(word, "www", 3)) continue;

		// dont spell check abbreviations with dots, e.g. p.m.
		char* dot = strchr(word, '.');
		if (dot && FindWord(word, 0)) continue;

		// joined number words like 100dollars
		at = word - 1;
		while (IsDigit(*++at) || *at == '.' || *at == ',');
		if (IsDigit(*word) && strlen(at) > 3 && ProbableKnownWord(at))
		{
			char first[MAX_WORD_SIZE];
			strncpy(first, word, (at - word));
			first[at - word] = 0;
			if (IsDigitWord(first, numberStyle, true))
			{
				tokens[1] = first;
				tokens[2] = at;
				fixedSpell = ReplaceWords("joined number word", i, 1, 2, tokens);
				continue;
			}
		}

		// mistaken " for 1 in don"t?
		char* quo = strchr(word, '"');
		if (quo && quo[1] == 't' && !quo[2])
		{
			*quo = '\'';
			tokens[1] = word;
			fixedSpell = ReplaceWords("q for dq", i, 1, 1, tokens);
			continue;
		}

		// allow data1 and its friends for regression tests
		// joined number words like dollars100, but allow single digits at end - assume model numbers are not normal words with number after
		at = end;
		while (at >= word && (IsDigit(*--at) || *at == '.' || *at == ','));
		if (IsDigit(*++at) && at[1] && (at - word) > 3 && IsDigitWord(at, numberStyle, true))
		{
			char first[MAX_WORD_SIZE];
			strncpy(first, word, (at - word));
			first[at - word] = 0;
			if (ProbableKnownWord(first))
			{
				tokens[1] = first;
				tokens[2] = at;
				fixedSpell = ReplaceWords("joined word number", i, 1, 2, tokens);
				continue;
			}
		}

		// dont spellcheck model numbers
		if (IsModelNumber(word))
		{
			WORDP X = FindWord(word, 0, UPPERCASE_LOOKUP);
			if (IsConceptMember(X) && !strcmp(word, X->word))
			{
				tokens[1] = X->word;
				fixedSpell = ReplaceWords("KnownUpperModelNumber", i, 1, 1, tokens);
			}
			continue;
		}

		// split conjoined sentetence Missouri.Fix  or Missouri..Fix
		// but dont split float values like 0.5%
		if (dot && dot != word && dot[1] && !IsDigit(dot[1]))
		{
			*dot = 0;
			// don't spell correct if this looks like a filename
			char* rest = dot + 1;
			if (!IsFileExtension(rest)) {
				WORDP X = FindWord(word, 0);
				while (rest[0] == '.') ++rest; // swallow all excess dots
				WORDP Y = FindWord(rest, 0);
				if (X && Y) // we recognize the words
				{
					char oper[10];
					tokens[1] = word;
					tokens[2] = oper;
					*oper = '.';
					oper[1] = 0;
					tokens[3] = rest;
					fixedSpell = ReplaceWords("dotsentence", i, 1, 3, tokens);
					*dot = '.';  // restore the dot so the original is still in derivationSentence
					continue;
				}
			}
			*dot = '.';  // restore the dot
		}

		//  dont spell check things with . in them
		if (dot) continue;

		char* number;
		if (GetCurrency((unsigned char*)word, number)) continue; // currency

		if (isEnglish && !stricmp(word, (char*)"am") && i != startWord &&
			(IsDigit(*wordStarts[i - 1]) || IsNumber(wordStarts[i - 1], numberStyle) == REAL_NUMBER)) // fails if not digit bug
		{
			tokens[1] = (char*)"a.m.";
			fixedSpell = ReplaceWords("am as time", i, 1, 1, tokens);
			continue;
		}

		// words with excess repeated characters >2 => 2
		char excess[MAX_WORD_SIZE];
		if (!IsDigit(word[1]) && word[1] != '.' && word[1] != ',')
		{
			size_t len = size;
			strcpy(excess, word);
			bool change = false;
			// refuse to believe any 3 or more repeats
			for (size_t j = 0; j < len; ++j)
			{
				if (excess[j] == excess[j + 1] && excess[j + 2] == excess[j])
				{
					memmove(excess + j + 1, excess + j + 2, strlen(excess + j + 1));
					j -= 1;
					--len;
					change = true;
					continue;
				}
			}
			if (change)
			{
				tokens[1] = excess;
				fixedSpell = ReplaceWords("multiple repeat letters", i, 1, 1, tokens);
				word = wordStarts[i];
				D = FindWord(excess);
				if (D && !IS_NEW_WORD(D)) continue;
			}
		}

		// words with excess repeated characters 2=>1 unless root noun or verb
		if (!FindWord(word) && !IsDigit(word[1]) && word[1] != '.' && word[1] != ',')
		{
			if (!stricmp(language, "english"))
			{
				char* noun = GetSingularNoun(word, false, true);
				if (noun) continue;
				char* verb = GetInfinitive(word, true);
				if (verb) continue;
			}
			strcpy(excess, word);
			bool change = false;
			for (int j = 0; j < size; ++j)
			{
				if (excess[j] == excess[j + 1])
				{
					memmove(excess + j + 1, excess + j + 2, strlen(excess + j + 1));
					if (FindWord(excess))
					{
						tokens[1] = excess;
						fixedSpell = ReplaceWords("2 repeat letters", i, 1, 1, tokens);
						break;
					}
					else strcpy(excess, word);
				}
			}
			if (change) continue;
		}

		// split arithmetic  1+2
		if (IsDigit(*word) && IsDigit(word[size - 1]))
		{
			at = word;
			while (IsDigit(*++at) || *at == '.' || *at == ',') { ; }
			char* op = at;
			if (*at == '+' || *at == '-' || *at == '*' || *at == '/')
			{
				while (IsDigit(*++at) || *at == '.' || *at == ',') { ; }
				if (!*at && (size != 9 || *op != '-'))  // 445+455 but not zip code
				{
					char oper[10];
					tokens[2] = oper;
					*oper = *op;
					oper[1] = 0;
					*op = 0;
					tokens[1] = word;
					tokens[3] = op + 1;
					fixedSpell = ReplaceWords("smooshed 1+2", i, 1, 3, tokens);
					continue;
				}
			}
		}

		// split off trailing - 
		if (size > 1 && word[size - 1] == '-')
		{
			WORDP X = FindWord(word, size - 1);
			if (X)
			{
				tokens[1] = X->word;
				tokens[2] = "-";
				fixedSpell = ReplaceWords("trailing hyphen split", i, 1, 2, tokens);
				continue;
			}
		}

		// merge with next token?
		if (i != wordCount && *wordStarts[i + 1] != '"')
		{
			// direct merge as a single word
			strcpy(bigword, word);
			strcat(bigword, wordStarts[i + 1]);
			D = FindWord(bigword, 0, (tokenControl & ONLY_LOWERCASE) ? PRIMARY_CASE_ALLOWED : STANDARD_LOOKUP);
			if (D && D->properties & PART_OF_SPEECH && !(D->properties & AUX_VERB)) {} // merge these two, except "going to" or wordnet composites of normal words
			else // merge with underscore?  shia tsu
			{
				strcpy(bigword, word);
				strcat(bigword, "_");
				strcat(bigword, wordStarts[i + 1]);
				D = FindWord(bigword, 0, (tokenControl & ONLY_LOWERCASE) ? PRIMARY_CASE_ALLOWED : STANDARD_LOOKUP);
				if (D && D->properties & PART_OF_SPEECH && !(D->properties & AUX_VERB)) // allow these two, except "going to" or wordnet composites of normal words
				{
					++i;
					continue;
				}
			}
			if (D && D->properties & PART_OF_SPEECH && !(D->properties & AUX_VERB)) // merge these two, except "going to" or wordnet composites of normal words
			{
				WORDP P1 = FindWord(word, 0, LOWERCASE_LOOKUP);
				WORDP P2 = FindWord(wordStarts[i + 1], 0, LOWERCASE_LOOKUP);
				if (!P1 || !P2 || !(P1->properties & PART_OF_SPEECH) || !(P2->properties & PART_OF_SPEECH))
				{
					tokens[1] = D->word;
					fixedSpell = ReplaceWords("merge", i, 2, 1, tokens);
					continue;
				}
			}
		}

		// merge with prior token?
		if (i != 1 && *wordStarts[i - 1] != '"') // allow piece to stand, sequences code will find it
		{
			strcpy(bigword, wordStarts[i - 1]);
			strcat(bigword, "_");
			strcat(bigword, word);
			D = FindWord(bigword, 0, (tokenControl & ONLY_LOWERCASE) ? PRIMARY_CASE_ALLOWED : STANDARD_LOOKUP);
			if (D && D->properties & PART_OF_SPEECH && !(D->properties & AUX_VERB))
			{
				++i;
				continue;
			}
			if (D && D->systemFlags & PATTERN_WORD)
			{
				++i;
				continue;
			}
		}

		// sloppy omitted g in lookin but not for kevin
		if (isEnglish && word[size - 1] == 'n' && word[size - 2] == 'i')
		{
			D = FindWord(word, size - 2);
			if (D && D->properties & BASIC_POS)
			{
				char test[MAX_WORD_SIZE];
				strcpy(test, word);
				strcat(test, "g");
				WORDP E = FindWord(word, 0, SECONDARY_CASE_ALLOWED);
				WORDP F = FindWord(test);
				if (!E || F) {
					tokens[1] = test;
					fixedSpell = ReplaceWords("omitg", i, 1, 1, tokens);
					continue;
				}
			}
		}

		char* known = ProbableKnownWord(word);
		if (known && !strcmp(known, word)) continue;	 // we know it
		if (known)
		{
			D = FindWord(known);
			if ((!D || !(D->internalBits & UPPERCASE_HASH)) && !IsUpperCase(*known)) // revised the word to lower case (avoid to upper case like "fields" to "Fields"
			{
				WORDP X = FindWord(known, 0, LOWERCASE_LOOKUP);
				if (X)
				{
					tokens[1] = X->word;
					fixedSpell = ReplaceWords("KnownWord", i, 1, 1, tokens);
					continue;
				}
			}
			else // is uppercase a concept member? then revise upwards
			{
				WORDP X = FindWord(known, 0, UPPERCASE_LOOKUP);
				if (IsConceptMember(X) || isGerman) // all german nouns are uppercase
				{
					tokens[1] = X->word;
					fixedSpell = ReplaceWords("KnownUpper", i, 1, 1, tokens);
					continue;
				}
			}
		}

		if (GetTemperatureLetter(word)) continue;	// bad ignore utf word or llegal length - also no composite words

		char* hyphen = strrchr(word, '-');// note is hyphenated - use trailing

		// see if we know the other case
		if (!(tokenControl & (ONLY_LOWERCASE | STRICT_CASING)) || (i == startSentence && !(tokenControl & ONLY_LOWERCASE)))
		{
			WORDP E = FindWord(word, 0, SECONDARY_CASE_ALLOWED);
			if (IS_NEW_WORD(E)) E = NULL; // must preexist in dictionary
			bool useAlternateCase = false;
			if (E && E->systemFlags & PATTERN_WORD) useAlternateCase = true;
			if (E && E->properties & (PART_OF_SPEECH | FOREIGN_WORD))
			{
				// if the word we find is UPPER case, and this might be a lower case noun plural, don't change case.
				if (isEnglish && word[size - 1] == 's')
				{
					WORDP F = FindWord(word, size - 1);
					if (!F || !(F->properties & (PART_OF_SPEECH | FOREIGN_WORD))) useAlternateCase = true;
					else continue;
				}
				else useAlternateCase = true;
			}
			else if (E) // does it have a member concept fact
			{
				if (IsConceptMember(E))
				{
					useAlternateCase = true;
					break;
				}
			}
			if (useAlternateCase)
			{
				tokens[1] = E->word;
				fixedSpell = ReplaceWords("Alternatecase", i, 1, 1, tokens);
				continue;
			}
		}

		// break apart slashed pair like eat/feed
		char* slash = strchr(word, '/');
		if (slash && !slash[1]) // remove trailing slash
		{
			strcpy(newword, word);
			newword[slash - word] = 0;
			word = wordStarts[i] = StoreWord(newword, AS_IS)->word;
		}
		char* slash1 = NULL;
		if (slash) slash1 = strchr(slash + 1, '/');
		if (slash && slash != word && slash[1] && !slash1) //   break apart word/word unless date
		{
			if ((wordCount + 2) >= REAL_SENTENCE_WORD_LIMIT) continue;	// no room
			*slash = 0;
			D = StoreWord(word);
			*slash = '/';
			WORDP E = StoreWord(slash + 1);
			char* tokenlist[4];
			tokenlist[1] = D->word;
			tokenlist[2] = "/";
			tokenlist[3] = E->word;
			fixedSpell = ReplaceWords("Split", i, 1, 3, tokenlist);
			--i;
			continue;
		}

		// see if hypenated word should be separate or joined (ignore obvious adjective suffix)
		if (hyphen && isEnglish && !stricmp(hyphen, (char*)"-like"))
		{
			StoreWord(word, ADJECTIVE_NORMAL | ADJECTIVE); // accept it as a word
			continue;
		}
		else if (hyphen && (hyphen - word) > 1 && !IsPlaceNumber(word, numberStyle)) // dont break up fifty-second
		{
			char test[MAX_WORD_SIZE];
			char first[MAX_WORD_SIZE];

			// test for split
			*hyphen = 0;
			strcpy(test, hyphen + 1);
			strcpy(first, word);
			*hyphen = '-';

			WORDP E = FindWord(test, 0, LOWERCASE_LOOKUP);
			D = FindWord(first, 0, LOWERCASE_LOOKUP);
			if (*first == 0)
			{
				wordStarts[i] = AllocateHeap(wordStarts[i] + 1); // -pieces  want to lose the leading hypen  (2-pieces)
				fixedSpell = true;
				continue;
			}
			else if (D && E && UsefulKnownWord(D) && UsefulKnownWord(E)) //   1st word gets replaced, we added another word after
			{
				if ((wordCount + 1) >= REAL_SENTENCE_WORD_LIMIT) continue;	// no room
				tokens[1] = D->word;
				tokens[2] = E->word;
				fixedSpell = ReplaceWords("Pair", i, 1, 2, tokens);
				++i; // skip testing next word
				continue;
			}
			else if (!stricmp(test, (char*)"old") || !stricmp(test, (char*)"olds")) //   break apart 5-year-old
			{
				if ((wordCount + 1) >= REAL_SENTENCE_WORD_LIMIT) continue;	// no room
				D = StoreWord(first);
				E = StoreWord(test);
				tokens[1] = D->word;
				tokens[2] = E->word;
				fixedSpell = ReplaceWords("Break old", i, 1, 2, tokens);
				--i;
				continue;
			}
			else // remove hyphen entirely?
			{
				strcpy(test, first);
				strcat(test, hyphen + 1);
				D = FindWord(test, 0, (tokenControl & ONLY_LOWERCASE) ? PRIMARY_CASE_ALLOWED : STANDARD_LOOKUP);
				if (D)
				{
					wordStarts[i] = D->word;
					fixedSpell = true;
					--i;
					continue;
				}
			}
		}

		// see if number in front of unit split like 10mg
		if (IsDigit(*word))
		{
			at = word;
			while (*++at && IsDigit(*at)) { ; }
			WORDP E = FindWord(at);
			if (E && strlen(at) > 2 && *at != 'm') // number in front of known word ( but must be longer than 2 char, 5th) but allow mg
			{
				char token1[MAX_WORD_SIZE];
				size_t len = at - word;
				strncpy(token1, word, len);
				token1[len] = 0;
				D = StoreWord(token1);
				tokens[1] = D->word;
				tokens[2] = E->word;
				fixedSpell = ReplaceWords("Split", i, 1, 2, tokens);
				continue;
			}
		}

		// dont spell check uppercase not at start or joined word
		if (IsUpperCase(word[0]) && (i != startWord || strchr(word, '_')) && tokenControl & NO_PROPER_SPELLCHECK) continue;

		// leave uppercase in first position if not adjusted yet... but check for lower case spell error
		if (IsUpperCase(word[0]) && tokenControl & NO_PROPER_SPELLCHECK)
		{
			char lower[MAX_WORD_SIZE];
			MakeLowerCopy(lower, word);
			D = FindWord(lower, 0, LOWERCASE_LOOKUP);
			if (!D && i == startWord)
			{
				char* okword = SpellFix(lower, i, PART_OF_SPEECH);
				if (okword)
				{
					WORDP E = StoreWord(okword);
					tokens[1] = E->word;
					fixedSpell = ReplaceWords("Spell", i, 1, 1, tokens);
				}
			}
			continue;
		}

		////////////////////////////////////////////////////
		///// We dont charge for fixes done above. But here on,
		///// we consider it a serious issue. Spellcheck and getPosData is expensive.
		///// Do it too much and user is probably useless.
		////////////////////////////////////////////////////

		// try for conjugations et all
		WORDP revise, entry, canonical;
		uint64 xflags = 0;
		uint64 cansysflags = 0;
		uint64 inferredProperties = GetPosData(-1, word, revise, entry, canonical, xflags, cansysflags);
		if (entry && entry->internalBits & HAS_SUBSTITUTE) entry = canonical = NULL;
		if (canonical && !stricmp(canonical->word, "unknown-word")) canonical = NULL;
		if (canonical && !(canonical->internalBits & UPPERCASE_HASH) && (canonical != entry) || (inferredProperties & (NOUN_NUMBER | ADJECTIVE_NUMBER)))
		{
			if (entry && strcmp(entry->word, word)) // probably case changed
			{
				tokens[1] = entry->word;
				fixedSpell = ReplaceWords("' getposdata", i, 1, 1, tokens);
			}
			else if (entry && (entry->internalBits & UPPERCASE_HASH)) // switch to canonical lowercase
			{
				tokens[1] = canonical->word;
				fixedSpell = ReplaceWords("' getposdata", i, 1, 1, tokens);
			}
			continue; // we know it is some conjugated form
		}
		if (entry && strcmp(entry->word, word)) // changed (maybe case)
		{
			tokens[1] = entry->word;
			fixedSpell = ReplaceWords("' posresult", i, 1, 1, tokens);
			continue;
		}

		retry[i] = 1; // do expensive last
		delayedspell = true;
		continue;
	}
	if (i > wordCount && delayedspell) for (i = startWord; i <= wordCount; ++i)
	{
		if (!retry[i]) continue; // no problem here
		char* word = wordStarts[i];

		// see if smooshed word pair
		if (*word != '\'' && (!FindCanonical(word, i, true) || IsUpperCase(word[0]))) // dont check quoted or findable words unless they are capitalized
		{
			char* word1 = SpellCheck(i);

			// dont spell check proper names to improper, if word before or after is lower case originally
			// unless a substitute like g-mail-> Gmail
			if (word1 && i != 1 && originalCapState[i] && !IsUpperCase(*word1))
			{
				WORDP X = FindWord(word1);
				if (X && X->internalBits & HAS_SUBSTITUTE) {}
				else if (!originalCapState[i - 1]) continue;
				else if (i != wordCount && !originalCapState[i + 1]) continue;
			}

			if (word1 && !*word1) // performed substitution on prior word, restart this one
			{
				fixedSpell = true;
				--i;
				continue;
			}
			if (word1)
			{
				tokens[1] = word1;
				fixedSpell = ReplaceWords("Nearest real word", i, 1, 1, tokens);
				continue;
			}
		}
	}
	if (spellbad > (badspellcount >> 16))
	{
		int size = wordCount - startWord + 1;
		badspellcount = (spellbad << 16) + size;
	}

	return fixedSpell;
}

static char UnaccentedChar(char* str)
{
	unsigned char c = (unsigned char)*str;
	if (!(c & 0x80)) return c;
	unsigned char c1 = (unsigned char)str[1];

	// can we change an accented lowercase letter to unaccented? 
	if (c == 0xc3)
	{// b0?  be
		if ((c1 >= 0xa0 && c1 <= 0xa5)) return 'a';
		else if ((c1 == 0xa7)) return 'c';
		else if ((c1 >= 0xa8 && c1 <= 0xab)) return 'e';
		else if ((c1 >= 0xac && c1 <= 0xaf)) return 'i';
		else if ((c1 == 0xb1)) return 'n';
		else if ((c1 >= 0xb2 && c1 <= 0xb6)) return 'o';
		else if ((c1 == 0xb8 )) return 'o';
		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 'u';
		else if ((c1 == 0xbd || (unsigned char)str[2] == 0xbf)) return 'y';
		// not doing ae else if (*currentCharReal == 'a' && *nextCharReal == 'e' && *nextCharDict == 0xa6)
//		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 'd';
//		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 'g';
//		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 'r';
//		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 's';
//		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 't';
//		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 'w';
//		else if ((c1 >= 0xb9 && c1 <= 0xbc)) return 'z';
	}
	else if (c == 0xc4)
	{
		if ((c1 == 0x81 || c1 == 0x83 || c1 == 0x85)) return 'a';
		else if ((c1 == 0x87 || c1 == 0x89 || c1 == 0x8b || c1 == 0x8d)) return 'c';
		else if ((c1 == 0x8f) || c1 == 0x91) return 'd';
		else if ((c1 == 0x93 || c1 == 0x95 || c1 == 0x97 || c1 == 0x99 || c1 == 0x9b)) return 'e';
		else if ((c1 == 0x9d || c1 == 0x9f|| c1 == 0xa1 || c1 == 0xa3)) return 'g';
		else if ((c1 == 0xa5 || c1 == 0xa7)) return 'h';
		else if ((c1 == 0xa9 || c1 == 0xab || c1 == 0xad || c1 == 0xaf || c1 == 0xb1)) return 'i';
		else if ((c1 == 0xb5)) return 'j';
		else if ((c1 == 0xb7)) return 'k';
		else if ((c1 == 0xba || c1 == 0xbc || c1 == 0xbe)) return 'l';
	}
	else if (c == 0xc5)
	{
		if ((c1 == 0x80 || c1 == 0x82 || c1 == 0x86 || c1 == 0x88)) return 'l';
		else if ((c1 == 0x80 || c1 == 0x82)) return 'n';
		else if ((c1 == 0x8d || c1 == 0x8f || c1 == 0x91)) return 'o';
		else if ((c1 == 0x97 || c1 == 0x99)) return 'r';
		else if ((c1 == 0x9b || c1 == 0x9d || c1 == 0x9f || c1 == 0xa1)) return 's';
		else if ((c1 == 0xa3 || c1 == 0xa5 || c1 == 0xa7)) return 't';
		else if ((c1 == 0xa9 || c1 == 0xab || c1 == 0xad || c1 == 0xaf || c1 == 0xb1 || c1 == 0xb3)) return 'u';
		else if ((c1 == 0xb5 )) return 'w';
		else if ((c1 == 0xb7)) return 'y';
		else if ((c1 == 0xba || c1 == 0xbc || c1 == 0xbe)) return 'z';
	}
	else if (c == 0xc7)
	{
		if ((c1 == 0x8e || c1 == 0xa1)) return 'a';
		else if ((c1 == 0x90)) return 'i';
		else if ((c1 == 0x92)) return 'o';
		else if ((c1 == 0x94 || c1 == 0x96 || c1 == 0x98 || c1 == 0x9a || c1 == 0x9c)) return 'u';
		else if ((c1 == 0xa5 || c1 == 0xa7|| c1 == 0x98 )) return 'g';
		else if ((c1 == 0xa9)) return 'k';
		else if ((c1 == 0xab || c1 == 0xad)) return 'o';
		else if ((c1 == 0xb0)) return 'j';
		else if ((c1 == 0xb5)) return 'g';
		else if ((c1 == 0xb9)) return 'n';
		else if ((c1 == 0xbb)) return 'a';
		else if ((c1 == 0xbf)) return 'o';
	}
	return 0;
}

static int EditDistance(WORDINFO& dictWordData, WORDINFO& realWordData,int min)
{//   dictword has no underscores, inputSet is already lower case
    char dictw[MAX_WORD_SIZE];
    MakeLowerCopy(dictw, dictWordData.word);
    char* dictinfo = dictw;
    char* realinfo = realWordData.word;
    char* dictstart = dictinfo;
	char* realstart = realWordData.word;
    int val = 0; //   a difference in length will manifest as a difference in letter count
	//  look at specific letter errors
    char priorCharDict[10];
    char priorCharReal[10];
    *priorCharDict = *priorCharReal = 0;
    char currentCharReal[10];
    char currentCharDict[10];
    *currentCharReal = *currentCharDict = 0;
    char nextCharReal[10];
    char nextCharDict[10];
    char next1CharReal[10];
    char next1CharDict[10];
    char* resumeReal2;
    char* resumeDict2;
    char* resumeReal;
    char* resumeDict;
    char* resumeReal1;
    char* resumeDict1;
	char baseCharReal;
	char baseCharDict;
    bool isFrench = (!stricmp(language, "french") ? true : false);
    bool isGerman = (!stricmp(language, "german") ? true : false);
    bool isSpanish = (!stricmp(language, "spanish") ? true : false);
    while (ALWAYS)
    {
        if (val > min) return 1000; // no good
        strcpy(priorCharReal, currentCharReal);
        strcpy(priorCharDict, currentCharDict);

        resumeReal = IsUTF8((char*)realinfo, currentCharReal);
        resumeDict = IsUTF8((char*)dictinfo, currentCharDict);
        if (!*currentCharReal && !*currentCharDict) break; //both at end
        if (!*currentCharReal || !*currentCharDict) // one ending, other has to catch up by adding a letter
        {
            val += 16; // add a letter
            if (*priorCharReal == *currentCharDict) val -= 5; // doubling letter at end
            dictinfo = resumeDict;
            realinfo = resumeReal;
            continue;
        }

		// punctuation in a word is bad tokenization, dont spell check it away
		if (*currentCharReal == '?' || *currentCharReal == '!' || *currentCharReal == '('
			|| *currentCharReal == ')' ||  *currentCharReal == '[' || *currentCharReal == ']'
			|| *currentCharReal == '{' || *currentCharReal == '}') return 200; // dont mess with this

        resumeReal1 = IsUTF8((char*)resumeReal, nextCharReal);
        resumeDict1 = IsUTF8((char*)resumeDict, nextCharDict);
        resumeReal2 = IsUTF8((char*)resumeReal1, next1CharReal); // 2 char ahead
        resumeDict2 = IsUTF8((char*)resumeDict1, next1CharDict);
		baseCharReal = UnaccentedChar(currentCharReal);
		baseCharDict = UnaccentedChar(currentCharDict);
		if (!stricmp(currentCharReal, currentCharDict)) // match chars
		{
			dictinfo = resumeDict;
			realinfo = resumeReal;
			continue;
		}
		if (baseCharReal && baseCharReal == baseCharDict)
		{
			dictinfo = resumeDict;
			realinfo = resumeReal;
			val += 1; // minimal charge but separate forms of delivre
			continue;
		}
        // treat german double s and ss equivalent
        if (isGerman)
        {
            if ((unsigned char)*currentCharReal == 0xc3 && (unsigned char)currentCharReal[1] == 0x9f && *currentCharDict == 's' && *nextCharDict == 's')
            {
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
            if ((unsigned char)*currentCharDict == 0xc3 && (unsigned char)currentCharDict[1] == 0x9f && *currentCharReal == 's'  && *nextCharReal == 's')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal1;
                continue;
            }
        }
        // spanish alternative spellings
        if (isSpanish) // ch-x | qu-k | c-k | do-o | b-v | bue-w | vue-w | z-s | s-c | h- | y-i | y-ll | m-n  1st is valid
        {
            if (*currentCharReal == 'c' && *currentCharDict == 'k')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'b' && *currentCharDict == 'v')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'v' && *currentCharDict == 'b')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'z' && *currentCharDict == 's')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 's' && *currentCharDict == 'c')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'y' && *currentCharDict == 'i')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'm' && *currentCharDict == 'n')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'n' && *currentCharDict == 'm')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharDict == 'h')
            {
                dictinfo = resumeDict;
                continue;
            }
            if (*currentCharReal == 'x' && *currentCharDict == 'c' && *nextCharDict == 'h')
            {
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'k' && *currentCharDict == 'q' && *nextCharDict == 'u')
            {
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'o' && *currentCharDict == 'd' && *nextCharDict == 'o')
            {
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'w' && *currentCharDict == 'b' && *nextCharDict == 'u'  && *next1CharDict == 'e')
            {
                dictinfo = resumeDict2;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'w' && *currentCharDict == 'v' && *nextCharDict == 'u'  && *next1CharDict == 'e')
            {
                dictinfo = resumeDict2;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'l' && *nextCharReal == 'l' && *currentCharDict == 'y')
            {
                dictinfo = resumeDict;
                realinfo = resumeReal1;
                continue;
            }
            if (*currentCharReal == 'y' && *currentCharDict == 'l' && *nextCharDict == 'l')
            {
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
        }
        // french common bad spellings
        if (isFrench)
        {
            if (*currentCharReal == 'a' && SameUTF(currentCharDict, (char *)"â"))
            {
            		val += 1;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'e' && SameUTF(currentCharDict, (char *)"ê"))
            {
				val += 10;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if ((unsigned char)*currentCharReal == 0xc3 && (unsigned char)currentCharReal[1] == 0xa8 && SameUTF(currentCharDict, (char *)"ê"))
            {
            	val += 5;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'i' && SameUTF(currentCharDict, (char *) "î"))
            {
				val += 1;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'o' && SameUTF(currentCharDict, (char *)"ô"))
            {
					val += 1;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'u' && SameUTF(currentCharDict, (char *)"û"))
            {
			 	val += 5;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'y' && *currentCharDict == 'l' && *nextCharDict == 'l')
            {
            	val += 10;
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'k' && *currentCharDict == 'q' && *nextCharDict == 'u')
            {
            	val += 10;
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'f' && *currentCharDict == 'p' && *nextCharDict == 'h')
            {
            	val += 5;
                dictinfo = resumeDict1;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 's' && *currentCharDict == 'c')
            {
            	val += 10;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 's' && SameUTF(currentCharDict, (char *)"ç"))
            {
            		val += 5;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
            if (*currentCharReal == 'c' && SameUTF(currentCharDict, (char *)"ç"))
            {
				val += 5;
                dictinfo = resumeDict;
                realinfo = resumeReal;
                continue;
            }
        }
        // probable transposition since swapping syncs up
        if (!strcmp(currentCharReal, nextCharDict) && !strcmp(nextCharReal, currentCharDict))
        {
            val += 10; // should be more expensive if next is not correct after transposition
			dictinfo = resumeDict1; 
            realinfo = resumeReal1;
            continue;
        }

        // probable mistyped letter since next matches up
        if (!strcmp(nextCharReal, nextCharDict))
        {
            val += 16; // more expensive if 2nd next is not correct after transposition
            if (*currentCharReal == 's' && *currentCharDict == 'z') val -= 5;
            else if (*currentCharReal == 'z' && *currentCharDict == 's') val -= 5;
            else if (IsVowel(*currentCharReal) && IsVowel(*currentCharDict))  val -= 6; //  low cost for switching a vowel 

            dictinfo = resumeDict; 
            realinfo = resumeReal;
            continue;
        }
        // probable excess letter by user since next matches up to current
        if (!strcmp(nextCharReal, currentCharDict))
        {
            val += 16;  // only delete 1 letter

            if (*priorCharDict == *currentCharReal) val -= 14; // low cost for dropping an excess repeated letter (wherre->where not wherry)
            else if (*currentCharReal == '-') val -= 14; //   very low cost for removing a hypen 

            dictinfo = resumeDict; // skip over letter we match momentarily
            realinfo = resumeReal1; // move on past our junk letter and match letter
            continue;
        }
        // probable missing letter by user since current matches up to nextdict
        if (!strcmp(nextCharDict, currentCharReal))
        {
            val += 16; // only add 1 letter
            // better to add repeated letter than to drop a letter
            if (*currentCharDict == *priorCharReal) val -= 6; // low cost for adding a repeated letter
            else if (*currentCharDict == '-') 
                val -= 14; // very low cost for adding a hyphen
            else if (*currentCharDict == 'e' && *nextCharDict == 'o') val -= 10; // yoman->yeoman
            dictinfo = resumeDict1; // skip over letter we match momentarily
            realinfo = resumeReal; // move on past our junk letter and match letter
            continue;
        }
    
        // complete mismatch with no understanding of why, just fix them and move on
        dictinfo = resumeDict; // skip over letter we match momentarily
        realinfo = resumeReal; // move on past our junk letter and match letter
        val += 16;
    }
    return val;
}

static char* StemSpell(char* word,unsigned int i,uint64& base)
{
    static char word1[MAX_WORD_SIZE];
    strcpy(word1,word);
    size_t len = strlen(word);

    bool isEnglish = (!stricmp(language, "english") ? true : false);
    bool isFrench = (!stricmp(language, "french") ? true : false);

	char* ending = NULL;
    char* best = NULL;
    
	//   suffixes
	if (len < 5){;} // too small to have a suffix we care about (suffix == 2 at min)
    else if (isEnglish && !strnicmp(word+len-3,(char*)"ing",3))
    {
        word1[len-3] = 0;
        best = SpellFix(word1,0,VERB); 
        base = VERB;
        if (best && FindWord(best,0,LOWERCASE_LOOKUP)) return GetPresentParticiple(best);
	}
    else if (isEnglish && !strnicmp(word + len - 3, (char*)"ies", 3))
    {
        word1[len - 3] = 'y';
        word1[len - 2] = 0;
        best = SpellFix(word1, 0, NOUN);
        if (best)
        {
            base = NOUN | NOUN_PLURAL;
            char* plu = GetPluralNoun(FindWord(best));
            return (plu) ? plu : NULL;
        }
    }
    else if (isEnglish && !strnicmp(word+len-2,(char*)"ed",2))
    {
        word1[len-2] = 0;
        best = SpellFix(word1,0,VERB); 
        if (best)
        {
			char* past = GetPastTense(best);
            base = VERB;
			return past ? past : NULL;
        }
    }
	else
	{
		unsigned int j = 0;
		char* suffix;
		if (isEnglish)
		{
			while ((suffix = stems[j].word))
			{
				uint64 kind = stems[j++].flags;
				size_t suffixlen = strlen(suffix);
				if (!strnicmp(word+len-suffixlen,suffix,suffixlen))
				{
					word1[len-suffixlen] = 0;
					best = SpellFix(word1,0,kind); 
					if (best) 
					{
						ending = suffix;
	                    base = stems[j].flags;
						break;
					}
				}
			}
		}
		else if (isFrench)
		{
			while ((suffix = stems_french[i].word))
			{
				uint64 kind = stems_french[i++].flags;
				size_t suffixlen = strlen(suffix);
				if (!strnicmp(word+len-suffixlen,suffix,suffixlen))
				{
					word1[len-suffixlen] = 0;
					best = SpellFix(word1,0,kind); 
					if (best) 
					{
						ending = suffix;
	                    base = stems_french[i].flags;
						break;
					}
				}
			}
		}
	}
	if (isEnglish && !ending && word[len-1] == 's')
    {
        word1[len-1] = 0;
        best = SpellFix(word1,0,VERB|NOUN); 
        if (best)
        {
			WORDP F = FindWord(best,0,(tokenControl & ONLY_LOWERCASE) ?  PRIMARY_CASE_ALLOWED : STANDARD_LOOKUP);
            if (F && F->properties & NOUN)
            {
                base = NOUN | NOUN_PLURAL;
                return GetPluralNoun(F);
            }
            base = VERB | VERB_PRESENT_3PS;
			ending = "s";
        }
   }
   if (ending)
   {
		strcpy(word1,best);
		strcat(word1,ending);
		return word1;
   }
   return NULL;
}

void CheckWord(char* originalWord, WORDINFO& realWordData, WORDP D, WORDP* choices, unsigned int& index, int& min)
{
    WORDINFO dictWordData;
    ComputeWordData(D->word, &dictWordData);
    int val = EditDistance(dictWordData, realWordData, min);

    // adjustments
    if (*D->word != *originalWord) ++val;  // starts not same
    if (D->word[strlen(D->word) - 1] != originalWord[strlen(originalWord) - 1]) ++val; // doesnt end the same
    if (D->internalBits & UPPERCASE_HASH) ++val; // lower case should win any tie against proper name

    if (val <= min && !(D->internalBits & HAS_SUBSTITUTE)) // as good or better
    {
       if (spellTrace) Log(STDUSERLOG, "    found: %s %d\r\n", D->word, val);
       if (val < min)
       {
            if (trace == TRACE_SPELLING) Log(STDUSERLOG, (char*)"    Better: %s against %s value: %d\r\n", D->word, originalWord, val);
            index = 0;
            min = val;
        }
        else if (val == min && trace == TRACE_SPELLING) Log(STDUSERLOG, (char*)"    Equal: %s against %s value: %d\r\n", D->word, originalWord, val);

        if (!(D->internalBits & BEEN_HERE))
        {
            choices[index++] = D;
            AddInternalFlag(D, BEEN_HERE);
        }
    }
}

char* SpellFix(char* originalWord,int start,uint64 posflags)
{
    bool isEnglish = (!stricmp(language, "english") ? true : false);
	if (spellTrace) Log(STDUSERLOG,"Correcting: %s:\r\n", originalWord);
	multichoice = false;
    char word[MAX_WORD_SIZE];
    MakeLowerCopy(word, originalWord);
	char word1[MAX_WORD_SIZE];
	MakeUpperCopy(word1, originalWord);

	WORDINFO realWordData;
    ComputeWordData(word, &realWordData);

	if (realWordData.bytelen >= 100 || realWordData.bytelen == 0) return NULL;
	if (IsDigit(*originalWord)) return NULL; // number-based words and numbers must be treated elsewhere
    char letterLow = *word;
	char letterHigh = *word1;
	bool hasUnderscore = (strchr(originalWord,'_')) ? true : false;
	bool isUpper = IsUpperCase(originalWord[0]);
	if (IsUpperCase(originalWord[1])) isUpper = false;	// not if all caps
	if (trace == TRACE_SPELLING) Log(STDUSERLOG,(char*)"Spell: %s\r\n",originalWord);

	//   Priority is to a word that looks like what the user typed, because the user probably would have noticed if it didnt and changed it. So add/delete  has priority over tranform
    WORDP choices[4000];
    WORDP bestGuess[4000];
    unsigned int index = 0;
    unsigned int bestGuessindex = 0;
    int min = 35; // allow 2 changes as needed
      
	uint64  pos = PART_OF_SPEECH;  // all pos allowed
    WORDP D;
    if (posflags == PART_OF_SPEECH && start < wordCount) // see if we can restrict word based on next word
    {
        D = FindWord(wordStarts[start+1],0,PRIMARY_CASE_ALLOWED);
		uint64 flags = (D) ? D->properties : (uint64)-1; //   if we dont know the word, it could be anything
		if ((flags & PART_OF_SPEECH) == PREPOSITION) pos &= -1 ^ (PREPOSITION | NOUN);   //   prep cannot be preceeded by noun or prep
		if (!(flags & (PREPOSITION | VERB | CONJUNCTION | ADVERB)) && flags & DETERMINER) pos &= -1 ^ (DETERMINER | ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER); //   determiner cannot be preceeded by noun determiner adjective
		if (!(flags & (PREPOSITION | VERB | CONJUNCTION | DETERMINER | ADVERB)) && flags & ADJECTIVE) pos &= -1 ^ (NOUN);
		if (!(flags & (PREPOSITION | NOUN | CONJUNCTION | DETERMINER | ADVERB | ADJECTIVE)) && flags & VERB) pos &= -1 ^ (VERB); //   we know all helper verbs we might be
		if (isEnglish && D && *D->word == '\'' && D->word[1] == 's' ) pos &= NOUN;    //   we can only be a noun if possessive - contracted 's should already be removed by now
    }
    if (posflags == PART_OF_SPEECH && start > 1)
    {
        D = FindWord(wordStarts[start-1],0,PRIMARY_CASE_ALLOWED);
        uint64 flags = (D) ? D->properties : (-1); // if we dont know the word, it could be anything
        if (flags & DETERMINER) pos &= -1 ^ (VERB|CONJUNCTION|PREPOSITION|DETERMINER);  
    }
    posflags &= pos; //   if pos types are known and restricted and dont match
	static int range[] = {0,-1,1,-2,2};
	for (unsigned int i = 0; i < 5; ++i)
	{
		if (i >= 3) break;
		MEANING offset = lengthLists[realWordData.charlen + range[i]];
		if (trace == TRACE_SPELLING) Log(STDUSERLOG,(char*)"\r\n  Begin offset %d\r\n",i);
		while (offset)
		{
			D = Meaning2Word(offset);
			offset = D->spellNode;
			if (PART_OF_SPEECH == posflags  && D->systemFlags & PATTERN_WORD){;} // legal generic match
			else if (!(D->properties & posflags)) continue; // wrong kind of word
			char* under = strchr(D->word,'_');
			// SPELLING lists have no underscore or space words in them
			if (hasUnderscore && !under) continue;	 // require keep any underscore
			if (!hasUnderscore && under) continue;	 // require not have any underscore
            
            CheckWord(originalWord, realWordData, D, choices, index, min);
			if (index > 3997) break; 
		}
	}
	// try endings ing, s, etc
	if (start && !index) // no stem spell if COMING from a stem spell attempt (start == 0) or we have a good guess already
	{
        uint64 flags = 0;
		char* stem = StemSpell(word,start,flags);
		if (stem) 
		{
            WORDP X = StoreWord(stem,flags); 
			if (X) choices[index++] = X;
		}
	}

    if (!index)  return NULL; 
    if (index > 1) multichoice = true;

	// take our guesses, and pick the most common or substitute (earliest learned or most frequently used) word
    uint64 commonmin = 0;
    bestGuess[0] = NULL;
	for (unsigned int j = 0; j < index; ++j) RemoveInternalFlag(choices[j],BEEN_HERE);
    if (index == 1) 
	{
		if (trace == TRACE_SPELLING) Log(STDUSERLOG,(char*)"    Single best spell: %s\r\n",choices[0]->word);
		return choices[0]->word;	// pick the one
	}
    for (unsigned int j = 0; j < index; ++j) 
    {
        uint64 common = choices[j]->systemFlags & COMMONNESS;
		// if we had upper case and we are spell checking to lower case,
		// DONT unless a detected substitution is allowed. dont want to lose unknown proper names
		if (isUpper && !(choices[j]->internalBits & UPPERCASE_HASH) && !(choices[j]->internalBits & HAS_SUBSTITUTE) && start != 1)
		{
			continue;
		}
		if (choices[j]->internalBits & UPPERCASE_HASH && index > 1) continue;	// ignore proper names for spell better when some other choice exists

		if (choices[j]->internalBits & HAS_SUBSTITUTE)
			common = 0xff10000000000000ULL;
		common |= choices[j]->systemFlags & AGE_LEARNED;

        if (common < commonmin) continue;
        if (common > commonmin) // this one is more common
        {
            commonmin = common;
            bestGuessindex = 0;
        }
        bestGuess[bestGuessindex++] = choices[j];
    }
	if (bestGuessindex) 
	{
        if (bestGuessindex > 1) multichoice = true;
		if (trace == TRACE_SPELLING) Log(STDUSERLOG,(char*)"    Pick spell: %s\r\n",bestGuess[0]->word);
		return bestGuess[0]->word; 
	}
	return NULL;
}

