/**************************************************************************
*
*   Function: pltest
*
*    Purpose: Test popuplst.  Generally this program will execute (DosExecPgm)
*             a specified program and send the program a handle to a response
*             pipe and whatever arguments were given on the command line
*
*   To build: pmake pltest.mak
*
*     To use: pltest testfunc args
*                    ^--------^---- Function to be tested
*                             ^-----Arguments to send to the function
*   Defaults: none
*
*       File: popuplst.c
*
*   Abstract: Pltest simply opens a pipe for response, invokes the specified
*             function with the following arguments: 1) the ascii decimal
*             handle to the reponse pipe and 2) whatever arguments were passed
*             to pltest.  On completion, the ascii response string from the
*             function being tested is printed.
*
**************************************************************************/

#define INCL_DOSFILEMGR
#define INCL_DOSPROCESS
#define INCL_DOSQUEUES
#define INCL_DOSSEMAPHORES
#define INCL_DOSSESMGR
#include <os2.h>
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
/* Local Routines */ 
#define ERROR_EXIT(string, rcvd_status) \
  { printf("%s with status %d, exiting\n", string, rcvd_status); \
    DosExit(EXIT_PROCESS, rcvd_status); }

/*  Global variables */

/**************************************************************************
*
*   Function: main
*
*    Purpose: Create pipe and exec program.  Print results.
*
**************************************************************************/

int main( int argc, char *argv[] )
{
  CHAR achFailName[128];
  RESULTCODES rescResults;
  HFILE hfRead, hfWrite;
  HEV ReadSyncSem;
  ULONG cbIn, api_err, api_err2;
  char argp[40], pInMsg[40], *tempp;    // Pointer to arguments
  char *argp2;                          /* Pointer to start of actual arguments */
  int ii;
  STARTDATA stdat;
  ULONG sess_id, pid;


        /* Setup Pipe to receive responses on                               */
  api_err = DosCreatePipe(&hfRead, &hfWrite, 4096);
  if (api_err != 0) ERROR_EXIT("DosCreatePipe failed", api_err);

        /* Setup arguments to specified function                            */
  tempp = argp;
  tempp += sprintf(argp, "%s", argv[1])+1;  /* Copy pgm name + NULL to args   */
  argp2 = tempp;                        /* Remember where args actually start */
  tempp += sprintf(tempp, "%lu ", hfWrite);  /* Pipe handle to respond to     */
  for (ii=2; ii<argc; ii++)             /* Copy remainder of users arguments  */
    tempp += sprintf(tempp, "\"%s\" ", argv[ii]); /* Next argument            */
  *++tempp = '\0';                      /* 2nd NULL to terminate arglist      */

        /* Execute function                                                 */
  stdat.Length      = 30;               /* Only use up to InheritOpt        */
  stdat.Related     = 1;                /* For setsession and restore focus */
  stdat.FgBg        = TRUE;             /* Start session in background OK   */
  stdat.TraceOpt    = 0;                /* No debug tracing                 */
  stdat.PgmTitle    = "Desktop interface";
  stdat.PgmName     = argv[1];          /* Program user is starting         */
  stdat.PgmInputs   = argp2;            /* User's args and response pipe    */
  stdat.TermQ       = NULL;             /* No termination status Queue      */
  stdat.Environment = 0;                /* Inherit current environment      */
  stdat.InheritOpt  = 1;                /* Inherit open handles (resp pipe) */
#if 0
    /* By setting length to 30, the rest of the options get defaults        */
  stdat.SessionType = 0;                /* Let OS/2 choose session style    */
  stdat.IconFile    = NULL;             /* No ICON which we determine       */
  stdat.PgmHandle   = 0L;
  stdat.PgmControl  = 0;                /* Just make it visible             */
  stdat.InitXPos    = 0;                /* Let OS/2 or the program decide   */
  stdat.InitYPos    = 0;
  stdat.InitXSize   = 0;
  stdat.InitYSize   = 0;
#endif

  api_err = DosStartSession(&stdat, &sess_id, &pid);
  if (api_err != 0) {
    ERROR_EXIT("DosStartSession failed", api_err);
  }

  api_err = DosSleep(1000L);            /* Child must be in tasklist, wait  */
  if (api_err != 0) {
    ERROR_EXIT("DosStartSession failed", api_err);
  }

  api_err = DosSelectSession(sess_id);  /* Put our child into foreground    */
  if (api_err != 0) {
    ERROR_EXIT("DosSelectSession failed", api_err);
  }

  DosClose(hfWrite);                    /* We only receive from pipe        */
  api_err = DosReadAsync(hfRead, &ReadSyncSem, &api_err2, pInMsg,
                                                      sizeof(pInMsg), &cbIn);
  if (api_err != 0) ERROR_EXIT("DosReadAsync dispatch failed", api_err);
  api_err = DosWaitEventSem(ReadSyncSem, 2*60*1000L);
  if (api_err != 0) ERROR_EXIT("DosWaitEventSem failed", api_err);
  if (api_err2 != 0) ERROR_EXIT("DosReadAsync execution failed", api_err2);
  DosCloseEventSem(ReadSyncSem);

  DosClose(hfRead);                     /* Done with read side of pipe now  */
  printf("Normal completion.");
  if (pInMsg[cbIn-1] == '\0') cbIn--;   /* Test for normal string           */
  else printf("  No null terminator in response message.");
  printf("  Response message:\n");      /* Output response message          */
  if (cbIn != 0) fwrite(pInMsg, 1, cbIn, stdout);
  else           fwrite("<NULL>", 1, 6, stdout);
  putchar('\n');
}
