// csocket.cpp - handles client/server behaviors  (not needed in a product)

/*
*   C++ sockets on Unix and Windows Copyright (C) 2002 --- see HEADER file
*/

#ifdef INFORMATION
We have these threads :

1. HandleTCPClient which runs on a thread to service a particular client(as many threads as clients)
2. AcceptSockets, which is a simnple thread waiting for user connections
3. ChatbotServer, which is the sole server to be the chatbot
4. main thread, which is a simple thread that creates ChatbotServer threads as needed(if ones crash in LINUX, it can spawn a new one)

chatLock - either chatbot engine or a client controls it.It is how a client gets started and then the server is summoned

While processing chat, the client holds the clientlock and the server holds the chatlock and donelock.
The client is waiting for the donelock.
When the server finishes, he releases the donelock and waits for the clientlock.
If the client exists, server gets the donelock, then releases it and waits for the clientlock.
If the client doesnt exist anymore, he has released clientlock.

It releases the clientlock and the chatlock and tries to reacquire chatlock with flag on.
when it succeeds, it has a new client.

We have these variables :
chatbotExists - the chatbot engine is now initialized.
serverFinishedBy is what time the answer must be delivered(1 second before the memory will disappear)

#endif

#include "common.h"

#ifndef WIN32
#include <ifaddrs.h>
#include <net/if.h>
#endif
static int servertransfersize;
bool echoServer = false;

char serverIP[100];

static int pass = 0;
static int fail = 0;
typedef unsigned int (*initsystem)(int, char* [], char*, char*, char*, USERFILESYSTEM*, DEBUGAPI, DEBUGAPI);
typedef unsigned int (*performchat)(char*, char*, char*, char*, char*) ;

void GetPrimaryIP(char* buffer)
{
#ifdef WIN32
	InitWinsock();
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1 ) printf("Error at socket(): %d\n", WSAGetLastError());
	const char* kGoogleDnsIp = "8.8.8.8";
	uint16_t kDnsPort = 53;
	struct sockaddr_in serv;
	memset(&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	serv.sin_addr.s_addr = inet_addr(kGoogleDnsIp);
	serv.sin_port = htons(kDnsPort);
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	int err = connect(sock, (const sockaddr*)&serv, sizeof(serv));

	sockaddr_in name;
	socklen_t namelen = sizeof(name);
	err = getsockname(sock, (sockaddr*)&name, &namelen);

	const char* p = inet_ntop(AF_INET, &name.sin_addr, buffer, MAX_WORD_SIZE);

	closesocket(sock);
	if (!server) WSACleanup();
#else
    struct ifaddrs *ifaddr, *ifa;

    // get a linked list of network interfaces
    // https://man7.org/linux/man-pages/man3/getifaddrs.3.html
    if (getifaddrs(&ifaddr) == -1) return;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL) continue;

        // only want the IPv4 address from an interface that is up and not a loopback
        // the name could be "en0" or "eth0"
        if (ifa->ifa_addr->sa_family == AF_INET && ifa->ifa_flags & IFF_UP && !(ifa->ifa_flags & IFF_LOOPBACK))
        {
            char host[NI_MAXHOST];
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0)
            {
                sprintf(buffer,"%s",host);
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
#endif
    // use localhost if nothing found
    if (!*buffer) sprintf(buffer,"127.0.0.1");
}

#ifndef DISCARDSERVER

#ifdef EVSERVER
#include "cs_ev.h"
#include "evserver.h"
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
const char *SocketException::what() const throw() { return userMessage.c_str(); }

//   Function to fill in address structure given an address and port
static void fillAddr(const string &address, unsigned short myport, sockaddr_in &addr) {
	memset(&addr, 0, sizeof(addr));  //   Zero out address structure
	addr.sin_family = AF_INET;       //   Internet address
	hostent *host;  //   Resolve name
	if ((host = gethostbyname(address.c_str())) == NULL)  throw SocketException((char*)"Failed to resolve name (gethostbyname())");
	strcpy(hostname, host->h_name);
	addr.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);
	addr.sin_port = htons(myport);     //   Assign port in network byte order
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

CSocket::CSocket(int sockDesc) { this->sockDesc = sockDesc; }

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
	if (getsockname(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)  throw SocketException((char*)"Fetch of local address failed (getsockname())", true);
	return inet_ntoa(addr.sin_addr);
}

unsigned short CSocket::getLocalPort() throw(SocketException) {
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);
	if (getsockname(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)  throw SocketException((char*)"Fetch of local port failed (getsockname())", true);
	return ntohs(addr.sin_port);
}

void CSocket::setLocalPort(unsigned short localPort) throw(SocketException) {
#ifdef WIN32
	if (InitWinsock() == FAILRULE_BIT) throw SocketException((char*)"Unable to load WinSock DLL");
#endif

	int on = 1;
	int off = 0;
#ifdef WIN32
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (char*)&off, sizeof(on));
#else
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (void*)&off, sizeof(on));
#endif

	//   Bind the socket to its port
	sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localAddr.sin_port = htons(localPort);
	if (::bind(sockDesc, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0) throw SocketException((char*)"Set of local port failed (bind())", true);
}

void CSocket::setLocalAddressAndPort(const string &localAddress,
	unsigned short localPort) throw(SocketException) {
	//   Get the address of the requested host
	sockaddr_in localAddr;
	fillAddr(localAddress, localPort, localAddr);
	int on = 1;
	int off = 0;
#ifdef WIN32
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (char*)&off, sizeof(on));
#else
	setsockopt(sockDesc, SOL_SOCKET, SO_REUSEADDR, (void*)&off, sizeof(on));
#endif
	if (::bind(sockDesc, (sockaddr *)&localAddr, sizeof(sockaddr_in)) < 0)  throw SocketException((char*)"Set of local address and port failed (bind())", true);
}

void CSocket::cleanUp() throw(SocketException) {
#ifdef WIN32
	if (WSACleanup() != 0)  throw SocketException((char*)"WSACleanup() failed");
#endif
}

