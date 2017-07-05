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

/* sdom.cpp */

#define SablotAsExport
#include "sdom.h"

#ifdef ENABLE_DOM

#include "base.h"
#include "verts.h"
#include "tree.h"
#include "context.h"
#include "guard.h"
#include "platform.h"

#define getTmpList(v) (toV(v) -> getOwner().tmpList)

char* SDOM_ExceptionMsg[] = 
{
    // SDOM_OK
    (char *) "OK",
    
    // SDOM_INDEX_SIZE_ERR 1
    (char *) "list index out of bounds",
    
    // SDOM_DOMSTRING_SIZE_ERR 2
    (char *) "invalid size of string",

    // SDOM_HIERARCHY_REQUEST_ERR 3
    (char *) "hierarchy request error",
    
    // SDOM_WRONG_DOCUMENT_ERR 4
    (char *) "node used in a document that did not create it",
    
    // SDOM_INVALID_CHARACTER_ERR 5
    (char *) "invalid character in string",

    // SDOM_NO_DATA_ALLOWED_ERR 6 
    (char *) "data not allowed",

    // SDOM_NO_MODIFICATION_ALLOWED_ERR 7
    (char *) "modification not allowed",
    
    // SDOM_NOT_FOUND_ERR 8
    (char *) "reference to a non-existent node",

    // SDOM_NOT_FOUND_ERR 9
    (char *) "functionality not supported",

    // SDOM_INUSE_ATTRIBUTE_ERR 10
    (char *) "attribute is inuse",

    // SDOM_NOT_FOUND_ERR 10
    (char *) "attribute is inuse",
        
    // SDOM_INVALID_STATE_ERR 11
    (char *) "invalid state",

    // SDOM_SYNTAX_ERR 12
    (char *) "syntax error",

    // SDOM_INVALID_MODIFICATION_ERR 13
    (char *) "invalid modification",

    // SDOM_NAMESPACE_ERR 14
    (char *) "namespace error",

    // SDOM_INVALID_ACCESS_ERR 15
    (char *) "invalid access",

    // INVALID_NODE_TYPE
    (char *) "invalid node type",
    
    // SDOM_QUERY_PARSE_ERR
    (char *) "query parsing error",
    
    // SDOM_QUERY_EXECUTION_ERR
    (char *) "query execution error",
    
    // SDOM_NOT_OK
    (char *) "general exception"
};

#define SDOM_Err(SITUA, CODE) { SIT( SITUA ).setSDOMExceptionCode( CODE );\
    SIT( SITUA ).message(\
    MT_ERROR, E2_SDOM, CODE, SDOM_ExceptionMsg[CODE]); return CODE; }

#define SE(statement) {SDOM_Exception code__ = statement; \
    if (code__) return code__;}
    
#define SIT(PTR) (*(Situation*)PTR)

#define isXMLNS_(qn, resolver) \
    (qn.getPrefix() == resolver -> getOwner().stdPhrase(PHRASE_XMLNS) || \
        (qn.getPrefix() == UNDEF_PHRASE && \
	     qn.getLocal() == resolver -> getOwner().stdPhrase(PHRASE_XMLNS)))

//
//    globals
//

SDOM_NodeCallback* theDisposeCallback = NULL;
#define ownerDoc(NODE) ( &(toV(NODE)->getOwner().getRoot()) )

char* SDOM_newString(const Str& strg)
{
    int len = strg.length();
    char *p = new char[len + 1];
    strcpy(p, (char*)(const char*)strg);
    p[len] = 0;
    return p;
}
 
//_JP_ v
// helper functions for namespace handling
SDOM_Exception ___SDOM_swallowParentNSrec(SablotSituation s, SDOM_Node n, Tree* t, NSList* parentNs)
{
  // inherits from parentNs to begin of n->namespaces with respect to scoping
  // do it recursively for all child elements
    if ( isElement(toV(n)) ) {
      NSList* currentNs   = &(toE(n) -> namespaces);
      NmSpace* nm;
      const int imax = parentNs->number() - 1;
      for (int i = imax; i >= 0; i--) {
          nm = toNS((*parentNs)[i]);
	  if (currentNs -> findNdx(nm -> prefix) == -1) {
	    nm = new(&(NZ(t) -> getArena())) NmSpace(*t, nm -> prefix, nm -> uri, 
						     NSKIND_PARENT);
	    currentNs -> append(toV(nm));
	    nm -> parent = toV(n);
	  };
      };
      //zero in args => all appended namespaces will have kind == NSKIND_PARENT
      //parentNs->giveCurrent(SIT(s), *currentNs, t, 0);
      SDOM_Node checkedNode;
      SE( SDOM_getFirstChild(s, n, &checkedNode) );
      while (checkedNode) {
	SE( ___SDOM_swallowParentNSrec(s, checkedNode, t, currentNs) );
	SE( SDOM_getNextSibling(s, checkedNode, &checkedNode) );
      };
    };
    return SDOM_OK;
}

SDOM_Exception __SDOM_swallowParentNS(SablotSituation s, SDOM_Node n)
{
  // inherits from parent's NSList to begin of n->namespaces with respect to scoping
  // do it recursively for all child elements
    if ( n && isElement(toV(n)) && toE(n) -> parent ) {
      NSList* parentNs = &(toE(toE(n) -> parent) -> namespaces);
      Tree* t = toTree(ownerDoc(toE(n)));
      SE( ___SDOM_swallowParentNSrec(s, n, t, parentNs) );
    };
    return SDOM_OK;
}

SDOM_Exception __SDOM_dropParentNS(SablotSituation s, SDOM_Node n)
{
  // removes inherited namespaces from n->namespaces 
  // and from all child elements recursively
    if ( n && isElement(toV(n)) ) {
      NSList *checkedNs = &(toE(n) -> namespaces);
      NmSpace *nm;
      const int imax = checkedNs->number() - 1;
      for (int i = imax; i >= 0; i--) { 
	nm = toNS((*checkedNs)[i]);
	if ( nm -> usageCount == 0 && nm -> kind == NSKIND_PARENT ) {
	  checkedNs->rm(i);
	  nm -> parent = NULL;
	  //getTmpList(toE(n)).append(nm);
	  delete nm;
	};
      };
      SDOM_Node checkedNode;
      SE( SDOM_getFirstChild(s, n, &checkedNode) );
      while (checkedNode) {
	SE( __SDOM_dropParentNS(s, checkedNode) );
	SE( SDOM_getNextSibling(s, checkedNode, &checkedNode) );
      };
    };
    return SDOM_OK;
}

SDOM_Exception __SDOM_refreshNS(SablotSituation s, SDOM_Node n, NmSpace* nmspace)
{
  // appends or changes nmspace in all child elements recursively
  // with respect to scoping
    SDOM_Node checkedNode;
    Tree *t;
    NmSpace *nm;
    SE( SDOM_getFirstChild(s, n, &checkedNode) );
    while (checkedNode) {
      if ( isElement(checkedNode) ) {
	  nm = toE(checkedNode) -> namespaces.find(nmspace -> prefix);
	  if ( nm ) {
	      if ( nm -> usageCount == 0 && nm -> kind == NSKIND_PARENT ) {
		  nm -> uri = nmspace -> uri;
		  SE( __SDOM_refreshNS(s, checkedNode, nmspace) );
	      };
	  } else {
	      //t = toTree(ownerDoc(toE(n)));
	      t = &(toE(n) -> getOwner());
	      nm = new(&(t -> getArena())) NmSpace(*t, nmspace -> prefix, 
					               nmspace -> uri, 
						       NSKIND_PARENT);
	      toE(checkedNode) -> namespaces.append(nm);
	      nm -> setParent(toE(checkedNode));
	      SE( __SDOM_refreshNS(s, checkedNode, nmspace) );
	  };
      };
      SE( SDOM_getNextSibling(s, checkedNode, &checkedNode) );
    };    
    return SDOM_OK;
}

SDOM_Exception __SDOM_canChangeUriNS(SablotSituation s, SDOM_Node n, NmSpace* nmspace, const SDOM_char *uri)
{
  // order of tests has meaning
  if ( !n || nmspace->usageCount == 0 ) 
    // namespace without parent is changeable
    // not used namespace is changeable
      return SDOM_OK;
  
  Str olduri = toV(nmspace)->getOwner().expand(nmspace->uri);
  if ( !strcmp((char*)olduri,(char*)uri) ) 
      return SDOM_OK;// no change will be done

  return SDOM_NAMESPACE_ERR;
}


SDOM_Exception __SDOM_touchNS(SablotSituation s, SDOM_Node n, Phrase prefix, Phrase uri, NsKind kind, unsigned int maxUsageCount)
{
  //appends or changes n.namespaces to push prefix:uri into scope
  //potentially refreshes childrens' namespaces
  //suppose if kind == NSKIND_DECLARED then nm will NOT be immediatelly used
  //suppose if kind == NSKIND_PARENT then nm will be immediatelly used 
  NmSpace *nm = toE(n)->namespaces.find(prefix);
  if (nm) {
    //nm->prefix == prefix
    if (nm->uri == uri) {
      if (kind == NSKIND_PARENT) nm -> usageCount++;
      else nm -> kind = kind;
    } else {
      if ( kind == NSKIND_PARENT ) {
	if (nm -> kind == NSKIND_PARENT && nm -> usageCount <= maxUsageCount) {
	  nm -> uri = uri;
	  return __SDOM_refreshNS(s, n, nm);
	} else {
	  return SDOM_NAMESPACE_ERR;//_JP_ not in spec - namespace collision
	};
      } else {
	if (nm -> usageCount <= maxUsageCount) {
	  nm -> kind = kind;
	  nm -> uri = uri;
	  return __SDOM_refreshNS(s, n, nm);
	} else {
	  return SDOM_NAMESPACE_ERR;//_JP_ not in spec - namespace collision
	};	
      };
    };
  } else {
    //nm with prefix not exists
    //!!chybka
	//Tree *auxt = NZ(toTree(ownerDoc(toE(n))));
	Tree *auxt = &(toE(n) -> getOwner());
    nm = new(&(auxt -> getArena())) 
            NmSpace(toE(n) -> getOwner(),prefix, uri, kind);
    if (kind == NSKIND_PARENT) nm -> usageCount = 1;
    toE(n) -> namespaces.append(nm);
    toV(nm) -> setParent(toE(n));
  };
  return SDOM_OK;
}


