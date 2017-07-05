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

#include "numbering.h"
#include "verts.h"
#include "expr.h"
#include "context.h"
#include "domprovider.h"

Bool cmpNames(Sit S, NodeHandle v, NodeHandle w)
{
  int ret = 0;
  const char *aux1, *aux2;
  aux1 = S.dom().getNodeNameLocal(v);
  aux2 = S.dom().getNodeNameLocal(w);
  ret = strcmp(aux1, aux2);
  S.dom().freeName(v, (char*)aux1);
  S.dom().freeName(w, (char*)aux2);
  if (!ret) 
    {
      aux1 = S.dom().getNodeNameURI(v);
      aux2 = S.dom().getNodeNameURI(w);
      ret = strcmp(aux1, aux2);
      S.dom().freeName(v, (char*)aux1);
      S.dom().freeName(w, (char*)aux2);
    }
  return ret;
}

Bool similarVerts(Sit S, NodeHandle v, NodeHandle w)
{
  //some optimalization for native node may be done (compare QNames)
  sabassert(!nhNull(v) && !nhNull(w));
  SXP_NodeType typeV = S.dom().getNodeType(v);
  // vertices of different types don't match
  if (typeV != S.dom().getNodeType(w) )
    return FALSE;
  //test similarity
  switch(typeV)
    {
    case DOCUMENT_NODE:
    case TEXT_NODE:
    case COMMENT_NODE:
      // no expanded name, they match
      return TRUE;
    case ELEMENT_NODE:
      //return toE(v) -> getName() == toE(w) -> getName();
    case NAMESPACE_NODE:
      //return toNS(v) -> getName() == toNS(w) -> getName();
    case ATTRIBUTE_NODE:
      //return toA(v) -> getName() == toA(w) -> getName();
    case PROCESSING_INSTRUCTION_NODE:
      //return toPI(v) -> getName() == toPI(w) -> getName();
      return !cmpNames(S, v, w);
    default:
      // do this to keep compiler happy
      return FALSE;
    }
}

NodeHandle gotoPreceding(Sit S, NodeHandle v, Bool siblingOnly)
{
    sabassert(v);
    switch(S.dom().getNodeType(v))
    {
    case DOCUMENT_NODE:
    case ATTRIBUTE_NODE:
    case NAMESPACE_NODE:
	// no preceding nodes according to XPath spec
	return NULL;
    default:
      {
	NodeHandle par = S.dom().getParent(v);
	
	if (siblingOnly) 
	  {
	    return S.dom().getPreviousSibling(v);
	    //if (v -> ordinal)
	    //  return toD(par) -> contents[v -> ordinal - 1];
	    //else
	    //  return NULL;
	  }
	else //preceding and ancestor-or-self union
	  {
	    //if we have preceding sibling we switch to it and dril
	    NodeHandle w = S.dom().getPreviousSibling(v);
	    if (!nhNull(w)) 
	      {
		for (v = w;
		     !nhNull(v) && S.dom().getChildCount(v); 
		     v = S.dom().getChildNo(v, S.dom().getChildCount(v) - 1));
		return v;
	      }
	    else //go to parent
	      {
		return S.dom().getNodeType(par) == DOCUMENT_NODE ? NULL : par;
	      }
	  }
      }
    }
}

eFlag countMatchingSiblings(Sit S, int& num, NodeHandle v, Expression *count)
{
    num = 0;
    NodeHandle w;
    Bool doesMatch;
    Context c(NULL); //_cn_ we're matching a pattern
    for (w = v; !nhNull(w); w = gotoPreceding(S, w, /* siblingOnly = */ TRUE))
    {
	if (count)
	{
	    c.deppendall();
	    c.set(w);
	    E( count -> matchesPattern(S, &c, doesMatch) );
	}
	else
	    doesMatch = similarVerts(S, v, w);
	if (doesMatch)
	    num++;
    }
    return OK;
}

