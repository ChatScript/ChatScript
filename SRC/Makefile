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

# This is what a straight make will do.
default: debugserver

QUIET_WARNINGS=-Wno-unused-variable -Wno-char-subscripts -Wno-write-strings -Wno-unknown-pragmas -Wno-deprecated

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	ODBCLIB := -lodbc -lmsodbcsql-17 -lz
	LIBTREETAGGER := -ltreetagger
	RDYNAMIC := -rdynamic
	CENTOS_OS := $(shell grep 'CENTOS' '/etc/os-release')
	UBUNTU_OS := $(shell grep 'Ubuntu' '/etc/os-release')
	ARCH_OS   := $(shell grep 'Arch Linux' '/etc/os-release')
ifneq ("$(CENTOS_OS)", "")
	OS := 'CENTOS'
	LIBMECAB := -lmecab
endif
ifneq ("$(UBUNTU_OS)", "")
	OS := 'UBUNTU'
	UBUNTU_OS := $(shell grep 'UBUNTU' '/etc/os-release')
	LIBMECAB := -lmecab
endif
ifneq ("$(ARCH_OS)", "")
	OS := 'ARCH_OS'	
	LIBMECAB := -lmecab
endif
else
	OS := 'MAC'
	ODBCLIB := -lodbc -lmsodbcsql.17 -lz
	LIBTREETAGGER := -L../BINARIES -lmactreetagger
	LIBMECAB := -liconv -lmecab
endif

# store executable names in make variables so that cleanall target works

executable.server=../BINARIES/ChatScript
server: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1 -DDISCARD_JAPANESE=1
server: PGLOAD= -pthread -ldl
server: binary
server: EXECUTABLE=$(executable.server)
server: INCLUDEDIRS=-Ievserver -Iinclude  
server: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) $(RDYNAMIC) 

executable.debugserver=../BINARIES/ChatScriptDebug
debugserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMICROSOFTSQL=1 -DDISCARDMYSQL=1 -DTREETAGGER=1 -DDEBUG -DDISCARD_TEXT_COMPRESSION=1
debugserver: PGLOAD= -pthread
debugserver: INCLUDEDIRS=-Ievserver
debugserver: binary
debugserver: EXECUTABLE=$(executable.debugserver)
debugserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char  $(QUIET_WARNINGS) -g $(RDYNAMIC)
debugserver: LDFLAGS+=$(ODBCLIB) $(LIBMECAB) $(LIBTREETAGGER)

executable.standalone=../BINARIES/ChatScript
standalone: DEFINES+=  -DDISCARDSERVER=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
standalone: PGLOAD= -pthread
standalone: INCLUDEDIRS=-Ievserver
standalone: binary
standalone: EXECUTABLE=$(executable.standalone)
standalone: CFLAGS=-c -std=c++11  -Wall -funsigned-char  $(QUIET_WARNINGS)  -O1 $(RDYNAMIC)

executable.sharedobject=../BINARIES/ChatScript.so
sharedobject: DEFINES+= -DLOCKUSERFILE=1   -DDLL=1 -DJA=1 -DDISCARDSERVER=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DDISCARD_TEXT_COMPRESSION=1  -DTREETAGGER=1
sharedobject: PGLOAD= -pthread   -ldl
sharedobject: INCLUDEDIRS=-Ievserver -Iinclude  
sharedobject: binary
sharedobject: EXECUTABLE=$(executable.sharedobject)
sharedobject: CFLAGS=-c -std=c++11  -Wall -funsigned-char  $(QUIET_WARNINGS) -fPIC -g $(RDYNAMIC)
sharedobject: LDFLAGS+=-shared $(ODBCLIB) $(LIBMECAB) -lcurl -ltreetagger 

executable.mongoserver=../BINARIES/ChatScriptMongo
mongoserver: DEFINES+=  -DDISCARDPOSTGRES=1 -DDISCARDMYSQL=1  -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
mongoserver: PGLOAD= -pthread
mongoserver: binary
mongoserver: EXECUTABLE=$(executable.mongoserver)
mongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0 -I/include/mongo/bison  -I/include/mongo/mongoc 
mongoserver: CFLAGS=-c -std=c++11  -Wall -funsigned-char $(QUIET_WARNINGS)  -O1

