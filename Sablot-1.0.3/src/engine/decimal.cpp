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

// decimal.cpp

#include "decimal.h"
#include "expr.h"
#include "math.h"
#include "guard.h"

DecimalFormat::DecimalFormat(const EQName& name_)
    : name(name_),
      decimalSeparator(".", TRUE, XSLA_DECIMAL_SEPARATOR),
      groupingSeparator(",", TRUE, XSLA_GROUPING_SEPARATOR),
      infinity("Infinity", FALSE, XSLA_INFINITY),
      minusSign("-", TRUE, XSLA_MINUS_SIGN),
      NaN("NaN", FALSE, XSLA_NAN),
      percent("%", TRUE, XSLA_PERCENT),
      perMille("\xe2\x80\x83", TRUE, XSLA_PER_MILLE), // U+2030
      zeroDigit("0", TRUE, XSLA_ZERO_DIGIT),
      digit("#", TRUE, XSLA_DIGIT),
      patternSeparator(";", TRUE, XSLA_PATTERN_SEPARATOR)
{
}

DecimalFormat::~DecimalFormat()
{
}

eFlag DecimalFormat::setItem(Sit S, XSL_ATT itemId, const Str& value)
{
    return NZ(findItem(itemId)) -> set(S, value);
}

const Str& DecimalFormat::getItem(XSL_ATT itemId)
{
    return NZ(findItem(itemId)) -> get();
}

DefaultedStr* DecimalFormat::findItem(XSL_ATT itemId)
{
    switch(itemId)
    {
    case XSLA_DECIMAL_SEPARATOR:
	return &decimalSeparator;
    case XSLA_GROUPING_SEPARATOR:
	return &groupingSeparator;
    case XSLA_INFINITY:
	return &infinity;
    case XSLA_MINUS_SIGN:
	return &minusSign;
    case XSLA_NAN:
	return &NaN;
    case XSLA_PERCENT:
	return &percent;
    case XSLA_PER_MILLE:
	return &perMille;
    case XSLA_ZERO_DIGIT:
	return &zeroDigit;
    case XSLA_DIGIT:
	return &digit;
    case XSLA_PATTERN_SEPARATOR:
	return &patternSeparator;
    default:
	return NULL;
    }
}

#define max(x,y) (x > y ? x : y)

