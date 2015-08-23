/* xlispw.h - definitions for xlispw.c */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#ifndef __XLISPW_H__
#define __XLISPW_H__

/* globals */
extern HINSTANCE hInstance;
extern HWND hMainWnd;
extern char appName[];

/* prototypes */
void ShowErrorMessage(DWORD err);

#endif