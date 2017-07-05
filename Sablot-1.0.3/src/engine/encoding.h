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

#ifndef EncodingHIncl
#define EncodingHIncl

// GP: clean :-)

#include "base.h"

enum EncMethod
{
    ENC_ICONV,
    ENC_INTERNAL,
    ENC_HANDLER,
    ENC_NONE
};

enum EncResult
{
    ENC_OK,
    ENC_EINVAL,
    ENC_E2BIG,
    ENC_EILSEQ
};


struct ConvInfo
{
    EncMethod method;
    void* physCD;
};

typedef ConvInfo* CDesc;

//  Recoder
//  covers all the encoding stuff, dispatching requests to either iconv, the internal
//  routine (sabconv), or the external encoding handler

class Recoder
{
public:
    Recoder();
    Bool handledByExpat(const Str& enc) const;
    eFlag openFromUTF8(Sit S, const Str& enc, CDesc& cd);
    eFlag openToUTF8(Sit S, const Str& enc, CDesc& cd);
    eFlag conv(Sit S, CDesc cd, const char *&inbuf, size_t &inbytesleft, 
        char *&outbuf, size_t &outbytesleft, EncResult& result);
    eFlag close(Sit S, CDesc cd);
    void clear(Sit S);
    ~Recoder();
private:
    eFlag open(Sit S, const Str& enc, Bool toUTF8, CDesc& cd);
    PList<ConvInfo*> items;
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);	
};

#endif // EncodingHIncl
