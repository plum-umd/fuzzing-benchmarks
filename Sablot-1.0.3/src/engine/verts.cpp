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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define SablotAsExport
#include "verts.h"
#include "tree.h"
#include "expr.h"
#include "proc.h"
#include "context.h"
#include "vars.h"
#include "parser.h"

#include "guard.h"      // GP: clean
#include "numbering.h"
#include "utf8.h"
#include "domprovider.h"
#include "uri.h"

#ifdef SABLOT_DEBUGGER
#include "debugger.h"
#endif

#ifdef ENABLE_DOM
#include "sdom.h"
#endif

/*****************************************************************
    Instruction tables
*****************************************************************/

struct AttTableItem
{
    XSL_ATT
        attCode;
    Bool 
        required,
        avtemplate;
    ExType
        exprType;
};

typedef AttTableItem AttTable[];
//declarations using AttTable doesn't work with Sun CC.

AttTableItem at_empty[] = 
{
  {XSLA_NONE, FALSE, FALSE, EX_NONE}
};
AttTableItem at_apply_templates[]  = 
{
    {XSLA_SELECT, FALSE, FALSE, EX_NODESET},
    {XSLA_MODE, FALSE, FALSE, EX_NONE}
};
AttTableItem at_attribute[] = 
{
    {XSLA_NAME, TRUE, TRUE, EX_STRING},
    {XSLA_NAMESPACE, FALSE, TRUE, EX_STRING}
};
AttTableItem at_attribute_set[] =
{
    {XSLA_NAME, TRUE, FALSE, EX_NONE},
    {XSLA_USE_ATTR_SETS, FALSE, FALSE, EX_NONE},
};
AttTableItem at_call_template[] = 
{
  {XSLA_NAME, TRUE, FALSE, EX_NONE}
};
AttTableItem  at_copy[] =
{
  {XSLA_USE_ATTR_SETS, FALSE,  FALSE, EX_NONE}
};
AttTableItem at_copy_of[] =
{
  {XSLA_SELECT, TRUE, FALSE, EX_UNKNOWN}
};
AttTableItem at_decimal_format[] =
{
  {XSLA_NAME, FALSE, FALSE, EX_NONE},
  {XSLA_DECIMAL_SEPARATOR, FALSE, FALSE, EX_NONE},
  {XSLA_GROUPING_SEPARATOR, FALSE, FALSE, EX_NONE},
  {XSLA_INFINITY, FALSE, FALSE, EX_NONE},
  {XSLA_MINUS_SIGN, FALSE, FALSE, EX_NONE},
  {XSLA_NAN, FALSE, FALSE, EX_NONE},
  {XSLA_PERCENT, FALSE, FALSE, EX_NONE},
  {XSLA_PER_MILLE, FALSE, FALSE, EX_NONE},
  {XSLA_ZERO_DIGIT, FALSE, FALSE, EX_NONE},
  {XSLA_DIGIT, FALSE, FALSE, EX_NONE},
  {XSLA_PATTERN_SEPARATOR, FALSE, FALSE, EX_NONE}
};
AttTableItem at_element[] =
{
  {XSLA_NAME, TRUE, TRUE, EX_STRING},
  {XSLA_NAMESPACE, FALSE, TRUE, EX_STRING},
  {XSLA_USE_ATTR_SETS, FALSE, FALSE, EX_NONE}
};
AttTableItem at_for_each[] =
{
  {XSLA_SELECT, TRUE, FALSE, EX_NODESET}
};
AttTableItem at_if[] =
{
  {XSLA_TEST, TRUE, FALSE, EX_BOOLEAN}
};
AttTableItem at_import[] =
{
  {XSLA_HREF, TRUE, FALSE, EX_NONE}
};
AttTableItem at_include[] =
{
  {XSLA_HREF, TRUE, FALSE, EX_NONE}
};
AttTableItem at_key[] =
{
  {XSLA_NAME, TRUE, FALSE, EX_NONE},
  {XSLA_MATCH, TRUE, FALSE, EX_NODESET_PATTERN},
  {XSLA_USE, TRUE, FALSE, EX_UNKNOWN}
};
AttTableItem at_message[] =
{
  {XSLA_TERMINATE, FALSE, FALSE, EX_NONE}
};
AttTableItem at_ns_alias[] =
{
  {XSLA_STYLESHEET_PREFIX, TRUE, FALSE, EX_NONE},
  {XSLA_RESULT_PREFIX, TRUE, FALSE, EX_NONE}
};
AttTableItem at_number[] =
{
  {XSLA_LEVEL, FALSE, FALSE, EX_NONE},
  {XSLA_COUNT, FALSE, FALSE, EX_NODESET_PATTERN},
  {XSLA_FROM, FALSE, FALSE, EX_NODESET_PATTERN},
  {XSLA_VALUE, FALSE, FALSE, EX_NUMBER},
  {XSLA_FORMAT, FALSE, TRUE, EX_STRING},
  {XSLA_LANG, FALSE, TRUE, EX_STRING},
  {XSLA_LETTER_VALUE, FALSE, TRUE, EX_STRING},
  {XSLA_GROUPING_SEPARATOR, FALSE, TRUE, EX_STRING},
  {XSLA_GROUPING_SIZE, FALSE, TRUE, EX_STRING}
};
AttTableItem at_output[] =
  {
    {XSLA_METHOD, FALSE, FALSE, EX_NONE},
    {XSLA_VERSION, FALSE, FALSE, EX_NONE},
    {XSLA_ENCODING, FALSE, FALSE, EX_NONE},
    {XSLA_OMIT_XML_DECL, FALSE, FALSE, EX_NONE},
    {XSLA_STANDALONE, FALSE, FALSE, EX_NONE},
    {XSLA_DOCTYPE_PUBLIC, FALSE, FALSE, EX_NONE},
    {XSLA_DOCTYPE_SYSTEM, FALSE, FALSE, EX_NONE},
    {XSLA_CDATA_SECT_ELEMS, FALSE, FALSE, EX_NONE},
    {XSLA_INDENT, FALSE, FALSE, EX_NONE},
    {XSLA_MEDIA_TYPE, FALSE, FALSE, EX_NONE}
  };
AttTableItem at_param[] =
{
  {XSLA_NAME, TRUE, FALSE, EX_NONE},
  {XSLA_SELECT, FALSE, FALSE, EX_UNKNOWN}
};
AttTableItem at_preserve_space[] =
{
  {XSLA_ELEMENTS, TRUE, FALSE, EX_NONE}
};
AttTableItem at_pi[] =
{
  {XSLA_NAME, TRUE, TRUE, EX_STRING}
};
AttTableItem at_sort[] =
{
  {XSLA_SELECT, FALSE, FALSE, EX_STRING},
  {XSLA_LANG, FALSE, TRUE, EX_STRING},
  {XSLA_DATA_TYPE, FALSE, TRUE, EX_STRING},
  {XSLA_ORDER, FALSE, TRUE, EX_STRING},
  {XSLA_CASE_ORDER, FALSE, TRUE, EX_STRING}
};
AttTableItem at_strip_space[] =
{
  {XSLA_ELEMENTS, TRUE, FALSE, EX_NONE}
};
AttTableItem at_template[] =
{
  {XSLA_MATCH, FALSE, FALSE, EX_NODESET_PATTERN},
  {XSLA_NAME, FALSE, FALSE, EX_NONE},
  {XSLA_PRIORITY, FALSE, FALSE, EX_NONE},
  {XSLA_MODE, FALSE, FALSE, EX_NONE}
};
AttTableItem at_text[] =
{
  {XSLA_DISABLE_OUTPUT_ESC, FALSE, FALSE, EX_NONE}
};
AttTableItem at_transform[] =
{
  {XSLA_ID, FALSE, FALSE, EX_NONE},
  {XSLA_EXT_ELEM_PREFIXES, FALSE, FALSE, EX_NONE},
  {XSLA_EXCL_RES_PREFIXES, FALSE, FALSE, EX_NONE},
  {XSLA_VERSION, TRUE, FALSE, EX_NONE}
};
AttTableItem at_value_of[] =
{
  {XSLA_SELECT, TRUE, FALSE, EX_STRING},
  {XSLA_DISABLE_OUTPUT_ESC, FALSE, FALSE, EX_NONE}
};
AttTableItem at_variable[] =
{
  {XSLA_NAME, TRUE, FALSE, EX_NONE},
  {XSLA_SELECT, FALSE, FALSE, EX_UNKNOWN}
};
AttTableItem at_when[] =
{
  {XSLA_TEST, TRUE, FALSE, EX_BOOLEAN}
};
AttTableItem at_with_param[] =
{
  {XSLA_NAME, TRUE, FALSE, EX_NONE},
  {XSLA_SELECT, FALSE, FALSE, EX_UNKNOWN}
};

#define instr(code, elemflg, req, max, table) \
{code, elemflg, req, max, (AttTableItem*) table}

enum ElemFlags
{
//    ELEM_EMPTY = 1,
    ELEM_TOPLEVEL = 2,
    ELEM_INSTR = 4,
    ELEM_EXTRA = 8,
    //
    ELEM_CONT_PCDATA = 0x10,
    ELEM_CONT_TOPLEVEL = 0x20,
    ELEM_CONT_INSTR = 0x40,
    ELEM_CONT_EXTRA = 0x80,
    ELEM_CONT_EXTENSION = 0x100,
    //
    ELEM_SELF = ELEM_TOPLEVEL | ELEM_INSTR | ELEM_EXTRA,
    ELEM_CONT = ELEM_CONT_PCDATA | ELEM_CONT_TOPLEVEL | ELEM_CONT_INSTR | ELEM_CONT_EXTRA,
    //
    ELEM_I0 = ELEM_INSTR,
    ELEM_IX = ELEM_INSTR | ELEM_CONT_EXTRA,
    ELEM_II = ELEM_INSTR | ELEM_CONT_INSTR,
    ELEM_IT_I = ELEM_INSTR | ELEM_TOPLEVEL | ELEM_CONT_INSTR,
    ELEM_I_XI = ELEM_INSTR | ELEM_CONT_EXTRA | ELEM_CONT_INSTR,
    ELEM_I__ = ELEM_INSTR | ELEM_CONT_PCDATA,
    //
    ELEM_T0 = ELEM_TOPLEVEL,
    ELEM_TX = ELEM_TOPLEVEL | ELEM_CONT_EXTRA,
    ELEM_TX_I = ELEM_TOPLEVEL | ELEM_EXTRA | ELEM_CONT_INSTR,
    ELEM_T_XI = ELEM_TOPLEVEL | ELEM_CONT_EXTRA | ELEM_CONT_INSTR,
    //
    ELEM_X0 = ELEM_EXTRA,
    ELEM_XI = ELEM_EXTRA | ELEM_CONT_INSTR,
    ELEM_X_XT = ELEM_EXTRA | ELEM_CONT_EXTRA | ELEM_CONT_TOPLEVEL |
                ELEM_CONT_EXTENSION,
    ELEM_NONE
};

