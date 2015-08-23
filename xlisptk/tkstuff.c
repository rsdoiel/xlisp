/*
 * tcl/tk interfaces for Xlisp 3.0
 */

/* $Id: tkstuff.c,v 1.22 2002/09/06 14:22:21 dbetz Exp $ */

#define LOG_XLISP
#define LOG_TCL

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef WIN32
//#define USE_MDI
//#define CONSOLE
#include <windows.h>
#include <locale.h>
#ifdef USE_MDI
#include <tkwin.h>
#endif
#else
#include <sys/wait.h>
#endif

#include <tcl.h>
#include <tk.h>

#include "xlisp.h"
#include "tkstuff.h"

#ifdef USE_MDI
int setparent_cmd(ClientData ourdata,Tcl_Interp *interp,int argc,char *argv[]);
#endif

/* the global Tcl interpreter */
Tcl_Interp *tcl_interp;

/* flag to indicate the interpreter started up OK */
int tcl_init_ok;

/* foreign type symbols */
xlValue s_tkobj;

static xlValue xrunprogram(void);
static xlValue xtcl(void);
static xlValue xtclverbose(void);

xlSubrDef tksubrtab[] = {
{       "TCL",                    xtcl              },
{       "TCL-VERBOSE",            xtclverbose       },
{0,0}};

xlXSubrDef tkxsubrtab[] = {
{0,0}};

/* prototypes */
static xlValue (*FindSubr(char *name))(void);
static int InvokeXLISP(Tcl_Interp *interp,char *softwareType);
static int SetupTclPath(Tcl_Interp *interp);
static void tkenter(void);
static xlValue (*tkfindsubr(char *name))(void);
static void SetupCallbacks(xlCallbacks *callbacks);
static char *LoadPath(void);
static char *ParsePath(char **pp);
static int DirectorySeparator(void);
static void Error(char *msg);
static void Exit(int sts);
static int ConsoleGetC(void);
static void ConsolePutC(int ch);
static void ConsolePutS(char *str);
static int ConsoleAtBOLP(void);
static void FlushInput(void);
static int ConsoleCheck(void);
static void FlushOutput(void);

#ifdef LOG_XLISP
static void LogCommand(char *fmt,...)
{
    static FILE *fp = NULL;
    if (!fp) fp = fopen("xlisp.log","w");
    if (fp) {
        va_list ap;
        va_start(ap,fmt);
        vfprintf(fp,fmt,ap);
        fflush(fp);
        va_end(ap);
    }
}
#endif

/*
 * code to implement the "xleval" command
 */
int
xleval_cmd(ClientData ourdata,Tcl_Interp *interp,int argc,char *argv[])
{
    xlValue val;

    if (argc != 2) {
        interp->result = argc < 2 ? "too few arguments" : "too many arguments";
	interp->freeProc = 0;
	return TCL_ERROR;
    }

    /* evaluate the string */
    if (xlEvaluateString(&val,1,argv[1],strlen(argv[1])) < 0) {
	interp->result = "error evaluating string";
	interp->freeProc = 0;
	return TCL_ERROR;
    }

    /* print the result to a string */
    if ((interp->result = xlDisplayToString(val,NULL,0)) == NULL) {
	interp->result = "error printing result to string";
	interp->freeProc = 0;
	return TCL_ERROR;
    }

    /* return the result */
    interp->freeProc = xlFreeString;
    return TCL_OK;
}


/*
 * code to implement the "xlisp" command
 */
