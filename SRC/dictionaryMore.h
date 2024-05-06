
#define MAX_SYNLOOP	60
#define MAX_MULTIDICT 7	
#define MAX_HASH_BUCKETS 215127 
#define UNIQUEENTRY '`'				// leads words that wont clash with other dictionary entries
#define MAX_HASH_DEPTH  80
#define GETNEXTNODE(D) (D->nextNode)		// top byte is the length of joined phrases of which this is header
#define GETMULTIWORDHEADER(D)  (D->headercount)
#define SETMULTIWORDHEADER(D,n) (  D->headercount = n )
#define WORDLENGTH(D) (D->length)
#define IS_NEW_WORD(x) (!x || (x >= dictionaryPreBuild[2] &&  !xbuildDictionary)) // created by user volley
bool SUPERCEDED(WORDP D);
#define ALL_OBJECTS ( MAINOBJECT | MAININDIRECTOBJECT | OBJECT2 | INDIRECTOBJECT2 )
#define SUBSTITUTE_SEPARATOR '|'
#define CHECKSTAMP 18  // binary version id  - affects dict*.bin, TOPIC (bin)
#define CHECKSTAMPRAW 3  // if compile output format changes source version id  - affects  TOPIC (*.txt)
#define GET_LANGUAGE_INDEX(D)  ((D->internalBits & LANGUAGE_BITS) >> LANGUAGE_SHIFT)
#define GET_FOREIGN_INDEX(D)  (((D->internalBits & LANGUAGE_BITS) >> LANGUAGE_SHIFT) - 1)
#define GET_LANGUAGE_INDEX_OR_UNIVERSAL(D)  ((D->internalBits & UNIVERSAL_WORD) ? 0 : ((D->internalBits & LANGUAGE_BITS) >> LANGUAGE_SHIFT))

// system internal bits on dictionary entries internalBits

// these values of word->internalBits are NOT stored in the dictionary text files but are generated while reading in data

// various sources of livedata substitutions on real words (will not be on topic,var,function,concept names)
// These are on tokencontrol, enabling substitutions
// the original word will have this flag | HAS_SUBSTITUTE
//	 	0x00000001 # DO_ESSENTIALS
#define NOTIME_TOPIC				0x00000001		// dont time this topic (topic names)
#define NOTIME_FN						NOTIME_TOPIC	// dont time this function (on functions only)

//		0x00000002 # DO_SUBSTITUTES
#define VAR_CHANGED				0x00000002		// $variable has changed value this volley 
#define NOTRACE_TOPIC			VAR_CHANGED		// dont trace this topic (topic names)
#define NOTRACE_FN					VAR_CHANGED		// dont trace this function (on functions only)
#define CONCEPT_DUPLICATES	VAR_CHANGED		// concept allows duplication

//		0x00000004 # DO_CONTRACTIONS
#define FROM_FILE						0x00000004	//  for scriptcompiler to tell stuff comes from FILE not DIRECTORY
#define MACRO_TIME					FROM_FILE	// turn on timing for this function (only used when live running)
#define NO_CONCEPT_DUPLICATES	FROM_FILE		// concept disallows duplication

//		0x00000008 # DO_INTERJECTIONS
#define HAS_EXCLUDE				0x00000008		// concept/topic has keywords to exclude
#define TABBED							HAS_EXCLUDE     // table macro is tabbed
#define BOTVAR							HAS_EXCLUDE		// variable is a bot var (not real user var)

//		0x00000010 # DO_BRITISH
#define FUNCTION_NAME				0x00000010 	//   name of a ^function  (has non-zero ->x.codeIndex if system, else is user but can be patternmacro,outputmacro, or plan) only applicable to ^ words
#define SETIGNORE	 FUNCTION_NAME

//		0x00000020 # DO_SPELLING
#define CONCEPT								0x00000020	// all ~words are concepts (and may be topics)
																					// This flag used during compilation to see if concept is being redefined

//		0x00000040 # DO_TEXTING
#define TOPIC									0x00000040	//  this is a ~xxxx topic name in the system - only applicable to ~ words

