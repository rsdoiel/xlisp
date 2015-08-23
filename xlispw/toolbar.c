/* toolbar.c - routines to support toolbars */
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
#include "tclicontobitmap.h"

/* local variables */
static xlValue c_toolbar;

/* keywords */
static xlValue k_tip;

/* externals */
extern xlValue c_widget;

/* prototypes */
static xlValue xToolbarInitialize(void);
static xlValue xAddButton(void);
static xlValue xAddStandardButton(void);
static xlValue xAddSeparator(void);
static xlValue xGetState(void);
static xlValue xSetState(void);
static void RecomputeSize(Widget *toolbar);
static int ToolbarHandler(Widget *widget,int fcn);

/* InitToolbar - initialize the toolbar support */
void InitToolbars(void)
{
    /* enter the built-in symbols */
    k_tip = xlEnterKeyword("TIP");

    /* 'toolbar' class */
    c_toolbar = xlClass("TOOLBAR",c_widget,"");
    xlMethod(c_toolbar,"INITIALIZE",xToolbarInitialize);
    xlMethod(c_toolbar,"ADD-BUTTON!",xAddButton);
    xlMethod(c_toolbar,"ADD-STANDARD-BUTTON!",xAddStandardButton);
    xlMethod(c_toolbar,"ADD-SEPARATOR!",xAddSeparator);
    xlMethod(c_toolbar,"GET-STATE",xGetState);
    xlMethod(c_toolbar,"SET-STATE!",xSetState);

    /* standard button icon ids */
    xlSetValue(xlEnter("*STD-COPY-ICON*"),xlMakeFixnum(STD_COPY)); 
    xlSetValue(xlEnter("*STD-PASTE-ICON*"),xlMakeFixnum(STD_PASTE));
    xlSetValue(xlEnter("*STD-CUT-ICON*"),xlMakeFixnum(STD_CUT));
    xlSetValue(xlEnter("*STD-PRINT-ICON*"),xlMakeFixnum(STD_PRINT));
    xlSetValue(xlEnter("*STD-DELETE-ICON*"),xlMakeFixnum(STD_DELETE));
    xlSetValue(xlEnter("*STD-PRINT-PREVIEW-ICON*"),xlMakeFixnum(STD_PRINTPRE));
    xlSetValue(xlEnter("*STD-FILE-NEW-ICON*"),xlMakeFixnum(STD_FILENEW));
    xlSetValue(xlEnter("*STD-PROPERTIES-ICON*"),xlMakeFixnum(STD_PROPERTIES));
    xlSetValue(xlEnter("*STD-FILE-OPEN-ICON*"),xlMakeFixnum(STD_FILEOPEN));
    xlSetValue(xlEnter("*STD-REDO-ICON*"),xlMakeFixnum(STD_REDOW));
    xlSetValue(xlEnter("*STD-FILE-SAVE-ICON*"),xlMakeFixnum(STD_FILESAVE));
    xlSetValue(xlEnter("*STD-REPLACE-ICON*"),xlMakeFixnum(STD_REPLACE));
    xlSetValue(xlEnter("*STD-FIND-ICON*"),xlMakeFixnum(STD_FIND));
    xlSetValue(xlEnter("*STD-UNDO-ICON*"),xlMakeFixnum(STD_UNDO));
    xlSetValue(xlEnter("*STD-HELP-ICON*"),xlMakeFixnum(STD_HELP));
}

