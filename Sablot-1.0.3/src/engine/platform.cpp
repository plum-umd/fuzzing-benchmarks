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

// platform.cpp

// GP: clean

#include <float.h>
#include <math.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

// includes for time measurement
#include <time.h>      // needed by <sys/timeb.h> for definition of time_t

#if defined(HAVE_SYS_TIMEB_H) || defined(WIN32)
#include <sys/timeb.h>
#endif

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif

/* needed for isinf on certain platforms */
#ifdef HAVE_SUNMATH_H
#include <sunmath.h>
#endif

/* needed for wcscmp__ */
#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif

/*
  include direct.h for getcwd()
*/
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#elif defined(WIN32)
#include <direct.h>
#elif defined(__BORLANDC__)
#include <dir.h>
#else
#error "Can't find appropriate include (unistd.h & Co.)"
#endif

#include "datastr.h"

int isnan__(double x)
{
#ifdef HAVE_ISNAN
#if !defined(HAVE_ISNAND)
    return isnan(x);
#else
    return isnand(x);
#endif
#elif defined(WIN32)
    return _isnan(x);
#else
#    error "Don't know how to detect NaN on this platform"
#endif
}

int isinf__(double x)
{
#ifdef HAVE_ISINF
    return isinf(x);
#elif defined (HAVE_FINITE)
    return ! finite(x) && x == x;
#elif defined (WIN32)
    return ! _finite(x) && x == x;
#else
#    error "Don't know how to detect Infinity on this platform"
#endif
}

// time: get the number of milliseconds
double getMillisecs()
{
    double ret;
#if defined (WIN32)
    struct _timeb theTime;
    _ftime(&theTime);
    ret = theTime.time + theTime.millitm/1000.0;
#elif defined (HAVE_FTIME)
    struct timeb theTime;
    ftime(&theTime);
    ret = theTime.time + theTime.millitm/1000.0;
#elif defined (HAVE_GETTIMEOFDAY)
    timeval theTime;
    gettimeofday(&theTime, NULL);
    ret = theTime.tv_sec + theTime.tv_usec/1000000.0;
#else
#error "Can't find function ftime() or similar on this platform"
#endif
    return ret;
}

//wcs compare
int wcscmp__(const char* a, const char* b)
{
#if defined (SAB_WCS_COLLATE)
  return wcscmp((const wchar_t*) a, (const wchar_t*) b);
#else
  return strcmp(a, b);
#endif
}

// get current working directory
void my_getcwd(DStr &dir)
{
char buf[256];
#if defined(WIN32)
    _getcwd(buf, 256);
#else //if defined(__linux__) || defined(__BORLANDC__) || defined(__unix)
    getcwd(buf,256);
#endif
#ifdef __WIN_TOOLS
    // we suppose this function is combined with "file://" only
    dir = "/";
#else
    dir = "";
#endif
    dir += buf;
    if (!(dir == (const char*) "/"))
        dir += '/';
}

//set line buffered output
void setlinebuf__(FILE *file)
{
#if defined(HAVE_SETVBUF) || defined(__WIN_TOOLS)
  setvbuf(file, NULL, _IOLBF, 0);
#endif
}

