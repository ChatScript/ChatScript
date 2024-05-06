#include "common.h"


typedef struct InternationalPhones
{
	const char* countrydigits; 
	const char* concept;
	const int countrycode;		
	const int areacode;			
	const int exchangecode; // variable digits counts are put here
	const int subscriber;
} InternationalPhones;

InternationalPhones phoneData[] = {
	{"1",  "~usinternational",1,3,3,4},
	{"44","~ukinternational",2,0,3,6},
	{"49","~deinternational",2,0,3,4}, 
	{"81","~jpinternational",2,0,3,4},
	{"34","~esinternational",2,3,3,3},
	{"52","~mxinternational",2,2,4,4},
	NULL
};


typedef struct JapanCharInfo
{
	const char* charword;		//  japanese 3 byte character
	const unsigned char convert;		// ascii equivalent
} JapanCharInfo;
bool outputapicall = false;
char* currentInput = NULL; // latest input we are processing from GetNextInput for error reporting
static HEAPREF startSupplementalInput = NULL;
unsigned int startSentence;
bool moreToComeQuestion = false;				// is there a ? in later sentences
bool moreToCome = false;						// are there more sentences pending
unsigned int endSentence;
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
unsigned char hasHighChar = 0;
char conditionalCompile[MAX_CONDITIONALS + 1][50];
int conditionalCompiledIndex = 0;
char numberComma = ',';
char numberPeriod = '.';

char tmpWord[MAX_WORD_SIZE];					// globally visible scratch word
char* userRecordSourceBuffer = 0;				// input source for reading is this text stream of user file
unsigned char toHex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
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
	int word;				//   word of a number index
	unsigned int length;	//   length of word
	int64 value;			//   value of word
	int realNumber;			//	 the type: DIGIT_NUMBER(1), CURRENCY_NUMBER, PLACETYPE_NUMBER, ROMAN_NUMBER, WORD_NUMBER,
} NUMBERDECODE;

static NUMBERDECODE* numberValues[MAX_MULTIDICT]; // multilanguage

static JapanCharInfo japanSet[] =
{
	// ranged characters - only substituted in all variables
	{"\xEF\xBC\xA1",(unsigned char)'A'}, // "Ａ"
	{"\xEF\xBC\xBA",(unsigned char)'Z'}, //"Ｚ" 
	{"\xEF\xBD\x81",(unsigned char)'a'}, // "ａ" 
	{"\xEF\xBD\x9A",(unsigned char)'z'}, // "ｚ" 
	{"\xEF\xBC\x90",(unsigned char)'0'}, // "０" 
	{"\xEF\xBC\x99",(unsigned char)'9' },// "９" 
	// one-off characters  always substituted in variables
	{"\xE3\x80\x82",(unsigned char)'.'}, // "。" ideographic_full_stop, code point U+3002       
	{"\xEF\xBC\x8E", (unsigned char)'.'}, // "．" fullwidth full stop, code point U+FF0E  
	{"\xEF\xBC\x8D", (unsigned char)'-'},// "－" fullwidth hyphen-minus, code point U+FF0D uft8 
	{"\xEF\xBC\xBF", (unsigned char)'_'}, // "＿"fullwidth low line, code point U+FF3F 
	{"\xEF\xBC\x8E", (unsigned char)'.'}, // "。"  full width stop 
	{"\xEF\xBD\xA1", (unsigned char)'.'}, // "｡" jp half-period stop
    // From https://en.wikipedia.org/wiki/List_of_Japanese_typographic_symbols
	{0,0}
};

static JapanCharInfo japanControlSet[] = // always change pattern and output
{
	{ "\xE3\x80\x80",(unsigned char)' ' },// "　" ideographic space, code point U+3000  
	{ "\xEF\xBC\x88", (unsigned char)'(' }, // "（", jp open paren, U+FF08
	{ "\xEF\xBC\x89", (unsigned char)')' }, // "）", jp close paren, U+FF09
	{ "\xEF\xBD\x9B", (unsigned char)'{' }, // "｛", jp open curly brace, U+FF5B
	{ "\xEF\xBD\x9D", (unsigned char)'}' }, // "｝", jp close curly brace, U+FF5D
	{ "\xEF\xBC\xBB", (unsigned char)'(' }, // "［", jp open bracket, U+FF3B
	{ "\xEF\xBC\xBD", (unsigned char)')' }, // "］", jp close bracket, U+FF3D
	{ 0,0 }
};

typedef struct CURRENCYDECODE
{
	int word; // word index of currency abbrev
	int concept;	// word index of concept to mark it
} CURRENCYDECODE;

static CURRENCYDECODE* currencies[MAX_MULTIDICT];
static int* monthnames[MAX_MULTIDICT];
static int* topleveldomains;
static int* emojishortnames;

unsigned char toLowercaseData[256] = // convert upper to lower case
{  // encoder/decoder https://mothereff.in/utf-8
	// table of values site:
	// https://www.utf8-chartable.de/unicode-utf8-table.pl?start=12288

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
	190,191,192,193 ,194 ,195 ,196 ,197 ,198 ,199 ,	200 ,201 ,202 ,203 ,204 ,205 ,206 ,207 ,208 ,209 ,
	210 ,211,212 ,213 ,214 ,215 ,216 ,217 ,218 ,219 ,	220 ,221 ,222,223,224,225,226,227,228,229,
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
	190,191,192-32,193 ,194 ,195 ,196 ,197 ,198 ,199 ,	200 ,201 ,202 ,203 ,204 ,205 ,206 ,207 ,208 ,209 ,
	210 ,211 ,212 ,213 ,214 ,215 ,216 ,217 ,218 ,219 ,	220 ,221 ,222-32,223,224,225,226,227,228,229,
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
	BRACKETS,BRACKETS,ARITHMETICS | QUOTERS , ARITHMETICS,PUNCTUATIONS, ARITHMETICS | ENDERS,ARITHMETICS | ENDERS,ARITHMETICS,0,0, //   () * + ,  - .  /
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

unsigned char isAlphabeticDigitData[256] = //    non-digit number starter (+-.) == 1 isdigit == 2 isupper == 3 leower == 4 isletter >= 3 utf8 == 5
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

void Clear_CRNL(char* incoming)
{
	char* at = incoming;
	while ((at = strchr(at, '\r')))
	{
		*at = ' ';
		if (at[1] == '\n') at[1] = ' '; // if had one, likely have the other
	}
	at = incoming;
	while ((at = strchr(at, '\n'))) *at = ' ';
}

void InitUniversalTextUtilities()
{
	ClearNumbers();
	memset(utf82extendedascii, 0, 128);
	extendedascii2utf8[0xe1 - 128] = 0x9f; // german fancy ss
	for (unsigned int i = 0; i < 128; ++i)
	{
		unsigned char c = extendedascii2utf8[i];
		if (c) c -= 128;	// remove top bit
		utf82extendedascii[c] = (unsigned char)i;
	}

	char word[MAX_WORD_SIZE];
	char* limit;
	char* stack;
	int* data;
	FILE* in;
	size_t size;
	char file[SMALL_WORD_SIZE];
	// universal system livedata
	sprintf(word, (char*)"%s/systemessentials.txt", systemFolder);
	ReadSubstitutes(word, 0, NULL, DO_ESSENTIALS, true);

	sprintf(word, (char*)"%s/queries.txt", systemFolder);
	ReadQueryLabels(word);

	// most up to date list of all TLDs at http://data.iana.org/TLD/tlds-alpha-by-domain.txt
	sprintf(file, (char*)"%s/topleveldomains.txt", systemFolder);
	in = FopenStaticReadOnly(file);
	if (in)
	{
		stack = InfiniteStack(limit, "inittlds");
		data = (int*)stack;

		while (ReadALine(readBuffer, in) >= 0) // top level domain
		{
			if (*readBuffer == '#' || *readBuffer == 0) continue;
			ReadCompiledWord(readBuffer, word);
			*data++ = Heap2Index(AllocateHeap(word, 0));
		}
		*data++ = 0;     // terminal value for end detection
		size = (char*)data - stack;
		size /= 8;     // 64bit chunks
		topleveldomains = (int*)AllocateHeap(stack, size + 1, 8);
		ReleaseInfiniteStack();
		FClose(in);
	}

	// create the empty word
	StoreWord("", AS_IS);
}

void Translate_UTF16_2_UTF8(char* input)
{
	char* output = input--;
	while (*++input)
	{
		char c = *input;
		if (c == '\\' && input[1] == 'u' && IsHexDigit(input[2]) && IsHexDigit(input[3]) && IsHexDigit(input[4]) && IsHexDigit(input[5]))
		{
			char* utf8 = UTF16_2_UTF8(input + 1, false);
			if (utf8)
			{
				size_t len = strlen(utf8);
				input += 5; // skip what we read
				// shrink to fit hole
				memmove(input + len, input + 6, strlen(input + 5));
				strncpy(input, utf8, len);
				input += len - 1;
			}
		}
		// else leave input/output alone
	}
}

char* UTF16_2_UTF8(char* in,bool withinquote)
{ // u0000 notation
	static char answer[20];
	*answer = 'x'; //default
	answer[1] = 0;
	//First code point	Last code point	Byte 1	Byte 2	Byte 3	Byte 4
				// direct utf16 to ascii converts

	int n = atoi(in + 1);
	if (n == 2019 || n == 2018)
	{
		return "\'";// left and right curly ' to normal '
	}
	char d = in[4];
	if (n == 201 && (d == 'c' || d == 'd' || d == 'C' || d == 'D'))
	{
		if (withinquote) return "\\\""; // escape it
		return "\""; // left and right curly dq to normal dq
	}
	if (n == 22)  return "\\\""; // std ascii
	if (n == 27)  return "\\'"; // std quote	if (n == 27)  return "'"; // std quote	if (n == 27)  return "\\'"; // std quote	if (n == 27)  return "'"; // std quote
	if (n == 2013 || n == 2014)  // preserve m and n dash for now for regression test
	{
		return NULL; // decline to change
	}
	//	U + 0080	U + 07FF		110xxxxx	10xxxxxx
	//	U + 0800	U + FFFF	1110xxxx	10xxxxxx	10xxxxxx
	//  U + 10000[nb 2]U +	10FFFF	11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
	if (*in != 'u' ) return answer;  // not legal

	// 20AC is binary 0010 0000 1010 1100.
	unsigned char c = in[1];
	if (c < 'a' && c >= 'A') c -= 'A' + 'a'; // flip to lowercase if uppercase
	unsigned char hex1 = (IsDigit(c)) ? (c - '0') : (10 + c - 'a');

	c = in[2];
	if (c < 'a' && c >= 'A') c -= 'A' + 'a';
	unsigned char hex2 = (IsDigit(c)) ? (c - '0') : (10 + c - 'a');

	c = in[3];
	if (c < 'a' && c >= 'A') c -= 'A' + 'a';
	unsigned char hex3 = (IsDigit(c)) ? (c - '0') : (10 + c - 'a');

	c = in[4];
	if (c < 'a' && c >= 'A') c -= 'A' + 'a';
	unsigned char hex4 = (IsDigit(c)) ? (c - '0') : (10 + c - 'a');

	// private use codepoints
	// The main range in the BMP is U + E000..U + F8FF, containing 6, 400 private - use characters.That range is often referred to as the Private Use Area(PUA).But there are also two large ranges of supplementary private - use characters, consisting of most of the code points on Planes 15 and 16: U + F0000..U + FFFFD and U + 100000..U + 10FFFD.Together those ranges allocate another 131, 068 private - use characters.Altogether, then, there are 137, 468 private - use characters in Unicode.
	if (hex1 == 0x0d || hex1 == 0x0e) return NULL; // utf16 emoji which may be considered unallocated for utf8
	if (hex1 == 0x0f && hex2 < 0x09) return NULL; // private utf16
	if (hex1 == 0 && hex2 == 0 && hex3 <= 7) //	U + 00 00 00 7F		0xxxxxxx
	{// 1 byte utf (normal ascii)
		*answer = (hex3 << 4) + hex4;
		answer[1] = 0;
	}
	else if (hex1 == 0 && hex2 <= 7) // 2  byte message 0080-07FF	110xxxxx	10xxxxxx
	{
		answer[0] = 0xc0 | (hex2 << 2) | (hex3 >> 2); // 3+2 = 5 bit
		answer[1] = 0x80 | ((hex3 & 0x03) << 4) | hex4; // 2+4 = 6bit 
		answer[2] = 0;
	}
	else if ((hex1 == 8 && hex2 >= 8) || hex1) // 3 byte message 0800-FFFF	1110xxxx	10xxxxxx	10xxxxxx	
	{
		answer[0] = 0xe0 | hex1;										// 4 = 4 bit
		answer[1] = 0x80 | (hex2 << 2) | (hex3 >> 2);	// 4 + 2bit = 6 bit
		answer[2] = 0x80 | ((hex3 & 0x03) << 4) | hex4; // 2+4 = 6 bit
		answer[3] = 0;
	}
	else if ((hex1 == 0 && hex2 == 0) || (hex1 == 0 && hex2 <= 7)) // 4 byte message
	{// WRONG? BUG 
		// 10000-10FFFF		11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
		// disable
		answer[0] = 'x';
		answer[1] = 0;
	}
	else
	{
		*answer = 'x'; //default
		answer[1] = 'x'; //default
		answer[2] = 0;
	}
	return answer;
}


bool IsComparator(char* word)
{
	return (!stricmp(word, (char*)"&=") || !stricmp(word, (char*)"|=") || !stricmp(word, (char*)"^=") || !stricmp(word, (char*)"=") || !stricmp(word, (char*)"+=") || !stricmp(word, (char*)"-=") || !stricmp(word, (char*)"/=") || !stricmp(word, (char*)"*="));
}

void ClearNumbers()
{
	memset(numberValues,0,sizeof(numberValues)); // heap is discarded, need to discard this
	memset(currencies, 0, sizeof(currencies));
}

char* ReadTokenMass(char* ptr, char* word)
{
	ptr = SkipWhitespace(ptr);
	while (*ptr && *ptr != ' ') *word++ = *ptr++;	// find end of word
	*word = 0;
	return ptr;
}


bool LegalVarChar(char at)
{
	return  (IsAlphaUTF8OrDigit(at) || at == '_' || at == '-');
}

void InitTextUtilitiesByLanguage(char* lang)
{
	char word[MAX_WORD_SIZE];
	char value[MAX_WORD_SIZE];
	char kind[MAX_WORD_SIZE];
	char* limit;
	char* stack;
	int* data;
	size_t size;
	char file[SMALL_WORD_SIZE];

	sprintf(file, (char*)"%s/%s/numbers.txt", livedataFolder, lang);
	FILE* in = FopenStaticReadOnly(file); // LIVEDATA substitutes or from script TOPIC world
	if (in)
	{
		stack = InfiniteStack(limit, "initnumbers");
		data = (int*)stack;
		// WORD_NUMBER is a word that implies a number value, like dozen
		// ORDINAL_NUMBER is word_number and a real number that is a place number like first, second, etc
		// REAL_NUMBER is a word that directly represents a number, like two
		// FRACTION_NUMBER is a word that implies a faction value like half
		while (ReadALine(readBuffer, in) >= 0) // once 1 REALNUMBER
		{
			if (*readBuffer == '#' || *readBuffer == 0) continue;
			char* ptr = ReadCompiledWord(readBuffer, word);
			ptr = ReadCompiledWord(ptr, value);
			ptr = ReadCompiledWord(ptr, kind);
			int64 val;
			ReadInt64(value, val);
			int kindval = 0;
			uint64 flags = ADJECTIVE | NOUN | ADJECTIVE_NUMBER | NOUN_NUMBER; 
			// if (!stricmp(kind, "DIGIT_NUMBER")) kindval = DIGIT_NUMBER;
			// else 
			if (!stricmp(kind, "FRACTION_NUMBER")) kindval = FRACTION_NUMBER; // a fractional value like quarter
			else if (!stricmp(kind, "WORD_NUMBER")) kindval = WORD_NUMBER; //implies a number value, like dozen
			else if (!stricmp(kind, "REAL_NUMBER")) kindval = REAL_NUMBER; //directly represents a number, like two
			else if (!stricmp(kind, "ORDINAL_NUMBER")) //place number like first
			{
				kindval = PLACETYPE_NUMBER;
				flags |= PLACE_NUMBER;
			}
			else myexit("Bad number table");
			char* w = StoreFlaggedWord(word, AS_IS | flags, NOUN_NODETERMINER)->word;
			int index = Heap2Index(w);
			*data++ = index;
			*data++ = strlen(word);
			*(int64*)data = val; // int64 value
			data += 2;
			*data++ = kindval;
			*data++ = 0; // alignment to keep block at int64 style
		}
		*data++ = 0;	 // terminal value for end detection
		size = (char*)data - stack;
		size /= 8;	 // 64bit chunks
		numberValues[languageIndex] = (NUMBERDECODE*)AllocateHeap(stack, size + 1, 8);
		ReleaseInfiniteStack();
		FClose(in);
	}

	sprintf(file, (char*)"%s/%s/currencies.txt", livedataFolder, lang);
	in = FopenStaticReadOnly(file);
	if (in)
	{
		stack = InfiniteStack(limit, "initcurrency");
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
		currencies[languageIndex] = (CURRENCYDECODE*)AllocateHeap(stack, size + 1, 8);
		ReleaseInfiniteStack();
		FClose(in);
	}

	sprintf(file, (char*)"%s/%s/months.txt", livedataFolder, lang);
	in = FopenStaticReadOnly(file);
	if (in)
	{
		stack = InfiniteStack(limit, "initmonths");
		data = (int*)stack;

		while (ReadALine(readBuffer, in) >= 0) // month name or abbrev
		{
			if (*readBuffer == '#' || *readBuffer == 0) continue;
			ReadCompiledWord(readBuffer, word);
			char* w = StoreWord(word, AS_IS)->word; // month name
			int index = Heap2Index(w);
			*data++ = index;
		}
		*data++ = 0;	 // terminal value for end detection
		size = (char*)data - stack;
		size /= 8;	 // 64bit chunks
		monthnames[languageIndex] = (int*)AllocateHeap(stack, size + 1, 8);
		ReleaseInfiniteStack();
		FClose(in);
	}

	// emoji short names, see https://unicode.org/Public/emoji/13.0/emoji-sequences.txt
	// or https://raw.githubusercontent.com/omnidan/node-emoji/master/lib/emoji.json
	sprintf(file, (char*)"%s/emojishortnames.txt", systemFolder);
	in = FopenStaticReadOnly(file);
	if (in)
	{
		stack = InfiniteStack(limit, "initemoji");
		data = (int*)stack;

		while (ReadALine(readBuffer, in) >= 0) // short name
		{
			if (*readBuffer == '#' || *readBuffer == 0) continue;
			ReadCompiledWord(readBuffer, word);
			// strip any colons
			char* ptr = word;
			if (*ptr == ':')
			{
				++ptr;
				size_t len = strlen(ptr) - 1;
				if (ptr[len] == ':') {
					ptr[len] = 0;
				}
			}
			*data++ = Heap2Index(AllocateHeap(ptr, 0));
		}
		*data++ = 0;     // terminal value for end detection
		size = (char*)data - stack;
		size /= 8;     // 64bit chunks
		emojishortnames = (int*)AllocateHeap(stack, size + 1, 8);
		ReleaseInfiniteStack();
		fclose(in);
	}
}

char* FindJMSeparator(char* ptr, char c)
{
	char* at = ptr - 1;
	bool quote = false;
	while (*++at)
	{
		if (*at == '"') quote = !quote;
		if (!quote && *at == c) return at; // found terminator not in quotes
	}
	return NULL;
}

void CopyParam(char* to, char* from, unsigned int limit)
{
	size_t len = strlen(from);
	if (*from == '"')
	{
		++from;
		--len;
		if (from[len - 1] == '"') --len;
	}
	if (len > limit) len = limit - 1; // truncate silently
	for (unsigned int i = 0; i < len; ++i)
	{
		if (from[i] == '\\') ++i;
		*to++ = from[i];
	}
	*to = 0;
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
		else if (*original != separator && separatorcount == 2 && !alpha) return true; //when there is already a date but still have different seperators.(eg: 18-02-2019.)
		else if (*original != separator) return false; // cannot be
		if (separator && separator != '/' && separator != '-' && separator != '.') return false;  // 10:30:00-05:00 is not a date

		if (*original == separator) // start a new block
		{
			if (digitcount == 4)  // only 1 year is allowed
			{
				if (digit4) return false; // only 1 of these
				digit4 = true;
			}
			else if (digitcount > 2) return false; // not legal count, must be 1,2 or 4 digits

			if (acceptalpha == false && ++alphacount > 1) return false;	// only one text month is allowed
			acceptalpha = true;

			digitcount = 0;
			if (++separatorcount > 2) return false; // only 2 separators (3 components) allowed
		}
	}
	if (!separator) return false;
	if (!alpha)
	{
		return (separatorcount == 2);
	}

	if (separatorcount < 1) return false; // 2 or 3 allowed

										  // confirm its a month
	int* name = monthnames[languageIndex];
	for (int i = 0; i < 1000; ++i)
	{
		if (!name || !*name) return false;
		char* w = Index2Heap(*name);
		name++;
		if (*w != toUppercaseData[*alpha]) continue;
		size_t n = strlen(w);
		if (strnicmp(w, alpha, n)) continue;
		if (alpha[n] == 0 || alpha[n] == separator) return true;
	}
	return false;
}

void ClearSupplementalInput()
{
	startSupplementalInput = NULL;
}

void CloseTextUtilities()
{
}

bool IsFraction(char* token)
{
	if (IsDigit(token[0])) // fraction?
	{
		char* at = strchr(token, '/');
		if (at)
		{
			at = token;
			while (IsDigit(*++at)) { ; }
			if (*at == '/' && (at[1] != '0' || at[2]))
			{
				while (IsDigit(*++at)) { ; }
				if (!*at) return true;
			}
		}
	}
	return false;
}

bool IsAllUpper(char* ptr)
{
    bool foundDigit = false;
    bool foundNonDigit = false;
	--ptr;
	while (*++ptr)
	{
		if (!IsUpperCase(*ptr) && *ptr != '&' && *ptr != '-' && *ptr != '_' && !IsDigit(*ptr)) break;
        if (IsDigit(*ptr)) foundDigit = true;
        else foundNonDigit = true;
	}
    if (foundDigit && !foundNonDigit) return false;  // can't be all digits
	return (!*ptr);
}

void ConvertJapanText(char* output,bool pattern)
{
	char* base = output--;
	bool variable = false;
	while (*++output)
	{
		int index = output - base;
		unsigned char c = *output;
		int kind = IsLegalNameCharacter(c); // 1==utf8 0=non letter/digit
		
		// universally replace [] () {}
		int k = -1;
		JapanCharInfo* fn;
		if (kind == 1) // is utf8 based
		{
			while ((fn = &japanControlSet[++k]) && fn->charword)
			{
				if (!strncmp((char*)output, fn->charword, 3))
				{
					memmove(output + 1, output + 3, strlen(output + 2));
					*output = fn->convert;
					// stuff ends variables unless its json access $x[5]
					if (*output != '[' && *output != ']') variable = false;
					else if (variable && *output == ']') // is it closing a variable or merely arrayref
					{
						char* at = output;
						while (*--at != USERVAR_PREFIX)
						{
							if (*at == '[') break; // opening []
						}
						if (*at == USERVAR_PREFIX)  variable = false; 
					}
					break;
				}
			}
			if (fn->charword) continue; // we already made a subsitution
		}

		// all variables need . - _ letters and digits converted from japanese to ascii
		if (c == USERVAR_PREFIX) variable = true;
		else if (variable && c == ']') // is it closing a variable or merely arrayref
		{
			char* at = output;
			while (*--at != USERVAR_PREFIX)
			{
				if (*at == '[') break; // opening []
			}
			if (*at == USERVAR_PREFIX)  variable = false;
		}
		else if (!kind ) variable = false; // not utf8 and not var legal
		else if (pattern || (variable && kind == 1)) // convert to ascii only
		{
			k = -1;
			while ((fn = &japanSet[++k]) && fn->charword)
			{
                if (k < 6) // ranged
                {
                    if (pattern && !variable) continue; // dont convert ranged values except in variables
                    // within range?
                    unsigned int endval = (unsigned int)output[2];
                    int lower = strncmp(output, fn->charword, 3);
                    int upper = strncmp(output, japanSet[k + 1].charword, 3);
                    if (lower >= 0 && upper <= 0) // in range
                    {
                        memmove(output + 1, output + 3, strlen(output + 2) );
                        *output = (char)(fn->convert + (endval - (unsigned int)fn->charword[2]));
                        ++k; // only check a range once at start
                        break;
                    }
                    ++k; // only check a range once at start
                    continue;
                }
                else if (!strncmp((char*)output, fn->charword, 3)) // non range
                {
                    memmove(output + 1, output + 3, strlen(output + 2));
                    *output = fn->convert;
                    break;
                }
			}
			if (!fn->charword) variable = false; // didnt match legal vardata
		}
	}
}

char* RemoveEscapesWeAdded(char* at)
{
	if (!at || !*at) return at;
	char* startScan = at;
	while ((at = strchr(at, '\\'))) // we always add ESCAPE_FLAG backslash   
	{
		if (*(at - 1) == ESCAPE_FLAG) // alter the escape in some way
		{
			if (at[1] == 't')  at[1] = '\t'; // legal 
			else if (at[1] == 'n') at[1] = '\n';  // legal 
			else if (at[1] == 'r')  at[1] = '\r';  // legal 
			// other choices dont translate, eg double quote
			memmove(at - 1, at + 1, strlen(at)); // remove the escape flag and the escape
		}
		else ++at; // we dont mark " with ESCAPE_FLAG, we are not protecting it.
	}
	return startScan;
}

