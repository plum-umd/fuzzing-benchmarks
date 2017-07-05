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

#ifndef TreeHIncl
#define TreeHIncl

// GP: clean

#include "base.h"
#include "verts.h"
#include "datastr.h"
#include "output.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

class RootNode;

#define TREE_ARENA_SIZE      0x10000
#define TREE_DICT_LOGSIZE    10

//atts cache used for parsing
class AttsCache: public PList<Attribute*>
{
public: 
  AttsCache(): keep(FALSE) {};
  ~AttsCache() {if (!keep) freeall(FALSE);};
  void keepThem() { keep = TRUE; };
  Attribute* find(XSL_ATT what);
  Attribute* find(const QName& attName);
  int findNdx(const QName& attName);
private:
  Bool keep;
};

//_TH_ v

/****************************************
T m p L i s t
****************************************/

class TmpList : public PList<Vertex *>
{
public:
  TmpList();
  ~TmpList();
  void append(void *what);
  void rm(int n);
  void freeall(Bool a);
  //  void freeall(SablotSituation s);
  //  void dump(SablotSituation sit, int p=0);
  void dump(int p=0);
  int findNum(void *p) const;
  void rmP(void *p);
};
//_TH_ ^

class StylesheetStructure;

//
// SubtreeInfo
// holds information about a document which is (1) xsl:included,
// (2) xsl:imported or (3) included as external entity to form a part of 
// the tree. The SubtreeInfo records are linked to form a tree, each one
// holds the base URI, pointer to the associated StylesheetStructure
// (only exists for xsl:imports) and 'dependency type' which is 
// (1), (2) or (3) above (with values XSL_INCLUDE, XSL_IMPORT and XSL_NONE,
// respectively).
//

class SubtreeInfo
{
public:
    SubtreeInfo(const Str& baseURI_, XSL_OP dependency_,
		StylesheetStructure* structure_, Bool inline_) : 
#ifdef SABLOT_DEBUGGER
      nextBPIndex(-2),
#endif
      baseURI(baseURI_), dependency(dependency_), inlineVal(inline_),
      structure(structure_), parentSubtree(NULL), masterSubtree(NULL) {};
    ~SubtreeInfo();
    Str& getBaseURI()
	{ return baseURI; }
    XSL_OP getDependency()
	{ return dependency; }
    StylesheetStructure* getStructure()
	{ return structure; }
    SubtreeInfo* getParentSubtree()
	{ return parentSubtree; }
    void setParentSubtree(SubtreeInfo *parentSubtree_)
	{ parentSubtree = parentSubtree_; }
    SubtreeInfo* getMasterSubtree()
      { return masterSubtree ? masterSubtree : this; };
    void setMasterSubtree(SubtreeInfo *mstr)
      { masterSubtree = mstr; };
    Bool isInline() { return inlineVal; };
    UriList &getExcludedNS() { return excludedNS; };
    void pushNamespaceMarks();
    void popNamespaceMarks();
    UriList &getExtensionNS() { return extensionNS; };

    // this is not thread safe, but it is used by the debugger only,
    // what is single threaded by default
#ifdef SABLOT_DEBUGGER
    int nextBPIndex;
#endif

private:
    Str baseURI;
    XSL_OP dependency;
    Bool inlineVal;
    StylesheetStructure* structure;
    //was not used; now it hold pointer to the closest non-inline tree
    SubtreeInfo* parentSubtree;
    SubtreeInfo* masterSubtree;
    UriList excludedNS;
    UriList extensionNS;
    List<int> excludedCount;
    List<int> extensionCount;
};

// the list of subtrees
// although a list, it also has a rooted tree structure

class SubtreeList : public PList<SubtreeInfo*>
{
public:
    SubtreeList() 
	: currentSub(NULL)
	{
	};

    // append subtree to list, make it current in the tree structure
    void push(SubtreeInfo* newSub)
	{
	    append(newSub);
	    NZ(newSub) -> setParentSubtree(currentSub);
	    currentSub = newSub;
	};

