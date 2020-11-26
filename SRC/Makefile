clean = .PHONY binary server debugserver standalone pgserver debugpgserver mysqlserver mssqlserver debugmysqlserver debugmssqlserver debugmssqlserver mongoserver debugmongoserver foreignserver mongotreetaggerserver 

# WARNING: optimization level -O3 is potentially hazardous
# I think I have seen code fail because of it.

# Removed the 'all' target.  It does not  make sense since each build target requires a different set of defines.
# So we need to clean before building each target in order compile in the right stuff.  
#
# E.g. to build pg and mysql:
#
# 1. make clean pgserver
# 2. make clean mysqlserver

# Note: to build mysql targets on Ubuntu:
# sudo apt-get install libmysqlclient-dev

# store executable names in make variables so that cleanall target works
executable.server=../BINARIES/ChatScript
server: DEFINES+=   -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1
server: PGLOAD= -pthread
server: INCLUDEDIRS=-Ievserver
server: binary
server: EXECUTABLE=$(executable.server)
server: CFLAGS= -c  -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -o2 -rdynamic

executable.debugserver=../BINARIES/ChatScriptDebug
debugserver: DEFINES+= -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1  -DDEBUG   -DEVSERVER=1 -DEVSERVER_FORK=1 
debugserver: PGLOAD= -pthread
debugserver: INCLUDEDIRS=-Ievserver
debugserver: binary
debugserver: EXECUTABLE=$(executable.debugserver)
debugserver: CFLAGS= -c  -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g 

executable.standalone=../BINARIES/ChatScript
standalone: DEFINES+=  -DDISCARDSERVER=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1 
standalone: PGLOAD= -pthread
standalone: INCLUDEDIRS=-Ievserver
standalone: binary
standalone: EXECUTABLE=$(executable.standalone)
standalone: CFLAGS=-c -std=c++11  -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g -rdynamic

executable.sharedobject=../BINARIES/ChatScript.so
sharedobject: DEFINES+=  -DDLL=1 -DDISCARDSERVER=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1 -DEVSERVER=1 -DEVSERVER_FORK=1
sharedobject: PGLOAD= -pthread
sharedobject: INCLUDEDIRS=-Ievserver
sharedobject: binary
sharedobject: EXECUTABLE=$(executable.sharedobject)
sharedobject: CFLAGS=-c -std=c++11  -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing  -fPIC -g
sharedobject: LDFLAGS=-shared -lcurl

executable.mongoserver=../BINARIES/ChatScriptMongo
mongoserver: DEFINES+=  -DDISCARDPOSTGRES=1 -DDISCARDMYSQL=1  -DDISCARDMICROSOFTSQL=1
mongoserver: PGLOAD= -pthread
mongoserver: binary
mongoserver: EXECUTABLE=$(executable.mongoserver)
mongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0 -I/include/mongo/bison  -I/include/mongo/mongoc 
mongoserver: CFLAGS=-c -std=c++11  -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing  -O1

executable.debugmongoserver=../BINARIES/ChatScriptMongoDebug
debugmongoserver: DEFINES+= -DDISCARDPOSTGRES=1  -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1
debugmongoserver: PGLOAD= -pthread -lmongoc-1.0 -lbson-1.0
mongoserver: LDFLAGS+= -lcrypt -lsasl2 -L/usr/local/lib -lmongoc-1.0 -lbson-1.0   -lodbc -lmsodbcsql.17 -Wno-write-strings -fpermissive --std=c++11 -lz
debugmongoserver: binary
debugmongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0
debugmongoserver: EXECUTABLE=$(executable.debugmongoserver)
debugmongoserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.pgserver=../BINARIES/ChatScriptpg
pgserver: DEFINES+= -DDISCARDMONGO=1 -DDISCARDMYSQL=1  -DDISCARDMICROSOFTSQL=1
pgserver: PGLOAD= -lpq -pthread
pgserver: binary
pgserver: EXECUTABLE=$(executable.pgserver)
pgserver: INCLUDEDIRS=-Ievserver -Ipostgres
pgserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g -rdynamic

executable.debugpgserver=../BINARIES/ChatScriptpgDebug
debugpgserver: DEFINES+=   -DDISCARDMONGO=1 -DDISCARDMYSQL=1  -DDISCARDMICROSOFTSQL=1
debugpgserver: PGLOAD= -lpq -pthread
debugpgserver: binary
debugpgserver: EXECUTABLE=$(executable.debugpgserver)
debugpgserver: INCLUDEDIRS=-Ievserver -Ipostgres 
debugpgserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.mysqlserver=../BINARIES/ChatScriptMysql
mysqlserver: DEFINES+= -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMICROSOFTSQL=1
mysqlserver: PGLOAD= -pthread -lmysqlclient 
mysqlserver: binary
mysqlserver: EXECUTABLE=$(executable.mysqlserver)
mysqlserver: INCLUDEDIRS=-Ievserver 
mysqlserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing  -O1

executable.debugmysqlserver=../BINARIES/ChatScriptMysqlDebug
debugmysqlserver: DEFINES+= -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1   -DDISCARDMICROSOFTSQL=1
debugmysqlserver: PGLOAD= -pthread -lmysqlclient 
debugmysqlserver: binary
debugmysqlserver: EXECUTABLE=$(executable.debugmysqlserver)
debugmysqlserver: INCLUDEDIRS=-Ievserver
debugmysqlserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	ODBCLIB := -lodbc -lmsodbcsql-17
else
	ODBCLIB := -lodbc -lmsodbcsql.17
