#ifndef _MAXH_
#define _MAXH_
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

#ifdef WIN32
#pragma warning( disable :  4206 4456 4457 4459  4505 4611) 
#pragma warning( disable :  26481 26462 26446 26494 0260 26485) 
#endif

// These can be used to shed components of the system to save space
//#define DISCARDSERVER 1
//#define DISCARDSCRIPTCOMPILER 1
//#define DISCARDPARSER 1
//#define DISCARDTESTING 1
//#define DISCARDTCPOPEN 1
//#define DISCARDMYSQL 1
//#define DISCARDPOSTGRES 1
//#define DISCARDMONGO 1
//#define DISCARDMICROSOFTSQL 1
//#define DISCARDJSONOPEN 1 
//#define DISCARDJAVASCRIPT 1
//#define DISCARD_TEXT_COMPRESSION 1
//#define DISCARDWEBSOCKET 1
//#define DISCARD_JAPANESE 1
#define DISCARD_PYTHON 1

// these can add components
//#define  TREETAGGER 1
//#define HARDCRASH 1 - dont trap interrupt (for debugging)

#ifdef DLL
#define NOMAIN 1
#define DISCARD_TEXT_COMPRESSION 1
#ifndef JA
#define DISCARDMICROSOFTSQL 1
#endif
#endif

#ifdef  WIN32
//#define USERPATHPREFIX 1

#elif IOS
#define DISCARDCOUNTER 1
#define DISCARDMYSQL 1
#define DISCARDMICROSOFTSQL 1
#define DISCARD_TEXT_COMPRESSION 1
#define DISCARDPOSTGRES 1
#define DISCARDMONGO 1
#define DISCARDCOUNTER 1
#define DISCARDCLIENT 1
#define DISCARDJSONOPEN 1
#define DISCARDWEBSOCKET 1
#define DISCARD_JAPANESE 1

#elif ANDROID
#define DISCARDCOUNTER 1
#define DISCARDMYSQL 1
#define DISCARDMICROSOFTSQL 1
#define DISCARD_TEXT_COMPRESSION 1
#define DISCARDPOSTGRES 1
#define DISCARDMONGO 1
#define DISCARDCOUNTER 1
#define DISCARDCLIENT 1
#define DISCARDJSONOPEN 1
#define DISCARDWEBSOCKET 1
#define DISCARD_JAPANESE 1

#elif __MACH__
#define DISCARDPOSTGRES 1
#define DISCARDMICROSOFTSQL 1
#define DISCARDMYSQL 1
#define DISCARDWEBSOCKET 1
#define DISCARDMONGO 1
#define DISCARD_TEXT_COMPRESSION 1
#else // GENERIC LINUX
#ifndef EVSERVER
#define SAFETIME 1  // protect server from time concurrency issues in C++ library.
#endif
#endif

// These can be used to include LINUX EVSERVER component - this is automatically done by the makefile in src - EV Server does not work under windows
// #define EVSERVER 1

// These can be used to embed chatscript within another application (then you must call InitSystem on startup yourself)
// #define NOMAIN 1

#ifndef WIN32
	typedef long long long_t; 
	typedef unsigned long long ulong_t; 
	#include <sys/time.h> 
	#include <termios.h> 
	#include <unistd.h>
	#define stricmp strcasecmp 
	#define strnicmp strncasecmp 
#else
	typedef long long_t;       
	typedef unsigned long ulong_t; 
#endif

#ifdef WIN32
	#pragma comment(lib, "ws2_32.lib") 
	#pragma warning( disable : 4100 ) 
	#pragma warning( disable : 4189 ) 
	#pragma warning( disable : 4290 ) 
	#pragma warning( disable : 4706 )
	#pragma warning( disable : 4996 ) 

	#define _CRT_SECURE_NO_DEPRECATE
	#define _CRT_SECURE_NO_WARNINGS 1
	#define _CRT_NONSTDC_NO_DEPRECATE
	#define _CRT_NONSTDC_NO_WARNINGS 1