    // go one level higher in the tree structure (but keep subtree in list!) 
    SubtreeInfo* pop()
	{
	    sabassert(currentSub);
		SubtreeInfo *was = currentSub;
	    currentSub = currentSub -> getParentSubtree();
		return was;
	};

    // get current subtree
    SubtreeInfo* getCurrent()
	{
	    return currentSub;
	}

    SubtreeInfo* findAmongPredecessors(const Str& uri)
	{
	    sabassert(currentSub);
	    for (SubtreeInfo* i = currentSub -> getParentSubtree(); 
		 i; 
		 i = i -> getParentSubtree() )
		if (i -> getBaseURI() == uri)
		    return i;
	    return NULL;
	}

private:
    SubtreeInfo* currentSub;    
};

//
//
//  VarDirectory
//  list of top-level variables and params used to find those of
//  highest import precedence, while checking for duplicates in
//  each source document
//

class VarDirectoryItem
{
public:
    VarDirectoryItem(QName &name_, XSLElement *varElem_)
	: varElem(varElem_), name(name_)
	{};
    XSLElement* getElement()
	{ return varElem; }
    void setElement(XSLElement *varElem_)
	{ varElem = varElem_; }
    const QName &getName()
	{ return name; }
    void setName(const QName &name_)
	{ name = name_; }
private:
    XSLElement *varElem;
    QName name;
};

class VarDirectory : public PList<VarDirectoryItem*>
{
public:
    VarDirectory();
    // kills all VarDirectoryItems
    ~VarDirectory();
    // find the var/param of the given QName (relative to the stylesheet
    // dictionary), NULL if not found
    XSLElement* find(QName& name);
    // insert the var/param (overwriting any previous entry of the same name)
    eFlag insert(Sit S, QName &name, XSLElement* var);
private:
    // find index of entry with the given QName, -1 if not found
    int findNdx(const QName& name);
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const
	{ S.message(type, code, arg1, arg2); }
};

class SpaceNameList : public PList<EQName*>
{
public:
  Bool findName(EQName &ename, double &prio);
};

/****************************************
StylesheetStructure
 
Holds information about the stylesheet tree (pointers
to template rules etc.)
Created when a document is parsed as stylesheet.
The 'imports' list points to structures for all
imported stylesheets (these form a tree). 
****************************************/

class StylesheetStructure
{
public:
    StylesheetStructure(int importPrecedence_)
	: importPrecedence(importPrecedence_), topLevelFound(false) {};
    ~StylesheetStructure()
	{
	    importChildren.freeall(FALSE);
	    // rulesList.freeall(FALSE); - done in ~RuleSList
	    strippedNamesList.freeall(FALSE);
	    preservedNamesList.freeall(FALSE);
	}

/***
    findBestRule 

    Finds the rule of highest import precedence and highest priority
    that is satisfied by the current node of the context c. If
    several rules are satisfied, returns the one that occurs last. 
    The current mode is given in currMode (may be NULL). The result,
    a pointer to the template rule node, is returned in ret.

    If no rule is found, asks the structures for imported stylesheets.
    If this doesn't yield a match too, returns NULL.

    If 'importsOnly' is true, looks only at the imported stylesheets.

    ASSUMPTION: 'rules' is sorted by priority in reverse order.
***/

    eFlag findBestRule(Sit S, XSLElement *&ret, Context *c,
		       QName *currMode, Bool importsOnly);

    // finds the rule of given name with highest import precedence
    // 't' is the stylesheet
    XSLElement* findRuleByName(Tree& t, QName &q);

    // return the list of template rules from this structure
    RuleSList &rules()
	{ return rulesList; }

    // insert a template rule into 'rulesList'
    // the rule is given as a pointer to the corresponding xsl:template
    eFlag insertRule(Sit S, XSLElement *tmpl);

