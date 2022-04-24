#include "common.h"
#include "evserver.h"

extern SYSTEMVARIABLE sysvars[];

char* compileDate = __DATE__    " "  __TIME__;


static char systemValue[MAX_WORD_SIZE]; // common answer place

////////////////////////////////////////////////////
/// OVERVIEW CODE 
////////////////////////////////////////////////////
char dbparams[MAX_WORD_SIZE*4];

void InitSystemVariables()
{
	*dbparams = 0;
	unsigned int i = 0;
	while (sysvars[++i].name)
	{
		if (sysvars[i].name[0] == SYSVAR_PREFIX) 
		{
			StoreWord((char*) sysvars[i].name)->x.topicIndex = (unsigned short) i;  // not a header
			(*sysvars[i].address)((char*)"."); // force a reset-- testing calls this to reset after changes
		}
	}
}

char* SystemVariable(char* word,char* value)
{
	WORDP D = FindWord(word);
	unsigned int index = (D) ? D->x.topicIndex : 0;
	if (!index) return "";
	char* var = (*sysvars[index].address)(value);
	if (!var) return "";
	return var;
}

void DumpSystemVariables()
{
	unsigned int i = 0;
	while (sysvars[++i].name)
	{
		char* result = (sysvars[i].address) ? (*sysvars[i].address)(NULL) : (char*)""; // actual variable or header
		if (!*result) 
		{
			if (strstr(sysvars[i].comment,(char*)"Boolean")) result = "null";
			else if (strstr(sysvars[i].comment,(char*)"Numeric")) result = "0";
			else result = "null";
		}
		if (sysvars[i].address) Log(USERLOG,"%s = %s - %s\r\n",sysvars[i].name, result,sysvars[i].comment);  // actual variable
		else Log(USERLOG,"%s\r\n",sysvars[i].name);  // header
	}
}

static char* AssignValue(char* hold, char* value)
{
	if (value[0] == value[1] && value[1] == '"') *value = 0;	// null string
	else if (!stricmp(value,(char*)"NULL") ) *value = 0; 
	strcpy(hold,value);
	return hold;
}

////////////////////////////////////////////////////
/// TIME AND DATE 
////////////////////////////////////////////////////

static char* Sdate(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "1";
	struct tm ptm;
 	char* x = GetTimeInfo(&ptm) + 8;
    ReadCompiledWord(x,systemValue);
    return systemValue; //   1 or 2 digit date
}

static char* SdayOfWeek(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
    if (regression) return "Monday";
	struct tm ptm;
    ReadCompiledWord(GetTimeInfo(&ptm),systemValue);
    switch(systemValue[1])
    {
        case 'o': return "Monday";
        case 'u': return (char*)((systemValue[0] == 'T') ? (char*)"Tuesday" : (char*)"Sunday");
        case 'e': return "Wednesday";
        case 'h': return "Thursday";
        case 'r': return "Friday";
        case 'a': return "Saturday";
    }
	return "";
}

static char* SdayNumberOfWeek(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "2";
	struct tm ptm;
	ReadCompiledWord(GetTimeInfo(&ptm),systemValue);
	int n;
    switch(systemValue[1])
    {
		case 'u': n = (systemValue[0] != 'T') ? 1 : 3; break;
		case 'o': n = 2; break;
		case 'e': n = 4; break;
		case 'h': n = 5; break;
		case 'r': n = 6; break;
		case 'a': n = 7; break;
		default: n = 0; break;
	}
	systemValue[0] = (char)(n + '0');
	systemValue[1] = 0;
	return systemValue;
}

char* SFullTime(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	uint64 curr = (uint64) time(0);
    if (regression) curr = 44444444; 
#ifdef WIN32
   sprintf(systemValue,(char*)"%I64u",curr); 
#else
   sprintf(systemValue,(char*)"%llu",curr); 
#endif
    return systemValue;
}

static char* SStartTimeMS(char* value)
{
	// full time in milliseconds
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	sprintf(systemValue, (char*)"%llu", callStartTime);
	return systemValue;
}

static char* SFullTimeMS(char* value)
{
    // full time in milliseconds
    static char hold[50] = ".";
    if (value) return AssignValue(hold, value);
    if (*hold != '.') return hold;
    uint64 elapsedMS = ElapsedMilliseconds();
    sprintf(systemValue, (char*)"%llu", elapsedMS);
    return systemValue;
}

static char* Shour(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "11";
	struct tm ptm;
	strncpy(systemValue,GetTimeInfo(&ptm)+11,2);
	systemValue[2] = 0;
    return  systemValue;
}

static char* SleapYear(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "";
	time_t rawtime;
	time (&rawtime );
	struct tm timeinfo;
	mylocaltime (&rawtime,&timeinfo );
    int year = timeinfo.tm_year + 1900;
	bool leap = false;
	if ((year % 400) == 0) leap = true;
	else if ((year % 100) != 0 && (year % 4) == 0) leap = true;
    return leap ? (char*)"1" : (char*)"";
}  

