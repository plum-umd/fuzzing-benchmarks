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

/****************************************
*                                       *
i n c l u d e s
*                                       *
****************************************/

// #include<iostream.h>

// GP: clean

#include <stdarg.h>

#define SablotAsExport
#include "sablot.h"

#include "base.h"
#include "verts.h"
#include "tree.h"
#include "expr.h"
#include "proc.h"
#include "situa.h"
#include "uri.h"
#include "parser.h"
#include "guard.h"
#include "sxpath.h"
#include "domprovider.h"

//
//  defines
//

#define P( PTR ) ((Processor *)(PTR))
#define EE(statement) {int code___=(statement); if (code___) return code___;}
#define ES(statement) {if (statement) return SIT(S).getError();}
// same as EE() but destroys the given processor
#define EE_D(CODE,PROC) {int code___=(CODE); if (code___) {SablotDestroyProcessor(PROC); return code___;}}
// error-checking chain
#define EC( VAR, STMT ) {if (!VAR) VAR = STMT;}
#define SIT( S ) (*(Situation*)S)


/****************************************
g l o b a l s
****************************************/
#define checkErr(situation) {if (situation->isError()) \
    {Warn(situation, W_PENDING_ERROR); return NOT_OK;}}


/****************************************
d e f i n i t i o n s
****************************************/

void doStart(Sit S)
{
    S.message(MT_LOG, L_START, S.timeStr(), (char*) NULL);
//    Log1(S, L_START, S.timeStr());
};

void doEnd(Sit S)
{
    S.message(MT_LOG, L_STOP, S.timeStr(), (char*) NULL);
//    Log1(S, L_STOP, S.timeStr());
}

#define ErrQ(S, e) {\
    S.message(MT_ERROR, e,(char*) NULL, (char*)NULL); \
    doEnd(S); \
    return e; \
	}

// report function to enable the Sablot... globals to use Err and Log		
void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2) 
{
    S.message(type, code, arg1, arg2);
}


//
//
//      situation functions
//
//

int SablotCreateSituation(SablotSituation *sPtr)
{
    *sPtr = new Situation;
    if (!*sPtr)
        return E_MEMORY;
    return OK;
}

int SablotDestroySituation(SablotSituation S)
{
    if (S) delete (Situation*)S;
    S = NULL;
    return OK;
}

int SablotSetOptions(SablotSituation S, int flags)
{
    ((Situation*)S) -> setFlags(flags);
    return OK;
}

int SablotGetOptions(SablotSituation S)
{
  return ((Situation*)S) -> getFlags();
}

int SablotClearSituation(SablotSituation S)
{
    ((Situation*)S) -> clear();
    return OK;
}

//
//    document functions
//

int SablotCreateDocument(SablotSituation S, SDOM_Document *D)
{
    // non-XSL tree with empty name
    Tree *t = new Tree((char*)"", FALSE);
    SabArena &ar = t -> getArena();
    NmSpace *nm = new (&ar) NmSpace(*t, t -> unexpand("xml"), 
				    t -> unexpand(theXMLNamespace),
				    TRUE, NSKIND_DECLARED);
    RootNode &root = t -> getRoot();
    root.namespaces.append(nm);
    *D = &root;
    return OK;
}