    // add structure for imported stylesheet
    void addImportStructure(Sit S, StylesheetStructure *imported)
      //{ importChildren.append(imported); }
      { importChildren.insertBefore(imported, 0); }

    // get the import precedence associated with this imported subtree
    int getImportPrecedence()
      { return importPrecedence; }

    //set the import precedence
    void setImportPrecedence(int prec)
      {	importPrecedence = prec; }

    SpaceNameList &strippedNames() { return strippedNamesList; }
    SpaceNameList &preservedNames() { return preservedNamesList; }
    Bool findStrippedName(EQName &ename, int &prec, double &prio);
    Bool findPreservedName(EQName &ename, int &prec, double &prio);
    Bool hasAnyStripped();
    Bool hasAnyPreserved();
    Bool getTopLevelFound() { return topLevelFound; };
    void setTopLevelFound(Bool val) { topLevelFound = val; }
private:
    PList<StylesheetStructure*> importChildren;
    RuleSList rulesList;
    SpaceNameList strippedNamesList;
    SpaceNameList preservedNamesList;
    int importPrecedence;
    Bool topLevelFound; //used during the xslt parse only, thread safe
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const
	{ S.message(type, code, arg1, arg2); }
};

/****************************************************************
 
AttributeSets

*****************************************************************/

class AttSetMember
{
public:
    AttSetMember(QName &attName_)
	: attDef(NULL), redefinition(NULL), 
      attName(attName_), precedence(-1) {};
    XSLElement* getAttributeDef()
	{return attDef;}
    // see if there was multiple definition with highest import precedence
    XSLElement* getRedefinition()
	{return redefinition;}

    QName &getAttName()
	{ return attName; }
    // sets to a new value if importPrecedence is smaller (stronger)
    // sets 'redefinition' if equal precedence
    void set(XSLElement *newAttDef);
private:
    XSLElement 
	*attDef,
	*redefinition;
    QName attName;
    int precedence;
};

class AttSet : public PList<AttSetMember*>
{
public:
    AttSet(QName &name_);
    ~AttSet();
    void insertAttributeDef(XSLElement *attDef, QName &attName);
    void insertUses(QName &usedSet);
    const QName& getName()
	{ return name; }
    eFlag checkRedefinitions(Sit S);
    // execute the attribute set
    eFlag execute(Sit S, Context *c, Tree& sheet, 
		  QNameList& history,  Bool resolvingGlobals);
    int findNdx(QName &attName);
private:
    QName name;
    QNameList usedSets;
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const
	{ S.message(type, code, arg1, arg2); }
};

class AttSetList : public PList<AttSet*>
{
public:
    AttSetList();
    ~AttSetList();
    // check all attribute sets for multiply defined attributes
    eFlag checkRedefinitions(Sit S);
    // insert an AttSet named 'name' and return pointer (the set may already exist)
    AttSet* insert(QName& name);
    eFlag executeAttSet(Sit S, QName &name, Context *c, Tree &sheet, 
			QNameList& history, Bool resolvingGlobals);
    AttSet* findByName(const QName &name) const
	{ int ndx = findNdx(name); return (ndx == -1) ? NULL : (*this)[ndx]; }
private:
    int findNdx(const QName &name) const;
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const
	{ S.message(type, code, arg1, arg2); }
};

//
//
//  AliasItem
//  records one alias for use in AliasPrecList
//  holds the key-value pair, checks import precedence on setting
//
//

class AliasItem
{
public:
    AliasItem()
      :key(UNDEF_PHRASE), value(UNDEF_PHRASE), prefix(UNDEF_PHRASE), 
      precedence(-1), redefinition(NULL) {};
    Phrase getKey() {return key;}
    Phrase getValue() {return value;}
    Phrase getPrefix() {return prefix;}
      
