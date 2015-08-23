/* tkstuff.h - definitions for tkstuff.c */

/* $Id: tkstuff.h,v 1.3 2001/01/09 02:34:21 cheiny Exp $ */

#ifndef __TKSTUFF_H__
#define __TKSTUFF_H__

char *ApplicationBanner(void);
char *ApplicationInitializationFile(void);
void ApplicationEnter(void);
xlValue (*ApplicationFindSubr(char *name))(void);
char *ApplicationLoadPath(void);
void TclPathCopy(char *dst,char *src);

#endif
