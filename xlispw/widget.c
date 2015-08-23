/* widget.c - widget routines for xlispw */
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
xlValue c_widget;
xlValue c_frame;

/* classes */
static xlValue c_scrollBar;
static xlValue c_slider;
static xlValue c_pushButton;
static xlValue c_radioButton;
static xlValue c_checkBox;
static xlValue c_groupBox;
static xlValue c_dropDownMenu;
static xlValue c_listBox;
static xlValue c_staticText;

/* instance variable offsets */
#define widgetHANDLE    xlFIRSTIVAR

/* symbols and keywords */
static xlValue s_widget;
static xlValue k_handler;
static xlValue k_from;
static xlValue k_to;
static xlValue k_initialvalue;
static xlValue k_label;
static xlValue k_labellist;
static xlValue k_defaultp;
static xlValue k_tabstopp;
static xlValue k_groupid;
static xlValue k_x;
static xlValue k_y;
static xlValue k_width;
static xlValue k_height;
static xlValue k_side;
static xlValue k_anchor;
static xlValue k_fill;
static xlValue k_sortp;
static xlValue k_vscrollp;
static xlValue k_borderp;
static xlValue s_left;
static xlValue s_right;
static xlValue s_top;
static xlValue s_bottom;
static xlValue s_center;
static xlValue s_topleft;
static xlValue s_topright;
static xlValue s_bottomleft;
static xlValue s_bottomright;
static xlValue s_none;
static xlValue s_x;
static xlValue s_y;
static xlValue s_both;
static xlValue s_draw;

/* function handlers */
static xlValue xMakeWindow(void);
static xlValue xMakeDialog(void);
static xlValue xBringWindowToTop(void);
static xlValue xUpdateWindow(void);
static xlValue xMoveWindow(void);
static xlValue xShowWindow(void);
static xlValue xHideWindow(void);
static void xGetWindowRect(void);
static void xGetClientRect(void);
static xlValue xMakeFrame(void);
static xlValue xGetWidgetText(void);
static xlValue xSetWidgetText(void);
static xlValue xMakeScrollBar(void);
static xlValue xMakeSlider(void);
static xlValue xMakePushButton(void);
static xlValue xMakeRadioButton(void);
static xlValue xMakeCheckBox(void);
static xlValue xMakeGroupBox(void);
static xlValue xMakeDropDownMenu(void);
static xlValue xListBoxInitialize(void);
static xlValue xListBoxAddString(void);
static xlValue xListBoxSetRedraw(void);
static xlValue xListBoxSelectionChanged(void);
static xlValue xListBoxDoubleClick(void);
static xlValue xListBoxGetSelection(void);
static xlValue xListBoxSetSelection(void);
static xlValue xListBoxGetItemData(void);
static xlValue xListBoxReset(void);
static xlValue xMakeStaticText(void);

/* prototypes */
static void InitWidget(FrameWidget *parent,Widget *widget,int type);
static xlValue MakeButton(xlValue obj,FrameWidget *frame,int type,int groupID,int sendValueP);
static int DropDownMenuHandler(Widget *widget,int fcn);
LRESULT CALLBACK GroupBoxFilter(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam);
static LRESULT ControlScrollBar(Widget *data,int part,int value);
static LRESULT ControlSlider(Widget *data,int part,int value);
static int TopLevelWindowP(Widget *widget);

