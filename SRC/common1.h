#ifndef COMMON1H
#define COMMON1H

typedef unsigned long long int  uint64;
typedef signed long long  int64;
#define ALWAYS (1 == always)

#define ARGUMENT(n)  currentCallFrame->arguments[n] 
#define FNVAR(n) currentFNVarFrame->arguments[atoi(n+1)] // ^1 is index 1
#define QUOTEDFNVAR(n) currentFNVarFrame->arguments[atoi(n+2)] // ^1 is index 1
char* ReadCompiledWord(const char* ptr, char* word,bool noquote = false,bool var = false,bool nolimit = false);
char* ReadCompiledWordOrCall(char* ptr, char* word,bool noquote = false,bool var = false);
char* ReadCodeWord(const char* ptr, char* word, bool noquote = false, bool var = false, bool nolimit = false);
#define MAX_BUFFER_SIZE		80000 // default

#define NUMBER_OF_LAYERS 4 // 0,1,boot,user

#define MAX_CONFIG_LINES 200

typedef unsigned int MEANING;					//   a flagged indexed dict ptr
#define MAX_DICTIONARY	 0x003fffff	//   4M word vocabulary limit 


// A meaning = TYPE_RESTRICTION(4 bit) + INDEX_BITS (5 bits) + SYNSET_MARKER(1 bit) + MEANING_BASE(22 bits) 
#define MEANING_BASE		MAX_DICTIONARY 	//   the index of the dictionary item 
#define MAX_MEANING			31  // limit on index_bits break has around 64.
#define INDEX_OFFSET          22          //   shift for ontoindex  (range 0..31)  
#define TYPE_RESTRICTION	0xf0000000  // corresponds to noun,verb,adj,adv  (cannot merge adj/adv or breaks wordnet dictionary data) 
#define SYNSET_MARKER		0x08000000  // this meaning is a synset head - on keyword import, its quote flag for binary read
#define INDEX_BITS				0x07c00000  //   5 bits of ontology meaning indexing ability  31 possible meanings allowed, generic uses value 0
#define INDEX_MINUS			0x00400000  // what to decrement to decrement the meaning index
#define TYPE_RESTRICTION_SHIFT 0

#define SYSVAR_PREFIX '%'
#define MATCHVAR_PREFIX '_'
#define USERVAR_PREFIX '$'
#define TRANSIENTVAR_PREFIX '$'
#define LOCALVAR_PREFIX '_'
#define FACTSET_PREFIX '@'
#define INDIRECTION_PREFIX '^'
#define FUNCTION_PREFIX '^'
#define FUNCTIONVAR_PREFIX '^'
#define TOPICCONCEPT_PREFIX '~'
#define INDIRECT_PREFIX '^'

#define BIG_WORD_SIZE   10000
#define MAX_WORD_SIZE   3000   
#define SMALL_WORD_SIZE  500  

#undef WORDP //   remove windows version (unsigned short) for ours

enum CompileStatus {
	 NOT_COMPILING = 0,
	FULL_COMPILE = 1,
	PIECE_COMPILE = 2,
	CONCEPTSTRING_COMPILE = 3
};

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
	RESTART_BIT =	0x00080000,
	FAILTOPRULE_BIT = 0x00100000,
	ENDTOPRULE_BIT = 0x00200000

};
 FunctionResult JavascriptArgEval(unsigned int index, char* buffer);
#endif
