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

#include "debugger.h"

#ifdef SABLOT_DEBUGGER

#include "sabdbg.h"
#include "datastr.h"
#include "situa.h"
#include "context.h"
#include "verts.h"
#include "tree.h"
#include "uri.h"
#include "guard.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef SABLOT_GPL
#ifdef HAVE_READLINE_READLINE_H
#include <readline/readline.h>
#endif
#ifdef HAVE_READLINE_HISTORY_H
#include <readline/history.h>
#endif
#endif

////////////////////////////////////////////////////////////
//globals
////////////////////////////////////////////////////////////

Debugger* debugger = NULL;

void debuggerInit()
{
  sabassert(!debugger);
  debugger = new Debugger();
}

void debuggerDone()
{
  delete debugger;
}

void debuggerEnterIdle()
{
  debugger -> enterIdle();
}

char* dbgGetLine()
{
  char* aux;
#ifdef HAVE_READLINE_READLINE_H
  aux = readline("sablot> ");
#ifdef HAVE_READLINE_HISTORY_H
  if (aux)
    add_history(aux);
#endif
#else
  aux = (char*)malloc(256);
  printf("sablot> ");
  fgets(aux, 256, stdin);
#endif
  return aux;
}

////////////////////////////////////////////////////////////
// help string
////////////////////////////////////////////////////////////

const char* theUsageStr =
"Breakpoints:\n" \
"  break filename:line  - set the breakpoint\n" \
"  bstat                - show breakpoint stats (total/enabled/break)\n" \
"  B                    - list all breakpoints\n" \
"  condition num cond   - for the breakpoint NUM set the condition COND\n" \
"  del num              - delete the breakpoint NUM\n" \
"  disable num          - (toggle) the breakpoint number NUM\n" \
"  D                    - delete all breakpoints\n" \
"  ignore num count     - ignore the breakpoint NUM for COUNT times\n" \
"Execution control:\n" \
"  continue             - continue the execution\n" \
"  finish               - finish the current node parent\n" \
"  kill                 - stop the processing immediately\n" \
"  next                 - go to the next sibling\n" \
"  run                  - run the processor\n" \
"  step                 - continue until the next element\n" \
"  templ                - continue until the next template executed\n" \
"Processed data:\n" \
"  data                 - set the data file\n" \
"  param name value     - set the external parameter\n" \
"  P                    - list all params\n" \
"  PP                   - clear all params\n" \
"  sheet                - set the stylesheet\n" \
"Evaluation:\n" \
"  eval                 - evaluate the XPath expression\n" \
"  x [list | num]       - examine the current context\n" \
"Miscellaneous:\n" \
"  batch filename       - load the command set for file\n" \
"  help                 - print this help\n" \
"  output               - toggles output on/off\n" \
"  point                - show where you are\n" \
"  quit                 - quite the debgger\n";




////////////////////////////////////////////////////////////
// helpers
////////////////////////////////////////////////////////////

DbgCommandDescription theNullCommand =  
  {NULL,          0,  0, false, false, SDBG_NOOP};

/*!!!!! MUST be sorted by command !!!! */
DbgCommandDescription dbgCommands[] = {
  { "break",      1,  1, false, false, SDBG_BREAK },
  { "batch",      1,  1, false, false, SDBG_BATCH },
  { "bstat",      0,  0, false, false, SDBG_BREAK_STAT },
  { "B",          0,  0, false, false, SDBG_BREAK_LIST },
  { "continue",   0,  0, false, false, SDBG_CONTINUE },
  { "condition",  1,  2, true,  false, SDBG_BREAK_COND },
  { "del",        1,  1, false, false, SDBG_BREAK_CLEAR },
  { "data",       0,  1, false, false, SDBG_DATA },
  { "disable",    1,  1, false, false, SDBG_BREAK_DISABLE },
  { "D",          0,  0, false, false, SDBG_BREAK_CLEARALL },
  { "eval",       1,  1, true,  false, SDBG_EVAL },
  { "finish",     0,  0, false, false, SDBG_FINISH },
  { "help",       0,  0, false, false, SDBG_HELP },
  { "ignore",     2,  2, false, false, SDBG_BREAK_IGNORE },
  { "kill",       0,  0, false, false, SDBG_KILL },
  { "next",       0,  0, false, false, SDBG_NEXT },
  { "output",     0,  0, false, false, SDBG_OUTPUT },
  { "param",      2,  2, true,  false, SDBG_PARAM },
  { "point",      0,  0, false, false, SDBG_POINT },
  { "P",          0,  0, false, false, SDBG_PARAM_LIST },
  { "PP",         0,  0, false, false, SDBG_PARAM_CLEAR },
  { "run",        0,  0, false, false, SDBG_RUN },
  { "step",       0,  0, false, false, SDBG_STEP },
  { "sheet",      0,  1, false, false, SDBG_SHEET },
  { "templ",      0,  0, false, false, SDBG_TEMPLATE },
  { "quit",       0,  0, false, false, SDBG_QUIT },
  { "x",          0,  1, false, false, SDBG_CONTEXT },
  {NULL,          0,  0, false, false, SDBG_NOOP}
};

