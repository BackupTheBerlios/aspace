
#include "pic.h"

/***********************************************************************/

struct MUI_CustomClass  *picClass = NULL;

/***********************************************************************/

#ifdef __MORPHOS__
ULONG WritePixelArrayAlpha(APTR, UWORD, UWORD, UWORD, struct RastPort *, UWORD, UWORD, UWORD, UWORD, ULONG);
#define WritePixelArrayAlpha(__p0, __p1, __p2, __p3, __p4, __p5, __p6, __p7, __p8, __p9) \
	LP10(216, ULONG , WritePixelArrayAlpha, \
		APTR ,             __p0, a0, \
		UWORD ,            __p1, d0, \
		UWORD ,            __p2, d1, \
		UWORD , 	   __p3, d2, \
		struct RastPort *, __p4, a1, \
		UWORD , 	   __p5, d3, \
		UWORD , 	   __p6, d4, \
		UWORD ,            __p7, d5, \
		UWORD ,            __p8, d6, \
		ULONG ,            __p9, d7, \
		, CyberGfxBase, 0, 0, 0, 0, 0, 0)
#else
ULONG WritePixelArrayAlpha(APTR,UWORD,UWORD,UWORD,struct RastPort *,UWORD,UWORD,UWORD,UWORD,ULONG);
#pragma libcall CyberGfxBase WritePixelArrayAlpha d8 76543921080a
#endif

/***********************************************************************/

#define FACTOR 100
#define RAWIDTH(w) ((((UWORD)(w))+15)>>3 & 0xFFFE)

/***********************************************************************/

struct data
{
	struct IClass 		*cl;
    Object              *obj;

    Object              *dto;			
    struct BitMapHeader *bmh;
    UWORD               picWidth;
    UWORD               picHeight;

    APTR 				source;
    ULONG				sourceType;
    ULONG				pics;
    ULONG				pic;

    struct BitMap       *bitMap;
    struct BitMap       *scaledBitMap;
    APTR                plane;
	struct BitMap		*planeBitMap;

    struct Screen		*screen;
    ULONG               precision;

    UBYTE				*chunky;		
	UBYTE				*scaledChunky;

    UWORD               scaleFactor;
    UWORD               maxSize;
    UWORD               fWidth;
    UWORD               fHeight;
    UWORD               scaledWidth;
    UWORD               scaledHeight;

    UWORD               l, t, w, h;

	ULONG				flags;
};

enum
{
	FLG_Show        = 1<<0,
	FLG_FixedWidth  = 1<<1,
	FLG_FixedHeight = 1<<2,
	FLG_FreeHoriz   = 1<<3,
	FLG_FreeVert    = 1<<4,
	FLG_HCenter     = 1<<5,
	FLG_VCenter     = 1<<6,
	FLG_Transparent = 1<<7,
	FLG_UseMinterm  = 1<<9,
	FLG_Disabled    = 1<<10,
	FLG_Setup       = 1<<12,
	FLG_XUnlimited  = 1<<13,
	FLG_YUnlimited  = 1<<14,
	FLG_Cyber       = 1<<15,
	FLG_RealAlpha   = 1<<16,
	FLG_Scale       = 1<<17,
};

/***********************************************************************/

static ULONG
mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
    Object         			*dto = NULL;
    struct BitMapHeader     *bmh;
    register struct TagItem *attrs = msg->ops_AttrList;

    if (obj = (Object *)DoSuperMethodA(cl,obj,(Msg)msg))
    {
        struct data *data = INST_DATA(cl,obj);

        data->cl  = cl;
        data->obj = obj;

		data->sourceType = DTST_FILE;

        msg->MethodID = OM_SET;
        DoMethodA(obj,(Msg)msg);
        msg->MethodID = OM_NEW;

        return (ULONG)obj;
    }

    return 0;
}

/***********************************************************************/