static char* Sdaylightsavings(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "1";
	time_t rawtime;
	time (&rawtime );
 	struct tm timeinfo;
	mylocaltime (&rawtime,&timeinfo );
    int dst = timeinfo.tm_isdst;
    return dst ? (char*)"1" : (char*)"";
}  

static char* Sminute(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "12";
	struct tm ptm;
	ReadCompiledWord(GetTimeInfo(&ptm)+14,systemValue);
	systemValue[2] = 0;
	return systemValue;
}

static char* Smonth(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "6";
 	struct tm ptm;
    ReadCompiledWord(GetTimeInfo(&ptm)+SKIPWEEKDAY,systemValue);
	switch(systemValue[0])
	{
		case 'J':  //   january june july 
			if (systemValue[1] == 'a') return "1";
			else if (systemValue[2] == 'n') return "6";
			else if (systemValue[2] == 'l') return "7";
			break;
		case 'F': return "2";
		case 'M': return (systemValue[2] != 'y') ? (char*)"3" : (char*)"5"; 
  		case 'A': return (systemValue[1] == 'p') ? (char*)"4" : (char*)"8";
		case 'S': return "9";
		case 'O': return "10";
        case 'N': return "11";
        case 'D': return "12";
	}
	return "";
}

static char* SmonthName(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "June";
 	struct tm ptm;
    ReadCompiledWord(GetTimeInfo(&ptm)+SKIPWEEKDAY,systemValue);
	switch(systemValue[0])
	{
		case 'J':  //   january june july 
			if (systemValue[1] == 'a') return "January";
			else if (systemValue[2] == 'n') return "June";
			else if (systemValue[2] == 'l') return "July";
			break;
		case 'F': return "February";
		case 'M': return (systemValue[2] != 'y') ? (char*)"March" : (char*)"May"; 
  		case 'A': return (systemValue[1] == 'p') ? (char*)"April" : (char*)"August";
		case 'S': return "September";
		case 'O': return "October";
        case 'N': return "November";
        case 'D': return "December";
	}
	return "";
}

static char* Ssecond(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "12";
	struct tm ptm;
    ReadCompiledWord(GetTimeInfo(&ptm)+17,systemValue);
    systemValue[2] = 0;
    return systemValue;
}

static char* Svolleytime(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "12";
	uint64 diff = ElapsedMilliseconds() - volleyStartTime;
    sprintf(systemValue,(char*)"%u",(unsigned int)diff);
    return systemValue;
}

static char* Stime(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "01:40";
	struct tm ptm;
    strncpy(systemValue,GetTimeInfo(&ptm)+11,5);
    systemValue[5] = 0;
    return systemValue;
}

static char* Szulutime(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	struct tm ptm;
    GetTimeInfo(&ptm,true,true);
    uint64 elapsedMS = ElapsedMilliseconds();
	sprintf(systemValue,(char*)"%d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d.%3.3dZ",ptm.tm_year+1900,ptm.tm_mon+1,ptm.tm_mday,
		ptm.tm_hour,ptm.tm_min,ptm.tm_sec, (int)(elapsedMS%1000));
    return systemValue;
}

static char* Stimenumbers(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	struct tm ptm;
    GetTimeInfo(&ptm);
	sprintf(systemValue,(char*)"%2.2d %2.2d %2.2d %d %d %d %d",ptm.tm_sec,ptm.tm_min, ptm.tm_hour, ptm.tm_wday, ptm.tm_mday, ptm.tm_mon, ptm.tm_year+1900); 
    return systemValue;
}
static char* SweekOfMonth(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
    if (regression) return "1";
	int n;
	struct tm ptm;
	char* x = GetTimeInfo(&ptm) + 8;
	if (*x == ' ') ++x; // Mac uses space, pc uses 0 for 1 digit numbers 
    ReadInt(x,n);
	systemValue[0] = (char)('0' + (n/7) + 1);
	systemValue[1] = 0;
    return systemValue;
}

static char* Syear(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "2017";
	struct tm ptm;
    ReadCompiledWord(GetTimeInfo(&ptm)+20,systemValue);
    return (regression) ? (char*)"1951" : systemValue;
}

static char* Srand(char* value) // 1 .. 100
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (regression) return "0";
	sprintf(systemValue,(char*)"%u",random(100)+1);
	return systemValue;
}

////////////////////////////////////////////////////
/// SYSTEM 
////////////////////////////////////////////////////

static char* Sall(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	return (all != 0) ? (char*)"1" : (char*)"";
}

static char* Sscript(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (topicBlockPtrs[2]) sprintf(systemValue,(char*)"Script1: %s Script0: %s Script2: %s",timeStamp[1],timeStamp[0],timeStamp[2]);
	else sprintf(systemValue,(char*)"Script1: %s Script0: %s",timeStamp[1],timeStamp[0]);
    return systemValue;
}

static char* Sengine(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"Engine: %s",compileDate);
	if (systemValue[12] == ' ') systemValue[12] = '0';
    return systemValue;
}

static char* Sip(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue,ipAddress);
    return systemValue;
}

