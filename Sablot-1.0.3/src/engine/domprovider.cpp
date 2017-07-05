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

#define SablotAsExport
#include "sablot.h"
#include "domprovider.h"
#include "verts.h"
#include "tree.h"
#include "context.h"
#include "proc.h"
#include "vars.h"
#include "guard.h"
#include <string.h>

/* DOMProvider */
void DOMProvider::constructStringValue(SXP_Node n, DStr &result)
{
    switch(getNodeType(n)) {
    case ATTRIBUTE_NODE:
    case NAMESPACE_NODE:
    case PROCESSING_INSTRUCTION_NODE:
    case COMMENT_NODE:
    case TEXT_NODE:
      {
	const SXP_char* val = getNodeValue(n);
	if (val)
	  {
	    result += val; 
	    freeValue(n, (char*)val);
	  }
      }; break;
    case DOCUMENT_NODE:
    case ELEMENT_NODE:
      {
	for (NodeHandle son = getFirstChild(n); !nhNull(son); son = getNextSibling(son))
	  switch(getNodeType(son)) {
	  case ELEMENT_NODE:
	  case TEXT_NODE:
	    {
	      constructStringValue(son, result);
	    }; break;
	  default:
	    {
	      //nothing
	    }; break;
	  }
      }; break;
    }
}

SXP_Node DOMProvider::getFirstChild(SXP_Node n)
{
    return getChildNo(n,0);
}

void DOMProvider::freeName(NodeHandle n, char *buff)
{
  sabassert(!"abstract DOMProvider::freeName()");
}

void DOMProvider::freeValue(NodeHandle n, char *buff)
{
  sabassert(!"abstract DOMProvider::freeValue()");
}

eFlag DOMProvider::startCopy(Sit S, NodeHandle n, OutputterObj &outputter)
{
  sabassert(!"abstract DOMProvider::startCopy()");
  return OK;
}

eFlag DOMProvider::endCopy(Sit S, NodeHandle n, OutputterObj &outputter)
{
  sabassert(!"abstract DOMProvider::endCopy()");
  return OK;
}

eFlag DOMProvider::copyNode(Sit S, NodeHandle n, OutputterObj &outputter)
{
  sabassert(!"abstract DOMProvider::copyNode()");
  return OK;
}

eFlag DOMProvider::getMatchingList(Sit S, NodeHandle n, 
				  Expression &match, Context &result)
{
  //try itself
  Context aux(NULL);
  aux.set(n);
  Bool yes;
  E( match.matchesPattern(S, &aux, yes) );
  if (yes) result.append(n);
  //iterate the tree
  SXP_NodeType type;
  switch ( type = getNodeType(n) ) {
  case DOCUMENT_NODE:
  case ELEMENT_NODE:
    {
      int num, i;
      if (type == ELEMENT_NODE) //!!! CHECK
	{
	  //namespaces
	  num = getNamespaceCount(n);
	  for (i = 0; i < num; i++)
	    getMatchingList(S, getNamespaceNo(n, i), match, result);
	  //attributes
	  num = getAttributeCount(n);
	  for (i = 0; i < num; i++)
	    getMatchingList(S, getAttributeNo(n, i), match, result);
	}
      num = getChildCount(n);
      for (i = 0; i < num; i++)
	getMatchingList(S, getChildNo(n, i), match, result);
    }; break;
  }
  return OK;  
}

/************************************************************
dom provider external
************************************************************/
void DOMProviderExternal::getNodeEName(NodeHandle n, EQName &ename)
{
  const char *name, *uri;
  name = getNodeName(n);
  char *aux = (char *)strchr(name, ':');
  if (aux)
    {
      *aux = '\0';
      ename.setPrefix(name);
      ename.setLocal(aux + 1);
      *aux = ':';
    }
  else
    {
      ename.setLocal(name);
    }
  ename.setUri(uri = getNodeNameURI(n));
  freeName(n, (char*)name);
  freeName(n, (char*)uri);
}

