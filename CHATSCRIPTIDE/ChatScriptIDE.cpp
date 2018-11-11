// ChatScriptIDE.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "ChatScriptIDE.h"
#include <string>
#include <WindowsX.h>
#include "../SRC/common.h"
using namespace std;

/* entry points:
DebugCall
DebugAction
DebugVar
DebugMessage
DebugMark
*/

typedef struct WINDOWSTRUCT
{
    HWND window;
    int linesPerPage;
    int charsPerPage;
    int maxLines;
    int maxChars;
    RECT rect;
    SIZE metrics;
    int xposition;
    int yposition;
} WINDOWSTRUCT;

WINDOWSTRUCT scriptData, statData, varData, parentData, sentenceData,stackData, inputData,outputData;

#define NO_BREAK 0
#define BREAK_CALL 1
#define BREAK_ACTION 2
#define BREAK_VAR 3
#define BREAK_MSG 4

typedef struct MAPDATA
{
    uint64 botid;  // restriction if any
    MAPDATA* next; // maps are kept in order start to finish of a file,
    char* filename;
    int fileline;
    char* name;
} MAPDATA;

typedef struct LINEDATA
{
    LINEDATA* next;
    LINEDATA* prior;
    MAPDATA* mapentry;
    char text[1]; // actually it can be any size
} LINEDATA;

typedef struct FILEDATA
{
    FILEDATA* next;
    char* filename;
    LINEDATA* filelines; // actually, char*, char*, linecharacters
    MAPDATA* map;       // start of map info on this file
    char* status;
    int charsize;
    int lineCount;
} FILEDATA;

typedef struct CLICKLIST
{
    FILEDATA* file;
    CLICKLIST* next;
    int yposition;
    int mousey;
} CLICKLIST;
static void UpdateWindowMetrics(WINDOWSTRUCT& data, bool font = false);
CLICKLIST* clickList = NULL;
static void AddTab(char* name);
FILEDATA* currentFileData = NULL;
static int offsetCode = -1;
int breakType = NO_BREAK;
static bool WhereAmI(int depth);
static char breakAt[400][40];
static int rulesInMap = 0;
static int breakIndex = 0;
int sentenceMode = 0;
static bool disableBreakpoints = false;
static bool sysfail = false;
static CALLFRAME* ownerFrame;
static int nextdepth = -1;
static int nextlevel = -1;
static int idedepth = -1;
static int ideoutputlevel = -1;
static char* fnvars = NULL;
static int fnlevel = -1;
static void ClearStops();
static uintptr_t csThread = 0;
static MAPDATA* lastItem = NULL;
static char filenames[1000];
static char varnames[1000];
static char varbreaknames[1000];
#define MAX_VARVALUES 20
static char varname[MAX_VARVALUES+1][50];
static char vartest[MAX_VARVALUES + 1];
static int varnamesindex = 0;
static char varvalue[MAX_VARVALUES+1][50];
static MAPDATA* firstMap = NULL;
static MAPDATA* priorMapLine = NULL;
static char* DebugInput(char* input);
static void MakeCurrentFile(char* name,uint64 botid);
static char windowTitle[MAX_WORD_SIZE];
static int breakpointCount = 0;
static FILEDATA* fileList = NULL;
static char varAt[100][40];
static int varIndex = 0;
LINEDATA* firstline = NULL;
LINEDATA* priorline = NULL;
LINEDATA* line = NULL;
static int outlevel = 0;
static bool globalAbort = false;

#define MAX_LOADSTRING 100
#define ID_EDITCHILD 100 // output
#define ID_EDITCHILD1 101 //  input
HWND hParent = NULL;
HWND  ioWindow = NULL;
static void RemoveBreak(char* name);
HWND hGoButton, hStopButton, hClearButton,hNextButton, hInButton, hOutButton;
HWND hGlobalsButton,hBackButton,hBreakMessageButton;
HWND hFontButton,hFailButton;
int fontsize = 30;
size_t editLen = 0;
bool changingEdit = false;
SCROLLINFO si;
static int ProcessBreak(char* input,int flags);
HFONT hFont,hFontBigger,hButtonFont;
HPEN hPen;
HBRUSH hBrush;
HBRUSH hBrushBlue;
HBRUSH hBrushOrange;
int mouseLine = -1;
int codeLine = -1; // where are we
FILEDATA* codeFile = NULL;
int codeOffset = -1;
char* codePtr = NULL;
int mouseCharacter = -1;
char transientBreak[200];
RECT titlerect;
RECT numrect;
RECT tagrect;
static char* DebugVar(char* name, char* value);
static char* DebugMark(char* name, char* value);
int outdepth = -1;
static char* DebugOutput(char* output);

// Global Variables:
HINSTANCE hInst;                                // current instance
char szTitle[MAX_LOADSTRING];                  // The title bar text
char szWindowClass[MAX_LOADSTRING];            // the main scriptData.window class name
char* DoneCallback(char* buffer);

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance,  LPCSTR szWindowClass);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
static FILEDATA* GetFile(char* name,uint64 botid);
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
	if (!SetCurrentDirectory((LPCSTR)".."))
	{
		printf("no change dir");
	}
    // TODO: Place code here.

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_CHATSCRIPTIDE, szWindowClass, MAX_LOADSTRING);
    ATOM j = MyRegisterClass(hInstance,(LPCSTR) szWindowClass);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHATSCRIPTIDE));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int) msg.wParam;
}

static MAPDATA* FindFileMap(char* filename,uint64 botid)
{
    MAPDATA* at = firstMap;
    while (at)
    {
        // map has link, file, line, name
        // we match either full path or tail naming only
        if ((at->botid == botid || at->botid & botid) && 
            (!stricmp(at->name, filename) || !stricmp(at->filename, filename)))
        {
            return at;
        }
        at = at->next; // use forward ptr
    }
    return NULL;
}

static MAPDATA* FindLine(int line)
{
    return NULL;
}

static void ReadWindow(HWND window,FILE* in)
{
    char buffer[500];
    ReadALine(buffer,in);
    char* data = buffer;
    char word[MAX_WORD_SIZE];
    data = ReadCompiledWord(data, word);
    data = ReadCompiledWord(data, word);
    int x = atoi(word);
    data = ReadCompiledWord(data, word);
    int y = atoi(word);
    data = ReadCompiledWord(data, word);
    int z = atoi(word);
    data = ReadCompiledWord(data, word);
    int q = atoi(word);
    MoveWindow(window, x, y, z, q, true);
}

static void RestoreWindows()
{
    FILE* in = FopenReadOnly("idesettings.txt");
    if (!in) return;
    ReadWindow(scriptData.window, in);
    UpdateWindowMetrics(scriptData, true);

    ReadWindow(inputData.window, in);
    ReadWindow(outputData.window, in);
    UpdateWindowMetrics(outputData, true);
    ReadWindow(statData.window, in);
    UpdateWindowMetrics(statData, true);

    ReadWindow(varData.window, in);
    UpdateWindowMetrics(varData, true);
    
    ReadWindow(stackData.window, in);
    UpdateWindowMetrics(stackData, true);

    ReadWindow(hGoButton, in);
    ReadWindow(hInButton, in);
    ReadWindow(hOutButton, in);
    ReadWindow(hNextButton, in);
    ReadWindow(hStopButton, in);
    ReadWindow(hBreakMessageButton, in);
    ReadWindow(hClearButton, in);
    ReadWindow(hGlobalsButton, in);
    ReadWindow(hBackButton, in);
    ReadWindow(hFontButton, in);
    ReadWindow(hFailButton, in);
    char buffer[100];
    ReadALine(buffer, in);
    fontsize = atoi(buffer + 10);
    fclose(in);
}

static void SaveWindow(HWND window,char* name,FILE* out)
{
    RECT rect;
    GetWindowRect(window,&rect);
    POINT pt;
    pt.x = rect.left;
    pt.y = rect.top;
    ScreenToClient(GetParent(window),&pt);
    fprintf(out, "%s: %d %d ", name, pt.x, pt.y);
    fprintf(out, "%d %d \r\n", rect.right - rect.left, rect.bottom-rect.top);
}

static void SaveWindows()
{
    FILE* out = FopenBinaryWrite("idesettings.txt");
    if (!out) return;
    SaveWindow(scriptData.window, "script",out);
    SaveWindow(inputData.window, "input", out);
    SaveWindow(outputData.window, "output", out);
    SaveWindow(statData.window, "statistics", out);
    SaveWindow(varData.window, "variables", out);
    SaveWindow(stackData.window, "stack", out);
    SaveWindow(hGoButton, "go", out);
    SaveWindow(hInButton, "in", out);
    SaveWindow(hOutButton, "out", out);
    SaveWindow(hNextButton, "next", out);
    SaveWindow(hStopButton, "stop", out);
    SaveWindow(hBreakMessageButton, "msgbreak", out);
    SaveWindow(hClearButton, "clear", out);
    SaveWindow(hGlobalsButton, "globals", out);
    SaveWindow(hBackButton, "back", out);
    SaveWindow(hFontButton, "font", out);
    SaveWindow(hFailButton, "fail", out);
    fprintf(out, "fontsize: %d\r\n", fontsize);
    fclose(out);
}

static void DumpMaps()
{
    FILE* out = FopenBinaryWrite("tmp/tmp.txt");
    MAPDATA* map = firstMap;
    while (map)
    {
        fprintf(out, "%d %s %s\r\n", map->fileline, map->filename, map->name);
        map = map->next;
    }
    fclose(out);
}

static MAPDATA* FindMap(char* name,char* filename)
{
    MAPDATA* at = firstMap;
    char* oldname = NULL;
    while (at)
    {
        if (at->filename != oldname)
        {
            oldname = at->filename;
            Log(STDTRACELOG, "%s\r\n", oldname);
        }
        char* entry = at->name; // map has link, file, line, name
        if (filename && stricmp(at->filename, filename)) { ; }
        else
        {
            if (*entry == '^' || *entry == '~') priorMapLine = at;
            if (!stricmp(name, entry)) 
                return at;
        }
        at = at->next; // use forward ptr
    }
    return NULL;
}


static MAPDATA* FindLabelMap(char* label, uint64 botid)
{
    MAPDATA* at = firstMap;
    while (at)
    {
        // map has link, file, line, name
        // we match either full path or tail naming only
        if ((at->botid == botid || at->botid & botid) &&
            *at->name == '~')
        {
            char* hyphen = strchr(at->name, '-');
            if (hyphen && strchr(at->name,'.')) // tag + label
            {
                if (!stricmp(hyphen + 1, label))
                    return at;
            }
        }
        at = at->next; // use forward ptr
    }
    return NULL;
}

static void DefineVertScroll(WINDOWSTRUCT& data, int pos)
{
    if (!data.window) return;
    si.fMask = SIF_POS | SIF_RANGE;
    if (pos >= 0) si.nPos = pos;
    si.nMin = 0;
    si.nMax = data.maxLines;
    SetScrollInfo(data.window, SB_VERT, &si, true);
    InvalidateRgn(data.window, NULL, true);
}

static char* ShowActions(char* actions,int level)
{
    static char src[100];
    sprintf(src, "lvl:%d  ", level);
    char* at = src + strlen(src);
    strncpy(at, actions, 40);
    at[40] = 0;
    at = strchr(at, '`');
    if (at) *at = 0;
    return src;

}

static char* GetDefn(int depth)
{
    CALLFRAME* frame = GetCallFrame(depth);
    if (*frame->label != '^') return NULL;  // not a callframe
    char* definition = frame->definition;
    if (!definition) return NULL;
    return ++definition; // ( xxx xxx )
}

static void StackVariables(int depth)
{
    fnvars = NULL;
    fnlevel = -1;
    if (depth <= 0 || depth > globalDepth) return;
    fnvars = GetDefn(depth);
    sprintf(windowTitle, "local variables");
    SetWindowText(varData.window, (LPCTSTR)windowTitle);
    fnlevel = depth;
    // now at "(xxx xxx xxx )" argument list + locals
}

static void BackClick()
{
    if (!clickList) return;
    CLICKLIST* click = clickList;
    clickList = click->next;
    mouseLine = click->mousey;
    scriptData.yposition = click->yposition;
}

static void AddClick()
{
    // already there
    if (clickList && clickList->file == currentFileData &&
        clickList->yposition == scriptData.yposition) return;

    CLICKLIST* click = (CLICKLIST*)malloc(sizeof(CLICKLIST));
    click->next = clickList;
    clickList = click;
    click->file = currentFileData;
    click->yposition = scriptData.yposition;
    click->mousey = mouseLine;
}

static void RestoreClick()
{
    if (!clickList) return;
    CLICKLIST* click = clickList;
    clickList = clickList->next;
    scriptData.yposition = click->yposition;
    mouseLine = click->mousey;
    currentFileData = click->file;
    AddTab(click->file->filename);
    scriptData.maxLines = currentFileData->lineCount;
    scriptData.maxChars = currentFileData->charsize;
    free(click); 
    InvalidateRgn(scriptData.window, NULL, true);
}

static void FreeClicklist()
{
    while (clickList)
    {
        CLICKLIST* next = clickList->next;
        free(clickList);
        clickList = next;
    }
}

static char* ChaseFnVariable(char* name, int mydepth)
{
    if (globalDepth <= 0) return NULL; // not stopped in execution

    int depth = (mydepth <= 0) ? 1 : mydepth; // go to current
    char label[MAX_WORD_SIZE];
    sprintf(label, "%s ", name);
    CALLFRAME* frame = GetCallFrame(depth); 
    if (*name == '$') while (++depth <= globalDepth) // go deeper to see
    {
        frame = GetCallFrame(depth);
        if (*frame->label == '~' && strchr(frame->label, '.')) continue;
        char* defn = frame->definition;
        if (!defn) continue;
        char* end = strchr((char*)defn, ')');
        *end = 0;
        // find variable in list
        char* found = strstr((char*)defn, label);
        *end = ')';
        if (!found) continue;   // not used by this

        // find which display arg it is
        *found = 0;
        int n = 0;
        char* at = (char*)defn+2;
        char arg[MAX_WORD_SIZE];
        while (*at)
        {
            at = ReadCompiledWord(at, arg);
            if (*arg == USERVAR_PREFIX) ++n; // we dont count ^args
        }
        *found = *label;

        // need the nth arg old display value
        char* val = frame->display[n];
        return (!*val) ? "null" : val;
    }
    if (*name == '^')
    {
        // find which display arg it is
        char* defn = frame->definition; 
        if (!defn) return "null"; // not yet visible
        char* at = (char*)defn + 2;
        char arg[MAX_WORD_SIZE];
        int n = 1; // ARGUMENT(1)
        while (*at)
        {
            at = ReadCompiledWord(at, arg);
            if (!stricmp(arg, name)) break;
            ++n; 
            if (n > frame->arguments)  return "null"; // not there, may not have been even called
        }

        char* answer = callArgumentList[frame->varBaseIndex + n];
        return answer;
    }
    return GetUserVariable(name); // current value is correct
}

static void DefineHorzScroll(WINDOWSTRUCT& data, int pos)
{
    if (!data.window) return;
    si.fMask = SIF_POS | SIF_RANGE;
    if (pos >= 0) si.nPos = pos;
    si.nMin = 0;
    si.nMax = data.maxChars;
    SetScrollInfo(data.window, SB_HORZ, &si, true);
    InvalidateRgn(data.window, NULL, true);
}

