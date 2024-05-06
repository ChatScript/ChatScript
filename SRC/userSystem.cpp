#include "common.h"

#ifndef USERFACTS 
#define USERFACTS 100
#endif
bool noretrybackup = false;
unsigned int userFactCount = USERFACTS;			// how many facts user may save in topic file
bool serverRetryOK = false;
bool stopUserWrite = false;
static char* backupMessages = NULL;
FACT* migratetop = NULL;

#define MAX_USER_MESSAGES MAX_USED

//   replies we have tried already
char chatbotSaid[MAX_USED+1][SAID_LIMIT+3];  //   tracks last n messages sent to user
char humanSaid[MAX_USED+1][SAID_LIMIT+3]; //   tracks last n messages sent by user in parallel with userSaid
int humanSaidIndex;
int chatbotSaidIndex;
char caller[MAX_WORD_SIZE];
char callee[MAX_WORD_SIZE];
char* userDataBase = NULL;                // current write user record base

static char* saveVersion = "jul2116";	// format of save file

unsigned int userFirstLine = 0;	// start volley of current conversation
uint64 setControl = 0;	// which sets should be saved with user

char ipAddress[ID_SIZE];
char computerID[ID_SIZE];
char computerIDwSpace[ID_SIZE];
char loginID[ID_SIZE];    //   user FILE name (lower case)
char loginName[ID_SIZE];    //   user typed name
char callerIP[ID_SIZE];

char timeturn15[100];
char timeturn0[50];
char timePrior[50];
static void SaveJSON(WORDP D);

void PartialLogin(char* caller,char* ip)
{
    //   make user name safe for file system
	char*  id = loginID;
	char* at = caller-1;
	char c;
	while ((c = *++at)) 
	{
		if (IsAlphaUTF8OrDigit(c) || c == '-' || c == '_'  || c == '+') *id++ = c;
		else if (c == ' ') *id++ = '_';
	}
	*id = 0;

	sprintf(logFilename,(char*)"%s/%slog-%s.txt",usersfolder,GetUserPath(loginID),loginID); // user log goes here

	if (ip) strcpy(callerIP,ip);
	else *callerIP = 0;
}

bool Login(char* user,char* usee,char* ip,char* incoming) //   select the participants
{
	//   case insensitive logins
	callee[0] = 0;
	caller[0] = 0;
	MakeLowerCopy(callee, usee);
	if (user)
	{
		strcpy(caller, user);
		//   allowed to name callee as part of caller name
		char* separator = strchr(caller, ':');
		if (separator) *separator = 0;
		if (separator && !*usee) MakeLowerCopy(callee, separator + 1); // override the bot
		strcpy(loginName, caller); // original name as he typed it

		MakeLowerCopy(caller, caller);
	}

	bool fakeContinue;
	if (callee[0] == '&') // allow to hook onto existing conversation w/o new start
	{
		*callee = 0;
		fakeContinue = true;
	}
	else fakeContinue = false;

	if (incoming[0] && incoming[1] == '#' && incoming[2] == '!') // naming bot to talk to- and whether to initiate or not - e.g. #!Rosette#my message
	{
		char* next = strchr(incoming + 3, '#');
		if (next)
		{
			*next = 0;
			MakeLowerCopy(callee, incoming + 3); // override the bot name (including future defaults if such)
			memcpy(incoming + 1, next + 1,strlen(next));	// the actual message.
		}
	}

	strcpy(ipAddress,(ip) ? ip : (char*)"");
	char* traceit = strstr(callee, "trace");
	if (callee == traceit) // enable tracing during login, given as bot name
	{
		trace = (unsigned int) -1;
		echo = true;
		*callee = 0;
	}
	if (traceit && *(traceit-1) == '/') // appended to bot name
	{
		trace = (unsigned int)-1;
		echo = true;
		*(traceit-1) = 0;
	}
	if (!stricmp(usee, (char*)"time")) // enable timing during login
	{
		timing = (unsigned int)-1 ^ TIME_ALWAYS;
		*callee = 0;
	}
	if (*callee) MakeLowerCopy(computerID, callee);
	else ReadComputerID(); //   we are defaulting the chatee
	if (!*computerID) ReportBug((char*)"No default bot?\r\n");

	//   for topic access validation
	*computerIDwSpace = ' ';
	MakeLowerCopy(computerIDwSpace+1,computerID);
	strcat(computerIDwSpace,(char*)" ");
	if (*ipAddress) // maybe use ip in generating unique login
	{
		if (!stricmp(caller,(char*)"guest")) sprintf(caller,(char*)"guest%s", ipAddress);
		else if (*caller == '.') sprintf(caller,(char*)"%s", ipAddress);
	}
	char* ptr = caller-1;
	while (*++ptr) 
	{
		if (*ptr == '/'  || *ptr == '?' || *ptr == '\\' || *ptr == '%' || *ptr == '*' || *ptr == ':'
			|| *ptr == '|' || *ptr == '"' || *ptr == '<' || *ptr == '>') *ptr = '_';
	}

    //   prepare for chat
    PartialLogin(caller, ipAddress);
	return fakeContinue;
 }