    // see if there was multiple definition with highest import precedence
    XSLElement* getRedefinition()
	{return redefinition;}
    // sets to a new value if newPrecedence is smaller (stronger)
    // sets 'redefinition' if precedences were equal (typically an error)
    // but sets key/value anyway
    void set(Phrase newKey, Phrase newValue, Phrase newPrefix,
	     int newPrecedence, XSLElement *source);
private:
    Phrase key, value, prefix;
    int precedence;
    XSLElement *redefinition;
};

//
//
//  AliasList
//  list of namespace aliases defined by xsl:namespace-alias
//  checks import precedence when inserting
//
//

class AliasList : public PList<AliasItem*>
{
public:
    // find the value of the element with the given key
    Phrase find(Phrase) const;
    // insert an alias (or overwrite old if precedence is not weaker)
    void insertAlias(Phrase key, Phrase value, Phrase prefix, int precedence, 
		     XSLElement *source);
    eFlag checkRedefinitions(Sit S, Tree &sheet);
private:
    // find the index of the element with the given key
    int findNdx(Phrase) const;
    void report(Sit S, MsgType type, MsgCode code, const Str &arg1, 
		const Str &arg2) const
	{ S.message(type, code, arg1, arg2); }
};




/****************************************
T r e e

Represents a XML tree in memory. Created by parsing a document.
A tree can be used in several processing sessions and even by
several threads, so it SHOULD NOT CHANGE during processing (in other
words, it holds information which is static with respect to processing).
****************************************/

class Tree
{
public:
    // pass the URI of the tree's document
    // isXSL_ = TRUE iff the tree represents a stylesheet
    Tree(const Str& uri, Bool isXSL_);
    ~Tree();
    BOOL XSLTree;
    eFlag appendVertex(Sit S, Vertex *);
    Vertex* popVertex();
    Vertex* appendText(Sit S, char *, int);
    eFlag parseFinished(Sit S);
    void dump();
    Vertex *stackTop;
    NSList &pendingNS() {return *(pendingNSList.last());};
    void dropCurrentElement(Vertex *);
    void flushPendingText();
    Element& dummyElement() const;
    HashTable& dict();
    SabArena& getArena();
    const QName& getTheEmptyQName() const;
    Phrase stdPhrase(StandardPhrase phrase_) const 
    { 
        sabassert(phrase_ <= PHRASE_LAST); 
	    return stdPhrases[phrase_]; 
    };
    Bool cmpQNames(const QName &q1, const QName &q2) const;
    Bool cmpQNamesForeign(const QName &q, const HashTable& dictForeign, const QName &qForeign);
    Bool cmpQNameStrings(const QName &q, const Str& uri, const Str& local);
    void expandQ(const QName& q, EQName &expanded);
    void expandQStr(const QName& q, Str &expName);
    const Str& expand(Phrase ph);
    Phrase unexpand(const Str& strg);
    RootNode& getRoot() const {return* NZ(root);} 
    eFlag serialize(Sit S, char *& result);
    eFlag serializeNode(Sit S, Element *v, char *& result);
    void makeStamps();

    //// methods to access stylesheet structure

    // finds the best matching rule
    // see StylesheetStructure::findBestRule for description
    eFlag findBestRule(Sit S, XSLElement *&ret, Context *c,
		       QName *currMode, Bool importsOnly, 
		       SubtreeInfo *subtree = NULL);

    // finds the rule of given name with highest import precedence
    XSLElement* findRuleByName(QName &q);


    // returns the list of attribute sets defined
    AttSetList &attSets()
	{ return attSetList; }

    // The list of namespace aliases
    AliasList &aliases()
	{ return aliasesList; }

    // Output definition holding compiled information from various
    //   xsl:output elements
    // At runtime, this is copied by Processor and defaults are provided
    //   FOR THE COPY (cannot be guessed at parse time)
    OutputDefinition outputDef;
    
    // this is called after each element has been parsed in
    // used to compile StylesheetStructure (template rule pointers), process
    //   includes/imports, etc.
    eFlag processVertexAfterParse(Sit S, Vertex *v, TreeConstructer* tc);

    // creates this tree by parsing the given dataline
    eFlag parse(Sit S, DataLine *d);

