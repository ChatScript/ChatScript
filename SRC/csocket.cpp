// csocket.cpp - handles client/server behaviors  (not needed in a product)

/*
 *   C++ sockets on Unix and Windows Copyright (C) 2002 --- see HEADER file
 */

#ifdef INFORMATION
We have these threads:

1. HandleTCPClient which runs on a thread to service a particular client (as many threads as clients)
2. AcceptSockets, which is a simnple thread waiting for user connections
3. ChatbotServer, which is the sole server to be the chatbot
4. main thread, which is a simple thread that creates ChatbotServer threads as needed (if ones crash in LINUX, it can spawn a new one)

chatLock- either chatbot engine or a client controls it. It is how a client gets started and then the server is summoned

While processing chat, the client holds the clientlock and the server holds the chatlock and donelock.
The client is waiting for the donelock.
When the server finishes, he releases the donelock and waits for the clientlock.
If the client exists, server gets the donelock, then releases it and waits for the clientlock.
If the client doesnt exist anymore, he has released clientlock.

It releases the clientlock and the chatlock and tries to reacquire chatlock with flag on.
when it succeeds, it has a new client.

We have these variables :
chatbotExists - the chatbot engine is now initialized.
serverFinishedBy is what time the answer must be delivered (1 second before the memory will disappear)

#endif

#include "common.h"

bool echoServer = false;

char serverIP[100];
#ifndef DISCARDSERVER

#ifdef EVSERVER
#include "cs_ev.h"
#include "evserver.h"
#endif

#ifdef WIN32
//#define WEBSOCKET 1
#include "easywclient/easywsclient.hpp"
#include "easywclient/easywsclient.cpp" 
#endif

#define DOSOCKETS 1
#endif
#ifndef DISCARDCLIENT
#define DOSOCKETS 1
#endif
#ifndef DISCARDTCPOPEN
#define DOSOCKETS 1
#endif

#ifdef DOSOCKETS

// CODE below here is from PRACTICAL SOCKETS, needed for either a server or a client

//   SocketException Code

SocketException::SocketException(const string &message, bool inclSysMsg)
  throw() : userMessage(message) {
  if (inclSysMsg) {
    userMessage.append((char*)": ");
    userMessage.append(strerror(errno));
  }
}

SocketException::~SocketException() throw() {}
const char *SocketException::what() const throw() { return userMessage.c_str();}

//   Function to fill in address structure given an address and port
static void fillAddr(const string &address, unsigned short port, sockaddr_in &addr) {
  memset(&addr, 0, sizeof(addr));  //   Zero out address structure
  addr.sin_family = AF_INET;       //   Internet address
  hostent *host;  //   Resolve name
  if ((host = gethostbyname(address.c_str())) == NULL)  throw SocketException((char*)"Failed to resolve name (gethostbyname())");
  strcpy(hostname,host->h_name);
  addr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
  addr.sin_port = htons(port);     //   Assign port in network byte order
}

//   Socket Code
#define MAKEWORDX(a, b)      ((unsigned short)(((BYTE)(((DWORD_PTR)(a)) & 0xff)) | (((unsigned short)((BYTE)(((DWORD_PTR)(b)) & 0xff))) << 8)))

CSocket::CSocket(int type, int protocol) throw(SocketException) {
  #ifdef WIN32
	if (InitWinsock() == FAILRULE_BIT)  throw SocketException((char*)"Unable to load WinSock DLL");  //   Load WinSock DLL
  #endif

  //   Make a new socket
  if ((sockDesc = socket(PF_INET, type, protocol)) < 0)  throw SocketException((char*)"Socket creation failed (socket())", true);
}

CSocket::CSocket(int sockDesc) {this->sockDesc = sockDesc;}

CSocket::~CSocket() {
  #ifdef WIN32
    ::closesocket(sockDesc);
  #else
    ::close(sockDesc);
  #endif
  sockDesc = -1;
}

string CSocket::getLocalAddress() throw(SocketException) {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0)  throw SocketException((char*)"Fetch of local address failed (getsockname())", true);
  return inet_ntoa(addr.sin_addr);
}

unsigned short CSocket::getLocalPort() throw(SocketException) {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getsockname(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0)  throw SocketException((char*)"Fetch of local port failed (getsockname())", true);
  return ntohs(addr.sin_port);
}

void CSocket::setLocalPort(unsigned short localPort) throw(SocketException) {
#ifdef WIN32
	if (InitWinsock() == FAILRULE_BIT) throw SocketException((char*)"Unable to load WinSock DLL");
#endif

	int on = 1;
	int off = 0;
#ifdef WIN32
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (char*) &off, sizeof(on));
#else
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (void*) &off, sizeof(on));
#endif
	
	//   Bind the socket to its port
	sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(localPort);
	if (::bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) throw SocketException((char*)"Set of local port failed (bind())", true);
}

void CSocket::setLocalAddressAndPort(const string &localAddress,
    unsigned short localPort) throw(SocketException) {
	//   Get the address of the requested host
	sockaddr_in localAddr;
	fillAddr(localAddress, localPort, localAddr);
	int on = 1;
	int off = 0;
#ifdef WIN32
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (char*) &off, sizeof(on));
#else
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (void*) &off, sizeof(on));
#endif
	if (::bind(sockDesc, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0)  throw SocketException((char*)"Set of local address and port failed (bind())", true);
}

void CSocket::cleanUp() throw(SocketException) {
  #ifdef WIN32
    if (WSACleanup() != 0)  throw SocketException((char*)"WSACleanup() failed");
  #endif
}