int
XlispCmd(ClientData ourdata, Tcl_Interp *interp, int argc, char *argv[])
{
    char *xlstring, *dest, *src;
    int xlisp_top, xllen, i;
    xlValue val;

    if (!strcmp(argv[0], "xlisp_top"))
	xlisp_top = 1;
    else
	xlisp_top = 0;

    if (argc < 2) {
	interp->result = "too few arguments";
	interp->freeProc = 0;
	return TCL_ERROR;
    }

    /* build an Xlisp list in a string */
    xllen = 3;
    for (i = 1; i < argc; i++) {
	xllen += strlen(argv[i]) + 1;
    }
    if ((dest = xlstring = malloc(xllen)) == NULL) {
	interp->result = "error allocating expression buffer";
	interp->freeProc = 0;
	return TCL_ERROR;
    }

    if (xlisp_top)
	xllen--;
    else
	*dest++ = '(';
    for (i = 1; i < argc; i++) {
	src = argv[i];
	while (*src) {
	    *dest++ = *src++;
	}
	*dest++ = ' ';
    }
    if (xlisp_top)
	xllen--;
    else
	*dest++ = ')';
    *dest++ = 0;

    /* evaluate the string */
    if (xlisp_top) {

        /* read the string */
        if (xlReadFromString(xlstring, xllen, &val) == xlsSuccess) {

	    /* evaluate and print the result */
	    xlCallFunctionByName(NULL,0,"EVAL-PRINT",1,val);

	    /* display the next prompt */
	    xlCallFunctionByName(NULL,0,"LISTENER-PROMPT",0);

	    /* flush output */
            xlosFlushOutput();
	}

        /* return an empty result */
        interp->result = "";
        interp->freeProc = 0;
    }
    
    else {
#ifdef LOG_XLISP
        LogCommand("xlisp: '%s'",xlstring);
#endif

        /* evaluate the string */
        if (xlEvaluateString(&val,1,xlstring,xllen) < 0) {
#ifdef LOG_XLISP
            LogCommand(" (xlisp) -> (evaluate failed)\n");
#endif
	    interp->result = "error evaluating string";
	    interp->freeProc = 0;
            free(xlstring);
	    return TCL_ERROR;
        }

        /* print the result to a string */
        if ((interp->result = xlDisplayToString(val,NULL,0)) == NULL) {
#ifdef LOG_XLISP
            LogCommand(" (xlisp) -> (print failed)\n");
#endif
	    interp->result = "error printing result to string";
	    interp->freeProc = 0;
            free(xlstring);
	    return TCL_ERROR;
        }

#ifdef LOG_XLISP
        LogCommand(" (xlisp) -> '%s'\n",interp->result);
#endif
        /* return the result */
        interp->freeProc = xlFreeString;
    }
    free(xlstring);

    return TCL_OK;
}

/*
 * code to implement the "xlscroll" command
 */
int
xlscroll_cmd(ClientData ourdata, Tcl_Interp *interp, int argc, char *argv[])
{
    char *xlstring, *dest, *src;
    int i, xllen;
    xlValue val;
    int xlisp_top;

    if (!strcmp(argv[0], "xlisp_top"))
	xlisp_top = 1;
    else
	xlisp_top = 0;

    if (argc < 2) {
	interp->result = "too few arguments";
	interp->freeProc = 0;
	return TCL_ERROR;
    }

    /* build an Xlisp list in a string */
    xllen = 3;
    for (i = 1; i < argc; i++) {
	xllen += strlen(argv[i]) + 1;
        if (i > 1 && isalpha(argv[i][0]))
            ++xllen;
    }
    dest = xlstring = malloc(xllen);

    if (xlisp_top)
	xllen--;
    else
	*dest++ = '(';
    for (i = 1; i < argc; i++) {
	src = argv[i];
        if (i > 1 && isalpha(*src))
            *dest++ = '\'';
	while (*src) {
	    *dest++ = *src++;
	}
	*dest++ = ' ';
    }
    if (xlisp_top)
	xllen--;
    else
	*dest++ = ')';
    *dest++ = 0;

    /* evaluate the string */

#ifdef LOG_XLISP
        LogCommand("xlscroll: '%s'\n",xlstring);
#endif
    if (xlReadFromString(xlstring, xllen, &val) == xlsSuccess) {
	if (xlisp_top) {
	    /* evaluate and print the result */
	    xlCallFunctionByName(NULL,0,"EVAL-PRINT",1,val);

	    /* display the next prompt */
	    xlCallFunctionByName(NULL,0,"LISTENER-PROMPT",0);
	} else {
	    xlCallFunctionByName(NULL,0,"EVAL",1,val);
	}
	/* flush terminal output */
	xlosFlushOutput();
    }
    free(xlstring);

    interp->result = "";
    interp->freeProc = 0;
    return TCL_OK;
}

/*
 * routine called when the "xlisp" command is deleted;
 * can shut down xlisp cleanly
 */