eFlag DOMProviderExternal::startCopy(Sit S, NodeHandle n, 
				     OutputterObj &outputter)
{
  switch ( getNodeType(n) ) {
  case ELEMENT_NODE:
    {
      EQName ename;
      getNodeEName(n, ename);
      E( outputter.eventElementStart(S, ename) );
      //copy namespaces
      int num = getNamespaceCount(n);
      for (int i = 0; i < num; i++)
	startCopy(S, getNamespaceNo(n, i), outputter);
    }; break;
  case ATTRIBUTE_NODE:
    {
      EQName ename;
      getNodeEName(n, ename);
      const char *val = getNodeValue(n);
      E( outputter.eventAttributeStart(S, ename) );
      E( outputter.eventData(S, val) );
      E( outputter.eventAttributeEnd(S) );
      freeValue(n, (char*)val);
    }; break;
  case TEXT_NODE:
    {
      const char *val = getNodeValue(n);
      E( outputter.eventData(S, val) );
      freeValue(n, (char*)val);
    }; break;
  case PROCESSING_INSTRUCTION_NODE:
    {
      const char *name = getNodeNameLocal(n);
      const char *val = getNodeValue(n);
      E( outputter.eventPIStart(S, name) );
      E( outputter.eventData(S, val) );
      E( outputter.eventPIEnd(S) );
      freeName(n, (char*)name);
      freeValue(n, (char*)val);
    }; break;
  case COMMENT_NODE:
    {
      const char *val = getNodeValue(n);
      E( outputter.eventCommentStart(S) );
      E( outputter.eventData(S, val) );
      E( outputter.eventCommentEnd(S) );
      freeValue(n, (char*)val);
    }; break;
  case DOCUMENT_NODE:
    {}; break;
  case NAMESPACE_NODE:
    {
      const char *name = getNodeName(n);
      const char *val = getNodeValue(n);
      outputter.eventNamespace(S, name, val);
      freeName(n, (char*)name);
      freeValue(n, (char*)val);
    }; break;
  }
  return OK;
}

eFlag DOMProviderExternal::endCopy(Sit S, NodeHandle n, 
				   OutputterObj &outputter)
{
  switch ( getNodeType(n) ) {
  case ELEMENT_NODE:
    {
      EQName ename;
      getNodeEName(n, ename);
      E( outputter.eventElementEnd(S, ename ));
    }; break;
  case ATTRIBUTE_NODE:
  case TEXT_NODE:
  case PROCESSING_INSTRUCTION_NODE:
  case COMMENT_NODE:
  case DOCUMENT_NODE:
  case NAMESPACE_NODE:
    break;
  }
  return OK;
}

eFlag DOMProviderExternal::copyNode(Sit S, NodeHandle n, 
				    OutputterObj &outputter)
{
  SXP_NodeType type;
  switch ( type = getNodeType(n) ) {
  case DOCUMENT_NODE:
  case ELEMENT_NODE:
    {
      int num, i;
      E( startCopy(S, n, outputter) );
      //attributes
      if (type == ELEMENT_NODE)
	{
	  num = getAttributeCount(n);
	  for (i = 0; i < num; i++)
	    E( copyNode(S, getAttributeNo(n, i), outputter) );
	}
      //contents
      num = getChildCount(n);
      for (i = 0; i < num; i++)
	E( copyNode(S, getChildNo(n, i), outputter) );
      //end
      E( endCopy(S, n, outputter) );
    }; break;
  case ATTRIBUTE_NODE:
  case TEXT_NODE:
  case PROCESSING_INSTRUCTION_NODE:
  case COMMENT_NODE:
  case NAMESPACE_NODE:
    {
      startCopy(S, n, outputter);
      endCopy(S, n, outputter);
    }; break;
  }
  return OK;
}

/************************************************************
dom provider standard
************************************************************/

SXP_NodeType DOMProviderStandard::getNodeType(SXP_Node n)
{
    switch(baseType(n))
    {
    case VT_ELEMENT: return ELEMENT_NODE;
    case VT_ATTRIBUTE: return ATTRIBUTE_NODE;
    case VT_NAMESPACE: return NAMESPACE_NODE;
    case VT_TEXT: return TEXT_NODE;
    case VT_COMMENT: return COMMENT_NODE;
    case VT_PI: return PROCESSING_INSTRUCTION_NODE;
    case VT_ROOT: return DOCUMENT_NODE;
    default: 
      {
	sabassert(0); 
	return (SXP_NodeType)0;
      }
    }
}

/*
		case EXFF_NAME:
		    toV(v) -> getOwner().expandQStr(q, strg); break;
		case EXFF_LOCAL_NAME:
		    strg = toV(v) -> getOwner().expand(q.getLocal()); break;
		case EXFF_NAMESPACE_URI:
		    strg = toV(v) -> getOwner().expand(q.getUri()); break;
*/

const SXP_char* DOMProviderStandard::getNodeName(SXP_Node n)
{
  Str aux;
  const QName& q = toV(n) -> getName();
  toV(n) -> getOwner().expandQStr(q, aux);
  return aux.cloneData();
}

const SXP_char* DOMProviderStandard::getNodeNameURI(SXP_Node n)
{
  const QName& q = toV(n) -> getName();
  Str aux = toV(n) -> getOwner().expand(q.getUri());
  return aux.cloneData();
}

