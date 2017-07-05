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

#ifndef ErrorHIncl
#define ErrorHIncl

// GP: clean

/*
#define Err(situation, code)   { if (situation) { situation->error(code, (char*) NULL, (char*) NULL); return NOT_OK; } }
#define Err1(situation, code, arg1) { if (situation) { situation->error(code, arg1, (char*) NULL); return NOT_OK; } }
#define Err2(situation, code, arg1, arg2) { if (situation) { situation->error(code, arg1, arg2); return NOT_OK; } }
*/

#define Err(sit, code) \
{ report(sit, MT_ERROR, code, (char*)NULL, (char*)NULL); return NOT_OK; }
#define Err1(sit, code, arg1) \
{ report(sit, MT_ERROR, code, arg1, (char*)NULL); return NOT_OK; }
#define Err2(sit, code, arg1, arg2) \
{ report(sit, MT_ERROR, code, arg1, arg2); return NOT_OK; }

#define ErrT(obj, sit, code) \
{ obj -> report(sit, MT_ERROR, code, (char*)NULL, (char*)NULL); return NOT_OK; }
#define Err1T(obj, sit, code, arg1) \
{ obj -> report(sit, MT_ERROR, code, arg1, (char*)NULL); return NOT_OK; }
#define Err2T(obj, sit, code, arg1, arg2) \
{ obj -> report(sit, MT_ERROR, code, arg1, arg2); return NOT_OK; }

#define E(statement) {if (statement) return NOT_OK;}
#define E_DO(statement, onExit) {if (statement) {{onExit;}; return NOT_OK;}}

// the memory allocation checking macro
// use as M( newInstance = new SomeClass );
#define M( sit, ptr ) {if (!(ptr)) Err(sit, E_MEMORY);}
#define MT( obj, sit, ptr ) {if (!(ptr)) ErrT(obj, sit, E_MEMORY);}

#define Warn(sit, code) \
report(sit, MT_WARN, code, (char*)NULL, (char*)NULL);
#define Warn1(sit, code, arg1) \
report(sit, MT_WARN, code, arg1, (char*)NULL);
#define Warn2(sit, code, arg1, arg2) \
report(sit, MT_WARN, code, arg1, arg2);
#define Log(sit, code) \
report(sit, MT_LOG, code, (char*)NULL, (char*)NULL);
#define Log1(sit, code, arg1) \
report(sit, MT_LOG, code, arg1, (char*)NULL);
#define Log2(sit, code, arg1, arg2) \
report(sit, MT_LOG, code, arg1, arg2);

enum MsgCode
{
        
    //
    // errors
    //
	    
    // no error:
    E_OK,
    // generic error
    E_NOT_OK,
	
    // XML parsing

    // expat parsing error
    E_XML,
    // bad char for encoding '...' 
    E1_BAD_CHAR,
	
    // resource allocation
	
    // file open problem
    E_FILE_OPEN,
    // not enough memory to open processor
    E_MEMORY,

    // XSLT parsing
		        
    // unsupported XSL instruction
    E1_UNSUPP_XSL,
    // nonexistent XSL instruction
    ET_BAD_XSL,
    // missing required attribute
    ET_REQ_ATTR,
    // unexpected attribute
    ET_BAD_ATTR,
    // bad axis
    E1_UNKNOWN_AXIS,
    // general expression syntax error
    ET_EXPR_SYNTAX,
    // number in bad format
    ET_BAD_NUMBER,
    // illegal variable name
    ET_BAD_VAR,
    // missing right delimiter of a literal
    ET_INFINITE_LITERAL,
    // '(' expected
    ET_LPAREN_EXP,
    // ')' expected
    ET_RPAREN_EXP,
    // '(' or '[' expected
    ET_LPARCKET_EXP,
    // ']' expected
    ET_RBRACKET_EXP,
    // pattern empty
    ET_EMPTY_PATT,
    // token not recognized
    ET_BAD_TOKEN,
    // patterns allow only several axes
    E_BAD_AXIS_IN_PATTERN,
    // invalid pattern
    E_BAD_PATTERN,
    // variable in match att
    E_VAR_IN_MATCH,
    // invalid namespace prefix
    ET_BAD_PREFIX,
    // name contains 2 colons
    E1_EXTRA_COLON,
    // this XSLT element must be empty
    E_ELEM_MUST_EMPTY,
    // element can only contain XSLT elements
    E_ELEM_CONT_TEXT_OR_LRE,
    // element cannot contain this element at this point
    E_ELEM_CONTAINS_ELEM,
    //element must contain PCDATA only
    E_ELEM_MUST_BE_PCDATA,
    // element can only be used at top level
    E1_ELEM_TOPLEVEL,
    // bad element content
    E_BAD_ELEM_CONTENT,
	