/* xToolbarInitialize - built-in method (toolbar 'initialize) */
static xlValue xToolbarInitialize(void)
{
    FrameWidget *parent;
    TBADDBITMAP addBitmap;
    HWND wnd,tipsWnd;
    Widget *widget;
    DWORD style;
    xlValue obj;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_toolbar);
    parent = GetArgFrameWidget();
    xlLastArg();
    
    /* allocate the window data structure */
    if (!(widget = MakeWidget(parent,wtToolbar,sizeof(Widget))))
        return xlNil;
    
    /* setup the window style */
    style = WS_CHILD | TBSTYLE_TOOLTIPS | CCS_ADJUSTABLE | CCS_TOP;

    /* create the window */
    wnd = CreateWindow(TOOLBARCLASSNAME,    /* class name */
                       NULL,                /* window name */
                       style,               /* window style */
                       0,                   /* horizontal position */
                       0,                   /* vertical position */
                       0,                   /* width */
                       0,                   /* height */
                       parent->widget.wnd,  /* parent */
                       (HMENU)1,            /* menu */
                       hInstance,           /* application instance */
                       NULL);               /* window creation data */

    /* make sure the window was created */
    if (wnd == NULL) {
        FreeWidget(widget);
        return xlNil;
    }

    /* send the TB_BUTTONSTRUCTSIZE message (required for backward compatibility) */
    SendMessage(wnd,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0); 

    /* get the tool tips window */
    tipsWnd = (HWND)SendMessage(wnd,TB_GETTOOLTIPS,0,0);

    /* store the widget pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)widget);
    if (tipsWnd)
        SetWindowLong(tipsWnd,GWL_USERDATA,(long)widget);
    widget->wnd = wnd;

    /* add the standard button bitmaps */
    addBitmap.hInst = HINST_COMMCTRL;
    addBitmap.nID = IDB_STD_SMALL_COLOR;
    SendMessage(wnd,TB_ADDBITMAP,(WPARAM)15,(LPARAM)&addBitmap);

    /* initialize the widget */
    widget->flags |= wfAutoPlace;
    widget->handler = ToolbarHandler;
    widget->obj = obj;
    widget->position.side = wsTop;
    widget->position.anchor = waTop;
    widget->position.fill = wfNone;
    widget->position.x = CW_USEDEFAULT;
    widget->position.y = CW_USEDEFAULT;
    widget->position.width = CW_USEDEFAULT;
    widget->position.height = CW_USEDEFAULT;

    /* show the toolbar */
    ShowWindow(wnd,SW_SHOW);

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* xAddButton - built-in method (toolbar 'add-button!) */
static xlValue xAddButton(void)
{
    Widget *toolbar;
    DWORD width,height;
    TBADDBITMAP bitmap;
    TBBUTTON button;
    HBITMAP hBitmap;
    xlValue obj,tip;
    char *iconStr;
    UINT id;
    HDC dc;
    int i;

    /* parse the arguments */
    obj = xlGetArgInstance(c_toolbar);
    xlVal = xlGetArgString(); iconStr = xlGetString(xlVal);
    xlVal = xlGetArg();
    xlGetKeyArg(k_tip,xlNil,&tip); EnsureStringOrNil(tip);

    /* get the toolbar widget */
    toolbar = GetWidget(obj);

    /* make a device context for the toolbar */
    if ((dc = GetDC(toolbar->wnd)) == NULL)
        return xlNil;

    /* make a command binding */
    if ((id = MakeBinding(toolbar,xlVal)) == 0)
        return xlFalse;

    /* make the button bitmap */
    hBitmap = TclIconToBitmap(dc,iconStr,&width,&height);
    ReleaseDC(toolbar->wnd,dc);

    /* make sure the bitmap was created */
    if (!hBitmap) {
        RemoveBinding(toolbar,id);
        return xlNil;
    }

    /* setup the bitmap definition */
    bitmap.hInst = NULL;
    bitmap.nID = (UINT)hBitmap;

    /* add the bitmap to the toolbar */
    i = SendMessage(toolbar->wnd,TB_ADDBITMAP,(WPARAM)1,(LPARAM)&bitmap);

    /* setup the button definition */
    button.iBitmap = i; 
    button.idCommand = id; 
    button.fsState = TBSTATE_ENABLED; 
    button.fsStyle = TBSTYLE_BUTTON; 
    button.dwData = 0; 
    button.iString = 0;

    /* add the button */
    if (!SendMessage(toolbar->wnd,TB_ADDBUTTONS,(WPARAM)1,(LPARAM)&button)) {
        RemoveBinding(toolbar,id);
        return xlFalse;
    }

    /* add the tool tip */
    if (tip != xlNil) {
        ToolTipText *ttt = (ToolTipText *)malloc(sizeof(ToolTipText));
        char *text = xlGetString(tip);
        if (!ttt) {
            RemoveBinding(toolbar,id);
            return xlFalse;
        }
        if ((ttt->text = malloc(strlen(text) + 1)) == NULL) {
            free(ttt);
            RemoveBinding(toolbar,id);
            return xlFalse;
        }
        strcpy(ttt->text,text);
        ttt->id = id;
        ttt->next = toolbar->tips;
        toolbar->tips = ttt;
    }

    /* recompute the size of the toolbar */
    RecomputeSize(toolbar);

    /* return the first index */
    return xlMakeFixnum((xlFIXTYPE)id);
}

/* xAddStandardButton - built-in function 'add-standard-button!' */
static xlValue xAddStandardButton(void)
{
    Widget *toolbar;
    TBBUTTON button;
    xlValue obj,tip;
    int id;

    /* parse the arguments */
    obj = xlGetArgInstance(c_toolbar);
    xlVal = xlGetArgFixnum(); button.iBitmap = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArg();
    xlGetKeyArg(k_tip,xlNil,&tip); EnsureStringOrNil(tip);

    /* get the toolbar */
    toolbar = GetWidget(obj);

    /* make a command binding */
    if ((id = MakeBinding(toolbar,xlVal)) == 0)
        return xlFalse;

    /* setup the button definition */
    button.idCommand = id; 
    button.fsState = TBSTATE_ENABLED; 
    button.fsStyle = TBSTYLE_BUTTON; 
    button.dwData = 0; 
    button.iString = 0; 

    /* add the button */
    if (!SendMessage(toolbar->wnd,TB_ADDBUTTONS,(WPARAM)1,(LPARAM)&button)) {
        RemoveBinding(toolbar,id);
        return xlFalse;
    }

    /* recompute the size of the toolbar */
    RecomputeSize(toolbar);

    /* return the first index */
    return xlMakeFixnum((xlFIXTYPE)id);
}

/* xAddSeparator - built-in method (toolbar 'add-separator!) */
static xlValue xAddSeparator(void)
{
    Widget *toolbar;
    TBBUTTON button;
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_toolbar);
    xlLastArg();

    /* get the toolbar widget */
    toolbar = GetWidget(obj);

    /* setup the button definition */
    button.iBitmap = 0; 
    button.idCommand = 0; 
    button.fsState = 0; 
    button.fsStyle = TBSTYLE_SEP; 
    button.dwData = 0; 
    button.iString = 0;

    /* add the separator */
    if (!SendMessage(toolbar->wnd,TB_ADDBUTTONS,(WPARAM)1,(LPARAM)&button))
        return xlFalse;

    /* recompute the size of the toolbar */
    RecomputeSize(toolbar);

    /* return successfully */
    return xlTrue;
}

