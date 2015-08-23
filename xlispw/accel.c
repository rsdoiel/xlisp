/* accel.c - accelerator routines */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#include <string.h>
#include <ctype.h>
#include "widget.h"

/* accelerator list structure */
typedef struct AccelEntry AccelEntry;
struct AccelEntry {
    ACCEL accel;
    xlValue fcn;
    AccelEntry *next;
};

/* character name structure */
typedef struct CharName CharName;
struct CharName {
    char *name;
    int flags;
    WORD key;
};

/* character names */
static CharName charNames[] = {
{   "F1",       FVIRTKEY,   VK_F1       },
{   "F2",       FVIRTKEY,   VK_F2       },
{   "F3",       FVIRTKEY,   VK_F3       },
{   "F4",       FVIRTKEY,   VK_F4       },
{   "F5",       FVIRTKEY,   VK_F5       },
{   "F6",       FVIRTKEY,   VK_F6       },
{   "F7",       FVIRTKEY,   VK_F7       },
{   "F8",       FVIRTKEY,   VK_F8       },
{   "F9",       FVIRTKEY,   VK_F9       },
{   "F10",      FVIRTKEY,   VK_F10      },
{   "F11",      FVIRTKEY,   VK_F11      },
{   "F12",      FVIRTKEY,   VK_F12      },
{   "ENTER",    FVIRTKEY,   VK_RETURN   },
{   NULL,       0,          0           }
};

/* external variables */
extern xlValue c_window;

/* local variables */
static int nextID = 0;
static int tableChanged = FALSE;
static AccelEntry *entries = NULL;
static int entryCount = 0;
static HWND hAccelWnd = NULL;
static HACCEL hAccelTable = NULL;

/* prototypes */
static void MakeAcceleratorTable(void);
static xlValue xAddAccelerator(void);
static int ParseKeyString(char *key,ACCEL *accel);
static int ComparePrefix(char *start,char *end,char *prefix);
static int CompareKeyNames(char *key,char *key2);
static int FindCharacter(char *key,ACCEL *accel);

/* InitAccelerators - initialize the accelerators */
void InitAccelerators(void)
{
    xlMethod(c_window,"ADD-ACCELERATOR!",xAddAccelerator);
}

/* GetAcceleratorTable - get the current accelerator table */
int GetAcceleratorTable(HWND *pWnd,HACCEL *pAccel)
{
    if (tableChanged) {
        tableChanged = FALSE;
        MakeAcceleratorTable();
    }
    *pWnd = hAccelWnd;
    *pAccel = hAccelTable;
    return *pAccel != NULL;
}

/* MakeAcceleratorTable - make an accelerator table */
static void MakeAcceleratorTable(void)
{
    ACCEL *accelTable,*p;
    AccelEntry *e;
    
    /* destroy the previous table */
    if (hAccelTable) {
        DestroyAcceleratorTable(hAccelTable);
        hAccelTable = NULL;
    }

    /* just return if there are no entries */
    if (entryCount == 0)
        return;

    /* allocate an array of ACCEL structures */
    accelTable = (ACCEL *)malloc(sizeof(ACCEL) * entryCount);
    if (accelTable == NULL) {
        /* should report an error here! */
        return;
    }

    /* fill in the array */
    p = accelTable; 
    for (e = entries; e != NULL; e = e->next)
        *p++ = e->accel;

    /* make the accelerator table */
    hAccelTable = CreateAcceleratorTable(accelTable,entryCount);
    free(accelTable);
}

/* CheckAccelerators - check for an accelerator and call the handler */
int CheckAccelerators(UINT msg)
{
    AccelEntry *entry = entries;
    while (entry) {
        if (msg == entry->accel.cmd) {
            if (entry->fcn != xlNil)
                xlCallFunction(NULL,0,entry->fcn,0);
            return TRUE;
        }
        entry = entry->next;
    }
    return FALSE;
}

/* xAddAccelerator - built-in method (window 'add-accelerator!) */
static xlValue xAddAccelerator(void)
{
    AccelEntry *entry;
    xlValue obj,fcn;
    char *key;

    /* parse the argument list */
    obj = xlGetArgInstance(c_window);
    xlVal = xlGetArgString(); key = xlGetString(xlVal);
    fcn = xlGetArg();
    xlLastArg();

    /* allocate the entry structure */
    if ((entry = (AccelEntry *)malloc(sizeof(AccelEntry))) == NULL)
        return xlNil;

    /* make sure there are more ids available */
    if ((entry->accel.cmd = ++nextID) > 0xffff) {
        --nextID;
        free(entry);
        return xlNil;
    }

    /* parse the key string */
    if (!ParseKeyString(key,&entry->accel)) {
        free(entry);
        return xlNil;
    }
    
    /* initialize the binding */
    xlProtect(&entry->fcn);
    entry->fcn = fcn;
    entry->next = entries;
    entries = entry;
    ++entryCount;

    /* set the window associated with the accelerators */
    hAccelWnd = GetWidget(obj)->wnd;

    /* the table has changed */
    tableChanged = TRUE;

    /* return successfully */
    return xlMakeFixnum((xlFIXTYPE)entry->accel.cmd);
}

/* ParseKeyString - parse a key string and fill in an ACCEL structure */
static int ParseKeyString(char *key,ACCEL *accel)
{
    char *p;

    /* initialize the ACCEL structure */
    accel->fVirt = 0;

    /* find character prefixes */
    while ((p = strchr(key,'-')) != NULL) {
        if (ComparePrefix(key,p,"CTRL"))
            accel->fVirt |= FCONTROL;
        else if (ComparePrefix(key,p,"SHIFT"))
            accel->fVirt |= FSHIFT;
        else if (ComparePrefix(key,p,"ALT"))
            accel->fVirt |= FALT;
        else
            return FALSE;
        key = p + 1;
    }

    /* check for a single character */
    if (strlen(key) == 1) {
        accel->key = key[0];
        if (accel->fVirt & (FCONTROL | FSHIFT | FALT))
            accel->fVirt |= FVIRTKEY;
    }

    /* lookup a character name */
    else if (!FindCharacter(key,accel))
        return FALSE;

    /* return successfully */
    return TRUE;
}

/* ComparePrefix - compare a character prefix */
static int ComparePrefix(char *start,char *end,char *prefix)
{
    while (start < end) {
        int ch = *start++;
        if (islower(ch))
            ch = toupper(ch);
        if (ch != *prefix++)
            return FALSE;
    }
    return *prefix == '\0';
}

/* CompareKeyNames - compare two key names */
static int CompareKeyNames(char *key,char *key2)
{
    while (*key && *key2) {
        int ch = *key++;
        if (islower(ch))
            ch = toupper(ch);
        if (ch != *key2++)
            return FALSE;
    }
    return *key == '\0' && *key2 == '\0';
}

/* FindCharacter - find a character by name */
static int FindCharacter(char *key,ACCEL *accel)
{
    CharName *c = charNames;
    for (c = charNames; c->name; ++c)
        if (CompareKeyNames(key,c->name)) {
            accel->fVirt |= c->flags;
            accel->key = c->key;
            return TRUE;
        }
    return FALSE;
}