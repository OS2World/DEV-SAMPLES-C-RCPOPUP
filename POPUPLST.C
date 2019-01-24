/**************************************************************************
*
*   Function: popuplst
*
*    Purpose: pop up a list of choices, accept a choice or cancel, return
*             the choice to the caller via pipe.
*
*   To build: pmake popuplst
*
*     To use: popuplst hRespPipe cMaxWidth "comma,delimited,args" ["Title"]
*             Decimal handle # ^         ^                      ^        ^
*                Width of column in list ^                      ^        ^
*                     The list of choices (seperated by commas) ^        ^
*                             The title bar to be placed in the list box ^
*
*   Defaults: none
*
*       File: popuplst.c
*
*   Abstract: Typical PM main function which initializes PM, creates a
*             message queue, registers a window class, creates a window,
*             gets and dispatches messages to its winproc until its time
*             to quit, and then tidies up before terminating.
*
*             This particular PM function supports only a single list box.
*             The selections to be presented in the list are one of the
*             command line arguments of popuplst.  Popup sends the string
*             selected (or a NULL string for CANCEL) to the response pipe
*             designated in the 1st argument.  If a filename is given
*             instead of a number as the 1st argument, that filename will
*             be opened instead and the terminal string will be placed in
*             the file.
*
**************************************************************************/

#define INCL_WINDIALOGS
#define INCL_WINERRORS
#define INCL_WINFRAMEMGR
#define INCL_WINLISTBOXES
#define INCL_WINMESSAGEMGR
#define INCL_WINSYS
#define INCL_WINWINDOWMGR
#define M_I86L
#define INCL_DOSFILEMGR
#define INCL_VIO
#include <os2.h>
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
 
#include "popuplst.h"
 
/* Local Routines */ 
static void errex(char *str, unsigned short api_err);
VOID cdecl CenterDlgBox(HWND);
 
/* Window Procedures */
static MRESULT EXPENTRY fnwpMainWnd(HWND, USHORT, MPARAM, MPARAM);
static MRESULT EXPENTRY fnwpListBoxDlg(HWND, USHORT, MPARAM, MPARAM);
 
/*  Global variables */
#define MAXLEN_LISTBOXENTRY         80

CHAR  szSelection[MAXLEN_LISTBOXENTRY] = "";  /* Common variable for list */
                                        /* code to copy selected value to */
HAB   hab;                              /* Anchor block handle     */
HWND  hwndClient;                       /* Client Window handle    */
HWND  hwndFrame;                        /* Frame Window handle     */
short cNumChoices, cChoiceWidth;        /* Length and Width of box */
HFILE hfWrite;                          /* Handle of pipe to respond to */
FILE  *fpOutFile;                       /* File pointer to write term string */
char  *psChoiceStr;                     /* String of choices */
char  *psTitleStr;                      /* Optional list box title */

/**************************************************************************
*
*   Function: main
*
*    Purpose: Window setup and argument parsing.  Sends response.
*
**************************************************************************/