unsigned short CSocket::resolveService(const string &service,
	const string &protocol) {
	struct servent *serv;        /* Structure containing service information */
	if ((serv = getservbyname(service.c_str(), protocol.c_str())) == NULL) return (unsigned short)atoi(service.c_str());  /* Service is port number */
	else return ntohs(serv->s_port);    /* Found port (network byte order) by name */
}

//   CommunicatingSocket Code

CommunicatingSocket::CommunicatingSocket(int type, int protocol)  throw(SocketException) : CSocket(type, protocol) {
}

CommunicatingSocket::CommunicatingSocket(int newConnSD) : CSocket(newConnSD) {
}

void CommunicatingSocket::connect(const string &foreignAddress, unsigned short foreignPort) throw(SocketException) {
	//   Get the address of the requested host
	sockaddr_in destAddr;
	fillAddr(foreignAddress, foreignPort, destAddr);

	//   Try to connect to the given port
	if (::connect(sockDesc, (sockaddr *)&destAddr, sizeof(destAddr)) < 0)   throw SocketException((char*)"Connect failed (connect())", true);
}

void CommunicatingSocket::send(const void *buffer, int bufferLen) throw(SocketException) {
	if (::send(sockDesc, (raw_type *)buffer, bufferLen, 0) < 0)  throw SocketException((char*)"Send failed (send())", true);
}

int CommunicatingSocket::recv(void *buffer, int bufferLen) throw(SocketException) {
	int rtn;
	if ((rtn = ::recv(sockDesc, (raw_type *)buffer, bufferLen, 0)) < 0)  throw SocketException((char*)"Received failed (recv())", true);
	return rtn;
}

string CommunicatingSocket::getForeignAddress()  throw(SocketException) {
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);
	if (getpeername(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)  throw SocketException((char*)"Fetch of foreign address failed (getpeername())", true);
	return inet_ntoa(addr.sin_addr);
}