char *ctxArgs[] = {
  "list", "current", NULL
};

////////////////////////////////////////////////////////////
// globals
////////////////////////////////////////////////////////////

Bool strToInt(Str *str, long int& out)
{
  char *val = (char*)*str;
  char *end;
  out = strtol(val, &end, 10);
  return !*end;
}

////////////////////////////////////////////////////////////
// Breakpoint
////////////////////////////////////////////////////////////

Breakpoint::~Breakpoint()
{
  if (expr) delete expr;
}

Bool Breakpoint::buildExpression(Sit S)
{
  if (valid && element && !condition.isEmpty())
    {
      GP(Expression) e = new Expression(*element);
      Bool failed = (*e).parse(S, condition);
      if (!failed)
	{
	  expr = e.keep();
	}
      else
	{
	  enabled = false;
	  return false;      
	}
    }
  return true;
}

Bool Breakpoint::matches(Sit S, Context* c, Bool& wasError)
{
  wasError = false;
  if (valid && expr)
    {
      sabassert(element);
      Expression ret(*element);
      Bool failed = expr -> eval(S, ret, c);
      if (! failed)
	{
	  return ret.tobool();
	}
      else
	{
	  wasError = true;
	  return true;
	}
    }
  else
    return true;
}

Bool Breakpoint::countMatches(Sit S, Context* c, Bool& wasError)
{
  Bool ret = matches(S, c, wasError);
  if (ret) hitMatched++;
  ret = ret && (wasError || hitMatched > ignoreNum);  
  if (ret) hitBreak++;
  return ret;
}

void Breakpoint::reset()
{
  valid = false;
  if (expr) delete expr;
  expr = NULL;
  hitTotal = hitEnabled = hitBreak = hitMatched =  0;
}

Bool Breakpoint::countEnabled()
{
  hitTotal++;
  if (enabled) hitEnabled++;
  return enabled;
}

////////////////////////////////////////////////////////////
// Breakpoint List
////////////////////////////////////////////////////////////

Breakpoint* BPList::getBP(Str& uri, int line)
{
  for (int i = 0; i < number(); i++)
    {
      if ((*this)[i] -> uri == uri && (*this)[i] -> line == line)
	return (*this)[i];
    }
  return NULL;
}

Breakpoint* BPList::setBP(Str& uri, int line)
{
  Breakpoint *ret = NULL;
  Breakpoint *item = getBP(uri, line);
  if (! item)
    {
      item = new Breakpoint();
      append(item);
      item -> uri = uri;
      item -> line = line;
      ret = item;
    }

  return ret;
}

Bool BPList::clearBP(int idx, Bool running)
{
  if (idx >= 0 && idx < number())
    {
      if (running && (*this)[idx] -> element)
	(*this)[idx] -> element -> breakpoint = NULL;
      freerm(idx, FALSE);
      printf("breakpoint deleted\n");
    }
  else
    printf("invalid breakpoint number\n");
  return true;
}

Bool BPList::disableBP(int idx)
{
  if (idx >= 0 && idx < number())
    {
      (*this)[idx] -> enabled = !(*this)[idx] -> enabled;
      printf("breakpoint #%d is now %s\n", idx,
	     (*this)[idx] -> enabled ? "enabled" : "disabled");
      return true;
    }
  else
    {
      printf("invalid breakpoint number\n");
      return false;
    }
}

void BPList::clearAll(Bool running)
{
  for (int i = number() - 1; i >= 0; i--)
    {
      clearBP(i, running);
    }
}

void BPList::listAll(Bool running)
{
  if ( number() )
    {
      for (int i = 0; i < number(); i++)
	{
	  Breakpoint *item = (*this)[i];
	  printf("#%d: ", i);
	  if (running && ! item -> valid)
	    printf("(invalid) ");
	  else if (!item -> enabled)
	    printf("(disabled) ");
	  printf("%s:%d", (char*)item -> uri, item -> line);
	  if (! item -> condition.isEmpty())
	    printf(" condition:'%s'", (char*)item -> condition);
	  if ( item -> ignoreNum )
	    printf(" ignore:%d", item -> ignoreNum);
	  printf("\n");
	}
    }
  else
    {
      printf("no breakpoints\n");
    }
}

