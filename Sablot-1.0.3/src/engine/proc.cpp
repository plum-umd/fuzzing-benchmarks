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
#include "proc.h"
#include "expr.h"
#include "tree.h"
#include "context.h"
#include "vars.h"
// #include "xmlparse.h" - moved to parser.h
#include "parser.h"
#include "guard.h"
#include "key.h"
#include "domprovider.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef SABLOT_DEBUGGER
#include "debugger.h"
#endif

// GP: clean

/*****************************************************************
R u l e I t e m   methods
*****************************************************************/

RuleItem::RuleItem(XSLElement *arule,double aprio, QName &_name,
                   QName *_mode)
:
rule(arule), priority(aprio), name(_name), mode(_mode)
{
  match = rule -> atts.find(XSLA_MATCH);
};


//
//  ~RuleItem
//  has to dispose of the mode
//
RuleItem::~RuleItem()
{
    cdelete(mode);
}


/*****************************************************************
R u l e S L i s t   methods
*****************************************************************/

RuleSList::RuleSList()
:
SList<RuleItem*>(LIST_SIZE_LARGE)
{};

RuleSList::~RuleSList()
{
    freeall(FALSE);
}

int RuleSList::compare(int first, int second, void *data)
{
    return fcomp((*this)[first] -> priority, (*this)[second] -> priority);
};


XSLElement* RuleSList::findByName(const Tree& t, const QName &what) const
{
    int theNumber = number();
    for (int i=0; i < theNumber; i++)
        if (t.cmpQNames((*this)[i] -> name, what))
            return (*this)[i] -> rule;
    return NULL;
}

/*****************************************************************

  DataLineItem

*****************************************************************/

DataLineItem::DataLineItem(Sit S_)
: situation(S_)
{
}

DataLineItem::~DataLineItem()
{
    // the following close should be checked for error!!
    if (_dataline && _dataline -> mode != DLMODE_CLOSED)
        _dataline -> close(situation);
    cdelete(_dataline);
    if (!_preparsedTree)
        cdelete(_tree);
}

/*****************************************************************

  DataLinesList

*****************************************************************/

int DataLinesList::findNum(Str &absoluteURI, Bool _isXSL,
    DLAccessMode _mode)
{
    int theNumber = number();
    for (int i = 0; i < theNumber; i++)
    {
        DataLineItem* item = (*this)[i];
        if ((item->_dataline -> fullUri == absoluteURI) &&
            (item->_isXSL == _isXSL) && 
            (item-> _dataline -> mode == _mode || 
	         item-> _dataline -> mode == DLMODE_CLOSED))
        return i;
    }
    return -1;
}

Tree* DataLinesList::getTree(Str &absoluteURI, Bool _isXSL, 
    DLAccessMode _mode)
{
    int n;
    if ((n = findNum(absoluteURI, _isXSL, _mode)) != -1)
        return (*this)[n] -> _tree;
    else
        return NULL;
}

eFlag DataLinesList::addLine(Sit S, DataLine *d, Tree *t, Bool isXSL,
    Bool preparsedTree /* = FALSE */)
{
    DataLineItem *item = new DataLineItem(S);
    item -> _dataline = d;
    item -> _tree = t;
    item -> _isXSL = isXSL;
    item -> _preparsedTree = preparsedTree;
    append(item);
    return OK;
}

/*****************************************************************
*                                                                *
*   P r o c e s s o r   methods                                  *
*                                                                *
*****************************************************************/

// disable the MSVC warning "'this' used in the initializer list"

#ifdef WIN32
#pragma warning( disable : 4355 )
#endif

Processor::Processor() 
: 
  theArena(PROC_ARENA_SIZE)
#ifdef ENABLE_JS
  , jscontexts(this)
#endif

#ifdef WIN32
#pragma warning( default : 4355 )
#endif

{
//    pushNewHandler();
    theSchemeHandler = NULL;
    theMessageHandler = NULL;
    theSAXHandler = NULL;
    theMiscHandler = NULL;
    theEncHandler = NULL;
    theSchemeUserData = theMessageUserData = 
        theSAXUserData = theMiscUserData = theEncUserData = NULL;    
    instanceData = NULL;
    instanceSituation = NULL;
    addedFlag = FALSE;
    vars = NULL;
    input = styleSheet = NULL;
    keys = NULL;
    nsUnique = 1;
    hasMasterSituation = FALSE;
    runsOnExternal = FALSE;
};

void Processor::prepareForRun()
{
    // do this in open():
    // vars = new VarsList();
    input = styleSheet = NULL;
    decimals().initialize();
}

void Processor::freeNonArgDatalines()
{
    int i = 0;
    while(i < datalines.number())
    {
        if (datalines[i] -> _dataline -> scheme != URI_ARG)
	    {
            datalines.freerm(i, FALSE);
	    }
        else
        {
            // removing the tree here because the arena gets disposed
            // in cleanupAfterRun(). So only the arg buffers remain.
	        // only remove the tree if not preparsed
		    if (!datalines[i] -> _preparsedTree)
                cdelete(datalines[i] -> _tree);
	        // don't move the following line out of the else!
            i++;        	
        };
    }
    addedFlag = FALSE;
}

