/* canvas.c - canvas class */
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

/* classes */
static xlValue c_canvas;

/* keywords and symbols */
static xlValue k_borderp;
static xlValue s_draw;

/* externals */
extern xlValue c_widget;

/* function handlers */
static xlValue xCanvasInitialize(void);
static xlValue xSetFont(void);
static xlValue xGetPixel(void);
static xlValue xSetPixel(void);
static xlValue xSetPixelRow(void);
static xlValue xTextOut(void);
static xlValue xStreamOut(void);
static xlValue xDraw(void);

/* prototypes */
static xlValue MakeWindow(xlValue obj,FrameWidget *parent,char *title,char *className,DWORD style);
LRESULT CALLBACK CanvasWndProc(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam);
static void DrawCanvas(Widget *widget);
static Widget *GetWidgetWithDC(xlValue obj);

/* InitCanvas - initialize the canvas class */
void InitCanvas(void)
{
    WNDCLASSEX wndClass;

    /* setup the window class structure */
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wndClass.lpfnWndProc = CanvasWndProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = hInstance;
    wndClass.hIcon = LoadIcon(NULL,IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL,IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "xlcanvas";
    wndClass.hIconSm = LoadIcon(NULL,IDI_APPLICATION);

    /* register the window class */
    RegisterClassEx(&wndClass);

    /* enter the built-in symbols */
    k_borderp           = xlEnterKeyword("BORDER?");
    s_draw              = xlEnter("DRAW");

    /* 'canvas' class */
    c_canvas = xlClass("CANVAS",c_widget,"");
    xlMethod(c_canvas,"INITIALIZE",xCanvasInitialize);
    xlMethod(c_canvas,"SET-FONT!",xSetFont);
    xlMethod(c_canvas,"GET-PIXEL",xGetPixel);
    xlMethod(c_canvas,"SET-PIXEL!",xSetPixel);
    xlMethod(c_canvas,"SET-PIXEL-ROW!",xSetPixelRow);
    xlMethod(c_canvas,"TEXT-OUT",xTextOut);
    xlMethod(c_canvas,"STREAM-OUT",xStreamOut);
    xlMethod(c_canvas,"DRAW",xDraw);
}

/* xCanvasInitialize - built-in method (canvas 'initialize) */
static xlValue xCanvasInitialize(void)
{
    xlValue obj,borderp;
    FrameWidget *parent;
    WidgetPosition pos;
    Widget *widget;
    DWORD style;
    HWND wnd;
    
    /* parse the arguments */
    obj = xlGetArgInstance(c_canvas);
    parent = GetArgFrameWidget();
    xlGetKeyArg(k_borderp,xlTrue,&borderp);
    GetWidgetPosition(&pos);
    
    /* allocate the window data structure */
    if (!(widget = MakeWidget(parent,wtCanvas,sizeof(Widget))))
        return xlNil;
    widget->position = pos;

    /* setup the window style */
    style = WS_CHILD | WS_VISIBLE;
    if (borderp != xlFalse)
        style |= WS_BORDER;

    /* create the window */
    wnd = CreateWindow("xlcanvas",              /* class name */
                       NULL,                    /* window name */
                       style,                   /* window style */
                       pos.x,                   /* horizontal position */
                       pos.y,                   /* vertical position */
                       pos.width,               /* width */
                       pos.height,              /* height */
                       GetParentWnd(parent),    /* parent */
                       NULL,                    /* menu */
                       hInstance,               /* application instance */
                       NULL);                   /* window creation data */

    /* make sure the window was created */
    if (wnd == NULL) {
        FreeWidget(widget);
        return xlNil;
    }

    /* store the widget pointer */
    SetWindowLong(wnd,GWL_USERDATA,(long)widget);
    widget->wnd = wnd;

    /* initialize the widget */
    widget->obj = obj;

    /* return the widget object */
    SetWidget(obj,widget);
    return obj;
}

/* xSetFont - built-in method (canvas 'set-font!) */
static xlValue xSetFont(void)
{
    LOGFONT fontInfo;
    xlValue obj,name;
    Widget *widget;
    HFONT font;
    LONG size;

    /* parse the arguments */
    obj = xlGetArgInstance(c_canvas);
    name = xlGetArgString();
    xlVal = xlGetArgFixnum(); size = (LONG)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the widget structure */
    widget = GetWidget(obj);

    /* setup the font info */
    memset(&fontInfo,0,sizeof(fontInfo));
    fontInfo.lfHeight = size;
    fontInfo.lfWeight = 700; /* bold */
    strncpy(fontInfo.lfFaceName,xlGetString(name),LF_FACESIZE - 1);
    fontInfo.lfFaceName[LF_FACESIZE - 1] = '\0';

    /* get a handle to the font */
    if (!(font = CreateFontIndirect(&fontInfo)))
        return xlFalse;

    /* delete the previous font */
    if (widget->font)
        DeleteObject(widget->font);

    /* save the new font */
    widget->font = font;

    /* return successfully */
    return xlTrue; 
}

/* xGetPixel - built-in method (canvas 'get-pixel) */
static xlValue xGetPixel(void)
{
    Widget *widget;
    xlValue obj;
    int x,y;

    /* parse the argument list */
    obj = xlGetArgInstance(c_canvas);
    xlVal = xlGetArgFixnum(); x = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); y = (int)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the widget with a valid device context */
    if ((widget = GetWidgetWithDC(obj)) == NULL)
        return xlNil;

    /* return the pixel */
    return xlMakeFixnum((xlFIXTYPE)GetPixel(widget->dc,x,y));
}