static char* Slanguage(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue,language);
    return systemValue;
}
static char* Sos(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
#ifdef WIN32
	strcpy(systemValue,(char*)"windows");
#elif  __MACH__
	strcpy(systemValue,(char*)"mac");
#elif IOS
	strcpy(systemValue,(char*)"ios");
#elif  ANDROID
	strcpy(systemValue,(char*)"android");
#else 
	strcpy(systemValue,(char*)"linux");
#endif

    return systemValue;
}

char* Slogging(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	sprintf(systemValue, "%d %d  %s ",serverLog, userLog, hostname);
	return  systemValue;
}

static char* Suser(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue,loginID);
    return systemValue;
}

static char* Sdbparams(char* value)
{
	static char hold[1000] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	return dbparams;
}

static char* Sbot(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue,computerID);
    return systemValue;
}

static char* Sbotid(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	sprintf(systemValue, "%u", (unsigned int)myBot);
	return systemValue;
}
static char* Sdict(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"Dictionary: %s",dictionaryTimeStamp);
    return systemValue;
}

static char* StableInput(char* value)
{
    static char hold[50] = ".";
    if (value) return AssignValue(hold, value);
    if (*hold != '.') return hold;
    return (tableinput) ? tableinput : (char*)"";
}

static char* SfactExhaustion(char* value)
{
    static char hold[50] = ".";
    if (value)
    {
        if (!stricmp(value, "false")) factsExhausted = false;
        else return AssignValue(hold, value);
    }
    if (*hold != '.') return hold;
    return (factsExhausted) ? (char*)"1" : (char*)"";
}

static char* StsvSource(char* value)
{
	static char hold[50] = ".";
	if (value)
	{
		return AssignValue(hold, value);
	}
	if (*hold != '.') return hold;
	return (tsvFile) ? (char*)"1" : (char*)"";
}

static char* Sversion(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"Version: %s",version);
    return systemValue;
}

#ifndef DISCARDJSONOPEN
static char* Scurlversion(char* value)
{
    static char hold[50] = ".";
    if (value) return AssignValue(hold,value);
    if (*hold != '.') return hold;
    sprintf(systemValue,(char*)"%s",CurlVersion());
    return systemValue;
}
#endif

static char* Sdbversion(char* value)
{
    static char hold[50] = ".";
    if (value) return AssignValue(hold,value);
    if (*hold != '.') return hold;
    sprintf(systemValue,(char*)"");
#ifndef DISCARDMYSQL
    strcat(systemValue,MySQLVersion());
    strcat(systemValue,(char*)" ");
#endif
#ifndef DISCARDMICROSOFTSQL
    strcat(systemValue,MsSqlVersion());
    strcat(systemValue,(char*)" ");
#endif
#ifndef DISCARDPOSTGRES
    strcat(systemValue,PostgresVersion());
    strcat(systemValue,(char*)" ");
#endif
#ifndef DISCARDMONGO
    strcat(systemValue,MongoVersion());
    strcat(systemValue,(char*)" ");
#endif
    return systemValue;
}

static char* Sfact(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%u",Fact2Index(lastFactUsed));
    return systemValue;
}

static char* Shost(char* value)
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value != '.') regression = *value != '0';
		return strcpy(hold,value);
	}
	if (*hold != '.') return hold;
	return hostname;
}

static char* Sregression(char* value)
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value != '.') regression = *value != '0';
		return strcpy(hold,value);
	}
	if (*hold != '.') return hold;
	return (regression != 0) ? (char*)"1" : (char*)"";
}

static char* ShttpResponse(char* value)
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value != '.') regression = *value != '0';
		return strcpy(hold,value);
	}
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%d",(int)http_response);
    return systemValue;
}

static char* Slastcurltime(char* value)
{
	static char hold[100] = ".";
	if (value) 
	{
		if (*value != '.') regression = *value != '0';
		return strcpy(hold,value);
	}
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%s",lastcurltime);
    return systemValue;
}

static char* SmaxMatchVariables(char* value)
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value != '.') regression = *value != '0';
		return strcpy(hold,value);
	}
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%d",MAX_WILDCARDS);
    return systemValue;
}

static char* SmaxFactSets(char* value)
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value != '.') regression = *value != '0';
		return strcpy(hold,value);
	}
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%d",MAX_FIND_SETS);
    return systemValue;
}

static char* Sspeaker(char* value)
{
	static char hold[4000];
	if (value)
	{
		size_t len = strlen(value);
		if (len >= 4000) value[len - 1] = 0;
		strcpy(hold, value);
	}
	return (*hold != '.') ? hold :  speaker;
}

static char* ScrossTalk(char* value)
{
	static char hold[4000];
	if (value) 
	{
		size_t len = strlen(value);
		if (len >= 4000) value[len-1] = 0;
		strcpy(hold,value); 
	}
	return (*hold != '.') ? hold :  (char*)"";
}

static char* ScrossTalk1(char* value)
{
	static char hold[4000];
	if (value)
	{
		size_t len = strlen(value);
		if (len >= 4000) value[len - 1] = 0;
		strcpy(hold, value);
	}
	return (*hold != '.') ? hold : (char*)"";
}

