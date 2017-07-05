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

#ifndef VertsHIncl
#define VertsHIncl

// GP: clean

#include "base.h"
#include "arena.h"
#include "datastr.h"
#include "expr.h"
#include "hash.h"
// #include "tree.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef SABLOT_DEBUGGER
#include "debugger.h"
#endif

class Tree;

class Context;
class Expression;
class OutputterObj;
class OutputDefinition;
class AttList;

typedef void *NodeHandle;
class SubtreeInfo;

/****************************************
V e r t e x
****************************************/

class Vertex : public SabArenaMember
{
public:
    Vertex(Tree& owner_, VTYPE avt = VT_VERTEX_WF)
      : owner(owner_), vt(avt), parent(NULL), ordinal(0), subtree(NULL),
	outputDocument(NULL)
    {
        stamp = lineno = 0;
	    instanceData = NULL;
    };
    virtual ~Vertex();
    virtual eFlag execute(Sit S, Context *, Bool resolvingGlobals);
    virtual eFlag value(Sit S, DStr &, Context *);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual eFlag endCopy(Sit S, OutputterObj& out);
    virtual eFlag copy(Sit S, OutputterObj& out);
    virtual void speak(DStr &, SpeakMode);
    void setParent(Vertex *v);
    Vertex* getPreviousSibling();
    Vertex* getNextSibling();
    virtual const QName& getName() const;
    Tree& getOwner() const;
    virtual eFlag serialize(Sit S, OutputterObj &out);
    virtual eFlag getMatchingList(Sit S, Expression& match, Context& result);

    Tree &owner;
    VTYPE vt;
    Vertex *parent;
    int ordinal;
    SubtreeInfo* subtree;
    int lineno;
    int stamp;          // the vertex number in doc order (0 = root)
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2) const;
    HashTable& dict() const;
    void setInstanceData(void *data) {instanceData = data;}
    void* getInstanceData() {return instanceData;}
    virtual void makeStamps(int& stamp);

    // set the subtree information (base URI etc.)
    void setSubtreeInfo(SubtreeInfo* subtree_)
	{ subtree = subtree_; }
    // get the subtree information 
    SubtreeInfo* getSubtreeInfo() const
	{ return subtree; }
    int getImportPrecedence();
    void setOutputDocument(OutputDocument *doc) 
      {outputDocument = doc;}
    OutputDocument* getOutputDefinition() {return outputDocument;};
protected:
    eFlag startDocument(Sit S, OutputterObj*& out);
    eFlag finishDocument(Sit S);
private:
    void *instanceData;    // for Perl interface
    // this is set in Tree::appendVertex
    OutputDocument *outputDocument;
};

/****************************************
V e r t e x L i s t
****************************************/

// FIXME: changed PList to SList...

class VertexList: public SList<Vertex*>
{
public:
    VertexList(int logBlockSize_ = LIST_SIZE_SMALL);
    virtual ~VertexList();
    eFlag execute (Sit S, Context*, Bool resolvingGlobals);
    eFlag value(Sit S, DStr&, Context *);
    virtual void speak(DStr &, SpeakMode);
    virtual void append(Vertex *);
    void rm(int ndx);
    void destructMembers();
    int strip();
    eFlag copy(Sit S, OutputterObj& out);
    void insertBefore(Vertex *newChild, int refIndex);
    int getIndex(Vertex *v);
    eFlag serialize(Sit S, OutputterObj &out);
    void makeStamps(int& stamp);
    eFlag getMatchingList(Sit S, Expression& match, Context& result);
};

/****************************************
A r e n a V e r t e x L i s t
****************************************/

class SabArenaVertexList : public VertexList, public SabArenaMember
{
public:
    SabArenaVertexList(SabArena *arena__=NULL, int logBlockSize_ = LIST_SIZE_SMALL)
        : VertexList(logBlockSize_), arena_(arena__) 
    {
    };

