/* edit.c - edit classes */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <windows.h>
#include <commctrl.h>
#include <richedit.h>
#include "xlisp.h"
#include "xlispw.h"
#include "widget.h"

/* globals */
xlValue c_basicEditText;

/* externals */
extern xlValue c_widget;

/* classes */
static xlValue c_editText;
static xlValue c_richEditText;

/* keywords and symbols */
static xlValue k_label;
static xlValue k_hscrollp;
static xlValue k_vscrollp;
static xlValue k_autohscrollp;
static xlValue k_autovscrollp;
static xlValue k_multilinep;
static xlValue k_justify;
static xlValue k_borderp;
static xlValue s_left;
static xlValue s_right;
static xlValue s_center;

/* function handlers */
static void xGetSelection(void);
static xlValue xSetSelection(void);
static xlValue xReplaceSelection(void);
static xlValue xClearSelection(void);
static xlValue xLineIndex(void);
static xlValue xLineFromChar(void);
static xlValue xGetLineCount(void);
static xlValue xGetModify(void);
static xlValue xSetModify(void);
static xlValue xOnChar(void);
static xlValue xMakeEditText(void);
static xlValue xMakeRichEditText(void);
static xlValue xMakeEditWindow(void);
static xlValue xLoadFile(void);
static xlValue xRichLoadFile(void);
static xlValue xSaveFile(void);
static xlValue xRichSaveFile(void);
static xlValue xGetSelectedText(void);
static xlValue xRichGetSelectedText(void);

/* prototypes */
static xlValue MakeEditText(xlValue obj,int type);
static int LoadFile(Widget *widget,char *name);
static DWORD CALLBACK RichLoader(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb);
static int RichLoadFile(Widget *widget,char *name);
static DWORD CALLBACK RichSaver(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb);
static int SaveFile(Widget *widget,char *name);
static int RichSaveFile(Widget *widget,char *name);
LRESULT CALLBACK TextFilter(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam);

/* InitEdit - initialize the edit classes */
void InitEdit(void)
{
    /* enter the built-in symbols */
    k_label             = xlEnterKeyword("LABEL");
    k_hscrollp          = xlEnterKeyword("HSCROLL?");
    k_vscrollp          = xlEnterKeyword("VSCROLL?");
    k_autohscrollp      = xlEnterKeyword("AUTO-HSCROLL?");
    k_autovscrollp      = xlEnterKeyword("AUTO-VSCROLL?");
    k_multilinep        = xlEnterKeyword("MULTILINE?");
    k_justify           = xlEnterKeyword("JUSTIFY");
    k_borderp           = xlEnterKeyword("BORDER?");
    s_left              = xlEnter("LEFT");
    s_right             = xlEnter("RIGHT");
    s_center            = xlEnter("CENTER");

    /* 'basic-edit-text' class */
    c_basicEditText = xlClass("BASIC-EDIT-TEXT",c_widget,"");
    xlXMethod(c_basicEditText,"GET-SELECTION",xGetSelection);
    xlMethod(c_basicEditText,"SET-SELECTION!",xSetSelection);
    xlMethod(c_basicEditText,"REPLACE-SELECTION!",xReplaceSelection);
    xlMethod(c_basicEditText,"CLEAR-SELECTION!",xClearSelection);
    xlMethod(c_basicEditText,"LINE-INDEX",xLineIndex);
    xlMethod(c_basicEditText,"LINE-FROM-CHAR",xLineFromChar);
    xlMethod(c_basicEditText,"GET-LINE-COUNT",xGetLineCount);
    xlMethod(c_basicEditText,"GET-MODIFY",xGetModify);
    xlMethod(c_basicEditText,"SET-MODIFY!",xSetModify);
    xlMethod(c_basicEditText,"ON-CHAR",xOnChar);

    /* 'edit-text' class */
    c_editText = xlClass("EDIT-TEXT",c_basicEditText,"");
    xlMethod(c_editText,"INITIALIZE",xMakeEditText);
    xlMethod(c_editText,"LOAD-FILE!",xLoadFile);
    xlMethod(c_editText,"SAVE-FILE",xSaveFile);
    xlMethod(c_editText,"GET-SELECTED-TEXT",xGetSelectedText);

    /* 'rich-edit-text' class */
    c_richEditText = xlClass("RICH-EDIT-TEXT",c_basicEditText,"");
    xlMethod(c_richEditText,"INITIALIZE",xMakeRichEditText);
    xlMethod(c_richEditText,"LOAD-FILE!",xRichLoadFile);
    xlMethod(c_richEditText,"SAVE-FILE",xRichSaveFile);
    xlMethod(c_richEditText,"GET-SELECTED-TEXT",xRichGetSelectedText);
}

