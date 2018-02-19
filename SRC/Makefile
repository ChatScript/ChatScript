.PHONY = clean binary server debugserver standalone pgserver debugpgserver mysqlserver debugmysqlserver mongoserver debugmongoserver foreignserver mongotreetaggerserver 

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
server: DEFINES+= -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1  -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 
server: PGLOAD= -pthread
server: INCLUDEDIRS=-Ievserver
server: binary
server: EXECUTABLE=$(executable.server)
server: CFLAGS=-c  -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing 

executable.debugserver=../BINARIES/ChatScriptDebug
debugserver: DEFINES+= -DLOCKUSERFILE=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1  -DDEBUG -DEVSERVER=1 -DEVSERVER_FORK=1 
debugserver: PGLOAD= -pthread
debugserver: INCLUDEDIRS=-Ievserver
debugserver: binary
debugserver: EXECUTABLE=$(executable.debugserver)
debugserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.standalone=../BINARIES/ChatScript
standalone: DEFINES+=  -DDISCARDSERVER=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DEVSERVER=1 -DEVSERVER_FORK=1
standalone: PGLOAD= -pthread
standalone: INCLUDEDIRS=-Ievserver
standalone: binary
standalone: EXECUTABLE=$(executable.standalone)
standalone: CFLAGS=-c -std=c++11  -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing 

executable.pgserver=../BINARIES/ChatScriptpg
pgserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1
pgserver: PGLOAD= -lpq -pthread
pgserver: binary
pgserver: EXECUTABLE=$(executable.pgserver)
pgserver: INCLUDEDIRS=-Ievserver -Ipostgres
pgserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing 

executable.debugpgserver=../BINARIES/ChatScriptpgDebug
debugpgserver: DEFINES+= -DLOCKUSERFILE=1  -DDEBUG -DEVSERVER=1 -DEVSERVER_FORK=1  -DDISCARDMONGO=1 -DDISCARDMYSQL=1
debugpgserver: PGLOAD= -lpq -pthread
debugpgserver: INCLUDEDIRS=-Ievserver -Ipostgres 
debugpgserver: binary
debugpgserver: EXECUTABLE=$(executable.debugpgserver)
debugpgserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.mysqlserver=../BINARIES/ChatScriptMysql
mysqlserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1
mysqlserver: PGLOAD= -lmysqlclient -pthread
mysqlserver: binary
mysqlserver: EXECUTABLE=$(executable.mysqlserver)
mysqlserver: INCLUDEDIRS=-Ievserver 
mysqlserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing 

executable.debugmysqlserver=../BINARIES/ChatScriptMysqlDebug
debugmysqlserver: DEFINES+= -DLOCKUSERFILE=1  -DDEBUG -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1  
debugmysqlserver: PGLOAD= -lmysqlclient -pthread 
debugmysqlserver: INCLUDEDIRS=-Ievserver
debugmysqlserver: binary
debugmysqlserver: EXECUTABLE=$(executable.debugmysqlserver)
debugmysqlserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.mongoserver=../BINARIES/ChatScriptMongo
mongoserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMYSQL=1
mongoserver: PGLOAD= -pthread
mongoserver: binary
mongoserver: EXECUTABLE=$(executable.mongoserver)
mongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0
mongoserver: CFLAGS=-c -std=c++11  -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.treetaggerserver=../BINARIES/ChatScriptTT
treetaggerserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMYSQL=1  -DDISCARDMONGO=1  -DTREETAGGER=1
treetaggerserver: PGLOAD= -pthread -L../BINARIES -ltreetagger
treetaggerserver: binary
treetaggerserver: EXECUTABLE=$(executable.treetaggerserver)
treetaggerserver: INCLUDEDIRS=-Iinclude 
treetaggerserver: CFLAGS=-c -std=c++11  -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.debugmongoserver=../BINARIES/ChatScriptMongoDebug
debugmongoserver: DEFINES+= -DLOCKUSERFILE=1  -DDEBUG -DEVSERVER=1 -DEVSERVER_FORK=1  -DDISCARDPOSTGRES=1  -DDISCARDMYSQL=1
debugmongoserver: PGLOAD= -lpq -pthread
debugmongoserver: binary
debugmongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0
debugmongoserver: EXECUTABLE=$(executable.debugmongoserver)
debugmongoserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing -g

