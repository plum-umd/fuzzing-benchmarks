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
 * Contributor(s): Marc Lehmann <pcg@goof.com>
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

#include "expr.h"
#include "verts.h"
#include <float.h>
#include <math.h>
#include "context.h"
#include "tree.h"
#include "utf8.h"
#include "domprovider.h"
#include "guard.h"

// GP: clean.....

// period is not here as it can also start a number
char tokenShort[]=  
".. :: // != <= >= "
"(  )  [  ]  @  ,  "
"|  +  -  =  <  >  "
"/  *  "
"\x0\x0\x0";

ExToken tokenShortX[]=
{
    TOK_DPERIOD, TOK_DCOLON, TOK_DSLASH, TOK_NEQ, TOK_LE, TOK_GE,
        TOK_LPAREN, TOK_RPAREN, TOK_LBRACKET, TOK_RBRACKET, TOK_ATSIGN, TOK_COMMA,
        TOK_VERT, TOK_PLUS, TOK_MINUS, TOK_EQ, TOK_LT, TOK_GT,
        TOK_SLASH, TOK_STAR
};

/*================================================================
    function info table
================================================================*/

struct FuncInfoItem 
{
    const char *name;
    ExFunctor func;
    ExType type;
}
funcInfoTable[] =
{
    // XPath functions - NODESET category
    {"last",EXFF_LAST,EX_NUMBER},
    {"position",EXFF_POSITION,EX_NUMBER},
    {"count",EXFF_COUNT,EX_NUMBER},
    {"id",EXFF_ID,EX_NODESET},
    {"local-name",EXFF_LOCAL_NAME,EX_STRING},
    {"namespace-uri",EXFF_NAMESPACE_URI,EX_STRING},
    {"name",EXFF_NAME,EX_STRING},

    // XPath - STRING category
    {"string",EXFF_STRING,EX_STRING},
    {"concat",EXFF_CONCAT,EX_STRING},
    {"starts-with",EXFF_STARTS_WITH,EX_BOOLEAN},
    {"contains",EXFF_CONTAINS,EX_BOOLEAN},
    {"substring-before",EXFF_SUBSTRING_BEFORE,EX_STRING},
    {"substring-after",EXFF_SUBSTRING_AFTER,EX_STRING},
    {"substring",EXFF_SUBSTRING,EX_STRING},
    {"string-length",EXFF_STRING_LENGTH,EX_NUMBER},
    {"normalize-space",EXFF_NORMALIZE_SPACE,EX_STRING},
    {"translate",EXFF_TRANSLATE,EX_STRING},

    // XPath - BOOLEAN category
    {"boolean",EXFF_BOOLEAN,EX_BOOLEAN},
    {"not",EXFF_NOT,EX_BOOLEAN},
    {"true",EXFF_TRUE,EX_BOOLEAN},
    {"false",EXFF_FALSE,EX_BOOLEAN},
    {"lang",EXFF_LANG,EX_BOOLEAN},

    // XPath - NUMBER category
    {"number", EXFF_NUMBER, EX_NUMBER},
    {"sum", EXFF_SUM, EX_NUMBER},
    {"floor", EXFF_FLOOR, EX_NUMBER},
    {"ceiling", EXFF_CEILING, EX_NUMBER},
    {"round", EXFF_ROUND, EX_NUMBER},

    // XSLT core
    {"document",EXFF_DOCUMENT,EX_NODESET},
    {"key",EXFF_KEY,EX_NODESET},
    {"format-number",EXFF_FORMAT_NUMBER, EX_STRING},
    {"current",EXFF_CURRENT, EX_NODESET},
    {"unparsed-entity-uri",EXFF_UNPARSED_ENTITY_URI, EX_STRING},
    {"generate-id",EXFF_GENERATE_ID,EX_STRING},
    {"system-property",EXFF_SYSTEM_PROPERTY, EX_STRING},

    // XSLT extensions
    {"function-available",EXFF_FUNCTION_AVAILABLE,EX_BOOLEAN},
    {"element-available",EXFF_ELEMENT_AVAILABLE,EX_BOOLEAN},
    {NULL, EXFF_NONE, EX_UNKNOWN}
};

struct ExtFuncInfoItem 
{
    const char *name;
    ExFunctor func;
    ExType type;
}
extFuncInfoTable[] =
{
    {"evaluate",EXFF_EVAL,EX_NODESET},
    {NULL, EXFF_NONE, EX_UNKNOWN}
};

Str getFuncName(ExFunctor functor)
{
    return funcInfoTable[functor - EXF_FUNCTION - 1].name;
};



/**********************************************************
TokenItem
**********************************************************/

void TokenItem::speak(DStr &s, SpeakMode mode)
{
    switch(tok)
    {
    case TOK_VAR:   // remove leading $
        s.nadd(firstc + 1, len - 1);
        break;
    case TOK_LITERAL:// remove enclosing quotes or dquotes
        s.nadd(firstc + 1, len - 2);
        break;
    default:
        s.nadd(firstc,len);
    };
//    s += '\0'; - thrown out
};

/**********************************************************

  Tokenizer

**********************************************************/

Tokenizer::Tokenizer(Expression &owner_)
:
items(LIST_SIZE_EXPR_TOKENS), owner(owner_)
{
};

Tokenizer::~Tokenizer()
{
    items.freeall(FALSE);
}

eFlag Tokenizer::tokenize(Sit S, const Str &astring)
{
    char *p;
    TokenItem item;
    string = astring;
    p = (char *) string;

    E( getToken(S, p, item, TOK_NONE) );
    ExToken prevToken = item.tok;
    while ((item.tok != TOK_END) && (item.tok != TOK_NONE))
    {
        items.append(new TokenItem(item));
        E( getToken(S, p, item, prevToken) );        
        prevToken = item.tok;
    };
    
    if (item.tok == TOK_NONE)
    {
        DStr itemStr;
        item.speak(itemStr, SM_OFFICIAL);
        Err1(S, ET_BAD_TOKEN, itemStr);
    }
    else
        items.append(new TokenItem(item));

    return OK;
}

/*================================================================
namerTable
a table of tokens which have the effect that the following
name is recognized as a NCName (rather than operator name) and * is
recognized as a wildcard (rather than multiplication operator).
    The table should end with TOK_NONE (which is of this type too).
================================================================*/

static ExToken namerTable[] = {
    TOK_ATSIGN, TOK_DCOLON, TOK_LPAREN, TOK_LBRACKET,
        // operators:
    TOK_OR, TOK_AND, TOK_EQ, TOK_NEQ, TOK_LT, TOK_GT, TOK_LE, TOK_GE,
    TOK_PLUS, TOK_MINUS, TOK_MINUS1, TOK_MULT, TOK_DIV, TOK_MOD, TOK_VERT,
        // slashes are operators too but not for us
    TOK_SLASH, TOK_DSLASH, TOK_COMMA,
        // TOK_NONE (terminator)
    TOK_NONE};

/*================================================================
Bool isNamer()
returns True iff the token is contained in the namerTable table.
================================================================*/

static Bool isNamer(ExToken tok)
{
    int i;
    if (tok == TOK_NONE) return TRUE;
    for (i = 0; (namerTable[i] != tok) && 
        (namerTable[i] != TOK_NONE); i++);
    return (Bool) (namerTable[i] == tok);
}

/*================================================================
ExToken tryShort()
looks up a few characters at p in the tokenShort table containing
the (up-to-3-char) symbols
RETURNS the token identified, or TOK_NONE if no match is found
================================================================*/

ExToken Tokenizer::tryShort(char*& p, ExToken prevToken)
{
    int i;
    char* t;
    ExToken result;
    
    for (i=0, t=tokenShort; *t; i++,t+=3)
        if (*p==*t)
        if ((t[1] == ' ') || (t[1] == p[1])) break;
    if (*t)
    {
        p += ((t[1] == ' ')? 1 : 2);
        result = tokenShortX[i];
        if (result == TOK_STAR)
            result = (isNamer(prevToken)? TOK_NAME : TOK_MULT);
        if ((result == TOK_MINUS) && isNamer(prevToken))
            result = TOK_MINUS1;
    }
    else result = TOK_NONE;
    return result;
}

/*================================================================
eFlag lookToken()
sets 'ret' to the following token, but does not change the pointer p
================================================================*/

eFlag Tokenizer::lookToken(Sit S, ExToken &ret, char* p, ExToken prevToken)
{
    // getToken_() changes p but this is passed by value so
    // remains unchanged

    E( getToken_(S, ret, p, prevToken) );
    return OK;
}

/*================================================================
Bool findChar
sets p to address FOLLOWING the next occurence of c in p
(to skip a character reference, use findChar(p,';'))
RETURNS false iff the next occurence was not found
================================================================*/

static Bool findChar(char*& p, char c)
{
    while (*p && (*p != c)) p++;
    if (*p)
    {
        p++;
        return TRUE;
    }
    else
        return FALSE;
}

/*================================================================
findSame
sets p to address FOLLOWING the next occurence of *p in p
RETURNS false iff another occurence was not found
================================================================*/

static Bool findSame(char*& p)
{
    char first = *p++;
    return findChar(p, first);
};

eFlag Tokenizer::getToken_(Sit S, ExToken &ret, char*& p, ExToken prevToken)
{
    ExToken tok;
    char c;
    
    skipWhite(p);
    if (!*p)
    {
        ret = TOK_END; 
        return OK;
    }
    else
    {
        // the following may also translate * into TOK_NAME
        if ((tok = tryShort(p, prevToken)) != TOK_NONE)
        {
            ret = tok;
            return OK;
        };
        switch (c = *p)
        {
        case '$': 
            {
                // call getName with prevToken=TOK_NONE
                // to ensure that e.g. 'and' in '$and' is treated as a name
                E( getName(S, ret, ++p, TOK_NONE) );
                if (ret != TOK_NONE)
                    ret = TOK_VAR;
                else 
                    Err(S, ET_BAD_VAR);
            };
            break;
        case '\"':
        case '\'': 
            if(!findSame(p))
                Err(S, ET_INFINITE_LITERAL)
            else
                ret = TOK_LITERAL;
            break;
        case '&': sabassert(0);  //DBG: do not process entity references so far
            break;
        case '.':
            if (utf8IsDigit(utf8CharCode(p+1)))
            {
                E( getNumber(S, p) );
                ret = TOK_NUMBER;
            }
            else {
                p++; 
                ret = TOK_PERIOD;
            };
            break;
        default:
            {
                if (utf8IsDigit(utf8CharCode(p)))
                {
                    E( getNumber(S, p) );
                    ret = TOK_NUMBER;
                }
                else
                {
                    if (utf8IsLetter(utf8CharCode(p)) || (*p == '_') || (*p == ':'))
                    {
                        // the following call finds TOK_NAME, TOK_FNAME,
                        // TOK_AXISNAME,
                        // as well as TOK_AND etc. (based on prev token)
                        E( getName(S, ret, p, prevToken) ); 
                    }
                    else 
                    {
                        Str temp;
                        temp.nset(p, 1);
                        Err1(S, ET_BAD_TOKEN, temp); //unknown token
                    }
                }
            };  //default
        };      //switch
    };          //else
    return OK;
};              //getToken_

eFlag Tokenizer::getToken(Sit S, char*& p, TokenItem& item, ExToken prevToken)
{
    ExToken t;
    skipWhite(p);
    item.firstc = p;
    E( getToken_(S, t, p, prevToken) );
    item.len = (long)(p - item.firstc);
    item.tok = t;
    return OK;
}

eFlag Tokenizer::getNumber(Sit S, char*& p)
{
    Bool wasDot = FALSE;
    while ((*p) && (utf8IsDigit(utf8CharCode(p))) || (*p == '.'))
    {
        if (*p == '.')
            if (wasDot)
            Err(S, ET_BAD_NUMBER)
            else wasDot = TRUE;
        p += utf8SingleCharLength(p);
    };
    return OK;
};

/*================================================================
getWordOp
checks whether the sequence at p of given length is an operator name
RETURNS the appropriate token if so; TOK_NONE otherwise
================================================================*/

static ExToken getWordOp(char *p, int length)
{
    if (length > 3) return TOK_NONE;
    if (length < 2) length = 2;
    if (!strncmp(p,"or",length)) return TOK_OR;
    if (length < 3) length = 3;
    if (!strncmp(p,"and",length)) return TOK_AND;
    if (!strncmp(p,"div",length)) return TOK_DIV;
    if (!strncmp(p,"mod",length)) return TOK_MOD;
    return TOK_NONE;
}

static Bool isNodeTest(char *p, int length)
{
    const char *q;
    int qlen;
    for (int i = 0; (q = exNodeTypeNames[i]) != NULL; i++)
    {
        if (!strncmp(q,p,
            (length < (qlen = strlen(q))? qlen : length)))
            break;
    };
    return (Bool)(q != NULL);
}

#define nameCharExtended(CH, PTR) ((CH = utf8CharCode(PTR))!= 0) &&\
        (utf8IsNameChar(CH) || strchr(".-_:*",CH))

int nameLength(char* from)
{
    char *q = from;
    int length = 0;
    unsigned long c;
    while(nameCharExtended(c,q)) {
       q += utf8SingleCharLength(q);
       length++;
    }
    return length;
}

eFlag Tokenizer::getName(Sit S, ExToken &ret, char*& p, ExToken prevToken)
{
    char *former = p;
    unsigned long c;
    BOOL wasColon = FALSE;
    
    if ((!utf8IsLetter(utf8CharCode(p))) && (*p != '_'))
    {
        ret = TOK_NONE;
        return OK;
    }

    while (nameCharExtended(c,p))
    {
        if (c == ':')
        {
            if (wasColon)
            {
                // identify the bad qname;
                Str theName;
                theName.nset(former, nameLength(former));
                Err1(S, E1_EXTRA_COLON, theName);
            }
            else
            {
                switch(*(p+1))
                {
                case ':':
                    {
                        ret = TOK_AXISNAME;
                        return OK;
                    };
                case '*':
                    {
                        ret = TOK_NAME;
                        p += 2;
                        return OK;
                    };
                default:
                    wasColon = TRUE;
                };
            };
        }
        else if (c == '*')
        {
            if ((p - former) && *(p - 1) != ':')   // the first condition could be dropped
            {
                ret = TOK_NAME;
                return OK;
            }
        }
        p += utf8SingleCharLength (p);
    }

    if (!wasColon && !isNamer(prevToken))
    {
        if ((ret = getWordOp(former, (int) (p - former))) != TOK_NONE)
            return OK;
    };

    ExToken next;

    // look at following token with prev=TOK_NAME
    E( lookToken(S, next,p,TOK_NAME) );
    switch(next)
    {
    case TOK_DCOLON:
        ret = TOK_AXISNAME;
        break;
    case TOK_LPAREN:
        {
            if (isNodeTest(former, (int) (p - former)))
                ret = TOK_NTNAME;
            else
                ret = TOK_FNAME;
        }; break;
    default:
        ret = TOK_NAME;
    };
    return OK;
};