    ~SabArenaVertexList()
    {
        deppendall();
    }

protected:
    virtual Vertex** claimMemory( int nbytes ) const 
    { 
        return arena_ ? (Vertex**)(arena_ -> armalloc(nbytes)) : 
            (Vertex**)VertexList::claimMemory(nbytes); 
    }

    virtual Vertex** reclaimMemory( Vertex**p, int newbytes, int oldbytes ) const 
    {
        if (!arena_)
            return VertexList::reclaimMemory(p, newbytes, oldbytes);
        else if (newbytes > oldbytes)
        {
            Vertex **newpos = (Vertex**) (arena_ -> armalloc(newbytes));
            memcpy(newpos, p, oldbytes);
            return newpos;
        }
        else
            return p;
    }

    virtual void returnMemory( Vertex**&p ) const 
    {
        if (arena_)
            arena_ -> arfree(p);
        else
            if (p) VertexList::returnMemory(p); 
        p = NULL;
    }
    SabArena *arena_;
};


/****************************************
D a d d y
****************************************/

class Daddy : public Vertex
{
public:
    Daddy(Tree& owner_, VTYPE avt = VT_DADDY_WF);
    virtual ~Daddy(void);
    virtual eFlag execute(Sit S, Context *c, Bool resolvingGlobals);
   	virtual eFlag value(Sit S, DStr &, Context *);
    virtual eFlag newChild(Sit S, Vertex*);
    virtual eFlag checkChildren(Sit S);
    virtual void speak(DStr &, SpeakMode);
    virtual int strip();
    virtual eFlag getMatchingList(Sit S, Expression& match, Context& result);
    //
    SabArenaVertexList contents;
};

/****************************************
A t t r i b u t e
****************************************/

class Attribute : public Vertex
{
public:
    Attribute(Tree& owner_, const QName&, const Str&, XSL_ATT);
    virtual ~Attribute();
    eFlag buildExpr(Sit S, Bool, ExType);
    virtual eFlag execute(Sit S, Context *c, Bool resolvingGlobals);
    virtual void speak(DStr &, SpeakMode);
    virtual eFlag value(Sit S, DStr &, Context *);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual const QName& getName() const;
    void setValue(const Str& newValue);
    virtual eFlag serialize(Sit S, OutputterObj &out);
    QName name;
    SabArenaStr cont;
    Expression *expr;
    XSL_ATT op;
};

/****************************************
A t t L i s t
****************************************/

class AttList: public SabArenaVertexList
{
public:
    AttList(SabArena *arena_)
        : SabArenaVertexList(arena_)
    {};
    Attribute* find(XSL_ATT);
    // assumes that attName belongs to the same hash
    Attribute* find(const QName& attName);
    int findNdx(const QName& attName);
};

/*****************************************************************
    N m S p a c e
*****************************************************************/
typedef enum 
{
    NSKIND_PARENT = 0,   // namespace is NOT explicitly declared by owning element - for instance inherited via namespace scoping
    NSKIND_DECLARED = 1  // namespace is explicitly declared by owning element
} NsKind;

class NmSpace: public Vertex
{
public:
  //  NmSpace(Tree& owner_, Phrase _prefix, Phrase _uri, NsKind _kind);
    NmSpace(Tree& owner_, Phrase _prefix, Phrase _uri, 
	    Bool _excluded = FALSE, NsKind _kind = NSKIND_PARENT);
    virtual ~NmSpace();
    Phrase
        prefix,
        uri;
    QName name;
    NsKind kind; 
    Bool excluded;
    unsigned int usageCount;
    virtual eFlag execute(Sit S, Context *, Bool resolvingGlobals);
    virtual eFlag executeSkip(Sit S, Context *, Bool resolvingGlobals,
			      EQName & exName, Bool aliased);
    virtual void speak(DStr &, SpeakMode);
   	virtual eFlag value(Sit S, DStr &, Context *);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual const QName& getName() const;
    virtual eFlag serialize(Sit S, OutputterObj &out);
    NsKind setKind(NsKind kind_);
};

