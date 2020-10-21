# Routines to make tests for mssql interface

# zlib install linux
# sudo apt install zliblg-dev

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	ODBCLIB := -lodbc -lmsodbcsql-17
else
	ODBCLIB := -lodbc -lmsodbcsql.17
endif

all: imp_test
	./imp_test

SRC := ../mssql.cpp imp_test.cpp timer.cpp 
imp_test: $(SRC) Makefile
	g++ -o imp_test $(SRC) -DDISCARD_MSSQL_INTERFACE_FOR_TEST -g \
	$(ODBCLIB) -Wno-write-strings -Wdeprecated -fpermissive --std=c++11

clean:
	rm -f imp_test *.o *~ 