const SXP_char* DOMProviderStandard::getNodeNameLocal(SXP_Node n)
{
  const QName& q = toV(n) -> getName();
  Str aux = toV(n) -> getOwner().expand(q.getLocal());
  return aux.cloneData();
}

const SXP_char* DOMProviderStandard::getNodeValue(SXP_Node n)
{
    switch(baseType(n))
    {
        case VT_ATTRIBUTE:
	        return toA(n) -> cont;
		case VT_NAMESPACE:
		{
	        return toNS(n) -> getOwner().dict().getKey(toNS(n) -> uri);
		};
		case VT_PI:
	        return toPI(n) -> cont;
		case VT_COMMENT:
		    return toComment(n) -> cont;
		case VT_TEXT:
		    return toText(n) -> cont;
		default:
		    return NULL;
    }
}

SXP_Node DOMProviderStandard::getNextSibling(SXP_Node n)
{
    SXP_Node par = getParent(n);
    int ndx = toV(n) -> ordinal,
        count = -1;
    if (!par) return NULL;
    if (baseType(n) == VT_ATTRIBUTE || baseType(n) == VT_NAMESPACE)
      return NULL;
    count = toD(par) -> contents.number();
    if (ndx >= count - 1)
        return NULL;
    return toD(par) -> contents[ndx + 1];
}

SXP_Node DOMProviderStandard::getPreviousSibling(SXP_Node n)
{
    SXP_Node par = getParent(n);
    int ndx = toV(n) -> ordinal;
    if (!par || !ndx) return NULL;
    if (baseType(n) == VT_ATTRIBUTE || baseType(n) == VT_NAMESPACE)
        return NULL;
    return toD(par) -> contents[ndx - 1];
}

SXP_Node DOMProviderStandard::getNextAttrNS(SXP_Node n)
{
    SXP_Node par = getParent(n);
    int ndx = toV(n) -> ordinal,
        count = -1;
    if (!par) return NULL;
    switch(baseType(n))
    {
        case VT_ATTRIBUTE:
	        count = toE(par) -> atts.number(); break;
        case VT_NAMESPACE:
	        count = toE(par) -> namespaces.number(); break;
		default:
		    return NULL;
    }
    if (ndx >= count - 1)
        return NULL;
    switch(baseType(n))
    {
        case VT_ATTRIBUTE:
	        return toE(par) -> atts[ndx + 1]; break;
        case VT_NAMESPACE:
	  return toE(par) -> namespaces[ndx + 1]; break;
    default:
      // do this to keep compiler happy
      return NULL;
    }	
}

SXP_Node DOMProviderStandard::getPreviousAttrNS(SXP_Node n)
{
    SXP_Node par = getParent(n);
    int ndx = toV(n) -> ordinal;
    if (!par || !ndx) return NULL;
    switch(baseType(n))
    {
        case VT_ATTRIBUTE:
	        return toE(par) -> atts[ndx - 1]; break;
        case VT_NAMESPACE:
	        return toE(par) -> namespaces[ndx - 1]; break;
		default:
		    return NULL;
    }	
}

int DOMProviderStandard::getChildCount(SXP_Node n)
{
    if (baseType(n) != VT_ELEMENT && baseType(n) != VT_ROOT)
        return 0;
	else
	    return toE(n) -> contents.number();
}

int DOMProviderStandard::getAttributeCount(SXP_Node n)
{
    if (baseType(n) != VT_ELEMENT)
        return 0;
	else
	    return toE(n) -> atts.number();
}

int DOMProviderStandard::getNamespaceCount(SXP_Node n)
{
    if (baseType(n) != VT_ELEMENT)
        return 0;
	else
	    return toE(n) -> namespaces.number();
}

SXP_Node DOMProviderStandard::getChildNo(SXP_Node n, int ndx)
{
    if (baseType(n) != VT_ELEMENT && baseType(n) != VT_ROOT)
        return NULL;
	else
	{
	    if (ndx < 0 || ndx >= toD(n) -> contents.number())
	        return NULL;
		else
	        return toD(n) -> contents[ndx];
	}
}

SXP_Node DOMProviderStandard::getAttributeNo(SXP_Node n, int ndx)
{
    if (baseType(n) != VT_ELEMENT)
        return NULL;
	else
	{
	    if (ndx < 0 || ndx >= toE(n) -> atts.number())
	        return NULL;
		else
	        return toE(n) -> atts[ndx];
	}
}

