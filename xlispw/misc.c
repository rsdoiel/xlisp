/* misc.c - miscellaneous routines */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <windows.h>
#include "xlisp.h"
#include "xlispw.h"
#include "widget.h"

/* symbols */
static xlValue k_buttons;
static xlValue k_icon;
static xlValue k_default;
static xlValue s_abortretryignore;
static xlValue s_ok;
static xlValue s_okcancel;
static xlValue s_retrycancel;
static xlValue s_yesno;
static xlValue s_yesnocancel;
static xlValue s_abort;
static xlValue s_cancel;
static xlValue s_ignore;
static xlValue s_no;
static xlValue s_retry;
static xlValue s_yes;
static xlValue s_warning;
static xlValue s_information;
static xlValue s_question;
static xlValue s_error;

/* function handlers */
static void xMessageBox(void);
static xlValue xGetFullPathName(void);

/* InitMisc - initialize the miscellaneous routines */
void InitMisc(void)
{
    /* enter the built-in symbols */
    k_buttons           = xlEnterKeyword("BUTTONS");
    k_default           = xlEnterKeyword("DEFAULT");
    k_icon              = xlEnterKeyword("ICON");
    s_abortretryignore  = xlEnter("ABORT-RETRY-IGNORE");
    s_ok                = xlEnter("OK");
    s_okcancel          = xlEnter("OK-CANCEL");
    s_retrycancel       = xlEnter("RETRY-CANCEL");
    s_yesno             = xlEnter("YES-NO");
    s_yesnocancel       = xlEnter("YES-NO-CANCEL");
    s_abort             = xlEnter("ABORT");
    s_cancel            = xlEnter("CANCEL");
    s_ignore            = xlEnter("IGNORE");
    s_no                = xlEnter("NO");
    s_retry             = xlEnter("RETRY");
    s_yes               = xlEnter("YES");
    s_warning           = xlEnter("WARNING");
    s_information       = xlEnter("INFORMATION");
    s_question          = xlEnter("QUESTION");
    s_error             = xlEnter("ERROR");

    /* enter the built-in functions */
    xlXSubr("WIN:MESSAGE-BOX",xMessageBox);
    xlSubr("WIN:GET-FULL-PATH-NAME",xGetFullPathName);
}

