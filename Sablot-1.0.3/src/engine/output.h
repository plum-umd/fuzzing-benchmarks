/* -*- mode: c++ -*-
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

#if !defined(OutputHIncl)
#define OutputHIncl

// GP: clean

#include "datastr.h"
#include "utf8.h"
#include "encoding.h"

class DataLine;     // see uri.h

enum OutputMethod
{
    OUTPUT_XML,
    OUTPUT_HTML,
    OUTPUT_TEXT,
    OUTPUT_XHTML,
    OUTPUT_UNKNOWN
};

enum EscMode
{
    ESCAPING_NONE,
    ESCAPING_URI,
    ESCAPING_ATTR,
//    ESCAPING_CDATA, - use ESCAPING_NONE
    ESCAPING_LT_AMP,
    ESCAPING_HTML_URI,
    ESCAPING_HTML_ATTR
};

enum SAXOutputType
{
  SAXOUTPUT_NONE,
  //hard copy, no translations, no hidden ns....
  SAXOUTPUT_COPY_TREE,
  //for physical output, hidden ns, aliasing
  SAXOUTPUT_AS_PHYSICAL,
  //as physical, but use expat like names
  SAXOUTPUT_INT_PHYSICAL
};

#define THE_NAMESPACE_SEPARATOR '`'

// the following must match the size of outputStringAtts[] - 1
// as defined in output.cpp
#define STRING_ITEMS_COUNT 8

// constants for special values of precedence for xsl:output attributes
// use for OutputDefinition::setItemStr and setItemEQName

// specify only if not yet specified, otherwise no error
#define OUTPUT_PRECEDENCE_WEAKEST -1
// override, no error
#define OUTPUT_PRECEDENCE_STRONGEST -2
// for internal use only
#define OUTPUT_PRECEDENCE_UNSPECIFIED -3

//
//  StrPrec
//  string with (import) precedence
//  unspecified: precedence = -1
//  to set, new precedence must be less than existing (unless existing == -1)
//  special values of new precedence: override without checking (-1), set only if unspecified (-2)
//

class StrPrec
{

public:
    StrPrec()
	{
	    precedence = OUTPUT_PRECEDENCE_UNSPECIFIED;
	};
    // sets the string if new precedence is stronger
    // returns TRUE if new pref is non-negative and equal to old (does not set in this case)
    Bool set(const Str& newString, int newPrecedence);
    const Str& get() const 
	{
	    return string;
	}
private:
    Str string;
    int precedence;
};

//
//  EQNamePrec
//  same as StrPrec but holding EQName
//

class EQNamePrec
{

public:
    EQNamePrec()
	{
	    precedence = OUTPUT_PRECEDENCE_UNSPECIFIED;
	};
  // sets the string if new precedence is stronger
  // returns TRUE if new pref is non-negative and equal to old (does not set in this case)
    Bool set(const EQName& newName, int newPrecedence);
    const EQName& get() const 
	{
	    return name;
	}
private:
    EQName name;
    int precedence;
};

//
//  OutputDef
//  stores the output-controlling information for an XSLT tree
//

#define OUTPUT_BUFFER_LIMIT     1024
#define OUTPUT_BUFFER_SIZE      OUTPUT_BUFFER_LIMIT + 64


#define HTML_SPECIAL            TRUE
#define NOT_HTML_SPECIAL        FALSE

class OutputDefinition
{
public:
    OutputDefinition();
    ~OutputDefinition();
    // sets a string item
    // itemId identifies the attribute code, 'value' is the new value
    // 'caller' is the xsl:output element
    // precedence is the current import precedence 
    // (may be OUTPUT_PRECEDENCE_STRONGEST to override, ..._WEAKEST to only set if unset) if !caller
    eFlag setItemStr(
	Sit S, XSL_ATT itemId, const Str& value, Vertex *caller, int precedence);
    // sets an EQName item (@method or @cdata-sect-elems)
    eFlag setItemEQName(
	Sit S, XSL_ATT itemId, const EQName& value, Vertex *caller, int precedence);
    eFlag setDefaults(Sit S);
    const Str& getValueStr(XSL_ATT itemId) const;
    const EQName& getValueEQName(XSL_ATT itemId) const;
    Bool askEQNameList(XSL_ATT itemId, const EQName &what) const;
    int getStatus(XSL_ATT itemId) const;
    OutputMethod getMethod() const;
    Bool getIndent() const;
    const Str& getEncoding() const;
private:
    StrPrec stringItems[STRING_ITEMS_COUNT];
    EQNamePrec
        method;
    EQNameList
        cdataElems;
    DataLine *targetDataLine;
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
};

//
//
//  PhysicalOutputterObj
//
//

class PhysicalOutputLayerObj
{
public:
    PhysicalOutputLayerObj(CDesc encodingCD_);
    ~PhysicalOutputLayerObj();
    eFlag setOptions(Sit S, DataLine *targetDataLine_, OutputDefinition *outDef_);
    eFlag outputElementStart(Sit S, const Str& name, 
        const NamespaceStack& namespaces, const int namespace_index,
        const StrStrList& atts, Bool isEmpty);
    eFlag outputElementEnd(Sit S, const Str& name, Bool isEmpty);
    eFlag outputText(Sit S, const Str& contents, Bool outputEscaping, Bool inHTMLSpecial);
    eFlag outputComment(Sit S, const Str& contents);
    eFlag outputCDataSection(Sit S, const Str& contents);
    eFlag outputPI(Sit S, const Str& target, const Str& data);
    eFlag outputDTD(Sit S, const Str& name, const Str& publicId, const Str& systemId);
    eFlag outputTrailingNewline(Sit S);
    eFlag outputDone(Sit S);
    eFlag setMethodByDefault(Sit S, OutputMethod method_);
    DataLine* getDataLine() { return targetDataLine; };
    eFlag close(Sit S);
private:
    DataLine *targetDataLine;
    OutputDefinition *outDef;
    OutputMethod method;
    Bool indent;
    Bool after_markup;  //pc
    int level;  //pc
    char buffer[OUTPUT_BUFFER_SIZE],
        smallBuf[SMALL_BUFFER_SIZE];
    int curr;
    Str encoding;
    CDesc encodingCD;
    Bool defaultNSWas;
    eFlag sendOut(Sit S, const char* data, int length, EscMode escapeMode);
    eFlag sendOutUntil(Sit S, const char* & data, int length,
        EscMode escapeMode, const char* stoppingText);
    int writeCharacterRef(char* dest, const char *src, EscMode escapeMode);
    eFlag flushBuffer(Sit S);
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
};

//
//
//  OutputterObj
//
//

//output document (for multiple document output)
enum OutdocState {
  OUTDOC_NEW,
  OUTDOC_ACTIVE,
  OUTDOC_FINISHED
};

class OutputterObj;
class OutputDocument
{
public:
  OutputDocument(Str h, OutputDefinition *d) : 
    href (h), outputter(NULL), state(OUTDOC_NEW), def(d) {};
  ~OutputDocument();
  OutputterObj* setOutputter(OutputterObj* new_);
  OutputterObj* getOutputter() { return outputter; };
  OutputDefinition* getDefinition() { return def; };
  OutdocState getState() { return state; };
  void setState(OutdocState s) { state = s; };
  eFlag finish(Sit S);
  Str &getHref() { return href; };
  Str &getURI() { return uri; };
  void setURI(Str& val) { uri = val; };
private:
  Str href;
  OutputterObj *outputter;
  OutdocState state;
  OutputDefinition *def;
  Str uri;
};

//following structures are used for correct namespace exclusions
class SubtreeInfo;

enum OutputterState
{
    STATE_OUTSIDE = 0,
    STATE_IN_MARKUP,
    STATE_IN_ELEMENT,
    STATE_IN_ATTRIBUTE,
    STATE_IN_COMMENT,
    STATE_IN_PI,
    STATE_DONE,
    STATE_UNDEFINED
};

struct OutputHistoryItem
{
  int flags;
  int firstOwnNS;
  OutputDocument *document;
  OutputDocument *useDocument;
};

typedef PList<OutputHistoryItem*> OutputHistory;

//
//  FrontMatter
//

enum FrontMatterKind
{
    FM_TEXT,
    FM_COMMENT,
    FM_PI
};

struct FrontMatterItem
{
    FrontMatterKind kind;
    Str string1, string2;
    Bool disableEsc;
};

class FrontMatter : public PList<FrontMatterItem*>
{
public:
    eFlag appendConstruct(Sit S, FrontMatterKind kind, const Str& string1,
        const Str& string2, Bool disableEsc);
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
};


class OutputterObj
{
public:
    OutputterObj();
    ~OutputterObj();
    eFlag setOptions(Sit S, DataLine *targetDataLine_, OutputDefinition *outDef_);
    eFlag setOptionsSAX(Sit S, SAXHandler *streaming_, void *userData, SAXOutputType saxout_);
    eFlag eventBeginOutput(Sit S);
    //_PH_ used for subtree serialization, ends with eventEndOutput
    eFlag eventBeginSubtree(Sit S); 
    eFlag eventElementStart(Sit S, const EQName& name);
    eFlag eventElementEnd(Sit S, const EQName& name);
    eFlag eventAttributeStart(Sit S, const EQName& name);
    eFlag eventAttributeEnd(Sit S);
    eFlag eventCommentStart(Sit S);
    eFlag eventCommentEnd(Sit S);
  //eFlag eventExcludeNS(Sit S, UriList &exNS);
    eFlag eventNamespace(Sit S, const Str& prefix, const Str& uri, 
			 Bool hidden = FALSE);
    eFlag eventPIStart(Sit S, const Str& name);
    eFlag eventPIEnd(Sit S);
    eFlag eventData(Sit S, const Str& data, Bool hardCData = FALSE);
    eFlag eventCDataSection(Sit S, const Str& data);
    eFlag eventDisableEscapingForNext(Sit S);
    eFlag eventEndOutput(Sit S, Bool closePhysical = false);
    eFlag eventTrailingNewline(Sit S);
    eFlag setDocumentForLevel(Sit S, OutputDocument *doc);
    OutputDocument* getDocumentForLevel(Bool forElement);
    SAXOutputType getSAXOutputType() { return mySAXOutputType; };
    PhysicalOutputLayerObj* getPhysical() { return physical; };
private:
    eFlag reportCurrData(Sit S, Bool hardCData = FALSE);
    eFlag reportStartTag(Sit S, Bool isEmpty);
    eFlag throwInMeta(Sit S);
    Str* nameForSAX(Sit S, const EQName& q);
    void pushLevel(const EQName& name);
//    void popLevel();
    int getLevel() 
    {
      return history.number() - 1; //ignore the '/' frame
    };
    int getFlags()
    {
        return (history.number()? history.last() -> flags : 0);
    }
    int getFirstOwnNS()
    {
        return (history.number()? history.last() -> firstOwnNS : 0);
    }
    Bool nsExcluded(Sit S, Str & uri);
    eFlag reportXMLDeclIfMust(Sit S);
    eFlag reportDTDIfMust(Sit S, const EQName& docElementName);
    eFlag reportFront(Sit S);
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);    
    //
    PhysicalOutputLayerObj *physical;
    SAXHandler *mySAXHandler;
    void *mySAXUserData;
    SAXOutputType mySAXOutputType;
    OutputDefinition *outDef;
    OutputMethod method;
    CDesc encodingCD;
    OutputterState state;
    Bool outputEscaping;
    DStr currData;
    Str
        currPIName;
    EQName
        currElement, 
        currAttName;
    NamespaceStack
        currNamespaces;
    EQNameStrList
        currAtts;
    OutputHistory history;
    Bool noElementYet,
      noHeadYet,
      delayedDTD;
    FrontMatter front;
};


#endif  // OutputHIncl