/* xGetSelection - built-in function 'get-selection' */
static void xGetSelection(void)
{
    DWORD start,end;
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlLastArg();

    /* get the selection */
    SendMessage(GetWidget(obj)->wnd,EM_GETSEL,(WPARAM)&start,(LPARAM)&end);
    xlCheck(2);
    xlPush(xlMakeFixnum((xlFIXTYPE)end));
    xlPush(xlMakeFixnum((xlFIXTYPE)start));
    xlMVReturn(2);
}

/* xSetSelection - built-in function 'set-selection!' */
static xlValue xSetSelection(void)
{
    DWORD start,end;
    Widget *widget;
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlVal = xlGetArgFixnum(); start = (DWORD)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); end = (DWORD)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the widget structure */
    widget = GetWidget(obj);

    /* set the selection */
    SendMessage(widget->wnd,EM_SETSEL,(WPARAM)start,(LPARAM)end);
    SendMessage(widget->wnd,EM_SCROLLCARET,0,0);
    return obj;
}

/* xReplaceSelection - built-in function 'replace-selection!' */
static xlValue xReplaceSelection(void)
{
    xlValue obj,str;
    int undoP;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    str = xlGetArgString();
    undoP = xlMoreArgsP() ? xlGetArg() != xlFalse : TRUE;
    xlLastArg();

    /* get the selection */
    SendMessage(GetWidget(obj)->wnd,EM_REPLACESEL,(WPARAM)undoP,(LPARAM)xlGetString(str));

    /* return the object */
    return obj;
}

/* xLineIndex - built-in function 'line-index' */
static xlValue xLineIndex(void)
{
    xlValue obj;
    DWORD line;
    int index;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlVal = xlGetArgFixnum(); line = xlGetFixnum(xlVal);
    xlLastArg();

    /* get the line index */
    index = SendMessage(GetWidget(obj)->wnd,EM_LINEINDEX,(WPARAM)line,0);
    return index < 0 ? xlNil : xlMakeFixnum((xlFIXTYPE)index);
}

/* xLineFromChar - built-in function 'line-from-char' */
static xlValue xLineFromChar(void)
{
    xlValue obj;
    DWORD offset;
    int index;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlVal = xlGetArgFixnum(); offset = xlGetFixnum(xlVal);
    xlLastArg();

    /* get the line index */
    index = SendMessage(GetWidget(obj)->wnd,EM_LINEFROMCHAR,(WPARAM)offset,0);
    return index < 0 ? xlNil : xlMakeFixnum((xlFIXTYPE)index);
}

/* xGetLineCount - built-in function 'get-line-count' */
static xlValue xGetLineCount(void)
{
    xlValue obj;
    int count;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlLastArg();

    /* get the line index */
    count = SendMessage(GetWidget(obj)->wnd,EM_GETLINECOUNT,0,0);
    return count < 0 ? xlNil : xlMakeFixnum((xlFIXTYPE)count);
}

/* xClearSelection - built-in function 'clear-selection!' */
static xlValue xClearSelection(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlLastArg();

    /* clear the selection and return the object */
    SendMessage(GetWidget(obj)->wnd,WM_CLEAR,0,0);
    return obj;
}


/* xGetModify - built-in function 'get-modify' */
static xlValue xGetModify(void)
{
    xlValue obj;
    int modify;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlLastArg();

    /* get the line index */
    modify = SendMessage(GetWidget(obj)->wnd,EM_GETMODIFY,0,0);
    return modify ? xlTrue : xlFalse;
}

/* xSetModify - built-in function 'set-modify!' */
static xlValue xSetModify(void)
{
    xlValue obj;
    int modify;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    modify = xlGetArg() != xlFalse;
    xlLastArg();

    /* get the line index */
    SendMessage(GetWidget(obj)->wnd,EM_SETMODIFY,(WPARAM)modify,0);
    return xlNil;
}