void
DeleteXlispCmd(ClientData ourdata)
{
#if 0
    fprintf(stderr, "deleting xlisp\n");
#endif
}

/*
 * xlisp command line arguments
 */
int xl_argc;
char **xl_argv;

/*
 * tcl command line arguments (faked out)
 */

char *tcl_argv[] = {
    "XlispTk",
    (char *)0
};

/* InvokeXLISP - invoke the xlisp interpreter */
static int InvokeXLISP(Tcl_Interp *interp,char *softwareType)
{
    xlCallbacks *callbacks;
    char *str;
    
    /* get the default callback structure */
    callbacks = xlDefaultCallbacks(NULL);
    SetupCallbacks(callbacks);

    /* initialize xlisp */
    if (!xlInit(callbacks,xl_argc,xl_argv,NULL)) {
	fprintf(stderr, "Error initializing xlisp\n");
	interp->result = "Unable to initialize xlisp!";
	interp->freeProc = 0;
	return FALSE;
    }

    /* set our platform type */
    xlSetSoftwareType(softwareType);

    /* add all of our functions */
    ApplicationEnter();
    tkenter();

    /* display the banner */
    if ((str = ApplicationBanner()) != NULL)
        xlInfo("%s\n",str);
    xlInfo("%s",xlBanner());
    
    /* setup the tcl path variable */
    SetupTclPath(interp);

    /* load the lisp code */
    if ((str = ApplicationInitializationFile()) != NULL)
        xlLoadFile(str);
    else
        xlLoadFile("xlisp.ini");
    
    /* display the initial prompt */
    xlCallFunctionByName(NULL,0,"LISTENER-PROMPT",0);

    /* flush terminal output */
    xlosFlushOutput();

    /* return successfully */
    return TRUE;
}

#define PathSetCmdHead "set xlispPath [list"
#define PathSetCmdTail "]"

/* SetupTclPath - setup the tcl path variable */
static int SetupTclPath(Tcl_Interp *interp)
{
    xlFIXTYPE len = strlen(PathSetCmdHead) + strlen(PathSetCmdTail);
    xlValue path,next;
    char *cmd,*p;

    /* get the xlisp path */
    path = xlGetValue(xlEnter("*LOAD-PATH*"));

    /* find the length of the command string */
    for (next = path; xlConsP(next); next = xlCdr(next)) {
	xlValue pathEntry = xlCar(next);
        if (xlStringP(pathEntry))
            len += strlen(xlGetString(pathEntry)) + 3; /* <space>{<string>} */
    }

    /* allocate the command string */
    if (!(cmd = p = malloc(len + 1)))
	return FALSE;
    strcpy(p,PathSetCmdHead);
    p += strlen(p);
    
    /* build the command string */
    for (next = path; xlConsP(next); next = xlCdr(next)) {
	xlValue pathEntry = xlCar(next);
        if (xlStringP(pathEntry)) {
            *p++ = ' ';
            *p++ = '{';
            TclPathCopy(p,xlGetString(pathEntry));
            p += strlen(p);
            *p++ = '}';
        }
    }
    strcpy(p,PathSetCmdTail);

    /* set the xlispPath variable */
    if (Tcl_Eval(tcl_interp,cmd) != TCL_OK) {
	free(cmd);
	return FALSE;
    }

    /* free the buffer and return successfully */
    free(cmd);
    return TRUE;
}

/* TclPathCopy = copy a string replacing backslash with slash */
void TclPathCopy(char *dst,char *src)
{
    int ch;
    while ((ch = *src++) != '\0') {
        if (ch == '\\')
            ch = '/';
        *dst++ = ch;
    }
    *dst = '\0';
}

#ifdef WIN32

EXTERN void		TkConsoleCreate(void);
EXTERN int		TkConsoleInit(Tcl_Interp *interp);

static void		setargv _ANSI_ARGS_((int *argcPtr, char ***argvPtr));
static void		WishPanic _ANSI_ARGS_(TCL_VARARGS(char *,format));