int SablotParse_(
    SablotSituation S, 
    const char *uri, 
    const char *buffer,
    SDOM_Document *D,
    Bool asStylesheet)
{
    Str absolute;
    StrStrList argList;
    DStr base;
    *D = NULL;
    double time_was = getMillisecs();
    int code = 0;

    SIT(S).clear();
    // make absolute address
    char *parserBase = NULL;
    if (!buffer)
    {
        my_getcwd(base);
        base = Str("file://") + base;
        // should call Processor::findBaseURI() here (but have no Processor)
        makeAbsoluteURI(SIT(S), uri, base, absolute);
	parserBase = (char*)absolute;
    }
    else
      absolute = "arg:/_parsed_";
    
    // create the tree and dataline
    DataLine line;
    GP( Tree ) newTree = new Tree(absolute, /* isXSL = */ asStylesheet);
    TreeConstructer tc(SIT(S));
    
    // add buffer if there is one
    if (buffer)
        argList.appendConstruct("/_parsed_", buffer);
		    
    // open the line
    EC( code, line.open(SIT(S), absolute, DLMODE_READ, &argList) );
    
    Log1(SIT(S), L1_PARSING, absolute);
    EC( code, tc.parseDataLineUsingExpat(SIT(S), newTree, &line, parserBase) );        
    EC( code, line.close(SIT(S)) );
    *D = &(newTree.keep() -> getRoot());    
    Log1(SIT(S), L1_PARSE_DONE, getMillisecsDiff(time_was));

    argList.freeall(FALSE);
    return SIT(S).getError();
}


int SablotParse(
    SablotSituation S, 
    const char *uri, 
    SDOM_Document *D)
{
    return SablotParse_(S, uri, NULL, D, FALSE);
}

int SablotParseBuffer(
    SablotSituation S, 
    const char *buffer, 
    SDOM_Document *D)
{
    return SablotParse_(S, NULL, buffer, D, FALSE);
}

int SablotParseStylesheet(
    SablotSituation S, 
    const char *uri, 
    SDOM_Document *D)
{
    return SablotParse_(S, uri, NULL, D, TRUE);
}

int SablotParseStylesheetBuffer(
    SablotSituation S, 
    const char *buffer, 
    SDOM_Document *D)
{
    return SablotParse_(S, NULL, buffer, D, TRUE);
}

int SablotLockDocument(SablotSituation S, SDOM_Document D)
{
    ((RootNode*)D) -> getOwner().makeStamps();
    return OK;
}

int SablotDestroyDocument(SablotSituation S, SDOM_Document D)
{
  delete &((toRoot(toV(D))) -> getOwner());
  return OK;
}


//
//
//      SablotCreateProcessor
//
//

int SablotCreateProcessor(void **processorPtr)
{
    // to debug memory leaks in MSVC, uncomment the call to checkLeak()
    // and set _crtBreakAlloc to the # of block (proc=51)

    // create a new situation for this processor
    void *newSit = NULL;
    SablotCreateSituation(&newSit);

#if defined(HAVE_MTRACE) && defined(CHECK_LEAKS)
    mtrace();
#endif
    *processorPtr = new Processor;
    if (!*processorPtr)
    {
        ((Situation*)newSit) -> message(MT_ERROR, E_MEMORY, (char*) NULL, (char*) NULL);
	return E_MEMORY;
        // Err( *newSit, E_MEMORY );
    }
    // the processor must remember its situation (this is just to
    // avoid compatibility breakdown)
    P( *processorPtr ) -> rememberSituation((Situation*)newSit);
    SIT( newSit).setProcessor( P(*processorPtr) );

    // the following seems superfluous - the processor is new
    // checkErr( P(*processorPtr)->situation );
    doStart ( SIT( newSit ) );
    return OK;
}

int SablotCreateProcessorForSituation(SablotSituation S, void **processorPtr)
{
#if defined(HAVE_MTRACE) && defined(CHECK_LEAKS)
    mtrace();
#endif
    *processorPtr = new Processor;
    if (!*processorPtr)
    {
        SIT(S).message(MT_ERROR, E_MEMORY, (char*) NULL, (char*) NULL);
	return E_MEMORY;
    }
    P( *processorPtr ) -> rememberMasterSituation((Situation*)S);
    SIT(S).setProcessor( P(*processorPtr) ); // this will go asap
    doStart ( SIT(S) );
    return OK;
}

