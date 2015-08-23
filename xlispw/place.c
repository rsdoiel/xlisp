/* place.c - routines to place widgets in a frame */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <windows.h>
#include "xlisp.h"
#include "xlispw.h"
#include "widget.h"

/* prototypes */
static void ComputeWidgetSize(Widget *widget);
static void ComputeFrameSize(FrameWidget *frame);
static void PositionWidget(Widget *widget,int x,int y,int width,int height,int sizeP);
static void PositionFrame(FrameWidget *frame);
static int WidgetFrameP(Widget *widget);

/* PlaceWidgets - places widget in a window */
void PlaceWidgets(FrameWidget *frame,int sizeP)
{
    int width,height;
    RECT wRect;
    
    /* get the window position */
    GetWindowRect(frame->widget.wnd,&wRect);

    /* use the current size */
    if (!sizeP) {
        RECT cRect;
        GetClientRect(frame->widget.wnd,&cRect);
        frame->widget.position.width = cRect.right - cRect.left;
        frame->widget.position.height = cRect.bottom - cRect.top;
    }

    /* compute the size of the widget */
    ComputeWidgetSize((Widget *)frame);

    /* use the size we computed */
    if (sizeP) {
        width = frame->widget.width;
        height = frame->widget.height;
    }

    /* position the widget */
    if (!frame->widget.handler || !(*frame->widget.handler)((Widget *)frame,whfPlace))
        PositionWidget((Widget *)frame,wRect.left,wRect.top,width,height,sizeP);
}

/* ComputeWidgetSize - compute the size of a widget */
static void ComputeWidgetSize(Widget *widget)
{
    /* set widget size */
    if (!widget->handler || !(*widget->handler)(widget,whfSize)) {
        widget->width = widget->position.width;
        widget->height = widget->position.height;
    }

    /* dispatch on widget type */
    if (WidgetFrameP(widget))
        ComputeFrameSize((FrameWidget *)widget);
}

/* ComputeFrameSize - compute the size of a frame or window widget */
static void ComputeFrameSize(FrameWidget *frame)
{
    int width,height,maxWidth,maxHeight;
    int computingSizeP;
    Widget *child;

    /* initialize */
    width = height = maxWidth = maxHeight = 0;

    /* determine whether we need to compute the frame size */
    computingSizeP = frame->widget.width == CW_USEDEFAULT
                  || frame->widget.height == CW_USEDEFAULT;

    /* compute the minimum size required */
    for (child = frame->children; child != NULL; child = child->next) {

        /* compute the size of this child */
        ComputeWidgetSize(child);

        /* compute the frame size if it was not specified */
        switch (child->position.side) {
        case wsLeft:
        case wsRight:
            if (computingSizeP) {
                if (frame->widget.width == CW_USEDEFAULT
                &&  child->width != CW_USEDEFAULT)
                    width += child->width;
                if (frame->widget.height == CW_USEDEFAULT
                &&  child->height != CW_USEDEFAULT
                &&  child->height > maxHeight)
                    maxHeight = child->height;
                if (height > maxHeight)
                    maxHeight = height;
                height = 0;
            }
            break;
        case wsTop:
        case wsBottom:
            if (computingSizeP) {
                if (frame->widget.height == CW_USEDEFAULT
                &&  child->height != CW_USEDEFAULT)
                    height += child->height;
                if (frame->widget.width == CW_USEDEFAULT
                &&  child->width != CW_USEDEFAULT
                &&  child->width > maxWidth)
                    maxWidth = child->width;
                if (width > maxWidth)
                    maxWidth = width;
                width = 0;
            }
            break;
        }
    }

    /* handle the last row and column */
    if (computingSizeP) {

        /* check the last run */
        if (width > maxWidth)
            maxWidth = width;
        if (height > maxHeight)
            maxHeight = height;

        /* return the dimensions */
        if (frame->widget.width == CW_USEDEFAULT) {
            frame->widget.width = frame->leftMargin
                                + maxWidth
                                + frame->rightMargin;
        }
        if (frame->widget.height == CW_USEDEFAULT) {
            frame->widget.height = frame->topMargin
                                 + maxHeight
                                 + frame->bottomMargin;
        }
    }
}

/* PositionWidget - position a widget in a window */
static void PositionWidget(Widget *widget,int x,int y,int width,int height,int sizeP)
{
    int extraWidth = width - widget->width;
    int extraHeight = height - widget->height;

    /* adjust widget size */
    if (widget->position.fill & wfX) {
        widget->width += extraWidth;
        extraWidth = 0;
    }
    if (widget->position.fill & wfY) {
        widget->height += extraHeight;
        extraHeight = 0;
    }
    
    /* adjust widget position */
    switch (widget->position.anchor) {
    case waCenter:
        x += extraWidth / 2;
        y += extraHeight / 2;
        break;
    case waLeft:
        y += extraHeight / 2;
        break;
    case waRight:
        x += extraWidth;
        y += extraHeight / 2;
        break;
    case waTop:
        x += extraWidth / 2;
        break;
    case waBottom:
        x += extraWidth / 2;
        y += extraHeight;
        break;
    case waTopLeft:
        /* no adjustment */
        break;
    case waTopRight:
        x += extraWidth;
        break;
    case waBottomLeft:
        y += extraHeight;
        break;
    case waBottomRight:
        x += extraWidth;
        y += extraHeight;
        break;
    }
    
    /* set the widget position */
    widget->x = x;
    widget->y = y;

    /* compute widget position */
    if (WidgetFrameP(widget))
        PositionFrame((FrameWidget *)widget);

    /* move widget */
    if (sizeP && widget->wnd) {

        /* compute display size */
        if (!widget->handler || !(*widget->handler)(widget,whfDisplaySize)) {
            widget->displayWidth = widget->width;
            widget->displayHeight = widget->height;
        }

        /* move the widget */
        MoveWindow(widget->wnd,widget->x,
                               widget->y,
                               widget->displayWidth,
                               widget->displayHeight,
                               TRUE);
    }
}

