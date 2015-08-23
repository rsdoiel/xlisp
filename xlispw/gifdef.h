
//
//	Bit masks for the Packed field
//


#define MAX_LINES 800			// for interlaced, only up to 800 lines!

#define GIF_PACKED_MASK_CT_SIZE		0x07
#define GIF_PACKED_BIT_CT_SORT		0x08
#define GIF_PACKED_MASK_COLR_RES	0x07
#define GIF_PACKED_SHIFT_COLR_RES	0x04
#define GIF_PACKED_BIT_GLOBAL_CT	0x80

//
// GIF Color Table Layout
//
typedef struct _gifcolortable
{
	BYTE	nRed;
	BYTE	nGreen;
	BYTE	nBlue;
} GIFCOLORTABLE;



///////////////////////////////////////////////////////////////////////////////
// GifDefs.h - GIF Defines

#ifndef _GifDefs_h_
#define _GifDefs_h_
#if 0
#include <afxtempl.h>
#endif
//
// GIF Header (use single-byte packing please)
//
#pragma pack(1)
typedef struct _gifheader
{
	// Header
	char	arrSignature[3];	// Header Signature (always 'GIF')
	char	arrVersion[3];		// GOF format version ('87a' or '89a')

	// Logical Screen Descriptor
	WORD	wScreenWidth;		// In pixels
	WORD	wScreenHeight;		// In pixels
	BYTE	fPacked;			// Screen and color map information
	BYTE	nBackgroundColor;	// Bg color index
	BYTE	nAspectRatio;		// Pixel aspect ratio
} GIFHEADER;

#define GIF_SIGNATURE	"GIF"
#define GIF_V87a		"87a"
#define GIF_V89a		"89a"

#define GIF_TRAILER		0x3b
#define GIF_SEPERATOR	0x2c
#define GIF_ZEROBLOCK	0x00

//	Local GIF Image Descriptor
//
typedef struct _gifimgdesc
{
	BYTE	idSeperator;	// Image descriptor identifier
	WORD	wLeft;			// X position of image on the display
	WORD	wTop;			// Y position of image on the display
	WORD	wWidth;			// Width of the image in pixels
	WORD	wHeight;		// Height of the image in pixels
	BYTE	fPacked;		// Image and Color Table Data
} GIFIMGDESC;

#define GIFIMG_PACKED_BIT_LCL_CT			0x80
#define GIFIMG_PACKED_BIT_INTERLACE			0x40
#define GIFIMG_PACKED_BIT_SORT				0x20
#define GIFIMG_PACKED_MASK_LCL_CT_SIZE		0x07
#define GIFIMG_PACKED_SHIFT_LCL_CT_SIZE		0x00



#pragma pack()



////////////////////////////////////////
// GIF 89a Extensions
//

#define GIF_EXTENSION		0x21
#define GIF_EXT_GRAPHICS	0xf9
#define GIF_EXT_TEXT		0x01
#define GIF_EXT_APP			0xff
#define GIF_EXT_COMMENT		0xfe
//
// GIF Graphics Control Extension
// NOT NEEDED!
#if 0

typedef struct _gifGraphicControlExtention {
	BOOLEAN		bTransparentColor;
	BOOLEAN		bUserInput;
	WORD		wDisposalMethod;
	WORD		wDelayTime;
	BYTE		cColorIndex;
} GIFGRAPHCTRL;

typedef CArray<GIFGRAPHCTRL, GIFGRAPHCTRL&> GIFGRAPHCTRLArray;

//
// GIF Plain Text Extension
//
typedef struct _gifTextExtension {
#pragma pack(1)
	WORD		wTextGridLeft;		// X pos of text
	WORD		wTextGridTop;		// Y pos of text
	WORD		wTextGridWidth;	
	WORD		wTextGridHeight;	
	BYTE		cCellWidth;
	BYTE		cCellHeight;
	BYTE		cTextFgColorIndex;
	BYTE		cTextBgColorIndex;
#pragma pack()
	CByteArray	arrTextData;
} GIFTXTEXT;

typedef  CArray<GIFTXTEXT, GIFTXTEXT&> GIFTXTEXTArray;

//
// GIF Application Extension
//
typedef struct _gifAppExtension {
#pragma pack(1)
	BYTE		cIdentifier[8];
	BYTE		cAuthentCode[3];
#pragma pack()
	CByteArray	arrAppData;
} GIFAPPEXT;

typedef  CArray<GIFAPPEXT, GIFAPPEXT&> GIFAPPEXTArray;


//
// GIF Comment Extension
//
typedef struct _gifCommentExtension {
	CByteArray	arrCommentData;
} GIFCOMEXT;

typedef  CArray<GIFCOMEXT, GIFCOMEXT&> GIFCOMEXTArray;

#endif

/////////////////////////////////////
// GIF Image Info
//
typedef struct _gifImgInfo GIFIMGINFO;

struct _gifImgInfo {
	GIFIMGINFO			*pNext;			// Points to next element in list
	GIFIMGINFO			*pTail;			// Points to tail of list - CAUTION: only valid for head of list
	DWORD				nCount;			// Number of elemnts in list - CAUTION: only valid for head of list
	GIFIMGDESC			imgHeader;
	DWORD				dwImgHeadFilePos;
	DWORD				dwImgDataFilePos;
	DWORD				dwLocalCTFilePos;
	DWORD				dwLineAdvance;
	BOOLEAN				bLocalCT;
	BOOLEAN				bInterlace;
	BOOLEAN				bLocalCTSort;
	WORD				wLocalCTSize;
	WORD				nLocalCTEntries;
	WORD				wScan;
	WORD				arrLastLineInScan[4];
	BOOL				bImageLoaded;
	BYTE				*pImageBuf;
	DWORD				dwImageBufSize;
	DWORD				arrMappedRowTable[MAX_LINES];		// only up to 800 lines!!!
#if 0 // NOT NEEDED:
	CDWordArray			arrMappedRowTable;
	GIFGRAPHCTRLArray	arrGraphControls;
	GIFTXTEXTArray		arrTextExtensions;
	GIFAPPEXTArray		arrAppExtensions;
	GIFCOMEXTArray		arrCommentExtensions;
#endif
};

//
// Configuration structure
//
typedef struct _gifconfig
{
	BOOL bGIF89a;
	BOOL bInterlaced;
	BOOL bLocalColorTable;
} GIFCONFIG;

#endif