void ReadComputerID()
{
	if (*defaultbot)
	{
		strcpy(computerID, defaultbot); // command line parameter
		return;
	}
	strcpy(computerID,(char*)"anonymous");
	unsigned int oldlang = language_bits;
	WORDP D = FindWord((char*)"defaultbot", 0); // do we have a FACT with the default bot in it as verb
	if (!D && language_bits) // regular language failed
	{
		language_bits = LANGUAGE_UNIVERSAL;
		D = FindWord((char*)"defaultbot", 0); // do we have a FACT with the default bot in it as verb
	}
	language_bits = oldlang;
	if (D)
	{
		FACT* F = GetVerbNondeadHead(D);
		if (F) MakeLowerCopy(computerID,Meaning2Word(F->subject)->word);
	}
}

void ResetUserChat()
{
 	chatbotSaidIndex = humanSaidIndex = 0;
	setControl = 0;
}

static char* WriteUserFacts(char* ptr,bool sharefile,unsigned int limit,char* saveJSON)
{
	if (!ptr) return NULL;

    //   write out fact sets first, before destroying any transient facts
	sprintf(ptr,(char*)"%x #set flags\r\n",(unsigned int) setControl);
	ptr += strlen(ptr);
	int i;
    int count;
	if (!shared || sharefile)  for (i = 0; i <= MAX_FIND_SETS; ++i)
    {
		if (!(setControl & (uint64) ((uint64)1 << i))) continue; // purely transient stuff

		//   remove dead references 
		FACT** set = factSet[i];
        count = FACTSET_COUNT(i);
		int j;
        for (j = 1; j <= count; ++j)
		{
			FACT* F = set[j];
            if (F && F->flags & FACTDEAD)
			{
				memmove(&set[j],&set[j+1],sizeof(FACT*) * (count - j));
				--count;
				--j;
			}
		}
        if (!count) continue;

		// save this set
		sprintf(ptr,(char*)"#set %d\r\n",i); 
		ptr += strlen(ptr);
        for (j = 1; j <= count; ++j)
		{
			FACT* F = factSet[i][j];
			if (!F) strcpy(ptr,(char*)"0\r\n");
			else
			{
				WriteFact(F,false,ptr,false,true);
				if (F > factLocked) F->flags |= MARKED_FACT;	 // since we wrote this out here, DONT write out in general writeouts..
			}
			ptr += strlen(ptr);
			if ((unsigned int)(ptr - userDataBase) >= (userCacheSize - OVERFLOW_SAFETY_MARGIN))
			{
				ReportBug("WriteFacts overflow");
				return NULL;
			}
		}
		sprintf(ptr,(char*)"%s",(char*)"#end set\n"); 
		ptr += strlen(ptr);
     }
	strcpy(ptr,(char*)"#`end fact sets\r\n");
	ptr += strlen(ptr);

	// most recent facts, in order, but not those written out already as part of fact set (in case FACTDUPLICATE is on, dont want to recreate and not build2 layer facts)
	FACT* F = lastFactUsed+1; // point to 1st unused fact
	while (--F > factLocked && limit) // backwards down to base system facts
	{
		if (shared && !sharefile)  continue;
		if (saveJSON && !(F->flags & MARKED_FACT2) && F->flags & (JSON_OBJECT_FACT | JSON_ARRAY_FACT)) continue; // dont write this out
		if (!(F->flags & (FACTDEAD|FACTTRANSIENT|MARKED_FACT| FACTBOOT))) --limit; // we will write this
	}
	// ends on factlocked, which is not to be written out
	int counter = 0;
	char* xxstart = ptr;
 	while (++F <= lastFactUsed)  // lastFactUsed is a valid fact
	{
		if (shared && !sharefile)  continue;
		if (saveJSON && !(F->flags & MARKED_FACT2) && F->flags & (JSON_OBJECT_FACT | JSON_ARRAY_FACT)) continue; // dont write this out
		if (F->flags & MARKED_FACT2) F->flags ^= MARKED_FACT2;	// turn off json marking bit
        if (F->flags & FACTBOOT && !(F->flags & (FACTDEAD | FACTTRANSIENT))) 
            bootFacts = true;
        if (!(F->flags & (FACTDEAD|FACTTRANSIENT|MARKED_FACT| FACTBOOT)))
		{
			++counter;
			WriteFact(F,true,ptr,false,true); // facts are escaped safe for JSON
            if (trace & TRACE_USERFACT) Log(USERLOG,"Saved %s\r\n", ptr);
			ptr += strlen(ptr);
			if ((unsigned int)(ptr - userDataBase) >= (userCacheSize - OVERFLOW_SAFETY_MARGIN)) 
			{
				ReportBug("WriteFacts overflow");
				return NULL;
			}
		}
	}
	sprintf(ptr,(char*)"#`end user facts %d\r\n",counter);
	ptr += strlen(ptr);

	return ptr;
}

