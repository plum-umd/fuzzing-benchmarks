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

#ifndef ShandlerHIncl
#define ShandlerHIncl

/* we have to deal with the size_t type, sys/types.h;
   is needed on some platforms */
#if !defined(_MSC_VER) && !defined(__BORLANDC__)
#include <sabcfg.h>
#endif

#include <stddef.h>

/* GP: clean */

/*****************************************************************

  handler types

*****************************************************************/

typedef enum
{
    HLR_MESSAGE = 0,
    HLR_SCHEME,
    HLR_SAX,
    HLR_MISC,
    HLR_ENC
} HandlerType;

extern const char* hlrTypeNames[]; /* found in base.cpp */

typedef enum 
{
    SH_ERR_OK = 0,
    SH_ERR_NOT_OK = 1,
    SH_ERR_UNSUPPORTED_SCHEME
} SchemeHandlerErrors;

/*****************************************************************
SchemeHandler 

  is a structure for a scheme handler. It contains pointers to
  the following functions of the handler: 
        open(), get(), put(), close().
  All of these function return an error flag (0=OK, 1=not). 
  open() may also return SH_ERR_UNSUPPORTED_SCHEME.
*****************************************************************/

/*  getAll: open the URI and return the whole string
        scheme = URI scheme (e.g. "http")
        rest = the rest of the URI (without colon)
        the document is returned in a handler-allocated buffer
        byteCount holds the byte count on return
        return *buffer = NULL if not processed
*/
typedef int SchemeHandlerGetAll(void *userData, SablotHandle processor_,
    const char *scheme, const char *rest, 
    char **buffer, int *byteCount);

/*  freeMemory: free the buffer allocated by getAll
*/

typedef int SchemeHandlerFreeMemory(void *userData, SablotHandle processor_,
    char *buffer);

/*  open: open the URI and return a handle
        scheme = URI scheme (e.g. "http")
        rest = the rest of the URI (without colon)
        the resulting handle is returned in '*handle'
*/
typedef int SchemeHandlerOpen(void *userData, SablotHandle processor_,
    const char *scheme, const char *rest, int *handle);

/*  get: retrieve data from the URI
        handle = the handle assigned on open
        buffer = pointer to the data
        *byteCount = number of bytes to read 
            (the number actually read is returned here)
*/
typedef int SchemeHandlerGet(void *userData, SablotHandle processor_,
    int handle, char *buffer, int *byteCount);

/*  put: save data to the URI (if possible)
        handle = the handle assigned on open
        buffer = pointer to the data
        *byteCount = number of bytes to write
            (the number actually written is returned here)
*/
typedef int SchemeHandlerPut(void *userData, SablotHandle processor_,
    int handle, const char *buffer, int *byteCount);

/*  close: close the URI with the given handle
        handle = the handle assigned on open
*/
typedef int SchemeHandlerClose(void *userData, SablotHandle processor_,
    int handle);

typedef struct
{
    SchemeHandlerGetAll *getAll;
    SchemeHandlerFreeMemory *freeMemory;
    SchemeHandlerOpen *open;
    SchemeHandlerGet *get;
    SchemeHandlerPut *put;
    SchemeHandlerClose *close;
} SchemeHandler;

/*****************************************************************
MessageHandler 

  a structure for external message handlers. Such a handler, if set,
  receives all error reports, displays them, keeps the log, the 
  error trace, etc.
*****************************************************************/

/*
   define the "facility number" for Sablotron. This does not mean much
   nowadays.
*/

#define MH_FACILITY_SABLOTRON 2

/* type for the error codes used by the message handler */

typedef unsigned long MH_ERROR;

/* logging levels for the message handler */

typedef enum 
{
    MH_LEVEL_DEBUG,
    MH_LEVEL_INFO,
    MH_LEVEL_WARN,
    MH_LEVEL_ERROR,
    MH_LEVEL_CRITICAL
} MH_LEVEL;

/*
   makeCode()
   makes the "external" error code to report with log() or error()
   call with facility = module id; severity = 1 iff critical.
   'code' is the error code internal to Sablotron.
*/

typedef MH_ERROR 
MessageHandlerMakeCode(
    void *userData, SablotHandle processor_,
    int severity, unsigned short facility, unsigned short code);

/*
   log()
   pass code created by makeCode, level as necessary
   fields is a NULL-terminated list of strings in form "field:contents"
   distinguished fields include: msg, file, line, token
*/

typedef MH_ERROR 
MessageHandlerLog(
    void *userData, SablotHandle processor_,
    MH_ERROR code, MH_LEVEL level, char **fields);

