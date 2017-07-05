/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is the Sablotron XSLT Processor.
 *
 * The Initial Developer of the Original Code is Ginger Alliance Ltd.
 * Portions created by Ginger Alliance are Copyright (C) 2000-2002
 * Ginger  Alliance Ltd. All Rights Reserved.
 *
 * Contributor(s): Sven Neumann <neo@netzquadrat.de>,
 *                 Marc Lehmann <pcg@goof.com> (character ranges)
 *
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL"), in which case the provisions of the GPL are applicable
 * instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of the GPL and not to
 * allow others to use your version of this file under the MPL,
 * indicate your decision by deleting the provisions above and
 * replace them with the notice and other provisions required by
 * the GPL.  If you do not delete the provisions above, a recipient
 * may use your version of this file under either the MPL or the
 * GPL.
 */

//
//      utf8.cpp
//

// GP: clean

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include "utf8.h"

#if defined(HAVE_WCHAR_H) || defined(_MSC_VER)
#include <wchar.h>
#endif

/*
inline int utf8SingleCharLength (const char* text)
{
  if (!(*text & 0x80)) return 1;
  if (!(*text & 0x40)) return 0;
  for (int len = 2; len < 7; len++)
      if (!(*text & (0x80 >> len))) return len;
  return 0;
}
*/

int utf8StrLength (const char* text)
{
    int len;
    for (len = 0; *text; len++)
    {
        if (!(*text & 0x80))
	        text++;
		else text += utf8SingleCharLength(text);
    }
    return len;
}

int utf8Strchr(const char* text, const char* character)
{
    int len = utf8SingleCharLength(character),
        pos = 0;
    while (*text)
    {
        // the following works in a prefix encoding
        if (!strncmp(text, character, len))
	        return pos;
		text += utf8SingleCharLength(text);
		pos++;
    }
    return -1;
}

char* utf8StrIndex(char* text, int index)
{
    for (int i = 0; *text && (i < index); i++)
        text += utf8SingleCharLength(text);
	return *text ? text : NULL;
}

// this ought to return the Unicode equivalent of the UTF-8 char
// (for character references like &#22331;)
//
unsigned long utf8CharCode(const char *text)
{
    int i, len = utf8SingleCharLength(text);
    if (!len) return (unsigned long) -1;
    if (len == 1) return *text;
    unsigned long code = (*text & (0xff >> (len + 1)));   // get 1st byte
    for (i = 1; i < len; i++)
        code = (code << 6) | (text[i] & 0x3f);
    return code;
}

unsigned long utf16CharCode(const wchar_t *src)
{
    unsigned long first = *src & 0xffffUL; // fix for Solaris
    if (first < 0xd800U || first >= 0xe000U)
        return first;
    unsigned short second = (unsigned short)(src[1]);
    unsigned long code = (first - 0xd7c0) << 10;
    code |= (second & ~0xdc00);
    return code;
}

// returns length in *words*
int utf16SingleCharLength(const wchar_t *src)
{
    // issues a warning on win32:
    // wchar_t first = *src & 0xffffUL;
    unsigned long first = *src & 0xffffUL; // fix for Solaris
    if (first < 0xd800U || first >= 0xe000U)
        return 1;
    else
        return 2;
}

int utf8FromCharCode(char *dest, unsigned long code)
// this is based on Roman Czyborra's description of the algorithm
{
    char *dest0 = dest;
    if (code < 0x80) 
        *(dest++) = (char)code;
    else if (code < 0x800) {
        *(dest++) = 0xc0 | (char)(code>>6);
        *(dest++) = 0x80 | (char)code & 0x3f;
    }
    else if (code < 0x10000UL) {
        *(dest++) = 0xe0 | (char)(code>>12);
        *(dest++) = 0x80 | (char)(code>>6) & 0x3f;
        *(dest++) = 0x80 | (char)code & 0x3f;
    }
    else if (code < 0x200000UL) {
        *(dest++) = 0xf0 | (char)(code>>18);
        *(dest++) = 0x80 | (char)(code>>12) & 0x3f;
        *(dest++) = 0x80 | (char)(code>>6) & 0x3f;
        *(dest++) = 0x80 | (char)code & 0x3f;
    };
    return (dest - dest0);
}