static bool ReadUserFacts()
{	
    //   read in fact sets
    char word[MAX_WORD_SIZE];
    *word = 0;
    ReadALine(readBuffer, 0); //   setControl
	ReadHex(readBuffer,setControl);
    while (ReadALine(readBuffer, 0)>= 0) 
    {
		if (*readBuffer == '#' && readBuffer[1] == ENDUNIT) break; // end of sets to read
		char* ptr = ReadCompiledWord(readBuffer,word);
        int setid;
        ptr = ReadInt(ptr,setid); 
		SET_FACTSET_COUNT(setid,0);
		if (trace & TRACE_USER) Log(USERLOG,"Facts[%d]\r\n",setid);
	    while (ReadALine(readBuffer, 0)>= 0) 
		{
			if (*readBuffer == '#') break;
			char* lclptr = readBuffer;
			FACT* F = ReadFact(lclptr,0);
			AddFact(setid,F);
			if (trace & TRACE_USERFACT) TraceFact(F);
        }
		if (*readBuffer == '#' && readBuffer[1] == ENDUNIT) break; // otherwise has #end set as the line
	}
	if (strcmp(readBuffer,(char*)"#`end fact sets")) 
	{
		ReportBug((char*)"Bad fact sets alignment\r\n");
		return false;
	}

	unsigned int oldlang = language_bits;
	language_bits = 0; // fact declares its language, don't use current language

	// read long-term user facts
	while (ReadALine(readBuffer, 0)>= 0) 
	{
		if (*readBuffer == '#' && readBuffer[1] == ENDUNIT) break;
		char* data = readBuffer;
		if (*data == '(' && strchr(data,')')) 
		{
			if (!ReadFact(data,0)) return false;
		}
		else 
		{
			ReportBug((char*)"Bad user fact %s\r\n",readBuffer);
			return false;
		}
	}	
	language_bits = oldlang;

    if (strncmp(readBuffer,(char*)"#`end user facts",16)) 
	{
		ReportBug((char*)"Bad user facts alignment\r\n");
		return false;
	}

	return true;
}

#define INFINITE_BUFFER 10000000
static char* SafeLine(char* line) // be JSON text safe
{
	if (!line) return line; // null variable (traced)
	char* word = AllocateBuffer(); // transient safe
	AddEscapes(word,line,false,MAX_BUFFER_SIZE);
	FreeBuffer();
	return word; // buffer will be used immediately so is safe
}

static char* WriteRecentMessages(char* ptr,bool sharefile,int messageCount)
{
	if (!ptr) return NULL; // buffer ran out long ago

    //   recent human inputs
	int start = humanSaidIndex - messageCount; 
	if (start < 0) start = 0;
	int i;
    if (!shared || sharefile) for (i = start; i < humanSaidIndex; ++i)  
	{
		size_t len = strlen(humanSaid[i]);
		if (len == 0) continue;
		if (len > 200) humanSaid[i][200] = 0;
		sprintf(ptr,(char*)"%s\r\n",SafeLine(humanSaid[i]));
		ptr += strlen(ptr);
		if ((unsigned int)(ptr - userDataBase) >= (userCacheSize - OVERFLOW_SAFETY_MARGIN)) return NULL;
	}
	strcpy(ptr,(char*)"#`end user\r\n");
	ptr += strlen(ptr);
	
	// recent chatbot outputs
 	start = chatbotSaidIndex - messageCount;
	if (start < 0) start = 0;
    if (!shared || sharefile) for (i = start; i < chatbotSaidIndex; ++i) 
	{
		size_t len = strlen(chatbotSaid[i]);
		if (len == 0) continue;
		if (len > 200) chatbotSaid[i][200] = 0;
		sprintf(ptr,(char*)"%s\r\n",SafeLine(chatbotSaid[i]));
		ptr += strlen(ptr);
		if ((unsigned int)(ptr - userDataBase) >= (userCacheSize - OVERFLOW_SAFETY_MARGIN)) return NULL;
	}
	strcpy(ptr,(char*)"#`end chatbot\r\n");
	ptr += strlen(ptr);

	return ptr;
}

static bool ReadRecentMessages() 
{
	char* buffer = AllocateBuffer(); // messages are limited in size so are safe
	char* recover = AllocateBuffer(); // messages are limited in size so are safe
	char* original = buffer;
	backupMessages = NULL;
	*buffer = 0;
	buffer[1] = 0;
	backupMessages = NULL;
    for (humanSaidIndex = 0; humanSaidIndex <= MAX_USED; ++humanSaidIndex) 
    {
        ReadALine(recover, 0);
		strcpy(humanSaid[humanSaidIndex],RemoveEscapesWeAdded( recover));
		if (*humanSaid[humanSaidIndex] == '#' && humanSaid[humanSaidIndex][1] == ENDUNIT) break; // #end
		strcpy(buffer,humanSaid[humanSaidIndex]);
		buffer += strlen(buffer) + 1;
    }
	if (humanSaidIndex > MAX_USED || strcmp(humanSaid[humanSaidIndex],(char*)"#`end user"))  // failure to end right
	{
		humanSaidIndex = 0;
		chatbotSaidIndex = 0;
		backupMessages = NULL;
		ReleaseStack(buffer);
		ReportBug((char*)"bad humansaid");
		return false;
	}
	else *humanSaid[humanSaidIndex] = 0;
	*buffer++ = 0;

	for (chatbotSaidIndex = 0; chatbotSaidIndex <= MAX_USED; ++chatbotSaidIndex) 
    {
        ReadALine(recover, 0);
		strcpy(chatbotSaid[chatbotSaidIndex],RemoveEscapesWeAdded( recover));
		if (*chatbotSaid[chatbotSaidIndex] == '#' && chatbotSaid[chatbotSaidIndex][1] == ENDUNIT) break; // #end
		strcpy(buffer,chatbotSaid[chatbotSaidIndex]);
		buffer += strlen(buffer) + 1;
    }
	if (chatbotSaidIndex > MAX_USED || strcmp(chatbotSaid[chatbotSaidIndex],(char*)"#`end chatbot")) // failure to end right
	{
		chatbotSaidIndex = 0;
		backupMessages = NULL;
		ReleaseStack(buffer);
		backupMessages = NULL;
		ReportBug((char*)"Bad message alignment\r\n");
		return false;
	}
	else *chatbotSaid[chatbotSaidIndex] = 0;
	*buffer++ = 0;
	backupMessages = AllocateHeap(original,buffer-original); // create a backup copy
	FreeBuffer();
    FreeBuffer();
	return true;
}