static char* ScrossTalk2(char* value)
{
	static char hold[4000];
	if (value)
	{
		size_t len = strlen(value);
		if (len >= 4000) value[len - 1] = 0;
		strcpy(hold, value);
	}
	return (*hold != '.') ? hold : (char*)"";
}

static char* ScrossTalk3(char* value)
{
	static char hold[4000];
	if (value)
	{
		size_t len = strlen(value);
		if (len >= 4000) value[len - 1] = 0;
		strcpy(hold, value);
	}
	return (*hold != '.') ? hold : (char*)"";
}

static char* Sdocument(char* value)
{
	static char hold[50] = ".";
	if (value) return strcpy(hold,value); // may not legall set on one's own
	if (*hold != '.') return hold;
	return (documentMode != 0) ? (char*)"1" : (char*)"";
}

static char* SfreeText(char* value)
{
	static char hold[50] = ".";
	if (value) return strcpy(hold,value); // may not legally set on one's own
	int nominalUsed = maxHeapBytes - (heapBase - heapFree);
	sprintf(hold,(char*)"%d",nominalUsed / 1000);
	return hold;
}

static char* SfreeWord(char* value)
{
	static char hold[50] = ".";
	if (value) return strcpy(hold,value); // may not legally set on one's own
	sprintf(hold,(char*)"%lu",((unsigned int)maxDictEntries)-(dictionaryFree-dictionaryBase));
	return hold;
}

static char* SfreeFact(char* value)
{
	static char hold[50] = ".";
	if (value) return strcpy(hold,value); // may not legally set on one's own
	sprintf(hold,(char*)"%ld",factEnd-lastFactUsed);
	return hold;
}

static char* Srule(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (currentTopicID == 0 || currentRuleID == -1) return "";
	sprintf(systemValue,(char*)"%s.%u.%u",GetTopicName(currentTopicID),TOPLEVELID(currentRuleID),REJOINDERID(currentRuleID));
    return systemValue;
}

static char* StestPattern(char* value)
{
    static char hold[50] = ".";
    if (value) return AssignValue(hold,value);
    if (*hold != '.') return hold;
    if (testPatternIndex == -1) return "";
    sprintf(systemValue,(char*)"%d",testPatternIndex);
    return systemValue;
}

static char* Sserver(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (!server) return "";

	sprintf(systemValue,(char*)"%u",port);
	return systemValue;
}

static char* SmyIP(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	return myip;
}

static char* Stopic(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	GetActiveTopicName(systemValue);
    return systemValue;
}

static char* SactualTopic(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue,GetTopicName(currentTopicID,false)); 
    return systemValue;
}

static char* STrace(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (!trace) return "0";
	sprintf(systemValue,(char*)"%u",trace);
	return systemValue;
}

static char* SPID(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
#ifdef LINUX
	sprintf(systemValue,(char*)"%d",getpid());
#elif __MACH__
    sprintf(systemValue,(char*)"%d",getpid());
#else
	sprintf(systemValue,(char*)"%d",0);
#endif
	return systemValue;
}

static char* SForkCount(char* value)
{
    static char hold[50] = ".";
    if (value) return AssignValue(hold,value);
    if (*hold != '.') return hold;
    if (!server) return (char*)"";
#ifdef EVSERVER_FORK
    sprintf(systemValue,(char*)"%d",forkcount);
#else
    sprintf(systemValue,(char*)"%d",0);
#endif
    return systemValue;
}

static char* SServerType(char* value)
{
    static char hold[50] = ".";
    if (value) return AssignValue(hold,value);
    if (*hold != '.') return hold;
    if (!server) return (char*)"";
#ifdef EVSERVER_FORK
    if (no_children_g > 0) return parent_g ? (char*)"parent" : (char*)"fork";
#endif
    return (char*)"server";
}

static char* SRestart(char* value)
{
	static char systemRestartValue[MAX_WORD_SIZE]; // restart passback
	static bool inited = false;
	if (!inited) 
	{
		inited = true;
		*systemRestartValue = 0;
	}

	if (value && *value == '.') return ""; // ignore init
	else if (value) 
	{
		if (strlen(value) >= MAX_WORD_SIZE) value[MAX_WORD_SIZE-1] = 0;
		strcpy(systemRestartValue,value);
		return "";
	}
	else return systemRestartValue;
}

static char* STimeout(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	return  timerCheckInstance == TIMEOUT_INSTANCE ? (char*)"1" : (char*)"";
}

static char* SHeapSize(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	int d = (heapFree - stackFree); 
	sprintf(systemValue,"%d", d);
	return  systemValue;
}

////////////////////////////////////////////////////
/// USER INPUT
////////////////////////////////////////////////////

static char* Sforeign(char* value)
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"1")) tokenFlags |= FOREIGN_TOKENS;
		else if (!stricmp(value,(char*)"0")) tokenFlags &= -1 ^ FOREIGN_TOKENS;
	}
	if (*hold != '.') return hold;
	return tokenFlags & FOREIGN_TOKENS ?  (char*)"1" : (char*)"";
}

