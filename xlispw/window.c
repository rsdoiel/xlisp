/* window.c - window classes */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include "xlisp.h"
#include "xlispw.h"
#include "widget.h"

/* instance variable offsets */
#define windowMENU      (xlFIRSTIVAR + 1)
#define devCtxtHANDLE   xlFIRSTIVAR

/* globals */
xlValue c_window;

/* classes */
static xlValue c_basicWindow;
static xlValue c_dialog;

/* keywords and symbols */
static xlValue k_title;
static xlValue k_filter;
static xlValue k_defaultname;
static xlValue k_defaultextension;
static xlValue k_initialdirectory;
static xlValue s_close;
static xlValue s_selectionchanged;
static xlValue s_doubleclick;

/* externals */
extern xlValue c_widget;
extern xlValue c_frame;
extern xlValue c_menu;

/* function handlers */
static xlValue xWindowInitialize(void);
static xlValue xDialogInitialize(void);
static xlValue xBringToTop(void);
static xlValue xUpdate(void);
static xlValue xClose(void);
static xlValue xGetMenu(void);
static xlValue xSetMenu(void);
static xlValue xDrawMenuBar(void);
static void xOpenFileDialog(void);
static void xSaveFileDialog(void);

/* prototypes */
static xlValue MakeWindow(xlValue obj,FrameWidget *parent,char *title,char *className,DWORD style);
static int WindowHandler(Widget *widget,int fcn);
static void InitOpenFileNameStructure(OPENFILENAME *ofn,char *path,char *file);
LRESULT CALLBACK ControlWndProc(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam);
static LRESULT ControlCommand(HWND wnd,int code);
static int WidgetControlP(Widget *widget);

/* InitWindows - initialize the window classes */
void InitWindows(void)
{
    WNDCLASSEX wndClass;

    /* setup the window class structure */
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = ControlWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL,IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "xlwindow";
    wndClass.hIconSm = LoadIcon(NULL,IDI_APPLICATION);

    /* register the window class */
    RegisterClassEx(&wndClass);

    /* setup the window class structure */
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = ControlWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL,IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(LTGRAY_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "xldialog";
    wndClass.hIconSm = LoadIcon(NULL,IDI_APPLICATION);

    /* register the window class */
    RegisterClassEx(&wndClass);

    /* enter the built-in symbols */
    k_title             = xlEnterKeyword("TITLE");
    k_filter            = xlEnterKeyword("FILTER");
    k_defaultname       = xlEnterKeyword("DEFAULT-NAME");
    k_defaultextension  = xlEnterKeyword("DEFAULT-EXTENSION");
    k_initialdirectory  = xlEnterKeyword("INITIAL-DIRECTORY");
    s_close             = xlEnter("CLOSE");
    s_selectionchanged  = xlEnter("SELECTION-CHANGED");
    s_doubleclick       = xlEnter("DOUBLE-CLICK");

    /* 'basic-window' class */
    c_basicWindow = xlClass("BASIC-WINDOW",c_frame,"");
    xlMethod(c_basicWindow,"BRING-TO-TOP",xBringToTop);
    xlMethod(c_basicWindow,"UPDATE",xUpdate);
    xlMethod(c_basicWindow,"CLOSE",xClose);

    /* 'window' class */
    c_window = xlClass("WINDOW",c_basicWindow,"MENU");
    xlMethod(c_window,"INITIALIZE",xWindowInitialize);
    xlMethod(c_window,"GET-MENU",xGetMenu);
    xlMethod(c_window,"SET-MENU!",xSetMenu);
    xlMethod(c_window,"DRAW-MENU-BAR",xDrawMenuBar);
    xlXMethod(c_window,"OPEN-FILE-DIALOG",xOpenFileDialog);
    xlXMethod(c_window,"SAVE-FILE-DIALOG",xSaveFileDialog);

    /* 'dialog' class */
    c_dialog = xlClass("DIALOG",c_basicWindow,"");
    xlMethod(c_dialog,"INITIALIZE",xDialogInitialize);
}

/* xWindowInitialize - built-in method (window 'initialize) */
static xlValue xWindowInitialize(void)
{
    xlValue obj = xlGetArgInstance(c_window);
    xlVal = xlGetArgString();
    return MakeWindow(obj,NULL,xlGetString(xlVal),"xlwindow",WS_OVERLAPPEDWINDOW);
}

/* xDialogInitialize - built-in method (dialog 'initialize) */
static xlValue xDialogInitialize(void)
{
    xlValue obj = xlGetArgInstance(c_dialog);
    xlVal = xlGetArgString();
    return MakeWindow(obj,NULL,xlGetString(xlVal),"xldialog",WS_OVERLAPPEDWINDOW);
}

/* MakeWindow - make a window */
static xlValue MakeWindow(xlValue obj,FrameWidget *parent,char *title,char *className,DWORD style)
{
    WidgetPosition pos;
    FrameWidget *frame;
    HWND parentWnd,wnd;
    
    /* parse the arguments */
    GetWidgetPosition(&pos);
    
    /* allocate the window data structure */
    if (!(frame = MakeFrameWidget(NULL,wtWindow,sizeof(FrameWidget))))
        return xlNil;
    frame->widget.position = pos;
    frame->widget.handler = WindowHandler;
    frame->style = style;

    /* determine the parent window */
    parentWnd = parent ? GetParentWnd(parent) : NULL;

    /* create the window */
    wnd = CreateWindow(className,           /* class name */
                       title,               /* window name */
                       frame->style,        /* window style */
                       pos.x,               /* horizontal position */
                       pos.y,               /* vertical position */
                       pos.width,           /* width */
                       pos.height,          /* height */
                       parentWnd,           /* parent */
                       NULL,                /* menu */
                       hInstance,           /* application instance */
                       NULL);               /* window creation data */

    /* make sure the window was created */
    if (wnd == NULL) {
        FreeWidget((Widget *)frame);
        return xlNil;
    }

    /* store the widget pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)frame);
    frame->widget.wnd = wnd;

    /* initialize the widget */
    frame->widget.obj = obj;

    /* return the widget object */
    SetWidget(obj,(Widget *)frame);
    return obj;
}

/* WindowHandler - handler function for windows */
static int WindowHandler(Widget *widget,int fcn)
{
    FrameWidget *frame = (FrameWidget *)widget;
    RECT rect;
    switch (fcn) {
    case whfDisplaySize:
        rect.left = 0;
        rect.top = 0;
        rect.right = frame->widget.width;
        rect.bottom = frame->widget.height;
        AdjustWindowRect(&rect,frame->style,frame->menu ? TRUE : FALSE);
        frame->widget.displayWidth = rect.right - rect.left;
        frame->widget.displayHeight = rect.bottom - rect.top;
        return TRUE;
    }
    return FALSE;
}

/* xBringToTop - built-in method (window 'bring-to-top) */
static xlValue xBringToTop(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicWindow);
    xlLastArg();

    /* bring the window to the top */
    BringWindowToTop(GetWidget(obj)->wnd);
    return obj;
}

