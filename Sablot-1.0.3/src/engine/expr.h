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

// EXPR.H

#ifndef ExprHIncl
#define ExprHIncl

// GP: clean

#include "base.h"
#include "datastr.h"
#include "proc.h"
#include "sxpath.h"
#include "jsext.h"

class Vertex;

/**********************************************************
token types
**********************************************************/

enum ExToken
{
    //end of parsed string
    TOK_END,
        
        //names
        TOK_NAME,       // name test, incl. '*' and 'prefix:*'
        TOK_AXISNAME,   // name followed by ::
        TOK_NTNAME,     // node(), text() etc.
        TOK_FNAME,      // other names followed by (
        
        //symbols
        TOK_LPAREN,     TOK_RPAREN,
        TOK_LBRACKET,   TOK_RBRACKET,
        TOK_PERIOD,     TOK_DPERIOD,
        TOK_ATSIGN,     TOK_COMMA,
        TOK_DCOLON,     TOK_DSLASH,
        TOK_SLASH,
        
        //variable reference ($...)
        TOK_VAR,
        //string in quotes
        TOK_LITERAL,
        //real number
        TOK_NUMBER,
        
        //operators
        TOKGROUP_OPERATORS,
        TOK_OR = TOKGROUP_OPERATORS,
        TOK_AND,
        TOK_EQ,
        TOK_NEQ,
        TOK_LT,
        TOK_GT,
        TOK_LE,
        TOK_GE,
        TOK_PLUS,
        TOK_MINUS2,
        TOK_MINUS = TOK_MINUS2,
        TOK_MULT,
        TOK_MOD,
        TOK_DIV,
        TOK_MINUS1,
        TOK_VERT,

        TOK_STAR,        // as wildcard in names
        TOK_NONE
}; 

class TokenItem /* : public SabObj */
{
public:
    ExToken tok;
    char* firstc;
    int len;
    void speak(DStr &, SpeakMode mode);
};

//ExNodeType is defined in base.h
//array exNodeTypeNames in base.cpp

class Tokenizer
{
public:
    Tokenizer(Expression &owner_);
    ~Tokenizer();
	int findTop(ExToken token, int from);
    eFlag tokenize(Sit S, const Str &astring);
    eFlag getToken(Sit S, char *&, TokenItem&, ExToken);
    eFlag getDelim(Sit S, int &, Bool = FALSE);
    eFlag stripParens(Sit S, int &, int &);
    DStr string;
    PList<TokenItem*> items;
private:
    ExToken tryShort(char *&, ExToken);
    eFlag lookToken(Sit S, ExToken &, char*, ExToken);
    eFlag getToken_(Sit S, ExToken &, char*&, ExToken);
    eFlag getNumber(Sit S, char *&);
    eFlag getName(Sit S, ExToken&, char *&, ExToken);
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
    Expression &owner;
};

/*
XPath expression types. Correspondence to Sablotron classes:
NUMBER          class Number
STRING          class Str
BOOLEAN         Bool
NODESET         class Context
*/

enum ExType
{
    EX_NUMBER,
    EX_STRING,
    EX_BOOLEAN,
    EX_NODESET,
    EX_NODESET_PATTERN,
    EX_NONE,
    EX_FRAGMENT,
    EX_EXTERNAL,
    EX_UNKNOWN
    
};

/*
The following enum comprises four categories of "functors": 
(1) basic (atom, variable reference etc.) 
(2) operators (EXFO_...)
(3) functions in the XPath core library (EXFF_XPATH_...)
(4) functions in the XSLT core library (EXFF_XSLT_...)

Groups (2) through (4) should be listed in the given order (that is, operators -
XPath functions - XSLT functions) and should end with an item ..._Z 
(e.g. EXFO_Z).
*/

enum ExFunctor 
{
    EXF_ATOM,
        EXF_VAR,
        EXF_LOCPATH,
        EXF_LOCSTEP,
        EXF_OTHER_FUNC,
        EXF_FILTER,
        EXF_STRINGSEQ,
        EXF_FRAGMENT,
        EXF_NONE,
        
