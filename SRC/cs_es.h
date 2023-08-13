#ifndef SPANISH_H
#define SPANISH_H
#ifdef INFORMATION
Copyright(C)2011 - 2022 by Bruce Wilcox

Permission is hereby granted, free of charge, to any person obtaining a copy of this softwareand associated documentation files(the "Software"), to deal
in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions :

The above copyright noticeand this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#endif
void C_ComputeSpanish(char* word);
uint64 GetSpanishLemmaBits(char* word);
void MarkSpanishTags(WORDP OL,int i);
uint64 KnownSpanishUnaccented(char* original, WORDP& entry,uint64& sysflags);
uint64 ComputeSpanish(int at, const char* original, WORDP& entry, WORDP& canonical, uint64& sysflags); // case sensitive, may add word to dictionary, will not augment flags of existing words
uint64 ComputeFilipino(int at, const char* original, WORDP & entry, WORDP & canonical, uint64 & sysflags);
void C_JaUnittest(char* word);

#endif // SPANISH_H