void Processor::cleanupAfterRun(Situation *Sp)
{
/*
These deletes are now subsumed in "datalines.freeall()":
    cdelete(input);
    cdelete(result);
    cdelete(styleSheet);
*/

  styleSheet = input = NULL;
    cdelete(vars);
    cdelete(keys);
    decimals().freeall(FALSE);
#ifdef ENABLE_JS
    jscontexts.freeall(FALSE);
#endif

    outputDocuments.freeall(FALSE);
    outputDocumentURIs.freeall(FALSE);

    freeNonArgDatalines();
    if (Sp && !Sp -> isError())
      {
        sabassert(modes.isEmpty());
        sabassert(outputters_.isEmpty());
      }
    else
      {
        modes.freeall(FALSE);
        outputters_.freeall(FALSE);
      };
    if (Sp)
	Sp -> clear();
    // hope this works:
    //DETACH: move to Tree
    // dictionary.destroy();
    theArena.dispose();
    runsOnExternal = FALSE;
}

Processor::~Processor()
{
//    popNewHandler();
    baseURIMappings.freeall(FALSE);
    // FIXME: MUST clear theRecoder but got no situation
//    theRecoder.clear();
};

eFlag Processor::open(Sit S, const char *sheetURI, const char *inputURI)
{
    Str temp;
    DStr theBase;

    my_getcwd(theBase);
    theBase = findBaseURI(S, Str("file://") + theBase);

    E( readTreeFromURI(S, styleSheet, temp = sheetURI, theBase, TRUE) );
    //styleSheet -> speakDebug();
    if ( S.hasFlag(SAB_DUMP_SHEET_STRUCTURE) )
      styleSheet -> dumpStructure(S);

#ifdef SABLOT_DEBUGGER
    if (debugger) debugger -> setBreakpoints(S, styleSheet);
#endif

    runsOnExternal = !inputURI;
    if (inputURI)
      E( readTreeFromURI(S, input, temp = inputURI, theBase, FALSE) );
    vars = new VarsList (*styleSheet);
    keys = new KeySet;
    return OK;
}

eFlag Processor::run(Sit S, const char* resultURI, NodeHandle doc /*= NULL */)
{
    Str temp;
    DStr theBase;
    my_getcwd(theBase);
    theBase = findBaseURI(S, Str("file://") + theBase);

    //strip spaces from the document
    if (input) E( stripTree(S, *input) )

    Log1(S, L1_EXECUTING, styleSheet -> getURI());
    double time_was = getMillisecs();
    E( pushOutputterForURI(S, temp = resultURI, theBase) );
    E( outputter() -> eventBeginOutput(S) );

    inputRoot = !nhNull(doc) ? doc : (NodeHandle)&(NZ(input) -> getRoot());
    GP( Context ) c = new Context(inputRoot);
    (*c).set(inputRoot);

    /*
        in vars, global prebindings go to call level 0,
        global bindings to call level 1
    */
    vars -> startCall();
    // start by resolving all globals
    //E( resolveGlobals(S, c) ); It has been done twice (XSLElement::execute)
    E( styleSheet -> getRoot().execute(S, c, FALSE) );
    vars -> endCall();
    c.del();

    E( outputter() -> eventTrailingNewline(S) );
    E( outputter() -> eventEndOutput(S) );

    //done all outputetrs
    //E( outputDocuments.finish(S) );
        
    // report info about the output document to the MiscHandler
    void *miscUserData;
    OutputDefinition *outDef = &(styleSheet -> outputDef);
    MiscHandler *miscHlr = getMiscHandler(&miscUserData);
    if (miscHlr)
        miscHlr -> documentInfo(
            miscUserData,
            this,   // processor
            outDef -> getValueStr(XSLA_MEDIA_TYPE), 
            outDef -> getValueStr(XSLA_ENCODING));

    E( popOutputter(S) );
    
    //check statuses
    //this is not needed, 
    //cleanupAfterRun takes care in the case of error
    //sabassert(!outputters_.number());

    Log1(S, L1_EXECUTION_DONE, getMillisecsDiff(time_was));
    return OK;
}



/*================================================================
getVarBinding 
    finds the current binding for the variable with the given name.
    The binding is an expression. If none is present, returns NULL.
================================================================*/

Expression* Processor::getVarBinding(QName &which)
{
    return vars -> getBinding(which);
}

/*================================================================
execute
both flavours DEALLOCATE the context and set the pointer to NULL
================================================================*/

eFlag Processor::execute(Sit S, Vertex *IP, Context *&c, Bool resolvingGlobals)
{
    while (!c -> isFinished())
    {
      c -> setCurrentNode(c -> current());
        if (IP) 
	  {
            E( IP -> execute(S, c, resolvingGlobals) );
	  }
        else
	  {
            E( execApplyTemplates(S, c, resolvingGlobals) );
	  }
        c -> shift();
    };
    cdelete(c);
    return OK;
}

eFlag Processor::execute(Sit S, VertexList &IPlist, Context *&c, Bool resolvingGlobals)
{
    // we may need to remove some variable bindings on each pass
    // through a for loop
    Vertex *theParent = IPlist.number()? IPlist[0] -> parent : NULL;
    XSLElement *theForParent;
    if (theParent && isXSLElement(theParent) && toX(theParent) -> op == XSL_FOR_EACH)
        theForParent = toX(theParent);
    else
        theForParent = NULL;
    
    while (c -> current())
    {
        c -> setCurrentNode(c -> current());
        E( IPlist.execute(S, c, resolvingGlobals) );
        c -> shift();
        if (theForParent)
            theForParent -> removeBindings(S);
    };
    cdelete(c);
    return OK;
}

