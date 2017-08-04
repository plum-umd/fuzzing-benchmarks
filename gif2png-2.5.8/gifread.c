/* This code is Copyright 1990 - 1994, David Koblas <koblas@netcom.com> */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "gif2png.h"

#define CM_RED          0
#define CM_GREEN        1
#define CM_BLUE         2

#define MAX_LWZ_BITS            12

#define INTERLACE               0x40
#define GLOBALCOLORMAP  0x80
#define LOCALCOLORMAP   0x80
#define BitSet(byte, bit)       (((byte) & (bit)) == (bit))

#define ReadOK(file,buffer,len) (fread(buffer, len, 1, file) != 0)

#define LM_to_uint(a,b)                 (((b)<<8)|(a))

struct gif_scr GifScreen;

struct {
    int   transparent;
    int   delayTime;
    int   inputFlag;
    int   disposal;
} Gif89;

extern int c_437_l1[];
extern int verbose;

static bool ReadColorMap(FILE *fd, unsigned int number, 
			 /*@out@*/GifColor buffer[]);
static bool DoExtension(FILE *fd, int label);
static ssize_t GetDataBlock(FILE *fd, /*@out@*/unsigned char *buf);
static bool ReadImage(FILE *fd, int x_off, int y_off, int len, int height, 
		      unsigned int cmapSize, GifColor cmap[], bool interlace);

static int imagecount;
static bool recover_message; /* if true, message already printed */

/* check if recover option is active, if not, print warning */

static int check_recover(bool some_data)
{
    if(recover)
	return imagecount;
    if(!recover_message && (imagecount>0 || some_data)) {
	(void)fprintf(stderr, "gif2png: image reading error, use option -r to recover ");
	if(imagecount>0)
	    (void)fprintf(stderr, "%d complete image%s ", imagecount,
		    (imagecount>1?"s":""));
	if(imagecount>0 && some_data)
	    (void)fprintf(stderr, "and ");
	if(some_data)
	    (void)fprintf(stderr, "partial data of a broken image");
	(void)fprintf(stderr, "\n");
	recover_message=true;
    }
    return -1;
}