/*================================================================
int findTop()
    finds the first top-level occurence of 'token' starting with
    position 'from'. If there is none, return value points at TOK_END.
================================================================*/

int Tokenizer::findTop(ExToken token, int from)
{
    int level = 0;
    ExToken ct;
    int i;
    for (i = from; 
        ((ct = items[i] -> tok) != TOK_END) && (level || (ct != token));
        i++)
        {
            if ((ct == TOK_LPAREN) || (ct == TOK_LBRACKET))
                level++;
            if ((ct == TOK_RPAREN) || (ct == TOK_RBRACKET))
                level--;
        }
    return i;
}


/*================================================================
eFlag getDelim()
given a position in 'pos', finds the corresponding next token.
If the left token is ( or [, looks for the matching right paren/bracket,
Otherwise looks for the occurence of the same token. 
Returns pos pointing at the matching token, or at TOK_END if there 
is none. (In case of failed reverse search, returns -1.)
================================================================*/

eFlag Tokenizer::getDelim(Sit S, int &pos, Bool reverse /*=FALSE*/)
{
    ExToken first, second, tok;
    int level = 0,
        i = pos;
    
    switch(first = items[pos] -> tok)
    {
    case TOK_LBRACKET: 
        second = TOK_RBRACKET; 
        break;
    case TOK_LPAREN: 
        second = TOK_RPAREN; 
        break;
    case TOK_RBRACKET: 
        second = TOK_LBRACKET; 
        break;
    case TOK_RPAREN: 
        second = TOK_LPAREN; 
        break;
    default: 
        second = first;
    }
    
    i += reverse? -1 : 1;
    
    while ((i >= 0) && ((tok = items[i] -> tok) != TOK_END))
    {
        if (tok == second)
        {
            if (!level)
            {
                pos = i;
                return OK;
            }
            else level--;
        }
        else if (tok == first) level++;
        i += reverse? -1 : 1;
    }
    pos = i;
    return OK;
}

/*================================================================
stripParens()
given the left and right end positions of a tokenizer fragment, 
shifts them inwards to strip any outer parentheses.
================================================================*/

eFlag Tokenizer::stripParens(Sit S, int &left, int &right)
{
    int left0 = left;
    if (items[right]->tok == TOK_END)
        right--;
//    sabassert(left <= right);
    while ((items[left]->tok == TOK_LPAREN)
        && (items[right]->tok == TOK_RPAREN))
    {
        left0 = left;
        E( getDelim(S, left0) );
        if (left0 == right)
        {
            left++; 
            right--;
        }
        else break;
    };
    return OK;
}

void Tokenizer::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    owner.report(S, type, code, arg1, arg2);
}


/*****************************************************************
*                                                                *
      L o c S t e p 
*                                                                *
*****************************************************************/

LocStep::LocStep(Element& ownerV_, ExAxis _axis       /*=AXIS_NONE*/, 
                 ExNodeType _ntype  /*=EXNODE_NONE*/)
: preds(LIST_SIZE_1), ownerV(ownerV_)
{
    set(_axis, _ntype);
    positional = FALSE;
    badPreds = 0;
}

LocStep::~LocStep()
{
    preds.freeall(FALSE);
}

void LocStep::set(ExAxis _axis, ExNodeType _ntype)
{
    ax = _axis;
    ntype = _ntype;
}

void LocStep::speak(Sit S, DStr &strg, SpeakMode mode)
{
    if (!(mode & SM_CONTENTS)) return;
    switch(ax)
    {
    case AXIS_CHILD:
//    case AXIS_DESC_OR_SELF:
    case AXIS_ROOT:
        break;
    case AXIS_ATTRIBUTE:
        strg += '@';
        break;
    default:
        {
            strg += axisNames[ax];
            strg += "::";
        };
    };
    if((ntype != EXNODE_NONE) //&& (ax != AXIS_DESC_OR_SELF)
        && (ax != AXIS_ROOT))
    {
        strg += exNodeTypeNames[ntype];
        strg += "()";
    }
    else
    {
	    Str fullName;
	    getOwnerElement().getOwner().expandQStr(ntest,fullName);
        // ntest.speak(S, strg,mode);
	    strg += fullName;
	}
    int i, predsNumber = preds.number();
    for (i = 0; i < predsNumber; i++)
    {
        strg += '[';
        preds[i] -> speak(S, strg,mode);
        strg += ']';
    };
};

eFlag LocStep::parse(Sit S, Tokenizer& tokens, int& pos,
		     Bool defaultToo /* = FALSE */)
{
    int right;
    int &i = pos;
    DStr temp;
    ExToken tok;
    
    tok = tokens.items[i++]->tok;
    if (tok == TOK_END)
        Err(S, ET_EMPTY_PATT);
    switch (tok)
    {
    case TOK_PERIOD:
        ax = AXIS_SELF;
        ntype = EXNODE_NODE;
        return OK;
        break;
    case TOK_DPERIOD:
        ax = AXIS_PARENT;
        ntype = EXNODE_NODE;
        return OK;
        break;
    case TOK_ATSIGN:
        {
            ax = AXIS_ATTRIBUTE;
            tok = tokens.items[i++]->tok;
        };
        break;
    case TOK_STAR:
        {
            ax = AXIS_CHILD;
        };
        break;
    case TOK_AXISNAME:
        {
            tokens.items[i-1] -> speak(temp, SM_OFFICIAL);
            if ((ax = (ExAxis) lookup(temp, axisNames)) == AXIS_NONE)
                Err1(S, E1_UNKNOWN_AXIS, temp);
            i++;
            tok = tokens.items[i++] -> tok;
        };
        break;
    case TOK_NAME:
    case TOK_NTNAME:
        {
	  //	  if (!defaultAx)
	  //  Err(S, ET_EXPR_SYNTAX);
	  ax = AXIS_CHILD;
        };
        break;
    default:
        Err(S, ET_EXPR_SYNTAX); // axis name or node-test expected
    };
    
    // axis has been determined; tok must now be a name or a node-test
    temp.empty();
    if ((tok != TOK_NAME) && (tok != TOK_NTNAME))
        Err(S, ET_EXPR_SYNTAX); // axis name or node-test expected
    tokens.items[i-1] -> speak(temp,SM_OFFICIAL);
    ntype = EXNODE_NONE;

    if (tok == TOK_NTNAME)
    {
        ntype = (ExNodeType) lookup(temp, exNodeTypeNames);
        if (tokens.items[i++]->tok != TOK_LPAREN)
            Err(S, ET_LPAREN_EXP);
	
	//pi test may hav one literal param
	if (ntype == EXNODE_PI && tokens.items[i]->tok == TOK_LITERAL) {
	  DStr pilit;
	  tokens.items[i++]->speak(pilit, SM_CONTENTS);
	  piname = pilit;
	}

        if (tokens.items[i++]->tok != TOK_RPAREN)
	  {
            Err(S, ET_RPAREN_EXP);
	  }
    }
    else
    {
        // set the QName from prefix:uri using
        // the namespace declarations in 'ownerE'
        // (passed through from the containing attribute)
        // DEFAULT: without using the default namespace
        E( getOwnerElement().setLogical(S, ntest, temp, defaultToo) ); 
	}

    while ((tokens.items[i] -> tok) == TOK_LBRACKET)
    {
        badPreds = 0;
        E( tokens.getDelim(S, right = i) );
        if (tokens.items[right] -> tok == TOK_END)
            Err(S, ET_RBRACKET_EXP)
        else 
        {
            GP( Expression ) ex = new Expression(getOwnerElement());
            E( (*ex).parse(S, tokens,i+1,right-1) );
            // find out about the use of last() and position()
            int exPositionalType = (*ex).optimizePositional(0);
            if (exPositionalType)
            {
                positional = TRUE;
                if (exPositionalType == 2)
                    badPreds++;
                // find sufficient position bounds
                (*ex).optimizePositionBounds();
            }
            preds.append(ex.keep());
        };
        i = right + 1;
    };

    return OK;  // pointing at the first char that does not start a predicate

};      //  end LocStep::parse()


Bool LocStep::matchesWithoutPreds(Sit S, NodeHandle v)
{
    // removed the following:
    // sabassert(v);
    // (because the parent-of-root calls etc.)
    if (nhNull(v))
        return FALSE;
    // ANDed with VT_BASE for ordinary vertices
    SXP_NodeType ty = S.dom().getNodeType(v); 
    switch(ntype)
    {
    case EXNODE_NODE:
        break;
    case EXNODE_TEXT:
        if (ty != TEXT_NODE)
            return FALSE;
        break;
    case EXNODE_PI:
        if (ty != PROCESSING_INSTRUCTION_NODE) 
            return FALSE;
        break;
    case EXNODE_COMMENT:
        if (ty != COMMENT_NODE)
            return FALSE;
        break;
    case EXNODE_NONE:
        if ((ty == TEXT_NODE) || (ty == COMMENT_NODE) || 
	    (ty == DOCUMENT_NODE) || (ty == PROCESSING_INSTRUCTION_NODE))
	  return FALSE;
        break;
    };

    switch(ax)
    {
    case AXIS_ROOT: 
        return (Bool) (ty == DOCUMENT_NODE); 
        break;
    case AXIS_ATTRIBUTE: 
        if (ty != ATTRIBUTE_NODE) return FALSE; 
        break;
    case AXIS_NAMESPACE: 
        if (ty != NAMESPACE_NODE) return FALSE; 
        break;
    case AXIS_CHILD: 
    case AXIS_DESCENDANT:
    case AXIS_DESC_OR_SELF:
    case AXIS_ANCESTOR:
    case AXIS_ANC_OR_SELF:
    case AXIS_FOLL_SIBLING:
    case AXIS_PREC_SIBLING:
    case AXIS_FOLLOWING:
    case AXIS_PRECEDING:
        switch (ty)
        {
        case ATTRIBUTE_NODE:
        case NAMESPACE_NODE:
            return FALSE;
        case DOCUMENT_NODE:
            switch(ax)
            {
            case AXIS_DESC_OR_SELF:
            case AXIS_ANCESTOR:
            case AXIS_ANC_OR_SELF:
                break;
            default:
                return FALSE;
            };
        };
        break;
    case AXIS_SELF:
      {
	if (ntype == EXNODE_NONE && ty != ELEMENT_NODE) return false;
      }; break;
    case AXIS_PARENT:
        break;
    default: sabassert(0); //should be handled in parse
    };

    //if (ntype != EXNODE_NONE)
    //    return TRUE;
    switch (ntype) 
      {
      case EXNODE_NONE:
	break;
      case EXNODE_PI:
	{
	  if (! S.domExternal(v) && ! (piname == ""))
	    {
	      ProcInstr *pi = toPI(v);
	      EQName ename;
	      pi->getOwner().expandQ(pi->name, ename);
	      return ename.getLocal() == piname;
	    }
	  else
	    return TRUE;
	};
      default:
	return TRUE;
      }

    //exnode_none are processed here
    if (S.domExternal(v))
      {
	const char *uri = S.dom().getNodeNameURI(v);
	const char *local = S.dom().getNodeNameLocal(v);
        Bool ret =  getOwnerElement().getOwner()
	  .cmpQNameStrings(ntest, uri, local);
	S.dom().freeName(v, (char*)uri);
	S.dom().freeName(v, (char*)local);
	return ret;
      }
    else
      {
        const QName &hisname = toV(v) -> getName();
        return getOwnerElement().getOwner()
	  .cmpQNamesForeign(ntest, toV(v) -> dict(), hisname);
      }
}


eFlag LocStep::shift(Sit S, NodeHandle &v, NodeHandle baseV)
{
    NodeHandle w = NULL;       // the result
    switch(ax)
    {
    case AXIS_ATTRIBUTE:
    {
	sabassert(S.domExternal(v) || nhNull(v) || 
	       isElement(baseV) && baseV == toV(v) -> parent);
	// i.e. v==NULL and baseV isn't daddy
	if (S.dom().getNodeType(baseV) != ELEMENT_NODE) break; 
	for (w = (!nhNull(v) ? 
		  S.dom().getNextAttrNS(v) : 
		  S.dom().getAttributeNo(baseV, 0)); 
	     !nhNull(w) && !matchesWithoutPreds(S, w);
	     w = S.dom().getNextAttrNS(w));
    }; break;    
		    
    case AXIS_CHILD:
    {
	sabassert(S.domExternal(v) || nhNull(v) || 
	       isDaddy(baseV) && baseV == toV(v) -> parent);
	if (S.dom().getNodeType(baseV) != ELEMENT_NODE &&
	    S.dom().getNodeType(baseV) != DOCUMENT_NODE) break; 
	for (w = !nhNull(v) ?
	       S.dom().getNextSibling(v) : 
	       S.dom().getFirstChild(baseV);
	     !nhNull(w) && !matchesWithoutPreds(S, w);
	     w = S.dom().getNextSibling(w));
    }; break;
        
    case AXIS_NAMESPACE:
    {
	sabassert(S.domExternal(v) || nhNull(v) || 
	       isElement(baseV) && baseV == toV(v) -> parent);
	if (S.dom().getNodeType(baseV) != ELEMENT_NODE) break;
	for (w = !nhNull(v) ? 
	       S.dom().getNextAttrNS(v) : 
	       S.dom().getNamespaceNo(baseV, 0); 
	     !nhNull(w) && !matchesWithoutPreds(S, w);
	     w = S.dom().getNextAttrNS(w));
    }; break;
        
    case AXIS_ROOT:     // technically not an axis
    {
	if (nhNull(v))
	{
	    w = S.dom().getOwnerDocument(baseV);
	    if (nhNull(w)) // w must have been the root itself
		w = baseV;
	}
    }; break;
        
    case AXIS_SELF:
    {
	if (nhNull(v) && matchesWithoutPreds(S, baseV))
	    w = baseV;
    }; break;
        
    case AXIS_PARENT:
    {
	if (nhNull(v) && matchesWithoutPreds(S, S.dom().getParent(baseV)))
	    w = S.dom().getParent(baseV);
    }; break;
        
    case AXIS_ANCESTOR:
    case AXIS_ANC_OR_SELF:
    {
	if (!nhNull(v)) 
	    w = S.dom().getParent(v);
	else
	    w = (ax == AXIS_ANCESTOR) ? S.dom().getParent(baseV) : baseV;
	for (; !nhNull(w) && !matchesWithoutPreds(S, w); 
	       w = S.dom().getParent(w));
    }; break;
        
    case AXIS_FOLL_SIBLING:
    {
	switch(S.dom().getNodeType(baseV))
	{
	case DOCUMENT_NODE:
	case NAMESPACE_NODE:
	case ATTRIBUTE_NODE:
	    break;
	default:
	    for (w = S.dom().getNextSibling(!nhNull(v) ? v : baseV);
		 !nhNull(w) && !matchesWithoutPreds(S, w);
		 w = S.dom().getNextSibling(w));				
	}
    }; break;
        
    case AXIS_PREC_SIBLING:
    {
	switch(S.dom().getNodeType(baseV))
	{
	case DOCUMENT_NODE:
	case NAMESPACE_NODE:
	case ATTRIBUTE_NODE:
	    break;
	default:
	    for (w = S.dom().getPreviousSibling(!nhNull(v) ? v : baseV);
		 !nhNull(w) && !matchesWithoutPreds(S, w);
		 w = S.dom().getPreviousSibling(w));				
	}
    }; break;
        
    case AXIS_DESCENDANT:
    case AXIS_DESC_OR_SELF:
    {
	if (nhNull(v))
	{
	    if (ax == AXIS_DESC_OR_SELF && matchesWithoutPreds(S, baseV))
	    {
		w = baseV;
		break;
	    }
	    else
		v = baseV;
	}
            
	// find next descendant
	do
	{
	    if ((S.dom().getNodeType(v) == ELEMENT_NODE ||
		 (S.dom().getNodeType(v) == DOCUMENT_NODE)) &&
		S.dom().getChildCount(v))
	      v = S.dom().getFirstChild(v);
	    else
	    {
		while (v != baseV)
		{
		    SXP_Node v0 = v;
		    v = S.dom().getNextSibling(v);
		    if (!nhNull(v))
			break;
		    else
			v = S.dom().getParent(v0);
		}
	    };
	    if (v != baseV && matchesWithoutPreds(S, v))
	    {
		w = v;
		break;
	    };
	}
	while(v != baseV);
    }; break;
    case AXIS_FOLLOWING:
    {
      do {
	if (!nhNull(v) && S.dom().getChildCount(v))
	  w = S.dom().getChildNo(v,0);
	else
	  {
	    if (nhNull(v)) 
	      v = baseV;
	    while (!nhNull(v) && nhNull(S.dom().getNextSibling(v)))
	      v = S.dom().getParent(v);
	    if (!nhNull(v))
	      w = S.dom().getNextSibling(v);
	    else
	      w = NULL;
	  }
	v = w;
      } while (!nhNull(w) && !matchesWithoutPreds(S, w));
    }; break;
    case AXIS_PRECEDING:
    {
      do {
	v = !nhNull(v) ? v : baseV;
	w = S.dom().getPreviousSibling(v);
	if (!nhNull(w)) {
	  int childCount;
	  while (0 != (childCount = S.dom().getChildCount(w)))
	    w = S.dom().getChildNo(w,childCount - 1);
	} else {
	  w = S.dom().getParent(v);
	  //eliminate ancestors of baseV
	  for (v = S.dom().getParent(baseV); 
	       !nhNull(v) && v != w; 
	       v = S.dom().getParent(v));
	  //we're in the ancestor line
	  if (!nhNull(v)) {
	    while (!nhNull(v) && !(S.dom().getPreviousSibling(v)))
	      v = S.dom().getParent(v);
	    if (!nhNull(v)) 
	      w = S.dom().getPreviousSibling(v);
	    else
	      w = NULL;
	  }
	}
	v = w;
      } while (!nhNull(w) && !matchesWithoutPreds(S, w));
    }; break;
    default:
        sabassert(0); 
    };
    v = w;
    return OK;
};