unsigned short CommunicatingSocket::getForeignPort() throw(SocketException) {
	sockaddr_in addr;
	unsigned int addr_len = sizeof(addr);
	if (getpeername(sockDesc, (sockaddr *)&addr, (socklen_t *)&addr_len) < 0)  throw SocketException((char*)"Fetch of foreign port failed (getpeername())", true);
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

static void ReadSocket(TCPSocket* sock, char* response)
{
	int bytesReceived = 1;              // Bytes read on each recv()
	int totalBytesReceived = 0;         // Total bytes read
	char* base = response;
	*response = 0;
	while (bytesReceived > 0)
	{
		// Receive up to the buffer size bytes from the sender
		bytesReceived = sock->recv(base, MAX_WORD_SIZE);
		totalBytesReceived += bytesReceived;
		base += bytesReceived;
		if (trace) (*printer)((char*)"Received %d bytes\r\n", bytesReceived);
		if (totalBytesReceived > 2 && response[totalBytesReceived - 3] == 0 
			&& (unsigned char)response[totalBytesReceived - 2] == (unsigned char)0xfe && (unsigned char)response[totalBytesReceived - 1] == (unsigned char)0xff) break; // positive confirmation was enabled
	}
	*base = 0;
}

void NoBlankStart(char* ptr, char* where)
{
	while (*ptr == ' ') ++ptr;
	strcpy(where, ptr);
	size_t len = strlen(ptr);
	while (where[len - 1] == ' ') where[--len] = 0;
}

static bool GetSourceFile(char* ptr)
{
	char file[SMALL_WORD_SIZE];
	ReadCompiledWord(ptr, file);
	sourceFile = fopen(file, (char*)"rb");
	if (!sourceFile)
	{
		sourceFile = stdin;
		printf("No such source file %s\r\n", file);
		return false;
	}
	else return true;

}

void Client(char* login)// test client for a server
{
#ifndef DISCARDSERVER
	InitStackHeap();
	sourceFile = stdin;
	char word[MAX_WORD_SIZE];
	if (!trace) echo = false;
	(*printer)((char*)"%s", (char*)"\r\n\r\n** Client launched\r\n");
	char* from = login;
	char priorcat[MAX_WORD_SIZE];
	char prioruser[MAX_WORD_SIZE];
	char priorspec[MAX_WORD_SIZE];
	char* data = AllocateBuffer(); // read from user or file
	char* preview = AllocateBuffer();
	char* totalConvo = AllocateBuffer();
	char* response = AllocateBuffer(); // returned from chatbot
	char* sendbuffer = (char* ) mymalloc(fullInputLimit +maxBufferSize+100); // staged message to chatbot
	char user[500];
	size_t userlen;
	size_t botlen;
	size_t msglen;
	*prioruser = 0;
	*priorcat = 0;
	*priorspec = 0;
	*user = 0;
	char* msg;
	char bot[500];
	*bot = 0;
	char* botp = bot;
	*data = 0;
	int count = -1;
	int skip = 0;
	if (*from == '*') // let user go first.
	{
		++from;
		(*printer)((char*)"%s", (char*)"\r\ninput:    ");
		ReadALine(data, sourceFile); // actual user input
		msg = data;
	}
	else msg = (char*)"";

restart: // start with user
	char* separator = strchr(from, ':'); // login is username  or  username:botname or username:botname/trace or username:trace
	if (separator)
	{
		*separator = 0;
		botp = separator + 1;
		strcpy(bot, botp);
	}
	else
	{
		botp = from + strlen(from);	// just a 0
		*botp = 0;
	}
	sprintf(logFilename, (char*)"log-%s.txt", from);
	strcpy(user, from);

	// message to server is 3 strings-   username, botname, null (start conversation) or message
	char* ptr = sendbuffer;
	strcpy(ptr, user); // username
	ptr += strlen(user);
	*ptr++ = 0;
	strcpy(ptr, botp);
	ptr += strlen(ptr); // botname (and may have optional flags on it)
	*ptr++ = 0;
	*ptr = 0;
	size_t baselen = ptr - sendbuffer; // length of  message user/bot header not including message
	bool jaconverse = false;
	bool jamonologue = false;
	bool jastarts = false;
	bool source = false;
	bool raw = false;
	bool botturn = false;
	bool converse = false;
	bool endConvo = false;
	if (!*loginID) strcpy(loginID, user);

	try
	{

	SOURCE:
		if (!strnicmp(ptr, (char*)":converse ", 9))
		{
			char file[SMALL_WORD_SIZE];
			ReadCompiledWord(ptr + 8, file);
			sourceFile = fopen(file, (char*)"rb");
			converse = true;
		}
		else if (!strnicmp(ptr, (char*)":jastarts ", 9))
		{
			char file[SMALL_WORD_SIZE];
			ptr = ReadCompiledWord(ptr + 9, file);
			sourceFile = fopen(file, (char*)"rb");
			if (!sourceFile)
			{
				printf("%s not found\r\n", file);
				myexit("not found");
			}
			jastarts = true;
			ptr = SkipWhitespace(ptr);
			char num[100];
			ptr = ReadCompiledWord(ptr, num);
			count = atoi(num);
			if (count == 0) count = 1000000; // count and skip optional, default to all
			ptr = ReadCompiledWord(ptr, num);
			skip = atoi(num);
			ptr = data;
		}
		else if (!strnicmp(ptr, (char*)":jaconverse", 11) || !strnicmp(ptr, (char*)":jamonologue", 12))
		{
			char file[SMALL_WORD_SIZE];
			int diff =  11;
			if (!strnicmp(ptr, (char*)":jamonologue", 12))
			{
				jamonologue = true;
				diff += 1;
			}
			ptr = ReadCompiledWord(ptr + diff, file);
			sourceFile = fopen(file, (char*)"rb");
			if (!sourceFile)
			{
				printf("%s not found\r\n", file);
				myexit("not found");
			}
			jastarts = true;
			jaconverse = true;
			botturn = true;
			ptr = SkipWhitespace(ptr);
			char num[100];
			ptr = ReadCompiledWord(ptr, num);
			count = atoi(num);
			if (count == 0) count = 1000000; // count and skip optional, default to all
			ptr = ReadCompiledWord(ptr, num);
			skip = atoi(num);
			ptr = data;
		}
		else if (!strnicmp(ptr, (char*)":raw ", 5))
		{
			char file[SMALL_WORD_SIZE];
			ptr = ReadCompiledWord(ptr + 5, file);
			sourceFile = fopen(file, (char*)"rb");
			raw = true;
			ptr = SkipWhitespace(ptr);
			char num[100];
			ptr = ReadCompiledWord(ptr, num);
			count = atoi(num);
			if (count == 0) count = 1000000;
			ptr = ReadCompiledWord(ptr, num);
			skip = atoi(num);
			ptr = data;
		}
		char* sep = NULL;
		TCPSocket *sock;
		bool failonce = false;
		int n = 0;
		char copy[MAX_BUFFER_SIZE];
		char output[MAX_WORD_SIZE * 10];
		*output = 0;
		while (ALWAYS)
		{
			if ((++n % 100) == 0)  (*printer)((char*)"On Line %d\r\n", n);
			if (!strnicmp(ptr, (char*)":source ", 8))
			{
				source = GetSourceFile(ptr + 8);
			}
			if (source)
			{
				int ans = ReadALine(ptr, sourceFile, fullInputLimit + maxBufferSize - 100);
				if (ans <= 0)
				{
					source = false;
					sourceFile = stdin;
					ans = ReadALine(ptr, sourceFile, fullInputLimit + maxBufferSize - 100);
					continue;
				}
			}
			else if (converse) // do a conversation of multiple lines each tagged with user until done, not JA style
			{
				ptr = data;
				if (Myfgets(ptr, 100000 - 100, sourceFile) == NULL) break;
				if (HasUTF8BOM(ptr)) memmove(data, data + 3, strlen(data + 2));// UTF8 BOM
																																							   // (*printer)((char*)"Read %s\r\n",  data);
				strcpy(copy, ptr);
				size_t l = strlen(ptr);
				ptr[l - 2] = 0; // remove crlf
								// a line is username message
				char* blank = strchr(ptr, '\t'); // user / botname
				if (!blank) continue;
				*blank = 0;  // user string now there

				sep = strchr(blank + 1, '\t'); // botname / message
				if (!sep) continue;
				*sep = 0; // bot string now there
				strcpy(bot, blank + 1);

				botp = bot;
				botlen = strlen(bot);
				if (stricmp(ptr, user)) // change over user with null start message
				{
					strcpy(user, data);
					strcpy(sendbuffer, user);
					userlen = strlen(user);
					sendbuffer[userlen + 1] = 0;
					botlen = strlen(bot);
					strcpy(sendbuffer + userlen + 1, bot);
					sendbuffer[userlen + 1 + botlen + 1] = 0;
					baselen = userlen + botlen + 2;
					sendbuffer[baselen] = 0;
					// (*printer)((char*)"Sent login %s\r\n", data);
					sock = new TCPSocket(serverIP, (unsigned short)port);
					sock->send(sendbuffer, baselen + 1);
					ReadSocket(sock, response);
					delete(sock);
				}
				msg = sep + 1;
				ptr = msg;
			}
			else if (jastarts) // includes single lines and continuous conversations
			{
				if (*preview) strcpy(data, preview); // become primary
				else *data = 0;
				if (Myfgets(preview, 100000 - 100, sourceFile) == NULL)
				{
					if (failonce) break; // really over
					failonce = true; // now only have preview copy left
				}
				if (!*data) continue; // starting out, priming preview buffer

				if (--skip > 0) continue;
				if (--count < 0) break;

				char newuser[1000];
				char* blank = strchr(preview, '\t');
				if (!blank) continue;
				*blank = 0;
				NoBlankStart(preview, newuser);
				*blank = '\t';

				ptr = data; // process current data
				if (HasUTF8BOM(ptr)) memmove(data, data + 3, strlen(data + 2));// UTF8 BOM

							// Get User name
				blank = strchr(ptr, '\t');
				if (!blank) continue;
				*blank = 0;  // user string now there
				NoBlankStart(data, user);
				*blank = '\t';  // user string now there

				endConvo = failonce || strcmp(newuser, user) || strstr(preview, "a secure page"); // changing user or last line of data
				strcpy(copy, ptr);
				size_t l = strlen(ptr);
				while (ptr[l - 1] == '\t' || ptr[l - 1] == '\n' || ptr[l - 1] == '\r') ptr[--l] = 0; // remove trailing tabs
				strcpy(copy, ptr);
				char cat[MAX_WORD_SIZE];
				char spec[MAX_WORD_SIZE];
				char loc[MAX_WORD_SIZE];
				// userid, cat, spec, loc, message, {output}
				// userid can be blank, we overwrite with user1 name
				// if userid is 1, these are conversations which end when cat/spec changes

				// get category
				ptr = blank + 1; // cat star
				blank = strchr(ptr, '\t');
				if (!blank) continue;
				*blank = 0;
				NoBlankStart(ptr, cat);
				char* space = cat;
				while ((space = strchr(space, ' ')))
				{
					if (space && IsAlphaUTF8(space[1]) && space[1] != ' ') *space = '_';
					else ++space;
				}

				// get specialty
				ptr = blank + 1; // spec
				blank = strchr(ptr, '\t');
				if (!blank) continue;
				*blank = 0;
				NoBlankStart(ptr, spec);
				space = spec;
				while ((space = strchr(space, ' ')))
				{
					if (space && IsAlphaUTF8(space[1]) && space[1] != ' ') *space = '_';
					else ++space;
				}
				if (!stricmp(spec, "none") || !stricmp(spec, "null") || *spec == 0) strcpy(spec, "general");

				// get location
				ptr = blank + 1; // loc
				blank = strchr(ptr, '\t'); // end of loc
				if (!blank) continue;
				*blank = 0;
				if (*ptr == ' ') ++ptr;
				NoBlankStart(ptr, loc);

				// Get Input & check output
				ptr = blank + 1; // start of message
				blank = strchr(ptr, '\t');
				if (blank) // input has output column
				{
					strcpy(output, blank + 1);
					*blank = 0; // mark end of input message
				}
				else *output = 0;

				bool newstart = false;
				// do we send the start message (new user, category, or specialty)
				if (stricmp(user, prioruser) || stricmp(cat, priorcat) || stricmp(spec, priorspec)) // changing cat/spec
				{
					newstart = true;
					botturn = true;
				}

				if (jaconverse) botturn = !botturn;  // flip viewpoint
				if (jaconverse && !botturn && !newstart && !jamonologue) continue; // ignore this as swallowed

				strcpy(priorcat, cat);
				strcpy(priorspec, spec);
				strcpy(prioruser, user);
				if (!stricmp(cat, "general")) continue; // sip rerouter, we have no idea where it went

														// construct std start message  user,bot,0
				sprintf(sendbuffer, "%s",loginID); // same user here always -- fails to clear some globals w/o :reset:
											 // else strcpy(sendbuffer, user);
				userlen = strlen(sendbuffer);
				*bot = 0;
				botlen = strlen(bot);
				sendbuffer[userlen + 1] = 0;
				strcpy(sendbuffer + userlen + 1, bot);
				sendbuffer[userlen + 1 + botlen + 1] = 0;
				baselen = userlen + botlen + 2;
				char* at = sendbuffer + baselen;

				if (newstart)
				{
					*totalConvo = 0;
					if (*output)
					{
						if (!*loc) sprintf(at, ":reset: [ category: %s specialty: %s id: %s expect: \"%s\"]", cat, spec, user, output);
						else sprintf(at, ":reset: [ category: %s specialty: %s id: %s location: %s  expect: \"%s\"]", cat, spec, user, loc, output);
					}
					else
					{
						if (!*loc) sprintf(at, ":reset: [ category: %s specialty: %s id: %s ]", cat, spec, user);
						else sprintf(at, ":reset: [ category: %s specialty: %s id: %s location: %s ]", cat, spec, user, loc);
					}
					sock = new TCPSocket(serverIP, (unsigned short)port);
					sock->send(sendbuffer, baselen + 1 + strlen(at));
					ReadSocket(sock, response);
					delete(sock);
				}
				if (jaconverse && !botturn && !jamonologue) continue; // ignore this now that converse is started
				else
				{
					strcat(totalConvo, " | ");
					strcat(totalConvo, ptr);
				}
			}
			else if (raw)
			{
				ptr = data;
				if (Myfgets(ptr, 100000 - 100, sourceFile) == NULL)
					break;
				if (--skip > 0) continue;
				if (--count < 0)
					break;
				strcpy(copy, ptr);
				if (HasUTF8BOM(data))  memmove(data, data + 3, strlen(data + 2));// UTF8 BOM
				strcpy(user, "user1");
				strcpy(sendbuffer, user);
				userlen = strlen(sendbuffer);
				*bot = 0;
				botlen = strlen(bot);
				sendbuffer[userlen + 1] = 0;
				strcpy(sendbuffer + userlen + 1, bot);
				sendbuffer[userlen + 1 + botlen + 1] = 0;
				baselen = userlen + botlen + 2;
				char* at = sendbuffer + baselen;
				sprintf(at, "[ category: %s ]", "legal");
				sock = new TCPSocket(serverIP, (unsigned short)port);
				sock->send(sendbuffer, baselen + 1 + strlen(at));
				ReadSocket(sock, response);
				delete(sock);
			}
			// send our normal message now
			msglen = strlen(ptr);
			char* at = sendbuffer + baselen;
			if (*output || endConvo)
			{
				if (*output && endConvo) sprintf(at, "[expect: %s end: \"%s\"] ", output, totalConvo);
				else if (*output) sprintf(at, "[expect: %s] ", output);
				else sprintf(at, "[end: \"totalConvo\"] ");
				at += strlen(at);
			}
			strncpy(at, ptr, msglen + 1);

			size_t len = baselen + strlen(sendbuffer + baselen) + 1;
			sock = new TCPSocket(serverIP, (unsigned short)port);
			sock->send(sendbuffer, len);
			if (!converse && !jastarts && !raw) (*printer)((char*)"Sent %d bytes of data to port %d - %s|%s\r\n", (int)len, port, sendbuffer, msg);
			ReadSocket(sock, response);
			if (!trace) echo = !converse;
			delete(sock);
			char* pastoob = strrchr(response, ']');
			if (pastoob) ++pastoob;
			*data = 0;

			// we say that  until :exit
			if (!converse && !jastarts && !raw && !source)
			{
				(*printer)((char*)"%s", response);
				(*printer)((char*)"%s", (char*)"\r\n>    ");

				// check for oob callback 
				ProcessOOB(response);
				while (ProcessInputDelays(data + 1, KeyReady())) { ; }// use our fake callback input? loop waiting if no user input found
			}
			else if (jastarts || raw) {}
			else Log(USERLOG, "%s %s %s\r\n", user, bot, response);
			if (!converse && !jastarts && !jaconverse && !raw && !source)
			{
				if (*data) {}
				else if (ReadALine(data, sourceFile, 100000 - 100) < 0) break; // next thing we want to send
				strcat(data, (char*)" "); // never send empty line
				msg = data;
				ptr = data;
				// special instructions
				if (!strnicmp(SkipWhitespace(data), (char*)":quit", 5)) break;
				else if (!strnicmp(SkipWhitespace(data), (char*)":source", 7)) goto SOURCE;
				else if (!strnicmp(SkipWhitespace(data), (char*)":jastarts", 7))
				{
					ptr = data;
					goto SOURCE;
				}
				else if (!strnicmp(SkipWhitespace(data), (char*)":jaconverse", 11) || !strnicmp(SkipWhitespace(data), (char*)":jamonologue", 12))
				{
					ptr = data;
					goto SOURCE;
				}
				else if (!strnicmp(ptr, (char*)":dllchat", 8))
				{
#ifdef WIN32
					HINSTANCE hGetProcIDDLL = LoadLibrary((LPCSTR)"chatscript.dll");
					if (!hGetProcIDDLL) {
						printf("could not load cs dll\r\n");
						myexit("no dll");
					}
					// resolve function address here
					initsystem x = (initsystem)GetProcAddress(hGetProcIDDLL, "InitSystem");
						//unsigned int InitSystem(int argcx, char* argvx[], char* unchangedPath, char* readablePath, char* writeablePath, USERFILESYSTEM* userfiles, DEBUGAPI infn, DEBUGAPI outfn)
					if (!x) {
							printf("could not locate InitSystem\r\n");
							return;
					}
					printf("Loading DLL\r\n");
					if (x(0, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
							printf(" InitSystem failed\r\n");
							return;
					}
					performchat y = (performchat)GetProcAddress(hGetProcIDDLL, "PerformChat");
					//	PerformChat(loginID, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer); // no ip
					if (!y) {
						printf("could not locate PerformChat\r\n");
						return;
					}

					char file[SMALL_WORD_SIZE];
					ReadCompiledWord(ptr + 8, file);
					char* input = AllocateBuffer();
					char* output = AllocateBuffer();
					sourceFile = fopen(file, (char*)"rb");
					sprintf(serverLogfileName, "%s/serverlogdll.txt",logsfolder);
					while (ReadALine(input, sourceFile, MAX_BUFFER_SIZE) >= 0)
					{
						*output = 0;
						server = true;
						Log(SERVERLOG, "ServerPre: %s (%s) size:%d %s\r\n", user, bot, strlen(input), input);
						server = false; 
						y((char*)"dll-user", (char*)"", input, (char*)"11.11.11.11", output);
						server = true; 
						Log(SERVERLOG, "Respond: %s (%s) %s\r\n", user, bot, output);
						printf(output);
						server = false;
					}
					FClose(sourceFile);
					printf("\r\ndone\r\n");
					FreeBuffer();
					FreeBuffer();
#endif
					exit(0);
				}
				else if (!strnicmp(SkipWhitespace(data), (char*)":jaraw", 4))
				{
					ptr = data;
					goto SOURCE;
				}
				else if (!converse && !strncmp(data, (char*)":converse ", 9))
				{
					char file[SMALL_WORD_SIZE];
					ReadCompiledWord(data + 9, file);
					sourceFile = fopen(file, (char*)"rb");
					converse = true;
					continue;
				}
				else if (!strnicmp(SkipWhitespace(data), (char*)":restart", 8))
				{
					// send restart on to server...
					sock = new TCPSocket(serverIP, (unsigned short)port);
					sock->send(data, strlen(data) + 1);
					(*printer)((char*)":restart sent data to port %d\r\n", port);
					ReadSocket(sock, response);
					Log(USERLOG,"%s", response); 	// chatbot replies this
					delete(sock);

					(*printer)((char*)"%s", (char*)"\r\nEnter client user name: ");
					ReadALine(word, sourceFile);
					(*printer)((char*)"%s", (char*)"\r\n");
					from = word;
					goto restart;
				}
			}
		}

	}
	catch (SocketException e) {
		myexit((char*)"failed to connect to server");
	}
	FClose(sourceFile);
#endif
}
#endif

#ifndef DISCARDSERVER


#define SERVERTRANSERSIZE (4 + 100 + fullInputLimit)  // offset to output buffer
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

#ifndef EVSERVER
static void* MainChatbotServer();
static void* HandleTCPClient(void* sock1);
static void* AcceptSockets(void*);
static unsigned int errorCount = 0;
static time_t lastCrash = 0;
#endif

static int loadid = 0;

#ifndef WIN32 // for LINUX
#include <pthread.h> 
#include <signal.h> 
pthread_t chatThread;
static pthread_mutex_t chatLock = PTHREAD_MUTEX_INITIALIZER;	//   access lock to shared chatbot processor 
static pthread_mutex_t testLock = PTHREAD_MUTEX_INITIALIZER;	//   right to use test memory
static pthread_cond_t  server_var = PTHREAD_COND_INITIALIZER; // client ready for server to proceed
static pthread_cond_t  server_done_var = PTHREAD_COND_INITIALIZER;	// server ready for clint to take data

#else // for WINDOWS
#include <winsock2.h>    
HANDLE  hChatLockMutex;
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
	(*printer)((char*)"\r\n\r\n** Load %d launched\r\n", ++loadid);
	char data[MAX_WORD_SIZE];
	char from[100];
	sprintf(from, (char*)"%d", loadid);
	 const char* bot = "";
	sprintf(logFilename, (char*)"log-%s.txt", from);
	unsigned int msg = 0;
	unsigned int volleys = 0;
	unsigned int longVolleys = 0;
	unsigned int xlongVolleys = 0;

	int maxTime = 0;
	int cycleTime = 0;
	int currentCycleTime = 0;
	int avgTime = 0;

	// message to server is 3 strings-   username, botname, null (start conversation) or message
	echo = true;
	char* ptr = data;
	strcpy(ptr, from);
	ptr += strlen(ptr) + 1;
	strcpy(ptr, bot);
	ptr += strlen(ptr) + 1;
	*buffer = 0;
	int counter = 0;
	while (1)
	{
		if (ReadALine(revertBuffer + 1, in, maxBufferSize - 100) < 0) break; // end of input
																			 // when reading from file, see if line is empty or comment
		char word[MAX_WORD_SIZE];
		ReadCompiledWord(revertBuffer + 1, word);
		if (!*word || *word == '#' || *word == ':') continue;
		strcpy(ptr, revertBuffer + 1);
		try
		{
			size_t len = (ptr - data) + 1 + strlen(ptr);
			++volleys;
			uint64 start_time = ElapsedMilliseconds();

			TCPSocket *sock = new TCPSocket(serverIP, port);
			sock->send(data, len);

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
			if (diff > 2000) ++longVolleys;
			if (diff > 5000) ++xlongVolleys;
			currentCycleTime += diff;
			// chatbot replies this
			//	(*printer)((char*)"real:%d avg:%d max:%d volley:%d 2slong:%d 5slong:%d %s => %s\r\n",diff,avgTime,maxTime,volleys,longVolleys,xlongVolleys,ptr,base);
		}
		catch (SocketException e) { myexit((char*)"failed to connect to server"); }
		if (++counter == 100)
		{
			counter = 0;
			cycleTime = currentCycleTime;
			currentCycleTime = 0;
			avgTime = cycleTime / 100;
			(*printer)((char*)"From: %s avg:%d max:%d volley:%d 2slong:%d 5slong:%d\r\n", from, avgTime, maxTime, volleys, longVolleys, xlongVolleys);
		}
		else msg++;
	}
	return 0;
}

#ifndef EVSERVER // til end of file
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
	} while (result != 0 and (abs_timeout->tv_sec == 0 || abs_timeout->tv_sec > ts1.tv_sec)); // if we pass to new second

	return result;
}
#endif 