/*@-noret@*/
int
ReadGIF(FILE *fd)
{
    unsigned char buf[16];
    unsigned char c;
    GifColor      localColorMap[MAXCMSIZE];
    bool          useGlobalColormap;
    unsigned int  bitPixel;
    char          gif_version[4];
    int i;
    int w, h, x_off, y_off;

    imagecount=0;
    recover_message=false;

    /*
     * Initialize GIF89 extensions
     */
    Gif89.transparent = -1;
    Gif89.delayTime = -1;
    Gif89.inputFlag = -1;
    Gif89.disposal = 0;

    if (! ReadOK(fd,buf,6)) {
	(void)fprintf(stderr, "gif2png: error reading magic number\n");
	return(-1);
    }

    if (strncmp((char *)buf,"GIF",3) != 0) {
	(void)fprintf(stderr, "gif2png: not a GIF file\n");
	return(-1);
    }

    strncpy(gif_version, (char *)buf + 3, 3);
    gif_version[3] = '\0';

    if ((strcmp(gif_version, "87a") != 0) && (strcmp(gif_version, "89a") != 0)) {
	(void)fprintf(stderr, "gif2png: bad version number, not '87a' or '89a', trying anyway\n");
    }

    if (! ReadOK(fd,buf,7)) {
	(void)fprintf(stderr, "gif2png: failed to read screen descriptor\n");
	return(-1);
    }

    GifScreen.Width           = LM_to_uint(buf[0],buf[1]);
    GifScreen.Height          = LM_to_uint(buf[2],buf[3]);
    GifScreen.BitPixel        = 2<<(buf[4]&0x07);
    GifScreen.ColorResolution = (((buf[4]&0x70)>>3)+1);
    GifScreen.ColorMap_present = BitSet(buf[4], GLOBALCOLORMAP);

    if (GifScreen.ColorMap_present) {     /* Global Colormap */

	if (ReadColorMap(fd, GifScreen.BitPixel, GifScreen.ColorMap)) {
	    (void)fprintf(stderr, "gif2png: error reading global colormap\n");
	    return(-1);
	}
    } else {
	/* the GIF spec says that if neither global nor local
	 * color maps are present, the decoder should use a system
	 * default map, which should have black and white as the
	 * first two colors. So we use black, white, red, green, blue,
	 * yellow, purple and cyan.
	 * I don't think missing color tables are a common case,
	 * at least it's not handled by most GIF readers.
	 */

	static int colors[]={0, 7, 1, 2, 4, 3, 5, 6};

	/*@-predboolint -type@*/
	for(i=0 ; i<MAXCMSIZE-8 ; i++) {
	    GifScreen.ColorMap[i].red   = (colors[i&7]&1) ? ((255-i)&0xf8) : 0;
	    GifScreen.ColorMap[i].green = (colors[i&7]&2) ? ((255-i)&0xf8) : 0;
	    GifScreen.ColorMap[i].blue  = (colors[i&7]&4) ? ((255-i)&0xf8) : 0;
	}
	for(i=MAXCMSIZE-8 ; i<MAXCMSIZE; i++) {
	    GifScreen.ColorMap[i].red   = (colors[i&7]&1) ? 4 : 0;
	    GifScreen.ColorMap[i].green = (colors[i&7]&2) ? 4 : 0;
	    GifScreen.ColorMap[i].blue  = (colors[i&7]&4) ? 4 : 0;
	}
	/*@=predboolint =type@*/
    }

    if(GifScreen.ColorMap_present) {
	GifScreen.Background       = buf[5];
    } else {
	GifScreen.Background       = -1; /* background unspecified */
    }

    GifScreen.AspectRatio     = buf[6];

    for (;;) {
	if (! ReadOK(fd,&c,1)) {
	    (void)fprintf(stderr, "gif2png: EOF / read error on image data\n");
	    return check_recover(false);
	}

	if (c == GIFterminator) {           /* GIF terminator */
	    if (verbose > 1)
		(void)fprintf(stderr, "gif2png: end of GIF data stream\n");
	    return(imagecount);
	}

	if (c == GIFextension) {    /* Extension */
	    if (! ReadOK(fd,&c,1)) {
		(void)fprintf(stderr, "gif2png: EOF / read error on extension function code\n");
		return check_recover(false);
	    }
	    if(DoExtension(fd, c))
		return check_recover(false);
	    continue;
	}

	if (c != GIFimage) {                /* Not a valid start character */
	    (void)fprintf(stderr, "gif2png: bogus character 0x%02x\n",
			  (unsigned int)c);
	    return check_recover(false);
	}

	if (! ReadOK(fd,buf,9)) {
	    (void)fprintf(stderr,"gif2png: couldn't read left/top/width/height\n");
	    return check_recover(false);
	}

	useGlobalColormap = ! BitSet(buf[8], LOCALCOLORMAP);

	bitPixel = 1<<((buf[8]&0x07)+1);

	x_off = LM_to_uint(buf[0],buf[1]);
	y_off = LM_to_uint(buf[2],buf[3]);
	w = LM_to_uint(buf[4],buf[5]);
	h = LM_to_uint(buf[6],buf[7]);

	if (! useGlobalColormap) {
	    if (ReadColorMap(fd, bitPixel, localColorMap)) {
		(void)fprintf(stderr, 
			      "gif2png: error reading local colormap\n");
		return check_recover(false);
	    }

	    if(!ReadImage(fd, x_off, y_off, w, h, bitPixel,
			  localColorMap, BitSet(buf[8], INTERLACE))) {
		imagecount++;
	    }
	} else {
	    if(!GifScreen.ColorMap_present) {
		if (verbose > 1)
		    (void)fprintf(stderr, 
				  "gif2png: neither global nor local colormap, using default\n");
	    }

	    if(!ReadImage(fd, x_off, y_off, w, h, GifScreen.BitPixel,
			  GifScreen.ColorMap,  BitSet(buf[8], INTERLACE))) {
		imagecount++;
	    }
	}

	/*
	 * reset GIF89 extensions after image
	 */
	Gif89.transparent = -1;
	Gif89.delayTime = -1;
	Gif89.inputFlag = -1;
	Gif89.disposal = 0;
    }

    /* should never get here */
}
/*@=noret@*/