/* xGetState - built-in method (toolbar 'get-state) */
static xlValue xGetState(void)
{
    Widget *toolbar;
    LRESULT state;
    xlValue obj;
    int i;

    /* parse the arguments */
    obj = xlGetArgInstance(c_toolbar);
    xlVal = xlGetArgFixnum(); i = (int)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the toolbar widget */
    toolbar = GetWidget(obj);

    /* get the button state */
    state = SendMessage(toolbar->wnd,TB_GETSTATE,(WPARAM)i,0);

    /* return the state */
    return state & TBSTATE_PRESSED ? xlTrue : xlFalse;
}

/* xSetState - built-in method (toolbar 'set-state!) */
static xlValue xSetState(void)
{
    Widget *toolbar;
    LRESULT state;
    xlValue obj;
    int i;

    /* parse the arguments */
    obj = xlGetArgInstance(c_toolbar);
    xlVal = xlGetArgFixnum(); i = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArg();
    xlLastArg();

    /* get the toolbar widget */
    toolbar = GetWidget(obj);

    /* get the current button state */
    state = SendMessage(toolbar->wnd,TB_GETSTATE,(WPARAM)i,0);

    /* set the 'pressed' state */
    if (xlVal != xlFalse)
        state |= TBSTATE_PRESSED;
    else
        state &= ~TBSTATE_PRESSED;

    /* set the new state */
    SendMessage(toolbar->wnd,TB_SETSTATE,(WPARAM)i,(LPARAM)MAKELONG(state,0));

    /* return the toolbar */
    return obj;
}

/* RecomputeSize - recompute the size of a toolbar */
static void RecomputeSize(Widget *toolbar)
{
    LONG hMargins,vMargins;
    RECT fullRect;
    int count,i;

    /* get the number of buttons */
    count = SendMessage(toolbar->wnd,TB_BUTTONCOUNT,0,0);

    /* initialize the full rectangle */
    fullRect.left = 0x7fffffff;
    fullRect.top = 0x7fffffff;
    fullRect.right = 0;
    fullRect.bottom = 0;

    /* get the size of each button */
    for (i = 0; i < count; ++i) {
        RECT rect;
        SendMessage(toolbar->wnd,TB_GETITEMRECT,(WPARAM)i,(LPARAM)&rect);
        if (rect.left < fullRect.left)
            fullRect.left = rect.left;
        if (rect.top < fullRect.top)
            fullRect.top = rect.top;
        if (rect.right > fullRect.right)
            fullRect.right = rect.right;
        if (rect.bottom > fullRect.bottom)
            fullRect.bottom = rect.bottom;
    }

    /* compute the margins */
    hMargins = 2 * fullRect.left;
    vMargins = 2 * fullRect.top;

    /* set the widget size */
    toolbar->position.width = hMargins + (fullRect.right - fullRect.left);
    toolbar->position.height = vMargins + (fullRect.bottom - fullRect.top);
}

/* ToolbarHandler - handler function for toolbars */
static int ToolbarHandler(Widget *widget,int fcn)
{
    switch (fcn) {
    case whfPlace:
        SendMessage(widget->wnd,TB_AUTOSIZE,0,0);
        return TRUE;
    }
    return FALSE;
}

/* GetToolTipText - get the text associated with a toolbar button */
char *GetToolTipText(Widget *widget,UINT id)
{
    ToolTipText *ttt;
    for (ttt = widget->tips; ttt != NULL; ttt = ttt->next)
        if (id == ttt->id)
            return ttt->text;
    return NULL;
}