// LINUX/MAC SERVER
#ifndef WIN32

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
	if (rc != 0) return false;	// we timed out
	return true;
}

static bool ClientWaitForServer(char* data, char* msg, uint64& timeout) //LINUX
{
	timeout = GetFutureSeconds(120);
	serverFinishedBy = timeout - 1;

	pendingClients++;
	pthread_cond_signal(&server_var); // let server know he is needed
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
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	int val = pthread_create(&clientThread, &attr, HandleTCPClient, (void*)junk);
	int x;
	if (val != 0)
	{
		switch (val)
		{
		case EAGAIN:
			x = sysconf(_SC_THREAD_THREADS_MAX);
			(*printer)((char*)"create thread failed EAGAIN limit: %d \r\n", (int)x);
			break;
		case EPERM:  (*printer)((char*)"create thread failed EPERM \r\n");   break;
		case EINVAL:  (*printer)((char*)"create thread failed EINVAL \r\n");   break;
		default:  (*printer)((char*)"create thread failed default \r\n");   break;
		}
	}
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
		pthread_cond_wait(&server_var, &chatLock); // wait for someone to signal the server var...
	}
}

void InternetServer()  //LINUX
{
	servertransfersize = (4 + 100 + maxBufferSize); // offset to output buffer

	//   thread to accept incoming connections, doesnt need much stack
	pthread_t socketThread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//pthread_attr_setdetachstate(&attr,  PTHREAD_CREATE_DETACHED);
	pthread_create(&socketThread, &attr, AcceptSockets, (void*)NULL);

	//  we merely loop to create chatbot services. If it dies, spawn another and try again
	while (1)
	{
		pthread_create(&chatThread, &attr, ChatbotServer, (void*)NULL);
		void* status;
		pthread_join(chatThread, &status);
		if (!status) break; //   chatbot thread exit (0= requested exit - other means it crashed
		Log(SERVERLOG,"Spawning new chatserver\r\n");
	}
	exit(0);
}
#endif  // end LINUX