static ULONG
mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
    struct data *data = INST_DATA(cl,obj);

    switch(msg->opg_AttrID)
    {
        case MUIA_Pic_Transparent:       *msg->opg_Storage = (data->flags & FLG_Transparent) ? TRUE : FALSE; return TRUE;
        case MUIA_Pic_FreeVert:          *msg->opg_Storage = (data->flags & FLG_FreeVert)    ? TRUE : FALSE; return TRUE;
        case MUIA_Pic_FreeHoriz:         *msg->opg_Storage = (data->flags & FLG_FreeHoriz)   ? TRUE : FALSE; return TRUE;
        case MUIA_Pic_HCenter:           *msg->opg_Storage = (data->flags & FLG_HCenter)     ? TRUE : FALSE; return TRUE;
        case MUIA_Pic_VCenter:           *msg->opg_Storage = (data->flags & FLG_VCenter)     ? TRUE : FALSE; return TRUE;
        case MUIA_Pic_XUnlimited:        *msg->opg_Storage = (data->flags & FLG_XUnlimited)  ? TRUE : FALSE; return TRUE;
        case MUIA_Pic_YUnlimited:        *msg->opg_Storage = (data->flags & FLG_YUnlimited)  ? TRUE : FALSE; return TRUE;
        case MUIA_Pic_Width:       	 *msg->opg_Storage = data->scaledWidth; 							 return TRUE;
        case MUIA_Pic_Height:      	 *msg->opg_Storage = data->scaledHeight; 							 return TRUE;
        case MUIA_Pic_ScaleFactor:       *msg->opg_Storage = data->scaleFactor;								 return TRUE;

        case MUIA_Pic_Pics:              *msg->opg_Storage = data->pics; 									 return TRUE;
        case MUIA_Pic_BitmapWidth:       *msg->opg_Storage = data->picWidth; 		     return TRUE;
        case MUIA_Pic_BitmapHeight:      *msg->opg_Storage = data->picHeight; 		     return TRUE;
        case MUIA_Pic_BitmapDepth:       *msg->opg_Storage = data->bmh->bmh_Depth; 	     return TRUE;
        case MUIA_Pic_BitmapMasking:     *msg->opg_Storage = data->bmh->bmh_Masking; 	     return TRUE;
        case MUIA_Pic_BitmapCompression: *msg->opg_Storage = data->bmh->bmh_Compression;     return TRUE;
        case MUIA_Pic_BitmapTransparent: *msg->opg_Storage = data->bmh->bmh_Transparent;     return TRUE;
        case MUIA_Pic_BitmapXAspect:     *msg->opg_Storage = data->bmh->bmh_XAspect; 	     return TRUE;
        case MUIA_Pic_BitmapYAspect:     *msg->opg_Storage = data->bmh->bmh_YAspect; 	     return TRUE;
    }

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

static void
scaleMask(struct data *data,UWORD bpr,UWORD rows,UWORD dw,UWORD dh,UWORD xdf,UWORD ydf)
{
	if (data->scaledBitMap && data->plane)
    {
	    struct BitMap *bm;

	    if (bm = AllocBitMap(dw,dh,1,0,NULL))
	    {
	        struct BitScaleArgs bsa;
	        struct BitMap       sbm;

	        memset(&sbm,0,sizeof(sbm));
	        sbm.BytesPerRow = RAWIDTH(bpr);
	        sbm.Rows        = rows;
	        sbm.Depth       = 1;
	        sbm.Planes[0]   = data->plane;

	        memset(&bsa,0,sizeof(bsa));

	        bsa.bsa_SrcBitMap   = &sbm;
	        bsa.bsa_DestBitMap  = bm;

	        bsa.bsa_SrcWidth    = data->picWidth;
	        bsa.bsa_SrcHeight   = data->picHeight;

	        bsa.bsa_XSrcFactor  = FACTOR;
	        bsa.bsa_XDestFactor = xdf;

	        bsa.bsa_YSrcFactor  = FACTOR;
	        bsa.bsa_YDestFactor = ydf;

	        BitMapScale(&bsa);
        	data->planeBitMap = bm;
	    }
	}
}

