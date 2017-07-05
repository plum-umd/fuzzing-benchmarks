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

#include "jsext.h"

#ifdef ENABLE_JS

#include "expr.h"
#include "error.h"
#include "context.h"
#include "guard.h"
#include "domprovider.h"
#include "jsdom.h"

/************* extern constants ******************/
const char* theArrayProtoRoot = "_array_prototype_root_";

/*************** JS error reporter ******************/

void SJSErrorReporter(JSContext *cx, const char *message,
		      JSErrorReport *report)
{
  JSContextItem *item = (JSContextItem*)JS_GetContextPrivate(cx);
  if (item) {
    if (item -> errInfo.message) delete item -> errInfo.message;
    if (item -> errInfo.token) delete item -> errInfo.token;
    item -> errInfo.message = NULL;
    item -> errInfo.token = NULL;
    if (message) {
      item -> errInfo.message = new char[strlen(message) + 1];
      strcpy(item -> errInfo.message, message);
    }
    if (report -> tokenptr) {
      item -> errInfo.token = new char[strlen(report -> tokenptr) + 1];
      strcpy(item -> errInfo.token, report -> tokenptr);
    }
    item -> errInfo.line = report -> lineno;
    item -> errInfo.errNumber = report -> errorNumber;
  }
}

/************************ MANAGER *******************/

JSRuntime_Sab* gJSRuntime;

JSRuntime_Sab* JSManager::getRuntime()
{
  if (! gJSRuntime ) {
    gJSRuntime = JS_NewRuntime(JS_RUNTIME_SIZE);
  }
  return gJSRuntime;
}

JSContext_Sab* JSManager::createContext(int size /* =JS_CONTEXT_SIZE */) 
{
  return JS_NewContext(getRuntime(), size);
}


void JSManager::finalize() 
{
  if ( gJSRuntime ) JS_DestroyRuntime(gJSRuntime);
}

/******************************delegates etc. ******************/

JS_METHOD(jsglobalLog) {
  JSContextItem *item = (JSContextItem*)JS_GetContextPrivate(cx);
  Situation *sit = item -> proc -> recallSituation();
  JSString *str = JS_ValueToString(cx, argv[0]);
  char *msg = JS_GetStringBytes(str);
  sit -> message(MT_LOG, L_JS_LOG, (const char*) msg, (const char*)NULL);
  return TRUE;
}

/****************************************************************

JSContexts

****************************************************************/

JSContextItem::JSContextItem(JSContext_Sab *cx_, Str &uri_, Processor *proc_) 
  : cx(cx_), uri(uri_), proc(proc_)
{
  //cls = NULL;
  errInfo.message = NULL;
  errInfo.token = NULL;
  node = NULL;
  domex = NULL;
  domimpl = NULL;
  nlclass = NULL;
  array_proto = NULL;
};

JSContextItem::~JSContextItem()
{
  names.freeall(FALSE);
  if (errInfo.message) delete errInfo.message;
  if (errInfo.token) delete errInfo.token;
  if (cx) 
    {
#ifdef ENABLE_JS_THREADS
      JS_ResumeRequest(cx);
#endif
      if (array_proto) JS_RemoveRoot(cx, &array_proto);
#ifdef ENABLE_JS_THREADS
      JS_EndRequest(cx);
#endif
      JS_GC(cx);
      JS_DestroyContext(cx);
    }
  //if (cls) delete cls;
}

