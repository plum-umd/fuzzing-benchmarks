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
 * Contributor(s): Tim Crook <tcrook@accelio.com>
 *                 Christian Lefebvre <christian.lefebvre@atosorigin.com>
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

//  parser.cpp

#include "base.h"
#include "parser.h"
#include "situa.h"
#include "tree.h"
#include "error.h"
#include "proc.h"
#include "encoding.h"
#include "guard.h"

// GP: clean

// 
//
//      TreeConstructer
//
//

SAXHandlerInternal TreeConstructer::myHandlerRecord =
{
    saxStartDocument,
    saxStartElement,
    saxEndElement,
    saxStartNamespace,
    saxEndNamespace,
    saxComment,
    saxPI,
    saxCharacters,
    saxEndDocument,
    saxStartNamespace2
};

TreeConstructer::TreeConstructer(Sit S_)
: S(S_)
{
  theParser = NULL;
  theTree = NULL;
  theDataLine = NULL;
  theLineNumber = 0;
  inSAXForAWhile.append(FALSE);
  theSAXUri.append(new Str("fake_uri"));
  nsCount.append(0);
}

TreeConstructer::~TreeConstructer()
{
  inSAXForAWhile.deppend();
  sabassert(! inSAXForAWhile.number());
  theSAXUri.freelast(FALSE);
  sabassert(! theSAXUri.number());
}


eFlag TreeConstructer::parseDataLineUsingGivenExpat(Sit S, 
    Tree *t, DataLine *d, XML_Parser theParser_)
{
    theTree = t;
    theDataLine = d;
    theParser = theParser_;

    S.setCurrFile(d -> fullUri);
    E( feedDocumentToParser(S, this) );
    E( t -> parseFinished(S) );
    if (t -> XSLTree)
    {
        t -> stripped += t -> getRoot().strip();
	E( t -> aliases().checkRedefinitions(S, *t) );
	E( t -> attSets().checkRedefinitions(S) );
    }
    return OK;
};


eFlag TreeConstructer::parseDataLineUsingExpat(Sit S, 
					       Tree *t, DataLine *d, 
					       char *base_ /*=NULL*/)
{
    theParser = XML_ParserCreateNS(NULL, THE_NAMESPACE_SEPARATOR);

    M( S, theParser );
    // XML_UseParserAsHandlerArg(parser);
    XML_SetElementHandler(theParser, 
        tcStartElement,
        tcEndElement);
    XML_SetCharacterDataHandler(theParser, 
        tcCharacters);
    XML_SetNamespaceDeclHandler(theParser, 
        tcStartNamespace, 
        tcEndNamespace);
    XML_SetCommentHandler(theParser,
        tcComment);
    XML_SetProcessingInstructionHandler(theParser,
        tcPI);
    
    // the unknown encoding handler is no more used:
    // XML_SetUnknownEncodingHandler(theParser, 
    //     tcUnknownEncoding, 
    //     NULL);

    XML_SetExternalEntityRefHandler(theParser,
				    tcExternalEntityRef);
    XML_SetEntityDeclHandler(theParser, tcEntityDecl);


    XML_SetUserData(theParser, this);
    if (S.getProcessor())
      XML_SetBase(theParser, S.getProcessor() -> findBaseURI(S, t ->getURI()));
    else if (base_)
      XML_SetBase(theParser, base_);
    
    // wasn't this done before?
    XML_SetParamEntityParsing(theParser, XML_PARAM_ENTITY_PARSING_ALWAYS);

    //we want to get all info (including prefixes)
#ifdef EXPAT_SUPPORTS_TRIPLETS
    XML_SetReturnNSTriplet(theParser, 1);
#endif

    eFlag eCode = parseDataLineUsingGivenExpat(S, t, d, theParser);

    XML_ParserFree(theParser);
    if( eCode )
	return S.getError();
    return OK;
};

eFlag TreeConstructer::parseUsingSAX(Sit S, Tree *t, OutputterObj& source,
				     SAXOutputType ot)
{
    theTree = t;
    theDataLine = NULL;
    // register the handler with the outputter
    E( source.setOptionsSAX(S, (SAXHandler*)&myHandlerRecord, this, ot) );
    E( source.eventBeginOutput(S) );
    return OK;
}

eFlag TreeConstructer::parseUsingSAXForAWhile(Sit S, OutputterObj& source,
					      Str &saxUri, 
					      Bool resetNamespaces,
					      Tree* srcTree,
					      NSList &swallowNS)