/* InitWidgets - initialize the widget functions */
void InitWidgets(void)
{
    /* load the rich edit control */
    LoadLibrary("RICHED32.DLL");

    /* enter the built-in symbols */
    s_widget            = xlEnter("WIDGET");
    k_handler           = xlEnterKeyword("HANDLER");
    k_from              = xlEnterKeyword("FROM");
    k_to                = xlEnterKeyword("TO");
    k_label             = xlEnterKeyword("LABEL");
    k_labellist         = xlEnterKeyword("LABEL-LIST");
    k_initialvalue      = xlEnterKeyword("INITIAL-VALUE");
    k_defaultp          = xlEnterKeyword("DEFAULT?");
    k_tabstopp          = xlEnterKeyword("TAB-STOP?");
    k_groupid           = xlEnterKeyword("GROUP-ID");
    k_x                 = xlEnterKeyword("X");
    k_y                 = xlEnterKeyword("Y");
    k_width             = xlEnterKeyword("WIDTH");
    k_height            = xlEnterKeyword("HEIGHT");
    k_side              = xlEnterKeyword("SIDE");
    k_anchor            = xlEnterKeyword("ANCHOR");
    k_fill              = xlEnterKeyword("FILL");
    k_sortp             = xlEnterKeyword("SORT?");
    k_vscrollp          = xlEnterKeyword("VSCROLL?");
    k_borderp           = xlEnterKeyword("BORDER?");
    s_left              = xlEnter("LEFT");
    s_right             = xlEnter("RIGHT");
    s_top               = xlEnter("TOP");
    s_bottom            = xlEnter("BOTTOM");
    s_center            = xlEnter("CENTER");
    s_topleft           = xlEnter("TOP-LEFT");
    s_topright          = xlEnter("TOP-RIGHT");
    s_bottomleft        = xlEnter("BOTTOM-LEFT");
    s_bottomright       = xlEnter("BOTTOM-RIGHT");
    s_none              = xlEnter("NONE");
    s_x                 = xlEnter("X");
    s_y                 = xlEnter("Y");
    s_both              = xlEnter("BOTH");

    /* 'widget' class */
    c_widget = xlClass("WIDGET",NULL,"HANDLE");
    xlMethod(c_widget,"GET-TEXT",xGetWidgetText);
    xlMethod(c_widget,"SET-TEXT!",xSetWidgetText);
    xlMethod(c_widget,"HIDE",xHideWindow);
    xlMethod(c_widget,"SHOW",xShowWindow);
    xlMethod(c_widget,"MOVE",xMoveWindow);
    xlXMethod(c_widget,"GET-WINDOW-RECT",xGetWindowRect);
    xlXMethod(c_widget,"GET-CLIENT-RECT",xGetClientRect);

    /* 'frame' class */
    c_frame = xlClass("FRAME", c_widget,"");
    xlMethod(c_frame,"INITIALIZE",xMakeFrame);

    /* 'scroll-bar' class */
    c_scrollBar = xlClass("SCROLL-BAR",c_widget,"");
    xlMethod(c_scrollBar,"INITIALIZE",xMakeScrollBar);

    /* 'slider' class */
    c_slider = xlClass("SLIDER",c_widget,"");
    xlMethod(c_slider,"INITIALIZE",xMakeSlider);

    /* 'push-button' class */
    c_pushButton = xlClass("PUSH-BUTTON",c_widget,"");
    xlMethod(c_pushButton,"INITIALIZE",xMakePushButton);

    /* 'radio-button' class */
    c_radioButton = xlClass("RADIO-BUTTON",c_widget,"");
    xlMethod(c_radioButton,"INITIALIZE",xMakeRadioButton);

    /* 'check-box' class */
    c_checkBox = xlClass("CHECK-BOX",c_widget,"");
    xlMethod(c_checkBox,"INITIALIZE",xMakeCheckBox);

    /* 'group-box' class */
    c_groupBox = xlClass("GROUP-BOX",c_frame,"");
    xlMethod(c_groupBox,"INITIALIZE",xMakeGroupBox);

    /* 'drop-down-menu' class */
    c_dropDownMenu = xlClass("DROP-DOWN-MENU",c_widget,"");
    xlMethod(c_dropDownMenu,"INITIALIZE",xMakeDropDownMenu);

    /* 'list-box' class */
    c_listBox = xlClass("LIST-BOX",c_widget,"");
    xlMethod(c_listBox,"INITIALIZE",xListBoxInitialize);
    xlMethod(c_listBox,"ADD-STRING!",xListBoxAddString);
    xlMethod(c_listBox,"SET-REDRAW!",xListBoxSetRedraw);
    xlMethod(c_listBox,"SELECTION-CHANGED",xListBoxSelectionChanged);
    xlMethod(c_listBox,"DOUBLE-CLICK",xListBoxDoubleClick);
    xlMethod(c_listBox,"GET-SELECTION",xListBoxGetSelection);
    xlMethod(c_listBox,"SET-SELECTION!",xListBoxSetSelection);
    xlMethod(c_listBox,"GET-ITEM-DATA",xListBoxGetItemData);
    xlMethod(c_listBox,"RESET!",xListBoxReset);

    /* 'static-text' class */
    c_staticText = xlClass("STATIC-TEXT",c_widget,"");
    xlMethod(c_staticText,"INITIALIZE",xMakeStaticText);
}

/* MakeWidget - make and initialize a new widget */
Widget *MakeWidget(FrameWidget *parent,int type,size_t size)
{
    Widget *widget;

    /* allocate the widget */
    if (!(widget = (Widget *)malloc(size)))
        return NULL;

    /* initialize the new widget */
    InitWidget(parent,widget,type);

    /* return the new widget */
    return widget;
}

/* MakeFrameWidget - make and initialize a new frame widget */
FrameWidget *MakeFrameWidget(FrameWidget *parent,int type,size_t size)
{
    FrameWidget *frame;

    /* allocate the widget */
    if (!(frame = (FrameWidget *)malloc(size)))
        return NULL;

    /* initialize the new widget */
    InitWidget(parent,(Widget *)frame,type);
    frame->widget.flags = 0;
    frame->style = 0;
    frame->menu = NULL;
    frame->newOriginP = TRUE;
    frame->leftMargin = 0;
    frame->rightMargin = 0;
    frame->topMargin = 0;
    frame->bottomMargin = 0;
    frame->nextChildID = 0;
    frame->children = NULL;
    frame->pNextChild = &frame->children;

    /* return the new widget */
    return frame;
}