/*****************************************************************
    N S L i s t
*****************************************************************/

class NSList : public SabArenaVertexList
{
public:
  NSList (SabArena* arena_ = NULL)
    : SabArenaVertexList(arena_,0)
  { }
  
  ~NSList();
  NmSpace* find(Phrase) const;
  int findNdx(Phrase prefix) const;
  eFlag resolve(Sit S, Phrase&, Bool) const;
  void unresolve(Phrase&) const;
  void giveCurrent(Sit S, NSList &, Tree*, int nscount) const;
  void swallow(Sit S, NSList &other, Tree *strTree, Tree *t);
  eFlag executeSkip(Sit S, Context *, Bool resolvingGlobals,
		    EQName & exName, Bool aliased);
  // unresolve the qname's prefix
  void findPrefix(QName &q);
  void report(Sit S, MsgType type, MsgCode code, 
	      const Str &arg1, const Str &arg2) const;
  void setPrefixKind(Phrase prefix_, NsKind kind_) const;
  void incPrefixUsage(Phrase prefix_) const;
  void decPrefixUsage(Phrase prefix_) const;
};


/****************************************
E l e m e n t
****************************************/

class Element : public Daddy
{
public:
    Element(Tree& owner_, QName&, VTYPE = VT_ELEMENT_WF);
    virtual ~Element();
    virtual int strip();
    virtual eFlag execute(Sit S, Context *c, Bool resolvingGlobals);
    virtual eFlag executeFallback(Sit S, Context *c, Bool &hasSome,
				  Bool resolvingGlobals);
    virtual eFlag newChild(Sit S, Vertex*);
    virtual void speak(DStr &, SpeakMode);
    void removeBindings(Sit S);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual eFlag endCopy(Sit S, OutputterObj& out);
    virtual eFlag copy(Sit S, OutputterObj& out);
    virtual const QName& getName() const;
    virtual eFlag serialize(Sit S, OutputterObj &out);
    virtual eFlag serializeSubtree(Sit S, OutputterObj &out);
    void removeChild(Vertex *child);
    virtual void makeStamps(int& stamp);
    virtual eFlag getMatchingList(Sit S, Expression& match, Context& result);
    
    // set qname to "prefix:local". Last param says
    // whether to expand the default namespace
    eFlag setLogical(Sit S, QName &q, const Str&, 
		     Bool defaultToo, Phrase defUri = UNDEF_PHRASE) const;
    //
    NSList
        namespaces;
    AttList 
        atts;
    QName
        name;
    QNameList* attSetNames(Bool);
    eFlag executeAttributeSets(Sit S, Context *c, Bool resolvingGlobals);
    Bool preserveSpace;
#ifdef SABLOT_DEBUGGER
    Breakpoint *breakpoint;
#endif
protected:
    QNameList *attsNames;
private:
};

/****************************************
R o o t N o d e
****************************************/

class RootNode : public Element
{
public:
    RootNode(Tree& owner_, QName& name_)
        : Element(owner_, name_, VT_ROOT_WF)
    {
    };
    virtual ~RootNode();
    virtual eFlag execute(Sit S, Context *c, Bool resolvingGlobals);
    virtual eFlag newChild(Sit S, Vertex*);
    virtual eFlag checkChildren(Sit S);
    virtual void speak(DStr &, SpeakMode);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual eFlag endCopy(Sit S, OutputterObj& out);
    virtual eFlag copy(Sit S, OutputterObj& out);
    virtual eFlag serialize(Sit S, OutputterObj &out);
};

/****************************************
T e x t
****************************************/

class Text : public Vertex
{
public:
    SabArenaStr cont;
    Text(Tree& owner_, char *acont, int alen=0);
    virtual ~Text();
    virtual eFlag execute(Sit S, Context *, Bool resolvingGlobals);
    virtual void speak(DStr &, SpeakMode);
   	virtual eFlag value(Sit S, DStr &, Context *);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual eFlag serialize(Sit S, OutputterObj &out);
    void beCDATA();
    Bool isCDATA(); 
private:
    Bool isCDATAFlag;
};