eFlag Processor::execApplyTemplates(Sit S, Context *c, Bool resolvingGlobals)
{
    XSLElement *rule;
    E( NZ(styleSheet) -> findBestRule(S, rule, c, getCurrentMode(), FALSE) );
    if (!rule)
        E( builtinRule(S, c, resolvingGlobals) )
    else
        E( rule -> execute(S, c, resolvingGlobals) );
    return OK;
}

eFlag Processor::execApplyImports(Sit S, Context *c, SubtreeInfo *subtree,
				  Bool resolvingGlobals)
{
    XSLElement *rule;
    E( NZ(styleSheet) -> 
       findBestRule(S, rule, c, getCurrentMode(), TRUE, subtree) );
    if (rule)
        E( rule -> execute(S, c, FALSE) );
    return OK;
}

eFlag Processor::builtinRule(Sit S, Context *c, Bool resolvingGlobals)
{
  // may assume v to be a physical vertex
  //Vertex *v = toPhysical(c -> current());
  NodeHandle v = c -> current();
    
  switch (S.dom().getNodeType(v)) {
  case DOCUMENT_NODE:
  case ELEMENT_NODE:
    //apply-templates
    {
      GP( Expression ) e = new Expression(styleSheet->getRoot(), EXF_LOCPATH);
      (*e).setLS(AXIS_CHILD, EXNODE_NODE);
      GP( Context ) newc;
      newc.assign(c);
      E( (*e).createContext(S, newc) );
      newc.unkeep();
      E( execute(S, NULL, newc, resolvingGlobals) );
      newc.keep(); // GP: it's NULL by this time anyway
      e.del();
    }; break;
  case TEXT_NODE:
  case ATTRIBUTE_NODE:
    //copy contents to output
    {
      const char *val = S.dom().getNodeValue(v);
      E( outputter() -> eventData(S, Str(val)) );
      S.dom().freeValue(v, (char*)val);
    }; break;
  case PROCESSING_INSTRUCTION_NODE:
  case COMMENT_NODE:
    {};
  };

  /*
  switch(v -> vt & VT_BASE)
  {
  case VT_ROOT:
  case VT_ELEMENT:
    //apply-templates
    {
      GP( Expression ) e = new Expression(*toE(v), EXF_LOCPATH);
      (*e).setLS(AXIS_CHILD, EXNODE_NODE);
      GP( Context ) newc;
      newc.assign(c);
      E( (*e).createContext(S, newc) );
      newc.unkeep();
      E( execute(S, NULL, newc, resolvingGlobals) );
      newc.keep(); // GP: it's NULL by this time anyway
      e.del();
    }; break;
    case VT_TEXT:
  case VT_ATTRIBUTE:
    //copy contents to output
    {
      DStr temp;
      E( v -> value(S, temp, c) );
      E( outputter() -> eventData(S, temp) );
    }; break;
  case VT_PI:
  case VT_COMMENT:
    {};
  };
  */
  return OK;
};


/*................................................................
getCurrentMode()
RETURN: pointer to the current mode which is a QName
................................................................*/

QName* Processor::getCurrentMode()
{
    return (modes.number() ? modes.last() : NULL);
}

/*................................................................
pushMode()
called before every mode-changing apply-templates
ARGS:
m       the mode to be placed on top of the mode stack
................................................................*/

void Processor::pushMode(QName *m)
{
    modes.append(m);
}

/*................................................................
popMode()
called after every mode-changing apply-templates
removes the current mode from the mode stack FREEING it
................................................................*/

void Processor::popMode()
{
    modes.freelast(FALSE);
}

eFlag Processor::addLineNoTree(Sit S, DataLine *&d, Str &absolute, Bool isXSL)
{
    GP( DataLine ) d_;
    M( S, d_ = new DataLine );
    E( (*d_).open(S, absolute, DLMODE_READ, &argList) );
    E( datalines.addLine(S, d_, NULL, isXSL) );
    d = d_.keep();
    return OK;
}

eFlag Processor::addLineTreeOnly(Sit S, DataLine *&d, Str &absolute, Bool isXSL,
    Tree *t)
{
    GP( DataLine ) d_;
    M( S, d_ = new DataLine );
    E( (*d_).setURIAndClose(S, absolute) );
    E( datalines.addLine(S, d_, t, isXSL, /* preparsedTree = */ TRUE) );
    d = d_.keep();
    return OK;
}

eFlag Processor::addLineParse(Sit S, Tree *& newTree, Str &absolute, 
			      Bool isXSL, Bool ignoreErr /* = FALSE */)
{
    GP( DataLine ) d = new DataLine;
    E( (*d).open(S, absolute, DLMODE_READ, &argList, ignoreErr) );
    GP( Tree ) newTree_ = new Tree(absolute, isXSL );
    //E( (*newTree_).parse(S, d) );
    eFlag status = (*newTree_).parse(S, d);
    E( (*d).close(S) );
    //hold parse errors after dataline is closed
    E( status );
    E( datalines.addLine(S, d.keep(), newTree = newTree_.keep(), isXSL) );
    return OK;
}

