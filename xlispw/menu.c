/* menu.c - menu routines for xlispw */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <windows.h>
#include "xlisp.h"
#include "xlispw.h"
#include "widget.h"

/* classes */
xlValue c_menu;

/* instance variable offsets */
#define menuHANDLE  xlFIRSTIVAR

/* locals */
static xlValue s_menu;
static xlValue s_prepare;
static MessageBinding *bindings = NULL;
static int nextID = 0;

/* function handlers */
static xlValue xMenuInitialize(void);
static xlValue xAppendString(void);
static xlValue xAppendPopup(void);
static xlValue xAppendSeparator(void);
static xlValue xCheckItem(void);
static xlValue xItemCheckedP(void);
static xlValue xPrepare(void);

/* prototypes */
static UINT MakeMenuBinding(xlValue fcn);
static void RemoveMenuBinding(UINT msg);
static void SetMenuHandle(xlValue obj,Menu *menu);

/* InitMenus - initialize the menu routines */
void InitMenus(void)
{
    /* enter the built-in symbols */
    s_menu = xlEnter("MENU");
    s_prepare = xlEnter("PREPARE");

    /* 'menu' class */
    c_menu = xlClass("MENU",NULL,"HANDLE");
    xlMethod(c_menu,"INITIALIZE",xMenuInitialize);
    xlMethod(c_menu,"APPEND-STRING!",xAppendString);
    xlMethod(c_menu,"APPEND-POPUP!",xAppendPopup);
    xlMethod(c_menu,"APPEND-SEPARATOR!",xAppendSeparator);
    xlMethod(c_menu,"CHECK-ITEM!",xCheckItem);
    xlMethod(c_menu,"ITEM-CHECKED?",xItemCheckedP);
    xlMethod(c_menu,"PREPARE",xPrepare);
}

/* xMenuInitialize - built-in method (menu 'initialize) */
static xlValue xMenuInitialize(void)
{
    xlValue obj;
    Menu *menu;

    /* parse the arguments */
    obj = xlGetArgInstance(c_menu);
    xlLastArg();

    /* allocate the menu structure */
    if ((menu = (Menu *)malloc(sizeof(Menu))) == NULL)
        return xlNil;

    /* initialize the menu structure */
    xlProtect(&menu->obj);
    menu->obj = obj;
    menu->parent = NULL;
    menu->children = NULL;
    menu->next = NULL;

    /* create the menu */
    if ((menu->h = CreateMenu()) == NULL) {
        free(menu);
        return xlNil;
    }

    /* return the menu */
    SetMenuHandle(obj,menu);
    return obj;
}

/* xAppendString - built-in method (menu 'append-string!) */
static xlValue xAppendString(void)
{
    xlValue obj;
    char *entry;
    UINT id;

    /* parse the arguments */
    obj = xlGetArgInstance(c_menu);
    xlVal = xlGetArgString(); entry = xlGetString(xlVal);
    xlVal = xlGetArg();
    xlLastArg();

    /* make a command binding */
    if ((id = MakeMenuBinding(xlVal)) == 0)
        return xlFalse;

    /* append the meny entry */
    if (!AppendMenu(GetMenuHandle(obj)->h,MF_STRING,id,entry)) {
        RemoveMenuBinding(id);
        return xlNil;
    }

    /* return successfully */
    return xlMakeFixnum((xlFIXTYPE)id);
}

/* xAppendPopup - built-in method (menu 'append-popup!) */
static xlValue xAppendPopup(void)
{
    Menu *menu,*popup;
    char *entry;
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_menu);
    xlVal = xlGetArgString(); entry = xlGetString(xlVal);
    xlVal = xlGetArgInstance(c_menu);
    xlLastArg();

    /* get the menus */
    menu = GetMenuHandle(obj);
    popup = GetMenuHandle(xlVal);

    /* make sure the popup isn't already a child */
    if (popup->parent)
        return xlFalse;

    /* append the meny entry */
    if (!AppendMenu(menu->h,MF_POPUP,(UINT)popup->h,entry))
        return xlFalse;
    
    /* add the popup as a child of the menu */
    popup->parent = menu;
    popup->next = menu->children;
    menu->children = popup;

    /* return successfully */
    return xlTrue;
}