void LocStep::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    ownerV.report(S, type, code, arg1, arg2);
}



/**********************************************************
N u m b e r
**********************************************************/

Number::Number()
{
    *this = 0.0;
}

Number::Number(double y)
{
    *this = y;
}

Number& 
Number::operator= (double y)
{
    x = y;
    return *this;
};

Number& 
Number::operator= (const Str &s)
{
    char *endptr, *startptr = s;
    skipWhite(startptr);
    if (*startptr)
    {
        x = strtod(startptr, &endptr);
        if (endptr) {
	  skipWhite(endptr);
	  if (*endptr)
            setNaN();
	}
    }
    else
        setNaN();
    return *this;
};

Number::operator double() const
{
    return x;
}

Bool Number::operator== (double y)
{
    if (isNaN() || isnan__(y))
        return FALSE;
    if (isInf() || isinf__(y))
        return isInf() && isinf__(y) && !((x > 0) ^ (y > 0));
    double d = x - y;
    return (Bool)((d < EPS) && (d > -EPS));
}

Bool Number::operator== (Number& y)
{
    if (isNaN()) return FALSE;
    return (Bool)(operator== ((double) y));
}


Bool Number::operator< (double y) 
{
    return (Bool)(x < y);
}

Bool Number::operator< (Number& y)
{
    return (Bool)(x < (double) y);
}

Bool Number::operator> (double y)
{
    return (Bool)(x > y);
}

Bool Number::operator> (Number& y)
{
    return (Bool)(x > (double) y);
}

Bool Number::isNaN()
{
    return isnan__(x);
}

Bool Number::isInf()
{
    return isinf__(x);
};

void Number::setNaN()
{
    // divide by zero using a variable, to fool too clever compilers
    // 'volatile' suggested by Dirk Siebnich
    volatile int zero = 0;
    x = 0.0 / zero;
}

int Number::round()
{
    if (isNaN() || isInf())
        return 0;
    else return (int)(floor(x + 0.5)); // FIXME: ignoring the 'negative zero'
}


//________________________________________________________________

/*****************************************************************
|                                                                |
    JS - External
|                                                                |
*****************************************************************/

#ifdef ENABLE_JS
External::External(void *prv, void *val) 
{
  priv = new JSExternalPrivate(prv, val);
}

External::External(External& other)
  : priv(NULL)
{
  assign(other);
}


External::~External() 
{ 
  decref(); 
}

void* External::getValue() 
{ 
  return priv->value; 
};

void External::assign(const External& other)
{
  if (priv) decref(); 
  priv = other.priv; priv->refcnt++;
}

void External::decref() 
{
  if (priv) 
    {
      priv->refcnt--; 
      if (!priv->refcnt) 
	{ 
	  delete priv; 
	  priv = NULL;
	}
    }
}
#endif

/*****************************************************************
|                                                                |
    E x p r e s s i o n
|                                                                |
*****************************************************************/

Expression::Expression(Element &owner_,
                       ExFunctor _functor   /* = EXF_NONE   */
                       )
: args(1), owner(owner_)
{
    functor = _functor;
    switch(functor)
    {
    case EXF_LOCSTEP:
        {
            step = new LocStep(owner_);
            type = EX_NODESET;
        }; break;
    case EXF_LOCPATH:
        {
            type = EX_NODESET;
        }; break;
    case EXF_STRINGSEQ:
        type = EX_STRING;
        break;
    default:
        {
            type = EX_UNKNOWN;
        }; break;
    };
    hasPath = FALSE;
    isPattern = FALSE;
    pTree = NULL;
    // the following sets patomnodeset, note that e.g. patomnumber
    // is with it in a union
    patomnodeset = NULL;
    usesLast = FALSE;
    positional = FALSE;
    optimizePositionFrom = optimizePositionTo = 0; // see header
}

void Expression::setLS(ExAxis _axis, ExNodeType _ntype)
{
    sabassert(functor == EXF_LOCPATH);
    Expression *ls = new Expression(getOwnerElement(), EXF_LOCSTEP); // GP: OK
    args.append(ls);
    ls -> step -> set(_axis, _ntype);    
}

//
//  Expression destructor
//  
Expression::~Expression()
{
    clearContent();
}

#define deleteZ( PTR )    { cdelete( PTR ); } 

//
//  clearContent()
//  called when setting the expression to a new value
//  to dispose of any existing contents.
//
void Expression::clearContent()
{
    args.freeall(FALSE);
    switch(functor)
    {
    case EXF_ATOM:
        {
            switch(type)
            {
            case EX_NODESET:
                deleteZ ( patomnodeset );
                break;
            case EX_STRING:
                deleteZ ( patomstring );
                break;
            case EX_NUMBER:
                deleteZ ( patomnumber );
                break;
#ifdef ENABLE_JS
	    case EX_EXTERNAL:
	        deleteZ( patomexternal);
	        break;
#endif
            };
        }; break;
    case EXF_LOCSTEP:
        deleteZ ( step );
        break;
    case EXF_VAR:
    case EXF_OTHER_FUNC:
        deleteZ ( pName );
        break;
    }
    cdelete( pTree );
};



/*================================================================
speak
    writes the expression to string 'strg'. Formatting is specified
    by 'mode'.
================================================================*/

eFlag Expression::speak(Sit S, DStr &strg, SpeakMode mode)
{
    int i, argsNumber = args.number();
    switch(functor)
    {
    case EXF_ATOM:
        {
	        Str temp;
		    E( tostring(S, temp) );
            strg += temp;
        }; break;
    case EXF_LOCSTEP:
        {
            step -> speak(S, strg, mode);
        }; break;
    case EXF_LOCPATH:
        {
            for(i = 0; i < argsNumber; i++)
            {
                args[i] -> speak(S, strg, mode);
                if (i < argsNumber-1) 
                    strg += "/";
                else if ((argsNumber == 1) && 
                    (args[0] -> step -> ax == AXIS_ROOT))
                    strg += "/";
            }
        }; break;
    default:
        {
            strg += (DStr("\nfunctor ") + (int) functor + "\n--------ARGS:\n");
            for (i = 0; i < argsNumber; i++)
            {
                strg += DStr("(") + (i+1) + ")   ";
                args[i] -> speak(S, strg, mode);
                strg += "\n";
            };
            strg += "--------ARGS end\n";
        }
    };
    return OK;
}

/*================================================================
matches
    returns TRUE iff the current vertex of c satisfies the
    Expression's condition.
    PROBLEM:
    perhaps this should also return an error in case the expression is
    not of type nodeset?
================================================================*/

eFlag Expression::matchesPattern(Sit S, Context *c, Bool &result)
{
    sabassert(type == EX_NODESET);
    if (functor == EXF_LOCPATH)
    {
        E(matchesSinglePath(S, c -> current(), args.number() - 1, result));
        return OK;
    }
    if (functor == EXFO_UNION)
    {
        int j, argsNumber = args.number();
        for (j = 0; j < argsNumber; j++)
        {
            E( args[j] -> matchesPattern(S, c, result) );
            if (result)
                RetOK( result, TRUE );
        };
    }
    RetOK(result, FALSE);
}

eFlag Expression::trueFor(Sit S, Context *c, Bool& result)
{
    Expression ex(getOwnerElement());
    E( eval(S, ex, c) );
    switch(ex.type)
    {
    case EX_NUMBER:
        result = (Bool) (ex.tonumber(S) == (double) (c -> getPosition() + 1));
        break;
    default:
        result = ex.tobool();
    }
    return OK;
}


Bool Expression::tobool()
{
    sabassert(functor == EXF_ATOM);
    switch(type)
    {
    case EX_NUMBER:
        return (Bool) !(*patomnumber == 0.0 || patomnumber -> isNaN());
        break;
    case EX_STRING:
        return (Bool) !(patomstring -> isEmpty());
        break;
    case EX_BOOLEAN:
        return atombool;
        break;
    case EX_NODESET:
        return (Bool) !!(patomnodeset -> getSize());
        break;
    default: sabassert(0);
    };
    return FALSE;   //just to return something
}

eFlag Expression::tostring(Sit S, Str& strg)
{
    sabassert(functor == EXF_ATOM);
    switch(type)
    {
    case EX_NUMBER:
        if (patomnumber -> isNaN())
            strg = (char*)"NaN";
        else
        {
            if (!patomnumber -> isInf())
                strg = (double)(*patomnumber);
            else if (*patomnumber > 0.0)
                strg = (char*)"+Infinity";
            else strg = (char*)"-Infinity";
        }
        break;
    case EX_STRING:
        strg = *patomstring;
        break;
    case EX_BOOLEAN:
        strg = (atombool ? (char *)"true" : (char *)"false");
        break;
    case EX_NODESET:
        if (!patomnodeset -> getSize())
            strg = (char*)"";
        else 
        {
            DStr temp;
            S.dom().constructStringValue(patomnodeset -> current(), temp);
            strg = temp;
        }
        break;
    case EX_EXTERNAL:
      {
	strg = (char*)"[External Object]";
      }; break;
    default: sabassert(0);
    };
    return OK;
}

const Str& Expression::tostringRef() const
{
    sabassert((functor == EXF_ATOM) && (type == EX_STRING));
    return (*NZ(patomstring));
}

Number Expression::tonumber(Sit S)
{
    sabassert(functor == EXF_ATOM);
    Number n;
    switch(type)
    {
    case EX_NUMBER:
        n = *patomnumber;
        break;
    case EX_STRING:
        n = *patomstring;
        break; 
    case EX_BOOLEAN:
        n = (atombool ? 1.0 : 0.0);
        break;
    case EX_NODESET:
      {
            // to avoid the following, tostring() must return const Str&:
            Str strg;
	        tostring(S, strg);
            n = strg;
	    // but note that changing it to n = tostringRef() failed
      };
        break;
    default: sabassert(0);
    };
    return n;
}

#ifdef ENABLE_JS
External& Expression::toexternal(Sit S)
{
  sabassert((functor == EXF_ATOM) && (type == EX_EXTERNAL));
  return *patomexternal;
  /*
    sabassert(functor == EXF_ATOM);
    //External e;
    switch(type)
    {
    case EX_EXTERNAL:
      {
	e.assign(*patomexternal);
	//e.setValue(patomexternal -> getValue());
      }; break;
    default: sabassert(0);
    };
    //return e;
    return NULL;
  */
}
#endif

Context& Expression::tonodeset()
{
    sabassert((functor == EXF_ATOM) && (type == EX_NODESET));
    return *(patomnodeset -> copy());
}

const Context& Expression::tonodesetRef()
{
    sabassert((functor == EXF_ATOM) && (type == EX_NODESET));
    return *patomnodeset;
}

eFlag Expression::patternOK(Sit S)
{
    int i,
        argsNumber = args.number();
    // sabassert(functor == EXFO_UNION || functor == EXF_LOCPATH);

    if ( containsFunctor(EXFF_CURRENT) )
      Err(S, E_BAD_PATTERN);

    switch(functor)
    {
    case EXF_LOCPATH:
        {
            for (i = 0; i < argsNumber; i++)
            {
                LocStep *ls = args[i] -> step;
                switch (ls -> ax)
                {
                case AXIS_CHILD:
                case AXIS_ATTRIBUTE:
                case AXIS_ROOT:
                    break;
                case AXIS_DESC_OR_SELF:
                    if (ls -> ntype != EXNODE_NODE)
                        Err(S, E_BAD_PATTERN);
                    break;
                default:
                    Err(S, E_BAD_AXIS_IN_PATTERN);
                }
            }
        }; break;
    case EXFO_UNION:
        {
            for (i=0; i < argsNumber; i++)
                E(args[i] -> patternOK(S));
        }; break;
    default:
        Err(S, E_BAD_PATTERN);
        // sabassert(!"patternOK()");
    };
    return OK;
}

