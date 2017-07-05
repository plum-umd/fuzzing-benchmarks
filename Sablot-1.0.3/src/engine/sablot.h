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
 * Portions created by Ginger Alliance are Copyright (C) 2000-2003
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

/*
sablot.h
TK Dec 14, 99
header file for Sablot.cpp
*/

#ifndef SablotHIncl
#define SablotHIncl


/* version info */
#define SAB_VERSION "1.0.3"
#define SAB_DATE "June 26, 2006"

/* common types */
typedef void *SablotHandle;
typedef void *SDOM_Document;
typedef void *SablotSituation;

#if defined(WIN32) && defined(_MSC_VER) && !defined(SABLOT_STATIC)
#if defined(SablotAsExport)
#define DllImpExp __declspec( dllexport )
#else
#define DllImpExp __declspec( dllimport )
#endif /* SablotAsExport */
#else  /* WIN32 && _MSC_VER */
#define DllImpExp extern
#endif

#ifdef __cplusplus
#define DeclBegin extern "C" { DllImpExp
#define DeclEnd }
#else
#define DeclBegin DllImpExp
#define DeclEnd
#endif

#define Declare(STATEMENT) DeclBegin STATEMENT DeclEnd

#include "shandler.h"
#include "sxpath.h"

typedef enum
{
  SAB_NO_ERROR_REPORTING    =  0x1,
  SAB_PARSE_PUBLIC_ENTITIES =  0x2,
  SAB_DISABLE_ADDING_META   =  0x4,
  SAB_DISABLE_STRIPPING     =  0x8,
  SAB_IGNORE_DOC_NOT_FOUND  = 0x10,
  SAB_FILES_TO_HANDLER      = 0x20,
  SAB_DUMP_SHEET_STRUCTURE  = 0x40,
  SAB_NO_EXTERNAL_ENTITIES  = 0x80
}  SablotFlag;

/* create a new document */

Declare
(
    int SablotCreateDocument(
        SablotSituation S, 
	    SDOM_Document *D);
)

/* parse in a document from the given URI */

Declare
(
    int SablotParse(
        SablotSituation S, 
        const char *uri, 
	    SDOM_Document *D);
)

/* parse a document given in an in-memory buffer */

Declare
(
    int SablotParseBuffer(
        SablotSituation S, 
        const char *buffer, 
	    SDOM_Document *D);
)

Declare
(
    int SablotParseStylesheet(
        SablotSituation S, 
        const char *uri, 
        SDOM_Document *D);
)


Declare
(
    int SablotParseStylesheetBuffer(
        SablotSituation S, 
        const char *buffer, 
        SDOM_Document *D);
)


/* lock document before using it */

Declare
(
    int SablotLockDocument(
        SablotSituation S, 
	    SDOM_Document D);
)

Declare
(
    int SablotDestroyDocument(
        SablotSituation S, 
	    SDOM_Document D);
)

Declare
(
    int SablotAddParam(
        SablotSituation S,
        void *processor_,
        const char *paramName,
        const char *paramValue);
)

Declare
(
    int SablotAddArgBuffer(
        SablotSituation S,
        void *processor_,
        const char *argName,
        const char *bufferValue);
)

Declare(
    int SablotAddArgTree(
        SablotSituation S,
        void *processor_,
        const char *argName,
        SDOM_Document tree);
)

Declare
(
    int SablotRunProcessorGen(
        SablotSituation S,
        void *processor_,
        const char *sheetURI, 
        const char *inputURI, 
        const char *resultURI);    
)

Declare
(
    int SablotRunProcessorExt(
        SablotSituation S,
        void *processor_,
        const char *sheetURI, 
        const char *resultURI,
	NodeHandle doc);
)

/*
 *    Situation functions
 */

/* Creates a new situation. */

Declare
(
    int SablotCreateSituation(SablotSituation *sPtr);
)

/* Sets situation flags. */