static void
scaleBitMap(struct data *data,UWORD w,UWORD h)
{
    register UWORD x, y;

    if (w==data->picWidth && h==data->picHeight)
        return;

    if (w==0 || w>16383 || h==0 || h>16383)
        return;

    x = ScalerDiv(w,FACTOR,data->picWidth);
    y = ScalerDiv(h,FACTOR,data->picHeight);

    if (x==0 || x>16383 || y==0 || y>16383)
    	return;

    w = ScalerDiv(x,data->picWidth,FACTOR);
    h = ScalerDiv(y,data->picHeight,FACTOR);

    if (w==0 || w>16383 || h==0 || h>16383)
        return;

    if (data->scaledBitMap = AllocBitMap(w,h,data->bmh->bmh_Depth,((data->flags & FLG_Cyber) ? BMF_MINPLANES : 0)|BMF_CLEAR,data->screen->RastPort.BitMap))
    {
        struct BitScaleArgs scale;

        memset(&scale,0,sizeof(struct BitScaleArgs));

        scale.bsa_SrcBitMap   = data->bitMap;
        scale.bsa_DestBitMap  = data->scaledBitMap;

        scale.bsa_SrcWidth    = data->picWidth;
        scale.bsa_SrcHeight   = data->picHeight;

        scale.bsa_XSrcFactor  = FACTOR;
        scale.bsa_XDestFactor = x;

        scale.bsa_YSrcFactor  = FACTOR;
        scale.bsa_YDestFactor = y;

        BitMapScale(&scale);

        data->scaledWidth  = w;
        data->scaledHeight = h;

		scaleMask(data,data->picWidth,data->picHeight,scale.bsa_DestWidth,scale.bsa_DestHeight,x,y);
        SetSuperAttrs(data->cl,data->obj,MUIA_Pic_Width,w,MUIA_Pic_Height,h,TAG_DONE);
    }
}

/***********************************************************************/

static void
scaleBitMapFactor(struct data *data)
{
    register UWORD w, h;

    if (data->scaleFactor==0 || data->scaleFactor>16383)
        return;

    w = ScalerDiv(data->picWidth,data->scaleFactor,FACTOR);
    h = ScalerDiv(data->picHeight,data->scaleFactor,FACTOR);

    if (w==0 || w>16383 || w==data->picWidth || h==0 || h>16383 || h==data->picHeight)
        return;

	scaleBitMap(data,w,h);
}

/***********************************************************************/

static void
checkAlpha(struct data *data)
{
    register UBYTE *src;
    register ULONG reallyHasAlpha = FALSE;
    register int   x, y, h;

    src = data->chunky;

    for (y = 0, h = data->picHeight; y<h; y++)
    {
		register int w;

        for (x = 0, w = data->picWidth; x<w; x++)
        {
            if (*src) reallyHasAlpha = TRUE;
            src += 4;
        }
    }

    if (reallyHasAlpha) data->flags |= FLG_RealAlpha;
    else data->flags &= ~FLG_RealAlpha;
}

struct scale
{
    UWORD w;
    UWORD tw;
    UWORD sl;
    UWORD st;
    UWORD sw;
    UWORD sh;
    UWORD dw;
    UWORD dh;
};

struct scaleData
{
    LONG  cy;
    float sourcey;
    float deltax;
    float deltay;
};

static void
scaleLineRGB(struct scale *sce,struct scaleData *data,ULONG *src,ULONG *dst)
{
    LONG w8 = (sce->dw>>3)+1, cx = 0, dx = data->deltax*65536;

    src = (ULONG *)((UBYTE *)src+4*sce->sl+(data->cy+sce->st)*sce->tw);

    switch (sce->dw & 7)
    {
        do
        {
                    *dst++ = src[cx>>16]; cx += dx;
            case 7: *dst++ = src[cx>>16]; cx += dx;
            case 6: *dst++ = src[cx>>16]; cx += dx;
            case 5: *dst++ = src[cx>>16]; cx += dx;
            case 4: *dst++ = src[cx>>16]; cx += dx;
            case 3: *dst++ = src[cx>>16]; cx += dx;
            case 2: *dst++ = src[cx>>16]; cx += dx;
            case 1: *dst++ = src[cx>>16]; cx += dx;
            case 0: w8--;

        } while (w8);
    }

    data->cy = data->sourcey += data->deltay;
}

static void
scaleRGB(struct scale *sce,ULONG *src,ULONG *dst)
{
    if (sce && src && dst && sce->dw-1>0 && sce->dh-1>0)
    {
        struct scaleData scdata;
        LONG             y;

        scdata.cy       = 0;
        scdata.sourcey  = 0;

        scdata.deltax   = sce->sw-1;
        scdata.deltax  /= (sce->dw-1);

        scdata.deltay   = sce->sh-1;
        scdata.deltay  /= (sce->dh-1);

        for (y = 0; y<sce->dh; y++)
        {
            scaleLineRGB(sce,&scdata,src,dst);
            dst += sce->dw;
        }
    }
}