eFlag Expression::parse(Sit S, const DStr &string,
                        Bool _isPattern /* = FALSE  */,
			Bool defaultToo /* = FALSE  */)
{
    isPattern = _isPattern;
    Tokenizer t(*this);
    E( t.tokenize(S, string) );
    E( parse(S, t, 0, t.items.number() - 1, defaultToo) );
    if (_isPattern)
        E( patternOK(S) );
    return OK;
}

/*================================================================
Bool isOp()
returns True if the given token is an operator, in which case
'precedence' is set to its precedence
================================================================*/

Bool Expression::isOp(ExToken token, int &precedence)
{
    Bool is = TRUE;
    switch(token)
    {
    case TOK_OR:
        precedence = 0;
        break;
    case TOK_AND:
        precedence = 1;
        break;
    case TOK_EQ:
    case TOK_NEQ:
        precedence = 2;
        break;
    case TOK_LT:
    case TOK_GT:
    case TOK_LE:
    case TOK_GE:
        precedence = 3;
        break;
    case TOK_PLUS:
    case TOK_MINUS:
        precedence = 4;
        break;
    case TOK_MULT:
    case TOK_DIV:
    case TOK_MOD:
        precedence = 5;
        break;
    case TOK_MINUS1:
        precedence = 6;
        break;
    case TOK_VERT:
        precedence = 7;
        break;
    default:
        {
            is = FALSE;
            precedence = -1;
        };
    };
    return is;
}

/*================================================================
void getFunctionInfo()
returns function code and type for the function with given name
    if no such builtin function, returns EXFF_NONE
================================================================*/

void getFunctionInfo(const Str &s, ExFunctor &code, ExType &type)
{
    char *p = (char *) s;
    int i;

    for (i = 0; funcInfoTable[i].name; i++)
    {
        if (!strcmp(funcInfoTable[i].name,p))
            break;
    };
    code = funcInfoTable[i].func;
    type = funcInfoTable[i].type;
}

/*================================================================
void getExternalFunctionInfo()
returns function code and type for the function with given name
    if no such builtin function, returns EXFF_NONE
================================================================*/

void getExternalFunctionInfo(const Str &u, const Str &n, ExFunctor &code, ExType &type)
{
  char *p = (char *) n;
  char *u2 = (char *) u;
  int i;
  for (i = 0; extFuncInfoTable[i].name; i++)
    {
      if (!strcmp(extFuncInfoTable[i].name,p) && 
	  ((!strcmp(theSabExtNamespace,u2)) || !strcmp(theEXSLTDynNamespace,u2)) )
	break;
    };
  code = extFuncInfoTable[i].func;
  type = extFuncInfoTable[i].type;
}

struct OpItem
{
    ExFunctor fu;
    ExType ty;
    int arity;
} 
opTable[] =
{
    {EXFO_OR, EX_BOOLEAN, 3},
    {EXFO_AND, EX_BOOLEAN, 3},
    {EXFO_EQ, EX_BOOLEAN, 2},
    {EXFO_NEQ, EX_BOOLEAN, 2},
    {EXFO_LT, EX_BOOLEAN, 2},
    {EXFO_GT, EX_BOOLEAN, 2},
    {EXFO_LE, EX_BOOLEAN, 2},
    {EXFO_GE, EX_BOOLEAN, 2},
    {EXFO_PLUS, EX_NUMBER, 2},
    {EXFO_MINUS2, EX_NUMBER, 2},
    {EXFO_MULT, EX_NUMBER, 2},
    {EXFO_MOD, EX_NUMBER, 2},
    {EXFO_DIV, EX_NUMBER, 2},
    {EXFO_MINUS1, EX_NUMBER, 1},
    {EXFO_UNION, EX_NODESET, 3}
};

/*================================================================
eFlag parseLP()
================================================================*/

eFlag Expression::parseLP(Sit S, Tokenizer& tokens, int &pos, 
                     Bool dropRoot, Bool defaultToo /*=FALSE*/)
{
    sabassert(functor == EXF_LOCPATH);
    ExToken tok;
    BOOL getaway = FALSE;
    Expression *ls; // GP: OK (immediately appended)
    int& i = pos;
    Bool 
      slashPending = FALSE,
      nameWas = FALSE,
      nameRecent = FALSE;
    
    tok = tokens.items[i] -> tok;
    if (tok == TOK_END)
        Err(S, ET_EMPTY_PATT);
    if ((tok == TOK_SLASH) || (tok== TOK_DSLASH))
    {
        if (!dropRoot)
        {
            args.append(ls = new Expression(getOwnerElement(),EXF_LOCSTEP));
            ls -> step -> set(AXIS_ROOT,EXNODE_NODE);
        }
        if (tok == TOK_SLASH)
            i++;
    }
    
    while (!getaway)
    {
        tok = tokens.items[i] -> tok;
        switch(tok)
        {
        case TOK_NAME:
        case TOK_NTNAME:
        case TOK_AXISNAME:
        case TOK_ATSIGN:
        case TOK_PERIOD:
        case TOK_DPERIOD:
            {
	      if (nameRecent)
		Err(S, ET_EXPR_SYNTAX);
	      args.append(ls = new Expression(getOwnerElement(),EXF_LOCSTEP));
	      E( ls -> step -> parse(S, tokens, i, defaultToo) );
	      slashPending = FALSE;
	      nameWas = TRUE;
	      //nameRecent = (tok == TOK_NAME || tok == TOK_NTNAME)
	      nameRecent = TRUE;
            };
            break;
        case TOK_DSLASH:
            {
                args.append(ls = new Expression(getOwnerElement(),EXF_LOCSTEP));
                ls -> step -> set(AXIS_DESC_OR_SELF, EXNODE_NODE);
            };
            // no break here?
        case TOK_SLASH:
            {
                if (slashPending)
                    Err(S, ET_EXPR_SYNTAX);
                slashPending = TRUE;
                i++;
                if (tokens.items[i] -> tok == TOK_END) 
                    Err(S, ET_EMPTY_PATT);
		nameRecent = FALSE;
            };
            break;
        case TOK_VERT:
        case TOK_END:
        default: getaway = TRUE;
        };
    };
    if ((slashPending && nameWas) || !args.number())
        Err(S, ET_EMPTY_PATT);

    return OK;
}


/*================================================================
eFlag parseBasic()
    parses the basic expression in tokenizer 't' between positions
    'from' and 'to' inclusive. The basic expression is guaranteed
    to contain no operators (except for / and //) nor outer 
    parentheses.
================================================================*/

eFlag Expression::parseBasic(Sit S, Tokenizer &t, int from, int to,
    Bool defaultToo /* = FALSE */)
{
    GP( Expression ) e, lp;
    // find the start of the filtering predicates
    int fstart, fright, fleft;
    ExToken tok;

    switch(t.items[from] -> tok)
    {
    case TOK_VAR:
    case TOK_LITERAL:
    case TOK_NUMBER:
        fstart = from + 1;
        break;
    case TOK_FNAME:
        {
            t.getDelim(S, fstart = from + 1);
            fstart++;
        };
        break;
    case TOK_LPAREN:
        {
            t.getDelim(S, fstart = from);
            fstart++;
        };
        break;
    default:
        fstart = -1;
    };

//#pragma Msg("adding '+1':")
    if ((fstart != -1) && (fstart <= to))
    {
        switch(t.items[fstart] -> tok)
        {
        case TOK_LBRACKET:
        case TOK_SLASH:
        case TOK_DSLASH:
            {
                // parse the filtered expression into args[0]
                e = new Expression(getOwnerElement()); // is a GP
                E( (*e).parse(S, t, from, fstart - 1));
                args.append(e.keep());
                //
                functor = EXF_FILTER;
                type = EX_NODESET;
                fleft = fstart;
                while (t.items[fleft] -> tok == TOK_LBRACKET)
                {
                    t.getDelim(S, fright = fleft);
                    if ((t.items[fright] -> tok == TOK_END) || (fright > to))
                        Err(S, ET_RBRACKET_EXP);
                    if (fleft + 1 == fright)
                        Err(S, ET_EXPR_SYNTAX);
                    E( (e = new Expression(getOwnerElement())) -> parse(S, t,
                        fleft + 1, fright - 1, defaultToo) );
                    args.append(e.keep());
                    fleft = fright + 1;
                };
                if (((tok = t.items[fleft] -> tok) == TOK_SLASH)
                    || (tok == TOK_DSLASH))
                {
                    E( (lp = new Expression(getOwnerElement(), EXF_LOCPATH)) -> parseLP(
                        S, t, fleft, TRUE, defaultToo) );
                    hasPath = TRUE;
                    args.append(lp.keep());
                };
                if (fleft != to + 1)
                    Err(S, ET_EXPR_SYNTAX);
                return OK;
            };
            break;
        }
    };

    DStr temp;
    tok = t.items[from] -> tok;
    t.items[from] -> speak(temp,SM_OFFICIAL);
    if ((tok == TOK_VAR) || (tok == TOK_LITERAL)
        || (tok == TOK_NUMBER))
    {
        switch(t.items[from] -> tok)
        {
        case TOK_VAR:
            {
                functor = EXF_VAR;
                type = EX_UNKNOWN;
                // GP: OK (member)
                E( getOwnerElement().setLogical(S, 
		            *(pName = new QName), temp, FALSE) );
            }; break;
        case TOK_LITERAL:
            {
                functor = EXF_ATOM;
                type = EX_STRING;
                patomstring = new Str(temp);
            }; break;
        case TOK_NUMBER:
            {
                functor = EXF_ATOM;
                type = EX_NUMBER;
                *(patomnumber = new Number) = temp;
            }; break;
        };
        if (to != from)
            Err(S, ET_EXPR_SYNTAX);
    }
    else
    {
        if (tok == TOK_FNAME)
        {
            ExFunctor funcNo;
            ExType funcType;
            getFunctionInfo(temp,funcNo,funcType);
            if (funcNo != EXFF_NONE)
            {
                functor = funcNo;
                type = funcType;
            }
            else
	    {
	      QName funcName;
	      E( getOwnerElement().setLogical(S, funcName, temp, FALSE) );
	      Str uri = getOwnerTree().expand(funcName.getUri());
	      Str name = getOwnerTree().expand(funcName.getLocal());
	      getExternalFunctionInfo(uri,name,funcNo,funcType);
	      if (funcNo != EXFF_NONE) {
                functor = funcNo;
                type = funcType;
	      }
	      else {
		
                functor = EXF_OTHER_FUNC;
                E( getOwnerElement().setLogical(S, 
		            *(pName = new QName), temp, FALSE) );
                type = EX_UNKNOWN;
	      };
	    }
            int i = from+1,
                j;
            sabassert(t.items[i] -> tok == TOK_LPAREN);
            i++;
            // original loop test:
            // while (t.items[j = t.findTop(TOK_COMMA,i)] -> tok != TOK_END)
            while (((j = t.findTop(TOK_COMMA,i)) <= to) && (t.items[j] -> tok != TOK_END))
            {
                switch(t.items[j-1] -> tok)
                {   
                case TOK_COMMA:
                case TOK_LPAREN:
                    Err(S, ET_EXPR_SYNTAX);
                };
                args.append(e = new Expression(getOwnerElement()));
                e.keep();
                E( (*e).parse(S, t,i,j-1, defaultToo) );
                i = j+1;
            };

            if ((t.items[j = t.findTop(TOK_RPAREN,i)]->tok == TOK_END) || 
		(j > to))
	      {
                Err(S, ET_RPAREN_EXP);
	      }
            if(t.items[j-1] -> tok == TOK_COMMA)
                Err(S, ET_EXPR_SYNTAX);
            if (j > i)  // if any args
            {
                args.append(e = new Expression(getOwnerElement()));
                e.keep();
                E( (*e).parse(S, t,i,j-1, defaultToo) );
            }
            if (to != j)
                Err(S, ET_EXPR_SYNTAX);
        } // end "tok == TOK_FNAME"
        else
        {   // it must be a LocPath
            type = EX_NODESET;
            functor = EXF_LOCPATH;
            int howfar = from;
            E( parseLP(S, t, howfar, FALSE, defaultToo) );
            if (howfar != to + 1)
                Err(S, ET_EXPR_SYNTAX);
        }
    }
    return OK;
}

/*================================================================
eFlag parse()
    translates the given token list into an expression (a tree of
    'Expression' objects plus some leaves).
INPUT
    t           a tokenizer whose tokenize() method has been called
    from,to     first and last position in the token list the parsing
                applies to (i.e. a complex expression will parse the
                subexpressions with the same tokenizer but different
                limits)
================================================================*/

eFlag Expression::parse(Sit S, Tokenizer& t, int from, int to,
    Bool defaultToo /* = FALSE */)
// isOp, skipParens
{
    int i;
    ExToken 
        token, 
        mintoken = TOK_NONE;
    int precedence,
        minprec = 999,
        minndx = -1,
        leftmost = 0,
        arity;

    if (from > to)
        Err(S, ET_EXPR_SYNTAX);

    t.stripParens(S, from,to);
    // search from right to left (left-associativity)
    for (i = to; i >= from; i--)
    {
        switch(token = t.items[i] -> tok)
        {
        case TOK_RPAREN:
        case TOK_RBRACKET:
            {
                // reverse search:
                E( t.getDelim(S, i,TRUE) ); // i is decremented at loop end
                if (i == -1)
                    Err(S, ET_LPARCKET_EXP);
            };
            break;
        default: 
            {
                if (isOp(token, precedence) && (precedence < minprec))
                {
                    minprec = precedence;
                    minndx = leftmost = i;
                    mintoken = token;
//                    if (token == TOK_OR) break;
                }
                else 
                    if (token == mintoken)
                        leftmost = i;
            };
        };
    };

    //minndx now points to the rightmost lowest-precedence operator
    // leftmost points at its leftmost occurence

    if (minndx == -1)
        E( parseBasic(S, t, from, to, defaultToo) )
    else 
    {
        int tablendx = t.items[minndx] -> tok - TOKGROUP_OPERATORS;
        functor = opTable[tablendx].fu;
        type = opTable[tablendx].ty;
        arity = opTable[tablendx].arity;
        Expression *e = new Expression(getOwnerElement()); // GP: OK

        args.append(e);
        switch(arity)
        {
        case 1: 
            {
            if (minndx != from)
                Err(S, ET_EXPR_SYNTAX)
            else
                E( e -> parse(S, t,from+1,to, defaultToo) );
            };
            break;
        case 2:
            {
                E( e -> parse(S, t,from,minndx - 1, defaultToo) );
                args.append(e = new Expression(getOwnerElement())); // GP: OK
                E( e -> parse(S, t,minndx + 1, to, defaultToo) );
            };
            break;
        default:
            {
                E( e -> parse(S, t,from,leftmost - 1, defaultToo) );
                int another = leftmost, 
                    lastone = leftmost;
                // tom 24-10-00
                // the following fails for "x and not(x and x)"
                // t.getDelim(another);
                // changing it to:
                another = t.findTop(t.items[another]->tok, another+1);
                while((another <= to) && (t.items[another]->tok != TOK_END))
                {
                    args.append(e = new Expression(getOwnerElement())); // GP: OK
                    E( e -> parse(S, t, lastone + 1, another - 1, defaultToo));
                    lastone = another;

                    // tom 14-11-00
                    // t.getDelim(another);     failed too, for "x and x and (x and x)"
                    another = t.findTop(t.items[another]->tok, another+1);
                };
                args.append(e = new Expression(getOwnerElement())); // GP: OK
                E( e -> parse(S, t,lastone + 1, to, defaultToo) );
            };
        };
    }
    return OK;
}