static void ShowStack()
{
    if (stackData.window) // compute scroll data for stack window
    {
        int maxchar = 0;
        for (int i = 1; i <= globalDepth; ++i)
        {
            CALLFRAME* frame = GetCallFrame(i);
            size_t len = strlen(frame->label);
            if (len > maxchar) maxchar = len;
        }
        stackData.maxChars = maxchar;
        stackData.maxLines = globalDepth;
        stackData.xposition = 0;
        if (globalDepth > stackData.linesPerPage)
            stackData.yposition = globalDepth - stackData.linesPerPage;

        DefineVertScroll(stackData, stackData.yposition);
        DefineHorzScroll(stackData, 0);
        InvalidateRgn(stackData.window, NULL, true);

        StackVariables(globalDepth);   
        InvalidateRgn(varData.window, NULL, true);
    }
}

static void UseGlobals()
{
    fnvars = NULL; // back to globals
    fnlevel = -1;
    SetWindowText(varData.window, "global variables");
    InvalidateRgn(varData.window, NULL, true);
}

static void ShowVariables()
{
    UseGlobals();
}

char* DoneCallback(char* buffer)
{
    sprintf(windowTitle, "%s=>%s (%I64u)", loginID, computerID,myBot);
    SetWindowText(scriptData.window, (LPCTSTR)windowTitle);
    ShowVariables();

    stackData.maxLines = 0;
    stackData.maxChars = 0;
    DefineVertScroll(stackData, 0);
    DefineHorzScroll(stackData, 0);

    // no reason to update stack window, we have finished volley
    return buffer;
}

static void UpdateWindowMetrics(WINDOWSTRUCT& data,bool font)
{
    HDC dc = GetDC(data.window);
    if (font) SelectObject(dc, hFontBigger);
    else SelectObject(dc, hFont);
    GetTextExtentPoint32(dc, (LPCTSTR)"12TzqbABCDEFGHIJKLMNOPQRSTUVWXYZ", 32, &data.metrics);
    ReleaseDC(data.window, dc);
 
    data.metrics.cx /= 32;
    data.metrics.cx += 1;
    data.charsPerPage = ((data.rect.right - data.rect.left) / data.metrics.cx) - 1;
    data.linesPerPage = ((data.rect.bottom - data.rect.top) / data.metrics.cy) ;
    si.fMask = SIF_RANGE | SIF_PAGE;
    si.nPage = data.charsPerPage;
    si.nMin = 0;
    si.nMax = data.maxChars;
    SetScrollInfo(data.window, SB_HORZ, &si, false);

    // we can display this many lines
    si.nPage = data.linesPerPage;
    si.nMin = 0;
    si.nMax = data.maxLines;
    SetScrollInfo(data.window, SB_VERT, &si, false);
}

static int FindVarBreakValue(char* var)
{
    // break only on value
    for (int i = 0; i < varnamesindex; ++i)
    {
        if (!stricmp(var, varname[i])) return i;
    }
    return -1;
}

static void UpdateMetrics()
{
    UpdateWindowMetrics(scriptData);
    scriptData.rect.top = scriptData.metrics.cy;
    titlerect.bottom = scriptData.metrics.cy;
    InvalidateRgn(scriptData.window, NULL, true);

    if (varData.window)
    {
        UpdateWindowMetrics(varData);
        InvalidateRgn(varData.window, NULL, true);
    }

    if (outputData.window)
    {
        UpdateWindowMetrics(outputData);
        InvalidateRgn(outputData.window, NULL, true);
    }
    if (statData.window)
    {
        UpdateWindowMetrics(statData);
        InvalidateRgn(statData.window, NULL, true);
    }

    if (stackData.window)
    {
        UpdateWindowMetrics(stackData);
        InvalidateRgn(stackData.window, NULL, true);
    }
}

void EraseVariable(int which)
{
    if (!*varnames) return;
    char* var = varnames;
    size_t len;
    char name[MAX_WORD_SIZE];
    char* end;
    while (*var)
    {
        // get next variable name
        end = strstr(var, "\r\n");
        if (!end) break; *end = 0;
        if (which-- == 0) break;
        *end = '\r';
        var = end + 2;
    }
    ReadCompiledWord(var, name);
    if (end) *end = '\r';
    char junk[MAX_WORD_SIZE];
    sprintf(junk, "%s\r\n", name);

    char* found = strstr(varnames, junk);
    if (!found) return;
    len = strlen(name);
    --varData.maxLines;
    char* after = found + len + 2;
    memmove(found, after, strlen(after) + 2); // remove name
}

static void VerifyMap()
{
    FILEDATA* file = fileList;
    int count = 0;
    bool found = false;

    while (file)
    {
        LINEDATA* fileLines = file->filelines;
        while(fileLines)
        {
            MAPDATA* map = fileLines->mapentry;
            if (map->name && (map->name, "control.9")) found = true;
            ++count;
            fileLines = fileLines->next;
        }
        file = file->next;
    }
    if (found)
    {
        int xx = 0;
    }
    else
    {
        int xx = 0;
    }
}

static void DoBreakVariable(int which)
{
    if (!*varnames && !fnvars)
    {
        return;
    }
    char* var = (fnvars) ? fnvars : varnames;
    size_t len;
    char name[MAX_WORD_SIZE];
    char* end;
    while (*var)
    {
        // get next variable name
        end = strstr(var, "\r\n");
        if (!end) end = strstr(var, " "); // fnvars
        if (!end) end = strstr(var, ")"); // fnvars
        if (!end) break;
        *end = 0;
        if (which-- == 0) break;
        *end = '\r';
        var = end + 2;
    }
    ReadCompiledWord(var, name);
    if (*name == '^')
    {
        DebugPrint("cannot breakpoint a ^var\r\n");
        return; // cannot break on ^var
    }
    if (end) *end = '\r';
    char junk[MAX_WORD_SIZE];
    sprintf(junk, "%s\r\n", name);

    char* found = strstr(varbreaknames, junk);
    len = strlen(name);
    if (found) // remove name from existing list
    {
        char* after = found + len + 2;
        memmove(found, after, strlen(after) + 2); // remove name
        if (!*varbreaknames) debugVar = NULL;
        return;
    }

    // insert at start of list
    memmove(varbreaknames + len + 2, varbreaknames, strlen(varbreaknames) + 1);
    memmove(varbreaknames, name, len);
    varbreaknames[len] = '\r';
    varbreaknames[len + 1] = '\n';
    debugVar = DebugVar;
}

static void AddVariable(char* name)
{
    char junk[MAX_WORD_SIZE];
    sprintf(junk, "%s\r\n", name);
    MakeLowerCase(junk);
    char* found = strstr(varnames, junk);
    size_t len = strlen(name);
    if (found) // remove name from existing list
    {
        --varData.maxLines;
        char* after = found + len + 2;
        memmove(found, after, strlen(after) + 2); // remove name
    }
    // insert at start of list
    memmove(varnames + len + 2, varnames, strlen(varnames) + 1);
    memmove(varnames, junk, len);
    varnames[len] = '\r';
    varnames[len + 1] = '\n';
    ++varData.maxLines;

    DefineVertScroll(varData, 0);

    if (*loginID && !csThread) ReadUserData(); // but not if suspended or unstarted
    ShowVariables();
    if (!csThread) ResetToPreUser();// but not if suspended
}

static int NameLine(char* name)
{
    MAPDATA* symbol = FindMap(name, NULL); // find name in any file
    if (!symbol) return -1;
    char* file = symbol->filename;
    MakeCurrentFile(file,symbol->botid);
    return symbol->fileline;
}

uint64 FindBotID(char* name)
{
    char id[MAX_WORD_SIZE];
    MakeLowerCopy(id,name);
    strcat(id, "`bot");
    MAPDATA* at = firstMap;
    while (at)
    {
        char* entry = at->name; // map has link, file, line, name
        if (!stricmp(id, entry))
            return at->botid;
        at = at->next; // use forward ptr
    }
    return (uint64)(int64)-1;
}

static void RemoveTab(char* name)
{
    char fullname[1000];
    sprintf(fullname, " %s ",name);
    char* found = strstr(filenames, fullname);
    size_t len = strlen(fullname);
    if (found) // remove short name + space from existing list
    {
        char* after = found + len - 1;
        memmove(found, after, strlen(after) + 1); // remove name
        if (filenames[1] == ' ') // if it was leader, change leader
        {
            memmove(filenames, filenames + 1, strlen(filenames) + 1);
            char word[MAX_WORD_SIZE];
            ReadCompiledWord(filenames, word);
            AddTab(word);   // reinsert properly
        }
        InvalidateRgn(scriptData.window, NULL, true);
    }
}

static void AddTab(char* name)
{
    RemoveTab(name);
    size_t len = strlen(name);

    // remove 2 extra spaces on separator, make single space
    char old[1000];
    char* triple = strstr(filenames, "   ");
    if (triple)
    {
        strcpy(old, triple + 4);
        strcpy(triple + 1,old);
    }
    strcpy(old, filenames);
    *filenames = ' ';
    strcpy(filenames + 1, name);
    strcat(filenames, "    ");
    if (*old) strcat(filenames, old + 1); //leave off extra space
}

MAPDATA* AlignName(char* name)
{
    char namex[1000];
    strcpy(namex, name);
    char* at = strchr(namex, '(');
    if (at) *at = 0;
    at = strchr(namex, '{');
    if (at) *at = 0;
    MAPDATA* symbol = FindMap(namex,NULL); // find name in any file
    if (!symbol)symbol = FindLabelMap(namex, NULL);
    if (!symbol) return NULL;
    char* file = symbol->filename;
    FILEDATA* oldfile = currentFileData;
    AddClick();
    MakeCurrentFile(file,symbol->botid);
    mouseLine = symbol->fileline ; // 1-based, whereas yposition is 0-based
    mouseCharacter = 0;
    scriptData.maxLines = currentFileData->lineCount;
    scriptData.maxChars = currentFileData->charsize;

    if (oldfile != currentFileData)
    {
        scriptData.yposition = symbol->fileline - 3; // start with item showing in context
        scriptData.xposition = 0; // reset
    }
    else if (mouseLine > (scriptData.yposition + 1 + scriptData.linesPerPage))
        scriptData.yposition = symbol->fileline - 3; // start with item showing in context
    else if (mouseLine < scriptData.yposition +1)
        scriptData.yposition = symbol->fileline - 3; // start with item showing in context
    else if ((symbol->fileline - scriptData.yposition - 1) < (scriptData.yposition + scriptData.linesPerPage)) { ; } // same page
    else if (currentFileData->lineCount > scriptData.maxLines)
        scriptData.yposition = symbol->fileline - 3; // start with item showing in context
    else scriptData.yposition = 0;
    if (scriptData.yposition < 0) scriptData.yposition = 0; // file name must not back up
     
    DefineVertScroll(scriptData, scriptData.yposition);
    
    scriptData.maxChars = currentFileData->charsize;
    DefineHorzScroll(scriptData, 0);
    return symbol;
}

void FindBot(char* name)
{
    myBot = -1; // all  access pass
    if (name) // happens when we have logged in and gotten a bot
    {
        AlignName(name);
        myBot = FindBotID(name);
        sprintf(windowTitle, "%s=>%s (%I64u)", loginID, computerID, myBot);
        SetWindowText(scriptData.window, (LPCTSTR)windowTitle);
        return;
    }
    // happens on startup before login has occured
    if (*defaultbot)
    {
        AlignName(defaultbot);
        myBot = FindBotID(defaultbot);
        sprintf(windowTitle, "%s=>%s (%I64u)", "", defaultbot, myBot);
        SetWindowText(scriptData.window, (LPCTSTR)windowTitle);
        return;
    }
    WORDP D = FindWord("defaultbot");
    FACT* F = GetVerbNondeadHead(D);
    while (F)
    {
        if (F->verb == F->object)
        {
            char name[MAX_WORD_SIZE];
            D = Meaning2Word(F->subject);
            sprintf(name, "^%s", D->word);
            AlignName(name);
            myBot = FindBotID(name);
            sprintf(windowTitle, "%s=>%s (%I64u)", "", name, myBot);
            SetWindowText(scriptData.window, (LPCTSTR)windowTitle);
            break;
        }
        F = GetVerbNondeadNext(F);
    }
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the scriptData.window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance, LPCSTR szWindowClass)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHATSCRIPTIDE));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_CHATSCRIPTIDE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

static char* GetStartCharacter(LINEDATA* line)
{
    if (!line) return NULL;
	char* msg = (char*)&line->text;
	if (scriptData.xposition)
	{
		int count = scriptData.xposition;
		while (count-- && *msg) ++msg;
	}
	return msg;
}

static LINEDATA* GetStartLine(int count)
{
    if (!currentFileData) return NULL;
	LINEDATA* at = currentFileData->filelines;
	if (count)
	{
		while (count-- && at) at = at->next; // use forward ptr
	}
	return at;
}

static MAPDATA* MapAllocate(uint64 botid, char* file, char* name, char* line)
{
    size_t len = strlen(name);
    MAPDATA* map =  (MAPDATA*)malloc(sizeof(MAPDATA)); // link, file, line, name
    map->next = NULL; // forward link
    map->botid = botid;
    map->filename = file; // set file name
    map->fileline = (line) ? atoi(line) : 0; // set line number
    // when name is a number (line number) or if/else/loop then line is an offset into block or rule data
    // rule: ~medical_glean.43.0-MEDICALBODYPARTKIND u:  246
    char* x = (char*)malloc(len + 1);
    strcpy(x, name);
    map->name = x; 
    if (!firstMap) firstMap = map; // ordered
    if (priorMapLine) priorMapLine->next = map; // set old forward ptr
    priorMapLine = map;
    return map;
}

static LINEDATA* ReadFile(char* name,size_t &maxchars,int & maxlines, uint64 botid )
{
    FILE* in = fopen(name, "rb");
    if (!in) return NULL;
    char buffer[20000];
    firstline = NULL;
    priorline = NULL;
    line = NULL;
    maxlines = 0;
    maxchars = 0;
    MAPDATA* map = FindFileMap(name,botid); // get map for this
    if (!map) return NULL;
    int currentMapLine = map->fileline;
    while (fgets(buffer, 20000, in)) // make linked list of all text
    {
        ++maxlines;
        char* at = buffer;
        while ((at = strchr(at, '\t')))
        {
            memmove(at + 4, at + 1, strlen(at));
            memmove(at, "    ", 4);
        }
        size_t len = strlen(buffer);
        if (buffer[len - 2] == '\r')
        {
            len -= 2;
            buffer[len] = 0;
        }
        else if (buffer[len - 1] == '\n') buffer[--len] = 0;
        if (len > maxchars) maxchars = len;
        
        line = (LINEDATA*)malloc(sizeof(LINEDATA) + len + 1 );
        if (!firstline) firstline = line;
        strcpy(line->text, buffer); // set content
        
        line->mapentry = NULL;
        if (map)
        {
            int oldline = map->fileline;
            while (map && map->fileline < maxlines)
            {
               map = map->next;
               if (map && strstr(map->name, "~control"))
               {
                   int xx = 0;
               }
               if (map && map->fileline < oldline) map = NULL;
            }
            if (map && map->fileline == maxlines) 
                line->mapentry = map;
        }

        if (priorline) priorline->next = line; // set forward ptr
        line->prior = priorline; // set backward ptr
        priorline = line;
    }
    fclose(in);
    if (line) line->next = NULL; // set final forward ptr

    return firstline;
}

static void FakeMapEntry(char* name, char* comment, int & maxlines, int& maxchar, char* sysfile)
{
    // create fake map entry
    char index[100];
    char buffer[20000];
    sprintf(index, "%d", maxlines);
    MAPDATA* map = MapAllocate(0, sysfile, (char*)name, index);
    if (*name == '\r')
    {
        name += 2;
        comment = "";
    }
    *buffer = 0;
    if (*name) sprintf(buffer, "%s : %s", name, comment);
    size_t len = strlen(buffer);
    if (len > maxchar) maxchar = len;
    ++maxlines;
    
    line = (LINEDATA*)malloc(sizeof(LINEDATA) + len + 1);
    if (!firstline) firstline = line;
    strcpy(line->text, buffer); // set content
    line->mapentry = map;

    if (priorline) priorline->next = line; // set forward ptr
    line->prior = priorline; // set backward ptr
    priorline = line;
}