struct InstrTableItem
{
    XSL_OP op;
    ElemFlags flags;
    int 
        reqAtts,
        maxAtts;
    AttTableItem *att;
} instrTable[]
=
{
    instr(XSL_APPLY_IMPORTS,    ELEM_I0,    0, 0, at_empty),
    instr(XSL_APPLY_TEMPLATES,  ELEM_IX,    0, 2, at_apply_templates),
    instr(XSL_ATTRIBUTE,        ELEM_II,    1, 2, at_attribute),
    instr(XSL_ATTRIBUTE_SET,    ELEM_TX,    1, 2, at_attribute_set),
    instr(XSL_CALL_TEMPLATE,    ELEM_IX,    1, 1, at_call_template),
    instr(XSL_CHOOSE,           ELEM_IX,    0, 0, at_empty),
    instr(XSL_COMMENT,          ELEM_II,    0, 0, at_empty),
    instr(XSL_COPY,             ELEM_II,    0, 1, at_copy),
    instr(XSL_COPY_OF,          ELEM_I0,    1, 1, at_copy_of),
    instr(XSL_DECIMAL_FORMAT,   ELEM_T0,    0, 11, at_decimal_format),
    instr(XSL_ELEMENT,          ELEM_II,    1, 3, at_element),
    instr(XSL_FALLBACK,         ELEM_II,    0, 0, at_empty),
    instr(XSL_FOR_EACH,         ELEM_I_XI,  1, 1, at_for_each),
    instr(XSL_IF,               ELEM_II,    1, 1, at_if),
    instr(XSL_IMPORT,           ELEM_T0,    1, 1, at_import),
    instr(XSL_INCLUDE,          ELEM_T0,    1, 1, at_include),
    instr(XSL_KEY,              ELEM_T0,    3, 3, at_key),
    instr(XSL_MESSAGE,          ELEM_II,    0, 1, at_message),
    instr(XSL_NAMESPACE_ALIAS,  ELEM_T0,    2, 2, at_ns_alias),
    instr(XSL_NUMBER,           ELEM_I0,    0, 9, at_number),
    instr(XSL_OTHERWISE,        ELEM_XI,    0, 0, at_empty),
    instr(XSL_OUTPUT,           ELEM_T0,    0, 10, at_output),
    instr(XSL_PARAM,            ELEM_TX_I,  1, 2, at_param),
    instr(XSL_PRESERVE_SPACE,   ELEM_T0,    1, 1, at_preserve_space),
    instr(XSL_PROCESSING_INSTR, ELEM_II,    1, 1, at_pi),
    instr(XSL_SORT,             ELEM_X0,    0, 5, at_sort),
    instr(XSL_STRIP_SPACE,      ELEM_T0,    1, 1, at_strip_space),
    instr(XSL_STYLESHEET,       ELEM_X_XT,  1, 4, at_transform),
    instr(XSL_TEMPLATE,         ELEM_T_XI,  0, 4, at_template),
    instr(XSL_TEXT,             ELEM_I__,   0, 1, at_text),
    instr(XSL_TRANSFORM,        ELEM_X_XT,  1, 4, at_transform),
    instr(XSL_VALUE_OF,         ELEM_I0,    1, 2, at_value_of),
    instr(XSL_VARIABLE,         ELEM_IT_I,  1, 2, at_variable),
    instr(XSL_WHEN,             ELEM_XI,    1, 1, at_when),
    instr(XSL_WITH_PARAM,       ELEM_XI,    1, 2, at_with_param)
};

/****************************************
V e r t e x
****************************************/

Vertex::~Vertex()
{
#ifdef _TH_DEBUG
  printf("Vertex destructor >%x<\n",this);
#endif
    // call DOM dispose callback if set
#ifdef ENABLE_DOM
  if (SDOM_getDisposeCallback())
    (*(SDOM_getDisposeCallback()))(this);
#endif
};

eFlag Vertex::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    sabassert(FALSE);
    return NOT_OK;
}

eFlag Vertex::value(Sit S, DStr& ret, Context *c)
{
    sabassert(0);
    return NOT_OK;
}

eFlag Vertex::startCopy(Sit S, OutputterObj& out)
{
    return OK;
};

eFlag Vertex::endCopy(Sit S, OutputterObj& out)
{
    return OK;
};

eFlag Vertex::copy(Sit S, OutputterObj& out)
{
  S.setCurrSAXLine(lineno); //for SAX copying - be careful

  OutputterObj *aux;
  E( startDocument(S, aux) );

  OutputterObj &useOut = aux ? *aux : out;
  E( startCopy(S, useOut) );
  E( endCopy(S, useOut) );

  E( finishDocument(S) );

  return OK;
}

void Vertex::speak(DStr &ret, SpeakMode mode)
{
}

void Vertex::setParent(Vertex *v)
{
    parent = v;
}

Vertex* Vertex::getPreviousSibling()
{
    if (!parent || !isElement(parent))
        return NULL;
	if (!ordinal)
	    return NULL;
	return toE(parent) -> contents[ordinal - 1];
}

Vertex* Vertex::getNextSibling()
{
    if (!parent || !isElement(parent))
        return NULL;
	if (ordinal >= toE(parent) -> contents.number() - 1)
	    return NULL;
	return toE(parent) -> contents[ordinal + 1];
}

const QName& Vertex::getName() const
{
    return owner.getTheEmptyQName();
}

Tree& Vertex::getOwner() const
{    
    return owner;
}

void Vertex::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2) const
{
    S.setCurrV((Vertex*)this);
    S.setCurrFile(getSubtreeInfo() -> getBaseURI());
    S.message(type, code, arg1, arg2);
}

HashTable& Vertex::dict() const
{
    return getOwner().dict();
}

eFlag Vertex::serialize(Sit S, OutputterObj &out)
{
    return OK;
}

void Vertex::makeStamps(int& stamp_)
{
    stamp = stamp_++;
}

eFlag Vertex::getMatchingList(Sit S, Expression& match, Context& result)
{
  Context aux(NULL);
  aux.set(this);
  Bool yes;
  E( match.matchesPattern(S, &aux, yes) );
  if (yes)
    result.append(this);
  return OK;
}

int Vertex::getImportPrecedence()
{ 
    return NZ( getSubtreeInfo() ) -> getStructure() -> getImportPrecedence(); 
}

eFlag Vertex::startDocument(Sit S, OutputterObj*& out)
{
  Processor *proc;
  if (outputDocument && (proc = S.getProcessor()))
    {
      E( proc -> startDocument(S, outputDocument) );
      out = NZ( outputDocument -> getOutputter() );
    }
  else
    out = NULL;
  return OK;
}

eFlag Vertex::finishDocument(Sit S)
{
  Processor *proc;
  if (outputDocument && (proc = S.getProcessor()))
    {
      E( proc -> finishDocument(S, outputDocument, FALSE) );
    }
  return OK;
}

/****************************************
V e r t e x L i s t
****************************************/

VertexList::VertexList(int logBlockSize_ /*=LIST_SIZE_SMALL*/)
  : SList<Vertex*>(logBlockSize_)
{
}

VertexList::~VertexList()
{
    deppendall();
}

void VertexList::destructMembers()
{
    for (int i = 0; i < nItems; i++)
    {
        Vertex *v = (*this)[i];
	    if (isDaddy(v))
	        toD(v) -> ~Daddy();
		else
		    v -> ~Vertex();
	}
}


eFlag VertexList::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    int i;
    for (i=0; i<number(); i++)
        E( (*this)[i] -> execute(S, c, resolvingGlobals) );
    return OK;
}

eFlag VertexList::value(Sit S, DStr &ret, Context *c)
{
    int i;
    DStr temp;

    ret.empty();
    for (i=0; i<number(); i++)
    {
        E( (*this)[i] -> value(S, temp, c) );
        temp.appendSelf(ret);
    };
    return OK;
}

void VertexList::speak(DStr &ret, SpeakMode mode)
{
    int i;
    for (i = 0; i < number(); i++)
    {
        (*this)[i] -> speak(ret, mode);
        if ((mode & SM_INS_SPACES) && (i < number() - 1))
             ret += ' ';
    };
}

void VertexList::append(Vertex *v)
{
    v -> ordinal = number();
    List<Vertex*>::append(v);
}

int VertexList::strip()
{
    int cnt = 0;
    for (int i = 0; i < number(); i++)
    {
        Vertex *node = (*this)[i];
        if ((node -> vt == VT_TEXT) && isAllWhite((char*)(toText(node) -> cont)))
        {
            cnt++;
            // not doing a freerm() since the node is allocated in the arena
            // freerm(i--,FALSE);
            rm(i--);
        }
    };
    return cnt;
}

eFlag VertexList::copy(Sit S, OutputterObj& out)
{
    for (int i = 0; i < number(); i++)
        E( (*this)[i] -> copy(S, out) );
    return OK;
}

void VertexList::insertBefore(Vertex *newChild, int refIndex)
{
    PList<Vertex*>::insertBefore(newChild, refIndex);
    for (int i = refIndex; i < number(); i++)
        (*this)[i] -> ordinal = i;
}

void VertexList::rm(int ndx)
{
    SList<Vertex*>::rm(ndx);
    for (int i = ndx; i < number(); i++)
        (*this)[i] -> ordinal = i;    
}

int VertexList::getIndex(Vertex *v)
{
    for (int i = 0; i < number(); i++)
        if ((*this)[i] == v) return i;
	return -1;
}

eFlag VertexList::serialize(Sit S, OutputterObj &out)
{
    int i;
    for (i = 0; i < number(); i++)
        E( (*this)[i] -> serialize(S, out) );
    return OK;
}

void VertexList::makeStamps(int& stamp_)
{
    for (int i = 0; i < number(); i++)
        (*this)[i] -> makeStamps(stamp_);
}

eFlag VertexList::getMatchingList(Sit S, Expression& match, Context& result)
{
  for (int i = 0; i < number(); i++)
    E( (*this)[i] -> getMatchingList(S, match, result) );
  return OK;
}

/****************************************
D a d d y
a vertex with contents
****************************************/

Daddy::Daddy(Tree& owner_, VTYPE avt /* = VT_DADDY_WF */)
: Vertex(owner_, avt), contents(&(owner_.getArena()))
{
};


Daddy::~Daddy()
{
    // not doing freeall since it's all in the arena
    // contents.freeall(FALSE);
    contents.destructMembers();
};

eFlag Daddy::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    E( contents.execute(S, c, resolvingGlobals) );
    return OK;
};

eFlag Daddy::value(Sit S, DStr &ret, Context *c)
{
    E( contents.value(S, ret, c) );
    return OK;
}

eFlag Daddy::checkChildren(Sit S)
{
    sabassert(!"Daddy::checkChildren()");
    return NOT_OK;
}

eFlag Daddy::newChild(Sit S, Vertex *v)
{
    // contents.appendAndSetOrdinal(v);
    contents.append(v);
    v -> setParent(this);
    return OK;
};

void Daddy::speak(DStr &ret, SpeakMode mode)
{
    if (mode & SM_CONTENTS)
        contents.speak(ret, mode);
}

int Daddy::strip()
{
    return contents.strip();
}

eFlag Daddy::getMatchingList(Sit S, Expression& match, Context& result)
{
    E( Vertex::getMatchingList(S, match, result) );
    E( contents.getMatchingList(S, match, result) );
	return OK;
}



/****************************************
R o o t N o d e
a vertex with prolog and contents 
****************************************/

RootNode::~RootNode()
{
}

eFlag RootNode::execute(Sit S, Context *c, Bool resolvingGlobals)
{
  //! process prolog
  //
  E( Daddy::execute(S, c, resolvingGlobals) );
  return OK;
};

eFlag RootNode::checkChildren(Sit S)
{
    return OK;
}

eFlag RootNode::newChild(Sit S, Vertex *v)
{
    //! insert prolog
    //
    E( Daddy::newChild(S, v) );
    return OK;
};

void RootNode::speak(DStr &s, SpeakMode mode)
{
    if (mode & SM_DESCRIBE)
        s += "[ROOT]";
    if (mode & SM_CONTENTS)
        Daddy::speak(s,mode);
}

eFlag RootNode::startCopy(Sit S, OutputterObj& out)
{
    return Vertex::startCopy(S, out);
};

eFlag RootNode::endCopy(Sit S, OutputterObj& out)
{
    return Vertex::endCopy(S, out);
};

