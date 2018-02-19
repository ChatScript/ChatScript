#ifndef COMMON1H
#define COMMON1H

typedef unsigned long long int  uint64;
typedef signed long long  int64;
#define ALWAYS (1 == always)

#define MAX_ARGUMENT_COUNT 400 //  assume 10 args 40 nested calls max. 
extern char* callArgumentList[MAX_ARGUMENT_COUNT+1];    //   function callArgumentList
extern int callArgumentBase;
extern int fnVarbase;

#define ARGUMENT(n) callArgumentList[callArgumentBase+n]
#define FNVAR(n) callArgumentList[fnVarbase+atoi(n)+1] // ^0 is index 1
char* ReadCompiledWord(char* ptr, char* word,bool noquote = false,bool var = false,bool nolimit = false);
char* ReadCompiledWordOrCall(char* ptr, char* word,bool noquote = false,bool var = false);

#define INPUT_BUFFER_SIZE   80000
#define MAX_BUFFER_SIZE		80000

#define NUMBER_OF_LAYERS 4

typedef unsigned int MEANING;					//   a flagged indexed dict ptr
#define MAX_DICTIONARY	 0x001fffff				//   2M word vocabulary limit 
#define NODEBITS 0x00ffffff
#define MULTIWORDHEADER_SHIFT 24
#define MULTIHEADERBITS 0xFF000000

#define MAX_MEANING			63
#define MEANING_BASE		0x001fffff	//   the index of the dictionary item 
#define SYNSET_MARKER		0x00200000  // this meaning is a synset head - on keyword import, its quote flag for binary read
#define INDEX_BITS          0x0fC00000  //   6 bits of ontology meaning indexing ability  63 possible meanings allowed
#define INDEX_MINUS			0x00400000  // what to decrement to decrement the meaning index
#define INDEX_OFFSET        22          //   shift for ontoindex  (rang 0..63)  
#define TYPE_RESTRICTION	0xf0000000  // corresponds to noun,verb,adj,adv  (cannot merge adj/adv or breaks wordnet dictionary data) 
#define TYPE_RESTRICTION_SHIFT 0

// A meaning = TYPE_RESTRICTION(4 bit) + INDEX_BITS (6 bits) + MEANING_BASE(21 bits) + SYNSET_MARKER(1 bit)

#define SYSVAR_PREFIX '%'
#define MATCHVAR_PREFIX '_'
#define USERVAR_PREFIX '$'
#define TRANSIENTVAR_PREFIX '$'
#define LOCALVAR_PREFIX '_'
#define FACTSET_PREFIX '@'
#define FUNCTIONVAR_PREFIX '^'
#define TOPICCONCEPT_PREFIX '~'

#define BIG_WORD_SIZE   10000
#define MAX_WORD_SIZE   3000   
#define SMALL_WORD_SIZE  500  

#undef WORDP //   remove windows version (unsigned short) for ours

//   DoFunction results
 enum FunctionResult {
	NOPROBLEM_BIT = 0,
	ENDRULE_BIT = 0x00000001,
	FAILRULE_BIT  = 0x00000002,

	RETRYRULE_BIT =  0x00000004,
	RETRYTOPRULE_BIT =  0x00000008,

	ENDTOPIC_BIT =  0x00000010,
	FAILTOPIC_BIT  = 0x00000020,
	RETRYTOPIC_BIT  = 0x00000040,

	ENDSENTENCE_BIT =  0x00000080,
	FAILSENTENCE_BIT =  0x00000100,
	RETRYSENTENCE_BIT =  0x00000200,

	ENDINPUT_BIT  = 0x00000400,
	FAILINPUT_BIT  = 0x00000800,
	RETRYINPUT_BIT = 0x00001000,

	FAILMATCH_BIT  = 0x00002000,			// transient result of TestRule, converts to FAILRULE_BIT
	FAILLOOP_BIT  = 0x00004000,
	ENDLOOP_BIT  = 0x00008000,

	UNDEFINED_FUNCTION = 0x00010000, //   potential function call has no definition so isnt
	ENDCALL_BIT  =		0x00020000,
	NEXTLOOP_BIT =		0x00040000,
	RESTART_BIT =	0x00080000
};
 FunctionResult JavascriptArgEval(unsigned int index, char* buffer);
#endif