/* PositionFrame - position widgets within a frame */
static void PositionFrame(FrameWidget *frame)
{
    int x,y,xMax,yMax,rowP,expandCount,expandAmount;
    Widget *start,*end,*child,*prevChild;

    /* initialize */
    if (frame->newOriginP) {
        x = frame->leftMargin;
        y = frame->topMargin;
    }
    else {
        x = frame->widget.x + frame->leftMargin;
        y = frame->widget.y + frame->topMargin;
    }
    xMax = x + frame->widget.width - frame->leftMargin - frame->rightMargin;
    yMax = y + frame->widget.height - frame->topMargin - frame->bottomMargin;

    /* initialize */
    child = frame->children;
    prevChild = NULL;

    /* position each child */
    while (child) {

        /* initialize run */
        start = end = NULL;
        expandCount = 0;

        /* find the next run */
        while (child) {
            
            /* start a new run or extend the current run */
            switch (child->position.side) {
            case wsLeft:
            case wsRight:
                if (start) {
                    if (!rowP) {
                        end = prevChild;
                        break;
                    }
                }
                else {
                    expandAmount = xMax - x;
                    start = child;
                    rowP = TRUE;
                }
                if (child->position.fill &wfX)
                    ++expandCount;
                if (child->width != CW_USEDEFAULT)
                    expandAmount -= child->width;
                break;
            case wsTop:
            case wsBottom:
                if (start) {
                    if (rowP) {
                        end = prevChild;
                        break;
                    }
                }
                else {
                    expandAmount = yMax - y;
                    start = child;
                    rowP = FALSE;
                }
                if (child->position.fill & wfY)
                    ++expandCount;
                if (child->height != CW_USEDEFAULT)
                    expandAmount -= child->height;
                break;
            }

            /* move on to the next child */
            prevChild = child;
            child = child->next;
        }

        /* position the current run */
        if (start) {
            
            /* handle the last run */
            if (!end)
                end = prevChild;

            /* compute the size of each expandable widget */
            if (expandCount)
                expandAmount /= expandCount;

            /* position each widget in the run */
            for (;;) {

                /* position widget based on alignment */
                switch (start->position.side) {
                case wsLeft:
                    if (start->position.fill & wfX) {
                        if (start->width == CW_USEDEFAULT)
                            start->width = expandAmount;
                        else
                            start->width += expandAmount;
                        if (start->height == CW_USEDEFAULT)
                            start->height = yMax - y;
                    }
                    PositionWidget(start,x,y,start->width,yMax - y,TRUE);
                    x += start->width;
                    break;
                case wsRight:
                    if (start->position.fill & wfX) {
                        if (start->width == CW_USEDEFAULT)
                            start->width = expandAmount;
                        else
                            start->width += expandAmount;
                        if (start->height == CW_USEDEFAULT)
                            start->height = yMax - y;
                    }
                    xMax -= start->width;
                    PositionWidget(start,xMax,y,start->width,yMax - y,TRUE);
                    break;
                case wsTop:
                    if (start->position.fill & wfY) {
                        if (start->height == CW_USEDEFAULT)
                            start->height = expandAmount;
                        else
                            start->height += expandAmount;
                        if (start->width == CW_USEDEFAULT)
                            start->width = xMax - x;
                    }
                    PositionWidget(start,x,y,xMax - x,start->height,TRUE);
                    y += start->height;
                    break;
                case wsBottom:
                    if (start->position.fill & wfY) {
                        if (start->height == CW_USEDEFAULT)
                            start->height = expandAmount;
                        else
                            start->height += expandAmount;
                        if (start->width == CW_USEDEFAULT)
                            start->width = xMax - x;
                    }
                    yMax -= start->height;
                    PositionWidget(start,x,yMax,xMax - x,start->height,TRUE);
                    break;
                }

                /* check for the end of the run */
                if (start == end)
                    break;

                /* move on to the next child */
                start = start->next;
            }
        }
    }
}

/* WidgetFrameP - widget frame predicate */
static int WidgetFrameP(Widget *widget)
{
    int sts;
    switch (widget->type) {
    case wtWindow:
    case wtFrame:
    case wtGroupBox:
        sts = TRUE;
        break;
    default:
        sts = FALSE;
        break;
    }
    return sts;
}