static char* Sinput(char* value)
{
	static char hold[50] = ".";
	if (value)
	{  
		if (value[0] != '.') volleyCount = atoi(value); // actually changes it
		else strcpy(hold,value);
	}
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%u",volleyCount); 
	return systemValue;
}

static char* Slength(char* value) 
{
	static char hold[50] = ".";
	if (value)  return AssignValue(hold,value);
	if (*hold != '.') return hold;
 	sprintf(systemValue,(char*)"%d",wordCount); 
	return systemValue;
}

static char* Smore(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
    return moreToCome ? (char*)"1" : (char*)"";
}  

static char* SmoreQuestion(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
    return moreToComeQuestion ? (char*)"1" : (char*)"";
}   

static char* sentencehold = NULL;

char* SoriginalInput(char* value)
{
	if (csapicall == TEST_PATTERN) return testpatterninput;

	static char hold[50] = ".";
	if (value)
	{
		if (*value == '.' && !value[1]) sentencehold = NULL; // end use by testpattern
		else if (strlen(value) > 45)
		{
			sentencehold = value; // from testpattern
			return value;
		}
		return AssignValue(hold, value);
	}
	if (*hold != '.') return hold; // simple override
	if (sentencehold) return sentencehold;// testpattern override for long inputs
	char* input = SkipWhitespace(originalUserInput);
	while ((unsigned char)input[0] == 0xEF && (unsigned char)input[1] == 0xBB && (unsigned char)input[2] == 0xBF)
	{
		input += 3; // skip UTF8 BOM (from file)
	}
	char* at = SkipOOB(input);
    return SkipWhitespace(at);
}   

static char* SoriginalSentence(char* value)
{
	static char hold[50] = ".";
	if (value)
	{
		if (*value == '.' && !value[1]) sentencehold = NULL; // end use by testpattern
		else if (strlen(value) > 45) 
		{
			sentencehold = value; // from testpattern
			return value;
		}
		return AssignValue(hold, value);
	}
	if (*hold != '.') return hold; // simple override
	if (sentencehold) return sentencehold;// testpattern override for long inputs
    return rawSentenceCopy; // normal actual initial input
}   
static char* Sparsed(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
    return tokenFlags & FAULTY_PARSE ? (char*)"" : (char*)"1";
}  

static char* Ssentence(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
    return tokenFlags & NOT_SENTENCE ? (char*)"" : (char*)"1";
}  

static char* Squestion(char* value) 
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"1")) tokenFlags |= QUESTIONMARK;
		else if (!stricmp(value,(char*)"0")) tokenFlags &= -1 ^ QUESTIONMARK;
	}
	if (*hold != '.') return hold;
    return tokenFlags & QUESTIONMARK ? (char*)"1" : (char*)"";
}  

static char* Scommand(char* value) 
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"1")) tokenFlags |= COMMANDMARK;
		else if (!stricmp(value,(char*)"0")) tokenFlags &= -1 ^ COMMANDMARK;
	}
	if (*hold != '.') return hold;
    return tokenFlags & COMMANDMARK ? (char*)"1" : (char*)"";
}  

static char* Squotation(char* value) 
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"1")) tokenFlags |= QUOTATION;
		else if (!stricmp(value,(char*)"0")) tokenFlags &= -1 ^ QUOTATION;
	}
	if (*hold != '.') return hold;
    return tokenFlags & QUOTATION ? (char*)"1" : (char*)"";
}  

static char* Simpliedyou(char* value) 
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"1")) tokenFlags |= IMPLIED_YOU;
		else if (!stricmp(value,(char*)"0")) tokenFlags &= -1 ^ IMPLIED_YOU;
	}
	if (*hold != '.') return hold;
    else return tokenFlags & IMPLIED_YOU ? (char*)"1" : (char*)"";
}  

static char* SimpliedSubject(char* value)
{
	static char hold[50] = ".";
	if (value)
	{
		if (*value == '.') strcpy(hold, value);
		else if (!stricmp(value, (char*)"1")) tokenFlags |= IMPLIED_SUBJECT;
		else if (!stricmp(value, (char*)"0")) tokenFlags &= -1 ^ IMPLIED_SUBJECT;
	}
	if (*hold != '.') return hold;
	else return tokenFlags & IMPLIED_SUBJECT ? (char*)"1" : (char*)"";
}
static char* Stense(char* value) 
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"past")) {tokenFlags &= -1 & (PRESENT|FUTURE); tokenFlags |= PAST;}
		else if (!stricmp(value,(char*)"future")) {tokenFlags &= -1 & (PRESENT|PAST); tokenFlags |= FUTURE;}
		else if (!stricmp(value,(char*)"present"))  {tokenFlags &= -1 & (FUTURE|PAST); tokenFlags |= PRESENT;}
	}
	if (*hold != '.') return hold;
	else if (tokenFlags & PAST) return "past";
	else if (tokenFlags & FUTURE) return "future";
	else return "present";
}

static char* SexternalTagging(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
#ifdef TREETAGGER
	return externalPostagger ? (char*)"1" : (char*)"";
#else
	return "";
#endif
}

