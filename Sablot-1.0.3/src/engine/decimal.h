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

#ifndef DecimalHIncl
#define DecimalHIncl

#include "base.h"
#include "datastr.h"

class DefaultedStr;
class Number;

class DefaultedStr
{
public:
    DefaultedStr(const char* defaultValue_, Bool singleChar_, XSL_ATT id_);
    ~DefaultedStr();
    eFlag set(Sit S, const Str& value);
    const Str& get();
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const;
private:
    char* ownName();
    Str defaultValue,
	specifiedValue;
    Bool specified,
	singleChar;
    XSL_ATT id;
};

// the DecimalFormat class made after java.text.DecimalFormat
// it's rather a 'format format' or meta-format

class DecimalFormat
{
public:
    DecimalFormat(const EQName& name);
    ~DecimalFormat();
    eFlag setItem(Sit S, XSL_ATT itemId, const Str& value);
    const Str& getItem(XSL_ATT itemId);
    eFlag format(Sit S, Number& num, const Str& fmt, Str& result);
    const EQName& getname();
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const;
private:
    EQName name;
    DefaultedStr
	decimalSeparator,
	groupingSeparator,
	infinity,
	minusSign,
	NaN,
	percent,
	perMille,
	zeroDigit,
	digit,
	patternSeparator;
    DefaultedStr* findItem(XSL_ATT itemId);
    XSL_ATT whichToken(const char* ptr, int len);
    eFlag parseSubpattern(
	Sit S, const char *&ptr, Bool negative, 
	Str& prefix, Str& suffix, int& factor,
	int& iDigitsMin, int& fDigits, int& fDigitsMin, int& gSize);
    eFlag parse(
	Sit S, const Str &src, Bool negative,
	Str& prefix, Str& suffix, int& factor,
	int& iDigitsMin, int& fDigits, int& fDigitsMin, int& gSize);
};

class DecimalFormatList: public PList<DecimalFormat*>
{
public:
    DecimalFormatList();
    void initialize();
    ~DecimalFormatList();
    eFlag add(Sit S, const EQName& name, DecimalFormat*& result);
    eFlag format(Sit S, const EQName& name, Number& num, const Str& fmt, Str& result);
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const;
private:
    int findNdx(const EQName& name);
};

#endif // DecimalHIncl
