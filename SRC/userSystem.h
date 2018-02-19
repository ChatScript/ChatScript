#ifndef _USERSYSTEMH
#define _USERSYSTEMH
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

#define MAX_USED 30 // we want to track 20, keeping 10 available for new input from user

#define SAID_LIMIT 1000
extern char chatbotSaid[MAX_USED+1][SAID_LIMIT+3]; // last n messages sent to user 
extern char humanSaid[MAX_USED+1][SAID_LIMIT+3]; //   last n messages read from human
extern int humanSaidIndex;
extern int chatbotSaidIndex;
extern char timeturn15[100];
extern char ipAddress[ID_SIZE];
extern char timeturn0[20];
extern char timePrior[20];
extern int userFirstLine;
extern bool stopUserWrite;
extern uint64 setControl;
extern unsigned int userFactCount;

// login data
extern char computerID[ID_SIZE];
extern char computerIDwSpace[ID_SIZE];
extern char loginID[ID_SIZE];	// lower case form of loginName
extern char loginName[ID_SIZE]; // as user typed it
extern char callerIP[ID_SIZE];

extern bool serverRetryOK;

// process user
void ReadNewUser();
void ReadUserData();
void WriteUserData(time_t curr,bool nobackup);
char* WriteUserVariables(char* ptr,bool sharefile, bool compiling,char* saveJSON);
void RecoverUser();
void CopyUserTopicFile(char* newname);
// login
void ReadComputerID();
void Login(char* ptr,char* callee,char* ip);
void PartialLogin(char* caller,char* ip);
void ResetUserChat();
void KillShare();
#endif