unsigned short CSocket::resolveService(const string &service,
                                      const string &protocol) {
  struct servent *serv;        /* Structure containing service information */
  if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL) return (unsigned short) atoi(service.c_str());  /* Service is port number */
  else return ntohs(serv->s_port);    /* Found port (network byte order) by name */
}

//   CommunicatingSocket Code

CommunicatingSocket::CommunicatingSocket(int type, int protocol)  throw(SocketException) : CSocket(type, protocol) {
}

CommunicatingSocket::CommunicatingSocket(int newConnSD) : CSocket(newConnSD) {
}

void CommunicatingSocket::connect(const string &foreignAddress,unsigned short foreignPort) throw(SocketException) {
  //   Get the address of the requested host
  sockaddr_in destAddr;
  fillAddr(foreignAddress, foreignPort, destAddr);

  //   Try to connect to the given port
  if (::connect(sockDesc, (sockaddr *) &destAddr, sizeof(destAddr)) < 0)   throw SocketException((char*)"Connect failed (connect())", true);
}

void CommunicatingSocket::send(const void *buffer, int bufferLen) throw(SocketException) {
  if (::send(sockDesc, (raw_type *) buffer, bufferLen, 0) < 0)  throw SocketException((char*)"Send failed (send())", true);
}

int CommunicatingSocket::recv(void *buffer, int bufferLen) throw(SocketException) {
  int rtn;
  if ((rtn = ::recv(sockDesc, (raw_type *) buffer, bufferLen, 0)) < 0)  throw SocketException((char*)"Received failed (recv())", true);
  return rtn;
}

string CommunicatingSocket::getForeignAddress()  throw(SocketException) {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getpeername(sockDesc, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0)  throw SocketException((char*)"Fetch of foreign address failed (getpeername())", true);
  return inet_ntoa(addr.sin_addr);
}

unsigned short CommunicatingSocket::getForeignPort() throw(SocketException) {
  sockaddr_in addr;
  unsigned int addr_len = sizeof(addr);
  if (getpeername(sockDesc, (sockaddr *) &addr, (socklen_t *) &addr_len) < 0)  throw SocketException((char*)"Fetch of foreign port failed (getpeername())", true);
  return ntohs(addr.sin_port);
}

//   TCPSocket Code

TCPSocket::TCPSocket() throw(SocketException) : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {
}

TCPSocket::TCPSocket(const string &foreignAddress, unsigned short foreignPort) throw(SocketException) : CommunicatingSocket(SOCK_STREAM, IPPROTO_TCP) {
  connect(foreignAddress, foreignPort);
}

TCPSocket::TCPSocket(int newConnSD) : CommunicatingSocket(newConnSD) {
}

//   TCPServerSocket Code

TCPServerSocket::TCPServerSocket(unsigned short localPort, int queueLen) throw(SocketException) : CSocket(SOCK_STREAM, IPPROTO_TCP) {
  setLocalPort(localPort);
  setListen(queueLen);
}

TCPServerSocket::TCPServerSocket(const string &localAddress, unsigned short localPort, int queueLen) throw(SocketException) : CSocket(SOCK_STREAM, IPPROTO_TCP) {
  setLocalAddressAndPort(localAddress, localPort);
  setListen(queueLen);
}

TCPSocket *TCPServerSocket::accept() throw(SocketException) {
  int newConnSD;
  if ((newConnSD = ::accept(sockDesc, NULL, 0)) < 0)  throw SocketException((char*)"Accept failed (accept())", true);
  return new TCPSocket(newConnSD);
}

void TCPServerSocket::setListen(int queueLen) throw(SocketException) {
  if (listen(sockDesc, queueLen) < 0)  throw SocketException((char*)"Set listening socket failed (listen())", true);
}

#endif

#ifndef DISCARDCLIENT

