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
 * Contributor(s): Han Qi
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

#include "jsdom.h"

#ifdef ENABLE_JS

#define SablotAsExport
#include "base.h"
#include "sdom.h"
#include "verts.h"

#define declPRIV  NodePrivate *np = (NodePrivate*)JS_GetPrivate(cx, obj)
#define NPDOM np->situa->dom()

/************************ domex  *********************/

void domexFinalize(JSContext *cx, JSObject *obj)
{
  DomExPrivate *priv = (DomExPrivate*)JS_GetPrivate(cx, obj);
  if (priv) {
    delete priv;
  }
}

JSClass domexClass = {
  "DOMException",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  domexFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JSBool domexGetProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  *vp = id;
  return TRUE;
}

JS_PROP(domexGetCode)
{
  DomExPrivate *priv = (DomExPrivate*)JS_GetPrivate(cx, obj);
  *rval = INT_TO_JSVAL(priv -> code);
  return TRUE;
}

JSPropertySpec domexProtoProps[] =
{
  {"INDEX_SIZE_ERR", 1, PROP_OPT, domexGetProperty, NULL},
  {"STRING_SIZE_ERR", 2, PROP_OPT, domexGetProperty, NULL},
  {"HIERARCHY_REQUEST_ERR", 3, PROP_OPT, domexGetProperty, NULL},
  {"WRONG_DOCUMENT_ERR", 4, PROP_OPT, domexGetProperty, NULL},
  {"INVALID_CHARACTER_ERR", 5, PROP_OPT, domexGetProperty, NULL},
  {"NO_DATA_ALLOWED_ERR", 6, PROP_OPT, domexGetProperty, NULL},
  {"NO_MODIFICATION_ALLOWED_ERR", 7, PROP_OPT, domexGetProperty, NULL},
  {"NOT_FOUND_ERR", 8, PROP_OPT, domexGetProperty, NULL},
  {"NOT_SUPPORTED_ERR", 9, PROP_OPT, domexGetProperty, NULL},
  {"INUSE_ATTRIBUTE_ERR", 10, PROP_OPT, domexGetProperty, NULL},
  {"INVALID_STATE_ERR", 11, PROP_OPT, domexGetProperty, NULL},
  {"SYNTAX_ERR", 12, PROP_OPT, domexGetProperty, NULL},
  {"INVALID_MODIFICATION_ERR", 13, PROP_OPT, domexGetProperty, NULL},
  {"NAMESPACE_ERR", 14, PROP_OPT, domexGetProperty, NULL},
  {"INVALID_ACCESS_ERR", 15, PROP_OPT, domexGetProperty, NULL},
  {NULL, 0, 0, 0, 0}
};

JS_METHOD(domexToString)
{
  DomExPrivate *priv = (DomExPrivate*)JS_GetPrivate(cx, obj);
  char buff[128];
  //find the entry
  JSPropertySpec* aux = domexProtoProps;
  while (aux -> name)
    {
      if (aux -> tinyid == priv -> code) break;
      aux++;
    }

  sprintf(buff, "[DOMException %d = %s]", priv -> code,
	  aux -> name ? aux -> name : "unknown code");
  JSString *str = JS_NewStringCopyZ(cx, buff);
  *rval = STRING_TO_JSVAL(str);
  return TRUE;
}

JSPropertySpec domexProps[] =
{
  {"code", 0, PROP_OPT, domexGetCode, NULL},
  {NULL, 0, 0, 0, 0}
};

