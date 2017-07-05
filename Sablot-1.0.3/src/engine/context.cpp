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

/*****************************************************************

    c   o   n   t   e   x   t   .   c   p   p

*****************************************************************/

#include "context.h"
#include "tree.h"
#include "utf8.h"
#include <locale.h>
#include "key.h"
#include "domprovider.h"
#include "guard.h"  // GP: clean

/*****************************************************************
    language name aliases
    primarily for different language names in Windows setlocale
    (unix setlocale respects ISO 639 which is also the standard
    for language codes in XML docs)
    SO FAR FOR TESTING PURPOSES

    each entry contains 2 strings with space-separated codes
    - first with the ISO codes (may appear in XML)
    - second with the setlocale-specific codes (needn't be ISO outside unix)
*****************************************************************/

struct LangAlias
{
    const char 
        *namesISO, *namesLocale;
};

LangAlias langAliases[] =
{
    {"en",                      "en_US"},
    {"de",                      "de_DE"},
    {"fr",                      "fr_FR"},
    {"it",                      "it_IT"},
    {"cze ces cs cs_CZ",        "cs_CZ czech"},
    {"sk slk slo sk_SK",        "sk_SK slovak"},
    {NULL, NULL}
};

/*****************************************************************
    C L i s t   methods
*****************************************************************/

CList::CList()
:
SList<NodeHandle>(LIST_SIZE_LARGE)
{
    sortDefs = NULL;
    refCount = 1;
    currLevel = 0;
    wcsValues = FALSE;
}

// GP: added method to kill values

CList::~CList()
{
    values.freeall(TRUE);
}

void CList::incRefCount()
{
    refCount++;
}

int CList::decRefCount()
{
    return --refCount;
}

//  compareWithoutDocOrd
//  assumes that the 'values' entries have been created
//  compares 2 vertices based on just the values, not their relative document order

Bool CList::tagChanged(int i, int j)
{
  return tags[i] != tags[j];
}

int CList::compareWithoutDocOrd(int i, int j)
{
    sabassert(sortDefs && currLevel < sortDefs -> number());
    sabassert(i < values.number() && j < values.number());

    SortDef *def = (*sortDefs)[currLevel];
    // *** use case-order and lang from def!!!
    int result;
    if (def -> asText)
      {
	if (wcsValues) 
	  {
	    result = wcscmp__((char*)values[i], (char*)values[j]);
	  }
	else
	  {
	    result = strcmp((char*)values[i], (char*)values[j]);
	  }
      }
    else
      {
        Number n1, n2;
        n1 = (char*)values[i];
        n2 = (char*)values[j];
        if (n1 < n2)
            result = -1;
        else 
            result = (n2 < n1) ? 1 : 0;
      }
    return def -> ascend ? result : -result;
}


//  compare assumes that the 'values' entries have been created
int CList::compare(int i, int j, void *data)
{
    if (sortDefs)
    {
        int result = compareWithoutDocOrd(i, j);
        if (result)
            return result;
    }
    // sort in document order
    NodeHandle v1 = block[i], v2 = block[j];
    sabassert(v1 && v2);
    return ((DOMProvider*)data) -> compareNodes(v1, v2);
}

Bool hasWord(const char *sentence, const char *w)
{
    const char *p;
    Str currWord;
    int len;
    for (p = sentence; *p; p += len + strspn(p, " "))
    {
        currWord.nset(p, len = strcspn(p, " "));
        if (currWord.eqNoCase(w)) return TRUE;
    }
    return FALSE;
}

char* setLang(const Str& src)
{
    char *result;
    const char *p;
    Str dest;
    int len;
    if (NULL != (result = setlocale(LC_COLLATE, src))) 
        return result;
    
    LangAlias *a = langAliases;
    while(a -> namesISO && !hasWord(a -> namesISO, src)) a++;
    if (a -> namesISO)
    {
        for (p = a -> namesLocale; *p; p += len + strspn(p, " "))
        {
            dest.nset(p, len = strcspn(p, " "));
            if (NULL != (result = setlocale(LC_COLLATE, dest))) 
                return result;
        }
    }
    return FALSE;
}