    // returns in 'result' the list of elements matching the given expression
    // used for key creation
    eFlag getMatchingList(Sit S, Expression& match, Context& result);

    // find global variable in directory
    XSLElement* findVarInDirectory(QName &name)
	{ return toplevelVars.find(name); }

    // the list of temporary nodes which belong to the tree but are not part
    // of it (used for cleaning up in SDOM)
    TmpList tmpList;
    
    // EXTENSION NAMESPACES
    // FIXME: efficiency?
    // FIXME: extension URIs are only valid outside imports/includes!!
    Bool isExtensionUri(Phrase uri);
    // MAIN METHODS TO RECORD THE PARSING OF EXTERNAL ENTITIES/INCLUDES/IMPORTS
    // start a subtree, recording its base URI. 
    // 'dependency' is XSL_NONE (external entity), XSL_INCLUDE or XSL_IMPORT
    eFlag startSubtree(Sit S, const Str& baseURI, XSL_OP dependency,
		       Bool isInline = FALSE);
    // end a subtree
    eFlag endSubtree(Sit S, XSL_OP dependency);
    // get the tree's URI
    const Str& getURI()
      {
	return subtrees[0] -> getBaseURI();
      }
    // resolve all global variables before starting execute
    // called from Processor::resolveGlobals
    //eFlag resolveGlobals(Sit S, Context *c, Processor *proc);
    //unparsed entties storge
    void setUnparsedEntityUri(Str &name, Str &uri);
    Str* getUnparsedEntityUri(Str &name);
    int stripped;
    Bool hasAnyStrippedName();
    Bool hasAnyPreservedName();
    Bool findPreservedName(EQName &name, int &prec, double &prio);
    Bool findStrippedName(EQName &name, int &prec, double &prio);
    Element* findStylesheet(Daddy& d);
    eFlag pushPendingNS(Sit S, Tree* srcTree, NSList &other);
    eFlag popPendingNS(Sit S);
    SubtreeInfo* getRootSubtree() { return subtrees[0]; }
    SubtreeInfo* getCurrentInfo() 
      {return subtrees.getCurrent()->getMasterSubtree();}
    eFlag markNamespacePrefixes(Sit S);
    eFlag pushNamespacePrefixes(Sit, Str&, XSL_ATT);
    eFlag popNamespacePrefixes(Sit);
    void updateImportStatus();
    void speakDebug();
    void dumpStructure(Sit S);
private:
    SabArena theArena;
    Text *pendingTextNode;
    DStr pendingText;
    int vcount;             // # of vertices in tree
    Element *theDummyElement;
    HashTable theDictionary;
    PList<NSList*> pendingNSList;
    const QName theEmptyQName;
    Phrase stdPhrases[PHRASE_LAST];
    void initDict();
    RootNode *root;
    RuleSList rulesList;
    StrStrList unparsedEntities;
    eFlag insertRule(Sit S, XSLElement *tmpl);
    eFlag insertAttSet(Sit S, XSLElement *tmpl);
    double defaultPriorityLP(Expression *);
    double defaultPriority(XSLElement *);      
    void report(Sit S, MsgType type, MsgCode code, const Str &arg1, const Str &arg2) const
	{ S.message(type, code, arg1, arg2); }
    eFlag getSpaceNames(Sit S, Element &e, Str &str, SpaceNameList &where);
    eFlag extractUsedSets(Sit S, Element *e);
    void excludeStdNamespaces();
    // create a new structure
    StylesheetStructure* createStylesheetStructure(Sit S);

    // the structure information about the stylesheet
    // (separated for import purposes)
    StylesheetStructure structure;

    // list of subtrees 
    SubtreeList subtrees;    
    AttSetList attSetList;
    AliasList aliasesList;
    VarDirectory toplevelVars;
    int hasAnyStripped, hasAnyPreserved;
    int importUnique;
};

#endif //ifndef TreeHIncl