/* InitWidget - initialize a widget */
void InitWidget(FrameWidget *parent,Widget *widget,int type)
{
    /* initialize */
    xlProtect(&widget->obj);
    widget->flags = 0;
    widget->handler = NULL;
    widget->type = type;
    widget->wnd = NULL;
    widget->font = NULL;
    widget->dc = NULL;
    widget->groupID = -1;
    widget->nextID = 0;
    widget->charBindings = NULL;
    widget->bindings = NULL;
    widget->parent = NULL;
    widget->next = NULL;
    widget->tips = NULL;

    /* add the widget as a child of its parent */
    if ((widget->parent = parent) != NULL) {
        widget->window = parent->widget.window;
        widget->window->widget.flags |= wfRecalc;
        widget->childID = ++parent->nextChildID;
        *parent->pNextChild = widget;
        parent->pNextChild = &widget->next;
    }

    /* top level widget */
    else {
        widget->window = (FrameWidget *)widget;
        widget->childID = 0;
    }
}

/* FreeWidget - remove a child widget from its parent and free it */
void FreeWidget(Widget *widget)
{
    FrameWidget *parent = widget->parent;
    if (parent) {
        Widget *child,**pChild = &parent->children;
        for (; (child = *pChild) != NULL; pChild = &child->next)
            if (child == widget) {
                if ((*pChild = child->next) == NULL)
                    parent->pNextChild = pChild;
                break;
            }
    }
    xlUnprotect(&widget->obj);
    if (widget->wnd != NULL)
        DestroyWindow(widget->wnd);
    free(widget);
}

/* MakeBinding - make a command binding */
UINT MakeBinding(Widget *widget,xlValue fcn)
{
    MessageBinding *binding;
    UINT id;

    /* allocate the binding data structure */
    if ((binding = (MessageBinding *)malloc(sizeof(MessageBinding))) == NULL)
        return 0;

    /* make sure there are more ids available */
    if ((id = ++widget->nextID) > 0xffff) {
        --widget->nextID;
        free(binding);
        return 0;
    }
    
    /* initialize the binding */
    binding->msg = id;
    xlProtect(&binding->fcn);
    binding->fcn = fcn;
    binding->next = widget->bindings;
    widget->bindings = binding;

    /* return the command id */
    return id;
}

/* RemoveBinding - remove a binding */
void RemoveBinding(Widget *widget,UINT msg)
{
    MessageBinding *binding,**pBinding = &widget->bindings;
    while ((binding = *pBinding) != NULL) {
        if (msg == binding->msg) {
            *pBinding = binding->next;
            xlUnprotect(&binding->fcn);
            break;
        }
        pBinding = &binding->next;
    }
}

/* CheckBindings - check for a command binding and call the handler */
int CheckBindings(Widget *widget,UINT msg)
{
    MessageBinding *binding = widget->bindings;
    while (binding) {
        if (msg == binding->msg) {
            if (binding->fcn != xlNil)
                xlCallFunction(NULL,0,binding->fcn,0);
            return TRUE;
        }
        binding = binding->next;
    }
    return FALSE;
}

/* xGetWidgetText - built-in function 'get-widget-text' */
static xlValue xGetWidgetText(void)
{
    char buf[1024];
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_widget);
    xlLastArg();

    /* get the window text */
    GetWindowText(GetWidget(obj)->wnd,buf,sizeof(buf));
    return xlMakeCString(buf);
}

/* xSetWidgetText - built-in function 'set-widget-text!' */
static xlValue xSetWidgetText(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_widget);
    xlVal = xlGetArgString();
    xlLastArg();

    /* set the window text */
    SetWindowText(GetWidget(obj)->wnd,xlGetString(xlVal));
    return obj;
}

/* xMoveWindow - built-in function 'move-window' */
static xlValue xMoveWindow(void)
{
    Widget *widget;
    xlValue obj;
    int x,y,w,h;

    /* parse the arguments */
    obj = xlGetArgInstance(c_widget);
    xlVal = xlGetArgFixnum(); x = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); y = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); w = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); h = (int)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the widget */
    widget = GetWidget(obj);

    /* set the resize flag for top level windows */
    if (TopLevelWindowP(widget))
        widget->flags |= wfResize;

    /* move the window */
    MoveWindow(widget->wnd,x,y,w,h,TRUE);
    return obj;
}

/* xShowWindow - built-in function 'show-window' */
static xlValue xShowWindow(void)
{
    Widget *widget;
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_widget);
    xlLastArg();

    /* get the widget */
    widget = GetWidget(obj);

    /* recalc before showing top level windows */
    if (TopLevelWindowP(widget)) {
        if (widget->flags & wfRecalc)
            PlaceWidgets((FrameWidget *)widget,TRUE);
    }

    /* show the window */
    ShowWindow(widget->wnd,SW_SHOW);
    return obj;
}

/* xHideWindow - built-in function 'hide-window' */
static xlValue xHideWindow(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_widget);
    xlLastArg();

    /* hide the window */
    ShowWindow(GetWidget(obj)->wnd,SW_HIDE);
    return obj;
}