void Client(char* login)// test client for a server
{
	char word[MAX_WORD_SIZE];
	(*printer)((char*)"%s",(char*)"\r\n\r\n** Client launched\r\n");
	char* data = AllocateBuffer(); // limited size
	char* input = AllocateBuffer(); // limited size
	FILE* source = stdin;

	char* from = login;
	*input = 0;
	if (*from == '*') // let user go first.
	{
		++from;
		(*printer)((char*)"%s",(char*)"\r\ninput:    ");
		ReadALine(input,source);
	}

restart: // start with user
	char* separator = strchr(from,':'); // login is username  or  username:botname
	char* bot;
	if (separator)
	{
		*separator = 0;
		bot = separator + 1;
	}
	else bot = from + strlen(from);	// just a 0
	sprintf(logFilename,(char*)"log-%s.txt",from);

	// message to server is 3 strings-   username, botname, null (start conversation) or message
	char* ptr = data;
	strcpy(ptr,from); // username
	ptr += strlen(ptr);
	*ptr++ = 0; 
	strcpy(ptr,bot);
	ptr += strlen(ptr); // botname
	*ptr++ = 0; 
	strcpy(ptr,input);  // null message - start conversation or given message, user starts first
	try 
	{

SOURCE: 
	if (!strncmp(ptr,(char*)":source ",8)) 
	{
		char file[SMALL_WORD_SIZE];
		ReadCompiledWord(ptr+8,file);
		FILE* sourcefile = fopen(file,(char*)"rb");
		ReadALine(ptr,sourcefile,100000 - 100);
	}

	echo = 1;
	while (ALWAYS)
	{
			size_t len = (ptr-data) + 1 + strlen(ptr);
			TCPSocket *sock = new TCPSocket(serverIP, (unsigned short)port);
			sock->send(data, len );
			(*printer)((char*)"Sent %d bytes of data to port %d\r\n",(int)len, port);

			int bytesReceived = 1;              // Bytes read on each recv()
			int totalBytesReceived = 0;         // Total bytes read
			char* base = ptr;
			while (bytesReceived > 0) 
			{
				// Receive up to the buffer size bytes from the sender
				bytesReceived = sock->recv(base, MAX_WORD_SIZE);
				totalBytesReceived += bytesReceived;
				base += bytesReceived;
				(*printer)((char*)"Received %d bytes\r\n",bytesReceived);
				if (totalBytesReceived > 2 && ptr[totalBytesReceived-3] == 0 && ptr[totalBytesReceived-2] == 0xfe && ptr[totalBytesReceived-1] == 0xff) break; // positive confirmation was enabled
			}
			delete(sock);
			*base = 0;

			// chatbot replies this
			Log(STDTRACELOG,(char*)"%s",ptr);

			// we say that  until :exit
			(*printer)((char*)"%s",(char*)"\r\n>    ");
			ReadALine(ptr,source,100000 - 100);
			strcat(ptr,(char*)" "); // never send empty line
			if (!strnicmp(SkipWhitespace(ptr),(char*)":quit",5)) break;
			if (!strnicmp(SkipWhitespace(ptr),(char*)":source",7)) goto SOURCE;
			if (!strnicmp(SkipWhitespace(ptr),(char*)":restart",8)) 
			{
				// send restart on to server...
				len = (ptr-data) + 1 + strlen(ptr);
				sock = new TCPSocket(serverIP, (unsigned short)port);
				sock->send(data, len );
				(*printer)((char*)":restart sent data to port %d\r\n",port);

				int bytesReceived = 1;              // Bytes read on each recv()
				int totalBytesRead = 0;         // Total bytes read
				char* inbase = ptr;
				while (bytesReceived > 0) 
				{
					// Receive up to the buffer size bytes from the sender
					bytesReceived = sock->recv(inbase, MAX_WORD_SIZE);
					totalBytesRead += bytesReceived;
					inbase += bytesReceived;
					(*printer)((char*)"Received %d bytes\r\n",bytesReceived);
					if (totalBytesRead > 2 && ptr[totalBytesRead-3] == 0 && ptr[totalBytesRead-2] == 0xfe && ptr[totalBytesRead-1] == 0xff) break; // positive confirmation was enabled
				}
				delete(sock);
				*inbase = 0;
				Log(STDTRACELOG,(char*)"%s",ptr); 	// chatbot replies this

				(*printer)((char*)"%s",(char*)"\r\nEnter client user name: ");
				ReadALine(word,source);
				(*printer)((char*)"%s",(char*)"\r\n");
				from = word;
				goto restart;
			}
		}
	}

	catch(SocketException e) { myexit((char*)"failed to connect to server\r\n");}
}
#endif

#ifndef DISCARDSERVER


#define SERVERTRANSERSIZE (4 + 100 + INPUT_BUFFER_SIZE)  // offset to output buffer
#ifdef WIN32
  #pragma warning(push,1)
  #pragma warning(disable: 4290) 
  #include <process.h>
#endif
#include <cstdio>
#include <errno.h>  
using namespace std;

static TCPServerSocket* serverSocket = NULL;

unsigned int serverFinishedBy = 0; // server must complete by this or not bother

// buffers used to send in and out of chatbot

static char* clientBuffer;			//   current client spot input was and output goes
static bool chatWanted = false;		//  client is still expecting answer (has not timed out/canceled)
static bool chatbotExists = false;	//	has chatbot engine been set up and ready to go?
static int pendingClients = 0;          // number of clients waiting for server to handle them

static void* MainChatbotServer();
static void* HandleTCPClient(void *sock1);
static void* AcceptSockets(void*);
static unsigned int errorCount = 0;
static time_t lastCrash = 0;
static int loadid = 0;

uint64 startServerTime;

#ifndef WIN32 // for LINUX
#include <pthread.h> 
#include <signal.h> 
pthread_t chatThread;
static pthread_mutex_t chatLock = PTHREAD_MUTEX_INITIALIZER;	//   access lock to shared chatbot processor 
static pthread_mutex_t logLock = PTHREAD_MUTEX_INITIALIZER;		//   right to use log file output
static pthread_mutex_t testLock = PTHREAD_MUTEX_INITIALIZER;	//   right to use test memory
static pthread_cond_t  server_var   = PTHREAD_COND_INITIALIZER; // client ready for server to proceed
static pthread_cond_t  server_done_var   = PTHREAD_COND_INITIALIZER;	// server ready for clint to take data

#else // for WINDOWS
#include <winsock2.h>    
HANDLE  hChatLockMutex;    
CRITICAL_SECTION LogCriticalSection; 
CRITICAL_SECTION TestCriticalSection; 
#endif

void CloseServer() {
	if (serverSocket != NULL) {
		delete serverSocket;
		serverSocket = NULL;
	}
}