Declare
(
    int SablotSetOptions(SablotSituation S, int flags);
)

Declare
(
    int SablotGetOptions(SablotSituation S);
)

Declare
(
    int SablotClearSituation(SablotSituation S);
)

Declare
(
    const char* SablotGetErrorURI(SablotSituation S);
)

Declare
(
    int SablotGetErrorLine(SablotSituation S);
)

Declare
(
    const char* SablotGetErrorMsg(SablotSituation S);
)

/* Disposes of the situation. */
 
Declare
(
    int SablotDestroySituation(SablotSituation S);
)


/*****************************************************************
SablotCreateProcessor
creates a new instance of the processor
*****************************************************************/

Declare
(
    int SablotCreateProcessor(SablotHandle *processorPtr);
)

/*****************************************************************
SablotCreateProcessorForSituation
use this instead of SablotCreateProcessor with any of the
situation-aware functions like SablotRunProcessorGen,
SablotAddArgTree etc.
*****************************************************************/

Declare
(
    int SablotCreateProcessorForSituation(SablotSituation S, void **processorPtr);
)


/*****************************************************************
SablotDestroyProcessor
destroys the processor
*****************************************************************/

Declare
(
    int SablotDestroyProcessor(SablotHandle processor_);
)

/*****************************************************************
SablotRunProcessor
runs the existing instance on the given documents
*****************************************************************/

Declare
(
    int SablotRunProcessor(SablotHandle processor_,
        const char *sheetURI, 
        const char *inputURI, 
        const char *resultURI,
        const char **params, 
        const char **arguments);
)

/*****************************************************************
SablotGetResultArg
gets the result arg buffer from the last Sablot run
the buffer is identified by an URI (to enable output to
    multiple documents) 
*****************************************************************/

Declare
(
    int SablotGetResultArg(SablotHandle processor_,
        const char *argURI,
        char **argValue);
)

/*****************************************************************
SablotFreeResultArgs
kill all result arg buffers from the last Sablot run
*****************************************************************/

Declare
(
    int SablotFreeResultArgs(SablotHandle processor_);
)

/*****************************************************************
SablotRegHandler
General handler registrator.
    type        the type of the handler (scheme etc.)
    handler     pointer to the struct of callbacks of the given type
    userData    the user data this handler wishes to receive
*****************************************************************/

Declare
(
    int SablotRegHandler(
        SablotHandle processor_, 
        HandlerType type,   /* HLR_MESSAGE, HLR_SCHEME, HLR_SAX */
        void *handler, 
        void *userData);
)

/*****************************************************************
SablotUnregHandler

  General handler unregistrator.
*****************************************************************/

Declare
(
    int SablotUnregHandler(
        SablotHandle processor_, 
        HandlerType type,   /* HLR_MESSAGE, HLR_SCHEME, HLR_SAX */
        void *handler,
        void *userData);
)

/*****************************************************************
SablotSetBase

    overrides the default base URI for relative reference 
    resolution.
*****************************************************************/

Declare
(
    int SablotSetBase(
        SablotHandle processor_, 
        const char *theBase);
)

/*****************************************************************
SablotSetBaseForScheme

    a softer form of SablotSetBase: the hard base URI will only
    be in effect for relative references whose bases have
    the given scheme.

  Example: assume we call 
    SablotSetBaseForScheme( P, "arg", "http://server" )
  and then runs a stylesheet at "arg:/xxx" which contains "document('foo.xml')".
  The relative reference is resolved as "http://server/foo.xml"
  but if the stylesheet were at "file:/xxx" it would become "file:/foo.xml". 
*****************************************************************/

Declare
(
    int SablotSetBaseForScheme(void* processor_, 
        const char *scheme, 
        const char *base);
)

/****************************************************************
SablotSetLog

  sets the logging options. Logging is off by default.
*****************************************************************/

Declare
(
    int SablotSetLog(
        SablotHandle processor_,
        const char *logFilename, 
        int logLevel);
)