SXP_Node DOMProviderStandard::getNamespaceNo(SXP_Node n, int ndx)
{
    if (baseType(n) != VT_ELEMENT)
        return NULL;
	else
	{
	    if (ndx < 0 || ndx >= toE(n) -> namespaces.number())
	        return NULL;
		else
	        return toE(n) -> namespaces[ndx];
    }
}

SXP_Node DOMProviderStandard::getParent(SXP_Node n)
{
    return toV(n) -> parent;
}

SXP_Document DOMProviderStandard::getOwnerDocument(SXP_Node n)
{
    return &(toV(n) -> getOwner().getRoot());
}

int DOMProviderStandard::compareNodes(SXP_Node n1, SXP_Node n2)
{
  if (&(toV(n1) -> getOwner()) == &(toV(n2) -> getOwner()) ) //the same owner
    {
      int s1 = toV(n1) -> stamp;
      int s2 = toV(n2) -> stamp;
      if (s1 < s2)
	return -1;
      if (s1 == s2)
	return 0;
      return 1;
    }
  else
    {
      return strcmp( (char*)(toV(n1) -> getOwner().getURI()),
		     (char*)(toV(n2) -> getOwner().getURI()) );
    }
}

SXP_Document DOMProviderStandard::retrieveDocument(const SXP_char* uri, const SXP_char* baseUri)
{
    sabassert(0);
    return NULL;
}

SXP_Node DOMProviderStandard::getNodeWithID(SXP_Document doc, const SXP_char* id)
{
    // not reading the DTD, we don't know any ID atts
    return NULL;
}

/*****************************************/
/* universal provider */
/*****************************************/

DOMProviderUniversal::DOMProviderUniversal()
{
  standard = new DOMProviderStandard();
  external = NULL;
  maskBit = 0;
  options = 0;
}

DOMProviderUniversal::~DOMProviderUniversal()
{
  cdelete(standard);
  if (external) cdelete(external);
}

void DOMProviderUniversal::setExtProvider(DOMHandler *domh, void *data)
{
  if (external) cdelete(external);
  if (domh)
    {
      external = new DOMProviderExternal(domh, data);
      external -> setSXPMask(maskBit);
      external -> setOptions(options);
    }
  else
    external = NULL;
}

void DOMProviderUniversal::freeName(NodeHandle n, char *data)
{
  //sabassert( (NHC(n) & 1) ); //shouldn't happen for internal nodes
  if ( ( NHC(n) & 1 ) && (options & SXPF_DISPOSE_NAMES) )
    {
      NZ(external)->freeName(n, data);
    }
}

void DOMProviderUniversal::freeValue(NodeHandle n, char *data)
{
  //sabassert( (NHC(n) & 1) ); //shouldn't happen for internal nodes
  if ( ( NHC(n) & 1 ) && (options & SXPF_DISPOSE_VALUES) )
    {
      NZ(external)->freeValue(n, data);
    }
}

void DOMProviderUniversal::setMaskBit(int mask)
{
  maskBit = mask;
  if (external) external -> setSXPMask(mask);
}

int DOMProviderUniversal::getMaskBit()
{
  return maskBit;
}

int DOMProviderUniversal::compareNodes(SXP_Node n1, SXP_Node n2)
{ 
  if ((NHC(n1) & 1) ^ (NHC(n2) & 1)) //one external other interal
    {
      return NHC(n1) & 1 ? -1 : 1;
    }
    
  return (NHC(n1) & 1) ? 
    NZ(external)->compareNodes(n1, n2) :
    standard->compareNodes(n1, n2);
}

SXP_Document DOMProviderUniversal::retrieveDocument(const SXP_char* uri, const SXP_char* baseUri)
{
  if (external)
    {
      return external -> retrieveDocument(uri, baseUri);
    }
  else
    return NULL;
}

eFlag DOMProviderUniversal::startCopy(Sit S, NodeHandle n, 
				      OutputterObj &outputter)
{
  if ( SXP_EXTERNAL(n) )
    return external -> startCopy(S, n, outputter);
  else
    return toPhysical(n) -> startCopy(S, outputter);
}

eFlag DOMProviderUniversal::endCopy(Sit S, NodeHandle n, 
				    OutputterObj &outputter)
{
  if ( SXP_EXTERNAL(n) )
    return external -> endCopy(S, n, outputter);
  else
    return toPhysical(n) -> endCopy(S, outputter);
}

eFlag DOMProviderUniversal::copyNode(Sit S, NodeHandle n, 
				     OutputterObj &outputter)
{
  if ( SXP_EXTERNAL(n) )
    return external -> copyNode(S, n, outputter);
  else
    return toPhysical(n) -> copy(S, outputter);
}

