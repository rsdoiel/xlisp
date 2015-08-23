//
//	TclIconToBitmap.cpp
//

#include "windows.h"
#include "GifDef.h"
#include "IX_Gifdecoder.h"
#include "TclIconToBitmap.h"

/////////////////////////////////////////////////////////////////////////////////
// BASE 64 CONVERTER

extern int b64bin(char *pIn, unsigned char *pOut);

/////////////////////////////////////////////////////////////////////////////////
// LOCAL FUNCTIONS

static	GIFIMGINFO	*NewImgInfo(void);
static	GIFIMGINFO	*GetImgInfo(DWORD);
static	void		DeleteImgInfos(void);
static	BOOL		NewLinesReady(DWORD dwImageIndex, DWORD dwBytesReady, LPRECT pCurRect, LPRECT pNewRect );
static	DWORD		GetFirstLineOffset(void);
static	HRESULT		IsFileSupported( IN IMoniker *pmk );
static	HRESULT		ParseGIFHeader(GIFHEADER *gifh );
static	HRESULT		SeekGifImage(DWORD nImgIndex);
static	HRESULT		ExploreGifImages(void);
static	HRESULT		PrepareToGetScanLines(GIFIMGINFO*);
static	HRESULT		GetScanLine(LPBYTE, WORD);
static	HRESULT		SkipOverScanLine(WORD wWidth);
static	HRESULT		LoadImage(GIFIMGINFO *pGIFImage);
static	HRESULT		LoadImageData(GIFIMGINFO *pGIFImage);
static  HRESULT		GIFGetPalette(LPDWORD lpdwSize, RGBQUAD *pPalEntries );
static  HRESULT		GIFGetDIBData(LPRECT lpRect, LPDWORD lpdwSize, LPBYTE lpData);
static	void		FreeImage(GIFIMGINFO *pGIFImage);

/////////////////////////////////////////////////////////////////////////////////
// LOCAL DATA

#define PRISM_DIB_MONOCHROME	1
#define PRISM_DIB_4BITPALETTE	2
#define PRISM_DIB_8BITPALETTE	4


static	GIFHEADER		g_gifHeader;		// GIF Header
static	WORD			g_wDIBType;			// DIB Type of Images
static	BOOL			g_bIs89a;			// Flag (is a 89a file)
static	BOOL			g_bGlobalCT;		// Flag (Global Color Table existance)
static	WORD			g_wGlobalCTSize;	// Size of Global Color Table
static	BOOL			g_bCTSortFlag;		// Color Table is sorted
static	BOOL			g_bTrailerInfo;		// If true, g_arrImgInfo is one bigger than g_nImgCount and countains trailer info
static	WORD			g_wColorRes;		// Color resolution
static	WORD			g_nGlobalCTEntries;	// # of Global Color Table Entries
static	BOOL			g_bValidHeader;		// is header data valid (eq: Image loaded)
static	BOOL			g_bBGColorOverWrite;// Disposal Method
static	DWORD			g_nTotalImgCount;	// Total number of images in this GIF file
static	WORD			g_wBytesPerPixel;	// Bytes per pixel
static	WORD			g_wPixelsPerByte;	// Number of Pixels per Byte
static  WORD			g_wBitsPerPixel;	// Bits Per Pixel
static	WORD			g_nBits;
static	GIFIMGINFO		g_1stImgInfo;		// Image Info (local header) of every image

static CIX_GifDecoder	g_gifDecoder;		// Our GIF Decoder

//////////////////////////////////////////////////////////////////////////////////////
// Memory IO DATA:

static	BYTE			*g_pReadBuffer;
static	DWORD			g_dwReadBufferSize;
static	DWORD			g_dwSeekIndex;

// Memory IO Routines

void	InitBinBufferIO(BYTE *pData, DWORD dwSize);
HRESULT BinBufferRead(BYTE *pDest, DWORD dwBytesToRead, LPDWORD pdwBytesRead);
HRESULT BinBufferSeek(LONG lPos, DWORD dwMode, DWORD *lpdwNewPos);
BYTE	BinBufferGetByte(void);

///////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////
//	TclIconToBitmap()
//		Our Interface to the outside world!
//	Takes a device contents, a base 64 encoded GIF and returns an HBITMAP.
//
///////////////////////////////////////////////////////////////////////////////////

