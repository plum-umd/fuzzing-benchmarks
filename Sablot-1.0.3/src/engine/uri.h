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
    uri.h
    Sablotron XSLT processor project
    
This module does the work related to retrieving data in answer to
a URI request.
*****************************************************************/

#ifndef UriHIncl
#define UriHIncl

// GP; clean

#include "base.h"
#include "shandler.h"
#include "datastr.h"

/*  URIScheme
    possible URI schemes for a DataLine 
        - file
        - arg for access to named buffers
        - other scheme to by handled by an extension (a SchemeHandler)
*/
typedef enum
{
    URI_FILE, URI_ARG, URI_EXTENSION, URI_NONE
} URIScheme;

/*****************************************************************
makeAbsoluteURI()

  merges a (possibly relative) URI reference with a base URI.
ARGS
  uri       the URI reference
  base      the base URI
RETURNS
  absolute  the result
*****************************************************************/


URIScheme makeAbsoluteURI(Sit S, const char* uri,
			  const char* base, Str& absolute);
URIScheme uri2SchemePath(Sit S, const char *absolute, Str& scheme, Str& rest);


/*  DLAccessMode
    possible access modes for a DataLine
*/

typedef enum
{
    DLMODE_NONE, DLMODE_READ, DLMODE_WRITE, DLMODE_CLOSED
} DLAccessMode;

/*****************************************************************  
DataLine
    a class associated to any data source or destination at a given URI; 
    - can be open for reading or for writing
    - 'Processor' class has a list of open DataLines 
        together with the associated trees (serving as a cache)
*****************************************************************/

class StrStrList;

class DataLine
{
public:
    DataLine();
    ~DataLine();
    // opens the resource at _uri for reading/writing based on _mode:
    eFlag open(Sit S, const char *_uri, DLAccessMode _mode, 
	       StrStrList* argList_, Bool ignoreErr = FALSE);
    eFlag close(Sit S);
    // sends data to the resource
    eFlag save(Sit S, const char *data, int len);
    int get(Sit S, char *where, int maxcount);
    DynBlock* getOutBuffer();
    Str fullUri;
    DLAccessMode mode;
    URIScheme scheme;
    eFlag setURIAndClose(Sit S, const char *_uri);
private:
    FILE *f;
    char *buffer;
    DynBlock *outBuf;
    int bufCurr;
    SchemeHandler *handler;
    void *handlerUD;
    int handle;
    Bool 
        fileIsStd,
        utf16Encoded,
        gotWholeDocument;
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);	
};
    
#endif
