#ifdef INFORMATION
Copyright (C) 2011-2012 by Outfit7
Further modifed by Bruce Wilcox 2014-2018

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

/*
Copyright (C) 2011-2012Outfit7
by Igor Lautar <igor.lautar@outfit7.com>

Server implementation uses libev to trigger requests. Each request on a core is single-threaded
so there are no concurrency issues.
To compile, define EVSERVER during compilation, otherwise old pthread implementation is used.

Server listener:
 - listener socket is created in evsrv_init()
 - listener socket is added to libev for read event

Accepting requests:
 - libev triggers evsrv_accept
 - handler tries to accept as many connections as possible, creating Client_t instance for each
 - each client socket is registered in libev for read event

Handling client:
 - when data is available on client socket, client_read is called
 - it reads available data, if it is not enough it goes back to ev for more
 - if request is complete, it prepares the buffers and performs the chat
 - it immediatelly tries to send chat response, if socket is busy (EAGAIN) it adds write listener to ev
 - if write listener is triggered, client data is written
 - if there is still some data to write, write listener is added to ev again, until all data is written
 - once data is written, client instance is deregistered from libev, socket closed and object destroyed
*/
#include "common.h"
#ifdef WIN32
	#include "ev.h"
#else
	#include "evserver/ev.h"
#endif
#include "evserver.h"

#include <vector>
#include <algorithm>
extern char serverLogfileName[200];
extern char dbTimeLogfileName[200];
extern bool serverctrlz;
#define CLIENT_CHUNK_LENGTH 4*1024
#define HIDDEN_OVERLAP 103	// possible concealed data
#define HIDDEN_OFFSET 3 // past 2 ctrl z's

// server stuff
string interface_g;
int port_g;
int listen_queue_length_g = 16*1024; // 16 k

int srv_socket_g = -1;

// EV stuff
struct ev_loop *l_g = 0;
ev_io ev_accept_r_g;
ev_timer tt_g;

bool postgresInited = false;
	   
#ifdef EVSERVER_FORK
// child monitors
#define MAX_CHILDREN_D 50
ev_child children_g[MAX_CHILDREN_D];
int no_children_g = 0;
int cur_children_g = 0;
bool parent_g = true;
#endif

static void evsrv_accept(EV_P_ ev_io *w, int revents);
static void client_read(EV_P_ ev_io *w, int revents);
static void client_write(EV_P_ ev_io *w, int revents);
static void evsrv_child_died(EV_P_ ev_child *w, int revents);

struct Client_t;
int evsrv_do_chat(Client_t *client);

typedef vector<char> Buffer_t;

#define ITER_TO_OFFSET(ctr, i) (&ctr[0] + (i - ctr.begin()))

struct Client_t
{
    char magic[9];
    int fd;
    ev_io ev_r;
    ev_io ev_w;
    struct ev_loop *l;
    bool requestValid;
    string ip;
    char* bot;
    char* message;
    char* user;
    char* data = NULL;
    Buffer_t incomming;

    Client_t(int fd, struct ev_loop *l_p) : fd(fd), l(l_p), requestValid(false)
    {
        strcpy(this->magic, "deadbeef");
        ev_io_init(&this->ev_r, client_read, this->fd, EV_READ);
        ev_io_init(&this->ev_w, client_write, this->fd, EV_WRITE);
        this->ev_r.data = this;
        this->ev_w.data = this;
        ev_io_start(this->l, &this->ev_r);
    }

    ~Client_t() // if child dies, this destructor is not called
    {
		if (this->data) free(this->data); 
		this->data = NULL;
        if (ev_is_active(&this->ev_r))  ev_io_stop(this->l, &this->ev_r);
        if (ev_is_active(&this->ev_w))  ev_io_stop(this->l, &this->ev_w);
        close(this->fd);
    }
    
    void prepare_for_next_request()
    {
        this->incomming.clear();
        this->requestValid = false;
        this->bot = 0;
        this->message = 0;
        this->user = 0;
        if (this->data) *this->data = 0;
        if (!ev_is_active(&this->ev_r))  ev_io_start(this->l, &this->ev_r);
        if (ev_is_active(&this->ev_w))   ev_io_stop(this->l, &this->ev_w);
    }

    string get_foreign_address()
    {
        sockaddr_in addr;
        unsigned int addr_len = sizeof(addr);
        if (getpeername(this->fd, (sockaddr *) &addr,(socklen_t *) &addr_len) < 0)  return "";
        return inet_ntoa(addr.sin_addr);
    }