void* RegressLoad(void* junk)// test load for a server
{
	FILE* in = FopenReadOnly((char*)"REGRESS/bigregress.txt");
	if (!in) return 0;
	
	char buffer[8000];
	(*printer)((char*)"\r\n\r\n** Load %d launched\r\n",++loadid);
	char data[MAX_WORD_SIZE];
	char from[100];
	sprintf(from,(char*)"%d",loadid);
	char* bot = "";
	sprintf(logFilename,(char*)"log-%s.txt",from);
	unsigned int msg = 0;
	unsigned int volleys = 0;
	unsigned int longVolleys = 0;
	unsigned int xlongVolleys = 0;

	int maxTime = 0;
	int cycleTime = 0;
	int currentCycleTime = 0;
	int avgTime = 0;

	// message to server is 3 strings-   username, botname, null (start conversation) or message
	echo = 1;
	char* ptr = data;
	strcpy(ptr,from);
	ptr += strlen(ptr) + 1;
	strcpy(ptr,bot);
	ptr += strlen(ptr) + 1;
	*buffer = 0;
	int counter = 0;
	while (1)
	{
		if (ReadALine(revertBuffer+1,in,INPUT_BUFFER_SIZE-100) < 0) break; // end of input
		// when reading from file, see if line is empty or comment
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(revertBuffer+1,word);
		if (!*word || *word == '#' || *word == ':') continue;
		strcpy(ptr,revertBuffer+1);
		try 
		{
			size_t len = (ptr-data) + 1 + strlen(ptr);
			++volleys;
			uint64 start_time = ElapsedMilliseconds();

			TCPSocket *sock = new TCPSocket(serverIP, port);
			sock->send(data, len );
	
			int bytesReceived = 1;              // Bytes read on each recv()
			int totalBytesReceived = 0;         // Total bytes read
			char* base = ptr;
			while (bytesReceived > 0) 
			{
				// Receive up to the buffer size bytes from the sender
				bytesReceived = sock->recv(base, MAX_WORD_SIZE);
				totalBytesReceived += bytesReceived;
				base += bytesReceived;
			}
			uint64 end_time = ElapsedMilliseconds();
	
			delete(sock);
			*base = 0;
			int diff = end_time - start_time;
			if (diff > maxTime) maxTime = diff;
			if ( diff > 2000) ++longVolleys;
			if (diff > 5000) ++ xlongVolleys;
			currentCycleTime += diff;
			// chatbot replies this
		//	(*printer)((char*)"real:%d avg:%d max:%d volley:%d 2slong:%d 5slong:%d %s => %s\r\n",diff,avgTime,maxTime,volleys,longVolleys,xlongVolleys,ptr,base);
		}
		catch(SocketException e) { myexit((char*)"failed to connect to server\r\n");}
		if (++counter == 100) 
		{
			counter = 0;
			cycleTime = currentCycleTime;
			currentCycleTime = 0;
			avgTime = cycleTime / 100;
			(*printer)((char*)"From: %s avg:%d max:%d volley:%d 2slong:%d 5slong:%d\r\n",from,avgTime,maxTime,volleys,longVolleys,xlongVolleys);
		}
		else msg++;
	}
	return 0;
}

void LogChat(uint64 starttime,char* user,char* bot,char* IP, int turn,char* input,char* output)
{
	struct tm ptm;
	char* date = GetTimeInfo(&ptm,true)+SKIPWEEKDAY;
	date[15] = 0;	// suppress year
	memmove(date+3,date+4,13); // compress out space
	char* why = output + strlen(output) + 3; //skip terminator + 2 ctrl z end marker
	char* activeTopic = why + strlen(why) + 1;
	uint64 endtime = ElapsedMilliseconds(); 
	char* nl = (LogEndedCleanly()) ? (char*) "" : (char*) "\r\n";
	if (*input) Log(SERVERLOG,(char*)"%sRespond: user:%s bot:%s ip:%s (%s) %d %s  ==> %s  When:%s %dms %s\r\n", nl,user,bot,IP,activeTopic,turn,input,Purify(output),date,(int)(endtime - starttime),why);
	else Log(SERVERLOG,(char*)"%sStart: user:%s bot:%s ip:%s (%s) %d ==> %s  When:%s %dms Version:%s Build0:%s Build1:%s %s\r\n",nl, user,bot,IP,activeTopic,turn,Purify(output),date,(int)(endtime - starttime),version,timeStamp[0],timeStamp[1],why);
}

#ifndef EVSERVER // til end of file
void Crash()
{
    time_t now = time(0);
    unsigned int delay = (unsigned int) difftime(now,lastCrash);
    if (delay > 180) errorCount = 0; //   3 minutes delay is fine
    lastCrash = now;
    ++errorCount;
	if (errorCount > 6) myexit((char*)"too many crashes in a row"); //   too many crashes in a row, let it reboot from scratch
#ifndef WIN32
	//   clear chat thread if it crashed
	if (chatThread == pthread_self()) 
    {
		if (!chatbotExists ) myexit((char*)"chatbot server doesn't exist anymore");
        chatbotExists = false;
        chatWanted = false;
        pthread_mutex_unlock(&chatLock); 
    }
#endif
	longjmp(scriptJump[SERVER_RECOVERY],1);
}

#ifdef __MACH__
int pthread_mutex_timedlock(pthread_mutex_t *mutex, const struct timespec *abs_timeout)
{
	int result;
	struct timespec ts1;
	do
	{
		result = pthread_mutex_trylock(mutex);
		if (result == EBUSY)
		{
			timespec ts;
			ts.tv_sec = 0;
			ts.tv_sec = 10000000;

			/* Sleep for 10,000,000 nanoseconds before trying again. */
			int status = -1;
			while (status == -1) status = nanosleep(&ts, &ts);
		}
		else break;
		clock_get_mactime(ts1);
	}
	while (result != 0 and (abs_timeout->tv_sec == 0 ||  abs_timeout->tv_sec > ts1.tv_sec )); // if we pass to new second

	return result;
}
#endif 