/*!!! _ph_ remove */
void __dump(CList& lst, int idx1, int idx2)
{
  printf("\n--- DUMP - BEGIN (%d, %d) ---\n", idx1, idx2);
  for (int i = 0; i < lst.number(); i++)
    {
      if (isElement(lst[i]))
	{
	  Element* e = toE(lst[i]);
	  for (int j = 0; j < e -> atts.number(); j++)
	    {
	      printf("%2s", (char*)((Attribute*)e -> atts[j]) -> cont);
	    }
	  printf("\n");
	}
    }
  printf("--- DUMP - END ---\n\n");
}

eFlag CList::sort(Sit S, XSLElement *caller, Context *ctxt, SortDefList* sortDefs_ /* = NULL */)
{
  // GP: on error, locale is reset
  // checked that all callers destroy the context on error
  int currTag = 0;
  sabassert(caller || !sortDefs_);
  Str theLang;
  sortDefs = sortDefs_;
  if (sortDefs)
    {
      if (!setLang((*sortDefs)[0] -> lang))
        {
	  Warn1(S, W1_LANG, (*sortDefs)[0] -> lang);
	  setlocale(LC_COLLATE, "C");
        }
      // this is to reset the locale if there's an error
      E_DO( makeValues(S, 0, number() - 1, 0, caller, ctxt), 
	    setlocale(LC_COLLATE, "C") );
    }
  currLevel = 0;
  SList<NodeHandle>::sort(0, number() - 1, &(S.dom()));
  for (int i = 1; sortDefs && (i < sortDefs -> number()); i++)
    {
      if (!setLang((*sortDefs)[i] -> lang))
        {
	  Warn1(S, W1_LANG, (*sortDefs)[i] -> lang);
	  setlocale(LC_COLLATE, "C");
        }
      int j0 = 0;
      currLevel = i - 1;
      for (int j = 1; j <= number(); j++)
	if (j == number() || compareWithoutDocOrd(j0,j) || 
	    tagChanged(j0, j))
	  {
	    if (j > j0 + 1)
	      {
		currLevel = i;
		E_DO( makeValues(S, j0, j-1, i, caller, ctxt), 
		      setlocale(LC_COLLATE, "C"));
		// printf("Sorting %d:%d to %d\n",i,j0,j-1);
		SList<NodeHandle>::sort(j0, j-1, &(S.dom()));
		//set tags for the next level
		currTag++;
		for (int aux = j0; aux < j; aux++) tags[aux] = currTag;
		// reset currLevel for next compareWithoutDocOrd()
		currLevel = i - 1;
	      };
	    j0 = j;
	  }
    }
  if (sortDefs)
    {
      // *** unset the temp locale as necessary
      setlocale(LC_ALL, "C");
      values.freeall(TRUE);
      tags.deppendall();
    }
  // reset the current element of parent context
  ctxt -> setPosition(0);
  return OK;
}

void CList::swap(int i, int j)
{
    SList<NodeHandle>::swap(i, j);
    if (sortDefs)
      {
        values.swap(i, j);
	tags.swap(i, j);
      }
}

eFlag CList::makeValues(Sit S, int from, int to, int level, 
                        XSLElement *caller, Context *ctxt)
{
    sabassert(ctxt);
    wcsValues = FALSE;
    if (!sortDefs) return OK;
    
    // we may assume that the node handles are actually pointers now
    sabassert(level < sortDefs -> number());
    SortDef *def = (*sortDefs)[level];
    DStr d;
    //GP( Str ) strg;
    //char *strg;
    GP(GChar) strg;
    Expression *e, result(*caller, EXF_ATOM);
    for (int i = from; i <= to; i++)
    {
        ctxt -> setPosition(i);
        //strg = new Str;
        e = def -> sortExpr;
        if (!e)
        {
            E( toPhysical((*this)[i]) -> value(S, d, ctxt) );
            if (def -> asText)
	      {
#               if defined(SAB_WCS_COLLATE)
	        strg.assign( utf8xfrm(d) );
   	        wcsValues = TRUE;
#               else
                strg.assign( d.cloneData() );
#               endif
	      }
            else
                strg.assign(d.cloneData() );
        }
        else
        {
            E( e -> eval(S, result, ctxt) );

            if (def -> asText)
	        {
#               if defined(SAB_WCS_COLLATE)
                Str temp;
		E( result.tostring(S, temp) );
                strg.assign( utf8xfrm(temp) );
                wcsValues = TRUE;
#               else
		DStr aux;
                E(result.tostring(S,aux));
		strg.assign( aux.cloneData() );
#               endif
            }
            else {
	      DStr aux;
	      E(result.tostring(S, aux));
	      strg.assign( aux.cloneData() );
	    }
        }
        if (!level)
	  {
            values.append(strg.keep());
	    tags.append(0);
	  }
        else
	  {
            cdeleteArr(values[i]);
            values[i] = strg.keep();
	  }
    }
    return OK;
}