/* xAppendSeparator - built-in function (menu 'append-separator!) */
static xlValue xAppendSeparator(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_menu);
    xlLastArg();

    /* append the separator */
    return AppendMenu(GetMenuHandle(obj)->h,MF_SEPARATOR,0,NULL) ? xlTrue : xlFalse;
}

/* xCheckItem - built-in function (menu 'check-item!) */
static xlValue xCheckItem(void)
{
    xlValue obj;
    DWORD prev;
    UINT id;

    /* parse the arguments */
    obj = xlGetArgInstance(c_menu);
    xlVal = xlGetArgFixnum(); id = (UINT)xlGetFixnum(xlVal);
    xlVal = xlGetArg();
    xlLastArg();

    /* check the menu item */
    prev = CheckMenuItem(GetMenuHandle(obj)->h,id,xlVal == xlFalse ? MF_UNCHECKED
                                                                   : MF_CHECKED);
    /* return the previous menu item state */
    return prev == MF_CHECKED ? xlTrue : xlFalse;
}

/* xItemCheckedP - build-in function (menu 'item-checked?) */
static xlValue xItemCheckedP(void)
{
    MENUITEMINFO info;
    xlValue obj;
    UINT id;

    /* parse the arguments */
    obj = xlGetArgInstance(c_menu);
    xlVal = xlGetArgFixnum(); id = (UINT)xlGetFixnum(xlVal);
    xlLastArg();

    /* check the menu item */
    info.cbSize = sizeof(info);
    info.fMask = MIIM_STATE;
    if (!GetMenuItemInfo(GetMenuHandle(obj)->h,id,FALSE,&info))
        return xlFalse;

    /* return the menu item state */
    return info.fState == MFS_CHECKED ? xlTrue : xlFalse;
}

/* xPrepare - build-in function (menu 'prepare) */
static xlValue xPrepare(void)
{
    xlValue obj;

    /* parse the arguments */
    obj = xlGetArgInstance(c_menu);
    xlLastArg();

    /* return the object */
    return obj;
}

/* MakeMenuBinding - make a command binding */
static UINT MakeMenuBinding(xlValue fcn)
{
    MessageBinding *binding;
    UINT id;

    /* allocate the binding data structure */
    if ((binding = (MessageBinding *)malloc(sizeof(MessageBinding))) == NULL)
        return 0;

    /* make sure there are more ids available */
    if ((id = ++nextID) > 0xffff) {
        --nextID;
        free(binding);
        return 0;
    }
    
    /* initialize the binding */
    binding->msg = id;
    xlProtect(&binding->fcn);
    binding->fcn = fcn;
    binding->next = bindings;
    bindings = binding;

    /* return the command id */
    return id;
}

/* RemoveMenuBinding - remove a binding */
static void RemoveMenuBinding(UINT msg)
{
    MessageBinding *binding,**pBinding = &bindings;
    while ((binding = *pBinding) != NULL) {
        if (msg == binding->msg) {
            *pBinding = binding->next;
            xlUnprotect(&binding->fcn);
            break;
        }
        pBinding = &binding->next;
    }
}

/* CheckMenuBindings - check for a command binding and call the handler */
int CheckMenuBindings(UINT msg)
{
    MessageBinding *binding = bindings;
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

/* PrepareMenu - prepare a menu for display */
int PrepareMenu(Menu *menu,HMENU hMenu)
{
    /* prepare this menu */
    if (hMenu == menu->h) {

        /* send the 'prepare message to the matching menu */
        xlSendMessage(NULL,0,menu->obj,s_prepare,0);

        /* return successfully */
        return TRUE;
    }

    /* search for the menu amongst the children of this menu */
    else {
        for (menu = menu->children; menu != NULL; menu = menu->next)
            if (PrepareMenu(menu,hMenu))
                return TRUE;

        /* menu not found */
        return FALSE;
    }
}

/* GetMenuHandle - get the menu structure of a menu object */
Menu *GetMenuHandle(xlValue obj)
{
    xlValue handle = xlGetIVar(obj,menuHANDLE);
    if (!xlForeignPtrP(handle) || xlGetFPType(handle) != s_menu)
        xlError("bad handle in menu object",handle);
    return (Menu *)xlGetFPtr(handle);
}

/* SetMenuHandle - set the menu structure of a menu object */
static void SetMenuHandle(xlValue obj,Menu *menu)
{
    xlSetIVar(obj,menuHANDLE,xlMakeForeignPtr(s_menu,(void *)menu));
}