const Str& Processor::findBaseURI(Sit S, const Str& unmappedBase)
{
    Str scheme, rest;
    uri2SchemePath(S, unmappedBase, scheme, rest);
    Str *mapping = baseURIMappings.find(scheme);
    if (mapping) return *mapping;
    mapping = baseURIMappings.find(""/**theEmptyString*/);
    if (mapping) return *mapping;
    return unmappedBase;
}

//
//          Processor::baseForVertex
//
//  returns the *translated* base URI in effect for a given vertex
//

const Str& Processor::baseForVertex(Sit S, Element *v)
{
    return findBaseURI(S, NZ(v) -> getSubtreeInfo() -> getBaseURI());
}


//
//          Processor::readTreeFromURI
//
//  reads in a tree from URI 'location' resolved against 'base' which is 
//  a *translated* base (one for which the 'hard base URIs' have been taken 
//  into account)
//

eFlag Processor::readTreeFromURI(Sit S, Tree*& newTree, const Str& location, 
				 const Str& base, Bool isXSL, 
				 Bool ignoreErr /* = FALSE */)
{
    Str
        absolute;
    makeAbsoluteURI(S, location, base, absolute);
    newTree = datalines.getTree(absolute, isXSL, DLMODE_READ);
    if (!newTree)
        E( addLineParse(S, newTree, absolute, isXSL, ignoreErr) );
    return OK;
}

eFlag Processor::createOutputterForURI(Sit S, //Str& location, Str& base,
				       Str& absolute,
				       OutputterObj*& result,
				       OutputDefinition *outDef /*=null*/)
{
  //Str absolute;
  //makeAbsoluteURI(S, location, base, absolute);
  if (datalines.getTree(absolute, FALSE, DLMODE_WRITE))
    Err1(S, E1_URI_WRITE, absolute);
  GP( DataLine ) d;
  if ( !(absolute == "arg:/null") ) 
    { 
      M( S, d = new DataLine );
      E( (*d).open(S, absolute, DLMODE_WRITE, &argList) );
      // FIXME: the NULL tree in the following
      E( datalines.addLine(S, d.keep(), NULL, FALSE) );
    } else { //physical output not required
      d = NULL;
    };
  GP( OutputterObj ) newOut;
  M( S, newOut = new OutputterObj );
  
  if (!outDef) outDef = &(styleSheet -> outputDef);
  E( (*newOut).setOptions(S, d, outDef) );
  if (theSAXHandler)
    E( (*newOut).setOptionsSAX(S, theSAXHandler, theSAXUserData, 
			       SAXOUTPUT_AS_PHYSICAL) );
  
  result = newOut.keep();
  
  return OK;
}

eFlag Processor::getOutputDocument(Sit S, Str& href,
				      OutputDocument *&resultDoc, 
				      OutputDefinition *def)
{
  //OutputDocument * aux = outputDocuments.find(def -> 
  
  //GP(OutputDocument) doc = new OutputDocument(newOut, def);
  GP(OutputDocument) doc = new OutputDocument(href, def);

  resultDoc = doc.keep();
  outputDocuments.append(resultDoc);

  return OK;
}

eFlag Processor::startDocument(Sit S, OutputDocument *doc)
{
  switch (doc -> getState()) {
  case OUTDOC_ACTIVE:
    {
      sabassert(!"rewrite document");
      //printf("---> aaa\n");
      //OutputterObj *out = NZ( doc -> getOutputter());
      //E( pushOutputter(S, out) );
    }; break;
  case OUTDOC_NEW:
    {
      ////////////////////////////////////////
      //create the outputter
      Str &href = doc -> getHref();
      DStr base = "";
      OutputterObj *current = outputter();
      PhysicalOutputLayerObj *phys;
      if (current && (phys = current->getPhysical()) && phys->getDataLine())
	{
	  base = phys -> getDataLine() -> fullUri;
	}
      if (base == "" || base == "file:///__stdout" || base == "file:///__stderr")
	{
	  DStr cwd;
	  my_getcwd(cwd);
	  base = "file://";
	  base += cwd;
	  S.message(MT_LOG, L2_SUBDOC_BASE, href, base);
	}
      S.message(MT_LOG, L2_SUBDOC, href, base);

      OutputterObj *newOut;
      Str absolute;
      makeAbsoluteURI(S, href, base, absolute);

      //test if the document was not output recently
      if ( outputDocumentURIs.findIdx(absolute) != -1 ) {
	Err1( S, E_DUPLICIT_OUTDOC, absolute );
      } else {
	doc -> setURI(absolute);
	outputDocumentURIs.insert(new Str(absolute));
      }

      E( createOutputterForURI(S, absolute, newOut, doc->getDefinition()) );
      E( pushOutputter(S, doc -> setOutputter(newOut)) );
      E( outputter() -> eventBeginOutput(S) );

      doc -> setState(OUTDOC_ACTIVE);
    }; break;
  case OUTDOC_FINISHED:
    sabassert(! "Couldn't write the document twice"); break;
  }
  return OK;
}