static bool
ReadColorMap(FILE *fd, unsigned int number, GifColor colors[])
{
    unsigned int   i;
    byte  rgb[3];

    for (i = 0; i < number; ++i) {
	if (!ReadOK(fd, rgb, sizeof(rgb))) {
	    (void)fprintf(stderr, "gif2png: bad colormap\n");
	    return(true);
	}

	colors[i].red   = rgb[0];
	colors[i].green = rgb[1];
	colors[i].blue  = rgb[2];
    }

    for (i = number; i<MAXCMSIZE; ++i) {
	colors[i].red   = 0;
	colors[i].green = 0;
	colors[i].blue  = 0;
    }

    return false;
}

static bool
DoExtension(FILE *fd, int label)
{
    static char   buf[256];
    char *str;
    char *p,*p2;
    ssize_t size;
    bool last_cr;

    switch (label) {
    case GIFplaintext:            /* Plain Text Extension */
	str = "Plain Text Extension";
	/*
	 * reset GIF89 extensions after Plain Text
	 */
	Gif89.transparent = -1;
	Gif89.delayTime = -1;
	Gif89.inputFlag = -1;
	Gif89.disposal = 0;
	if (verbose > 1)
	    (void)fprintf(stderr, "gif2png: got a 'Plain Text Extension' extension (skipping)\n");

	while (GetDataBlock(fd, (unsigned char*) buf) > 0)
	    continue;
	return (false);

    case GIFapplication:/* Application Extension */
	str = "Application Extension";
	break;

    case GIFcomment:      /* Comment Extension */
	allocate_element(); /* current now points to new element struct */

	/* the GIF89a spec defines comment extensions to contain ASCII
	   text, but doesn't specify what end-of-line delimiter is to
	   be used.  Most programs use DOS style (CR/LF), at least one
	   uses CR (Graphic Workshop for Windows), unix programs
	   probably use LF. We just delete CRs unless they are not
	   followed by a LF, then we convert CR to LF.  Even though 7
	   bit ASCII is specified, there are some GIF files with
	   comments that use codepage 437 and most DOS GIF readers
	   display this in the DOS charset (e.g. VPIC). We convert the
	   characters both in cp437 and latin1 and try to approximate
	   some of the box drawing chars.
	*/

	last_cr=false;

	while ((size=GetDataBlock(fd, (unsigned char*) buf)) > 0) {

	    p2=p=buf;

	    /* take care of CR as last char of one sub-block and no LF at first of
	       the next */
	    if(last_cr && *p!='\n') {
		store_block("\n",1);
		last_cr=false;
	    }

	    while(p-buf<size) {
		if(last_cr) {
		    if(*p!='\n')
			*p2++='\n';
		    last_cr=false;
		}
		if(*p=='\r')
		    last_cr=true;
		else
		    *p2++=c_437_l1[(unsigned char)*p];
		p++;
	    }
	    size=p2-buf;

	    store_block(buf, (size_t)size);
	}

	if (verbose > 1)
	    (void)fprintf(stderr, "gif2png: got a 'Comment Extension' extension\n");

	current->GIFtype=label;

	fix_current();
	return false;

    case GIFgraphicctl:           /* Graphic Control Extension */
	str = "Graphic Control Extension";
	size = GetDataBlock(fd, (unsigned char*) buf);
	/*@-shiftimplementation@*/
	Gif89.disposal    = (buf[0] >> 2) & 0x7;
	Gif89.inputFlag   = (buf[0] >> 1) & 0x1;
	Gif89.delayTime   = LM_to_uint(buf[1],buf[2]);
	/*@=shiftimplementation@*/
	if ((buf[0] & 0x1) != 0)
	    Gif89.transparent = (int)((unsigned char)buf[3]);

	if(Gif89.disposal==0 && Gif89.inputFlag==0 &&
	   Gif89.delayTime==0) {

	    /* do not keep GCEs only indicating transparency */

	    while (GetDataBlock(fd, (unsigned char*) buf) > 0)
		;

	    return (false);
	} else {
	    /* copy block already read */
	    allocate_element(); /* current now points to new element struct */

	    goto copy_block;
	}
    default:
	if (verbose > 1)
	    (void)fprintf(stderr, "gif2png: skipping unknown extension 0x%02x\n",
		    (unsigned char)label);
	while (GetDataBlock(fd, (unsigned char*) buf) > 0)
	    ;
	return (true);
    }

    allocate_element(); /* current now points to new element struct */

    while ((size=GetDataBlock(fd, (unsigned char*) buf)) > 0) {
    copy_block:
	store_block(buf, (size_t)size);
    }

    if (verbose > 1)
	(void)fprintf(stderr, "gif2png: got a '%s' extension\n", str);

    current->GIFtype=label;

    fix_current();

    return (false);
}

