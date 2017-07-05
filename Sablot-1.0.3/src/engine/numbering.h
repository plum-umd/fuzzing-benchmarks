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

#ifndef NumberingHIncl
#define NumberingHIncl

#include "base.h"
#include "datastr.h"

typedef List<int> ListInt;

// possible values of xsl:number/@level

enum NumberingLevel
{
    NUM_SINGLE,
    NUM_MULTIPLE,
    NUM_ANY
};

// possible values of xsl:number/@letter-value

enum NumberingLetterValue
{
    NUM_ALPHABETIC,
    NUM_TRADITIONAL
};

//class Vertex;
class Expression;

// counts nodes as xsl:number without @value should
// curr is the current node
// returns the count in 'result'

eFlag xslNumberCount(
    Sit S, NumberingLevel level, 
    Expression* count, Expression* from, 
    NodeHandle curr, ListInt& result);

// formats 'num' according to the format string
// returns the string in 'result'

eFlag xslNumberFormat(
    Sit S, ListInt& nums, const Str& format, 
    const Str& lang, NumberingLetterValue letterValue, 
    const Str& groupingSep, int groupingSize, Str& result);

#endif // NumberingHIncl
