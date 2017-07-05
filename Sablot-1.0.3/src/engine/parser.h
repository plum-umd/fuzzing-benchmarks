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

//  parser.h
//  Sablotron's interface to a SAX-like XML parser (currently expat)

#ifndef ParserHIncl
#define ParserHIncl

// GP: clean

#include "base.h"
#include "output.h"

#if defined (HAVE_EXPAT_H)
#include <expat.h>
#elif defined (HAVE_XMLPARSE_H)
#include <xmlparse.h>
#elif defined (HAVE_XMLTOK_XMLPARSE_H)
#include <xmltok/xmlparse.h>
#elif defined (WIN32)
#include "expat.h"
#endif

class Tree;
class DataLine;
class NSList;

// TreeConstructer

typedef SAX_RETURN 
SAXHandlerStartNamespace2(void* userData, SablotHandle processor_,
    const char* prefix, const char* uri, Bool hidden);

/* must be compatible with SAXHandler */
typedef struct
{
  SAXHandlerStartDocument     *startDocument;
  SAXHandlerStartElement      *startElement;
  SAXHandlerEndElement        *endElement;
  SAXHandlerStartNamespace    *startNamespace;
  SAXHandlerEndNamespace      *endNamespace;
  SAXHandlerComment           *comment;
  SAXHandlerPI                *processingInstruction;
  SAXHandlerCharacters        *characters;
  SAXHandlerEndDocument       *endDocument;
  SAXHandlerStartNamespace2   *startNamespace2;
} SAXHandlerInternal;

class TreeConstructer
{
public:
    TreeConstructer(Sit S_);
    ~TreeConstructer();
    eFlag parseDataLineUsingExpat(Sit S,
        Tree *t, DataLine *d, char* base_ = NULL);
    eFlag parseUsingSAX(Sit S, Tree *t, OutputterObj& source, 
			SAXOutputType ot = SAXOUTPUT_COPY_TREE);

    eFlag parseUsingSAXForAWhile(Sit S, OutputterObj& source, Str &saxUri,
				 Bool resetNamespaces,
				 Tree *srcTree,
				 NSList &swallowNS);
    eFlag parseUsingSAXForAWhileDone(Sit S, OutputterObj& source, 
				     Bool resetNamespaces);

    //methods for expat
    static void tcStartDocument(
        void* userData);
    static void tcStartElement(
        void *constructer, const char *elName, const char **atts);
    static void tcEndElement(
        void* constructer, const char* name);
    static void tcStartNamespace(
        void* userData, const char* prefix, const char* uri);
    static void tcStartNamespace2(
        void* userData, const char* prefix, const char* uri, Bool hidden);
    static void tcEndNamespace(
        void* constructer, const char* prefix);
    static void tcComment(
        void* constructer, const char* contents);
    static void tcPI(
        void* constructer, const char* target, const char* contents);
    static void tcCharacters(
        void* constructer, const char* contents, int length);
    static void tcEndDocument(
        void* constructer);
    // the unknown encoding handler is no more used:
    // static int tcUnknownEncoding(
    //     void *encodingHandlerData, const char *name, XML_Encoding *info);
    static int tcExternalEntityRef(
        XML_Parser parser, const char* context, const char* base,
        const char* systemId, const char* publicId);
    static void tcEntityDecl(void *userData, const char *entityName,
			    int is_parameter_entity, const char *value,
			    int value_length, const char *base,
			    const char *systemId, const char *publicId,
			    const char *notationName);
    //methods for sax handler (the processor is passed in)
    static void saxStartDocument(
        void* userData, SablotHandle processor_);
    static void saxStartElement(
        void *constructer,  SablotHandle processor_,
	const char *elName, const char **atts);
    static void saxEndElement(
        void* constructer, SablotHandle processor_, const char* name);
    static void saxStartNamespace(
        void* userData, SablotHandle processor_,
	const char* prefix, const char* uri);
    static void saxStartNamespace2(
        void* userData, SablotHandle processor_,
	const char* prefix, const char* uri, Bool hidden);
    static void saxEndNamespace(
        void* constructer, SablotHandle processor_, const char* prefix);
    static void saxComment(
        void* constructer, SablotHandle processor_, const char* contents);
    static void saxPI(
        void* constructer, SablotHandle processor_,
	const char* target, const char* contents);
    static void saxCharacters(
        void* constructer, SablotHandle processor_,
	const char* contents, int length);
    static void saxEndDocument(
        void* constructer, SablotHandle processor_);
private:
    Situation& S;
    eFlag parseDataLineUsingGivenExpat(Sit S, Tree *t, DataLine *d, 
				       XML_Parser theParser_);
    static eFlag setQNameFromExpat(Sit S, TreeConstructer* this_, 
				   QName& qname_, const char* text);
    static eFlag feedDocumentToParser(Sit S, void* constructer);
    static eFlag getDocEncoding(Sit S, const char *buf, Str& theEncoding, 
				TreeConstructer *this_);
    int getCurrentLineNumber() const;
    XML_Parser theParser;
    Tree *theTree;
    List<int> nsCount; 
    DataLine *theDataLine;
    List<Bool> inSAXForAWhile;
    PList<Str*> theSAXUri;
    int theLineNumber;
    static SAXHandlerInternal myHandlerRecord; // initialized in parser.h
    void report(Sit S, MsgType type, MsgCode code, 
        const Str& arg1, const Str& arg2) const;
    List<Bool> preserveSpace;
};

#endif
