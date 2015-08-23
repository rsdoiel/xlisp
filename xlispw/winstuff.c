/* winstuff.c - windows 95 interface routines for xlisp 3.0 */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "xlisp.h"
#include "xlispw.h"
#include "winstuff.h"
#include "widget.h"

static xlSubrDef subrtab[] = {
{0,0}};

static xlXSubrDef xsubrtab[] = {
{0,0}};

/* prototypes */
static xlValue (*FindSubr(char *name))(void);
static void Error(char *msg);
static void Exit(int sts);

/* InvokeXLISP - invoke the xlisp interpreter */
int InvokeXLISP(xlCallbacks *callbacks,int argc,char *argv[])
{
    xlSubrDef *sdp;
    xlXSubrDef *xsdp;
    
    /* initialize xlisp */
    if (!xlInit(callbacks,argc,argv,NULL)) {
        Error("initializing xlisp");
        return FALSE;
    }

    /* set our platform type */
    xlSetSoftwareType("WIN95");

    /* add all of our functions */
    for (sdp = subrtab; sdp->name != NULL; ++sdp)
        xlSubr(sdp->name,sdp->subr);
    for (xsdp = xsubrtab; xsdp->name != NULL; ++xsdp)
        xlXSubr(xsdp->name,xsdp->subr);

    /* initialize the widget classes */
    InitWidgets();

    /* initialize the window classes */
    InitWindows();

    /* initialize the canvas class */
    InitCanvas();
    
    /* initialize the menu classes */
    InitMenus();

    /* initialize the edit classes */
    InitEdit();

    /* initialize the toolbar routines */
    InitToolbars();

    /* initialize the accelerator routines */
    InitAccelerators();

    /* initialize the listener routines */
    InitListener();

    /* initialize the miscellaneous routines */
    InitMisc();

    /* setup the *command-line* variable */
    xlVal = xlMakeCString((char *)GetCommandLine());
    xlSetValue(xlEnter("*COMMAND-LINE*"),xlVal);

    /* load the lisp code */
    xlLoadFile("xlispw.lsp");
    
    /* display the banner */
    xlInfo("%s",xlBanner());
    
    /* display the initial prompt */
    xlCallFunctionByName(NULL,0,"LISTENER-PROMPT",0);

    /* flush terminal output */
    xlosFlushOutput();

    /* return successfully */
    return TRUE;
}

/* FindSubr - find a subr or xsubr address */
static xlValue (*FindSubr(char *name))(void)
{
    xlSubrDef *sdp;
    xlXSubrDef *xsdp;

    /* find the built-in function */
    for (sdp = subrtab; sdp->name != NULL; ++sdp)
        if (strcmp(sdp->name,name) == 0)
            return sdp->subr;
    for (xsdp = xsubrtab; xsdp->name != NULL; ++xsdp)
        if (strcmp(xsdp->name,name) == 0)
            return (xlValue (*)(void))xsdp->subr;
    return NULL;
}

/* Error - print an error message */
static void Error(char *msg)
{
    xlosConsolePutS("\nerror: ");
    xlosConsolePutS(msg);
}

/* Exit - exit from XLISP */
static void Exit(int sts)
{
    PostQuitMessage(0);
}

/* SetupCallbacks - setup the o/s interface callbacks */
void SetupCallbacks(xlCallbacks *callbacks)
{
    callbacks->findSubr = FindSubr;
    callbacks->exit = Exit;
    callbacks->error = Error;
}
