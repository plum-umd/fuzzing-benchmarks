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


#ifndef BaseHIncl
#define BaseHIncl

// GP: clean

/****************************************
*                                       *
    i n c l u d e s
*                                       *
****************************************/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

//#define SablotNSBegin namespace _sablotns {
//#define SablotNSEnd }

#include "platform.h"
#include "sablot.h"
// #include "shandler.h"

// see also end of file

// **************************************
// enum for list sizes

//start the internal namespace

enum ListSize
{
    LIST_SIZE_1 = 0,
    LIST_SIZE_2 = 1,
    LIST_SIZE_SMALL = 2,
    LIST_SIZE_8 = 3,
    LIST_SIZE_MEDIUM = 4,
    LIST_SIZE_LARGE = 5,
    LIST_SIZE_EXPR_TOKENS = LIST_SIZE_MEDIUM
};

// powers of 2
#define TWO_TO(EXPONENT) (1L << EXPONENT)

/****************************************
*                                       *
    c o n s t a n t s
*                                       *
****************************************/

//maximum expression length in chars:
#define MAX_EXPR_LEN 80

//max # of params and args passed to SablotProcessVA
#define ARG_COUNT 32

// the size of the buffer allocated from expat
#define PARSE_BUFSIZE 16384
// this is for the buffer with raw text to convert
#define PARSE_CONV_BUFSIZE PARSE_BUFSIZE * 4

#define EPS 1E-10
#define PRIORITY_NOMATCH -10e6

#define NS_SEP '`'


/****************************************
*                                       *
    e n u m s
*                                       *
****************************************/

// vertex type. To be replaced by OTYPE (Sablot object type) (??)

enum VTYPE
{
    VT_VERTEX,VT_ROOT,VT_ELEMENT,VT_ATTRIBUTE,VT_TEXT,
    VT_PI, VT_COMMENT, VT_NAMESPACE,
    VT_VERTEX_WF = VT_VERTEX,
    VT_ATTRIBUTE_WF = VT_ATTRIBUTE,
    VT_TEXT_WF = VT_TEXT,
    VT_BASE = 0x000f,
    VT_DADDY_FLAG = 0x2000,
    VT_DADDY_WF = VT_VERTEX | VT_DADDY_FLAG,
    VT_ELEMENT_WF = VT_ELEMENT | VT_DADDY_FLAG,  //WF = with flags
    VT_ROOT_WF = VT_ROOT | VT_DADDY_FLAG,
    VT_XSL = 0x4000,
    VT_XSL_ELEMENT_WF = VT_XSL | VT_ELEMENT_WF,
    VT_EXT = 0x8000,
    VT_EXT_ELEMENT_WF = VT_EXT | VT_ELEMENT_WF,
    VT_TOP_FOREIGN = 0x10000 //used during parse
};

// SpeakMode. Used to distinguish between vertex output for logging and for 'official' output.

enum SpeakMode
{
    SM_NAME = 1,
        SM_CONTENTS = 2,

    SM_OFFICIAL = SM_NAME | SM_CONTENTS,

        SM_INS_SPACES = 0x1000,
        SM_DESCRIBE = 0x2000,
        SM_ESCAPE = 0x4000
};

/****************************************
    X S L   e n u m s
****************************************/
// the corresponding string tables are declared later and defined in base.cpp

enum    XSL_OP
{
    XSL_APPLY_IMPORTS, XSL_APPLY_TEMPLATES,
    XSL_ATTRIBUTE, XSL_ATTRIBUTE_SET,
    XSL_CALL_TEMPLATE, XSL_CHOOSE,
    XSL_COMMENT, XSL_COPY, XSL_COPY_OF,
    XSL_DECIMAL_FORMAT, XSL_ELEMENT,
    XSL_FALLBACK, XSL_FOR_EACH,
    XSL_IF, XSL_IMPORT,
    XSL_INCLUDE, XSL_KEY,
    XSL_MESSAGE, XSL_NAMESPACE_ALIAS,
    XSL_NUMBER, XSL_OTHERWISE,
    XSL_OUTPUT, XSL_PARAM,
    XSL_PRESERVE_SPACE, XSL_PROCESSING_INSTR,
    XSL_SORT, XSL_STRIP_SPACE,
    XSL_STYLESHEET, XSL_TEMPLATE,
    XSL_TEXT, XSL_TRANSFORM,
    XSL_VALUE_OF, XSL_VARIABLE,
    XSL_WHEN, XSL_WITH_PARAM,
    XSL_NONE
};