/*****************************************************************
SablotProcess

  the main function published by Sablotron. Feeds given XML
  input to a stylesheet. Both of these as well as the location for
  output are identified by a URI. One can also pass top-level
  stylesheet parameters and named buffers ('args').
ARGS
  sheetURI      URI of the XSLT stylesheet
  inputURI      URI of the XML document
  resultURI     URI of the output document
  params        a NULL-terminated array of char*'s defining the top-level
                parameters to be passed, interpreted as a
                sequence of (name, value) pairs. 
  arguments     a NULL-terminated array of char*'s defining the named
                buffers to be passed, interpreted as a
                sequence of (name, value) pairs. Both params and arguments
                may be NULL. 
RETURNS
  .             nonzero iff error
  resultArg     in case output goes to a named buffer, *resultArg is set to                     
                point to it (otherwise to NULL). Free it using SablotFree().
*****************************************************************/

Declare
(
    int SablotProcess(
        const char *sheetURI, const char *inputURI, const char *resultURI,
        const char **params, const char **arguments, char **resultArg);
)

/*****************************************************************
SablotProcessFiles

  calls SablotProcess to process files identified by their
  file names. No named buffers or params are passed. Included
  for backward compatibility.
ARGUMENTS
  styleSheetName, inputName, resultName
            file names of the stylesheet, XML input and output,
            respectively.
RETURNS
  .         error flag
*****************************************************************/

Declare
(
    int SablotProcessFiles(
        const char *styleSheetName,
        const char *inputName,
        const char *resultName);
)

/*****************************************************************
SablotProcessStrings

  calls SablotProcess to process documents in memory, passing
  pointers to the documents. No named buffers or params are passed.
  Included for backward compatibility.
ARGUMENTS
  styleSheetStr, inputStr
                text of the stylesheet and the XML input
RETURNS
  .             error flag
  *resultStr    pointer to the newly allocated block containing
                the result 
*****************************************************************/

Declare
(
    int SablotProcessStrings(
        const char *styleSheetStr,
        const char *inputStr,
        char **resultStr);
)

/*****************************************************************
SablotProcessStringsWithBase

    Like SablotProcessStrings but lets you pass an URI that replaces
    the stylesheet's URI in relative address resolution.

ARGUMENTS
  styleSheetStr, inputStr
                text of the stylesheet and the XML input
  theHardBase   the "hard base URI" replacing the stylesheet's URI
                in all relevant situations
RETURNS
  .             error flag
  *resultStr    pointer to the newly allocated block containing
                the result 
*****************************************************************/

Declare
(
    int SablotProcessStringsWithBase(
        const char *styleSheetStr,
        const char *inputStr,
        char **resultStr,
        const char *theHardBase);
)

/*****************************************************************
SablotFree

  Frees the Sablotron-allocated buffer. The user cannot free it
  directly by free().
*****************************************************************/

Declare
(
    int SablotFree(char *resultStr);
)

/*****************************************************************
SablotClearError

  Clears the pending error for the given instance of Sablot.
*****************************************************************/

Declare
(
    int SablotClearError(SablotHandle processor_);
)

/*****************************************************************
SablotGetMsgText

  Returns the text of a message with the given code.
*****************************************************************/

Declare
(
    const char* SablotGetMsgText(int code);
)

/*****************************************************************
SablotSetInstanceData
*****************************************************************/

Declare
(
    void SablotSetInstanceData(SablotHandle processor_, void *idata);
)

/*****************************************************************
SablotGetInstanceData
*****************************************************************/

Declare
(
    void* SablotGetInstanceData(SablotHandle processor_);
)

/*
      SablotSetEncoding
      sets the output encoding to be used regardless of the encoding
      specified by the stylesheet
      To unset, call with encoding_ NULL.
*/

Declare
(
    void SablotSetEncoding(SablotHandle processor_, char *encoding_);
)

#endif