eFlag DecimalFormat::format(Sit S, Number& num, const Str& fmt, Str& result)
{
    if (num.isNaN())
    {
	result = getItem(XSLA_NAN);
	return OK;
    }
    int factor = 1,
	iDigitsMin = 0, 
	fDigits = 0, 
	fDigitsMin = 0,
	gSize = 0;
    Str prefix,
	suffix;
    E( parse(S, fmt, num < 0.0, prefix, suffix, factor, iDigitsMin, fDigits, fDigitsMin, gSize) );
    
    //    printf("pfx %s sfx %s factor %d imin %d fmax %d fmin %d gsize %d",
    //      (char*)prefix, (char*)suffix, factor, iDigitsMin, fDigits, fDigitsMin, gSize);
    
    // start with the prefix (DO NOT we need to add it as the very last step)
    //DStr tempResult = prefix;
    DStr tempResult;
    if (num.isInf())
    {
      tempResult = prefix;
      tempResult += getItem(XSLA_INFINITY);
      tempResult += suffix;
      result = tempResult;
      return OK;
    }
    // we have a legitimate number
    // convert number to string form
    
    double theNumber = fabs((double) num * factor);
    
    //char buff[64];
    //deallocates automatically
    GPA( GChar ) buff = new char[1024];

    DStr auxFmt = "%";
    if (iDigitsMin)
	auxFmt += "0";
    //auxFmt += iDigitsMin + fDigits + 1 /* + (num < 0.0 ? 1 : 0) */;
    auxFmt += iDigitsMin + (fDigits ? fDigits + 1 : 0);
    auxFmt += ".";
    auxFmt += fDigits;
    auxFmt += "f";
    sprintf((char*)buff, (char*) auxFmt, theNumber);

    // buff now holds the number without sign, grouping marks and with possibly too many trailing zeroes
    // scan the number for decimal point position and trailing zeroes count
    //if #.00 was specified, we still have a leading zero (0.12 instead of .12)
    char *buff1 = (char*)buff;
    if ( ! iDigitsMin ) //remove leading zeros
      while ( *buff1 == '0' ) buff1++;

    int decPt = -1,
	trailingCount = 0,
	j;
    for (j = 0; buff1[j]; j++)
    {
	if (buff1[j] == '.' && decPt < 0)
	    decPt = j;
	else if (decPt >= 0)
	{
	    if (buff1[j] != '0')
		trailingCount = 0;
	    else
		trailingCount++;
	}
    }
    if (decPt == -1)
	decPt = j;
    
    // insert separators into integer part
    int block = gSize ? decPt % gSize : decPt;
    if (!block) block = gSize;
    for (int i = 0; i < decPt; i += block, block = gSize)
    {
	for (int k = 0; k < block; k++)
	{
	    if (buff1[i + k] != 0)
		tempResult.nadd(buff1 + i + k, 1);
	    else
		tempResult += getItem(XSLA_ZERO_DIGIT);
	}
	if (i + block < decPt)
	    tempResult += getItem(XSLA_GROUPING_SEPARATOR);	
    }
    // add a decimal point if needed
    if (fDigitsMin || !(trailingCount == fDigits))	
	tempResult += getItem(XSLA_DECIMAL_SEPARATOR);

    // copy decimal part, omitting the extra trailing zeroes
    tempResult.nadd(buff1 + decPt + 1, max(fDigits - trailingCount, fDigitsMin));

    //replace the zero character (tell me why for god's sakes)
    char zd = *((char*)zeroDigit.get());
    if (zd != '0')
      {
	char *aux = (char*)tempResult;
	while ((aux = strchr(aux, '0')))
	  *aux = zd;
      }

    // add percent or per-mille sign
    if (factor == 100)
	tempResult += getItem(XSLA_PERCENT);
    if (factor == 1000)
	tempResult += getItem(XSLA_PER_MILLE );
    
    // add the prefix and suffix
    DStr final = prefix + tempResult + suffix;
    result = final;
    return OK;
}

const EQName& DecimalFormat::getname()
{
    return name;
}

#define readToken(STRG, PTR, LENGTH) STRG.nset(PTR, LENGTH)

XSL_ATT tokensList[] = 
{
    XSLA_DIGIT, 
    XSLA_ZERO_DIGIT,
    XSLA_GROUPING_SEPARATOR,
    XSLA_DECIMAL_SEPARATOR,
    XSLA_PATTERN_SEPARATOR,
    XSLA_PERCENT,
    XSLA_PER_MILLE,
    XSLA_NONE
};

XSL_ATT DecimalFormat::whichToken(const char* ptr, int len)
{
    for (int i = 0; tokensList[i] != XSLA_NONE; i++)
	if (!strncmp(ptr, getItem(tokensList[i]), len))
	    return tokensList[i];
    return XSLA_NONE;
}