// returns length of the result *in words*

int utf8ToUtf16(wchar_t *dest, const char *src)
{
    unsigned long code;
    int len = 0,
        thislen;
    for (const char *p = src; *p; p += utf8SingleCharLength(p))
    {
        code = utf8CharCode(p);
        if (code < 0x10000UL)
        {
            *dest = (wchar_t)(code);
            thislen = 1;
        }
        else
        {
            dest[0] = 0xd7c0U + (code >> 10);
            dest[1] = 0xdc00U | code & 0x3ff;
            thislen = 2;
        };
        dest += thislen;
        len += thislen;
    }
    *dest = 0;
    return len;
}

int utf8FromUtf16(char *dest, const wchar_t *src)
{
    unsigned long code;
    int len = 0, thislen;
    for (;*src;)
    {
        code = utf16CharCode(src);
        src += utf16SingleCharLength(src);
        thislen = utf8FromCharCode(dest, code);
        len += thislen;
        dest += thislen;
    }
    *dest = 0;
    return len;
}


// this does the same as strcoll for utf8 strings
/*
Str utf8xfrm(const Str &src)
{
#   if !defined(HAVE_WCSXFRM) && !defined(_MSC_VER)
    // empty if no wcsxfrm()
    return (char*)"";
#   else
    int wideReserved = src.length() + 1, // *words*
        transReserved;
    wchar_t *wide = new wchar_t[wideReserved];
    utf8ToUtf16(wide, src);
    // sadly, this doesn't work in MSVC 6.0
    // looks like there's a bug in MS's wcsxfrm.c:

    // transReserved = 1 + wcsxfrm( NULL, wide, 0 );
    // wchar_t *transformed = new wchar_t[transReserved];

    // FIXME: this is wild. when fixing, be sure to free 'transformed' at the end 
    transReserved = 1024;
    wchar_t transformed[1024];

    int transLen = wcsxfrm(transformed, wide, transReserved);
    sabassert(transReserved >= transLen + 1);
    
#   if defined(SABLOT_WCSXFRM_SWAP)
    // the implementation of wcsxfrm on Solaris systems requires us
    // to swap the words in the 4-byte wchar_t. This was found by Steve Rowe.
    wchar_t *p = transformed;
    for (int i = 0; i < transLen; i++, p++)
    {
        // swap words in transformed[i]
	    *p = (*p >> 16) | ((*p & 0xffffUL) << 16);
    }
#   endif
    
    // FIXME: better estimate?
    int resultReserved = (transLen << 3) + 1;
    char *result = new char[resultReserved];
    int resultLen = utf8FromUtf16(result, transformed);
    sabassert (resultLen < resultReserved);
    Str resultStr = result;
    delete[] wide;
    // delete[] transformed; - temporarily allocating on the stack
    delete[] result;
    return resultStr;
#   endif // no wcsxfrm
}
*/

//returns pointer to char due CList values typing
char* utf8xfrm(const Str &src)
{
#   if !defined(SAB_WCS_COLLATE)
    // empty if no wcsxfrm()
    return (char*)"";
#   else
    int wideReserved = src.length() + 1, // *words*
        transReserved;
    wchar_t *wide = new wchar_t[wideReserved];
    utf8ToUtf16(wide, src);

    transReserved = wideReserved * 3 / 2; //naive estimation
    char *transformed = new char[transReserved * sizeof(wchar_t)];

    int transLen = wcsxfrm((wchar_t*)transformed, wide, transReserved);
    while (transReserved < transLen + 1)
      {
	delete[] transformed;
	transReserved = (transReserved > 1 ? transReserved : 2) * 3 / 2; //some reallocation
	transformed = new char[transReserved * sizeof(wchar_t)];
	transLen = wcsxfrm((wchar_t*)transformed, wide, transReserved);
      }
    
    // FIXME: better estimate?
    delete[] wide;
    return transformed;
#   endif // no wcsxfrm
}

int utf8GetChar(char *dest, const char *src)
{
    int len = utf8SingleCharLength (src);
    memcpy (dest, src, len);
    return len;
}


// tests whether a character in "c" is Within Range
#define wr(a,b) (((c) - (a)) <= (b) - (a))

