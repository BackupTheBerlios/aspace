
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/datatypes.h>
#include <proto/muimaster.h>
#include <proto/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>
#include <hardware/blit.h>
#include <libraries/mui.h>
#include <clib/alib_protos.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/***********************************************************************/

/* Imported */
extern struct ExecBase         *SysBase;
extern struct DosLibrary       *DOSBase;
extern struct IntuitionBase    *IntuitionBase;
extern struct GfxBase          *GfxBase;
extern struct Library          *UtilityBase;
extern struct Library          *DataTypesBase;
extern struct Library          *CyberGfxBase;
extern struct Library          *MUIMasterBase;

/* Exported */
extern struct MUI_CustomClass  *picClass;

/***********************************************************************/

ULONG initPicClass ( void );
void freePicClass ( void );

#define PICTAGBASE (TAG_USER+1) /* define this */

#define PICTAG(x) (PICTAGBASE+x)
#define picObject NewObject(picClass->mcc_Class,NULL

/***********************************************************************/

#define MUIA_Pic_Source 		PICTAG(0)  /* APTR,  [I...] */
#define MUIA_Pic_SourceType 		PICTAG(1)  /* ULONG, [I...] */
#define MUIA_Pic_PicNumber 		PICTAG(2)  /* ULONG, [I...] */
#define MUIA_Pic_Precision 		PICTAG(3)  /* ULONG, [I...] */
#define MUIA_Pic_Transparent 		PICTAG(4)  /* BOOL,  [ISGN] */
#define MUIA_Pic_FreeHoriz 		PICTAG(5)  /* BOOL,  [ISGN] */
#define MUIA_Pic_FreeVert 		PICTAG(6)  /* BOOL,  [ISGN] */
#define MUIA_Pic_Free     		PICTAG(7)  /* BOOL,  [IS..] */
#define MUIA_Pic_HCenter 		PICTAG(8)  /* BOOL,  [ISGN] */
#define MUIA_Pic_VCenter 		PICTAG(9)  /* BOOL,  [ISGN] */
#define MUIA_Pic_Center 		PICTAG(10) /* BOOL,  [IS..] */
#define MUIA_Pic_XUnlimited 		PICTAG(11) /* BOOL,  [ISGN] */
#define MUIA_Pic_YUnlimited 		PICTAG(12) /* BOOL,  [ISGN] */
#define MUIA_Pic_Unlimited 		PICTAG(13) /* BOOL,  [IS..] */
#define MUIA_Pic_Width      	        PICTAG(14) /* ULONG, [ISGN] */
#define MUIA_Pic_Height			PICTAG(15) /* ULONG, [ISGN] */
#define MUIA_Pic_ScaleFactor    	PICTAG(16) /* ULONG, [ISGN] */
#define MUIA_Pic_MaxSize		PICTAG(17) /* ULONG, [IS..] */
#define MUIA_Pic_Pics 			PICTAG(18) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapWidth 		PICTAG(19) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapHeight 		PICTAG(20) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapDepth 		PICTAG(21) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapMasking 		PICTAG(22) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapCompression	PICTAG(23) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapTransparent	PICTAG(24) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapXAspect 		PICTAG(25) /* ULONG, [.G..] */
#define MUIA_Pic_BitmapYAspect 		PICTAG(26) /* ULONG, [.G..] */

/***********************************************************************/

#define MUIM_Pic_CopyToClip 		PICTAG(0)
#define MUIM_Pic_Write 				PICTAG(1)

struct MUIP_Pic_Write
{
    ULONG  MethodID;
    STRPTR file;
};

/***********************************************************************/

