//
// Basic 64 Decoder
//

#include <stdio.h>

static unsigned char b64_bin[256]; /* => 0..63, pad=64, inv=65, else ign */

static char *pData;
static unsigned char *pOutput;

static int do_decode(void);

/*
  -------------
  init_b64bin()
  -------------

  setup B64 (ascii) => binary table
*/
int b64bin(char *pIn, unsigned char *pOut)
  {
  int i,j;

  pData = pIn;
  pOutput = pOut;

  for(i = 0; i <= 32; i++) b64_bin[i] = 66;       /* ignore controls */
  for(i = 33; i < 127; i++) b64_bin[i] = 65;      /* printable => inv */
  for(i = 127; i <= 160; i++) b64_bin[i] = 66;    /* ignore controls */
  for(i = 161; i < 256; i++) b64_bin[i] = 65;     /* printable => inv */

  j = 0;
  for(i = 'A'; i <= 'Z'; i++, j++) b64_bin[i] = j;
  for(i = 'a'; i <= 'z'; i++, j++) b64_bin[i] = j;
  for(i = '0'; i <= '9'; i++, j++) b64_bin[i] = j;
  b64_bin['+'] = j++;
  b64_bin['/'] = j++;

  b64_bin['='] = 64;

  return do_decode();

  }


/*
  ------------
  GNC_b64bin()
  ------------

  get next 6-bit group, or pad
*/
static int GNC_b64bin(unsigned int *ip)
  {
  int c;

  do
    {
    c = *pData++;
    if (c == '\0') return EOF;
    *ip = b64_bin[c & 0xFF];
    } while(*ip > 65);

  return 0;
  }


/*
  -----------
  do_decode()
  -----------

  decode => stdout
*/
static int do_decode(void)
  {
  int /*logical*/ done;
  unsigned int v6;
  unsigned int v24;               /* room for >= 24 bits */

  #define ERROR1 return (-1)

  done = 0;

  while(GNC_b64bin(&v6) != EOF)
    {
    if(done) ERROR1;         /* bytes following pad */

    if(v6 > 63) ERROR1;
    v24 = v6;
    if(GNC_b64bin(&v6) == EOF) ERROR1;
    if(v6 > 63) ERROR1;
    v24 = (v24 << 6) | v6;

    *pOutput++ = ((v24 >> (2*6-8)) & 0xFF);

    if(GNC_b64bin(&v6) == EOF) ERROR1;
    if(v6 > 64) ERROR1;      /* invalid */
    if(v6 == 64)
      {                  /* pad after 1 char */
      done = 1;
      if(GNC_b64bin(&v6) == EOF) ERROR1;
      if(v6 != 64) ERROR1;
      }
    else
      {
      v24 = (v24 << 6) | v6;

      *pOutput++ = ((v24 >> (3*6-16)) & 0xFF);

      if(GNC_b64bin(&v6) == EOF) ERROR1;
      if(v6 > 64) ERROR1;      /* invalid */

      if(v6 == 64)
        {          /* pad after 2 char */
        done = 1;
        }
      else
        {
        *pOutput++ = (((v24 << 6) | v6) & 0xFF);
        }
      }
    }

  return 0;
  }