// it's assumed that this is a tree constructer over a dataline
// (not a SAX one)
{
    sabassert(theTree && theDataLine);
    inSAXForAWhile.append(TRUE);
    theSAXUri.append(new Str(saxUri));

    if (resetNamespaces) 
      E( theTree -> pushPendingNS(S, srcTree, swallowNS) );

    // register the handler with the outputter
    E( source.setOptionsSAX(S, (SAXHandler*)&myHandlerRecord, this, SAXOUTPUT_COPY_TREE) );
    E( source.eventBeginOutput(S) );
    return OK;
}

eFlag TreeConstructer::parseUsingSAXForAWhileDone(Sit S, OutputterObj& source,
						  Bool resetNamespaces)
{
    E( source.eventEndOutput(S) );

    inSAXForAWhile.deppend();
    theSAXUri.freelast(FALSE);

    if (resetNamespaces) 
      E( theTree -> popPendingNS(S) );

    return OK;
}

int TreeConstructer::getCurrentLineNumber() const
{
  if (this -> inSAXForAWhile.last())
    {
      return this -> S.getCurrSAXLine();
    }
  else if (theParser)
    {
      return XML_GetCurrentLineNumber(theParser);
    }
  else
    return theLineNumber;
}
  
/* static */
eFlag TreeConstructer::getDocEncoding(Sit S, const char *buf, Str& theEncoding, TreeConstructer *this_)
  // assumes the parse buffer null-terminated
{
  // copy the unsigned shorts out of the string to avoid misalignment
    unsigned short bom_part1, bom_part2;
    memcpy (&bom_part1, buf, sizeof(unsigned short));
    switch (bom_part1)
    {
    case 0xfeff:
    case 0xfffe:
        theEncoding = "UTF-16";
        break;
    case 0x003c:
    case 0x3c00:
        // no byte order mark!
        memcpy (&bom_part2, buf+1, sizeof(unsigned short));
        if ((bom_part2 == 0x003f) ||
            (bom_part2 == 0x3f00))
            theEncoding = "UTF-16";
        else
            this_ -> Warn(S, W_BAD_START);
        break;
    case 0x0000:
        memcpy (&bom_part2, buf+1, sizeof(unsigned short));
        if ((bom_part2 == 0x003c) ||
            (bom_part2 == 0x3c00))
            theEncoding = "ISO-10646-UCS-4";
        else
            this_ -> Warn(S, W_BAD_START);
        break;
    default:
        {
	// look for "<?xml " to distinguish from "<?xml-stylesheet"
	// or any other similar processing instructions
	  Bool found = FALSE;
	  if (buf[0] == '<' && buf[1] == '?' && buf[2] == 'x' 
	      && buf[3] == 'm' && buf[4] == 'l' && buf[5] == ' ')
            {
	      const char *p = buf;
	      p = strpbrk(p + 2,"=?");
	      while (! found && p && *p == '=')
		{
		  //now find the pseudo-attribute name
		  const char *beg = p - 1;
		  while ( isWhite(*beg) ) beg--;
		  while ( (!isWhite(*beg)) && *beg != '?') beg--;
		  if ( strncmp(beg + 1, "encoding", 8) )
		    p = strpbrk(p + 1, "=?");
		  else
		    {
		      p++;
		      skipWhite(p);
		  
		      const char *q = strpbrk(p + 1, "?\'\"");
		      if (q && *q != '?' && *q == *p)
			{
			  theEncoding.nset(p + 1, q - p - 1);
			  found = TRUE;
			}
		    }
		}
            }
	  if (! found) theEncoding = "UTF-8";
	};
    }  
    return OK;
}