eFlag RootNode::copy(Sit S, OutputterObj& out)
{
  //RootNode shouldn't need to case for outputDocument

  E( startCopy(S, out) );
  E( contents.copy(S, out) );
  E( endCopy(S, out) );
  return OK;
}

eFlag RootNode::serialize(Sit S, OutputterObj &out)
{
    E( out.eventBeginOutput(S) );
    E( contents.serialize(S, out) );
    E( out.eventEndOutput(S) );
    return OK;
}

/****************************************
E l e m e n t
a vertex with attributes, defs and contents
****************************************/

Element::Element(Tree& owner_, QName& aqname, VTYPE avt /*= VT_ELEMENT_WF*/)
: Daddy(owner_, avt), 
  namespaces(&(owner_.getArena())), 
  atts(&(owner_.getArena()))
{
    name = aqname;
    //origContext = NULL;
    //exNS = NULL;
    //extNS = NULL;
    attsNames = NULL;
    preserveSpace = FALSE;
#ifdef SABLOT_DEBUGGER
    breakpoint = NULL;
#endif
};

Element::~Element()
{
    // not doing freeall since it's all in the arena
    // namespaces.freeall(FALSE);
    // atts.freeall(FALSE);
    if (attsNames) {
      attsNames -> freeall(FALSE);
      delete attsNames;
    }
    namespaces.destructMembers();
    atts.destructMembers();
};

int Element::strip() 
{
  if (!preserveSpace)
    return contents.strip();
  else
    return 0;
}

eFlag Element::execute(Sit S, Context *c, Bool resolvingGlobals)
{
#ifdef SABLOT_DEBUGGER
  if (debugger) E( debugger -> breakOnElement(S, this, c) );
#endif

    EQName expandedName;
    owner.expandQ(name, expandedName);
    OutputterObj * out = NZ(S.getProcessor()) -> outputter();

    //name aliasing
    Bool aliased = false;
    NZ(S.getProcessor()) -> getAliasedName(expandedName, aliased);

    //set subtree for exclusions
    E( out -> eventElementStart(S, expandedName) );
    //output namespaces (and extra aliased namespace if needed)
    E( namespaces.executeSkip(S, c, resolvingGlobals, expandedName, aliased) );
    if (aliased)
      E( out -> eventNamespace(S, expandedName.getPrefix(), 
			       expandedName.getUri() ) )
    //attribute sets
    if (attSetNames(FALSE))
      E( executeAttributeSets(S, c, resolvingGlobals) );
    //plain attributes
    E( atts.execute(S, c, resolvingGlobals) );
    //content
    E( Daddy::execute(S, c, resolvingGlobals) );
    removeBindings(S);
    //end tag
    E( out -> eventElementEnd(S, expandedName));

    return OK;
}

eFlag Element::executeFallback(Sit S, Context *c, Bool &hasSome, 
			       Bool resolvingGlobals)
{
  Processor *proc = NZ(S.getProcessor());
  for (int i = 0; i < contents.number(); i++) 
    {
      Vertex *v = contents[i];
      if (isXSLElement(v) && (toX(v) -> op == XSL_FALLBACK) )
	{
	  proc -> vars -> startApplyOne();
	  E( toE(v) -> contents.execute(S, c, resolvingGlobals) );
	  proc -> vars -> endApplyOne();
	  hasSome = TRUE;
	}
    }
  return OK;
}

eFlag Element::newChild(Sit S, Vertex *v)
{
    v -> setParent(this);
    if isAttr(v)
        atts.append(v);
        // atts.appendAndSetOrdinal(v);
    else
    {
        if (isNS(v))
            namespaces.append(v);
            // namespaces.appendAndSetOrdinal(v);
        else
            E( Daddy::newChild(S, v) );
    }
    return OK;
};

void Element::speak(DStr &ret, SpeakMode mode)
{
    if (mode & (SM_NAME | SM_CONTENTS))
    {
        ret += '<';
	    Str fullName;
	    owner.expandQStr(name, fullName);
	    ret += fullName;
//        name.speak(ret,mode);
        if (mode & SM_CONTENTS)
        {
            if (namespaces.number())
            {
                ret += ' ';
                namespaces.speak(ret, (SpeakMode)(mode | SM_INS_SPACES));
            }
            if (atts.number())
            {
                ret += ' ';
                atts.speak(ret, (SpeakMode)(mode | SM_INS_SPACES));
            };
            {
                ret += '>';
                contents.speak(ret,(SpeakMode)(mode & ~SM_INS_SPACES));
                ret += "</";
        	    ret += fullName;
//                name.speak(ret,mode);
                ret += '>';
            };
        }
        else ret += '>';
    };
};

/*================================================================
removeBindings
    This removes all VARIABLE bindings for the variables
    that are children of this element. The param and with-param
    bindings are removed by the 'vars' list itself.
================================================================*/

void Element::removeBindings(Sit S)
{
    Attribute *a;
    Vertex *v;
    for (int i = contents.number() - 1; i >= 0; i--)
    {
        v = contents[i];
        if (isXSLElement(v) && (cast(XSLElement*, v) -> op == XSL_VARIABLE))
        {
            QName q;
            a = NZ(cast(XSLElement*, v) -> atts.find(XSLA_NAME));
	        // FIXME: check for error in setLogical
            setLogical(S, q, a -> cont, FALSE);
            NZ(S.getProcessor()) -> vars -> rmBinding(q);
        }
    }
}

eFlag Element::startCopy(Sit S, OutputterObj& out)
{
  EQName expandedName;
  owner.expandQ(name, expandedName);
  E( out.eventElementStart(S, expandedName ));
  E( namespaces.copy(S, out) );
  return OK;
}

eFlag Element::endCopy(Sit S, OutputterObj& out)
{
  EQName expandedName;
  owner.expandQ(name, expandedName);
  E( out.eventElementEnd(S, expandedName ));
  return OK;
}

eFlag Element::copy(Sit S, OutputterObj& out)
{
  OutputterObj *aux;
  E( startDocument(S, aux) );

  OutputterObj &useOut = aux ? *aux : out;

  S.setCurrSAXLine(lineno); //for SAX copying - be careful
  E( startCopy(S, useOut) );
  E( atts.copy(S, useOut) );
  E( contents.copy(S, useOut) );
  E( endCopy(S, useOut) );

  E( finishDocument(S) );

  return OK;
};

const QName& Element::getName() const
{
    return name;
}

eFlag Element::setLogical(Sit S, QName &qn, const Str &string, 
			  Bool defaultToo, Phrase defUri /*= UNDEF_PHRASE*/) const
// GP: OK
{
    Phrase thePrefix, resolved;
    char *p = (char *)(Str &) string,
        *q = strchr(p,':'),
        *localPart;
    if (q)
    {
        *q = 0;
        qn.setPrefix(resolved = thePrefix = dict().insert(p));
        *q = ':';
        localPart = q + 1;
    }
    else
    {
        qn.setPrefix(resolved = thePrefix = UNDEF_PHRASE);
        localPart = p;
    };
    
    if (defUri != UNDEF_PHRASE) {
      resolved = defUri;
    }
    else{
      if (thePrefix != getOwner().stdPhrase(PHRASE_XMLNS))
	{
	  E( namespaces.resolve(S, resolved, defaultToo) );
	  // check if a non-undef was resolved to an undef
	  if (resolved == UNDEF_PHRASE && thePrefix != UNDEF_PHRASE)
	    Err1(S, ET_BAD_PREFIX, (char *)(Str &)string);
	}
      else
	resolved = UNDEF_PHRASE;
    }
    if (strchr(localPart,':'))
      Err1(S, E1_EXTRA_COLON, (char *)(Str &)string);
    qn.setUri(resolved);
    qn.setLocal(dict().insert(localPart));
    return OK;
}

eFlag Element::serialize(Sit S, OutputterObj &out)
{
    EQName ename;
    getOwner().expandQ(name, ename);
    E( out.eventElementStart(S, ename) );
    E( namespaces.serialize(S, out) );
    E( atts.serialize(S, out) );
    E( contents.serialize(S, out) );
    E( out.eventElementEnd(S, ename) );
    return OK;
}

eFlag Element::serializeSubtree(Sit S, OutputterObj &out)
{
    E( out.eventBeginSubtree(S) );

    EQName ename;
    getOwner().expandQ(name, ename);
    E( out.eventElementStart(S, ename) );
    E( namespaces.serialize(S, out) );
    E( atts.serialize(S, out) );
    E( contents.serialize(S, out) );
    E( out.eventElementEnd(S, ename) );

    E( out.eventEndOutput(S) );
    return OK;
}

void Element::removeChild(Vertex *child)
{
    sabassert(child -> parent == this);
    contents.rm(child -> ordinal);
    child -> setParent(NULL);    
}

void Element::makeStamps(int& stamp_)
{
    Vertex::makeStamps(stamp_);
    namespaces.makeStamps(stamp_);
    atts.makeStamps(stamp_);
    contents.makeStamps(stamp_);
}

eFlag Element::getMatchingList(Sit S, Expression& match, Context& result)
{
    E( Vertex::getMatchingList(S, match, result) );
    E( namespaces.getMatchingList(S, match, result) );
    E( atts.getMatchingList(S, match, result) );    
    E( contents.getMatchingList(S, match, result) );
	return OK;
}

QNameList* Element::attSetNames(Bool canCreate) {
  if (!attsNames && canCreate) attsNames = new QNameList;
  return attsNames;
}

eFlag Element::executeAttributeSets(Sit S, Context *c, Bool resolvingGlobals)
{
  Processor *proc = NZ(S.getProcessor());
  proc -> vars -> pushCallLevel(0);
  if (attsNames)
    {
      for (int i = 0; i < attsNames -> number(); i++)
	{
	  QNameList history;
	  E( getOwner().attSets().executeAttSet(S, *((*attsNames)[i]), 
						c, getOwner(), history, 
						resolvingGlobals) );
	}
    }
  proc -> vars -> popCallLevel();
  return OK;
}

/****************************************
A t t r i b u t e
****************************************/

Attribute::Attribute(Tree &owner_, const QName &aqname, 
    const Str &acont, XSL_ATT aop)
: Vertex(owner_, (VTYPE) (VT_ATTRIBUTE_WF | 
    (aop != XSLA_NONE ? VT_XSL : 0))),
    cont(&(owner_.getArena()))
{
    expr = NULL;
    name = aqname;
    cont = acont;
    op = aop;
};

Attribute::~Attribute()
{
    if (expr)
        delete expr;
}

eFlag findAVTBrace(Sit S, char *&p, char which, DStr &copybuf)
{
    char *p0 = p; // ,c;
    int len;
    copybuf.empty();
    while (*p)
    {
        if (*p != which) 
            p++;
        else
        {
            if (*(p+1) == which)
            {
                len = p+1 - p0;
                if (len)
                    copybuf.nadd(p0, len);
                p += 2;
                p0 = p;
            }
            else break;
        };
    };
    len = p - p0;
    if (len) copybuf.nadd(p0, len);
    return OK;
}