#define _STAT_LEN 10
void BPList::statAll(Bool running)
{
  char buff[32];
  if ( number() )
    {
      for (int i = 0; i < number(); i++)
	{
	  Breakpoint *item = (*this)[i];
	  printf("#%d: ", i);
	  sprintf(buff, "(%d/%d/%d)", item -> hitTotal, 
		  item -> hitEnabled, item -> hitBreak);
	  int len = strlen(buff);
	  printf("%s", buff);
	  if (len < _STAT_LEN)
	    {
	      memset(buff, ' ', _STAT_LEN - len);
	      buff[_STAT_LEN - len] = 0;
	    }
	  else
	    strcpy(buff, " ");
	  printf("%s", buff);
	  printf("%s at %d", (char*)item -> uri, item -> line);
	  printf("\n");
	}
    }
  else
    {
      printf("no breakpoints\n");
    }
}

void BPList::getLinesForURI(Str& uri, List<int>& lines)
{
  int i;
  for (i = 0; i < number(); i++)
    {
      if ((*this)[i] -> uri == uri)
	lines.append((*this)[i] -> line);
    }
  //bubble sort
  Bool cont = true;
  while (cont)
    {
      cont = false;
      for (i = 1; i < lines.number(); i++)
	{
	  if (lines[i - 1] > lines[i]) 
	    {
	      cont = true;
	      lines.swap(i - 1, i);
	    }
	}
    }
}

void BPList::reset()
{
  for (int i = 0; i < number(); i++)
    {
      Breakpoint *bp = (*this)[i];
      bp -> reset();
    }
}

Bool BPList::ignore(long int idx, long int num)
{
  if (idx >= 0 && idx < number())
    {
      (*this)[idx] -> ignoreNum = num;
      return true;
    }
  else
    {
      printf("invalid breakpoint number\n");  
      return false;
    }
}

Bool BPList::setExpression(Sit S, int idx, ArgList& args, Bool running)
{
  if (idx >= 0 && idx < number())
    {
      Breakpoint *bp = (*this)[idx];
      //delete old expression
      if (bp -> expr) delete bp -> expr;
      bp -> expr = NULL;
      bp -> condition.empty();
      if (args.number() == 3)
	{
	  //set new one
	  bp -> condition = *(args[2]);
	  if (running && bp -> valid)
	    {
	      sabassert(bp -> element);
	      if (! bp -> buildExpression(S)) return false;
	    }
	}
    }
  else
    printf("invalid breakpoint number\n");
  return true;
}

////////////////////////////////////////////////////////////
/* debugger functions */
////////////////////////////////////////////////////////////

Debugger::Debugger() 
{
  running = false;
  SablotCreateSituation(&situa);

  msgHandler.makeCode = mhMakeCode;
  msgHandler.log = mhLog;
  msgHandler.error = mhError;

  instep = false;
  intempl = false;
  nextele = NULL;
  output = true;

  sheet = NULL;
}

Debugger::~Debugger()
{
  bpoints.freeall(FALSE);
}

// message handler
MH_ERROR Debugger::mhMakeCode(void *userData, SablotHandle processor_,
			   int severity, unsigned short facility, 
			   unsigned short code)
{
  return code;
}

MH_ERROR Debugger::mhLog(void *userData, SablotHandle processor_,
		      MH_ERROR code, MH_LEVEL level, char **fields)
{
  return code;
}

MH_ERROR Debugger::mhError(void *userData, SablotHandle processor_,
			MH_ERROR code, MH_LEVEL level, 
			char **fields)
{
  char **foo = fields;
  Debugger *this_ = (Debugger*)userData;
  while (*foo)
    {
      char *aux = strchr(*foo, ':');
      Str key, val;
      if (aux) 
	{
	  *aux = 0;
	  key = *foo;
	  val = aux + 1;
	  *aux = ':';
	}
      else
	{
	  val = *foo;
	}
      this_ -> messages.appendConstruct(key, val);
      this_ -> errorCode = code;
      foo++;
    }
  return code;
}

//the salt

void Debugger::setBreakpointsOnElement(Sit S, Element *e)
{
  SubtreeInfo *info = e -> subtree;
  if (info != currentInfo)
    {
      currentInfo = info;
      if (info)
	{
	  currentLines.deppendall();
	  bpoints.getLinesForURI(info -> getBaseURI(), currentLines);
	  //first met? set the next breakpoint line index to be set
	  if (info -> nextBPIndex == -2) 
	    info -> nextBPIndex = currentLines.number() ? 0 : -1;
	}
    }
  if (info && info -> nextBPIndex >= 0)
    {
      int line = currentLines[info -> nextBPIndex];
      if (e -> lineno == line)
	{
	  Breakpoint *bp = bpoints.getBP(info -> getBaseURI(), line);
	  sabassert(bp);
	  e -> breakpoint = bp;
	  bp -> element = e;
	  bp -> valid = true;
	  //build expression (if any)
	  if (! bp -> buildExpression(S) )
	    {
	      reportError("wrong breakpoit expression:", false);
	    }
	  if (info -> nextBPIndex < (currentLines.number() - 1))
	    info -> nextBPIndex ++;
	  else
	    info -> nextBPIndex = -1;
	}
    }
  //process recursively
  for (int i = 0; i < e -> contents.number(); i++)
    if (isElement(e -> contents[i]))
	setBreakpointsOnElement(S, toE(e -> contents[i]));
}

