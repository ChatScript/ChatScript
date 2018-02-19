#include "common.h"

int startSentence;
int endSentence;
bool fullfloat = false;	// 2 float digits or all
int numberStyle = AMERICAN_NUMBERS;
FILE* docOut = NULL;
bool showBadUTF = false;			// log bad utf8 characer words
static bool blockComment = false;
bool singleSource = false;			// in ReadDocument treat each line as an independent sentence
bool newline = false;
int docSampleRate = 0;
int docSample = 0;
uint64 docVolleyStartTime = 0;
bool hasHighChar = false;
char conditionalCompile[MAX_CONDITIONALS+1][50];
int conditionalCompiledIndex = 0;
char numberComma = ',';
char numberPeriod = '.';

char tmpWord[MAX_WORD_SIZE];					// globally visible scratch word
char* userRecordSourceBuffer = 0;				// input source for reading is this text stream of user file

int BOM = NOBOM;								// current ByteOrderMark
static char holdc = 0;		//   holds extra character from readahead

unsigned char utf82extendedascii[128];
unsigned char extendedascii2utf8[128] =
{
	0,0xbc,0xa9,0xa2,0xa4,0xa0,0xa5, 0xa7,0xaa,0xab,	0xa8,0xaf,0xae,0xac,0x84,0x85,0x89,0xa6,0x86,0xb4,  
	0xb6,0xb2,0xbb,0xb9,0xbf,0x96,0x9c,0xa2,0xa3,0xa5,	0x00,0xa1,0xad,0xb3,0xba,0xb1,0x91,
};

typedef struct NUMBERDECODE
{
	int word;		//   word of a number index
	unsigned int length;	//   length of word
	int64 value;				//   value of word
	int realNumber;			// the type, one two are real, third is fraction
} NUMBERDECODE;

static NUMBERDECODE* numberValues;

typedef struct CURRENCYDECODE
{
	int word; // word index of currency abbrev
	int concept;	// word index of concept to mark it
} CURRENCYDECODE;

static CURRENCYDECODE* currencies;
static int* monthnames;

