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

#ifndef DomProviderHIncl
#define DomProviderHIncl

#include "base.h"
#include "sxpath.h"

class OutputterObj;
class Expression;
class Context;

class DOMProvider
{
public:
    virtual SXP_NodeType getNodeType(SXP_Node n) = 0;
    virtual const SXP_char* getNodeName(SXP_Node n) = 0;
    virtual const SXP_char* getNodeNameURI(SXP_Node n) = 0;
    virtual const SXP_char* getNodeNameLocal(SXP_Node n) = 0;
    virtual const SXP_char* getNodeValue(SXP_Node n) = 0;
    virtual SXP_Node     getNextSibling(SXP_Node n) = 0;
    virtual SXP_Node     getPreviousSibling(SXP_Node n) = 0;
    virtual SXP_Node     getNextAttrNS(SXP_Node n) = 0;
    virtual SXP_Node     getPreviousAttrNS(SXP_Node n) = 0;
    virtual int          getChildCount(SXP_Node n) = 0;
    virtual int          getAttributeCount(SXP_Node n) = 0;
    virtual int          getNamespaceCount(SXP_Node n) = 0;
    virtual SXP_Node     getChildNo(SXP_Node n, int ndx) = 0;
    virtual SXP_Node     getAttributeNo(SXP_Node n, int ndx) = 0;
    virtual SXP_Node     getNamespaceNo(SXP_Node n, int ndx) = 0;
    virtual SXP_Node     getParent(SXP_Node n) = 0;
    virtual SXP_Document getOwnerDocument(SXP_Node n) = 0;    
    virtual int          compareNodes(SXP_Node n1, SXP_Node n2) = 0;
    virtual SXP_Document retrieveDocument(const SXP_char* uri, const SXP_char* baseUri) = 0;    
    void                 constructStringValue(SXP_Node n, DStr &result);
    SXP_Node             getFirstChild(SXP_Node n);
    virtual SXP_Node     getNodeWithID(SXP_Document doc, 
				       const SXP_char* id) = 0;
    //extended function
    virtual void         freeName(NodeHandle n, char* buff);
    virtual void         freeValue(NodeHandle n, char* buff);
    //special functions for processing
    virtual eFlag startCopy(Sit S, NodeHandle n, OutputterObj &outputter);
    virtual eFlag endCopy(Sit S, NodeHandle n, OutputterObj &outputter);
    virtual eFlag copyNode(Sit S, NodeHandle n, OutputterObj &outputter);
    virtual eFlag getMatchingList(Sit S, NodeHandle n, 
				  Expression &match, Context &result);
};

class DOMProviderStandard : public DOMProvider
{
public:
    virtual SXP_NodeType getNodeType(SXP_Node n);
    virtual const SXP_char* getNodeName(SXP_Node n);
    virtual const SXP_char* getNodeNameURI(SXP_Node n);
    virtual const SXP_char* getNodeNameLocal(SXP_Node n);
    virtual const SXP_char* getNodeValue(SXP_Node n);
    virtual SXP_Node     getNextSibling(SXP_Node n);
    virtual SXP_Node     getPreviousSibling(SXP_Node n);
    virtual SXP_Node     getNextAttrNS(SXP_Node n);
    virtual SXP_Node     getPreviousAttrNS(SXP_Node n);
    virtual int          getChildCount(SXP_Node n);
    virtual int          getAttributeCount(SXP_Node n);
    virtual int          getNamespaceCount(SXP_Node n);
    virtual SXP_Node     getChildNo(SXP_Node n, int ndx);
    virtual SXP_Node     getAttributeNo(SXP_Node n, int ndx);
    virtual SXP_Node     getNamespaceNo(SXP_Node n, int ndx);
    virtual SXP_Node     getParent(SXP_Node n);
    virtual SXP_Document getOwnerDocument(SXP_Node n);    
    virtual int          compareNodes(SXP_Node n1, SXP_Node n2);
    virtual SXP_Document retrieveDocument(const SXP_char* uri, const SXP_char* baseUri);    
    virtual SXP_Node     getNodeWithID(SXP_Document doc, 
				       const SXP_char* id);
    //extras
    virtual void freeName(NodeHandle n, char *buff)
      { delete[] buff; }
    virtual void freeValue(NodeHandle n, char *buff)
      { /*does nothing */}
};