//		0x00000080 # DO_NOISE -- without HAS_SUBSTITUTE this is a script compiler flag to disable warnings on spelling
#define IS_OUTPUT_MACRO			0x00000080	// function is an output macro
#define SETUNDERSCORE	 IS_OUTPUT_MACRO

//		0x00000100 thru PRIVATE
#define IS_TABLE_MACRO				0x00000100	// function is a table macro - transient executable output function
#define IS_PLAN_MACRO			( IS_TABLE_MACRO | IS_OUTPUT_MACRO )	// function is a plan macro (specialized form of IS_OUTPUT_MACRO has a codeindex which is the topicindex)

#define IS_PATTERN_MACRO		0x00000200 
#define FUNCTION_BITS ( IS_PATTERN_MACRO | IS_OUTPUT_MACRO | IS_TABLE_MACRO | IS_PLAN_MACRO )

#define PREFER_THIS_UPPERCASE	0x00000400		// given choice of uppercases, retrieve this

#define UNIVERSAL_WORD	0x00000800 				// word merges multiple language data OR was created as universal language

#define UTF8										0x00001000		// word has utf8 non-ascii char in it (all normal words)
#define UPPERCASE_HASH			0x00002000		// word has upper case character in it
#define QUERY_KIND						0x00004000	// is a query item (from LIVEDATA or query:)
#define LABEL									QUERY_KIND		// transient scriptcompiler use
#define RENAMED							QUERY_KIND		// _alpha name renames _number or @name renames @n
#define OVERRIDE_CONCEPT        QUERY_KIND      // this concept name is overridden by ^testpattern
#define WORDNET_ID						0x00008000		// a wordnet synset header node (MASTER w gloss ) only used when building a dictionary -- or transient flag for unduplicate
#define MACRO_TRACE					WORDNET_ID		// turn on tracing for this function or variable (only used when live running)
#define BIT_CHANGED					WORDNET_ID	// changing properties, systemflags, etc during loading
#define CONDITIONAL_IDIOM		0x00010000	// word may or may not merge into an idiom during pos-tagging -  blocks automerge during tokenization --D->w data conflicts and overrules glosses
#define BEEN_HERE							0x00020000		// used in internal word searches that might recurse
#define FAKE_NOCONCEPTLIST	0x00040000		// used on concepts declared NOCONCEPTLIST
#define DELETED_MARK				0x00080000		// transient marker for  deleted words in dictionary build - they dont get written out - includes script table macros that are transient
#define BUILD0									0x00100000		// comes from build0 data (marker on functions, concepts, topics)
#define BUILD1									0x00200000		// comes from build1 data
#define DELAYED_RECURSIVE_DIRECT_MEMBER	 0x00400000  // concept will be built with all indirect members made direct
#define MULTIPLE_WORD				0x00800000		// has space or _ in it
#define CONSTANT_IS_NEGATIVE		0x01000000	
#define INTERNAL_MARK			0x02000000		// transient marker for Intersect coding and Country testing in :trim
#define IS_SUPERCEDED  0x04000000  // word was replaced by UNIVERSAL_WORD
#define VARIABLE_ARGS_TABLE	 0x08000000		// only for table macros and output macros 
#define UPPERCASE_MATCH			VARIABLE_ARGS_TABLE	// match on this concept should store canonical as upper case
#define DEFINES								0x80000000		// word is a define (has no language bits), starts with `, uses ->properties and ->infermark as back and forth links

#define LANGUAGE_BITS				0x70000000	 // which language is this word from (applies only to ordinary words, not variables, etc)
#define LANGUAGE_SHIFT			28 // matches FACT's use of LANGUAGE_SHIFT on opposite end
#define LANGUAGE_UNIVERSAL 0
#define LANGUAGE_1 0x10000000

#define GETWORDSLIMIT 20

#define FN_TRACE_BITS ( MACRO_TRACE | NOTRACE_FN )
#define FN_TIME_BITS ( MACRO_TIME | NOTIME_FN )

///   DEFINITION OF A MEANING 
unsigned int GETTYPERESTRICTION(MEANING x);
#define STDMEANING ( INDEX_BITS | MEANING_BASE | TYPE_RESTRICTION ) // no synset marker
#define SIMPLEMEANING ( INDEX_BITS | MEANING_BASE ) // simple meaning, no type