endif

executable.mssqlserver=../BINARIES/ChatScriptMssql
mssqlserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 
mssqlserver: PGLOAD= -pthread $(ODBCLIB) -ldl
mssqlserver: binary
mssqlserver: EXECUTABLE=$(executable.mssqlserver)
mssqlserver: INCLUDEDIRS=-Ievserver 
mssqlserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing  -O1
mssqlserver: LDFLAGS+=$(ODBCLIB) -lz

executable.debugmssqlserver=../BINARIES/ChatScriptMssqlDebug
debugmssqlserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1
debugmssqlserver: PGLOAD= -pthread $(ODBCLIB) -ldl
debugmssqlserver: binary
debugmssqlserver: EXECUTABLE=$(executable.debugmssqlserver)
debugmssqlserver: INCLUDEDIRS=-Ievserver
debugmssqlserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g
debugmssqlserver: LDFLAGS=$(ODBCLIB) -lz -lcurl -g

UNAME := $(shell uname)

CC=g++

ifeq ($(UNAME), Linux)
$(info ************ LINUX VERSION ************)

DEFINES+= -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1 
# To add treetagger, comment above and uncomment below
# DEFINES+= -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1  -DTREETAGGER=1 

LDFLAGS= -L/usr/lib64 -lrt -lcurl   -L../BINARIES  -L/usr/local/lib  --verbose $(PGLOAD) -Wno-write-strings -fpermissive
# To add treetagger, comment above and uncomment below
# LDFLAGS= -L/usr/lib64 -lrt -lcurl   -L../BINARIES  -L/usr/local/lib  --verbose $(PGLOAD) -Wno-write-strings -fpermissive -ltreetagger

endif

ifeq ($(UNAME), Darwin)
OSXRELEASE := $(shell defaults read loginwindow SystemVersionStampAsString)
$(info ************ MAC VERSION $OSXRELEASE detected ************)

DEFINES+= -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1  
# To add treetagger, comment above and uncomment below
# DEFINES+= -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1  -DTREETAGGER=1

LDFLAGS=-mmacosx-version-min=$(OSXRELEASE) -lcurl  -L../BINARIES -L/usr/local/lib $(PGLOAD)    -lodbc -lmsodbcsql.17 -Wno-write-strings -fpermissive --std=c++11 -lz  
# To add treetagger, comment above and uncomment below
# LDFLAGS=-mmacosx-version-min=$(OSXRELEASE) -lcurl  -L../BINARIES -L/usr/local/lib $(PGLOAD)    -lodbc -lmsodbcsql.17 -Wno-write-strings -fpermissive --std=c++11 -lz   -ltreetagger

CFLAGS=-mmacosx-version-min=$(OSXRELEASE) -c  -Wall  -funsigned-char -Wno-write-strings -Wno-char-subscripts  -Wunused-variable
endif

ifeq ($(UNAME), FreeBSD)
$(info ************ FREEBSD VERSION ************)

DEFINES+= -DFREEBSD=1 -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1  
# To add treetagger, comment above and uncomment below
# DEFINES+= -DFREEBSD=1 -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1    -DTREETAGGER=1

LDFLAGS= -L/usr/local/lib -lrt -lcurl -lev   -L../BINARIES --verbose $(PGLOAD)  
# To add treetagger, comment above and uncomment below
# LDFLAGS= -L/usr/local/lib -lrt -lcurl -lev   -L../BINARIES --verbose $(PGLOAD)    -ltreetagger

INCLUDEDIRS+= -I/usr/local/include
endif

SOURCES=constructCode.cpp duktape/duktape.cpp evserver.cpp csocket.cpp cs_ev.c dictionarySystem.cpp englishTagger.cpp factSystem.cpp json.cpp functionExecute.cpp english.cpp infer.cpp javascript.cpp jsmn.cpp markSystem.cpp  mssql.cpp mysql.cpp mongodb.cpp os.cpp outputSystem.cpp patternSystem.cpp postgres.cpp privatesrc.cpp scriptCompile.cpp spellcheck.cpp secure.cpp systemVariables.cpp tagger.cpp testing.cpp textUtilities.cpp tokenSystem.cpp topicSystem.cpp userCache.cpp userSystem.cpp variableSystem.cpp mainSystem.cpp 
OBJECTS=$(SOURCES:.cpp=.o)

default: server

clean:
	-rm -f *.o

cleanall: clean
	-rm -f $(executable.server)
	-rm -f $(executable.debugserver)
	-rm -f $(executable.standalone)
	-rm -f $(executable.sharedobject)

	-rm -f $(executable.pgserver)
	-rm -f $(executable.debugpgserver)

	-rm -f $(executable.mysqlserver)
	-rm -f $(executable.debugmysqlserver)

	-rm -f $(executable.mongoserver)
	-rm -f $(executable.debugmongoserver)

	-rm -f $(executable.mssqlserver)
	-rm -f $(executable.debugmssqlserver)


binary: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(DEFINES) $(INCLUDEDIRS) -o $(EXECUTABLE)

.cpp.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDEDIRS) $< -o $@
