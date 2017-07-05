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

/*****************************************************************
    uri.cpp
*****************************************************************/

#include "uri.h"
#include <string.h>
#include "proc.h"
#include "platform.h"

// GP: clean

/*****************************************************************

    global functions

*****************************************************************/

#define RF(CONDITION) {if (!(CONDITION)) return;}

// definition of names for various URI-reference parts
#define U_SCHEME    0
#define U_AUTH      1
#define U_PATH      2
#define U_QUERY     3
#define U_FRAG      4

// definition of slahes in path names
#define slashes "/\\"
#define isSlash(c) (c == '/' || c == '\\')

//special name reporting under windows
#ifdef __WIN_TOOLS
#define winName(name) ((name[0] == '/' && name[2] == ':') ? name + 1 : name)
#else
#define winName(name) name
#endif

/*****************************************************************
uriHasAuthority

  decides, whether given schema should contain the authority
*****************************************************************/

Bool uriHasAuthority(Str & scheme)
{
  return (scheme == (const char*)"file");
}

/*****************************************************************
splitBy

  splits a given string into two parts divided by the first occurence
  of a delimiter from a given set. If no delimiter is found, returns FALSE
  and leaves 'string' as is; otherwise shifts 'string' to the character
  following the delimiter.
ARGS:
  string        the asciiz string to be split
  delims        the asciiz set of delimiters (all of them ASCII chars)
  part1         first of the two parts
RETURN:
  string        shifted to the other part (past the delimiter)
  .             the delimiter found (or 0)
*****************************************************************/

char splitBy(const char *&string, const char *delims, Str &part1)
{
    char c;
    int firstLen = strcspn(string, delims);
    part1.nset(string, firstLen);
    if (!!(c = string[firstLen]))
        string += firstLen + 1;
    return c;
}

typedef Str FiveStr[5];

void splitURI(const char *uri, FiveStr &parts)
{
    const char *rest;
    char c;
    for (int i = 0; i < 5; i++) 
        parts[i].empty();
    RF( uri && *uri );
    // extract the scheme part of the URI
    if (!splitBy(rest = uri, ":", parts[U_SCHEME]))
        parts[U_SCHEME].empty();
    // if "//" follows, extract the authority part
    c = 'A';    // marks the absence of auth
    if (isSlash(*rest) && isSlash(rest[1]))
        RF( c = splitBy(rest += 2, slashes"?#", parts[U_AUTH]) );
    if (isSlash(c) || c == 'A')
      // extract the path
      RF( c = splitBy(rest -= (isSlash(c)), "?#", parts[U_PATH]) );
    //query and fragment
    if (c == '?')
        // extract the query
        RF( c = splitBy(rest, "#", parts[U_QUERY]) );
    // copy the fragment
    parts[U_FRAG] = (char *) rest;
};

void joinURI(DStr &joined, FiveStr &parts, Bool schemeToo)
{
  joined.empty();
  if (schemeToo && !parts[U_SCHEME].isEmpty())
    joined = parts[U_SCHEME] + ":";
  //if (!parts[U_AUTH].isEmpty())
  if (uriHasAuthority(parts[U_SCHEME]))
    joined += Str("//") + parts[U_AUTH];  // add authority
  joined += parts[U_PATH];                // add path
  if (!parts[U_QUERY].isEmpty())          // add query
    joined += Str("?") + parts[U_QUERY];
  if (!parts[U_FRAG].isEmpty())           // add fragment
    joined += Str("#") + parts[U_FRAG];
}

/*****************************************************************
schemeToURI_()

  converts the scheme given as Str to one of the URI_... constants.
  If the scheme is neither "file" or "arg" then URI_EXTENSION is
  simply returned.
*****************************************************************/

URIScheme schemeToURI_(Sit S, Str& scheme)
{
    if (scheme.eqNoCase("file") && !S.hasFlag(SAB_FILES_TO_HANDLER))
        return URI_FILE;
    else
    {
        if (scheme.eqNoCase("arg"))
            return URI_ARG;
        else 
            return URI_EXTENSION;
    }
}


