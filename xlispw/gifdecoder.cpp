/////////////////////////////////////////////////////////////////////////////
// IX_Gifdecoder.cpp - Gif Decoder

// **************************************************************************
// * WARNING: You will need an LZW patent license from Unisys in order to   *
// * use this file legally in any commercial or shareware application.      *
// **************************************************************************

#include "windows.h"
#include "IX_Gifdecoder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// PUBLIC INTERFACE FUNCTIONS
//


// Init GIF Decoder
// Input:  File Pointer To Read From
//		   Initial Code Size (usually the byte right in front of the data)
// Output: S_OK	
HRESULT	CIX_GifDecoder::InitDecoder(BYTE (*pGetByteCallBack)(void), WORD wCodeSize) 
{ 
	m_pCurrentCachePtr = &m_arrCache[READLINE_CACHESIZE]; 
	m_dwBytesInCache   = READLINE_CACHESIZE;

	m_pGetByteCallBack = pGetByteCallBack; 
	m_wInputCodeSize = wCodeSize; 
	InitLZWCode(); 

	return S_OK;
};

HRESULT	CIX_GifDecoder::ReleaseDecoder(void) 
{ 
	return S_OK;
};

// Read an LZW-compressed byte 
// Input:  Pointer To Data Buffer
// Output: Status
HRESULT CIX_GifDecoder::DecoderReadByte(WORD *pData)
{
  register WORD wCode;		// current working code 
  WORD			wIncode;	// saves actual input code 
  HRESULT		hr;

  if (pData == NULL)
	  return E_INVALIDARG;

  // First time, just eat the expected Clear code(s) and return next code, 
  // which is expected to be a raw byte. 
  if (m_bFirstTime) {
    m_bFirstTime = FALSE;
    wCode = m_wClearCode;		// enables sharing code with Clear case 
  } else {

    // If any codes are stacked from a previously read symbol, return them
    if (m_pStack > m_arrSymbolStack) {
      *pData =  (WORD) *(-- m_pStack);
	  return S_OK;
	}
	 
    // Time to read a new symbol 
    hr = GetCode(&wCode);
	if (FAILED(hr))
		return hr;
  }

  if (wCode == m_wClearCode) {
    // Reinit state, swallow any extra Clear codes, and 
    // return next code, which is expected to be a raw byte. 
    ReInitLZWCode();
    do {
      hr = GetCode(&wCode);
	  if (FAILED(hr))
		return hr;
    } while (wCode == m_wClearCode);
    if (wCode > m_wClearCode) { // make sure it is a raw byte
	  // TRACE("WARNING: Gif decoder reports bad data.\n");
      wCode = 0;				// use something valid 
    }
    // Make firstcode, oldcode valid! 
    m_wFirstCode = m_wOldCode = wCode;

	// Deliver Data
	*pData = wCode;

    return S_OK;
  }

  if (wCode == m_wEndCode) {
    // Skip the rest of the image, unless GetCode already read terminator 
    if (!m_bOutOfBlocks) {
      hr = SkipDataBlocks();
	  if (FAILED(hr))
		  return hr;
      m_bOutOfBlocks = TRUE;
    }
    // Complain that there's not enough data
	// TRACE("WARNING: Gif decoder reports not enough data.\n");
    // Pad data with 0's 

	// Deliver Result

	*pData = 0;				// fake something usable 

    return S_OK;				
  }

  // Got normal raw byte or LZW symbol 
  wIncode = wCode;		// save for a moment 
  
  if (wCode >= m_wMaxCode) { // special case for not-yet-defined symbol 
    // code == max_code is OK; anything bigger is bad data 
    if (wCode > m_wMaxCode) {
	  // TRACE("WARNING: Gif decoder reports bad data.\n");
      wIncode = 0;		// prevent creation of loops in symbol table 
    }
    // This symbol will be defined as oldcode/firstcode 
    *(m_pStack++) = (BYTE) m_wFirstCode;
	wCode = m_wOldCode;
  }

  // If it's a symbol, expand it into the stack 
  while (wCode >= m_wClearCode) {
    *(m_pStack++) = m_arrSymbolTail[wCode]; // tail is a byte value 
    wCode		  = m_arrSymbolHead[wCode]; // head is another LZW symbol 
  }
  // At this point code just represents a raw byte 
  m_wFirstCode = wCode;		// save for possible future use 

  // If there's room in table,
  if ((wCode = m_wMaxCode) < LZW_TABLE_SIZE) {
    // Define a new symbol = prev sym + head of this sym's expansion 
	m_arrSymbolHead[wCode] = m_wOldCode;
    m_arrSymbolTail[wCode] = (BYTE) m_wFirstCode;
    m_wMaxCode++;
    // Is it time to increase code_size? 
    if ((m_wMaxCode >= m_wLimitCode) && (m_wCodeSize < MAX_LZW_BITS)) {
      m_wCodeSize++;
      m_wLimitCode <<= 1;	// keep equal to 2^code_size 
    }
  }
  
  m_wOldCode = wIncode;		// save last input symbol for future use 
  *pData = m_wFirstCode;	// return first byte of symbol's expansion 

  return S_OK;
}