/****************************************
C o m m e n t
****************************************/

class Comment: public Vertex
{
public:
    Comment(Tree& owner_, const Str& cont_);
    virtual ~Comment();
    virtual eFlag execute(Sit S, Context *, Bool resolvingGlobals);
    virtual void speak(DStr &, SpeakMode);
   	virtual eFlag value(Sit S, DStr &, Context *);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual eFlag serialize(Sit S, OutputterObj &out);
    SabArenaStr cont;
};


/****************************************
P r o c I n s t r
****************************************/

class ProcInstr: public Vertex
{
public:
    ProcInstr(Tree& owner_, Phrase name_, const Str& cont_);
    virtual ~ProcInstr();
    virtual eFlag execute(Sit S, Context *, Bool resolvingGlobals);
    virtual void speak(DStr &, SpeakMode);
   	virtual eFlag value(Sit S, DStr &, Context *);
    virtual eFlag startCopy(Sit S, OutputterObj& out);
    virtual const QName& getName() const;
    virtual eFlag serialize(Sit S, OutputterObj &out);
    SabArenaStr cont;
    QName name;
};


/****************************************
X S L E l e m e n t
****************************************/

class XSLElement : public Element
{
public:
    XSLElement(Tree& owner_, QName&, XSL_OP);
    virtual eFlag execute(Sit S, Context *c, Bool resolvingGlobals);
    virtual eFlag newChild(Sit S, Vertex*);
    eFlag checkToplevel(Sit S);
    virtual eFlag checkChildren(Sit S);
    void checkExtraChildren(int& k);
    Expression* getAttExpr(XSL_ATT);
    virtual int strip();
    XSL_OP op;
    eFlag checkAtts(Sit S);       
//    VertexList defs;
private:
    // applies only to XSL_APPLY_TEMPLATES and XSL_FOR_EACH:
    eFlag makeSortDefs(Sit S, SortDefList &sortDefs, Context *c);
    // applies only to XSL_SORT:
    eFlag make1SortDef(Sit S, SortDef *&def, Context *c);
};

/****************************************
X S L E l e m e n t
****************************************/
enum ExtNamespace {
  EXTNS_EXSLT_FUNCTIONS,
  EXTNS_EXSLT_FUNCTIONS_2,
  EXTNS_EXSLT_COMMON,
  EXTNS_UNKNOWN
};

enum ExtElement {
  //exslt.org/functions
  EXTE_EXSLT_FUNCTIONS = 0,
  EXTE_EXSLT_SCRIPT = 0,
  //exsl.org/common
  EXTE_EXSLT_COMMON = 100,
  EXTE_EXSLT_DOCUMENT = 100,
  EXTE_UNKNOWN
};

class ExtensionElement : public Element
{
public:  
  ExtensionElement(Tree& owner_, QName& aqname);
  virtual eFlag execute(Sit S, Context *c, Bool resolvingGlobals);
  virtual eFlag checkChildren(Sit S);
  eFlag checkAtts(Sit S);       
  static Bool elementAvailable(Tree &t, QName &name);
private:
  static void lookupExt(Tree &t, QName &name, 
			ExtNamespace &extns_, ExtElement &op_);
#ifdef ENABLE_JS
  eFlag executeEXSLTScript(Sit S, Context *c, Bool resolvingGlobals);
#endif
  eFlag executeEXSLTDocument(Sit S, Context *c, Bool resolvingGlobals);
  eFlag checkHasAttr(Sit S, const char *name);
  eFlag exsltDocGetOutputterDef(Sit S, Context *c, OutputDefinition &def);
  ExtNamespace extns;
  ExtElement op;
};

#endif //ifndef VertsHIncl