void CList::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type, code, arg1, arg2);
}


/*****************************************************************

    C o n t e x t    methods

******************************************************************/

Context::Context(NodeHandle current, int isForKey_ /* = FALSE */)
{
    isForKey = isForKey_;
    array = isForKey? new KList : new CList;
    currentNode = current;
    position = -1;
    virtualPosition = 0;
    virtualSize = -1;
}

Context::~Context()
{
    sabassert(array);
    if (!array -> decRefCount())
        delete array;
}

int Context::getPosition() const
{
    return position + virtualPosition;
}

int Context::getSize() const
{
    if (virtualSize != -1)
        return virtualSize;
    else
        return array -> number();
}

NodeHandle Context::current() const
{
    if (!isFinished())
        return (*array)[position];
    else
        return NULL;
};

NodeHandle Context::getCurrentNode() const
{
  return currentNode;
  /*
    if (currentNode)
        return currentNode;
    else
        return current();
  */
}

void Context::setCurrentNode(NodeHandle v)
{
    currentNode = v;
}

void Context::reset()
{
    if (!array -> number())
        position = -1;
    else
        position = 0;
}

Bool Context::isFinished() const
{
    return (Bool) !((position >= 0) && (position < array -> number()));
}

Bool Context::isVoid() const
{
    return (Bool) !(array -> number());
}

NodeHandle Context::shift()
{
    if ((position >= 0) && (position < array -> number() - 1))
        return (*this)[++position];
    position = -1;
    return NULL;
}

void Context::set(NodeHandle v)
{
    array -> append(v);
    reset();
}

void Context::setVirtual(NodeHandle v, int virtualPosition_, int virtualSize_)
{
    sabassert(!array -> number() && "setVirtual() on nonvoid context");
    array -> append(v);
    virtualSize = virtualSize_;
    virtualPosition = virtualPosition_;
    reset();
}

NodeHandle Context::operator[] (int n) const
{
    return (*array)[n];
}

void Context::deppendall()
{
    if (!array -> decRefCount())
        delete array;
    array = isForKey? new KList : new CList;
    position = -1;
}

void Context::append(NodeHandle v)
{
    array -> append(v);
    reset();
}

void Context::deppend()
{
    array -> deppend();
    if (position >= array -> number())
        position = array -> number() - 1;
}

Context* Context::copy()
{
    Context *newc = new Context(currentNode);
    delete NZ(newc -> array);
    newc -> array = array;
    newc -> virtualPosition = virtualPosition;
    newc -> virtualSize = virtualSize;
    array -> incRefCount();
    newc -> reset();
    return newc;
}

void Context::swap(int i, int j)
{
    array -> swap(i, j);
}

Context* Context::swallow(Sit S, Context *other)
{
    Context *result = new Context(currentNode); // GP: OK since no E()
    int i = 0, j = 0,
        iLimit = array -> number(), jLimit = other -> array -> number();
    NodeHandle v, w;
    while ((i < iLimit) && (j < jLimit))
    {
        v = (*array)[i];
	w = (*other)[j];
        switch(S.dom().compareNodes(v, w))
        {
        case 0: 
            j++; break;
	case -1:
	{
	    result -> append(v);
	    i++;
	}; break;
	case 1:
	{
	    result -> append(w);
	    j++;
	}; break;
        }
    }
	
    while (i < iLimit)
        result -> append((*array)[i++]);
    while (j < jLimit)
        result -> append((*(other -> array))[j++]);
    deppendall();
    other -> deppendall();
    return result;
}

Bool Context::contains(NodeHandle v) const
{
    int i;
    for (i = 0; i < array -> number(); i++)
    {
        if (v == (*array)[i])
            return TRUE;
    }
    return FALSE;
}

void Context::uniquize()
{
    int i;
    for (i = array -> number() - 2; i >= 0; i--)
        if ((*array)[i] == (*array)[i+1])
            array -> rm(i);
}

CList* Context::getArrayForDOM()
{
    return array;
}

KList* Context::getKeyArray()
{
    sabassert(isForKey);
    return cast(KList*, array);
}