/* xMessageBox - built-in function 'message-box' */
static void xMessageBox(void)
{
    xlValue text,title,buttons,icon,def;
    int result;
    UINT type;

    /* parse the arguments */
    text = xlGetArgString();
    title = xlGetArgString();
    xlGetKeyArg(k_buttons,s_ok,&buttons);
    xlGetKeyArg(k_icon,xlNil,&icon);
    xlGetKeyArg(k_default,xlNil,&def);

    /* setup the buttons */
    if (buttons == s_abortretryignore)
        type = MB_ABORTRETRYIGNORE;
    else if (buttons == s_ok)
        type = MB_OK;
    else if (buttons == s_okcancel)
        type = MB_OKCANCEL;
    else if (buttons == s_retrycancel)
        type = MB_RETRYCANCEL;
    else if (buttons == s_yesno)
        type = MB_YESNO;
    else if (buttons == s_yesnocancel)
        type = MB_YESNOCANCEL;
    else
        xlError("expecting a button specifier",buttons);

    /* setup the default button */
    if (def == xlNil)
        type |= MB_DEFBUTTON1;
    else if (def == s_abort) {
        switch (type) {
        case MB_ABORTRETRYIGNORE:
            type |= MB_DEFBUTTON1;
            break;
        case MB_OK:
        case MB_OKCANCEL:
        case MB_RETRYCANCEL:
        case MB_YESNO:
        case MB_YESNOCANCEL:
            xlError("no such button",def);
        }
    }
    else if (def == s_cancel) {
        switch (type) {
        case MB_OKCANCEL:
        case MB_RETRYCANCEL:
            type |= MB_DEFBUTTON2;
            break;
        case MB_YESNOCANCEL:
            type |= MB_DEFBUTTON3;
            break;
        case MB_ABORTRETRYIGNORE:
        case MB_OK:
        case MB_YESNO:
            xlError("no such button",def);
        }
    }
    else if (def == s_ignore) {
        switch (type) {
        case MB_ABORTRETRYIGNORE:
            type |= MB_DEFBUTTON3;
            break;
        case MB_OK:
        case MB_OKCANCEL:
        case MB_RETRYCANCEL:
        case MB_YESNO:
        case MB_YESNOCANCEL:
            xlError("no such button",def);
        }
    }
    else if (def == s_no) {
        switch (type) {
        case MB_YESNO:
        case MB_YESNOCANCEL:
            type |= MB_DEFBUTTON2;
            break;
        case MB_ABORTRETRYIGNORE:
        case MB_OK:
        case MB_OKCANCEL:
        case MB_RETRYCANCEL:
            xlError("no such button",def);
        }
    }
    else if (def == s_ok) {
        switch (type) {
        case MB_OK:
        case MB_OKCANCEL:
            type |= MB_DEFBUTTON1;
            break;
        case MB_ABORTRETRYIGNORE:
        case MB_RETRYCANCEL:
        case MB_YESNO:
        case MB_YESNOCANCEL:
            xlError("no such button",def);
        }
    }
    else if (def == s_retry) {
        switch (type) {
        case MB_ABORTRETRYIGNORE:
            type |= MB_DEFBUTTON2;
            break;
        case MB_RETRYCANCEL:
            type |= MB_DEFBUTTON1;
            break;
        case MB_OK:
        case MB_OKCANCEL:
        case MB_YESNO:
        case MB_YESNOCANCEL:
            xlError("no such button",def);
        }
    }
    else if (def == s_yes) {
        switch (type) {
        case MB_YESNO:
        case MB_YESNOCANCEL:
            type |= MB_DEFBUTTON1;
            break;
        case MB_ABORTRETRYIGNORE:
        case MB_OK:
        case MB_OKCANCEL:
        case MB_RETRYCANCEL:
            xlError("no such button",def);
        }
    }
    else
        xlError("expecting a default button specifier",def);

    /* setup the icon */
    if (icon == xlNil)
        ;
    else if (icon == s_warning)
        type |= MB_ICONWARNING;
    else if (icon == s_information)
        type |= MB_ICONINFORMATION;
    else if (icon == s_question)
        type |= MB_ICONQUESTION;
    else if (icon == s_error)
        type |= MB_ICONERROR;
    else
        xlError("expecting an icon specifier",icon);

    /* make it modal */
    type |= MB_TASKMODAL;

    /* display the message box */
    result = MessageBox(NULL,
                        xlGetString(text),
                        xlGetString(title),
                        type);

    /* determine the result */
    switch (result) {
    case IDABORT:
        result = FALSE;
        xlVal = s_abort;
        break;
    case IDCANCEL:
        result = FALSE;
        xlVal = s_cancel;
        break;
    case IDIGNORE:
        result = FALSE;
        xlVal = s_ignore;
        break;
    case IDNO:
        result = FALSE;
        xlVal = s_no;
        break;
    case IDOK:
        result = TRUE;
        xlVal = s_ok;
        break;
    case IDRETRY:
        result = TRUE;
        xlVal = s_retry;
        break;
    case IDYES:
        result = TRUE;
        xlVal = s_yes;
        break;
    }

    /* return the results */
    xlCheck(2);
    xlPush(xlVal);
    xlPush(result ? xlTrue : xlFalse);
    xlMVReturn(2);
}

/* xGetFullPathName - built-in function 'get-full-path-name' */
static xlValue xGetFullPathName(void)
{
    char buf[256],*namePtr;
    xlValue str;
    DWORD cnt;
    
    /* parse the argument list */
    xlVal = xlGetArgString();
    xlLastArg();

    /* get the full path name */
    cnt = GetFullPathName(xlGetString(xlVal),(DWORD)sizeof(buf),buf,&namePtr);
    if (cnt == 0)
        return xlNil;
    else if (cnt < sizeof(buf))
        return xlMakeCString(buf);

    /* allocate a string big enough for the result */
    str = xlNewString(cnt);
    cnt = GetFullPathName(xlGetString(xlVal),cnt + 1,xlGetString(str),&namePtr);
    if (cnt == 0)
        return xlNil;
    return str;
}