void Expression::setAtom(Context *c)
{
    clearContent();
    functor = EXF_ATOM;
    type = EX_NODESET;
    patomnodeset = c;
}

void Expression::setAtom(const Number& n)
{
    clearContent();
    functor = EXF_ATOM;
    type = EX_NUMBER;
    *(patomnumber = new Number) = (Number&) n;
}

void Expression::setAtom(Bool b)
{
    clearContent();
    functor = EXF_ATOM;
    type = EX_BOOLEAN;
    atombool = b;
}

void Expression::setAtom(const DStr &s)
{
    clearContent();
    functor = EXF_ATOM;
    type = EX_STRING;
    patomstring = new Str(s);
}

#ifdef ENABLE_JS
void Expression::setAtom(const External &ext)
{
    clearContent();
    functor = EXF_ATOM;
    type = EX_EXTERNAL;
    patomexternal = new External();
    patomexternal -> assign(ext);
}
#endif

/*================================================================
setFragment
    sets the expression to point to a 'result tree fragment' - a newly
    constructed tree - whose address it returns
================================================================*/

Tree* Expression::setFragment()
{
    functor = EXF_FRAGMENT;
    type = EX_NODESET;
    return pTree = new Tree("RTF", FALSE); // not an XSL tree
}

#define funcIsOperator(f) ((EXFO_OR <= f) && (f <= EXFO_Z))
#define funcIsBuiltin(f) ((EXF_FUNCTION <= f) && (f <= EXFF_NONE))

eFlag Expression::eval(Sit S, Expression &retxpr, Context *c, Bool resolvingGlobals /* = FALSE */)
{
    sabassert(!isPattern && "evaluating pattern!");
    GP( Context ) newc;

    switch(functor)
    {
    case EXF_ATOM:
    {
	//cannot use retxpr = *this !!!
	switch(type)
	{
	case EX_STRING:
	    retxpr.setAtom(*patomstring);
	    break;
	case EX_NUMBER:
	    retxpr.setAtom(*patomnumber);
	    break;
	case EX_NODESET:
	    retxpr.setAtom(patomnodeset -> copy());
	    break;
	case EX_BOOLEAN:
	    retxpr.setAtom(atombool);
	    break;
#ifdef ENABLE_JS
	case EX_EXTERNAL:
	    retxpr.setAtom(*patomexternal);
	    break;
#endif
	default: sabassert(0);
	}
    }; break;
    case EXF_VAR:
    {
	Expression *ex = NZ(S.getProcessor()) -> getVarBinding(*pName);
	if (!ex)
	{
	    // if we're resolving globals, this may mean a forward reference
	    if (resolvingGlobals)
	      {
		E( S.getProcessor() -> resolveGlobal(S, c, *pName) );
		ex = S.getProcessor() -> getVarBinding(*pName);
	      }
	    else
	      {
		Str fullName;
		getOwnerTree().expandQStr(*pName, fullName);
		Err1(S, E1_VAR_NOT_FOUND, fullName);
	      }
	};
    E( ex -> eval(S, retxpr, c) );
    }; break;
    case EXF_LOCPATH:
    case EXFO_UNION:   
    {
	sabassert(c && "context is null!");
	newc.assign(c);
	E( createContext(S, newc) );
	newc.unkeep();
	// assign newc directly without copying
	retxpr.setAtom((*newc).copy()); 
	newc.del(); 
    }; break;
    case EXF_OTHER_FUNC: // other function
        {
	  Str ret;
	  Str uri = getOwnerTree().expand(pName -> getUri());
	  Str name = getOwnerTree().expand(pName -> getLocal());
	  
	  if (! S.getProcessor() -> supportsFunction(uri, name) )
	    {
	      Str fullName;
	      getOwnerTree().expandQStr(*pName, fullName);
	      Err1(S, ET_FUNC_NOT_SUPPORTED,fullName);
	    }
	  int i, 
	    argsNumber = args.number();
	  // ExprList atoms(LIST_SIZE_1);
	  GPD( ExprList ) atoms = new ExprList(LIST_SIZE_1);
	  atoms.autodelete();
	  GP( Expression ) ex;
	  for (i = 0; i < argsNumber; i++)
	    {
	      ex = new Expression(getOwnerElement());
	      E( args[i]->eval(S, *ex, c, resolvingGlobals) );
	      (*atoms).append(ex.keep());
	    };

#ifdef ENABLE_JS	  
	  E( S.getProcessor()->callJSFunction(S, c, uri, name, *atoms, retxpr) );
#endif
        }; break;
    case EXF_FILTER:
    {
	sabassert(c && "context is null!");
	newc.assign(c);
	E( createContext(S, newc, c -> getPosition()) );
	newc.unkeep();
	retxpr.setAtom((*newc).copy());
	newc.del();
    }; break;
    case EXF_STRINGSEQ:
    {
	DStr result;
	Expression temp(getOwnerElement());
	int i,
	    argsNumber = args.number();
	for (i = 0; i < argsNumber; i++)
	{
	    E( args[i] -> eval(S, temp, c, resolvingGlobals) );
	    Str tempStr;
	    E( temp.tostring(S, tempStr) );
	    result += tempStr;
	};
	retxpr.setAtom(result);
    }; break;
    case EXF_FRAGMENT:
    {
        newc = new Context(NULL); //current nde not needed _cn_
	(*newc).set(&(pTree -> getRoot()));
	retxpr.setAtom((*newc).copy());
	newc.del();
    }; break;
    default: 
    {
	int i, 
	    argsNumber = args.number();
	// ExprList atoms(LIST_SIZE_1);
	GPD( ExprList ) atoms = new ExprList(LIST_SIZE_1);
	atoms.autodelete();
	GP( Expression ) ex;
	for (i = 0; i < argsNumber; i++)
	{
	    ex = new Expression(getOwnerElement());
	    E( args[i]->eval(S, *ex, c, resolvingGlobals) );
	    (*atoms).append(ex.keep());
	};
	if (funcIsOperator(functor))
	  E( callOp(S, retxpr, *atoms) )           //an operator
	    else 
	      if (funcIsBuiltin(functor))
		E( callFunc(S, retxpr, *atoms, c) ) //a core XPath function
		  else
		    {
		      Str fullName;
		      getOwnerTree().expandQStr(*pName, fullName);
		      Err1( S, ET_FUNC_NOT_SUPPORTED, fullName );
		    }
	// atoms autodeleted
    };
    };
    return OK;
}



template<class T>
Bool hardCompare(ExFunctor op, T b1, T b2)
{
        Str p,q;
    switch(op)
    {
    case EXFO_EQ: return (Bool) (b1 == b2); break;
    case EXFO_NEQ: return (Bool) !(b1 == b2); break;
    case EXFO_LT: return (Bool) (b1 < b2); break;
    case EXFO_GT: return (Bool) (b2 < b1); break;
    case EXFO_LE: return (Bool) ((b1 < b2) || (b1 == b2)); break;
    case EXFO_GE: return (Bool) ((b2 < b1) || (b1 == b2)); break;
    default: sabassert(0);
    }
    return FALSE; //just to return something
}

ExFunctor _invertOp(ExFunctor op) 
{
    switch(op)
    {
    case EXFO_EQ: return EXFO_EQ; break;
    case EXFO_NEQ: return EXFO_NEQ; break;
    case EXFO_LT: return EXFO_GT; break;
    case EXFO_GT: return EXFO_LT; break;
    case EXFO_LE: return EXFO_GE; break;
    case EXFO_GE: return EXFO_LE; break;
    default: sabassert(!"_invertOp"); return EXF_NONE; // to return something
    }
}

Bool atomicCompare(ExFunctor op, const Str& str1, const Str& str2, 
		   Number* num2)
{
  switch(op) {
  case EXFO_EQ:
  case EXFO_NEQ:
    {
      return hardCompare(op, str1, str2);
    }; break;
  case EXFO_LT:
  case EXFO_GT:
  case EXFO_LE:
  case EXFO_GE:
      {
	Number n1, n2;
	n1 = str1;
	if (num2) n2 = *num2;
	else n2 = str2;
	return hardCompare(op, n1, n2);
      }; break;
  default:
    {
      sabassert(!"atomicCompare");
    }
  }
  return FALSE; //just to return something
}

Bool Expression::compareCC(Sit S, ExFunctor op, const Context &c1, const Context &c2)
{
    DStr str1, str2;
    GP( Context )
        c1prime = ((Context&) c1).copy(),
        c2prime = ((Context&) c2).copy();
    Bool resulting = FALSE;
    (*c1prime).reset();
    while ((*c1prime).current())
    {
	str1.empty();
	S.dom().constructStringValue((*c1prime).current(), str1); 
        (*c2prime).reset();
        while((*c2prime).current())
        {
	    str2.empty();
            S.dom().constructStringValue((*c2prime).current(), str2);
            if ( atomicCompare(op, str1, str2, NULL) )
            {
                resulting = TRUE;
                break;
            }
            (*c2prime).shift();
        };
        (*c1prime).shift();
    };
    // would be done automatically:
    c1prime.del();
    c2prime.del();
    return resulting;
}

Bool Expression::compareCS(Sit S, ExFunctor op, const Context &c1, const Str &str2)
{
    DStr str1;
    Bool resulting = FALSE;
    GP( Context ) c = ((Context&) c1).copy();
    //some optimization on str to num conversion
    Number* num2 = NULL;
    if (op != EXFO_EQ && op != EXFO_NEQ)
      {
	num2 = new Number();
	*num2 = str2;
      }
    
    (*c).reset();
    while((*c).current())
    {
        str1.empty();
        S.dom().constructStringValue((*c).current(), str1);
        if ( atomicCompare(op, str1, str2, num2) )
        {
            resulting = TRUE;
            break;
        }
        (*c).shift();
    };
    c.del();
    if (num2) delete num2;
    return resulting;
}

Bool Expression::compareCN(Sit S, ExFunctor op, const Context &c1, const Number& n2)
{
    Number n1;
    DStr s1;
    GP( Context ) c = ((Context&) c1).copy();
    Bool resulting = FALSE;

    (*c).reset();
    while((*c).current())
    {
        s1.empty();
        S.dom().constructStringValue((*c).current(), s1);
        n1 = s1;
        if (hardCompare(op, n1, (Number)n2))
        {
            resulting = TRUE;
            break;
        }
        (*c).shift();
    };
    c.del();
    return resulting;
}

eFlag Expression::compare(Sit S, Bool &result, Expression &other, ExFunctor op)
// Both *this and other are assumed to be ATOMS.
{
  sabassert(functor == EXF_ATOM);
  sabassert(other.functor == EXF_ATOM);
  
  ExType histype = other.type;

  //check external object
  if (type == EX_EXTERNAL || histype == EX_EXTERNAL)
    {
      Err(S, E_INVALID_OPERAND_TYPE);
    }

  //
  if (type == EX_NODESET)
    {
      if (other.type == EX_BOOLEAN)
	result = hardCompare(op, tobool(), other.tobool());
      else
        {
	  Context& mynodeset = tonodeset(); // GP: OK
	  switch(other.type)
            {
            case EX_NODESET:
	      result = compareCC(S, op, mynodeset, other.tonodesetRef());
	      break;
            case EX_STRING:
	      {
		Str temp;
		E( other.tostring(S, temp) );
		result = compareCS(S, op, mynodeset, temp);
	      }; break;
            case EX_NUMBER:
	      result = compareCN(S, op, mynodeset, other.tonumber(S));
	      break;
            default: sabassert(0);
            };
	  delete &mynodeset;
        }
    }
  else 
    {
      if (histype == EX_NODESET)
	{
	  E( other.compare(S, result, *this, _invertOp(op)) );
	}
      else
	{   // none of the two are nodesets
	  switch (op) {
	  case EXFO_EQ:
	  case EXFO_NEQ:
	    {
	      if (type == EX_BOOLEAN || histype == EX_BOOLEAN)
		result = hardCompare(op, tobool(), other.tobool());
	      else
		{
		  if (type == EX_NUMBER || histype == EX_NUMBER)
		    result = hardCompare(op, tonumber(S), other.tonumber(S));
		  else
		    {
		      if (type == EX_STRING || histype == EX_STRING)
			{
			  Str temp, otherTemp;
			  E( tostring(S, temp) );
			  E( other.tostring(S, otherTemp) );   
			  result = hardCompare(op, temp, otherTemp);
			}
		      else
			sabassert(0);
		    }
		}
	    }; break;
	  case EXFO_LT:
	  case EXFO_GT:
	  case EXFO_LE:
	  case EXFO_GE:
	    {
	      result = hardCompare(op, tonumber(S), other.tonumber(S));
	    }; break;
	  }
	}
    }
  return OK;
}

eFlag Expression::callOp(Sit S, Expression& retxpr, ExprList& atoms)
{
    int i,
        atomsNumber = atoms.number();
    switch(functor)
    {
    case EXFO_OR:
    case EXFO_AND:
        {
            sabassert(atomsNumber > 1);
            Bool result;
            result = atoms[0] -> tobool();
            for (i = 1; i < atomsNumber; i++)
            {
                if (functor == EXFO_OR)
                {
                    if (atoms[i] -> tobool())
                    {
                        result = TRUE;
                        break;
                    }
                }
                else    //EXFO_AND
                {
                    if (!atoms[i] -> tobool())
                    {
                        result = FALSE;
                        break;
                    }
                };
            };
            retxpr.setAtom(result);
        }; break;
    case EXFO_EQ:
    case EXFO_NEQ:
    case EXFO_LT:
    case EXFO_LE:
    case EXFO_GT:
    case EXFO_GE:
        {
            sabassert(atomsNumber == 2);
            Bool result;
            E( atoms[0]->compare(S, result,*(atoms[1]),functor) );
            retxpr.setAtom(result);
        }; break;
    case EXFO_PLUS:
    case EXFO_MINUS2:
    case EXFO_MULT:
    case EXFO_DIV:
    case EXFO_MOD:
        {
            sabassert(atomsNumber > 1);
            double result;
            result = (double) (atoms[0] -> tonumber(S));
            for (i = 1; i < atomsNumber; i++)
            {
                switch(functor)
                {
                case EXFO_PLUS:
                    result += atoms[i] -> tonumber(S);
                    break;
                case EXFO_MINUS2:
                    result -= atoms[i] -> tonumber(S);
                    break;
                case EXFO_MULT:
                    result *= atoms[i] -> tonumber(S);
                    break;
                case EXFO_DIV:
                    result /= atoms[i] -> tonumber(S);
                    break;
                case EXFO_MOD:
                    {
		      double d = atoms[i] -> tonumber(S);
		      double aux = result/d;
		      aux = aux > 0 ? floor(aux) : ceil(aux);
		      result =  result - (aux * d);
                    };
                    break;
                };
            };
	    //eliminate the -0 effect
	    if (!result) result = 0;
            retxpr.setAtom(Number(result));
        }; break;
    case EXFO_MINUS1:
        {
            sabassert(atomsNumber == 1);
            retxpr.setAtom(Number(-(double)(atoms[0] -> tonumber(S))));
        }; break;
    };
    return OK;
}