void RecoverUser() // regain stuff we loaded from user
{
	chatbotSaidIndex = humanSaidIndex = 0;
	char* at = backupMessages;
	while (at && *at) // read human said messages
	{
		strcpy(humanSaid[humanSaidIndex++],at);
		at += strlen(at)+1;
	}
	*humanSaid[humanSaidIndex] = 0;

	if (at) ++at; // skip over null separator

	while (at && *at) // read human said messages
	{
		strcpy(chatbotSaid[chatbotSaidIndex++],at);
		at += strlen(at)+1;
	}
	*chatbotSaid[chatbotSaidIndex] = 0;

	randIndex =  oldRandIndex;

	// recover loaded topic info
	memcpy(pendingTopicList,originalPendingTopicList,sizeof(unsigned int) * (originalPendingTopicIndex + 1));
	pendingTopicIndex = originalPendingTopicIndex;
}

void CopyUserTopicFile(char* newname)
{
    char file[SMALL_WORD_SIZE];
    sprintf(file,(char*)"%s/topic_%s_%s.txt",usersfolder,loginID,computerID);
    if (stricmp(current_language, "english")) sprintf(file, (char*)"%s/topic_%s_%s_%s.txt", usersfolder, loginID, computerID, current_language);
    char newfile[MAX_WORD_SIZE];
    sprintf(newfile,(char*)"LOGS/%s-topic_%s_%s.txt",newname,loginID,computerID);
    if (stricmp(current_language, "english")) sprintf(newfile, (char*)"LOGS/%s-topic_%s_%s_%s.txt", newname, loginID, computerID, current_language);
    CopyFile2File(file,newfile,false);
}

static void FollowJSON(FACT* F)
{
	if (F->flags & FACTTRANSIENT) return;	// not a real future reference
	F->flags |= MARKED_FACT2;	// we will save this fact
	if (F->flags & (JSON_ARRAY_VALUE | JSON_OBJECT_VALUE)) // follow forward
	{
		WORDP D = Meaning2Word(F->object);
		SaveJSON(D);  
	}
	// presume JSON object never points to a fact itself
}

static void SaveJSON(WORDP D) // json structure head
{
	if (!D) return;
	FACT* F = GetSubjectNondeadHead(D); // all facts of this object
	while (F)
	{
		if (!(F->flags & MARKED_FACT2) && F > factLocked) FollowJSON(F); // write out all this as well/ not yet marked and user writeable
		F = GetSubjectNondeadNext(F);
	}
}

char* WriteUserVariables(char* ptr,bool sharefile, bool compiled,char* saveJSON)
{
	if (!ptr) return NULL;
	sprintf(ptr, "$cs_language=%s\r\n", current_language); // where we left off
	ptr += strlen(ptr);
    if (!compiled)
    {
        sprintf(ptr, "$cs_jid=%u\r\n", builduserjid); // where we left off
        ptr += strlen(ptr);
    }

    HEAPREF varthread = userVariableThreadList;
	bool traceseen = false;
	bool timingseen = false;
	char word[MAX_WORD_SIZE];

	if (modifiedTrace) trace = modifiedTraceVal; // script set the value
	if (modifiedTiming) timing = modifiedTimingVal; // script set the value
	while (varthread)
	{
        uint64 Dx;
        varthread = UnpackHeapval(varthread, Dx, discard, discard);
        WORDP D = (WORDP)Dx;
        if (shared && !sharefile && !strnicmp(D->word,(char*)"$share_",7)) continue;
  		else if (shared && sharefile && strnicmp(D->word,(char*)"$share_",7)) continue;
		else if ( D->word[1] !=  TRANSIENTVAR_PREFIX && D->word[1] != LOCALVAR_PREFIX && (D->w.userValue || (D->internalBits & MACRO_TRACE))) // transients not dumped, nor are NULL values
		{
			char* val = D->w.userValue;
			// track json structures referred to
			if (val && val[0] == 'j' && (val[1] == 'o' || val[1] == 'a') && val[2] == '-' && val[3] != 't' && saveJSON) SaveJSON(FindWord(val));
			if (!stricmp(D->word, "$cs_language")) continue; // already done by engine
			if (!stricmp(D->word, "$cs_jid")) continue; // already done by engine
			if (!stricmp(D->word,"$cs_trace"))
			{
				if (traceUniversal) continue; // dont touch this
				traceseen = true;
				sprintf(word,(char*)"%u",(unsigned int)trace);
				val = word;
			}
			if (!stricmp(D->word, "$cs_time"))
			{
				timingseen = true;
				sprintf(word, (char*)"%u", (unsigned int)timing);
				val = word;
			}
			if (!val) val = ""; // for null variables being marked as traced
			if (D->internalBits & MACRO_TRACE) sprintf(ptr,(char*)"%s=`%s\r\n",D->word,SafeLine(val));
			else sprintf(ptr,(char*)"%s=%s\r\n",D->word,SafeLine(val));
			
			ptr += strlen(ptr);
			if (!compiled)
			{
				if ((unsigned int)(ptr - userDataBase) >= (userCacheSize - OVERFLOW_SAFETY_MARGIN)) return NULL;
			}
		}
    }
	if (!traceseen && !traceUniversal && !compiling)
	{
		sprintf(ptr,(char*)"$cs_trace=%u\r\n",trace);
		ptr += strlen(ptr);
	}
	if (!timingseen && !compiling)
	{
		sprintf(ptr, (char*)"$cs_time=%u\r\n", timing);
		ptr += strlen(ptr);
	}

	// now put out the function tracing bits
	unsigned int index = tracedFunctionsIndex;
	while (index)
    {
		WORDP D = tracedFunctionsList[--index];
		if (D->inferMark && D->internalBits & (FN_TRACE_BITS | NOTRACE_TOPIC | FN_TIME_BITS))
		{
			sprintf(ptr,(char*)"%s=%u %u\r\n",D->word,D->internalBits & (FN_TRACE_BITS | NOTRACE_TOPIC| FN_TIME_BITS),D->inferMark);
			ptr += strlen(ptr);
		}
	 }

	strcpy(ptr,(char*)"#`end variables\r\n");
	ptr += strlen(ptr);
	return ptr;
}

