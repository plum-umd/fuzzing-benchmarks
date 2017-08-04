/* See the file COPYING for conditions of use */

/* get png type definitions */
#include "png.h"

#include <stdbool.h>

#define GIFterminator ';'
#define GIFextension '!'
#define GIFimage ','

#define GIFcomment 0xfe
#define GIFapplication 0xff
#define GIFplaintext 0x01
#define GIFgraphicctl 0xf9

#define MAXCMSIZE 256

typedef unsigned char byte;

typedef png_color GifColor;

struct GIFimagestruct {
  GifColor colors[MAXCMSIZE];
  unsigned long color_count[MAXCMSIZE];
  int offset_x;
  int offset_y;
  png_uint_32 width;
  png_uint_32 height;
  int trans;
  bool interlace;
};

struct GIFelement {
  struct GIFelement *next;
  char GIFtype;
  byte *data;
  size_t allocated_size;
  size_t size;
  /* only used if GIFtype==GIFimage */
  struct GIFimagestruct *imagestruct;
};

extern struct gif_scr{
  unsigned int  Width;
  unsigned int  Height;
  GifColor      ColorMap[MAXCMSIZE];
  bool          ColorMap_present;
  unsigned int  BitPixel;
  unsigned int  ColorResolution;
  int           Background;
  unsigned int  AspectRatio;
} GifScreen;

int ReadGIF(FILE *);
int MatteGIF(GifColor);

void allocate_element(void);
void store_block(char *, size_t);
void allocate_image(void);
void set_size(size_t);

void *xalloc(size_t);
void *xrealloc(void *, size_t);
void fix_current(void);
void free_mem(void);

int interlace_line(int height, int line);
int inv_interlace_line(int height, int line);

extern struct GIFelement first;
extern struct GIFelement *current;
extern bool recover;

extern const char version[];
extern const char compile_info[];

extern int skip_pte;