int SablotAddParam(
    SablotSituation S,
    void *processor_,
    const char *paramName,
    const char *paramValue)
{
    SIT(S).clear();
    ES( P(processor_) -> addGlobalParam(SIT(S), 
        (char*)paramName, (char*)paramValue) );
    return OK;
}

int SablotAddArgBuffer(
    SablotSituation S,
    void *processor_,
    const char *argName,
    const char *bufferValue)
{
    SIT(S).clear();
    if (!(P(processor_) -> getAddedFlag()))
        SablotFreeResultArgs (processor_);
    ES( P(processor_) -> useArg(SIT(S), (char*)argName, (char*)bufferValue) );
    return OK;
}

int SablotAddArgTree(
    SablotSituation S,
    void *processor_,
    const char *argName,
    SDOM_Document tree)
{
    SIT(S).clear();
    if (!(P(processor_) -> getAddedFlag()))
        SablotFreeResultArgs (processor_);
    ES( P(processor_) -> useTree(SIT(S), (char*) argName, 
				 &(toRoot(tree) -> getOwner())) );
    return OK;
}

int SablotRunProcessorGen(
    SablotSituation S,
    void *processor_,
    const char *sheetURI, 
    const char *inputURI, 
    const char *resultURI)
{
    int code = 0;

    void *saveproc = processor_;
    SIT(S).swapProcessor(saveproc);

    SIT(S).clear();
    // remove the existing 'arg' buffers
    if (!(P(processor_) -> getAddedFlag()))
        EC( code, SablotFreeResultArgs (processor_) );
    if (!code)
	P(processor_) -> prepareForRun();
    // must first open the files, then use global params
    // because VarsList needs stylesheet's dictionary
    EC( code, P(processor_) -> open(SIT(S), sheetURI,inputURI) );
    EC( code, P(processor_) -> useGlobalParams(SIT(S)) );
    EC( code, P(processor_) -> run(SIT(S), resultURI) );
    code = SIT(S).getError();
    P(processor_) -> cleanupAfterRun((Situation*)S);
    if (code)
    {
        // GP: if not all files could be opened, those that could are
        // closed in cleanupAfterRun
        // code = SIT(S).getError();
        P(processor_) -> freeResultArgs(SIT(S));
    };
    
    SIT(S).swapProcessor(saveproc);
    return code;
}

int SablotRunProcessorExt(
    SablotSituation S,
    void *processor_,
    const char *sheetURI, 
    const char *resultURI, 
    NodeHandle doc)
{
    int code = 0;
    //mask external node
    doc = SXP_MASK_LEVEL(doc, SIT(S).getSXPMaskBit());

    void *saveproc = processor_;
    SIT(S).swapProcessor(saveproc);

    SIT(S).clear();
    // remove the existing 'arg' buffers
    if (!(P(processor_) -> getAddedFlag()))
        EC( code, SablotFreeResultArgs (processor_) );
    if (!code)
	P(processor_) -> prepareForRun();
    // must first open the files, then use global params
    // because VarsList needs stylesheet's dictionary
    EC( code, P(processor_) -> open(SIT(S), sheetURI, NULL) );
    EC( code, P(processor_) -> useGlobalParams(SIT(S)) );
    EC( code, P(processor_) -> run(SIT(S), resultURI, doc) );
    code = SIT(S).getError();
    P(processor_) -> cleanupAfterRun((Situation*)S);
    if (code)
    {
        // GP: if not all files could be opened, those that could are
        // closed in cleanupAfterRun
        // code = SIT(S).getError();
        P(processor_) -> freeResultArgs(SIT(S));
    };
    
    SIT(S).swapProcessor(saveproc);
    return code;
}