/*
   error()
   for reporting errors, meaning as with log()
*/

typedef MH_ERROR 
MessageHandlerError(void *userData, SablotHandle processor_,
    MH_ERROR code, MH_LEVEL level, char **fields);

/* the message handler structure. Use SablotRegMessageHandler() to register. */

typedef struct
{
    MessageHandlerMakeCode *makeCode;
    MessageHandlerLog *log;
    MessageHandlerError *error;
} MessageHandler;





/*
  
                SAXHandler
    a SAX-like, streaming interface for access to XML docs
  
*/


#define SAX_RETURN void

typedef SAX_RETURN 
SAXHandlerStartDocument(void* userData, SablotHandle processor_);

typedef SAX_RETURN 
SAXHandlerStartElement(void* userData, SablotHandle processor_,
    const char* name, const char** atts);

typedef SAX_RETURN 
SAXHandlerEndElement(void* userData, SablotHandle processor_,
    const char* name);

typedef SAX_RETURN 
SAXHandlerStartNamespace(void* userData, SablotHandle processor_,
    const char* prefix, const char* uri);

typedef SAX_RETURN 
SAXHandlerEndNamespace(void* userData, SablotHandle processor_,
    const char* prefix);

typedef SAX_RETURN 
SAXHandlerComment(void* userData, SablotHandle processor_,
    const char* contents);

typedef SAX_RETURN 
SAXHandlerPI(void* userData, SablotHandle processor_,
    const char* target, const char* contents);

typedef SAX_RETURN 
SAXHandlerCharacters(void* userData, SablotHandle processor_,
    const char* contents, int length);

typedef SAX_RETURN 
SAXHandlerEndDocument(void* userData, SablotHandle processor_);


/*
    The SAX handler structure. Use SablotRegSAXHandler() to register.
*/


typedef struct
{
    SAXHandlerStartDocument     *startDocument;
    SAXHandlerStartElement      *startElement;
    SAXHandlerEndElement        *endElement;
    SAXHandlerStartNamespace    *startNamespace;
    SAXHandlerEndNamespace      *endNamespace;
    SAXHandlerComment           *comment;
    SAXHandlerPI                *processingInstruction;
    SAXHandlerCharacters        *characters;
    SAXHandlerEndDocument       *endDocument;
} SAXHandler;


/*****************************************************************
MiscHandler

  Collects miscellaneous callbacks.
*****************************************************************/

/*
    documentInfo()
    If set, this callback gets called after the output of a result
    document is finished, giving information about its content type
    and encoding.
*/

typedef void
MiscHandlerDocumentInfo(void* userData, SablotHandle processor_,
    const char *contentType, const char *encoding);

/*
    The Misc handler structure. 
    Use SablotRegHandler(HLR_MISC, ...) to register.
*/

typedef struct
{
    MiscHandlerDocumentInfo     *documentInfo;
} MiscHandler;

/*****************************************************************
EncHandler

  Handler for recoding requests in absence of iconv.
*****************************************************************/

#define EH_FROM_UTF8 1
#define EH_TO_UTF8 0

/*
    the conversion descriptor like iconv_t
*/

typedef void* EHDescriptor;

typedef enum 
{
    EH_OK,
    EH_EINVAL,
    EH_E2BIG,
    EH_EILSEQ
} EHResult;

/*
    open()
    direction is either EH_FROM_UTF8 or EH_TO_UTF8
    encoding is the other encoding
    RETURN the descriptor, or -1 if the encoding is not supported
*/

typedef EHDescriptor EncHandlerOpen(void* userData, SablotHandle processor_,
    int direction, const char *encoding);

/*
    conv()
    arguments 3 through 7 are just like for iconv, see the manpage
    RETURN -1 on error (set errno), a different value (e.g. 0) if OK
*/

typedef EHResult EncHandlerConv(void* userData, SablotHandle processor_,
    EHDescriptor cd, const char** inbuf, size_t *inbytesleft,
    char ** outbuf, size_t *outbytesleft);

/*
    close()
    cd is the descriptor to close. Return 0 if OK, -1 on error.
*/

typedef int EncHandlerClose(void* userData, SablotHandle processor_,
    EHDescriptor cd);

/*
    The EncHandler structure. 
    Use SablotRegHandler(HLR_ENC, ...) to register.
*/

typedef struct
{
    EncHandlerOpen      *open;
    EncHandlerConv      *conv;
    EncHandlerClose     *close;
} EncHandler;

#endif