SDOM_Exception __SDOM_touchNSByChar(SablotSituation s, SDOM_Node n, SDOM_char* prefix, SDOM_char* uri, NsKind kind, int maxUsageCount)
{
  //appends or changes n.namespaces to push prefix:uri into scope
  //potentially refreshes childrens' namespaces
  Phrase ph_prefix = prefix && strcmp((char*)prefix,"xmlns") ? toE(ownerDoc(toE(n)))->dict().insert((const char*)prefix) : UNDEF_PHRASE;
  Phrase ph_uri = uri ? toE(ownerDoc(toE(n)))->dict().insert((const char*)uri) : UNDEF_PHRASE;
  return __SDOM_touchNS(s, n, ph_prefix, ph_uri, kind, maxUsageCount);
}


//_JP_ ^


SDOM_Exception SDOM_createElement(SablotSituation s, SDOM_Document d, SDOM_Node *pn, const SDOM_char *tagName)
{
    QName q;
    //try to find the document element
    Element *e = NULL;
    for (int i = 0; i < toRoot(d) -> contents.number(); i++)
      {
	if (isElement(toRoot(d) -> contents[i]))
	  {
	    e = toE(toRoot(d) -> contents[i]);
	    break;
	  }
      }
    if (!e) e = toRoot(d);

    e -> setLogical(SIT(s), q, tagName, TRUE);
    *pn = new(&(toTree(d) -> getArena())) Element(*toTree(d), q);
    //_TH_ v
    getTmpList(d).append(*pn);
    //_TH_ ^
    return SDOM_OK;
}

//_JP_ v
SDOM_Exception SDOM_createElementNS(SablotSituation s, SDOM_Document d, SDOM_Node *pn, const SDOM_char *uri, const SDOM_char *qName)
{
    if ( !isValidQName((char*)qName) )
        return SDOM_INVALID_CHARACTER_ERR;

    Str prefix = Str();
    const char *colon = strchr((char*)qName,':');
    QName q;

    if ( colon ) { //qName has prefix

        if ( !uri ) 
	    return SDOM_NAMESPACE_ERR;  

	prefix.nset((char*)qName, intPtrDiff(colon, qName));

	if ( !strcmp((const char*)prefix,"xml") && strcmp(theXMLNamespace,uri) )
	    //redefinition of xml-namespace
	    return SDOM_NAMESPACE_ERR;

	q.setPrefix(toE(d)->dict().insert((const char*)prefix));
	q.setLocal(toE(d)->dict().insert((const char*)((ptrdiff_t)colon + 1)));
	q.setUri(toE(d)->dict().insert((const char*)uri));

    } else {

        q.setPrefix(UNDEF_PHRASE);
	q.setLocal(toE(d)->dict().insert((const char*)qName));

	if ( uri && strcmp((char*)uri,"") ) {
	    q.setUri(toE(d)->dict().insert((const char*)uri));
	} else {
	    q.setUri(UNDEF_PHRASE);
	};    
    };

    *pn = new(&(toTree(d) -> getArena())) Element(*toTree(d), q);
    // no parent, no childs, no attributes => only 1 namespace:
    NmSpace* newNS = new(&(toTree(d) -> getArena())) NmSpace(*toTree(d),
							     q.getPrefix(), 
							     q.getUri(), 
							     NSKIND_DECLARED);
    newNS -> usageCount = 1;
    toE(*pn) -> namespaces.append(newNS);
    toV(newNS) -> setParent(toE(*pn));
    //_TH_ v
    getTmpList(d).append(*pn);
    //_TH_ ^
    return SDOM_OK;
}
//_JP_ ^

SDOM_Exception _SDOM_createAttributeWithParent(SablotSituation s, SDOM_Document d, SDOM_Node *pn, const SDOM_char *name, SDOM_Node parent)
{
    QName q;
    //_JP_ v
    // uses parent's NSList, if exists
    if ( parent )
        toE(parent) -> setLogical(SIT(s), q, name, FALSE);
    else
        toRoot(d) -> setLogical(SIT(s), q, name, FALSE);
    //_JP_ ^ 
    if ( !isXMLNS_(q, toRoot(d)) ) {
        *pn = new(&(toTree(d) -> getArena())) 
	    Attribute(*toTree(d), q, (char*)"", XSLA_NONE);
	if (parent) 
	    toE(parent)->namespaces.incPrefixUsage(q.getPrefix());
    } else
        *pn = new(&(toTree(d) -> getArena())) 
	    NmSpace(*toTree(d), 
		    q.getPrefix() == UNDEF_PHRASE ? UNDEF_PHRASE : q.getLocal(),
		    UNDEF_PHRASE,
		    NSKIND_DECLARED);
    //_TH_ v
    getTmpList(d).append(*pn);
    //_TH_ ^
    return SDOM_OK;
}

SDOM_Exception SDOM_createAttribute(SablotSituation s, SDOM_Document d, SDOM_Node *pn, const SDOM_char *name)
{
    return _SDOM_createAttributeWithParent(s, d, pn, name, NULL);
}

//_JP_ v
SDOM_Exception SDOM_createAttributeNS(SablotSituation s, SDOM_Document d, SDOM_Node *pn, const SDOM_char *uri, const SDOM_char *qName)
{
    if ( !isValidQName((char*)qName) )
        return SDOM_INVALID_CHARACTER_ERR;

    Str prefix = Str();
    const char *colon = strchr((char*)qName,':');
    QName q;

    if ( colon ) { //qName has prefix

        if ( !uri )
	    return SDOM_NAMESPACE_ERR;

	prefix.nset((char*)qName, intPtrDiff(colon, qName));

	if ( !strcmp((const char*)prefix,"xml") && strcmp(theXMLNamespace,uri) )
	    //redefinition of xml namespace
	    return SDOM_NAMESPACE_ERR;

	if ( !strcmp((const char*)prefix,"xmlns") && strcmp(theXMLNSNamespace,uri) )
	    //redefinition of xmlns namespace
	    return SDOM_NAMESPACE_ERR;

	q.setPrefix(toE(d)->dict().insert((const char*)prefix));
	q.setLocal(toE(d)->dict().insert((const char*)((ptrdiff_t)colon + 1)));
	q.setUri(toE(d)->dict().insert((const char*)uri));

    } else {

        q.setPrefix(UNDEF_PHRASE);
	q.setLocal(toE(d)->dict().insert((const char*)qName));
	if ( uri && strcmp((char*)uri,"") ) {
	    q.setUri(toE(d)->dict().insert((const char*)uri));
	} else {
	    q.setUri(UNDEF_PHRASE);
	};    
    };

    if ( !isXMLNS_(q, toRoot(d)) )
        *pn = new(&(toTree(d) -> getArena())) 
	    Attribute(*toTree(d), q, (char*)"", XSLA_NONE);
    else
        *pn = new(&(toTree(d) -> getArena())) 
	    NmSpace(*toTree(d), 
		    q.getLocal(),
		    UNDEF_PHRASE,
		    NSKIND_DECLARED);

    //_TH_ v
    getTmpList(d).append(*pn);
    //_TH_ ^
    return SDOM_OK;
}
//_JP_ ^

SDOM_Exception SDOM_createTextNode(SablotSituation s, SDOM_Document d, SDOM_Node *pn, const SDOM_char *data)
{
    *pn = new(&(toTree(d) -> getArena())) Text(*toTree(d), (char*) data);
    //_TH_ v
    getTmpList(d).append(*pn);
    //_TH_ ^
    return SDOM_OK;
}

SDOM_Exception SDOM_createCDATASection(SablotSituation s, SDOM_Document d, SDOM_Node *pn, const SDOM_char *data)
{
    SE( SDOM_createTextNode(s, d, pn, data) );
    toText(toV(*pn)) -> beCDATA();
    return SDOM_OK;
}

SDOM_Exception SDOM_createComment(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn, 
    const SDOM_char *data)
{
    *pn = new(&(toTree(d) -> getArena())) Comment(*toTree(d), (char*) data);
    //_TH_ v
    getTmpList(d).append(*pn);
    //_TH_ ^
    return SDOM_OK;
}

SDOM_Exception SDOM_createProcessingInstruction(
    SablotSituation s, 
    SDOM_Document d, 
    SDOM_Node *pn, 
    const SDOM_char *target,
    const SDOM_char *data)
{
    *pn = new(&(toTree(d) -> getArena())) ProcInstr(
        *toTree(d), 
        toTree(d) -> unexpand((char*) target), 
	(char*) data);
    //_TH_ v
    getTmpList(d).append(*pn);
    //_TH_ ^
    return SDOM_OK;
}