//
//
//      SablotRunProcessor
//
//
int SablotRunProcessor(void *processor_,
                       const char *sheetURI, 
                       const char *inputURI, 
                       const char *resultURI,
                       const char **params, 
                       const char **arguments)
{    
    Bool problem = FALSE;
    Sit S = *(NZ(P( processor_ )) -> recallSituation());

    {
        // remove the existing 'arg' buffers
	    S.clearError();
        E( SablotFreeResultArgs (processor_));
        P(processor_) -> prepareForRun();
        const char **p;
        if (arguments)
        {
            for (p = arguments; *p && !problem; p += 2)
                problem = P(processor_) -> useArg(S, p[0], p[1]);
        }
	    if (!problem)
	    {
	        // must first open the files, then use global params
		    // because VarsList needs stylesheet's dictionary
	        problem = P(processor_) -> open(S, sheetURI,inputURI);
            if (params)
            {
                for (p = params; *p && !problem; p += 2)
                    problem = P(processor_) -> useGlobalParam(S, p[0], p[1]);
            }
	    }
        if (problem || P(processor_) -> run(S, resultURI))
        {
            // GP: if not all files could be opened, those that could are
            // closed in cleanupAfterRun
            int code = S.getError();
            P(processor_) -> cleanupAfterRun(&S);
            P(processor_) -> freeResultArgs(S);
//            cdelete(proc);
            // S.clear(); (why forget the message?)
            return code;
        }
    }
    P(processor_) -> cleanupAfterRun(&S);
    return OK;
}



//
//
//      SablotDestroyProcessor
//
//


int SablotDestroyProcessor(void *processor_)
{
    int code;
    Bool killSit = !(P( processor_ ) -> situationIsMaster());
    Situation *SP = P(processor_) -> recallSituation();
    code = SablotFreeResultArgs(processor_);
    doEnd(*SP);
    if(processor_) delete P( processor_);
    processor_ = NULL;    
    if (killSit)
	cdelete(SP);
#if defined(WIN32) && defined(CHECK_LEAKS)
    memStats();
    checkLeak();
#endif
    return code;
};



//
//
//      SablotSetBase
//
//

int SablotSetBase(void* processor_, 
                  const char *theBase)
{
    P(processor_) -> setHardBaseURI(theBase);
    return OK;
}


//
//
//      SablotSetBaseForScheme
//
//

int SablotSetBaseForScheme(void* processor_, 
                  const char *scheme, const char *base)
{
    P(processor_) -> addBaseURIMapping(scheme, base);
    return OK;
}




//
//
//      SablotGetResultArg
//
//

int SablotGetResultArg(void *processor_,
                       const char *argURI,
                       char **argValue)
{
    char *newCopy; // GP: OK
    // we even obtain the index of the output buffer but won't use it
    int resultNdx_;
    if (argValue)
    {
      Situation *s = P(processor_) -> recallSituation();
      sabassert(s); //we always should have some situation, master or internal
      P(processor_) -> copyArg(SIT(s), argURI, &resultNdx_, newCopy);
      // if output does not go to a buffer, newCopy is NULL
      *argValue = newCopy;
    }
    return OK;
}


//
//
//      SablotSetLog
//
//

int SablotSetLog(void *processor_,
    const char *logFilename, int logLevel)
{
    P(processor_) -> recallSituation()->msgOutputFile((char *)"/__stderr",(char*) logFilename);
    // logLevel ignored so far
    return OK;
}


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

int SablotProcess(const char *sheetURI, const char *inputURI, 
		  const char *resultURI,
		  const char **params, const char **arguments, 
		  char **resultArg)
{
    void *theproc;
    EE( SablotCreateProcessor (&theproc) );
    EE_D( SablotRunProcessor(theproc,
        sheetURI, inputURI, resultURI,
        params, arguments), theproc );
    EE_D( SablotGetResultArg(theproc, resultURI, resultArg), theproc );
    EE( SablotDestroyProcessor(theproc) );
    return OK;
};


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