executable.debugmongoserver=../BINARIES/ChatScriptMongoDebug
debugmongoserver: DEFINES+= -DDISCARDPOSTGRES=1  -DDISCARDMYSQL=1 -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
debugmongoserver: PGLOAD= -pthread -lmongoc-1.0 -lbson-1.0
mongoserver: LDFLAGS+= -lcrypt -lsasl2 -L/usr/local/lib -lmongoc-1.0 -lbson-1.0   -lodbc -lmsodbcsql.17 -Wno-write-strings -fpermissive --std=c++11 -lz
debugmongoserver: binary
debugmongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0
debugmongoserver: EXECUTABLE=$(executable.debugmongoserver)
debugmongoserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -g

executable.pgserver=../BINARIES/ChatScriptpg
pgserver: DEFINES+= -DDISCARDMONGO=1 -DDISCARDMYSQL=1  -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
pgserver: PGLOAD= -lpq -pthread
pgserver: binary
pgserver: EXECUTABLE=$(executable.pgserver)
pgserver: INCLUDEDIRS=-Ievserver -Ipostgres
pgserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char  $(QUIET_WARNINGS) -g $(RDYNAMIC)

executable.debugpgserver=../BINARIES/ChatScriptpgDebug
debugpgserver: DEFINES+=   -DDISCARDMONGO=1 -DDISCARDMYSQL=1  -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
debugpgserver: PGLOAD= -lpq -pthread
debugpgserver: binary
debugpgserver: EXECUTABLE=$(executable.debugpgserver)
debugpgserver: INCLUDEDIRS=-Ievserver -Ipostgres 
debugpgserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char ($QUIET_WARNINGS) -g

executable.mysqlserver=../BINARIES/ChatScriptMysql
mysqlserver: DEFINES+= -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
mysqlserver: PGLOAD= -pthread -lmysqlclient 
mysqlserver: binary
mysqlserver: EXECUTABLE=$(executable.mysqlserver)
mysqlserver: INCLUDEDIRS=-Ievserver 
mysqlserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -O1

executable.debugmysqlserver=../BINARIES/ChatScriptMysqlDebug
debugmysqlserver: DEFINES+= -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1   -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
debugmysqlserver: PGLOAD= -pthread -lmysqlclient 
debugmysqlserver: binary
debugmysqlserver: EXECUTABLE=$(executable.debugmysqlserver)
debugmysqlserver: INCLUDEDIRS=-Ievserver
debugmysqlserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -g

foo:
	echo "OS $(OS)"
	echo "LIBMECAB $(LIBMECAB)"

executable.mssqlserver=../BINARIES/ChatScriptMssql
mssqlserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1 
mssqlserver: PGLOAD= -pthread
mssqlserver: binary
mssqlserver: EXECUTABLE=$(executable.mssqlserver)
mssqlserver: INCLUDEDIRS=-Ievserver 
mssqlserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -O1
mssqlserver: LDFLAGS+=$(ODBCLIB) $(LIBMECAB) -ldl

executable.debugmssqlserver=../BINARIES/ChatScriptMssqlDebug
debugmssqlserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1
debugmssqlserver: PGLOAD= -pthread
debugmssqlserver: binary
debugmssqlserver: EXECUTABLE=$(executable.debugmssqlserver)
debugmssqlserver: INCLUDEDIRS=-Ievserver
debugmssqlserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -g
debugmssqlserver: LDFLAGS=$(ODBCLIB) $(LIBMECAB) -ldl -lcurl -g

executable.mysqlserver=../BINARIES/ChatScriptMysql
mysqlserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
mysqlserver: PGLOAD= -pthread -lmysqlclient 
mysqlserver: binary
mysqlserver: EXECUTABLE=$(executable.mysqlserver)
mysqlserver: INCLUDEDIRS=-Ievserver 
mysqlserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -O1

executable.debugmysqlserver=../BINARIES/ChatScriptMysqlDebug
debugmysqlserver: DEFINES+= -DLOCKUSERFILE=1  -DDEBUG -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1   -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1 
debugmysqlserver: PGLOAD= -pthread 
debugmysqlserver: INCLUDEDIRS=-Ievserver
debugmysqlserver: binary
debugmysqlserver: EXECUTABLE=$(executable.debugmysqlserver)
debugmysqlserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char  $(QUIET_WARNINGS) -g