static char* StokenFlags(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
#ifdef WIN32
	sprintf(systemValue,(char*)"%I64d",(long long int) tokenFlags); 
#else
	sprintf(systemValue,(char*)"%lld",(long long int) tokenFlags); 
#endif	

	return systemValue;
}

static char* SuserFirstLine(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%u",userFirstLine); 
	return systemValue;
}

static char* SuserInput(char* value) 
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"1")) tokenFlags |= USERINPUT;
		else if (!stricmp(value,(char*)"0")) tokenFlags &= -1 ^ USERINPUT;
	}
	if (*hold != '.') return hold;
	return tokenFlags & USERINPUT ? (char*)"1" : (char*)"";
}   

static char* SrevisedInput(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	return inputNest ? (char*)"1" : (char*)"";
}   

static char* Svoice(char* value) 
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') strcpy(hold,value);
		else if (!stricmp(value,(char*)"1")) tokenFlags |= PASSIVE;
		else if (!stricmp(value,(char*)"0")) tokenFlags &= -1 ^ PASSIVE;
	}
	if (*hold != '.') return hold;
	return (tokenFlags & PASSIVE) ? (char*)"passive" : (char*)"active";
}

static char* SinputSize(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	sprintf(systemValue, "%d", inputSize);
	return systemValue;
}

static char* SinputLimited(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	return (inputLimitHit) ? (char*)"1" : (char*)"";
}

static char* SbadSpell(char* value)
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold, value);
	if (*hold != '.') return hold;
	int bad = badspellcount >> 16;
	int size = badspellcount & 0x00ff;
	sprintf(systemValue, (char*)"%d-%d", bad,size);
	return systemValue;
}

////////////////////////////////////////////////////
/// OUTPUT VARIABLES
////////////////////////////////////////////////////

static char* SinputRejoinder(char* value)
{ 
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') return AssignValue(hold,value);
		char* dot = strchr(value,'.');
		if (dot)
		{
			*dot = 0;
			inputRejoinderTopic = FindTopicIDByName(value);
			inputRejoinderRuleID = atoi(dot+1);
			*dot = '.';
			dot = strchr(dot+1,'.');
			if (dot) inputRejoinderRuleID |= MAKE_REJOINDERID(atoi(dot+1));
		}
		return value;
	}
	if (*hold != '.') return hold;
	if (inputRejoinderTopic == NO_REJOINDER) return (char*)"";
	sprintf(systemValue,(char*)"%s.%u.%u",GetTopicName(inputRejoinderTopic),TOPLEVELID(inputRejoinderRuleID),REJOINDERID(inputRejoinderRuleID)); 
	return systemValue;
}

static char* SlastOutput(char* value) 
{
	static char hold[500];
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	return (responseIndex) ? responseData[responseOrder[responseIndex-1]].response : (char*)"";
}

static char* SlastQuestion(char* value) 
{
	static char hold[50] = ".";
 	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	if (!responseIndex) return "";
	char* sentence = responseData[responseOrder[responseIndex-1]].response;
	return (strchr(sentence, '?')) ? (char*)"1" : (char*)"";
}

static char* SoutputRejoinder(char* value)
{
	static char hold[50] = ".";
	if (value) 
	{
		if (*value == '.') return AssignValue(hold,value);
		char* dot = strchr(value,'.');
		if (dot)
		{
			*dot = 0;
			outputRejoinderTopic = FindTopicIDByName(value);
			outputRejoinderRuleID = atoi(dot+1);
			*dot = '.';
			dot = strchr(dot+1,'.');
			if (dot) outputRejoinderRuleID |= MAKE_REJOINDERID(atoi(dot+1));
		}
		return value;
	}
	if (*hold != '.') return hold;
	if (outputRejoinderTopic == NO_REJOINDER) return (char*)"";
	sprintf(systemValue,(char*)"%s.%u.%u",GetTopicName(outputRejoinderTopic),TOPLEVELID(outputRejoinderRuleID),REJOINDERID(outputRejoinderRuleID)); 
	return systemValue;
}

static char* Sresponse(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	sprintf(systemValue,(char*)"%d",responseIndex);
	return systemValue;
}   

static char* Sserverlogfolder(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue, logsfolder);
    return systemValue;
}

static char* Suserlogfolder(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue, usersfolder);
    return systemValue;
}

static char* Stmpfolder(char* value) 
{
	static char hold[50] = ".";
	if (value) return AssignValue(hold,value);
	if (*hold != '.') return hold;
	strcpy(systemValue, tmpfolder);
    return systemValue;
}