int APIENTRY
WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int nCmdShow)
{
    char **argv, *p;
    int argc;
    char buffer[MAX_PATH];

    Tcl_SetPanicProc(WishPanic);

    /*
     * Set up the default locale to be standard "C" locale so parsing
     * is performed correctly.
     */

    setlocale(LC_ALL, "C");


    /*
     * Increase the application queue size from default value of 8.
     * At the default value, cross application SendMessage of WM_KILLFOCUS
     * will fail because the handler will not be able to do a PostMessage!
     * This is only needed for Windows 3.x, since NT dynamically expands
     * the queue.
     */
    SetMessageQueue(64);

    /*
     * Create the console channels and install them as the standard
     * channels.  All I/O will be discarded until TkConsoleInit is
     * called to attach the console to a text widget.
     */

#ifdef CONSOLE
    TkConsoleCreate();
#endif

    setargv(&argc, &argv);

    xl_argc = argc;
    xl_argv = argv;

    /*
     * Replace argv[0] with full pathname of executable, and forward
     * slashes substituted for backslashes.
     */

    GetModuleFileName(NULL, buffer, sizeof(buffer));
    argv[0] = buffer;
    for (p = buffer; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }

    Tk_Main(1, argv, Tcl_AppInit);
    return 1;
}

#define CLASS_NAME  "TkTopLevel"
#define WINDOW_NAME "Puffin2k - MMP"