executable.mongoserver=../BINARIES/ChatScriptMongo
mongoserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMYSQL=1  -DDISCARDMICROSOFTSQL=1  -DDISCARD_TEXT_COMPRESSION=1
mongoserver: PGLOAD= -pthread
mongoserver: binary
mongoserver: EXECUTABLE=$(executable.mongoserver)
mongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0
mongoserver: CFLAGS=-c -std=c++11  -Wall -funsigned-char $(QUIET_WARNINGS) -O1

executable.msdebugserver=../BINARIES/ChatScript
msdebugserver: DEFINES+= -DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 -DDISCARDMONGO=1 -DDISCARDMYSQL=1   -DTREETAGGER=1
msdebugserver: PGLOAD= -pthread $(ODBCLIB) -ldl
msdebugserver: binary
msdebugserver: EXECUTABLE=$(executable.server)
msdebugserver: INCLUDEDIRS=-Ievserver -Iinclude  -Izlib
msdebugserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -O1  $(RDYNAMIC) -Wunused-variable -Wunused-but-set-variable -g
msdebugserver: LDFLAGS+= $(ODBCLIB) -lz 

executable.debugmongoserver=../BINARIES/ChatScriptMongoDebug
debugmongoserver: DEFINES+= -DLOCKUSERFILE=1  -DDEBUG -DEVSERVER=1 -DEVSERVER_FORK=1  -DDISCARDPOSTGRES=1  -DDISCARDMYSQL=1 -DDISCARD_TEXT_COMPRESSION=1
debugmongoserver: PGLOAD= -lpq -pthread
debugmongoserver: binary
debugmongoserver: INCLUDEDIRS=-Iinclude -I/usr/local/include/libbson-1.0 -I/usr/local/include/libmongoc-1.0
debugmongoserver: EXECUTABLE=$(executable.debugmongoserver)
debugmongoserver: CFLAGS=-c -std=c++11 -Wall -funsigned-char $(QUIET_WARNINGS) -g

UNAME := $(shell uname)

CC=g++

ifeq ($(UNAME), Linux)
$(info ************ LINUX VERSION ************)
LDFLAGS= -L/usr/lib64 -L../BINARIES -lrt -lcurl --verbose $(PGLOAD)
endif

ifeq ($(UNAME), Darwin)
OSXRELEASE := $(shell defaults read loginwindow SystemVersionStampAsString)
$(info ************ MAC VERSION $OSXRELEASE detected ************)
LDFLAGS=-mmacosx-version-min=$(OSXRELEASE) -lcurl $(PGLOAD)
# mssqlserver: LDFLAGS+= -lodbc -lmsodbcsql.17 -Wno-write-strings -fpermissive --std=c++11 -lz
CFLAGS=-mmacosx-version-min=$(OSXRELEASE) -c  -Wall -funsigned-char $(QUIET_WARNINGS)
endif

ifeq ($(UNAME), FreeBSD)
$(info ************ FREEBSD VERSION ************)
DEFINES+= -DFREEBSD=1
INCLUDEDIRS+= -I/usr/local/include
LDFLAGS= -L/usr/local/lib -lrt -lcurl -lev  --verbose $(PGLOAD)
endif

SOURCES=constructCode.cpp csocket.cpp cs_ev.cpp cs_es.cpp cs_german.cpp cs_jp.cpp dictionarySystem.cpp duktape/duktape.cpp english.cpp englishTagger.cpp evserver.cpp factSystem.cpp functionExecute.cpp infer.cpp javascript.cpp jsmn.cpp json.cpp  mainSystem.cpp markSystem.cpp mongodb.cpp mssql.cpp mysql.cpp os.cpp outputSystem.cpp patternSystem.cpp postgres.cpp privatesrc.cpp scriptCompile.cpp secure.cpp spellcheck.cpp systemVariables.cpp tagger.cpp testing.cpp textUtilities.cpp tokenSystem.cpp topicSystem.cpp userCache.cpp userSystem.cpp variableSystem.cpp zif.cpp
OBJECTS=$(SOURCES:.cpp=.o)

default: server

clean:
	-rm -f $(OBJECTS)

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
	-rm -f $(executable.debserver)

binary: $(OBJECTS)
	@echo "Linking $@"
	$(CC) $(OBJECTS) $(LDFLAGS) $(INCLUDEDIRS) -o $(EXECUTABLE)

.cpp.o:
	$(CC) $(CFLAGS) $(DEFINES) $(INCLUDEDIRS) $< -o $@