SDOM_Exception SDOM_disposeNode(SablotSituation s, SDOM_Node n)
{
    Vertex *v = toV(n);
    switch(v -> vt & VT_BASE)
    {
        case VT_ELEMENT:
	        ccdelete(v, toE(v)); break;
	    case VT_ATTRIBUTE:
	        ccdelete(v, toA(v)); break;
	    case VT_NAMESPACE:
	        ccdelete(v, toNS(v)); break;
	    case VT_PI:
	        ccdelete(v, toPI(v)); break;
	    case VT_TEXT: // and CDATA
	        ccdelete(v, toText(v)); break;            	        
	    case VT_XSL:
	        ccdelete(v, toX(v)); break;
	    case VT_COMMENT:
	        ccdelete(v, toComment(v)); break;
		default:
		    sabassert(!"disposeSDOM_Node");	    
    }
    return SDOM_OK;
}

SDOM_Exception SDOM_getNodeType(SablotSituation s, SDOM_Node n, SDOM_NodeType *pType)
{
    Vertex *v = toV(n);
    switch(basetype(v))
    {
        case VT_ROOT:
	        *pType = SDOM_DOCUMENT_NODE; break;
        case VT_ELEMENT:
	        *pType = SDOM_ELEMENT_NODE; break;
	    case VT_ATTRIBUTE:
	        *pType = SDOM_ATTRIBUTE_NODE; break;
	    case VT_TEXT:
	    {
	        if (toText(v) -> isCDATA())
		        *pType = SDOM_CDATA_SECTION_NODE;
			else
	            *pType = SDOM_TEXT_NODE; 
		}; break;
	    case VT_COMMENT:
	        *pType = SDOM_COMMENT_NODE; break;
		case VT_PI:
		    *pType = SDOM_PROCESSING_INSTRUCTION_NODE; break;
		case VT_NAMESPACE:
		  *pType = SDOM_ATTRIBUTE_NODE; break;
		default:
		    *pType = SDOM_OTHER_NODE;
	}
	return SDOM_OK;
}

SDOM_Exception SDOM_getNodeName(SablotSituation s, SDOM_Node n, SDOM_char **pName)
{
    Str fullName;
    Vertex *v = toV(n);
    switch(v -> vt & VT_BASE)
    {
    case VT_ELEMENT:
    {
	v -> getOwner().expandQStr(toE(v) -> getName(), fullName);
	*pName = SDOM_newString(fullName);
    };
    break;
    case VT_ATTRIBUTE:
    {
	v -> getOwner().expandQStr(toA(v) -> getName(), fullName);
	*pName = SDOM_newString(fullName);
    };
    break;
    case VT_NAMESPACE:
    {
	v -> getOwner().expandQStr(toNS(v) -> getName(), fullName);
	DStr fullName2 = "xmlns";
	if ( strcmp(fullName,"") ) {
	  fullName2 += ":";
	  fullName2 += fullName;
	}
	*pName = SDOM_newString(fullName2);
    };
    break;
    case VT_TEXT: // and CDATA
    {
	if (toText(v) -> isCDATA())
	    *pName = SDOM_newString("#cdata-section");
	else
	    *pName = SDOM_newString("#text"); 
    }; break;		
    case VT_COMMENT:
	*pName = SDOM_newString("#comment"); break;
    case VT_ROOT:
	*pName = SDOM_newString("#document"); break;		    
    case VT_PI:
	*pName = SDOM_newString(toPI(v) -> getOwner().expand(toPI(v) -> getName().getLocal())); break;		
	    
    default:
	*pName = NULL;
    }
    return SDOM_OK;
}

SDOM_Exception SDOM_getNodeNSUri(SablotSituation s, SDOM_Node n, SDOM_char **pName)
{
    Str value;
    Vertex *v = toV(n);
    switch(v -> vt & VT_BASE)
    {
    case VT_ELEMENT:
    {
	value = v -> getOwner().expand(toE(v) -> getName().getUri());
	*pName = SDOM_newString(value);
    };
    break;
    case VT_ATTRIBUTE:
    {
	value = v -> getOwner().expand(toA(v) -> getName().getUri());
	*pName = SDOM_newString(value);
    };
    break;
    case VT_NAMESPACE:
    {
      *pName = SDOM_newString(theXMLNSNamespace);
    };
    break;
    default:
	*pName = NULL;
    }
    return SDOM_OK;
}

SDOM_Exception SDOM_getNodePrefix(SablotSituation s, SDOM_Node n, SDOM_char **pName)
{
    Str value;
    Vertex *v = toV(n);
    switch(v -> vt & VT_BASE)
    {
    case VT_ELEMENT:
    {
	value = v -> getOwner().expand(toE(v) -> getName().getPrefix());
	*pName = SDOM_newString(value);
    };
    break;
    case VT_ATTRIBUTE:
    {
	value = v -> getOwner().expand(toA(v) -> getName().getPrefix());
	*pName = SDOM_newString(value);
    };
    break;
    case VT_NAMESPACE:
    {
	v -> getOwner().expandQStr(toNS(v) -> getName(), value);
	if ( !strcmp(value,"") ) {
	  *pName = SDOM_newString("");
	} else {
	  *pName = SDOM_newString("xmlns");
	};
    };
    break;
    default:
	*pName = NULL;
    }
    return SDOM_OK;
}

SDOM_Exception SDOM_getNodeLocalName(SablotSituation s, SDOM_Node n, SDOM_char **pName)
{
    Str value;
    Vertex *v = toV(n);
    switch(v -> vt & VT_BASE)
    {
    case VT_ELEMENT:
    {
	value = v -> getOwner().expand(toE(v) -> getName().getLocal());
	*pName = SDOM_newString(value);
    };
    break;
    case VT_ATTRIBUTE:
    {
	value = v -> getOwner().expand(toA(v) -> getName().getLocal());
	*pName = SDOM_newString(value);
    };
    break;
    case VT_NAMESPACE:
    {
	v -> getOwner().expandQStr(toNS(v) -> getName(), value);
	if ( !strcmp(value,"") ) {
	  *pName = SDOM_newString("xmlns");
	} else {
	  value = v -> getOwner().expand(toNS(v) -> getName().getLocal());
	  *pName = SDOM_newString(value);
	};

	
    };
    break;
    default:
	*pName = NULL;
    }
    return SDOM_OK;
}

SDOM_Exception SDOM_setNodeName(SablotSituation s, SDOM_Node n, const SDOM_char *name)
{
    Vertex *v = toV(n);
    QName q;
    if (isRoot(v))
        SDOM_Err(s, SDOM_NO_MODIFICATION_ALLOWED_ERR);
    switch (v -> vt & VT_BASE) 
    {
    case VT_ELEMENT:
	toE(v) -> setLogical(SIT(s), q, name, TRUE);
	break;
    default:
    {
	if (v -> parent)
	    toE(v -> parent) -> setLogical(SIT(s), q, name, FALSE);
	else
	    v -> getOwner().getRoot().setLogical(SIT(s), q, name, FALSE);
    }
    }

    switch(v -> vt & VT_BASE)
    {
    case VT_ELEMENT:
        if ( q.getPrefix() != UNDEF_PHRASE && !toE(v) -> namespaces.find(q.getPrefix()) )
	  SDOM_Err(s, SDOM_NAMESPACE_ERR);
        toE(v) -> namespaces.decPrefixUsage(toE(v) -> name.getPrefix());
	toE(v) -> name = q;
	toE(v) -> namespaces.incPrefixUsage(q.getPrefix());
	break;
    case VT_ATTRIBUTE:
          if ( toA(v) -> parent ) {
	    Phrase curPrefix = toA(v) -> name.getPrefix(); 
	    Phrase newPrefix = q.getPrefix(); 
	    if ( (newPrefix != UNDEF_PHRASE 
		  && !toE(toA(v) -> parent) -> namespaces.find(newPrefix))
		 || 
		 (newPrefix == toA(v)-> getOwner().stdPhrase(PHRASE_XMLNS) 
		  || !strcmp(name,"xmlns")) ){
	      //attempt to change attribute to namespace
	      SDOM_Err(s, SDOM_NAMESPACE_ERR);
	    };
	    if ( curPrefix != UNDEF_PHRASE)
	      toE(toA(v) -> parent) -> namespaces.decPrefixUsage(curPrefix);
	    toA(v) -> name = q;
	    if ( newPrefix != UNDEF_PHRASE)
	      toE(toA(v) -> parent) -> namespaces.incPrefixUsage(newPrefix);
	  } else 
	    toA(v) -> name = q;
	  break;
    case VT_NAMESPACE:
    {
      if ( strcmp(name,"xmlns") ) {
	if ( !(q.getPrefix() == toNS(v)-> getOwner().stdPhrase(PHRASE_XMLNS)) ) {
	  //attempt to change namespace to attribute
	  SDOM_Err(s, SDOM_NAMESPACE_ERR);
	};
      } else {
	//default namespace
	q.setLocal(UNDEF_PHRASE);
      };
      q.setPrefix(UNDEF_PHRASE);
      if ( !(toNS(v) -> name == q) ) {
	if ( toNS(v) -> usageCount != 0 ) {
	  SDOM_Err(s, SDOM_NO_MODIFICATION_ALLOWED_ERR);
	} else {
	  toNS(v) -> prefix = q.getLocal();
	  toNS(v) -> name = q;
	};
      };
    }; break;
    case VT_PI:
	toPI(v) -> name = q;
	break;
    default:
	SDOM_Err(s, SDOM_NO_MODIFICATION_ALLOWED_ERR);
    }
    return SDOM_OK;
}