/* xGetWindowRect - built-in function 'get-window-rect' */
static void xGetWindowRect(void)
{
    HWND parent,wnd;
    xlValue obj;
    RECT rect;

    /* parse the arguments */
    obj = xlGetArgInstance(c_widget);
    xlLastArg();

    /* get the window */
    wnd = GetWidget(obj)->wnd;

    /* get the window rect */
    GetWindowRect(wnd,&rect);

    /* check for a parent window */
    if ((parent = GetParent(wnd)) != NULL) {
        RECT pRect;
        GetWindowRect(parent,&pRect);
        rect.left -= pRect.left;
        rect.top -= pRect.top;
        rect.right -= pRect.left;
        rect.bottom -= pRect.top;
    }

    /* return the window rect */
    xlCheck(4);
    xlPush(xlMakeFixnum(rect.bottom));
    xlPush(xlMakeFixnum(rect.right));
    xlPush(xlMakeFixnum(rect.top));
    xlPush(xlMakeFixnum(rect.left));
    xlMVReturn(4);
}

/* xGetClientRect - built-in function 'get-client-rect' */
static void xGetClientRect(void)
{
    xlValue obj;
    RECT rect;

    /* parse the arguments */
    obj = xlGetArgInstance(c_widget);
    xlLastArg();

    /* get the client rect */
    GetClientRect(GetWidget(obj)->wnd,&rect);

    /* return the client rect */
    xlCheck(4);
    xlPush(xlMakeFixnum(rect.bottom));
    xlPush(xlMakeFixnum(rect.right));
    xlPush(xlMakeFixnum(rect.top));
    xlPush(xlMakeFixnum(rect.left));
    xlMVReturn(4);
}

/* xMakeFrame - built-in function 'make-frame' */
static xlValue xMakeFrame(void)
{
    FrameWidget *parent,*frame;
    WidgetPosition pos;
    xlValue obj;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_frame);
    parent = GetArgFrameWidget();
    GetWidgetPosition(&pos);

    /* allocate the widget data structure */
    if (!(frame = MakeFrameWidget(parent,wtFrame,sizeof(FrameWidget))))
        return xlNil;
    frame->widget.position = pos;

    /* initialize the widget */
    frame->newOriginP = FALSE;

    /* return the widget object */
    SetWidget(obj,(Widget *)frame);
    return obj;
}