/* static */
eFlag TreeConstructer::feedDocumentToParser(Sit S, void* constructer)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    //char convBuffer[PARSE_CONV_BUFSIZE];
    //char rawBuffer[PARSE_BUFSIZE + 1];

    char *convBuffer, *rawBuffer;
    GPA( GChar ) convAux = convBuffer = new char[PARSE_CONV_BUFSIZE];
    GPA( GChar ) rawAux = rawBuffer = new char[PARSE_BUFSIZE + 1];

    char *outbuf = (char *) convBuffer;
    const char *inbuf = (char *) rawBuffer;

    Bool quit = FALSE, firstTime = TRUE;
    Bool mustConvert = FALSE, haveReadAll = FALSE;
    int res = 0, bytes = 0;
    size_t inleft = 0, outleft = 0;
    EncResult convResult = ENC_OK;
    CDesc cd = (CDesc) -1;
    // seems to be useless here
    // void *recoderUD;
    // EncHandler *recoder = NZ(S.getProcessor()) -> getEncHandler(&recoderUD);
    Str theEncoding;
    
    if (S.isError())
        return NOT_OK;
    XML_Parser parser = NZ( this_ -> theParser );

    // GP: tree constructer does get killed on error
    // callers are Processor::parse (stack variable) and ext ent handler (disposes)

    // this is to add the 'xml' namespace declarations
    //
    tcStartNamespace(constructer, "xml", theXMLNamespace);

    while (!quit)
    {
//        char *buf = (char*) XML_GetBuffer(parser, PARSE_BUFSIZE + 1);
        switch (convResult)
        {
        case ENC_OK:
            {
                // just get the next block of data
                bytes = this_ -> theDataLine -> get(S, rawBuffer, PARSE_BUFSIZE); 
                haveReadAll = (bytes < PARSE_BUFSIZE);
                inbuf = rawBuffer;
                inleft = bytes;
            }; break;
        case ENC_EINVAL:
            {
                // buffer ended with an incomplete sequence
                // copy the sequence
                memmove(rawBuffer, inbuf, inleft);
                // get the rest from dataline
                bytes = this_ -> theDataLine -> get(S, rawBuffer + inleft, PARSE_BUFSIZE - inleft); 
                haveReadAll = (bytes < PARSE_BUFSIZE - (int) inleft);
                inbuf = rawBuffer;
                inleft += bytes;
            }; break;
        case ENC_E2BIG:
            {
                // the converted text was too big to fit in convBuffer
                // don't get more, just convert from input buffer
                // not changing haveReadAll, inbuf or inleft
            }; break;
        default:
            sabassert(!"feedDocumentToParser");
        }

        // the test for bytes==-1 is superfluous but just in case
        if (bytes == -1 || S.isError())
        {
            // read error, bailing out
	  //XML_ParserFree(parser);
	  //the parser is freed at another place too
            return NOT_OK;
        };

        if (firstTime)
        {
            // find the document's encoding
            E( getDocEncoding(S, rawBuffer, theEncoding, this_) );
            // decide whether to recode or not
            if (S.recoder().handledByExpat(theEncoding))
                mustConvert = FALSE;
            else
            {
                mustConvert = TRUE;
                XML_SetEncoding(parser, "UTF-8");
                E( S.recoder().openToUTF8(S, theEncoding, cd) );
            }
        }
        
        if (mustConvert)
        {
            outleft = PARSE_CONV_BUFSIZE;
            outbuf = (char *) convBuffer;
            E( S.recoder().conv(S, cd,
                inbuf, inleft, outbuf, outleft, 
                convResult) );
            switch(convResult)
            {
            case ENC_OK:
                quit = haveReadAll;
                break;
            case ENC_EINVAL:
            case ENC_E2BIG:
                quit = FALSE;
                break;
            case ENC_EILSEQ:
		        Err1T(this_, S, E1_BAD_CHAR, theEncoding);
                break;
            default:
                sabassert(!"bad convResult");
            };
            bytes = PARSE_CONV_BUFSIZE - outleft;
            res = XML_Parse(parser,convBuffer,bytes,FALSE);
        }
        else
        {
            quit = haveReadAll;
            res = XML_Parse(parser,rawBuffer,bytes,FALSE);
        }
        if (S.isError())
            return NOT_OK;
	if (!res)
	  break;
        firstTime = FALSE;
    }

    if (res) 
	{
        //finish with parser
        res = XML_Parse(parser, NULL, 0, TRUE);
        if (S.isError())
            return NOT_OK;
	}
    if (!res) 
    {
        // parsing error to be reported
        // situation_.setCurrFile(t -> name);   is unnecessary as already set

        // hack to avoid an apparent bug in expat causing crashes when an UTF-8 text
        // happens to start with a byte order mark by mistake
        if (!(firstTime && rawBuffer[0] == (char) 0xEF && 
            rawBuffer[1] == (char) 0xBB && rawBuffer[2] == (char) 0xBF))
            S.setCurrLine(XML_GetCurrentLineNumber(parser));
        XML_Error code = XML_GetErrorCode(parser); 
        Str eCodeStr, eNameStr;
        eCodeStr = (int)code;
        eNameStr = (char*) XML_ErrorString(code);
        // XML_ParserFree(parser); -- done later
        Err2T(this_, S, E_XML, eCodeStr, eNameStr);
    }
    
    // remove the 'xml' namespace declarations
    //
    tcEndNamespace(constructer, "xml");
    return OK;
}