static void SetFile(FILEDATA* file,char* name)
{
    currentFileData = file;
    scriptData.maxLines = currentFileData->lineCount;
    scriptData.maxChars = currentFileData->charsize;
    scriptData.xposition = 0;
    scriptData.yposition = 0;
    AddTab(name);

}

static void ReadSystemFunctionsMap(char* sysfile)
{ 
    char buffer[20000];
    firstline = NULL;
    priorline = NULL;
    line = NULL;
    int maxlines = 1;
    int maxchar = 0;

    // define a system fake file
    MAPDATA* filemap = MapAllocate(0, sysfile, sysfile, 0);

    SystemFunctionInfo *fn;
    int i = 0;
    while ((fn = &systemFunctionSet[++i]) && fn->word)
    {
        char name[MAX_WORD_SIZE];
        if (strstr(fn->word,"---")) strcpy(name, fn->word);
        else  sprintf(name, "    %s", fn->word);
        // create fake map entry
        char varcount[100];
        sprintf(varcount, "%d", fn->argumentCount);
        if (fn->argumentCount == -1) strcpy(varcount, "...");
        if (fn->argumentCount == -2) strcpy(varcount, "stream");
        if (fn->argumentCount == -3) strcpy(varcount, "unevaled");
        sprintf(buffer, "(%s) : %s", varcount,fn->comment);
        
        FakeMapEntry(name, buffer, maxlines, maxchar, sysfile);
    }
    if (line) line->next = NULL; // set final forward ptr

    FILEDATA* file = (FILEDATA*)malloc(sizeof(FILEDATA)); // next, name, filedata, maxx, maxy
    file->next = fileList;
    fileList = file;
    file->map = filemap;
    char* fname = (char*)malloc(strlen(sysfile) + 1);
    strcpy(fname, sysfile);
    file->status = (char*)malloc(maxlines + 1);
    memset(file->status, ' ', maxlines + 1); // breakpoint data
    file->filename = fname; // set file name
    file->filelines = firstline; // set file data
    file->charsize = maxchar;
    file->lineCount = maxlines;
    SetFile(file, "systemFunctions");
}