eFlag Attribute::buildExpr(Sit S, Bool asTemplate, ExType ty)
{
    char *q;
    GP( Expression ) eadd;
    if (asTemplate)
    {
        DStr sadd;
        expr = new Expression(*toE(parent), EXF_STRINGSEQ);  // GP: OK
        char *p = (char*) cont;
        while (*p)
        {
            E( findAVTBrace(S, q = p, '{', sadd) );
            if (!sadd.isEmpty())
            {
                eadd = new Expression(*toE(parent), EXF_ATOM);
                (*eadd).setAtom(sadd);
                expr -> args.append(eadd.keep());
            };
            if (!*q) break;
            //
            if (!*(p = q + 1)) break;
            E( findAVTBrace(S, q = p, '}',sadd) );
            if (!sadd.isEmpty())
            {
                eadd = new Expression(*toE(parent));
                E( (*eadd).parse(S, sadd));
                expr -> args.append(eadd.keep());
            };
            if (!*q) break;
            p = q + 1;
        }
    }
    else
    {
        expr = new Expression(*toE(parent));    // GP: OK
        E( expr -> parse(S, cont, (Bool) (ty == EX_NODESET_PATTERN)) );
    }
    return OK;
}

eFlag Attribute::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    sabassert(parent);
    EQName ename;
    owner.expandQ(name, ename);
    OutputterObj &out = *(NZ(S.getProcessor()) -> outputter());

    if (isXSLElement(parent) || op != XSLA_NONE || 
	ename.getUri() == theXSLTNamespace )
      {
        return OK;
      }

    E( out.eventAttributeStart(S, ename) );
    DStr temp;
    E( value(S, temp,c) );
    E( out.eventData(S, temp) );
    E( out.eventAttributeEnd(S) );
    return OK;
}

eFlag Attribute::value(Sit S, DStr &ret, Context *c)
{
    if (expr)
    {
        Expression temp(*toE(parent));
        E( expr -> eval(S, temp, c) );
        E( temp.tostring(S, ret) );
    }
    else
        ret = cont;
    return OK;
}

void Attribute::speak(DStr &ret, SpeakMode mode)
{
    if (mode & (SM_NAME | SM_CONTENTS))
    {
	    Str fullName;
	    owner.expandQStr(name, fullName);
	    ret += fullName;
	    // name.speak(ret,mode);
	}
    if (mode & SM_CONTENTS)
    {
        ret += "=\"";
        // escape whitespace and quotes in value
        DStr escapedCont;
        const char* escNTQLG[] = 
            {escNewline, escTab, escQuote, escLess, escGreater, NULL};
        escapeChars(escapedCont, cont, "\n\t\"<>", escNTQLG);
        escapedCont.appendSelf(ret);
        ret += '\"';
    }
}

eFlag Attribute::startCopy(Sit S, OutputterObj& out)
{
    EQName ename;
    owner.expandQ(name, ename);    
    E( out.eventAttributeStart(S, ename) );
    E( out.eventData(S, cont) );
    E( out.eventAttributeEnd(S) );
    return OK;
}

const QName& Attribute::getName() const
{
    return name;
}

void Attribute::setValue(const Str& newValue)
{
    cont = newValue;
}

eFlag Attribute::serialize(Sit S, OutputterObj &out)
{
    EQName ename;
    getOwner().expandQ(name, ename);
    E( out.eventAttributeStart(S, ename) );
    E( out.eventData(S, cont) );
    E( out.eventAttributeEnd(S) );
    return OK;
}

/****************************************
A t t L i s t
****************************************/

Attribute* AttList::find(XSL_ATT what)
{
    int i;
    Attribute *a;
    int num = number();
    for (i = 0; i < num; i++)
    {
	// need to use a temporary variable
	// to get around Solaris template problem
        Vertex * pTemp = (*this)[i];
        a = cast(Attribute *, pTemp);
        if (a -> op == what)
            return a;
    };
    return NULL;
}

int AttList::findNdx(const QName& attName)
{
    int i;
    Attribute *a;
    int num = number();
    for (i = 0; i < num; i++)
    {
	// need to use a temporary variable
	// to get around Solaris template problem
        Vertex * pTemp = (*this)[i];
        a = toA(pTemp);
        if (attName == a -> getName())
            return i;
    };
    return -1;
}

Attribute* AttList::find(const QName& attName)
{
    int ndx = findNdx(attName);
    // need to use a temporary variable
    // to get around Solaris template problem
    if (ndx != -1)
      {
	Vertex * pTemp = (*this)[ndx];
	return toA(pTemp);
      } 
    else
      {
	return NULL;
      }
}

/*****************************************************************
    N m S p a c e
*****************************************************************/
//NmSpace::NmSpace(Tree& owner_, Phrase _prefix, Phrase _uri, NsKind _kind)
NmSpace::NmSpace(Tree& owner_, Phrase _prefix, Phrase _uri, 
		 Bool _excluded, NsKind _kind)
: Vertex(owner_, VT_NAMESPACE)
{
    prefix = _prefix;
    uri = _uri;
    name.setLocal(prefix);
    kind = _kind;
    excluded = _excluded;
    usageCount = 0;
}

NmSpace::~NmSpace()
{
}

eFlag NmSpace::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    sabassert(parent);

    E( NZ(S.getProcessor()) -> outputter() -> 
       eventNamespace(S,
		      //was: proc -> aliasXL(prefix)),
		      getOwner().dict().getKey(prefix), 
		      getOwner().dict().getKey(uri),
		      excluded
		      ));
    return OK;
}

eFlag NmSpace::executeSkip(Sit S, Context *c, Bool resolvingGlobals,
			   EQName & exName, Bool aliased)
{
    sabassert(parent);

    const Str & pStr = getOwner().dict().getKey(prefix);
    const Str & uStr = getOwner().dict().getKey(uri);

    if (!aliased || !(exName.getPrefix() == pStr))
      E( NZ(S.getProcessor()) -> outputter() -> 
	 eventNamespace(S, pStr, uStr, excluded) );
    return OK;
}

void NmSpace::speak(DStr& s, SpeakMode mode)
{
    s += "xmlns";
    if (prefix != UNDEF_PHRASE)
    {
        s += ':';
        s += getOwner().dict().getKey(prefix);
    }
    s += "=\"";
    s += getOwner().dict().getKey(uri);
    s += '\"';
}

eFlag NmSpace::value(Sit S, DStr &s, Context *c)
{
    s = getOwner().dict().getKey(uri);
    return OK;
}

eFlag NmSpace::startCopy(Sit S, OutputterObj& out)
{
    E( out.eventNamespace(S,
			  getOwner().dict().getKey(prefix), 
			  getOwner().dict().getKey(uri),
			  excluded) );
    return OK;
}

const QName& NmSpace::getName() const
{
    return name;
}

eFlag NmSpace::serialize(Sit S, OutputterObj &out)
{
    const Str &prefixStr = getOwner().expand(prefix),
        &uriStr = getOwner().expand(uri);
	// hide the "xml" binding
	if (!(prefixStr == "xml"))
        E( out.eventNamespace(S, prefixStr, uriStr) );
    return OK;
}

NsKind NmSpace::setKind(NsKind kind_) {
    if (kind < kind_) kind = kind_;
    return kind;
}
/*****************************************************************
    N S L i s t
*****************************************************************/

NSList::~NSList()
{
}

NmSpace* NSList::find(Phrase prefix) const
{
    int ndx = findNdx(prefix);
    return (ndx == -1) ? NULL : toNS((*this)[ndx]);
}

int NSList::findNdx(Phrase prefix) const
{
    int i;
    for (i = 0; i < number(); i++) 
      if (toNS((*this)[i]) -> prefix == prefix)
	return i;
    return -1;
}

eFlag NSList::resolve(Sit S, Phrase &what, Bool defaultToo) const
{
    Bool emptystr = (what == UNDEF_PHRASE);
    if (emptystr && !defaultToo)
        return OK;
    NmSpace *p = find(what);
    if (!p)
    {
        if (!emptystr)
            what = UNDEF_PHRASE;
    }
    else
        what = p -> uri;
    return OK;
}

void NSList::unresolve(Phrase &what) const
{
    sabassert(what != UNDEF_PHRASE);
    NmSpace *currNS;
    for (int i = 0; i < number(); i++)
    {
        currNS = toNS((*this)[i]);
        if (what == currNS -> uri)
        {
            what = currNS -> prefix;
            return;
        }
    };
    sabassert(0);
}

/*
appends all current namespace declarations as vertices to tree t
'other' is the list where they are being appended in t
*/

void NSList::giveCurrent(Sit S, NSList &other, Tree *t, int nscount) const
{
    const NmSpace *currNS, *addedNS;
    const int imax = number() - 1;
    UriList &excludedNS = t -> getCurrentInfo() -> getExcludedNS();
    for (int i = imax; i >= 0; i--)
    {
        currNS = toNS((*this)[i]);
        if (other.findNdx(currNS -> prefix) == -1) {
	    addedNS = new(&(NZ(t) -> getArena())) 
	      NmSpace(*t, currNS -> prefix, currNS -> uri, 
		      excludedNS.findNdx(currNS->uri)!=-1 || currNS->excluded,
		      i > (imax - nscount) ?  NSKIND_DECLARED : NSKIND_PARENT);
            t -> appendVertex(S, toV(addedNS));
	    // _JP_ ??? why not other.append(toV(addedNS));
	};
    }
}

void NSList::swallow(Sit S, NSList &other, Tree* srcTree, Tree *t)
{
  for (int i = 0; i < other.number(); i++)
    {
      NmSpace *currNS = toNS(other[i]);

      Phrase p, u;
      //translate between dictionaries
      if (srcTree && srcTree != t)
	{
	  if (currNS -> prefix == UNDEF_PHRASE)
	    {
	      p = UNDEF_PHRASE;
	    }
	  else {
	    const Str &aux1 = srcTree -> expand(currNS -> prefix);
	    p = t -> unexpand(aux1);
	  }

	  const Str &aux2 = srcTree -> expand(currNS -> uri);
	  u = t -> unexpand(aux2);
	}
      else
	{
	  sabassert(!srcTree);
	  p = currNS -> prefix;
	  u = currNS -> uri;
	}

      NmSpace *nm = new (&(NZ(t) -> getArena())) NmSpace(*t, p, u, NSKIND_PARENT);
      append(nm);
    }
}


void NSList::findPrefix(QName &q)
{
    if (q.getUri() == UNDEF_PHRASE)
        q.setPrefix(UNDEF_PHRASE);
    else
    {
        Phrase thePrefix = q.getUri();
        unresolve(thePrefix);
	    q.setPrefix(thePrefix);
	}
}

void NSList::report(Sit S, MsgType type, MsgCode code, 
    const Str &arg1, const Str &arg2) const
{
    S.message(type, code, arg1, arg2);
}

void NSList::setPrefixKind(Phrase prefix_, NsKind kind_) const
{
    int ndx = findNdx(prefix_);
    if (ndx != -1) {
      toNS((*this)[ndx]) -> setKind(kind_);
    };
}

void NSList::incPrefixUsage(Phrase prefix_) const
{
    int ndx = findNdx(prefix_);
    if (ndx != -1) {
      toNS((*this)[ndx]) -> usageCount++;
    };
}

void NSList::decPrefixUsage(Phrase prefix_) const
{
    int ndx = findNdx(prefix_);
    if (ndx != -1) {
      toNS((*this)[ndx]) -> usageCount--;
    };
}

eFlag NSList::executeSkip(Sit S, Context * c, Bool resolvingGlobals,
			  EQName & exName, Bool aliased)
{
  int i;
  
  for (i=0; i<number(); i++)
    E( toNS((*this)[i]) -> executeSkip(S, c, resolvingGlobals,
				 exName, aliased) );
  return OK;
}

/****************************************
T e x t
****************************************/

Text::Text(Tree& owner_, char *acont, int alen /* =0 */)
    : Vertex(owner_, VT_TEXT_WF), cont(&(owner_.getArena()))
{
    if (alen)
        cont.nset(acont,alen);
    else cont = (char*) acont;
    isCDATAFlag = FALSE;
}

Text::~Text()
{
}

eFlag Text::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    E( NZ(S.getProcessor()) -> outputter() -> eventData(S, cont) );
    return OK;
}