Bool utf8IsDigit(unsigned long c)
{
   return
      wr(0x0030,0x0039) || wr(0x0660,0x0669) || wr(0x06F0,0x06F9) || wr(0x0966,0x096F) || wr(0x09E6,0x09EF) || wr(0x0A66,0x0A6F) || wr(0x0AE6,0x0AEF) || wr(0x0B66,0x0B6F) || wr(0x0BE7,0x0BEF) || wr(0x0C66,0x0C6F) || wr(0x0CE6,0x0CEF)
      || wr(0x0D66,0x0D6F) || wr(0x0E50,0x0E59) || wr(0x0ED0,0x0ED9) || wr(0x0F20,0x0F29);
}

Bool utf8IsBaseChar(unsigned long c)
{
   return
      wr(0x0041,0x005A) || wr(0x0061,0x007A) || wr(0x00C0,0x00D6) || wr(0x00D8,0x00F6) || wr(0x00F8,0x00FF) || wr(0x0100,0x0131) || wr(0x0134,0x013E) || wr(0x0141,0x0148) || wr(0x014A,0x017E) || wr(0x0180,0x01C3) || wr(0x01CD,0x01F0)
      || wr(0x01F4,0x01F5) || wr(0x01FA,0x0217) || wr(0x0250,0x02A8) || wr(0x02BB,0x02C1) || c == 0x0386 || wr(0x0388,0x038A) || c == 0x038C || wr(0x038E,0x03A1) || wr(0x03A3,0x03CE) || wr(0x03D0,0x03D6) || c == 0x03DA || c == 0x03DC || c == 0x03DE
      || c == 0x03E0 || wr(0x03E2,0x03F3) || wr(0x0401,0x040C) || wr(0x040E,0x044F) || wr(0x0451,0x045C) || wr(0x045E,0x0481) || wr(0x0490,0x04C4) || wr(0x04C7,0x04C8) || wr(0x04CB,0x04CC) || wr(0x04D0,0x04EB) || wr(0x04EE,0x04F5)
      || wr(0x04F8,0x04F9) || wr(0x0531,0x0556) || c == 0x0559 || wr(0x0561,0x0586) || wr(0x05D0,0x05EA) || wr(0x05F0,0x05F2) || wr(0x0621,0x063A) || wr(0x0641,0x064A) || wr(0x0671,0x06B7) || wr(0x06BA,0x06BE) || wr(0x06C0,0x06CE)
      || wr(0x06D0,0x06D3) || c == 0x06D5 || wr(0x06E5,0x06E6) || wr(0x0905,0x0939) || c == 0x093D || wr(0x0958,0x0961) || wr(0x0985,0x098C) || wr(0x098F,0x0990) || wr(0x0993,0x09A8) || wr(0x09AA,0x09B0) || c == 0x09B2 || wr(0x09B6,0x09B9)
      || wr(0x09DC,0x09DD) || wr(0x09DF,0x09E1) || wr(0x09F0,0x09F1) || wr(0x0A05,0x0A0A) || wr(0x0A0F,0x0A10) || wr(0x0A13,0x0A28) || wr(0x0A2A,0x0A30) || wr(0x0A32,0x0A33) || wr(0x0A35,0x0A36) || wr(0x0A38,0x0A39)
      || wr(0x0A59,0x0A5C) || c == 0x0A5E || wr(0x0A72,0x0A74) || wr(0x0A85,0x0A8B) || c == 0x0A8D || wr(0x0A8F,0x0A91) || wr(0x0A93,0x0AA8) || wr(0x0AAA,0x0AB0) || wr(0x0AB2,0x0AB3) || wr(0x0AB5,0x0AB9) || c == 0x0ABD || c == 0x0AE0
      || wr(0x0B05,0x0B0C) || wr(0x0B0F,0x0B10) || wr(0x0B13,0x0B28) || wr(0x0B2A,0x0B30) || wr(0x0B32,0x0B33) || wr(0x0B36,0x0B39) || c == 0x0B3D || wr(0x0B5C,0x0B5D) || wr(0x0B5F,0x0B61) || wr(0x0B85,0x0B8A) || wr(0x0B8E,0x0B90)
      || wr(0x0B92,0x0B95) || wr(0x0B99,0x0B9A) || c == 0x0B9C || wr(0x0B9E,0x0B9F) || wr(0x0BA3,0x0BA4) || wr(0x0BA8,0x0BAA) || wr(0x0BAE,0x0BB5) || wr(0x0BB7,0x0BB9) || wr(0x0C05,0x0C0C) || wr(0x0C0E,0x0C10) || wr(0x0C12,0x0C28)
      || wr(0x0C2A,0x0C33) || wr(0x0C35,0x0C39) || wr(0x0C60,0x0C61) || wr(0x0C85,0x0C8C) || wr(0x0C8E,0x0C90) || wr(0x0C92,0x0CA8) || wr(0x0CAA,0x0CB3) || wr(0x0CB5,0x0CB9) || c == 0x0CDE || wr(0x0CE0,0x0CE1) || wr(0x0D05,0x0D0C)
      || wr(0x0D0E,0x0D10) || wr(0x0D12,0x0D28) || wr(0x0D2A,0x0D39) || wr(0x0D60,0x0D61) || wr(0x0E01,0x0E2E) || c == 0x0E30 || wr(0x0E32,0x0E33) || wr(0x0E40,0x0E45) || wr(0x0E81,0x0E82) || c == 0x0E84 || wr(0x0E87,0x0E88) || c == 0x0E8A
      || c == 0x0E8D || wr(0x0E94,0x0E97) || wr(0x0E99,0x0E9F) || wr(0x0EA1,0x0EA3) || c == 0x0EA5 || c == 0x0EA7 || wr(0x0EAA,0x0EAB) || wr(0x0EAD,0x0EAE) || c == 0x0EB0 || wr(0x0EB2,0x0EB3) || c == 0x0EBD || wr(0x0EC0,0x0EC4) || wr(0x0F40,0x0F47)
      || wr(0x0F49,0x0F69) || wr(0x10A0,0x10C5) || wr(0x10D0,0x10F6) || c == 0x1100 || wr(0x1102,0x1103) || wr(0x1105,0x1107) || c == 0x1109 || wr(0x110B,0x110C) || wr(0x110E,0x1112) || c == 0x113C || c == 0x113E || c == 0x1140 || c == 0x114C || c == 0x114E
      || c == 0x1150 || wr(0x1154,0x1155) || c == 0x1159 || wr(0x115F,0x1161) || c == 0x1163 || c == 0x1165 || c == 0x1167 || c == 0x1169 || wr(0x116D,0x116E) || wr(0x1172,0x1173) || c == 0x1175 || c == 0x119E || c == 0x11A8 || c == 0x11AB || wr(0x11AE,0x11AF)
      || wr(0x11B7,0x11B8) || c == 0x11BA || wr(0x11BC,0x11C2) || c == 0x11EB || c == 0x11F0 || c == 0x11F9 || wr(0x1E00,0x1E9B) || wr(0x1EA0,0x1EF9) || wr(0x1F00,0x1F15) || wr(0x1F18,0x1F1D) || wr(0x1F20,0x1F45) || wr(0x1F48,0x1F4D)
      || wr(0x1F50,0x1F57) || c == 0x1F59 || c == 0x1F5B || c == 0x1F5D || wr(0x1F5F,0x1F7D) || wr(0x1F80,0x1FB4) || wr(0x1FB6,0x1FBC) || c == 0x1FBE || wr(0x1FC2,0x1FC4) || wr(0x1FC6,0x1FCC) || wr(0x1FD0,0x1FD3) || wr(0x1FD6,0x1FDB)
      || wr(0x1FE0,0x1FEC) || wr(0x1FF2,0x1FF4) || wr(0x1FF6,0x1FFC) || c == 0x2126 || wr(0x212A,0x212B) || c == 0x212E || wr(0x2180,0x2182) || wr(0x3041,0x3094) || wr(0x30A1,0x30FA) || wr(0x3105,0x312C) || wr(0xAC00,0xD7A3);
}