SDOM_Exception SDOM_getNodeValue(SablotSituation s, SDOM_Node n, SDOM_char **pValue)
{
    Vertex *v = toV(n);
    switch(v -> vt & VT_BASE)
    {
	    case VT_ATTRIBUTE:
	        *pValue = SDOM_newString(toA(v) -> cont);
		    break;
	    case VT_NAMESPACE:
	        *pValue = SDOM_newString(v -> getOwner().expand(toNS(v) -> uri));
		    break;
	    case VT_TEXT: // and CDATA section
	        *pValue = SDOM_newString(toText(v) -> cont);
		    break;
	    case VT_COMMENT:
	        *pValue = SDOM_newString(toComment(v) -> cont);
		    break;
		case VT_PI:
	        *pValue = SDOM_newString(toPI(v) -> cont);
		    break;
		default:
		    // element and document (root) have void value
		    *pValue = NULL;
	}
	return SDOM_OK;
}

SDOM_Exception SDOM_setNodeValue(SablotSituation s, SDOM_Node n, const SDOM_char *value)
{
    Vertex *v = toV(n);
    switch(v -> vt & VT_BASE)
    {
	    case VT_ATTRIBUTE:
	        toA(v) -> cont = value;
		    break;
	    case VT_NAMESPACE:
	        SE( __SDOM_canChangeUriNS(s, toNS(v) -> parent, toNS(v), value) );
	        toNS(v) -> uri = v -> getOwner().unexpand(value);
		    break;
	    case VT_TEXT: // and CDATA section
	        toText(v) -> cont = value;
		    break;
	    case VT_COMMENT:
	        toComment(v) -> cont = value;
		    break;
		case VT_PI:
	        toPI(v) -> cont = value;
		    break;
		default:
		    // element and document (root) have void value
		    SDOM_Err(s, SDOM_NO_MODIFICATION_ALLOWED_ERR);
	}
	return SDOM_OK;
}

SDOM_Exception SDOM_getParentNode(SablotSituation s, SDOM_Node n, SDOM_Node *pParent)
{
    Vertex *v = toV(n);
    if (isRoot(v) || isAttr(v) || isNS(v))
        *pParent = NULL;
	else
	    *pParent = v -> parent;
	return SDOM_OK;
}

SDOM_Exception SDOM_getFirstChild(SablotSituation s, SDOM_Node n, SDOM_Node *pFirstChild)
{
    Vertex *v = toV(n);
    if (!isElement(v) && !isRoot(v))
        *pFirstChild = NULL;
	else
	{
	    int childCount = toE(v) -> contents.number();
	    if (childCount)
	        *pFirstChild = (SDOM_Node)(toE(v) -> contents[0]);
		else
		    *pFirstChild = NULL;
	}
	return SDOM_OK;
}

SDOM_Exception SDOM_getLastChild(SablotSituation s, SDOM_Node n, SDOM_Node *pLastChild)
{
    Vertex *v = toV(n);
    if (!isElement(v) && !isRoot(v))
        *pLastChild = NULL;
	else
	{
	    int childCount = toE(v) -> contents.number();
	    if (childCount)
	        *pLastChild = (SDOM_Node)(toE(v) -> contents.last());
		else
		    *pLastChild = NULL;
	}
	return SDOM_OK;
}

SDOM_Exception SDOM_getPreviousSibling(SablotSituation s, SDOM_Node n, SDOM_Node *pPreviousSibling)
{
    switch(toV(n) -> vt & VT_BASE)
    {
        case VT_ATTRIBUTE:
	    case VT_NAMESPACE:
        case VT_ROOT:	            
	        *pPreviousSibling = NULL;
		    break;
        default:
            *pPreviousSibling = toV(n) -> getPreviousSibling();
	}
    return SDOM_OK;
}

SDOM_Exception SDOM_getNextSibling(SablotSituation s, SDOM_Node n, SDOM_Node *pNextSibling)
{
    switch(toV(n) -> vt & VT_BASE)
    {
        case VT_ATTRIBUTE:
    	case VT_NAMESPACE:    
        case VT_ROOT:	            
	        *pNextSibling = NULL;
		    break;
        default:
            *pNextSibling = toV(n) -> getNextSibling();
	}
    return SDOM_OK;
}

SDOM_Exception SDOM_getChildNodeIndex(SablotSituation s, SDOM_Node n, int index, SDOM_Node *pChildNode)
{
    Vertex *v = toV(n);
    if ( (!isElement(v) && !isRoot(v))
	 || index < 0 
	 || index >= toE(v) -> contents.number() )
        *pChildNode = NULL;  
    else
	*pChildNode = (SDOM_Node)(toE(n) -> contents[index]);
    return SDOM_OK;
}

SDOM_Exception SDOM_getChildNodeCount(SablotSituation s, SDOM_Node n, int *count)
{
    Vertex *v = toV(n);
    if ( !isElement(v) && !isRoot(v) )
        *count = 0;  
    else
	*count = toE(v) -> contents.number();
    return SDOM_OK;
}

SDOM_Exception SDOM_getOwnerDocument(SablotSituation s, SDOM_Node n, SDOM_Document *pOwnerDocument)
{
    if (isRoot(toV(n)))
        *pOwnerDocument = NULL;
	else
        *pOwnerDocument = ownerDoc(n);
    return SDOM_OK;
}

Bool hasElementChild(RootNode *r)
{
    for (int i = 0; i < r -> contents.number(); i++)
        if (isElement(r -> contents[i]))
	  return TRUE;
    return FALSE;
}

// is first ancestor of second?
Bool isAncestor(Vertex *first, Vertex *second)
{
    for (Vertex *p = second; p; p = p -> parent)
        if (p == first) return TRUE;
	return FALSE;
}

SDOM_Exception SDOM_insertBefore(SablotSituation s, SDOM_Node n, SDOM_Node newChild, SDOM_Node refChild)
{
    Vertex *v = toV(n);
    
    // check if v is an element (or root)
    if (!isElement(v))
        SDOM_Err(s, SDOM_HIERARCHY_REQUEST_ERR);
	
	// check the type of newChild
	if (!newChild)
	    SDOM_Err(s, SDOM_NOT_FOUND_ERR)
	else
	    switch(basetype(newChild))
    	{
    	    case VT_ATTRIBUTE:
    	    case VT_NAMESPACE:
    	    case VT_ROOT:
    	        SDOM_Err(s, SDOM_HIERARCHY_REQUEST_ERR); 	    
    	}    
	// check if newChild is from the same doc
	if ((isRoot(v) && ownerDoc(newChild) != v) ||
	    (!isRoot(v) && ownerDoc(v) != ownerDoc(newChild)))
	    SDOM_Err(s, SDOM_WRONG_DOCUMENT_ERR);
	
	// check type of the reference child
	if (refChild)
	    switch(toV(refChild) -> vt & VT_BASE)
    	{
    	    case VT_ATTRIBUTE:
    	    case VT_NAMESPACE:
    	    case VT_ROOT:
    	        SDOM_Err(s, SDOM_HIERARCHY_REQUEST_ERR); 	    
    	}
	
	// check if newChild is not an ancestor of n
	if (isAncestor(toV(newChild), toV(n)))
	    SDOM_Err(s, SDOM_HIERARCHY_REQUEST_ERR);
	    
	// check if not attempting to have more doc elements
	if (isRoot(v) && isElement(newChild) && hasElementChild(toRoot(v)))
	    SDOM_Err(s, SDOM_HIERARCHY_REQUEST_ERR);
	    
	// see if newChild needs to be removed from tree
	Vertex *parent;
	if (NULL != (parent = toV(newChild) -> parent))
	    SE( SDOM_removeChild(s, parent, newChild) );
	//_JP_ v
// 	// v ??? 
// 	int ndx = toE(v) -> contents.getIndex(toV(newChild));
// 	if (ndx != -1)
// 	    toE(v) -> contents.rm(ndx);
// 	// ^ ???
// 	//_TH_ v
// 	getTmpList(n).rmP(newChild);
// 	//_TH_ ^
 	int ndx = toE(v) -> contents.getIndex(toV(newChild));
 	if (ndx != -1)
 	    toE(v) -> contents.rm(ndx);
	else
	    getTmpList(n).rmP(newChild);
	//_JP_ ^
	if (refChild)
	{
    	ndx = toE(v) -> contents.getIndex(toV(refChild));
	    if (ndx == -1)
	        SDOM_Err(s, SDOM_NOT_FOUND_ERR);
	    toE(v) -> contents.insertBefore(toV(newChild), ndx);
	}
	else
	    toE(v) -> contents.append(toV(newChild));
	    // toE(v) -> contents.appendAndSetOrdinal(toV(newChild));
	toV(newChild) -> setParent(v);
	//_JP_ v
	//old_parent's not used namespaces was cleaned by SDOM_removeChild
	SE( __SDOM_swallowParentNS(s, newChild) );
	//_JP_ ^
	return SDOM_OK;
}

SDOM_Exception SDOM_removeChild(SablotSituation s, SDOM_Node n, SDOM_Node oldChild)
{
    Vertex *v = toV(n);
    if (!isElement(v))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
	switch(toV(oldChild) -> vt & VT_BASE)
    	{
    	    case VT_ATTRIBUTE:
    	    case VT_NAMESPACE:
    	    case VT_ROOT:
    	        SDOM_Err(s, SDOM_INVALID_NODE_TYPE); 	    
    	}
	if (toV(oldChild) -> parent != toV(n))
	    SDOM_Err(s, SDOM_NOT_FOUND_ERR);
	//_JP_ v
	//SE( __SDOM_dropParentNS(s, oldChild) );
	//_JP_ ^
	toE(v) -> removeChild(toV(oldChild));
	//_TH_ v
	getTmpList(n).append(oldChild);
	//_TH_ ^
    return SDOM_OK;
}