eFlag Text::value(Sit S, DStr& ret, Context *c)
{
    ret = cont;
    return OK;
}

void Text::speak(DStr &ret, SpeakMode mode)
{
    if (mode & SM_ESCAPE)
        cont.speakTerse(ret);
    else
        ret += cont;
};

eFlag Text::startCopy(Sit S, OutputterObj& out)
{
    E( out.eventData(S, cont) );
    return OK;
}

eFlag Text::serialize(Sit S, OutputterObj &out)
{
    if (isCDATAFlag)
        E( out.eventCDataSection(S, cont) )
	else
        E( out.eventData(S, cont) );
    return OK;
}

void Text::beCDATA()
{
    isCDATAFlag = TRUE;
}

Bool Text::isCDATA()
{
    return isCDATAFlag;
}

/****************************************
C o m m e n t
****************************************/

Comment::Comment(Tree& owner_, const Str& cont_)
: 
Vertex(owner_, VT_COMMENT), cont(&(owner_.getArena()))
{
    cont = cont_;
}

Comment::~Comment()
{
}

eFlag Comment::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    return OK;
}

eFlag Comment::value(Sit S, DStr& ret, Context *c)
{
    ret = cont;
    return OK;
}

void Comment::speak(DStr &ret, SpeakMode mode)
{
    ret += cont;
};

eFlag Comment::startCopy(Sit S, OutputterObj& out)
{
    E( out.eventCommentStart(S) );
    E( out.eventData(S, cont) );
    E( out.eventCommentEnd(S) );
    return OK;
}

eFlag Comment::serialize(Sit S, OutputterObj &out)
{
    E( out.eventCommentStart(S) );
    E( out.eventData(S, cont) );
    E( out.eventCommentEnd(S) );
    return OK;
}

/****************************************
P r o c I n s t r
****************************************/

ProcInstr::ProcInstr(Tree& owner_, Phrase name_, const Str& cont_)
: 
Vertex(owner_, VT_PI), cont(&(owner_.getArena()))
{
    name.empty();
    name.setLocal(name_);
    cont = cont_;
}

ProcInstr::~ProcInstr()
{
}

eFlag ProcInstr::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    return OK;
}

eFlag ProcInstr::value(Sit S, DStr& ret, Context *c)
{
    ret = cont;
    return OK;
}

void ProcInstr::speak(DStr &ret, SpeakMode mode)
{
    ret += cont;
};

eFlag ProcInstr::startCopy(Sit S, OutputterObj& out)
{
    E( out.eventPIStart(S, owner.expand(name.getLocal())) );
    E( out.eventData(S, cont) );
    E( out.eventPIEnd(S) );
    return OK;
}

const QName& ProcInstr::getName() const
{
    return name;
}

eFlag ProcInstr::serialize(Sit S, OutputterObj &out)
{
    const Str& nameStr = getOwner().expand(name.getLocal());
    E( out.eventPIStart(S, nameStr) );
    E( out.eventData(S, cont) );
    E( out.eventPIEnd(S) );
    return OK;
}

/****************************************
X S L E l e m e n t
****************************************/

XSLElement::XSLElement(Tree& owner_, QName& aqname, XSL_OP code)
:
Element(owner_, aqname, VT_XSL_ELEMENT_WF)
{
    sabassert(code != XSL_NONE);
    op = code;
};


eFlag XSLElement::newChild(Sit S, Vertex *v)
{
    // process defs
    //
    E( Element::newChild(S, v) );
    return OK;
};