static void
scaleChunky(struct data *data,UWORD w,UWORD h)
{
    struct scale    sce;
    register UBYTE  *chunky;
    register ULONG  size;
    UWORD  			x, y, tw;

    if (w==0 || w>16383 || h==0 || h>16383)
        return;

    if (w==data->picWidth && h==data->picHeight)
        return;

    x = ScalerDiv(w,FACTOR,data->picWidth);
    y = ScalerDiv(h,FACTOR,data->picHeight);

    w = ScalerDiv(x,data->picWidth,FACTOR);
    h = ScalerDiv(y,data->picHeight,FACTOR);

    tw   = ((w+15)>>4)<<4;
    size = tw*h;
    size += size+size+size;

    if (!(chunky = AllocVec(size,MEMF_PUBLIC)))
		return;

    sce.w  = data->picWidth;
    sce.tw = data->picWidth*4;
    sce.sl = 0;
    sce.st = 0;
    sce.sw = data->picWidth;
    sce.sh = data->picHeight;
    sce.dw = w;
    sce.dh = h;

    scaleRGB(&sce,(ULONG *)data->chunky,(ULONG *)chunky);

    data->scaledChunky = chunky;
    data->scaledWidth  = w;
    data->scaledHeight = h;

	SetSuperAttrs(data->cl,data->obj,MUIA_Pic_Width,w,MUIA_Pic_Height,h,TAG_DONE);
}

static void
scaleChunkyFactor(struct data *data)
{
    UWORD w, h;

    w = ScalerDiv(data->picWidth,data->scaleFactor,FACTOR);
    h = ScalerDiv(data->picHeight,data->scaleFactor,FACTOR);

	scaleChunky(data,w,h);
}

/***********************************************************************/

static void
freePic(struct data *data)
{
    if (data->planeBitMap)
    {
    	FreeBitMap(data->planeBitMap);
        data->planeBitMap = NULL;
	}

    if (data->scaledBitMap)
    {
        FreeBitMap(data->scaledBitMap);
        data->scaledBitMap = NULL;
    }

	if (data->scaledChunky)
    {
	    FreeVec(data->scaledChunky);
    	data->scaledChunky = NULL;
    }

    if (data->chunky)
    {
        FreeVec(data->chunky);
        data->chunky = NULL;
    }

	data->bitMap = NULL;
	data->chunky = NULL;
}

/***********************************************************************/

static void
freeDPic(struct data *data)
{

    if (data->flags & (FLG_FreeHoriz|FLG_FreeVert))
    {
        if (data->planeBitMap)
        {
        	FreeBitMap(data->planeBitMap);
            data->planeBitMap = NULL;
		}

        if (data->scaledBitMap)
        {
            FreeBitMap(data->scaledBitMap);
            data->scaledBitMap = NULL;
        }

    	if (data->scaledChunky)
	    {
    	    FreeVec(data->scaledChunky);
        	data->scaledChunky = NULL;
	    }
    }
}

/***********************************************************************/

static void
freeDTO(struct data *data)
{
	if (data->dto)
    {
	    DisposeDTObject(data->dto);
    	data->dto = NULL;
	}

	data->picWidth  = data->fWidth  = data->picHeight = data->fHeight = data->scaleFactor = 0;
}

/***********************************************************************/

static ULONG
createDTO(struct data *data)
{
    ULONG pics = PDTANUMPICTURES_Unknown;

    if (data->dto = NewDTObject(data->source,DTA_SourceType,       data->sourceType,
                                 	   		 DTA_GroupID,          GID_PICTURE,
                                 	  		 PDTA_WhichPicture,    0,
                                 			 PDTA_GetNumPictures,  &pics,
                                 			 TAG_DONE))
    {
        if (data->pic)
        {
            DisposeDTObject(data->dto);
            data->dto = NULL;

            if (pics!=PDTANUMPICTURES_Unknown && pics>1)
            {
                data->dto = NewDTObject(data->source,DTA_SourceType,    data->sourceType,
                                         	   		 DTA_GroupID,       GID_PICTURE,
                                         	   		 PDTA_WhichPicture, data->pic,
                                         			 TAG_DONE);
            }
        }
    }

    if (data->dto)
    {
		if (GetDTAttrs(data->dto,PDTA_BitMapHeader,&data->bmh,TAG_DONE))
        {
	        data->picWidth  = data->fWidth  = data->bmh->bmh_Width;
    	    data->picHeight = data->fHeight = data->bmh->bmh_Height;

            return TRUE;
		}

		freeDTO(data);
	}

	return FALSE;
}