eFlag DecimalFormat::parseSubpattern(Sit S, const char *&ptr, Bool negative, 
				     Str& prefix, Str& suffix, int& factor,
				     int& iDigitsMin, int& fDigits, int& fDigitsMin, int& gSize)
{
    int len;
    int state = 0;
    Bool wasDigit = FALSE;
    XSL_ATT token;
    prefix.empty();
    suffix.empty();
    iDigitsMin = fDigits = fDigitsMin = 0;
    gSize = -1;
    factor = 1;
    for (; *ptr && state < 5; ptr += len)
    {
	if (*ptr == '\'')
	{
	    // escaped special character; if non-special follows, throw error
	    ptr++;
	    if (!*ptr) break;
	    len = utf8SingleCharLength(ptr);
	    if (whichToken(ptr, len) == XSLA_NONE)
		Err(S, E_FORMAT_INVALID);
	    token = XSLA_NONE;	    
	}
	else
	{
	    len = utf8SingleCharLength(ptr);	    
	    token = whichToken(ptr, len);
	}
	
	// check for the forbidden currency sign U+00a4
	if ((unsigned char)*ptr == 0xc2 && (unsigned char)(ptr[1]) == 0xa4)
	    Err(S, E_FORMAT_INVALID);
	
	switch(state)
	{
	case 0:
	    // the very start
	{
	    if (token == XSLA_NONE)
	    {
	      //readToken(prefix, ptr, len);
	      Str foo;
	      readToken(foo, ptr, len);
	      prefix = prefix + foo;
		break;
	    }
	    state = 1;
	}; 
	case 1:
	    // after the prefix has been retrieved
	{
	    switch(token)
	    {
	    case XSLA_DIGIT:
	    case XSLA_ZERO_DIGIT:
	    {
		if (gSize >= 0) gSize++;
		wasDigit = TRUE;
		if (token == XSLA_DIGIT)
		{
		    if (iDigitsMin)
			Err(S, E_FORMAT_INVALID);
		}
		else
		    iDigitsMin++;
	    }; break;
	    case XSLA_GROUPING_SEPARATOR:
	    {
		// forbid consecutive commas and comma at start
		if (!gSize || !wasDigit)
		    Err(S, E_FORMAT_INVALID);
		gSize = 0;
	    }; break;
	    case XSLA_DECIMAL_SEPARATOR:
		state = 2; break;
	    case XSLA_PERCENT:
	    case XSLA_PER_MILLE:
	    {
		if (factor != 1)
		    Err(S, E_FORMAT_INVALID);
		factor = (token == XSLA_PERCENT) ? 100 : 1000;
		//state = 3;
	    }; break;
	    case XSLA_NONE:
	    {
		readToken(suffix, ptr, len);
		state = 4;
	    }; break;
	    case XSLA_PATTERN_SEPARATOR:
	    {
		if (negative)
		    Err(S, E_FORMAT_INVALID);
		state = 5; 
	    }; break;
	    default:
		Err(S, E_FORMAT_INVALID);
	    }
	}; break;
	case 2:
	    // after the decimal point
	{
	    switch(token)
	    {
	    case XSLA_ZERO_DIGIT:
	    {
		if (fDigits > fDigitsMin)
		    Err(S, E_FORMAT_INVALID);
		fDigitsMin++; 
		fDigits++;
	    }; break;
	    case XSLA_DIGIT:
		fDigits++; break;
	    case XSLA_PERCENT:
	    case XSLA_PER_MILLE:
	    {
		if (factor != 1)
		    Err(S, E_FORMAT_INVALID);
		factor = (token == XSLA_PERCENT) ? 100 : 1000;
		//state = 3;
	    }; break;
	    case XSLA_NONE:
	    {
		readToken(suffix, ptr, len);
		state = 4;
	    }; break;
	    case XSLA_PATTERN_SEPARATOR:
	    {
		if (negative)
		    Err(S, E_FORMAT_INVALID);
		state = 5; 
	    }; break;
	    default:
		Err(S, E_FORMAT_INVALID);
	    }
	}; break;
	case 3:
	    // after the percent/per-mille sign was retrieved
	{
	    switch(token)
	    {
	    case XSLA_NONE:
	    {
		readToken(suffix, ptr, len);
		state = 4;
	    }; break;
	    case XSLA_PATTERN_SEPARATOR:
	    {
		if (negative)
		    Err(S, E_FORMAT_INVALID);
		state = 5; 
	    }; break;
	    default:
		Err(S, E_FORMAT_INVALID);
	    }
	}; break;
	case 4:
	    // after the suffix was retrieved
	{
	  if (token == XSLA_NONE) {
	    Str foo;
	    readToken(foo, ptr, len);
	    suffix = suffix + foo;
	    break;
	  }
	  if (token != XSLA_PATTERN_SEPARATOR || negative)
	    Err(S, E_FORMAT_INVALID);
	  state = 5;
	}; break;
	}
    }
    if ((negative && *ptr)) //|| !iDigitsMin)
	Err(S, E_FORMAT_INVALID);
    // forbid trailing ;
    if (!negative && !*ptr && state == 5)
	Err(S, E_FORMAT_INVALID);
    if (gSize == -1)
	gSize = 0;
    return OK;
}

