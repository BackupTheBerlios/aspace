/* gcc -noixemul -o Execute-Test Execute-Test.c */
/* =================================================================== */
      /* Includes */
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <string.h>

/* =================================================================== */

      /* Vars */
BPTR OutFile;
BOOL Command;


      /* Consts */
STRPTR CommandLine = "lha";

/* =================================================================== */

int
main (int argc, char argv)
{

    OutFile = Open("PIPE:Output", MODE_READWRITE);

    Command = Execute(CommandLine,NULL,OutFile);

    if (Command)
        Close(OutFile);
    else
        return RETURN_ERROR;

    // stampa a video PIPE:OUTPUT

    // Cancella PIPE:OUTPUT

    return RETURN_OK;

}