/***********************************************************************/

static void
createPic(struct data *data)
{
    if (data->dto && (data->flags & FLG_Setup))
    {
		if ((data->flags & FLG_Cyber) && (data->bmh->bmh_Depth>8))
    	{
	    	ULONG size;

			size  = data->picWidth*data->picHeight;
	        size += size+size+size;

			if (data->chunky = AllocVec(size,MEMF_PUBLIC))
	        {
				DoMethod(data->dto,PDTM_READPIXELARRAY,(ULONG)data->chunky,PBPAFMT_ARGB,data->picWidth<<2,0,0,data->picWidth,data->picHeight);
	            checkAlpha(data);

			    if (data->flags & FLG_Scale)
			    {
			        if (data->scaleFactor) scaleChunkyFactor(data);
					else scaleChunky(data,data->fWidth,data->fHeight);
				}
			}
	    }

	    if (!data->chunky)
	    {
		    SetAttrs(data->dto,PDTA_Screen,			 data->screen,
		                       OBP_Precision,        data->precision,
		                       PDTA_Remap,           TRUE,
		                       PDTA_DestMode,        (data->flags & FLG_Cyber) ? PMODE_V43 : PMODE_V42,
		                       PDTA_UseFriendBitMap, (data->flags & FLG_Cyber),
		                       TAG_DONE);

		    if (DoMethod(data->dto,DTM_PROCLAYOUT,NULL,TRUE) &&
		        (GetDTAttrs(data->dto,PDTA_DestBitMap,&data->bitMap,TAG_DONE) ||
		         GetDTAttrs(data->dto,PDTA_BitMap,&data->bitMap,TAG_DONE)))
			{
				GetDTAttrs(data->dto,PDTA_MaskPlane,&data->plane,TAG_DONE);

			    if (data->flags & FLG_Scale)
			        if (data->scaleFactor) scaleBitMapFactor(data);
		    	    else scaleBitMap(data,data->fWidth,data->fHeight);
			}
    	}
	}
}

/***********************************************************************/

void
createDPic(struct data *data)
{
 	if ((data->chunky || data->bitMap) && (data->flags & FLG_Show))
 	{
 	    if (data->flags & (FLG_FreeHoriz|FLG_FreeVert))
 	    {
 	        register UWORD w, h;

 	        w = (data->flags & FLG_FreeHoriz) ? _mwidth(data->obj)  : data->picWidth;
 	        h = (data->flags & FLG_FreeVert)  ? _mheight(data->obj) : data->picHeight;

 	        if (data->chunky) scaleChunky(data,w,h);
 			else scaleBitMap(data,w,h);
 	    }
	}
}

/***********************************************************************/

#define _BOOLSAME(a,b) ((a ? 1 : 0)==(b ? 1 : 0))