eFlag Processor::finishDocument(Sit S, OutputDocument *doc, Bool canClose)
{
  switch (doc -> getState()) {
  case OUTDOC_NEW:
  case OUTDOC_FINISHED:
    sabassert(!"Could not finish unopened/finished document"); break;
  case OUTDOC_ACTIVE:
    {
      E( doc -> finish(S) );
      E( popOutputter(S) );
    }; break;
  }
  return OK;
}

eFlag Processor::pushOutputterForURI(Sit S, Str& location, Str& base, 
				     OutputDefinition *outDef /*=NULL*/)
{
  OutputterObj *newOut;

  Str absolute;
  makeAbsoluteURI(S, location, base, absolute);
  E( createOutputterForURI(S, absolute, newOut, outDef) );

  outputters_.append(newOut);
  return OK;
}

eFlag Processor::pushTreeConstructer(Sit S, TreeConstructer *& newTC, Tree *t,
				     SAXOutputType ot)
{
    newTC = NULL;
    GP( TreeConstructer ) newTC_ = new TreeConstructer(S);
    GP( OutputterObj ) newTCSource = new OutputterObj;
    //E( (*newTCSource).setOptions(S, NULL, &(styleSheet -> outputDef)) ); //_PH_
    M( S, newTC_+0 );
    outputters_.append(newTCSource);
    E( (*newTC_).parseUsingSAX(S, t, *newTCSource, ot) );
    newTCSource.keep();
    newTC = newTC_.keep();
    return OK;
}

eFlag Processor::pushOutputter(Sit S, OutputterObj* out_)
{
    outputters_.append(out_);
    return OK;
}

eFlag Processor::popOutputter(Sit S)
{
    outputters_.freelast(FALSE);
    return OK;
}

eFlag Processor::popOutputterNoFree(Sit S)
{
    outputters_.deppend();
    return OK;
}

eFlag Processor::popTreeConstructer(Sit S, TreeConstructer *theTC)
{
    popOutputter(S);
    delete theTC;
    return OK;
}

/*
eFlag Processor::parse(Sit S, Tree *t, DataLine *d)
{
    Log1(S, L1_PARSING, t -> name);
    double time_was = getMillisecs();
    TreeConstructer tc(S);
    eFlag retval = tc.parseDataLineUsingExpat(S, t, d);
    if (!retval)
    {
        Log1(S, L1_PARSE_DONE, getMillisecsDiff(time_was));
    }
    return retval;
}
*/

eFlag Processor::getArg(Sit S, const char* name, char*& buffer, Bool isUTF16)
{
    Str temp, *value = argList.find(temp = (char*)name);
    if (!value)
        Err1(S, E1_ARG_NOT_FOUND,(char*) name);
    buffer = (char*) *value;
    return OK;
}

//
//
//
//      plugin handler stuff
//
//
//

eFlag Processor::setHandler(Sit S, HandlerType type, void *handler, void *userData)
{
    void **whereHandler, **whereUserData;
    switch(type)
    {
    case HLR_SCHEME: 
        {
            whereHandler = (void **)&theSchemeHandler;
            whereUserData = &theSchemeUserData;
        }; break;
    case HLR_MESSAGE: 
        {
            whereHandler = (void **)&theMessageHandler;
            whereUserData = &theMessageUserData;
        }; break;
    case HLR_SAX: 
        {
            whereHandler = (void **)&theSAXHandler;
            whereUserData = &theSAXUserData;
        }; break;
    case HLR_MISC: 
        {
            whereHandler = (void **)&theMiscHandler;
            whereUserData = &theMiscUserData;
        }; break;
    case HLR_ENC:
        {
            whereHandler = (void **)&theEncHandler;
            whereUserData = &theEncUserData;
        }; break;
    default: 
        Err1(S, E1_INVALID_HLR_TYPE, (int) type);
    }
    if (*whereHandler)
    {
        if (handler)
            Warn1(S, W1_HLR_REGISTERED, hlrTypeNames[type])
        else
        {
            *whereHandler = NULL;
            *whereUserData = NULL;
        }
    }
    else
    {
        if (handler)
        {
            *whereHandler = handler;
            *whereUserData = userData;
        }
        else
            Warn1(S, W1_HLR_NOT_REGISTERED, hlrTypeNames[type])
    }
    return OK;
}


SchemeHandler* Processor::getSchemeHandler(void **udata)
{
    if (udata)
        *udata = theSchemeUserData;
    return theSchemeHandler;
}

MessageHandler* Processor::getMessageHandler(void **udata)
{
    if (udata)
        *udata = theMessageUserData;
    return theMessageHandler;
}

SAXHandler* Processor::getSAXHandler(void **udata)
{
    if (udata)
        *udata = theSAXUserData;
    return theSAXHandler;
}

MiscHandler* Processor::getMiscHandler(void **udata)
{
    if (udata)
        *udata = theMiscUserData;
    return theMiscHandler;
}

EncHandler* Processor::getEncHandler(void **udata)
{
    if (udata)
        *udata = theEncUserData;
    return theEncHandler;
}

void* Processor::getHandlerUserData(HandlerType type, void *handler)
{
    switch(type)
    {
    case HLR_SCHEME: return theSchemeUserData;
    case HLR_MESSAGE: return theMessageUserData;
    case HLR_MISC: return theMiscUserData;
    default: return theSAXUserData;
    }
}