char toHex[16] = {
	'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

unsigned char toLowercaseData[256] = // convert upper to lower case
{
	0,1,2,3,4,5,6,7,8,9,			10,11,12,13,14,15,16,17,18,19,
	20,21,22,23,24,25,26,27,28,29,	30,31,32,33,34,35,36,37,38,39,
	40,41,42,43,44,45,46,47,48,49,	50,51,52,53,54,55,56,57,58,59,
	60,61,62,63,64,'a','b','c','d','e',
	'f','g','h','i','j','k','l','m','n','o',
	'p','q','r','s','t','u','v','w','x','y',
	'z',91,92,93,94,95,96,97,98,99,				100,101,102,103,104,105,106,107,108,109,
	110,111,112,113,114,115,116,117,118,119,	120,121,122,123,124,125,126,127,128,129,
	130,131,132,133,134,135,136,137,138,139,	140,141,142,143,144,145,146,147,148,149,
	150,151,152,153,154,155,156,157,158,159,	160,161,162,163,164,165,166,167,168,169,
	170,171,172,173,174,175,176,177,178,179,	180,181,182,183,184,185,186,187,188,189,
	190,191,192,193,194,195,196,197,198,199,	200,201,202,203,204,205,206,207,208,209,
	210,211,212,213,214,215,216,217,218,219,	220,221,222,223,224,225,226,227,228,229,
	230,231,232,233,234,235,236,237,238,239,	240,241,242,243,244,245,246,247,248,249,
	250,251,252,253,254,255
};

unsigned char toUppercaseData[256] = // convert lower to upper case
{
	0,1,2,3,4,5,6,7,8,9,			10,11,12,13,14,15,16,17,18,19,
	20,21,22,23,24,25,26,27,28,29,	30,31,32,33,34,35,36,37,38,39,
	40,41,42,43,44,45,46,47,48,49,	50,51,52,53,54,55,56,57,58,59,
	60,61,62,63,64,'A','B','C','D','E',
	'F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y',
	'Z',91,92,93,94,95,96,'A','B','C',			'D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W',	'X','Y','Z',123,124,125,126,127,128,129,
	130,131,132,133,134,135,136,137,138,139,	140,141,142,143,144,145,146,147,148,149,
	150,151,152,153,154,155,156,157,158,159,	160,161,162,163,164,165,166,167,168,169,
	170,171,172,173,174,175,176,177,178,179,	180,181,182,183,184,185,186,187,188,189,
	190,191,192,193,194,195,196,197,198,199,	200,201,202,203,204,205,206,207,208,209,
	210,211,212,213,214,215,216,217,218,219,	220,221,222,223,224,225,226,227,228,229,
	230,231,232,233,234,235,236,237,238,239,	240,241,242,243,244,245,246,247,248,249,
	250,251,252,253,254,255
};

unsigned char isVowelData[256] = // english vowels
{
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, 
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, 
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, 
	0,0,0,0,0,'a',0,0,0,'e', 0,0,0,'i',0,0,0,0,0,'o', 
	0,0,0,0,0,'u',0,0,0,'y', 0,0,0,0,0,0,0,'a',0,0, 
	0,'e',0,0,0,'i',0,0,0,0, 0,'o',0,0,0,0,0,'u',0,0,
	0,'y',0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0
};

signed char nestingData[256] = // the matching bracket things: () [] {}
{
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	1,-1,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,  //   () 
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,1,0,-1,0,0,0,0,0,0,  //   [  ]
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,1,0,-1,0,0,0,0, //   { }
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0
};

unsigned char legalNaming[256] = // what we allow in script-created names (like ~topicname or $uservar)
{
	0,0,0,0,0,0,0,0,0,0,			0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,			0,0,0,0,0,0,0,0,0,'\'',
	0,0,0,0,0,'-','.',0,'0','1',	'2','3','4','5','6','7','8','9',0,0,
	0,0,0,0,0,'A','B','C','D','E', 	'F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y', 	'Z',0,0,0,0,'_',0,'A','B','C',			
	'D','E','F','G','H','I','J','K','L','M',
	'N','O','P','Q','R','S','T','U','V','W',	'X','Y','Z',0,0,0,0,0,1,1,
	1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,			1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,
};

unsigned char punctuation[256] = //   direct lookup of punctuation --   //    / is normal because can be  a numeric fraction
{
	ENDERS,0,0,0,0,0,0,0,0,SPACES, //   10  null  \t
	SPACES,0,0,SPACES,0,0,0,0,0,0, //   20  \n \r
	0,0,0,0,0,0,0,0,0,0, //   30
	0,0,SPACES,ENDERS,QUOTERS,SYMBOLS,SYMBOLS,ARITHMETICS,CONVERTERS,QUOTERS, //   40 space  ! " # $ % & '
	BRACKETS,BRACKETS,ARITHMETICS|QUOTERS , ARITHMETICS,PUNCTUATIONS, ARITHMETICS|ENDERS,ARITHMETICS|ENDERS,ARITHMETICS,0,0, //   () * + ,  - .  /
	0,0,0,0,0,0,0,0,ENDERS,ENDERS, //   60  : ;
	BRACKETS,ARITHMETICS,BRACKETS,ENDERS,SYMBOLS,0,0,0,0,0, //   70 < = > ? @
	0,0,0,0,0,0,0,0,0,0, //   80
	0,0,0,0,0,0,0,0,0,0, //   90
	0,BRACKETS,0,BRACKETS,ARITHMETICS,0,CONVERTERS,0,0,0, //   100  [ \ ] ^  `   _ is normal  \ is unusual
	0,0,0,0,0,0,0,0,0,0, //   110
	0,0,0,0,0,0,0,0,0,0, //   120
	0,0,0,BRACKETS,PUNCTUATIONS,BRACKETS,SYMBOLS,0,0,0, //   130 { |  } ~
	0,0,0,0,0,0,0,0,0,0, //   140
	0,0,0,0,0,0,0,0,0,0, //   150
	0,0,0,0,0,0,0,0,0,0, //   160
	0,0,0,0,0,0,0,0,0,0, //   170
	0,0,0,0,0,0,0,0,0,0, //   180
	0,0,0,0,0,0,0,0,0,0, //   190
	0,0,0,0,0,0,0,0,0,0, //   200
	0,0,0,0,0,0,0,0,0,0, //   210
	0,0,0,0,0,0,0,0,0,0, //   220
	0,0,0,0,0,0,0,0,0,0, //   230
	0,0,0,0,0,0,0,0,0,0, //   240
	0,0,0,0,0,0,0,0,0,0, //   250
	0,0,0,0,0,0, 
};

unsigned char realPunctuation[256] = // punctuation characters
{
	0,0,0,0,0,0,0,0,0,0, //   10  
	0,0,0,0,0,0,0,0,0,0, //   20  
	0,0,0,0,0,0,0,0,0,0, //   30
	0,0,0,PUNCTUATIONS,0,0,0,0,0,0, //   40   ! 
	0,0,0 , 0,PUNCTUATIONS, 0,PUNCTUATIONS,0,0,0, //    ,   .  
	0,0,0,0,0,0,0,0,PUNCTUATIONS,PUNCTUATIONS, //   60  : ;
	0,0,0,PUNCTUATIONS,0,0,0,0,0,0, //  ? 
	0,0,0,0,0,0,0,0,0,0, //   80
	0,0,0,0,0,0,0,0,0,0, //   90
	0,0,0,0,0,0,0,0,0,0, //   100  
	0,0,0,0,0,0,0,0,0,0, //   110
	0,0,0,0,0,0,0,0,0,0, //   120
	0,0,0,0,0,0,0,0,0,0, //   130 
	0,0,0,0,0,0,0,0,0,0, //   140
	0,0,0,0,0,0,0,0,0,0, //   150
	0,0,0,0,0,0,0,0,0,0, //   160
	0,0,0,0,0,0,0,0,0,0, //   170
	0,0,0,0,0,0,0,0,0,0, //   180
	0,0,0,0,0,0,0,0,0,0, //   190
	0,0,0,0,0,0,0,0,0,0, //   200
	0,0,0,0,0,0,0,0,0,0, //   210
	0,0,0,0,0,0,0,0,0,0, //   220
	0,0,0,0,0,0,0,0,0,0, //   230
	0,0,0,0,0,0,0,0,0,0, //   240
	0,0,0,0,0,0,0,0,0,0, //   250
	0,0,0,0,0,0, 
};
  
unsigned char isAlphabeticDigitData[256] = //    non-digit number starter (+-.) == 1 isdigit == 2 isupper == 3 islower == 4 isletter >= 3 utf8 == 5
{
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,1,1,0,0,0,  //   # and $
	0,0,0,VALIDNONDIGIT,0,VALIDNONDIGIT,VALIDNONDIGIT,0,VALIDDIGIT,VALIDDIGIT,	VALIDDIGIT,VALIDDIGIT,VALIDDIGIT,VALIDDIGIT,VALIDDIGIT,VALIDDIGIT,VALIDDIGIT,VALIDDIGIT,0,0,		//   + and . and -
	0,0,0,0,0,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,
	VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,
	VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,VALIDUPPER,
	VALIDUPPER,0,0,0,0,0,0,4,4,4,
	VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,
	VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,VALIDLOWER,
	VALIDLOWER,VALIDLOWER,VALIDLOWER,0,0,0,0,0,  VALIDUTF8,VALIDUTF8,
	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,
	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,
	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,
	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,
	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,
	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,
	VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8,VALIDUTF8
};

unsigned int roman[256] = // values of roman numerals
{
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //20
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //40
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //60 
	0,0,0,0,0,0,0,100,500,0,	0,0,0,1,0,0,50,1000,0,0, //80  C(67)=100 D(68)=500 I(73)=1  L(76)=50  M(77)=1000 
	0,0,0,0,0,0,5,0,10,0,	0,0,0,0,0,0,0,0,0,0, //100 V(86)=5  X(88)=10

	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,  
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,  
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,  
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,  
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,  

	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,
};

unsigned char isComparatorData[256] = //    = < > & ? ! %
{
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //20
	0,0,0,0,0,0,0,0,0,0,	0,0,0,1,0,0,0,0,1,0, //40 33=! 38=&
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //60
	1,1,1,1,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //80 < = > ?
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //100
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0, //120
	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,	0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0
};

unsigned char decimalMarkData[3] = // American, Indian, French
{
	'.','.',','
};

unsigned char digitGroupingData[3] = // American, Indian, French
{
	',',',','.'
};

/////////////////////////////////////////////
// STARTUP
/////////////////////////////////////////////

void InitTextUtilities()
{
	memset(utf82extendedascii, 0, 128);
	extendedascii2utf8[0xe1 - 128] = 0x9f; // german fancy ss
	for (unsigned int i = 0; i < 128; ++i)
	{
		unsigned char c = extendedascii2utf8[i];
		if (c) c -= 128;	// remove top bit
		utf82extendedascii[c] = (unsigned char)i;
	}
}

bool IsComparator(char* word)
{
	return (!stricmp(word, (char*)"&=") || !stricmp(word, (char*)"|=") || !stricmp(word, (char*)"^=") || !stricmp(word, (char*)"=") || !stricmp(word, (char*)"+=") || !stricmp(word, (char*)"-=") || !stricmp(word, (char*)"/=") || !stricmp(word, (char*)"*="));
}

void ClearNumbers()
{
	numberValues = 0; // heap is discarded, need to discard this
	currencies = 0;
}

char* ReadTokenMass(char* ptr, char* word)
{
	ptr = SkipWhitespace(ptr);
	while (*ptr && *ptr != ' ') *word++ = *ptr++;	// find end of word
	*word = 0;
	return ptr;
}

void InitTextUtilities1()
{
	char file[SMALL_WORD_SIZE];
	sprintf(file, (char*)"%s/%s/numbers.txt", livedata, language);
	FILE* in = FopenStaticReadOnly(file); // LIVEDATA substitutes or from script TOPIC world
	if (!in) return;
	char word[MAX_WORD_SIZE];
	char value[MAX_WORD_SIZE];
	char kind[MAX_WORD_SIZE];
	char* limit;
	char* stack = InfiniteStack64(limit,"initnumbers");
	int* data = (int*)stack;

	while (ReadALine(readBuffer, in) >= 0) // once 1 REALNUMBER
	{
		if (*readBuffer == '#' || *readBuffer == 0) continue;
		char* ptr = ReadCompiledWord(readBuffer, word); 
		ptr = ReadCompiledWord(ptr, value);
		ptr = ReadCompiledWord(ptr, kind);
		int64 val;
		ReadInt64(value, val);
		int kindval = 0;
		if (!stricmp(kind, "DIGIT_NUMBER")) kindval = DIGIT_NUMBER;
		else if (!stricmp(kind, "FRACTION_NUMBER")) kindval = FRACTION_NUMBER;
		else if (!stricmp(kind, "WORD_NUMBER")) kindval = WORD_NUMBER;
		else if (!stricmp(kind, "REAL_NUMBER")) kindval = WORD_NUMBER;
		else myexit("Bad number table");
		char* w = StoreWord(word,AS_IS| ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER)->word;
		int index = Heap2Index(w);
		*data++ = index;
		*data++ = strlen(word);
		*(int64*)data = val; // int64 value 
		data += 2;
		*data++ = kindval;
		*data++ = 0; // alignment to keep block at int64 style
	}
	*data++ = 0;	 // terminal value for end detection
	size_t size = (char*)data - stack;
	size /= 8;	 // 64bit chunks
	numberValues = (NUMBERDECODE*)AllocateHeap(stack, size+1,8);
	ReleaseInfiniteStack();
	fclose(in);

	sprintf(file, (char*)"%s/%s/currencies.txt", livedata, language);
	in = FopenStaticReadOnly(file); 
	if (!in) return;
	stack = InfiniteStack64(limit, "initcurrency");
	data = (int*)stack;

	while (ReadALine(readBuffer, in) >= 0) // $ ~dollar
	{
		if (*readBuffer == '#' || *readBuffer == 0) continue;
		char* ptr = ReadCompiledWord(readBuffer, word);
		MakeLowerCase(word);
		ptr = ReadCompiledWord(ptr, kind);
		MakeLowerCase(kind);
		if (*kind != '~') myexit("Bad currency table");
		char* w = StoreWord(word, AS_IS | CURRENCY)->word; // currency name
		int index = Heap2Index(w);
		*data++ = index;
		w = StoreWord(kind, AS_IS)->word; // currency name
		index = Heap2Index(w);
		*data++ = index;
	}
	*data++ = 0;	 // terminal value for end detection
	size = (char*)data - stack;
	size /= 8;	 // 64bit chunks
	currencies = (CURRENCYDECODE*)AllocateHeap(stack, size + 1, 8);
	ReleaseInfiniteStack();
	fclose(in);

	sprintf(file, (char*)"%s/%s/months.txt", livedata, language);
	in = FopenStaticReadOnly(file); 
	if (!in) return;
	stack = InfiniteStack64(limit, "initmonths");
	data = (int*)stack;
	while (ReadALine(readBuffer, in) >= 0) // month name or abbrev
	{
		if (*readBuffer == '#' || *readBuffer == 0) continue;
		char* ptr = ReadCompiledWord(readBuffer, word);
		char* w = StoreWord(word, AS_IS)->word; // month name
		int index = Heap2Index(w);
		*data++ = index;
	}
	*data++ = 0;	 // terminal value for end detection
	size = (char*)data - stack;
	size /= 8;	 // 64bit chunks
	monthnames = (int*)AllocateHeap(stack, size + 1, 8);
	ReleaseInfiniteStack();
	fclose(in);
}

bool IsDate(char* original)
{
	// Jan-26-2017 or 2017/MAY/26   also jan-02  or jan-2017  or 2017-jan or 02-jan BUT NOT 02-2017
	// 24/15/2007 etc
	size_t len = strlen(original);
	if (!IsDigit(original[0]) && !IsDigit(original[len - 1])) return false;
	// date must either start or end with a digit. 

	char separator = 0;
	int digitcount = 0;
	bool digit4 = false;
	int separatorcount = 0;
	char* alpha = NULL;
	int alphacount = 0;
	bool acceptalpha = true;
	--original;
	while (*++original)
	{
		if (IsDigit(*original))
		{
			if (acceptalpha == false) return false; // cannot mix alpha and digit
			++digitcount; // count digit block size
		}
		else if (IsAlphaUTF8(*original) && *original != '-' && *original != '.' && *original != '/')
		{
			if (acceptalpha)
			{
				alpha = original; // 1st one seen
				acceptalpha = false;
			}
		}
		else if (!separator) separator = *original;
		else if (*original != separator) return false; // cannot be

		if (*original == separator) // start a new block
		{
			if (digitcount == 4)  // only 1 year is allowed
			{
				if (digit4) return false; // only 1 of these
				digit4 = true;
			}
			else if (digitcount > 2) return false; // not legal count, must be 1,2 or 4 digits

			if (acceptalpha == false && ++alphacount  > 1) return false;	// only one text month is allowed
			acceptalpha = true;

			digitcount = 0;
			if (++separatorcount > 2) return false; // only 2 separators (3 components) allowed
		}
	}
	if (separator != '/'  && separator != '-' && separator != '.') return false;
	if (!alpha)
	{
		return (separatorcount == 2);
	}

	if (separatorcount < 1) return false; // 2 or 3 allowed

										  // confirm its a month
	int* name = monthnames;
	for (int i = 0; i < 1000; ++i)
	{
		if (!*name) return false;
		char* w = Index2Heap(*name);
		name++;
		if (*w != toUppercaseData[*alpha]) continue;
		size_t n = strlen(w);
		if (strnicmp(w, alpha, n)) continue;
		if (alpha[n] == 0 || alpha[n] == separator) return true;
	}
	return false;
}

void CloseTextUtilities()
{
}

bool IsFraction(char* token)
{
	if (IsDigit(token[0])) // fraction?
	{
		char* at = strchr(token,'/');
		if (at)
		{
			at = token;
			while (IsDigit(*++at)) {;}
			if (*at == '/' && (at[1] != '0' || at[2]))
			{
				while (IsDigit(*++at)) {;}
				if (!*at ) return true;
			}
		}
	}
	return false;
}

char* RemoveEscapesWeAdded(char* at)
{
	if (!at || !*at) return at;
	char* startScan = at;
	while ((at = strchr(at,'\\'))) // we always add ESCAPE_FLAG backslash   
	{
		if (*(at-1) == ESCAPE_FLAG) // alter the escape in some way
		{
			if (at[1] == 't')  at[1] = '\t'; // legal 
			else if (at[1] == 'n') at[1] = '\n';  // legal 
			else if (at[1] == 'r')  at[1] = '\r';  // legal 
			// other choices dont translate, eg double quote
			memmove(at-1,at+1,strlen(at)); // remove the escape flag and the escape
		}
		else ++at; // we dont mark " with ESCAPE_FLAG, we are not protecting it.
	}
	return startScan;
}

char* CopyRemoveEscapes(char* to, char* at,int limit,bool all) // all includes ones we didnt add as well
{
	*to = 0;
	char* start = to;
	if (!at || !*at) return to;
	// need to remove escapes we put there
	--at;
	while (*++at)
	{
		if (*at == ESCAPE_FLAG && at[1] == '\\') // we added an escape here
		{
			at += 2; // move to escaped item
			if (*(at-1) == ESCAPE_FLAG) // dual escape is special marker, there is no escaped item
			{
				*to++ = '\r';  // legal 
				*to++ = '\n';  // legal 
				--at; // rescan the item
			}
			else if (*at == 't') *to++ = '\t';  // legal 
			else if (*at == 'n') *to++ = '\n';  // legal 
			else if (*at == 'r') *to++ = '\r';  // legal 
			else *to++ = *at; // remove our escape pair and pass the item
		}
		else if (*at == '\\' && all) // remove ALL other escapes in addition to ones we put there
		{
			++at; // move to escaped item
			if (*at  == 't') *to++ = '\t';
			else if (*at  == 'n') *to++ = '\n';  // legal 
			else if (*at  == 'r') *to++ = '\r';  // legal 
			else {*to++ = *at; } // just pass along untranslated
		}
		else *to++ = *at;
		if ((to-start) >= limit) // too much, kill it
		{
			*to = 0;
			break;
		}
	}
	*to = 0;
	return start;
}

void RemoveImpure(char* buffer)
{
	char* p;
	while ((p = strchr(buffer,'\r'))) *p = ' '; // legal
	while ((p = strchr(buffer,'\n'))) *p = ' '; // legal
	while ((p = strchr(buffer,'\t'))) *p = ' '; // legal
}

void ChangeSpecial(char* buffer)
{
	char* limit;
	char* buf = InfiniteStack(limit,"ChangeSpecial");
	AddEscapes(buf,buffer,true,MAX_BUFFER_SIZE);
	strcpy(buffer,buf);
	ReleaseInfiniteStack();
}

char* AddEscapes(char* to, char* from, bool normal,int limit) // normal true means dont flag with extra markers
{
	limit -= 200; // dont get close to limit
	char* start = to;
	char* at = from - 1;
	// if we NEED to add an escape, we have to mark it as such so  we know to remove them later.
	while (*++at)
	{
		// convert these
		if (*at == '\n') { // not expected to see
			if (!normal) *to++ = ESCAPE_FLAG; 
			*to++ = '\\'; 
			*to++ = 'n'; // legal
		}
		else if (*at == '\r') {// not expected to see
			if (!normal) *to++ = ESCAPE_FLAG; 
			*to++ = '\\'; 
			*to++ = 'r';// legal
		} 
		else if (*at == '\t') {// not expected to see
			if (!normal) *to++ = ESCAPE_FLAG; 
			*to++ = '\\'; 
			*to++ = 't'; // legal
		}
		else if (*at == '"') { // we  need to preserve that it was escaped, though we always escape it in json anyway, because writeuservars needs to know
			if (!normal) *to++ = ESCAPE_FLAG; 
			*to++ = '\\'; 
			*to++ = '"';
		}
		// detect it is already escaped
		else if (*at == '\\')
		{
			char* at1 = at + 1;
			if (*at1 && (*at1 == 'n' || *at1 == 'r' || *at1 == 't' || *at1 == '"' || *at1 == 'u' || *at1 == '\\'))  // just pass it along
			{
				*to++ = *at;
				*to++ = *++at;
			}
			else { 
				if (!normal) *to++ = ESCAPE_FLAG; 
				*to++ = '\\'; 
				*to++ = '\\'; 
			}
		}
		// no escape needed
		else *to++ = *at;
		if ((to-start) > limit && false) 	// dont overflow just abort silently
		{
			ReportBug((char*)"AddEscapes overflowing buffer");
			break;
		}
	}
	*to = 0;
	return to; // return where we ended
}

double Convert2Float(char* original, int useNumberStyle)
{
	char decimalMark = decimalMarkData[useNumberStyle];
	char digitGroup = digitGroupingData[useNumberStyle];
	char num[MAX_WORD_SIZE];
	char* numloc = num;
	strcpy(num, original);
	char* comma;
	while ((comma = strchr(num, digitGroup))) memmove(comma, comma + 1, strlen(comma));
	char* period = strchr(num, decimalMark);
	if (period) *period = '.'; // force us period

	char* currency1;
	char* cur1 = (char*)GetCurrency((unsigned char*)numloc, currency1); // alpha part
	if (cur1 && cur1 == numloc) numloc = currency1; // point to number part
	double val = atof(numloc);
	if (IsDigitWithNumberSuffix(numloc, useNumberStyle)) // 10K  10M 10B
	{
		size_t len = strlen(numloc);
		char d = numloc[len - 1];
		if (d == 'k' || d == 'K') val *= 1000;
		else if (d == 'm' || d == 'M') val *= 1000000;
		else if (d == 'B' || d == 'b' || d == 'G' || d == 'g') val *= 1000000000;
	}
	return val;
}

void AcquireDefines(char* fileName)
{ // dictionary entries:  `xxxx (property names)  ``xxxx  (systemflag names)  ``` (parse flags values)  -- and flipped:  `nxxxx and ``nnxxxx and ```nnnxxx with infermrak being ptr to original name
	FILE* in = FopenStaticReadOnly(fileName); // SRC/dictionarySystem.h
	if (!in) 
	{
		if ( !server) (*printer)((char*)"%s", (char*)"Unable to read dictionarySystem.h\r\n");
		return;
	}
	char label[MAX_WORD_SIZE];
	char word[MAX_WORD_SIZE];
    bool orop = false;
    bool shiftop = false;
	bool plusop = false;
	bool minusop = false;
	bool timesop = false;
	bool excludeop = false;
	int offset = 1;
	word[0] = ENDUNIT;
	bool endsystem = false;
	while (ReadALine(readBuffer, in) >= 0)
	{
		uint64 result = NOPROBLEM_BIT;
        int64 value;
		if (!strnicmp(readBuffer,(char*)"// system flags",15))  // end of property flags seen
		{
			word[1] = ENDUNIT; // system flag words have `` in front
			offset = 2;
		}
		else if (!strnicmp(readBuffer,(char*)"// end system flags",19))  // end of system flags seen
		{
			offset = 1;
		}
		else if (!strnicmp(readBuffer,(char*)"// parse flags",14))  // start of parse flags seen
		{
			word[1] = ENDUNIT; // parse flag words have ``` in front
			word[2] = ENDUNIT; // parse flag words have ``` in front
			offset = 3;
		}
		else if (!strnicmp(readBuffer,(char*)"// end parse flags",18))  // end of parse flags seen
		{
			word[1] = ENDUNIT; // misc flag words have ```` in front and do not xref from number to word
			word[2] = ENDUNIT; // misc flag words have ```` in front
			word[3] = ENDUNIT; // misc flag words have ```` in front
			offset = 4;
			endsystem = true;
		}

		char* ptr = ReadCompiledWord(readBuffer,word+offset);
		if (stricmp(word+offset,(char*)"#define")) continue;

		//   accept lines line #define NAME 0x...
		ptr = ReadCompiledWord(ptr,word+offset); //   the #define name 
        if (ptr == 0 || *ptr == 0 || strchr(word+offset,'(')) continue; // if a paren is IMMEDIATELY attached to the name, its a macro, ignore it.
		while (ptr) //   read value of the define
        {
            ptr = SkipWhitespace(ptr);
            if (ptr == 0) break;
			char c = *ptr++;
			if (!c) break;
            if (c == ')' || c == '/') break; //   a form of ending

			if (c == '+' && *ptr == ' ') plusop = true;
			else if (c == '-' && *ptr == ' ') minusop = true;
  			else if (c == '*' && *ptr == ' ') timesop = true;
			else if (c == '^' && *ptr == ' ') excludeop = true;
            else if (IsDigit(c))
            {
				ptr = (*ptr == 'x' || *ptr == 'X') ? ReadHex(ptr-1,(uint64&)value) : ReadInt64(ptr-1,value); 
                if (plusop) result += value;
                else if (minusop) result -= value;
                else if (timesop) result *= value;
                else if (excludeop) result ^= value;
	            else if (orop) result |= value;
                else if (shiftop) result <<= value;
                else result = value;
                excludeop = plusop = minusop = orop = shiftop = timesop = false;
            }
			else if (c == '(');	//   start of a (expression in a define, ignore it
            else if (c == '|')  orop = true;
            else if (c == '<') //    <<
            {
                ++ptr; 
                shiftop = true;
            }
           else //   reusing word
            {
                ptr = ReadCompiledWord(ptr-1,label);
                value = FindValueByName(label);
				bool olddict = xbuildDictionary;
				xbuildDictionary = false;
                if (!value)  value = FindSystemValueByName(label);
	            if (!value)  value = FindMiscValueByName(label);
	            if (!value)  value = FindParseValueByName(label);
				xbuildDictionary = olddict;
				if (!value)  ReportBug((char*)"missing modifier value for %s\r\n",label)
                if (orop) result |= value;
                else if (shiftop) result <<= value;
				else if (plusop) result += value;
				else if (minusop) result -= value;
 				else if (timesop) result *= value;
				else if (excludeop) result ^= value;
                else result = value;
                excludeop = plusop = minusop = orop = shiftop = timesop = false;
            }
        }
		if (!stricmp(word,"`AS_IS"))  continue; // dont need to store AS_IS
		WORDP D = StoreWord(word,AS_IS | result);
		AddInternalFlag(D,DEFINES);

#ifdef WIN32
		sprintf(word+offset,(char*)"%I64d",result);
#else
		sprintf(word+offset,(char*)"%lld",result); 
#endif
		if (!endsystem) // cross ref from number to value only for properties and system flags and parsemarks for decoding bits back to words for marking
		{
			WORDP E = StoreWord(word);
			AddInternalFlag(E,DEFINES);
			if (!E->inferMark) E->inferMark = MakeMeaning(D); // if number value NOT already defined, use this definition
		}
	}
	FClose(in);
}

static MEANING ConceptFact(char* word, MEANING M, bool facts)
{
	WORDP D = BUILDCONCEPT(word);
	MEANING X = MakeMeaning(D);
	if (facts) CreateFact(X, Mmember, M);
	return X;
}

void AcquirePosMeanings(bool facts)
{
	// create pos meanings and sets
	// if (buildDictionary) return;	// dont add these into dictionary
	uint64 bit = START_BIT;
	char name[MAX_WORD_SIZE];
	*name = '~';
	MEANING pos = ConceptFact((char*)"~pos", 0, false);
	MEANING sys = ConceptFact((char*)"~sys", 0, false);
	for (int i = 63; i >= 0; --i) // properties get named concepts
	{
		char* word = FindNameByValue(bit);
 		if (word) 
		{
			MakeLowerCopy(name+1,word); // all pos names start with `
			posMeanings[i] = ConceptFact(name,pos,facts);
		}
		
		word = FindSystemNameByValue(bit);
		if (word) 
		{
			MakeLowerCopy(name+1,word); // all sys names start with ``
			sysMeanings[i] = ConceptFact(name, sys, facts);
		}
		
		bit >>= 1;
	}

	MEANING M = ConceptFact((char*)"~aux_verb", pos, facts);
	ConceptFact((char*)"~aux_verb_future", M, facts);
	ConceptFact((char*)"~aux_verb_past", M, facts);
	ConceptFact((char*)"~aux_verb_present", M, facts);
	ConceptFact((char*)"~aux_be", M, facts);
	ConceptFact((char*)"~aux_have", M, facts);
	ConceptFact((char*)"~aux_do", M, facts);

	M = ConceptFact((char*)"~aux_verb_tenses",pos,facts);
	ConceptFact((char*)"~aux_verb_future",M,facts);
	ConceptFact((char*)"~aux_verb_past",M, facts);
	ConceptFact((char*)"~aux_verb_present",M, facts);
	
	M = ConceptFact((char*)"~conjunction",pos,facts);
	ConceptFact((char*)"~conjunction_subordinate",M,facts);
	ConceptFact((char*)"~conjunction_coordinate",M,facts);

	M = ConceptFact((char*)"~determiner_bits", pos, facts);
	ConceptFact((char*)"~determiner", M, facts);
	ConceptFact((char*)"~predeterminer", M, facts);
		
	M = ConceptFact((char*)"~possessive_bits", pos, facts);
	ConceptFact((char*)"~possessive", M, facts);
	ConceptFact((char*)"~pronoun_possessive", M, facts);

	M = ConceptFact((char*)"~noun_bits", pos, facts);
	ConceptFact((char*)"~noun_singular", M, facts);
	ConceptFact((char*)"~noun_plural", M, facts);
	ConceptFact((char*)"~noun_proper_singular", M, facts);
	ConceptFact((char*)"~noun_proper_plural", M, facts);
	ConceptFact((char*)"~noun_number", M, facts);
	ConceptFact((char*)"~noun_adjective", M, facts);
	ConceptFact((char*)"~noun_gerund", M, facts);
	
	M = ConceptFact((char*)"~normal_noun_bits", pos, facts);
	ConceptFact((char*)"~noun_singular", M, facts);
	ConceptFact((char*)"~noun_plural", M, facts);
	ConceptFact((char*)"~noun_proper_singular", M, facts);
	ConceptFact((char*)"~noun_proper_plural", M, facts);

	M = ConceptFact((char*)"~pronoun_bits", pos, facts);
	ConceptFact((char*)"~pronoun_subject", M, facts);
	ConceptFact((char*)"~pronoun_object", M, facts);

	M = ConceptFact((char*)"~verb_bits", pos, facts);
	ConceptFact((char*)"~verb_present", M, facts);
	ConceptFact((char*)"~verb_present_3ps", M, facts);
	ConceptFact((char*)"~verb_past", M, facts);
	ConceptFact((char*)"~verb_past_participle", M, facts);
	ConceptFact((char*)"~verb_present_participle", M, facts);

	M = ConceptFact((char*)"~punctuation", pos, facts);
	ConceptFact((char*)"~paren", M, facts);
	ConceptFact((char*)"~comma", M, facts);
	ConceptFact((char*)"~quote", M, facts);
	ConceptFact((char*)"~currency", M, facts);

	MEANING role = ConceptFact((char*)"~grammar_role", 0, false);
	unsigned int i = 0;
	char* ptr;
	while ((ptr = roleSets[i++]) != 0) 
	{
		ConceptFact(ptr, role, facts);
	}
}

uint64 FindValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE];
	word[0] = ENDUNIT;
	MakeUpperCopy(word+1,name);
	WORDP D = FindWord(word);
	return (!D|| !(D->internalBits & DEFINES)) ? 0 : D->properties;
}