static ULONG
mSets(struct IClass *cl,Object *obj,struct opSet *msg)
{
    struct data    			*data = INST_DATA(cl,obj);
    register struct TagItem *tag, *tag1, *tag2;
    struct TagItem          *tstate;
    ULONG          			remake, redraw;

	tstate = msg->ops_AttrList;
	tag1 = tag2 = NULL;

    if ((tag  = FindTagItem(MUIA_Pic_Source,tstate)) ||
    	(tag1 = FindTagItem(MUIA_Pic_SourceType,tstate)) ||
    	(tag2 = FindTagItem(MUIA_Pic_PicNumber,tstate)))
    {
		if (tag)  data->source     = (APTR)tag->ti_Data;
		if (tag1) data->sourceType = tag1->ti_Data;
		if (tag2) data->pic        = tag2->ti_Data;

        freeDTO(data);
        createDTO(data);
    }

    for (remake = redraw = FALSE; tag = NextTagItem(&tstate); )
    {
        register ULONG tidata = tag->ti_Data;

        switch(tag->ti_Tag)
        {
            case MUIA_Pic_Precision:
	        	data->precision = tidata;
				break;

	        case MUIA_Pic_Free:
	            if (tidata) data->flags |= FLG_FreeHoriz|FLG_FreeVert;
	            else data->flags &= ~(FLG_FreeHoriz|FLG_FreeVert);
	            break;

	        case MUIA_Pic_FreeHoriz:
	            if (_BOOLSAME(tidata,data->flags & FLG_FreeHoriz)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_FreeHoriz|(FLG_FreeHoriz|FLG_FreeVert);
	                else data->flags &= ~FLG_FreeHoriz;
				}
	            break;

	        case MUIA_Pic_FreeVert:
	            if (_BOOLSAME(tidata,data->flags & FLG_FreeVert)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_FreeVert|(FLG_FreeHoriz|FLG_FreeVert);
	                else data->flags &= ~FLG_FreeVert;
				}
	            break;

	        case MUIA_Pic_Unlimited:
	            if (tidata) data->flags |= FLG_XUnlimited|FLG_YUnlimited;
	            else data->flags &= ~(FLG_XUnlimited|FLG_YUnlimited);
	            break;

	        case MUIA_Pic_XUnlimited:
	            if (_BOOLSAME(tidata,data->flags & FLG_XUnlimited)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_XUnlimited;
	               	else data->flags &= ~FLG_XUnlimited;
				}
	            break;

	        case MUIA_Pic_YUnlimited:
	            if (_BOOLSAME(tidata,data->flags & FLG_YUnlimited)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_YUnlimited;
	               	else data->flags &= ~FLG_YUnlimited;
				}
	            break;

	        case MUIA_Pic_Transparent:
	            if (_BOOLSAME(tidata,data->flags & FLG_Transparent)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_Transparent;
	                else data->flags &= ~FLG_Transparent;

	                redraw = TRUE;
				}
	            break;

	        case MUIA_Disabled:
	            if (_BOOLSAME(tidata,data->flags & FLG_Disabled)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_Disabled;
	                else data->flags &= ~FLG_Disabled;

	            	redraw = TRUE;
				}
	            break;

	        case MUIA_Pic_Center:
	            if (tidata) data->flags |= FLG_HCenter|FLG_VCenter;
	            else data->flags &= ~(FLG_HCenter|FLG_VCenter);
	            break;

	        case MUIA_Pic_HCenter:
	            if (_BOOLSAME(tidata,data->flags & FLG_HCenter)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_HCenter;
	                else data->flags &= ~FLG_HCenter;
				}
	            break;

	        case MUIA_Pic_VCenter:
	            if (_BOOLSAME(tidata,data->flags & FLG_VCenter)) tag->ti_Tag = TAG_IGNORE;
	            else
	            {
	                if (tidata) data->flags |= FLG_VCenter;
	                else data->flags &= ~FLG_VCenter;
				}
	            break;

	        case MUIA_Pic_Width:
	            if ((UWORD)tidata==data->fWidth) tag->ti_Tag = TAG_IGNORE;
		        {
		        	data->fWidth = (UWORD)tidata;
		            data->flags |= FLG_Scale;

		            if (data->flags & (FLG_FreeHoriz|FLG_XUnlimited))
		            	redraw = remake = TRUE;
				}
				break;

	        case MUIA_Pic_Height:
				if ((UWORD)tidata==data->fHeight) tag->ti_Tag = TAG_IGNORE;
	            {
	                data->fHeight = (UWORD)tidata;
	                data->flags |= FLG_Scale;

	                if (data->flags & (FLG_FreeVert|FLG_YUnlimited))
	                	redraw = remake = TRUE;
				}
	            break;

	        case MUIA_Pic_ScaleFactor:
				if ((UWORD)tidata==data->scaleFactor) tag->ti_Tag = TAG_IGNORE;
	            {
	                data->scaleFactor = (UWORD)tidata;
	                data->flags |= FLG_Scale;

	                if (data->flags & (FLG_FreeHoriz|FLG_FreeVert|FLG_XUnlimited|FLG_YUnlimited))
	                    redraw = remake = TRUE;
				}
	            break;

	        case MUIA_Pic_MaxSize:
				if ((UWORD)tidata==data->maxSize) tag->ti_Tag = TAG_IGNORE;
				else
                {
					data->maxSize = (UWORD)tidata;

                	if (data->dto)
			        {
			            UWORD w, h, max;

			            w   = data->fWidth;
			            h   = data->fHeight;
			            max = (UWORD)tidata;

						if (w>max || h>max)
			            {
			                if (w==h) w = h = max;
			                else
							    if (w>h)
							    {
							        h = (max*h/w);
							        w = max;
							    }
							    else
							    {
							        w = (max*w/h);
							        h = max;
							    }

						    if (w>0 && w<=16383 && h>0 && h<=16383 && (w!=data->fWidth || h!=data->fHeight))
							{
			        			data->fWidth  = w;
			        			data->fHeight = h;
								data->flags   |= FLG_Scale;

		                        redraw = remake = TRUE;
			                }
			            }
					}
                }
	            break;
	    }
	}

	if (remake && (data->flags & FLG_Setup))
    {
    	freePic(data);
        createPic(data);
    	
        if (data->flags & FLG_Show)
        {
    		freeDPic(data);
        	createDPic(data);
		}
    }

	if (redraw && (data->flags & FLG_Show))
		MUI_Redraw(obj,MADF_DRAWUPDATE);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

static ULONG
mSetup(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);
    ULONG       cyber;

    if (!DoSuperMethodA(cl,obj,(Msg)msg)) return 0;

    data->flags |= FLG_Setup;
	data->screen = _screen(obj);

    if (cyber = CyberGfxBase && IsCyberModeID(GetVPModeID(&_screen(obj)->ViewPort)))
		data->flags |= FLG_Cyber;

    createPic(data);

    return TRUE;
}