void Processor::setHardEncoding(const Str& hardEncoding_)
{
    hardEncoding = hardEncoding_;
}

const Str& Processor::getHardEncoding() const
{
    return hardEncoding;
};

/*****************************************************************
copyArg
  called if the result document's location uses the arg:
  scheme. Returns information about the associated named buffer.
  if not found, returns -1 in argOrdinal and NULL in newCopy.
ARGS
  argName       the name of the arg
RETURNS
  *argOrdinal    the ordinal number of this arg. This is the number
                of the call to useArg() which defined the arg.
  newCopy       pointer to a new copy of the arg (allocated via
                malloc())
*****************************************************************/

void Processor::copyArg(Sit S, const Str& argName, int* argOrdinal,
    char*& newCopy)
{
    Str absolute;
    int lineNo;
    if ((makeAbsoluteURI(S, (Str&)argName, "arg:/", absolute) != URI_ARG)
        || (lineNo = datalines.findNum(absolute, FALSE, DLMODE_WRITE)) == -1)
    {
        newCopy = NULL;
        *argOrdinal = -1;
        return;
    }
    DynBlock *block = NZ( datalines[lineNo] -> _dataline -> getOutBuffer() );
    newCopy = block -> compactToBuffer(); // GP: OK (no exit route)

    //  set *argOrdinal
    *argOrdinal = argList.findNum((char *)absolute + 4);    // skip 'arg:'

}

eFlag Processor::useArg(Sit S, const char *name, const char *val)
{
    sabassert(name);
    DStr nameStr;
    if (*name != '/')
        nameStr = "/";
    nameStr += name;
    if (argList.find(nameStr))
        Err1(S, E1_DUPLICATE_ARG, nameStr);
    StrStr *p = new StrStr;
    p -> key = nameStr;
    if (val)
        p -> value = val;
    else
        p -> value.empty();
    argList.append(p);
    addedFlag = TRUE;

    return OK;
}

eFlag Processor::useTree(Sit S, const char *name, Tree *t)
{
  sabassert(name);
  DStr nameStr;
  if (*name != '/')
    nameStr = "/";
  nameStr += name;
  // to check for duplicate trees
  E( useArg(S, name, NULL) );
  
  Str absolute;
  DataLine *d;
  makeAbsoluteURI(S, nameStr, "arg:/", absolute);
  E( addLineTreeOnly(S, d, absolute, t -> XSLTree, t) );
  addedFlag = TRUE;
  return OK;
}

eFlag Processor::addGlobalParam(Sit S, const char *name, const char *val)
{
    sabassert(name);
    if (!val) val = (char*)"";
    globalParamsList.appendConstruct(name, val);
    return OK;
}

eFlag Processor::useGlobalParam(Sit S, const char *name, const char *val)
{
    sabassert(name);
    QName q;
    q.setLocal(NZ(styleSheet) -> unexpand(name));
    Expression *expr = new Expression(styleSheet -> getRoot(), EXF_ATOM);
    Str aux = val;
    expr -> setAtom(aux);
    vars -> addPrebinding(S, q, expr);
    return OK;
}

eFlag Processor::useGlobalParams(Sit S)
{
    while (globalParamsList.number())
    {
        StrStr& item = *(globalParamsList.last());
        E( useGlobalParam(S, item.key, item.value) );
	    globalParamsList.freelast(FALSE);	    
    }
    return OK;
}

void Processor::setHardBaseURI(const char* hardBase)
{
  addBaseURIMapping(""/**theEmptyString*/, (const Str) hardBase);
}

void Processor::addBaseURIMapping(const Str& scheme, const Str& mapping)
{
    int ndx = baseURIMappings.findNum(scheme);
    if (ndx != -1)
        baseURIMappings.freerm(ndx, FALSE);
    if (!mapping.isEmpty())
        baseURIMappings.appendConstruct(scheme, mapping);
}

eFlag Processor::freeResultArgs(Sit S)
{
    datalines.freeall(FALSE);
    argList.freeall(FALSE);
    addedFlag = FALSE;
    return OK;
}

SabArena* Processor::getArena() 
{
    return &theArena;
}

eFlag Processor::prefixIsAliasTarget(Sit S, const Str& prefix, Bool& result)
{
  result = FALSE;
  //we may enter during the tree copy as well
  if (styleSheet)
    {
      Phrase p = NZ(styleSheet) -> unexpand(prefix);
      for (int i = 0; i < styleSheet -> aliases().number(); i++)
	{
	  if (styleSheet -> aliases()[i] -> getValue() == p)
	    {
	      result = TRUE;
	      break;
	    }
	}
    }
  return OK;
}

void Processor::getAliasedName(EQName & name, Bool & aliased)
{
  Str myUri = name.getUri();
  for (int i = 0; i < styleSheet -> aliases().number(); i++)
    {
      const Str & aliasUri = 
	styleSheet -> expand(styleSheet -> aliases()[i]->getKey());
      if (aliasUri && aliasUri == myUri)
	{
	  Phrase newUri = styleSheet -> aliases()[i] -> getValue();
	  Phrase newPrefix = styleSheet -> aliases()[i] -> getPrefix();
	  name.setUri(styleSheet -> expand(newUri));
	  //name.setPrefix(styleSheet -> expand(newPrefix));
	  aliased = true;
	  break;
	};	        
    }
}

