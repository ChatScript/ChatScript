#ifndef _JSONH_
#define _JSONH_
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
extern unsigned int build0jid;
extern unsigned int build1jid;
extern unsigned int buildbootjid;
extern unsigned int builduserjid;
extern unsigned int buildtransientjid;

extern int jsonOpenSize;
extern int json_open_counter;
extern uint64 json_open_time ;
extern bool curlFail;
extern int jsonDefaults;
void InitJson();
FunctionResult JSONTreeCode(char* buffer);
FunctionResult JSONKindCode(char* buffer);
FunctionResult JSONStorageCode(char* buffer);
FunctionResult JSONPathCode(char* buffer);
FunctionResult JSONFormatCode(char* buffer);
FunctionResult JSONParseFileCode(char* buffer);
char* jwritehierarchy(bool log, bool defaultZero, int depth, char* buffer, WORDP D, int subject, int nest);
FunctionResult JSONObjectInsertCode(char* buffer) ;
FunctionResult JSONVariableAssign(char* word,char* value, bool stripQuotes = true);
FunctionResult JSONArrayInsertCode(char* buffer) ;
FunctionResult JSONLabelCode(char* buffer) ;
FunctionResult JSONUndecodeStringCode(char* buffer) ;
FunctionResult JSONWriteCode(char* buffer);
FunctionResult JSONParseCode(char* buffer);
FunctionResult JSONTextCode(char* buffer);
FunctionResult JSONArrayDeleteCode(char* buffer);
FunctionResult JSONArraySizeCode(char* buffer);
FunctionResult JSONGatherCode(char* buffer);
FunctionResult DoJSONArrayInsert(bool nodup, WORDP array, MEANING value, int flags, char* buffer); //  objectfact objectvalue  BEFORE/AFTER 
FunctionResult ParseJson(char* buffer, char* message, size_t size,bool nofail,char* ignore,bool purify = false, char* underscore = NULL);
FunctionResult JSONDeleteCode(char* buffer); 
FunctionResult JsonReuseKillCode(char* buffer);
FunctionResult JSONMergeCode(char* buffer);
FunctionResult JSONCopyCode(char* buffer);
FunctionResult JSONCreateCode(char* buffer);
FunctionResult JSONReadCSVCode(char* buffer);
MEANING GetUniqueJsonComposite(char* prefix, unsigned int permenent = 0);
MEANING jsonValue(char* value, unsigned int& flags, bool stripQuotes = true);
void JsonRenumber(FACT* F);
void jkillfact(WORDP D);
void InitJSONNames();
unsigned int jwritesize(WORDP D, int subject, bool plain, unsigned int size = 0);
char* jwrite(char* start,char* buffer, WORDP D, int subject,bool plain = false,unsigned int limit = 0);
bool IsValidJSONName(char* word, char type = 0);
int orderJsonArrayMembers(WORDP D, FACT * *store,size_t & size);
void WalkJsonHierarchy(WORDP D, FACT_FUNCTION func, uint64 data = 0, int depth = 0);

#ifndef DISCARDJSONOPEN
char* UrlEncodePiece(char* input);
FunctionResult JSONOpenCode(char* buffer);
FunctionResult InitCurl();
void CurlShutdown();
const char* CurlVersion();
#endif

#endif
