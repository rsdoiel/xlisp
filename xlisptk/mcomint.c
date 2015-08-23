/* mstuff.c - merlin specific routines */

#include "xlisp.h"
#include "mcom.h"

static xlValue xMComInitialize(void);
static xlValue xMComClose(void);
static xlValue xMComDataAvailableP(void);
static void xMComReadPacket(void);
static xlValue xMComWritePacket(void);

/* instance variable offsets */
#define connectionHANDLE    xlFIRSTIVAR

/* local variables */
static xlValue s_connection;
static xlValue c_connection;

/* prototypes */
static CommConnection *GetConnection(void);
static void SetConnection(xlValue obj,CommConnection *c);

/* MComInitialize - enter merlin specific symbols and functions */
void MComInitialize(void)
{
    /* enter the foreign pointer type */
    s_connection = xlEnter("MCOM-CONNECTION");

    /* enter the connection class */
    c_connection = xlClass("MCOM-CONNECTION",NULL,"HANDLE");
    xlMethod(c_connection,"INITIALIZE",xMComInitialize);
    xlMethod(c_connection,"CLOSE",xMComClose);
    xlMethod(c_connection,"DATA-AVAILABLE?",xMComDataAvailableP);
    xlXMethod(c_connection,"READ-PACKET",xMComReadPacket);
    xlMethod(c_connection,"WRITE-PACKET",xMComWritePacket);
}

/* xMComInitialize - built-in method 'initialize' */
static xlValue xMComInitialize(void)
{
    char *port = NULL;
    CommConnection *c;
    xlValue obj;
    long addr;

    /* parse the arguments */
    obj = xlGetArgInstance(c_connection);
    xlVal = xlGetArgFixnum(); addr = (long)xlGetFixnum(xlVal);
    if (xlMoreArgsP()) {
        xlVal = xlGetArgFixnum();
        addr = afVirtualAddr(addr,(long)xlGetFixnum(xlVal));
        if (xlMoreArgsP())
            port = xlGetString(xlGetArgString());
    }
    xlLastArg();

    /* default the port to the value of the environment variable MD_PORT */
    if (!port)
        port = getenv("MD_PORT");

    /* try to open the connection */
    if (!port || !(c = mcomOpen(port,addr)))
        return xlNil;

    /* return the connection */
    SetConnection(obj,c);
    return obj;
}

/* xMComClose - built-in function 'mcom-close' */
static xlValue xMComClose(void)
{
    CommConnection *c;

    /* parse the arguments */
    xlVal = xlGetArgInstance(c_connection);
    xlLastArg();

    /* make sure it's not already closed */
    if ((c = (CommConnection *)xlGetFPtr(xlVal)) == 0)
        xlError("attempt to close an already closed mcom connection",xlVal);

    /* close the connection */
    SetConnection(xlVal,0);
    mcomClose(c);

    /* return successfully */
    return xlTrue;
}

/* xMComDataAvailableP - built-in function 'mcom-data-available?' */
static xlValue xMComDataAvailableP(void)
{
    CommConnection *c;
    int timeout = 0;

    /* parse the arguments */
    c = GetConnection();
    if (xlMoreArgsP()) {
        xlVal = xlGetArgFixnum(); timeout = (int)xlGetFixnum(xlVal);
    }
    xlLastArg();

    /* check to see if data is available */
    return mcomReadDataAvailableP(c,timeout) ? xlTrue : xlFalse;
}

/* xMComReadPacket - built-in function 'mcom-read-packet' */
static void xMComReadPacket(void)
{
    int timeout,timeoutP = FALSE;
    CommConnection *c;
    long v0,v1,v2,v3;

    /* parse the arguments */
    c = GetConnection();
    if (xlMoreArgsP()) {
        xlVal = xlGetArgFixnum(); timeout = (int)xlGetFixnum(xlVal);
        timeoutP = TRUE;
    }
    xlLastArg();

    /* make sure data is available if a timeout was requested */
    if (timeoutP) {
        if (!mcomReadDataAvailableP(c,timeout))
            xlMVReturn(0);
    }

    /* read the packet */
    if (!mcomReadPacket(c,&v0,&v1,&v2,&v3))
        xlMVReturn(0);

    /* return the packet */
    xlCheck(4);
    xlPush(xlMakeFixnum((xlFIXTYPE)v3));
    xlPush(xlMakeFixnum((xlFIXTYPE)v2));
    xlPush(xlMakeFixnum((xlFIXTYPE)v1));
    xlPush(xlMakeFixnum((xlFIXTYPE)v0));
    xlMVReturn(4);
}

/* xMComWritePacket - built-in function 'mcom-write-packet' */
static xlValue xMComWritePacket(void)
{
    CommConnection *c;
    long v0,v1,v2,v3;

    /* parse the arguments */
    c = GetConnection();
    xlVal = xlGetArgFixnum(); v0 = (long)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); v1 = (long)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); v2 = (long)xlGetFixnum(xlVal);
    xlVal = xlGetArgFixnum(); v3 = (long)xlGetFixnum(xlVal);
    xlLastArg();

    /* write the packet */
    return mcomWritePacket(c,v0,v1,v2,v3) ? xlTrue : xlFalse;
}

/* GetConnection - get a mmp memory argument */
static CommConnection *GetConnection(void)
{
    xlValue arg,handle;
    CommConnection *c;

    /* get the next argument */
    arg = xlGetArgInstance(c_connection);

    /* get the connection handle */
    handle = xlGetIVar(arg,connectionHANDLE);
    if (!xlForeignPtrP(handle) || xlGetFPType(handle) != s_connection)
        xlError("bad handle in connection object",handle);

    /* make sure it's not closed */
    if ((c = (CommConnection *)xlGetFPtr(handle)) == 0)
        xlError("attempt to use a closed mcom connection",handle);

    /* return the connection */
    return (CommConnection *)xlGetFPtr(handle);
}

/* SetConnection - set the widget structure of a widget object */
static void SetConnection(xlValue obj,CommConnection *c)
{
    xlSetIVar(obj,connectionHANDLE,xlMakeForeignPtr(s_connection,c));
}