SDOM_Exception SDOM_replaceChild(SablotSituation s, SDOM_Node n, SDOM_Node newChild, SDOM_Node oldChild)
{
    SDOM_Node tmp;
    SE( SDOM_getParentNode(s, newChild, &tmp) );
    if (tmp) SE( SDOM_removeChild(s, tmp, newChild) );
    
    SE( SDOM_getNextSibling(s, oldChild, &tmp) );
    SE( SDOM_removeChild(s, n, oldChild) );
    SE( SDOM_insertBefore(s, n, newChild, tmp) );
    
    //_JP_ old version not satisfying DOM spec:
    //SE( SDOM_insertBefore(s, n, newChild, oldChild) );
    //SE( SDOM_removeChild(s, n, oldChild) );

    return SDOM_OK;
}

SDOM_Exception SDOM_appendChild(SablotSituation s, SDOM_Node n, SDOM_Node newChild)
{
    return SDOM_insertBefore(s, n, newChild, NULL);
}

SDOM_Exception cloneVertex(SablotSituation, Tree *, Vertex *, int, Vertex **);

SDOM_Exception cloneVertexList(SablotSituation s, Tree *t, VertexList *vlForeign, int deep, Element *tParent)
{
    Vertex *newVertex;
    for (int i = 0; i < vlForeign -> number(); i++)
    {
        SE( cloneVertex(s, t, (*vlForeign)[i], deep, &newVertex) );
	    // the following also handles atts and namespaces correctly
	    tParent -> newChild(SIT(s), newVertex);    	    
    }   
    return SDOM_OK;
}

SDOM_Exception cloneVertex(SablotSituation s, Tree *t, Vertex *foreign, int deep, Vertex **clone)
{
    Tree *tForeign = &(foreign -> getOwner());
    QName q;
    EQName expanded;

	if (basetype(foreign) == VT_ROOT) 
	    SDOM_Err(s, SDOM_INVALID_NODE_TYPE);

	// get the correctly unexpanded name	
	if (basetype(foreign) == VT_ELEMENT || 
	    basetype(foreign) == VT_ATTRIBUTE ||
	    basetype(foreign) == VT_PI ||
	    basetype(foreign) == VT_NAMESPACE)
	{	
        switch(basetype(foreign))
        {
            case VT_ELEMENT:
    		    tForeign -> expandQ(toE(foreign) -> getName(), expanded);
    		    break;
    	    case VT_ATTRIBUTE:
    		    tForeign -> expandQ(toA(foreign) -> getName(), expanded);
    		    break;
    	    case VT_NAMESPACE:
    		    tForeign -> expandQ(toNS(foreign) -> getName(), expanded);
    		    break;
    		case VT_PI:
    		    tForeign -> expandQ(toPI(foreign) -> getName(), expanded);
    		    break;
    	};
	    q.setLocal(t -> unexpand(expanded.getLocal()));
	    q.setPrefix(t -> unexpand(expanded.getPrefix()));
	    q.setUri(t -> unexpand(expanded.getUri()));
    }

    // create the actual copy
    switch(basetype(foreign))
    {
        case VT_ELEMENT:
            *clone = new(&(t -> getArena())) 
	            Element(*t, q);
	        break;
	    case VT_ATTRIBUTE:
            *clone = new(&(t -> getArena())) 
	            Attribute(*t, q, toA(foreign) -> cont, XSLA_NONE);
		    break;
		case VT_NAMESPACE:
            *clone = new(&(t -> getArena())) NmSpace(*t, 
	            t -> unexpand(tForeign -> expand(toNS(foreign) -> prefix)),
		        t -> unexpand(tForeign -> expand(toNS(foreign) -> uri)),
			toNS(foreign) -> kind);
	    // _JP_ v
	    toNS(*clone) -> usageCount = toNS(foreign) -> usageCount;
	    // _JP_ ^
		    break;
		case VT_PI:
            *clone = new(&(t -> getArena())) ProcInstr(*t, q.getLocal(), toPI(foreign) -> cont);
		    break;
	    case VT_COMMENT:
            *clone = new(&(t -> getArena())) Comment(*t, toComment(foreign) -> cont);
	            break;
	    case VT_TEXT:
	        {
	            *clone = new(&(t -> getArena())) Text(*t, toText(foreign) -> cont);	        		
    	        if (toText(foreign) -> isCDATA())		
                    toText(*clone) -> beCDATA();
			}; break;
    }
    
    if (isElement(foreign))
    {
        // must clone atts and namespaces in any case
	    sabassert(isElement(*clone));
        cloneVertexList(s, t, &(toE(foreign) -> atts), deep, toE(*clone));
        cloneVertexList(s, t, &(toE(foreign) -> namespaces), deep, toE(*clone));
	    
	    // if deep then recurse
	    if (deep)
            cloneVertexList(s, t, &(toE(foreign) -> contents), deep, toE(*clone));        
	}
    return SDOM_OK;
}

SDOM_Exception SDOM_cloneForeignNode(SablotSituation s, SDOM_Document d, SDOM_Node n, int deep, SDOM_Node *clone)
{
  SDOM_Exception se = cloneVertex(s, toTree(d), toV(n), deep, (Vertex**) clone);
    //_JP_ v
  se = (se != SDOM_OK) ? se : __SDOM_dropParentNS(s, *clone);
    //_JP_ ^
    //_TH_ v
  getTmpList(d).append(*clone);
    //_TH_ ^
  return se;
}

SDOM_Exception SDOM_cloneNode(SablotSituation s, SDOM_Node n, int deep, SDOM_Node *clone)
{
    return SDOM_cloneForeignNode(s, ownerDoc(n), n, deep, clone);
}

SDOM_Exception SDOM_getAttribute(SablotSituation s, SDOM_Node n, const SDOM_char *name, SDOM_char **pValue)
{
    QName q;
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));

    e -> setLogical(SIT(s), q, name, FALSE);
    Vertex *v = NULL;
    Bool isXMLNS = isXMLNS_(q, e);
    if (!isXMLNS)
        v = e -> atts.find(q);
	else
	    v = e -> namespaces.find(
	        q.getLocal() != UNDEF_PHRASE ? q.getLocal() : UNDEF_PHRASE);
    if (!v)
        *pValue = SDOM_newString("");
	else
	{
	    if (!isXMLNS)
	      *pValue = SDOM_newString(toA(v) -> cont);
	    else
	      *pValue = SDOM_newString(e -> getOwner().expand(toNS(v) -> uri));
	}
    return SDOM_OK;	    
}

SDOM_Exception SDOM_getAttributeNS(SablotSituation s, SDOM_Node n, SDOM_char *uri, SDOM_char *local, SDOM_char **pValue)
{
    QName q;
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));

    Vertex *v = NULL;
    Bool isXMLNS = ( !strcmp((char*)uri,theXMLNSNamespace) );
    if ( !isXMLNS ) {
        const int attcount = e -> atts.number();
	for (int i = 0; i < attcount; i++) {
	  q = toA(e->atts[i]) -> name;
	  if (!strcmp((char*)local, (char*)(e -> getOwner().expand(q.getLocal())))
	      && !strcmp((char*)uri, (char*)(e -> getOwner().expand(q.getUri())))) {
	    v = e -> atts[i];
	    break;
	  };
	};
    } else {
        const int nscount = e -> namespaces.number();
	for (int i = 0; i < nscount; i++) {
	  q = toNS(e->namespaces[i]) -> name;
	  if (!strcmp((char*)local, (char*)(e -> getOwner().expand(toNS(e->namespaces[i])->prefix)))) {
	    v = e -> namespaces[i];
	    break;
	  };
	};
    }
    if (!v)
        *pValue = SDOM_newString("");
	else
	{
	    if (!isXMLNS)
	      *pValue = SDOM_newString(toA(v) -> cont);
	    else
	      *pValue = SDOM_newString(e -> getOwner().expand(toNS(v) -> uri));
	}
    return SDOM_OK;	    
}

SDOM_Exception SDOM_getAttributeNode(SablotSituation s, SDOM_Node n, const SDOM_char *name, SDOM_Node *attr)
{
    QName q;
    if ( !isElement(toV(n)) )
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));

    e -> setLogical(SIT(s), q, name, FALSE);
    Vertex *v = NULL;
    Bool isXMLNS = isXMLNS_(q, e);
    if (!isXMLNS)
        v = e -> atts.find(q);
	else
	  {
	    v = e -> namespaces.find(
			q.getLocal() != e -> getOwner().stdPhrase(PHRASE_XMLNS)? 
				     q.getLocal() : UNDEF_PHRASE);
	  }
    if (!v)
        *attr = 0;
    else
      {
	*attr = (SDOM_Node)v;
      }
    return SDOM_OK;	    
}

SDOM_Exception SDOM_getAttributeNodeNS(SablotSituation s, SDOM_Node n, SDOM_char *uri, SDOM_char *local, SDOM_Node *attr)
{
    QName q;
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));

    Vertex *v = NULL;
    if ( strcmp((char*)uri,theXMLNSNamespace) ) {
      //true attribute
        const int attcount = e -> atts.number();
	for (int i = 0; i < attcount; i++) {
	  q = toA(e->atts[i]) -> name;
	  if (!strcmp((char*)local, (char*)(e -> getOwner().expand(q.getLocal())))
	      && !strcmp((char*)uri, (char*)(e -> getOwner().expand(q.getUri())))) {
	    v = e -> atts[i];
	    break;
	  };
	};
    } else {
      //namespace
        const int nscount = e -> namespaces.number();
	char* pre;
	if ( !strcmp(local,"xmlns") )
	  pre = "";
	else
	  pre = (char*)local;
	
	for (int i = 0; i < nscount; i++) {
	  q = toNS(e->namespaces[i]) -> name;
	  if (!strcmp(pre, (char*)(e -> getOwner().expand(toNS(e->namespaces[i])->prefix)))) {
	    v = e -> namespaces[i];
	    break;
	  };
	};
    };
    *attr = v;
    return SDOM_OK;
}