void Debugger::setBreakpoints(Sit S, Tree* sheet_)
{
  //info should be NULL
  sheet = sheet_;
  sabassert(sheet);
  currentInfo = NULL;
  currentLines.deppendall();
  bpoints.reset();
  setBreakpointsOnElement(S, &(sheet -> getRoot()));
}

void Debugger::enterIdle()
{
  Bool quited;
  prompt(*(Situation*)situa, NULL, NULL, quited);
  while (!quited)
    prompt(*(Situation*)situa, NULL, NULL, quited);
}


void Debugger::reportTrace(const char* msg, Str& uri, int line, Element *e)
{
  Str name;
  e -> getOwner().expandQStr(e -> getName(), name);
  printf("\n%s in %s:%d (<%s>)\n",
	 msg, (char*)uri, line, (char*)name);
}

Bool Debugger::elementBreakable(Sit S, Element *e, Context *c, Bool report)
{
  Bool ret = false;
  Bool wasError = false;
  if (e)
    {
      Str &uri = e -> getSubtreeInfo() -> getBaseURI();
      int line = e -> lineno;

      if ((ret = instep)) 
	{
	  if (report) reportTrace("Step to", uri, line, e);
	}
      else if ((ret = (intempl && isXSL(e) && toX(e) -> op == XSL_TEMPLATE)))
	{
	  if (report) reportTrace("Template met", uri, line, e);
	}
      else if ((ret = (e == nextele)))
	{
	  if (report) reportTrace("Step (next) to", uri, line, e);
	}
      else if ((ret = (e -> breakpoint && e -> breakpoint -> countEnabled()
		       && e -> breakpoint -> countMatches(S, c, wasError))))
	{
	  if (wasError)
	    reportError("error evaluating bp condition:", false);
	  if (report) reportTrace("Breakpoint met", uri, line, e);
	}
    }
  //clear helpers
  instep = false;
  if (ret) nextele = NULL;
  if (ret) intempl = false;

  return ret;
}

Bool Debugger::tokenize(char* what_, ArgList &where,
			DbgCommandDescription& desc)
{
  Bool ret;
  desc = theNullCommand;
  char token[256];
  char *what = what_;
  what += strspn(what, " \n\t");
  char *aux = strpbrk(what, " \n\t");
  if (! aux) aux = what + strlen(what);

  strncpy(token, what, aux - what);
  token[aux - what] = 0;

  //perhaps we didnt find anything
  if (!token[0]) return true;

  //now we lookup the command in the table
  Str str(token);
  //return the result of the command lookup
  ret = lookupCommand(str, desc);
  //
  where.append(new Str(token));

  int numargs = 100;
  if (desc.joinRest) numargs = desc.argsMax;

  //read the rest
  int currnum = 1;
  while (*aux && currnum < numargs)
    {
      aux += strspn(aux, " \n\t");
      if (*aux)
	{
	  what = aux;
	  aux = strpbrk(what, " \n\t");
	  if (! aux) aux = what + strlen(what);
	  strncpy(token, what, aux - what);
	  token[aux - what] = 0;
	  where.append(new Str(token));
	}
      currnum++;
    }
  //read the last
  if (*aux)
    {
      aux += strspn(aux, " \n\t");
      if (*aux)
	  where.append(new Str(aux));
    }
  return ret;
}

Bool Debugger::lookupArg(Str &str, char** lst, int &op)
{
  int idx = 0;
  op = -1;
  char *item = lst[idx++];
  while (op == -1 && item)
    {
      char* aux = (char*) str;
      int len = strlen(aux);
      if ( ! strncmp(aux, item, len) )
	{
	  //lookahead
	  char* auxi = lst[idx];
	  if ( auxi )
	    {
	      if ( strncmp(aux, auxi, len) )
		op = idx - 1;
	      else 
		return false;
	    }
	  else
	    op = idx - 1;
	}
      item = lst[idx++];
    }
  return true;
}

Bool Debugger::lookupCommand(Str& str, DbgCommandDescription& out)
{
  int idx = 0;
  out = theNullCommand;
  DbgCommandDescription desc = dbgCommands[idx++];
  while (out.op == SDBG_NOOP && desc.command)
    {
      char* aux = (char*) str;
      int len = strlen(aux);
      if ( ! strncmp(aux, desc.command, len) )
	{
	  //lookahead
	  DbgCommandDescription auxd = dbgCommands[idx];
	  if ( desc.strict && auxd.command )
	    {
	      if ( strncmp(aux, auxd.command, len) )
		out = desc;
	      else 
		return false;
	    }
	  else
	    out = desc;
	}
      desc = dbgCommands[idx++];
    }

  return true;
}

