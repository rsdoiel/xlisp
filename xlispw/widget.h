/* widget.h - definitions for the widget support in xlispw */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#ifndef __WIDGET_H__
#define __WIDGET_H__

#include <windows.h>
#include "xlisp.h"

/* data types */
typedef struct WidgetPosition WidgetPosition;
typedef struct Widget Widget;
typedef struct FrameWidget FrameWidget;
typedef struct CharBinding CharBinding;
typedef struct MessageBinding MessageBinding;
typedef struct Menu Menu;
typedef struct ToolTipText ToolTipText;

/* widget types */
#define wtWindow        0
#define wtFrame         1
#define wtScrollBar     2
#define wtSlider        3
#define wtPushButton    4
#define wtDefPushButton 5
#define wtRadioButton   6
#define wtCheckBox      7
#define wtGroupBox      8
#define wtDropDownMenu  9
#define wtStaticText    10
#define wtEditText      11
#define wtRichEditText  12
#define wtCanvas        13
#define wtToolbar       14
#define wtListBox       15

/* character binding */
struct CharBinding {
    TCHAR ch;
    xlValue fcn;
    CharBinding *next;
};

/* message binding */
struct MessageBinding {
    UINT msg;
    xlValue fcn;
    MessageBinding *next;
};

/* widget side values */
#define wsLeft      1
#define wsRight     2
#define wsTop       3
#define wsBottom    4

/* widget anchor values */
#define waCenter        1
#define waLeft          2
#define waRight         3
#define waTop           4
#define waBottom        5
#define waTopLeft       6
#define waTopRight      7
#define waBottomLeft    8
#define waBottomRight   9

/* widget fill values */
#define wfNone          0
#define wfX             1
#define wfY             2
#define wfBoth          (wfX | wfY)

/* widget position structure */
struct WidgetPosition {
    int side;
    int anchor;
    int fill;
    xlFIXTYPE x;
    xlFIXTYPE y;
    xlFIXTYPE width;
    xlFIXTYPE height;
};

/* widget handler functions */
#define whfSize         1   /* determine widget size */
#define whfDisplaySize  2   /* determine widget display size */
#define whfPlace        3   /* place widget (if wfAutoPlace is set) */

/* widget handler */
typedef int (*WidgetHandler)(Widget *widget,int fcn);

/* widget flags */
#define wfRecalc        1
#define wfResize        2
#define wfAutoPlace     4

/* widget data structure */
struct Widget {
    int flags;
    FrameWidget *window;
    FrameWidget *parent;
    Widget *next;
    WidgetHandler handler;
    int type;
    WidgetPosition position;
    int x;
    int y;
    int width;
    int height;
    int displayWidth;
    int displayHeight;
    HWND wnd;
    HFONT font;
    WNDPROC wndProc;
    HDC dc;
    UINT nextID;
    CharBinding *charBindings;
    MessageBinding *bindings;
    xlValue obj;
    int childID;
    int groupID;        /* for radio buttons */
    int sendValueP;     /* for buttons */
    ToolTipText *tips;  /* for toolbars */
};

/* frame widget structure */
struct FrameWidget {
    Widget widget;
    DWORD style;
    Menu *menu;
    int newOriginP;
    int leftMargin;
    int rightMargin;
    int topMargin;
    int bottomMargin;
    int nextChildID;
    Widget *children;
    Widget **pNextChild;
};

/* menu structure */
struct Menu {
    HMENU h;
    xlValue obj;
    Menu *parent;
    Menu *children;
    Menu *next;
};

/* tool tip text structure */
struct ToolTipText {
    char *text;
    UINT id;
    ToolTipText *next;
};

/* from widget.c */
void InitWidgets(void);
Widget *MakeWidget(FrameWidget *parent,int type,size_t size);
FrameWidget *MakeFrameWidget(FrameWidget *parent,int type,size_t size);
void FreeWidget(Widget *widget);
UINT MakeBinding(Widget *widget,xlValue fcn);
void RemoveBinding(Widget *widget,UINT msg);
int CheckBindings(Widget *widget,UINT msg);
LRESULT ControlScroll(HWND wnd,int part,int value);
HWND GetParentWnd(FrameWidget *frame);
Widget *GetWidget(xlValue obj);
void SetWidget(xlValue obj,Widget *widget);
FrameWidget *GetArgFrameWidget(void);
void GetWidgetPosition(WidgetPosition *p);
void EnsureStringOrNil(xlValue arg);

/* from window.c */
void InitWindows(void);

/* from canvas.c */
void InitCanvas(void);

/* from menu.c */
void InitMenus(void);
int CheckMenuBindings(UINT msg);
int PrepareMenu(Menu *windowMenu,HMENU hMenu);
Menu *GetMenuHandle(xlValue obj);

/* from edit.c */
void InitEdit(void);
void DisplayLastError(void);

/* from toolbar.c */
void InitToolbars(void);
char *GetToolTipText(Widget *widget,UINT id);

/* from accel.c */
void InitAccelerators(void);
int GetAcceleratorTable(HWND *pWnd,HACCEL *pAccel);
int CheckAccelerators(UINT msg);

/* from listener.c */
void InitListener(void);
int ConsoleGetC(void);
void ConsoleFlushInput(void);
void ConsolePutC(int ch);
void ConsoleFlushOutput(void);
int ConsoleAtBOLP(void);
int ConsoleCheck(void);

/* from misc.c */
void InitMisc(void);

/* from place.c */
void PlaceWidgets(FrameWidget *widget,int sizeP);

#endif