HBITMAP _cdecl TclIconToBitmap(HDC hdc, char *pData, DWORD *pdwWidth, DWORD *pdwHeight)
{
	if (pData == NULL)
		return NULL;

	HRESULT		hr = S_OK;
	DWORD		dwMaxSize = strlen(pData);
	BYTE		*pBinData = NULL;	
	BITMAPINFO	*pbmi = NULL;
	DWORD		nPaletteEntries = 0;
	GIFIMGINFO  *pGIF;
	VOID		*pvBits;
	HBITMAP		hBM;
	BOOL		bLoaded = FALSE;

	pBinData = new BYTE[dwMaxSize];	
	if (pBinData == NULL)
		return NULL;

	// First, Convert Base64 ASCII to binaray
	if (b64bin(pData, pBinData) != 0)	
		return NULL;		// Error in decdoding

	// Init our Memory Buffer I/O 
	InitBinBufferIO(pBinData, dwMaxSize);

	// Parse the GIF Heade
	hr = ParseGIFHeader(&g_gifHeader);
	if (FAILED(hr))
		goto _err_exit;
	hr = ExploreGifImages();
	if (hr == S_FALSE)
		goto _err_exit;
	else if( hr != S_OK )
		goto _err_exit;	
	// Set a few things...
	if (g_wColorRes == 1) {
		g_wDIBType = PRISM_DIB_MONOCHROME;
		g_wPixelsPerByte = 8;
		g_wBitsPerPixel  = 1;
	} else if (g_wColorRes <= 4) {
		g_wDIBType = PRISM_DIB_4BITPALETTE;
		g_wPixelsPerByte = 2;
		g_wBitsPerPixel  = 4;
	} else {
		g_wDIBType = PRISM_DIB_8BITPALETTE;
		g_wPixelsPerByte = 1;
		g_wBitsPerPixel  = 8;
	}
	g_wBytesPerPixel = 1;

	pGIF = g_1stImgInfo.pNext;

	// Get Palette Size
	if (g_1stImgInfo.pNext->bLocalCT)
		nPaletteEntries = g_1stImgInfo.pNext->nLocalCTEntries;
	else
		nPaletteEntries = g_nGlobalCTEntries;

	// Create Bitmap Info
	pbmi = (BITMAPINFO*) new  BYTE [sizeof(BITMAPINFOHEADER) + 
					(1 << g_wBitsPerPixel)  * sizeof(RGBQUAD)];
	if (pbmi == NULL) {
		hr = E_OUTOFMEMORY;
		goto _err_exit;
	}
	pbmi->bmiHeader.biSize			= sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biWidth			= pGIF->imgHeader.wWidth;
	pbmi->bmiHeader.biHeight		= pGIF->imgHeader.wHeight;
	pbmi->bmiHeader.biPlanes		= 1;
	pbmi->bmiHeader.biBitCount		= g_wBitsPerPixel;
	pbmi->bmiHeader.biCompression	= BI_RGB;
	pbmi->bmiHeader.biSizeImage		= pGIF->dwLineAdvance * 
									  pGIF->imgHeader.wHeight;
	pbmi->bmiHeader.biXPelsPerMeter = 0;
	pbmi->bmiHeader.biYPelsPerMeter = 0;
	pbmi->bmiHeader.biClrUsed		= 1 << (g_wBitsPerPixel);
	pbmi->bmiHeader.biClrImportant = 0;

	// Get Palette
	hr = GIFGetPalette(&nPaletteEntries, pbmi->bmiColors);
	if (FAILED(hr))
		goto _err_exit;

	// Get DIB Data
	hr = SeekGifImage(0);
	if (FAILED(hr))
		goto _err_exit;
	hr = LoadImage(pGIF);
	if (FAILED(hr))
		goto _err_exit;
	bLoaded = TRUE;

	hBM = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

	if (hBM != NULL) {
		RECT	rect;
		DWORD	dwSize = pbmi->bmiHeader.biSizeImage;

		rect.left  = 0;
		rect.top   = 0;
		rect.right = pGIF->imgHeader.wWidth;
		rect.bottom= pGIF->imgHeader.wHeight;
		hr = GIFGetDIBData(&rect, &dwSize, (BYTE*) pvBits);
		if (FAILED(hr))
			goto _err_exit;
		if (pdwWidth)
			*pdwWidth = pGIF->imgHeader.wWidth;
		if (pdwHeight)
			*pdwHeight = pGIF->imgHeader.wHeight;

	}
_err_exit:
	if (bLoaded)
		FreeImage(pGIF);
	DeleteImgInfos();
	if (pbmi != NULL)
		delete pbmi;
	if (pBinData != NULL)
		delete pBinData;
	if (hr == S_OK)
		return hBM;
	else 
		return NULL;
}


//
// Parse the global GIF header
//
static HRESULT ParseGIFHeader(GIFHEADER *gifh )
{
	HRESULT			hr;
	DWORD			dwRead;

	if (gifh == NULL)
		return E_INVALIDARG;

	// Seek to the beginning of the stream (required sometimes)

	hr = BinBufferSeek(0, SEEK_SET, NULL ); 

	// Read the GIF header
	hr = BinBufferRead((BYTE*)gifh, (UINT)sizeof(GIFHEADER), &dwRead);
	if (FAILED(hr))
		return hr;

	if (dwRead != sizeof(GIFHEADER))	// if file is smaller than Gif header
		return S_FALSE;				// it's a safe bet it's not a Gif file


	// Perform some basic checks
	if ((strncmp(gifh->arrSignature, GIF_SIGNATURE, strlen(GIF_SIGNATURE)) != 0) ||
		((strncmp(gifh->arrVersion, GIF_V87a, strlen(GIF_V87a)) != 0) &&
		 (strncmp(gifh->arrVersion, GIF_V89a, strlen(GIF_V89a)) != 0)))
		 return S_FALSE;
	
	// Find out with which version of GIF we have to deal
	if (strncmp(gifh->arrVersion, GIF_V89a, strlen(GIF_V89a)) == 0)
		g_bIs89a = TRUE;
	else
		g_bIs89a = FALSE;

	g_wColorRes	= 1 + ((gifh->fPacked >> GIF_PACKED_SHIFT_COLR_RES) & 
					GIF_PACKED_MASK_COLR_RES);	

	// Check if a Global Color Table is present
	g_bGlobalCT = gifh->fPacked & GIF_PACKED_BIT_GLOBAL_CT;
	if (g_bGlobalCT) 
	{
		g_wGlobalCTSize	=  gifh->fPacked & GIF_PACKED_MASK_CT_SIZE;
		g_bCTSortFlag	=  gifh->fPacked & GIF_PACKED_BIT_CT_SORT;

		g_nGlobalCTEntries = (1 << (g_wGlobalCTSize + 1));

		// Some seem to have a wrong ColorRes field, so let's set it here
		switch (g_nGlobalCTEntries) {
		case 2:
			g_wColorRes = 1;
			break;
		case 4:
			g_wColorRes = 2;
			break;
		case 8:
			g_wColorRes = 3;
			break;
		case 16:
			g_wColorRes = 4;
			break;
		case 256:
			g_wColorRes = 8;
			break;
		}

		DWORD dwSeek =  g_nGlobalCTEntries * sizeof(GIFCOLORTABLE);
		hr = BinBufferSeek(dwSeek, SEEK_CUR, NULL);
		if (FAILED(hr))
			return hr;
	} else {
		g_wGlobalCTSize	= 0;
		g_bCTSortFlag	= FALSE;
		g_nGlobalCTEntries = 0;
	}
	return S_OK;			// Ok, it's GIF and we got the header
}