/* xUpdate - built-in method (window 'update) */
static xlValue xUpdate(void)
{
    Widget *widget;
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicWindow);
    xlLastArg();

    /* get the widget */
    widget = GetWidget(obj);

    /* update the window */
    InvalidateRect(widget->wnd,NULL,FALSE);
    UpdateWindow(widget->wnd);
    return obj;
}

/* xClose - built-in method (window 'close) */
static xlValue xClose(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_basicWindow);
    xlLastArg();

    /* just return the window object */
    return obj;
}

/* xGetMenu - built-in method (window 'get-menu) */
static xlValue xGetMenu(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_window);
    xlLastArg();

    /* get and return the menu */
    return xlGetIVar(obj,windowMENU);
}

/* xSetMenu - built-in method (window 'set-menu!) */
static xlValue xSetMenu(void)
{
    FrameWidget *frame;
    xlValue obj,menu;

    /* parse the arguments */
    obj = xlGetArgInstance(c_window);
    menu = xlGetArgInstance(c_menu);
    xlLastArg();

    /* get the frame */
    frame = (FrameWidget *)GetWidget(obj);

    /* set the menu */
    xlSetIVar(obj,windowMENU,menu);
    frame->menu = GetMenuHandle(menu);
    return SetMenu(frame->widget.wnd,frame->menu->h) ? xlTrue : xlFalse;
}

/* xDrawMenuBar - built-in method (window 'draw-menu-bar) */
static xlValue xDrawMenuBar(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_window);
    xlLastArg();

    /* draw the menu bar */
    return DrawMenuBar(GetWidget(obj)->wnd) ? xlTrue : xlFalse;
}

/* xOpenFileDialog - built-in method (window 'open-file-dialog) */
static void xOpenFileDialog(void)
{
    char path[_MAX_PATH],file[_MAX_FNAME + _MAX_EXT];
    OPENFILENAME ofn;

    /* fill in the open file name structure */
    InitOpenFileNameStructure(&ofn,path,file);
    
    /* display the dialog */
    if (!GetOpenFileName(&ofn)) {
        xlVal = xlNil;
        xlSVReturn();
    }

    /* return the path and file name */
    xlCheck(3);
    xlPush(xlMakeFixnum(ofn.nFilterIndex));
    xlPush(xlMakeCString(file));
    xlPush(xlMakeCString(path));
    xlMVReturn(3);
}