executable.foreignserver=../BINARIES/ChatScriptForeign
foreignserver: DEFINES+= -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1  -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DTREETAGGER=1
foreignserver: PGLOAD= -pthread -L../treetagger -L../BINARIES -ltreetagger
foreignserver: INCLUDEDIRS=-Ievserver
foreignserver: binary
foreignserver: EXECUTABLE=$(executable.foreignserver)
foreignserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing 

executable.mongotreetaggerserver=../BINARIES/ChatScriptMongoForeign
mongotreetaggerserver: DEFINES+= -DLOCKUSERFILE=1  -DEVSERVER=1 -DEVSERVER_FORK=1  -DDISCARDPOSTGRES=1  -DDISCARDMYSQL=1 -DTREETAGGER=1
mongotreetaggerserver: PGLOAD= -pthread  -L../BINARIES -ltreetagger
mongotreetaggerserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0
mongotreetaggerserver: binary
mongotreetaggerserver: EXECUTABLE=$(executable.mongotreetaggerserver)
mongotreetaggerserver: CFLAGS=-c -std=c++11 -Wall  -funsigned-char  -Wno-write-strings -Wno-char-subscripts -Wno-strict-aliasing 

UNAME := $(shell uname)

CC=g++

ifeq ($(UNAME), Linux)
$(info ************ LINUX VERSION ************)
LDFLAGS= -L/usr/lib64 -lrt -lcurl  --verbose $(PGLOAD)
mongoserver: LDFLAGS+= -lcrypt -lsasl2 -L/usr/local/lib -lmongoc-1.0 -lbson-1.0 
debugmongoserver: LDFLAGS+= -lcrypt -lsasl2 -L/usr/local/lib -lmongoc-1.0 -lbson-1.0 
mongotreetaggerserver: LDFLAGS+= -lcrypt -lsasl2 -L/usr/local/lib -lmongoc-1.0 -lbson-1.0 
endif

ifeq ($(UNAME), Darwin)
OSXRELEASE := $(shell defaults read loginwindow SystemVersionStampAsString)
$(info ************ MAC VERSION $OSXRELEASE detected ************)
LDFLAGS=-mmacosx-version-min=$(OSXRELEASE) -lcurl $(PGLOAD)  
CFLAGS=-mmacosx-version-min=$(OSXRELEASE) -c  -Wall  -funsigned-char -Wno-write-strings -Wno-char-subscripts 
endif

ifeq ($(UNAME), FreeBSD)
$(info ************ FREEBSD VERSION ************)
DEFINES+= -DFREEBSD=1
INCLUDEDIRS+= -I/usr/local/include
LDFLAGS= -L/usr/local/lib -lrt -lcurl -lev  --verbose $(PGLOAD)
mongoserver: LDFLAGS+= -lgcrypt -lsasl2 -lmongoc-1.0 -lbson-1.0 
debugmongoserver: LDFLAGS+= -lgcrypt -lsasl2 -lmongoc-1.0 -lbson-1.0 
mongotreetaggerserver: LDFLAGS+= -lgcrypt -lsasl2 -lmongoc-1.0 -lbson-1.0 
endif

SOURCES=constructCode.cpp duktape/duktape.cpp evserver.cpp csocket.cpp cs_ev.c dictionarySystem.cpp englishTagger.cpp factSystem.cpp json.cpp functionExecute.cpp english.cpp infer.cpp javascript.cpp jsmn.cpp markSystem.cpp mysql.cpp mongodb.cpp os.cpp outputSystem.cpp patternSystem.cpp postgres.cpp privatesrc.cpp scriptCompile.cpp spellcheck.cpp secure.cpp systemVariables.cpp tagger.cpp testing.cpp textUtilities.cpp tokenSystem.cpp topicSystem.cpp userCache.cpp userSystem.cpp variableSystem.cpp mainSystem.cpp 
OBJECTS=$(SOURCES:.cpp=.o)

default: server

clean:
	-rm -f *.o

cleanall: clean
	-rm -f $(executable.server)
	-rm -f $(executable.debugserver)
	-rm -f $(executable.standalone)
	-rm -f $(executable.pgserver)
	-rm -f $(executable.debugpgserver)
	-rm -f $(executable.mysqlserver)
	-rm -f $(executable.debugmysqlserver)
	-rm -f $(executable.mongoserver)
	-rm -f $(executable.debugmongoserver)
	-rm -f $(executable.foreignserver)
	-rm -f $(executable.mongotreetaggerserver)
	-rm -f $(executable.treetaggerserver)

binary: $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) $(DEFINES) $(INCLUDEDIRS) -o $(EXECUTABLE)

.cpp.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDEDIRS) $< -o $@
