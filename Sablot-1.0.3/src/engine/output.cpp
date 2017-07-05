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

//
//  output.cpp
//

#include "output.h"
#include "uri.h"
#include "proc.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef SABLOT_DEBUGGER
#include "debugger.h"
#endif

#include "guard.h"

// GP: clean

#define NONEMPTY_ELEMENT FALSE
#define EMPTY_ELEMENT TRUE


enum OutputterFlags
{
    HTML_ELEMENT = 1,
    HTML_SCRIPT_OR_STYLE = 2,
    CDATA_SECTION = 4
};

#define IF_SAX1( FUNC )\
    { if (mySAXHandler)\
        { mySAXHandler -> FUNC(mySAXUserData, S.getProcessor()); }}
#define IF_SAX2( FUNC, ARG1 )\
    { if (mySAXHandler)\
        { mySAXHandler -> FUNC(mySAXUserData, S.getProcessor(), (ARG1)); }}
#define IF_SAX3( FUNC, ARG1, ARG2 )\
    { if (mySAXHandler)\
        { mySAXHandler -> FUNC(mySAXUserData, S.getProcessor(), (ARG1), (ARG2)); }}

//macros on internal handler
#define IF_INT_SAX4( FUNC, ARG1, ARG2, ARG3 )\
    { if (mySAXHandler)\
        { ((SAXHandlerInternal*)mySAXHandler) -> FUNC(mySAXUserData, S.getProcessor(), (ARG1), (ARG2), (ARG3)); }}

#define IF_PH1( FUNC )\
    { if (physical)\
        { physical -> FUNC(S); }}
#define IF_PH2( FUNC, ARG1)\
    { if (physical)\
        { physical -> FUNC(S, ARG1); }}
#define IF_PH3( FUNC, ARG1, ARG2 )\
    { if (physical)\
        { physical -> FUNC(S, (ARG1), (ARG2)); }}

// STRING_ITEMS_COUNT (output.h) must be in sync with the size
// of the following table:

XSL_ATT outputStringAtts[STRING_ITEMS_COUNT + 1] = {
    XSLA_VERSION, XSLA_ENCODING, XSLA_OMIT_XML_DECL, XSLA_STANDALONE,
    XSLA_DOCTYPE_PUBLIC, XSLA_DOCTYPE_SYSTEM, XSLA_INDENT, XSLA_MEDIA_TYPE,
    XSLA_NONE
};

const char* theEmptyHTML40Tags[] = 
{
    "area", "base", "basefont", "br", "col", "frame", "hr", "img",
    "input", "isindex", "link", "meta", "param", NULL
};

const char* theURI_HTMLAtts[] = 
{
    "action",       // FORM
    "archive",      // OBJECT
    "background",   // BODY
    "cite",         // BLOCKQUOTE, Q, DEL, INS
    "classid",      // OBJECT
    "codebase",     // OBJECT, APPLET
    "data",         // OBJECT
    "href",         // A, AREA, LINK, BASE
    "longdesc",     // IMG, FRAME, IFRAME
    "profile",      // HEAD
    "src",          // SCRIPT, INPUT, FRAME, IFRAME, IMG
    "usemap",       // IMG, INPUT, OBJECT
    NULL
};

const char* theHTMLBooleanAtts[] =
{
  "checked",
  "compact",
  "declare",
  "defer",
  "disabled",
  "ismap",
  "multiple",
  "nohref",
  "noresize",
  "noshade",
  "nowrap",
  "readonly",
  "selected",
  NULL
};

const char* theHTMLNoEscapeTags[] = 
{
  "script",
  "style",
  NULL
};

// tags new lines should not be inserted after by indent
const char* theNoEolHTMLTags[] = 
{
    "img", "span", NULL
};

Bool isEmptyHTMLTag(const Str& name)
{
    int ndx = lookupNoCase(name, theEmptyHTML40Tags);
    // return TRUE iff found
    return !!theEmptyHTML40Tags[ndx];
}

Bool isHTMLNoEscapeTag(const Str& name)
{
    int ndx = lookupNoCase(name, theHTMLNoEscapeTags);
    return !!theHTMLNoEscapeTags[ndx];
}

Bool isBooleanHTMLAtt(const Str& name)
{
  int ndx = lookupNoCase(name, theHTMLBooleanAtts);
 return !!theHTMLBooleanAtts[ndx];
}

//  this should be improved by checking the name of the parent element
//  see the element names in theURI_HTMLAtts
Bool isURI_HTMLAtt(const Str& name)
{
    int ndx = lookupNoCase(name, theURI_HTMLAtts);
    // return TRUE iff found
    return !!theURI_HTMLAtts[ndx];
}

Bool isNoEolHTMLTag(const Str& name)
{
    int ndx = lookupNoCase(name, theNoEolHTMLTags);
    // return TRUE iff found
    return !!theNoEolHTMLTags[ndx];
}

//
//  StrPrec and EQNamePrec implementation
//


// returns -1 if oldPrec wins, +1 if newPrec wins, 0 if equal (usually error)
int cmpPrecedences(int oldPrec, int newPrec)
{
    if (oldPrec == OUTPUT_PRECEDENCE_UNSPECIFIED ||
	newPrec == OUTPUT_PRECEDENCE_STRONGEST ||
	newPrec >= 0 && newPrec < oldPrec)
	return 1;
    if (newPrec >= 0 && oldPrec == newPrec)
	return 0;
    return -1;
}

Bool StrPrec::set(const Str& newString, int newPrecedence)
{
    int cmp = cmpPrecedences(precedence, newPrecedence);
    // if new wins or there's a draw, set the value
    if (cmp >= 0)
    {
	string = newString;
	precedence = newPrecedence;
    };
    // if cmp == 0, return TRUE
    return cmp ? FALSE : TRUE;
}

Bool EQNamePrec::set(const EQName& newName, int newPrecedence)
{
    int cmp = cmpPrecedences(precedence, newPrecedence);
    // if new wins or there's a draw, set the value
    if (cmp >= 0)
    {
	name = newName;
	precedence = newPrecedence;
    };
    // if cmp == 0, return TRUE
    return cmp ? FALSE : TRUE;
}

//
//  OutputDefinition
//

Bool checkYesNo(const Str& what)
{
    return(what == (const char*) "yes" || what == (const char*) "no");
}

OutputDefinition::OutputDefinition()
: method()
{
}

OutputDefinition::~OutputDefinition()
{
    cdataElems.freeall(FALSE);
}

int lookupAttCode(XSL_ATT* table, XSL_ATT what)
{
    int i;
    for (i = 0; table[i] != XSLA_NONE && table[i] != what; i++) {};
    return (table[i] == XSLA_NONE ? -1 : i);
}

eFlag OutputDefinition::setItemStr(Sit S, XSL_ATT itemId, const Str& value, Vertex *caller, int precedence)
{
    if (caller)
	precedence = caller -> getImportPrecedence();
    switch(itemId)
    {
    case XSLA_OMIT_XML_DECL:
    case XSLA_STANDALONE:
    case XSLA_INDENT:
    {
	if (!checkYesNo(value))
	{
	    S.setCurrVDoc(caller);
	    Err1(S, E1_ATTR_YES_NO, xslAttNames[itemId]);
	}
    }; break;
    };
    int index = lookupAttCode(outputStringAtts, itemId);
    sabassert(index >= 0);
    if (stringItems[index].set(value, precedence))
    {
	S.setCurrVDoc(caller);
	Warn1(S, W1_OUTPUT_ATTR, xslAttNames[itemId]);
    }
    return OK;
}