// this appends the list of nodes with given ID, possibly with multiplicities
void appendNodesWithID(Sit S, Str& ids, Context *c, Context& result)
{
    char *p = (char*) ids;
    Str singleID;
    NodeHandle singleNode;
    int tokenLen;
    for (skipWhite(p); *p; skipWhite(p))
    {
	singleID.nset(p, tokenLen = strcspn(p,theWhitespace));
	SXP_Document doc = S.dom().getOwnerDocument(c -> current());
	singleNode = S.dom().getNodeWithID(doc, singleID);
	if (singleNode)
	    result.append(singleNode);
	p += tokenLen;
    };
}

/*================================================================
callFunc
    calls the built-in XPath or XSLT function as determined by 
    'this -> functor'. Returns an expression in 'retxpr'.
    'atoms' is a list of atomized arguments, 'c' is the current context
    necessary for certain functions.
================================================================*/

#define checkArgsCount(x) if (atomsNumber != x)\
    Err(S, ET_BAD_ARGS_N);
#define checkArgsCountMax(x) if (atomsNumber > x)\
    Err(S, ET_BAD_ARGS_N);
#define checkArgsCountMin(x) if (atomsNumber < x)\
    Err(S, ET_BAD_ARGS_N);
#define checkArgsCountBetween(x,y) if ((atomsNumber < x) || \
    (atomsNumber > y)) Err(S, ET_BAD_ARGS_N);

// only check for being a nodeset
#define checkIsNodeset(x) if (atoms[x] -> type != EX_NODESET)\
    Err(S, ET_BAD_ARG_TYPE);

// Everything is a string, in a cosmic sense
#define checkIsString(x)
#define checkIsString2(x,y) 
#define checkIsNumber(x)

/*................
firstOccurence
Finds the first complete occurence of q in p; returns the 0-based starting
position, or -1 if not found
Now works for UTF-8 strings.
................*/

int firstOccurence(char *p, char *q)
{
    int i = 0,
        iCurr = 0,
        j = 0,
	    pos = 0,
	    charlen;
    while(p[i] && q[j])
    {
	charlen = utf8SingleCharLength(p + i);
	if (!strncmp(p + i, q + j, charlen))
	{
	    i += charlen;
	    j += charlen;
	}
	else
	{
	    i = (iCurr += utf8SingleCharLength(p + iCurr));
	    pos++;
	    j = 0;
	}
    };
    if (q[j]) 
        return -1;
    else
        return pos;
}

/*................
getBetween
Returns in s the portion of the source string between from and to inclusive.
If to == -1 then copies the whole rest of the string.
Now works for multibyte UTF-8 characters.
................*/

void getBetween(Str& s, char *source, int from, int to)
{
    sabassert(source);
    char *start = NULL;
    int i;
    if (from < 0) from = 0;
    for (i = 0; *source && (i <= to || to == -1); i++)
    {
	    if (i == from)
	    {
	        start = source;
		    if (to == -1)
		        // no need to go through the rest of the string
		        break;
        }
        if (!(*source & 0x80))
	        // optimize for ASCII chars
            source++;
	    else
            source += utf8SingleCharLength(source);
	}
	if (!start)
	    s.empty();
	else
	{
	    if (to == -1)
	        s = start;
		else
	        s.nset(start,(int)(source - start));
	}
}

/*................
getCurrValue
Returns the string value of the current node
................*/

eFlag getCurrValue(Sit S, Str &s, Context *c)
{
    NodeHandle v;
    DStr temp;
    if (!!(v = c -> current()))
        S.dom().constructStringValue(v, temp);
    else
        s.empty();
    s = temp;
    return OK;
}

// An auxiliary function which retrieves the document from a given URI,
// regardless of whether DOM is external or not. Should be moved into S.dom().

// new model for processing on external documents: first if have the
// processor and we're running on external, the DOMProvider is asked

eFlag Expression::getDocument_(Sit S, NodeHandle& newroot, 
			       const Str& location, const Str& baseUri,
			       Processor* proc)
{
  newroot = NULL;
  if ( proc && proc -> processingExternal() )
    {
      /* FIXME: this is incomplete */
      newroot = S.dom().retrieveDocument(location, baseUri);
    }
  //external not active or failed
  if ( nhNull(newroot) )
    {
      if ( proc ) 
	{
	  Tree *newtree;
	  Str uri, locBase;
	  if (baseUri == (const char*)"")
	      locBase = proc -> baseForVertex(S, &getOwnerElement());
	  else
	      locBase = baseUri;
	  makeAbsoluteURI(S, location, locBase, uri);

	  //deny document fragments for the 'file' scheme
	  const char* aux = (char*)uri;
	  const char *colon = strchr(aux, ':');
	  if ( colon
	       && ( ((colon - aux == 4) && !strncmp(aux, "file", 4)) ||
		    ((colon - aux == 3) && !strncmp(aux, "arg", 3))))
	    {
	      if ( strchr((const char*)uri, '#') )
		Err1(S, E_DOC_FRAGMENT, (char*)uri);
	    }
	  //if we found no colon, it's bad, but we do not care right here

	  Bool iserr = 
	    proc -> readTreeFromURI(S, newtree, uri, 
				    proc ->
				    baseForVertex(S, &getOwnerElement()), 
				    FALSE, 
				    S.hasFlag(SAB_IGNORE_DOC_NOT_FOUND));
	  if (!iserr) {
	    newroot = (NodeHandle) &(newtree -> getRoot());
	    //strip spaces
	    proc -> stripTree(S, *(newtree));
	  } else if (! S.hasFlag(SAB_IGNORE_DOC_NOT_FOUND))
	    return NOT_OK; //propagate an error
	}
      else
	{
	  Err1(S, E1_URI_OPEN, location);
	}
    }
  return OK;
}		    