/* xSaveFileDialog - built-in method (window 'save-file-dialog) */
static void xSaveFileDialog(void)
{
    char path[_MAX_PATH],file[_MAX_FNAME + _MAX_EXT];
    OPENFILENAME ofn;

    /* fill in the open file name structure */
    InitOpenFileNameStructure(&ofn,path,file);
    ofn.Flags = OFN_OVERWRITEPROMPT;
    
    /* display the dialog */
    if (!GetSaveFileName(&ofn)) {
        xlVal = xlNil;
        xlSVReturn();
    }

    /* return the path and file name */
    xlCheck(3);
    xlPush(xlMakeFixnum(ofn.nFilterIndex));
    xlPush(xlMakeCString(file));
    xlPush(xlMakeCString(path));
    xlMVReturn(3);
}

/* InitOpenFileNameStructure - initialize the open file name structure */
static void InitOpenFileNameStructure(OPENFILENAME *ofn,char *path,char *file)
{
    xlValue obj,title,filter,defaultName,defaultExtension,initialDirectory;
    char *src,*dst;
    xlFIXTYPE len;
    int ch;

    /* parse the argument list */
    obj = xlGetArgInstance(c_window);
    xlGetKeyArg(k_title,xlNil,&title);
    xlGetKeyArg(k_filter,xlNil,&filter);
    xlGetKeyArg(k_defaultname,xlNil,&defaultName);
    xlGetKeyArg(k_defaultextension,xlNil,&defaultExtension);
    xlGetKeyArg(k_initialdirectory,xlNil,&initialDirectory);

    /* make sure they are all strings */
    EnsureStringOrNil(title);
    EnsureStringOrNil(filter);
    EnsureStringOrNil(defaultName);
    EnsureStringOrNil(defaultExtension);
    EnsureStringOrNil(initialDirectory);

    /* make the filter string */
    if (filter != xlNil) {
        xlCheck(5);
        xlPush(title);
        xlPush(filter);
        xlPush(defaultName);
        xlPush(defaultExtension);
        xlPush(initialDirectory);
        len = xlGetSLength(filter);
        xlVal = xlNewString(len + 2);
        src = xlGetString(filter);
        dst = xlGetString(xlVal);
        while (--len >= 0) {
            if ((ch = *src++) == '|')
                *dst++ = '\0';
            else
                *dst++ = ch;
        }
        *dst++ = '\0';
        *dst++ = '\0';
        xlDrop(5);
    }

    /* initialize the open file name structure */
    ofn->lStructSize = sizeof(OPENFILENAME);
    ofn->hwndOwner = GetWidget(obj)->wnd;
    ofn->hInstance = NULL;
    ofn->lpstrFilter = NULL;
    ofn->lpstrCustomFilter = NULL;
    ofn->nMaxCustFilter = 0;
    ofn->nFilterIndex = 0;
    ofn->lpstrFile = NULL;
    ofn->nMaxFile = 0;
    ofn->lpstrFileTitle = NULL;
    ofn->nMaxFileTitle = 0;
    ofn->lpstrInitialDir = NULL;
    ofn->lpstrTitle = NULL;
    ofn->Flags = 0;
    ofn->nFileOffset = 0;
    ofn->nFileExtension = 0;
    ofn->lpstrDefExt = NULL;
    ofn->lCustData = 0;
    ofn->lpfnHook = NULL;
    ofn->lpTemplateName = NULL;

    /* fill in the structure */
    if (filter != xlNil)
        ofn->lpstrFilter = xlGetString(xlVal);
    ofn->lpstrFile = path;
    ofn->nMaxFile = _MAX_PATH;
    ofn->lpstrFileTitle = file;
    ofn->nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
    if (initialDirectory != xlNil)
        ofn->lpstrInitialDir = xlGetString(initialDirectory);
    if (title != xlNil)
        ofn->lpstrTitle = xlGetString(title);
    if (defaultExtension != xlNil)
        ofn->lpstrDefExt = xlGetString(defaultExtension);

    /* initialize the file name */
    if (defaultName == xlNil)
        path[0] = '\0';
    else {
        strncpy(path,xlGetString(defaultName),sizeof(path));
        path[sizeof(path) - 1] = '\0';
    }
}