static bool ReadUserVariables()
{
	tracedFunctionsIndex = 0;
	while (ReadALine(readBuffer, 0)>= 0) //   user variables
	{
		if (*readBuffer != USERVAR_PREFIX && *readBuffer != FUNCTIONVAR_PREFIX) break; // end of variables
        char* ptr = strchr(readBuffer,'=');
        *ptr = 0; // isolate user var name from rest of buffer
		bool traceit = ptr[1] == '`';
		if (traceit)  ++ptr;
		if (*readBuffer == '$')
		{
			if (ptr[1] == 'j' && ptr[3] == '-' && (ptr[2] == 'a' || ptr[2] == 'o')) StoreWord(ptr + 1,AS_IS); // force possibly empty json struct to be recorded
			SetUserVariable(readBuffer, RemoveEscapesWeAdded(ptr + 1));
		}
		else // tracing bits on a function
		{
			WORDP F = FindWord(readBuffer,0,LOWERCASE_LOOKUP);
			if (F) 
			{
				char value[MAX_WORD_SIZE];
				ptr = ReadCompiledWord(ptr+1,value);
				int bits = atoi(value); // control flags
				ptr = ReadCompiledWord(ptr,value);
				int val = atoi(value); // which tracings
				if (val || bits)
				{
					F->internalBits |= bits; // covers trace and time bits
					F->inferMark = val;		// set its trace details bits
					tracedFunctionsList[tracedFunctionsIndex++] = F;
				}
			}
		}
		if (traceit) // we are tracing this variable, turn on his flag
		{ 
			WORDP D = StoreWord(readBuffer);
			PrepareVariableChange(D,"",false); // keep it alive as long as it is traced
			AddInternalFlag(D,MACRO_TRACE);
		}
		if (trace & TRACE_VARIABLE) Log(USERLOG,"%s=%s\r\n",readBuffer,ptr+1);
    }

	if (strcmp(readBuffer,(char*)"#`end variables")) 
	{
		ReportBug((char*)"Bad variable alignment\r\n");
		return false;
	}
	return true;
}

// unreferenced json MIGHT be referenced from factset

// user data never contains control characters (except what we add as line separators here).

static char* GatherUserData(char* ptr,time_t curr,bool sharefile)
{
	// need to get fact limit variable for WriteUserFacts BEFORE writing user variables, which clears them
	unsigned int count = userFactCount;
	char* value = GetUserVariable("$cs_userfactlimit",false);
    if (*value == '*') count = (unsigned int)-1;
    else if (*value) count = atoi(value);

	int messageCount = MAX_USER_MESSAGES;
	value = GetUserVariable("$cs_userhistorylimit", false);
	if (*value) messageCount = atoi(value);

	// each line MUST end with cr/lf  so it can be made potentially safe for Mongo or encryption w/o adjusting size of data (which would be inefficient)
	char* xxstart = ptr;
	if (!timeturn15[1] && volleyCount >= 15 && responseIndex) sprintf(timeturn15,(char*)"%lu-%d%s",(unsigned long)curr,responseData[0].topic,responseData[0].id); // delimit time of turn 15 and location...
	sprintf(ptr,(char*)"%s %s %s %s |\r\n",saveVersion,timeturn0,timePrior,timeturn15); 
	ptr += strlen(ptr);
	*ptr = 0;
	ptr = WriteUserTopics(ptr,sharefile);
	if (!ptr)
	{
		ReportBug("User file topic data too big %s",loginID);
		return NULL;
	}
	char* saveJSON = GetUserVariable("$cs_saveusedJson", false);
	if (!*saveJSON) saveJSON = NULL;

	ptr = WriteUserVariables(ptr,sharefile,false, saveJSON);  // json safe - does not write kernel or boot bot variables
	if (!ptr)
	{
		ReportBug("User file variable data too big %s",loginID);
		return NULL;
	}

	echo = false;
	ptr = WriteUserFacts(ptr,sharefile,count, saveJSON);  // json safe
	if (!ptr)
	{
		ReportBug("User file fact data too big %s",loginID);
		return NULL;
	}
	ptr = WriteUserContext(ptr,sharefile); 
	if (!ptr)
	{
		ReportBug("User file context data too big %s",loginID);
		return NULL;
	}
	ptr = WriteRecentMessages(ptr,sharefile, messageCount); // json safe
	if (!ptr)
	{
		ReportBug("User file message data too big %s", loginID);
		return NULL;
	}
	return ptr;
}