//
//  tcStartDocument
//  callback for the document start event
//

/* static */
void TreeConstructer::tcStartDocument(
    void* constructer)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    // new slot for count of pending namespaces
    this_ -> nsCount.append(0);
};



//
//  tcStartElement
//  callback for the element start event
//

void _dump(Tree *tree, QName &name)
{
  Str uri = tree -> expand(name.getUri());
  Str loc = tree -> expand(name.getLocal());
  Str pre = tree -> expand(name.getPrefix());

  printf("%s, %s, %s\n", (char*)uri, (char*)pre, (char*)loc);
}

/* static */
void TreeConstructer::tcStartElement(
    void *constructer,const char *elName,const char **atts)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;

    Tree *t = this_ -> theTree;
    char **p = (char**) atts;
    XSL_OP opCode;
    XSL_ATT attCode;
    BOOL itsXSL = FALSE;
    BOOL itsExtension = FALSE;
    Vertex *v;  // GP: OK (goes to tree)
    Attribute *a;   // GP: OK (goes to tree)
    QName el;
    int elemLine;
    int xmlSpace = -1;
    AttsCache auxAtts;

    if (this_ -> S.isError())
        return;

    this_ -> S.setCurrLine(elemLine = this_ -> getCurrentLineNumber());

    //get elName to QName q - DO NOT CHANGE LATER!!
    if (setQNameFromExpat(this_ -> S, this_, el, elName))
        return;

    itsXSL = (t -> XSLTree) && (t -> expand(el.getUri()) == theXSLTNamespace);

    //first we have to create all attributes, we need them for extension
    //and excluded namespace definitions
    t -> markNamespacePrefixes(this_ -> S); //IMPORTANT

    while(*p)
    {
      QName q;
      if (setQNameFromExpat(this_ -> S, this_, q, (char *)p[0]))
	return;
      
      //check xml attributes (currently xml:space)
      if (q.getUri() == t -> stdPhrase(PHRASE_XML_NAMESPACE) && 
	  t -> expand(q.getLocal()) == (const char*)"space")
	{
	  if (!strcmp(p[1], "preserve"))
	    xmlSpace = 1;
	  else
	    xmlSpace = 0;
	}
      
      //xsl attributes may occure in literal ements!!
      //attCode = (itsXSL ? 
      attCode = (itsXSL || (t -> expand(q.getUri()) == theXSLTNamespace) ?
		 (XSL_ATT) lookup(t -> expand(q.getLocal()),xslAttNames) : 
		 XSLA_NONE);
      //check duplicities
      if (auxAtts.find(q))
	{
	  Str name;
	  t -> expandQStr(q, name);
	  this_ -> S.message(MT_ERROR, E_DUPLICIT_ATTRIBUTE, 
			     (char*)name, (char*)NULL);
	}
      a = new(&(t -> getArena())) Attribute(*t, q, p[1],attCode);
      a -> lineno = this_ -> getCurrentLineNumber();
      auxAtts.append(a);

      //process extension and excluded prefixes
      if (attCode == XSLA_EXT_ELEM_PREFIXES || 
	  attCode == XSLA_EXCL_RES_PREFIXES)
	{
	  t -> pushNamespacePrefixes(this_ -> S, a -> cont, attCode);
	}
      //move on
      p += 2;
    };

    // DETACH: optimize this comparison ###
    if (itsXSL) 
      //t -> stdPhrase(PHRASE_XSL_NAMESPACE)))
      {
        opCode = (XSL_OP) lookup((char*)(t->expand(el.getLocal())),xslOpNames);
        if (opCode == XSL_NONE)
	  {
	    if (this_ -> theDataLine) {
	      if (this_ -> inSAXForAWhile.last())
		this_ -> S.setCurrFile(*(this_ -> theSAXUri.last()));
	      else
		this_ -> S.setCurrFile(this_ -> theDataLine -> fullUri);
	    }
	    this_ -> S.message(MT_ERROR, ET_BAD_XSL, (char*)NULL, (char*)NULL);
	    return;
	  };
        v = new(&(t -> getArena())) XSLElement(*t, el, opCode);
      }
    else if ((t->XSLTree) && t -> isExtensionUri(el.getUri())) 
      { 
	//extension element
	v = new(&(t -> getArena())) ExtensionElement(*t, el);
	itsExtension = TRUE;
      }
    else
      {
        v = new(&(t -> getArena())) Element(*t, el);
	Processor *proc = this_->S.getProcessor();
	if ( proc && proc->outputter() ) 
	  {
	    OutputDocument *doc = proc->outputter()->getDocumentForLevel(TRUE);
	    toE(v)->setOutputDocument(doc);
	  }
      }
    
    //check whether it is top level foreign
    if (t -> XSLTree) 
      {
	if ( (t -> stackTop -> vt & VT_TOP_FOREIGN) ||
	     (!isXSL(v) &&
	      isXSL(t -> stackTop) && 
	      ((toX(t -> stackTop) -> op == XSL_STYLESHEET) || 
	       (toX(t -> stackTop) -> op == XSL_TRANSFORM))) )
	  {
	    v -> vt = (VTYPE)(v -> vt | VT_TOP_FOREIGN);
	  }
      }

    v -> lineno = elemLine;
    t -> appendVertex(this_ -> S, v);

    //if we are looking at the document element we need to check,
    // whether the default namespace was declared
    if (isRoot(v -> parent) && t -> pendingNS().findNdx(UNDEF_PHRASE) == -1)
      tcStartNamespace(constructer, NULL, ""); //empty default namespace

    t -> pendingNS().giveCurrent(this_ -> S, toE(v) -> namespaces, t,
				 this_ -> nsCount.last());