eFlag XSLElement::execute(Sit S, Context *c, Bool resolvingGlobals)
{
    Attribute *a;
    Bool didNotExecute = FALSE;
    Processor *proc = NZ(S.getProcessor());
    // ???
    if (c -> isFinished())
        return OK;

    //c->setCurrentNode(NULL); _cn_

    NodeHandle v = c -> current();
    sabassert(v);

    //we store the current context to the element
    //e.g. the current() function needs it
    //setOrigContext(c);

#ifdef SABLOT_DEBUGGER
    if (debugger) E( debugger -> breakOnElement(S, this, c) );
#endif

    switch(op)
    {
    case XSL_APPLY_TEMPLATES:
    {
	Attribute *aSelect = atts.find(XSLA_SELECT);
	GP( Expression ) e;
	if (aSelect)
	    e.assign( NZ(aSelect -> expr) );
	else
	{
	    e = new Expression(*this, EXF_LOCPATH);
	    (*e).setLS(
		AXIS_CHILD, 
		EXNODE_NODE);
	};

	// mode
	Bool addedMode = TRUE;
	a = atts.find(XSLA_MODE);
	if (a)
	{
	    GP( QName ) m = new QName;
	    E( setLogical(S, *m, a -> cont,FALSE) );
	    proc -> pushMode(m.keep());
	}
	else if (proc -> getCurrentMode())
	    proc -> pushMode(NULL);
	else addedMode = FALSE;
            
	GP( Context ) newc;
	newc.assign(c);
	E( (*e).createContext( S, newc ) );
	newc.unkeep();
	if (!(*newc).isVoid())
	{
	    // *** SORT HERE ***
	    SortDefList sortDefs;
	    makeSortDefs(S, sortDefs, c);
	    if (sortDefs.number())
		E( (*newc).sort(S, this, &sortDefs) );
	    sortDefs.freeall(FALSE);

	    // process with-params
	    E( contents.execute(S, c, resolvingGlobals) );
	    // proc will dispose of newc:
	    E( proc -> execute(S, (Vertex*) NULL, newc, resolvingGlobals) );
	    newc.keep();
	    // remove the prebindings introduced by with-param
	    proc -> vars -> rmPrebindings();
	}
	else
	    newc.del();    // GP: done automatically anyway
	if (addedMode)
	    proc -> popMode();
	if (!aSelect)
	    e.del();    // GP: done automatically anyway
    }; break;

    case XSL_APPLY_IMPORTS:
    {
	E( proc -> execApplyImports(S, c, subtree, resolvingGlobals) );
    }; break;
    
    case XSL_IF:
    {
	a = NZ( atts.find(XSLA_TEST) );
	Expression boolexpr(*this);
	E( NZ(a -> expr) -> eval(S, boolexpr,c) );
	Bool boolval = boolexpr.tobool();
	if ((boolval) && (contents.number()))
	    E( contents.execute(S, c, resolvingGlobals) )
		else
		    didNotExecute = TRUE;
    }; break;
    case XSL_CHOOSE:
    {
	Bool done = FALSE;
	for (int i = 0; i < contents.number() && !done; i++)
	{
	    // need to use a temporary variable
	    // to get around Solaris template problem
            Vertex * pTemp = contents[i];
	    XSLElement *x = cast(XSLElement*,pTemp);
	    //needed for the current() function
	    //problem was, that trueFor is called before x -> execute
	    //x -> setOrigContext(c); 
	    if (x -> op == XSL_WHEN)
	    {
	      a = NZ( x -> atts.find(XSLA_TEST) );
	      Expression boolexpr(*this);
	      E( NZ(a -> expr) -> eval(S, boolexpr, c) );
	      done = boolexpr.tobool();

	      if (done) 
		E( x -> execute(S, c, resolvingGlobals) );
	    }
	    else
	    {
		sabassert(x -> op == XSL_OTHERWISE);
		E( x -> execute(S, c, resolvingGlobals) );
	    };
	};
    }; break;
    case XSL_WHEN: // condition tested by CHOOSE
    {
	E( contents.execute(S, c, resolvingGlobals) );
    }; break;
    case XSL_OTHERWISE: // condition tested by CHOOSE
    {
	E( contents.execute(S, c, resolvingGlobals) );
    }; break;
    case XSL_ELEMENT:
    {
	QName q;
	DStr nameStr;
	E( NZ( atts.find(XSLA_NAME) ) -> value(S, nameStr, c) );
	if (! isValidQName((char*)nameStr) ) 
	  Err1(S, E_INVALID_QNAME, (char*)nameStr);

	Phrase nsuri = UNDEF_PHRASE;
	DStr uristr;
	//prepare for namespace handling
	Attribute *a = atts.find(XSLA_NAMESPACE);
	if (a) {
	  E( a -> value(S, uristr, c) );
	  nsuri = owner.unexpand(uristr);
	}

	E( setLogical(S, q, nameStr, TRUE, nsuri) );

	Str prefix = owner.expand(q.getPrefix());
	if (nsuri != UNDEF_PHRASE &&
	    (q.getPrefix() == UNDEF_PHRASE || prefix == "xmlns"))
	  {
    	    Str pfx = S.getProcessor() -> getNextNSPrefix();
    	    q.setPrefix(owner.unexpand(pfx));
	  }

	EQName ename;
	owner.expandQ(q, ename);

	E( proc -> outputter() -> eventElementStart(S, ename) );
	//we have to inherit namespaces
	E( namespaces.execute(S, c, resolvingGlobals) );
	//add namespace from 'namespace' attribute
	if (a) 
	  {
	    prefix = ename.getPrefix();
	    proc -> outputter() -> eventNamespace(S, prefix, uristr);
	  }
	//execute attribute sets
	E( executeAttributeSets(S, c, resolvingGlobals) );
	//and so on...
	E( Daddy::execute(S, c, resolvingGlobals) );
	E( proc -> outputter() -> eventElementEnd(S, ename) );
    }; break;

    case XSL_PROCESSING_INSTR:
    {
	QName q;
	DStr nameStr;
	E( NZ( atts.find(XSLA_NAME) ) -> value(S, nameStr, c) );
	if (! isValidNCName((char*)nameStr) ) 
	  Err1(S, E_INVALID_NCNAME, (char*)nameStr);

	E( setLogical(S, q, nameStr, FALSE) );
	const Str& qloc = owner.expand(q.getLocal());
	if (( q.getPrefix() != UNDEF_PHRASE ) || 
	    strEqNoCase( qloc, "xml") || (qloc == "") )
	    Err1(S, E1_PI_TARGET, nameStr);
	E( proc -> outputter() -> eventPIStart( S, qloc ) );
	E( contents.execute(S, c, resolvingGlobals) );
	//for better error report
	eFlag flg = proc -> outputter() -> eventPIEnd(S); 
	if (S.getError() == E_INVALID_DATA_IN_PI)
	  Err(S, E_INVALID_DATA_IN_PI);
	E( flg );
    }; break;
    case XSL_COMMENT:
    {
	E( proc -> outputter() -> eventCommentStart(S) );
	E( contents.execute(S, c, resolvingGlobals) );
	E( proc -> outputter() -> eventCommentEnd(S) ); 
    }; break;
    case XSL_ATTRIBUTE:
    {
	QName q;
	DStr nameStr;
	E( NZ( atts.find(XSLA_NAME) ) -> value(S, nameStr, c) );
	if (! isValidQName((char*)nameStr) ) 
	  Err1(S, E_INVALID_QNAME, (char*)nameStr);

	Phrase nsuri = UNDEF_PHRASE;
	DStr uristr;
	//prepare for namespace handling
	Attribute *a = atts.find(XSLA_NAMESPACE);
	if (a) {
	  E( a -> value(S, uristr, c) );
	  nsuri = owner.unexpand(uristr);
	}

	E( setLogical(S, q, nameStr, FALSE, nsuri) );
	
	Str prefix = owner.expand(q.getPrefix());
	if (nsuri != UNDEF_PHRASE && 
	    (q.getPrefix() == UNDEF_PHRASE || prefix == "xmlns"))
	  {

	    Str pfx = S.getProcessor() -> getNextNSPrefix();
	    q.setPrefix(owner.unexpand(pfx));
	  }

	EQName ename;
	owner.expandQ(q, ename);

	//output new namespace if needed
	if (a)
	  {
	    prefix = ename.getPrefix();
	    proc -> outputter() -> eventNamespace(S, prefix, uristr);
	  }
	
	E( proc -> outputter() -> eventAttributeStart(S, ename) );
	E( contents.execute(S, c, resolvingGlobals) );
	E( proc -> outputter() -> eventAttributeEnd(S) );
    }; break;
    case XSL_TEXT:
    {
	Attribute *disableEsc = atts.find(XSLA_DISABLE_OUTPUT_ESC);
	if (disableEsc && disableEsc -> cont == (const char*)"yes")
	    E( proc -> outputter() -> eventDisableEscapingForNext(S) );
	E( contents.execute(S, c, resolvingGlobals) );
    }; break;
    case XSL_TEMPLATE:
      {
	proc -> vars -> startApplyOne();
	E( Daddy::execute(S, c, resolvingGlobals) );
	proc -> vars -> endApplyOne();
      }; break;
    case XSL_FALLBACK: //simply does nothing
      break;
    case XSL_STYLESHEET:
    case XSL_TRANSFORM:
    {
	GP( Context ) newc = c -> copy();
	
	Processor *proc = NZ(S.getProcessor());
	
	// process globals
	//_PH_ globals
	//E( getOwner().resolveGlobals(S, c, S.getProcessor()) );
	// process other top level elements
	for (int i = 0; i < contents.number(); i++)
	{
	    Vertex *son = contents[i];
	    if (isXSLElement(son) && (instrTable[toX(son) -> op].flags & ELEM_TOPLEVEL))
	    {
		switch(toX(son) -> op)
		{
		case XSL_TEMPLATE: 
		    break;
		    // global vars have been executed before
		case XSL_VARIABLE:
		case XSL_PARAM:
		  {
		    //_PH_ globals
		    QName temp;
		    temp.empty();
		    E( proc -> resolveGlobal(S, c, temp, toX(son)) );
		  }; break;
		default: E( son -> execute(S, c, resolvingGlobals) );
		}
	    }
	    //execute top-level extension elements as well
	    if ( isExtension(son) ) {
	      E( son -> execute(S, c, resolvingGlobals) );
	    }
	}

	proc -> vars -> startCall();

	E( proc -> execute(S, (Vertex*) NULL, newc, resolvingGlobals ));

	// newc has been deleted by now
	newc.keep();
	proc -> vars -> endCall();

    }; break;
    case XSL_VALUE_OF:
    {
	a = NZ( atts.find(XSLA_SELECT) );
	Expression temp(*this);
	E( NZ( a -> expr ) -> eval(S, temp, c, resolvingGlobals) );
	// set output escaping
	Attribute *disableEsc = atts.find(XSLA_DISABLE_OUTPUT_ESC);
	if (disableEsc && disableEsc -> cont == (const char*)"yes")
	    E( proc -> outputter() -> eventDisableEscapingForNext(S) );
	// dump contents
	Str cont;
	E( temp.tostring(S, cont) );
	E( proc -> outputter() -> eventData(S, cont) );
    };
    break;
    case XSL_COPY_OF:
    {
	a = NZ( atts.find(XSLA_SELECT) );
	Expression temp(*this);
	E( NZ( a -> expr ) -> eval(S, temp, c, resolvingGlobals) );
	if (temp.type == EX_NODESET)
	{
	    const Context& ctxt = temp.tonodesetRef();
	    int k, kLimit = ctxt.getSize();

	    for (k = 0; k < kLimit; k++)
		// assuming here the context holds physical vertices
	      //E( toPhysical(ctxt[k]) -> copy(S, *(proc -> outputter())) );
	      E( S.dom().copyNode(S, ctxt[k], *(proc -> outputter())) );
	}
	else
	{
	    Str cont;
	    E( temp.tostring(S, cont) );
	    E( proc -> outputter() -> eventData(S, cont) );
	}
    }; break;
    case XSL_COPY:
    {
      NodeHandle curr = c -> current();
      E( S.dom().startCopy(S, curr, *(proc -> outputter())) );
      //E( curr -> startCopy(S, *(proc -> outputter())) );
      //execute attribute sets
      E( executeAttributeSets(S, c, resolvingGlobals) );
      
      E( contents.execute(S, c, resolvingGlobals) );
      E( S.dom().endCopy(S, curr, *(proc -> outputter())) );
      //E( curr -> endCopy(S, *(proc -> outputter())) );
    }; break;
    case XSL_FOR_EACH:
    {
	a = NZ( atts.find(XSLA_SELECT) );
	GP( Context ) newc;
	newc.assign(c);
	E( NZ( a -> expr ) -> createContext(S, newc));
	newc.unkeep();
	if (!(*newc).isVoid() && contents.number())
	{
	    // *** SORT HERE ***
	    SortDefList sortDefs;
	    makeSortDefs(S, sortDefs, c);
	    if (sortDefs.number())
		E( (*newc).sort(S, this, &sortDefs) );
	    sortDefs.freeall(FALSE);

	    E( proc -> execute(S, contents, newc, resolvingGlobals) );
	    // proc disposes of the new context
	    newc.keep();
	}
/*
  else
  newc.del();    // done automatically
*/
    };
    break;
    case XSL_CALL_TEMPLATE:
    {
	QName q;
	a = NZ( atts.find(XSLA_NAME) );
	XSLElement *thatrule;
	E( setLogical(S, q, a -> cont, FALSE) );
	if (!(thatrule = getOwner().findRuleByName(q))) 
	  {
	    Err1(S, E1_RULE_NOT_FOUND, (char*)getOwner().expand(q.getLocal()));
	  }

//            proc -> vars -> rmPrebindings();
	//process with-param
	E( contents.execute(S, c, resolvingGlobals) );
	//execute the other rule
//            proc -> vars -> startLevel();
	E( thatrule -> execute(S, c, resolvingGlobals) );
	proc -> vars -> rmPrebindings();
//            proc -> vars -> endLevel();
    }; break;
    case XSL_MESSAGE:
    {
	DStr msg;
	GP( Expression ) expr = new Expression(*this);
	if (contents.isEmpty())
	{
	  (*expr).setAtom((const char*)"");
	} 
	else 
	{
	    proc -> vars -> startNested();
	    TreeConstructer *newTC;
	    E( proc -> pushTreeConstructer(S, newTC, (*expr).setFragment(), 
					   SAXOUTPUT_INT_PHYSICAL) );
	    E( contents.execute(S, c, resolvingGlobals) );
	    E( proc -> outputter() -> eventEndOutput(S) );
	    E( proc -> popTreeConstructer(S, newTC) );
	    proc -> vars -> endNested();
	};
	GP( Expression ) temp = new Expression(*this);
	E( (*expr).eval(S, *temp,c, resolvingGlobals) );
	(*temp).pTree = (*expr).pTree;
	(*expr).pTree = NULL;
	a =  atts.find(XSLA_TERMINATE);
	Str tempStr;
	(*temp).tostring(S, tempStr); 
	if (a && a -> cont == (const char*) "yes")
	{
	    Err1(S, E1_XSL_MESSAGE_TERM, tempStr);
	} 
	else 
	{
	    Warn1(S, W1_XSL_MESSAGE_NOTERM, tempStr);
	};
/*
  delete NZ(expr);     GP: automatic
  delete NZ(temp);
*/
    };
    break;
    // this is only executed for non-toplevel vars/params
    // (see case XSL_STYLESHEET)
    case XSL_VARIABLE:
    case XSL_WITH_PARAM:
    case XSL_PARAM:
    {
	QName q;

	GP( Expression ) expr = NULL;

	// there must be a 'name' attribute
	a = NZ(atts.find(XSLA_NAME));
	// stick the name into q referring to this element's namespace decl's
	E( setLogical(S, q, a -> cont, FALSE) );

	// if there's a 'select', use the expression it gives
	a = atts.find(XSLA_SELECT);
	if (a)
	  expr.assign(NZ(a -> expr));

	// otherwise, construct a new expression; it may be an empty string
	// if this element has void content, or a result tree fragment otherwise 
	else
	  {
	    expr = new Expression(*this);
	    if (contents.isEmpty())
		(*expr).setAtom((const char*)"");
	    else // result tree fragment
	    {
		// starting a nesting will make the current prebindings
		// invisible
		// don't do it if we are resolving globals
	      
	      proc -> pushInBinding(TRUE);

	      if (!resolvingGlobals)
		proc -> vars -> startNested();
	      TreeConstructer *newTC;
	      E( proc -> pushTreeConstructer(S, newTC, (*expr).setFragment(),
					     SAXOUTPUT_INT_PHYSICAL) );
	      // execute the inside to create the fragment
	      E( contents.execute(S, c, resolvingGlobals) );
	      E( proc -> outputter() -> eventEndOutput(S) );
	      E( proc -> popTreeConstructer(S, newTC) );
	      
	      // end the shadowing of preexisting bindings
	      if (!resolvingGlobals)
		proc -> vars -> endNested();
	      
	      proc -> popInBinding();
	    };
	}
	//
	// evaluate the expression
	GP( Expression ) temp = new Expression(*this);  
	E( (*expr).eval(S, *temp, c, resolvingGlobals) );
	(*temp).pTree = (*expr).pTree;
	(*expr).pTree = NULL;

	// expr is automatically deallocated if newly constructed

	// add the new binding
	switch(op)
	{
	case XSL_PARAM:
	{
	    E( proc -> vars -> addBinding(S, q, temp, FALSE) );
	}; break;
	case XSL_WITH_PARAM:
	{
	    E( proc -> vars -> addPrebinding(S, q, temp) );
	}; break;
	case XSL_VARIABLE:
	{
	    E( proc -> vars -> addBinding(S, q, temp, TRUE) );
	}; break;
	};
	temp.keep();    // deleted when removing the binding
    }; break;
    case XSL_KEY:
    {
	Attribute 
	    *name = atts.find(XSLA_NAME),
	    *match = atts.find(XSLA_MATCH),
	    *use = atts.find(XSLA_USE);
	sabassert(name && match && use && match -> expr && use -> expr);
	EQName ename;
	QName q;
	E( setLogical(S, q, NZ(name) -> cont, FALSE) );
	getOwner().expandQ(q, ename);
	E( proc -> addKey(S, ename, *(match -> expr), *(use -> expr)) );
    }; break;
    case XSL_DECIMAL_FORMAT:
    {
	Attribute *name = atts.find(XSLA_NAME);
	QName q;
	if (name)
	    E( setLogical(S, q, name -> cont, FALSE) );
	EQName ename;
	getOwner().expandQ(q, ename);
	DecimalFormat *decFormat;
	E( proc -> decimals().add(S, ename, decFormat) );
	for (int k = 0; k < atts.number(); k++)
	{
	    if (toA(atts[k]) -> op != XSLA_NAME)
		E( decFormat -> setItem(S, toA(atts[k]) -> op, toA(atts[k]) -> cont) );
	}
    }; break;

#define evalToStr(ATT, VAR) \
{ if (NULL != (e = getAttExpr(ATT)))\
    { E( e -> eval(S, evaluated, c, resolvingGlobals) );\
      E( evaluated.tostring(S, VAR) ); }}

    case XSL_NUMBER:
    {
	// auxiliary functions are in numbering.h
	ListInt theList;
	Str theString;
	// find @value
	Expression *e = getAttExpr(XSLA_VALUE),
	    evaluated(*this, EXF_ATOM);
	if (e)
	{
	    E( e -> eval(S, evaluated, c, resolvingGlobals) );
	    theList.append(evaluated.tonumber(S).round());
	}
	else
	{
	    NumberingLevel level = NUM_SINGLE;
	    Attribute *a;
	    if (NULL != (a = atts.find(XSLA_LEVEL)))
	    {
		if (a -> cont == "multiple")
		    level = NUM_MULTIPLE;
		else if (a -> cont == "any")
		    level = NUM_ANY;
		else if (!(a -> cont == "single"))
		    Err1(S, E1_NUMBER_LEVEL, a -> cont);
	    }
	    E( xslNumberCount(S, level, 
			      getAttExpr(XSLA_COUNT),
			      getAttExpr(XSLA_FROM),
			      v, theList) );
	}
	Str format = "1",
	    lang,
	    letterValueStr,
	    groupingSep;
	int groupingSize = 0;

	evalToStr(XSLA_FORMAT, format);  // defined above case XSL_NUMBER
	evalToStr(XSLA_LANG, lang);

	// get @letter-value
	NumberingLetterValue letterValue = NUM_ALPHABETIC;
	if (NULL != (e = getAttExpr(XSLA_LETTER_VALUE)))
	{
	    E( e -> eval(S, evaluated, c, resolvingGlobals) );
	    E( evaluated.tostring(S, letterValueStr) );
	    if (letterValueStr == "traditional")
		letterValue = NUM_TRADITIONAL;
	    else if (!(letterValueStr == "alphabetic"))
		Err1(S, E1_NUMBER_LETTER_VALUE, letterValueStr);
	}

	// get @grouping-size
	if (NULL != (e = getAttExpr(XSLA_GROUPING_SIZE)))
	{
	    E( e -> eval(S, evaluated, c, resolvingGlobals) );
	    groupingSize = evaluated.tonumber(S).round();
	}

	// get @grouping-separator
	Expression *eSep;
	if (NULL != (eSep = getAttExpr(XSLA_GROUPING_SEPARATOR)))
	{
	    E( eSep -> eval(S, evaluated, c, resolvingGlobals) );
	    E( evaluated.tostring(S, groupingSep) );
	}

	if (e || eSep)
	{
	    if (!e || !eSep || groupingSize <= 0 || utf8StrLength(groupingSep) != 1)
	    {
		Warn(S, W_NUMBER_GROUPING);
		groupingSize = 0;
		groupingSep.empty();
	    }
	}

	E( xslNumberFormat(S, theList, format, 
			   lang, letterValue, groupingSep, groupingSize, theString) );
	
	E( proc -> outputter() -> eventData(S, theString) );
    }; break;
    case XSL_OUTPUT:
    case XSL_NAMESPACE_ALIAS:
      // these were processed during parse
	break;
    case XSL_SORT:
      // processed by parent
	break;
    case XSL_ATTRIBUTE_SET:
      //processed during parse
      break;
    // unsupported instructions that are not considered harmful
    case XSL_STRIP_SPACE:
    case XSL_PRESERVE_SPACE:
      //processed during parse
      //Warn1(S, W1_UNSUPP_XSL, xslOpNames[op]);
	break;
    default: 
	Err1(S, E1_UNSUPP_XSL, xslOpNames[op]);
    };

    //remove the variable bindings that occured inside this element
    if ((op != XSL_TEMPLATE) && (op != XSL_TRANSFORM) && (op != XSL_STYLESHEET) && 
        (op != XSL_FOR_EACH) && !didNotExecute)
        removeBindings(S);
    return OK;
}

