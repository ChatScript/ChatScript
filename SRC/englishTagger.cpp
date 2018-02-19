#include "common.h"

extern unsigned int tagRuleCount;
unsigned int ambiguousWords;
bool reverseWords = false;
int ignoreRule = -1;
static int ttLastChanged = 0;
static WORDP firstAux = NULL;
static bool ApplyRules();
static void Tags(char* buffer, int i);
static bool ProcessOmittedClause(unsigned int verb1,bool &changed) ;
static unsigned int quotationCounter;
static unsigned char clauseBit;
static unsigned char prepBit;
static unsigned char verbalBit;
static unsigned int quotationRoles[20];
static unsigned int quotationRoleIndex;
static unsigned char startStack[MAX_CLAUSES];  // where we began this level
static unsigned char auxVerbStack[MAX_CLAUSES];	// most recent aux for this level
static unsigned char objectStack[MAX_CLAUSES];	// most recent object for this level
unsigned char subjectStack[MAX_CLAUSES];  // the subject found for this level of sentence piece (tied to roleIndex)
unsigned char verbStack[MAX_CLAUSES];  // the verb found for this level of sentence piece (tied to roleIndex)
unsigned int needRoles[MAX_CLAUSES]; // what we seek in direct object land or verb land at currnet level of main/phrase/clause
unsigned int roleIndex;
static unsigned int currentMainVerb = 0;
static unsigned int currentVerb2 = 0;
static bool NounFollows( int i,bool thruComma);
char* usedTrace = NULL;
int usedWordIndex = 0;
uint64 usedType = 0;

static int firstnoun;	 // first noun we see in a sentence (maybe object of wrapped prep from end)
static int determineVerbal;
static int firstNounClause;

#define UNKNOWN_CONSTRAINT 2
#define NO_FIELD_INCREMENT 3

#ifdef INFORMATION
A rule consists of 4 64bit values, representing 4 comparator words (result uses one of them also), and a uint64 control word
The control word represents 6 bytes (describing how to interpret the 4 patterns and result), and a 1-byte offset locator to orient the pattern
The result is to either discard named bits or to restrict to named bits on the PRIMARY location, using the bits of the primary include...

A std rule has 4 tests it can consult, anchored around a starting word. A big rule uses a 2nd rule as a continuation, to see 4 more tests.
basic/0:  		6-CONTROL_OP	3-CONTROL_FLAGS		PART2_BIT		----------------3-RESULT_INDEX	48-PATTERN_BITS 
value1:  		6-CONTROL_OP	3-CONTROL_FLAGS		KEEP_BIT	    ----------------3-OFFSET_SHIFT	48-PATTERN_BITS
value2:  		6-CONTROL_OP	3-CONTROL_FLAGS		REVERSE_BIT 	1-unused		1-?		        48-PATTERN_BITS
value3: 		6-CONTROL_OP	3-CONTROL_FLAGS		TRACE_BIT		USED_BIT		2-?	PART1_BIT	48-PATTERN_BITS

The pattern component identies what std word properties are to be checked for from D->properties.
Result (1st field) indicates which test has the word whose bits we want to modify.
Offset_shift (2nd field) indicates where first test word is relative to base word (+ or -)
Control flags are: SKIP, STAY, NOT. 
The control_op  specifies what test to perform on that field.

Rules are executed in forward order only, so later rules can presume earlier rules have already processed appropriately.

# should cardinal adjectives be under DETERMINER and not ADJECTIVE?
# the word HOME is wonderfully overloaded as noun,verb,adjective,adverb for testing

# TRACE on start of a rule result allows you to watch it
# NOGUESS		# do no guessing
# INVERTWORDS	# test sentence words in opposite order

# HAS = any bit matching (could be only bit it has)
# IS = bits from this collection match and no other bits are left over
# !IS will fail if result is ambiguous so it may or may not be.
# INCLUDE = has one or more of these bits AND has other bits -- DO NOT ! this, must be the ONLY field which also has * on it
# -- beware of using !IS (should use HAS) because it will match while still ambiguous
# A pattern should have only one "include", the bits you are deciding to keep or discard. Other places should use IS or HAS.

# SKIP takes a test and a value set.  But it AUTOMATICALLY skips over every phrase or clause already marked

# START, END check location of this word relative to sentence start and end
# ISORIGINAL = is this  word
# ISCANONICAL = is this root word
# ISMEMBER = is canonical word a direct member of this set
# HASPROPERTY = check for systemflag presence of PRONOUN_SINGULAR, OTHER_PLURAL. absence means nothing but presence is important so dont use ! (defined for determiner and pronoun)
# START = this is just before 1st word of sentence
# ISQUESTION aux or qword - sentence begins with possible aux verb or question word
# 0 = no bits
# ! inverts test
# STAY = dont move to next sentence word yet
# x means control/bits not used

# reverse means going backwards... Before the word still has offset -1 (means actually after the word)

# actions: DISCARD, KEEP, or DISABLE
# the current position should always be tested as INCLUDE, because it should have too many bits. and this must be that fields first test (to set result bits)
# tests with HAS mean it may or may not have been fully resolved yet, you are making a heuristic guess

# try to make rules self-standing, not merely a default happening after a prior rule fails to fire
# rules should be independent of each other and the order they are run in. Periodically test inverse order of rules by saying "INVERTRULES" before the first rule.

# top level parts of speech are: 
# PREDETERMINER DETERMINER NOUN_BITS VERB_BITS AUX_VERB ADJECTIVE_BITS ADVERB PREPOSITION CONJUNCTION_COORDINATE CONJUNCTION_SUBORDINATE
# THERE_EXISTENTIAL TO_INFINITIVE PRONOUN_BITS POSSESSIVE_BITS COMMA PAREN PARTICLE NOUN_INFINITIVE

#endif

uint64* dataBuf;
char** commentsData;

static void DropLevel();

// zone data is transient per call to assignroles
#define ZONE_LIMIT 25
static unsigned char zoneBoundary[ZONE_LIMIT];	 // where commas are
static unsigned int	 zoneData[ZONE_LIMIT];		// what can be found in a zone - noun before verb, verb, noun after verb
static unsigned char zoneMember[MAX_SENTENCE_LENGTH];
static unsigned int zoneIndex;			
static int predicateZone; // where is main verb found
static unsigned int currentZone;
static unsigned int ambiguous;
static bool ResolveByStatistic(int i,bool &changed);

#ifdef JUNK
Subject complements are after linking verbs. We label noun complements as direct objects and adjective complements as subject_complement.
Object complements follow a direct object and is noun-like or adjective-like- "The convention named him President" -- not appositive?  verb takes object complement.???
	"The clown got the children *excited"
Verb complement is direct or indirect object of verb. 
	Additionally some verbs expect object complements which are directly nouns, eg FACTITIVE_NOUN_VERB "we elected him *president" which has an omitted "as"  - we elected him as president
	Some verbs expect object complements to be adjectives. eg FACTITIVE_ADJECTIVE_VERB  "we made him *happy"
	Some verbs expect object complements to be infinitive eg VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE  "we want him to go"
			if (parseFlags[i] & (FACTITIVE_ADJECTIVE_VERB|FACTITIVE_NOUN_VERB))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; //

Two nouns in a row:
1. appositive (renaming a noun after - "my *dog *Bob") - can be on any noun
2. Adjectival noun ((char*)"*bank *teller") - can be on any noun
3. indirectobject/directobject ((char*)"I gave *Bob the *ball") - expected by verb
4. object object-complement ((char*)"the convention named *Bob *President" ) expected by verb 
5. omitted clause starter?

To Infinitives verbals can be nouns, postnominal adjectives, or adverb. Cannot be appositive.
1. postnominal adjective:  "her plan to subsidize him was sound"
2. object - "she wanted to raise taxes"
3. subject- "to watch is fun"
4. adverb - "he went to college to study math"

parentheitical infinitive:  to sum up, I worked

adjective_noun 

How do you tell postnominal adjective from adverb--  
can postnominal adjective NOT follow object of a phrase?  
But "it is time to go" or "that was a sight to see"

//  ~factitive_adjective_Verbs take object complement adjective after direct object noun
// ~factitive_noun_verbs take object complement noun after direct object noun
// ~adjectivecomplement_taking_noun_infinitive adjectives can take a noun to infinitive after them as adjective "I was able to go"

Basic main sentence requirements on verb are:
	mainverb
	mainverb subjectcomplement (linking verbs like "be" take noun or adjective as subject complement though we label noun as direct object and adjective as subject complement)
	mainverb directobject  (directobject can be noun, to-infinitve, infinitive, as well as clause, depending on verb)
	mainverb indirectobject directobject
	mainverb directobject objectcomplement (objectcomplement can be noun, infinitive, adjective depending on verb)
#endif

static char* tagOps [] = 
{
	(char*)"?",(char*)"HAS",(char*)"IS",(char*)"INCLUDE",(char*)"CANONLYBE",(char*)"HASORIGINAL",(char*)"PRIORPOS",(char*)"POSTPOS",(char*)"PASSIVEVERB",
	(char*)"HASPROPERTY",(char*)"HASALLPROPERTIES",(char*)"HASCANONICALPROPERTY",(char*)"NOTPOSSIBLEVERBPARTICLEPAIR",
	(char*)"PARSEMARK",
	(char*)"ISORIGINAL",(char*)"ISCANONICAL",(char*)"PRIORCANONICAL",(char*)"ISMEMBER",(char*)"PRIORORIGINAL",(char*)"POSTORIGINAL",
	(char*)"POSITION",(char*)"RESETLOCATION",
	(char*)"HAS2VERBS",(char*)"ISQWORD",(char*)"ISQUESTION",(char*)"ISABSTRACT",
	(char*)"POSSIBLEINFINITIVE",(char*)"POSSIBLEADJECTIVE", "POSSIBLETOLESSVERB",// 23
	(char*)"POSSIBLEADJECTIVEPARTICIPLE",(char*)"HOWSTART",(char*)"POSSIBLEPHRASAL",(char*)"POSSIBLEPARTICLE",(char*)"ISCOMPARATIVE",(char*)"ISEXCLAIM",
	(char*)"ISORIGINALMEMBER",(char*)"ISSUPERLATIVE",(char*)"SINGULAR",(char*)"ISPROBABLE",(char*)"PLURAL",
};
	
unsigned char bitCounts[MAX_SENTENCE_LENGTH]; // number of tags still to resolve in this word position
int lastClause = 0; 
int lastVerbal = 0;
int lastPhrase = 0;
int lastConjunction = 0;
static bool idiomed = false;

unsigned char quotationInProgress = 0;


#ifdef TREETAGGER
// TreeTagger is something you must license for pos-tagging a collection of foreign languages
// Buying a license will get the the library you need to load with this code
// http://www.cis.uni-muenchen.de/~schmid/tools/TreeTagger/

#pragma comment(lib, "../BINARIES/treetagger.lib") // where windows library is

typedef struct {
	int  number_of_words;  /* number of words to be tagged */
	int  next_word;        /* needed internally */
	char **word;           /* array of pointers to the words */
	char **inputtag;       /* array of pointers to the pretagging information */
	const char **resulttag;/* array of pointers to the resulting tags */
	const char **lemma;    /* array of pointers to the lemmas */
} TAGGER_STRUCT;
typedef char*(*FindIt)(char* word);
#ifdef WIN32
bool __declspec(dllimport)  init_treetagger(char *param_file_name, AllocatePtr allocator, FindIt getwordfn);
double __declspec(dllimport)  tag_sentence(int index, TAGGER_STRUCT *ts);
void __declspec(dllimport)  write_treetagger();
#else
bool init_treetagger(char *param_file_name, AllocatePtr allocator, FindIt getwordfn);
double  tag_sentence(int index, TAGGER_STRUCT *ts);
void   write_treetagger();
#endif

TAGGER_STRUCT ts;  /* tagger interface data structure */
TAGGER_STRUCT tschunk;  /* tagger interface data structure */
char* GetTag(int i)
{
	return (char*) ts.resulttag[i - 1];
}

bool MatchTag(char* tag,int i)
{
	char* tttag = (char*)ts.resulttag[i - 1];
	if (!stricmp(tag, "NNP") && !stricmp("NP", tttag)) return true;
	else if (!stricmp(tag, "NNPS") && !stricmp("NPS", tttag)) return true;
	else if (!stricmp(tag, "PRP") && !stricmp("PP", tttag)) return true;
	else if (!stricmp(tag, "PRP$") && !stricmp("PP$", tttag)) return true;
	else if (!stricmp(tag, "-LRB-") && !stricmp("(", tttag)) return true;
	else if (!stricmp(tag, "-RRB-") && !stricmp(")", tttag)) return true;
	// quotes: '/`` self-medicating/VBG ' / ''   - same for both
	else if (!stricmp(tag, "``") || !stricmp(tag, "''"))
	{ // OVERLY GENEROUS- REDACT LATER
		if (!stricmp(tttag, "``") || !stricmp(tttag, "''")) return true;
	}
	else if (!stricmp(tag, "MD")) // modal pennbank
	{
		if (!stricmp("MD", tttag)) return true; //   modals (could would)
		if (!stricmp("VH", tttag)) return true; //   be
		// infinitive
		if (!stricmp("VH", tttag)) return true; //   be
		if (!stricmp("VHD", tttag)) return true; //   have
		if (!stricmp("VDD", tttag)) return true; //   do
		// past
		if (!stricmp("VBD", tttag)) return true; //   be
		if (!stricmp("VHD", tttag)) return true; //   have
		if (!stricmp("VDD", tttag)) return true; //   do
		// gerund
		if (!stricmp("VBG", tttag)) return true; //   be
		if (!stricmp("VHG", tttag)) return true; //   have
		if (!stricmp("VDG", tttag)) return true; //   do
		// past participle
		if (!stricmp("VBN", tttag)) return true; //   be
		if (!stricmp("VHN", tttag)) return true; //   have
		if (!stricmp("VDN", tttag)) return true; //   do
		// 3rd present
		if (!stricmp("VBP", tttag)) return true; //   be
		if (!stricmp("VHP", tttag)) return true; //   have
		if (!stricmp("VDP", tttag)) return true; //   do
		// non-3rd present
		if (!stricmp("VBZ", tttag)) return true; //   be
		if (!stricmp("VHZ", tttag)) return true; //   have
		if (!stricmp("VDZ", tttag)) return true; //   do
	}
	else if (!stricmp(tag, "VB")) // infinitive
	{
		if (!stricmp("VB", tttag)) return true; //   be
		if (!stricmp("VH", tttag)) return true; //   have
		if (!stricmp("VD", tttag)) return true; //   do
		if (!stricmp("VV", tttag)) return true; //   normal
	}
	else if (!stricmp(tag, "VBD")) // past
	{
		if (!stricmp("VBD", tttag)) return true; //   be
		if (!stricmp("VHD", tttag)) return true; //   have
		if (!stricmp("VDD", tttag)) return true; //   do
		if (!stricmp("VVD", tttag)) return true; //   normal
	}
	else if (!stricmp(tag, "VBG")) // gerund/pres participle
	{
		if (!stricmp("VBG", tttag)) return true; //   be
		if (!stricmp("VHG", tttag)) return true; //   have
		if (!stricmp("VDG", tttag)) return true; //   do
		if (!stricmp("VVG", tttag)) return true; //   normal
	}
	else if (!stricmp(tag, "VBN")) // past participle
	{
		if (!stricmp("VBN", tttag)) return true; //   be
		if (!stricmp("VHN", tttag)) return true; //   have
		if (!stricmp("VDN", tttag)) return true; //   do
		if (!stricmp("VVN", tttag)) return true; //   normal
	}
	else if (!stricmp(tag, "VBP")) // non 3rd person present
	{
		if (!stricmp("VBP", tttag)) return true; //   be
		if (!stricmp("VHP", tttag)) return true; //   have
		if (!stricmp("VDP", tttag)) return true; //   do
		if (!stricmp("VVP", tttag)) return true; //   normal
	}
	else if (!stricmp(tag, "VBZ")) //  3rd person present
	{
		if (!stricmp("VBZ", tttag)) return true; //   be
		if (!stricmp("VHZ", tttag)) return true; //   have
		if (!stricmp("VDZ", tttag)) return true; //   do
		if (!stricmp("VVZ", tttag)) return true; //   normal
	}
	else if (!stricmp(tag, "IN") && !stricmp("IN/that", tttag)) return true; //  that
	else if (!stricmp(tag, "HYPH")) // joiner ; - --
	{
		if (!stricmp(":", tttag)) return true;
	}
	// Distinguishes be(VB) and have(VH) from other(non - modal) verbs(VV)
	// SENT for end - of - sentence punctuation(other punctuation tags may also differ)
	else if (!stricmp(tag, tttag)) return true;
	return false;
}

void MarkChunk()
{
	if (tschunk.number_of_words == 0) return;

	WORDP type = NULL;
	int start = 0;
	unsigned int i;
	char word[MAX_WORD_SIZE];
	*word = '~';
	for (i = 0; i < wordCount; ++i)
	{
		char* tag = (char*)tschunk.resulttag[i];
/* Status can be B - starting a chunk or I - inside a chunk continuing a chunk or O - outside a chunk like punctuation, quotation, parentheses, coordinating conjunctions(and, or ).
English tags:
ADJC	  adjective chunks(not inside of noun chunks)
ADVC	  adverb chunks(not inside of noun or adjective chunks)
CONJC	  complex coordinating conjunctions such as "as well (as)" or "rather (than)"
INTJ	  interjection
LST	  enumeration symbol
NC	  noun chunk(non - recursive noun phrase)
PC	  prepositional chunk(usually embeds a noun chunk
PRT	  verb particle
VC	  verb complex
CD/B-NC
*/
		char* complex = strrchr(tag, '-'); // just before complex
		if (!complex) continue;
		strcpy(word+1, complex);
		if (*(complex - 1) == 'B') // complex begin
		{
			if (type) // end prior chunk
			{
				MarkMeaningAndImplications(0, 0, MakeMeaning(type), start, i,false, true);
			}
			type = StoreWord(word);
			AddInternalFlag(type,CONCEPT);
			start = i+1;
		}
		else if (*(complex - 1) == 'O') // complex output of chunk (like punctuation)
		{
			if (type) // end prior chunk
			{
				MarkMeaningAndImplications(0, 0, MakeMeaning(type), start, i,false, true);
				type = NULL;
			}
		}
		else if (!strcmp(type->word,complex))  // I  prior complex continued
		{
		}
		else// shouldnt happen
		{
		}
	}
	if (type) MarkMeaningAndImplications(0, 0, MakeMeaning(type), start, wordCount,false,true);
}

static void TreeTagger()
{
	int i;
	for (i = 0; i < wordCount; ++i)
	{
		ts.word[i] = wordStarts[i + 1];
		ts.inputtag[i] = NULL;
	}
	ts.number_of_words = wordCount;
	tag_sentence(0, &ts);
	if (trace & (TRACE_PREPARE | TRACE_POS | TRACE_TREETAGGER)) Log(STDTRACELOG, "External Tagging:\r\n");

	bool chunk = strstr(treetaggerParams, "chunk");
	if (chunk) // do chunking here but marking later
	{
		for (i = 0; i < wordCount; ++i)
		{
			tschunk.word[i] = (char*)ts.resulttag[i];
			tschunk.inputtag[i] = NULL;
		}
		tschunk.number_of_words = wordCount;
		tag_sentence(1, &tschunk);
		int starter = -1;
		char type[20];
		for (i = 0; i < wordCount; ++i)
		{
			char status = *(strrchr((char*)tschunk.resulttag[i], '/') + 1);
			char* kind = strrchr((char*)tschunk.resulttag[i], '-') + 1;
			if (status == 'I') continue; // continue a chunk
			if (starter >= 0) // just ended a chunk
			{
				if (trace & (TRACE_PREPARE | TRACE_TREETAGGER)) Log(STDTRACELOG, (char*)"(%s) %s..%s \r\n", type, wordStarts[starter + 1], wordStarts[i]);
				MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord(type)), starter + 1, i);
				starter = 0;
			}
			if (status == 'B') // begin a chunk
			{
				starter = i;
				strcpy(type, kind);
			}
		}
		if (starter >= 0) // just ended a chunk
		{
			if (trace & (TRACE_PREPARE | TRACE_TREETAGGER)) Log(STDTRACELOG, (char*)"(%s) %s..%s\r\n", type, wordStarts[starter + 1], wordStarts[wordCount]);
			MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord(type)), starter + 1, wordCount);
		}
	}

	// set up arrays with TT results in case they are to be adjusted
	char newtag[MAX_WORD_SIZE];
	for (i = 1; i <= wordCount; i++)
	{
		WORDP canonical = NULL;
		char* lemma = (char*)ts.lemma[i - 1];
		if (!lemma || !strcmp(lemma, "<unknown>")) lemma = (char*)"unknown-word";
		if (strcmp(lemma, "@card@")) { // leave special card canonical alone, preserving the original number 
			canonical = StoreWord(lemma, 0);
			wordCanonical[i] = canonical->word;
		}
		else
		{
			canonical = StoreWord(wordCanonical[i], 0); // make sure the canonical word exists
		}

		char* tag = (char*)ts.resulttag[i - 1];
		if (!tag) tag = (char*)"unknown-tag";
		*newtag = '~';	// concept from the tag
		strcpy(newtag + 1, tag);
		WORDP tagword = FindWord(newtag);
		if (!tagword)
		{
			strcpy(newtag + 1, (char*)"unknown-tag");
			tagword = FindWord(newtag);
		}
		wordTag[i] = tagword;
	}

	// TreeTagger results may need a bit of tweaking, so gambit a topic
	char* taggingTopic = GetUserVariable((char*)"$cs_externaltag");
	if (*taggingTopic)
	{
		char* oldhow = howTopic;
		howTopic = "gambit";

		int oldreuseid = currentReuseID;
		int oldreusetopic = currentReuseTopic;
		int topicid = FindTopicIDByName(taggingTopic);
		if (topic && !(GetTopicFlags(topicid) & TOPIC_BLOCKED))
		{
            CALLFRAME* frame = ChangeDepth(1, (char*)"$cs_externaltag");
            int pushed = PushTopic(topicid);
			if (pushed >= 0)
			{
				PerformTopic(GAMBIT, currentOutputBase);
				if (pushed) PopTopic();
			}
			ChangeDepth(-1, (char*)"$cs_externaltag");
			currentReuseID = oldreuseid;
			currentReuseTopic = oldreusetopic;
			howTopic = oldhow;
		}
	}

	/* mix a bit of ours and theirs */
	if (stricmp(language, "english")) for (i = 1; i <= wordCount; i++)
	{
		originalUpper[i] = NULL;
		originalLower[i] = NULL;

		// Canonicals might have adjusted lemma explicitily via SetCanon() in the $cs_externaltag topic
		WORDP canonical0 = FindWord(wordCanonical[i]);

		// Might have adjusted TT's initial tag via SetTag() in the $cs_externaltag topic
		char newtag[MAX_WORD_SIZE];
		strcpy(newtag, wordTag[i]->word);
		*newtag = '_';
		WORDP X = FindWord(newtag);

		int start = 1;
		WORDP entry = NULL;
		WORDP canonical = 0;
		uint64 sysflags = 0;
		uint64 cansysflags = 0;
		WORDP revise;
		uint64 flags = GetPosData(i, wordStarts[i], revise, entry, canonical, sysflags, cansysflags, true, false, start); // flags will be potentially beyond what is stored on the word itself (like noun_gerund) but not adjective_noun
		if (revise != NULL) wordStarts[i] = revise->word;

		// Reuse the lemma word unless
		// - the word is a concept, 
		// - we have found a better version of the canonical
		// - this is a number and TT thinks so too, the CS canonical will be digits
		if (*wordStarts[i] == '~') { ; }
		else if (canonical0->properties == 0 && canonical->properties > 0) { ; }
		else if (flags & NUMBER_BITS && X && X->properties & NUMBER_BITS) { ; }
		else
		{
			canonical = canonical0;
			flags = 0;
		}
		if (!canonical) canonical = entry;

		if (IsUpperCase(canonical->internalBits & UPPERCASE_HASH))
		{
			originalUpper[i] = entry;
			canonicalUpper[i] = canonical;
		}
		else
		{
			originalLower[i] = entry;
			canonicalLower[i] = canonical;
		}

		parseFlags[i] = canonical->parseBits;
		posValues[i] = flags;
		if (originalLower[i]) lcSysFlags[i] = sysflags; // from lower case
		canSysFlags[i] = cansysflags;
		if (entry->properties & PART_OF_SPEECH) ++knownWords; // known as lower or upper
		if (*wordStarts[i] == '~') posValues[i] = 0;	// interjection
		wordCanonical[i] = canonical->word;
		if (X) posValues[i] |= X->properties; // english pos tag references added
	}
	if (trace & (TRACE_PREPARE | TRACE_POS))
	{
		for (i = 1; i <= wordCount; i++)
		{
			char* tag = (char*)ts.resulttag[i - 1];
			if (chunk)
			{
				char* tag1 = (char*)tschunk.resulttag[i - 1];
				Log(STDTRACELOG, "%s(%s %s) ", wordStarts[i], tag1, ts.lemma[i - 1]);
			}
			else Log(STDTRACELOG, "%s(%s %s) ", wordStarts[i], tag, ts.lemma[i - 1]);
		}
		Log(STDTRACELOG, "\r\n");
	}
}

static void BlendWithTreetagger(bool &changed)
{
	if (!ts.number_of_words) return;
	static int modified = 0;

	// ts.number_of_words = 0;	// do only once
	char word[MAX_WORD_SIZE];
	*word = '_';	// marker to keep any collision away from foreign pos
	int i;
	int blocked = 0;
	for (i = 1; i <= wordCount; ++i)
	{
		strcpy(word + 1, ts.resulttag[i - 1]);
		WORDP D = FindWord(word, 0, PRIMARY_CASE_ALLOWED);
		if (!D) continue;	// didnt find it?
		uint64 bits = D->properties & TAG_TEST; // dont want general headers like NOUN or VERB
		uint64 possiblebits = posValues[i] & TAG_TEST; // bits we are trying to resolve
		if (bits & VERB_INFINITIVE && possiblebits & VERB_PRESENT) possiblebits |= VERB_INFINITIVE; // when we launch, we want to be right
		if (bits & PREPOSITION && possiblebits & PARTICLE) bits |= PARTICLE; 
		if (bits & PARTICLE && possiblebits & PREPOSITION) bits |= PREPOSITION;
		if (bits & NOUN_SINGULAR && possiblebits & (PRONOUN_SUBJECT | PRONOUN_OBJECT)) bits |= PRONOUN_SUBJECT | PRONOUN_OBJECT;
		if (!stricmp(D->word+1,"CD") && possiblebits & (PRONOUN_SUBJECT | PRONOUN_OBJECT))  bits |= PRONOUN_SUBJECT | PRONOUN_OBJECT; // "one" as pronoun
		if (bits & ADJECTIVE_NORMAL && possiblebits & DETERMINER)
		{
			bits ^= ADJECTIVE_NORMAL;
			bits |= DETERMINER; // I could make *other arrangements
		}
		uint64 allowable = bits & possiblebits; // bits for this tagtype
		if (allowable && allowable != possiblebits) // we can update choices
		{
			posValues[i] ^= possiblebits;
			posValues[i] |= allowable;
			bitCounts[i] = BitCount(posValues[i]);
			changed = true;
			if (trace & TRACE_TREETAGGER)
			{
				++modified;
				Log(STDTRACELOG, (char*)"Treetagger adjusted %d: \"%s\" given %s, removing: ", i, wordStarts[i], ts.resulttag[i - 1]);
				uint64 lost = possiblebits ^ allowable; // these bits disappeared
				uint64 bit = START_BIT;
				while (bit)
				{
					if (lost & bit) Log(STDTRACELOG, (char*)"%s ", FindNameByValue(bit));
					bit >>= 1;
				}
				Log(STDTRACELOG, (char*)" CS Remain: ");
				bit = START_BIT;
				while (bit)
				{
					if (allowable & bit)
						Log(STDTRACELOG, (char*)"%s ", FindNameByValue(bit));
					bit >>= 1;
				}
				Log(STDTRACELOG, (char*)"\r\n");
			}
			break;	// as soon as changed. return
		}
		else if (!allowable && trace & (TRACE_PREPARE | TRACE_TREETAGGER))
		{
			Log(STDTRACELOG, (char*)"Treetagger could not adjust %d: \"%s\" given %s, leaves CS as: ", i, wordStarts[i], ts.resulttag[i - 1]);
			uint64 bit = START_BIT;
			++blocked;
			while (bit)
			{
				if (possiblebits & bit) Log(STDTRACELOG, (char*)"%s ", FindNameByValue(bit));
				bit >>= 1;
			}

			Log(STDTRACELOG, (char*)"instead of changing to:");
			uint64 lost = bits & (-1 ^ possiblebits); // these bits disappeared from treetagger
			bit = START_BIT;
			while (bit)
			{
				if (lost & bit) Log(STDTRACELOG, (char*)"%s ", FindNameByValue(bit));
				bit >>= 1;
			}
			Log(STDTRACELOG, (char*)"\r\n");
		}
		else if (trace & (TRACE_PREPARE | TRACE_TREETAGGER))
		{
			Log(STDTRACELOG, (char*)"Treetagger matches %d: \"%s\" TT: %s CS: ", i, wordStarts[i], ts.resulttag[i - 1]);
			uint64 bit = START_BIT;
			while (bit)
			{
				if (allowable & bit)
					Log(STDTRACELOG, (char*)"%s ", FindNameByValue(bit));
				bit >>= 1;
			}
			Log(STDTRACELOG, (char*)"\r\n");
		}
	}
	if (trace & (TRACE_PREPARE | TRACE_TREETAGGER)) Log(STDTRACELOG, (char*)"\r\n");
	ttLastChanged = i;  // or it ran out
	if (i > wordCount)
	{
		if (trace & (TRACE_PREPARE | TRACE_TREETAGGER))	Log(STDTRACELOG, (char*)"TT Adjustments: %d  Refusals: %d  Words %d\r\n\r\n", modified, blocked,wordCount);
		modified = 0;
	}
}

void InitTreeTagger(char* params) // tags=xxxx - just triggers this thing
{
	if (!*params) return;

	// load each foreign postag and its correspondence to english postags
	char name[MAX_WORD_SIZE];
	char lang[MAX_WORD_SIZE];
	MakeLowerCopy(lang, language);
	sprintf(name, "DICT/%s_tags.txt", lang);
	if (!ReadForeignPosTags(name)) return; //failed 

	externalTagger = 2;	// using external tagging
	char langfile[MAX_WORD_SIZE];
	sprintf(langfile, "treetagger/%s.par", language);
	MakeLowerCase(langfile);
	//write_treetagger();
	bool result = init_treetagger(langfile, AllocateHeap, GetWord); //  NULL, NULL);  /*  Initialization of the tagger with the language parameter file */
	if (strstr(params, "chunk") && result)
	{
		sprintf(langfile, "treetagger/%s_chunker.par", language);
		MakeLowerCase(langfile);
		result = init_treetagger(langfile, AllocateHeap, GetWord); //  NULL, NULL);  /*  Initialization of the tagger with the chunker parameter file */
		if (!result) strcpy(params, "1"); // give up chunking
	}
	if (result) externalPostagger = TreeTagger;
	else
	{
		(*printer)("Unable to load %s\r\n", langfile);
		return;
	}

	/* Memory allocation (the maximal input sentence length is here 1000) */
	ts.word = (char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH);
	ts.inputtag = (char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH, true);
	ts.resulttag = (const char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH, true);
	ts.lemma = (const char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH, true);
	tschunk.word = (char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH);
	tschunk.inputtag = (char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH, true);
	tschunk.resulttag = (const char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH, true);
	tschunk.lemma = (const char**)AllocateHeap(NULL, sizeof(char*) * MAX_SENTENCE_LENGTH, true);
}
#endif

static void DumpCrossReference(int start, int end)
{
	Log(STDTRACELOG,(char*)"Xref: ");
	for (int i = start; i <= end; ++i)
	{
		Log(STDTRACELOG,(char*)"%d:%s",i,wordStarts[i]);
		if (crossReference[i]) Log(STDTRACELOG,(char*)" >%d",crossReference[i]);
		if (indirectObjectRef[i]) Log(STDTRACELOG,(char*)" i%d",indirectObjectRef[i]);
		if (objectRef[i]) Log(STDTRACELOG,(char*)" o%d",objectRef[i]);
		if (complementRef[i]) Log(STDTRACELOG,(char*)" c%d",complementRef[i]);
		Log(STDTRACELOG,(char*)"   ");
	}
	Log(STDTRACELOG,(char*)"\r\n");
	for (int i = start; i <= end; ++i)
	{
		Log(STDTRACELOG,(char*)"%d:%s",i,wordStarts[i]);
		if (phrases[i]) Log(STDTRACELOG,(char*)" p%x",phrases[i]);
		if (verbals[i]) Log(STDTRACELOG,(char*)" v%x",verbals[i]);
		if (clauses[i]) Log(STDTRACELOG,(char*)" c%x",clauses[i]);
		Log(STDTRACELOG,(char*)"   ");
	}
	Log(STDTRACELOG,(char*)"\r\n");
}

static void SetCanonicalValue(int start,int end)
{
	int upper = 0;
	int lower = 0;
	for (int i = start; i <= end; ++i)
	{
		if (ignoreWord[i]) continue;
		if (IsUpperCase(*wordStarts[i])) ++upper;
		else ++lower;
	}
	bool caseSignificant = (lower > 3 && lower > upper);

	// now set canonical lowercase forms
	for (int i = start; i <= end; ++i)
    {
		if (ignoreWord[i]) continue;

		char* original =  wordStarts[i];
		WORDP can = canonicalLower[i];
		if (originalLower[i]) original = originalLower[i]->word;
		uint64 pos = posValues[i] & (TAG_TEST|PART_OF_SPEECH);
		if (!pos && !(*original == '~')) posValues[i] = pos = NOUN;		// default it back to something
		WORDP D = FindWord(original);
		WORDP canon1 = (D) ? GetCanonical(D) : NULL;
		char* canon =  (canon1) ? canon1->word : NULL;
		if (posValues[i] & (DETERMINER| IDIOM) && original[1] == 0)  // treat "a" as not a letter A
		{
			canon = NULL;
			canonicalLower[i] = originalLower[i];
		}
		// a word like "won" has noun, verb, adjective meanings. We prefer a canonical that's different from the original
		if (canon && IsUpperCase(*canon)) canonicalUpper[i] = FindWord(canon);
		else if (canon) canonicalLower[i] = FindWord(canon);
		else if (pos & NUMBER_BITS); // must occur before verbs and nouns, since "second" is a verb and a noun
		else if (canonicalLower[i] && canonicalLower[i]->properties & (NOUN_NUMBER|ADJECTIVE_NUMBER)); // dont change canonical numbers like December second
		else if (allOriginalWordBits[i] & NOUN_GERUND) // because singing is a dict word, we might prefer noun over gerund. We shouldned
		{
			canonicalLower[i] = FindWord(GetInfinitive(original,false));
		}
		else if (pos & (VERB_BITS | NOUN_GERUND | NOUN_INFINITIVE|ADJECTIVE_PARTICIPLE) ) 
		{
			canonicalLower[i] = FindWord(GetInfinitive(original,false));
		}
		else if (pos & ADJECTIVE_NORMAL && !(D && D->properties & (MORE_FORM|MOST_FORM)))
		{
			canonicalLower[i] = originalLower[i]; // "his *fixed view should be adjective and not participle given it is an adjective- arbitrary
			if (allOriginalWordBits[i] & ADJECTIVE_PARTICIPLE) 
			{
				char* verb = GetInfinitive(wordStarts[i],true);
				if (verb) canonicalLower[i] = FindWord(verb);
			}
		}
		else if (pos & CONJUNCTION)
		{
			canonicalLower[i] = originalLower[i]; // "his *fixed view should be adjective and not participle given it is an adjective- arbitrary
		}
		else if (pos & (NOUN_BITS - NOUN_GERUND - NOUN_ADJECTIVE)  || (canonicalLower[i] && !stricmp(canonicalLower[i]->word,original))) 
		{
			if (pos & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL) && canonicalUpper[i] && canonicalUpper[i]->properties & NOUN) // can it be upper case interpretation?
			{
				// if ONLY upper case interpretation
				if (!(pos & (VERB_BITS|NOUN_SINGULAR|NOUN_PLURAL|ADJECTIVE_NOUN)) && !canonicalLower[i]) 
				{
					char* word = (originalUpper[i]) ? originalUpper[i]->word : canonicalUpper[i]->word;
					word = AllocateHeap(word); // dont share because we might edit the word in place (eg ONLY_LOWER)
					original = wordStarts[i] = word; // make it upper case
					originalLower[i] = canonicalLower[i] = 0;	// blow away any lower meaning
				}
			}
			if (canonicalLower[i] && canonicalLower[i]->properties & (DETERMINER|NUMBER_BITS));
			else if (IsAlphaUTF8(*original) &&  canonicalLower[i] && !strcmp(canonicalLower[i]->word,(char*)"unknown-word"));	// keep unknown-ness
 			else if (pos & NOUN_BITS && !canonicalUpper[i]) 
			{
				char* noun = GetSingularNoun(original,false,true);
				if (noun) canonicalLower[i] = FindWord(noun);
			}
		}
		else if (pos & (ADJECTIVE_BITS - ADJECTIVE_PARTICIPLE - ADJECTIVE_NOUN) || (canonicalLower[i] && !stricmp(canonicalLower[i]->word,original))) 
		{
			if (canonicalLower[i] && canonicalLower[i]->properties & NUMBER_BITS);
			else 
			{
				char* adj = GetAdjectiveBase(original,false);
				if (adj) canonicalLower[i] = FindWord(adj);
			}

			// for adjectives that are verbs, like married, go canonical to the verb if adjective is unchanged
			if (canonicalLower[i] && !strcmp(canonicalLower[i]->word,original))
			{
				char* infinitive = GetInfinitive(original,false);
				if (infinitive) canonicalLower[i] = FindWord(infinitive);
			}
		}
		else if (pos & ADJECTIVE_NOUN) 
		{
			if (canonicalLower[i] && canonicalLower[i]->properties & NUMBER_BITS);
			else if (IsUpperCase(*wordStarts[i]) && caseSignificant) {;}  //  upper case is intentional
			else
			{
				char* adj = GetAdjectiveBase(original,false);
				if (adj) canonicalLower[i] = FindWord(adj);
			}
		}
		else if (pos & ADVERB || (canonicalLower[i] && !stricmp(canonicalLower[i]->word,original))) 
		{
			if (canonicalLower[i] && canonicalLower[i]->properties & NUMBER_BITS);
			else canonicalLower[i] = FindWord(GetAdverbBase(original,false));
			// for adverbs that are adjectives, like faster, go canonical to the adjective if adverb is unchanged
			if (canonicalLower[i] && !strcmp(canonicalLower[i]->word,original))
			{
				char* adjective = GetAdjectiveBase(original,false);
				if (adjective) canonicalLower[i] = FindWord(adjective);
			}
		}
		else if (pos & (PRONOUN_BITS|CONJUNCTION|PREPOSITION|DETERMINER_BITS|PUNCTUATION|COMMA|PAREN)) canonicalLower[i] = FindWord(original);
		else if (*original == '~') canonicalLower[i] = FindWord(original);
		else if (!IsAlphaUTF8(*original)) canonicalLower[i] = FindWord(original);

		if (pos & PRONOUN_BITS && !stricmp(original,(char*)"one")) // make it a number
		{
			canonicalLower[i] = StoreWord((char*)"1",NOUN|NOUN_NUMBER);
		}

		// handle composite verb canonical for single hypen case
		char* hyphen = strchr(original,'-');
		if (hyphen && pos & (VERB_BITS|NOUN_GERUND|ADJECTIVE_PARTICIPLE|NOUN_INFINITIVE)) // find the verb root.
		{
			char word[MAX_WORD_SIZE];
			strcpy(word,original);
			char* h = word + (hyphen-original);
			char* verb = GetInfinitive(h+1,true);
			if (verb) // 2nd half
			{
				strcpy(h+1,verb);
				canonicalLower[i] = StoreWord(word,VERB|VERB_INFINITIVE);
			}
			else 
			{
				*h = 0;
				verb = GetInfinitive(word,true);
				if (verb)
				{
					strcpy(word,verb);
					strcat(word,hyphen);
					canonicalLower[i] = StoreWord(word,VERB|VERB_INFINITIVE);
				}
			}
		}

		if (can == DunknownWord) // restore unknown word status
		{
			if (IsUpperCase(*original)) canonicalUpper[i] = can;
			else canonicalLower[i] = can;
		}
		if (canonicalLower[i] && IsDigit(*canonicalLower[i]->word)) wordCanonical[i] = canonicalLower[i]->word; // leave numbers alone
		else if (canonicalLower[i] && originalLower[i]) 
		{
			if (posValues[i] & NOUN_SINGULAR && !(allOriginalWordBits[i] & NOUN_GERUND) && stricmp(canonicalLower[i]->word,(char*)"unknown-word")) // saw does not become see, it stays original - but singing should still be sing and "what do you think of dafatgat" should remain
			{
				canonicalLower[i] = originalLower[i];
				wordCanonical[i] = originalLower[i]->word; 
			}
			else wordCanonical[i] = canonicalLower[i]->word;
		}
		else if (canonicalUpper[i]) wordCanonical[i] = canonicalUpper[i]->word;
		else wordCanonical[i] = wordStarts[i];
	}
	SetSentenceTense(start,end);
}

static char* PosBits(uint64 bits, char* buff)
{
	while (bits)  // shows lowest order bits first
	{  
		uint64 oldbits = bits;
		bits &= (bits - 1); 
  		strcat(buff,(char*)" ");
		strcat(buff,FindNameByValue(oldbits ^ bits));
	}  
	return buff;
}

static int NextPos(int i)
{
	while (posValues[++i] == IDIOM || posValues[i] == QUOTE ){;} 
	return i;
}

static int Next2Pos(int i)
{
	while (posValues[++i] == IDIOM || posValues[i] == QUOTE){;} 
	while (posValues[++i] == IDIOM || posValues[i] == QUOTE){;} 
	return i;
}

static char* PropertyBits(uint64 bits, char* buff)
{
	while (bits)  // shows lowest order bits first
	{  
		uint64 oldbits = bits;
		bits &= (bits - 1); 
  		strcat(buff,(char*)" ");
		strcat(buff,FindSystemNameByValue(oldbits ^ bits));
	}  
	return buff;
}

static bool LimitValues(int i, uint64 bits,char* msg,bool& changed)
{
	uint64 old = posValues[i];
	posValues[i] &= bits;
	char buff[MAX_WORD_SIZE];
	if (old != posValues[i])
	{
		int oldcount = bitCounts[i];
		changed = true;
		if (posValues[i] == 0) // shrank to nothing
		{
			if (bits & ADJECTIVE_NOUN) // special insertion of adjective_noun
			{
				posValues[i] = ADJECTIVE_NOUN;
				allOriginalWordBits[i] |= ADJECTIVE_NOUN;
			}
			else if (bits & NOUN_ADJECTIVE) // special insertion of this
			{
				posValues[i] = NOUN_ADJECTIVE;
				allOriginalWordBits[i] |= NOUN_ADJECTIVE;
			}
			else if (!bits) posValues[i] = allOriginalWordBits[i] & TAG_TEST;
			else posValues[i] = bits & allOriginalWordBits[i] & TAG_TEST; // back up to what it COULD have been originally that we will now accept
			if (trace & TRACE_POS)
			{
				*buff = 0;
				PosBits(posValues[i],buff);
				Log(STDTRACELOG,(char*)"    Limit recovery  \"%s\"(%d) %s -> %s\r\n",wordStarts[i],i,msg,buff);
			}
		}
		bitCounts[i] = BitCount(posValues[i]);

		if (bitCounts[i] == 1 && oldcount != 1) --ambiguous;	
		else if (bitCounts[i] > 1 && oldcount == 1) ++ambiguous;
		else if (bitCounts[i] == 0) // couldnt return to old??? 
		{
			if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"    Limit recovery  error %s %d \r\n",wordStarts[i],i);
			ambiguous = 1000; // cannot be fixed
			return false;
		}

		if (trace & TRACE_POS) 
		{
			*buff = 0;
			PosBits(posValues[i],buff);
			Log(STDTRACELOG,(char*)"    Limit \"%s\"(%d) %s -> %s\r\n",wordStarts[i],i,msg,buff);
		}
		return true;
	}
	return false;
}

static void ParseFlags0(char* buffer,  int i);
static void PerformPosTag(int start, int end)
{
	if (start > end) 
	{
		wordCanonical[end] = wordStarts[end];
		canonicalUpper[end] = canonicalLower[end] = 0;
		canonicalLower[end] = originalLower[end] = StoreWord(wordStarts[end]); // have something
		return;
	}

	// if has upper and lower case but no lower case words, there are only nouns and adjective_numbers
	bool haslower = false;
	bool mixedcase = false;
	for (int i = start; i <= end; ++i)
	{
		if (IsLowerCase(wordStarts[i][0]))
		{
			haslower = true;
			break;
		}
		if (IsLowerCase(wordStarts[i][1])) mixedcase = true;
	}
	if (!haslower && mixedcase)// all words are uppercase start and mixed case.
	{
		for (int i = start; i <= end; ++i) posValues[i] &= NOUN_BITS;
	}

	bool noPosTagging = false;

	// decide if sentence has too many caps, is a titled thing that should be considered all lower case
	unsigned int total = 0; // words within quotes
	uint64 oldTokenControl = tokenControl;
	unsigned int maxContigLower = 0;
	unsigned int contigLower = 0;
	for ( int j = start; j <= end; ++j)
    {
		if (ignoreWord[j]) continue;	
		if (!IsAlphaUTF8(*wordStarts[j])) continue;	 // not part of count is punctuation and numbers
		++total;
		if (capState[j]) 
		{
			if (contigLower > maxContigLower) maxContigLower = contigLower;
			contigLower = 0;
		}
		else 
		{
			++contigLower;
		}
	}
	if (contigLower > maxContigLower) maxContigLower = contigLower;
	// DO NOT FORCE STRICT CASING WHEN uppercase given

	if (oobExists) noPosTagging = true; // no out-of-band parsetagging
	else if (prepareMode == POS_MODE || tmpPrepareMode == POS_MODE || prepareMode == POSVERIFY_MODE){;} // told to try regardless
	else if (tokenControl & DO_PARSE ) {;} // pos tag at a minimum
	else noPosTagging = true; 

	startSentence = start;
	endSentence = end;

	// initialize the words with whatever pos they MIGHT be, from dictionary as well as what might be inferred
    for (int i = start; i <= end; ++i)
    {
		char* original =  wordStarts[i];
		if (!*original) continue; // bug?
		if (tokenControl & ONLY_LOWERCASE && IsUpperCase(*original)  && (*original != 'I' || original[1])) 
			MakeLowerCase(original);
		
		WORDP D = StoreWord(original);
		if (D->internalBits & UPPERCASE_HASH)
		{
			originalLower[i] = (D->internalBits & UPPERCASE_HASH) ? NULL : D;
			canonicalLower[i] = NULL;
			originalUpper[i] = (D->internalBits & UPPERCASE_HASH) ? D : NULL;
			canonicalUpper[i] = NULL;
		}

		WORDP entry = NULL;
		WORDP canonical = NULL;
		uint64 sysflags = 0;
		uint64 cansysflags = 0;
		WORDP revise;
		uint64 flags = GetPosData(i,original,revise,entry,canonical,sysflags,cansysflags,true,false,start); // flags will be potentially beyond what is stored on the word itself (like noun_gerund) but not adjective_noun
		if (revise != NULL) wordStarts[i] = revise->word;
		if (!canonical) canonical = entry;
		if (entry->internalBits & UPPERCASE_HASH && !(flags & PRONOUN_BITS)) // treat I as lower case original -- note: (char*)"Sue" fails to have cannonical upper
		{
			originalUpper[i] = entry;
			canonicalUpper[i] = canonical;
			originalLower[i] = NULL;
			canonicalLower[i] = NULL;
		}
		else
		{
			originalLower[i] = entry;
			canonicalLower[i] = canonical;
			originalUpper[i] = NULL;
			canonicalUpper[i] = NULL;
		}
		parseFlags[i] = canonical->parseBits;
		if (flags & POSSESSIVE && *entry->word == '\'') // is this a possessive quotemark or a QUOTE?
		{
			size_t len = (i > start) ? strlen(wordStarts[i-1]) : 0;
			if (posValues[i-1] & NORMAL_NOUN_BITS){;} // it can be possessive after a noun
			else if (!len || wordStarts[i-1][len-1] != 's')  
			{
				if (stricmp(wordStarts[i-1],(char*)"he") && stricmp(wordStarts[i-1],(char*)"they") && stricmp(wordStarts[i-1],(char*)"it") && stricmp(wordStarts[i-1],(char*)"hers")) 
				{
					flags = QUOTE;	// change from possessive to QUOTE if word before it does not end in s or is not it, he, they,hers
				}
			}
		}
		posValues[i] = flags;
		if (originalLower[i]) lcSysFlags[i] = sysflags; // from lower case
		canSysFlags[i] = cansysflags;
		if (entry->properties & PART_OF_SPEECH) ++knownWords; // known as lower or upper
		if (*wordStarts[i] == '~') posValues[i] = 0;	// interjection
		if ( noPosTagging) // display nothing
		{
//			posValues[i] = 0; // concepts wont work if bits are turned off. we just need them all on that might be
//			canSysFlags[i] = 0;
			wordCanonical[i] = canonical->word;
		}
	}

	// first seem plural but is a name like "Bears have"  
	if (posValues[start] & NOUN_PROPER_SINGULAR)
	{
		if (!(posValues[start+1] & SINGULAR_PERSON)) // this is good  "*Billings was" otherwise enter if
		{
			size_t len = strlen(wordStarts[start]);
			if (wordStarts[start][len-1] == 's') // is this actually plural lowercase noun?
			{
				bool mismatch = false;
				if (posValues[start+1] & (VERB_PRESENT|PREPOSITION)) mismatch = true;
				if (!(posValues[start+1] & (VERB_BITS|AUX_VERB)) && posValues[start+2] & (VERB_BITS|AUX_VERB) && posValues[start+2] & VERB_PAST) mismatch = true; 
				if (posValues[start+1] & (AUX_BE|AUX_HAVE|AUX_DO) && posValues[start+1] & VERB_PAST) mismatch = true; // plural person aux
				if (mismatch)
				{
					char word[MAX_WORD_SIZE];
					MakeLowerCopy(word,wordStarts[start]);
					wordStarts[start] = StoreWord(word)->word;
					WORDP entry;
					WORDP canonical;
					uint64 sysflags = 0;
					uint64 cansysflags = 0;
					WORDP revise;
					uint64 flags = GetPosData(start,wordStarts[start],revise,entry,canonical,sysflags,cansysflags,true,false,start); // flags will be potentially beyond what is stored on the word itself (like noun_gerund) but not adjective_noun
					if (revise) wordStarts[start] = revise->word;
					if (!canonical) canonical = entry;
					originalLower[start] = entry;
					canonicalLower[start] = canonical;
					posValues[start] = flags;
					lcSysFlags[start] = sysflags; // from lower case
					canSysFlags[start] = cansysflags;
					if ( noPosTagging) // display nothing
					{
						posValues[start] = 0;
						canSysFlags[start] = 0;
						wordCanonical[start] = canonical->word;
					}
				}
			}
		}
	}

	 // get all possible flags first and backfix PRONOUN followed by ' Possessive as PRONOUN_POSSESSIVE
	for ( int i = start; i <= end; ++i)
	{
		if (!(tokenControl & STRICT_CASING)  && posValues[i+1] & POSSESSIVE && posValues[i] & PRONOUN_SUBJECT && *wordStarts[i] != 'i') posValues[i] = PRONOUN_POSSESSIVE; // his became he '  
		allOriginalWordBits[i] = posValues[i];	// for words to know what was possible
		posValues[i] &= TAG_TEST;  // only the bits we care about for pos tagging
		// generator's going down... rewrite here
		if (posValues[i] & POSSESSIVE && wordStarts[i][1] && posValues[i-1] & NOUN_SINGULAR && posValues[i+1] & VERB_PRESENT_PARTICIPLE && bitCounts[i+1] == 1)
		{
			wordStarts[i] = AllocateHeap((char*)"is");
			originalLower[i] = FindWord(wordStarts[i]);
			posValues[i] = AUX_VERB_PRESENT;
			canonicalLower[i] = FindWord((char*)"be");
		}
	}

	if (tokenControl & NO_IMPERATIVE && posValues[start] & VERB_BITS && posValues[start+1] & NOUN_BITS)
	{
		WORDP imperative = FindWord((char*)"~legal_imperatives");
		int i = startSentence;
		if (!stricmp(wordStarts[startSentence],(char*)"please")) ++i;
		if (canonicalLower[i] && SetContains(MakeMeaning(imperative),MakeMeaning(canonicalLower[i]))){;}
		else posValues[i] &= -1 ^ (VERB_BITS|VERB);	// no command sentence starts
	}
	tokenControl = oldTokenControl;
	if (noPosTagging || stricmp(language, "english")) return;

	uint64 startTime = 0;
	ttLastChanged = 0;
	if (prepareMode == POSTIME_MODE) startTime = ElapsedMilliseconds();

	// process sentence zones
	for (int i = start; i <= end; ++i) 
	{
		if (ignoreWord[i]) continue;
		bitCounts[i] = BitCount(posValues[i]);
		if (bitCounts[i] != 1) ++ambiguousWords; // used only by testing in :pennmatch
	}

	// "After hitting Sue, who is  the one person that I love among all the people I know who can walk, the ball struck a tree."
	subjectStack[MAINLEVEL] = verbStack[MAINLEVEL] = auxVerbStack[MAINLEVEL] = 0;

	// mark quotes
	if (start > 2 && *wordStarts[start-1] == '"') // ended with a quote now starting something else
	{
		if (parseFlags[start] & QUOTEABLE_VERB) // absorb quote reference into sentence
		{
			bool changed = false;
			LimitValues(start,VERB_BITS,(char*)"quotable verb start",changed);
			--start;
			--startSentence;
			// quote will already be tagged since we process quotes first
		}
	}
	else if (end < wordCount && *wordStarts[end+1] == '"') // ended with a quote now starting something else
	{
		++end;
		++endSentence;
		// quote will already be tagged since we process quotes first
	}
	idiomed = false;
	tokenFlags &= -1 ^ FAULTY_PARSE;
	if ((wordCount < 2 && posValues[start] & VERB_INFINITIVE) || !ApplyRules()) tokenFlags |= FAULTY_PARSE; // dont pos 1 or 2 word sentences.... not likely to get it right
	if (prepareMode == POSTIME_MODE) posTiming += ElapsedMilliseconds() - startTime;

	unsigned int foreign = 0;
	for (int i = start; i <= end; ++i) 
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & FOREIGN_WORD && !(posValues[i] & PART_OF_SPEECH)) foreign += 10;
	}
	bool noparse = false;
	if (foreign > 20 &&  (foreign  / (endSentence-startSentence)) >= 5) 
	{
		noparse = true;
		tokenFlags |= FOREIGN_TOKENS;
	}
	if (!noparse) SetCanonicalValue(start,end); // dont process with gesture data intruding

	if (trace & TRACE_PREPARE || prepareMode == PREPARE_MODE || prepareMode == POS_MODE) 
	{
		if (prepareMode != POS_MODE) DumpCrossReference(start,end);
		Log(STDTRACELOG,(char*)"%s",DumpAnalysis(start,end,posValues,(char*)"Tagged POS",false,true));
		DumpSentence(start,end);
	}
}

static void InitRoleSentence(int start, int end)
{
	roleIndex = 0;
    quotationRoleIndex = 0;
    quotationCounter = 0;
    int i;
    for (i = start; i <= end; ++i)
    {
        roles[i] = 0;
        indirectObjectRef[i] = 0;
        objectRef[i] = 0;
        complementRef[i] = 0;
        needRoles[0] = COMMA_PHRASE;		// a non-zero level
    }
    determineVerbal = 0;
    currentVerb2 = currentMainVerb = 0;
    predicateZone = -1;
    lastPhrase = lastVerbal = lastClause = 0;
    firstAux = NULL;
    firstnoun = firstNounClause = 0;

    verbStack[0] = 0;
    subjectStack[0] = 0;
    objectStack[0] = 0;
    auxVerbStack[0] = 0;
    startStack[0] = 0;

    verbStack[MAINLEVEL] = 0;
    subjectStack[MAINLEVEL] = 0;
    objectStack[MAINLEVEL] = 0;
    auxVerbStack[MAINLEVEL] = 0;
    needRoles[MAINLEVEL] = 0;
    startStack[MAINLEVEL] = 0;

    startStack[0] = (unsigned char)startSentence;
    objectRef[0] = objectRef[MAX_SENTENCE_LENGTH - 1] = 0;
    if (*wordStarts[start] == '"' && parseFlags[start + 1] & QUOTEABLE_VERB) // absorb quote reference into sentence
    {
        SetRole(start, MAINOBJECT, true);
    }

    if (trace & TRACE_POS) Log(STDTRACELOG, (char*)"  *** Sentence start: %s (%d) to %s (%d)\r\n", wordStarts[startSentence], startSentence, wordStarts[endSentence], endSentence);
}

void TagInit()
{
	// dynamic data
	memset(bitCounts,0,sizeof(unsigned char)*(wordCount+2));
	memset(posValues,0,sizeof(uint64)*(wordCount+2));
	memset(coordinates,0,sizeof(unsigned char) * (wordCount+4)); // conjunction loop
	memset(crossReference,0,sizeof(unsigned char) * (wordCount+4)); //link  noun to possessive,  idiom pre to idiom end, auxverb to verb,
	// determinr or adjective to noun, adverb to adjective, prep to what it modifies
	memset(phrasalVerb,0,sizeof(unsigned char) * (wordCount+4) ); // forward chain of phrasal verb

	// fixed data
	memset(originalLower,0,sizeof(WORDP)*(wordCount+2));
	memset(originalUpper,0,sizeof(WORDP)*(wordCount+2));
	memset(canonicalLower,0,sizeof(WORDP)*(wordCount+2));
	memset(canonicalUpper,0,sizeof(WORDP)*(wordCount+2));
	memset(wordRole,0,sizeof(char*) * (wordCount+2));
	memset(wordTag,0,sizeof(char*) * (wordCount+2));
	memcpy(wordCanonical,wordStarts,sizeof(char*) * (wordCount+2));
	memset(allOriginalWordBits,0,sizeof(uint64)*(wordCount+2));
	memset(lcSysFlags,0,sizeof(uint64)*(wordCount+2));
	memset(canSysFlags,0,sizeof(uint64)*(wordCount+2));
	memset(parseFlags,0,sizeof(unsigned int)*(wordCount+2));

    InitRoleSentence(1,wordCount);
}

void TagIt() // get the set of all possible tags. Parse if one can to reduce this set and determine word roles
{
#ifndef DISCARDPARSER
	roleIndex = 0; 
	prepBit = clauseBit = verbalBit = 1;
	memset(tried,0,sizeof(char) * (wordCount + 2));
	memset(phrases,0,sizeof(int) *(wordCount+4));
	memset(verbals,0,sizeof(int) *(wordCount+4));
	memset(clauses,0,sizeof(int) *(wordCount+4));
	memset(roles,0,(wordCount+4) * sizeof(uint64));
	memset(parseFlags,0,(wordCount+2) * sizeof(int));
	memset(indirectObjectRef,0,sizeof(char) *(wordCount+4)); // verb to indirect object
	memset(objectRef,0,sizeof(char) *(wordCount+4)); // verb to object
	memset(complementRef,0,sizeof(char) *(wordCount+4)); // verb to object complement
#endif

	// AssignRoles manages:  posValues+bitCounts, roles+needRoles+rolelevel, crossreference  (can we remove coordinates or complementref or objectref or indorectobjecref?), phrasal_verb?
	TagInit();
	memset(ignoreWord,0,sizeof(unsigned char) * (wordCount+4));

	wordStarts[wordCount+1] = AllocateHeap((char*)"");
	knownWords = 0;
	int i;

	// count words beginning lowercase and note which ones - also init wordcanonical, in case it never goes to postag
	lowercaseWords = 0;
	unsigned char wasLower[MAX_SENTENCE_LENGTH];
	unsigned int parens = 0;
	for (i = 1; i <= wordCount; ++i)
	{
		if (IsLowerCase(*wordStarts[i])) 
		{
			++lowercaseWords; 
			wasLower[i] = 1;
		}
		else wasLower[i] = 0;

		// ignore paren asides
		if (*wordStarts[i] == '(') 
		{
			int j;
			for (j = i + 1; j <= wordCount; ++j) if (*wordStarts[i] == ')') break;
			if (j <= wordCount) ++parens; // there is a close here
		}
		else if (*wordStarts[i] == ')' && parens) 
		{
			--parens;
			if (!parens) ignoreWord[i] = IGNOREPAREN;
			continue;
		}
		if (parens) ignoreWord[i] = IGNOREPAREN;
	}
	
	// default in case we dont find anything
	startSentence = 1;
	endSentence = wordCount;

#ifdef TREETAGGER
	ts.number_of_words = 0; // declare no data in english known to merge into our postagging code
	if (externalPostagger) (*externalPostagger)();
#endif
	if (!externalTagger && *GetUserVariable((char*)"$cs_externaltag"))
	{
		// not treetagger, just a named topic
		externalTagger = 1;
		OnceCode((char*)"$cs_externaltag");
	}

	if (externalPostagger && stricmp(language,"english")) return; // Nothing left to pos tag if done externally (by Treetagger) unless doing english

	// handle regular area
	for (i = 1; i <= wordCount; ++i)
	{
		if (ignoreWord[i]) continue; // not start of real sentence

		// see of semicolon separating sentences: except within a () block
		int j;
		for (j = i+1; j < wordCount; ++j) 
		{
			if (!ignoreWord[j] && *wordStarts[j] == ';' && !(tokenControl & NO_SEMICOLON_END) ) break;
		}
		unsigned int end;
		if (j <= wordCount && *wordStarts[j] == ';' && !(tokenControl & NO_SEMICOLON_END))
		{
			end = j - 1;
			tokenFlags &= -1 ^ SENTENCE_TOKENFLAGS; // reset results bits
			PerformPosTag(i,end); // do this zone
			i = end + 1;
			originalLower[j] = FindWord(wordStarts[j]);

            PerformPosTag(j, j); // do semicolon only
            continue;
		}

		end = wordCount;
		// last part of sentence
		for (j = wordCount; j > i; --j) // find an end
		{	
			if (!ignoreWord[j]) break;
		}
		if (j < wordCount && *wordStarts[j+1] == '"' && wasLower[j+1]) ++j;	// merge quote into current sentence since obvious intention. OTHERWISE it is a new sentence
		end = j;

		// bug - make a noun out of 1st quote if part of sentence...
		tokenFlags &= -1 ^ SENTENCE_TOKENFLAGS; // reset results bits
		PerformPosTag(i,end); // do this zone
		i = end;
	}
}

static char* OpDecode(uint64 field)
{
	static char buff[500];
	unsigned int control = (unsigned int) ( (field >> CONTROL_SHIFT) &0x01ff);
	strcpy(buff, (control & NOTCONTROL) ? (char*)"!" : (char*)"");

	uint64 bits = field & PATTERN_BITS;
	control >>= CTRL_SHIFT; // get the op
	strcat(buff, tagOps[control]);
	if (control == HAS || control == IS  || control == HASORIGINAL || control == CANONLYBE || control == INCLUDE  || control == PRIORPOS  || control == POSTPOS) PosBits(bits,buff);
	else if (control == POSITION)
	{
		if (bits == 1) strcat(buff,(char*)" START");
		else if (bits > 100) strcat(buff,(char*)" END");
	}
	else if (control == HASCANONICALPROPERTY || control == HASPROPERTY || control == ISPROBABLE || control == HASALLPROPERTIES) PropertyBits(bits,buff);
	else if (control == ISQUESTION)
	{
		if (bits & AUXQUESTION) strcat(buff,(char*)" AUX");
		if (bits & QWORDQUESTION) strcat(buff,(char*)" QWORD");	// may be a question started with a question word "how are you"
	}
	else if (control == ISCANONICAL || control == ISORIGINAL)
	{
		WORDP D = Meaning2Word((unsigned int)bits);
		strcat(buff,(char*)" ");
		if (D) strcat(buff,D->word);
		else strcat(buff, "bad bits");
	}
	return buff;
}

bool IsDeterminedNoun(int i,int& det)
{
	if (posValues[i] & ( NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL | NOUN_PLURAL | PRONOUN_BITS)) // needs no determiner
	{
		det = i;
		return true;
	}

	if (lcSysFlags[i] & NOUN_NODETERMINER)	// mass nouns need none also, same for special nouns	
	{
		det = i;
		return true;
	}

	// things like type of and sort of dont require headed noun
	if (parseFlags[i-1] & PREP_ALLOWS_UNDETERMINED_NOUN)  // of and per for example
	{
		det = i;
		return true;
	}

	// singular nouns must be determined  - "my cat, dog" is not legal but "my cat, a dog" is -- or a title like "Julie Winter president of the club" or "CEO Tom"
	// a possessived noun is also determined "John's cat"
	det = i - 1;
	while (det > 1 && posValues[det] & (ADJECTIVE_BITS|ADVERB) && !(posValues[det] & (ADJECTIVE_NUMBER|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|COMMA|CONJUNCTION))) 
	{
		--det;
	}
	return  (posValues[det] & (DETERMINER|POSSESSIVE_BITS|ADJECTIVE_NUMBER)) ? true : false;
}

static unsigned int PriorPhrasalVerb(int particle,WORDP & D)
{// testing for split particle (embedded direct object)
	char word[MAX_WORD_SIZE];
	if (particle == startSentence) return 0;
	
	// has potential object  before?
	bool objectBefore = false;

	// we now assume separated particles from verb
	int at = particle; 
	int mostRecentParticle = particle;
	while (--at)
	{
		if (posValues[at] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE)) break; //  found verb -  presumed  - but wont handle dual noun like "bank teller"
		if (posValues[at] & PARTICLE) mostRecentParticle = at;	
		if (posValues[at] & (NOUN_BITS|PRONOUN_BITS)) objectBefore = true;
		if (posValues[at] == PREPOSITION) objectBefore = false;	 // swallow any object we might have considered
		if (posValues[at] == COMMA) return 0;		 // we dont expect comma phrases to separate phrasals

	}
	if (!at) return 0; // not found anything anywhere
	if (at == (mostRecentParticle - 1)) return 0;		 // wasnt split apart from most recent particle

	// not immediately after "you make a fuss *over me" and no noun after
	// is it a particle phrase possible?
	char* verb = GetInfinitive(wordStarts[at],false);
	if (!verb) return 0;	
	
	// create full phrasal verb
	strcpy(word,verb);
	while (mostRecentParticle <= particle)
	{
		strcat(word,(char*)"_");
		strcat(word,wordStarts[mostRecentParticle]);
		++mostRecentParticle;
	}
	D = FindWord(word,0,LOWERCASE_LOOKUP);
	if (!D)  return 0;

	// never has both separable and non-separable on it at same time
	if ( (D->systemFlags & (SEPARABLE_PHRASAL_VERB | INSEPARABLE_PHRASAL_VERB)) == ( SEPARABLE_PHRASAL_VERB |  INSEPARABLE_PHRASAL_VERB))
	{
		ReportBug((char*)"%s has both separable and nonseparable\r\n",D->word);
	}

	if (D->systemFlags & INSEPARABLE_PHRASAL_VERB) return 0; // must not be separated EVER but we are only doing separated

	// any object MUST be before it (since we are separated)
	unsigned int obj = particle;
	if (posValues[obj+1] & (DETERMINER_BITS|PRONOUN_BITS|ADJECTIVE_BITS|NOUN_BITS) && bitCounts[obj+1] == 1) return 0;

	if (D->systemFlags & VERB_DIRECTOBJECT) // is there an object  before it? It can use one
	{
		if (objectBefore) return at; // BUG doesnt handle complex noun phrases
		return 0;
	}
	else if (D->systemFlags & VERB_NOOBJECT) // wants no object
	{
		return 0; // seems there is an object there and split would require it
	}
	return 0;
}

static bool NounFollows( int i,bool thruComma)
{
	while (++i <= endSentence)
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & PUNCTUATION) return false;
		if (posValues[i] & (DETERMINER|PRONOUN_BITS|NORMAL_NOUN_BITS|NOUN_NUMBER|NOUN_GERUND)) return true;
		if (posValues[i] & (CONJUNCTION_COORDINATE|COMMA)) // allowed for adjparticiple description, not for prepositino
		{
			if (thruComma) continue;
			break;
		}
		if (posValues[i] & (ADVERB|ADJECTIVE_BITS|QUOTE)) continue;
		break;
	}
	return false;
}
static int TestTag(int &i, int control, uint64 bits,int direction,bool tracex)
{
	bool notflag = false;
	static bool endwrap = false;	// allow endwrap ONLY immediately after successful end test going forward
	if (control & NOTCONTROL) notflag = true;
	bool skip = (control & SKIP) ? true : false;
	control >>= CTRL_SHIFT; // get the op
	if ( i <= 0 || i > endSentence) 
	{
		if (skip) return false;	// cannot stay any longer
		// endwrap allowed
		if (i > endSentence && endwrap && direction == 1) i = startSentence;
		else if ( i < startSentence  && endwrap && direction == -1) i = endSentence;
		else if (control == ISQUESTION) {;}
		else if (control == PRIORCANONICAL || control == PRIORORIGINAL) i = endSentence;
		else return (notflag) ? true : false;
	}
	endwrap = false;
	int answer = false;
	switch(control)
	{
		case HAS: // the bit is among those present of which there are at least 2
			answer = (posValues[i] & bits) != 0;
			break;
		case IS: // The bit is what is given --  If word is ambiguous then if fails both IS and !IS.
			if (bitCounts[i] == 1) // no ambiguity
			{
				if (posValues[i] & bits) answer = true;
			}
			else notflag = false;  // ambiguous value will be false always
			break;
		case POSITION: // FIRST LAST START END  where doublequotes can be ignored
			if ( bits == 1) 
			{
				if (i == startSentence) answer = true;
				else if ( i ==  (startSentence+1) && *wordStarts[startSentence] == '"') answer = true;
				if (answer && !notflag && direction == -1) endwrap = true;	// allowed to test wrap around end
			}
			else if ( bits > 100 && i == endSentence) 
			{
				answer = true;
				if (answer && !notflag && direction == 1) endwrap = true;	// allowed to test wrap around end
			}
			break;
		case HASORIGINAL:
			if  (allOriginalWordBits[i] & bits) answer = true;
			break;
		case PASSIVEVERB: // past particle preceeded by aux be -- only allow this in the positive sense - has no direct object
			{
				int at = i;
				while (--at >= startSentence)
				{
					if (ignoreWord[at]) continue;
					if (posValues[at] == AUX_BE) 
					{
						answer = true;
						break;
					}
					if (posValues[at] & VERB_BITS) break; // someone else may get it
				}
			}
			break;
		case HASPROPERTY: // system properties of lower case (never care about upper case properties)
			answer = (lcSysFlags[i] & bits) != 0; 
			break;
		case ISCOMPARATIVE:
			answer = (allOriginalWordBits[i] & (MORE_FORM| MOST_FORM)) != 0;
			break;
		case ISSUPERLATIVE:
			answer = (allOriginalWordBits[i] & MOST_FORM) != 0;
			break;
		case ISPROBABLE: // adjective or adverb
			{
				uint64 pos = 0;
				if (bits & ADJECTIVE_BITS) pos |= ADJECTIVE;
				if (bits & ADVERB) pos |= ADVERB;
				answer = (lcSysFlags[i] & pos) ? true : false;
				break;
			}
		case SINGULAR:	 // pronoun or noun
			if (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT)) answer = (lcSysFlags[i] & PRONOUN_SINGULAR) ? true : false;
			else if (posValues[i] & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR)) answer = true;
			else answer = false;
			break;
		case PLURAL:	 // pronoun or noun
			if (posValues[i] & (PRONOUN_SUBJECT|PRONOUN_OBJECT)) answer = (lcSysFlags[i] & PRONOUN_PLURAL) ? true : false;
			else if (posValues[i] & (NOUN_PLURAL|NOUN_PROPER_PLURAL)) answer = true;
			else answer = false;
			break;
		case HASALLPROPERTIES: // system properties of lower case (never care about upper case properties)
			answer = (lcSysFlags[i] & bits) == bits; 
			break;
		case HASCANONICALPROPERTY:
			answer = (canSysFlags[i] & bits) != 0;
			break;
		case NOTPOSSIBLEVERBPARTICLEPAIR:
			if (posValues[i+1] == PARTICLE) //known particle
			{
				char* verb = GetInfinitive(wordStarts[i], true);
				char word[MAX_WORD_SIZE*2];
				if (verb)
				{
					sprintf(word,(char*)"%s_%s",verb,wordStarts[i+1]);
					WORDP D = FindWord(word);
					if (D && D->systemFlags & PHRASAL_VERB && !(D->systemFlags & MUST_BE_SEPARATE_PHRASAL_VERB)) 
					{
						answer = true; 
					}
				}
			}
			break;
		case ISABSTRACT:
			if (allOriginalWordBits[i] & NOUN_ABSTRACT || lcSysFlags[i] & NOUN_NODETERMINER) answer = true;
			break;
		case ISORIGINAL: // original word is this
		{
			WORDP X = Meaning2Word((int)bits);
			if (!X) { Bug(); break; }
			answer = !stricmp(wordStarts[i], X->word);
			if (posValues[i - 1] == IDIOM) answer = false;	// doesnt match eg  "I scamper *out *of her way
			if (tracex) Log(STDTRACELOG, (char*)" vs %s ", X->word);
		}
			break;
		case ISQWORD:
			if (originalLower[i] && originalLower[i]->properties & QWORD) answer = true;
			break;
		case ISCANONICAL: // canonical form of word is this (we never have canonical on upper that we care about)
			{
				WORDP X =  Meaning2Word((int)bits);
				if (!X) { Bug(); break; }
				answer = canonicalLower[i] == X;
				if (posValues[i - 1] == IDIOM) answer = false;	// doesnt match
				if (tracex) Log(STDTRACELOG, (char*)" vs %s ", X->word);
			}
			break;
		case HAS2VERBS: // GLOBAL    subord conjunct will require it
			{
				int n = 0;
				for (int x = startSentence; x <= endSentence; ++x)
				{
					if (posValues[x] & VERB_BITS) 
					{
						if (notflag) ++n; // pessimise to 2 possible verb
						else if (bitCounts[x] == 1) ++n;  // KNOWN to be true
					}
				}
				if (n > 1) answer = NO_FIELD_INCREMENT;
			}
			break;
		case PRIORCANONICAL:
			{
				WORDP D = Meaning2Word((int)bits);
				if (!D) { Bug(); break; }
				if (tracex) Log(STDTRACELOG,(char*)" vs %s ",D->word);
				int x = i;
				while (--x >= startSentence)
				{
					if (ignoreWord[x]) continue;
					if (canonicalLower[x] == D) 
					{
						answer = true;
						break;
					}
				}
			}
			break;
			case PRIORORIGINAL:
			{
				WORDP D = Meaning2Word((int)bits);
				if (!D) { Bug(); break; }
				if (tracex) Log(STDTRACELOG,(char*)" vs %s ",D->word);
				int x = i;
				while (--x >= startSentence)
				{
					if (ignoreWord[x]) continue;
					if (originalLower[x] == D) 
					{
						answer = true;
						break;
					}
				}
				break;
			}
			case POSTORIGINAL:
			{
				WORDP D = Meaning2Word((int)bits);
				if (!D) { Bug(); break; }
				if (tracex) Log(STDTRACELOG,(char*)" vs %s ",D->word);
				int x = i;
				while (++x <= endSentence)
				{
					if (ignoreWord[x]) continue;
					if (originalLower[x] == D) 
					{
						answer = true;
						break;
					}
				}
			}
			break;
		case PRIORPOS: // is there a x before
			{
				int at = i;
				while (--at >= startSentence)
				{
					if (ignoreWord[at]) continue;
					if (posValues[at] & bits) break;
				}
				answer = (posValues[at] & bits) != 0;
				break;
			}
			case POSTPOS: // is there a x after
			{
				int at = i;
				while (++at  <= endSentence)
				{
					if (ignoreWord[at]) continue;
					if (posValues[at] & bits) break;
				}
				answer = (posValues[at] & bits) != 0;
				break;
			}
			case POSSIBLEADJECTIVEPARTICIPLE: // could we be adj participle  with or without conjunction- "although *dying of cancer, he"  "*dying of cancer, he"
				{
					// at sentence start if comma follows with no verb in this chunk. (so not a gerund)
					if (i == startSentence)  // Studying hard, he ran.
					{
						int j;
						for (j = i+1; j < endSentence; ++j)
						{
							if (*wordStarts[j] == ',') break; // if found a verb, might be gerund, but still possible participle
						}
						if (*wordStarts[j] == ',') answer = true;
					}
					// at sentence start if comma follows with no verb in this chunk. (so not a gerund)
					if ((i-1) == startSentence && posValues[i-1] & CONJUNCTION)  // after Studying hard, he ran.
					{
						int j;
						for (j = i+1; j < endSentence; ++j)
						{
							if (*wordStarts[j] == ',') break; // if found a verb, might be gerund, but still possible participle
						}
						if (*wordStarts[j] == ',') answer = true;
					}

					// after "as" object as adjective
					if (i > 1 && !stricmp(wordStarts[i-1],(char*)"as")) // I remember him as silly
					{
						answer = true;
						break;
					}
					if (i > 2 && !stricmp(wordStarts[i-2],(char*)"as") && posValues[i-1] & ADVERB) // I remember his as seriously silly
					{
						answer = true;
						break;
					}

					// before a noun (cannot have objects)
					if (NounFollows(i,true)) 
					{
						answer = true;
						break;
					}
					// after a noun (possible adverb in way) or comma
					int at = i;
					while (posValues[--at] & (COMMA|ADVERB))
					{
						if (posValues[at] & NORMAL_NOUN_BITS) break;
					}
					if (posValues[at] & NORMAL_NOUN_BITS)
					{
						answer = true;
						break;
					}
					at = i;
					// after verb wanting adjective object or after coord conjunction that might allow it
					while (--at >= startSentence)
					{
						if (ignoreWord[at]) continue;
						if ((i-at) > 4) break; // getting too far away
						if (posValues[at] & VERB_BITS)
						{
							if (canSysFlags[at] & VERB_TAKES_ADJECTIVE)
							{
								answer = true;
								break;
							}
							if (parseFlags[at] & FACTITIVE_ADJECTIVE_VERB)
							{
								answer = true;
								break;
							}
							if (bitCounts[at] == 1) break; // MUST BE THIS VERB and fails
						}
						if (posValues[at] & CONJUNCTION_COORDINATE)
						{
							answer = true;
							break;
						}
					}
				}
				break;
		case POSSIBLEADJECTIVE: // could we be adj 
				{
					if (!(posValues[i] & ADJECTIVE_BITS)) break; // cant be adjective anyway
					if (startSentence == endSentence) // only word
					{
						answer = true;
						break;
					}
					
					// after "as" object as adjective
					if (i > 1 && !stricmp(wordStarts[i-1],(char*)"as")) // I remember him as silly
					{
						answer = true;
						break;
					}
					if (i > 2 && !stricmp(wordStarts[i-2],(char*)"as") && posValues[i-1] & ADVERB) // I remember his as seriously silly
					{
						answer = true;
						break;
					}
					// how *pretty are your eyes?
					if ((i-1) == (int)startSentence && !stricmp(wordStarts[startSentence],(char*)"how"))
					{
						answer = true;
						break;
					}
					if (i == (int)startSentence && posValues[i] & (PUNCTUATION|COMMA))
					{
						answer = true;
						break;
					}
					// before a noun (cannot have objects)
					if (NounFollows(i,true)) 
					{
						answer = true;
						break;
					}
					if (posValues[i-1] & NORMAL_NOUN_BITS && lcSysFlags[i] & ADJECTIVE_POSTPOSITIVE)
					{
						answer = true;
						break;
					}
					if (posValues[i-1] == COMMA) // comma phrase separating noun? " my wife, that dear person, ready to handle anything, was fit."
					{
						int at = i;
						while (--at > startSentence && (posValues[at] != COMMA || ignoreWord[at])) {;}
						if (posValues[at] == COMMA && posValues[at-1] & NORMAL_NOUN_BITS)
						{
							answer = true;
							break;
						}
					}
					if (posValues[i+1] == COMMA) // before comma, part of set
					{
						int at = i+1;
						while (++at < endSentence)
						{
							if (ignoreWord[at]) continue;
							if (posValues[at] & ADJECTIVE_NORMAL) 
							{
								answer = true;
								break;
							}
							if (!(posValues[at] & (ADVERB|CONJUNCTION_COORDINATE)) ) break;
						}
					}

					// acting as noun
					if (i > 1 && (posValues[i - 1] & PRONOUN_POSSESSIVE || !stricmp(wordStarts[i - 1], (char*)"the")))  // actually only with "the" and possessive pronouns"  = "on her *own"   "the *rich eat well"
					{
						answer = true;
						break;
					}
					// after verb wanting adjective object or after coord conjunction that might allow it
					int at = i;
					while (--at >= startSentence)
					{
						if (ignoreWord[at]) continue;
						if ((i-at) > 4) break; // getting too far away
						if (posValues[at] & (VERB_BITS|NOUN_INFINITIVE))
						{
							if (canSysFlags[at] & VERB_TAKES_ADJECTIVE)
							{
								answer = true;
								break;
							}
							if (parseFlags[at] & FACTITIVE_ADJECTIVE_VERB)
							{
								answer = true;
								break;
							}
							if (bitCounts[at] == 1) break; // MUST BE THIS VERB and fails
						}
						if (posValues[at] & CONJUNCTION) // subord OR coord
						{
							answer = true;
							break;
						}
					}
				}
				break;
			case POSSIBLETOLESSVERB: // allows noun infinitive
			{
				 int verb = i;
				bool nonadverb = false;
				while (--verb >= startSentence)
				{
					if (ignoreWord[verb]) continue;
					if (posValues[verb] & VERB_BITS)
					{
						if ( canSysFlags[verb] & VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE )
						{
							answer = true;
							break;
						}
					}
					else if (posValues[verb] & TO_INFINITIVE && !nonadverb)
					{
						answer = true;
						break;
					}
					if (!(posValues[verb] & ADVERB)) nonadverb = true;
				}
				break;
			}
		case HOWSTART:
			{
				unsigned int x = startSentence;
				if (*wordStarts[x] == '"') ++x;
				answer = !stricmp(wordStarts[x],(char*)"how");
			}
			break;
		case POSSIBLEPARTICLE:
		{
			int at = i;
			while (--at >= startSentence)
			{
				if (ignoreWord[at]) continue;
				if (posValues[at] & VERB_BITS)
				{
					char* verb = GetInfinitive(wordStarts[at], true);
					char word[MAX_WORD_SIZE*2];
					if (verb)
					{
						sprintf(word,(char*)"%s_%s",verb,wordStarts[i]);
						WORDP D = FindWord(word);
						if (D && D->systemFlags & PHRASAL_VERB) 
						{
							answer = true; 
							break;
						}
					}
				}
			}
			break;
		}
		case POSSIBLEPHRASAL:
			{
				char* verb = GetInfinitive(wordStarts[i-1], true);
				char word[MAX_WORD_SIZE*2];
				if (verb)
				{
					sprintf(word,(char*)"%s_%s",verb,wordStarts[i]);
					WORDP D = FindWord(word);
					if (D && D->systemFlags & PHRASAL_VERB) answer = true; 
				}
				break;
			}
		case CANONLYBE: // has no bits we didn't mention
			answer = (posValues[i] & (-1 ^ bits)) == 0;
			break;
		case ISMEMBER:  case ISORIGINALMEMBER:// direct member...
			{
				WORDP D = Meaning2Word((int)bits);
				if (!D) { Bug(); break; }
				if (tracex) Log(STDTRACELOG,(char*)" vs %s ",D->word);
				FACT* F = GetObjectNondeadHead(D);
				MEANING M = (control == ISMEMBER) ? MakeMeaning(canonicalLower[i]) : MakeMeaning(originalLower[i]);
				while (F)
				{
					if (F->subject == M)
					{
						answer = true;
						break;
					}
					F = GetObjectNondeadNext(F);
				}
			}
			break;
		case INCLUDE:
			answer = posValues[i] & bits && posValues[i] & (-1LL ^ bits); // there are bits OTHER than what we are testing for
			break;
		case POSSIBLEINFINITIVE: // ways we could be infinitive - tested for negative to erase... and if it needs object does one exist
			{
                if (!stricmp(wordStarts[i], "get") && i == 1)
                {
                    answer = true;
                    break; // command get
                }
				int at;
				if (!(canSysFlags[i] & VERB_NOOBJECT)) // see if it lacks a needed object -- bug for "*carry it off" since idiom
				{
					for (at = i+1; at <= endSentence; ++at)
					{
						if (posValues[at] & (NOUN_BITS|PRONOUN_OBJECT)) break;
					}
					if (at > endSentence)
					{
						answer = false;
						break;
					}
				}

				// if there is aux verb tense before or a verb taking infinitive
				for (at = i-1; at >= startSentence; --at)
				{
					if (allOriginalWordBits[at] & (AUX_VERB_TENSES|AUX_DO)) break;
					if (canSysFlags[at] & (VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE | VERB_TAKES_VERBINFINITIVE)) break;
				}
				if (allOriginalWordBits[at] & (AUX_VERB_TENSES|AUX_DO) || canSysFlags[at] & (VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE | VERB_TAKES_VERBINFINITIVE)) 
				{
					answer = true;
					break;
				}
				if (i > 1 && !stricmp(wordStarts[i-1],(char*)"not")) // why not go?  why, then, not put it off
				{
					answer = true;
					break;
				}
				at = i;
				while (posValues[--at] == ADVERB || posValues[at] == COMMA) {;}  // allow adverb before the infinitive
				if (at > 0 && (!stricmp(wordStarts[at],(char*)"except") || !stricmp(wordStarts[at],(char*)"but") || !stricmp(wordStarts[at],(char*)"why") || !stricmp(wordStarts[at],(char*)"than") || !stricmp(wordStarts[at],(char*)"sooner") || !stricmp(wordStarts[at],(char*)"rather")|| !stricmp(wordStarts[at],(char*)"better"))) // why, would rather, would sooner, rather than, had better"
				{
					answer = true;
					break;
				}
				if (posValues[i-1] & AUX_BE && posValues[i-2] & AUX_DO) //  after  "do be" eg did was call. do is wait and see
				{
					answer = true;
					break;
				}
				// after comma or and if infinitve comes before somewhere
				bool baseseen = false;
				for (at = i-1; at >= startSentence; --at)
				{
					if (posValues[at] & (COMMA | CONJUNCTION_COORDINATE)) baseseen = true;
					if (at > 0 &&  *wordStarts[at] == ';' || *wordStarts[at] == ':')  baseseen = true;
					if (posValues[at] & (VERB_INFINITIVE|NOUN_INFINITIVE) && baseseen) 
					{
						answer = true;
						break;
					}
				}
				if (answer) break;
				
				// simple imperative with no subject before? how can we tell unless we are close to front
				if (i > 1 && ((i - startSentence) < 4 || posValues[i - 1] == COMMA || *wordStarts[i - 1] == ';' || *wordStarts[i - 1] == ':')) // "here, ned, *catch this ball"
				{
					answer = true;
					break;
				}
			}
			break;
		case PARSEMARK:
			if (parseFlags[i] & bits) answer = true;
			break;
		case ISEXCLAIM:
			if (tokenFlags & EXCLAMATIONMARK) answer = NO_FIELD_INCREMENT;
			break;
	
		case ISQUESTION: // GLOBAL   COULD this be a question (does not insure that it is, only that it could be)
			{
				if (tokenFlags & QUESTIONMARK) answer = NO_FIELD_INCREMENT;
				else if (bits & AUXQUESTION && posValues[startSentence] & AUX_VERB) answer = NO_FIELD_INCREMENT;	// MAY BE A QUESTION started by aux "did you go"
				else if (bits & QWORDQUESTION && allOriginalWordBits[startSentence] & QWORD) answer = NO_FIELD_INCREMENT;	// may be a question started with a question word "how are you"
				else if (bits & QWORDQUESTION && allOriginalWordBits[startSentence+1] & QWORD && allOriginalWordBits[startSentence] & (PREPOSITION|ADVERB)) answer = NO_FIELD_INCREMENT;	// may be a question started with a question word "for whom  are you"
			}
			break;
		default: // unknown control
			return UNKNOWN_CONSTRAINT;
	}
	if (notflag) answer = !answer;
	return answer;
}

static  char* BitLabels(uint64 properties)
{
	static char buffer[MAX_WORD_SIZE];
	*buffer = 0;
	char* ptr = buffer;
	uint64 bit = START_BIT;		//   starting bit
	while (properties)
	{
		if (properties & bit)
		{
			properties ^= bit;
			char* label = FindNameByValue(bit);
			sprintf(ptr,(char*)"%s+",label);
			ptr += strlen(ptr);
		}
		bit >>= 1;
	}
	if (ptr > buffer) *--ptr = 0;
	return buffer;
}

static bool ApplyRulesToWord( int j,bool & changed)
{
	bool keep;
	int offset;
	int start;
	int direction;
	uint64* data;
	bool tracex;
	char* word = wordStarts[j];
	//  do rules to reduce flags
	for (unsigned int i = 0; i < tagRuleCount; ++i)
	{
		if (ignoreRule >= 0 && (int)i == ignoreRule) continue;
		data = tags + (i * MAX_TAG_FIELDS);
		uint64 basic = *data;
		char* mycomment = 0;
		if (comments) mycomment = comments[i];
		if (basic == 0) continue; // skip over change of file header

		// reasons this rule doesnt apply
		if (basic & PART2_BIT) continue; // skip 2nd part
		unsigned int resultOffset = (basic >> RESULT_SHIFT) & 0x0007; 
		uint64 resultBits = data[resultOffset] & PATTERN_BITS;	// we will want to change THIS FIELD
		if (!(posValues[j] & resultBits) || !(posValues[j] & (-1LL ^ resultBits)) ) continue;		// cannot help  this- no bits in common or all bits would be kept or erased

		char* comment = (comments) ? comments[i] : NULL;
		tracex = (data[3] & TRACE_BIT) && (trace & TRACE_POS); 
		if (tracex) 
			Log(STDTRACELOG,(char*)"   => Trace rule:%d   %s (%d) %s \r\n",i,word,j,comment);
		
		unsigned int limit = (data[3] & PART1_BIT) ? (MAX_TAG_FIELDS * 2) : MAX_TAG_FIELDS;
		offset = ((data[1] >> OFFSET_SHIFT) & 0x00000007) - 3; // where to start pattern match relative to current field (3 bits)
		keep =  (data[1] & KEEP_BIT) != 0;
		direction =  (data[2] & REVERSE_BIT) ? -1 : 1;
		offset *= direction;	
		start = j + offset - direction;
		unsigned int k;
		int result;
		for (k = 0; k < limit; ++k) // test rule fields
		{
			uint64 field = data[k];
			unsigned int control = (unsigned int) ( (field >> CONTROL_SHIFT) &0x01ff);
			if (control && !(control & STAY)) // there is a rule and it is not stay marked
			{
				start += direction;
				while (start > 0 && start <= wordCount && posValues[start] == IDIOM) start += direction; // move off the idiom
			}
			if (k == resultOffset) result = 1; // the result include is known to match if we are here
			else if (control) // if we care
			{
				if (control & SKIP) // check for any number of matches until fails.
				{
					result = TestTag(start,control,field & PATTERN_BITS,direction,tracex); // might change start to end or start of PREPOSITIONAL_PHRASE if skipping
					if (tracex) Log(STDTRACELOG,(char*)"    =%d  SKIP %s(%d) op:%s result:%d\r\n",k,wordStarts[start],start, OpDecode(field),result);
					if ( result == UNKNOWN_CONSTRAINT)
					{
						ReportBug((char*)"unknown control %d) %s Rule #%d %s\r\n",j,word,i,comment)
						result = false;
					}
					while (result) 
					{
						start += direction;
						while (posValues[start] == IDIOM) start += direction; // move off the idiom
						result = TestTag(start,control,field & PATTERN_BITS,direction,tracex);
						if (tracex ) 
							Log(STDTRACELOG,(char*)"    =%d  SKIP %s(%d)  result:%d @%s(%d)\r\n",k,wordStarts[start],start,result,wordStarts[start],start);
					}
					start -= direction; // ends on non-skip, back up so will see again
					while (posValues[start] == IDIOM) start -= direction; // move off the idiom
				}
				else if ((control >> CTRL_SHIFT) == RESETLOCATION)  // reset to include offset
				{
					if (field & PATTERN_BITS) direction = - direction; // flip scan direction
					start = j; // be back on the word as field
					if (tracex) Log(STDTRACELOG,(char*)"    =%d  RESETLOCATION %d\r\n",k,direction);
				}
				else
				{
					result = TestTag(start,control,field & PATTERN_BITS,direction,tracex); // might change start to end or start of PREPOSITIONAL_PHRASE if skipping
					if (tracex && k && k <= wordCount)
						Log(STDTRACELOG,(char*)"    =%d  %s(%d) op:%s result:%d\r\n",k,wordStarts[start],start,OpDecode(field),result);
					if (result == UNKNOWN_CONSTRAINT)
					{
						ReportBug((char*)"unknown control1 %d) %s Rule #%d %s\r\n",j,word,i,comment)
						result = false;
					}
					if (result == NO_FIELD_INCREMENT) start -= direction; // DONT MOVE
					if (!result) break;	// fails to match // if matches, move along. If not, just skip over this (used to align commas for example)
				}
			}
		} // end test on fields

		// pattern matched, apply change if we can
		if (k >= limit) 
		{
			if (tracex) Log(STDTRACELOG,(char*)"   <= matched\r\n");
			uint64 old = posValues[j] & ((keep) ? resultBits : ( -1LL ^ resultBits));
			if (data[3] & USED_BIT && usedTrace) 
			{
				sprintf(usedTrace,(char*)"USED: line: %d => Rule:%d %s  Sentence: ",currentFileLine,i,comment);
				for (int x = 1; x <= wordCount; ++x) 
				{
					if (j == x) 
					{	
						strcat(usedTrace,(char*)"*"); // mark word involved
						usedWordIndex = j;
						usedType =  resultBits;
					}
					strcat(usedTrace,wordStarts[x]);
					strcat(usedTrace,(char*)" ");
				}
				strcat(usedTrace,(char*)"\r\n");
			}

			if (trace & TRACE_POS && (prepareMode == POS_MODE || tmpPrepareMode == POS_MODE || prepareMode == POSVERIFY_MODE || prepareMode == PENN_MODE))
			{
				char* which = (keep) ? (char*) "KEEP" : (char*)"DISCARD";
				uint64 properties = old; // old is what is kept
				if (!keep) properties = posValues[j] - old;	// what is discarded
				if (!properties) ReportBug((char*)"bad result in pos tag %s\r\n",comment) // SHOULDNT HAPPEN
					
				char* name = FindNameByValue(resultBits);
				if (!name) name = BitLabels(properties);
				Log(STDTRACELOG,(char*)" %d) %s: %s %s Rule #%d %s\r\n",j,word,which,name,i,comment+1);
				char buff[MAX_WORD_SIZE];
				*buff = 0;
				PosBits(old,buff);
				Log(STDTRACELOG,(char*)"   now %s\r\n",buff);
			}

			// make the change
			posValues[j] = old;
			bitCounts[j] = BitCount(old);
			changed = true;
			if (bitCounts[j] == 1)
			{
				--ambiguous;
				return true;	// we now know
			}
		} // end result change k > limit
		else if (tracex) Log(STDTRACELOG,(char*)"   <= unmatched\r\n"); // pattern failed to match
	} // end loop on rules 
	return false;
}

static void SetIdiom(int at, int end)
{
	posValues[at] = IDIOM;
	allOriginalWordBits[at] = IDIOM;
	bitCounts[at] = 1;
	crossReference[at] = (unsigned char)end;
}

unsigned int ProcessIdiom(char* word, int i, unsigned int words,bool &changed)
{
	WORDP D = FindWord(word,0,LOWERCASE_LOOKUP);
	if (!D) D = FindWord(word,0,UPPERCASE_LOOKUP);
	// WE DONT HANDLE "walk through" by checking it must be a verb?
	if (!D){;}
	else if (D->systemFlags & PHRASAL_VERB) // contiguous phrasal verb
	{
		unsigned int verb = i - words + 1;
		if (!(posValues[verb] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE))) return 0; // cannot accept phrasal verb, its not used as a verb

		// dont want "as factors in the loss" to be phrasal verb, since verb makes no sense
		if (!(posValues[verb-1] & (NOUN_BITS|PRONOUN_BITS)) && stricmp(wordStarts[verb-1],(char*)"and") && stricmp(wordStarts[verb - 1], (char*)"&") && stricmp(wordStarts[verb-1],(char*)"or") )
		{
			return 0;
		}

		if (posValues[i] & TO_INFINITIVE && posValues[i+1] & NOUN_INFINITIVE) // could this be to infinitive
		{
			LimitValues(i,-1 ^ PARTICLE,(char*)"particle loses to To infinitive",changed);
			return 0;
		}

		// see if following it is an object
		int next = i;
		while (++next < endSentence)
		{
			if (ignoreWord[next]) continue;
			if (posValues[next] & (DETERMINER_BITS|NOUN_BITS|ADJECTIVE_BITS|PRONOUN_BITS)) break; // optimistic
			if (posValues[next] != ADVERB &&  posValues[next] != QUOTE) break;
		}

		bool uncertainObject = false;
	
		// verb wants gerund, not object
		if (D->systemFlags & VERB_TAKES_GERUND && !(D->systemFlags & (VERB_NOOBJECT| VERB_DIRECTOBJECT)))  // "keep on singing"
		{
			if (! (posValues[i+1] & NOUN_GERUND) ) return 0;
		}

		// there appears to be an object, can we use it? 
		else if (posValues[next] & (DETERMINER_BITS|NOUN_BITS|ADJECTIVE_BITS|PRONOUN_BITS))
		{
			if (!(D->systemFlags & VERB_DIRECTOBJECT)) return 0;	// we cant use it
			if (!stricmp(wordStarts[startSentence],(char*)"what") || !stricmp(wordStarts[startSentence],(char*)"which")) return 0; // already will have one from question
		}
		// there appears to be no object, did we require one
		else if (!(D->systemFlags & VERB_NOOBJECT)) 
		{
			// maybe there is a preceeding that or what forming a clause and being our object
			int at = i;
			while (--at > startSentence)
			{
				if (ignoreWord[next]) continue;
				if (allOriginalWordBits[at] & PRONOUN_BITS && (!stricmp(wordStarts[at],(char*)"that") || !stricmp(wordStarts[at], "what"))) uncertainObject = true;
			}
			if (!uncertainObject) return 0;	// we can find no possible object
		}

		if (uncertainObject)
		{
			for (int x = verb + 1; x <= i; ++x) phrasalVerb[x-1] = (unsigned char)x;  // mark all the particles  - they remain ambiguous until we confirm clause reference
		}
		else
		{ // expand the possibilities of what it could be and what comes after as particle as well.
			LimitValues(verb,NOUN_BITS|VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE,(char*)"designated sequential phrasal verb or noun still possible",changed);
			for (int x = verb + 1; x <= i; ++x) 
			{
				posValues[x] = PARTICLE; // we'd have to see if noun infinitive was after // can be particle whatever it is, part of verb
				bitCounts[x] = 1;
				phrasalVerb[x-1] = (unsigned char)x; // forward pointer
			}
		}
		phrasalVerb[i] = (unsigned char)verb; // loop back
		
		// revise objects of verb if this is to be a verb it will be particle verb
		canSysFlags[verb] &= -1 ^ (VERB_DIRECTOBJECT|VERB_NOOBJECT);
		canSysFlags[verb] |= D->systemFlags & (VERB_DIRECTOBJECT|VERB_NOOBJECT);

		return words;
	}
	else if (D->systemFlags & CONDITIONAL_IDIOM) // eg. more_than -  ?[AR1]=R ?=<Rp+s	#	if followed by  number or adjective or adverb, its an adverb "more than 20", if if followed by noun/pronoun, its words "more than human" as adverb and preposition/subordconjunct
	{
		char* script = D->w.conditionalIdiom;
		bool valid = true;
		bool invert = false;
		bool invertor = false;
		bool orOp = false;
		unsigned int before = false;
		while (script && *script)
		{
			int at = NextPos(i);
			uint64 val = posValues[at];
			switch(*script++)
			{
			case '?' : valid  = true; continue;
			case '!' : invert = true; continue;
			case '_' : continue; // just a visual spacer
			case '[' : orOp = true; if (invert) { invertor = true; invert = false; } continue;
			case ']' :
				orOp = false;
				valid = false;
				if (invertor)
				{ 
					invertor = false;
					invert = true;
				}
				break;
			case '>' : // end of sentence
				if (i != endSentence) valid = false;
				break;
			case '-' : 
				if (i > startSentence)
				{
					before = i-1; // start with checking word before
					at = before;
					val = posValues[before];
					continue;
				}
				else
				{
					valid = false;
					break;
				}
			case 'N' : // check noun
				if (!(val & NOUN_BITS)) valid = false;
				break;
			case 'P' :// check pronoun
				if (!(val & PRONOUN_BITS)) valid = false;
				break;
			case  'V' : // check verb and aux
				if (!(val & (VERB_BITS | AUX_VERB))) valid = false;
				break;
			case  'A' :
				if (!(val & ADJECTIVE_BITS)) valid = false;
				break;
			case 'R' :
				if (!(val & ADVERB)) valid = false;
				break;
			case 'T':
				if (!(val & TO_INFINITIVE)) valid = false;
				break;
			case  'd' :
				if (!(val & DETERMINER_BITS)) valid = false;
				break;
			case  'p' : // should be preposition
				if (!(val & PREPOSITION)) valid = false;
				break;
			case  'e' :
				if (!(val & PARTICLE)) valid = false;
				break;
			case  's' :
				if (!(val & CONJUNCTION_SUBORDINATE)) valid = false;
				break;
			case  'c' :
				if (!(val & CONJUNCTION_COORDINATE)) valid = false;
				break;
			case  '1' :
				if (canonicalLower[at] && IsDigit(*canonicalLower[at]->word)) {;} else valid = false;
				break;
			case  '$' :
				if (!(val & CURRENCY)) valid = false;
				break;
			case  'w' : // is it this word
				{
				char* end = strchr(script,'.');
				valid = !invert;
				*end = '.';
				script = end - 1; // BUGGY???
				break;
				}
			case  '=' : // matched, set this pos unit or set all words
				{
				uint64 bits;
				unsigned int spread;
				if (*script == '<') // force spread answer, not idiom
				{
					spread = 1;
					i = i - words + 1; // back at the start, doing each word
				}		
				else
				{
					spread = 0;
					--script;
				}
				bits =  posValues[i];
				posValues[i] = 0;	// fresh slate
				bitCounts[i] = 0;
				while (*++script)
				{
					if (!*script || *script == '?' || *script == '_') break; // end of assignment zone
					else if (*script == '*')  posValues[i] = bits; 
					else if (*script == 'w')  // force this word (presumably to capitalize)
					{
						char* end = strchr(script,'.');
						if (!end) // bug
						{
							ReportBug((char*)"Bad Script %s\r\n",script);
							break; 
						}
						*end = 0;
						WORDP X = StoreWord(script+1,NOUN_PROPER_SINGULAR);
						*end = '.';
						canonicalUpper[i] = X;
						canonicalLower[i] = NULL;
						originalUpper[i] = X;
						originalLower[i] = NULL;
						wordStarts[i] = X->word;
						if (X->internalBits & UPPERCASE_HASH)
						{
							allOriginalWordBits[i] = posValues[i] = NOUN_PROPER_SINGULAR;
							bitCounts[i] = 1;
						}
						script = end;
					}
					else if (*script == 'N') posValues[i] |= NOUN_SINGULAR;
					else if (*script == 'T') posValues[i] |= TO_INFINITIVE;
					else if (*script == 'P') posValues[i] |= PRONOUN_OBJECT;
					else if (*script == 'R') posValues[i] |= ADVERB;
					else if (*script == 'A') posValues[i] |= ADJECTIVE_NORMAL;
					else if (*script == 'p') posValues[i] |= PREPOSITION;
					else if (*script == 'e') posValues[i] |= PARTICLE;
					else if (*script == 's') posValues[i] |= CONJUNCTION_SUBORDINATE;
					else if (*script == 'c') posValues[i] |= CONJUNCTION_COORDINATE;
					else if (*script == 'd') posValues[i] |= DETERMINER;
					else if (*script == '|') {;}
					else 
						ReportBug((char*)"Bad conditional idiom script %s %s at %s\r\n",D->word,D->w.conditionalIdiom,script);
					bitCounts[i] = BitCount(posValues[i]);
	
					if (spread) 
					{
						if (script[1] == '+') 
						{
							++script;
							continue; // allow alternative on this choice
						}
						bitCounts[i] = BitCount(posValues[i]);
						if (!script[1] || script[1] == '?' || script[1] == '_') break; // done
						bits = posValues[++i];
						posValues[i] = 0; // on to next item, cleared and ready
						bitCounts[i] = 0;
					}
				}
				if (!spread) // we keep it as an idiom, not as words
				{
					if (posValues[i]) // we assigned it a result
					{
						unsigned int tagged = words;
						while (--tagged) SetIdiom(i-tagged,i); // default mark as idiom
					}
					else // we were unable to assign a bit - what we wanted to assign had already been discounted by the system  eg "in" can be adjective and/or prep before a noun, system decided adj earlier, and now cant assign prep
					{
						posValues[i] = bits;	// restore old stuff
						bitCounts[i] = BitCount(bits);
						return 0;
					}
				}
				changed = true;
				return words; // done when we find 1 thing to set with
				}
			}
			before = false;
			if (invert) valid = !valid;
			if (orOp)
			{
				if (!valid) valid = true; // just move on to next check
				else // found an answer, close out the or
				{
					script = strchr(script,']') + 1; // close the or
					orOp = false;
					if (invertor) valid = false;
					invertor = invert = false;
				}
			}
			else invert = false; // no ! inside an or, ! covers scope of or
			if (!valid) 
			{
				script = strchr(script,'?'); // find start of next chunk that we might match
				if (!script) return 0;	// no more
				orOp = false;
				invert = false;
			}
		}
	}
	else if (!IS_NEW_WORD(D) && D->properties & (PREPOSITION|ADVERB|CONJUNCTION_SUBORDINATE|CONJUNCTION_COORDINATE|ADJECTIVE|PRONOUN_OBJECT|PRONOUN_SUBJECT) && !(posValues[i] == PARTICLE)) // ordinary ore-exustubg multiword we will consider -- "taken up" as particle verb detected, dont allow join here
	{
		if (i  == endSentence) posValues[i] = D->properties & TAG_TEST;
		else if (D->properties & CONJUNCTION && !(posValues[NextPos(i)] & SIGNS_OF_NOUN_BITS) &&  !(posValues[Next2Pos(i)] & SIGNS_OF_NOUN_BITS) ) posValues[i] = D->properties & TAG_TEST;
		else if (D->properties & CONJUNCTION && posValues[NextPos(i)] & SIGNS_OF_NOUN_BITS) posValues[i] = D->properties & (PREPOSITION|CONJUNCTION); // restriction
		else posValues[i] =  D->properties & TAG_TEST;
		allOriginalWordBits[i] |= D->properties & TAG_TEST; // everything it might be in addition -
		canSysFlags[i] = D->systemFlags;
		bitCounts[i] = BitCount(posValues[i]);
		unsigned int tagged = words;
		while (--tagged) SetIdiom(i-tagged,i); // default mark as idiom
		changed = true;
		return words;
	}
	return 0;
}

static void WordIdioms(bool &changed)
{
	// "a little" can be an adverb if not preceeding a noun. in front of adjective, prep or other, it is adverb
	

	// manage 2-5-word idioms looking forwards - adverbs, preps, conjunction subordinate,and contiguous phrasal verbs
	// because "in place" and "in place of" looking backwards start at different places, we need to look forward at the longest instead
	for (int i = startSentence; i <= endSentence; ++i)
	{
		// check for dates
		int start,end;
		if (DateZone(i,start,end))
		{
			int at = start - 1;
			while (++at < end) SetIdiom(at,end); // default mark as idiom
			changed = true;
			posValues[end] = NOUN_PROPER_SINGULAR;
			bitCounts[end] = 1;
			i = end;
			continue;
		}

		if (capState[i] && i != startSentence && !(tokenControl & ONLY_LOWERCASE)) continue; // no upper case idioms (except dates)
		char* base = wordStarts[i];
		char word[MAX_WORD_SIZE * 10]; // any single word will be < max word size
		unsigned int diff =  endSentence - i ;   
		int tagged = 0;
		char* verb = NULL;

		// date idioms
		
		if (posValues[i] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE)) // go for sequential phrasal verb
		{
			verb = GetInfinitive(base,true);
			if (verb) base = verb;
		}

		// detect longest possible idioms first - in adverb/conjunction conflicts, try to prove out status
		if (diff > 3) // 5 word idioms
		{
			sprintf(word,(char*)"%s_%s_%s_%s_%s",base,wordStarts[i+1],wordStarts[i+2],wordStarts[i+3],wordStarts[i+4]);
			tagged = ProcessIdiom(word,i+4,5,changed);
		}
		if (!tagged && diff > 2) // 4 word idioms
		{
			sprintf(word,(char*)"%s_%s_%s_%s",base,wordStarts[i+1],wordStarts[i+2],wordStarts[i+3]);
			tagged = ProcessIdiom(word,i+3,4,changed);
		}

		if (!tagged && diff > 1) // 3 word idioms
		{
			sprintf(word,(char*)"%s_%s_%s",base,wordStarts[i+1],wordStarts[i+2]);
			tagged = ProcessIdiom(word,i+2,3,changed);
		}

		if (!tagged && diff > 0) // 2 word idioms
		{
			sprintf(word,(char*)"%s_%s",base,wordStarts[i+1]);
			tagged = ProcessIdiom(word,i+1,2,changed);
		}
		// "step by step" idiom
		if (!tagged && i > 1 && !stricmp(wordStarts[i],(char*)"by") && posValues[i-1] & NOUN_SINGULAR && i <= (wordCount-1) )
		{
			if (!stricmp(wordStarts[i-1],wordStarts[i+1])) 
			{
				tagged = 1;
				SetIdiom(i-1, i);
				posValues[i] = PREPOSITION;
				bitCounts[i] = 1;
				posValues[i+1] = NOUN_SINGULAR;
				bitCounts[i+1] = 1;
				--i; // use word before
			}
			else if (i > 1 && i < wordCount && !stricmp(wordStarts[i-1],wordStarts[i+2])) 
			{
				tagged = 2;
				SetIdiom(i-1, i);
				posValues[i] = PREPOSITION;
				bitCounts[i] = 1;
				posValues[i+2] = NOUN_SINGULAR;
				bitCounts[i+2] = 1;
				--i; // use word before
			}
		}

		if (!tagged && bitCounts[i] != 1  && lcSysFlags[i] & CONDITIONAL_IDIOM) // for single words, only if they are marked. And they should only be marked if generic rules cant handle it
		{
			strcpy(word,base);
			tagged = ProcessIdiom(word,i,1,changed);
		}
		if (tagged)
		{
			if (trace & TRACE_POS)  
			{
				char tags[MAX_WORD_SIZE];
				*tags = 0;
				char* ptr = tags;
				int at = i;
				while (at < (i + tagged)) 
				{
					Tags(ptr,at++);
					strcat(ptr,(char*)" ");
					ptr += strlen(ptr);
				}
				Log(STDTRACELOG,(char*)"Word Idiom %d:%s (%s)\r\n",tagged,word,tags);
			}
			i += tagged - 1;	// skip over matched zone
//			if (posValues[i] == PARTICLE) --i;	// rescan for another idiom like "I live *on my own" - "live on" and "on my own"
		}
		else if (posValues[i] & PARTICLE && !(posValues[i-1] & VERB_BITS)) // consider discontiguous phrasal verb (though it looks at both)  - can be multiple discontiguous like "take x for granted"
		{
			// deal with 1 or 2 word separations like "take x *for *granted"
			WORDP D = NULL;
			unsigned int priorPhrasalVerb = 0;
			bool longTail = false;
			if (posValues[i+1] & PARTICLE)
			{
				priorPhrasalVerb = PriorPhrasalVerb(i+1,D);
				if (priorPhrasalVerb) longTail = true; 
			}
			if (!priorPhrasalVerb) priorPhrasalVerb = PriorPhrasalVerb(i,D); // try on middle word
			if (!priorPhrasalVerb)
			{
				// if we dont recognize the verb but it can only be a particle, we are stuck with it: formerly "I hate what I have to contend *with, but I will survive"
				if (posValues[i] != PARTICLE) 
					LimitValues(i,-1 ^ PARTICLE,(char*)"verb before potential particle cant have one, dont be one",changed); 
			}
			else 
			{
				if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"%s ",D->word);
				phrasalVerb[priorPhrasalVerb] = (unsigned char)i;
				if (longTail)
				{
					phrasalVerb[i] = (unsigned char)(i+1);
					phrasalVerb[i+1] = (unsigned char)priorPhrasalVerb;
				}
				else phrasalVerb[i] = (unsigned char)priorPhrasalVerb;
				if (bitCounts[priorPhrasalVerb] == 1) 
					LimitValues(i,PARTICLE,(char*)"verb before potential particle has one, be particle",changed);
	
				// revise objects of verb to be that of phrasal
				canSysFlags[priorPhrasalVerb] &= -1 ^ (VERB_DIRECTOBJECT|VERB_NOOBJECT);
				canSysFlags[priorPhrasalVerb] |= D->systemFlags & (VERB_DIRECTOBJECT|VERB_NOOBJECT);
			}
		}
	}
}

static void GuessOnProbability(bool &changed) 
{
	if (1) return; // doesnt work well
	// test all items in the sentence
	for (int wordIndex =  startSentence; wordIndex <= endSentence; ++wordIndex)
	{
		if (ignoreWord[wordIndex]) continue;
		int i = (reverseWords) ? (endSentence + 1 - wordIndex) : wordIndex; // test from rear of sentence forwards to prove rules are independent
		if (bitCounts[i] != 1)  
		{
			uint64 bits = 0;
			WORDP D = canonicalLower[i];
			if (!D || !(D->systemFlags & ESSENTIAL_FLAGS)) continue; // we cant help. no known probabilities
			if (D->systemFlags & NOUN) bits |= posValues[i] & (NOUN_BITS - NOUN_INFINITIVE - NOUN_GERUND); 
			if (D->systemFlags & VERB) bits |= posValues[i] & (AUX_VERB| VERB_BITS | NOUN_INFINITIVE| NOUN_GERUND);
			if (D->systemFlags & ADJECTIVE) bits |= posValues[i] & (ADJECTIVE_BITS);
			if (D->systemFlags & ADVERB) bits |= posValues[i] & (ADVERB);
			if (D->systemFlags & PREPOSITION) bits |= posValues[i] & (PREPOSITION|CONJUNCTION);
			if (bits && bits != posValues[i]) 
			{
				posValues[i] = bits;
				bitCounts[i] = BitCount(i);
				changed = true;
			}
		}
	} // end loop on words
}

static bool ApplyRules() // get the set of all possible tags.
{
	if (!tags) return true;
	if (trace & TRACE_POS && prepareMode != PREPARE_MODE) Log(STDTRACELOG,(char*)"%s",DumpAnalysis(startSentence,endSentence,posValues,(char*)"\r\nOriginal POS:",true,true));
	unsigned int pass = 0;
	unsigned int limit = 50;
retry:
	roleIndex = 0;
	ambiguous = 1;
	bool resolved = false;
	bool parsed = false;
	bool changed = true;
	while (changed && ambiguous) // while we have something different and something not resolved
	{
		parsed = false;
		ambiguous = 0;
		for (int i =  startSentence; i <= endSentence; ++i) if (bitCounts[i] != 1) ++ambiguous;
		++pass;
		changed = false;
		if (--limit == 0) 
		{
			if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"ApplyRules overran\r\n");
			return false;
		}
		if (trace & TRACE_POS) 
			Log(STDTRACELOG,(char*)"\r\n------------- POS rules pass %d: \r\n",pass);

		// test all items in the sentence
		for (int wordIndex =  startSentence; wordIndex <= endSentence; ++wordIndex)
		{
			if (ignoreWord[wordIndex]) continue;
			int j = (reverseWords) ? (endSentence + 1 - wordIndex) : wordIndex; // test from rear of sentence forwards to prove rules are independent
			if (bitCounts[j] != 1)  
				ApplyRulesToWord(j,changed);
			else if (trace & TRACE_TREETAGGER && wordIndex == endSentence) // purely for display on trace
			{
#ifdef TREETAGGER
				if (ts.number_of_words && ttLastChanged <= endSentence) BlendWithTreetagger(changed); // can override us even when we know
#endif
			}
		} // end loop on words
		
		if (ambiguous && trace & TRACE_POS && changed) Log(STDTRACELOG,(char*)"\r\n%s",DumpAnalysis(startSentence,endSentence,posValues,(char*)"POS",false,false));
#ifdef TREETAGGER
		if (!changed && ts.number_of_words  && ttLastChanged <= endSentence) BlendWithTreetagger(changed); // can override us even when we know
		if (changed) continue;
#endif	
		if (!changed && ambiguous) // no more rules changed anything and we still need help
		{
			if (!(tokenControl & (DO_PARSE-DO_POSTAG))) // guess based on probability
			{
				GuessOnProbability(changed);
				if (changed) continue;
			}

#ifndef DISCARDPARSER
			bool modified = false;
			ParseSentence(resolved,modified);
			parsed = true;
			if (resolved) break; // we completed
			else if (modified) 
			{
				parsed = false;
				changed = true;
				ambiguous = 1; // force it to continue
			}
			else if (!modified && ambiguous) break; // nothing else we can do, quit
#endif
		}
	}
#ifdef TREETAGGER
	if (ts.number_of_words && ttLastChanged <= endSentence) BlendWithTreetagger(changed); // can override us even when we know
#endif	
#ifndef DISCARDPARSER
	bool modified = false;
	if (!parsed)  ParseSentence(resolved,modified); 
	if (modified && !resolved) goto retry;
#endif
	return resolved;
}

static void Showit(char* buffer, const char* what,uint64 bits)
{
	if (bits) strcat(buffer,(char*)"+");
	strcat(buffer,what);
}

static void ParseFlags0(char* buffer, int i)
{
#ifndef DISCARDPARSER
	if (parseFlags[i] & FACTITIVE_ADJECTIVE_VERB) strcat(buffer,(char*)"factitive_adjective_verb ");
	if (parseFlags[i] & FACTITIVE_NOUN_VERB) strcat(buffer,(char*)"factitive_noun_verb ");
	if (parseFlags[i] & QUOTEABLE_VERB) strcat(buffer,(char*)"quotable_verb ");
	if (parseFlags[i] & ADJECTIVE_TAKING_NOUN_INFINITIVE) strcat(buffer,(char*)"adjective_complement_verb ");
	if (parseFlags[i] & OMITTABLE_THAT_VERB) strcat(buffer,(char*)"omittable_that_verb ");
	if (parseFlags[i] & NEGATIVE_ADVERB_STARTER) strcat(buffer,(char*)"negative_adverb_starter ");
	if (parseFlags[i] & NON_COMPLEMENT_ADJECTIVE) strcat(buffer,(char*)"non_complement_adjective ");
	
	if (parseFlags[i] & CONJUNCTIONS_OF_TIME) strcat(buffer,(char*)"conjunction_of_time ");
	if (parseFlags[i] & CONJUNCTIONS_OF_SPACE) strcat(buffer,(char*)"conjunction_of_space ");
	if (parseFlags[i] & CONJUNCTIONS_OF_ADVERB) strcat(buffer,(char*)"conjunction_of_adverb ");
	if (parseFlags[i] & NONDESCRIPTIVE_ADJECTIVE) strcat(buffer,(char*)"nondescriptive_adjective ");
	if (parseFlags[i] & NEGATIVE_SV_INVERTER) strcat(buffer,(char*)"NEGATIVE_SV_INVERTER ");
	if (parseFlags[i] & LOCATIONAL_INVERTER) strcat(buffer,(char*)"LOCATIONAL_INVERTER ");
	if (parseFlags[i] & ADVERB_POSTADJECTIVE) strcat(buffer,(char*)"ADVERB_POSTADJECTIVE ");
	if (parseFlags[i] & QUANTITY_NOUN) strcat(buffer,(char*)"QUANTITY_NOUN ");
	if (parseFlags[i] & CONJUNCTIVE_ADVERB) strcat(buffer,(char*)"CONJUNCTIVE_ADVERB ");
	if (parseFlags[i] & ADVERB_POSTADJECTIVE) strcat(buffer,(char*)"ADVERB_POSTADJECTIVE ");
	if (parseFlags[i] & CORRELATIVE_ADVERB) strcat(buffer,(char*)"CORRELATIVE_ADVERB ");
	if (parseFlags[i] & POTENTIAL_CLAUSE_STARTER) strcat(buffer,(char*)"POTENTIAL_CLAUSE_STARTER ");
	if (parseFlags[i] & VERB_ALLOWS_OF_AFTER) strcat(buffer,(char*)"VERB_ALLOWS_OF_AFTER ");
	if (parseFlags[i] & ADJECTIVE_GOOD_SUBJECT_COMPLEMENT) strcat(buffer,(char*)"ADJECTIVE_GOOD_SUBJECT_COMPLEMENT ");

	if (parseFlags[i] & DISTANCE_NOUN) strcat(buffer,(char*)"DISTANCE_NOUN ");
	if (parseFlags[i] & TIME_NOUN) strcat(buffer,(char*)"TIME_NOUN ");
	if (parseFlags[i] & TIME_ADJECTIVE) strcat(buffer,(char*)"TIME_ADJECTIVE ");
	if (parseFlags[i] & DISTANCE_ADJECTIVE) strcat(buffer,(char*)"DISTANCE_ADJECTIVE ");
	if (parseFlags[i] & DISTANCE_ADVERB) strcat(buffer,(char*)"DISTANCE_ADVERB ");
	if (parseFlags[i] & TIME_ADVERB) strcat(buffer,(char*)"TIME_ADVERB ");

# endif
}

static void ParseFlags(char* buffer, int i)
{
#ifndef DISCARDPARSER
	ParseFlags0(buffer,i);

	if (posValues[i] & VERB_BITS) 
	{
		if (canSysFlags[i] & VERB_NOOBJECT) strcat(buffer,(char*)"noobject "); // 1 Something ----s   2 Somebody ----s
		if (canSysFlags[i] & VERB_INDIRECTOBJECT) strcat(buffer,(char*)"indirectobj ");
		if (canSysFlags[i] & VERB_DIRECTOBJECT) strcat(buffer,(char*)"directobj "); // 8 Somebody ----s something  9 Somebody ----s somebody 20PP  Somebody ----s somebody  21PP Somebody ----s something 10 Something ----s somebody  11 Something ----s something
		if (canSysFlags[i] & VERB_TAKES_ADJECTIVE) strcat(buffer,(char*)"adjective-object "); // 7    Somebody ----s Adjective
		if (canSysFlags[i] & VERB_TAKES_TOINFINITIVE) strcat(buffer,(char*)"direct-toinfinitive "); // proto 28 "Somebody ----s to INFINITIVE"   "we agreed to plan"
		if (canSysFlags[i] & VERB_TAKES_VERBINFINITIVE) strcat(buffer,(char*)"direct-infinitive "); // proto 32, 35 "Somebody ----s INFINITIVE"   "Something ----s INFINITIVE"
		if (canSysFlags[i] & VERB_TAKES_INDIRECT_THEN_TOINFINITIVE) strcat(buffer,(char*)"VERB_TAKES_INDIRECT_THEN_TOINFINITIVE "); // proto 24  --  verbs taking to infinitive after object: (char*)"Somebody ----s somebody to INFINITIVE"  "I advise you *to go"
		if (canSysFlags[i] & VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE) strcat(buffer,(char*)"VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE "); // proto 25  -  verbs that take direct infinitive after object  "Somebody ----s somebody INFINITIVE"  "i heard her sing"
		if (canSysFlags[i] & VERB_TAKES_GERUND) strcat(buffer,(char*)"gerundobj "); // 33    Somebody ----s VERB-ing
#ifdef JUNK
//x3    It is ----ing		INTRANS
//x?4    Something is ----ing PP	INTRANS
x?5    Something ----s something Adjective/Noun  (does this mean modified noun or adj?)  ADJOBJECT
x?6    Something ----s Adjective/Noun  ADJOBJECT
12    Something ----s to somebody 
13    Somebody ----s on something 
x14    Somebody ----s somebody something TRANS  (dual)
15    Somebody ----s something to somebody 
16    Somebody ----s something from somebody 
17    Somebody ----s somebody with something 
18    Somebody ----s somebody of something 
19    Somebody ----s something on somebody 
22    Somebody ----s PP 
x23    Somebody s (body part) ----s INTRANS
26    Somebody ----s that CLAUSE 
27    Somebody ----s to somebody 
29    Somebody ----s whether INFINITIVE 
30    Somebody ----s somebody into V-ing something 
31    Somebody ----s something with something 
34    It ----s that CLAUSE 
#endif	
	}
	if (posValues[i] & (NOUN_BITS|PRONOUN_BITS))
	{
		if (canSysFlags[i] & TAKES_POSTPOSITIVE_ADJECTIVE) strcat(buffer,(char*)"takepostposadj ");
		if (canSysFlags[i] & ORDINAL) strcat(buffer,(char*)"ordinal ");
	}
	if (posValues[i] & ADJECTIVE_BITS)
	{
		if (canSysFlags[i] & COMMON_PARTICIPLE_VERB) strcat(buffer,(char*)"commonparticipleverb ");
		if (canSysFlags[i] & ADJECTIVE_POSTPOSITIVE) strcat(buffer,(char*)"adjpostpositive ");
	}
	if (posValues[i] & ADVERB)
	{
		if (canSysFlags[i] & EXTENT_ADVERB) strcat(buffer,(char*)"extendadv ");
	}
	if (posValues[i] & CONJUNCTION_SUBORDINATE)
	{
		if (canSysFlags[i] & CONJUNCT_SUBORD_NOUN) strcat(buffer,(char*)"subordnoun ");
	}
#endif
}

void DecodeTag(char* buffer, uint64 type, uint64 tie, uint64 originalBits)
{
	*buffer = 0;
	if (type & IDIOM) strcat(buffer,(char*)"Idiom ");
	if (type & PUNCTUATION) strcat(buffer,(char*)"Punctuation ");
	if (type & PAREN) strcat(buffer,(char*)"Parenthesis ");
	if (type & COMMA) strcat(buffer,(char*)"Comma ");
	if (type & CURRENCY) strcat(buffer,(char*)"Currency ");
	if (type & QUOTE) strcat(buffer,(char*)"Quote ");
	if (type & POSSESSIVE) strcat(buffer,(char*)"Possessive ");
	if (type & FOREIGN_WORD) strcat(buffer,(char*)"Foreign_word ");
	if (type & (NOUN | NOUN_BITS)) 
	{
		if (type & NOUN_PLURAL) Showit(buffer,(char*)"Noun_plural ",tie&NOUN);
		if (type & NOUN_ADJECTIVE) Showit(buffer,(char*)"Noun_adjective ",tie&ADJECTIVE);
		if (type & NOUN_GERUND) Showit(buffer,(char*)"Noun_gerund ",tie&VERB);
		if (type & NOUN_INFINITIVE) Showit(buffer,(char*)"Noun_infinitive ",tie&VERB);
		if (type & NOUN_SINGULAR) Showit(buffer,(char*)"Noun_singular ",tie&NOUN);
		if (type & NOUN_PROPER_SINGULAR) Showit(buffer,(char*)"Noun_proper_singular ",tie&NOUN);
		if (type & NOUN_PROPER_PLURAL) Showit(buffer,(char*)"Noun_proper_plural ",tie&NOUN);
		if (type & NOUN_NUMBER) Showit(buffer,(char*)"Noun_number ",tie&NOUN);
		if (type & PLACE_NUMBER) Showit(buffer,(char*)"Place_number ",tie&NOUN);
		if (!(type & NOUN_BITS)) 
			Showit(buffer,(char*)"Noun_unknown ",tie&NOUN);
	}
	if (type & AUX_VERB) 
	{
		if (type & AUX_VERB_FUTURE) strcat(buffer,(char*)"Aux_verb_future ");
		if (type & AUX_VERB_PAST) strcat(buffer,(char*)"Aux_verb_past ");
		if (type & AUX_VERB_PRESENT) strcat(buffer,(char*)"Aux_verb_present ");
		if (type & AUX_BE && originalBits & VERB_INFINITIVE) strcat(buffer,(char*)"Aux_be_infinitive ");
		if (type & AUX_BE && originalBits & VERB_PAST_PARTICIPLE) strcat(buffer,(char*)"Aux_be_pastparticiple ");
		if (type & AUX_BE && originalBits & VERB_PRESENT_PARTICIPLE) strcat(buffer,(char*)"Aux_be_presentparticiple ");
		if (type & AUX_BE && originalBits & VERB_PRESENT) strcat(buffer,(char*)"Aux_be_present ");
		if (type & AUX_BE && originalBits & VERB_PRESENT_3PS) strcat(buffer,(char*)"Aux_be_present_3ps ");
		if (type & AUX_BE && originalBits & VERB_PAST) strcat(buffer,(char*)"Aux_be_past ");
		if (type & AUX_BE && !*buffer) strcat(buffer,(char*)"Aux_be ");
		if (type & AUX_HAVE && originalBits & VERB_PRESENT_3PS) strcat(buffer,(char*)"Aux_have_present_3ps ");
		if (type & AUX_HAVE && originalBits & VERB_PRESENT) strcat(buffer,(char*)"Aux_have_present "); //  have in first position will be conjugated
		else if (type & AUX_HAVE && originalBits & VERB_INFINITIVE) strcat(buffer,(char*)"Aux_have_infinitive "); 
		if (type & AUX_HAVE && originalBits & VERB_PAST) strcat(buffer,(char*)"Aux_have_past ");
		if (type & AUX_HAVE && !*buffer) strcat(buffer,(char*)"Aux_have ");
		if (type & AUX_DO && originalBits & VERB_PAST) strcat(buffer,(char*)"Aux_do_past ");
		if (type & AUX_DO && originalBits & VERB_PRESENT) strcat(buffer,(char*)"Aux_do_present ");
		if (type & AUX_DO && originalBits & VERB_PRESENT_3PS) strcat(buffer,(char*)"Aux_do_present_3ps ");
		if (type & AUX_DO && !*buffer) strcat(buffer,(char*)"Aux_do ");
	}
	if (type & (VERB|VERB_BITS)) 
	{
		if (type & VERB_INFINITIVE) Showit(buffer,(char*)"Verb_infinitive ",tie&VERB);
		if (type & VERB_PRESENT_PARTICIPLE) Showit(buffer,(char*)"Verb_present_participle ",tie&VERB);
		if (type & VERB_PAST) Showit(buffer,(char*)"Verb_past ",tie&VERB);
		if (type & VERB_PAST_PARTICIPLE) Showit(buffer,(char*)"Verb_past_participle ",tie&VERB);
		if (type & VERB_PRESENT) Showit(buffer,(char*)"Verb_present ",tie&VERB);
		if (type & VERB_PRESENT_3PS) Showit(buffer,(char*)"Verb_present_3ps ",tie&VERB);
		if (!(type & VERB_BITS)) 
			Showit(buffer,(char*)"Verb_unknown ",tie&VERB);
	}
	if (type & PARTICLE) strcat(buffer,(char*)"Particle ");
	if (type & INTERJECTION) strcat(buffer,(char*)"Interjection ");
	if (type & (ADJECTIVE|ADJECTIVE_BITS))
	{
		if (type & ADJECTIVE_NOUN) Showit(buffer,(char*)"Adjective_noun ",tie&NOUN); // can be dual kind of adjective
		if (type & ADJECTIVE_PARTICIPLE) Showit(buffer,(char*)"Adjective_participle ",tie&VERB); // can be dual kind of adjective
		if (type & ADJECTIVE_NUMBER) Showit(buffer,(char*)"Adjective_number ",tie&ADJECTIVE);
		if (type & PLACE_NUMBER) Showit(buffer,(char*)"Place_number ",tie&ADJECTIVE);
		if (type & ADJECTIVE_NORMAL) 
		{
			if (type & MORE_FORM) Showit(buffer,(char*)"Adjective_more ",tie&ADJECTIVE);
			else if (type & MOST_FORM) Showit(buffer,(char*)"Adjective_most ",tie&ADJECTIVE);
			else  Showit(buffer,(char*)"Adjective_normal ",tie&ADJECTIVE);
		}
		if (!(type & ADJECTIVE_BITS))  Showit(buffer,(char*)"Adjective_unknown ",tie&ADJECTIVE);
	}
	if (type & ADVERB)
	{
		if (type & MORE_FORM) Showit(buffer,(char*)"Adverb_more ",tie&ADVERB);
		else if (type & MOST_FORM) Showit(buffer,(char*)"Adverb_most ",tie&ADVERB);
		else Showit(buffer,(char*)"Adverb ",tie&ADVERB);
	}
	if (type & PREPOSITION) Showit(buffer,(char*)"Preposition ",tie&PREPOSITION);
	if (type & TO_INFINITIVE) strcat(buffer,(char*)"To_infinitive ");
	if (type & PRONOUN_BITS)
	{
		if (type & PRONOUN_OBJECT) strcat(buffer,(char*)"Pronoun_object ");
		if (type & PRONOUN_SUBJECT) strcat(buffer,(char*)"Pronoun_subject ");
	}
	if (type & THERE_EXISTENTIAL)  strcat(buffer,(char*)"There_existential ");
	if (type & SINGULAR_PERSON)  strcat(buffer,(char*)"Singular_person ");
	if (type & CONJUNCTION)
	{
		if (type & CONJUNCTION_COORDINATE) Showit(buffer,(char*)"Conjunction_coordinate ",0);
		if (type & CONJUNCTION_SUBORDINATE) Showit(buffer,(char*)"Conjunction_subordinate ",0);
	}
	if (type & DETERMINER_BITS) 
	{
		if (type & PRONOUN_POSSESSIVE) strcat(buffer,(char*)"Pronoun_possessive ");
		if (type & PREDETERMINER) strcat(buffer,(char*)"Predeterminer ");
		if (type & DETERMINER) strcat(buffer,(char*)"Determiner ");
	}
}

static void Tags(char* buffer, int i)
{
	*buffer = 0;
	WORDP D = (originalLower[i]) ? originalLower[i] : originalUpper[i];
	if (!D) return;

	uint64 tie = D->systemFlags & ESSENTIAL_FLAGS; // tie break values
#ifndef DISCARDPARSER
	if (bitCounts[i] <= 1) tie = 0;
#endif
	uint64 type = posValues[i];
//	if (type & AUX_HAVE && D->properties & VERB_PRESENT && !firstAux && i > 1 && stricmp(wordStarts[i-1],(char*)"to")) type |= VERB_PRESENT; //  have in first position will be conjugated
	type |= D->properties & (MORE_FORM|MOST_FORM); // supplement
	DecodeTag(buffer,type, tie,allOriginalWordBits[i]);
	if (type & AUX_VERB) firstAux = D;
}

char* DumpAnalysis(int start, int end,uint64 flags[MAX_SENTENCE_LENGTH],const char* label,bool original,bool roleDisplay)
{
	static char buffer[BIG_WORD_SIZE];
	*buffer = 0;
	char* ambiguous = "";
	char* faultyparse = "";
	if (!original && tokenFlags & FAULTY_PARSE) faultyparse = "badparse "; // only one of ambiguous (worse) and faultyparse will be true
	sprintf(buffer,(char*)"%s%s%s %d words: ",ambiguous,faultyparse,label,end-start+1);
	unsigned int lenpre;
	firstAux = NULL;

	for (int i = start; i <= end; ++i)
    {
		WORDP D = originalLower[i];
		if (flags[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL) && originalUpper[i]) D = originalUpper[i];

		if (D) strcat(buffer,D->word); // we know it as lower or upper depending
		else  strcat(buffer,wordStarts[i]); // what he gave which we didnt know as lower or upper
		if (original && ignoreWord[i]) 
		{
			strcat(buffer,(char*)" ( --- )  ");
			continue;
		}
		// canonical
		if (!original) 
		{
			if (canonicalLower[i] && originalLower[i] && !strcmp(canonicalLower[i]->word,originalLower[i]->word)); // same
			else if (canonicalUpper[i] && originalUpper[i] && !strcmp(canonicalUpper[i]->word,originalUpper[i]->word)); // same
			else 
			{
				if (D && !(D->properties & PART_OF_SPEECH) && D->properties & FOREIGN_WORD && canonicalLower[i] && !stricmp(canonicalLower[i]->word,(char*)"unknown-word"))  strcat(buffer,(char*)"/foreign");
				else if (canonicalLower[i]) 
				{
					strcat(buffer,(char*)"/");
					strcat(buffer,canonicalLower[i]->word);
				}
				else if (canonicalUpper[i]) 
				{
					strcat(buffer,(char*)"/");
					strcat(buffer,canonicalUpper[i]->word);
				}
				else if (D && D->properties & FOREIGN_WORD)  strcat(buffer,(char*)"/foreign");
			}
		}
		strcat(buffer,(char*)" (");
		lenpre = strlen(buffer);
#ifndef DISCARDPARSER
		if (roleDisplay)
		{
			if (!original && clauses[i] && clauses[i-1] != clauses[i]) strcat(buffer,(char*)"<Clause ");
			if (!original && verbals[i] && verbals[i-1] != verbals[i]) strcat(buffer,(char*)"<Verbal ");
			if (i == startSentence && phrases[startSentence] == phrases[endSentence]){;} // wrapped phrase "*what is this world coming to"
			else if (!original && phrases[i] && phrases[i-1] != phrases[i]) 
			{
				if (posValues[i] & (PREPOSITION|NOUN_BITS|PRONOUN_BITS) || roles[i] & OMITTED_TIME_PREP || i != startSentence)  
				{
					if (posValues[i] & ABSOLUTE_PHRASE) strcat(buffer,(char*)"<AbsolutePhrase ");
					else strcat(buffer,(char*)"<Phrase ");  // wont be true on sentence start if wrapped from end  BUT ABSOLUTE will be true
				}
			}
			if (!original && roles[i]) strcat(buffer,GetRole(roles[i]));
		}
#endif

		Tags(buffer+strlen(buffer),i); // tags clears

#ifndef DISCARDPARSER
		if (roleDisplay && !original) // show endings of phrases
		{
			if (phrases[i] && ( !(phrases[i+1] & phrases[i]) || i == endSentence) ) // a phrase where next is different 
			{
				if (i != endSentence || !(phrases[startSentence] & phrases[endSentence])) strcat(buffer,(char*)"Phrase> "); // if wrapped to start from end of sentence, this wont end here, ends at start
			}
			if (phrases[i] && phrases[i-1] && phrases[i] != phrases[i-1]  ) // a phrase overlapped "I yelled from *inside the house" 
			{
				strcat(buffer,(char*)"Phrase> "); // if wrapped to start from end of sentence, this wont end here, ends at start
			}
			if (verbals[i] && verbals[i+1] != verbals[i]) strcat(buffer,(char*)"Verbal> ");
			if (clauses[i] && clauses[i+1] != clauses[i]) strcat(buffer,(char*)"Clause> ");
		}
#endif
		size_t len = strlen(buffer);
		if (len == lenpre) strcpy(buffer+len,(char*)")  ");
		else strcpy(buffer+len-1,(char*)")  ");
	}

	strcat(buffer,(char*)"\r\n");
	return buffer;
}

void MarkTags(unsigned int i)
{
	uint64 bits = finalPosValues[i];
	// Bruce Wilcox- bruce is adjective noun
	if (bits & (NOUN | ADJECTIVE_NOUN)) bits |= allOriginalWordBits[i] & (NOUN_HUMAN | NOUN_FIRSTNAME | NOUN_SHE | NOUN_HE | NOUN_THEY | NOUN_TITLE_OF_ADDRESS | NOUN_TITLE_OF_WORK| NOUN_ABSTRACT );
	if (allOriginalWordBits[i] & LOWERCASE_TITLE ) bits |= LOWERCASE_TITLE;

	// special word mark split particle verbs handled by setsequencemark
	
	unsigned int start = i;
	unsigned int stop = i;

	// swallow idioms as single marks
	while (posValues[start-1] == IDIOM) --start;	

	// swallow sequential particle verbs as single marks and idioms also
	while (posValues[stop] & VERB && posValues[stop+1] == PARTICLE) ++stop;
	while (posValues[stop] == IDIOM) ++stop; // onto the non-idiom

	// mark pos data and supplemental checking on original proper names, because canonical may mess up, like james -->  jam
	uint64 bit = START_BIT;
	for (int j = 63; j >= 0; --j)
	{
		if (bits & bit) 
		{
			if (bit & PRONOUN_BITS)  MarkMeaningAndImplications(0,0,MakeMeaning(Dpronoun),start,stop); // general as opposed to specific
			else if (bit & AUX_VERB)  MarkMeaningAndImplications(0, 0,MakeMeaning(Dauxverb),start,stop); // general as opposed to specific

			// determiners are a kind of adjective- "how *much time do you like" but we dont want them marked
			//	if (bit & DETERMINER_BITS) MarkMeaningAndImplications(0,0,MakeMeaning(Dadjective),start,stop);
			
			MarkMeaningAndImplications(0, 0,posMeanings[j],start,stop); // NOUNS which have singular like "well" but could be infinitive, are listed as nouns but not infintive
			if (bit & NOUN_HUMAN) // impute additional meanings
			{
				if (bits & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) 
				{
					MarkMeaningAndImplications(0, 0,MakeMeaning(Dhumanname),start,stop);
					MarkMeaningAndImplications(0, 0,MakeMeaning(Dpropername),start,stop);
				}
				if (bits & NOUN_HE) MarkMeaningAndImplications(0, 0,MakeMeaning(Dmalename),start,stop);
				if (bits & NOUN_SHE) MarkMeaningAndImplications(0, 0,MakeMeaning(Dfemalename),start,stop);
			}
		}

		bit >>= 1;
	}
		
	// system flags we allow
	if (originalLower[i]) 
	{
		bit = START_BIT;
		for (int j = 63; j >= 0; --j)
		{
			if (!(originalLower[i]->systemFlags & bit)) {;}
			else if (bit & WEB_URL && strchr(originalLower[i]->word+1,'@'))
			{
				char* x = strchr(originalLower[i]->word+1,'@');
				if (x && IsAlphaUTF8(x[1])) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~email_url")),start,stop);
			}
			else if (bit & MARK_FLAGS)  MarkMeaningAndImplications(0, 0,sysMeanings[j],start,stop);
			bit >>= 1;
		}
		uint64 age = originalLower[i]->systemFlags & AGE_LEARNED;
		if (!age){;}
		else if (age == KINDERGARTEN)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~kindergarten")),start,stop);
		else if (age == GRADE1_2)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~grade1_2")),start,stop);
		else if (age == GRADE3_4)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~grade3_4")),start,stop);
		else if (age == GRADE5_6)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~grade5_6")),start,stop);
	}
	if (bits & AUX_VERB ) finalPosValues[i] |= VERB;	// lest it report "can" as untyped, and thus become a noun -- but not ~verb from system view

}

/////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////// PARSER CODE

/////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DISCARDPARSER
#ifdef INFORMATION

The algorithm is:

1. figure out what POS tags we can be certain of
2. Assign roles as we go, forcing bindings that seem to make sense at the time. This is subject to garden-path phenomena, which means when we detect inconsistency later,
we have to go back and patch up the roles and even the POS tags to make a coherent whole. Probability is built into the expectation of role coercions we do as we go.

E.g., when we have two nouns and dont need the second, we have to decide is it appositive or is it start of implied clause.

3. As parsing progresses, we enter and exit new nested roleLevels as phrases, verbals and clauses spring into being and become complete. A roleLevel indicates what we are currently seeking or allow
like a subject, verb, object, etc. So until we see a verb, for example, we are not expected to see an object. We may mislabel an object as a subject (because we were seeking that) but have to
patch later when the structure becomes more apparent.

Structures:

1. Object complement:    It is most often used with verbs of creating or nominating such as make, name, elect, paint, call, choosing, judging, appointing, etc. in the active voice,.. implied missing "to be"
	Subject Verb Object nounphrase.		He considered her his grandmother.
	Subject Verb Object adjective.		He drove her crazy.
	Subject Verb Object prep-phrase.	He loved her like a sister.???
	Subject verb object gerund	?		I saw the Prime Minister sleeping
	Subject verb object clause			I like the shawl *people wear

2. Appostive noun (at end will have a comma, lest it be object ocmplement)
	He considered Joe, a self-starter.	(comma helps)
	The commander stood his ground, a heroic figure. (delayed appositive of commander)
	It is a pleasure *to meet up (delayed appositive of it but fine as appossitive of pleasure)
	I, the leader of the rebellion that defines eternity, call this to order.

3. Clause-omitting starter
	The shawl *people wear is green (subject complement)
	Roger likes the shawl *people wear (object complement)

4. Prep phrase omitting prep
	Time phrases					He will walk *next week

5.	Adjective Object
	Subject verb adjective		The shawl is green


The first noun encountered is generally the subject of the sentence except:

0. Inside a prepositional phrase, verbal, or clause.
1. Appositive noun at sentence start with comma:  "A dark wedge, the eagle hurtled  at nearly 200 miles per hour."
	detectable because it starts the sentence and has no verb before the comma, and has a noun and verb after the comma.
2. Questions often flip subject/object:  "what is your name"
3. Absolute phrase at start. separated by comma, 
	a) lacking a full verb (a participle w/o aux)
		Their reputation as winners secured by victory, the New York Liberty charged into the semifinals
	b) lacking any verb (sometimes implied) --
		Your best friends, where are they now, when you need them?  # NOUN PHRASE (NO VERB)  # http://grammar.ccc.commnet.edu/grammar/phrases.htm#absolute
		The season over, they were mobbed by fans in Times Square  # IMPLIED BEING AS VERB
		Stars all their adult lives, they seemed used to the attention # IMPLIED [Having been AS VERB] 
4. Direct address

PARENS include:
1.  pretend they are commas for appositive: Steve Case (AOL s former CEO)
2. junk extra "You will need a flashlight for the camping trip (don't forget the batteries!). "

COMMAS denote:
1. appositives embedded " Bill, our CEO, ate here"
2. a list of nouns	"he ate green bananas, lush oranges, and pineapple"
3. multiple adjectives " he ate ripe, green fruit."
4. after intro prep phrase  "After the show, John and I went out to dinner."
5. before coord conjunction: "Ryan went to the beach yesterday, but he forgot his sunscreen."
6. Direct address by name: "Amber, could you come here for a moment?"
7. leading adjective_participle: "Running toward third base, he suddenly realized how stupid he looked."

# opinion size age shape colour origin material is adjective order

ROLES:

Where a noun infinitive is the complement, we note the role on the verb, not on the To.  If the noun infintiive is not a complement,
then its role will be VERB2 (generic).

PHRASES and CLAUSES

The system recognizes:
1. prepositional phrase + ommited prep time phrase
2. verbals (noun-infinitive, adjective participle, noun-gerund)
3. subordinate clause
	a. started by subordinate conjunction
	b. started by relative pronoun serving as subject
	c. ommitted
4. absolute phrase
5. address phrase
6. appostive phrase

Back to back nouns can occur as follows:

1. A clause/phrase/verbal is ending and noun is subject of next chunk - best with commas
		"Smelling food cats often eat"  though better with a comma
2. Appositive - can use commas
	    "My friend Tom lives here"
		"Tom my friend lives here" though better with comma
		one or both must be determined nouns, where one the other must be a proper name. Determiners a and the must be intermixed, not duplicated.
3.  Adjective nouns -- do not use commas
		"I saw the bank clerk"
4. with commas, we can have address  "Bill, dogs eat meat" -- requires commas

Verb structures:

1.  Intransitive - takes no object
2.  Transitive - takes a direct object
3.  DiTransitive - takes direct and indirect object
4.  Factitive - takes two objects (second is like prep phrase "as") "elect him president" or noun adjective
5.  Causative - causes an action - generally followed by a noun or pronoun, then by an infinitive.  The infinitive in "Peter helped Polly correct exams." is to correct.  The to is understood, and not necessary to include it  


Is there a distinction between: OBJECT_COMPLEMENT  in "they found the way *to go"
and POSTNOMINAL_ADJECTIVE in "They described the way *to survive"
and direct appositive -- "The mistake, *to elect him", was critical - this requires comma separation

Postnominal can occur after any normal noun (including subjects).  It is either a predictable adjective (there are some specific ones that usually are post noun, or
a verbal.

Some Verbs "expect" an object complement after object, but that complement can be 
a POSTNOMINAL_ADJECTIVE or a noun. As a adjective, the adjective is an OBJECT_COMPLEMENT that describes how the object ended up AFTER the
verb happened (causal verbs).

How to tell a interrupter infinitive, sandwiched by commas from an appositive infinitive- cant. we will let it be appositive



subject auxverb inversion happens with:
1. questions
2. sometimes when the sentence starts with a place expression (here comes the bride)  (existential there) and nowhere
3. when the sentence begins with some negative words (negative adverbials) and no/only expressions)
4. when we use a condition without if -- had were and should starting as helpers
5. sometimes when the sentence has a comparison

subject complement can preceed subject after: how 

#endif


#define INCLUSIVE 1
#define EXCLUSIVE 2

#define AMBIGUOUS_VERBAL (ADJECTIVE_PARTICIPLE | NOUN_GERUND )
#define AMBIGUOUS_PRONOUN (PRONOUN_SUBJECT | PRONOUN_OBJECT )

#define GUESS_NOCHANGE 0
#define GUESS_ABORT 1
#define GUESS_RETRY 2
#define GUESS_CONTINUE 3
#define GUESS_RESTART 4

static unsigned int GetClauseTail(int i)
{
	int clause = clauses[i];
	if (clause)
	{
		while (++i <= endSentence && clauses[i] & clause){;}
		return i-1;
	}
	else return 0;
}

static unsigned int GetVerbalTail(int i)
{
	int verbal = verbals[i];
	if (verbal)
	{
		while (++i <= endSentence && verbals[i] & verbal){;}
		return i-1;
	}
	else return 0;
}

static unsigned int GetPhraseHead(int i)
{
	int phrase = phrases[i];
	if (phrase)
	{
		while (--i >= startSentence && phrases[i] & phrase){;}
		return i+1;
	}
	else return 0;
}

static unsigned int GetPhraseTail(int i)
{
	int phrase = phrases[i];
	if (phrase)
	{
		while (++i <= endSentence && phrases[i] & phrase){;}
		return i-1;
	}
	else return 0;
}

static void ErasePhrase(int i)
{
	int phrase = phrases[i--];
	while (++i <= endSentence && phrases[i] & phrase)
	{
		if (roles[i] & OBJECT2) 
		{
			roles[i] ^= OBJECT2;
			objectRef[i] = 0;
		}
		phrases[i] ^= phrase;
	}
}

static void AddRoleLevel(unsigned int roles,int i)
{
	if (roleIndex >= (MAX_CLAUSES-1)) return;	 // overflow
	needRoles[++roleIndex] = roles; 
	subjectStack[roleIndex] = 0;
	verbStack[roleIndex] = 0;
	objectStack[roleIndex] = 0;
	auxVerbStack[roleIndex] = 0;
	startStack[roleIndex] = (unsigned char) i;
}

static bool NounSeriesFollowedByVerb(int at)
{
	while (posValues[++at] & (DETERMINER|ADJECTIVE_NORMAL|ADJECTIVE_NUMBER|NORMAL_NOUN_BITS)  && bitCounts[at] == 1);
	return (posValues[at] & (VERB_BITS|AUX_VERB) && bitCounts[at] == 1);
}

static bool ResolveByStatistic(int i,bool &changed) 
{
	if (bitCounts[i] == 1) return false;
	
	// if something could be noun or verb, and  we are not looking for a verb yet, be a noun.
	if (posValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_PLURAL|NOUN_SINGULAR) && posValues[i] & VERB_BITS && roleIndex == MAINLEVEL && !(needRoles[MAINLEVEL] & MAINVERB))
	{
		LimitValues(i,-1 ^ VERB_BITS,(char*)"in noun/verb conflict discarding verb when none is sought",changed);
	}
	else if (lcSysFlags[i] & ESSENTIAL_FLAGS)
	{
		if (posValues[i] & ADVERB)
		{
			if (posValues[i] & (ADJECTIVE_BITS|DETERMINER) && posValues[i+1] & NOUN_BITS) return false;  // "*no price for the new shares has been set"
			else LimitValues(i,ADVERB,(char*)"when nothing needed, prefer adverb",changed);
		}
		if (lcSysFlags[i] & NOUN && posValues[i] & NOUN_BITS)
		{
			LimitValues(i,NOUN_BITS,(char*)"statistic force original noun",changed);
		}
		if (lcSysFlags[i] & VERB && posValues[i] & VERB_BITS)
		{
			LimitValues(i,VERB_BITS,(char*)"statistic force original verb",changed);
		}
		if (lcSysFlags[i] & PREPOSITION && posValues[i] & PREPOSITION)
		{
			LimitValues(i,PREPOSITION,(char*)"statistic force original preposition",changed);
		}
		if (lcSysFlags[i] & ADVERB && posValues[i] & ADVERB)
		{
			LimitValues(i,ADVERB,(char*)"statistic force original adverb",changed);
		}
		if (lcSysFlags[i] & ADJECTIVE && posValues[i] & ADJECTIVE_BITS)
		{
			LimitValues(i,ADJECTIVE_BITS,(char*)"statistic force original adjective",changed);
		}
	}
	if (canSysFlags[i] & ESSENTIAL_FLAGS)
	{
		if (canSysFlags[i] & NOUN && posValues[i] & NOUN_BITS)
		{
			LimitValues(i,NOUN_BITS,(char*)"statistic force canonical noun",changed);
		}
		if (canSysFlags[i] & VERB && posValues[i] & VERB_BITS)
		{
			LimitValues(i,VERB_BITS,(char*)"statistic force canonical verb",changed);
		}
		if (canSysFlags[i] & PREPOSITION && posValues[i] & PREPOSITION)
		{
			LimitValues(i,PREPOSITION,(char*)"statistic force canonical preposition",changed);
		}
		if (canSysFlags[i] & ADVERB && posValues[i] & ADVERB)
		{
			LimitValues(i,ADVERB,(char*)"statistic force canonical adverb",changed);
		}
		if (canSysFlags[i] & ADJECTIVE && posValues[i] & ADJECTIVE_BITS)
		{
			LimitValues(i,ADJECTIVE_BITS,(char*)"statistic force canonical adjective",changed);
		}
	}
	if (posValues[i] & PRONOUN_POSSESSIVE && NounSeriesFollowedByVerb(i)) // "he hid because *his bank notes were bad"
	{
		LimitValues(i,PRONOUN_POSSESSIVE,(char*)"statistic force possesive pronoun with following noun-based series and then verb",changed);
	}
	
	if (posValues[i] & NOUN_BITS)
	{
		LimitValues(i,NOUN_BITS,(char*)"statistic force noun",changed);
		if (posValues[i] & NOUN_NUMBER) 
		{
			LimitValues(i,NOUN_NUMBER,(char*)"statistic force cardinal noun",changed);
		}
	}
	if (posValues[i] & AUX_VERB)
	{
		LimitValues(i,AUX_VERB,(char*)"statistic force aux verb",changed);
	}
	if (posValues[i] & VERB_BITS)
	{
		LimitValues(i,VERB_BITS,(char*)"statistic force verb",changed);
	}
	if (posValues[i] & FOREIGN_WORD)
	{
		LimitValues(i,FOREIGN_WORD,(char*)"statistic force foreign word",changed);
	}
	if (posValues[i] & PREPOSITION) 
	{
		LimitValues(i,PREPOSITION,(char*)"statistic force preposition",changed);
	}
	if (posValues[i] & CONJUNCTION_SUBORDINATE && !(posValues[i] & DETERMINER)) // but not I cant cut *that tree down with
	{
		int at = i;
		while (++at <= endSentence)
		{
			if (ignoreWord[at]) continue;
			if (posValues[at] & PREPOSITION) break; 
			if (posValues[at] & VERB_BITS) 
			{
				LimitValues(i,CONJUNCTION_SUBORDINATE,(char*)"statistic force subord conjuct",changed);
				break;
			}
		}
	}
	if (posValues[i] & ADVERB)
	{
		LimitValues(i,ADVERB,(char*)"statistic force adverb",changed);
	}
	if (posValues[i] & ADJECTIVE_BITS)
	{
		LimitValues(i,ADJECTIVE_BITS,(char*)"statistic force adjective",changed);
		if (posValues[i] & ADJECTIVE_NUMBER) // vs subject
		{
			LimitValues(i,ADJECTIVE_NUMBER,(char*)"statistic force numeric adjective",changed);
		}
		if (posValues[i] & ADJECTIVE_NORMAL) // vs subject
		{
			LimitValues(i,ADJECTIVE_NORMAL,(char*)"statistic force adjective normal",changed);
		}
	}

	if (posValues[i] & NORMAL_NOUN_BITS)
	{
		LimitValues(i,NORMAL_NOUN_BITS,(char*)"statistic force normal noun",changed);
		if (posValues[i] & NOUN_PROPER_PLURAL) // vs singular proper
		{
			LimitValues(i,NOUN_PROPER_PLURAL,(char*)"statistic force proper plural noun",changed);
		}
	}
	if (posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS))
	{
		LimitValues(i,(VERB_PRESENT|VERB_PRESENT_3PS),(char*)"statistic force present tense",changed);
	}
	if (posValues[i] & VERB_PAST) // vs infinitive of another like found/find
	{
		LimitValues(i,VERB_PAST,(char*)"statistic force past tense",changed);
	}
	if (posValues[i] & CONJUNCTION_SUBORDINATE && !(posValues[i] & DETERMINER)) // vs subject -- not "I can cut *that tree down"
	{
		LimitValues(i,CONJUNCTION_SUBORDINATE,(char*)"statistic force subordinate conjunction and see what happens",changed);
	}
	if (posValues[i] & DETERMINER && posValues[i+1] & (NOUN_BITS | ADJECTIVE_BITS)) // vs subject "I can cut *that tree down"
	{
		LimitValues(i,DETERMINER,(char*)"statistic force determiner",changed);
	}
	if (posValues[i] & PRONOUN_OBJECT && !(posValues[i] & DETERMINER)) // vs subject "I can cut *that tree down"
	{
		LimitValues(i,PRONOUN_OBJECT,(char*)"statistic force object pronoun",changed);
	}
	if (posValues[i] & PRONOUN_SUBJECT) // vs determiner
	{
		LimitValues(i,PRONOUN_SUBJECT,(char*)"statistic force subject pronoun",changed);
	}
	if (posValues[i] & TO_INFINITIVE) // vs particle perhaps
	{
		LimitValues(i,TO_INFINITIVE,(char*)"statistic force to infinitive",changed);
	}
	return false;
}

static int GetZone(unsigned int i)
{
	for (unsigned int zone = 0; zone < zoneIndex; ++zone)
	{
		if (i >= zoneBoundary[zone] && i < zoneBoundary[zone+1]) return zone;
	}
	return -1;
}

void MarkRoles(int i)
{
	// mark its main parser role - not all roles are marked and parsing might not have been done
	int start = i;
	int stop = i;
	// swallow idioms as single marks
	while (posValues[start-1] == IDIOM) --start;	
	if (posValues[i] & NOUN_INFINITIVE && posValues[start-1] & TO_INFINITIVE) --start;

	// swallow sequential particle verbs as single marks
	while (posValues[stop+1] == PARTICLE) ++stop;

	uint64 role = roles[i];

	if (role & MAINSUBJECT)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~mainsubject")),start,stop);
	if (role & MAINVERB) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~mainverb")),start,stop);
	if (role & MAININDIRECTOBJECT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~mainindirectobject")),start,stop);
	if (role & MAINOBJECT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~mainobject")),start,stop);
	if (role & SUBJECT_COMPLEMENT)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~subjectcomplement")),start,stop);
	if (role & OBJECT_COMPLEMENT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~objectcomplement")),start,stop);
	if (role & ADJECTIVE_COMPLEMENT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~adjectivecomplement")),start,stop);
	
	if (role & SUBJECT2) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~subject2")),start,stop);
	if (role & VERB2) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~verb2")),start,stop);
	if (role & INDIRECTOBJECT2)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~indirectobject2")),start,stop);
	if (role & OBJECT2)  MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~object2")),start,stop);
	
	if (role & ADDRESS) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~address")),start,stop);
	if (role & APPOSITIVE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~appositive")),start,stop);
	if (role & POSTNOMINAL_ADJECTIVE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~postnominaladjective")),start,stop);
	if (role & REFLEXIVE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~reflexive")),start,stop);
	if (role & ABSOLUTE_PHRASE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~absolutephrase")),start,stop);
	if (role & OMITTED_TIME_PREP) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~omittedtimeprep")),start,stop);
	if (role & DISTANCE_NOUN_MODIFY_ADVERB) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~DISTANCE_NOUN_MODIFY_ADVERB")),start,stop);
	if (role & DISTANCE_NOUN_MODIFY_ADJECTIVE) MarkMeaningAndImplications(0,0,MakeMeaning(StoreWord((char*)"~DISTANCE_NOUN_MODIFY_ADJECTIVE")),start,stop); 
	if (role & TIME_NOUN_MODIFY_ADVERB) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~TIME_NOUN_MODIFY_ADVERB")),start,stop);
	if (role & TIME_NOUN_MODIFY_ADJECTIVE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~TIME_NOUN_MODIFY_ADJECTIVE")),start,stop);
	if (role & OMITTED_OF_PREP) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~omittedofprep")),start,stop);

	uint64 crole = role & CONJUNCT_KINDS;
	if (crole == CONJUNCT_PHRASE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~CONJUNCT_PHRASE")),start,stop);
	if (crole ==  CONJUNCT_CLAUSE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~CONJUNCT_CLAUSE")),start,stop);
	if (crole ==  CONJUNCT_SENTENCE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~CONJUNCT_SENTENCE")),start,stop);
	if (crole ==  CONJUNCT_NOUN) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~CONJUNCT_NOUN")),start,stop);
	if (crole ==  CONJUNCT_VERB) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~CONJUNCT_VERB")),start,stop);
	if (crole ==  CONJUNCT_ADJECTIVE) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~CONJUNCT_ADJECTIVE")),start,stop);
	if (crole == CONJUNCT_ADVERB) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~CONJUNCT_ADVERB")),start,stop);

	crole = role & ADVERBIALTYPE;
	if (crole == WHENUNIT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~whenunit")),start,stop);
	if (crole ==  WHEREUNIT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~whereunit")),start,stop);
	if (crole ==  HOWUNIT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~howunit")),start,stop);
	if (crole ==  WHYUNIT) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~whyunit")),start,stop);

	// meanwhile mark start/end of phrases
	int phrase = phrases[i];
	if (phrase && phrase != phrases[i-1])
	{
		start = stop = i;
		int bit = phrase;
		while (phrases[++stop] & bit){;}
		if (i == startSentence && phrase == phrases[endSentence]) {;} // start at end instead
		else
		{
			if (posValues[i] & NOUN_BITS) MarkMeaningAndImplications(0, 0,MabsolutePhrase,start,stop-1);
			else if (posValues[i] & (ADVERB|ADJECTIVE_BITS)) MarkMeaningAndImplications(0, 0,MtimePhrase,start,stop-1);
			else MarkMeaningAndImplications(0, 0,Mphrase,start,stop-1);
		}
	}
	
	// meanwhile mark start/end of clause
	int clause = clauses[i];
	if (clause && clause != clauses[i-1])
	{
		start = stop = i;
		int bit = clause;
		while (clauses[++stop] & bit){;}
		MarkMeaningAndImplications(0, 0,MakeMeaning(Dclause),start,stop-1);
	}

	// meanwhile mark start/end of verbals
	int verbal = verbals[i];
	if (verbal && verbal != verbals[i-1])
	{
		start = stop = i;
		int bit = verbal;
		while (verbals[++stop] & bit){;}
		MarkMeaningAndImplications(0,0,MakeMeaning(Dverbal),start,stop-1);
	}
	if (role & SENTENCE_END) MarkMeaningAndImplications(0, 0,MakeMeaning(StoreWord((char*)"~sentenceend")),start,stop);

	//mark noun-phrases
	if (posValues[i] & (NOUN_BITS  | PRONOUN_SUBJECT | PRONOUN_OBJECT)
		&& !(posValues[i + 1] & (NOUN_BITS| PRONOUN_SUBJECT | PRONOUN_OBJECT)))
	{
		int j;
		for (j = i - 1; j >= 0; --j)
		{
			if (!(posValues[j] & (ADJECTIVE_BITS | NOUN_BITS | ADVERB | DETERMINER | PREDETERMINER | POSSESSIVE_BITS))) break;
			if (posValues[j] & ADVERB && !(posValues[j + 1] & (ADVERB | ADJECTIVE_BITS))) break;
		}
		MarkMeaningAndImplications(0, 0, MakeMeaning(StoreWord("~noun_phrase")), j+1,i);

	}
}

static void AddRole( int i, uint64 role)
{
	// all major assignments go thru setrole -- but VERB2 can overlap objects, so it wont harm them
	if (!roles[i] || role & (MAINSUBJECT | SUBJECT2 | MAINVERB| MAINOBJECT | OBJECT2 | MAININDIRECTOBJECT | INDIRECTOBJECT2 | OBJECT_COMPLEMENT | SUBJECT_COMPLEMENT))
	{
		SetRole(i,role,false,0);
		return;
	}
	roles[i] |= role; // assign secondary role characteristic
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"   +%s->%s\r\n",wordStarts[i],GetRole(roles[i]));
}

void SetRole( int i, uint64 role,bool revise, int currentVerb)
{
	if (i < startSentence || i > endSentence) return; // precaution

	// remove old reference if it had one
	if (subjectStack[roleIndex] == i && roles[i] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT2)) subjectStack[roleIndex] = 0;
	if (verbStack[roleIndex] == i && roles[i] & (MAINVERB|VERB2)) verbStack[roleIndex] = 0;
	if (objectStack[roleIndex] == i && roles[i] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT2)) objectStack[roleIndex] = 0;
	if (roles[i] & (MAININDIRECTOBJECT|INDIRECTOBJECT2) && indirectObjectRef[currentVerb] == i) indirectObjectRef[currentVerb] = 0;
	if (roles[i] & (OBJECT2|MAINOBJECT) && objectRef[currentVerb] == i) objectRef[currentVerb] = 0;
	if (roles[i] & OBJECT2 && objectRef[lastPhrase] == i) objectRef[lastPhrase] = 0;
	if (roles[i] & (SUBJECT_COMPLEMENT|OBJECT_COMPLEMENT) && complementRef[currentVerb] == i) complementRef[currentVerb] = 0;
	
	roles[i] = role;
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"   +%s->%s\r\n",wordStarts[i],GetRole(roles[i]));

	// set new reference
	if ((role & needRoles[roleIndex]) || revise) // store role xref when we were looking for it - we might see objects added when we have one already
	{ // dont store if already there (conjunction multiple form for example)
		if (role & (MAINSUBJECT|SUBJECT2) && subjectStack[roleIndex] != i) subjectStack[roleIndex] = (unsigned char)i;
		else if (role & (VERB2|MAINVERB) && verbStack[roleIndex] != i) verbStack[roleIndex] = (unsigned char)i;
		else if (role & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) indirectObjectRef[currentVerb] = (unsigned char)i; // implicit at 0 cache if no verb yet
		else if (role & MAINOBJECT)  objectStack[roleIndex] = objectRef[currentVerb] = (unsigned char)i; // implicit at 0 cache if no verb yet
		else if (role & OBJECT2) 
		{
			if (lastPhrase) objectRef[lastPhrase] = (unsigned char)i;
			else if (currentVerb2) objectRef[currentVerb] = (unsigned char)i;
			else objectRef[MAX_SENTENCE_LENGTH-1] = (unsigned char)i; // cache at end til we do see the verb
		}
		else if (role & (SUBJECT_COMPLEMENT|OBJECT_COMPLEMENT)) complementRef[currentVerb] = (unsigned char)i;
	}
	needRoles[roleIndex] &= -1 ^ role; // remove what was supplied
	if (role & (OBJECT2|MAINOBJECT|VERB_INFINITIVE_OBJECT|VERB_INFINITIVE_OBJECT|SUBJECT_COMPLEMENT)) // in theory we are done with subject
	{
		if (quotationCounter){;} // but "i love you" said my mother  is inverse order
		else needRoles[roleIndex] &= -1 ^ (SUBJECT_COMPLEMENT|MAININDIRECTOBJECT|INDIRECTOBJECT2|OBJECT2|MAINOBJECT|VERB_INFINITIVE_OBJECT|TO_INFINITIVE_OBJECT);	// cant be both - can still have inifitive pending- "he wanted his students to *read"
	}
	if (role & SUBJECT_COMPLEMENT) needRoles[roleIndex] &= -1 ^ (ALL_OBJECTS|OBJECT_COMPLEMENT);	// cant be these now
	// if (role & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) needRoles[roleIndex] &= -1 ^ OBJECT_COMPLEMENT; // wrong for "this makes Jenn's mom mad
	if (role & MAINVERB) 
	{
		if (needRoles[roleIndex] & MAINSUBJECT)
		{
			// "are you" questions do not abandon subject yet, same for non-aux have -- but not are as AUX
			// but "be" will be different as infinitive.
			if (parseFlags[startSentence] & (LOCATIONAL_INVERTER|NEGATIVE_SV_INVERTER)) {;} // inversion expected
			else if (canonicalLower[i] &&  (!stricmp(canonicalLower[i]->word,(char*)"be")||!stricmp(canonicalLower[i]->word,(char*)"have"))) {;} // no subject yet
			else needRoles[roleIndex] &= -1 ^ MAINSUBJECT;
		}
	}
	if (role & VERB2 && roleIndex > 1)  // conjunction copy of a verb might be at main level but copy an object/verb2 "let us stand *and *hear the crowd"
	{
		// assume clause subject met, unless implicide clause starter -- bug 
		needRoles[roleIndex] &= -1 ^ SUBJECT2;
	}
	if (role & OBJECT2 && roles[i-1] & OBJECT_COMPLEMENT) SetRole(i-1,INDIRECTOBJECT2); // need to revise backward "I had the mechanic *check the car"
	if (role & MAINOBJECT && roles[i-1] & OBJECT_COMPLEMENT) SetRole(i-1,MAININDIRECTOBJECT);
	if (role & POSTNOMINAL_ADJECTIVE)
	{
		if (roles[i-1] & (INDIRECTOBJECT2|MAININDIRECTOBJECT)) // cannot post describ indirect, becomes direct
		{
			needRoles[roleIndex] &= -1 & OBJECT_COMPLEMENT; // cannot have one if we are postnominal adj
			SetRole(i-1, (roles[i-1] & INDIRECTOBJECT2) ? OBJECT2 : MAINOBJECT,true);
		}
	}
}

static bool ProcessSplitNoun(unsigned int verb1,bool &changed)
{
	// 1 when we have dual valid verbs, maybe we swallowed a noun into a prep phrase that shouldnt have been.  (sequence: noun noun (ad:v) verb)
	// 2 Or We may have an inner implied clause -- the shawl people often wear isn't yellow ==  I hate the sh~awl people wear
	// 3 or a clause in succession when object side- to sleep is the thing eli wanted
	// 4 or a clause on subject side -  what they found thrilled them

	// 1 For 1st verb, see if we have  noun noun {adv} verb  formats (which implies noun should have been separated,
	unsigned int before = verb1;
	if (posValues[verb1] & VERB_BITS) // past tense needs to go to participle
	{
		while (--before > 0) 
		{
			if (posValues[before] & ADVERB) continue; // ignore this
			// find the subject just before our 2nd verb
			if (posValues[before] & (NOUN_BITS - NOUN_GERUND - NOUN_NUMBER + PRONOUN_SUBJECT + PRONOUN_OBJECT) &&
				posValues[before-1] & (NOUN_BITS - NOUN_GERUND - NOUN_NUMBER  + PRONOUN_SUBJECT + PRONOUN_OBJECT))
			{ // misses, the shawl the people wear
				if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"split noun @ %s(%d) to %s(%d)\r\n",wordStarts[before],before,wordStarts[verb1],verb1);

				if (posValues[before] & (PRONOUN_BITS)) // pronoun will already be split, but might be a clause starter
				{
					if (parseFlags[before-1]  & POTENTIAL_CLAUSE_STARTER)
					{
						SetRole(before,SUBJECT2,true);
						phrases[before] = 0;
						SetRole(verb1, VERB2,true);
						if (objectRef[verb1]) SetRole(objectRef[verb1],OBJECT2,true);
						--before; // subsume starter for clause
					}
					else
					{
						SetRole(before,SUBJECT2,true);
						phrases[before] = 0;
						SetRole(verb1,VERB2,true);
						if (objectRef[verb1]) SetRole(objectRef[verb1],OBJECT2,true);
					}
				}
				else if (posValues[before-1] & (PRONOUN_BITS))// pronoun will already be split
				{
					SetRole(before,SUBJECT2,true);
					phrases[before] = 0;
					SetRole(verb1,VERB2,true);
					if (objectRef[verb1]) SetRole(objectRef[verb1],OBJECT2,true);
				}
				else 
				{
					SetRole(before-1,roles[before]);
					SetRole(before,SUBJECT2,true);
					phrases[before] = 0;
					SetRole(verb1,VERB2,true);
					if (objectRef[verb1]) SetRole(objectRef[verb1],OBJECT2,true);
				}

				// if we find an entire given clause with starter, add the starter "what they found thrilled them"  
				while (before <= verb1) clauses[before++] |= clauseBit;
				clauseBit <<= 1;
				changed = true;
				return true;	
			}
			else break;
		}
	}
	return false;
}

static bool ProcessImpliedClause(unsigned int verb1,bool &changed) 
{
	if (clauses[verb1]) return false;	// already in a clause
	if (roles[verb1] & MAINVERB) return false;	// assume 1st is main verb

	unsigned int subject = verb1 - 1;
	if ( posValues[subject] & (AUX_VERB|ADVERB)) --subject;

	if (roles[subject] & MAINOBJECT) // implied clause as direct object  "I hope (Bob will go)"
	{
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"implied direct object clause @ %s(%d)\r\n",wordStarts[subject],subject);
		SetRole(subject,SUBJECT2|MAINOBJECT,true);
		SetRole(verb1,VERB2,true);
		while (subject <= verb1) clauses[subject++] |= clauseBit;
		clauseBit <<= 1;
		changed = true;
		return true;	
	}
	return false;
}

static bool ProcessCommaClause(unsigned int verb1,bool &changed)
{
	if (!clauses[verb1]) return false; 
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Comma clause\r\n");
	needRoles[roleIndex] &= -1 ^ CLAUSE;
	unsigned int subject = verb1;
	while (--subject && posValues[subject] != COMMA);
	while (++subject <= verb1) clauses[subject] |= clauseBit;
	clauseBit <<= 1;
	return true;	
}

static unsigned int ForceValues(int i, uint64 bits,char* msg,bool& changed)
{
	uint64 old = posValues[i];
	posValues[i] = bits;
	if (old != posValues[i])
	{
		changed = true;
		bitCounts[i] = BitCount(posValues[i]);
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Force \"%s\"(%d) %s\r\n",wordStarts[i],i,msg);
	}
	return bitCounts[i];
}

static bool AcceptsInfinitiveObject(int i)
{
	return (canSysFlags[i] & (VERB_TAKES_INDIRECT_THEN_TOINFINITIVE |VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE) ) ? true : false;
}

static bool ProcessReducedPassive(unsigned int verb1,bool allowObject,bool &changed)
{
	if (clauses[verb1]) return false;	// already in a clause
	if (objectRef[verb1] && !allowObject)  // be pessimistic, we dont want the main verb taken over
	{
		// the man given the ball is ok.
		// but the man dressed the cow is not - AND normal verbs might have objects
		// but "called" "named" etc create subjects of their objects and are ok.
		return false;
	}

	// or a past participle clause immediately after a noun --  the woman dressed in red
	// Or past particple clause starting sentence (before subject)  Dressed in red the woman screamed.   OR  In the park dressed in red, the woman screamed
	// OR -- the men driven by hunger ate first (directly known participle past)
	char* base = GetInfinitive(wordStarts[verb1],false);
	if (!base) return false;
	char* pastpart = GetPastParticiple(base);
	if (!pastpart) return false;
	if (stricmp(pastpart,wordStarts[verb1])) return false;	// not a possible past particple

	unsigned int before = verb1;
	if (posValues[verb1] & (VERB_PAST | VERB_PAST_PARTICIPLE)) 
	{
		bool causal = false;
		if (AcceptsInfinitiveObject(verb1) )
		{
			while (--before > 0) // past tense needs to go to participle
			{
				if (posValues[before] & ADVERB) continue; // ignore this
				if (posValues[before] & (NOUN_BITS - NOUN_GERUND - NOUN_NUMBER + PRONOUN_SUBJECT + PRONOUN_OBJECT)) 
				{
					if (phrases[before]) before = 0;	// cannot take object of prep and make it subject of clause directly
					break;
				}
				if (posValues[before] & COMMA) continue;
				return false;	// doesnt come after a noun or pronoun
			}

			// should this be main verb due to made me do it verb "the devil *made me do it"  causal verbs
			int after = verb1;
			while (++after < endSentence)
			{
				if (ignoreWord[after]) continue;
				if (canSysFlags[after] & ANIMATE_BEING || IsUpperCase(*wordStarts[after]))
				{
					if (posValues[after+1] & (TO_INFINITIVE|VERB_INFINITIVE)) break;
				}
				else if (posValues[after] & (DETERMINER|ADJECTIVE_BITS)) continue;
				else break;
			}
			if (canSysFlags[verb1] & (VERB_TAKES_INDIRECT_THEN_TOINFINITIVE|VERB_TAKES_TOINFINITIVE)  && posValues[after+1] & TO_INFINITIVE) causal = true; 
			else if (canSysFlags[verb1] & (VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE|VERB_TAKES_VERBINFINITIVE) && posValues[after+1] & VERB_INFINITIVE) causal = true; 
		}

		if (!causal) // "the devil *made me do it" blocks this reduction to a clause
		{
			// before might be 0 when clause at start of sentence
			LimitValues(verb1,VERB_PAST_PARTICIPLE,(char*)"reduced passive",changed);
			SetRole(verb1,VERB2,true);
			if (objectRef[verb1]) SetRole(objectRef[verb1],OBJECT2,true);
			clauses[verb1] |= clauseBit;
			clauseBit <<= 1;
			return true;	
		}
	}
	// alternative is we have coordinating sentences we didnt know about
	// defines the object, we come after main verb - We like the boy Eli *hated -   but "after he left home *I walked out
	// clause defines the subject, we come before main verb - The boy Eli hated *ate his dog - boy was marked as subject, and hated as mainverb.
	return false;
}

static void AddClause(int i,char* msg)
{
	// if we found a verb and still want a subject, seeing this clause--- it's not coming!  "under the bed is *where I am"
	if (needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT) && !(needRoles[roleIndex] & (VERB2|MAINVERB))) needRoles[roleIndex] &= -1 ^ (SUBJECT2|MAINSUBJECT);
	// be the needed object if one is wanted
	if (!(posValues[i] & CONJUNCTION_COORDINATE) && needRoles[roleIndex] & (OBJECT2|MAINOBJECT)  && !(parseFlags[i] & CONJUNCTIONS_OF_ADVERB) ) 
	 // clause should not intervene before direct object unless describing indirect object
	{ 
		SetRole(i,needRoles[roleIndex] & (OBJECT2|MAINOBJECT));
		needRoles[roleIndex] &= -1 ^ OBJECT_COMPLEMENT; // in addition remove this
	}
	if (trace & TRACE_POS) Log(STDTRACELOG,msg,wordStarts[i]);
	if (!clauses[i])
	{
		clauses[i] |= clauseBit;
		clauseBit <<= 1;
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"    Clause added at %s(%d)\r\n",wordStarts[i],i);
	}

	if (clauses[i] && clauses[i] != clauses[i-1]) // a new clause runs thru here
	{
		if (posValues[i-1] & ADJECTIVE_BITS) SetRole(i,ADJECTIVE_COMPLEMENT);
		AddRoleLevel(CLAUSE|SUBJECT2|VERB2,i); // start of a clause triggers a new level for it, looking for subject and verb (which will be found).
		if (!stricmp(wordStarts[i],"that") || !stricmp(wordStarts[i],"who")) // fibers "that" show up
		{
			if (posValues[i+1] & (VERB_BITS | AUX_VERB)) needRoles[roleIndex] &= -1 ^ SUBJECT2; // could be a verb
		}
		// possibly the start is the subject (or object) or possibly not. cannot tell in advance usually.
		if (!firstNounClause) // pending noun clause as subject for verb
		{					
			WORDP D = FindWord(wordStarts[i]);
			if (D && D->systemFlags & CONJUNCT_SUBORD_NOUN) firstNounClause = i; 
		}
		lastClause = i;		// where last clause was, if any
	}
}

static void ExtendChunk(int from, int to,int chunk[MAX_SENTENCE_LENGTH])
{
	int unit = chunk[from];
	if (!unit) unit = chunk[to]; 
	if (unit) while (from <= to) chunk[from++] |= unit;
}
	
// commas occur after a phrase at start, after an inserted clause or phrase, and in conjunction lists, and with closing phrases

static int FindPriorVerb(int at)
{
	while (--at)
	{
		if (ignoreWord[at]) continue;
		if (roles[at] & (VERB2|MAINVERB)) break;
	}
	return at;
}

static int FindPriorNoun(int i)
{
	int priorNoun = i;
	while (--priorNoun && (!(posValues[priorNoun] & (NORMAL_NOUN_BITS|PRONOUN_BITS)) || ignoreWord[priorNoun])) {;}
	return priorNoun;
}

static bool ImpliedNounObjectPeople(int i, uint64 role,bool & changed) // for object
{
	if (!role || !(posValues[i] & ADJECTIVE_NORMAL)) return false;
	if ((stricmp(wordStarts[i-1],(char*)"the") && !(posValues[i-1] & PRONOUN_POSSESSIVE)) && !(parseFlags[i-1] & TIME_NOUN)) return false; // on her own   or  the rich live in hell or "my six year *old"
	int at = i;
	while (++at <= endSentence && posValues[at] & ADVERB) {;}	// the wicked very often *can outwit others"
	if (at > endSentence || posValues[at] & (VERB_BITS|AUX_VERB_TENSES|PREPOSITION|TO_INFINITIVE))
	{
		LimitValues(i,NOUN_ADJECTIVE,(char*)"adjective acting as noun",changed);
		SetRole(i,role);
	}
	return changed;
}

static void MigrateObjects(int start, int end)
{
	// migrate direct objects into clauses and verbals....  (should have been done already) and mark BY-objects
	for (int i = start; i <= end; ++i)
	{
		if (ignoreWord[i]) continue;
		if (!objectRef[i]) continue; // nothing interesting
		int object;
		int at;
		if (verbals[i])
		{
			int phrase = phrases[i];	// verbal might be object of a phrase
			at = i;

			// see if it has an object also...spread to cover that...
			while (at && (object = objectRef[at]) && object > at)
			{
				ExtendChunk(at, object, verbals);
				if (phrase) ExtendChunk(at, object, phrases);
				at = objectRef[at]; // extend to cover HIS object if he is gerund or infintiive
			}
		}
		if (clauses[i])
		{
			at = i;
			// see if it has an object also...spread to cover that...
			while (at && (object = objectRef[at]) && object > at)
			{
				ExtendChunk(at, object, clauses);
				at = objectRef[object]; // extend to cover HIS object
			}
		}
		if (phrases[i])
		{
			at = i;
			object = objectRef[at];
			if (!stricmp(wordStarts[i], (char*)"by") && object && roles[object] & OBJECT2) roles[object] |= BYOBJECT2;
			if (!stricmp(wordStarts[i], (char*)"of") && object && roles[object] & OBJECT2) roles[object] |= OFOBJECT2;
			// see if it has an object also...spread to cover that... "after eating *rocks"
			while (at && (object = objectRef[at]) && object > at)
			{
				ExtendChunk(at, object, phrases);
				at = objectRef[at]; // extend to cover HIS object
			}
		}
	}
}

static bool FinishSentenceAdjust(bool resolved, bool & changed, int start, int end)
{
	if (verbStack[MAINLEVEL] > end)
	{
		ReportBug((char*)"mainverb out of range in sentence %d>%d", verbStack[MAINLEVEL], end);
		return true;
	}
	if (subjectStack[MAINLEVEL] > end)
	{
		ReportBug((char*)"mainSubject out of range", subjectStack[MAINLEVEL], end);
		return true;
	}
	if (ImpliedNounObjectPeople(end, needRoles[roleIndex] & (MAINOBJECT | OBJECT2), changed)) return false;

	////////////////////////////////////////////////////////////////////////////////
	///// This part can revise pos tagging and return false (reanalyze sentence)
	////////////////////////////////////////////////////////////////////////////////


	///////////////////////////////////////////////////////////////////////////
	// CLOSE OUT NEEDS
	///////////////////////////////////////////////////////////////////////////
	int subj = subjectStack[MAINLEVEL];

	// we still need a main verb--- might this have been command - "talk about stuff"
	if (needRoles[MAINLEVEL] & MAINVERB)
	{
		int at = start;
		if (posValues[at] & ADVERB) ++at;	// skip over 1st word, trying for 2nd
		if (allOriginalWordBits[at] & VERB_INFINITIVE)
		{
			bool missingObject = false;
			// if after is unheaded noun, then we cant change this from adjective... "*brave old dog"
			int noun = at;
			while (++noun <= endSentence)
			{
				if (ignoreWord[noun]) continue;
				if (!(posValues[noun] & (DETERMINER | ADJECTIVE_BITS | ADVERB | POSSESSIVE))) break;
			}
			if (posValues[noun] & NOUN_BITS)
			{
				int det;
				if (!IsDeterminedNoun(noun, det)) // we CANT change this
				{
					resolved = false;
					return true; // we cant do more
				}
			}
			else if (!(canSysFlags[at] & VERB_NOOBJECT)) missingObject = true;

			if (!missingObject)
			{
				LimitValues(at, VERB_INFINITIVE, (char*)"missing main verb, retrofix to command or verb question at start", changed);
				SetRole(at, MAINVERB);
				int v = verbals[at];
				int i = 0;
				while (verbals[++i] == v) verbals[i] ^= v; // rip out the verbal
				resolved = false;
				return false;
			}
		}

		// maybe we made a noun instead of verb-- "the good *die young" OR used adjective which could have been noun-ized "the six year old swims"
		if (subj)
		{
			int start = subj;
			while (posValues[start - 1] & ADJECTIVE_NOUN) --start; // back to start of nouns...
			if (allOriginalWordBits[start] & VERB_BITS && posValues[start - 1] & ADJECTIVE_NORMAL && posValues[start - 2] & DETERMINER)
			{
				SetRole(subj, 0, true);
				LimitValues(start, VERB_BITS - VERB_INFINITIVE, (char*)"missing verb recovered from bad noun1", changed);
				if (posValues[start - 1] & ADJECTIVE_NOUN) {
					posValues[start - 1] ^= ADJECTIVE_NOUN;
					posValues[start - 1] |= allOriginalWordBits[start - 1] & (NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL);
				}
				while (++start <= subj)
				{
					if (ignoreWord[start]) continue;
					LimitValues(start, 0, (char*)"retrying all bits after verb flipover", changed);
				}
				resolved = false;
				return false;
			}
			else if (allOriginalWordBits[subj] & VERB_BITS && allOriginalWordBits[subj - 1] & NOUN_BITS) // the six year old *swims
			{
				SetRole(subj, 0, true);
				LimitValues(subj, VERB_BITS - VERB_INFINITIVE, (char*)"missing verb recovered from bad noun2", changed);
				if (posValues[subj - 1] & ADJECTIVE_NOUN) {
					posValues[subj - 1] ^= ADJECTIVE_NOUN;
					posValues[subj - 1] |= allOriginalWordBits[subj - 1] & (NOUN_SINGULAR | NOUN_PLURAL | NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL);
				}
				resolved = false;
				return false;
			}
		}
	}
	if (needRoles[roleIndex] & PHRASE && posValues[endSentence] & PREPOSITION && roles[startSentence] & OBJECT2) --roleIndex;


	// need a subject, can thing before verb be it "the *old swims"
	int verb = verbStack[roleIndex];
	if (verb && !subj)
	{
		if (ImpliedNounObjectPeople(verb - 1, (roleIndex == MAINLEVEL) ? MAINSUBJECT : SUBJECT2, changed)) return false;
	}

	while (needRoles[roleIndex] & (OBJECT2 | MAINOBJECT | OBJECT_COMPLEMENT) || needRoles[roleIndex] == PHRASE || needRoles[roleIndex] == CLAUSE || needRoles[roleIndex] == VERBAL) // close of sentence proves we get no final object... 
	{
		if (needRoles[roleIndex] & (VERB2 | MAINVERB | MAINSUBJECT | SUBJECT2) || needRoles[roleIndex] == (PHRASE | OBJECT2)) // we HAVE NOTHING finished here
		{
			resolved = false;
			break;
		}
		if (needRoles[roleIndex] & CLAUSE && lastClause) ExtendChunk(lastClause, end, clauses);
		if (needRoles[roleIndex] & VERBAL && lastVerbal) ExtendChunk(lastVerbal, end, verbals);
		if (needRoles[roleIndex] & PHRASE && lastPhrase) ExtendChunk(lastPhrase, end, phrases);

		DropLevel(); // close out level that is complete
		if (roleIndex == 0) // put back main sentence level
		{
			++roleIndex;
			break;
		}
	}

	// inverted subject complement "how truly *fine he looks"
	if (!stricmp(wordStarts[startSentence], (char*)"how") && posValues[startSentence + 1] & ADJECTIVE_BITS && !roles[startSentence + 1])
	{
		LimitValues(startSentence + 1, ADJECTIVE_BITS, (char*)"inverted subject complement is adjective", changed);
		SetRole(startSentence + 1, SUBJECT_COMPLEMENT);
	}
	if (!stricmp(wordStarts[startSentence], (char*)"how") && posValues[startSentence + 1] & ADVERB && posValues[startSentence + 2] & ADVERB && !roles[startSentence + 2])
	{
		LimitValues(startSentence + 2, ADJECTIVE_BITS, (char*)"inverted subject complement is adjective", changed);
		SetRole(startSentence + 2, SUBJECT_COMPLEMENT);
	}

	///////////////////////////////////////////////////////////////
	//  DOWNGRADE ITEMS
	///////////////////////////////////////////////////////////////

	// check clause completion, may have to switch to prep
	int currentClause = 0;
	int currentVerbal = 0;
	int currentVerb = 0;
	int commas = 0;
	int mainVerb = 0;
	currentMainVerb = 0;
	currentVerb2 = 0;

	for (int i = start; i <= end; ++i)
	{
		if (ignoreWord[i]) continue;
		if (roles[i] == TAGQUESTION) break;		// not interested in what happens after this
		if (!clauses[i])
		{
			if (currentClause) 
			{
				if (!currentVerb) // clause is a problem...  (we do allow missing subject when verb is a gerund) - AND a clause started with "than" can omit subject and verb
				{
					if (!stricmp(wordStarts[currentClause],(char*)"than")){;} // "he is taller than in October"  omits "he was"
					else if (allOriginalWordBits[currentClause] & PREPOSITION) // change it over
					{
						LimitValues(currentClause,PREPOSITION,(char*)"clause missing verb, changing over to preposition",changed);
						for (int x = currentClause; x <= i; ++x) clauses[x] = 0; // rip out clause
					}
				}
				// we ended the clause, check it
				currentClause = 0;
				currentVerb = 0;
			}
		}
		else if (!currentClause) currentClause = i;
		else if (posValues[i] & NOUN_GERUND && !currentVerb) currentVerb = i; // may be noun_gerund
		else if (roles[i] & VERB2 && !currentVerb) currentVerb = i; // may be noun_gerund

		if (!verbals[i])
		{
			if (currentVerbal) currentVerbal = 0;
		}
		else if (!currentVerbal) currentVerbal = i;


		// downgrade indirect object to direct when none there
		if (roles[i] & MAINVERB)
		{
			currentMainVerb = i;
			if (indirectObjectRef[i] && !objectRef[i]) SetRole(indirectObjectRef[i],MAINOBJECT,true,i);
			
			// if start is COMMAND and seems to have object but its not headed AND start might have been adjective- "*brave old dog"
			if (!subjectStack[MAINLEVEL] && posValues[i] & VERB_INFINITIVE  && allOriginalWordBits[i] & ADJECTIVE_NORMAL)
			{
				int obj = objectRef[i];
				int det;
				if (obj && !IsDeterminedNoun(obj,det))
				{
					LimitValues(i,ADJECTIVE_NORMAL,(char*)"command has undetermined object, become adjective phrase instead",changed);
					SetRole(i,0,true,i);
					SetRole(obj,0,true,i);
					return false; 
				}
			}
		}
		else if (roles[i] & VERB2)
		{
			if (indirectObjectRef[i] && !objectRef[i]) SetRole(indirectObjectRef[i],OBJECT2,true,i); // change object 
		}

		// when something was possible adjective participle over adjective, we accepted it. Now downgrade if has no object attached 
		// Particularly when participle describes a condition, not an action
		if (posValues[i] == ADJECTIVE_PARTICIPLE && !objectRef[i] && allOriginalWordBits[i] & ADJECTIVE_NORMAL && !(posValues[NextPos(i)] & PARTICLE)) // not "I found her *worn out"
		{
			LimitValues(i,ADJECTIVE_NORMAL,(char*)"adjective participle sans object downgraded to normal adjective",changed);
			roles[i] &= -1 ^ VERB2;
			verbals[i] = 0;
		}

		// remaining conflict adj/adv resolve to adverb "let us stand *still, and hear"
		if (posValues[i] & ADJECTIVE_BITS && posValues[i] & ADVERB)
		{
			if (posValues[NextPos(i)] & NOUN_BITS && posValues[i] & DETERMINER && !(posValues[i-1] & (ADJECTIVE_BITS|DETERMINER))) LimitValues(i,DETERMINER,(char*)"adj/adv/det pending resolved by being adjective in front of noun when not after adjective",changed); // "*no price for the new shares has been set" 
			else if (posValues[NextPos(i)] & (NOUN_BITS|PRONOUN_BITS)) LimitValues(i,ADJECTIVE_BITS,(char*)"adj/adv pending resolved by being adjective in front of noun",changed);
			else if (canSysFlags[i] & ADJECTIVE_NOT_PREDICATE)  LimitValues(i,-1 ^ ADJECTIVE_BITS,(char*)"adj/adv pending resolved by adv suitable for subject complement",changed);
			// if verb is go then do not accept adj if could be adverb
			else if (canonicalLower[currentVerb] && !stricmp(canonicalLower[currentVerb]->word,(char*)"go"))
			{
				LimitValues(i, ADVERB,(char*)"for verb go we prefer adverb instead of subject complement",changed);
			}
			// do we still have a pending main need
			else  if (needRoles[MAINLEVEL] & SUBJECT_COMPLEMENT && roles[i-1] & MAINVERB)
			{
				LimitValues(i, ADJECTIVE_BITS,(char*)"we needed subject complement, so fill that in",changed);
			}
			
			else if (!verbStack[MAINLEVEL]) 
			{
				if (wordCount == 1) {;} // who knows...  eg.   "*pretty"
				else if (canSysFlags[i] & ADVERB) LimitValues(i,ADVERB,(char*)"adj/adv pending resolved by being probable adverb given no main verb exists",changed); // " Good!"
				else LimitValues(i,ADJECTIVE_BITS,(char*)"adj/adv pending resolved by being probable adjective given no main verb exists",changed); // " Good!"
			}
			else LimitValues(i,-1 ^ ADJECTIVE_BITS,(char*)"adj/adv pending resolved by rejecting adjective",changed);
		}

		// BUG  when something was participle VERB but had no object and could have been subject complement of "be" verb, alter it...
		if (false && roles[i] & MAINVERB && posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && allOriginalWordBits[i] & ADJECTIVE_NORMAL && !objectRef[i] && posValues[i-1] & AUX_BE)
		{
			LimitValues(i,ADJECTIVE_NORMAL,(char*)"verb tense with no object and could have been subject complement adjective, downgrade to normal adjective",changed);
			SetRole(i,SUBJECT_COMPLEMENT,true,i-1);

			uint64 tense = allOriginalWordBits[i-1] & VERB_BITS;
			if (tense & VERB_PRESENT) LimitValues(i-1,VERB_PRESENT,(char*)"convert aux to present",changed);
			else if (tense & VERB_PRESENT_3PS) LimitValues(i-1,VERB_PRESENT_3PS,(char*)"convert aux to present 3ps",changed);
			else if (tense & VERB_PAST) LimitValues(i-1,VERB_PAST,(char*)"convert aux to past",changed);
			else if (tense & VERB_PAST_PARTICIPLE) LimitValues(i-1,VERB_PAST_PARTICIPLE,(char*)"convert aux to past particple",changed);
			SetRole(i-1,MAINVERB,true,i-1);
		}
		if (posValues[i] == COMMA) ++commas;
		if (!(roles[i] & MAINVERB) && posValues[i] & VERB_BITS && !mainVerb  && !clauses[i] && !phrases[i] && !verbals[i]) mainVerb = i;  // 1st actual main verb if role not given yet
	
		// update after things, because VERB2 can be used as object of earlier VERB2 before becoming verb2 in its own right "let us stop and hear him *play the guitar"
		if (roles[i] & VERB2) currentVerb2 = i;

		if (clauses[i] && posValues[i] == VERB_PRESENT)
			LimitValues(i, VERB_INFINITIVE, (char*)"change present tense in clause to infinitive", changed);
	}

	// was aux assigned as main verb, now we have actual verb?
	int assignedMainVerb = verbStack[MAINLEVEL]; // 1st verb
	if (assignedMainVerb &&  mainVerb && !roles[mainVerb] && allOriginalWordBits[assignedMainVerb] & (AUX_VERB_TENSES|AUX_BE|AUX_HAVE|AUX_DO)) // we dont know what this extraneous verb is doing
	{
		SetRole(assignedMainVerb,0);
		LimitValues(assignedMainVerb,allOriginalWordBits[assignedMainVerb] & (AUX_VERB_TENSES|AUX_BE|AUX_HAVE|AUX_DO),(char*)"changing aux from mainverb to aux",changed);
		LimitValues(mainVerb,allOriginalWordBits[mainVerb] & VERB_BITS,(char*)"changing mainverb from unknown",changed);
		SetRole(mainVerb,MAINVERB,true,mainVerb);
		assignedMainVerb = mainVerb;
	}

	if (assignedMainVerb) mainVerb = assignedMainVerb;
	else if (mainVerb)
	{
		SetRole(mainVerb,MAINVERB,true,mainVerb); 
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Forcing found verb into main verb \"%s\"(%d)\r\n",wordStarts[mainVerb],mainVerb);
		if (objectRef[mainVerb] ) SetRole(objectRef[mainVerb],MAINOBJECT,true,mainVerb);
	}
	// found no main verb? resurrect a clause into main verb - prefer removal of clause modifying prep object
	else 
	{
		for (int i = start; i <= end; ++i)
		{
			if (ignoreWord[i]) continue;
			if (posValues[i] & VERB_BITS && clauses[i] && phrases[i-1]) // verb w/o a role?
			{
				SetRole(i,MAINVERB,false,i); 
				if ( objectRef[i] ) SetRole(objectRef[i],MAINOBJECT,false,i);
				clauses[i] = 0;
				if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Lacking main verb, resurrect clause into main verb \"%s\"(%d)\r\n",wordStarts[i],i);
				break;
			}
		}
	}

	int assignedMainSubject = subjectStack[MAINLEVEL]; // 1st subject
	int assignedMainObject = objectRef[assignedMainVerb];
	
	// we have no mainverb or mainsubject, but we started with coordinating conjunction so downgrade to adverb if we can...
	if (!mainVerb && !assignedMainSubject && posValues[startSentence] & CONJUNCTION && allOriginalWordBits[startSentence] & ADVERB && !(posValues[startSentence-1] & CONJUNCTION_COORDINATE))
	{ // not "compound yields assume reinvestment of dividents and *that the current yield continues for a year"
		// but "that year they died"
		if (allOriginalWordBits[startSentence] & DETERMINER && posValues[startSentence+1] & (NOUN_BITS|ADJECTIVE_BITS))
		{
			LimitValues(startSentence,DETERMINER,(char*)"changing coordconjunct to determiner since no main sentence but have noun or adj following",changed);
		}
		else
		{
			LimitValues(startSentence,ADVERB,(char*)"changing coordconjunct to adverb since no main sentence",changed);
		}
		int c = clauses[startSentence];
		int at = startSentence - 1;
		while (clauses[++at] == c)
		{
			if (roles[at] & SUBJECT2 && !assignedMainSubject) 
			{
				SetRole(at,MAINSUBJECT);
				subjectStack[MAINLEVEL] = (unsigned char)at;
				assignedMainSubject = at;
			}
			if (roles[at] & VERB2 && !assignedMainVerb) 
			{
				SetRole(at,MAINVERB);
				verbStack[MAINLEVEL] = (unsigned char)at;
				mainVerb = assignedMainVerb = at;
				if (indirectObjectRef[at]) SetRole(indirectObjectRef[at],MAININDIRECTOBJECT,false,at);
				int object = objectRef[at];
				if (object) 
				{
					if (needRoles[MAINLEVEL] & MAINSUBJECT) 
					{
						SetRole(object,MAINSUBJECT,true,at);
						if (posValues[object] & PRONOUN_OBJECT) LimitValues(object,PRONOUN_SUBJECT,(char*)"discovered subject changes pronoun",changed);
					}
					else SetRole(object,MAINOBJECT,true,at);
					objectRef[at] = 0;
				}
			}
			clauses[at] = 0;	// remove clause
		}
	}

	// no verb but have subject, and if we were coordinate conjunct that could have been prep, downgrade
	if (!mainVerb && assignedMainSubject && start != 1 && posValues[start-1] == CONJUNCTION_COORDINATE && 
		allOriginalWordBits[start-1] & PREPOSITION)
	{
		LimitValues(start-1,PREPOSITION,(char*)"no verb for conjunction, demote to prep",changed);
		SetRole(assignedMainSubject,OBJECT2,true,0);
		return false;
	}

	// main subject cant be in zone adjacent to verb, maybe appositve is bad: "Hamlet(A), the guy(S) who has to decide, decides(V) to stick it out for a while."
	if (mainVerb && assignedMainSubject && GetZone(assignedMainSubject) == (GetZone(mainVerb)-1))
	{
		int priorNoun = FindPriorNoun(assignedMainSubject);
		if (priorNoun && roles[priorNoun] & APPOSITIVE && GetZone(assignedMainSubject) == 1 && GetZone(priorNoun) == 0)
		{
			SetRole(priorNoun,MAINSUBJECT,true,0);
			SetRole(assignedMainSubject,APPOSITIVE,true,0);
			assignedMainSubject = priorNoun;
		}
	}

	// starting prep describes subject?
	if (posValues[start] & PREPOSITION && assignedMainSubject) crossReference[start] = (unsigned char)assignedMainSubject;

	if (assignedMainSubject && assignedMainVerb && (zoneMember[assignedMainVerb] - zoneMember[assignedMainSubject]) == 1 && clauses[assignedMainSubject]) // was this a clause and an imperative: (char*)"whoever comes to the door, ask them to wait"
	{
		if (allOriginalWordBits[assignedMainVerb] & VERB_INFINITIVE) // rewrite as imperative
		{
			SetRole(assignedMainSubject,SUBJECT2,true,0);
			LimitValues(assignedMainVerb,VERB_INFINITIVE,(char*)"clause is not subject, making sentence imperative",changed);
			assignedMainSubject = 0;
			subjectStack[MAINLEVEL] = 0;
		}
	}

	// here, Ned, jump out
	int zoneSubject = (assignedMainSubject) ? GetZone(assignedMainSubject) : 0;
	int zoneVerb = (assignedMainVerb) ? GetZone(assignedMainVerb) : 0;
	// "Hector, address the class"  is command, not subject, verb also "here, Ned, catch this"
	if (assignedMainSubject && assignedMainVerb && ( zoneVerb - zoneSubject) == 1 && allOriginalWordBits[assignedMainVerb] & VERB_INFINITIVE && (zoneBoundary[zoneVerb] + 1) == assignedMainVerb)
	{
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Forcing subject in leading comma zone to be address \"%s\"(%d)\r\n",wordStarts[startSentence],1);
		SetRole(assignedMainSubject,ADDRESS,true,0);
		LimitValues(assignedMainVerb,VERB_INFINITIVE,(char*)"Force verb to be infinitive on command",changed);
		assignedMainSubject = 0;
	}
	
	////////////////////////////////////////////////////////////////////////////////
	///// This part can no longer revise pos tagging, only roles
	////////////////////////////////////////////////////////////////////////////////
	
	// if we have no subject, have an object, and it's not a command verb, object must become subject (particularly for clauses): "under the bed is *where we looked"
	if (assignedMainObject && !assignedMainSubject && assignedMainVerb && !(posValues[assignedMainVerb] & VERB_INFINITIVE))
	{
		currentVerb = assignedMainVerb;
		SetRole(assignedMainObject,MAINSUBJECT,true,0);
		assignedMainSubject = assignedMainObject;
		assignedMainObject = 0;
	}

	for (int i = start; i <= end; ++i)
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & VERB_BITS && !roles[i]) // verb w/o a role?
		{
			ProcessCommaClause(i,changed); // clause bounded by commas?
			ProcessSplitNoun(i,changed); // see if is clause instead
			ProcessReducedPassive(i,false,changed); // we carefully avoid verbs with object in this pass
			ProcessImpliedClause(i,changed);
			if (!clauses[i]) 
			{
				ProcessReducedPassive(i,true,changed); // allow objects to be accepted
				ProcessOmittedClause(i,changed);		  // whole seemingly second sentence
			}
		}
	}	

	// sentence starting in qword, not in a clause. maybe flip subject and object 
	if (subjectStack[MAINLEVEL] &&  !clauses[startSentence] && verbStack[MAINLEVEL] && objectRef[verbStack[MAINLEVEL]] && (allOriginalWordBits[startSentence] & QWORD || (allOriginalWordBits[startSentence] & PREPOSITION && allOriginalWordBits[2] & QWORD) ))
	{
		//  WHAT (what did john eat == john did eat what) 
		// but not when WHO (who hit the ball != the ball hit who)
		bool flip = true;
		int subj = subjectStack[MAINLEVEL];
		// NEVER flip particple verbs:   who is hitting the ball. What is hitting the ball.  what squashed the ball
		if (posValues[verbStack[MAINLEVEL]] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE)) flip = false;
		else if (objectRef[verbStack[MAINLEVEL]] < subj) flip = false; // already flipped
		else if (!(allOriginalWordBits[subj] & QWORD)) 
		{
			if (posValues[startSentence] & DETERMINER && allOriginalWordBits[startSentence] & QWORD) flip = true;	// which cat did you hit
			else flip = false; // "why did *john hit the ball"
		}
		else if ((!stricmp(wordStarts[subj],(char*)"who") || !stricmp(wordStarts[subj],(char*)"which") || !stricmp(wordStarts[subj],(char*)"whoever") || !stricmp(wordStarts[subj],(char*)"whichever") || !stricmp(wordStarts[subj],(char*)"whatever") || !stricmp(wordStarts[subj],(char*)"what")|| !stricmp(wordStarts[subj],(char*)"how")) && !(parseFlags[verbStack[MAINLEVEL]] & VERB_TAKES_ADJECTIVE)) flip = false; // "*who will go" but not "who is president"
		// "which will bell the cat" does not flip but "which is the cat" will
		else flip = true;

		if (flip)
		{
			currentMainVerb = verbStack[MAINLEVEL];
			 int newsubject = objectRef[currentMainVerb];
			int newobject = subjectStack[MAINLEVEL];
			SetRole(newobject, MAINOBJECT,true,currentMainVerb);
			if (posValues[newobject] == PRONOUN_SUBJECT) posValues[newobject] = PRONOUN_OBJECT;
			SetRole(newsubject,MAINSUBJECT,true,currentMainVerb);
			if (posValues[newsubject] == PRONOUN_OBJECT) posValues[newsubject] = PRONOUN_SUBJECT;
			if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Flipping subject and object due to qword start \"%s\"(%d)\r\n",wordStarts[newobject],newobject);
		}
	}

	// sentence starting in qword and ending in prep- remove qword as main object and make it OBJECT2 of prep
	if (end == endSentence && allOriginalWordBits[startSentence] & QWORD && posValues[end] == PREPOSITION && roles[startSentence] & MAINOBJECT)
	{
		SetRole(startSentence,OBJECT2,false,verbStack[MAINLEVEL]);
		if (allOriginalWordBits[startSentence] & PRONOUN_OBJECT) posValues[startSentence] = PRONOUN_OBJECT;
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Pronoun wrap to start, coercing start to object2 and maybe pronoun object \"%s\"(%d)\r\n",wordStarts[startSentence],startSentence);
	}

	// sentence starting with here or there as subject
	if (subjectStack[MAINLEVEL] && objectRef[verbStack[MAINLEVEL]] && verbStack[MAINLEVEL] && (!stricmp(wordStarts[subjectStack[MAINLEVEL]],(char*)"here") || !stricmp(wordStarts[subjectStack[MAINLEVEL]],(char*)"there"))) // HERE and THERE can not be the subject of a sentence:  here is the pig.. BUT  here is nice is legal.
	{
		int subject = subjectStack[MAINLEVEL];
		int object = objectRef[verbStack[MAINLEVEL]];
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Here/There cannot be subject, revise object\"%s\"(%d)\r\n",wordStarts[object],object);
		SetRole(subject,0,false,0); 
		if (!stricmp(wordStarts[subject],(char*)"here"))  LimitValues(subject,ADVERB,(char*)"Here as subject is adverb",changed);
		else if (posValues[subject] == THERE_EXISTENTIAL) {;}
		else LimitValues(subject,ADVERB,(char*)"There as subject is adverb not existential",changed); 
		SetRole(object,MAINSUBJECT,true,0);
	}

	//////////////////////////////////////////////////////////////
	// HOOK UP CROSS REFERENCES NOT YET HOOKED (phrases, aux, verbals, clauses
	//////////////////////////////////////////////////////////////

	for ( int i = start; i <= end; ++i)
	{
		if (ignoreWord[i]) continue;
	
		// fix xref of aux to a verb
		if (posValues[i] & AUX_VERB)
		{
			for ( int j = i+1; j <= endSentence; ++j)
			{
				if (posValues[j] & VERB_BITS) // closest verb
				{
					crossReference[i] = (unsigned char) j;
					break;
				}
			}
		}
		if (posValues[i] & (PRONOUN_BITS|NOUN_BITS)) // find adjectives and determiners to bind to it
		{
			for ( int j = i-1; j >= start; --j)
			{
				if (roles[j] & OMITTED_TIME_PREP){;} // dont link that to time
				else if (posValues[j] & (DETERMINER|PREDETERMINER|ADJECTIVE_BITS)) crossReference[j] = (unsigned char)i; 
				else if (posValues[j] & ADVERB) crossReference[j] = (unsigned char)(j+1); 
				else break; // no more to see
			}
		}

		if (posValues[i] & PRONOUN_POSSESSIVE) // link to what it possesses
		{
			int at = i;
			while (!(posValues[++at] & NOUN_BITS) || ignoreWord[at]) {;}
			crossReference[i] = (unsigned char) at;
		}

		if (posValues[i] & POSSESSIVE) // link to what it possesses
		{
			int at = i;
			while (!(posValues[++at] & NOUN_BITS) || ignoreWord[at]) {;}
			crossReference[i] = (unsigned char) at;  // Bob 's dog   has possessive linked to dog
			crossReference[i-1] = (unsigned char) i;	// Bob 's dog   has bob linked to possessive
		}

		if (posValues[i] & ADVERB)
		{
			if (posValues[NextPos(i)] & NOUN_BITS && parseFlags[NextPos(i)] & QUANTITY_NOUN) crossReference[i] = (unsigned char)(i + 1); // "*over half are there"
			else if (posValues[Next2Pos(i)] & NOUN_BITS && parseFlags[Next2Pos(i)] & QUANTITY_NOUN && posValues[NextPos(i)] & DETERMINER) crossReference[i] = (unsigned char)(i + 2); // "*over a third are there"
			else if (posValues[i-1] & ADJECTIVE_BITS && roles[i-1] & SUBJECT_COMPLEMENT && parseFlags[i] & ADVERB_POSTADJECTIVE) crossReference[i] = (unsigned char)(i - 1); // "he is powerful *enough"
			else if (posValues[NextPos(i)] & (ADJECTIVE_BITS|ADVERB)) crossReference[i] = (unsigned char)(i + 1); // if next is ADJECTIVE/ADVERB, then it binds to that?  "I like *very big olives"
			else if (posValues[NextPos(i)] & VERB_BITS) //  next to next verb bind forward ((char*)"if this is tea please bring me sugar")
			{
				if (clauses[i+1]) clauses[i] = clauses[i+1];
				if (verbals[i+1]) verbals[i] = verbals[i+1];
				crossReference[i] = (unsigned char)(i + 1);
			}
			// adverb as object of prep...
			else if (posValues[i-1] & PREPOSITION && !stricmp(wordStarts[i-1], "from") && !(posValues[NextPos(i)] & (ADVERB|ADJECTIVE_BITS|DETERMINER_BITS|NOUN_BITS|PRONOUN_BITS)))
			{
				crossReference[i] = (unsigned char)(i + 1);
			}
			else // bind back to prior verb or forward to main verb 
			{
				if (clauses[i-1]) clauses[i] = clauses[i-1];
				if (verbals[i-1]) verbals[i] = verbals[i-1];
				if ( phrases[i-1]  && !phrases[i] && posValues[i-1] & NOUN_GERUND) phrases[i] = phrases[i-1];
				int prior = FindPriorVerb(i);
				if (prior) crossReference[i] = (unsigned char)prior;
				else if (i == startSentence && mainVerb) crossReference[i] = (unsigned char)mainVerb;
			}
		}

		// http://www.brighthubeducation.com/english-homework-help/46995-the-nominal-functions-of-prepositions-and-prepositional-phrases/
		// Phrases can act as adjective modifying a noun (ADJECTIVAL) , as an adverb modifying a verb (ADVERBIAL), or as a nominal when used in conjunction with "be".
		// The park is next to the hospital. The student is between an A and a B. The fight scene is before the second act.
		// The old farmhouse stood for years, after the revolution, by the fork in the road, beyond the orange grove, over the wooden bridge, at the farthest edge of the family's land, toward the great basin, down in the valley, under the old mining town, outside the city's limits, and past the end of the county maintained road.
		if (phrases[i] != phrases[i-1]  && phrases[i]) // start of a prep phrase, always needs hooking
		{
			int v = 0;
			bool adjectival = false;
			if (!(posValues[i] & PREPOSITION)) // is EITHER wrapped object from prep at end of clause or sentence, OR  its an ommited prep like omitted time prep
			{
				if (roles[i] & ABSOLUTE_PHRASE) adjectival = true;
				else if (i == 1 && roles[i] & OMITTED_TIME_PREP) // find first noun or verb to link onto
				{
					 int j = i;
					while (++j < endSentence && phrases[j] == phrases[j-1]){;}
					while (!(posValues[j] & VERB_BITS) || ignoreWord[j]) ++j;	 // find noun or verb
					v = j; // post verb
				}
				else if (roles[i] & OMITTED_TIME_PREP) v = FindPriorVerb(i); // xref is the verb BEFORE it
			}
			else
			{
				if (i == 1) // find first noun or verb to link onto
				{
					 int j = i;
					while (++j < endSentence && phrases[j] == phrases[j-1]){;}
					while (!(posValues[j] & (NOUN_BITS|PRONOUN_BITS|VERB_BITS)) || ignoreWord[j]) ++j;	 // find noun or verb
					if (posValues[j] & (NOUN_BITS|PRONOUN_BITS)) adjectival = true;
					else v = j; // post verb
				}
				else v = FindPriorVerb(i); // xref is the verb BEFORE it

				if (posValues[i-1] & ADJECTIVE_BITS && !(parseFlags[i-1] & NONDESCRIPTIVE_ADJECTIVE) ) 
				{
					SetRole(i,ADJECTIVE_COMPLEMENT,false,0); // "I was shocked *by the news"
					crossReference[i] = (unsigned char)(i - 1);
					if (clauses[i-1]) ExtendChunk(i-1,GetPhraseTail(i),clauses);
					if (verbals[i-1]) ExtendChunk(i-1,GetPhraseTail(i),verbals);
				}
				else if (posValues[i-1] & (NOUN_BITS - NOUN_GERUND) && canSysFlags[i] & LOCATIONWORD) // it COULD describe the preceeding noun
				{
					SetRole(i,ADJECTIVAL,false,0); // "I made a home *of wood"
					crossReference[i] = (unsigned char)(i - 1);	// linked to noun
					adjectival = true;
					if (clauses[i-1]) ExtendChunk(i-1,GetPhraseTail(i),clauses);
					if (verbals[i-1]) ExtendChunk(i-1,GetPhraseTail(i),verbals);
				}
				// phrase as object of prep from... "I called from *within the house"
				else if (posValues[i-1] & PREPOSITION && !stricmp(wordStarts[i-1], "from"))
				{
					ExtendChunk(i-1,i,phrases);
					crossReference[i] = (unsigned char)(i - 1);
					adjectival = true;
				}
			}
			if (!adjectival)
			{
				AddRole(i,ADVERBIAL);  // "after passing *through the area"
				// set range of roles to resolve when we see its object
				bool when = false;
				bool where = false;
				bool why = false;
				bool how = false;
				if (canSysFlags[i] & TIMEWORD && !(canSysFlags[i] & (HOWWORD|LOCATIONWORD ))) when = true;
				else if (canSysFlags[i] & LOCATIONWORD && !(canSysFlags[i] & (HOWWORD|TIMEWORD)) ) where = true;
				else if ((canSysFlags[i] & (LOCATIONWORD | TIMEWORD|HOWWORD )) == (LOCATIONWORD | TIMEWORD |HOWWORD) ) where = when = how = true; // needs closer inspection, might be either, decide when we see the object
				else if ((canSysFlags[i] & (LOCATIONWORD | TIMEWORD )) == (LOCATIONWORD | TIMEWORD ) ) when = where = true;// needs closer inspection, might be either, decide when we see the object
				else if ((canSysFlags[i] & (TIMEWORD|HOWWORD )) == (TIMEWORD |HOWWORD) ) when = how = true; // needs closer inspection, might be either, decide when we see the object
				else if ((canSysFlags[i] & (LOCATIONWORD|HOWWORD) ) == (LOCATIONWORD | HOWWORD) ) where = how = true; // needs closer inspection, might be either, decide when we see the object
				else how = true; // default is how if not when or where or why
				// if we dont know the prep role yet, assign it based on this object
				if (!(roles[i] & ADVERBIALTYPE) && when)
				{
					if (canonicalLower[i] && canonicalLower[i]->systemFlags & TIMEWORD) roles[i] |=  WHENUNIT;
				}
				if (!(roles[i] & ADVERBIALTYPE) && where)
				{
					if (canSysFlags[i] & LOCATIONWORD) roles[i] |= WHEREUNIT;
				}
				if (!(roles[i] & ADVERBIALTYPE)  && how) roles[i] |=  HOWUNIT;
				if (!(roles[i] & ADVERBIALTYPE)  && why) roles[i] |=  WHYUNIT;
				if (v) 
				{
					crossReference[i] = (unsigned char)v;
					if (clauses[v]) ExtendChunk(v,GetPhraseTail(i),clauses);
					if (verbals[v]) ExtendChunk(v,GetPhraseTail(i),verbals);
				}
			}
		}
		if (clauses[i] != clauses[i-1] && clauses[i] && !(roles[i] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT2))) // start of clause that needs hooking
		{
			if (roles[i-1] & (OBJECT2|MAINOBJECT|SUBJECT2|MAINSUBJECT)) // clause immediately after a noun item will be adjectival, describing that noun
			{
				crossReference[i] = (unsigned char) (i-1);
				AddRole(i,ADJECTIVAL);
			}
			else if (parseFlags[i] & CONJUNCTIONS_OF_ADVERB)
			{
				AddRole(i,ADVERBIAL);
				if (parseFlags[i] & CONJUNCTIONS_OF_TIME && !(parseFlags[i] & CONJUNCTIONS_OF_SPACE)) AddRole(i,WHENUNIT);
				if (!(parseFlags[i] & CONJUNCTIONS_OF_TIME) && parseFlags[i] & CONJUNCTIONS_OF_SPACE) AddRole(i,WHEREUNIT);
				
				// xref is the verb BEFORE it
				int v = FindPriorVerb(i);
				if (v) 
				{
					crossReference[i] = (unsigned char)v;
					if (verbals[v]) ExtendChunk(v,GetClauseTail(i),verbals);
				}
			}
		}

		// verbals (infintives, gerunds, participles)  can be adjective (participles are always adjectives) or adverb or noun (gerunds are always nounds)
		if (verbals[i] != verbals[i-1] && verbals[i])
		{
			if (coordinates[i] && coordinates[i] < i)  // cover chunk to here?
			{
				if (clauses[coordinates[i] ]) ExtendChunk(coordinates[i] ,GetVerbalTail(i),clauses); // cover us from same clause
			}
			while (i <= endSentence && (!(roles[i] & VERB2) || ignoreWord[i])) ++i;	// find the verb to annotate
			if (roles[i] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT2|SUBJECT_COMPLEMENT|APPOSITIVE|OBJECT_COMPLEMENT)) {;} // already hook into role
			else if (roles[i-1] & (OBJECT2|MAINOBJECT|SUBJECT2|MAINSUBJECT|SUBJECT_COMPLEMENT)) // verbal  immediately after a noun item (or subject complement adjective) will be adjectival, describing that noun
			{
				crossReference[i] = (unsigned char) (i-1);
				AddRole(i,ADJECTIVAL);
				if (clauses[i-1]) ExtendChunk(i-1,GetVerbalTail(i),clauses);
			}
			else if (posValues[i] & NOUN_GERUND) 
			{
				// gerund a noun , marked as object2, mainsubject, etc
			}
			else if (posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE))
			{
				// might lead off sentence, describing subject -- "Hidden by the trees, Jerry waited to scare Mark."
				for (int j = start; j<= endSentence; ++j)
				{
					if (verbals[j] || phrases[j]) continue;
					if (posValues[j] & (NOUN_BITS|PRONOUN_BITS))
					{
						crossReference[i] = (unsigned char) j;
						AddRole(i,ADJECTIVAL);
					}
					// else ReportBug((char*)"participle verbal must be adjective");
				}
			}
			else
			{
				AddRole(i,ADVERBIAL);
				int v = FindPriorVerb(i);
				if (v) 
				{
					crossReference[i] = (unsigned char)v;
					if (clauses[v]) ExtendChunk(v,GetVerbalTail(i),clauses);
				}
			}
		}
	}
	MigrateObjects(start, end);
	
	// if we have OBJECT2 not in a clause or phrase.... maybe we misfiled it. "Hugging the ground, Nathan peered."
	if (resolved) for ( int i = start; i <= end; ++i)
	{
		if (ignoreWord[i]) continue;
		if (!(roles[i] & OBJECT2) || phrases[i] || clauses[i] || verbals[i]) continue;
		if (subjectStack[MAINLEVEL] && subjectStack[MAINLEVEL] < i && posValues[subjectStack[MAINLEVEL]] == NOUN_GERUND) // the Verbal will be an adjective phrase on the main subject
		{
			SetRole(i,MAINSUBJECT,false,0);
			SetRole(subjectStack[MAINLEVEL],SUBJECT2,false,0);
		}
		// if subject is 1st word, after it is clause, and comma, then our phrase, migrate into clause -- those dressed in red, the men ate first
		if (subjectStack[MAINLEVEL] == 1 && clauses[start+1] && i < verbStack[MAINLEVEL]) // just assume it is right for now if is before main verb
		{
			SetRole(subjectStack[MAINLEVEL],SUBJECT2,false,0); // establishs transient subjectstack on wrong level but is fixed by next setrole
			SetRole(i, MAINSUBJECT,false,0);
			clauses[startSentence] = clauses[startSentence+1];
		}
	}

	roles[end] |= SENTENCE_END;  // we mark end of sentence
	return true;
}

static int FindCoordinate(int i) // i is on comma
{
	int comma2 = 0;
	while (++i < endSentence)
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & COMMA) comma2 = i; // there is a comma
		if (posValues[i] & CONJUNCTION_COORDINATE)
		{
			if (comma2 == (i-1)) return i; // conjunction immediately after a second comma is a clear end of sequence
		}
	}
	return -1; // didnt find
}

static void DoCoord( int i,  int before,  int after, uint64 type)
{
	SetRole(i,type);	
	if (type != CONJUNCT_SENTENCE) SetRole(after,roles[before]); // pass along any earlier role
	crossReference[after] = crossReference[before];
	if ((type == CONJUNCT_NOUN || type == CONJUNCT_VERB))
	{
		// if he has previously filled a noun role, we need to request it again
		if (roles[before] & (SUBJECT2 | MAINSUBJECT | MAINOBJECT | OBJECT2 )) needRoles[roleIndex] |= roles[before] & (SUBJECT2 | MAINSUBJECT | MAINOBJECT | OBJECT2 );
		else if (roles[before] & (VERB2|MAINVERB)) 
		{
			needRoles[roleIndex] |= roles[before];
			needRoles[roleIndex] &= -1 ^ (OBJECT2|MAINOBJECT|OBJECT_COMPLEMENT|SUBJECT_COMPLEMENT); // cant have these intruding after the and or comma
		}
	}

	// form or continue a ring - x AND y  where y loops back to x
	 int hook = coordinates[before]; // loop onto any prior ring 
	if (!hook || hook > before) hook = before; // if already hooked later, dont use that hook. it should be part of earlier loop
	coordinates[after] = (unsigned char)hook;
	coordinates[before] = (unsigned char)i;
	coordinates[i] = (unsigned char)after;
	if (phrases[before]) ExtendChunk(before,after,phrases); // expand dual object in prep phrase
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Conjunct %s(%d) joins %s(%d) with %s(%d) hooked at %d\r\n",wordStarts[i],i,wordStarts[before],before,wordStarts[after],after,hook);

	// if what we conjoin is a chunk, extend the chunk to cover us also
	if (phrases[before] && type == CONJUNCT_NOUN) ExtendChunk(before,after,phrases);  // multiple objects in phrase
	if (verbals[before] && type == CONJUNCT_NOUN && !(posValues[after] & (NOUN_GERUND|NOUN_INFINITIVE))) ExtendChunk(before,after,verbals);   // multiple objects in verbal
	if (clauses[before]) ExtendChunk(before,after,clauses); // can span mulitple verbs or subjects or objects
}

static void ForceNounPhrase(int i, bool &changed) // insure things before are proper words
{
	while (--i  >= startSentence && (!(posValues[i] & CONJUNCTION_COORDINATE) || ignoreWord[i]))
	{
		if (bitCounts[i] != 1 && posValues[i] & (ADJECTIVE_BITS|DETERMINER))
		{
			LimitValues(i,ADJECTIVE_BITS|DETERMINER,(char*)"ForceNounPhrase",changed);
		}
	}
}

static int NearestProbableNoun(int i)
{
	while (++i < endSentence)
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & (PRONOUN_BITS|NOUN_BITS) && (posValues[i] & (PRONOUN_BITS) || bitCounts[i] == 1 || (originalLower[i] && originalLower[i]->systemFlags & NOUN))) break; // probable noun
	}
	return (i <= endSentence) ? i : 0;
}

static int NearestProbableVerb(int i)
{
	while (++i < endSentence)
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & (VERB_BITS|AUX_VERB) && (bitCounts[i] == 1 || (originalLower[i] && originalLower[i]->systemFlags & VERB))) break; // probable verb
	}
	return (i <= endSentence) ? i : 0;
}

static int ConjoinPhrase(int i, bool &changed)
{
	int before = i - 1;
	int after = i + 1;
	if (posValues[before] == COMMA) --before;					
	// do before NOUN because of issues about noun at end of phrase being involved. more likely prep phrases than dual noun, but can test for prep

	if (posValues[after] & PREPOSITION && posValues[before] & (NOUN_BITS|PRONOUN_BITS))  
	{
		int r = before;
		while (--r) // prove phrase matching going on
		{
			if (ignoreWord[i]) continue;
			if (! (posValues[r] & (NOUN_BITS|DETERMINER|PREDETERMINER|ADJECTIVE_BITS|ADVERB))) break;
		}
		if ( posValues[r] & PREPOSITION) 
		{
			SetRole(i, CONJUNCT_PHRASE);
			coordinates[i] = (unsigned char)r;
		}
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"DuplicatePhrase");
		return 0;
	}

	// phrase before and possible preposition after
	if (phrases[before] && posValues[after] & PREPOSITION)  
	{
		int deepBefore = before;
		while (deepBefore && phrases[deepBefore] == phrases[deepBefore-1]) --deepBefore; // find the prep
		if (bitCounts[after] != 1) // can we force preposition on after?
		{
			if (LimitValues(after,PREPOSITION,(char*)"forcing PREPOSITION conflict to clear conjunction after",changed)) return GUESS_RETRY;
		}
		if (posValues[deepBefore] == posValues[after]) // preps now match
		{
			DoCoord(i,deepBefore,after,CONJUNCT_PHRASE);
			return 0;
		}
	}
	// phrase after and possible preposition before
	if (phrases[after] && posValues[after] & PREPOSITION && !phrases[before])  
	{
		int deepBefore = before;
		while (deepBefore && (posValues[deepBefore] & (NOUN_BITS|ADJECTIVE_BITS|DETERMINER|PREDETERMINER) || ignoreWord[deepBefore])) --deepBefore; // find the prep if any
		if (posValues[deepBefore] & PREPOSITION && bitCounts[deepBefore] != 1) // can we force preposition on after?
		{
			if (LimitValues(deepBefore,PREPOSITION,(char*)"forcing PREPOSITION conflict to clear conjunction before",changed)) return GUESS_RETRY;
		}
		if (posValues[deepBefore] == posValues[after]) // preps now match
		{
			DoCoord(i,deepBefore,after,CONJUNCT_PHRASE);
			return 0;
		}
	}
	return -1;
}

static int ConjoinAdjective( int i, bool &changed)
{
	int before = i - 1;
	int after = i + 1;
	if (posValues[before] == COMMA) --before;					

		// after any coord one might have an intervening adverb to ignore like "I like dogs but rarely cats" or "I like dogs and often cats"
	while (posValues[after] == ADVERB && after < endSentence) ++after;

	// simple ADJECTIVE  "green and red beets"  
	int deepAfter = after;
	while (posValues[deepAfter] & ADVERB && (!(posValues[deepAfter] & ADJECTIVE_BITS) || ignoreWord[deepAfter])) ++deepAfter;	// "I like green *and very red ham" can skip adverbs intervening
	if (posValues[before] & ADJECTIVE_BITS && posValues[deepAfter] & ADJECTIVE_BITS) 
	{
		if (bitCounts[before] == 1 && bitCounts[deepAfter] != 1) 
		{
			if (LimitValues(deepAfter,ADJECTIVE_BITS,(char*)"forcing ADJECTIVE conflict to clear conjunction after",changed)) return GUESS_RETRY;
		}
		else if (bitCounts[deepAfter] == 1  && bitCounts[before] != 1) 
		{
			if (LimitValues(before,ADJECTIVE_BITS,(char*)"forcing ADJECTIVE conflict to clear conjunction before",changed)) return GUESS_RETRY;
		}
		if (bitCounts[deepAfter] == 1 &&  bitCounts[before] == 1)
		{
			DoCoord(i,before,deepAfter,CONJUNCT_ADJECTIVE);
			return 0;
		}
	}
	return -1;
}

static int ConjoinNoun( int i, bool &changed)
{
	 int before = i - 1;
	 int after = i + 1;
	if (posValues[before] == COMMA) --before;		

	int deepBefore = i;
	while (--deepBefore && (!(posValues[deepBefore] & NOUN_INFINITIVE) || ignoreWord[deepBefore])){;}

	if (posValues[deepBefore] & NOUN_INFINITIVE && posValues[after] & NOUN_INFINITIVE)
	{
		LimitValues(after, NOUN_INFINITIVE,(char*)"forcing NOUN infinitive after conjoin",changed);
		DoCoord(i,deepBefore,after,CONJUNCT_NOUN);
		return 0;
	}

	// conjoin noun-infinitive?
	if (posValues[before] == NOUN_INFINITIVE)
	{
		int deepAfter = i;
		while (++deepAfter <= endSentence)
		{
			if (posValues[deepAfter] & (NOUN_BITS | VERB_BITS | AUX_VERB) && !ignoreWord[deepAfter]) break;
		}
		if (posValues[deepAfter] & NOUN_INFINITIVE)
		{
			DoCoord(i,before,deepAfter,CONJUNCT_NOUN);
			LimitValues(deepAfter, NOUN_INFINITIVE,(char*)"forcing NOUN infinitive after conjoin",changed);
			return 0;
		}
	}
	
	int deepAfterVerb = after;
	if (needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT)){;} // wont be sentence conjoin
	else if (!(needRoles[roleIndex] & (OBJECT2|MAINOBJECT)))
	{
		while (++deepAfterVerb < endSentence && (!(posValues[deepAfterVerb] & (COMMA|CONJUNCTION_COORDINATE)) || ignoreWord[deepAfterVerb]) )
		{
			if (posValues[deepAfterVerb] & VERB_BITS) break;	 // possible verb
		}
	}

	// simple NOUN after "ate rocks and greens" 
	// But be careful about noun when might be prep phrase instead being paired

	if (posValues[before] & (PRONOUN_BITS|NOUN_BITS) && posValues[after] &  (PRONOUN_BITS|NOUN_BITS) && !(posValues[deepAfterVerb] & VERB_BITS)) 
	{
		if (bitCounts[after] == 1  && bitCounts[before] != 1) 
		{
			if (LimitValues(before, (PRONOUN_BITS|NOUN_BITS),(char*)"forcing NOUN conflict to clear conjunction before",changed)) return GUESS_RETRY;
		}
		if (bitCounts[after] != 1  && bitCounts[before] == 1) 
		{
			if (LimitValues(after, (PRONOUN_BITS|NOUN_BITS),(char*)"forcing NOUN conflict to clear conjunction after",changed)) return GUESS_RETRY;
		}
		if (bitCounts[after] == 1 &&  bitCounts[before] == 1) // before and after are known
		{
			DoCoord(i,before,after,CONJUNCT_NOUN);
			ForceNounPhrase(after,changed);
			return 0;
		}
	}

	// known noun before and described noun after (simple noun would already be done by now)
	if (posValues[before] &  (PRONOUN_BITS|NOUN_BITS) && bitCounts[before] == 1 && posValues[after] & (DETERMINER|ADVERB|ADJECTIVE_BITS|POSSESSIVE_BITS)  && bitCounts[after] == 1) 
	{
		int deepAfter = after;
		while (posValues[deepAfter] & (DETERMINER|ADVERB|ADJECTIVE_BITS|POSSESSIVE_BITS) || ignoreWord[deepAfter]) ++deepAfter;
		if (deepAfter <= (int)endSentence && posValues[deepAfter] &  (PRONOUN_BITS|NOUN_BITS))
		{
			if (bitCounts[deepAfter] != 1) 
			{
				if (LimitValues(deepAfter, (PRONOUN_BITS|NOUN_BITS),(char*)"forcing NOUN conflict to clear conjunction after",changed)) return GUESS_RETRY;
			}
			if (bitCounts[deepAfter] == 1 &&  bitCounts[before] == 1)
			{
				DoCoord(i,before,deepAfter,CONJUNCT_NOUN);
				ForceNounPhrase(deepAfter,changed);
				return 0;
			}
		}
	}

	int deepAfter = after;
	while (posValues[deepAfter] & (DETERMINER|ADVERB|ADJECTIVE_BITS|POSSESSIVE_BITS) || ignoreWord[deepAfter]) 
	{
		if (posValues[deepAfter] & NORMAL_NOUN_BITS && !ignoreWord[deepAfter]) break;
		++deepAfter;
	}

	// known noun before (mainsubject) and verb still pending, be NOUN coord on mainsubject 
	if (roleIndex == MAINLEVEL && needRoles[roleIndex] & MAINVERB && posValues[i-1] & NORMAL_NOUN_BITS && roles[i-1] & MAINSUBJECT) // be main subject conjunct
	{
		if (bitCounts[deepAfter] != 1 && LimitValues(deepAfter, NOUN_BITS,(char*)"forcing NOUN to match conjunction before",changed)) return GUESS_RETRY;
		DoCoord(i,before,deepAfter,CONJUNCT_NOUN);
		ForceNounPhrase(deepAfter,changed);
		return 0;
	}

	// known noun after w/o following verb or adverb, hunt for before
	if (posValues[after] & NOUN_BITS && bitCounts[after] == 1 && !(posValues[after+1] & (ADVERB|AUX_VERB|VERB_BITS)))
	{
		int deepBefore = i;
		while (--deepBefore) 
		{
			if (posValues[deepBefore] & NOUN_BITS && bitCounts[deepBefore] == 1 && !ignoreWord[deepBefore])
			{
				DoCoord(i,deepBefore,after,CONJUNCT_NOUN);
				ForceNounPhrase(after,changed);
				return 0;
			}
		}
	}
	
	if (!(needRoles[MAINLEVEL] & (MAINSUBJECT|MAINVERB))) 
	{
		// if prior was a direct object and we have only a few words left, replicate it
		int noun = NearestProbableNoun(i);
		int verb = NearestProbableVerb(i);
		if (roles[before] & MAINOBJECT && (int)noun > i && !verb ) // we have some noun we could find, and no verb to make into a clause maybe
		{
			needRoles[MAINLEVEL] |= MAINOBJECT;
			roles[i] = CONJUNCT_NOUN;
			return 0;
		}
	}

	// dual object? (char*)"what a big nose he has, *and big eyes too"
	if (objectRef[currentMainVerb] && posValues[deepAfter] & NOUN_BITS)
	{
		if (bitCounts[deepAfter] != 1 && LimitValues(deepAfter, NOUN_BITS,(char*)"forcing NOUN1 to match conjunction before",changed)) return GUESS_RETRY;
		DoCoord(i,objectRef[currentMainVerb],deepAfter,CONJUNCT_NOUN);
		ForceNounPhrase(deepAfter,changed);
		return 0;
	}

	return -1;
}

static int ConjoinVerb( int i, bool &changed)
{
	int before = i - 1;
	int after = i + 1;
	if (posValues[before] == COMMA) --before;					
	int firstAfter = i;
	while (++firstAfter <= endSentence && (!(posValues[firstAfter] & (NOUN_INFINITIVE|VERB_BITS)) || ignoreWord[firstAfter])){;}		// forward to a potential verbal 

	int deepBefore = i;
	while (--deepBefore && (!(posValues[deepBefore] & (VERB_BITS|NOUN_INFINITIVE)) || ignoreWord[deepBefore])) {;}

	// closest verbal after precludes a subject for a sentence match
	if (posValues[deepBefore] & NOUN_INFINITIVE && posValues[firstAfter] & (NOUN_INFINITIVE|VERB_INFINITIVE) && firstAfter == (i + 1)) // "she taught him how to lace his boots, make his bed, and eat food."
	{
		if (LimitValues(firstAfter,NOUN_INFINITIVE,(char*)"Forcing noun infinitive as closest match",changed)) return GUESS_RETRY; 
		return -1;
	}

	// simple verb after matches verb before (adverb already sorted out earlier) "he danced and pranced"
	if (posValues[after] & VERB_BITS) 
	{
		if (deepBefore) // tense must match up as possible "he pulled carrots and *dip" cannot match up verbs
		{
			uint64 tense = posValues[deepBefore] & VERB_BITS;
			if (posValues[after] & tense) // maybe can match up verb tenses
			{
				if (LimitValues(after,tense,(char*)"Forcing tense match for AND verb",changed)) return GUESS_RETRY;
				if (posValues[deepBefore] == posValues[after]) // tenses now match
				{
					DoCoord(i,deepBefore,after,CONJUNCT_VERB);
					return 0;
				}
			}
			else if (posValues[after] & AUX_VERB) // "she is coming *and can scamper"
			{
				if (LimitValues(after,AUX_VERB,(char*)"Forcing tense aux for AND verb",changed)) return GUESS_RETRY;
				DoCoord(i,deepBefore,after,CONJUNCT_VERB);
				return 0;
			}
			// cannot match up verb tenses
			else
			{
				if (bitCounts[after] > 1 && bitCounts[deepBefore] == 1) 
				{
					if (LimitValues(after,-1 ^ VERB_BITS,(char*)"tenses dont line up for AND, so remove verb potential",changed)) return GUESS_RETRY;
				}
			}
		}
	}
	
	if (posValues[before] & PARTICLE)  // he stood up and quickly ran  down the street
	{
		 int deepBefore = before;
		while (--deepBefore > 2) if ( posValues[deepBefore] & (VERB_BITS|NOUN_INFINITIVE)) break;
		 int deepAfter = after -1;
		while (++deepAfter <= endSentence)
		{
			if ( posValues[deepAfter] & (VERB_BITS|NOUN_INFINITIVE|NORMAL_NOUN_BITS|DETERMINER) && !ignoreWord[deepAfter]) break;
		}
		uint64 val = posValues[deepAfter] & (VERB_BITS|NOUN_INFINITIVE);
		if (val & posValues[deepBefore] ) // verbs both there (same tense?)
		{
			DoCoord(i,deepBefore,deepAfter,CONJUNCT_VERB);
			return 0;
		}
	}
	
	int deepAfter = i;
	while (++deepAfter <= endSentence && (!(posValues[deepAfter] & VERB_BITS) || ignoreWord[deepAfter])) // forward to a potential verb 
	{	
		if (posValues[deepAfter] & (NOUN_BITS|PRONOUN_BITS)) return -1;	// cannot have intervening noun. might be sentence
	}

	if (posValues[deepAfter] & VERB_BITS && verbStack[MAINLEVEL] && posValues[deepAfter] & NOUN_BITS) // verb/noun conflict... "what a big nose you have, and big eyes too"
	{
		if (posValues[deepAfter-1] & ADVERB) return -1; // wont have an adverb BEFORE the verb when conjoined
	}

	if (posValues[deepAfter] & VERB_BITS && verbStack[MAINLEVEL]) // we and to a verb... expect this to be WORD
	{
		if (LimitValues(deepAfter,VERB_BITS,(char*)"conjoined after is VERB",changed)) return GUESS_RETRY;
		SetRole(i,CONJUNCT_VERB);
		int verb = i;
		while (--verb && (!(posValues[verb] & VERB_BITS) || ignoreWord[verb])); // find verb before
		SetRole(deepAfter,roles[verb]);	// copy verb role from before
		coordinates[i] = (unsigned char)verb;
		if (roles[deepAfter] & MAINVERB && canSysFlags[i] & (VERB_DIRECTOBJECT|VERB_INDIRECTOBJECT)) // change to wanting object(s)
		{
			needRoles[MAINLEVEL] |= MAINOBJECT;
			if ( canonicalLower[i]->properties & VERB_INDIRECTOBJECT) needRoles[MAINLEVEL] |= MAININDIRECTOBJECT;
			if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"DuplicateVerb");
		}
	}

	return -1;
}

static int ConjoinImmediate( int i, bool & changed)
{
	 int before = i - 1;
	 int after = i + 1;
	if (posValues[before] == COMMA) --before;			
	
	// conjoin adjective?
	if (posValues[before] & (ADJECTIVE_NORMAL|ADJECTIVE_PARTICIPLE))
	{
		if (posValues[after] & (ADJECTIVE_NORMAL|ADJECTIVE_PARTICIPLE)) // "how soft and *white its fur is"
		{
			LimitValues(after, (ADJECTIVE_NORMAL|ADJECTIVE_PARTICIPLE),(char*)"forcing simple adjective or participle adjective after conjoin",changed);
			LimitValues(before, (ADJECTIVE_NORMAL|ADJECTIVE_PARTICIPLE),(char*)"forcing simple adjective or participle adjective before conjoin",changed);
			DoCoord(i,before,after,CONJUNCT_ADJECTIVE);
			return 0;
		}
	}
	// conjoin adverb?
	if (posValues[before] & ADVERB)
	{
		if (posValues[after] & ADVERB)
		{
			LimitValues(after, ADVERB,(char*)"forcing simple adverb after conjoin",changed);
			DoCoord(i,before,after,CONJUNCT_ADVERB);
			return 0;
		}
	}
	// conjoin infinitive or gerund
	if (posValues[before] & (NOUN_INFINITIVE|NOUN_GERUND))
	{
		if (posValues[after] & (NOUN_INFINITIVE|NOUN_GERUND))
		{
			LimitValues(after, (NOUN_INFINITIVE|NOUN_GERUND),(char*)"forcing noun infinitive/gerund conjoin",changed);
			DoCoord(i,before,after,CONJUNCT_NOUN);
			return 0;
		}
	}

	// conjoin noun?
	if (posValues[before] & ((NOUN_BITS  |PRONOUN_BITS) ^ NOUN_INFINITIVE))
	{
		int deepAfter = after;
		if (posValues[deepAfter] & SIGNS_OF_NOUN_BITS) // we know a noun follows -- dont require assurance - "I want to correlate ufos and fish"
		{
			--deepAfter;
			while (++deepAfter <= endSentence)
			{
				if (posValues[deepAfter] & (NOUN_BITS|PRONOUN_BITS) && !ignoreWord[deepAfter]) break; // but may be wrong one
			}
		}
		else if (posValues[deepAfter] & NORMAL_NOUN_BITS && needRoles[roleIndex] & OBJECT2)
		{
			LimitValues(deepAfter,NORMAL_NOUN_BITS,(char*)"noun before or, need nothing, can be noun after, be simple",changed);
		}
		else deepAfter = 0;
		if (deepAfter && posValues[deepAfter] & (NOUN_BITS|PRONOUN_BITS)) // "I like apples and *Bob"
		{
			// cannot conjoin if followed by verb
			 int thereafter = deepAfter;
			while (++thereafter <= endSentence)
			{
				if (ignoreWord[thereafter]) continue;
				if (posValues[thereafter] & (CONJUNCTION|COMMA)) break;
				if (posValues[thereafter] & (AUX_VERB|VERB_BITS)) break;
			}
			if (posValues[thereafter] & (AUX_VERB|VERB_BITS)) return -1; // cannot safely conjoin, might be conjoining sentences "I like apples and *Bob likes gold"
			else
			{
				LimitValues(deepAfter, NOUN_BITS|PRONOUN_BITS,(char*)"forcing simple noun/pronoun conjoin",changed);
				DoCoord(i,before,deepAfter,CONJUNCT_NOUN);
				return 0;
			}
		}
	}

	// conjoin verb?
	if (posValues[before] & VERB_BITS)
	{
		if (posValues[after] & VERB_BITS && !(posValues[after] & NORMAL_NOUN_BITS))
		{
			uint64 tenses = posValues[before] & VERB_BITS;
			if (posValues[after] & tenses)
			{
				LimitValues(after, tenses,(char*)"forcing simple verb matching tense conjoin",changed);
			}
			else LimitValues(after, VERB_BITS,(char*)"forcing simple verb conjoin",changed);
			DoCoord(i,before,after,CONJUNCT_VERB);
			return 0;
		}
	}

	return -1;
}

static bool FlipPrepToConjunction( int i,bool question, bool changed) // at the verb we detect
{
	 int prep = GetPhraseHead(i-1);
	if (!(posValues[prep] & PREPOSITION)) return false;	// phrase at i-1 is BAD!!!
	if (allOriginalWordBits[prep] & CONJUNCTION_SUBORDINATE && !clauses[prep]) // BUG- but if prep ended clause, then we wouldn't know...
	{
		LimitValues(prep,CONJUNCTION_SUBORDINATE ,(char*)"unexpected verb after phrase, convert prep to subord clause",changed);
		if (question) return true;
		int object = objectRef[prep];
		ErasePhrase(prep);
		if (posValues[object] & PRONOUN_OBJECT) LimitValues(object,PRONOUN_SUBJECT,(char*)"switching to pronoun subject to use as subject",changed);
		return true;
	}
	else if (allOriginalWordBits[prep] & CONJUNCTION_COORDINATE && !clauses[prep])
	{
		LimitValues(prep,CONJUNCTION_COORDINATE ,(char*)"unexpected verb after phrase, convert prep to coord clause",changed);
		if (question) return true;
		unsigned int object = objectRef[prep];
		ErasePhrase(prep);
		if (posValues[object] & PRONOUN_OBJECT) LimitValues(object,PRONOUN_SUBJECT,(char*)"switching to pronoun subject to use as subject",changed);
		return true;
	}
	return false;
}

static  int ConjoinAdverb( int i, bool &changed)
{
	 int before = i - 1;
	 int after = i + 1;
	if (posValues[before] == COMMA) --before;					

	// simple ADVERB before  "moved quickly and quietly"  
	if (posValues[before] & ADVERB && posValues[after] & ADVERB) 
	{
		bool badmix = false;
		// - but not if a verb is before and after like "they worked hard and then walked"
		if (posValues[before-1] & (VERB_INFINITIVE|VERB_PAST|VERB_PRESENT|VERB_PRESENT_3PS) && posValues[after+1] & (VERB_INFINITIVE|VERB_PAST|VERB_PRESENT|VERB_PRESENT_3PS) && 
			bitCounts[after+1] == 1) badmix = true; // know verb after
		 // dont mix time and OTHER adverbs with and
		else if ((canSysFlags[before] & TIMEWORD) != (canSysFlags[after] & TIMEWORD)) badmix = true;
		else if (bitCounts[before] == 1 && bitCounts[after] != 1) 
		{
			if (LimitValues(after,ADVERB,(char*)"forcing ADVERB conflict to clear conjunction after",changed)) return GUESS_RETRY;
		}
		else if (bitCounts[after] == 1  && bitCounts[before] != 1) 
		{
			if (LimitValues(before,ADVERB,(char*)"forcing ADVERB conflict to clear conjunction before",changed)) return GUESS_RETRY;
		}
		if (!badmix && bitCounts[after] == 1 &&  bitCounts[before] == 1)
		{
			DoCoord(i,before,after,CONJUNCT_ADVERB);
			return 0;
		}
	}
	return -1;
}

static unsigned int HandleCoordConjunct( int i,bool &changed) // determine kind of conjunction- 
{// Word (adjective/adjectve, adverb/adverb, noun/noun, verb/verb) 
 // Phrase/Clause/Sentence
	//  x coord y will be linked in a ring structure.

	if (!stricmp(wordStarts[i],(char*)"so") || !stricmp(wordStarts[i],(char*)"for")) // these only do sentences
	{
		SetRole(i,CONJUNCT_SENTENCE); 
		return 0;
	}

	unsigned int comma = lastConjunction;
	if (comma) // bind the way we did before
	{
		uint64 match = 0;
		match = posValues[coordinates[comma]];	// what is being bound
		if (match & NOUN_INFINITIVE) match = NOUN_INFINITIVE | VERB_INFINITIVE;
		else if (match & (NOUN_BITS | PRONOUN_BITS)) match = NOUN_BITS | PRONOUN_SUBJECT | PRONOUN_OBJECT;
		else if (match & VERB_BITS) match = VERB_BITS;
		else if (match & ADJECTIVE_BITS) match = ADJECTIVE_BITS;
		else if (match & ADVERB) match = ADVERB;
		else match = 0;
		int deepAfter = i;
		while (++deepAfter <= endSentence)
		{
			if (ignoreWord[deepAfter]) continue;
			if (posValues[deepAfter] & match) // closest matching type
			{
				if (posValues[coordinates[comma]] & NOUN_INFINITIVE) match = NOUN_INFINITIVE; // convert verb infintiive to noun
				LimitValues(deepAfter,match,(char*)"Forcing comma match of pos",changed);
				break;
			}
			// if we want to match a verb but find a noun or pronoun that could be subject, will have to be a coord clause instead "let us run and jump *and that will keep us warm"
			if (match & VERB_BITS && posValues[deepAfter] & (PRONOUN_BITS|NOUN_BITS))
			{
				deepAfter = endSentence;
			}
		}
		if (deepAfter <= endSentence)	
		{
			DoCoord(i,coordinates[comma],deepAfter,roles[comma]);
			return 0;
		}
	}
	
	// find the first match from scratch to determine the type

	int result = ConjoinImmediate(i,changed); // try for most LOCAL matching
	if (result >= 0) return result;

	result = ConjoinAdverb(i,changed);
	if (result >= 0) return result;
	
	result = ConjoinAdjective(i,changed);
	if (result >= 0) return result;

	// test for verb match first before noun, since maybe we can confirm nouns on tense...
	result = ConjoinVerb(i,changed);
	if (result >= 0) return result;

	result = ConjoinPhrase(i,changed); 
	if (result >= 0) return result;
	
	result = ConjoinNoun(i,changed);
	if (result >= 0) return result;

	// BUG not handling conjunction of clause
	
	unsigned int before = i - 1;
	if (posValues[before] == COMMA) --before;					

	if (roleIndex > 1 && subjectStack[MAINLEVEL] && verbStack[MAINLEVEL] && subjectStack[2])
	{
		DoCoord(i,subjectStack[MAINLEVEL],subjectStack[2],CONJUNCT_SENTENCE);
		return 0;
	}
	else if (!(needRoles[MAINLEVEL] & (MAINSUBJECT|MAINVERB))) // can it be sentence we need
	{
		// if prior was a direct object and we have only a few words left, replicate it
		unsigned int noun = NearestProbableNoun(i);
		unsigned int verb = NearestProbableVerb(i);
		if (roles[before] & MAINOBJECT && (int)noun > i && !verb ) {;} // we have some noun we could find, and no verb to make into a clause maybe
		else if (noun && verb > noun) // we can find a potential noun and verb in succession, so assume it is a sentence
		{
			SetRole(i,CONJUNCT_SENTENCE); 
		}
	}
	// we dont know what it binds to yet--- might be mainsubject, might be to a current verbal
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"bind Conjunct=%s(%d)\r\n",wordStarts[i],i);
	return 0;
}

static void DropLevel()
{
	if (trace &  TRACE_POS) Log(STDTRACELOG,(char*)"    Dropped Level %d\r\n",roleIndex);
	if (roleIndex) --roleIndex;
}

static void CloseLevel(int  i)
{
	if (needRoles[roleIndex] & QUOTATION_UTTERANCE) return;
	while (roleIndex > 1 ) 
	{
		int at = i;
		if (needRoles[roleIndex] & PHRASE) // closes out a phrase, drag phrase over it
		{
			if (needRoles[roleIndex] & OBJECT2) break;	 // need vital stuff
			if (posValues[at] & COMMA) --at; // stop before here
			if (!phrases[at])
			{
				ExtendChunk(lastPhrase, at, phrases); 
				if (at > 1 && clauses[at-1]) ExtendChunk(lastPhrase-1, at, clauses);
			}
			lastPhrase = 0;
		}
		else if (needRoles[roleIndex] & CLAUSE) // closes out a clause, drag clause over it
		{
			if (roles[lastClause] == OMITTED_SUBJECT_VERB){;} // allow "if dead, awaken"
			else  if (needRoles[roleIndex] & (SUBJECT2|VERB2)) break;	// need vital stuff
			else if (needRoles[roleIndex] != CLAUSE && posValues[at] != COMMA) break;	// we think we need something and its still going
			if (posValues[at] & COMMA) --at; // stop before here
			if (!clauses[at]) ExtendChunk(lastClause,at,clauses);
			lastClause = 0;
		}
		else if (needRoles[roleIndex] & VERBAL) // closes out a verbal, drag verbal over it
		{
			if (needRoles[roleIndex] & VERB2) break;	// need vital stuff
			if (needRoles[roleIndex] != VERBAL && posValues[at] != COMMA) break;	// we think we need something and its still going
			if (posValues[at] & COMMA) --at; // stop before here
			if (!verbals[at]) ExtendChunk(lastVerbal,at,verbals);
			lastVerbal = 0;
		}

		DropLevel(); 	// level complete (not looking for anything anymore)
	}
}

static void DecodeneedRoles(unsigned int x,char* buffer)
{
	*buffer = 0;
	if (needRoles[x] & MAINSUBJECT) strcat(buffer,(char*)" MainSubject");
	if (needRoles[x] & MAINVERB) strcat(buffer,(char*)" MainVerb");
	if (needRoles[x] & MAININDIRECTOBJECT) strcat(buffer,(char*)" MainIO");
	if (needRoles[x] & MAINOBJECT) strcat(buffer,(char*)" Mainobj");

	if (needRoles[x] & SUBJECT2) strcat(buffer,(char*)" Subject2");
	if (needRoles[x] & VERB2) strcat(buffer,(char*)" Verb2");
	if (needRoles[x] & INDIRECTOBJECT2) strcat(buffer,(char*)" Io2");
	if (needRoles[x] & OBJECT2) strcat(buffer,(char*)" Obj2");

	if (needRoles[x] & PHRASE) strcat(buffer,(char*)" Phrase:");
	if (needRoles[x] & CLAUSE) strcat(buffer,(char*)" Clause:");
	if (needRoles[x] & VERBAL) strcat(buffer,(char*)" Verbal:");
	if (needRoles[x] & QUOTATION_UTTERANCE) strcat(buffer,(char*)" Quote:");
	if (!(needRoles[x] & (PHRASE|CLAUSE|VERBAL|QUOTATION_UTTERANCE))) strcat(buffer,(char*)" Main:");
	
	if (needRoles[x] & SUBJECT_COMPLEMENT) strcat(buffer,(char*)" SubCompl"); // adjective as object
	if (needRoles[x] & OBJECT_COMPLEMENT) strcat(buffer,(char*)" ObjCompl"); // usually adjective after object "paint his car *purple"
	if (needRoles[x] & TO_INFINITIVE_OBJECT) strcat(buffer,(char*)" verb-toinfinitive-object");
	if (needRoles[x] & VERB_INFINITIVE_OBJECT) strcat(buffer,(char*)" verb-infinitive-object");
	if (needRoles[x] & OMITTED_SUBJECT_VERB) strcat(buffer,(char*)" omitted-subject-verb");
}

WORDP GetPhrasalVerb( int i)
{
	char word[MAX_WORD_SIZE];
	strcpy(word,GetInfinitive(wordStarts[i],true));
	int at = phrasalVerb[i];
	while (at != i)
	{
		strcat(word,(char*)"_");
		strcat(word,wordStarts[at]);
		at = phrasalVerb[at];
	}
	return FindWord(word);
}

static void SeekObjects( int i) // this is a verb, what objects does it want
{
	if (allOriginalWordBits[i] & VERB_PAST_PARTICIPLE)
	{
		if (posValues[i] & ADJECTIVE_PARTICIPLE) return;	// cannot have objects on postnominal past adjective participle
		// passive voice takes no objects.
		if (posValues[i] & VERB_PAST_PARTICIPLE) // I have gone home
		{
			int at = i;
			while (--at >= startSentence)
			{
				if (ignoreWord[at]) continue;
				if (posValues[at] & AUX_BE && stricmp(wordStarts[at],(char*)"been")) return;			// the ball was hit
				else if (posValues[at] & AUX_HAVE) break;	// I have gone home
			}
		}
	}

	if (!verbStack[roleIndex]) verbStack[roleIndex] = (unsigned char)i;	// this is the verb for this level
	
	if (roleIndex == MAINLEVEL && objectStack[roleIndex] && !coordinates[i]) // main verb has an objects already (qword like in "what do you *like to do") - but not if we have a coordinating word verb needing own object
	{
		objectRef[currentMainVerb] = objectStack[roleIndex];
		if (objectStack[roleIndex] && *wordStarts[objectStack[roleIndex]] == '"') // quoted object
		{
			crossReference[objectStack[roleIndex]] = (unsigned char) i;
			needRoles[roleIndex] &= -1 ^ (MAINOBJECT|MAININDIRECTOBJECT|OBJECT2|INDIRECTOBJECT2);
			if (!subjectStack[roleIndex] && !(needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT))) needRoles[roleIndex] |= (roleIndex == MAINLEVEL) ? MAINSUBJECT : SUBJECT2;
		}

		if (objectStack[roleIndex] < currentMainVerb) return; // object was presupplied
	}

	if (canonicalLower[i] == DunknownWord)  // unknown verb -- presume takes object and indirect object and maybe object complement
	{ // presumed NOT a linking verb, so cannot take SUBJECT_COMPLEMENT.
		if (roleIndex == MAINLEVEL) needRoles[MAINLEVEL] |= MAININDIRECTOBJECT | MAINOBJECT | OBJECT_COMPLEMENT;
		else needRoles[roleIndex] |= OBJECT2 | INDIRECTOBJECT2 | OBJECT_COMPLEMENT;
		return;
	}

	if (canSysFlags[i] & VERB_DIRECTOBJECT) // change to wanting object(s)
	{
		// if a question, maybe object already filled in "what dog do you like"
		if (roleIndex == MAINLEVEL && ( !objectRef[currentMainVerb] || coordinates[i])) 
		{
			needRoles[roleIndex] |= MAINOBJECT;
			if (parseFlags[i] & (FACTITIVE_ADJECTIVE_VERB|FACTITIVE_NOUN_VERB))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; // "we made him *happy"  "we elected him *president"
			else if (canSysFlags[i] & (VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE|VERB_TAKES_INDIRECT_THEN_TOINFINITIVE))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; // "we want him to go"
		}
		else if (roleIndex > MAINLEVEL && (!objectRef[currentVerb2] || coordinates[i])) 
		{
			needRoles[roleIndex] |= OBJECT2;
			if (parseFlags[i] & (FACTITIVE_ADJECTIVE_VERB|FACTITIVE_NOUN_VERB))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; // "we made him *happy"  "we elected him *president"
			else if (canSysFlags[i] & (VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE|VERB_TAKES_INDIRECT_THEN_TOINFINITIVE))  needRoles[roleIndex] |= OBJECT_COMPLEMENT; // "we want him to go"
		}
		// are you able to drive a car wants object2 for car
	}

	// various infinitives in object positions
	if (canSysFlags[i] & VERB_TAKES_INDIRECT_THEN_TOINFINITIVE)  // "I allowed John *to run"
	{
		if ( !(posValues[NextPos(i)] & (DETERMINER | PREDETERMINER | ADJECTIVE | ADVERB | PRONOUN_OBJECT  | PRONOUN_POSSESSIVE | NOUN_BITS))); // must come immediately
		else needRoles[roleIndex] |= TO_INFINITIVE_OBJECT | ((roleIndex == MAINLEVEL) ? MAININDIRECTOBJECT : INDIRECTOBJECT2);
	}
	if (canSysFlags[i] & VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE) // "he made John *run"
	{
		if ( !(posValues[NextPos(i)] & (DETERMINER | PREDETERMINER | ADJECTIVE | ADVERB | PRONOUN_OBJECT  | PRONOUN_POSSESSIVE | NOUN_BITS))); // must come immediately
		else needRoles[roleIndex] |= VERB_INFINITIVE_OBJECT | ((roleIndex == MAINLEVEL) ? MAININDIRECTOBJECT : INDIRECTOBJECT2);
	}
	if (canSysFlags[i] & VERB_TAKES_TOINFINITIVE) // proto 28 "Somebody ----s to INFINITIVE"   "we agreed to plan"
	{
		needRoles[roleIndex] |= TO_INFINITIVE_OBJECT;
	}
	if (canSysFlags[i] & VERB_TAKES_VERBINFINITIVE) // proto 32, 35 "Somebody ----s INFINITIVE"   "Something ----s INFINITIVE"
	{
		needRoles[roleIndex] |= VERB_INFINITIVE_OBJECT; 
	}

	if (canSysFlags[i] & VERB_INDIRECTOBJECT) 
	{
		if ( !(posValues[NextPos(i)] & (DETERMINER | PREDETERMINER | ADJECTIVE | ADVERB | PRONOUN_OBJECT  | PRONOUN_POSSESSIVE | NOUN_BITS))); // must come immediately
		else needRoles[roleIndex] |=  (roleIndex == MAINLEVEL) ? MAININDIRECTOBJECT : INDIRECTOBJECT2;
	}
	
	if (canSysFlags[i] & VERB_TAKES_ADJECTIVE)  
	{
		needRoles[roleIndex] |= SUBJECT_COMPLEMENT; // linking verbs expect adjective  "he is pretty"
		needRoles[roleIndex] |= (roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2; // but may take objects instead "he seems a fool"
	}

	// verb wants noun complement - we will eventually have to decide on object complement vs indirect object, but when we could be both, we wait
	if (parseFlags[i] & FACTITIVE_NOUN_VERB)  needRoles[roleIndex] |= OBJECT_COMPLEMENT;
}

static bool IsLegalAddress(unsigned int first)
{
	unsigned int start = startSentence;
	if (*wordStarts[start] == '"') ++start; // ignore quote
	unsigned int end = endSentence;
	if (*wordStarts[end] == '"') --end; // ignore quote
	if (posValues[end] & PUNCTUATION) --end; // ignore terminator
	// simple address
	if (posValues[first] & NOUN_PROPER_SINGULAR && posValues[first-1] == COMMA && first == end) return true; // tail "I hate you, Barbara"
	if (posValues[first] & NOUN_PROPER_SINGULAR && posValues[first+1] == COMMA && first == start && posValues[first+2] & PRONOUN_SUBJECT) return true; // tail "Barbara, I hate you"

	return false;
}

static bool PossibleNounAfter(int i)
{
	while (++i <= endSentence)
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & (NOUN_BITS|PRONOUN_BITS)) return true;
	}
	return false; // wrong when ambigious pronoun at end wraps to front   BUG
}

static bool IsLegalAppositive(int first, int second)
{
	if (roles[second-1] & OBJECT_COMPLEMENT) // I have a friend named Harry *who likes to fish
		return false;
	if (needRoles[roleIndex] & (MAINSUBJECT|MAINOBJECT|SUBJECT2|OBJECT2)) return false;	// we dont need it to be appositive
	if (!(posValues[first] & (NOUN_SINGULAR|NOUN_PLURAL|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_NUMBER)) || bitCounts[first] != 1) return false;
	if (!(posValues[second] & (NOUN_SINGULAR|NOUN_PLURAL|NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_NUMBER)) || bitCounts[second] != 1) return false;

	// subject object inversion in clause:  "do you know what game *Harry likes?
	if (needRoles[roleIndex] & VERB2 && lastClause && (!stricmp(wordStarts[lastClause],(char*)"what") ||  !stricmp(wordStarts[lastClause],(char*)"which")) && roles[first] == SUBJECT2)
	{
		SetRole(first,OBJECT2,true);
		needRoles[roleIndex] |= SUBJECT2;
		return false;
	}

	// first is proper, 2nd is not and 1st is not human and is adjacent
	if (posValues[first] & NOUN_PROPER_SINGULAR && posValues[second] & (NOUN_SINGULAR|NOUN_PLURAL) && !(allOriginalWordBits[first] & NOUN_HUMAN) && (second-first) < 2) return false;

	// nouns must be determined or proper - "my cat, dog" is not legal but "my cat, a dog" is -- or a title like "Julie Winter president of the club" or "CEO Tom"
	int det1, det2;
	if (!IsDeterminedNoun(first,det1)) return false;
	if (!IsDeterminedNoun(second,det2)) return false;

	// cannot have both using the "the cat, the dog" is illegal. But "A cat, a fine species" is legal
	if (!stricmp(wordStarts[det1],(char*)"the") && !stricmp(wordStarts[det2],(char*)"the")) return false;

	return true;
}

static bool ProcessOmittedClause(unsigned int verb,bool &changed) // They were certain (they were happy)
{
	if (clauses[verb]) return false;	// already done
	unsigned int subject = verb+1;
	while (--subject)
	{
		if (ignoreWord[subject]) continue;
		if (posValues[subject] & (NOUN_BITS | PRONOUN_BITS)) break;
		if (posValues[subject] & (AUX_VERB|ADVERB)) continue;
		return false;
	}
	if (!subject) return false;

	AddClause(subject,(char*)"omitted clause");
	SeekObjects(verb);
	SetRole(verb,VERB2,true);
	SetRole(subject,SUBJECT2,true);
	if (posValues[subject] == PRONOUN_OBJECT) 
	{
		LimitValues(subject,PRONOUN_SUBJECT,(char*)"omitted clause subject goes from object to subject pronoun",changed);
	}
	ExtendChunk(subject,verb,clauses);
	if (needRoles[roleIndex] == CLAUSE) DropLevel();  // now down maybe...
	changed = true;
	return true;
}

static void HandleComplement( int i,bool & changed)
{
	if (roles[i]) return;  // already labelled. (eg forward conjunction coordinate processing)

	// this cannot be an indirect object
	if (posValues[i] & PRONOUN_OBJECT && !(lcSysFlags[i] & PRONOUN_INDIRECTOBJECT)) needRoles[roleIndex] &= -1 ^ (MAININDIRECTOBJECT|INDIRECTOBJECT2);
	if (posValues[i] & (NOUN_GERUND|NOUN_INFINITIVE|VERB_BITS)) needRoles[roleIndex] &= -1 ^ (MAININDIRECTOBJECT|INDIRECTOBJECT2);	// cannot be indirect object with these...

	// object complements are handled elsewhere
	int subject = needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT);
	int indirectObject = needRoles[roleIndex] & (MAININDIRECTOBJECT|INDIRECTOBJECT2);
	if (posValues[i+1] & ADJECTIVE_PARTICIPLE) indirectObject = 0;		// "I have a *friend named Harry" not likely to see "I threw *Harry walking dogs"
	int directObject = needRoles[roleIndex] & (MAINOBJECT|OBJECT2);

	int priorNoun = FindPriorNoun(i);
	int currentVerb = verbStack[roleIndex]; 

// beware: Dogs, my favorite animals, are swift.   SUBJECT/APPOSITIVE/PREDICATE
// vs Bill, I hate you.	ADDRESS/SENTENCE 
// Bill, dogs, those fast creatures, can outrun you. ADDRESS/SUBJECT/APPOSITIVE/PREDICATE
//  Fleeing home, they left.
// walking home, my favorite activity, happens often. - gerund, appositive, predicate  OR  adjectivepariticle, subject, predicate (cannot be)
//  subject, predicate is not a possible format...  subject, description, predicate is possible
	// predicate object is not a possible format either.

	// having no expected use for gerund, is it a comma'ed off appositive
	if (posValues[i] & NOUN_GERUND && !indirectObject && !directObject && !subject && !(zoneData[zoneIndex] & ZONE_FULLVERB)) // is this an appositivie gerund "his goal, walking briskly, was in sight"
	{
		int at = i;
		while (--at)
		{
			if (ignoreWord[at]) continue;
			if (posValues[at] & COMMA) break;
			if (posValues[at] & NOUN_BITS) break;
		}
		if (at && posValues[at] & COMMA && posValues[at-1] & NORMAL_NOUN_BITS) // we are in zone after ending noun.
		{
			SetRole(i,APPOSITIVE);
			return;
		}
	}

	// infinitives
	if (posValues[i] & (NOUN_INFINITIVE|VERB_INFINITIVE)) 
	{
		int indirect = indirectObjectRef[currentVerb];
		if (indirect && needRoles[roleIndex] & OBJECT_COMPLEMENT) // change format to mainobject + complement for causals like "I caused Ben to think" and "I make Ben think"
		{
			SetRole(indirect,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2,true); 
			SetRole(i,OBJECT_COMPLEMENT,false);
		}
		else if (subject && needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT)) SetRole(i,(roleIndex == MAINLEVEL) ? MAINSUBJECT : SUBJECT2,true);
		else if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) SetRole(i,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2,true); 
		// otherwise is just part of verbal
		else if (posValues[i] & NOUN_INFINITIVE)
		{
			int at = i;
			while (--at > startSentence && (!(posValues[at] & TO_INFINITIVE) || ignoreWord[at])) {;} // locate the to
			// adjective expects a modifier infinitive phrase
			if (posValues[at-1] & ADJECTIVE_NORMAL && parseFlags[at-1] & ADJECTIVE_TAKING_NOUN_INFINITIVE)  SetRole(i,ADJECTIVE_COMPLEMENT);
			else if (posValues[at-1] & (NOUN_SINGULAR)) SetRole(at,POSTNOMINAL_ADJECTIVE); // postnominal adjective
			// trailing or embedded appositive -- "his wish, to eat, was fulfilled"
			else if (posValues[at-1] == COMMA && roles[at-2] & (OBJECT2|MAINOBJECT|SUBJECT2|MAINSUBJECT) ) SetRole(i,APPOSITIVE);
		}
		return;
	}
	
	// indirect object in normal speech must be an animate being.  But "I told the mountain my name" would be legal, just uncommon.
	if (indirectObject )
	{
		// is next thing compatible with the kind of direct object allowed?
		uint64 verbFlags = canSysFlags[currentVerb];
		bool allowed = false;
		if (verbFlags & VERB_INDIRECTOBJECT) allowed = true; // generic is allowed
		else if (verbFlags & VERB_TAKES_INDIRECT_THEN_TOINFINITIVE && !stricmp(wordStarts[NextPos(i)],"to")) allowed = true;  // "I allowed John *to run"
		else if (verbFlags & VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE && posValues[NextPos(i)] & VERB_INFINITIVE) allowed = true; // "he made John *run"
		if (!allowed) indirectObject = 0; 
		else if (originalLower[i] && originalLower[i]->properties & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL | NOUN_HUMAN | PRONOUN_SUBJECT | PRONOUN_OBJECT )) // proper names are good as are pronouns
		{
		}
		else if (canSysFlags[i] & ANIMATE_BEING) // animate beings are good
		{
		}
		else if (i > 1 && IsUpperCase(*wordStarts[i])){;} // anything uppercase not 1st word is good
		else 
		{
			if (trace & TRACE_POS && (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"    Abandoning indirect object at %s nonanimate\r\n",wordStarts[i]);
			needRoles[roleIndex] &= -1 ^ (MAININDIRECTOBJECT|INDIRECTOBJECT2); 
			indirectObject = 0;
		}
	}
retry:
	 // if we have a subject but no verb yet, on main level presume we are requesting an object to handle subject/object inversions
	if (!subject && !directObject && !indirectObject && !currentVerb && roleIndex == MAINLEVEL) directObject = 1;
	if (subject)
	{
		if (predicateZone == -1) {;}									// never located verb yet (ambiguous or missing)
		else if ((int)currentZone == predicateZone) {;}							// zone data says this will be subject since must happen before verb
		else if ((predicateZone - currentZone) == 1 && roleIndex == MAINLEVEL && !(zoneData[currentZone] & ZONE_AMBIGUOUS)) subject = 0;			// cannot have main subject in zone immediately before predicate unless its a clause zone
		else if (zoneData[predicateZone] & ZONE_SUBJECT && roleIndex == MAINLEVEL) subject = 0;		// main sentence is elsewhere
		
		// initial zone cannot support verb which is in next zone with subject, so we are apposive if noun zone
		if (currentZone == 0 && !(zoneData[0] & ZONE_AMBIGUOUS) && zoneData[0] & ZONE_SUBJECT && !(zoneData[0] & ZONE_VERB) && !clauses[i-1] && !verbals[i-1] && !phrases[i-1] && 
			(zoneData[1] & (ZONE_SUBJECT|ZONE_FULLVERB)) == (ZONE_SUBJECT|ZONE_FULLVERB)) 
		{
			subject = 0;
			SetRole(i,APPOSITIVE);
		}
		else if (i < 3 && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] & MAINSUBJECT && IsLegalAddress(i)) SetRole(i,ADDRESS);
		else if (subject)
		{
			// seeking subject but this is noun matching ending prep, with qword heading, so cant be subject "*who did you give your number to" 
			// HOWEVER "what is the name of my friend you talked about"  be verb allowed
			if (i == firstnoun && posValues[endSentence] == PREPOSITION && originalLower[startSentence] && originalLower[startSentence]->properties & QWORD && !(allOriginalWordBits[i+1] & AUX_BE))
			{
				SetRole(i,OBJECT2);
			}
			else
			{
				if (roles[i] & (OBJECT2|MAINOBJECT) && clauses[i]) AddRole(i,subject); // clause fulfilling mainobject role or object2 role
				else SetRole(i,subject);
				if (posValues[i] & PRONOUN_OBJECT) LimitValues(i,PRONOUN_SUBJECT,(char*)"being subject pronoun changes",changed);

			}
			// guessed appostive now has US as appositive clause, so revert and make him subject - "my mom, whose name is, is"
			if (roleIndex == 2 && lastClause && priorNoun && currentZone && zoneBoundary[currentZone] > priorNoun && priorNoun > zoneBoundary[currentZone-1] && roles[priorNoun] & APPOSITIVE)
			{
				AddRole(i,APPOSITIVE); // we are HIS appositive
				--roleIndex;
				SetRole(priorNoun,MAINSUBJECT,true);
				++roleIndex;
		}
		}
		else if ( posValues[i] & NOUN_NUMBER) // cannot be appositive (except-  "John, the first, went home" maybe)
		{
		}
		return;
	}
	else if (indirectObject )
	{
		if (posValues[i] & PRONOUN_SUBJECT) ForceValues(i,PRONOUN_OBJECT,(char*)"Forcing object pronoun for indirect object",changed);

		bool allowedIndirect = false;
		// can be indirect ONLY if next thing is known a noun indicator (including gerund)
		// except for causal verbs expecting To Infinitive
		unsigned int currentVerb = verbStack[roleIndex];
		while (coordinates[currentVerb] && coordinates[currentVerb] < i && coordinates[currentVerb] > currentVerb) currentVerb = coordinates[currentVerb]; // find most recent verb if conjoined
		if (AcceptsInfinitiveObject(currentVerb) )
		{
			allowedIndirect = true; // causal verb can be in any tense--- "he painted his jalopy black"
		}
		else if (needRoles[roleIndex] & TO_INFINITIVE_OBJECT  && posValues[NextPos(i)] & NOUN_INFINITIVE) allowedIndirect = true;
		else if (posValues[NextPos(i)] & (SIGNS_OF_NOUN_BITS|TO_INFINITIVE))
		{
			if (posValues[NextPos(i)] & ADJECTIVE_BITS && NextPos(i) == endSentence) {;} // if sentence ends in adjective, this is not indirect obj - eg "This made *Jen mad"
			else allowedIndirect = true;
		}
		if (allowedIndirect && stricmp(wordStarts[i+1],(char*)"who" )  && stricmp(wordStarts[i+1],(char*)"that" )) // no clause starter modifying it
		{
			SetRole(i,indirectObject);
			directObject = 0;
		}
		else
		{
			indirectObject = 0;
			goto retry;
		}
	}
	else if (directObject )
	{
		// sentence starting with qword determiner (which and what for example) or possessive (whose) on this noun  "what fruit do you like"
		if (roleIndex == MAINLEVEL)
		{
			bool flip = false;
			int oldsubject = subjectStack[roleIndex];
			if (!oldsubject){;}
			else if (allOriginalWordBits[oldsubject] & QWORD)
			{
				// What(O) is a dog(S)?  What(O) do dogs(S) eat?  But not "What dogs eat is good" or "what eats dogs"
				// Who(O) is my master(S) but not "who do dogs eat but we allow it because who/whom is hard for people
				if (verbStack[MAINLEVEL] && allOriginalWordBits[verbStack[MAINLEVEL]] & AUX_BE) flip = true; 
				else if (auxVerbStack[MAINLEVEL] && needRoles[roleIndex] & MAINVERB) flip = true;
			}
			else if (allOriginalWordBits[startSentence] & QWORD && oldsubject == (startSentence+1))
			{
				// What color(O) is a dog(S)?  
				if (verbStack[MAINLEVEL] && allOriginalWordBits[verbStack[MAINLEVEL]] & AUX_BE) flip = true; 
				else if (auxVerbStack[MAINLEVEL] && needRoles[roleIndex] & MAINVERB) flip = true;
			}

			// main sentence ending in preposition, with Qword at front, qword becomes OBJECT2--- Eg. who are you afraid of -- but not what dog are you afraid of
			if (posValues[endSentence] == PREPOSITION  && allOriginalWordBits[oldsubject] & QWORD && startSentence == oldsubject )
			{
				SetRole(i,MAINSUBJECT,true);
				if (posValues[i] & PRONOUN_OBJECT)  ForceValues(i,PRONOUN_SUBJECT,(char*)"Forcing subject pronoun",changed);
				SetRole(oldsubject,OBJECT2,true);
				if (posValues[oldsubject] & PRONOUN_SUBJECT)  ForceValues(oldsubject,PRONOUN_OBJECT,(char*)"Forcing object pronoun",changed);
			}
			// existential there  "There is no one working with Albert"
			else if (allOriginalWordBits[oldsubject] & THERE_EXISTENTIAL && allOriginalWordBits[auxVerbStack[MAINLEVEL]] & AUX_BE )
			{
				SetRole(oldsubject,0,true);
				SetRole(i,MAINSUBJECT,true);
				if (posValues[i] & PRONOUN_OBJECT)  ForceValues(i,PRONOUN_SUBJECT,(char*)"Forcing subject pronoun from existential there",changed);
			}
			else if (flip) 
			{
				needRoles[roleIndex] &= -1 ^ (OBJECT_COMPLEMENT|MAININDIRECTOBJECT); // in addition to ones cleared by setrole
				SetRole(oldsubject,MAINOBJECT,true);
				if (posValues[oldsubject] & PRONOUN_SUBJECT)  ForceValues(oldsubject,PRONOUN_OBJECT,(char*)"Forcing object pronoun",changed);
				SetRole(i,MAINSUBJECT,true);
				if (posValues[i] & PRONOUN_OBJECT)  ForceValues(i,PRONOUN_SUBJECT,(char*)"Forcing subject pronoun",changed);
			}
			else SetRole(i,MAINOBJECT);
			directObject = 0;

			// if this is NOUN and verb was factitive, revise to not have indirect object - "they elected the milkman president"  "they named our college the best" -- but not "joey found his baby bottle" but "he found the baby unpleasant"
			if (parseFlags[currentMainVerb] & FACTITIVE_NOUN_VERB && indirectObjectRef[currentMainVerb] && posValues[i] & NOUN_BITS)  
			{
				SetRole(indirectObjectRef[currentMainVerb],MAINOBJECT,true);
				SetRole(i,OBJECT_COMPLEMENT);
			}
			// singular direct object must be determined or possessed or mass or Proper -- "I walked my dog" not "I walked dog"
			else if (roleIndex == MAINLEVEL && (currentZone - predicateZone ) == 1 && !roles[i]) // illegal for object to be in adjacent zone unless it was a propogated already role from Coord conjunct - "tom sent email, cards, and letters"
			{
			}
		}
		// if this is NOUN and verb was factitive, revise to not have indirect object - "they elected the milkman president"  "they named our college the best" -- but not "joey found his baby bottle" but "he found the baby unpleasant"
		else if (roleIndex > MAINLEVEL && parseFlags[currentVerb2] & FACTITIVE_NOUN_VERB && indirectObjectRef[currentVerb2] && posValues[i] & NOUN_BITS)  
		{
			SetRole(indirectObjectRef[currentVerb2], OBJECT2);
			SetRole(i,OBJECT_COMPLEMENT);
		}
		else
		{
			if (posValues[i] & PRONOUN_SUBJECT) ForceValues(i,PRONOUN_OBJECT,(char*)"Forcing object pronoun",changed);
			SetRole(i,directObject);
			if (needRoles[roleIndex] & VERBAL) 
			{
				ExtendChunk(lastVerbal,i,verbals);
				DropLevel(); 
				lastVerbal = 0;
			}
			if (needRoles[roleIndex] & CLAUSE) 
			{
				unsigned int prior = FindPriorVerb(i);
				if (prior) crossReference[i] = (unsigned char)prior;		// link from object to clause verb
				ExtendChunk(lastClause,i,clauses);
				DropLevel(); 
				lastClause = 0;
			}
			if (needRoles[roleIndex] & PHRASE)
			{
				crossReference[i] = (unsigned char) lastPhrase; // objects point back to prep that spawned them
				ExtendChunk(lastPhrase,i,phrases);
				DropLevel(); 
				// though I walk thru the valley *"of the shadow" merge to clause
				if (lastPhrase != 1 && clauses[lastPhrase - 1])
					ExtendChunk(lastPhrase - 1, i, clauses);
				lastPhrase = 0;
			}
		}
	}		
	else if (roleIndex == MAINLEVEL && needRoles[roleIndex] == 0 && IsLegalAddress(i) && !coordinates[i]) SetRole(i,ADDRESS);
	// simple superfluous noun immediately follows simple noun used as object, presume he was adjective_noun
	else if (roles[i-1] & OBJECT2 && posValues[i-1] & NOUN_SINGULAR && posValues[i] & (NOUN_SINGULAR|NOUN_PLURAL))
	{
		roles[i] = OBJECT2; // direct object assign, no level associated any more probably
		roles[i-1] = 0;
		verbals[i] = verbals[i-1];
		phrases[i] = phrases[i-1];
		clauses[i] = clauses[i-1];
		LimitValues(i-1,ADJECTIVE_NOUN,(char*)"superfluous noun forcing prior noun to adjective status",changed);
	}
	else if (!(posValues[i] & (NOUN_INFINITIVE|NOUN_GERUND)) && priorNoun && phrases[priorNoun] && !lastPhrase && (i - priorNoun) < 5) // there was a prep object, now finished, are we appostive to it - "John Kennedy the popular US president was quite different from John Kennedy the unfaithful *husband."
	{
		SetRole(i,APPOSITIVE);
		ExtendChunk(priorNoun,i,phrases);
	}
	// appositives will be next noun (not being used as adjective)
	else if (objectRef[currentVerb] && objectRef[currentVerb] < i) // object happens earlier 
	{
		int base = i;
		while (--base > objectRef[currentVerb]) 
		{
			if (ignoreWord[base]) continue;
			if (posValues[base] & CONJUNCTION_COORDINATE) break;
		}
		if (posValues[base] & CONJUNCTION_COORDINATE && (roles[base] & CONJUNCT_KINDS) == CONJUNCT_NOUN) // later must match earlier
		{
			unsigned int after = coordinates[base];
			unsigned int before = coordinates[after];
			SetRole(i,roles[before]);
		}
		else if (needRoles[roleIndex] & SUBJECT2) SetRole(i,SUBJECT2);
		else if (needRoles[roleIndex] & OBJECT_COMPLEMENT && verbStack[roleIndex] && objectRef[currentVerb]) SetRole(i,OBJECT_COMPLEMENT);
		else if (base == objectRef[currentVerb] && !(posValues[i] & (NOUN_GERUND|NOUN_INFINITIVE))) SetRole(i,APPOSITIVE); // or form of address
		else  SetRole(i,OBJECT2); // unknown
	}
	else if (subjectStack[roleIndex] && subjectStack[roleIndex] < i) // subject happens earlier 
	{
		if (needRoles[roleIndex] == ( CLAUSE | VERB2)) // clause has subject, was it qword?
		{
			// change subject to object and us be subject
			if (originalLower[subjectStack[roleIndex]] && originalLower[subjectStack[roleIndex]]->properties & QWORD && roles[subjectStack[roleIndex]] & SUBJECT2)
			{
				SetRole(subjectStack[roleIndex],OBJECT2);
				SetRole(i,SUBJECT2);
				return;
			}
		}

		// can this be actual object instead of subject complement structure?
		int at = i;
		while (--at > startSentence)
		{
			if (ignoreWord[at]) continue;
			if (roles[at] & SUBJECT_COMPLEMENT) break;
			if (posValues[at] & (DETERMINER_BITS|ADJECTIVE_BITS)) {;}
			else break;  // not the right thing
		}
		if (roles[at] & SUBJECT_COMPLEMENT) // change this over
		{
			SetRole(at,0,true);
			if (allOriginalWordBits[at] & DETERMINER_BITS) 
			{
				LimitValues(at,DETERMINER_BITS,(char*)"convert subject complement to determiner of some kind",changed);
			}
			SetRole(i,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2,true);
			return;
		}

		int base = i;
		while (--base > subjectStack[roleIndex]) 
		{
			if (ignoreWord[base]) continue;
			if (posValues[base] & CONJUNCTION_COORDINATE) break; 
		}
		if (coordinates[i] && coordinates[i] < 1){;} // coord copies the role
		else if ( verbStack[roleIndex] && subjectStack[roleIndex] < verbStack[roleIndex] && i > verbStack[roleIndex]) 
		{
			SetRole(i,OBJECT2); // handles "the man was eager to *die" - adjective_object can take a complement sometimes
		}
		else if (base == subjectStack[roleIndex] && IsLegalAppositive(base,i)) // appostive SUBJECT?
		{
			// if the word before is ALSO a noun appositive, convert it to an adjective noun
			if (roles[i-1] & APPOSITIVE && posValues[i-1] & NOUN_BITS)
			{
				roles[i-1] = 0;
				LimitValues(i-1,ADJECTIVE_NOUN,(char*)"Forcing prior noun appositive to be adjective noun",changed); 
				if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Forcing prior noun appositive to be adjective noun %s\r\n",wordStarts[i-1]);
			}
			SetRole(i,APPOSITIVE); 
		}
		else if (!roles[i])  SetRole(i,SUBJECT2); // unknown
		// or form of address
	}
	 // presuming it is appositive after noun :  "I hit my friend *George"
}

static void ShowZone(unsigned int zone)
{
	if (zoneData[zone] & ZONE_AMBIGUOUS) Log(STDTRACELOG,(char*)"Ambiguous zone, ");
	else if (zoneData[zone] & ZONE_ABSOLUTE) Log(STDTRACELOG,(char*)"Absolute zone, ");
	else if ((zoneData[zone] & (ZONE_SUBJECT|ZONE_VERB)) == (ZONE_SUBJECT|ZONE_VERB)) 
	{
		if (zoneData[zone] & ZONE_FULLVERB) Log(STDTRACELOG,(char*)"subject/verb zone, ");
		else Log(STDTRACELOG,(char*)"Appositive zone, ");
	}
	else if ((zoneData[zone] & ZONE_VERB)) Log(STDTRACELOG,(char*)"Predicate zone, ");
	else if (zoneData[zone] & ZONE_SUBJECT && !(zoneData[zone] & ZONE_VERB)) 
	{
		if (zoneData[zone] & ZONE_ADDRESS) Log(STDTRACELOG,(char*)"Address zone, ");
		else Log(STDTRACELOG,(char*)"NounPhrase zone, "); // might be subject or object with appositive or something else after it
	}
	else if (zoneData[zone] == ZONE_PCV) Log(STDTRACELOG,(char*)"Phrases zone, ");
	else Log(STDTRACELOG,(char*)"??? zone,(char*)");
}

#ifdef INFORMATION
A needRoles value indicates what we are currently working on (nested), what we want to find to complete this role.
Base level is always the main sentence. Initially subject and verb. When we get a verb, we may add on looking for object and indirect object.
	Other levels may be:
1. phrase (from start of phrase which is usually a preposition but might be omitted) til noun ending phrase
but-- noun might be gerund in which case it extends to find objects of that.
2. clause (from start of clause which might be omitted til verb ending clause
but -- might have objects which extend it, might have gerunds which extend it (and it will include prep phrases attached at the end of it - presumed attached to ending noun)
3. verbal (infinitive and gerund and adjective_participle) - adds a level to include potential objects. 

The MarkPrep handles finding and marking basic phrases and clauses. AssignRoles extends those when the system "might" find an object.
It will backpatch later if it finds it swalllowed a noun into the wrong use.
#endif

static bool ValidateSentence(bool &resolved)
{
	// now verify sentence is proper   
	// 0. all words are resolved and there is a main verb
	// 1. all nouns and pronouns and verbs have roles. Any object2 role noun must be a part of a phrase or clause or verbal (not free)
	// 2. predeterminer determiner or adjective must find its closing noun -- but adjective MAY use linking verb to do so
	// 3. preposition must find its closing noun
	// 4. clause has both subject and verb
	// 5. currently no constraints on adverbs or particles (though they go with a verb)
	// 6. currently dont require predeterminer to find a determiner
	// main object cannot preceed main verb unless in 1st position?
	// if it has no subject, the verb is infinitive.
	// main verb is present/past if no aux, is infinitive or participle if has aux
	 int subject = subjectStack[MAINLEVEL];
	 int verb = verbStack[MAINLEVEL];
	 int object = objectRef[verbStack[MAINLEVEL]];
	 int aux = auxVerbStack[MAINLEVEL];
	 int lastSubjectComplement = 0;

	bool isQuestion =  ((tokenFlags & QUESTIONMARK) || (allOriginalWordBits[startSentence] & QWORD) || (allOriginalWordBits[startSentence+1] & QWORD && allOriginalWordBits[startSentence] & PREPOSITION)) ? true : false;

	if (roleIndex > MAINLEVEL) 
	{
		resolved = false; 
		if (trace & TRACE_POS && (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: roleIndex > main level\r\n");
	}
	// check main verb  vs aux
	else if (!verb) // main verb doesnt exist
	{
		resolved = false; // some unfinished business
		if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: no main verb\r\n");
	}
	if (auxVerbStack[MAINLEVEL] && resolved) // aux exists, prove main verb matches
	{ // legal: "have you had your breakfast"  -- but so is "you are having your breakfast"
		if (posValues[verb] & (VERB_PRESENT|VERB_PRESENT_3PS))
		{
			resolved = false; // some unfinished business
			if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: main verb %s has aux but not aux tense\r\n",wordStarts[verb]);
		}
	}
	else if (resolved)// no aux exists, prove main verb matches
	{
		if (posValues[verb] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS)){;}
		else if (posValues[verb] == VERB_INFINITIVE && !subjectStack[MAINLEVEL]) {;} // command
		else if (posValues[verb] == VERB_INFINITIVE && aux == 1) {;} // helped verb
		else if (roles[verb] & VERB2) {;} // allowable infinitive
		else
		{
			resolved = false; // some unfinished business
			if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: main verb %s has aux tense but no aux\r\n",wordStarts[verb]);
		}
	}
	// prove subject/verb match
	if (!subject)
	{
		if (resolved && posValues[verb] != VERB_INFINITIVE)
		{
			resolved = false; // some unfinished business
			if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: main verb lacks subject and is not infinitive\r\n");
		}
		else if (resolved && posValues[verb] == VERB_PRESENT  && posValues[subject] & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR))
		{
			resolved = false; // some unfinished business
			if (trace & TRACE_POS  &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: main verb is plural and subject is singulare\r\n");
		}
		else if (resolved && posValues[verb] == VERB_PRESENT_3PS && posValues[subject] & (NOUN_PLURAL|NOUN_PROPER_PLURAL) )
		{
			resolved = false; // some unfinished business
			if (trace & TRACE_POS  &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: main verb is singular and subject is plural\r\n");
		}
	}
	// prove subject is not in zone immediately before verb zone (except coordinate zone is fine since conjoined subject will span multiple zones
	if (resolved && subject && verb && (zoneMember[verb] - zoneMember[subject]) == 1 && !clauses[subject] && !coordinates[subject])  
	{
		// resolved = false; // some unfinished business
		// if (trace & TRACE_POS  &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: subject in zone immediately before verb\r\n"); // dumb 1st grade has "the dog with the black spot on his tail, is with Tom"
	}

	int adjective = 0;
	int preposition = 0;
	bool clauseSubject = false;
	bool clauseVerb = false;
	unsigned int currentClause = 0;
	int i = 0;

	bool accepted = false;
	int quotationIndex = 0;

	if (resolved) for (i = startSentence; i <= endSentence+1; ++i) // walk off end of sentence
	{
		if (ignoreWord[i]) continue;
		if (roles[i] == TAGQUESTION) break; // dont care after here
		char* xxword = wordStarts[i]; // for debugging
		if (roles[i] & MAINVERB) verb = i; // most recent copy since there can be conjoined ones, some with objects and some without
		if (roles[i] & MAINOBJECT)
		{
			object = i; // most recent copy
			// prove object is not in zone immediately after verb zone
			if (resolved && object && verb && (zoneMember[object] - zoneMember[verb]) == 1 && !coordinates[object]) // verb can be in earlier zone if conjoined objects
			{
				resolved = false; // some unfinished business
				if (trace & TRACE_POS  &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: object in zone immediately after verb\r\n");
			}
		}

		if (roles[i] & (MAINVERB|VERB2) && indirectObjectRef[i] && !objectRef[i])
		{
				resolved = false; // some unfinished business
				if (trace & TRACE_POS  &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: freestanding indirect object\r\n");
		}
	
		else if (roles[i] & MAINVERB && !isQuestion && objectRef[i] && objectRef[i] < verb && objectRef[i] != startSentence)
		{
			resolved = false; // some unfinished business
			if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: object preceeds verb and not start or question r\n");
		}
		
		// check for tag questions
		if (roles[i] & TAGQUESTION)
		{
			accepted = true;
			break;
		}
	
		// object 2 can occur at start of sentence after ending prep "who did you give it to"   or during quotation
		if (roles[i] & OBJECT2 && !clauses[i] &&  !quotationIndex && !verbals[i] && !phrases[i] && (!coordinates[i] || coordinates[i] > i) ) 
		{
			if (posValues[endSentence] == PREPOSITION && i == firstnoun) 
			{
				for (int x = 1; x <= firstnoun; ++x) phrases[x] = phrases[endSentence];
			}
			else
			{
				if (trace & TRACE_POS  &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: %s OBJECT2 not in clause/verbal/phrase\r\n",wordStarts[i]);
				break;	// free floating noun?
			}
		}

		if (roles[i] & SUBJECT_COMPLEMENT)
		{
			if ((i-1) == lastSubjectComplement) 
			{
				if (trace & TRACE_POS  &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: Two subject complements in a row: %s %s\r\n",wordStarts[i-1],wordStarts[i]);
				break;	// free floating noun?
			}
			lastSubjectComplement = i;
		}
		// confirm clause has subject and verb or omits them intentionally
		if (!clauses[i] && currentClause) // clause is ending 
		{
			// we are allowed to omit a clause subject - but can we omit the verb
			if (clauseVerb && roles[currentClause] != OMITTED_SUBJECT_VERB) 
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: ending a clause not fulfilled\r\n");
				break;	// didnt fulfill these
			}
			currentClause = 0;
		}
		else if (posValues[i] & QUOTE && quotationIndex == 0) // quotation clause is starting
		{
			++quotationIndex;
			clauseSubject = clauseVerb = true; 
		}
		else if (clauses[i] != clauses[i-1]) // clause is starting
		{
			if (currentClause && clauseVerb)// we are allowed to omit a clause subject but not verb
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: changing from a clause not fulfilled\r\n");
				break; // failed to complete earlier clause
			}
			clauseSubject = clauseVerb = true; 
			if (roles[i] == OMITTED_SUBJECT_VERB)  clauseSubject = clauseVerb = false;  //  supplies implied "it is"
			currentClause = clauses[i];
		}
		if (clauseSubject && roles[i] & SUBJECT2) clauseSubject = 0;
		if (clauseVerb && roles[i] & VERB2)  clauseVerb = 0;

		if (preposition && roles[i] & OBJECT2) preposition = 0;

		if (posValues[i] & ADJECTIVE_PARTICIPLE) {;} // doesnt require a noun (comes after or before or with be verb)
		else if (posValues[i] & NOUN_ADJECTIVE) 
		{
			adjective = preposition = 0; 
		}
		else if (posValues[i] & DETERMINER_BITS)
		{
			adjective = i; // want a noun to complete this
		}
		else if (posValues[i] & ADJECTIVE_BITS)
		{
			if (!(roles[i] & (SUBJECT_COMPLEMENT|OBJECT_COMPLEMENT))) adjective = i; // want a noun to complete this
		}
		else if (posValues[i] & PREPOSITION)// adjective or preposition requires NOUN closure by now
		{
			if (adjective && !(roles[adjective] & POSTNOMINAL_ADJECTIVE)) 
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: adj %s (%d) not fulfilled on seeing prep %s\r\n",wordStarts[adjective],adjective,wordStarts[i]);
				preposition = adjective = 0;
				break;
			}
			if (preposition && stricmp(wordStarts[preposition],(char*)"from")) // "from under the bed" is legal" - from is an unusual prep 
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: prep not fulfilled on seeing prep %s\r\n",wordStarts[preposition],preposition,wordStarts[i]);
				preposition = adjective = 0;
				break;
			}
			adjective = 0;
			preposition = i;
		}
		else if (posValues[i] & VERB_BITS)
		{
			if (adjective || preposition) // adjective or preposition requires NOUN closure
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) 
				{
					if (adjective) Log(STDTRACELOG,(char*)"badparse: adj %s not fulfilled on seeing verb %s\r\n",wordStarts[adjective],wordStarts[i]);
					else  Log(STDTRACELOG,(char*)"badparse: prep %s not fulfilled on seeing verb %s\r\n",wordStarts[preposition],wordStarts[i]);
				}
				break;
			}
			if (!roles[i] && !verbals[i]) // if verbal, might be adverb verbal
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: verb with no known role\r\n");
				break;	// we dont know what it is doing
			}
			if (roles[i] & MAINVERB && !subjectStack[MAINLEVEL] && posValues[i] != VERB_INFINITIVE)// imperative failure
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: non-imperative main verb %s has no subject \r\n,wordStarts[i]");
				break;
			}
		}
		else if (posValues[i] & ADVERB)
		{
			if (preposition && stricmp(wordStarts[preposition],(char*)"from") && !(posValues[NextPos(i)] & (ADVERB|ADJECTIVE_BITS|DETERMINER_BITS|NOUN_BITS|PRONOUN_BITS))) // "from under the bed" is legal" - from is an unusual prep 
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: prep not fulfilled on seeing adverb %s\r\n",wordStarts[preposition],preposition,wordStarts[i]);
				preposition = adjective = 0;
				break;
			}
		}
		else if (posValues[i] & NOUN_INFINITIVE)
		{
			if (adjective) // cannot describe a noun infinitive
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: unfinished adjective %s at noun infinitive %s\r\n",wordStarts[adjective],wordStarts[i]);
				break;
			}
			// its marked on to infinitve
		}
		else if (posValues[i] & TO_INFINITIVE)
		{
			if (!roles[i]) // we dont know what it is doing, must be an adverb phrase
			{
			}
		}
		else if (posValues[i] & NOUN_BITS)
		{
			adjective = false;
			if (!roles[i]) 	// we dont know what it is doing
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: unknown role for noun %s\r\n",wordStarts[i]);
				break;
			}
		}
		else if (posValues[i] & (PRONOUN_BITS))
		{
			adjective = false; // if we had adjectives before "the blue one" or the "blue you", this closes it
			if (!roles[i]) 	// we dont know what it is doing
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse: unknown role for pronoun %s\r\n",wordStarts[i]);
				break;
			}
		}
		else if (posValues[i] & CONJUNCTION_SUBORDINATE)
		{
			if (adjective || preposition) // adjective or prep requires NOUN closure
			{
				if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) 
				{
					if (adjective) Log(STDTRACELOG,(char*)"badparse: adj %s not fulfilled on seeing verb %s\r\n",wordStarts[adjective],wordStarts[i]);
					else  Log(STDTRACELOG,(char*)"badparse: prep %s not fulfilled on seeing verb %s\r\n",wordStarts[preposition],wordStarts[i]);
				}
				break;
			}
		}
		else if (i > endSentence) //  end of sentence
		{
			if (preposition && phrases[endSentence] == phrases[startSentence]) {;} // wraps back to front
			else if (preposition || clauseVerb) // adjective or preposition or clause requires  closure - we can omit clause subjects
			{
				if (posValues[endSentence] == PREPOSITION && roles[startSentence] & OBJECT2){;} // wrapped prep to start as question
				else
				{
					if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse:  unfinished prep or clause at end of sentence\r\n");
					break; 
				}
			}
			if (adjective && verbStack[MAINLEVEL]) // allowed to hang only if in main be sentence with linking verb or as object complement "This made mom mad" or adjective omitted noun
			{
				if (canonicalLower[verbStack[MAINLEVEL]] && !(canSysFlags[verbStack[MAINLEVEL]] & VERB_TAKES_ADJECTIVE) && !(roles[adjective-1] & MAINOBJECT))
				{
					if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) Log(STDTRACELOG,(char*)"badparse:  unfinished adjective %s at end of sentence\r\n",wordStarts[adjective]);
					break; // not a be sentence -- bug need to do seem as well
				}
			}		
		}
		if (roles[i] & SUBJECT2 && !clauses[i] && !(roles[i] & ABSOLUTE_PHRASE))
		{
			if (trace & TRACE_POS &&  (prepareMode != NO_MODE && prepareMode != POSVERIFY_MODE)) 
			{
				Log(STDTRACELOG,(char*)"badparse:  Subject2 %s not in clause\r\n",wordStarts[i]);
				break; 
			}
		}
		// CONSIDER ALSO COORDINATE CONJUNCTION IF SENTENCE COORDINATOR
		//else if (posValues[i] && (ADVERB | PARTICLES | COMMA | PAREN | NOUN_INFINITIVE | THERE_EXISTENTIAL |  TO_INFINITIVE));
	}

	if (i == (endSentence+2)) accepted = true;	 // found nothing to stop us
	else if (roles[i] == TAGQUESTION) accepted = true;	 // found our way to tag
	else if (i <= (endSentence + 1) && !accepted) resolved = false; // failed something along the way

	return resolved;
}

static void AssignZones() // zones are comma areas
{
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"     Zones: ");
	memset(zoneData,0,sizeof(unsigned int) * ZONE_LIMIT); 
	memset(zoneMember,0,sizeof(unsigned char) * MAX_SENTENCE_LENGTH); 
	currentZone = 0;
	zoneIndex = 0;
	for ( int i = startSentence; i <= (int)endSentence; ++i)
	{
		if (ignoreWord[i]) continue;	// ignore these
		zoneMember[i] = (unsigned char)zoneIndex; // word is in current zone
		if (phrases[i] || clauses[i] || verbals[i]) zoneData[zoneIndex] |= ZONE_PCV; // a phrase/clause/verbal runs thru here -- and has been marked all the way thru the noun as well
		if (bitCounts[i] != 1) zoneData[zoneIndex] |= ZONE_AMBIGUOUS;
		else if (posValues[i] & (PRONOUN_BITS|NOUN_BITS|DETERMINER)) // this is definitely a noun here
		{
			zoneData[zoneIndex] |=  (zoneData[zoneIndex] & ZONE_VERB) ? ZONE_OBJECT : ZONE_SUBJECT;
			// the only way to know an address zone is NOT the subject, is to find the subject in a different zone in advance (presumably next)
			// and zone 1 wouldnt be appositive in that case  "Bill, my parrot is good" because a proper name wouldnt be appositive coming first
			if ( i == startSentence && posValues[i] & NOUN_PROPER_SINGULAR && posValues[NextPos(i)] == COMMA && zoneData[zoneIndex+1] & ZONE_SUBJECT) 
				zoneData[zoneIndex] |= ZONE_ADDRESS;
			if ( i == startSentence && posValues[i] & NOUN_PROPER_SINGULAR && posValues[NextPos(i)] == COMMA && posValues[Next2Pos(i)] & (PRONOUN_SUBJECT | PRONOUN_OBJECT |NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) 
				zoneData[zoneIndex] |= ZONE_ADDRESS;
		}
		else if (posValues[i] & VERB_BITS) // this is definitely a verb but might be from a clause
		{
			zoneData[zoneIndex] |= ZONE_VERB;
			if (posValues[i] & (VERB_PAST_PARTICIPLE|VERB_PRESENT_PARTICIPLE))
			{
				if (zoneData[zoneIndex] & ZONE_SUBJECT && !(zoneData[zoneIndex] & ZONE_AUX) ){;}
				else if (zoneData[zoneIndex] & ZONE_AUX) predicateZone = zoneIndex;
			}
			else  zoneData[zoneIndex] |= ZONE_FULLVERB;
		}
		else if (posValues[i] & AUX_VERB) zoneData[zoneIndex] |= ZONE_FULLVERB | ZONE_AUX; 
		else if (posValues[i] & COMMA) 
		{
			// too many commas
			if (zoneIndex == (ZONE_LIMIT-1)) return;
			zoneData[++zoneIndex] = 0;
			zoneBoundary[zoneIndex] = (unsigned char)i;
		}
	}
	if (trace & TRACE_POS) 
	{
		for (unsigned int i = 0; i <= zoneIndex; ++i) ShowZone(i);
		Log(STDTRACELOG,(char*)"\r\n");
	}
	if (zoneIndex == (ZONE_LIMIT-1)) return;
	zoneBoundary[++zoneIndex] = (unsigned char)(endSentence + 1);	// end of last current zone

	unsigned int lastComma = (zoneIndex) ? zoneBoundary[zoneIndex-1] : 0;
	if (lastComma)
	{
		if ((endSentence-lastComma) == 3 && posValues[lastComma+1] & VERB_BITS && allOriginalWordBits[lastComma+1] & AUX_VERB && !stricmp(wordStarts[lastComma+2],(char*)"not") && posValues[lastComma+3] & PRONOUN_BITS) roles[lastComma] = TAGQUESTION; // don't they ?
		if ((endSentence-lastComma) == 2 && posValues[lastComma+1] & VERB_BITS  && allOriginalWordBits[lastComma+1] & AUX_VERB  && posValues[lastComma+2] & PRONOUN_BITS) roles[lastComma] = TAGQUESTION; // do you ? do theirs?
		if ((endSentence-lastComma) == 2 &&  (!stricmp(wordStarts[lastComma+1],(char*)"and") || !stricmp(wordStarts[lastComma + 1], (char*)"&") ) &&  !strnicmp(wordStarts[lastComma+2],(char*)"you",3)) roles[lastComma] = TAGQUESTION;//  and you ?  - and yours?
		if ((endSentence-lastComma) == 2 &&  !strnicmp(wordStarts[lastComma+1],(char*)"you",3) &&  !stricmp(wordStarts[lastComma+2],(char*)"too")) roles[lastComma] = TAGQUESTION;//  yours too?
		if ((endSentence-lastComma) == 1 &&  !strnicmp(wordStarts[lastComma+1],(char*)"you",3) ) roles[lastComma] = TAGQUESTION; //  you ?  yours?
	}
}

static unsigned int GuessAmbiguousNoun(int i, bool &changed)
{
	unsigned int currentVerb = (roleIndex == MAINLEVEL) ? currentMainVerb : currentVerb2;

	// if this is adjective or noun and following is a possible noun, be adjective, unless we are sentence start and "here" "there" etc
	if (posValues[i] & ADJECTIVE_NORMAL && posValues[i] & NOUN_SINGULAR && posValues[NextPos(i)] & (NOUN_SINGULAR|NOUN_PLURAL))
	{
		if (posValues[i] & ADVERB && i == startSentence && (!stricmp(wordStarts[i],(char*)"here") || !stricmp(wordStarts[i],(char*)"there"))) {;}
		// not a conflict if we need indirect and this is human
		else if (canSysFlags[i] & ANIMATE_BEING && needRoles[roleIndex] & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) // "he had the *mechanic check the brakes"
		{
			if (LimitValues(i,NOUN_SINGULAR ,(char*)"adj noun conflict needing indirect, use noun",changed)) return GUESS_RETRY;
		}
	}

	// big congressional spending *bill - need nothing because spending was made noun, but could have been other...
	if (posValues[i] & NOUN_SINGULAR && roleIndex == MAINLEVEL && needRoles[roleIndex] == 0 &&
		posValues[i-1] & NOUN_SINGULAR && allOriginalWordBits[i-1] & ADJECTIVE_PARTICIPLE)
	{
		uint64 role = roles[i-1];
		LimitValues(i-1,ADJECTIVE_PARTICIPLE,(char*)"converting to adj-participle something before unneeded noun",changed);
		SetRole(i-1,0,true);
		if (role & (MAINSUBJECT|MAINOBJECT)) needRoles[MAINLEVEL] |= role & (MAINSUBJECT|MAINOBJECT);
		if (LimitValues(i,NOUN_SINGULAR ,(char*)"forcing noun after adj-particple adjusted",changed)) return GUESS_RETRY;
	}

	int beAdj = 0;
	if (posValues[NextPos(i)] & NOUN_BITS && bitCounts[NextPos(i)] == 1) beAdj = i+1;
	else if (i < (endSentence - 1) && posValues[NextPos(i)] & ADJECTIVE_BITS  && bitCounts[NextPos(i)] == 1 && posValues[Next2Pos(i)] & NOUN_BITS) beAdj = Next2Pos(i);
	else if (i < (endSentence - 2) && posValues[NextPos(i)] & ADJECTIVE_BITS  && bitCounts[NextPos(i)] == 1 && posValues[Next2Pos(i)] & ADJECTIVE_BITS && posValues[i+3]) beAdj = Next2Pos(i)+1;
	if (!beAdj){;}
	else if (bitCounts[beAdj] == 1) {;} // yes
	else if (canSysFlags[beAdj] & NOUN) {;}
	else beAdj = 0;	// not a probable noun
	if (posValues[i] & NOUN_GERUND && beAdj && !(posValues[i] & (PARTICLE|PREPOSITION))) // can be an adjective, have upcoming noun, and cant be particle (whose verb might take an object) "mom picked *up Eve after school"
	{
		// if verb cannot take an object, must be adjective   // "he hates *decaying broccoli"
		if (!(canSysFlags[i]  & VERB_DIRECTOBJECT)){;}
		// if verb needs an object, be verbal --  // "he hates *eating broccoli"
		else 
		{
			if (LimitValues(i,NOUN_GERUND,(char*)"potential gerund preceeding nearby noun as object, be that",changed)) return GUESS_RETRY;
		}
	}
	
	if (posValues[i] == AMBIGUOUS_VERBAL)
	{
		// we need a noun as subject of a clause, but gerund can mean subject is omitted...
		if (needRoles[roleIndex] & CLAUSE && needRoles[roleIndex] & SUBJECT2)
		{
			if (LimitValues(i,NOUN_GERUND,(char*)"possibly omitted subject w gerund verb in clause",changed)) return GUESS_RETRY; 
		}

		// we need a noun, go for that, unless it is followed by a noun, in which case we be adjective particile
		if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2|MAINSUBJECT|SUBJECT2))
		{
			if (posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS) && !(posValues[NextPos(i)] & (-1 ^ (ADJECTIVE_BITS|NOUN_BITS))) ) {;}
			else if (LimitValues(i,NOUN_GERUND,(char*)"need noun so accept noun-gerund",changed)) return GUESS_RETRY; 
		}
		//  NOUN_GERUND ADJECTIVE_PARTICIPLE conflict after and which has verbal before it
		else if (i > startSentence && posValues[i-1] & CONJUNCTION_COORDINATE && verbals[i-2])
		{
			if (LimitValues(i,NOUN_GERUND,(char*)"gerund/adjparticle conflict after coordinate with verbal before it, go verbal",changed)) return GUESS_RETRY; 
		}
	}

		// do we need a verb next which is in conflict with a noun
	uint64 remainder = posValues[i] & (-1 ^ (NOUN_BITS|VERB_BITS));
	if (needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & VERB_BITS && posValues[i] & NOUN_BITS && !remainder && 
	!(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)))
	{
		if (currentZone < zoneIndex && zoneData[currentZone+1] & ZONE_FULLVERB)	{;} // we expect verb in NEXT zone, so dont force it here
		else if (posValues[NextPos(i)] & (VERB_BITS|AUX_VERB) && !(posValues[NextPos(i)] & (NOUN_BITS|ADJECTIVE_BITS|DETERMINER|POSSESSIVE_BITS)))
		{
			if (LimitValues(i,NOUN_BITS,(char*)"says verb required, but next can be verb or aux so bind this as noun",changed)) return GUESS_RETRY;
		}
	}

	//starting up, if we need subject and verb and after us is noun evidence, we might be flippsed order with qword "what a big *nose he has" - but not "how often can I go"
	if (posValues[i] & VERB_BITS && posValues[i] & NORMAL_NOUN_BITS && posValues[NextPos(i)] & (PRONOUN_BITS|NORMAL_NOUN_BITS) && originalLower[startSentence] && originalLower[startSentence]->properties & QWORD)
	{
		if (!stricmp(wordStarts[i],(char*)"can") && LimitValues(i, AUX_VERB_TENSES,(char*)"qword with can will be aux inverted verb start",changed)) return GUESS_RETRY;
		if (LimitValues(i, NORMAL_NOUN_BITS,(char*)"qword inverted object before subject, accept noun meaning",changed)) return GUESS_RETRY;
	}

	// if follows to infintiive, prefer noun inf over noun sing "they went to *pick berries"
	if (posValues[i-1] == TO_INFINITIVE  && posValues[i] & NORMAL_NOUN_BITS && posValues[i] & NOUN_INFINITIVE)
	{
		if (LimitValues(i,-1 ^ NORMAL_NOUN_BITS,(char*)"following to infinitive wont be normal noun",changed)) return GUESS_RETRY;
	}

	// the word "home" w/o a determiner or adjective and not in prep phrase will be adverb - "I walk *home" but not "I walk to my *home"
	if (!lastPhrase && !stricmp(wordStarts[i],(char*)"home") && posValues[i] & ADVERB && !(posValues[i-1] & (DETERMINER|POSSESSIVE_BITS|ADJECTIVE_BITS)))
	{
		if (LimitValues(i,-1 ^ NORMAL_NOUN_BITS,(char*)"undetermined home will not be a noun",changed)) return GUESS_RETRY;
	}
	
	if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && posValues[i] & NORMAL_NOUN_BITS && !(posValues[i+1] & NORMAL_NOUN_BITS)) // "she was a big *apple"
	{
		// if this is a number and next could be adjective, lets just wait "I have *3 purple mugs"
		if (posValues[i] & NOUN_NUMBER && posValues[i] & ADJECTIVE_NUMBER && posValues[NextPos(i)] & (ADJECTIVE_BITS|NORMAL_NOUN_BITS)) {;}
		else if (posValues[i] & VERB_INFINITIVE && needRoles[roleIndex] & VERB_INFINITIVE_OBJECT){;} // prefer this
		// unless can be adjective and our verb will want complement
		else if (needRoles[roleIndex] & SUBJECT_COMPLEMENT && posValues[i] & ADJECTIVE_BITS) {;} // "she was getting *upset"
		// can be adverb and after it is a noun or evidence - "I bought *back my shoe"
		else if (posValues[i] & ADVERB && posValues[i+1] & (NORMAL_NOUN_BITS|ADJECTIVE_BITS|DETERMINER|PRONOUN_POSSESSIVE)) {}
		else if (LimitValues(i,NORMAL_NOUN_BITS,(char*)"if can be normal noun as object, be that",changed)) return GUESS_RETRY;
	}

	if (posValues[i] & ADJECTIVE_NORMAL && needRoles[roleIndex] & SUBJECT_COMPLEMENT && posValues[i] & (NOUN_INFINITIVE|NOUN_SINGULAR)) // "she was feeling *down"
	{ // particularly "go *home"
		if (LimitValues(i,-1 ^ (NOUN_INFINITIVE|NOUN_SINGULAR),(char*)"if can be adjective as subject complement, dont be noun infintive or singular",changed)) return GUESS_RETRY;
	}

	// if seeking object for main or clause and noun is not determined, kill it unless there is a potential verb after it "bob likes to play"
	int det;
	if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2|MAINSUBJECT|SUBJECT2) && !(needRoles[roleIndex] & PHRASE) && !IsDeterminedNoun(i,det) && !(posValues[i+1] & VERB_BITS))
	{
		if (LimitValues(i,-1 ^ NOUN_BITS,(char*)"undetermined noun cannot be subject or object",changed)) return GUESS_RETRY;
	}

	// if seeking direct object and we have seen adverb just before us, be not a noun if can be an adverb  "John came back *inside"  - cant put adverb before direct object unless it modifies adjective
	if (i > 2 && needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && posValues[i-1] & ADVERB && posValues[i-2] & VERB_BITS && tokenControl & TOKEN_AS_IS)
	{
		needRoles[roleIndex] &= -1 ^ ALL_OBJECTS;
		if (LimitValues(i,-1 ^ NOUN_BITS,(char*)"no noun after verb then adverb as direct object ",changed)) return GUESS_RETRY;
	}

	// object complement:  causal infinitive- "we watched them play"
	if (!(posValues[i] & PREPOSITION) && posValues[i] & VERB_INFINITIVE && needRoles[roleIndex] & OBJECT_COMPLEMENT && !(posValues[i-1] & (NOUN_INFINITIVE|NOUN_GERUND)))
	{
		if (currentVerb && canSysFlags[currentVerb] & VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE && indirectObjectRef[currentVerb] && !objectRef[currentVerb])
		{
			if (LimitValues(i,-1 ^ (NOUN_BITS - NOUN_INFINITIVE),(char*)"conflict noun infinitive, needing object after indirect- go  infinitive if directobject_infinitive complement verb",changed)) return GUESS_RETRY; 
		}
	}

	// if something could be noun or verb infinitive, and we need a noun, use noun - but not "while walking *home, Jack ran"
	// AND must have TO before it or be a verb not requiring it...  not "Nina raced her brother Carlos *down the street"
	if (!(posValues[i] & PREPOSITION) && posValues[i] & NOUN_INFINITIVE && needRoles[roleIndex] & (MAINOBJECT|OBJECT2) && !(posValues[i-1] & (NOUN_INFINITIVE|NOUN_GERUND)))
	{
		// unless causal infinitive- "we watched them play"
	
		if (posValues[i] & ADJECTIVE_PARTICIPLE && needRoles[roleIndex] & SUBJECT_COMPLEMENT) // she was getting *upset"
		{
			if (LimitValues(i,-1 ^ NOUN_BITS,(char*)"need complement, reject noun",changed)) return GUESS_RETRY; 
		}
		if (currentVerb && canSysFlags[currentVerb] & VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE && indirectObjectRef[currentVerb] && !objectRef[currentVerb])
		{
			if (LimitValues(i,NOUN_INFINITIVE,(char*)"conflict noun infinitive, needing object after indirect- go noun infinitive if directobject_infinitive complement verb",changed)) return GUESS_RETRY; 
		}
		else if (needRoles[roleIndex] & TO_INFINITIVE_OBJECT && indirectObjectRef[currentVerb] && !objectRef[currentVerb])
		{
			if (LimitValues(i,NOUN_INFINITIVE,(char*)"conflict noun infinitive, needing object- go noun infinitive if causal verb",changed)) return GUESS_RETRY; 
		}
		else if (posValues[i] & NOUN_SINGULAR ) 
		{
			if (LimitValues(i,NOUN_SINGULAR,(char*)"conflict noun infinitive and singular, needing object- go noun singular",changed)) return GUESS_RETRY;
		}
	}

	// will not have infinitive noun after a gerund. "walking *home is fun
	if (posValues[i] & NOUN_INFINITIVE && posValues[i-1] == NOUN_GERUND)
	{
		if (LimitValues(i,-1 ^ NOUN_INFINITIVE,(char*)"no noun infinitive after a gerund",changed)) return GUESS_RETRY;
	}
	// will not have gerund after a gerund. 
	if (posValues[i] & NOUN_GERUND && posValues[i-1] == NOUN_GERUND)
	{
		if (LimitValues(i,-1 ^ NOUN_GERUND,(char*)"no noun gerund after a gerund",changed)) return GUESS_RETRY;
	}

	// we arent expecting anything, but if word before was noun and we are likely noun, make him adjective and us the noun " he hit a line *item veto"
	if (posValues[i] & (NOUN_BITS - NOUN_PROPER_SINGULAR - NOUN_PROPER_PLURAL ) &&  canSysFlags[i] & NOUN && posValues[i-1] & NOUN_SINGULAR && roles[i-1])
	{
		needRoles[roleIndex] |= roles[i-1];
		SetRole(i-1,0);
		LimitValues(i-1,NOUN_ADJECTIVE,(char*)"convert prior noun to description on this noun",changed);
		if (LimitValues(i,NOUN_BITS,(char*)"needing nothing, extend prior noun to here",changed)) return GUESS_RETRY; 
	}

	// we arent expecting anything, so kill off noun meanings - but cant kill off proper nouns ever - not "I hit my friend *Carlos"
	if (posValues[i] & (NOUN_BITS - NOUN_PROPER_SINGULAR - NOUN_PROPER_PLURAL ) && posValues[i] & (ADVERB|ADJECTIVE_BITS|PARTICLE) && !(posValues[i] & ADJECTIVE_PARTICIPLE) && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0)
	{// but not participles as adjectives so "the plant's construction *cost" can still be noun
		if (LimitValues(i,-1 ^ NOUN_BITS,(char*)"needing nothing, kill off noun meanings in favor of adverb/adj/particle",changed)) return GUESS_RETRY; 
	}

	if (posValues[i] & NOUN_INFINITIVE && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && i == endSentence)
	{
		if (LimitValues(i,-1 ^ NOUN_INFINITIVE,(char*)"no use for noun infinitive at end of sentence",changed)) return GUESS_RETRY;  // "i want to take the day off"
	}

	if (posValues[i-1] == PRONOUN_POSSESSIVE && posValues[i] & (NOUN_INFINITIVE|NOUN_GERUND) && posValues[i] & NOUN_SINGULAR) // remove ambiguity
	{
		if (LimitValues(i,-1 ^ (NOUN_INFINITIVE|NOUN_GERUND),(char*)"potential gerund or infintive preceeded by possessive is killed",changed)) return GUESS_RETRY; 
	}
	
	if (needRoles[roleIndex] & (MAINSUBJECT|OBJECT2|MAINOBJECT|SUBJECT2) && posValues[i] & (NOUN_INFINITIVE|NOUN_GERUND))
	{
		if (i == startSentence && posValues[i] & VERB_INFINITIVE && posValues[NextPos(i)] & (PRONOUN_BITS|DETERMINER_BITS|ADJECTIVE_BITS|NOUN))
		{
			 if (LimitValues(i,-1 ^ NOUN_BITS,(char*)"Potential command at start has object after it be not noun",changed)) return GUESS_RETRY;  // "let him go"
		}
		// Joey found his baby *blanket"
		else if (posValues[i] & NOUN_INFINITIVE && needRoles[roleIndex] & TO_INFINITIVE_OBJECT) // "she ran *back home"
		{
			if (LimitValues(i,-1 ^ NOUN_INFINITIVE,(char*)"Need a noun, but infinitive lacks to and prior verb cant accept that, delete",changed)) return GUESS_RETRY; 
		}
		else if (posValues[i] & NOUN_INFINITIVE && verbStack[roleIndex] &&  posValues[i] & (NOUN_SINGULAR|NOUN_PLURAL) && posValues[i-1] & (DETERMINER|ADJECTIVE_BITS))  
		{
			if (LimitValues(i,(NOUN_SINGULAR|NOUN_PLURAL),(char*)"Need a noun and prior is determiner/adjective, be normal noun not infinitive",changed)) return GUESS_RETRY; // "Never have I seen this *dog."
		}
		else if (posValues[i] & NOUN_INFINITIVE && verbStack[roleIndex] &&  AcceptsInfinitiveObject(verbStack[roleIndex]))  
		{
			if (LimitValues(i,NOUN_INFINITIVE,(char*)"Need a noun and verb expects infinitives, take that",changed)) return GUESS_RETRY;
		}
		else if (posValues[i] & NOUN_SINGULAR && posValues[i-1] & (ADJECTIVE_BITS|DETERMINER|POSSESSIVE_BITS)) 
		{
			if (LimitValues(i,NOUN_SINGULAR,(char*)"Need a noun, take simple noun with extras",changed)) return GUESS_RETRY;
		}
		// dont accept singular noun lacking pre-evidence and being noun-infinitive unless mass noun... "We  had  nothing  to  do  except  *look  at  the  cinema  posters"
		else if (posValues[i] & NOUN_SINGULAR && !(posValues[i-1] & (NOUN_BITS|ADJECTIVE_BITS|DETERMINER)) && posValues[i] & NOUN_INFINITIVE && !(lcSysFlags[i] & NOUN_NODETERMINER))
		{
			if (LimitValues(i,-1 ^ NOUN_SINGULAR,(char*)"Need a noun, but simple noun not preceeeded by evidence and not mass noun, prefer noun infinitive",changed)) return GUESS_RETRY;
		}
		else if (posValues[i] & NOUN_SINGULAR && (i == endSentence || posValues[NextPos(i)] & (PREPOSITION|TO_INFINITIVE|CONJUNCTION|COMMA))) // Tashonda sent e-mail
		{
			if (LimitValues(i,NOUN_SINGULAR,(char*)"Need a noun, take simple noun when gerund not followed by interesting stuff",changed)) return GUESS_RETRY;
		}
		else if (roleIndex == MAINLEVEL &&  i <= 2 && posValues[i] & VERB_INFINITIVE && !(posValues[i-1] & (NOUN_BITS|PRONOUN_BITS))) 
		{
			needRoles[roleIndex] &= (unsigned int)( -1 ^ MAINSUBJECT);	// you is the subject
			if (LimitValues(i,-1 ^ NOUN_BITS,(char*)"Leading verb infinitive, be a command and not noun",changed)) return GUESS_RETRY;
		}
		else if (posValues[i] & ADJECTIVE_BITS && needRoles[roleIndex] & SUBJECT_COMPLEMENT) {;} // "it was *wrong"
		else 
		{
			if (LimitValues(i,(NOUN_INFINITIVE|NOUN_GERUND),(char*)"Need a noun, take potential gerund or infintive",changed)) return GUESS_RETRY;
		}
	}

	//  if something could be noun followed by potential prep and we need a noun, take it - but "our old dog Gizmo dreamed" should not
	if (posValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_PLURAL|NOUN_SINGULAR) && posValues[NextPos(i)] & PREPOSITION && needRoles[roleIndex] & (SUBJECT2|OBJECT2|MAINSUBJECT|MAINOBJECT))
	{
		if (posValues[i] & VERB_INFINITIVE &&  canSysFlags[currentVerb] & (VERB_TAKES_INDIRECT_THEN_VERBINFINITIVE | VERB_TAKES_VERBINFINITIVE)) {;}
		else if (LimitValues(i,NOUN_BITS,(char*)"something looking like noun followed by potential prep, make it a noun when we need one",changed)) return GUESS_RETRY;
	}
	// if something could be noun and next could be verb and we are hunting for both, be noun. Unless we have obvious verb coming up
	if (posValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|NOUN_PLURAL|NOUN_SINGULAR) && posValues[NextPos(i)] & ((VERB_BITS - VERB_INFINITIVE)|AUX_VERB) && needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT) && needRoles[roleIndex] & (MAINVERB|VERB2))
	{
		int at = i;
		while (++at <= endSentence && !(posValues[at] & (VERB_BITS|AUX_VERB)))
		{
			if (posValues[at] & (COMMA | PREPOSITION | CONJUNCTION_SUBORDINATE | CONJUNCTION_COORDINATE)) break;
		}
		if (!(posValues[at] & (VERB_BITS|AUX_VERB)) || bitCounts[at] != 1)
		{
			if (LimitValues(i,NOUN_BITS,(char*)"potential noun followed by potential verb/aux and we need both, take it as noun",changed)) return GUESS_RETRY;
		}
	}

	// "it flew *past me"
	if (posValues[i] & PREPOSITION && needRoles[roleIndex] & MAINOBJECT && !(needRoles[roleIndex] & SUBJECT_COMPLEMENT) && roleIndex == MAINLEVEL && !phrases[i]) // not "the exercise is the *least expensive"
	{
		unsigned int priorVerb = FindPriorVerb(i);
		if (priorVerb && canSysFlags[priorVerb] & VERB_NOOBJECT)
		{
			if (LimitValues(i,-1 ^ NOUN_BITS,(char*)"forcing preposition of noun1 since can live without object and are followed by known object2",changed)) return GUESS_RETRY;
		}
	}

	// if we want a main object and instead have ambigous non-headed noun - "I walked *home yesterday"  BUG- consider the impossiblity "I walked home the dog"
	if (posValues[i] & NOUN_SINGULAR && needRoles[roleIndex] & MAINOBJECT && !(needRoles[roleIndex] & SUBJECT_COMPLEMENT) && roleIndex == MAINLEVEL && !phrases[i]) // not "the exercise is the *least expensive"
	{
		if (LimitValues(i,NOUN_BITS,(char*)"forcing noun  since want direct object",changed)) return GUESS_RETRY;
	}

	// a potential noun that needs a determiner will abandoned "Cat is where the heart is"  but time words are ok "today is yesterday"
	// but hard to tell so dont do restriction test

	// if noun can be singular or plural, PRESUME plural, unless verb following is 3ps 
	if (posValues[i] == (NOUN_SINGULAR|NOUN_PLURAL))
	{
		if (posValues[i] & VERB_PRESENT_3PS) 
		{
			if (LimitValues(i, NOUN_SINGULAR,(char*)"is singular over plural since verb next is 3ps",changed)) return GUESS_RETRY;
		}
		else if (LimitValues(i, NOUN_PLURAL,(char*)"is plural over singular",changed)) return GUESS_RETRY; 
	}

	// if we want a noun, be one
	if (posValues[i] & (NOUN_SINGULAR|NOUN_PLURAL) && needRoles[roleIndex] & (OBJECT2 | MAINSUBJECT | MAINOBJECT))
	{
		// unless can be adjective and our verb will want complement
		if (needRoles[roleIndex] & SUBJECT_COMPLEMENT && posValues[i] & ADJECTIVE_BITS) {;} // "she was getting *upset"

		// if our priority is not as noun but as adjective or adverb, and next could be noun or adjective, be adjective and defer - "the *little old dog hit the man"
		else if (canSysFlags[i] & (ADJECTIVE|ADVERB) && !(canSysFlags[i] & NOUN))
		{
			if (posValues[NextPos(i)] & (NORMAL_NOUN_BITS | ADJECTIVE_BITS))
			{
				if (LimitValues(i, ADJECTIVE_BITS|ADVERB,(char*)"we are likely to describe a noun following still",changed)) return GUESS_RETRY;
			}
		}
		else if (LimitValues(i, NOUN_PLURAL|NOUN_SINGULAR,(char*)"be a noun because we need one",changed)) return GUESS_RETRY; 
	}

	// if we come after an adj or det (in a noun phrase) and after us MIGHT be a noun and we are NOT likely a noun, kill off nouniness
	if (posValues[i-1] & (ADJECTIVE_BITS|DETERMINER|ADVERB) && posValues[NextPos(i)] & (NORMAL_NOUN_BITS|NOUN_NUMBER) && !(canSysFlags[i] & NOUN))
	{
		if (LimitValues(i,-1 ^ (NOUN_NUMBER | NORMAL_NOUN_BITS),(char*)"we are unlikely noun, in noun phrase and next could be noun, so we wont be",changed)) return GUESS_RETRY;
	}

	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousVerb(int i, bool &changed)
{
	// possble adverb followed by comma, be adverb like "*last, he went"
	if ( i == startSentence && posValues[NextPos(i)] & COMMA  && posValues[i] & VERB_BITS && posValues[i] & ADVERB)
	{
		if (LimitValues(i, -1 ^ VERB_BITS ,(char*)"don't start a sentence with verb then comma if it can be adverb",changed)) return GUESS_RETRY; 
	}

	if (posValues[NextPos(i)] == PARTICLE)
	{
		if (LimitValues(i, VERB_BITS ,(char*)"potential verb followed by known particle will be verb",changed)) return GUESS_RETRY; 
	}
	
	// (A) if we have a noun coming up and we can be adjectives to get there, do so if noun is probable - unless gerund
	// "Doc made his students *read 4 books." - verb infinitve expected over participle being adjective

	if (needRoles[roleIndex] & (MAINSUBJECT|OBJECT2|MAINOBJECT|SUBJECT2) && posValues[i] & VERB_INFINITIVE)
	{
		if (i == startSentence && posValues[NextPos(i)] & (PRONOUN_BITS|DETERMINER_BITS|ADJECTIVE_BITS|NOUN))
		{
			 if (LimitValues(i,VERB_INFINITIVE,(char*)"Potential command at start has object after it be infintiive",changed)) return GUESS_RETRY;  // "let him go"
		}
		if (roleIndex == MAINLEVEL &&  i <= 2 && posValues[i] & VERB_INFINITIVE && !(posValues[i-1] & (NOUN_BITS|PRONOUN_BITS))) 
		{
			needRoles[roleIndex] &= (unsigned int)( -1 ^ MAINSUBJECT);	// you is the subject
			if (LimitValues(i,VERB_INFINITIVE,(char*)"Leading verb infinitive, be a command",changed)) return GUESS_RETRY;
		}
	}

	// could this be ommited subject "people"
	bool impliedPeople = false;
	int at = i;
	while (--at > startSentence && posValues[at] & ADVERB) --at;	// the wicked very often *can outwit others"
	if (at > 1 && needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2) && posValues[at] & ADJECTIVE_NORMAL && !stricmp(wordStarts[at-1],(char*)"the")) impliedPeople = true; 

	if (posValues[i] & (VERB_PAST|VERB_PRESENT) && impliedPeople)
	{
		 if (LimitValues(i,VERB_PAST|VERB_PRESENT,(char*)"adjective as subject forces verb past/present",changed)) return GUESS_RETRY;  // "the beautiful run"
	}

	// *talk about stuff
	if (posValues[i] & VERB_INFINITIVE && needRoles[roleIndex] & (MAINSUBJECT|OBJECT2|MAINOBJECT|SUBJECT2) &&
		posValues[i] & (NOUN_INFINITIVE|NOUN_GERUND))
	{
		if (i == startSentence && !(posValues[i] & PREPOSITION)) 
		{
			// prove there is no other verb in sight
			int at;
			for (at = i+1; at < endSentence; ++at)
			{
				if (posValues[at] == CONJUNCTION_COORDINATE || posValues[at] == CONJUNCTION_SUBORDINATE) at = endSentence; // abort
				else if (posValues[at] & (AUX_VERB|VERB_BITS)) break; // potential verb, maybe we are noun
			}
			if (at >= endSentence) 
			{
				if (LimitValues(i,VERB_INFINITIVE,(char*)"Potential command at start has object after it",changed)) return GUESS_RETRY; // "let him go"
			}
		}
	}
	
	// we dont need a verb, we are ending a phrase coming up on a comma, be another noun
	if (posValues[i] & NOUN_BITS && posValues[i] & VERB_BITS && currentZone < zoneIndex && zoneData[currentZone+1] & ZONE_FULLVERB &&// we expect verb in NEXT zone, so dont need it here -- "My brother's car, a sporty red convertible with bucket *seats, is the envy of my friends."
		phrases[i-1] && !phrases[i] && posValues[NextPos(i)] & COMMA)
	{
		if (LimitValues(i,-1 ^ VERB_BITS,(char*)"wrapping up a prep object before a comma, not needing verb, be a noun",changed)) return GUESS_RETRY; // but would not match: (char*)"The dog near the man eats, to the best of my knowledge, but doesn't drink"
	}

	// if we are possible noun and have a pending adj or det, we must be not verb "a big *nose is what he has"
	if (posValues[i] & NORMAL_NOUN_BITS)
	{
		unsigned int deepBefore = i;
		while (--deepBefore)
		{
			if (ignoreWord[deepBefore]) continue;
			if (posValues[deepBefore] & (ADJECTIVE_BITS| DETERMINER) && bitCounts[deepBefore] == 1) break; // we have solid value
			if (posValues[deepBefore] & (ADJECTIVE_BITS|ADVERB)) continue;
			break; // will be no good
		}
		if (posValues[deepBefore] & (ADJECTIVE_BITS| DETERMINER) && bitCounts[deepBefore] == 1 && needRoles[roleIndex] & (MAINVERB|VERB2))
		{
			if (LimitValues(i, VERB_BITS|AUX_VERB,(char*)"pending noun status blocks possible verb or aux we need",changed)) return GUESS_RETRY;
		}
	}
	
	// starting up, if need subject and verb and after us is noun evidence and we could be verb, go with verb -- "here *comes the band"
	if (posValues[i] & NORMAL_NOUN_BITS && posValues[i] & (VERB_PRESENT_3PS|VERB_PRESENT) && needRoles[roleIndex] & MAINSUBJECT && needRoles[roleIndex] & MAINVERB && bitCounts[NextPos(i)] == 1 && posValues[NextPos(i)] & (PRONOUN_BITS|ADJECTIVE_BITS|DETERMINER|NOUN_BITS|PREDETERMINER))
	{
		int noun = i;
		while (++noun <= endSentence && (!(posValues[noun] & (PRONOUN_BITS|NORMAL_NOUN_BITS))|| ignoreWord[noun])){;}
		if (posValues[noun] & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR))
		{
			if (LimitValues(i, VERB_PRESENT_3PS,(char*)"3ps verb needed and shows up before clear singular noun",changed)) return GUESS_RETRY;
		}
		else if (posValues[noun] & (NOUN_PLURAL|NOUN_PROPER_PLURAL))
		{
			if (LimitValues(i, VERB_PRESENT,(char*)"present verb needed and shows up before clear singular noun",changed)) return GUESS_RETRY;
		}
		else 
		{
			if (LimitValues(i, VERB_PRESENT|VERB_PRESENT_3PS,(char*)"present verb needed and shows up before some kind of pronoun",changed)) return GUESS_RETRY;
		}
	}

	//starting up, if we need subject and verb and after us is noun evidence, we might be flippsed order with qword "what a big *nose he has" 
	if (posValues[i] & NORMAL_NOUN_BITS && posValues[NextPos(i)] & (PRONOUN_BITS|NORMAL_NOUN_BITS) && originalLower[startSentence] && originalLower[startSentence]->properties & QWORD)
	{
		if (LimitValues(i, -1 ^ VERB_BITS,(char*)"qword inverted object before subject, reject verb meaning",changed)) return GUESS_RETRY;
	}

	// imperative infinitive when not followed by some bad things - "Go home" but not "just *like earthworms, I eat"
	if (i == startSentence && posValues[i] & VERB_INFINITIVE && !(posValues[i] & (AUX_VERB|NOUN_BITS|PRONOUN_BITS)) &&
		!(posValues[i] & (PRONOUN_BITS | CONJUNCTION_SUBORDINATE  | PREPOSITION  | ADJECTIVE_BITS | DETERMINER_BITS))) //  cant allow "have you seen"  Do you want, etc.
	{
		// but see if subject verb available next zone 
		if (currentZone == 0 &&  (zoneData[1] & (ZONE_SUBJECT|ZONE_FULLVERB)) == (ZONE_SUBJECT|ZONE_FULLVERB)) {;}
		else if (LimitValues(i, -1 ^ VERB_INFINITIVE,(char*)"command infinitive",changed)) return GUESS_RETRY; 
	}
	// imperative infinitive when not followed by some bad things - "*Boldly go home" but not "just *like earthworms, I eat" 
	if (i == (startSentence+1) && posValues[i] & VERB_INFINITIVE && !(posValues[i] & (AUX_VERB|NOUN_BITS|PRONOUN_BITS)) && posValues[i-1] & ADVERB &&
		!(posValues[i] & (PRONOUN_BITS | CONJUNCTION_SUBORDINATE  | PREPOSITION  | ADJECTIVE_BITS | DETERMINER_BITS))) //  cant allow "have you seen"  Do you want, etc.
	{
		// but see if subject verb available next zone 
		if (currentZone == 0 &&  (zoneData[1] & (ZONE_SUBJECT|ZONE_FULLVERB)) == (ZONE_SUBJECT|ZONE_FULLVERB)) {;}
		else if (LimitValues(i, -1 ^ VERB_INFINITIVE,(char*)"command infinitive",changed)) return GUESS_RETRY; 
	}

	// inverted order based on start as here,there,and nowhere - "nowhere *is that more evident." or based on negative starts- only once seldom "only once have I seen it"
	if (parseFlags[startSentence] & (LOCATIONAL_INVERTER|NEGATIVE_SV_INVERTER)  && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] & MAINSUBJECT && posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS|VERB_PAST))
	{
		if (LimitValues(i, VERB_PRESENT|VERB_PRESENT_3PS|VERB_PAST,(char*)"inverted subject after negative or locational start means this is verb",changed)) return GUESS_RETRY; 
	}

	// present-tense std verb preceeded by singular noun subject will not be such "the pill *bug is" unless  conjunction in effect
	if (posValues[i] & VERB_PRESENT && roles[i-1] & (SUBJECT2|MAINSUBJECT) && posValues[i-1] & (NOUN_SINGULAR|NOUN_NUMBER|NOUN_PROPER_SINGULAR) && !coordinates[i-1])
	{
		if (LimitValues(i, -1 ^ VERB_PRESENT,(char*)"present plural verb after singular noun subject removed",changed)) return GUESS_RETRY; 
	}

	// present-tense 3ps  verb preceeded by singular noun subject will not be such "the pill *bug is"
	if (posValues[i] & VERB_PRESENT_3PS && roles[i-1] & (SUBJECT2|MAINSUBJECT) && posValues[i-1] & (NOUN_PLURAL|NOUN_PROPER_PLURAL))
	{
		if (LimitValues(i, -1 ^ (VERB_PRESENT_3PS|VERB_INFINITIVE),(char*)"singular plural verb or infinitive after plural subject noun removed",changed)) return GUESS_RETRY; 
	}

	// present-tense 3ps  verb preceeded by multiple noun subjects part of coordination will not be such "the pill, bottle, and *bug is"
	if (posValues[i] & VERB_PRESENT_3PS && roles[i-1] & (SUBJECT2|MAINSUBJECT) && posValues[i-1] & (NOUN_SINGULAR|NOUN_PROPER_SINGULAR) && coordinates[i-1])
	{
		if (LimitValues(i, -1 ^ VERB_PRESENT_3PS,(char*)"singular  verb  after conjuncted multiple subject nouns removed",changed)) return GUESS_RETRY; 
	}

	// infinitive verb after subject ONLY if aux question
	if (posValues[i] & VERB_INFINITIVE && roles[i-1] & (SUBJECT2|MAINSUBJECT)) 
	{
		int at = i-1;
		while (--at && !(posValues[at] & AUX_VERB))
		{
			if (ignoreWord[at]) continue;
			if (posValues[at] & (CONJUNCTION|COMMA|PUNCTUATION|NOUN_BITS|PRONOUN_BITS|PREPOSITION|TO_INFINITIVE)) break;
		}
		if  (posValues[at] & AUX_VERB){;}
		else if (LimitValues(i, -1 ^ VERB_INFINITIVE,(char*)"cannot have infinitive verb after subject w/o aux",changed)) return GUESS_RETRY; 
	}

	// could also do same for pronouns, but not so vital

	// potential verb infinitive but we dont need anything and are followed by conjunction
	if (posValues[i] & VERB_INFINITIVE && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && posValues[NextPos(i)] & CONJUNCTION)
	{
		if (LimitValues(i,-1 ^ VERB_INFINITIVE,(char*)"infinitive unneeded and followed by conjunction, discard",changed)) return GUESS_RETRY;  // "let him go"
	}

	// potential verb infinitive and we want one
	if (posValues[i] & VERB_INFINITIVE && needRoles[roleIndex] & VERB_INFINITIVE_OBJECT)
	{
		if (LimitValues(i,VERB_INFINITIVE,(char*)"infinitive with causal verb infinitive, keep",changed)) return GUESS_RETRY; // "He heard the bird *sing"
	}  
	// potential verb infinitive following a noun and verb before does not take causal verb to infinitve
	if (posValues[i] & VERB_INFINITIVE && posValues[i-1] == NOUN_SINGULAR)
	{
		int at = FindPriorVerb(i);
		if (posValues[at] & VERB_BITS && !(canSysFlags[at] & VERB_TAKES_INDIRECT_THEN_TOINFINITIVE) )
		{
			if (LimitValues(i,-1 ^ VERB_INFINITIVE,(char*)"infinitive follows noun but not causal verb infinitive, discard",changed)) return GUESS_RETRY; // "He took the day *off"
		}
	}

	// command
	if (i == startSentence && posValues[i] & VERB_INFINITIVE && posValues[NextPos(i)] & (PRONOUN_BITS|DETERMINER_BITS|ADJECTIVE_BITS|NOUN_BITS))
	{
		if (LimitValues(i,VERB_INFINITIVE,(char*)"Potential command at start has object after it so use it",changed)) return GUESS_RETRY; // "let him go"
	}

	// we arent expecting anything, but a moment ago it was a phrase that could have been elevated to a clause...
	if (posValues[i] & (VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST) && roleIndex == MAINLEVEL && !(needRoles[roleIndex] & (VERB2|MAINVERB)) && phrases[i-1])
	{
		if (FlipPrepToConjunction(i,true,changed))
		{
			LimitValues(i,(VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST)  ,(char*)"verb expected for clause",changed);
			return GUESS_RESTART;
		}
	}

	// we arent expecting anything, so kill off verb meanings
	if (posValues[i] & VERB_BITS && posValues[i] & (ADVERB|ADJECTIVE_BITS|PARTICLE) && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0)
	{
		if (LimitValues(i,-1 ^ (VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST | VERB_PAST_PARTICIPLE | VERB_PRESENT_PARTICIPLE) ,(char*)"needing nothing, kill off verb meanings in favor of adverb/adj/particle except infinitive",changed)) return GUESS_RETRY;
	}
	
	// we arent expecting anything, keep verb infinitive (verbal) or particle after noun or pronoun but must have something after it
	if (posValues[i] & VERB_INFINITIVE && posValues[i-1] & (PRONOUN_BITS|NOUN_BITS) && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && i != endSentence && !(posValues[NextPos(i)] & CONJUNCTION)
		&& !(needRoles[roleIndex] & VERB_INFINITIVE_OBJECT)) // not "that will keep us *warm"
	{
		if (posValues[i] & PARTICLE) {;} 
		else if (LimitValues(i,VERB_INFINITIVE ,(char*)"needing nothing, keep potential verbal infinitive after noun/pronoun acting as postnominal adjective phrase",changed)) return GUESS_RETRY;
	}
	
	// "we watched them *play"
	if (posValues[i] & VERB_INFINITIVE && needRoles[roleIndex] & VERB_INFINITIVE_OBJECT)
	{
		if (LimitValues(i,VERB_INFINITIVE,(char*)"needing infinitive - go  infinitive",changed)) return GUESS_RETRY; 
	}

	// if something at the end of the sentence could be a verb and we dont want one.... dont be  "He can speak Spanish but he can't write it very *well."
	if (posValues[i] & VERB_BITS && i == (int)endSentence && !(needRoles[MAINLEVEL] & MAINVERB) && roleIndex == MAINLEVEL)
	{
		// but if current verb is possible aux, convert over
		unsigned int verb = verbStack[roleIndex];
		if (verb && allOriginalWordBits[verb] & AUX_VERB) // "what song do you like"
		{
			bool limited = LimitValues(verb,AUX_VERB,(char*)"potential verb at end of sentence becomes main, This main becomes aux",changed);
			limited |= LimitValues(i,VERB_BITS,(char*)"potential verb at end of sentence becomes main, main becomes aux",changed); 
			if (limited) return GUESS_RETRY;
		}
		else if (verb && canSysFlags[verb] & VERB_TAKES_VERBINFINITIVE && needRoles[roleIndex] & MAINOBJECT) {;} //"let him run"
		else if (LimitValues(i,-1 ^ VERB_BITS,(char*)"potential verb at end of sentence has no use",changed)) return GUESS_RETRY;
	}

	// if something might be verb, but AUX is later and no conjunction_subordinate exists, assume it cannot be a verb "*like what does she look"
	// BUT WE CANT TELL ABOUT CLAUSES EASILY

	// when cant tell the tense, pick past  "I *hit my friend"
	if (posValues[i] == (VERB_PRESENT|VERB_PAST))
	{
		if (LimitValues(i,VERB_PAST,(char*)"forcing PRESENT/PAST conflict on verb to past",changed)) return GUESS_RETRY;
	}

	// do we need a verb next which is in conflict with a noun
	uint64 remainder = posValues[i] & (-1 ^ (NOUN_BITS|VERB_BITS));
	if (needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & VERB_BITS && posValues[i] & NOUN_BITS && !remainder && 
	!(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)))
	{
		if (currentZone < zoneIndex && zoneData[currentZone+1] & ZONE_FULLVERB)	{;} // we expect verb in NEXT zone, so dont force it here
		else if (posValues[NextPos(i)] & (VERB_BITS|AUX_VERB) && !(posValues[NextPos(i)] & (NOUN_BITS|ADJECTIVE_BITS|DETERMINER|POSSESSIVE_BITS))) {;}
		else
		{
			if (LimitValues(i,VERB_BITS,(char*)"says verb required, binding tenses over noun",changed)) return GUESS_RETRY;
		}
	}
			
	// we need a verb- take whatever we can get - "the road following the edge of the frozen lake *died"
	if (currentZone < zoneIndex && zoneData[currentZone+1] & ZONE_FULLVERB)  {;} // we expect verb in NEXT zone, so dont force it here -- "My brother's car, a sporty red convertible with bucket *seats, is the envy of my friends."
	else if (needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & VERB_BITS &&  !(posValues[i] & AUX_VERB) && !(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)))
	{
		// if we have no aux and this cannot be absolute because 1st is already that, drop past participle
		if (currentZone != 0 && posValues[i] & VERB_PAST_PARTICIPLE && zoneData[0] & ZONE_ABSOLUTE && !auxVerbStack[roleIndex]) 
		{
			if (LimitValues(i,-1 ^ VERB_PAST_PARTICIPLE,(char*)"limiting to anything but participle",changed)) return GUESS_RETRY;
		}
		if (auxVerbStack[roleIndex] && posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_INFINITIVE|VERB_PAST_PARTICIPLE)) 
		{
			if (LimitValues(i,VERB_PRESENT_PARTICIPLE|VERB_INFINITIVE|VERB_PAST_PARTICIPLE,(char*)"limiting to participles and infinitives",changed)) return GUESS_RETRY;
		}
		else if (posValues[NextPos(i)] & (VERB_BITS|AUX_VERB) && bitCounts[NextPos(i)] == 1) // "I *even walked home"
		{
			if (LimitValues(i,-1 ^ (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS),(char*)"removingverb tense since we need a verb but next is a verb",changed)) return GUESS_RETRY;
		}
		else if (posValues[NextPos(i)] & VERB_BITS && canSysFlags[NextPos(i)] & VERB && canSysFlags[i] & ADVERB && posValues[i] & ADVERB) // "I *even make it home" - PROBABLE verb vs probable adverb
		{
			if (LimitValues(i,-1 ^ (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS),(char*)"removingverb tense since we need a verb but next is a verb",changed)) return GUESS_RETRY;
		}
		else if (posValues[i] & ( VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS )) 
		{
			if (LimitValues(i, VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS,(char*)"limiting to verb tense since we need a verb",changed)) return GUESS_RETRY;
		}
	}

	// if this is a clausal verb w/o aux, then we must be normal...
	unsigned int startOf = zoneBoundary[currentZone] + 1; 
	if (clauses[startOf] && needRoles[roleIndex] & VERB2 && !auxVerbStack[roleIndex])
	{
		if (LimitValues(i,VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS,(char*)"says normal verb required for clause w/o aux",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// "arms *folded, we lept"
	// if we are 1st zone and follow a noun with no aux and predicate is in next zone,  we are participle absolute phrase. unless we are a clause
	if (currentZone == 0 && posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && predicateZone >= 0 && zoneData[predicateZone] & ZONE_SUBJECT &&
		zoneData[predicateZone] != ZONE_AMBIGUOUS && (zoneData[currentZone] & ZONE_SUBJECT) && 
		!(zoneData[currentZone] & ZONE_AUX))
	{
		if (LimitValues(i, VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE,(char*)"says VERB_PARTICIPLE required for absolute phrase",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	
	 // can we infer present tense or infinitive when we need a verb -- "do you *attend this school"
	 if (posValues[i] == (VERB_PRESENT|VERB_INFINITIVE) && needRoles[roleIndex] & (VERB2|MAINVERB) )
	 {
		if (auxVerbStack[roleIndex] || needRoles[roleIndex] & MAINSUBJECT) // a command?
		{
			if (LimitValues(i, VERB_INFINITIVE,(char*)"is present tense verb since aux or is command",changed)) return GUESS_RETRY;
		}
		else if (LimitValues(i, VERB_PRESENT,(char*)"is present tense verb since no aux",changed)) return GUESS_RETRY;
	 }

	 // if following potential aux is potential verb, be aux.... "the sun *has set"
	 if (posValues[i] & VERB_BITS && posValues[i] & AUX_VERB && posValues[NextPos(i)] & VERB_BITS) 
	 {
		if (LimitValues(i,-1 ^ VERB_BITS,(char*)"presume aux not verb given potential verb thereafter",changed)) return GUESS_RETRY; 
	 }
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousParticle(int i, bool & changed)
{
	 int verb = phrasalVerb[i];
	while (verb > i) verb = phrasalVerb[verb]; // find potentially matching verb or none
	if (verb && !(posValues[verb] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE))) // possible verb lost its status, erase reference loop of phrasal verb
	{
		while (verb)
		{
			 int old = phrasalVerb[verb]; // ptr to 1st particle (at was the verb)
			phrasalVerb[verb] = 0;
			verb = old;
		}
	}

	if (!phrasalVerb[i]) // not detected previously
	{
		if (LimitValues(i,-1 ^ PARTICLE,(char*)"verb before potential particle cant have one, do not be one",changed)) return GUESS_RETRY;
	}

	// we can now consider be particle-- if we need an object and dont see one ahead and we were contiguous, maybe we are looped back to clause start
	if (phrasalVerb[i-1] == i) // requires sequential
	{
	}
		
	// cannot have adverb before split phrasal: "I crawled all *over" not allowed
	if (posValues[i-1] & ADVERB)
	{
		if (LimitValues(i,-1 ^ PARTICLE,(char*)"particle can only be split by object, not adverb",changed)) return GUESS_RETRY;
	}
	// if it might be particle or adverb, be particle - "the thought of getting up is bad"
	if (posValues[i] == (ADVERB|PARTICLE)) // if causal is possible
	{
		if (LimitValues(i, PARTICLE,(char*)" be particle in particle-adverb conflict",changed)) return GUESS_RETRY; 
	}
	// if prep or particle
	if (!(posValues[i] & TO_INFINITIVE)) // does verb before want a particle?
	{
		if (bitCounts[NextPos(i)] == 1 && posValues[NextPos(i)] & (NOUN_SINGULAR|NOUN_PLURAL|ADJECTIVE_BITS|PREDETERMINER|DETERMINER|PRONOUN_POSSESSIVE))
		{
			if (LimitValues(i,PARTICLE,(char*)"verb before potential particle can have one, but we have noun phrase after, so assume particle w object",changed)) return GUESS_RETRY;
		}
		else if (posValues[NextPos(i)] & NOUN_GERUND) // "Wolfson wrote a book *about playing basketball"
		{
			if (LimitValues(i,-1 ^ PARTICLE,(char*)"verb before potential particle can have one, but we have noun gerund after, so assume !particle",changed)) return GUESS_RETRY;
		}
		else 
		{
			if (LimitValues(i,PARTICLE,(char*)"verb before potential particle can have one and not to infinitive, be one",changed)) return GUESS_RETRY; // "I did not know if my boss would let me take the day *off, so I left early" but not "I would write to him"
		}
	}

	// we arent expecting anything, keep verb infinitive (verbal) or particle after noun or pronoun but must have something after it
	if (posValues[i] & VERB_INFINITIVE && posValues[i-1] & (PRONOUN_BITS|NOUN_BITS) && roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && i != endSentence && !(posValues[NextPos(i)] & CONJUNCTION))
	{
		if (posValues[i] & PARTICLE) 
		{
			if (LimitValues(i,PARTICLE ,(char*)"needing nothing, keep potential particle after noun/pronoun",changed)) return GUESS_RETRY; // "The guy who has to decide, Hamlet, decides to stick it *out for a while."
		}
	}

	// in adverb particle conflict at end of sentence, be particle "jack stopped before continuing *on."
	if (posValues[i] == (ADJECTIVE_NORMAL|PARTICLE) && i == endSentence)
	{
		if (LimitValues(i, PARTICLE,(char*)"ending adjective/particle conflict, be particle",changed)) return GUESS_RETRY; 
	}
	// if likely adjective and seeking factitive adjective for object, accept it, unless its particle "why not make it *up to him"
	if (posValues[i] & ADJECTIVE_NORMAL && needRoles[roleIndex] & OBJECT_COMPLEMENT)
	{
		if (posValues[i] & PARTICLE)
		{
			if (LimitValues(i, PARTICLE,(char*)" be particle in particle-factative conflict needing object complement",changed)) return GUESS_RETRY;
		}
	}
	
	// DONT be particle if before you is adverb "I looked all *around" - adverb should be AFTER particle "I looked up rapidly" not "I looked rapidly up"
	if (posValues[i-1] == ADVERB)
	{
		if (LimitValues(i,-1 ^ PARTICLE,(char*)"adverb immediately before means we wont be particle",changed)) return GUESS_RETRY; 
	}
	// in adverb particle conflict at end of sentence, be particle "jack stopped before continuing *on."
	if (posValues[i] == (ADVERB|PARTICLE) && i == endSentence)
	{
		if (LimitValues(i, PARTICLE,(char*)"ending adverb/particle conflict, be particle",changed)) return GUESS_RETRY; 
	}
	
	// if it might be particle or adverb, be particle - "the thought of getting up is bad"
	if (posValues[i] == (ADVERB|PARTICLE)) // if causal is possible
	{
		if (LimitValues(i, PARTICLE,(char*)" be particle in particle-adverb conflict",changed)) return GUESS_RETRY; 
	}
	// if we want a noun, maybe be particle instead
	if (posValues[i] & (NOUN_SINGULAR|NOUN_PLURAL) && needRoles[roleIndex] & (OBJECT2 | MAINSUBJECT | MAINOBJECT))
	{
		// if looking for object and this could be particle, take it unless prep phrase...  "I like getting *up and working out" but not "I rolled on my *back"
		if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && posValues[i] & PARTICLE && !(needRoles[roleIndex] & PHRASE))
		{
			if (LimitValues(i, PARTICLE,(char*)"noun vs particle as object, go for particle",changed)) return GUESS_RETRY;
		}
	}
	
	return GUESS_NOCHANGE;
}

static bool CouldBeAdjective(int i)
{
	while (++i <= endSentence)
	{
		if (ignoreWord[i]) continue;
		if (posValues[i] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) return false; // no one puts adjectives in front of proper names?
		if (posValues[i] & NORMAL_NOUN_BITS) return true;
		if (!(posValues[i] & (ADJECTIVE_BITS|ADVERB))) return false;
	} 
	return false;
}

static unsigned int GuessAmbiguousAdjective(int i, bool &changed)
{
	unsigned int currentVerb = (roleIndex == MAINLEVEL) ? currentMainVerb : currentVerb2;

	// we follow subor conjuct and end at end of sentence or with comma, omitted subject/verb clause
	int prior = i;
	while (--prior > startSentence && posValues[prior] == ADVERB) {;}
	if (posValues[prior] == CONJUNCTION_SUBORDINATE)
	{
		if (posValues[i+1] == COMMA || (i+1) == endSentence) // if dead, awaken
		{
			if (LimitValues(i, ADJECTIVE_BITS,(char*)"adjective of omitted subject/verb subord clause",changed)) return GUESS_RETRY;  // "there were many little shells"
		}
	}

	// we need subject complement, so be it
	if (needRoles[roleIndex] & SUBJECT_COMPLEMENT) 
	{
		// adjective not a good complement
		if (parseFlags[i] & NON_COMPLEMENT_ADJECTIVE) // "the balloon is *little" -
		{
			if (LimitValues(i, -1 ^ ADJECTIVE_BITS,(char*)"need subject complement, but adj is lousy, discard",changed)) return GUESS_RETRY;  // "there were many little shells"
		}
	
		// if ambiguity is Adjective_basic Adverb_basic and wanting adjective as object, be adjective if next is junk "Samson was powerful but foolish"  but not "Max looked *all_over for her" and not "the ball was *inside"
		if (posValues[i] == (ADJECTIVE_NORMAL|ADVERB)  && (posValues[NextPos(i)] & (CONJUNCTION|COMMA|PAREN) || i == endSentence))
		{
			// locational adverbs should be priority - "the ball was inside"
			if (canSysFlags[i] & LOCATIONWORD) 
			{
				if (LimitValues(i, -1 ^ ADJECTIVE_NORMAL ,(char*)"adjective that could be locational adverb at end of clause discarded",changed)) return GUESS_RETRY;
			}
			else if (canonicalLower[currentVerb] && !stricmp(canonicalLower[currentVerb]->word,(char*)"go"))
			{
				if (LimitValues(i, -1 ^ ADJECTIVE_NORMAL ,(char*)"adjective-adverb when go used  and next is junk",changed)) return GUESS_RETRY;
			} // adverbs are likely directiooal (go inside, go home)
			else if (LimitValues(i, ADJECTIVE_NORMAL ,(char*)"adjective-adverb when expecting adjective and next is junk",changed)) return GUESS_RETRY; 
		}

		// we need subject complement and before us was adverb, be it
		if (posValues[i-1] & ADVERB) // "the kitty was too *small to eat"
		{
			if (LimitValues(i, ADJECTIVE_BITS,(char*)"need subject complement, be adj preceeded by adverb",changed)) return GUESS_RETRY;  // "there were many little shells"
		}

		// adjective adverb conflict but looks good as an adjective
		if (parseFlags[i] & ADJECTIVE_GOOD_SUBJECT_COMPLEMENT) // "the balloon is *little" -
		{
			if (LimitValues(i, ADJECTIVE_BITS,(char*)"need subject complement, be adj preceeded by adverb",changed)) return GUESS_RETRY;  // "there were many little shells"
		}

		if ( !(posValues[i] & ADVERB))// not "she was *up on the bank"
		{
			if (CouldBeAdjective(i) && needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) // we will pass on being subject comp // "there were many little shells"
			{
				// if usually this is preposition and not adjective, be not adjective - "Bob was *under John"
				if (posValues[i] & PREPOSITION && canSysFlags[i] & PREPOSITION && !(canSysFlags[i] & ADJECTIVE))
				{
					if (LimitValues(i, -1 ^ ADJECTIVE_BITS,(char*)"likely prep rather than subj complement adjective with noun",changed)) return GUESS_RETRY;  // "there were many little shells"
				}
				else
				{
					needRoles[roleIndex] &= -1 ^ SUBJECT_COMPLEMENT;
					if (LimitValues(i, ADJECTIVE_BITS,(char*)"skip subject complement, be adj leading to noun",changed)) return GUESS_RETRY;  // "there were many little shells"
				}
			}
			else if (posValues[i] & PREPOSITION && canSysFlags[i] & PREPOSITION && !(canSysFlags[i] & ADJECTIVE))
			{
				if (LimitValues(i, -1 ^ ADJECTIVE_BITS,(char*)"likely prep rather than subj complement adjective with noun",changed)) return GUESS_RETRY;  // "there were many little shells"
			}
			else if (LimitValues(i, ADJECTIVE_BITS,(char*)"need subject complement, so be it",changed)) return GUESS_RETRY;  // "there were many little shells"
		}
	}

	// if this is a number and next could be adjective, lets just wait "I have *3 purple mugs"
	if (posValues[i] & NOUN_NUMBER && posValues[NextPos(i)] & (ADJECTIVE_BITS|NORMAL_NOUN_BITS)) 
	{
		if (LimitValues(i, ADJECTIVE_BITS,(char*)"need subject complement, so be it",changed)) return GUESS_RETRY;  
	}

	// after verb and before preposition, will not be adjective if not complement expected (may be particle or adverb) "josh ran *ahead of her"
	if (roles[i-1] & (VERB2|MAINVERB) && !(needRoles[roleIndex] & (SUBJECT_COMPLEMENT|OBJECT_COMPLEMENT)) && !(posValues[NextPos(i)] & (DETERMINER_BITS|ADJECTIVE_BITS|NOUN_BITS)))
	{
		if (LimitValues(i, -1 ^ ADJECTIVE_BITS,(char*)"unwanted adj after verb, discard ",changed)) return GUESS_RETRY; 
	}

	// if we are expecting an object and this could be ADJECTIVE OR DETERMINER and nothing came before and what follows is signs of noun, be it - but not prep as in "they are under the pots"
	if (!(posValues[i-1] & (DETERMINER_BITS|ADJECTIVE_BITS|POSSESSIVE_BITS)) && posValues[i] & DETERMINER_BITS  && needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && posValues[NextPos(i)] & (ADJECTIVE_BITS | NORMAL_NOUN_BITS)) // "I won *first prize"
	{
		if (LimitValues(i, -1 ^ ADJECTIVE_BITS,(char*)"determiner/adjective clash when wanting object and after seems adj or noun, be not adjective if we would head the noun",changed)) return GUESS_RETRY; 
	}
	
	// if something is a noun or adjective, and before it was noun starter, and after is not noun but might be adjective or prep and is likely prep, assume we are noun "The chief surgeon, an *expert in organ transplant procedures, took her nephew on a hospital tour."
	if (posValues[i] & ADJECTIVE_NORMAL && posValues[i-1] & (ADJECTIVE_BITS|DETERMINER) && !(posValues[NextPos(i)] & NORMAL_NOUN_BITS) && posValues[NextPos(i)] & PREPOSITION && canSysFlags[NextPos(i)] & PREPOSITION)
	{
		if (LimitValues(i, -1 ^ ADJECTIVE_NORMAL,(char*)"be not adj after noun phrase started when next is probably prep",changed)) return GUESS_RETRY; 
	}

	// if we are not looking for anything at level 1 (had object) but we could be a noun or verb and prior was a noun direct object, presume he should be adjectival noun "he ate the carrot *cake"
	if (roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && posValues[i-1] & NOUN_SINGULAR && roles[i-1] & MAINOBJECT && posValues[i] & NOUN_SINGULAR && posValues[i] & VERB_BITS)
	{
		if (allOriginalWordBits[i-1] & ADJECTIVE_NORMAL)
		{
			if (LimitValues(i-1,ADJECTIVE_NORMAL,(char*)"convert prior presumed noun to adjective that it could have been",changed))
			{
				if (roles[i-1]) SetRole(i-1,0);
				return GUESS_RETRY;
			}
		}
		else LimitValues(i-1,ADJECTIVE_NOUN,(char*)"convert prior presumed noun to adjective from us",changed);
		needRoles[MAINLEVEL] |= MAINOBJECT;
	}

	// if something could be noun or verb infinitive, and we need a noun, use noun - but not "while walking *home, Jack ran"
	// AND must have TO before it or be a verb not requiring it...  not "Nina raced her brother Carlos *down the street"
	if (!(posValues[i] & PREPOSITION) && posValues[i] & NOUN_INFINITIVE && needRoles[roleIndex] & (MAINOBJECT|OBJECT2) && !(posValues[i-1] & (NOUN_INFINITIVE|NOUN_GERUND)))
	{
		// unless causal infinitive- "we watched them play"
		if (posValues[i] & ADJECTIVE_PARTICIPLE && needRoles[roleIndex] & SUBJECT_COMPLEMENT) // she was getting *upset"
		{
			if (LimitValues(i,ADJECTIVE_PARTICIPLE,(char*)"need complement, take adjective particple",changed)) return GUESS_RETRY; 
		}
	}

	// correlative adverb:  neither wind nor 
	if (parseFlags[i] & CORRELATIVE_ADVERB)
	{
		int at = i;
		while (++at < endSentence)
		{
			if (ignoreWord[at]) continue;
			if (!stricmp(wordStarts[at],(char*)"nor") && !stricmp(wordStarts[i],(char*)"neither")) break;
			if (!stricmp(wordStarts[at],(char*)"and") && !stricmp(wordStarts[i],(char*)"both")) break;
			if (!stricmp(wordStarts[at], (char*)"&") && !stricmp(wordStarts[i], (char*)"both")) break;
			if (!stricmp(wordStarts[at],(char*)"or") && (!stricmp(wordStarts[i],(char*)"whether") || !stricmp(wordStarts[i],(char*)"either"))) break;
			if (!stricmp(wordStarts[at],(char*)"but_also") && (!stricmp(wordStarts[i],(char*)"not_only") || !stricmp(wordStarts[i],(char*)"either"))) break;
			if (at > 1 && !stricmp(wordStarts[at],(char*)"also") && !stricmp(wordStarts[at-1],(char*)"but") && !stricmp(wordStarts[i-1],(char*)"not") && !stricmp(wordStarts[i],(char*)"only")) break;
		}
		if (at < endSentence)
		{
			LimitValues(at, CONJUNCTION_COORDINATE,(char*)"other part of correlative found",changed);
			if (LimitValues(i, -1 ^ ADJECTIVE_BITS,(char*)"correlative adverb found a mate, be not adjective",changed))  return GUESS_RETRY; 
		}
	}

	// if we could be adjective but are not expecting a complement and are followed by a pronoun, dont be adjective - having seen verb: not "how *fine he looks"   but "I got her *some more."
	if (currentMainVerb && posValues[NextPos(i)] & (PRONOUN_BITS|NOUN_PROPER_SINGULAR))
	{
//		if (LimitValues(i,-1 ^ ADJECTIVE_BITS,(char*)"possible adjective is followed by a pronoun, so no.",changed)) return GUESS_RETRY; // might not be real -
	}

	// absolute phrase at start or end of sentence?  "arms *waving, he jumped"
	if (posValues[i] & ADJECTIVE_PARTICIPLE && !(posValues[i] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS)) && (currentZone == 0 || currentZone == (zoneIndex-1)) && zoneIndex > 1 && !auxVerbStack[MAINLEVEL])
	{
		int at = i;
		while (--at > 1 && !phrases[at] && !verbals[at] && !clauses[at]  && (!(roles[at] & ( SUBJECT2 | MAINSUBJECT | OBJECT2) ) || ignoreWord[at])) {;}
		if (needRoles[MAINLEVEL] & MAINVERB) // we need a verb and this cant be it
		{
			if (LimitValues(i, ADJECTIVE_PARTICIPLE,(char*)"absolute phrase allows participles",changed)) return GUESS_RETRY;
		}
	}
	
	// if we need a verb and dont need a subject, then dont be adjective if can be a verb
	if (!(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)) && needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & VERB_BITS) // Ponto then *found her"
	{
		if (LimitValues(i,-1 ^ ADJECTIVE_BITS,(char*)"dont be adjective when can be verb and we need a verb",changed)) return GUESS_RETRY; 
	}

	// nouninfintive following to infinintive, be noun infinitive "it is time to *go"
	if (posValues[i] & (NOUN_INFINITIVE|VERB_INFINITIVE) && posValues[i-1] == TO_INFINITIVE)
	{
		if (LimitValues(i,-1 ^ ADJECTIVE_BITS,(char*)"after TO infinitive will not be adjective",changed)) return GUESS_RETRY; 
	}

	if (needRoles[roleIndex] & SUBJECT_COMPLEMENT) // "it was *wrong"
	{
		if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) {;} // but can be used for this
		else if (canSysFlags[i] & ADJECTIVE_NOT_PREDICATE) // "the bread is *up near the top"
		{
			if (LimitValues(i, -1 ^ ADJECTIVE_BITS,(char*)"adjective not usable as subject complement",changed)) return GUESS_RETRY; 
		}
		else if (LimitValues(i, ADJECTIVE_BITS,(char*)"can use subject complement, so be adjective",changed)) return GUESS_RETRY; 
	} 
		
	if (posValues[i] == AMBIGUOUS_VERBAL)
	{
		// we need a noun, go for that, unless it is followed by a noun, in which case we be adjective particile
		if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2|MAINSUBJECT|SUBJECT2))
		{
			if (posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS) && !(posValues[NextPos(i)] & (-1 ^ (ADJECTIVE_BITS|NOUN_BITS))) ) {;} 
			{
				if (LimitValues(i,ADJECTIVE_PARTICIPLE,(char*)"expect we describe following noun",changed)) return GUESS_RETRY;  // can only be a noun
			}
		}
	}
	
	// if this is adjective or noun and following is a possible noun, be adjective, unless we are sentence start and "here" "there" etc
	if (posValues[i] & ADJECTIVE_NORMAL && posValues[i] & NOUN_SINGULAR && posValues[NextPos(i)] & (NOUN_SINGULAR|NOUN_PLURAL))
	{
		if (posValues[i] & ADVERB && i == startSentence && (!stricmp(wordStarts[i],(char*)"here") || !stricmp(wordStarts[i],(char*)"there")))
		{
			if (LimitValues(i,-1 ^ ADJECTIVE_BITS ,(char*)"dont be adj when here/there starts a sentence",changed)) return GUESS_RETRY;
		}
		// not a conflict if we need indirect and this is human
		else if (canSysFlags[i] & ANIMATE_BEING && needRoles[roleIndex] & (MAININDIRECTOBJECT|INDIRECTOBJECT2)) {;}// "he had the *mechanic check the brakes"
		else
		{
			if (LimitValues(i,ADJECTIVE_NORMAL ,(char*)"adj noun conflict before potential normal noun, be adj",changed)) return GUESS_RETRY;
		}
	}
	
	// if prior is det/adj and this  is adjective or noun and not followed by more adj or noun, we must not be adjective   "His red *convertible from home was green"
	if (posValues[i-1] & (DETERMINER|ADJECTIVE_BITS) && posValues[i] & ADJECTIVE_NORMAL && !(posValues[NextPos(i)] & (NOUN_BITS|ADJECTIVE_BITS|ADVERB)))
	{
		if (LimitValues(i,-1 ^ ADJECTIVE_NORMAL ,(char*)"noun phrase started but this potential adj not followed by more noun phrase stuff, be not adjective",changed)) return GUESS_RETRY;
	}

	// ending adjective makes no sense - but legal is "john painted his old jalopy *purple"
	if (posValues[i] & ADJECTIVE_NORMAL && i == endSentence && i != startSentence && roleIndex == MAINLEVEL && !(needRoles[roleIndex] & (SUBJECT_COMPLEMENT|OBJECT_COMPLEMENT)) && !(posValues[i-1] & PRONOUN_BITS)) // "there is nothing *good."
	{
		if (LimitValues(i, -1 ^ ADJECTIVE_NORMAL ,(char*)"not ending sentence in adjective since not subject complement or after pronoun",changed)) return GUESS_RETRY;
	}
	
	// if ambiguity is adjectvie, immediately after indefinitie pronoun, be adjective 
	if (posValues[i] & ADJECTIVE_NORMAL && posValues[i-1] & PRONOUN_BITS && parseFlags[i-1] & INDEFINITE_PRONOUN)
	{
		if (LimitValues(i, ADJECTIVE_NORMAL ,(char*)"probable postnominal adjective after main direct object",changed)) return GUESS_RETRY; 
	}
	
	// if ambiguity is adjectvie, delayed after indefinitie pronoun, be adjective 
	if (posValues[i] & ADJECTIVE_NORMAL && (i > 2) && posValues[i-2] & PRONOUN_BITS && parseFlags[i-2] & INDEFINITE_PRONOUN && posValues[i-1] & ADVERB)
	{
		if (LimitValues(i, ADJECTIVE_NORMAL ,(char*)"probable postnominal adjective after main direct object",changed)) return GUESS_RETRY; 
	}

	// if ambiguity is adjectvie/adverb, immediately after object/mainobject, go with adjective (assume its postnominal adjective even if verb didnt require it)
	if (posValues[i] == (ADJECTIVE_NORMAL|ADVERB) && roles[i-1] & MAINOBJECT  && !(canSysFlags[i] & ADJECTIVE_NOT_PREDICATE))
	{
		if (LimitValues(i, ADJECTIVE_NORMAL ,(char*)"probable postnominal adjective after main direct object",changed)) return GUESS_RETRY; 
	}

	// if likely adjective and seeking factitive adjective for object, accept it, unless its particle "why not make it *up to him" or unless determiner before noun or adjective
	if (posValues[i] & ADJECTIVE_NORMAL && needRoles[roleIndex] & OBJECT_COMPLEMENT && !(posValues[i] & PREPOSITION)) // not "tom has his hat *in his hand"
	{
		if (canSysFlags[i] & ADJECTIVE_NOT_PREDICATE) {;}
		else if (posValues[i] & DETERMINER && posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS)) {;} // my dog found another ball
		else if (LimitValues(i, ADJECTIVE_NORMAL ,(char*)"factitive verb expects adjective, be that",changed)) return GUESS_RETRY;
	}

	// we are allowed to be postpostive adj after noun - except if preposition "see the duck *on the pond"
	if (posValues[i-1] & (NOUN_SINGULAR|NOUN_PLURAL) && canSysFlags[i] & ADJECTIVE_POSTPOSITIVE && !(posValues[i] & PREPOSITION))
	{
		if (LimitValues(i, ADJECTIVE_NORMAL ,(char*)"postpositive legal adjective after known noun",changed)) return GUESS_RETRY; 
	}
	
	// if ambiguity is Adjective_basic Adverb_basic and wanting adjective as object, be adjective if next is junk "Samson was powerful but foolish"  but not "Max looked *all_over for her" and not "the ball was *inside"
	if (posValues[i] == (ADJECTIVE_NORMAL|ADVERB) && needRoles[roleIndex] & SUBJECT_COMPLEMENT && (posValues[NextPos(i)] & (CONJUNCTION|COMMA|PAREN) || i == endSentence) &&
		!(parseFlags[i] & NON_COMPLEMENT_ADJECTIVE))
	{
		// locational adverbs should be priority - "the ball was inside"
		if (canSysFlags[i] & LOCATIONWORD) {;}
		else if (posValues[i] & ADVERB && canonicalLower[currentVerb] && !stricmp(canonicalLower[currentVerb]->word,(char*)"go")){;} // adverbs are likely directiooal (go inside, go home)
		else if (LimitValues(i, ADJECTIVE_NORMAL ,(char*)"adjective-adverb when expecting adjective and next is junk",changed)) return GUESS_RETRY; 
	}

	int beAdj = 0;
	if (posValues[NextPos(i)] & NOUN_BITS && bitCounts[NextPos(i)] == 1) beAdj = NextPos(i);
	else if (i < (endSentence - 1) && posValues[NextPos(i)] & ADJECTIVE_BITS  && bitCounts[NextPos(i)] == 1 && posValues[Next2Pos(i)] & NOUN_BITS) beAdj = Next2Pos(i);
	else if (i < (endSentence - 2) && posValues[NextPos(i)] & ADJECTIVE_BITS  && bitCounts[NextPos(i)] == 1 && posValues[Next2Pos(i)] & ADJECTIVE_BITS && posValues[Next2Pos(i)+1]) beAdj = Next2Pos(i)+1;
	if (!beAdj){;}
	else if (bitCounts[beAdj] == 1) {;} // yes
	else if (canSysFlags[beAdj] & NOUN) {;}
	else beAdj = 0;	// not a probable noun
	if (beAdj && !(posValues[i] & (PARTICLE|PREPOSITION))) // can be an adjective, have upcoming noun, and cant be particle (whose verb might take an object) "mom picked *up Eve after school"
	{
		if (posValues[i+1] & NOUN_PROPER_SINGULAR && posValues[i] & NOUN_SINGULAR) // do you know what *game Harry plays?  expect this to be noun
		{
			if (LimitValues(i,NOUN_BITS,(char*)"not likely potential adj before proper name, it can be noun assume it is",changed)) return GUESS_RETRY;
	}
		if (!(posValues[i] & NOUN_GERUND) && !(posValues[i] & PREPOSITION)) // cant be gerund or particle -- "just *like earthworms I eat
		{
			if (LimitValues(i,ADJECTIVE_BITS|DETERMINER,(char*)"potential adj nongerund preceeding nearby noun, be adjective or determiner",changed)) return GUESS_RETRY;
		}
		// if verb cannot take an object, must be adjective   // "he hates *decaying broccoli"
		else if (posValues[i] & NOUN_GERUND && !(canSysFlags[i]  & VERB_DIRECTOBJECT))
		{
			if (LimitValues(i,ADJECTIVE_BITS|DETERMINER,(char*)"potential adj not verb needing direct object preceeding nearby noun, be adjective or determiner",changed)) return GUESS_RETRY;
		}
	}
	
	// if this is LIKELY an adjective not a noun and we are seeking a noun but could find it later, be an adjective
	if (originalLower[i] && originalLower[i]->systemFlags & ADJECTIVE && !(originalLower[i]->systemFlags & NOUN) && needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2|OBJECT2|MAINOBJECT))
	{
		int at = i;
		while (++at <= endSentence)
		{ 
			if (ignoreWord[at]) continue;
			if (posValues[at] & (NOUN_BITS|PRONOUN_BITS))
			{ // "Juanita and Celso worked *hard and then rested."
				if (LimitValues(i,ADJECTIVE_BITS|ADVERB,(char*)"probable adj/adverb while needing a noun and one found later, be adj/adv",changed)) return GUESS_RETRY;
				break;
			}
		}
	}


	// if we could be adjective participle having no noun in front of us, we dont want to be gerund if we havent come to subject zone yet
	// we want to describe subject in later zone
	if (posValues[i] & ADJECTIVE_PARTICIPLE && predicateZone >= 0 && zoneData[predicateZone] & ZONE_SUBJECT &&
		(int) currentZone == (predicateZone - 1) && !(zoneData[predicateZone] & ZONE_AMBIGUOUS) && !(zoneData[currentZone] & ZONE_SUBJECT))
	{
		if (LimitValues(i, ADJECTIVE_PARTICIPLE,(char*)"says ADJECTIVE_PARTICIPLE required because subj/verb is in next zone",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
		// could we be a noun appositive?  seems unlikely
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousAdverb(int i, bool &changed)
{	

	// object-as-adjective "i remember him as silly"
	if (!stricmp(wordStarts[i],(char*)"as") && verbStack[MAINLEVEL] && parseFlags[verbStack[MAINLEVEL]] & OBJECT_AS_ADJECTIVE && posValues[i+1] & (ADVERB|ADJECTIVE_BITS) && roles[i-1] & (MAINOBJECT|MAININDIRECTOBJECT))
	{
		if (LimitValues(i, ADVERB,(char*)"object-as-adjective as",changed)) 
		{
			LimitValues(i+1, ADVERB|ADJECTIVE_BITS,(char*)"object-as-adjective successor",changed);
			return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	// how often can you wonder *about green rice
	if (posValues[i] & PREPOSITION  && posValues[NextPos(i)] & (ADJECTIVE_NORMAL|NORMAL_NOUN_BITS))
	{
		if (LimitValues(i, -1 ^ ADVERB,(char*)"possible prep/adverb  but adj/noun may follow, prefer prep",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// "is fatter than x but *less fat than"  following a comparison logic
	if (posValues[i-1] & CONJUNCTION_COORDINATE && posValues[i+1] & ADJECTIVE_NORMAL && bitCounts[i+1] > 1)
	{
		int at = i;
		while (--at > startSentence) 
		{
			if (canonicalLower[at] == canonicalLower[i+1]) 
			{
				LimitValues(i+1,ADJECTIVE_NORMAL,(char*)"adverb/adjective pair in comparison matches earlier",changed);
				if (LimitValues(i,ADVERB,(char*)"adverb/adjective pair in comparison matches earlier",changed)) return GUESS_RETRY;
				break; // match the adjective
			}
		}
	}

	// if this can be adverb, and next could be adjective, do that "i feel *so blue" -- but "I feel *so blue people dont have to"

	if (i != startSentence && posValues[i] & ADVERB && !stricmp(wordStarts[i], "but") && posValues[NextPos(i)] & ADJECTIVE_BITS)  // "there is but one god." dont make conjunction or prep
	{
		if (posValues[Next2Pos(i)] & NORMAL_NOUN_BITS) // need a noun after to make this modify an adjective modifying a noun- otherwise might be conjoined adjective_object
		{
			if (LimitValues(i,ADVERB,(char*)"defaulting adverb/prep/conjunction as adverb like 'but one god'",changed)) return GUESS_RETRY;
		}
	}

	if (needRoles[roleIndex] & SUBJECT_COMPLEMENT && posValues[i] & ADVERB && posValues[NextPos(i)] & ADJECTIVE_BITS) // "they  are *most active at night"
	{
		if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2) && posValues[NextPos(i)] & (PRONOUN_BITS|NOUN_BITS)){;} // "I will get her *some more" 
		else if (LimitValues(i,ADVERB,(char*)"resolving to adverb based on need of subject compleement and following potential adj ",changed)) return GUESS_RETRY; 
	}

	// if noun evidence follows it, be not an adverb unless it is sentence start 
	if (i != startSentence && posValues[i] & ADVERB && posValues[NextPos(i)] & (DETERMINER_BITS|NOUN_BITS|PRONOUN_BITS) && bitCounts[NextPos(i)] == 1 ) // "i have a paper to write *before class"
	{
		if (LimitValues(i, -1 ^ ADVERB,(char*)" prep/adv conflict followed by noun evidence, be not adverb",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// if this could be adjective or adverb and is in front of adjective, prefer adjective - "the *little old lady" but not "we were *all afraid
	if (posValues[i] & ADJECTIVE_NORMAL && posValues[i] & ADVERB && posValues[NextPos(i)] == ADJECTIVE_NORMAL)
	{
		if (needRoles[roleIndex] & SUBJECT_COMPLEMENT) // cant have 2 sub compl  in  a row
		{
			if (LimitValues(i,ADVERB,(char*)"adverb or adj conflict in front of adjective, be adverb since cant have 2 subject complements in row",changed)) return GUESS_RETRY;
		}
		else if (LimitValues(i,-1 ^ ADVERB,(char*)"adverb or adj conflict in front of adjective, be adjective",changed)) return GUESS_RETRY;
	}

	// if ambiguity is Adjective_basic Adverb_basic and wanting adjective as object, be adjective if next is junk "Samson was powerful but foolish"  but not "Max looked *all_over for her" and not "the ball was *inside"
	if (posValues[i] == (ADJECTIVE_NORMAL|ADVERB) && needRoles[roleIndex] & SUBJECT_COMPLEMENT && (posValues[NextPos(i)] & (CONJUNCTION|COMMA|PAREN) || i == endSentence) &&
		!(parseFlags[i] & NON_COMPLEMENT_ADJECTIVE))
	{
		// locational adverbs should be priority - "the ball was inside"
		if (canSysFlags[i] & LOCATIONWORD)
		{
			if (LimitValues(i, ADVERB ,(char*)"subject complement adjective-adverb where adverb is locational, prefer it",changed)) return GUESS_RETRY; 
		}
	}

	if (i == startSentence && !stricmp(wordStarts[i],(char*)"why")) // assume why is an adverb
	{
		if (LimitValues(i,ADVERB,(char*)"starting sentence with why will be adverb, not noun",changed)) return GUESS_RETRY; 
	}

	// correlative adverb:  neither wind nor 
	if (parseFlags[i] & CORRELATIVE_ADVERB)
	{
		int at = i;
		while (++at < endSentence)
		{
			if (ignoreWord[at]) continue;
			if (!stricmp(wordStarts[at],(char*)"nor") && !stricmp(wordStarts[i],(char*)"neither")) break;
			if (!stricmp(wordStarts[at],(char*)"and") && !stricmp(wordStarts[i],(char*)"both")) break;
			if (!stricmp(wordStarts[at], (char*)"&") && !stricmp(wordStarts[i], (char*)"both")) break;
			if (!stricmp(wordStarts[at],(char*)"or") && (!stricmp(wordStarts[i],(char*)"whether") || !stricmp(wordStarts[i],(char*)"either"))) break;
			if (!stricmp(wordStarts[at],(char*)"but_also") && (!stricmp(wordStarts[i],(char*)"not_only") || !stricmp(wordStarts[i],(char*)"either"))) break;
			if (at > 1 && !stricmp(wordStarts[at],(char*)"also") && !stricmp(wordStarts[at-1],(char*)"but") && !stricmp(wordStarts[i-1],(char*)"not") && !stricmp(wordStarts[i],(char*)"only")) break;
		}
		if (at < endSentence)
		{
			LimitValues(at, CONJUNCTION_COORDINATE,(char*)"other part of correlative found",changed);
			if (LimitValues(i, ADVERB,(char*)"correlative adverb found a mate",changed)) return GUESS_RETRY; 
		}
	}
	

	// place number as adverb cannot be in front of noun or adjective, "first, he wept"
	if (allOriginalWordBits[i] & NOUN_NUMBER && posValues[NextPos(i)] & (NOUN_BITS|ADJECTIVE_BITS|DETERMINER|POSSESSIVE_BITS))
	{
		if (LimitValues(i, -1 ^ ADVERB,(char*)"place number cannot be adverb in front of adjective or noun or det or possessive",changed)) return GUESS_RETRY; 
	}

	// start of sentence adverb that might be coordinating conjunction, DONT be adverb "but he was dead"
	if (posValues[i] & CONJUNCTION_COORDINATE && i == startSentence)
	{
		if (LimitValues(i, -1 ^ ADVERB,(char*)"sentence start possible conjunction or adverb, be not adverb",changed)) return GUESS_RETRY; 
	}
	

	// if adverb is probably and immediately after aux and possible ver after it, be adverb "jack can *still smell"
	if (canSysFlags[i] & ADVERB && posValues[i-1] & AUX_VERB && posValues[NextPos(i)] & VERB_BITS)
	{
		if (LimitValues(i, ADVERB,(char*)"probable adverb sandwiched after aux and before potential verb",changed)) return GUESS_RETRY; 
	}

	// if word is sandwiched after a verb and before a conjunction, be adverb
	if (posValues[i-1] & (VERB_BITS|NOUN_GERUND|NOUN_INFINITIVE) && posValues[NextPos(i)] & CONJUNCTION)
	{
		if (LimitValues(i, ADVERB,(char*)" be adverb sandwiched between verb and conjunction",changed)) return GUESS_RETRY; 
	}

	// if word is sandwiched after a verb and at sentence end -- but not if possible adjective of causal verb or postpositive adjective
	if (!(posValues[i] & (ADJECTIVE_BITS|PRONOUN_BITS)) && posValues[i-1] & (VERB_BITS|NOUN_GERUND|NOUN_INFINITIVE) && i == endSentence)
	{ // but not "I earned *more"
		if (LimitValues(i, ADVERB,(char*)" be adverb after verb at end",changed)) return GUESS_RETRY; 
	}

	// if a timeword could be an adverb, make it so (if we wanted a noun by now it would be one)  "right *now we will go" but not "it flew *past me"
	if (posValues[i] & ADVERB && originalLower[i] && (originalLower[i]->systemFlags & TIMEWORD) && !(posValues[i] & PREPOSITION))
	{
		if (LimitValues(i, ADVERB,(char*)" time word could be adverb, make it so",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	// if word after could be a timeword and we could be an adverb, make it so  "*right now we will go" but not "but now"
	if (posValues[i] & ADVERB && !(posValues[i] & CONJUNCTION) && posValues[NextPos(i)] & ADVERB && originalLower[NextPos(i)] && canSysFlags[NextPos(i)] & TIMEWORD )
	{
		if (LimitValues(i, ADVERB,(char*)" next could be time word and we could be adverb, make it so",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousPreposition(int i, bool &changed) // and TO_INFINITIVE
{
	// cant be prep in one already "of the *next people"
	if (needRoles[roleIndex] & PHRASE)
	{
		if (LimitValues(i, -1 ^ PREPOSITION,(char*)"cannot be prep inside of prep",changed))return GUESS_RETRY; // 
	}

	// sentence starting with possible prep or adjective will be prep if noun evidence follows "under measures approved, we will go"
	if (i == startSentence && posValues[i+1] & (DETERMINER|NOUN_BITS|ADJECTIVE_BITS))
	{
		if (LimitValues(i, PREPOSITION|CONJUNCTION_COORDINATE|CONJUNCTION_SUBORDINATE ,(char*)"sentence start followed by noun evidence will be prep or conjunct, not adj or adv",changed))return GUESS_RETRY; // 
	}

	// possible prep followed by determiner or adjective or noun will be prep, not adverb
	if (posValues[i] & ADVERB && posValues[i + 1] & (DETERMINER|NOUN_NUMBER|TO_INFINITIVE|ADJECTIVE_NUMBER|CURRENCY|ADJECTIVE_BITS|PRONOUN_BITS|NOUN_BITS) && bitCounts[i+1] == 1) // While many of the risks were anticipated when Minneapolis-based Cray Research first announced the spinoff in May, they didn't work out
	{
		if (LimitValues(i, -1 ^ ADVERB ,(char*)" possible prep followed by obvious object shall not be adverb",changed))return GUESS_RETRY; // 
	}

	// possible to infinitive when next is potential infintive and we need a subject or object - but this is not "it is time *to go home" because no object needed
	if (posValues[i] & TO_INFINITIVE)
	{
		if ( needRoles[roleIndex] & (OBJECT2|MAINOBJECT|SUBJECT2|MAINSUBJECT) && posValues[NextPos(i)] & NOUN_INFINITIVE && !(posValues[NextPos(i)] & NORMAL_NOUN_BITS))
		{
			if (LimitValues(i, TO_INFINITIVE,(char*)" be to infinitive if followed by potential infinitve and need an object or subject",changed))return GUESS_RETRY; // we have a verb, can supply verb now...
		}

		// possible to infinitive when next is potential infintive immediately after object or subject - "the time *to go is now" BUT not "I give love *to friends"
		if (roles[i-1] & (OBJECT2|MAINOBJECT|SUBJECT2|MAINSUBJECT) && posValues[NextPos(i)] & NOUN_INFINITIVE && !(posValues[NextPos(i)] & NORMAL_NOUN_BITS))
		{
			if (LimitValues(i, TO_INFINITIVE,(char*)" be to infinitive if following subject/object and followed by potential verb infinitive",changed))return GUESS_RETRY; // we have a verb, can supply verb now...
		}


		if (posValues[NextPos(i)] & (VERB_INFINITIVE|NOUN_INFINITIVE) && needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) // "She was made *to pay back the money."
		{
			unsigned int prior = FindPriorVerb(i);
			if (prior && !(canSysFlags[prior] & VERB_NOOBJECT)) // "I swam *to shore" - since phrases shouldnt intrude before object if one is happening
			{
				if (LimitValues(i, TO_INFINITIVE,(char*)"prep could be to infinitive before verb infinitive and we need an object, be that",changed)) return GUESS_RETRY; 
			}
		}

		// To cannot be prep if next is potential verb and not undetermined noun
		if (posValues[NextPos(i)] & (VERB_INFINITIVE|NOUN_INFINITIVE) ) // "She was made *to pay back the money."
		{
			int det;
			// UNLESS we are idiom like "I give love *to friend and foe alike"
			if (posValues[Next2Pos(i)] & CONJUNCTION_COORDINATE) {;}
			else if (!(posValues[NextPos(i)] & NOUN_BITS)) 
			{
				if (LimitValues(i, TO_INFINITIVE,(char*)"to cannot be prep because verb follows and not noun",changed)) return GUESS_RETRY; 
			}
			else if (posValues[NextPos(i)] & NOUN_SINGULAR && !IsDeterminedNoun(i+1,det))   // "it is fun *to run"
			{
				if (LimitValues(i, TO_INFINITIVE,(char*)"to cannot be prep because potential noun after is not determined",changed)) return GUESS_RETRY; 
			}
		}
	}


	if (startSentence != i && posValues[i] & PREPOSITION && !stricmp(wordStarts[i], "but") && posValues[NextPos(i)] & (ADJECTIVE_BITS|PRONOUN_BITS|DETERMINER_BITS|POSSESSIVE))  // "all went but me." 
	{
		if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT)) needRoles[roleIndex] &= -1 ^ ALL_OBJECTS;
		if (LimitValues(i,PREPOSITION,(char*)"defaulting adverb/prep/conjunction to prep like 'but me'",changed)) return GUESS_RETRY;
	}

	if (posValues[i] & ADVERB && !stricmp(wordStarts[i], "but") && posValues[NextPos(i)] & (NOUN_BITS|ADJECTIVE_BITS|PRONOUN_BITS|DETERMINER_BITS) && posValues[i-1] & (VERB_BITS))  // "there is but one god." dont make conjunction
	{
		// start could be "but one lives" so dont eliminate
		if (i != startSentence && !(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT2)) && posValues[i] & PREPOSITION) // we cant accept a noun next
		{
			if (LimitValues(i,PREPOSITION,(char*)"defaulting adverb/prep/conjunction as prep like 'but me'",changed)) return GUESS_RETRY; // might not be real -
		}
	}

	// if particle before Qword, cannot be prep "pick *up where I left off"
	if (posValues[i] & PARTICLE && posValues[NextPos(i)] & CONJUNCTION)
	{
		if (LimitValues(i, -1 ^ PREPOSITION,(char*)" particle with conjunction following, dont be prep",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// if we could be TO infinitive, be that (has more options for taking stuff after)
	if (posValues[i] & TO_INFINITIVE && posValues[NextPos(i)] & NOUN_INFINITIVE && !(posValues[NextPos(i)] & NORMAL_NOUN_BITS))
	{
		if (LimitValues(i,-1 ^ PREPOSITION ,(char*)"needing nothing, keep potential To infinitive over prep",changed)) return GUESS_RETRY;
	}

	// we arent expecting anything, keep preposition but must have something after it
	if (posValues[i] & PREPOSITION &&  roleIndex == MAINLEVEL && needRoles[MAINLEVEL] == 0 && PossibleNounAfter(i) )
	{
		if (LimitValues(i,PREPOSITION ,(char*)"needing nothing, keep potential preposition",changed)) return GUESS_RETRY;
	}

	// "it flew *past me"
	if (posValues[i] & NOUN_BITS && needRoles[roleIndex] & MAINOBJECT && !(needRoles[roleIndex] & SUBJECT_COMPLEMENT) && roleIndex == MAINLEVEL && !phrases[i]) // not "the exercise is the *least expensive"
	{
		unsigned int priorVerb = FindPriorVerb(i);
		if (priorVerb && canSysFlags[priorVerb] & VERB_NOOBJECT)
		{
			if (LimitValues(i,PREPOSITION,(char*)"forcing preposition of noun since can live without object and are followed by known object2",changed)) return GUESS_RETRY;
		}
	}
	
	// we've seen main verb but possible more verb exists - "he waited *until it was ready" - but not "It went into the flower *for some honey , and maybe it went to sleep ."

	// conjunction subrod vs prep when noun_gerund follows, be conjunction
	if (posValues[i] & CONJUNCTION_SUBORDINATE && (posValues[NextPos(i)] == NOUN_GERUND || posValues[Next2Pos(i)] == NOUN_GERUND))
	{
		if (LimitValues(i, -1 ^ PREPOSITION,(char*)" prep/conjunct conflict followed by gerund, be conjunc",changed))return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	
	// if noun evidence follows it, be not an adverb unless it is sentence start 
	if (i == startSentence && posValues[i] & ADVERB && posValues[NextPos(i)] & (DETERMINER_BITS|NOUN_BITS|PRONOUN_BITS) ) // "*next I went home"
	{
		// find NOUN and prove it not followed by verb
		int afterNoun = i;
		while (++afterNoun < endSentence)
		{
			if (ignoreWord[afterNoun]) continue;
			if (posValues[afterNoun] & (PRONOUN_BITS|NORMAL_NOUN_BITS)) break;
		}
		if (posValues[afterNoun+1] & (VERB_BITS|AUX_VERB))
		{
			if (LimitValues(i, -1 ^ PREPOSITION,(char*)" prep/adv conflict at start followed by noun evidence, be adverb if possible verb immediate after",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	// if no noun follows, we mus give up prep
	if (!NounFollows(i,false) && !(posValues[i+1] & CONJUNCTION_SUBORDINATE))
	{
		if (LimitValues(i, -1 ^ PREPOSITION,(char*)" prep has no noun or clause following",changed)) return GUESS_RETRY; 
	}

	// if we have PREPOSITION vs subord conjunct check for following subject/verb "I ate dinner *before I moved"
	if (posValues[i] & CONJUNCTION_SUBORDINATE)
	{
		int at = i;
		while (++at <= endSentence)
		{
			if (ignoreWord[at]) continue;
			if (posValues[at] == COMMA) break; // not expecting comma along the way
			if (posValues[at] & VERB_BITS) 
			{
				if (LimitValues(i,-1 ^ PREPOSITION,(char*)"avoid prep as possible subj/verb follows",changed))  return GUESS_RETRY; // might not be real - may have to override later
				break;
			}
		}
		if (LimitValues(i,PREPOSITION,(char*)"becoming prep as simplest choice over conjunct",changed))  return GUESS_RETRY; // might not be real - may have to override later
	}
	
	// if we have PREPOSITION vs coord conjunct 
	if (posValues[i] & PREPOSITION && posValues[i] & CONJUNCTION_COORDINATE)
	{
		if (LimitValues(i,PREPOSITION,(char*)"becoming prep as simplest choice over conjunct",changed)) return GUESS_RETRY; // might not be real - may have to override later
	}

	// end is possible prep and start is object
	if (i == endSentence &&  posValues[i] & PREPOSITION && firstnoun && roles[firstnoun] & OBJECT2)
	{
		if (LimitValues(i,PREPOSITION,(char*)"ending possible prep and have object at start, use prep",changed)) return GUESS_RETRY; // might not be real
	}

	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousDeterminer(int i, bool & changed) // and predeterminer
{
	// if we are expecting an object and this could be ADJECTIVE OR DETERMINER and follows is signs of noun, be it - but not prep as in "they are under the pots"
	if (posValues[i-1] & DETERMINER && posValues[i] & ADJECTIVE_BITS) // "I won *first prize"
	{
		if (LimitValues(i, -1 ^ DETERMINER,(char*)"no determiner after determiner",changed)) return GUESS_RETRY; 
	}
	
	// if we are before currency or number, be determiner
	if (posValues[i+1] & (NOUN_NUMBER|ADJECTIVE_NUMBER|CURRENCY)) // "The documents also said that Cray Computer anticipates needing perhaps another $ 120 million in financing beginning next September"
	{
		if (LimitValues(i, DETERMINER,(char*)"determiner before number or currencty",changed)) return GUESS_RETRY; 
	}

	int nextNoun = i;
	while (++nextNoun <= wordCount)
	{
		if (ignoreWord[nextNoun]) continue;
		if (posValues[nextNoun] & NORMAL_NOUN_BITS) 
		{
			if (posValues[nextNoun] & VERB_BITS || !(posValues[nextNoun+1] & NORMAL_NOUN_BITS)) break; // accept multiple nouns in a row "these bank tellers" but not "*this makes Jenn mad"
		}
	}
	if (lcSysFlags[i] & DETERMINER_SINGULAR && posValues[nextNoun] & NOUN_PLURAL)
	{
		if (LimitValues(i,-1 ^ DETERMINER,(char*)"singular determiner cannot be used with subsequent plural noun ",changed)) return GUESS_RETRY; 
	}
	if (lcSysFlags[i] & DETERMINER_PLURAL && posValues[nextNoun] & NOUN_SINGULAR && !(lcSysFlags[nextNoun] & NOUN_NODETERMINER))
	{
		if (LimitValues(i,-1 ^ DETERMINER,(char*)"plural determiner cannot be used with subsequent singular noun ",changed)) return GUESS_RETRY; 
	}
	
	// possible determiner  "*what will is this"
	if (i == startSentence && posValues[i] & PRONOUN_BITS && posValues[i] & DETERMINER_BITS && posValues[NextPos(i)] & NOUN_BITS  &&  posValues[NextPos(i+1)] & (VERB_BITS | AUX_VERB) )
	{
		if (LimitValues(i,DETERMINER,(char*)"resolving pronoun/determiner to determiner since followed by potential noun and verb",changed)) return GUESS_RETRY; // might not be real
	}

	// not "I am certain *that companies are good"
	if (needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT|MAINOBJECT|OBJECT2) && !(posValues[i] & CONJUNCTION_SUBORDINATE) && posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS|DETERMINER|PRONOUN_POSSESSIVE)) // "*some people eat meat"
	{
		if (posValues[NextPos(i)] & (AUX_VERB|VERB_BITS) && needRoles[roleIndex] & (MAINVERB|VERB2)) {;} // not "*what will my dog do?"
		else if (LimitValues(i,DETERMINER|PREDETERMINER,(char*)"resolving to determiner based on need of subject or object and following potential adj or noun",changed)) return GUESS_RETRY; 
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousPronoun(int i, bool &changed)
{
	// if we start sentence and what follows can only be pronoun or adjective, be not pronoun us "*each one told"
	if (i == startSentence && posValues[NextPos(i)] & PRONOUN_SUBJECT && posValues[i] & DETERMINER)
	{
		if (LimitValues(i, -1 ^ PRONOUN_BITS ,(char*)"sentence start will prefer determiner if following could be subject pronoun",changed)) return GUESS_RETRY; 
	}

	// needing subject be pronoun subject
	if (needRoles[roleIndex] & (SUBJECT2|MAINSUBJECT) && posValues[i] & PRONOUN_SUBJECT )
	{
		if (posValues[i] & DETERMINER && posValues[NextPos(i)] & (NORMAL_NOUN_BITS|ADJECTIVE_NORMAL)) 
		{
			if (LimitValues(i,  DETERMINER ,(char*)"be determiner before a normal noun or adjective",changed)) return GUESS_RETRY; 
		}
		else if (posValues[i] & CONJUNCTION_SUBORDINATE && posValues[NextPos(i)] & (DETERMINER_BITS|NORMAL_NOUN_BITS|PRONOUN_BITS)) // we wont be subject of clause
		{
			if (LimitValues(i,  CONJUNCTION_SUBORDINATE ,(char*)"seeking subject of clause assume we are subordinate conjunction",changed)) return GUESS_RETRY; 
		}
		else if (LimitValues(i, PRONOUN_SUBJECT ,(char*)"seeking subject assume we are  pronoun subject",changed)) return GUESS_RETRY; 
	}

	// if we want indirect object, be that if you can "I gave her the ball"
	if (needRoles[roleIndex] & (INDIRECTOBJECT2|MAININDIRECTOBJECT) && posValues[i] & PRONOUN_OBJECT )
	{
		if (posValues[i] & CONJUNCTION_SUBORDINATE){;} // we determined *that he was bad"
		// we can be an indirect object, be so and fix later
		else if (lcSysFlags[i] & PRONOUN_INDIRECTOBJECT)
		{
			if (LimitValues(i, PRONOUN_OBJECT ,(char*)"seeking indirect object, be pronoun object",changed)) return GUESS_RETRY; 
		}
		else if (posValues[i] & DETERMINER_BITS)
		{
			if (LimitValues(i, DETERMINER_BITS ,(char*)"seeking indirect object which we cant be, be determiner",changed)) return GUESS_RETRY; 
		}
		else // we cannot be indirect. maybe we are direct. But not if we can be a number and after us is adjective or noun
		{
			if (posValues[i] & (NOUN_NUMBER|ADJECTIVE_NUMBER) && posValues[i+1] & (NORMAL_NOUN_BITS|ADJECTIVE_BITS))
			{
				if (LimitValues(i, NOUN_NUMBER|ADJECTIVE_NUMBER ,(char*)"seeking indirect object, assume none and think we are number describing noun later",changed)) return GUESS_RETRY; 
			}
			else if (LimitValues(i, PRONOUN_OBJECT ,(char*)"seeking indirect object, assume none and we are pronoun object",changed)) return GUESS_RETRY; 
		}
	}

	// if we want object and this might be determiner or pronoun, check forward (go for longest) like "I gave her *some more money+", but "I gave her *some+ more" 
	if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && posValues[i] & DETERMINER_BITS && !(posValues[i] & CONJUNCTION_SUBORDINATE))
	{
		if (posValues[NextPos(i)] & NORMAL_NOUN_BITS)
		{
			if (LimitValues(i, DETERMINER_BITS ,(char*)"seeking object, be determiner bit not pronoun when in front of noun possible",changed)) return GUESS_RETRY; 
		}
		if (posValues[NextPos(i)] & ADJECTIVE_BITS && posValues[Next2Pos(i)] & NORMAL_NOUN_BITS)
		{
			if (LimitValues(i, DETERMINER_BITS ,(char*)"seeking object, be determiner bit not pronoun when in front of possible adjective possible noun",changed)) return GUESS_RETRY; 
		}
	}

	// noun/pronoun conflict, if determiner then is not pronoun "I gave her this *one"
	if (posValues[i] & NOUN_BITS && posValues[i-1] & (DETERMINER|ADJECTIVE_BITS))
	{
		if (LimitValues(i, -1 ^ PRONOUN_BITS ,(char*)"pronoun-noun conflict with determiner or adjective, be not pronoun",changed)) return GUESS_RETRY; 
	}

	// if possible pronoun/adv and followed by pronoun, dont be pronoun "it was *where he would sleep"
	if (posValues[i] & ADVERB && posValues[NextPos(i)] & (PRONOUN_BITS) && !(posValues[i] & CONJUNCTION_SUBORDINATE))
	{
		if (LimitValues(i, -1 ^ (PRONOUN_BITS) ,(char*)"prefer adverb over pronoun when pronoun follow",changed)) return GUESS_RETRY; 
	}
	//possible pronoun/adv followed by noun, dont be pronoun "it was *where John would sleep"
	if (posValues[i] & (PRONOUN_BITS) && bitCounts[NextPos(i)] == 1 && posValues[NextPos(i)] & NORMAL_NOUN_BITS)
	{
		if (LimitValues(i, -1 ^ (PRONOUN_BITS) ,(char*)"prefer adverb over pronoun when oun follows",changed)) return GUESS_RETRY; 
	}

	// if possessive vs pronoun object, and followed by absolute or possible prep followed by prep object "we take them to *her in my hat"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[NextPos(i)] & PREPOSITION && posValues[i] & PRONOUN_BITS)
	{
		if (posValues[Next2Pos(i)] & (POSSESSIVE_BITS | DETERMINER | ADJECTIVE_BITS) && bitCounts[Next2Pos(i)] == 1) 
		{
			if ( LimitValues(i,PRONOUN_BITS,(char*)"resolving pronoun possessive vs object as possessive based on following noun or adjective",changed)) return GUESS_RETRY; // might not be real
		}
	}

	// if possessive vs pronoun object, and followed by an adj or noun, be possessive  -- "his new car"
	if (posValues[i] & PRONOUN_POSSESSIVE && NounSeriesFollowedByVerb(i)) // "he hid because *his bank notes were bad"
	{
		if (LimitValues(i,PRONOUN_POSSESSIVE,(char*)"resolving pronoun possessive vs object as possessive based on following noun or d",changed)) return GUESS_RETRY; // might not be real
	}
	// if possessive vs pronoun object, and followed by an adj or noun, be possessive  -- "his new car"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[NextPos(i)] & (ADJECTIVE_BITS|NORMAL_NOUN_BITS) && bitCounts[NextPos(i)] == 1) // "he hid *her own car away"
	{
		if (LimitValues(i,PRONOUN_POSSESSIVE,(char*)"resolving pronoun possessive vs object as possessive based on following noun or adjective",changed)) return GUESS_RETRY; // might not be real
	}

		// possissive vs object followed by clear verbal will be object "we heard *her sing" - but not "Bob pressed *his nose"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[i] & PRONOUN_OBJECT && posValues[NextPos(i)] & (NOUN_INFINITIVE|NOUN_GERUND) && !(posValues[NextPos(i)] & NOUN_SINGULAR))
	{
		if (LimitValues(i,PRONOUN_OBJECT,(char*)"resolving pronoun possessive vs object as object based on following verbal",changed)) return GUESS_RETRY; // might not be real
	}
	// if possessive vs pronoun object, and followed by  noun and adjective afer that, noun will be verbal "I saw her *light the fire"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[i] & PRONOUN_OBJECT && posValues[NextPos(i)] & NOUN_SINGULAR  && posValues[NextPos(i)] & NOUN_INFINITIVE &&
		bitCounts[Next2Pos(i)] == 1 && posValues[Next2Pos(i)] & (DETERMINER|ADJECTIVE_BITS|NOUN_BITS|PRONOUN_BITS))
	{
		if (LimitValues(i,PRONOUN_OBJECT,(char*)"resolving pronoun possessive vs object as object based on following noun infinitive potential with clear object after it",changed)) return GUESS_RETRY; // might not be real
	}
	// if possessive vs pronoun object, and followed by an adj or noun clearly not verbal, be possessive  -- "his new car"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[i] & PRONOUN_OBJECT && posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS) && bitCounts[NextPos(i)] == 1 )
	{
		if (LimitValues(i,PRONOUN_POSSESSIVE,(char*)"resolving pronoun possessive vs object as possessive based on following noun",changed)) return GUESS_RETRY; // might not be real
		
	}
	// pronoun possessive his and hers will never be indirect objects, and if in front of adj or noun will be possesive "*his new car"


	// if we can be possessive pronoun and after us is determiner, adjective or probably noun, be possessive pronoun "*his great ambition is to fly"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[NextPos(i)] & DETERMINER_BITS && bitCounts[NextPos(i)] == 1)
	{
		if (LimitValues(i,-1 ^ PRONOUN_POSSESSIVE,(char*)"pronoun possesive not before determiner",changed)) return GUESS_RETRY;
	}
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[NextPos(i)] & ADJECTIVE_NORMAL && !(needRoles[roleIndex] & (MAININDIRECTOBJECT|INDIRECTOBJECT2)))
	{
		if (LimitValues(i,PRONOUN_POSSESSIVE,(char*)"pronoun possesive before simple adjective",changed)) return GUESS_RETRY;
	}
	
	// if we can be possessive pronoun and after us is probable noun "his ambition is to fly"
	if (posValues[i] & PRONOUN_POSSESSIVE && posValues[NextPos(i)] & NOUN_SINGULAR && canSysFlags[NextPos(i)] & NOUN)
	{
		if (LimitValues(i,PRONOUN_POSSESSIVE,(char*)"pronoun possesive beforeprobable noun",changed)) return GUESS_RETRY;
	}

	// if something could be a pronoun subject and we are looking for a subject, be it - but not "*some people eat stuff"
	if (posValues[i] & (PRONOUN_BITS) && !(posValues[i] & (DETERMINER|ADJECTIVE_BITS)) && needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2))
	{
		if (LimitValues(i,(PRONOUN_BITS),(char*)"needing a subject, force pronoun subject to be it",changed))  return GUESS_RETRY; // even though *some are left behind, so what.
	}
	
	// possible pronoun followed by possible aux and end is prep "*where do you come from" -- but "*what will is this"
	if (i == startSentence && posValues[i] & PRONOUN_BITS && posValues[NextPos(i)] & AUX_VERB && bitCounts[NextPos(i)] == 1 && posValues[endSentence] == PREPOSITION)
	{
		if (LimitValues(i,PRONOUN_BITS,(char*)"requiring pronoun object since aux follows and prep ends sentence",changed)) return GUESS_RETRY; // might not be real
	}

	// prior prep phrase has not ended with noun and we are becoming aux.... "from *where do you come"
	if (phrases[i-1] && !(posValues[i-1] & (NOUN_BITS|PRONOUN_BITS)) && posValues[i] & PRONOUN_BITS && posValues[NextPos(i)] & AUX_VERB && bitCounts[NextPos(i)] == 1)
	{
		if (LimitValues(i,PRONOUN_BITS,(char*)"requiring pronoun object since aux follows and prep ends sentence",changed)) return GUESS_RETRY; // might not be real
	}

	// if we have pronoun subject and object, decide what we needed
	if (posValues[i] == (PRONOUN_BITS))
	{
		// if we are starting a conjunction and following us is a noun/pronoun, we must be object "he told us *what we said"
		if (roleIndex > MAINLEVEL && needRoles[roleIndex] & CLAUSE && posValues[NextPos(i)] & (PRONOUN_SUBJECT| NOUN_PLURAL|NOUN_SINGULAR|TO_INFINITIVE))
		{
			if (LimitValues(i,PRONOUN_OBJECT,(char*)" clause needs subject but seems to be after us, assume we are object",changed)) return GUESS_RETRY;
		}
		else if (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2) || roles[i] & SUBJECT2)
		{
			if (LimitValues(i,PRONOUN_SUBJECT,(char*)" says subject pronoun needed",changed)) return GUESS_RETRY;
		}
		else if (needRoles[roleIndex] & (MAINOBJECT|OBJECT2) || roles[i] & OBJECT2)
		{
			if (LimitValues(i,PRONOUN_OBJECT,(char*)" says object pronoun needed",changed)) return GUESS_RETRY;
		}
	}
	if (needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && posValues[i] & (PRONOUN_BITS)) // "they all gave Susy *some"
	{
		if (posValues[NextPos(i)] & ADJECTIVE_BITS && originalLower[i] && originalLower[i]->systemFlags & TAKES_POSTPOSITIVE_ADJECTIVE) // "some more" will never use pronoun on the some
		{
			 if (LimitValues(i, PRONOUN_BITS,(char*)"be  pronoun if next is accepted postpositive adjective",changed)) return GUESS_RETRY;
		}
		else if (posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS)){;} // next may be what we need  "we gave her *some balls"
		else if (LimitValues(i,PRONOUN_BITS,(char*)" be pronoun s/o because we need an object",changed)) return GUESS_RETRY;
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousAux(int i, bool &changed)
{
	// we arent expecting anything, but a moment ago it was a phrase that could have been elevated to a clause...
	if (roleIndex == MAINLEVEL && !(needRoles[roleIndex] & (VERB2|MAINVERB)) && phrases[i-1])
	{
		if (FlipPrepToConjunction(i,true,changed))
		{
			LimitValues(i,AUX_VERB  ,(char*)"aux verb expected for clause",changed);
			return GUESS_RESTART;
		}
	}

	 // simple infinitive after aux "I *can help"
	 if (posValues[NextPos(i)] & VERB_INFINITIVE && posValues[NextPos(i)] & (DETERMINER|ADJECTIVE_BITS|NOUN_BITS) && !(posValues[NextPos(i)] & CONJUNCTION)) // but not "*do as I say"
	 {
		if (LimitValues(i, AUX_VERB,(char*)"presume aux with infintiive after",changed)) return GUESS_RETRY; 
	 }

	 // be aux followed by possible participle later presumed aux
	 if (posValues[i] & VERB_BITS && posValues[i] & (AUX_DO|AUX_BE|AUX_HAVE)) // was he born yesterday
	 {
		 int at = i;
		 while (++at <= endSentence)
		 {
			 if (ignoreWord[at]) continue;
			 if (posValues[at] & CONJUNCTION) break; // dare not look into what might be more sentence fragments - "this *is before anyone heard of Dylan"
			 if (posValues[at] & VERB_INFINITIVE && posValues[i] & AUX_DO) // "when *do you swim"
			 {
				if (LimitValues(i, AUX_VERB,(char*)"presume aux do because infinitive follows",changed)) return GUESS_RETRY; 
			 }
			 if (posValues[at] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && !(canSysFlags[at] & COMMON_PARTICIPLE_VERB) && (posValues[i] & AUX_BE)) // when are you dying
			 {
				if (LimitValues(i, AUX_VERB,(char*)"presume aux be because participle not common adj follows",changed)) return GUESS_RETRY; 
			 }
			 if (posValues[at] & (VERB_PAST_PARTICIPLE) && posValues[i] & AUX_HAVE) // "when have you dyed your hair"
			 {
				if (LimitValues(i, AUX_VERB,(char*)"presume aux have because participle not common adj follows",changed)) return GUESS_RETRY; 
			 }
		 }
	 }

	 // she *does her best work - in verb/aux conflict, assume verb, and when we find extra verb later, revert it to aux
	 if (posValues[i] & VERB_BITS && posValues[i] & AUX_VERB && posValues[NextPos(i)] & (DETERMINER|ADJECTIVE_BITS|NOUN_BITS) && !(posValues[NextPos(i)] & CONJUNCTION)) // but not "*do as I say"
	 {
		if (LimitValues(i, -1 ^ AUX_VERB,(char*)"presume aux is real, override later if extra verb",changed)) return GUESS_RETRY; 
	 }

	if (i == (startSentence+1) && originalLower[startSentence] && originalLower[startSentence]->properties & QWORD) // assume we are aux after qword
	{
		if (LimitValues(i,AUX_VERB,(char*)"requiring aux since qword preceeds at 1st word",changed)) return GUESS_RETRY;
	}

	// we are followed by appropriate verb infinitive potential :   not "this *is mine" which requires participle
	if (!(posValues[i] & (AUX_BE|AUX_HAVE)))  // I can go
	{
		if (posValues[NextPos(i)] & VERB_INFINITIVE)
		{
			if (LimitValues(i,AUX_VERB,(char*)"normal aux followed by potential infinitive, be aux",changed)) return GUESS_RETRY;
		}
	}

	// perfect tenses - we are followed by appropriate verb past  potential :   "we *have walked" "we *had walked"
	if (!(posValues[i] & AUX_HAVE) )
	{
		if (posValues[NextPos(i)] & VERB_PAST_PARTICIPLE)
		{
			if (LimitValues(i,AUX_VERB,(char*)"have aux followed by potential past participle, be aux",changed)) return GUESS_RETRY;
		}
	}

	// continuous tenses  - we are followed by appropriate verb past  potential :   "we *are walking" "we *had walked"
	if (!(posValues[i] & AUX_BE) )
	{
		if (posValues[NextPos(i)] & VERB_PRESENT_PARTICIPLE)
		{
			if (LimitValues(i,AUX_VERB,(char*)"be aux followed by potential present participle, be aux",changed)) return GUESS_RETRY;
		}
	}

	// this is start and after might be prep/conjunction so be verb infinitive
	if (i == startSentence && posValues[i] & VERB_INFINITIVE && posValues[NextPos(i)] & (PREPOSITION | CONJUNCTION))
	{
		if (LimitValues(i,-1 ^ AUX_VERB,(char*)"looks like command with possible prep/conjuct after",changed)) return GUESS_RETRY;
	}

	// if this can be aux and later is verb only, be aux
	if (posValues[i] & AUX_VERB)
	{
		int at = i;
		int counter  = 0;
		while (++at <= endSentence)
		{
			if (ignoreWord[at]) continue;
			if (posValues[at] & VERB_BITS && !(posValues[at] & (-1 ^ VERB_BITS))) break; // some kind of insured verb
			if (posValues[at] & (NOUN_BITS |PRONOUN_BITS) && bitCounts[at] == 1) // if found 2nd noun, then could have been inverted aux 
			{
				++counter;
				if (counter > 1) break; 
				if (roleIndex == MAINLEVEL && !(needRoles[MAINLEVEL] & MAINSUBJECT)) break; // "Susie got her son" object cannot intrude between aux and verb in normal order
			}
			if (posValues[at] & TO_INFINITIVE) break; 
		}
		if (at <= endSentence && posValues[at] & VERB_BITS) 
		{
			bool possibleSubjectComplement = false;
			if (canSysFlags[i] & VERB_TAKES_ADJECTIVE) // might this be linking to adjective first
			{
				int adj = i;
				while (++adj < at) 
				{
					if (posValues[adj] & ADJECTIVE_BITS && !ignoreWord[adj]) break;
				}
				if (adj < at) possibleSubjectComplement = true;
			}

			if (!possibleSubjectComplement)
			{
				if (LimitValues(i,AUX_VERB,(char*)"requiring aux since see verb later w/o subjectcomplement needed",changed)) return GUESS_RETRY;
			}
		}
	}
	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguousConjunction(int i, bool &changed)
{
	// possible conjunctsubord followed by determiner or noun will not adverb
	if (posValues[i] & ADVERB && posValues[i + 1] & (DETERMINER|NOUN_NUMBER|ADJECTIVE_NUMBER|CURRENCY|PRONOUN_BITS|NOUN_BITS) && bitCounts[i+1] == 1) // "I know *that you lie"
	{ // but "it was not *that important" may be adverb
		if (LimitValues(i, -1 ^ ADVERB ,(char*)" possible subord conjunct followed by obvious noun shall not be adverb",changed))return GUESS_RETRY; // 
	}

	// possible conjunction followed by comma phrase: ": Proponents of the funding arrangement predict *that , based on recent filing levels of more than 2,000 a year , the fees will yield at least $ 40 million this fiscal year , or $ 10 million more than the budget cuts ."
	if (i > (startSentence+5) && posValues[i+1] & COMMA && posValues[i-1] != COMMA) // if government or private watchdogs insist, *however, on doing stuff, we will be stuck"
	{ // "too often *now, a single court ruling becomes law"
		int at  = i + 1;
		while (++at < endSentence) if (posValues[at] & COMMA) break; // find end
		if(posValues[at] & COMMA && posValues[at+1] & (DETERMINER|NOUN_BITS|ADJECTIVE_BITS))
		{
			if (LimitValues(i,CONJUNCTION_SUBORDINATE|CONJUNCTION_COORDINATE ,(char*)" noun appearance after , insertion, be conjunct",changed))return GUESS_RETRY; // 
		}
	}

	// possible conjunctsubord after mainverb followed by adjective that has later verb will be conjunction "Mr. Rowe also noted *that political concerns also worried New England Electric . "
	if (posValues[i] & ADVERB && posValues[i + 1] & ADJECTIVE_BITS ) 
	{ 
		int at = i;
		while (++at <= endSentence)
		{
			if (posValues[at] & (VERB_BITS|AUX_VERB) && LimitValues(i, -1 ^ ADVERB ,(char*)" possible subord conjunct followed by adjective and probably verb shall not be adverb",changed))return GUESS_RETRY; // 
		}
	}

	// probable  conjunction at sentence start if sign of noun after - "*but when will we go"
	if (posValues[i] & CONJUNCTION_COORDINATE && i == startSentence)
	{
		if (LimitValues(i,CONJUNCTION_COORDINATE,(char*)"prefer coord conjunct at sentence start",changed)) return GUESS_RETRY;
	}

	// probably conjunction if follows noun and had later noun and verb possible
	if (posValues[i] & CONJUNCTION_SUBORDINATE && posValues[i-1] & NOUN_BITS)  // "I love the car *that John drives" 
	{
		int at = i;
		bool noun = false;
		bool verb = false;
		while (++at <= endSentence)
		{
			if (noun && posValues[at] & VERB_BITS) verb = true;
			if ( posValues[at] & (PRONOUN_SUBJECT|NOUN_BITS)) noun = true;
		}
		if (verb && LimitValues(i,CONJUNCTION_SUBORDINATE,(char*)"prefer coord subjunct after noun and having noun + verb possible after",changed)) return GUESS_RETRY;
	}

	int at = i;
	while (++at < endSentence && (!(posValues[at] & VERB_BITS) || ignoreWord[at])) {;}
	if (!(posValues[at] & VERB_BITS)) 
	{
		if (LimitValues(i,-1 ^ CONJUNCTION,(char*)"can't be conjunction since no possible verb later",changed)) return GUESS_RETRY; // might not be real -
	}

	// why is unusual qword because it can precede command verb and others cant
	// if qword starts sentence followed by aux or command infinitive, dont believe its conjunction - "*how can I tell" and "why stand up"
	if (i == startSentence && originalLower[startSentence]->properties & QWORD && posValues[NextPos(i)] & (AUX_VERB | VERB_INFINITIVE))
	{
		if (LimitValues(i,-1 ^ CONJUNCTION,(char*)"qword at start wont be conjunction",changed)) return GUESS_RETRY; // might not be real - if cant find verb, may need to downgrade to preoposition if subject found
	}

	at = i;
	while (++at <= endSentence)
	{
		if (ignoreWord[at]) continue;
		if (posValues[at] & ((VERB_BITS - VERB_INFINITIVE) | NOUN_GERUND|AUX_VERB)) break;
	}
	if (at > endSentence && posValues[i] & CONJUNCTION_SUBORDINATE) // no verb for subordinate conjunction
	{
		if (LimitValues(i,-1 ^ CONJUNCTION_SUBORDINATE,(char*)"potential subord conjunction has no verb/gerund after it",changed)) return GUESS_RETRY;
	}

	// at start of sentence followed by comma, default NOT conjunction
	if (i == startSentence && posValues[NextPos(i)] & COMMA)
	{
		if (LimitValues(i,-1 ^ CONJUNCTION,(char*)"defaulting not conjunction when at start and follows with comma ",changed)) return GUESS_RETRY; // might not be real - if cant find verb, may need to downgrade to preoposition if subject found
	}

	return GUESS_NOCHANGE;
}

static unsigned int GuessAmbiguous(int i, bool &changed)
{
	if (ApplyRulesToWord(i,changed))  return GUESS_RETRY;

	// the various GuessAmbiguousXXX routines may only keep or discard their choices, they can't speak for a different part of speech

	// can we INFER what things are
	unsigned int result;
	uint64 priority = (originalLower[i]) ? (originalLower[i]->systemFlags & ESSENTIAL_FLAGS) : 0;
	if (!priority) priority = canSysFlags[i]  & ESSENTIAL_FLAGS;
	uint64 bits = posValues[i];

	if (bits & PARTICLE)
	{
		bits &= -1 ^ PARTICLE;
		result = GuessAmbiguousParticle(i,changed); // clean out this
		if (result != GUESS_NOCHANGE) return result;
	}

	// prefer static priority to solve
	if (priority & ADJECTIVE && bits & ADJECTIVE_BITS) 
	{
		bits &= -1 ^ ADJECTIVE_BITS;
		result = GuessAmbiguousAdjective(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}	
	if (priority & ADVERB && bits & ADVERB) 
	{
		bits &= -1 ^ ADVERB;
		result = GuessAmbiguousAdverb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}	
	if (priority & PREPOSITION && bits & PREPOSITION) 
	{
		bits &= -1 ^ PREPOSITION;
		result = GuessAmbiguousPreposition(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}	
	if (bits & AUX_VERB) // resolve aux before nouns ((char*)"how *can you live")
	{
		bits &= -1 ^ AUX_VERB;
		result = GuessAmbiguousAux(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}

	if (priority & NOUN && bits & NOUN_BITS) 
	{
		bits &= -1 ^ NOUN_BITS;
		result = GuessAmbiguousNoun(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (priority & VERB && bits & VERB_BITS) 
	{
		bits &= -1 ^ VERB_BITS;
		result = GuessAmbiguousVerb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}

	// try things we havent tried if they are relevant
	if (bits & CONJUNCTION)
	{
		result = GuessAmbiguousConjunction(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & ADJECTIVE_BITS)
	{
		result = GuessAmbiguousAdjective(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & ADVERB)
	{
		result = GuessAmbiguousAdverb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & VERB_BITS)
	{
		result = GuessAmbiguousVerb(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & NOUN_BITS)
	{
		result = GuessAmbiguousNoun(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & (DETERMINER|PREDETERMINER))
	{
		result = GuessAmbiguousDeterminer(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & (PRONOUN_BITS|PRONOUN_POSSESSIVE))
	{
		result = GuessAmbiguousPronoun(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & AUX_VERB)
	{
		result = GuessAmbiguousAux(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	if (bits & PREPOSITION)
	{
		result = GuessAmbiguousPreposition(i,changed);
		if (result != GUESS_NOCHANGE) return result;
	}
	
	// can we infer preposition since noun follows.. if not needing noun then yes
	if (needRoles[roleIndex] & (VERB2|MAINVERB) && posValues[i] & PREPOSITION) // between subject and verb
	{
		if (posValues[NextPos(i)] & ( NOUN_BITS | DETERMINER | ADJECTIVE | PRONOUN_POSSESSIVE) && bitCounts[NextPos(i)] == 1) // we know noun is coming
		{
			if (LimitValues(i,PREPOSITION,(char*)"says preposition required to contain upcoming noun",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}
	
	// this is an ambiguous verbal, we can know we have met its verb level and hunt for its objects
	if (posValues[i] == (NOUN_INFINITIVE | VERB_INFINITIVE) && needRoles[roleIndex] & VERB2)
	{
		needRoles[roleIndex] &= -1 ^ VERB2;
		AddRole(i,VERB2);
		SeekObjects(i);

		// if level below needs a noun, this can be that.. BUT maybe it would be a verb if describing instead???
		if (needRoles[roleIndex-1] & (MAINSUBJECT | SUBJECT2 | MAINOBJECT | OBJECT_COMPLEMENT | OBJECT2))
		{
			if (LimitValues(i, NOUN_INFINITIVE,(char*)" is a noun infinitive because we want one below",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	// can we infer noun? do we need a subject or an object -- cant use potential adjective/determiner/adverb cause it is legal
	if (needRoles[roleIndex] & (MAINSUBJECT | SUBJECT2 | MAINOBJECT  | OBJECT_COMPLEMENT | OBJECT2) && posValues[i] & NOUN_BITS) // can we conclude this must be a noun?
	{
		// not if it might be adverb and we are looking for an object (meaning coming after a verb)
		if (needRoles[roleIndex] & (MAINOBJECT|OBJECT_COMPLEMENT|OBJECT2) && posValues[i] & ADVERB) // "the men ate first."
		{
		}
		else if ( !(posValues[i] & (ADVERB|ADJECTIVE_BITS|DETERMINER)))
		{
			if (LimitValues(i, NOUN_BITS,(char*)"is a noun because we want one",changed)) return GUESS_RETRY;

			if (posValues[i] & NOUN_INFINITIVE && AcceptsInfinitiveObject(currentMainVerb))
			{
				if (LimitValues(i, NOUN_INFINITIVE,(char*)"we want an object complement and expect an infinitive so be one",changed)) return GUESS_RETRY;
			}

			if (posValues[i] & NOUN_SINGULAR) 
			{
				if (LimitValues(i, NOUN_SINGULAR,(char*)"is a singularnoun because",changed)) return GUESS_RETRY;
			}
		}
		else if (posValues[i] & ADJECTIVE_BITS && !(posValues[i] & NOUN_BITS) && posValues[NextPos(i)] & NOUN_BITS) // john has *2 cats.
		{
			if (LimitValues(i, ADJECTIVE_BITS,(char*)" is an adj because we want a noun and thing after it could be and we are not",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	}

	// can we infer verb vs aux verb in clause?
	uint64 remainder = posValues[i] & (-1 ^ (AUX_VERB|VERB_BITS));
	if (needRoles[roleIndex] & VERB2 && posValues[i] & AUX_VERB && posValues[i] & VERB_BITS && !remainder) // can we decide if this is aux or verb?
	{
		// to be aux, we need to find verb shortly. Skip over adverbs, block at pronouns,nouns (except in prep phrases)
		 int x = i;
		while (++x <= endSentence)
		{
			if (ignoreWord[x]) continue;
			if (posValues[x] & (NOUN_BITS|PRONOUN_BITS)) // failed to find a verb
			{
				if (LimitValues(i, VERB_BITS,(char*)"is a verb because we found following noun/pronoun",changed)) return GUESS_ABORT;
			}
			if (posValues[x] & VERB_BITS) // found a verb. assume aux.
			{
				if (LimitValues(i, AUX_VERB,(char*)"is a aux not verb because we found verb",changed)) return GUESS_ABORT;
			}
		}
	}

	// can we infer verb past vs adj participle
	remainder = posValues[i] & (-1 ^ (ADJECTIVE_PARTICIPLE|VERB_PAST));
	if (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2) && posValues[i] & ADJECTIVE_PARTICIPLE && posValues[i] & VERB_PAST) // can we decide if this is adj part or verb past?
	{
		if (LimitValues(i, -1 ^ VERB_PAST,(char*)" is not verb past since expecting a subject, not a verb",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}
	if (!(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)) && needRoles[roleIndex] & (MAINVERB|VERB2) && posValues[i] & ADJECTIVE_PARTICIPLE && posValues[i] & VERB_PAST && !remainder) // can we decide if this is adj part or verb past?
	{
		if (LimitValues(i, VERB_PAST,(char*)" is a verb past not adj participle because we need it",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
	}

	// can we infer adjective_participle vs verb_participle?   "it was a tomb *built by man"
	 remainder = posValues[i] & (-1 ^ (ADJECTIVE_PARTICIPLE|VERB_BITS));
	 if (posValues[i] & ADJECTIVE_PARTICIPLE && posValues[i] & VERB_BITS && !remainder) // can we decide if this is aux or verb?
	 {
		 // if BEFORE us is a noun and no one is looking for a verb, we should describe him...
		 if (posValues[i-1] & NOUN_BITS && bitCounts[i-1] == 1 && !(needRoles[roleIndex] & (MAINVERB|VERB2))) 
		 {
			if (LimitValues(i, ADJECTIVE_PARTICIPLE,(char*)"is an adjective participle because we found noun before and no desire for a verb",changed)) return GUESS_RETRY; // we have a verb, can supply verb now...
		}
	 }

	 // if ambiguity is between adverb and adjective, ignore it and keep going to solve a different ambiguity
	 remainder = posValues[i] & (-1 ^ (ADJECTIVE_BITS|ADVERB));
	 if (posValues[i] & ADJECTIVE_BITS && posValues[i] & ADVERB && !remainder) // can we decide if this is aux or verb?
	 {
		 return GUESS_CONTINUE;
	 }


	WORDP D = originalLower[i];
	uint64 tie = (D) ? (D->systemFlags & ESSENTIAL_FLAGS) : 0; // tie break values
	// remove non-possible values
	if (!(posValues[i] & NOUN_BITS)) tie &= -1 ^ NOUN;
	if (!(posValues[i] & VERB_BITS)) tie &= -1 ^ VERB;
	if (!(posValues[i] & ADJECTIVE_BITS)) tie &= -1 ^ ADJECTIVE;
	if (!(posValues[i] & ADVERB)) tie &= -1 ^ ADVERB;
	if (!(posValues[i] & PREPOSITION)) tie &= -1 ^ PREPOSITION;

	// if clear stats, go with guess
	if (posValues[i] & NOUN_BITS && (tie & ESSENTIAL_FLAGS) == NOUN)
	{
		if (posValues[i] & ADJECTIVE_NUMBER && posValues[NextPos(i)] & NOUN_BITS)
		{
			if (LimitValues(i,ADJECTIVE_NUMBER,(char*)"numeric noun better as adjective before noun",changed)) return GUESS_RETRY; // "I put *one shoe on"
		}
		else if (posValues[i] & NOUN_NUMBER && posValues[i] & PRONOUN_BITS)
		{
			if (LimitValues(i,PRONOUN_BITS,(char*)"numeric noun better as pronoun",changed)) return GUESS_RETRY; // "no *one is here"
		}
		else if (LimitValues(i,NOUN_BITS,(char*)"noun is purely probably",changed)) return GUESS_RETRY;
	}
	if (posValues[i] & VERB_BITS && (tie & ESSENTIAL_FLAGS) == VERB)
	{
		if (LimitValues(i,VERB_BITS,(char*)"verb is purely probably",changed)) return GUESS_RETRY;
	}
	if (posValues[i] & PREPOSITION && (tie & ESSENTIAL_FLAGS) == PREPOSITION)
	{
		if (posValues[i] & TO_INFINITIVE && posValues[i+1] & NOUN_INFINITIVE && !(canSysFlags[i+1] & NOUN_NODETERMINER)) // I would want to be able *to use the procedure  but not "I can go to *church"
		{
			if (LimitValues(i,TO_INFINITIVE,(char*)"guessing to since word after could be infinitive",changed)) return GUESS_RETRY;
		}
		else if (LimitValues(i,PREPOSITION,(char*)"preposition is purely probably",changed)) return GUESS_RETRY;
	}
	if (posValues[i] & ADJECTIVE_BITS && (tie & ESSENTIAL_FLAGS) == ADJECTIVE)
	{
		if (posValues[i] & ADVERB)
		{
			if (LimitValues(i,posValues[i] & (ADJECTIVE_BITS|ADVERB|DETERMINER),(char*)"adjective adverb det will remain pending",changed)) return GUESS_RETRY;
		}
		else if (LimitValues(i,ADJECTIVE_BITS,(char*)"adjective is purely probably",changed)) return GUESS_RETRY;
	}
	if (posValues[i] & ADVERB && (tie & ESSENTIAL_FLAGS) == ADVERB)
	{
		if (posValues[i] & ADJECTIVE_BITS)
		{
			if (LimitValues(i,posValues[i] & (ADJECTIVE_BITS|ADVERB|DETERMINER),(char*)"adjective adverb det1 will remain pending",changed)) return GUESS_RETRY;
		}
		else if (LimitValues(i,ADVERB,(char*)"adverb is purely probably",changed)) return GUESS_RETRY;
	}

	return GUESS_CONTINUE;
}

static void AddPhrase( int i)
{
	if (!phrases[i]) 
	{
		phrases[i] |= prepBit;
		prepBit <<= 1;
	}
	AddRoleLevel(PHRASE|OBJECT2,i);
	lastPhrase = i;		// where last verbal was, if any
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"    Phrase added at %s(%d)\r\n",wordStarts[i],i);
}

static void AddVerbal( int i)
{
	if (needRoles[roleIndex] & PHRASE && needRoles[roleIndex] & OBJECT2) needRoles[roleIndex] ^= OBJECT2; // verbal acts as object
	if (!verbals[i]) 
	{
		verbals[i] |= verbalBit;
		verbalBit <<= 1;
	}
	AddRoleLevel(VERBAL|VERB2,i);
	if (posValues[i] == AMBIGUOUS_VERBAL) determineVerbal = i; // If we don't know if this is a GERUND or PARTICIPLE, we need to find out.
	lastVerbal = i;		// where last verbal was, if any
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"    Verbal added at %s(%d)\r\n",wordStarts[i],i);
}

static void StartImpliedPhrase(int i,bool &changed)
{
	if (roles[i] & OMITTED_TIME_PREP) // reestablish prior phrase we found
	{
		if (needRoles[roleIndex] & (CLAUSE|VERBAL) && roleIndex > 1) DropLevel(); 	 // was formerly a clause or verbal, ended by prep phrase  clause/verbal will never have direct object after this
		AddPhrase(i);
		return;
	}

	// absolute phrase is like implied prep phrase "with" in "legs quivering, we ran"  requires comma seaparation and no aux verbs
	if (!phrases[i] && !auxVerbStack[roleIndex] && posValues[i] & ADJECTIVE_PARTICIPLE && (currentZone == 0|| currentZone == (int)(zoneIndex-1)) && zoneIndex > 1)
	{
		int at = i;
		while (--at > 1 && !phrases[at] && !verbals[at] && !clauses[at] && (!(posValues[at] & (COMMA|CONJUNCTION_SUBORDINATE)) || ignoreWord[at]) && !(roles[at] & ( SUBJECT2 | MAINSUBJECT | OBJECT2) )) {;}
		bool abs = false;
		if (roles[at] & (OBJECT2 | APPOSITIVE | SUBJECT2) || !roles[at])  abs = true;
		else if (roles[at] & MAINSUBJECT && needRoles[MAINLEVEL] & MAINVERB  && !auxVerbStack[MAINLEVEL]) abs = true; // we need a verb and this cant be it
		if (abs) 
		{
			zoneData[currentZone] = ZONE_ABSOLUTE;
			SetRole(at,0,true);
			AddPhrase(i);
			SetRole(at,SUBJECT2);
			AddRole(at,ABSOLUTE_PHRASE);
			ExtendChunk(at,i,phrases); // cover with prep phrase (not treated as clause)
		}
		return;
	}

	bool valid = false;
	WORDP D = originalLower[i];
	WORDP E = canonicalLower[NextPos(i)];
	if (E && E->systemFlags & TIMEWORD && posValues[NextPos(i)] & NOUN_BITS && D && posValues[i] & (ADJECTIVE_BITS|DETERMINER))  // some time expressions can omit a preposition and just have an object
	{
		if (parseFlags[i+1] & TIME_NOUN && parseFlags[i+2] & TIME_ADJECTIVE){;} // not "I am six *years old"
		else if (!phrases[i] && !roles[i] && D && D->systemFlags & OMITTABLE_TIME_PREPOSITION) // after it is timeword?  // omitted prepoistion time phrase (adjective)  "next year"  - also "one day"  or   "five days ago"
		{
			if ( i == startSentence && endSentence > (startSentence+3) && *wordStarts[startSentence+2] == ',') valid = true;
			else if ( i == (endSentence - 1)) valid = true;
			else if ((endSentence - i) < 3) {;}
			else if (i == startSentence && posValues[Next2Pos(i)] & (DETERMINER|ADJECTIVE_BITS|NOUN_BITS|(PRONOUN_BITS))) valid = true; // we  see a subject potential after us...
			else if (phrases[i+2] || clauses[i+2]  || verbals[i+2] || posValues[Next2Pos(i)] == COMMA) valid = true; // we are isolated from stuff
			else valid = true; // we will meet in Australia *next week to decide
		}
		else if (posValues[i] & ADJECTIVE_NUMBER) valid = true; // "one day"
	}
	if (valid) // time phrase at start or end
	{
		LimitValues(i,ADJECTIVE_BITS|DETERMINER,(char*)"forced determiner/adj for timephrase",changed);
		LimitValues(i+1,NOUN_BITS,(char*)"Assigning noun to time word at start or end",changed);
		if (needRoles[roleIndex] & (CLAUSE|VERBAL) && roleIndex > 1) DropLevel(); 	 // was formerly a clause or verbal, ended by prep phrase  clause/verbal will never have direct object after this
		SetRole(i, OMITTED_TIME_PREP);
		AddPhrase(i);
	}
}

static void StartImpliedClause( int i,bool & changed)
{
	if (posValues[i] & PREPOSITION) return;	// at best a conflict, we prefer prep and overrule later.
	if (posValues[i] == CONJUNCTION_SUBORDINATE) return;	// at best a conflict, we prefer prep and overrule later.
	if (clauses[i]) return; // clause already started here

	// noun phrase starting a "that"-less clause?
	if (bitCounts[i] == 1 && posValues[i] & (DETERMINER_BITS|NORMAL_NOUN_BITS|ADJECTIVE_BITS|PRONOUN_BITS) && !(posValues[i] & ADJECTIVE_PARTICIPLE)) // not "i have a friend *named harry"
	{
		// we are at main level and have subject but no verb yet, "another way *a squid can fight is"
		if (roles[i-1] & (MAINSUBJECT|SUBJECT2) && needRoles[roleIndex] & (MAINVERB|VERB2))
		{
			unsigned int verbCount = 0;
			// are there 2 possible verbs ahead?
			int at = i - 1;
			while ( ++at < endSentence)
			{
				if (ignoreWord[at]) continue;
				if (posValues[at] & (NORMAL_NOUN_BITS|PRONOUN_BITS)) break; // find a subject
			}
			while (++at <= endSentence) if (posValues[at] & VERB_BITS && !ignoreWord[at]) ++verbCount;
			if (verbCount > 1) 
			{
				AddClause(i,(char*)"omitted that clause as subject appositive ");
				crossReference[i] = (unsigned char) (i-1);
			}
			return;
		}
		// could be a clause as object:  "I say *he should go home" But not "do you know what *game Harry likes"
		if (roleIndex == MAINLEVEL && needRoles[MAINLEVEL] & MAINOBJECT && verbStack[MAINLEVEL] && parseFlags[verbStack[MAINLEVEL]] & OMITTABLE_THAT_VERB && !(posValues[i] & (PRONOUN_OBJECT|NORMAL_NOUN_BITS)))
		{//not "I have a friend named Harry who likes to play tennis"
			int at = i;
			while (at < endSentence)
			{
				if (posValues[at] & (PRONOUN_SUBJECT|NORMAL_NOUN_BITS))
				{ // BUG
					if (posValues[at] & (PRONOUN_SUBJECT|NORMAL_NOUN_BITS)) break; // not "do you know what *game harry likes"
					if (allOriginalWordBits[at+1] & (QWORD|CONJUNCTION_SUBORDINATE)) break;
					if (allOriginalWordBits[at+2] & (QWORD|CONJUNCTION_SUBORDINATE)) break;
					if (allOriginalWordBits[at+3] & (QWORD|CONJUNCTION_SUBORDINATE)) break;
					if (allOriginalWordBits[at+4] & (QWORD|CONJUNCTION_SUBORDINATE)) break;
					if (posValues[at+1] & (AUX_VERB_TENSES|VERB_PRESENT | VERB_PRESENT_3PS | VERB_PAST)) // have subject and verb, good enough
					{
						SetRole(at,MAINOBJECT,true);
						AddClause(at,(char*)"omitted that clause0 as object ");
						SetRole(at,SUBJECT2,true);
						return;
					}
				}
				else if (posValues[at] & (VERB_BITS | CONJUNCTION|PREPOSITION)) break;
				++at;
			}
		}
		// we are at main level and have subject   object, "I like the pictures children draw"
		// we are after closing object of a phrase, "I like the songs of pictures children draw"
		// but if we just closed a clause "though I walk through the valley I fear no evil" dont worry.
		if (roles[i-1] & (MAINOBJECT|OBJECT2|MAINSUBJECT) && roleIndex == MAINLEVEL && !clauses[i-1])
		{
			//  possible verb ahead?
			int at = i;
			while (++at <= endSentence)
			{
				if (ignoreWord[at]) continue;
				if (posValues[at] & VERB_BITS) // and we have a verb
				{
					AddClause(i,(char*)"omitted that clause1 as object appositive ");
					crossReference[i] = (unsigned char) (i-1);
				}
			}
			return;
		}
	}
	// we are at MAIN level and experienceing a verb we don't need....  hidden clause "I think it *is good"
	if (roleIndex == MAINLEVEL && !(needRoles[MAINLEVEL] & MAINVERB) && roles[i-1] & (MAINOBJECT|MAININDIRECTOBJECT) && posValues[i] & (AUX_VERB_TENSES|VERB_BITS) && bitCounts[i] == 1)
	{
		if (posValues[i] & VERB_INFINITIVE && AcceptsInfinitiveObject(currentMainVerb)) {;} // "let us *stand still"
		else
		{
			SetRole(i-1,MAINOBJECT,true);	// force prior to be main object 
			needRoles[roleIndex] = 0;	// we are fulfilled. Do not take an object complement as well.
			AddClause(i-1,(char*)"omitted that clause as main object ");
			roles[i-1] |= SUBJECT2; // assign secondary role characteristic
			if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"   +%s->%s\r\n",wordStarts[i],GetRole(roles[i]));
			return;
		}
	}
		
	// prove there is a possible verb later - no "I can do *that too"
	int at = i;
	while (++at < endSentence && (!(posValues[at] & VERB_BITS) || ignoreWord[at])) {;}
	if (!(posValues[at] & VERB_BITS)) return;	// cannot be a clause

	// find clauses  (markers start, ending with a verb)
	if (!stricmp(wordStarts[i],(char*)"however") ||  !stricmp(wordStarts[i],(char*)"whoever") || !stricmp(wordStarts[i],(char*)"whomever") ||  !stricmp(wordStarts[i],(char*)"whatever")|| !stricmp(wordStarts[i],(char*)"whenever") || !stricmp(wordStarts[i],(char*)"whichever")) {;} // will be a clause most likely
	else if (i == startSentence && originalLower[i] && originalLower[i]->properties & QWORD && posValues[startSentence+1] & (VERB_BITS|AUX_VERB))  // qwords are not clauses but main sentence-- what is my name, EXCEPT "when the"
	{
		return;
	}
		
	// a certain  clause

// SUBJECT clause starters immediately follow a noun they describe
	// OBJECT clause starters arise in any object position and may even be ommited  -This is the man (whom/that) I wanted to speak to . The library didn't have the book I wanted
	// The book whose author won a Pulitzer has become a bestseller.
	// clause starter CANNOT be used as adjective... can be pronoun or can be possessive determiner (like whose)
	// USE originalLower because "that" becomes "a" when canonical
	if (i == startSentence && posValues[i] != CONJUNCTION_SUBORDINATE) {;} // these things may all be a normal question, not a clause
	else if (posValues[i] == DETERMINER) {;} // that, used as a determiner so not a clause starter
	else if (originalLower[i] && (parseFlags[i] & POTENTIAL_CLAUSE_STARTER || posValues[i] & CONJUNCTION_SUBORDINATE)) // who whom whoever which whichever  what whatever whatsoever where wherever when whenever how however why that whether  whose?
	{
		if (posValues[NextPos(i)] & AUX_VERB && !(needRoles[roleIndex] & (MAINOBJECT|OBJECT2))) return;	// more a real sentence, UNLESS we were looking for an object "I ate what was given"
		//whoever, whatever, whichever, however, whenever and wherever are called compound relative pronouns -- acts as a subject, object or adverb in its own clause;
		if (!clauses[i])
		{
			if (tried[i]) return; // been here already, it didn't work out
			if (parseFlags[i] & CONJUNCTIONS_OF_ADVERB) {;}
			else if (!stricmp(wordStarts[i],(char*)"what") || !stricmp(wordStarts[i],(char*)"who") || !stricmp(wordStarts[i],(char*)"whom") || !stricmp(wordStarts[i],(char*)"whoever") || !stricmp(wordStarts[i],(char*)"whatever") 
				|| !stricmp(wordStarts[i],(char*)"whichever") || !stricmp(wordStarts[i],(char*)"whatsoever")) 
			{
				if (posValues[i] == CONJUNCTION_SUBORDINATE){;}
				else LimitValues(i,PRONOUN_BITS,(char*)"clause starts with WH-pronoun, forcing pronoun",changed); // WP Penntag
				if (!(posValues[i] & CONJUNCTION_COORDINATE) && needRoles[roleIndex] & (MAINOBJECT | OBJECT2)) // we will satisfy this as a clause
				{
					SetRole(i,needRoles[roleIndex] & (MAINOBJECT | OBJECT2));
				}
			}
			else if (!stricmp(wordStarts[i],(char*)"which") || !stricmp(wordStarts[i],(char*)"that") ) // wh-determiners (or simple clause starters)
			{
				if (posValues[i] == CONJUNCTION_SUBORDINATE){;} // leave it alone
				// need an object but not two and one follows us
				else if (posValues[NextPos(i)] & (PRONOUN_BITS|DETERMINER_BITS))
				{
					if (posValues[i] & CONJUNCTION_SUBORDINATE) LimitValues(i,CONJUNCTION_SUBORDINATE,(char*)"clause starts with no need of extra noun so be simple conjunction for now",changed); 
					else LimitValues(i,PRONOUN_OBJECT,(char*)"clause starts with no need of extra noun so be object",changed); 
				}
				else if (posValues[NextPos(i)] & (AUX_VERB|VERB_BITS) && bitCounts[NextPos(i)] == 1)
				{
					LimitValues(i,PRONOUN_SUBJECT,(char*)"clause starts with WH-pronoun followed by verb or aux, forcing Pronoun subject",changed); // WDT Penntag
				}
			}
			else if (!stricmp(wordStarts[i],(char*)"whose") ) // wh-possessive pronoun
			{
				LimitValues(i,PRONOUN_POSSESSIVE,(char*)"clause starts with WH-pronoun, forcing POSSESSIVE",changed); // WP$ Penntag
				if (posValues[i-1] & COMMA) AddClause(i,(char*)"clause - qword starter\r\n"); // expecting it to be appositive clause "my mom, whose life is described, is fun"
				return;		// just because possessive doesnt mean clause
			}
		}

		AddClause(i,(char*)"clause - qword starter\r\n");
		if (posValues[i] & CONJUNCTION_SUBORDINATE) LimitValues(i,CONJUNCTION_SUBORDINATE,(char*)"adverb clause forcing subord conjunct, forcing",changed); // WRB Penntag
		return;
	}
	else // implicit clause
	{
//normally not implicit: , what, which, who, whoever, whom, whomever, whose -- after, although, as, because, before, if, once, since, though, till, unless, until, , whenever, where, wherever, while 
//I must admit * Ralph was my first and only blind date. (Noun clause--direct object)
//The first blind date * I ever had was Ralph. (Adjective clause BEFORE MAIN VERB) 
// Ralph was my first and only blind date because I married him. (Adverb clause NOTHING OMITTED)
	//I'll never forget the day * I met her.   -- after a noun to describe it -- The day I met her was warm
	//He discovered * he was hungry and * the fridge was empty
	// He believes * Mary is beatiful. -- common AFTER main verb is done
	// President Jefferson believed * the headwaters of the Missouri might reach all the way to the Canadian border and * he could claim all that land for the United States.
//An elliptical clause may seem incorrect as it may be missing essential sentence elements, but it is actually accepted grammatically. As these clauses must appear together with complete clauses which contain the missing words, repetition is avoided by leaving the same words (or relative pronoun) out in the elliptical clause. This conciseness actually adds to the flow of the text and promotes writing that is more elegant.
//The Louvre museum was one of the sites (that) we did not want to miss.
//[The relative pronoun that is omitted from the adjective clause]
//After (we visited) the Louvre, we went out to dinner at a French bistro.
//[subject and verb omitted from adverb clause]
//The French make better croissants than the American (make or do).
//[second half of comparison omitted]
//Though (they) sometimes (appear) impatient and somewhat assertive, most French people are actually kind and warm-hearted.
//[subject and verb omitted from adverb clause]

		// usual PUNCTUATION is  < subclause, main sentence    OR    <  main sentence subclause (no comma)
		int prior = i;
		while (--prior && posValues[prior] & (ADJECTIVE_BITS | DETERMINER | POSSESSIVE_BITS)); // find the major unit before our noun
		// we will need 2 verbs for this to work
		if (needRoles[MAINLEVEL] & MAINVERB)
		{
			unsigned int count = 0;
			int at = i;
			while (++at <= endSentence)
			{
				if (posValues[at] & VERB_BITS && !ignoreWord[at]) ++count;
			}
			if (count < 2) return;	// cannot be clause along with main sentence
		}

		// conditionals lacking "if" starting the sentence
		if (i == startSentence && (!stricmp(wordStarts[i],(char*)"had") || !stricmp(wordStarts[i],(char*)"were") || !stricmp(wordStarts[i],(char*)"should")) && zoneIndex > 1)
		{
			AddClause(i,(char*)"omitted if");
			return;
		}

		// a noun arising unexpected following a noun is likely a clause with "that" or "when" omitted unless its an appositive
		if (prior && posValues[prior] & NOUN_BITS && posValues[i] & (NOUN_BITS | ADJECTIVE_NOUN | PRONOUN_BITS) && !(posValues[i] & (-1 ^ (NOUN_BITS | ADJECTIVE_NOUN| PRONOUN_BITS))) && !(needRoles[roleIndex] & (MAINSUBJECT|MAINOBJECT|OBJECT_COMPLEMENT|OBJECT2)) )
		{
			if (posValues[i] & NORMAL_NOUN_BITS && verbStack[roleIndex] && parseFlags[verbStack[roleIndex]] & FACTITIVE_NOUN_VERB) // can take a complement - He painted his old jalopy *purple 
			{
				LimitValues(i,NORMAL_NOUN_BITS,(char*)"Forcing 2nd noun as factitive complement",changed);
				return;
			}

			else if (IsLegalAppositive(prior,i)) return; // let it be appositive

			// but if the prior noun could have been adjectives "the little old lady"
			else if (posValues[i] & NORMAL_NOUN_BITS && allOriginalWordBits[i-1] & ADJECTIVE_BITS)
			{
				if (prior == (i-1)) 
				{
					LimitValues(i-1,allOriginalWordBits[i-1] & ADJECTIVE_BITS,(char*)"Forcing prior to adjective hitting unexpected noun",changed);
					needRoles[roleIndex] |= roles[prior];
					roles[prior] = 0;
					return;
				}
				if (prior == (i-2) && allOriginalWordBits[i-2] & ADJECTIVE_BITS)
				{
					LimitValues(i-2,allOriginalWordBits[i-2] & ADJECTIVE_BITS,(char*)"Forcing 2x prior to adjective hitting unexpected noun",changed);
					needRoles[roleIndex] |= roles[prior];
					roles[prior] = 0;
					return;
				}
			}

			if (posValues[NextPos(i)] & (AUX_VERB|VERB_BITS) && !roles[i])
			{
				if (clauses[i-1] && roles[i-1] & (SUBJECT2|OBJECT2)) // but not "I am curious what color *it is" or "I am curious who *I am"
				{
					SetRole(i-1,OBJECT2,true);
					SetRole(i,SUBJECT2);
				}
				// we have dual subject stuff and awaiting verb
				else if (!stricmp(wordStarts[startSentence],(char*)"what") && roleIndex == MAINLEVEL && subjectStack[MAINLEVEL] && needRoles[MAINLEVEL] & MAINVERB) // what a big nose *he has.
				{
					unsigned int subject = subjectStack[MAINLEVEL];
					SetRole(subject,MAINOBJECT,true);
					SetRole(i,MAINSUBJECT);
				}
				else  AddClause(i,(char*)"omitted that or when?"); 
			}
		}
	}
}

static bool IsFinalNoun( int i)
{
	bool finalNoun = false;
	// see if this is final noun in a series
	if (posValues[NextPos(i)] & POSSESSIVE){;}
	else if (posValues[i] & (PRONOUN_BITS)) finalNoun = true;
	else if (posValues[i] & (NOUN_PLURAL | NOUN_ADJECTIVE)) finalNoun = true; // "She gave the *children homework" 
	else if (posValues[i] & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL) && bitCounts[NextPos(i)] == 1 && (posValues[NextPos(i)] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL|POSSESSIVE|NOUN_SINGULAR|NOUN_PLURAL))) {;} 
	else if (posValues[i] & NOUN_PROPER_PLURAL) finalNoun = true; 
	else if (posValues[i] & NOUN_PROPER_SINGULAR && allOriginalWordBits[i] & NOUN_HUMAN) finalNoun = true; // human names will be final unless made possessive
	else if (posValues[i] & ( NOUN_INFINITIVE|NOUN_GERUND)) finalNoun = true; // can also accept objects
	else if (!(posValues[NextPos(i)] & ((ADJECTIVE_BITS - ADJECTIVE_PARTICIPLE)|POSSESSIVE|(NOUN_BITS-NOUN_GERUND)))) finalNoun = true;
	else if (IsLegalAppositive(i,i+1)) finalNoun = true;
	else if (canonicalUpper[i+1] && !canonicalLower[NextPos(i)] && posValues[NextPos(i)] & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) finalNoun = true; // this will be noun appositive or omitted preposition - "the thing Eli wanted" or "my brother Billy"
	else if (!clauses[i] && clauses[NextPos(i)]) finalNoun = true;	// we break on clause boundary
	else if (bitCounts[NextPos(i)] != 1) finalNoun = true;	// we must assume it for now...  "eating children tastes good"  "joey found his *baby blanket" 
	else if (posValues[NextPos(i)] & (ADJECTIVE_BITS - ADJECTIVE_PARTICIPLE)) // could this be adjective complement after noun? (char*)"this makes Jenn's *mom mad"
	{
		finalNoun = true;
	}
	return finalNoun;
}

static bool AssignRoles(bool &changed)
{
	unsigned int oldStart = startSentence; // in case we revise due to conjunction
	// Roles and Phrases are cumulative from call to call
	
	if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"\r\n---- Assign roles\r\n");
	char goals[MAX_WORD_SIZE];
	
	// preanalyze comma zones, to see what MIGHT be done within a zone..
restart0:
	AssignZones();

restart:
	int startComma = 0;
	int endComma = 0;
	int commalist = 0;
    InitRoleSentence(startSentence, endSentence);
    AddRoleLevel(MAINSUBJECT | MAINVERB, startSentence);

	// Once a role is assigned, it keeps it forever unless someone overrides it explicitly.
	for (int i = startSentence; i <= endSentence; ++i)
	{
		if (roles[i] & TAGQUESTION) 
		{
			CloseLevel(i);
			needRoles[MAINLEVEL] = 0;	// now cancelled on needs
		}

		if (roles[i-1] & MAINVERB)  
		{
			currentMainVerb = i - 1;
			if (objectRef[0]) // object preceeded verb
			{
				objectRef[currentMainVerb] = objectRef[0];
				objectRef[0] = 0;
			}
		}
		else if (roles[i-1] & VERB2) 
		{
			currentVerb2 = i - 1; // can object preceed in a clause and need delayed storage?
			if (objectRef[MAX_SENTENCE_LENGTH-1]) // object preceeded verb
			{
				objectRef[currentVerb2] = objectRef[MAX_SENTENCE_LENGTH-1];
				objectRef[MAX_SENTENCE_LENGTH-1] = 0;
			}
		}
		
		if (ignoreWord[i])
		{
			if (ignoreWord[i] == IGNOREPAREN)  ResolveByStatistic(i,changed);
			continue;	// ignore these
		}
	
		if (i == endSentence && *wordStarts[i] == '"' && endSentence < wordCount) // absorb quote reference into sentence if not end of input
		{
			SetRole(i,MAINOBJECT,true);
			continue;
		}
		char* word = wordStarts[i];

		if (trace & TRACE_POS)
		{
			char flags[MAX_WORD_SIZE];
			strcpy(flags,(char*)"Flags: ");
			ParseFlags(flags,i);
			if (!flags[7]) *flags = 0;	// remove header if none there
			char tags[MAX_WORD_SIZE];
			*tags = 0;
			Tags(tags,i);
			strcat(tags,flags);
			WORDP X =  canonicalLower[i];
			if (!X) X = canonicalUpper[i];
			if (!X) X = StoreWord("Missing canonical");
			Log(STDTRACELOG,(char*)"%d) \"raw: %s canonical: %s\"",i,word,X->word);
			if (phrasalVerb[i] && phrasalVerb[i] > i && posValues[i] & (VERB_BITS|NOUN_INFINITIVE|NOUN_GERUND|ADJECTIVE_PARTICIPLE))
			{
				WORDP verb = GetPhrasalVerb(i);
				if (verb) Log(STDTRACELOG,(char*)" %s ",verb->word);
			}
			Log(STDTRACELOG,(char*)" (%s)\r\n",tags);
			for (unsigned int x = roleIndex; x >= 1; --x)
			{
				DecodeneedRoles(x,goals);
				if (x == MAINLEVEL) Log(STDTRACELOG, "    need %d %s - s:%d aux:%d v:%d io:%d o:%d\r\n",x,goals,subjectStack[x],auxVerbStack[x],verbStack[x],indirectObjectRef[currentMainVerb],objectRef[currentMainVerb]);
				else Log(STDTRACELOG, "    need %d %s - s:%d aux:%d v:%d io:%d o:%d\r\n",x,goals,subjectStack[x],auxVerbStack[x],verbStack[x],indirectObjectRef[currentVerb2],objectRef[currentVerb2]);
			}
		}

		if (bitCounts[i] > 1)
		{
			unsigned int result = GuessAmbiguous(i,changed);
			if (result == GUESS_RETRY) // somebody fixed it. try again
			{
				--i;
				continue;
			}
			if (result == GUESS_RESTART) goto restart;
			if (result == GUESS_ABORT) return false; // abort if we cannot resolve noun or verb conflict
			if (posValues[i] == (ADJECTIVE_NORMAL|ADVERB)){;}
			else ResolveByStatistic(i,changed); // prior call set back something to ambigous again.
		}
		StartImpliedPhrase(i,changed);
		StartImpliedClause(i,changed); // for things NOT known to be conjunction_subordinate

		switch(posValues[i]) // mostly only handles exact match values
		{
		case AUX_VERB_PRESENT: case AUX_VERB_FUTURE: case AUX_VERB_PAST: case AUX_BE: case AUX_HAVE: case AUX_DO: // AUX_VERB
			
			// we arent expecting anything, but a moment ago it was a phrase that could have been elevated to a clause...
			if (roleIndex == MAINLEVEL && !(needRoles[roleIndex] & (VERB2|MAINVERB)) && phrases[i-1])
			{
				if (FlipPrepToConjunction(i,false,changed)) goto restart;
			}

			// if omitted subject, imply it from adjective preceeded by the "the beautiful can do no wrong"
			if (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2))
			{
				int at = i-1;
				if (at > startSentence && posValues[at] & ADVERB) --at;	// the wicked often *can outwit others"
				if (at > 1 && posValues[at] & ADJECTIVE_NORMAL && !stricmp(wordStarts[at-1],(char*)"the"))
				{
					LimitValues(at,NOUN_ADJECTIVE,(char*)"adjective acting as noun",changed);
					SetRole(at, (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)));
				}
			}
			
			// if we are a auxverb immediately after indirectobject, presume this was an object clause instead- "do you think *she can be ready"
			if (posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS|VERB_PAST) && roles[i-1] & (MAININDIRECTOBJECT|INDIRECTOBJECT2) && parseFlags[verbStack[roleIndex]] & OMITTABLE_THAT_VERB)
			{
				SetRole(i-1,OBJECT2); // no longer indirect
				if (posValues[i-1] & PRONOUN_OBJECT) LimitValues(i-1,PRONOUN_OBJECT,(char*)"pronoun subject of clause forced to subject",changed);
				AddClause(i-1,(char*)"clause - implied by verb following indirectobject\r\n");
				SetRole(i-1,SUBJECT2); // head of clause
			}

			if (roleIndex == 2 && needRoles[roleIndex] & CLAUSE && needRoles[roleIndex] & VERB2 && !(needRoles[roleIndex] & SUBJECT2) && !subjectStack[MAINLEVEL] && allOriginalWordBits[startSentence] & QWORD) // misreading "whose bike were you riding" as a clause when it isnt, its a simple question, but if we already HAVE a subject for main level not bad- "I do not know if he will go"
			{// dont drop for "although findings were *reported more than a year ago, we died"
				DropLevel(); 	// drop back to main sentence treating this as direct object instead of clause
				for (int x = lastClause; x < i; ++x) 
				{
					if (roles[x] & SUBJECT2) SetRole(x,MAINOBJECT);
					clauses[x] = 0;
				}
				lastClause = 0;
			}
			if (needRoles[roleIndex] & OBJECT2 && roleIndex > 1)
			{	
				DropLevel(); 	// not going to happen, we are done with this level
			}

			if (!verbStack[roleIndex]) auxVerbStack[roleIndex] = (unsigned char)i;	// most recent aux for this level, but only if no verb found yet (must preceed verb)
			break;
		case VERB_INFINITIVE: 
			if (roleIndex == MAINLEVEL && needRoles[roleIndex] & MAINSUBJECT && needRoles[roleIndex] & MAINVERB) needRoles[roleIndex] &= -1 ^ MAINSUBJECT; // command
	
			// drop thru to other verbs
		case VERB_PRESENT: case VERB_PRESENT_3PS: case VERB_PAST: case VERB_PAST_PARTICIPLE: case VERB_PRESENT_PARTICIPLE: //  VERB_BITS
			 // does not include NOUN_INFINITIVE or NOUN_GERUND or ADJECTIVE_PARTICIPLE - its a true verb (not aux)
			{
				// we arent expecting anything, but a moment ago it was a phrase that could have been elevated to a clause...
				if (roleIndex == MAINLEVEL && !(needRoles[roleIndex] & (VERB2|MAINVERB)) && phrases[i-1])
				{
					if (FlipPrepToConjunction(i,false,changed))	goto restart;
				}

				// if omitted subject, imply it from adjective preceeded by the "the beautiful can do no wrong"
				if (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2))
				{
					int at = i-1;
					if (at > startSentence && posValues[at] & ADVERB) --at;	// the wicked often *can outwit others"
					if (at > 1 && posValues[at] & ADJECTIVE_NORMAL && !stricmp(wordStarts[at-1],(char*)"the"))
					{
						LimitValues(at,NOUN_ADJECTIVE,(char*)"adjective acting as noun",changed);
						SetRole(at,(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)));
					}
				}
	
				// if we are a verb immediately after indirectobject, presume this was an object clause instead- "do you think *she is ready"
				if (posValues[i] & (VERB_PRESENT|VERB_PRESENT_3PS|VERB_PAST) && roles[i-1] & (MAININDIRECTOBJECT|INDIRECTOBJECT2) && parseFlags[verbStack[roleIndex]] & OMITTABLE_THAT_VERB)
				{
					SetRole(i-1,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2); // no longer indirect
					if (posValues[i-1] & PRONOUN_OBJECT) LimitValues(i-1,PRONOUN_OBJECT,(char*)"pronoun subject of clause forced to subject",changed);
					AddClause(i-1,(char*)"clause - implied by verb following indirectobject\r\n");
					SetRole(i-1,SUBJECT2); // head of clause
				}

				// if we dont need a verb and prior was a verb and could have been a noun, revert it and cancel...
				if (posValues[i] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS) &&
					allOriginalWordBits[i-1] & NORMAL_NOUN_BITS &&
					roles[i-1] & (VERB2|MAINVERB))
				{
					// if we are a satisifed verbal and prior level needed a verb, we should close out this level
					//  "to wait *seemd too much"
					if (needRoles[roleIndex] & VERBAL && !(needRoles[roleIndex] & VERB2))
					{
						DropLevel(); 
					}
					else
					{
						// could this be a parenthetical inside a question "what actress do you think *is prettiest"
						if (roleIndex == MAINLEVEL && allOriginalWordBits[startSentence] & QWORD && objectStack[MAINLEVEL] && roles[i-2] == MAINSUBJECT && roles[i-1] == MAINVERB)
						{
							unsigned int start = i-2;
							if (posValues[i-3] & AUX_VERB) start = i-3;
							AddClause(start,(char*)"clause - parenthetical\r\n");
							SetRole(i-2,SUBJECT2,true,0);
							SetRole(i-1,VERB2,true,0);
							CloseLevel(i-1);
							SetRole(objectStack[MAINLEVEL],MAINSUBJECT,true,0);
							needRoles[MAINLEVEL] =  MAINVERB; // we will still need this as our verb
						}
						else 
						{
							needRoles[roleIndex] |= roles[i-1];
							SetRole(i-1,0);
							LimitValues(i-1,allOriginalWordBits[i-1] & (NOUN_BITS - NOUN_INFINITIVE - NOUN_GERUND),(char*)"hitting 2 verbs, force prior to noun",changed);
						}
					}
				}

				//	if (!verbStack[roleIndex]) verbStack[roleIndex] = (unsigned char)i;	// current verb if we dont have one yet  - handled by seekobjects
				
				// a real verb will terminate a bunch of things
				//To sleep *is (no objects will be found)
				if (needRoles[roleIndex] & VERB2) // verb is inside a clause, probably clausal verb we expected
				{
					// if we are MISSING a needed subject, can we fill that in NOW?
					if (needRoles[roleIndex] & SUBJECT2)
					{
						// if a clause, is the starter a potential subject (like which, who, etc)
						if (lastClause)
						{
							if (originalLower[lastClause] && originalLower[lastClause]->properties & PRONOUN_BITS)
							{
								SetRole(lastClause,SUBJECT2);
							}
						}
						needRoles[roleIndex] &=  -1 ^ SUBJECT2;
					}

					AddRole(i,VERB2); // in case "I have to *find a ball" assigned MAINOBJECT before VERB2
					if (lastClause && needRoles[roleIndex] & CLAUSE) // drag clause thru here
					{
						// if we still need a subject, can starter of clause be it?
						if (needRoles[roleIndex] & SUBJECT2)
						{
							if (originalLower[lastClause] && originalLower[lastClause]->properties & PRONOUN_SUBJECT)
							{
								SetRole(lastClause,SUBJECT2);
							}
						}
						ExtendChunk(lastClause,i,clauses);
					}
					if (lastVerbal && needRoles[roleIndex] & VERBAL) ExtendChunk(lastVerbal,i,verbals); // drag verbal thru here
				}
				else if (needRoles[roleIndex] & MAINVERB) // we seek a main verb....
				{
					// ASSIGN SUBJECT IF WE CAN

					// if we get here and HAVE no subject, but we have a pending verbal, assign it as the subject
					if (needRoles[roleIndex] & MAINSUBJECT && lastVerbal) 
					{
						SetRole(lastVerbal,MAINSUBJECT);
						if ( bitCounts[lastVerbal] != 1 && posValues[lastVerbal] & (NOUN_GERUND|NOUN_INFINITIVE))
						{
							LimitValues(lastVerbal,NOUN_GERUND|NOUN_INFINITIVE,(char*)"pending verbal assigned as subject, be gerund or infinitive",changed);
							if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"Resolve AMBIGUOUS_VERBAL as NOUN_GERUND or NOUN_INFINITIVE\r\n");
						}
					}

					// we have no subject, but we have a probable noun clause, use that
					if (needRoles[roleIndex] & MAINSUBJECT && firstNounClause) // clause should not intervene before direct object unless describing indirect object
					{
						SetRole(firstNounClause,MAINSUBJECT);
						firstNounClause =  0;
					}

					// NOW HANDLE MAIN VERB and its requirements
					unsigned int needed = needRoles[roleIndex];
					SetRole(i,MAINVERB);
					predicateZone = currentZone;
					// is not a command if the OBJECT has already been seen (like a quotation: "love you" *said Jack"
					if (needed & MAINSUBJECT && !(needRoles[roleIndex] & MAINSUBJECT) && allOriginalWordBits[i] & VERB_INFINITIVE && !objectRef[currentMainVerb])
					{
						LimitValues(i,VERB_INFINITIVE,(char*)"Resolve verb as Infinitive",changed); // go with imperative
					}
				}
				else  if (posValues[i] & VERB_INFINITIVE && needRoles[roleIndex] & VERB_INFINITIVE_OBJECT) 
				{
					int indirect = indirectObjectRef[verbStack[roleIndex]];
					if (indirect)
					{
						SetRole(indirect,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2,true);
						SetRole(i,OBJECT_COMPLEMENT); 
					}
					else SetRole(i,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2); 
					AddVerbal(i);
					AddRole(i,VERB2);
				}
				else if (posValues[i] & VERB_INFINITIVE)
				{
					AddRoleLevel(CLAUSE,i);
					verbStack[roleIndex] = (unsigned char)i;
					AddRole(i,VERB2);
				}
				// when an infinitive is waiting to close because it wants an object, this MAIN verb
				// this might be wrong if you can insert a clause in there somewhere 
				else if (needRoles[roleIndex] & OBJECT2)  // not absolute phrase
				{
					needRoles[roleIndex] &= -1 ^ ( OBJECT2 | INDIRECTOBJECT2);	// we wont find these any more
					CloseLevel(i-1);
				}
				else if (subjectStack[MAINLEVEL] && subjectStack[MAINLEVEL] < startComma && i < endComma) // saw a subject, now in a comma phrase, wont be main verb (except it might be- hugging the wall, nathan *peered  Or comma phrase separated subject from verb as appositive or such
				{
					AddRole(i,VERB2);
					AddRoleLevel(COMMA_PHRASE,i);
				}
				// we didnt need this verb, but was a phrase before. Maybe it should have been a conjunction.
				else if (phrases[i-1] && !(needRoles[roleIndex] & (MAINVERB|VERB2))) // "he went *until the honey ran out"
				{
					int before = i;
					int phrase = phrases[i-1];
					while (--before && phrases[before] == phrase) phrases[before] = 0;
					++before;	
					if (allOriginalWordBits[before] & CONJUNCTION)
					{
						LimitValues(before,CONJUNCTION,(char*)"unexpected verb after phrase, convert phrase to clause",changed);
					}
					AddClause(before,(char*)"converting phrase to clause");
					SetRole(i-1,SUBJECT2,false);
					crossReference[i-1] = (unsigned char) before;
					SetRole(i,VERB2,false);
					crossReference[i] = (unsigned char) before;
					ExtendChunk(before,i,clauses);
				}

				// see if verb might take an object of some kind

				// A passive voice does not take an object - need aux to be "be" verb
				bool noobject = false;
				if (posValues[i] & VERB_PAST_PARTICIPLE) // after the dog had been walked (passive takes no objects)
				{
					unsigned int x = i;
					while (--x)
					{
						if (posValues[x] & AUX_VERB_PAST && !ignoreWord[x]) // window was broken. but not window is broken (participle description)
						{
							if (canonicalLower[x] && canonicalLower[x]->word[0] == 'b' ) noobject = true;
							break;
						}
					}
				}
				if (roles[i] & MAINVERB) currentMainVerb = i;
				else if (roles[i] & VERB2) currentVerb2 = i;
				if (!noobject) SeekObjects(i);
				// if no object is expected. If we see a noun following, PRESUME it is part of an implied "that" clause - "I hope she goes", but might be a time phrase like "this morning."
				if (needRoles[roleIndex] == CLAUSE) lastClause = 0;	// all fulfilled
				if (needRoles[roleIndex] == VERBAL && !(needRoles[roleIndex] & VERB2)) lastVerbal = 0; // all fulfilled (and didnt launch a new verbal) "let us hear the men *stand as they pass"
			}
			break;
			case AMBIGUOUS_VERBAL: // (ADJECTIVE_PARTICIPLE | NOUN_GERUND ) 
				AddVerbal(i);

				// drop thru to adjective participle
			case ADJECTIVE_NOUN: 
			case ADJECTIVE_NORMAL: case ADJECTIVE_PARTICIPLE: case ADJECTIVE_NUMBER: // ADJECTIVE_BITS
			{
				// absolute phrase is like implied prep phrase "with" in "legs quivering, we ran".  requires comma seaparation and no aux verbs
				if (!phrases[i] && !auxVerbStack[roleIndex] && (posValues[i] & ADJECTIVE_PARTICIPLE) && (currentZone == 0|| currentZone == (int)(zoneIndex-1)) && zoneIndex > 1)
				{
					int at = i;
					while (--at && !phrases[at] && !verbals[at] && !clauses[at] && (!(posValues[at] & (COMMA|CONJUNCTION_SUBORDINATE)) || ignoreWord[at]) && !(roles[at] & ( SUBJECT2 | MAINSUBJECT | OBJECT2) )) {;}
					bool abs = false;
					if (roles[at] & SUBJECT2 || roles[at] & OBJECT2 || !roles[at]) 
					{
						roles[at] = OBJECT2;
						abs = true;
					}
					else if (roles[at] & MAINSUBJECT && needRoles[MAINLEVEL] & MAINVERB && !(posValues[i] & (VERB_PRESENT|VERB_PAST|VERB_PRESENT_3PS))) // we need a verb and this cant be it
					{
						// if aux, we can also tolerate participles
						if (posValues[i] & (VERB_PRESENT_PARTICIPLE|VERB_PAST_PARTICIPLE) && auxVerbStack[MAINLEVEL]){;}
						else
						{
							needRoles[MAINLEVEL] |= MAINSUBJECT;
							roles[at] = OBJECT2;
							abs = true;
						}
					}
					if (abs) 
					{
						LimitValues(i,ADJECTIVE_PARTICIPLE,(char*)"absolute phrase will take adjective participle",changed);
						zoneData[currentZone] = ZONE_ABSOLUTE;
						AddPhrase(at);
						DropLevel();  // drop phrase level, we're done
						ExtendChunk(at,i,phrases); // cover verbal with prep phrase
					}
				}

				// if we are adjective, can we describe the noun after: or "my six year *old wins" 
				if (parseFlags[i+1] & TIME_NOUN && parseFlags [i+2] & (TIME_ADVERB|TIME_ADJECTIVE))// we are normal adjective
				{
				} 
				else if (parseFlags[i+1] & DISTANCE_NOUN && parseFlags [i+2] & (DISTANCE_ADVERB|DISTANCE_ADJECTIVE))// we are normal adjective
				{
					needRoles[roleIndex] &= -1 ^ (MAINOBJECT|OBJECT2);	
				} 
				else if (needRoles[roleIndex] & SUBJECT_COMPLEMENT) SetRole(i,SUBJECT_COMPLEMENT);
				// verb expects adjective after direct object - and not nondescriptive adjective
				else if (roles[i-1] & (MAINOBJECT|OBJECT2) && needRoles[roleIndex] & OBJECT_COMPLEMENT && !(needRoles[roleIndex] & (INDIRECTOBJECT2|MAININDIRECTOBJECT)) && posValues[i-1] & (NOUN_BITS|PRONOUN_BITS) && 
					parseFlags[verbStack[roleIndex]] & FACTITIVE_ADJECTIVE_VERB && !(parseFlags[i] & NONDESCRIPTIVE_ADJECTIVE))
				{
					if (roleIndex == MAINLEVEL && indirectObjectRef[MAINLEVEL]) SetRole(indirectObjectRef[currentMainVerb],MAINOBJECT,true);
					else if (roleIndex > MAINLEVEL && indirectObjectRef[roleIndex]) SetRole(indirectObjectRef[currentVerb2],OBJECT2,true);
					needRoles[roleIndex] &= -1 ^ (MAINOBJECT|OBJECT2);	// we are here instead
					SetRole(i,OBJECT_COMPLEMENT);
				}
				else if (parseFlags[i] & NONDESCRIPTIVE_ADJECTIVE && posValues[NextPos(i)] & PREPOSITION) needRoles[roleIndex] &= -1 ^ OBJECT_COMPLEMENT; // junk (actually an idiom) but wont be wanting a complement any more
				// hopefully a legal trailing adjective
				else if (posValues[i-1] & (PRONOUN_BITS|NOUN_BITS) && !(posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS)) ) // however, when suzy came home from school, she found her mother *tired and worn out"
					SetRole(i,POSTNOMINAL_ADJECTIVE); 
				// hopefully a legal trailing adjective
				else if (canSysFlags[i-1] & TAKES_POSTPOSITIVE_ADJECTIVE && posValues[i-1] & (NORMAL_NOUN_BITS|PRONOUN_BITS) )
					SetRole(i,POSTNOMINAL_ADJECTIVE); 
			
				unsigned int level = roleIndex;
				if (posValues[i] == ADJECTIVE_PARTICIPLE)  // "the men using a sword were"  and "the walking dead"
				{
					if (roles[i]){;}
					// accidents have been reported involving passengers
					else if ( roles[i-1] & MAINVERB && allOriginalWordBits[i] & VERB_PRESENT_PARTICIPLE)
					{
						AddRole(i,VERB2);	// default role -- will be verbal describing verb...EXCEPT it may be an adjective object of a linking verb
					}
					else if (posValues[NextPos(i)] & (NOUN_BITS|DETERMINER|PRONOUN_POSSESSIVE)) {;} // describing a noun as boring adjective UNLESS its our object!
					else SetRole(i,0);	// default role
				
					// establish a permanent verbal zone if this is freestanding or following a noun and isnt in a zone
					if (posValues[i-1] & (ADJECTIVE_BITS|DETERMINER|POSSESSIVE|PRONOUN_POSSESSIVE|CONJUNCTION)){;} // wont take an object, can only describe one "the tall and distinguished gentleman"
					
					// we dont care about adj part occurring BEFORE a noun.. that's ordinary adjective
					else 
					{
						AddVerbal(i); // BUT NOT if it is following an adjective,determiner, possessive, etc (before a noun)
						needRoles[roleIndex] &= -1 ^ VERB2;
					}
					//  "who is a *dating service for" has the object as not part of this level
					
					// present participle can start a sentence and have objects
					bool mightWantObject = (i == startSentence); // as a sentence start, it can want an object.
					if (posValues[i] & VERB_PAST_PARTICIPLE) mightWantObject = false; // past participle cannot have an object
					else if (verbals[i] && posValues[i-1] & (ADJECTIVE_BITS|DETERMINER)) mightWantObject = false; // not following a noun
					else if (posValues[i] & ADJECTIVE_PARTICIPLE) mightWantObject = false;
					else if (verbals[i] ) // occuring after the noun // note- adjective-participles may occur AFTER the noun and have objects but not when before - we have the people *taken care of
					{
						mightWantObject = true;
						if (canSysFlags[i] & (VERB_DIRECTOBJECT|VERB_INDIRECTOBJECT)) // change to wanting object(s)
						{
							// wont have indirect object we presume
							verbStack[roleIndex] = (unsigned char)i;	// this is the verb for this
							needRoles[roleIndex] |= OBJECT2; // mark this kind of level (infinitive or gerund) UNLESS this is actually the main verb
						}
					}
					//else- occurring before the noun it is just a descriptor and cant take objects

					if (mightWantObject) SeekObjects(i);
				}

				// if we follow a pronoun like something, with possible adverb between "she was nothing but *nice"
				if (!roles[i] && posValues[i-1] & PRONOUN_BITS  && parseFlags[i-1] & INDEFINITE_PRONOUN )
				{
					SetRole(i, POSTNOMINAL_ADJECTIVE);
				}

				// if we follow a pronoun like something, with possible adverb between "she was nothing but *nice"
				if (!roles[i] && i > startSentence && posValues[i-2] & PRONOUN_BITS && parseFlags[i-2] & INDEFINITE_PRONOUN && posValues[i-1] & ADVERB)
				{
					SetRole(i, POSTNOMINAL_ADJECTIVE);
				}

				// assign adjective object for this level if appropriate
				// not "he is a green man"
				// he is green. but not they are green and sticky men
				if (!roles[i] && needRoles[level] & ADJECTIVE_COMPLEMENT) // canSysFlags[verbStack[level]] & VERB_TAKES_ADJECTIVE) 
				{
					// see if we are postnominal
					if (posValues[i-1] & (NOUN_BITS | PRONOUN_BITS) && roles[i-1] & MAINOBJECT ) 
					{
						SetRole(i, OBJECT_COMPLEMENT);
						continue; // not for object complements:  they considered him crazy  
					}
					// check is we are prenominal
					 int j = i;
					while (++j <= endSentence) // prove no nouns in our way after this -- are you the scared cat  vs  are you afraid of him
					{
						if (ignoreWord[j]) continue;
						if ((clauses[j] && clauses[i] != clauses[j]) || (verbals[j] && verbals[i] != verbals[j]) || 
							(phrases[j] && phrases[i] != phrases[j])) break;	// not relevant (though detecting these in a clause would be nice)
						if (posValues[j] & (PAREN | COMMA | CONJUNCTION|PREPOSITION|TO_INFINITIVE|NOUN_GERUND)) break; // fall off "we are *responsible for it all"
						if (posValues[j] & NOUN_BITS)  //  but not they are green men  -- we probably describe a noun
						{
							j =  2000; // fall off
							break;
						}
					}
					if ( j != 2000) 
					{
						SetRole(i,SUBJECT_COMPLEMENT);
						needRoles[level] &= -1 ^ ALL_OBJECTS; // prior level is done
					}
				}
				// we can be an adj complement IF expected and not in front of some other continuing thing like "I like *gooey ice cream"
				if (!roles[i] && needRoles[roleIndex] & OBJECT_COMPLEMENT && !(posValues[NextPos(i)] & (ADJECTIVE_BITS|NOUN_BITS)))
				{
					SetRole(i, OBJECT_COMPLEMENT);
					continue;  
				}
				// we follow subor conjuct and end at end of sentence or with comma, omitted subject/verb clause "if *dead, awaken"
				int prior = i;
				while (--prior > startSentence && posValues[prior] == ADVERB) {;}
				if (posValues[prior] == CONJUNCTION_SUBORDINATE && roles[prior] != OMITTED_SUBJECT_VERB)
				{
					if (posValues[i+1] == COMMA || (i+1) == endSentence) // if dead, awaken
					{
						SetRole(prior,OMITTED_SUBJECT_VERB);
						SetRole(i, SUBJECT_COMPLEMENT);
					}
				}
			}
			break;
			case AMBIGUOUS_PRONOUN: // (PRONOUN_SUBJECT | PRONOUN_OBJECT )
				// drop thru to pronouns
			case NOUN_SINGULAR: case NOUN_PLURAL: case NOUN_PROPER_SINGULAR: case NOUN_PROPER_PLURAL: case NOUN_GERUND: case NOUN_NUMBER: case NOUN_INFINITIVE: case NOUN_ADJECTIVE: //NOUN_BITS
			case PRONOUN_SUBJECT: case PRONOUN_OBJECT:
			{
				// cancels direct object if adverb after verb before noun - EXCEPT "there are always cookies tomorrow 
				//if (posValues[i-1] & ADVERB && posValues[i-2] & VERB_BITS && needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && tokenControl & TOKEN_AS_IS) 
				//{
					//needRoles[roleIndex] &= -1 ^ (OBJECT2|MAINOBJECT|MAININDIRECTOBJECT|INDIRECTOBJECT2);
				//}
	
				// forced role on reflexives so wont be  or some kind of compelement "I prefer basketball *myself" but not "I cut *myself"
				if (originalLower[i] && originalLower[i]->systemFlags & PRONOUN_REFLEXIVE && !(needRoles[roleIndex] & (MAINOBJECT|OBJECT2)))
				{
					SetRole(i,REFLEXIVE);
					break;
				}
				if (roles[i] & ADDRESS) break;	// we already know we are an address role, dont rethink it
				
				// we thought prior was a complete noun, but now we know it wasnt - change it over 
				if (posValues[i-1] == NOUN_SINGULAR && posValues[i] & (NOUN_SINGULAR|NOUN_PLURAL) && roles[i-1] && !(parseFlags[verbStack[roleIndex]] & FACTITIVE_NOUN_VERB))
				{
					uint64 role = roles[i-1];
					SetRole(i-1,0);
					SetRole(i,role,true); // move role over to here
					LimitValues(i-1,ADJECTIVE_NOUN,(char*)"revising prior nounsingular to adjectivenoun",changed);
					if (phrases[i-1]) phrases[i] = phrases[i-1];	// keep phrase going to here
					break;
				}

				if (!IsFinalNoun(i) && posValues[i] & NORMAL_NOUN_BITS && posValues[i] & ADJECTIVE_BITS)  // not really at end of noun sequence, not the REAL noun - dont touch "*Mindy 's"
				{
					LimitValues(i,ADJECTIVE_BITS,(char*)"forcing adjective since not final",changed);
					SetRole(i,0);
					continue;
				}
				
				if (!IsFinalNoun(i) && posValues[i] & NORMAL_NOUN_BITS)  // not really at end of noun sequence, not the REAL noun - dont touch "*Mindy 's"
				{
					LimitValues(i,ADJECTIVE_NOUN,(char*)"forcing adj noun since not final",changed);
					SetRole(i,0);
					continue;
				}

				// if we thought a leading verbal was the subject and we have a superfluous noun here in next zone, revise...
				if (roleIndex == MAINLEVEL && needRoles[MAINLEVEL] & MAINVERB && !(needRoles[MAINLEVEL] & MAINSUBJECT) &&
					currentZone == 1 && subjectStack[MAINLEVEL] && posValues[subjectStack[MAINLEVEL]] & NOUN_GERUND)
				{
					if (firstnoun == subjectStack[MAINLEVEL]) firstnoun = 0;
					LimitValues(subjectStack[MAINLEVEL],ADJECTIVE_PARTICIPLE,(char*)"revising old mainsubject on finding new one",changed);
					SetRole(subjectStack[MAINLEVEL], 0);	// remove role
					needRoles[MAINLEVEL] |= MAINSUBJECT;	// add it back
				}

				// noun of address? - "here, *Ned, catch the ball"
				if (posValues[i] & NOUN_PROPER_SINGULAR && posValues[i-1] & COMMA && posValues[NextPos(i)] & COMMA && roleIndex == MAINLEVEL && needRoles[roleIndex] & MAINSUBJECT && posValues[Next2Pos(i)] & VERB_INFINITIVE)
				{
					SetRole(i,ADDRESS);
					LimitValues(i+2,VERB_INFINITIVE,(char*)"command infinitive after address noun",changed);
					break;
				}

				// distance noun?
				if (parseFlags[i] & DISTANCE_NOUN && parseFlags[i+1] & DISTANCE_ADVERB)
				{
					SetRole(i,DISTANCE_NOUN_MODIFY_ADVERB);
					break;
				}
				if (parseFlags[i] & DISTANCE_NOUN && parseFlags[i+1] & DISTANCE_ADJECTIVE)
				{
					SetRole(i,DISTANCE_NOUN_MODIFY_ADJECTIVE);
					break;
				}
				// time noun?
				if (parseFlags[i] & TIME_NOUN && parseFlags[i+1] & TIME_ADVERB)
				{
					SetRole(i,TIME_NOUN_MODIFY_ADVERB);
					break;
				}
				// time noun?
				if (parseFlags[i] & TIME_NOUN && parseFlags[i+1] & TIME_ADJECTIVE)
				{
					SetRole(i,TIME_NOUN_MODIFY_ADJECTIVE);
					break;
				}

				if (!firstnoun) firstnoun = i; // note noun for potential prep wrapped from end

				// something wanting subject or object can solve unknown pronoun now. 
				if (posValues[i] != AMBIGUOUS_PRONOUN || !(needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2|MAINOBJECT|OBJECT_COMPLEMENT|OBJECT2)) ){;} // we are not ambigous pronoun and not looking to have a noun at present
				else 
				{
					if (needRoles[roleIndex] & (MAINSUBJECT|SUBJECT2)) 
					{
						LimitValues(i,PRONOUN_SUBJECT,(char*)"Resolve AMBIGUOUS_PRONOUN as subject",changed);
					}
					else 
					{
						if (needRoles[roleIndex] & VERBAL) ExtendChunk(lastVerbal,i,verbals); // drag  verbal to here
						else if (needRoles[roleIndex] & CLAUSE) ExtendChunk(lastClause,i,clauses) ; // drag clause  to here
						LimitValues(i,PRONOUN_OBJECT,(char*)"Resolve AMBIGUOUS_PRONOUN as object",changed);
					}
				}
	
				if (parseFlags[i] & FACTITIVE_NOUN_VERB && needRoles[roleIndex] & (MAINOBJECT|OBJECT2) && !(needRoles[roleIndex] & OBJECT_COMPLEMENT)) // 1st time thru, set factitive needs
				{
					needRoles[roleIndex] |= OBJECT_COMPLEMENT;
					needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2|MAININDIRECTOBJECT); // we elected Jim president will be caught here  but we elected Jim to run will be parsed wrongly....
				}

				// ASSSIGN ROLE OF NOUN

				// unexpected noun/pronoun - if we are still awaiting mainverb and think we have main subject as "what"
				// make this the subject
				if (needRoles[MAINLEVEL] & VERB && roleIndex == MAINLEVEL && roles[startSentence] & MAINSUBJECT &&
					canonicalLower[startSentence] && canonicalLower[startSentence]->properties & QWORD)
				{
					SetRole(i,MAINSUBJECT);
					SetRole(startSentence,MAINOBJECT);
				}
				// unexpected noun/pronoun - if we are still awaiting verb for clause and think we have  subject as "what"
				// make this the subject
				else if (needRoles[roleIndex] & VERB && roles[i-1]& SUBJECT2 && canonicalLower[i-1] && canonicalLower[i-1]->properties & QWORD) // "I ate what they said"
				{
					SetRole(i-1,MAINOBJECT);
					SetRole(i,MAINSUBJECT);
				}
				// unexpected noun/pronoun - may be appositive
				else if (IsLegalAppositive(i-1,i))
				{
					SetRole(i,APPOSITIVE);
					crossReference[i] =  (unsigned char) (i-1);
				}
		
				else if (posValues[i] == NOUN_GERUND && needRoles[roleIndex] & CLAUSE && needRoles[roleIndex] & SUBJECT2) {;} // omitted subject "John ran while *walking"
				else if (roles[i-1] & (MAINOBJECT|OBJECT2) && parseFlags[verbStack[roleIndex]] & FACTITIVE_NOUN_VERB) SetRole(i,OBJECT_COMPLEMENT);
				else HandleComplement(i,changed);
				
				// NOW HANDLE VERBAL PROPERTIES
				
				// if this is a verbal (NOUN_GERUND/NOUN_INFINITIVE) it will be at new level ..
				//  Raise it back to its level and fill it in
				// extend reach for supplemental objects
				if (posValues[i] & (NOUN_GERUND|NOUN_INFINITIVE))  
				{
					AddVerbal(i);
					if (posValues[i] == NOUN_GERUND && needRoles[roleIndex] & CLAUSE && needRoles[roleIndex] & SUBJECT2) // omitted subject "John ran while *walking" - participle clause
					{
						needRoles[roleIndex-1] &= -1 &  ( SUBJECT2 | MAINSUBJECT);	// no subject expected now
						ExtendChunk(lastClause,i,clauses);
					}
					else if (posValues[i-1] & TO_INFINITIVE && posValues[i] & NOUN_INFINITIVE) ExtendChunk(i-1,i,verbals);
					else if (i > startSentence && posValues[i-2] & TO_INFINITIVE && posValues[i] & NOUN_INFINITIVE)  ExtendChunk(i-1,i,verbals);
					uint64 tmprole = roles[i];
					SetRole(i,VERB2); // force "will reading improve me" to do the right thing to reading
					roles[i] |= tmprole;
					SeekObjects(i);
					// are you able to drive a car wants object2 for car
					// "Be a man"
					// need to know if this is supplying an object or is describing a noun...
					// EG  I want to eat   vs  I want chitlins to eat ...
				}
				
				// "a flying wedge, the *eagle soared" is other order  of appoistive
				// MIGHT be appositive, or might be run-on 
			}
			break;
			case PREPOSITION: // PREPOSITION
			{
				// preposition as object of prep? (char*)"I came from *within the house"
				if (needRoles[roleIndex] & PHRASE && !stricmp(wordStarts[i-1],(char*)"from"))
				{
					SetRole(i,OBJECT2);
					CloseLevel(lastPhrase);
				}

				ImpliedNounObjectPeople(i-1, needRoles[roleIndex] & (MAINOBJECT|OBJECT2),changed);
	
				if (!(posValues[i-1] & ADJECTIVE_BITS)) needRoles[roleIndex] &= -1 ^ SUBJECT_COMPLEMENT;	// cannot have linking verb result after this (unless before was possible adjectives) "it was better *than a cookie"
				if (needRoles[roleIndex] & (CLAUSE|VERBAL) && roleIndex > 1) DropLevel(); 	 // was formerly a clause or verbal, ended by prep phrase  clause/verbal will never have direct object after this

				AddPhrase(i);// uncovered preposition via ambiguity

				if (needRoles[roleIndex-1] & MAINOBJECT && subjectStack[MAINLEVEL] > verbStack[MAINLEVEL]){;} // if main subject comes after main verb (question), then prep phrase cancels nothing
				else if (needRoles[roleIndex-1] & (OBJECT2|MAINOBJECT|OBJECT_COMPLEMENT) && !objectRef[verbStack[roleIndex-1]]) // phrase should not intervene before direct object unless describing indirect object
				{
					needRoles[roleIndex-1] &= -1 ^ ALL_OBJECTS;
				}

				// ENDING of sentence preposition wrapped to front
				if (i == endSentence && roles[startSentence] & MAINOBJECT  && posValues[startSentence] & (PRONOUN_BITS)  && canonicalLower[startSentence] && canonicalLower[startSentence]->properties & QWORD )
				{
					objectRef[startSentence] = (unsigned char)0; // implied higher level for prep phrase, so disable on level 1
					SetRole(startSentence,OBJECT2);
					phrases[startSentence] = phrases[endSentence]; // same marker
				}
			}
			break;
			case CONJUNCTION_COORDINATE: // CONJUNCTION_COORDINATE
			{
				if (i == startSentence) break;	// ignore coordination since it occurs to PRIOR sentence.
				if (posValues[NextPos(i)] & CONJUNCTION_SUBORDINATE && !stricmp(wordStarts[i],(char*)"so") && !stricmp(wordStarts[i+1],(char*)"that")) break; // "so that" is conjunction subordinate idiom
				if (!stricmp(wordStarts[i],(char*)"neither") || !stricmp(wordStarts[i],(char*)"either") || !stricmp(wordStarts[i],(char*)"not_only") || !stricmp(wordStarts[i],(char*)"whether") ) break;
				if (!stricmp(wordStarts[i],(char*)"nor"))
				{
					int at = i;
					while (--at >= startSentence) 
					{
						if (ignoreWord[at]) continue;
						if (!stricmp(wordStarts[at],(char*)"neither"))
						{
							LimitValues(at,CONJUNCTION_COORDINATE,(char*)"neither mated to nor",changed);
							break;
						}
					}
				}
				if (!stricmp(wordStarts[i],(char*)"or") && phrases[i] && !(needRoles[roleIndex] & PHRASE)) // restore fake prep OR level -- "which is better, cats or dogs"
				{
					AddRoleLevel(PHRASE|OBJECT2,i);
					lastPhrase = i;		// where last verbal was, if any
				}

				if (needRoles[roleIndex] & VERBAL && needRoles[roleIndex] & (SUBJECT_COMPLEMENT|TO_INFINITIVE_OBJECT|VERB_INFINITIVE_OBJECT|OBJECT2)) // also for if we didnt decide its role
				{
					lastVerbal = 0; // cancel existing verbal which is waiting for an object. It wont get one
					DropLevel(); 
				}

				unsigned int result = HandleCoordConjunct(i,changed);
				if (result == GUESS_RETRY) // somebody fixed it. try again
				{
					--i;
					continue;
				}

				if ((roles[i] & CONJUNCT_KINDS) == CONJUNCT_ADJECTIVE) {;}
				else if ((roles[i] & CONJUNCT_KINDS) == CONJUNCT_ADVERB) {;}
				lastConjunction = 0;
				if ((roles[i] & CONJUNCT_KINDS) == CONJUNCT_SENTENCE) // we want a whole new sentence
				{
					if (!FinishSentenceAdjust(false,changed,startSentence,i-1)) break; // start over, it changed stuff
					startSentence = i+1;
					goto restart0;
				}
			}
			break;
			case CONJUNCTION_SUBORDINATE:  // any clause will have been started by StartClause because many start w/o conjunction
			{
				ImpliedNounObjectPeople(i-1, needRoles[roleIndex] & (MAINOBJECT|OBJECT2),changed);			
				// remaining conflict adj/adv resolve to adverb "let us stand *still, and hear"
				if (posValues[i-1] & ADJECTIVE_BITS && posValues[i-1] & ADVERB)
				{
					LimitValues(i-1,ADVERB,(char*)"adj/adv pending resolved by a conjunction",changed);
				}
				AddClause(i,(char*)"clause - conjunction subordinate\r\n");
				if (!stricmp(wordStarts[i],(char*)"albeit")) // means "although it is"
				{
					needRoles[roleIndex] &= -1 ^ ( SUBJECT2 | VERB2);
					needRoles[roleIndex] |= SUBJECT_COMPLEMENT;
					SetRole(i,OMITTED_SUBJECT_VERB);
				}
			}
			break;
			case PARTICLE: //  particle binds to PRIOR verb
			{
				int at = phrasalVerb[i];
				while (posValues[at] & PARTICLE && phrasalVerb[at] < at) at = phrasalVerb[at]; // find the verb
				crossReference[i] = (unsigned char) at;  // back link to verb from particle
				ExtendChunk(at,i,clauses);
				ExtendChunk(at,i,verbals);
				if (!phrases[i] && posValues[at] & NOUN_GERUND) ExtendChunk(at,i,phrases);
			}
			break;

			case INTERJECTION:
				break;

			case ADVERB: // adverb  binds to PRIOR verb, not next verb unless next to next verb ((char*)"if this is tea *please bring me sugar")
			{
				// adverb as object of prep? (char*)"I came from *within"
				if (needRoles[roleIndex] & PHRASE && !stricmp(wordStarts[i-1],(char*)"from") && !(posValues[NextPos(i)] & ADJECTIVE_BITS)) // BUG for I came from *very bad parents"
				{
					SetRole(i,OBJECT2);
					ExtendChunk(i-1,i,phrases);
					CloseLevel(i);
				}

				if (i > 1 && *wordStarts[i-1] == ';' && *wordStarts[i+1] == ',' && parseFlags[i] & CONJUNCTIVE_ADVERB) // conjunctive adverb
				{
					SetRole(i,CONJUNCT_SENTENCE);
					if (!FinishSentenceAdjust(false,changed,startSentence,i)) break; // start over, it changed stuff
					++i;
					startSentence = i+1;
					InitRoleSentence(startSentence,wordCount);
				}
				else if (posValues[NextPos(i)] & (ADJECTIVE_BITS|ADVERB)) crossReference[i] = (unsigned char)(i + 1); // if next is ADJECTIVE/ADVERB, then it binds to that?  "I like *very big olives"
				else if (posValues[NextPos(i)] & VERB_BITS) //  next to next verb ((char*)"if this is tea please bring me sugar")
				{
					if (clauses[i+1]) clauses[i] = clauses[i+1];
					if (verbals[i+1]) verbals[i] = verbals[i+1];
					crossReference[i] = (unsigned char)(i + 1);
				}
				else if (i > 1) // bind to prior verb
				{
					if (clauses[i-1]) clauses[i] = clauses[i-1];
					if (verbals[i-1]) verbals[i] = verbals[i-1];
					if ( phrases[i-1]  && !phrases[i] && posValues[i-1] & NOUN_GERUND) phrases[i] = phrases[i-1];
					unsigned int prior = FindPriorVerb(i);
					if (prior) crossReference[i] = (unsigned char)prior;
				}
			}
			break;
			case PUNCTUATION:
				if (!stricmp(wordStarts[i],(char*)"--")) // emphatic comma
				{
					// objects cannot pass thru -- boundaries 
					if (needRoles[roleIndex] & (ALL_OBJECTS|OBJECT_COMPLEMENT|SUBJECT_COMPLEMENT)) needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2 | OBJECT2 | MAINOBJECT | MAINOBJECT|OBJECT_COMPLEMENT|SUBJECT_COMPLEMENT);
					CloseLevel(i);  // drop the clause/verbal now.... "my mom, whose life is *, is"
					posValues[i] = COMMA; // revise to comma use for later noun-appositive tests
				}
				else if (*wordStarts[i] == ';') // do new sentence 
				{
					if (!FinishSentenceAdjust(false,changed,startSentence,i-1)) break; // altered world
					startSentence = i+1;
					InitRoleSentence(startSentence,wordCount);
				}
				else if (*wordStarts[i] == ':') // do new sentence
				{
					if (!FinishSentenceAdjust(false,changed,startSentence,i-1)) break; // altered world
					startSentence = i+1;
					InitRoleSentence(startSentence,wordCount);
				}
				// BUG not handling ( ) stuff yet
			break;
			case QUOTE:
				{
					++quotationCounter;
					if (!(quotationCounter & 1)) // closing quote
					{
						quotationRoleIndex = 0;
						unsigned int i;
						int found = 0;
						for (  i = 1; i <= roleIndex; ++i)
						{
							if (needRoles[i] & QUOTATION_UTTERANCE) found = i;
							if (found) quotationRoles[++quotationRoleIndex] = needRoles[i];	// replicate for possible restoration
						}
						if (found) roleIndex = found - 1; // drop quotation data for now

					}
					else if (quotationRoleIndex) // re-opening quote
					{
						for (unsigned int i = 1; i <= quotationRoleIndex; ++i) needRoles[++roleIndex] = quotationRoles[i];
						quotationRoleIndex = 0;
					}
					else // pure opening quote
					{
						AddRole(i,(roleIndex == MAINLEVEL) ? MAINOBJECT : OBJECT2);
						AddRoleLevel(QUOTATION_UTTERANCE|SUBJECT2|VERB2,i); // start of a clause triggers a new level for it, looking for subject and verb (which will be found).
					}
				}
				break;
			case COMMA: // close needs for indirect objects, ends clauses e.g.   after being reprimanded, she went home
			{
				++currentZone;
	
				// MAY NOT close needs for indirect objects, ends clauses because adjectives separated by commas) - hugging the cliff, Nathan follows the narrow, undulating road.
				if (posValues[i-1] & ADJECTIVE_BITS && posValues[NextPos(i)] & (NOUN_BITS|ADVERB|ADJECTIVE_BITS|CONJUNCTION_COORDINATE) && !(allOriginalWordBits[startSentence] & QWORD)) break; 
				
				// objects cannot pass thru comma boundaries unless they get reinstated by handleconjunct from the earlier side role
				if (needRoles[roleIndex] & (ALL_OBJECTS|OBJECT_COMPLEMENT|SUBJECT_COMPLEMENT)) needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2 | OBJECT2 | MAINOBJECT | MAINOBJECT|OBJECT_COMPLEMENT|SUBJECT_COMPLEMENT);
				
				// "the guy <c who has <v to decide >c>v, Hamlet, is"
				CloseLevel(i);  // drop the clause/verbal now.... "my mom, whose life is *, is"

				// see if comma for "which is better*, a boy or a girl"
				if (allOriginalWordBits[i-1] & ADJECTIVE_BITS && allOriginalWordBits[startSentence] & QWORD)
				{
					int at = i;
					while (++at < endSentence)
					{
						if (!stricmp(wordStarts[at],(char*)"or")) // found a comparator
						{
							if (posValues[at-1] & (NOUN_BITS | ADJECTIVE_NORMAL)) // noun or adjective choices - wont be verb (would be gerund) or adverb
							{
								SetRole(i,OMITTED_OF_PREP); 
								AddPhrase(i);//  fake preposition 
								crossReference[i] = (unsigned char) startSentence;
								ExtendChunk(i,at,phrases);
								break;
							}
						}
					}
					if (at < endSentence) break;
				}

				// one might have a start of sentence commas off from rest, like: After I arrived, he left and went home.
				// one might have a middle part like: I find that often, to my surprise, I like the new food.
				// one might have a tail part like:  I like you, but so what?
				// or comma might be in a list:  I like apples, pears, and rubbish.

				// one might have a start of sentence commas off from rest, like: After I arrived, he left and went home.
				// one might have a middle part like: I find that often, to my surprise, I like the new food.
				// one might have a tail part like:  I like you, but so what?
				// or comma might be in a list:  I like apples, pears, and rubbish.
				if (!startComma) // we have a comma, find a close.  if this is a comma list, potential 3d comma might exist
				{
					startComma = i;
					for ( int j = i+1; j < endSentence; ++j) 
					{
						if (posValues[j] & COMMA) // this comma ends first here at next comma
						{
							endComma = j;
							break;
						}
					}
				}
				// if we already had a comma and this is the matching end we found for it and we are in a comma-phrase, end it.
				else if (i == endComma && needRoles[roleIndex] & COMMA_PHRASE) 
				{
					DropLevel(); // end phrase
					if (roleIndex == 0) ++roleIndex;
				}

				// address?  "come here, Ned, and eat stuff"
				if (roleIndex == MAINLEVEL && !(needRoles[roleIndex] & MAINVERB) && posValues[NextPos(i)] & NOUN_PROPER_SINGULAR && posValues[Next2Pos(i)] == COMMA)
				{
					SetRole(i+1,ADDRESS);
					i += 2; // swall  ,Ned,
					++currentZone;
					continue;
				}

				// commas end some units like phrases, clauses and infinitives
				commalist = FindCoordinate(i); // commalist is now the matching conjunction after last comma
				if (commalist > 0) // treat this as coordinate conjunction (but skip over possible adj conjunct)
				{
					if (!(posValues[NextPos(i)] & CONJUNCTION_COORDINATE)) // when you get , and  --- want it on the and (for type) and not the comma so last comma is ignored
					{
						unsigned int  result = HandleCoordConjunct(i,changed);
						if (result)
						{
							--i;
							continue;
						}
						lastConjunction = i;

						if ((roles[i] & CONJUNCT_KINDS) == CONJUNCT_SENTENCE) // start a new sentence now, after cleanup of old sentence
						{
							if (!FinishSentenceAdjust(false,changed,startSentence,i-1)) break;
							startSentence = i+1;
							InitRoleSentence(startSentence,wordCount);
						}
					}
				}
				else 
				{
					needRoles[roleIndex] &= -1 ^ (INDIRECTOBJECT2 | OBJECT2);
				}
			} 
			break;
			case TO_INFINITIVE:
				// adverb verbal to express purpose of verb (why) but looks like adjective verbal as well:  "he bought flowers *to give his wife"
				// after verbs of thinking and feeling and saying : "he decided *to eat"
				// some (TO_INFINITIVE_OBJECT_verbs) verbs take direct object + to infinitive: "he encouraged his friends *to vote"  as object complement
				// some subject complement adjectives take it to give a reason (~adjectivecomplement_taking_noun_infinitive) : " he was unhappy *to live" also treated as object complement
				{
					currentVerb2 = 0; // we will be seeing a new current verb
					ImpliedNounObjectPeople(i-1, needRoles[roleIndex] & (MAINOBJECT|OBJECT2),changed);
					if (posValues[NextPos(i)] & NOUN_INFINITIVE)
					{
						LimitValues(i+1,NOUN_INFINITIVE,(char*)"following TO immediate noun infinitive",changed);
						crossReference[i] = (unsigned char) i+1;
					}
					else if (posValues[Next2Pos(i)] & NOUN_INFINITIVE)
					{
						LimitValues(i+2,NOUN_INFINITIVE,(char*)"following TO delayed noun infinitive",changed);
						crossReference[i] = (unsigned char) i+2;
					}			
				}
				break;
			case FOREIGN_WORD:
				break;
			case DETERMINER: case PREDETERMINER:
				needRoles[roleIndex] &= -1 ^ SUBJECT_COMPLEMENT;
		
				// cancels direct object if adverb after verb before det
				if (posValues[i-1] & ADVERB && posValues[i-2] & VERB_BITS && needRoles[roleIndex] & (OBJECT2|MAINOBJECT) && tokenControl & TOKEN_AS_IS) needRoles[roleIndex] &= -1 ^ (OBJECT2|MAINOBJECT|MAININDIRECTOBJECT|INDIRECTOBJECT2);
				break;
			case PRONOUN_POSSESSIVE: case POSSESSIVE:
				needRoles[roleIndex] &= -1 ^ SUBJECT_COMPLEMENT;
				break;
			case THERE_EXISTENTIAL:
				SetRole(i,MAINSUBJECT); // have to overrule it later
				break;
			case PAREN:
				break;
			default:  // UNKNOWN COMBINATIONS (undecided values) 
			{
				if (posValues[i] & (NOUN_BITS|VERB_BITS)) return false;	// unresolved noun or verb conflict
			}
		}// end of switch
		if (bitCounts[i] != 1)	// now still confused about futures....dont risk altering things later on except if we are hunting for a noun
		{
			// if conflict is merely adjective vs adverb, particle, etc, ignore it for now since its outcome doesn't determine much
			if (posValues[i] & (NOUN_BITS|VERB_BITS)) return false;
		}
		CloseLevel(i);

		// see if we can assign a pending verbal
		if (determineVerbal && !phrases[i] && !clauses[i] && !verbals[i] && wordStarts[i][0] != ',') // we are back to main sentence
		{
			if (bitCounts[i] != 1) {;} // we dont know what this is
			else if (posValues[i] & (NOUN_BITS | PRONOUN_SUBJECT | PRONOUN_OBJECT)) // we find a noun, it must have described it:  "Walking home, he left me"
			{
				LimitValues(determineVerbal,ADJECTIVE_PARTICIPLE,(char*)"says verbal is adjective",changed);
			}
			else if (posValues[i] & (AUX_VERB | VERB_BITS)) // we find a verb, it must have been the subject: "Walking home is fun"
			{
				LimitValues(determineVerbal,NOUN_GERUND,(char*)"says verbal is gerund",changed);
				SetRole(determineVerbal,SUBJECT2);
			}
			determineVerbal = 0;
		}
	}

	if (trace & TRACE_POS)
	{
		for (unsigned int x = roleIndex; x >= 1; --x)
		{
			DecodeneedRoles(x,goals);
			Log(STDTRACELOG,(char*)"      - leftover want %d: %s\r\n",x,goals);
		}

		Log(STDTRACELOG,(char*)"        ->  subject:%s  verb:%s  indirectobject:%s  object:%s  lastclause@:%s  lastverbal@:%s\r\n",wordStarts[subjectStack[MAINLEVEL]],wordStarts[verbStack[MAINLEVEL]],wordStarts[indirectObjectRef[currentMainVerb]],wordStarts[objectRef[currentMainVerb]],wordStarts[lastClause],wordStarts[lastVerbal]);
		Log(STDTRACELOG,(char*)"PreFinishSentence: ");
		for ( int i = startSentence; i <= endSentence; ++i)
		{
			if (ignoreWord[i]) continue;	// ignore these
			char word[MAX_WORD_SIZE];
			char* role = GetRole(roles[i]);
			if (!*role) 
			{
				role = "";
				if (bitCounts[i] != 1) 
				{
					*word = 0; 
					Tags(word,i);
					role = word;
				}
			}
			Log(STDTRACELOG,(char*)"%s (%s) ",wordStarts[i],role);
		}
		Log(STDTRACELOG,(char*)"\r\nZones: ");
		for (unsigned int i = 0; i < zoneIndex; ++i) ShowZone(i);
		Log(STDTRACELOG,(char*)"\r\n");
	}

	bool resolved = true;
 	if (!FinishSentenceAdjust(resolved,changed,startSentence,endSentence))
	{
		startSentence = oldStart; // in case we ran a coordinating conjunction
		ambiguous = 1;
		return false;
	}
	ValidateSentence(resolved);
	if (!resolved && !changed && noReact) (*printer)((char*)"input: %s\r\n",currentInput); //for debugging shows what sentences failed to be resolved by parser acceptibly
	startSentence = oldStart; // in case we ran a coordinating conjunction
	return resolved;
}

void ParseSentence(bool &resolved,bool &changed)
{
	if (!idiomed && ignoreRule == -1)
	{
		WordIdioms(changed); // mark idioms after using normal words for rule processing but as part of PENN matching so we can accept all sorts of junk from penn
		idiomed = true;
		resolved = false;
		if (changed) return; // allow one more pass before assigning roles
	}
	if ((tokenControl & DO_PARSE) == DO_PARSE)
	{
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"\r\n%s",DumpAnalysis(startSentence,endSentence,posValues,(char*)"POS",false,false));
		unsigned int start = startSentence;
		unsigned int end = endSentence;
		resolved =  AssignRoles(changed); 
		startSentence = start; // parsing can change startSentence when it is parsing a dual sentence (conjunctino)
		endSentence = end;
	}
	else
	{
		if (trace & TRACE_POS) Log(STDTRACELOG,(char*)"\r\nNot trying to parse\r\n");
	}
}
#endif
