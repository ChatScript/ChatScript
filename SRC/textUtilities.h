#ifndef _TEXTUTILITIESH_
#define _TEXTUTILITIESH_

#ifdef INFORMATION
Copyright (C)2011-2018 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

// end of word status on ptr
#define TOKEN_INCLUSIVE 1
#define TOKEN_EXCLUSIVE 2
#define TOKEN_INCOMPLETE 4

#define BOMSET 2
#define BOMUTF8 1
#define NOBOM 0

#define ESCAPE_FLAG  0x7f  // we added an escape for compatibility from ascii, revert it out when reading in

#define SPACES 1			//   \t \r \n 
#define PUNCTUATIONS 2      //    , | -  (see also ENDERS )
#define ENDERS	4			//   . ; : ? ! -
#define BRACKETS 8			//   () [ ] { } < >
#define ARITHMETICS 16		//    % * + - ^ = / .  (but / can be part of a word)
#define SYMBOLS 32			//    $ # @ ~  ($ and # can preceed, & is stand alone)
#define CONVERTERS 64		//   & `
#define QUOTERS 128			//   " ' *

#define VALIDNONDIGIT 1
#define VALIDDIGIT 2
#define VALIDUPPER 3
#define VALIDLOWER 4
#define VALIDUTF8 6

#define NOSTYLE_NUMBERS -1
#define AMERICAN_NUMBERS 0
#define INDIAN_NUMBERS 1
#define FRENCH_NUMBERS 2

#define UNINIT -1

#define SHOUT 1
#define ABBREVIATION 2

#define NOT_A_NUMBER 2147483646
#define DIGIT_NUMBER 1 // composed entirely of number characters same as realnumber
#define CURRENCY_NUMBER 2
#define PLACETYPE_NUMBER 3
#define ROMAN_NUMBER 4
#define WORD_NUMBER 5 // a word that implies a number
#define REAL_NUMBER WORD_NUMBER // a word that IS a number
#define FRACTION_NUMBER 6

#define LETTERMAX 40

typedef struct WORDINFO
{
    char* word;
    int charlen; // characters in word
    int bytelen; // bytes in word
} WORDINFO;

// accesses to these arrays MUST use unsigned char in the face of UTF8 strings
extern int BOM;
extern unsigned char punctuation[];
extern unsigned char toLowercaseData[];
extern unsigned char toUppercaseData[];
extern unsigned char isVowelData[];
extern unsigned char isAlphabeticDigitData[];
extern unsigned char isComparatorData[];
extern unsigned char legalNaming[256];
extern unsigned char realPunctuation[256];
extern signed char nestingData[];
extern bool fullfloat;
extern FILE* docOut;
extern bool newline;
#define MAX_CONDITIONALS 10
extern int numberStyle;
extern char conditionalCompile[MAX_CONDITIONALS+1][50];
extern int conditionalCompiledIndex;
extern char numberComma;
extern char numberPeriod;

extern bool showBadUTF;
extern char* userRecordSourceBuffer;
extern char tmpWord[MAX_WORD_SIZE];
extern bool singleSource;
extern bool echoDocument;
extern char* documentBuffer;
extern char toHex[16];
extern int docSampleRate, docSample;
extern uint64 docVolleyStartTime;
#define IsPunctuation(c) (punctuation[(unsigned char)c])
#define IsRealPunctuation(c) (realPunctuation[(unsigned char)c])
#define GetLowercaseData(c) (toLowercaseData[(unsigned char)c])
#define GetUppercaseData(c) (toUppercaseData[(unsigned char)c])
#define GetNestingData(c) (nestingData[(unsigned char)c])