    int recv_data()
    {
        if (this->requestValid)
        {
            ReportBug((char*)"recv called although we got whole request, should process it first")
            return -1;
        }

        int read_size = CLIENT_CHUNK_LENGTH;
        size_t dataEnd = this->incomming.size();
        this->incomming.resize(dataEnd + read_size);

        char *read_ptr = &this->incomming[dataEnd];

        int r = recv(this->fd, read_ptr, read_size, 0);

        // make buffer as long as actual data
        if (r > 0)  this->incomming.resize(dataEnd + r);

        return r;
    }

    int send_data()
    {
		size_t len = strlen(this->data);
		if (serverctrlz) len += 3; // send end marker for positive confirmation of completion

        // try to send, if we get eagain error, schedule it for later
        int r = send(this->fd, this->data, len, 0);

        if (r < 0) 
		{
            if (errno == EAGAIN)
            {
                ev_io_start(this->l, &this->ev_w);
                return 0;
            }

            Log(SERVERLOG, "evserver: send_data(%d) could not send, errno: %s", len,strerror(errno));
			(*printer)( "evserver: send_data(%d) could not send, errno: %s", len,strerror(errno));
			return -1;
        }

        if (r < (int)len) 
		{
            ev_io_start(this->l, &this->ev_w);
			memmove(data,data+r,(len-r) + HIDDEN_OVERLAP);
            return 0;
        }

        // sent all data
        this->prepare_for_next_request();

        return 1;
    }

    int prepare_for_chat()
    {
        if (this->data) *this->data = 0;
        this->ip = this->get_foreign_address();
        if (this->ip.length() == 0) 
		{
            Log(SERVERLOG, "evserver: prepare_for_chat() could not get ip for client: %d\r\n", this->fd);
			(*printer)("evserver: prepare_for_chat() could not get ip for client: %d\r\n", this->fd);
			return -1;
        }

        return 1;
    }

    int received_request() {
        int nulls = count(this->incomming.begin(), this->incomming.end(), 0);
		if ((nulls) < 3)  return 0;
        if ((nulls) > 3)   return -1;

        this->requestValid = true;
        user = &this->incomming[0];

        if (strlen(user) == 0)  return -1;

		Buffer_t::iterator next_null;
		next_null = find(this->incomming.begin(), this->incomming.end(), 0);
		bot = ITER_TO_OFFSET(this->incomming, next_null + 1);
		next_null = find(next_null + 1, this->incomming.end(), 0);
		message = ITER_TO_OFFSET(this->incomming, next_null + 1);

        // since we received complete request, we will stop reading from client socket until we process it
        ev_io_stop(this->l, &this->ev_r);
        
        return 1;
    }
};

static int settcpnodelay(int fd)
{
    int on = 1;
#ifdef WIN32
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char*) &on, sizeof(on));
#else
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*) &on, sizeof(on));
#endif
}

static int setnonblocking(int fd)
{
    // non blocking (fix for windows)
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        Log(SERVERLOG, "evserver: setnonblocking() fcntl(F_GETFL) failed, errno: %s\r\n", strerror(errno));
		(*printer)( "evserver: setnonblocking() fcntl(F_GETFL) failed, errno: %s\r\n", strerror(errno));
		return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) 
	{
        Log(SERVERLOG, "evserver: setnonblocking() fcntl(F_SETFL) failed, errno: %s\r\n", strerror(errno));
		(*printer)( "evserver: setnonblocking() fcntl(F_SETFL) failed, errno: %s\r\n", strerror(errno));
		return -1;
    }
    return 1;
}

#ifdef EVSERVER_FORK
int fork_child(ev_child *child_watcher = 0)
{
    string curDir;
    pid_t pid = 0;
  
    pid = fork();
    if (pid < 0) {
		Log(SERVERLOG, "evserver: fork failed, errno %d\r\n", errno);
		(*printer)( "evserver: fork failed, errno %d\r\n", errno);
		return -1;
    }
 
    if (pid > 0) {
        // parent
        if (!child_watcher) {
            child_watcher = &children_g[cur_children_g];
            cur_children_g++;
        } 
		else   ev_child_stop(l_g, child_watcher);

        ev_child_init(child_watcher, evsrv_child_died, pid, 0);
        ev_child_start(l_g, child_watcher);

        return 0;
    }

    int b = false;
    if (!b)   sleep(1);

    // child
    ev_loop_fork(l_g);
#ifndef DISCARDJSONOPEN
    CurlShutdown();
#endif
    for (int i = 0; i < cur_children_g; i++)   ev_child_stop(l_g, &children_g[i]);
    cur_children_g = 0;
    parent_g = false;
    
    return 1;
}