#ifndef EXPAT_SUPPORTS_TRIPLETS
    toE(v) -> namespaces.findPrefix(toE(v) -> name);
#endif
      //_dump(t, toE(v) -> name);

    toE(v) -> namespaces.incPrefixUsage(toE(v) -> name.prefix);

    //add attributes to the element
    for (int n = 0; n < auxAtts.number(); n++)
      {
	const QName *auxName = &(auxAtts[n] -> getName());
#ifndef EXPAT_SUPPORTS_TRIPLETS
	//force the prefix!!
	toE(v) -> namespaces.findPrefix(*(const_cast<QName*>(auxName)));
#endif
	t -> appendVertex(this_ -> S, auxAtts[n]);
	if (auxName -> getPrefix() != UNDEF_PHRASE)
	  toE(v) -> namespaces.incPrefixUsage(auxName -> getPrefix());
      }
    auxAtts.keepThem();

     //process xml space
    if (xmlSpace == -1)
      {
 	Bool val = this_ -> preserveSpace.number() ? 
 	  this_ -> preserveSpace.last() : FALSE;
 	this_ -> preserveSpace.append(val);
      }
    else
      this_ -> preserveSpace.append(xmlSpace);
    toE(v) -> preserveSpace = this_ -> preserveSpace.last();
    //xml-space end

    if (itsXSL)
      {
        //t -> extensionNamespaces(this_ -> S, toE(v));
        if (toX(v) -> checkAtts(this_ -> S)) return; 
        // also check if a toplevel element does not 
	//have a non-stylesheet parent
        if (toX(v) -> checkToplevel(this_ -> S)) return; 
      }
    else if (itsExtension)
      {
        if (toExtension(v) -> checkAtts(this_ -> S)) return; 
      }
    else
      {
        if (t -> XSLTree)
	  {
	    int k;
	    int kLimit = toE(v) -> atts.number();
	    for (k = 0; k < kLimit; k++)
	      if (toA(toE(v) -> atts[k]) -> buildExpr(this_ -> S,
						      TRUE, EX_NONE)) return;
	    // this calls situation.error() on error
        }
      }
    this_ -> nsCount.append(0);
}

//
//  tcEndElement
//  callback for the element end event
//

/* static */
void TreeConstructer::tcEndElement(
    void* constructer, const char* name)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    Tree *t = this_ -> theTree;
    // destroyed slot for count of pending namespaces
    this_ -> nsCount.deppend();

    Vertex *v = NZ( t -> stackTop );
    t -> flushPendingText();
    
    if (t -> XSLTree)
        t -> stripped += 
        (isXSLElement(v)? toX(v) : cast(Daddy*, v))
            -> strip();

    if (isXSLElement(v))
    {
        // situation.error() is called in the following
        if (toX(v) -> checkChildren(this_ -> S))
            return;
    } 
    else if ( isExtension(v) ) 
      {
        if (toExtension(v) -> checkChildren(this_ -> S))
            return;
      }

    //IMPORTANT STUFF
    t -> processVertexAfterParse(this_ -> S, v, this_);

    //cleanup
    t -> popNamespacePrefixes(this_ -> S);
    
    //pop preserver space
    this_ -> preserveSpace.deppend();
}


