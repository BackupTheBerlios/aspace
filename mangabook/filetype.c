/* includes */
#include "filetype.h";

/* program */

/* TODO: I have to find something more flexible
*/

STRPTR
FileType(BPTR File)
{
    STRPTR Command;

    if ( readHeader(File) )
    {
        if ((Hdr[0] == 0x52) && (Hdr[1] == 0x61) && (Hdr[2] == 0x72) && (Hdr[3] == 0x21))
        {
	    Command = "unrar ";
        }
        if ((Hdr[0] == 0x50) && (Hdr[1] == 0x4B) && (Hdr[2] == 0x03) && (Hdr[3] == 0x04) && (Hdr[4] == 0x14))
        {
	    Command = "zip ";
        }
        if ((Hdr[2] == 0x2D) && (Hdr[3] == 0x6C) && (Hdr[4] == 0x68) && (Hdr[6] == 0x2D))
        {
	    Command = "lha ";
        }
    else
    {
	Command = NULL;
    }

    return Command;

    }
}

/* Header reader function */
int
readHeader(BPTR file)
{
    long HdrCount;
    int error;

    if ( (HdrCount=FRead(file, Header, 8, 1)) != 1 )
    {
        error = IoErr();
        printf("Error! in FRead()\n", error);
        return error;
    }
    error = IoErr();
    return error;
}
