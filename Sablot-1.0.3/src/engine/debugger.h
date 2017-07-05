/* -*- mode: c++ -*-
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

#ifndef DebuggerHIncl
#define DebuggerHIncl

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef SABLOT_DEBUGGER

#include "base.h"
#include "datastr.h"
#include "shandler.h"

class Element;
class Context;
class Tree;
class SubtreeInfo;
class Expression;

/*helper classes */
class ArgList: public PList<Str*> {};

class Breakpoint {
 public:
  Breakpoint(): line(-1), enabled(true), 
    valid(false), element(NULL), expr(NULL), ignoreNum(0),
    hitTotal(0), hitEnabled(0), hitMatched(0), hitBreak(0) {};
  ~Breakpoint();
  //methods
  Bool buildExpression(Sit);
  Bool matches(Sit S, Context* c, Bool& wasError);
  Bool countMatches(Sit S, Context* c, Bool& wasError);
  void reset();
  Bool countEnabled();
  //fields
  Str uri;
  int line;
  Str condition;
  Bool enabled;
  Bool valid;
  Element* element;
  Expression *expr;
  int ignoreNum;
  int hitTotal;
  int hitEnabled;
  int hitMatched;
  int hitBreak;
};

class BPList: public PList<Breakpoint*> {
 public:
  ~BPList() { freeall(FALSE); };
  Breakpoint* setBP(Str& uri, int line);
  Breakpoint* getBP(Str& uri, int line);
  Bool clearBP(int idx, Bool running);
  Bool disableBP(int idx);
  void clearAll(Bool);
  void listAll(Bool);
  void statAll(Bool);
  void getLinesForURI(Str& uri, List<int>& lines);
  void reset();
  Bool ignore(long int idx, long int num);
  Bool setExpression(Sit, int, ArgList&, Bool);
};


enum DbgCommand 
{
  SDBG_NOOP,
  SDBG_BATCH,
  SDBG_BREAK,
  SDBG_BREAK_CLEAR,
  SDBG_BREAK_CLEARALL,
  SDBG_BREAK_DISABLE,
  SDBG_BREAK_LIST,
  SDBG_BREAK_COND,
  SDBG_BREAK_STAT,
  SDBG_BREAK_IGNORE,
  SDBG_CONTEXT,
  SDBG_CONTINUE,
  SDBG_KILL,
  SDBG_DATA,
  SDBG_EVAL,
  SDBG_NEXT,
  SDBG_FINISH,
  SDBG_OUTPUT,
  SDBG_STEP,
  SDBG_PARAM,
  SDBG_PARAM_CLEAR,
  SDBG_PARAM_LIST,
  SDBG_POINT,
  SDBG_QUIT,
  SDBG_RUN,
  SDBG_SHEET,
  SDBG_TEMPLATE,
  SDBG_TEST,
  SDBG_HELP
};

struct DbgCommandDescription {
  const char* command;
  int argsMin;
  int argsMax;
  Bool joinRest; //take rest params as one
  Bool strict; //prevent ambiguous abbreviations
  DbgCommand op;
};

class Debugger 
{
 public:
  Debugger();
  ~Debugger();
  Bool isRunning() { return running; };
  Bool outputActive() {return output; };
  void enterIdle();
  eFlag prompt(Sit S, Element *e, Context *c, Bool& quited);
  eFlag breakOnElement(Sit S, Element *e, Context *c);
  void setBreakpoints(Sit S, Tree*);
  void printOutput(const char* buff, int size);
 private:
  eFlag doCommand(Sit S, Element *e, Context *c, char* line, Bool&, Bool&);
  void reportTrace(const char* msg, Str& uri, int line, Element *e);
  Bool elementBreakable(Sit S, Element *e, Context *c, Bool report);
  Bool tokenize(char* what, ArgList &where, DbgCommandDescription& desc);
  Bool lookupArg(Str &str, char** lst, int &op);
  Bool lookupCommand(Str& str, DbgCommandDescription& out);
  const char* nodeTypeName(Vertex *v);
  void listNode(Sit S, NodeHandle node, Bool detailed);
  void listContext(Sit S, Element *e, Context *c, ArgList &args);
  void setBreakpoint(Sit S, Element *e, ArgList& args);
  void doSetBreakpoint(Str& uri, int line);
  void setBreakpointsOnElement(Sit S, Element *e);
  Bool findElementForBreakpoint(Element *e, Breakpoint* bp);
  Bool setBreakpointRunning(Breakpoint *bp);
  Bool checkIdle();
  Bool checkRunning();
  void run();
  void reportError(const char* msg, Bool details);
  void eval(Sit S, Context *c, Element *e, ArgList& args);
  void playBatch(Sit S, Element *e, Context *c, Str& file);
  void printUsage();
  void addParam(ArgList& args);
  void report(Sit S, MsgType type, MsgCode code, 
	      const Str& arg1, const Str& arg2) const;
  Bool checkArgs(DbgCommandDescription& desc, ArgList& args);
  static MH_ERROR mhMakeCode(void *userData, SablotHandle processor_,
			     int severity, unsigned short facility, 
			     unsigned short code);
  static MH_ERROR mhLog(void *userData, SablotHandle processor_,
			MH_ERROR code, MH_LEVEL level, char **fields);
  static MH_ERROR mhError(void *userData, SablotHandle processor_,
			  MH_ERROR code, MH_LEVEL level, 
			  char **fields);
  BPList bpoints;
  Bool running;
  SablotHandle situa;
  Str sheetURI;
  Str dataURI;
  Bool instep;
  Bool intempl;
  Element* nextele;
  Bool output;
  int outputLine;
  int outputLastLine;
  MessageHandler msgHandler;
  StrStrList messages;
  StrStrList params;
  SubtreeInfo *currentInfo; //used for bp setting
  List<int> currentLines; //used for bp setting
  Bool inRightTree;
  Tree *sheet;
  int errorCode;
};


//globals
extern Debugger* debugger;

#endif //SABLOT_DEBUGGER
#endif //DebuggerHIncl