char* FindNameByValue(uint64 val) // works for invertable pos bits only
{
	char word[MAX_WORD_SIZE];
	word[0] = ENDUNIT;
#ifdef WIN32
	sprintf(word+1,(char*)"%I64d",val);
#else
	sprintf(word+1,(char*)"%lld",val); 
#endif
	WORDP D = FindWord(word);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	D = Meaning2Word(D->inferMark); 
	return D->word+1;
}

bool IsModelNumber(char* word)
{
	int alphanumeric = 0;
	char* start = word;
	--word;
	while (*++word)
	{
		if (IsAlphaUTF8(*word)) alphanumeric |= 1;
		else if (IsDigit(*word)) alphanumeric |= 2;
	}
	return (alphanumeric == 3 && !IsDate(start));
}

uint64 FindSystemValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE];
	word[0] = ENDUNIT;
	word[1] = ENDUNIT;
	MakeUpperCopy(word+2,name);
	WORDP D = FindWord(word);
	if (!D || !(D->internalBits & DEFINES)) 
	{
		if (xbuildDictionary) 
			ReportBug((char*)"Failed to find system value %s",name);
		return 0;
	}
	return D->properties;
}

char* FindSystemNameByValue(uint64 val) // works for invertable system bits only
{
	char word[MAX_WORD_SIZE];
	word[0] = ENDUNIT;
	word[1] = ENDUNIT;
#ifdef WIN32
	sprintf(word+2,(char*)"%I64d",val);
#else
	sprintf(word+2,(char*)"%lld",val); 
#endif
	WORDP D = FindWord(word);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	return Meaning2Word(D->inferMark)->word+2;
}

char* FindParseNameByValue(uint64 val)
{
	char word[MAX_WORD_SIZE];
	word[0] = ENDUNIT;
	word[1] = ENDUNIT;
	word[2] = ENDUNIT;
#ifdef WIN32
	sprintf(word+3,(char*)"%I64d",val);
#else
	sprintf(word+3,(char*)"%lld",val); 
#endif
	WORDP D = FindWord(word);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	return Meaning2Word(D->inferMark)->word+3;
}

uint64 FindParseValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE];
	word[0] = ENDUNIT;
	word[1] = ENDUNIT;
	word[2] = ENDUNIT;
	MakeUpperCopy(word+3,name);
	WORDP D = FindWord(word);
	if (!D || !(D->internalBits & DEFINES)) 
	{
		if (xbuildDictionary) ReportBug((char*)"Failed to find parse value %s",name);
		return 0;
	}
	return D->properties;
}

uint64 FindMiscValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE];
	word[0] = ENDUNIT;
	word[1] = ENDUNIT;
	word[2] = ENDUNIT;
	word[3] = ENDUNIT;
	MakeUpperCopy(word+4,name);
	WORDP D = FindWord(word);
	if (!D || !(D->internalBits & DEFINES)) 
	{
		if (xbuildDictionary) ReportBug((char*)"Failed to find misc value %s",name);
		return 0;
	}
	return D->properties;
}

/////////////////////////////////////////////
// BOOLEAN-STYLE QUESTIONS
/////////////////////////////////////////////
 
bool IsArithmeticOperator(char* word)
{
	word = SkipWhitespace(word);
	char c = *word;
	if (c == '+' || c == '-' || c == '*' || c == '/'  || c == '&') 
	{
		if (IsDigit(word[1]) || word[1] == ' ' || word[1] == '=') return true;
		return false;
	}

	return
		((c == '|' && (word[1] == ' ' || word[1] == '^' || word[1] == '=')) || 
		(c == '%' && !word[1]) || 
		(c == '%' && word[1] == ' ') || 
		(c == '%' && word[1] == '=') || 
		(c == '^' && !word[1]) || 
		(c == '^' && word[1] == ' ') ||
		(c == '^' && word[1] == '=') ||
		(c == '<' && word[1] == '<') || 
		(c == '>' && word[1] == '>')
		);
} 

char* IsUTF8(char* buffer,char* character) // swallow a single utf8 character (ptr past it) or return null 
{
	*character = *buffer;
	character[1] = 0;  // simple ascii character
    if (*buffer == 0) return buffer; // dont walk past end
	if (((unsigned char)*buffer) < 127) return buffer+1; // not utf8

	char* start = buffer;
	unsigned int count = UTFCharSize(buffer) - 1; // bytes beyond first
	if (count == 0) count = 300; // not real?

	// does count of extenders match requested count
	unsigned int n = 0;
	while (*++buffer && *(unsigned char*) buffer >= 128 && *(unsigned char*) buffer <= 191) ++n;
	if (n == count)
	{
		strncpy(character,start,count+1);
		character[count+1] = 0;
		return buffer;
	}
	return start+1; 
}

char GetTemperatureLetter(char* ptr)
{
	if (!ptr || !ptr[1] || !IsDigit(*ptr)) return 0; // format must be like 1C or 19F
	size_t len = strlen(ptr) - 1;
	char lastc = GetUppercaseData(ptr[len]);
	if (lastc == 'F' || lastc == 'C' || lastc == 'K') // ends in a temp letter
	{
		// prove rest of prefix is a number
		char* p = ptr-1;
		while (IsDigit(*++p)); // find end of number prefix
		if (len == (size_t)(p - ptr)) return lastc;
	}
	return 0;
}

char* IsTextCurrency(char* ptr,char* end)
{
	if (!end)
	{
		end = ptr;
		while (*++end && !(IsDigit(*end) || IsNonDigitNumberStarter(*end))); // locate nominal end of text
	}

	// look up direct word numbers
	WORDP D = FindWord(ptr, (end - ptr), LOWERCASE_LOOKUP);
	if ( D && D->properties & CURRENCY && currencies) for (unsigned int i = 0; i < 10000; ++i)
	{
		size_t len = end - ptr;
		int index = currencies[i].word;
		if (!index) break;
		char* w = Index2Heap(index);
		if (!strnicmp(ptr, w, len))
		{
			if (ptr[len] == 0 || IsDigit(ptr[len]) || IsNonDigitNumberStarter(ptr[len])) return Index2Heap(currencies[i].concept);  // a match 
		}
	}
	return NULL;
}

char* IsSymbolCurrency(char* ptr)
{
	if (!*ptr) return NULL;
	char* end = ptr;
	while (*++end && !IsDigit(*end)); // locate nominal end of text

	if (*ptr == 0xe2 && ptr[1] == 0x82 && ptr[2] == 0xac) return end;// euro is prefix
	else if (*ptr == 0xC2 && ptr[1] == 0xA2) return end; // cent sign
	else if (*ptr == 0xc2) // yen is prefix
	{
		char c = ptr[1];
		if (c == 0xa2 || c == 0xa3 || c == 0xa4 || c == 0xa5) return end;
	}
	else if (*ptr == 0xc3 && ptr[1] == 0xb1) return end; // british pound
	else if (*ptr == 0xe2 && ptr[1] == 0x82 && ptr[2] == 0xb9) return end; // rupee
	else if (IsTextCurrency(ptr,end) != NULL) return end;
	return NULL;
}
   
unsigned char* GetCurrency(unsigned char* ptr,char* &number) // does this point to a currency token, return currency and point to number (NOT PROVEN its a number)
{
	unsigned char* at = ptr;
	if (*at == '+' || *at == '-') ++at; //   skip sign indicator, -$1,234.56

	char* prefixEnd = IsSymbolCurrency((char*)at); // is it at start
	if (prefixEnd)
	{
		number = prefixEnd;
		if (at != ptr) // was there a sign before the symbol
		{
			char c = *ptr;
			memmove(ptr, at, (number - (char*)at));
			(--number)[0] = c;
		}
		return ptr;
	}

	if (IsDigit(*at))  // number first
	{
		while (IsDigit(*at) || *at == '.' || *at == ',') ++at; // get end of number
		prefixEnd = IsSymbolCurrency((char*)at);
		if (prefixEnd)
		{
			number = (char*)ptr;
			return at;
		}
	}
	
	return 0;
}

bool IsLegalName(char* name, bool label) // start alpha (or ~) and be alpha _ digit (concepts and topics can use . or - also)
{
	char start = *name;
	if (*name == '~' || *name == SYSVAR_PREFIX) ++name;
	if (label && IsDigit(*name)) {}
	else if (!IsAlphaUTF8(*name) ) return false;
	while (*++name)
	{
		if (*name == '.' && start != '~') return false;	// may not use . in a variable name
		if (!IsLegalNameCharacter(*name)) return false;
	}
	return true;
}

bool IsDigitWithNumberSuffix(char* number, int useNumberStyle)
{
	size_t len = strlen(number);
	char d = number[len-1];
	bool num = false;
	if (d == 'k' || d == 'K' || d == 'm' || d == 'M' || d == 'B' || d == 'b' || d == 'G' || d == 'g' || d == '$')
	{
		number[len-1] = 0;
		num = IsDigitWord(number, useNumberStyle);
		number[len-1] = d;
	}
	return num;
}

bool IsInteger(char* ptr, bool comma, int useNumberStyle)
{
	char digitGroup = digitGroupingData[useNumberStyle];
	if (IsNonDigitNumberStarter(*ptr)) ++ptr; //   skip numeric nondigit header (+ - # )
	bool foundDigit = false;
	while (*ptr)
	{
		if (IsDigit(*ptr)) foundDigit = true; // we found SOME part of a number
		else if (*ptr == digitGroup && comma); // allow comma
		else  return false;
		++ptr;
	}
	return foundDigit;
}

bool IsDigitWord(char* ptr,int useNumberStyle,bool comma) // digitized number
{
	if (!*ptr) return false;
	char* end = ptr + strlen(ptr);
	char decimalMark = decimalMarkData[useNumberStyle];
	char digitGroup = digitGroupingData[useNumberStyle];
	if (IsFloat(ptr, end, useNumberStyle)) return true; // sentence end if . at end, not a float
    //   signing, # marker or currency markers are still numbers
	char* number = 0;
	char* currency = 0;
	if ((currency = ( char*)GetCurrency((unsigned char*) ptr,number))) ptr = number; // if currency, find number start of it
	if (IsNonDigitNumberStarter(*ptr)) ++ptr; //   skip numeric nondigit header (+ - # )
	if (!*ptr) return false;

    bool foundDigit = false;
    while (*ptr) 
    {
		if (IsDigit(*ptr)) foundDigit = true; // we found SOME part of a number
		else if (*ptr == decimalMark) return false; // float will find that
		else if (*ptr == '%' && !ptr[1]) break; // percentage
		else if (*ptr == ':');	//   TIME delimiter
		else if (*ptr == digitGroup && comma && IsCommaNumberSegment(ptr+1, currency)); // allow comma, but rest of word needs to be valid too
		else if (ptr == currency) break; // dont need to see currency end
		else return false;		//   1800s is done by substitute, so fail this
		++ptr;
    }
    return foundDigit;
}  

bool IsCommaNumberSegment(char* ptr, char* end) // number after a comma
{
	// next three characters (perhaps only 2 if Indian) should be numbers
	if (!*ptr || !IsDigit(*ptr) || !IsDigit(ptr[1])) return false; // must have 2 digits

	int i = (numberStyle == INDIAN_NUMBERS) ? 2 : 3;
	char* comma = ptr + i;

	if (!IsDigit(*(comma-1))) return false; // must be a number in the preceeding position

	if (end && comma == end) return true; // end of number, start of currency symbol
	if (i == 3 && !*comma) return true; // end of number
	if (i == 2 && IsDigit(*comma) && !comma[1]) return true; // end of number, final 3 digit segment

	if (*comma && *comma == numberComma && IsCommaNumberSegment(comma+1, end)) return true; // valid number segments all the way
	return false;
}

bool IsRomanNumeral(char* word, uint64& val)
{
	if (*word == 'I' && !word[1]) return false;		// BUG cannot accept I  for now. too confusing.
	val = 0;
	--word;
	unsigned int value;
	unsigned int oldvalue = 100000;
	while ((value = roman[(unsigned char)*++word]))
	{
		if (value > oldvalue) // subtractive?
		{
			// I can be placed before V and X to make 4 units (IV) and 9 units (IX) respectively
			// X can be placed before L and C to make 40 (XL) and 90 (XC) respectively
			// C can be placed before D and M to make 400 (CD) and 900 (CM) according to the same pattern[5]
			if (value == 5 && oldvalue == 1) val += (value - 2);
			else if (value == 10 && oldvalue == 1) val += (value - 2);
			else if (value == 50 && oldvalue == 10) val += (value - 20);
			else if (value == 100 && oldvalue == 10) val += (value - 20);
			else if (value == 500 && oldvalue == 100) val += (value - 200);
			else if (value == 1000 && oldvalue == 100) val += (value - 200);
			else return false;	 // not legal
		}
		else val += value;
		oldvalue = value;
	}
	return (value && !*word); // finished or not
}