static bool     ZeroDataBlock = false;

/*@-mustdefine@*/
static ssize_t
GetDataBlock(FILE *fd, unsigned char *buf)
{
    size_t count;

    count = 0;
    if (! ReadOK(fd, &count, 1)) {
	(void)fprintf(stderr, "gif2png: error in getting DataBlock size\n");
	return -1;
    }

    ZeroDataBlock = count == 0;

    if ((count != 0) && (! ReadOK(fd, buf, count))) {
	(void)fprintf(stderr, "gif2png: error in reading DataBlock\n");
	return -1;
    }

    return((ssize_t)count);
}
/*@=mustdefine@*/

/*
**  Pulled out of nextCode
*/
static  int             curbit, lastbit, last_byte;
static  bool            return_clear, get_done;
/*
**  Out of nextLWZ
*/
static int      stack[(1<<(MAX_LWZ_BITS))*2], *sp;
static int      code_size, set_code_size;
static int      max_code, max_code_size;
static int      clear_code, end_code;

static void initLWZ(unsigned int input_code_size)
{
    /*@-shiftnegative@*/
    set_code_size = (int)input_code_size;
    code_size     = set_code_size + 1;
    clear_code    = 1 << set_code_size ;
    end_code      = clear_code + 1;
    max_code_size = 2 * clear_code;
    max_code      = clear_code + 2;
    /*@=shiftnegative@*/

    curbit = lastbit = 0;
    last_byte = 2;
    get_done = false;

    return_clear = true;

    sp = stack;
}

static int nextCode(FILE *fd, int code_size)
{
    static unsigned char    buf[280];
    static int maskTbl[16] = {
	0x0000, 0x0001, 0x0003, 0x0007,
	0x000f, 0x001f, 0x003f, 0x007f,
	0x00ff, 0x01ff, 0x03ff, 0x07ff,
	0x0fff, 0x1fff, 0x3fff, 0x7fff,
    };
    int i, j, end;
    long ret;

    if (return_clear) {
	return_clear = false;
	return clear_code;
    }

    end = curbit + code_size;

    if (end >= lastbit) {
	ssize_t     count;

	if (get_done) {
	    if (verbose!=0 && curbit >= lastbit) {
		(void)fprintf(stderr, "gif2png: ran off the end of input bits\n");
	    }
	    return -1;
	}
	buf[0] = buf[last_byte-2];
	buf[1] = buf[last_byte-1];

	if ((count = GetDataBlock(fd, &buf[2])) == 0)
	    get_done = true;

	if (count<0) return -1;

	last_byte = 2 + (int)count;
	curbit = (curbit - lastbit) + 16;
	lastbit = (2+(int)count)*8 ;

	end = curbit + code_size;
    }

    j = end / 8;
    i = curbit / 8;

    /*@-shiftimplementation@*/
    if (i == j)
	ret = (long)buf[i];
    else if (i + 1 == j)
	ret = (long)buf[i] | ((long)buf[i+1] << 8);
    else
	ret = (long)buf[i] | ((long)buf[i+1] << 8) | ((long)buf[i+2] << 16);

    /*@-shiftnegative@*/
    ret = (ret >> (curbit % 8)) & maskTbl[code_size];
    /*@=shiftnegative@*/
    /*@=shiftimplementation@*/

    curbit += code_size;

    return (int)ret;
}

