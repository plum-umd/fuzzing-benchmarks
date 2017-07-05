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

//#include <iostream.h>
#include <assert.h>
#include "error.h"

// GP: clean

SabMsg SablotMessages[] = 
{
    {E_OK, "OK"},
    {E_NOT_OK, "error"},
    {E_XML, "XML parser error %s: %s"}, 
    {E_FILE_OPEN, "cannot open file '%s'"}, 
    {E_MEMORY, "out of memory"},
    {E1_UNSUPP_XSL, "unsupported XSL instruction '%s'"}, 
    {ET_BAD_XSL, "non-XSL instruction"}, 
    {ET_REQ_ATTR, "missing required XSLT attribute"}, 
    {ET_BAD_ATTR, "unexpected attribute '%s'"},
    {E1_UNKNOWN_AXIS, "unknown axis '%s'"}, 
    {ET_EXPR_SYNTAX, "wrong expression syntax"}, 
    {ET_BAD_NUMBER, "number in bad format"}, 
    {ET_BAD_VAR, "bad variable"},
    {ET_INFINITE_LITERAL, "infinite literal"},
    {ET_LPAREN_EXP, "'(' expected"}, 
    {ET_RPAREN_EXP, "')' expected"}, 
    {ET_LPARCKET_EXP, "'(' or '[' expected"}, 
    {ET_RBRACKET_EXP, "']' expected"}, 
    {ET_EMPTY_PATT, "pattern is empty"}, 
    {ET_BAD_TOKEN, "token '%s' not recognized"}, 
    {E_BAD_AXIS_IN_PATTERN, "illegal axis in pattern"},
    {E_BAD_PATTERN, "invalid pattern"},
    {E_VAR_IN_MATCH, "match pattern contains a variable reference"},
    {E1_VAR_NOT_FOUND, "variable '%s' not found"}, 
    {E1_VAR_CIRCULAR_REF, "circular reference to variable '%s'"},
    {ET_CONTEXT_FOR_BAD_EXPR, "expression is not a node set"}, 
    {ET_BAD_ARGS_N, "invalid number of function arguments"},
    {ET_BAD_ARG_TYPE, "illegal argument type"},
    {ET_FUNC_NOT_SUPPORTED, "function '%s' not supported"}, 
    {ET_BAD_PREFIX, "invalid namespace prefix '%s'"}, 
    {E1_RULE_NOT_FOUND, "called nonexistent rule '%s'"}, 
    {ET_DUPLICATE_RULE_NAME, "duplicate rule name '%s'"}, 
    {ET_DUPLICATE_ASET_NAME, "duplicate attribute set name '%s'"}, 
    {E1_NONEX_ASET_NAME, "nonexistent attribute set name '%s'"}, 
    {E1_CIRCULAR_ASET_REF, "circular reference to attribute set '%s'"}, 
    {E1_DUPLICIT_KEY, "duplicate key '%s'"},
    {E1_KEY_NOT_FOUND, "key '%s' not found"},
    {E1_FORMAT_DUPLICIT_OPTION, "option '%s' specified twice"},
    {E1_FORMAT_OPTION_CHAR, "option '%s' must be a single character"},
    {E1_FORMAT_DUPLICIT_NAME, "duplicate decimal-format name '%s'"},
    {E1_FORMAT_NOT_FOUND, "decimal-format '%s' not defined"},
    {E_FORMAT_INVALID, "invalid format string"},
    {E1_NUMBER_LEVEL, "value '%s' invalid for attribute level of xsl:number"},
    {E1_NUMBER_LETTER_VALUE, "value '%s' invalid for attribute letter-value of xsl:number"},
    {E1_CIRCULAR_INCLUSION, "circular inclusion of '%s'"},
    {E1_MULT_ASSIGNMENT, "conflicting variable bindings '%s'"},
    {E1_EXTRA_COLON, "name '%s' contains two colons"},
    {E_ELEM_MUST_EMPTY, "XSL element '%s' must be empty"},
    {E_ELEM_CONT_TEXT_OR_LRE, "XSL element '%s' can only contain XSL elements"},
    {E_ELEM_CONTAINS_ELEM, "XSL element '%s' cannot contain element '%s' at this point"},
    {E_ELEM_MUST_BE_PCDATA, "XSL element '%s' must contain PCDATA only"},
    {E1_ELEM_TOPLEVEL, "XSL element '%s' can only be used at the top level"},
    {E_BAD_ELEM_CONTENT, "bad element content"},
    {E1_UNSUPPORTED_SCHEME, "unsupported URI scheme '%s'"},
    {E1_URI_OPEN, "could not open document '%s'"},
    {E1_URI_CLOSE, "could not close document '%s'"},
    {E1_URI_READ, "cannot read from URI '%s'"},
    {E1_URI_WRITE, "cannot write to URI '%s'"},
    {E1_ARG_NOT_FOUND,"'arg:%s' not found"},
    {E1_DUPLICATE_ARG, "duplicate argument '%s'"},
    {E1_ATTR_YES_NO, "attribute '%s' may only have a yes/no value"},
    {E1_ATTRIBUTE_TOO_LATE, "attribute '%s' created after a child has been added"},
    {E1_ATTRIBUTE_OUTSIDE, "attribute '%s' created with no parent element"},
    {E1_BAD_CHAR_IN_ENC, "character '%s' is illegal in the output encoding"},
    {E1_BAD_CHAR, "illegal character for encoding '%s'"},
    {E1_PI_TARGET, "invalid processing instruction target '%s'"},
    {E_ELEM_IN_COMMENT_PI, "element created inside a comment, processing instruction or attribute"},
    {E1_INVALID_HLR_TYPE, "invalid handler type '%s'"},
    {E1_XSL_MESSAGE_TERM, "xsl:message (%s) - terminating"},
    {E1_UNKNOWN_ENC, "unknown encoding '%s'"},
    
    {E2_SDOM, "DOM exception %s: %s"},
    {E_NONE, ""},

    {E_EX_NAMESPACE_UNKNOWN, "excluded prefix '%s' is not bound to any namespace"},
    {E_INVALID_DATA_IN_PI, "invalid seqence in PI data"},

    {E_UNSUPPORTED_EXELEMENT, "extension element '%s' not supported"},
    {E_ATTR_MISSING, "required attribute '%s' is missing"},
    {E_JS_EVAL_ERROR, "error evaluating JavaScript (line: %s; msg: %s)"},

    {E_DUPLICIT_ATTRIBUTE, "duplicate attribute '%s'"},
    {E_DUPLICIT_OUTDOC, "duplicate output document uri '%s'"},

    //
    // warnings
    //
    
    {W_UNSUPP_LANG, "function lang() not supported, returning false"},
    {W_NO_STYLESHEET, "the included or imported stylesheet '%s' contains "\
        "no xsl:stylesheet or xsl:transform element"},
    {W1_HLR_REGISTERED, "conflicting registration of a %s handler ignored"},
    {W1_HLR_NOT_REGISTERED, "cannot unregister %s handler, none registered"},
    {W1_OLD_NS_USED, "the obsolete XSLT namespace URI '%s' not recognized"},
    {W1_XSL_NOT_XSL, "namespace URI '%s' bound to 'xsl' is not the current XSLT URI"},
    {W1_OUTPUT_ATTR, "conflicting attribute '%s' on xsl:output, using last"},
    {W_DISABLE_OUTPUT_ESC, "output escaping cannot be disabled on a non-text node"},
    {W1_UNSUPP_OUT_ENCODING, "output encoding '%s' not supported, using 'UTF-8'"},
    {W1_UNSUPP_XSL, "ignoring unsupported XSL instruction '%s'"},
    {W1_XSL_MESSAGE_NOTERM, "xsl:message (%s)"},
    {W1_SORT_DATA_TYPE, "unknown data type '%s', sorting as text"},
    {W1_SORT_ORDER, "invalid sort order '%s', using ascending sort"},
    {W1_SORT_CASE_ORDER, "invalid case order '%s', using default"},
    {W1_LANG, "unsupported language '%s'"},
    {W_NO_WCSXFRM, "international sorting unavailable on this system - sorting naively"},
    {W_BAD_START, "document starts with unrecognized sequence"},
    {W_NUMBER_GROUPING, "no valid value of attribute grouping-size or grouping-separator of xsl:number -- ignoring"}, 
    {W_NUMBER_NOT_POSITIVE, "number is not positive -- using absolute value or 1"}, 
    {W2_ATTSET_REDEF, "attribute '%s' redefined in attribute set '%s'"},
    {W1_ALIAS_REDEF, "alias '%s' redefinition"},
    {E_INVALID_QNAME, "'%s' is not a valid QName"},
    {E_INVALID_NCNAME, "'%s' is not a valid NCName"},
    {E_DOC_FRAGMENT, "Fragment identifiers are not supported in the document() function (uri: '%s')"},
    {E_INVALID_OPERAND_TYPE, "Invalid operand type in comparison"},
    {E1_ATTRIBUTE_MISPLACED, "attribute '%s' misplaced"},
    {W_NONE, ""},
    
    //
    // log messages
    //
    
    {L_START, "\n--------starting processing: %s"},
    {L_STOP, "\n--------stopping: %s"},
    {L1_PARSING, "Parsing '%s'..."},
    {L1_PARSE_DONE, "Parse done in %s seconds"},
    {L1_EXECUTING, "Executing stylesheet '%s'..."},
    {L1_EXECUTION_DONE, "Execution done in %s seconds"},
//    {L1_OUTPUTTING, "Outputting the result using '%s' method..."},
//    {L1_OUTPUT_DONE, "Output done in %s seconds"},
    {L1_READING_EXT_ENTITY, "Parsing an external entity from '%s'"},
    {L2_DISP_ARENA, "Destroying the arena: %s B asked, %s B allocated"},
    {L2_DISP_HASH, "Destroying the hash table: %s items in %s buckets"},
    {L2_KEY_ADDED, "%s items in key '%s'"},
    {L2_SUBDOC, "creating subsidiary document '%s' with base '%s'"},
    {L2_SUBDOC_BASE, "defaulting base uri for '%s' to '%s'"},
    {L2_SUBDOC_STARTED, "catching output for uri '%s' in the '%s' mode"},
    {L_JS_LOG, "js-debug: %s"},
    {L_SHEET_STRUCTURE, "--- stylesheet structure"},
    {L_SHEET_ITEM, "-- %s"},
    {L_NONE, ""},
    //debugger specials
    {DBG_BREAK_PROCESSOR, "interrupted in debugger"},
    //
    {MSG_ERROR, "Error"},
    {MSG_WARNING, "Warning"},
    {MSG_LOG,"Log message"},

    {MSG_LAST, ""}
};


SabMsg* GetMessage(MsgCode e)
{
    SabMsg *p = SablotMessages;
    while ((p -> code != e) && (p -> code != MSG_LAST))
        p++;
    return p;
}