int SablotProcessFiles(const char *styleSheetName,
                       const char *inputName,
                       const char *resultName)
{
    return SablotProcess(
        styleSheetName, inputName, resultName, 
        NULL, NULL, NULL);
};


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
int SablotProcessStrings(const char *styleSheetStr,
                         const char *inputStr,
                         char **resultStr)
{
  const char *argums[] =
  {
    "/_stylesheet", styleSheetStr,
    "/_xmlinput", inputStr,
    "/_output", NULL, //was: *resultStr,
    NULL
  };
  return SablotProcess("arg:/_stylesheet",
                       "arg:/_xmlinput",
                       "arg:/_output",
                       NULL, argums, resultStr);
};


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

int SablotProcessStringsWithBase(const char *styleSheetStr,
                         const char *inputStr,
                         char **resultStr,
                         const char *theHardBase)
{
    const char *argums[] =
    {
        "/_stylesheet", styleSheetStr,
        "/_xmlinput", inputStr,
        "/_output", NULL, //was: *resultStr,
        NULL
    };
    void *theproc;
    EE( SablotCreateProcessor(&theproc));
    EE_D( SablotSetBase(theproc, theHardBase), theproc );
    EE_D( SablotRunProcessor(theproc,
        "arg:/_stylesheet",
        "arg:/_xmlinput",
        "arg:/_output",
        NULL, argums), theproc);
    EE_D( SablotGetResultArg(theproc,
        "arg:/_output", resultStr), theproc);
    EE( SablotDestroyProcessor(theproc));
    return OK;
};


/*****************************************************************
SablotFree

  Frees the Sablotron-allocated buffer.
*****************************************************************/

int SablotFree(char *resultStr)
{
    if (resultStr)
        delete[] resultStr;
    return OK;
}


//
//
//      SablotFreeResultArgs
//      frees the result arg buffers after the have been obtained
//      via SablotGetResultArg()
//
//

int SablotFreeResultArgs(void *processor_)
{
    EE( P(processor_) -> freeResultArgs(*(P(processor_) -> recallSituation())) );
    return OK;
}


//
//
//      SablotRegHandler
//
//

int SablotRegHandler(void *processor_, 
                     /* HLR_MESSAGE, HLR_SCHEME, HLR_SAX, HLR_MISC */
                     HandlerType type,   
                     void *handler, 
                     void *userData)
{
    Sit S = *(P(processor_) -> recallSituation());
    EE( P(processor_) -> setHandler(S, type, handler, userData) );
    if (type == HLR_MESSAGE)
        EE( S.closeFiles() );
    return OK;
}



//
//
//      SablotUnregHandler
//
//

int SablotUnregHandler(void *processor_, 
                       HandlerType type,   /* HLR_MESSAGE, HLR_SCHEME, HLR_SAX */
                       void *handler,
                       void *userData)
{
    Sit S = *(P(processor_) -> recallSituation());
    EE( P(processor_) -> setHandler(S, type, NULL, NULL) );
    if (type == HLR_MESSAGE)
        EE( S.openDefaultFiles() );
    return OK;
}


//
//
//      SablotGetMsgText
//
//

const char* SablotGetMsgText(int code)
{
    return GetMessage((MsgCode) code) -> text;
}

//
//
//      SablotClearError
//
//

int SablotClearError(void *processor_)
{
    P(processor_) -> recallSituation() -> clear();
    return OK;
}

//
//
//      SablotSetInstanceData
//
//

void SablotSetInstanceData(void *processor_, void *idata)
{
    P(processor_) -> setInstanceData(idata);
}

//
//
//      SablotGetInstanceData
//
//

void* SablotGetInstanceData(void *processor_)
{
    return P(processor_) -> getInstanceData();
}

//
//
//      SablotSetEncoding
//      sets the output encoding to be used regardless of the encoding
//      specified by the stylesheet
//      To unset, call with encoding_ NULL.
//
//

void SablotSetEncoding(void *processor_, char *encoding_)
{
    P(processor_) -> setHardEncoding(
        encoding_ ? encoding_ : (const char*) "");
}
