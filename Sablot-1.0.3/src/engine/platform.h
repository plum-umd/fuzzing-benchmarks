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

// platform.h

#if !defined(PlatformHIncl)
#define PlatformHIncl

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

//common symbol for MSVC and Borland tools
#if defined(_MSC_VER) || defined (__BORLANDC__)
#define __WIN_TOOLS 1
#endif

#ifdef HAVE_ICONV_H
#include <iconv.h>
#else
#undef iconv_t
typedef void* iconv_t;
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> // needed too in Windows
#endif

#if defined(HAVE_LIMITS_H) || defined (__WIN_TOOLS)
#include <limits.h>
#endif

// GP: clean

//misc combined defines
#if defined(__WIN_TOOLS) || (defined(HAVE_WCSXFRM) && defined(HAVE_WCSCMP))
#define SAB_WCS_COLLATE 1
#endif

// disable the "no placement delete" warning of MSVC 6
// (MSVC 4.2 doesn't accept placement deletes)
#if defined(WIN32)
#pragma warning(disable: 4291)
// some more warnings we don't care about
#pragma warning(disable: 4514 4100 4512)
#endif

extern int isnan__(double x);
extern int isinf__(double x);
extern double getMillisecs();
extern int wcscmp__(const char* a, const char* b);
extern void setlinebuf__(FILE *file);

class DStr;
void my_getcwd(DStr &dir);

#endif

#if defined(WIN32)
#define snprintf _snprintf
#define strncasecmp _strnicmp
#else
#include <strings.h>		// definition of strncasecmp
#endif

//pointer arithmetics
#define intPtrDiff(p1,p2) \
           ((ptrdiff_t)((p1) - (p2)) > (ptrdiff_t)(INT_MAX) ? (int) 0 \
                      : (int) (((p1) - (p2)) & (ptrdiff_t)(INT_MAX)))