//
//	Seek to a specific GIF Image inside the GIF File. Defaut image index is 0.
//  NOTE: NO FILE SEEK DONE HERE!
static HRESULT SeekGifImage(DWORD nImgIndex = 0)
{
	DWORD			i, j;

	// Set the current image index to the given index
	// Note: we don't do a file see here. That is done every time we need
	// to access the encoded dated. Before scan lines are read a call to
	// PrepareToGetScanLines() is made. The file seek is done in there.
	
	// Initialize the Row Map Table
	GIFIMGINFO *pImgInfo = GetImgInfo(nImgIndex);

	if (pImgInfo == NULL)
		return E_UNEXPECTED;

	WORD wHeight = (pImgInfo->imgHeader.wHeight >= MAX_LINES) ? MAX_LINES : pImgInfo->imgHeader.wHeight;

	if (pImgInfo->bInterlace) {
		// Initialize each array element 
	
		for (j = 0, i = 0; i < wHeight; j++, i += 8)
			pImgInfo->arrMappedRowTable[j] = i;
		pImgInfo->arrLastLineInScan[0] = (WORD)j - 1;
		for (i = 4; i < wHeight; j++, i += 8)
			pImgInfo->arrMappedRowTable[j] = i;
		pImgInfo->arrLastLineInScan[1] = (WORD)j - 1;
		for (i = 2; i < wHeight; j++, i += 4)
			pImgInfo->arrMappedRowTable[j] = i;
		pImgInfo->arrLastLineInScan[3] = (WORD)j - 1;
		for (i = 1; i < wHeight; j++, i += 2)
			pImgInfo->arrMappedRowTable[j] = i;
		pImgInfo->arrLastLineInScan[4] = (WORD)j - 1;
	} else {
		for (i = 0, j = 0; i < wHeight; j++, i += 1)
			pImgInfo->arrMappedRowTable[j] = i;
		pImgInfo->arrLastLineInScan[0] = (WORD)j - 1;
	}	


	return S_OK;
}