JSFunctionSpec domexFunctions[] =
{
  {"toString", domexToString, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

/***************nodelist array **********************/

JS_METHOD(nlistItem)
{
  JSBool rv;
  if (argc >= 1) {
    rv = JS_GetElement(cx, obj, JSVAL_TO_INT(argv[0]), rval);
  } 
  else {
    *rval = JSVAL_VOID;
    rv = TRUE;
  }
  return rv;
}

//  JS_METHOD(nlistDebug)
//  {
//    JSObject *foo = obj;
//    while (foo)
//      {
//        JSClass *cls = JS_GetClass(foo);
//        printf("---> clsname: %s\n", cls -> name);
//        foo = JS_GetPrototype(cx, foo);
//      }

//    return TRUE;
//  }

JS_METHOD(nlistAppend)
{
  jsuint len;
  JS_GetArrayLength(cx, obj, &len);
  for (unsigned int i = 0; i < argc; i++)
    {
      JS_SetElement(cx, obj, len++, &argv[i]);
    }
  return TRUE;
}

JS_METHOD(nlistRemove)
{
  if (argc > 0)
    {
      JS_DeleteElement(cx, obj, JSVAL_TO_INT(argv[0]));
    }
  return TRUE;
}

JSFunctionSpec nlistFunctions[] = {
  {"item", nlistItem, 0, 0, 0},
  {"append", nlistAppend, 0, 0, 0},
  //{"remove", nlistRemove, 0, 0, 0}, //BUGGY
   {NULL, 0, 0, 0, 0}
};

JS_METHOD(nlistConstructor)
{
  //create an array object to replace obj instance
  JSObject *arr = JS_NewArrayObject(cx, 0, NULL);
  
  //get array prototypr (all methods)
//    JSContextItem *cxi = (JSContextItem*)JS_GetContextPrivate(cx);
//    sabassert(cxi);
//    if (cxi -> array_proto) 
//      {
//        //JS_SetPrototype(cx, obj, cxi -> array_proto);
//        JS_SetPrototype(cx, arr, cxi -> array_proto);
//      }

  //set original object as a prototype of returned array
  JS_SetPrototype(cx, arr, obj);
  //set specific object methods
  JS_DefineFunctions(cx, obj, nlistFunctions);

  *rval = OBJECT_TO_JSVAL(arr);
  return TRUE;
}

JSClass nlistClass = {
  "NodeList", 0,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  JS_FinalizeStub,
  0,
  0,
  NULL,
  nlistConstructor, NULL, NULL, 0, 0
};

/******************** dom implementation ************/

JSClass domimplClass = {
  "DOMImplementation", 0,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  JS_FinalizeStub, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JS_METHOD(domimplHasFeature)
{
  *rval = JSVAL_FALSE;
  return TRUE;
}

JS_METHOD(domimplCreateDocumentType)
{
  DOM_EX( 9 );
  return TRUE;
}

JS_METHOD(domimplCreateDocument)
{
  DOM_EX( 9 );
  return TRUE;
}

JSFunctionSpec domimplFunctions[] = {
  {"hasFeature", domimplHasFeature, 0, 0, 0},
  {"createDocumentType", domimplCreateDocumentType, 0, 0, 0},
  {"createDocument", domimplCreateDocument, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

/************************ node  *********************/
void* _jsdom_getNodePrivate(Sit S, NodeHandle node)
{
  NodePrivate *np = new NodePrivate;
  np -> situa = &S;
  np -> node = node;
  return (void*)np;
}

JSBool nodeGetProperty (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  *vp = id;
  return TRUE;
}

void nodeFinalize(JSContext *cx, JSObject *obj)
{
  NodePrivate *np = (NodePrivate*)JS_GetPrivate(cx, obj);
  if (np) {
    delete np;
  }
}


JS_PROP(nodeGetNodeName)
{
  declPRIV;
  if (np) {
    char* val;
    Bool doFree = FALSE;
    switch(NPDOM.getNodeType(np->node)) {
    case ELEMENT_NODE:
    case ATTRIBUTE_NODE:
    case PROCESSING_INSTRUCTION_NODE:
      val = (char *)NPDOM.getNodeName(np->node);
      doFree = TRUE;
      break;
    case TEXT_NODE:
      val = "#text";
      break;
    case COMMENT_NODE:
      val = "#comment";
      break;
    case DOCUMENT_NODE:
      val = "#document";
      break;
    default:
      /* NAMESPACE_NODE - not in DOM 2;
       * CDATASection, DocumentFragment, DocumentType,
       * Entity,EntityReference, Notation - not in Sablotron
       */
      val = (char *)NPDOM.getNodeName(np->node);
      doFree = TRUE;
    }
    JSString *str = JS_NewStringCopyZ(cx, val);
    if (doFree) NPDOM.freeName(np->node, val);
    *rval = STRING_TO_JSVAL(str);
    return TRUE;
  } 
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetNodeValue)
{
  declPRIV;
  if (np) {
    char* val = (char *)NPDOM.getNodeValue(np->node);
    JSString *str = JS_NewStringCopyZ(cx, val);
    NPDOM.freeValue(np->node, val);
    *rval = STRING_TO_JSVAL(str);
    return TRUE;
  } 
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetNodeType)
{
  declPRIV;
  if (np) {
    *rval = INT_TO_JSVAL(NPDOM.getNodeType(np->node));
    return TRUE;
  } 
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetParentNode)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, NPDOM.getParent(np->node));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetChildNodes)
{
  declPRIV;
  if (np) {
    JSObject *arr = jsdom_createNodeList(cx, 0);
    *rval = OBJECT_TO_JSVAL(arr);
    int count = NPDOM.getChildCount(np->node);
	
    for (int i = 0; i < count; i++)
      {
	JSObject *x = jsdom_createNode(cx, np, NPDOM.getChildNo(np->node, i));
	jsval xx = OBJECT_TO_JSVAL(x);
	JS_SetElement(cx, arr, i, &xx);
      }
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetFirstChild)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, NPDOM.getChildNo(np->node, 0));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetLastChild)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, 
				   NPDOM.getChildNo(np->node, 
						    NPDOM.getChildCount(np->node) - 1 ));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetPreviousSibling)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, NPDOM.getPreviousSibling(np->node));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetNextSibling)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, NPDOM.getNextSibling(np->node));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetAttributes)
{
  declPRIV;
  if (np) {
    if (NPDOM.getNodeType(np->node) == ELEMENT_NODE) {
      JSObject *nmap = jsdom_createNamedNodeMap(cx, 0);
      //iterate attributes
      int attcount = NPDOM.getAttributeCount(np->node);
      for (int i = 0; i < attcount; i++)
	{
	  SXP_Node n = NPDOM.getAttributeNo(np->node, i);
	  JSObject *att = jsdom_wrapNode(*(np->situa), cx, n);
	  jsval val = OBJECT_TO_JSVAL(att);
	  char *name = (char *)NPDOM.getNodeName(n);
	  JS_SetProperty(cx, nmap, name, &val);
	  NPDOM.freeName(n,name);
	}
      //return 
      *rval = OBJECT_TO_JSVAL(nmap);
    } else {
      *rval = JSVAL_NULL;
    }
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetOwnerDocument)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, NPDOM.getOwnerDocument(np->node));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetNamespaceUri)
{
  declPRIV;
  if (np) {
    char* val = (char *)NPDOM.getNodeNameURI(np->node);
    if (val) {
      JSString *str = JS_NewStringCopyZ(cx, val);
      *rval = STRING_TO_JSVAL(str);
    } 
    else {
      *rval = JSVAL_NULL;
    }
    NPDOM.freeName(np->node,val);
    return TRUE;
  } 
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetPrefix)
{
  declPRIV;
  if (np) {
    char *val = NULL, *colon = NULL;
    bool doFree = false;
    switch(NPDOM.getNodeType(np->node)) {
    case TEXT_NODE:
    case DOCUMENT_NODE:
    case COMMENT_NODE:
      break;
    case ELEMENT_NODE:
    case ATTRIBUTE_NODE:
    case PROCESSING_INSTRUCTION_NODE:
    default: // see comment within nodeGetNodeName 
      val = (char *)NPDOM.getNodeName(np->node);
      colon = strchr(val,':');
      doFree = true;
    }
    if (val && colon) {
      JSString *str = JS_NewStringCopyN(cx, val, colon - val);
      *rval = STRING_TO_JSVAL(str);
    } 
    else {
      *rval = JSVAL_NULL;
    }
    if (doFree) NPDOM.freeName(np->node, val);
    return TRUE;
  } 
  else {
    return FALSE;
  }
}