//
//  tcStartNamespace
//  callback for the namespace scope start event
//

/* static */
void TreeConstructer::tcStartNamespace(
    void* constructer, const char* prefix, const char* uri)
{
  tcStartNamespace2(constructer, prefix, uri, false);
}

void TreeConstructer::tcStartNamespace2(
    void* constructer, const char* prefix, const char* uri, Bool hidden)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    Tree *t = this_ -> theTree;

    Vertex *newv;   // GP: OK (goes to tree)

    //XML_Parser parser = this_ -> theParser;

    Phrase prefixPh, uriPh;
    if (prefix && *prefix) //not empty/default prefix
      {
        t -> dict().insert(prefix, prefixPh);
      }
    else
      {
        prefixPh = UNDEF_PHRASE;
      }
    t -> dict().insert(uri, uriPh);

    // t -> pendingNS().appendAndSetOrdinal(
    t -> pendingNS().append(
        newv = new(&(t -> getArena())) 
	    NmSpace(*t, prefixPh, uriPh, hidden));
    
    newv -> lineno = this_ -> getCurrentLineNumber();
    // count of pending namespaces increased
    this_ -> nsCount[this_ -> nsCount.number()-1]++;

    // warn on obsolete namespace
    if (uri && !strcmp(oldXSLTNamespace, uri)) /* _PH_ */
        this_ -> Warn1(this_ -> S, W1_OLD_NS_USED, (char*)uri)
    else
      {
        if (prefix && !strcmp(prefix, "xsl") && 
	      uri && strcmp(theXSLTNamespace, uri)) /* _PH_ */
	  this_ -> Warn1(this_ -> S, W1_XSL_NOT_XSL, (char*) uri);
      }
};

//
//  tcEndNamespace
//  callback for the namespace scope end event
//

/* static */
void TreeConstructer::tcEndNamespace(
    void* constructer, const char* prefix)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    Tree *t = this_ -> theTree;

#ifdef _DEBUG
    // hope this works
    Phrase prefixPh;
    if (prefix && *prefix)
        prefixPh = t -> dict().lookup(prefix);
    else
        prefixPh = UNDEF_PHRASE;
    //sabassert(toNS(t -> pendingNS().last()) -> prefix == prefixPh);
#endif
    t -> pendingNS().freelast(FALSE);
    // count of pending namespaces decreased
    this_ -> nsCount[this_ -> nsCount.number()-1]--;
};


//
//  tcComment
//  callback for the comment event
//

/* static */
void TreeConstructer::tcComment(
    void* constructer, const char* contents)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    Tree *t = this_ -> theTree;

    if (t -> XSLTree)
        return;

    Comment *newNode;   // GP: OK
    newNode = new(&(t -> getArena())) Comment(*t, contents);
    newNode -> lineno = this_ -> getCurrentLineNumber();

    Processor *proc = this_ -> S.getProcessor();
    if ( proc && proc->outputter() ) 
      {
	OutputDocument *doc = proc->outputter()->getDocumentForLevel(FALSE);
	newNode -> setOutputDocument(doc);
      }

    t -> appendVertex(this_ -> S, newNode);
};


//
//  tcPI
//  callback for the processing instruction event
//

/* static */
void TreeConstructer::tcPI(
    void* constructer, const char* target, const char* contents)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    Tree *t = this_ -> theTree;

    if (t -> XSLTree)
        return;

    ProcInstr *newNode; // GP: OK
    Phrase targetPh;
    t -> dict().insert(target, targetPh);

    newNode = new(&(t -> getArena()))
        ProcInstr(*t, targetPh, contents);
    newNode -> lineno = this_ -> getCurrentLineNumber();

    Processor *proc = this_ -> S.getProcessor();
    if ( proc && proc->outputter() ) 
      {
	OutputDocument *doc = proc->outputter()->getDocumentForLevel(FALSE);
	newNode -> setOutputDocument(doc);
      }
    
    t -> appendVertex(this_ -> S, newNode);
};