eFlag xslNumberCount(
    Sit S, NumberingLevel level, 
    Expression* count, Expression* from, 
    NodeHandle curr, ListInt& result)
{
    result.deppendall();
    int num;
    NodeHandle w = NULL;
    List<NodeHandle> matchingList;
    Bool doesMatch;
    Context c(NULL); //_cn_ we're matching a pattern
    // construct the list of matching ancestors/preceding nodes
    for (w = curr; !nhNull(w); )
    {
	c.deppendall();
	c.set(w);
	if  (from)
	{
	    E( from -> matchesPattern(S, &c, doesMatch) );
	    if (doesMatch) break; // leave the for loop
	}
	if (count)
	    E( count -> matchesPattern(S, &c, doesMatch) )
	else
	    doesMatch = similarVerts(S, curr, w);
	if (doesMatch)
	{
	    matchingList.append(w);
	    if (level == NUM_SINGLE) break; // leave the for loop after finding a match
	}
	if (level == NUM_ANY)
	    w = gotoPreceding(S, w, /* siblingOnly = */ FALSE);
	else
	    w = S.dom().getParent(w);
    }
    // construct the integer list out of matchingList
    if (level == NUM_ANY)
	result.append(matchingList.number());
    else
	for (int i = matchingList.number() - 1; i >= 0; i--)
	{
	    E( countMatchingSiblings(S, num, matchingList[i], count) );
	    result.append(num);
	}
    return OK;
}

Bool isAlnumFToken(const Str& s)
{
    unsigned long c = utf8CharCode((const char*)s);
    return utf8IsDigit(c) || utf8IsLetter(c);
}

Bool getFToken(const char *&p, Str& fmt)
{
    if (!*p)
	return FALSE;
    const char* pOrig = p;
    Bool alnum = isAlnumFToken(p);
    do
	p += utf8SingleCharLength(p);
    while(*p && isAlnumFToken(p) == alnum);
    fmt.nset(pOrig, (int)(p - pOrig));
    return TRUE;
}

void getFTokenParams(const Str& fmt, char& type, int& width)
{
    // defaults:
    type = '1';
    width = 1;
    int len = utf8StrLength(fmt);
    sabassert(len);
    if (len > 1 && fmt[0] != '0') return; // with default values
    switch(fmt[0])
    {
    case 'A':
    case 'a':
    case 'I':
    case 'i':
    {
	type = fmt[0];
	return;
    }
    case '0':
	break;
    default:
	return;
    }
    // it remains to take care of the '0':
    for (int i = 1; i < len - 1; i++)
	if (fmt[i] != '0') return;
    if (fmt[len - 1] != '1') return;
    width = len;
}

void appendABC(int num, Bool uppercase, DStr& result)
{
    DStr reversed;
    do
    {
	num--;
	reversed += (char)((uppercase ? 'A' : 'a') + num % 26);
	num /= 26;
    }
    while (num > 0);
    for (int i = reversed.length() - 1; i >= 0; i--)
	result += reversed[i];
}

struct RomanDef
{
    int value;
    char symbol[3];
};

RomanDef romans[] =
{
    { 1000, "mM"},
    { 500, "dD"},
    { 100, "cC"},
    { 50, "lL"},
    { 10, "xX"},
    { 5, "vV"},
    { 1, "iI"},
    { 0, "oO"}
};

void appendRoman(int num, Bool uppercase, DStr& result)
{
    int step = 0,
	prefix,
	val;
    if (uppercase != 0)
	uppercase = 1;
    while (num > 0)
    {
	if (num >= (val = romans[step].value))
	{
	    result += romans[step].symbol[uppercase];
	    num -= val;
	}
	else 
	{
	    prefix = step + 2 - step % 2;
	    if (val > 1 && num >= (val - romans[prefix].value))
	    {
		result += romans[prefix].symbol[uppercase];
		result += romans[step].symbol[uppercase];
		num -= (val - romans[prefix].value);
	    }
	    else
		step++;
	}
    }
}

