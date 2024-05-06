#include "common.h"

#ifdef INFORMATION

This file covers routines that createand access a "dictionary entry" (WORDP) and the "meaning" of words(MEANING).

The dictionary acts as the central hash mechanism for accessing various kinds of data.

The dictionary consists of data imported from WORDNET 3.0 (copyright notice at end of file) + augmentations + system and script pseudo - words.

A word also gets the WordNet meaning ontology(->meanings & ->meaningCount).The definition of meaning in WordNet
is words that are synonyms in some particular context.Such a collection in WordNet is called a synset.

Since words can have multiple meanings(and be multiple parts of speech), the flags of a word are a summary
of all of the properties it might haveand it has a list of entries called "meanings".Each entry is a MEANING
and points to the circular list, one of which marks the word you land at as the synset head.
This is referred to as the "master" meaning and has the gloss(definition) of the meaning.The meaning list of a master node points back to
all the real words which comprise it.

Since WordNet has an ontology, its synsets are hooked to other synsets in various relations, particular that
of parentand child.ChatScript represents these as facts.The hierarchy relation uses the verb "is" and
has the child as subject and the parent as object.Such a fact runs from the master entries, not any of the actual
word entries.So to see if "dog" is an "animal", you could walk every meaning list of the word animaland
mark the master nodes they point at.Then you would search every meaning of dog, jumping to the master nodes,
then look at facts with the master node as subjectand the verb "is" and walk up to the object.If the object is
marked, you are there.Otherwise you take that object node as subjectand continue walking up.Eventually you arrive at
a marked node or run out at the top of the tree.

Some words DO NOT have a master node.Their meaning is defined to be themselves(things like pronouns or determiners), so
their meaning value for a meaning merely points to themselves.
The meaning system is established by building the dictionaryand NEVER changes thereafter by chatbot execution.
New words can transiently enter the dictionary for various purposes, but they will not have "meanings".

A MEANING is a reference to a specific meaning of a specific word.It is an index into the dictionary
(identifying the word) and an index into that words meaning list(identifying the specific meaning).
An meaning list index of 0 refers to all meanings of the word.A meaning index of 0 can also be type restricted
so that it only refers to noun, verb, adjective, or adverb meanings of the word.

Since there are only two words in WordNet with more than 63 meanings(breakand cut) we limit all words to having no
more than 63 meanings by discarding the excess meanings.Since meanings are stored most important first,
these are no big loss.This leaves room for the 5 essential type flags used for restricting a generic meaning.

Space for dictionary words comes from a common pool.Dictionary words are
allocated linearly forward in the pool.Strings have their own pool(the heap).
All dictionary entries are indexable as a giant array.

The dictionary separately stores uppercaseand lowercase forms of the same words(since they have different meanings).
There is only one uppercase form stored, so Unitedand UnItED would be saved as one entry.The system will have
to decide which case a user intended, since they may not have bothered to capitalize a proper noun, or they
may have shouted a lowercase noun, and a noun at the start of the sentence could be either proper or not.

Dictionary words are hashed as lower case, but if the word has an upper case letter it will be stored
in the adjacent higher bucket.Words of the basic system are stored in their appropriate hash bucket.
After the basic system is read in, the dictionary is frozen.This means it remembers the spots the allocation
pointers are at for the dictionaryand heap spaceand is using mark - release memory management.
The hash buckets are themselves dictionary entries, sharing space.After the basic layers are loaded,
new dictionary entries are always allocated.Until then, if the word hashs to an empty bucket, that bucket
becomes the dictionary entry being added.

We mark sysnet entries with the word& meaning number& POS of word in the dictionary entry.The POS not used explicitly by lots of the system
but is needed when seeing the dictionary definitions(:word) and if one wants to use pos - restricted meanings in a match or in keywords.

#endif
#define DICTSEARCHLIMIT 1000
bool exportdictionary = false;
bool warnedaboutbuild0 = false;
char current_language[150] = "";							// indicate current language used
char language_list[400];// indicate legal languages- comma separated
unsigned int wordhash;
bool hasUpperCharacters, hasUTF8Characters, hasSeparatorCharacters;
unsigned int fullhash;
unsigned int language_bits = LANGUAGE_UNIVERSAL; // current language used to mark/filter WORDP items
unsigned int languageIndex; // current language used to mark facts and index data by language
unsigned int max_language = LANGUAGE_1;
unsigned int max_fact_language = FACTLANGUAGE_1;
static WORDP startOfLanguage = NULL;
static WORDP endOfLanguage = NULL;
int worstDictAvail = 1000000;
bool dictionaryBitsChanged = false;
bool multidict = false;
unsigned int languageCount = 1;
HEAPREF propertyRedefines = NULL;	// property changes on locked dictionary entries
HEAPREF flagsRedefines = NULL;		// systemflags changes on locked dictionary entries
HEAPREF ongoingDictChanges = NULL;  // ability to revert dynamic changes
HEAPREF ongoingUniversalDictChanges = NULL;  // ability to revert dynamic changes
bool monitorDictChanges = false;
bool xbuildDictionary = false;				// indicate when building a dictionary
char dictionaryTimeStamp[50];		// indicate when dictionary was built
const char* mini = ""; // what language
unsigned int* hashbuckets = 0;
#define ALL_CONTROLBITS 0xFFFF000000000000ULL // 6 bits controls + other data


static unsigned char* writePtr;				// used for binary dictionary writes

// memory data
unsigned long maxHashBuckets = MAX_HASH_BUCKETS;
bool setMaxHashBuckets = false;
uint64 maxDictEntries = MAX_DICTIONARY;

MEANING posMeanings[64];				// concept associated with propertyFlags of WORDs
MEANING sysMeanings[64];				// concept associated with systemFlags of WORDs

#include <map>
using namespace std;
std::map <WORDP, WORDP> irregularNouns;
std::map <WORDP, WORDP> irregularVerbs;
std::map <WORDP, WORDP> irregularAdjectives;
std::map <WORDP, WORDP> canonicalWords;
std::map <WORDP, int> wordValues; // per volley
std::map <WORDP, MEANING> backtracks; // per volley
std::map <WORDP, int> countData;

static void ReadAsciiDictionary();

HEAPREF concepts[MAX_SENTENCE_LENGTH];  // concept chains per word
HEAPREF topics[MAX_SENTENCE_LENGTH];  // topics chains per word

bool fullDictionary = true;				// we have a big master dictionary, not a mini dictionary
bool primaryLookupSucceeded = false;

void LoadRawDictionary(int mini);

bool TraceHierarchyTest(int x)
{
	if (!(x & TRACE_HIERARCHY)) return false;
	x &= -1 ^ (TRACE_ON | TRACE_ALWAYS | TRACE_HIERARCHY | TRACE_ECHO);
	return x == 0; // must exactly have been trace hierarchy
}

MEANING GetMeaning(WORDP D, int index)
{
	MEANING* meanings = GetMeanings(D);
	if (meanings) return meanings[index];
	else return MakeMeaning(D); // switch to generic non null meaning
}

unsigned int LanguageRestrict(char* word)
{
	char* restrict = strstr(word, "~l");
	if (restrict && restrict[2] >= '0' && restrict[2] <= '7')
	{
		*restrict = 0;
		unsigned int bits = (restrict[2] - '0') << LANGUAGE_SHIFT;
		if (!multidict && bits > max_language) 
			return language_bits;
		return bits;
	}
	return language_bits;
}

void SetTried(WORDP D, int value)
{
	triedData[D] = value;
}

bool IsUniversal(char* word, uint64 properties)
{
	char c = *word;
	if (c == USERVAR_PREFIX || c == FUNCTION_PREFIX || c == SYSVAR_PREFIX || c == TOPICCONCEPT_PREFIX
		|| c == '+' || c == '-' || c == '.') return true;
	if (c == MATCHVAR_PREFIX && (!word[1] || IsDigit(word[1]))) return true;
	if (IsValidJSONName(word)) return true;
	if (!strcmp(word,"member") || !strcmp(word, "is")) return true; // system verbs
	if (properties & PUNCTUATION_BITS) return true;
	if (IsPureNumber(word))	return true;
	return false;
}

bool IsValidLanguage(WORDP D)
{
	if (!D || !D->word || SUPERCEDED(D)) return false;
	if (!multidict || D->internalBits & UNIVERSAL_WORD) return true;

	// language independent ideas (numbers, concepts, variables, functions, systemvars )
	// should do international currencies but not doing yet
	unsigned int bits = D->internalBits & LANGUAGE_BITS;
	if (!bits) return true; // universal cs words match always
	if (IsUniversal(D->word, 0)) return true;
	if (bits == language_bits) return true;
	// matches language or we match ALL!
	return false;
}

void RemoveConceptTopic(HEAPREF list[256], WORDP D, int index)
{ // these lists are only for script to easily access, engine doesnt use them itself
	HEAPREF at = list[index];
	HEAPREF  prior = NULL;
	while (at)
	{
		uint64 m;
		uint64* currentEntry = (uint64*)at;
		at = UnpackHeapval(at, m, discard);
		if (((uint64)D) == m)
		{
			if (prior == NULL) list[index] = at; // list head
			else ((uint64*)prior)[0] = currentEntry[0]; // splice this out
			return;	// assumed only on list once
		}
		prior = at;
	}
}

void Add2ConceptTopicList(HEAPREF list[256], WORDP D, int start, int end, bool unique)
{
	if (unique)
	{
		HEAPREF at = list[start];
		while (at)
		{
			uint64 D1;
			at = UnpackHeapval(at, D1, discard, discard);
			if (D1 == (uint64)D) return;	// already on list
		}
	}
	// concepts[i] and topics[i]  lists of word indices
	list[start] = AllocateHeapval(HV1_WORDP,list[start], (uint64)D, 0, 0);
}

void ClearHeapThreads()
{
	memoryMarkThreadList = NULL;
	userVariableThreadList = NULL;
	savedSentencesThreadList = NULL;
	propertyRedefines = NULL;	// property changes on locked dictionary entries
	flagsRedefines = NULL; // system flag changes on locked dictionary entries
	factThreadList = NULL;
	rulematches = NULL;
	patternwordthread = NULL;
    variableChangedThreadlist = NULL;
    memoryVariableThreadList = NULL; // list of variable changes for each mark
    memoryVariableChangesThreadList = NULL; // list of original values in this mark
    matchedWordsList = NULL;
    for (int i = 1; i < MAX_SENTENCE_LENGTH; ++i) concepts[i] = topics[i] = NULL;
	// kernelVariableThreadList
	// botVariableThreadList;
}

void ClearVolleyWordMaps()
{
	wordValues.clear();
	backtracks.clear();
	ClearWhereInSentence();
}

void ClearWordMaps() // both static for whole dictionary and dynamic per volley
{
	irregularNouns.clear();
	irregularVerbs.clear();
	irregularAdjectives.clear();
	canonicalWords.clear();
	triedData.clear(); // prevent document reuse
	ClearVolleyWordMaps();
}

void ClearWhereAt(int where) // remove all concepts and markings at this slot in sentence
{
	WORDP D;
	if (trace & TRACE_HIERARCHY)
	{
		HEAPREF list = concepts[where];
		while (list)
		{
			uint64 val;
			list = UnpackHeapval(list, val, discard, discard);
			WORDP X = (WORDP)val;
			Log(USERLOG, " %s ", X->word);
		}
		list = topics[where];
		while (list)
		{
			uint64 val;
			list = UnpackHeapval(list, val, discard, discard);
			WORDP X = (WORDP)val;
			Log(USERLOG, " %s ", X->word);
		}
	}
	concepts[where] = NULL; // drop memory list at this slot
	topics[where] = NULL; // drop memory list
	WORDP holdlist[10000];
	HEAPINDEX holdint[10000];
	int mark[10000];
	memset(mark, 0, sizeof(mark));
	unsigned int index = 0;
	bool changed = false;

	// copy because we will rewrite triedData.
	map<WORDP, HEAPINDEX>::iterator it1;
	for (it1 = triedData.begin(); it1 != triedData.end(); ++it1)
	{
		D = it1->first;
		unsigned int x = it1->second;
		holdint[index] = x;
		holdlist[index++] = D;
	}

	for (unsigned int item = 0; item < index; ++item)
	{
		HEAPINDEX access = holdint[item]; // heap access
		char* data = Index2Heap(access);
		if (data == 0) continue;

		unsigned char* lcltriedData = (unsigned char*)(data + 8);	// skip over 64bit tried by meaning field
		int i;
		int counter = 0;
		for (i = 0; i < MAXREFSENTENCE_BYTES; i += REF_ELEMENT_SIZE) // 6 bytes per entry x 300 entries
		{
			unsigned char start = lcltriedData[i];
			if (start == where) // modify it
			{
				if (!mark[item]) // change over to a copy (dont harm original)
				{
					int newaccess = CopyWhereInSentence(access);
					holdint[item] = newaccess;
					mark[item] = 1;
					changed = true;
					data = Index2Heap(newaccess);
					lcltriedData = (unsigned char*)(data + 8);	// skip over 64bit tried by meaning field
				}

				int remain = MAXREFSENTENCE_BYTES - i - REF_ELEMENT_SIZE;
				if (remain == 0) // just chop it off
				{
					lcltriedData[i] = 0xff;
					lcltriedData[i + 1] = 0xff;
					break;
				}
				memmove(lcltriedData + i, lcltriedData + i + REF_ELEMENT_SIZE, remain); // remove element
				// insert end marker at end
				lcltriedData[MAXREFSENTENCE_BYTES - REF_ELEMENT_SIZE] = 0xff;
				lcltriedData[MAXREFSENTENCE_BYTES - REF_ELEMENT_SIZE + 1] = 0xff;
				if (++counter > 100) // precaution 
					break;
				i -= REF_ELEMENT_SIZE;
				continue;    // check next one
			}
			else if (start > where) break;
		}
	}

	if (changed)
	{
		for (unsigned int item = 0; item < index; ++item)
		{
			if (!mark[item]) continue;
			D = holdlist[item];
			HEAPINDEX access = holdint[item]; // heap access
			triedData[D] = access;   // replace now
		}
	}
}

void ClearWordWhere(WORDP D, int at)
{
	triedData.erase(D);

	// also remove from concepts/topics lists
	if (at == -1)
	{
		for (unsigned int i = 1; i <= (unsigned int)wordCount; ++i)
		{
			RemoveConceptTopic(concepts, D, i);
			RemoveConceptTopic(topics, D, i);
		}
	}
}

WORDP GetLanguageWord(WORDP D)
{ // if universal word node and want specific language, get current language form of it
	if (languageIndex == 0) return D; // we are requesting universal
	if (!D->foreignFlags) return D;
	WORDP E = D->foreignFlags[languageIndex - 1]; // Compaq is in spanish and german, but not in english, so have to stay in the unversal whichever
	return (E) ? E : D;
}

void SetFactBack(WORDP D, MEANING M)
{
	if (GetFactBack(D) == 0)
	{
		backtracks[D] = M;
	}
}

MEANING GetFactBack(WORDP D)
{
	std::map<WORDP, MEANING>::iterator it;
	it = backtracks.find(D);
	return (it != backtracks.end()) ? it->second : 0;
}

void ClearBacktracks()
{
	backtracks.clear();
}

void SetPlural(WORDP D, MEANING M)
{
	if (!D) return;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	irregularNouns[D] = dictionaryBase + M; // must directly assign since word on load may not exist
}

void SetComparison(WORDP D, MEANING M)
{
	if (!D) return;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	irregularAdjectives[D] = dictionaryBase + M; // must directly assign since word on load may not exist
}

void SetTense(WORDP D, MEANING M)
{
	if (!D) return;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	irregularVerbs[D] = dictionaryBase + M; // must directly assign since word on load may not exist
}

void SetCanonical(WORDP D, MEANING lemma)
{
	if (!D) return;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	canonicalWords[D] = dictionaryBase + lemma;  // must directly assign since word on load may not exist
}

WORDP RawCanonical(WORDP D)
{
	if (!D) return NULL;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	std::map<WORDP, WORDP>::iterator it;
	it = canonicalWords.find(D);
	if (it == canonicalWords.end()) return NULL;
	return it->second;
}

WORDP GetCanonical(WORDP D, uint64 kind)
{
	if (!D) return NULL;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	std::map<WORDP, WORDP>::iterator it;
	it = canonicalWords.find(D);
	if (it == canonicalWords.end()) return NULL;
	WORDP E = it->second;
	if (*E->word != '`'  || kind == (uint64) -1) return E; // normal english canonical or simple canonical of foreign (not multiple choice). 0 kind means want all

	// foreign canonicals have multiple choices. This code only picks first one if no type given. Other code may decide more complex, but generally treetagger will do that
	char word[MAX_WORD_SIZE];
	char type[MAX_WORD_SIZE];
	strcpy(word, E->word + 1);
	char* lemma = word;
	char* tags = strrchr(word, '`');
	if (tags) *tags++ = 0; // type decriptors
	WORDP lemmachoice = NULL;
	int lemmacount = 0;
	// walk list of pos tags and simultaneously walk the list of lemmas
	while (tags && *tags && (tags = ReadCompiledWord(tags, type)))
	{
		char* split = strchr(lemma, '`');
		if (split) *split = 0;
		// if we have multiple meanings unresolved, we cannot pick a lemma based on type
		if (*type == 'V' && kind & VERB)
		{
			lemmachoice = StoreWord(lemma, AS_IS|VERB);
			++lemmacount;
		}
		else if (*type == 'N' && kind & NOUN)
		{
			lemmachoice = StoreWord(lemma, AS_IS|NOUN);
			++lemmacount;
		}
		else if (*type == 'A' && type[2] == 'J' && kind & ADJECTIVE)
		{
			lemmachoice = StoreWord(lemma, AS_IS| NOUN);
			++lemmacount;
		}
		if (split)
		{
			*split = '`';
			lemma = split + 1;
		}
		tags = SkipWhitespace(tags);
	}

	if (lemmacount == 1) return lemmachoice;

	// if we dont know the type, we dont know which lemma to pick
	// So pick one different from original, so we get broader mark coverage
	char* next = word;
	while (*next)
	{
		char* end = strchr(next, '`'); // multiple choice?
		if (end)
		{
			WORDP X = FindWord(next, end - next,PRIMARY_CASE_ALLOWED);
			if (X && stricmp(X->word, D->word)) return X; // prefer a different one
			next = end + 1;
		}
		else
		{
			WORDP X = FindWord(next);
			if (X) return X; 
			break;
		}
	}
	return StoreWord(word, AS_IS);
}

WORDP GetTense(WORDP D)
{
	if (!D) return NULL;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1 ];
	std::map<WORDP, WORDP>::iterator it;
	it = irregularVerbs.find(D);
	if (it == irregularVerbs.end()) return NULL;
	return it->second;
}

WORDP GetPlural(WORDP D)
{// given singular returns plural and visa versa
	if (!D) return NULL;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	std::map<WORDP, WORDP>::iterator it;
	it = irregularNouns.find(D);
	if (it == irregularNouns.end()) return NULL;
	return  it->second;
}

WORDP GetComparison(WORDP D)
{
	if (!D) return NULL;
	if (D->foreignFlags) D = D->foreignFlags[languageIndex - 1];
	std::map<WORDP, WORDP>::iterator it;
	it = irregularAdjectives.find(D);
	if (it == irregularAdjectives.end()) return NULL;
	return it->second;
}

void SetWordValue(WORDP D, int x)
{ // will be stored on a universal
	if (!D) return;
	wordValues[D] = x;
}

int GetWordValue(WORDP D)
{
	std::map<WORDP, int>::iterator it;
	it = wordValues.find(D);
	return (it != wordValues.end()) ? it->second : 0;
}

// start and ends of space allocations
WORDP dictionaryBase = NULL;			// base of allocated space that encompasses dictionary, heap space, and meanings
WORDP dictionaryFree = NULL;				// current next dict space available going forward (not a valid entry)

// return-to values for layers
WORDP dictionaryPreBuild[NUMBER_OF_LAYERS + 1];
char* heapPreBuild[NUMBER_OF_LAYERS + 1];
// build0Facts 

// return-to values after build1 loaded, before user is loaded
WORDP dictionaryLocked = NULL;
FACT* factLocked = 0;
char* stringLocked;

// format of word looked up
uint64 verbFormat;
uint64 nounFormat;
uint64 adjectiveFormat;
uint64 adverbFormat;

// dictionary ptrs for these words
WORDP Dplacenumber;
WORDP Dpropername;
MEANING Mphrase;
MEANING MabsolutePhrase;
MEANING MtimePhrase;
WORDP Dclause;
WORDP Dverbal;
WORDP Dmalename, Dfemalename, Dhumanname;
WORDP Dtime;
WORDP Dunknown;
WORDP Dchild, Dadult;
WORDP Dtopic;
MEANING Mchatoutput;
MEANING Mburst;
MEANING Mpending;
MEANING Mkeywordtopics;
MEANING Mconceptlist;
MEANING Mmoney;
MEANING Mintersect;
MEANING MgambitTopics;
MEANING MadjectiveNoun;
MEANING Mnumber;
WORDP Dpronoun;
WORDP Dadjective;
WORDP Dauxverb;
WORDP DunknownWord;

static char* predefinedSets[] = //  some internally mapped concepts not including emotions from LIVEDATA/interjections
{
	 (char*)"~repeatme",(char*)"~repeatinput1",(char*)"~repeatinput2",(char*)"~repeatinput3",(char*)"~repeatinput4",(char*)"~repeatinput5",(char*)"~repeatinput6",(char*)"~uppercase",(char*)"~utf8",(char*)"~sentenceend",
	(char*)"~pos",(char*)"~sys",(char*)"~grammar_role",(char*)"~daynumber",(char*)"~yearnumber",(char*)"~dateinfo",(char*)"~formatteddate",(char*)"~email_url",(char*)"~fahrenheit",(char*)"~celsius",(char*)"~kelvin",
	(char*)"~kindergarten",(char*)"~grade1_2",(char*)"~grade3_4",(char*)"~grade5_6",(char*)"~twitter_name",(char*)"~hashtag_label",(char*)"~filename",
	(char*)"~shout",(char*)"~distance_noun_modify_adverb",
	(char*)"~distance_noun_modify_adjective",(char*)"~modelnumber",
	(char*)"~passive_verb",
	(char*)"~time_noun_modify_adverb",
	(char*)"~time_noun_modify_adjective",
	(char*)"~capacronym",(char*)"~emoji",
	NULL
};

void RestorePropAndSystem(char* stringUsed)
{
	uint64 item;
	while (propertyRedefines) // must release these
	{
		uint64 properties;
		if (stringUsed && propertyRedefines >= stringUsed) break;	// not part of this freeing
		propertyRedefines = UnpackHeapval(propertyRedefines, item, properties, discard);
		WORDP D = (WORDP)item;
		D->properties = properties;
	}
	while (flagsRedefines) // must release these
	{
		uint64 properties;
		if (stringUsed && flagsRedefines >= stringUsed) break;	// not part of this freeing
		flagsRedefines = UnpackHeapval(flagsRedefines, item, properties, discard);
		WORDP D = (WORDP)item;
		D->systemFlags = properties;
	}
}

void DictionaryRelease(WORDP until, char* stringUsed)
{
	RestorePropAndSystem(stringUsed);
	ongoingDictChanges = NULL; // discard user tracked changes since we have our own essential tracking
	ongoingUniversalDictChanges = NULL;
	if (until) while (dictionaryFree > until) DeleteDictionaryEntry(--dictionaryFree); //   remove entry from buckets
	lastheapfree = heapFree = stringUsed;
}

char* UseDictionaryFile(const char* name,const char* list)
{
	static char junk[100];
	if (*mini) sprintf(junk, (char*)"DICT/%s", mini);
	else if (!*current_language) sprintf(junk, (char*)"%s", (char*)"DICT");
	else if (!name) sprintf(junk, (char*)"DICT/%s", current_language);
	else if (list && strchr(list, ',')) sprintf(junk, (char*)"DICT"); // use global conglomerate
	else sprintf(junk, (char*)"DICT/%s", current_language);
	MakeDirectory(junk); // if it doesnt exist
	if (name && *name)
	{
		strcat(junk, (char*)"/");
		strcat(junk, name);
	}
	return junk;
}

MEANING FindChild(MEANING who, int n)
{ // GIVEN SYNSET
	FACT* F = GetObjectNondeadHead(who);
	unsigned int index = Meaning2Index(who);
	while (F)
	{
		FACT* at = F;
		F = GetObjectNondeadNext(F);
		if (at->verb != Mis) continue;
		if (index && at->object != who) continue;	// not us
		if (--n == 0)   return at->subject;
	}
	return 0;
}

bool ReadForeignPosTags(const char* fname)
{
	FILE* in = FopenReadOnly(fname);
	if (!in) return false;
	char word[MAX_WORD_SIZE];
	char wordx[MAX_WORD_SIZE];
	*word = '_';	// marker to keep any collision away from foreign pos
	*wordx = '~';   // corresponding concept
	while (ReadALine(readBuffer, in) >= 0)  // foreign name followed by bits of english pos
	{
		char* ptr = ReadCompiledWord(readBuffer, word + 1);
		if (!word[1] || word[1] == '#')
			continue;
		uint64 flags = 0;
		char flag[MAX_WORD_SIZE];
		while (*ptr) // get english pos values of this tag
		{
			ptr = ReadCompiledWord(ptr, flag);
			if (!*flag || *flag == '#') break;
			uint64 val = FindPropertyValueByName(flag);
			if (!val)
			{
				(*printer)("Unable to find flag %s\r\n", flag);
			}
			flags |= val;
		}
		StoreWord(word, flags); // _foreignpos gets bits
		MakeLowerCopy(wordx + 1, word + 1); // force the name to lowercase because the name might not be valid, e.g. DET:ART
		BUILDCONCEPT(wordx);
	}
	FClose(in);
	// Also store the unknown tag, just in case the pos tagger fails to attach anything
	strcpy(word + 1, (char*)"unknown-tag");
	StoreWord(word, 0);
	*word = '~'; // corresponding concept
	BUILDCONCEPT(word);
	return true;
}

unsigned char BitCount(uint64 n)
{
	unsigned char count = 0;
	while (n)
	{
		count++;
		n &= (n - 1);
	}
	return count;
}

uint64 GetProperties(WORDP D)
{ // language index of this word may not match currently language ONLY if we are universal node
	if (!D) return NULL;
	if (D->foreignFlags) D = D->foreignFlags[GET_FOREIGN_INDEX(D)];
	return D->properties;
}

void SetProperties(WORDP D,uint64 properties)
{ // language index of this word may not match currently language ONLY if we are universal node
	if (!D) return;
	if (D->foreignFlags) D = D->foreignFlags[GET_FOREIGN_INDEX(D)];
	D->properties = properties;
}

uint64 GetSystemFlags(WORDP D)
{ // language index of this word may not match currently language ONLY if we are universal node
	if (!D) return NULL;
	if (D->foreignFlags) D = D->foreignFlags[GET_FOREIGN_INDEX(D)];
	return D->systemFlags;
}

void SetSystemFlags(WORDP D, uint64 flags)
{ // language index of this word may not match currently language ONLY if we are universal node
	if (!D) return;
	if (D->foreignFlags) D = D->foreignFlags[GET_FOREIGN_INDEX(D)];
	 D->systemFlags = flags;
}

WORDP GetSubstitute(WORDP D)
{ // language index of this word may not match currently language ONLY if we are universal node
	if (!D) return NULL;
	int index = GET_FOREIGN_INDEX(D);
	WORDP E = D;
	if (E->foreignFlags) E = D->foreignFlags[index];
	if (!E) 
		return NULL;
	return (E->systemFlags & HAS_SUBSTITUTE) ? E->w.substitutes : NULL;
}

void BuildShortDictionaryBase();

static void EraseFile(const char* file,bool tmp)
{
	char word[MAX_WORD_SIZE];
	FILE* out;
	if (tmp)
	{
		sprintf(word, "tmp/%s", file);
		out = FopenUTF8Write(word);
	}
	else out = FopenUTF8Write(UseDictionaryFile(file));
	FClose(out);
}

void ClearDictionaryFiles(bool tmp)
{
	char buffer[MAX_WORD_SIZE];
	remove(UseDictionaryFile((char*)"dict.bin"));
	remove(UseDictionaryFile((char*)"facts.bin"));
	EraseFile((char*)"dict.bin",tmp); //   create but empty file
	EraseFile((char*)"facts.bin",tmp); //   create but empty file
	unsigned int i;
	for (i = 'a'; i <= 'z'; ++i)
	{
		sprintf(buffer, (char*)"%c.txt", i);
		EraseFile(buffer,tmp); //   create but empty file
	}
	for (i = '0'; i <= '9'; ++i)
	{
		sprintf(buffer, (char*)"%c.txt", i);
		EraseFile(buffer,tmp); //   create but empty file
	}
	EraseFile("other.txt", tmp); //   create but empty file
}

static const char* GetLanguage(WORDP D)
{
	if (!D) return current_language;
	char* comma = strchr(language_list, ','); 
	unsigned int n = GET_LANGUAGE_INDEX(D);
	if (!n || !comma) 
	{
		return (comma) ? "UNIVERSAL" : current_language;
	}
	char list[MAX_WORD_SIZE];
	strcpy(list, language_list);
	static char languages[20][100];
	memset(languages, 0, sizeof(languages));
	unsigned int x = 1;
	char* ptr = list;
	while ((comma = strchr(ptr,',')))
	{
		*comma = 0;
		strcpy(languages[x++], ptr);
		ptr = comma + 1;
	}
	strcpy(languages[x++], ptr);
	if (n >= 1 && n <= languageCount) return languages[n];
	else return "unknown language";
}

bool SetLanguage(char* arg1)
{
	if (!arg1 || !*arg1) return false;
	
	char lang[100];
	arg1 = TrimSpaces(arg1);
	MakeUpperCopy(lang, arg1);
	if (!stricmp(lang, "UNIVERSAL"))
	{
		languageIndex = language_bits = LANGUAGE_UNIVERSAL;
		strcpy(current_language, "UNIVERSAL");
		return true;
	}
	
	char copy[200];
	strcpy(copy, language_list);
	char* which = strstr(copy, lang);
	if (!which) // is it a legal choice?
	{
		if (compiling) BADSCRIPT("Illegal set language %s",arg1)
		return false;
	}
	// eg language=english,spanish,german,japanese, chinese
	strcpy(current_language, lang);
	if (!stricmp(lang, "GERMAN") || !stricmp(lang, "SPANISH") || !stricmp(lang, "FRENCH"))
	{
		numberStyle = FRENCH_NUMBERS;
		numberComma = '.';
		numberPeriod = ',';
	}
	else
	{
		numberStyle = AMERICAN_NUMBERS; // ignores indian
		numberComma = ',';
		numberPeriod = '.';
	}

	if (!multidict) 
	{ 
	}
	else
	{
		languageIndex = 1;
		*which = 0; // hide all language at and after
		which = copy - 1;
		// language=english,spanish,german,japanese
		while ((which = strchr(++which, ','))) 
			++languageIndex; // how many commas before
		language_bits = languageIndex << LANGUAGE_SHIFT; // could have up to 7 languages
	}
	return true;
} 

static void WriteDictionaryReference(char* label, WORDP D, FILE* out)
{
	if (!D) return;
	if (D->internalBits & DELETED_MARK) return;	// ignore bad links
	fprintf(out, (char*)"%s=%s ", label, D->word);
}

void WriteTextEntry(WORDP D,bool tmp)
{
	char word[MAX_WORD_SIZE];
	MakeLowerCopy(word, D->word);
	// choose appropriate subfile
	char c = *word;
	char name[40];
	if (IsDigit(c)) sprintf(name, (char*)"%c.txt", c); //   main real dictionary
	else if (!IsLowerCase(c) || (unsigned char)c > 127) 
		sprintf(name, (char*)"%s", (char*)"other.txt"); //   main real dictionary
	else sprintf(name, (char*)"%c.txt", c);//   main real dictionary
	FILE* out;
	if (!tmp) out = FopenUTF8WriteAppend(UseDictionaryFile(name));
	else
	{
		char hold[MAX_WORD_SIZE];
		sprintf(hold, "tmp/%s", name);
		out = FopenUTF8WriteAppend(hold);
	}
	if (!out)  myexit((char*)"Dict write failed");
	if (!stricmp(current_language,"spanish") && RawCanonical(D)) // we were given a lemma for it
	{
		WORDP entry, canonical;
		uint64 sysflags;
		if (ComputeSpanish(2, D->word, entry, canonical, sysflags))
		{

			// if canonical and we are different and canonical matches what we expected
			if (!stricmp(canonical->word, RawCanonical(D)->word) && stricmp(canonical->word,D->word))
			{
				bool discard = false;
				char* type = "";
				// no other meaning to worry about with this conjugation and we already know the lemma for it and can detect it
				if (D->properties & (VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST | VERB_PAST_PARTICIPLE | VERB_PRESENT_PARTICIPLE)
					&& !(D->properties & (NOUN | ADVERB | ADJECTIVE | PREPOSITION | CONJUNCTION))) {
					discard = true;
					type = "verb";
				}
				else if (D->properties & NOUN_PLURAL && !(D->properties & (VERB | ADVERB | ADJECTIVE | PREPOSITION | CONJUNCTION)))
				{
					discard = true;
					type = "noun";
				}
				if (discard)
				{
					FClose(out);
					printf("discard %s %s\r\n", D->word,type);
					return; // already handled
				}
			}
		}
	}

	//   write out the basics name ( meaningcount idiomcount)
	fprintf(out, (char*)" %s ( ", D->word); // marker for termination of word and some other things
	int count = GetMeaningCount(D);
	if (count) fprintf(out, (char*)"meanings=%d ", count);

	// check valid glosses (sometimes we have troubles that SHOULD have been found earlier)
	int ngloss = 0;
	if (!(D->internalBits & CONDITIONAL_IDIOM)) // glosses lose out to conditional idiom data
	{
		for (int i = 1; i <= count; ++i)
		{
			if (GetGloss(D, i)) ++ngloss;
		}
		if (ngloss) fprintf(out, (char*)"glosses=%d ", ngloss);
		if (ngloss != GetGlossCount(D))
		{
			ReportBug((char*)"Bad gloss count for %s\r\n", D->word);
		}
	}
	//   now do the dictionary bits into english
	char flags[MAX_WORD_SIZE];
	WriteDictionaryFlags(D, flags);
	if (*flags) fprintf(out, "%s", flags);
	if (D->internalBits & CONDITIONAL_IDIOM)
		fprintf(out, (char*)" CONDITIONAL_IDIOM poscondition=%s ", D->w.conditionalIdiom->word);
	fprintf(out, (char*)"%s", (char*)") ");

	//   these must have valuable ->properties on them
	WriteDictionaryReference((char*)"conjugate", GetTense(D), out);
	WriteDictionaryReference((char*)"plural", GetPlural(D), out);
	WriteDictionaryReference((char*)"comparative", GetComparison(D), out);
	if (RawCanonical(D)) WriteDictionaryReference((char*)"lemma", RawCanonical(D), out);

	//   show the meanings, with illustrative gloss

	fprintf(out, (char*)"\r\n");
	//   now dump the meanings and their glosses
	for (int i = 1; i <= count; ++i)
	{
		MEANING M = GetMeaning(D, i);
		fprintf(out, (char*)"    %s ", WriteMeaning(M, true)); // words are small
		if (M & SYNSET_MARKER) //   facts for this will be OUR facts
		{
			M = MakeMeaning(D, i); // our meaning id
			FACT* F = GetSubjectNondeadHead(D);
			while (F)
			{
				if (M == F->subject && F->verb == Mis) // show up path as information only
				{
					fprintf(out, (char*)"(^%s) ", WriteMeaning(F->object));  // small word. shows an object that we link to up (parent)
					break;
				}
				F = GetSubjectNondeadNext(F);
			}
		}
		if (D->internalBits & CONDITIONAL_IDIOM) fprintf(out, (char*)"\r\n");
		else
		{
			char* gloss = GetGloss(D, i);
			if (gloss == NULL) gloss = "";
			fprintf(out, (char*)"%s\r\n", gloss);
		}
	}

	FClose(out);
}

void ExportCurrentDictionary()
{	
	if (!startOfLanguage)
	{
		printf("erase dict.bin first, then try again. and clear TOPIC/");
		return;
	}
	ClearDictionaryFiles(true);
	exportdictionary = true;
	WORDP D = startOfLanguage - 1;
	while (++D < endOfLanguage) WriteTextEntry(D, true);
	exportdictionary = false;
}

void WriteDictionary(WORDP D,uint64 junk)
{
	if (D->internalBits & DELETED_MARK) return;
	if (*D->word == USERVAR_PREFIX && D->word[1]) return;	// var never and money never, but let $ punctuation through
	unsigned int internal = D->internalBits;
	RemoveInternalFlag(D, UTF8 | UPPERCASE_HASH | DEFINES);  // remove these
	WriteTextEntry(D);
	D->internalBits = internal;
}

void BuildDictionary(char* label)
{
	xbuildDictionary = true;
	int miniDict = 0;
	char word[MAX_WORD_SIZE];
	mini = current_language;
	char* ptr = ReadCompiledWord(label, word);
	bool makeBaseList = false;
	if (!stricmp(word, (char*)"wordnet")) // the FULL wordnet dictionary w/o synset removal
	{
		miniDict = -1;
		ReadCompiledWord(ptr, word);
		mini = "WORDNET";
	}
	else if (!stricmp(word, (char*)"basic"))
	{
		miniDict = 1;
		makeBaseList = true;
		FClose(FopenUTF8Write((char*)"RAWDICT/basicwordlist.txt"));
		maxHashBuckets = 10000;
		setMaxHashBuckets = true;
		mini = "BASIC";
	}
	else if (!stricmp(word, (char*)"layer0") || !stricmp(word, (char*)"layer1")) // a mini dictionary
	{
		miniDict = !stricmp(word, (char*)"layer0") ? 2 : 3;
		mini = (miniDict == 2) ? (char*)"LAYER0" : (char*)"LAYER1";
		maxHashBuckets = 10000;
		setMaxHashBuckets = true;
	}
	else if (stricmp(current_language, (char*)"english")) // a foreign dictionary
	{
		miniDict = 6;
		maxHashBuckets = 10000;
		setMaxHashBuckets = true;
	}
	UseDictionaryFile(NULL);

	PartiallyCloseSystem();

	InitStackHeap();
	InitFacts(); // must do facts before dict since dict defines system facts
	InitUniversalTextUtilities(); // also part of basic system before a build
	InitDictionary();
	WalkLanguages(InitTextUtilitiesByLanguage); // also part of basic system before a build
	InitUserCache();

	LoadRawDictionary(miniDict);
	if (miniDict && miniDict != 6) StoreWord((char*)"minidict"); // mark it as a mini dictionary

	// dictionary has been built now
	(*printer)((char*)"%s", (char*)"Dumping dictionary\r\n");
	ClearDictionaryFiles();
	WalkDictionary(WriteDictionary);
	if (makeBaseList) BuildShortDictionaryBase(); // write out the basic dictionary

	remove(UseDictionaryFile((char*)"dict.bin")); // invalidate cache of dictionary, forcing binary rebuild later
	myBot = 0;
	FILE* out = WriteFacts(FopenUTF8Write(UseDictionaryFile((char*)"facts.txt")), factBase);
	FClose(out);
	sprintf(logFilename, (char*)"%s/build_log.txt", usersfolder); // all data logged here by default
	out = FopenUTF8Write(logFilename);
	FClose(out);
	(*printer)((char*)"dictionary dump complete %d\r\n", miniDict);

	echo = true;
	xbuildDictionary = false;
	CreateSystem();
}

void InitDictionary()
{
	warnedaboutbuild0 = false;

	// read what the default dictionary wants as hash if parameter didnt set it
	if (!setMaxHashBuckets)
	{
		FILE* in = FopenStaticReadOnly(UseDictionaryFile((char*)"dict.bin"));  // DICT
		if (in)
		{
			unsigned int check = Read32(in);  // checkstamp
			if (check == CHECKSTAMP) maxHashBuckets = Read32(in); // bucket size used by dictionary file
			FClose(in);
		}
	}

	dictionaryLocked = 0;
	size_t size = (size_t)maxDictEntries;
	size = (size * sizeof(WORDENTRY)) + sizeof(WORDENTRY); // 0th entry also -- bytes needed
	size /= 64;  // 64-bit words needed
	size = (size * 64) + 64;  // back to rounded bytes needed
	int hsize = (maxHashBuckets + 1) * sizeof(int);
	if (!dictionaryBase)
	{
		//   dictionary and meanings 
		// on FUTURE startups (not 1st) the userCacheCount has been preserved while the rest of the system is reloaded
		hashbuckets = (unsigned int*)mymalloc(size + hsize);
		if (!hashbuckets) ReportBug("FATAL: Cannot allocate dictionary space");
		dictionaryBase = (WORDP)(((char*)hashbuckets) + hsize);

#ifdef EXPLAIN
		Conjoined heap space					Independent heap space
			heapBase -- allocates downwards
			heapBase - allocates downwards		heapFree
			heapFree								heapEnd  -- stackFree - allocates upwards
			------------------------ - disjoint memory
			dictionaryFree
			dictionaryBase - allocates upwards

			userTopicStore 1..n
			userTable refers to userTopicStore
			cacheBase
#endif
	}
	memset(hashbuckets, 0, size + hsize);
	dictionaryFree = dictionaryBase + 1;	// dont write on 0
	dictionaryPreBuild[LAYER_0] = dictionaryPreBuild[LAYER_1] = dictionaryPreBuild[LAYER_BOOT] = 0;	// in initial dictionary
	factsPreBuild[LAYER_0] = factsPreBuild[LAYER_1] = factsPreBuild[LAYER_BOOT] = lastFactUsed;	// last fact in dictionary 
}

void AddInternalFlag(WORDP D, unsigned int flag)
{
	if (flag && (flag & D->internalBits) != flag) // prove there is a change - dont use & because if some bits are set is not enough
	{
		if (D < dictionaryLocked && !csapicall && compiling == NOT_COMPILING) // compiler and api can change substitutions
			return;
		D->internalBits |= (unsigned int)flag;
		if (monitorChange) D->internalBits |= BIT_CHANGED;
	}
}

void RemoveInternalFlag(WORDP D, unsigned int flag)
{
	D->internalBits &= -1 ^ flag;
}

static void PreserveSystemFlags(WORDP D)
{
	flagsRedefines = AllocateHeapval(HV1_WORDP |HV2_INT,flagsRedefines, (uint64)D, D->systemFlags);
}

static void ChangeFlags(WORDP D, uint64 oldflags, uint64 newflags)
{
	if (oldflags != newflags)
	{
		D = GetLanguageWord(D);
		if (D < dictionaryLocked && compiling == NOT_COMPILING) PreserveSystemFlags(D);
		if (monitorDictChanges)
		{
			ongoingDictChanges = AllocateHeapval(HV1_WORDP | HV2_INT | HV3_INT, ongoingDictChanges, (uint64)D, D->properties, oldflags);
		}
		D->systemFlags = newflags;
		if (monitorChange && oldflags != MARKED_WORD) D->internalBits |= BIT_CHANGED;
	}
}

void AddSystemFlag(WORDP D, uint64 flag)
{
	if (!D) return;
	if (flag & NOCONCEPTLIST && *D->word != '~') flag ^= NOCONCEPTLIST; // not allowed to mark anything but concepts with this
	D = GetLanguageWord(D);
	uint64 sysflags = D->systemFlags;
	if ((sysflags & flag) != flag)
	{
		ChangeFlags(D, sysflags,sysflags | flag);
	}
}

void AddParseBits(WORDP D, unsigned int flag)
{
	D = GetLanguageWord(D);
	if (flag && flag != (flag & D->parseBits)) // prove there is a change - dont use & because if some bits are set is not enough
	{
		if (D < dictionaryLocked) return;
		D->parseBits |= (unsigned int)flag;
		if (monitorChange) D->internalBits |= BIT_CHANGED;
	}
}

void RemoveSystemFlag(WORDP D, uint64 flags)
{
	if (!D) return;
	if (flags & NOCONCEPTLIST && *D->word != '~') flags ^= NOCONCEPTLIST; // not allowed to mark anything but concepts with this
	D = GetLanguageWord(D);
	uint64 sysflags = D->systemFlags;
	if (sysflags & flags) // they have some in common
	{
		ChangeFlags(D, sysflags, sysflags ^ (uint64)-1 ^ flags);
	}
}

static void PreserveProperty(WORDP D)
{
	propertyRedefines = AllocateHeapval(HV1_WORDP|HV2_INT,propertyRedefines, (uint64)D, D->properties);
}

void ReverseDictionaryChanges(HEAPREF start)
{
	uint64 val1, val2, val3;
	while (start)
	{
		start = UnpackHeapval(start, val1, val2, val3);
		if (val1) {
			WORDP D = (WORDP)val1;
			if (!D) continue; // end of list
			D->properties = val2;
			D->systemFlags = val3;
		}
	}
}

void ReverseUniversalDictionaryChanges(HEAPREF start)
{
	uint64 val1, val2, val3;
	while (start)
	{
		start = UnpackHeapval(start, val1, val2, val3);
		if (val1) {
			WORDP D = (WORDP)val1;
			if (!D) continue; // end of list
			D->foreignFlags = (WORDP*)val2;
		}
	}
}

unsigned int GetHeaderWord(char* w)
{
	unsigned int n = BurstWord(w);
	if (n != 1)
	{
		char* buffer = AllocateBuffer();
		char* word = JoinWords(1, false, buffer);
		WORDP E = StoreWord(word, AS_IS);		// create the 1-word header
		FreeBuffer();
		// while main word might be universal (throat_singing), the header word might
		// be currently in a language, and need to go universal
		if (n != 1 && n > GETMULTIWORDHEADER(E))
		{
			// while main word might be universal (throat_singing), the header word might
			// be currently in a language, and need to go universal
			SETMULTIWORDHEADER(E, n);	//   mark it can go this far for an idiom
		}
	}
	return n;
}

static void ChangeProperty(WORDP D, uint64 oldprops,uint64 newprops)
{
	D = GetLanguageWord(D);
	if (oldprops != newprops)
	{
		if (D < dictionaryLocked && compiling == NOT_COMPILING) PreserveProperty(D);
		if (monitorDictChanges)
		{
			ongoingDictChanges = AllocateHeapval(HV1_WORDP | HV2_INT | HV3_INT, ongoingDictChanges, (uint64)D, oldprops, D->systemFlags);
		}
		D->properties = newprops;

		if (monitorChange) D->internalBits |= BIT_CHANGED;
	}
}

void AddProperty(WORDP D, uint64 properties)
{
	if (!D) return;
	D = GetLanguageWord(D);
	uint64 props = D->properties;
	if (properties != props)
	{
		properties |= props;
		ChangeProperty(D, props, properties);
	}
}

void RemoveProperty(WORDP D, uint64 properties)
{
	if (!D) return;
	D = GetLanguageWord(D);
	uint64 props = D->properties;
	if (properties & props)
	{
		props ^= (uint64)-1 ^ properties;
		ChangeProperty(D, props, properties);
	}
}

bool StricmpUTF(char* w1, char* w2, int len)
{
	unsigned char c1, c2;
	unsigned char* word1 = (unsigned char*)w1;
	unsigned char* word2 = (unsigned char*)w2;
	while (len && *word1 && *word2)
	{
		if (*word1 == 0xc3) // utf8 case sensitive
		{
			c1 = *word1;
			c2 = *word2;
			if (c1 == c2);
			else if (c1 >= 0x9f && c1 <= 0xbf) // lower case form
			{
				if (c2 >= 0x80 && c2 <= 0x9e && c1 != 0x9f && c1 != 0xb7) // uppercase form
				{
					c1 -= 0xa0;
					c1 += 0x80;
				}
				if (c1 != c2) return true;
			}
			else if (c2 >= 0x9f && c2 <= 0xbf) // lower case form
			{
				if (c1 >= 0x80 && c1 <= 0x9e && c2 != 0x9f && c2 != 0xb7) // uppercase form
				{
					c2 -= 0xa0;
					c2 += 0x80;
				}
				if (c1 != c2) return true;
			}
		}
		else if (word1[1] >= 0xc4 && word1[1] <= 0xc9)
		{
			c1 = *word1;
			c2 = *word2;
			if (c1 == c2);
			else if (c1 & 1 && (c1 & 0xf7) == c2); // lower case form
			else if (c2 & 1 && (c2 & 0xf7) == c1);
			else return true;
		}
		else if (toLowercaseData[*word1] != toLowercaseData[*word2]) return true;
		int size = UTFCharSize((char*)word1);
		word1 += size;
		word2 += size;
		len -= size;
	}
	if (len == 0) return false; // if matched all characters we needed to, then we are done
	return *word1 || *word2;
}

int GetWords(char* word, WORDP* set, bool strictcase, bool allvalid)
{
	size_t len = strlen(word);
	if (len >= MAX_WORD_SIZE) return 0;	// not legal

	char commonword[MAX_WORD_SIZE];
	strcpy(commonword, word);
	if (len < MAX_WORD_SIZE)
	{
		char* at = commonword;
		while ((at = strchr(at, ' '))) *at = '_';	 // match as underscores whenever spaces occur (hash also treats them the same)
	}

	int index = 0;

	fullhash = Hashit((unsigned char*)word, len, hasUpperCharacters, hasUTF8Characters,hasSeparatorCharacters); //   sets hasUpperCharacters and hasUTF8Characters
	wordhash = (fullhash % maxHashBuckets); // mod by the size of the table

	 //   lowercase bucket
	WORDP D = Index2Word( hashbuckets[wordhash]);
	char word1[MAX_WORD_SIZE];
	if (strictcase && hasUpperCharacters) D = dictionaryBase; // lower case not allowed to match uppercase input
	int limit = GETWORDSLIMIT;
	while (D && D != dictionaryBase)
	{
		if (WORDLENGTH(D) != len) {;}
		else if (!allvalid && !IsValidLanguage(D) && !seeAllFacts) { ; }
		else 
		{
			char* potentialMatch = D->word;
			if (len < MAX_WORD_SIZE)
			{
				strcpy(word1, D->word);
				char* at = word1;
				while ((at = strchr(at, ' '))) *at = '_';	 // match as underscores whenever spaces occur (hash also treats them the same)
				potentialMatch = word1;
			}
			if (!StricmpUTF(potentialMatch, commonword, len)) // match upper or lower case
			{
				if (--limit <= 0)
				{
					ReportBug("Getwords0 finds too many for %s", word);
					break;
				}
				set[index++] = D;
			}
		}
		D = Index2Word( GETNEXTNODE(D));
	}

	// upper case bucket
	D = Index2Word(hashbuckets[wordhash + 1]);
	if (strictcase && !hasUpperCharacters) D = NULL; // upper case not allowed to match lowercase input
	while (D && D != dictionaryBase)
	{
		if (WORDLENGTH(D) != len ) { ; }
		else  if (!IsValidLanguage(D) && !seeAllFacts) { ; }
		else 
		{
			if (!StricmpUTF(D->word, commonword, len))
			{
				if (--limit <= 0)
				{
					ReportBug("Getwords2 finds too many for %s", word);
					break;
				}
				set[index++] = D;
			}
		}
		D = Index2Word( GETNEXTNODE(D));
	}
	return index;
}

int UTFCharSize(char* utf)
{
	unsigned int count = 1; // bytes of character
	unsigned char c = (unsigned char)*utf;
	if (!(c & 0x80)) { ; }// ordinary ascii char
	else if ((c & 0xE0) == 0xC0) count = 2; // 110 0xc0
	else if ((c & 0Xf0) == 0Xe0)  count = 3; // 1110 0xE0
	else if ((c & 0Xf8) == 0XF0) count = 4; // 11110 0xF0
	return count;
}

WORDP FindWord(const char* word, unsigned int len, uint64 caseAllowed,bool exactmatch, bool underscorespaceequivalent)
{
// See engine documentation for ## Dictionary Entries (normal words in a single language)
// 	   and # Multiple Languages for multiple language processing.

	if (word == NULL || *word == 0 || dictionaryBase == NULL) return NULL;
	if (len == 0) len = strlen(word);
	fullhash = Hashit((unsigned char*)word, len, hasUpperCharacters, hasUTF8Characters, hasSeparatorCharacters); //   sets hasUpperCharacters and hasUTF8Characters 
	wordhash = (fullhash % maxHashBuckets); // mod by the size of the table
	
	// Hash returns index for lowercase bucket.
	// If we have uppercase characters and  are allowed to match uppercase, we will up bucket 1 
	if (caseAllowed & LOWERCASE_LOOKUP) { ; } // stay in lower bucket regardless
	// rule label (topic.name) uses uppercase lookup
	else if (*word == SYSVAR_PREFIX || *word == USERVAR_PREFIX || (*word == '~' && !strchr(word, '.')) || *word == '^')
	{
		if (caseAllowed == UPPERCASE_LOOKUP) return NULL; // not allowed to find
		caseAllowed = LOWERCASE_LOOKUP; // these are always lower case
	}
	else if (hasUpperCharacters || (caseAllowed & UPPERCASE_LOOKUP)) ++wordhash;

	// you can search on upper or lower specifically (not both) or primary or secondary or both

	//   normal or fixed case bucket
	WORDP D;

	// some phrases may be using underscores or spaces, convert incoming to common if requested
	char* common = NULL;
	if (underscorespaceequivalent && len < MAX_WORD_SIZE)
	{
		common = AllocateBuffer();
		strcpy(common, word);
		while ((common = strchr(common, ' '))) *common = '_'; // convert to common form
	}
	
	primaryLookupSucceeded = true;
	if (caseAllowed & (PRIMARY_CASE_ALLOWED | LOWERCASE_LOOKUP | UPPERCASE_LOOKUP))
	{ // d3a from :word
		D = Index2Word(hashbuckets[wordhash]);
		WORDP almost = NULL;
		WORDP preferred = NULL;
		WORDP exact = NULL;
		int limit = DICTSEARCHLIMIT;
		while (D && --limit)
		{
			// exactmatch requires we find the word in the current language (eg when loading a language)
			// EXCEPT any entry whose base language is universal 
			// These are universal predefined words for CS like member.
			if (fullhash != D->hash || WORDLENGTH(D) != len ) { ; }
			else  if (!strncmp(D->word, word, len)) // exact string match
			{
				unsigned int lang = D->internalBits & LANGUAGE_BITS;
				if (!exactmatch && D->internalBits & IS_SUPERCEDED) {;} // not allowed to see unless looking hard for it
				else if (lang == LANGUAGE_UNIVERSAL || lang == language_bits) return D; 
				else if (!exactmatch && D->internalBits & UNIVERSAL_WORD) return D;
			}
			else if (exactmatch) { ; }
			else if (IsValidLanguage(D))
			{
				char* potentialMatch = D->word;
				if (common) // on the fly convert to common form
				{
					potentialMatch = AllocateBuffer();
					strcpy(potentialMatch, D->word);
					while ((potentialMatch = strchr(potentialMatch, ' '))) *potentialMatch = '_'; // convert to common form
					FreeBuffer(); // does not destroy content
				}

				if ( !StricmpUTF(potentialMatch,(char*)word, len)) // they match independent of case- 
				{
					if (caseAllowed == LOWERCASE_LOOKUP)
					{
						preferred = D; // incoming word MIGHT have uppercase letters but we will be in lower bucket
						break;
					}
					else if (hasUpperCharacters) // we are looking for uppercase or primary case and are in uppercase bucket
					{
						if (D->internalBits & PREFER_THIS_UPPERCASE) preferred = D;
						else if (!strncmp(potentialMatch, word, len)) exact = D; // exactly  what we want in upper case
						else almost = D; // remember semi-match in upper case
					}
					else // if uppercase lookup, we are in uppercase bucket (input was lower case) else we are in lowercase bucket
					{
						preferred = D;
						break; //  it is exactly what we want in lower case OR its an uppercase acceptable match
					}
				}
			}
			D = Index2Word( GETNEXTNODE(D));
		}

		if (preferred) D = preferred;
		else if (exact) D = exact;
		else if (almost) D = almost; // uppercase request we found in a different form only
		else D = NULL;
		if (D)
		{
			if (common) FreeBuffer();
			return D;
		}
	}

	//    alternate case bucket (checking opposite case)
	primaryLookupSucceeded = false;
	if (caseAllowed & SECONDARY_CASE_ALLOWED)
	{
		WORDP almost = NULL;
		WORDP preferred = NULL;
		D =Index2Word( hashbuckets[wordhash + ((hasUpperCharacters) ? -1 : 1)]);
		int limit = DICTSEARCHLIMIT;
		while (D && D != dictionaryBase && --limit)
		{
			if (fullhash != D->hash || WORDLENGTH(D) != len) { ; }
			else if (exactmatch && !strncmp(D->word, word, len) && // from inbuild foreign postaggers
				(D->internalBits & LANGUAGE_BITS) == language_bits)  return D;
			else if (!IsValidLanguage(D)) { ; }
			else
			{
				char* potentialMatch = D->word;
				if (common) // on the fly convert to common form
				{
					potentialMatch = AllocateBuffer();
					strcpy(potentialMatch, D->word);
					while ((potentialMatch = strchr(potentialMatch, ' '))) *potentialMatch = '_'; // convert to common form
					FreeBuffer(); // does not destroy content
				}
				if (!StricmpUTF(potentialMatch, (char*)word, len))
				{
					if (hasUpperCharacters)  // lowercase form
					{
						preferred = D;
						break;
					}
					if (!exactmatch && D->internalBits & UNIVERSAL_WORD) // universal is found in preference
					{
						preferred = D;
						break;
					}
					if (D->internalBits & PREFER_THIS_UPPERCASE) preferred = D;
					else almost = D; // remember a match in upper case
				}
			}
			D = Index2Word( GETNEXTNODE(D));
		}
		if (common) FreeBuffer();
		if (preferred) return preferred;
		else if (almost) return almost;	// uppercase request we found in a different form only
	}

	return NULL;
}

WORDP AllocateEntry(char* name)
{
	WORDP  D = dictionaryFree++;
	int index = Word2Index(D);
	int avail = (int)(maxDictEntries - index);
	if (avail <= 0) ReportBug((char*)"FATAL: used up all dict nodes\r\n");
		if (avail < worstDictAvail) worstDictAvail = avail;
	memset(D, 0, sizeof(WORDENTRY));
	if (name) 	D->word  = AllocateHeap(name, 0); 
	return D;
}

WORDP StoreIntWord(int val) // create a number word
{
	char value[MAX_WORD_SIZE];
	sprintf(value, (char*)"%d", val);
	return StoreWord(value,AS_IS);
}

WORDP StoreFlaggedWord(const char* word, uint64 properties, uint64 flags)
{
	WORDP D = StoreWord(word, properties);
	AddSystemFlag(D, flags);
	return D;
}

void KillWord(WORDP D,bool complete)
{ 
	D->internalBits |= IS_SUPERCEDED; 
	if (!complete) return;
	// these facts will have come from build0 and us build1 because build1 facts would not even have been written out
	FACT* F = GetSubjectHead(D);
	while (F)
	{
		F->flags |= FACTDEAD; // dont write old, priors will delete later
		F = GetSubjectNext(F);
	}
	F = GetVerbHead(D);
	while (F)
	{
		F->flags |= FACTDEAD; // dont write old, priors will delete later
		F = GetVerbNext(F);
	}
	F = GetObjectHead(D);
	while (F)
	{
		F->flags |= FACTDEAD; // dont write old, priors will delete later
		F = GetObjectNext(F);
	}
}

static void AdjustFactsLanguage(WORDP D) 
{
	// word went universal. Its facts point to it still, but their language has changed
	// if fact is from prior level, we need to write out revised fact to update it
	FACT* F = GetSubjectNondeadHead(D);
	while (F)
	{
		AdjustFactLanguage(F,F->subject,0);
		F = GetSubjectNondeadNext(F);
	}
	F = GetVerbNondeadHead(D);
	while (F)
	{
		AdjustFactLanguage(F,F->verb,1);
		F = GetVerbNondeadNext(F);
	}
	F = GetObjectNondeadHead(D);
	while (F)
	{
		AdjustFactLanguage(F,F->object,2);
		F = GetObjectNondeadNext(F);
	}
}

WORDP Convert2Universal(WORDP D,WORDP* oldwords,unsigned int oldwordcount)
{
	if (!D) D = oldwords[0]; // first seen if not exact match on primary language
	D->internalBits |= UNIVERSAL_WORD;
	
	AdjustFactsLanguage(D); // our facts point to us already, we need to change the fact language 
	if (oldwordcount <= 1) return D; // nobody to kill
	if (!D->foreignFlags) D->foreignFlags = (WORDP*)AllocateHeap(NULL, languageCount, sizeof(WORDP),true );
	
	MEANING M = MakeMeaning(D);
	uint64 oldbot = myBot;
	for (unsigned int i = 0; i < oldwordcount; ++i)
	{
		WORDP tmp = oldwords[i];
		int index = GET_LANGUAGE_INDEX(tmp) - 1; // 0 based it, since we are not holding universals
		D->foreignFlags[index] = tmp; // self ptr
		if (tmp != D) tmp->internalBits |= IS_SUPERCEDED;
		else continue; // we adjusted language on our existing facts which point to us
		// note copied facts do not necessarily use the language of original, since it may become universal
		FACT* F = GetSubjectHead(tmp);
		while (F)
		{
			myBot = F->botBits;
			CreateFact(M, F->verb, F->object, F->flags & (-1 ^ FACTLANGUAGEBITS)); // moved fact, no language data yet
			LostFact(F);
			F = GetSubjectNext(F);
		}
		 F = GetVerbHead(tmp);
		while (F)
		{
			myBot = F->botBits;
			CreateFact(F->subject, M, F->object, F->flags & (-1 ^ FACTLANGUAGEBITS));
			LostFact(F);
			F = GetVerbNext(F);
		}
		F = GetObjectHead(tmp);
		while (F)
		{
			myBot = F->botBits;
			CreateFact(F->subject,F->verb, M, F->flags & (-1 ^ FACTLANGUAGEBITS));
			LostFact(F);
			F = GetObjectNext(F);
		}
		if (D != tmp) KillWord(tmp, false);
	}
	myBot = oldbot;
	return D;
}

static WORDP oldwords[12];
static unsigned int oldwordcount;

bool SUPERCEDED(WORDP D)
{
	return  (!D || D->internalBits & IS_SUPERCEDED);
}

WORDP GetLanguageWord(const char* word)
{ 
	if (!dictionaryBase)
		return NULL;
	size_t len = strlen(word);
	WORDP E = FindWord(word, len, PRIMARY_CASE_ALLOWED, true);  // see if have universal of it
	if (E) // entry either unique to language or universal word
	{
		if (E->foreignFlags) // multilanguage universal word
		{
			E = E->foreignFlags[languageIndex];
		}
		if (E) return E; 
	}
	// hash and fullhash was set by call to findword

	// We dont have a unique language entry, make one which will be transient
	WORDP D = AllocateEntry((char*)word);
	D->nextNode = hashbuckets[wordhash];
	hashbuckets[wordhash] = Word2Index(D);
	D->hash = fullhash;
	D->length = (unsigned short)len;
	unsigned int flags = IsUniversal(D->word, 0) ? UNIVERSAL_WORD :language_bits;
	if (hasUTF8Characters) flags |= UTF8;
	if (hasUpperCharacters) flags |= UPPERCASE_HASH; // dont label it NOUN or NOUN_UPPERCASE
	if (hasSeparatorCharacters) flags |= MULTIPLE_WORD;
	AddInternalFlag(D, flags);
	return D;
}

WORDP StoreWord(const char* word, uint64 properties)
{
	if (!dictionaryBase) 
		return NULL;
	if (!*word) // this is legal coming from json parse. we dont expect it anywhere else
	{
		int xx = 0;
	}
	if (properties & AS_IS) {}
	else if (strchr(word, '\n') || strchr(word, '\r') || strchr(word, '\t')) // force backslash format on these characters
	{ // THIS SHOULD NEVER HAPPEN, not allowed these inside data
		char* buf = AllocateBuffer(); // jsmn fact creation may be using INFINITIE stack space.
		AddEscapes(buf, word, true, maxBufferSize);
		FreeBuffer();
		word = buf;
	}
	char* buffer = NULL;
	unsigned int n = 0;
	bool lowercase = false;

	//   make all words normalized with no blanks in them.
	char wordx[MAX_WORD_SIZE];
	if ((*word == SYSVAR_PREFIX || *word == USERVAR_PREFIX || *word == '~' || *word == '^') &&
		IsLegalName(word + 1))  // dont change something that looks like json reference off of variable
	{
		lowercase = true; // these are always lower case
		MakeLowerCopy(wordx, word); // JUST IN CASE HE SYNTHESIZED it with upper case
		word = wordx;
	}
	else if (properties & (PUNCTUATION_BITS | AS_IS)) {}
	else if (*word == '"' || *word == '_' || *word == '`') { ; } // dont change any quoted things or things beginning with _ (we use them in facts for a "missing" value) or user var names
	else if (*word == USERVAR_PREFIX && word[1] == '_' && word[2] == '_' && IsDigit(word[3])) {} // internal parameter names for jsonreadcsv are not legal names
	else
	{
		n = BurstWord(word, 0);
		if (n != 1)
		{
			buffer = AllocateBuffer();
			word = JoinWords(n, false, buffer); //   when reading in the dictionary, BurstWord depends on it already being in, so just use the literal text here
		}
	}
	properties &= -1 ^ AS_IS;

	size_t len = strlen(word);
	fullhash = Hashit((unsigned char*)word, len, hasUpperCharacters, hasUTF8Characters,hasSeparatorCharacters); //   sets hasUpperCharacters and hasUTF8Characters
	wordhash = (fullhash % maxHashBuckets); //   mod the size of the table (saving 0 to mean no pointer and reserving an end upper case bucket)
	if (hasUpperCharacters)
	{
		if (lowercase) hasUpperCharacters = false; // refuse the case
		else ++wordhash;
	}

	//   locate spot existing entry goes - we use different buckets for lower case and upper case forms (next bucket up)
	unsigned int offset = hashbuckets[wordhash];
	WORDP D = Index2Word(offset);
	WORDP match = NULL;
	int limit = DICTSEARCHLIMIT;
	WORDP earlierWord = NULL;
	oldwordcount = 0;

	while (D && D != dictionaryBase && --limit)
	{
		if (WORDLENGTH(D) != len) { ; }
		else if (!strcmp(D->word, (char*)word)) // do we have a precise match
		{
			// we cannot create a language based word if it exists in universal (you can use MakeLanguage Word when reading in dictionaries)
			if (D->internalBits & UNIVERSAL_WORD)// found universal, not allowed to create language form
			{
				match = D; 
				break;
			}
			// IF we have a universal, then all language specific forms will have been superceded
			else if (IsValidLanguage(D) ) // 
			{
				// we are not trying to make a universal 
					match = D;
					break;
			}
			// track all  entries that we might create a universal from
			else if (multidict && !language_bits) // primary language inherit as universal entry
			{
				oldwords[oldwordcount++] = D; // this might have been it
				if (GET_LANGUAGE_INDEX(D)  == 1) earlierWord = D; // default language preference
				else if (!earlierWord) earlierWord = D;
			}
		}
		D = Index2Word(GETNEXTNODE(D));
	}
	if (match)
	{
		if (properties) AddProperty(match, properties);
		if (buffer) FreeBuffer();
		return match;
	}

	// upgrade some language entry to universal?
 	if (!language_bits && earlierWord)
	{
		if (buffer) FreeBuffer();
		D = Convert2Universal(earlierWord, oldwords, oldwordcount);
		if (properties) AddProperty(D, properties);
		unsigned int index = Word2Index(D); // debug info
		return D;
	}
	//   not found, add entry 
	D = AllocateEntry((char*)word);
	unsigned int index = Word2Index(D); // debug info
	D->nextNode = hashbuckets[wordhash];
	hashbuckets[wordhash] = Word2Index(D);
	// fill in data on word
	D->hash = fullhash;
	D->length = (unsigned short)len;
	// incoming json to cs from api should be universal
	// so make all json universal access
	unsigned int flags = (IsUniversal(D->word, properties)) ? (LANGUAGE_UNIVERSAL | UNIVERSAL_WORD) : language_bits;
	if (language_bits == LANGUAGE_UNIVERSAL) flags |= UNIVERSAL_WORD;
	if (hasUTF8Characters) flags |= UTF8;
	if (hasUpperCharacters) flags |= UPPERCASE_HASH; // dont label it NOUN or NOUN_UPPERCASE
	if (hasSeparatorCharacters) flags |= MULTIPLE_WORD; 
	if (flags) AddInternalFlag(D, flags);
	if (properties) AddProperty(D, properties);
	if (buffer) FreeBuffer();
	return D;
}

void AddCircularEntry(WORDP base, unsigned int field, WORDP entry)
{
	if (!base) return;
	WORDP D;
	WORDP old;

	//   verify item to be added not already in circular list of this kind - dont add if it is
	if (field == COMPARISONFIELD)
	{
		D = GetComparison(entry);
		old = GetComparison(base);
	}
	else if (field == TENSEFIELD)
	{
		D = GetTense(entry);
		old = GetTense(base);
	}
	else if (field == PLURALFIELD)
	{
		D = GetPlural(entry);
		old = GetPlural(base);
	}
	else return;
	if (D)
	{
		return;
	}

	if (!old) old = base; // init to self

	if (field == COMPARISONFIELD)
	{
		irregularAdjectives[base] = entry;
		irregularAdjectives[entry] = old;
	}
	else if (field == TENSEFIELD)
	{
		irregularVerbs[base] = entry;
		irregularVerbs[entry] = old;
	}
	else if (field == PLURALFIELD)
	{
		irregularNouns[base] = entry;
		irregularNouns[entry] = old;
	}
}

static void ReadLanguage(char* language)
{
	max_fact_language = languageIndex; // track highest index we see
	max_language = languageIndex << LANGUAGE_SHIFT;
	if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese")) return; // no dicts for these

	ReadAsciiDictionary();
	ReadFacts(UseDictionaryFile((char*)"facts.txt"),NULL, -1);
	*currentFilename = 0;
}

char* GetNextLanguage(char*& data)
{
	if (!data || !*data) return NULL;

	static char language[100];
	char* comma = strchr(data, ',');
	if (comma) *comma = 0;
	strcpy(language, data);
	if (comma)
	{
		*comma = ',';
		data = comma + 1;
	}
	else data = NULL;
	return language;
}

void WalkLanguages(LANGUAGE_FUNCTION func)
{
	// load english last (so it comes up first in buckets) but it is first in list for universal merging
	char oldlang[100];
	strcpy(oldlang, current_language);
	char list[300];
	strcpy(list, language_list);
	char* lang;
	char* data = language_list;
	while ((lang = strrchr(list, ','))) // walk list backwards
	{
		*lang++ = 0;
		SetLanguage(lang);
		if (language_bits || !multidict) (func)(lang);
	}
	SetLanguage(list); // 1st in list
	if (language_bits || !multidict) (func)(list);

	strcpy(current_language, oldlang);
}

void WalkDictionary(DICTIONARY_FUNCTION func, uint64 data)
{
	for (WORDP D = dictionaryBase + 1; D < dictionaryFree; ++D)
	{
		// allowed to see concepts and variables
		if (D->word && *D->word && IsValidLanguage(D))
		{
			(func)(D, data);
		}
	}
}

void DeleteDictionaryEntry(WORDP D)
{
	// Newly added dictionary entries will be after some marker (provided we are not loading the base dictionary).
	wordhash = (D->hash % maxHashBuckets);
	if (D->internalBits & UPPERCASE_HASH) 
		++wordhash;
	WORDP E = Index2Word(hashbuckets[wordhash]);
	if (E != D) // should be top of bucket - maybe lacks uppercase bit?
		myexit("Dictionary delete is wrong");
	hashbuckets[wordhash] = GETNEXTNODE(D); // move this out of bucket
}

void ShowStats(bool reset)
{
	static unsigned int maxRules = 0;
	static unsigned int maxDict = 0;
	static unsigned int maxText = 0;
	static unsigned int maxFact = 0;
	if (reset) maxRules = maxDict = maxText = maxFact = ruleCount = 0;
	if (stats && !reset)
	{
		unsigned int dictUsed = dictionaryFree - dictionaryLocked;
		if (dictUsed > maxDict) maxDict = dictUsed;
		unsigned int factUsed = lastFactUsed - factLocked;
		if (factUsed > maxFact) maxFact = factUsed;
		unsigned int textUsed = stringLocked - heapFree;
		if (textUsed > maxText) maxText = textUsed;
		if (ruleCount > maxRules) maxRules = ruleCount;

		unsigned int diff = (unsigned int)(ElapsedMilliseconds() - volleyStartTime);
		unsigned int mspl = (inputSentenceCount) ? (diff / inputSentenceCount) : 0;
		double fract = (double)(diff / 1000.0); // part of seccond
		double time = (double)(tokenCount / fract);
		Log(ECHOUSERLOG, (char*)"\r\nRead: %d sentences (%d tokens) in %d ms = %d ms/l or %f token/s\r\n", inputSentenceCount, tokenCount, diff, mspl, time);

		Log(ECHOUSERLOG, (char*)"used: rules=%d dict=%d words fact=%d facts text=%d mark=%d\r\n", ruleCount, dictUsed, factUsed, textUsed, xrefCount);
		Log(ECHOUSERLOG, (char*)"      maxrules=%d  maxdict=%d words maxfact=%d facts maxtext=%d\r\n", maxRules, maxDict, maxFact, maxText);
		ruleCount = 0;
		xrefCount = 0;
	}
}

void WriteDictDetailsBeforeLayer(int layer)
{// used by: writedictchanged scriptcompile, return to levels
	char word[MAX_WORD_SIZE];
	sprintf(word, (char*)"%s/prebuild%d.bin", tmpfolder, layer);
	FILE* out = FopenBinaryWrite(word); // binary file, no BOM
	if (out)
	{
		unsigned int check = CHECKSTAMP;
		fwrite(&check, 1, 4, out);
		char x = 77;
		char y = 76;
		for (WORDP D = dictionaryBase + 1; D < dictionaryPreBuild[layer]; ++D)
		{
			if (!D->word) continue; // not actually here
			unsigned int offset = Word2Index(D);
			fwrite(&offset, 1, 4, out);
			fwrite(&D->properties, 1, 8, out);
			D->systemFlags &= -1 ^ (MARKED_WORD); // remove transient flag
			fwrite(&D->systemFlags, 1, 8, out);
			D->internalBits &= -1 ^ (BEEN_HERE); // remove transient flag
			fwrite(&D->internalBits, 1, 4, out);
			unsigned char header = (unsigned char) GETMULTIWORDHEADER(D);
			fwrite(&header, 1, 1, out);
			fwrite(&D->length, 1, 4, out);
			// substitutes only has value if substitue set, so we lose glosses
			unsigned int index = 0;
			if (D->systemFlags & HAS_SUBSTITUTE && D->w.substitutes)
				index = Word2Index(D->w.substitutes);
			else if (D->internalBits & CONDITIONAL_IDIOM)
				index = Word2Index(D->w.conditionalIdiom);
			fwrite(&index, 1, 4, out);
			index = 0;
			if (D->w.userValue && IsQuery(D))
			{
				index = strlen(D->w.userValue) + 1;
			}
			fwrite(&index, 1, 4, out);
			if (index) fwrite(D->w.userValue, 1, index, out); // query
			if (D->foreignFlags)
			{
				fwrite(&y, 1, 1, out); // marker for foreign
				fwrite(D->foreignFlags, languageCount, sizeof(WORDP), out);
			}
			fwrite(&x, 1, 1, out);
		}
		FClose(out);
	}
	else
	{
		(*printer)((char*)"%s", (char*)"Unable to open TMP prebuild file\r\n");
		Log(USERLOG, "Unable to open TMP prebuild file\r\n");
	}
}


void WordnetLockDictionary() // dictionary and facts before build0 layer 
{
	currentBeforeLayer = -1;
	LockLayer(); // memorize dictionary values for backup to pre build locations :build0 operations (reseting word to dictionary state)
}

void LockLevel()
{
	++currentBeforeLayer;
	dictionaryLocked = dictionaryFree;
	stringLocked = heapFree;
	factLocked = lastFactUsed; // lastFactUsed is a fact in use (increment before use) so factlocked is a fact in use
}

void UnlockLayer(int layer)
{
	dictionaryLocked = NULL;
	stringLocked = NULL;
	factLocked = NULL;
	currentBeforeLayer = layer;
}

void LockLayer()
{
	// stores the results of the current layer as the starting point of the next layers (hence size of arrays are +1)
	for (int i = currentBeforeLayer + 1; i < NUMBER_OF_LAYERS; ++i)
	{
		numberOfTopicsInLayer[i] = (currentBeforeLayer == -1) ? 0 : numberOfTopicsInLayer[currentBeforeLayer];
		dictionaryPreBuild[i] = dictionaryFree;
		heapPreBuild[i] = heapFree;
		factsPreBuild[i] = lastFactUsed;
		topicBlockPtrs[i] = NULL;
		buildStamp[i][0] = 0;
		compileVersion[i][0] = 0;
	}

	LockLevel();
}

void ReturnToAfterLayer(int layer, bool unlocked)
{
	if (layer == LAYER_BOOT) UnwindUserLayerProtect(); // unwind any user layer protection (dynamic loading of layer past user layer- not used )
	while (lastFactUsed > factsPreBuild[layer + 1]) FreeFact(lastFactUsed--); //   restore back to facts alone
	DictionaryRelease(dictionaryPreBuild[layer + 1], heapPreBuild[layer + 1]);

	dictionaryBitsChanged = false;
	LockLayer();	// dont write data to file
	numberOfTopics = numberOfTopicsInLayer[layer];

	// canonical map in layer 1 is now garbage- 
	if (unlocked) UnlockLayer(layer);
}

void ReturnBeforeBootLayer()
{
	ClearUserVariables();
	UnwindUserLayerProtect();
	while (lastFactUsed > factsPreBuild[LAYER_BOOT]) FreeFact(lastFactUsed--); //   restore back to facts alone
	DictionaryRelease(dictionaryPreBuild[LAYER_BOOT], heapPreBuild[LAYER_BOOT]);
	botVariableThreadList = NULL; // this layer is gone
	dictionaryBitsChanged = false;
	LockLayer();	// dont write data to file
	numberOfTopics = numberOfTopicsInLayer[LAYER_BOOT];
	currentBeforeLayer = LAYER_BOOT;
	// canonical map in layer 1 is now garbage- 
	UnlockLayer(LAYER_BOOT);
}

void CloseDictionary()
{
	ClearWordMaps();
	CloseTextUtilities();
}

void FreeDictionary()
{
	CloseDictionary();
	if (hashbuckets) myfree(hashbuckets);
	hashbuckets = NULL;
	dictionaryBase = NULL; // based off hashbuckets
}

static void Write8(unsigned int val, FILE * out)
{
	unsigned char x[1];
	x[0] = val & 0x000000ff;
	if (out) fwrite(x, 1, 1, out);
	else *writePtr++ = *x;
}

bool IsQuery(WORDP D)
{
	return (D && D->internalBits & QUERY_KIND && *D->word != '@' && *D->word != '#' && *D->word != '_');
}

bool IsRename(WORDP D)
{
	return (D->internalBits & QUERY_KIND && (*D->word == '@' || *D->word == '#' || *D->word == '_'));
}

static void Write16(unsigned int val, FILE * out)
{
	unsigned char x[2];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	if (out) fwrite(x, 1, 2, out);
	else
	{
		memcpy(writePtr, (unsigned char*)x, 2);
		writePtr += 2;
	}
}

void Write24(unsigned int val, FILE * out)
{
	unsigned char x[3];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	if (out) fwrite(x, 1, 3, out);
	else
	{
		memcpy(writePtr, (unsigned char*)x, 3);
		writePtr += 3;
	}
}

void Write32(unsigned int val, FILE * out)
{
	unsigned char x[4];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	x[3] = (val >> 24) & 0x000000ff;
	if (out) fwrite(x, 1, 4, out);
	else
	{
		memcpy(writePtr, (unsigned char*)x, 4);
		writePtr += 4;
	}
}

void Write64(uint64 val, FILE * out)
{
	unsigned char x[8];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	x[3] = (val >> 24) & 0x000000ff;
	x[4] = (val >> 32) & 0x000000ff;
	x[5] = (val >> 40) & 0x000000ff;
	x[6] = (val >> 48) & 0x000000ff;
	x[7] = (val >> 56) & 0x000000ff;
	if (out) fwrite(x, 1, 8, out);
	else
	{
		memcpy(writePtr, (unsigned char*)x, 8);
		writePtr += 8;
	}
}

void WriteDWord(WORDP ptr, FILE * out)
{
	unsigned int val = (ptr) ? Word2Index(ptr) : 0;
	unsigned char x[3];
	x[0] = val & 0x000000ff;
	x[1] = (val >> 8) & 0x000000ff;
	x[2] = (val >> 16) & 0x000000ff;
	if (out) fwrite(x, 1, 3, out);
	else
	{
		memcpy(writePtr, (unsigned char*)x, 3);
		writePtr += 3;
	}
}

static void WriteString(char* str, FILE * out)
{
	if (!str || !*str) Write16(0, out);
	else
	{
		size_t len = strlen(str);
		Write16(len, out);
		if (out) fwrite(str, 1, len + 1, out);
		else
		{
			memcpy(writePtr, (unsigned char*)str, len + 1);
			writePtr += len + 1;
		}
	}
}

static void WriteBinaryEntry(WORDP D, FILE * out)
{
	unsigned char c;
	writePtr = (unsigned char*)(readBuffer + 2); // reserve size space
	WriteString(D->word, 0);
	unsigned int bits = 0;
	if (GETMULTIWORDHEADER(D)) bits |= 1 << 0;
	if (GetTense(D)) bits |= 1 << 1;
	if (GetPlural(D)) bits |= 1 << 2;
	if (GetComparison(D)) bits |= 1 << 3;
	if (GetMeaningCount(D)) bits |= 1 << 4;
	if (GetGlossCount(D)) bits |= 1 << 5;
	if (D->systemFlags || D->properties || D->parseBits) bits |= 1 << 6;
	if (D->internalBits) bits |= 1 << 7;
	if (RawCanonical(D)) bits |= 1 << 8;
	if (D->subjectHead) bits |= 1 << 9;
	if (D->verbHead) bits |= 1 << 10;
	if (D->objectHead) bits |= 1 << 11;
	if (D->foreignFlags) 
		bits |= 1 << 12; // will be a universal entry
	Write16(bits, 0);
	Write64(D->hash, 0);
	if (bits & (1 << 6))
	{
		Write64(D->properties, 0);
		Write64(D->systemFlags, 0);
		Write32(D->parseBits, 0);
	}
	if (D->subjectHead) Write32(D->subjectHead, 0);
	if (D->verbHead) Write32(D->verbHead, 0);
	if (D->objectHead) Write32(D->objectHead, 0);
	if (D->foreignFlags) // wont be any such entries at present
	{
		for (unsigned int i = 0; i < languageCount; ++i) Write32(Word2Index(D->foreignFlags[i]), 0);
	}

	if (D->internalBits) Write32(D->internalBits, 0);
	Write32(D->nextNode, 0);
	if (D->internalBits & CONDITIONAL_IDIOM)
	{
		if (!D->w.conditionalIdiom) Write32(0,0);
		else Write32(Word2Index(D->w.conditionalIdiom), 0);
	}
	else if (D->systemFlags & HAS_SUBSTITUTE)
	{
		if (!D->w.substitutes) Write32(0, 0);
		else Write32(Word2Index(D->w.substitutes), 0);
	}

	if (GETMULTIWORDHEADER(D))
	{
		c = (unsigned char)GETMULTIWORDHEADER(D);
		Write8(c, 0); //   limit 255 no problem
	}
	if (GetTense(D))
	{
		WORDP X = GetTense(D);
		MEANING M = MakeMeaning(X);
		Write32(M, 0);
	}
	if (GetPlural(D)) Write32(MakeMeaning(GetPlural(D)), 0);
	if (GetComparison(D)) Write32(MakeMeaning(GetComparison(D)), 0);
	WORDP canon = RawCanonical(D);
	if (canon) Write32(MakeMeaning(canon), 0);

	if (GetMeaningCount(D))
	{
		c = (unsigned char)GetMeaningCount(D);
		Write8(c, 0);  //   limit 255 no problem
		for (int i = 1; i <= GetMeaningCount(D); ++i) Write32(GetMeaning(D, i), 0);
	}
	if (GetGlossCount(D))
	{
		c = (unsigned char)GetGlossCount(D);
		Write8(c, 0); //   limit 255 no problem
		for (int i = 1; i <= GetGlossCount(D); ++i)
		{
			Write8(D->w.glosses[2*i] , 0);
			WriteString(Index2Heap(D->w.glosses[2*i - 1]), 0);
		}
	}
	Write8('0', 0);
	unsigned int len = writePtr - (unsigned char*)readBuffer;
	*readBuffer = (unsigned char)(len >> 8);
	readBuffer[1] = (unsigned char)(len & 0x00ff);
	fwrite(readBuffer, 1, len, out);
}

void WriteBinaryDictionary()
{
	FILE* out = FopenBinaryWrite(UseDictionaryFile((char*)"dict.bin",language_list)); // binary file, no BOM
	if (!out) return;
	Write32(CHECKSTAMP, out); // version stamp
	Write32(maxHashBuckets, out); // bucket size used
	size_t len = strlen(language_list);
	Write8(len, out);
	fwrite(language_list, 1, len, out);
	for (unsigned long i = 0; i <= maxHashBuckets; ++i) Write32(hashbuckets[i], out);
	WORDP D = dictionaryBase;
	unsigned int n = 0;
	while (++D < dictionaryFree)
	{
		++n;
		WriteBinaryEntry(D, out);
	}
	printf("wrote %u binary dict to %u\r\n", ++n,Word2Index(dictionaryFree));
	char x[2];
	x[0] = x[1] = 0;
	fwrite(x, 1, 2, out); //   end marker for synchronization
	// add datestamp
	strcpy(dictionaryTimeStamp, GetMyTime(time(0)));
	fwrite(dictionaryTimeStamp, 1, strlen(dictionaryTimeStamp), out);
	FClose(out);

	int unused = 0;
	for (unsigned long i = 1; i <= maxHashBuckets; ++i) if (!hashbuckets[i]) ++unused;
	(*printer)((char*)"wrote binary dictionary %d buckets, %ld words written, %d unused buckets\r\n", maxHashBuckets, (long int)(dictionaryFree - dictionaryBase - 1), unused);
}

static unsigned char Read8(FILE * in)
{
	if (in)
	{
		unsigned char x[1];
		return (fread(x, 1, 1, in) != 1) ? 0 : (*x);
	}
	else return *writePtr++;
}

static unsigned short Read16(FILE * in)
{
	if (in)
	{
		unsigned char x[2];
		return (fread(x, 1, 2, in) != 2) ? 0 : ((*x) | (x[1] << 8));
	}
	else
	{
		unsigned int n = *writePtr++;
		return (unsigned short)(n | (*writePtr++ << 8));
	}
}

static unsigned int Read24(FILE * in)
{
	if (in)
	{
		unsigned char x[3];
		if (fread(x, 1, 3, in) != 3) return 0;
		return (*x) | ((x[1] << 8) | (x[2] << 16));
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		return n | (*writePtr++ << 16);
	}
}

unsigned int Read32(FILE * in)
{
	if (in)
	{
		unsigned char x[4];
		if (fread(x, 1, 4, in) != 4) return 0;
		unsigned int x3 = x[3];
		x3 <<= 24;
		return (*x) | (x[1] << 8) | (x[2] << 16) | x3;
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		n |= (*writePtr++ << 16);
		return n | (*writePtr++ << 24);
	}
}

uint64 Read64(FILE * in)
{
	if (in)
	{
		unsigned char x[8];
		if (fread(x, 1, 8, in) != 8) return 0;
		unsigned int x1, x2, x3, x4, x5, x6, x7, x8;
		x1 = x[0];
		x2 = x[1];
		x3 = x[2];
		x4 = x[3];
		x5 = x[4];
		x6 = x[5];
		x7 = x[6];
		x8 = x[7];
		uint64 a = x1 | (x2 << 8) | (x3 << 16) | (x4 << 24);
		uint64 b = x5 | (x6 << 8) | (x7 << 16) | (x8 << 24);
		b <<= 32;
		b <<= 16;
		a |= b;
		return a;
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		n |= (*writePtr++ << 16);
		n |= (*writePtr++ << 24);

		unsigned int n1 = *writePtr++;
		n1 |= (*writePtr++ << 8);
		n1 |= (*writePtr++ << 16);
		n1 |= (*writePtr++ << 24);

		uint64 ans = n1;
		ans <<= 32;
		return n | ans;
	}
}

WORDP ReadDWord(FILE * in)
{
	if (in)
	{
		unsigned char x[3];
		if (fread(x, 1, 3, in) != 3) return 0;
		unsigned int index = x[0] + (x[1] << 8) + (x[2] << 16);
		return Index2Word(index);
	}
	else
	{
		unsigned int n = *writePtr++;
		n |= (*writePtr++ << 8);
		n |= (*writePtr++ << 16);
		return Index2Word(n);
	}
}

static char* ReadString(FILE * in)
{
	unsigned int len = Read16(in);
	if (!len) return NULL;
	char* str;
	if (in)
	{
		char* limit;
		char* buffer = InfiniteStack(limit, "ReadString");
		if (fread(buffer, 1, len + 1, in) != len + 1) return NULL;
		if (buffer[0] == 0) 
			str = NULL;
		else str = AllocateHeap(buffer, len); // readstring
		ReleaseInfiniteStack();
	}
	else
	{
		str = AllocateHeap((char*)writePtr, len); // readstring
		writePtr += len + 1;
	}
	return str;
}

static WORDP ReadBinaryEntry(FILE * in)
{
	writePtr = (unsigned char*)readBuffer;
	unsigned int len = fread(writePtr, 1, 2, in);
	if (writePtr[0] == 0 && writePtr[1] == 0) return (WORDP)1;	//   normal ending in synch
	len = (writePtr[0] << 8) | writePtr[1];
	writePtr += 2;
	if (fread(writePtr, 1, len - 2, in) != len - 2) return NULL;

	unsigned int nameLen = *writePtr | (writePtr[1] << 8); // peek ahead
	char* name = ReadString(0);
	if (!name)
		return NULL;
	WORDP D = AllocateEntry(name);
	D->length = nameLen;
	echo = true;
	unsigned int bits = Read16(0);
	D->hash = Read64(0);
	if (bits & (1 << 6))
	{
		D->properties = Read64(0);
		D->systemFlags = Read64(0);
		D->parseBits = Read32(0);
	}

	if (bits & (1 << 9)) D->subjectHead = Read32(0);
	if (bits & (1 << 10)) D->verbHead = Read32(0);
	if (bits & (1 << 11)) D->objectHead = Read32(0);
	if (bits & (1 << 12))  // will be NONE at present
	{
		D->foreignFlags = (WORDP*)AllocateHeap(NULL, languageCount, sizeof(WORDP), true);
		for (unsigned int i = 0; i < languageCount; ++i)
		{
			D->foreignFlags[i] = Index2Word(Read32(0)) ;
		}
	}

	if (bits & (1 << 7)) D->internalBits = Read32(0);
	D->nextNode = Read32(0);
	if (D->internalBits & CONDITIONAL_IDIOM)
	{
		D->w.conditionalIdiom = Index2Word(Read32(0));
	}
	if (D->systemFlags & HAS_SUBSTITUTE)
	{
		D->w.substitutes = Index2Word(Read32(0));
	}
	if (bits & (1 << 0))
	{
		unsigned char c = Read8(0);
		SETMULTIWORDHEADER(D, c);
	}
	if (bits & (1 << 1))  SetTense(D, Read32(0));
	if (bits & (1 << 2)) SetPlural(D, Read32(0));
	if (bits & (1 << 3)) SetComparison(D, Read32(0));
	if (bits & 1 << 8)
	{
		MEANING M = Read32(0);
		SetCanonical(D, M);
	}
	if (bits & (1 << 4))
	{
		unsigned char c = Read8(0);
		MEANING* meanings = (MEANING*)AllocateHeap(NULL, (c + 1), sizeof(MEANING));
		memset(meanings, 0, (c + 1) * sizeof(MEANING));
		D->meanings = Heap2Index((char*)meanings);
		meanings[0] = c;
		for (unsigned int i = 1; i <= c; ++i)
		{
			meanings[i] = Read32(0);
			if (meanings[i] == 0)
			{
				ReportBug((char*)"binary entry meaning is null %s", name);
					return NULL;
			}
		}
	}
	if (bits & (1 << 5)) // glosses
	{
		unsigned char c = Read8(0);
		MEANING* glosses = (MEANING*)AllocateHeap(NULL, (2 * c + 1), sizeof(MEANING));
		memset(glosses, 0, (2 * c + 1) * sizeof(MEANING));
		glosses[0] = c;
		D->w.glosses = glosses;
		for (unsigned int i = 1; i <= c; ++i)
		{
			unsigned int index = Read8(0);
			char* string = ReadString(0);
			D->w.glosses[2 * i] = index;
			D->w.glosses[2*i - 1] = Heap2Index(string);
		}
	}
	if (Read8(0) != '0')
	{
		ReportBug("Bad Binary Dictionary entry, rebuild the binary dictionary %s\r\n", name);
		return NULL;
	}
	return D;
}

bool ReadBinaryDictionary()
{
	char* file = UseDictionaryFile((char*)"dict.bin",language_list);
	FILE* in = FopenStaticReadOnly(file);  // DICT
	if (!in) return false;
	unsigned int check = Read32(in); // version stamp
	if (check != CHECKSTAMP)
	{
		FClose(in);
		return false;
	}
	
	unsigned int size = Read32(in); // bucket size used
	if (size != maxHashBuckets) // if size has changed, rewrite binary dictionary
	{
		ReportBug((char*)"Binary dictionary uses hash=%d but system is using %d -- rebuilding binary dictionary\r\n", size, maxHashBuckets);
		return false;
	}
	
	// confirm same languages in same order 
	unsigned char list[100];
	fread(list, 1, 1, in);
	size_t len = list[0];
	fread(list, 1, len, in);
	if (strncmp(language_list,(char*)list, len))
	{
		FClose(in);
		return false;
	}

	ClearWordMaps();

	for (unsigned long i = 0; i <= maxHashBuckets; ++i)
	{
		hashbuckets[i] = Read32(in);
	}
	dictionaryFree = dictionaryBase + 1;
	WORDP end;
	while ((end = ReadBinaryEntry(in)))
	{
		if (end == (WORDP)1) break; // normal end
	}
	if (end) fread(dictionaryTimeStamp, 1, 20, in);
	FClose(in);
	return (end) ? true : false;
}

char* WriteDictionaryFlags(WORDP D, char* outbuf)
{
	*outbuf = 0;
	if (D->internalBits & DEFINES) return outbuf; // they dont need explaining, loaded before us
	uint64 properties = D->properties;
	uint64 bit = START_BIT;
	// write essentials first, refinements second
	while (properties && bit)
	{
		if (properties & bit)
		{
			properties ^= bit;
			char* label = FindNameByValue(bit);
			if (label)
			{
				sprintf(outbuf, (char*)"%s ", label);
				outbuf += strlen(outbuf);
			}
		}
		bit >>= 1;
	}

	properties = D->systemFlags;
	bit = START_BIT;
	bool posdefault = false;
	bool agedefault = false;
	while (properties)
	{
		if (properties & bit)
		{
			char* label = NULL;
			if (bit & ESSENTIAL_FLAGS)
			{
				if (!posdefault)
				{
					if (D->systemFlags & NOUN) sprintf(outbuf, (char*)"%s", (char*)"posdefault:NOUN ");
					outbuf += strlen(outbuf);
					if (D->systemFlags & VERB) sprintf(outbuf, (char*)"%s", (char*)"posdefault:VERB ");
					outbuf += strlen(outbuf);
					if (D->systemFlags & ADJECTIVE) sprintf(outbuf, (char*)"%s", (char*)"posdefault:ADJECTIVE ");
					outbuf += strlen(outbuf);
					if (D->systemFlags & ADVERB) sprintf(outbuf, (char*)"%s", (char*)"posdefault:ADVERB ");
					outbuf += strlen(outbuf);
					if (D->systemFlags & PREPOSITION) sprintf(outbuf, (char*)"%s", (char*)"posdefault:PREPOSITION ");
					outbuf += strlen(outbuf);
					posdefault = true;
				}
			}
			else if (bit & AGE_LEARNED)
			{
				if (!agedefault)
				{
					agedefault = true;
					uint64 age = D->systemFlags & AGE_LEARNED;
					if (age == KINDERGARTEN) sprintf(outbuf, (char*)"%s", (char*)"KINDERGARTEN ");
					else if (age == GRADE1_2) sprintf(outbuf, (char*)"%s", (char*)"GRADE1_2 ");
					else if (age == GRADE3_4) sprintf(outbuf, (char*)"%s", (char*)"GRADE3_4 ");
					else if (age == GRADE5_6) sprintf(outbuf, (char*)"%s", (char*)"GRADE5_6 ");
					outbuf += strlen(outbuf);
				}
			}
			else label = FindSystemNameByValue(bit);
			properties ^= bit;
			if (label)
			{
				sprintf(outbuf, (char*)"%s ", label);
				outbuf += strlen(outbuf);
			}
		}
		bit >>= 1;
	}

	properties = D->parseBits;
	bit = START_BIT;
	while (properties)
	{
		if (properties & bit)
		{
			properties ^= bit;
			char* label = FindParseNameByValue(bit);
			if (label)
			{
				sprintf(outbuf, (char*)"%s ", label);
				outbuf += strlen(outbuf);
			}
		}
		bit >>= 1;
	}
	if (D->internalBits & PREFER_THIS_UPPERCASE)
	{
		sprintf(outbuf, (char*)"%s ", "PREFER_THIS_UPPERCASE");
		outbuf += strlen(outbuf);
	}
	return outbuf;
}

char* GetGloss(WORDP D, unsigned int index)
{
	index = GetGlossIndex(D, index); // if we have no gloss for this, index will go to 0
	if (!index) return NULL;
	char* p = (char*)Index2Heap(D->w.glosses[index]);
	return p;
}

unsigned int GetGlossIndex(WORDP D, unsigned int index)
{
	unsigned int count = GetGlossCount(D);
	MEANING* glosses = D->w.glosses;
	for (unsigned int i = 1; i <= count; ++i)
	{
		if (glosses[2 * i] == index) return (2*i) - 1 ; // index 1 = 1 heapptr 2 index  return h
	}
	return 0;
}

char* ReadDictionaryFlags(WORDP D, char* ptr, unsigned int* meaningcount, unsigned int* glosscount)
{
	char junk[MAX_WORD_SIZE];
	ptr = ReadCompiledWord(ptr, junk);
	uint64 properties = D->properties;
	uint64 flags = D->systemFlags;
	uint64 parsebits = D->parseBits;
	while (*junk && *junk != ')')		//   read until closing paren
	{
		if (!strncmp(junk, (char*)"meanings=", 9))
		{
			if (meaningcount) *meaningcount = atoi(junk + 9);
		}
		else if (!strncmp(junk, (char*)"glosses=", 8))
		{
			if (glosscount) *glosscount = atoi(junk + 8);
		}
		else if (!strncmp(junk, (char*)"poscondition=", 13))
		{
			if (!strstr(junk, "null")) // null condition is boring
			{
				ptr = ReadCompiledWord(junk + 13, junk);
				D->w.conditionalIdiom = StoreWord(junk,AS_IS);
				AddInternalFlag(D, CONDITIONAL_IDIOM);
			}
		}
		else if (!strncmp(junk, (char*)"#=", 2));
		else if (!strcmp(junk, (char*)"posdefault:NOUN")) flags |= NOUN;
		else if (!strcmp(junk, (char*)"posdefault:VERB")) flags |= VERB;
		else if (!strcmp(junk, (char*)"posdefault:ADJECTIVE")) flags |= ADJECTIVE;
		else if (!strcmp(junk, (char*)"posdefault:ADVERB")) flags |= ADVERB;
		else if (!strcmp(junk, (char*)"posdefault:PREPOSITION")) flags |= PREPOSITION;
		else if (!strcmp(junk, (char*)"CONCEPT")) {}
		else if (!strcmp(junk, (char*)"TOPIC")) AddInternalFlag(D, TOPIC); // only from a fact dictionary entry
		else if (!strcmp(junk, (char*)"PREFER_THIS_UPPERCASE")) AddInternalFlag(D, PREFER_THIS_UPPERCASE); // only from a fact dictionary entry
		else if (!stricmp(junk, "null")) {;}
		else
		{
			if (!strcmp(junk, "null)"))
			{
				ptr = ReadCompiledWord(ptr, junk);
				continue; // BUG, ignoring
			}
			size_t len = strlen(junk);
			if (junk[len - 1] == ')')
			{
				junk[len - 1] = 0; // closing parent touching token, remove it
				ptr -= 2;
			}
			uint64 val = FindPropertyValueByName(junk);
			if (val) properties |= val;
			else
			{
				val = FindSystemValueByName(junk);
				if (val) flags |= val;
				else
				{
					val = FindParseValueByName(junk);
					if (val) parsebits |= val;
					else if (stricmp(current_language, "Spanish")) {}
					else if (xbuildDictionary) ReportBug((char*)"Failed to find system value %s", junk);
				}
			}
		}
		ptr = ReadCompiledWord(ptr, junk);
	}
	if (!(D->internalBits & DEFINES))
	{
		AddProperty(D, properties); // dont override existing define value
		AddSystemFlag(D, flags);
		AddParseBits(D, (unsigned int)parsebits);
	}
	return ptr;
}

void AddGloss(WORDP D, char* glossy, unsigned int index) // only a synset head can have a gloss
{
	//   cannot add gloss to entries before the freeze (space will free up when transient space chopped back but pointers will be bad).
	//   If some dictionary words cannot add after the fact, none should
	if (dictionaryLocked) return;
	if (D->internalBits & CONDITIONAL_IDIOM) return;	// shared ptr with gloss, not allowed gloss any more
#ifndef NOGLOSS
	MEANING gloss = Heap2Index(glossy) | (index << 24);
	MEANING* glosses = D->w.glosses;
	//   if we run out of room, reallocate gloss space double in size (ignore the hole in memory)
	unsigned int oldCount = GetGlossCount(D);

	//prove we dont already have this here
	for (unsigned int i = 1; i <= oldCount; ++i) if (glosses[i] == gloss) return;

	unsigned int count = oldCount + 1;
	if (!(count & oldCount)) //   new count has no bits in common with old count, is a new power of 2
	{
		glosses = (MEANING*)AllocateHeap(NULL, (count << 1), sizeof(MEANING));
		memset(glosses, 0, (count << 1) * sizeof(MEANING)); //   just to be purist
		memcpy(glosses + 1, D->w.glosses + 1, oldCount * sizeof(MEANING));
		D->w.glosses = glosses;
	}
	*glosses = count;
	glosses[count] = gloss;
#endif
}

static MEANING AddTypedMeaning(WORDP D, unsigned int type)
{
	unsigned int count = 1 + GetMeaningCount(D);
	MEANING M = MakeTypedMeaning(D, count, SYNSET_MARKER | type); // this points to synset head
	return AddMeaning(D, M);
}

MEANING AddMeaning(WORDP D, MEANING M)
{ //   worst case wordnet meaning count = 75 (break). We limit 62
	//   meaning is 1-based (0 means generic)
	//   cannot add meaning to entries before the freeze (space will free up when transient space chopped back but pointers will be bad).
	//   If some dictionary words cannot add after the fact, none should
	//   Meanings disambiguate multiple POS per word. User not likely to be able to add words that have
	//   multiple parts of speech.
	if (dictionaryLocked) return 0;
	//   no meaning given, use self with meaning one
	if (!(((ulong_t)M) & MAX_DICTIONARY)) M |= MakeMeaning(D, (1 + GetMeaningCount(D))) | SYNSET_MARKER;
	//   meanings[0] is always the count of existing meanings
	//   Actual space available is always a power of 2.
	MEANING* meanings = GetMeanings(D);
	//   if we run out of room, reallocate meaning space double in size (ignore the hole in memory)
	unsigned int oldCount = GetMeaningCount(D);
	if (oldCount == MAX_MEANING) 
		return 0; // refuse more -- (break and cut)

	//prove we dont already have this here
	for (unsigned int i = 1; i <= oldCount; ++i)
	{
		if (meanings[i] == M) return M;
	}

	unsigned int count = oldCount + 1;
	if (!(count & oldCount)) //   new count has no bits in common with old count, is a new power of 2
	{
		meanings = (MEANING*)AllocateHeap(NULL, (count << 1), sizeof(MEANING));
		memset(meanings, 0, (count << 1) * sizeof(MEANING)); //   just to be purist
		memcpy(meanings + 1, &GetMeanings(D)[1], oldCount * sizeof(MEANING));
		D->meanings = Heap2Index((char*)meanings);
	}
	meanings[0] = count;

	return meanings[count] = M;
}

MEANING GetMaster(MEANING T)
{ //   for a specific meaning return node that is master or return general node if all fails.
	if (!T) return 0;
	WORDP D = Meaning2Word(T);
	int index = Meaning2Index(T);
	if (!GetMeaningCount(D)) return MakeMeaning(D, index); // has none, all erased
	if (index > GetMeaningCount(D))
	{
		return MakeMeaning(D, 0);
	}
	if (index == 0) return T; // already at master

	MEANING old = T;
	MEANING at = GetMeanings(D)[index];
	unsigned int n = 0;
	while (!(at & SYNSET_MARKER)) // find the correct ptr to return. The marked ptr means OUR dict entry is master, not that the ptr points to.
	{
		old = at;
		WORDP X = Meaning2Word(at); // safe access to smaller annotated items
		int ind = Meaning2Index(at);
        if (ind == 0) return old; // run out of meanings, hit limit
		if (ind > GetMeaningCount(X))
		{
			ReportBug((char*)"synset master failure %s", X->word);
				return old;
		}
		MEANING* meanings = GetMeanings(X);
		if (meanings) at = meanings[ind];
		else at = 0;
		if (++n >= 20) break; // force an end arbitrarily
	}
	return old & SIMPLEMEANING; // never return the type flags or synset marker -- FindChild loop wont want them nor will fact creation.
}

MEANING* GetMeanings(WORDP D)
{
	if (!D) return 0;
	return (MEANING*)Index2Heap(D->meanings);
}

void RemoveMeaning(MEANING M, MEANING M1)
{
	M1 &= STDMEANING;

	//   remove meaning and keep only valid main POS values (may not have a synset ptr when its irregular conjugation or such)
	WORDP D = Meaning2Word(M);
	for (int i = 1; i <= GetMeaningCount(D); ++i)
	{
		if ((GetMeaning(D, i) & STDMEANING) == M1) // he points to ourself
		{
			GetMeanings(D)[i] = 0;
			unsigned int g = GetGlossIndex(D, i);
			if (g) D->w.glosses[g] = D->w.glosses[g+1] = 0; // kill the gloss pair also if any
		}
	}
}

MEANING ReadMeaning(char* word, bool create, bool precreated)
{// be wary of multiple deletes of same word in low-to-high-order. And infiniteStack may be in effect
	if (!*word) return 0;
	if (*word == '\\' && word[1] && !word[2])  ++word;
	char* hold = AllocateBuffer();
	strcpy(hold, word);
	word = hold;
	unsigned int oldlanguage = language_bits;
	char* language = strstr(hold, "~l");
	if (language &&  language[2] >= '0' && language[2] <= '7') // will be last after pos data if any
	{
		language_bits = atoi(language + 2) << LANGUAGE_SHIFT;
		*language = 0;
	}

	unsigned int flags = 0;
	unsigned int index = 0;

	char* at = (*word != '~') ? strchr(word, '~') : NULL;
	if (at && *word != '"') // beware of topics or other things, dont lose them. we want xxx~n (one character) or  xxx~digits  or xxx~23n
	{
		char* space = strchr(word, ' ');
		if (space && space < at) { ; } // not a normal word
		else if (IsDigit(at[1]))  // number starter  at~3  or   at~3n
		{
			index = atoi(at + 1);
			char* p = at;
			while (IsDigit(*++p)); // find end
			if (*p == 'n') flags = NOUN;
			else if (*p == 'v') flags = VERB;
			else if (*p == 'a') flags = ADJECTIVE;
			else if (*p == 'b') flags = ADVERB;
			else if (*p == 'z') flags |= SYNSET_MARKER;
			if (flags)
			{
				++p;
				if (*p == 'z')
				{
					flags |= SYNSET_MARKER;
					++p;
				}
			}
			if (*p) // this is NOT a normal word but some pattern or whatever
			{
				WORDP D = (create) ? StoreWord(word, AS_IS) : FindWord(word, 0, PRIMARY_CASE_ALLOWED);
				FreeBuffer();
				return (!D) ? (MEANING)0 : (MakeMeaning(D, index) | flags);
			}
			*at = 0; // drop the tail
		}
		if (index == 0) //   at~nz
		{
			if (at[2] && at[2] != ' ' && at[2] != 'z') { ; } // insure flag only the last character - write meaning can write multiple types, but only for internal display. externally only 1 type at a time is allowed to be input
			else if (at[1] == 'n') flags = NOUN;
			else if (at[1] == 'v') flags = VERB;
			else if (at[1] == 'a') flags = ADJECTIVE;
			else if (at[1] == 'b') flags = ADVERB;
			if (at[1] == 'z' || at[2] == 'z') flags |= SYNSET_MARKER;
			if (flags) *at = 0;
		}
	}
	if (*word == '"')
	{
		if (!precreated)
		{
			JoinWords(BurstWord(word, CONTRACTIONS), false, hold);
			word = hold;
		}
		else if (word[1] == FUNCTIONSTRING) { ; } // compiled script string
		else // some other quoted thing (like conceptpattern), strip off the quotes, becomes raw text
		{
			if (!word[1])
			{
				FreeBuffer();
				return (MEANING)0;	 // just a " is bad
			}
			memmove(hold, word + 1, strlen(word)); // the system should already have this correct if reading a file. dont burst and rejoin
			size_t len = strlen(hold);
			hold[len - 1] = 0;	// remove ending dq
			word = hold; // hereinafter, this fact will be written out as `xxxx` instead
		}
	}
	WORDP D;
	if (create) D = StoreWord(word, AS_IS);
	else
	{
		D= FindWord(word, 0, PRIMARY_CASE_ALLOWED,true); // find the language specific version
		if (!D) D = FindWord(word, 0, STANDARD_LOOKUP, false); // find  any casing
	}
	FreeBuffer();
	language_bits = oldlanguage;
	if (index > MAX_MEANING) index = 0; // generic reference we cant use
	MEANING Z = MakeMeaning(D, index);
	return (!D) ? (MEANING)0 : (Z | flags);
}

bool ReadDictionary(char* file)
{
	char word[MAX_WORD_SIZE];
	char* ptr;
	char* equal;
	FILE* in = FopenReadOnly(file); // DICT text dictionary file
	if (!in) return false;
	while (ReadALine(readBuffer, in) >= 0)
	{
		ptr = readBuffer;
		if (HasUTF8BOM(ptr)) ptr += 3;
		ptr = SkipWhitespace(ptr);
		if (!*ptr) continue;
		if (*ptr == '(' && ptr[1] != ' ') continue; //bad idea (_A_)(CONDITIONAL_IDIOM poscondition = (null))
		char* lemma = strstr(ptr, "lemma=");
		if (lemma)
		{
			*lemma = 0; // incase lemma has ( in it
			if (!lemma[6]) // none was actually given
				lemma = 0;
		}
		char* end = strchr(ptr, ' '); // end of word
		*end = 0;
		strcpy(word, ptr);
		if (!*word) continue;
		end[0] = ' ';
		if (lemma) *lemma = 'l';
		WORDENTRY WORD;
		WORDP D = &WORD;
		memset(D, 0, sizeof(WORDENTRY));

		unsigned int meaningCount = 0;
		unsigned int glossCount = 0;
		// for foreignlanguages, an entry like  `abad`abar` NC  VLfin       
		// means choice of lemmas with the types after it (these are lost for now)
		ptr = ReadDictionaryFlags(D, end + 2, &meaningCount, &glossCount);
		//   precreate meanings...

		// we dont want entries that have no useful data
		if (!D->properties && !D->systemFlags)  continue;
		WORDP hold = D;
		// a foreign verb with no tense bits and no lemma will be canonical infinitiv
		if (stricmp(current_language, "english") &&
			hold->properties & VERB && !(hold->properties & VERB_BITS) && !lemma)
			hold->properties |= VERB_INFINITIVE;
		D = GetLanguageWord(word); // unique word
		size_t len = strlen(word);
		if (stricmp(D->word, word)) 
			ReportBug((char*)"Dictionary read does not match original %s %s\r\n", D->word, word);
		D->properties |= hold->properties;
		D->systemFlags |= hold->systemFlags;
		D->parseBits = hold->parseBits;
	
		//   read cross-reference attribute ptrs
		while (*ptr)		//   read until closing paren
		{
			char* at = SkipWhitespace(ptr);
			ptr = ReadCompiledWord(ptr, word);
			if (!*word) break;
			equal = strchr(word, '=');
			*equal++ = 0;
			if (!strcmp(word, (char*)"conjugate")) { SetTense(D, MakeMeaning(GetLanguageWord(equal))); } // only happens with english
			else if (!strcmp(word, (char*)"plural")) { SetPlural(D, MakeMeaning(GetLanguageWord(equal))); } // only happens with english
			else if (!strcmp(word, (char*)"comparative")) { SetComparison(D, MakeMeaning(GetLanguageWord(equal))); } // only happens with english
			else if (!strcmp(word, (char*)"lemma")) // foreign languages only
			{
				char copy[MAX_WORD_SIZE];
				char* lemma = TrimSpaces(at + 6);
				if (!*lemma) break; // claimed but wasnt there
				strcpy(copy, lemma);
				
				// getcanonical equivalent without pulling out just 1 tag
				// if a complex lemma has already been assigned, dont overwrite it
				WORDP Z = GetCanonical(D,(uint64)-1); // get raw form
				if (Z && strchr(Z->word, '`')) break; // dont override lemma

				char* end1 = strchr(lemma + 1, '`'); // first end
				if (!end1)
				{
					SetCanonical(D, MakeMeaning(GetLanguageWord(copy))); // multiple choice tells lemmas and which pos in order
					continue;
				}
				char* end2 = strchr(end1 + 1, '`'); // maybe 2nd end
				char* end3 = NULL;
				char* end4 = NULL;
				char* end5 = NULL;
				char* end6 = NULL;
				if (end2) end3 = strchr(end2 + 1, '`');
				if (end3) end4 = strchr(end3 + 1, '`');
				if (end4) end5 = strchr(end4 + 1, '`');
				if (end5) end6 = strchr(end5 + 1, '`');
				*end1 = 0;
				if (end2) *end2 = 0;
				if (end3) *end3 = 0;
				if (end4) *end4 = 0;
				if (end5) *end5 = 0;
				if (end6) *end6 = 0;
				if (end6 && !strcmp(end5 + 1, end1 + 1)) end5 = NULL;
				if (!end6 && end5 && !strcmp(end4 + 1, end1 + 1)) end4 = NULL;
				if (!end5 && end4 && !strcmp(end3 + 1, end1 + 1)) end4= NULL;
				if (!end4 && end3 && !strcmp(end2 + 1, end1 + 1)) end3 = NULL;
				if (!end3 && end2 && !strcmp(lemma + 1, end1 + 1)) end2 = NULL;
				if (!end2) SetCanonical(D, MakeMeaning(GetLanguageWord(lemma+1))); // single word
				else SetCanonical(D, MakeMeaning(GetLanguageWord(copy))); // multiple choice tells lemmas and which pos in order
				break; // swallow rest of line
			}
		}
		//   directly create meanings, since we know the size-- no meanings may be added after this
		if (meaningCount) 
		{
			MEANING* meanings = (MEANING*)AllocateHeap(NULL, (meaningCount + 1), sizeof(MEANING), true);
			meanings[0] = 0;
			D->meanings = Heap2Index((char*)meanings);
			unsigned int glossIndex = 0;
			//   directly create gloss space, since we know the size-- no glosses may be added after this
			if (glossCount) // reserve space for this many glosses and their indices
			{
				MEANING* glosses = (MEANING*)AllocateHeap(NULL, (2 * glossCount + 1), sizeof(MEANING), true);
				D->w.glosses = glosses;
				glosses[0] = 0;
			}
			int meaningindex = 0;
			for (unsigned int i = 1; i <= meaningCount; ++i) //   read each meaning
			{
				char reference[MAX_WORD_SIZE];
				ReadALine(readBuffer, in);
				char* p = ReadCompiledWord(readBuffer, reference); // the meaning we link to
				meanings[++meaningindex] = ReadMeaning(reference, true, true);
				char* tilde = strchr(reference, '~');
				if (tilde)
				{
					int index = atoi(tilde + 1);
					// if (index > MAX_MEANING) we are referring to someone who cannot be named
					// we store the reference but later cant use it
				}
				if (*p == '(') p = strchr(p, ')') + 2; // point after the ), an uplink reference on synset master
				if (glossCount && *p) 
				{
					D->w.glosses[++glossIndex] = Heap2Index(AllocateHeap(p)); // 1=ptr
					D->w.glosses[++glossIndex] = meaningindex; // 2 = index (1)
				}
			}
			meanings[0] = meaningindex; // meanings we could store
			if (glossIndex) D->w.glosses[0] = (MEANING)(glossIndex/2); // glosses we stored

		}
	}
	FClose(in);
	return true;
}

MEANING MakeTypedMeaning(WORDP x, unsigned int y, unsigned int flags)
{
	return (!x) ? 0 : (((MEANING)(Word2Index(x) + (unsigned int)(((uint64)y) << INDEX_OFFSET))) | (flags << TYPE_RESTRICTION_SHIFT));
}

MEANING MakeMeaning(WORDP x, unsigned int y) //   compose a meaning
{
	return (!x) ? 0 : (((MEANING)(Word2Index(x) + (unsigned int)(((uint64)y) << INDEX_OFFSET))));
}

WORDP Meaning2Word(MEANING x) //   convert meaning to its dictionary entry
{
	x &= MAX_DICTIONARY;
	WORDP D = Index2Word(x);
	return D;
}

unsigned int GetMeaningType(MEANING T)
{
	if (T == 0) return 0;
	WORDP D = Meaning2Word(T);
	unsigned int index = Meaning2Index(T);
	if (index) T = GetMeaning(D, index); //   change to synset head for specific meaning
	else if (GETTYPERESTRICTION(T)) return (int)GETTYPERESTRICTION(T); //   generic word type it must be
	D = Meaning2Word(T);
	return (unsigned int)(D->properties & PART_OF_SPEECH);
}

MEANING FindSynsetParent(MEANING T, unsigned int which) //  presume we are at the master, next wordnet
{
	WORDP D = Meaning2Word(T);
	unsigned int index = Meaning2Index(T);
	FACT* F = GetSubjectNondeadHead(D); //   facts involving word 
	unsigned int count = 0;
	while (F)
	{
		FACT* at = F;
		F = GetSubjectNondeadNext(F);
		if (at->verb == Mis) // wordnet meaning
		{
			//   prove indexes mate up
			if (index && index != Meaning2Index(at->subject)) continue; // must match generic or specific precisely
			if (count++ == which)
			{
				if (TraceHierarchyTest(trace)) TraceFact(at);
				return at->object; //   next set/class in line
			}
		}
	}
	return 0;
}

MEANING FindSetParent(MEANING T, int n) //   next set parent
{
	WORDP D = Meaning2Word(T);
	unsigned int index = Meaning2Index(T);
	uint64 restrict = GETTYPERESTRICTION(T);
	if (!restrict && index) restrict = GETTYPERESTRICTION(GetMeaning(D, index));
	FACT* F = GetSubjectNondeadHead(D); //   facts involving word 
	while (F)
	{
		FACT* at = F;
		F = GetSubjectNondeadNext(F);
		if (!(at->verb == Mmember)) continue;

		//   prove indexes mate up
		unsigned int localIndex = Meaning2Index(at->subject); //   what fact says
		if (index != localIndex && localIndex) continue; //   must match generic or specific precisely
		if (restrict && GETTYPERESTRICTION(at->subject) && !(GETTYPERESTRICTION(at->subject) & restrict)) continue; // not match type restriction 

		if (--n == 0)
		{
			if (TraceHierarchyTest(trace)) TraceFact(F);
			return at->object; // next set/class in line
		}
	}
	return 0;
}

void SuffixMeaning(MEANING T, char* at, bool withPos)
{
	WORDP D = Meaning2Word(T);
	//   index 
	unsigned int index = Meaning2Index(T);
	if (index > 9)
	{
		*at++ = '~';
		*at++ = (char)((index / 10) + '0');
		*at++ = (char)((index % 10) + '0');
	}
	else if (index)
	{
		*at++ = '~';
		*at++ = (char)(index + '0');
	}

	if (withPos)
	{
		unsigned int type = GETTYPERESTRICTION(T);
		if (type && !index) *at++ = '~';	// pos marker needed on generic
		if (type & NOUN) *at++ = 'n';
		else if (type & VERB) *at++ = 'v';
		else if (type & (ADJECTIVE | ADVERB)) *at++ = 'a';
	}
	if (T & SYNSET_MARKER) *at++ = 'z';
	*at = 0;

	if ( multidict && GET_LANGUAGE_INDEX(D) && !(D->internalBits & UNIVERSAL_WORD)  )
	{
		sprintf(at, "~l%u", D->internalBits >> LANGUAGE_SHIFT);  // add language request
		at += strlen(at);
	}
}

unsigned int GETTYPERESTRICTION(MEANING x)
{
	unsigned int type = ((x) >> TYPE_RESTRICTION_SHIFT) & TYPE_RESTRICTION;
	if (type & ADJECTIVE) type |= ADVERB;
	return type;
}

char* WriteMeaning(MEANING T, bool withPos, char* buf)
{ // always annotate with language, maybe annotate with pos info
	char* answer = NULL;
	WORDP D = Meaning2Word(T);
	if (!D->word)
	{
		ReportBug("Missing word on D (T=%d)\r\n", T);
		answer = "deadcow";
	}
	else answer = D->word; // additional data on T
	if (!buf) buf = AllocateStack(answer, strlen(answer) + 20); // leave room and let it truncate somehow later
	if (!*answer) strcpy(buf, "``");
	else strcpy(buf, answer);
	SuffixMeaning(T, buf +strlen(buf), (T & MEANING_BASE) != T);
	return buf;
}

char* GetWord(char* word)
{
	WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
	if (D) return D->word;
	return NULL;
}

void NoteLanguage()
{
	FILE* out = FopenUTF8Write((char*)"language.txt");
	fprintf(out, (char*)"%s\r\n", current_language); // current default language
	FClose(out);
}

void UndoSubstitutes(HEAPREF list)
{
	while (list)
	{
		uint64 val1, val2, val3;
		list = UnpackHeapval(list, val1, val2, val3);
		// leader will have 0 as val3 on 1st of 2 links, recipient not
		if (val3 == 0) //substitutor
		{
			//list = AllocateHeapval(HV1_WORDP|HV2_WORDP,list, (uint64)D, (uint64)D->w.substitutes,); // note old status of this word as substitute original
			//list = AllocateHeapval(HV1_WORDP|HV2_INT|HV3_INT,list, (uint64)D, D->systemFlags, (uint64)D->internalBits); // note old status of this word as substitute original
			WORDP D = (WORDP)val1;
			D->w.substitutes = (WORDP)val2;
			list = UnpackHeapval(list, val1, val2, val3);
			D->systemFlags = val2;
			D->internalBits = (unsigned int)val3;
		}
		else if (val3 == 1) // multiword header
		{
			//list = AllocateHeapval(HV1_WORDP|HV2_INT|HV3_INT,list, (uint64)E, E->nextNode, (uint64)1); // note old status of this word as substitute original
			WORDP D = (WORDP)val1;
			D->nextNode = (unsigned int)val2;
		}
		// we are the substitutee
		else
		{
			//list = AllocateHeapval(HV1_WORDP|HV2_INT,list, S->properties, S->systemFlags, 0); // note old status of this word result
			WORDP D = WORDP(val3);
			D->properties = val1;
			D->systemFlags = val2;
		}
	}
}

#pragma warning(disable: 4068)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wbitwise-op-parentheses"

static void AddSub2Dict(char* original)
{ // protect subwords of a substitution from spellcheck
	char copy[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	strcpy(copy, original);
	char* at = copy;
	while ((at = strchr(at, SUBSTITUTE_SEPARATOR)) != NULL) *at = ' '; // separate words
	at = copy;
	while (*at && (at = ReadToken(at, word)))
	{
		if (!*word) continue; // ignore this errors
		WORDP D = StoreWord(word, AS_IS); 
		AddSystemFlag(D, PATTERN_WORD);
	}
}

HEAPREF SetSubstitute(char* originalx, char* replacementx, unsigned int build, unsigned int fileFlag, HEAPREF list)
{
	unsigned int oldlanguagebits = language_bits;
	unsigned int language1 = LanguageRestrict(originalx);
	char originaly[MAX_WORD_SIZE];
	strcpy(originaly, originalx);
	char* original = originaly;
	if (*original == '"') // remove double quotes
	{
		size_t len = strlen(original) - 1;
		if (original[len] == '"') original[len] = 0;
		++original;
	}
	if (*original == '_') ++original; // bad data, skip it
	char* at = original;
	while ((at = strchr(at, ' '))) *at = '_';	// change spaces to underscores (its what we do with quoted strings elsewhere)
	at = original;
	while ((at = strchr(at, '+')))
	{
		if (at[1] == 0) break;
		if (at[1] == '_') ++at; // real plus will have _ after it
		else *at = '_';	// change pluses to underscores
	}
	at = original;
	// alter possesive to new token
	while ((at = strstr(++at, "'s")))
	{
		if (at == original || *(at - 1) == '_') { ; }
		else if (at[2] == 0 || at[2] == '_')
		{
			memmove(at + 1, at, strlen(at)+1);
			*at = '_';
		}
	}
	at = original;
	while ((at = strstr(++at, "'")))
	{
		if (at == original || *(at - 1) == '_') {; }
		else if (at[1] == 0 || at[1] == '_')
		{
			memmove(at + 1, at, strlen(at)+1);
			*at = '_';
		}
	}
	at = original;
	while ((at = strchr(at, '_'))) *at++ = SUBSTITUTE_SEPARATOR;// make safe form so we can have _ in output
	// add each word to dict also, to protect from spellcheck changing
	AddSub2Dict(original);

	language_bits = language1;
	WORDP D = FindWord(original, 0, LOWERCASE_LOOKUP);	//   do we know original already? 

	char replacementy[MAX_WORD_SIZE];
	strcpy(replacementy, replacementx);
	char* replacement = replacementy;
	if (*replacement == '"')
	{
		size_t len = strlen(replacement) - 1;
		if (replacement[len] == '"') replacement[len] = 0;
		++replacement;
	}
	at = replacement;
	if (*at != '\'' && *at != '(' )  //  ' " xxx xxx" protects spaces except in patterns
	{
		while ((at = strchr(at, ' '))) *at = '+';	// change spaces to plus - leave _ to be single token
	}

	bool fromExists = false;
	if (D && D->systemFlags & HAS_SUBSTITUTE) // user allowed to override base but will get warning on load
	{
		fromExists = true;
		if (csapicall != TEST_PATTERN)
		{ // warn if multiple substitutes and we are different
			WORDP S = D->w.substitutes;
			if (S && stricmp(S->word, replacement))
				Log(ECHOUSERLOG, (char*)"Overriding substitute for %s which was %s and is now %s \r\n", original, S->word, replacement);
		}
	}
	D = StoreWord(original, AS_IS); //   original word (which will include _ word separators)
	if (D->internalBits & CONDITIONAL_IDIOM)
	{
		if (csapicall != TEST_PATTERN) (*printer)((char*)"BAD Substitute conflicts with conditional idiom %s\r\n", original);
		language_bits = oldlanguagebits;
		return list;
	}

	if (csapicall == TEST_PATTERN) // track change to undo
	{
		list = AllocateHeapval(HV1_WORDP|HV2_INT|HV3_INT,list, (uint64)D, D->systemFlags, (uint64)D->internalBits); // note old status of this word as substitute original
		list = AllocateHeapval(HV1_WORDP | HV2_WORDP,list, (uint64)D, (uint64)D->w.substitutes); // note old status of this word as substitute original
	}
	
	if (!fromExists) AddInternalFlag(D, fileFlag | build);
	AddSystemFlag(D, HAS_SUBSTITUTE);
	D->w.substitutes = NULL; // from is no longer a real word

	// set up multiword header matching for Substitutes processing
	char copy[MAX_WORD_SIZE];
	strcpy(copy, D->word);
	char* sep = strchr(copy, '|');
	if (sep) *sep = 0;
	char wd[MAX_WORD_SIZE];
	strcpy(wd, copy);
	if (sep) *sep = '|';
	// now count how many words in this sequence
	unsigned int n = 1;
	sep = copy;
	while ((sep = strchr(sep + 1, '|'))) ++n;

	// now determine the multiword headerness...
	char* myword = wd;
	if (*myword == '<') ++myword;		// do not show the < starter for lookup
	size_t len = strlen(myword);
	if (len > 1 && myword[len - 1] == '>')  myword[len - 1] = 0;	// do not show the > on the starter for lookup
	WORDP E = StoreWord(myword,AS_IS);		// create the 1-word header
	unsigned int h = GETMULTIWORDHEADER(E);
	if (n > h)
	{
		if (csapicall == TEST_PATTERN) // track change to undo
		{
			list = AllocateHeapval(HV1_WORDP|HV2_INT|HV3_INT,list, (uint64)E, E->nextNode, (uint64)1); // note old status of this word as substitute original
		}
		SETMULTIWORDHEADER(E, n);	//   mark it can go this far for an idiom
	}
	// note the replacement
	WORDP S = NULL;
	if (replacement[0] != 0 && replacement[0] != '#') 	//   with no substitute, it will just erase itself
	{
		WORDP T = FindWord(replacement,0,PRIMARY_CASE_ALLOWED,true);
		uint64 props = (T ? T->properties : 0);
		uint64 flags = SUBSTITUTE_RECIPIENT;

		// if new single word, replicate the properties of the source
		if (!T && *replacement != '~' && !strchr(replacement, '+') && D->properties > 0 && !IsDigitWord(replacement))
		{
			props |= D->properties;
			flags |= (D->systemFlags ^ HAS_SUBSTITUTE);
		}
		S = StoreWord(replacement,AS_IS); // it not a language word, its just text to replace but will be language specific
		D->w.substitutes = S;  //   the valid word
		if (csapicall == TEST_PATTERN)
		{
			list = AllocateHeapval(HV1_INT|HV2_INT|HV3_WORDP,list, S->properties, S->systemFlags, (uint64)S); // note old status of this word result
		}
		S->properties |= props;
		S->systemFlags |= flags; // wont->won't and won't->will+not
		// for the emotions (like ~emoyes) we want to be able to reverse access, so make them a member of the set also
		if (*S->word == '~')
		{
			int flags = (csapicall == TEST_PATTERN) ? FACTTRANSIENT : 0;
			if (*D->word == '<') flags |= START_ONLY;
			if (D->word[strlen(D->word) - 1] == '>') flags |= END_ONLY;
			CreateFact(MakeMeaning(D), Mmember, MakeMeaning(S), flags);
			strcpy(copy, D->word);
			char* ptr = copy;
			if (*ptr == '<')
			{
				++ptr;
				char* end = strrchr(ptr, '>');
				if (end) *end = 0;
				char* match = ptr;
				while ((match = strchr(match, SUBSTITUTE_SEPARATOR))) *match = '_';
				CreateFact(MakeMeaning(StoreWord(ptr)), Mmember, MakeMeaning(S), flags);
			}
		}
	}

	//   if original has hyphens, replace as single words also. Note burst form for initial marking will be the same
	bool hadHyphen = false;
	strcpy(copy, original);
	char* ptr = copy;
	while (*++ptr) // replace all alphabetic hypens using _
	{
		if (*ptr == '-' && IsAlphaUTF8(ptr[1]))
		{
			*ptr = '_';
			hadHyphen = true;
		}
	}
	if (hadHyphen)
	{
		language_bits = language1;
		D = FindWord(copy);	//   do we know original already?
		if (D && D->systemFlags & HAS_SUBSTITUTE && D->w.substitutes) // fastdict may have this marked but we have still to redo it with xrefs
		{
			if (csapicall != TEST_PATTERN && stricmp(D->w.substitutes->word, S->word)) ReportBug((char*)"Already have a different substitute yielding %s from %s so ignoring %s\r\n", S->word, original, readBuffer);
		}

		D = StoreWord(copy, AS_IS);
		if (csapicall == TEST_PATTERN)
		{
			list = AllocateHeapval(HV1_WORDP|HV2_INT|HV3_INT,list, (uint64)D, D->systemFlags, (uint64)D->internalBits); // note old status of this word as substitute original
			list = AllocateHeapval(HV1_WORDP|HV2_WORDP,list, (uint64)D, (uint64)D->w.substitutes); // note old status of this word as substitute original
		}
		AddInternalFlag(D, fileFlag);
		AddSystemFlag(D, HAS_SUBSTITUTE);
		// alternate is no longer a word either
		D->w.substitutes = S;
	}
	language_bits = oldlanguagebits;
	return list;
}
#pragma GCC diagnostic pop

void ReadSubstitutes(const char* name, unsigned int build, const char* layer, unsigned int fileFlag, bool filegiven)
{
	char file[SMALL_WORD_SIZE];
	if (layer) sprintf(file, "%s/%s", topicfolder, name);
	else if (filegiven) strcpy(file, name);
	else sprintf(file, (char*)"%s/%s/SUBSTITUTES/%s", livedataFolder, current_language, name);
	char original[MAX_WORD_SIZE];
	char replacement[MAX_WORD_SIZE];
	HEAPREF list = NULL;
	FILE* in = FopenStaticReadOnly(file); // LIVEDATA substitutes or from script TOPIC world
	if (!in && layer)
	{
		sprintf(file, "%s/BUILD%s/%s", topicfolder, layer, name);
		in = FopenReadOnly(file);
	}
	if (!in)
	{
		if (strstr(file, "british") && stricmp(current_language, "english")) {}
		return;
	}
	
	WORDP D;
	WORDP interjections = StoreWord("~interjections", AS_IS);
	
	// mark all concepts listed as interjections (only place we subsitute to a concept name)
	FACT* F = GetObjectHead(interjections); 
	while (F)
	{
		D = Meaning2Word(F->subject);
		if (F->verb == Mmember && *D->word == '~')     D->internalBits |= BEEN_HERE;
		F = GetObjectNext(F);
	}
	int oldlanguage = language_bits;
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* ptr = SkipWhitespace(readBuffer);
		if (*ptr == '#' || *ptr == 0) continue;
		ptr = ReadToken(ptr, original); //   original phrase
		if (original[0] == 0 || original[0] == '#') continue;
		ptr = ReadToken(ptr, replacement);    //   replacement phrase

		char pos[MAX_WORD_SIZE];
		if (fileFlag == DO_INTERJECTIONS) // treat as interjections (like end sentence punctuation)
		{
			ptr = ReadToken(ptr, pos);
			if (!stricmp(pos, "EMOJI"))
			{
				StoreWord(original, AS_IS | EMOJI);
			}
		}
		unsigned int flag = fileFlag;
		language_bits = oldlanguage;
		if (fileFlag == DO_PRIVATE && *replacement == '~')// private interjections will list as interjections file, instead of private file so you can disable privates or interjectsions appropriately
		{
			D = StoreWord(replacement, AS_IS);
			if (D->internalBits & BEEN_HERE) flag = DO_INTERJECTIONS;
		}
		SetSubstitute(original, replacement, build, flag, list);
	}
	// just ignore wasted heapref memory (compiling doesnt matter, data is small enough to ignore in private)
	FClose(in);
	language_bits = oldlanguage;

	// unmark all interjections
	F = GetObjectHead(interjections); 
	while (F)
	{
		D = Meaning2Word(F->subject);
		if (D) D->internalBits &= -1 ^ BEEN_HERE;
		F = GetObjectNext(F);
	}
}

void ReadWordsOf(char* name, uint64 mark)
{
	char file[SMALL_WORD_SIZE];
	sprintf(file, (char*)"%s/%s/SUBSTITUTES/%s", livedataFolder, current_language, name);
	char word[MAX_WORD_SIZE];
	FILE* in = FopenStaticReadOnly(file); // LOWERCASE TITLES LIVEDATA scriptcompile nonwords allowed OR lowercase title words
	if (!in) return;
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* ptr = ReadCompiledWord(readBuffer, word);
		if (*word != 0 && *word != '#')
		{
			WORDP D = StoreWord(word, mark);
			ReadCompiledWord(ptr, word);
			if (!stricmp(word, (char*)"FOREIGN_WORD")) AddProperty(D, FOREIGN_WORD);
		}
	}
	FClose(in);
}

void ReadCanonicals(const char* file, const char* layer)
{
	char original[MAX_WORD_SIZE];
	char replacement[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	if (layer) sprintf(word, "%s/%s", topicfolder, file);
	else strcpy(word, file);
	FILE* in = FopenStaticReadOnly(word); // LIVEDATA canonicals
	if (!in && layer)
	{
		sprintf(word, "%s/BUILD%s/%s", topicfolder, layer, file);
		in = FopenStaticReadOnly(word);
	}
	if (!in) return;

	int oldlanguage = language_bits;
	while (ReadALine(readBuffer, in) >= 0)
	{
		if (*readBuffer == '#' || *readBuffer == 0) continue;
		char* ptr = ReadToken(readBuffer, replacement ); //   canonical phrase
		language_bits = LanguageRestrict(replacement);
		WORDP C = StoreWord(replacement, 0);

		WORDP D = NULL;
		while (*ptr)
		{
			ptr = ReadToken(ptr, original);    //   original word
			if (*original == '#' || !*original) break; // comment or end of data
			WORDP D = StoreWord(original, 0);
			WORDP X = GetCanonical(D);
			if (X && X->word[1]) // we allow changing letters from LC to UC 
			{
				if (C != X) // trying to change
					printf("Warning- %s canonical repeat for original %s- with replacement %s  prior replacement %s\r\n", current_language,original, replacement,X->word);
			}
			else SetCanonical(D, MakeMeaning(C));
			char form[MAX_WORD_SIZE];
			ReadCompiledWord(ptr, form);
			if (!stricmp(form, "MORE_FORM"))
			{
				AddProperty(D, MORE_FORM);
				break;
			}
			else if (!stricmp(form, "MOST_FORM"))
			{
				AddProperty(D, MOST_FORM);
				break;
			}
		}
	}
	FClose(in);
	language_bits = oldlanguage;
}

void ReadLemmas(const char* file, const char* layer)
{ // for foreign language lists of words that are NOT canonicals except to CS (like the->a)
	char lemma[MAX_WORD_SIZE];
	char conjugate[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	if (layer) sprintf(word, "%s/%s", topicfolder, file);
	else strcpy(word, file);
	FILE* in = FopenStaticReadOnly(word); // LIVEDATA lemmas
	if (!in && layer)
	{
		sprintf(word, "%s/BUILD%s/%s", topicfolder, layer, file);
		in = FopenStaticReadOnly(word);
	}
	if (!in) return;

	while (ReadALine(readBuffer, in) >= 0)
	{
		if (*readBuffer == '#' || *readBuffer == 0) continue;
		char* ptr = ReadToken(readBuffer, lemma); //   original phrase
		while (ptr)
		{
			ptr = ReadToken(ptr, conjugate);    
		}

		WORDP D = StoreWord(lemma);
		WORDP R = StoreWord(conjugate);
		SetCanonical(D, MakeMeaning(R));
	}
	FClose(in);
}


void ReadAbbreviations(char* name)
{
	char file[SMALL_WORD_SIZE];
	sprintf(file, (char*)"%s/%s/SUBSTITUTES/%s", livedataFolder, current_language, name);
	char word[MAX_WORD_SIZE];
	FILE* in = FopenStaticReadOnly(file); // LIVEDATA/LANGUAGE/SUBSTITUTES/ abbreviations
	if (!in) return;
	while (ReadALine(readBuffer, in) >= 0)
	{
		ReadCompiledWord(readBuffer, word);
		if (*word != 0 && *word != '#')
		{
			WORDP D = StoreFlaggedWord(word, 0, KINDERGARTEN);
			RemoveInternalFlag(D, DELETED_MARK);
		}
	}
	FClose(in);
}

void ReadQueryLabels(char* name)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenStaticReadOnly(name); // LIVEDATA queries.txt
	if (!in)
	{
		Log(USERLOG, "** Missing querylabels file %s\r\n", name);
		return;
	}
	while (ReadALine(readBuffer, in) >= 0)
	{
		if (*readBuffer == '#' || *readBuffer == 0) continue;
		char* ptr = ReadCompiledWord(readBuffer, word);    // the name or query: name
		if (*word == 0) continue;
		if (!stricmp(word, (char*)"query:")) ptr = ReadCompiledWord(ptr, word);

		ptr = SkipWhitespace(ptr); // in case excess blanks before control string
		WORDP D = StoreWord(word);
		AddInternalFlag(D, QUERY_KIND);
		if (*ptr == '"') // allowed quoted string, remove quotes
		{
			ReadCompiledWord(ptr, word);
			ptr = word + 1;
			char* at = strchr(ptr, '"');
			if (at) *at = 0;
		}
		else // ordinary chars
		{
			char* at = strchr(ptr, ' '); // in case has blanks after control string
			if (at) *at = 0;
		}
		D->w.userValue = AllocateHeap(ptr);    // read query labels
	}
	FClose(in);
}

static void ReadPosPatterns(char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenStaticReadOnly(file); // ENGLISH folder
	if (!in) return;
	uint64* dataptr;

	dataptr = dataBuf + (tagRuleCount * MAX_TAG_FIELDS);
	memset(dataptr, 0, sizeof(uint64) * MAX_TAG_FIELDS);
	commentsData[tagRuleCount] = AllocateHeap(file); // put in null change of file marker for debugging
	++tagRuleCount;

	uint64 val;
	uint64 flags = 0;
	uint64 v = 0;
	uint64 offsetValue;
	char comment[MAX_WORD_SIZE];
	while (ReadALine(readBuffer, in) >= 0) // read new rule or comments
	{
		char* ptr = ReadCompiledWord(readBuffer, word);
		if (!*word) continue;
		if (*word == '#')
		{
			strcpy(comment, readBuffer);
			continue;
		}
		if (!stricmp(word, (char*)"INVERTWORDS")) // run sentences backwards
		{
			reverseWords = true;
			continue;
		}

		int c = 0;
		int offset;
		bool reverse = false;
		unsigned int limit = MAX_TAG_FIELDS;
		bool big = false;

		dataptr = dataBuf + (tagRuleCount * MAX_TAG_FIELDS);
		uint64* base = dataptr;
		memset(base, 0, sizeof(uint64) * 8);	// clear all potential rule info for a double rule
		bool skipped = false;
		if (!stricmp(word, (char*)"USED")) // either order
		{
			base[3] |= USED_BIT;
			ptr = ReadCompiledWord(ptr, word);
		}
		if (!stricmp(word, (char*)"TRACE"))
		{
			base[3] |= TRACE_BIT;
			ptr = ReadCompiledWord(ptr, word);
		}
		if (!stricmp(word, (char*)"USED")) // either order
		{
			base[3] |= USED_BIT;
			ptr = ReadCompiledWord(ptr, word);
		}
		bool backwards = false;

		if (!stricmp(word, (char*)"reverse"))
		{
			reverse = true;
			backwards = true;
			ptr = ReadCompiledWord(ptr, word);
			c = 0;
			if (!IsDigit(word[0]) && word[0] != '-')
			{
				(*printer)((char*)"Missing reverse offset  %s rule: %d comment: %s\r\n", word, tagRuleCount, comment);
				FClose(in);
				return;
			}
			c = atoi(word);
			if (c < -3 || c > 3) // 3 bits
			{
				(*printer)((char*)"Bad offset (+-3 max)  %s rule: %d comment: %s\r\n", word, tagRuleCount, comment);
				FClose(in);
				return;
			}
		}
		else if (!IsDigit(word[0]) && word[0] != '-') continue; // not an offset start of rule
		else
		{
			c = atoi(word);
			if (c < -3 || c > 3) // 3 bits
			{
				(*printer)((char*)"Bad offset (+-3 max)  %s rule: %d comment: %s\r\n", word, tagRuleCount, comment);
				FClose(in);
				return;
			}
		}
		offset = (reverse) ? (c + 1) : (c - 1);
		offsetValue = (uint64)((c + 3) & 0x00000007); // 3 bits offset
		offsetValue <<= OFFSET_SHIFT;
		int resultIndex = -1;
		unsigned int includes = 0;
		bool doReverse = reverse;

		for (unsigned int i = 1; i <= (limit * 2); ++i)
		{
			unsigned int kind = 0;
			if (reverse)
			{
				reverse = false;
				--offset;
			}
			else ++offset;
			flags = 0;
		resume:
			// read control for this field
			ptr = ReadCompiledWord(ptr, word);
			if (!*word || *word == '#')
			{
				ReadALine(readBuffer, in);
				ptr = readBuffer;
				--i;
				continue;
			}
			if (!stricmp(word, (char*)"debug"))  // just a debug label so we can stop here if we want to understand reading a rule
				goto resume;

			// LAST LINE OF PATTERN
			if (!stricmp(word, (char*)"KEEP") || !stricmp(word, (char*)"DISCARD") || !stricmp(word, (char*)"DISABLE"))
			{
				if (i < 5) while (i++ < 5) dataptr++;  // use up blank fields
				else if (i > 5 && i < 9) while (i++ < 9) dataptr++;  // use up blank fields
				ptr = readBuffer;
				break;
			}

			if (i == 5) // we are moving into big pattern territory
			{
				big = true; // extended pattern
				*(dataptr - 1) |= PART1_BIT; // mark last field as continuing to 8zone
			}

			if (!stricmp(word, (char*)"STAY"))
			{
				if (doReverse) ++offset;
				else --offset;
				kind |= STAY;
				goto resume;
			}
			if (!stricmp(word, (char*)"SKIP")) // cannot do wide negative offset and then SKIP to fill
			{
				skipped = true;
				if (resultIndex == -1)
				{
					if (!reverse) (*printer)((char*)"Cannot do SKIP before the primary field -- offsets are unreliable (need to use REVERSE) Rule: %d comment: %s at line %d in %s\r\n", tagRuleCount, comment, currentFileLine, currentFilename);
					else (*printer)((char*)"Cannot do SKIP before the primary field -- offsets are unreliable Rule: %d comment: %s at line %d in %s\r\n", tagRuleCount, comment, currentFileLine, currentFilename);
					FClose(in);
					return;
				}

				if (doReverse) ++offset;
				else --offset;
				kind |= SKIP;
				goto resume;
			}

			if (*word == 'x') val = 0; // no field to process
			else if (*word == '!')
			{
				if (!stricmp(word + 1, (char*)"ROLE") || !stricmp(word + 1, (char*)"NEED"))
				{
					(*printer)((char*)"Cannot do !ROLE or !NEED Rule: %d comment: %s at line %d in %s\r\n", tagRuleCount, comment, currentFileLine, currentFilename);
					FClose(in);
					return;
				}
				if (!stricmp(word + 1, (char*)"STAY"))
				{
					(*printer)((char*)"Cannot do !STAY (move ! after STAY)  Rule: %d comment: %s at line %d in %s\r\n", tagRuleCount, comment, currentFileLine, currentFilename);
					FClose(in);
					return;
				}
				val = FindPropertyValueByName(word + 1);
				if (val == 0) val = FindParseValueByName(word + 1);
				if (val == 0) val = FindMiscValueByName(word + 1);
				if (val == 0) val = FindSystemValueByName(word + 1);
				if (val == 0)
				{
					(*printer)((char*)"Bad notted control word %s rule: %d comment: %s at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
					FClose(in);
					return;
				}

				if (!stricmp(word + 1, (char*)"include"))
				{
					(*printer)((char*)"Use !has instead of !include-  %s rule: %d comment: %s at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
					FClose(in);
					return;
				}
				kind |= NOTCONTROL;
				if (!stricmp(word + 1, (char*)"start")) flags = 1;
				else if (!stricmp(word + 1, (char*)"first")) flags = 1;
				else if (!stricmp(word + 1, (char*)"end")) flags = 10000;
				else if (!stricmp(word + 1, (char*)"last")) flags = 10000;
			}
			else
			{
				val = FindPropertyValueByName(word);
				if (val == 0) val = FindParseValueByName(word);
				if (val == 0) val = FindMiscValueByName(word);
				if (val == 0 || val > LASTCONTROL)
				{
					(*printer)((char*)"Bad control word %s rule: %d comment: %s at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
					FClose(in);
					return;
				}
				if (!stricmp(word, (char*)"include")) ++includes;
				if (!stricmp(word, (char*)"start")) flags |= 1;
				else if (!stricmp(word, (char*)"first")) flags |= 1;
				else if (!stricmp(word, (char*)"end")) flags |= 10000;
				else if (!stricmp(word + 1, (char*)"last")) flags = 10000;
			}
			flags |= val << OP_SHIFT;	// top byte
			flags |= ((uint64)kind) << CONTROL_SHIFT;
			if (i == (MAX_TAG_FIELDS + 1)) flags |= PART2_BIT;	// big marker on 2nd
			if (flags) // there is something to test
			{
				// read flags for this field
				bool subtract = false;
				bool once = false;
				uint64 baseval = flags >> OP_SHIFT;
				bool foundarg = false;
				while (ALWAYS)
				{
					ptr = ReadCompiledWord(ptr, word);
					if (baseval == ISCANONICAL || baseval == ISORIGINAL || baseval == PRIORCANONICAL || baseval == PRIORORIGINAL || baseval == POSTORIGINAL)
					{
						if ((!*word || *word == '#') && !foundarg) // missing argument
							(*printer)((char*)"Missing word argument for %s rule: %d %s at line %d in %s?\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
						foundarg = true;
					}
					if (!*word || *word == '#') break;	// end of flags
					if (baseval == ISCANONICAL || baseval == ISORIGINAL || baseval == PRIORCANONICAL || baseval == PRIORORIGINAL || baseval == POSTORIGINAL)
					{
						if ((FindPropertyValueByName(word) || FindParseValueByName(word)) && stricmp(word, (char*)"not") && !once)
						{
							(*printer)((char*)"Did you intend to use HASORIGINAL for %s rule: %d %s at line %d in %s?\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
							once = true;
						}
						v = MakeMeaning(StoreWord(word,0));
					}
					else if (IsDigit(word[0])) v = atoi(word); // for POSITION 
					else if (!stricmp(word, (char*)"reverse")) v = 1;	// for RESETLOCATION
					else if (!stricmp(word, (char*)"ALL")) v = TAG_TEST;
					else if (word[0] == '-')
					{
						subtract = true;
						v = 0;
					}
					else if (*word == '*')
					{
						if (offset != 0)
						{
							(*printer)((char*)"INCLUDE * must be centered at 0 rule: %d comment: %s at line %d in %s\r\n", tagRuleCount, comment, currentFileLine, currentFilename);
							FClose(in);
							return;
						}
						if (resultIndex != -1)
						{
							(*printer)((char*)"Already have pattern result bits %s rule: %d comment: %s at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
							FClose(in);
							return;
						}
						resultIndex = i - 1;
						v = 0;
					}
					else if (baseval == ISMEMBER || baseval == ISORIGINALMEMBER)
					{
						WORDP D = FindWord(word,0,PRIMARY_CASE_ALLOWED);
						if (!D && !warnedaboutbuild0)
						{
							warnedaboutbuild0 = true;
							(*printer)((char*)"Failed to find set %s - English POS tagger incomplete because build 0 not yet done.\r\n", word);
						}
						v = MakeMeaning(D);

					}
					else if (baseval == ISQUESTION)
					{
						if (!stricmp(word, (char*)"aux")) v = AUXQUESTION;
						else if (!stricmp(word, (char*)"qword")) v = QWORDQUESTION;
						else (*printer)((char*)"Bad ISQUESTION %s\r\n", word);
					}
					else
					{
						v = FindPropertyValueByName(word);
						if (v == 0)
						{
							v = FindSystemValueByName(word);
							if (v == 0) v = FindParseValueByName(word);
							if (v == 0) v = FindMiscValueByName(word);
							if (!v)
							{
								(*printer)((char*)"Bad flag word %s rule: %d %s at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
								FClose(in);
								return;
							}
						}
						else if (v & (NOUN | VERB | ADJECTIVE) && baseval != HASALLPROPERTIES && baseval != HASPROPERTY && baseval != ISPROBABLE)
						{
							(*printer)((char*)"Bad flag word overlaps BASIC bits %s rule: %d %s at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
							FClose(in);
							return;
						}
						if (baseval == PRIORPOS || baseval == POSTPOS)
						{
							if (!(kind & STAY)) (*printer)((char*)"PRIORPOS and POSTPOS should use STAY as well - rule: %d %s at line %d in %s\r\n", tagRuleCount, comment, currentFileLine, currentFilename);
						}
						if (baseval == IS || baseval == HAS || baseval == HASPROPERTY || baseval == ISPROBABLE || baseval == CANONLYBE || baseval == PARSEMARK || baseval == PRIORPOS || baseval == POSTPOS)
						{
							if (v & ALL_CONTROLBITS)
								(*printer)((char*)"Bad  bits overlap control fields %s rule: %d %s at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
						}

						if (subtract)
						{
							flags &= -1 ^ v;
							v = 0;
							subtract = false;
						}
					}
					flags |= v;
				}
			}
			if (includes > 1)
			{
				(*printer)((char*)"INCLUDE should only be on primary field - use HAS Rule: %d %s at line %d in %s\r\n", tagRuleCount, comment, currentFileLine, currentFilename);
				FClose(in);
				return;
			}
			if (i == 2) flags |= offsetValue;	// 2nd field gets offset shift
			*dataptr++ |= flags;

			ReadALine(readBuffer, in);
			ptr = readBuffer;
		} // end of fields loop

		  // now get results data
		ptr = ReadCompiledWord(ptr, word);
		if (!stricmp(word, (char*)"KEEP") || !stricmp(word, (char*)"DISCARD") || !stricmp(word, (char*)"DISABLE")) { ; }
		else
		{
			(*printer)((char*)"Too many fields before result %s: %d %s\r\n", word, tagRuleCount, comment);
			FClose(in);
			return;
		}
		if (doReverse) base[2] |= REVERSE_BIT;
		while (ALWAYS)
		{
			if (!stricmp(word, (char*)"DISABLE")) base[1] = ((uint64)HAS) << CONTROL_SHIFT; // 2nd pattern becomes HAS with 0 bits which cannot match
			else break;
			ptr = ReadCompiledWord(ptr, word);
		}
		if (!stricmp(word, (char*)"KEEP")) base[1] |= KEEP_BIT;
		else if (!stricmp(word, (char*)"DISCARD")) { ; }
		else
		{
			(*printer)((char*)"Too many fields before result? %s  rule: %d comment: %s  at line %d in %s\r\n", word, tagRuleCount, comment, currentFileLine, currentFilename);
			FClose(in);
			return;
		}
		if (resultIndex == -1)
		{
			(*printer)((char*)"Needs * on result bits %s rule: %d comment: %s\r\n", word, tagRuleCount, comment);
			FClose(in);
			return;
		}

		*base |= ((uint64)resultIndex) << RESULT_SHIFT;
		if (backwards && !skipped) (*printer)((char*)"Running backwards w/o a skip? Use forwards with minus start. %d %s.   at line %d in %s \r\n", tagRuleCount, comment, currentFileLine, currentFilename);

		commentsData[tagRuleCount] = AllocateHeap(comment);
		++tagRuleCount;
		if (big)
		{
			commentsData[tagRuleCount] = " ";
			++tagRuleCount;		// double-size rule
		}
	}
	FClose(in);
}

void ReadLivePosData()
{
	SetLanguage("ENGLISH"); 
	// read pos rules of english langauge - we dont have parser for any language  but english built in
	uint64 xdata[MAX_POS_RULES * MAX_TAG_FIELDS];
	char* xcommentsData[MAX_POS_RULES];
	dataBuf = xdata;
	commentsData = xcommentsData;
	tagRuleCount = 0;
#ifndef DISCARDPARSER
	char word[MAX_WORD_SIZE];
	sprintf(word, (char*)"%s/POS/adjectiverules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/adverbrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/auxverbrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/conjunctionrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/determinerrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/interjectionrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/miscposrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/nounrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/particle.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/prepositionrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/pronounrules.txt", languageFolder);
	ReadPosPatterns(word);
	sprintf(word, (char*)"%s/POS/verbrules.txt", languageFolder);
	ReadPosPatterns(word);
#endif
	tags = (uint64*)AllocateHeap((char*)xdata, tagRuleCount * MAX_TAG_FIELDS, sizeof(uint64), false);
	comments = NULL;
	bool haveComments = true;
#ifdef IOS // applications dont want comments
	haveComments = false;
#endif
#ifdef NOMAIN
	haveComments = false;
#endif
	if (haveComments) comments = (char**)AllocateHeap((char*)xcommentsData, tagRuleCount, sizeof(char*), true);
}

static void ReadPlurals(char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing plurals file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (ReadALine(input_string, in, maxBufferSize) >= 0)
	{
		char plural[MAX_WORD_SIZE];
		char* ptr = input_string;
		ptr = ReadCompiledWord(ptr, word);    //   the singular
		if (!*word || *word == '#') continue;
		ReadCompiledWord(ptr, plural);
		if (!*plural || *plural == '#') continue;
		WORDP singular = StoreWord(word, AS_IS);
		WORDP plurality = StoreWord(plural, AS_IS);
		SetPlural(singular, MakeMeaning(plurality));
	}
	FClose(in);
}

void ReadLiveData(char* language) // language-specific - if live changes, may have to rebuild topic data due to dict index changes
{
	if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese")) return; // we have no data

	char word[MAX_WORD_SIZE];
	sprintf(word, (char*)"%s/%s/plurals.txt", livedataFolder, language);
	ReadPlurals(word);


	if (!noboot) // command line parameter
	{
		sprintf(word, (char*)"%s/%s/canonical.txt", livedataFolder, language);
		ReadCanonicals(word, NULL);
		sprintf(word, (char*)"%s/%s/lemma.txt", livedataFolder, language);
		ReadLemmas(word, NULL);
		ReadSubstitutes((char*)"substitutes.txt", 0, NULL, DO_SUBSTITUTES);
		ReadSubstitutes((char*)"contractions.txt", 0, NULL, DO_CONTRACTIONS);
		ReadSubstitutes((char*)"interjections.txt", 0, NULL, DO_INTERJECTIONS);
		ReadSubstitutes((char*)"british.txt", 0, NULL, DO_BRITISH);
		ReadSubstitutes((char*)"spellfix.txt", 0, NULL, DO_SPELLING);
		ReadSubstitutes((char*)"texting.txt", 0, NULL, DO_TEXTING);
		ReadSubstitutes((char*)"noise.txt", 0, NULL, DO_NOISE);
		ReadWordsOf((char*)"lowercaseTitles.txt", LOWERCASE_TITLE);
	}
}

static void ReadAsciiDictionary()
{
	printf("Reading ascii %s dictionary\r\n", current_language);
	char buffer[50];
	unsigned int n = 0;
	for (char i = '0'; i <= '9'; ++i)
	{
		sprintf(buffer, (char*)"%c.txt", i);
		if (!ReadDictionary(UseDictionaryFile(buffer))) ++n;
	}
	for (char i = 'a'; i <= 'z'; ++i)
	{
		sprintf(buffer, (char*)"%c.txt", i);
		if (!ReadDictionary(UseDictionaryFile(buffer))) ++n;
	}
	if (!ReadDictionary(UseDictionaryFile((char*)"other.txt"))) ++n;
	if (n) (*printer)((char*)"Missing %d word files\r\n", n);
	ReadDictionary(UseDictionaryFile((char*)"extra.txt"));

	ReadAbbreviations((char*)"abbreviations.txt"); // needed for burst/tokenizing
}

static void ReadAsciiDictionaries()
{
	startOfLanguage = dictionaryFree;
	if (!strchr(language_list, ','))
	{
		ReadAsciiDictionary();
		ReadFacts(UseDictionaryFile((char*)"facts.txt"), NULL,-1);
	}
	else WalkLanguages(ReadLanguage);
	endOfLanguage = dictionaryFree;
	language_bits = LANGUAGE_UNIVERSAL;
}

void VerifyEntries(WORDP D, uint64 junk) // prove meanings have synset heads and major kinds have subkinds
{
	if (stricmp(current_language, "english")) return;	// dont validate
	if (D->internalBits & (DELETED_MARK | WORDNET_ID) || D->internalBits & DEFINES) return;

	if (D->properties & VERB && !(D->properties & VERB_BITS)) ReportBug((char*)"Verb %s lacks tenses\r\n", D->word);
	if (D->properties & NOUN && !(D->properties & NOUN_BITS)) ReportBug((char*)"Noun %s lacks subkind\r\n", D->word);
	if (D->properties & ADVERB && !(D->properties & ADVERB)) ReportBug((char*)"Adverb %s lacks subkind\r\n", D->word);
	if (D->properties & ADJECTIVE && !(D->properties & ADJECTIVE_BITS)) ReportBug((char*)"Adjective %s lacks subkind\r\n", D->word);
	WORDP X;
	unsigned int count = GetMeaningCount(D);
	for (unsigned int i = 1; i <= count; ++i)
	{
		MEANING M = GetMeanings(D)[i]; // what we point to in synset land
		if (!M)
		{
			ReportBug((char*)"Has no meaning %s %d\r\n", D->word, i);
				return;
		}
		X = Meaning2Word(M);
		int index = Meaning2Index(M);
		int count1 = GetMeaningCount(X);
		if (index > count1)
			ReportBug((char*)"Has meaning index too high %s.%d points to %s.%d but limit is %d\r\n", D->word, i, X->word, index, count1);

			// can we find the master meaning for this meaning?
			MEANING at = M;
		int n = 0;
		while (!(at & SYNSET_MARKER)) // find the correct ptr to return as master
		{
			X = Meaning2Word(at);
			if (X->internalBits & DELETED_MARK) ReportBug((char*)"Synset goes to dead word %s", X->word);
				int ind = Meaning2Index(at);
			if (ind > GetMeaningCount(X))
			{
				ReportBug((char*)"synsset master failure %s", X->word);
					return;
			}
			if (!ind)
			{
				ReportBug((char*)"synset master failure %s refers to no meaning index", X->word);
					return;
			}
			at = GetMeanings(X)[ind];
			if (++n >= MAX_SYNLOOP)
			{
				MEANING* meanings = GetMeanings(X);
				meanings[ind] |= SYNSET_MARKER;
				at |= SYNSET_MARKER;  // go add missing marker arbitrarily
				ReportBug((char*)"syset master loop overflow %s", X->word);
					return;
			}
		}
	}

	// verify glosses match up legal
	unsigned int glossCount = GetGlossCount(D);
	if (glossCount > 400)
	{
		ReportBug("Illegal gloss count");
		return;
	}
	for (unsigned int x = 1; x <= glossCount; ++x)
	{
		if (D->w.glosses[2*x] > count)
		{
			ReportBug((char*)"Gloss out of range for gloss %d   %s~%d with count only  %d\r\n", x, D->word, D->w.glosses[2*x], count);
			D->w.glosses[x] = D->w.glosses[x-1] = 0;	// bad ref
		}
	}

	count = GetMeaningCount(D);
	unsigned int synsetHeads;
	for (unsigned int i = 1; i <= count; ++i)
	{
		synsetHeads = 0;
		unsigned int counter = 0;
		MEANING M = GetMeaning(D, i);
		X = Meaning2Word(M);
		int index = Meaning2Index(M);
		if (M & SYNSET_MARKER);
		else while (X != D) // run til we loop once back to this entry, counting synset heads we see
		{
			if (X->internalBits & DELETED_MARK)
			{
				ReportBug((char*)"Synset references dead entry %s word: %s meaning: %d\r\n", X->word, D->word, i);
					break;
			}
			if (M & SYNSET_MARKER)
				++synsetHeads; // prior was a synset head
			index = Meaning2Index(M);
			if (index == 0) break; // generic pointer
			if (!GetMeanings(X))
			{
				M = 0;
				ReportBug((char*)"Missing synsets for %s word: %s meaning: %d\r\n", X->word, D->word, i);
					break;
			}
			if (GetMeaningCount(X) < index)
			{
				ReportBug((char*)"Missing synset index %s %s\r\n", X->word, D->word);
					break;
			}
			M = GetMeaning(X, index);
			WORDP Z = Meaning2Word(M);
			if (X == Z) // self ptr, no synset flag
			{
				MEANING* meanings = GetMeanings(X);
				meanings[index] |= SYNSET_MARKER;
				M |= SYNSET_MARKER;  // go add missing marker arbitrarily
				ReportBug((char*)"Missing synset head: %s\r\n", D->word);
					break; // in case of trouble
			}
			X = Z;
			if (++counter > MAX_SYNLOOP)
			{
				MEANING* meanings = GetMeanings(X);
				meanings[index] |= SYNSET_MARKER;
				M |= SYNSET_MARKER;  // go add missing marker arbitrarily
				ReportBug((char*)"Missing synset head: %s\r\n", D->word);
					break; // in case of trouble
			}
		}
		if (M & SYNSET_MARKER) ++synsetHeads; // prior was a synset head
		if (synsetHeads != 1)
			ReportBug((char*)"Bad synset list %s heads: %d count: %d\r\n", D->word, synsetHeads, counter);
	}
	int limit = 1000;
	if (GetTense(D))
	{
		WORDP E = GetTense(D);
		while (E != D && --limit > 0)
		{
			if (!E)
			{
				ReportBug((char*)"Missing conjugation %s \r\n", D->word);
					break;
			}
			if (E->internalBits & DELETED_MARK)
			{
				ReportBug((char*)"Deleted conjucation %s %s \r\n", D->word, E->word);
					break;
			}
			E = GetTense(E);
		}
	}
	if (GetPlural(D))
	{
		WORDP E = GetPlural(D);
		while (E != D && --limit > 0)
		{
			if (!E)
			{
				ReportBug((char*)"Missing plurality %s \r\n", D->word);
					break;
			}
			if (E->internalBits & DELETED_MARK)
			{
				ReportBug((char*)"Deleted plurality %s %s \r\n", D->word, E->word);
					break;
			}
			E = GetPlural(E);
		}
	}
	if (GetComparison(D))
	{
		WORDP E = GetComparison(D);
		while (E != D && --limit > 0)
		{
			if (!E)
			{
				ReportBug((char*)"Missing comparison %s \r\n", D->word);
					break;
			}
			if (E->internalBits & DELETED_MARK)
			{
				ReportBug((char*)"Deleted comparison %s %s \r\n", D->word, E->word);
					break;
			}
			E = GetComparison(E);
		}
	}

	// anything with a singular noun meaning should have an uplink
	if (D->properties & NOUN_SINGULAR && GetMeanings(D) && xbuildDictionary)
	{
		count = GetMeaningCount(D);
		for (unsigned int i = 1; i <= count; ++i)
		{
			if (!(GETTYPERESTRICTION(GetMeaning(D, i)) & NOUN)) continue;

			// might be just a "more noun" word
			X = Meaning2Word(GetMeaning(D, i));
			if (X == D) continue; // points to self is good enough

			MEANING M = GetMaster(GetMeaning(D, i));
			X = Meaning2Word(M);
			FACT* F = GetSubjectNondeadHead(X);
			while (F)
			{
				if (F->subject == M && F->verb == Mis && !(F->flags & FACTDEAD)) break;
				F = GetSubjectNondeadNext(F);
			}

			if (!F && !(D->internalBits & UPPERCASE_HASH))
				ReportBug((char*)"Meaning %d of %s with master %s missing uplink IS", i, D->word, WriteMeaning(M));
		}
	}
	FACT* F;
	if (D->subjectHead)
	{
		F = Index2Fact(D->subjectHead);
		if (Meaning2Word(F->subject) != D)
			ReportBug("Subject head not consistent %s -> %s", D->word, Meaning2Word(F->subject)->word);
	}
	if (D->verbHead)
	{
		F = Index2Fact(D->verbHead);
		if (Meaning2Word(F->verb) != D)
			ReportBug("Verb head not consistent %s -> %s", D->word, Meaning2Word(F->verb)->word);
	}
	if (D->objectHead)
	{
		F = Index2Fact(D->objectHead);
		if (Meaning2Word(F->object) != D)
			ReportBug("Object head not consistent %s -> %s", D->word, Meaning2Word(F->object)->word);
	}
}

char* WordWithLanguage(WORDP D)
{
	if (!D) return "";
	static char word[SMALL_WORD_SIZE];
	unsigned int len = WORDLENGTH(D);
	if (len > 50) len = 50;
	strncpy(word, D->word,len);
	word[len] = 0;
	if (len == 50)
	{
		strcat(word, "...");
		len += 3;
	}
	if (!IsUniversal(D->word, D->properties))
	{
		unsigned int language = GET_LANGUAGE_INDEX(D);
		sprintf(word + len, "~l%c", language + '0');

	}
	return word;
}

void LoadDictionary(char* heapstart)
{
	bool okresult;
	language_bits = LANGUAGE_UNIVERSAL;
	char* heapbase = heapFree;
	WORDP dictbase = dictionaryFree;
	FACT* factbase = lastFactUsed;
	keywordBase = NULL;
	okresult = ReadBinaryDictionary();
	if (okresult)
	{
		char* filename = UseDictionaryFile((char*)"facts.bin",language_list);
		int result = ReadBinaryFacts(FopenStaticReadOnly(filename), true,Word2Index(dictionaryFree));
		if (result != 2) okresult = false; // fatal, even if file missing
		// fast access to internal fact verbs (always first in dictionary from InitFacts call)
		Mmember = MakeMeaning(dictionaryBase + 1);
		Mexclude = MakeMeaning(dictionaryBase + 2);
		Mis = MakeMeaning(dictionaryBase + 3);
		Mremapfact = MakeMeaning(dictionaryBase + 4); 
	}
	if (!okresult) // if either is invalid, have to redo all binaries
	{
		sprintf(currentFilename, "%s/BUILD0/allwords0.bin", topicfolder);
		remove(currentFilename);
		sprintf(currentFilename, "%s/BUILD0/allfacts0.bin", topicfolder);
		remove(currentFilename);
		sprintf(currentFilename, "%s/BUILD1/allwords1.bin", topicfolder);
		remove(currentFilename);
		sprintf(currentFilename, "%s/BUILD1/allfacts1.bin", topicfolder);
		remove(currentFilename);
		remove(UseDictionaryFile((char*)"dict.bin"));
		remove(UseDictionaryFile((char*)"facts.bin")); // insure no erroneous binary of facts

		lastFactUsed = factBase;         // starting over
		lastheapfree = heapFree = heapstart;			 // starting over (after bucket allocation)
		dictionaryFree = dictionaryBase + 1; // starting over
		memset(hashbuckets, 0, (maxHashBuckets + 1) * sizeof(int));

		ClearWordMaps();
		InitFactWords(); // define MMember, Mexclude, Mis
		AcquireDefines((char*)"SRC/dictionarySystem.h"); //   get dictionary defines (must occur before loop that decodes properties into sets (below)
		ReadAsciiDictionaries();
		if (Word2Index(dictionaryFree) == 1) myexit("Ascii dictionary not read");

		AcquirePosMeanings(true); // add to vocab, store in tables, AND build pos facts
		ExtendDictionary(); // also sets global variables
		char name[MAX_WORD_SIZE];
		sprintf(name, (char*)"%s/%s/systemfacts.txt", livedataFolder, current_language);
		ReadFacts(name, NULL, -1); // part of wordnet, not level 0 build, just list of names, not really facts 

//		WalkDictionary(VerifyEntries); // prove good before writeout
//		VerifyFacts();				// prove good before writeout
//		printf("Verified dict and facts\r\n");

		// all entries are singles from the languages, there are no composite entries yet. those come from :build 
		WriteBinaryDictionary(); //   store the faster read form of dictionary
		keywordBase = NULL;
		WriteBinaryFacts(FopenBinaryWrite(UseDictionaryFile((char*)"facts.bin",language_list)), factBase, Word2Index(dictionaryFree));
	}
	else
	{
		WORDP oldd = dictionaryFree;
		AcquirePosMeanings(false); // set pos tables from dict entries (doesnt add new entries to dict or fact)
		ExtendDictionary(); // also sets global variables
		if (dictionaryFree != oldd) ReportBug("Adding pos facts changed dictionary"); // word added to dictionary?
	}
	*currentFilename = 0;

	printf("Dictionary loaded\r\n");

	fullDictionary = (!stricmp(current_language, (char*)"ENGLISH")) || (dictionaryFree - dictionaryBase) > 170000; // a lot of words are defined, must be a full dictionary.
}

WORDP BUILDCONCEPT(char* word)
{
	return StoreWord(word);
}

void ExtendDictionary()
{
	Dtopic = BUILDCONCEPT((char*)"~topic");
	Mburst = MakeMeaning(StoreWord((char*)"^burst"));
	Mchatoutput = MakeMeaning(StoreWord((char*)"chatoutput"));
	MgambitTopics = MakeMeaning(StoreWord((char*)"^gambittopics"));
	Mintersect = MakeMeaning(StoreWord((char*)"^intersect"));
	Mkeywordtopics = MakeMeaning(StoreWord((char*)"^keywordtopics"));
	Mconceptlist = MakeMeaning(StoreWord((char*)"^conceptlist"));
	Mmoney = MakeMeaning(BUILDCONCEPT((char*)"~moneynumber"));
	Mnumber = MakeMeaning(BUILDCONCEPT((char*)"~number"));
	BUILDCONCEPT((char*)"~float");
	BUILDCONCEPT((char*)"~integer");
	BUILDCONCEPT((char*)"~positiveinteger");
	BUILDCONCEPT((char*)"~negativeinteger");
	MadjectiveNoun = MakeMeaning(BUILDCONCEPT((char*)"~adjective_noun"));
	Mpending = MakeMeaning(StoreWord((char*)"^pending"));
	DunknownWord = StoreWord((char*)"unknown-word");

	BUILDCONCEPT((char*)"~noun_phrase");
	BUILDCONCEPT((char*)"~utf8");
	BUILDCONCEPT((char*)"~kelvin");
	BUILDCONCEPT((char*)"~celsius");
	BUILDCONCEPT((char*)"~fahrenheit");
	BUILDCONCEPT((char*)"~capacronym");
	BUILDCONCEPT((char*)"~emoji");
	BUILDCONCEPT((char*)"~filename");
	BUILDCONCEPT((char*)"~hashtag_label");
	BUILDCONCEPT((char*)"~twitter_name");
	BUILDCONCEPT((char*)"~shout");

	// generic concepts the engine marks automatically
	Dadult = BUILDCONCEPT((char*)"~adultword");
	Dchild = BUILDCONCEPT((char*)"~childword");
	Dfemalename = BUILDCONCEPT((char*)"~femalename");
	Dhumanname = BUILDCONCEPT((char*)"~humanname");
	Dmalename = BUILDCONCEPT((char*)"~malename");
	Dplacenumber = BUILDCONCEPT((char*)"~placenumber");
	Dpronoun = BUILDCONCEPT((char*)"~pronoun");
	Dadjective = BUILDCONCEPT((char*)"~adjective");
	Dauxverb = BUILDCONCEPT((char*)"~aux_verb");
	Dpropername = BUILDCONCEPT((char*)"~propername");
	Mphrase = MakeMeaning(BUILDCONCEPT((char*)"~phrase"));
	MabsolutePhrase = MakeMeaning(BUILDCONCEPT((char*)"~absolutephrase"));
	MtimePhrase = MakeMeaning(BUILDCONCEPT((char*)"~timephrase"));
	Dclause = BUILDCONCEPT((char*)"~clause");
	Dverbal = BUILDCONCEPT((char*)"~verbal");
	Dtime = BUILDCONCEPT((char*)"~timeword");
	Dunknown = BUILDCONCEPT((char*)"~unknownword");
	// predefine builtin sets with no engine variables
	unsigned int i = 0;
	char* ptr;
	while ((ptr = predefinedSets[i++]) != 0) BUILDCONCEPT(ptr);
}

char* FindCanonical(char* word, unsigned int i, bool notNew)
{
	uint64 controls = PRIMARY_CASE_ALLOWED;
	WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
	if (i == startSentence || i == 1)
	{
		WORDP S = FindWord(word, 0, SECONDARY_CASE_ALLOWED);
		if (S && IsLowerCase(*S->word))  D = S;
	}

	// if we know the word, we dont want to guess its roots-- abhorrent is not a noun
	if (xbuildDictionary && D && D->properties & TAG_TEST) // we know this word, we dont need its cannonical
	{
		return D->word;
	}
	if (D && !xbuildDictionary)
	{
		WORDP answer = GetCanonical(D);
		if (answer) return answer->word; //   special canonical form (various pronouns typically)
	}

	//    numbers - use digit form
	char* number;
	if (IsNumber(word) != NOT_A_NUMBER)
	{
		char word1[MAX_WORD_SIZE];
		if (strchr(word, '.') || strlen(word) > 9)  //   big numbers need float
		{
			double y;
			if (*word == '$') y = Convert2Double(word + 1);
			else y = Convert2Double(word);
			int x = (int)y;
			if (((double)x) == y) sprintf(word1, (char*)"%d", x); //   use int where you can
			else WriteFloat(word1, Convert2Double(word));
		}
		else if (GetCurrency((unsigned char*)word, number)) sprintf(word1, (char*)"%d", atoi(number));
		else strcpy(word1, PrintU64(Convert2Integer(word)));
		WORDP N = StoreFlaggedWord(word1, ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER,NOUN_NODETERMINER); // digit format cannot be ordinal
		return N->word;
	}

	// before seeing if canoncial can come from verb, see if it is from a known noun.  "cavities" shouldnt create cavity as a verb
	char* noun = NULL;
	size_t len = strlen(word);
	if (word[len - 1] == 's') noun = GetSingularNoun(word, true, true); // do we already KNOW this as a an extension of a noun

	//   VERB
	char* verb = GetInfinitive(word, (noun) ? true : notNew);
	if (verb)
	{
		WORDP V = FindWord(verb, 0, controls);
		verb = (V) ? V->word : NULL;
	}
	if (verb) return verb; //   we prefer verb base-- gerunds are nouns and verbs for which we want the verb

	//   NOUN
	noun = GetSingularNoun(word, true, notNew);
	if (noun)
	{
		WORDP  N = FindWord(noun, 0, controls);
		noun = (N) ? N->word : NULL;
	}
	if (noun) return noun;

	//   ADJECTIVE
	char* adjective = GetAdjectiveBase(word, notNew);
	if (adjective)
	{
		WORDP A = FindWord(adjective, 0, controls);
		adjective = (A) ? A->word : NULL;
	}
	if (adjective) return adjective;

	//   ADVERB
	char* adverb = GetAdverbBase(word, notNew);
	if (adverb)
	{
		WORDP A = FindWord(adverb, 0, controls);
		adverb = (A) ? A->word : NULL;
	}
	if (adverb) return adverb;

	return (D && D->properties & PART_OF_SPEECH && !IS_NEW_WORD(D)) ? D->word : NULL;
}

bool IsHelper(char* word)
{
	WORDP D = FindWord(word, 0, tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : 0);
	return (D && D->properties & AUX_VERB);
}

bool IsFutureHelper(char* word)
{
	WORDP D = FindWord(word, 0, tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : 0);
	return (D && D->properties & AUX_VERB_FUTURE);
}

bool IsPresentHelper(char* word)
{
	WORDP D = FindWord(word, 0, tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : 0);
	return (D && D->properties & AUX_VERB && D->properties & (VERB_PRESENT | VERB_PRESENT_3PS | AUX_VERB_PRESENT));
}

bool IsPastHelper(char* word)
{
	WORDP D = FindWord(word, 0, tokenControl & STRICT_CASING ? PRIMARY_CASE_ALLOWED : 0);
	return (D && D->properties & AUX_VERB && D->properties & (AUX_VERB_PAST | VERB_PAST));
}

void DumpDictionaryEntry(char* word, unsigned int limit)
{
	word = SkipWhitespace(word);
	char name[MAX_WORD_SIZE];
	strcpy(name, word);
	char* back;
	while ((back = strchr(name, '|'))) *back = '`'; // to allow seeing substitute source
	MEANING M = ReadMeaning(name, false, true);
	WORDP D;
	if (IsDigit(*word) && IsDigit(word[1])) // presume the actual dict index override
	{
		unsigned int n = atoi(word);
		D = Index2Word(n);
		strcpy(name, D->word);
		M = MakeMeaning(D);
	}

	unsigned int index = Meaning2Index(M);
	uint64 oldtoken = tokenControl;
	unsigned int oldbits = language_bits;
	tokenControl |= STRICT_CASING;
	D = Meaning2Word(M);
	WORDP E = Meaning2Word(M);
	char* lang = (char*)GetLanguage(D);
	if (D) language_bits = D->internalBits & LANGUAGE_BITS;

	if (D && IS_NEW_WORD(D) && (*D->word == '~' || *D->word == USERVAR_PREFIX || *D->word == '^')) D = 0;	// debugging may have forced this to store, its not in base system
	if (limit == 0) limit = 5; // default
	char* old = (D <= dictionaryPreBuild[LAYER_0]) ? (char*)"old" : (char*)"";
	if (D)
	{
			char* univ = "";
			if (D->foreignFlags) univ = "(universal multi)";
			else if (D->internalBits & UNIVERSAL_WORD) univ = "(universal)";
			else if ( D->internalBits & IS_SUPERCEDED) univ = "(superceded)";
			Log(USERLOG, "\r\n%s x%08x (%d): %s in %s %s\r\n", D->word, D, Word2Index(D), old, lang, univ);
	}
	else Log(USERLOG, "\r\n%s (unknown word in %s):\r\n", name, current_language);
	
	if (E && E->word[0] == '$')
		Log(USERLOG, "  Variable value: %s\r\n", E->w.userValue);
	if (E && IsQuery(E)) Log(USERLOG, "  Query value: %s\r\n", E->w.userValue);
	uint64 properties = (D) ? D->properties : 0;
	if (D)
	{
		Log(USERLOG, "  Properties: x%08x %08x\r\n", (unsigned int)(properties >> 32), (unsigned int)(properties & 0x00000000ffffffff));
	}
	uint64 sysflags = (D) ? D->systemFlags : 0;
	uint64 cansysflags = 0;
	uint64 bit = START_BIT;
	while (properties && bit)
	{
		if (properties & bit) 
			Log(USERLOG, "%s ", FindNameByValue(bit));
		bit >>= 1;
	}

	WORDP entry = NULL;
	WORDP canonical = NULL;
	char* tilde = strchr(name + 1, '~');
	wordStarts[0] = 0;
	wordStarts[1] = AllocateHeap((char*)"");
	posValues[1] = 0;
	wordStarts[2] = AllocateHeap((char*)"");
	posValues[2] = 0;
	wordStarts[3] = AllocateHeap(word);
	posValues[3] = 0;
	wordStarts[4] = 0;
	if (tilde && IsDigit(tilde[1])) *tilde = 0;	// turn off specificity
	uint64 xflags = 0;
	WORDP revise;
	uint64 inferredProperties = (name[0] != '~' && name[0] != '^') ? GetPosData((unsigned int)-1, name, revise, entry, canonical, xflags, cansysflags) : 0;
	if (D && D->systemFlags & HAS_SUBSTITUTE )
	{
		if (D->w.substitutes)
		{
			if (GetSubstitute(D)) Log(USERLOG, "\r\n        SUBSTITUTION SOURCE-> %s \r\n", GetSubstitute(D)->word);
		}
		else  Log(USERLOG, "\r\nSUBSTITUTION SOURCE-> erase original\r\n");
	}
	unsigned int nn = (D) ? GETMULTIWORDHEADER(D) : 0;
	if (nn) Log(USERLOG, "MultiwordHeader %d\r\n",nn);
	
	if (stricmp(current_language, "english") && D)
	{
		WORDP fullcanon = GetCanonical(D, 0);
		if (fullcanon) canonical = fullcanon;
	}
	if (D && entry && D != entry)
	{
		Log(USERLOG, "\r\n  Changed to %s\r\n", entry->word);
		D = entry;
		properties = D->properties;
	}

	bit = START_BIT;
	bool extended = false;
	uint64 unionprop = inferredProperties | properties;
	while ( bit)
	{
		if (unionprop & bit)
		{
			char* label = FindNameByValue(bit);
			if (!(properties & bit)) Log(USERLOG, "%s+ ", label); // bits beyond what was directly known in dictionary before
		}
		bit >>= 1;
	}

	sysflags |= xflags;
	if (sysflags)
	{
		Log(USERLOG, "\r\n  SystemFlags: x%08x %08x\r\n", (unsigned int)(sysflags >> 32), (unsigned int)(sysflags & 0x00000000ffffffff));
	}
	bit = START_BIT;
	while (sysflags && bit)
	{
		if (sysflags & bit)
		{
			char xword[MAX_WORD_SIZE];
			*xword = 0;
			Log(USERLOG, "%s ", MakeLowerCopy(xword, FindSystemNameByValue(bit)));
		}
		bit >>= 1;
	}
	if (sysflags & ESSENTIAL_FLAGS)
	{
		Log(USERLOG, " POS-tiebreak: ");
		if (sysflags & NOUN) Log(USERLOG, "NOUN ");
		if (sysflags & VERB) Log(USERLOG, "VERB ");
		if (sysflags & ADJECTIVE) Log(USERLOG, "ADJECTIVE ");
		if (sysflags & ADVERB) Log(USERLOG, "ADVERB ");
		if (sysflags & PREPOSITION) Log(USERLOG, "PREPOSITION ");
	}

	if (D && D->internalBits & CONDITIONAL_IDIOM)  Log(USERLOG, "conditional idiom: %s\r\n", D->w.conditionalIdiom->word);

	if (D && D->parseBits)
	{
		if (!(D->internalBits & DEFINES))
		{
			Log(USERLOG, "\r\n  ParseBits: ");
			bit = START_BIT;
			while ( D->parseBits && bit)
			{
				if ( D->parseBits & bit)
				{
					char xword[MAX_WORD_SIZE];
					Log(USERLOG, "%s ", MakeLowerCopy(xword, FindParseNameByValue(bit)));
				}
				bit >>= 1;
			}
		}
		else Log(USERLOG, "\r\n  ParseBits: %s ",Index2Word(D->parseBits));
	}

	if (D) Log(USERLOG, "  \r\n  InternalBits: 0x%08x ", D->internalBits);
#ifndef DISCARDTESTING
	unsigned int basestamp = inferMark;
#endif
	NextInferMark();

#ifndef DISCARDTESTING 
	if (D && D->word[0] == '~' && !(D->internalBits & TOPIC))
	{
		NextInferMark();
		Log(USERLOG, "concept (%d members) ", CountSet(D, basestamp));
	}
#endif

	if (D && D->internalBits & QUERY_KIND && !IsQuery(D) ) Log(USERLOG, "rename ");
	if (D && IsQuery(D)) Log(USERLOG, "query ");
	if (D && D->internalBits & UTF8) Log(USERLOG, "utf8 ");
	if (D && D->internalBits & UPPERCASE_HASH) Log(USERLOG, "uppercase ");
	if (D && D->internalBits & FUNCTION_BITS && D->internalBits & MACRO_TRACE) Log(USERLOG, "being-traced ");
	if (D && D->internalBits & FUNCTION_BITS && D->internalBits & MACRO_TIME) Log(USERLOG, "being-timed ");
	if (D && D->word[0] == '~' && !(D->internalBits & TOPIC)) Log(USERLOG, "concept ");
	if (D && D->internalBits & TOPIC) Log(USERLOG, "topic ");
	if (D && D->internalBits & BUILD0) Log(USERLOG, "build0 ");
	if (D && D->internalBits & BUILD1) Log(USERLOG, "build1 ");
	if (D && D->internalBits & HAS_EXCLUDE) Log(USERLOG, "has_exclude ");
	if (D && D->internalBits & DO_ESSENTIALS)  Log(USERLOG, "do_essentials ");
	if (D && D->internalBits & DO_SUBSTITUTES)  Log(USERLOG, "do_substitutes ");
	if (D && D->internalBits & DO_CONTRACTIONS)  Log(USERLOG, "do_contractions ");
	if (D && D->internalBits & DO_INTERJECTIONS)  Log(USERLOG, "do_interjections ");
	if (D && D->internalBits & DO_BRITISH)  Log(USERLOG, "do_british ");
	if (D && D->internalBits & DO_SPELLING)  Log(USERLOG, "do_spelling ");
	if (D && D->internalBits & DO_TEXTING)  Log(USERLOG, "do_texting ");
	if (D && D->internalBits & DO_NOISE)  Log(USERLOG, "do_noise ");
	if (D && D->internalBits & DO_PRIVATE)  Log(USERLOG, "do_private ");
	if (D && D->internalBits & FUNCTION_NAME)
	{
		char* kind = "";

		SystemFunctionInfo* info = NULL;
		if (D->x.codeIndex)	info = &systemFunctionSet[D->x.codeIndex];	//   system function

		if ((D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO) kind = (char*)"plan";
		else if ((D->internalBits & FUNCTION_BITS) == IS_OUTPUT_MACRO) kind = (char*)"output";
		else if ((D->internalBits & FUNCTION_BITS) == IS_TABLE_MACRO) kind = (char*)"table";
		else if ((D->internalBits & FUNCTION_BITS) == IS_PATTERN_MACRO) kind = (char*)"pattern";
		else if ((D->internalBits & FUNCTION_BITS) == (IS_PATTERN_MACRO | IS_OUTPUT_MACRO)) kind = (char*)"dual";
		if (D->x.codeIndex && (D->internalBits & FUNCTION_BITS) != IS_PLAN_MACRO) Log(USERLOG, "systemfunction %d (%d arguments) ", D->x.codeIndex, info->argumentCount);
		else if (D->x.codeIndex && (D->internalBits & FUNCTION_BITS) == IS_PLAN_MACRO) Log(USERLOG, "plan (%d arguments)", D->w.planArgCount);
		else Log(USERLOG, "user %s macro (%d arguments)\r\n  %s \r\n", kind, MACRO_ARGUMENT_COUNT(GetDefinition(D)), GetDefinition(D));
		if (D->internalBits & VARIABLE_ARGS_TABLE) Log(USERLOG, "variableArgs");
	}

	if (D && D->foreignFlags) Log(USERLOG, "\r\n  ForeignFlagsPtr:  x%s\r\n", PrintX64((uint64)D->foreignFlags));
	Log(USERLOG, "\r\n");

	if (D && *D->word == SYSVAR_PREFIX) Log(USERLOG, "systemvar ");
	if (D && *D->word == USERVAR_PREFIX)
	{
		char* val = GetUserVariable(D->word);
		Log(USERLOG, "VariableValue= \"%s\" ", val);
	}
	if (canonical && D) Log(USERLOG, "  canonical: %s ", canonical->word);
	Log(USERLOG, "\r\n");
	tokenControl = oldtoken;
	if (GetTense(D))
	{
		Log(USERLOG, "  ConjugationLoop= ");
		WORDP G = GetTense(D);
		while (G && G != D)
		{
			Log(USERLOG, "-> %s ", G->word);
			G = GetTense(G);
		}
		Log(USERLOG, "\r\n");
	}
	if (GetPlural(D))
	{
		Log(USERLOG, "  PluralityLoop= ");
		WORDP G = GetPlural(D);
		int n = 0;
		while (G && G != D)
		{
			if (++n > 100) break;
			Log(USERLOG, "-> %s ", G->word);
			G = GetPlural(G);
		}
		Log(USERLOG, "\r\n");
	}
	if (GetComparison(D))
	{
		Log(USERLOG, "  comparativeLoop= ");
		WORDP G = GetComparison(D);
		while (G && G != D)
		{
			Log(USERLOG, "-> %s ", G->word);
			G = GetComparison(G);
		}
		Log(USERLOG, "\r\n");
	}

	//   now dump the meanings
	unsigned int count = (D) ? GetMeaningCount(D) : 0;
	if (count) Log(USERLOG, "  Meanings:");
	uint64 kind = 0;
	for (unsigned int i = count; i >= 1; --i)
	{
		if (index && i != index) continue;
		M = GetMeaning(D, i);
		WORDP E = Meaning2Word(M);
		uint64 k1 = GETTYPERESTRICTION(M);
		if (kind != k1)
		{
			kind = k1;
			if (GETTYPERESTRICTION(M) & NOUN) Log(USERLOG, "\r\n    noun\r\n");
			else if (GETTYPERESTRICTION(M) & VERB) Log(USERLOG, "\r\n    verb\r\n");
			else if (GETTYPERESTRICTION(M) & ADJECTIVE) Log(USERLOG, "\r\n    adjective/adverb\r\n");
			else Log(USERLOG, "\r\n    misc\r\n");
		}
		char* gloss;
		MEANING master = GetMaster(M);
		E = Meaning2Word(master);
		MEANING parent = 0;
		FACT* F = GetSubjectHead(E);
		while (F)
		{
			if (F->verb == Mis && F->subject == master)
			{
				parent = F->object;
				break;
			}
			F = GetSubjectNext(F);
		}
		gloss = GetGloss(E, Meaning2Index(master));
		if (!gloss) gloss = "";
		if (parent)
		{
			char mean[MAX_WORD_SIZE];
			Log(USERLOG, "    %d: %s (parent: %s) %s\r\n", i, WriteMeaning(master & STDMEANING, true), WriteMeaning(parent & STDMEANING, true, mean), gloss);
		}
		else Log(USERLOG, "    %d: %s %s\r\n", i, WriteMeaning(master & STDMEANING, true), gloss);
		M = GetMeaning(D, i) & STDMEANING;
		bool did = false;
		while (Meaning2Word(M) != D) // if it points to base word, the loop is over.
		{
			MEANING next = GetMeaning(Meaning2Word(M), Meaning2Index(M));
			if ((next & STDMEANING) == M) break;	 // only to itself
			if (!did)
			{
				did = true;
				Log(USERLOG, "      synonyms: ");
			}
			if (next & SYNSET_MARKER) Log(USERLOG, " *%s ", WriteMeaning(M));	// mark us as master for display
			else Log(USERLOG, " %s ", WriteMeaning(M));
            if (Meaning2Index(M) == 0) break;
			M = next & STDMEANING;
		}
		if (did) // need to display closure in for seeing MASTER marker, even though closure is us
		{
            if (Meaning2Index(M) == 0) ;
            else if (GetMeaning(D, i) & SYNSET_MARKER) Log(USERLOG, " *%s ", WriteMeaning(M));
			else Log(USERLOG, " %s ", WriteMeaning(M));
			Log(USERLOG, "\r\n"); //   header for this list
		}
		did = false;
		F = GetObjectNondeadHead(Meaning2Word(master));
		int n = 0;
		while (F)
		{
			if (F->verb == Mis && F->object == master)
			{
				if (!did)
				{
					did = true;
					Log(USERLOG, "      children: ");
				}
				Log(USERLOG, "%s ", WriteMeaning(F->subject & STDMEANING, true));
				if (++n == 5)
				{
					n = 0;
					Log(USERLOG, "\r\n                ");
				}
			}
			F = GetObjectNondeadNext(F);
		}
		if (did) Log(USERLOG, "\r\n");// need to display closure in for seeing MASTER marker, even though closure is us
		did = false;
	}

	if (D && D->inferMark) Log(USERLOG, "  Istamp- %d\r\n", D->inferMark);
	if (D && GETMULTIWORDHEADER(D)) Log(USERLOG, "  MultiWordHeader length: %d\r\n", GETMULTIWORDHEADER(D));

	seeAllFacts = true;

	// show concept/topic members
	FACT* F = GetSubjectHead(D);
	Log(USERLOG, "  Direct Sets: ");
	while (F)
	{
		char* name = Meaning2Word(F->object)->word;
		if (index && Meaning2Index(F->subject) && Meaning2Index(F->subject) != index) { ; } // wrong path member
		if (F->verb == Mmember || F->verb == Mis)
		{
			char word[MAX_WORD_SIZE];
			strcpy(word, name);
			if (!IsUniversal(name, D->properties))
			{
				char lang[20];
				unsigned int bits = GET_LANGUAGE_INDEX(D);
				sprintf(lang, "`%d~l%u", (int)F->botBits, bits);
				strcat(word, lang);
			}
			Log(USERLOG, "%s ", word);
		}
		F = GetSubjectNext(F);
	}
	Log(USERLOG, "\r\n");

	char* limited;
	char* buffer = InfiniteStack(limited, "DumpDictionaryEntry");
	Log(USERLOG, "  Facts:\r\n");
	count = 0;
	F = GetSubjectHead(D);
	while (F)
	{
		if (F->verb != Mmember)
		{
			Log(USERLOG, "  %d  %s",  F - factBase, WriteFact(F, false, buffer, false, true));
			F = GetSubjectNext(F);
			if (++count >= limit && F)
			{
				Log(USERLOG, "    ...\r\n");
				break;
			}
		}
		else 	F = GetSubjectNext(F);
	}

	F = GetVerbHead(D);
	count = 0;
	while (F)
	{
		Log(USERLOG, "  %d  %s",  F - factBase,WriteFact(F, false, buffer, false, true));
		F = GetVerbNext(F);
		if (++count >= limit && F)
		{
			Log(USERLOG, "    ...\r\n");
			break;
		}
	}

	F = GetObjectHead(D);
	count = 0;
	while (F)
	{
		Log(USERLOG, "  %d  %s", F - factBase, WriteFact(F, false, buffer, false, true));
		F = GetObjectNext(F);
		if (++count >= limit && F)
		{
			Log(USERLOG, "    ...\r\n");
			break;
		}
	}
	Log(USERLOG, "\r\n");
	ReleaseInfiniteStack();
	

	// multi forms and languages?
	WORDP set[GETWORDSLIMIT];
	int i = GetWords(word, set, false, true); // words in any case and with mixed underscore and spaces
	Log(USERLOG, "all forms: \r\n");
	for (int x = 0; x < i; ++x)
	{
		WORDP E = set[x];
		const char* lang = GetLanguage(E);
		char props[100];
		strcpy(props, PrintX64(E->properties));
		if (E->internalBits & IS_SUPERCEDED) strcat(props, " superceded ");
		Log(USERLOG, "         %s: %s address: x%s dictindex: %d props: x%s \r\n", lang, E->word,PrintX64((uint64)E),Word2Index(E),props);
	}
	seeAllFacts = false;
}

#include <map>
typedef std::map <char*, int> MapType;

static char base[MAX_WORD_SIZE];
#define DICTBEGIN 1
#define MAINFOUND 2
#define FUNCTIONFOUND 3
#define SPEECHFOUND 4
#define GLOSSFOUND 5
#define BASEFOUND 6
static WORDP Dqword;
static void DefineShortCanonicals(WORDP D, uint64 junk);
static bool didSomething;
static void ReadWordFrequency(const char* name, unsigned int count, bool until);
//   http://wordnet.princeton.edu 

#ifdef INFORMATION
Nounsand verbs are organized into hierarchies based on the hypernymy / hyponymy relation between synsets.Additional pointers are be used to indicate other relations.

Adjectives are arranged in clusters containing head synsets and satellite synsets.Each cluster is organized around antonymous pairs(and occasionally antonymous triplets).The antonymous pairs(or triplets) are indicated in the head synsets of a cluster.Most head synsets have one or more satellite synsets, each of which represents a concept that is similar in meaning to the concept represented by the head synset.One way to think of the adjective cluster organization is to visualize a wheel, with a head synset as the huband satellite synsets as the spokes.Two or more wheels are logically connected via antonymy, which can be thought of as an axle between the wheels.

Pertainyms are relational adjectivesand do not follow the structure just described.Pertainyms do not have antonyms; the synset for a pertainym most often contains only one word or collocation and a lexical pointer to the noun that the adjective is "pertaining to".Participial adjectives have lexical pointers to the verbs that they are derived from.

Adverbs are often derived from adjectives, and sometimes have antonyms; therefore the synset for an adverb usually contains a lexical pointer to the adjective from which it is derived.

#endif

WORDP Ddebug = NULL;
static void readMassNouns(const char* file);

static WORDP FindCircularEntry(WORDP baseentry, uint64 propertyBit)
{
	if (!baseentry) return NULL;
	WORDP at = GetPlural(baseentry);
	if (!at) return NULL;
	while (at != baseentry)
	{
		if (at->properties & propertyBit) return at;
		at = GetPlural(at);
		if (!at) return NULL;		// BUG if happens
	}
	return NULL;	//   not found
}

static void FixSynsets(WORDP D, uint64 data)
{	// revise synset header data
	if (!(D->internalBits & WORDNET_ID)) return;
	// check original masters back to their switchover
	if (!GetMeaningCount(D)) // useless synset
	{
		AddInternalFlag(D, DELETED_MARK);
		return;
	}
	//   now decide on the master referent--- it will be the most common name, longest of those
	MEANING name = 0;
	int64 bestage = -1;
	int64 best = -1;
	for (int k = 1; k <= GetMeaningCount(D); ++k)
	{
		MEANING M = GetMeaning(D, k);
		M |= (unsigned int)(D->properties & BASIC_POS);
		GetMeanings(D)[k] = M;		// type flagged for consistency
		WORDP m = Meaning2Word(M);
		int64 age = m->systemFlags & AGE_LEARNED;
		if (age > bestage || (age == bestage && WORDLENGTH(m)> Meaning2Word(name)->length)) //   is this more common or longer?
		{
			bestage = age;
			name = M;
			best = k;
		}
	}
	if (best != -1)
	{
		GetMeanings(D)[best] = GetMeaning(D, 1);
		GetMeanings(D)[1] = name;
	}
}

static void ExtractSynsets(WORDP D, uint64 data) // now rearrange to get synsets inline
{
	if (!(D->internalBits & WORDNET_ID)) return;

	if (!D->meanings)
	{
		AddInternalFlag(D, DELETED_MARK);
		FACT* F = GetSubjectHead(D);	// all facts he is head of
		while (F)
		{
			KillFact(F);
			F = GetSubjectNext(F);
		}
		return;
	}

	MEANING master = GetMeaning(D, 1) & SIMPLEMEANING;	// this is the master normal word of this synset, who currently points to us. 
	WORDP E = Meaning2Word(master);
	unsigned int index = Meaning2Index(master);

	MEANING synset = GetMeaning(E, index); // he points to synset	which should be us
	WORDP syn = Meaning2Word(synset);
	unsigned int count = GetMeaningCount(E);
	if (count < index || syn != D) // he doesnt point back to us!
	{
		ReportBug("Bad extract master");
			myexit(0);
	}
	MEANING oldmeaning = 0;

	// convert pointers from (words->synsets)  to  circular word list with marker on head when you are at it.
	for (int k = 1; k <= GetMeaningCount(D); ++k)  // each meaning of the synset
	{
		MEANING M = GetMeaning(D, k); // ptr from synset to actual word
		WORDP referent = Meaning2Word(M); // actual dictionary word
		unsigned int offset = Meaning2Index(M);
		if (GetMeaning(referent, offset) != synset) // prove points to synset head
		{
			ReportBug("bad extract master2 synset: %s index: %d word %s index %d  synset checked against %s %d but was %s %d\r\n", D->word, k, referent->word, offset, Meaning2Word(synset)->word, Meaning2Index(synset),
                      Meaning2Word(GetMeaning(referent, offset))->word, Meaning2Index(GetMeaning(referent, offset)));
			myexit(0);
		}
		GetMeanings(referent)[offset] = oldmeaning; // change his synset ptr to be our new master
		oldmeaning = M;
	}
	GetMeanings(E)[index] = oldmeaning | SYNSET_MARKER; // circular loop complete and stamp
	if (D->w.glosses)
	{
		char* gloss = Index2Heap(D->w.glosses[1] & 0x00ffffff);
		AddGloss(E, gloss, index);
	}
	// now adjust his facts...(synsets only have is facts between synsets)
	FACT* F = GetSubjectHead(D);	// all facts he is head of
	while (F)
	{
		if (F->flags & (FACTSUBJECT | FACTVERB | FACTOBJECT))
		{
			ReportBug("bad fact on synset");
				myexit(0);
		}
		if (F->verb == Mis);
		else
		{
			ReportBug("unknonw synset fact");
				myexit(0);
		}
		MEANING object = F->object;		// originally a synset id
		WORDP o = Meaning2Word(object);
		if (!(o->internalBits & WORDNET_ID))
		{
			ReportBug("failed link");
				myexit(0);
		}
		if (o->meanings) // if it has meaning now. - if not, hasnt been converted yet
		{
			object = GetMeaning(o, 1) & SIMPLEMEANING;	// this synsets main link
			WORDP x = Meaning2Word(object); // debug only
			CreateFact(master, Mis, object);
		}
		KillFact(F);
		F = GetSubjectNext(F);
	}

	AddInternalFlag(D, DELETED_MARK); // we are done with this synset, kill it
}

static void PurgeDictionary(WORDP D, uint64 data)
{ // meanings are currently to self or to synset headers.  we will add links from headers back to us
	if (D->internalBits & WORDNET_ID)  return;
	if (*D->word == '_' && D->internalBits & RENAMED) return;	// is rename _xxx
	unsigned int remapIndex = GetMeaningCount(D);
	if (!D->properties && !remapIndex && !GetSubstitute(D) && D->word[0] != '~' && !(D->internalBits & CONDITIONAL_IDIOM)) // useless word
	{
		AddInternalFlag(D, DELETED_MARK); //   not interesting data
										  // remove its facts
		FACT* F = GetSubjectHead(D);
		while (F)
		{
			KillFact(F);
			F = GetSubjectNext(F);
		}
		F = GetVerbHead(D);
		while (F)
		{
			KillFact(F);
			F = GetVerbNext(F);
		}
		F = GetObjectHead(D);
		while (F)
		{
			KillFact(F);
			F = GetObjectNext(F);
		}

		return;
	}

	// word had meanings, does it still or have they been deleted
	unsigned int remap[1000];
	unsigned int meaningCount = remapIndex;
	for (unsigned int j = 1; j <= meaningCount; ++j)
	{
		MEANING M = GetMeaning(D, j);
		remap[j] = M; // what WAS the value before we delete anything
		if (M && Meaning2Word(M) == D)
		{
			unsigned int index = Meaning2Index(M);
			if (index != j)
			{
				ReportBug("Self ptr not pointing to own index\r\n");
			}
		}
	}

	//   now verify meanings not deleted. if are then delete
	for (unsigned int i = 1; i <= meaningCount; ++i)
	{
		MEANING M = GetMeaning(D, i);
		WORDP X = Meaning2Word(M);
		if (M && !(Meaning2Word(M)->internalBits & WORDNET_ID) && Meaning2Word(M) != D) //invalid link
		{
			ReportBug("bad xlink %s %s", D->word, Meaning2Word(M)->word);
				GetMeanings(D)[i] = 0;
		}
		if (!M) // was deleted 
		{
			remap[i] = 0; // deleting this
			for (unsigned int j = i + 1; j <= remapIndex; ++j)
			{
				if (Meaning2Word(remap[j]) == D) remap[j] -= INDEX_MINUS; // renumber later self-ptr  to what they will actually be index-wise 
			}

			// now remove facts involving this index
			FACT* F = GetSubjectHead(D);
			while (F)
			{
				unsigned int index = Meaning2Index(F->subject);
				if (index == i)  KillFact(F);
				F = GetSubjectNext(F);
			}
			F = GetVerbHead(D);
			while (F)
			{
				unsigned int index = Meaning2Index(F->verb);
				if (index == i) KillFact(F);
				F = GetVerbNext(F);
			}
			F = GetObjectHead(D);
			while (F)
			{
				unsigned int index = Meaning2Index(F->object);
				if (index == i)  KillFact(F);
				F = GetObjectNext(F);
			}
		}
	}

	// alter facts of still living definitions
	for (unsigned int j = 1; j <= remapIndex; ++j)
	{
		MEANING newMeaning = remap[j]; // a self ptr to the NEW slot of self
		if (newMeaning == GetMeaning(D, j) || !newMeaning) continue;	 // unchanged or already killed off
		GetMeanings(D)[j] = newMeaning;

		// update facts pointing to the OLD self to new self ptr
		FACT* F = GetSubjectHead(D);
		while (F)
		{
			unsigned int index = Meaning2Index(F->subject);
			if (index == j)  F->subject = newMeaning;
			F = GetSubjectNext(F);
		}
		F = GetVerbHead(D);
		while (F)
		{
			unsigned int index = Meaning2Index(F->verb);
			if (index == j) F->verb = newMeaning;
			F = GetVerbNext(F);
		}
		F = GetObjectHead(D);
		while (F)
		{
			unsigned int index = Meaning2Index(F->object);
			if (index == j)  F->object = newMeaning;
			F = GetObjectNext(F);
		}
	}
	// set up backlinks from synset head
	for (int i = 1; i <= GetMeaningCount(D); ++i)
	{
		MEANING M = GetMeaning(D, i);
		WORDP master = Meaning2Word(M);

		if (!master) // remove defunct meaning
		{
			memmove(&GetMeanings(D)[i], &GetMeanings(D)[i + 1], sizeof(MEANING) * (GetMeaningCount(D) - i));
			--GetMeanings(D)[0];
			--i;
		}
		else if (master != D) AddMeaning(master, MakeMeaning(D, i) | (M & TYPE_RESTRICTION)); // we now link synset head back to word
																							  // else was self ptr or deleted, no real synset
	}
}

static void RemoveComments(char* at)
{
	int paren = 0;
	--at;
	while (*++at)    //   scan to end or comment marker (will not occur in a pattern area)
	{
		if (*at == '#' && !paren) break;
		else if (*at == '(') ++paren;
		else if (*at == ')') --paren;
	}
	*at = 0;               //   ERASE comment if any
}

static void RemoveTrailingWhite(char* ptr)
{
	//   remove trailing line separator and blanks
	size_t len = strlen(ptr);
	char* at = ptr + len - 1;      //   trim off trailing blanks
	while (at > ptr && *at == ' ') --at;
	at[1] = 0;              //   erase any trailing blanks
}

static int fget_input_string(bool cpp, bool postprocess, char* input_string, FILE * in)
{
	while (ReadALine(input_string, in) >= 0) //   used when reading a file
	{
		//   erase trailing \r\n
		if (!cpp && *input_string == '#') continue; //    DO NOT treat # as comment when reading the modifers.h C++ file
		if (postprocess) RemoveComments(input_string);
		RemoveTrailingWhite(input_string);
		return 1;
	}
	return 0;
}

static void AddQuestionAdverbs()
{
	WORDP D = StoreFlaggedWord("whom", QWORD, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("who", QWORD, KINDERGARTEN);  // wh_pronoun  also whether
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whose", QWORD, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whomever", QWORD, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whoever", QWORD, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("what", QWORD, KINDERGARTEN);  // determiner also   wh_determiner
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("which", QWORD, KINDERGARTEN);   // determiner also
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whichever", QWORD, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whatever", QWORD, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));

	D = StoreFlaggedWord("where", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("when", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("why", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("how", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));

	D = StoreFlaggedWord("whence", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whereby", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whereein", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whereupon", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));

	D = StoreFlaggedWord("however", ADVERB | QWORD | ADVERB | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));

	D = StoreFlaggedWord("whenever", ADVERB | ADVERB | QWORD | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whereever", ADVERB | QWORD | ADVERB | CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));

	D = StoreFlaggedWord("whereeof", CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whether", CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
	D = StoreFlaggedWord("whither", CONJUNCTION_SUBORDINATE, KINDERGARTEN);
	CreateFact(MakeMeaning(D), Mmember, MakeMeaning(Dqword));
}

static char* ReadWord(char* ptr, char* spot) //   mass of  non-whitespace
{
	ptr = SkipWhitespace(ptr);
	char* original = spot;
	*spot = 0;
	if (!ptr || !*ptr) return NULL;

	if (*ptr == '"' || (*ptr == '\\' && ptr[1] == '"'))  //   grab whole string -- but if space follows quote, check if its like 17" 
	{
		char* beginptr = ptr;
		ptr = ReadQuote(ptr, spot);
		if (ptr != beginptr) return ptr;
	}

	char c;
	while ((c = *ptr++) && !IsWhiteSpace(c) && c != '`')  *spot++ = c;  //   ways of ending the string
	*spot = 0;
	if (original[0] == '"' && original[1] == '"' && original[2] == 0) *original = 0;
	if (*ptr == ENDUNIT) 
		ReportBug("funny separator shouldnt be found");
	return ptr;
}

bool IgnoreEntry(char* word)
{
	if (!strnicmp(word, "atomic_number_", 14) && IsDigit(word[14])) return true;
	if (!strnicmp(word, "element_", 8) && IsDigit(word[8])) return true;
	if (!strnicmp(word, "capital_of", 10)) return true;
	if (!strnicmp(word, "family_", 7))
	{
		size_t len = strlen(word);
		if (word[len - 1] == 'e' && word[len - 2] == 'a') return true;	//   family_xxxae  latin name
	}
	return false;
}

static void readIndex(const char* file, uint64 prior)
{// has the ORDER of most likely meaning that needs to change the meanings list of a word
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing index file %s\r\n", file);
		return;
	}
	currentFileLine = 0;
	int synsetCount;
	char pos[50];
	int junk;
	while (ReadALine(readBuffer, in) >= 0)
	{
		if (readBuffer[0] == ' ' || readBuffer[0] == 0) continue;
		char* ptr = ReadWord(readBuffer, word); //   get lowercase version of the word
		strcpy(word, JoinWords(BurstWord(word, CONTRACTIONS), false)); // our formatting
		if (IgnoreEntry(word)) continue;
		ptr = ReadWord(ptr, pos); //  pos type (n,v,a,r)
		if (*pos == 'a')  *pos = 'j';
		else if (*pos == 'r')  *pos = 'b';

		// NOTE-- index words are all in lower case, even though the actual word might be upper case

		WORDP D = FindWord(word);
		if (!D)
			continue; // HUH??

		ptr = SkipWhitespace(ptr);
		ptr = ReadInt(ptr, synsetCount); //  number of synsets it particates in, IN ORDER
		ptr = SkipWhitespace(ptr);
		ptr = ReadInt(ptr, junk); //  ptr count
		for (int i = 0; i < junk; ++i)
		{
			ptr = SkipWhitespace(ptr);
			ptr = ReadWord(ptr, word); // skip ptr tpe
		}
		ptr = SkipWhitespace(ptr);
		ptr = ReadInt(ptr, junk); //  ignore sense_cnt
		ptr = SkipWhitespace(ptr);
		ptr = ReadInt(ptr, junk); //  ignore tagsense_cnt
		if (synsetCount >= 63) synsetCount = 63;//   refuse to go beyond this
		int actual = 1; // the actual slot to put it in, increases monotonically while "i" may miss because not found
		while (actual < GetMeaningCount(D)) // skip previously sorted word pos types
		{
			if (Meaning2Word(GetMeaning(D, actual))->properties & prior)
			{
				++actual;
			}
			else break;
		}
		int i;
		for (i = 0; i < synsetCount; ++i)
		{
			ptr = ReadWord(ptr, word); // skip ptr tpe
			strcat(word, pos);  // name of synset...
			WORDP master = FindWord(word);
			if (!master)
				continue; // huh?
			MEANING masterMeaning = MakeMeaning(master);
			for (int j = 1; j <= GetMeaningCount(D); ++j)
			{
				if (GetMeaning(D, j) == masterMeaning) // found it on the lower case word - if we dont find it, it will be on upper case, and we dont CARE about order there.
				{
					if (j != actual) // occurs later or earlier, must move it here by swap
					{
						MEANING M = GetMeaning(D, actual);
						GetMeanings(D)[actual] = masterMeaning;
						GetMeanings(D)[j] = M;
					}
					++actual;
					break;
				}
			}
		}
	}
	FClose(in);
}

static void readData(const char* file)
{
	char word[MAX_WORD_SIZE];
	uint64 synsetCount;
	int ptrCount;
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing data file %s\r\n", file);
		return;
	}
	char* gloss = AllocateBuffer();

	currentFileLine = 0;
	MEANING meanings[1000];
	unsigned int meaningIndex;
	while (ReadALine(readBuffer, in) >= 0)
	{
		if (readBuffer[0] == ' ' || readBuffer[0] == 0) continue;
		//lemma pos synset_cnt p_cnt [ptr_symbol...] sense_cnt tagsense_cnt synset_offset [synset_offset...] 
		//   looks like this: 01302724 05 n 01 pet 0 001 @ 00015024 n 0000 | a domesticated animal kept for companionship or amusement  
		//   which means: id 01302724 from lexfile 5 is type n (noun) and has 1 word in synset
		//   1st word is "pet" lexid 
		//   there are 1 links to other synsets. the @ LINK TO 00015024 type noun
		//   EACH link is: pointer_symbol synset_offset pos source/target
		//   there are 0 frames (only verbs have frames)
		//   The gloss is after | 
		char* ptr = readBuffer;
		meaningIndex = 0;
		char id[MAX_WORD_SIZE];
		char junk[MAX_WORD_SIZE];
		char pos[MAX_WORD_SIZE];
		ptr = ReadWord(ptr, id); //   this is the offsetid for this word
		if (!*id) break;
		int lexnoun = 0;
		ptr = SkipWhitespace(ptr);
		ptr = ReadInt(ptr, lexnoun); //   the lexfile it comes from
		unsigned int lex = lexnoun;
		lexnoun = 1 << lexnoun; //   bit representation
		if (lex > 28) //   28 is lextime value from wordnet //   beyond nouns, into verbs
		{
			// int lexverb = 1 << (lex - 28);
			lexnoun = 0;
		}
#ifdef INFORMATION
		troponym  more precise form of a general verb
			Lexname names the lexographer files :
		00  adj.all  all adjective clusters
			01  adj.pert  relational adjectives(pertainyms) related to or pertaining to
			02  adv.all  all adverbs
			44  adj.ppl  participial adjectives
#endif
			ptr = ReadWord(ptr, pos); //   get pos type
		uint64 flags = 0;
		uint64 sysflags = 0;
		if (pos[0] == 'n')
		{
			flags = NOUN;
			strcat(id, "n");
		}
		else if (pos[0] == 'v')
		{
			flags = VERB | VERB_INFINITIVE | VERB_PRESENT;
			strcat(id, "v");
		}
		else if (pos[0] == 'a' || pos[0] == 's')
		{
			flags = ADJECTIVE | ADJECTIVE_NORMAL;
			sysflags = 0;
			strcat(id, "j");
		}
		else if (pos[0] == 'r')
		{
			flags = ADVERB;
			sysflags = 0;
			strcat(id, "b");
		}
		WORDP master = StoreFlaggedWord(id, flags, sysflags);
		master->internalBits |= WORDNET_ID;
		MEANING masterMeaning = MakeMeaning(master, 0) | (unsigned int)(flags & BASIC_POS); // synset head ptr
		ptr = SkipWhitespace(ptr);
		ptr = ReadHex(ptr, synsetCount); //   number of words in the synset
		uint64 oldflags = flags;
		while (synsetCount-- > 0) //   dictionary items participating in this synset 
		{
			flags = oldflags;

			bool upcase = false;
			ptr = ReadCompiledWord(ptr, word); //   the id

			strcpy(word, JoinWords(BurstWord(word, CONTRACTIONS)));
			//   ADJECTIVE may have (xx) after it indicating restrictions:
			//(p)    predicate position 
			//(a)    prenominal (attributive) position 
			//(ip)   immediately postnominal position 
			char* px = strchr(word, '(');
			char restrict = 0;
			if (px)
			{
				restrict = px[1];
				*px = 0;	// remove restriction "word(p)"
			}
			else if (*ptr == '(')
			{
				restrict = ptr[1];
				while (*ptr++ != ')'); //remove restriction
			}
			size_t len = strlen(word);
			for (unsigned int k = 0; k < len; ++k) //   check casing of word
			{
				if (IsUpperCase(word[k])) upcase = true;
			}
			ptr = ReadWord(ptr, junk); //   skip lexid
									   //   adjust wordnet composite word entries that dont match our tokenization, like cat's_eye should be cat_'s_eye
			if (IgnoreEntry(word)) continue;

			int n = BurstWord(word);
			strcpy(word, JoinWords(n, false));
			char original[MAX_WORD_SIZE];
			strcpy(original, word);

			if (upcase && flags & NOUN) //   prove this is a proper name, not something like TV_show
			{ //Not enough to start with upper. van_Eyck is proper, for instance.
				char* start = original;
				char* xptr = original;
				while (ALWAYS)
				{
					if (*++xptr == 0 || *xptr == '_') //   end of a chunk
					{
						WORDP W = FindWord(start, xptr - start, PRIMARY_CASE_ALLOWED);
						if (W && !(W->properties & (DETERMINER | PREPOSITION | CONJUNCTION))) //   word needs to be capped if title
						{
							if (IsLowerCase(*start))  break;//   lower case word makes this an unlikely proper name
						}

						if (*xptr == 0) break;
						start = xptr + 1;
					}
				}
			}
			bool place = IsPlaceNumber(word);
			// leave word numbers like "score" alone
			if (IsNumber(word) != NOT_A_NUMBER && !place && IsDigit(word[0])) continue; // dont bother with number adjective or adverb definitions?
			if (place && flags & (NOUN | ADJECTIVE)) continue; //dont do place numbers in dictionary
			if (!strcmp(word, "MArch")) strcpy(word, "March"); // patch master of arch which comes first
			WORDP D = StoreFlaggedWord(word, flags, sysflags);
			if (flags & ADJECTIVE)
			{
				if (restrict == 'p') AddSystemFlag(D, ADJECTIVE_ONLY_PREDICATE); // predicate  he was *afraid
				if (restrict == 'a') AddSystemFlag(D, ADJECTIVE_NOT_PREDICATE); // attributive "the *lonely boy"
				if (restrict == 'i') AddSystemFlag(D, ADJECTIVE_POSTPOSITIVE); // postnominal "we have rooms *available"
			}
			else if (flags & ADVERB)
			{
			}
			else if (flags & NOUN)
			{
				AddProperty(D, (D->internalBits & UPPERCASE_HASH) ? NOUN_PROPER_SINGULAR : NOUN_SINGULAR);
			}
			else if (flags & VERB)
			{
			}
			if (meaningIndex < 63 && GetMeaningCount(D) <= 63)
			{
				if (AddMeaning(D, masterMeaning)) // they point to each other, unless overflow
				{
					meanings[meaningIndex++] = MakeMeaning(D, GetMeaningCount(D)) | (flags & BASIC_POS); // most recent meaning reference
				}
			}
		}
		if (meaningIndex == 0) //   no meanings?  
		{
			AddInternalFlag(master, DELETED_MARK);
			continue;
		}

		//   now link up rest of synset members to master (they are all unaware of the other members)
		//   transfer key top properties down

		//   for each slave, they point to this synset header node as a meaning and he points to them (though synset might pt to an upper and a lower case form of the same word)
		//   coming out of above, master is the MEANING used for this synset id

		ptr = ReadInt(ptr, ptrCount); //   number of  relational pointers
		while (ptrCount-- > 0) //   read index marker quad
		{ //   ptrtype synsetoffset pos  source/target
			char ptrtype[MAX_WORD_SIZE];
			ptr = ReadWord(ptr, ptrtype);
			if (*ptrtype == '|')
			{
				Log(USERLOG, "ran into defn\r\n");
				ptr -= 2;
				break;
			}
			char refsyn[MAX_WORD_SIZE];
			ptr = ReadWord(ptr, refsyn); //   the other synset
			ptr = ReadWord(ptr, pos);   //   the pos type
			if (*ptrtype == '|')
			{
				Log(USERLOG, "ran into defn\r\n");
				ptr -= 2;
				break;
			}
			uint64 source, target;
			ptr = SkipWhitespace(ptr);
			char c = ptr[2];
			ptr[2] = 0;
			ReadHex(ptr, source); //   which word of ours (0 = all)
			ptr[2] = c;
			ptr = ReadHex(ptr + 2, target); //   which word of his (0 = all)
			uint64 wordkind = 0;
			if (*pos == 'n')
			{
				strcat(refsyn, "n");
				wordkind = NOUN;
			}
			else if (*pos == 'v')
			{
				strcat(refsyn, "v");
				wordkind = VERB;
			}
			else if (*pos == 'a' || *pos == 's')
			{
				strcat(refsyn, "j");
				wordkind = ADJECTIVE;
			}
			else if (*pos == 'r')
			{
				strcat(refsyn, "b");
				wordkind = ADVERB;
			}
			MEANING masterid = MakeMeaning(master, (int)source);
			WORDP referd = StoreWord(refsyn, wordkind);
			referd->internalBits |= WORDNET_ID;
			MEANING referMeaning = MakeMeaning(referd, (int)target);

			//   ALL FACTS IN ONTOLOGY are hooked from HEADS of synsets only
			char referName[MAX_WORD_SIZE]; //   things in the list of relations to the entry item
			referName[0] = 0;
			strcpy(referName, WriteMeaning(referMeaning));
			char TName[MAX_WORD_SIZE]; //   THE ENTRY ITEM
			TName[0] = 0;
			strcpy(TName, WriteMeaning(masterid));
			if (!(flags & NOUN));	// ONLY ACCEPT NOUN HIERARCHY OF WORDNET
			else if (!strcmp(ptrtype, "+")) { ; } //   derivationaly related
			else if (!strcmp(ptrtype, "&")) { ; } //   adj which comes from verb -- SIMILAR
			else if (!strcmp(ptrtype, "<")) { ; } //   adj which comes from verb
			else if (!strcmp(ptrtype, "*")) { ; } //   implies (verb)
			else if (!strcmp(ptrtype, ">")) { ; } //   cause (verb)
			else if (!strcmp(ptrtype, "=")) //   attribute (noun)
			{
				//	CreateFact(masterid ,MakeMeaning(StoreWord("attribute")),referMeaning);   //   unworthy attribute/value of worthiness
			}
			else if (!strcmp(ptrtype, "@") || !strcmp(ptrtype, "@i")) //   we are a child of or an instance of (NOUNS AND VERBS ONLY)
			{
				CreateFact(masterid, Mis, referMeaning); //we have refer as parent (and he has us as child) -- NOT just for classes.
			}
			else if (!strcmp(ptrtype, "!")) { ; } //     see opposite.tbl
			else if (!strcmp(ptrtype, "~")) //   referName is a child of TName (NOUNS and VERBS ONLY)
			{
				CreateFact(referMeaning, Mis, masterid); //   there can be multiple is_a pointers (like person is causal_agent and an organism )
			}
			else if (!strcmp(ptrtype, "#m")) { ; }//   member of (second baseman)
			else if (!strcmp(ptrtype, "%m")) { ; } //   whole of (baseball team)
			else if (!strcmp(ptrtype, "%p")) { ; }//   has as parts
			else if (!strcmp(ptrtype, "#p")) { ; } //   part of it
			else if (!strcmp(ptrtype, "#s")) { ; } //   flour goes into cake
			else if (!strcmp(ptrtype, "%s")) { ; } //   joint made of cannabis
		}
		flags = oldflags;

		//   mark multiword verbs phrasal/prepositional, as requiring direct object and contiguous or split
		if (flags & VERB)
		{
			for (unsigned int i = 0; i < meaningIndex; ++i)
			{
				MEANING M = meanings[i];
				if (!M) continue;	//   got erased
				WORDP D = Meaning2Word(M); //   THIS was the dictionary name
				char* xptr = strrchr(D->word, '_');
				if (!xptr) continue;
				//   look at last word. if it is preposition verb REQUIRES direct object
				// WORDP E = FindWord(xptr + 1, 0, PRIMARY_CASE_ALLOWED);
			}
		}

		//   verbs  have prototype sentences format (indicating the need for an object or not)
		if (ptr[1] != '|' && flags & VERB)
		{
			int frameCnt; //   number of entries
			ptr = SkipWhitespace(ptr);
			ptr = ReadInt(ptr, frameCnt);
			while (frameCnt-- > 0) //   for each entry
			{
				ptr = ReadWord(ptr, junk); //   the +
				int frameNumber;
				ptr = SkipWhitespace(ptr);
				ptr = ReadInt(ptr, frameNumber); //   from list beow
				if (!frameNumber) assert(false);
#ifdef INFORMATION
				x1    Something----s   INTRANS
					x2    Somebody----s	INTRANS
					x3    It is----ing		INTRANS
					x ? 4    Something is----ing PP	INTRANS
					x ? 5    Something----s something Adjective / Noun(does this mean modified noun or adj ? )  ADJOBJECT
					x ? 6    Something----s Adjective / Noun  ADJOBJECT
					x7    Somebody----s Adjective  ADJOBJ
					x8    Somebody----s something  TRANS
					x9    Somebody----s somebody TRANS
					x10    Something----s somebody TRANS
					x11    Something----s something TRANS
					12    Something----s to somebody
					13    Somebody----s on something
					x14    Somebody----s somebody something TRANS(dual)
					15    Somebody----s something to somebody
					16    Somebody----s something from somebody
					17    Somebody----s somebody with something
					18    Somebody----s somebody of something
					19    Somebody----s something on somebody
					20    Somebody----s somebody PP
					21    Somebody----s something PP
					22    Somebody----s PP
					x23    Somebody s(body part)----s INTRANS
					24    Somebody----s somebody to INFINITIVE x
					25    Somebody----s somebody INFINITIVE  x
					26    Somebody----s that CLAUSE
					27    Somebody----s to somebody
					28    Somebody----s to INFINITIVE  x
					29    Somebody----s whether INFINITIVE
					30    Somebody----s somebody into V - ing something
					31    Somebody----s something with something
					32    Somebody----s INFINITIVE  x
					33    Somebody----s VERB - ing
					34    It----s that CLAUSE
					35    Something----s INFINITIVE x
#endif
					uint64 synsetNum;
				ptr = SkipWhitespace(ptr);
				ptr = ReadHex(ptr, synsetNum); //   word in synset it applies to (00 means ALL)
				for (unsigned int i = 0; i < meaningIndex; ++i)
				{
					MEANING M = meanings[i];
					if (!M) continue;	//   deleted meaning
					WORDP D = Meaning2Word(M); //   THIS was the dictionary name
					if (synsetNum == 0 || synsetNum == (i - 1)) //   all or us?
					{
						if (frameNumber == 1 || frameNumber == 2 || frameNumber == 3 || frameNumber == 4 || frameNumber == 23)
						{
							AddSystemFlag(D, VERB_NOOBJECT);
							// ALL can be used w/o object
						}
						else if (frameNumber == 8 || frameNumber == 9 || frameNumber == 10 || frameNumber == 11 || (frameNumber >= 15 && frameNumber <= 21)) AddSystemFlag(D, VERB_DIRECTOBJECT);
						else if (frameNumber == 24)  AddSystemFlag(D, VERB_TAKES_INDIRECT_THEN_TOINFINITIVE);
						else if (frameNumber == 25)
						{
							AddSystemFlag(D, VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE);
						}
						//   BUG- what if frame 6 uses noun? how do we know what that looks like?
						else if (frameNumber == 31 || frameNumber == 30 || frameNumber == 7 || frameNumber == 6 || frameNumber == 5)
						{
							//AddSystemFlag(D,VERB_ADJECTIVE_OBJECT); //meaningless? not a linking verb
						}
						else if (frameNumber == 14) AddSystemFlag(D, VERB_INDIRECTOBJECT | VERB_DIRECTOBJECT);
						else if (frameNumber == 28) AddSystemFlag(D, VERB_TAKES_TOINFINITIVE);
						else if (frameNumber == 32 || frameNumber == 35) AddSystemFlag(D, VERB_TAKES_VERBINFINITIVE);
						else if (frameNumber == 33) AddSystemFlag(D, VERB_TAKES_GERUND);
						else
						{
							//  ReportBug("unknown sentence construction");
						}
					}
					if (synsetNum != 0) //   we are done with the specific one we wanted
						break;
				}
			}
		}
		if (ptr[0] == '|') ++ptr;
		else if (ptr[1] == '|') ptr += 2;
		else ReportBug("baddata");
			if (*ptr == ' ') ++ptr; //   skip leading blank

									//   grab gloss - erase end of line and any quote area
		char* p = gloss;
		while (*ptr != 0 && *ptr != '\n' && *ptr != '\r' && *ptr != '"') *p++ = *ptr++;
		*p = 0;

		//   delete a parenthetical expression from gloss
		if ((p = strchr(gloss, '(')))
		{
			char* q = strchr(p, ')');
			if (q) strcpy(p, q + 1);
			else *p = 0; //   failed to find end, just delete it all
		}
		// delete notes like: -- used only in infinitive form
		p = strstr(gloss, "--");
		if (p) *p = 0;
		TrimSpaces(gloss);
		char* at = strchr(gloss, ';');
		if (at) *at = 0;
		at = gloss;
		while (*at == ' ') at++;

		if (*at)
		{
			// now remove excess blanks (so readcompiledword doesnt complain)
			char* xptr = at;
			while (*++xptr)
			{
				if (*xptr == ' ' && xptr[1] == ' ')
				{
					memmove(xptr, xptr + 1, strlen(xptr));
					--xptr;
				}
			}
			AddGloss(master, AllocateHeap(at), 1);
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readConjunction(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing conjunction file %s\r\n", file);
		return;
	}
	while (fget_input_string(false, false, readBuffer, in) != 0)
	{
		if (readBuffer[0] == '#' || readBuffer[0] == 0) continue;
		char* ptr = readBuffer;
		ptr = ReadWord(ptr, word);    //   the conjunction
		if (*word == 0 || *word == '#')  continue;
		WORDP D = StoreFlaggedWord(word, 0, KINDERGARTEN);

		// the flags to add
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word); //   see if  PROPERTY exists
			if (!*word || *word == '#') break;
			uint64 bit = FindPropertyValueByName(word);
			if (bit) AddProperty(D, bit);
			else
			{
				bit = FindSystemValueByName(word);
				if (bit) AddSystemFlag(D, bit);
			}
		}
		if (D->properties & CONJUNCTION) AddMeaning(D, 0);//   reserve meaning untyped (conjunction) -- dont reserve meaning for adverb

	}
	FClose(in);
}

static void readWordKind(const char* file, unsigned int flags)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing wordkind file %s\r\n", file);
		return;
	}
	uint64 wordkind = 0;
	uint64 sys = 0;
	while (fget_input_string(false, false, readBuffer, in) != 0)
	{
		if (readBuffer[0] == '#' || readBuffer[0] == 0) continue;
		char* ptr = readBuffer;
		while (ALWAYS)//   read all words on this line
		{
			ptr = ReadWord(ptr, word);
			if (*word == 0 || *word == '#') break;
			if (*word == ':') //   word class (preposition, noun, etc - will be on a line by itself
			{
				MakeUpperCase(word);
				wordkind = FindPropertyValueByName(word + 1);
				sys = 0;
				if (wordkind == ADJECTIVE)
				{
					wordkind |= ADJECTIVE_NORMAL;
					sys = 0;
				}
				else if (wordkind == ADVERB)
				{
					wordkind |= ADVERB;
					sys = 0;
				}
				else if (!wordkind) ReportBug("word class not found %s", word);
			}
			else
			{
				WORDP D = StoreFlaggedWord(word, wordkind, sys);
				AddSystemFlag(D, flags | KINDERGARTEN);
				if (D->properties & NOUN)
				{
					char flag[MAX_WORD_SIZE];
					char* at = ReadWord(ptr, flag);
					if (!stricmp(flag, "NOUN_NUMBER"))
					{
						ptr = at;
						RemoveProperty(D, NOUN_SINGULAR | NOUN_PLURAL);
						AddProperty(D, NOUN_NUMBER);
						AddSystemFlag(D, NOUN_NODETERMINER);
					}
					else if (D->internalBits & UPPERCASE_HASH) AddProperty(D, NOUN_PROPER_SINGULAR);
					else if (IsNumber(D->word, false) != NOT_A_NUMBER)
					{
						AddProperty(D, NOUN_NUMBER);
						AddSystemFlag(D, NOUN_NODETERMINER);
					}
					else AddProperty(D, NOUN_SINGULAR);
				}
				if (D->properties & ADJECTIVE)
				{
					if (IsNumber(D->word, false) != NOT_A_NUMBER)
					{
						RemoveProperty(D, ADJECTIVE_BITS);
						AddProperty(D, ADJECTIVE_NUMBER);
					}
				}
				if (IsPlaceNumber(word))
				{
					RemoveProperty(D, ADJECTIVE_BITS | NOUN_NUMBER);
					AddProperty(D, ADJECTIVE | ADJECTIVE_NUMBER | NOUN_NUMBER); // web says must only be adjective, but "the 4th of July" requires noun
					AddSystemFlag(D, ORDINAL|NOUN_NODETERMINER);
					AddParseBits(D, QUANTITY_NOUN);
				}
			}
		}
	}
	FClose(in);
}

static void readPrepositions(const char* file, bool addmeaning)
{ //   LINE oriented
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing prepositions file %s\r\n", file);
		return;
	}
	WORDP D;
	while (fget_input_string(false, false, readBuffer, in) != 0)
	{
		if (readBuffer[0] == '#' || readBuffer[0] == 0) continue;
		char* ptr = ReadCompiledWord(readBuffer, word);    //   the prep or kind
		if (*word == 0 || *word == '#') continue; //   end of line
		D = StoreWord(word, PREPOSITION);
		// a prep can also be a noun "behalf" so dont erase it
		if (addmeaning) AddTypedMeaning(D, 0);//   reserve synset (us) meaning but prep is not a type of relevance
		ptr = ReadCompiledWord(ptr, word);
		if (!stricmp(word, "CONDITIONAL_IDIOM"))
		{
			ptr = ReadCompiledWord(ptr, word);
			AddInternalFlag(D, CONDITIONAL_IDIOM);
			D->w.conditionalIdiom = StoreWord(word,AS_IS);
		}
	}
	FClose(in);
}


static void readSpellingExceptions(const char* file) // dont double consonants when making past tense
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing spellingexceptions file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);
		if (*word == 0) continue;
		WORDP D = StoreWord(word, 0);
		AddSystemFlag(D, SPELLING_EXCEPTION);
	}
	FClose(in);
	FreeBuffer();
}

static void AdjNotPredicate(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing adjnotpredicate file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (*input_string == '#' || *input_string == 0) continue;
		char* ptr = input_string;
		while (ptr)
		{
			ptr = ReadWord(ptr, word);
			if (*word == 0) break;
			WORDP D = StoreWord(word, ADJECTIVE | ADJECTIVE_NORMAL);
			AddSystemFlag(D, ADJECTIVE_NOT_PREDICATE);
		}
	}
	FClose(in);
	FreeBuffer();
}

static bool readPronouns(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing pronouns file %s\r\n", file);
		return false;
	}
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the pronoun
		if (*word == 0) continue;
		WORDP D = StoreWord(word, 0);
		AddSystemFlag(D, KINDERGARTEN);
		AddMeaning(D, 0);//   reserve meaning to self - no pronoun type

		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word); //   see if  PROPERTY exists
			if (!*word || *word == '#') break;
			uint64 bit = FindPropertyValueByName(word);
			if (bit) AddProperty(D, bit);
			else
			{
				bit = FindSystemValueByName(word);
				if (bit) AddSystemFlag(D, bit);
			}
		}
	}
	FClose(in);
	FreeBuffer();
	return true;
}

static void readHomophones(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing homophones file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		WORDP D = NULL;
		WORDP E = NULL;
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word);    //   the word
			if (*word == 0) break;
			if (*word == '/') continue;
			if (*word == '(') break; //   pure pronunciation
			unsigned int n = BurstWord(word);
			strcpy(word, JoinWords(n, false));
			D = StoreWord(word, 0);
			if (E)	CreateFact(MakeMeaning(D, 0), MakeMeaning(FindWord("homophone")), MakeMeaning(E, 0));
			E = D;
		}
	}
	FClose(in);
	FreeBuffer();
}

static void AdjustAdjectiveAdverb(WORDP D, uint64 junk) // fix comparitives that look like normal
{
	if (!(D->properties & (MORE_FORM | MOST_FORM))) { ; }
	else return;

	unsigned int len = WORDLENGTH(D);
	char word[MAX_WORD_SIZE];
	strcpy(word, D->word);
	WORDP X;
	if (len > 4 && !strcmp(D->word + len - 2, "er"))
	{
		word[len - 1] = 0; // see if word ENDED in E and R was added
		X = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (!X || !(X->properties & (ADJECTIVE | ADVERB)))
		{
			word[len - 2] = 0;		// see if word had ER added
			X = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		}
		if (!X || !(X->properties & (ADJECTIVE | ADVERB)))
		{
			if (word[len - 3] == word[len - 4]) word[len - 3] = 0; // see if word doubled end consonant to add er
			else if (word[len - 3] == 'i') word[len - 3] = 'y'; // see if word converted y to i and then er
			X = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		}
		if (X && (X->properties & (ADJECTIVE | ADVERB)))
		{
			AddCircularEntry(X, COMPARISONFIELD, D);
			if (D->properties & ADJECTIVE) AddProperty(D, MORE_FORM);
			if (D->properties & ADVERB) AddProperty(D, MORE_FORM);
		}
	}
	else if (len > 5 && !strcmp(D->word + len - 3, "est"))
	{
		word[len - 2] = 0; // see if word ENDED in E and ST was added
		X = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (!X || !(X->systemFlags & (ADJECTIVE | ADVERB)))
		{
			word[len - 3] = 0;		// see if word had EST added
			X = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		}
		if (!X || !(X->systemFlags & (ADJECTIVE | ADVERB)))
		{
			if (word[len - 4] == word[len - 5]) word[len - 4] = 0; // see if word doubled end consonant to add est
			else if (word[len - 4] == 'i') word[len - 4] = 'y'; // see if word converted y to i and then er
			X = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		}
		if (X && X->systemFlags & (ADJECTIVE | ADVERB))
		{
			AddCircularEntry(X, COMPARISONFIELD, D);
			if (D->properties & ADJECTIVE) AddProperty(D, MOST_FORM);
			if (D->properties & ADVERB) AddProperty(D, MOST_FORM);
		}
	}
}

static void ReadBNCPosData()
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal("RAWDICT/bncfreq1.txt");
	if (!in)
	{
		Log(USERLOG, "** Missing file RAWDICT/bncfreq1.txt\r\n");
		return;
	}
	showBadUTF = false;
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* ptr = ReadCompiledWord(readBuffer, word);    //   word NOUN:4 VERB:0 ADJECTIVE:0 ADVERB:0 PREOPOSTION:0 CONJUNCTION:0 AUX:0 
		if (word[0] == '#' || word[0] == 0) continue;
		WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (D && D->internalBits & DELETED_MARK) D = NULL; // deleted word
		if (!D || !(D->properties & PART_OF_SPEECH))  continue; // we dont know this word naturally already and they have junk words...

		int best = 0;
		int count = 0;
		int ntally, vtally, jtally, rtally, ptally;
		int ctally = 0;
		ptr = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(ptr, word); // NOUN:
		ReadInt(word + 5, ntally);
		if (ntally > best) best = ntally;

		ptr = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(ptr, word); // VERB:
		ReadInt(word + 5, vtally);
		if (vtally > best)  best = vtally;

		ptr = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(ptr, word); // ADJECTIVE:
		ReadInt(word + 10, jtally);
		if (jtally > best)  best = jtally;

		ptr = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(ptr, word); // ADVERB:
		ReadInt(word + 7, rtally);
		if (rtally > best)  best = rtally;

		ptr = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(ptr, word); // PREPOSITION:
		ReadInt(word + 12, ptally);
		if (ptally > best)
			best = ptally;

		ptr = SkipWhitespace(ptr);
		ptr = ReadCompiledWord(ptr, word); // CONJUNCTION:
		ReadInt(word + 12, ctally);
		if (ctally > best)
			best = ctally;

		// now remove clear noncontenders
		if (ntally && ((ntally * 2) < best)) ntally = 0;
		if (vtally && ((vtally * 2) < best)) vtally = 0;
		if (jtally && ((jtally * 2) < best)) jtally = 0;
		if (rtally && ((rtally * 2) < best)) rtally = 0;
		if (ptally && ((ptally * 2) < best)) ptally = 0;
		if (ctally && ((ctally * 2) < best)) ctally = 0;
		uint64 newflags = 0;

		if (ntally) newflags |= NOUN;
		if (vtally) newflags |= VERB;
		if (jtally) newflags |= ADJECTIVE;
		if (rtally) newflags |= ADVERB;
		if (ptally || ctally) newflags |= PREPOSITION; // either
		AddSystemFlag(D, newflags);
	}
	FClose(in);
	showBadUTF = true;
}

static void readFix(const char* file, uint64 flag) //   locate a base form from an inflection
{
	char word[MAX_WORD_SIZE];
	char baseword[MAX_WORD_SIZE];
	char word2[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing fix file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (ReadALine(input_string, in) >= 0)
	{
		//   file is always pairs of words, with base the 2nd
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the inflected/modified form
		if (*word == 0)
			continue;
		if (!stricmp(word, "won") || !stricmp(word, "shot") || !stricmp(word, "spat")) continue;	//   dont want wordnet wonned won stuff
		ptr = ReadWord(ptr, baseword); //   2nd form is the base
		if (!stricmp(word, baseword))
		{
			Log(USERLOG, "     inflected %s not different\r\n", word);
			continue;
		}

		WORDP B = StoreWord(baseword, flag);
		WORDP D = StoreWord(word, flag);
		if (flag & VERB) //   will be PAST or PARTICIPLE, NO guarantee of order (comes in alphabetically)
		{
			AddProperty(B, VERB_INFINITIVE | VERB_PRESENT);
			if (!stricmp(B->word, "bit") || !stricmp(B->word, "be") || !stricmp(B->word, "have")) continue; //   they dont conjugate in wordnet in proper order for these to be accepted
			size_t len = strlen(word);

			if (word[len - 1] == 'g' && word[len - 2] == 'n' && word[len - 3] == 'i') AddProperty(D, VERB_PRESENT_PARTICIPLE);
			else if (GetTense(B)) AddProperty(D, VERB_PAST_PARTICIPLE);
			else AddProperty(D, VERB_PAST);  // 1st one will be past  
			AddCircularEntry(B, TENSEFIELD, D);
		}
		if (flag & ADJECTIVE && stricmp(baseword, word)) // ignore dual same stuff, like modest modest from wordnet
		{
			size_t len = strlen(word);
			//   base cannot also be comparative form
			if (B->properties & (MORE_FORM | MOST_FORM)) Log(USERLOG, "inconsistent adjective irregular %s\r\n", B->word);
			else
			{
				AddCircularEntry(B, COMPARISONFIELD, D);
				RemoveProperty(D, MORE_FORM);
				AddProperty(D, (word[len - 1] == 't' && word[len - 2] == 's') ? MOST_FORM : MORE_FORM);
			}
		}
		if (flag & ADVERB && stricmp(baseword, word)) // ignore dual same stuff from wordnet
		{
			//   base cannot also be comparative form
			if (B->properties & (MORE_FORM | MOST_FORM)) Log(USERLOG, "inconsistent adjective irregular %s\r\n", B->word);
			AddCircularEntry(B, COMPARISONFIELD, D);
			RemoveProperty(D, MOST_FORM);
			AddProperty(D, MORE_FORM);
			ReadWord(ptr, word2);	// "most" form
			if (*word2)
			{
				WORDP G = StoreWord(word2, ADVERB | ADVERB | MOST_FORM);
				RemoveProperty(G, MORE_FORM);
				AddCircularEntry(B, COMPARISONFIELD, G);
			}
		}
		if (flag & NOUN)
		{
			RemoveProperty(B, NOUN_PLURAL);
			AddProperty(B, NOUN_SINGULAR);
			if (stricmp(B->word, D->word)) RemoveProperty(D, NOUN_SINGULAR); // different words word
			AddProperty(D, NOUN_PLURAL);
			if (stricmp(D->word, B->word) && !(D->properties & PRONOUN_BITS))    //   not same word, can they find
			{
				WORDP plural = FindCircularEntry(D, NOUN_PLURAL);
				if (plural == D) { ; } // already on list and is good
				else if (plural) {;} // on list but NOT what we are expecting
					//Log(USERLOG, "plural->Singular collision %s %s %s\r\n", B->word, D->word, plural->word);
				else AddCircularEntry(D, PLURALFIELD, B);
			}
		}
	}
	FClose(in);
	FreeBuffer();
}

static void ReadTitles(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing titles file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == 0) continue;
		char* ptr = input_string;
		while (ptr && *ptr)
		{
			ptr = ReadWord(ptr, word);    //   the noun
			if (*word == 0 || *word == '#') break;
			WORDP D = StoreFlaggedWord(word, NOUN_TITLE_OF_ADDRESS | NOUN | NOUN_PROPER_SINGULAR, KINDERGARTEN);
			RemoveInternalFlag(D, DELETED_MARK);
		}
	}
	FClose(in);
	FreeBuffer();
}

static void InsurePhrasalMeaning(char* verb, char* gloss)
{
	// mark all particles
	char* end = strrchr(verb, '_');
	StoreWord(end + 1, PARTICLE);  // mark last word as a particle

								   // in case of 4 long phrase which we dont handle "be_cut_out_for"  but we handle 

	unsigned int index;
	WORDP D = FindWord(verb, 0, PRIMARY_CASE_ALLOWED); // does it already exist?
	unsigned int hasgloss = 0;
	unsigned int nogloss = 0;
	bool hasVerbMeaning = false;
	if (D && D->properties & VERB && D->properties & VERB_INFINITIVE) // has prexising verbs
	{
		for (unsigned int i = GetMeaningCount(D); i >= 1; --i)
		{
			MEANING M = GetMeaning(D, i);
			if (!(M & VERB)) continue;
			// self pointing sysnet verb ref will be what we want
			if (M & SYNSET_MARKER && Meaning2Word(M) == D)
			{
				if (GetGloss(D, i)) hasgloss = i;
				else nogloss = i;
			}
			else hasVerbMeaning = true;
		}
	}
	D = StoreFlaggedWord(verb, VERB | VERB_INFINITIVE | VERB_PRESENT, PHRASAL_VERB | VERB_CONJUGATE1);
	if (hasVerbMeaning) { ; } // from wordnet already
	else if (!hasgloss && nogloss)
	{
		if (gloss) AddGloss(D, AllocateHeap(gloss), nogloss); // found an existing place to add it
	}
	else if (!hasgloss && !nogloss)// create a meaning for it
	{
		AddTypedMeaning(D, VERB);
		index = GetMeaningCount(D);
		if (gloss) AddGloss(D, AllocateHeap(gloss), index);
	}
}

static void ReadPhrasalVerb(const char* file)
{
	char verb[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing phrasalverb file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		char* ptr = input_string;
		ptr = ReadCompiledWord(ptr, verb);    //   the word
		if (!*verb || *verb == '#')   continue;
		char* gloss = strchr(input_string, ':');
		if (gloss) *gloss++ = 0;
		else gloss = NULL;

		// read words or flags of various flavors
		uint64 flags = 0;
		while (ptr && *ptr)
		{
			ptr = ReadCompiledWord(ptr, word);
			if (!*word) break;
			else if (IsUpperCase(*word)) flags |= FindSystemValueByName(word);// direct flags of our sort given
		}

		WORDP X = FindWord(verb);
		if (X)
		{
			//		if (flags & VERB_NOOBJECT && !(X->systemFlags & VERB_NOOBJECT)) Log(USERLOG,"Extra noobject on %s\r\n",X->word);
			//		if (flags & VERB_DIRECTOBJECT && !(X->systemFlags & VERB_DIRECTOBJECT)) Log(USERLOG,"Extra directobject on %s\r\n",X->word);
			//		continue;
		}

		// now add in word
		InsurePhrasalMeaning(verb, gloss);
		StoreFlaggedWord(verb, VERB | VERB_PRESENT | VERB_INFINITIVE, flags);
	}
	FClose(in);
	FreeBuffer();
}

static void readCommonness()
{
	FILE* in = fopen("RAWDICT/1gramfreq.txt", "rb");
	if (!in) return;
	while (ReadALine(readBuffer, in) >= 0)
	{
		char word[MAX_WORD_SIZE];
		char junk[MAX_WORD_SIZE];
		char* ptr = ReadCompiledWord(readBuffer, word);
		ptr = ReadCompiledWord(ptr, junk);
		if (IsUpperCase(word[0])) continue; // ignore proper
		WORDP D = FindWord(word);
		if (!D) continue; // we dont know the word
		unsigned int val = atoi(ptr);
		if (val > 1000000000) AddSystemFlag(D, COMMON7); // above 1 billion
		else if (val > 100000000) AddSystemFlag(D, COMMON6); // above 100 million
		else if (val > 10000000) AddSystemFlag(D, COMMON5);  // above 10 million
		else if (val > 1000000) AddSystemFlag(D, COMMON4);  // above 1 million
		else if (val > 100000) AddSystemFlag(D, COMMON3);  // above 100 K
		else if (val > 10000) AddSystemFlag(D, COMMON2);  // above 10 K
		else if (val > 100) AddSystemFlag(D, COMMON1);  // above 100
														// max 2,147,483,647   
														// min 200
	}
	FClose(in);
}

static void readSupplementalWord(const char* file, uint64 wordkind, uint64 flags)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing supplementalword file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	char condition[MAX_WORD_SIZE];
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the word
		if (!*word || *word == '#')
		{
			continue;
		}
		WORDP D;
		if (wordkind & NOUN)
		{
			// noun
			// noun plural
			D = StoreFlaggedWord(word, wordkind, flags);
			if (D->internalBits & UPPERCASE_HASH) AddProperty(D, NOUN_PROPER_SINGULAR);
			else AddProperty(D, NOUN_SINGULAR);
			AddTypedMeaning(D, NOUN); // self point

			uint64 props = 0;
			while (ptr && *ptr)
			{
				ptr = ReadWord(ptr, word); // "add a meaning or is the plural or is a mass noun
				if (!*word || *word == '#') break;
				if (!stricmp(word, "CONDITIONAL_IDIOM"))
				{
					ptr = ReadWord(ptr, condition);
					AddInternalFlag(D, CONDITIONAL_IDIOM);
					D->w.conditionalIdiom = StoreWord(condition,AS_IS);
					continue;
				}
				if (!stricmp(word, "NOUN_MASS")) flags |= NOUN_NODETERMINER;
				else if (!stricmp(word, "NOUN_PLURAL")) props |= NOUN_PLURAL;
				else if (!stricmp(word, "NOUN_SINGULAR")) props |= NOUN_SINGULAR;
				else if (!stricmp(word, "NOUN_PROPER_SINGULAR")) props |= NOUN_PROPER_SINGULAR;
				else if (!stricmp(word, "NOUN_PROPER_PLURAL")) props |= NOUN_PROPER_PLURAL;
				else if (!stricmp(word, "NOUN_NUMBER")) props |= NOUN_NUMBER;
				else  // is conjugation
				{
					WORDP G = StoreFlaggedWord(word, wordkind, flags);
					if (G->internalBits & UPPERCASE_HASH) AddProperty(G, NOUN_PROPER_PLURAL);
					else AddProperty(G, NOUN_PLURAL);
					RemoveProperty(G, NOUN_PROPER_SINGULAR);
					if (!GetPlural(D))  SetPlural(D, MakeMeaning(G));
					if (!GetPlural(G)) SetPlural(G, MakeMeaning(D));
				}
			}
			if (props)
			{
				AddProperty(D, props);
				if (props & NOUN_SINGULAR && !(props & NOUN_PLURAL)) RemoveProperty(D, NOUN_PLURAL);
				if (props & NOUN_PROPER_SINGULAR && !(props & NOUN_PROPER_PLURAL)) RemoveProperty(D, NOUN_PROPER_PLURAL);
				if (props & NOUN_PLURAL && !(props & NOUN_SINGULAR)) RemoveProperty(D, NOUN_SINGULAR);
				if (props & NOUN_PROPER_PLURAL && !(props & NOUN_PROPER_SINGULAR)) RemoveProperty(D, NOUN_PROPER_SINGULAR);
				if (props & NOUN_NUMBER) RemoveProperty(D, NOUN_SINGULAR | NOUN_PLURAL);
			}
			if (flags) AddSystemFlag(D, flags);
		}
		else if (wordkind & VERB)
		{
			uint64 sysflags = 0;

			// determine conjugation
			int n = BurstWord(word, HYPHENS);	//   find the words
			if (n > 1) //   see spot conjugated
			{
				char* w;
				if ((w = GetBurstWord(0)) && w[0] == '\'') sysflags |= VERB_CONJUGATE1;
				else if (n > 1 && (w = GetBurstWord(1)) && w[0] == '\'') sysflags |= VERB_CONJUGATE2;
				else if (n > 2 && (w = GetBurstWord(2)) && w[0] == '\'') sysflags |= VERB_CONJUGATE3;
				w = strchr(word, '\'');
				if (w) memmove(w, w + 1, strlen(w));	//   erase the conjugation mark

														//   assume it if not given 
				if (!(sysflags & (VERB_CONJUGATE1 | VERB_CONJUGATE2 | VERB_CONJUGATE3)))
				{
					if (strchr(word, '-')) sysflags |= VERB_CONJUGATE2;
					else if (strchr(word, '_')) sysflags |= VERB_CONJUGATE1;
				}
			}
			D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
			bool wasverb = (D && D->properties & VERB) ? true : false;

			// verb MODIFIER words
			char past[MAX_WORD_SIZE];
			uint64 flagx = 0;
			StoreWord(word, 0);
			while (ALWAYS)
			{
				ptr = ReadWord(ptr, past);    //   the flag
				if (!*past || *past == ':' || *past == '#') break;
				WORDP G;
				uint64 flag = FindPropertyValueByName(past);
				uint64 sysx = 0;
				if (!flag) sysx = FindSystemValueByName(past);
				if (!flag && !sysx) //   must be past tense since we dont recognize it as a modifier
				{
					G = StoreWord(past, VERB | VERB_PAST);
					AddCircularEntry(D, TENSEFIELD, G);
				}
				else //   presume for now only synset transitive data
				{
					if (flag & (VERB_PRESENT_PARTICIPLE | VERB_PAST)) // tense given
					{
						ptr = ReadWord(ptr, past);    //   the word
						G = StoreFlaggedWord(past, VERB | flag, sysx);
						AddCircularEntry(D, TENSEFIELD, G);
					}
					else if (flag) AddProperty(D, flag); // property given, 
					else if (sysx) flagx |= sysx;
				}
			}

			char* last = strrchr(word, '_');
			if (last) Log(USERLOG, "Phrasal verb?? %s\r\n", word);
			else
			{
				if (!wasverb)
				{
					D = StoreWord(word, VERB | VERB_INFINITIVE | VERB_PRESENT);
					AddTypedMeaning(D, VERB);//   reserve a meaning
				}
				if (!(D->systemFlags & (VERB_CONJUGATE2 | VERB_CONJUGATE1 | VERB_CONJUGATE3)) && strchr(word, '-')) AddSystemFlag(D, VERB_CONJUGATE2); // assume default for re-xxx pre-xxx etc
				if (!flagx) AddSystemFlag(D, VERB_DIRECTOBJECT | VERB_NOOBJECT); // default all object notations 
				else AddSystemFlag(D, flagx);
			}
		} //   end verb
		else // all other parts of speech
		{
			WORDP E = StoreWord(word);
			uint64 props = wordkind;
			uint64 sys = flags;
			while (ALWAYS)
			{
				char val[MAX_WORD_SIZE];
				ptr = ReadWord(ptr, val);    //   the flag
				if (!*val || *val == '#') break; 
				uint64 flag = FindPropertyValueByName(val);
				if (!stricmp(val,"CONDITIONAL_IDIOM"))
				{
					ptr = ReadWord(ptr, condition);
					props = 0;
					if (E->internalBits & CONDITIONAL_IDIOM) strcat(condition, E->w.conditionalIdiom->word); // merge idiom controls
					else AddInternalFlag(E, CONDITIONAL_IDIOM);
					E->w.conditionalIdiom = StoreWord(condition,AS_IS);
				}
				else if (!flag)
				{
					flag = FindSystemValueByName(val);
					if (flag) sys |= flag;
					else Log(USERLOG, "unknown flag on %s %s\r\n", word, val);
				}
				else  props |= flag;
			}
			if (props & (ADJECTIVE | ADVERB)) AddTypedMeaning(E, props & (ADJECTIVE | ADVERB));
			if (props) AddProperty(E, props);
			if (sys) AddSystemFlag(E, sys);
		}
	}
	FClose(in);
	FreeBuffer();
}

static void ReadDeterminers(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing determiners file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the determiner
		if (*word == 0)
			continue;
		WORDP D = StoreFlaggedWord(word, 0, KINDERGARTEN);
		AddMeaning(D, 0);//   reserve meaning no determiner type

						 // the flags to add
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word); //   see if  PROPERTY exists
			if (!*word || *word == '#') break;
			uint64 bit = FindPropertyValueByName(word);
			if (bit) AddProperty(D, bit);
			else
			{
				bit = FindSystemValueByName(word);
				if (bit) AddSystemFlag(D, bit);
			}
		}
		if (!(D->properties & (PREDETERMINER | PREDETERMINER_TARGET))) AddProperty(D, DETERMINER);
	}
	RemoveProperty(FindWord("a"), NOUN);
	RemoveProperty(FindWord("no"), NOUN);
	RemoveProperty(FindWord("la"), NOUN);
	FClose(in);
	FreeBuffer();
}

static void ReadSystemFlaggedWords(const char* file, uint64 flags)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing systemflagged file %s\r\n", file);
		return;
	}

	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word);    //   the noun
			if (*word == 0 || *word == '#' || *word == '~') break;
			StoreFlaggedWord(word, 0, flags | KINDERGARTEN);
		}
	}
	FClose(in);
	FreeBuffer();
}

static void ReadParseWords(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing file RAWDICT/parsewords.txt\r\n");
		return;
	}
	char* input_string = AllocateBuffer();
	unsigned int parsebit = 0;

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		WORDP D;
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word);
			if (*word == 0) break;
			if (*word == ':')
			{
				parsebit = (unsigned int)FindParseValueByName(word + 1);
				if (!parsebit)
				{
					Log(USERLOG, "** Missing parsebit %s\r\n", word);
					FreeBuffer();
					return;
				}
			}
			else if (*word == '~') {} // just a header label
			else
			{
				D = StoreWord(word);
				AddParseBits(D, parsebit);
			}
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readParticipleVerbs(const char* file) // verbs that should be considered adjective participles after linking verb
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing participleverbs file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the verb
		if (!*word || *word == '#') continue;
		StoreFlaggedWord(word, 0, COMMON_PARTICIPLE_VERB); // leave implied its verb status and adjective status so it can work it out on its own
	}
	FClose(in);
	FreeBuffer();
}

static void readNounNoDeterminer(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing nounnodeterminer file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word);    //   the noun
			if (*word == 0) break;
			WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
			if (!D) continue;
			if (!(D->properties & (NOUN_SINGULAR | NOUN_PROPER_SINGULAR))) continue;
			size_t len = strlen(word);
			if (D->properties & VERB && word[len - 1] == 'g' && word[len - 2] == 'n' && word[len - 3] == 'i') continue;	// skip gerunds
			AddSystemFlag(D, NOUN_NODETERMINER);
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readMonths(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing months file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word);    //   the month
			if (*word == 0) break;
			WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
			if (!D) continue;
			AddSystemFlag(D, MONTH);
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readFirstNames(const char* file) //   human sexed first names
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing firstnames file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		char sex[MAX_WORD_SIZE];
		ptr = ReadWord(ptr, word);    //   the name
		if (*word == 0 || *word == '#') break;
		ptr = ReadWord(ptr, sex);    //   the sex M or F
		if (*sex != 'M' && *sex != 'F') continue; //   bad name
		*word = toUppercaseData[(unsigned char)*word];
		WORDP D = FindWord(word, 0, LOWERCASE_LOOKUP);
		if (D && D->properties & PART_OF_SPEECH)
		{
			if (D->systemFlags & AGE_LEARNED) continue;
		}
		uint64 flag = (*sex == 'F' || *sex == 'f') ? NOUN_SHE : NOUN_HE;
		D = StoreWord(word, NOUN_HUMAN | NOUN | NOUN_FIRSTNAME | NOUN_PROPER_SINGULAR | flag);
		D->meanings = 0; // DONT live with any meanings like Jesus or John or whatever
	}
	FClose(in);
	FreeBuffer();
}

static void readReducedFirstNames(const char* file) //   human sexed first names
{
	char word[MAX_WORD_SIZE];
	char male[MAX_WORD_SIZE];
	char female[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing reducedfirstnames file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the rank
		if (*word == 0 || *word == '#') break;
		ptr = ReadWord(ptr, male);
		ptr = ReadWord(ptr, female);
		WORDP D = FindWord(male);
		if (D && D->properties & (NOUN_HE | NOUN_SHE)) { ; }
		else
		{
			D = StoreWord(male, NOUN_HUMAN | NOUN | NOUN_FIRSTNAME | NOUN_PROPER_SINGULAR | NOUN_HE);
		}
		// if already noun he or she, ignore - Linkparser and normal names are more reliable than reducedNames list
		D = FindWord(female);
		if (D && D->properties & (NOUN_HE | NOUN_SHE)) { ; }
		else
		{
			D = StoreWord(female, NOUN_HUMAN | NOUN | NOUN_FIRSTNAME | NOUN_PROPER_SINGULAR | NOUN_SHE);
		}
		AddSystemFlag(D, KINDERGARTEN);
	}
	FClose(in);
	FreeBuffer();
}

static void readNames(const char* file, uint64 flag, uint64 sys)
{
	//   note dictionary words that are proper names are already marked NOUN_PROPER
	//   AND humans are marked NOUN_HUMAN... Now we may change to having sexed name instead of neutral name
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing names file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word);    //   the name
			if (*word == 0 || *word == '#') break;
			if (IsLowerCase(*word)) *word -= 'a' - 'A';  //   Make 1st letter upper case if its not
			size_t len = strlen(word);
			if (word[len - 2] == '.') word[len - 2] = 0;   //   remove lp's .m or .f  or  .b labelling
			WORDP D = FindWord(word, 0, LOWERCASE_LOOKUP);
			if (D && D->properties & PART_OF_SPEECH)
			{
				if (D->systemFlags & AGE_LEARNED)
				{
					Log(USERLOG, "Name already has common lower meaning %s, ignored\r\n", word);
					continue;
				}
			}
			D = StoreFlaggedWord(word, flag, sys);
			D->meanings = 0;	// remove all definitions
			AddProperty(D, NOUN_PROPER_SINGULAR);
			if ((D->properties & (NOUN_HE | NOUN_SHE)) == (NOUN_HE | NOUN_SHE))
			{
				D->properties ^= NOUN_HE | NOUN_SHE; // unknown sex
			}
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readNonWords(const char* file)
{
	//   RIP OUT THESE WORDS
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing nonwords file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the name
		if (*word == 0) continue;

		WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (!D) continue;
		D->properties = 0;
	}
	FClose(in);
	FreeBuffer();
}

static void readNonNames(const char* file)
{
	//   these words should NOT be treated as proper names
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing nonnames file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the name
		if (*word == 0) continue;

		WORDP D = FindWord(word, 0, UPPERCASE_LOOKUP);
		if (!D) continue;
		RemoveProperty(D, NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_HE | NOUN_SHE | NOUN);
		AddInternalFlag(D, DELETED_MARK);
	}
	FClose(in);
	FreeBuffer();
}

static void RemoveSynSet(MEANING T)
{
	WORDP D = Meaning2Word(T);
	if (!D->meanings) return;	// no synsets anyway
	unsigned int index = Meaning2Index(T);
	unsigned int restriction = T & TYPE_RESTRICTION;
	if (index == 0)// remove all meanings
	{
		unsigned int limit = GetMeaningCount(D);
		for (unsigned int i = limit; i >= 1; --i)
		{
			T = MakeMeaning(D, i);
			MEANING master = GetMeaning(D, i);
			if (!master) continue; //   already removed
			if (restriction && (master & restriction) == 0) continue;	 // not part of our limit
			FACT* F = FindFact(T, Mis, master); // from normal word to synset head
			if (F) KillFact(F);
			RemoveMeaning(T, master);
		}
		return;
	}

	MEANING master = GetMeaning(D, index);
	if (!master) return; //   already removed
	WORDP S = Meaning2Word(master);
	if (S == D) // self ptr
	{
		if (GetMeaningCount(D)) Log(USERLOG, "Cannot remove, we are master meaning %s\r\n", D->word);
		else AddInternalFlag(D, DELETED_MARK);
		return;
	}
	FACT* F = FindFact(T, Mis, master); // from normal word to synset head
	if (F) KillFact(F);
	RemoveMeaning(T, master);
}

static void readNonPos(const char* file)
{
	//   these words should NOT be treated as pos listed
	char word[MAX_WORD_SIZE];
	char pos[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing nonpos file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the word
		if (*word == 0) continue;
		WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (!D) continue;

		while (ALWAYS)
		{
			ptr = ReadCompiledWord(ptr, pos);
			if (!*pos) break;

			//   remove wholesale
			uint64 flags = 0;
			uint64 sysflags = 0;
			// wholesale removal
			if (!stricmp(pos, "NOUN")) flags = NOUN | NOUN_BITS | NOUN_PROPERTIES;
			else if (!stricmp(pos, "PREPOSITION")) flags = PREPOSITION;
			else if (!stricmp(pos, "VERB"))
			{
				flags = VERB | VERB_BITS | VERB_PROPERTIES;
				sysflags = VERB_SYSTEM_PROPERTIES;
			}
			else if (!stricmp(pos, "ADJECTIVE"))
			{
				flags = ADJECTIVE | ADJECTIVE_BITS | MORE_FORM | MOST_FORM;
				sysflags = ADJECTIVE_POSTPOSITIVE;
			}
			else if (!stricmp(pos, "ADVERB"))
			{
				flags = ADVERB | MORE_FORM | MOST_FORM;
				sysflags = 0;
			}
			else // remove individual flag
			{
				flags = FindPropertyValueByName(pos);
				if (!flags)
				{
					sysflags = FindSystemValueByName(pos);
					if (!sysflags) ReportBug("No such flag %s to remove from %s\r\n", pos, D->word);
				}
				RemoveProperty(D, flags);
				RemoveSystemFlag(D, sysflags);
				continue; // take away no meanings
			}

			RemoveProperty(D, flags);
			RemoveSystemFlag(D, sysflags);

			//   remove meanings
			for (int j = 1; j <= GetMeaningCount(D); ++j)
			{
				MEANING T = GetMeaning(D, j); //   the synset master
				if (!T) continue;
				WORDP G = Meaning2Word(T);
				if (!(G->internalBits & WORDNET_ID)) continue;	//   never remove self ptrs
				if (!(G->properties & flags)) continue;	//   not correct type
				RemoveSynSet(MakeMeaning(D, j)); //   remove this meaning
				j = 0; //   retry them again
			}
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readOnomatopoeia(const char* file)
{//   sounds made by x 
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing Onomatopoeia file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();

	currentFileLine = 0;
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the name
		if (*word == 0) continue;
		StoreWord(word, NOUN | NOUN_SINGULAR);
		while (ALWAYS)
		{
			ptr = ReadWord(ptr, word);
			if (!*word) break;
			StoreWord(word, NOUN | NOUN_SINGULAR);
			//   bug assign relationship
		}
	}
	FClose(in);
	FreeBuffer();
}

static void SetHelper(const char* word, uint64 flags)
{
	WORDP D = StoreWord(word, 0);
	RemoveProperty(D, VERB_BITS | VERB | AUX_VERB | VERB_INFINITIVE);	// we will set all that are required
	AddProperty(D, flags);
	AddSystemFlag(D, KINDERGARTEN);
	if (flags & VERB) AddTypedMeaning(D, VERB); // need a meaning entry to preserve VERB attributes
}

static void readIrregularVerbs(const char* file)
{ //   format is present past past  optional-present-participle
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing irregularverbs file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	WORDP be = StoreWord("be");
	WORDP n = StoreWord("am");
	AddCircularEntry(be, TENSEFIELD, n);
	n = StoreWord("are");
	AddCircularEntry(be, TENSEFIELD, n);
	n = StoreWord("is");
	AddCircularEntry(be, TENSEFIELD, n);
	n = StoreWord("being");
	AddCircularEntry(be, TENSEFIELD, n);
	n = StoreWord("been");
	AddCircularEntry(be, TENSEFIELD, n);
	n = StoreWord("was");
	AddCircularEntry(be, TENSEFIELD, n);
	n = StoreWord("were");
	AddCircularEntry(be, TENSEFIELD, n);

	while (ReadALine(input_string, in) >= 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;

		//   present tense
		ptr = ReadWord(ptr, word);
		if (*word == 0) continue;
		WORDP mainverb = StoreWord(word, VERB);
		RemoveProperty(mainverb, VERB_BITS | VERB_INFINITIVE);
		AddProperty(mainverb, VERB_INFINITIVE);
		if (stricmp(word, "be")) AddProperty(mainverb, VERB_PRESENT); // "be" is not a present tense
		ptr = SkipWhitespace(ptr);
		WORDP D;
		while (*ptr == '/')  //   alternate presents
		{
			ptr += 2;
			ptr = ReadWord(ptr, word);
			D = StoreWord(word, VERB | VERB_PRESENT_3PS);
			AddCircularEntry(mainverb, TENSEFIELD, D);
			ptr = SkipWhitespace(ptr);
		}

		//   past tense
		ptr = ReadWord(ptr, word);
		D = StoreWord(word, VERB);
		AddProperty(D, VERB_PAST);
		AddCircularEntry(mainverb, TENSEFIELD, D);
		ptr = SkipWhitespace(ptr);
		while (*ptr == '/')  //   alternate simple past
		{
			ptr += 2;
			ptr = ReadWord(ptr, word);    //   the past tense
			D = StoreWord(word, VERB | VERB_PAST);
			AddCircularEntry(mainverb, TENSEFIELD, D);
			ptr = SkipWhitespace(ptr);
		}

		//   past participle
		ptr = ReadWord(ptr, word);
		if (!*word)
		{
			Log(USERLOG, "Missing required past participle %s\r\n", mainverb->word);
			continue;
		}
		D = StoreWord(word, VERB | VERB_PAST_PARTICIPLE | ADJECTIVE | ADJECTIVE_PARTICIPLE);
		if (!stricmp(word, "had")) RemoveProperty(D, ADJECTIVE | ADJECTIVE_PARTICIPLE); // had will never be but "the children *having food" might be
		AddCircularEntry(mainverb, TENSEFIELD, D);
		ptr = SkipWhitespace(ptr);
		while (*ptr == '/')  //   alternate full past
		{
			ptr += 2;
			ptr = ReadWord(ptr, word);    //   the past tense
			D = StoreWord(word, VERB | VERB_PAST_PARTICIPLE);
			AddCircularEntry(mainverb, TENSEFIELD, D);
			ptr = SkipWhitespace(ptr);
		}

		//   present participle (optional)
		ptr = ReadWord(ptr, word);
		if (!*word) continue;      //   optional present participle not given
		D = StoreWord(word, VERB | VERB_PRESENT_PARTICIPLE | ADJECTIVE | ADJECTIVE_PARTICIPLE);
		AddCircularEntry(mainverb, TENSEFIELD, D);
	}
	FClose(in);
	FreeBuffer();
}

static void readIrregularNouns(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing irregularnouns file %s\r\n", file);
		return;
	}
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the singular
		if (*word == 0) continue;
		WORDP singular = StoreWord(word, NOUN);
		AddProperty(singular, (singular->internalBits & UPPERCASE_HASH) ? NOUN_PROPER_SINGULAR : NOUN_SINGULAR);
		//   GetPlural will map a plural to its singular form and a singular to one of its plurals (there may be multiple plural forms)

		while (*ptr) //   read all plurals
		{
			ptr = ReadWord(ptr, word);    //   a  plural
			if (!*word) break;
			WORDP D = StoreWord(word, NOUN);
			AddProperty(D, (D->internalBits & UPPERCASE_HASH) ? NOUN_PROPER_PLURAL : NOUN_PLURAL);
			if (stricmp(D->word, singular->word))    //   not same word
			{
				WORDP plural = FindCircularEntry(singular, NOUN_PLURAL | NOUN_PROPER_PLURAL);
				if (plural) { ; }
					// Log(USERLOG, "plural->S collision %s\r\n", plural->word);
				else  AddCircularEntry(singular, PLURALFIELD, D);
			}
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readIrregularAdverbs(const char* file)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing irregularadverbs file %s\r\n", file);
		return;
	}

	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the basic
		if (*word == 0) continue;
		WORDP basic = StoreWord(word, ADVERB | ADVERB);
		RemoveProperty(basic, MORE_FORM | MOST_FORM);

		ptr = ReadWord(ptr, word);    //   a  more
		WORDP more = StoreWord(word, ADVERB | ADVERB);
		RemoveProperty(more, MORE_FORM | MOST_FORM);
		AddProperty(more, MORE_FORM);

		ptr = ReadWord(ptr, word);    //   a  most
		WORDP most = StoreWord(word, ADVERB | ADVERB);
		RemoveProperty(most, MORE_FORM | MOST_FORM);
		AddProperty(most, MOST_FORM);

		AddCircularEntry(basic, COMPARISONFIELD, more);
		AddCircularEntry(basic, COMPARISONFIELD, most);
	}
	FClose(in);
	FreeBuffer();
}

static void readWordByAge(const char* file, uint64 gradelearned)
{
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing wordbyage file %s\r\n", file);
		return;
	}

	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the name
		if (*word == 0) continue;
		size_t len = strlen(word);
		WORDP D = FindWord(word, len, PRIMARY_CASE_ALLOWED);
		if (!D)
		{
			char* itemx = FindCanonical(word, len);
			if (itemx) D = FindWord(itemx, 0, PRIMARY_CASE_ALLOWED);
		}
		if (D && !(D->properties & PART_OF_SPEECH))
			D = NULL;

		//   we know it now
		if (!D) D = StoreWord(word, 0);
		if (D->systemFlags & AGE_LEARNED) continue;  //   already know this word younger
		if (IsNumber(D->word) != NOT_A_NUMBER && IsPlaceNumber(D->word))
		{
			AddProperty(D, ADJECTIVE | ADJECTIVE_NUMBER);
			AddSystemFlag(D, ORDINAL);
			AddParseBits(D, QUANTITY_NOUN);
		}
		AddSystemFlag(D, gradelearned);

		char* itemx = FindCanonical(D->word, 2,true);
		WORDP X = (itemx) ? FindWord(itemx, 0, PRIMARY_CASE_ALLOWED) : NULL;
		if (!X) continue;

		// propogate to all related forms
		WORDP E = GetComparison(X); //  adj/adv
		while (E && E != X)
		{
			AddSystemFlag(E, gradelearned);
			WORDP O = E;
			E = GetComparison(E);
			if (O == E)
				break;
		}
		E = GetTense(X); // verb
		while (E && E != X)
		{
			AddSystemFlag(E, gradelearned);
			WORDP O = E;
			E = GetTense(E);
			if (O == E)
				break;
		}
		E = GetPlural(X); // noun
		while (E && E != X)
		{
			AddSystemFlag(E, gradelearned);
			WORDP O = E;
			E = GetPlural(E);
			if (O == E)
				break;
		}
	}
	FClose(in);
	FreeBuffer();
}

static void ReadSexed(const char* file, uint64 properties)
{ //   format is canonical word then all words that map to it (except WORDNET_ID uses it for other things)
	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing sexed file %s\r\n", file);
		return;
	}

	while (ReadALine(readBuffer, in) >= 0)
	{
		if (readBuffer[0] == '#' || readBuffer[0] == 0) continue;
		ReadWord(readBuffer, word);
		if (*word == 0 || *word == '#') continue;
		if (properties & NOUN_THEY && IsUpperCase(*word)) continue;	//   dont do names of groups
		WORDP D = StoreWord(word, properties);
		AddSystemFlag(D, KINDERGARTEN);
	}
	FClose(in);
}

static char* EatQuotedWord(char* ptr, char* spot) //   contiguous mass of characters
{
	char* start = spot;
	if (!ptr || *ptr == 0)
	{
		*spot = 0;
		return 0;
	}
	while (*ptr && (*ptr == ' ' || *ptr == '\t')) ++ptr;  //   skip preceeding blanks
	while (*ptr != ' ' && *ptr != '\t' && *ptr != '\r' && *ptr != '\n' && *ptr != 0)
	{
		if ((*ptr == ')' || *ptr == ']') && spot != start) break;		//   end of a list
		char c = *ptr++;
		*spot++ = c;  //   put char into word
		if (c == '(' || c == '[') break;
		if (c == '"') //   grab whole string 
		{
			char* hold = ptr;
			--ptr;
			while (*++ptr != '"' && *ptr)
			{
				*spot++ = *ptr; //   grab all in quotes
			}
			if (*ptr != '"') //   single double quote??
			{
				ptr = hold;
			}
			else
			{
				*spot++ = *ptr; //   put in tail ending quote (or eol)
				if (*ptr) ++ptr;//   move off the ""
			}
		}
	}
	*spot = 0;
	if (start[0] == '"' && start[1] == '"' && start[2] == 0) *start = 0;   //   NULL STRING
	return ptr;
}

void ReadTriple(const char* file)
{
	FILE* in = FopenReadNormal(file);
	if (!in)
	{
		Log(USERLOG, "** Missing triple file %s\r\n", file);
		return;
	}

	while (fget_input_string(false, false, readBuffer, in))
	{
		if (*readBuffer == 0 || *readBuffer == '#') continue;
		char* ptr = readBuffer;

		char word[MAX_WORD_SIZE];
		MEANING subject = 0;
		MEANING verb = 0;
		MEANING object = 0;
		char subjectname[MAX_WORD_SIZE];
		ptr = EatQuotedWord(ptr, subjectname);
		if (!*subjectname) continue; //   unless it is the null fact
		strcpy(subjectname, JoinWords(BurstWord(subjectname), false)); //   a composite word
		subject = ReadMeaning(subjectname);

		ptr = ReadWord(ptr, word);
		MakeLowerCase(word); //   all verbs are lower case
		verb = MakeMeaning(StoreWord(word));

		char objectname[MAX_WORD_SIZE];
		ptr = EatQuotedWord(ptr, objectname);
		strcpy(objectname, JoinWords(BurstWord(objectname), false)); //   a composite word
		object = ReadMeaning(objectname);

		CreateFact(subject, verb, object);
	}
	FClose(in);
}

static FACT* UpLink(MEANING M)
{
	M &= -1 ^ (TYPE_RESTRICTION | SYNSET_MARKER);
	WORDP D = Meaning2Word(M);
	FACT* F = GetSubjectHead(D);
	while (F)
	{
		if (F->subject == M && F->verb == Mis) return F;
		F = GetSubjectNext(F);
	}
	return NULL;
}

static void PostFixNumber(WORDP D, uint64 junk)
{
	if (D->properties & NOUN_NUMBER)  RemoveProperty(D, NOUN_SINGULAR);
}

static void VerifyDictEntry(WORDP D, uint64 junk)
{
	if (*D->word == '_' && D->internalBits & RENAMED) return; // rename

	uint64 pos = D->properties;
	if (!IsAlphaUTF8(*D->word) && !(D->internalBits & UTF8)) //   abnormal word (maybe wordnet synset header)   but could be 60_minutes or simple punctuation or whatever
	{
		size_t len = strlen(D->word);
		if (!IsAlphaUTF8(D->word[len - 1])) return;	 // has no alpha in it
	}

	// if some meanings have been removed, adjust flags
	uint64 flags = 0;
	uint64 removeFlag = 0;
	bool removed = false;
	// NOTE-- if SOMETHING gets removed, then meanings added without a master can also be deleted again 
	int n = GetMeaningCount(D);
	for (int i = 1; i <= n; ++i)
	{
		MEANING master = GetMeaning(D, i);
		if (!master) removed = true;
		else flags |= master & TYPE_RESTRICTION; // what are LEGAL values
	}
	if (removed)
	{
		if (!(flags & VERB) && !GetTense(D)) RemoveProperty(D, VERB);
		if (!(flags & ADJECTIVE)) RemoveProperty(D, ADJECTIVE);
		if (!(flags & NOUN) && !GetPlural(D) && !(pos & (NOUN_FIRSTNAME | NOUN_HUMAN | NOUN_HE | NOUN_SHE | NOUN_THEY | NOUN_TITLE_OF_ADDRESS))) RemoveProperty(D, NOUN); // if we removed meanings, remove the noun bit IF not a name
		if (!(flags & ADVERB)) RemoveProperty(D, ADVERB);
	}
	if (D->properties & NOUN && !(D->properties & NOUN_BITS)) RemoveProperty(D, NOUN);
	if (D->properties & VERB && !(D->properties & VERB_BITS)) RemoveProperty(D, VERB);
	if (D->properties & ADJECTIVE && !(D->properties & ADJECTIVE_BITS)) RemoveProperty(D, ADJECTIVE_BITS);

	if (!(D->properties & NOUN)) RemoveProperty(D, NOUN_PROPERTIES);
	if (!(D->properties & VERB))
	{
		RemoveSystemFlag(D, VERB_CONJUGATE1 | VERB_CONJUGATE2 | VERB_CONJUGATE3 | VERB_DIRECTOBJECT | VERB_INDIRECTOBJECT | VERB_TAKES_ADJECTIVE);
		RemoveProperty(D, VERB_INFINITIVE | VERB_PRESENT | VERB_PRESENT_3PS | VERB_PRESENT_PARTICIPLE);
		if (D->properties & VERB_PAST) AddProperty(D, VERB);	// felt is past tense, unrelated to verb to felt
	}
	if (!(D->properties & ADJECTIVE)) RemoveProperty(D, ADJECTIVE_BITS);

	int validMeanings = 0;
	for (int j = 1; j <= GetMeaningCount(D); ++j) if (GetMeaning(D, j)) ++validMeanings;
	if (validMeanings == 0 && !GetSubstitute(D) && !GetComparison(D) && !GetTense(D) && !GetPlural(D) && !D->properties && !D->systemFlags && !GETMULTIWORDHEADER(D)) // has no accepted meanings or utility
	{
		AddInternalFlag(D, DELETED_MARK);
		return;
	}

	if (D->properties & ADJECTIVE)
	{   //   Irregular ones form a loop, regular ones that JUST happen to have entries in the other forms are merely marked for their form.

		if ((GetComparison(D)) == D) Log(USERLOG, "Self looping adjective %s\r\n", D->word);
		if (GetComparison(D))
		{
			if (D->properties & MORE_FORM) //   check other two
			{
				WORDP compare = GetComparison(D);
				if (!compare) Log(USERLOG, "Adjective %s lacks MOST form\r\n", D->word);
				else
				{
					if (!GetAdjectiveBase(D->word, false)) 
						Log(USERLOG, "Adjective %s lacks base form\r\n", D->word);
				}
			}
			else if (D->properties & MOST_FORM) //   its most, check other 2
			{
				WORDP compare = GetComparison(D); //   on to base form
				if (!compare) 
					Log(USERLOG, "Adjective %s lacks base\r\n", D->word);
				else //   has something, is it a base
				{
					if (!GetAdjectiveBase(D->word, false)) Log(USERLOG, "Adjective %s lacks base form\r\n", D->word);
				}
			}
		}
	}

	if (D->properties & NOUN)
	{
		// dead noun with no other value (like "am" has verb value)
		if (GetMeaningCount(D) == 0 && (IsLowerCase(D->word[0]) && IsLowerCase(D->word[1]))) // a lower case standin from the index for an upper case word
		{
			WORDP G = FindWord(D->word, 0, UPPERCASE_LOOKUP);
			if (G && !(G->internalBits & DELETED_MARK) && !(D->properties & (VERB | ADVERB | ADJECTIVE))) // has an upper case equivalent and no OTHER meanings
			{
				if (!GetPlural(D) && !GetMeaningCount(D))
				{
					Log(USERLOG, "Lowercase noun has upper value: %s %s \r\n", D->word, G->word);
					//RemoveProperty(D, ((NOUN_BITS - NOUN_GERUND) | NOUN)); // remove noun ness since is not plural or useful -- asl and katar
				}
				if (!(D->properties & PART_OF_SPEECH) && !GetPlural(D) && !GETMULTIWORDHEADER(D)) // no other utility.
				{
					AddInternalFlag(D, DELETED_MARK);
					return;
				}
			}
		}

		// ALLOW words like headquarters to be both singular AND plural, meaning it can use both kinds of verbs
		//if (D->properties & NOUN_PLURAL && D->properties & NOUN_SINGULAR && IsLowerCase(D->word[0])) // for now dont allow nouns to be multiply singular and plural, since we cant disambiguate well
		//{
		//RemoveProperty(D,NOUN_SINGULAR);	// no conflict please, "media" makes a mess in pos tagging
		//char* word = GetSingularNoun(D->word,false,true);
		//if ( !word || !stricmp(word,D->word)) // cant find a singular--- so have no plural instead
		//{
		//	RemoveProperty(D,NOUN_PLURAL);
		//	AddProperty(D,NOUN_SINGULAR);
		//}
		//}

		char* baseword = GetSingularNoun(D->word, false, true);
		if (!baseword)
		{
			Log(USERLOG, "----Missing base singular: %s\r\n", D->word);
			return;
		}
	}

	if (D->properties & VERB)
	{
		// if it can be either separated or not, remove both flags as it has no requirement on it
		if (D->systemFlags & INSEPARABLE_PHRASAL_VERB && D->systemFlags & SEPARABLE_PHRASAL_VERB)
		{
			RemoveSystemFlag(D, INSEPARABLE_PHRASAL_VERB | SEPARABLE_PHRASAL_VERB);
		}

		// any multiword verb must designate its syllable of conjugation - fault it to 1st place 
		if (strchr(D->word, '_') && !(D->systemFlags & (VERB_CONJUGATE1 | VERB_CONJUGATE2 | VERB_CONJUGATE3))) AddSystemFlag(D, VERB_CONJUGATE1);

		if (D->properties & AUX_VERB) return;   //   DOES not conjugate

												//   EVERYTHING marked as a verb must have complete conjugation -- note proper name CAN BE A VERB, like Charleston the dance
		char* baseword = GetInfinitive(D->word, true);
		if (!baseword)
		{
			if (*D->word != '`') 
				Log(USERLOG, "*****Missing verb infinitive: %s\r\n", D->word); // ignore #define value
			return;
		}
		char* past = GetPastTense(baseword); // find has past tense found but found has infinitive find OR found
		char* inf = (past) ? GetInfinitive(past, true) : NULL;
		if (!inf )
		{
			Log(USERLOG, "*****Missing verb past link to infinitive: %s %s\r\n", D->word, (inf) ? inf : (char*)" ");
			return;
		}
		char* pastparticiple = GetPastParticiple(baseword);
		inf = (pastparticiple) ? GetInfinitive(pastparticiple, true) : NULL;
		if (!inf)
		{
			Log(USERLOG, "*****Missing verb pastparticiple link to infinitive: %s\r\n", D->word);
			return;
		}
		char* presentparticiple = GetPresentParticiple(baseword);
		inf = (presentparticiple) ? GetInfinitive(presentparticiple, true) : NULL;
		if (!inf)
		{
			Log(USERLOG, "Missing verb presentparticiple link to infinitive: %s\r\n", D->word);
			return;
		}
	}

	if (pos && !(D->properties & pos) && !(D->internalBits & CONDITIONAL_IDIOM)) AddInternalFlag(D, DELETED_MARK); // has nothing left to work with
}

static void ReadDeadSynset(const char* name)
{//   given entry, ISOLATE entry into own synset or kill it entirely
	FILE* in = FopenReadNormal(name);
	if (!in)
	{
		Log(USERLOG, "** Missing deadsynset file %s\r\n", name);
		return;
	}

	while (ReadALine(readBuffer, in) >= 0)
	{
		if (*readBuffer == 0 || *readBuffer == '#') continue;
		char main[MAX_WORD_SIZE];
		ReadCompiledWord(readBuffer, main);
		char* tilde = strchr(main, '~');
		char* range = (tilde) ? strchr(tilde, '-') : NULL;
		if (range)
		{
			unsigned int start = atoi(tilde + 1);
			unsigned int end = atoi(range + 1);
			while (start <= end)
			{
				tilde[1] = 0; // truncate to word~
				sprintf(tilde + 1, "%u", start++);
				RemoveSynSet(ReadMeaning(main, true));
			}
		}
		else RemoveSynSet(ReadMeaning(main, true));
	}
	FClose(in);
}

static void ReadDeadFacts(const char* name) //   kill these WordNet facts
{
	FILE* in = FopenReadNormal(name);
	if (!in)
	{
		Log(USERLOG, "** Missing deadfacts file %s\r\n", name);
		return;
	}

	while (ReadALine(readBuffer, in) >= 0)
	{
		if (*readBuffer == 0 || *readBuffer == '#') continue;
		//   facts are direct copies of other facts, are ReadCompiledWord safe
		char* ptr = readBuffer;
		FACT* F = ReadFact(ptr, 0);
		if (F) KillFact(F);
		else Log(USERLOG, "ReadDeadFacts fact not found: %s\r\n", readBuffer);
	}
	FClose(in);
}

static void DeleteAllWords(WORDP D, uint64 junk)
{
	if (GetSubstitute(D)) return;	// dont delete these
	if (D->properties & (QWORD | PUNCTUATION_BITS | PARTICLE | INTERJECTION | THERE_EXISTENTIAL | TO_INFINITIVE)) return;
	if (D->properties & (DETERMINER_BITS | POSSESSIVE_BITS | CONJUNCTION | AUX_VERB | PRONOUN_BITS)) return;

	if (D->word[0] == '~' && !(D->internalBits & (BUILD1 | BUILD0))) return;//  system concepts alone

	if (D->internalBits & UPPERCASE_HASH) { ; } // no proper names
	else if (!(D->systemFlags & (KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6))) // delete advanced words unless its irregular and base is vital
	{
		char* noun = GetSingularNoun(D->word, false, true);
		WORDP G = FindWord(noun, 0,LOWERCASE_LOOKUP);
		if (G && G->systemFlags & (KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6)) return; // base is known
		char* verb = GetInfinitive(D->word, true);
		G = FindWord(verb, 0,LOWERCASE_LOOKUP);
		if (G && G->systemFlags & (KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6)) return; // base is known
		char* adjective = GetAdjectiveBase(D->word, true);
		G = FindWord(adjective, 0,LOWERCASE_LOOKUP);
		if (G && G->systemFlags & (KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6)) return; // base is known
		char* adverb = GetAdverbBase(D->word, true);
		G = FindWord(adverb,0, LOWERCASE_LOOKUP);
		if (G && G->systemFlags & (KINDERGARTEN | GRADE1_2 | GRADE3_4 | GRADE5_6)) return; // base is known
	}
	else if (D->properties & NORMAL_WORD) return;
	else if (D->properties & NOUN_FIRSTNAME) return;		// keep first names for recognition 

	AddInternalFlag(D, DELETED_MARK);
}

static void CheckShortFacts()
{
	char* name = "TOPIC/BUILD0/facts0.txt";
	StartFile("TOPIC/BUILD0/facts0.txt");
	FILE* in = FopenReadWritten(name); //  fact files
	if (!in)
	{
		Log(USERLOG, "** Missing facts0 file %s\r\n", name);
		return;
	}

	char word[MAX_WORD_SIZE];
	while (ReadALine(readBuffer, in) >= 0)
	{
		ReadCompiledWord(readBuffer, word);
		if (*word == 0 || *word == '#') continue; //   empty or comment
		char* ptr = readBuffer;
		//   fact may start indented.  Will start with (or be 0 for a null fact
		ptr = ReadCompiledWord(ptr, word);  //   BUG reconsider spacing need this?
		if (word[0] == '0') continue;
		unsigned int flags = 0;

		//   look at subject-- nested fact?
		char subjectname[MAX_WORD_SIZE];
		char* s = subjectname;
		ptr = ReadField(ptr, s, 's', flags);
		char verbname[MAX_WORD_SIZE];
		char* v = verbname;
		ptr = ReadField(ptr, v, 'v', flags);
		char objectname[MAX_WORD_SIZE];
		char* o = objectname;
		ptr = ReadField(ptr, o, 'o', flags);
		if (!ptr) continue;

		//   handle the flags on the fact
		uint64 properties = 0;
		if (!*ptr || *ptr == ')'); //   end of fact
		else if (*ptr == '0') ptr = ReadHex(ptr, properties);
		else
		{
			char flag[MAX_WORD_SIZE];
			while (*ptr && *ptr != ')')
			{
				ptr = ReadCompiledWord(ptr, flag);
				properties |= FindPropertyValueByName(flag);
				ptr = SkipWhitespace(ptr);
			}
		}
		flags |= (unsigned int)properties;
		while (*ptr == ' ') ++ptr;
		WORDP D;
		if (!(properties & FACTSUBJECT))
		{
			MEANING subject = ReadMeaning(subjectname, true, true);
			D = Meaning2Word(subject);
			AddSystemFlag(D, MARKED_WORD);
			if (D->internalBits & DELETED_MARK && !(D->internalBits & TOPIC))  RemoveInternalFlag(D, DELETED_MARK);
		}
		if (!(properties & FACTVERB))
		{
			MEANING verb = ReadMeaning(verbname, true, true);
			D = Meaning2Word(verb);
			AddSystemFlag(D, MARKED_WORD);
			if (D->internalBits & DELETED_MARK && !(D->internalBits & TOPIC)) RemoveInternalFlag(D, DELETED_MARK);
		}
		if (!(properties & FACTOBJECT))
		{
			MEANING object = ReadMeaning(objectname, true, true);
			D = Meaning2Word(object);
			AddSystemFlag(D, MARKED_WORD);
			if (D->internalBits & DELETED_MARK && !(D->internalBits & TOPIC)) RemoveInternalFlag(D, DELETED_MARK);
		}
	}
	FClose(in);
}

static void CheckShortDictionary(const char* name, bool pattern)
{
	FILE* in = FopenReadNormal(name);
	if (!in)
	{
		Log(USERLOG, "** Missing shortdictionary file %s\r\n", name);
		return;
	}

	while (ReadALine(readBuffer, in) >= 0)
	{
		if (*readBuffer == 0 || *readBuffer == '#') continue;
		//   facts are direct copies of other facts, are ReadCompiledWord safe
		char word[MAX_WORD_SIZE];
		char* ptr = ReadWord(readBuffer, word);
		uint64 flags = 0;
		char* underscore = strrchr(word + 1, '_'); // find last one in case "Musee_de_la_Citie"
		bool isUpper = false;

		if (underscore && IsUpperCase(underscore[1]) && (IsUpperCase(word[0]) || IsDigit(word[0]))) isUpper = true;
		else if (underscore) continue;

		// DONT bring in phrases, they arent needed
		if (isUpper || (IsUpperCase(word[0]) && word[1])) { flags |= NOUN | NOUN_PROPER_SINGULAR; } // infer it but not for letters 

																									// revive verb base?
		char* infinitive = GetInfinitive(word, true);
		if (infinitive)
		{
			WORDP X = FindWord(infinitive, 0, PRIMARY_CASE_ALLOWED);
			if (X && X->properties & VERB)
			{
				AddSystemFlag(X, MARKED_WORD);
				if (X->internalBits & DELETED_MARK) RemoveInternalFlag(X, DELETED_MARK);
				continue;	// take the base instead
			}
		}

		// revive noun base?
		char* basenoun = GetSingularNoun(word, true, true);
		if (basenoun)
		{
			WORDP X = FindWord(basenoun, 0, PRIMARY_CASE_ALLOWED);
			if (X && X->properties & NOUN)
			{
				AddSystemFlag(X, MARKED_WORD);
				if (X->internalBits & DELETED_MARK) RemoveInternalFlag(X, DELETED_MARK);
				continue;	// take the base instead
			}
		}

		WORDP D = StoreWord(word, flags); // force it to stay kept
		AddSystemFlag(D, MARKED_WORD);
		if (D->internalBits & DELETED_MARK) RemoveInternalFlag(D, DELETED_MARK);

		unsigned int meaningcount = 0;
		unsigned int glosscount = 0;

		ReadDictionaryFlags(D, ptr, &meaningcount, &glosscount);
		uint64 bits = D->properties;
		if (bits == NOUN && !(bits & NOUN_PROPER_SINGULAR)) AddProperty(D, NOUN_SINGULAR);
		if (bits & VERB && !(bits & VERB_BITS)) AddProperty(D, VERB_INFINITIVE);

		// keep multiword headers
		unsigned int n = BurstWord(D->word);
		// force multiword to keep its idiom header
		if (n != 1 && *word != '~' && *word != '^')  // dont touch concept or function names
		{
			char* wordx = JoinWords(1);
			WORDP G = FindWord(wordx, 0, PRIMARY_CASE_ALLOWED);
			if (!G) ReportBug("idiom header missing");
			else
			{
				AddSystemFlag(G, MARKED_WORD);
				if (G->internalBits & DELETED_MARK)  RemoveInternalFlag(G, DELETED_MARK);
			}
		}
	}
	FClose(in);
}

static bool IsAbstract(FACT * F1, unsigned int depth)
{
	WORDP D = Meaning2Word(F1->object);
	if (!stricmp(D->word, "abstract_entity")) return true;
	FACT* F = GetSubjectHead(D);
	while (F)
	{
		if (F->subject == F1->object && F->verb == Mis)
		{
			return IsAbstract(F, depth + 1);
		}
		F = GetSubjectNext(F);
	}
	return false;
}

static void MarkAbstract(WORDP D, uint64 junk)
{
	if (D->internalBits & (DELETED_MARK | WORDNET_ID)) return;
	if (!(D->properties & NOUN)) return;

	// while we are here, change 1920s to plural
	if (IsDigit(D->word[0]) && D->properties & NOUN_SINGULAR)
	{
		size_t len = strlen(D->word);
		if (D->word[len - 1] == 's')
		{
			AddProperty(D, NOUN_PLURAL);
			RemoveProperty(D, NOUN_SINGULAR);
		}
	}

	FACT* F = GetSubjectHead(D);
	while (F)
	{
		if (F->verb == Mis && IsAbstract(F, 0))
		{
			AddProperty(D, NOUN_ABSTRACT);
			return;
		}
		F = GetSubjectNext(F);
	}
}

static void CleanDead(WORDP D, uint64 junk) // insure all synonym circulars point only to living members and alter up chains
{
	if (D->internalBits & (DELETED_MARK | WORDNET_ID)) return;
	int n = GetMeaningCount(D);
	if (n == 0) D->w.glosses = NULL;
	//   verify meanings lists point to undeleted items in their synset.
	for (int i = 1; i <= n; ++i)
	{
		MEANING M = GetMeaning(D, i);
		int index = Meaning2Index(M);
		WORDP X = Meaning2Word(M);
		if (GetMeaningCount(X) < index)
			ReportBug("Bad meaning ref in cleandead of %s.%d which points to %s.%d which only has %d\r\n", D->word, i, X->word, index, GetMeaningCount(X));
		bool seenMarker = (M & SYNSET_MARKER) ? true : false; // we are the synset head and we are not dead
		MEANING synhead = 0;
		if (X->internalBits & DELETED_MARK) // need to patch around this....
		{
			while (X->internalBits & DELETED_MARK) // may end up back on us...
			{
				M = GetMeaning(X, index);
				if (M & SYNSET_MARKER)
				{
					if (seenMarker) ReportBug("duplicate synset head?");
						seenMarker = true; // deleted word was synset header, we must take on that role.
					synhead = MakeMeaning(X, index) | (M & (BASIC_POS));
				}
				index = Meaning2Index(M);
				X = Meaning2Word(M);
			}
			if (X == D) M = MakeMeaning(X, index) | (M & TYPE_RESTRICTION) | SYNSET_MARKER; // we completed the loop, everyone else is dead, make us generic
			if (seenMarker) M |= SYNSET_MARKER; // replace synset header with us or found unit since synset head is dead
			GetMeanings(D)[i] = M; // use this marker unchanged or changed
		}

		// seenmarker will be true if we are the synset head that results. our uppath will need to change perhaps. synhead will have been the original head

		if (GetMeanings(D)[i] & SYNSET_MARKER) // we are the synset head, adjust our links upward to be dead or not
		{
			MEANING Mx = MakeMeaning(D, i); // correct ptr to use for uplink has no flags on it
			MEANING basex = Mx;
			if (synhead) Mx = synhead;	// the actual facts base for synset was dead
			FACT* F = NULL;
			while (ALWAYS)
			{
				F = UpLink(Mx);  // goes up one level from ORIGINAL synset master - all words are already alive or dead marked
				if (!F) break; // no uplink
				if (Meaning2Word(F->object)->internalBits & DELETED_MARK) // fact SHOULD be dead as object certainly is
				{
					F->flags |= FACTDEAD;
				}
				else break; // valid uplink endpoint found (though link itself may or may not be dead already)
				Mx = F->object;	// transfer thru dead link to find valid
			}
			if (F && F->subject != basex)
			{
				F->flags |= FACTDEAD; // last fact from a dead trail
				CreateFact(basex, Mis, F->object); // indirect uplinks all dead, make a direct uplink
			}
		}
	}

	// any word having NOTHING of value must die now
	if (n == 0 && !D->properties && !GetTense(D) && !GetPlural(D) && !GetComparison(D) && D->word[0] != '~' && D->word[0] != '^' && !(D->internalBits & CONDITIONAL_IDIOM) && !(D->systemFlags & (VERB_SYSTEM_PROPERTIES | VERB_PHRASAL_PROPERTIES)))
	{
		uint64 flags = D->systemFlags;
		flags &= -1 ^ (TIMEWORD | GRADE5_6 | GRADE3_4 | GRADE1_2 | KINDERGARTEN);
		if (!flags) AddInternalFlag(D, DELETED_MARK);
	}
}

static void ReviveWords(WORDP D, uint64 junk) // now that dead meaning references have been stripped, check canonicals on the living words and revive some
{
	if (D->systemFlags & MARKED_WORD) // we WANT to revive this word if nedded
	{
		D->systemFlags ^= MARKED_WORD;
		RemoveInternalFlag(D, DELETED_MARK);
	}
	if (D->internalBits & DELETED_MARK) return;
	int limit = 1000;
	if (GetTense(D))
	{
		WORDP E = GetTense(D);
		while (E && E != D && --limit > 0)
		{
			if (E->internalBits & DELETED_MARK)
			{
				RemoveInternalFlag(E, DELETED_MARK);	// undelete, we need it
				didSomething = true;
			}
			E = GetTense(E);
		}
	}
	if (GetPlural(D))
	{
		WORDP E = GetPlural(D);
		while (E && E != D && --limit > 0)
		{
			if (E->internalBits & DELETED_MARK)
			{
				RemoveInternalFlag(E, DELETED_MARK);	// undelete, we need it
				didSomething = true;
			}
			E = GetPlural(E);
		}
	}
	if (GetComparison(D))
	{
		WORDP E = GetComparison(D);
		while (E && E != D && --limit > 0)
		{
			if (E->internalBits & DELETED_MARK)
			{
				RemoveInternalFlag(E, DELETED_MARK);	// undelete, we need it
				didSomething = true;
			}
			E = GetComparison(E);
		}
	}
}

static void AddShortDict(const char* name)
{
	FILE* in = FopenReadWritten(name); //  fact files
	if (!in)
	{
		Log(USERLOG, "** Missing shortdict fact file %s\r\n", name);
		return;
	}
	StartFile(name);
	char word[MAX_WORD_SIZE];
	WORDP basex = dictionaryFree;
	while (ReadALine(readBuffer, in) >= 0)
	{
		ReadCompiledWord(readBuffer, word);
		if (*word == 0 || *word == '#') continue; //   empty or comment
		if (*word == '+') //   dictionary entry
		{
			char* at = ReadCompiledWord(readBuffer + 2, word); //   start at valid token past space
			WORDP D = StoreWord(word, 0);
			if (D->internalBits & DELETED_MARK) RemoveInternalFlag(D, DELETED_MARK);
			ReadDictionaryFlags(D, at);
			AddSystemFlag(D, MARKED_WORD | BUILD0);
		}
	}
	FClose(in);

	if (dictionaryFree == basex)
	{
		Log(USERLOG, "***** NO dict from topics? %s\r\n", name);
	}
}

static void MoveSetsToBase(WORDP D, uint64 junk)
{
	if (D->word[0] == '~' ) RemoveInternalFlag(D, BUILD0 | BUILD1);	// move concept to base (allow user redefine but build in)
}

void ReadForeign()
{
	// first get the mapping between foreign pos and english.
	char name[MAX_WORD_SIZE];
	sprintf(name, "DICT/%s_tags.txt", current_language);
	if (!ReadForeignPosTags(name)) return; //failed 

	char filename[100];
	sprintf(filename, "treetagger/%s_rawwords.txt", current_language);
	FILE* in = FopenReadOnly(filename);
	if (!in) return;

	char pos[100];
	*pos = '_'; // foreign pos tags stored with _ to not conflict with other words
	while (ReadALine(readBuffer, in) >= 0)
	{
		char word[MAX_WORD_SIZE];
		char* ptr = ReadTokenMass(readBuffer, word);
		if (!*word) continue;
		ptr = strrchr(readBuffer, '`');
		if (!ptr) continue; // no lemma
		uint64 flag = 0;
		++ptr;
		while (*ptr)
		{
			ptr = ReadCompiledWord(ptr, pos + 1); // foreign pos
			if (!*pos) break;
			WORDP P = FindWord(pos);
			if (P) flag |= P->properties; // english pos equivalent
		}
		StoreWord(word, flag);
	}
	FClose(in);
}

static void ReadEnglish(int minid)
{
	//   proper names
	readNames("RAWDICT/LINKPARSER/entities.locations.sing", NOUN | NOUN_PROPER_SINGULAR, 0);
	// readNames("RAWDICT/lastnames.txt", NOUN | NOUN_PROPER_SINGULAR | NOUN_HUMAN, 0);
	// before nouns so we can autogender proper names
	if (minid <= 0) // full dictionary only
	{
		readFirstNames("RAWDICT/firstnames.txt");
		readNames("RAWDICT/LINKPARSER/entities.given-female.sing", NOUN | NOUN_SHE | NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_PROPER_SINGULAR, 0);
		readNames("RAWDICT/LINKPARSER/entities.given-male.sing", NOUN | NOUN_HE | NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_PROPER_SINGULAR, 0);
		readNames("RAWDICT/LINKPARSER/entities.given-bisex.sing", NOUN | NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_PROPER_SINGULAR | NOUN_HE | NOUN_SHE, 0);
	}
	readReducedFirstNames("RAWDICT/reducedfirstnames.txt");


	// ADD WORD LEAST LIKELY TO BE USED FIRST TO IMPROVE DICTIONARY BUCKET LOOKUP
	if (minid <= 0) 	readSupplementalWord("RAWDICT/foreign.txt", FOREIGN_WORD, 0); // only on a full dictionary

																				  //   noun data
	(*printer)("reading noun data\r\n");
	readData("RAWDICT/WORDNET/data.noun"); //   
	readIndex("RAWDICT/WORDNET/index.noun", 0); //   
	readIrregularNouns("RAWDICT/noun_irregular.txt");
	readSupplementalWord("RAWDICT/noun_more.txt", NOUN, 0);
	readFix("RAWDICT/WORDNET/noun.exc", NOUN | NOUN_SINGULAR);
	readNounNoDeterminer("RAWDICT/nounnondeterminer.txt");
	readMonths("RAWDICT/months.txt");

	//   special word properties
	readOnomatopoeia("RAWDICT/onomatopoeia.txt");
	readSupplementalWord("RAWDICT/interjectionlist.txt", INTERJECTION, 0);
	readSupplementalWord("RAWDICT/otherlist.txt", 0, 0);
	if (minid <= 0) readHomophones("RAWDICT/homophones.txt");// full dictionary

															// gender information
	ReadSexed("RAWDICT/he.txt", NOUN_HE | NOUN_HUMAN);
	ReadSexed("RAWDICT/she.txt", NOUN_SHE | NOUN_HUMAN);
	ReadSexed("RAWDICT/they.txt", NOUN_THEY | NOUN_HUMAN);

	readWordKind("RAWDICT/timewords.txt", TIMEWORD);

	//   adjectives
	(*printer)("reading adjectives\r\n");
	readData("RAWDICT/WORDNET/data.adj"); //   
	readIndex("RAWDICT/WORDNET/index.adj", NOUN | VERB); //   
	readSupplementalWord("RAWDICT/adj_more.txt", ADJECTIVE | ADJECTIVE_NORMAL, 0);
	readWordKind("RAWDICT/numbers.txt", 0);
	readFix("RAWDICT/WORDNET/adj.exc", ADJECTIVE | ADJECTIVE_NORMAL); //   more and most forms
	WalkDictionary(PostFixNumber, 0);
	ReadSystemFlaggedWords("RAWDICT/adjectivepostpositive.txt", ADJECTIVE_POSTPOSITIVE);
	readParticipleVerbs("RAWDICT/participleadjectives.txt");
	AdjNotPredicate("RAWDICT/adj_notpredicate.txt");

	//   adverbs (before verbs to detect separable phrasal adverbs)
	(*printer)("reading adverbs\r\n");
	readData("RAWDICT/WORDNET/data.adv"); //   
	readIndex("RAWDICT/WORDNET/index.adv", NOUN | VERB | ADJECTIVE); //   
	readSupplementalWord("RAWDICT/adv_more.txt", ADVERB, 0);
	readIrregularAdverbs("RAWDICT/adverbs_irregular.txt");
	readFix("RAWDICT/WORDNET/adv.exc", ADVERB | ADVERB);
	AddSystemFlag(FindWord("more"), KINDERGARTEN);
	AddSystemFlag(FindWord("most"), KINDERGARTEN);
	AddSystemFlag(FindWord("less"), KINDERGARTEN);
	AddSystemFlag(FindWord("least"), KINDERGARTEN);
	WalkDictionary(AdjustAdjectiveAdverb);
	AddQuestionAdverbs();

	// conjunctions
	readConjunction("RAWDICT/conjunction.txt");

	// read prepositions before verbs so verbs can detect separable phrasal verbs (tailing preposition)
	readPrepositions("RAWDICT/prepositions.txt", true); //   now add onology data
	ReadSystemFlaggedWords("RAWDICT/omittabletimepreps.txt", OMITTABLE_TIME_PREPOSITION);

	//   verb data
	(*printer)("reading verb data\r\n");
	readData("RAWDICT/WORDNET/data.verb"); //   make this read data last, MUST BE after prepositions, so ONTOLOGY doesnt grow, we put verb forms at end of it
	readIndex("RAWDICT/WORDNET/index.verb", NOUN); //   
												   // readFix("RAWDICT/WORDNET/verb.exc",VERB);   file DOES NOT INSURE its inflected forms are correctly ordered or all there
	readIrregularVerbs("RAWDICT/verb_irregular.txt"); //   read first, to block some wordnet "like wonned won"
													  // reading wordnets irregualr verbs is useless-- no ordering of data to know what they are supplying
	RemoveProperty(FindWord("will"), VERB_INFINITIVE | VERB_PRESENT);//   we consider these FUTURE
	readSupplementalWord("RAWDICT/verb_more.txt", VERB, 0); // normal verbs
	readSupplementalWord("RAWDICT/verb_infinitiveobject.txt", 0, VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE); // verbs taking infinitive after object
	readSupplementalWord("RAWDICT/verb_infinitivedirect.txt", 0, VERB | VERB_INFINITIVE | VERB_TAKES_VERBINFINITIVE); // verbs taking infinitive directly
	readSupplementalWord("RAWDICT/verb_causal_to.txt", 0, VERB | VERB_INFINITIVE | VERB_TAKES_INDIRECT_THEN_TOINFINITIVE); // verbs taking to infinitive after indirect
	ReadPhrasalVerb("RAWDICT/verb_phrasal.txt"); // full data
	readSpellingExceptions("RAWDICT/verb_spell.txt");

	//   add in the Auxillary verbs and helper verbs AFTER verbs with normal definitions to preserve meaning list ordering
	// for BE, HAVE, DO, dont list aux tense. Can only have the generic form (lest apply get confused)
	SetHelper("have", VERB_INFINITIVE | VERB_PRESENT | AUX_HAVE | VERB);
	SetHelper("having", VERB_PRESENT_PARTICIPLE | AUX_HAVE | VERB | NOUN | NOUN_GERUND);
	SetHelper("has", VERB_PRESENT_3PS | AUX_HAVE | VERB | SINGULAR_PERSON);
	SetHelper("had", VERB_PAST | VERB_PAST_PARTICIPLE | AUX_HAVE | VERB);

	SetHelper("be", VERB | VERB_INFINITIVE | VERB | AUX_BE);
	SetHelper("am", VERB_PRESENT | AUX_BE | VERB | SINGULAR_PERSON);
	SetHelper("are", VERB_PRESENT | AUX_BE | VERB);
	SetHelper("is", AUX_BE | VERB_PRESENT_3PS | VERB | SINGULAR_PERSON);
	SetHelper("being", AUX_BE | NOUN_GERUND | NOUN | VERB | VERB_PRESENT_PARTICIPLE);
	SetHelper("was", AUX_BE | VERB_PAST | VERB | SINGULAR_PERSON);
	SetHelper("were", AUX_BE | VERB_PAST | VERB);
	SetHelper("been", AUX_BE | VERB | VERB_PAST_PARTICIPLE);

	SetHelper("going_to", AUX_VERB_FUTURE);	// handled specially by substititions so wont exist UNLESS followed by potential infinitive
	SetHelper("can", VERB_PRESENT | VERB_INFINITIVE | AUX_VERB_PRESENT | VERB); // modal
	SetHelper("could", AUX_VERB_FUTURE);  // modal
	SetHelper("dare", VERB_PRESENT | AUX_VERB_PRESENT | VERB | VERB_INFINITIVE);
	SetHelper("dares", VERB_PRESENT_3PS | AUX_VERB_PRESENT | VERB);
	SetHelper("daring", VERB_PRESENT_PARTICIPLE | VERB);
	SetHelper("dared", VERB_PAST | AUX_VERB_PAST | VERB);
	SetHelper("do", VERB_PRESENT | VERB_INFINITIVE | VERB | AUX_DO);
	SetHelper("does", VERB_PRESENT_3PS | VERB | AUX_DO | SINGULAR_PERSON);
	SetHelper("doing", VERB_PRESENT_PARTICIPLE | VERB);
	SetHelper("did", VERB_PAST | VERB | AUX_DO);
	SetHelper("done", VERB_PAST_PARTICIPLE | VERB);
	SetHelper("get", VERB_PRESENT | VERB | VERB_INFINITIVE); // get is treated as aux BE
	SetHelper("got", VERB_PAST_PARTICIPLE | VERB_PAST | VERB);	/// colloquial for passive past  // the window got closed
	SetHelper("gotten", VERB_PAST_PARTICIPLE | VERB);
	SetHelper("gets", VERB_PRESENT_3PS | VERB);
	SetHelper("getting", VERB_PRESENT_PARTICIPLE | VERB | NOUN_GERUND | NOUN | VERB | VERB_PRESENT_PARTICIPLE);
	SetHelper("had_better", AUX_VERB_PRESENT);
	SetHelper("may", AUX_VERB_FUTURE); // modal
	SetHelper("might", AUX_VERB_FUTURE); // modal
	SetHelper("must", AUX_VERB_FUTURE);     // modal
	SetHelper("need", VERB_PRESENT | AUX_VERB_PRESENT | VERB | VERB_INFINITIVE);
	SetHelper("ought", AUX_VERB_FUTURE); // modal
	SetHelper("ought_to", AUX_VERB_FUTURE);  // modal
	SetHelper("shall", AUX_VERB_FUTURE); // modal
	SetHelper("should", AUX_VERB_FUTURE); // modal
	SetHelper("use_to", AUX_VERB_PAST | VERB | VERB_PRESENT | VERB_INFINITIVE);
	SetHelper("used_to", AUX_VERB_PAST | VERB | VERB_PAST | VERB_PAST_PARTICIPLE); //BUT cannot handle "are you used to it?"
	SetHelper("will", AUX_VERB_FUTURE); // modal
	SetHelper("would", AUX_VERB_FUTURE); // modal
										 // need "going" to be in dictionary so wont spell check into "going to"
	StoreWord("going", VERB | VERB_PRESENT_PARTICIPLE);
	StoreWord("''", QUOTE);
	StoreWord(".", PUNCTUATION); // closest we have 
	ReadSystemFlaggedWords("RAWDICT/existentialbe.txt", PRESENTATION_VERB);
	ReadSystemFlaggedWords("RAWDICT/adverbs_extent.txt", EXTENT_ADVERB);
	ReadSystemFlaggedWords("RAWDICT/verb_linking.txt", VERB_TAKES_ADJECTIVE); // copular verb (adjective as object)
	ReadParseWords("RAWDICT/parsemarks.txt");


	// pseudonouns (pronouns + there existential)
	if (!readPronouns("RAWDICT/pronouns.txt"))
	{
		Log(USERLOG, "Dictionary raw data missing\r\n");
		myexit(0);
	}
	readSupplementalWord("RAWDICT/takepostpositiveadjective.txt", 0, 0);
	StoreFlaggedWord("there", THERE_EXISTENTIAL, KINDERGARTEN);

	StoreFlaggedWord("to", TO_INFINITIVE, KINDERGARTEN);

	// DETERMINERS/PREDETERMINERS
	ReadDeterminers("RAWDICT/determiners.txt");

	StoreWord("\"", QUOTE);

	readSupplementalWord("RAWDICT/conditional_idioms.txt", 0, 0);

	// Add age data
	readWordByAge("RAWDICT/preschool_kindergarten.txt", KINDERGARTEN);
	readWordByAge("RAWDICT/grade1_2.txt", GRADE1_2);
	readWordByAge("RAWDICT/grade3_4.txt", GRADE3_4);
	readWordByAge("RAWDICT/grade5_6.txt", GRADE5_6);
	readWordByAge("RAWDICT/jrhigh.txt", GRADE5_6);
	readWordByAge("RAWDICT/basicEnglish.txt", GRADE5_6);
	readCommonness();
	ReadBNCPosData();
	readNonPos("RAWDICT/nonpos.txt");
	if (minid >= 0) ReadDeadSynset("RAWDICT/deadsynset.txt"); // if we dont want wordnet all (which is the usual)
	readNonNames("RAWDICT/nonnames.txt"); //   override our dictionary on some words it thinks of as names that are too common
	readNonWords("RAWDICT/nonwords.txt");
}

static void AddFlagDownHierarchy(MEANING T, unsigned int depth, uint64 flag)
{
	if (depth >= 1000 || !T) return;
	WORDP D = Meaning2Word(T);
	if (D->inferMark == inferMark) return;
	AddSystemFlag(D, flag);

	D->inferMark = inferMark;
	unsigned int index = Meaning2Index(T);
	unsigned int size = GetMeaningCount(D);
	if (!size) size = 1;

	for (unsigned int k = 1; k <= size; ++k) //   for each meaning of this dictionary word
	{
		if (index)
		{
			if (k != index) continue; //   not all, just one specific meaning
			T = GetMaster(GetMeaning(D, k));
		}
		else
		{
			if (GetMeaningCount(D)) T = GetMaster(GetMeaning(D, k));
			else T = MakeMeaning(D); //   did not request a specific meaning, look at each in turn
		}
		int index1 = Meaning2Index(T);
		if (index1) // do master loop marking
		{
			MEANING M = GetMeaning(D, k);
			int limit = 200;
			while (M)
			{
				WORDP X = Meaning2Word(M);
				if (X == D) break;
				AddSystemFlag(X, flag);
				M = GetMeaning(X, Meaning2Index(M));
				if (!--limit)
				{
					printf("master loop failed %s %s\r\n",D->word,X->word);
					break;
				}
			}
		}

		//   for the current T meaning
		int l = 0;
		while (++l) //   find the children of the meaning of T
		{
			MEANING child = FindChild(T, l); //   only headers sought
			if (!child) break;

			//   child and all syn names of child
			AddFlagDownHierarchy(child, depth, flag);
		} //   end of children for this value
		--depth;
	}
}

static void readPluralNouns(const char* file) // have no singular form
{
	FILE* in = FopenReadWritten(file);
	if (!in)
	{
		Log(USERLOG, "** Missing pluralnouns file %s\r\n", file);
		return;
	}
	StartFile(file);
	char word[MAX_WORD_SIZE];
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the noun
		if (*word == 0) break;

		WORDP D = FindWord(word);
		if (D && D->properties & NOUN_SINGULAR)
		{
			D->properties |= NOUN_PLURAL;
			D->properties ^= NOUN_SINGULAR;
		}
	}
	FClose(in);
	FreeBuffer();
}

static void readMassNouns(const char* file)
{
	FILE* in = FopenReadWritten(file);
	if (!in)
	{
		Log(USERLOG, "** Missing massnouns file %s\r\n", file);
		return;
	}
	StartFile(file);
	char word[MAX_WORD_SIZE];
	char* input_string = AllocateBuffer();
	while (fget_input_string(false, false, input_string, in) != 0)
	{
		if (input_string[0] == '#' || input_string[0] == 0) continue;
		char* ptr = input_string;
		ptr = ReadWord(ptr, word);    //   the noun
		if (*word == 0) break;

		// wordnet hiearchy
		char* tilde = strchr(word, '~');
		if (tilde && IsDigit(tilde[1]))
		{
			// mark mass nouns
			NextInferMark();
			MEANING M = GetMaster(ReadMeaning(word, false));
			AddFlagDownHierarchy(M, 0, NOUN_NODETERMINER);
			continue;
		}

		WORDP D = StoreWord(word);
		size_t len = strlen(word);
		if (word[len - 1] == 'g' && word[len - 2] == 'n' && word[len - 3] == 'i') continue;	// skip gerunds
		if (!(D->properties & (NOUN_SINGULAR | NOUN_PROPER_SINGULAR))) AddProperty(D, NOUN | NOUN_SINGULAR); // defining
		AddSystemFlag(D, NOUN_NODETERMINER);
	}
	FClose(in);
	FreeBuffer();
}

void LoadRawDictionary(int minid) // 6 == foreign
{
	sprintf(logFilename, "USERS/dictionary_log.txt");
	FILE* out = FopenUTF8Write(logFilename);
	if (minid && minid != 6) fprintf(out, "Mini: %d\r\n", minid);
	FClose(out);
	ClearWordMaps();

	AcquireDefines("src/dictionarySystem.h"); // need to process raw dictionary data but should NOT be final dict, since this is a dynamic file
	InitFactWords();
	ExtendDictionary();
	Dqword = StoreWord("~qwords");

	if (minid != 6) ReadEnglish(minid);
	// By now, the full NORMAL dictionary has been created.

	//   fix the dictionary
	*currentFilename = 0;
	if (!stricmp(current_language, "english"))
	{
		WalkDictionary(VerifyDictEntry); // removes flags based on deadsysnet removing entries
		WalkDictionary(PurgeDictionary); // adds meanings to the synset head stubs from its members
		WalkDictionary(FixSynsets);
		WalkDictionary(ExtractSynsets); // switch wordnet id nodes back to real words
		readMassNouns("RAWDICT/nounmass.txt");   // ENGLISH ONLY
		ReadDeadFacts("RAWDICT/deadfacts.txt"); // kill off some problematic dictionary facts
		readPluralNouns("RAWDICT/nounplural.txt");
		// mark mass nouns
		WalkDictionary(MarkAbstract);

	}
	// dictionary is full and normal right now

	// remove EVERYTHING by default (except aux verbs, prepositions, conjunctions, determiners, pronouns, currency, Punctuation)
	if (minid > 0 && minid != 6) WalkDictionary(DeleteAllWords, 0);

	if (minid >= 2 && minid != 6) // merge in layer0 stuff-- dont do layer 1, which should remain dynamic, but keep keywords we know about
	{
		// then revive specifics 
		AddShortDict("TOPIC/BUILD0/dict0.txt"); // add dictionary entries defined by level 0 dictionary entries
		if (minid == 3) AddShortDict("TOPIC/BUILD1/dict1.txt"); // add dictionary entries defined by level 1 dictionary entries

		FACT* F = lastFactUsed;
		InitKeywords("TOPIC/BUILD0/keywords0.txt", NULL, BUILD0, true); // BUILDING MINI DICTIONARY propogate entries into the dictionary with types as needed (just not concept names)- also creates facts we want to destroy
		if (minid == 3)	InitKeywords("TOPIC/BUILD1/keywords1.txt", NULL, BUILD1, true); // BUILDING MINI DICTIONARYpropogate entries into the dictionary with types as needed - also creates facts we want to destroy
		CheckShortFacts();
		while (lastFactUsed > F) FreeFact(lastFactUsed--);

		CheckShortDictionary("TOPIC/BUILD0/patternWords0.txt", true); // could live w/o pattern words, but extra amount is small
		if (minid == 3)	CheckShortDictionary("TOPIC/BUILD1/patternWords1.txt", true);

		WalkDictionary(MoveSetsToBase);
	}

	// these get added to dictionary regardless of mini status

	char aux[MAX_WORD_SIZE];
	sprintf(aux, "RAWDICT/auxdict.txt");
	if (minid != 6)
	{
		CheckShortDictionary(aux, false); // vocabulary specific to this app
		ReadAbbreviations("abbreviations.txt"); // needed for burst/tokenizing
		ReadTitles("RAWDICT/titles.txt"); //   forms of address like Mr. (before readData so things like potage_St._Germain are correctly handled - burst needs to know them...
		WalkDictionary(DefineShortCanonicals, 0); // all more and most forms as needed
		ReadWordFrequency("RAWDICT/500kwords.txt", 20000, minid > 0); // add most frequent words NOT already known or until dict is 20K
		didSomething = true;
		while (didSomething)
		{
			didSomething = false;
			WalkDictionary(ReviveWords);  // revive implied canonicals and all marked words to keep
		}
		WalkDictionary(CleanDead); // insure all synonym circulars point only to living members and alter up chains
		WalkDictionary(VerifyEntries); // prove good entries before writeout
	}
}

void SortAffect(const char* file)
{
	char* xwordlist[10000];
	unsigned int index = 0;
	FILE* out = fopen("newaffect.txt", "wb");
	char word[MAX_WORD_SIZE];
	FILE* in = fopen(file, "rb");
	char affectName[MAX_WORD_SIZE];
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* ptr = readBuffer;
		while (ptr && *ptr)
		{
			ptr = ReadCompiledWord(ptr, word);
			if (*word == 0 || *word == '#') break;
			if (*word == '~')
			{
				//   dump word list of old
				if (index) fprintf(out, "\r\n%s # %u\r\n", affectName, index);
				for (unsigned int i = 0; i < index; ++i)
				{
					fprintf(out, "%s ", xwordlist[i]);
					if (((i + 1) % 10) == 0) fprintf(out, "\r\n");
				}
				fprintf(out, "\r\n");
				index = 0;

				//   dump new header
				strcpy(affectName, word);
				continue;
			}

			//   now dump out accumulated words in order for affect kind
			unsigned int i;
			for (i = 0; i < index; ++i)
			{
				int val = stricmp(word, xwordlist[i]);
				if (val == 0) break;
				if (val < 0) //   insert here
				{
					for (unsigned int j = index; j > i; --j) xwordlist[j] = xwordlist[j - 1];
					++index;
					xwordlist[i] = AllocateHeap(word);
					break;
				}
			}
			if (i >= index) xwordlist[index++] = AllocateHeap(word);
		}
	}
	//   dump word list of remaining affect kind
	if (index) fprintf(out, "\r\n%s # %u\r\n", affectName, index);
	for (unsigned int i = 0; i < index; ++i)
	{
		fprintf(out, "%s ", xwordlist[i]);
		if (((i + 1) % 10) == 0) fprintf(out, "\r\n");
	}
	fprintf(out, "\r\n");
	FClose(in);
	FClose(out);
}

void SortSubstitions(const char* file)
{
	char* wordlist[30000];
	char* tolist[30000];
	char* linelist[30000];
	unsigned int index = 0;
	FILE* in = fopen(file, "rb");
	if (!in) return;
	FILE* out = fopen("new_substitutes.txt", "wb");
	char word[MAX_WORD_SIZE];

	while (ReadALine(readBuffer, in) >= 0)
	{
		if (readBuffer[0] == '#' || readBuffer[0] == 0)
		{
			fprintf(out, "%s\r\n", readBuffer);
			continue;
		}
		char* ptr = ReadCompiledWord(readBuffer, word);    //   to be replaced word
		if (*word == 0 || *word == '#') continue;

		unsigned int i;
		for (i = 0; i < index; ++i) //   find spot to insert
		{
			int val = stricmp(word, wordlist[i]);
			if (val == 0) break;
			if (val < 0) //   insert here
			{
				for (unsigned int j = index; j > i; --j)
				{
					wordlist[j] = wordlist[j - 1];
					linelist[j] = linelist[j - 1];
					tolist[j] = tolist[j - 1];
				}
				++index;
				wordlist[i] = AllocateHeap(word);
				linelist[i] = AllocateHeap(readBuffer);
				ReadCompiledWord(ptr, word); //   what to substitute
				tolist[i] = AllocateHeap(word);
				break;
			}
		}
		if (i >= index) //   insert at end
		{
			linelist[index] = AllocateHeap(readBuffer);
			wordlist[index] = AllocateHeap(word);
			ReadCompiledWord(ptr, word); //   what to substitute
			tolist[index++] = AllocateHeap(word);
		}
	}
	//   dump word list of old
	fprintf(out, "\r\n # %u\r\n", index);
	for (unsigned int i = 0; i < index; ++i) fprintf(out, "%s\r\n", linelist[i]);
	FClose(in);
	FClose(out);
}

static void WriteShortWords(WORDP D, uint64 junk)
{
	if (!(D->internalBits & DELETED_MARK))
	{
		FILE* out = (FILE*)junk;
		fprintf(out, "%s\r\n", D->word);
	}
}

static void MarkOtherForms(WORDP D)
{
	// propogate to all related forms
	int limit = 1000;
	WORDP E = GetComparison(D); //  adj/adv
	while (E && E != D && --limit > 0)
	{
		RemoveInternalFlag(E, DELETED_MARK);
		WORDP X = GetComparison(E);
		if (X == E) break;
		E = X;
	}
	E = GetTense(D); // verb
	while (E && E != D && --limit > 0)
	{
		RemoveInternalFlag(E, DELETED_MARK);
		WORDP X = GetTense(E);
		if (X == E) break;
		E = X;
	}
	E = GetPlural(D); // noun
	while (E && E != D && --limit > 0)
	{
		RemoveInternalFlag(E, DELETED_MARK);
		WORDP X = GetPlural(E);
		if (X == E) break;
		E = X;
	}
}

static void DefineShortCanonicals(WORDP D, uint64 junk)
{
	if (!(D->internalBits & DELETED_MARK)) // IF these, then these by implication
	{
		MarkOtherForms(D);
	}
}

static void ReadWordFrequency(const char* name, unsigned int count, bool until)
{
	if (count == 0) return;
	unsigned int used = (dictionaryFree - dictionaryBase - 1);
	if (until)
	{
		if (used >= count) return;
		count -= used;
	}

	char word[MAX_WORD_SIZE];
	FILE* in = FopenReadNormal(name);
	if (!in)
	{
		Log(USERLOG, "** Missing wordfrequency file %s\r\n", name);
		return;
	}
	currentFileLine = 0;
	while (ReadALine(readBuffer, in) >= 0)
	{
		char* ptr = ReadWord(readBuffer, word); //  get frequency
		ptr = ReadWord(ptr, word);	// the word
		WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (!D) continue;	// ignore it
		if (D->internalBits & UPPERCASE_HASH || !IsAlphaUTF8(D->word[0])) continue; // dont need these
		if ((D->properties & PART_OF_SPEECH) == 0) continue; // not a really useful word
		if (D->internalBits & DELETED_MARK) D->internalBits ^= DELETED_MARK; // allow the word
		MarkOtherForms(D);
		if (--count == 0) break;
	}
	FClose(in);
}

void BuildShortDictionaryBase()
{
	FILE* out = fopen("RAWDICT/basicwordlist.txt", "wb");
	WalkDictionary(WriteShortWords, (uint64)out);
	FClose(out);
}

void ReadBNC(const char* buffer)
{
	FILE* in;
	if (!*buffer) in = fopen("RAWDICT/bncsource.txt", "rb");
	else in = fopen("RAWDICT/bncfreq.txt", "rb");
	if (!in) return;
	FILE* out;
	if (!*buffer)  out = fopen("RAWDICT/bncfreq.txt", "wb");
	else out = fopen("RAWDICT/bncfreq1.txt", "wb");
	int frequency;
	char word[MAX_WORD_SIZE];
	char pos[MAX_WORD_SIZE];
	unsigned int noun = 0, verb = 0, adjective = 0, adverb = 0, preposition = 0, conjunction = 0, aux = 0;
	char prior[MAX_WORD_SIZE];
	*prior = 0;
	char priorBase[MAX_WORD_SIZE];
	*priorBase = 0;
	while (ReadALine(readBuffer, in) >= 0)
	{
		if (*readBuffer == '#' || !*readBuffer) continue;
		char* ptr = readBuffer;
		if (!*buffer) // BNC: frequency, word, pos, files it occurs in  OR  base  word pos freq
		{
			ptr = ReadInt(ptr, frequency);
			ptr = ReadCompiledWord(ptr, word);
		}
		else // word, base, type, frequency
		{
			ptr = ReadCompiledWord(ptr, word);
			ptr = ReadCompiledWord(ptr, priorBase);
			ptr = ReadCompiledWord(ptr, pos);
			ReadInt(ptr, frequency);
		}
		WORDP entry, canonical;
		uint64 sysflags = 0;
		uint64 cansysflags = 0;
		WORDP revise;
		GetPosData(0, word, revise, entry, canonical, sysflags, cansysflags, false);
		char* nounbase = 0;
		char* verbbase = 0;
		char* adjectivebase = 0;
		char* adverbbase = 0;

		if (!*buffer)
		{
			nounbase = GetSingularNoun(word, false, true);
			verbbase = GetInfinitive(word, true);
			adjectivebase = GetAdjectiveBase(word, true);
			adverbbase = GetAdverbBase(word, true);
			if (canonical && canonical->properties & PART_OF_SPEECH) { ; } // we know the word as a part of speech
			else continue; // word is meaningless to us
		}
		char baseword[MAX_WORD_SIZE];
		strcpy(baseword, word);
		if (*prior && stricmp(word, prior) && *buffer) // new word
		{
			// issue old word
			fprintf(out, " %s NOUN:%u VERB:%u ADJECTIVE:%u ADVERB:%u PREPOSTION:%u CONJUNCTION:%u AUX:%u\r\n", prior, noun, verb, adjective, adverb, preposition, conjunction, aux);
			*prior = 0;
			noun = 0, verb = 0, adjective = 0, adverb = 0, preposition = 0, conjunction = 0, aux = 0;
		}
		strcpy(prior, word);

		uint64 wordkind = 0;
		if (!*buffer) // BNC: frequency, word,      ==> pos, files it occurs in  OR  base  word pos freq
		{
			ReadCompiledWord(ptr, pos);
			if (!stricmp(pos, "AJ0") || !stricmp(pos, "AJC") || !stricmp(pos, "AJS")) wordkind = ADJECTIVE;
			if (!stricmp(pos, "AV0")) wordkind = ADVERB;
			if (!stricmp(pos, "CJC") || !stricmp(pos, "CJS") || !stricmp(pos, "CJT")) wordkind = CONJUNCTION_COORDINATE;
			if (!stricmp(pos, "NN1-NP0") || !stricmp(pos, "NN0") || !stricmp(pos, "NN1") || !stricmp(pos, "NN2") || !stricmp(pos, "NP0")) wordkind = NOUN;
			if (!stricmp(pos, "PRF") || !stricmp(pos, "PRP")) wordkind = PREPOSITION;
			if (!stricmp(pos, "VRB") || !stricmp(pos, "VBD") || !stricmp(pos, "VBG") || !stricmp(pos, "VBI") || !stricmp(pos, "VBN") || !stricmp(pos, "VBZ")) wordkind = VERB;
			if (!stricmp(pos, "VDB") || !stricmp(pos, "VDD") || !stricmp(pos, "VDG") || !stricmp(pos, "VDI") || !stricmp(pos, "VDZ")) wordkind = VERB;
			if (!stricmp(pos, "VVD-VVN") || !stricmp(pos, "VHB") || !stricmp(pos, "VHD") || !stricmp(pos, "VHG") || !stricmp(pos, "VHI") || !stricmp(pos, "VHN") || !stricmp(pos, "VHZ")) wordkind = VERB;
			if (!stricmp(pos, "VVB") || !stricmp(pos, "VVD") || !stricmp(pos, "VVG") || !stricmp(pos, "VVI") || !stricmp(pos, "VVN") || !stricmp(pos, "VVZ")) wordkind = VERB;
			if (!stricmp(pos, "VM0")) wordkind = AUX_VERB_FUTURE;
			if (!wordkind) continue; // dont know the type
			const char* typex = 0;
			if (wordkind == NOUN) typex = "noun";
			if (wordkind == VERB) typex = "verb";
			if (wordkind == ADJECTIVE) typex = "adjective";
			if (wordkind == ADVERB) typex = "adverb";
			if (wordkind == PREPOSITION) typex = "preposition";
			if (wordkind == CONJUNCTION_COORDINATE) typex = "conjunction";
			if (wordkind == AUX_VERB_FUTURE) typex = "aux";

			if (wordkind == NOUN && nounbase) strcpy(base, nounbase);
			if (wordkind == VERB && verbbase) strcpy(base, verbbase);
			if (wordkind == ADJECTIVE && adjectivebase) strcpy(base, adjectivebase);
			if (wordkind == ADVERB && adverbbase) strcpy(base, adverbbase);
			if (wordkind == AUX_VERB_FUTURE && verbbase) strcpy(base, verbbase);
			if (wordkind == NOUN && entry && entry->properties & (NOUN_INFINITIVE | NOUN_GERUND)) continue;	// ignore verbal noun use (its a verb to us)
			if (wordkind == ADJECTIVE && entry && entry->properties & ADJECTIVE_PARTICIPLE) continue;	 // ignore adjective use of verb thingy

			if (wordkind == NOUN && !nounbase) continue;	// WE dont think it can be this
			if (wordkind == VERB && !verbbase) continue;	// WE dont think it can be this
			if (wordkind == ADJECTIVE && !adjectivebase) continue;	// WE dont think it can be this
			if (wordkind == ADVERB && !adverbbase) continue;	// WE dont think it can be this

			fprintf(out, " %s %s  %s %d\r\n", word, base, typex, frequency);
			continue;
		}
		else
		{
			if (!stricmp(pos, "noun")) noun += frequency;
			if (!stricmp(pos, "verb")) verb += frequency;
			if (!stricmp(pos, "adjective")) adjective += frequency;
			if (!stricmp(pos, "adverb")) adverb += frequency;
			if (!stricmp(pos, "preposition")) preposition += frequency;
			if (!stricmp(pos, "conjunction")) conjunction += frequency;
			if (!stricmp(pos, "aux")) aux += frequency;
		}
	}
	FClose(in);
	FClose(out);
}

#ifdef COPYRIGHT

Per use of the WordNet dictionary data....

This softwareand database is being provided to you, the LICENSEE, by
2 Princeton University under the following license.By obtaining, using
3 and /or copying this software and database, you agree that you have
4 read, understood, and will comply with these termsand conditions.:
5
6 Permission to use, copy, modifyand distribute this software and
7 database and its documentation for any purposeand without fee or
8 royalty is hereby granted, provided that you agree to comply with
9 the following copyright notice and statements, including the disclaimer,
10 and that the same appear on ALL copies of the software, database and
11 documentation, including modifications that you make for internal
12 use or for distribution.
13
14 WordNet 3.0 Copyright 2006 by Princeton University.All rights reserved.
15
16 THIS SOFTWARE AND DATABASE IS PROVIDED "AS IS" AND PRINCETON
17 UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS OR
18 IMPLIED.BY WAY OF EXAMPLE, BUT NOT LIMITATION, PRINCETON
19 UNIVERSITY MAKES NO REPRESENTATIONS OR WARRANTIES OF MERCHANT -
20 ABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE
21 OF THE LICENSED SOFTWARE, DATABASE OR DOCUMENTATION WILL NOT
22 INFRINGE ANY THIRD PARTY PATENTS, COPYRIGHTS, TRADEMARKS OR
23 OTHER RIGHTS.
24
25 The name of Princeton University or Princeton may not be used in
26 advertising or publicity pertaining to distribution of the software
27 and /or database.Title to copyright in this software, database and
28 any associated documentation shall at all times remain with
29 Princeton University and LICENSEE agrees to preserve same.
#endif