Expression* XSLElement::getAttExpr(XSL_ATT code)
{
    Attribute *a = atts.find(code);
    return a ? (a -> expr) : NULL;
}

int XSLElement::strip()
{
    if (op != XSL_TEXT && !preserveSpace)
        return /* defs.strip() + */ contents.strip();
    else return 0;
}

/*................................................................
findAttNdx()
    returns the index of given attribute in the attribute table
    or -1 if not found
................................................................*/

int findAttNdx(InstrTableItem &iitem, Attribute *a)
{
    for (int i = 0; i < iitem.maxAtts; i++)
        if (iitem.att[i].attCode == a -> op) return i;
    return -1;
}

/*================================================================
checkAtts
    called when all attributes have been parsed in. Returns FALSE
    iff they are OK. Return value of TRUE means that the end tag handler
    signals error.
================================================================*/

eFlag XSLElement::checkAtts(Sit S)
{
    InstrTableItem &instrData = instrTable[op];
    sabassert(instrData.op == op);
    int
        attNdx,
        reqCount = 0;
    Attribute *a;
    for (int i = 0; i < atts.number(); i++)
    {
	// need to use a temporary variable
	// to get around Solaris template problem
        Vertex * pTemp = atts[i];
        a = cast(Attribute*, pTemp);
        if (((attNdx = findAttNdx(instrData,a)) == -1) && 
            a -> name.getUri() == UNDEF_PHRASE)
            // FIXME: issue a warning when ignoring an att??
	    {
	        Str fullName;
		    owner.expandQStr(a -> name, fullName);
            Err1(S, ET_BAD_ATTR, fullName);
	    }
        if (instrData.att[attNdx].required)
            reqCount++;
        if (instrData.att[attNdx].exprType != EX_NONE)
	  {
            E( a -> buildExpr(S,
			      instrData.att[attNdx].avtemplate,
			      instrData.att[attNdx].exprType) );
	    if (op == XSL_TEMPLATE && a->op == XSLA_MATCH)
	      {
		//must not contain variable ref
		if (a->expr && a->expr->containsFunctor(EXF_VAR))
		  Err(S, E_VAR_IN_MATCH);
	      }
	  }
    };
    if (reqCount < instrData.reqAtts)
        Err(S, ET_REQ_ATTR);
    return OK;
};

//................................................................

void XSLElement::checkExtraChildren(int& k)
{
    Vertex *w;
    XSL_OP hisop;
    int status = 0;
    for (k = 0; k < contents.number(); k++)
    {
        w = contents[k];
        if (!isXSLElement(w)) return;
        hisop = toX(w) -> op;
        switch(op)
        {
        case XSL_APPLY_TEMPLATES:
            if ((hisop != XSL_SORT) && (hisop != XSL_WITH_PARAM)) return;
            break;
        case XSL_ATTRIBUTE_SET:
            if (hisop != XSL_ATTRIBUTE) return;
            break;
        case XSL_CALL_TEMPLATE:
            if (hisop != XSL_WITH_PARAM) return;
            break;
        case XSL_CHOOSE:
            switch(hisop)
            {
            case XSL_WHEN:
                {
                    if (status <= 1) status = 1;
                    else return;
                }; break;
            case XSL_OTHERWISE:
                {
                    if (status == 1) status = 2;
                    else return;
                }; break;
            default: return;
            }; 
            break;
        case XSL_FOR_EACH:
            if (hisop != XSL_SORT) return;
            break;
        case XSL_STYLESHEET:
        case XSL_TRANSFORM:
            if (hisop != XSL_WITH_PARAM) return;
            break;
        case XSL_TEMPLATE:
            if (hisop != XSL_PARAM) return;
            break;
        default: 
            return;
        }
    }
}

eFlag XSLElement::checkToplevel(Sit S)
{
  //if we're nested in top level foreign node, all is ok
  if (vt & VT_TOP_FOREIGN) return OK;

  if (!(instrTable[op].flags & ELEM_INSTR) && 
      !(instrTable[op].flags & ELEM_EXTRA) &&
      (!isXSL(parent) || 
       !(instrTable[toX(parent) -> op].flags & ELEM_CONT_TOPLEVEL)))
    Err1(S, E1_ELEM_TOPLEVEL, xslOpNames[op]);
  return OK;
}

eFlag XSLElement::checkChildren(Sit S)
{
    InstrTableItem &iData = instrTable[op];
    sabassert(iData.op == op);

    if (!(iData.flags & ELEM_CONT) && contents.number())
        Err1(S, E_ELEM_MUST_EMPTY, xslOpNames[op]);

    int firstAfter = 0;
    if (iData.flags & ELEM_CONT_EXTRA)
        checkExtraChildren(firstAfter);

    for (int k = firstAfter; k < contents.number(); k++)
    {
        Vertex *w = contents[k];

	if (isElement(w) && (toE(w)->getName().getPrefix() != UNDEF_PHRASE) &&
	    (iData.flags & ELEM_CONT_EXTENSION))
	  {
	    continue;
	  }

	if (isElement(w) && (iData.flags & ELEM_CONT_PCDATA)) {
	  Err1(S, E_ELEM_MUST_BE_PCDATA, xslOpNames[op]);
	}

        if (isText(w) || (isElement(w) && !isXSLElement(w)))
        {
            if (!(iData.flags & (ELEM_CONT_INSTR | ELEM_CONT_PCDATA)))
                Err1(S, E_ELEM_CONT_TEXT_OR_LRE, xslOpNames[op]);
        }
        else
        {
            if (isXSLElement(w))
            {
                int hisflags = instrTable[toX(w) -> op].flags;
                if (!(((hisflags & ELEM_TOPLEVEL) && (iData.flags & ELEM_CONT_TOPLEVEL)) ||
                    ((hisflags & ELEM_INSTR) && (iData.flags & ELEM_CONT_INSTR))))
                    Err2(S, E_ELEM_CONTAINS_ELEM, xslOpNames[op], xslOpNames[toX(w) -> op]);
            }
            else
                Err1(S, E_BAD_ELEM_CONTENT, xslOpNames[op]);
        }
    };

    return OK;
}

eFlag XSLElement::make1SortDef(Sit S, SortDef *&def, Context *c)
{
    DStr temp;

    sabassert(op == XSL_SORT);
    def = NULL;
    GP( SortDef ) newDef = new SortDef;
    
    Attribute *a = atts.find(XSLA_SELECT);
    if (a)
        (*newDef).sortExpr = a -> expr;

    a = atts.find(XSLA_LANG);
    if (a)
    {
        E( a -> value(S, temp, c));
        (*newDef).lang = temp;
    }
    else
    {
        // lang is system-dependent!!! How about setting ""?
        (*newDef).lang = "en";
    };

    a = atts.find(XSLA_DATA_TYPE);
    if (a)
    {
        E( a -> value(S, temp, c) );
        if (temp == (const char*) "number")
            (*newDef).asText = FALSE;
        else
            if (!(temp == (const char*) "text"))
                Warn1(S, W1_SORT_DATA_TYPE, temp);
    }

    a = atts.find(XSLA_ORDER);
    if (a)
    {
        E( a -> value(S, temp, c) );
        if (temp == (const char*) "descending")
            (*newDef).ascend = FALSE;
        else if (!(temp == (const char*) "ascending"))
            Warn1(S, W1_SORT_ORDER, temp);
    }
    
    a = atts.find(XSLA_CASE_ORDER);
    if (a)
    {
        E( a -> value(S, temp, c) );
        if (temp == (const char*) "lower-first")
            (*newDef).upper1st = FALSE;
        else 
        {
            if (!(temp == (const char*) "upper-first"))
                Warn1(S, W1_SORT_CASE_ORDER, temp);
            // get upper1st as system default!!
            (*newDef).upper1st = TRUE;
        }
    }
    def = newDef.keep();
    return OK;
}

eFlag XSLElement::makeSortDefs(Sit S, SortDefList &sortDefs, Context *c)
{
    sabassert(op == XSL_APPLY_TEMPLATES || op == XSL_FOR_EACH);

    Vertex *child;
    for (int i = 0; i < contents.number(); i++)
    {
        child = contents[i];
        if (!isXSLElement(child)) break;
        if (toX(child) -> op == XSL_SORT)
        {
            // *** make the single sort def
            SortDef *newDef;
            E( toX(child) -> make1SortDef(S, newDef, c) );
            sortDefs.append(newDef);
#           if !defined(SAB_WCS_COLLATE)
            // warn that string sorting may be incorrect - 
            // sorting as ascii
            if (newDef -> asText)
                Warn(S, W_NO_WCSXFRM);
#           endif

        }
        else if (toX(child) -> op != XSL_WITH_PARAM) break;
    };
    return OK;
}