enum XSL_ATT
{
    XSLA_CASE_ORDER, XSLA_CDATA_SECT_ELEMS, XSLA_COUNT, 
    XSLA_DATA_TYPE, XSLA_DECIMAL_SEPARATOR, XSLA_DIGIT, XSLA_DISABLE_OUTPUT_ESC, XSLA_DOCTYPE_PUBLIC, XSLA_DOCTYPE_SYSTEM,
    XSLA_ELEMENTS, XSLA_ENCODING, XSLA_EXCL_RES_PREFIXES, XSLA_EXT_ELEM_PREFIXES, 
    XSLA_FORMAT, XSLA_FROM, 
    XSLA_GROUPING_SEPARATOR, XSLA_GROUPING_SIZE,
    XSLA_HREF, 
    XSLA_ID, XSLA_INDENT, XSLA_INFINITY, 
    XSLA_LANG, XSLA_LETTER_VALUE, XSLA_LEVEL,
    XSLA_MATCH, XSLA_MEDIA_TYPE, XSLA_METHOD, XSLA_MINUS_SIGN, XSLA_MODE,
    XSLA_NAME, XSLA_NAMESPACE, XSLA_NAN, 
    XSLA_OMIT_XML_DECL, XSLA_ORDER, 
    XSLA_PATTERN_SEPARATOR, XSLA_PERCENT, XSLA_PER_MILLE, XSLA_PRIORITY,
    XSLA_RESULT_PREFIX,
    XSLA_SELECT, XSLA_STANDALONE, XSLA_STYLESHEET_PREFIX, 
    XSLA_TERMINATE, XSLA_TEST,
    XSLA_USE, XSLA_USE_ATTR_SETS, 
    XSLA_VALUE, XSLA_VERSION, 
    XSLA_ZERO_DIGIT,
    XSLA_NONE
};

enum ExAxis
{
    AXIS_ANCESTOR, AXIS_ANC_OR_SELF, AXIS_ATTRIBUTE, AXIS_CHILD,
        AXIS_DESCENDANT, AXIS_DESC_OR_SELF, AXIS_FOLLOWING, AXIS_FOLL_SIBLING,
        AXIS_NAMESPACE, AXIS_PARENT, AXIS_PRECEDING, AXIS_PREC_SIBLING,
        AXIS_SELF,
        AXIS_NONE,
        
        //not recognized in stylesheet:
        AXIS_ROOT
};

enum ExNodeType
{
    EXNODE_NODE,
    EXNODE_TEXT,
    EXNODE_PI,
    EXNODE_COMMENT,
    EXNODE_NONE
};

/****************************************
*                                       *
    t y p e d e f s
*                                       *
****************************************/

/*    the following was truly a burden:
enum Bool
{
    FALSE = 0,
        TRUE = 1
};
*/

#define ITEM_NOT_FOUND      (unsigned long) -1L
#define PHRASE_NOT_FOUND    ITEM_NOT_FOUND
#define UNDEF_PHRASE        (unsigned long) -2L

typedef unsigned long oolong;
typedef oolong HashId;
typedef HashId Phrase;

typedef int Bool;
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif


/****************************************
*                                       *
    m a c r o s
*                                       *
****************************************/

//this gets rid of warnings on platforms where pointer is 
//sized differently from boolean
#define sabassert(x) (assert(!!(x)))

#define NONZERO(x) (sabassert(x),x)
#define NZ(x) NONZERO(x)
#define cdelete(PTR) {if (PTR) delete PTR; PTR = NULL;}
#define ccdelete(PTR, TYPED) {if (PTR) delete TYPED; PTR = NULL;}
#define cdeleteArr(PTR) {if (PTR) delete[] PTR; PTR = NULL;}

#ifdef _DEBUG
#define cast(TYPE,PTR) (NZ(dynamic_cast<TYPE>(PTR)))
#else
#define cast(TYPE,PTR) ((TYPE) PTR)
#endif

// macros for checking vertex types

#define baseType(v) (((Vertex*)NZ(v)) -> vt & VT_BASE)
#define basetype(v) baseType(v)
#define isDaddy(v) (((Vertex*)NZ(v)) -> vt & VT_DADDY_FLAG)
#define isRoot(v) (baseType(v) == VT_ROOT)
#define isPI(v) (baseType(v) == VT_PI)
#define isComment(v) (baseType(v) == VT_COMMENT)
#define isElement(v) (baseType(v) == VT_ELEMENT || baseType(v) == VT_ROOT)
#define isText(v) (baseType(v) == VT_TEXT)
#define isXSL(v) (((Vertex*)NZ(v)) -> vt & VT_XSL)
#define isExt(v) (((Vertex*)NZ(v)) -> vt & VT_EXT)
#define isXSLElement(v) ((isElement(v)) && (isXSL(v)))
#define isExtension(v) ((isElement(v)) && (isExt(v)))
#define isAttr(v) (baseType(v) == VT_ATTRIBUTE)
#define isNS(v) (baseType(v) == VT_NAMESPACE)