void WriteUserData(time_t curr, bool nobackup)
{ // does not modify any data, just transcribes to file
	// if (!numberOfTopics)  return; //   no topics ever loaded or we are not responding
	if (!userCacheCount) return;	// never save users - no history

	uint64 start_time = ElapsedMilliseconds();

	if (globalDepth == 0) currentRule = NULL;

	char name[MAX_WORD_SIZE];
	char filename[SMALL_WORD_SIZE];
	// NOTE mongo does not allow . in a filename
	sprintf(name, (char*)"%s/%stopic_%s_%s.txt", usersfolder, GetUserPath(loginID), loginID, computerID);
	if (!multidict && stricmp(current_language, "english")) sprintf(name, (char*)"%s/%stopic_%s_%s_%s.txt", usersfolder, GetUserPath(loginID), loginID, computerID, current_language);
	strcpy(filename, name);
	strcat(filename, "\r\n");
	userDataBase = GetFileBuffer();
	if (!userDataBase) return;		// not saving anything
	strcpy(userDataBase, filename);

#ifndef DISCARDTESTING
	if (filesystemOverride == NORMALFILES && (!server || serverRetryOK) && !documentMode && !callback)
	{
		char fname[MAX_WORD_SIZE];
		sprintf(fname, (char*)"%s/backup-%s_%s.txt", tmpfolder, loginID, computerID);
		if (!nobackup && !noretrybackup) CopyFile2File(fname, name, false);	// backup for debugging BUT NOT if callback of some kind...
		if (redo) // multilevel backup enabled
		{
			sprintf(fname, (char*)"%s/backup%u-%s_%s.txt", tmpfolder, volleyCount, loginID, computerID);
			if (!nobackup) CopyFile2File(fname, userDataBase, false);	// backup for debugging BUT NOT if callback of some kind...
		}
	}
#endif

	char* ptr = GatherUserData(userDataBase + strlen(filename), curr, false);
	if (ptr) Cache(userDataBase, ptr - userDataBase);
    FreeFileBuffer();
	if (ptr && shared)
	{
		sprintf(name, (char*)"%s/%stopic_%s_%s.txt", usersfolder, GetUserPath(loginID), loginID, (char*)"share");
		if (stricmp(current_language, "english")) sprintf(name, (char*)"%s/%stopic_%s_%s_%s.txt", usersfolder, GetUserPath(loginID), loginID, current_language, (char*)"share");
		strcpy(filename, name);
		strcat(filename, "\r\n");
		userDataBase = GetFileBuffer(); // cannot fail if we got to here
		strcpy(userDataBase, filename);

		// we write backup JUST BEFORE we save new state
#ifndef DISCARDTESTING
		if (!noretrybackup && filesystemOverride == NORMALFILES && (!server || serverRetryOK) && !documentMode && !callback)
		{
			sprintf(name, (char*)"%s/backup-share-%s_%s.txt", tmpfolder, loginID, computerID);
			if (!nobackup && !noretrybackup) CopyFile2File(name, userDataBase, false);	// backup for debugging
			if (redo)
			{
				sprintf(name, (char*)"%s/backup%u-share-%s_%s.txt", tmpfolder, volleyCount, loginID, computerID);
				if (!nobackup) CopyFile2File(name, userDataBase, false);	// backup for debugging BUT NOT if callback of some kind...
			}
		}
#endif
		ptr = GatherUserData(userDataBase + strlen(filename), curr, true);
		if (ptr) Cache(userDataBase, ptr - userDataBase);
        FreeFileBuffer();
    }

	if (timing & TIME_USER) {
		int diff = (int)(ElapsedMilliseconds() - start_time);
		if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"Write user data time: %d ms\r\n", diff);
	}

	migratetop = lastFactUsed;
}

static char* GetFileRead(char* user,char* computer)
{
    char word[MAX_WORD_SIZE];
    sprintf(word,(char*)"%s/%stopic_%s_%s.txt",usersfolder,GetUserPath(loginID),user,computer);
    if (stricmp(current_language,"english")) sprintf(word, (char*)"%s/%stopic_%s_%s_%s.txt", usersfolder, GetUserPath(loginID), user, computer, current_language);
    char* buffer;
    if ( filesystemOverride == NORMALFILES) // local files
    {
        char name[MAX_WORD_SIZE];
        strcpy(name,word);
        buffer = FindFileCache(name); // sets currentCache and makes it first if non-zero return -  will either find but not assign if not found
        if (buffer) return buffer;
    }
    uint64 start_time = ElapsedMilliseconds();

    // have to go read it
    buffer = GetFileBuffer(); // get cache buffer
    FILE* in = (!buffer) ? NULL : userFileSystem.userOpen(word); // user topic file

#ifdef LOCKUSERFILE
#ifdef LINUX
    if (server && in &&  filesystemOverride == NORMALFILES)
    {
        int fd = fileno(in);
        if (fd < 0)
        {
            userFileSystem.userClose(in);
            in = 0;
        }
        else
        {
            struct flock fl;
            fl.l_type   = F_RDLCK;  /* F_RDLCK, F_WRLCK, F_UNLCK    */
            fl.l_whence = SEEK_SET; /* SEEK_SET, SEEK_CUR, SEEK_END */
            fl.l_start  = 0;    /* Offset from l_whence        */
            fl.l_len    = 0;    /* length, 0 = to EOF        */
            fl.l_pid    = getpid(); /* our PID            */
            if (fcntl(fd, F_SETLKW, &fl) < 0)
            {
                userFileSystem.userClose(in);
                in = 0;
            }
        }
    }
#endif
#endif
    if (in) // read in data if file exists
    {
        size_t readit;
        readit = DecryptableFileRead(buffer,1,userCacheSize,in,userEncrypt,"USER"); // reading topic file of user
        buffer[readit] = 0;
        buffer[readit+1] = 0; // insure nothing can overrun
        userFileSystem.userClose(in);
        if (trace & TRACE_USERCACHE) Log((server) ? SERVERLOG : USERLOG,(char*)"read in %s \r\n",word);
        if (timing & TIME_USERCACHE) {
            int diff = (int)(ElapsedMilliseconds() - start_time);
            if (timing & TIME_ALWAYS || diff > 0) Log((server) ? SERVERLOG : STDTIMELOG, (char*)"Read user topic file in file %s time: %d ms\r\n", word, diff);
        }
    }
    CompleteBindStack(); // Trim the infinite stack to just what is needed
    return buffer;
}