#define readLWZ(fd) ((sp > stack) ? *--sp : nextLWZ(fd))

static int nextLWZ(FILE *fd)
{
    static int       table[2][(1<< MAX_LWZ_BITS)];
    static int       firstcode, oldcode;
    int              code, incode;
    register int     i;

    while ((code = nextCode(fd, code_size)) >= 0) {
	if (code == clear_code) {

	    /* corrupt GIFs can make this happen */
	    if (clear_code >= (1<<MAX_LWZ_BITS)) {
		return -2;
	    }

	    for (i = 0; i < clear_code; ++i) {
		table[0][i] = 0;
		table[1][i] = i;
	    }
	    for (; i < (1<<MAX_LWZ_BITS); ++i)
		table[0][i] = table[1][i] = 0;
	    code_size = set_code_size+1;
	    max_code_size = 2*clear_code;
	    max_code = clear_code+2;
	    sp = stack;
	    do {
		firstcode = oldcode = nextCode(fd, code_size);
	    } while (firstcode == clear_code);

	    return firstcode;
	}
	if (code == end_code) {
	    ssize_t         count;
	    unsigned char   buf[260];

	    if (ZeroDataBlock)
		return -2;

	    while ((count = GetDataBlock(fd, buf)) > 0)
		;

	    if (count != 0) {
		(void)fprintf(stderr, "gif2png: missing EOD in data stream (common occurrence)\n");
	    }
	    return -2;
	}

	incode = code;

	if (code >= max_code) {
	    *sp++ = firstcode;
	    code = oldcode;
	}

	while (code >= clear_code) {
	    *sp++ = table[1][code];
	    if (code == table[0][code]) {
		(void)fprintf(stderr, "gif2png: circular table entry BIG ERROR\n");
		return(code);
	    }
	    if ((size_t)((char *)sp - (char *)stack) >= sizeof(stack)) {
		(void)fprintf(stderr, "gif2png: circular table STACK OVERFLOW!\n");
		return(code);
	    }
	    code = table[0][code];
	}

	*sp++ = firstcode = table[1][code];

	if ((code = max_code) <(1<<MAX_LWZ_BITS)) {
	    table[0][code] = oldcode;
	    table[1][code] = firstcode;
	    ++max_code;
	    if ((max_code >= max_code_size) &&
		(max_code_size < (1<<MAX_LWZ_BITS))) {
		max_code_size *= 2;
		++code_size;
	    }
	}

	oldcode = incode;

	if (sp > stack)
	    return *--sp;
    }
    return code;
}

/* find the nearest line above that already has data in it */

/*@-dependenttrans -branchstate@*/
static byte *
get_prev_line(int width, int height, int line)
{
    int prev_line;
    byte *res;

    prev_line=inv_interlace_line(height,line)-1;
    while(interlace_line(height, prev_line)>=line)
	prev_line--;

    res = current->data + width*interlace_line(height, prev_line);

    return res;
}
/*@=dependenttrans -branchstate@*/