eFlag Expression::callFunc(Sit S, Expression &retxpr, ExprList &atoms, Context *c)
{
    NodeHandle v;
    int atomsNumber = atoms.number();
    switch(functor)
    {
    case EXFF_LAST:
    {
	checkArgsCount(0);
	retxpr.setAtom( Number(c -> getSize()) );
    }; break;
    case EXFF_POSITION:
    {
	checkArgsCount(0);
	retxpr.setAtom( Number(c -> getPosition() + 1) );
    }; break;
    case EXFF_COUNT:
    {
	checkArgsCount(1);
	checkIsNodeset(0);
	retxpr.setAtom( 
	    Number(atoms[0] -> tonodesetRef().getSize()) );
    }; break;
    case EXFF_LOCAL_NAME:
    case EXFF_NAMESPACE_URI:
    case EXFF_NAME:
    {
	checkArgsCountMax(1);
	Str strg;
	if (!atomsNumber)
	    v = (c -> isFinished()) ? NULL : c -> current();
	else
	{
	    checkIsNodeset(0);
	    const Context& newc = atoms[0] -> tonodesetRef();
	    v = (newc.isVoid()? NULL : newc.current());
	};
	if (v)
	{
	    if (!S.domExternal(v))
	      {
		const QName& q = toV(v) -> getName();
		switch(functor)
		{
		case EXFF_NAME:
		    toV(v) -> getOwner().expandQStr(q, strg); break;
		case EXFF_LOCAL_NAME:
		    strg = toV(v) -> getOwner().expand(q.getLocal()); break;
		case EXFF_NAMESPACE_URI:
		    strg = toV(v) -> getOwner().expand(q.getUri()); break;
		}
	      }
	    else
	      {
		const char *aux;
		switch(functor) {
		case EXFF_NAME:
		  {
		    strg = (aux = S.dom().getNodeName(v));
		    S.dom().freeName(v, (char*)aux);
		  } break;
		case EXFF_LOCAL_NAME:
		  {
		    strg = (aux = S.dom().getNodeNameLocal(v));
		    S.dom().freeName(v, (char*)aux);
		  } break;
		case EXFF_NAMESPACE_URI:
		  {
		    strg = (aux = S.dom().getNodeNameURI(v));
		    S.dom().freeName(v, (char*)aux);
		  } break;
		}
	      }
	};
	retxpr.setAtom(strg);
    };
    break;

    case EXFF_STRING:
    {
	Str string;
	checkArgsCountMax(1);
	if (!atomsNumber)
	    E( getCurrValue(S, string, c) )
		else
		    E( atoms[0] -> tostring(S, string) );
	retxpr.setAtom(string);
    }; break;

    case EXFF_CONCAT:
    {
	checkArgsCountMin(2);
	DStr strg;
	for (int k = 0; k < atomsNumber; k++)
	{
	    checkIsString(k);
	    Str atomStr;
	    E( atoms[k] -> tostring(S, atomStr) );
	    strg += atomStr;
	};
	retxpr.setAtom(strg);
    }; break;

    case EXFF_STARTS_WITH:
    {
	checkArgsCount(2);
	checkIsString2(0,1);
	Str a0Str, a1Str;
	E( atoms[0] -> tostring(S, a0Str) );
	E( atoms[1] -> tostring(S, a1Str) );
	retxpr.setAtom((Bool) !firstOccurence(
	    a0Str, a1Str));
    }; break;

    case EXFF_CONTAINS:
    {
	checkArgsCount(2);
	checkIsString2(0,1);
	Str a0Str, a1Str;
	E( atoms[0] -> tostring(S, a0Str) );
	E( atoms[1] -> tostring(S, a1Str) );
	retxpr.setAtom((Bool) (firstOccurence(
	    a0Str, a1Str) != -1));
    }; break;

    case EXFF_SUBSTRING_BEFORE:
    case EXFF_SUBSTRING_AFTER:
    {
	Str strg;
	Str theBigger, theSmaller;
	E( atoms[0] -> tostring(S, theBigger) );
	E( atoms[1] -> tostring(S, theSmaller) );
	checkArgsCount(2);
	checkIsString2(0,1);
	int where = firstOccurence(theBigger,theSmaller);
	if (where == -1)
	    strg.empty();
	else
	{
	    if (functor == EXFF_SUBSTRING_BEFORE)
	    {
		if (where == 0)
		    strg.empty();
		else
		    getBetween(strg, theBigger, 0, where-1);
	    }
	    else
		getBetween(strg, theBigger, 
			   where + utf8StrLength(theSmaller), -1);
	};
	retxpr.setAtom(strg);
    }; break;

    case EXFF_SUBSTRING:
    {
	checkArgsCountBetween(2,3); 
	checkIsString(0); checkIsNumber(1);
	/* useless test causing a warning in MSVC:

	   if (atomsNumber == 3)
	   checkIsNumber(2);
	*/
	Str strg;
	Number from_ = atoms[1] -> tonumber(S) - 1;
	if (!from_.isNaN() && !from_.isInf())
	{
	    int from = from_.round(), 
		to = -1;
	    if (atomsNumber > 2)
	    {
		// use length in 3rd argument
		Number len = atoms[2] -> tonumber(S);
		if (len <= 0 || len.isNaN())
		    to = -2;
		else if (!len.isInf())
		    to = from + len.round() - 1;  // otherwise it remains -1
	    }
	    Str a0Str;
	    E( atoms[0] -> tostring(S, a0Str) );
	    getBetween(strg, a0Str, from, to);
	}
	retxpr.setAtom(strg);
    }; break;

    case EXFF_STRING_LENGTH:
    {
	checkArgsCountBetween(0,1);
	if (atomsNumber)
	{
	    checkIsString(0);
	    Str a0Str;
	    E( atoms[0] -> tostring(S, a0Str) );
	    retxpr.setAtom(Number(utf8StrLength(a0Str)));
	}
	else
	{
	    Str string;
	    E( getCurrValue(S, string, c) );
	    retxpr.setAtom(Number(utf8StrLength(string)));
	}
    }; break;

    case EXFF_NORMALIZE_SPACE:
    {
	checkArgsCountBetween(0,1);
	Str string;
	if (atomsNumber)
	{
	    checkIsString(0);
	    E( atoms[0] -> tostring(S, string) );
	}
	else
	    E( getCurrValue(S, string, c) );
	char *p = (char*) string;
	DStr stripped;
	skipWhite(p);
	while(*p)
	{
	    if (isWhite(*p))
	    {
		skipWhite(p);
		if (*p)
		    stripped += ' ';
		p--;
	    }
	    else
		stripped += *p;
	    p++;
	}
	retxpr.setAtom(stripped);
    }; break;

    case EXFF_TRANSLATE:
    {
	checkArgsCount(3);
	checkIsString2(0,1);
	checkIsString(2);

	DStr resulting;
	Str baseStr, srcStr, destStr;
	int pos;
	E( atoms[0] -> tostring(S, baseStr) );
	E( atoms[1] -> tostring(S, srcStr) );
	E( atoms[2] -> tostring(S, destStr) );
	char
	    // changing tostringCharPtr() to tostring():
	    *p = baseStr,
	    *src = srcStr,
	    *dest = destStr,
	    *destchar;
	while(*p)
	{
	    pos = utf8Strchr(src, p);
	    if (pos == -1) {
	      destchar = p;
	    } 
	    else if ((destchar = utf8StrIndex(dest,pos)) == NULL) {
	      p += utf8SingleCharLength(p);
	      continue;
	    }
	    resulting.nadd(destchar, utf8SingleCharLength(destchar));
	    p += utf8SingleCharLength(p);
	};
	retxpr.setAtom(resulting);
    }; break;

    case EXFF_BOOLEAN:
    {
	checkArgsCount(1);
	retxpr.setAtom(atoms[0] -> tobool());
    }; break;

    case EXFF_NOT:
    {
	checkArgsCount(1);
	retxpr.setAtom((Bool)!(atoms[0] -> tobool()));
    }; break;

    case EXFF_TRUE:
    {
	checkArgsCount(0);
	retxpr.setAtom(TRUE);
    }; break;

    case EXFF_FALSE:
    {
	checkArgsCount(0);
	retxpr.setAtom(FALSE);
    }; break;

    case EXFF_LANG:
    {
	checkArgsCount(1);
	checkIsString(0);
	// get the argument
	Str langQuery;
	E( atoms[0] -> tostring(S, langQuery) );
	NodeHandle w, att = NULL;
	int attCount, i;
	const char* langValue = NULL;
	for (w = c -> current(); w && !langValue; w = S.dom().getParent(w))
	{
	    // find whether w has an xml:lang attribute
	  if (!S.domExternal(w)) {
	    QName searchName;
	    searchName.setUri(getOwnerTree().unexpand(theXMLNamespace));
	    searchName.setLocal(getOwnerTree().unexpand("lang"));
	    int idx = toE(w) -> atts.findNdx(searchName);
	    if (idx != -1) {
	      langValue = toA( toE(w) -> atts[idx]) -> cont;
	    }
	  } else {
	    attCount = S.dom().getAttributeCount(w);
	    for (i = 0; i < attCount && !langValue; i++)
	      {
		att = S.dom().getAttributeNo(w, i);
		const char *uri = S.dom().getNodeNameURI(att);
		const char *local = S.dom().getNodeNameLocal(att);
		if (!strcmp(uri, theXMLNamespace)
		    && !strcmp(local, "lang"))
		    langValue = S.dom().getNodeValue(att);
		S.dom().freeName(att, (char*)uri);
		S.dom().freeName(att, (char*)local);
	      }
	  }
	}
	if (langValue)
	{
	  char *langQPtr = (char*) langQuery;
	    int qlen;
	    
	    // if strings equal except for possibly a suffix starting with -
	    // then return TRUE
	    if (!strncasecmp(langQPtr, langValue, qlen = langQuery.length())
		&& (langValue[qlen] == 0 || langValue[qlen] == '-'))
	      retxpr.setAtom(TRUE);
	    else
	      retxpr.setAtom(FALSE);
	    if (att)
	      S.dom().freeValue(att, (char*)langValue);
	}
	else
	    retxpr.setAtom(FALSE);
    }; break;

    case EXFF_NUMBER:
    {
	checkArgsCountMax(1);
	Number n;
	if (!atomsNumber)
	{
	    Str string;
	    E( getCurrValue(S, string, c) );
	    n = string;
	}
	else
	    n = atoms[0] -> tonumber(S);
	retxpr.setAtom(n);
    }; break;

    case EXFF_SUM:
    {
	DStr string;
	Number n, sum = 0;
	checkArgsCount(1);
	checkIsNodeset(0);
	GP( Context ) newc = &(atoms[0] -> tonodeset());
	(*newc).reset();
	while (!(*newc).isFinished())
	{
	    string.empty();
	    S.dom().constructStringValue((*newc).current(), string);
	    n = string;
	    if (n.isNaN())
	    {
		sum.setNaN(); 
		break;
	    };
	    sum = sum + n;
	    (*newc).shift();
	};
	newc.del();
	retxpr.setAtom(sum);
    }; break;

    case EXFF_FLOOR:
    case EXFF_CEILING:
    case EXFF_ROUND:
    {
	checkArgsCount(1);
	checkIsNumber(0);
	Number n = atoms[0] -> tonumber(S);
	switch(functor)
	{
	case EXFF_FLOOR:
	    n = floor((double)n); break;
	case EXFF_CEILING:
	    n = ceil((double)n); break;
	case EXFF_ROUND:
	    n = floor((double)n + .5); break;
	};
	retxpr.setAtom(n);
    }; break;

    case EXFF_DOCUMENT:
    {
	checkArgsCountMin(1);
	checkArgsCountMax(2);
	checkIsString(0);
	DStr location;
	Str baseUri;
	if (atomsNumber == 2)
	  {
	    checkIsNodeset(1);
	    const Context& aux = atoms[1] -> tonodesetRef();
	    if ( ! aux.isVoid() ) 
	      {
		NodeHandle n = aux[0];
		if (! S.domExternal(n) ) 
		  {
		    baseUri = NZ(toV(n) -> subtree) -> getBaseURI();
		  } else {
		    baseUri = "";
		  }
	      } else {
		baseUri = "";
	      }
	    //atoms[1] -> tostring(S, baseUri);
	  }
	else
	  baseUri = "";
	NodeHandle newroot = NULL;
	Processor *proc = S.getProcessor();
	//sabassert(S.domExternal((void*)1) || proc); //_ph_ sxp
	GP( Context ) newc = new Context(c->getCurrentNode());

	// Current node doesn't change
	//(*newc).setCurrentNode (c->getCurrentNode());

	// GP: the context doesn't autodelete anything on error
	// since readTreeFromURI adds the trees to datalines list
	// All datalines removed on error (in cleanupAfterRun())

	if (atoms[0] -> type == EX_NODESET)
	{
	    const Context& ctxt = atoms[0] -> tonodesetRef();
	    int ctxtNumber = ctxt.getSize();
	    for (int k = 0; k < ctxtNumber; k++)
	    {
		S.dom().constructStringValue(ctxt[k], location);
		E( getDocument_(S, newroot, location, baseUri, proc) );
		// check for duplicities and correct URI sorting!
		if (! nhNull(newroot) ) 
		  {
		    (*newc).append(newroot);
		    if (proc && !S.domExternal(newroot))  // _ph_ spx
		      E( proc -> makeKeysForDoc(S, newroot) );
		  }
	    };
	}
	else
	{
	    E( atoms[0] -> tostring(S, location) );
	    E( getDocument_(S, newroot, location, baseUri, proc) );
	    if (! nhNull(newroot) )
	      {
		(*newc).append(newroot);
		if (proc && !S.domExternal(newroot)) //_ph_ spx
		  E( proc -> makeKeysForDoc(S, newroot) );
	      }
	}
	retxpr.setAtom(newc.keep());
    }; break;

    case EXFF_GENERATE_ID:
    {
	DStr s;
	switch(atomsNumber)
	{
	case 0:
	    v = (c -> isFinished() ? NULL : c -> current());
	    break;
	case 1:
	{
	    checkIsNodeset(0);
	    const Context& newc = atoms[0] -> tonodesetRef();
	    v = (newc.isVoid()? NULL : newc.current());
	}; break;
	default:
	    Err(S, ET_BAD_ARGS_N);
	};
	if (v)
	{
	    s = "i__";
	    if (S.domExternal(v))
	      {
		// need cast to long then to int to avoid
		// compiler error on IBM AIX
		s += (int) (long) v;
	      }
	    else 
	      {
		s += (int) (long) &(toV(v) -> getOwner());
		s += "_";
		s += toV(v) -> stamp;
	      }
	}
	retxpr.setAtom(s);
    }; break;

    case EXFF_SYSTEM_PROPERTY:
    {
	checkArgsCount(1);
	checkIsString(0);
	QName q;
	Str a0Str;
	E( atoms[0] -> tostring(S, a0Str) );
	E( getOwnerElement().setLogical(S, q, a0Str, FALSE) );
	if (q.getUri() == getOwnerTree().stdPhrase(PHRASE_XSL_NAMESPACE))
	  {
	    const Str& localStr = getOwnerTree().expand(q.getLocal());
	    if (localStr == (const char*) "version")
	      retxpr.setAtom(Number(1.0));
	    else if (localStr == (const char*) "vendor")
	      retxpr.setAtom(Str("Ginger Alliance"));
	    else if (localStr == (const char*) "vendor-url")
	      retxpr.setAtom(Str("www.gingerall.com"));
	    else
	      retxpr.setAtom(Str(""));
	}
	else if (q.getUri() == getOwnerTree().stdPhrase(PHRASE_SABEXT_NAMESPACE))
	  {
	    const Str& localStr = getOwnerTree().expand(q.getLocal());
	    if (localStr == (const char*) "version")
	      retxpr.setAtom(Str(SAB_VERSION));
	    else
	      retxpr.setAtom(Str(""));
	  }
	else
	    retxpr.setAtom(Str(""));
    }; break;

    case EXFF_EVAL:
    {
	Str string;
	checkArgsCount(1);
	E( atoms[0] -> tostring(S, string) );
	Expression *ex = new Expression(getOwnerElement());
	E( (*ex).parse(S,string,FALSE,FALSE) );
	E( (*ex).eval(S,retxpr,c) )
    }; break;

    case EXFF_CURRENT:
    {
	Context *newc = new Context(NULL);  //_cn_ no need for cn
	newc -> set ( NZ(c -> getCurrentNode()));
	//Context *origc = getOwnerElement().getOrigContext();
	//sabassert(origc);
	//newc -> set ( origc -> current());
	retxpr.setAtom (newc);
    }; break;
    case EXFF_KEY:
    {
	checkArgsCount(2);
	checkIsString(0);
	QName q;
	Str a0Str;
	E( atoms[0] -> tostring(S, a0Str) );
	getOwnerElement().setLogical(S, q, a0Str, FALSE);
	EQName ename;
	getOwnerTree().expandQ(q, ename);
	Processor *proc = NZ(S.getProcessor());
	GP( Context ) newc = new Context(NULL); //_cn_
	GP( Context ) newc2;
	Context auxContext(NULL);
	DStr oneString;
	if (atoms[1] -> type == EX_NODESET)
	{
	    const Context& ctxt = atoms[1] -> tonodesetRef();
	    int ctxtNumber = ctxt.getSize();
	    for (int k = 0; k < ctxtNumber; k++)
	    {
		S.dom().constructStringValue(ctxt[k], oneString);
		E( proc -> getKeyNodes(S, ename, 
				       oneString, auxContext, 
				       S.dom().getOwnerDocument(c -> current())) );
		newc2 = (*newc).swallow(S, &auxContext);
		newc.del();
		newc.assign(newc2.keep());
	    };
	}
	else
	{
	    E( atoms[1] -> tostring(S, oneString) );
	    E( proc -> getKeyNodes(S, ename, 
				   oneString, *newc, 
				   S.dom().getOwnerDocument(c -> current())) );
	}
	retxpr.setAtom(newc.keep());	                	        
    }; break;
    case EXFF_FORMAT_NUMBER:
    {
	checkArgsCountBetween(2,3);
	Number num = atoms[0] -> tonumber(S);
	Str fmt;
	E( atoms[1] -> tostring(S, fmt) );
	EQName ename;
	if (atomsNumber == 3)
	{
	    Str nameStr;
	    E( atoms[2] -> tostring(S, nameStr) );
	    QName q;
	    getOwnerElement().setLogical(S, q, nameStr, FALSE);
	    getOwnerTree().expandQ(q, ename);
	}
	Str result;
	E( NZ(S.getProcessor()) -> decimals().format(S, ename, num, fmt, result) );
	retxpr.setAtom(result);	
    }; break;

    case EXFF_ID:
    {
	checkArgsCount(1);
	GP( Context ) result = new Context(NULL);
	DStr ids;
	if (atoms[0] -> type == EX_NODESET)
	{
	    const Context& ctxt = atoms[0] -> tonodesetRef();
	    int ctxtNumber = ctxt.getSize();
	    for (int k = 0; k < ctxtNumber; k++)
	    {
		S.dom().constructStringValue(ctxt[k], ids);
		appendNodesWithID(S, ids, c, *result);
	    };
	}
	else
	{
	    E( atoms[0] -> tostring(S, ids) );
	    appendNodesWithID(S, ids, c, *result);
	};
	E( (*result).sort(S) );
	(*result).uniquize();
	retxpr.setAtom(result.keep());
    }; break;

    case EXFF_FUNCTION_AVAILABLE:
      {
	checkArgsCount(1);
	Str nameStr;
	QName funcName;            
	ExFunctor funcNo;
	ExType funcType;
	E( atoms[0] -> tostring(S, nameStr));
	E( getOwnerElement().setLogical(S, funcName, nameStr, FALSE) );
	Str uri = getOwnerTree().expand(funcName.getUri());
	Str name = getOwnerTree().expand(funcName.getLocal());
	getExternalFunctionInfo(uri,name,funcNo,funcType);
	if (funcNo == EXFF_NONE) {
	  retxpr.setAtom(S.getProcessor() -> supportsFunction(uri, name));
	} else {
	  retxpr.setAtom(TRUE);
	};
      }; break;
      
    case EXFF_ELEMENT_AVAILABLE:
      {
	checkArgsCount(1);
	Str nameStr;
	QName elName;
	E( atoms[0] -> tostring(S, nameStr));
	E( getOwnerElement().setLogical(S, elName, nameStr, FALSE) );
	Bool ret = ExtensionElement::elementAvailable(getOwnerTree(), elName);
	retxpr.setAtom(ret);
      }; break;
    case EXFF_UNPARSED_ENTITY_URI:
      {
	checkArgsCount(1);
	NodeHandle curr;
	NZ( curr = c -> current() );
	if (S.domExternal(curr))
	  {
	    retxpr.setAtom(""); //not supported for external docs
	    //add check for SXPF_SUPPORTS_UNPARSED_ENTITIES
	  }
	else
	  {
	    Str name;
	    E( atoms[0] -> tostring(S, name) );
	    Str *uri = toV(curr) -> getOwner().getUnparsedEntityUri(name);
	    if (uri) 
	      retxpr.setAtom(*uri);
	    else
	      retxpr.setAtom("");
	  }
      }; break;

    default:
        Err1(S, ET_FUNC_NOT_SUPPORTED, getFuncName(functor));
    }
    return OK;
}

eFlag Expression::createLPContext(Sit S, Context *&c, int baseNdx, NodeHandle givenGlobalCurrent /* = NULL */)
{
    sabassert(functor == EXF_LOCPATH);
    GP( Context ) theResult = new Context(c->getCurrentNode()); //_cn_ result has no cn
    Context info(givenGlobalCurrent ? givenGlobalCurrent : c -> getCurrentNode()); //_cn_
    //info.setCurrentNode(givenGlobalCurrent ? givenGlobalCurrent : c -> current());
    //(*theResult).setCurrentNode(c -> getCurrentNode());
    E( createLPContextLevel(S, 0, args.number(), c -> current(), info, theResult) );
    E( (*theResult).sort(S) );
    (*theResult).uniquize();
    c = theResult.keep();
    return OK;
}

/*
 *  createLPContextLevel
 *  ranges over all nodes that satisfy the stepLevel-th step, calling self
 *  recursively until the last one is reached. The vertices satisfying the last
 *  step are added to theResult. 
 *  Base is passed from the preceding step and used for expression
 *  evaluation. info holds the 'globally current' vertex.
 *  The purpose of this routine is to generate a context without having to
 *  also generate the intermediate contexts for each step. Also, some of the predicates
 *  may be known to use last(), in which case we first have to compute the number
 *  of nodes that reach such a predicate at all. 
 */