#ifdef WIN32

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
		dwWaitResult = WaitForSingleObject(hChatLockMutex, 100); // try and return after 100 ms
		time1 = ElapsedMilliseconds();
		diff = time1 - time;
		if (dwWaitResult == WAIT_TIMEOUT) continue; // we didnt get the lock
		else if (!chatbotExists || chatWanted) ReleaseMutex(hChatLockMutex); // not really ready
		else return true;
	}
	return false;
}

static bool ClientWaitForServer(char* data, char* msg, uint64& timeout) // windows
{
	bool result = false;
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
		if (ElapsedMilliseconds() >= timeout) // give up
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
	_beginthread((void(__cdecl *)(void *)) HandleTCPClient, 0, sock);
}

static void ServerStartup()
{
	WaitForSingleObject(hChatLockMutex, INFINITE);
	//   get initial control over the mutex so we can start - now locked by us. Any client grabbing it will be disappointed since chatbotExists = false
}

static void ServerGetChatLock() // WINDOWS
{
	chatWanted = false;
	while (!chatWanted || pendingRestart) //   dont exit unless someone actually wanted us
	{
		ReleaseMutex(hChatLockMutex);
		WaitForSingleObject(hChatLockMutex, INFINITE);
		//wait forever til somebody has something for us - we may busy spin here waiting for chatWanted to go true
		// but windows DOESNT WAIT, so
		Sleep(1);
	}

	// we have the chatlock and someone wants server
}