/* xOnChar - built-in function 'on-char' */
static xlValue xOnChar(void)
{
    CharBinding *binding;
    xlValue obj,fcn;
    Widget *widget;
    TCHAR ch;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlVal = xlGetArgChar(); ch = (TCHAR)xlGetChCode(xlVal);
    fcn = xlGetArg();
    xlLastArg();

    /* get the widget */
    widget = GetWidget(obj);

    /* look for an existing binding for this character */
    for (binding = widget->charBindings; binding; binding = binding->next)
        if (ch == binding->ch) {
            binding->fcn = fcn;
            return xlTrue;
        }

    /* allocate a binding structure */
    if ((binding = (CharBinding *)malloc(sizeof(CharBinding))) == NULL)
        return xlFalse;

    /* initialize the binding structure */
    xlProtect(&binding->fcn);
    binding->ch = ch;
    binding->fcn = fcn;
    binding->next = widget->charBindings;
    widget->charBindings = binding;

    /* return successfully */
    return xlTrue;
}

/* xMakeEditText - built-in function 'make-edit-text' */
static xlValue xMakeEditText(void)
{
    xlValue obj = xlGetArgInstance(c_editText);
    return MakeEditText(obj,wtEditText);
}

/* xMakeRichEditText - built-in function 'make-rich-edit-text' */
static xlValue xMakeRichEditText(void)
{
    xlValue obj = xlGetArgInstance(c_richEditText);
    return MakeEditText(obj,wtRichEditText);
}