/*****************************************************************
cutLast()

  truncates a path after 'howmany'-th slash from the right (1-based).
  If there are fewer slashes, sets path to empty string and returns
  FALSE, otherwise returns TRUE.
ARGS
  path      the path to be truncated
  howmany   # of slashes that disappear in truncation, MINUS 1
RETURNS
  .         TRUE iff that many slashes were found
  path      the truncated path
*****************************************************************/

Bool cutLast(Str& path, int howmany)
{
    Str temp = path;
    char *p = (char*) temp;
    int slashCount = 0,
        i;
    for (i = temp.length() - 1; i >= 0; i--)
    {
        if (isSlash(p[i]))
            slashCount++;
        if (slashCount == howmany)
            break;
    };
    if (i >= 0)
        path.nset(p, i+1);
    else
        path.empty();
    return (Bool)(i >= 0);
};

/*****************************************************************
joinPaths()

  merges a relative path with a base path
ARGS
  relPath       the relative path. The result is returned here.
  basePath      the base path (always absolute)
RETURNS
  relPath       the newly constructed absolute path
*****************************************************************/

Bool segP(Str &s, int oneOrTwo)
{
    return (Bool) !strcmp((char *) s, (oneOrTwo == 1 ? "." : ".."));
}

void joinPaths(Str& relPath, const Str& basePath)
{
    Str segment;
    DStr absPath;
    // append the relPath to all-but-the-last-segment-of-basePath

    Bool endSlash = cutLast(absPath = basePath, 1),
        lastSeg;
    DStr result = absPath + (endSlash? "" : "/") + relPath;
       
    // throw out all '.' from the path
    const char *p = (const char*) result;
    absPath.empty();
    while(splitBy(p, slashes, segment))
    {
        if (!segP(segment, 1))
            absPath += segment + "/";
    }
    if (!segP(segment, 1))
        absPath += segment;

    // throw out all "something/.." from the path
    p = (char*) absPath;
    int depth = 0;
    result.empty();
    do
    {
        lastSeg = (Bool) !splitBy(p, slashes, segment);
        if (!segP(segment, 2))
        {
            result += segment + (lastSeg ? "" : "/");
            depth++;
        }
        else
        {
            if (depth > 1)
            {
                cutLast(result, 2);
                depth--;
            }
            else
                result += segment + (lastSeg ? "" : "/");
        };
    }
    while(!lastSeg);
    relPath = result;
}


URIScheme makeAbsoluteURI(Sit S, const char* uri,
			   const char* base, Str& absolute)
{
    FiveStr 
        u_parts, 
        b_parts;
    Bool 
        u_defined[5],
        u_any = FALSE;
    Str scheme;

    // first, break up the URIs into their 5 components
    splitURI(uri, u_parts);
    splitURI(base, b_parts);

    // set u_defined[i] to TRUE if the i-th uri component is nonvoid
    for (int i = 0; i < 5; i++)
        u_any = (Bool) ((u_defined[i] = (Bool) !u_parts[i].isEmpty()) || u_any);

    if (!u_any) // all components empty: the reference is to the current document
    {
        splitURI(base,u_parts);
        u_parts[U_QUERY].empty();       // query and fragment are NOT inherited from base
        u_parts[U_FRAG].empty();
    }
    else    // not all components are empty
    {
        if (!u_defined[U_SCHEME])                       // undefined scheme
        {
            u_parts[U_SCHEME] = b_parts[U_SCHEME];      // inherit scheme from base
            if (!u_defined[U_AUTH])                     // undefined authority
            {
                u_parts[U_AUTH] = b_parts[U_AUTH];      // inherit authority from base
                if (!isSlash(u_parts[U_PATH][0]))       // path is relative
                    joinPaths(u_parts[U_PATH], b_parts[U_PATH]);    // append path to base path
                // query and fragment stay as they are in 'uri'
            }
        }
        else
        {
	    scheme = u_parts[U_SCHEME];

	    URIScheme uri_scheme = schemeToURI_(S, scheme);
	    if (uri_scheme == URI_EXTENSION)
	    {
		absolute = uri;
		return URI_EXTENSION;
	    }
	    // scheme defined, check for paths not starting with '/'
            if (!u_defined[U_AUTH] && !isSlash(u_parts[U_PATH][0]))
                u_parts[U_PATH] = Str("/") + u_parts[U_PATH];
        }
    }
    DStr joined = absolute;
    joinURI(joined, u_parts, FALSE);         // join all components into a URI for return (no scheme)

    scheme = u_parts[U_SCHEME];
    absolute = (scheme + ":") + joined;
    return schemeToURI_(S, scheme);
}