//js class for global object
JSClass sabGlobalClass = {
  "global",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub,
  JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,
  JS_FinalizeStub, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JSContextItem* JSContextList::find(Str uri, Bool canCreate /*TRUE*/)
{
  JSContextItem *item = NULL;
  for (int i = 0; i < number(); i++) 
    {
      if ((*this)[i] -> uri == uri) item = (*this)[i];
    }
  if (!item && canCreate) 
    {
      JSContext_Sab *cx;
      cx = JSManager::createContext();
#ifdef ENABLE_JS_THREADS
      JS_BeginRequest(cx);
#endif
      item = new JSContextItem(cx, uri, proc);
      append(item);
      //initalize context
      //JSClass *cls = SJSGetGlobalClass();
      //item -> cls = cls;
      JSObject *global = JS_NewObject(cx, &sabGlobalClass, NULL, NULL);
      JS_InitStandardClasses(cx, global);
      //set debugging globals
      JS_DefineFunction(cx, global, "log", jsglobalLog, 1, 0);
      //private data and error handling
      JS_SetContextPrivate(cx, (void*)item);
      JS_SetErrorReporter(cx, SJSErrorReporter);
      //create persistent objects
      jsdom_delegateDOM(cx);
#ifdef ENABLE_JS_THREADS
      JS_Suspendrequest(cx);
#endif
    }
  return item;
}

/************************************************************/
// JSExternalPrivate
/************************************************************/

JSExternalPrivate::JSExternalPrivate(void *p_, void *v_)
: priv(p_), value(v_), refcnt(1) 
{
  if (value)
    {
      JS_AddRoot((JSContext*)priv, &value);
    }
};

JSExternalPrivate::~JSExternalPrivate() 
{
  if (value) 
    {
      JS_RemoveRoot((JSContext*)priv, &value); 
    }
}

/**************** ordinary functions */

Bool sjs_instanceOf(JSContext_Sab *cx, JSObject_Sab *obj, JSClass_Sab *cls)
{
  JSObject *o = obj;
  while (o)
    {
      JSClass *c = JS_GET_CLASS(cx, o);
      if (c && c == cls) return TRUE;
      o = JS_GetPrototype(cx, o);
    }
  return FALSE;
}

//  const char* gClassName = "global";

//  JSClass* SJSGetGlobalClass() 
//  {
//  #ifdef HAVE_JSAPI_H
//    JSClass *ret = new JSClass();
//    ret -> name = gClassName;
//    ret -> flags = 0;
//    ret -> addProperty = JS_PropertyStub;
//    ret -> delProperty = JS_PropertyStub;
//    ret -> getProperty = JS_PropertyStub;
//    ret -> setProperty = JS_PropertyStub;
//    ret -> enumerate = JS_EnumerateStub;
//    ret -> resolve = JS_ResolveStub;
//    ret -> convert = JS_ConvertStub;
//    ret -> finalize = JS_FinalizeStub;
  
//    return ret;
//  #else
//    return NULL;
//  #endif
//  }

Bool SJSEvaluate(JSContextItem &item, DStr &script)
{
  jsval rval;
  JSBool status;
  JSContext *cx = item.cx;

#ifdef ENABLE_JS_THREADS
  JS_ResumeRequest(cx);
#endif

  /*
    int len = utf8StrLength((const char*) script);
    wchar_t *ucscript = new wchar_t[len + 1];
    utf8ToUtf16(ucscript, (const char*) script);
    
    status = JS_EvaluateUCScript(cx, JS_GetGlobalObject(cx), 
    (jschar*)ucscript, len,
    "stylesheet", 0, &rval);
    delete ucscript;
  */
  
  char *scr = (char *) script;
  status = JS_EvaluateScript(cx, JS_GetGlobalObject(cx), 
			     scr, strlen(scr),
			     "stylesheet", 0, &rval);
  if ( JS_IsExceptionPending(cx) ) 
    {
      JS_ClearPendingException(cx);
    }
  
  if (status) {
    //read all functions
    JSIdArray *arr = JS_Enumerate(cx, JS_GetGlobalObject(cx));
    item.names.freeall(FALSE);
    if (arr) {
      for (int i = 0; i < arr -> length; i++) {
	jsval propname;
	jsid id = arr -> vector[i];
	JS_IdToValue(cx, id, &propname);
	
	JSString *str = JS_ValueToString(cx, propname);
	jsval prop;
	JS_GetProperty(cx, JS_GetGlobalObject(cx), 
		       JS_GetStringBytes(str), &prop);
	
	JSFunction *func = JS_ValueToFunction(cx, prop);
	if (func) {
	  const char* fname = JS_GetFunctionName(func);
	  item.names.append(new Str(fname));
	}
      }
      JS_DestroyIdArray(cx, arr);
    }
  } 
  else { //error occured
    
  }

#ifdef ENABLE_JS_THREADS
  JS_SuspendRequest(cx);
#endif

  return (Bool)status;
}

/************** XSLTContext *******************/

struct XSLTContextPrivate {
  Context *ctx;
  Situation *situa;
};

void ctxFinalize(JSContext *cx, JSObject *obj)
{
  XSLTContextPrivate *priv = (XSLTContextPrivate*)JS_GetPrivate(cx, obj);
  if (priv) delete priv;
}

JS_PROP(ctxGetPosition)
{
  XSLTContextPrivate *priv = (XSLTContextPrivate*)JS_GetPrivate(cx, obj);
  if (priv) {
    *rval = INT_TO_JSVAL(priv -> ctx -> getPosition() + 1);
    return TRUE;
  } 
  else {
    return FALSE;
  };
}

JS_PROP(ctxGetSize)
{
  XSLTContextPrivate *priv = (XSLTContextPrivate*)JS_GetPrivate(cx, obj);
  if (priv) {
    *rval = INT_TO_JSVAL(priv -> ctx -> getSize());
    return TRUE;
  } 
  else {
    return FALSE;
  };
}

JS_PROP(ctxGetContextNode)
{
  XSLTContextPrivate *priv = (XSLTContextPrivate*)JS_GetPrivate(cx, obj);
  if (priv) {
    JSObject *obj = jsdom_wrapNode(*(priv->situa), cx, priv->ctx->current());
    *rval = OBJECT_TO_JSVAL(obj);
    return TRUE;
  } else {
    return FALSE;
  }
}

JS_PROP(ctxGetCurrentNode)
{
  XSLTContextPrivate *priv = (XSLTContextPrivate*)JS_GetPrivate(cx, obj);
  if (priv) {
    JSObject *obj = jsdom_wrapNode(*(priv->situa), cx, 
				   priv->ctx->getCurrentNode());
    *rval = OBJECT_TO_JSVAL(obj);
    return TRUE;
  } else {
    return FALSE;
  }
}

JS_PROP(ctxGetOwnerDocument)
{
  XSLTContextPrivate *priv = (XSLTContextPrivate*)JS_GetPrivate(cx, obj);
  if (priv) {
    Tree &tree = toV(priv->ctx->current())->getOwner();
    JSObject *obj = jsdom_wrapNode(*(priv -> situa), cx, 
				   &(tree.getRoot()));
    *rval = OBJECT_TO_JSVAL(obj);
    return TRUE;
  } else {
    return FALSE;
  }
}

JS_METHOD(ctxSystemProperty)
{
  JSString *str =  JS_NewStringCopyN(cx, "not supported", 13);
  *rval = STRING_TO_JSVAL(str);
  return TRUE;
}

JS_METHOD(ctxStringValue)
{
//    if (argc > 0 && JSVAL_IS_OBJECT(argv[0]) 
//        && sjs_instanceOf(cx, JSVAL_TO_OBJECT(argv[0]), &nodeClass))
//      {
//        JSObject *node = JSVAL_TO_OBJECT(argv[0]);
//        NodePrivate *np = (NodePrivate*)JS_GetPrivate(cx, obj);
//        sabassert(np);
//        DStr val;
//        np->situa->dom().constructStringValue(np->node, val);
//        JSString *str = JS_NewStringCopyZ(cx, (char*)val);
//        *rval = STRING_TO_JSVAL(str);
//        return TRUE;
//      } else {
//        return FALSE;
//      }
  DOM_EX( 9 );
  return TRUE;
}

JSClass ctxClass = {
  "XSLTContext",
  JSCLASS_HAS_PRIVATE,
  JS_PropertyStub, JS_PropertyStub,
  JS_PropertyStub, JS_PropertyStub,
  JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, 
  ctxFinalize, 0, 0, NULL, NULL, NULL, NULL, 0, 0
};

JSPropertySpec ctxProps[] =
{
  {"contextPosition", 0, PROP_OPT, ctxGetPosition, NULL},
  {"contextSize", 0, PROP_OPT, ctxGetSize, NULL},
  {"contextNode", 0, PROP_OPT, ctxGetContextNode, NULL},
  {"currentNode", 0, PROP_OPT, ctxGetCurrentNode, NULL},
  {"ownerDocument", 0, PROP_OPT, ctxGetOwnerDocument, NULL},
  {NULL, 0, 0, 0, 0}
};

JSFunctionSpec ctxFunctions[] = 
{
  {"systemProperty", ctxSystemProperty, 0, 0, 0},
  {"stringValue", ctxStringValue, 0, 0, 0},
  {NULL, 0, 0, 0, 0}
};

void sjs_PublishContext(Sit S, JSContext_Sab *cx, Context *c) 
{
  JSObject *obj = JS_DefineObject(cx, JS_GetGlobalObject(cx), "XSLTContext",
				  &ctxClass, NULL, 
				  JSPROP_ENUMERATE | 
				  JSPROP_READONLY);
  
  XSLTContextPrivate *priv = new XSLTContextPrivate;
  priv -> ctx = c;
  priv -> situa = &S;
  JS_SetPrivate(cx, obj, priv);
  JS_DefineProperties(cx, obj, ctxProps);
  JS_DefineFunctions(cx, obj, ctxFunctions);
}

/************* function call ******************/
Bool SJSCallFunction(Sit S, Context *c, JSContextItem &item, Str &name, 
		      ExprList &atoms, Expression &retxpr)
{
  JSContext *cx = item.cx;
  sabassert(cx);

#ifdef ENABLE_JS_THREADS
  JS_ResumeRequest(cx);
#endif

  sjs_PublishContext(S, cx, c);

  //get args
  int argc = atoms.number();
  jsval *args = new jsval[argc];
  for (int i = 0; i < argc; i++) {
    switch (atoms[i] -> type) {
    case EX_NUMBER:
      {
	jsdouble d = (double)(atoms[i]->tonumber(S));
	JS_NewDoubleValue(cx, d, &args[i]);
      }; break;
    case EX_BOOLEAN:
      {
	args[i] = BOOLEAN_TO_JSVAL(atoms[i]->tobool());
      }; break;
    case EX_STRING:
      {
	Str str;
	atoms[i]->tostring(S, str);
	char *p = (char*)str;
	JSString *s = JS_NewStringCopyN(cx, p, strlen(p));
	args[i] = STRING_TO_JSVAL(s);
      }; break;
    case EX_NODESET:
      {
	const Context &ctx = atoms[i]->tonodesetRef();
	int num = ctx.getSize();
	JSObject *arr = jsdom_createNodeList(cx, num);
	args[i] = OBJECT_TO_JSVAL(arr);
	//iterate
	for (int j = 0; j < num; j++) {
	  NodeHandle node = ctx[j];
	  jsval val;// = new jsval;
	  val = OBJECT_TO_JSVAL(jsdom_wrapNode(S, cx, node));
	  JS_SetElement(cx, arr, j, &val);
	}
      }; break;
    case EX_EXTERNAL:
      {
	External e;
	e.assign(atoms[i]->toexternal(S));
	JSObject *o = (JSObject*)e.getValue();
	if ( o )
	  {
	    args[i] = OBJECT_TO_JSVAL(o);
	  }
	else
	  {
	    args[i] = JSVAL_NULL;
	  }
      }; break;
    default:
      {
	//convert to string
	Str str;
	atoms[i]->tostring(S, str);
	char *p = (char*)str;
	JSString *s = JS_NewStringCopyN(cx, p, strlen(p));
	args[i] = STRING_TO_JSVAL(s);
      }
    }
  }
  
  //call function
  jsval rval;
  JSBool status = JS_CallFunctionName(cx, JS_GetGlobalObject(cx), 
				      (char*)name, argc, args, &rval);
  if ( JS_IsExceptionPending(cx) ) 
    {
      JS_ClearPendingException(cx);
    }

  //remove XSLT context
  JS_DeleteProperty(cx, JS_GetGlobalObject(cx), "XSLTContext");

  delete[] args;

  if (status) {
    if (JSVAL_IS_VOID(rval))
      {
	//return an emtpy nodeset
	GP( Context ) newc = new Context(c->getCurrentNode());
	retxpr.setAtom(newc.keep());
      }
    if (JSVAL_IS_NULL(rval)) 
      {
	External e(cx, NULL);
	retxpr.setAtom(e);
      }
    if (JSVAL_IS_OBJECT(rval)) 
      {
	JSObject *obj = JSVAL_TO_OBJECT(rval);
	//node lists
	if (sjs_instanceOf(cx, obj, &nlistClass))
	  {
	    jsuint len;
	    JS_GetArrayLength(cx, obj, &len);
	    GP( Context ) newc = new Context(c->getCurrentNode());
	    for (unsigned int i = 0; i < len; i++)
	      {
		jsval jnode;
		JS_GetElement(cx, obj, i, &jnode);
		JSObject *onode = JSVAL_TO_OBJECT(jnode);
		if (onode && sjs_instanceOf(cx, onode, &nodeClass))
		  {
		    NodePrivate *priv = (NodePrivate*)JS_GetPrivate(cx, onode);
		    sabassert(priv);
		    (*newc).append(priv -> node);
		  }
	      }
	    retxpr.setAtom(newc.keep());
	  }
	//single nodes
	else if (sjs_instanceOf(cx, obj, &nodeClass))
	  {
	    GP( Context ) newc = new Context(c->getCurrentNode());

	    NodePrivate *priv = (NodePrivate*)JS_GetPrivate(cx, obj);
	    sabassert(priv);
	    (*newc).append(priv -> node);

	    retxpr.setAtom(newc.keep());
	  }
	//other objects
	else
	  {
	    External e(cx, obj);
	    retxpr.setAtom(e);
	  }
      }
    //strings
    else if (JSVAL_IS_STRING(rval))
      {
	JSString *str = JS_ValueToString(cx, rval);
	Str rstr = (char*) JS_GetStringBytes(str);
	retxpr.setAtom(rstr);
      }
    else if (JSVAL_IS_NUMBER(rval))
      {
	jsdouble d;
	JS_ValueToNumber(cx, rval, &d);
	Number num((double)d);
	retxpr.setAtom(num);
      }
    else if (JSVAL_IS_BOOLEAN(rval))
      {
	retxpr.setAtom(JSVAL_TO_BOOLEAN(rval));
      }
    else 
      {
	//all other types are treated as strings (shouldn't happen)
	JSString *str = JS_ValueToString(cx, rval);
	Str rstr = (char*) JS_GetStringBytes(str);
	retxpr.setAtom(rstr);
      }
  }

#ifdef ENABLE_JS_THREADS
  JS_SuspendRequest(cx);
#endif

  //run GC
  JS_MaybeGC(cx);
  //JS_GC(cx);
  return status;
}

#endif //ENABLE_JS
