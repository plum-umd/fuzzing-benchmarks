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

#ifndef ProcHIncl
#define ProcHIncl

// GP: clean

#define PROC_ARENA_SIZE         0x10000

enum StandardPhrase
{
    PHRASE_EMPTY = 0,
    PHRASE_XSL,
    PHRASE_XSL_NAMESPACE,
    PHRASE_XML_NAMESPACE,
    PHRASE_STAR,
    PHRASE_XMLNS,
    PHRASE_LANG,
    PHRASE_SABEXT_NAMESPACE,
    PHRASE_LAST
};

#include "base.h"
#include "arena.h"
#include "hash.h"
#include "datastr.h"
#include "uri.h"
#include "output.h"
#include "encoding.h"
#include "decimal.h"
#include "jsext.h"
#include "sxpath.h"
// #include "vars.h"
//#include "expr.h"
//#include "context.h"

extern const char *theXSLTNamespace;

class Tree;
class TreeConstructer;
class LocStep;
class Vertex;
class Daddy;
class Attribute;
class XSLElement;
class Expression;
class Context;
typedef PList<Expression*> ExprList;

/*****************************************************************

    R u l e I t e m
    an item of RuleSList describing a rule

*****************************************************************/

class RuleItem
{
public:
    RuleItem(XSLElement *,double, QName &, QName *);
    ~RuleItem();
    XSLElement *rule;
    Attribute *match;
    double priority;
    QName name,
        *mode;
};


class Processor;

/*****************************************************************

    R u l e S L i s t
    sorted list of rules

*****************************************************************/

class RuleSList : public SList<RuleItem*>
{
public:
    RuleSList();
    ~RuleSList();
    int compare(int first, int second, void *data);
    XSLElement* findByName(const Tree &t, const QName &q) const;
};

/*****************************************************************

DataLinesList

*****************************************************************/


class DataLineItem
{
public:
    DataLineItem(Sit S_);
    ~DataLineItem();
    DataLine *_dataline;
    Tree *_tree;
    Bool _isXSL;
    Bool _preparsedTree;
private:
    Sit situation;
};

class DataLinesList: public PList<DataLineItem*>
{
public:
    int findNum(Str &absoluteURI, Bool _isXSL,
        DLAccessMode _mode);
    Tree* getTree(Str &absoluteURI, Bool _isXSL, DLAccessMode _mode);
    eFlag addLine(Sit S, DataLine *d, Tree *t, Bool isXSL,
        Bool preparsedTree = FALSE);
	
};

/*****************************************************************

    OutputDocumentList

*****************************************************************/

class OutputDocumentList: public PList<OutputDocument*>
{
};


/*****************************************************************

    P   r   o   c   e   s   s   o   r

*****************************************************************/

class VarsList;
class VertexList;
class Element;
class KeySet;

class Processor /* : public SabObj */
{
public:
    Tree *input,
        *styleSheet;
    Processor();
    ~Processor();
    eFlag open(Sit S, const char *sheetURI, const char *inputURI);
    eFlag run(Sit S, const char* resultURI, NodeHandle doc = NULL);
    eFlag execute(Sit S, Vertex *, Context *&,  Bool resolvingGlobals);
    eFlag execute(Sit S, VertexList&, Context *&,  Bool resolvingGlobals);

    // finds the best matching rule imported into styleSheet
    // and applies it to the current node of c
    eFlag execApplyImports(Sit S, Context *c, SubtreeInfo *subtree,
			   Bool resolvingGlobals);

    // executes the default rule for the current node of c
    eFlag builtinRule(Sit S, Context *c,  Bool resolvingGlobals);

    FILE *logfile;
    unsigned long allocObjects,
        allocBytes,
        maxAllocObjects,
        maxAllocBytes;
    Expression* getVarBinding(QName &);
    VarsList *vars;
    OutputterObj* outputter()
	{
	    if (outputters_.number())
		return outputters_.last();
	    else return NULL;
	}
    QName* getCurrentMode();
    void pushMode(QName *);
    void popMode();
    eFlag readTreeFromURI(Sit S, Tree*& newTree, const Str& location, 
			  const Str& base, Bool isXSL, 
			  Bool ignoreErr = FALSE);
    eFlag pushOutputterForURI(Sit S, Str& location, Str& base, 
			      OutputDefinition *outDef = NULL);
    eFlag createOutputterForURI(Sit S, //Str& location, Str& base,
				Str& absolute,
				OutputterObj*& result,
				OutputDefinition *outDef = NULL);
    eFlag pushTreeConstructer(Sit S, TreeConstructer *&newTC, Tree *t,
			      SAXOutputType ot = SAXOUTPUT_COPY_TREE);
    eFlag pushOutputter(Sit S, OutputterObj* out_);
    eFlag popOutputter(Sit S);
    eFlag popOutputterNoFree(Sit S);
    eFlag popTreeConstructer(Sit S, TreeConstructer *theTC);
    eFlag processVertexAfterParse(Sit S, Vertex *, Tree *, TreeConstructer *tc);

    eFlag setHandler(Sit S, HandlerType type, void *handler, void *userData);
    SchemeHandler* getSchemeHandler(void **udata);
    MessageHandler* getMessageHandler(void **udata);
    SAXHandler* getSAXHandler(void **udata);
    MiscHandler* getMiscHandler(void **udata);
    EncHandler* getEncHandler(void **udata);

    void* getHandlerUserData(HandlerType type, void *handler);

    void setHardEncoding(const Str& hardEncoding_);
    const Str& getHardEncoding() const;