//    URIScheme makeAbsoluteURI(uri, base, absolute)
//
//    Merges a (possibly relative) URI reference with a base URI, setting
//    'absolute' to the result. 
//

// URIScheme makeAbsoluteURI(Sit S, const char* uri,
// 			  const char* base, Str& absolute)
// {
//   return makeAbsoluteURI2(S, uri, base, absolute);
// }


URIScheme uri2SchemePath(Sit S, const char *absolute, Str& scheme, Str& rest)
{
    Bool found = (Bool) !!splitBy(absolute, ":", scheme);
    sabassert(found);
    rest = (char*) absolute;
/*
 *    if (isSlash(*absolute) && isSlash(absolute[1]))
 *       rest = (char*) absolute + 2;
 *   else
 *       rest = (char*) absolute;
 */
    return schemeToURI_(S, scheme);
}


/*****************************************************************
DataLine

  is a class that holds the machinery needed to retrieve data from
  a given URI. There are two internally supported URI schemes:
  file  (the plain "file://...")
  arg   (for access to named memory blocks passed to Sablotron)

  Other schemes are passed to the extending scheme handler (if
  one has been registered). This way, requests such as http:...
  can be processed.

  The life cycle of a DataLine:
    Upon construction, no URI is attached yet.
    Call open() to associate a URI.
    Repeatedly call save() or get() to retrieve data.
    Call close() to close the resource.
    Call the destructor.

  The 'write' data line with the scheme of 'arg' will need to be
  accessible to the user even after the Processor object is destroyed;
  it is then freed by 'SablotFreeBuffer'.
*****************************************************************/

/*****************************************************************
DataLine::DataLine()

  This constructor just sets everything to zeroes and such.
*****************************************************************/

DataLine::DataLine()
{
    mode = DLMODE_NONE;       
    scheme = URI_NONE;      
    f = NULL;               
    buffer = NULL;          
    outBuf = NULL;
    bufCurr = 0;
    fileIsStd = FALSE;
    utf16Encoded = FALSE;
    handler = NULL;
    handlerUD = NULL;
    handle = 0;
    gotWholeDocument = FALSE;
}

/*****************************************************************
DataLine::~DataLine()

  The destructor asserts that the data line had been closed.
*****************************************************************/

DataLine::~DataLine()
{
    // removing the asserts (can be killed anytime due to error)
    // assert(mode == DLMODE_CLOSED || mode == DLMODE_NONE);
    // assert(!f);
    // if there is an outBuf, delete it now
    if (outBuf)
        delete outBuf;
}

/*****************************************************************
DataLine::open()

  Opens the data line for a given URI and access mode. Actual
  data transfer is only done on subsequent get() or save() calls.
  open() tries to call the extending scheme handler if it cannot
  handle a request itself.

ARGS
_uri        the URI identifier for the resource, including the
            scheme (e.g. "file:///x.xml")
_baseUri    the base URI used in case the reference in _uri is
            relative
_mode       the access mode (DLMODE_READ, DLMODE_WRITE)
*****************************************************************/

#define specErr1(S, code, arg) \
{if (ignoreErr) {Warn1(S,code,arg); return NOT_OK;} else Err1(S,code,arg);}

