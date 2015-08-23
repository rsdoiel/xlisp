// IX_GifDecoder.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIX_GifDecoder 


// GIF BUFFER SIZES
#define CODEBUFFER_SIZE		256+4
#define SYMBOLTABLE_SIZE	4096	

// GIF CODE DEFS
#define	MAX_LZW_BITS		12					/* maximum LZW code size	 */
#define LZW_TABLE_SIZE		(1<<MAX_LZW_BITS)	/* # of possible LZW symbols */

#define READLINE_CACHESIZE (1024)

// The GIF DECODER is based on LZW compression

// LZW decompression tables look like this:
//   symbol_head[K] = prefix symbol of any LZW symbol K (0..LZW_TABLE_SIZE-1)
//   symbol_tail[K] = suffix byte   of any LZW symbol K (0..LZW_TABLE_SIZE-1)
// Note that entries 0..end_code of the above tables are not used,
// since those symbols represent raw bytes or special codes.
//
// The stack represents the not-yet-used expansion of the last LZW symbol.
// In the worst case, a symbol could expand to as many bytes as there are
// LZW symbols, so we allocate LZW_TABLE_SIZE bytes for the stack.
// (This is conservative since that number includes the raw-byte symbols.)
//
// The tables are allocated from FAR heap space since they would use up
// rather a lot of the near data space in a PC.

class CIX_GifDecoder
{

	// Constructor
public:
	CIX_GifDecoder() { InitLZWCode(); };     // Constructor

	// Private Attributes
private:
	// File to access
	BYTE		(*m_pGetByteCallBack)(void);
	// State for GetCode and LZWReadByte
	BYTE		m_arrCodeBuffer[CODEBUFFER_SIZE];	// Current input data block
	WORD		m_wLastByte;						// # of bytes in code buffer
	WORD		m_wLastBit;							// # of bits in code buffer
	WORD		m_wCurrentBit;						// Next bit index to read
	BOOLEAN		m_bOutOfBlocks;						// TRUE if hit terminator data block
	WORD		m_wInputCodeSize;					// Code size given in GIF file
	WORD		m_wClearCode;						// Value for clear code
	WORD		m_wCodeSize;						// Current actual code size
	WORD		m_wLimitCode;						// 2^wCodeSize
	WORD		m_wMaxCode;							// First unused code value
	BOOLEAN		m_bFirstTime;						// Flags first call to LZWReadByte

	// State of LZWReadByte
	WORD		m_wOldCode;							// Previous LZW symbol
	WORD		m_wFirstCode;						// First byte of old code's expansion

	// LZW symbol table and expansion stack
	WORD		m_arrSymbolHead [SYMBOLTABLE_SIZE];				// Table of prefix symbols
	BYTE		m_arrSymbolTail [SYMBOLTABLE_SIZE];				// Table of suffix bytes
	BYTE		m_arrSymbolStack[SYMBOLTABLE_SIZE];				// Stack for symbol expansion
	BYTE*		m_pStack;										// Stack pointer
	
	BYTE			m_arrCache[READLINE_CACHESIZE];
	BYTE*			m_pCurrentCachePtr;
	DWORD			m_dwBytesInCache;

	void		InitLZWCode();
	void		ReInitLZWCode();
	HRESULT		SkipDataBlocks();
	HRESULT		GetCode(WORD *);
	HRESULT		GetDataBlock(BYTE *, WORD *);
	HRESULT		ReadByte(WORD *);
	HRESULT		ReadBytes(BYTE *, WORD);

// Operations
public:
	HRESULT		InitDecoder(BYTE (*pGetByteCallBack)(void), WORD wCodeSize);
	HRESULT		DecoderReadByte(WORD *pData);
	HRESULT		ReleaseDecoder(void);

// Data - public because we want to check if an additional EndCode is stuck to the end
	WORD		m_wEndCode;							// Value for end code
};