static  bool ReadFileData(char* bot) // passed  buffer with file content (where feasible)
{
	strcpy(timePrior, (char*)"0");
	strcpy(timeturn15, (char*)"0");
	strcpy(timeturn0, (char*)"0");

	if (newuser) // dont load user, generate anew
	{
		newuser = false; // only good for this volley
		ReadNewUser();
		strcpy(timeturn0, GetMyTime(time(0))); // startup time
		return true;
	}

	loadingUser = true;
	char* buffer = GetFileRead(loginID, bot);
	char junk[MAX_WORD_SIZE];
	*junk = 0;

	// set bom
	if (buffer && *buffer != 0) // readable data
	{
		maxFileLine = currentFileLine = 0;
		BOM = BOMSET;
		char* at = strchr(buffer, '\r');
		if (at)
		{
			userRecordSourceBuffer = at + 2; // skip \r\n
			ReadALine(readBuffer, 0);

			char* x = ReadCompiledWord(readBuffer, junk);
			x = ReadCompiledWord(x, timeturn0); // the start stamp id if there
			x = ReadCompiledWord(x, timePrior); // the prior stamp id if there
			ReadCompiledWord(x, timeturn15); // the timeturn id if there
			if (stricmp(junk, saveVersion)) *buffer = 0;// obsolete format
		}
		else *buffer = 0;
	}
	if (!buffer || !*buffer)
	{
		// if shared file exists, we dont have to kill it. If one does exist, we merely want to use it to add to existing bots
		ReadNewUser();
		strcpy(timeturn0, GetMyTime(time(0))); // startup time
	}
	else
	{
		if (trace & TRACE_USER) Log(USERLOG,"\r\nLoading user: %s bot: %s\r\n",loginID, bot);
		char* start = buffer;
		size_t len = strlen(start);
		if (!ReadUserTopics())
		{
			if (len < 40000) { ReportBug("User data file TOPICS inconsistent %d %s \r\n", len, start); }
			else { ReportBug("User data file %d TOPICS inconsistent\r\n", len); }
            loadingUser = false;
            FreeFileBuffer();
            return false;
		}
		if (!ReadUserVariables())
		{
			if (len < 40000) ReportBug((char*)"User data file VARIABLES inconsistent %d %s \r\n",len,start);
			else ReportBug((char*)"User data file %d VARIABLES inconsistent\r\n", len);
            loadingUser = false;
            FreeFileBuffer();
            return false;
		}
		if (!ReadUserFacts())
		{
			if (len < 40000) ReportBug((char*)"User data file FACTS inconsistent %d %s \r\n", len, start);
			else ReportBug((char*)"User data file %d FACTS inconsistent\r\n", len);
            loadingUser = false;
            FreeFileBuffer();
            return false;
		}
		if (!ReadUserContext())
		{
			if (len < 40000) ReportBug((char*)"User data file CONTEXT inconsistent %d %s \r\n", len,start);
			else ReportBug((char*)"User data file %d CONTEXT inconsistent\r\n", len);
            loadingUser = false;
            FreeFileBuffer();
            return true; // accept failure
		}
		if (!ReadRecentMessages())
		{
			if (len < 40000) ReportBug((char*)"User data file MESSAGES inconsistent %d %s \r\n", len,start);
			else ReportBug((char*)"User data file %d MESSAGES inconsistent\r\n", len);
            loadingUser = false;
            FreeFileBuffer();
            return true;
		}
		if (trace & TRACE_USER) Log(USERLOG, "user loaded normally\r\n");
		oldRandIndex = randIndex = atoi(GetUserVariable((char*)"$cs_randindex", false)) + (volleyCount % MAXRAND);	// rand base assigned to user
	}
	userRecordSourceBuffer = NULL;
	if (traceUniversal) trace = traceUniversal;

	char* at = traceuser;
	size_t len = strlen(loginID);
	while (*at && (at = strstr(at, loginID))) // any in list
	{
		if (at == traceuser || at[len] == ',' || !at[len]) trace = (unsigned int)-1;
		at += 1;
	}
    loadingUser = false;
    FreeFileBuffer();
    return true;
}