const char* Debugger::nodeTypeName(Vertex *v)
{
  if ( isRoot(v) )
    return "Root";
  else if ( isXSL(v) )
    return "XSLElement";
  else if ( isExt(v) )
    return "ExtensionElement";
  else if ( isElement(v) )
    return "Element";
  else if ( isAttr(v) )
    return "Attribute";
  else if ( isPI(v) )
    return "ProcessingInstruction";
  else if ( isNS(v) )
    return "NmSpace";
  else if ( isComment(v) )
    return "Comment";
  else if ( isText(v) )
    return "Text";
  else 
    return "unknown";
}

void Debugger::listNode(Sit S, NodeHandle node, Bool detailed)
{
  Vertex *v = toV(node);

  //get name
  Str name, ns, cont;
  if (isElement(v) || isAttr(v) || isPI(v))
    {
      v -> getOwner().expandQStr(v -> getName(), name);
    }
  //namespace
  if (isElement(v) || isAttr(v))
    {
      ns = v -> getOwner().expand(v -> getName().getUri());
    }

  switch (baseType(v)) {
  case VT_ATTRIBUTE:
    cont = toA(v) -> cont; break;
  case VT_TEXT:
    cont = toText(v) -> cont; break;
  case VT_PI:
    cont = toPI(v) -> cont; break;
  case VT_COMMENT:
    cont = toComment(v) -> cont; break;
  }

  if (detailed)
    {
      //type
      printf("type: %s\n", nodeTypeName(v));
      //name
      if (! name.isEmpty() )
	{
	  printf("name: %s\n", (char*)name);
	}
      if (! ns.isEmpty() )
	{
	  printf("ns: %s\n", (char*)ns);
	}
      if (! cont.isEmpty() )
	{
	  printf("cont: %s\n", (char*)cont);
	}
    }
  else
    {
      //type
      printf("%s:", nodeTypeName(v));
      //name
      if (! name.isEmpty() )
	  printf(" name='%s'", (const char*) name);
      //namespace
      if (! ns.isEmpty() )
	  printf(" ns='%s'", (const char*) ns);
      //content
      if (! cont.isEmpty() )
	{
	  char foo[11];
	  strncpy(foo, (char*)cont, 10);
	  foo[10] = 0;
	  printf(" cont='%s';", foo);
	}

      //terminal
      printf("\n");
    }
}

void Debugger::listContext(Sit S, Element* e, Context* c, ArgList &args)
{
  if (args.number() == 1)
    {
      printf("Context:\n  size=%d\n  position=%d\n", 
	     c -> getSize(), c -> getPosition());
      //list namespaces
      printf("Namespaces:\n");
      PList<Str*> curr;
      Str prefix, uri;
      for (int i = e -> namespaces.number() - 1; i >= 0; i--)
	{
	  NmSpace *nm = toNS(e-> namespaces[i]);
	  prefix = e -> getOwner().expand(nm -> prefix);
	  uri = e -> getOwner().expand(nm -> uri);

	  Bool found = false;
	  for (int j = 0; j < curr.number() && !found; j++)
	    {
	      found = *(curr[j]) == prefix;
	    }
	if (!found)
	  {
	    curr.append(new Str(prefix));
	    printf("  %s => %s\n", (char*)prefix, (char*)uri);
	  }
	}
      curr.freeall(false);
    }
  else
    {
      int op;
      if (! lookupArg(*(args[1]), ctxArgs, op) )
	printf("ambiguous command argument");
      else
	{
	  if (op >= 0) 
	    {
	      switch (op) {
	      case 0:
		{
		  for (int i = 0; i < c -> getSize(); i++)
		    {
		      if (i == c -> getPosition())
			printf(">");
		      else
			printf("#");
		      printf("%d: ", i);
		      listNode(S, (*c)[i], false);
		    }
		}; break;
	      case 1:
		{
		  printf("#current: ");
		  listNode(S, c -> getCurrentNode(), true);
		}; break;
	      default:
		{
		}; break;
	      }
	    }
	  else
	    {
	      //try integer
	      long int idx;
	      if (strToInt(args[1], idx))
		{
		  if (idx >= 0 && idx < c -> getSize())
		    {
		      listNode(S, (*c)[idx], true);
		    }
		  else
		    {
		      printf("index out of bounds\n");
		    }
		}
	      else
		{
		  printf("unknown command argument\n");
		}
	    }
	}
    }
}