//   codes for BurstWord argument
#define SIMPLE 0
#define STDBURST 0		// normal burst behavior
#define POSSESSIVES 1
#define CONTRACTIONS 2
#define HYPHENS 4
#define COMPILEDBURST 8  // prepare argument as though it were output script		
#define NOBURST 16		// dont burst (like for a quoted text string)

#define FUNCTIONSTRING '^'

#define KINDS_OF_PHRASES ( CLAUSE | PHRASE | VERBAL | OMITTED_TIME_PREP | OMITTED_OF_PREP | QUOTATION_UTTERANCE )

// pos tagger ZONE roles for a comma zone
#define ZONE_SUBJECT			0x000001	// noun before any verb
#define ZONE_VERB				0x000002
#define ZONE_OBJECT			0x000004	// noun AFTER a verb
#define ZONE_CONJUNCT		0x000008	// coord or subord conjunction
#define ZONE_FULLVERB		0x000010	// has normal verb tense or has aux
#define ZONE_AUX					0x000020	// there is aux in the zone
#define ZONE_PCV					0x000040	// zone is entirely phrases, clauses, and verbals
#define ZONE_ADDRESS			0x000080	// zone is an addressing name start. "Bob, you are amazing."
#define ZONE_ABSOLUTE		0x000100	// absolute zone has subject and partial participle verb, used to describe noun in another zone
#define ZONE_AMBIGUOUS	0x000200	// type of zone is not yet known

//   values for FindWord lookup
#define PRIMARY_CASE_ALLOWED 1024
#define SECONDARY_CASE_ALLOWED 2048
#define STANDARD_LOOKUP (PRIMARY_CASE_ALLOWED |  SECONDARY_CASE_ALLOWED )
#define LOWERCASE_LOOKUP 4096
#define UPPERCASE_LOOKUP 8192

#define NO_EXTENDED_WRITE_FLAGS ( PATTERN_WORD | MARKED_WORD )

// system flags revealed via concepts
#define MARK_FLAGS (  TIMEWORD | ACTUAL_TIME | WEB_URL | LOCATIONWORD | PRONOUN_REFLEXIVE | NOUN_NODETERMINER | VERB_CONJUGATE3 | VERB_CONJUGATE2 | VERB_CONJUGATE1 | INSEPARABLE_PHRASAL_VERB  | MUST_BE_SEPARATE_PHRASAL_VERB  | SEPARABLE_PHRASAL_VERB	|  PHRASAL_VERB  ) // system bits we display as concepts
			
// postag composites 
#define PUNCTUATION_BITS	( COMMA | PAREN | PUNCTUATION | QUOTE | CURRENCY )

#define NORMAL_WORD		( PARTICLE | ESSENTIAL_FLAGS | FOREIGN_WORD | INTERJECTION | THERE_EXISTENTIAL | TO_INFINITIVE | QWORD | DETERMINER_BITS | POSSESSIVE_BITS | CONJUNCTION | AUX_VERB | PRONOUN_BITS )
#define PART_OF_SPEECH		( PUNCTUATION_BITS  | NORMAL_WORD   ) 

#define VERB_CONJUGATION_PROPERTIES ( VERB_CONJUGATE1 | VERB_CONJUGATE2 | VERB_CONJUGATE3 ) 
#define VERB_PHRASAL_PROPERTIES ( INSEPARABLE_PHRASAL_VERB | MUST_BE_SEPARATE_PHRASAL_VERB | SEPARABLE_PHRASAL_VERB | PHRASAL_VERB )
#define VERB_OBJECTS ( VERB_NOOBJECT | VERB_INDIRECTOBJECT | VERB_DIRECTOBJECT | VERB_TAKES_GERUND | VERB_TAKES_ADJECTIVE | VERB_TAKES_INDIRECT_THEN_TOINFINITIVE | VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE | VERB_TAKES_TOINFINITIVE | VERB_TAKES_VERBINFINITIVE )
#define VERB_SYSTEM_PROPERTIES ( PRESENTATION_VERB | COMMON_PARTICIPLE_VERB | VERB_CONJUGATION_PROPERTIES | VERB_PHRASAL_PROPERTIES | VERB_OBJECTS ) 