char* CopyRemoveEscapes(char* to, char* at, int limit, bool allitems) // all includes ones we didnt add as well
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
			if (*(at - 1) == ESCAPE_FLAG) // dual escape is special marker, there is no escaped item
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
		else if (*at == '\\' && allitems) // remove ALL other escapes in addition to ones we put there
		{
			++at; // move to escaped item
			if (*at == 't') *to++ = '\t';
			else if (*at == 'n') *to++ = '\n';  // legal 
			else if (*at == 'r') *to++ = '\r';  // legal 
			else { *to++ = *at; } // just pass along untranslated
		}
		else *to++ = *at;
		if ((to - start) >= limit) // too much, kill it
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
	while ((p = strchr(buffer, '\r'))) *p = ' '; // legal
	while ((p = strchr(buffer, '\n'))) *p = ' '; // legal
	while ((p = strchr(buffer, '\t'))) *p = ' '; // legal
}

char* RemoveQuotes(char* item)
{
	char* q;
	while ((q = strchr(item, '"'))) memmove(q, q + 1, strlen(q));
	return item;
}

void ChangeSpecial(char* buffer)
{
	char* limit;
	char* buf = InfiniteStack(limit, "ChangeSpecial");
	AddEscapes(buf, buffer, true, maxBufferSize);
	strcpy(buffer, buf);
	ReleaseInfiniteStack();
}

static int UTF8_2_UTF16(unsigned char*& from, unsigned char*& to)
{
	int size = 1;
	//U + 0000	U + 007F					0xxxxxxx
	//U + 0080	U + 07FF					110xxxxx	10xxxxxx
	//U + 0800	U + FFFF				1110xxxx	10xxxxxx	10xxxxxx
	//U + 10000[nb 2]U + 10FFFF	11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
	if (*from < 0x80) *to++ = *from; // 0xxxxxxx
	else if ((*from & 0xE0) == 0xc0)  // 110xxxxx	10xxxxxx  11 bits
	{
		*to++ = '\\';
		*to++ = 'u';
		*to++ = '0';
		*to++ = '0';
		unsigned char c0 = *from++ & 0x1f;
		unsigned char c1 = *from & 0x3f;
		unsigned int u16 = (c0 << 6) | c1;
		*to++ = toHex[u16 >> 4];
		*to++ = toHex[u16 & 0x0f];
		*to = 0;
		size = 2;
	}
	else if ((*from & 0xf0) == 0xe0)  // 1110xxxx	  10xxxxxx	10xxxxxx
	{
		//The Unicode code point for "€" is U + 20AC.
		// The three bytes 11100010 10000010 10101100 => E2 82 AC.
		*to++ = '\\';
		*to++ = 'u';
		unsigned char cx0 = *from++ & 0x0f;
		unsigned char cx1 = *from++ & 0x3f;
		unsigned char cx2 = *from & 0x3f;
		unsigned int utf16 = cx2 | (cx1 << 6) | (cx0 << 12);
		unsigned char c1 = (unsigned char)(utf16 >> 12);
		unsigned char c2 = (utf16 >> 8) & 0x0f;
		unsigned char c3 = (utf16 >> 4) & 0x0f;
		unsigned char c4 = utf16 & 0x0f;
		*to++ = toHex[c1];
		*to++ = toHex[c2];
		*to++ = toHex[c3];
		*to++ = toHex[c4];
		*to = 0;
		size = 3;
	}
	else if ((*from & 0xf8) == 0xf0) // 11110xxx	10xxxxxx	10xxxxxx	10xxxxxx
	{
		*to++ = '\\';
		*to++ = 'u';
		unsigned char c0 = *from++ & 0x07;
		unsigned char c1 = *from++ & 0x3f;
		unsigned char c2 = *from++ & 0x3f;
		unsigned char c3 = *from & 0x3f;
		unsigned int u16 = (c0 << 12) | (c1 << 6) | c2;
		*to++ = toHex[u16 >> 12];
		*to++ = toHex[(u16 >> 8) & 0x0f];
		*to++ = toHex[(u16 >> 4) & 0x0f];
		*to++ = toHex[u16 & 0x0f];
		*to = 0;
		size = 4;
	}
	else // illegal
	{
		*to++ = 'x';
	}
	return size;
}

int IsJapanese(char* utf8letter,unsigned char* utf16letter,int& kind)
{ // 3 or 4 bytes
	kind = 0;
	if (!(*utf8letter & 0x80)) return 0; // not utf8 but extended ascii could be mistaken
	unsigned char* utfbase = utf16letter;
	int size = UTF8_2_UTF16((unsigned char* &)utf8letter, utf16letter);
	
	if (utfbase[2] == '3' && utfbase[3] == '0' && utfbase[4] < '4') kind = JAPANESE_PUNCTUATION;
	// Japanese - style punctuation(3000 - 303f)
		//3000   　  、  。  〃  〄  々  〆  〇  〈  〉  《  》  「  」  『  』
		//3010   【  】  〒  〓  〔  〕  〖  〗  〘  〙  〚  〛  〜  〝  〞  〟
		//3020   〠  〡  〢  〣  〤  〥  〦  〧  〨  〩  〪  〫  〬  〭  〮  〯
		//3030   〰  〱  〲  〳  〴  〵  〶  〷  〸  〹  〺  〻  〼  〽  〾  〿
	else if (utfbase[2] == 'F' && utfbase[3] == 'F' && utfbase[4] == '0' && utfbase[5] == '1') kind = JAPANESE_PUNCTUATION; // full width !
	else if (utfbase[2] == 'F' && utfbase[3] == 'F' && utfbase[4] == '0' && utfbase[5] == 'E') kind = JAPANESE_PUNCTUATION; // full width .
	else if (utfbase[2] == 'F' && utfbase[3] == 'F' && utfbase[4] == '1' && utfbase[5] == 'F') kind = JAPANESE_PUNCTUATION; // full width ?
	else if (utfbase[2] == 'F' && utfbase[3] == 'F' ) kind = JAPANESE_KANJI; // ff00-ffEF are half-full width chars and FFF0-FFFF are specials
	else if (utfbase[2] == 'F' && utfbase[3] == 'E'  && (utfbase[4] == '3' || utfbase[4] == '4'))  kind = JAPANESE_KANJI; // CJK Compatibility Forms
	else if (utfbase[2] == 'F' && (utfbase[3] == '9' ||  utfbase[3] == 'A'))  kind = JAPANESE_KANJI; //  F900 FAFF	 CJK Compatibility Ideographs
	else if (utfbase[2] == '3' && utfbase[3] == '0' && utfbase[4] <= '9') kind = JAPANESE_HIRAGANA; 
	// Hiragana(3040 - 309f)
		//3040   ぀  ぁ  あ  ぃ  い  ぅ  う  ぇ  え  ぉ  お  か  が  き  ぎ  く
		//3050   ぐ  け  げ  こ  ご  さ  ざ  し  じ  す  ず  せ  ぜ  そ  ぞ  た
		//3060   だ  ち  ぢ  っ  つ  づ  て  で  と  ど  な  に  ぬ  ね  の  は
		//3070   ば  ぱ  ひ  び  ぴ  ふ  ぶ  ぷ  へ  べ  ぺ  ほ  ぼ  ぽ  ま  み
		//3080   む  め  も  ゃ  や  ゅ  ゆ  ょ  よ  ら  り  る  れ  ろ  ゎ  わ
		//3090   ゐ  ゑ  を  ん  ゔ  ゕ  ゖ  ゗  ゘  ゙  ゚  ゛  ゜  ゝ  ゞ  ゟ
	else if (utfbase[2] == '3' && utfbase[3] == '0' && utfbase[4] > '9') kind = JAPANESE_KATAKANA;
	// Katakana ( 30a0 - 30ff)
		//30a0   ゠  ァ  ア  ィ  イ  ゥ  ウ  ェ  エ  ォ  オ  カ  ガ  キ  ギ  ク
		//30b0   グ  ケ  ゲ  コ  ゴ  サ  ザ  シ  ジ  ス  ズ  セ  ゼ  ソ  ゾ  タ
		//30c0   ダ  チ  ヂ  ッ  ツ  ヅ  テ  デ  ト  ド  ナ  ニ  ヌ  ネ  ノ  ハ
		//30d0   バ  パ  ヒ  ビ  ピ  フ  ブ  プ  ヘ  ベ  ペ  ホ  ボ  ポ  マ  ミ
		//30e0   ム  メ  モ  ャ  ヤ  ュ  ユ  ョ  ヨ  ラ  リ  ル  レ  ロ  ヮ  ワ
		//30f0   ヰ  ヱ  ヲ  ン  ヴ  ヵ  ヶ  ヷ  ヸ  ヹ  ヺ  ・  ー  ヽ  ヾ  ヿ
	else if (utfbase[2] < '4') return 0;
	else if (utfbase[2] >= '4' && utfbase[2] <= '9' ) kind = JAPANESE_KANJI;
	// CJK unifed ideographs - Common and uncommon kanji ( 4e00 - 9faf)
	else if (utfbase[2] == '3' && utfbase[3] >= '4') kind = JAPANESE_KANJI;
	else if (utfbase[2] == '4' && utfbase[3] <= 'D') kind = JAPANESE_KANJI;
	else if (utfbase[2] == '4' && utfbase[3] == 'D' && utfbase[4] <= 'B') kind = JAPANESE_KANJI;
	// CJK unified ideographs Extension A - Rare kanji ( 3400 - 4dbf)	
	else return 0;
	return size; // the found letter and size
}

char* AddEscapes(char* to, const char* from, bool normal, int limit, bool addescapes, bool convert2utf16) // normal true means dont flag with extra markers
{
	limit -= 200; // dont get close to limit
	char* start = to;
	char* at = (char*) (from - 1);
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
		// detect it is already escaped
		else if (*at == '\\')
		{
			const char* at1 = at + 1;
            if (*at1 && (*at1 == 'n' || *at1 == 'r' || *at1 == 't' || *at1 == '"' || *at1 == '\\' || (*at1 == 'u' && IsHexDigit(at[2]) && IsHexDigit(at[3]) && IsHexDigit(at[4]) && IsHexDigit(at[5])) ))  // just pass it along
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
		else if (*at == '"' || (*at == '\\' && !addescapes)) { // we  need to preserve that it was escaped, though we always escape it in json anyway, because writeuservars needs to know
			if (!normal) *to++ = ESCAPE_FLAG;
			*to++ = '\\';
			*to++ = *at;
		}
		// translate utf8 to uxxxx notation 
#ifdef DISABLE_CLAUSE
		else if (false && convert2utf16 && *at & 0x80)
		{
			UTF8_2_UTF16((unsigned char*&)at, (unsigned char*&)to);
		}
#endif
        else if (*at < 32)
        {
            // ASCII control characters are not valid by themselves
            if (!normal) *to++ = ESCAPE_FLAG;
            *to++ = '\\';
            *to++ = 'u';
            sprintf(to, "%04x", *at);
            to += 4;
        }
		else *to++ = *at;
		if ((to - start) > limit && false) 	// dont overflow just abort silently
		{
			ReportBug((char*)"AddEscapes overflowing buffer");
			break;
		}
	}
	*to = 0;
	return to; // return where we ended
}

double Convert2Double(char* original, int useNumberStyle)
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

void AcquireDefines(const char* fileName)
{ // dictionary entries:  `xxxx (property names)  ``xxxx  (systemflag names)  ``` (parse flags values)  ````(misc including internalbits) -- and flipped:  `nxxxx and ``nnxxxx and ```nnnxxx with infermark being ptr to original name
	FILE* in = FopenStaticReadOnly(fileName); // SRC/dictionarySystem.h
	if (!in)
	{
		if (!server) (*printer)((char*)"%s", (char*)"Unable to read dictionarySystem.h\r\n");
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
	word[0] = UNIQUEENTRY;
	bool endsystem = false;
	while (ReadALine(readBuffer, in) >= 0)
	{
		uint64 result = NOPROBLEM_BIT;
		int64 value;
		if (!strnicmp(readBuffer, (char*)"// system flags", 15))  // end of property flags seen
		{
			word[1] = UNIQUEENTRY; // system flag words have `` in front
			offset = 2;
		}
		else if (!strnicmp(readBuffer, (char*)"// end system flags", 19))  // end of system flags seen
		{
			offset = 1;
		}
		else if (!strnicmp(readBuffer, (char*)"// parse flags", 14))  // start of parse flags seen, have ```
		{
			word[1] = UNIQUEENTRY; // parse flag words have ``` in front
			word[2] = UNIQUEENTRY; // parse flag words have ``` in front
			offset = 3;
		}
		else if (!strnicmp(readBuffer, (char*)"// end parse flags", 18))  // end of parse flags seen
		{
			word[1] = UNIQUEENTRY; // misc flag words have ```` in front and do not xref from number to word
			word[2] = UNIQUEENTRY; // misc flag words have ```` in front
			word[3] = UNIQUEENTRY; // misc flag words have ```` in front
			offset = 4;
			endsystem = true;
		}

		char* ptr = ReadCompiledWord(readBuffer, word + offset);
		if (stricmp(word + offset, (char*)"#define")) continue;

		//   accept lines line #define NAME 0x...
		ptr = ReadCompiledWord(ptr, word + offset); //   the #define name 
		if (ptr == 0 || *ptr == 0 || strchr(word + offset, '(')) continue; // if a paren is IMMEDIATELY attached to the name, its a macro, ignore it.
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
				ptr = (*ptr == 'x' || *ptr == 'X') ? ReadHex(ptr - 1, (uint64&)value) : ReadInt64(ptr - 1, value);
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
				ptr = ReadCompiledWord(ptr - 1, label);
				value = FindPropertyValueByName(label);
				bool olddict = xbuildDictionary;
				xbuildDictionary = false;
				if (!value)  value = FindSystemValueByName(label);
				if (!value)  value = FindMiscValueByName(label);
				if (!value)  value = FindParseValueByName(label);
				xbuildDictionary = olddict;
				if (!value)  
					ReportBug((char*)"missing modifier value for %s\r\n", label);
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
		if (!stricmp(word, "`AS_IS"))  continue; // dont need to store AS_IS
		WORDP D = StoreWord(word, AS_IS);
		D->properties = result; // all values stored here, using top (as_is) bit if needed
		AddInternalFlag(D, DEFINES);
		strcpy(word + offset, PrintU64(result));
		if (!endsystem) // cross ref from number to value only for properties and system flags and parsemarks for decoding bits back to words for marking
		{
			WORDP E = StoreWord(word, AS_IS);
			AddInternalFlag(E, DEFINES);
			if (!E->parseBits) E->parseBits = MakeMeaning(D); // if number value NOT already defined, use this definition
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
			MakeLowerCopy(name + 1, word); // all pos names start with `
			posMeanings[i] = ConceptFact(name, pos, facts);
		}

		word = FindSystemNameByValue(bit);
		if (word)
		{
			MakeLowerCopy(name + 1, word); // all sys names start with ``
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

	M = ConceptFact((char*)"~aux_verb_tenses", pos, facts);
	ConceptFact((char*)"~aux_verb_future", M, facts);
	ConceptFact((char*)"~aux_verb_past", M, facts);
	ConceptFact((char*)"~aux_verb_present", M, facts);

	M = ConceptFact((char*)"~conjunction", pos, facts);
	ConceptFact((char*)"~conjunction_subordinate", M, facts);
	ConceptFact((char*)"~conjunction_coordinate", M, facts);

	M = ConceptFact((char*)"~determiner_bits", pos, facts);
	ConceptFact((char*)"~determiner", M, facts);
	ConceptFact((char*)"~predeterminer", M, facts);

	M = ConceptFact((char*)"~possessive_bits", pos, facts);
	ConceptFact((char*)"~possessive", M, facts);
	ConceptFact((char*)"~pronoun_possessive", M, facts);

	M = ConceptFact((char*)"~noun_bits", pos, facts);
	ConceptFact((char*)"~verb_phrase", M, facts);
	ConceptFact((char*)"~noun_phrase", M, facts);
	ConceptFact((char*)"~prep_phrase", M, facts);
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

uint64 FindPropertyValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE * 4];
	word[0] = UNIQUEENTRY;
	MakeUpperCopy(word + 1, name);
	WORDP D = FindWord(word);
	return (!D || !(D->internalBits & DEFINES)) ? 0 : D->properties;
}

char* FindNameByValue(uint64 val) // works for invertable pos bits only
{
	char word[MAX_WORD_SIZE];
	word[0] = UNIQUEENTRY;
	strcpy(word + 1, PrintU64(val));
	WORDP D = FindWord(word);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	D = Meaning2Word(D->parseBits);
	return D->word + 1;
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
	return (alphanumeric == 3 && !IsDate(start) && !IsUrl(start, 0));
}

uint64 FindSystemValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE * 4];
	word[0] = UNIQUEENTRY;
	word[1] = UNIQUEENTRY;
	MakeUpperCopy(word + 2, name);
	WORDP D = FindWord(word,0, UPPERCASE_LOOKUP);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	return D->properties;
}

char* FindSystemNameByValue(uint64 val) // works for invertable system bits only
{
	char word[MAX_WORD_SIZE];
	word[0] = UNIQUEENTRY;
	word[1] = UNIQUEENTRY;
	strcpy(word + 2, PrintU64(val));
	WORDP D = FindWord(word,0, PRIMARY_CASE_ALLOWED);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	return Meaning2Word(D->parseBits)->word + 2;
}

char* FindParseNameByValue(uint64 val)
{
	char word[MAX_WORD_SIZE];
	word[0] = UNIQUEENTRY;
	word[1] = UNIQUEENTRY;
	word[2] = UNIQUEENTRY;
	strcpy(word + 3, PrintU64(val));
	WORDP D = FindWord(word,0, PRIMARY_CASE_ALLOWED);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	return Meaning2Word(D->parseBits)->word + 3;
}

uint64 FindParseValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE];
	word[0] = UNIQUEENTRY;
	word[1] = UNIQUEENTRY;
	word[2] = UNIQUEENTRY;
	MakeUpperCopy(word + 3, name);
	WORDP D = FindWord(word,0, UPPERCASE_LOOKUP);
	if (!D || !(D->internalBits & DEFINES)) return 0;
	return D->properties;
}