    // XSLT stylesheet compilation
	
    // duplicate rule name
    ET_DUPLICATE_RULE_NAME,
    //duplicate attribute set name
    ET_DUPLICATE_ASET_NAME,
    E1_NONEX_ASET_NAME,
    E1_CIRCULAR_ASET_REF, 
    // name contains 2 colons
    // E1_EXTRA_COLON,   (above)
    // number in bad format
    // ET_BAD_NUMBER,    (above)
    // duplicate key name
    E1_DUPLICIT_KEY,
    // non-existent key name
    E1_KEY_NOT_FOUND,

    // decimal-format related:

    // option specified twice
    E1_FORMAT_DUPLICIT_OPTION,

    // option value must be char
    E1_FORMAT_OPTION_CHAR,

    // decimal-format name conflict
    E1_FORMAT_DUPLICIT_NAME,

    // decimal-format not defined
    E1_FORMAT_NOT_FOUND,

    // invalid format string
    E_FORMAT_INVALID,

    // xsl:number related:
    // invalid value of @level:
    E1_NUMBER_LEVEL,
    // invalid value of @letter-value:
    E1_NUMBER_LETTER_VALUE,

    // circular include/import/ext entity ref
    E1_CIRCULAR_INCLUSION,
	
    // XSLT execution
		
    // variable not found
    E1_VAR_NOT_FOUND,
    // circular var reference
    E1_VAR_CIRCULAR_REF,
    // expression is not a node set
    ET_CONTEXT_FOR_BAD_EXPR,
    // invalid # of arguments
    ET_BAD_ARGS_N,
    // illegal function argument type
    ET_BAD_ARG_TYPE,
    // called unsupported function
    ET_FUNC_NOT_SUPPORTED,
    // invalid namespace prefix
    // ET_BAD_PREFIX,  (above)
    // called non-existent rule
    E1_RULE_NOT_FOUND,
    // conflicting variable bindings (name)
    E1_MULT_ASSIGNMENT,
    // name contains 2 colons
    // E1_EXTRA_COLON, (above)
    // must be a yes/no attribute
    E1_ATTR_YES_NO,
    // attribute created after a child was added
    E1_ATTRIBUTE_TOO_LATE,
    // attribute with no parent
    E1_ATTRIBUTE_OUTSIDE,
    // character '...' is illegal in this encoding (on output)
    E1_BAD_CHAR_IN_ENC,
    // invalid processing instruction target
    E1_PI_TARGET,
    // element created inside a comment or PI
    E_ELEM_IN_COMMENT_PI,
    // terminating xsl:message
    E1_XSL_MESSAGE_TERM,
	
    // hard to file
	
    // can't open document
    E1_URI_OPEN,
    // can't close document
    E1_URI_CLOSE,
    // can't read from document
    E1_URI_READ,
    // can't write to document
    E1_URI_WRITE,
    // named buffer (arg://...) not found
    E1_ARG_NOT_FOUND,
    // duplicate named buffer
    E1_DUPLICATE_ARG,
    // unsupported URI scheme
    E1_UNSUPPORTED_SCHEME,
    // invalid handler type
    E1_INVALID_HLR_TYPE,
    // unknown encoding
    E1_UNKNOWN_ENC,	    
	