static HRESULT ExploreGifImages(void)
{
	HRESULT			hr;
	BYTE			id;
#if 0
	BYTE			len;
	BYTE			cBuf;
	DWORD			n, m;
#endif
	DWORD			y;
	DWORD			nImgCnt;
	WORD			wEOI;
	GIFIMGINFO		*pImgInfo;
	
	// Since we call GetScanLine we have to set up these values in order to use it!
	if (g_wColorRes == 1) {
		g_wDIBType = PRISM_DIB_MONOCHROME;
		g_wPixelsPerByte = 8;
	} else if (g_wColorRes <= 4) {
		g_wDIBType = PRISM_DIB_4BITPALETTE;
		g_wPixelsPerByte = 2;
	} else {
		g_wDIBType = PRISM_DIB_8BITPALETTE;
		g_wPixelsPerByte = 1;
	}
	 g_wBytesPerPixel = 1;

	// 
	// Position file pointer immediaetely after GIF header
	//
	hr = BinBufferSeek(sizeof(GIFHEADER), STREAM_SEEK_SET, NULL);
	if (FAILED(hr))
		return hr;

	// If a Global Color Table is present, skip it
	if (g_bGlobalCT) {
		hr = BinBufferSeek(g_nGlobalCTEntries * sizeof(GIFCOLORTABLE), STREAM_SEEK_CUR, NULL);
		if (FAILED(hr))
			return hr;
	}
	//
	// Get information about all images in this file
	//
	nImgCnt			= 0;
	g_bTrailerInfo  = FALSE;
	
	while (TRUE) {
		// read extension introducer or end of data marker
		do {
			hr = BinBufferRead(&id, 1, NULL);
			if (FAILED(hr))
				return hr;
		} while (id == 0x00);		// skip zero length blocks
		// Check for end of data
		if (id == GIF_TRAILER)
			break;
		
		// HACK FOR THIS VERSION: ONLY DEAL WITH FIRST IMAGE
		if (nImgCnt >= 1)
			break;
		// Make room for another image
		if ((nImgCnt+1) > g_1stImgInfo.nCount) {
			pImgInfo = NewImgInfo(); 
			if (pImgInfo == NULL)
				return E_UNEXPECTED;
		} else {
			pImgInfo = GetImgInfo(nImgCnt);
			if (pImgInfo == NULL)
				return E_UNEXPECTED;
			// - Since we reset the size I don't think we need to remove
			// - the old arrays here. Will leave this in here as a comment
			// - in case we are wrong: (07/18/97 hmk)
			// -
			// - for (i = 0; i < pImgInfo->arrTextExtensions.GetSize(); i++)
			// -	pImgInfo->arrTextExtensions[i].arrTextData.RemoveAll();
			// - for (i = 0; i < pImgInfo->arrCommentExtensions.GetSize(); i++)
			// -	pImgInfo->arrCommentExtensions[i].arrCommentData.RemoveAll();
			// - for (i = 0; i < pImgInfo->arrAppExtensions.GetSize(); i++)
			// - 	pImgInfo->arrAppExtensions[i].arrAppData.RemoveAll();
			// -	pImgInfo->arrMappedRowTable.RemoveAll();
			// - pImgInfo->arrGraphControls.RemoveAll();
			// - pImgInfo->arrTextExtensions.RemoveAll();
			// - pImgInfo->arrAppExtensions.RemoveAll();
			// - pImgInfo->arrCommentExtensions.RemoveAll();
		}


		// LOOK for extentions
		// HACK FOR THIS VERSION: EXT. NOT NEEDED!!!
#if 0
		while (id == GIF_EXTENSION) {
			hr = pStream->Read(&id, 1, NULL);		// Label
			if (FAILED(hr))
				return hr;

			switch (id) {
			case GIF_EXT_GRAPHICS:
				// Create new Graphic Control Entry
				try {
					pImgInfo->arrGraphControls.SetSize((n = pImgInfo->arrGraphControls.GetSize())+1);
				}
				catch(CMemoryException) {
					return E_OUTOFMEMORY;
				}
				// Ignore length field
				hr = pStream->Read(&len, 1, NULL);
				if (FAILED(hr))
					return hr;
				// Read Packed field and parse it...
				hr = pStream->Read(&cBuf, 1, NULL);	
				if (FAILED(hr))
					return hr;
				pImgInfo->arrGraphControls[n].bTransparentColor = cBuf & 0x01;
				pImgInfo->arrGraphControls[n].bUserInput        = (cBuf & 0x02) ? 1 : 0;
				pImgInfo->arrGraphControls[n].wDisposalMethod   = (cBuf >> 2) & 0x03;
				// Read Delay Time
				
				hr = pStream->Read(&pImgInfo->arrGraphControls[n].wDelayTime, 2, NULL);
				if (FAILED(hr))
					return hr;
				// Read Color Index
				hr = pStream->Read(&pImgInfo->arrGraphControls[n].cColorIndex, 1, NULL);
				if (FAILED(hr))
					return hr;

				// Skip Terminator
				hr = pStream->Read(&cBuf, 1, NULL);
				if (FAILED(hr))
					return hr;
				break;
			case GIF_EXT_TEXT:
				// Create new Text Entry
				try {
					pImgInfo->arrTextExtensions.SetSize((n = pImgInfo->arrTextExtensions.GetSize())+1);
				}
				catch(CMemoryException) {
					return E_OUTOFMEMORY;
				}

				// Skip lenght field
				hr = pStream->Read(&len, 1, NULL);
				if (FAILED(hr))
					return hr;
				// Read text extensions fields
				hr = pStream->Read(&pImgInfo->arrTextExtensions[n], len, NULL);
				if (FAILED(hr))
					return hr;
				// Read actual text data
				hr = pStream->Read(&len, 1, NULL);
				if (FAILED(hr))
					return hr;
				while (len != 0) {
					m = pImgInfo->arrTextExtensions[n].arrTextData.GetSize();
					try {
						pImgInfo->arrTextExtensions[n].arrTextData.SetSize(m+len);
					}
					catch(CMemoryException) {
						return E_OUTOFMEMORY;
					}

					hr = pStream->Read(&pImgInfo->arrTextExtensions[n].arrTextData[m], len, NULL);
					if (FAILED(hr))
						return hr;
					hr = pStream->Read(&len, 1, NULL);
					if (FAILED(hr))
						return hr;
				}
				break;
			case GIF_EXT_APP:
				// Create new App Entry
				try {
					pImgInfo->arrAppExtensions.SetSize((n = pImgInfo->arrAppExtensions.GetSize())+1);
				}
				catch(CMemoryException) {
					return E_OUTOFMEMORY;
				}

				// Skip lenght field
				hr = pStream->Read(&len, 1, NULL);
				if (FAILED(hr))
					return hr;
				// Read app extensions fields
				hr = pStream->Read(&pImgInfo->arrAppExtensions[n], len, NULL);
				if (FAILED(hr))
					return hr;
				// Read actual app data
				hr = pStream->Read(&len, 1, NULL);
				if (FAILED(hr))
					return hr;
				while (len != 0) {
					m = pImgInfo->arrAppExtensions[n].arrAppData.GetSize();
					try {
						pImgInfo->arrAppExtensions[n].arrAppData.SetSize(m+len);
					}
					catch(CMemoryException) {
						return E_OUTOFMEMORY;
					}

					hr = pStream->Read(&pImgInfo->arrAppExtensions[n].arrAppData[m], len, NULL);
					if (FAILED(hr))
						return hr;
					hr = pStream->Read(&len, 1, NULL);
					if (FAILED(hr))
						return hr;
				}
				break;
			case GIF_EXT_COMMENT:
				// Create new Comment Entry
				try {
					pImgInfo->arrCommentExtensions.SetSize((n = pImgInfo->arrCommentExtensions.GetSize())+1);
				}
				catch(CMemoryException) {
					return E_OUTOFMEMORY;
				}

				// Read actual comment text data
				hr = pStream->Read(&len, 1, NULL);
				if (FAILED(hr))
					return hr;
				while (len != 0) {
					m = pImgInfo->arrCommentExtensions[n].arrCommentData.GetSize();
					try {
						pImgInfo->arrCommentExtensions[n].arrCommentData.SetSize(m+len);
					}
					catch(CMemoryException) {
						return E_OUTOFMEMORY;
					}
					hr = pStream->Read(&pImgInfo->arrCommentExtensions[n].arrCommentData[m], len, NULL);
					if (FAILED(hr))
						return hr;
					hr = pStream->Read(&len, 1, NULL);
					if (FAILED(hr))
						return hr;
				}
				break;
			}
			hr = pStream->Read(&id, 1, NULL);	
			if (FAILED(hr))
				return hr;
		}
#endif
		// Check if we are done already
		if (id == GIF_TRAILER) {
		// NOTE: hmk 05/06/97
		// I don't think we need this anymore since we start with an image index 
		// equal to 0. If this line is there, some images don't load. So we leave it
		// out for now unless we later run into a new problem where the number of
		// images is larger by one than it should be. If this happens, we will have
		// to rethink the algorithm of this function!
#if 0
			nImgCnt--;				// last one is not an image
#endif
			//
			// Mark that we have an additional entry in the
			// g_arrImgInfo arrary containing GIF Extesion infos
			g_bTrailerInfo = TRUE;	

			break;			// get out of loop it 
		}
		// No more extensions, so go back one byte
		hr = BinBufferSeek(-1, STREAM_SEEK_CUR, NULL);
		if (FAILED(hr))
			return hr;
		///////////////////////////
		//	Read IMG Header
		//
		DWORD dwPos;
		hr = BinBufferSeek(0, STREAM_SEEK_CUR, &dwPos );
		if (FAILED(hr))
			return hr;
		pImgInfo->dwImgHeadFilePos = dwPos;

		// PORT: Structure byte ordering
		BinBufferRead((BYTE*)&pImgInfo->imgHeader, sizeof(GIFIMGDESC), NULL);
		
		WORD c = pImgInfo->imgHeader.idSeperator;
		WORD d = pImgInfo->imgHeader.idSeperator;
		//
		//	Check Seperator
		//

		if (pImgInfo->imgHeader.idSeperator != GIF_SEPERATOR) {
			if (nImgCnt >= 1) {
				break; 
			} else {
				//TRACE("Unknown image seperator found.\n");
				return S_FALSE;
			}
		}
		//	Unpack Logical Screen Descriptor
		pImgInfo->bLocalCT     =  pImgInfo->imgHeader.fPacked & GIFIMG_PACKED_BIT_LCL_CT;
		pImgInfo->wLocalCTSize = (pImgInfo->imgHeader.fPacked >> GIFIMG_PACKED_SHIFT_LCL_CT_SIZE) & GIFIMG_PACKED_MASK_LCL_CT_SIZE;
		pImgInfo->bLocalCTSort =  pImgInfo->imgHeader.fPacked & GIFIMG_PACKED_BIT_SORT;
		pImgInfo->bInterlace   =  pImgInfo->imgHeader.fPacked & GIFIMG_PACKED_BIT_INTERLACE;

		pImgInfo->nLocalCTEntries = (1 << (pImgInfo->wLocalCTSize + 1));
		pImgInfo->dwLineAdvance = ((((g_wBytesPerPixel * pImgInfo->imgHeader.wWidth)+ g_wPixelsPerByte - 1) / g_wPixelsPerByte)+1)&0xFFFFFFFE;


		// If present, get position of Local Color Table
		if (pImgInfo->bLocalCT) {
			DWORD dwPos;
			hr = BinBufferSeek(0, STREAM_SEEK_CUR, &dwPos );
			if (FAILED(hr))
				return hr;
			pImgInfo->dwLocalCTFilePos = dwPos;

			hr = BinBufferSeek(pImgInfo->nLocalCTEntries * sizeof(GIFCOLORTABLE), STREAM_SEEK_CUR, NULL);
			if (FAILED(hr))
				return hr;
		} else
			pImgInfo->dwLocalCTFilePos = 0;

		// Get file position of start of image data
		hr = BinBufferSeek(0, STREAM_SEEK_CUR, &dwPos );
		if (FAILED(hr))
			return hr;
		pImgInfo->dwImgDataFilePos = dwPos;


		// Unfortunately we have to read and decode the whole image in order
		// to skip it.
		WORD wWidth= pImgInfo->imgHeader.wWidth;


		// NOTE: We need to check for error code here!!!!! and also in the
		//       loop!!!
		PrepareToGetScanLines(pImgInfo);
		for (y = 0; y < pImgInfo->imgHeader.wHeight; y++) {
			hr = SkipOverScanLine(wWidth);
			if (FAILED(hr)) {
					return hr;
			}
		}
		g_gifDecoder.ReleaseDecoder();

		// +++ HMK 12/22/97 - added after converting CLOCK.AVI to GIF & did not load
		// Extra EOI code there? -> if so, we skip
		hr = BinBufferRead((BYTE*)&wEOI, sizeof(WORD), NULL);
		if (FAILED(hr))
			return hr;
		// NOTE: SWAPWORD NEEDED HERE????
		if (wEOI != g_gifDecoder.m_wEndCode) {
			// nop, was NOT there, put data back into stream...
			hr = BinBufferSeek(-2, STREAM_SEEK_CUR, NULL);
			if (FAILED(hr))
				return hr;
		}

		// Read Terminator byte
		hr = BinBufferRead(&id, 1, NULL);
		if (FAILED(hr))
			return hr;
		if (id != 0x00) {
			hr = BinBufferSeek(-1, STREAM_SEEK_CUR, NULL);
			if (FAILED(hr))
				return hr;

		}

		// Advance Image Counter
		nImgCnt++;	
	}
#if 0
async_out:
#endif
	g_nTotalImgCount = nImgCnt;
	g_wBytesPerPixel = 1; 

	return S_OK;
}