//
//  tcCharacters
//  callback for the character data ("text") event
//

/* static */
void TreeConstructer::tcCharacters(
    void* constructer, const char* contents, int length)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    Tree *t = this_ -> theTree;

    Vertex *newVertex;  // GP: OK
    if (!!(newVertex = t -> appendText(this_ -> S, (char *) contents, length)))
        newVertex -> lineno = this_ -> getCurrentLineNumber();
};



//
//  tcEndDocument
//  callback for the document end event
//

/* static */
void TreeConstructer::tcEndDocument(
    void* constructer)
{
    TreeConstructer *this_ =
        (TreeConstructer*) constructer;
    if (this_ -> S.isError())
        return;
    // slot for namespaces counted for childs of this element destroyed 
    this_ -> nsCount.deppend();
};


//
//  tcUnknownEncoding
//  callback for the unknown encoding event (expat)
//  needs to have "enc1250.h" included
//


// the unknown encoding handler is no more used:
/* static */
/*
int TreeConstructer::tcUnknownEncoding(
    void *encodingHandlerData, const char *name, XML_Encoding *info)
{
    int *theTable;
    if (strEqNoCase((char*) name,"windows-1250"))
        theTable = Enc1250;
    else if (strEqNoCase((char*) name,"iso-8859-2"))
        theTable = EncLatin2;
    else
        return 0;
    int i;
    for (i = 0; i < 0x80; i++)
    {
        info -> map[i] = i;
        info -> map[i + 0x80] = theTable[i];
    }
    info -> map[0x7f] = -1;
    info -> data = NULL;
    info -> convert = NULL;
    info -> release = NULL;
    return 1;
};
*/

//
//  tcExternalEntity
//  callback for the external entity reference event (expat)
//

/* static */
int TreeConstructer::tcExternalEntityRef(
    XML_Parser parser, const char* context, const char* base,
    const char* systemId, const char* publicId)
{
    TreeConstructer *this_ =
        (TreeConstructer*) XML_GetUserData(parser);

    //ignore external entities at all or just public ones
    if (this_ -> S.hasFlag(SAB_NO_EXTERNAL_ENTITIES) ||
	(publicId && ! this_ -> S.hasFlag(SAB_PARSE_PUBLIC_ENTITIES)))
      return 1;
    
    if (this_ -> S.isError())
        return 0;
    Tree *t = this_ -> theTree;

    this_ -> Log1(this_ -> S, L1_READING_EXT_ENTITY, systemId);    
    XML_Parser newParser =
        XML_ExternalEntityParserCreate(parser, context, /* encoding= */ NULL);

    if (!newParser)
        return 0;

    Str absolute;
    makeAbsoluteURI(this_ -> S,systemId, base, absolute);
    XML_SetBase(newParser, absolute); //Christian + PH

    GP(DataLine) newDL = new DataLine;
    if ( (*newDL).open(this_ -> S, absolute, DLMODE_READ, NULL) )
      {
        XML_ParserFree(newParser);
        return 0;
      }

    /*
    if (NZ(this_->S.getProcessor())->addLineNoTree(this_ -> S, newDL, absolute, 
						   t -> XSLTree) || !newDL)
      {
        XML_ParserFree(newParser);
        return 0;
      }
    */

    TreeConstructer *newTC;
    MT( this_, this_ -> S, newTC = new TreeConstructer(this_ -> S));

    // record subtree information for the external entity
    // XSL_NONE stands for 'external entity'
    eFlag code = t -> startSubtree(this_ -> S, absolute, XSL_NONE, TRUE);
    if (!code)
    {
	code = newTC -> parseDataLineUsingGivenExpat(this_ -> S, t, newDL, newParser);
	t -> endSubtree(this_ -> S, XSL_NONE); 
    }
    (*newDL).close(this_ -> S);
    XML_ParserFree(newParser);
    delete newTC;
    return code ? 0 : 1;
}

void TreeConstructer::tcEntityDecl(void *userData, const char *entityName,
				   int is_parameter_entity, const char *value,
				   int value_length, const char *base,
				   const char *systemId, const char *publicId,
				   const char *notationName)
{
  TreeConstructer *this_ = (TreeConstructer*) userData;
  if (notationName)
    {
      Str name = entityName;
      Str absolute;
      makeAbsoluteURI(this_ -> S, systemId, base, absolute);
      this_ -> theTree -> setUnparsedEntityUri( name, absolute );
    }
}