/*
 *
 *    QueryContextClass
 *
 */
    
QueryContextClass::QueryContextClass(Sit S)
    : theSituation(&S)
{
    baseTree = new Tree((char*)"urn:_external_",FALSE);
    queryExpression = NULL;
    resultExpression = NULL;
    if (NULL == (proc = theSituation -> getProcessor()))
    {
	SablotCreateProcessorForSituation(theSituation, (SablotHandle*)&proc);
	mustKillProc = TRUE;
    }
    else
	mustKillProc = FALSE;
    NZ(proc) -> initForSXP(baseTree);
    stringValue = NULL;
    numberValue = NULL;
}

QueryContextClass::~QueryContextClass()
{
    proc -> cleanupAfterRun(NULL);
    cdelete(queryExpression);
    cdelete(resultExpression);
    cdelete(baseTree);
    cdelete(stringValue);
    cdelete(numberValue);
    if (mustKillProc)
	cdelete(proc);
}

eFlag QueryContextClass::query(
    const SXP_char* queryText,
    SXP_Node n,
    int contextPosition,
    int contextSize)
{
    eFlag ecode;
    //we have to mask the passed-in node handle
    n = SXP_MASK_LEVEL(n, theSituation->getSXPMaskBit());

    cdelete(queryExpression); // unnecessary but just in case
    cdelete(resultExpression);
    cdelete(stringValue);
    cdelete(numberValue);
    GP( Tree ) baseTreeNew = new Tree((char*)"", FALSE);
    queryExpression = new Expression(baseTree -> getRoot());
    resultExpression = new Expression((*baseTreeNew).getRoot());
    proc -> initForSXP(baseTree);
    ecode = queryExpression -> parse(*theSituation, queryText);
    if (!ecode)
    {
      Context c(NULL); //no current node !!!
      c.setVirtual(n, contextPosition, contextSize);
      ecode = queryExpression -> eval(*theSituation, *resultExpression, &c);
    }
    cdelete(queryExpression);
    cdelete(baseTree);
    baseTree = baseTreeNew.keep();
    proc -> cleanupAfterSXP();
    return ecode;
}

eFlag QueryContextClass::addVariableBinding(
    const SXP_char* name, 
    QueryContextClass &source)
{
    return addVariableExpr(name, source.getExpression_());
}

eFlag QueryContextClass::addVariableExpr(
    const SXP_char* name, 
    Expression* value)
{
    QName q;
    E( baseTree -> getRoot().setLogical(*theSituation, q, (char*)name, TRUE) );
    E( proc -> vars -> addBinding(*theSituation, q, value, TRUE) );
    return OK;
}

Expression* QueryContextClass::getNewExpr()
{
    return new Expression(baseTree -> getRoot());
}

eFlag QueryContextClass::addNamespaceDeclaration(
    const SXP_char* prefix, 
    const SXP_char* uri)
{
    baseTree -> getRoot().namespaces.append(
        new(&(baseTree -> getArena())) NmSpace(*baseTree, 
		    baseTree -> unexpand(prefix),
		    baseTree -> unexpand(uri),
		    NSKIND_DECLARED));    
    return OK;
}

Expression* QueryContextClass::getExpression_() const
{
    return resultExpression;
}

SXP_ExpressionType QueryContextClass::getType()
{
    if (!resultExpression)
        return SXP_NONE;
    switch(resultExpression -> type)
    {
        case EX_NUMBER: return SXP_NUMBER;
	    case EX_STRING: return SXP_STRING;
	    case EX_BOOLEAN: return SXP_BOOLEAN;
	    case EX_NODESET: return SXP_NODESET;
	    default: return SXP_NONE;
    }
}

const Number* QueryContextClass::getNumber()
{
    if (!resultExpression)
        return NULL;
	else
	{
	    cdelete(numberValue);
	    numberValue = new Number(resultExpression -> tonumber(*theSituation));
	    return numberValue;
	}
}

const Str* QueryContextClass::getString()
{
    if (!resultExpression)
        return NULL;
	else
	{
	    if (!stringValue) stringValue = new Str;
	    // should be checked for error
	    resultExpression -> tostring(*theSituation, *stringValue);
	    return stringValue;
	}
}

Bool QueryContextClass::getBoolean()
{
    if (!resultExpression)
        return FALSE;
	else
	    return resultExpression -> tobool();
}

const Context* QueryContextClass::getNodeset()
{
    if (!resultExpression || resultExpression -> type != EX_NODESET)
        return NULL;
	else
	    return &(resultExpression -> tonodesetRef());
}