eFlag DataLine::open(Sit S, const char *_uri, DLAccessMode _mode, 
		     StrStrList* argList_, Bool ignoreErr /* = FALSE */)
{
    sabassert(mode == DLMODE_NONE);  // the buffer must not be open yet
    // combine _uri and _baseUri into one
    Str strScheme, strPath;
    scheme = uri2SchemePath(S, _uri, strScheme, strPath);
    char *name = (char*) strPath;

    // mode set in the end
    fullUri = (char*)_uri;

    switch(scheme)
    {
    case URI_FILE:
        {
            if (name[0] == '/' && name[1] == '/')
                name += 2;          // skipping the "//" in front
            // try to open the file
#ifdef _MSC_VER
            if (!(f = stdopen(name,_mode == DLMODE_WRITE ? "wb" : "rb")))
#else
            if (!(f = stdopen(name,_mode == DLMODE_WRITE ? "w" : "r")))
#endif
                specErr1(S, E_FILE_OPEN, winName(name));
            // set fileIsStd if filename is "stdin", "stdout" or "stderr"
            fileIsStd = isstd(name);
        }; break;
    case URI_ARG:
        {
            // if opening for read access, get the pointer to the argument contents
            // plus some extra information
            if (_mode == DLMODE_READ)
            {
	      Str *value = NULL;
	      if (argList_)
		value = argList_ -> find(name);
	      if (!value)
		specErr1(S, E1_ARG_NOT_FOUND, name);
	      buffer = (char*)*value;
            }
            // if opening for write access, just allocate a new dynamic block
            else
                outBuf = new DynBlock;
        }; break;
    default:
        {
            // try the extending scheme handler
            // ask the handler address from the Processor
	  Processor *proc = S.getProcessor();
	  if (proc)
	    handler = proc->getSchemeHandler(&handlerUD);
	  else
	    handler = NULL;
	  // if there is no handler, report unsupported scheme
	  if (!handler)
	    specErr1(S, E1_UNSUPPORTED_SCHEME, strScheme);
	  // try the fast way
	  int count = 0;
	  buffer = NULL;
	  if (_mode == DLMODE_READ && handler -> getAll)
	    handler -> getAll(handlerUD, proc,
			      strScheme, name, &buffer, &count);
	  if (buffer && (count != -1))
            {
	      gotWholeDocument = TRUE;
	      bufCurr = 0;
            }
	  else
            {
	      // call the handler's open() function, obtaining a handle
	      switch(handler -> open(handlerUD, proc, 
				     strScheme, name, &handle))
                {
                case SH_ERR_UNSUPPORTED_SCHEME:     // scheme not supported
		  specErr1(S, E1_UNSUPPORTED_SCHEME, strScheme);
                case SH_ERR_NOT_OK:
		  specErr1(S, E1_URI_OPEN, strScheme + ":" + strPath);
                };
            }
        };
    };
    // open successfully completed. Set the new mode.
    mode = _mode;
    return OK;
}

/*****************************************************************
DataLine::close()

  closes the resource attached to this data line.
*****************************************************************/
eFlag DataLine::close(Sit S)
{
    sabassert(mode != DLMODE_NONE);
    switch(scheme)
    {
    case URI_FILE:
        {
            sabassert(f);
            if (!fileIsStd)
            {
	      if (fclose(f))
		Err1(S, E1_URI_CLOSE, fullUri);
            };
            f = NULL;
        }; break;
    case URI_ARG:
        break;
    case URI_EXTENSION:
        {
            if (gotWholeDocument)
            {
                NZ(handler) -> freeMemory(handlerUD, S.getProcessor(), buffer);
            }
            else
            {
                if(NZ(handler) -> close(handlerUD, S.getProcessor(), handle))
                    Err1(S, E1_URI_CLOSE, fullUri);
            }
        }; break;
    };
    mode = DLMODE_CLOSED;
    return OK;
}

/*****************************************************************
save()

  saves an UTF-8 string pointed to by data to the data line.
  This is the place to perform any recoding, escaping and other operations
  that require char-by-char scanning of the string.
*****************************************************************/

int my_wcslen(const char *p)
{
    int len;
    for (len = 2;  *(short int*)p; p += 2, len += 2);
    return len;
}

