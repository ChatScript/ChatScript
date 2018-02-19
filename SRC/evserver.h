#ifdef INFORMATION
Copyright (C) 2011-2016by Outfit7

Released under Bruce Wilcox License as follows:

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#ifndef DISCARDSERVER
#ifdef EVSERVER
#ifndef __EVSERVER_H__
#define __EVSERVER_H__

#ifdef WIN32
#pragma warning(push,1)
#pragma warning(disable: 4290) 
              typedef char raw_type;       
              typedef int socklen_t;
#include <winsock.h>        
#else
              typedef void raw_type;    
#include <arpa/inet.h>      
#include <netdb.h>         
#include <netinet/in.h>     
#include <sys/socket.h>      
#include <sys/types.h>      
#include <unistd.h>         
#include <netinet/tcp.h>
#endif

// evsrv stuff
int evsrv_init(const string &interfaceKind, int port, char *arg);
int evsrv_run();

#ifdef WIN32
#pragma warning( pop )
#endif

#endif /* __EVSERVER_H__ */
#endif /* EVSERVER */
#endif 