void PrepareServer()
{
	InitializeCriticalSection(&TestCriticalSection);
}

void InternetServer()
{
	_beginthread((void(__cdecl *)(void *))AcceptSockets, 0, 0);	// the thread that does accepts... spinning off clients
	servertransfersize = (4 + 100 + maxBufferSize); // offset to output buffer
	MainChatbotServer();  // run the server from the main thread
	CloseHandle(hChatLockMutex);
	DeleteCriticalSection(&TestCriticalSection);
}
#endif

static void ServerTransferDataToClient()
{
	size_t len = strlen(ourMainOutputBuffer); // main message
	char* why = ourMainOutputBuffer + len + 1;
	size_t len1 = strlen(why); // hidden why
	char* topic = why + len1 + 1;
	size_t len2 = strlen(topic); // hidden active topic
	size_t offset = SERVERTRANSERSIZE;
	memcpy(clientBuffer + offset, ourMainOutputBuffer, len + len1 + len2 + 4);
	clientBuffer[sizeof(int)] = 0; // mark we are done.... 
#ifndef WIN32
	pendingClients--;
	pthread_cond_signal(&server_done_var); // let client know we are done
#endif
}

static void* AcceptSockets(void*) // accepts incoming connections from users
{
	try {
		while (1)
		{
			struct tm ptm;
			TCPSocket *sock = serverSocket->accept();
			LaunchClient((void*)sock);
		}
	}
	catch (SocketException& e) { (*printer)((char*)"%s", (char*)"accept busy\r\n"); ReportBug((char*)"***Accept busy\r\n"); }
	myexit((char*)"end of internet server");
	return NULL;
}