/* xSetPixel - built-in method (canvas 'set-pixel!) */
static xlValue xSetPixel(void)
{
    Widget *widget;
    xlValue obj;
    int x,y,c;

    /* parse the argument list */
    obj = xlGetArgInstance(c_canvas);
    xlVal = xlGetArgFixnum(); x = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); y = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); c = (int)xlGetFixnum(xlVal);
    xlLastArg();

    /* get the widget with a valid device context */
    if ((widget = GetWidgetWithDC(obj)) == NULL)
        return xlNil;

    /* set the pixel */
    return xlMakeFixnum((xlFIXTYPE)SetPixel(widget->dc,x,y,c));
}

/* xSetPixelRow - built-in method (canvas 'set-pixel-row!) */
static xlValue xSetPixelRow(void)
{
    unsigned char *p,*lastP;
    Widget *widget;
    xlValue obj;
    int x,y;

    /* parse the argument list */
    obj = xlGetArgInstance(c_canvas);
    xlVal = xlGetArgFixnum(); y = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgString();
    xlLastArg();

    /* get the widget with a valid device context */
    if ((widget = GetWidgetWithDC(obj)) == NULL)
        return xlNil;

    /* get the string pointer */
    p = (unsigned char *)xlGetString(xlVal);
    lastP = p + xlGetSLength(xlVal) - 3;

    /* loop through the pixels in the string */
    for (x = 0; p <= lastP; ++x) {
        int r = *p++;
        int g = *p++;
        int b = *p++;
        SetPixelV(widget->dc,x,y,RGB(r,g,b));
    }

    /* return the object */
    return obj;
}

/* xTextOut - built-in method (canvas 'text-out) */
static xlValue xTextOut(void)
{
    Widget *widget;
    xlValue obj;
    int x,y;

    /* parse the argument list */
    obj = xlGetArgInstance(c_canvas);
    xlVal = xlGetArgFixnum(); x = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); y = (int)xlGetFixnum(xlVal);
    xlVal = xlGetArgString();
    xlLastArg();

    /* get the widget with a valid device context */
    if ((widget = GetWidgetWithDC(obj)) == NULL)
        return xlNil;

    /* add the string */
    TextOut(widget->dc,x,y,xlGetString(xlVal),xlGetSLength(xlVal));
    return obj;
}

#define DRAWBUFSIZE     256
#define LMARGIN         2
#define TMARGIN         2

/* xStreamOut - built-in method (canvas 'stream-out) */
static xlValue xStreamOut(void)
{
    int tmargin,lmargin,lineheight,ch,line = 0;
    char buf[DRAWBUFSIZE + 1],*p = buf;
    TEXTMETRIC metrics;
    Widget *widget;
    xlValue obj;

    /* parse the argument list */
    obj = xlGetArgInstance(c_canvas);
    xlVal = xlGetArgInputPort();
    xlLastArg();

    /* get the widget with a valid device context */
    if ((widget = GetWidgetWithDC(obj)) == NULL)
        return xlNil;

    /* get the text metrics for this window */
    GetTextMetrics(widget->dc,&metrics);
    tmargin = TMARGIN;
    lmargin = LMARGIN;
    lineheight = metrics.tmHeight + metrics.tmExternalLeading;

    /* display the output function output */
    while ((ch = xlGetC(xlVal)) != EOF) {
        if (ch == '\n') {
            if (p > buf) {
                TextOut(widget->dc,lmargin,tmargin + lineheight * line,buf,p - buf);
                p = buf;
            }
            ++line;
        }
        else if (p < &buf[DRAWBUFSIZE])
            *p++ = ch;
    }

    /* output the remaining buffer */
    if (p > buf)
        TextOut(widget->dc,lmargin,tmargin + lineheight * line,buf,p - buf);

    /* return the object */
    return obj;
}

/* xDraw - built-in method (canvas 'draw) */
static xlValue xDraw(void)
{
    xlVal = xlGetArgInstance(c_canvas);
    xlLastArg();
    return xlVal;
}

/* CanvasWndProc - canvas window procedure */
LRESULT CALLBACK CanvasWndProc(HWND wnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    Widget *widget = (Widget *)GetWindowLong(wnd,GWL_USERDATA);

    /* dispatch on the message type */
    if (widget) {
        switch (msg) {
        case WM_PAINT:
            DrawCanvas(widget);
            return 0;
        }
    }

    /* call the default window procedure to handle other messages */
    return DefWindowProc(wnd,msg,wParam,lParam);
}

/* DrawCanvas - draw a canvas widget */
static void DrawCanvas(Widget *widget)
{
    PAINTSTRUCT ps;
    if (widget->dc)
        ReleaseDC(widget->wnd,widget->dc);
    if ((widget->dc = BeginPaint(widget->wnd,&ps)) != NULL) {
        SelectObject(widget->dc,(HBRUSH)GetStockObject(WHITE_BRUSH));
        if (widget->font)
            SelectObject(widget->dc,widget->font);
        else {
            HFONT font = (HFONT)GetStockObject(ANSI_FIXED_FONT);
            if (font)
                SelectObject(widget->dc,font);
        }
        xlSendMessage(NULL,0,widget->obj,s_draw,0);
        EndPaint(widget->wnd,&ps);
    }
}

/* GetWidgetWithDC - get the widget with a valid device context */
static Widget *GetWidgetWithDC(xlValue obj)
{
    Widget *widget;

    /* get the widget */
    widget = GetWidget(obj);

    /* setup the device context */
    if (widget->dc == NULL) {
        if ((widget->dc = GetDC(widget->wnd)) == NULL)
            return NULL;
    }

    /* return the widget */
    return widget;
}