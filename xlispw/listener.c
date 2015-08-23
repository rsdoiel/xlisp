/* listener.c - listener routines for xlispw */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include "widget.h"

/* listener widget */
static Widget *listener = NULL;

/* log file */
static FILE *logFP = NULL;

/* terminal output buffer */
#define OUTBUFSIZE 1024
static char outBuf[OUTBUFSIZE + 1];
static char *outPtr = outBuf;

/* beginning of line */
static int atBOLP = TRUE;

/* function handlers */
static xlValue xSetupListener(void);
static xlValue xCollectExpression(void);

static xlSubrDef subrtab[] = {
{   "COLLECT-EXPRESSION",       xCollectExpression  },
{0,0}};

static xlXSubrDef xsubrtab[] = {
{0,0}};

/* externals */
extern xlValue c_basicEditText;

/* prototypes */
static void GuaranteeListenerSpace(HWND wnd);

/* InitListener - initialize the listener functions */
void InitListener(void)
{
    xlSubrDef *sdp;
    xlXSubrDef *xsdp;

    /* enter the built-in functions */
    for (sdp = subrtab; sdp->name != NULL; ++sdp)
        xlSubr(sdp->name,sdp->subr);
    for (xsdp = xsubrtab; xsdp->name != NULL; ++xsdp)
        xlXSubr(xsdp->name,xsdp->subr);

    /* add methods to basic-edit-text */
    xlMethod(c_basicEditText,"SETUP-LISTENER",xSetupListener);
}

/* xSetupListener - built-in function 'setup-listener' */
static xlValue xSetupListener(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlLastArg();

    /* flush the current output buffer */
    if (logFP) {
        fclose(logFP);
        logFP = NULL;
    }

    /* store the current listener */
    listener = GetWidget(obj);

    /* return the object */
    return obj;
}

/* GuaranteeListenerSpace - make sure there is enough space in the listener window */
static void GuaranteeListenerSpace(HWND wnd)
{
    int lineCount = SendMessage(wnd,EM_GETLINECOUNT,0,0);
    int charCount = SendMessage(wnd,EM_LINEINDEX,lineCount - 1,0);
    if (charCount > 20000) {
        long saveStart,saveEnd;
        int lineToDelete = SendMessage(wnd,EM_LINEFROMCHAR,2000,0);
        int endDelete = SendMessage(wnd,EM_LINEINDEX,lineToDelete,0);
        SendMessage(wnd,EM_GETSEL,(WPARAM)&saveStart,(LPARAM)&saveEnd);
        SendMessage(wnd,EM_SETSEL,0,endDelete);
        SendMessage(wnd,WM_CLEAR,0,0);
        saveStart -= endDelete;
        saveEnd -= endDelete;
        if (saveStart < 0) {
            saveStart = 0;
            saveEnd = -1;
        }
        SendMessage(wnd,EM_SETSEL,saveStart,saveEnd);
    }
}

/* ConsoleGetC - get a character from the console */
int ConsoleGetC(void)
{
    ConsoleFlushOutput();
    return EOF;
}

/* ConsoleFlushInput - flush the console input buffer */
void ConsoleFlushInput(void)
{
}

/* ConsolePutC - put a character to the console */
void ConsolePutC(int ch)
{
    /* check for the end of a line */
    if (ch == '\n') {
        ConsolePutC('\r');
        atBOLP = TRUE;
    }
    else
        atBOLP = FALSE;
    
    /* buffer the character */
    if (outPtr >= &outBuf[OUTBUFSIZE])
        ConsoleFlushOutput();
    *outPtr++ = ch;
}

/* ConsoleFlushOutput - flush the console output buffer */
void ConsoleFlushOutput(void)
{
    if (outPtr != outBuf) {

        /* terminate the output buffer */
        *outPtr = '\0';

        /* output to listener window */
        if (listener) {
            GuaranteeListenerSpace(listener->wnd);
            SendMessage(listener->wnd,EM_REPLACESEL,FALSE,(LPARAM)outBuf);
        }

        /* output to log file */
        else {

            /* open the log file if it hasn't already been opened */
            if (!logFP)
                logFP = fopen("xlispw.log","wb");

            /* write the buffer to the file */
            if (logFP) {
                fputs(outBuf,logFP);
                fflush(logFP);
            }
        }

        /* clear the output buffer */
        outPtr = outBuf;
    }
}

/* ConsoleAtBOLP - check to see if the console is at the beginning of a line */
int ConsoleAtBOLP(void)
{
    ConsoleFlushOutput();
    return atBOLP;
}

/* ConsoleCheck - check for a control character */
int ConsoleCheck(void)
{
    return 0;
}

/* xCollectExpression - built-in function 'collect-expression' */
static xlValue xCollectExpression(void)
{
    int gotSomethingP = FALSE;
    int inStringP = FALSE;
    int parenLevel = 0;
    char *buffer,*p;
    xlValue str;
    int ch;

    /* parse the argument list */
    str = xlGetArgString();
    xlLastArg();

    /* initialize the string pointers to start at the end */
    buffer = xlGetString(str);
    p = buffer + strlen(buffer);

    /* skip initial spaces */
    while (p > buffer && isspace(p[-1]))
        --p;
    
    while (p > buffer) {
        ch = *--p;
        if (p >= buffer && *p == '\\')
            gotSomethingP = TRUE;
        else {
            if (inStringP) {
                switch (ch) {
                case '"':
                    gotSomethingP = TRUE;
                    inStringP = FALSE;
                    if (parenLevel == 0)
                        goto done;
                    break;
                }
            }
            else {
                switch (ch) {
                case ')':
                    ++parenLevel;
                    break;
                case '(':
                    if (parenLevel == 0) {
                        ++p;
                        goto done;
                    }
                    else if (--parenLevel == 0) {
                        gotSomethingP = TRUE;
                        goto done;
                    }
                    break;
                case '"':
                    inStringP = TRUE;
                    break;
                default:
                    if (isspace(ch)) {
                        if (parenLevel == 0)
                            goto done;
                    }
                    else if (parenLevel == 0)
                        gotSomethingP = TRUE;
                    break;
                }
            }
        }
    }

done:
    /* return the expression */
    return gotSomethingP ? xlMakeCString(p) : xlNil;
}