Bool utf8IsIdeographic(unsigned long c)
{
   return
      wr(0x4E00,0x9FA5) || c == 0x3007 || wr(0x3021,0x3029);
}

Bool utf8IsExtender(unsigned long c)
{
   return
      c == 0x00B7 || c == 0x02D0 || c == 0x02D1 || c == 0x0387 || c == 0x0640 || c == 0x0E46 || c == 0x0EC6 || c == 0x3005 || wr(0x3031,0x3035) || wr(0x309D,0x309E) || wr(0x30FC,0x30FE);
}

Bool utf8IsCombiningChar(unsigned long c)
{
   return
      wr(0x0300,0x0345) || wr(0x0360,0x0361) || wr(0x0483,0x0486) || wr(0x0591,0x05A1) || wr(0x05A3,0x05B9) || wr(0x05BB,0x05BD) || c == 0x05BF || wr(0x05C1,0x05C2) || c == 0x05C4 || wr(0x064B,0x0652) || c == 0x0670 || wr(0x06D6,0x06DC)
      || wr(0x06DD,0x06DF) || wr(0x06E0,0x06E4) || wr(0x06E7,0x06E8) || wr(0x06EA,0x06ED) || wr(0x0901,0x0903) || c == 0x093C || wr(0x093E,0x094C) || c == 0x094D || wr(0x0951,0x0954) || wr(0x0962,0x0963) || wr(0x0981,0x0983) || c == 0x09BC
      || c == 0x09BE || c == 0x09BF || wr(0x09C0,0x09C4) || wr(0x09C7,0x09C8) || wr(0x09CB,0x09CD) || c == 0x09D7 || wr(0x09E2,0x09E3) || c == 0x0A02 || c == 0x0A3C || c == 0x0A3E || c == 0x0A3F || wr(0x0A40,0x0A42) || wr(0x0A47,0x0A48) || wr(0x0A4B,0x0A4D)
      || wr(0x0A70,0x0A71) || wr(0x0A81,0x0A83) || c == 0x0ABC || wr(0x0ABE,0x0AC5) || wr(0x0AC7,0x0AC9) || wr(0x0ACB,0x0ACD) || wr(0x0B01,0x0B03) || c == 0x0B3C || wr(0x0B3E,0x0B43) || wr(0x0B47,0x0B48) || wr(0x0B4B,0x0B4D)
      || wr(0x0B56,0x0B57) || wr(0x0B82,0x0B83) || wr(0x0BBE,0x0BC2) || wr(0x0BC6,0x0BC8) || wr(0x0BCA,0x0BCD) || c == 0x0BD7 || wr(0x0C01,0x0C03) || wr(0x0C3E,0x0C44) || wr(0x0C46,0x0C48) || wr(0x0C4A,0x0C4D) || wr(0x0C55,0x0C56)
      || wr(0x0C82,0x0C83) || wr(0x0CBE,0x0CC4) || wr(0x0CC6,0x0CC8) || wr(0x0CCA,0x0CCD) || wr(0x0CD5,0x0CD6) || wr(0x0D02,0x0D03) || wr(0x0D3E,0x0D43) || wr(0x0D46,0x0D48) || wr(0x0D4A,0x0D4D) || c == 0x0D57 || c == 0x0E31
      || wr(0x0E34,0x0E3A) || wr(0x0E47,0x0E4E) || c == 0x0EB1 || wr(0x0EB4,0x0EB9) || wr(0x0EBB,0x0EBC) || wr(0x0EC8,0x0ECD) || wr(0x0F18,0x0F19) || c == 0x0F35 || c == 0x0F37 || c == 0x0F39 || c == 0x0F3E || c == 0x0F3F || wr(0x0F71,0x0F84)
      || wr(0x0F86,0x0F8B) || wr(0x0F90,0x0F95) || c == 0x0F97 || wr(0x0F99,0x0FAD) || wr(0x0FB1,0x0FB7) || c == 0x0FB9 || wr(0x20D0,0x20DC) || c == 0x20E1 || wr(0x302A,0x302F) || c == 0x3099 || c == 0x309A;
}

Bool utf8IsLetter(unsigned long c)
{
   return utf8IsBaseChar(c) || utf8IsIdeographic(c);
}

Bool utf8IsNameChar(unsigned long c)
{
   return utf8IsLetter(c) || utf8IsDigit(c)
          || c == '.' || c == '-' ||  c =='_' ||  c ==':'
          || utf8IsCombiningChar(c) || utf8IsExtender(c);
}
