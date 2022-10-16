#ifndef MSSQL_H
#define MSSQL_H

#ifdef INFORMATION
/*
Copyright (C)2011-2020 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#endif
extern char mssqlparams[];

#ifdef  DISCARDMICROSOFTSQL
FunctionResult MsSqlScriptDummyInitCode(char* buffer);
FunctionResult MsSqlScriptDummyCloseCode(char* buffer);
FunctionResult MsSqlScriptDummyReadCode(char* buffer);
FunctionResult MsSqlScriptDummyWriteCode(char* buffer);
#endif

#ifndef DISCARDMICROSOFTSQL
const char* MsSqlVersion();
void MsSqlUserFilesCode(char* params); // file system init
void MsSqlFullCloseCode();

// script commands
FunctionResult MsSqlScriptInitCode(char* buffer);
FunctionResult MsSqlScriptCloseCode(char* buffer);
FunctionResult MsSqlScriptReadCode(char* buffer);
FunctionResult MsSqlScriptWriteCode(char* buffer);

#endif /* DISCARDMICROSOFTSQL */

// These are outside the DISCARDMICROSOFTSQL on purpose.
void malloc_mssql_buffer();
void free_mssql_buffer();

#endif	/* MSSQL_H */