#define NOUN_PROPERTIES ( NOUN_HE | NOUN_THEY | NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_ABSTRACT | NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_SHE | NOUN_TITLE_OF_ADDRESS | NOUN_TITLE_OF_WORK  )
#define SIGNS_OF_NOUN_BITS ( NOUN_DESCRIPTION_BITS | PRONOUN_SUBJECT | PRONOUN_OBJECT )

#define NUMBER_BITS ( NOUN_NUMBER | ADJECTIVE_NUMBER )

#define VERB_PROPERTIES (  VERB_BITS )

#define TRACE_BITS (-1 ^ (TRACE_ECHO | TRACE_ON))

#define COMPARISONFIELD 1
#define TENSEFIELD 2
#define PLURALFIELD 3

#define Index2Word(n) (n ? ( dictionaryBase + (size_t)n) : NULL)
#define Word2Index(D) ((unsigned int) ((D) ? ((D)-dictionaryBase) : 0))
MEANING GetMeaning(WORDP D, int index);
#define GetMeaningsFromMeaning(T) (GetMeanings(Meaning2Word(T)))
#define Meaning2Index(x) ( (((unsigned int)x) & INDEX_BITS)  >> INDEX_OFFSET) //   which dict entry meaning
#define CommonLevel(x) ((int)((x & COMMONNESS) >> (uint64)(64-14)))
extern char current_language[150];
extern unsigned int max_language;
extern unsigned int max_fact_language;
extern unsigned int languageCount;
extern char language_list[400];
extern unsigned int language_bits;
unsigned char* GetWhereInSentence(WORDP D); // always skips the linking field at front
extern unsigned int* hashbuckets;
extern int worstDictAvail;
#define OOB_START '['
#define OOB_END ']'
void LockLevel();
void WriteDictionary(WORDP D, uint64 junk);
void WriteTextEntry(WORDP D, bool tmp=false);
void UnlockLayer(int layer);
void WriteDictDetailsBeforeLayer(int layer);
WORDP GetLanguageWord(WORDP word);
char* GetWord(char* word);
extern bool hasUpperCharacters, hasUTF8Characters, hasSeparatorCharacters;

WORDP GetPlural(WORDP D);
bool IsValidLanguage(WORDP D);
bool SetLanguage(char* arg1);
char* GetNextLanguage(char*& data);
void SetPlural(WORDP D,MEANING M);
WORDP GetComparison(WORDP D);
void SetComparison(WORDP D,MEANING M);
WORDP GetTense(WORDP D);
void SetTense(WORDP D,MEANING M);
WORDP GetCanonical(WORDP D, uint64 kind = 0);
WORDP RawCanonical(WORDP D);
void SetCanonical(WORDP D,MEANING M);
void ExportCurrentDictionary();
uint64 GetTriedMeaning(WORDP D);
void SetTriedMeaning(WORDP D,uint64 bits);
unsigned int LanguageRestrict(char* word);
void ReverseDictionaryChanges(HEAPREF start);
void ReverseUniversalDictionaryChanges(HEAPREF start);
MEANING* GetMeanings(WORDP D);
void SetTriedMeaningWithData(uint64 bits, unsigned int* data);
void ReadSubstitutes(const char* name,unsigned int build,const char* layer,unsigned int fileFlag,bool filegiven = false);
void Add2ConceptTopicList(HEAPREF list[256], WORDP D,int start,int end,bool unique);
void SuffixMeaning(MEANING T,char* at, bool withPos);
int UTFCharSize(char* utf);
bool IsUniversal(char* word, uint64 properties);
HEAPREF SetSubstitute(char* original, char* replacement, unsigned int build, unsigned int fileFlag, HEAPREF list);
// memory data
extern WORDP dictionaryBase;
extern uint64 maxDictEntries;
extern unsigned int userTopicStoreSize;
extern unsigned int userTableSize;
extern bool multidict;
extern HEAPREF ongoingDictChanges;  
extern HEAPREF ongoingUniversalDictChanges;
extern bool monitorDictChanges;
extern unsigned long maxHashBuckets;
extern bool setMaxHashBuckets;
extern WORDP dictionaryLocked;
extern bool fullDictionary;
extern unsigned int languageIndex;
extern uint64 verbFormat;
extern bool exportdictionary;
extern uint64 nounFormat;
extern uint64 adjectiveFormat;
extern uint64 adverbFormat;
extern MEANING posMeanings[64];
extern MEANING sysMeanings[64];
extern bool xbuildDictionary;
extern HEAPREF propertyRedefines;	// property changes on locked dictionary entries
extern HEAPREF flagsRedefines;		// systemflags changes on locked dictionary entries
WORDP Convert2Universal(WORDP D, uint64 properties, WORDP* oldwords, unsigned int oldwordcount);
extern FACT* factLocked;
extern char* stringLocked;