#define NHC(nh) ((unsigned long)nh)
#define SXP_EXTERNAL(nh) (NHC(nh) & 1)
#define SXP_CLEAR(nh) (NHC(nh) & ~1)
#define SXP_MASK_LEVEL(nh,level) (NodeHandle)(((NHC(nh) & 1) << (level)) | (NHC(nh) & ~(1 << (level))) | 1)
#define SXP_UNMASK_LEVEL(nh,level) (NodeHandle)((((NHC(nh) >> (level)) & 1) | NHC(nh) & ~1) & ~(1 << (level)))
#define SXP_MASK(nh) SXP_MASK_LEVEL((nh),sxpMask)
#define SXP_UNMASK(nh) SXP_UNMASK_LEVEL((nh),sxpMask)

//handy macros used in comon code
#define nhNull(nh) (!SXP_CLEAR(nh))

class DOMProviderExternal : public DOMProvider
{
public:
    DOMProviderExternal(DOMHandler *domh_, void* udata_) 
	: domh(domh_), udata(udata_), sxpMask(0), options(0) {};
	virtual SXP_NodeType getNodeType(SXP_Node n)
	  { return domh -> getNodeType 
		  ? domh -> getNodeType(SXP_UNMASK(n)) 
		  : domh -> getNodeTypeExt(SXP_UNMASK(n), udata); }

    virtual const SXP_char* getNodeName(SXP_Node n)
	  { return domh -> getNodeName 
		  ? domh -> getNodeName(SXP_UNMASK(n)) 
		  :	domh -> getNodeNameExt(SXP_UNMASK(n), udata);}

    virtual const SXP_char* getNodeNameURI(SXP_Node n)
	  { return domh -> getNodeNameURI 
		  ? domh -> getNodeNameURI(SXP_UNMASK(n)) 
		  :	domh -> getNodeNameURIExt(SXP_UNMASK(n), udata); }

    virtual const SXP_char* getNodeNameLocal(SXP_Node n)
	  { return domh -> getNodeNameLocal 
		  ? domh -> getNodeNameLocal(SXP_UNMASK(n)) 
		  :	domh -> getNodeNameLocalExt(SXP_UNMASK(n), udata); }

    virtual const SXP_char* getNodeValue(SXP_Node n)
	  { return domh -> getNodeValue 
		  ? domh -> getNodeValue(SXP_UNMASK(n)) 
		  :	domh -> getNodeValueExt(SXP_UNMASK(n), udata); }

    virtual SXP_Node     getNextSibling(SXP_Node n)
	  { return domh -> getNextSibling  
		  ? SXP_MASK( domh -> getNextSibling(SXP_UNMASK(n)) )
		  : SXP_MASK( domh -> getNextSiblingExt(SXP_UNMASK(n), udata) ); }

    virtual SXP_Node     getPreviousSibling(SXP_Node n)
	  { return domh -> getPreviousSibling 
		  ? SXP_MASK( domh -> getPreviousSibling(SXP_UNMASK(n)) ) 
		  : SXP_MASK( domh -> getPreviousSiblingExt(SXP_UNMASK(n), udata) ); }

    virtual SXP_Node     getNextAttrNS(SXP_Node n)
	  { return domh -> getNextAttrNS 
		  ? SXP_MASK( domh -> getNextAttrNS(SXP_UNMASK(n)) )
		  : SXP_MASK( domh -> getNextAttrNSExt(SXP_UNMASK(n), udata) ); }

    virtual SXP_Node     getPreviousAttrNS(SXP_Node n)
	  { return domh -> getPreviousAttrNS 
		  ? SXP_MASK( domh -> getPreviousAttrNS(SXP_UNMASK(n)) )
		  : SXP_MASK( domh -> getPreviousAttrNSExt(SXP_UNMASK(n), udata) ); }

    virtual int          getChildCount(SXP_Node n)
	  { return domh -> getChildCount 
		  ? domh -> getChildCount(SXP_UNMASK(n)) 
		  : domh -> getChildCountExt(SXP_UNMASK(n), udata); }

    virtual int          getAttributeCount(SXP_Node n)
	  { return domh -> getAttributeCount 
		  ? domh -> getAttributeCount(SXP_UNMASK(n)) 
		  : domh -> getAttributeCountExt(SXP_UNMASK(n), udata); }

    virtual int          getNamespaceCount(SXP_Node n)
	  { return domh -> getNamespaceCount 
		  ? domh -> getNamespaceCount(SXP_UNMASK(n)) 
		  : domh -> getNamespaceCountExt(SXP_UNMASK(n), udata); }

    virtual SXP_Node     getChildNo(SXP_Node n, int ndx)
	  { return domh -> getChildNo 
		  ? SXP_MASK( domh -> getChildNo(SXP_UNMASK(n), ndx) )
		  : SXP_MASK( domh -> getChildNoExt(SXP_UNMASK(n), ndx, udata) ); }