// Call this every time you want to get scan lines
// This positions the file pointer to the encoded image data.
//
// NOTE: Here we could implement something that when the reading is done on
// an interlaced image we read the whole image into a virtual buffer and get
// the scan lines from there to speed things up.


static HRESULT PrepareToGetScanLines(GIFIMGINFO *pGIFImage)
{
	BYTE nInitCodeSize;

	// Check for Graph Control Extenions
	WORD n		   = 0; //pGIFImage->arrGraphControls.GetSize();
	WORD wDisposal = 0;

	// NOTE: Since this is not the movie module we only handle 
	//       Background Overwrites as the Disposal Method
	// If we are not working on the very first image and graph extensions
	// exist, we have to use the disposal method specified. According to the
	// spec. only one Graph Control Extension should exist per image. So we 
	// use only the first one.
#if 0 // HACK: NOT NEEDED
	if ((n > 0))
		wDisposal = pGIFImage->arrGraphControls[0].wDisposalMethod;
#endif
	switch (wDisposal) {
	case 2:
		// TODO:	Worry about this again when we do the movie interface
		//			Had problems when using it on the MrDebug GIF from Don's
		//			www.icwhen.com web page	
		g_bBGColorOverWrite = FALSE; // TRUE; -- 4/30/98 changed to False to avoid problems for now...
		break;
	default:
		g_bBGColorOverWrite = FALSE;
		break;
	}

	//
	// Seek to beginning of encoded GIF image
	//
	HRESULT hr;
	hr = BinBufferSeek(pGIFImage->dwImgDataFilePos, SEEK_SET, NULL);

	if (FAILED(hr))
		return hr;

	hr = BinBufferRead(&nInitCodeSize, 1, NULL);
	if (FAILED(hr))
		return hr;
    if ((nInitCodeSize < 2) || (9 < nInitCodeSize))
		return S_FALSE;

	g_gifDecoder.InitDecoder(BinBufferGetByte, (WORD) nInitCodeSize);

	return S_OK;
}