#define IsWhiteSpace(c) (punctuation[(unsigned char)c] == SPACES)
#define IsWordTerminator(c) (punctuation[(unsigned char)c] == SPACES || c == 0)
#define IsVowel(c) (isVowelData[(unsigned char)c] != 0)
#define IsAlphaUTF8DigitNumeric(c) (isAlphabeticDigitData[(unsigned char)c] != 0)
#define IsUpperCase(c) (isAlphabeticDigitData[(unsigned char)c] == 3)
#define IsLowerCase(c) (isAlphabeticDigitData[(unsigned char)c] == 4)
#define IsAlphaUTF8(c) (isAlphabeticDigitData[(unsigned char)c] >= VALIDUPPER) // upper, lower, utf8
#define IsLegalNameCharacter(c)  legalNaming[(unsigned char)c] 
#define IsDigit(c) (isAlphabeticDigitData[(unsigned char)c] == VALIDDIGIT)
#define IsAlphaUTF8OrDigit(c) (isAlphabeticDigitData[(unsigned char)c] >= VALIDDIGIT)
#define IsNonDigitNumberStarter(c) (isAlphabeticDigitData[(unsigned char)c] == VALIDNONDIGIT)
#define IsNumberStarter(c) (isAlphabeticDigitData[(unsigned char)c] && isAlphabeticDigitData[(unsigned char)c] <= VALIDDIGIT)
#define IsComparison(c) (isComparatorData[(unsigned char)c])
WORDP BUILDCONCEPT(char* word) ;
void RemoveTilde(char* output);
double Convert2Float(char* original,int useNumberStyle = AMERICAN_NUMBERS);
char* RemoveEscapesWeAdded(char* at);
bool IsComparator(char* word);
void ConvertNL(char* ptr);
char* IsSymbolCurrency(char* ptr);
void ComputeWordData(char* word, WORDINFO* info);
char* CopyRemoveEscapes(char* to, char* at,int limit,bool all = false);
char* AddEscapes(char* to, char* from,bool normal,int limit);
void AcquireDefines(char* fileName);
void AcquirePosMeanings(bool facts);
char* FindNameByValue(uint64 val); // properties
uint64 FindValueByName(char* name);
void ClearNumbers();
char* FindSystemNameByValue(uint64 val); // system flags
uint64 FindSystemValueByName(char* name);
char* FindParseNameByValue(uint64 val); // parse flags
uint64 FindParseValueByName(char* name); // parse flags
uint64 FindMiscValueByName(char* name); // misc data
void CloseTextUtilities();
bool IsModelNumber(char* word);
bool IsInteger(char* ptr, bool comma, int useNumberStyle = AMERICAN_NUMBERS);
char* IsUTF8(char* buffer,char* character);
char* Purify(char* msg);
void WriteInteger(char* word, char* buffer, int useNumberStyle = NOSTYLE_NUMBERS);
void BOMAccess(int &BOMvalue, char &oldc, int &oldCurrentLine);
size_t OutputLimit(unsigned char* data);
void ConvertQuotes(char* ptr);
extern int startSentence;
extern int endSentence;
extern bool hasHighChar;
bool IsFraction(char* token);
// boolean style tests
bool AdjustUTF8(char* start, char* buffer);
bool IsArithmeticOperator(char* word);
unsigned IsNumber(char* word,int useNumberStyle = AMERICAN_NUMBERS,bool placeAllowed = true); // returns kind of number
bool IsPlaceNumber(char* word, int useNumberStyle = AMERICAN_NUMBERS);
bool IsDigitWord(char* word,int useNumberStyle = AMERICAN_NUMBERS,bool comma = false);
bool IsDigitWithNumberSuffix(char* number,int useNumberStyle = AMERICAN_NUMBERS);
bool IsUrl(char* word, char* end);
unsigned int IsMadeOfInitials(char * word,char* end);
bool IsNumericDate(char* word,char* end);
bool IsFloat(char* word, char* end, int useNumberStyle = AMERICAN_NUMBERS);
char GetTemperatureLetter (char* ptr);
char* IsTextCurrency(char* ptr, char* end);
bool IsLegalName(char* name,bool label = false);
unsigned char* GetCurrency(unsigned char* ptr,char* &number);
bool IsCommaNumberSegment(char* start,char* end);
bool IsRomanNumeral(char* word, uint64& val);
char* WriteFloat(char* buffer, double value, int useNumberStyle = NOSTYLE_NUMBERS);
unsigned int UTFStrlen(char* ptr);
unsigned int UTFPosition(char* ptr, unsigned int pos);
unsigned int UTFOffset(char* ptr, char* c);

// conversion reoutines
void MakeLowerCase(char* ptr);
void MakeUpperCase(char* ptr);
char* MakeLowerCopy(char* to,char* from);
char* MakeUpperCopy(char* to,char* from);
void UpcaseStarters(char* ptr);
void Convert2Underscores(char* buffer);
void Convert2Blanks(char* output);
void ForceUnderscores(char* ptr);
char* TrimSpaces(char* msg,bool start = true);
char* UTF2ExtendedAscii(char* bufferfrom);
void RemoveImpure(char* buffer);
void ChangeSpecial(char* buffer);
void FormatFloat(char* word, char* buffer, int useNumberStyle = NOSTYLE_NUMBERS);

// startup
void InitTextUtilities();
void InitTextUtilities1();
bool ReadDocument(char* inBuffer,FILE* sourceFile);

// reading functions
char* ReadFlags(char* ptr,uint64& flags,bool &bad, bool &response, bool factcall = false);
char* ReadHex(char* ptr, uint64 & value);
char* ReadInt(char* ptr, int & value);
char* ReadInt64(char* ptr, int64 & w);
int64 atoi64(char* ptr );
char* ReadQuote(char* ptr, char* buffer,bool backslash = false, bool noblank = true,int limit = MAX_WORD_SIZE - 10);
char* ReadArgument(char* ptr, char* buffer, FunctionResult &result);

int ReadALine(char* buf,FILE* file,unsigned int limit = maxBufferSize,bool returnEmptyLines = false,bool convertTabs = true);
char* SkipWhitespace(char* ptr);
char* BalanceParen(char* ptr,bool within=true,bool wildcards=false);
int64 NumberPower(char* number,int useNumberStyle = AMERICAN_NUMBERS);
int64 Convert2Integer(char* word,int useNumberStyle = AMERICAN_NUMBERS);

#endif