eFlag OutputDefinition::setItemEQName(Sit S, XSL_ATT itemId, const EQName& value, Vertex *caller, int precedence) 
{
    if (caller)
	precedence = caller -> getImportPrecedence();
    switch(itemId)
    {
    case XSLA_CDATA_SECT_ELEMS:
        cdataElems.append(new EQName(value)); break;
    default: 
    {
        sabassert(itemId == XSLA_METHOD);
	if (method.set(value, precedence))
	{
	    S.setCurrVDoc(caller);
	    Warn1(S, W1_OUTPUT_ATTR, xslAttNames[itemId]);
	}
    }
    }
    return OK;
}

const Str& OutputDefinition::getValueStr(XSL_ATT itemId) const
{
    int index = lookupAttCode(outputStringAtts, itemId);
    sabassert(index >= 0);
    return stringItems[index].get();
}

const EQName& OutputDefinition::getValueEQName(XSL_ATT itemId) const
{
    sabassert(itemId == XSLA_METHOD);
    return method.get();
};

Bool OutputDefinition::askEQNameList(XSL_ATT itemId, const EQName &what) const
{
    sabassert(itemId == XSLA_CDATA_SECT_ELEMS);
    return (cdataElems.find(what) != NULL);
}

OutputMethod OutputDefinition::getMethod() const
{
    const Str& method_ = getValueEQName(XSLA_METHOD).getLocal();
    if (method_ == (const char*) "html")
        return OUTPUT_HTML;
    else if (method_ == (const char*) "text")
        return OUTPUT_TEXT;
    else if (method_ == (const char*) "xml")
        return OUTPUT_XML;
    else if (method_ == (const char*) "xhtml")
        return OUTPUT_XHTML;
    else return OUTPUT_UNKNOWN;
}

const Str& OutputDefinition::getEncoding() const
{
    return getValueStr(XSLA_ENCODING);
}

Bool OutputDefinition::getIndent() const
{
    const Str& indent_ = getValueStr(XSLA_INDENT);
    return (indent_ == (char*)"yes");        
}