extern WORDP dictionaryPreBuild[NUMBER_OF_LAYERS+1];
extern char* heapPreBuild[NUMBER_OF_LAYERS+1];
extern WORDP dictionaryFree;
extern char dictionaryTimeStamp[50];
extern bool primaryLookupSucceeded;

// internal references to defined words
extern WORDP Dplacenumber;
extern WORDP Dpropername;
extern MEANING Mphrase;
extern MEANING MabsolutePhrase;
extern MEANING MtimePhrase;
extern WORDP Dclause;
extern WORDP Dverbal;
extern WORDP Dmalename,Dfemalename,Dhumanname;
extern WORDP Dtime;
extern WORDP Dunknown;
extern WORDP DunknownWord;
extern WORDP Dpronoun;
extern WORDP Dadjective;
extern WORDP Dauxverb;
extern WORDP Dchild,Dadult;
extern WORDP Dtopic;
extern MEANING Mmoney;
extern MEANING Mchatoutput;
extern MEANING Mburst;
extern MEANING Mpending;
extern MEANING Mkeywordtopics;
extern MEANING Mconceptlist;
extern MEANING Mintersect;
extern MEANING MgambitTopics;
extern MEANING MadjectiveNoun;
extern MEANING Mnumber;
extern bool dictionaryBitsChanged;
WORDP StoreIntWord(int);
void ClearHeapThreads();
void ClearWordMaps();
bool TraceHierarchyTest(int x);
WORDP GetLanguageWord(const char* word);
WORDP GetLanguageWord(const char* word);
WORDP StoreWord(const char* word, uint64 properties = 0);
WORDP StoreFlaggedWord(const char* word, uint64 properties, uint64 flags);
WORDP FindWord(const char* word, unsigned int len = 0,uint64 caseAllowed = STANDARD_LOOKUP,bool exact = false,bool underscorespaceequivalent = false);
unsigned char BitCount(uint64 n);
void ClearVolleyWordMaps();
void ClearBacktracks();
unsigned int* AllocateWhereInSentence(WORDP D);
MEANING GetFactBack(WORDP D);
void SetFactBack(WORDP D, MEANING M);
void KillWord(WORDP D,bool complete);
bool IsQuery(WORDP D);
bool IsRename(WORDP D);
bool ReadForeignPosTags(const char* fname);
int GetWords(char* word, WORDP* set,bool strict,bool allvalid = false);
bool StricmpUTF(char* w1, char* w2, int len);
void ReadQueryLabels(char* file);
void FreeDictionary();
void ClearWordWhere(WORDP D,int at);
void RemoveConceptTopic(HEAPREF list[256],WORDP D, int at);
char* UseDictionaryFile(const char* name,const char* list = NULL);
void ClearWhereInSentence();
char* WordWithLanguage(WORDP D);
void LoadDictionary(char* heapstart);
unsigned int GetHeaderWord(char* w);
void ClearDictionaryFiles(bool tmp=false);
HEAPINDEX CopyWhereInSentence(int oldindex);
void RestorePropAndSystem(char* stringUsed);
void ReadAbbreviations(char* file);
void ReadLiveData(char* language);
void ReadLivePosData();
WORDP GetSubstitute(WORDP D);
uint64 GetProperties(WORDP D);
uint64 GetSystemFlags(WORDP D);
void SetProperties(WORDP D,uint64 properties);
void SetSystemFlags(WORDP D,uint64 systemFlags);
void ShowStats(bool reset);
MEANING FindChild(MEANING who,int n);
void ReadCanonicals(const char* file,const char* layer);
void UndoSubstitutes(HEAPREF list);
WORDP AllocateEntry(char* name);