// LINUX/MAC SERVER
#ifndef WIN32

void GetLogLock()  //LINUX
{
	pthread_mutex_lock(&logLock); 
}

void ReleaseLogLock()  //LINUX
{
	pthread_mutex_unlock(&logLock); 
}

void GetTestLock() //LINUX
{
	pthread_mutex_lock(&testLock); 
}

void ReleaseTestLock()  //LINUX
{
	pthread_mutex_unlock(&testLock); 
}

static bool ClientGetChatLock()  //LINUX
{
    int rc = pthread_mutex_lock(&chatLock);
    if (rc != 0 ) return false;	// we timed out
    return true;
}

static bool ClientWaitForServer(char* data,char* msg,uint64& timeout) //LINUX
{
	timeout = GetFutureSeconds(120);
	serverFinishedBy = timeout - 1;
	
    pendingClients++;
	pthread_cond_signal( &server_var ); // let server know he is needed
	int rc = pthread_cond_wait(&server_done_var, &chatLock);

     // error, timeout etc.
     bool rv = true;
     if (rc != 0) rv = false;  // mutex already unlocked
     else if (*data != 0)  rv = false;
     pthread_mutex_unlock(&chatLock);
	return rv;
}

static void LaunchClient(void* junk) // accepts incoming connections from users  //LINUX
{
	pthread_t clientThread;     
	pthread_attr_t attr; 
    pthread_attr_init(&attr); 
	pthread_attr_setdetachstate(&attr,  PTHREAD_CREATE_DETACHED);
	int val = pthread_create(&clientThread,&attr,HandleTCPClient,(void*)junk);
	int x;
	if ( val != 0)
	{
		 switch(val)
		 {
			case EAGAIN: 
			  x = sysconf( _SC_THREAD_THREADS_MAX );
			  (*printer)((char*)"create thread failed EAGAIN limit: %d \r\n",(int)x);   
			  break;
			case EPERM:  (*printer)((char*)"create thread failed EPERM \r\n");   break;
			case EINVAL:  (*printer)((char*)"create thread failed EINVAL \r\n");   break;
			default:  (*printer)((char*)"create thread failed default \r\n");   break;
		 }
	 }
}

//   use sigaction instead of signal -- bug

//   we really only expect the chatbot thread to crash, when it does, the chatLock will be under its control
void AnsiSignalMapperHandler(int sigNumber) 
{
    signal(-1, AnsiSignalMapperHandler); 
    Crash();
}

void fpViolationHandler(int sigNumber) 
{
    Crash();
}

void SegmentFaultHandler(int sigNumber) 
{
    Crash();
}

static void* ChatbotServer(void* junk)
{
	//   get initial control over the mutex so we can start. Any client grabbing it will be disappointed since chatbotExists = false, and will relinquish.
	MainChatbotServer();
	return NULL;
}

static void ServerStartup()  //LINUX
{
	pthread_mutex_lock(&chatLock);   //   get initial control over the mutex so we can start - now locked by us. Any client grabbing it will be disappointed since chatbotExists = false
}

static void ServerGetChatLock()  //LINUX
{
	while (pendingClients == 0) //   dont exit unless someone actually wanted us
	{
		pthread_cond_wait( &server_var, &chatLock ); // wait for someone to signal the server var...
	}
}

void InternetServer()  //LINUX
{
	// set error handlers
signal(-1, AnsiSignalMapperHandler);  
    
	struct sigaction action; 
    memset(&action, 0, sizeof(action)); 
sigemptyset(&action.sa_mask); 

    action.sa_flags = 0; 
    action.sa_handler = fpViolationHandler; 
sigaction(SIGFPE,&action,NULL); 
    action.sa_handler = SegmentFaultHandler; 
sigaction(SIGSEGV,&action,NULL); 

    //   thread to accept incoming connections, doesnt need much stack
    pthread_t socketThread;     
	pthread_attr_t attr; 
    pthread_attr_init(&attr); 
	//pthread_attr_setdetachstate(&attr,  PTHREAD_CREATE_DETACHED);
	pthread_create(&socketThread, &attr, AcceptSockets, (void*) NULL);   

    //  we merely loop to create chatbot services. If it dies, spawn another and try again
    while (1)
    {
        pthread_create(&chatThread, &attr, ChatbotServer, (void*) NULL);  
        void* status; 
        pthread_join(chatThread, &status); 
        if (!status) break; //   chatbot thread exit (0= requested exit - other means it crashed
		Log(SERVERLOG,(char*)"Spawning new chatserver\r\n");
     }
	myexit((char*)"end of internet server");
}
#endif  // end LINUX

#ifdef WIN32
 
void GetLogLock()
{
	if (!quitting) EnterCriticalSection(&LogCriticalSection);
}

void ReleaseLogLock()
{
	if (!quitting) LeaveCriticalSection(&LogCriticalSection);
}

void GetTestLock()
{
	if (!quitting) EnterCriticalSection(&TestCriticalSection);
}

void ReleaseTestLock()
{
	if (!quitting) LeaveCriticalSection(&TestCriticalSection);
}

static bool ClientGetChatLock()
{
	// wait to get attention of chatbot server
	DWORD dwWaitResult;
	uint64 time = ElapsedMilliseconds();
    uint64 time1;
    uint64 diff = 0;
	while (diff < 100000) // loop to get control over chatbot server 
    {
		dwWaitResult = WaitForSingleObject(hChatLockMutex,100); // try and return after 100 ms
        time1 = ElapsedMilliseconds();
        diff = time1 - time;
		if (dwWaitResult == WAIT_TIMEOUT) continue; // we didnt get the lock
        else if (!chatbotExists || chatWanted) ReleaseMutex(hChatLockMutex); // not really ready
        else return true;
    }
	return false;
}

