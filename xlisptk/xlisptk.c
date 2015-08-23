/* xlisptk.c - dummy customization routines for tkstuff.c */

#include "xlisp.h"
#include "tkstuff.h"

/* ApplicationBanner - return the application banner string */
char *ApplicationBanner(void)
{
    return NULL;
}

/* ApplicationInitializationFile - return the name of the application initialization file */
char *ApplicationInitializationFile(void)
{
    return "xlisptk.lsp";
}

/* ApplicationEnter - enter application specific symbols */
void ApplicationEnter(void)
{
}

/* ApplicationFindSubr - find a subr or xsubr address */
xlValue (*ApplicationFindSubr(char *name))(void)
{
    return NULL;
}

#ifdef WIN32
#include <windows.h>
#include <direct.h>
#define DIRSEP '\\'
#define ENVSEP ';'
#else
#include <unistd.h>
#define DIRSEP '/'
#define ENVSEP ':'
#endif

#define MAX_LOADPATH_LEN 1024

static char *GetProgramDirectory(char *buf,int size);
static char *GetWorkingDirectory(char *buf,int size);

/* ApplicationLoadPath - return the path xlisp uses for loading files */
char *ApplicationLoadPath(void)
{
    char    stdPath[MAX_LOADPATH_LEN];
    char    *pch;
    
    static char xlispLoadPath[MAX_LOADPATH_LEN];
    char *next = xlispLoadPath;
    char *end = next + MAX_LOADPATH_LEN - 1;

    /* first put the current working directory */
    if ( !GetWorkingDirectory( next, end - next ))
        xlispLoadPath[0] = '\0';
    next += strlen( next );

    // first try using XLPATH
    if ( (pch = getenv( "XLPATH" )) != NULL )  {
        if ( xlispLoadPath[0] && next < end )
            *next++ = ENVSEP;
        strncat( next, pch, end - next );
    }

    // if that fails, use the program directory
    else {
        
        // try using the directory where the executable was found
        if ( GetProgramDirectory( stdPath, MAX_LOADPATH_LEN )) {

            // remove \bin from the stdPath
            if ( (pch = strrchr( stdPath, DIRSEP )) != NULL )
                *pch = '\0';

            // add a separator if appending to existing path
            if ( xlispLoadPath[0] && next < end )
                *next++ = ENVSEP;

            // append the xlisp directory
            strncat( next, stdPath, end - next );
        }
    }

    return xlispLoadPath;
}

/* GetProgramDirectory - get the directory containing the program */
static char *GetProgramDirectory(char *buf,int size)
{
#ifdef WIN32
    long    pathLen;
    char    *pch;

    // set the standard search path from current executable location
    pathLen = SearchPath(   NULL, "xlisp.dll", NULL, size, 
                            buf, &pch );

    // check to see if the call succeeded
    if ( ( 0 == pathLen ) )
        return NULL;

    // remove exe name from stdPath
    *( --pch ) = '\0';

    // return the buffer
    return buf;
#else
    return NULL;
#endif
}

/* GetWorkingDirectory - get the working directory */
static char *GetWorkingDirectory(char *buf,int size)
{
#ifdef WIN32
    return _getcwd( buf, size );
#else
    return getcwd( buf, size );
#endif
}


