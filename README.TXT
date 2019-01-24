
 Package: popuplst / pltest version 2

 Creator: Rick Curry, Janus Systems, Inc., 833 Flynn Camarillo, CA 93012
          (805)484-9770

 Purpose: To demonstrate IPC capabilities between character mode
          applications and Presentation Manager (PM) applications.

To build: NMAKE -F POPUPLST ; NMAKE -F PLTEST

  To use: pltest popuplst.exe 30 "arg1,arg2,arg3" "Title"
                               ^               ^       ^
      Max width for list items-^               ^       ^
      List items (seperated by commas)---------^       ^
      Title to appear on list box----------------------^

     This package is a set of sample code which demonstrates calling a
Presentation Manager application (in this case a popup list) from a
character mode application.  The response from the list box is
transmitted back to the character mode application via unnamed
(anonymous) pipe.  This code was developed partly to verify that child
processes do inherit open pipes under CSET/2.  This package is designed
to be built under CSET/2 and the make files will generate the
appropriate compilation switches for the multi-threaded library
(required by async.c).  The PM code was adapted from an MSC 6.0 window
sample but now runs in the 32-bit model.

     The async.c module contains replacements (as near as I can get
them) for the DosReadAsync and DosWriteAsync functions which existed in
MSC 6.0 but are not supported under CSET/2.

 Changes: Rewrote listbox code to pure 32-bit, fixed the need to
          'CTL-ESC' from full-screen sessions, and you can now
          select from the task list window without repercussions.

     CAVEATS:
        - popuplst.exe MUST be in the path for this to work.