/****************************************
ExtensionElement
****************************************/
const char* extNSUri[] = {
  "http://exslt.org/functions",
  "http://www.exslt.org/functions",
  "http://exslt.org/common",
  NULL
};

const char* exsltElementsFunctions[] = {
  "script", 
  NULL
};

const char* exsltElementsCommon[] = {
  "document", 
  NULL
};

ExtensionElement::ExtensionElement(Tree& owner_, QName& aqname)
:
Element(owner_, aqname, VT_EXT_ELEMENT_WF)
{
  extns = EXTNS_UNKNOWN;
  op = EXTE_UNKNOWN;
  lookupExt(owner, aqname, extns, op);
};

#ifdef ENABLE_JS
eFlag ExtensionElement::executeEXSLTScript(Sit S, Context *c, 
					   Bool resolvingGlobals)
{
#ifdef SABLOT_DEBUGGER
  if (debugger) E( debugger -> breakOnElement(S, this, c) );
#endif

  QName attName;
  //why no uri?????
  //attName.setUri(getName().getUri());
  attName.setLocal(owner.unexpand("language"));
  Attribute *a = NZ( atts.find(attName) );
  
  QName lang;
  E( setLogical(S, lang, a -> cont, false) );
  //debug
  EQName elang;
  getOwner().expandQ(lang, elang);
  //debug - end
  if (lang.getUri() == UNDEF_PHRASE || elang.getUri() == extNSUri[0])
    {
      if (! (elang.getLocal() == (const char*)"javascript") &&
	  ! (elang.getLocal() == (const char*)"ecmascript"))
	return OK;
    }
  else
    return OK;
  

  //if (!(a -> cont == (const char*) "javascript") && 
  //    !(a -> cont == (const char*) "ecmascript"))
  //  return OK;
  
  //implements-prefix
  attName.setLocal(owner.unexpand("implements-prefix"));
  a = NZ( atts.find(attName) );
  
  Phrase uri = owner.unexpand(a -> cont);
  E( namespaces.resolve(S, uri, TRUE) );
  Str suri = owner.expand(uri);
  //check uri validity!!!
  
  //script
  DStr script;
  
  attName.setLocal(owner.unexpand("src"));
  a = atts.find(attName);
  if (a)
    {
      DataLine src;
      StrStrList &args = S.getProcessor() -> getArgList();
      
      const Str &base = S.findBaseURI(getSubtreeInfo() -> getBaseURI());
      Str absolute;
      makeAbsoluteURI(S, a -> cont, base, absolute);
      
      eFlag status = src.open(S, absolute, DLMODE_READ,  &args);
      
      if (status == OK)
	{
	  char buff[1024];
	  int read;
	  while ((read = src.get(S, buff, 1024)))
	    {
	      script.nadd(buff, read);
	    }
	}
      else
	{
	  E( status );
	}
      E( src.close(S) );
    }
  else
    {
      for (int i = 0; i < contents.number(); i++) 
	{
	  Vertex *v = contents[i];
	  if ( isText(v) ) 
	    {
	      script = script + toText(v) -> cont;
	    }
	}
    }
  E( NZ( S.getProcessor() ) -> evaluateJavaScript(S, suri, script) );
  return OK;
}
#endif

const char* _exsltDocAtts[] = {
  "version", 
  "encoding",
  "omit-xml-declaration",
  "standalone",
  "doctype-public",
  "doctype-system",
  "indent",
  "media-type",
  NULL
};

XSL_ATT _exsltDocMapping[] = {
  XSLA_VERSION,
  XSLA_ENCODING,
  XSLA_OMIT_XML_DECL,
  XSLA_STANDALONE,
  XSLA_DOCTYPE_PUBLIC,
  XSLA_DOCTYPE_SYSTEM,
  XSLA_INDENT,
  XSLA_MEDIA_TYPE,
  XSLA_NONE
};

XSL_ATT _lookupEXSLTDocumentAttr(const char* name)
{
  int idx = lookup(name, _exsltDocAtts);
  return _exsltDocMapping[idx];
}

eFlag ExtensionElement::exsltDocGetOutputterDef(Sit S, Context *c,
						OutputDefinition &def)
{
  int i, attsNumber = atts.number();
  Attribute *theAtt;
  for (i = 0; i < attsNumber; i++)
    {
      theAtt = toA(atts[i]);
      Str attName = getOwner().expand(theAtt -> getName().getLocal());
      Str attUri = getOwner().expand(theAtt -> getName().getUri());

      //skip xslt attributes
      if ( attUri == theXSLTNamespace )
	continue;

      if (attName == (const char*) "method")
	{
	  QName q;
	  EQName eq;
	  DStr val;
	  theAtt -> value(S, val, c);
	  E( setLogical(S, q, val, FALSE) );
	  getOwner().expandQ(q, eq);
	  E( def.setItemEQName(S, XSLA_METHOD, eq, theAtt, 0) );
	}
      else if (attName == (const char*) "cdata-section-elements")
	{
	  QName q;
	  Bool someRemains;
	  Str listPart;
	  DStr val;
	  theAtt -> value(S, val, c);
	  char *p = (char*)val;
	  do
	    {
	      someRemains = getWhDelimString(p, listPart);
	      if (someRemains)
		{
		  E( setLogical(S, q, listPart, TRUE) );
		  EQName expanded;
		  getOwner().expandQ(q, expanded);
		  E( def.setItemEQName(S, XSLA_CDATA_SECT_ELEMS,
				       expanded, theAtt, 0) );
		};
	    }
	  while (someRemains);
	}
      else if ( !(attName == (const char*) "href") )
	{
	  DStr val;
	  theAtt -> value(S, val, c);
	  XSL_ATT attType = _lookupEXSLTDocumentAttr(attName);
	  if (attType == XSLA_NONE)
	    Err1(S, ET_BAD_ATTR, (char*)attName);
	  E( def.setItemStr(S, attType, val, theAtt, 0) );
	  
	};
    };
  return OK;
}

eFlag ExtensionElement::executeEXSLTDocument(Sit S, Context *c, 
					     Bool resolvingGlobals)
{
  Processor *proc = NZ( S.getProcessor() );

  //get output uri
  QName attName;
  attName.setLocal(owner.unexpand("href"));
  Attribute *a = NZ( atts.find(attName) );
  
  DStr href;
  E( a -> value(S, href, c) );

  GP(OutputDefinition) def = new OutputDefinition();
  E( exsltDocGetOutputterDef(S, c, *def) );

  OutputDocument *doc;
  E( proc -> getOutputDocument(S, href, doc, def.keep()) );
  //we do not need to keep the definition, since document
  //initialized the outputter with it (I hope)
  
  Bool handleOutputter = !proc -> isInBinding();
  
  if (handleOutputter)
    {
      S.message(MT_LOG, L2_SUBDOC_STARTED, href, "output");
      E( proc -> startDocument(S, doc) );
    }
  else
    {
      S.message(MT_LOG, L2_SUBDOC_STARTED, href, "variable");
      E( proc -> outputter() -> setDocumentForLevel(S, doc) );
    }

  E( contents.execute(S, c, resolvingGlobals) );

  if (handleOutputter)
    {
      E( proc -> finishDocument(S, doc, TRUE) );
    }

  return OK;
}

eFlag ExtensionElement::execute(Sit S, Context *c, Bool resolvingGlobals) 
{
  switch (op) {
  case EXTE_EXSLT_SCRIPT:
#ifdef ENABLE_JS
    E( executeEXSLTScript(S, c, resolvingGlobals) );
#else
    sabassert(!"JSExtension not built");
#endif
    break;
  case EXTE_EXSLT_DOCUMENT:
    E( executeEXSLTDocument(S, c, resolvingGlobals) );
    break;
  case EXTE_UNKNOWN:
    {
      //do fallback
      Bool hasSome = FALSE;
      executeFallback(S, c, hasSome, resolvingGlobals);
      if (! hasSome) 
	{
	  Str name = owner.expand(getName().getPrefix());
	  name = name + ":";
	  name = name + owner.expand(getName().getLocal());
	  Err1(S, E_UNSUPPORTED_EXELEMENT, (char*)name);
	}
    }; break;
  default:
    {};
  }
  return OK;
}

eFlag ExtensionElement::checkChildren(Sit S) 
{
  switch (op) {
  case EXTNS_EXSLT_FUNCTIONS:
  case EXTNS_EXSLT_FUNCTIONS_2:
    {
      int num = contents.number();
      for (int i = 0; i < num; i++) 
	{
	  Vertex *v = contents[i];
	  if (!isText(v) && 
	      (! isXSLElement(v) || !(toX(v) -> op == XSL_FALLBACK)) )
	  {
	    Err(S, E_BAD_ELEM_CONTENT);
	  }
	}
    }; break;
  }

  return OK;
}

eFlag ExtensionElement::checkHasAttr(Sit S, const char *name)
{
  QName attName;
  attName.setLocal(owner.unexpand(name));
  Attribute *a = atts.find(attName);    
  if (!a) Err1(S, E_ATTR_MISSING, name);
  return OK;
}

eFlag ExtensionElement::checkAtts(Sit S) 
{
  switch (op) {
  case EXTE_EXSLT_SCRIPT:
    {
      E( checkHasAttr(S, "implements-prefix") );
      E( checkHasAttr(S, "language") );
    }; break;
  case EXTE_EXSLT_DOCUMENT:
    {
      E( checkHasAttr(S, "href") );
      //all attributes are value templates
      int num = atts.number();
      for (int i = 0; i < num; i++)
	E( toA(atts[i]) -> buildExpr(S, TRUE, EX_NONE) );
    }; break;
  }
  return OK;
}

void ExtensionElement::lookupExt(Tree &tree, QName &name,
				 ExtNamespace &extns_, ExtElement &op_) 
{
  Str myns = tree.expand(name.getUri());
  Str mylocal = tree.expand(name.getLocal());
  extns_ = (ExtNamespace) lookup(myns, extNSUri);
  switch (extns_) {
  case EXTNS_EXSLT_FUNCTIONS:
  case EXTNS_EXSLT_FUNCTIONS_2:
    {
      int i = lookup(mylocal, exsltElementsFunctions);
      if ( exsltElementsFunctions[i] ) {
	op_ = (ExtElement) (i + (int)EXTE_EXSLT_FUNCTIONS);
      } else {
	op_ = EXTE_UNKNOWN;
      }
    }; break;
  case EXTNS_EXSLT_COMMON:
    {
      int i = lookup(mylocal, exsltElementsCommon);
      if ( exsltElementsCommon[i] ) {
	op_ = (ExtElement) (i + (int)EXTE_EXSLT_COMMON);
      } else {
	op_ = EXTE_UNKNOWN;
      }
    }; break;
  default:
    {
      op_ = EXTE_UNKNOWN;
    }
  }
  //now we have to check, whether the element is really supported
#ifndef ENABLE_JS
  if (op_ == EXTE_EXSLT_SCRIPT) op_ = EXTE_UNKNOWN;
#endif
}

Bool ExtensionElement::elementAvailable(Tree &t, QName &name) 
{
  ExtNamespace ns;
  ExtElement op;
  lookupExt(t, name, ns, op);
#ifndef ENABLE_JS
  if (op == EXTE_EXSLT_SCRIPT) op = EXTE_UNKNOWN;
#endif
  return op != EXTE_UNKNOWN;
}
