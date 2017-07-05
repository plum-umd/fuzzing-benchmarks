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
#include <errno.h>

#include "encoding.h"
#include "proc.h"
#include "guard.h"
#include "utf8.h"
// #include "shandler.h"

/*
 ******************************
 * internal recoding functions
 * (shall be replaced with a call to the sabconv library function)
 ******************************
 */

// encoding tables for the functions

short EncTable1250[] =
{
    0x20ac,     -1, 0x201a,     -1, 0x201e, 0x2026, 0x2020, 0x2021, 
        -1, 0x2030, 0x0160, 0x2039, 0x015a, 0x0164, 0x017d, 0x0179, 
        -1, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 
        -1, 0x2122, 0x0161, 0x203a, 0x015b, 0x0165, 0x017e, 0x017a, 
    0x00a0, 0x02c7, 0x02d8, 0x0141, 0x00a4, 0x0104, 0x00a6, 0x00a7, 
    0x00a8, 0x00a9, 0x015e, 0x00ab,     -1, 0x00ad, 0x00ae, 0x017b, 
    0x00b0, 0x00b1, 0x02db, 0x0142, 0x00b4, 0x00b5, 0x00b6, 0x00b7, 
    0x00b8, 0x0105, 0x015f, 0x00bb, 0x013d, 0x02dd, 0x013e, 0x017c, 
    0x0154, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139, 0x0106, 0x00c7, 
    0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce, 0x010e, 
    0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7, 
    0x0158, 0x016e, 0x00da, 0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df, 
    0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7, 
    0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f, 
    0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00f7, 
    0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9
};

short EncTableLatin2[] =
{
        -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1, 
        -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1, 
        -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1, 
        -1,     -1,     -1,     -1,     -1,     -1,     -1,     -1, 
    0x00a0, 0x0104, 0x02d8, 0x0141, 0x00a4, 0x013d, 0x015a, 0x00a7, 
    0x00a8, 0x0160, 0x015e, 0x0164, 0x0179, 0x00ad, 0x017d, 0x017b, 
    0x00b0, 0x0105, 0x02db, 0x0142, 0x00b4, 0x013e, 0x015b, 0x02c7, 
    0x00b8, 0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, 
    0x0154, 0x00c1, 0x00c2, 0x0102, 0x00c4, 0x0139, 0x0106, 0x00c7, 
    0x010c, 0x00c9, 0x0118, 0x00cb, 0x011a, 0x00cd, 0x00ce, 0x010e, 
    0x0110, 0x0143, 0x0147, 0x00d3, 0x00d4, 0x0150, 0x00d6, 0x00d7, 
    0x0158, 0x016e, 0x00da, 0x0170, 0x00dc, 0x00dd, 0x0162, 0x00df, 
    0x0155, 0x00e1, 0x00e2, 0x0103, 0x00e4, 0x013a, 0x0107, 0x00e7, 
    0x010d, 0x00e9, 0x0119, 0x00eb, 0x011b, 0x00ed, 0x00ee, 0x010f, 
    0x0111, 0x0144, 0x0148, 0x00f3, 0x00f4, 0x0151, 0x00f6, 0x00f7, 
    0x0159, 0x016f, 0x00fa, 0x0171, 0x00fc, 0x00fd, 0x0163, 0x02d9, 
};

void* encInternalOpen(const Str& enc, Bool toUTF8)
{
    if (!toUTF8)
        return (void*)-1;
    if (enc.eqNoCase("ISO-8859-2"))
        return EncTableLatin2;
    else if (enc.eqNoCase("windows-1250"))
        return EncTable1250;
    // more builtin conversions can come here
    else return (void*)-1;
}

int encInternalClose(void* intCD)
{
    return 0; // iconv's value for OK
}

// the following only does 1-byte encodings for which an EncTable is defined

EncResult encInternalConv(void *intCD, const char** inbuf, 
                                  size_t *inbytesleft,
                                  char **outbuf, size_t *outbytesleft)
{
    sabassert(intCD && intCD != (void*)-1);
    char charbuf[6];
    int val;
    size_t len;
    unsigned char thischar;
    for (; *inbytesleft > 0; )
    {
        thischar = **inbuf;
        if (thischar < 0x80)
        {
            **outbuf = thischar;
            len = 1;
        }
        else
        {
            val = ((short*)intCD)[thischar - 0x80];
            if (val == -1)
                return ENC_EILSEQ;
            len = utf8FromCharCode(charbuf, val);
            if (len <= *outbytesleft)
                memcpy(*outbuf, charbuf, len);
            else
                return ENC_E2BIG;
        };
        *outbuf += len;
        *outbytesleft -= len;
        (*inbuf)++;
        (*inbytesleft)--;                
    }
    return ENC_OK;
}

/*
 *
 *      Recoder class
 *
 */


Recoder::Recoder()
{
}

Bool Recoder::handledByExpat(const Str& enc) const
{
    return (enc.eqNoCase("UTF-8") ||
        enc.eqNoCase("UTF-16") ||
        enc.eqNoCase("ISO-8859-1") ||
        enc.eqNoCase("US-ASCII"));
}

