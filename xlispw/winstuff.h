/* winstuff.h - definitions for winstuff.c */
/*      Copyright (c) 1984-2002, by David Michael Betz
        All Rights Reserved
        See the included file 'license.txt' for the full license.
*/

#ifndef __WINSTUFF_H__
#define __WINSTUFF_H__

void SetupCallbacks(xlCallbacks *callbacks);
int InvokeXLISP(xlCallbacks *callbacks,int argc,char *argv[]);

#endif