eFlag DecimalFormat::parse(Sit S, const Str &src, Bool negative,
			   Str& prefix, Str& suffix, int& factor,
			   int& iDigitsMin, int& fDigits, int& fDigitsMin, int& gSize)
{
    const char *p = (const char*) src;
    E( parseSubpattern(S, p, FALSE, prefix, suffix, factor, 
		       iDigitsMin, fDigits, fDigitsMin, gSize) );
    if (negative)
    {
	if (*p)
	{
	    int iDigitsMin_, fDigits_, fDigitsMin_, gSize_;
	    // determine prefix, suffix and factor, rest given by positive subpattern
	    E( parseSubpattern(S, p, TRUE, prefix, suffix, factor, 
			       iDigitsMin_, fDigits_, fDigitsMin_, gSize_) );
	}
	else
	{
	    // defaults
	    prefix = DStr(getItem(XSLA_MINUS_SIGN)) + prefix;
	    // suffix and factor remain
	}
    }
    return OK;
}

void DecimalFormat::report(Sit S, MsgType type, MsgCode code, 
    const Str &arg1, const Str &arg2) const
{
    S.message(type, code, arg1, arg2);
}

//
//
//  DefaultedStr
//
// 

DefaultedStr::DefaultedStr(const char *defaultValue_, Bool singleChar_, XSL_ATT id_)
    : defaultValue(defaultValue_), specified(FALSE), singleChar(singleChar_), id(id_)
{
}

DefaultedStr::~DefaultedStr()
{
}

eFlag DefaultedStr::set(Sit S, const Str& value)
{
    if (specified && value != specifiedValue)
	Err1(S, E1_FORMAT_DUPLICIT_OPTION, ownName());
    if (singleChar && utf8StrLength(value) != 1)
	Err1(S, E1_FORMAT_OPTION_CHAR, ownName());
    specifiedValue = value;
    specified = TRUE;
    return OK;
}

const Str& DefaultedStr::get()
{
    if (specified)
	return specifiedValue;
    else
	return defaultValue;
}

char* DefaultedStr::ownName()
{
    return (char*)xslAttNames[id];
}

void DefaultedStr::report(Sit S, MsgType type, MsgCode code, 
    const Str &arg1, const Str &arg2) const
{
    S.message(type, code, arg1, arg2);
}

//
//
//  DecimalFormatList
//
//  

DecimalFormatList::DecimalFormatList()
{
    initialize();
}

void DecimalFormatList::initialize()
{
    freeall(FALSE);
    EQName emptyName;
    // append the default format
    append(new DecimalFormat(emptyName));
}

DecimalFormatList::~DecimalFormatList()
{
    freeall(FALSE);
}

eFlag DecimalFormatList::add(Sit S, const EQName& name, DecimalFormat*& result)
{
    int ndx = findNdx(name);
    if (ndx != -1)
	result = (*this)[ndx];
    else
	append(result = new DecimalFormat(name));
    return OK;
}

eFlag DecimalFormatList::format(Sit S, const EQName& name, Number& num, const Str& fmt, Str& result)
{
    int ndx = findNdx(name);
    if (ndx != -1)
	E( (*this)[ndx] -> format(S, num, fmt, result) )
    else
    {
	Str fullname;
	name.getname(fullname);
	Err1(S, E1_FORMAT_NOT_FOUND, fullname);
    }
    return OK;
}

int DecimalFormatList::findNdx(const EQName& name)
{
    int i;
    for (i = 0; i < number(); i++)
	if ((*this)[i] -> getname() == name)
	    return i;
    return -1;
}

void DecimalFormatList::report(Sit S, MsgType type, MsgCode code, 
    const Str &arg1, const Str &arg2) const
{
    S.message(type, code, arg1, arg2);
}