///////////////////////////////////////////////////////////
// PRIVATE FUNCTIONS
//


void CIX_GifDecoder::InitLZWCode(void)
{
		// GetCode initialization
		m_wLastByte		= 2;		// Make safe to "recopy last two bytes"
		m_wLastBit		= 0;		// Nothing in the buffer
		m_wCurrentBit	= 0;		// Force buffer load on first call
		m_bOutOfBlocks	= FALSE;

		// LZWReadByte initialization:
		// compute special code values (note that these do not change later) 
		m_wClearCode	= 1 << m_wInputCodeSize;
		m_wEndCode		= m_wClearCode + 1;
		m_bFirstTime	= TRUE;
		m_pStack		= m_arrSymbolStack;
}


// (Re)initialize LZW state; shared code for startup and Clear processing
void CIX_GifDecoder::ReInitLZWCode(void)
{

	m_wCodeSize		= m_wInputCodeSize + 1;
	m_wLimitCode	= m_wClearCode << 1;		// 2^code_size 
	m_wMaxCode		= m_wClearCode + 2;			// First unused code value
	m_pStack		= m_arrSymbolStack;			// Init stack to empty
}


// Fetch the next code_size bits from the GIF data 
// We assume code_size is less than 16 
HRESULT CIX_GifDecoder::GetCode(WORD *pData)
{
  register LONG lAccum;
  WORD			wOffs;
  WORD			wRet;
  WORD			wCount;
  HRESULT		hr;

  if (pData == NULL)
	  return E_INVALIDARG;

  while ( (m_wCurrentBit + m_wCodeSize) > m_wLastBit) {
    // Time to reload the buffer 
    if (m_bOutOfBlocks) {
	  // TRACE("WARNING: GIF Decoder reports no more data (1).\n");
      return m_wEndCode;	// fake something useful 
    }
    // Preserve last two bytes of what we have -- assume code_size <= 16 
    m_arrCodeBuffer[0] = m_arrCodeBuffer[m_wLastByte - 2];
    m_arrCodeBuffer[1] = m_arrCodeBuffer[m_wLastByte - 1];
    // Load more bytes; set flag if we reach the terminator block
    hr = GetDataBlock(&m_arrCodeBuffer[2], &wCount);
	if (FAILED(hr))
		return hr;

	if (wCount == 0) {
      m_bOutOfBlocks = TRUE;
	  // TRACE("WARNING: GIF Decoder reports no more data (2).\n"); 
	  *pData = m_wEndCode;	// fake something useful 
	  return S_OK;
    }
    // Reset counters 
    m_wCurrentBit	= (m_wCurrentBit - m_wLastBit) + 16;
    m_wLastByte		= 2 + wCount;
    m_wLastBit		= m_wLastByte * 8;
  }

  // Form up next 24 bits in accum 
  wOffs = m_wCurrentBit >> 3;	// byte containing cur_bit 

  lAccum  =  m_arrCodeBuffer[wOffs+2] & 0xFF;
  lAccum <<= 8;
  lAccum |= (m_arrCodeBuffer[wOffs+1] & 0xFF);
  lAccum <<= 8;
  lAccum |= (m_arrCodeBuffer[wOffs]   & 0xFF);


  // Right-align cur_bit in accum, then mask off desired number of bits 
  lAccum >>= (m_wCurrentBit & 7);
  wRet = ((WORD) lAccum) & ((1 << m_wCodeSize) - 1);
  
  m_wCurrentBit += m_wCodeSize;
  *pData = wRet;
  return S_OK;
}



// Skip a series of data blocks, until a block terminator is found 
HRESULT CIX_GifDecoder::SkipDataBlocks(void)
{
	WORD	wCount;
	BYTE	arrBuffer[256];
	HRESULT hr;

	do {
		hr = GetDataBlock(arrBuffer, &wCount);	// Skip
		if (FAILED(hr))
			return hr;
	} while (wCount > 0);
	return hr;
}

// Read a GIF data block, which has a leading count byte 
// A zero-length block marks the end of a data block sequence

HRESULT CIX_GifDecoder::GetDataBlock(BYTE *pBuffer, WORD *pCount)
{
  WORD		wCount;
  HRESULT	hr;


  if (pBuffer == NULL || pCount == NULL)
	  return E_INVALIDARG;

  hr = ReadByte(&wCount);
  if (FAILED(hr))
	return hr;

  if (wCount > 0) {
    hr = ReadBytes(pBuffer, wCount);
	if (FAILED(hr)) {
	  //TRACE("WARNING: Gif Decoder reports read error\n.");
	  return hr;
	}
  }
  *pCount = wCount;
  return S_OK;;
}


// Read single byte
HRESULT CIX_GifDecoder::ReadByte(WORD *pData)
{
	if (pData == NULL)
		return E_INVALIDARG;

	*pData = (WORD) m_pGetByteCallBack();

	return S_OK;
}

// Read multiple bytes
HRESULT CIX_GifDecoder::ReadBytes(BYTE *pBuffer, WORD wCount)
{
	if (pBuffer == NULL)
		return E_INVALIDARG;

	for (int i = 0; i < (int)wCount; i++)
		pBuffer[i] = m_pGetByteCallBack();

	return S_OK;
}