void ComputeWordData(char* word, WORDINFO* info) // how many characters in word
{
    memset(info, 0, sizeof(WORDINFO));
    info->word = word;
    int n = 0;
    char utfcharacter[10];
    while (*word)
    {
        char* x = IsUTF8(word, utfcharacter); // return after this character if it is valid.
        ++n;
        if (utfcharacter[1]) // utf8 char
        {
            ++info->charlen;
            info->bytelen += (x - word);
			word = x;
		}
        else // normal ascii
        {
            ++info->charlen;
            ++info->bytelen;
            ++word;
        }
    }
}

unsigned int IsNumber(char* num, int useNumberStyle, bool placeAllowed) // simple digit number or word number or currency number
{
	if (!*num) return false;
	char word[MAX_WORD_SIZE];
	MakeLowerCopy(word, num); // accept number words in upper case as well
	if (word[1] && (word[1] == ':' || word[2] == ':')) return false;	// 05:00 // time not allowed
	size_t len = strlen(word);
	if (word[len - 1] == '%') word[len - 1] = 0;	// % after a number is still a number

	char* number = NULL;
	char* cur = (char*)GetCurrency((unsigned char*)word, number);
	if (cur)
	{
		char c = *cur;
		*cur = 0;
		char decimalMark = decimalMarkData[useNumberStyle];
		char* at = strchr(number, decimalMark);
		if (at) *at = 0;
		int64 val = Convert2Integer(number, useNumberStyle);
		if (at) *at = decimalMark;
		*cur = c;
		return (val != NOT_A_NUMBER) ? CURRENCY_NUMBER : NOT_A_NUMBER;
	}
	if (IsDigitWord(word, useNumberStyle, true)) return DIGIT_NUMBER; // a numeric number

	if (*word == '#' && IsDigitWord(word + 1, useNumberStyle)) return DIGIT_NUMBER; // #123

	if (*word == '\'' && !strchr(word + 1, '\'') && IsDigitWord(word + 1, useNumberStyle)) return DIGIT_NUMBER;	// includes date and feet
	uint64 valx;
	if (IsRomanNumeral(word, valx)) return ROMAN_NUMBER;
	if (IsDigitWithNumberSuffix(word, useNumberStyle)) return WORD_NUMBER;
	WORDP D;
	char* ptr;
	if (placeAllowed && IsPlaceNumber(word, useNumberStyle)) return PLACETYPE_NUMBER; // th or first or second etc. but dont call if came from there
	else if (!IsDigit(*word) && ((ptr = strchr(word + 1, '-')) || (ptr = strchr(word + 1, '_'))))	// composite number as word, but not digits
	{
		D = FindWord(word, ptr - word);			// 1st part
		WORDP W = FindWord(ptr + 1);		// 2nd part of word
		if (D && W && D->properties & NUMBER_BITS && W->properties & NUMBER_BITS && IsPlaceNumber(W->word)) return FRACTION_NUMBER;
		if (D && W && D->properties & NUMBER_BITS && W->properties & NUMBER_BITS) return WORD_NUMBER;
	}

	char* hyphen = strchr(word + 1, '-');
	if (!hyphen) hyphen = strchr(word + 1, '_'); // two_thirds
	if (hyphen && hyphen[1] && (IsDigit(*word) || IsDigit(hyphen[1]))) return NOT_A_NUMBER; // not multihypened words
	if (hyphen && hyphen[1])
	{
		char c = *hyphen;
		*hyphen = 0;
		int kind = IsNumber(word, useNumberStyle); // what kind of number
		int64 piece1 = Convert2Integer(word, useNumberStyle);
		*hyphen = c;
		if (piece1 == NOT_A_NUMBER && stricmp(word, (char*)"zero") && *word != '0') { ; }
		else if (IsPlaceNumber(hyphen + 1, useNumberStyle) || kind == FRACTION_NUMBER) return FRACTION_NUMBER;
	}

	// test for fraction or percentage
	bool slash = false;
	bool percent = false;
	if (IsDigit(*word)) // see if all digits now.
	{
		char* ptr = word;
		while (*++ptr)
		{
			if (*ptr == '/' && !slash) slash = true;
			else if (*ptr == '%' && !ptr[1]) percent = true;
			else if (!IsDigit(*ptr)) break;	// not good
		}
		if (slash && !*ptr) return FRACTION_NUMBER;
		if (percent && !*ptr) return FRACTION_NUMBER;
	}

	// look up direct word numbers
	D = FindWord(word);
	len = strlen(word);
	if (D && D->properties & (NOUN_NUMBER|ADJECTIVE_NUMBER) && numberValues) for (unsigned int i = 0; i < 1000; ++i)
	{
		int index = numberValues[i].word;
		if (!index) break;
		char* w = Index2Heap(index);
		char* num = Index2Heap(index);
		if (len == numberValues[i].length && !strnicmp(word, num, len))
		{
			return numberValues[i].realNumber;  // a match 
		}
	}

	if (D && D->properties & NUMBER_BITS)
		return (D->systemFlags & ORDINAL) ? PLACETYPE_NUMBER : WORD_NUMBER;   // known number

    return (Convert2Integer(word,useNumberStyle) != NOT_A_NUMBER) ? WORD_NUMBER : NOT_A_NUMBER;		//   try to read the number
}

bool IsPlaceNumber(char* word, int useNumberStyle) // place number and fraction numbers
{   
    size_t len = strlen(word);
	if (len < 3) return false; // min is 1st
	
	// word place numbers
	char tok[MAX_WORD_SIZE];
	strcpy(tok, word);
	size_t size = 0;
	if (stricmp(language, "english")) {;}
	else if (len > 4 && !strcmp(word+len-5,(char*)"first") ) size = 5;
	else if (len > 5 && !strcmp(word+len-6,(char*)"second") ) size = 6;
	else if (len > 4 && !strcmp(word+len-5,(char*)"third") ) size = 5;
	else if (len > 4 && !strcmp(word+len-5,(char*)"fifth") ) size = 5;
	if (size)
	{
		if (len == size) return true;
		size_t l = strlen(tok);
		tok[l - size] = 0;
		if (tok[l-size-1] == '-') tok[l - size - 1] = 0; // fifty-second
		if (IsNumber(tok,useNumberStyle) != NOT_A_NUMBER) return true;
		else return false;
	}

	// does it have proper endings?
	if (stricmp(language, "english")) { ; }
	else if (word[len-2] == 's' && word[len-1] == 't') {;}  // 1st
	else if (word[len-2] == 'n' && word[len-1] == 'd') // 2nd
	{
		if (!stricmp(word,(char*)"thousand")) return false;	// 2nd but not thousand
	} 
	else if (word[len-1] == 'd') // 3rd
	{
		if (word[len-2] != 'r') return false;	// 3rd is ok
	} 
	else if (word[len-2] == 't' && word[len-1] == 'h') {;} // 4th
	else if (word[len-3] == 't' && word[len-2] == 'h' && word[len-1] == 's') {;} // 4ths
	else return false;	// no place suffix
	if (strchr(word, '_')) return false;	 // cannot be place number

	// separator?
	if (word[len-3] == '\'' || word[len-3] == '-') {;}// 24'th or 24-th
	else if (strchr(word,'-')) return false;	 // cannot be place number

    if (len < 4 && !IsDigit(*word)) return false; // want a number
	char num[MAX_WORD_SIZE];
	strcpy(num,word);
	return (IsNumber(num,useNumberStyle,false) == PLACETYPE_NUMBER) ? true : false; // show it is correctly a number - pass false to avoid recursion from IsNumber
}

void WriteInteger(char* word, char* buffer, int useNumberStyle)
{
	if (useNumberStyle == NOSTYLE_NUMBERS)
	{
		strcpy(buffer, word);
		return;
	}
	char reverseFill[MAX_WORD_SIZE];
	char* end = word + strlen(word);
	char* at = reverseFill + 500;
	*at = 0;
	int counter = 0;
	int limit = 3;
	while (end != word)
	{
		*--at = *--end;
		if (IsDigit(*at) && ++counter == limit && end != word)
		{
			*--at = (useNumberStyle == FRENCH_NUMBERS) ? '.' : ',';
			counter = 0;
			limit = (useNumberStyle == INDIAN_NUMBERS) ? 2 : 3;
		}
	}
	strcpy(buffer, at);
}

void FormatFloat(char* word, char* buffer, int useNumberStyle)
{
	char* dot = strchr(word, '.');
	if (dot) *dot = 0;
	WriteInteger(word, buffer, useNumberStyle); // leading float/int bit

	// Localise the decimal point
	if (dot)
	{
		*dot = (useNumberStyle == FRENCH_NUMBERS) ? ',' : '.';
		strcat(buffer, dot);
	}
}

char* WriteFloat(char* buffer, double value, int useNumberStyle)
{
	char floatpart[MAX_WORD_SIZE];
	if (!fullfloat) 
	{
		sprintf(floatpart, (char*)"%1.2f", value);
		char* at = strchr(floatpart, '.');
		if (at)
		{
			char* loc = at;
			while (*++at)
			{
				if (*at != '0') break; // not pure
			}
			if (!*at) *loc = 0; // switch to integer
		}
	}
	else sprintf(floatpart, (char*)"%1.50g", value);

	FormatFloat(floatpart, buffer, useNumberStyle);
	return buffer;
}

bool IsFloat(char* word, char* end, int useNumberStyle)
{
	if (*(end - 1) == '.') return false;	 // float does not end with ., that is sentence end
	if (*word == '-' || *word == '+') ++word; // ignore sign
	if (!IsDigit(*word)) return false;
	char decimalMark = decimalMarkData[useNumberStyle];
	char digitGroup = digitGroupingData[useNumberStyle];
	int period = 0;
	--word;
	bool exponent = false;
	while (++word < end) // count periods
	{
	    if (*word == decimalMark && !exponent) ++period;
		else if (!IsDigit(*word) && *word != digitGroup)
		{
			if ((*word == 'e' || *word == 'E') && !exponent)
			{
				exponent = true;
				if (word[1] == '-' || word[1] == '+') ++word; // ignore exponent sign
				if (!IsDigit(word[1])) return false; // need a number after the exponent, 10euros is not a float
			}
			else return false; // non digit is fatal
		}
    }
    return (period == 1 || exponent);
}

bool IsNumericDate(char* word,char* end) // 01.02.2009 or 1.02.2009 or 1.2.2009
{ // 3 pieces separated by periods or slashes. with 1 or 2 digits in two pieces and 2 or 4 pieces in last piece

	char separator = 0;
	int counter = 0;
	int piece = 0;
	int size[100];
	memset(size,0, sizeof(size));
	--word;
	while (++word < end) 
	{
		char c = *word;
		if (IsDigit(c)) ++counter;
		else if (c == '.' || c == '/') // date separator
		{
			if (!counter) return false;	// no piece before it
			size[piece++] = counter;	// how big was the piece
			counter = 0;
			if (!separator) separator = c;		// seeing this kind of separator
			if (c != separator) return false;	// cannot mix
			if (!IsDigit(word[1])) return false;	// doesnt continue with digits
		}
		else return false;					//  illegal date
	}
	if (piece != 2) return false;	//   incorrect piece count
	if (size[0] != 4)
	{
		if (size[2] != 4 || size[0] > 2 || size[1] > 2) return false;
	}
	else if (size[1] > 2 || size[2] > 2) return false;
	return true;
}

bool IsUrl(char* word, char* end)
{
    if (*word == '@') return false;
	if (strchr(word, ' ') || !strchr(word, '.')) return false; // cannot have space, must have dot dot
	if (!strnicmp((char*)"www.",word,4))
	{
		char* colon = strchr(word,':');
		if (colon && (colon[1] != '/' || colon[2] != '/')) return false;
		return true; // classic web url without protocol
	}
	if (!strnicmp((char*)"http", word, 4) || !strnicmp((char*)"ftp:", word, 4))
	{
		char* colon = strchr(word, ':');
		if (colon && colon[1] == '/' && colon[2] == '/' && IsAlphaUTF8OrDigit(colon[3]) ) return true; // classic urls, with something beyond just the protocol
		if (colon) return false; // only protocol is not enough
	}
	size_t len = strlen(word);
	if (len > 200) return false;
    char tmp[MAX_WORD_SIZE];
	MakeLowerCopy(tmp,word);
    if (!end) end = word + len; 
    tmp[end-word] = 0;
    char* ptr = tmp;
	
	char* at = strchr(tmp,'@');	// check for email
	if (at) 
	{
		char* dot = strchr(at+2,'.'); // must have character after @ and before .
		if (dot && IsAlphaUTF8(dot[1])) return true;
	}

	//	check domain suffix is somewhat known as a TLD
	//	fireze.it OR www.amazon.co.uk OR amazon.com OR kore.ai
	char* firstPeriod = strchr(tmp, '.');
	if (!firstPeriod) return false; // not a possible url
	char* domainEnd = strchr(firstPeriod, '/');
	if (domainEnd) *domainEnd = 0;
	ptr = strrchr(tmp, '.'); // last period - KNOWN to exist
	if (!ptr) return false;	// stops compiler warning

	if (IsAlphaUTF8(ptr[1]) && IsAlphaUTF8(ptr[2]) && !ptr[3]) return true;	 // two character TLD
	++ptr;
	// check for common suffices - https://w3techs.com/technologies/overview/top_level_domain/all
	// most up to date list of all TLDs at http://data.iana.org/TLD/tlds-alpha-by-domain.txt
	return (!strnicmp(ptr,(char*)"com",3) || !strnicmp(ptr,(char*)"net",3) || !strnicmp(ptr,(char*)"org",3) || !strnicmp(ptr,(char*)"edu",3) || !strnicmp(ptr,(char*)"biz",3) || !strnicmp(ptr,(char*)"gov",3) || !strnicmp(ptr,(char*)"mil",3) || !strnicmp(ptr, (char*)"info", 4));
}

unsigned int IsMadeOfInitials(char * word,char* end) 
{  
	if (IsDigit(*word)) return 0; // it's a number
	char* ptr = word-1;
	while (++ptr < end) // check for alternating character and periods
	{
		if (!IsAlphaUTF8OrDigit(*ptr)) return false;
		if (*++ptr != '.') return false;
    }
    if (ptr >= end)  return ABBREVIATION; // alternates letter and period perfectly (abbreviation or middle initial)

    //   if lower case start and upper later, it's a typo. Call it a shout (will lower case if we dont know as upper)
    ptr = word-1;
    if (IsLowerCase(*word))
    {
        while (++ptr != end)
        {
	        if (IsUpperCase(*ptr)) return SHOUT;
        }
		return 0;
    }

    //   ALL CAPS 
    while (++ptr != end)
    {
	    if (!IsUpperCase(*ptr)) return 0;
    }
	
	//   its all caps, needs to be lower cased
	WORDP D = FindWord(word);
	if (D) // see if there are any legal allcaps forms
	{
		WORDP set[20];
		int n = GetWords(word,set,true);
		while (n)
		{
			WORDP X = set[--n];
			if (!strcmp(word,X->word)) return 0;	// we know it in all caps format
		}
	}
	return (D && D->properties & (NOUN_PROPER_SINGULAR|NOUN_PROPER_PLURAL)) ? ABBREVIATION : SHOUT; 
}

////////////////////////////////////////////////////////////////////
/// READING ROUTINES
////////////////////////////////////////////////////////////////////