SDOM_Exception SDOM_getAttributeNodeIndex(SablotSituation s, SDOM_Node n, const int index, SDOM_Node *attr)
{
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));
    int nscount = e -> namespaces.number();

    if (index < 0 || index >= nscount + e -> atts.number() )
        *attr = NULL;
    else
        if (index < nscount)
	    *attr = (SDOM_Node)(e -> namespaces[index]);
	else 
            *attr = (SDOM_Node)(e -> atts[index-nscount]);
    return SDOM_OK;	    
}

SDOM_Exception SDOM_getAttributeNodeCount(SablotSituation s, SDOM_Node n, int *count)
{
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));
    *count = e -> namespaces.number()  +  e -> atts.number();
    return SDOM_OK;	    
}


SDOM_Exception SDOM_setAttribute(SablotSituation s, SDOM_Node n, const SDOM_char *attName, const SDOM_char *attValue)
{
    QName q;
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));

    e -> setLogical(SIT(s), q, attName, FALSE);
    Vertex *v;
    if ( !isXMLNS_(q, e) ) {
        v = e -> atts.find(q);
	if (!v) {
	    SE( _SDOM_createAttributeWithParent(s, ownerDoc(e), (SDOM_Node*) &v, attName, e));
	    //_TH_ v
	    getTmpList(n).rmP(v);
	    //_TH_ ^
	    e -> atts.append(toA(v));
	    toV(v) -> setParent(e);
	};
	SE( SDOM_setNodeValue(s, v, attValue) );	
    } else { //attribute is namespace declaration
        v = e -> namespaces.find(q.getLocal() != e -> getOwner().stdPhrase(PHRASE_XMLNS) ? q.getLocal() : UNDEF_PHRASE);
	if (!v) {
	    SE( _SDOM_createAttributeWithParent(s, ownerDoc(e), (SDOM_Node*) &v, attName, e));
	    //_TH_ v
	    getTmpList(n).rmP(v);
	    //_TH_ ^
	    e -> namespaces.append(toNS(v));
	    toV(v) -> setParent(e);
	};
	SE( SDOM_setNodeValue(s, v, attValue) );
	//_JP_ v
	toNS(v) -> kind = NSKIND_DECLARED;	
	SE( __SDOM_refreshNS(s, n, toNS(v)) );
	//_JP_ ^	
    };

    return SDOM_OK;	    
}

//_JP_ v
SDOM_Exception SDOM_setAttributeNS(SablotSituation s, SDOM_Node n, const SDOM_char *uri, const SDOM_char *qName, const SDOM_char *value)
{
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);

    if ( !isValidQName((char*)qName) )
        SDOM_Err(s, SDOM_INVALID_CHARACTER_ERR);

    QName q;
    Element *e = toE(toV(n));
    Str prefix = Str();
    char *colon = strchr((char*)qName,':');

    if ( colon ) { //qName has prefix

        if ( !uri ) 
	    SDOM_Err(s, SDOM_NAMESPACE_ERR);  

	prefix.nset((char*)qName, intPtrDiff(colon, qName));

	if ( !strcmp((const char*)prefix,"xml") && strcmp(theXMLNamespace,uri) )
	    //redefinition of xml-namespace
	    SDOM_Err(s, SDOM_NAMESPACE_ERR);

	if ( !strcmp((const char*)prefix,"xmlns") && strcmp(theXMLNSNamespace,uri) )
	    //redefinition of xmlns-namespace
	    SDOM_Err(s, SDOM_NAMESPACE_ERR);

	q.setPrefix(ownerDoc(toE(n))->dict().insert((const char*)prefix));
	q.setLocal(ownerDoc(toE(n))->dict().insert((const char*)((ptrdiff_t)colon + 1)));
	q.setUri(ownerDoc(toE(n))->dict().insert((const char*)uri));

    } else {

        q.setPrefix(UNDEF_PHRASE);
	q.setLocal(ownerDoc(toE(n))->dict().insert((const char*)qName));

	if ( uri && strcmp((char*)uri,"") ) {
	    q.setUri(ownerDoc(toE(n))->dict().insert((const char*)uri));
	} else {
	    q.setUri(UNDEF_PHRASE);
	};    
    };

    SDOM_Node replaced;
    SE( SDOM_getAttributeNodeNS(s, n, 
				(SDOM_char*)(e -> getOwner().expand(q.getUri())), 
				(SDOM_char*)(e -> getOwner().expand(q.getLocal())),
				&replaced) );

    if ( strcmp((char*)uri, theXMLNSNamespace) ) {
      //true attribute
      if ( replaced ) {
	//attr with the same uri:local found
	if ( q.getPrefix() == toA(replaced)->name.getPrefix() ) {
	  //the same prefix
	} else {
	  //prefix != replaced.prefix
	  if ( q.getPrefix() != UNDEF_PHRASE ) {
	    SE( __SDOM_touchNS(s, n, 
			       q.getPrefix(), 
			       q.getUri(),
			       NSKIND_PARENT, 0) );
	    e -> namespaces.decPrefixUsage(toA(replaced)->name.getPrefix());
	  };
	  toA(replaced)->name.setPrefix(q.getPrefix());
	};
      } else {
	//attr with the same uri:local NOT found
	if ( q.getPrefix() != UNDEF_PHRASE )
	  SE( __SDOM_touchNS(s, n, 
			     q.getPrefix(), 
			     q.getUri(),
			     NSKIND_PARENT, 0) );
	Tree *tOwner = toTree(ownerDoc(e));
	replaced = new(&(tOwner -> getArena())) 
	           Attribute(*tOwner, q, (char*)"", XSLA_NONE);
	e -> atts.append(toA(replaced));
	toV(replaced) -> setParent(e);
      };
      SE( SDOM_setNodeValue(s, replaced, value) );
      
    } else {
      // att is namespace
      SE( __SDOM_touchNSByChar(s, n, 
			       (SDOM_char*)(e -> getOwner().expand(q.getLocal())), 
			       (SDOM_char*)value,
			       NSKIND_DECLARED, 0) );
  };
    return SDOM_OK;	    
}


SDOM_Exception SDOM_setAttributeNode(SablotSituation s, SDOM_Node n, SDOM_Node attnode, SDOM_Node *replaced)
{
  if ( !( isElement(toV(n)) && (isAttr(toV(attnode)) 
				|| isNS(toV(attnode))) ) )
    SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
  if ( ownerDoc(n) != ownerDoc(attnode) )
    SDOM_Err(s, SDOM_WRONG_DOCUMENT_ERR);
  if ( toV(attnode) -> parent )
    SDOM_Err(s, SDOM_INUSE_ATTRIBUTE_ERR);
  Element *e = toE(toV(n));
  //NmSpace *nm;
  if ( isAttr(toV(attnode)) ) {
    Str attname;
    e -> getOwner().expandQStr(toA(attnode) -> getName(), attname);
    SE( SDOM_getAttributeNode(s, n, (SDOM_char*)attname, replaced) );
    if ( *replaced ) {
      //attr with the same prefix:local found
      if ( toA(attnode)->name.getUri() == toA(*replaced)->name.getUri() ) {
	//the same uri
// 	int ndx = e -> atts.findNdx(toA(*replaced)->name);
// 	toV(*replaced) -> parent = NULL;
// 	//_TH_ v
// 	getTmpList(n).append(*replaced);
// 	//_TH_ ^
// 	e -> atts[ndx] = toV(attnode);
	//_JP_ v
	int ndx = toV(*replaced) -> ordinal;
	toV(*replaced) -> parent = NULL;
	//_TH_ v
	getTmpList(n).append(*replaced);
	getTmpList(n).rmP(attnode);
	//_TH_ ^
	e -> atts[ndx] = toV(attnode);
	toV(attnode) -> ordinal = ndx;
	//_JP_ ^
      } else {
	//attnode.uri != replaced.uri
	if ( toA(attnode)->name.getPrefix() != UNDEF_PHRASE )
	  SE( __SDOM_touchNS(s, n, 
			     toA(attnode)->name.getPrefix(), 
			     toA(attnode)->name.getUri(),
			     NSKIND_PARENT, 1) );
// 	int ndx = e -> atts.findNdx(toA(*replaced)->name);
// 	toV(*replaced) -> parent = NULL;
// 	//_TH_ v
// 	getTmpList(n).append(*replaced);
// 	//_TH_ ^
// 	e -> atts[ndx] = toV(attnode);
	//_JP_ v
	int ndx = toV(*replaced) -> ordinal;
	toV(*replaced) -> parent = NULL;
	//_TH_ v
	getTmpList(n).append(*replaced);
	getTmpList(n).rmP(attnode);
	//_TH_ ^
	e -> atts[ndx] = toV(attnode);
	toV(attnode) -> ordinal = ndx;
	//_JP_ ^
      };
    } else {
      //attr with the same prefix:local NOT found
      if ( toA(attnode)->name.getPrefix() != UNDEF_PHRASE )
	SE( __SDOM_touchNS(s, n, 
			   toA(attnode)->name.getPrefix(), 
			   toA(attnode)->name.getUri(),
			   NSKIND_PARENT, 0) );
      //_TH_ v
      getTmpList(n).rmP(attnode);
      //_TH_ ^
      e -> atts.append(toA(attnode));
    };
    toV(attnode) -> setParent(e);
    
  } else {
    // attnode is namespace
    int ndx = e -> namespaces.findNdx(toNS(attnode)->prefix);
    if (ndx != -1) {
      //namespace with the attnode.prefix exists
      NmSpace *nm = toNS(e -> namespaces[ndx]);
      if ( nm -> uri == toNS(attnode) -> uri ) {
	//namespace with attnode.prefix has attnode.uri
	//_TH_ v
	getTmpList(n).rmP(attnode);
	//_TH_ ^
	toV(nm) -> parent = NULL;
	//_TH_ v
	getTmpList(n).append(nm);
	//_TH_ ^
	e -> namespaces[ndx] = toV(attnode);
	toV(attnode) -> setParent(e);
	toV(attnode) -> ordinal = ndx;
	toNS(attnode)->kind = nm->kind;
	toNS(attnode)->usageCount = nm->usageCount;
	*replaced = (SDOM_Node)nm;
      } else {
	//namespace with attnode.prefix has NOT attnode.uri
	if ( nm -> usageCount != 0 || nm -> kind == NSKIND_DECLARED ) {
	  // namespace used
	  SDOM_Err(s, SDOM_NAMESPACE_ERR);
	} else {
	  // namespace NOT used
	  //_TH_ v
	  getTmpList(n).rmP(attnode);
	  //_TH_ ^
	  toV(nm) -> parent = NULL;
	  //_TH_ v
	  getTmpList(n).append(nm);
	  //_TH_ ^
	  e -> namespaces[ndx] = toV(attnode);
	  toV(attnode) -> setParent(e);
	  toV(attnode) -> ordinal = ndx;
	  toNS(attnode)->kind = NSKIND_DECLARED;
	  toNS(attnode)->usageCount = 0;
	  *replaced = (SDOM_Node)nm;
	};
      };
    } else {
      //namespace with attnode.prefix NOT exists
      *replaced = NULL;
      toNS(attnode)->kind = NSKIND_DECLARED;
      toNS(attnode)->usageCount = 0;
      //_TH_ v
      getTmpList(n).rmP(attnode);
      //_TH_ ^
      e -> namespaces.append(toNS(attnode));
      toV(attnode) -> setParent(e);
      SE( __SDOM_refreshNS(s, n, toNS(attnode)) );
    };
  };
  return SDOM_OK;	    
}