    virtual SXP_Node     getAttributeNo(SXP_Node n, int ndx)
	  { return domh -> getAttributeNo 
		  ? SXP_MASK( domh -> getAttributeNo(SXP_UNMASK(n), ndx) )
		  :SXP_MASK(  domh -> getAttributeNoExt(SXP_UNMASK(n), ndx, udata) ); }

    virtual SXP_Node     getNamespaceNo(SXP_Node n, int ndx)
	  { return domh -> getNamespaceNo 
		  ? SXP_MASK( domh -> getNamespaceNo(SXP_UNMASK(n), ndx) )
		  : SXP_MASK( domh -> getNamespaceNoExt(SXP_UNMASK(n), ndx, udata) ); }

    virtual SXP_Node     getParent(SXP_Node n)
	  { return domh -> getParent 
		  ? SXP_MASK( domh -> getParent(SXP_UNMASK(n)) )
		  : SXP_MASK( domh -> getParentExt(SXP_UNMASK(n), udata) ); }

    virtual SXP_Document getOwnerDocument(SXP_Node n)
	  { return domh -> getOwnerDocument
		  ? SXP_MASK( domh -> getOwnerDocument(SXP_UNMASK(n)) )
		  : SXP_MASK( domh -> getOwnerDocumentExt(SXP_UNMASK(n), udata) ); }

    virtual int          compareNodes(SXP_Node n1, SXP_Node n2)
	  { return domh -> compareNodes 
		  ? domh -> compareNodes(SXP_UNMASK(n1), SXP_UNMASK(n2)) 
		  : domh -> compareNodesExt(SXP_UNMASK(n1), SXP_UNMASK(n2), udata); }


    virtual SXP_Document retrieveDocument(const SXP_char* uri, const SXP_char* baseUri)
	  { return domh -> retrieveDocument
	          ? SXP_MASK( domh -> retrieveDocument(uri, udata) )
	          : SXP_MASK( domh -> retrieveDocumentExt(uri, baseUri, udata) ); }

    virtual SXP_Node     getNodeWithID(SXP_Document doc, const SXP_char* id)
	  { return domh -> getNodeWithID 
		  ? SXP_MASK( domh -> getNodeWithID(SXP_UNMASK(doc), id) )
		  : SXP_MASK( domh -> getNodeWithIDExt(SXP_UNMASK(doc), id, udata) ); }
    //extras
    virtual void freeName(NodeHandle n, char *buff)
      { if (options & SXPF_DISPOSE_NAMES) 
		domh -> freeBuffer 
		  ? domh -> freeBuffer((SXP_char*)buff) 
		  : domh -> freeBufferExt(SXP_UNMASK(n), (SXP_char*)buff, udata); }

    virtual void freeValue(NodeHandle n, char *buff)
      { if (options & SXPF_DISPOSE_VALUES) 
		domh -> freeBuffer 
		  ? domh -> freeBuffer((SXP_char*)buff) 
		  : domh -> freeBufferExt(SXP_UNMASK(n), (SXP_char*)buff, udata); }

    //functions for processor
    virtual eFlag startCopy(Sit S, NodeHandle n, OutputterObj &outputter);
    virtual eFlag endCopy(Sit S, NodeHandle n, OutputterObj &outputter);
    virtual eFlag copyNode(Sit S, NodeHandle n, OutputterObj &outputter);
    //others
    void setSXPMask(int mask) { sxpMask = mask; }
    void setOptions(unsigned long opt) { options = opt; }
private:
    DOMHandler *domh;
    void* udata;
    int sxpMask;
    unsigned long options;
    void getNodeEName(NodeHandle n, EQName &ename);
};

#define SXP_REDIRECT_NH(name,nh) ((NHC(nh) & 1) ? NZ(external)->get##name(nh) : standard->get##name(nh))
#define SXP_REDIRECT_NH1(name,nh,arg1) ((NHC(nh) & 1) ? NZ(external)->get##name(nh,arg1) : standard->get##name(nh,arg1))

class DOMProviderUniversal : public DOMProvider
{
 public:
  DOMProviderUniversal();
  virtual ~DOMProviderUniversal();
  void setExtProvider(DOMHandler *domh, void* data);
  void setOptions(unsigned long options_)
    { options = options_; if (external) external->setOptions(options); }
  unsigned long getOptions() { return options; }
  //virtuals
  virtual SXP_NodeType getNodeType(SXP_Node n)
    { return SXP_REDIRECT_NH(NodeType, n); }