/* methods for internal sax handler */
void TreeConstructer::saxStartDocument(void* userData, 
				       SablotHandle processor_)
{
  tcStartDocument(userData);
}

void TreeConstructer::saxStartElement(void *constructer,
				      SablotHandle processor_,
				      const char *elName, const char **atts)
{
  tcStartElement(constructer, elName, atts);
}
 
void TreeConstructer::saxEndElement(void* constructer, 
				    SablotHandle processor_, 
				    const char* name)
{
  tcEndElement(constructer, name);
}

void TreeConstructer::saxStartNamespace(void* userData, 
					SablotHandle processor_,
					const char* prefix, const char* uri)
{
  tcStartNamespace(userData, prefix, uri);
}

void TreeConstructer::saxStartNamespace2(void* userData, 
					 SablotHandle processor_,
					 const char* prefix, const char* uri,
					 Bool hidden)
{
  tcStartNamespace2(userData, prefix, uri, hidden);
}

void TreeConstructer::saxEndNamespace(void* constructer, 
				      SablotHandle processor_, 
				      const char* prefix)
{
  tcEndNamespace(constructer, prefix);
}

void TreeConstructer::saxComment(void* constructer, 
				 SablotHandle processor_, 
				 const char* contents)
{
  tcComment(constructer, contents);
}

void TreeConstructer::saxPI(void* constructer, 
			    SablotHandle processor_,
			    const char* target, const char* contents)
{
  tcPI(constructer, target, contents);
}

void TreeConstructer::saxCharacters(void* constructer, 
				    SablotHandle processor_,
				    const char* contents, int length)
{
  tcCharacters(constructer, contents, length);
}

void TreeConstructer::saxEndDocument(void* constructer, 
				     SablotHandle processor_)
{
  tcEndDocument(constructer);
}

/* static MUST NOT RELY ON EXPAT_SUPPORTS_TRIPLETS*/
eFlag TreeConstructer::setQNameFromExpat(Sit S, TreeConstructer* this_, QName& qname_, const char* text)
{
    Tree *t = this_ -> theTree;
    char *p = (char*) text,
        *q = strchr(p, THE_NAMESPACE_SEPARATOR);
    if (q)
    {
        *q = 0;

        qname_.setUri(t -> unexpand(p));
        *q = NS_SEP;
	
	p = q + 1;
	q = strchr(p, THE_NAMESPACE_SEPARATOR);
	if (q) *q = 0;

        qname_.setLocal(t -> unexpand(p));
        if (strchr(p,':'))
        {
            DStr msg = "{";
            msg += t -> expand(qname_.getUri());
            msg += "}:";
            msg += t -> expand(qname_.getLocal());
            Err1T(this_, S, E1_EXTRA_COLON, (char *)msg);
        }
	//read the prefix - should append only if XML_SetReturnNSTriplet
	//works
	if (q)
	  {
	    *q = NS_SEP;
	    qname_.setPrefix(t -> unexpand(q + 1));
	  }
    }
    else
    {
        qname_.uri = UNDEF_PHRASE;
        qname_.setLocal(t -> unexpand(p));
        char *isColon;
        qname_.prefix = UNDEF_PHRASE;
        if (!!(isColon = strchr(p,':')))
        {
            *isColon = 0;

            // fixing what appears a bug in expat 
	    //- sometimes the xml:lang attr arrives unexpanded
            // apparently only in XSL and when it's not at the top level
            if (!strEqNoCase(p,"xml"))
                Err1T(this_, S, ET_BAD_PREFIX,p)
            else
            {
                qname_.setLocal(t -> unexpand(isColon + 1));
                qname_.setUri(t -> stdPhrase(PHRASE_XML_NAMESPACE));
		        qname_.setPrefix(t -> unexpand((char*)"xml"));
                // E( t -> dict().insert(S, "xml",qname_.getPrefix) );                
            }
        }
    };
    return OK;
}

void TreeConstructer::report(Sit S, MsgType type, MsgCode code, 
    const Str& arg1, const Str& arg2) const 
{
  if (inSAXForAWhile.last())
    S.setCurrFile(*(theSAXUri.last()));
  else if (theDataLine)
    S.setCurrFile(theDataLine -> fullUri);
  S.setCurrLine(getCurrentLineNumber());
  S.message(type, code, arg1, arg2);
};