// macros for downcasting

#define toV(v) ((Vertex*)v)
#define toNS(v) (cast(NmSpace*,toV(v)))
#define toE(v) (cast(Element*,toV(v)))
#define toX(v) (cast(XSLElement*,toV(v)))
#define toExtension(v) (cast(ExtensionElement*,toV(v)))
#define toA(v) (cast(Attribute*,toV(v)))
#define toText(v) (cast(Text*,toV(v)))
#define toD(v) (cast(Daddy*,toV(v)))
#define toRoot(v) (cast(RootNode*,toV(v)))
#define toComment(v) (cast(Comment*,toV(v)))
#define toPI(v) (cast(ProcInstr*,toV(v)))
// for "casting" a root node to the containing document
#define toTree(v) (&(toRoot(v) -> getOwner()))
// casts a Tree object to an SXP_Document
#define toSXP_Document(t) &((t).getRoot())

#define toPhysical(v) toV(v)

#define OK FALSE
#define NOT_OK TRUE
#define eFlag Bool
#define BOOL Bool

// return OK, putting a VALUE in a VARiable
#define RetOK( VAR, VALUE ) {VAR = VALUE; return OK;}

// to support the change to Unicode
#define Mchar char
#define Mstrlen strlen
#define Mstrchr strchr
#define Mstrcmp strcmp
#define Mstrncmp strncmp

// parsing macros

extern const char* theWhitespace; 
#define skipWhite(p)    p+=strspn((p), theWhitespace)
#define isWhite(c)  (!!(strchr(theWhitespace,(c))))
#define isAllWhite( STR ) (!((STR)[strspn((STR), theWhitespace)]))

// messaging macro from "Advanced Windows"
// #pragma Msg(some_message) emits a message at compile time
#define MsgToS(x) #x
#define MsgToS2(x) MsgToS(x)
#define Msg(x) message(__FILE__"(" MsgToS2(__LINE__) "): "#x"        <---")

/****************************************
*                                       *
    e x t e r n s
*                                       *
****************************************/

extern const char* xslOpNames[];
extern const char* xslAttNames[];
extern const char* axisNames[];
extern const char* exNodeTypeNames[];
extern const char* vertexTypeNames[];
extern int lookup(const char* str, const char** table);
extern int lookupNoCase(const char* str, const char** table);
extern int fcomp(double,double);        //float comparison
extern Bool isstd(const char *);
extern int stdclose(FILE *f);
extern FILE* stdopen(const char *fn, const char *mode);
extern Bool strEqNoCase(const char*, const char*);
//
class SituationObj;
// extern Processor* proc;
// extern SituationObj situation;
//extern void pushNewHandler();
//extern void popNewHandler();

extern SchemeHandler *theSchemeHandler;
extern MessageHandler *theMessageHandler;
extern SAXHandler *theSAXHandler;
extern void* theSAXUserData;

//XML name checks
extern Bool isValidNCName(const char* name);
extern Bool isValidQName(const char* name);

//string parsing
class Str;
extern Bool getWhDelimString(char *&list, Str& firstPart);

/*
  the XSLT namespace
*/

/* class Str; */
extern const char *theXSLTNamespace, *oldXSLTNamespace, *theXMLNamespace,
  *theXHTMLNamespace, *theXMLNSNamespace, *theSabExtNamespace, 
  *theEXSLTDynNamespace;

//
//  escapes
//

extern const char 
    *escNewline, *escTab, *escQuote, *escApos,
    *escLess, *escGreater;

/****************************************
*                                       *
    c l a s s e s
*                                       *
****************************************/

/*
class SabObj
{
public:
    void *operator new(unsigned int);
    void operator delete(void *, unsigned int);
};
*/

/*****************************************************************
Memory leak test stuff
*****************************************************************/
#if defined(CHECK_LEAKS)
#ifdef WIN32
#define CRTDBG_MAP_ALLOC 
#include <crtdbg.h>
#else // if defined(__linux__)
#include <mcheck.h>
#endif
#endif

extern void checkLeak();
extern void memStats();

class Str;
extern Str getMillisecsDiff(double originalTime);


/****************************************
*                                       *
    f i n a l   i n c l u d e s
*                                       *
****************************************/

#ifndef BaseH_NoErrorH
#include "situa.h"
#else
#undef BaseH_NoErrorH
#endif

#ifdef MPATROL
#include <mpatrol.h>
#endif

#endif //ifndef BaseHIncl