eFlag DataLine::save(Sit S, const char *data, int length)
{
    sabassert(mode == DLMODE_WRITE); // assume the file open for writing
    // int length = utf16Encoded ? my_wcslen(data) : strlen(data);
    switch (scheme)             // choose the output procedure 
    {
    case URI_FILE:              // file: scheme
        {
            sabassert(f);          // the file must be open
            // fputs(data, f);
            fwrite(data, 1, length, f);
        }; break;
    case URI_ARG:               // arg: scheme
        {
            sabassert(outBuf);     // the output buffer must exist
            outBuf -> nadd(data, length); 
        }; break;
    case URI_EXTENSION:         // external handler
        {
            int actual = length;
            if( NZ(handler) -> put(handlerUD, S.getProcessor(), handle, data, &actual) )
                Err1(S, E1_URI_WRITE, fullUri);
        };
    }
    return OK;
}

/*................................................................
pointsAtEnd()

  DESCRIPTION
a macro that returns nonzero if the given char* points at a 
string terminator

  ARGS
p       the pointer
is16    TRUE iff the string is UTF-16
................................................................*/

#define pointsAtEnd(p, is16) ((is16) ? (!*(unsigned short*)(p)) : (!*(p)))

/*****************************************************************
    get()

- retrieves at most 'maxcount' bytes into buffer 'dest'.
- input should be NUL-terminated
- if a terminating 0 is reached, copying stops
*****************************************************************/

int DataLine::get(Sit S, char *dest,int maxcount)
{
    int result = 0;
    sabassert(mode == DLMODE_READ);  // assume the file open for reading
    switch(scheme)
    {
    case URI_FILE:
        {
            sabassert(f);          // the file must be open
            result = fread(dest,1,maxcount,f);
            // return the number of bytes read
        }; break;
    case URI_ARG:
        {
            sabassert(buffer);     // the buffer must exist
            // do a 'strncpy' that shifts dest and bufCurr;
            // i counts the number of bytes transferred
			char * copyChar = dest;
            int i;
            for (i = 0; 
                (!pointsAtEnd(buffer + bufCurr, utf16Encoded)) && (i < maxcount); 
                i++)
                {
                    *(copyChar++) = buffer[bufCurr++];
                };
                result = i;
        }; break;
    case URI_EXTENSION:         // external handler
        {
            if (gotWholeDocument)
            {
                // ugly hack: copied the following from above
                sabassert(buffer);     // the buffer must exist
				char * copyChar = dest;
                int i;
                for (i = 0; 
                (!pointsAtEnd(buffer + bufCurr, utf16Encoded)) && (i < maxcount); 
                i++)
                {
                    *(copyChar++) = buffer[bufCurr++];
                };
                result = i;
            }
            else
            {
                int actual = maxcount;
                if( NZ(handler) -> get(handlerUD, S.getProcessor(), handle, dest, &actual) )
                {
                    S.message( MT_ERROR, E1_URI_READ, fullUri, "" );
                    return -1;
                }
                result = actual;
            }
        }; break;
    }
	// need to NUL terminate in order to prevent C string
	// functions running off the end of the buffer

	// assignment assumes that the passed in dest is allocated
	// one bigger than maxcount
	dest[result] = '\0';
    return result;              // return the number of bytes read
}

/*****************************************************************
getOutBuffer()

  returns the pointer to the output buffer which may be used after
  all processing is finished (remains allocated along with the
  whole DataLine)
*****************************************************************/

DynBlock* DataLine::getOutBuffer()
{
    // check that the output buffer exists and that we're open for write
    sabassert(mode == DLMODE_WRITE && scheme == URI_ARG); 
    return NZ(outBuf); // -> getPointer();
}

eFlag DataLine::setURIAndClose(Sit S, const char *_uri)
{
    sabassert( mode == DLMODE_NONE );
    mode = DLMODE_CLOSED;
    scheme = URI_ARG;
    fullUri = _uri;
    return OK;
}

void DataLine::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type, code, arg1, arg2);
}