/* MakeEditText - make an edit text or rich edit text control */
static xlValue MakeEditText(xlValue obj,int type)
{
    xlValue hscrollp,vscrollp,autohscrollp,autovscrollp;
    xlValue label,multilinep,justify,borderp;
    WidgetPosition pos;
    FrameWidget *parent;
    Widget *widget;
    DWORD style;
    HWND wnd;
    
    /* parse the arguments */
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_label,xlNil,&label);
    xlGetKeyArg(k_hscrollp,xlFalse,&hscrollp);
    xlGetKeyArg(k_vscrollp,xlFalse,&vscrollp);
    xlGetKeyArg(k_autohscrollp,xlFalse,&autohscrollp);
    xlGetKeyArg(k_autovscrollp,xlFalse,&autovscrollp);
    xlGetKeyArg(k_multilinep,xlFalse,&multilinep);
    xlGetKeyArg(k_justify,s_left,&justify);
    xlGetKeyArg(k_borderp,xlTrue,&borderp);
    GetWidgetPosition(&pos);

    /* setup the style */
    style = WS_CHILD | WS_VISIBLE | ES_NOHIDESEL;
    if (type == wtEditText)
        style |= DS_LOCALEDIT;
    if (hscrollp != xlFalse)
        style |= WS_HSCROLL;
    if (vscrollp != xlFalse)
        style |= WS_VSCROLL;
    if (autohscrollp != xlFalse)
        style |= ES_AUTOHSCROLL;
    if (autovscrollp != xlFalse)
        style |= ES_AUTOVSCROLL;
    if (multilinep != xlFalse)
        style |= ES_MULTILINE;
    if (justify == s_left)
        style |= ES_LEFT;
    else if (justify == s_right)
        style |= ES_RIGHT;
    else if (justify == s_center)
        style |= ES_CENTER;
    else
        xlError("expecting 'left, 'right or 'center",justify);
    if (borderp != xlFalse)
        style |= WS_BORDER;

    /* allocate the widget data structure */
    if (!(widget = MakeWidget(parent,type,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* make the button */
    wnd = CreateWindow(type == wtEditText ? "edit" : "RichEdit",
                       xlStringP(label) ? xlGetString(label) : "Edit Text",
                       style,
                       pos.x,               /* horizontal position */
                       pos.y,               /* vertical position */
                       pos.width,           /* width */
                       pos.height,          /* height */
                       GetParentWnd(parent),
                       (HMENU)widget->childID,
                       hInstance,
                       NULL);

    /* make sure the widget was created */
    if ((widget->wnd = wnd) == NULL) {
        FreeWidget(widget);
        return xlNil;
    }

    /* store the widget data structure pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)widget);

    /* setup to filter messages */
    widget->wndProc = (WNDPROC)SetWindowLong(widget->wnd,GWL_WNDPROC,(LONG)TextFilter);

    /* set the font */
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(ANSI_FIXED_FONT),0);
    
    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* LoadFile - load a file into an edit text control */
static int LoadFile(Widget *widget,char *name)
{
    char *buffer;
    size_t size;
    FILE *fp;

    /* open the file */
    if ((fp = fopen(name,"rb")) == NULL)
        return FALSE;

    /* get the file size */
    fseek(fp,0L,SEEK_END);
    size = ftell(fp);
    fseek(fp,0L,SEEK_SET);

    /* make sure the file will fit in the buffer */
    if (size > 32767) {
        fclose(fp);
        return FALSE;
    }

    /* allocate a buffer */
    if ((buffer = malloc(size)) == NULL) {
        fclose(fp);
        return FALSE;
    }

    /* read the file into the buffer */
    if (fread(buffer,1,size,fp) != size) {
        free(buffer);
        fclose(fp);
        return FALSE;
    }
    fclose(fp);

    /* set the edit text */
    SetWindowText(widget->wnd,buffer);
    free(buffer);
    
    /* return successfully */
    return TRUE;
}

/* RichLoader - loader function for rich edit text controls */
static DWORD CALLBACK RichLoader(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb)
{
    size_t cnt = fread((char *)pbBuff,1,(size_t)cb,(FILE *)dwCookie);
    if (cnt >= 0) *pcb = (LONG)cnt;
    return cnt >= 0 ? 0 : 1;
}

/* RichLoadFile - load a file into a rich edit text control */
static int RichLoadFile(Widget *widget,char *name)
{
    EDITSTREAM stream;
    LRESULT cnt;
    FILE *fp;

    /* open the file */
    if ((fp = fopen(name,"rb")) == NULL)
        return FALSE;

    /* setup the input stream */
    stream.dwCookie = (DWORD)fp;
    stream.dwError = 0;
    stream.pfnCallback = RichLoader;

    /* load the file */
    cnt = SendMessage(widget->wnd,EM_STREAMIN,(WPARAM)SF_TEXT,(LPARAM)&stream);

    /* close the file and return */
    fclose(fp);
    return cnt >= 0;
}

/* xLoadFile - built-in function 'load-file!' */
static xlValue xLoadFile(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_editText);
    xlVal = xlGetArgString();
    xlLastArg();

    /* load the file */
    return LoadFile(GetWidget(obj),xlGetString(xlVal)) ? xlTrue : xlFalse;
}

/* xRichLoadFile - built-in function 'load-file!' */
static xlValue xRichLoadFile(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_richEditText);
    xlVal = xlGetArgString();
    xlLastArg();

    /* load the file */
    return RichLoadFile(GetWidget(obj),xlGetString(xlVal)) ? xlTrue : xlFalse;
}

/* SaveFile - save an edit text control into a file */
static int SaveFile(Widget *widget,char *name)
{
    char *buffer;
    size_t size;
    FILE *fp;

    /* open the file */
    if ((fp = fopen(name,"wb")) == NULL)
        return FALSE;

    /* get the window text length */
    size = GetWindowTextLength(widget->wnd);

    /* allocate a buffer */
    if ((buffer = malloc(size + 1)) == NULL) {
        fclose(fp);
        return FALSE;
    }

    /* get the edit text */
    GetWindowText(widget->wnd,buffer,size + 1);

    /* write the text to the file */
    if (fwrite(buffer,1,size,fp) != size) {
        free(buffer);
        return FALSE;
    }
    free(buffer);
    fclose(fp);

    /* return successfully */
    return TRUE;
}

/* RichSaver - saver function for rich edit text controls */
static DWORD CALLBACK RichSaver(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb)
{
    size_t cnt = fwrite((char *)pbBuff,1,(size_t)cb,(FILE *)dwCookie);
    if ((LONG)cnt == cb) *pcb = (LONG)cnt;
    return (LONG)cnt == cb ? 0 : 1;
}