static void* Done(TCPSocket * sock, char* memory)
{
	--userQueue;
	try {
		char* output = memory + SERVERTRANSERSIZE;
		size_t len = strlen(output);
		if (len)
		{
			if (serverctrlz) len += 3; // send end marker for positive confirmation of completion
			sock->send(output, len);
		}
	}
	catch (...) { ReportBug((char*)"sending failed\r\n"); }
	delete sock;
	myfree(memory);
	return NULL;
}

static void* HandleTCPClient(void *sock1)  // individual client, data on STACK... might overflow... // WINDOWS + LINUX
{
	uint64 starttime = ElapsedMilliseconds();
	char* memory = mymalloc(4 + 100 + fullInputLimit + outputsize + 8); // our data in 1st chunk, his reply info in 2nd - 80k limit
	if (!memory) return NULL; // ignore him if we run out of memory
	char* output = memory + SERVERTRANSERSIZE;
	*output = 0;
	char* buffer = memory + sizeof(unsigned int); // reserve the turn field
	*((unsigned int*)memory) = 0; // will be the volley count when done
	TCPSocket *sock = (TCPSocket*)sock1;
	char IP[20];
	*IP = 0;
	try {
		strcpy(buffer, sock->getForeignAddress().c_str());	// get IP address
		strcpy(IP, buffer);
		buffer += strlen(buffer) + 1;				// use this space after IP for messaging
	}
	catch (SocketException e)
	{
		ReportBug((char*)"Socket errx");
		cerr << "Unable to get port" << endl;
		return Done(sock, memory);
	}

	char userName[500] = { 0 };
	*userName = 0;
	char* user = NULL;
	char* bot = NULL;
	char* msg = NULL;
	// A user name with a . in front of it means tie it with IP address so it remains unique 
	try {
		char* p = buffer; // current end of content
		int len = 0;
		int strs = 0;
		user = buffer;
		char* at = buffer; // where we read a string starting at
		while (strs < 3) // read until you get 3 c strings... java sometimes sends 1 char, then the rest... tedious
		{
			int len1 = sock->recv(p, SERVERTRANSERSIZE - 50); // leave ip address in front alone
			len += len1; // total read in so far

			if (len1 <= 0 || len >= (int)(SERVERTRANSERSIZE - 100))  // error or too big a transfer. we dont want it
			{
				if (len1 < 0)
				{
					ReportBug((char*)"TCP recv from %s returned error: %d\r\n", sock->getForeignAddress().c_str(), errno);
					Log(SERVERLOG,"TCP recv from %s returned error: %d\r\n", sock->getForeignAddress().c_str(), errno);
				}
				else if (len >= (int)(SERVERTRANSERSIZE - 100))
				{
					ReportBug((char*)"Refusing overly long input from %s\r\n", sock->getForeignAddress().c_str());
					Log(SERVERLOG,"Refusing overly long input from %s\r\n", sock->getForeignAddress().c_str());
				}
				else
				{
					ReportBug((char*)"TCP %s closed connection prematurely\r\n", sock->getForeignAddress().c_str());
					Log(SERVERLOG,"TCP %s closed connection prematurely\r\n", sock->getForeignAddress().c_str());
				}
				delete sock;
				myfree(memory);
				return NULL;
			}
			p[len1] = 0;  // force extra string end at end of buffer

						  // break apart the data into its 3 strings
			p += len1; // actual end of read buffer
			char* nul = p; // just a loop starter
			while (nul && strs < 3) // find nulls in data
			{
				nul = strchr(at, 0);	// find a 0 string terminator
				if (nul >= p) break; // at end of data, doesnt count
				if (nul) // found one - we have a string we can process
				{
					++strs;
					if (strs == 1) bot = nul + 1; // where bot will start
					else if (strs == 2) msg = nul + 1; // where message will start
					else if (strs == 3) memset(nul + 1, 0, 3); // insure excess clear
					at = nul + 1;
				}
			}
		}
		if (!*user)
		{
			if (*bot == '1' && bot[1] == 0) // echo test to prove server running (generates no log traces)
			{
				strcpy(output, (char*)"1");
				return Done(sock, memory);
			}

			strcpy(output, (char*)"[you have no user id]\r\n");
			return Done(sock, memory);
		}

		++userQueue;
		if (pendingUserLimit && userQueue >= pendingUserLimit)
		{
			sock->send(overflowMessage, overflowLen);
			char* output = memory + SERVERTRANSERSIZE;
			*output = 0;
			return Done(sock, memory);
		}

		strcpy(userName, user);

		// Request load of user data

		// wait to get attention of chatbot server, timeout if doesnt come soon enough
		bool ready = ClientGetChatLock(); // client gets chatlock  ...
		if (!ready)  // if it takes to long waiting to get server attn, give up
		{
			PartialLogin(userName, memory); // sets LOG file so we record his messages
			output[0] = ' ';
			output[1] = 0;
			output[2] = 0xfe; //ctrl markers
			output[3] = 0xff;
			output[4] = 0;
			Log(USERLOG,"Timeout waiting for service: %s  =>  %s\r\n", msg, output);
			return Done(sock, memory);
		}

	}
	catch (...)
	{
		struct tm ptm;
		ReportBug((char*)"***%s client presocket failed %s\r\n", IP, GetTimeInfo(&ptm, true) + SKIPWEEKDAY);
		return Done(sock, memory);
	}

	uint64 serverstarttime = ElapsedMilliseconds();

	// we currently own the lock,
	uint64 endWait = 0;
	bool success = true;
	try {
		chatWanted = true;	// indicate we want the chatbot server - no other client can get chatLock while this is true
		clientBuffer = memory;
		// if we get the server, endWait will be set to when we should give up

		// waits on signal for server
		// releases lock or give up on timeout
		success = ClientWaitForServer(memory + sizeof(int), msg, endWait);
		if (!success)
		{
			strcpy(output, (char*)" ");
			output[4] = 0; // hidden why data after positive end markers
		}
		size_t len = strlen(output);
		if (outputLength) len = OutputLimit((unsigned char*)output);
		if (serverctrlz) len += 3; // send end marker for positive confirmation of completion

		sock->send(output, len);
	}
	catch (...) {
		(*printer)((char*)"%s", (char*)"client socket fail\r\n");
	}
	--userQueue;
	delete sock;

	// do not delete memory til after server would have given up
	myfree(memory);
#ifndef WIN32
	pthread_exit(0);
#endif
	return NULL;
}