eFlag Expression::createLPContextLevel(Sit S, 
    int stepLevel, int stepsCount, NodeHandle base,
    Context &info, Context *theResult)
{
    // GP: theResult will be freed on error since the caller (createLPContext) holds it in a GP

    sabassert(functor == EXF_LOCPATH);
    int i, j, init,
        predsCount = args[stepLevel] -> step -> preds.number(),
        lastBad = -1;     // last bad predicate, or the step itself


    // keep a stack of positions, one for each predicate IN THIS STEP
    List<int> reached(predsCount),  // serves as position for next pred
        totalReached(predsCount);   // serves as size for next (bad) pred

    // there will be as many dry (size-counting) runs as there are bad preds
    Bool dryRun = TRUE,
        quitThisRound = FALSE, quitThisVertex = FALSE;

    // i ranges over predicates. Value i==predsCount is the special last run
    for (i = 0; i <= predsCount; i++)
    {
        if (i == predsCount)
            // the last run, not a dry-run
            dryRun = FALSE;
        // if this is the last run, or if the current pred uses last(), compute
        // the context size
        if (!dryRun || args[stepLevel] -> step -> preds[i] -> usesLast)
        {
            // initialize the size arrays: 
            // append base values for preds past the last bad one, 
            // up to this bad one (incl.)
            for (init = 0; init <= lastBad; init++)
                reached[init] = 0;
            for (init = lastBad + 1; init <= i; init++)
            {
                reached.append(0);
                totalReached.append(-1);     // -1 just for safety
            };

            // locally current vertex 
            NodeHandle locCurr = NULL;

            quitThisRound = FALSE;
            do
            {
                // shift the locally current vertex
	      //_speed_ this should be replaced with smart context 
	      //created with createContext on local step argument
                E( args[stepLevel] -> step -> shift(S, locCurr, base) );
                if (!nhNull(locCurr))
                {
                    if ((lastBad < 0) || !dryRun) ++reached[0];
                    quitThisVertex = FALSE;
                    for (j = 0; j < i; j++)
                    {
                        Bool satisfies;
                        info.deppendall();
                        // set locCurr as current at position reached[j]-1 in the context
                        info.setVirtual(locCurr, reached[j] - 1, totalReached[j]);

                        Expression *thisPred = 
                            args[stepLevel] -> step -> preds[j];
                        // find whether we're in position bounds for this pred
                        switch(thisPred -> inBounds(reached[j] - 1))
                        {
                        case 0:
                            // within bounds
                            {
                                E( thisPred -> trueFor(S, &info, satisfies) );
                                if (satisfies)
                                    ++reached[j + 1];
                                else
                                    quitThisVertex = TRUE;
                            }; break;
                        case -1:
                            // before start, move to another vertex
                            quitThisVertex = TRUE;
                            break;
                        case 1:
                            // past the end, bail out
                            quitThisRound = TRUE;
                            break;
                        };
                        if (quitThisVertex || quitThisRound)
                            break;
                    };
                    if (j == i && !dryRun)     // passed all preds
                    {
                        if (stepLevel < stepsCount - 1)
                            E( createLPContextLevel(S, 
                                stepLevel + 1, stepsCount,
                                locCurr, info, theResult))
                        else
                            theResult -> append(locCurr);

                    }   // if ! dryRun
                }       // if locCurr
            } while (!nhNull(locCurr) && !quitThisRound);
            // move all data collected to safe places
            for (init = lastBad + 1; init <= i; init++)
                totalReached[init] = reached[init];
            lastBad = i;
        }               // if bad predicate
    }                   // for, over all preds
    return OK;
}


eFlag Expression::createLPContextSum(Sit S, Context *&c, NodeHandle globalCurrent /* = NULL */)
{
    sabassert(functor == EXF_LOCPATH);
    GP( Context ) newc = new Context(c->getCurrentNode()); //_cn_ needed?
    Context 
        *newc2, *returnedc;
    int cNumber = c -> getSize();
    for (int j = 0; j < cNumber; j++)
    {
        E( createLPContext(S, returnedc = c, j, globalCurrent) );
        newc2 = (*newc).swallow(S, returnedc);
        newc.del();
        newc = newc2;
        delete returnedc;
	/* tom 01/11/25 */
	c -> shift();
    }
    c = newc.keep();
    return OK;
}


/*................................................................
createContext()
    creates a context for this expression, based on its functor.
................................................................*/

// GP: createContext is clean
// if unsuccessful, returns NULL in c and performs no allocations


eFlag Expression::createContext(Sit S, Context *& c, int baseNdx /* = -1 */)
{
    GP( Context ) newc; // newc gets assigned c ONLY IN THE END
    Context *c_orig = c;
    newc.assign(c);
    c = NULL;

    int i, j, 
        argsNumber = args.number();
    if (baseNdx == -1)
        baseNdx = (*newc).getPosition();
    switch(functor)
    {
    case EXF_VAR:
        {
	  Expression *deref = NULL;
	  if (S.getProcessor())
	    deref = S.getProcessor() -> getVarBinding(*pName);
	  if (!deref)
	  {
	      Str fullName;
	      getOwnerTree().expandQStr(*pName, fullName);
	      Err1(S, E1_VAR_NOT_FOUND, fullName);
	  }
	  NodeHandle current_node = (*newc).getCurrentNode();
	  E( deref -> createContext(S, newc, baseNdx) );
	  newc.unkeep();
	  (*newc).setCurrentNode(current_node);
        };
        break;
    case EXF_ATOM:
        {
            if (type != EX_NODESET)
                Err(S, ET_CONTEXT_FOR_BAD_EXPR);
            newc = patomnodeset -> copy();
            newc.unkeep();
        }; break;
    case EXFO_UNION:
        {
            sabassert(baseNdx != -1);  // meaningful only for a locpath
            GP( Context ) csummand;
            Context *newc2;     // GP: OK
            sabassert(argsNumber);
            E( args[0] -> createContext(S, newc, baseNdx) );
            newc.unkeep();
            for (i = 1; i < argsNumber; i++)
            {
                csummand.assign(c_orig);
                E( args[i] -> createContext(S, csummand, baseNdx) );
                newc2 = (*newc).swallow(S, csummand);
                csummand.del();
                newc.del();
                newc = newc2;
            }
            // clean up newc!
            (*newc).reset();
        }
        break;
    case EXF_LOCPATH:
        {
            E( createLPContext(S, newc, baseNdx) );
            newc.unkeep();
        }
        break;
    case EXF_FILTER:
        {
            sabassert(baseNdx != -1);  // meaningful only for a locpath
            NodeHandle wasCurrent = (*newc).getCurrentNode();
            E( args[0] -> createContext(S, newc, baseNdx) );
            newc.unkeep();
            (*newc).setCurrentNode(wasCurrent);

            GP( Context ) filteredc;
            for (i = 1; i < argsNumber - (int) hasPath; i++)
            {
                filteredc = new Context(c_orig -> getCurrentNode());
                (*newc).reset();
                Bool istrue;
                int newcNumber = (*newc).getSize();
                for (j = 0; j < newcNumber; j++)
                {
                    E(args[i] -> trueFor(S, newc, istrue));
                    if (istrue)
                        (*filteredc).append((*newc)[j]);
                    (*newc).shift();
                };
                newc.del();
                newc = filteredc.keep();
                if (!(*newc).getSize()) break;
            };
            if (hasPath)
            {
                filteredc.assign(newc);
		filteredc = newc;  // a patch due to SGI MIPSpro compiler
                E( args[argsNumber-1] -> createLPContextSum(S, filteredc, (*newc).getCurrentNode()) );
                newc.del();
                newc = filteredc.keep();
            }
        }
        break;
    case EXF_LOCSTEP:
        {
            sabassert(step);
            sabassert(baseNdx != -1);  // meaningful only for a locpath

            /////////
            // E( step -> createContextNoPreds(newc = c, baseNdx) );    - done as follows:
            GP( Context ) newc2 = new Context(c_orig -> getCurrentNode());
            NodeHandle curr = NULL;
            do
            {
	      //_speed_ here we should test the axis type
	      // and use smart context if possible
                E( step -> shift(S, curr, (*newc)[baseNdx]) );
                if (!nhNull(curr))
                    (*newc2).append(curr);
            } 
            while (!nhNull(curr));
            /////////

            GP( Context ) filteredc;
            int stepPredsNumber = step -> preds.number();
            for (i = 0; i < stepPredsNumber; i++)
            {
                filteredc = new Context(c_orig -> getCurrentNode());
                (*newc2).reset();
                Bool istrue;
                int newc2Number = (*newc2).getSize();
                for (j = 0; j < newc2Number; j++)
                {
                    E( step -> preds[i] -> trueFor(S, newc2,istrue) );
                    if (istrue)
                        (*filteredc).append((*newc2)[j]);
                    (*newc2).shift();
                };
                newc2.del();
                newc2 = filteredc.keep();
                if (!(*newc2).getSize()) break;
            };
            newc = newc2.keep();
        };
        break;
    case EXF_OTHER_FUNC:
	{
	  Expression resolved(getOwnerElement());
	  E( eval(S, resolved, newc) );
	  E( resolved.createContext(S, newc, baseNdx) );
	  newc.unkeep();
	}; break;
    default:
      if (funcIsBuiltin(functor))
        {
            Expression resolved(getOwnerElement());
            E( eval(S, resolved, newc) );
            E( resolved.createContext(S, newc, baseNdx) );
            newc.unkeep();
        }
        else
        Err(S, ET_CONTEXT_FOR_BAD_EXPR);
    };
    c = newc.keep();
    return OK;
}


eFlag Expression::matchesSingleStep(Sit S, NodeHandle v, Bool &result)
{
    sabassert(functor == EXF_LOCSTEP);
    if (!NZ(step) -> matchesWithoutPreds(S, v))
        RetOK(result, FALSE);
    if (!step -> preds.number())
        RetOK(result, TRUE);
    if (!S.dom().getParent(v))
        RetOK(result, FALSE);
    if (!step -> positional)
    {
      GP( Context ) c = new Context(NULL); //_cn_ current() is not allowed in patterns
        (*c).set(v);
        Bool stillOK = TRUE;
        for (int i = 0; i < step -> preds.number() && stillOK; i++)
            E(step -> preds[i] -> trueFor(S, c,stillOK));
        c.del();
        RetOK(result,stillOK);
    }
    else // positional case
    {
        GP( Context ) c = new Context(NULL);  //_cn_ current() is not allowed in patterns
        Context *newc; // GP: OK
        (*c).set(S.dom().getParent(v));
        E( createContext(S, newc = c+0, 0) );
        result = (newc -> contains(v));
        c.del();
        delete newc;
    }
    return OK;
}


eFlag Expression::matchesSinglePath(Sit S, NodeHandle v, int lastIndex, Bool& result)
{
    sabassert(functor == EXF_LOCPATH);
    int i;
    NodeHandle w = v;
    for (i = lastIndex; i >= 0; i--)
    {
        if (!w) 
            RetOK(result, FALSE);
        switch(args[i] -> step -> ax)
        {
        case AXIS_ROOT:
	        {
                if (i)
                    sabassert(!"root not first");
                E( args[i] -> matchesSingleStep(S, w, result) );
                if (!result) RetOK(result, FALSE);
		    };
            break;
        case AXIS_CHILD:
        case AXIS_ATTRIBUTE:
            {
                E( args[i] -> matchesSingleStep(S, w, result) );
                if (!result) RetOK(result, FALSE);
                w = S.dom().getParent(w);
            };
            break;
        case AXIS_DESC_OR_SELF:
            {
                E( args[i] -> matchesSingleStep(S, w, result) );
                if (!result) RetOK(result, FALSE);
                NodeHandle previous = w;
                while (previous)
                {
                    E(matchesSinglePath(S, previous, i-1, result));
                    if (result)
                        return OK;
                    else
                        previous = S.dom().getParent(previous);
                };
                RetOK( result, FALSE );
            }
            break;
        default:
            sabassert(!"bad axis in pattern");
        }
    }
    result = TRUE;
    return OK;
}

/*
 *  optimizePositional()
 *  called for a predicate of a locstep
 *  returns 2 if the predicate uses the last() function
 *    so the size of the context has to be determined
 *  returns 1 if the predicate only uses position()
 *          0 if neither
 */

int Expression::optimizePositional(int level)
{
    int result = 0;

    switch(functor)
    {
    case EXFF_LAST:
        result = 2; break;
    case EXFF_POSITION:
        result = 1; break;
    case EXF_ATOM:
    case EXF_VAR:
    case EXF_LOCPATH:
        /* result = 0; */ break;
    case EXF_STRINGSEQ:
    case EXF_FRAGMENT:
    case EXF_LOCSTEP:
        sabassert(!"invalid predicate type");
        break;
    case EXF_FILTER: // can be e.g. "current()/@attr"
    default: // all the functions and operators, including EXF_OTHER_FUNC
        {
            int sub = 0;
            for (int i = 0; i < args.number(); i++)
            {
                if (!!(sub = args[i] -> optimizePositional(level + 1)))
                {
                    result = sub;
                    if (result == 2) break;
                }
            }
        }
    }
    //PH: when we're called w/o recursion, 
    //we might be called for situation like foo[45], 
    //so we should return 1, if the expression 
    //is of the type of EX_NUMBER
    if (!level && type == EX_NUMBER && !result) result = 1;

    usesLast = (result == 2);
    positional = (result >= 1);
    return result; 
}

/*
 *  optimizePositionBounds()
 *  called for a predicate of a locstep
 *  returns the range of positions that need to be examined for a context
 *  e.g. the predicate in foo[1] will return both set to 1.
 *  the positions returned are 1-based, 0 means "no restriction"
 */

void Expression::optimizePositionBounds()
{
    int from = 0, to = 0;
    switch(functor)
    {
    case EXF_ATOM:
        {
            if (type == EX_NUMBER)
                from = to = NZ(patomnumber) -> round();    // bad values like NaN return 0 which is OK.
        }; break;
    case EXFO_EQ:
    case EXFO_LE:
    case EXFO_LT:
    case EXFO_GE:
    case EXFO_GT:
        {
            if (args[0] -> functor == EXFF_POSITION && 
                args[1] -> functor == EXF_ATOM && args[1] -> type == EX_NUMBER)
            {
                int bound = args[1] -> patomnumber -> round();
                switch(functor)
                {
                case EXFO_EQ: from = to = bound; break;
                case EXFO_LE: to = bound; break;
                case EXFO_LT: to = bound - 1; break;
                case EXFO_GE: from = bound; break;
                case EXFO_GT: from = bound + 1; break;
                }
            }
        }; break;
    }
    optimizePositionFrom = from;
    optimizePositionTo = to;
}

int Expression::inBounds(int position) const
{
    if (optimizePositionTo && position > optimizePositionTo-1)
        return 1;
    if (optimizePositionFrom && position < optimizePositionFrom-1)
        return -1;
    return 0;
}

Element& Expression::getOwnerElement() const
{
    return owner;
};

Tree& Expression::getOwnerTree() const
{
    return owner.getOwner();
}


void Expression::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    getOwnerElement().report(S,type,code,arg1,arg2);
}

Bool Expression::containsFunctor(ExFunctor func)
{
  if (functor == func)
    return TRUE;
  else if (functor == EXF_LOCSTEP)
    {
      for (int i = 0; i < step -> preds.number(); i++)
	{
	  if (step -> preds[i] -> containsFunctor(func))
	    return TRUE;
	}
    }

  for (int i = 0; i < args.number(); i++)
    {
      if (args[i] -> containsFunctor(func))
	return TRUE;
    }
  return FALSE;
}