static void ReadSystemVariablesMap(char* sysfile)
{
    firstline = NULL;
    priorline = NULL;
    line = NULL;
    int maxlines = 1;
    int maxchar = 0;

    // define a var fake file
    MAPDATA* filemap = MapAllocate(0, sysfile, sysfile, 0);
    SYSTEMVARIABLE *fn;
    FakeMapEntry("$cs_token", "controls input processing", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_response", "controls automatic handling of outputs to user", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_control_pre", "name of topic to run in gambit mode on pre - pass", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_prepass", "name of topic to run before $cs_control_main", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_control_main", "name of topic to run every sentence of volley", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_control_post", "name of topic to run once after all sentences have been handled", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_wildcardseparator", "when a match variable covers multiple words, what should separate them", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_looplimit", "stops a loop after n iterations", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_trace", "turn on these trace bits for user", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_userfactlimit", "how many facts to keep for user", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_response", "controls some characteristics of how responses are formatted ", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_numbers", "if defined, causes the system to output numbers in a different language style", maxlines, maxchar, sysfile);
    FakeMapEntry("$cs_externaltag", "name of a topic to use to replace existing internal English pos - parser", maxlines, maxchar, sysfile);
    FakeMapEntry("", "", maxlines, maxchar, sysfile);
    int i = 0;
    while ((fn = &sysvars[++i]) && fn->name)
    {
        char name[MAX_WORD_SIZE];
        if (strstr(fn->name, "---")) strcpy(name, fn->name);
        else sprintf(name, "    %s", fn->name);
        FakeMapEntry(name, (char*)fn->comment, maxlines, maxchar, sysfile);
    }
    if (line) line->next = NULL; // set final forward ptr

    FILEDATA* file = (FILEDATA*)malloc(sizeof(FILEDATA)); // next, name, filedata, maxx, maxy
    file->next = fileList;
    fileList = file; 
    file->map = filemap;
    char* fname = (char*)malloc(strlen(sysfile) + 1);
    strcpy(fname, sysfile);
    file->status = (char*)malloc(maxlines + 1);
    memset(file->status, ' ', maxlines + 1); // breakpoint data
    file->filename = fname; // set file name
    file->filelines = firstline; // set file data
    file->charsize = maxchar;
    file->lineCount = maxlines;
    SetFile(file, "systemVariables");
}

static void ReadDebugCommandsMap(char* sysfile)
{
    firstline = NULL;
    priorline = NULL;
    line = NULL;
    int maxlines = 1;
    int maxchar = 0;
    CommandInfo *routine = NULL;

    // define a debug command fake file
    MAPDATA* filemap = MapAllocate(0, sysfile, sysfile, 0);
    int i = 0;
    while ((routine = &commandSet[++i]) && routine->word)
    {
        char name[MAX_WORD_SIZE];
        if (strstr(routine->word, "---")) strcpy(name, routine->word);
        else sprintf(name, "    %s", routine->word);
        FakeMapEntry(name, (char*)routine->comment, maxlines, maxchar,sysfile);
    }
    if (line) line->next = NULL; // set final forward ptr

    FILEDATA* file = (FILEDATA*)malloc(sizeof(FILEDATA)); // next, name, filedata, maxx, maxy
    file->next = fileList;
    fileList = file;
    file->map = filemap;
    char* fname = (char*)malloc(strlen(sysfile) + 1);
    strcpy(fname, sysfile);
    file->status = (char*)malloc(maxlines + 1);
    memset(file->status, ' ', maxlines + 1); // breakpoint data
    file->filename = fname; // set file name
    file->filelines = firstline; // set file data
    file->charsize = maxchar;
    file->lineCount = maxlines;
    SetFile(file, "debugCommands");
}

static void ReadIDECommandsMap(char* sysfile)
{
    firstline = NULL;
    priorline = NULL;
    line = NULL;
    int maxlines = 1;
    int maxchar = 0;

    // define a debugger fake file
    MAPDATA* filemap = MapAllocate(0, sysfile, sysfile, 0);
    FakeMapEntry("Script Window", "", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click in tag/line#", "set/clear breakpoint", maxlines, maxchar, sysfile);
    FakeMapEntry("    R-click in tag/line#", "set transient break,  go, erase when stop somewhere", maxlines, maxchar, sysfile);
    FakeMapEntry("    Ctrl-L-click in tag/line#", "change (if possible) code pointer of this level to this line for future execution", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click on $var, %var, _n, @n", "display in variables window", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click on ^fn, %concept, %topic", "display that area of script", maxlines, maxchar, sysfile);
    FakeMapEntry("Title area", "", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click on name", "display that file of script", maxlines, maxchar, sysfile);
    FakeMapEntry("    R-click on name", "remove that file of script from display", maxlines, maxchar, sysfile);
    FakeMapEntry("Variables Window", "", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click left of variable", "break on change to it", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click on variable", "remove entry from window", maxlines, maxchar, sysfile);
    FakeMapEntry("Stack Window", "", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click on line", "display locals of that entry in variables window", maxlines, maxchar, sysfile);
    FakeMapEntry("Input Window", "", maxlines, maxchar, sysfile);
    FakeMapEntry("    :i xxx", "analogous to clicking on words in script window", maxlines, maxchar, sysfile);
    FakeMapEntry("    :i var = xxx", "break when var is set to xxx  can also be < or > values", maxlines, maxchar, sysfile);
    FakeMapEntry("    :xxx", "you can use various engine debug commands", maxlines, maxchar, sysfile);
    FakeMapEntry("    :trace", "also sets default trace value when R-click on movement buttons", maxlines, maxchar, sysfile);
    FakeMapEntry("    xxx", "input to ChatScript engine", maxlines, maxchar, sysfile);
    FakeMapEntry("Output Window", "", maxlines, maxchar, sysfile);
    FakeMapEntry("Sentence Window", "", maxlines, maxchar, sysfile);
    FakeMapEntry("    L-click cycles mode", "displays: current sentence internal, sentence user typed, cannonical form of sentence", maxlines, maxchar, sysfile);
    FakeMapEntry("    R-click on word", "displays: displays marked concepts/topics of word", maxlines, maxchar, sysfile);
    FakeMapEntry("Buttons", "", maxlines, maxchar, sysfile);
    FakeMapEntry("    Go", "resume execution - rclick enables trace all", maxlines, maxchar, sysfile);
    FakeMapEntry("    In", "go to more refined level - rclick enables trace", maxlines, maxchar, sysfile);
    FakeMapEntry("    Out", "go to less refined level - rclick enables trace", maxlines, maxchar, sysfile);
    FakeMapEntry("    Next", "go to next item at same level or go out - rclick enables trace", maxlines, maxchar, sysfile);
    FakeMapEntry("    Stop", "used during execution, stop as soon as possible", maxlines, maxchar, sysfile);
    FakeMapEntry("    Msg", "l-click: break when submitting output for user   r-click: turn off break", maxlines, maxchar, sysfile);
    FakeMapEntry("    Fail", "l-click: enable break if system function fails  r-click: turn off break   ctr-click: fail input", maxlines, maxchar, sysfile);
    FakeMapEntry("    Clear", "l-remove all breakpoints  r-click: disable/enable breakpoints", maxlines, maxchar, sysfile);
    FakeMapEntry("    Global", "flip variables window from locals back to globals", maxlines, maxchar, sysfile);
    FakeMapEntry("    Back", "go back to viewing prior script location", maxlines, maxchar, sysfile);
    FakeMapEntry("    Font", "l-click for smaller, r-click for bigger", maxlines, maxchar, sysfile);
    if (line) line->next = NULL; // set final forward ptr

    FILEDATA* file = (FILEDATA*)malloc(sizeof(FILEDATA)); // next, name, filedata, maxx, maxy
    file->next = fileList;
    fileList = file;
    file->map = filemap;
    char* fname = (char*)malloc(strlen(sysfile) + 1);
    strcpy(fname, sysfile);
    file->status = (char*)malloc(maxlines + 1);
    memset(file->status, ' ', maxlines + 1); // breakpoint data
    file->filename = fname; // set file name
    file->filelines = firstline; // set file data
    file->charsize = maxchar;
    file->lineCount = maxlines;
    SetFile(file, "ideCommands");
}

static void MakeCurrentFile(char* name,uint64 botid)
{
    currentFileData = GetFile(name,botid);
    scriptData.maxLines = currentFileData->lineCount;
    scriptData.maxChars = currentFileData->charsize;
    scriptData.xposition = 0;
    scriptData.yposition = 0;
}

static void DisableAllBreakpoints()
{
    disableBreakpoints = !disableBreakpoints;
}

static void ClearAllBreakpoints()
{
    breakpointCount = 0;
    debugMessage = NULL;
    FILEDATA* at = fileList;
    while (at)
    {
        memset(at->status, ' ', at->lineCount+1);
        at = at->next;
    }
    breakIndex = 0; // engine
    DebugOutput("All breakpoints cleared.\r\n");
}

static FILEDATA* GetFile(char* name,uint64 botid) // name is full path cs relative
{
    char fn[MAX_WORD_SIZE];
    ReadCompiledWord(filenames, fn);
    if (!stricmp(fn, name)) return currentFileData; // already is there first

    char fxname[MAX_WORD_SIZE];
    char* endname = strrchr(name, '/') ;
    if (endname) strcpy(fxname, endname + 1); // spaces around the name always
    else strcpy(fxname, name);
    AddTab(fxname);

    // find the file if we can
    FILEDATA* at = fileList;
    while (at)
    {
        if (!stricmp(at->filename, name)) // found full path entry
        {
            return at; // the data
        }
        at = at->next;
    }

    // allocate the file not found
    int maxlines = 0;
    size_t maxchar = 0;
    LINEDATA* fileinfo = ReadFile(name,maxchar,maxlines,botid);
    size_t len = strlen(name);
    FILEDATA* file = (FILEDATA*)malloc(sizeof(FILEDATA)); // next, name, filedata, maxx, maxy
    file->next = fileList;
    fileList = file;
    file->map = FindFileMap(name, botid);
    char* fname = (char*)malloc(strlen(name) + 1);
    strcpy(fname, name);
    file->status = (char*)malloc(maxlines + 1);
    memset(file->status, ' ', maxlines + 1); // breakpoint data
    file->filename = fname; // set file name
    file->filelines = fileinfo; // set file data
    file->charsize = maxchar;
    file->lineCount = maxlines;
    return file;
}

static void FindClickWord(int mouseCharacter, int mouseLine)
{
    LINEDATA* at = GetStartLine(scriptData.yposition);
    if (!at) return;
    int yline = scriptData.yposition;
    char name[MAX_WORD_SIZE];
    *name = 0;
    while (at)
    {
        if (!at->mapentry)
        {
            at = at->next; // use forward ptr
            continue;
        }
        if (yline == mouseLine && mouseCharacter >= 0 && mouseCharacter >= scriptData.xposition)
        {
            char* msg = GetStartCharacter(at);
            size_t len = strlen(msg);
            if (mouseCharacter <= (scriptData.xposition + len))
            {
                int n = mouseCharacter;
                while (IsLegalNameCharacter(msg[n]) || msg[n] == '$' || msg[n] == '@' || msg[n] == '%' || msg[n] == '~' || msg[n] == '^') --n;
                ++n;
                ReadCompiledWord(msg + n, name); 
                n = 1; // simple # 
                if (name[1]) while (name[++n] && (IsLegalNameCharacter(name[n]) || name[n] == '$')) { ; }
                name[n] = 0;
            }
            break;
        }
        if (at->mapentry->fileline <= yline) at = at->next; // use forward ptr
        ++yline;
    }
    char junk[MAX_WORD_SIZE];
    if (*name == '\'') sprintf(junk, "%s", name+1); // strip quote
    else sprintf(junk, "%s", name);
    MakeLowerCase(junk);

    if (*junk == '$' || *junk == '%' || ((*junk == '_' || *junk == '@') && IsDigit(junk[1])))
    {
        AddVariable(junk);
        fnlevel = -1; // show globals
    }
    else if (*junk && AlignName(junk)) { ; }
    else if (*junk == '^')
    {
        AddVariable(junk); // fn var?
        fnlevel = -1; // show globals
    }
}

static void ReleaseFiles()
{
    while (fileList)
    { 
        RemoveTab(fileList->filename);
        free(fileList->filename);
        free(fileList->map);
        free(fileList->status);
        LINEDATA* line = fileList->filelines;
        while (line)
        {
            // line->text is built into the actual line struct
			MAPDATA* entry = line->mapentry;
			free(entry);
            LINEDATA* next = line->next;
            free(line);
            line = next;
        }
        FILEDATA* next = fileList->next;
        free(fileList);
        fileList = next;
    }
    firstMap = NULL; // just trash the data here
    priorMapLine = NULL;
    *filenames = 0;
    currentFileData = NULL;
}

static void ReadMap(char* name)
{
    FILE* in = fopen(name, "rb");
    if (!in) return;
    char buffer[20000];
    char* file;

//file: 0 full_path_to_file optional_botid
//macro : start_line_in_file name_of_macro optional_botid(definition of user function)
//line: start_line_in_file offset_byte_in_script (action unit in output) 
//concept : start_line_in_file name_of_concept optional_botid(concept definition)
//topic : start_line_in_file name_of_topic optional_botid(topic definition)
//rule : start_line_in_file full_rule_tag_with_possible_label rule_kind(rule definition)
//Complexity of name_of_macro complexity_metric(complexity metric for function)
//Complexity of rule full_rule_tag_with_possible_label rule_kind complexity_metric(complexity metric for rule)

    while (fgets(buffer, 20000, in))
    {
        int64 botid = 0;
        size_t len = strlen(buffer);
        if (buffer[len - 2] == '\r') buffer[len - 2] = 0;
        char kind[MAX_WORD_SIZE];
        char line[MAX_WORD_SIZE];
        char* at = ReadCompiledWord(buffer, kind);
        if (!*kind || *kind == '#') continue;
        at = ReadCompiledWord(at, line); // line number
        char title[MAX_WORD_SIZE];
        at = ReadCompiledWord(at, title);
        at = SkipWhitespace(at);
        if (strstr(kind, "file:"))
        {
            file = NULL;
            uint64 id = atoi64(at); // optional
            MakeLowerCase(title);
            file = (char*)malloc(strlen(title) + 1);
            strcpy(file, title); // start of a file
            char* x = strrchr(title, '/');
            if (x) MapAllocate(id,file, x+1, 0);
        }
        else if (!file) continue;   // ignoring all this
        else if (!stricmp(kind, "MACRO:") || !stricmp(kind, "TOPIC:") || !stricmp(kind, "CONCEPT:"))
        {
            botid = atoi64(at); // optional
            MapAllocate(botid,file, title, line);
        }
        else if (!stricmp(kind, "LINE:") )
        { // line: 244 fileline  scriptOffset
            MapAllocate(0,file, title, line);
        }
        else if (!stricmp(kind, "BOT:"))
        { // line: 244 botname botid 
            strcat(title, "`bot");
            uint64 botid = atoi64(at);
            MapAllocate(botid, file, title, line );
        }
        else if (!stricmp(kind, "RULE:"))
        { // rule: 246 ~medical_glean.43.0-MEDICALBODYPARTKIND u:  
            char x[MAX_WORD_SIZE];
            sprintf(x,"%s\r\n", title);
            ++rulesInMap;
            MapAllocate(0,file, title,line); // ignoring type of rule for now
        }
        else if (!strnicmp(kind, "if",2) || !strnicmp(kind, "else", 4) || !strnicmp(kind, "loop", 4))
        { // if: 246 73   (line) (offset)
            char x[MAX_WORD_SIZE];
            sprintf(x, "%s:%s", kind,title);
            MapAllocate(0, file, x, line); // ignoring type of rule for now
        }


    }
    fclose(in);
}

static void StatData()
{
    if (!statData.window) return;

    char word[MAX_WORD_SIZE];
    int heapfree = (heapFree - stackFree) / 1000;
    int index = Word2Index(dictionaryFree);
    int factfree = factEnd - factFree;
    int gap = maxReleaseStackGap / 1000;
    int dictfree = (int)(maxDictEntries - index);
    sprintf(word, "%d", worstDictAvail);
    sprintf(word, "Memory: %dKb Worst: %dKb   Buffer: %d Worst: %d   Dict: %d Worst: %d   Fact: %d Worst: %d  BotRules: %d  Executed: %d",
        heapfree, gap,
        maxBufferLimit - bufferIndex, maxBufferLimit - maxBufferUsed,
        dictfree, worstDictAvail,
        factfree, worstFactFree, rulesInMap, rulesExecuted);

    SendMessage(statData.window, EM_SETSEL, 0, -1); // append to output
    SendMessage(statData.window, EM_REPLACESEL, false, (LPARAM)word);
    
    // count lines of data in answer
    si.nPos = 0;
    si.nMin = 0;
    si.nMax = strlen(word);
    SetScrollInfo(statData.window, SB_HORZ, &si, true);
    InvalidateRgn(statData.window, NULL, true);
}

static char* DebugOutput(char* output) // add to output
{
    if (!outputData.window) return NULL;
    char* answer = AllocateBuffer();
    changingEdit = true;
    SendMessage(outputData.window, EM_SETSEL, -1, -1); // append to output
    SendMessage(outputData.window, EM_REPLACESEL, false, (LPARAM)output);
    SendMessage(outputData.window, EM_SETSEL, -1, -1); // append to output
    // SendMessage(outputData.window, EM_LINESCROLL, -1, -1);
    GetWindowText(outputData.window, answer, MAX_BUFFER_SIZE);
    editLen = strlen(answer); // we leave off here
    if (editLen > 20400) // chop it 20k
    {
        char* at = answer + 20000; //49k
        while (*++at && *at != '\n');
        ++at; // fresh start
        SendMessage(outputData.window, EM_SETSEL, 0, -1);
        SendMessage(outputData.window, EM_REPLACESEL, false, (LPARAM)at);
        editLen = strlen(answer);
    }
    // count lines of data in answer
    char* at = answer;
    char* start = answer;
    int lines = 0;
    int chars = 0;
    int maxchars = 0;
    while ((at = strstr(at, "\r\n")))
    {
        *at = 0;
        size_t n = strlen(start);
        if (n > maxchars) maxchars = n;
        *at = '\r';
        ++lines;
        at += 2;
    }

    si.fMask = SIF_POS | SIF_RANGE;
    si.nPos = lines;
    si.nMin = 0;
    si.nMax = lines;
    SetScrollInfo(outputData.window, SB_VERT, &si, true);
    si.nPos = 0;
    si.nMin = 0;
    si.nMax = maxchars;
    SetScrollInfo(outputData.window, SB_HORZ, &si, true);
    changingEdit = false;
    FreeBuffer();
    return NULL;
}

static void ClearStops() 
{
    RemoveBreak(transientBreak);
    nextdepth = -1;
    idestop = false;
    nextlevel = -1;
    outdepth = -1;
    outlevel = -1;
}

bool StopIntended()
{
    if (nextdepth == globalDepth || idestop) return true;
    if (outdepth > globalDepth) return true;
    return false;
}

 void  MyWorkerThread(void* pParam)
{
    char word[MAX_WORD_SIZE];
    char* at = ReadCompiledWord(ourMainInputBuffer, word);
    rulesExecuted = 0;
    if (!*loginID) // initial message from chatbot given login name input
    {
        char user[MAX_WORD_SIZE];
        strcpy(user, ourMainInputBuffer);
        PerformChat(user, computerID, ourMainInputBuffer, NULL, ourMainOutputBuffer); // unknown bot, no input,no ip
        EmitOutput();

    }
    else
    {
		if (!stricmp(word, ":build") || !stricmp(word, ":bot") || !stricmp(word, ":restart")) ReleaseFiles(); // release map and files
		ProcessInputFile(); // come back when this input is used up
        if (!stricmp(word, ":trace"))
        {
            idetrace = trace;
        }
        else if (!stricmp(word, ":build") || !stricmp(word, ":bot") || !stricmp(word, ":restart")) // release map and files
        {
            rulesInMap = 0;
            ReadMap("TOPIC/build1/map1.txt");
            ReadMap("TOPIC/build0/map0.txt");
            ReadSystemFunctionsMap("systemFunctions");
            ReadSystemVariablesMap("systemVariables");
            ReadDebugCommandsMap("debugCommands");
            ReadIDECommandsMap("ideCommands");
            currentFileData = NULL;
            FindBot(computerID);
            scriptData.xposition = scriptData.yposition = 0;
            sprintf(windowTitle, "%s=>%s (%I64u)  depth: %d/%d", loginID, computerID, myBot, idedepth, globalDepth);
            SetWindowText(scriptData.window, (LPCTSTR)windowTitle);
        }
        char fn[MAX_WORD_SIZE];
        sprintf(fn, "^%s", computerID);
        FindBot(fn);
    }
    csThread = 0;
    fnlevel = -1;
    globalAbort = false;

    // terminate any stop pending since we finished
    ClearStops();

    mouseCharacter = -1;
    mouseLine = -1; // no highlight now
    codeFile = NULL;
    codeLine = NULL;
    InvalidateRgn(scriptData.window, NULL, true);
    InvalidateRgn(stackData.window, NULL, true);
    InvalidateRgn(varData.window, NULL, true);
	StatData();
	//InvalidateRgn(hParent, NULL, true);
 }

 void  MyWorkerInitThread(void* pParam)
 {
     if (InitSystem(argc, argv, NULL, NULL, NULL, NULL, DebugInput, DebugOutput)) myexit((char*)"failed to load memory\r\n");
     InvalidateRgn(hParent, NULL, true);
     // defines the default bot
     ReadMap("TOPIC/build1/map1.txt");
     ReadMap("TOPIC/build0/map0.txt");
     ReadSystemFunctionsMap("systemFunctions");
     ReadSystemVariablesMap("systemVariables");
     ReadDebugCommandsMap("debugCommands");
     ReadIDECommandsMap("ideCommands");
     // process break requests
     char* at = debugEntry;
     char word[MAX_WORD_SIZE];
     while (*at)
     {
         char* comma = strchr(at, ',');
         if (comma) *comma = 0;
         at = ReadCompiledWord(at, word);
         if (*word == '*') idestop = true;
         else ProcessBreak(word, 0);
         if (comma) at = comma + 1;
     }

     UpdateWindowMetrics(scriptData);

     if (!server)
     {
         quitting = false; // allow local bots to continue regardless
         *ourMainInputBuffer = 0;
         if (!*loginID)
         {
             (*printer)((char*)"%s", (char*)"\r\nEnter user name: ");
         }
     }
     if (inputData.window) SendMessage(inputData.window, EM_SCROLLCARET, 0, 0);
     changingEdit = false;
     csThread = 0;
     fnlevel = -1;
     globalAbort = false;
     InvalidateRgn(scriptData.window, NULL, true);
     InvalidateRgn(stackData.window, NULL, true);
     InvalidateRgn(varData.window, NULL, true);
 }
 
 static void Go()
 {
     if (csThread)
     { 
        ideoutputlevel = outputlevel; // for action execution of function or rule
        idedepth = globalDepth; // restore to normal
        fnvars = NULL;
        fnlevel = -1;
        DebugOutput("Resume execution\r\n");
        ResumeThread((HANDLE)csThread);
     }
 }

 static void DiscardInput()
 {
     SendMessage(inputData.window, WM_SETTEXT, 0, (LPARAM)"");
     SendMessage(inputData.window, EM_SETSEL, -1, -1);
     FreeBuffer();
 }

 static void RemoveBreak(char* name)
 {
 /*    if (!stricmp(name, "abort"))
     {
         abortCheck = false;
         return;
     } */
     char* excess = strchr(name, '@');
     if (excess) *excess = 0;
     size_t len = strlen(name);
     for (int i = 0; i < breakIndex; ++i)
     {
         if (!strnicmp(name, breakAt[i], len) && (breakAt[i][len] == 0 || breakAt[i][len] == '@' || breakAt[i][len] == ' '))
         {
             --breakIndex;
             for (int j = i; j < breakIndex; ++j) strcpy(breakAt[j], breakAt[j + 1]);
             break;
         }
     }
     if (!breakIndex) debugAction = NULL;
     if (!breakIndex && nextdepth == -1)
     {
         debugCall = NULL;
     }
 }

 static void RemoveVar(char* name)
 {
     size_t len = strlen(name);
     for (int i = 0; i < breakIndex; ++i)
     {
         if (!strnicmp(name, varAt[i], len) && varAt[i][len] == 0)
         {
             --varIndex;
             for (int j = i; j < varIndex; ++j) strcpy(varAt[j], varAt[j + 1]);
             break;
         }
     }
 }

 static int ProcessAssign(char* input) // set breakpoint
 {
     int answer = 0;
     char word[MAX_WORD_SIZE];
     while (*input)
     {
         input = ReadCompiledWord(input, word);
         if (!*word) break;
         char* dot = strchr(word, '.');
         if (dot && !FindWord(word, dot - word))
         {
             DebugPrint("Unknown context: %s\r\n", word);
             continue;
         }
         if (!stricmp(word, "go")) answer = 1;
         else if (*word == '!') RemoveVar(word + 1);
         else if (!dot && *word != '$')
         {
             DebugPrint("Illegal variable: %s\r\n", word);
             continue;
         }
         else if (dot && (dot[1] != '$' || dot[2] != '_'))
         {
             DebugPrint("Illegal context variable: %s\r\n", word);
             continue;
         }
         else strcpy(varAt[varIndex++], word);
     }
     return answer;
 }

void CheckForEnter() // on input edit scriptData.window
{
    if (!inputData.window) return;
    idekey = true; // some input seen
    char* answer = AllocateBuffer();
    char* original = answer;
    GetWindowText(inputData.window, answer, MAX_BUFFER_SIZE);
    char* at = strchr(answer, '\n');
    if (!at)
    {
        FreeBuffer(); // answr is still visible for use
        return; // request incomplete
    }
    *at = 0;
    if (*(at -1) == '\r') *--at = 0;

    // debugger commands
    char word[MAX_WORD_SIZE];
    at = ReadCompiledWord(answer, word);
    if (!stricmp(word, ":dump")) DumpMaps();
    else if (!stricmp(word, ":i") && ((*at == '@' && IsDigit(at[1])) || *at == '$' || *at == '%' || !strnicmp(at,"ja-",3) || !strnicmp(at,"jo-",3)) || (*at == '_' && IsDigit(at[1])) )
    {
        at = ReadCompiledWord(at, word);
        at = SkipWhitespace(at);
        if (*at == '=' || *at == '<' || *at == '>')
        {
            char value[MAX_WORD_SIZE];
            *value = 0;
            if (strlen(at + 1) > 45) DebugOutput("value too big\r\n");
            else strcpy(value, SkipWhitespace(at + 1));
            
            int at1 = FindVarBreakValue(word);
            if (at1 >= 0) // change it
            {
                if (!*value) // remove it by overwrite from end
                {
                    vartest[at1] = *at;
                    strcpy(varvalue[at1], varvalue[varnamesindex - 1]);
                    strcpy(varname[at1], varname[--varnamesindex]);
                }
                else
                {
                    vartest[at1] = *at;
                    if (!stricmp(value, "null")) *value = 0; // that's null
                    strcpy(varvalue[at1], word);
                }
            }
            else if (varnamesindex < MAX_VARVALUES)// create new
            {
                vartest[varnamesindex] = *at;
                strcpy(varname[varnamesindex], word);
                strcpy(varvalue[varnamesindex++], value);
            }
            debugVar = DebugVar; // not just monitor
        }
        AddVariable(word);
        DiscardInput();
    }
    else if (!stricmp(word, ":i"))
    {
        at = ReadCompiledWord(at, word);
        if (*word != '~' && *word != '$' && *word != '^')
        {
            MAPDATA* map = FindFileMap(word, myBot); // get map for this
            if (map)
            {
                FILEDATA* file = GetFile(map->filename, myBot);
                if (file)
                {
                    currentFileData = file;
                    scriptData.yposition = 0;
                }
            }
            else if (AlignName(word));
            else 
            {
                char tag[MAX_WORD_SIZE];
                FunctionResult result = FindRuleCode1(word, tag);
                MAPDATA* found = NULL;
                if (result == NOPROBLEM_BIT) // found the topic
                {
                    char* at = strchr(tag, '.');
                    strcpy(at + 1, word);
                    found = AlignName(word);
                }
                if (!found)
                {
                    strcat(word, " - not found\r\n");
                    DebugOutput(word);
                }
            }
        }
        else
        {
            MAPDATA* found = AlignName(word);
            if (!found)
            {
                strcat(word, " - not found\r\n");
                DebugOutput(word);
            }
        }
        DiscardInput();
        InvalidateRgn(scriptData.window, NULL, true);
    }
    else if (!stricmp(word, ":break"))
    {
        ReadCompiledWord(at, word);
        if (*word == '!') ++at;
        ProcessBreak(word,0);
        AlignName(at);
        int line = NameLine(at);
        currentFileData->status[line] = (*word == '!') ? ' ' : 'B';
        if (*word == '!') --breakpointCount;
        else ++breakpointCount;
        DiscardInput();
    }
    else if (*word == ':' && !stricmp(word,":build") && !stricmp(word, ":restart") && !stricmp(word, ":bot") && !stricmp(word, ":user"))
    {
        char* buffer = AllocateBuffer();
        DoCommand(answer, buffer);
        FreeBuffer();
        DiscardInput();
    }
    else if (csThread)
    {
        DebugPrint("Cannot input to CS at breakpoint");
        DiscardInput();
    }
    // normal cs input
    else if (!csThread)
    {
            idekey = false;
            if (!server) // refresh prompts from a loaded bot since mainloop happens before user is loaded
            {
                WORDP dBotPrefix = FindWord((char*)"$botprompt");
                strcpy(botPrefix, (dBotPrefix && dBotPrefix->w.userValue) ? dBotPrefix->w.userValue : (char*)"");
                WORDP dUserPrefix = FindWord((char*)"$userprompt");
                strcpy(userPrefix, (dUserPrefix && dUserPrefix->w.userValue) ? dUserPrefix->w.userValue : (char*)"");
            }
            char msg[100];
            sprintf(msg, "\r\n%s", userPrefix);
            DebugOutput(msg);
            DebugOutput(answer);
            DebugOutput("\r\n");
            char prefix[MAX_WORD_SIZE];
            if (*botPrefix)
            {
                sprintf(prefix, (char*)"%s ", ReviseOutput(botPrefix, prefix));
                DebugOutput(prefix);
            }
            DiscardInput();

            strcpy(ourMainInputBuffer, answer);
            csThread = _beginthread(MyWorkerThread, 0, NULL);
        }
        else if (!stricmp(word, ":go"))
        {
            DiscardInput();
            Go(); // resume
        }
}

static char* DebugInput(char* input)
{
    char* answer = NULL;
    if (!inputData.window) return NULL;
    GetWindowText(inputData.window, answer, MAX_BUFFER_SIZE);
    answer += editLen;

    // swallow the input
    SendMessage(inputData.window, EM_SETSEL, -1, -1); // append to output
    GetWindowText(inputData.window, answer, MAX_BUFFER_SIZE);
    editLen = strlen(answer); // we leave off here

    return answer;
}

static MAPDATA* FindRuleMap(char* topicline) // get map entry for a line if there is one (line and rule and macro and topic entries)
{
    MAPDATA* at = currentFileData->map;
    while (at)
    {
        if (!stricmp(topicline, at->name)) return at;
        at = at->next;
    }
    return NULL;
}

static void AssignDepth(int depth)
{
    CALLFRAME* frame = GetCallFrame(depth);
    if (!stricmp(frame->label,"ruleoutput")) frame = GetCallFrame(--depth);
    
    // for rule break info is:  ~topic.n.n
    char topic[MAX_WORD_SIZE];
    strcpy(topic, frame->label);
    char message[MAX_WORD_SIZE];
    if (offsetCode >= 0 && codePtr)
    {
        char word[MAX_WORD_SIZE];
        strncpy(word, codePtr, 40);
        word[40] = 0;
        sprintf(message, "At break: %s+%d  %s\r\n", frame->label, offsetCode,word);
    }
    else sprintf(message, "At break: %s\r\n", frame->label);
    DebugOutput(message);
    offsetCode = -1;
}

static char* DebugRealCaller() // decide who we are currently within
{
    for (int i = globalDepth; i > 0; --i)
    {
        CALLFRAME* frame = GetCallFrame(i);
        if (!*(frame->label) || !stricmp(frame->label, "ruleoutput")) continue;
        if (*frame->label == '^' || *frame->label == '~') return frame->label; // function, rule, or topic owner
    }
    return "";
}

static void ShowSentence()
{
    char* buffer = AllocateBuffer();
    char* base = buffer;
    if (sentenceMode == 0) strcpy(buffer, "adjusted:  ");
    else if (sentenceMode == 1) strcpy(buffer, "raw:  ");
    else strcpy(buffer, "canonical:  ");
    buffer += strlen(buffer);
    *buffer = 0;
    if (sentenceMode != 1) for (int i = 1; i <= wordCount; ++i)
    {
        if (!wordStarts[i]) continue; // ignore
        if (unmarked[i]) strcpy(buffer, "***");
        else if (sentenceMode == 0) strcpy(buffer, wordStarts[i]);
        else strcpy(buffer, wordCanonical[i]);
        buffer += strlen(buffer);
        *buffer++ = ' ';
    }
    *buffer = 0;
    if (sentenceMode == 1 && mainInputBuffer) strcpy(buffer, mainInputBuffer);

    sentenceData.maxChars = buffer - base;
    si.fMask = SIF_POS | SIF_RANGE;
    si.nPos = 0;
    si.nMin = 0;
    si.nMax = sentenceData.maxChars;
    SetScrollInfo(sentenceData.window, SB_HORZ, &si, true);
    SendMessage(sentenceData.window, WM_SETTEXT, 0, (LPARAM)base);
    FreeBuffer(buffer);
    InvalidateRgn(sentenceData.window, NULL, true);
}

static void Wait()
{
    ClearStops();
    StatData();
    fnlevel = globalDepth;
    char msg[MAX_WORD_SIZE];
    sprintf(msg, "callstack  depth: %d  level:%d",globalDepth, outputlevel);
    SendMessage(stackData.window, WM_SETTEXT, 0, (LPARAM)msg);
    idedepth = globalDepth;
    fnvars = NULL; // drop back to globals

    WhereAmI(globalDepth);

    sentenceMode = 0;
    ShowSentence();
    char junk[100];
    DoneCallback(junk);
    if (breakType == BREAK_ACTION && offsetCode == 0 && *ownerFrame->label == '^' && ownerFrame->code) ShowStack();

    if (!breakIndex && nextdepth == -1 && outdepth == -1)
        debugCall = NULL;

    FreeClicklist();

    // engine thread suspends itself now
    SuspendThread((HANDLE)csThread);
}

static char* DebugVar(char* name, char* value) // incoming variable assigns from engine
{
    if (loadingUser || disableBreakpoints || globalAbort) return NULL;
    if (!value) value = "";
    size_t len = strlen(name);
    char id[MAX_WORD_SIZE];
    strcpy(id, name);
    char* jsondot = strchr(id, '.');
    if (jsondot) *jsondot = 0; // use raw but list change full
    char* jsonbrakcet = strchr(id, '[');
    if (jsonbrakcet) *jsonbrakcet = 0; // use raw but list change full
    strcat(id, "\r\n");
    if (idestop || strstr(varbreaknames,id))
    {
        int index = FindVarBreakValue(id);
        if (index >= 0)
        {
            if (vartest[index] == '=' && stricmp(varvalue[index], value)) return NULL;
            if (vartest[index] == '<' && atoi(varvalue[index]) >= atoi(value)) return NULL;
            if (vartest[index] == '>' && atoi(varvalue[index]) <= atoi(value)) return NULL;
        }
        char val[120];
        len = strlen(value);
        if (len > 100)
        {
            strncpy(val, value, 100);
            strcpy(val + 100, "...");
        }
        else strcpy(val, value);
        DebugPrint("Var Break %s = %s\r\n",name, val);
        InvalidateRgn(varData.window, NULL, true);
        // assignment happens from code (we cant see locals)
        trace = 0;
        breakType = BREAK_VAR;
        Wait();
    }
    
    return NULL;
}

char* DebugCall(char* name,bool in)
{
    if (globalAbort) return (char*)1;
	if (in)
	{
		if (*name == '~')
		{
			int xx = 0; // topic call
		}
		else
		{
			int xx = 0; // function call
		}
	}
    int i;
    int depth = globalDepth + 1; // if inside () dont react
    CALLFRAME* frame;
    while (--depth) // see if system function inside of some other arg processing
    {
        frame = GetCallFrame(depth);
        if (strchr(frame->label, '('))
        {
            return NULL; // not stop in arg processing
        }
        if (strchr(frame->label, '{') && frame->code)
        {
            break; // legal to be here if not sys function
        }
    }

    depth = globalDepth;
    frame = GetCallFrame(globalDepth);
    if (idestop && *name == '^' && frame->code) return NULL; // proceed "in" to action in a user routine

    char* paren = strchr(frame->label, '(');

    // get raw label
    char label[500];
    strcpy(label, frame->label);
    char* at = strchr(label, '(');
    if (at) *at = 0;
    at = strchr(label, '{');
    if (at) *at = 0;

	bool stop = false;

    if (paren && *frame->label == '^') return NULL; // no argument processing
    if (paren && !strstr(frame->label,"if")) return NULL; // no argument processing
    if (paren && !strstr(frame->label, "loop")) return NULL; // no argument processing
    if (sysfail && *name == '^' && !frame->code &&
        frame->x.result & FAILCODES && stricmp(name,"^fail") && !disableBreakpoints)
    {
        stop = true;
        DebugPrint("Break failing system call %s\r\n", name);
    }
  
    else if (strchr(frame->label, '{') && globalDepth == outdepth)
    {
        return NULL; // we want to step over but this is in
    }
    else if (globalDepth < outdepth) // returned before, stop somewhere
    {
        stop = true;
    }
    else if (!csThread || !in) return NULL;
    else if (globalDepth <= nextdepth ) 
        stop = true;
    else if (!stricmp(name, "Loop{}") || !strncmp(name, "If", 2) || !strnicmp(name, "else", 4))
    {
        if (StopIntended()) 
            idestop = true; // stop in or out if we are to stop at all
        return NULL;
    }
    else if (strchr(name, '{') && strchr(name,'.') && idestop)
    {
        return NULL;    // wait for action to trigger now.
    }

    else if (idestop)  
        stop = true;// stop button or startup request
    // stop because breakpoint
    else if (!disableBreakpoints) for (i = 0; i < breakIndex; ++i)
    {
        if (!stricmp(label, breakAt[i]) || !stricmp(label, transientBreak))
        {
            if (*name == '~') DebugPrint("Entering %s depth %d in %s mode\r\n", name, globalDepth, howTopic);
            else DebugPrint("Break entering %s(%d)\r\n", name, globalDepth);
            stop = true;
            break;
        }
    }
    if (!stop || !in) return NULL;

    breakType = BREAK_CALL;
    *transientBreak = 0;
    trace = 0;
    Wait();

    if (globalAbort) return (char*)1;// give up current input
        
    // and is awakened by someone else
    return NULL;
}

CALLFRAME* GetCodeOwnerFrame(int depth)
{
    ownerFrame = NULL;
    // find function or rule or topic owner
    ++depth;
    while (--depth > 0)
    {
        ownerFrame = GetCallFrame(depth);
        // LOOP and IF  dont own their code
        if (*ownerFrame->label == '^' && ownerFrame->code) break;
        else if (*ownerFrame->label == '^' ) break;
        // system functions must continue to backtrack to find an owner
        else if (*ownerFrame->label == '~') break; // topic or rule owns what happens
    }
    return ownerFrame;
}

// Find where we are for script window display
#ifdef INFORMATION
Possible callframes are:
~topic - a topic
~topic.n.n1() - a rule at pattern evaluation
~topic.n.n1{} - a rule executing its code
^function() - a function at argument evaluation
^function{} - a user function executing its code
^if() - a code if at condition evaluation
^if{} - a code if executing its code
^loop() - a code loop at loop count evaluation
^loop{} - a loop executing its code
---
Code can be associated with a rule or a function.
In such cases, we have entered the code when the outputlevel of the frame is 
less or equal to current outputlevel.
#endif

static bool WhereAmI(int depth)
{
    // find offset in file of current
    CALLFRAME* actualFrame = GetCallFrame(depth);
    CALLFRAME* ownerFrame = GetCodeOwnerFrame(depth);
    char* codebase = ownerFrame->code; 
    if (ownerFrame) // found owner level, now find display map entry
    {
        MAPDATA* map = AlignName(ownerFrame->label); // bring up the file/line of the named owner frame
        if (map && codebase && outputlevel >= ownerFrame->outputlevel) // found map and its in code itself
        {
            char* actualCode = outputCode[outputlevel]; // where it is executing now
            int offset = actualCode - codebase;
            int index;
            while (map) // now deal with offset within the owner
            {
                map = map->next;
                if (!IsDigit(map->name[0]))
                {
                    if (!strnicmp(map->name, "if", 2) || !strnicmp(map->name, "else", 4)) {}
                    else if (!strnicmp(map->name, "loop",4)) {}
                    else break; // end of line data
                }
                index = atoi(map->name);
                if (index >= offset)
                {
                    codePtr = actualCode;
                    codeFile = currentFileData;
                    mouseLine = map->fileline;
                    codeLine = mouseLine; // where are we
                    if (mouseLine > (scriptData.yposition + 1 + scriptData.linesPerPage))
                        scriptData.yposition = mouseLine - 3; // start with item showing in context
                    else if (mouseLine < scriptData.yposition + 1)
                        scriptData.yposition = mouseLine - 3; // start with item showing in context
                    offsetCode = offset;
                    char* src = ShowActions(actualCode, ownerFrame->outputlevel);
                    char name[MAX_WORD_SIZE];
                    strcpy(name, ownerFrame->label);
                    char* at = strchr(name, '{');
                    if (at) *at = 0;
                    DebugPrint("Code Break at %s:%d %s \r\n", name, offset,src);
                    return true;
                }
            }
        }
        else
        {
           AssignDepth(depth); // change script window
           codeLine = mouseLine; // where are we
           codeFile = currentFileData;
           codePtr = NULL;
        }
    }
    InvalidateRgn(scriptData.window, NULL, true);
    return false;
}

static char* DebugAction(char* ptr)
{
    if (globalAbort) return (char*)NULL;

    if (*ptr == '}' ) return NULL; // just a closing marker
    bool stop = false;
    int depth = globalDepth+1; // if inside () dont react
    CALLFRAME* frame;
    while (--depth)
    {
        frame = GetCallFrame(depth);
        if (strchr(frame->label, '('))
        {
            return NULL; // not stop in arg processing
        }
        if (strchr(frame->label, '{')) break; // legal to be here
    }

    frame = GetCallFrame(globalDepth);
    
    // cant stop in argument processing, only in expected level of call stack
    if (outputlevel != frame->outputlevel) return NULL;
    if (!stricmp(frame->label, "ruleoutput") && frame->code == ptr) return NULL; // we caught this already in DebugCall
    else if (*ptr == '`') return NULL;   // end of rule
    else if (outputlevel > frame->outputlevel) return NULL; // in argument processing
    else if (outputlevel < outlevel) // stop when you go out
    {
        stop = true;
    }
    else if (outputlevel <= nextlevel  || idestop)  
        stop = true; // triggered step in or next or out
    else
    {
        // find offset in file of current
        CALLFRAME* ownerFrame = GetCodeOwnerFrame(globalDepth);
        if (!ownerFrame) return NULL;
        char* codebase = ownerFrame->code;
        if (!disableBreakpoints && ownerFrame && codebase && ownerFrame->name) // found owner level, now find display map entry
        {
            char label[MAX_WORD_SIZE];
            sprintf(label, "%s:%d", ownerFrame->name->word, (ptr - codebase));
            for (int i = 0; i < breakIndex; ++i)
            {
                if (!stricmp(label, breakAt[i]) || !stricmp(label, transientBreak))
                {
                    if (*label == '~') DebugPrint("Entering %s\r\n", label);
                    else DebugPrint("Break entering %s(%d)\r\n", label, globalDepth);
                    stop = true;
                    break;
                }
            }
        }
    }
    if (stop)
    {
        breakType = BREAK_ACTION;
        trace = 0;
        Wait(); // WhereAmI sets where we are
    }
    return NULL;
}

static int ProcessBreak(char* input,int flags) // set breakpoint
{
    int answer = 0;
    char word[MAX_WORD_SIZE];
    while (*input)
    {
        input = ReadCompiledWord(input, word);
        char* dot = strchr(word, '.');
        char* colon = strchr(word, ':');
        if (!*word) break;
        if (!stricmp(word, "go")) answer = 1; // continue
        else if (*word == '!' && word[1] == '^' && !dot) RemoveBreak(word + 1); // remove this one
        else if (*word == '!' && word[1] == '~' && !strchr(word, '.')) RemoveBreak(word + 1); // remove this one
        else if (*word == '!' && word[1] == '~' && dot && dot[1] != '$') RemoveBreak(word + 1); // remove this one
        else if (*word == '!' && word[1] == '~' && dot && dot[1] == '$') RemoveVar(word + 1); // remove this one
        else if (*word == '!' && word[1] == '^' && dot && dot[1] == '$') RemoveVar(word + 1); // remove this one
        else if (*word == '^' && dot && dot[1] == '$') ProcessAssign(word);
        else if (*word == '~' && dot && dot[1] == '$') ProcessAssign(word);
        else if (!FindWord(word) && !dot && !colon) DebugPrint("    Cannot find %s\r\n", word);
        else
        {
            if (colon) debugAction = DebugAction;
            char base[MAX_WORD_SIZE];
            strcpy(base, word);
            char* at = strchr(base, '@');
            if (at) *at = 0;
            size_t len = strlen(base);

            int j;
            for (j = 0; j < breakIndex; ++j) // replacing?
            {
                if (!strnicmp(breakAt[j], word, len)
                    && (breakAt[j][len] == 0 || breakAt[j][len] == '@'))
                {
                    strcpy(breakAt[j], word);
                    break;
                }
            }
            if (flags) strcpy(transientBreak, word);
            if (j == breakIndex) strcpy(breakAt[breakIndex++], word); // new one
        }
    }
    if (breakIndex) debugCall = DebugCall;

    return answer;
}

char* DebugMessage(char* output)
{
    if (disableBreakpoints || globalAbort) return NULL;
    char* buf = AllocateBuffer();
    strcpy(buf, "UserOutput: ");
    strcat(buf, output);
    DebugOutput("Message break: ");
    DebugOutput(output);
    DebugOutput("\r\n");
    FreeBuffer();
    Wait();
    return NULL;
}

HWND MakeButton(char* name, int x, int y, int width, int height, long extra = 0)
{
   HWND window = CreateWindow(
        (LPCSTR)szWindowClass,  // Predefined class; Unicode assumed 
        (LPCSTR)name,      // Button text 
       WS_VISIBLE | WS_CHILDWINDOW | extra,  
       x, y, width, height, hParent, nullptr, hInst, nullptr);
   MoveWindow(window, x, y, width, height,true);
   return window;
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    *filenames = 0;
    ClearStops();
    *varnames = *varbreaknames = 0;
    memset(varAt, 0, sizeof(varAt));
    memset(breakAt, 0, sizeof(breakAt));
    breakIndex = 0;
    ZeroMemory(&si, sizeof(si));
    si.cbSize = sizeof(si);
    myBot = 0;
    hInst = hInstance; 
    changingEdit = true;
    debugCall = DebugCall; 
    debugEndTurn = DoneCallback;
   *filenames = 0;
   filenames[1] = 0;
   *varnames = 0;
   hParent = CreateWindow(szWindowClass, szTitle,  WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_MAXIMIZE | WS_SIZEBOX,
       0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);
   if (!hParent) return FALSE;

   ioWindow = CreateWindow(szWindowClass, szTitle, WS_VISIBLE  |
     WS_HSCROLL |
        WS_CAPTION | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
      WS_BORDER | WS_SIZEBOX  
       | ES_MULTILINE | WS_VSCROLL   | WS_BORDER
       | ES_LEFT, WS_MAXIMIZE | WS_SIZEBOX
       ,0, 0, 0, 0, nullptr, nullptr, nullptr, nullptr);
   if (!ioWindow) return FALSE;
 
   int xlimit = GetSystemMetrics(SM_CXMAXIMIZED);
   int ylimit = GetSystemMetrics(SM_CYMAXIMIZED);
   xlimit -= 30; // scroll
   ylimit -= 30; // scroll
   parentData.window = hParent;
   parentData.rect.left = 20;
   parentData.rect.right = xlimit;
   parentData.rect.top = 20;
   parentData.rect.bottom = ylimit;
   MoveWindow(hParent, parentData.rect.left, parentData.rect.top, parentData.rect.right, parentData.rect.bottom, true);
   
   hPen = CreatePen(PS_SOLID, 5, RGB(0, 0, 0));
   hBrush = CreateSolidBrush(RGB(0, 0, 0));
   hBrushBlue = CreateSolidBrush(RGB(0, 0, 255)); //https://yuilibrary.com/yui/docs/color/rgb-slider.html
   hBrushOrange = CreateSolidBrush(RGB(2555, 165, 0));
   hFont = CreateFont(fontsize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
       OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
       DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"));
   hFontBigger = CreateFont(fontsize+10, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
       OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
       DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"));
   hButtonFont = hFont;

   HDC dc = GetDC(parentData.window);
   SelectObject(dc, hFont);
   GetTextExtentPoint32(dc, (LPCTSTR)"ABCDEF", 6, &scriptData.metrics);
   ReleaseDC(parentData.window, dc);

   int butheight = scriptData.metrics.cy + 5;
   int butwidth = scriptData.metrics.cx + 10;
   hGoButton = MakeButton("Go", 10, 10, butwidth, butheight);
   hInButton = MakeButton("In", 10 + butwidth * 1, 10, butwidth, butheight);
   hOutButton = MakeButton("Out", 10 + butwidth * 2, 10, butwidth, butheight);
   hNextButton = MakeButton("Next", 10 + butwidth * 3, 10, butwidth, butheight);
   int zone2x = 10 + butwidth * 4 + (butwidth/2);
   hStopButton = MakeButton("Stop",  zone2x, 10,butwidth, butheight);
   hBreakMessageButton = MakeButton("Msg", zone2x + butwidth , 10, butwidth, butheight);
   hFailButton = MakeButton("Fail", zone2x + butwidth * 2, 10, butwidth, butheight);
   hClearButton = MakeButton("Clear", zone2x + butwidth * 3, 10, butwidth, butheight);
   int zone3x = zone2x + butwidth * 4 + (butwidth/2);
   hGlobalsButton = MakeButton("Global", zone3x, 10, butwidth, butheight);
   hBackButton = MakeButton("Back", zone3x + butwidth ,10, butwidth, butheight);
   hFontButton = MakeButton("-Font+", zone3x + butwidth * 2, 10,butwidth, butheight);
   int sentenceY = butheight + 10 + 10;
   statData.rect.bottom = ylimit - 120;
   statData.rect.top = statData.rect.bottom - butheight * 2;
      
   ide = true; // we have ide attached to cs
   idestop = false;
   
   // source text fits in this rect of scriptwindow
   scriptData.rect.left = 10;
   scriptData.rect.right = (xlimit * 2) / 3;
   scriptData.rect.top = sentenceY + 3 * butheight + 10;
   scriptData.rect.bottom = statData.rect.top - 30;
   scriptData.window = CreateWindow(szWindowClass, (LPCSTR) "script",
       WS_VISIBLE | WS_CHILDWINDOW| WS_HSCROLL | WS_VSCROLL 
       | WS_CAPTION | WS_BORDER | WS_SIZEBOX,
      0, 0, 0, 0, hParent, nullptr, hInstance, nullptr);
   if (!scriptData.window) return FALSE;

   int subwindowheight = scriptData.rect.bottom - scriptData.rect.top;
   subwindowheight -= 3 * butheight;    // input window
   subwindowheight -= 30; // 10 sep between windows
   subwindowheight /= 3; 

   scriptData.window = scriptData.window;
   MoveWindow(scriptData.window, scriptData.rect.left, scriptData.rect.top, scriptData.rect.right-scriptData.rect.left, scriptData.rect.bottom - scriptData.rect.top,true);
   ShowWindow(scriptData.window, SW_SHOW);
   UpdateWindow(scriptData.window);
   UpdateWindowMetrics(scriptData);

   UpdateWindowMetrics(parentData);

   inputData.rect.top = 0; // scriptData.rect.top;
   inputData.rect.bottom = 100; // inputData.rect.top + 2 * butheight;
   inputData.rect.left = 0; // scriptData.rect.right + 40;
   inputData.rect.right = 400; //  xlimit - 40;
   inputData.window = ioWindow;
   /* CreateWindow(
       "EDIT",
       (LPCSTR) "cs input",
       WS_BORDER | WS_SIZEBOX  // | WS_CAPTION -- with this we lose edit dataentry 
       | ES_MULTILINE | WS_VSCROLL | WS_CHILD | WS_VISIBLE | WS_BORDER |
       ES_WANTRETURN | ES_LEFT,  // | WS_CAPTION
       0, 0, 0, 0,
       hParent,         // parent window 
       (HMENU)ID_EDITCHILD1,   // edit control ID 
       (HINSTANCE)GetWindowLong(hParent, GWL_HINSTANCE),
       NULL);        // pointer not needed 
       */
   MoveWindow(inputData.window,
       inputData.rect.left, inputData.rect.top, // starting x- and y-coordinates 
       inputData.rect.right - inputData.rect.left,  // width of client area 
       inputData.rect.bottom - inputData.rect.top,   // height of client area 
       TRUE);
   SetFocus(inputData.window); // key input goes here
   SendMessage(inputData.window, WM_SETTEXT, 0, (LPARAM)"");
   SendMessage(inputData.window, WM_SETFONT, (WPARAM)hFont, false);
   SendMessage(inputData.window, EM_SETSEL, -1, -1);
   ShowWindow(inputData.window, SW_SHOW);
   UpdateWindow(inputData.window);
   UpdateWindowMetrics(inputData);

   // output from cs
   outputData.rect.top = inputData.rect.top;
   outputData.rect.left = inputData.rect.left;
   outputData.rect.right = inputData.rect.right;
   outputData.rect.bottom = inputData.rect.bottom;
   outputData.window = ioWindow;
     /* CreateWindow(
       "EDIT",
       (LPCSTR) "output",
       ES_READONLY | WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
       WS_BORDER | WS_SIZEBOX | WS_CAPTION | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
       0, 0, 0, 0, hParent,
       (HMENU)ID_EDITCHILD,
       (HINSTANCE)GetWindowLong(scriptData.window, GWL_HINSTANCE),
       NULL);        // pointer not needed 
   */
   SendMessage(outputData.window, WM_SETTEXT, 0, (LPARAM)"");
   SendMessage(outputData.window, WM_SETFONT, (WPARAM)hFont, false);
   SendMessage(outputData.window, EM_SETSEL, -1, -1);
  /* MoveWindow(outputData.window,
       outputData.rect.left, outputData.rect.top, // starting x- and y-coordinates 
       outputData.rect.right - outputData.rect.left,  // width of client area 
       outputData.rect.bottom - outputData.rect.top,   // height of client area 
       TRUE); */
   //ShowWindow(outputData.window, SW_SHOW);
   //UpdateWindow(outputData.window);
   UpdateWindowMetrics(outputData);

   // stats from cs
   statData.rect.left = scriptData.rect.left;
   statData.rect.right = outputData.rect.right;
   statData.window = CreateWindow(
       "EDIT",
       (LPCSTR) "stats",
       ES_READONLY | WS_CHILD | WS_VISIBLE  | WS_HSCROLL |
       WS_SIZEBOX | ES_LEFT | ES_AUTOHSCROLL,
       0, 0, 0, 0, hParent,
       (HMENU)ID_EDITCHILD,
       (HINSTANCE)GetWindowLong(statData.window, GWL_HINSTANCE),
       NULL);        // pointer not needed 
   SendMessage(statData.window, WM_SETTEXT, 0, (LPARAM)"");
   SendMessage(statData.window, WM_SETFONT, (WPARAM)hFont, false);
   SendMessage(statData.window, EM_SETSEL, -1, -1);
   MoveWindow(statData.window,
       statData.rect.left, statData.rect.top, // starting x- and y-coordinates 
       statData.rect.right - statData.rect.left,  // width of client area 
       statData.rect.bottom - statData.rect.top,   // height of client area 
       TRUE);
   ShowWindow(statData.window, SW_SHOW);
   UpdateWindow(statData.window);
   UpdateWindowMetrics(statData);

   sentenceData.window = MakeButton("sentence", 10, sentenceY, outputData.rect.right - scriptData.rect.left, 3 * butheight, WS_HSCROLL);

    varData.window = CreateWindow(
        szWindowClass,(LPCSTR) "global variables",
        WS_VISIBLE | WS_CHILDWINDOW | WS_HSCROLL | WS_VSCROLL
        | WS_CAPTION | WS_BORDER | WS_SIZEBOX, 
        0, 0, 0, 0, hParent,  
        0,
        (HINSTANCE)GetWindowLong(hParent, GWL_HINSTANCE),
        NULL);        // pointer not needed 
    varData.rect = outputData.rect;
    varData.rect.top = outputData.rect.bottom + 10;
    varData.rect.bottom = varData.rect.top + subwindowheight;
    MoveWindow(varData.window,
        varData.rect.left, varData.rect.top, // starting x- and y-coordinates 
        varData.rect.right - varData.rect.left,  // width of client area 
        varData.rect.bottom - varData.rect.top,   // height of client area 
        TRUE);
    ShowWindow(varData.window, SW_SHOW);
    UpdateWindow(varData.window);
    UpdateWindowMetrics(varData);

    stackData.window = CreateWindow(
        szWindowClass, (LPCSTR) "callstack",
        WS_VISIBLE | WS_CHILDWINDOW | WS_HSCROLL | WS_VSCROLL
        | WS_CAPTION | WS_BORDER | WS_SIZEBOX,
        0, 0, 0, 0,  hParent, 0,
        (HINSTANCE)GetWindowLong(hParent, GWL_HINSTANCE),
        NULL);        // pointer not needed 
    stackData.rect = varData.rect;
    stackData.rect.top = varData.rect.bottom + 10;
    stackData.rect.bottom = scriptData.rect.bottom;
    MoveWindow(stackData.window, stackData.rect.left, stackData.rect.top, stackData.rect.right - stackData.rect.left, stackData.rect.bottom - stackData.rect.top, true);

    ShowWindow(stackData.window, SW_SHOW);
    UpdateWindow(stackData.window);
    UpdateWindowMetrics(stackData);

    UpdateWindowMetrics(sentenceData,true);
    RestoreWindows();

    // rects have been set up now, get local coords
    GetClientRect(scriptData.window, &scriptData.rect);
    GetClientRect(varData.window, &varData.rect);
    GetClientRect(stackData.window, &stackData.rect);
    //GetClientRect(outputData.window, &outputData.rect);
    GetClientRect(inputData.window, &inputData.rect);
    GetClientRect(sentenceData.window, &sentenceData.rect);
    GetClientRect(statData.window, &statData.rect);
    // assign title space
    titlerect = scriptData.rect;
    titlerect.bottom = titlerect.top + scriptData.metrics.cy;
    titlerect.right = scriptData.rect.right;
    scriptData.rect.top += scriptData.metrics.cy;
    scriptData.maxLines -= 2;

    // associated subdivisions of script window
    tagrect = scriptData.rect;
    tagrect.left = 5;
    tagrect.right = scriptData.metrics.cx * 5;
    numrect = scriptData.rect;
    numrect.left = tagrect.right + 10;
    numrect.right = numrect.left + (scriptData.metrics.cx * 5);
    scriptData.rect.left = numrect.right + 30; // leave room
    
    StatData();

    printer = myprintf;
    csThread = _beginthread(MyWorkerInitThread, 0, NULL);
  

   return TRUE;
}

static MAPDATA* FindMapLine(int line) // get map entry for a line if there is one (line and rule and macro and topic entries)
{
    MAPDATA* at = currentFileData->map;
    priorMapLine = at;
    while (at )
    {
        if (!strnicmp(at->name, "if", 2) || !strnicmp(at->name, "else", 4)) {}
        else if (!strnicmp(at->name, "loop", 4)) {}
        else if (at->fileline >= line) break; // going to farr

        // record base of any offset for action
        if (*at->name == '~' || *at->name == '^') priorMapLine = at;
        at = at->next;
    }
    if (at && at->fileline == line)
        return at;
    return NULL;
}

static void ShowConcepts(int i)
{
    char word[MAX_WORD_SIZE];
    sprintf(word, "%s: --------\r\n", wordStarts[i]);
    DebugOutput(word);
    DebugConcepts(concepts[i], i);
    DebugConcepts(topics[i], i);
}

static void ChangeCodePtr(int line)
{
    MAPDATA* map = FindMapLine(line); // look this up
    CALLFRAME* frame = GetCallFrame(idedepth);
    if (!map) return;
    if (IsDigit(*map->name)) // want a line number (action), not a rule
    {
        codeOffset = atoi(map->name);
        if (frame->code)
        {
            char* code = frame->code + codeOffset;
            outputCode[frame->outputlevel] = code;
            if (idedepth == globalDepth) codeLine = line;
            InvalidateRgn(stackData.window, NULL, true);
            InvalidateRgn(scriptData.window, NULL, true);
            WhereAmI(idedepth);
            return;
        }
    }
    DebugOutput("Can only move around in actions, not in rules\r\n");
}

static void SetBreakpoint(int line,int flags)
{
    MAPDATA* map = FindMapLine(line); // look this up
 
    // temp block action lines
    if (map && IsDigit(*map->name)) codeOffset = atoi(map->name);
    else codeOffset = 0;
    if (map) // clicked on this line within  priorMapLine
    {
        char loc[MAX_WORD_SIZE];
        *loc = 0;
        // for lines within a macro or topic/rule
        if (*map->name != '^' && *map->name != '~')
        {
            strcpy(loc, priorMapLine->name); // rule tag OR code offset in script off macro or rule
            strcat(loc, ":");
        }
        strcat(loc, map->name); // either a duplicate of name OR an offset
        if (currentFileData->status[line] == ' ')  // enable
        {
            currentFileData->status[line] = 'B';
            ++breakpointCount;
        }
        else // disable back to normal
        {
            memcpy(loc + 1, loc, strlen(loc) + 1);
            *loc = '!';
            currentFileData->status[line] = ' ';
            --breakpointCount;
        }
  
        ProcessBreak(loc,flags); // engine request
        ide &= -1 ^ HAS_BREAKPOINTS;
        if (breakpointCount) ide |= HAS_BREAKPOINTS;
        InvalidateRgn(scriptData.window, NULL, true);
    }
}

#define ID_EDITCHILD 100
#define IDM_EDUNDO 1101
#define IDM_EDCUT 1102
#define IDM_EDCOPY 1103
#define IDM_EDPASTE 1104
#define IDM_EDDEL 1105
bool tesxt = false;
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    GetClientRect(hWnd, &rect);
    WINDOWSTRUCT *data;
    if (hWnd == scriptData.window) 
        data = &scriptData;
    else if (hWnd == outputData.window) 
        data = &outputData;
    else if (hWnd == parentData.window)
        data = &parentData;
    else if (hWnd == statData.window)
        data = &statData;
    else if (hWnd == stackData.window)
        data = &stackData;
    else if (hWnd == varData.window) 
        data = &varData;
    else if (hWnd == inputData.window) 
        data = &inputData;
    else if (hWnd == sentenceData.window)
        data = &sentenceData;
    switch (message)
	{
	case WM_CREATE:
		return 0;
	//case WM_SETFOCUS:
		//(hWnd);
		//return 0;
	case WM_HSCROLL:
        switch (LOWORD(wParam)) {
            case SB_THUMBTRACK:
            {  
                si.fMask = SIF_PAGE | SIF_TRACKPOS | SIF_RANGE | SIF_POS;
                if (!GetScrollInfo(hWnd, SB_HORZ, &si)) return 1; // GetScrollInfo failed
                data->xposition = si.nTrackPos;
                si.fMask = SIF_POS;
                si.nPos = si.nTrackPos;
                SetScrollInfo(hWnd, SB_HORZ, &si, true);
                break;
            }
            case SB_LINELEFT:
                if (data->xposition == 0) return 0; 
                --data->xposition;
                if (data->xposition < 0) data->xposition = 0;
                si.fMask = SIF_POS;
                si.nPos = data->xposition;
                SetScrollInfo(hWnd, SB_HORZ, &si, true);              
                break;
            case SB_PAGELEFT:
                if (data->xposition == 0) return 0;
                data->xposition -= data->maxChars - 10; // keep some context
                if (data->xposition < 0) data->xposition = 0;
                si.fMask = SIF_POS;
                si.nPos = data->xposition;
                SetScrollInfo(hWnd, SB_HORZ, &si, true);
                break;
            case SB_LINERIGHT:
                ++data->xposition;
                if (data->xposition > data->maxChars - data->charsPerPage) data->xposition = data->maxChars - data->charsPerPage;
                si.nPos = data->xposition;
                si.fMask = SIF_POS;
                si.nPos = data->xposition;
                SetScrollInfo(hWnd, SB_HORZ, &si, true);
                break;
            case SB_PAGERIGHT:
                data->xposition += data->maxChars - 10; // keep some context
                if (data->xposition > data->maxChars - data->charsPerPage) data->xposition = data->maxChars - data->charsPerPage;
                si.nPos = data->xposition;
                si.fMask = SIF_POS;
                si.nPos = data->xposition;
                SetScrollInfo(hWnd, SB_HORZ, &si, true);
                break;
        }
        InvalidateRgn(hWnd, NULL, true);
		return 0;
    case WM_MOUSEMOVE:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
    }
        break;
	case WM_VSCROLL:
        switch (LOWORD(wParam)) {
        case SB_THUMBTRACK:
        {
            si.fMask = SIF_PAGE|SIF_TRACKPOS | SIF_RANGE|SIF_POS;
            if (!GetScrollInfo(hWnd, SB_VERT, &si)) return 1; // GetScrollInfo failed
            data->yposition =  si.nTrackPos;
            si.nPos = si.nTrackPos;
            si.fMask = SIF_POS; 
            SetScrollInfo(hWnd, SB_VERT, &si, true);
            break;
        }
        case SB_PAGEUP:
            if (data->yposition == 0) return 0; 
            data->yposition -= data->linesPerPage - 3; // keep some context
            if (data->yposition < 0) data->yposition = 0;
             si.nPos = data->yposition;
            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &si, true);
            break;
        case  SB_LINEUP:
            if (data->yposition == 0) return 0;
            --data->yposition;
            si.nPos = data->yposition;
            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &si, true);
            break;
        case SB_PAGEDOWN:
            if (data->yposition == ( data->maxLines - data->linesPerPage)) return 0;
            data->yposition += data->maxLines - 3; // keep some context
            if (data->yposition > data->maxLines - data->linesPerPage) data->yposition = data->maxLines - data->linesPerPage;
            si.nPos = data->yposition;
            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &si, true);
            break;
        case SB_LINEDOWN:
            ++data->yposition;
            if (data->yposition > data->maxLines - data->linesPerPage) data->yposition = data->maxLines - data->linesPerPage;
            si.nPos = data->yposition;
            si.fMask = SIF_POS;
            SetScrollInfo(hWnd, SB_VERT, &si, true);
            break;
        }
        InvalidateRgn(hWnd, NULL, true);

        return 0;

	case  WM_KEYDOWN: // normal ascii characters in wParam
		if (wParam == VK_DOWN) // down
		{
            ++scriptData.yposition; 
            if (scriptData.yposition >= currentFileData->lineCount) scriptData.yposition = currentFileData->lineCount - 1;
            si.fMask = SIF_POS;
            si.nPos = scriptData.yposition;
            SetScrollInfo(hWnd, SB_VERT, &si, true);
            InvalidateRgn(hWnd, NULL, true);
		}
		else if (wParam == VK_UP) // up
		{
            --scriptData.yposition;
            if (scriptData.yposition < 0) scriptData.yposition = 0;
            si.fMask = SIF_POS;
            si.nPos = scriptData.yposition;
            SetScrollInfo(hWnd, SB_VERT, &si, true);
            InvalidateRgn(hWnd, NULL, true);
		}
		else if (wParam == VK_RIGHT) // right
		{
            ++scriptData.xposition;
            if (scriptData.xposition > currentFileData->charsize - scriptData.maxChars) scriptData.xposition = currentFileData->charsize - scriptData.maxChars;
            si.nPos = scriptData.xposition;
            si.fMask = SIF_POS;
            si.nPos = scriptData.xposition;
            SetScrollInfo(hWnd, SB_HORZ, &si, true);
            InvalidateRgn(hWnd, NULL, true);
		}
		else if (wParam == VK_LEFT) // left
		{
			--scriptData.xposition;
			if (scriptData.xposition < 0) scriptData.xposition = 0;
            si.fMask = SIF_POS;
            si.nPos = scriptData.xposition;
            SetScrollInfo(hWnd, SB_HORZ, &si, true);
			InvalidateRgn(hWnd, NULL, true);
		}
		return 0;

	case WM_LBUTTONDOWN: 
	case WM_RBUTTONDOWN: // jump to function or concept or topic
	{
        char word[MAX_WORD_SIZE];
        GetWindowText(hWnd, word, MAX_WORD_SIZE);
        int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
        if (hWnd == scriptData.window)
        {
            if (xPos >= scriptData.rect.left && xPos <= scriptData.rect.right &&
                yPos >= scriptData.rect.top && yPos <= scriptData.rect.bottom)
            {
                xPos -= scriptData.rect.left;
                xPos /= scriptData.metrics.cx; // avg size
                xPos += scriptData.xposition;
                mouseCharacter = xPos; // we pointed to this character
                yPos -= scriptData.rect.top; // where in text area?
                yPos /= scriptData.metrics.cy; // how many scriptData.maxLines in
                yPos += scriptData.yposition; // where is actual start of line
                ++yPos; // our 0 is file line 1 in map
                mouseLine = yPos; // this is the line we clicked on
                                  // find word referenced by click
                FindClickWord(mouseCharacter, mouseLine);
            }
            else if (xPos >= tagrect.left && xPos <= numrect.right &&
                yPos >= scriptData.rect.top && yPos <= scriptData.rect.bottom)
            { 
                // title list is outside of scriptData, so Line 1 of file  == click on 0 of here
                yPos -= scriptData.rect.top; // where in text area?
                yPos /= scriptData.metrics.cy; // how many scriptData.maxLines in
                yPos += scriptData.yposition; // where is actual start of line
                ++yPos; // our 0 is file line 1 in map
                mouseLine = yPos; // this is the file line# we clicked on
                if (message == WM_LBUTTONDOWN && wParam & MK_CONTROL)
                {
                    ChangeCodePtr(yPos);
                }
                else if (message == WM_LBUTTONDOWN) SetBreakpoint(mouseLine, false);  // simple break
                else // runto
                {
                    SetBreakpoint(mouseLine, true);
                    Go();
                }
            }
            else if (xPos >= titlerect.left && xPos <= titlerect.right &&
                yPos >= titlerect.top && yPos <= titlerect.bottom)
            {
                xPos -= titlerect.left;
                xPos /= scriptData.metrics.cx; // avg size
                xPos += scriptData.xposition; // point to this character
                size_t len = strlen(filenames);
                while (IsLegalNameCharacter(filenames[xPos])) --xPos;
                ++xPos;
                char name[MAX_WORD_SIZE];
                ReadCompiledWord(filenames + xPos, name);
                len = strlen(name);
                while (!IsLegalNameCharacter(name[len])) --len;
                name[len + 1] = 0;
                if (message == WM_LBUTTONDOWN) AlignName(name);
                else RemoveTab(name);
            }
            else
            {
                mouseCharacter = -1;
                mouseLine = -1;
            }
        }
        else if (hWnd == sentenceData.window)
        {
            if (message == WM_LBUTTONDOWN)
            {
                if (++sentenceMode > 2) sentenceMode = 0;
                ShowSentence();
            }
            else
            {
                xPos -= sentenceData.rect.left + 10;
                xPos /= sentenceData.metrics.cx; // avg size
                xPos += sentenceData.xposition;
                char* buffer = AllocateBuffer();
                GetWindowText(sentenceData.window,buffer,MAX_BUFFER_SIZE);
                char* in = buffer + xPos;
                int wordIndex = -1;
                for (int i = 0; i < xPos; ++i)
                {
                    if (buffer[i] == ' ') ++wordIndex;
                }
                FreeBuffer();
                ShowConcepts(wordIndex);
            }
        }
        else if (hWnd == varData.window)
        {
            xPos -= varData.rect.left;
            xPos /= varData.metrics.cx;
            xPos += varData.xposition;
            yPos -= varData.rect.top; // where in text area?
            yPos /= varData.metrics.cy; // how many scriptData.maxLines in
            yPos += varData.yposition; // where is actual start of line
            if (xPos > 2)
            {
               if (!fnvars) EraseVariable(yPos); // remove this
            }
            else DoBreakVariable(yPos);
            InvalidateRgn(varData.window, NULL, true);
        }
        else if (hWnd == stackData.window)
        {
            yPos -= stackData.rect.top; // where in text area?
            yPos /= stackData.metrics.cy; // how many scriptData.maxLines in
            yPos += stackData.yposition; // where is actual start of line
            yPos += 1; // stack starts at 1
            if (yPos > globalDepth || yPos <= 0) break; // ignore bad values
            StackVariables(yPos); 
            idedepth = yPos;
            
            sprintf(windowTitle, "%s Depth: %d/%d", GetCallFrame(idedepth)->label, idedepth, globalDepth);
            SetWindowText(stackData.window, (LPCTSTR)windowTitle);
            WhereAmI(yPos);
            InvalidateRgn(varData.window, NULL, true);
            InvalidateRgn(stackData.window, NULL, true);
        }
        else if (hWnd == hGoButton)
        {
            outdepth = outlevel = -1;
            nextdepth = nextlevel = -1;
            if (message == WM_LBUTTONDOWN) trace = 0;
            else { trace = -1; echo = true; }
            Go();
        }
        else if (hWnd == hClearButton) 
        {
            if (message == WM_LBUTTONDOWN)
            {
                ClearAllBreakpoints();
                DebugOutput("Breakpoints cleared\r\n");
            }
            else
            {
                DisableAllBreakpoints();
                if (disableBreakpoints) DebugOutput("Breakpoints disabled\r\n");
                else DebugOutput("Breakpoints re-enabled\r\n");
            }
        }
        else if (hWnd == hFailButton)
        {
            if (message == WM_LBUTTONDOWN)
            {
                if (wParam & MK_CONTROL)
                {
                    globalAbort = true;
                    DebugOutput("Abort input\r\n");
                    Go();
                }
                else
                {
                    sysfail = true;
                    DebugOutput("Break on system function failure\r\n");
                }
            }
            else
            {
                sysfail = false;
                DebugOutput("Cleared break on system function failure\r\n");
            }
        }
        else if (hWnd == hFontButton)
        {
            if (message == WM_LBUTTONDOWN) // smaller
            {
                if (fontsize <= 10) break;
                fontsize -= 10;
                hFont = CreateFont(fontsize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"));
            }
            else // bigger
            {
                if (fontsize >= 80) break;
                fontsize += 10;
                hFont = CreateFont(fontsize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET,
                    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, TEXT("Courier New"));
            }
            SendMessage(outputData.window, WM_SETFONT, (WPARAM)hFont, false);
            SendMessage(scriptData.window, WM_SETFONT, (WPARAM)hFont, false);
            SendMessage(varData.window, WM_SETFONT, (WPARAM)hFont, false);
            SendMessage(stackData.window, WM_SETFONT, (WPARAM)hFont, false);
            UpdateMetrics();
        }
        else if (hWnd == hStopButton)
        {
            debugCall = DebugCall;
            idestop = true;
            DebugOutput("stop set\r\n");
        }
        else if (hWnd == hNextButton)
        {
            // next is over or out
            outlevel = outputlevel;
            outdepth = globalDepth; // out can be next
            nextdepth = globalDepth;
            nextlevel = outputlevel;

            debugCall = DebugCall;
            debugAction = DebugAction;

            if (message == WM_LBUTTONDOWN) trace = 0;
            else { trace = idetrace; if (!trace) trace = -1;}
            Go();
        }
        else if (hWnd == hInButton)
        {
            // IN means stop as soon as you can
            debugAction = DebugAction;
            debugCall = DebugCall;
            idestop = true; // break anywhere in or over or out
            if (message == WM_LBUTTONDOWN) trace = 0;
            else { trace = idetrace; if (!trace) trace = -1; }
            Go();
        }
        else if (hWnd == hOutButton)
        {
            if (message == WM_LBUTTONDOWN) trace = 0;
            else { trace = idetrace; if (!trace) trace = -1;}

            // OUT means leave either current action level or call depth and 
            // stop as soon as possible thereafter
            outlevel = outputlevel;
            outdepth = globalDepth;
            debugCall = DebugCall;
            Go();
        }
        else if (hWnd == hBreakMessageButton)
        {
            if (message != WM_LBUTTONDOWN)
            {
                debugMessage = NULL;
                DebugOutput("Message break turned off\r\n");
            }
            else
            {
                debugMessage = DebugMessage;
                DebugOutput("Message break turned on\r\n");
            }
        }
        else if (hWnd == hGlobalsButton) UseGlobals();
        else if (hWnd == hBackButton)
        {
            RestoreClick();
        }
    }
		return 0;

    case WM_EXITSIZEMOVE:
        SaveWindows();
        break;
	case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
			int val = HIWORD(wParam);
            switch (val) 
            {
            case BN_CLICKED:
               break;
            case EN_CHANGE:
                if (lParam == (LPARAM)inputData.window && !changingEdit)
                {
                    CheckForEnter();
                }
                break;
            }

            switch (wmId)
            {
				case IDM_EDUNDO:
					// Send WM_UNDO only if there is something to be undone. 
					if (SendMessage(hWnd, EM_CANUNDO, 0, 0))
						SendMessage(hWnd, WM_UNDO, 0, 0);
					else
					{
						MessageBox(hWnd,
							(LPCSTR)"Nothing to undo.",
							(LPCSTR)"Undo notification",
							MB_OK);
					}
					break;

				case IDM_EDCUT:
					SendMessage(hWnd, WM_CUT, 0, 0);
					break;

				case IDM_EDCOPY:
					SendMessage(hWnd, WM_COPY, 0, 0);
					break;

				case IDM_EDPASTE:
					SendMessage(hWnd, WM_PASTE, 0, 0);
					break;

				case IDM_EDDEL:
					SendMessage(hWnd, WM_CLEAR, 0, 0);
					break;
				case IDM_ABOUT:
				{
					int bufferLen = GetWindowTextLength(hWnd) + 1; // why send the message twice?
					//string data(bufferLen, '\0');
					//GetWindowText(hWnd, (LPCSTR)data, bufferLen);
					// DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
					
					FILE* in = fopen("test.txt", "rb");
					if (in)
					{ 
						fseek(in, 0, SEEK_END);   // non-portable
						int size = ftell(in);
						fseek(in, 0, SEEK_SET);   // non-portable
						char* msg = (char*)malloc(size + 4);
						fread(msg, 1, size, in);
						fclose(in);
						SendMessage(hWnd, WM_SETTEXT, 0, (LPARAM)msg);
					}
				}
					break;
				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;

				case WM_KEYDOWN:
					if (hWnd == hWnd)
					{
						if (val == VK_DOWN)
						{
							int xx = 0;
						}
					}
					
				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
        }
        break;
    
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        RECT rect;
        GetClientRect(hWnd, &rect);
        RECT breakrect;
        breakrect = rect;
        rect.left += 3 * varData.metrics.cx;
        SelectObject(hdc, hFont);
        SetTextColor(hdc, RGB(0, 0, 0)); // black text
        SetBkColor(hdc, RGB(255, 255, 255)); // white background
        if (hWnd == varData.window)
        {
            char msg[MAX_WORD_SIZE];
            char myvars[MAX_WORD_SIZE];
            strcpy(myvars, varnames);
            char* var = myvars;
            int d = fnlevel;
            CALLFRAME* frame = GetCallFrame(d);
            // system call
            if (frame && frame->name && *frame->name->word == '^' && !frame->code)
            {
                for (unsigned int i = 0; i < frame->arguments; ++i)
                {
                    char data[MAX_WORD_SIZE];
                    char* msg = callArgumentList[frame->argumentStartIndex + i];
                    size_t len = strlen(msg);
                    if (len > (MAX_WORD_SIZE - 20)) len = MAX_WORD_SIZE - 20;
                    sprintf(data, "(%d) ", i + 1);
                    char * at = data + strlen(data);
                    strncpy(at, msg, len);
                    at[len] = 0;
                    DrawText(hdc, (LPCSTR)data, -1, &rect, DT_LEFT | DT_TOP);
                    rect.top += varData.metrics.cy;
                }
                EndPaint(hWnd, &ps);
                break;
            }

            if (fnvars) var = (char*)(fnvars + 1); // start of
            //else if (fnlevel >= 0) 
            //    var = "";
            size_t len;
            char name[MAX_WORD_SIZE];
            int limit = varData.linesPerPage;
            int lineCount = 0;
            while (*var && *var != ')')
            {
                char* at = msg;

                // get next variable name
                if (!fnvars)
                {
                    char* end = strstr(var, "\r\n");
                    if (end) *end = 0;
                    strcpy(name, var);
                    if (end) *end = '\r';
                    var = end + 2;
                }
                else // fn var
                {
                    var = ReadCompiledWord(var, name);
                }

                if (++lineCount > limit) break;

                // get value and line to show
                char* value = NULL;
                char* buffer = NULL;
                char word[MAX_WORD_SIZE];
                if (*name == '^') value = ChaseFnVariable(name, fnlevel);
                else if (*name == '$') value = GetUserVariable(name);
                else if (*name == '@')
                {
                    int store = GetSetID(name);
                    unsigned int count = FACTSET_COUNT(store);
                    sprintf(word, "[%d]", count);
                    value = word;
                }
                else if (*name == '%')  value = SystemVariable(name, NULL);
                else if (*name == '_')
                {
                    int index = GetWildcardID(name ); //   which one
                    value = wildcardOriginalText[index];
                }
                else if (*name == 'j' && name[2] == '-')
                {
                    buffer = AllocateBuffer();
                    WORDP D = FindWord(name);
                    jwrite(buffer, D, true);
                    value = buffer;
                }
                if (!value) continue;
                len = strlen(value);
                if (!*value) value = "null";

                int brk = FindVarBreakValue(name);
                if (brk >= 0)
                {
                    sprintf(at, "%s(%d)>%s< = ", name, len,varvalue[brk]);
                }
                else if (*name != '@') sprintf(at, "%s(%d)= ", name, len);
                else sprintf(at, "%s = ", name);
                at = at + strlen(at);
                bool more = false;
                size_t len = strlen(value);
                if (len > 400)
                {
                    more = true;
                    len = 400;
                }
                strncpy(at, value, len);
                at += len;
                *at = 0;
                if (more) strcpy(at, "...");// excessive length for display
                
                DrawText(hdc, (LPCSTR)msg, -1, &rect, DT_LEFT| DT_TOP);
                if (buffer) FreeBuffer();

                char junk[MAX_WORD_SIZE];
                sprintf(junk, "%s\r\n", name);
                if (strstr(varbreaknames, junk))
                {
                    DrawText(hdc, (LPCSTR)"B", -1, &breakrect, DT_LEFT | DT_TOP);
                }

                rect.top += varData.metrics.cy;
            }
        }
        else if (hWnd == stackData.window)
        {
            char msg[MAX_WORD_SIZE];
            CALLFRAME* frame;
            int limit = stackData.yposition + stackData.linesPerPage;

            for (int i = 1; i <= globalDepth; ++i)
            {
                *msg = 0;
                if (i < stackData.yposition) continue; // wait to start
                if (i > limit) break;
                frame = GetCallFrame(i);
                bool showcode = false;
                if (strchr(frame->label, '(')) { ; } // not in code yet
                else if (!strnicmp(frame->label, "If", 2) || !strnicmp(frame->label, "Loop", 4)) showcode = true;
                else if (*frame->label == '^' && frame->code) showcode = true;
                else if (*frame->label != '^' && strchr(frame->label,'{')) showcode = true;
                sprintf(msg, "%d: %s", i, frame->label);
                if (showcode)
                {
                    char* at = strchr(msg, '{');
                    if (at) *at = 0; // not for functions
                    CALLFRAME* ownerFrame = GetCodeOwnerFrame(i);
                    char size[20];
                    
                    if (!stricmp(frame->label, "Loop{}"))
                    { // show loop count
                        char extra[10];
                        sprintf(extra,":%d", frame->x.ownvalue);
                        strcat(msg, extra);
                        strcat(msg, ShowActions(outputCode[frame->outputlevel], frame->outputlevel));
                    }
                    else if (outputlevel >= ownerFrame->outputlevel)
                    { // at or beyond local code block
                        int offset = outputCode[ownerFrame->outputlevel] - ownerFrame->code;
                        sprintf(size, ":%d", offset);
                        strcat(msg, size);
                        if (*frame->label == '^' && frame->code && !offset) strcat(msg, "(macro) ");
                        strcat(msg, " ");
                        strcat(msg,ShowActions(outputCode[frame->outputlevel], frame->outputlevel));
                    }
                }
                else
                {
                    if (*frame->label == '^' && frame->code) strcat(msg, " (macro)");
                }
                size_t len = strlen(msg);
                if (idedepth == i)
                {
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkColor(hdc, RGB(0, 0, 255));
                }
                else
                {
                    SetTextColor(hdc, RGB(0, 0, 0));
                    SetBkColor(hdc, RGB(255, 255, 255));
                }
                int n = DrawText( hdc, (LPCSTR)msg, -1, &rect, DT_LEFT| DT_TOP);
                rect.top += stackData.metrics.cy;
            }
        }
        else if (hWnd == scriptData.window)
        {
           if (tesxt) VerifyMap();
            rect = scriptData.rect;
            int limit = scriptData.yposition + scriptData.linesPerPage;

            numrect.top = rect.top;
            tagrect.top = rect.top;

            // title of visible doc
            if (currentFileData)
            {
                DrawText( hdc, (LPCSTR)filenames, -1, &titlerect, DT_LEFT|DT_TOP );
            }
            LINEDATA* at = GetStartLine(scriptData.yposition);
            if (!at) break;
            int yline = scriptData.yposition; // 0-based but mouseline and codeline are 1-based
            char* msg;
            while (at)
            {
                if (yline > limit) break;
                msg = GetStartCharacter(at);
                if (!msg) break;
                size_t len = strlen(msg);
               
                ++yline; // now 1 based data (matched file line numbers)
                char stuff[MAX_WORD_SIZE];
                *stuff = 0;
                if (at->mapentry && at->mapentry->name[0] == '~')
                {
                    // is it a rule id
                    char* start = strchr(at->mapentry->name, '.');
                    if (start)
                    {
                        strcpy(stuff, start + 1);
                        char* label = strchr(stuff, '-');
                        if (label) *label = 0; // pure tag
                        DrawText(hdc, (LPCSTR)stuff, -1, &tagrect, DT_RIGHT | DT_TOP);
                    }
                }
                // code code location
                if (codeFile == currentFileData && codeLine == yline)
                {
                    RECT r = tagrect;
                    r.left = 0;
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkColor(hdc, RGB(0, 0, 255));
                    DrawText(hdc, (LPCSTR)">>", -1, &r, DT_LEFT | DT_TOP);
                    SetTextColor(hdc, RGB(0, 0, 0));
                    SetBkColor(hdc, RGB(255, 255, 255));
                }
                if (yline == mouseLine) // highlight text
                {
                    SetTextColor(hdc, RGB(255, 255, 255));
                    SetBkColor(hdc, RGB(0, 0, 255));
                    DrawText(hdc, (LPCSTR)msg, -1, &rect, DT_LEFT | DT_TOP);
                    SetTextColor(hdc, RGB(0, 0, 0));
                    SetBkColor(hdc, RGB(255, 255, 255));
                }
                else DrawText(hdc, (LPCSTR)msg, -1, &rect, DT_LEFT | DT_TOP);

                // show source line numbers
                char num[50];
                sprintf(num, "%d %c", yline, currentFileData->status[yline]);
                DrawText( hdc, (LPCSTR)num, -1, &numrect, DT_RIGHT| DT_TOP);
                *stuff = 0;
                if (at->mapentry && at->mapentry->name[0] == '~')
                {
                    // is it a rule id
                    char* start = strchr(at->mapentry->name, '.');
                    if (start)
                    {
                        strcpy(stuff, start+1);
                        char* label = strchr(stuff, '-');
                        if (label) *label = 0; // pure tag
                        DrawText( hdc, (LPCSTR)stuff, -1, &tagrect, DT_RIGHT| DT_TOP);
                    }
                }
 
                rect.top += scriptData.metrics.cy;
                numrect.top += scriptData.metrics.cy;
                tagrect.top += scriptData.metrics.cy;
                at = at->next; // use forward ptr
            }
            GetClientRect(hWnd, &rect);
            FrameRect(hdc, &rect, hBrush);
        }
        else if (hWnd == hParent) 
        { 
            GetClientRect(hWnd, &rect);
            FillRect(hdc, &rect, hBrushOrange);
        }
        else if (hWnd == sentenceData.window)
        {
            SelectObject(hdc, hFontBigger);
            GetClientRect(hWnd, &rect);
            FrameRect(hdc, &rect, hBrush);
            char word[MAX_WORD_SIZE];
            GetWindowText(hWnd, word, MAX_WORD_SIZE);
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkColor(hdc, RGB(255, 255, 255));
            rect.top += 10;
            rect.left += 10;
            DrawText(hdc, (LPCSTR)word, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_TOP);
        }
        else // some button rect
        {
            SelectObject(hdc, hButtonFont);
            GetClientRect(hWnd, &rect);
            FrameRect(hdc, &rect, hBrush);
            char word[MAX_WORD_SIZE];
            GetWindowText(hWnd, word, MAX_WORD_SIZE);
            SetTextColor(hdc, RGB(0, 0, 0));
            SetBkColor(hdc, RGB(255, 255, 255));
            DrawText(hdc, (LPCSTR)word, -1, &rect, DT_SINGLELINE|DT_CENTER | DT_VCENTER);
        }
        EndPaint(hWnd, &ps);
    }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        if (HIWORD(wParam) == EN_CHANGE)
        {
            int xx = 0;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