uint64 FindMiscValueByName(char* name)
{
	if (!*name || *name == '?') return 0; // ? is the default argument to call
	char word[MAX_WORD_SIZE];
	word[0] = UNIQUEENTRY;
	word[1] = UNIQUEENTRY;
	word[2] = UNIQUEENTRY;
	word[3] = UNIQUEENTRY;
	MakeUpperCopy(word + 4, name);
	WORDP D = FindWord(word,0, UPPERCASE_LOOKUP);
	if (!D || !(D->internalBits & DEFINES))
	{
		if (xbuildDictionary) ReportBug((char*)"Failed to find misc value %s", name);
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
	if (c == '+' || c == '-' || c == '*' || c == '/' || c == '&')
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

bool IsArithmeticOp(char* word)
{
	word = SkipWhitespace(word);
	char c = *word;
	if (c == '+' || c == '-' || c == '*' || c == '/' || c == '&') return true;
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

bool IsAssignOp(char* word)
{
	char c = *word;
	if (c == '=' && !word[1]) return true;
	if (word[1] != '='  || word[3]) return false;
	if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '&' || c == '|' || c == '^')  return true;
	return false;
}

char* IsUTF8(char* buffer, char* character) // swallow a single utf8 character (ptr past it) or return null 
{
	*character = 0;
	if (!buffer || !*buffer) return NULL;
	*character = *buffer;
	character[1] = 0;  // simple ascii character
	if (*buffer == 0) return buffer; // dont walk past end
	if (((unsigned char)*buffer) < 127) return buffer + 1; // not utf8

	char* start = buffer;
	unsigned int count = UTFCharSize(buffer) - 1; // bytes beyond first
	if (count == 0) count = 300; // not real?

	// does count of extenders match requested count
	unsigned int n = 0;
	while (*++buffer && *(unsigned char*)buffer >= 128 && *(unsigned char*)buffer <= 191) ++n;
	if (n == count)
	{
		strncpy(character, start, count + 1);
		character[count + 1] = 0;
		return buffer;
	}
	return start + 1;
}

char GetTemperatureLetter(char* ptr)
{
	if (!ptr || !ptr[1] || !IsDigit(*ptr)) return 0; // format must be like 1C or 19F
	size_t len = strlen(ptr) - 1;
	char lastc = GetUppercaseData(ptr[len]);
	if (lastc == 'F' || lastc == 'C' || lastc == 'K') // ends in a temp letter
	{
		// prove rest of prefix is a number
		char* p = ptr - 1;
		while (IsDigit(*++p)); // find end of number prefix
		if (len == (size_t)(p - ptr)) return lastc;
	}
	return 0;
}

char* IsTextCurrency(char* ptr, char* end)
{
	if (!end)
	{
		end = ptr;
		while (*++end && !(IsDigit(*end) || IsNonDigitNumberStarter(*end))); // locate nominal end of text
	}

	// look up direct word numbers
	WORDP D = FindWord(ptr, (end - ptr), LOWERCASE_LOOKUP);
	if (D && D->properties & CURRENCY && currencies[languageIndex]) for (unsigned int i = 0; i < 10000; ++i)
	{
		size_t len = end - ptr;
		int index = currencies[languageIndex][i].word;
		if (!index) break;
		char* w = Index2Heap(index);
		if (!strnicmp(ptr, w, len))
		{
			if (ptr[len] == 0 || IsDigit(ptr[len]) || IsNonDigitNumberStarter(ptr[len])) return Index2Heap(currencies[languageIndex][i].concept);  // a match 
		}
	}
	return NULL;
}

char* IsSymbolCurrency(char* ptr)
{
	if (!*ptr) return NULL;
	char* end = ptr;
	unsigned char* p = (unsigned char*)ptr;
	while (*++end && !IsDigit(*end) && !IsSign(*end)); // locate nominal end of text

	if (*p == 0xe2 && p[1] == 0x82 && p[2] == 0xac) return end;// euro is prefix
	else if (*p == 0xC2 && p[1] == 0xA2) return end; // cent sign
	else if (*p == 0xc2) // yen is prefix
	{
		char c = p[1];
		if (c == 0xa2 || c == 0xa3 || c == 0xa4 || c == 0xa5) return end;
	}
	else if (*p == 0xc3 && p[1] == 0xb1) return end; // british pound
	else if (*p == 0xc3 && p[1] == 0xa3) return end; // british pound extended ascii
	else if (*p == 0xe2 && p[1] == 0x82 && p[2] == 0xb9) return end; // rupee
	else if (IsTextCurrency((char*)p, end) != NULL) return end;
	return NULL;
}

unsigned char* GetCurrency(unsigned char* ptr, char*& number) // does this point to a currency token, return currency and point to number (NOT PROVEN its a number)
{
	unsigned char* at = ptr;
	if (IsSign(*at)) ++at; //   skip sign indicator, -$1,234.56

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

bool IsLegalName(const char* name, bool label) // start alpha (or ~) and be alpha _ digit (concepts and topics can use . or - also)
{
	char start = *name;
	if (*name == '~' || *name == SYSVAR_PREFIX) ++name;
	if (label && IsDigit(*name)) {}
	else if (!IsAlphaUTF8(*name)) return false;
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
	char d = number[len - 1];
	bool num = false;
	if (d == 'k' || d == 'K' || d == 'm' || d == 'M' || d == 'B' || d == 'b' || d == 'G' || d == 'g' || d == '$')
	{
		number[len - 1] = 0;
		num = IsDigitWord(number, useNumberStyle);
		number[len - 1] = d;
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

char* FixHtmlTags(char* html)
{
	char* start = html--;
	while ((html = strchr(++html, '&')))
	{
		if (html[1] == '#')
		{
			int n = atoi(html + 2);
			// dont convert _ (95) because cs does _ to space mapping on output - html is an escape for that
			if (IsDigit(html[2]) && IsDigit(html[3]) && html[4] == ';' && n != 95) // &#32;
			{
				*html = (char)n; // normal ascii value
				memmove(html + 1, html + 5, strlen(html + 4));
				continue;
			}
		}

		if (!strnicmp(html, (char*)"&lt;", 4))
		{
			memmove(html + 1, html + 4, strlen(html + 3)); // all moves include closing nul
			*html = '<';
		}
		else if (!strnicmp(html, (char*)"&gt;", 4))
		{
			memmove(html + 1, html + 4, strlen(html + 3));
			*html = '>';
		}
		else if (!strnicmp(html, (char*)"&lsquot;", 8))
		{
			memmove(html + 1, html + 8, strlen(html + 7));
			*html = '"';
		}
		else if (!strnicmp(html, (char*)"&rsquot;", 8))
		{
			memmove(html + 1, html + 8, strlen(html + 7));
			*html = '"';
		}
		else if (!strnicmp(html, (char*)"&quot;", 6))
		{
			memmove(html + 1, html + 6, strlen(html + 5));
			*html = '"';
		}
		else if (!strnicmp(html, (char*)"&ldquo;", 7))
		{
			memmove(html + 1, html + 7, strlen(html + 6));
			*html = '"';
		}
		else if (!strnicmp(html, (char*)"&rdquo;", 7))
		{
			memmove(html + 1, html + 7, strlen(html + 6));
			*html = '"';
		}
		else if (!strnicmp(html, (char*)"&laquo;", 7))
		{
			memmove(html + 1, html + 7, strlen(html + 6));
			*html = '"';
		}
		else if (!strnicmp(html, (char*)"&raquo;", 7))
		{
			memmove(html + 1, html + 7, strlen(html + 6));
			*html = '"';
		}
		else if (!strnicmp(html, (char*)"&amp;ndash;", 11)) // buggy from some websites
		{
			memmove(html + 3, html + 11, strlen(html + 10));
			*html = ' ';
			html[1] = '-';
			html[2] = ' ';
		}
		else if (!strnicmp(html, (char*)"&ndash;", 7))
		{
			memmove(html + 1, html + 7, strlen(html + 6));
			*html = '-';
		}
		else if (!strnicmp(html, (char*)"&mdash;", 7))
		{
			memmove(html + 1, html + 7, strlen(html + 6));
			*html = '-';
		}
		else if (!strnicmp(html, (char*)"&amp;", 5))
		{
			memmove(html + 1, html + 5, strlen(html + 4));
			*html = '&';
		}
		else if (!strnicmp(html, (char*)"&nbsp;", 6))
		{
			memmove(html + 1, html + 6, strlen(html + 5));
			*html = ' ';
		}
		else if (!strnicmp(html, (char*)"&#8217;", 7))
		{
			memmove(html + 1, html + 7, strlen(html + 6));
			*html = '\'';
		}
		else if (html[1] == '#' && html[2] == 'x' && 
			(html[6] == ';' || html[7] == ';')) // hex html
		{
			char* end = strchr(html, ';');
			unsigned int x = 0;
			char* ptr = html + 3;
			while (IsDigit(*ptr))
			{
				x = (x << 4);
				char c = toUppercaseData[*ptr++];
				if (c >= 'A') c = 10 + c - 'A';
				else c -= '0';
				x += c;
			}

			char data[100];
			sprintf(data, "u%u", x);
			char* answer = UTF16_2_UTF8(data, false);
			size_t n = strlen(answer);
			strncpy(html, answer, n);
			memmove(html + n, end + 1, strlen(end));
		}
		else if (html[1] == '#' && IsDigit(html[2]) &&
			(html[5] == ';' || html[6] == ';')) // decimal html
		{
			char* end = strchr(html, ';');
			unsigned int x = 0;
			char* ptr = html + 2;
			while (IsDigit(*ptr))
			{
				x = (x * 10) + (*ptr++ - '0');
			}
			char data[100];
			sprintf(data, "u%u", x);
			char* answer = UTF16_2_UTF8(data, false);
			size_t n = strlen(answer);
			strncpy(html, answer, n);
			memmove(html + n, end + 1, strlen(end));
		}
	}
	return start;
}

bool IsDigitWord(char* ptr, int useNumberStyle, bool comma, bool checkAll) // digitized number
{
	if (!*ptr) return false;
	char* end = ptr + strlen(ptr);
	char decimalMark = decimalMarkData[useNumberStyle];
	char digitGroup = digitGroupingData[useNumberStyle];
	if (IsFloat(ptr, end, useNumberStyle)) return true; // sentence end if . at end, not a float
	//   signing, # marker or currency markers are still numbers
	char* number = 0;
	char* currency = 0;
	if ((currency = (char*)GetCurrency((unsigned char*)ptr, number))) ptr = number; // if currency, find number start of it
	if (IsNonDigitNumberStarter(*ptr)) ++ptr; //   skip numeric nondigit header (+ - # )
	if (!*ptr) return false;

	bool foundDigit = false;
	while (*ptr)
	{
		if (IsDigit(*ptr)) foundDigit = true; // we found SOME part of a number
		else if (*ptr == decimalMark) return false; // float will find that
		else if (*ptr == '%' && !ptr[1]) break; // percentage
		else if (*ptr == ':');	//   TIME delimiter
		else if (*ptr == digitGroup && comma && IsCommaNumberSegment(ptr + 1, currency)); // allow comma, but rest of word needs to be valid too
		else if (ptr == currency) break; // dont need to see currency end
		else return false;		//   1800s is done by substitute, so fail this
		++ptr;
	}
	if (checkAll && ptr != end) return false;
	return foundDigit;
}

bool IsCommaNumberSegment(char* ptr, char* end) // number after a comma
{
	// next three characters (perhaps only 2 if Indian) should be numbers
	if (!*ptr || !IsDigit(*ptr) || !IsDigit(ptr[1])) return false; // must have 2 digits

	int i = (numberStyle == INDIAN_NUMBERS) ? 2 : 3;
	char* comma = ptr + i;

	if (!IsDigit(*(comma - 1))) return false; // must be a number in the preceeding position

	if (end && comma == end) return true; // end of number, start of currency symbol
	if (i == 3 && !*comma) return true; // end of number
	if (i == 2 && IsDigit(*comma) && !comma[1]) return true; // end of number, final 3 digit segment

	if (*comma && *comma == numberComma && IsCommaNumberSegment(comma + 1, end)) return true; // valid number segments all the way
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

uint64 ComposeNumber(unsigned int i, unsigned int& end)
{
	if (1==1) return 0;  // not ready
	if (i == wordCount) return 0; // no room to merge

	end = 0;
	char* number;
	char* word = wordStarts[i];
	bool isNumber = IsNumber(wordStarts[i], numberStyle) != NOT_A_NUMBER && !IsPlaceNumber(word, numberStyle) && !GetCurrency((unsigned char*)word, number) && !strchr(word, '.');
	if (!isNumber) return 0; // not a number or is float  
	char* word1 = wordStarts[++i];
	bool anded = false;
	if (!stricmp(word1, "and"))
	{
		if (i == wordCount) return 0;
		word1 = wordStarts[++i];
		bool isNumber1 = IsNumber(word1, numberStyle) != NOT_A_NUMBER && !IsPlaceNumber(word1, numberStyle) && !GetCurrency((unsigned char*)word1, number) && !strchr(word1, '.');
		if (!isNumber1) return 0; // not a joiner
		end = i;
		anded = true;
	}
	else end = i;

		//  one and twenty
		// five hundred
		// twenty thousand
	uint64 value = 0;
	int64 power1 = NumberPower(word, numberStyle);
	int64 power2 = NumberPower(word1, numberStyle);
	int64 val1 = Convert2Integer(word, numberStyle);
	int64 val2 = Convert2Integer(word1, numberStyle);
	if (power1 < power2) // one thousand, one and twenty, twenty thousand
	{
		if (anded) value += value + val1;
		else value += value * val1;
		power1 = power2; // dominant
	}
	else if (power1 == power2) return 0; // same granularity, don't merge, like "what is two and two"
	else // thirty 1
	{
		if (power2 == 1)
		{
			value += val1 + val2;
		}
		else return 0;
	}
	
	// we now have a first pair number. Time to absorb another number
	
	char* word3 = wordStarts[++i];
	if (i > wordCount) return value;

	anded = false;
	if (!stricmp(word3, "and"))
	{
		if (i == wordCount) return value;
		char* word4 = wordStarts[++i];
		bool isNumber3 = IsNumber(word3, numberStyle) != NOT_A_NUMBER && !IsPlaceNumber(word1, numberStyle) && !GetCurrency((unsigned char*)word1, number) && !strchr(word1, '.');
		if (!isNumber3) return value; // not a joiner
		anded = true;
	}

	char* word4 = wordStarts[++i];
	int64 power3 = NumberPower(word3, numberStyle);
	int64 val3 = Convert2Integer(word3, numberStyle);
	bool isNumber4 = word4 && IsNumber(word4, numberStyle) != NOT_A_NUMBER && !IsPlaceNumber(word4, numberStyle) && !GetCurrency((unsigned char*)word4, number) && !strchr(word4, '.');
	if (isNumber4) // swallow next pair
	{
		int64 val4 = Convert2Integer(word4, numberStyle);
		int64 power4 = NumberPower(word4, numberStyle);
		if (power4 > power3) // one thousand two hundred
		{
			uint64 newval = val3 * val4;
			value += newval;
			end = i;
			return value;
		}
		else  // one thousand fifty
		{
			value += val3;
		}
	}

	return value;
}

unsigned int IsNumber(char* num, int useNumberStyle, bool placeAllowed) // simple digit number or word number or place/ordinal number or currency number
{
	if (!*num) return NOT_A_NUMBER;
	if (*num == 'I' && !num[1]) return NOT_A_NUMBER; // never decode I to a number
	char word[MAX_WORD_SIZE];
	MakeLowerCopy(word, num); // accept number words in upper case as well
	if (word[1] && (word[1] == ':' || word[2] == ':')) return NOT_A_NUMBER;	// 05:00 // time not allowed
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
		int64 val = (!*number) ? NOT_A_NUMBER : Convert2Integer(number, useNumberStyle);
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
		if (D && W && D->properties & NUMBER_BITS && W->properties & NUMBER_BITS && IsFractionNumber(D->word)) return FRACTION_NUMBER;
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
		ptr = word;
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
	if (D && D->properties & (NOUN_NUMBER | ADJECTIVE_NUMBER) && numberValues[languageIndex]) for (unsigned int i = 0; i < 1000; ++i)
	{
		int index = numberValues[languageIndex][i].word;
		if (!index) break;
		char* numx = Index2Heap(index);
		if (len == numberValues[languageIndex][i].length && !strnicmp(word, numx, len))
		{
			return numberValues[languageIndex][i].realNumber;  // a match 
		}
	}

	if (D && D->properties & NUMBER_BITS)
		return (D->systemFlags & ORDINAL) ? PLACETYPE_NUMBER : WORD_NUMBER;   // known number
	if (!*word) return NOT_A_NUMBER;

	return (Convert2Integer(word, useNumberStyle) != NOT_A_NUMBER) ? WORD_NUMBER : NOT_A_NUMBER;		//   try to read the number
}

bool IsPlaceNumber(char* word, int useNumberStyle) // place number and fraction numbers
{
	size_t len = strlen(word);
	if (len < 3) return false; // min is 1st

	// word place numbers
	char tok[MAX_WORD_SIZE];
	strcpy(tok, word);
	size_t size = 0;
	if (stricmp(current_language, "english")) { ; }
	else if (len > 4 && !strcmp(word + len - 5, (char*)"first")) size = 5;
	else if (len > 5 && !strcmp(word + len - 6, (char*)"second")) size = 6;
	else if (len > 4 && !strcmp(word + len - 5, (char*)"third")) size = 5;
	else if (len > 4 && !strcmp(word + len - 5, (char*)"fifth")) size = 5;
	if (size)
	{
		if (len == size) return true;
		size_t l = strlen(tok);
		tok[l - size] = 0;
		if (tok[l - size - 1] == '-') tok[l - size - 1] = 0; // fifty-second
		if (IsNumber(tok, useNumberStyle) != NOT_A_NUMBER) return true;
		else return false;
	}

	// does it have proper endings?
	if (stricmp(current_language, "english")) { ; }
	else if (word[len - 2] == 's' && word[len - 1] == 't') { ; }  // 1st
	else if (word[len - 2] == 'n' && word[len - 1] == 'd') // 2nd
	{
		if (!stricmp(word, (char*)"thousand")) return false;	// 2nd but not thousand
	}
	else if (word[len - 1] == 'd') // 3rd
	{
		if (word[len - 2] != 'r') return false;	// 3rd is ok
	}
	else if (word[len - 2] == 't' && word[len - 1] == 'h') { ; } // 4th
	else if (word[len - 3] == 't' && word[len - 2] == 'h' && word[len - 1] == 's') { ; } // 4ths
	else return false;	// no place suffix
	if (strchr(word, '_')) return false;	 // cannot be place number

	// separator?
	if (word[len - 3] == '\'' || word[len - 3] == '-') { ; }// 24'th or 24-th
	else if (strchr(word, '-')) return false;	 // cannot be place number

	if (len < 4 && !IsDigit(*word)) return false; // want a number
	char num[MAX_WORD_SIZE];
	strcpy(num, word);
	return (IsNumber(num, useNumberStyle, false) == PLACETYPE_NUMBER) ? true : false; // show it is correctly a number - pass false to avoid recursion from IsNumber
}

char* GetActual(char* msg)
{
	char* actual = strstr(msg, "output\":"); // skip open quote
	if (actual)
	{
		actual += 10;
		char* endx = strstr(actual, "\",");
		if (!endx) endx = strstr(actual, "\"}");
		if (endx) *endx = 0;
		size_t len = strlen(actual);
		while (actual[len - 1] == ' ') actual[--len] = 0;
		char* at1 = actual - 1;
		while (*++at1)
		{
			if (*at1 == '\\') memmove(at1, at1 + 1, strlen(at1));
		}
	}
	else actual = (char*)"";
	return actual;
}

static char* FixNumber(char* from, char* to, int countrycode, int areacode,int citycode, int localcode)
{
	if (*from == '+') ++from; // skip marker
	char* start = to;
	if (countrycode)
	{
		*to++ = '+';
		memcpy(to, from, countrycode);
		to += countrycode;
		from += countrycode;
		*to++ = '-';
	}
	if (areacode)
	{
		memcpy(to, from, areacode);
		to += areacode;
		from += areacode;
		*to++ = '-';
	}
	if (citycode)
	{
		memcpy(to, from, citycode);
		to += citycode;
		from += citycode;
		*to++ = '-';
	}
	memcpy(to, from, localcode);
	to[localcode] = 0;
	return start;
}

void PhoneNumber(unsigned int start)
{
/* 
 7 or more digits in a phone number, separated with spaces or hyphens and area code in parens
 German Numbers are often written in blocks of two. Example: +49 (A AA) B BB BB

The standard format for writing international phone numbers is to use nothing except digits, spaces, and the plus sign at the beginning to signify that the following digits are the country code. 
For example, +1 999 555 0123 or +44 22 2345 1234 
(The use of other punctuation, including periods, dashes, slashes, and parentheses, is discouraged because those punctuation marks may have different meanings in different local formats.)
(555) 123-4567: This is a common format for phone numbers in the United States. The number is divided into three parts: the area code in parentheses, followed by a three-digit prefix, and then a four-digit line number.
555-123-4567: This format is similar to the previous one, but without the parentheses around the area code. It's also widely used and recognized in the United States.
1-555-123-4567: This format includes the country code for the United States, which is "1". The number is then divided into three parts, with a hyphen separating each part.

01234 567890: This format is commonly used in the United Kingdom. The number is divided into two parts: the area code (including the leading zero) and the local number. 
The area code and local number are separated by a space.
+44 1234 567890: This format includes the country code for the United Kingdom, which is "+44". 
The number is then divided into two parts: the area code (including the leading zero) and the local number. 
The country code, area code, and local number are separated by spaces.
44 (0) 1234 567890: This format is similar to the previous one but includes the "0" in parentheses after the country code. 
The number is then divided into two parts: the area code (including the leading zero) and the local number. 
The country code, area code, and local number are separated by spaces.

030 12345678: This format is commonly used in Germany. The number is divided into two parts: the area code and the local number. The area code and local number are separated by a space.
+49 30 12345678: This format includes the country code for Germany, which is "+49". The number is then divided into two parts: the area code and the local number. The country code, area code, and local number are separated by spaces.
49 (0) 30 12345678: This format is similar to the previous one but includes the "0" in parentheses after the country code. The number is then divided into two parts: the area code and the local number. The country code, area code, and local number are separated by spaces.

912 345 678: This format is commonly used in Spain. 
The number is divided into three parts: the first digit represents the province or region code, followed by a three-digit prefix, and then a three-digit line number. The province or region code is often omitted when dialing the number locally.
+34 912 345 678: This format includes the country code for Spain, which is "+34". 
The number is then divided into three parts: the province or region code, the three-digit prefix, and the three-digit line number. The country code, province or region code, prefix, and line number are separated by spaces.
34 912 345 678: This format is similar to the previous one but does not include the "+" symbol before the country code. 
The number is divided into three parts: the province or region code, the three-digit prefix, and the three-digit line number. 
The country code, province or region code, prefix, and line number are separated by spaces.
*/
	// are there more numbers AFTER this chunk. we start from the rear of the phone number
	if (*wordStarts[start] == '-') return; // invalid start
	if (start != wordCount) // invalid trailer
	{
		char* afterword = wordStarts[start + 1];
		if (IsDigit(*afterword)) return; // presume we are not at end of numbers
		if (*afterword == '-' && !afterword[1]) return; // hyphen between
	}

	unsigned int count = 0;
	int i;
	char* word;
	int countrycode = 0;
	char number[50];
	number[49] = 0;
	unsigned int index = 49;
	for (i = start; i > 0; --i) // scan contiguous number words backwards
	{
		if (countrycode) break; // no more
		word = wordStarts[i];
		size_t len = strlen(word);
		unsigned int startindex = index;
		for (int j = len-1; j >= 0; --j)
		{
			if (word[j] == '+' && j == 0) // presume country code marker
			{
				char* hyphen = strchr(word, '-');
				if (hyphen) countrycode = hyphen - word - 1;
				else countrycode = 1;
			}
			else if (IsDigit(word[j]))
			{
				number[--index] = word[j];
				if (index == 25) return; // seeing way too many digits to be phone number
			}
			else if (word[j] != '-' && word[j] != '(' && word[j] != ')') // invalid word 
			{
				index = startindex; // discard word impact and end scan
				break;
			}
		}
		if (index != startindex)  // we added some digits
		{
			if ((49 - index) > 15) return; // max international w countrycode is 15 digits
		} 
		// word was illegal, its numbers meaningless
		else if (word[1]  || (*word != '-' && *word != '(' && *word != ')')) break;
	}
	int size = 49 - index;
	if (size < 6) return; // not enough digits for a local number (us local number is 7 digits, vatican is 6)
	
	char renumber[20];
	// we always end on non-legal word index
	FixNumber(number + index, renumber, countrycode, 0, size - 4 - countrycode, 4);
	WORDP D = StoreWord(renumber, AS_IS);
	MarkWordHit(0, MakeMeaning(D), StoreWord("~phonenumber", AS_IS), 0,i + 1, start);

	if (size == 7 && numberStyle == AMERICAN_NUMBERS)
	{
		FixNumber(number + index, renumber, 0, 0,3, 4);
		WORDP D = StoreWord(renumber, AS_IS);
		MarkWordHit(0, MakeMeaning(D), StoreWord("~uslocal_phonenumber", AS_IS), 0, i + 1, start);
	}
	else if (size == 10 && numberStyle == AMERICAN_NUMBERS)
	{
		FixNumber(number + index, renumber,0,3, 3, 4);
		WORDP D = StoreWord(renumber, AS_IS);
		MarkWordHit(0, MakeMeaning(D), StoreWord("~usinternal_phonenumber", AS_IS), 0, i + 1, start);
	}
	else if (countrycode)
	{
		int dindex = -1;
		while (phoneData[++dindex].countrycode)
		{
			if (!strncmp(number + index, phoneData[dindex].countrydigits, strlen(phoneData[dindex].countrydigits)))
			{
				int exchangecode = size - phoneData[dindex].subscriber - phoneData[dindex].areacode - phoneData[dindex].countrycode;
				FixNumber(number + index, renumber, phoneData[dindex].countrycode, phoneData[dindex].areacode, exchangecode, phoneData[dindex].subscriber);
				WORDP D = StoreWord(renumber, AS_IS);
				MarkWordHit(0, MakeMeaning(D), StoreWord(phoneData[dindex].concept, AS_IS), 0, i + 1, start);
				break;
			}
		}
	}
	else if (number[index] == '0' && size == 11) // local uk
	{
		FixNumber(number + index, renumber, 0, 1, 4, 6);
		WORDP D = StoreWord(renumber, AS_IS);
		MarkWordHit(0, MakeMeaning(D), StoreWord("~ukinternal_phonenumber", AS_IS), 0, i + 1, start);
	}
}

char* ReadTabField(char* buffer, char* storage)
{
	if (!buffer) return NULL;

	char* end = strchr(buffer, '\t');
	if (!end) return NULL;
	*end = 0;
	strcpy(storage, SkipWhitespace(buffer));
	int len = strlen(storage);
	while (storage[len - 1] == ' ') storage[--len] = 0;

	char* at = storage - 1;
	while (++at = strchr(at, '"'))
	{
		if (at[1] == '"') memmove(at + 1, at + 2, strlen(at + 1)); // remove doubled quotes
	}
	return end + 1;
}

char* HexDisplay(char* str)
{
	char* buffer = AllocateBuffer();
	char* start = buffer;
	--str;
	while (*++str)
	{
		if (*str < 0x7f) sprintf(buffer, "%c", *str);
		else sprintf(buffer, "%x", *str);
		buffer += strlen(buffer);
	}
	FreeBuffer();
	return start; 
}

FunctionResult AnalyzeCode(char* buffer)
{
	char* word = ARGUMENT(1);
	bool more = false;
	char junk[100];
	char* check = ReadCompiledWord(word, junk);
	if (!stricmp(junk, "more"))
	{
		more = true;
		word = check;
	}
	HEAPREF oldinput = startSupplementalInput;
	ClearSupplementalInput();
	AddInput(word, 2); // analyze
	SAVEOLDCONTEXT()
		FunctionResult result;
	uint64 flags = OUTPUT_NOCOMMANUMBER + OUTPUT_KEEPQUERYSET;

	if (!directAnalyze) Output(word, buffer, result, (unsigned int)flags); // need to eval the input variable to get real data
	else strcpy(buffer, word);
	// really need to purify the input 
	size_t xsize = strlen(buffer);
	PurifyInput(buffer, buffer, xsize, INPUT_PURIFY); // change out special or illegal characters

	if (*buffer == '"') // if a string, remove quotes
	{
		size_t len = strlen(buffer);
		if (buffer[len - 1] == '"')
		{
			buffer[len - 1] = 0;
			*buffer = ' ';
		}
	}
	if (trace & (TRACE_OUTPUT | TRACE_PREPARE)) Log(USERLOG, "Analyze: %s \r\n", buffer);
	PrepareSentence(buffer, true, false, true, false);
	char* hold = GetNextInput();
	if (hold)
	{
		char* x = AllocateBuffer();
		strcpy(x, hold);
		strcpy(buffer, x); // set up for possible continuation (set by preparesentence)
		FreeBuffer();
	}
	else *buffer = 0;

	if (trace & TRACE_PATTERN || tracepatterndata) // either locally or globally controlled
	{
		char* normal = AllocateBuffer();
		char* canonical = AllocateBuffer();
		for (unsigned int i = 1; i <= wordCount; ++i)
		{
			strcat(normal, wordStarts[i]);
			strcat(normal, " ");
			strcat(canonical, wordCanonical[i]);
			strcat(canonical, " ");
			if (!stricmp(current_language, "japanese") || !stricmp(current_language, "chinese"))
			{
				strcat(normal, " ");
				strcat(canonical, " ");
			}
		}
		Log(USERLOG, "normal Analyze: %s\r\n", normal);
		Log(USERLOG, "canonical Analyze: %s\r\n", canonical);
		FreeBuffer();
		FreeBuffer();
		DumpTokenFlags((char*)"After parse");

	}

	RESTOREOLDCONTEXT()
		startSupplementalInput = oldinput; // recover prior status
	MoreToCome();
	return NOPROBLEM_BIT;
}

bool IsFractionNumber(char* word) // fraction numbers
{
	size_t len = strlen(word);
	if (numberValues[languageIndex]) for (unsigned int i = 0; i < 1000; ++i)
	{
		int index = numberValues[languageIndex][i].word;
		if (!index) break;
		if (numberValues[languageIndex][i].realNumber == FRACTION_NUMBER)
		{
			char* w = Index2Heap(index);
			if (len == numberValues[languageIndex][i].length && !strnicmp(word, w, len))
			{
				return true;
			}
		}
	}
	return false;
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
	char* wordstart = word;
	char sign = 0;
	if (IsSign(*word))
	{
		wordstart++;
		sign = *word;
	}
	*at = 0;
	int counter = 0;
	int limit = 3;
	while (end != wordstart)
	{
		*--at = *--end;
		if (IsDigit(*at) && ++counter == limit && end != wordstart)
		{
			*--at = (useNumberStyle == FRENCH_NUMBERS) ? '.' : ',';
			counter = 0;
			limit = (useNumberStyle == INDIAN_NUMBERS) ? 2 : 3;
		}
	}
	if (sign) *--at = sign;
	strcpy(buffer, at);
}

char* GetNextInput()
{
	if (!startSupplementalInput) return NULL;
	uint64* ref = (uint64*)startSupplementalInput;
	inputNest = (ref[2]) ? 1 : 0;
	return currentInput = (char*)ref[1]; // this input system is current
}

void SetContinuationInput(char* buffer) // set upon tokenization of prior piece
{
	uint64* ref = (uint64*)startSupplementalInput;
	if (!ref)
	{
		char* item = AllocateHeap(buffer);
		startSupplementalInput = AllocateHeapval(HV1_STRING,startSupplementalInput, (uint64)item);  // 1 == internal input
	}
	else
	{
		buffer = SkipWhitespace(buffer);
		if (*buffer) 	ref[1] = (uint64)buffer;
		else  startSupplementalInput = (HEAPREF)ref[0];// this input chunk is finished
	}
	MoreToCome();
}

void MoreToCome()
{
	// set %more and %morequestion
	moreToCome = moreToComeQuestion = false;
	if (sentenceOverflow) return; // internal limit will stop him
	char nextWord[MAX_WORD_SIZE];
	HEAPREF links = startSupplementalInput;
	while (links)
	{
		uint64* ref = (uint64*)links;
		links = (HEAPREF)ref[0];
		ReadCompiledWord((char*)ref[1], nextWord);
		WORDP next = FindWord(nextWord);
        if (next || strlen(SkipWhitespace(nextWord)) > 0) moreToCome = true;
		if (next && next->properties & QWORD) moreToComeQuestion = true; // assume it will be a question (misses later ones in same input)
		else moreToComeQuestion = (strchr((char*)ref[1], '?') != 0);
		if (moreToComeQuestion) break; // know everything of importance now
	}
}

void EraseCurrentInput()
{
	if (startSupplementalInput) startSupplementalInput = (HEAPREF)((uint64*)startSupplementalInput)[0];
}

bool AddInput(char* buffer, int kind, bool clear)
{ // add a potentially multi sentence input in front of current
	if (clear) ClearSupplementalInput();
	buffer = SkipWhitespace(buffer);
	if (!*buffer) buffer = (char*)" ";
	char* item = AllocateHeap(buffer);
	char* comment = strstr(item, "#!");
	if (comment) *comment = 0;
	startSupplementalInput = AllocateHeapval(HV1_STRING|HV2_STRING,startSupplementalInput, (uint64)item, kind);  // 1 == internal input
	MoreToCome();
	return true;
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

bool IsPureNumber(char* word)
{
	unsigned int periods = 0;
	unsigned int commas = 0;
	if (*word != '+' && *word != '-') --word;
	while (*++word)
	{
		if (*word == '.') ++periods;
		else if (*word == ',') ++commas;
		else if (*word >= '0' && *word <= '9') {}
		else return false;
	}
	// depending on language, this vary
	return true;
}

char IsFloat(char* word, char* end, int useNumberStyle)
{
	if (*(end - 1) == '.') return false;	 // float does not end with ., that is sentence end
	if (IsSign(*word)) ++word; // ignore sign
	if (!IsDigit(*word) && *word != '.') return false;
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
				if (IsSign(word[1])) ++word; // ignore exponent sign
				if (!IsDigit(word[1])) return false; // need a number after the exponent, 10euros is not a float
			}
			else return false; // non digit is fatal
		}
	}
	if (period == 1) return 1;
	if (exponent) return 'e';
	return 0;
}

bool IsNumericDate(char* word, char* end) // 01.02.2009 or 1.02.2009 or 1.2.2009
{ // 3 pieces separated by periods or slashes. with 1 or 2 digits in two pieces and 2 or 4 pieces in last piece

	char separator = 0;
	int counter = 0;
	int piece = 0;
	int size[100];
	memset(size, 0, sizeof(size));
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

bool IsMail(char* word)
{
	if (*word == '@') return false;
	if (strchr(word, ' ') || !strchr(word, '.')) return false; // cannot have space, must have dot dot
	char* at = strchr(word, '@');	// check for email
	bool valid = false;
	if (at)
	{
		// check local part, before @, for possible previous word end
		// see reasonable write up at https://www.jochentopf.com/email/chars.html
		// also skip over any mailto: protocol prefix
		char* ptr = word - 1;
		if (!strnicmp((ptr + 1), (char*)"mailto:", 7)) ptr += 7;
		while (++ptr < at)
		{
			if (IsInvalidEmailCharacter(*ptr)) return false;
		}

		// find end of email
		char* emailEnd = at;
		while (*++emailEnd && !IsInvalidEmailCharacter(*emailEnd)); // fred,andy@kore.com

		// cannot have a second @ before the end
		char* at2 = strchr(at + 1, '@');
		if (at2 && at2 < emailEnd) return false;

		char* dot = strchr(at + 2, '.'); // must have character or digit after @ and before . (RFC1123 section 2.1)
		if (!dot || !IsAlphaUTF8OrDigit(dot[1])) return false;
		while (*(emailEnd - 1) == '.') --emailEnd;
		char endChar = *emailEnd;
		*emailEnd = 0;
		valid = IsTLD(strrchr(at + 2, '.'));
		*emailEnd = endChar;
	}
	return valid;
}

bool IsTLD(char* word)
{
	char* ptr = word;
	if (!ptr || strlen(ptr) < 2) return false;
	if (*ptr == '.') ++ptr;

	// check for a known top level domain
	int* tld = topleveldomains;
	size_t n = strlen(ptr);
	for (unsigned int i = 0; i < 10000; ++i)
	{
		if (!tld || !*tld) break;
		char* w = Index2Heap(*tld);
		tld++;
		if (!stricmp(w, ptr) && w[n] == ptr[n]) return true;
	}

	// just in case there is no top level domains file
	// common two character TLD
	if (n == 2)
		return (!stricmp(ptr, (char*)"ai") || !stricmp(ptr, (char*)"au") || !stricmp(ptr, (char*)"be") ||
			!stricmp(ptr, (char*)"ca") || !stricmp(ptr, (char*)"ch") || !stricmp(ptr, (char*)"cn") ||
			!stricmp(ptr, (char*)"co") || !stricmp(ptr, (char*)"de") || !stricmp(ptr, (char*)"es") ||
			!stricmp(ptr, (char*)"eu") || !stricmp(ptr, (char*)"fr") || !stricmp(ptr, (char*)"gl") ||
			!stricmp(ptr, (char*)"in") || !stricmp(ptr, (char*)"io") || !stricmp(ptr, (char*)"it") ||
			!stricmp(ptr, (char*)"jp") || !stricmp(ptr, (char*)"ly") || !stricmp(ptr, (char*)"me") ||
			!stricmp(ptr, (char*)"nl") || !stricmp(ptr, (char*)"pl") || !stricmp(ptr, (char*)"ru") ||
			!stricmp(ptr, (char*)"tv") || !stricmp(ptr, (char*)"uk") || !stricmp(ptr, (char*)"us"));

	// common 3 and 4 character domains
	// check for common suffices - https://w3techs.com/technologies/overview/top_level_domain/all
	else if (n == 3)
		return (!stricmp(ptr, (char*)"com") || !stricmp(ptr, (char*)"net") || !stricmp(ptr, (char*)"org") ||
			!stricmp(ptr, (char*)"edu") || !stricmp(ptr, (char*)"biz") || !stricmp(ptr, (char*)"gov") ||
			!stricmp(ptr, (char*)"mil"));

	else if (n == 4)
		return (!stricmp(ptr, (char*)"info"));

	return false;
}

bool IsUrl(char* word, char* end)
{
	if (*word == '@') return false;
	if (strchr(word, ' ') || !strchr(word, '.')) return false; // cannot have space, must have dot dot
	if (!strnicmp((char*)"www.", word, 4))
	{
		char* colon = strchr(word, ':');
		if (colon && (colon[1] != '/' || colon[2] != '/')) return false;
		return true; // classic web url without protocol
	}
	if (!strnicmp((char*)"http", word, 4) || !strnicmp((char*)"ftp:", word, 4))
	{
		char* colon = strchr(word, ':');
		if (colon && colon[1] == '/' && colon[2] == '/' && IsAlphaUTF8OrDigit(colon[3])) return true; // classic urls, with something beyond just the protocol
		if (colon) return false; // only protocol is not enough
	}
	size_t len = strlen(word);
	if (len > 200) return false;
	char tmp[MAX_WORD_SIZE];
	MakeLowerCopy(tmp, word);
	if (!end) end = word + len;
	tmp[end - word] = 0;

	if (IsMail(tmp)) return true;

	//	check domain suffix is somewhat known as a TLD
	//	fireze.it OR www.amazon.co.uk OR amazon.com OR kore.ai
	char* firstPeriod = strchr(tmp, '.');
	if (!firstPeriod || firstPeriod[1] == '.') return false; // not a possible url
	char* ptr = tmp - 1;
	while (++ptr < firstPeriod)
	{
		if (IsInvalidEmailCharacter(*ptr)) return false; // looks like a word before the url
	}
	char* domainEnd = strchr(firstPeriod, '/');
	if (domainEnd) *domainEnd = 0;
	ptr = strrchr(tmp, '.'); // last period - KNOWN to exist
	if (!ptr) return false;	// stops compiler warning

	return IsTLD(ptr);
}

bool IsFileExtension(char* word)
{
	// needs to be lowercase alphanumeric
	char* ptr = word - 1;
	while (*++ptr)
	{
		if (IsLowerCase(*ptr) || IsDigit(*ptr) || *ptr == '.') continue;
		return false;
	}

	size_t len = strlen(word);
	// most extensions are 3 characters
	if (len == 3) return true;

	// other lengths are more exclusive : https://fileinfo.com/filetypes/common
	if (len == 1) {
		return (!strnicmp(word, (char*)"a", 1) || !strnicmp(word, (char*)"c", 1) || !strnicmp(word, (char*)"h", 1) || !strnicmp(word, (char*)"m", 1) || !strnicmp(word, (char*)"z", 1));
	}
	else if (len == 2) {
		return (!strnicmp(word, (char*)"7z", 2) || !strnicmp(word, (char*)"ai", 2) || !strnicmp(word, (char*)"cs", 2) || !strnicmp(word, (char*)"db", 2) || !strnicmp(word, (char*)"gz", 2) || !strnicmp(word, (char*)"js", 2) || !strnicmp(word, (char*)"pl", 2) || !strnicmp(word, (char*)"ps", 2) || !strnicmp(word, (char*)"py", 2) || !strnicmp(word, (char*)"rm", 2) || !strnicmp(word, (char*)"sh", 2) || !strnicmp(word, (char*)"vb", 2));
	}
	else if (len == 4) {
		return (!strnicmp(word, (char*)"aspx", 4) || !strnicmp(word, (char*)"docx", 4) || !strnicmp(word, (char*)"h264", 4) || !strnicmp(word, (char*)"heic", 4) || !strnicmp(word, (char*)"html", 4) || !strnicmp(word, (char*)"icns", 4) || !strnicmp(word, (char*)"indd", 4) || !strnicmp(word, (char*)"java", 4) ||
			!strnicmp(word, (char*)"jpeg", 4) || !strnicmp(word, (char*)"json", 4) || !strnicmp(word, (char*)"mpeg", 4) || !strnicmp(word, (char*)"midi", 4) || !strnicmp(word, (char*)"pptx", 4) || !strnicmp(word, (char*)"sitx", 4) || !strnicmp(word, (char*)"tiff", 4) || 
			!strnicmp(word, (char*)"xlsx", 4) || !strnicmp(word, (char*)"zipx", 4));
	}
	else {
		return (!strnicmp(word, (char*)"class", 5) || !strnicmp(word, (char*)"gadget", 6) || !strnicmp(word, (char*)"swift", 5) || !strnicmp(word, (char*)"tar.gz", 6) || !strnicmp(word, (char*)"toast", 5) || !strnicmp(word, (char*)"xhtml", 5));
	}
}

bool IsFileName(char* word)
{
	// check for a file extension
	char* ext = strrchr(word, '.');
	if (!ext) return false;

	// upreference
	if (word[0] == '.' && word[1] == '.' && word[2] == '/') return true;
	// local dir ref
	if (word[0] == '.' && word[1] == '/' && IsAlphaUTF8(word[2])) return true;
	// drive ref
	if (word[0] == 'C' && word[1] == ':' && word[2] == '/') return true;
	if (word[0] == 'D' && word[1] == ':' && word[2] == '/') return true;
	if (word[0] == 'E' && word[1] == ':' && word[2] == '/') return true;

	char* ptr = word - 1;
	char* last = ptr + strlen(word);
	char first = *word;

	// ignore wrapping quotes
	if ((first == '"' || first == '\'') && *last == first) {
		ptr++;
		*last = 0;
	}
	else {
		first = 0;
	}

	bool valid = IsFileExtension(ext + 1);
	if (valid) {
		if (ptr[1] == '*' && (ptr + 2) == ext) ptr = ext;		// *.ext
		else if (IsAlphaUTF8(ptr[1]) && ptr[2] == ':') { ptr += 2; }	// Windows drive

		while (valid && ++ptr < ext) {
			if (IsAlphaUTF8OrDigit(*ptr) || *ptr == '/' || *ptr == '\\') continue;
			if (first && *ptr == ' ') continue;  // spaces inside a quoted name
			valid = false;
		}
	}

	if (first) *last = first;
	return valid;
}

bool IsEmojiShortname(char* word, bool all)
{
	size_t len = strlen(word);
	if (len < 3 || word[0] != ':') return false;
	char* at = ++word;
	len -= 2;

	// check for a known emjoi short names
	int* emoji = emojishortnames;
	for (unsigned int i = 0; i < 10000; ++i)
	{
		if (!emoji || !*emoji) break;
		char* w = Index2Heap(*emoji);
		emoji++;

		size_t n = strlen(w);
		if (n > len || at[n] != ':') continue;

		char c = at[n];
		at[n] = 0;
		if (!stricmp(w, at))
		{
			at[n] = c;
			return (all ? n == len : true);
		}
		at[n] = c;
	}

	return false;
}

unsigned int IsMadeOfInitials(char* word, char* end)
{
	if (IsDigit(*word)) return 0; // it's a number
	char* ptr = word - 1;
	while (++ptr < end) // check for alternating character and periods
	{
		if (!IsAlphaUTF8OrDigit(*ptr)) return false;
		if (*++ptr != '.') return false;
	}
	if (ptr >= end)  return ABBREVIATION; // alternates letter and period perfectly (abbreviation or middle initial)

	//   if lower case start and upper later, it's a typo. Call it a shout (will lower case if we dont know as upper)
	ptr = word - 1;
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
		WORDP set[GETWORDSLIMIT];
		int n = GetWords(word, set, true);
		while (n)
		{
			WORDP X = set[--n];
			if (!strcmp(word, X->word)) return 0;	// we know it in all caps format
		}
	}
	return (D && D->properties & (NOUN_PROPER_SINGULAR | NOUN_PROPER_PLURAL)) ? ABBREVIATION : SHOUT;
}

////////////////////////////////////////////////////////////////////
/// READING ROUTINES
////////////////////////////////////////////////////////////////////

char* ReadFlags(char* ptr, uint64& flags, bool& bad, bool& response, bool factcall)
{
	flags = 0;
	response = false;
	char* start = ptr;
	if (!*ptr) return start;
	if (*ptr != '(') // simple solo flag
	{
		char word1[MAX_WORD_SIZE];
		char* word = word1;
		ptr = ReadCompiledWord(ptr, word);
		if (*word == '$') {
			char* val = GetUserVariable(word, false);
			val = ReadCompiledWord(val, word);
		}
		if (!strnicmp(word, (char*)"RESPONSE_", 9)) response = true; // saw a response flag
		if (IsDigit(*word) || *word == 'x') ReadInt64(word, (int64&)flags);
		else
		{
			flags = FindPropertyValueByName(word);
			if (!flags) flags = FindSystemValueByName(word);
			if (!flags) flags = FindMiscValueByName(word);
			if (!flags) bad = true;
		}
		if (factcall) return ptr;
		return  (!flags && !IsDigit(*word) && *word != 'x' && !response) ? start : ptr;	// if found nothing return start, else return end
	}

	char flag[MAX_WORD_SIZE];
	char* close = strchr(ptr, ')');
	if (close) *close = 0;
	while (*ptr) // read each flag
	{
		FunctionResult result;
		ptr = ReadShortCommandArg(ptr, flag, result); // swallows excess )
		if (result & ENDCODES) return ptr;
		if (*flag == '(') continue;	 // starter
		if (*flag == USERVAR_PREFIX) flags |= atoi(GetUserVariable(flag)); // user variable indirect
		else if (*flag == '0' && (flag[1] == 'x' || flag[1] == 'X')) // literal hex value
		{
			uint64 val;
			ptr = ReadHex(flag, val);
			flags |= val;
		}
		else if (IsDigit(*flag)) flags |= atoi(flag);
		else  // simple name of flag
		{
			uint64 n = FindPropertyValueByName(flag);
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

char* ReadInt(char* ptr, int& value)
{
	ptr = SkipWhitespace(ptr);

	value = 0;
	if (!ptr || !*ptr) return ptr;
	if (*ptr == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))  // hex number
	{
		uint64 val;
		ptr = ReadHex(ptr, val);
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
			Bug(); // ReportBug((char*)"bad number %s\r\n", original);
			while (*++ptr && *ptr != ' ');
			value = 0;
			return ptr;
		}
	}
	value *= sign;
	if (*ptr) ++ptr; // skip trailing blank
	return ptr;
}

int64 atoi64(char* ptr)
{
	int64 spot;
	ReadInt64(ptr, spot);
	return spot;
}

char* ReadInt64(char* ptr, int64& spot)
{
	ptr = SkipWhitespace(ptr);
	spot = 0;
	if (!ptr || !*ptr) return ptr;
	if (*ptr == 'x') return ReadHex(ptr, (uint64&)spot);
	if (*ptr == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) return ReadHex(ptr, (uint64&)spot);
	char* original = ptr;
	int sign = 1;
	if (*ptr == '-')
	{
		sign = -1;
		++ptr;
	}
	else if (*ptr == '+') ++ptr;
	char* currency1;
	char* cur1 = (char*)GetCurrency((unsigned char*)ptr, currency1); // alpha part
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
			Bug(); // ReportBug((char*)"bad number1 %s\r\n", original);
			while (*++ptr && *ptr != ' ');
			spot = 0;
			return ptr;
		}
	}
	spot *= sign;
	if (*ptr) ++ptr;	// skip trailing blank
	return ptr;
}

char* ReadHex(char* ptr, uint64& value)
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
		value += (IsDigit(c)) ? (c - '0') : (10 + c - 'a');
	}
	if (*ptr) ++ptr;// align past trailing space
	return ptr;
}

void BOMAccess(int& BOMvalue, char& oldc, int& oldCurrentLine) // used to get/set file access- -1 BOMValue gets, other sets all
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

static bool ConditionalReadRejected(char* start, char*& buffer, bool revise)
{
	char word[MAX_WORD_SIZE];
	if (!compiling) return false;
	char* at = ReadCompiledWord(start, word);
	if (!stricmp(word, "ifdef") || !stricmp(word, "ifndef") || !stricmp(word, "define")) return false;
	if (!stricmp(word, "endif") || !stricmp(word, "include")) return false;

	// a matching language declaration?
	if (!stricmp(current_language, word))
	{
		if (revise)
		{
			memmove(start - 1, at, strlen(at) + 1); // erase comment marker
			buffer = start + strlen(start);
		}
		return false; // allowed
	}

	// could it be a constant?
	uint64 n = FindPropertyValueByName(word);
	if (!n) n = FindSystemValueByName(word);
	if (!n) n = FindParseValueByName(word);
	if (!n) n = FindMiscValueByName(word);
	if (n) return false;	 // valid constant
	for (int i = 0; i < conditionalCompiledIndex; ++i)
	{
		if (!stricmp(conditionalCompile[i] + 1, word))
		{
			if (revise)
			{
				memmove(start - 1, at, strlen(at) + 1); // erase comment marker
				buffer = start + strlen(start);
			}
			return false; // allowed
		}
	}
	return true; // reject this line 
}

static bool PurifyUTF8(PurifyKind kind, char* start, char c, char*& read, char*& write)
{
	bool hasbadutf = false;
	char prior = (start == read) ? 0 : *(read - 1);
	char utfcharacter[10];
	char* x = IsUTF8(read, utfcharacter); // return after this character if it is valid.
	if (utfcharacter[1]) // rewrite some utf8 characters to std ascii
	{
		if (c == 0xC2 && read[1] == 0x00A0)  // change non breaking space to normal
		{
			*write++ = ' ';
			++read;
		}
		else if (c == 0xEF && read[1] == 0xBB && read[2] == 0xBF) read += 2; // UTF8 BOM
		else if (c == 0xc2 && read[1] == 0xb4) // a form of '
		{
			*write++ = '\'';
			read++;
		}
		else if (c == 0xc2 && read[1] == 0xa9) // copyright sign
		{
			read += 2;
			*write++ = '@';
			*write++ = 'c';
		}
		else if (c == 0xc2 && read[1] == 0xae) // registered sign
		{
			read += 2;
			*write++ = '@';
			*write++ = 'r';
		}
		else if (c == 0xe2 && read[1] == 0x84 && read[2] == 0xa2) // trademark sign
		{
			read += 3;
			*write++ = '@';
			*write++ = 't';
			*write++ = 'm';
		}
		else if (c == 0xe2 && read[1] == 0x84 && read[2] == 0xa0) // servicemark sign
		{
			read += 3;
			*write++ = '@';
			*write++ = 's';
			*write++ = 'm';
		}
		else if (c == 0xe2 && read[1] == 0x80 && read[2] == 0x98)  // open single quote
		{
			*write++ = '\'';
			read += 2;
		}
		else if (c == 0xef && read[1] == 0xbc && read[2] == 0x88)  // open curly paren 
		{
			*write++ = '(';
			read += 2;
		}
		else if (c == 0xef && read[1] == 0xbc && read[2] == 0x89)  // closing curly paren 
		{
			*write++ = ')';
			read += 2;
		}
		else if (c == 0xe2 && read[1] == 0x80 && read[2] == 0x99)  // closing single quote (may be embedded or acting as a quoted expression
		{
			*write++ = '\'';
			read += 2;
		}
		else if (c == 0xe2 && read[1] == 0x80 && read[2] == 0x9c)  // open double quote
		{
			*write++ = '\\';
			*write++ = '"';
			read += 2;
		}
		else if (c == 0xe2 && read[1] == 0x80 && read[2] == 0x9d)  // closing double quote
		{
			*write++ = '\\';
			*write++ = '"';
			read += 2;
		}
		else if (kind != INPUT_TESTOUTPUT_PURIFY && c == 0xe2 && read[1] == 0x80 && read[2] == 0x94 && !(tokenControl & NO_FIX_UTF))  // mdash
		{
			if (!compiling)
			{
				*write++ = '-';
				read += 2;
			}
			else
			{
				WARNSCRIPT("UTF8 mdash seen  %s | %s", start, read);
				*write++ = *read++;
				*write++ = *read++;
				*write++ = *read;
			}
		}
		else if (kind != INPUT_TESTOUTPUT_PURIFY && c == 0xe2 && read[1] == 0x80 && (read[2] == 0x94 || read[2] == 0x93) && !(tokenControl & NO_FIX_UTF))  // mdash
		{
			if (!compiling)
			{
				*write++ = '-';
				read += 2;
			}
			else
			{
				WARNSCRIPT("UTF8 mdash seen  %s | %s", start, read);
				*write++ = *read++;
				*write++ = *read++;
				*write++ = *read;
			}
		}
		else if (kind == INPUT_TESTOUTPUT_PURIFY) // accept all chars
		{
			if (!utfcharacter[2]) // 2bytes
			{
				*write++ = *read++;
				*write++ = *read;
			}
			else if (!utfcharacter[3]) // 3bytes
			{
				*write++ = *read++;
				*write++ = *read++;
				*write++ = *read;
			}
			else if (!utfcharacter[4]) // 4bytes
			{
				*write++ = *read++;
				*write++ = *read++;
				*write++ = *read++;
				*write++ = *read;
			}
			else
			{
				hasbadutf = true;
				*write++ = 'z';	// replace with legal char
			}
		}
		else if ((c & 0xe0) == 0xc0 && (read[1] & 0xc0) == 0x80) // 2byte utf8
		{
			*write++ = *read++;
			*write++ = *read;
		}
		else if ((c & 0xf0) == 0xe0 && (read[1] & 0xc0) == 0x80 && (read[2] & 0xc0) == 0x80) // 3byte utf8
		{
			*write++ = *read++;
			*write++ = *read++;
			*write++ = *read;
		}
		else if ((c & 0xf8) == 0xf0 && (read[1] & 0xc0) == 0x80 && (read[2] & 0xc0) == 0x80 && (read[3] & 0xc0) == 0x80) // 4byte utf8
		{
			*write++ = *read++;
			*write++ = *read++;
			*write++ = *read++;
			*write++ = *read;
		}
	}
	else // invalid utf8
	{
		hasbadutf = true;
		*write++ = 'z';	// replace with legal char
	}
	return hasbadutf;
}

static void PurifyHTML(char c, char*& read, char*& write, bool withinquote)
{
	if (read[1] == '#')
	{
        int n = 0;
        int offset = 0;
        if (read[2] == 'x' || read[2] == 'X')
        {
            uint64 val;
            char* ptr = ReadHex(read+2, val);
            if (*(--ptr) == ';' && val > 0)
            {
                n = (int)val;
                offset = (ptr - read);
            }
        }
        else
        {
            if (IsDigit(read[2]) && IsDigit(read[3]) && read[4] == ';') //eg &#32;
            {
                n = atoi(read + 2);
                offset = 4;
            }
            else if (IsDigit(read[2]) && IsDigit(read[3]) && IsDigit(read[4]) && IsDigit(read[5]) && read[6] == ';') //eg &#8216;
            {
                n = atoi(read + 2);
                offset = 6;
            }
        }
		// convert normal ascii characters out of html, except
		// underscore leave alone because this gets around cs output purify on target=&#95;blank 
		if (n > 0 && n != 95)
		{
            // some numeric codes for double quotes need to be escaped
            if (n == 34 || n == 171 || n == 187 || n == 8216 || n == 8217 || n == 8220 || n == 8221 || n == 8222)
            {
                if (withinquote) *write++ = '\\';
                *write++ = '"';
                read += offset;
            }
            else if (n < 256)
            {
                *write++ = (char)n; // normal ascii value
                read += offset;
            }
            else *write++ = c;
		}
		else *write++ = c;
	}
	else if (!strnicmp(read, (char*)"&tab;", 5))
	{
		*write++ = ' ';
		read += 4;
	}
	else if (!strnicmp(read, (char*)"&newline;", 9))
	{
		*write++ = ' ';
		read += 8;
	}
	else if (!strnicmp(read, (char*)"&apos;", 6))
	{
		*write++ = '\'';
		read += 5;
	}
	else if (!strnicmp(read, (char*)"&lt;", 4))
	{
		*write++ = '<';
		read += 3;
	}
	else if (!strnicmp(read, (char*)"&gt;", 4))
	{
		*write++ = '>';
		read += 3;
	}
	else if (!strnicmp(read, (char*)"&lsquot;", 8) || !strnicmp(read, (char*)"&rsquot;", 8))
	{
        if (withinquote) *write++ = '\\';
		*write++ = '"';
		read += 7;
	}
	else if (!strnicmp(read, (char*)"&rsquo;", 7) || !strnicmp(read, (char*)"&lsquo;", 7))
	{
        if (withinquote) *write++ = '\\';
		*write++ = '"';
		read += 6;
	}
	else if (!strnicmp(read, (char*)"&rsquote;", 9) || !strnicmp(read, (char*)"&lsquote;", 9)) // not real, someones typo
	{
        if (withinquote) *write++ = '\\';
		*write++ = '"';
		read += 8;
	}
	else if (!strnicmp(read, (char*)"&rsquor;", 8) || !strnicmp(read, (char*)"&lsquor;", 8))
	{
        if (withinquote) *write++ = '\\';
		*write++ = '"';
		read += 7;
	}
	else if (!strnicmp(read, (char*)"&quot;", 6))
	{
        if (withinquote) *write++ = '\\';
		*write++ = '"';
		read += 5;
	}
	else if (!strnicmp(read, (char*)"&ldquo;", 7) || !strnicmp(read, (char*)"&rdquo;", 7))
	{
        if (withinquote) *write++ = '\\';
		*write++ = '"';
		read += 6;
	}
	else if (!strnicmp(read, (char*)"&laquo;", 7) || !strnicmp(read, (char*)"&raquo;", 7))
	{
        if (withinquote) *write++ = '\\';
		*write++ = '"';
		read += 6;
	}
	else if (!strnicmp(read, (char*)"&amp;ndash;", 11)) // buggy from some websites
	{
		*write++ = ' ';
		*write++ = '-';
		*write++ = ' ';
		read += 10;
	}
	else if (!strnicmp(read, (char*)"&ndash;", 7))
	{
		*write++ = '-';
		read += 6;
	}
	else if (!strnicmp(read, (char*)"&mdash;", 7))
	{
		*write++ = '-';
		read += 6;
	}
	else if (!strnicmp(read, (char*)"&amp;", 5))
	{
		*write++ = '&';
		read += 4;
	}
	else if (!strnicmp(read, (char*)"&nbsp;", 6))
	{
		*write++ = ' ';
		read += 5;
	}
	else *write++ = c;
}

void CheckForOutputAPI(char* msg)
{
	if (!msg) return;
	// check for output api call (compile or test)
	char c = msg[60];
	msg[60] = 0;
	outputapicall = strstr(msg, "\"testoutput\"") ? true : false;
	if (!outputapicall) outputapicall = strstr(msg, "\"compileoutput\"") ? true : false;
	msg[60] = c;
}

char* PurifyInput(char* input, char* copy,size_t& size, PurifyKind kind)
{
	bool hasbadutf = false;
	size = 0;
	if (!input) return input;

	if (kind == INPUT_PURIFY && outputapicall) // but if this is api call, dont damage output data
	{
		kind = INPUT_TESTOUTPUT_PURIFY; // dont clean testoutput (leave mdashs etc)
	}

	// stream can only shrink, so we rewrite in place potentially as we go across
	char* write = copy;
	char* read = input;
	char* start = read--;
	bool withinquote = false;
	unsigned char c;

	int oobCount = 0;
	char* oobAnswer = NULL;
	char pendingUtf8[10];
	char* pending = NULL;
	while (1)
	{
		c = *++read;
		if (!c)
		{
			if (!pending) break; // true exhaustion
			read = pending;  // resume normal
			pending = NULL;
			continue;
		}
		char originalc = c;

		// convert &# html codes to utf16 
		if (c == '&' && read[1] == '#' && read[2] == 'x' &&
			(read[6] == ';' || read[7] == ';')) // hex html
		{
			char* end = strchr(read, ';');
			unsigned int x = 0;
			char* ptr = read + 3;
			while (IsDigit(*ptr))
			{
				x = (x << 4);
				char c1 = toUppercaseData[*ptr++];
				if (c1 >= 'A') c1 = 10 + c1 - 'A';
				else c1 -= '0';
				x += c1;
			}

			char data[100];
			sprintf(data, "\\u%04X", x);
			size_t n = strlen(data);
			strncpy(read, data, n);
			memmove(read + n, end + 1, strlen(end));
		}
		else if (read[1] == '#' && IsDigit(read[2]) &&
			(read[5] == ';' || read[6] == ';')) // decimal html
		{
			char* end = strchr(read, ';');
			unsigned int x = 0;
			char* ptr = read + 2;
			while (IsDigit(*ptr))
			{
				x = (x * 10) + (*ptr++ - '0');
			}
			char data[100];
			sprintf(data, "\\u%04X", x);
			size_t n = strlen(data);
			strncpy(read, data, n);
			memmove(read + n, end + 1, strlen(end));
		}

		// convert utf16 \unnnn encode from JSON to our std utf8
		if ( c == '\\' && read[1] == 'u' && IsHexDigit(read[2]) && IsHexDigit(read[3]) && IsHexDigit(read[4]) && IsHexDigit(read[5]))
		{
			char* utf8 = UTF16_2_UTF8(read + 1, withinquote);
			if (utf8)
			{
				read += 5; // skip what we read
				pending = read;  // transfer input to process the new utf8
				pendingUtf8[0] = 0;
				if (*utf8 == '"' && withinquote) strcpy(pendingUtf8, "\\");
				strcat(pendingUtf8, utf8);
				read = pendingUtf8 - 1;
			}
			else // declined to change
			{
				*write++ = *read;
			}
			continue;
		}

		// now all characters are ascii or utf8 (except for illegal utf16 characters shipped us)
		if (originalc == '"')
		{
			*write++ = '"';
			withinquote = !withinquote;
		}
		else if (c == '\\')
		{
			if (read[1] != '\'')
				*write++ = '\\'; // can\'t
			*write++ = *++read;
			continue;
		}
		else if (c == '`') 
			*write++ = '\''; // do not allow backtick from outside, make forward
		else if (c == '\t' && kind != TAB_PURIFY) *write++ = ' '; // convert control stuff to spaces - protect logging usage
		else if (c < 32 && c != '\t' && kind != SYSTEM_PURIFY) *write++ = ' '; // convert control stuff to spaces - protect logging usage
		else if (c == 0x92) *write++ = '\''; // windows smart quote not real unicode
		else if (tokenControl & NO_FIX_UTF || kind == INPUT_TESTOUTPUT_PURIFY)
		{ // directly accept utf8 and regular as is
			if ((c & 0xe0) == 0xc0 && (read[1] & 0xc0) == 0x80) // 2byte utf8
			{
				*write++ = *read++;
			}
			else if ((c & 0xf0) == 0xe0 && (read[1] & 0xc0) == 0x80 && (read[2] & 0xc0) == 0x80) // 3byte utf8
			{
				*write++ = *read++;
				*write++ = *read++;
			}
			else if ((c & 0xf8) == 0xf0 && (read[1] & 0xc0) == 0x80 && (read[2] & 0xc0) == 0x80 && (read[3] & 0xc0) == 0x80) // 4byte utf8
			{
				*write++ = *read++;
				*write++ = *read++;
				*write++ = *read++;
			}
			*write++ = *read; // simple ascii or last byte
			// Find start and end of OOB
			if (*read == ' ') {} // can be ignored
			else if (!withinquote && originalc == '[')
			{
				++oobCount;
			}
			else if (!withinquote && originalc == ']')
			{
				--oobCount;
				if (oobCount == 0)
				{
					oobCount = 1000000; // dont ever react again to ] in user inputs
					oobAnswer = write; // start of potential user input AFTER oob
				}
			}
			else if (!oobCount) oobCount = 1000000; // we arent starting with [, so block oob detect
		}
		else if (c == '&') // html?
		{
			PurifyHTML(c, read, write, withinquote);
		}
		else if (c & 0x80) // is utf8 in theory
		{
			hasbadutf |= PurifyUTF8(kind, start, c, read, write);
		} // end detected utf8's
		else // normal char
		{
			*write++ = *read;

			// Find start and end of OOB
			if (*read == ' ') {} // can be ignored
			else if (!withinquote && originalc == '[')
			{
				++oobCount;
			}
			else if (!withinquote && originalc == ']')
			{
				--oobCount;
				if (oobCount == 0)
				{
					oobCount = 1000000; // dont ever react again to ] in user inputs
					oobAnswer = write; // start of potential user input AFTER oob
				}
			}
			else if (!oobCount)   oobCount = 1000000; // we arent starting with [, so block oob detect
		}
	}
	*write++ = 0;

	size = write - copy;
	return oobAnswer;
}

bool AdjustUTF8(char* start, char* buffer)
{
	bool withinquote = false;
	bool hasbadutf = false;
	while (*++buffer)  // check every utf character
	{
		// if we have things we change to quotes that are within quotes, we have to escape them
		if (*buffer == '"' && *(buffer - 1) != '\\')
			withinquote = !withinquote;

		// utf16 \u encode
		if (*buffer == '\\' && buffer[1] == 'u'  && IsHexDigit(buffer[2])
			&& IsHexDigit(buffer[3]) && IsHexDigit(buffer[4]) && IsHexDigit(buffer[5]))
		{ // 00e7
			int n = atoi(buffer + 2);
			char c = buffer[5];
            char c4 = buffer[4];
            char c3 = buffer[3];
            char c2 = buffer[2];
			if (n == 2018 || n == 2019 || (n == 201 && (c == 'a' || c == 'b' || c == 'A' || c == 'B'))) // left and right curly ' to normal '
			{
				*buffer = '\'';
				memmove(buffer + 1, buffer + 6, strlen(buffer + 5));
			}
			else if (n == 201 && (c == 'c' || c == 'd' || c == 'e' || c == 'f' || c == 'C' || c == 'D' || c == 'E' || c == 'F')) // left and right curly dq to normal dq
			{
				// if withinquote need backslash for json
				if (withinquote) // presuming we will see corresponding other dq
				{
					buffer[1] = '"';
					memmove(buffer + 2, buffer + 6, strlen(buffer + 5));
					++buffer;
					withinquote = false;
				}
				else
				{
					*buffer = '"';
					memmove(buffer + 1, buffer + 6, strlen(buffer + 5));
					withinquote = true;
				}
			}
            else if (n == 0 && c2 == '0' && c3 == '0' && (c == 'b' || c == 'B') && (c4 == 'a' || c4 == 'b' || c4 == 'A' || c4 == 'B')) // double guillemets
            {
                *buffer = '\"';
                memmove(buffer + 1, buffer + 6, strlen(buffer + 5));
            }
            else if (n == 2039 || (n == 203 && (c == 'a' || c == 'A'))) // single guillemets
            {
                *buffer = '\'';
                memmove(buffer + 1, buffer + 6, strlen(buffer + 5));
            }
            // we dont like utf16 characters, but we have to live with them due to JSON.
		    // They dont work well in word match/spell correct
			buffer += 5;
		}
		else if (*buffer == '\\')
		{
            if (*(buffer+1) & 0x80) {;}
			else ++buffer;
		}
		else if (*buffer & 0x80) // is utf8 in theory
		{
			char prior = (start == buffer) ? 0 : *(buffer - 1);
			char utfcharacter[10];
			char* x = IsUTF8(buffer, utfcharacter); // return after this character if it is valid.
            if (prior == '\\' && x > buffer)
            {
                buffer = x - 1;
            }
			else if (utfcharacter[1]) // rewrite some utf8 characters to std ascii
			{
				if (buffer[0] == 0xc2 && buffer[1] == 0xb4) // a form of '
				{
					*buffer = '\'';
					memmove(buffer + 1, x, strlen(x) + 1);
					x = buffer + 1;
					if (compiling) WARNSCRIPT("UTF8 B4 ' revised to Ascii %s | %s", start, buffer);
				}
                else if (buffer[0] == 0xc2 && (buffer[1] == 0xab || buffer[1] == 0xbb)) // double guillemets
                {
                    *buffer = '\"';
                    memmove(buffer + 1, x, strlen(x) + 1);
                    x = buffer + 1;
                    if (compiling) WARNSCRIPT("UTF8 guillemet ' revised to Ascii %s | %s",start,buffer);
                }
                else if (buffer[0] == 0xe2 && (buffer[1] == 0x80 && (buffer[2] == 0xb9 || buffer[2] == 0xba))) // single guillemets
                {
                    *buffer = '\'';
                    memmove(buffer + 1, x, strlen(x) + 1);
                    x = buffer + 1;
                    if (compiling) WARNSCRIPT("UTF8 guillemet ' revised to Ascii %s | %s",start,buffer);
                }
                else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && (buffer[2] == 0x98 || buffer[2] == 0x9a))  // open single quote
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
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && (buffer[2] == 0x9c || buffer[2] == 0x9e))  // open double quote
				{
					if (withinquote)
					{
						*buffer = '\\';
						buffer[1] = '"';
						memmove(buffer + 2, x, strlen(x) + 1);
                        x = buffer + 2;
					}
					else
					{
						*buffer = '"';
						memmove(buffer + 1, x, strlen(x) + 1);
                        x = buffer + 1;
					}
					if (compiling) WARNSCRIPT("UTF8 opening double quote revised to Ascii %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xef && buffer[1] == 0xbc && buffer[2] == 0x88)  // open curly paren 
				{
					*buffer = '(';
					memmove(buffer + 1, x, strlen(x) + 1);
                    x = buffer + 1;
					if (compiling) WARNSCRIPT("UTF8 opening curly paren quote revised to Ascii %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xef && buffer[1] == 0xbc && buffer[2] == 0x89)  // closing curly paren 
				{
					*buffer = ')';
					memmove(buffer + 1, x, strlen(x) + 1);
                    x = buffer + 1;
					if (compiling) WARNSCRIPT("UTF8 closing curly paren quote revised to Ascii %s | %s", start, buffer);
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && buffer[2] == 0x9d)  // closing double quote
				{
					if (withinquote)
					{
						*buffer = '\\';
						buffer[1] = '"';
						memmove(buffer + 2, x, strlen(x) + 1);
                        x = buffer + 2;
					}
					else
					{
						*buffer = '"';
						memmove(buffer + 1, x, strlen(x) + 1);
                        x = buffer + 1;
					}
					if (compiling) WARNSCRIPT("UTF8 closing double quote revised to Ascii %s | %s", start, buffer)
				}
				else if (buffer[0] == 0xe2 && buffer[1] == 0x80 && (buffer[2] == 0x93 || buffer[2] == 0x94) && !(tokenControl & NO_FIX_UTF))  // mdash
				{
					if (!compiling)
					{
						*buffer = '-';
						memmove(buffer + 1, x, strlen(x) + 1);
                        x = buffer + 1;
					}
					else WARNSCRIPT("UTF8 mdash seen  %s | %s", start, buffer);
				}
				buffer = x - 1; // valid utf8
			}
			else if (*buffer == 0x92) // windows smart quote not real unicode
			{
				*buffer = '\'';
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

char* SkipOOB(char* buffer)
{
	// skip oob data 
	char* at = SkipWhitespace(buffer);
	char* start = at;
	bool traceskip = false; // if you need to see debugging
	bool full = false;
	char data[100];
	if (*at != '[') return buffer;
	else
	{
		int bracket = 1;
		int curly = 0;
		bool quote = false;
		int blocklevel = 0;
		char c;
		int concept = 0;
		while ((c = *++at))
		{
			if (full)
			{
				char tmp[50];
				strncpy(tmp, at, 40);
				tmp[40] = 0;
				Log(ECHOUSERLOG, "%c %s %d\r\n", c, tmp, quote);
			}
			if (c == '\\') // protected char
			{
				if (*++at == 'u' && IsDigit(at[1]))  at += 4; // skip past this utf16 character uxxxx
				continue;
			}

			if (c == '"')
			{
				quote = !quote; // ignore within quotes
				continue;
			}
			else if (quote) continue;
			else if (c == '{') // only top levels, not deep in patterns
			{
				++curly;
				if (traceskip && curly == 3)
				{
					if (!strnicmp(at,"{\"concept", 9))
					{
						concept = bracket + 1; // ignore all within []
					}
				}
				if (traceskip && !concept && curly <= 6 && !blocklevel)
				{
					strncpy(data, at, 40);
					data[40] = 0;
					int n = bracket + curly;
					for (int i = 0; i <= n; ++i) printf("  ");
					printf("%d: %s\r\n", n, data);
				}
			}
			else if (c == ',') // only top levels, not deep in patterns
			{
				if (traceskip &&  !concept && curly <= 6 && !blocklevel)
				{
					strncpy(data, at, 40);
					data[40] = 0;
					int n = bracket + curly;
					for (int i = 0; i <= n; ++i) printf("  ");
					printf("%d: %s\r\n", n, data);
				}
			}
			else if (c == '}')
			{
				--curly;
				if (blocklevel > (curly + bracket)) blocklevel = 0;
			}
			else if (c == '[')
			{
				++bracket;
				if (traceskip && !concept && bracket <= 6 && !blocklevel)
				{
					strncpy(data, at, 40);
					data[40] = 0;
					int n = bracket + curly;
					for (int i = 0; i <= n; ++i) printf("  ");
					printf("%d: %s\r\n", n, data);
				}
			}
			else if (c == ']')
			{
				if (bracket == concept) concept = 0; // turn off ignore
				--bracket;
				if (blocklevel > (curly + bracket)) blocklevel = 0;
				if (bracket == 0 && curly == 0)
				{
					at = SkipWhitespace(at + 1);	// after the oob end
					return at;
				}
			}
		}
		ReportBug("INFO: SkipOOB failed %d \r\n", strlen(originalUserInput));
	}
	return start; // didnt validate incoming oob
}

int ReadALine(char* buffer, FILE* in, unsigned int limit, bool returnEmptyLines, bool changeTabs, bool untouched)
{ //  reads text line stripping of cr/nl
	currentFileLine = maxFileLine; // revert to best seen
	char* startbuffer = buffer;
	if (currentFileLine == 0)
	{
		BOM = (BOM == BOMSET) ? BOMSET : NOBOM; // start of file, set BOM to null
		hasHighChar = false; // for outside verify
	}
	*buffer = 0;
	buffer[1] = 0;
	buffer[2] = 1;
	if (in == (FILE*)-1)
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
	char priorc = 0;
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
	char* utf16 = NULL;
	while (ALWAYS)
	{
		if (in)
		{
			if (!fread(&c, 1, 1, in))
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

		if (c == '\t' && changeTabs) c = ' ';
		if (c & 0x80 && !untouched) // high order utf?
		{
			unsigned char convert = 0;
			unsigned char x = 0;
			if (!BOM && !hasutf && !server) // local mode might get extended ansi so transcribe to utf8 (but dont touch BOM)
			{
				x = (unsigned char)c;
				if (x == 147 || x == 148) convert = c = '"';
				else if (x == 145 || x == 146) convert = c = '\'';
				else if (x == 150 || x == 151) convert = c = '-';
				else
				{
					convert = extendedascii2utf8[x - 128];
					if (convert)
					{
						*buffer++ = (unsigned char)0xc3;
						c = convert;
					}
				}
			}
			if (!convert) hasutf = true;
			else hasHighChar = x; // we change extended ansi to normal (not utf8)
		}
		else if (utf16) // possible \unnnn unicode character
		{
			int size = (buffer - utf16);
			if (size == 1 && c != 'u' ) utf16 = NULL;
			else if (size == 2 && !IsDigit(c)) utf16 = NULL; // digit 1
			else if (size == 3 && !IsDigit(c)) utf16 = NULL; // digit 2
			else if (size == 4 && (IsDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))// digit 3  - digit 4 may be hex
			{
				hasutf = true;
				utf16 = NULL;
			}
			else if (size >= 4) utf16 = NULL;
		}
		else if (c == '\\' && !untouched) utf16 = buffer;

		// format string stuff
		if (formatString)
		{
			if (formatString == IN_FORMAT_COMMENT && c != '\r' && c != '\n') continue; // flush comment
			//  format string
			else if (formatString && c == ender && *(buffer - 1) != '\\')
				formatString = NOT_IN_FORMAT_STRING; // end format string
			else if (formatString == IN_FORMAT_CONTINUATIONLINE)
			{
				if (c == ' ' || c == '\t') continue;	// ignore white space leading the line
				formatString = IN_FORMAT_STRING;
			}
			else if (formatString && formatString != IN_FORMAT_COMMENT && *(buffer - 1) == '#' && *(buffer - 2) != '\\' && c == ' ')  // comment start in format string
			{
				formatString = IN_FORMAT_COMMENT; // begin format comment
				buffer -= 2;
				continue;
			}
		}
		else if (compiling && !blockComment && (c == '"' || c == '\'') && (buffer - start) > 1 && *(buffer - 1) == '^')
		{
			formatString = IN_FORMAT_STRING; // begin format string
			ender = c;
		}

		// buffer overflow  handling
		if ((unsigned int)(buffer - start) >= (limit - 120))
		{
			runningOut = true;
			if ((unsigned int)(buffer - start) >= (limit - 80))  break;	 // couldnt find safe space boundary to break on
		}
		if (c == ' ' && runningOut) c = '\r';	// force termination on safe boundary

		if (c == '\r')  // part of cr lf or just a cr?
		{
			if (priorc == '^') // explicit continuation line
			{
				continue; // ignore \r
			}

			if (formatString) // unclosed format string continues onto next line
			{
				while (*--buffer == ' ') { ; }
				*++buffer = ' '; // single space at line split
				++buffer;
				formatString = IN_FORMAT_CONTINUATIONLINE;
				continue; // glide on past
			}

			c = 0;
			if (in)
			{
				if (fread(&c, 1, 1, in) == 0)
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
				maxFileLine = ++currentFileLine;	// for debugging error messages
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

			if (priorc == '^') // explicit continuation line
			{
				--buffer;
				*buffer = 0;  // remove continuation line marker
				continue; // ignore \n
			}
			if (formatString)
			{
				formatString = IN_FORMAT_CONTINUATIONLINE;
				while (*--buffer == ' ') { ; }
				*++buffer = ' '; // single space at line split
				++buffer;
				continue;
			}

			if (buffer == start) // read nothing as content
			{
				*start = 0;
				if (in == stdin || returnEmptyLines) return 0;
				continue;
			}
			if (blockComment)
			{
				if (endingBlockComment)
				{
					endingBlockComment = blockComment = false;
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
		if (c == '\\' && !untouched)
			utf16 = buffer;
		else if (utf16 && (buffer - utf16) == 1 && c != 'u')
			utf16 = NULL; // not a utf16 code
		*buffer++ = c;
		*buffer = 0;
		priorc = c;

		if (utf16 && (buffer - utf16) == 6)
		{
			int n = atoi(utf16 + 2);
			if (n == 2019 || n == 2018) // left and right curly ' to normal '
			{
				buffer = utf16;
				*buffer++ = '\'';
			}
			if (n == 201 && (c == 'c' || c == 'd' || c == 'C' || c == 'D')) // left and right curly dq to normal dq
			{
				buffer = utf16;
				*buffer++ = '"';
			}
			utf16 = NULL;
		}

		// we consider utf16 files unacceptable
		if (currentFileLine == 0 && (buffer - start) == 2) // only from file system 
		{
			if ((unsigned char)start[0] == 0xFE && (unsigned char)start[1] == 0xFF) // UTF16 BOM
			{
				buffer -= 2;
				*start = 0;
				hasutf = false;
				ReportBug("File is utf16. We want UTF8");
			}
		}

		// strip UTF8 BOM marker if any and just keep reading
		if (hasutf && currentFileLine == 0 && (buffer - start) == 3) // only from file system 
		{
			if (HasUTF8BOM(start))
			{
				buffer -= 3;
				*start = 0;
				BOM = BOMUTF8;
				hasutf = false;
			}
		}

		// strip UTF8 BOM marker if any and just keep reading
		if (currentFileLine == 0 && (buffer - start) == 3) // only from file system 
		{
			if (HasUTF8BOM(start))
			{
				buffer -= 3;
				*start = 0;
				BOM = BOMUTF8;
			}
		}

		// handle block comments
		if ((buffer - start) >= 4 && *(buffer - 3) == '#' && *(buffer - 4) == '#' && *(buffer - 2) == c) //  block comment mode ## 
		{
			if (c == '<')
			{
				blockComment = true;
				if (in && !fread(&c, 1, 1, in)) // see if attached language
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
					if (in) while (fread(&c, 1, 1, in))
					{
						if (IsAlphaUTF8(c) && c != ' ') *at++ = c;
						else break;
					}
					*at = 0;
					// this is either junk or a language marker...
					if (!ConditionalReadRejected(lang, buffer, false)) blockComment = false; // this is legal
				}
				if (c == '\n')  // reached line end
				{
					++currentFileLine;	// for debugging error messages
					maxFileLine = currentFileLine;
				}
			}
			else if (c == '>')
			{
				if (!blockComment) { ; }// superfluous end
				else endingBlockComment = true;
			}
			else continue;
			buffer -= 4;
			*buffer = 0;
			if (buffer != start && !endingBlockComment) break;	// return what we have so far in the buffer before the comment
			continue;
		}
		if (blockComment && (buffer - start) >= 5)
		{
			memmove(buffer - 5, buffer - 4, 5);// erase as we go, keeping last four to detect block end ##>>
			--buffer;
			*buffer = 0;
		}

	} // end of read loop
	*buffer = 0;

	// see if conditional compile line...
	if (*start == '#' && IsAlphaUTF8(start[1]))
	{
		if (ConditionalReadRejected(start + 1, buffer, true))
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

	if (*(buffer - 1) == '\n') buffer -= 2; // remove cr/lf if there
	*buffer = 0;
	buffer[1] = 0;
	buffer[2] = 1; //   clear ahead to make it obvious we are at end when debugging

	if (hasutf && (BOM == NOBOM || BOM == BOMUTF8) && !untouched)
		hasbadutf = AdjustUTF8(start, start - 1); // DO NOT ADJUST BINARY FILES
	if (hasbadutf && showBadUTF && !server)
		Log(USERLOG, "Bad UTF-8 %s at %d in %s\r\n", start, currentFileLine, currentFilename);
	return (buffer - start);
}

char* ReadQuote(char* ptr, char* buffer, bool backslash, bool noblank, int limit)
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
		char* end = strrchr(ptr + 2, ENDUNIT);
		if (end[1] == '"') // yes we matched the special argument format
		{
			memcpy(buffer, ptr + 2, end - ptr - 2);
			buffer[end - ptr - 2] = 0;
			return end + 3; // RETURN PAST space, aiming on the )
		}
	}
	int quoteline = currentFileLine;
	int quotecolumn = currentLineColumn;
	char* quoteptr = ptr;

	*buffer++ = *ptr;  // copy over the 1st char
	char ender = *ptr;
	if (*ptr == '\\')
	{
		*buffer++ = *++ptr;	//   swallow \ and "
		ender = *ptr;
	}

	while ((c = *++ptr) && c != ender && --n)
	{
		if (c == '\\') // handle any escaped character
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
	if (!n || !c) // ran dry instead of finding the end
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
		patternStarter = quoteptr;
		patternEnder = ptr;
		currentLineColumn = quotecolumn + ptr - quoteptr;
		if (compiling) BADSCRIPT((char*)"Missing close double quotes  %s started line %d col %d %s\r\n", start, quoteline, quotecolumn, currentFilename);

		return NULL;	// no closing quote... refuse
	}

	// close with ending quote
	*buffer = ender;
	*++buffer = 0;
	return (ptr[1] == ' ') ? (ptr + 2) : (ptr + 1); // after the quote end and any space
}

char* ReadArgument(char* ptr, char* buffer, FunctionResult& result) //   looking for a single token OR a list of tokens balanced - insure we start non-space
{ //   ptr is some buffer before the arg 
#ifdef INFORMATION
	Used for function calls, to read their callArgumentList.Arguments are not evaluated here.An argument is :
	1. a token(like $var or name or "my life as a rose")
		2. a function call which runs from the function name through the closing paren
		3. a list of tokens(unevaluated), enclosed in parens like(my life as a rose)
		4. a ^ functionarg
#endif
		char* start = buffer;
	result = NOPROBLEM_BIT;
	*buffer = 0;
	int paren = 0;
	ptr = SkipWhitespace(ptr);
	char* beginptr = ptr;
	if (*ptr == '^')
	{
		ptr = ReadCompiledWord(ptr, buffer); // get name and prepare to peek at next token
		if (IsDigit(buffer[1]))
		{
			strcpy(buffer, FNVAR(buffer)); // use the value and keep going // NEW
			return ptr;
		}
		else if (*ptr != '(')  return ptr; // not a function call
		else buffer += strlen(buffer); // read onto end of call
	}
	if (*ptr == '"' && ptr[1] == FUNCTIONSTRING && dictionaryLocked) // must execute it now...
	{
		return GetCommandArg(ptr, buffer, result, 0);
	}
	else if (*ptr == '"' || (*ptr == '\\' && ptr[1] == '"'))   // a string
	{
		ptr = ReadQuote(ptr, buffer);		// call past quote, return after quote
		if (ptr != beginptr) return ptr;	// if ptr == start, ReadQuote refused to read (no trailing ")
	}

	--ptr;
	while (*++ptr)
	{
		char c = *ptr;
		int x = GetNestingData(c);
		if (paren == 0 && (c == ' ' || x == -1 || c == ENDUNIT) && ptr != beginptr) break; // simple arg separator or outer closer or end of data
		if ((unsigned int)(buffer - start) < (unsigned int)(maxBufferSize - 2)) *buffer++ = c; // limit overflow into argument area
		*buffer = 0;
		if (x) paren += x;
	}
	if (start[0] == '"' && start[1] == '"' && start[2] == 0) *start = 0;   // NULL STRING
	if (*ptr == ' ') ++ptr;
	return ptr;
}

char* ReadCompiledWordOrCall(char* ptr, char* word, bool noquote, bool var)
{
	ptr = ReadCompiledWord(ptr, word, noquote, var);
	word += strlen(word);
	if (*ptr == '(') // its a call
	{
		char* end = BalanceParen(ptr + 1, true, false); // function call args
		strncpy(word, ptr, end - ptr);
		word += end - ptr;
		*word = 0;
		ptr = end;
	}
	return ptr;
}

char* ReadPatternToken(char* ptr, char* word)
{
	ptr = SkipWhitespace(ptr);
	char* original = word;
	bool quote = false;
	bool fncall = false;
	int nest = 0;
	while (*ptr)
	{
		if (!quote && !fncall && (*ptr == ' ' || *ptr == '\t')) break; // end of token
		if (*word == '"' && *(ptr - 1) != '\\') quote = !quote; // unescaped quote
		if (*ptr == '^' && !quote)
		{
			size_t len = 1;
			char* check = ptr;
			if (IsAlphaUTF8(check[1])) // ^x
			{
				while (*++check != ' ');
				len = check - ptr;
				WORDP D = FindWord(ptr, len);
				if (D && (D->internalBits & FUNCTION_NAME)) fncall = true;
			}
		}
		if (fncall && !quote && *ptr == '(') ++nest;
		*word++ = *ptr;
		if (fncall && !quote && *ptr == ')')
		{
			--nest;
			if (!nest) break;   // ended call token
		}
		ptr++;
	}
	*word = 0;
	return ptr;
}

static char* EatString(const char* ptr, char* word, char ender, char jsonactivestring)
{
	char c;
	char* original = word;
	const char* function = NULL;
	int call_level = 0;
	while ((c = *ptr)) // run til nul or matching closing  -- may have INTERNAL string as well, as well as strings inside function calls
	{
		++ptr;
		if ((word - original) > (MAX_WORD_SIZE - 3)) break;
		if (c == '^' && jsonactivestring) // detecting function call?
		{
			if (IsAlphaUTF8(ptr[1])) // fn call
			{
				const char* at = ptr + 1;
				while (IsAlphaUTF8(*++at)); // find end of name
				if ((*at == ' ' && at[1] == '(') || *at == '(')
				{
					function = ptr; // swallow call as well
					call_level = 0;
				}
			}
		}
		if (function && c == '(') ++call_level;
		else if (function && c == ')')
		{
			--call_level;
			if (call_level == 0) function = NULL; // end of call
		}
		if (c == '\\') // escaped something, copy both
		{
			*word++ = c;
			*word++ = *ptr++;
			continue;
		}
		else if (!function && c == ender && (IsWhiteSpace(*ptr) || *ptr == ','  || nestingData[(unsigned char)*ptr] == -1 || *ptr == 0 || *ptr == '`')) // a terminator followed by white space or closing bracket or end of data terminated
		{
			*word++ = c; // put in the close tring
			if (IsWhiteSpace(*ptr)) ++ptr; // move to next item start
			break; //  found the end of string
		}
		*word++ = c;
	}
	*word = 0; // be sure from caller to do word += strlen(word) if you need to
	return (char*)ptr;
}

char* ReadToken(const char* ptr, char* word)
{
	char* start = word;
	// want entire token, but beware of :"D, ignore internal quotes not escaped.
	// beware of \"xxx \"
	// For "xxx xxx" we return the full including quotes around (should we?)
	*word = 0;
	if (!ptr) return NULL;
	char c;
	ptr = SkipWhitespace((char*)ptr);

	if (*ptr == '`') //  bounded by ` 
	{
		const char* end = strchr(ptr + 1, '`');
		if (end)
		{
			strncpy(word, ptr + 1, end - ptr - 1);
			word[end - ptr - 1] = 0;
			return (char*)(end + 1);
		}
	}

	int quote = 0;
	if (*ptr == '\'' && ptr[1] == '"') // single quoted quote string
	{
		*word++ = *ptr++;
	}
	if (*ptr == '"')
	{
		quote = 1; // starting quote
		*word++ = *ptr++;
	}
	else if (*ptr == '\\' && ptr[1] == '"')
	{
		quote = 2; // escaped quote start
		*word++ = *ptr++;
		*word++ = *ptr++;
	}
	while ((c = *ptr))
	{
		if (c == '\\') // literal next
		{
			*word++ = *ptr++;
			if (quote == 2 && *ptr == '"') quote = 0; // end escaped quoting
			*word++ = *ptr++;
			continue;
		}
		if (quote == 1 && c == '\"') quote = 0;
		if ((c == ' ' || c == '\t') && !quote)
		{
			++ptr; // aim forward
			break; // white space ended it
		}
		if (c == 0 || c == '\n') break;
		*word++ = *ptr++;
	}
	*word = 0;
	return (char*)ptr;
}

char* ReadCompiledWord(const char* ptr, char* word, bool noquote, bool var, bool nolimit)
{//   a compiled word is either characters until a blank, or a ` quoted expression ending in blank or nul. or a double-quoted on both ends or a ^double quoted on both ends
	*word = 0;
	if (!ptr) return NULL;

	char c = 0;
	char* original = word;
	ptr = SkipWhitespace((char*)ptr);
	char* start = (char*)ptr;
	char special = 0;
	char priorchar = 0;
	if (!var) { ; }
	else if ((*ptr == SYSVAR_PREFIX && IsAlphaUTF8(ptr[1])) || (*ptr == '@' && IsDigit(ptr[1])) || (*ptr == USERVAR_PREFIX && (ptr[1] == LOCALVAR_PREFIX || IsAlphaUTF8(ptr[1]) || ptr[1] == TRANSIENTVAR_PREFIX)) || (*ptr == '_' && IsDigit(ptr[1])))  special = *ptr; // break off punctuation after variable
	char jsonactivestring = 0;
	char* function = NULL;
	char ender = 0;
	if (*ptr == FUNCTIONSTRING && isAlphabeticDigitData[ptr[1]])
	{
		char* space = (char*)strchr(ptr, ' '); 
		char* paren = (char*)strchr(ptr, '(');
		if (paren) // its a function call
		{
			if (space && space < paren) {}// already its own token
			else 
			{
				strncpy(word, ptr, paren - ptr);
				word[paren - ptr] = 0;
				ptr += paren - ptr ;
				return (char*)ptr;
			}
		}
	}
	if ((*ptr == FUNCTIONSTRING && (ptr[1] == '\'' || ptr[1] == '"')) ||
		(*ptr == '"' && ptr[1] == FUNCTIONSTRING) || (*ptr == '\'' && ptr[1] == FUNCTIONSTRING && !IsDigit(ptr[2]))) // compiled or uncompiled active string, but not raw version of argument
	{
		if (*ptr == FUNCTIONSTRING) c = ptr[1];
		else c = *ptr;
		*word++ = *ptr++;  // transfer ^ active string starter
		*word++ = *ptr++;
		*word = 0;
		priorchar = 0;
		ender = jsonactivestring = c; // this is the opener marker
	}
	else if (*ptr == '\'' && ptr[1] == '"') // quoted simple string
	{
		*word++ = *ptr++;  // transfer '  starter
		*word++ = *ptr++;  // transfer " string starter
		*word = 0;
		ender = '"';
	}
	else if (*ptr == '"') // simple string
	{
		*word++ = *ptr++;  // transfer " string starter
		*word = 0;
		ender = '"';
	}
	else if (*ptr == '`') // a fact field containg special data may be bounded by ` instead of having to escape each value
	{
		const char* end = strchr(ptr + 1, '`');
		if (end)
		{
			strncpy(word, ptr + 1, end - ptr - 1);
			word[end - ptr - 1] = 0;
			return (char*)(end + 1);
		}
	}

	// this is a kind of string (regular or active). run til it ends properly
	c = 0;
	if (!noquote && (*ptr == ENDUNIT || ender)) //   ends in blank or nul,  might be quoted, doublequoted, or ^"xxx" functional string or ^'xxx' jsonactivestring
	{ // active string has to be careful about ending prematurely on fake quotes
		ptr = EatString(ptr, word, ender, jsonactivestring);
		word += strlen(word);
		if (!*(ptr - 1)) // didnt find close, it was spurious... treat as normal
		{
			ptr = start;
			word = original;
			while ((c = *ptr++) && !IsWhiteSpace(c))
			{
				if ((word - original) > (MAX_WORD_SIZE - 3)) break;
				*word++ = c; // run til nul or blank
			}
		}
	}
	else // normal token, not some kind of string (but if pattern assign it could become)
	{
		bool bracket = false;

		while ((c = *ptr) && c != ENDUNIT)
		{ // worst case is in pattern $x:=^"^join(...)" where spaces are in fn call args inside of active string
		  // may have quotes inside it, so have to clear the call before can find quote end of active string.
		  // BUG if args themselves are active strings.
		  // white space in active string or quoted string does not end token
			
		  // dont do below, so this still works: [{"dse": {"testoutput": {"output": "Just click <a id=\"dashboard-link\ href=\"http://my.justanswer.com/dashboard\ target=\"&#95;blank\">here</a> to ask something else."}}}]
		  // if (c == '"' && priorchar != '^' && priorchar != '\\' && original != word) break;
			++ptr;
			if (IsWhiteSpace(c)) break;
			if (special) // try to end a variable if not utf8 char or such
			{
				if (special == '$' && (c == '.' || c == '[') && (LegalVarChar(*ptr) || *ptr == '$' || (*ptr == '\\' && ptr[1] == '$'))) { ; } // legal data following . or [
				else if (special == '$' && c == ']' && bracket) { ; } // allowed trailing array close
				else if (special == '$' && c == '$' && (priorchar == '.' || priorchar == '[' || priorchar == '$' || priorchar == 0 || priorchar == '\\')) { ; } // legal start of interval variable or transient var continued
				else if ((special == '%' || special == '_' || special == '@') && priorchar == 0) { ; }
				else if (!LegalVarChar(c) && c != '\\')
				{
					--ptr;
					break;
				}
				if (c == '[' && !bracket) bracket = true;
				else if (c == ']' && bracket) bracket = false;
			}
			if (*ptr == ':' && ptr[1] == '=' && ptr[2] == '^' && (ptr[3] == '"' || ptr[3] == '\'')) // extend for variable assignment
			{
				*word++ = c; //   run til nul or blank or end of rule 
				*word++ = *ptr++;
				*word++ = *ptr++;
				*word++ = *ptr++;
				ender = *ptr++;
				*word++ = ender;
				*word = 0;
				ptr = EatString(ptr, word, ender, ender);
				word += strlen(word);
				break;
			}
			// leave ( ) when embedded in things since titles of software, for example, might have them
			if (!nolimit && (word - original) > (MAX_WORD_SIZE - 3)) break;
			*word++ = c; //   run til nul or blank or end of rule 
			priorchar = c;
		}
	}
	*word = 0; //   null terminate word

    return (char*)ptr;
}

char* ReadCodeWord(const char* ptr, char* word, bool noquote, bool var, bool nolimit)
{
	char* original = word;
	char* answer = ReadCompiledWord(ptr, word, noquote, var, nolimit);
	size_t len = strlen(word);
	char end = word[len - 1];

	// dont break off a variable json reference
	if (*original == '$' && end == ']') end = 0;
	else if (strchr(original, '=')) end = 0; // dont break off assignments
	else if (*original == '\\' && original[1] == end) end = 0; // dont break off escaped char
	if (original[1] && (end == ']' || end == '}' || end == ')')) //attached closing marker of some kind
	{
		word[len - 1] = 0; // separate
		end = *--answer;
		if (end != ']' && end != '}' && end != ')') --answer;
	}
	else if (end == '>' && word[1] == '>')
	{
		*--word = 0; // separate
		end = *--ptr;
		if (end != '>') --ptr;
		--ptr;
	}
	return answer;
}
char* BalanceParen(char* ptr, bool within, bool wildcards) // text starting with ((unless within is true), find the closing ) and point to next item after
{
	char* start = ptr;
	int paren = 0;
	if (within) paren = 1; // function call or pattern (
	--ptr;
	bool quoting = false;
	while (*++ptr && *ptr != ENDUNIT) // jump to END of command to resume here, may have multiple parens within
	{
		if (*ptr == '\\' && ptr[1]) // ignore slashed item
		{
			++ptr;
			continue;
		}
		if (*ptr == '"')
		{
			quoting = !quoting;
			continue;
		}
		if (quoting) continue;	// stuff in quotes is safe 
		if (wildcards && *ptr == '_' && !IsDigit(ptr[1]) && IsWhiteSpace(*(ptr - 1)))
		{
			SetWildCardNull();
		}
		int value = nestingData[(unsigned char)*ptr]; // () {} []
		// parens dominate when we have them, we dont bother to care about other kinds of levels within if we have them
		// the << >> pair arenot in table, account for them
		if (*ptr == '<' && ptr[1] == '<' && ptr[2] != '<')
		{
			value = 1;
			++ptr;
		}
		if (*ptr == '>' && ptr[1] == '>' && ptr[2] != '>')
		{
			value = -1;
			++ptr; // ignore 1st one
		}

		if (value && (paren += value) == 0)
		{
			if (ptr[1] == ' ') ptr += 2;
			else ++ptr;  //   return on next token (skip paren + space) or  out of data (typically argument substitution)
			break;
		}
	}
	return ptr; //   return past end of data
}

char* SkipWhitespace(const char* ptr)
{
	if (!ptr || !*ptr) return (char*)ptr;
	while (*ptr)
	{
		if ((csapicall == COMPILE_PATTERN || csapicall == COMPILE_OUTPUT) && *ptr == '\\' && ptr[1] == 'n') {}
		else if (!IsWhiteSpace(*ptr)) break;

		if (*ptr == '\\' && ptr[1] == 'n') // api passes newlines across. but user doing manual tabs or return, those are not whitespace but tokens
		{
			maxFileLine = ++currentFileLine;
			currentLineColumn = 0;
			ptr += 2;
			linestartpoint = ptr;
			continue;
		}
		if (!convertTabs && *ptr == '\t') return (char*)ptr; // leave to be seen
		++ptr;
	}
	return (char*)ptr;
}

///////////////////////////////////////////////////////////////
//  CONVERSION UTILITIES
///////////////////////////////////////////////////////////////

char* Purify(char* msg) // used for logging to remove real newline characters so all fits on one line
{
	if (newline) return msg; // allow newlines to remain

	char* nl = strchr(msg, '\n');  // legal 
	if (!nl) return msg; // no problem
	char* limit;
	char* buffer = InfiniteStack(limit, "Purify"); // transient
	strcpy(buffer, msg);
	nl = (nl - msg) + buffer;
	while (nl)
	{
		*nl = ' ';
		nl = strchr(nl, '\n');  // legal 
	}
	char* cr = strchr(buffer, '\r');  // legal 
	while (cr)
	{
		*cr = ' ';
		cr = strchr(cr, '\r');  // legal 
	}
	ReleaseInfiniteStack();
	return buffer; // nothing else should use ReleaseStackspace 
}

size_t OutputLimit(unsigned char* data) // insert eols where limitations exist
{
	char* original1 = (char*)data;
	char extra[HIDDEN_OVERLAP + 1];
	char* mydata = (char*)data;
	size_t len = strlen(mydata) + 1; // ptr to ctrlz ctrlz + why
	char* zzwhy = mydata + len;
	size_t len1 = strlen(zzwhy) + 1;
	char* active = zzwhy + len1 + 1;
	memcpy(extra, zzwhy, HIDDEN_OVERLAP); // preserve any hidden data on why and serverctrlz

	unsigned char* original = data;
	unsigned char* lastBlank = data;
	unsigned char* lastAt = data;
	--data;
	while (*++data)
	{
		if (data > lastAt && (unsigned int)(data - lastAt) > outputLength)
		{
			memmove(lastBlank + 2, lastBlank + 1, strlen((char*)lastBlank));
			*lastBlank++ = '\r'; // legal
			*lastBlank++ = '\n'; // legal
			lastAt = lastBlank;
			++data;
		}
		if (*data == ' ') lastBlank = data;
		else if (*data == '\n') lastAt = lastBlank = data + 1; // internal newlines restart checking
	}
	memcpy(((char*)data) + 1, extra, HIDDEN_OVERLAP);
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
	unsigned char* buffer = (unsigned char*)InfiniteStack(limit, "UTF2ExtendedAscii"); // transient
	unsigned char* bufferto = buffer;
	while (*bufferfrom && (size_t)(bufferto - buffer) < (size_t)(maxBufferSize - 10)) // avoid overflow on local buffer
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
				*bufferto++ = (unsigned char)0xc3;
				*bufferto++ = *(bufferfrom - 1);
			}
		}
	}
	*bufferto = 0;
	if (outputLength) OutputLimit(buffer);
	ReleaseInfiniteStack();
	return (char*)buffer; // it is ONLY good for (*printer) immediate, not for anything long term
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
			if (ptr[1] == '_') memmove(ptr, ptr + 1, strlen(ptr));// convert 2 __ to 1 _  (allows web addressess as output)
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
				if (ptr > start && *(ptr - 1) != '\r') // auto add \r before it
				{
					*ptr = '\r'; // legal
					*++ptr = '\n'; // legal
				}
				else
				{
					*ptr = '\n'; // legal
					memmove(ptr + 1, ptr + 2, strlen(ptr + 2) + 1);
				}
			}
			else if (ptr[1] == 'r')
			{
				*ptr = '\r'; // legal
				memmove(ptr + 1, ptr + 2, strlen(ptr + 2) + 1);
			}
			else if (ptr[1] == 't')
			{
				*ptr = '\t'; // legal
				memmove(ptr + 1, ptr + 2, strlen(ptr + 2) + 1);
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
			if (ptr[1] == '\'' && ((ptr[2] == 's' && !IsAlphaUTF8OrDigit(ptr[3])) || !IsAlphaUTF8OrDigit(ptr[2])))// bob_'s  bobs_'  
			{
				memmove(ptr, ptr + 1, strlen(ptr));
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
		if (c == '~' && IsAlphaUTF8(ptr[1]) && (ptr == output || (*(ptr - 1)) == ' ')) //   REMOVE leading ~  in classnames
		{
			memmove(ptr, ptr + 1, strlen(ptr));
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
	if (!number || !*number) return 0;
	char decimalMark = decimalMarkData[useNumberStyle];
	char digitGroup = digitGroupingData[useNumberStyle];
	char c = *number;
	if (c == '$') { ; }
	else if (c == '#' && IsDigitWord(number + 1, useNumberStyle)) return Convert2Integer(number + 1, useNumberStyle);
	else if (!IsAlphaUTF8DigitNumeric(c) || c == decimalMark) return NOT_A_NUMBER; // not  0-9 letters + - 

	bool csEnglish = !stricmp(current_language, "english");

	size_t len = strlen(number);
	uint64 valx;
	if (IsRomanNumeral(number, valx)) return (int64)valx;
	if (IsDigitWithNumberSuffix(number, useNumberStyle)) // 10K  10M 10B or currency
	{
		char d = number[len - 1];
		number[len - 1] = 0;
		int64 answer = Convert2Integer(number, useNumberStyle);
		if (d == 'k' || d == 'K') answer *= 1000;
		else if (d == 'm' || d == 'M') answer *= 1000000;
		else if (d == 'B' || d == 'b' || d == 'G' || d == 'g') answer *= 1000000000;
		number[len - 1] = d;
		return answer;
	}

	//  grab sign if any
	char sign = *number;
	if (IsSign(sign)) ++number;
	else if (sign == '$')
	{
		sign = 0;
		++number;
	}
	else sign = 0;

	//  make canonical: remove commas (user typed a digit number with commas) and convert _ to -
	char copy[MAX_WORD_SIZE];
	MakeLowerCopy(copy + 1, number);
	*copy = ' '; // safe place to look behind at
	char* comma = copy;
	while (*++comma)
	{
		if (*comma == digitGroup) memmove(comma, comma + 1, strlen(comma));
		else if (*comma == '_') *comma = '-';
	}
	char* word = copy + 1;

	// remove place suffixes
	if (csEnglish && numberValues[languageIndex] && len > 3 && !stricmp(word + len - 3, (char*)"ies")) // twenties?
	{
		char xtra[MAX_WORD_SIZE];
		strcpy(xtra, word);
		strcpy(xtra + len - 3, (char*)"y");
		size_t len1 = strlen(xtra);
		for (unsigned int i = 0; i < 100000; ++i)
		{
			int wordIndex = numberValues[languageIndex][i].word;
			if (!wordIndex) break;
			if (len1 == numberValues[languageIndex][i].length && !strnicmp(xtra, Index2Heap(wordIndex), len1))
			{
				int kind = numberValues[languageIndex][i].realNumber;
				if (kind == FRACTION_NUMBER || kind == REAL_NUMBER)
				{
					strcpy(word, xtra);
					len = len1;
				}
				break;
			}
		}
	}

	if (csEnglish && numberValues[languageIndex] && len > 3 && word[len - 1] == 's') // if s attached to a fraction, remove it
	{
		size_t len1 = len - 1;
		if (word[len1 - 1] == 'e') --len1; // es ending like zeroes
		// look up direct word number as single
		for (unsigned int i = 0; i < 1000; ++i)
		{
			int index = numberValues[languageIndex][i].word;
			if (!index) break;
			if (len1 == numberValues[languageIndex][i].length && !strnicmp(word, Index2Heap(index), len1))
			{
				int kind = numberValues[languageIndex][i].realNumber;
				if (kind == FRACTION_NUMBER || kind == REAL_NUMBER)
				{
					word[len1] = 0; //  thirds to third    quarters to quarter  fifths to fith but not ones to one
					len = len1;
				}
				break;
			}
		}
	}
	unsigned int oldlen = len;
	// remove suffix
	if (!csEnglish);
	else if (len < 3); // cannot have suffix
	else if (word[len - 2] == 's' && word[len - 1] == 't' && !strstr(word, (char*)"first") && IsDigit(word[len - 3])) word[len -= 2] = 0; // 1st
	else if (word[len - 2] == 'n' && word[len - 1] == 'd' && !strstr(word, (char*)"second") && !strstr(word, (char*)"thousand") && IsDigit(word[len - 3])) word[len -= 2] = 0; // 2nd but not second or thousandf"
	else if (word[len - 2] == 'r' && word[len - 1] == 'd' && !strstr(word, (char*)"third") && IsDigit(word[len - 3])) word[len -= 2] = 0; // 3rd
	else if (word[len - 2] == 't' && word[len - 1] == 'h') //  excluding the word "fifth" which is not derived from five
	{
        // keeo exceptions, the word "fifth" which is not derived from five
        if (strstr(word, (char*)"fifth") || strstr(word, (char*)"eighth") || strstr(word, (char*)"ninth") || strstr(word, (char*)"twelfth")) {;}
        else
        {
            word[len -= 2] = 0;
            if (word[len - 1] == 'e' && word[len - 2] == 'i') // twentieth and its ilk
            {
                word[--len - 1] = 'y';
                word[len] = 0;
            }
        }
	}
	if (oldlen != len && (word[len - 1] == '-' || word[len - 1] == '\'')) word[--len] = 0; // trim off separator
	bool hasDigit = IsDigit(*word);
	char* hyp = strchr(word, '-');
	if (hyp) *hyp = 0;
	if (hasDigit) // see if all digits now.
	{
		char* ptr = word - 1;
		while (*++ptr)
		{
			if (ptr != word && *ptr == '-') { ; }
			else if (IsSign(*ptr)) return NOT_A_NUMBER;
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
	if (!hasDigit && numberValues[languageIndex]) for (unsigned int i = 0; i < 1000; ++i)
	{
		int index = numberValues[languageIndex][i].word;
		if (!index) break;
		char* w = Index2Heap(index);
		if (len == numberValues[languageIndex][i].length && !strnicmp(word, w, len))
		{
			return numberValues[languageIndex][i].value;  // a match (but may be a fraction number)
		}
	}

	// try for hyphenated composite
	char* hyphen = strchr(word, '-');
	if (!hyphen) hyphen = strchr(word, '_'); // alternate form of separation
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
	while ((xpiece = strchr(xpiece + 1, '-')))
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
	if (num != -1) // do last piece
	{
		val1 = Convert2Integer(oldhyphen, useNumberStyle);
		if (val1 != NOT_A_NUMBER)
		{
			if ((num % 10) > 0) num *= 10; // don't multiply if twenty-three
			if (val1 > 9) num = -1;
			else num += val1;
			return num;
		}
	}

	if (num >= 0 && num < 10) // simple digit
	{
		val1 = Convert2Integer(oldhyphen, useNumberStyle);
		if (val1 != NOT_A_NUMBER)
		{
			num *= 10;
			if (val1 < 10)
			{
				num += val1;
				return num;
			}
		}
	}

	// decode powers of ten names on 2nd pieces
	long billion = 0;
	char* found = strstr(word + 1, (char*)"billion"); // eight-billion 
	if (found && *(found - 1) == '-') // is 2nd piece
	{
		*(found - 1) = 0; // hide the word billion
		billion = (int)Convert2Integer(word, useNumberStyle);  // determine the number of billions
		if (billion == NOT_A_NUMBER && stricmp(word, (char*)"zero") && *word != '0') return NOT_A_NUMBER;
		word = found + 7; // now points to next part
		if (*word == '-' || *word == '_') ++word; // has another hypen after it
	}
	else if (val == 1000000000)
	{
		val = 0;
		billion = 1;
		if (hyphen) word = hyphen + 1;
	}

	hyphen = strchr(word, '-');
	if (!hyphen) hyphen = strchr(word, '_'); // alternate form of separation
	long million = 0;
	found = strstr(word, (char*)"million");
	if (found && *(found - 1) == '-')
	{
		*(found - 1) = 0;
		million = (int)Convert2Integer(word, useNumberStyle);
		if (million == NOT_A_NUMBER && stricmp(word, (char*)"zero") && *word != '0') return NOT_A_NUMBER;
		word = found + 7;
		if (*word == '-' || *word == '_') ++word; // has another hypen after it
	}
	else if (val == 1000000)
	{
		val = 0;
		million = 1;
		if (hyphen) word = hyphen + 1;
	}

	hyphen = strchr(word, '-');
	if (!hyphen) hyphen = strchr(word, '_'); // alternate form of separation
	long thousand = 0;
	found = strstr(word, (char*)"thousand");
	if (found && *(found - 1) == '-')
	{
		*(found - 1) = 0;
		thousand = (int)Convert2Integer(word, useNumberStyle);
		if (thousand == NOT_A_NUMBER && stricmp(word, (char*)"zero") && *word != '0') return NOT_A_NUMBER;
		word = found + 8;
		if (*word == '-' || *word == '_') ++word; // has another hypen after it
	}
	else if (val == 1000)
	{
		val = 0;
		thousand = 1;
		if (hyphen) word = hyphen + 1;
	}

	hyphen = strchr(word, '-');
	if (!hyphen) hyphen = strchr(word, '_'); // alternate form of separation
	long hundred = 0;
	found = strstr(word, (char*)"hundred");
	if (found && *(found - 1) == '-') // do we have four-hundred
	{
		*(found - 1) = 0;
		hundred = (int)Convert2Integer(word, useNumberStyle);
		if (hundred == NOT_A_NUMBER && stricmp(word, (char*)"zero") && *word != '0') return NOT_A_NUMBER;
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
	bool isFrench = (!stricmp(current_language, "french")) ? true : false;
	hyphen = strchr(word, '-');
	if (!hyphen) hyphen = strchr(word, '_');
	int64 value = 0;
	while (word && *word) // read each smaller part and scale
	{
		if (!hyphen) // last piece (a tens or a ones)
		{
			if (!strcmp(word, number)) return NOT_A_NUMBER;  // never decoded anything so far
			int64 n = Convert2Integer(word, useNumberStyle);
			if (n == NOT_A_NUMBER && stricmp(word, (char*)"zero") && *word != '0') return NOT_A_NUMBER;
			value += n; // handled LAST piece
			break;
		}

		// Find the longest item that is a single number
		// French has composite base numbers, e.g. quatre-vingt-un is 81 not 421, and quatre-vingt-dix-neuf is 99
		int64 piece1 = NOT_A_NUMBER;
		char* hyphen2 = word;
		while ((hyphen2 = strchr(hyphen2, '-')))
		{
			c = *hyphen2;
			*hyphen2 = 0;
			int64 val2 = Convert2Integer(word, useNumberStyle); // convert lead piece to see if its a number
			*hyphen2 = c;
			if (val2 == NOT_A_NUMBER) break;
			hyphen = hyphen2++;
			piece1 = val2;
		}

		*hyphen++ = 0; // split pieces

		// split 3rd piece if one exists
		char* next = strchr(hyphen, '-');
		if (!next) next = strchr(hyphen, '_');
		if (next) *next = 0;

		if (piece1 == NOT_A_NUMBER && stricmp(word, (char*)"zero") && *word != '0') return NOT_A_NUMBER;

		int64 piece2 = Convert2Integer(hyphen, useNumberStyle);
		if (piece2 == NOT_A_NUMBER && stricmp(hyphen, (char*)"0")) return NOT_A_NUMBER;

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

static void LowerUTF8Char(char* ptr)
{
    unsigned char c1 = *ptr;
    unsigned char c2 = ptr[1];
    unsigned char c3 = (strlen(ptr) > 2) ? ptr[2] : 0;
    unsigned char c4 = (strlen(ptr) > 3) ? ptr[3] : 0;

    switch (c1)
    {
        case 0xc3: // Latin 1
            if (c2 >= 0x80 && c2 <= 0x9e && c2 != 0x97) c2 += 0x20;
            break;

        case 0xc4: // Latin ext
            if (c2 >= 0x80 && c2 <= 0xb7 && c2 != 0xb0 && IsEven(c2)) ++c2;
            else if (c2 >= 0xb9 && c2 <= 0xbe && IsOdd(c2)) ++c2;
            else if (c2 == 0xbf)
            {
                c1 = 0xc5;
                c2 = 0x80;
            }
            break;

        case 0xc5: // Latin ext
            if (c2 >= 0x81 && c2 <= 0x88 && IsOdd(c2)) ++c2;
            else if (c2 >= 0x8a && c2 <= 0xb7 && IsEven(c2)) ++c2;
            else if (c2 == 0xb8)
            {
                c1 = 0xc3;
                c2 = 0xbf;
            }
            else if (c2 >= 0xb9 && c2 <= 0xbe && IsOdd(c2)) ++c2;
            break;

        case 0xc6: // Latin ext
            switch (c2)
            {
                case 0x81:
                    c1 = 0xc9;
                    c2 = 0x93;
                    break;
                case 0x86:
                    c1 = 0xc9;
                    c2 = 0x94;
                    break;
                case 0x89:
                    c1 = 0xc9;
                    c2 = 0x96;
                    break;
                case 0x8a:
                    c1 = 0xc9;
                    c2 = 0x97;
                    break;
                case 0x8e:
                    c1 = 0xc9;
                    c2 = 0x98;
                    break;
                case 0x8f:
                    c1 = 0xc9;
                    c2 = 0x99;
                    break;
                case 0x90:
                    c1 = 0xc9;
                    c2 = 0x9b;
                    break;
                case 0x93:
                    c1 = 0xc9;
                    c2 = 0xa0;
                    break;
                case 0x94:
                    c1 = 0xc9;
                    c2 = 0xa3;
                    break;
                case 0x96:
                    c1 = 0xc9;
                    c2 = 0xa9;
                    break;
                case 0x97:
                    c1 = 0xc9;
                    c2 = 0xa8;
                    break;
                case 0x9c:
                    c1 = 0xc9;
                    c2 = 0xaf;
                    break;
                case 0x9d:
                    c1 = 0xc9;
                    c2 = 0xb2;
                    break;
                case 0x9f:
                    c1 = 0xc9;
                    c2 = 0xb5;
                    break;
                case 0xa9:
                    c1 = 0xca;
                    c2 = 0x83;
                    break;
                case 0xae:
                    c1 = 0xca;
                    c2 = 0x88;
                    break;
                case 0xb1:
                    c1 = 0xca;
                    c2 = 0x8a;
                    break;
                case 0xb2:
                    c1 = 0xca;
                    c2 = 0x8b;
                    break;
                case 0xb7:
                    c1 = 0xca;
                    c2 = 0x92;
                    break;
                case 0x82:
                case 0x84:
                case 0x87:
                case 0x8b:
                case 0x91:
                case 0x98:
                case 0xa0:
                case 0xa2:
                case 0xa4:
                case 0xa7:
                case 0xac:
                case 0xaf:
                case 0xb3:
                case 0xb5:
                case 0xb8:
                case 0xbc:
                    c2++;
                    break;
                default:
                    break;
            }
            break;

        case 0xc7: // Latin ext
            if (c2 == 0x84) c2 = 0x86;
            else if (c2 == 0x85) c2++;
            else if (c2 == 0x87) c2 = 0x89;
            else if (c2 == 0x88) c2++;
            else if (c2 == 0x8a) c2 = 0x8c;
            else if (c2 == 0x8b) c2++;
            else if (c2 >= 0x8d && c2 <= 0x9c && IsOdd(c2)) c2++;
            else if (c2 >= 0x9e && c2 <= 0xaf && IsEven(c2)) c2++;
            else if (c2 == 0xb1) c2 = 0xb3;
            else if (c2 == 0xb2) c2++;
            else if (c2 == 0xb4) c2++;
            else if (c2 == 0xb6)
            {
                c1 = 0xc6;
                c2 = 0x95;
            }
            else if (c2 == 0xb7)
            {
                c1 = 0xc6;
                c2 = 0xbf;
            }
            else if (c2 >= 0xb8 && c2 <= 0xbf && IsEven(c2)) c2++;
            break;

        case 0xc8: // Latin ext
            if (c2 >= 0x80 && c2 <= 0x9f && IsEven(c2)) c2++;
            else if (c2 == 0xa0)
            {
                c1 = 0xc6;
                c2 = 0x9e;
            }
            else if (c2 >= 0xa2 && c2 <= 0xb3 && IsEven(c2)) c2++;
            else if (c2 == 0xbb) c2++;
            else if (c2 == 0xbd)
            {
                c1 = 0xc6;
                c2 = 0x9a;
            }
            break;

        case 0xc9: // Latin ext
            if (c2 == 0x81) c2++;
            else if (c2 == 0x83)
            {
                c1 = 0xc6;
                c2 = 0x80;
            }
            else if (c2 == 0x84)
            {
                c1 = 0xca;
                c2 = 0x89;
            }
            else if (c2 == 0x85)
            {
                c1 = 0xca;
                c2 = 0x8c;
            }
            else if (c2 >= 0x86 && c2 <= 0x8f && IsEven(c2)) c2++;
            break;

        case 0xcd: // Greek & Coptic
            switch (c2)
            {
                case 0xb0:
                case 0xb2:
                case 0xb6:
                    c2++;
                    break;
                case 0xbf:
                    c1 = 0xcf;
                    c2 = 0xb3;
                    break;
                default:
                    break;
            }
            break;
            
        case 0xce: // Greek & Coptic
            if (c2 == 0x86) c2 = 0xac;
            else if (c2 == 0x88) c2 = 0xad;
            else if (c2 == 0x89) c2 = 0xae;
            else if (c2 == 0x8a) c2 = 0xaf;
            else if (c2 == 0x8c)
            {
                c1 = 0xcf;
                c2 = 0x8c;
            }
            else if (c2 == 0x8e)
            {
                c1 = 0xcf;
                c2 = 0x8d;
            }
            else if (c2 == 0x8f)
            {
                c1 = 0xcf;
                c2 = 0x8e;
            }
            else if (c2 >= 0x91 && c2 <= 0x9f) c2 += 0x20;
            else if (c2 >= 0xa0 && c2 <= 0xab && c2 != 0xa2)
            {
                c1 = 0xcf;
                c2 -= 0x20;
            }
            break;
            
        case 0xcf: // Greek & Coptic
            if (c2 == 0x8f) c2 = 0x97;
            else if (c2 >= 0x98 && c2 <= 0xaf && IsEven(c2)) c2++;
            else if (c2 == 0xb4) c2 = 0x91;
            else if (c2 == 0xb7) c2++;
            else if (c2 == 0xb9) c2 = 0xb2;
            else if (c2 == 0xba) c2++;
            else if (c2 == 0xbd)
            {
                c1 = 0xcd;
                c2 = 0xbb;
            }
            else if (c2 == 0xbe)
            {
                c1 = 0xcd;
                c2 = 0xbc;
            }
            else if (c2 == 0xbf)
            {
                c1 = 0xcd;
                c2 = 0xbd;
            }
            break;
            
        case 0xd0: // Cyrillic
            if (c2 >= 0x80 && c2 <= 0x8f)
            {
                c1 = 0xd1;
                c2 += 0x10;
            }
            else if (c2 >= 0x90 && c2 <= 0x9f) c2 += 0x20;
            else if (c2 >= 0xa0 && c2 <= 0xaf)
            {
                c1 = 0xd1;
                c2 -= 0x20;
            }
            break;
            
        case 0xd1: // Cyrillic supplement
            if (c2 >= 0xa0 && c2 <= 0xbf && IsEven(c2)) c2++;
            break;
            
        case 0xd2: // Cyrillic supplement
            if (c2 == 0x80) c2++;
            else if (c2 >= 0x8a && c2 <= 0xbf && IsEven(c2)) c2++;
            break;
            
        case 0xd3: // Cyrillic supplement
            if (c2 == 0x80) c2 = 0x8f;
            else if (c2 >= 0x81 && c2 <= 0x8e && IsOdd(c2)) c2++;
            else if (c2 >= 0x90 && c2 <= 0xbf && IsEven(c2)) c2++;
            break;
            
        case 0xd4: // Cyrillic supplement & Armenian
            if (c2 >= 0x80 && c2 <= 0xaf && IsEven(c2)) c2++;
            else if (c2 >= 0xb1 && c2 <= 0xbf)
            {
                c1 = 0xd5;
                c2 -= 0x10;
            }
            break;
            
        case 0xd5: // Armenian
            if (c2 >= 0x80 && c2 <= 0x8f) c2 += 0x30;
            else if (c2 >= 0x90 && c2 <= 0x96)
            {
                c1 = 0xd6;
                c2 -= 0x10;
            }
            break;

        case 0xe1: // Three byte code
            switch (c2)
            {
                case 0x82: // Georgian asomtavruli
                    if (c3 >= 0xa0 && c3 <= 0xbf)
                    {
                        c2 = 0x83;
                        c3 -= 0x10;
                    }
                    break;
                case 0x83: // Georgian asomtavruli
                    if ((c3 >= 0x80 && c3 <= 0x85) || c3 == 0x87 || c3 == 0x8d) c3 += 0x30;
                    break;
                case 0x8e: // Cherokee
                    if (c3 >= 0xa0 && c3 <= 0xaf)
                    {
                        c1 = 0xea;
                        c2 = 0xad;
                        c3 += 0x10;
                    }
                    else if (c3 >= 0xb0 && c3 <= 0xbf)
                    {
                        c1 = 0xea;
                        c2 = 0xae;
                        c3 -= 0x30;
                    }
                    break;
                case 0x8f: // Cherokee
                    if (c3 >= 0x80 && c3 <= 0xaf)
                    {
                        c1 = 0xea;
                        c2 = 0xae;
                        c3 += 0x10;
                    }
                    else if (c3 >= 0xb0 && c3 <= 0xb5) {
                        c3 += 0x08;
                    }
                    break;
                case 0xb2: // Georgian mtavruli
                    if ((c3 >= 0x90 && c3 <= 0xba) || c3 == 0xbd || c3 == 0xbe || c3 == 0xbf) c2 = 0x83;
                    break;
                case 0xb8: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0xbf && IsEven(c3)) c3++;
                    break;
                case 0xb9: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0xbf && IsEven(c3)) c3++;
                    break;
                case 0xba: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0x94 && IsEven(c3)) c3++;
                    else if (c3 == 0x9e) // German capital sharp S
                    {
                        c1 = 0xc3;
                        c2 = 0x9f;
                        c3 = 0;
                    }
                    else if (c3 >= 0xa0 && c3 <= 0xbf && IsEven(c3)) c3++;
                    break;
                case 0xbb: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0xbf && IsEven(c3)) c3++;
                    break;
                case 0xbc: // Greek ex
                    if (c3 >= 0x88 && c3 <= 0x8f) c3 -= 0x08;
                    else if (c3 >= 0x98 && c3 <= 0x9d) c3 -= 0x08;
                    else if (c3 >= 0xa8 && c3 <= 0xaf) c3 -= 0x08;
                    else if (c3 >= 0xb8 && c3 <= 0xbf) c3 -= 0x08;
                    break;
                case 0xbd: // Greek ex
                    if (c3 >= 0x88 && c3 <= 0x8d) c3 -= 0x08;
                    else if (c3 == 0x99 || c3 == 0x9b || c3 == 0x9d || c3 == 0x9f) c3 -= 0x08;
                    else if (c3 >= 0xa8 && c3 <= 0xaf) c3 -= 0x08;
                    break;
                case 0xbe: // Greek ex
                    if (c3 >= 0x88 && c3 <= 0x8f) c3 -= 0x08;
                    else if (c3 >= 0x98 && c3 <= 0x9f) c3 -= 0x08;
                    else if (c3 >= 0xa8 && c3 <= 0xaf) c3 -= 0x08;
                    else if (c3 >= 0xb8 && c3 <= 0xb9) c3 -= 0x08;
                    else if (c3 >= 0xba && c3 <= 0xbb)
                    {
                        c2 = 0xbd;
                        c3 -= 0x0a;
                    }
                    else if (c3 == 0xbc) c3 -= 0x09;
                    break;
                case 0xbf: // Greek ex
                    if (c3 >= 0x88 && c3 <= 0x8b)
                    {
                        c2 = 0xbd;
                        c3 += 0x2a;
                    }
                    else if (c3 == 0x8c) c3 -= 0x09;
                    else if (c3 >= 0x98 && c3 <= 0x99) c3 -= 0x08;
                    else if (c3 >= 0x9a && c3 <= 0x9b)
                    {
                        c2 = 0xbd;
                        c3 += 0x1c;
                    }
                    else if (c3 >= 0xa8 && c3 <= 0xa9) c3 -= 0x08;
                    else if (c3 >= 0xaa && c3 <= 0xab)
                    {
                        c2 = 0xbd;
                        c3 += 0x10;
                    }
                    else if (c3 == 0xac) c3 -= 0x07;
                    else if (c3 >= 0xb8 && c3 <= 0xb9) c2 = 0xbd;
                    else if (c3 >= 0xba && c3 <= 0xbb)
                    {
                        c2 = 0xbd;
                        c3 += 0x02;
                    }
                    else if (c3 == 0xbc) c3 -= 0x09;
                    break;
                default:
                    break;
            }
            break;
            
        case 0xe2: // Three byte code
            switch (c2)
            {
                case 0xb0: // Glagolitic
                    if (c3 >= 0x80 && c3 <= 0x8f) c3 += 0x30;
                    else if (c3 >= 0x90 && c3 <= 0xae)
                    {
                        c2 = 0xb1;
                        c3 -= 0x10;
                    }
                    break;
                case 0xb1: // Latin ext
                    switch (c3)
                    {
                        case 0xa0:
                        case 0xa7:
                        case 0xa9:
                        case 0xab:
                        case 0xb2:
                        case 0xb5:
                            c3++;
                            break;
                        case 0xa2:
                        case 0xa4:
                        case 0xad:
                        case 0xae:
                        case 0xaf:
                        case 0xb0:
                        case 0xbe:
                        case 0xbf:
                            break;
                        case 0xa3:
                            c3 = 0xe1;
                            c2 = 0xb5;
                            c3 = 0xbd;
                            break;
                        default:
                            break;
                    }
                    break;
                case 0xb2: // Coptic
                    if (c3 >= 0x80 && c3 <= 0xbf && IsEven(c3)) c3++;
                    break;
                case 0xb3: // Coptic
                    if ((c3 >= 0x80 && c3 <= 0xa3 && IsEven(c2)) || c3 == 0xab || c3 == 0xad || c3 == 0xb2) c3++;
                    break;
                case 0xb4: // Georgian nuskhuri
                    if ((c3 >= 0x80 && c3 <= 0xa5) || c3 == 0xa7 || c3 == 0xad)
                    {
                        c1 = 0xe1;
                        c2 = 0x83;
                        c3 += 0x10;
                    }
                    break;
                default:
                    break;
            }
            break;
            
        case 0xea: // Three byte code
            switch (c2)
            {
                case 0x99: // Cyrillic
                    if (c3 >= 0x80 && c3 <= 0xad && IsEven(c3)) c3++;
                    break;
                case 0x9a: // Cyrillic
                    if (c3 >= 0x80 && c3 <= 0x9b && IsEven(c3)) c3++;
                    break;
                case 0x9c: // Latin ext
                    if (((c3 >= 0xa2 && c3 <= 0xaf) || (c3 >= 0xb2 && c3 <= 0xbf)) && IsEven(c3)) c3++;
                    break;
                case 0x9d: // Latin ext
                    if ((c3 >= 0x80 && c3 <= 0xaf && IsEven(c3)) || c3 == 0xb9 || c3 == 0xbb || c3 == 0xbe) c3++;
                    else if (c3 == 0xbd)
                    {
                        c1 = 0xe1;
                        c2 = 0xb5;
                        c3 = 0xb9;
                    }
                    break;
                case 0x9e: // Latin ext
                    if ((((c3 >= 0x80 && c3 <= 0x87) || (c3 >= 0x96 && c3 <= 0xa9) || (c3 >= 0xb4 && c3 <= 0xbf)) && IsEven(c3)) || c3 == 0x8b || c3 == 0x90 || c3 == 0x92) c3++;
                    else if (c3 == 0xb3)
                    {
                        c1 = 0xea;
                        c2 = 0xad;
                        c3 = 0x93;
                    }
                    break;
                case 0x9f: // Latin ext
                    if (c3 == 0x82 || c3 == 0x87 || c3 == 0x89 || c3 == 0xb5) c3++;
                    else if (c3 == 0x84)
                    {
                        c1 = 0xea;
                        c2 = 0x9e;
                        c3 = 0x94;
                    }
                    else if (c3 == 0x86)
                    {
                        c1 = 0xe1;
                        c2 = 0xb6;
                        c3 = 0x8e;
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0xef: // Three byte code
            switch (c2)
            {
                case 0xbc: // Latin fullwidth
                    if (c3 >= 0xa1 && c3 <= 0xba)
                    {
                        c2 = 0xbd;
                        c3 -= 0x20;
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0xf0: // Four byte code
            break;
            
        default:
            break;
    }
    
    *ptr++ = c1;
    *ptr++ = c2;
    if (c3) *ptr++ = c3;
    if (c4) *ptr++ = c4;
    *ptr = 0;
}

static void UpperUTF8Char(char* ptr)
{
    unsigned char c1 = *ptr;
    unsigned char c2 = ptr[1];
    unsigned char c3 = (strlen(ptr) > 2) ? ptr[2] : 0;
    unsigned char c4 = (strlen(ptr) > 3) ? ptr[3] : 0;

    switch (c1)
    {
        case 0xc3: // Latin 1
            if (c2 == 0x9f) // German sharp S
            {
                c1 = 0xe1;
                c2 = 0xba;
                c3 = 0x9e;
            }
            else if (c2 >= 0xa0 && c2 <= 0xbe && c2 != 0xb7) c2 -= 0x20;
            else if (c2 == 0xbf)
            {
                c1 = 0xc5;
                c2 = 0xb8;
            }
            break;

        case 0xc4: // Latin ext
            if (c2 >= 0x80 && c2 <= 0xb7 && c2 != 0xb1 && IsOdd(c2)) c2--;
            else if (c2 >= 0xb9 && c2 <= 0xbe && IsEven(c2)) c2--;
            break;

        case 0xc5: // Latin ext
            if (c2 == 0x80)
            {
                c1 = 0xc4;
                c2 = 0xbf;
            }
            else if (c2 >= 0x81 && c2 <= 0x88 && IsEven(c2)) c2--;
            else if (c2 >= 0x8a && c2 <= 0xb7 && IsOdd(c2)) c2--;
            else if (c2 == 0xb8)
            {
                c1 = 0xc5;
                c2 = 0xb8;
            }
            else if (c2 >= 0xb9 && c2 <= 0xbe && IsEven(c2)) c2--;
            break;

        case 0xc6: // Latin ext
            switch (c2)
            {
                case 0x83:
                case 0x85:
                case 0x88:
                case 0x8c:
                case 0x92:
                case 0x99:
                case 0xa1:
                case 0xa3:
                case 0xa5:
                case 0xa8:
                case 0xad:
                case 0xb0:
                case 0xb4:
                case 0xb6:
                case 0xb9:
                case 0xbd:
                    c2--;
                    break;
                case 0x80:
                    c1 = 0xc9;
                    c2 = 0x83;
                    break;
                case 0x95:
                    c1 = 0xc7;
                    c2 = 0xb6;
                    break;
                case 0x9a:
                    c1 = 0xc8;
                    c2 = 0xbd;
                    break;
                case 0x9e:
                    c1 = 0xc8;
                    c2 = 0xa0;
                    break;
                case 0xbf:
                    c1 = 0xc7;
                    c2 = 0xb7;
                    break;
                default:
                    break;
            }
            break;

        case 0xc7: // Latin ext
            if (c2 == 0x85) c2--;
            else if (c2 == 0x86) c2 = 0x84;
            else if (c2 == 0x88) c2--;
            else if (c2 == 0x89) c2 = 0x87;
            else if (c2 == 0x8b) c2--;
            else if (c2 == 0x8c) c2 = 0x8a;
            else if (c2 >= 0x8d && c2 <= 0x9c && IsEven(c2)) c2--;
            else if (c2 >= 0x9e && c2 <= 0xaf && IsOdd(c2)) c2--;
            else if (c2 == 0xb2) c2--;
            else if (c2 == 0xb3) c2 = 0xb1;
            else if (c2 == 0xb5) c2--;
            else if (c2 >= 0xb9 && c2 <= 0xbf && IsOdd(c2)) c2--;
            break;

        case 0xc8: // Latin ext
            if (c2 >= 0x80 && c2 <= 0x9f && IsOdd(c2)) c2--;
            else if (c2 >= 0xa2 && c2 <= 0xb3 && IsOdd(c2)) c2--;
            else if (c2 == 0xbc) c2--;
            break;

        case 0xc9: // Latin ext
            switch (c2)
            {
                case 0x80:
                case 0x90:
                case 0x91:
                case 0x92:
                case 0x9c:
                case 0xa1:
                case 0xa5:
                case 0xa6:
                case 0xab:
                case 0xac:
                case 0xb1:
                case 0xbd:
                    break;
                case 0x82:
                    c2--;
                    break;
                case 0x93:
                    c1 = 0xc6;
                    c2 = 0x81;
                    break;
                case 0x94:
                    c1 = 0xc6;
                    c2 = 0x86;
                    break;
                case 0x96:
                    c1 = 0xc6;
                    c2 = 0x89;
                    break;
                case 0x97:
                    c1 = 0xc6;
                    c2 = 0x8a;
                    break;
                case 0x98:
                    c1 = 0xc6;
                    c2 = 0x8e;
                    break;
                case 0x99:
                    c1 = 0xc6;
                    c2 = 0x8f;
                    break;
                case 0x9b:
                    c1 = 0xc6;
                    c2 = 0x90;
                    break;
                case 0xa0:
                    c1 = 0xc6;
                    c2 = 0x93;
                    break;
                case 0xa3:
                    c1 = 0xc6;
                    c2 = 0x94;
                    break;
                case 0xa8:
                    c1 = 0xc6;
                    c2 = 0x97;
                    break;
                case 0xa9:
                    c1 = 0xc6;
                    c2 = 0x96;
                    break;
                case 0xaf:
                    c1 = 0xc6;
                    c2 = 0x9c;
                    break;
                case 0xb2:
                    c1 = 0xc6;
                    c2 = 0x9d;
                    break;
                case 0xb5:
                    c1 = 0xc6;
                    c2 = 0x9f;
                    break;
                default:
                    if (c2 >= 0x87 && c2 <= 0x8f && IsOdd(c2)) c2--;
                    break;
            }
            break;

        case 0xca: // Latin ext
            switch (c2)
            {
                case 0x82:
                case 0x87:
                case 0x9d:
                case 0x9e:
                    break;
                case 0x83:
                    c1 = 0xc6;
                    c2 = 0xa9;
                    break;
                case 0x88:
                    c1 = 0xc6;
                    c2 = 0xae;
                    break;
                case 0x89:
                    c1 = 0xc9;
                    c2 = 0x84;
                    break;
                case 0x8a:
                    c1 = 0xc6;
                    c2 = 0xb1;
                    break;
                case 0x8b:
                    c1 = 0xc6;
                    c2 = 0xb2;
                    break;
                case 0x8c:
                    c1 = 0xc9;
                    c2 = 0x85;
                    break;
                case 0x92:
                    c1 = 0xc6;
                    c2 = 0xb7;
                    break;
                default:
                    break;
            }
            break;

        case 0xcd: // Greek & Coptic
            switch (c2)
            {
                case 0xb1:
                case 0xb3:
                case 0xb7:
                    c2--;
                    break;
                case 0xbb:
                    c1 = 0xcf;
                    c2 = 0xbd;
                    break;
                case 0xbc:
                    c1 = 0xcf;
                    c2 = 0xbe;
                    break;
                case 0xbd:
                    c1 = 0xcf;
                    c2 = 0xbf;
                    break;
                default:
                    break;
            }
            break;

        case 0xce: // Greek & Coptic
            if (c2 == 0xac) c2 = 0x86;
            else if (c2 == 0xad) c2 = 0x88;
            else if (c2 == 0xae) c2 = 0x89;
            else if (c2 == 0xaf) c2 = 0x8a;
            else if (c2 >= 0xb1 && c2 <= 0xbf) c2 -= 0x20;
            break;

        case 0xcf: // Greek & Coptic
            if (c2 == 0x82)
            {
                c1 = 0xce;
                c2 = 0xa3;
            }
            else if (c2 >= 0x80 && c2 <= 0x8b)
            {
                c1 = 0xce;
                c2 += 0x20;
            }
            else if (c2 == 0x8c)
            {
                c1 = 0xce;
                c2 = 0x8c;
            }
            else if (c2 == 0x8d)
            {
                c1 = 0xce;
                c2 = 0x8e;
            }
            else if (c2 == 0x8e)
            {
                c1 = 0xce;
                c2 = 0x8f;
            }
            else if (c2 == 0x91) c2 = 0xb4;
            else if (c2 == 0x97) c2 = 0x8f;
            else if (c2 >= 0x98 && c2 <= 0xaf && IsOdd(c2)) c2--;
            else if (c2 == 0xb2) c2 = 0xb9;
            else if (c2 == 0xb3)
            {
                c1 = 0xcd;
                c2 = 0xbf;
            }
            else if (c2 == 0xb8) c2--;
            else if (c2 == 0xbb) c2--;
            break;

        case 0xd0: // Cyrillic
            if (c2 >= 0xb0 && c2 <= 0xbf) c2 -= 0x20;
            break;

        case 0xd1: // Cyrillic supplement
            if (c2 >= 0x80 && c2 <= 0x8f)
            {
                c1 = 0xd0;
                c2 += 0x20;
            }
            else if (c2 >= 0x90 && c2 <= 0x9f)
            {
                c1 = 0xd0;
                c2 -= 0x10;
            }
            else if (c2 >= 0xa0 && c2 <= 0xbf && IsOdd(c2)) c2--;
            break;

        case 0xd2: // Cyrillic supplement
            if (c2 == 0x81) c2--;
            else if (c2 >= 0x8a && c2 <= 0xbf && IsOdd(c2)) c2--;
            break;

        case 0xd3: // Cyrillic supplement
            if (c2 >= 0x81 && c2 <= 0x8e && IsEven(c2)) c2--;
            else if (c2 == 0x8f) c2 = 0x80;
            else if (c2 >= 0x90 && c2 <= 0xbf && IsOdd(c2)) c2--;
            break;

        case 0xd4: // Cyrillic supplement & Armenian
            if (c2 >= 0x80 && c2 <= 0xaf && IsOdd(c2)) c2--;
            break;

        case 0xd5: // Armenian
            if (c2 >= 0xa1 && c2 <= 0xaf)
            {
                c1 = 0xd4;
                c2 += 0x10;
            }
            else if (c2 >= 0xb0 && c2 <= 0xbf) c2 -= 0x30;
            break;

        case 0xd6: // Armenian
            if (c2 >= 0x80 && c2 <= 0x86)
            {
                c1 = 0xd5;
                c2 += 0x10;
            }
            break;

        case 0xe1: // Three byte code
            switch (c2)
            {
                case 0x82: // Georgian Asomtavruli
                    if (c3 >= 0xa0 && c3 <= 0xbf)
                    {
                        c2 = 0xb2;
                        c3 -= 0x10;
                    }
                    break;
                case 0x83:
                    // Georgian Asomtavruli
                    if ((c3 >= 0x80 && c3 <= 0x85) || c3 == 0x87 || c3 == 0x8d)
                    {
                        c2 = 0xb2;
                        c3 += 0x30;
                    }
                    // Georgian mkhedruli
                    else if ((c3 >= 0x90 && c3 <= 0xba) || c3 == 0xbd || c3 == 0xbe || c3 == 0xbf)
                    {
                        c2 = 0xb2;
                    }
                    break;
                case 0x8f: // Cherokee
                    if (c3 >= 0xb8 && c3 <= 0xbd)
                    {
                        c3 -= 0x08;
                    }
                    break;
                case 0xb5: // Latin ext
                    if (c3 == 0xb9)
                    {
                        c1 = 0xea;
                        c2 = 0x9d;
                        c3 = 0xbd;
                    }
                    else if (c3 == 0xbd)
                    {
                        c1 = 0xe2;
                        c2 = 0xb1;
                        c3 = 0xa3;
                    }
                    break;
                case 0xb6: // Latin ext
                    if (c3 == 0x8e)
                    {
                        c1 = 0xea;
                        c2 = 0x9f;
                        c3 = 0x86;
                    }
                    break;
                case 0xb8: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0xbf && IsOdd(c3)) c3--;
                    break;
                case 0xb9: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0xbf && IsOdd(c3)) c3--;
                    break;
                case 0xba: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0x95 && IsOdd(c3)) c3--;
                    else if (c3 >= 0xa0 && c3 <= 0xbf && IsOdd(c3)) c3--;
                    break;
                case 0xbb: // Latin ext
                    if (c3 >= 0x80 && c3 <= 0xbf && IsOdd(c3)) c3--;
                    break;
                case 0xbc: // Greek ext
                    if (c3 >= 0x80 && c3 <= 0x87) c3 += 0x08;
                    else if (c3 >= 0x90 && c3 <= 0x95) c3 += 0x08;
                    else if (c3 >= 0xa0 && c3 <= 0xa7) c3 += 0x08;
                    else if (c3 >= 0xb0 && c3 <= 0xb7) c3 += 0x08;
                    break;
                case 0xbd: // Greek ext
                    if (c3 >= 0x80 && c3 <= 0x85) c3 += 0x08;
                    else if (c3 == 0x91 || c3 == 0x93 || c3 == 0x95 || c3 == 0x97) c3 += 0x08;
                    else if (c3 >= 0xa0 && c3 <= 0xa7) c3 += 0x08;
                    else if (c3 >= 0xb0 && c3 <= 0xb1)
                    {
                        c2 = 0xbe;
                        c3 += 0x0a;
                    }
                    else if (c3 >= 0xb2 && c3 <= 0xb5)
                    {
                        c2 = 0xbf;
                        c3 -= 0x2a;
                    }
                    else if (c3 >= 0xb6 && c3 <= 0xb7)
                    {
                        c2 = 0xbf;
                        c3 -= 0x1c;
                    }
                    else if (c3 >= 0xb8 && c3 <= 0xb9)
                    {
                        c2 = 0xbf;
                    }
                    else if (c3 >= 0xba && c3 <= 0xbb)
                    {
                        c2 = 0xbf;
                        c3 -= 0x10;
                    }
                    else if (c3 >= 0xbc && c3 <= 0xbd)
                    {
                        c2 = 0xbf;
                        c3 -= 0x02;
                    }
                    break;
                case 0xbe: // Greek ext
                    if (c3 >= 0x80 && c3 <= 0x87) c3 += 0x08;
                    else if (c3 >= 0x90 && c3 <= 0x97) c3 += 0x08;
                    else if (c3 >= 0xa0 && c3 <= 0xa7) c3 += 0x08;
                    else if (c3 >= 0xb0 && c3 <= 0xb1) c3 += 0x08;
                    else if (c3 == 0xb3) c3 += 0x09;
                    break;
                case 0xbf: // Greek ext
                    if (c3 == 0x83) c3 += 0x09;
                    else if (c3 >= 0x90 && c3 <= 0x91) c3 += 0x08;
                    else if (c3 >= 0xa0 && c3 <= 0xa1) c3 += 0x08;
                    else if (c3 == 0xa5) c3 += 0x07;
                    else if (c3 == 0xb3) c3 += 0x09;
                    break;
                default:
                    break;
            }
            break;

        case 0xe2: // Three byte code
            switch (c2)
            {
                case 0xb0: // Glagolitic
                    if (c3 >= 0xb0 && c3 <= 0xbf) c3 -= 0x30;
                    break;
                case 0xb1: // Glagolitic
                    if (c3 >= 0x80 && c3 <= 0x9e)
                    {
                        c2 = 0xb0;
                        c3 += 0x10;
                    }
                    else
                    { // Latin ext
                        switch (c3)
                        {
                            case 0xa1:
                            case 0xa8:
                            case 0xaa:
                            case 0xac:
                            case 0xb3:
                            case 0xb6:
                                c3--;
                                break;
                            case 0xa5:
                            case 0xa6:
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                case 0xb2: // Coptic
                    if (c3 >= 0x80 && c3 <= 0xbf && IsOdd(c3)) c3--;
                    break;
                case 0xb3: // Coptic
                    if ((c3 >= 0x80 && c3 <= 0xa3 && IsOdd(c3)) || c3 == 0xac || c3 == 0xae || c3 == 0xb3) c3--;
                    break;
                case 0xb4: // Georgian
                    if ((c3 >= 0x80 && c3 <= 0xa5) || c3 == 0xa7 || c3 == 0xad)
                    {
                        c1 = 0xe1;
                        c2 = 0xb2;
                        c3 += 0x10;
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0xea: // Three byte code
            switch (c2)
            {
                case 0x99: // Cyrillic
                    if (c3 >= 0x80 && c3 <= 0xad && IsOdd(c3)) c3--;
                    break;
                case 0x9a: // Cyrillic
                    if (c3 >= 0x80 && c3 <= 0x9b && IsOdd(c3)) c3--;
                    break;
                case 0x9c: // Latin ext
                    if (((c3 >= 0xa2 && c3 <= 0xaf) || (c3 >= 0xb2 && c3 <= 0xbf)) && IsOdd(c3)) c3--;
                    break;
                case 0x9d: // Latin ext
                    if ((c3 >= 0x80 && c3 <= 0xaf && IsOdd(c3)) || c3 == 0xba || c3 == 0xbc || c3 == 0xbf) c3--;
                    break;
                case 0x9e: // Latin ext
                    if ((((c3 >= 0x80 && c3 <= 0x87) || (c3 >= 0x96 && c3 <= 0xa9) || (c3 >= 0xb4 && c3 <= 0xbf)) && IsOdd(c3)) || c3 == 0x8c || c3 == 0x91 || c3 == 0x93) c3--;
                    else if (c3 == 0x94)
                    {
                        c1 = 0xea;
                        c2 = 0x9f;
                        c3 = 0x84;
                    }
                    break;
                case 0x9f: // Latin ext
                    if (c3 == 0x83 || c3 == 0x88 || c3 == 0x8a || c3 == 0xb6) c3--;
                    break;
                case 0xad:
                    // Latin ext
                    if (c3 == 0x93)
                    {
                        c2 = 0x9e;
                        c3 = 0xb3;
                    }
                    // Cherokee
                    else if (c3 >= 0xb0 && c3 <= 0xbf)
                    {
                        c1 = 0xe1;
                        c2 = 0x8e;
                        c3 -= 0x10;
                    }
                    break;
                case 0xae: // Cherokee
                    if (c3 >= 0x80 && c3 <= 0x8f)
                    {
                        c1 = 0xe1;
                        c2 = 0x8e;
                        c3 += 0x30;
                    }
                    else if (c3 >= 0x90 && c3 <= 0xbf)
                    {
                        c1 = 0xe1;
                        c2 = 0x8f;
                        c3 -= 0x10;
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0xef: // Three byte code
            switch (c2)
            {
                case 0xbd: // Latin fullwidth
                    if (c3 >= 0x81 && c3 <= 0x9a)
                    {
                        c2 = 0xbc;
                        c3 += 0x20;
                    }
                    break;
                default:
                    break;
            }
            break;

        case 0xf0: // Four byte code
            break;
            
        default:
            break;
    }

    *ptr++ = c1;
    *ptr++ = c2;
    if (c3) *ptr++ = c3;
    if (c4) *ptr++ = c4;
    *ptr = 0;
}

void MakeLowerCase(char* ptr)
{
	--ptr;
	while (*++ptr)
	{
		char utfcharacter[10];
		char* x = IsUTF8(ptr, utfcharacter); // return after this character if it is valid.
        if (utfcharacter[1])
        {
            size_t oldLen = strlen(utfcharacter);
            LowerUTF8Char(utfcharacter);
            size_t newLen = strlen(utfcharacter);
            if (newLen > oldLen) memmove(ptr+newLen,ptr+oldLen,strlen(ptr+oldLen)+1);
            strncpy(ptr, utfcharacter, newLen);
            ptr += newLen;
            if (newLen < oldLen) memmove(ptr,ptr+(oldLen-newLen),strlen(ptr+(oldLen-newLen))+1);
            --ptr;
        }
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
        if (utfcharacter[1])
        {
            size_t oldLen = strlen(utfcharacter);
            UpperUTF8Char(utfcharacter);
            size_t newLen = strlen(utfcharacter);
            if (newLen > oldLen) memmove(ptr+newLen,ptr+oldLen,strlen(ptr+oldLen)+1);
            strncpy(ptr, utfcharacter, newLen);
            ptr += newLen;
            if (newLen < oldLen) memmove(ptr,ptr+(oldLen-newLen),strlen(ptr+(oldLen-newLen))+1);
            --ptr;
        }
		else  *ptr = GetUppercaseData(*ptr);
	}
}

char* PartialLowerCopy(char* to, const char* from, int begin, int end)  	//excludes the part from start to end from being converted to lower case
{
	char* start = to;
	char tempCopy[MAX_WORD_SIZE], tempCopyFirst[MAX_WORD_SIZE], tempCopyLast[MAX_WORD_SIZE], tempLowerFirst[MAX_WORD_SIZE], tempLowerLast[MAX_WORD_SIZE];
	strncpy(tempCopyFirst, from, begin);
	strncpy(tempCopy, from + begin, end);
	strncpy(tempCopyLast, from + end, strlen(from) - 1);
	tempCopyFirst[begin] = '\0';
	tempCopy[end - begin + 1] = '\0';
	tempCopyLast[strlen(from) - end] = '\0';
	MakeLowerCopy(tempLowerFirst, tempCopyFirst);
	MakeLowerCopy(tempLowerLast, tempCopyLast);
	int index = 0;
	while (index <= (int)strlen(from))
	{
		if (index < begin) *(to + index) = *(tempLowerFirst + index);
		else if (index >= begin && index < end) *(to + index) = *(tempCopy + (index - begin));
		else *(to + index) = *(tempLowerLast + (index - end));
		index += 1;
	}
	*(to + index) = '\0';
	while (index > 1)
	{
		to++;
		index--;
	}
	*to = 0;
	return start;
}

char* MakeLowerCopy(char* to, const char* from)
{
	if (!from) 
		return "unknown";
	char* start = to;
	while (*from)
	{
		char utfcharacter[10];
		char* x = IsUTF8((char*)from, utfcharacter); // return after this character if it is valid.
        if (utfcharacter[1])
        {
            LowerUTF8Char(utfcharacter);
            strcpy(to, utfcharacter);
            to += strlen(to);
            from = x;
        }
		else *to++ = GetLowercaseData(*from++);
	}
	*to = 0;
	return start;
}

char* MakeUpperCopy(char* to, const char* from)
{
	char* start = to;
	while (*from)
	{
		char utfcharacter[10];
		char* x = IsUTF8((char*)from, utfcharacter); // return after this character if it is valid.
        if (utfcharacter[1])
        {
            UpperUTF8Char(utfcharacter);
            strcpy(to, utfcharacter);
            to += strlen(to);
            from = x;
        }
		else *to++ = GetUppercaseData(*from++);
	}
	*to = 0;
	return start;
}

char* TrimSpaces(char* msg, bool start)
{
	char* orig = msg;
	if (start) while (IsWhiteSpace(*msg)) ++msg;
	size_t len = strlen(msg);
	if (len == 0) // if all blanks, shift to null at start
	{
		msg = orig;
		*msg = 0;
	}
	while (len && IsWhiteSpace(msg[len - 1])) msg[--len] = 0;
	return msg;
}

void UpcaseStarters(char* ptr) //   take a multiword phrase with _ and try to capitalize it correctly (assuming its a proper noun)
{
	if (IsLowerCase(*ptr)) *ptr -= 'a' - 'A';
	while (*++ptr)
	{
		if (!IsLowerCase(*++ptr) || *ptr != '_') continue; //   word separator
		if (!strnicmp(ptr, (char*)"the_", 4) || !strnicmp(ptr, (char*)"of_", 3) || !strnicmp(ptr, (char*)"in_", 3) || !strnicmp(ptr, (char*)"and_", 4)) continue;
		*ptr -= 'a' - 'A';
	}
}

char* documentBuffer = 0;

bool ReadDocument(char* inBuffer, FILE* source)
{
	static bool wasEmptyLine = false;
RETRY: // for sampling loopback
	*inBuffer = 0;
	if (!*documentBuffer || singleSource)
	{
		while (ALWAYS)
		{
			if (ReadALine(documentBuffer, source) < 0)
			{
				wasEmptyLine = false;
				return false;	// end of input
			}
			if (!*SkipWhitespace(documentBuffer)) // empty line
			{
				if (wasEmptyLine) continue;	// ignore additional empty line
				wasEmptyLine = true;
				if (docOut) fprintf(docOut, (char*)"\r\n%s\r\n", inBuffer);
				return true; // no content, just null line or all white space
			}

			if (*documentBuffer == ':' && IsAlphaUTF8(documentBuffer[1]))
			{
				if (!stricmp(documentBuffer, (char*)":exit") || !stricmp(documentBuffer, (char*)":quit"))
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
		if (docOut) fprintf(docOut, (char*)"\r\n%s\r\n", inBuffer);
		return true;
	}

	unsigned int readAhead = 0;
	while (++readAhead < 4) // read up to 3 lines
	{
		// pick earliest punctuation that can terminate a sentence
		char* period = NULL;
		char* at = documentBuffer;
		char* oobfound = strchr(documentBuffer, OOB_START); // oobfound marker
		if (oobfound && (oobfound - documentBuffer) < 5) oobfound = strchr(oobfound + 3, OOB_START); // find a next one // seen at start doesnt count.
		if (!oobfound) oobfound = documentBuffer + 300000;	 // way past anything
		while (!period && (period = strchr(at, '.')))	// normal input end here?
		{
			if (period > oobfound) break;
			if (period[1] && !IsWhiteSpace(period[1]))
			{
				if (period[1] == '"' || period[1] == '\'') // period shifted inside the quote
				{
					++period;	 // report " or ' as the end so whole thing gets tokenized
					break;
				}
				at = period + 1;	// period in middle of something
			}
			else if (IsDigit(*(period - 1)))
			{
				// is this number at end of sentence- e.g.  "That was in 1992. It became obvious."
				char* next = SkipWhitespace(period + 1);
				if (*next && IsUpperCase(*next)) break;
				else break;	// ANY number ending in a period is an end of sentence. Assume no one used period unless they want a digit after it
				//at = period + 1; // presumed a number ending in period, not end of sentence
			}
			else if (IsWhiteSpace(*(period - 1))) break; // isolated period
			else // see if token it ends is reasonable
			{
				char* before = period;
				while (before > documentBuffer && !IsWhiteSpace(*--before));	// find start of word
				char next2 = (period[1]) ? *SkipWhitespace(period + 2) : 0;
				if (ValidPeriodToken(before + 1, period + 1, period[1], next2) == TOKEN_INCLUSIVE) at = period + 1;	// period is part of known token
				else break;
			}
			period = NULL;
		}
		char* question = strchr(documentBuffer, '?');
		if (question && (question[1] == '"' || question[1] == '\'') && question < oobfound) ++question;	// extend from inside a quote
		char* exclaim = strchr(documentBuffer, '!');
		if (exclaim && (exclaim[1] == '"' || exclaim[1] == '\'') && exclaim < oobfound) ++exclaim;	// extend from inside a quote
		if (!period && question) period = question;
		else if (!period && exclaim) period = exclaim;
		if (exclaim && exclaim < period) period = exclaim;
		if (question && question < period) period = question;

		// check for things other than sentence periods as well here
		if (period) // found end of a sentence before any oob
		{
			char was = period[1];
			period[1] = 0;
			strcpy(inBuffer, SkipWhitespace(documentBuffer)); // copied over to input
			period[1] = was;
			memmove(documentBuffer, period + 1, strlen(period + 1) + 1);	// rest moved up in holding area
			break;
		}
		else if (singleSource) // only 1 line at a time regardless of it not being a complete sentence
		{
			strcpy(inBuffer, SkipWhitespace(documentBuffer));
			break;
		}
		else // has no end yet
		{
			at = SkipWhitespace(documentBuffer);
			if (*at == OOB_START)  // starts with OOB, add no more lines onto it.
			{
				strcpy(ourMainInputBuffer, at);
				*documentBuffer = 0;
				break;
			}

			if (oobfound < (documentBuffer + 300000)) // we see oob data
			{
				if (at != oobfound) // stuff before is auto terminated by oob data
				{
					*oobfound = 0;
					strcpy(inBuffer, at);
					*oobfound = OOB_START;
					memmove(documentBuffer, oobfound, strlen(oobfound) + 1);
					break;
				}
			}

			size_t len = strlen(documentBuffer);
			documentBuffer[len] = ' '; // append 1 blanks
			ReadALine(documentBuffer + len + 1, source, maxBufferSize - 4 - len);	//	 ahead input and merge onto current
			if (!documentBuffer[len + 1]) // logical end of file
			{
				strcpy(inBuffer, SkipWhitespace(documentBuffer));
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
		Log(USERLOG, "Heavy long line? %s\r\n", documentBuffer);
	if (autonumber) Log(ECHOUSERLOG, (char*)"%d: %s\r\n", inputSentenceCount, inBuffer);
	else if (docstats)
	{
		if ((++docSentenceCount % 1000) == 0)  Log(ECHOUSERLOG, (char*)"%d: %s\r\n", docSentenceCount, inBuffer);
	}
	wasEmptyLine = false;
	if (docOut) fprintf(docOut, (char*)"\r\n%s\r\n", inBuffer);

	return true;
}