/*
Str Processor::getAliasedName(const EQName& name, 
			      NamespaceStack& currNamespaces,
			      Bool expatLike)
{
   DStr s;
   Str newPrefixStr;
   Str newUri;
   //Bool defaultNS = !name.hasPrefix();

   if (1 || name.hasPrefix())
   {
      Phrase newPrefix = UNDEF_PHRASE;
      Str myUri = name.getUri();
      int i;
      Bool aliasFound = FALSE;
      if (styleSheet)
      {
          for (i = 0; i < styleSheet -> aliases().number(); i++)
          {
	    const Str* aliasUri = 
	      currNamespaces.getUri(styleSheet -> expand(
	                         styleSheet -> aliases()[i]->getKey()));
	    if (aliasUri && *aliasUri == myUri)
              {
		newPrefix = styleSheet -> aliases()[i] -> getValue();
		aliasFound = TRUE;
              };	        
	  }
      };
      
      if (newPrefix == UNDEF_PHRASE)
	{
	  if (! aliasFound && name.hasPrefix()) 
	    newPrefixStr = name.getPrefix();
	} 
      else 
	{
	  newPrefixStr = styleSheet -> expand(newPrefix);
	  //s += newPrefixStr;
	  //we should proceed w/o this 
	  //currNamespaces.appendConstruct(name.getPrefix(),name.getUri(),TRUE);
	  //currNamespaces.appendConstruct(newPrefixStr,
	  // *currNamespaces.getUri(newPrefixStr));
	};
	//if (!s.isEmpty()) //could be empty e.g. for the default namespace
      //  s += ":";
   };
   if (expatLike)
     {
       s  = name.getUri();
       s += THE_NAMESPACE_SEPARATOR;
       s += name.getLocal();
       s += THE_NAMESPACE_SEPARATOR;
       s += newPrefixStr;
     }
   else 
     {
       s = newPrefixStr;
       if (! s.isEmpty()) s += ":";
       s += name.getLocal();
     }
   return s;
}
*/

eFlag Processor::addKey(Sit S, const EQName& ename,
			Expression& match, Expression &use)
{
  
  //E( keys -> addKey(S, ename, toSXP_Document(*input), match, use) );
  E( NZ(keys) -> addKey(S, ename, inputRoot, match, use) );
  return OK;
}

eFlag Processor::getKeyNodes(Sit S, const EQName& ename, 
			     const Str& value, Context& result, 
			     SXP_Document doc) const
{
    E( NZ(keys) -> getNodes(S, ename, doc, value, result) );
    return OK;
}

eFlag Processor::makeKeysForDoc(Sit S, SXP_Document doc)
{ 
    E( NZ(keys) -> makeKeysForDoc(S, doc) );
    return OK;
}


void Processor::report(Sit S, MsgType type, MsgCode code, 
    const Str &arg1, const Str &arg2) const
{
    S.message(type, code, arg1, arg2);
}

void Processor::initForSXP(Tree *baseTree)
{
  input = NULL;
  styleSheet = baseTree;
  runsOnExternal = TRUE;
  if (!vars)
    vars = new VarsList(*styleSheet);
}

void Processor::cleanupAfterSXP()
{
  vars -> freeall(FALSE);
  styleSheet = NULL;
  runsOnExternal = FALSE;
}

Bool Processor::supportsFunction(Str &uri, Str &name) 
{
#ifdef ENABLE_JS
  JSContextItem *item = jscontexts.find(uri, FALSE);  
  if (item) {
    for (int i = 0; i < item -> names.number(); i++) {
      if (*(item -> names[i]) == name) return TRUE;
    }
  }
  return FALSE;
#else
  return FALSE;
#endif
}

#ifdef ENABLE_JS
eFlag Processor::evaluateJavaScript(Sit S, Str &uri, DStr &script) 
{
  //printf("===> evaluating uri %s: %s\n", (char*)uri, (char*)script);

  JSContextItem *item = jscontexts.find(uri, TRUE);
  sabassert(item);
  if (item && item -> cx) {
    if (! SJSEvaluate(*item, script ) )
      {
	Str msg = "'";
	msg = msg + (item -> errInfo.message ? item -> errInfo.message : "");
	if (item -> errInfo.token) {
	  msg = msg + "' token: '";
	  msg = msg + (item -> errInfo.token ? item -> errInfo.token : "");
	}
	msg = msg + "'";
	Err2( S, E_JS_EVAL_ERROR, 
	      Str(item -> errInfo.line), msg );
      }
  }
  
  return OK;
}

eFlag Processor::callJSFunction(Sit S, Context *c, Str &uri, Str &name, 
				  ExprList &atoms, Expression &retxpr)
{
  if (supportsFunction(uri, name)) {
    JSContextItem *item = jscontexts.find(uri, FALSE);
    if (item) {
      if (! SJSCallFunction(S, c, *item, name, atoms, retxpr) )
	{
	  Str msg = "'";
	  msg = msg + (item -> errInfo.message ? item -> errInfo.message : "");
	  if (item -> errInfo.token) {
	    msg = msg + "' token: '";
	    msg = msg + (item -> errInfo.token ? item -> errInfo.token : "");
	  }
	  msg = msg + "'";
	  Err2( S, E_JS_EVAL_ERROR, 
		Str(item -> errInfo.line), msg );
	};
    }
  }

  return OK;
}
#endif