    eFlag addLineNoTree(Sit S, DataLine *&d, Str &absolute, Bool isXSL);
    eFlag addLineParse(Sit S, Tree *& newTree, Str &absolute, 
		       Bool isXSL, Bool ignoreErr = FALSE);
    eFlag addLineTreeOnly(Sit S, DataLine *&d, Str &absolute, Bool isXSL,
			  Tree *t);
    void copyArg(Sit S, const Str& argName, int* argOrdinal, char*& newCopy);
    eFlag getArg(Sit S, const char*, char*&, Bool);
    eFlag useArg(Sit S, const char *name, const char *val);
    eFlag useTree(Sit S, const char *name, Tree *t);
    eFlag freeResultArgs(Sit S);
    eFlag addGlobalParam(Sit S, const char *name, const char *val);
    eFlag useGlobalParam(Sit S, const char *name, const char *val);
    eFlag useGlobalParams(Sit S);
    const Str& baseForVertex(Sit S, Element *v);
    const Str& findBaseURI(Sit S, const Str& unmappedBase);
    void addBaseURIMapping(const Str& scheme, const Str& mapping);
    void setHardBaseURI(const char* hardBase);

    void setInstanceData(void *idata)
	{ 
	    instanceData = idata; 
	};

    void* getInstanceData()
	{ 
	    return instanceData; 
	};
    
    void rememberSituation(Situation *SP)
	{
	    instanceSituation = SP;
	}
    
    void rememberMasterSituation(Situation *SP)
	{
	    instanceSituation = SP;
	    hasMasterSituation = TRUE;
	}
    
    Situation* recallSituation()
	{
	    return instanceSituation;
	}

    Bool situationIsMaster()
	{
	    return hasMasterSituation;
	}
    
    void prepareForRun();
    void cleanupAfterRun(Situation *Sp);
    const QName theEmptyQName;
    SabArena* getArena();
    //Str getAliasedName(const EQName& name, NamespaceStack& currNamespaces,
    //Bool expatLike);
    void getAliasedName(EQName & name, Bool & aliased);
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const;
    Bool getAddedFlag() {return addedFlag;}
    eFlag addKey(Sit S, const EQName& ename, 
		 Expression& match, Expression &use);
    eFlag getKeyNodes(Sit S, const EQName& ename,  
		      const Str& value, Context& result, SXP_Document doc) const;
    eFlag makeKeysForDoc(Sit S, SXP_Document doc);
    DecimalFormatList& decimals()
	{
	    return decimalsList;
	}
    void initForSXP(Tree *baseTree);
    void cleanupAfterSXP();
    Bool supportsFunction(Str &uri, Str &name);
#ifdef ENABLE_JS
    eFlag evaluateJavaScript(Sit S, Str &uri, DStr &script);
    eFlag callJSFunction(Sit S, Context *c, Str &uri, Str &name, 
			 ExprList &atoms, Expression &retxpr);
#endif
    StrStrList &getArgList() {return argList;};
    Str getNextNSPrefix();
    // resolve a global variable identified by name and possibly pointer
    // if varElem != NULL. sets 'name' accordingly 
    eFlag resolveGlobal(Sit S, Context *c, QName &name, XSLElement *varElem = NULL);
    // resolve all global vars before executing
    //eFlag resolveGlobals(Sit S, Context *c);
    eFlag stripTree(Sit S, Tree &tree);
    eFlag stripElement(Sit S, Daddy *e);
    Bool processingExternal() { return runsOnExternal; };
    void pushInBinding(Bool val);
    void popInBinding();
    Bool isInBinding();
    //eFlag pushDocumentDefinition(Sit S, OutputDefinition *def,
    //					 OutputterObj*& out);
    //eFlag popDocumentDefinition(Sit S);
    eFlag getOutputDocument(Sit S, Str& href, OutputDocument*& doc, 
			    OutputDefinition *def);
    eFlag startDocument(Sit S, OutputDocument *doc);
    eFlag finishDocument(Sit S, OutputDocument *doc, Bool canClose);
    eFlag prefixIsAliasTarget(Sit S, const Str& prefix, Bool& result);
private:
    int nsUnique;
    eFlag openCommon(Sit S);
    eFlag execApplyTemplates(Sit S, Context *c, Bool resolvingGlobals);
    eFlag execApply(Sit S, Context *c, Bool resolvingGlobals);
    PList<QName*> modes;
    StrStrList argList;
    DataLinesList datalines;
    PList<OutputterObj*> outputters_;
    //  handlers
    SchemeHandler *theSchemeHandler;
    MessageHandler *theMessageHandler;
    SAXHandler *theSAXHandler;
    MiscHandler *theMiscHandler;
    EncHandler *theEncHandler;
    void 
    *theSchemeUserData,
        *theMessageUserData,
        *theSAXUserData,    
        *theMiscUserData,
        *theEncUserData;    
    void *instanceData;
    void freeNonArgDatalines();
    StrStrList baseURIMappings, globalParamsList;
    Str hardEncoding;
    SabArena theArena;
    Situation *instanceSituation;
    Bool hasMasterSituation;
    Bool addedFlag;
    KeySet *keys;
    DecimalFormatList decimalsList;
#ifdef ENABLE_JS
    JSContextList jscontexts;
#endif
    Bool runsOnExternal;
    NodeHandle inputRoot;
    List<Bool> inBinding;
    //PList<OutputDefinition*> documentDefinitions;
    OutputDocumentList outputDocuments;
    SortedStringList outputDocumentURIs;
};

#endif //ifndef ProcHIncl