/***********************************************************************/

static ULONG
mCleanup(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

	freePic(data);

    data->flags &= ~(FLG_Setup|FLG_Cyber);

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

static ULONG
mShow(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

    if (!DoSuperMethodA(cl,obj,(Msg)msg)) return FALSE;

    data->flags |= FLG_Show;

    createDPic(data);

	return TRUE;
}

/***********************************************************************/

static ULONG
mHide(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);

	freeDPic(data);

    data->flags &= ~FLG_Show;

    return DoSuperMethodA(cl,obj,(Msg)msg);
}

/***********************************************************************/

static ULONG
mAskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax *msg)
{
    struct data    *data = INST_DATA(cl,obj);
    register UWORD mw, mh, dw, dh, Mw, Mh;

    DoSuperMethodA(cl,obj,(Msg)msg);

    if (data->flags & FLG_FixedWidth) mw = dw = Mw = data->fWidth;
    else
    {
        register UWORD w;

        w = (data->scaledBitMap || data->scaledChunky) ? data->scaledWidth : data->picWidth;

        if (data->flags & FLG_FreeHoriz)
        {
            mw = dw = 0;
            Mw = MUI_MAXMAX;
        }
        else
        {
            mw = dw = w;

            Mw = (data->flags & FLG_XUnlimited) ? MUI_MAXMAX : w;
        }
    }

    if (data->flags & FLG_FixedHeight) mh = dh = Mh = data->fHeight;
    else
    {
        register UWORD h;

        h = (data->scaledBitMap || data->scaledChunky) ? data->scaledHeight : data->picHeight;

        if (data->flags & FLG_FreeVert)
        {
            mh = dh = 0;
            Mh = MUI_MAXMAX;
        }
        else
        {
            mh = dh = h;

            Mh = (data->flags & FLG_YUnlimited) ? MUI_MAXMAX : h;
        }
    }

    msg->MinMaxInfo->MinWidth  += mw;
    msg->MinMaxInfo->MinHeight += mh;
    msg->MinMaxInfo->DefWidth  += dw;
    msg->MinMaxInfo->DefHeight += dh;
    msg->MinMaxInfo->MaxWidth  += Mw;
    msg->MinMaxInfo->MaxHeight += Mh;

    return 0;
}

/***********************************************************************/