/* ControlWndProc - control window procedure */
LRESULT CALLBACK ControlWndProc(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    FrameWidget *frame = (FrameWidget *)GetWindowLong(wnd,GWL_USERDATA);
    TOOLTIPTEXT *ttText;
    HWND controlWnd;
    Widget *widget;
    NMHDR *nmHdr;
    char *p;

    /* handle messages during window creation */
    if (!frame)
        return DefWindowProc(wnd,msg,wParam,lParam);

    /* dispatch on the message type */
    switch (msg) {
    case WM_HSCROLL:
        return ControlScroll((HWND)lParam,LOWORD(wParam),HIWORD(wParam));
    case WM_COMMAND:
        if ((controlWnd = (HWND)lParam) != NULL) {
            Widget *widget = (Widget *)GetWindowLong(controlWnd,GWL_USERDATA);
            if (CheckBindings(widget,LOWORD(wParam)))
                return 0;
            return ControlCommand(controlWnd,HIWORD(wParam));
        }
        else if (HIWORD(wParam) == 0 && CheckMenuBindings(LOWORD(wParam)))
            return 0;
        else if (HIWORD(wParam) == 1 && CheckAccelerators(LOWORD(wParam)))
            return 0;
        break;
    case WM_SETFOCUS:
        for (widget = frame->children; widget != NULL; widget = widget->next)
            if (widget->wnd) {
                SetFocus(widget->wnd);
                return 0;
            }
        break;
    case WM_INITMENU:
    case WM_INITMENUPOPUP:
        if (frame->menu && PrepareMenu(frame->menu,(HMENU)wParam))
            return 0;
        break;
    case WM_NOTIFY:
        nmHdr = (NMHDR *)lParam;
        switch (nmHdr->code) {
        case TTN_NEEDTEXT:
            ttText = (TOOLTIPTEXT *)lParam;
            widget = (Widget *)GetWindowLong(nmHdr->hwndFrom,GWL_USERDATA);
            if (widget && (p = GetToolTipText(widget,nmHdr->idFrom)) != NULL) {
                strncpy(ttText->szText,p,sizeof(ttText->szText));
                ttText->szText[sizeof(ttText->szText) - 1] = '\0';
                return 0;
            }
            break;
        }
        break;
    case WM_SIZING:
        frame->widget.flags |= wfResize;
        break;
    case WM_SIZE:
        if (frame->widget.flags & wfResize) {
            frame->widget.flags &= ~wfResize;
            PlaceWidgets(frame,FALSE);
        }
        break;
    case WM_PAINT:
        if (frame->widget.flags & wfRecalc) {
            frame->widget.flags &= ~wfRecalc;
            PlaceWidgets(frame,TRUE);
        }
        break;
    case WM_CLOSE:
        if (frame->widget.obj != xlNil) 
            xlSendMessage(NULL,0,frame->widget.obj,s_close,0);
        break;
    }

    /* call the default window procedure to handle other messages */
    return DefWindowProc(wnd,msg,wParam,lParam);
}

/* ControlCommand - handle the WM_COMMAND message */
static LRESULT ControlCommand(HWND wnd,int code)
{
    Widget *widget = (Widget *)GetWindowLong(wnd,GWL_USERDATA);
    Widget *child;
    switch (code) {
    case BN_CLICKED:
        if (widget->groupID >= 0) {
            SendMessage(widget->wnd,BM_SETCHECK,1,0);
            for (child = widget->parent->children; child != NULL; child = child->next)
                if (WidgetControlP(child)
                &&  (Widget *)child != widget
                &&  ((Widget *)child)->groupID == widget->groupID)
                    SendMessage(child->wnd,BM_SETCHECK,0,0);
        }
        if (widget->obj != xlNil) {
            if (widget->sendValueP) {
                int value = SendMessage(wnd,BM_GETCHECK,0,0);
                xlCallFunction(NULL,0,widget->obj,1,value ? xlTrue : xlFalse);
            }
            else
                xlCallFunction(NULL,0,widget->obj,0);
        }
        break;
    case CBN_SELENDOK:
        if (widget->obj != xlNil) {
            int index = SendMessage(wnd,CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
            if (index != CB_ERR) {
                char buf[256];
                DWORD i = SendMessage(wnd,CB_GETITEMDATA,(WPARAM)index,(LPARAM)0);
                SendMessage(wnd,CB_GETLBTEXT,(WPARAM)index,(LPARAM)buf);
                xlCallFunction(NULL,0,widget->obj,2,xlMakeCString(buf),xlMakeFixnum(i));
            }
        }
        break;
    case LBN_SELCHANGE:
        xlSendMessage(NULL,0,widget->obj,s_selectionchanged,0);
        break;
    case LBN_DBLCLK:
        xlSendMessage(NULL,0,widget->obj,s_doubleclick,0);
        break;
    }
    return 0;
}

/* WidgetControlP - widget control predicate */
static int WidgetControlP(Widget *widget)
{
    int sts;
    switch (widget->type) {
    case wtScrollBar:
    case wtSlider:
    case wtPushButton:
    case wtDefPushButton:
    case wtRadioButton:
    case wtCheckBox:
    case wtGroupBox:
    case wtDropDownMenu:
    case wtStaticText:
    case wtEditText:
    case wtRichEditText:
    case wtCanvas:
    case wtToolbar:
        sts = TRUE;
        break;
    default:
        sts = FALSE;
        break;
    }
    return sts;
}