void GrabPort() // enable server port if you can... if not, we cannot run. 
{
	try {
		if (interfaceKind.length() == 0) serverSocket = new TCPServerSocket(port);
		else serverSocket = new TCPServerSocket(interfaceKind, port);
	}
	catch (SocketException &e) { exit(1); } // dont use myexit. dont want log entries for failed port used by cron to try to restart server which may be running ok.
	echo = true;

#ifdef WIN32
	hChatLockMutex = CreateMutex(NULL, FALSE, NULL);
#endif
}

static void* MainChatbotServer()
{
	ServerStartup(); //   get initial control over the mutex so we can start. - on linux if thread dies, we must reacquire here 
					 // we now own the chatlock
	uint64 lastTime = ElapsedMilliseconds();
	char user[MAX_WORD_SIZE];
	char bot[MAX_WORD_SIZE];
	char* ip; 

	chatbotExists = true;   //  if a client can get the chatlock now, he will be happy
	int oldserverlog = serverLog;
	serverLog = FILE_LOG;
	Log(ECHOSERVERLOG, (char*)"Server ready - logfile: %s serverLog: %d userLog: %d \r\n\r\n", serverLogfileName, oldserverlog, userLog);
	serverLog = oldserverlog;
	int returnValue = 0;

	while (1)
	{
#ifdef WIN32
		__try{ // catch crashes in windows
#else
		try {
#endif
			if (quitting) return NULL;

			ServerGetChatLock();
		RESTART_RETRY:
			// chatlock mutex controls whether server is processing data or client can hand server data.
			// That we now have it means a client has data for us.
			// we own the chatLock again from here on so no new client can try to pass in data. 
			// CLIENT has passed server in globals:  clientBuffer (ip,user,bot,message)
			// We will send back his answer in clientBuffer, overwriting it.
			ip = clientBuffer + 4; // skip fileread data buffer id(not used)
			char* ptr = ip;
			// incoming is 4 strings together:  ip, username, botname, message
			ptr += strlen(ip) + 1;	// ptr to username
			strcpy(user,ptr); // allow user var to be overwriteable, hence a copy
			ptr += strlen(ptr) + 1; // ptr to botname
			strcpy(bot,ptr);
			ptr += strlen(ptr) + 1; // ptr to message
			echo = false;
			size_t len = strlen(ptr);
			if (len >= (fullInputLimit - 100))
			{
				ReportBug("Too much input to server %d", len);
				strcpy(ourMainInputBuffer,(char*)"too much data");
			}
			else strcpy(ourMainInputBuffer,ptr); // xfer user message to our incoming feed
			cs_qsize = 0;
			returnValue = PerformChat(user,bot,ourMainInputBuffer,ip,ourMainOutputBuffer);	// this takes however long it takes, exclusive control of chatbot.
																							// special controls
			if (returnValue == PENDING_RESTART) // special messages
			{
				Restart();
				Log(SERVERLOG,"Server ready - logfile:%s serverLog:%d userLog:%d\r\n\r\n",serverLogfileName,oldserverlog,userLog);
				(*printer)((char*)"Server restarted - logfile:%s serverLog:%d userLog:%d\r\n\r\n",serverLogfileName,oldserverlog,userLog);
				char* at = SkipWhitespace(ourMainInputBuffer);
				if (*at != ':')	goto RESTART_RETRY;
				strcpy(ourMainOutputBuffer,"Restarted");
			}
			*((int*)clientBuffer) = returnValue;
		} // end try block on calling cs performchat
#ifdef WIN32
		__except(true)
#else
		catch (...)
#endif
		{
			char word[MAX_WORD_SIZE];
			sprintf(word, (char*)"Try/catch");
			Log(SERVERLOG, word);
			ReportBug("%s", word);
			serverLog = holdServerLog;
			longjmp(crashJump, 1);
		}

#ifdef WIN32
		__try{ // catch crashes in windows
#else
		try {
#endif
			ServerTransferDataToClient();
		}
#ifdef WIN32
		__except(true)
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
