#ifndef _TEXTUTILITIESH_
#define _TEXTUTILITIESH_

#ifdef INFORMATION
Copyright (C)2011-2024 by Bruce Wilcox

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

enum PurifyKind {
	SYSTEM_PURIFY = 0,
	INPUT_PURIFY = 1,
	INPUT_TESTOUTPUT_PURIFY = 2,
	TAB_PURIFY = 3
};

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
#define FRENCH_NUMBERS 2  // and german and spanish

#define UNINIT ((unsigned int)-1)

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


#define RESTORE_LOGGING() serverLog = holdServerLog; userLog = holdUserLog;

// accesses to these arrays MUST use unsigned char in the face of UTF8 strings
extern int BOM;
extern unsigned char punctuation[];
extern unsigned char toLowercaseData[];
extern unsigned char toUppercaseData[];
extern unsigned char isVowelData[];
extern unsigned char isAlphabeticDigitData[];
extern unsigned char isComparatorData[];
extern unsigned char legalNaming[256];
extern char* currentInput;
extern unsigned char realPunctuation[256];
extern bool moreToCome, moreToComeQuestion;
extern signed char nestingData[];
extern bool fullfloat;
extern FILE* docOut;
extern bool newline;
#define MAX_CONDITIONALS 10
extern int numberStyle;
extern char conditionalCompile[MAX_CONDITIONALS+1][50];
extern int conditionalCompiledIndex;
void CheckForOutputAPI(char* msg);
extern char numberComma;
extern char numberPeriod;
extern unsigned int startSentence;
extern unsigned int endSentence;
extern unsigned char toHex[16];
extern unsigned char hasHighChar;
extern bool showBadUTF;
extern char* userRecordSourceBuffer;
extern char tmpWord[MAX_WORD_SIZE];
extern bool singleSource;
extern bool echoDocument;
extern char* documentBuffer;
extern unsigned char toHex[16];
extern int docSampleRate, docSample;
extern uint64 docVolleyStartTime;

#define JAPANESE_PUNCTUATION 1
#define JAPANESE_HIRAGANA 2
#define JAPANESE_KATAKANA 3
#define JAPANESE_KANJI 4

#define IsPunctuation(c) (punctuation[(unsigned char)c])
#define IsRealPunctuation(c) (realPunctuation[(unsigned char)c])
#define GetLowercaseData(c) (toLowercaseData[(unsigned char)c])
#define GetUppercaseData(c) (toUppercaseData[(unsigned char)c])
#define GetNestingData(c) (nestingData[(unsigned char)c])
#define IsHexDigit(c) ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))

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
#define IsSign(c) (c == '-' || c == '+')
#define IsInvalidEmailCharacter(c) (punctuation[(unsigned char)c] == PUNCTUATIONS || punctuation[(unsigned char)c] == BRACKETS || punctuation[(unsigned char)c] == ENDERS) // arithmetic enders are OK
#define IsOdd(c) (c % 2)
#define IsEven(c) (!(c % 2))
WORDP BUILDCONCEPT(char* word) ;
void RemoveTilde(char* output);
double Convert2Double(char* original,int useNumberStyle = AMERICAN_NUMBERS);
char* RemoveEscapesWeAdded(char* at);
bool IsComparator(char* word);
void ConvertNL(char* ptr);
char* FindJMSeparator(char* ptr, char c);
void CopyParam(char* from, char* to, unsigned int limit = 100);
char* IsSymbolCurrency(char* ptr);
void ComputeWordData(char* word, WORDINFO* info);
char* CopyRemoveEscapes(char* to, char* at,int limit,bool all = false);
char* AddEscapes(char* to, const char* from,bool normal,int limit,bool addescape=true,bool convert2utf16=false);
void AcquireDefines(const char* fileName);
void AcquirePosMeanings(bool facts);
char* FindNameByValue(uint64 val); // properties
char* PurifyInput(char* input,char* copy,size_t& size,PurifyKind kind);
uint64 FindPropertyValueByName(char* name);
char* ReadToken(const char* ptr, char* word);
FunctionResult AnalyzeCode(char* buffer);
void SetContinuationInput(char* buffer);
bool IsAssignOp(char* word);
void ClearSupplementalInput();
void PhoneNumber(unsigned int start);
char* RemoveQuotes(char* item);
bool IsAllUpper(char* ptr);
void Translate_UTF16_2_UTF8(char* input);
void ConvertJapanText(char* output,bool pattern);
int IsJapanese(char* utf8letter, unsigned char* utf16letter,int& kind);
char* GetNextInput();
char* HexDisplay(char* str);
void MoreToCome();
void ClearNumbers();
void EraseCurrentInput();
bool LegalVarChar(char at);
char* GetActual(char* msg);
bool AddInput(char* buffer,int kind,bool clear = false);
char* FindSystemNameByValue(uint64 val); // system flags
uint64 FindSystemValueByName(char* name);
char* FindParseNameByValue(uint64 val); // parse flags
uint64 FindParseValueByName(char* name); // parse flags
uint64 FindMiscValueByName(char* name); // misc data
void CloseTextUtilities();
void Clear_CRNL(char* incoming);
char* FixHtmlTags(char* html);
bool IsModelNumber(char* word);
char* ReadPatternToken(char* ptr, char* word);
char* ReadTabField(char* buffer, char* storage);
bool IsInteger(char* ptr, bool comma, int useNumberStyle = AMERICAN_NUMBERS);
char* IsUTF8(char* buffer,char* character);
char* Purify(char* msg);
char* SkipOOB(char* buffer);
void WriteInteger(char* word, char* buffer, int useNumberStyle = NOSTYLE_NUMBERS);
void BOMAccess(int &BOMvalue, char &oldc, int &oldCurrentLine);
size_t OutputLimit(unsigned char* data);
void ConvertQuotes(char* ptr);
bool IsFraction(char* token);
char* UTF16_2_UTF8(char* in,bool withinquote);