Bool Debugger::findElementForBreakpoint(Element *e, Breakpoint* bp)
{
  SubtreeInfo* info = e -> subtree;
  if (info != currentInfo)
    {
      currentInfo = info;
      inRightTree = info && (info -> getBaseURI() == bp -> uri);
    }
  //set breakpoint if we're in place
  if (inRightTree && e -> lineno == bp -> line)
    {
      e -> breakpoint = bp;
      bp -> element = e;
      bp -> valid = true;
      return true;
    }
  //process babies
  for (int i = 0; i < e -> contents.number(); i++)
    {
      if (isElement(e -> contents[i]))
	if (findElementForBreakpoint(toE(e -> contents[i]), bp))
	  return true;
    }
  return false;
}

Bool Debugger::setBreakpointRunning(Breakpoint *bp)
{
  sabassert(sheet);
  inRightTree = false;
  currentInfo = NULL;

  return findElementForBreakpoint(&(sheet -> getRoot()), bp);
}

void Debugger::doSetBreakpoint(Str& uri, int line)
{
  Breakpoint *bp;
  if ((bp = bpoints.setBP(uri, line)))
    {
      if (!running || setBreakpointRunning(bp))
	printf("breakpoint set in %s at %d\n", (char*)uri, line);
      else
	printf("breakpoint set (invalid) in %s at %d\n", (char*)uri, line);
    }
  else
    printf("breakpoint already exists\n");
}

void Debugger::setBreakpoint(Sit S, Element *e, ArgList& args)
{
  if (args.number() == 1)
    {
      Str &uri = e -> getSubtreeInfo() -> getBaseURI();
      int line = e -> lineno;
      doSetBreakpoint(uri, line);
    }
  else
    {
      char *aux = (char*) *(args[1]);
      char *sep = strchr(aux, ':');
      if (sep)
	{
	  *sep = '\0';
	  
	  char *aline = sep + 1;
	  Str file = aux;
	  char *aux;
	  int line = strtol(aline, &aux, 10);
	  if (*aux != '\0' )
	    {
	      printf("bad location\n");
	    }
	  else
	    {
	      DStr theBase;
	      my_getcwd(theBase);
	      theBase = Str("file://") + theBase;
	      
	      Str abs;
	      makeAbsoluteURI(S, file, theBase, abs);
	      
	      doSetBreakpoint(abs, line);
	    }

	  *sep = ':';
	}
      else
	{
	  printf("bad location\n");
	}
    }
}

Bool Debugger::checkIdle()
{
  if (running)
    printf("processor is running\n");
  return (! running);
}

Bool Debugger::checkRunning()
{
  if (! running)
    printf("processor is NOT running\n");
  return (running);
}

void Debugger::run()
{

  errorCode = 0;
  outputLine = 1;
  outputLastLine = 0;
  messages.freeall(FALSE);

  if (dataURI.isEmpty())
    printf("no datafile given\n");

  if (sheetURI.isEmpty())
    printf("no stylesheet given\n");

  if (!dataURI.isEmpty() && !sheetURI.isEmpty())
    {
      SablotHandle proc;
      SablotCreateProcessorForSituation(situa, &proc);

      //register the message handler
      SablotRegHandler(proc, HLR_MESSAGE, &msgHandler, this);

      for (int i = 0; i < params.number(); i++)
	{
	  SablotAddParam(situa, proc, 
			 (char*)(params[i] -> key),
			 (char*)(params[i] -> value));
	}

      running = true;
      instep = false;
      printf("debugger started\n");
      int code = SablotRunProcessorGen(situa, proc, 
				       (const char*)sheetURI, 
				       (const char*)dataURI, 
				       "arg:/res");
      running = false;
      if (code)
	{
	  if (this -> errorCode != DBG_BREAK_PROCESSOR)
	    {
	      reportError("\ndebugger finished with error: ", true);
	    }
	  else 
	    {
	      printf("debugger finished (killed)\n");
	    }
	  SablotClearSituation(situa);
	}
      else
	{
	  printf("\ndebugger finished successfully\n");
	}
      SablotDestroyProcessor(proc);
    }
}

void Debugger::reportError(const char* msg, Bool details)
{
  printf("%s", msg);
  for (int i = 0; i < messages.number(); i++)
    {
      Str &key = messages[i] -> key;
      if (! (key == (const char*)"msgtype") &&
	  ! (key == (const char*)"module") &&
	  (details ||
	   (! (key == (const char*)"URI") &&
	    ! (key == (const char*)"line") &&
	    ! (key == (const char*)"node")))
	  )
	{
	  printf("[%s: %s]", (char*)key, (char*)(messages[i] -> value));
	}
    }
  printf("\n");
  SablotClearSituation(situa);
}