char* currentDateTime() {
    struct tm ptm;
    return GetTimeInfo(&ptm);
}

static void evsrv_child_died(EV_P_ ev_child *w, int revents) {
    char* timestamp = currentDateTime();
    Log(SERVERLOG, "%s - evserver: evsrv_child_died [pid: %d]\r\n", timestamp,w->pid);
	(*printer)("%s - evserver: evsrv_child_died [pid: %d]\r\n", timestamp, w->pid);

	int r = fork_child(w);
	if (r < 0)
	{
		Log(SERVERLOG, "  %s - evserver: could not re-spawn child after it died [pid: %d]\r\n", timestamp, w->pid);
		(*printer)("  %s - evserver: could not re-spawn child after it died [pid: %d]\r\n", timestamp,w->pid);
	}

	else if (r == 1)
	{
		// socket listener
		ev_io_init(&ev_accept_r_g, evsrv_accept, srv_socket_g, EV_READ);
		ev_io_start(l_g, &ev_accept_r_g);
		Log(SERVERLOG, "  %s - evserver child: re-spawned [pid: %d]\r\n", timestamp, getpid());
		(*printer)("  %s - evserver child: re-spawned [pid: %d]\r\n", timestamp, getpid());
		if (forkcount > 1){ 
            sprintf(serverLogfileName, (char*)"%s/serverlog%d-%d.txt", logs, port_g, getpid());
            sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d-%d.txt", logs, port_g, getpid());
        }
	}
}
#endif