char* ReadFlags(char* ptr,uint64& flags,bool &bad, bool &response,bool factcall)
{
	flags = 0;
	response = false;
	char* start = ptr;
	if (!*ptr) return start;
	if (*ptr != '(') // simple solo flag
	{
		char word[MAX_WORD_SIZE];
		ptr = ReadCompiledWord(ptr,word);
		if (!strnicmp(word,(char*)"RESPONSE_",9)) response = true; // saw a response flag
		if (IsDigit(*word) || *word == 'x') ReadInt64(word,(int64&)flags);
		else
		{
			flags = FindValueByName(word);
			if (!flags) flags = FindSystemValueByName(word);
			if (!flags) flags = FindMiscValueByName(word);
			if (!flags) bad = true;
		}
		if (factcall) return ptr;
		return  (!flags && !IsDigit(*word) && *word != 'x' && !response) ? start : ptr;	// if found nothing return start, else return end
	}

	char flag[MAX_WORD_SIZE];
	char* close = strchr(ptr,')');
	if (close) *close = 0;
	while (*ptr) // read each flag
	{
		FunctionResult result;
		ptr = ReadShortCommandArg(ptr,flag,result); // swallows excess )
 		if (result & ENDCODES) return ptr;
		if (*flag == '(') continue;	 // starter
		if (*flag == USERVAR_PREFIX) flags |= atoi(GetUserVariable(flag)); // user variable indirect
		else if (*flag == '0' && (flag[1] == 'x' || flag[1] == 'X')) // literal hex value
		{
			uint64 val;
			ptr = ReadHex(flag,val);
			flags |= val;
		}
		else if (IsDigit(*flag)) flags |= atoi(flag);
		else  // simple name of flag
		{
			uint64 n = FindValueByName(flag);
			if (!n) n = FindSystemValueByName(flag);
			if (!n) n = FindMiscValueByName(flag);
			if (!n) bad = true;
			flags |= n;
		}
		ptr = SkipWhitespace(ptr);
	}
	if (close)
	{
		*close = ')';
		ptr = SkipWhitespace(close + 1);
	}
	if (factcall) return ptr;
	return  (!flags) ? start : ptr; 
}

char* ReadInt(char* ptr, int &value)
{
	ptr = SkipWhitespace(ptr);

    value = 0;
    if (!ptr || !*ptr ) return ptr;
    if (*ptr == '0' && (ptr[1]== 'x' || ptr[1] == 'X'))  // hex number
	{
		uint64 val;
		ptr = ReadHex(ptr,val);
		value = (int)val;
		return ptr;
	}
	char* original = ptr;
	int sign = 1;
    if (*ptr == '-')
    {
         sign = -1;
         ++ptr;
    }
	--ptr;
    while (!IsWhiteSpace(*++ptr) && *ptr)  // find end of synset id in dictionary
    {  
         if (*ptr == ',') continue;    // swallow this
         value *= 10;
         if (IsDigit(*ptr)) value += *ptr - '0';
         else 
         {
			 Bug(); // ReportBug((char*)"bad number %s\r\n", original)
             while (*++ptr  && *ptr != ' ');
             value = 0;
             return ptr;
         }
     }
     value *= sign;
	 if (*ptr) ++ptr; // skip trailing blank
     return ptr;  
}

int64 atoi64(char* ptr )
{
	int64 spot;
	ReadInt64(ptr,spot);
	return spot;
}

char* ReadInt64(char* ptr, int64 &spot)
{
	ptr = SkipWhitespace(ptr);
    spot = 0;
    if (!ptr || !*ptr) return ptr;
    if (*ptr ==  'x' ) return ReadHex(ptr,(uint64&)spot);
	if (*ptr == '0' && (ptr[1]== 'x' || ptr[1] == 'X')) return ReadHex(ptr,(uint64&)spot);
	char* original = ptr;
     int sign = 1;
     if (*ptr == '-')
     {
         sign = -1;
         ++ptr;
     }
     else if (*ptr == '+') ++ptr;
     char* currency1;
	 char* cur1 = (char*) GetCurrency((unsigned char*)ptr, currency1); // alpha part
	 if (cur1 && cur1 == ptr) ptr = currency1; // point to number part

	 --ptr;
	 while (!IsWhiteSpace(*++ptr) && *ptr) 
     {  
         if (*ptr == ',') continue;    // swallow this
         spot *= 10;
         if (IsDigit(*ptr)) spot += *ptr - '0';
		 else if (cur1 && ptr == cur1) break;	 // end of number part of like 65$
         else 
         {
			 Bug(); // ReportBug((char*)"bad number1 %s\r\n", original)
             while (*++ptr  && *ptr != ' ');
             spot = 0;
             return ptr;
         }
     }
     spot *= sign;
	 if (*ptr) ++ptr;	// skip trailing blank
     return ptr;  
}

char* ReadHex(char* ptr, uint64 & value)
{
	ptr = SkipWhitespace(ptr);
    if (!ptr || !*ptr) return ptr;
	if (ptr[0] == 'x') ++ptr; // skip x
    else if (ptr[1] == 'x' || ptr[1] == 'X') ptr += 2; // skip 0x
    --ptr;
	value = 0;
	while (*++ptr)
    {
		char c = GetLowercaseData(*ptr);
		if (c == 'l' || c == 'u') continue;				// assuming L or LL or UL or ULL at end of hex
        if (!IsAlphaUTF8OrDigit(c) || c > 'f') break; //   no useful characters after lower case at end of range
        value <<= 4; // move over a digit
        value += (IsDigit(c))? (c - '0') : (10 + c - 'a');
    }
	if (*ptr) ++ptr;// align past trailing space
    return ptr; 
}

void BOMAccess(int &BOMvalue, char &oldc, int &oldCurrentLine) // used to get/set file access- -1 BOMValue gets, other sets all
{
	if (BOMvalue >= 0) // set back
	{
		BOM = BOMvalue;
		holdc = oldc;
		maxFileLine = currentFileLine = oldCurrentLine;
	}
	else // get copy and reinit for new file
	{
		BOMvalue = BOM;
		BOM = NOBOM;
		oldc = holdc;
		holdc = 0;
		oldCurrentLine = currentFileLine;
		maxFileLine = currentFileLine = 0;
	}
}

#define NOT_IN_FORMAT_STRING 0
#define IN_FORMAT_STRING 1
#define IN_FORMAT_CONTINUATIONLINE 2
#define IN_FORMAT_COMMENT 3

static bool ConditionalReadRejected(char* start,char*& buffer,bool revise)
{
	char word[MAX_WORD_SIZE];
	if (!compiling) return false;
	char* at = ReadCompiledWord(start,word);
	if (!stricmp(word,"ifdef") || !stricmp(word,"ifndef") || !stricmp(word,"define")) return false;
	if (!stricmp(word,"endif") || !stricmp(word,"include") ) return false;

	// a matching language declaration?
	if (!stricmp(language,word)) 
	{
		if (revise)
		{
			memmove(start-1,at, strlen(at)+1); // erase comment marker
			buffer = start + strlen(start);
		}
		return false; // allowed
	}

	// could it be a constant?
	uint64 n = FindValueByName(word);
	if (!n) n = FindSystemValueByName(word);
	if (!n) n = FindParseValueByName(word);
	if (!n) n = FindMiscValueByName(word);
	if (n) return false;	 // valid constant
	for (int i = 0; i < conditionalCompiledIndex; ++i)
	{
		if (!stricmp(conditionalCompile[i]+1,word)) 
		{
			if (revise)
			{
				memmove(start-1,at, strlen(at)+1); // erase comment marker
				buffer = start + strlen(start);
			}
			return false; // allowed
		}
	}
	return true; // reject this line 
}