// This is the interface to the decoder to read a scan line.
static HRESULT GetScanLine(LPBYTE pLineBuffer, WORD wWidth)
{
	WORD	pixel, bit;
	int		i, j;
	HRESULT hr;
	
	if (g_bBGColorOverWrite) {
		if (pLineBuffer == NULL)
			return S_OK;

		// Fill line with background color
		for (i = 0; i < (wWidth / g_wPixelsPerByte); i++) {
			switch (g_wDIBType) {
			case PRISM_DIB_MONOCHROME:
				if (g_gifHeader.nBackgroundColor)
						pLineBuffer[i] = 0xff;
				else
						pLineBuffer[i] = 0x00;
				break;
			case PRISM_DIB_4BITPALETTE:
				pLineBuffer[i] = g_gifHeader.nBackgroundColor;
				pLineBuffer[i] |= (g_gifHeader.nBackgroundColor << 4);
				break;
			default:
				pLineBuffer[i] = g_gifHeader.nBackgroundColor;
				break;
			}
		}
		if ((j = wWidth % g_wPixelsPerByte)) {
			switch (g_wDIBType) {
			case PRISM_DIB_MONOCHROME:
				pLineBuffer[i] = 0;
				for (i = 0; i < j; i++)
					pLineBuffer[i] |= (g_gifHeader.nBackgroundColor << (7 - i));
				break;
			case PRISM_DIB_4BITPALETTE:
				pLineBuffer[i] = g_gifHeader.nBackgroundColor << 4;
				break;
			}
		}
	} else {
		// Fill the line depending on the DIB type
		for (i = 0; i < (wWidth / g_wPixelsPerByte); i++) {
			pixel = 0;
			switch (g_wDIBType) {
			case PRISM_DIB_MONOCHROME:
				for (j = 0; j < 8; j++) {
					hr = g_gifDecoder.DecoderReadByte(&bit);
					if (FAILED(hr))
						return hr;
					pixel |= (BYTE) bit << (7 - j);
				}
				if (pLineBuffer != NULL)
					pLineBuffer[i] = (BYTE) pixel;
				break;
			case PRISM_DIB_4BITPALETTE:
				hr = g_gifDecoder.DecoderReadByte(&pixel);
				if (FAILED(hr))
					return hr;
				if (pLineBuffer != NULL)	
					pLineBuffer[i] = (BYTE) pixel << 4;
				hr = g_gifDecoder.DecoderReadByte(&pixel);
				if (FAILED(hr))
					return hr;
				if (pLineBuffer != NULL)
					pLineBuffer[i] |= (BYTE) pixel;
				break;
			default:
				hr = g_gifDecoder.DecoderReadByte(&pixel);
				if (FAILED(hr))
					return hr;
				if (pLineBuffer != NULL)
					pLineBuffer[i] = (BYTE) pixel;
				break;
			}
		}
		// Do the remaining pixel(s)
		for (j = 0, pixel = 0; j < (wWidth % g_wPixelsPerByte); j++) {
			switch (g_wDIBType) {
			case PRISM_DIB_MONOCHROME:
				hr = g_gifDecoder.DecoderReadByte(&bit);
				if (FAILED(hr))
					return hr;
				pixel |= (BYTE) bit << (7 - j);
				break;
			case PRISM_DIB_4BITPALETTE:
				hr = g_gifDecoder.DecoderReadByte(&pixel);
				if (FAILED(hr))
					return hr;
				pixel = (BYTE) pixel << 4;
				break;
			}
		}
		if ((j > 0) && pLineBuffer)
			pLineBuffer[i] = (BYTE) pixel;
	}
	return S_OK;
}


static HRESULT SkipOverScanLine(WORD wWidth)
{
	WORD	pixel, bit;
	int		i, j;
	HRESULT hr;
	
		// Fill the line depending on the DIB type

	switch (g_wDIBType) {
	case PRISM_DIB_MONOCHROME:
		for (i = 0; i < (wWidth / g_wPixelsPerByte); i++) {
			for (j = 0; j < 8; j++) {
				hr = g_gifDecoder.DecoderReadByte(&bit);
				if (FAILED(hr))
					return hr;
			}
		}
		break;
	case PRISM_DIB_4BITPALETTE:
		for (i = 0; i < (wWidth / g_wPixelsPerByte); i++) {
			hr = g_gifDecoder.DecoderReadByte(&pixel);
			if (FAILED(hr))
				return hr;
			hr = g_gifDecoder.DecoderReadByte(&pixel);
			if (FAILED(hr))
				return hr;
		}
		break;
	default:
		for (i = 0; i < (wWidth / g_wPixelsPerByte); i++) {
			hr = g_gifDecoder.DecoderReadByte(&pixel);
			if (FAILED(hr))
				return hr;
		}
		break;
	}
	// Do the remaining pixel(s)
	for (j = 0, pixel = 0; j < (wWidth % g_wPixelsPerByte); j++) {
		switch (g_wDIBType) {
		case PRISM_DIB_MONOCHROME:
			hr = g_gifDecoder.DecoderReadByte(&bit);
			if (FAILED(hr))
				return hr;
			break;
		case PRISM_DIB_4BITPALETTE:
			hr = g_gifDecoder.DecoderReadByte(&pixel);
			if (FAILED(hr))
				return hr;
			break;
		}
	}
	return S_OK;
}
//
// GIF IMAGE INFO LIST HELPER FUNCTIONS
//
static GIFIMGINFO *NewImgInfo(void)
{
		GIFIMGINFO *p;
		GIFIMGINFO *pNewMember;

		pNewMember = new GIFIMGINFO;
		if (pNewMember == NULL)
			return NULL;

		for (p = &g_1stImgInfo; ; p = p->pNext)
			if (p->pNext == NULL)
				break;

		p->pNext					= pNewMember;
		pNewMember->pNext			= NULL;
		pNewMember->dwImgHeadFilePos= 0;
		pNewMember->dwImgDataFilePos= 0;
		pNewMember->dwLocalCTFilePos= 0;
		pNewMember->dwLineAdvance	= 0;
		pNewMember->bLocalCT		= FALSE;
		pNewMember->bInterlace		= FALSE;
		pNewMember->bLocalCTSort	= FALSE;
		pNewMember->wLocalCTSize	= 0;
		pNewMember->nLocalCTEntries	= 0;
		pNewMember->wScan			= 0;
		pNewMember->arrLastLineInScan[0] = 0;
		pNewMember->arrLastLineInScan[1] = 0;
		pNewMember->arrLastLineInScan[2] = 0;
		pNewMember->arrLastLineInScan[3] = 0;
		pNewMember->bImageLoaded	= FALSE;
		pNewMember->pImageBuf		= NULL;
		pNewMember->dwImageBufSize	= 0;
		ZeroMemory(&pNewMember->imgHeader, sizeof(GIFIMGDESC));

		g_1stImgInfo.nCount++;
		g_1stImgInfo.pTail = pNewMember;	
		return pNewMember;
}

static GIFIMGINFO *GetImgInfo(DWORD dwIndex)
{
	GIFIMGINFO *p = g_1stImgInfo.pNext;

	if (p == NULL)
		return NULL;

	for (DWORD i = 0; i < dwIndex; i++)
		if ((p = p->pNext) == NULL)
			return NULL;
	return p;
}

