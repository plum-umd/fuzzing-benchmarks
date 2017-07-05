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

#include "base.h"
#include "platform.h"
#include "proc.h"
#include "utf8.h"
// #include <new.h>
#include <ctype.h>
// #include <strings.h>

// GP: clean

#if defined(CHECK_LEAKS)
#pragma Msg("Building with leak checking")
#endif

// includes for time measurement moved to platform.cpp

/****************************************
X S L   c o n s t a n t s
****************************************/

// !!!!!!!!!!!!!!!!!!!!!
// the order of items in the following tables must agree with
// that in the corresponding enums as defined in base.h

const char* xslOpNames[]=
{
    "apply-imports","apply-templates",
    "attribute","attribute-set",
    "call-template","choose",
    "comment", "copy","copy-of",
    "decimal-format","element",
    "fallback","for-each",
    "if","import",
    "include","key",
    "message","namespace-alias",
    "number","otherwise",
    "output","param",
    "preserve-space","processing-instruction",
    "sort","strip-space",
    "stylesheet","template",
    "text","transform",
    "value-of","variable",
    "when","with-param",
    NULL
};

const char* xslAttNames[]=
{
    "case-order", "cdata-section-elements", "count",
    "data-type", "decimal-separator", "digit", "disable-output-escaping", "doctype-public", "doctype-system",
    "elements", "encoding", "exclude-result-prefixes", "extension-element-prefixes",
    "format", "from",
    "grouping-separator", "grouping-size",
    "href",
    "id", "indent", "infinity",
    "lang", "letter-value", "level",
    "match", "media-type", "method", "minus-sign", "mode",
    "name", "namespace", "NaN",
    "omit-xml-declaration", "order",
    "pattern-separator", "percent", "per-mille", "priority",
    "result-prefix", 
    "select", "standalone", "stylesheet-prefix",
    "terminate", "test",
    "use", "use-attribute-sets",
    "value", "version",
    "zero-digit",
    NULL
};

const char* axisNames[]=
{
    "ancestor","ancestor-or-self",
        "attribute","child",
        "descendant","descendant-or-self",
        "following","following-sibling",
        "namespace","parent",
        "preceding","preceding-sibling",
        "self",
        NULL,
        "root"
};

const char* vertexTypeNames[] =
{
    "","root","element","attribute",
        "text","processing instruction","comment","namespace"
};

const char* exNodeTypeNames[] =
{
    "node", "text",
        "processing-instruction", "comment",
        NULL
};

const char* theXSLTNamespace = "http://www.w3.org/1999/XSL/Transform";
const char* oldXSLTNamespace = "http://www.w3.org/XSL/Transform/1.0";
const char* theXMLNamespace = "http://www.w3.org/XML/1998/namespace";
const char* theXHTMLNamespace = "http://www.w3.org/1999/xhtml";
const char* theXMLNSNamespace = "http://www.w3.org/2000/xmlns/";
const char* theSabExtNamespace = "http://www.gingerall.org/sablotron/extension";
const char* theEXSLTDynNamespace = "http://exslt.org/dynamic";

const char* theWhitespace = " \t\x0a\x0d";

//
//  escape strings
//

const char
    * escNewline = "&#10;",
    * escTab = "&#9;",
    * escLess = "&lt;",
    * escGreater = "&gt;",
    * escQuote = "&quot;",
    * escApos = "&apos;";

//
//  handler types
//  to match HandlerType in shandler.h
//

const char* hlrTypeNames[] = {"message", "scheme", "streaming",
                              "miscellaneous", "encoding"};

/*****************************************************************
Global handlers that can be set via Sablot functions
*****************************************************************/

// URI scheme handler
SchemeHandler* theSchemeHandler = NULL;
// message handler (errors, warnings, log messages...)
MessageHandler* theMessageHandler = NULL;
// SAX call handler (streamed access to the result document)
SAXHandler* theSAXHandler = NULL;
void* theSAXUserData = NULL;

/****************************************
l o o k u p
****************************************/

// Finds a string in a NULL-terminated table of pointers.

int lookup(const char* str, const char** table)
{
    const char **p = table;
    int i = 0;
    while (*p)
    {
        if (!strcmp(str,*p)) return i;
        p++; i++;
    };
    return i;
}

int lookupNoCase(const char* str, const char** table)
{
    const char **p = table;
    int i = 0;
    while (*p)
    {
        if (strEqNoCase(str,*p)) return i;
        p++; i++;
    };
    return i;
}



/*****************************************************************
stdopen(), stdclose()
*****************************************************************/

Bool isstd(const char *fname)
{
    if (!strcmp(fname,"/__stdin") || !strcmp(fname,"/__stderr")
        || !strcmp(fname,"/__stdout"))
        return TRUE;
    return FALSE;
}