void Debugger::eval(Sit S, Context *c, Element *e, ArgList& args)
{
  Str estr;
  if (args.number() == 1)
    {
      printf("no expression given\n");
      return;
    }
  else
    estr = *(args[1]);

  Expression expr(*e);
  Bool failed = false;
  failed = expr.parse(S, estr);

  if (! failed )
    {
      Expression ret(*e);
      failed = expr.eval(S, ret, c);
      if (!failed)
	{
	  switch (ret.type) {
	  case EX_NUMBER:
	    {
	      Str str;
	      ret.tostring(S, str);
	      printf("number: %s\n", (char*)str);
	    }; break;
	  case EX_STRING:
	    {
	      Str str;
	      ret.tostring(S, str);
	      printf("string: '%s'\n", (char*)str);
	    }; break;
	  case EX_BOOLEAN:
	    {
	      Str str;
	      ret.tostring(S, str);
	      printf("boolean: %s\n", (char*)str);
	    }; break;
	  case EX_NODESET:
	    {
	      const Context &nset = ret.tonodesetRef();
	      printf("nodeset (size: %d):\n", nset.getSize());
	      for (int i = 0; i < nset.getSize(); i++)
		{
		  printf("#%d: ", i);
		  listNode(S, nset[i], false);
		}
	    }; break;
	  case EX_FRAGMENT:
	    {
	      printf("RTF\n");
	    }; break;
	  case EX_EXTERNAL:
	    {
	      printf("ExternalValue\n");
	    }; break;
	  default:
	    {
	      printf("unknown return type (%d)\n", ret.type);
	    }
	  }
	}
    }

  if (failed) reportError("error in evaluation: ", false);
}

void Debugger::playBatch(Sit S, Element *e, Context *c, Str& file)
{
  Bool done = false;
  char *res, line[256];
  FILE *f;

  f = fopen((char*)file, "r");
  if (f)
    {
      Bool quited = false;
      res = fgets(line, 256, f);
      while (res && !done && !quited)
	{
	  doCommand(S, e, c, line, done, quited);
	  res = fgets(line, 256, f);
	}
    }
  else
    printf("can not read file %s\n", (char*)file);
}

void Debugger::addParam(ArgList& args)
{
  params.appendConstruct(*(args[1]), *(args[2]));
}

Bool Debugger::checkArgs(DbgCommandDescription& desc, ArgList& args)
{
  if (desc.op != SDBG_NOOP)
    {
      int i = args.number() - 1;
      return i >= desc.argsMin && i <= desc.argsMax;
    }
  else
    return true;
}