static void DeleteImgInfos(void)
{
	GIFIMGINFO *q, *p = g_1stImgInfo.pNext;
	int i = 0;
	while (p) {
#if 0 // HACK: Not Needed
		for (i = 0; i < p->arrTextExtensions.GetSize(); i++)
			p->arrTextExtensions[i].arrTextData.RemoveAll();
		for (i = 0; i < p->arrCommentExtensions.GetSize(); i++)
			p->arrCommentExtensions[i].arrCommentData.RemoveAll();
		for (i = 0; i < p->arrAppExtensions.GetSize(); i++)
			p->arrAppExtensions[i].arrAppData.RemoveAll();
		p->arrMappedRowTable.RemoveAll();
		p->arrGraphControls.RemoveAll();
		p->arrTextExtensions.RemoveAll();
		p->arrAppExtensions.RemoveAll();
		p->arrCommentExtensions.RemoveAll();
#endif
		FreeImage(p);
		q = p;
		p = p->pNext;
		delete q;
	}
	g_1stImgInfo.nCount = 0;
	g_1stImgInfo.pNext  = NULL;
	g_1stImgInfo.pTail  = NULL;
}


static HRESULT LoadImage(GIFIMGINFO *pGIFImage)
{
	// Load the cached image (if available)
	HRESULT hr;

	if (pGIFImage->bImageLoaded)	// already loaded?
		return S_OK;

	if (pGIFImage->pImageBuf == NULL) {
		pGIFImage->bImageLoaded = FALSE;

		// Determine the size of the image buffers
		pGIFImage->dwImageBufSize = pGIFImage->dwLineAdvance * pGIFImage->imgHeader.wHeight;

		// Allocate the buffer memory
		if ((pGIFImage->pImageBuf = (LPBYTE)::HeapAlloc( GetProcessHeap(), 0, pGIFImage->dwImageBufSize )) == NULL)
			return E_OUTOFMEMORY;
	} 
	// Get the image
	hr = LoadImageData(pGIFImage);
	if (FAILED(hr))
		return hr;
	return S_OK;
}

// Release the image data resources

void FreeImage(GIFIMGINFO *pGIFImage)
{
	if( pGIFImage->pImageBuf )
	{
		::HeapFree( GetProcessHeap(), 0, pGIFImage->pImageBuf );
		pGIFImage->pImageBuf = NULL;
	}

	pGIFImage->dwImageBufSize = 0;
	pGIFImage->bImageLoaded	  = FALSE;
}


static HRESULT LoadImageData(GIFIMGINFO *pGIFImage)
{
	HRESULT hr;

	PrepareToGetScanLines(pGIFImage);

	// Process each line
	for (DWORD dwLine, dw = 0; dw < pGIFImage->imgHeader.wHeight; dw++) {
		if (dw > MAX_LINES)
			dwLine = pGIFImage->arrMappedRowTable[dw];
		else
			dwLine = dw;
		hr = GetScanLine(&pGIFImage->pImageBuf[pGIFImage->dwLineAdvance * dwLine], pGIFImage->imgHeader.wWidth);
		if (hr == E_PENDING)
			return S_OK;
		if(FAILED(hr)) {
			return hr;
		}

		if (pGIFImage->bInterlace) {
			int i, x;

			if ((dwLine % 8) == 0)
				x = 7;
			else if (((dwLine - 4) % 8) == 0)
				x = 3;
			else if ((dwLine % 2) == 0)
				x = 1;
			else
				x = 0;

			for (i = 1; i <= x; i++ ){
				if ((dwLine + i) < pGIFImage->imgHeader.wHeight)
					::CopyMemory(&pGIFImage->pImageBuf[pGIFImage->dwLineAdvance * (dwLine + i)], &pGIFImage->pImageBuf[pGIFImage->dwLineAdvance * dwLine], pGIFImage->dwLineAdvance); 
			}
		}
	}
	if ((pGIFImage->bInterlace && (pGIFImage->wScan == 4))) {
		pGIFImage->bImageLoaded = TRUE;
		g_gifDecoder.ReleaseDecoder();
	}
	return S_OK;
}


static HRESULT GIFGetPalette(LPDWORD lpdwSize, RGBQUAD *pPalEntries )
{
	if (lpdwSize == NULL)
		return E_INVALIDARG;

	GIFIMGINFO *pGIFImage = (GIFIMGINFO *) g_1stImgInfo.pNext;

	// If this image has no palette, set the return to 0 and exit with success code
	if(!g_bGlobalCT && !pGIFImage->bLocalCT)
	{
		*lpdwSize = 0;
		return S_OK;
	}

	WORD	nCTEntries;
	if (pGIFImage->bLocalCT)
		nCTEntries = pGIFImage->nLocalCTEntries;
	else
		nCTEntries = g_nGlobalCTEntries;

	// if the image has a palette and the buffer given is too small, return
	DWORD dwPalSize = sizeof(RGBQUAD) * nCTEntries;

	if( *lpdwSize < nCTEntries)
	{
		*lpdwSize = nCTEntries;
		return E_FAIL;
	}
	else
		*lpdwSize = nCTEntries;
	// seek to start of palette
	HRESULT hr;
	if (pGIFImage->bLocalCT) {
		hr = BinBufferSeek(pGIFImage->dwLocalCTFilePos, SEEK_SET, NULL);
	} else {
		hr = BinBufferSeek(sizeof(GIFHEADER), STREAM_SEEK_SET, NULL);
	}
	if (FAILED(hr))
		return hr;

	// ok, slurp the palette entries
	UINT nBytes = sizeof(GIFCOLORTABLE);
	GIFCOLORTABLE color;

	for (int i = 0; i < nCTEntries; i++)
	{
		// PORT: Byte swapping
		hr = BinBufferRead((BYTE*) &color, nBytes, NULL );
		if (FAILED(hr))
			return hr;

		// parse the bits
		pPalEntries[i].rgbBlue  = color.nBlue; 
		pPalEntries[i].rgbGreen = color.nGreen; 
		pPalEntries[i].rgbRed   = color.nRed; 
	}
	return S_OK;
}