/* xMakeScrollBar - built-in function 'make-scroll-bar' */
static xlValue xMakeScrollBar(void)
{
    xlFIXTYPE from,to,value;
    WidgetPosition pos;
    FrameWidget *parent;
    Widget *widget;
    xlValue obj;
    HWND wnd;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_scrollBar);
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_handler,xlNil,&xlVal);
    xlGetKeyFixnum(k_from,0,&from);
    xlGetKeyFixnum(k_to,100,&to);
    xlGetKeyFixnum(k_initialvalue,0,&value);
    GetWidgetPosition(&pos);

    /* allocate the widget data structure */
    if (!(widget = MakeWidget(parent,wtScrollBar,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* make the slider */
    wnd = CreateWindow("scrollbar",
                       NULL,
                       WS_CHILD | WS_VISIBLE | SBS_HORZ,
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

    /* set the slider range */
    SetScrollRange(wnd,SB_CTL,(int)from,(int)to,FALSE);
    SetScrollPos(wnd,SB_CTL,(int)value,TRUE);

    /* initialize the widget */
    widget->obj = xlVal;

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* xMakeSlider - built-in function 'make-slider' */
static xlValue xMakeSlider(void)
{
    xlFIXTYPE from,to,value;
    WidgetPosition pos;
    FrameWidget *parent;
    Widget *widget;
    xlValue obj;
    HWND wnd;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_slider);
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_handler,xlNil,&xlVal);
    xlGetKeyFixnum(k_from,0,&from);
    xlGetKeyFixnum(k_to,100,&to);
    xlGetKeyFixnum(k_initialvalue,0,&value);
    GetWidgetPosition(&pos);

    /* allocate the widget data structure */
    if (!(widget = MakeWidget(parent,wtSlider,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* make the slider */
    wnd = CreateWindow(TRACKBAR_CLASS,
                       NULL,
                       WS_CHILD | WS_VISIBLE | TBS_HORZ,
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

    /* set the slider range */
    SendMessage(wnd,TBM_SETRANGEMIN,(WPARAM)FALSE,(LPARAM)from);
    SendMessage(wnd,TBM_SETRANGEMAX,(WPARAM)FALSE,(LPARAM)to);
    SendMessage(wnd,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)value);

    /* initialize the widget */
    widget->obj = xlVal;

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* xMakePushButton - built-in function 'make-push-button' */
static xlValue xMakePushButton(void)
{
    FrameWidget *frame;
    xlValue obj;
    int type;

    /* parse the arguments */
    obj = xlGetArgInstance(c_pushButton);
    frame = GetArgFrameWidget();
    xlGetKeyArg(k_defaultp,xlFalse,&xlVal);

    /* make the button */
    type = xlVal == xlFalse ? wtPushButton : wtDefPushButton;
    return MakeButton(obj,frame,type,-1,FALSE);
}

/* xMakeRadioButton - built-in function 'make-radio-button' */
static xlValue xMakeRadioButton(void)
{
    FrameWidget *frame;
    xlFIXTYPE groupID;
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_radioButton);
    frame = GetArgFrameWidget();
    xlGetKeyFixnum(k_groupid,0,&groupID);

    /* make the button */
    return MakeButton(obj,frame,wtRadioButton,groupID,FALSE);
}

/* xMakeCheckBox - built-in function 'make-check-box' */
static xlValue xMakeCheckBox(void)
{
    FrameWidget *frame;
    xlValue obj;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_checkBox);
    frame = GetArgFrameWidget();

    /* make the check box */
    return MakeButton(obj,frame,wtCheckBox,-1,TRUE);
}

/* MakeButton - make a button */
static xlValue MakeButton(xlValue obj,FrameWidget *parent,int type,int groupID,int sendValueP)
{
    xlValue value,label,tabstopp;
    WidgetPosition pos;
    Widget *widget;
    DWORD style;
    HWND wnd;
    
    /* parse the arguments */
    xlGetKeyArg(k_label,xlNil,&label);
    xlGetKeyArg(k_initialvalue,xlFalse,&value);
    xlGetKeyArg(k_handler,xlNil,&xlVal);
    xlGetKeyArg(k_tabstopp,xlTrue,&tabstopp);
    GetWidgetPosition(&pos);

    /* allocate the widget data structure */
    if (!(widget = MakeWidget(parent,type,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* choose the style */
    switch (type) {
    case wtPushButton:
        style = BS_PUSHBUTTON;
        break;
    case wtDefPushButton:
        style = BS_DEFPUSHBUTTON;
        break;
    case wtRadioButton:
        style = BS_RADIOBUTTON;
        break;
    case wtCheckBox:
        style = BS_AUTOCHECKBOX;
        break;
    }

    /* handle tab stops */
    if (tabstopp != xlFalse)
        style |= WS_TABSTOP;
    
    /* make the button */
    wnd = CreateWindow("button",
                       xlStringP(label) ? xlGetString(label) : "Button",
                       WS_CHILD | WS_VISIBLE | style,
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

    /* set the font */
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(ANSI_VAR_FONT),0);
    
    /* set the initial value */
    SendMessage(wnd,BM_SETCHECK,value == xlFalse ? 0 : 1,0);

    /* store the widget data structure pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)widget);

    /* initialize the widget */
    widget->obj = xlVal;
    widget->sendValueP = sendValueP;
    widget->groupID = groupID;

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* xMakeGroupBox - built-in function 'make-group-box' */
static xlValue xMakeGroupBox(void)
{
    WidgetPosition pos;
    FrameWidget *parent,*frame;
    xlValue label,obj;
    HWND wnd;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_groupBox);
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_label,xlNil,&label);
    GetWidgetPosition(&pos);

    /* allocate the widget data structure */
    if (!(frame = MakeFrameWidget(parent,wtGroupBox,sizeof(FrameWidget))))
        return xlNil;
    frame->widget.position = pos;

    /* make the button */
    wnd = CreateWindow("button",
                       xlStringP(label) ? xlGetString(label) : "Group Box",
                       WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                       pos.x,               /* horizontal position */
                       pos.y,               /* vertical position */
                       pos.width,           /* width */
                       pos.height,          /* height */
                       GetParentWnd(parent),
                       (HMENU)frame->widget.childID,
                       hInstance,
                       NULL);

    /* make sure the widget was created */
    if ((frame->widget.wnd = wnd) == NULL) {
        FreeWidget((Widget *)frame);
        return xlNil;
    }

    /* set the font */
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(ANSI_VAR_FONT),0);
    
    /* store the widget data structure pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)frame);

    /* setup to filter messages */
    frame->widget.wndProc = (WNDPROC)SetWindowLong(frame->widget.wnd,GWL_WNDPROC,(LONG)GroupBoxFilter);

    /* initialize the widget */
    frame->leftMargin = 2;
    frame->rightMargin = 2;
    frame->topMargin = 14;
    frame->bottomMargin = 2;
    frame->widget.obj = xlNil;

    /* return the widget object */
    SetWidget(obj,(Widget *)frame);
    return obj;
}

/* xMakeDropDownMenu - built-in function 'make-drop-down-menu' */
static xlValue xMakeDropDownMenu(void)
{
    xlValue labels,initialValue,sortP,vscrollP,obj;
    WidgetPosition pos;
    FrameWidget *parent;
    xlFIXTYPE i = 0;
    Widget *widget;
    DWORD style;
    HWND wnd;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_dropDownMenu);
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_labellist,xlNil,&labels);
    xlGetKeyArg(k_initialvalue,xlNil,&initialValue);
    xlGetKeyArg(k_sortp,xlTrue,&sortP);
    xlGetKeyArg(k_vscrollp,xlFalse,&vscrollP);
    xlGetKeyArg(k_handler,xlNil,&xlVal);
    GetWidgetPosition(&pos);

    /* make sure the default value is a string */
    EnsureStringOrNil(initialValue);

    /* allocate the widget data structure */
    if (!(widget = MakeWidget(parent,wtDropDownMenu,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* setup the style */
    style = WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST;
    if (sortP != xlFalse)
        style |= CBS_SORT;
    if (vscrollP != xlFalse)
        style |= WS_VSCROLL;

    /* make the combo box */
    wnd = CreateWindow("combobox",
                       NULL,
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

    /* set the font */
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(ANSI_VAR_FONT),0);
    
    /* add labels to the list */
    while (xlConsP(labels)) {
        xlValue label = xlCar(labels);
        int index;

        /* check for a string */
        if (xlStringP(label))
            ;/* use the existing value of i */

        /* check for a (string . integer) */
        else if (xlConsP(label)) {
            if (xlStringP(xlCar(label))
            &&  xlFixnumP(xlCdr(label))) {
                i = xlGetFixnum(xlCdr(label));
                label = xlCar(label);
            }
            else {
                FreeWidget(widget);
                xlFmtError("expecting string or (string . integer): ~S",label);
            }
        }

        /* illegal entry */
        else {
            FreeWidget(widget);
            xlFmtError("expecting string or (string . integer): ~S",label);
        }

        /* add the entry */
        index = SendMessage(wnd,CB_ADDSTRING,0,(LPARAM)xlGetString(label));
        SendMessage(wnd,CB_SETITEMDATA,(WPARAM)index,(LPARAM)i++);

        /* on to next entry */
        labels = xlCdr(labels);
    }

    /* make sure the list ended correctly */
    if (labels != xlNil) {
        FreeWidget(widget);
        xlFmtError("expecting end of label list: ~S",labels);
    }

    /* set the initial value */
    if (initialValue == xlNil)
        SendMessage(wnd,CB_SETCURSEL,(WPARAM)0,(LPARAM)0);
    else {
        char *str = xlGetString(initialValue);
        int index = SendMessage(wnd,CB_FINDSTRINGEXACT,(WPARAM)-1,(LPARAM)str);
        if (index == CB_ERR) {
            FreeWidget(widget);
            xlFmtError("value not a list entry: ~S",initialValue);
        }
        SendMessage(wnd,CB_SETCURSEL,(WPARAM)index,(LPARAM)0);
    }

    /* store the widget data structure pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)widget);

    /* initialize the widget */
    widget->handler = DropDownMenuHandler;
    widget->obj = xlVal;

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* DropDownMenuHandler - handler function for drop down menus */
static int DropDownMenuHandler(Widget *widget,int fcn)
{
    HDC dc;
    switch (fcn) {
    case whfSize:
        widget->width = widget->position.width;
        widget->height = 20;
        if ((dc = GetDC(widget->wnd)) != NULL) {
            TEXTMETRIC metrics;
            if (GetTextMetrics(dc,&metrics))
                widget->height = metrics.tmHeight + 5;
            ReleaseDC(widget->wnd,dc);
        }
        return TRUE;
    }
    return FALSE;
}

/* xListBoxInitialize - built-in function (list-box 'initialize) */
static xlValue xListBoxInitialize(void)
{
    xlValue obj,sortP,vscrollP,borderP;
    WidgetPosition pos;
    FrameWidget *parent;
    Widget *widget;
    DWORD style;
    HWND wnd;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_sortp,xlTrue,&sortP);
    xlGetKeyArg(k_vscrollp,xlFalse,&vscrollP);
    xlGetKeyArg(k_borderp,xlTrue,&borderP);
    GetWidgetPosition(&pos);

    /* allocate the widget data structure */
    if (!(widget = MakeWidget(parent,wtListBox,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* setup style */
    style = WS_CHILD | WS_VISIBLE | LBS_NOTIFY;
    if (sortP != xlFalse)
        style |= LBS_SORT;
    if (vscrollP != xlFalse)
        style |= WS_VSCROLL;
    if (borderP != xlFalse)
        style |= WS_BORDER;

    /* make the combo box */
    wnd = CreateWindow("listbox",
                       NULL,
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

    /* set the font */
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(ANSI_FIXED_FONT),0);
    
    /* store the widget data structure pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)widget);

    /* initialize the widget */
    widget->obj = obj;

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* xListBoxAddString - built-in method (list-box 'add-string!) */
static xlValue xListBoxAddString(void)
{
    int index,storeDataP = FALSE;
    xlValue obj,str;
    Widget *widget;
    DWORD data;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    str = xlGetArgString();
    if (xlMoreArgsP()) {
        xlVal = xlGetArgFixnum();
        data = (DWORD)xlGetFixnum(xlVal);
        storeDataP = TRUE;
    }
    xlLastArg();

    /* get the widget */
    widget = GetWidget(obj);

    /* add the string */
    index = SendMessage(widget->wnd,LB_ADDSTRING,0,(LPARAM)xlGetString(str));

    /* store the data associated with the item */
    if (storeDataP)
        SendMessage(widget->wnd,LB_SETITEMDATA,(WPARAM)index,(LPARAM)data);

    /* return the index */
    return xlMakeFixnum((xlFIXTYPE)index);
}

/* xListBoxSetRedraw - built-in method (list-box 'set-redraw!) */
static xlValue xListBoxSetRedraw(void)
{
    xlValue obj;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    xlVal = xlGetArg();
    xlLastArg();

    /* set the redraw flag */
    SendMessage(GetWidget(obj)->wnd,WM_SETREDRAW,(WPARAM)(xlVal != xlFalse),0);

    /* return the object */
    return obj;
}

/* xListBoxSelectionChanged - built-in method (list-box 'selection-changed) */
static xlValue xListBoxSelectionChanged(void)
{
    xlValue obj;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    xlLastArg();

    /* return the object */
    return obj;
}

/* xListBoxDoubleClick - built-in method (list-box 'double-click) */
static xlValue xListBoxDoubleClick(void)
{
    xlValue obj;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    xlLastArg();

    /* return the object */
    return obj;
}

/* xListBoxGetSelection - built-in method (list-box 'get-selection) */
static xlValue xListBoxGetSelection(void)
{
    xlValue obj;
    int index;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    xlLastArg();

    /* get the selection */
    index = SendMessage(GetWidget(obj)->wnd,LB_GETCURSEL,0,0);

    /* return the selection */
    return index == LB_ERR ? xlNil : xlMakeFixnum((xlFIXTYPE)index);
}

/* xListBoxSetSelection - built-in method (list-box 'set-selection!) */
static xlValue xListBoxSetSelection(void)
{
    xlValue obj;
    int index;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    xlVal = xlGetArgFixnum(); index = (int)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the selection */
    index = SendMessage(GetWidget(obj)->wnd,LB_SETCURSEL,(WPARAM)index,0);

    /* return status */
    return index == LB_ERR ? xlFalse : xlTrue;
}

/* xListBoxGetItemData - built-in method (list-box 'get-item-data) */
static xlValue xListBoxGetItemData(void)
{
    xlValue obj;
    DWORD data;
    int index;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    xlVal = xlGetArgFixnum(); index = (int)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the selection */
    data = SendMessage(GetWidget(obj)->wnd,LB_GETITEMDATA,(WPARAM)index,0);

    /* return status */
    return xlMakeFixnum((xlFIXTYPE)data);
}

/* xListBoxReset - built-in method (list-box 'reset!) */
static xlValue xListBoxReset(void)
{
    xlValue obj;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_listBox);
    xlLastArg();

    /* reset the contents of the list box */
    SendMessage(GetWidget(obj)->wnd,LB_RESETCONTENT,0,0);

    /* return the object */
    return obj;
}

/* xMakeStaticText - built-in function 'make-static-text' */
static xlValue xMakeStaticText(void)
{
    WidgetPosition pos;
    FrameWidget *parent;
    xlValue label,obj;
    Widget *widget;
    HWND wnd;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_staticText);
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_label,xlNil,&label);
    GetWidgetPosition(&pos);

    /* allocate the widget data structure */
    if (!(widget = MakeWidget(parent,wtStaticText,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* make the button */
    wnd = CreateWindow("static",
                       xlStringP(label) ? xlGetString(label) : "Static Text",
                       WS_CHILD | WS_VISIBLE | SS_LEFT,
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

    /* set the font */
    SendMessage(wnd,WM_SETFONT,(WPARAM)GetStockObject(ANSI_VAR_FONT),0);
    
    /* store the widget data structure pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)widget);

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* GetWidgetPosition - get widget position information */
void GetWidgetPosition(WidgetPosition *p)
{
    xlFIXTYPE x,y,width,height;
    xlValue side,anchor,fill;
    xlGetKeyFixnum(k_x,CW_USEDEFAULT,&x);
    xlGetKeyFixnum(k_y,CW_USEDEFAULT,&y);
    xlGetKeyFixnum(k_width,CW_USEDEFAULT,&width);
    xlGetKeyFixnum(k_height,CW_USEDEFAULT,&height);
    xlGetKeyArg(k_side,s_top,&side);
    xlGetKeyArg(k_anchor,s_center,&anchor);
    xlGetKeyArg(k_fill,s_none,&fill);

    /* set the dimensions */
    p->x = (int)x;
    p->y = (int)y;
    p->width = (int)width;
    p->height = (int)height;

    /* set the alignment side */
    if (side == s_top)
        p->side = wsTop;
    else if (side == s_bottom)
        p->side = wsBottom;
    else if (side == s_left)
        p->side = wsLeft;
    else if (side == s_right)
        p->side = wsRight;
    else
        xlError("expecting 'top, 'bottom, 'left or 'right",side);

    /* set the anchor position */
    if (anchor == s_center)
        p->anchor = waCenter;
    else if (anchor == s_left)
        p->anchor = waLeft;
    else if (anchor == s_right)
        p->anchor = waRight;
    else if (anchor == s_top)
        p->anchor = waTop;
    else if (anchor == s_bottom)
        p->anchor = waBottom;
    else if (anchor == s_topleft)
        p->anchor = waTopLeft;
    else if (anchor == s_topright)
        p->anchor = waTopRight;
    else if (anchor == s_bottomleft)
        p->anchor = waBottomLeft;
    else if (anchor == s_bottomright)
        p->anchor = waBottomRight;
    else
        xlError("expecting 'center, 'left, 'right, 'top, 'bottom, 'top-left, 'top-right, 'bottom-left or 'bottom-right",anchor);
    
    /* set the fill */
    if (fill == s_none)
        p->fill = wfNone;
    else if (fill == s_x)
        p->fill = wfX;
    else if (fill == s_y)
        p->fill = wfY;
    else if (fill == s_both)
        p->fill = wfBoth;
    else
        xlError("expecting 'none, 'x, 'y, or 'both",fill);
}

/* GroupBoxFilter - message filter for group box controls */
LRESULT CALLBACK GroupBoxFilter(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    Widget *widget = (Widget *)GetWindowLong(wnd,GWL_USERDATA);

    /* dispatch on the message type */
    switch (msg) {
    case WM_COMMAND:
        return SendMessage(GetParentWnd(widget->parent),msg,wParam,lParam);
    }
    
    /* call the original window procedure to handle other messages */
    return (*widget->wndProc)(wnd,msg,wParam,lParam);
}

/* ControlScroll - handle the WM_HSCROLL message */
LRESULT ControlScroll(HWND wnd,int part,int value)
{
    Widget *widget = (Widget *)GetWindowLong(wnd,GWL_USERDATA);
    switch (widget->type) {
    case wtScrollBar:
        return ControlScrollBar(widget,part,value);
    case wtSlider:
        return ControlSlider(widget,part,value);
    }
    return 0; /* never reached */
}

/* ControlScrollBar - handle the WM_HSCROLL message */
static LRESULT ControlScrollBar(Widget *widget,int part,int value)
{
    HWND wnd = (HWND)widget->wnd;
    SCROLLINFO info;
    int delta;
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    GetScrollInfo(wnd,SB_CTL,&info);
    switch (part) {
    case SB_LINEUP:
        delta = -1;
        break;
    case SB_LINEDOWN:
        delta = 1;
        break;
    case SB_PAGEUP:
        delta = -10;
        break;
    case SB_PAGEDOWN:
        delta = 10;
        break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        delta = info.nTrackPos - info.nPos;
        break;
    default:
        delta = 0;
        break;
    }
    if (delta) {
        if ((value = info.nPos + delta) < info.nMin)
            value = info.nMin;
        else if (value > info.nMax)
            value = info.nMax;
        if (value != info.nPos) {
            SetScrollPos(wnd,SB_CTL,value,TRUE);
            if (widget->obj != xlNil)
                xlCallFunction(NULL,0,widget->obj,1,xlMakeFixnum(value));
        }
    }
    return 0;
}

/* ControlSlider - handle the WM_HSCROLL message */
static LRESULT ControlSlider(Widget *widget,int part,int value)
{
    HWND wnd = (HWND)widget->wnd;
    int from,to,position;
    switch (part) {
    case TB_THUMBTRACK:
    case TB_THUMBPOSITION:
    case TB_ENDTRACK:
        from = SendMessage(wnd,TBM_GETRANGEMIN,(WPARAM)0,(LPARAM)0);
        to = SendMessage(wnd,TBM_GETRANGEMAX,(WPARAM)0,(LPARAM)0);
        position = SendMessage(wnd,TBM_GETPOS,(WPARAM)0,(LPARAM)0);
        if (position < from)
            position = from;
        else if (position > to)
            position = to;
        SendMessage(wnd,TBM_SETPOS,(WPARAM)TRUE,(LPARAM)position);
        if (widget->obj != xlNil)
            xlCallFunction(NULL,0,widget->obj,1,xlMakeFixnum(position));
        break;
    }
    return 0;
}

/* GetParentWnd - get the parent window of a widget */
HWND GetParentWnd(FrameWidget *frame)
{
    while (frame && !frame->widget.wnd)
        frame = frame->widget.parent;
    return frame ? frame->widget.wnd : NULL;
}

/* GetWidget - get the widget structure of a widget object */
Widget *GetWidget(xlValue obj)
{
    xlValue handle = xlGetIVar(obj,widgetHANDLE);
    if (!xlForeignPtrP(handle) || xlGetFPType(handle) != s_widget)
        xlError("bad handle in widget object",handle);
    return (Widget *)xlGetFPtr(handle);
}

/* SetWidget - set the widget structure of a widget object */
void SetWidget(xlValue obj,Widget *widget)
{
    xlSetIVar(obj,widgetHANDLE,xlMakeForeignPtr(s_widget,widget));
}

/* GetArgFrameWidget - get a widget window argument */
FrameWidget *GetArgFrameWidget(void)
{
    return (FrameWidget *)GetWidget(xlGetArgInstance(c_frame));
}

/* TopLevelWindowP - top level window predicate */
static int TopLevelWindowP(Widget *widget)
{
    switch (widget->type) {
    case wtWindow:
        return TRUE;
    default:
        return FALSE;
    }
}

/* EnsureStringOrNil - ensure that an argument is a string */
void EnsureStringOrNil(xlValue arg)
{
    if (arg != xlNil && !xlStringP(arg))
        xlFmtError("Expecting a string or nil - ~S",arg);
}