static bool ClientWaitForServer(char* data,char* msg,uint64& timeout) // windows
{
	bool result;
	uint64 startTime = ElapsedMilliseconds();
	timeout = startTime + (120 * 1000);
	serverFinishedBy = timeout - 1000;

	ReleaseMutex(hChatLockMutex); // chatbot server can start processing my request now
	while (1) // loop to get control over chatbot server - timeout if takes too long
	{
		Sleep(10);
		GetTestLock();
		if (*data == 0) // he answered us, he will turn off chatWanted himself
		{
			result = true;
			break;
		}
		// he hasnt answered us yet, so chatWanted must still be true
		if ( ElapsedMilliseconds() >= timeout) // give up
		{
			chatWanted = result = false;	// indicate we no longer want the answer
			break;
		}
		ReleaseTestLock();
	}

	ReleaseTestLock();
	
	return result; // timed out - he can't write any data to us now
}

static void LaunchClient(void* sock) 
{
	_beginthread((void (__cdecl *)(void *)) HandleTCPClient,0,sock);
}

static void ServerStartup()
{
	WaitForSingleObject( hChatLockMutex, INFINITE );
	//   get initial control over the mutex so we can start - now locked by us. Any client grabbing it will be disappointed since chatbotExists = false
}

static void ServerGetChatLock() // WINDOWS
{
	chatWanted = false;		
    while (!chatWanted || pendingRestart) //   dont exit unless someone actually wanted us
    {
		ReleaseMutex(hChatLockMutex);
 		WaitForSingleObject( hChatLockMutex, INFINITE );
        //wait forever til somebody has something for us - we may busy spin here waiting for chatWanted to go true
		// but windows DOESNT WAIT, so
		Sleep(1);
	}

	// we have the chatlock and someone wants server
}

void PrepareServer()
{
	InitializeCriticalSection(&LogCriticalSection ); 
	InitializeCriticalSection(&TestCriticalSection ) ;
}

void handle_message(const std::string & message)
{
	char* buffer = (char*)  message.c_str();
	(*printer)("received %s\r\n", buffer);
	char* ascii1 = buffer;
	while ((ascii1 = strchr(ascii1, 1))) *ascii1 = 0; // allow ascii 1 instead of 0 as separator for JavaScript conventions.
	char* user = buffer;
	char* bot = buffer + strlen(user) + 1;
	char* input = buffer + strlen(bot) + 1;
	int returnValue = PerformChat(user, bot, ourMainInputBuffer, "122.22.22.22", ourMainOutputBuffer);	// this takes however long it takes, exclusive control of chatbot.
}

void InternetServer()
{
	if (!*websocketParam)
	{
		_beginthread((void(__cdecl *)(void *))AcceptSockets, 0, 0);	// the thread that does accepts... spinning off clients
		MainChatbotServer();  // run the server from the main thread
		CloseHandle(hChatLockMutex);
		DeleteCriticalSection(&TestCriticalSection);
		DeleteCriticalSection(&LogCriticalSection);
	}
	else
	{
		using easywsclient::WebSocket;
		WebSocket::pointer ws = WebSocket::from_url(websocketParam); //"ws://localhost:8126/foo"
		if (!ws) myexit("unable to establish websocket\r\n");
		assert(ws);

		ws->send("{\"id\" : \"ai0\", \"secret\" : \"abc123\", \"status\":\"register\" }");
		ws->send("{\"id\" : \"ai0\", \"secret\" : \"abc123\",\"status\":\"roundInformation\" }");

		while (true) {
			ws->poll(-1);
			ws->dispatch(handle_message);
			(*printer)("sent %s\r\n", ourMainOutputBuffer);
			ws->send(ourMainOutputBuffer);
		}
		ws->close();
		delete ws;
	}
}
#endif

static void ServerTransferDataToClient()
{
    strcpy(clientBuffer+SERVERTRANSERSIZE,ourMainOutputBuffer);
    clientBuffer[sizeof(int)] = 0; // mark we are done.... 
#ifndef WIN32
    pendingClients--;
	pthread_cond_signal( &server_done_var ); // let client know we are done
#endif
}

static void* AcceptSockets(void*) // accepts incoming connections from users
{
   try {
        while(1)
        {  
			struct tm ptm;
            char* time = GetTimeInfo(&ptm,true)+SKIPWEEKDAY;
            TCPSocket *sock = serverSocket->accept();
			LaunchClient((void*)sock);
         }
	} catch (SocketException &e) {(*printer)((char*)"%s",(char*)"accept busy\r\n"); ReportBug((char*)"***Accept busy\r\n")}
    myexit((char*)"end of internet server");
	return NULL;
}

static void* Done(TCPSocket * sock,char* memory)
{
	try{
 		char* output = memory+SERVERTRANSERSIZE;
		size_t len = strlen(output);
		if (len)  
		{
			if (serverctrlz) len += 3; // send end marker for positive confirmation of completion
			sock->send(output, len);
		}
	}
	catch(...) {ReportBug((char*)"sending failed\r\n");}
	delete sock;
	free(memory);
	return NULL;
}

