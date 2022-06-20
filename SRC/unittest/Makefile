# Routines to make tests for mssql interface

# zlib install linux
# sudo apt install zliblg-dev

# Doc on makefile structure
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/

all: imp_test
	$(info Starting test)
	./imp_test

OBJDIR := obj

VPATH = . .. ../duktape ./$(OBJDIR)

SOURCES := \
	constructCode.cpp \
	cs_ev.cpp \
	cs_jp.cpp \
	csocket.cpp  \
	dictionarySystem.cpp  \
	duktape.cpp \
	english.cpp  \
	englishTagger.cpp  \
	evserver.cpp  \
	factSystem.cpp  \
	functionExecute.cpp  \
	infer.cpp  \
	javascript.cpp  \
	jsmn.cpp  \
	json.cpp  \
	mainSystem.cpp \
	markSystem.cpp  \
	mongodb.cpp  \
	mssql.cpp  \
	mysql.cpp  \
	os.cpp  \
	outputSystem.cpp  \
	patternSystem.cpp  \
	postgres.cpp  \
	privatesrc.cpp  \
	scriptCompile.cpp  \
	secure.cpp  \
	spellcheck.cpp  \
	systemVariables.cpp  \
	tagger.cpp  \
	testing.cpp  \
	textUtilities.cpp  \
	tokenSystem.cpp  \
	topicSystem.cpp  \
	userCache.cpp  \
	userSystem.cpp  \
	variableSystem.cpp  \
	zif.cpp  \
	test_mssql.cpp \
	test_scriptCompile.cpp \
	test_zif.cpp \
	test_main.cpp \
	test_jp.cpp \
	test_json.cpp \
	timer.cpp

OBJECTS_CPP=$(SOURCES:%.cpp=$(OBJDIR)/%.o)
OBJECTS=$(OBJECTS_CPP:%.c=$(OBJDIR)/%.o)

DEPDIR := deps
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

CXX := g++
CC := gcc
COMPILE.cpp = $(CXX) -c $(CPPFLAGS) $(CFLAGS) $(TARGET_ARCH) $(DEPFLAGS) 
COMPILE.c = $(CC) -c $(DEPFLAGS) $(CFLAGS) $(TARGET_ARCH)

QUIET_WARNINGS=-Wno-unused-variable -Wno-char-subscripts -Wno-write-strings \
	-Wno-unknown-pragmas -Wno-deprecated

DEFINES=-DLOCKUSERFILE=1 -DEVSERVER=1 -DEVSERVER_FORK=1 -DDISCARDPOSTGRES=1 \
	-DDISCARDMONGO=1 -DDISCARDMYSQL=1 -DNOMAIN

$(OBJDIR)/%.o : %.cpp $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.cpp) $(OUTPUT_OPTION) $<

$(OBJDIR)/%.o : %.c $(DEPDIR)/%.d | $(DEPDIR)
	$(COMPILE.c) $(OUTPUT_OPTION) $<

$(DEPDIR): ; @mkdir -p $@
$(OBJDIR): ; @mkdir -p $@

DEPFILES_CPP=$(SOURCES:%.cpp=$(DEPDIR)/%.d)
DEPFILES=$(DEPFILES_CPP:%.c=$(DEPDIR)/%.d)

$(DEPFILES):

CFLAGS := -Wall -funsigned-char $(QUIET_WARNINGS) $(DEFINES) 

CPPFLAGS := --std=c++11

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
$(info ************ LINUX VERSION ************)
ODBCLIB := -lodbc -lmsodbcsql-17
LDFLAGS= -L/usr/lib64 -lrt --verbose
LIBDIR := ../../BINARIES
endif

ifeq ($(UNAME), Darwin)
$(info ************ MAC VERSION $OSXRELEASE detected ************)
OSXRELEASE := $(shell defaults read loginwindow SystemVersionStampAsString)
ODBCLIB := -lodbc -lmsodbcsql.17
LDFLAGS=-mmacosx-version-min=$(OSXRELEASE)
LIBDIR := ../../BINARIES
ADDLIBS := -liconv
endif

LIBS = -L$(LIBDIR) $(ADDLIBS) /usr/local/lib/libmecab.a $(ODBCLIB) -lz -lcurl -lcppunit -g

include $(wildcard $(DEPFILES))

imp_test: $(OBJECTS) Makefile
	g++ -o imp_test $(OBJECTS) -g \
	$(LIBS) -Wno-write-strings -Wdeprecated -fpermissive --std=c++11

clean:
	rm -f imp_test *.o *~ deps/* obj/*