bool AdjustUTF8(char* start,char* buffer)
{
	bool hasbadutf = false;
	while (*++buffer)  // check every utf character
	{
		if (*buffer & 0x80) // is utf in theory
		{
			char prior = (start == buffer) ? 0 : *(buffer - 1);
			char utfcharacter[10];
			char* x = IsUTF8(buffer, utfcharacter); // return after this character if it is valid.
			if (utfcharacter[1]) // rewrite some utf8 characters to std ascii
			{
				if (buffer[0] == 0xc2 && buffer[1] == 0xb4) // a form of '
				{
					*buffer = '\'';
					memmove(buffer + 1, x, strlen(x) + 1);
					x = buffer + 1;
					if (compiling) WARNSCRIPT("UTF8 B4 ' revised to Ascii %s | %s",start,buffer);
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && buffer[2] == 0x98)  // open single quote
				{
					// if attached to punctuation, leave it so it will tokenize with them
					*buffer = ' ';
					buffer[1] = '\'';
					buffer[2] = ' ';
					if (compiling) WARNSCRIPT("UTF8 opening single quote revised to Ascii %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && buffer[2] == 0x99)  // closing single quote (may be embedded or acting as a quoted expression
				{
					if (buffer != start && IsAlphaUTF8(*(buffer - 1)) && IsAlphaUTF8(*x)) // embedded contraction
					{
						*buffer = '\'';
						memmove(buffer + 1, x, strlen(x) + 1);
						x = buffer + 1;
						if (compiling) WARNSCRIPT("UTF8 closing single quote revised to Ascii  %s | %s", start, buffer);
					}
					else if (prior == 's' && !IsAlphaUTF8(*x)) // ending possessive
					{
						*buffer = '\'';
						memmove(buffer + 1, x, strlen(x) + 1);
						x = buffer + 1;
					}
					else if (prior == '.' || prior == '?' || prior == '!') // leave attached to prior so readdocument can leave them together
					{
						*buffer = '\'';
						buffer[1] = ' ';
						buffer[2] = ' ';
					}
					else
					{
						*buffer = ' ';
						buffer[1] = '\'';
						buffer[2] = ' ';
					}
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && buffer[2] == 0x9c)  // open double quote
				{
					*buffer = '\'';
					memmove(buffer + 1, x, strlen(x) + 1);
					if (compiling) WARNSCRIPT("UTF8 opening double quote revised to Ascii %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xef && buffer[1] == 0xbc &&  buffer[2] == 0x88)  // open curly paren 
				{
					*buffer = '(';
					memmove(buffer + 1, x, strlen(x) + 1);
					if (compiling) WARNSCRIPT("UTF8 opening curly paren quote revised to Ascii %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xef && buffer[1] == 0xbc && buffer[2] == 0x89)  // closing curly paren 
				{
					*buffer = ')';
					memmove(buffer + 1, x, strlen(x) + 1);
					if (compiling) WARNSCRIPT("UTF8 closing curly paren quote revised to Ascii %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && buffer[2] == 0x9d)  // closing double quote
				{
					*buffer = '"';
					memmove(buffer + 1, x, strlen(x) + 1);
					if (compiling) WARNSCRIPT("UTF8 closing double quote revised to Ascii %s | %s", start, buffer)
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && buffer[2] == 0x94 && !(tokenControl & NO_FIX_UTF) )  // mdash
				{
					if (!compiling) *buffer = '-';
					if (compiling) WARNSCRIPT("UTF8 mdash seen  %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && (buffer[2] == 0x94 || buffer[2] == 0x93) && !(tokenControl & NO_FIX_UTF) )  // mdash
				{
					if (!compiling)
					{
						*buffer = '-';
						memmove(buffer + 1, x, strlen(x) + 1);
					}
					if (compiling) WARNSCRIPT("UTF8 mdash seen  %s | %s", start, buffer);
				}
				buffer = x - 1; // valid utf8
			}
			else // invalid utf8
			{
				hasbadutf = true;
				*buffer = 'z';	// replace with legal char
			}
		}
	}
	return hasbadutf;
}

int ReadALine(char* buffer,FILE* in,unsigned int limit,bool returnEmptyLines,bool convertTabs) 
{ //  reads text line stripping of cr/nl
	currentFileLine = maxFileLine; // revert to best seen
	if (currentFileLine == 0)
	{
		BOM = (BOM == BOMSET) ? BOMUTF8 : NOBOM; // start of file, set BOM to null
		hasHighChar = false; // for outside verify
	}
	*buffer = 0;
	buffer[1] = 0;
	buffer[2] = 1;
	if (in == (FILE*) -1)
	{
		return 0;
	}
	if (!in && !userRecordSourceBuffer)  
	{
		buffer[1] = 0;
		buffer[2] = 0; // clear ahead to make it obvious we are at end when debugging
		return -1;	// neither file nor buffer being read
	}
	int formatString = NOT_IN_FORMAT_STRING;
	char ender = 0;
	char* start = buffer;
	char c = 0;

	if (holdc)					//   if we had a leftover character from prior line lacking newline, put it in now
	{
		*buffer++ = holdc; 
		holdc = 0;
	}
	bool hasutf = false;
	bool hasbadutf = false;
	bool runningOut = false;
	bool endingBlockComment = false;
RESUME:
	while (ALWAYS)
	{
		if (in) 
		{
			if (!fread(&c,1,1,in)) 
			{
				++currentFileLine;	// for debugging error messages
				maxFileLine = currentFileLine;
				break;	// EOF
			}
		}
		else
		{
			c = *userRecordSourceBuffer++;
			if (!c) 
				break;	// end of buffer
		}
		if (c == '\t' && convertTabs) c = ' ';
		if (c & 0x80 ) // high order utf?
		{
			unsigned char convert = 0;
			if (!BOM && !hasutf && !server) // local mode might get extended ansi so transcribe to utf8 (but dont touch BOM)
			{
				unsigned char x = (unsigned char) c;
				if (x == 147 || x == 148) convert = c = '"';
				else if (x == 145 || x == 146) convert = c = '\'';
				else if (x == 150 || x == 151) convert = c = '-';
				else
				{
					convert = extendedascii2utf8[x - 128];
					if (convert)
					{
						*buffer++ = (unsigned char) 0xc3;
						c = convert;
					}
				}
			}
			if (!convert) hasutf = true;
			else hasHighChar = true; // we change extended ansi to normal (not utf8)
		}
		
		// format string stuff
		if (formatString)
		{
			if (formatString == IN_FORMAT_COMMENT && c != '\r' && c != '\n') continue; // flush comment
			//  format string
			else if (formatString && c == ender  && *(buffer-1) != '\\') 
				formatString = NOT_IN_FORMAT_STRING; // end format string
			else if (formatString == IN_FORMAT_CONTINUATIONLINE)
			{
				if (c == ' ' || c == '\t') continue;	// ignore white space leading the line
				formatString = IN_FORMAT_STRING;
			}
			else if (formatString && formatString != IN_FORMAT_COMMENT && *(buffer-1) == '#' && *(buffer-2) != '\\' && c == ' ' )  // comment start in format string
			{
				formatString = IN_FORMAT_COMMENT; // begin format comment
				buffer -= 2;
				continue;
			}
		}
		else if (compiling && !blockComment && (c == '"' || c == '\'') && (buffer-start) > 1 && *(buffer-1) == '^' ) 
		{
			formatString = IN_FORMAT_STRING; // begin format string
			ender = c;
		}

		// buffer overflow  handling
		if ((unsigned int)(buffer-start) >= (limit - 120)) 
		{
			runningOut = true;
			if ((unsigned int)(buffer-start) >= (limit - 80))  break;	 // couldnt find safe space boundary to break on
		}
		if (c == ' ' && runningOut) c = '\r';	// force termination on safe boundary

		if (c == '\r')  // part of cr lf or just a cr?
		{
			if (formatString) // unclosed format string continues onto next line
			{
				while (*--buffer == ' '){;}
				*++buffer = ' '; // single space at line split
				++buffer;
				formatString = IN_FORMAT_CONTINUATIONLINE;
				continue; // glide on past
			}

			c = 0;
			if (in) 
			{
				if (fread(&c,1,1,in) == 0) 
				{
					++currentFileLine;	// for debugging error messages
					maxFileLine = currentFileLine;
					break;	// EOF
				}
			}
			else
			{
				c = *userRecordSourceBuffer++;
				if (c == 0) 
					break;					// end of string - otherwise will be \n
			}
			if (c != '\n' && c != '\r') holdc = c; // failed to find lf... hold this character for later but ignoring 2 cr in a row
			if (c == '\n') // legal
			{
				++currentFileLine;	// for debugging error messages
				maxFileLine = currentFileLine;
			}
			if (blockComment) 
			{
				if (endingBlockComment) 
				{
					endingBlockComment = blockComment = false;
					buffer = start;
					*buffer = 0;
				}
				continue;
			}
			*buffer++ = '\r'; // put in the cr
			*buffer++ = '\n'; // put in the newline

			break;
		}
		else if (c == '\n')  // came without cr?
		{
			++currentFileLine;	// for debugging error messages
			maxFileLine = currentFileLine;
			if (formatString)
			{
				formatString = IN_FORMAT_CONTINUATIONLINE;
				while (*--buffer == ' '){;}
				*++buffer = ' '; // single space at line split
				++buffer;
				continue;
			}

			if (buffer == start) // read nothing as content
			{
				*start = 0;
				if ( in == stdin || returnEmptyLines) return 0;
				continue;
			}
			if (blockComment) 
			{
				if (endingBlockComment) 
				{
					endingBlockComment =  blockComment = false;
					buffer = start;
					*buffer = 0;
				}
				continue; // ignore
			}
			//   add missing \r
			*buffer++ = '\r';  // legal 
			*buffer++ = '\n';  // legal 

			break;	
		}
		*buffer++ = c; 
		*buffer = 0;
	
		// strip UTF8 BOM marker if any and just keep reading
		if (hasutf && currentFileLine == 0 && (buffer-start) == 3) // only from file system 
		{
			if ((unsigned char) start[0] ==  0xEF && (unsigned char) start[1] == 0xBB && (unsigned char) start[2] == 0xBF) // UTF8 BOM
			{
				buffer -= 3;
				*start = 0;
				BOM = BOMUTF8;
				hasutf = false;
			}
		}

		// handle block comments
		if ((buffer-start) >= 4 && *(buffer-3) == '#' && *(buffer-4) == '#' && *(buffer-2) == c) //  block comment mode ## 
		{
			if ( c == '<') 
			{
				blockComment = true; 
				if (!fread(&c,1,1,in)) // see if attached language
				{
					++currentFileLine;	// for debugging error messages
					maxFileLine = currentFileLine;
					break;	// EOF
				}
				if (IsAlphaUTF8(c)) // read the language
				{
					char lang[100];
					char* current = buffer;
					char* at = lang;
					*at++ = c;
					while (fread(&c,1,1,in))
					{
						if (IsAlphaUTF8(c) && c != ' ') *at++ = c;
						else break;
					}
					*at = 0;
					// this is either junk or a language marker...
					if (!ConditionalReadRejected(lang,buffer,false)) blockComment = false; // this is legal
				}
				if (c == '\n')  // reached line end
				{
					++currentFileLine;	// for debugging error messages
					maxFileLine = currentFileLine;
				}
			}
			else if (c == '>') 
			{
				if (!blockComment) {;}// superfluous end
				else endingBlockComment = true;
			}
			else continue;
			buffer -= 4;
			*buffer = 0;
			if (buffer != start && !endingBlockComment) break;	// return what we have so far in the buffer before the comment
			continue;
		}
		if (blockComment && (buffer-start) >= 5) 
		{
			memmove(buffer-5,buffer-4,5);// erase as we go, keeping last four to detect block end ##>>
			--buffer;
			*buffer = 0;
		}

	} // end of read loop
	*buffer = 0;
	
	// see if conditional compile line...
	if (*start == '#' && IsAlphaUTF8(start[1]))
	{
		if (ConditionalReadRejected(start+1,buffer,true))
		{
			buffer = start;
			*start = 0;
			goto RESUME;
		}
	}

	if (endingBlockComment) c = 0; // force next line to be active to clean up
	if (blockComment && !c) 
	{
		endingBlockComment = blockComment = false;// end of file while handling block comment
		buffer = start;
		*buffer = 0;
	}

	if (buffer == start) // no content read (EOF)
	{
		*buffer = 0;
		buffer[1] = 0;
		buffer[2] = 0; // clear ahead to make it obvious we are at end when debugging
		return -1;
	}

	if (*(buffer-1) == '\n') buffer -= 2; // remove cr/lf if there
	*buffer = 0;
	buffer[1] = 0;
	buffer[2] = 1; //   clear ahead to make it obvious we are at end when debugging

	if (hasutf && BOM)  hasbadutf = AdjustUTF8(start, start - 1); // DO NOT ADJUST BINARY FILES
	if (hasbadutf && showBadUTF && !server)  
		Log(STDTRACELOG,(char*)"Bad UTF-8 %s at %d in %s\r\n",start,currentFileLine,currentFilename);
    return (buffer - start);
}

char* ReadQuote(char* ptr, char* buffer,bool backslash,bool noblank,int limit)
{ //   ptr is at the opening quote or a \quote... internal	" must have \ preceeding the start of a quoted expression
	//  kinds of quoted strings:
	//  a) simple  "this is stuff"  -- same as \"xxxxx" - runs to first non-backslashed quote so cant have freestanding quotes in it but \" is converted at script compile
	//  b) literal \"this is stuff"  - the system outputs the double-quoted item with its quotes, unmolested  runs to first non-backslashed quote so cant have freestanding quotes in it but \" will get converted
	//  c) formatted ^"this is stuff" - the system outputs the double-quoted thing, interpreted and without the enclosing quotes
	//  d) literal quote \" - system outputs the quote only (script has nothing or blank or tab or ` after it
	//  e) internal "`xxxxx`"  - argument to tcpopen pass back untouched stripping the markers on both ends - allows us to pay no attention to OTHER quotes within
    char c;
    int n = limit - 2;		// quote must close within this limit
	char* start = ptr;
	char* original = buffer;
	// "` is an internal marker of argument passed from TCPOPEN   "'arguments'" ) , return the section untouched as one lump 
	if (ptr[1] == ENDUNIT)
	{
		char* end = strrchr(ptr+2,ENDUNIT);
		if (end[1] == '"') // yes we matched the special argument format
		{
			memcpy(buffer,ptr+2,end-ptr-2);
			buffer[end-ptr-2] = 0;
			return end + 3; // RETURN PAST space, aiming on the )
		}
	}

	*buffer++ = *ptr;  // copy over the 1st char
	char ender = *ptr; 
	if (*ptr == '\\') 
	{
		*buffer++ = *++ptr;	//   swallow \ and "
		ender = *ptr;
	}

	while ((c = *++ptr) && c != ender && --n) 
    {
		if (c == '\\' && ptr[1] == ender) // if c is \", means internal quoted expression.
		{
			*buffer++ = '\\';	// preserver internal marking - must stay with string til last possible moment.
			*buffer++ = *++ptr;
		}
        else *buffer++ = c; 
		if ((buffer - original) > limit)  // overflowing
		{
			c = 0;
			break;
		}
    }
    if (n == 0 || !c) // ran dry instead of finding the end
	{	
		if (backslash) // probably a normal end quote with attached stuff
		{
			// try again, seeking only whitespace
			ptr = start - 1;	
			buffer = original;
			while (*ptr && !IsWhiteSpace(*ptr)) *buffer++ = *ptr++;
			*buffer = 0;
			return ptr;
		}
		*buffer = 0;
 		if (!n) Log(STDTRACELOG,(char*)"bad double-quoting?  %s %d %s - string size exceeds limit of %d\r\n",start,currentFileLine,currentFilename,limit);
		else Log(STDTRACELOG,(char*)"bad double-quoting1?  %s %d %s missing tail doublequote \r\n",start,currentFileLine,currentFilename);
		return NULL;	// no closing quote... refuse
	}

    // close with ending quote
    *buffer = ender;	
    *++buffer = 0; 
	return (ptr[1] == ' ') ? (ptr+2) : (ptr+1); // after the quote end and any space
}

char* ReadArgument(char* ptr, char* buffer,FunctionResult &result) //   looking for a single token OR a list of tokens balanced - insure we start non-space
{ //   ptr is some buffer before the arg 
#ifdef INFORMATION
Used for function calls, to read their callArgumentList. Arguments are not evaluated here. An argument is:
	1. a token (like $var or name or "my life as a rose" )
	2. a function call which runs from the function name through the closing paren
	3. a list of tokens (unevaluated), enclosed in parens like (my life as a rose )
	4. a ^functionarg
#endif
    char* start = buffer;
	result = NOPROBLEM_BIT;
    *buffer = 0;
    int paren = 0;
	ptr = SkipWhitespace(ptr);
	char* beginptr = ptr;
	if (*ptr == '^')
    {
        ptr = ReadCompiledWord(ptr,buffer); // get name and prepare to peek at next token
		if (IsDigit(buffer[1]))  
		{
			strcpy(buffer,FNVAR(buffer+1)); // use the value and keep going // NEW
			return ptr;
		}
		else if (*ptr != '(')  return ptr; // not a function call
		else buffer += strlen(buffer); // read onto end of call
    }
	if (*ptr == '"' && ptr[1] == FUNCTIONSTRING && dictionaryLocked) // must execute it now...
	{
		return GetCommandArg(ptr, buffer,result,0); 
	}
	else if (*ptr == '"' || (*ptr == '\\' && ptr[1] == '"'))   // a string
	{
		ptr = ReadQuote(ptr,buffer);		// call past quote, return after quote
		if (ptr != beginptr) return ptr;	// if ptr == start, ReadQuote refused to read (no trailing ")
	}

	--ptr;
    while (*++ptr) 
    {
        char c = *ptr;
		int x = GetNestingData(c);
		if (paren == 0 && (c == ' ' || x == -1  || c == ENDUNIT) && ptr != beginptr) break; // simple arg separator or outer closer or end of data
        if ((unsigned int)(buffer-start) < (unsigned int)(maxBufferSize-2)) *buffer++ = c; // limit overflow into argument area
        *buffer = 0;
		if (x) paren += x;
    }
    if (start[0] == '"' && start[1] == '"' && start[2] == 0) *start = 0;   // NULL STRING
	if (*ptr == ' ') ++ptr;
    return ptr;
}

char* ReadCompiledWordOrCall(char* ptr, char* word,bool noquote,bool var) 
{
	ptr = ReadCompiledWord(ptr, word, noquote, var);
	word += strlen(word);
	if (*ptr == '(') // its a call
	{
		char* end = BalanceParen(ptr+1,true,false); // function call args
		strncpy(word,ptr,end-ptr);
		word += end-ptr;
		*word = 0;
		ptr = end;
	}
	return ptr;
}

char* ReadCompiledWord(char* ptr, char* word,bool noquote,bool var,bool nolimit) 
{//   a compiled word is either characters until a blank, or a ` quoted expression ending in blank or nul. or a double-quoted on both ends or a ^double quoted on both ends
	*word = 0;
	if (!ptr) return NULL;

	char c = 0;
	char* original = word;
	ptr = SkipWhitespace(ptr);
	char* start = ptr;
	char special = 0;
	if (!var) {;}
	else if ((*ptr == SYSVAR_PREFIX && IsAlphaUTF8(ptr[1]))  || (*ptr == '@' && IsDigit(ptr[1])) || (*ptr == USERVAR_PREFIX && (ptr[1] == LOCALVAR_PREFIX || IsAlphaUTF8(ptr[1]) || ptr[1] == TRANSIENTVAR_PREFIX)) || (*ptr == '_' && IsDigit(ptr[1]))) special = *ptr; // break off punctuation after variable
	bool jsonactivestring = false;
	if (*ptr == FUNCTIONSTRING && ptr[1] == '\'') jsonactivestring = true;
	if (!noquote && (*ptr == ENDUNIT || *ptr == '"' || (*ptr == FUNCTIONSTRING && ptr[1] == '"') || jsonactivestring )) //   ends in blank or nul,  might be quoted, doublequoted, or ^"xxx" functional string or ^'xxx' jsonactivestring
	{
		if (*ptr == '^') *word++ = *ptr++;	// move to the actual quote
		*word++ = *ptr; // keep opener  --- a worst case is "`( \"grow up" ) " from a fact
		char end = *ptr++; //   skip the starter, noting what we need for conclusion
		while ((c = *ptr++)) // run til nul or matching `  -- may have INTERNAL string as well
		{
			if ((word-original) > (MAX_WORD_SIZE - 3)) break;
			if ( c == '\\' && *ptr == end) // escaped quote, copy both
			{
				*word++ = c;
				*word++ = *ptr++;
				continue;
			}
			else if (c == end && (*ptr == ' ' || nestingData[(unsigned char)*ptr] == -1 || *ptr == 0)) // a terminator followed by white space or closing bracket or end of data terminated
			{
				if (*ptr == ' ') ++ptr;
				break; // blank or nul after closer
			}
			*word++ = c;
		}
		if (c) *word++ = end; // add back closer
		else // didnt find close, it was spurious... treat as normal
		{
			ptr = start;
			word = original;
			while ((c = *ptr++) && c != ' ') 
			{
				if ((word-original) > (MAX_WORD_SIZE - 3)) break;
				*word++ = c; // run til nul or blank
			}
		}
	}
	else 
	{
		bool quote = false;
		bool bracket = false;
		char priorchar = 0;
		while ((c = *ptr++) && c != ENDUNIT) 
		{
			if (c == ' ') break;
			// PAY NO SPECIAL ATTENTION TO NON-STARTING QUOTEMARKS, ESP 69"
			if (special) // try to end a variable if not utf8 char or such
			{
				if (special == '$' && (c == '.' || c == '[') && (LegalVarChar(*ptr) || *ptr == '$' )) {;} // legal data following . or [
				else if (special == '$' &&  c == ']' && bracket) {;} // allowed trailing array close
				else if (special == '$' && c == '$' && (priorchar == '.' || priorchar == '[' || priorchar == '$' || priorchar == 0)){;} // legal start of interval variable or transient var continued
				else if ((special == '%' || special == '_' || special == '@') && priorchar == 0) {;}
				else if (!LegalVarChar(c)) 
					break;
				if (c == '[' && !bracket) bracket = true;
				else if (c == ']' && bracket) bracket = false;
			}

			if (!nolimit && (word-original) > (MAX_WORD_SIZE - 3)) break;
			*word++ = c; //   run til nul or blank or end of rule 
			priorchar = c;
		}
	}
	*word = 0; //   null terminate word
	if (!c || special) --ptr; //   shouldnt move on to valid next start (preknown to have space before it, but user string uncompiled might not)
	return ptr;	
}

char* BalanceParen(char* ptr,bool within,bool wildcards) // text starting with ((unless within is true), find the closing ) and point to next item after
{
	int paren = 0;
	if (within) paren = 1;
	--ptr;
	bool quoting = false;
    while (*++ptr && *ptr != ENDUNIT) // jump to END of command to resume here, may have multiple parens within
    {
		if (*ptr == '\\' && ptr[1]) // ignore slashed item
		{
			++ptr;
			continue;
		}
		if ( *ptr == '"') 
		{
			quoting =  !quoting;
			continue;
		}
		if (quoting) continue;	// stuff in quotes is safe 
		if (wildcards && *ptr == '_' && !IsDigit(ptr[1]) && *(ptr-1) == ' ')
		{
			SetWildCardNull();
		}
		int value = nestingData[(unsigned char)*ptr];
		if (*ptr == '<' && ptr[1] == '<' && ptr[2] != '<') value = 1;
		if (*ptr == '>' && ptr[1] == '>' && ptr[2] != '>') 
		{
			value = -1;
			++ptr; // ignore 1st one
		}

		if (value && (paren += value) == 0) 
		{
			ptr += (ptr[1] && ptr[2]) ? 2 : 1; //   return on next token (skip paren + space) or  out of data (typically argument substitution)
			break;
		}
    }
    return ptr; //   return on end of data
}

char* SkipWhitespace(char* ptr)
{
    if (!ptr || !*ptr) return ptr;
    while (IsWhiteSpace(*ptr)) ++ptr;
    return ptr; 
}

///////////////////////////////////////////////////////////////
//  CONVERSION UTILITIES
///////////////////////////////////////////////////////////////

char* Purify(char* msg) // used for logging to remove real newline characters so all fits on one line
{
	if (newline) return msg; // allow newlines to remain

	char* nl = strchr(msg,'\n');  // legal 
	if (!nl) return msg; // no problem
	char* limit;
	char* buffer = InfiniteStack(limit,"Purify"); // transient
	strcpy(buffer,msg);
	nl = (nl - msg) + buffer;
	while (nl)
	{
		*nl = ' ';
		nl = strchr(nl,'\n');  // legal 
	}
	char* cr = strchr(buffer,'\r');  // legal 
	while (cr)
	{
		*cr = ' ';
		cr = strchr(cr,'\r');  // legal 
	}
	ReleaseInfiniteStack();
	return buffer; // nothing else should use ReleaseStackspace 
}

size_t OutputLimit(unsigned char* data) // insert eols where limitations exist
{
	char extra[HIDDEN_OVERLAP+1];
	strncpy(extra,((char*)data) + strlen((char*)data),HIDDEN_OVERLAP); // preserve any hidden data on why and serverctrlz

	unsigned char* original = data;
	unsigned char* lastBlank = data;
	unsigned char* lastAt = data;
	--data;
	while( *++data) 
	{
		if (data > lastAt && (unsigned int)(data - lastAt) > outputLength)
		{
			memmove(lastBlank+2,lastBlank+1,strlen((char*)lastBlank));
			*lastBlank++ = '\r'; // legal
			*lastBlank++ = '\n'; // legal
			lastAt = lastBlank;
			++data;
		}
		if (*data == ' ') lastBlank = data;
		else if (*data == '\n') lastAt = lastBlank = data+1; // internal newlines restart checking
	}
	strncpy(((char*)data) + strlen((char*)data), extra, HIDDEN_OVERLAP);
	return data - original;
}

unsigned int UTFStrlen(char* ptr)
{
	unsigned int len = 0;
	while (*ptr)
	{
		len++;
		ptr += UTFCharSize(ptr);
	}
	return len;
}

unsigned int UTFPosition(char* ptr, unsigned int pos)
{
	unsigned int index = 0;
	unsigned int posChar = 0;
	while (*ptr && index++ < pos)
	{
		unsigned int charLen = UTFCharSize(ptr);
		posChar += charLen;
		ptr += charLen;
	}
	return posChar;
}

unsigned int UTFOffset(char* ptr, char* c)
{
	unsigned int index = 0;
	unsigned int posChar = 0;
	while (*ptr && ptr < c)
	{
		ptr += UTFCharSize(ptr);
		index++;
	}
	return index;
}

char* UTF2ExtendedAscii(char* bufferfrom) 
{
	char* limit;
	unsigned char* buffer = (unsigned char*) InfiniteStack(limit,"UTF2ExtendedAscii"); // transient
	unsigned char* bufferto = buffer;
	while( *bufferfrom && (size_t)(bufferto-buffer) < (size_t)(maxBufferSize-10)) // avoid overflow on local buffer
	{
		if (*bufferfrom != 0xc3) *bufferto++ = *bufferfrom++;
		else 
		{
			++bufferfrom;
			unsigned char x = *bufferfrom++;
			unsigned char val = utf82extendedascii[x - 128];
			if (val) *bufferto++ = val + 128;
			else // we dont know this character
			{
				*bufferto++ = (unsigned char) 0xc3;
				*bufferto++ = *(bufferfrom-1);
			}
		}
	}
	*bufferto = 0;
	if (outputLength) OutputLimit(buffer);
	ReleaseInfiniteStack();
	return (char*) buffer; // it is ONLY good for (*printer) immediate, not for anything long term
}

void ForceUnderscores(char* ptr)
{
	--ptr;
	while (*++ptr) if (*ptr == ' ') *ptr = '_';
}

void ConvertQuotes(char* ptr)
{
	--ptr;
	bool start = false;
	while (*++ptr)
	{
		if (*ptr == '"' && *(ptr - 1) != '\\')
		{
			memmove(ptr + 2, ptr + 1, strlen(ptr + 1) + 1); // make room for utf8 replacement
			if (!start) *ptr = '"';
			if (start) *ptr = '"';
			start = !start;
		}
	}
}

void Convert2Blanks(char* ptr)
{
	--ptr;
	while (*++ptr) 
	{
		if (*ptr == '_') 
		{
			if (ptr[1] == '_') memmove(ptr,ptr+1,strlen(ptr));// convert 2 __ to 1 _  (allows web addressess as output)
			else *ptr = ' ';
		}
	}
}

void ConvertNL(char* ptr)
{
	char* start = ptr;
	--ptr;
	char c;
	while ((c = *++ptr)) 
    {
		if (c == '\\')
		{
			if (ptr[1] == 'n') 
			{
				if (*(ptr-1) != '\r') // auto add \r before it
				{
					*ptr = '\r'; // legal
					*++ptr = '\n'; // legal
				}
				else
				{
					*ptr = '\n'; // legal
					memmove(ptr+1,ptr+2,strlen(ptr+2)+1);
				}
			}
			else if (ptr[1] == 'r')
			{
				*ptr = '\r'; // legal
				memmove(ptr+1,ptr+2,strlen(ptr+2)+1);
			}
			else if (ptr[1] == 't')
			{
				*ptr = '\t'; // legal
				memmove(ptr+1,ptr+2,strlen(ptr+2)+1);
			}
		}
	}
}

void Convert2Underscores(char* output)
{ 
    char* ptr = output - 1;
	char c;
	if (ptr[1] == '"') // leave this area alone
	{
		ptr += 2;
		while (*++ptr && *ptr != '"');
	}
    while ((c = *++ptr)) 
    {
		if (c == '_' && ptr[1] != '_') // remove underscores from apostrophe of possession
        {
			// remove space on possessives
			if (ptr[1] == '\'' && ( (ptr[2] == 's' && !IsAlphaUTF8OrDigit(ptr[3]))  || !IsAlphaUTF8OrDigit(ptr[2]) ) )// bob_'s  bobs_'  
			{
				memmove(ptr,ptr+1,strlen(ptr)); 
				--ptr;
			}
		}
    }
}

void RemoveTilde(char* output)
{ 
    char* ptr = output - 1;
	char c;
	if (ptr[1] == '"') // leave this area alone
	{
		ptr += 2;
		while (*++ptr && *ptr != '"');
	}
    while ((c = *++ptr)) 
    {
		if (c == '~' && IsAlphaUTF8(ptr[1]) && (ptr == output || (*(ptr-1)) == ' ')  ) //   REMOVE leading ~  in classnames
        {
			memmove(ptr,ptr+1,strlen(ptr)); 
			--ptr;
        }
    }
}

int64 NumberPower(char* number, int useNumberStyle)
{
    if (*number == '-') return 2000000000;    // isolated - always stays in front
	int64 num = Convert2Integer(number, useNumberStyle);
	if (num < 10) return 1;
	if (num < 100) return 10;
	if (num < 1000) return 100;
	if (num < 10000) return 1000;
	if (num < 100000) return 10000;
	if (num < 1000000) return 100000;
	if (num < 10000000) return 1000000;
	if (num < 100000000) return 10000000;
	if (num < 1000000000) return 100000000; // max int is around 4 billion
	return  10000000000ULL; 
}

int64 Convert2Integer(char* number, int useNumberStyle)  //  non numbers return NOT_A_NUMBER    
{  // ProcessCompositeNumber will have joined all number words together in appropriate number power order: two hundred and fifty six billion and one -> two-hundred-fifty-six-billion-one , while four and twenty -> twenty-four
	if (!number || !*number) return NOT_A_NUMBER;
	char decimalMark = decimalMarkData[useNumberStyle];
	char digitGroup = digitGroupingData[useNumberStyle];
	char c = *number;
	if (c == '$'){;}
	else if (c == '#' && IsDigitWord(number+1,useNumberStyle)) return Convert2Integer(number+1, useNumberStyle);
	else if (!IsAlphaUTF8DigitNumeric(c) || c == decimalMark) return NOT_A_NUMBER; // not  0-9 letters + - 

	bool csEnglish = (externalTagger && stricmp(language, "english")) ? false : true;

	size_t len = strlen(number);
	uint64 valx;
	if (IsRomanNumeral(number,valx)) return (int64) valx;
	if (IsDigitWithNumberSuffix(number, useNumberStyle)) // 10K  10M 10B or currency
	{
		char d = number[len-1];
		number[len-1] = 0;
		int64 answer = Convert2Integer(number, useNumberStyle);
		if (d == 'k' || d == 'K') answer *= 1000;
		else if (d == 'm' || d == 'M') answer *= 1000000;
		else if (d == 'B' || d == 'b' || d == 'G' || d == 'g') answer *= 1000000000;
		number[len-1] = d;
		return answer;
	}
	
	//  grab sign if any
	char sign = *number;
	if (sign == '-' || sign == '+') ++number;
	else if (sign == '$')
	{
		sign = 0;
		++number;
	}
	else sign = 0;

     //  make canonical: remove commas (user typed a digit number with commas) and convert _ to -
    char copy[MAX_WORD_SIZE];
     MakeLowerCopy(copy+1,number); 
	*copy = ' '; // safe place to look behind at
	char* comma = copy;
	while (*++comma)
	{
		if (*comma == digitGroup) memmove(comma,comma+1,strlen(comma));
		else if (*comma == '_') *comma = '-';
	}
    char* word = copy+1;

	// remove place suffixes
	if (csEnglish && numberValues && len > 3 && !stricmp(word+len-3,(char*)"ies")) // twenties?
	{
		char xtra[MAX_WORD_SIZE];
		strcpy(xtra,word);
		strcpy(xtra+len-3,(char*)"y");
		size_t len1 = strlen(xtra);
		for (unsigned int i = 0; i < 100000; ++i)
		{
			int wordIndex = numberValues[i].word;
			if (!wordIndex) break;
			if (len1 == numberValues[i].length && !strnicmp(xtra,Index2Heap(wordIndex),len1))
			{
				if (numberValues[i].realNumber == FRACTION_NUMBER || numberValues[i].realNumber == REAL_NUMBER) 
				{
					strcpy(word,xtra);
					len = len1;
				}
				break;  
			}
		}
	}

    if (csEnglish && numberValues && len > 3 && word[len-1] == 's') // if s attached to a fraction, remove it
	{
		size_t len1 = len - 1;
		if (word[len1-1] == 'e') --len1; // es ending like zeroes
		// look up direct word number as single
		for (unsigned int i = 0; i < 1000; ++i)
		{
			int index = numberValues[i].word;
			if (!index) break;
			if (len1 == numberValues[i].length && !strnicmp(word,Index2Heap(index),len1)) 
			{
				if (numberValues[i].realNumber == FRACTION_NUMBER || numberValues[i].realNumber == REAL_NUMBER)
				{
					word[len1] = 0; //  thirds to third    quarters to quarter  fifths to fith but not ones to one
					len = len1;
				}
				break;  
			}
		}
	}
	unsigned int oldlen = len;
	// remove
	if (!csEnglish);
    else if (len < 3); // cannot have suffix
    else if (word[len-2] == 's' && word[len-1] == 't' && !strstr(word,(char*)"first")) word[len -= 2] = 0; // 1st 
    else if (word[len-2] == 'n' && word[len-1] == 'd' && !strstr(word,(char*)"second") && !strstr(word,(char*)"thousand")) word[len -= 2] = 0; // 2nd but not second or thousandf"
    else if (word[len-2] == 'r' && word[len-1] == 'd' && !strstr(word,(char*)"third")) word[len -= 2] = 0; // 3rd 
	else if (word[len-2] == 't' && word[len-1] == 'h' && !strstr(word,(char*)"fifth")) //  excluding the word "fifth" which is not derived from five
	{
		word[len -= 2] = 0; 
		if (word[len-1] == 'e' && word[len-2] == 'i') // twentieth and its ilk
		{
			word[--len - 1] = 'y';
			word[len] = 0;
		}
	}
	if (oldlen != len && (word[len-1] == '-' || word[len-1] == '\'')) word[--len] = 0; // trim off separator
	bool hasDigit = IsDigit(*word);
	char* hyp = strchr(word,'-');
	if (hyp) *hyp = 0;
	if (hasDigit) // see if all digits now.
	{
		char* ptr = word-1;
		while (*++ptr)
		{
			if (ptr != word && *ptr == '-') {;}
			else if (*ptr == '-' || *ptr == '+') return NOT_A_NUMBER;
			else if (!IsDigit(*ptr)) return NOT_A_NUMBER;	// not good
		}
		if (!*ptr && !hyp) 
		{
			if (strlen(word) > 18) return NOT_A_NUMBER; // very long sequence of digits that cannot be stored in 64bits
			return atoi64(word) * ((sign == '-') ? -1 : 1);	// all digits with sign
		}
	
		// try for digit sequence 9-1-1 or whatever
		ptr = word;
		if (hyp) *hyp = '-';
		int64 value = 0;
		while (IsDigit(*ptr))
		{
			value *= 10;
			value += *ptr - '0';
			if (*++ptr != '-') break;	// 91-55
			++ptr;
		}
		if (!*ptr) return value;	// dont know what to do with it
	}

	if (hyp) *hyp = '-';

	// look up direct word numbers
	if (!hasDigit && numberValues) for (unsigned int i = 0; i < 1000; ++i)
	{
		int index = numberValues[i].word;
		if (!index) break;
		char* w = Index2Heap(index);
		if (len == numberValues[i].length && !strnicmp(word, w, len))
		{
			return numberValues[i].value;  // a match (but may be a fraction number)
		}
	}

    // try for hyphenated composite
 	char*  hyphen = strchr(word,'-'); 
	if (!hyphen) hyphen = strchr(word,'_'); // alternate form of separation
    if (!hyphen || !hyphen[1])  return NOT_A_NUMBER;   // not multi part word
   	if (hasDigit && IsDigit(hyphen[1])) return NOT_A_NUMBER; // cannot hypenate a digit number but can do mixed "1-million" is legal
 
	// if lead piece is not a number, the whole thing isnt
	c = *hyphen;
	*hyphen = 0;
	int64 val = Convert2Integer(word, useNumberStyle); // convert lead piece to see if its a number
	*hyphen = c;
	if (val == NOT_A_NUMBER) return NOT_A_NUMBER; // lead is not a number

	// val is now the lead number
	
	// check if whole thing is a series of digits in any language
	int64 num = val;
	int64 val1;
	char* oldhyphen = hyphen + 1;
	char* xpiece = hyphen;
	while ((xpiece = strchr(xpiece + 1,'-')))
	{
		num *= 10;
		*xpiece = 0;
		val1 = Convert2Integer(oldhyphen, useNumberStyle);
		*xpiece = '-';
		if (val1 > 9)
		{
			num = -1;
			break; // not a sequence of digits
		}
		num += val1;
		oldhyphen = xpiece + 1;
	}
	if (hyphen && num != -1) // do last piece
	{
		if ((num % 10) > 0) num *= 10; // don't multiply if twenty-three
		val1 = Convert2Integer(oldhyphen, useNumberStyle);
		if (val1 > 9) num = -1;
		else num += val1;
		return num;
	}

	if (num >= 0 && num < 10) // simple digit
	{
		num *= 10;
		val1 = Convert2Integer(oldhyphen, useNumberStyle);
		if (val1 < 10)
		{
			num += val1;
			return num;
		}
	}

	// decode powers of ten names on 2nd pieces
    long billion = 0;
    char* found = strstr(word+1,(char*)"billion"); // eight-billion 
    if (found && *(found-1) == '-') // is 2nd piece
    {
        *(found-1) = 0; // hide the word billion
        billion = (int)Convert2Integer(word, useNumberStyle);  // determine the number of billions
        if (billion == NOT_A_NUMBER && stricmp(word,(char*)"zero") && *word != '0') return NOT_A_NUMBER;
        word = found + 7; // now points to next part
        if (*word == '-' || *word == '_') ++word; // has another hypen after it
    }
	else if (val == 1000000000) 
	{
		val = 0; 
		billion = 1;
		if (hyphen) word = hyphen + 1;
	}

	hyphen = strchr(word,'-'); 
	if (!hyphen) hyphen = strchr(word,'_'); // alternate form of separation
    long million = 0;
    found = strstr(word,(char*)"million");
    if (found && *(found-1) == '-')
    {
        *(found-1) = 0;
        million = (int)Convert2Integer(word, useNumberStyle); 
        if (million == NOT_A_NUMBER && stricmp(word,(char*)"zero") && *word != '0') return NOT_A_NUMBER;
        word = found + 7;
        if (*word == '-' || *word == '_') ++word; // has another hypen after it
    }
	else if (val == 1000000) 
	{
		val = 0; 
		million = 1;
		if (hyphen) word = hyphen + 1;
	}

	hyphen = strchr(word,'-'); 
	if (!hyphen) hyphen = strchr(word,'_'); // alternate form of separation
	long thousand = 0;
    found = strstr(word,(char*)"thousand");
    if (found && *(found-1) == '-')
    {
        *(found-1) = 0;
        thousand = (int)Convert2Integer(word, useNumberStyle);
        if (thousand == NOT_A_NUMBER && stricmp(word,(char*)"zero") && *word != '0') return NOT_A_NUMBER;
        word = found + 8;
		if (*word == '-' || *word == '_') ++word; // has another hypen after it
    }  
	else if (val == 1000) 
	{
		val = 0; 
		thousand = 1;
		if (hyphen) word = hyphen + 1;
	}

	hyphen = strchr(word,'-'); 
	if (!hyphen) hyphen = strchr(word,'_'); // alternate form of separation
    long hundred = 0;
    found = strstr(word,(char*)"hundred");  
    if (found && *(found-1) == '-') // do we have four-hundred
    {
        *(found-1) = 0;
        hundred = (int) Convert2Integer(word, useNumberStyle);
        if (hundred == NOT_A_NUMBER && stricmp(word,(char*)"zero") && *word != '0') return NOT_A_NUMBER;
        word = found + 7;
 		if (*word == '-' || *word == '_') ++word; // has another hypen after it
    }  
	else if (val == 100) 
	{
		val = 0; 
		hundred = 1;
		if (hyphen) word = hyphen + 1;
	}

	// now do tens and ones, which can include omitted hundreds label like two-fifty-two
	bool isFrench = (!stricmp(language, "french")) ? true : false;
	hyphen = strchr(word,'-');
	if (!hyphen) hyphen = strchr(word,'_'); 
    int64 value = 0;
    while (word && *word) // read each smaller part and scale
    {
        if (!hyphen) // last piece (a tens or a ones)
		{
            if (!strcmp(word,number)) return NOT_A_NUMBER;  // never decoded anything so far
            int64 n = Convert2Integer(word, useNumberStyle);
            if (n == NOT_A_NUMBER && stricmp(word,(char*)"zero") && *word != '0') return NOT_A_NUMBER;
            value += n; // handled LAST piece
            break;
        }

		// Find the longest item that is a single number
		// French has composite base numbers, e.g. quatre-vingt-un is 81 not 421, and quatre-vingt-dix-neuf is 99
		int64 piece1 = NOT_A_NUMBER;
		char* hyphen2 = word;
		while ((hyphen2 = strchr(hyphen2, '-')))
		{
			char c = *hyphen2;
			*hyphen2 = 0;
			int64 val2 = Convert2Integer(word, useNumberStyle); // convert lead piece to see if its a number
			*hyphen2 = c;
			if (val2 == NOT_A_NUMBER) break;
			hyphen = hyphen2++;
			piece1 = val2;
		}

        *hyphen++ = 0; // split pieces

		// split 3rd piece if one exists
        char* next = strchr(hyphen,'-');       
		if (!next) next = strchr(hyphen,'_');    
        if (next) *next = 0; 

        if (piece1 == NOT_A_NUMBER && stricmp(word,(char*)"zero") && *word != '0') return NOT_A_NUMBER;

        int64 piece2 = Convert2Integer(hyphen, useNumberStyle);   
        if (piece2 == NOT_A_NUMBER && stricmp(hyphen,(char*)"0")) return NOT_A_NUMBER;

        int64 subpiece = 0;
		if (piece1 > piece2 && piece2 < 10) subpiece = piece1 + piece2; // can be larger-smaller (like twenty one) 
		if (isFrench && (piece1 == 60 || piece1 == 80) && piece2 < 20) subpiece = piece1 + piece2; // French 60+ or 80+ 
		else if (piece2 >= 10 && piece2 < 100 && piece1 >= 1 && piece1 < 100) subpiece = piece1 * 100 + piece2; // two-fifty-two is omitted hundred
		else if (piece2 == 10 || piece2 == 100 || piece2 == 1000) subpiece = piece1 * piece2; // must be smaller larger pair (like four hundred)
        value += subpiece; // 2 pieces mean item was  power of ten and power of one
        // if 3rd piece, now recycle to handle the ones part
		if (next) ++next;
        word = next;
        hyphen = NULL; 
	}
	
	return value + ((int64)billion * 1000000000) + ((int64)million * 1000000) + ((int64)thousand * 1000) + ((int64)hundred * 100);
}


void MakeLowerCase(char* ptr)
{
	--ptr;
	while (*++ptr)
	{
		char utfcharacter[10];
		char* x = IsUTF8(ptr, utfcharacter); // return after this character if it is valid.
		if (utfcharacter[1] && *ptr == 0xc3 && (unsigned char)ptr[1] >= 0x80 && (unsigned char)ptr[1] <= 0x9e)
		{
			unsigned char c = (unsigned char)*++ptr; // get the cap form
			if (c != 0x97) // multiplication sign doesn't change
			{
				c -= 0x80;
				c += 0xa0; // get lower case form
			}
			*ptr = (char)c;
		}
		else if (utfcharacter[1] && *ptr >= 0xc4 && *ptr <= 0xc9)
		{
			unsigned char c = (unsigned char)*++ptr;
			*ptr = (char)c | 1;
		}
		else if (utfcharacter[1]) ptr = x - 1;
		else *ptr = GetLowercaseData(*ptr);
	}
}

void MakeUpperCase(char* ptr)
{
	--ptr;
	while (*++ptr)
	{
		char utfcharacter[10];
		char* x = IsUTF8(ptr, utfcharacter); // return after this character if it is valid.
		if (utfcharacter[1] && *ptr == 0xc3 && ptr[1] >= 0x9f && ptr[1] <= 0xbf)
		{
			unsigned char c = (unsigned char)*++ptr; // get the cap form
			if (c != 0x9f && c != 0xb7) // sharp s and division sign don't change
			{
				c -= 0xa0;
				c += 0x80; // get upper case form
			}
			*ptr = (char)c;
		}
		else if (utfcharacter[1] && *ptr >= 0xc4 && *ptr <= 0xc9)
		{
			unsigned char c = (unsigned char)*++ptr; // get the cap form
			*ptr = (char)c & 0xfe;
		}
		else if (utfcharacter[1]) ptr = x - 1;
		else *ptr = GetUppercaseData(*ptr);
	}
}

char*  MakeLowerCopy(char* to, char* from)
{
	char* start = to;
	while (*from)
	{
		char utfcharacter[10];
		char* x = IsUTF8(from, utfcharacter); // return after this character if it is valid.
		if (utfcharacter[1] && *from == 0xc3 && from[1] >= 0x80 && from[1] <= 0x9e)
		{
			*to++ = *from;
			unsigned char c = from[1]; // get the cap form
			if (c != 0x97) // multiplication sign doesn't change
			{
				c -= 0x80;
				c += 0xa0; // get lower case form
			}
			*to++ = c;
			from = x;
		}
		else if (utfcharacter[1] && *from >= 0xc4 && *from <= 0xc9)
		{
			*to++ = *from;
			*to++ = from[1] | 1; // lowercase from cap
			from = x;
		}
		else if (utfcharacter[1])
		{
			strcpy(to, utfcharacter);
			to += strlen(to);
			from = x;
		}
		else *to++ = GetLowercaseData(*from++);
	}
	*to = 0;
	return start;
}

char* MakeUpperCopy(char* to, char* from)
{
	char* start = to;
	while (*from)
	{
		char utfcharacter[10];
		char* x = IsUTF8(from, utfcharacter); // return after this character if it is valid.
		if (utfcharacter[1] && *from == 0xc3 && from[1] >= 0x9f && from[1] <= 0xbf)
		{
			*to++ = *from;
			unsigned char c = from[1]; // get the cap form
			if (c != 0x9f && c != 0xb7) // sharp s and division sign don't change
			{
				c -= 0xa0;
				c += 0x80; // get upper case form
			}
			*to++ = c;
			from = x;
		}
		else if (utfcharacter[1] && (*from >= 0xc4 && *from <= 0xc9))
		{
			*to++ = *from;
			*to++ = from[1] & 0xfe; // uppercase from small
			from = x;
		}
		else if (utfcharacter[1])
		{
			strcpy(to, utfcharacter);
			to += strlen(to);
			from = x;
		}
		else *to++ = GetUppercaseData(*from++);
	}
	*to = 0;
	return start;
}

char* TrimSpaces(char* msg,bool start)
{
	char* orig = msg;
	if (start) while (IsWhiteSpace(*msg)) ++msg;
	size_t len = strlen(msg);
	if (len == 0) // if all blanks, shift to null at start
	{
		msg = orig;
		*msg= 0;
	}
	while (len && IsWhiteSpace(msg[len-1])) msg[--len] = 0;
	return msg;
}

void UpcaseStarters(char* ptr) //   take a multiword phrase with _ and try to capitalize it correctly (assuming its a proper noun)
{
    if (IsLowerCase(*ptr)) *ptr -= 'a' - 'A';
    while (*++ptr)
    {
        if (!IsLowerCase(*++ptr) || *ptr != '_') continue; //   word separator
		if (!strnicmp(ptr,(char*)"the_",4) || !strnicmp(ptr,(char*)"of_",3) || !strnicmp(ptr,(char*)"in_",3) || !strnicmp(ptr,(char*)"and_",4)) continue;
		*ptr -= 'a' - 'A';
    }
}
	
char* documentBuffer = 0;

bool ReadDocument(char* inBuffer,FILE* sourceFile)
{
	static bool wasEmptyLine = false;
RETRY: // for sampling loopback
	*inBuffer = 0;
	if (!*documentBuffer || singleSource) 
	{
		while (ALWAYS)
		{
			if (ReadALine(documentBuffer,sourceFile) < 0)  
			{
				wasEmptyLine = false;
				return false;	// end of input
			}
			if (!*SkipWhitespace(documentBuffer)) // empty line
			{
				if (wasEmptyLine) continue;	// ignore additional empty line
				wasEmptyLine = true;
				if (docOut) fprintf(docOut,(char*)"\r\n%s\r\n",inBuffer);
				return true; // no content, just null line or all white space
			}

			if (*documentBuffer == ':' && IsAlphaUTF8(documentBuffer[1]))
			{
				if (!stricmp(documentBuffer,(char*)":exit") || !stricmp(documentBuffer,(char*)":quit"))
				{
					wasEmptyLine = false;
					return false;	
				}
			}
			break; // have something
		}
	}
	else if (*documentBuffer == 1) // holding an empty line from before to return
	{
		*documentBuffer = 0;
		wasEmptyLine = true;
		if (docOut) fprintf(docOut,(char*)"\r\n%s\r\n",inBuffer);
		return true;
	}

	unsigned int readAhead = 0;
	while (++readAhead < 4) // read up to 3 lines
	{
		// pick earliest punctuation that can terminate a sentence
		char* period = NULL;
		char* at = documentBuffer;
		char* oob = strchr(documentBuffer,OOB_START); // oob marker
		if (oob && (oob - documentBuffer) < 5) oob = strchr(oob+3,OOB_START); // find a next one // seen at start doesnt count.
		if (!oob) oob = documentBuffer + 300000;	 // way past anything
		while (!period && (period = strchr(at,'.')))	// normal input end here?
		{
			if (period > oob) break; 
			if (period[1] && !IsWhiteSpace(period[1])) 
			{
				if (period[1] == '"' || period[1] == '\'') // period shifted inside the quote
				{
					++period;	 // report " or ' as the end so whole thing gets tokenized
					break;	
				}
				at = period + 1;	// period in middle of something
			}
			else if (IsDigit(*(period-1))) 
			{
				// is this number at end of sentence- e.g.  "That was in 1992. It became obvious."
				char* next = SkipWhitespace(period+1);
				if (*next && IsUpperCase(*next)) break;
				else break;	// ANY number ending in a period is an end of sentence. Assume no one used period unless they want a digit after it
				//at = period + 1; // presumed a number ending in period, not end of sentence
			}
			else if (IsWhiteSpace(*(period-1))) break; // isolated period
			else // see if token it ends is reasonable
			{
				char* before = period;
				while (before > documentBuffer && !IsWhiteSpace(*--before));	// find start of word
				char next2 = (period[1]) ? *SkipWhitespace(period+2) : 0;
				if (ValidPeriodToken(before+1, period+1, period[1],next2) == TOKEN_INCLUSIVE) at = period + 1;	// period is part of known token
				else break;
			}
			period = NULL;
		}
		char* question = strchr(documentBuffer,'?');
		if (question && (question[1] == '"' || question[1] == '\'') && question < oob) ++question;	// extend from inside a quote
		char* exclaim = strchr(documentBuffer,'!');
		if (exclaim && (exclaim[1] == '"' || exclaim[1] == '\'') && exclaim < oob) ++exclaim;	// extend from inside a quote
		if (!period && question) period = question;
		else if (!period && exclaim) period = exclaim;
		if (exclaim && exclaim < period) period = exclaim;
		if (question && question < period) period = question;

		// check for things other than sentence periods as well here
		if (period) // found end of a sentence before any oob
		{
			char was = period[1];
			period[1] = 0;
			strcpy(inBuffer,SkipWhitespace(documentBuffer)); // copied over to input
			period[1] = was;
			memmove(documentBuffer,period+1,strlen(period+1) + 1);	// rest moved up in holding area
			break;
		}
		else if (singleSource) // only 1 line at a time regardless of it not being a complete sentence
		{
			strcpy(inBuffer,SkipWhitespace(documentBuffer));
			break;	
		}
		else // has no end yet
		{
			char* at = SkipWhitespace(documentBuffer);
			if (*at == OOB_START)  // starts with OOB, add no more lines onto it.
			{
				strcpy(mainInputBuffer,at);
				*documentBuffer = 0;
				break;
			}

			if (oob < (documentBuffer + 300000)) // we see oob data
			{
				if (at != oob) // stuff before is auto terminated by oob data
				{
					*oob = 0;
					strcpy(inBuffer,at);
					*oob = OOB_START;
					memmove(documentBuffer,oob,strlen(oob)+1);
					break;
				}
			}

			size_t len = strlen(documentBuffer);
			documentBuffer[len] = ' '; // append 1 blanks
			ReadALine(documentBuffer + len + 1,sourceFile,maxBufferSize - 4 - len);	//	 ahead input and merge onto current
			if (!documentBuffer[len+1]) // logical end of file
			{
				strcpy(inBuffer,SkipWhitespace(documentBuffer));
				*documentBuffer = 1;
				break;
			}
		}
	}

	// skim the file
	if (docSampleRate)
	{
		if (--docSample != 0) goto RETRY;	// decline to process
		docSample = docSampleRate;
	}

	if (readAhead >= 6)
		Log(STDTRACELOG,(char*)"Heavy long line? %s\r\n",documentBuffer);
	if (autonumber) Log(ECHOSTDTRACELOG,(char*)"%d: %s\r\n",inputSentenceCount,inBuffer);
	else if (docstats)
	{
		if ((++docSentenceCount % 1000) == 0)  Log(ECHOSTDTRACELOG,(char*)"%d: %s\r\n",docSentenceCount,inBuffer);
	}
	wasEmptyLine = false;
	if (docOut) fprintf(docOut,(char*)"\r\n%s\r\n",inBuffer);

	return true;
}