        // EXFO_... (operators)
        EXFO_OR,
        EXFO_AND,
        EXFO_EQ,
        EXFO_NEQ,
        EXFO_LT,
        EXFO_LE,
        EXFO_GT,
        EXFO_GE,
        EXFO_PLUS,
        EXFO_MINUS2,
        EXFO_MINUS = EXFO_MINUS2,
        EXFO_MULT,
        EXFO_DIV,
        EXFO_MOD,
        EXFO_MINUS1,
        EXFO_UNION,
        EXFO_Z,             //last in group
        
        //functions (fall into 2 groups: core XPath and core XSLT)
        EXF_FUNCTION,

        // XPath, NODESET
        EXFF_LAST,
        EXFF_POSITION,
        EXFF_COUNT,
        EXFF_ID,
        EXFF_LOCAL_NAME,
        EXFF_NAMESPACE_URI,
        EXFF_NAME,

        // XPath, STRING
        EXFF_STRING,
        EXFF_CONCAT,
        EXFF_STARTS_WITH,
        EXFF_CONTAINS,
        EXFF_SUBSTRING_BEFORE,
        EXFF_SUBSTRING_AFTER,
        EXFF_SUBSTRING,
        EXFF_STRING_LENGTH,
        EXFF_NORMALIZE_SPACE,
        EXFF_TRANSLATE,

        // XPath, BOOLEAN
        EXFF_BOOLEAN,
        EXFF_NOT,
        EXFF_TRUE,
        EXFF_FALSE,
        EXFF_LANG,

        // XPath, NUMBER
        EXFF_NUMBER,
        EXFF_SUM,
        EXFF_FLOOR,
        EXFF_CEILING,
        EXFF_ROUND,

        // XSLT core
        EXFF_DOCUMENT,
        EXFF_KEY,
        EXFF_FORMAT_NUMBER,
        EXFF_CURRENT,
        EXFF_UNPARSED_ENTITY_URI,
        EXFF_GENERATE_ID,
        EXFF_SYSTEM_PROPERTY,

    //XSLT extensions
        EXFF_FUNCTION_AVAILABLE,
        EXFF_ELEMENT_AVAILABLE,

    //GA extensions
    EXFF_EVAL,

        EXFF_NONE
};

class Expression;

/**********************************************************
N u m b e r
**********************************************************/

class Number /* : public SabObj */
{
public:
    Number();
    Number(double);
    Number& operator= (double);
    Number& operator= (const Str &);
    operator double () const;
    Bool operator== (double);
    Bool operator< (double);
    Bool operator> (double);
    Bool operator== (Number&);
    Bool operator< (Number&);
    Bool operator> (Number&);
    Bool isNaN();
    Bool isInf();
    void setNaN();
    int round();
private:
    double x;
};

/**********************************************************
External
**********************************************************/
#ifdef ENABLE_JS
class External
{
public:
  External() : priv(NULL) {};
  External(void *prv, void *val);
  External(External& other);
  ~External();
  void* getValue();
  void assign(const External& other);
private:
  void decref();
  JSExternalPrivate *priv;
};
#endif

/**********************************************************
L o c S t e p
**********************************************************/

class Element;

class LocStep /* : public SabObj */
{
    friend class Expression;
public:
    LocStep(Element &ownerV_, ExAxis = AXIS_NONE, ExNodeType = EXNODE_NONE);
    ~LocStep();
    void set(ExAxis, ExNodeType);
    eFlag parse(Sit S, Tokenizer&, int&, Bool defaultToo = FALSE);
    ExAxis ax;
    ExNodeType ntype;
    QName ntest;
    Str piname; //name for the processing-instruction() node test
    ExprList preds;
    void speak(Sit S, DStr &s, SpeakMode mode);
    Bool matchesWithoutPreds(Sit S, NodeHandle v);
    // eFlag createContextNoPreds(Context *&, int);
    eFlag shift(Sit S, NodeHandle &v, NodeHandle baseV);
    Bool positional;
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
    Element &getOwnerElement() {return ownerV;}
private:
    int badPreds;
    Element &ownerV;
};


/**********************************************************
E x p r e s s i o n
**********************************************************/