static void* HandleTCPClient(void *sock1)  // individual client, data on STACK... might overflow... // WINDOWS + LINUX
{
	uint64 starttime = ElapsedMilliseconds(); 
	char* memory = (char*) malloc(4 + 100 + INPUT_BUFFER_SIZE + outputsize + 8); // our data in 1st chunk, his reply info in 2nd - 80k limit
	if (!memory) return NULL; // ignore him if we run out of memory
	char* output = memory+SERVERTRANSERSIZE;
	*output = 0;
	char* buffer = memory + sizeof(unsigned int); // reserve the turn field
	*((unsigned int*)memory) = 0; // will be the volley count when done
	TCPSocket *sock = (TCPSocket*) sock1;
	char IP[20];
	*IP = 0;
    try {
        strcpy(buffer,sock->getForeignAddress().c_str());	// get IP address
		strcpy(IP,buffer);
		buffer += strlen(buffer) + 1;				// use this space after IP for messaging
    } catch (SocketException e)
	{ 
		ReportBug((char*)"Socket errx") cerr << "Unable to get port" << endl;
		return Done(sock,memory);
	}

    char userName[500] = {0};
	*userName = 0;
	char* user = NULL;
	char* bot = NULL;
	char* msg = NULL;
	// A user name with a . in front of it means tie it with IP address so it remains unique 
	try{
		char* p = buffer; // current end of content
		int len = 0;
		int strs = 0;
		user = buffer;
		char* at = buffer; // where we read a string starting at
		while (strs < 3) // read until you get 3 c strings... java sometimes sends 1 char, then the rest... tedious
		{
			int len1 = sock->recv(p, SERVERTRANSERSIZE-50); // leave ip address in front alone
			len += len1; // total read in so far

			if (len1 <= 0 || len >= (SERVERTRANSERSIZE - 100))  // error or too big a transfer. we dont want it
			{
				if (len1 < 0) 
				{
					ReportBug((char*)"TCP recv from %s returned error: %d\r\n", sock->getForeignAddress().c_str(), errno);
					Log(SERVERLOG,(char*)"TCP recv from %s returned error: %d\r\n", sock->getForeignAddress().c_str(), errno);
				}
				else if ( len >= (SERVERTRANSERSIZE - 100))
				{
					ReportBug((char*)"Refusing overly long input from %s\r\n", sock->getForeignAddress().c_str());
					Log(SERVERLOG,(char*)"Refusing overly long input from %s\r\n", sock->getForeignAddress().c_str());
				}
				else 
				{
					ReportBug((char*)"TCP %s closed connection prematurely\r\n", sock->getForeignAddress().c_str());
					Log(SERVERLOG,(char*)"TCP %s closed connection prematurely\r\n", sock->getForeignAddress().c_str());
				}
				delete sock;
				free(memory);
				return NULL;
			} 
			p[len1] = 0;  // force extra string end at end of buffer

			// break apart the data into its 3 strings
			p += len1; // actual end of read buffer
			char* nul = p; // just a loop starter
			while (nul && strs < 3) // find nulls in data
			{
				nul = strchr(at,0);	// find a 0 string terminator
				if (nul >= p) break; // at end of data, doesnt count
				if (nul) // found one - we have a string we can process
				{
					++strs;
					if (strs == 1) bot = nul + 1; // where bot will start
					else if (strs == 2) msg = nul + 1; // where message will start
					else if (strs == 3) memset(nul+1,0,3); // insure excess clear
					at = nul+1;
				}
			}
		}
		if (!*user)
		{
			if (*bot == '1' && bot[1] == 0) // echo test to prove server running (generates no log traces)
			{
				strcpy(output,(char*)"1");
				return Done(sock,memory);
			}

			strcpy(output,(char*)"[you have no user id]\r\n"); 
			return Done(sock,memory);
		}

		strcpy(userName,user);

		// Request load of user data

		// wait to get attention of chatbot server, timeout if doesnt come soon enough
		bool ready = ClientGetChatLock(); // client gets chatlock  ...
		if (!ready)  // if it takes to long waiting to get server attn, give up
		{
 			PartialLogin(userName,memory); // sets LOG file so we record his messages
			output[0] = ' ';
			output[1] = 0;
			output[2] = 0xfe; //ctrl markers
			output[3] = 0xff;
			output[4] = 0;	
			Log(STDTRACELOG,(char*)"Timeout waiting for service: %s  =>  %s\r\n",msg,output);
			return Done(sock,memory);
		}

	  } catch (...)  
	  {
			struct tm ptm;
			ReportBug((char*)"***%s client presocket failed %s\r\n",IP,GetTimeInfo(&ptm,true)+SKIPWEEKDAY)
 			return Done(sock,memory);
	  }

	 uint64 serverstarttime = ElapsedMilliseconds(); 

	 // we currently own the lock,
	 uint64 endWait = 0;
     bool success = true;
	 try{
 		chatWanted = true;	// indicate we want the chatbot server - no other client can get chatLock while this is true
		clientBuffer = memory; 
		// if we get the server, endWait will be set to when we should give up

        // waits on signal for server
        // releases lock or give up on timeout
        success = ClientWaitForServer(memory+sizeof(int),msg,endWait);
		if (!success) 
		{
			strcpy(output,(char*)" ");
			output[4] = 0; // hidden why data after positive end markers
		}
		size_t len = strlen(output);
		if (outputLength) len = OutputLimit((unsigned char*) output);
		if (serverctrlz) len += 3; // send end marker for positive confirmation of completion

		sock->send(output, len);
} catch (...)  {
		(*printer)((char*)"%s",(char*)"client socket fail\r\n");
		}

	delete sock;

	if (serverLog && msg)  LogChat(starttime,user,bot, IP,*((int*)memory),msg,output);
	
	// do not delete memory til after server would have given up
	free(memory);
#ifndef WIN32
	pthread_exit(0);
#endif
	return NULL;
}
	