SDOM_Exception SDOM_setAttributeNodeNS(SablotSituation s, SDOM_Node n, SDOM_Node attnode, SDOM_Node *replaced)
{
  if ( !( isElement(toV(n)) && (isAttr(toV(attnode)) 
				|| isNS(toV(attnode))) ) )
    SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
  if ( ownerDoc(n) != ownerDoc(attnode) )
    SDOM_Err(s, SDOM_WRONG_DOCUMENT_ERR);
  if ( toV(attnode) -> parent )
    SDOM_Err(s, SDOM_INUSE_ATTRIBUTE_ERR);
  Element *e = toE(toV(n));
  //NmSpace *nm;
  if ( isAttr(toV(attnode)) ) {
    Str attname;
    e -> getOwner().expandQStr(toA(attnode) -> getName(), attname);
    SE( SDOM_getAttributeNodeNS(s, n, 
				(SDOM_char*)(e -> getOwner().expand(toA(attnode)->name.getUri())), 
				(SDOM_char*)(e -> getOwner().expand(toA(attnode)->name.getLocal())),
				replaced) );
    if ( *replaced ) {
      //attr with the same uri:local found
      if ( toA(attnode)->name.getPrefix() == toA(*replaced)->name.getPrefix() ) {
	//the same prefix
// 	int ndx = e -> atts.findNdx(toA(*replaced)->name);
// 	toV(*replaced) -> parent = NULL;
// 	//_TH_ v
// 	getTmpList(n).append(*replaced);
// 	//_TH_ ^
// 	e -> atts[ndx] = toV(attnode);
	//_JP_ v
	int ndx = toV(*replaced) -> ordinal;
	toV(*replaced) -> parent = NULL;
	//_TH_ v
	getTmpList(n).append(*replaced);
	getTmpList(n).rmP(attnode);
	//_TH_ ^
	e -> atts[ndx] = toV(attnode);
	toV(attnode) -> ordinal = ndx;
	//_JP_ ^

	//+++++++++++++++++
      } else {
	//attnode.prefix != replaced.prefix
	if ( toA(attnode)->name.getPrefix() != UNDEF_PHRASE )
	  SE( __SDOM_touchNS(s, n, 
			     toA(attnode)->name.getPrefix(), 
			     toA(attnode)->name.getUri(),
			     NSKIND_PARENT, 1) );
	if ( toA(*replaced)->name.getPrefix() != UNDEF_PHRASE )
	  e -> namespaces.decPrefixUsage(toA(*replaced)->name.getPrefix());
// 	int ndx = e -> atts.findNdx(toA(*replaced)->name);
// 	toV(*replaced) -> parent = NULL;
// 	//_TH_ v
// 	getTmpList(n).append(*replaced);
// 	//_TH_ ^
// 	e -> atts[ndx] = toV(attnode);
	//_JP_ v
	int ndx = toV(*replaced) -> ordinal;
	toV(*replaced) -> parent = NULL;
	//_TH_ v
	getTmpList(n).rmP(attnode);
	getTmpList(n).append(*replaced);
	//_TH_ ^
	e -> atts[ndx] = toV(attnode);
	toV(attnode) -> ordinal = ndx;
	//_JP_ ^
      };
    } else {
      //attr with the same uri:local NOT found
      if ( toA(attnode)->name.getPrefix() != UNDEF_PHRASE )
	SE( __SDOM_touchNS(s, n, 
			   toA(attnode)->name.getPrefix(), 
			   toA(attnode)->name.getUri(),
			   NSKIND_PARENT, 0) );
      //_TH_ v
      getTmpList(n).rmP(attnode);
      //_TH_ ^
      e -> atts.append(toA(attnode));
    };
    toV(attnode) -> setParent(e);
    
  } else {
    // attnode is namespace
    int ndx = e -> namespaces.findNdx(toNS(attnode)->prefix);
    if (ndx != -1) {
      //namespace with the attnode.prefix exists
      NmSpace *nm = toNS(e -> namespaces[ndx]);
      if ( nm -> uri == toNS(attnode) -> uri ) {
	//namespace with attnode.prefix has attnode.uri
	//_TH_ v
	getTmpList(n).rmP(attnode);
	//_TH_ ^
	toV(nm) -> parent = NULL;
	//_TH_ v
	getTmpList(n).append(nm);
	//_TH_ ^
	e -> namespaces[ndx] = toV(attnode);
	toV(attnode) -> setParent(e);
	toV(attnode) -> ordinal = ndx;
	toNS(attnode)->kind = nm->kind;
	toNS(attnode)->usageCount = nm->usageCount;
	*replaced = (SDOM_Node)nm;
      } else {
	//namespace with attnode.prefix has NOT attnode.uri
	if ( nm -> usageCount != 0 || nm -> kind == NSKIND_DECLARED ) {
	  // namespace used
	  return SDOM_NAMESPACE_ERR;
	} else {
	  // namespace NOT used
	  //_TH_ v
	  getTmpList(n).rmP(attnode);
	  //_TH_ ^
	  toV(nm) -> parent = NULL;
	  //_TH_ v
	  getTmpList(n).append(nm);
	  //_TH_ ^
	  e -> namespaces[ndx] = toV(attnode);
	  toV(attnode) -> setParent(e);
	  toV(attnode) -> ordinal = ndx;
	  toNS(attnode)->kind = NSKIND_DECLARED;
	  toNS(attnode)->usageCount = 0;
	  *replaced = (SDOM_Node)nm;
	};
      };
    } else {
      //namespace with attnode.prefix NOT exists
      *replaced = NULL;
      toNS(attnode)->kind = NSKIND_DECLARED;
      toNS(attnode)->usageCount = 0;
      //_TH_ v
      getTmpList(n).rmP(attnode);
      //_TH_ ^
      e -> namespaces.append(toNS(attnode));
      toV(attnode) -> setParent(e);
      SE( __SDOM_refreshNS(s, n, toNS(attnode)) );
    };
  };
  return SDOM_OK;	    
}


SDOM_Exception SDOM_removeAttributeNode(SablotSituation s, SDOM_Node n, SDOM_Node attnode, SDOM_Node *removed)
{
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));

    if (isAttr(attnode))
    {
        int attNdx = e -> atts.findNdx(toA(attnode)->name);
        if (attNdx != -1)
        {
	    //_TH_ v
	    Vertex *tmp = e -> atts[attNdx];
	    //_TH_ ^
            tmp -> parent = NULL;
            e -> atts.rm(attNdx);
	    //_TH_ v
	    getTmpList(n).append(tmp);
	    //_TH_ ^
	    if (toA(tmp) -> name.getPrefix() != UNDEF_PHRASE)
	        e -> namespaces.decPrefixUsage(toA(tmp) -> name.getPrefix());
	    *removed = tmp;
    	} else {
	  SDOM_Err(s, SDOM_NOT_FOUND_ERR);
	};
    }
    else
    {
        int attNdx = e -> namespaces.findNdx(toNS(attnode)->prefix);
        if (attNdx != -1)
	{
	    if (toNS(e -> namespaces[attNdx]) -> usageCount != 0) 
	        SDOM_Err(s, SDOM_NO_MODIFICATION_ALLOWED_ERR);
	    //_TH_ v
	    Vertex *tmp = e -> namespaces[attNdx];
	    //_TH_ ^
            tmp -> parent = NULL;
            e -> namespaces.rm(attNdx);
	    //_TH_ v
	    getTmpList(n).append(tmp);
	    //_TH_ ^
	    *removed = tmp;
    	} else {
	  SDOM_Err(s, SDOM_NOT_FOUND_ERR);
	};
    }
    return SDOM_OK;	    
}
//_JP_ ^	