Bool Debugger::doCommand(Sit S, Element *e, Context *c, 
			 char* line, Bool& done, Bool& quited)
{
  ArgList args;
  quited = false;
  
  DbgCommandDescription cmd;
  if (! tokenize(line, args, cmd))
    {
      printf("ambiguous command\n");
    }
  else
    {
      if (!checkArgs(cmd, args))
	{
	  printf("wrong number of arguments\n");
	  return false;
	}
      //do the command
      switch (cmd.op) {
      case SDBG_NOOP:
	{
	  if (args.number()) printf("unknown command\n");
	}; break;
      case SDBG_BREAK:
	{
	  setBreakpoint(S, e, args);
	}; break;
      case SDBG_BREAK_LIST:
	{
	  bpoints.listAll(running);
	}; break;
      case SDBG_BREAK_CLEAR:
	{
	  long int idx;
	  if (strToInt(args[1], idx))
	      bpoints.clearBP(idx, running);
	  else
	    printf("invalid argument\n");
	}; break;
      case SDBG_BREAK_COND:
	{
	  long int idx;
	  if (strToInt(args[1], idx))
	    {
	      if (!bpoints.setExpression(S, idx, args, running))
		{
		  reportError("wrong breakpoit expression:", false);
		}
	      else
		printf("breakpoint modified\n");
	    }
	  else
	    printf("invalid argument\n");		  
	}; break;
      case SDBG_BREAK_DISABLE:
	{
	  long int idx;
	  if (strToInt(args[1], idx))
	    {
	      if (bpoints.disableBP(idx))
		printf("breakpoint modified\n");
	    }
	  else
	    printf("invalid argument\n");
	}; break;
      case SDBG_BREAK_CLEARALL:
	{
	  bpoints.clearAll(running);
	  printf("all breakpoints deleted\n");
	}; break;
      case SDBG_BREAK_STAT:
	{
	  bpoints.statAll(running);
	}; break;
      case SDBG_BREAK_IGNORE:
	{
	  long int idx, num;
	  if (strToInt(args[1], idx) && strToInt(args[2], num))
	    {
	      if (bpoints.ignore(idx, num))
		printf("breakpoint modified\n");
	    }
	  else
	    printf("invalid argument\n");
	}; break;
      case SDBG_CONTINUE:
	{
	  if (checkRunning())
	    {
	      done = true;
	    }
	}; break;
      case SDBG_CONTEXT:
	{
	  if (checkRunning())
	    {
	      listContext(S, e, c, args);
	    }
	}; break;
      case SDBG_EVAL:
	{
	  if (checkRunning())
	    {
	      sabassert(e);
	      eval(S, c, e, args);
	    }
	}; break;
      case SDBG_BATCH:
	{
	  playBatch(S, e, c, *(args[1]));
	}; break;
      case SDBG_PARAM:
	{
	  if (checkIdle())
	    {
	      addParam(args);
	    }
	}; break;
      case SDBG_PARAM_LIST:
	{
	  if (params.number())
	    for (int i = 0; i < params.number(); i++)
	      {
		printf("$%s = '%s'\n", 
		       (char*)(params[i] -> key), 
		       (char*)(params[i] -> value));
	      }
	  else
	    printf("no params\n");
	}; break;
      case SDBG_PARAM_CLEAR:
	{
	  if (checkIdle())
	    {
	      params.freeall(FALSE);
	      printf("all params removed\n");
	    }
	}; break;
      case SDBG_RUN:
	{
	  if (checkIdle())
	    {
	      intempl = false;
	      run();
	    }
	}; break;
      case SDBG_DATA:
	{
	  if (checkIdle())
	    {
	      if (args.number() < 2)
		{
		  if (!dataURI.isEmpty())
		    printf("using datafile %s\n", (char*)dataURI);
		  else
		    printf("no datafile given\n");
		}
	      else
		{
		  dataURI = *(args[1]);
		  printf("using datafile %s\n", (char*)dataURI);
		}
	    }
	}; break;
      case SDBG_SHEET:
	{
	  if (checkIdle())
	    {
	      if (args.number() < 2)
		{
		  if (!sheetURI.isEmpty())
		    printf("using stylesheet %s\n", (char*)sheetURI);
		  else
		    printf("no stylesheet given\n");
		}
	      else
		{
		  sheetURI = *(args[1]);
		  printf("using stylesheet %s\n", (char*)sheetURI);
		}
	    }
	}; break;
      case SDBG_STEP:
	{
	  if (checkRunning())
	    {
	      instep = true;
	      done = true;
	    }
	}; break;
      case SDBG_NEXT:
	{
	  if (checkRunning())
	    {
	      Vertex *aux = e -> getNextSibling();
	      if ( aux && isElement(aux) ) nextele = toE(aux);
	      done = true;
	    }
	}; break;
      case SDBG_FINISH:
	{
	  if (checkRunning())
	    {
	      Vertex *aux = e -> parent -> getNextSibling();
	      if ( aux && isElement(aux) ) nextele = toE(aux);
	      done = true;
	    }
	}; break;
      case SDBG_TEMPLATE:
	{
	  intempl = true;
	  if (!running) 
	    run();
	  else
	    done = true; //continue
	}; break;
      case SDBG_OUTPUT:
	{
	  output = !output;
	  printf("output is now %s\n", output ? "on" : "off");
	}; break;
      case SDBG_POINT:
	{
	  if (checkRunning())
	    {
	      Str &uri = e -> getSubtreeInfo() -> getBaseURI();
	      int line = e -> lineno;
	      printf("%s:%d\n", (char*)uri, line);
	    }
	}; break;
      case SDBG_KILL:
	{
	  if (checkRunning())
	    {
	      Err(S, DBG_BREAK_PROCESSOR);
	    }
	}; break;
      case SDBG_HELP:
	{
	  printUsage();
	} break;
      case SDBG_QUIT:
	{
	  if (checkIdle())
	    {
	      quited = true;
	      done = true;
	      printf("debugger quited\n");
	    }
	}; break;
      }
    }

  args.freeall(FALSE);
  return OK;
}

void Debugger::printUsage()
{
  printf("%s", theUsageStr);
}

eFlag Debugger::prompt(Sit S, Element *e, Context *c, Bool& quited)
{
  char *line = NULL;
  Bool done = false;
  while (! done)
    {
      if (line) free(line);
      line = dbgGetLine();
      if (line) 
	{
	  E( doCommand(S, e, c, line, done, quited) );
	}
    }
  if (line) free(line);
  return OK;
}

eFlag Debugger::breakOnElement(Sit S, Element *e, Context *c)
{
  Bool quited;
  if ( elementBreakable(S, e, c, true) )
    {
      E( prompt(S, e, c, quited) );
    }
  //quit may not happen while running
  return OK;
}

void Debugger::report(Sit S, MsgType type, MsgCode code, 
		      const Str& arg1, const Str& arg2) const
{
  S.message(type, code, arg1, arg2);
}


void Debugger::printOutput(const char* buff, int size)
{
  char fmt[32];
  if (output)
    {
      if (outputLastLine < outputLine)
	{
	  sprintf(fmt, "%d\t%%.%ds", outputLine, size);
	  outputLastLine = outputLine;
	}
      else
	sprintf(fmt, "%%.%ds", size);
      
      printf(fmt, buff);
    }
  if (*buff == '\n') outputLine++;
}

#endif //SABLOT_DEBUGGER
