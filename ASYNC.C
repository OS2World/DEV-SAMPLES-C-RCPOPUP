
/*****************************************************************************/
/*                                                                           */
/*  Function: DosReadAsync / DosWriteAsync                                   */
/*                                                                           */
/*   Creator: Rick Curry, Janus Systems, Inc., 833 Flynn Camarillo, CA 93012 */
/*            (805)484-9770                                                  */
/*                                                                           */
/*   Purpose: When IBM rewrote their API for OS/2 2.0, they decided that     */
/*            the Read/Write Async calls where either unnecessary or         */
/*            undesirable.  The following code implements the Async calls    */
/*            as closely as possible.  The most notable difference is that   */
/*            there are no longer RAM semaphores which is what the old       */
/*            (MicroSoft) functions used to use.  These functions open an    */
/*            anonymous (unnamed) semaphore for you and return the handle    */
/*            so that you can do a DosWaitEventSem (or whatever other Event  */
/*            semaphore functions you like to use.)  The calling function is */
/*            expected to DosCloseEventSem the semaphore.  Also since this   */
/*            function creates a thread to do the actual I/O, it is          */
/*            important that any program which calls this function is built  */
/*            for multi-threaded execution (/Gm+).                           */
/*                                                                           */
/*  To build: icc -c -W3 /Kf-c+e+p+r+ /Q /Gm async.c                         */
/*                                                                           */
/*    To use: See MicroSoft documentation for OS/2 1.2 or 1.3 API with       */
/*            differences noted in Purpose (above.)                          */
/*                                                                           */
/*      File: async.c                                                        */
/*                                                                           */
/*****************************************************************************/


#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#define INCL_DOSSEMAPHORES
#include <os2.h>

#include <stdlib.h>

        /* Packet of parameters for dispatched (engine) part of read/write */
struct AsyncParams {
  HFILE hf;                             /* Handle to open file */
  PHEV pphev;                           /* Pointer to handle for event sem */
  PULONG pulErrCode;                    /* Ptr to completion code for engine */
  PVOID pvBuf;                          /* Buffer to transfer */
  ULONG cbBuf;                          /* Req. # bytes to transfer */
  PULONG pcbBytesActual; };             /* Ptr to var rcvs actual bytes xfer */

        /* Dispatched (engine) part of DosReadAsync */
static void RAEngine(void *inp_arg) {
  struct AsyncParams *AP=inp_arg;
  APIRET rc;

  rc = DosRead(AP->hf, AP->pvBuf, AP->cbBuf, AP->pcbBytesActual);
  *AP->pulErrCode = rc;                 /* Post completion code */
  DosPostEventSem(*AP->pphev);          /* Release waiter(s) */
  free(AP);                             /* Free parameters packet */
}


        /* API for modified (CSET/2) version of DosReadAsync */
ULONG DosReadAsync(HFILE hf, PHEV pphev, PULONG pulErrCode, PVOID pvBuf, ULONG cbBuf, PULONG pcbBytesActual) {
  APIRET rc;
  ULONG tid;
  struct AsyncParams *AP;

        /* Allocate an AP packet */
  AP = (struct AsyncParams *)malloc(sizeof(*AP));
  if (AP == NULL) return(ERROR_NOT_ENOUGH_MEMORY);

  AP->hf              = hf;             /* Fill in the packet */
  AP->pphev           = pphev;
  AP->pulErrCode      = pulErrCode;
  AP->pvBuf           = pvBuf;
  AP->cbBuf           = cbBuf;
  AP->pcbBytesActual  = pcbBytesActual;

        /* Create an anonymous event sem to signal IO completion */
  rc = DosCreateEventSem(NULL, pphev, 0, FALSE);
  if (rc != 0) {
    free(AP);
    return(rc);
  }

        /* Dispatch work thread (engine) and forget the ID */
  tid = _beginthread(RAEngine, NULL, 4*4096, (void *)AP);
  if (tid == -1) {
    free(AP);
    DosCloseEventSem(*pphev);
    return(ERROR_MAX_THRDS_REACHED);
  }

  return(NO_ERROR);
}


        /* Dispatched (engine) part of DosWriteAsync */
static void WAEngine(void *inp_arg) {
  struct AsyncParams *AP=inp_arg;
  APIRET rc;

  rc = DosWrite(AP->hf, AP->pvBuf, AP->cbBuf, AP->pcbBytesActual);
  *AP->pulErrCode = rc;                 /* Post completion code */
  DosPostEventSem(*AP->pphev);          /* Release waiter(s) */
  free(AP);                             /* Free parameters packet */
}


        /* API for modified (CSET/2) version of DosWriteAsync */
ULONG DosWriteAsync(HFILE hf, PHEV pphev, PULONG pulErrCode, PVOID pvBuf, ULONG cbBuf, PULONG pcbBytesActual) {
  APIRET rc;
  ULONG tid;
  struct AsyncParams *AP;

        /* Allocate an AP packet */
  AP = (struct AsyncParams *)malloc(sizeof(*AP));
  if (AP == NULL) return(ERROR_NOT_ENOUGH_MEMORY);

  AP->hf              = hf;             /* Fill in the packet */
  AP->pphev           = pphev;
  AP->pulErrCode      = pulErrCode;
  AP->pvBuf           = pvBuf;
  AP->cbBuf           = cbBuf;
  AP->pcbBytesActual  = pcbBytesActual;

        /* Create an anonymous event sem to signal IO completion */
  rc = DosCreateEventSem(NULL, pphev, 0, FALSE);
  if (rc != 0) {
    free(AP);
    return(rc);
  }

        /* Dispatch work thread (engine) and forget the ID */
  tid = _beginthread(WAEngine, NULL, 4*4096, (void *)AP);
  if (tid == -1) {
    free(AP);
    DosCloseEventSem(*pphev);
    return(ERROR_MAX_THRDS_REACHED);
  }

  return(NO_ERROR);
}