SDOM_Exception SDOM_removeAttribute(SablotSituation s, SDOM_Node n, const SDOM_char *name)
{
    QName q;
    if (!isElement(toV(n)))
        SDOM_Err(s, SDOM_INVALID_NODE_TYPE);
    Element *e = toE(toV(n));

    e -> setLogical(SIT(s), q, name, FALSE);
    if (!isXMLNS_(q, e))
    {
        int attNdx = e -> atts.findNdx(q);
        if (attNdx != -1)
        {
	    //_TH_ v
	    Vertex *tmp = e -> atts[attNdx];
	    //_TH_ ^
            e -> atts[attNdx] -> parent = NULL;
            e -> atts.rm(attNdx);
	    //_TH_ v
	    getTmpList(n).append(tmp);
	    //_TH_ ^
	    //_JP_ v
	    if (toA(tmp) -> name.getPrefix() != UNDEF_PHRASE)
	        e -> namespaces.decPrefixUsage(toA(tmp) -> name.getPrefix());
	    //_JP_ ^
    	}
    }
    else
    {
        int attNdx = e -> namespaces.findNdx(
			q.getLocal() != e -> getOwner().stdPhrase(PHRASE_XMLNS) ? 
			q.getLocal() : UNDEF_PHRASE);
        if (attNdx != -1)
	{
	    //_JP_ v
	    if (toNS(e -> namespaces[attNdx]) -> usageCount != 0) 
	        return SDOM_NO_MODIFICATION_ALLOWED_ERR;
	    //_JP_ ^
            e -> namespaces[attNdx] -> parent = NULL;
            e -> namespaces.rm(attNdx);
    	}	
    }
    return SDOM_OK;	    
}


SDOM_Exception SDOM_getAttributeElement(SablotSituation s, SDOM_Node attr, SDOM_Node *owner)
{
  Vertex *v = toV(attr);
  if ( isAttr(v) | isNS(v) )
    {
      *owner = v -> parent;
      return SDOM_OK;
    }
  else
    return SDOM_HIERARCHY_REQUEST_ERR;
}

SDOM_Exception SDOM_getAttributeList(SablotSituation s, SDOM_Node n, SDOM_NodeList *pAttrList)
{
    *pAttrList = new CList;
    if (isElement(toV(n)))
    {
        int i;
        NSList& namespaces = toE(toV(n)) -> namespaces;
	    for (i = 0; i < namespaces.number(); i++)
	        ((CList*)(*pAttrList)) -> append(namespaces[i]);
        AttList& atts = toE(toV(n)) -> atts;
	    for (i = 0; i < atts.number(); i++)
	        ((CList*)(*pAttrList)) -> append(atts[i]);
    }    
    return SDOM_OK;
}

//
//
//

SDOM_Exception SDOM_docToString(SablotSituation s, SDOM_Document d, SDOM_char **pSerialized)
{
    // using explicit temp because of BeOS
    char *serializedTemp = NULL;
    toTree(d) -> serialize(SIT(s), serializedTemp);
    *pSerialized = serializedTemp;
    return SDOM_OK;
}

//_PH_
SDOM_Exception SDOM_nodeToString(SablotSituation s, SDOM_Document d, 
				 SDOM_Node n,
				 SDOM_char **pSerialized)
{
    char *serializedTemp = NULL;
    toTree(d) -> serializeNode(SIT(s), toE(n), serializedTemp);
    *pSerialized = serializedTemp;
    return SDOM_OK;
}

//
//
//

SDOM_Exception SDOM_getNodeListLength(SablotSituation s, SDOM_NodeList l, int *pLength)
{
    *pLength = ((CList*)l) -> number();
    return SDOM_OK;
}

SDOM_Exception SDOM_getNodeListItem(SablotSituation s, SDOM_NodeList l, int index, SDOM_Node *pItem)
{
    if ((index < 0) || (index >= ((CList*)l) -> number()))
        SDOM_Err(s, SDOM_INDEX_SIZE_ERR);
    *pItem = ((CList*)l) -> operator[](index);
    return SDOM_OK;
}

SDOM_Exception SDOM_disposeNodeList(SablotSituation s, SDOM_NodeList l)
{
    if (!((CList*)l) -> decRefCount())
        delete (CList*)l;
	return SDOM_OK;
}


//
//
//

SDOM_Exception SDOM_xql(SablotSituation s, const SDOM_char *query, SDOM_Node currentNode, SDOM_NodeList *pResult)
{
    RootNode &root = toV(currentNode) -> getOwner().getRoot();
    
    // find the document element
    int i;
    for (i = 0; i < root.contents.number(); i++)
        if (isElement(root.contents[i])) break;
	
    Element *docElement = (i < root.contents.number()) ?
        toE(root.contents[i]) : &root;
    Expression queryEx(*docElement);
    *pResult = NULL;
    
    // parse the query string as a non-pattern
    if (queryEx.parse(SIT(s), (char*)query, FALSE, TRUE))
        SDOM_Err(s, SDOM_QUERY_PARSE_ERR);
		
	// create an initial context
    GP( Context ) dummy = new Context(NULL); //_cn_ no current node
    GP( Context ) c;
    c.assign(dummy);
    (*c).set(toV(currentNode));
	
	// create the result context
	if (queryEx.createContext(SIT(s), c))
        SDOM_Err(s, SDOM_QUERY_EXECUTION_ERR);
	
	// preserve the CList
	(*c).getArrayForDOM() -> incRefCount();
    c.unkeep();
	*pResult = (SDOM_NodeList) (*c).getArrayForDOM();
    return SDOM_OK;
}

SDOM_Exception SDOM_xql_ns(SablotSituation s, const SDOM_char *query, SDOM_Node currentNode, char** nsmap, SDOM_NodeList *pResult)
{
  //RootNode &root = toV(currentNode) -> getOwner().getRoot();
    
  // find the document element
  //int i;
  //for (i = 0; i < root.contents.number(); i++)
  //    if (isElement(root.contents[i])) break;
  GP( Tree ) tree = new Tree(Str("noscheme:dummy-tree"), false);
  QName q; 
  q.setLocal((*tree).unexpand(Str("dummy-root")));
  Element *e = new(&((*tree).getArena())) Element(*tree, q);
  e -> setSubtreeInfo((*tree).getRootSubtree());
  char ** aux = nsmap;
  while (*aux) {
    char *pref = *aux;
    char *uri = *(aux + 1);
    NmSpace *nm = new(&((*tree).getArena())) 
      NmSpace(*tree, (*tree).unexpand(pref), (*tree).unexpand(uri),
	      false, NSKIND_DECLARED);
    e -> newChild(SIT(s), nm);
    aux += 2;
  }
	
  Expression queryEx(*e);
  *pResult = NULL;
  
  // parse the query string as a non-pattern
  if (queryEx.parse(SIT(s), (char*)query, FALSE, TRUE))
    SDOM_Err(s, SDOM_QUERY_PARSE_ERR);
  
  // create an initial context
  GP( Context ) dummy = new Context(NULL); //_cn_ no current node
  GP( Context ) c;
  c.assign(dummy);
  (*c).set(toV(currentNode));
  
  // create the result context
  if (queryEx.createContext(SIT(s), c))
    SDOM_Err(s, SDOM_QUERY_EXECUTION_ERR);
  
  // preserve the CList
  (*c).getArrayForDOM() -> incRefCount();
  c.unkeep();
  *pResult = (SDOM_NodeList) (*c).getArrayForDOM();
  return SDOM_OK;
}

//
//    exception retrieval
//

int SDOM_getExceptionCode(SablotSituation s)
{
    return SIT(s).getSDOMExceptionCode();
}

char* SDOM_getExceptionMessage(SablotSituation s)
{
    return SDOM_newString(
        SDOM_ExceptionMsg[SDOM_getExceptionCode(s)]);
}

void SDOM_getExceptionDetails(
    SablotSituation s,
    int *code,
    char **message,
    char **documentURI,
    int *fileLine)
{
    Str message_, documentURI_;
    MsgCode code_;
    int fileLine_;
    SIT(s).getSDOMExceptionExtra(code_, message_, documentURI_, fileLine_);
    *code = code_;
    *fileLine = fileLine_;
    *documentURI = SDOM_newString(documentURI_);
    *message = SDOM_newString(message_);
}



//
//    internal
//

void SDOM_setNodeInstanceData(SDOM_Node n, void *data)
{
    toV(n) -> setInstanceData(data);
}

void* SDOM_getNodeInstanceData(SDOM_Node n)
{
    return toV(n) -> getInstanceData();
}

void SDOM_setDisposeCallback(SDOM_NodeCallback *f)
{
    theDisposeCallback = f;
}

SDOM_NodeCallback* SDOM_getDisposeCallback()
{
    return theDisposeCallback;
}



//_TH_ v
void SDOM_tmpListDump(SDOM_Document doc, int p)
{
  getTmpList(doc).dump(p);
}
//_TH_ ^

SDOM_Exception SDOM_compareNodes(SablotSituation s, SDOM_Node n1, SDOM_Node n2, int *res)
{
  if (&(toV(n1) -> getOwner()) == &(toV(n2) -> getOwner()) ) //the same owner
    {
      int s1 = toV(n1) -> stamp;
      int s2 = toV(n2) -> stamp;
      if (s1 < s2)
	*res = -1;
      else if (s1 == s2)
	*res = 0;
      else 
	*res = 1;
    }
  else
    {
      *res = strcmp( (char*)(toV(n1) -> getOwner().getURI()),
		     (char*)(toV(n2) -> getOwner().getURI()) );
    }
  return SDOM_OK;
}

#endif /* ENABLE_DOM */