#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
	#include <winsock2.h> // needs to compile before any refs to winsock.h
	#include <ws2tcpip.h>
	#include <iphlpapi.h>
	#pragma comment(lib, "Ws2_32.lib")
	#pragma comment (lib, "Mswsock.lib")
	#pragma comment (lib, "AdvApi32.lib")
#elif IOS
	#include <dirent.h>
	#include <mach/clock.h>
	#include <mach/mach.h>
#elif __MACH__
	#include <dirent.h>
	#include <mach/clock.h>
	#include <mach/mach.h>
#ifndef  HARDCRASH
#define USESIGNALHANDLER 1
#endif
#else  // LINUX
	#include <dirent.h>
	#ifndef LINUX
		#define LINUX 1
	#endif
#ifndef  HARDCRASH
    #define USESIGNALHANDLER 1
#endif
#endif

#ifdef IOS
#elif __MACH__
#include <sys/malloc.h>
#elif FREEBSD
// avoid requesting malloc.h
#else
#include <malloc.h>
#endif

#ifndef DISCARD_PYTHON
#define PY_SSIZE_T_CLEAN
#include "Python/include/Python.h"
#endif

#include <algorithm>
#include <assert.h>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <ctype.h>
#include <errno.h>
#include <exception>  
#include <fcntl.h>
#include <iostream>
#include <math.h>
#include <memory.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <string>  
#include <string.h>
#include <sys/stat.h>
#include <sys/timeb.h> 
#include <sys/types.h>
#include <time.h>
#include <utility>
#include <vector>

#ifndef DISCARDMONGO
#include "bson.h"
#endif

using namespace std;

#ifdef EVSERVER
	#define EV_STANDALONE 1
	#define EV_CHILD_ENABLE 1
	#ifdef DISCARDMONGO
		#ifdef DISCARDPOSTGRES
			#define LOCKUSERFILE 1		// protect from multiple servers hitting same file
			//#define USERPATHPREFIX 1	// do binary tree of log file - for high speed servers recommend userlog, no server log and this (large server logs are a pain to handle)
		#endif
	#endif
#endif

#ifndef DISCARDJSONOPEN
#ifdef WIN32
#include "curl.h"
#ifdef DEBUG
#pragma comment(lib, "../SRC/curl/libcurld.lib")
#else
#pragma comment(lib, "../SRC/curl/libcurl.lib")
#endif
#else
#include <curl/curl.h>
#endif
#endif

#include "common1.h"

#include "os.h"
#include "dictionarySystem.h"
#include "mainSystem.h"
#include "factSystem.h"
#include "functionExecute.h"

#include "cs_jp.h"
#include "cs_es.h"
#include "cs_german.h"
#include "csocket.h"
#include "constructCode.h"
#include "english.h"
#include "infer.h"
#include "json.h"
#include "markSystem.h"
#include "mongodb.h"
#include "my_sql.h"
#include "ms_sql.h"
#include "outputSystem.h"
#include "patternSystem.h"
#include "postgres.h"
#include "scriptCompile.h"
#include "spellcheck.h"
#include "systemVariables.h"
#include "tagger.h"
#include "testing.h"
#include "textUtilities.h"
#include "tokenSystem.h"
#include "topicSystem.h"
#include "userCache.h"
#include "userSystem.h"
#include "variableSystem.h"
#ifndef DISCARDWEBSOCKET
	#include "easywclient/easywsclient.hpp"
#endif

#ifdef PRIVATE_CODE
#include "../privatecode/privatesrc.h"
#endif 

#ifdef WIN32
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #include <fcntl.h>
    #define GetCurrentDir getcwd
#endif

#endif

#ifdef USESIGNALHANDLER
	// headers for error handling
	#include <signal.h>
	#include <execinfo.h>
#endif