// boolean style tests
bool AdjustUTF8(char* start, char* buffer);
bool IsArithmeticOperator(char* word);
bool IsArithmeticOp(char* word);
uint64 ComposeNumber(unsigned int i, unsigned int& end);
unsigned IsNumber(char* word,int useNumberStyle = AMERICAN_NUMBERS,bool placeAllowed = true); // returns kind of number
bool IsPlaceNumber(char* word, int useNumberStyle = AMERICAN_NUMBERS);
bool IsFractionNumber(char* word);
bool IsDigitWord(char* word,int useNumberStyle = AMERICAN_NUMBERS,bool comma = false, bool checkAll = false);
bool IsDigitWithNumberSuffix(char* number,int useNumberStyle = AMERICAN_NUMBERS);
bool IsMail(char* word);
bool IsTLD(char* word);
bool IsUrl(char* word, char* end);
bool IsFileExtension(char* word);
bool IsFileName(char* word);
bool IsEmojiShortname(char* word, bool all = true);
bool IsPureNumber(char* word);
unsigned int IsMadeOfInitials(char * word,char* end);
bool IsNumericDate(char* word,char* end);
char IsFloat(char* word, char* end, int useNumberStyle = AMERICAN_NUMBERS);
char GetTemperatureLetter (char* ptr);
char* IsTextCurrency(char* ptr, char* end);
bool IsLegalName(const char* name,bool label = false);
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
char* MakeLowerCopy(char* to,const char* from);
char* MakeUpperCopy(char* to, const char* from);
char* PartialLowerCopy(char* to, const char* from,int begin,int end);
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
void InitUniversalTextUtilities();
void InitTextUtilitiesByLanguage(char* language);
bool ReadDocument(char* inBuffer,FILE* sourceFile);

// reading functions
char* ReadFlags(char* ptr,uint64& flags,bool &bad, bool &response, bool factcall = false);
char* ReadHex(char* ptr, uint64 & value);
char* ReadInt(char* ptr, int & value);
char* ReadInt64(char* ptr, int64 & w);
int64 atoi64(char* ptr );
char* ReadQuote(char* ptr, char* buffer,bool backslash = false, bool noblank = true,int limit = MAX_WORD_SIZE - 10);
char* ReadArgument(char* ptr, char* buffer, FunctionResult &result);

int ReadALine(char* buf,FILE* file,unsigned int limit = maxBufferSize,bool returnEmptyLines = false,bool convertTabs = true, bool untouched = false);
char* SkipWhitespace(const char* ptr);
char* BalanceParen(char* ptr,bool within=true,bool wildcards=false);
int64 NumberPower(char* number,int useNumberStyle = AMERICAN_NUMBERS);
int64 Convert2Integer(char* word,int useNumberStyle = AMERICAN_NUMBERS);

#endif