void appendArabic(
    int num, int width, 
    const Str& groupingSep, int groupingSize, DStr& tempResult)
{
    // just put separators and leading zeroes in the number
    DStr sprFmt = DStr("%0") + width + "d";
    char buff[32],
	*p = buff;
    int len = snprintf(buff, 32, (char*)sprFmt, num);
    if (!groupingSize)
	tempResult += buff;
    else
    {
	int first = len % groupingSize;
	if (first)
	{
	    tempResult.nadd(p, first);
	    p += first;
	    len -= first;
	    if (len)
		tempResult += groupingSep;
	}
	for (; len > 0; len -= groupingSize, p += groupingSize)
	{
	    tempResult.nadd(p, groupingSize);
	    if (len > groupingSize)
		tempResult += groupingSep;
	}
    }
}

void formatSingleNumber(
    Sit S, int num, const Str& fmt, 
    const Str& lang, NumberingLetterValue letterValue, 
    const Str& groupingSep, int groupingSize, DStr& tempResult)
{
    // we only add to tempResult, do not initialize it
    char type;
    int width;
    // check value of num
    if (num <= 0)
    {
	S.message(MT_WARN, W_NUMBER_NOT_POSITIVE, (char*)NULL, (char*)NULL);
	num = num ? abs(num) : 1;
    }
    getFTokenParams(fmt, type, width);
    switch(type)
    {
    case 'A':
    case 'a':
	appendABC(num, /* uppercase = */ type == 'A', tempResult);
	break;
    case 'I':
    case 'i':
	appendRoman(num, /* uppercase = */ type == 'I', tempResult);
	break;
    default:
	appendArabic(num, width, groupingSep, groupingSize, tempResult);
    }
}

eFlag xslNumberFormat(
    Sit S, ListInt& nums, const Str& format, 
    const Str& lang, NumberingLetterValue letterValue, 
    const Str& groupingSep, int groupingSize, Str& result)
{
    DStr tempResult;
    Str sep = ".", 
	sepRightmost, 
	alpha = "1",
	firstToken;

    const char *p = (const char*) format;
    int ndx = 0;

    if (getFToken(p, firstToken))
    {
	if (isAlnumFToken(firstToken) && nums.number())
	{
	    alpha = firstToken;
	    formatSingleNumber(
		S, nums[0], alpha, lang, letterValue,
		groupingSep, groupingSize, tempResult);
	    ndx = 1;
	}
	else
	{
	    // reset p to the beginning
	    p = (const char*) format;
	    if (!nums.number())
		tempResult += sepRightmost = firstToken;;
	}
    }
    // p points at the first separator (if any)
    Bool readAllFormat = *p ? FALSE : TRUE;
    for (; ndx < nums.number(); ndx++)
    {
	if (!readAllFormat)
	{
	    // always update the rightmost separator
	    if (getFToken(p, sepRightmost))
	    {
		if (getFToken(p, alpha))
		{
		    // alpha token found
		    // use the current separator and reset the rightmost one
		    sep = sepRightmost;
		    sepRightmost.empty();
		}
		else
		{
		    // no alpha token, use the same separator as last time
		    // keep current separator in sepRightmost
		    readAllFormat = TRUE;
		    if (!ndx)
			sep = sepRightmost;
		}
		
	    }
	    else
		// both empty, use the same separator and alpha as last time
		readAllFormat = TRUE;
	} // if (!readAllFormat)
	// sep and alpha are the last valid tokens of each kind
	tempResult += sep;
	formatSingleNumber(
	    S, nums[ndx], alpha, lang, letterValue,
	    groupingSep, groupingSize, tempResult);
    } // for loop
    // get the real rightmost separator
    if (!readAllFormat)
    {
	while(getFToken(p, sepRightmost));
	if (isAlnumFToken(sepRightmost))
	    sepRightmost.empty();
    }
    tempResult += sepRightmost;
    result = tempResult;
    return OK;
}