// adjust data on a dictionary entry
void AddProperty(WORDP D, uint64 flag);
void RemoveProperty(WORDP D, uint64 flag);
void RemoveSystemFlag(WORDP D, uint64 flag);
void AddSystemFlag(WORDP D, uint64 flag);
void AddInternalFlag(WORDP DP, unsigned int flag);
void AddParseBits(WORDP D, unsigned int flag);
void RemoveInternalFlag(WORDP D,unsigned int flag);
void WriteDWord(WORDP ptr, FILE* out);
WORDP ReadDWord(FILE* in);
void AddCircularEntry(WORDP base, unsigned int field,WORDP entry);
void SetWordValue(WORDP D, int x);
int GetWordValue(WORDP D);
void ReadForeign();
inline int GetMeaningCount(WORDP D) { if (!D) return 0;  return (D->meanings) ? GetMeaning(D, 0) : 0; }
inline int GetGlossCount(WORDP D) 
{
	return (D->w.glosses && *D->word != '~' && *D->word != '^' && *D->word != USERVAR_PREFIX && !(D->systemFlags & HAS_SUBSTITUTE) && !(D->internalBits & CONDITIONAL_IDIOM))  ? D->w.glosses[0] : 0;
}
char* GetGloss(WORDP D, unsigned int index);
unsigned int GetGlossIndex(WORDP D,unsigned int index);
void DictionaryRelease(WORDP until,char* stringUsed);
WORDP BUILDCONCEPT(char* word);

// startup and shutdown routines
void InitDictionary();
void CloseDictionary();
void ExtendDictionary();
void WordnetLockDictionary();
void LockLayer();
void ReturnToAfterLayer(int layer,bool unlocked);
void ReturnBeforeBootLayer();
void DeleteDictionaryEntry(WORDP D);
void BuildDictionary(char* junk);
HEAPINDEX GetAccess(WORDP D);
void SetTried(WORDP D, int value);

// read and write dictionary or its entries
void DumpDictionaryEntry(char* word,unsigned int limit);
bool ReadDictionary(char* file);
char* ReadDictionaryFlags(WORDP D, char* ptr,unsigned int *meaningcount = NULL, unsigned int *glosscount = NULL);
char* WriteDictionaryFlags(WORDP D, char* out);
void WriteBinaryDictionary();
bool ReadBinaryDictionary();
void Write32(unsigned int val, FILE* out);
unsigned int Read32(FILE* in);
void Write64(uint64 val, FILE* out);
uint64 Read64(FILE* in);
void Write24(unsigned int val, FILE* out);

// utilities
void ReadWordsOf(char* file,uint64 mark);
void WalkDictionary(DICTIONARY_FUNCTION func,uint64 data = 0);
void WalkLanguages(LANGUAGE_FUNCTION func);
char* FindCanonical(char* word, unsigned int i, bool nonew = false);
void VerifyEntries(WORDP D,uint64 junk);
void NoteLanguage();
void ClearWhereAt(int where);

bool IsHelper(char* word);
bool IsFutureHelper(char* word);
bool IsPresentHelper(char* word);
bool IsPastHelper(char* word);

///   code to manipulate MEANINGs
MEANING MakeTypedMeaning(WORDP x, unsigned int y, unsigned int flags);
MEANING MakeMeaning(WORDP x, unsigned int y = 0);
WORDP Meaning2Word(MEANING x);
MEANING AddMeaning(WORDP D,MEANING M);
void AddGloss(WORDP D,char* gloss,unsigned int index);
void RemoveMeaning(MEANING M, MEANING M1);
MEANING ReadMeaning(char* word,bool create=true,bool precreated = false);
char* WriteMeaning(MEANING T,bool withPOS = false,char* buffer = NULL);
MEANING GetMaster(MEANING T);
unsigned int GetMeaningType(MEANING T);
MEANING FindSynsetParent(MEANING T,unsigned int which = 0);
MEANING FindSetParent(MEANING T,int n);