void Recoder::clear(Sit S)
{
    for (int i = 0; i < items.number(); i++)
    {
        if (items[i] && items[i] -> physCD)
            close(S, items[i]);
    };
    items.freeall(FALSE);
}

Recoder::~Recoder()
{
    // can't clear here (have no situation)
}

eFlag Recoder::open(Sit S, const Str& enc, Bool toUTF8, CDesc& cd)
{
    GP(ConvInfo) newitem = new ConvInfo;
    (*newitem).method = ENC_NONE;
    (*newitem).physCD = NULL;
#ifdef HAVE_ICONV_H
    iconv_t icd = toUTF8 ? iconv_open("UTF-8", enc) : iconv_open(enc, "UTF-8");
    // switch off transliteration in iconv:
    // sadly non-standard, only works in windows port
    // int val = 0;
    // iconvctl(icd, ICONV_SET_TRANSLITERATE, &val);
    if (icd != (iconv_t) -1)
    {
        (*newitem).method = ENC_ICONV;
        (*newitem).physCD = (void *) icd;
    }
    else
#endif
    {
        // try to open internal recode
        void *physcd;
        physcd = encInternalOpen(enc, toUTF8);
        if (physcd != (void*)-1)
        {
            (*newitem).method = ENC_INTERNAL;
            (*newitem).physCD = physcd;
        }
        else
        {
            // try the encoding handler as a last resort
            void* enchlrUD = NULL;
            EncHandler *enchlr = NULL;
	        if (S.getProcessor())
		        enchlr = S.getProcessor() -> getEncHandler(&enchlrUD);
            if (enchlr)
            {
                void *physcd = enchlr -> open(enchlrUD, S.getProcessor(), toUTF8 ? EH_TO_UTF8 : EH_FROM_UTF8, enc);
                if (physcd != (void*) -1)
                {
                    (*newitem).method = ENC_HANDLER;
                    (*newitem).physCD = physcd;
                }
            }
        }
    };
    if ((*newitem).method != ENC_NONE)
        items.append(cd = newitem.keep());
    else
        Err1(S, E1_UNKNOWN_ENC, enc);
    return OK;
}

eFlag Recoder::openFromUTF8(Sit S, const Str& enc, CDesc& cd)
{
    return open(S, enc, FALSE, cd);
};

eFlag Recoder::openToUTF8(Sit S, const Str& enc, CDesc& cd)
{
    return open(S, enc, TRUE, cd);
};

eFlag Recoder::close(Sit S, CDesc cd)
{
    sabassert(cd);
    switch(cd -> method)
    {
    case ENC_ICONV:
#ifdef HAVE_ICONV_H
        iconv_close((iconv_t)(cd -> physCD));
#endif
        break;
    case ENC_INTERNAL:
        encInternalClose(cd -> physCD);
        break;
    case ENC_HANDLER:
        {
            void *enchlrUD = NULL;
	        EncHandler *enchlr = NULL;
	        if (S.getProcessor())
		        enchlr = S.getProcessor() -> getEncHandler(&enchlrUD);
            if (enchlr)
                enchlr -> close(enchlrUD, S.getProcessor(), cd -> physCD);
        }
        break;
    default: 
        sabassert(0);
    };
    return OK;
}

eFlag Recoder::conv(Sit S, CDesc cd, const char *& inbuf, size_t &inbytesleft, 
        char *& outbuf, size_t &outbytesleft, EncResult& result)
{
    sabassert(cd);
    switch(cd -> method)
    {
    case ENC_ICONV:
#ifdef HAVE_ICONV_H
        {
            errno = 0;
            iconv((iconv_t)(cd -> physCD), 
#               ifdef SABLOT_ICONV_CAST_OK
                    (char**)(&inbuf),
#               else
                    &inbuf,
#               endif
                    &inbytesleft, &outbuf, &outbytesleft);
            switch(errno)
            {
            case EINVAL:
                result = ENC_EINVAL; break;
            case E2BIG:
                result = ENC_E2BIG; break;
            case EILSEQ:
                result = ENC_EILSEQ; break;
            default:
                result = ENC_OK;
            };
        };
#else
        sabassert(0);
#endif
        break;
    case ENC_INTERNAL:
        {
            result = 
                encInternalConv(cd -> physCD, &inbuf, &inbytesleft, 
                &outbuf, &outbytesleft);
        };
        break;
    case ENC_HANDLER:
        {
            void *enchlrUD = NULL;
            EncHandler *enchlr = NULL;
	        if (S.getProcessor())
		        enchlr = S.getProcessor() -> getEncHandler(&enchlrUD);
			if (enchlr)
			{
                switch(enchlr -> conv(enchlrUD, S.getProcessor(), cd -> physCD, 
                    &inbuf, &inbytesleft, &outbuf, &outbytesleft))
                {
                case EH_EINVAL: result = ENC_EINVAL;
                    break;
                case EH_E2BIG: result = ENC_E2BIG;
                    break;
                case EH_EILSEQ: result = ENC_EILSEQ;
                    break;
                default: result = ENC_OK;
                };
		    };
        };
        break;
    default:
        sabassert(0);
    };
    return OK;
}

void Recoder::report(Sit S, MsgType type, MsgCode code, const Str &arg1, const Str & arg2)
{
    S.message(type, code, arg1, arg2);
}