  virtual const SXP_char* getNodeName(SXP_Node n)
    { return SXP_REDIRECT_NH(NodeName, n); }

  virtual const SXP_char* getNodeNameURI(SXP_Node n)
    { return SXP_REDIRECT_NH(NodeNameURI, n); }

  virtual const SXP_char* getNodeNameLocal(SXP_Node n)
    { return SXP_REDIRECT_NH(NodeNameLocal, n); }

  virtual const SXP_char* getNodeValue(SXP_Node n)
    { return SXP_REDIRECT_NH(NodeValue, n); }

  virtual SXP_Node     getNextSibling(SXP_Node n)
    { return SXP_REDIRECT_NH(NextSibling, n); }

  virtual SXP_Node     getPreviousSibling(SXP_Node n)
    { return SXP_REDIRECT_NH(PreviousSibling, n); }

  virtual SXP_Node     getNextAttrNS(SXP_Node n)
    { return SXP_REDIRECT_NH(NextAttrNS, n); }

  virtual SXP_Node     getPreviousAttrNS(SXP_Node n)
    { return SXP_REDIRECT_NH(PreviousAttrNS, n); }

  virtual int          getChildCount(SXP_Node n)
    { return SXP_REDIRECT_NH(ChildCount, n); }

  virtual int          getAttributeCount(SXP_Node n)
    { return SXP_REDIRECT_NH(AttributeCount, n); }

  virtual int          getNamespaceCount(SXP_Node n)
    { return SXP_REDIRECT_NH(NamespaceCount, n); }

  virtual SXP_Node     getChildNo(SXP_Node n, int ndx)
    { return SXP_REDIRECT_NH1(ChildNo, n, ndx); }

  virtual SXP_Node     getAttributeNo(SXP_Node n, int ndx)
    { return SXP_REDIRECT_NH1(AttributeNo, n, ndx); }

  virtual SXP_Node     getNamespaceNo(SXP_Node n, int ndx)
    { return SXP_REDIRECT_NH1(NamespaceNo, n, ndx); }

  virtual SXP_Node     getParent(SXP_Node n)
    { return SXP_REDIRECT_NH(Parent, n); }

  virtual SXP_Document getOwnerDocument(SXP_Node n)
    { return SXP_REDIRECT_NH(OwnerDocument, n); }

  virtual int compareNodes(SXP_Node n1, SXP_Node n2);

  virtual SXP_Document retrieveDocument(const SXP_char* uri, const SXP_char* baseUri);

  virtual SXP_Node     getNodeWithID(SXP_Document doc, 
				     const SXP_char* id)
    { return SXP_REDIRECT_NH1(NodeWithID, doc, id); }
  //extras
  virtual void freeName(NodeHandle n, char *buff);
  virtual void freeValue(NodeHandle n, char *buff);
  //function for processor
  virtual eFlag startCopy(Sit S, NodeHandle n, OutputterObj &outputter);
  virtual eFlag endCopy(Sit S, NodeHandle n, OutputterObj &outputter);
  virtual eFlag copyNode(Sit S, NodeHandle n, OutputterObj &outputter);
  //others
  void setMaskBit(int mask);
  int getMaskBit();
 private:
  unsigned long options;
  int maskBit;
  DOMProviderExternal *external;
  DOMProviderStandard *standard;
};

class Context;
class Number;
class Str;
class Element;
class Tree;
class Processor;

class QueryContextClass
{
public:
    QueryContextClass(Sit S);
    ~QueryContextClass();
    eFlag query(const SXP_char* queryText,
		SXP_Node n,
		int contextPosition,
		int contextSize);
    eFlag addVariableExpr(const SXP_char* name, Expression *value);	    
    eFlag addVariableBinding(const SXP_char* name, QueryContextClass &source);
    eFlag addNamespaceDeclaration(const SXP_char* prefix, 
				  const SXP_char* uri);
    SXP_ExpressionType getType();
    const Number* getNumber();
    const Str* getString();
    Bool getBoolean();
    const Context* getNodeset();	
    Expression* getNewExpr();
    int getError() {return NZ(theSituation) -> getError();} 
    Situation &getSituation() { return *theSituation; };
private:
    void cleanupAfterQuery();
    Tree *baseTree;
    Expression *queryExpression, *resultExpression;
    Str* stringValue;
    Number *numberValue;
    Situation *theSituation;
    Expression* getExpression_() const;
    Processor *proc;
    Bool mustKillProc;
};

#endif // DomProviderHIncl