VOID NEAR cdecl main( int argc, char *argv[] )
{
  HMQ     hmq;                          /* Message Queue handle         */
  QMSG    qmsg;                         /* Message                      */
  ULONG   flCreate;
  ERRORID erid;
  USHORT  erno;

  cChoiceWidth = atoi(argv[2]);         /* Width of list box            */
  psChoiceStr = argv[3];                /* String of choices            */
  psTitleStr = argv[4];                 /* Title block initializer      */

  hab   = WinInitialize( 0 );           /* Initialize PM                */
  if (hab == NULLHANDLE) {
    erid = WinGetLastError(hab);
    erno = ERRORIDERROR(erid);
    errex("WinInitialize failed", erno);
  }
  hmq   = WinCreateMsgQueue( hab, 0 );  /* Create application msg queue */
  if (hmq == NULLHANDLE) {
    erid = WinGetLastError(hab);
    erno = ERRORIDERROR(erid);
    errex("WinCreateMsgQueue failed", erno);
  }

  WinRegisterClass(                     /* Register Window Class        */
      hab,                              /* Anchor block handle          */
      "popuplst Class",                 /* Window Class name            */
      (PFNWP)fnwpMainWnd,               /* Address of Window Procedure  */
      (ULONG) NULL,                     /* No special class style       */
      0                                 /* No extra window words        */
      );
 
        /* Create only a minimal window -- tasklist is necessary for
           DosSelectSession (see pltest.c)                              */
  flCreate = FCF_TITLEBAR | FCF_ICON | FCF_TASKLIST;

        /* Create a pseudo (invisible) window to handle PM babble */
        /* OS/2 1.x always would use DESKTOP. 2.0 usually uses HWND_OBJECT */
  hwndFrame = WinCreateStdWindow(
        HWND_DESKTOP,                   /* Desktop Window is parent     */
        0,                              /* Window styles (INVISIBLE)    */
        (PVOID)&flCreate,               /* Window creation parameters   */
        "popuplst Class",               /* Window Class name            */
        "",                             /* Window Text                  */
        0L,                             /* Client style                 */
        0,                              /* Module handle                */
        ID_MAINWND,                     /* Window ID                    */
        (HWND FAR *)&hwndClient         /* Client Window handle         */
        );

        /* Pop up the list box */
  WinDlgBox(  HWND_DESKTOP,             /* Parent                    */
              hwndFrame,                /* Owner                     */
              (PFNWP)fnwpListBoxDlg,    /* Address of dialog proc    */
              0,                        /* Module handle             */
              DLG_LISTBOX,              /* Id of dialog in resource  */
              NULL );                   /* Initialisation data       */

  /* Message Loop */
  while( WinGetMsg( hab, (PQMSG)&qmsg, (HWND)NULL, 0, 0 ) )
    WinDispatchMsg( hab, (PQMSG)&qmsg );
 
  /* Cleanup code */
  WinDestroyWindow( hwndFrame );
  WinDestroyMsgQueue( hmq );
  WinTerminate( hab );

  hfWrite = atoi(argv[1]);              /* 1st arg is pipe response handle */
  if (hfWrite != 0) {                   /* If user supplied a pipe */
    USHORT api_err;                     /* generic err rtn code from API calls*/
    ULONG actual;                       /* Dummy # bytes written              */

    api_err = DosWrite(hfWrite, szSelection, strlen(szSelection)+1, &actual);
    if (api_err != 0) errex("DosWrite failed", api_err);
    api_err = DosBufReset(hfWrite);     /* Guarantee msg delivery */
    if (api_err != 0) errex("DosBufReset failed", api_err);
  }
  else {                                /* Assume we should write to a file */
    fpOutFile = fopen(argv[1], "w");
    if (fpOutFile != NULL) {
      fprintf(fpOutFile, "DLG complete with: '%s'\n", szSelection);
      fclose(fpOutFile);
    }
  }
}
 
/***********************************************************************
*
*   WinProc: fnwpMainWnd
*
*   Controls the state of the menu, and loads various dialogs.  The
*   dialogs will be modal or modeless depending on the setting of the
*   Modality menuitem.
*
***********************************************************************/
 
static MRESULT EXPENTRY fnwpMainWnd(HWND hwnd, USHORT message, MPARAM mp1, MPARAM mp2 )
{
  switch(message)
  {
    case WM_CLOSE:
      WinPostMsg( hwnd, WM_QUIT, 0L, 0L );  /* Cause termination    */
      break;
    default:
      return WinDefWindowProc( hwnd, message, mp1, mp2 );
  }
  return FALSE;
}
 
/***********************************************************************
*
*  DlgProc:  fnwpListBoxDlg
*
***********************************************************************/
 