/*@-mustfreeonly -mustfreefresh@*/
static bool
ReadImage(FILE *fd, int x_off, int y_off, int width, int height, 
	  unsigned int cmapSize, GifColor cmap[], bool interlace)
{
    unsigned char *dp, c;
    int           v;
    int           xpos = 0, ypos = 0;
    unsigned char *image;
    int i;
    unsigned long *count;

    /*
    **  Initialize the compression routines
    */
    if (! ReadOK(fd,&c,1)) {
	(void)fprintf(stderr, "gif2png: EOF / read error on image data\n");
	return(true);
    }

    initLWZ(c);

    if (verbose > 1)
	(void)fprintf(stderr, "gif2png: reading %d by %d%s GIF image\n",
		width, height, interlace ? " interlaced" : "" );

    allocate_image();
    /* since we know how large the image will be, we set the size in advance.
       This saves a lot of realloc calls, which may require copying memory. */

    set_size((size_t)width*height);

    image=xalloc((size_t)width);

    current->imagestruct->offset_x = x_off;
    current->imagestruct->offset_y = y_off;
    current->imagestruct->width    = (png_uint_32)width;
    current->imagestruct->height   = (png_uint_32)height;
    current->imagestruct->trans    = Gif89.transparent;
    current->imagestruct->interlace= interlace;

    /*@-mayaliasunique@*/
    memcpy(current->imagestruct->colors, cmap, sizeof(GifColor)*MAXCMSIZE);
    /*@=mayaliasunique@*/

    count=current->imagestruct->color_count;

    for(i=0;i<MAXCMSIZE;i++) {
	count[i]=0;
    }

    for (ypos = 0; ypos < height; ypos++) {
	dp=image;
	for (xpos = 0; xpos < width; xpos++) {
	    if ((v = readLWZ(fd)) < 0 || v >= (int)cmapSize) {
		if(v >= (int)cmapSize)
		    (void)fprintf(stderr, 
				  "gif2png: reference to undefined colormap entry\n");

		if(xpos>0 || ypos>0) {
		    if(recover) {
			if(!interlace) {
			    /* easy part, just fill the rest of the `screen' with color 0 */
			    memset(image+xpos,0, (size_t)(width-xpos));
			    store_block((char *)image,(size_t)width);
			    ypos++;

			    memset(image, 0, (size_t)width);
			    for( ; ypos < height ; ypos++)
				store_block((char *)image,(size_t)width);
			} else {
			    /* interlacing recovery is a bit tricky */

			    if(xpos>0) {
				if((inv_interlace_line(height, ypos)&7)==0) {
				    /* in 1st pass */
				    memset(image+xpos, 0, (size_t)(width-xpos));
				} else {
				    /* pass >=2 */
				    memcpy(image+xpos, get_prev_line(width, height, ypos)+xpos,
					   (size_t)(width-xpos));
				}
				store_block((char *)image,(size_t)width);
				ypos++;
			    }

			    /* fill rest of 1st pass with color 0 */
			    memset(image, 0, (size_t)width);
			    for( ; (inv_interlace_line(height, ypos)&7)==0 ; ypos++)
				store_block((char *)image,(size_t)width);

			    /* all other passes, copy from line above */
			    for( ; ypos<height ; ypos++) {
				memcpy(image, get_prev_line(width, height, ypos), (size_t)width);
				store_block((char *)image,(size_t)width);
			    }
			}
			goto fini;
		    } else {
			(void)check_recover(true);
			return(true);
		    }
		} else {
		    return(true);
		}
	    }

	    count[v]++;

	    *dp++ = (unsigned char)v;
	}
	store_block((char *)image,(size_t)width);
    }
fini:

    while(readLWZ(fd)>=0)
	continue;

    /*@-usedef@*/
    (void)free(image);
    /*@=usedef@*/

    fix_current();

    /*
     * reset GIF89 extensions after image
     */

    Gif89.transparent = -1;
    Gif89.delayTime = -1;
    Gif89.inputFlag = -1;
    Gif89.disposal = 0;

    return(false);
}
/*@=mustfreeonly =mustfreefresh@*/

/* gifread.c ends here */