eFlag OutputDefinition::setDefaults(Sit S)
{
    OutputMethod meth = getMethod();
    sabassert( meth != OUTPUT_UNKNOWN );
    char strYes[] = "yes", strNo[] = "no";
    E( setItemStr(S, XSLA_ENCODING, "UTF-8", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
    switch(meth)
    {
    case OUTPUT_XML:
        {
            E( setItemStr(S, XSLA_VERSION, "1.0", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_INDENT, strNo, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_MEDIA_TYPE, "text/xml", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_OMIT_XML_DECL, strNo, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
        }; break;
    case OUTPUT_HTML:
        {
            E( setItemStr(S, XSLA_VERSION, "4.0", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_INDENT, strYes, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_MEDIA_TYPE, "text/html", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_OMIT_XML_DECL, strYes, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
        }; break;
    case OUTPUT_TEXT:
        {
            E( setItemStr(S, XSLA_INDENT, strNo, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_MEDIA_TYPE, "text/plain", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_OMIT_XML_DECL, strYes, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
        }; break;
    case OUTPUT_XHTML:
        {
            E( setItemStr(S, XSLA_VERSION, "1.0", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_INDENT, strYes, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_MEDIA_TYPE, "text/html", NULL, OUTPUT_PRECEDENCE_WEAKEST) );
            E( setItemStr(S, XSLA_OMIT_XML_DECL, strYes, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
        }; break;
    }
    return OK;
}

void OutputDefinition::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type, code, arg1, arg2);
}



//
//
//  PhysicalOutputLayerObj
//
//

#define sendStrEsc(SIT, STRING,ESCAPING) sendOut(SIT, (STRING), (STRING.length()), (ESCAPING))
#define sendStr(SIT, STRING)     sendStrEsc(SIT, (STRING), ESCAPING_NONE)
#define sendLit(SIT, LITERAL)    sendOut(SIT, (LITERAL), sizeof(LITERAL) - 1, ESCAPING_NONE)
// S is the situation (existing in each context indentEOL is called in) 
#define indentEOL() {sendLit(S, "\n");}
#define indentSpace() {for (int i__ = 0; i__ < level; i__++) sendLit(S, "  ");}
#define indentFull() {if (indent && after_markup) {indentEOL(); indentSpace();}}
#define ignoreTextMethod() {if (method == OUTPUT_TEXT) return OK; }

PhysicalOutputLayerObj::PhysicalOutputLayerObj(CDesc encodingCD_)
{
    curr = 0;
    encodingCD = encodingCD_;
    indent = FALSE;
    after_markup = FALSE;  //pc
    level = 0;  //pc
    defaultNSWas = false;
}

PhysicalOutputLayerObj::~PhysicalOutputLayerObj()
{
}

eFlag PhysicalOutputLayerObj::setOptions(Sit S, 
    DataLine *targetDataLine_, OutputDefinition *outDef_)
{
    targetDataLine = targetDataLine_;
    outDef = outDef_;
    method = outDef -> getMethod();
    indent = outDef -> getIndent();
    //enc = outDef -> getEncoding();
    encoding = outDef -> getEncoding();
    return OK;
}

eFlag PhysicalOutputLayerObj::outputTrailingNewline (Sit S)
{
  switch (method) {
  case OUTPUT_HTML:
  case OUTPUT_XML:
  case OUTPUT_XHTML:
    indentEOL();
    break;
  }
  return OK;
}

eFlag PhysicalOutputLayerObj::outputDone(Sit S)
{
  E( flushBuffer(S) );
  return OK;
}

Str _localName(Str fullname)
{
  char *full = (char*)fullname;
  char *colon = strchr(fullname, ':');
  if (colon)
    return colon + 1;
  else 
    return full;
}

eFlag PhysicalOutputLayerObj::outputElementStart(Sit S, 
    const Str& name,
    const NamespaceStack& namespaces, const int namespace_index,
    const StrStrList& atts, 
    Bool isEmpty)
{
  //exit for text output method!!!
  ignoreTextMethod();

    // begin output of start tag: output element name
    if (!isNoEolHTMLTag(name) || (method != OUTPUT_HTML && method != OUTPUT_XHTML)) 
      indentFull();

    sendLit(S, "<");
    E( sendStr(S, name) );

    // output namespace declarations

    int i;
    const Str* prefix;
    for (i = namespace_index; i < namespaces.number(); i++)
    {
      NamespaceStackObj * nm = namespaces[i];
      prefix = &(nm -> prefix);
      //prefix = &(namespaces[i] -> prefix); 
      //if (!namespaces.isHidden(*prefix))

      //swallow hidden namespaces as well as the empty default
      //namespace, if no default namespace was output recently
      if (! nm -> hidden &&
	  (!(prefix -> isEmpty()) || !(nm -> uri.isEmpty()) || defaultNSWas))
	{
	  defaultNSWas = defaultNSWas || prefix -> isEmpty();

          sendLit(S, " xmlns");
          if (!prefix -> isEmpty())
          {
             sendLit(S, ":");
             E( sendStr(S, *prefix) );
          }
          sendLit(S, "=\"");
          E( sendStrEsc(S, nm -> uri, 
                        method == OUTPUT_HTML || method == OUTPUT_XHTML ?
                        ESCAPING_HTML_URI : ESCAPING_URI) );
          sendLit(S, "\"");
       };
    };

    // output attributes

    for (i = 0; i < atts.number(); i++)
    {
        sendLit(S, " ");
	/*
        const EQName& attQName = atts[i] -> key;
        if (attQName.hasPrefix())
        {
            E( sendStr(S, attQName.getPrefix()) );
            sendLit(S, ":");
        };
        const Str& localAttName = atts[i] -> key.getLocal();
        E( sendStr(S, localAttName) );
	*/
	sendOut(S, (char*)atts[i] -> key, atts[i] -> key.length(), 
		ESCAPING_NONE);

	//boolean atts for html
	if (method == OUTPUT_HTML && isBooleanHTMLAtt(atts[i]->key))
	    continue;

        sendLit(S, "=\"");

	/*
        EscMode escapingMode = 
            ((method == OUTPUT_HTML || method == OUTPUT_XHTML) ? 
	     ESCAPING_HTML_ATTR : ESCAPING_ATTR);
        if ((method == OUTPUT_HTML || method == OUTPUT_XHTML) && 
	    isURI_HTMLAtt(_localName(atts[i]->key)))
	  {
	    escapingMode = ESCAPING_HTML_URI;
	  }
	*/
	EscMode escapingMode = ESCAPING_ATTR;
	if (method == OUTPUT_HTML || method == OUTPUT_XHTML) 
	  {
	    if ( strchr(name, ':') ) 
	      {
		escapingMode = ESCAPING_ATTR;
	      }
	    else if ( isURI_HTMLAtt(_localName(atts[i]->key)) )
	      {
		escapingMode = ESCAPING_HTML_URI;
	      }
	    else 
	      {
		if (method == OUTPUT_HTML) 
		  escapingMode = ESCAPING_HTML_ATTR;
	      }
	  }

        E( sendStrEsc(S, atts[i] -> value, escapingMode));
        sendLit(S, "\"");
    };

    // close the tag

    after_markup = TRUE;  //pc

    if (!isEmpty)
    {
      sendLit(S, ">");
      level++;  //pc
    }
    else
    {
        if (method == OUTPUT_HTML || method == OUTPUT_XHTML)
	{
            if (!isEmptyHTMLTag(name))
            {
                sendLit(S, "></");
                sendStr(S, name);
                sendLit(S, ">");
            }
            else if (method == OUTPUT_HTML)
                sendLit(S, ">");
            else
                sendLit(S, " />");

	    if (isNoEolHTMLTag(name)) 
	        after_markup = FALSE;
        }
        else
	    sendLit(S, "/>");
    };
    return OK;
}



eFlag PhysicalOutputLayerObj::outputElementEnd(Sit S, 
    const Str& name, Bool isEmpty)
{
  //exit for text output method!!!
  ignoreTextMethod();

    if (!isEmpty)
    {
	level--;  //pc
        indentFull();
        sendLit(S, "</");
        E( sendStr(S, name) );
        sendLit(S, ">");
	if (!isNoEolHTMLTag(name)) after_markup = TRUE;  //pc
    };
    return OK;
}


eFlag PhysicalOutputLayerObj::outputComment(Sit S, const Str& contents)
{
  //exit for text output method!!!
  ignoreTextMethod();

    indentFull();
    sendLit(S, "<!--");
    const char *p = contents, *p_was = p;
    int len = contents.length();
    Bool trailingHyphen = len ? (contents[len - 1] == '-') : FALSE;
    while (*p)
    {
        E( sendOutUntil(S, p, len - (p - p_was), ESCAPING_NONE, "--") );
        if (*p)
        {
            sendLit(S, "- ");
            p++;
        }
    };
    if (trailingHyphen)
        sendLit(S, " ");
    sendLit(S, "-->");
    after_markup = TRUE;  //pc
    return OK;
}


eFlag PhysicalOutputLayerObj::outputCDataSection(Sit S, const Str& contents)
{         
  switch (method) {
  case OUTPUT_TEXT:
    {
      sendLit(S, contents);
    }; break;
  default:
    {
      const char *p = contents, *p_was = p;
      if (!*p)
        return OK;
      indentFull();
      sendLit(S, "<![CDATA[");
      while (*p)
	{
	  E( sendOutUntil(S, p, 
			  contents.length() -(int)(p - p_was), 
			  ESCAPING_NONE, "]]>") );
	  if (*p)
	    {
	      sendLit(S, "]]]]><![CDATA[>");
	      p += 3;
	    }
	};
      sendLit(S, "]]>");
      after_markup = TRUE;  //pc
    }
  }
  return OK;
}

eFlag PhysicalOutputLayerObj::outputPI(Sit S, const Str& target, const Str& data)
{
  //exit for text output method!!!
  ignoreTextMethod();

    indentFull();
    sendLit(S, "<?");
    E( sendStr(S, target) );
    sendLit(S, " ");
    E( sendStr(S, data) );
    if (method == OUTPUT_HTML && !(target == (const char*)"xml"))
        sendLit(S, ">");
    else
        sendLit(S, "?>");
    after_markup = TRUE;  //pc
    return OK;
}

eFlag PhysicalOutputLayerObj::outputDTD(Sit S, 
    const Str& name, const Str& publicId, const Str& systemId)
{
  //exit for text output method!!!
  ignoreTextMethod();

    indentFull();
    sendLit(S, "<!DOCTYPE ");
    switch(method)
    {
    case OUTPUT_XML:
    case OUTPUT_XHTML:
        {
            E( sendStr(S, name) );
            if (!systemId.isEmpty())
            {
                if (!publicId.isEmpty())
                {
                    sendLit(S, " PUBLIC \"");
                    E( sendStrEsc(S, publicId, ESCAPING_NONE) );
                    sendLit(S, "\"");
                }
                else
                    sendLit(S, " SYSTEM");
                sendLit(S, " \"");
                E( sendStrEsc(S, systemId, ESCAPING_URI) );
                sendLit(S, "\"");
            }
        }; break;
    case OUTPUT_HTML:
        {
            sendLit(S, "html");
            if (!publicId.isEmpty())
            {
                sendLit(S, " PUBLIC \"");
                E( sendStrEsc(S, publicId, ESCAPING_NONE) );
                sendLit(S, "\"");
            }
            if (!systemId.isEmpty())
            {
                if (publicId.isEmpty())
                    sendLit(S, " SYSTEM");
                sendLit(S, " \"");
                E( sendStrEsc(S, systemId, ESCAPING_URI) );
                sendLit(S, "\"");
            };
        }; break;
    };
    if (indent)
        sendLit(S, ">");
	else
        sendLit(S, ">\xa");
    after_markup = TRUE;  //pc
    return OK;
}

eFlag PhysicalOutputLayerObj::outputText(Sit S, 
    const Str& contents, Bool disableEsc,
    Bool inHTMLSpecial)
{
    switch(method)
    {
    case OUTPUT_XML:
    case OUTPUT_XHTML:
        {
            E( sendStrEsc(S, contents, (disableEsc || inHTMLSpecial) ? ESCAPING_NONE : ESCAPING_LT_AMP) );
        }; break;
    case OUTPUT_HTML:
        {
            E( sendStrEsc(S, contents, (disableEsc || inHTMLSpecial) ? ESCAPING_NONE : ESCAPING_LT_AMP) );
        }; break;
    case OUTPUT_TEXT:
        {
            E( sendStrEsc(S, contents, ESCAPING_NONE) );
        }; break;
    };
    after_markup = FALSE;  //pc
    return OK;
}


eFlag PhysicalOutputLayerObj::flushBuffer(Sit S)
{
    E( targetDataLine -> save(S, buffer, curr) );
    curr = 0;
    return OK;
}

int PhysicalOutputLayerObj::writeCharacterRef(char* dest, const char *src, EscMode escapeMode)
{
    char *dest_was = dest;
    if (escapeMode == ESCAPING_URI || escapeMode == ESCAPING_HTML_URI)
    {
        int i, iLimit = utf8SingleCharLength(src);
        for (i = 0; i < iLimit; i++)
	  {
            dest += sprintf(dest, "%%%02hhx", (unsigned char)src[i]);
	  }
        return (int)(dest - dest_was);
    }
    else
        return sprintf(dest, "&#%lu;", utf8CharCode(src));
}

// data is assumed null-terminated in the following

eFlag PhysicalOutputLayerObj::sendOutUntil(Sit S, 
    const char *& data, int length,
    EscMode escapeMode, const char* stoppingText)
{
    const char *p = strstr(data, stoppingText);
    int sending = (p ? p - data : length);
    E( sendOut(S, data, sending, escapeMode) );
    data += sending;
    return OK;
}



#define minimum(a,b) (((a) < (b)) ? (a) : (b))

//
// sendOut
//
// This is the place where *all* actual character output and conversion work takes place. The
// individual characters are recoded and written into a buffer. When the buffer is full, it is
// sent to the appropriate dataline.
//

eFlag PhysicalOutputLayerObj::sendOut(Sit S, 
    const char* data, int length,
    EscMode escapeMode)
{
    int count = 0;
    size_t srcCharLen, destCharLen; 
    Bool served;
    char *outbuf;
    size_t outleft, inleft;
    EncResult result;
    
    while (count < length)
    {
        srcCharLen = 1;
        served = FALSE;
        switch(*data)
        {
        case '<':
            {
                switch(escapeMode)
                {
/*  will escape < in URIs as &lt; not the %xx way
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                    {
                        E( sendOut(smallBuf, 
                            writeCharacterRef(smallBuf, data, escapeMode),
                            ESCAPING_NONE) );
                        served = TRUE;
                    }; break;
*/
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                case ESCAPING_ATTR:
                case ESCAPING_LT_AMP:
                    {
                        E( sendLit(S, "&lt;") );
                        served = TRUE;
                    }; break;
                }
            }; break;
        case '>':
            {
                switch(escapeMode)
                {
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                case ESCAPING_ATTR:
                case ESCAPING_LT_AMP:
                    {
		      //now we escape &gt; always to prevent bad
		      //escaping in cdata sections.
		      E( sendLit(S, "&gt;") );
		      served = TRUE;
                    }; break;
                }
            }; break;
        case '&':
            {
                switch(escapeMode)
                {
/*  will escape & in URIs as &amp; not the %xx way
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                    {
                        E( sendOut(smallBuf, 
                            writeCharacterRef(smallBuf, data, escapeMode),
                            ESCAPING_NONE) );
                        served = TRUE;
                    }; break;
*/
                case ESCAPING_HTML_ATTR:
                    {
                        if (data[1] == '{')
                            break;
                    }; // no break
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                case ESCAPING_ATTR:
                case ESCAPING_LT_AMP:
                    {
                        E( sendLit(S, "&amp;") );
                        served = TRUE;
                    }; break;
                }
            }; break;
        case '\"':
            {
                switch(escapeMode)
                {
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                    {
                        E( sendOut(S, smallBuf, 
                            writeCharacterRef(smallBuf, data, escapeMode),
                            ESCAPING_NONE) );
                        served = TRUE;
                    }; break;
                case ESCAPING_HTML_ATTR:
                case ESCAPING_ATTR:
                    {
                        E( sendLit(S, "&quot;") );
                        served = TRUE;
                    }; break;
                };
            }; break;
        case 9:
        case 10:
        case 13:
            {
                switch(escapeMode)
                {
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                case ESCAPING_ATTR:
                case ESCAPING_HTML_ATTR:
                    {
                        E( sendOut(S, smallBuf, 
                            writeCharacterRef(smallBuf, data, escapeMode),
                            ESCAPING_NONE) );
                        served = TRUE;
                    }; break;
                }
            }; break;
        case ' ':
            {
                switch(escapeMode)
                {
                case ESCAPING_URI:
                case ESCAPING_HTML_URI:
                    {
                        E( sendOut(S, smallBuf, 
                            writeCharacterRef(smallBuf, data, escapeMode),
                            ESCAPING_NONE) );
                        served = TRUE;
                    }; break;
                }
            }; break;
	    default:
	        {
		  // escape non-ASCII values in URIs
		  if (*data & 0x80)
		    {
		      switch(escapeMode)
			{
			case ESCAPING_URI:
			case ESCAPING_HTML_URI:
			  {
                            E( sendOut(S, smallBuf, 
				       writeCharacterRef(smallBuf, data, 
							 escapeMode),
				       ESCAPING_NONE) );
                            served = TRUE;
			  }; break;
			}
		    }			        
		}
        };


        if (!served)
        {
            srcCharLen = utf8SingleCharLength(data);
            sabassert(srcCharLen > 0);
            // if encoding differs from utf-8 then recode

#ifdef SABLOT_DEBUGGER
	    if ( debugger )
	      {
		debugger -> printOutput(data, srcCharLen);
	      }
#endif

            if (encodingCD == (void*)-1)
            {
                memcpy(buffer + curr, data, srcCharLen);
                data += srcCharLen;
                curr += srcCharLen;
            }
            else
            {
                // convert
                outbuf = buffer + curr;
                outleft = OUTPUT_BUFFER_SIZE - curr;
                inleft = srcCharLen;
		        // getProcessor must be non-null here since encodingCD is
                S.recoder().conv(S, encodingCD, data, inleft, outbuf, outleft, result);
                // data is shifted automatically, set curr
                curr = outbuf - buffer;
                // check result, write a character code if 
		//couldn't convert (unless ESCAPING_NONE)
                sabassert(result != ENC_EINVAL && result != ENC_E2BIG);
                // FIXME: in NT, must check differently
                if (result == ENC_EILSEQ)
                {
                    // unable to convert
                    destCharLen = writeCharacterRef(smallBuf, data, escapeMode);
                    //if (escapeMode == ESCAPING_NONE)
		    if (method == OUTPUT_TEXT)
                        Err1(S, E1_BAD_CHAR_IN_ENC, smallBuf)
                    else
		      {
                        E( sendOut(S, smallBuf, destCharLen, ESCAPING_NONE) );
                        data += srcCharLen;
                        served = TRUE;
		      }
                }
            }
        }   // if (!served)
        else
        {
            data += srcCharLen;
        }

        // send the buffer if over a threshold
        if (curr > OUTPUT_BUFFER_LIMIT)
            flushBuffer(S);

        count += srcCharLen;
        // curr already got shifted in all cases

    }       // while    
    return OK;
}


eFlag PhysicalOutputLayerObj::setMethodByDefault(Sit S, OutputMethod method_)
{
    EQName q;
    sabassert(method == OUTPUT_UNKNOWN);
    switch( method = method_ )
    {
    case OUTPUT_XML:
        q.setLocal("xml"); break;
    case OUTPUT_HTML:
        q.setLocal("html"); break;
    default:
        sabassert(!"PhysicalOutputLayerObj::setMethod()");
    }
    E( NZ(outDef) -> setItemEQName(S, XSLA_METHOD, q, NULL, OUTPUT_PRECEDENCE_STRONGEST) );
    E( outDef -> setDefaults(S) );
    return OK;
}

eFlag PhysicalOutputLayerObj::close(Sit S) 
{
  DataLine * dl;
  if (dl = getDataLine())
    {
      E( dl -> close(S) );
    }
  return OK;
}

void PhysicalOutputLayerObj::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type, code, arg1, arg2);
}



//
//
//      FrontMatter
//
//

eFlag FrontMatter::appendConstruct(Sit S, 
    FrontMatterKind kind, const Str& string1,
    const Str& string2, Bool disableEsc)
{
    FrontMatterItem* item;

    M( S, item = new FrontMatterItem ); // GP: OK
    item -> kind = kind;
    item -> string1 = string1;
    item -> string2 = string2;
    item -> disableEsc = disableEsc;
    append(item);
    return OK;
};

void FrontMatter::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type, code, arg1, arg2);
}

//
//
//  OutputDocument
//
//

OutputDocument::~OutputDocument()
{
  //we do not delete outputter objects as they are stored with
  //Processor
  // if (outputter) delete outputter;
  if (def) delete def;
}

eFlag OutputDocument::finish(Sit S)
{
  if (state == OUTDOC_ACTIVE)
    {
      E( NZ(outputter) -> eventTrailingNewline(S) );
      E( NZ(outputter) -> eventEndOutput(S, true) );
      setState(OUTDOC_FINISHED);
    }
  return OK;
}

OutputterObj* OutputDocument::setOutputter(OutputterObj* new_)
{
  if (outputter) delete outputter;
  return outputter = new_;
}

//
//
//  OutputterObj
//
//


Str* OutputterObj::nameForSAX(Sit S, const EQName& q)
{
  DStr temp;
  if (mySAXOutputType == SAXOUTPUT_COPY_TREE || 
      mySAXOutputType == SAXOUTPUT_INT_PHYSICAL)
    {
      if (q.getUri().isEmpty())
	return new Str(q.getLocal());
      temp = q.getUri();
      temp += THE_NAMESPACE_SEPARATOR;
      temp += q.getLocal();
      temp += THE_NAMESPACE_SEPARATOR;
      temp += q.getPrefix();
      return new Str(temp);
    }
  else
    {
      Str s;
      q.getname(s);
      return new Str(s);
    }
};

OutputterObj::OutputterObj()
{
    encodingCD = (CDesc) -1;
    outputEscaping = TRUE;
    physical = NULL;
    outDef = NULL;
    mySAXHandler = NULL;
    mySAXOutputType = SAXOUTPUT_NONE;
    method = OUTPUT_UNKNOWN;
    noElementYet = TRUE;
    noHeadYet = TRUE;
    delayedDTD = FALSE;
    // to recognize the first HEAD element
}

OutputterObj::~OutputterObj()
{
    history.freeall(FALSE);
    front.freeall(FALSE);
    currNamespaces.freeall(FALSE);
    cdelete(physical);
}

eFlag OutputterObj::setOptions(Sit S, 
    DataLine *targetDataLine_, OutputDefinition *outDef_)
{
    Str encoding;
    outDef = NZ(outDef_);
    method = outDef -> getMethod(); //_PH_
    if (method != OUTPUT_UNKNOWN)
        E( outDef -> setDefaults(S) );

    if (S.getProcessor())
      {
        encoding = S.getProcessor() -> getHardEncoding();
	if ( !encoding.isEmpty() )
	  outDef -> setItemStr(S, XSLA_ENCODING, encoding, NULL, 
			       OUTPUT_PRECEDENCE_STRONGEST);
      }
    else
	encoding.empty();
    if (encoding.isEmpty())
        encoding = outDef -> getValueStr(XSLA_ENCODING);
    if (!encoding.isEmpty() && !encoding.eqNoCase("utf-8"))
    {
        // set the conversion descriptor
	if (S.getProcessor())
	{
            E( S.recoder().openFromUTF8(S, 
					encoding, encodingCD) );
	}
	else 
	    encodingCD = (ConvInfo*)-1;
        if (encodingCD == (void*)-1)
        {
            Warn1(S, W1_UNSUPP_OUT_ENCODING, encoding);
            encoding = "UTF-8";
            E( outDef -> setItemStr(S, XSLA_ENCODING, encoding, 
				    NULL, OUTPUT_PRECEDENCE_STRONGEST) );
        }
    }
    else
	if (!encoding.isEmpty())
	    E( outDef -> setItemStr(S, XSLA_ENCODING, encoding, 
				    NULL, OUTPUT_PRECEDENCE_WEAKEST) );

    if (targetDataLine_)
    {
        M( S, physical = new PhysicalOutputLayerObj(encodingCD) );
        // GP: OK, OutputterObj gets disposed of in cleanupAfterRun
        E( physical -> setOptions(S, targetDataLine_, outDef_) ); 
    };

    return OK;
}

eFlag OutputterObj::setOptionsSAX(Sit S, 
    SAXHandler *streaming_, void* userData_, SAXOutputType saxout_)
{
    mySAXHandler = streaming_;
    mySAXUserData = userData_;
    mySAXOutputType = saxout_;
    return OK;
}

eFlag OutputterObj::reportXMLDeclIfMust(Sit S)
{
    if (!physical || method == OUTPUT_UNKNOWN || 
        outDef -> getValueStr(XSLA_OMIT_XML_DECL) == (const char*)"yes")
        return OK;
    DStr declText = "version=\"";
    declText += outDef -> getValueStr(XSLA_VERSION);
    declText += "\" encoding=\"";
    declText += outDef -> getValueStr(XSLA_ENCODING);
    declText += '\"';
    const Str &standaloneText = outDef -> getValueStr(XSLA_STANDALONE);
    if (!standaloneText.isEmpty())
    {
        declText += " standalone=\"";
        declText += standaloneText;
        declText += '\"';
    };
    E( physical -> outputPI(S, Str("xml"), declText) );
    return OK;
}

eFlag OutputterObj::reportDTDIfMust(Sit S, const EQName& docElementName)
// to be called only after the output method is determined
{
    sabassert(method != OUTPUT_TEXT);
    if (!physical)
        return OK;
    const Str& 
        DTSystem = outDef -> getValueStr(XSLA_DOCTYPE_SYSTEM),
        DTPublic = outDef -> getValueStr(XSLA_DOCTYPE_PUBLIC);        
    Bool writeDTD;
    switch(method)
    {
    case OUTPUT_XML:
    case OUTPUT_XHTML:
        writeDTD = !DTSystem.isEmpty();
        break;
    case OUTPUT_HTML:
        writeDTD = !DTSystem.isEmpty() || !DTPublic.isEmpty();
        break;
    default:
        writeDTD = FALSE;
    }

    delayedDTD = writeDTD;

    /*
    if (writeDTD)
    {
      Str fullName;
          docElementName.getname(fullName);

    };
    */
    return OK;
}


eFlag OutputterObj::reportFront(Sit S)
{
    sabassert(method != OUTPUT_UNKNOWN);
    int i, frontCount = front.number();
    FrontMatterItem *item;
    for(i = 0; i < frontCount; i++)
    {
        item = front[i];
        switch(item -> kind)
        {
        case FM_TEXT:
            {
                if (item -> disableEsc)
                    E( eventDisableEscapingForNext(S) );
                E( eventData(S, item -> string1) );
            }; break;
        case FM_PI:
            {
                E( eventPIStart(S, item -> string1) );
                E( eventData(S, item -> string2) );
                E( eventPIEnd(S) );
            }; break;
        case FM_COMMENT:
            {
                E( eventCommentStart(S) );
                E( eventData(S, item -> string1) );
                E( eventCommentEnd(S) );
            };
        }
    }
    return OK;
}


eFlag OutputterObj::eventBeginOutput(Sit S)
{
  const EQName dummy;
  pushLevel(dummy); //experimental
  method = outDef ? outDef -> getMethod() : OUTPUT_UNKNOWN;
  if (physical)
    {
      //??JP??both physical & method !=... 
      //duplicit with the same in reportXMLDeclIfMust 
      if (method != OUTPUT_UNKNOWN)
	E( reportXMLDeclIfMust(S) );
    }
  // initialize the SAX interface
  IF_SAX1( startDocument ); 
  state = STATE_OUTSIDE;
  return OK;
}

eFlag OutputterObj::eventBeginSubtree(Sit S)
{
  const EQName dummy;
  pushLevel(dummy); //experimntal
    method = outDef -> getMethod();
    if (physical)
    {
      //      method = outDef -> getMethod();
    }
    // initialize the SAX interface
    IF_SAX1( startDocument ); 
    state = STATE_OUTSIDE;
    return OK;
}


eFlag OutputterObj::eventElementStart(Sit S, const EQName& name)
{
  if (noElementYet)
    {
      noElementYet = FALSE;
      if (physical)
        {
	  if (method == OUTPUT_UNKNOWN)
            {
	      if (name.getUri().isEmpty() && name.getLocal().eqNoCase("html"))
		method = OUTPUT_HTML;
	      else
		method = OUTPUT_XML;
	      E( physical -> setMethodByDefault(S, method) );
	      E( reportXMLDeclIfMust(S) );
	      E( reportFront(S) );//??JP??sax
            };
	  // If this is an included stylesheet, output method cd be TEXT here
	  if (method != OUTPUT_TEXT) {
	    E( reportDTDIfMust( S, name ) );//??JP??sax
	  }
        };
    }
  switch(state)
    {
    case STATE_OUTSIDE:
    case STATE_IN_MARKUP:
    case STATE_IN_ELEMENT:
      {
	// reportStartTag also sends the event to SAX
	E( reportStartTag(S, NONEMPTY_ELEMENT) );
	E( reportCurrData(S) );
	pushLevel(name);
	// this sets state to STATE_IN_MARKUP
      };
      break;
    case STATE_IN_COMMENT:
    case STATE_IN_PI:
    case STATE_IN_ATTRIBUTE:
      Err( S, E_ELEM_IN_COMMENT_PI );
    default:
      sabassert(!"eventElementStart");
    };
  
  // determine whether we're in a HEAD html element so a META tag
  // needs to be added
  
  return OK;
}

eFlag OutputterObj::throwInMeta(Sit S)
{
    noHeadYet = FALSE;

    if ( S.hasFlag(SAB_DISABLE_ADDING_META) ) return OK;

    if (physical || mySAXHandler)
    {
        Str metaName("meta");
	Str httpEquiv("http-equiv");
	Str contentType("Content-Type");
	Str content("content");
        DStr contentValue = 
	  NZ(outDef) -> getValueStr(XSLA_MEDIA_TYPE) + "; charset=" +
	  outDef -> getValueStr(XSLA_ENCODING);

	if (physical) {
	  StrStrList metaAtts;
	  //EQName 
	  //   httpEquivName, 
	  //   contentName;
	  //httpEquivName.setLocal("http-equiv");
	  //contentName.setLocal("content");
	  metaAtts.appendConstruct(httpEquiv, contentType);
	  metaAtts.appendConstruct(content, contentValue);
	  E( physical -> outputElementStart(S, metaName, currNamespaces,
					    getFirstOwnNS(), metaAtts, TRUE) );
	  E( physical -> outputElementEnd(S, metaName, TRUE) );
	  metaAtts.freeall(FALSE);
	};

	if (mySAXHandler) {
	  char *saxAtts[5];
	  saxAtts[0] = httpEquiv;
	  saxAtts[1] = contentType;
	  saxAtts[2] = content;
	  saxAtts[3] = (char *) contentValue;
	  saxAtts[4] = NULL;
	  mySAXHandler->startElement(mySAXUserData, S.getProcessor(),
				     metaName, 
				     (const char**) saxAtts);
	  mySAXHandler->endElement(mySAXUserData, S.getProcessor(),
				   metaName);
	};

        state = STATE_IN_ELEMENT;//??JP?? why to change state ?

    };
    return OK;
}
    


eFlag OutputterObj::eventElementEnd(Sit S, const EQName& name)
{
  sabassert(!(physical && mySAXOutputType == SAXOUTPUT_INT_PHYSICAL));
  Str physName;
  //  if (S.getProcessor())
  //  temp = S.getProcessor() -> getAliasedName(name, 
  //					      currNamespaces,
  //					      mySAXOutputType == SAXOUTPUT_INT_PHYSICAL);
  //else name.getname(temp);
  
  switch(state)
    {
    case STATE_IN_MARKUP:
      E( reportStartTag(S, EMPTY_ELEMENT) ); 
      break;
    case STATE_IN_ELEMENT:
      {
	E( reportCurrData(S) );
	if (physical)
	  {
	    Str physName;
	    name.getname(physName);
	    physical -> outputElementEnd(S, physName, NONEMPTY_ELEMENT );
	  };
      }; break;
    default:
      sabassert(!"eventElementEnd");
    };
  
  // send the event to SAX
  switch(mySAXOutputType)
    {
    case SAXOUTPUT_COPY_TREE:
    case SAXOUTPUT_INT_PHYSICAL:
      {
        GP( Str ) theSAXname = nameForSAX(S, name);
	IF_SAX2( endElement, (const char*) *theSAXname );
	theSAXname.del();
	// report namespace scope ends
	//while (currNamespaces.number() > getFirstOwnNS())
	for (int ii = currNamespaces.number() - 1; ii >= getFirstOwnNS(); ii--)
	  {
	    IF_SAX2( endNamespace, currNamespaces[ii] -> prefix );
	    //debug
	  }
      };
      break;
    case SAXOUTPUT_AS_PHYSICAL:
      {
	Str physName;
	name.getname(physName);
        IF_SAX2( endElement, (const char*) physName );
	// report namespace scope ends
	Str* prefix;
	//while (currNamespaces.number() > getFirstOwnNS())
	for (int ii = currNamespaces.number() - 1; ii >= getFirstOwnNS(); ii--)
	  {
	    prefix = &(currNamespaces[ii] -> prefix);
	    if (! currNamespaces.isHidden(*prefix))
	      IF_SAX2( endNamespace, currNamespaces[ii] -> prefix );
	  }
      };
      break;
    case SAXOUTPUT_NONE: break;
    default:
      sabassert(!"eventElementEnd");
    };
  
  //remove our namespaces
  while (currNamespaces.number() > getFirstOwnNS())
    currNamespaces.freelast(FALSE);

  // pop one history level
  history.freelast(FALSE);
  if (getLevel()) //experimental
    state = STATE_IN_ELEMENT;
  else
    state = STATE_OUTSIDE;
  
  return OK;
}


eFlag OutputterObj::eventAttributeStart(Sit S, const EQName& name)
{
    Str fullName;
    name.getname(fullName);
    switch(state)
    {
    case STATE_IN_MARKUP:
        {
            state = STATE_IN_ATTRIBUTE;
            currAttName = name;
        }; break;
    case STATE_IN_ELEMENT:
        {
            Err1(S, E1_ATTRIBUTE_TOO_LATE, fullName);
	    };
        break;
    case STATE_OUTSIDE:
        {
            Err1(S, E1_ATTRIBUTE_OUTSIDE, fullName);
	    };
        break;
    default:
      Err1(S, E1_ATTRIBUTE_MISPLACED, fullName);
      //sabassert(!"eventAttributeStart");
    };
    return OK;
}


eFlag OutputterObj::eventAttributeEnd(Sit S)
{
    sabassert(state == STATE_IN_ATTRIBUTE);

    int currAttNameNdx = currAtts.findNdx(currAttName);
    if (currAttNameNdx != -1)
        currAtts[currAttNameNdx] -> value = currData;
    else
        currAtts.appendConstruct(currAttName, currData);
    currData.empty();
    state = STATE_IN_MARKUP;
    return OK;
}


eFlag OutputterObj::eventCommentStart(Sit S)
{
    switch(state)
    {
    case STATE_IN_MARKUP:
        E( reportStartTag(S, NONEMPTY_ELEMENT) );
        // no break
    case STATE_OUTSIDE:
    case STATE_IN_ELEMENT:
        {
            E( reportCurrData(S) );
            state = STATE_IN_COMMENT;
        }; break;
    default:
        sabassert(!"eventCommentStart");
    };
    return OK;
}

eFlag OutputterObj::eventCommentEnd(Sit S)
{
    sabassert(state == STATE_IN_COMMENT);
    if (physical && method == OUTPUT_UNKNOWN)
        E( front.appendConstruct(S, FM_COMMENT, currData, 
                                 ""/**theEmptyString*/, FALSE) )
    else
    {
        IF_PH2( outputComment, currData );
        IF_SAX2( comment, currData );
    }
    currData.empty();
    state = (getLevel() ? STATE_IN_ELEMENT : STATE_OUTSIDE); //experimental
    return OK;
}

eFlag OutputterObj::eventPIStart(Sit S, const Str& name)
{
    switch(state)
    {
    case STATE_IN_MARKUP:
        E( reportStartTag(S, NONEMPTY_ELEMENT) );
        // no break
    case STATE_IN_ELEMENT:
    case STATE_OUTSIDE:
        {
            E( reportCurrData(S) );
            state = STATE_IN_PI;
            currPIName = name;
        }; break;
    default:
        sabassert(!"eventPIStart");
    };
    return OK;
}

eFlag OutputterObj::eventPIEnd(Sit S)
{
    sabassert(state == STATE_IN_PI);

    //check te PI data
    if ( strstr((char*)currData, "?>") )
      Err(S, E_INVALID_DATA_IN_PI);

    if (physical && method == OUTPUT_UNKNOWN)
        E( front.appendConstruct(S, FM_PI, currPIName, currData, FALSE) )
    else
    {
        IF_PH3( outputPI, currPIName, currData );
        IF_SAX3( processingInstruction, currPIName, currData );
    }
    currData.empty();
    currPIName.empty();
    state = (getLevel() ? STATE_IN_ELEMENT : STATE_OUTSIDE);
    return OK;
}

eFlag OutputterObj::eventNamespace(Sit S, const Str& prefix, const Str& uri,
				   Bool hidden)
{
    sabassert(state == STATE_IN_MARKUP);
    // GP: OK, not allocated
    Str search = uri;
    int existing_ndx = currNamespaces.findNum(prefix);
    const Str *existing_namespace = (
       existing_ndx == -1 ? NULL : &(currNamespaces[existing_ndx] -> uri));
    //explanation: if we found previously inserted namespace, we have
    //to add new definition to override it with one exception. Two
    //namespaces with the same prefix may be added (e.g. for
    //xsl:element insrtuction) in this case (existing_ndx >=
    //getFirstOwnNS()) we need override former definition in place.

    /*
    if (uri == theXSLTNamespace)
      {
	//we need to reveal the xslt namespace if it is aliased
	Processor *proc = S.getProcessor();
	if (proc)
	  { 
	    Bool aliased;
	    E( proc -> prefixIsAliasTarget(S, prefix, aliased) );
	    hidden = hidden && ! aliased;
	  }
      }
    */

    if (!existing_namespace)
      {
	//if the namespace doesn't exist yet, we add it
	currNamespaces.appendConstruct(prefix, uri, hidden);
      }
    else
      {
	Bool exIsHidden = currNamespaces[existing_ndx]->hidden;
	if (!(*existing_namespace == uri))
	  {
	    if (existing_ndx >= getFirstOwnNS())
	      {
		currNamespaces[existing_ndx] -> uri = uri;
		currNamespaces[existing_ndx] -> hidden = hidden;
	      }
	    else
	      {
		currNamespaces.appendConstruct(prefix, uri, hidden);
	      }
	  }
	//may be we found excluded one, but in current subtree
	//it sould be kept
	else if (exIsHidden && !hidden)
	  {
	    currNamespaces.appendConstruct(prefix, uri, hidden);
	  }
      }
    return OK;
}

eFlag OutputterObj::eventDisableEscapingForNext(Sit S)
{
    if (method != OUTPUT_TEXT)
    {
        switch(state)
        {
        case STATE_IN_ATTRIBUTE:
        case STATE_IN_PI:
        case STATE_IN_COMMENT:
	  Warn(S, W_DISABLE_OUTPUT_ESC); break;
        default:
	  outputEscaping = FALSE;
        };
    }
    return OK;
}

eFlag OutputterObj::eventData(Sit S, const Str& data, Bool hardCData /* = FALSE */)
{
    if (physical && method == OUTPUT_UNKNOWN && state == STATE_OUTSIDE)
    {
        E( front.appendConstruct(S, FM_TEXT, data, 
                                 "" /**theEmptyString*/, !outputEscaping) );
        if (!isAllWhite((const char*) data))
        {
            E( physical -> setMethodByDefault(S, method = OUTPUT_XML) );
            E( reportXMLDeclIfMust(S) );
            E( reportFront(S) );
        }
        return OK;
    }

    switch(state)
    {
    case STATE_IN_MARKUP:
        E( reportStartTag(S, NONEMPTY_ELEMENT) );
        // no break
    case STATE_IN_ELEMENT:
    case STATE_OUTSIDE:
        {   // this is a text node
            if (!(getFlags() & CDATA_SECTION) && !hardCData)
            {
                int htmlScriptOrStyle = ( getFlags() & HTML_SCRIPT_OR_STYLE );
                if (physical)
                    E( physical -> outputText(S, data, !outputEscaping, 
					      htmlScriptOrStyle) );
            }
            // reset outputEscaping to default after use
            // even if we just save a part of cdata section
            outputEscaping = TRUE;
            state = getLevel() ? STATE_IN_ELEMENT : STATE_OUTSIDE;
        }   // no break
    case STATE_IN_COMMENT:
    case STATE_IN_PI:
    case STATE_IN_ATTRIBUTE:
        {
            currData += data;
        }; break;
    default:
        sabassert(!"eventData()");
    }
    return OK;
}

eFlag OutputterObj::eventCDataSection(Sit S, const Str& data)
{
    switch(state)
    {
        case STATE_IN_MARKUP:
	        E( reportStartTag(S, NONEMPTY_ELEMENT) );
		// no break
		case STATE_OUTSIDE:
		case STATE_IN_ELEMENT:
		{
		    E( reportCurrData(S) );
		    E( eventData(S, data, /* hardCData = */ TRUE) );
		    E( reportCurrData(S, /* hardCData = */ TRUE) );
		}; break;
		default:
		    sabassert(!"eventCDataSection()");
    };    
    return OK;
}

eFlag OutputterObj::eventEndOutput(Sit S, Bool closePhysical /* = false */)
{
    sabassert(state == STATE_OUTSIDE);
    E( reportCurrData(S) );
    if (physical && method == OUTPUT_UNKNOWN)
    {
        E( physical -> setMethodByDefault(S, method = OUTPUT_XML) );
        E( reportXMLDeclIfMust(S) );
        E( reportFront(S) );
    }
    IF_PH1( outputDone );
    IF_SAX1( endDocument );
    state = STATE_DONE;

    //pop fake history item
    history.freelast(FALSE);

    //close physical (used for multiple doc output)
    if (physical && closePhysical) 
      {
	E( physical -> close(S) );
      }
    
    return OK;
}

eFlag OutputterObj::eventTrailingNewline(Sit S)
{
  sabassert(state == STATE_OUTSIDE);
  if (physical)
    {
      E(physical -> outputTrailingNewline(S))
    }
  return OK;
}

eFlag OutputterObj::reportCurrData(Sit S, Bool hardCData /* = FALSE */)
{
    if (currData.isEmpty())
        return OK;
    switch(state)
    {
    case STATE_OUTSIDE:
    case STATE_IN_MARKUP:
    case STATE_IN_ELEMENT:
        {
            if (getFlags() & CDATA_SECTION || hardCData)
            {
                // we might now call the SAX startCData handler (if there was one)
                IF_SAX3( characters, currData, currData.length() );
                IF_PH2( outputCDataSection, currData );
                // we might call the SAX endCData handler
            }
            else
            {
                // the text had been output to physical in parts
                IF_SAX3( characters, currData, currData.length());
            }
        }; break;
    default:
        sabassert(!"reportCurrData()");
    }
    currData.empty();
    return OK;
}


/*
    GP: if this function is modified, care must be taken about the guarded
    pointers
*/

eFlag OutputterObj::reportStartTag(Sit S, Bool isEmpty)
{
  sabassert(!(physical && mySAXOutputType == SAXOUTPUT_INT_PHYSICAL));
  // FIXME: couldn't improve reportStartTag from GP point of view?
  if (state == STATE_OUTSIDE || currElement.isEmpty())
    return OK;
  
  int doThrowMeta = method == OUTPUT_HTML && noHeadYet && 
    currElement.getUri().isEmpty() && currElement.getLocal().eqNoCase("head");
  int i;
  //get attr aliased names
  StrStrList attAliased;
  // the temporary string is a workaround for VisualAge on AIX
  //Str temp;


  Str physName;
  Bool intPhys = mySAXOutputType == SAXOUTPUT_INT_PHYSICAL;

  if (physical || (mySAXHandler && (mySAXOutputType == SAXOUTPUT_AS_PHYSICAL)))
    {
      //if (S.getProcessor())
      //temp = S.getProcessor() -> getAliasedName(currElement, 
      //					  currNamespaces, intPhys);
      //else
      //currElement.getname(temp);
      currElement.getname(physName);
      
      for (i = 0; i < currAtts.number(); i++)
	{
	  Str aliname;
	  //if (S.getProcessor())
	  //  aliname = S.getProcessor() -> getAliasedName(currAtts[i] -> key,
	  //					 currNamespaces, 
	  //					 intPhys);
	  //else
	  //  currAtts[i] -> key.getname(aliname); 
	  currAtts[i] -> key.getname(aliname);
	  attAliased.appendConstruct(aliname, currAtts[i] -> value);
	}
      
      if (physical) {
	if ( delayedDTD )
	  {
	    const Str& 
	      DTSystem = outDef -> getValueStr(XSLA_DOCTYPE_SYSTEM),
	      DTPublic = outDef -> getValueStr(XSLA_DOCTYPE_PUBLIC);        
	    E( physical -> outputDTD( S, physName, DTPublic, DTSystem) );
	    delayedDTD = FALSE;
	  }

	E( physical -> outputElementStart(S, physName, currNamespaces, 
					  getFirstOwnNS(), attAliased, 
					  isEmpty && !doThrowMeta) );
      }
    }
  
  if (mySAXHandler)
    {
      const char** attsTable;
      GPA( ConstCharP ) attsArr = attsTable = new const char*[(currAtts.number() * 2) + 1];
      // GP: the allocations are OK: no exit routes
      
      PList<Str*> stringsForDeletion;
      attsTable[currAtts.number() * 2] = NULL;
      
      switch(mySAXOutputType)
	{
	case SAXOUTPUT_COPY_TREE:
	case SAXOUTPUT_INT_PHYSICAL:
	  {
	    // report only the new namespaces
	    Str* prefix;
	    for (i = getFirstOwnNS(); i < currNamespaces.number(); i++)
	      {
		prefix = &(currNamespaces[i] -> prefix);
		if (intPhys)
		  {
		    IF_INT_SAX4( startNamespace2, currNamespaces[i] -> prefix, 
				 currNamespaces[i] -> uri, 
				 currNamespaces.isHidden(*prefix) );
		  }
		else
		  IF_SAX3( startNamespace, currNamespaces[i] -> prefix, 
			   currNamespaces[i] -> uri );
	      }
	    // create the attribute table
	    for (i = 0; i < currAtts.number(); i++)
	      {
		Str *expatName = nameForSAX(S, currAtts[i] -> key);
		stringsForDeletion.append(expatName);
		attsTable[i * 2] = *expatName;
		attsTable[(i * 2) + 1] = currAtts[i] -> value;
	      };
	    Str* theSAXname = nameForSAX(S, currElement);
	    stringsForDeletion.append(theSAXname);
	    IF_SAX3(startElement, *theSAXname, attsTable);
	    stringsForDeletion.freeall(FALSE);
	  };
	  break;
	case SAXOUTPUT_AS_PHYSICAL:
	  // report only the new not hidden namespaces
	  Str* prefix;
	  for (i = getFirstOwnNS(); i < currNamespaces.number(); i++)
	    {
	      prefix = &(currNamespaces[i] -> prefix);
	      if (! currNamespaces.isHidden(*prefix)) 
		IF_SAX3( startNamespace, currNamespaces[i] -> prefix, 
			 currNamespaces[i] -> uri );
	    }
	  // create the attribute table
	  for (i = 0; i < currAtts.number(); i++)
	    {
	      attsTable[i * 2] = (char *)attAliased[i] -> key;
	      attsTable[(i * 2) + 1] = (char *)attAliased[i] -> value;
	    };
	  
	  IF_SAX3(startElement, physName, attsTable);
	  break;
	case SAXOUTPUT_NONE:
	  sabassert(!"saxoutput == NONE");
	default:
	  sabassert(!"eventElementEnd");
	};
      //attsTable.delArray();	
    }
  
  if (physical || (mySAXHandler && mySAXOutputType == SAXOUTPUT_AS_PHYSICAL))
    {
      attAliased.freeall(FALSE);
    }
  
  eFlag result = OK;
  if (doThrowMeta) {
    result = throwInMeta(S);
    //__PH__ close head if needed
    if (isEmpty) {
      if (physical) {
	E( physical ->outputElementEnd(S, physName, FALSE) );
      };
    };
  }
  
  currElement.empty();
  currAtts.freeall(FALSE);
  currData.empty();
  return result;
}

void OutputterObj::pushLevel(const EQName& name)
{
    currElement = name;
    GP( OutputHistoryItem ) newItem = new OutputHistoryItem;
    if (history.number())
      {
        *newItem = *(history.last());
	//next line is speed optimalization (very few)
	(*newItem).useDocument = (*(history.last())).document;
      }
    else
      {
        (*newItem).flags = 0;
	(*newItem).useDocument = NULL;
      }
    //definition is used for multiple document output
    (*newItem).document = NULL; //in any case
    if (physical)
    {
        if (outDef -> askEQNameList(XSLA_CDATA_SECT_ELEMS, name))
            (*newItem).flags |= CDATA_SECTION;
        else
            (*newItem).flags &= ~CDATA_SECTION;

	//style and script for html
	if ( (method == OUTPUT_HTML && name.getUri() == "") &&
	     isHTMLNoEscapeTag(name.getLocal()) )
	  {
	    (*newItem).flags |= HTML_SCRIPT_OR_STYLE;
	  }
	else
	  {
	    (*newItem).flags &= ~HTML_SCRIPT_OR_STYLE;
	  }
    };
    (*newItem).firstOwnNS = currNamespaces.number();
    history.append(newItem.keep());
    state = STATE_IN_MARKUP;
}

void OutputterObj::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type, code, arg1, arg2);
}

eFlag OutputterObj::setDocumentForLevel(Sit S, OutputDocument *doc)
{
  if (history.number())
    (*(history.last())).document = doc;
  return OK;
}

OutputDocument* OutputterObj::getDocumentForLevel(Bool forElement)
{
  if (history.number())
    return forElement ? (*(history.last())).useDocument :
    (*(history.last())).document;
  else 
    return NULL;
}