int
Tcl_AppInit(interp)
    Tcl_Interp *interp;		/* Interpreter for application. */
{
    HWND puffinWin;

    /* don't allow multiple instances of Puffin */
    if ((puffinWin = FindWindow(CLASS_NAME,WINDOW_NAME)) != NULL) {
        SetForegroundWindow(puffinWin);
        ExitProcess(1);
    }

    if (Tcl_Init(interp) == TCL_ERROR)
	goto error;

    if (Tk_Init(interp) == TCL_ERROR)
	goto error;

    Tcl_StaticPackage(interp, "Tk", Tk_Init, Tk_SafeInit);

    /*
     * Initialize the console only if we are running as an interactive
     * application.
     */

#ifdef CONSOLE
	TkConsoleCreate();
/*    if (TkConsoleInit(interp) == TCL_ERROR) */
/*	goto error; */
#endif

    tcl_interp = interp;

    Tcl_CreateCommand(interp, "xlisp", XlispCmd, 0L, DeleteXlispCmd);
    Tcl_CreateCommand(interp, "xlisp_top", XlispCmd, 0L, DeleteXlispCmd);
    Tcl_CreateCommand(interp, "xleval", xleval_cmd, NULL, (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "xlscroll", xlscroll_cmd, NULL, (Tcl_CmdDeleteProc *)NULL);
#ifdef USE_MDI
    Tcl_CreateCommand(interp, "setparent", setparent_cmd, 0L,
    	(Tcl_CmdDeleteProc *)NULL);
#endif
    tcl_init_ok = 1;

    /* invoke xlisp */
    return InvokeXLISP(interp,"WIN95") ? TCL_OK : TCL_ERROR;

error:
    WishPanic(interp->result);
    return TCL_ERROR;
}

void
WishPanic TCL_VARARGS_DEF(char *,arg1)
{
    va_list argList;
    char buf[1024];
    char *format;
    
    format = TCL_VARARGS_START(char *,arg1,argList);
    vsprintf(buf, format, argList);

    MessageBeep(MB_ICONEXCLAMATION);
    MessageBox(NULL, buf, "Fatal Error in Puffin2k",
	    MB_ICONSTOP | MB_OK | MB_TASKMODAL | MB_SETFOREGROUND);
#ifdef _MSC_VER
    _asm {
        int 3
    }
#endif
    ExitProcess(1);
}

static void
setargv(argcPtr, argvPtr)
    int *argcPtr;		/* Filled with number of argument strings. */
    char ***argvPtr;		/* Filled with argument strings (malloc'd). */
{
    char *cmdLine, *p, *arg, *argSpace;
    char **argv;
    int argc, size, inquote, copy, slashes;
    
    cmdLine = GetCommandLine();

    /*
     * Precompute an overly pessimistic guess at the number of arguments
     * in the command line by counting non-space spans.
     */

    size = 2;
    for (p = cmdLine; *p != '\0'; p++) {
	if (isspace(*p)) {
	    size++;
	    while (isspace(*p)) {
		p++;
	    }
	    if (*p == '\0') {
		break;
	    }
	}
    }
    argSpace = (char *) ckalloc((unsigned) (size * sizeof(char *) 
	    + strlen(cmdLine) + 1));
    argv = (char **) argSpace;
    argSpace += size * sizeof(char *);
    size--;

    p = cmdLine;
    for (argc = 0; argc < size; argc++) {
	argv[argc] = arg = argSpace;
	while (isspace(*p)) {
	    p++;
	}
	if (*p == '\0') {
	    break;
	}

	inquote = 0;
	slashes = 0;
	while (1) {
	    copy = 1;
	    while (*p == '\\') {
		slashes++;
		p++;
	    }
	    if (*p == '"') {
		if ((slashes & 1) == 0) {
		    copy = 0;
		    if ((inquote) && (p[1] == '"')) {
			p++;
			copy = 1;
		    } else {
			inquote = !inquote;
		    }
                }
                slashes >>= 1;
            }

            while (slashes) {
		*arg = '\\';
		arg++;
		slashes--;
	    }

	    if ((*p == '\0') || (!inquote && isspace(*p))) {
		break;
	    }
	    if (copy != 0) {
		*arg = *p;
		arg++;
	    }
	    p++;
        }
	*arg = '\0';
	argSpace = arg + 1;
    }
    argv[argc] = NULL;

    *argcPtr = argc;
    *argvPtr = argv;
}

#else

/*
 * main routine called by Tcl's startup code
 */

int
xlispTkInit(Tcl_Interp *interp)
{
    if (Tcl_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }
    if (Tk_Init(interp) == TCL_ERROR) {
        return TCL_ERROR;
    }

    tcl_interp = interp;

    Tcl_CreateCommand(interp, "xlisp", XlispCmd, 0L, DeleteXlispCmd);
    Tcl_CreateCommand(interp, "xlisp_top", XlispCmd, 0L, DeleteXlispCmd);
    Tcl_CreateCommand(interp, "xleval", xleval_cmd, NULL, (Tcl_CmdDeleteProc *)NULL);
    Tcl_CreateCommand(interp, "xlscroll", xlscroll_cmd, NULL, (Tcl_CmdDeleteProc *)NULL);

    tcl_init_ok = 1;

    /* invoke xlisp */
    return InvokeXLISP(interp,"UNIX") ? TCL_OK : TCL_ERROR;
}

int
main(int argc, char **argv)
{
    xl_argc = argc;
    xl_argv = argv;


	/* Tell tcl where to find the executable and init.tcl */
    Tcl_FindExecutable (argv[0]);

    Tk_Main(1, tcl_argv, xlispTkInit);

    return 0;
}

#endif

/* FindSubr - find a subr or xsubr address */
static xlValue (*FindSubr(char *name))(void)
{
    xlValue (*subr)(void);
    if ((subr = tkfindsubr(name)) != NULL)
        return subr;
    return ApplicationFindSubr(name);
}

/* LoadPath - return the path xlisp uses for loading files */
static char *LoadPath(void)
{
    char *path;
    if ((path = ApplicationLoadPath()) == NULL)
        path = getenv("XLPATH");
    return path;
}

/* ParsePath - parse a path string */
static char *ParsePath(char **pp)
{
    static char buf[256];
    char *src,*dst;
    int ch;
    
    /* find the next directory in the path */
    for (src = *pp, dst = buf; *src != '\0'; )
#ifdef WIN32
	if ((ch = *src++) == ';')
#else
	if ((ch = *src++) == ':')
#endif
	    break;
        else if (ch == '\\')
            *dst++ = '/';
        else
            *dst++ = ch;
    *dst = '\0';

    /* make sure the directory ends with a slash */
    if (dst > buf && dst[-1] != '/')
	strcat(dst,"/");
    
    /* return this directory and position in the path */
    *pp = src;
    return dst == buf ? NULL : buf;
}

/* DirectorySeparator - return the directory separator character */
static int DirectorySeparator(void)
{
    return '/';
}

/* tkenter - enter tk specific symbols and functions */
void tkenter(void)
{
    xlSubrDef *sdp;
    xlXSubrDef *xsdp;

    /* enter the built-in functions */
    for (sdp = tksubrtab; sdp->name != NULL; ++sdp)
        xlSubr(sdp->name,sdp->subr);
    for (xsdp = tkxsubrtab; xsdp->name != NULL; ++xsdp)
        xlXSubr(xsdp->name,xsdp->subr);

    /* enter foreign pointer types */
    s_tkobj = xlEnter("TK-OBJECT");
}

/* tkfindsubr - find an os specific function */
xlValue (*tkfindsubr(char *name))(void)
{
    xlSubrDef *sdp;
    xlXSubrDef *xsdp;

    /* find the built-in function */
    for (sdp = tksubrtab; sdp->name != NULL; ++sdp)
        if (strcmp(sdp->name,name) == 0)
	    return sdp->subr;
    for (xsdp = tkxsubrtab; xsdp->name != NULL; ++xsdp)
        if (strcmp(xsdp->name,name) == 0)
	    return (xlValue (*)(void))xsdp->subr;
    return NULL;
}

/* Error - print an error message */
static void Error(char *msg)
{
    ConsolePutS("error: ");
    ConsolePutS(msg);
    xlosConsolePutC('\n');
}

/*
 * I/O functions
 */
/* terminal output buffer */
#define OUTBUFSIZE 1024
static char outbuf[OUTBUFSIZE + 1];
static char *outptr = outbuf;

/* Exit - exit from XLISP */
static void Exit(int sts)
{
    Tcl_Eval(tcl_interp,"exit");
}

/* ConsoleGetC - get a character from the terminal */
static int ConsoleGetC(void)
{
    return EOF;
}

/* ConsolePutC - put a character to the terminal */
static void ConsolePutC(int ch)
{
    if (outptr >= &outbuf[OUTBUFSIZE])
        xlosFlushOutput();
    *outptr++ = ch;
}

/* ConsolePutS - output a string to the terminal */
static void ConsolePutS(char *str)
{
    while (*str != '\0')
	xlosConsolePutC(*str++);
}

/* ConsoleAtBOLP - are we at the beginning of a line? */
static int ConsoleAtBOLP(void)
{
    return FALSE;
}

/* FlushInput - flush the terminal input buffer */
static void FlushInput(void)
{
    xlosFlushOutput();
}

/* ConsoleCheck - check for control characters during execution */
static int ConsoleCheck(void)
{
    return 0;
}

/* FlushOutput - flush the terminal output buffer */
static void FlushOutput(void)
{
    char tclcmd[256];

    if (outptr != outbuf) {
        *outptr = '\0';

	if (tcl_init_ok) {
	    Tcl_SetVar(tcl_interp, "__lisp_output", outbuf, 0);
	    outptr = outbuf;
	    strcpy(tclcmd, ".xLispTk.lframe.listener insert insert $__lisp_output Output");
	    Tcl_Eval(tcl_interp, tclcmd);
	    strcpy(tclcmd, ".xLispTk.lframe.listener see insert");
	    Tcl_Eval(tcl_interp,tclcmd);
	} else {
	    fputs(outbuf, stderr);
	    outptr = outbuf;
	}
    }
}

/* xsystem - execute a system command */
static xlValue xsystem(void)
{
#ifdef WIN32
    char *cmd = "COMMAND";
#else
    char *cmd = "/bin/sh";
#endif
    if (xlMoreArgsP())
	cmd = xlGetString(xlGetArgString());
    xlLastArg();
    return xlMakeFixnum((xlFIXTYPE)system(cmd));
}

static int tcl_verbose_flag = 0;
static FILE *tclFile = NULL;

/* LogTclCommand - log a tcl command */
static void LogTclCommand(char *tag,char *cmd)
{
    if (tcl_verbose_flag) {
        if (!tclFile)
            tclFile = fopen("tcl.log","w");
        if (tclFile) {
            fprintf(tclFile,"%s%s\n",tag,cmd);
            fflush(tclFile);
        }
    }
}

/*
 * run a TCL command
 */

static xlValue
xtcl(void)
{
    xlValue *savesp, arg;
    xlFIXTYPE len;
    int saveargc;

    char *tclcmd, *str;

    /* save the argument list */
    saveargc = xlArgC;
    savesp = xlSP;

    /* find the length of the new string */
    for (len = 0; xlMoreArgsP(); ) {
	arg = xlGetArgString();
        len += xlGetSLength(arg);
    }

    /* restore the argument list */
    xlArgC = saveargc;
    xlSP = savesp;
    
    /* create the new string */
    tclcmd = malloc(len + 1);
    if (!tclcmd) {
	return xlNil;
    }

    *tclcmd = 0;
    str = tclcmd;

    while (xlMoreArgsP()) {
	arg = xlNextArg();
	len = xlGetSLength(arg);
	memcpy(str,xlGetString(arg),(size_t)len);
	str += len;
    }
    *str++ = 0;

#ifdef LOG_TCL
    LogCommand("tcl: '%s'",tclcmd);
#endif
    LogTclCommand("",tclcmd);
    if (Tcl_Eval(tcl_interp, tclcmd) != TCL_OK) {
#ifdef LOG_TCL
	LogCommand(" (tcl) -> (failed)\n");
#endif
        if (tcl_verbose_flag)
	    fprintf(stderr, "FAILED TCL COMMAND: %s\n", tclcmd);
	free(tclcmd);
	LogTclCommand("(error) ",tcl_interp->result);
        xlFmtError("Tcl error: ~A", xlMakeCString(tcl_interp->result));
	return xlNil;
    }
    free(tclcmd);
#ifdef LOG_TCL
    LogCommand(" (tcl) -> '%s'\n",tcl_interp->result);
#endif
    if (tcl_interp->result[0])
        LogTclCommand("--> ",tcl_interp->result);
    return xlMakeCString(tcl_interp->result);
}


/*
 * set flag to indicate whether failing tcl commands will be
 * printed or not
 */

static xlValue
xtclverbose(void)
{
    xlValue arg;
    xlValue ret;

    arg = xlGetArg();
    xlLastArg();

    ret = (tcl_verbose_flag) ? xlTrue : xlFalse;
    if (arg == xlFalse)
	tcl_verbose_flag = 0;
    else
	tcl_verbose_flag = 1;

    return ret;
}

/* SetupCallbacks - setup the o/s interface callbacks */
void SetupCallbacks(xlCallbacks *callbacks)
{
    callbacks->findSubr = FindSubr;
    callbacks->loadPath = LoadPath;
    callbacks->parsePath = ParsePath;
    callbacks->directorySeparator = DirectorySeparator;
    callbacks->exit = Exit;
    callbacks->error = Error;
    callbacks->consoleGetC = ConsoleGetC;
    callbacks->consolePutC = ConsolePutC;
    callbacks->consoleAtBOLP = ConsoleAtBOLP;
    callbacks->consoleFlushInput = FlushInput;
    callbacks->consoleFlushOutput = FlushOutput;
    callbacks->consoleCheck = ConsoleCheck;
}

#ifdef USE_MDI
/*
 * code to implement the new "setparent" command
 */
int
setparent_cmd(ClientData ourdata,Tcl_Interp *interp,int argc,char *argv[])
{
    Tk_Window *childPtr, *parentPtr;
    Tk_Window tkmainwin;
    HWND wrapper;
    
    if (argc != 2 && argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " child ?parent?\"", (char *)0);
	return TCL_ERROR;
    }
    tkmainwin = Tk_MainWindow(interp);
    
    childPtr = (Tk_Window *)Tk_NameToWindow(interp, argv[1], tkmainwin);
    if (!childPtr) {
        Tcl_AppendResult(interp, "unknown window: ", argv[1], (char *)0);
	return TCL_ERROR;
    }
    if (argc == 3) {
        parentPtr = (Tk_Window *)Tk_NameToWindow(interp, argv[2], tkmainwin);
	if (!parentPtr) {
	    Tcl_AppendResult(interp, "unknown window: ", argv[2], (char *)0);
	    return TCL_ERROR;
	}
    } else {
        parentPtr = NULL;
    }

    wrapper = ((HWND *)((Tk_FakeWin *)childPtr)->dummy16)[1];
    if (parentPtr == NULL ||
	    Tk_WindowId(parentPtr) == None) {
	    SetParent(wrapper, NULL);
    } else {
            SetParent(wrapper,
		Tk_GetHWND(Tk_WindowId(parentPtr)));
    }
    return TCL_OK;
}
#endif