// init server
int evsrv_init(const string &interfaceKind, int port, char* arg) {
    if (srv_socket_g != -1) 
	{
        ReportBug((char*)"evserver: server already initialized\r\n")
        return -1;
    }
    if (arg) {
        // parse additional arguments
        // cut evsrv:
        string args(arg + 6);
    
        // parms are split by ','

        const char *s = args.c_str();
        const char *e = strchr(s, ',');

        while (true) {
            string command(s);

            if (e)  command = string(s, e - s);

            // now split arg/value
            const char *eq = strchr(s, '=');
            if (eq) {
                string option(s, eq - s);
                string val(eq + 1);

                if (e)  val = string(eq + 1, e - eq - 1);

                if (option == "fork") {
					no_children_g = atoi(val.c_str());
                    if (no_children_g > MAX_CHILDREN_D)  no_children_g = MAX_CHILDREN_D;
                }
            } 
			else  ReportBug((char*)"Invalid argument to evserver: '%s'\r\n", command.c_str());

            if (e) 
			{
                s = e + 1;
                if (!*s) break;
                e = strchr(e + 1, ',');
            } 
			else break;
        }
    }

    interface_g = interfaceKind;
    port_g = port;

    // uf, got socket, now do EV
    l_g = EV_DEFAULT;

#ifdef WIN32
	if (InitWinSock() == FAILRULE_BIT) 
	{
        Log(SERVERLOG, "evsrv_init: WSAStartup failed\r\n");
        return -1;
    }
#endif

    srv_socket_g = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    if (srv_socket_g < 0) {
        Log(SERVERLOG, "evsrv_init: socket() failed, errno: %s\r\n", strerror(errno));
        return -1;
    }

	int on = 1;
#ifdef WIN32
	setsockopt(srv_socket_g, SOL_SOCKET, SO_REUSEADDR, (char*) &on, sizeof(on));
#else
	setsockopt(srv_socket_g, SOL_SOCKET, SO_REUSEADDR, (void*) &on, sizeof(on));
#endif
    
    // non blocking
    if (setnonblocking(srv_socket_g) == -1)  
	{
		Log(SERVERLOG, "evsrv_init: non-blocking ending init\r\n");
 		return -1;
	}
	// bind the socket to its port
	sockaddr_in localAddr;
	memset(&localAddr, 0, sizeof(localAddr));
	localAddr.sin_family = AF_INET;
    if (!inet_aton(interface_g.c_str(), &localAddr.sin_addr)) {
        Log(SERVERLOG, "evsrv_init: inet_aton failed, errno: %s\r\n", strerror(errno));
        return -1;
    }
	localAddr.sin_port = htons(port_g);
    
	if (::bind(srv_socket_g, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) 
	{
 		return -1; // typical when server is already running and cron tries to start
	}

#ifdef EVSERVER_FORK
    int parent_after_fork = -1;
	Log(SERVERLOG, "evserver child fork requests: %d\r\n", no_children_g);
    if (no_children_g > 0) {
        cur_children_g = 0;
        // fork
        int forked = 0;
        for (int i = 0; i < no_children_g; i++) {
            forked = fork_child();
            if (forked == -1) { // failed
                // continue by ourself
                parent_after_fork = 1;
                break;
            } else if (forked == 1) {
                parent_after_fork = 0;
				bool oldserverlog = serverLog;
				serverLog = true;
                Log(SERVERLOG, "  evserver: child %d alive\r\n", getpid());
				serverLog = oldserverlog;
                break;
            } 
			else  parent_after_fork = 1;
        }
    }
#endif

    if (parent_after_fork == 1 && cur_children_g > 0)  return 1; // parent of child does not accept/handle connections

    if (listen(srv_socket_g, listen_queue_length_g) < 0) {
        Log(SERVERLOG, "evserver: listen() failed, errno: %s\r\n", strerror(errno));
        return -1;
    }

#if 0
    // test timer
    ev_timer_init(&tt_g, timeout_cb, 5, 5);
    ev_timer_start(l_g, &tt_g);
#endif

    // socket listener
    ev_io_init(&ev_accept_r_g, evsrv_accept, srv_socket_g, EV_READ);
    ev_io_start(l_g, &ev_accept_r_g);
	Log(SERVERLOG, "  evserver: running pid: %d\r\n",getpid());
    (*printer)("  evserver: running pid: %d\r\n", getpid());
    if (forkcount > 1) {
        sprintf(serverLogfileName, (char*)"%s/serverlog%d-%d.txt", logs, port, getpid());
        sprintf(dbTimeLogfileName, (char*)"%s/dbtimelog%d-%d.txt", logs, port_g, getpid());
    }
    return 1;
}

// starts main server loop
int evsrv_run()
{
    if (!l_g) 
	{
        ReportBug((char*)"evsrv_run() called with no ev loop initialized\r\n")
        (*printer)((char*)"no ev loop initialized, nothing to do\r\n");
        return -1;
    }
    if (parent_g) Log(SERVERLOG, "evserver: parent ready (pid = %d), fork=%d\r\n", getpid(), no_children_g);
	else Log(SERVERLOG, "  evserver: child ready (pid = %d)\r\n", getpid());
	if (!parent_g) (*printer)((char*)"EVServer ready: pid: %d %s\r\n",getpid(),serverLogfileName);
    while (true) ev_run(l_g, 0);
    return 1;
}

// handler for incomming connections
static void evsrv_accept(EV_P_ ev_io *w, int revents)
{
    int fd = -1;
    struct sockaddr_storage addr;
    socklen_t sock_len = sizeof(addr);
 
    // try to accept as many connections as possible
    while (true)
    {
        fd = accept(w->fd, (struct sockaddr *)&(addr), &sock_len);
        if (fd == -1) {
            if (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN) return; // cannot accept at this time, ev will call us again later

            if (errno == EMFILE || errno == ENFILE)  return; // per-process/system table full, should be freed shortly, just do nothing for now

            Log(SERVERLOG, "evserver: error accepting connection, errno: %s", strerror(errno));
            return;
        }

        if (settcpnodelay(fd) == -1) {
            Log(SERVERLOG, "evserver: could not set TCP_NODELAY, errno: %s", strerror(errno));
            return;
        }

        if (setnonblocking(fd) == -1 || setnonblocking(fd) == -1)  return;

        new Client_t(fd, l_g);
    }
}

static void client_read(EV_P_ ev_io *w, int revents)
{
    Client_t *client = (Client_t*)w->data;

    int r = client->recv_data();
    if (r < 0) {
        if (errno == EAGAIN)  return;

        Log(SERVERLOG, "evserver: got error on read (errno: %s) dropping client %d\r\n", strerror(errno), w->fd);
        delete client;
        return;
    }
    else if (r == 0) {
        // client closed connection, lets close ours
        delete client;
        return;
    }

    // ok, got some data, do we have complete request?
    r = client->received_request();
    if (r == 0)  return; // no, read some more data
    if (r < 0) {
        // invalid request
        Log(SERVERLOG, "evserver: received invalid request from %d, ignoring\r\n", w->fd);
        delete client;
        return;
    }

    // yes, we have, do the chat
    r = evsrv_do_chat(client);
    if (r < 0) {
        // could not process it
        delete client;
        return;
    }

    r = client->send_data();
	if (client->data)
	{
		free(client->data);
		client->data = NULL;
	}

    if (r < 0) {
        Log(SERVERLOG, "evserver: could not sent data to client: %d\r\n", client->fd);
        delete client;
        return;
    }

    if (r == 1)  delete client; // client handling finished

    // if r = 0, it means there is still some data to be sent, which will be done next time this watcher is woken-up by ev_loop
}

static void client_write(EV_P_ ev_io *w, int revents)
{
    Client_t *client = (Client_t*)w->data;

    int r = client->send_data();
    if (r < 0) {
        Log(SERVERLOG, "evserver: could not sent data to client: %d\r\n", client->fd);
        delete client;
        return;
    }

    if (r == 1) delete client; // client handling finished

    // if r = 0, it means there is still some data to be sent, which will be done next time this watcher is woken-up by ev_loop
}

void LogChat(uint64 starttime,char* user,char* bot,char* IP, int turn,char* input,char* output);

int evsrv_do_chat(Client_t *client)
{
 	uint64 starttime = ElapsedMilliseconds(); 
    client->prepare_for_chat();
	size_t len = strlen(client->message);
	if (len >= INPUT_BUFFER_SIZE - 300) client->message[INPUT_BUFFER_SIZE-300] = 0; // limit user input
	echo = false;
	bool restarted = false;
#ifndef DISCARDPOSTGRES
	if (*postgresparams && !postgresInited)  
	{
		PGUserFilesCode(); //Forked must hook uniquely AFTER forking
		postgresInited = true;
	}
#endif
#ifndef DISCARDMYSQL
	if (mysqlconf) MySQLUserFilesCode(); //Forked must hook uniquely AFTER forking
#endif

	if (!client->data) 	client->data = (char*) malloc(outputsize+8);
	if (!client->data) (*printer)("Malloc failed for child data\r\n");

RESTART_RETRY:
	strcpy(ourMainInputBuffer,client->message);
	struct tm ptm;
    char* dateLog = GetTimeInfo(&ptm,true)+SKIPWEEKDAY;
	if (serverPreLog && restarted)  Log(SERVERLOG,(char*)"ServerPre: retry pid: %d %s (%s) %s %s\r\n",getpid(),client->user,client->bot,ourMainInputBuffer, dateLog);
 	else if (serverPreLog)  Log(SERVERLOG,(char*)"ServerPre: pid: %d %s (%s) %s %s\r\n",getpid(),client->user,client->bot,ourMainInputBuffer, dateLog);
	int turn = PerformChat(
        client->user,
        client->bot,
        ourMainInputBuffer, // input
        (char*)client->ip.c_str(),
        client->data); // where output goes
	if (turn == PENDING_RESTART) // do user over again in a moment
	{
		restarted = true;
		Log(SERVERLOG,(char*)"Restart Request: pid: %d %s \r\n",getpid(),client->user);
		Restart();
		*client->data = 0;
		char* at = SkipWhitespace(ourMainInputBuffer);
		if (*at != ':')	goto RESTART_RETRY;
		strcpy(client->data,"Restarted");
	}
		
	if (*client->data == 0) 
	{
		client->data[0] = ' ';
		client->data[1] = 0;
		client->data[2] = 0xfe; //ctrl markers
		client->data[3] = 0xff;
		client->data[4] = 0;	// null terminate hidden why data after room for positive ctrlz
	}
	
#ifndef DISCARDPOSTGRES
		if (false && *postgresparams && postgresInited)  // try to keep going per child
		{
			PostgresShutDown(); // any script connection
			PGUserFilesCloseCode();	// filesystem
			postgresInited = false;
		}
#endif
	if (serverLog) LogChat(starttime,client->user,client->bot,(char*)client->ip.c_str(),turn,ourMainInputBuffer,client->data);
    return 1;
}

#endif /* EVSERVER */
#endif 