JS_PROP(nodeGetLocalName)
{
  declPRIV;
  if (np) {
    char* val = (char *)NPDOM.getNodeNameLocal(np->node);
    if (val) {
      JSString *str = JS_NewStringCopyZ(cx, val);
      *rval = STRING_TO_JSVAL(str);
    } 
    else {
      *rval = JSVAL_NULL;
    }
    NPDOM.freeName(np->node,val);
    return TRUE;
  } 
  else {
    return FALSE;
  }
}

JS_METHOD(nodeToString)
{
  declPRIV;
  if (np) {
    DStr val;
    NPDOM.constructStringValue(np->node, val);
    JSString *str = JS_NewStringCopyZ(cx, (char*)val);
    *rval = STRING_TO_JSVAL(str);
    return TRUE;
  } 
  else {
    return FALSE;
  }
}

JS_METHOD(nodeUnsupported)
{
  DOM_EX( 9 );
  return TRUE;
}

JS_METHOD(nodeHasChildNodes)
{
  declPRIV;
  if (np) {
    if (NPDOM.getChildCount(np->node)) *rval = JSVAL_TRUE;
    else *rval = JSVAL_FALSE;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_METHOD(nodeNormalize)
{
  //simply does nothing
  return TRUE;
}

JS_METHOD(nodeIsSupported)
{
  //harmlessly unsupported
  *rval = JSVAL_FALSE;
  return TRUE;
}

JS_METHOD(nodeHasAttributes)
{
  declPRIV;
  if (np) {
    if (NPDOM.getAttributeCount(np->node)) *rval = JSVAL_TRUE;
    else *rval = JSVAL_FALSE;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JSClass nodeClass = {
  "Node",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

//node prototype properties
JSPropertySpec nodeProtoProps[] =
{
  //nodetype constants
  {"ELEMENT_NODE",  SDOM_ELEMENT_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"ATTRIBUTE_NODE",  SDOM_ATTRIBUTE_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"TEXT_NODE",  SDOM_TEXT_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"CDATA_SECTION_NODE",  SDOM_CDATA_SECTION_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"ENTITY_REFERENCE_NODE",  SDOM_ENTITY_REFERENCE_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"ENTITY_NODE",  SDOM_ENTITY_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"PROCESSING_INSTRUCTION_NODE",  SDOM_PROCESSING_INSTRUCTION_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"COMMENT_NODE",  SDOM_COMMENT_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"DOCUMENT_NODE",  SDOM_DOCUMENT_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"DOCUMENT_TYPE_NODE",  SDOM_DOCUMENT_TYPE_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"DOCUMENT_FRAGMENT_NODE",  SDOM_DOCUMENT_FRAGMENT_NODE, 
   PROP_OPT, nodeGetProperty, NULL},
  {"NOTATION_NODE",  SDOM_NOTATION_NODE , 
   PROP_OPT, nodeGetProperty, NULL},
  {NULL, 0, 0, 0, 0}
};

//'real' node props
JSPropertySpec nodeProps[] =
{
  //other properties
  {"nodeName", 0, PROP_OPT, nodeGetNodeName, NULL},
  {"nodeValue", 0, PROP_OPT, nodeGetNodeValue, NULL},
  {"nodeType", 0, PROP_OPT, nodeGetNodeType, NULL},
  {"parentNode", 0, PROP_OPT, nodeGetParentNode, NULL},
  {"childNodes", 0, PROP_OPT, nodeGetChildNodes, NULL},
  {"firstChild", 0, PROP_OPT, nodeGetFirstChild, NULL},
  {"lastChild", 0, PROP_OPT, nodeGetLastChild, NULL},
  {"previousSibling", 0, PROP_OPT, nodeGetPreviousSibling, NULL},
  {"nextSibling", 0, PROP_OPT, nodeGetNextSibling, NULL},
  {"attributes", 0, PROP_OPT, nodeGetAttributes, NULL},
  {"ownerDocument", 0, PROP_OPT, nodeGetOwnerDocument, NULL},
  {"namespaceURI", 0, PROP_OPT, nodeGetNamespaceUri, NULL},
  {"prefix", 0, PROP_OPT, nodeGetPrefix, NULL},
  {"localName", 0, PROP_OPT, nodeGetLocalName, NULL},
  {NULL, 0, 0, 0, 0}  
};


JSFunctionSpec nodeFunctions[] = 
{
  //unsupported
  {"toString", nodeToString, 0, 0, 0},
  {"insertBefore", nodeUnsupported, 0, 0, 0},
  {"replaceChild", nodeUnsupported, 0, 0, 0},
  {"removeChild", nodeUnsupported, 0, 0, 0},
  {"appendChild", nodeUnsupported, 0, 0, 0},
  {"cloneNode", nodeUnsupported, 0, 0, 0},
  //supported
  {"hasChildNodes", nodeHasChildNodes, 0, 0, 0},
  {"normalize", nodeNormalize, 0, 0, 0},
  {"isSupported", nodeIsSupported, 0, 0, 0},
  {"hasAttributes", nodeHasAttributes, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

/************************** document ***********************/

JSClass documentClass = {
  "Document",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JS_PROP(docGetType)
{
  *rval = JSVAL_VOID;
  return TRUE;
}

JS_PROP(docGetImplementation)
{
  JSContextItem *item = (JSContextItem*)JS_GetContextPrivate(cx);
  sabassert(item);

  *rval = OBJECT_TO_JSVAL(item -> domimpl);
  return TRUE;
}

JS_PROP(docGetDocumentElement)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, NPDOM.getChildNo(np->node, 0));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_METHOD(docElementsByTagName)
{
  DOM_EX( 9 );
  return TRUE;
}

JS_METHOD(docElementsByTagNameNS)
{
  DOM_EX( 9 );
  return TRUE;
}

JSPropertySpec documentProps[] = {
  {"doctype", 0, PROP_OPT, docGetType, NULL},
  {"implementation", 0, PROP_OPT, docGetImplementation, NULL},
  {"documentElement", 0, PROP_OPT, docGetDocumentElement, NULL},
  {NULL, 0, 0, 0, 0}
};

JSFunctionSpec documentFunctions[] = {
  //not needed
  {"createElement", nodeUnsupported, 0, 0, 0},
  {"createDocumentFragment", nodeUnsupported, 0, 0, 0},
  {"createTextNode", nodeUnsupported, 0, 0, 0},
  {"createComment", nodeUnsupported, 0, 0, 0},
  {"createCDATASection", nodeUnsupported, 0, 0, 0},
  {"createProcessingInstruction", nodeUnsupported, 0, 0, 0},
  {"createAttribute", nodeUnsupported, 0, 0, 0},
  {"createEntityReference", nodeUnsupported, 0, 0, 0},
  {"importNode", nodeUnsupported, 0, 0, 0},
  {"createElementNS", nodeUnsupported, 0, 0, 0},
  {"createAttributeNS", nodeUnsupported, 0, 0, 0},
  //we do not support ids
  {"getElementsById", nodeUnsupported, 0, 0, 0},
  //should be supported
  {"getElementsByTagName", docElementsByTagName, 0, 0, 0},
  {"getElementsByTagNameNS", docElementsByTagNameNS, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

/************************** element ************************/

JSClass elementClass = {
  "Element",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};


JS_PROP(eleGetTagName)
{
  declPRIV;
  if (np) {
    char *name;
    name = (char *)NPDOM.getNodeName(np->node);
    JSString *jname = JS_NewStringCopyZ(cx, name);
    *rval = STRING_TO_JSVAL(jname);
    NPDOM.freeName(np->node, name);
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_METHOD(eleGetAttribute)
{ 
  declPRIV;
  if (argc < 1) 
    {
      JS_ReportError(cx, "Element.getAttribute must have one parameter");
      return FALSE;
    }
  if (np) {
    JSString *str = JSVAL_TO_STRING(argv[0]);
    char *name = JS_GetStringBytes(str);
    char *value;
    int attcount = NPDOM.getAttributeCount(np->node);
    char *attname;
    NodeHandle attnode;
    JSString *jval;
    for (int i = 0; i < attcount; i++) {
      attnode = NPDOM.getAttributeNo(np->node, i);
      attname = (char *)NPDOM.getNodeName(attnode);
      if ( !strcmp(name, attname) ) {
	value = (char *)NPDOM.getNodeValue(attnode);
	jval = JS_NewStringCopyZ(cx, value);
	*rval = STRING_TO_JSVAL(jval);	
	NPDOM.freeValue(attnode, value);
	NPDOM.freeName(attnode, attname);
	return TRUE;
      };
      NPDOM.freeName(attnode, attname);
    };
    jval = JS_NewStringCopyZ(cx, "");
    *rval = STRING_TO_JSVAL(jval);
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_METHOD(eleGetAttributeNode)
{
  declPRIV;
  if (argc < 1) 
    {
      JS_ReportError(cx, "Element.getAttribute must have one parameter");
      return FALSE;
    }
  if (np) {
    JSString *str = JSVAL_TO_STRING(argv[0]);
    char *name = JS_GetStringBytes(str);
    int attcount = NPDOM.getAttributeCount(np->node);
    char *attname;
    NodeHandle attnode;
    for (int i = 0; i < attcount; i++) {
      attnode = NPDOM.getAttributeNo(np->node, i);
      attname = (char *)NPDOM.getNodeName(attnode);
      if ( !strcmp(name, attname) ) {
	JSObject *o = jsdom_createNode(cx, np, attnode);
	*rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
	NPDOM.freeName(attnode, attname);
	return TRUE;
      };
      NPDOM.freeName(attnode, attname);
    };
    *rval = JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_METHOD(eleGetElementsByTagName)
{
  *rval = JSVAL_NULL;
  return TRUE;
}

JS_METHOD(eleHasAttribute)
{
  declPRIV;
  if (np) {
    JSString *str = JSVAL_TO_STRING(argv[0]);
    char *name = JS_GetStringBytes(str);
    int attcount = NPDOM.getAttributeCount(np->node);
    char *attname;
    NodeHandle attnode;
    for (int i = 0; i < attcount; i++) {
      attnode = NPDOM.getAttributeNo(np->node, i);
      attname = (char *)NPDOM.getNodeName(attnode);
      if ( !strcmp(name, attname) ) {
	NPDOM.freeName(attnode, attname);
	*rval = JSVAL_TRUE;
	return TRUE;
      };
      NPDOM.freeName(attnode, attname);
    };
    *rval = JSVAL_FALSE;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JSPropertySpec elementProps[] =
{
  {"tagName", 0, PROP_OPT, eleGetTagName, NULL},
  {NULL, 0, 0, 0, 0}
};

JSFunctionSpec elementFunctions[] = 
{
  {"getAttribute", eleGetAttribute, 0, 0, 0},
  {"setAttribute", nodeUnsupported, 0, 0, 0},
  {"removeAttribute", nodeUnsupported, 0, 0, 0},
  {"getAttributeNode", eleGetAttributeNode, 0, 0, 0},
  {"setAttributeNode", nodeUnsupported, 0, 0, 0},
  {"removeAttributeNode", nodeUnsupported, 0, 0, 0},
  {"getElementsByTagName", eleGetElementsByTagName, 0, 0, 0},
  {"getAttributeNS", nodeUnsupported, 0, 0, 0},
  {"setAttributeNS", nodeUnsupported, 0, 0, 0},
  {"removeAttributeNS", nodeUnsupported, 0, 0, 0},
  {"getAttributeNodeNS", nodeUnsupported, 0, 0, 0},
  {"setAttributeNodeNS", nodeUnsupported, 0, 0, 0},
  {"getElementsByTagNameNS", nodeUnsupported, 0, 0, 0},
  {"hasAttribute", eleHasAttribute, 0, 0, 0},
  {"hasAttributeNS", nodeUnsupported, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

/********************* Attr ************************/

JSClass attrClass = {
  "Attr",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JS_PROP(attrGetSpecified)
{
  *rval = JSVAL_TRUE;
  return TRUE;
}

JS_PROP(attrGetOwnerElement)
{
  declPRIV;
  if (np) {
    JSObject *o = jsdom_createNode(cx, np, NPDOM.getParent(np->node));
    *rval = o ? OBJECT_TO_JSVAL(o) : JSVAL_NULL;
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JSPropertySpec attrProps[] = {
  {"name", 0, PROP_OPT, nodeGetNodeName, NULL},
  {"value", 0, PROP_OPT, nodeGetNodeValue, NULL},
  {"specified", 0, PROP_OPT, attrGetSpecified, NULL},
  {"ownerElement", 0, PROP_OPT, attrGetOwnerElement, NULL},
  {NULL, 0, 0, 0, 0}
};

/******************* character data & Co. **********/

JSClass chardataClass = {
  "CharacterData",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JS_PROP(chardataGetData)
{
  declPRIV;
  if (np) {
    char *val = (char *)NPDOM.getNodeValue(np->node);
    if (val) {
      JSString *jval = JS_NewStringCopyZ(cx, val);
      *rval = STRING_TO_JSVAL(jval);
    } else {
      *rval = JSVAL_NULL;
    }
    NPDOM.freeValue(np->node, val);
    return TRUE;
  }
  else {
    return FALSE;
  }
}

int _getDataLen(const char* data)
{
  return strlen((char *)data);
}

JS_PROP(chardataGetLength)
{
  declPRIV;
  if (np) {
    const char* val = NPDOM.getNodeValue(np->node);
    *rval = INT_TO_JSVAL(_getDataLen(val));
    NPDOM.freeValue(np->node, (char*)val);
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JS_METHOD(chardataSubstringData)
{
  declPRIV;
  if (argc < 2)
    {
      JS_ReportError(cx, 
		     "CharacterData.substringData must have two parameters");
      return FALSE;
    }
  if (np) {
    const char *val = NPDOM.getNodeValue(np->node);
    int len = _getDataLen(val);
    int off = JSVAL_TO_INT(argv[0]);
    int count = JSVAL_TO_INT(argv[1]);
    JSString *str;
    if (off >= len) 
      {
	str = JS_NewStringCopyZ(cx, "");
      } 
    else 
      {
	if (off + count > len) count = len - off;
	str = JS_NewStringCopyN(cx, val + off, count);
      }
    NPDOM.freeValue(np->node, (char*)val);
    *rval = STRING_TO_JSVAL(str);
    return TRUE;
  }
  else {
    return FALSE;
  }
}

JSPropertySpec chardataProps[] = 
{
  {"data", 0, PROP_OPT, chardataGetData, NULL},
  {"length", 0, PROP_OPT, chardataGetLength, NULL},
  {NULL, 0, 0, 0, 0}
};

JSFunctionSpec chardataFunctions[] = 
{
  {"substringData", chardataSubstringData, 0, 0, 0},
  {"appendData", nodeUnsupported, 0, 0, 0},
  {"insertData", nodeUnsupported, 0, 0, 0},
  {"deleteData", nodeUnsupported, 0, 0, 0},
  {"replaceData", nodeUnsupported, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

/********************* text ************************/

JSClass textClass = {
  "Text",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JSFunctionSpec textFunctions[] = 
{
  {"splitText", nodeUnsupported, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

/******************** cdata ************************/

JSClass cdataClass = {
  "CDATASection",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

/***************** comment *************************/

JSClass commentClass = {
  "Comment",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  nodeFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

/********************* named node map **************/

JS_PROP(nnmGetLength)
{
  JSIdArray *arr = JS_Enumerate(cx, obj);
  *rval = INT_TO_JSVAL(arr -> length - 1);
  JS_DestroyIdArray(cx, arr);
  return TRUE;
}

JS_METHOD(nnmGetNamedItem)
{
  JSBool rv;
  if (argc != 1) 
    {
      JS_ReportError(cx, "NamedNodeMap.getNamedItem must have one parameter");
      return FALSE;
    }
  JSString *str = JSVAL_TO_STRING(argv[0]);
  jsval value;
  char *name = JS_GetStringBytes(str);
  rv = JS_GetProperty(cx, obj, name, &value);
  if (rv) *rval = value;
  return rv;
}

JS_METHOD(nnmSetNamedItem)
{
  if (argc != 1) 
    {
      JS_ReportError(cx, "NamedNodeMap.setNamedItem must have one parameter");
      return FALSE;
    }
  //check the type here!!!
  JSObject *nobj = JSVAL_TO_OBJECT(argv[0]);
  JSClass *cls = JS_GET_CLASS(cx, nobj);
  JSObject *proto = JS_GetPrototype(cx, nobj);
  JSContextItem *cxi = (JSContextItem*)JS_GetContextPrivate(cx);
  if (proto == cxi->node || cls == &nodeClass)
    {
      NodePrivate *np = (NodePrivate*)JS_GetPrivate(cx, nobj);
      char *name = (char *)NPDOM.getNodeName(np->node);
      JS_SetProperty(cx, obj, name, &(argv[0]));
      NPDOM.freeName(np->node,name);
      return TRUE;
    }
  else
    {
      JS_ReportError(cx, "invalid NamedNodeMap.setNamedItem parameter");
      return FALSE;
    }
}

JS_METHOD(nnmRemoveNamedItem)
{
  if (argc != 1) 
    {
      JS_ReportError(cx, "NamedNodeMap.removeNamedItem must have one parameter");
      return FALSE;
    }
  JSString *str = JSVAL_TO_STRING(argv[0]);
  char *name = JS_GetStringBytes(str);
  return JS_DeleteProperty(cx, obj, name);
}

JS_METHOD(nnmItem)
{
  if (argc >= 1 || ! JSVAL_IS_INT(argv[0]) )
    {
      JSIdArray *arr = JS_Enumerate(cx, obj);
      int idx = JSVAL_TO_INT(argv[0]);
      if (idx < 0 || idx >= arr -> length - 1)
	{
	  *rval = JSVAL_VOID;
	}
      else
	{
	  jsval val;
	  //skip the 'length' property
	  JS_IdToValue(cx, arr->vector[idx + 1], &val);
	  JS_GetProperty(cx, obj, 
			 JS_GetStringBytes(JSVAL_TO_STRING(val)), rval);
	}
      JS_DestroyIdArray(cx, arr);
      return TRUE;
    }
  else
    {
      JS_ReportError(cx, "NamedNodeMap.item must have one parameter");
      return FALSE;
    }
}

JSPropertySpec nnmProps[] = {
  {"length", 0, PROP_OPT, nnmGetLength, NULL},
  {NULL, 0, 0, 0, 0}
};

JSFunctionSpec nnmFunctions[] = {
  {"getNamedItem", nnmGetNamedItem, 0, 0, 0},
  {"setNamedItem", nnmSetNamedItem, 0, 0, 0},
  {"removeNamedItem", nnmRemoveNamedItem, 0, 0, 0},
  {"item", nnmItem, 0, 0, 0},
  {"getNamedItemsNS", nodeUnsupported, 0, 0, 0},
  {"setNamedItemsNS", nodeUnsupported, 0, 0, 0},
  {"removeNamedItemsNS", nodeUnsupported, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

JSClass nnmClass = {
  "NamedNodeMap", 0,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  JS_FinalizeStub, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

/********************* functions *******************/

JSClass* _jsdom_getNodeClass(SDOM_NodeType type) 
{
  JSClass *cls;
  switch (type) 
    {
    case SDOM_ELEMENT_NODE:
      cls = &elementClass;
      break;
    case SDOM_DOCUMENT_NODE:
      cls = &documentClass;
      break;
    case SDOM_ATTRIBUTE_NODE:
      cls = &attrClass;
      break;
    case SDOM_TEXT_NODE:
      cls = &textClass;
      break;
    case SDOM_CDATA_SECTION_NODE:
      cls = &cdataClass;
      break;
    case SDOM_COMMENT_NODE:
      cls = &commentClass;
      break;
    default:
      {
	cls = &nodeClass;
      }
    }
  return cls;
}

JSClass* _jsdom_getNodeClass(Sit S, JSContext *cx, NodeHandle node) 
{
  return _jsdom_getNodeClass((SDOM_NodeType)S.dom().getNodeType(node));
}

JSObject* jsdom_wrapNode(Sit S, JSContext *cx, NodeHandle node) 
{
  JSObject *obj = NULL;
  if (node)
    {
      JSContextItem *item = (JSContextItem*)JS_GetContextPrivate(cx);
      sabassert(item);
      
      SDOM_NodeType type = (SDOM_NodeType)S.dom().getNodeType(node);

      obj = JS_NewObject(cx, _jsdom_getNodeClass(type), 
				   item->node, NULL);
      JS_SetPrivate(cx, obj, _jsdom_getNodePrivate(S, node));

      //common methods and properties
      JS_DefineProperties(cx, obj, nodeProps);
      JS_DefineFunctions(cx, obj, nodeFunctions);
      
      //tweak the class
      switch (type) 
	{
	case SDOM_ELEMENT_NODE:
	  {
	    JS_DefineProperties(cx, obj, elementProps);
	    JS_DefineFunctions(cx, obj, elementFunctions);
	  }; break;
	case SDOM_DOCUMENT_NODE:
	  {
	    JS_DefineProperties(cx, obj, documentProps);
	    JS_DefineFunctions(cx, obj, documentFunctions);
	  }; break;
	case SDOM_ATTRIBUTE_NODE:
	  {
	    JS_DefineProperties(cx, obj, attrProps);
	  }; break;
	case SDOM_TEXT_NODE:
	  {
	    JS_DefineProperties(cx, obj, chardataProps);
	    JS_DefineFunctions(cx, obj, chardataFunctions);
	    //text specs.
	    JS_DefineFunctions(cx, obj, textFunctions);
	  }; break;
	case SDOM_CDATA_SECTION_NODE:
	  {
	    JS_DefineProperties(cx, obj, chardataProps);
	    JS_DefineFunctions(cx, obj, chardataFunctions);
	  }; break;
	case SDOM_COMMENT_NODE:
	  {
	    JS_DefineProperties(cx, obj, chardataProps);
	    JS_DefineFunctions(cx, obj, chardataFunctions);
	  }; break;
	}
    }
  return obj;
}

JSObject* jsdom_createNode(JSContext *cx, NodePrivate *np_, NodeHandle node) 
{
  return jsdom_wrapNode(*(np_->situa), cx, node);
}


void jsdom_delegateDOM(JSContext *cx)
{
  JSContextItem *item = (JSContextItem*)JS_GetContextPrivate(cx);
  JSObject *obj;
  //Node prototype
  obj = JS_DefineObject(cx, JS_GetGlobalObject(cx), "Node",
			&nodeClass, NULL, 
			JSPROP_ENUMERATE | 
			JSPROP_READONLY |
			JSPROP_PERMANENT);
  JS_DefineProperties(cx, obj, nodeProtoProps);
  item -> node = obj;
  //DOMException prototype
  obj = JS_DefineObject(cx, JS_GetGlobalObject(cx), "DOMException",
			&domexClass, NULL, 
			JSPROP_ENUMERATE | 
			JSPROP_READONLY |
			JSPROP_PERMANENT);
  JS_DefineProperties(cx, obj, domexProtoProps);
  item -> domex = obj;
  //DOMImplementation
  obj = JS_DefineObject(cx, JS_GetGlobalObject(cx), "DOMImplementation",
			&domexClass, NULL, 
			JSPROP_ENUMERATE | 
			JSPROP_READONLY |
			JSPROP_PERMANENT);
  JS_DefineFunctions(cx, obj, domimplFunctions);
  item -> domimpl = obj;
  //nodelist class
  JSObject *nlclass = JS_InitClass(cx, JS_GetGlobalObject(cx),
				   NULL,
				   &nlistClass, nlistConstructor, 0,
				   NULL, NULL, NULL, NULL);
  item -> nlclass = nlclass;
  //nodelist class
  //  JSObject *nnmclass = JS_InitClass(cx, JS_GetGlobalObject(cx),
//  				    NULL,
//  				    &nnmClass, NULL, 0,
//  				    NULL, nnmFunctions, NULL, NULL);
//    item -> nnmclass = nnmclass;
}

void jsdom_raiseException(JSContext *cx, int code) 
{
  JSContextItem *item = (JSContextItem*)JS_GetContextPrivate(cx);
  sabassert(item);

  JSObject *obj = JS_NewObject(cx, &domexClass, item -> domex, NULL);
  //set private
  DomExPrivate *c = new DomExPrivate;
  c -> code = code;
  JS_SetPrivate(cx, obj, c);
  //instance props. and funcs.
  JS_DefineProperties(cx, obj, domexProps);
  JS_DefineFunctions(cx, obj, domexFunctions);
  //raise exception
  JS_SetPendingException(cx, OBJECT_TO_JSVAL(obj));
}

JSObject* jsdom_createNodeList(JSContext *cx, int num)
{
  //JSObject *obj = JS_NewArrayObject(cx, 0, NULL);
  //JS_DefineFunctions(cx, obj, nlistFunctions);
  JSObject *obj = JS_ConstructObject(cx, &nlistClass, NULL, NULL);

  return obj;
}

JSObject* jsdom_createNamedNodeMap(JSContext *cx, int num)
{
  JSObject *obj = JS_NewObject(cx, &nnmClass, NULL, NULL);
  JS_DefineFunctions(cx, obj, nnmFunctions);
  JS_DefineProperties(cx, obj, nnmProps);

  return obj;
}

#endif //ENABLE_JS