static HRESULT GIFGetDIBData(LPRECT lpRect, LPDWORD lpdwSize, LPBYTE lpData)
{

	HRESULT hr;
	if( lpRect == NULL || lpdwSize == NULL )
		return E_INVALIDARG;


	// Get required Image
	GIFIMGINFO *pGIFImage = (GIFIMGINFO *) g_1stImgInfo.pNext;
	// Create a CRECT
	RECT rect;
	rect.left   = lpRect->left;
	rect.top    = lpRect->top;
	rect.right  = lpRect->right;
	rect.bottom = lpRect->bottom;

	// Determine if the rectangle is valid
	// rect.NormalizeRect();
	if( rect.left < 0 || rect.top < 0 ||
		rect.right  > pGIFImage->imgHeader.wWidth || 
		rect.bottom > pGIFImage->imgHeader.wHeight )
		return E_INVALIDARG;

	// Get image into memory...
	hr = LoadImage(pGIFImage);
	if (FAILED(hr))
		return hr;

	// Width of current image
	WORD wWidth    = pGIFImage->imgHeader.wWidth;

	// Real # of bytes per line
	DWORD dwRealBytesPerLine	= (((rect.right - rect.left) * g_wBytesPerPixel) + g_wPixelsPerByte - 1) / g_wPixelsPerByte ;
	DWORD dwRealBytesToMalloc	= (rect.right - rect.left) * g_wBytesPerPixel;

	// # of DIB bytes per line (even multiple of longwords)
	DWORD dwPadBytesPerLine = (dwRealBytesPerLine + 3) & 0xFFFFFFFC;

	// Determine if the buffer the user gave us is big enough
	DWORD dwSize = dwPadBytesPerLine * (rect.bottom - rect.top);

	if (lpData == NULL) {
		*lpdwSize = dwSize;
		return S_OK;
	}
	// if buffer is too small, return
	if (*lpdwSize < dwSize) {
		*lpdwSize = dwSize;
		return E_FAIL;
	}
	else
		*lpdwSize = dwSize;

	// Allocate memory for a scanline
	BYTE *pLine;

	pLine = new BYTE[dwRealBytesToMalloc];
	if (pLine == NULL)
		return E_OUTOFMEMORY;

	////////////////////////////////////////
	int		iXoff = 0;
	BOOL	bEasyCopy =TRUE;

	if (rect.left > 0) {
		if (g_wPixelsPerByte == 8) {			// biBitCount == 8
			if ((rect.left & 0x7) != 0)
				bEasyCopy = FALSE;
			else
				iXoff = rect.left >> 3;
		} else if (g_wPixelsPerByte == 2) {  // biBitCount == 4
			if( (rect.left & 0x1) != 0 )
				bEasyCopy = FALSE;
			else
				iXoff = rect.left >> 1;
		} else
			iXoff = rect.left * g_wBytesPerPixel;
	}
	// Set up the image to be read in the most efficient order
	LONG	lStart;
	LONG	lEnd;
	LONG	lCurrentLine;

	lEnd			= rect.bottom - 1;
	lStart			= rect.top;
	lCurrentLine	= lStart; 

	// Fetch each scanline
	for(;;) {
		
		// Stuff the bits where they belong
		LONG lLine = (rect.bottom - rect.top) - (lCurrentLine - rect.top) - 1;

		if (TRUE || bEasyCopy) {
			::CopyMemory( &lpData[dwPadBytesPerLine * (lLine)], &pGIFImage->pImageBuf[lCurrentLine * pGIFImage->dwLineAdvance + iXoff], dwPadBytesPerLine);
		} else {
#if 0 // Not needed, always an easy copy
			if (g_wPixelsPerByte == 8)
				pThis->CopyMisalignedMonoPixels( &lpData[dwPadBytesPerLine * lLine], &pGIFImage->pImageBuf[lCurrentLine * pGIFImage->dwLineAdvance + iXoff], rect.left, rect.right - rect.left );
			else
				pThis->CopyMisaligned4bitPixels( &lpData[dwPadBytesPerLine * lLine], &pGIFImage->pImageBuf[lCurrentLine * pGIFImage->dwLineAdvance + iXoff], rect.left, rect.right - rect.left );
#endif
		}
		// Have we gotten all the lines we wanted?
		if (lCurrentLine >= lEnd)
			break;
		lCurrentLine++; 
	}

	// delete line buffer memory
	delete pLine;
	pLine = NULL;

	return S_OK;
}



//////////////////////////////////////////////////////////////////////////////////
// Memory I/O Implementation

void InitBinBufferIO(BYTE *pData, DWORD dwSize)
{
	g_pReadBuffer		 = pData;
	g_dwReadBufferSize   = dwSize;
	g_dwSeekIndex		 = 0;
}

HRESULT BinBufferRead(BYTE *pDest, DWORD dwBytesToRead, LPDWORD pdwBytesRead)
{

	if ((g_dwSeekIndex + dwBytesToRead) > g_dwReadBufferSize) {
		dwBytesToRead = ((int)((int)g_dwReadBufferSize - (int) g_dwSeekIndex) < 0) ? 0 : g_dwReadBufferSize - g_dwSeekIndex;
	}

	CopyMemory(pDest, &g_pReadBuffer[g_dwSeekIndex], dwBytesToRead);
	g_dwSeekIndex += dwBytesToRead;

	if (pdwBytesRead != NULL)
		*pdwBytesRead = dwBytesToRead;

	return S_OK;
}

HRESULT BinBufferSeek(LONG lPos, DWORD dwMode, DWORD *lpdwNewPos)
{
	switch (dwMode) {
	case SEEK_SET:
		if (lPos < 0) lPos = 0;
		if (lPos >= (long)g_dwReadBufferSize) lPos = g_dwReadBufferSize - 1;
		g_dwSeekIndex = lPos;
		break;
	case SEEK_CUR:
		g_dwSeekIndex += lPos;
		break;
	case SEEK_END:
		g_dwSeekIndex = (((long)g_dwReadBufferSize - lPos)  < 0) ? 0 : g_dwReadBufferSize - lPos;
		break;
	}
	if (lpdwNewPos != NULL)
		*lpdwNewPos = g_dwSeekIndex;
	return S_OK;
}

BYTE BinBufferGetByte()
{
	if (g_dwSeekIndex >= g_dwReadBufferSize)
		return 0;
	return g_pReadBuffer[g_dwSeekIndex++];
}