static MRESULT EXPENTRY fnwpListBoxDlg(HWND hwndDlg, USHORT message, MPARAM mp1, MPARAM mp2 )
{
  char *psNxtArg;
  SHORT  id;
 
  switch (message)
  {
    case WM_INITDLG:

      CenterDlgBox( hwndDlg );
 
        /* Initialize the listbox command line argument strings. */
      for ( psNxtArg = strtok(psChoiceStr, ","), cNumChoices=0;
            psNxtArg != NULL;
            psNxtArg = strtok(NULL, ","), cNumChoices++ ) {
        WinSendDlgItemMsg( hwndDlg,
                           LB_1,
                           LM_INSERTITEM,
                           MPFROM2SHORT( LIT_END, 0 ),
                           MPFROMP( psNxtArg )
                         );
      }
      if (cNumChoices==0) goto DO_CANCEL;   /* Misuse of this program! */
        /* If user has provided his own title */
      if ( (psTitleStr!=NULL) && (*psTitleStr!='\0') ) {
        WNDPARAMS wp;

        wp.fsStatus = WPM_TEXT;
        wp.cchText = strlen(psTitleStr);
        wp.pszText = psTitleStr;
        wp.cbPresParams = 0;
        wp.pPresParams = NULL;
        wp.cbCtlData = 0;
        wp.pCtlData = NULL;
        WinSendDlgItemMsg( hwndDlg,
                           DLG_TITLE,
                           WM_SETWINDOWPARAMS,
                           MPFROMP( &wp ),
                           NULL
                         );
      }
      break;
    case WM_COMMAND:
      switch( SHORT1FROMMP( mp1 ) )
      {
        case DID_OK:     /* Enter key or pushbutton pressed/ selected */
 
/***********************************************************************
*
*   Find out which item (if any) was selected and return the selected
*   item text.
*
***********************************************************************/
 
            id = SHORT1FROMMR( WinSendDlgItemMsg( hwndDlg,
                                                  LB_1,
                                                  LM_QUERYSELECTION,
                                                  0L,
                                                  0L ) );
            if( id == LIT_NONE )        /* If no entry selected */
              strcpy( szSelection, "" );
            else                        /* Copy sel. entry to szSelection */
              WinSendDlgItemMsg( hwndDlg,
                                 LB_1,
                                 LM_QUERYITEMTEXT,
                                 MPFROM2SHORT( id, cChoiceWidth ),
                                 MPFROMP( szSelection ) );
              /* Fall through to CANCEL cleanup code */

        case DID_CANCEL: /* Escape key or CANCEL pushbutton pressed/selected */
        DO_CANCEL:
            WinDismissDlg( hwndDlg, TRUE );
            WinPostMsg( hwndClient, WM_CLOSE, 0L, 0L );  /* Cause termination    */
          return FALSE;
        default:
          break;
      }
      break;
 
    default:  /* Pass all other messages to the default dialog proc */
      return WinDefDlgProc( hwndDlg, message, mp1, mp2 );
  }
  return FALSE;
}
 
/*************************************************************************
*
*   FUNCTION : CenterDlgBox
*
*   Positions the dialog box in the center of the screen
*
*************************************************************************/
 
VOID cdecl CenterDlgBox(HWND hwnd )
{
  SHORT ix, iy;
  SHORT iwidth, idepth;
  SWP   swp;
 
  iwidth = (SHORT)WinQuerySysValue( HWND_DESKTOP, SV_CXSCREEN );
  idepth = (SHORT)WinQuerySysValue( HWND_DESKTOP, SV_CYSCREEN );
  WinQueryWindowPos( hwnd, (PSWP)&swp );
  ix = ( iwidth  - swp.cx ) / 2;
  iy = ( idepth  - swp.cy ) / 2;
        /* Attempt to also put us on the top window so that everyone sees us */
  WinSetWindowPos( hwnd, HWND_TOP, ix, iy, 0, 0, SWP_MOVE|SWP_SHOW|SWP_ZORDER|SWP_ACTIVATE );
}

/**************************************************************************
*
*   Function: errex
*
*    Purpose: Do a popup if necessary to show errors
*
**************************************************************************/
static void errex(char *str, unsigned short api_err)
{
  USHORT fWait = VP_WAIT | VP_OPAQUE;
  char sOutStr[512];

  VioPopUp(&fWait, 0);
  sprintf(sOutStr, "%s with error %d\n", str, api_err);
  puts(sOutStr);
  puts("hit return to continue...");
  getchar();
  VioEndPopUp(0);
  DosExit(EXIT_PROCESS, api_err);
}