SYSTEMVARIABLE sysvars[] =
{ // do not use underscores in name
	{ (char*)"",0,(char*)""},

	{ (char*)"\r\n---- Time, Date, Number variables",0,(char*)""},
	{ (char*)"%date",Sdate,(char*)"Numeric day of the month"}, 
	{ (char*)"%day",SdayOfWeek,(char*)"Named day of the week"}, 
	{ (char*)"%daynumber",SdayNumberOfWeek,(char*)"Numeric day of week (0=sunday)"},  
	{ (char*)"%fulltime",SFullTime,(char*)"Numeric full time/date in seconds"}, 
	{ (char*)"%startmstime",SStartTimeMS,(char*)"Start of user request time/date in milliseconds" },
	{ (char*)"%fullmstime",SFullTimeMS,(char*)"Numeric full time/date in milliseconds" },
    { (char*)"%hour",Shour,(char*)"Numeric 2-digit current hour of day (00..23)"},
	{ (char*)"%leapyear",SleapYear,(char*)"Boolean - is it a leap year"}, 
	{ (char*)"%minute",Sminute,(char*)"Numeric 2-digit current minute"}, 
	{ (char*)"%month",Smonth,(char*)"Numeric month number (1..12)"},
	{ (char*)"%monthname",SmonthName,(char*)"Name of month"}, 
	{ (char*)"%rand",Srand,(char*)"Numeric random number (1..100)"}, 
	{ (char*)"%daylightsavings",Sdaylightsavings,(char*)"Boolean is daylight savings in effect"}, 
	{ (char*)"%second",Ssecond,(char*)"Numeric 2-digit current second"}, 
	{ (char*)"%time",Stime,(char*)"Current military time (e.g., 21:07)"}, 
	{ (char*)"%zulutime",Szulutime,(char*)"Time as: 2016-07-27T11:38:35.253Z"}, 
	{ (char*)"%timenumbers",Stimenumbers,(char*)"numbers, separated by blanks, of sec,min,hr,dayinweek,dayinmonth,month,year"}, 
	{ (char*)"%week",SweekOfMonth,(char*)"Numeric week of month (1..5)"}, 
	{ (char*)"%volleytime",Svolleytime,(char*)"Numeric milliseconds since volley start"}, 
	{ (char*)"%year",Syear,(char*)"Numeric current 4-digit year"},
	
	{ (char*)"\r\n---- System variables",0,(char*)""},
	{ (char*)"%all",Sall,(char*)"Boolean - is all flag on"}, 
	{ (char*)"%speaker",Sspeaker,(char*)"speaker variable from :tsvsource"},
	{ (char*)"%crosstalk",ScrossTalk,(char*)"cross bot/cross document variable storage"},
	{ (char*)"%crosstalk1",ScrossTalk1,(char*)"additional cross bot/cross document variable storage" },
	{ (char*)"%crosstalk2",ScrossTalk2,(char*)"cross bot/cross document variable storage"},
	{ (char*)"%crosstalk3",ScrossTalk3,(char*)"additional cross bot/cross document variable storage" },
#ifndef DISCARDJSONOPEN
    { (char*)"%curlversion",Scurlversion,(char*)"Curl version information"},
#endif
    { (char*)"%dbversion",Sdbversion,(char*)"Database version information"},
	{ (char*)"%document",Sdocument,(char*)"Boolean - is :document flag on"},
	{ (char*)"%fact",Sfact,(char*)"Most recent fact id"}, 
	{ (char*)"%tsvsource",StsvSource,(char*)"Boolean Inputting tsvsource file?"},
	{ (char*)"%freetext",SfreeText,(char*)"Kbytes of available text space"}, 
	{ (char*)"%freeword",SfreeWord,(char*)"number of available unused words"}, 
	{ (char*)"%freefact",SfreeFact,(char*)"number of available unused facts"}, 
	{ (char*)"%regression",Sregression,(char*)"Boolean - is regression flag on"}, 
	{ (char*)"%host",Shost,(char*)"machine ip if a server, else local"}, 
	{ (char*)"%httpresponse",ShttpResponse,(char*)"http response code from last call to JsonOpen"}, 
	{ (char*)"%lastcurltime",Slastcurltime,(char*)"http time taken for last call to JsonOpen"}, 
	{ (char*)"%maxmatchvariables",SmaxMatchVariables,(char*)"highest number of legal _match variables"}, 
	{ (char*)"%maxfactsets",SmaxFactSets,(char*)"highest number of legal @factsets"}, 
	{ (char*)"%rule",Srule,(char*)"Get a tag to current executing rule or null"}, 
	{ (char*)"%server",Sserver,(char*)"Port id of server or null if not server"}, 
	{ (char*)"%myip",SmyIP,(char*)"get ip of this server" },
	{ (char*)"%actualtopic",SactualTopic,(char*)"Current  topic executing (including system or nostay)"},
	{ (char*)"%topic",Stopic,(char*)"Current interesting topic executing (not system or nostay)"}, 
	{ (char*)"%trace",STrace,(char*)"Numeric value of trace flag"}, 
	{ (char*)"%pid",SPID,(char*)"Process id of this instance (linux)"}, 
    { (char*)"%forkCount",SForkCount,(char*)"Returns the number of forks"},
    { (char*)"%serverType",SServerType,(char*)"Returns parent, fork, server. server being this is a server environment where there are no child forks" },
	{ (char*)"%restart",SRestart,(char*)"pass string back to a restart"},
    { (char*)"%testpattern",StestPattern,(char*)"The index number of current pattern being matched in ^testpattern"},
    { (char*)"%timeout",STimeout,(char*)"did system time out happen" },
	{ (char*)"%heapsize",SHeapSize,(char*)"bytes of heap/stack left" },

	{ (char*)"\r\n---- Build variables",0,(char*)""},
    { (char*)"%tableinput",StableInput,(char*)"Current input line of table processing" },
    { (char*)"%dict",Sdict,(char*)"String - when dictionary was built"},
	{ (char*)"%engine",Sengine,(char*)"String - when engine was compiled (date/time)"}, 
	{ (char*)"%os",Sos,(char*)"String - os involved: linux windows mac ios"}, 
	{ (char*)"%script",Sscript,(char*)"String - when build1 and build0 were compiled)"}, 
	{ (char*)"%version",Sversion,(char*)"String - engine version number"}, 
    { (char*)"%factexhaustion",SfactExhaustion,(char*)"did fact space run out 1 - 0 " },

	{ (char*)"\r\n---- Input variables",0,(char*)""},
	{ (char*)"%dbparams",Sdbparams,(char*)"String - db params used for filesystem db use"},
	{ (char*)"%bot",Sbot,(char*)"String - bot in use"},
	{ (char*)"%botid",Sbotid,(char*)"String - bot id number in use"},
	{ (char*)"%command",Scommand,(char*)"Boolean - is the current input a command"},
	{ (char*)"%foreign",Sforeign,(char*)"Boolean - is the bulk of current input foreign words"},
	{ (char*)"%impliedyou",Simpliedyou,(char*)"Boolean - is the current input have you as an implied subject"},
	{ (char*)"%impliedsubject",SimpliedSubject,(char*)"Boolean - is the current input have I or most recent subject as an implied subject" },
	{ (char*)"%input",Sinput,(char*)"Numeric volley id of the current input"},
	{ (char*)"%volley",Sinput,(char*)"(same as %input) Numeric volley id of the current input"},
	{ (char*)"%ip",Sip,(char*)"String - ip address supplied"},
	{ (char*)"%language",Slanguage,(char*)"what language is enabled"},
	{ (char*)"%length",Slength,(char*)"Numeric count of words of current input"}, 
	{ (char*)"%login",Suser,(char*)"String - user login name supplied - same as %user"}, 
	{ (char*)"%logging",Slogging,(char*)"String - status of logging"},
	{ (char*)"%more",Smore,(char*)"Boolean - is there more input pending"},
	{ (char*)"%morequestion",SmoreQuestion,(char*)"Boolean - is there a ? in pending input"}, 
	{ (char*)"%originalinput",SoriginalInput,(char*)"returns the raw content of what user sent in"}, 
	{ (char*)"%originalsentence",SoriginalSentence,(char*)"returns the raw content of the current sentence"}, 
	{ (char*)"%parsed",Sparsed,(char*)"Boolean - was current input successfully parsed"}, 
	{ (char*)"%question",Squestion,(char*)"Boolean - is the current input a question"},
	{ (char*)"%quotation",Squotation,(char*)"Boolean - is the current input a quotation"},
	{ (char*)"%sentence",Ssentence,(char*)"Boolean - does it seem like a sentence - has subject and verb or is command"}, 
	{ (char*)"%tense",Stense,(char*)"Tense of current input (present, past, future)"}, 
	{ (char*)"%externalTagging",SexternalTagging,(char*)"Boolean - is external pos-tagging enabled" },
	{ (char*)"%tokenflags",StokenFlags,(char*)"Numeric value of all tokenflags"},
	{ (char*)"%user",Suser,(char*)"String - user login name suppled"}, 
	{ (char*)"%userfirstline",SuserFirstLine,(char*)"Numeric volley count at start of session"}, 
	{ (char*)"%userinput",SuserInput,(char*)"Boolean - is input coming from user"}, 
	{ (char*)"%revisedInput",SrevisedInput,(char*)"Boolean - is input coming from ^input"}, 
	{ (char*)"%voice",Svoice,(char*)"Voice of current input (active,passive)"}, 
	{ (char*)"%badspell",SbadSpell,(char*)"How many bad spellings were handled" },
	{ (char*)"%inputlimited",SinputLimited,(char*)"were we given too much input" },
	{ (char*)"%inputsize",SinputSize,(char*)"bytes of input passed in" },
	{ (char*)"\r\n---- Output variables",0,(char*)""},
	{ (char*)"%inputrejoinder",SinputRejoinder,(char*)"if pending input rejoinder, this is the tag of it else null"},
	{ (char*)"%lastoutput",SlastOutput,(char*)"Last line of currently generated output or null"},
	{ (char*)"%lastquestion",SlastQuestion,(char*)"Boolean - did last output end in a ?"}, 
	{ (char*)"%outputrejoinder",SoutputRejoinder,(char*)"tag of current output rejoinder or null"}, 
	{ (char*)"%response",Sresponse,(char*)"Numeric count of responses generated for current volley"}, 
	{ (char*)"%serverlogfolder",Sserverlogfolder,(char*)"server logs folder"}, 
	{ (char*)"%userlogfolder",Suserlogfolder,(char*)"user logs folder"}, 
	{ (char*)"%tmpfolder",Stmpfolder,(char*)"TMP folder"}, 
	
	{NULL,NULL,(char*)""},
};