Str Processor::getNextNSPrefix()
{
  char ret[10];
  sprintf(ret, "ns_%d", nsUnique++);
  return Str(ret);
}

eFlag Processor::resolveGlobal(Sit S, Context *c, QName &name, XSLElement *varElem /* = NULL */)    
{ 
  // if !varElem, we need to ask styleSheet's toplevel var directory
  /*
  if (!varElem)
    varElem = styleSheet -> findVarInDirectory(name);
  else
    {
      // set the variable name from the element
      E( varElem -> setLogical(S, name, NZ(varElem -> atts.find(XSLA_NAME)) -> cont, FALSE) );
    }
  */
  
  //we have either the name or the element, if both, the element wins
  if (varElem) 
      E( varElem->setLogical(S, name, 
			     NZ(varElem->atts.find(XSLA_NAME))->cont, FALSE) );
  //this element must be the same as the varElem if available
  XSLElement *aux = styleSheet -> findVarInDirectory(name);
  if (varElem)
    {
      //it should happen only if the caller is XSLElement::execute
      //and we have some overriden (import) declaration
      //in such a case we simply skip the processing
      if (varElem != aux) return OK;
    }
  else
    { 
      varElem = aux;
    }

  if (varElem)
    {
      // find the var record for several references
      VarBindings* record = vars -> find(name);
      // maybe it has a binding defined from a forward reference?
      //but it shouldn't happen now, due the ** condition
      if (record && vars -> getBinding(record))
	return OK;
      else
	{
	  // maybe it is an "open toplevel" (currently processed)
	  if (record && vars -> isOpenGlobal(record))
	    {
	      Str fullName;
	      styleSheet -> expandQStr(name, fullName);
	      Err1(S, E1_VAR_CIRCULAR_REF, fullName);
	    }
	  else
	    {
	      // process it normally
	      E( vars -> openGlobal(S, name, record) );
	      // varElem's expression will automatically evaluate with "resolvingGlobal"
	      E( varElem -> execute(S, c, /* resolvingGlobals = */ TRUE) );
	      E( vars -> closeGlobal(S, record) );		
	    }
	}
    }
  else
    {
      // this is not a global
      Str fullName;
      styleSheet -> expandQStr(name, fullName);
      Err1(S, E1_VAR_NOT_FOUND, fullName);
    }
  return OK;
}

/*
eFlag Processor::resolveGlobals(Sit S, Context *c)
{
     return styleSheet -> resolveGlobals(S, c, this);
}
*/

eFlag Processor::stripElement(Sit S, Daddy *e)
{
  if (isElement(e) && ! toE(e) -> preserveSpace)
    {
      EQName ename;
      e -> getOwner().expandQ(e -> getName(), ename);
      int sprec, pprec;
      double sprio, pprio;
      Bool s = styleSheet -> findStrippedName(ename, sprec, sprio);
      Bool p = styleSheet -> findPreservedName(ename, pprec, pprio);

      if (s && (!p || sprec < pprec || sprio > pprio))
	{
	  e -> contents.strip();
	}
    }

  //process recursively
  for (int i = 0; i < e -> contents.number(); i++)
    {
      if (isElement(e -> contents[i]))
	E( stripElement(S, toE(e -> contents[i])) );
    }
  return OK;
}

eFlag Processor::stripTree(Sit S, Tree &tree)
{
  //strip is forbidden
  if (S.hasFlag(SAB_DISABLE_STRIPPING)) return OK;

  //never strip the tree twice
  if (tree.stripped) return OK;

  //skip if no definition in the stylesheet
  if (!styleSheet -> hasAnyStrippedName() && 
      ! styleSheet -> hasAnyPreservedName())
    return OK;

  //do it
  Daddy &d = tree.getRoot();
  E( stripElement(S, &d) );

  tree.stripped = TRUE;

  return OK;
}

void Processor::pushInBinding(Bool val)
{
  inBinding.append(val);
}

void Processor::popInBinding()
{
  inBinding.deppend();
}

Bool Processor::isInBinding()
{
  return inBinding.number() && inBinding.last();
}

/*
eFlag Processor::pushDocumentDefinition(Sit S, OutputDefinition *def,
					OutputterObj*& out)
{
  sabassert(def);
  Bool change = !documentDefinitions.number() || 
    documentDefinitions.last() != def;

  documentDefinitions.append(def);
  
  if (change)
    {
      Str href = "file:///home/pavel/ga/sablot-ext/bin/inner.xml";
      Str base = "";
      E( pushOutputterForURI(S, href, base, def) );
      E( outputter() -> eventBeginOutput(S) );
    }

  out = outputter();
  return OK;
}

eFlag Processor::popDocumentDefinition(Sit S)
{
  sabassert(documentDefinitions.number());
  OutputDefinition *last = documentDefinitions.last();
  documentDefinitions.deppend();
  //
  if (!documentDefinitions.number() || documentDefinitions.last() != last)
    {
      E( outputter() -> eventTrailingNewline(S) );
      E( outputter() -> eventEndOutput(S) );
      E( popOutputter(S) );
    }
  return OK;
}

*/
