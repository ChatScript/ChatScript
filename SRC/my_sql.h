#ifndef _MYSQLH
#define _MYSQLH
#ifdef INFORMATION
Copyright (C)2011-2018 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif

#ifndef DISCARDMYSQL

extern bool mysqlconf;
extern char mysqlhost[300];
extern unsigned int mysqlport;
extern char mysqldb[300];
extern char mysqluser[300];
extern char mysqlpasswd[300];

extern char *mysql_userread;
extern char *mysql_userinsert;
extern char *mysql_userupdate;
extern char my_userread_sql[300];
extern char my_userinsert_sql[300];
extern char my_userupdate_sql[300];

void MySQLUserFilesCode();
void MySQLShutDown();
void MySQLserFilesCloseCode();
extern char* mySQLparams;

FunctionResult MySQLInitCode(char* buffer);
FunctionResult MySQLCloseCode(char* buffer);
FunctionResult MySQLExecuteCode(char* buffer);
#endif

#endif
