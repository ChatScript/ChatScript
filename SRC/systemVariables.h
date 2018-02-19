#ifndef _SYSTEMVARIABLESH_
#define _SYSTEMVARIABLESH_

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

typedef char* (*SYSTEMVARPTR)(char* value); //   value is strictly overrride for testing

typedef struct SYSTEMVARIABLE
{
    const char* name;			// script name
    SYSTEMVARPTR address;		// function used to access it
    const char* comment;		// description
} SYSTEMVARIABLE;
extern SYSTEMVARIABLE sysvars[];
char* SFullTime(char* value);
void DefineSystemVariables();
void DumpSystemVariables();
char* SystemVariable(char* word,char* value);
extern char* compileDate;
#endif