static ULONG
mDraw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
    struct data *data = INST_DATA(cl,obj);
	ULONG		res;

    res = DoSuperMethodA(cl,obj,(Msg)msg);

    if ((data->flags & FLG_Show) && (msg->flags & (MADF_DRAWOBJECT|MADF_DRAWUPDATE)))
    {
        struct BitMap *bm;
        UBYTE 		  *chunky;
		UWORD  		  bw, bh, w, h, l, t;

		if (data->flags & FLG_FixedWidth) bw = data->fWidth;
        else if (data->scaledBitMap || data->scaledChunky) bw = data->scaledWidth;
        	 else bw = data->picWidth;

		if (data->flags & FLG_FixedHeight) bh = data->fHeight;
        else if (data->scaledBitMap || data->scaledChunky) bh = data->scaledHeight;
        	 else bh = data->picHeight;

        w = _mwidth(obj);
        h = _mheight(obj);
        l = _mleft(obj);
        t = _mtop(obj);

        if (bw<w)
        {
            if (data->flags & FLG_HCenter) l += (w-bw)/2;
            w = bw;
        }

        if (bh<h)
        {
            if (data->flags & FLG_VCenter) t += (h-bh)/2;
            h = bh;
        }

    	if (!(msg->flags & MADF_DRAWOBJECT))
            DoSuperMethod(cl,obj,MUIM_DrawBackground,data->l,data->t,data->w,data->h,data->l,data->t,0);

        data->l = l;
        data->t = t;
        data->w = w;
        data->h = h;

		if (chunky = data->scaledChunky ? data->scaledChunky : data->chunky)
        {
            if ((data->flags & FLG_Transparent) && (data->flags & FLG_RealAlpha)) WritePixelArrayAlpha(chunky,0,0,bw*4,_rp(obj),l,t,w,h,0xFFFFFFFF);
            else WritePixelArray(chunky,0,0,bw*4,_rp(obj),l,t,w,h,PBPAFMT_ARGB);

            return 0;
		}

        if (bm = data->scaledBitMap ? data->scaledBitMap : data->bitMap)
        {
        	APTR  plane;

            plane = NULL;

            if (data->flags & FLG_Transparent)
				if (data->scaledBitMap)
                {
                	if (data->planeBitMap) plane = data->planeBitMap->Planes[0];
                }
            	else plane = data->plane;

            if (plane) BltMaskBitMapRastPort(bm,0,0,_rp(obj),l,t,w,h,0xc0,plane);
            else BltBitMapRastPort(bm,0,0,_rp(obj),l,t,w,h,0xc0);
        }
    }

    return res;
}

/***********************************************************************/

static ULONG
mDispose(struct IClass *cl,Object *obj,Msg msg)
{
    struct data *data = INST_DATA(cl,obj);
    Object      *dto = data->dto;
    ULONG       res;

    res = DoSuperMethodA(cl,obj,msg);

    DisposeDTObject(dto);

    return res;
}

/***********************************************************************/

#ifdef __MORPHOS__
static ULONG
dispatcher(void)
{
    struct IClass *cl = (struct IClass *)REG_A0;
    Object        *obj = (Object *)REG_A2;
    Msg            msg  = (Msg)REG_A1;
#else
static __saveds __asm ULONG
dispatcher(register __a0 struct IClass *cl,register __a2 Object *obj,register __a1 Msg msg)
{
#endif

    switch(msg->MethodID)
    {
        case OM_NEW:                    return mNew(cl,obj,(APTR)msg);
        case OM_DISPOSE:                return mDispose(cl,obj,(APTR)msg);
        case OM_SET:                    return mSets(cl,obj,(APTR)msg);
        case OM_GET:                    return mGet(cl,obj,(APTR)msg);

        case MUIM_Setup:                return mSetup(cl,obj,(APTR)msg);
        case MUIM_Cleanup:              return mCleanup(cl,obj,(APTR)msg);
        case MUIM_AskMinMax:            return mAskMinMax(cl,obj,(APTR)msg);
        case MUIM_Show:                 return mShow(cl,obj,(APTR)msg);
        case MUIM_Hide:                 return mHide(cl,obj,(APTR)msg);
        case MUIM_Draw:                 return mDraw(cl,obj,(APTR)msg);

        default:                        return DoSuperMethodA(cl,obj,msg);
    }
}

#ifdef __MORPHOS__
static struct EmulLibEntry dispatcherTrap = {TRAP_LIB,0,(void *)dispatcher};
#define DISP ((APTR)&dispatcherTrap)
#else
#define DISP dispatcher
#endif

/***********************************************************************/

ULONG
initPicClass(void)
{
    if (picClass = MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct data),DISP))
    {
		return TRUE;
    }

	return FALSE;
}

/***********************************************************************/

void
freePicClass(void)
{
    if (picClass)
    {
        MUI_DeleteCustomClass(picClass);
		picClass = NULL;
    }
}

/**********************************************************************/
