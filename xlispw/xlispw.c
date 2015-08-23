/* xlispw.c - the main routine for xlispw */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <setjmp.h>
#include <windows.h>
#include <commctrl.h>
#include "xlisp.h"
#include "xlispw.h"
#include "widget.h"
#include "winstuff.h"

/* globals */
char appName[] = "XLispW";
HINSTANCE hInstance;

/* locals */
static int argc = 0;
static char **argv = NULL;
static jmp_buf exitTarget;
static int exitStatus;

/* prototypes */
static xlValue xMessageLoop(void);
static int CheckForMessages(void);
static int ParseCommandLine(void);
static char *GetNextArgument(char **pCmdLine,char **pEnd);

/* WinMain - the main routine */
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hPrevInst,PSTR cmdLine,int cmdShow)
{
    xlCallbacks *callbacks;

    /* remember the instance handle */
    hInstance = hInst;

    /* make sure the common controls are available */
    InitCommonControls();
    
    /* load the rich text library */
    if (LoadLibrary("RICHED32.DLL") == NULL)
        return 0;

    /* parse the command line */
    if (!ParseCommandLine())
        return 0;

    /* get the default callback structure */
    callbacks = xlDefaultCallbacks(argv[0]);
    SetupCallbacks(callbacks);

    /* initialize the listener */
    callbacks->consoleGetC = ConsoleGetC;
    callbacks->consoleFlushInput = ConsoleFlushInput;
    callbacks->consolePutC = ConsolePutC;
    callbacks->consoleFlushOutput = ConsoleFlushOutput;
    callbacks->consoleAtBOLP = ConsoleAtBOLP;
    callbacks->consoleCheck = ConsoleCheck;

    /* setup the exit target */
    if (setjmp(exitTarget) != 0)
        return exitStatus;

    /* startup xlisp */
    if (!InvokeXLISP(callbacks,argc,argv))
        return -1;

    /* add built-in functions */
    xlSubr("MESSAGE-LOOP",xMessageLoop);
    
    /* message dispatch loop */
    for (;;)
        CheckForMessages();
}

/* xMessageLoop - built-in function 'message-loop' */
static xlValue xMessageLoop(void)
{
    xlValue val;

    /* parse the arguments */
    xlVal = xlGetArg();
    xlLastArg();
    
    /* wait for the thunk to return #f */
    for (;;) {

        /* call the thunk */
        if (xlCallFunction(&val,1,xlVal,0) > 0 && val == xlFalse)
            return xlTrue;

        /* check for messages */
        CheckForMessages();
    }
}

/* CheckForMessages - check the message queue for messages */
static int CheckForMessages(void)
{
    HACCEL hAccelTable;
    HWND hAccelWnd;
    MSG msg;
    
    /* check for messages */
    if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            exitStatus = msg.wParam;
            longjmp(exitTarget,1);
        }
        else {
            if (!GetAcceleratorTable(&hAccelWnd,&hAccelTable)
            ||  !TranslateAccelerator(hAccelWnd,hAccelTable,&msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            return TRUE;
        }
    }

    /* no messages */
    xlIdle();
    return FALSE;
}

/* ParseCommandLine - parse the command line */
static int ParseCommandLine(void)
{
    char *cmdLine,*start,*end,*p;
    size_t vectorSize = sizeof(char *);
    size_t argSize = 0;

    /* get the command line */
    cmdLine = p = (char *)GetCommandLine();

    /* count the number of arguments */
    while ((start = GetNextArgument(&p,&end)) != NULL) {
        vectorSize += sizeof(char *);
        argSize += p - start + 1;
    }

    /* allocate and initialize the argument vector */
    if (!(argv = (char **)malloc(vectorSize + argSize)))
        return FALSE;

    /* setup point to argument space */
    p = (char *)argv + vectorSize;

    /* insert each argument */
    for (argc = 0; start = GetNextArgument(&cmdLine,&end); ++argc) {
        int length = end - start;
        strncpy(p,start,length);
        argv[argc] = p;
        p += length;
        *p++ = '\0';
    }
    argv[argc] = NULL;

    /* return successfully */
    return TRUE;
}

/* GetNextArgument - get the next argument from a command line */
static char *GetNextArgument(char **pCmdLine,char **pEnd)
{
    char *cmdLine,*start;
    int ch;

    /* get the command line pointer */
    cmdLine = *pCmdLine;

    /* skip leading spaces */
    for (;;) {
        if ((ch = *cmdLine) == '\0')
            return NULL;
        else if (!isspace(ch))
            break;
        ++cmdLine;
    }

    /* handle a quoted argument */
    if (ch == '"' || ch == '\'') {
        int delimiter = ch;
        start = ++cmdLine;
        while ((ch = *cmdLine) != '\0' && ch != delimiter)
            ++cmdLine;
        *pEnd = cmdLine;
        if (ch == delimiter)
            ++cmdLine;
    }
    
    /* handle an space delimited argument */
    else {
        start = cmdLine;
        while ((ch = *cmdLine) != '\0' && !isspace(ch))
            ++cmdLine;
        *pEnd = cmdLine;
    }

    /* return the start of the argument and updated command line pointer */
    *pCmdLine = cmdLine;
    return start;
}

/* ShowErrorMessage - show a window error message and exit */
void ShowErrorMessage(DWORD err)
{
    char *msgBuf;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        err,
        MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
        (LPTSTR)&msgBuf,
        0,
        NULL);
    MessageBox(NULL,msgBuf,"GetLastError",MB_OK|MB_ICONINFORMATION);
    LocalFree(msgBuf);
    exitStatus = err;
    longjmp(exitTarget,1);
}