int stdclose(FILE *f)
{
    if ((!f) || (f == stdin) || (f == stdout) || (f == stderr))
        return 0;
    else return fclose(f);
}

FILE* stdopen(const char *fn, const char *mode)
{
  if (!strcmp(fn,"/__stderr"))
    return stderr;
  else if (!strcmp(fn,"/__stdout"))
    return stdout;
  else if (!strcmp(fn,"/__stdin"))
    return stdin;
  else 
    {
      //we suppose we are working with uri PATH part
      //on windows platforms it has form "/c:/bla/blah"
      //so we need to remove the leading slash
#ifdef __WIN_TOOLS
      const char * _fn;
      if (fn[0] == '/' && fn[2] == ':')
	_fn = fn + 1;
      else
	_fn = fn;
#else
      const char * _fn = fn;
#endif
      FILE* x = fopen(_fn,mode);
      if (x) return x;
      return NULL;
    };
};

/*****************************************************************
strEqNoCase
*****************************************************************/

Bool strEqNoCase(const char* s1, const char* s2)
{
    int i;
    for (i = 0; s1[i]; i++)
    {
        if (tolower(s1[i]) != tolower(s2[i]))
            return FALSE;
    }
    return s2[i] ? FALSE : TRUE;
}

/*****************************************************************
Memory leaks
*****************************************************************/

void checkLeak()
{
#if (defined(WIN32) && defined(CHECK_LEAKS))
    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
    _CrtMemDumpAllObjectsSince(NULL);
#endif
}

void memStats()
{
#if (defined(WIN32) && defined(CHECK_LEAKS))
    _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
    _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
    _CrtMemState memsnap;
    _CrtMemCheckpoint(&memsnap);
    _CrtMemDumpStatistics(&memsnap);
#endif
}

/*****************************************************************
new handler
*****************************************************************/

//#ifdef _DEBUG
//#pragma Msg("'new' handler in linux?")
//#endif
/*
#ifdef WIN32
static _PNH oldNewHandler = NULL;
#endif

int sablotNewHandler(size_t size)
{
#if defined(WIN32) || defined(__linux__) || defined(__unix)
        throw(E_MEMORY);
#else
        //situation.error(E_MEMORY, *theEmptyString, theEmptyString);
#endif
    return 0;
}

void pushNewHandler()
{
#ifdef WIN32
    sabassert(!oldNewHandler);
    oldNewHandler = _set_new_handler(sablotNewHandler);
#endif
}

void popNewHandler()
{
#ifdef WIN32
    _set_new_handler( oldNewHandler );
    oldNewHandler = NULL; 
#endif
}
*/
/*****************************************************************
fcomp

  float comparison: returns 0 / 1 / -1 if p1 == / > / < p2.
*****************************************************************/

int fcomp(double p1, double p2)
{
    double d = p1 - p2;
    if ((d < EPS) && (d > -EPS)) return 0;
    else return (d > 0 ? 1 : -1);
}

//  time

Str getMillisecsDiff(double originalTime)
{
    char buf[20];
    // getMillisecs() is in platform.cpp
    sprintf(buf, "%.3f", getMillisecs() - originalTime);
    return Str(buf);
}


/**************** XML name checks ********************/

Bool isValidNCName(const char* name)
{
  int len = utf8StrLength(name);
  if (len == 0) return FALSE;

  wchar_t *buff = new wchar_t[len + 1];

  utf8ToUtf16(buff, name);
  
  Bool result = utf8IsLetter(buff[0]) || buff[0] == 0x005F; //underscore  
  for (int i = 1; i < len && result; i++)
    {
      result = 
	utf8IsLetter(buff[i]) ||
	utf8IsDigit(buff[i]) ||
	utf8IsCombiningChar(buff[i]) ||
	utf8IsExtender(buff[i]) ||
	buff[i] == 0x002E || //dot
	buff[i] == 0x002D || //hyphen
	buff[i] == 0x005F; //underscore
    }

  delete[] buff;
  return result;
}

Bool isValidQName(const char* name)
{
  char *local = NULL;
  char *start = NULL;
  Bool copy = false;
  Bool result = TRUE;

  char *colon = (char *)strchr(name, ':');
  if (colon) 
    {
      //*colon = '\0';
      local = colon + 1;
      copy = true;
      start = new char[colon - name + 1];
      strncpy(start, name, colon - name);
      start[colon - name] = '\0';
    } else {
      start = (char*)name;
    }

  result = isValidNCName(start) && (local == NULL || isValidNCName(local));

  //if (colon) *colon = ':';
  if (copy) delete start;
  return result;
}


//string parsing
Bool getWhDelimString(char *&list, Str& firstPart)
{
    skipWhite(list);
    if (!*list) return FALSE;
    char *list_was = list;
    for(; *list && !isWhite(*list); list++);
    firstPart.nset(list_was, (int)(list - list_was));
    return TRUE;
}