class Element;

class Expression
{
public:
    Expression(Element & owner_, ExFunctor = EXF_NONE);
    ~Expression();
    void clearContent();
    void setLS(ExAxis, ExNodeType);
    eFlag speak(Sit S, DStr &,SpeakMode);
    eFlag parse(Sit S, const DStr &, Bool = FALSE, Bool defaultToo = FALSE);
    eFlag parse(Sit S, Tokenizer&, int, int, Bool defaultToo = FALSE);
    eFlag eval(Sit S, Expression &, Context *, Bool resolvingGlobals = FALSE);
    Number tonumber(Sit S);
#ifdef ENABLE_JS
    External& toexternal(Sit S);
#endif
    eFlag tostring(Sit S, Str& result);
    const Str& tostringRef() const;
    Bool tobool();
    Context& tonodeset();
    const Context& tonodesetRef();
    eFlag matchesPattern(Sit S, Context *, Bool &result);
    eFlag trueFor(Sit S, Context *, Bool&);
    //
    ExType type;
    ExFunctor functor;
    ExprList args;
    eFlag createContext(Sit S, Context *&, int = -1);
    LocStep *step;
    int hasPath;
    void setAtom(Context*);
    void setAtom(const Number&);
    void setAtom(const DStr&);
    void setAtom(Bool);
#ifdef ENABLE_JS
    void setAtom(const External&);
#endif
    Tree* setFragment();
    Tree* pTree;

    // optimizePositional() sets usesLast and positional
    // returns: 2 if pred contains last()   ( -> usesLast && positional )
    //          1 if position() only        ( -> positional )
    //          0 if neither
    int optimizePositional(int level);

    // optimizePositionBounds() sets optimizePosition...
    void optimizePositionBounds();

    // inBounds: 
    // returns 0 if the position is within bounds, -1 if before, 1 if after
    int inBounds(int position) const;
    Element& getOwnerElement() const;
    Tree& getOwnerTree() const;
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
    Bool containsFunctor(ExFunctor func);
protected: 
private:
    union
    {
      QName *pName;    //of function or variable
      Str *patomstring;
      Bool atombool;
      Number *patomnumber;
      Context *patomnodeset;
#ifdef ENABLE_JS
      External *patomexternal;
#endif
    };
    Bool compareCC(Sit S, ExFunctor, const Context &, const Context &);
    Bool compareCS(Sit S, ExFunctor, const Context &, const Str &);
    Bool compareCN(Sit S, ExFunctor, const Context &, const Number &);
    eFlag compare(Sit S, Bool &, Expression &, ExFunctor);
    Bool isOp(ExToken, int &);
    eFlag callOp(Sit S, Expression&, ExprList&);
    eFlag callFunc(Sit S, Expression&, ExprList&, Context *);
    eFlag parseBasic(Sit S, Tokenizer &, int, int, Bool defaultToo = FALSE);
    eFlag parseLP(Sit S, Tokenizer&, int &, Bool dropRoot, Bool defaultToo = FALSE);
    eFlag matchesSinglePath(Sit S, NodeHandle v, int lastIndex, Bool &result);
    eFlag matchesSingleStep(Sit S, NodeHandle v, Bool &result);
    eFlag createLPContextLevel(Sit S, int stepLevel, int stepsCount,
        NodeHandle base, Context& info, Context *theResult);
    eFlag createLPContext(Sit S, Context *&, int, NodeHandle globalCurrent = NULL);
    eFlag createLPContextSum(Sit S, Context *&, NodeHandle globalCurrent = NULL);
    eFlag getDocument_(Sit S, NodeHandle& newroot, 
        const Str& location, const Str& baseUri, Processor *proc);

    Bool 
        isPattern;
    eFlag patternOK(Sit S);
    void setAtomNoCopy(Context &);

    // in a predicate, these give the bounds to context positions that
    // suffice to be processed
    int optimizePositionFrom,
        optimizePositionTo;

    // in a predicate, this is true iff the predicate depends on last()
    Bool usesLast,
        positional;
	
    Element &owner;
};

#endif //ifndef ExprHIncl
