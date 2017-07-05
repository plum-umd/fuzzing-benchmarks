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
 * Ginger Alliance Ltd. All Rights Reserved.
 * 
 * Contributor(s):
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
//      utf8.h
//

#if !defined(Utf8HIncl)
#define Utf8HIncl

// GP: clean

#include "base.h"

#define SMALL_BUFFER_SIZE 32

//extern  int     utf8SingleCharLength(const char* text);
extern  int     utf8StrLength (const char* text);
extern  int     utf8Strchr(const char* text, const char* character);
extern  char*   utf8StrIndex(char* text, int index);

extern  unsigned long 
                utf8CharCode(const char *text);

extern  int     utf8GetChar(char *dest, const char *src);
extern  char*   utf8xfrm(const Str &src);
extern  int     utf8FromCharCode(char *dest, unsigned long code);

extern	Bool	utf8IsDigit(unsigned long c);
extern	Bool	utf8IsBaseChar(unsigned long c);
extern	Bool	utf8IsLetter(unsigned long c);
extern	Bool	utf8IsNameChar(unsigned long c);
extern	Bool	utf8IsIdeographic(unsigned long c);
extern	Bool	utf8IsExtender(unsigned long c);
extern	Bool	utf8IsCombiningChar(unsigned long c);

extern  int     utf8ToUtf16(wchar_t *dest, const char *src);

//speed optimization
inline int utf8SingleCharLength (const char* text)
{
  if (!(*text & 0x80)) return 1;
  if (!(*text & 0x40)) return 0;
  for (int len = 2; len < 7; len++)
      if (!(*text & (0x80 >> len))) return len;
  return 0;
}

#endif