void GetUserData(ResetMode& buildReset,char* incoming)
{
	loading = true;
	// accept a user topic file erase command
	char* x = NULL;
	if (*erasename && incoming) x = strstr(incoming, erasename);
	if (x)
	{
		buildReset = FULL_RESET;
		memset(x, ' ', strlen(erasename));
	}
	if (*newusername && incoming && strstr(incoming, newusername))
	{
		if (strstr(incoming, "testpattern") || strstr(incoming, "testoutput") ||
			strstr(incoming, "compilepattern") || strstr(incoming, "compileoutput"))
			buildReset = FULL_RESET;
	}

	// read user data, to control how to purifyinput if needed
	if (buildReset) // drop old user
	{
		if (trace & TRACE_USER) Log(USERLOG, "buildreset = %d\r\n", buildReset);
		if (buildReset == FULL_RESET)
		{
			newuser = true;
			KillShare();
		}
		ReadUserData();		//   now bring in user state
		buildReset = NO_RESET;
	}
	else if (!documentMode)
	{
		// preserve file status reference across this read use of ReadALine
		int BOMvalue = -1; // get prior value
		char oldc;
		int oldCurrentLine;
		BOMAccess(BOMvalue, oldc, oldCurrentLine); // copy out prior file access and reinit user file access
		char* suppressUser = GetUserVariable("$cs_new_user", false); // command line
		if (*suppressUser && strstr(originalUserInput, suppressUser)) newuser = true;
		ReadUserData();		//   now bring in user state, uses readbuffer
		BOMAccess(BOMvalue, oldc, oldCurrentLine); // restore old BOM values
	}
	loading = false;
}

void ReadUserData() // passed  buffer with file content (where feasible)
{	
	loading = true;
    myBot = 0;	// drop ownership of facts
	if (globalDepth == 0) currentRule = NULL;

	uint64 start_time = ElapsedMilliseconds();

	// std defaults
	tokenControl = (DO_SUBSTITUTE_SYSTEM | DO_INTERJECTION_SPLITTING | DO_PROPERNAME_MERGE | DO_NUMBER_MERGE | DO_SPELLCHECK );
	if (!stricmp(current_language,"english")) tokenControl |= DO_PARSE;
	responseControl = ALL_RESPONSES;
	*wildcardSeparator = ' ';
	wildcardSeparatorGiven = false;

	if (!ReadFileData(computerID))// read user file, if any, or get it from cache
	{
		(*printer)((char*)"%s",(char*)"User file inconsistent\r\n");
		shared = false;
		ResetToPreUser();
		ReadNewUser(); // abandon this user
	}
	if (shared) ReadFileData((char*)"share");  // read shared file, if any, or get it from cache

    int diff = (int)(ElapsedMilliseconds() - start_time);
    TrackTime((char*)"GetUserData",diff);
	if (timing & TIME_USER)
	{
		if (timing & TIME_ALWAYS || diff > 0) Log(STDTIMELOG, (char*)"Read user data time: %d ms\r\n", diff);
	}
	loading = false;
}

void KillShare()
{
	if (shared) 
	{
		char buffer[MAX_WORD_SIZE];
		sprintf(buffer,(char*)"%s/%stopic_%s_%s.txt",usersfolder,GetUserPath(loginID),loginID,(char*)"share");
		if (stricmp(current_language,"english")) sprintf(buffer, (char*)"%s/%stopic_%s_%s_%s.txt", usersfolder, GetUserPath(loginID), loginID, current_language,(char*)"share");
		unlink(buffer); // remove all shared data of this user
	}
}

void ReadNewUser()
{
	unsigned int oldtrace = trace;
	if (server) trace = 0;
	if (trace & TRACE_USER) Log(USERLOG,"0 Ctrl:New User\r\n");

	// if coming from script reset, these need to be cleared. 
	// They were already cleared at start of volley
	ClearUserVariables();
	ClearUserFacts();
	ResetTopicSystem(true);

	userFirstLine = 1;
	volleyCount = 0;
	// std defaults
	tokenControl = (DO_SUBSTITUTE_SYSTEM | DO_INTERJECTION_SPLITTING | DO_PROPERNAME_MERGE | DO_NUMBER_MERGE | DO_SPELLCHECK | DO_PARSE );
	responseControl = ALL_RESPONSES;
	*wildcardSeparator = ' ';
	wildcardSeparatorGiven = false;

	//   set his random seed
	unsigned int rand = (unsigned int) Hashit((unsigned char *) loginID,strlen(loginID),hasUpperCharacters,hasUTF8Characters, hasSeparatorCharacters);
	char word[MAX_WORD_SIZE];
	oldRandIndex = randIndex = rand & 4095;
    sprintf(word,(char*)"%u",randIndex);
	SetUserVariable((char*)"$cs_randindex",word ); 
	strcpy(word,computerID);
	*word = GetUppercaseData(*word);
	SetUserVariable((char*)"$cs_bot",word ); 
	SetUserVariable((char*)"$cs_login",loginName);

	sprintf(readBuffer,(char*)"^%s",computerID);
	WORDP D = FindWord(readBuffer,0,LOWERCASE_LOOKUP);
	if (!D) // no such bot
	{
		/// *computerID = 0; // allow anonymous bot to remain, so debug commands can function w/o bot
		return;
	}
	AllocateOutputBuffer();
	FunctionResult result;
	char arg[15]; // dofunction call needs to be alterable if tracing
	strcpy(arg,(char*)"()");
	DoFunction(D->word,arg,	currentOutputBase,result);
	FreeOutputBuffer();
	inputRejoinderTopic = inputRejoinderRuleID = NO_REJOINDER; 

	char file[SMALL_WORD_SIZE];
	sprintf(file,"%s-init.txt",loginName);
	userInitFile = fopen(file,(char*)"rb"); 
	trace = oldtrace;
	if (traceUniversal) trace = traceUniversal;
}