    // DOM exception
	
    E2_SDOM,
	    
    E_NONE,

    //namespace exclusions
    E_EX_NAMESPACE_UNKNOWN,

    // ?> in PI data
    E_INVALID_DATA_IN_PI,
	
    //extension elements
    E_UNSUPPORTED_EXELEMENT,
    E_ATTR_MISSING,
    E_JS_EVAL_ERROR,

    E_DUPLICIT_ATTRIBUTE,
    E_DUPLICIT_OUTDOC,

    //
    // warnings
    //
	
    // lang() not supported, returning false
    W_UNSUPP_LANG,
    // included/importe stylesheet is no stylesheet
    W_NO_STYLESHEET,
	
    //  scheme handler warnings
	
    // conflicting handler registration
    W1_HLR_REGISTERED,
    // handler not registered, can't unregister
    W1_HLR_NOT_REGISTERED,
    // obsolete XSLT namespace
    W1_OLD_NS_USED, 
    // xsl prefix bound to non-XSLT URI
    W1_XSL_NOT_XSL, 
    // conflicting attributes on xsl:output
    W1_OUTPUT_ATTR,
    // can't disable output escaping on non-text
    W_DISABLE_OUTPUT_ESC,
    // unsupported output encoding
    W1_UNSUPP_OUT_ENCODING,
    // unsupported XSL instruction
    W1_UNSUPP_XSL,
    // non-terminating xsl:message
    W1_XSL_MESSAGE_NOTERM,
    // unknown data type on xsl:sort
    W1_SORT_DATA_TYPE,
    // invlaid sort order on xsl:sort
    W1_SORT_ORDER,
    // invalid case order on xsl:sort
    W1_SORT_CASE_ORDER,
    // unsupported language on xsl:sort
    W1_LANG,
    // wcsxfrm unavailable
    W_NO_WCSXFRM,
    // document starts with unrecognizable sequence
    W_BAD_START,
    // no valid value of @grouping-size or @grouping-seoarator on xsl:number
    W_NUMBER_GROUPING,
    // a non-positive number in xsl:number
    W_NUMBER_NOT_POSITIVE,
    // attribute set redefinition
    W2_ATTSET_REDEF,
    // alias redefinition
    W1_ALIAS_REDEF,
    //invalid xml names
    E_INVALID_QNAME,
    E_INVALID_NCNAME,
    //doc fragment
    E_DOC_FRAGMENT,
    //invalid operand to comparison (usually EX_EXTERNAL)
    E_INVALID_OPERAND_TYPE,
    //attribute emitted in a bad place
    E1_ATTRIBUTE_MISPLACED,

    W_NONE,

    //
    // log messages
    //
	    
    // starting processing
    L_START,
    // stopping processing
    L_STOP,
    // starting parse
    L1_PARSING,
    // parse finished
    L1_PARSE_DONE,
    // starting execution
    L1_EXECUTING,
    // execution finished
    L1_EXECUTION_DONE,
    // parsing external entity
    L1_READING_EXT_ENTITY,
    // destroying the arena
    L2_DISP_ARENA,
    // destroying the hash table
    L2_DISP_HASH,
    // key added
    L2_KEY_ADDED,
    //subsidiary document
    L2_SUBDOC,
    L2_SUBDOC_BASE,
    L2_SUBDOC_STARTED,
    //Javascript Logging
    L_JS_LOG,
    //stylesheet structure
    L_SHEET_STRUCTURE,
    L_SHEET_ITEM,

    L_NONE,

    //used for debugger
    DBG_BREAK_PROCESSOR,

    MSG_ERROR = 0x4000,
    MSG_WARNING,
    MSG_LOG,
    
    //terminal
    MSG_LAST
};

struct SabMsg
{
    MsgCode code;
    const char *text;
};

extern SabMsg* GetMessage(MsgCode e);

#endif //ifndef ErrorHIncl
