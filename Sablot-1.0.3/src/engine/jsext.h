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

#ifndef JSExtHIncl
#define JSExtHIncl

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_JS

#include "datastr.h"
#include "base.h"

//hardcoded value - fix on Win etc.
#define XP_UNIX
#include <jsapi.h>

#define JSRuntime_Sab JSRuntime
#define JSContext_Sab JSContext
#define JSClass_Sab JSClass
#define JSObject_Sab JSObject

//macros for method/property declarations
#define JS_METHOD(name) JSBool name(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
#define JS_PROP(name) JSBool name(JSContext *cx, JSObject *obj, jsval id, jsval *rval)
#define PROP_OPT  JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT

#define JS_RUNTIME_SIZE 0x100000
#define JS_CONTEXT_SIZE 0x20000

//forwards
class Context;
class Processor;

class JSManager 
{
public:
  static JSRuntime_Sab* getRuntime(); 
  static JSContext_Sab* createContext(int size = JS_CONTEXT_SIZE);
  static void finalize();
private:
};

/*******************************************************
JS contexts
*******************************************************/
extern const char* theArrayProtoRoot;

class JSContextItem 
{
public: 
  JSContextItem(JSContext_Sab *cx_, Str &uri_, Processor* proc);
  ~JSContextItem();
  JSContext_Sab *cx;
  //JSClass_Sab *cls;
  Str uri;
  PList<Str*> names;
  Processor *proc     ;
  struct {
    char *message;
    char *token;
    int line;
    int errNumber;
  } errInfo;
  //prototypes
  JSObject_Sab *node;
  JSObject_Sab *domex;
  JSObject_Sab *domimpl;
  JSObject_Sab *nlclass;
  JSObject_Sab *nnmclass;
  JSObject_Sab *array_proto;
};

class JSContextList: public PList<JSContextItem*>
{
 public:
  JSContextList(Processor* proc_) : proc(proc_) {};
  JSContextItem* find(Str uri, Bool canCreate = FALSE);
 private:
  Processor* proc;
};

class External;

class JSExternalPrivate 
{
  friend class External;
public:
  JSExternalPrivate(void *p_, void *v_);
  ~JSExternalPrivate();
private:
  void *priv;
  void *value;
  int refcnt;
};

/************** ordinary functions ****************/
JSClass_Sab* SJSGetGlobalClass();
Bool SJSEvaluate(JSContextItem &item, DStr &script);
Bool SJSCallFunction(Sit S, Context *c, JSContextItem &item, Str &name, 
		      ExprList &atoms, Expression &retxpr);

#endif //ENABLE_JS

#endif //JSExtHIncl