void GrabPort() // enable server port if you can... if not, we cannot run. 
{
    try {
        if (interfaceKind.length() == 0) {
            serverSocket = new TCPServerSocket(port);
        } else {
            serverSocket = new TCPServerSocket(interfaceKind, port);
        }
    }
    catch (SocketException &e) {exit(1);} // dont use myexit. dont want log entries for failed port used by cron to try to restart server which may be running ok.
	echo = 1;
 
#ifdef WIN32
	hChatLockMutex = CreateMutex( NULL, FALSE, NULL );   
#endif
}

static void* MainChatbotServer()
{
	ServerStartup(); //   get initial control over the mutex so we can start. - on linux if thread dies, we must reacquire here 
	// we now own the chatlock
	uint64 lastTime = ElapsedMilliseconds(); 
	if (setjmp(scriptJump[SERVER_RECOVERY])) // crashes come back to here
	{
		(*printer)((char*)"%s",(char*)"***Server exception0\r\n");
		ReportBug((char*)"***Server exception0\r\n")
#ifdef WIN32
		char* bad = GetUserVariable((char*)"$cs_crashmsg");
		if (*bad) strcpy(ourMainOutputBuffer,bad);
		else strcpy(ourMainOutputBuffer,(char*)"Hey, sorry. I forgot what I was thinking about.");
		ServerTransferDataToClient();
#endif
		ResetBuffers(); //   in the event of a trapped bug, return here, we will still own the chatlock
	}
    chatbotExists = true;   //  if a client can get the chatlock now, he will be happy
	bool oldserverlog = serverLog;
	serverLog = true;
	Log(SERVERLOG,(char*)"Server ready - logfile:%s serverLog:%d userLog:%d\r\n\r\n",serverLogfileName,oldserverlog,userLog);
	(*printer)((char*)"Server ready - logfile:%s serverLog:%d userLog:%d\r\n\r\n",serverLogfileName,oldserverlog,userLog);
	serverLog = oldserverlog;
	int returnValue = 0;

	while (1)
	{
#ifdef WIN32
	_try  { // catch crashes in windows
#else
	try {
#endif
		if (quitting) return NULL; 

		ServerGetChatLock();
		startServerTime = ElapsedMilliseconds(); 
RESTART_RETRY:
		// chatlock mutex controls whether server is processing data or client can hand server data.
		// That we now have it means a client has data for us.
		// we own the chatLock again from here on so no new client can try to pass in data. 
		// CLIENT has passed server in globals:  clientBuffer (ip,user,bot,message)
		// We will send back his answer in clientBuffer, overwriting it.
		char user[MAX_WORD_SIZE];
		char bot[MAX_WORD_SIZE];
		char* ip = clientBuffer + 4; // skip fileread data buffer id(not used)
		char* returnData = clientBuffer+SERVERTRANSERSIZE;
		char* ptr = ip;
		// incoming is 4 strings together:  ip, username, botname, message
		ptr += strlen(ip) + 1;	// ptr to username
		strcpy(user,ptr); // allow user var to be overwriteable, hence a copy
		ptr += strlen(ptr) + 1; // ptr to botname
		strcpy(bot,ptr);
		ptr += strlen(ptr) + 1; // ptr to message
		size_t test = strlen(ptr);
		if (test >= (INPUT_BUFFER_SIZE - 100)) 
		{
			ReportBug("Too much input to server %d",test);
			strcpy(ourMainInputBuffer,(char*)"too much data");
		}
		else strcpy(ourMainInputBuffer,ptr); // xfer user message to our incoming feed
		echo = false;
		struct tm ptm;
        char* dateLog = GetTimeInfo(&ptm,true)+SKIPWEEKDAY;
		if (serverPreLog)  Log(SERVERLOG,(char*)"ServerPre: %s (%s) %s %s\r\n",user,bot,ourMainInputBuffer, dateLog);

		returnValue = PerformChat(user,bot,ourMainInputBuffer,ip,ourMainOutputBuffer);	// this takes however long it takes, exclusive control of chatbot.

		// special controls
		if (returnValue == PENDING_RESTART) // special messages
		{
			Restart();
			Log(SERVERLOG,(char*)"Server ready - logfile:%s serverLog:%d userLog:%d\r\n\r\n",serverLogfileName,oldserverlog,userLog);
			(*printer)((char*)"Server restarted - logfile:%s serverLog:%d userLog:%d\r\n\r\n",serverLogfileName,oldserverlog,userLog);
			char* at = SkipWhitespace(ourMainInputBuffer);
			if (*at != ':')	goto RESTART_RETRY;
			strcpy(ourMainOutputBuffer,"Restarted");
		}
		*((int*)clientBuffer) = returnValue;
	} // end try block on calling cs performchat
#ifdef WIN32
	_except(true)
#else
	catch (...) 
#endif
	{ ReportBug((char*)"Catch Server exception\r\n") Crash();}

#ifdef WIN32
	_try { // catch crashes in windows
#else
	try {
#endif
		ServerTransferDataToClient();
	}
#ifdef WIN32
	_except(true) 
#else
	catch (...) 
#endif
	{
		ReportBug((char *) "clientBuffer is no longer valid.  Server probably closed socket because it was open too long.  Ignoring and continuing.\r\n");
	}
	}
	return NULL;
}
#endif // ndef EVSERVER
#endif