/* RichSaveFile - save a rich edit text control into a file */
static int RichSaveFile(Widget *widget,char *name)
{
    EDITSTREAM stream;
    LRESULT cnt;
    FILE *fp;

    /* open the file */
    if ((fp = fopen(name,"wb")) == NULL)
        return FALSE;

    /* setup the input stream */
    stream.dwCookie = (DWORD)fp;
    stream.dwError = 0;
    stream.pfnCallback = RichSaver;

    /* load the file */
    cnt = SendMessage(widget->wnd,EM_STREAMOUT,(WPARAM)SF_TEXT,(LPARAM)&stream);

    /* close the file and return */
    fclose(fp);
    return cnt >= 0;
}

/* xSaveFile - built-in function 'save-file' */
static xlValue xSaveFile(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_editText);
    xlVal = xlGetArgString();
    xlLastArg();

    /* save the edit text control into the file */
    return SaveFile(GetWidget(obj),xlGetString(xlVal)) ? xlTrue : xlFalse;
}

/* xRichSaveFile - built-in function 'save-file' */
static xlValue xRichSaveFile(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_richEditText);
    xlVal = xlGetArgString();
    xlLastArg();

    /* save the edit text control into the file */
    return RichSaveFile(GetWidget(obj),xlGetString(xlVal)) ? xlTrue : xlFalse;
}

/* xGetSelectedText - built-in function 'get-selected-text' */
static xlValue xGetSelectedText(void)
{
    DWORD start,end,length;
    xlValue obj,str;
    Widget *widget;
    HLOCAL hText;
    char *p;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlLastArg();

    /* get the widget structure */
    widget = GetWidget(obj);

    /* get the selection */
    SendMessage(widget->wnd,EM_GETSEL,(WPARAM)&start,(LPARAM)&end);

    /* compute the text length */
    if ((length = end - start) < 0)
        return xlNil;

    /* get a handle to the edit text */
    if ((hText = (HLOCAL)SendMessage(widget->wnd,EM_GETHANDLE,0,0)) == NULL)
        return xlNil;

    /* allocate a string and get the selection */
    xlCPush(obj);
    str = xlNewString((xlFIXTYPE)length);
    if ((p = (char *)LocalLock(hText)) == NULL) {
        DisplayLastError();
        str = xlNil;
    }
    else {
        memcpy(xlGetString(str),p,length);
        LocalUnlock(hText);
    }
    xlDrop(1);

    /* return the string */
    return str;
}

/* xRichGetSelectedText - built-in function 'get-selected-text' */
static xlValue xRichGetSelectedText(void)
{
    DWORD start,end,length;
    xlValue obj,str;
    Widget *widget;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicEditText);
    xlLastArg();

    /* get the widget structure */
    widget = GetWidget(obj);

    /* get the selection */
    SendMessage(widget->wnd,EM_GETSEL,(WPARAM)&start,(LPARAM)&end);

    /* compute the text length */
    if ((length = end - start) < 0)
        return xlNil;

    /* allocate a string and get the selection */
    xlCPush(obj);
    str = xlNewString((xlFIXTYPE)length);
    SendMessage(widget->wnd,EM_GETSELTEXT,0,(LPARAM)xlGetString(str));
    xlDrop(1);

    /* return the string */
    return str;
}

/* TextFilter - message filter for edit-text and rich-edit-text controls */
LRESULT CALLBACK TextFilter(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    Widget *widget = (Widget *)GetWindowLong(wnd,GWL_USERDATA);
    CharBinding *binding;

    /* dispatch on the message type */
    switch (msg) {
    case WM_CHAR:
        for (binding = widget->charBindings; binding; binding = binding->next)
            if ((TCHAR)wParam == binding->ch) {
                xlValue val;
                xlCallFunction(&val,1,binding->fcn,1,xlMakeChar((xlCHRTYPE)wParam));
                if (val != xlFalse)
                    return 0;
            }
        break;
    }
    
    /* call the original window procedure to handle other messages */
    return (*widget->wndProc)(wnd,msg,wParam,lParam);
}

/* DisplayLastError - display the last system error */
void DisplayLastError(void)
{
    LPVOID lpMsgBuf;
    FormatMessage( 
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,    
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0,
        NULL );
    MessageBox(NULL,lpMsgBuf,"GetLastError",MB_OK | MB_ICONINFORMATION);
    LocalFree(lpMsgBuf);
}