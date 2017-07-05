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

#include "tree.h"
#include "proc.h"
#include "guard.h"
#include "key.h"

/* ------------------------------------------------------------ */
/* SpaceNameList */
/* -------------------------------------------------------------*/

Bool SpaceNameList::findName(EQName &ename, double &prio)
{
  int num = number();
  prio = -10;
  Bool found = 0;
  for (int i = 0; i < num; i++)
    {
      EQName *aux = operator[](i);
      if (aux -> getLocal() == (const char*) "*")
	{
	  if (aux -> getUri() == (const char*) "")
	    {
	      found = 1;
	      prio = -0.5;
	    }
	  else
	    if (ename.getUri() == aux -> getUri())
	      {
		found = 1;
		prio = -0.25;
	      }
	}
      else
	{
	  if (ename.getLocal() == aux -> getLocal() && 
	      ename.getUri() == aux -> getUri())
	    {
	      found = 1;
	      prio = 0; 
	      break; //exit loop, we can't get better match
	    }
	}
    }
  return found;
}

/* ------------------------------------------------------------ */
/* StylesheetStructure */
/* -------------------------------------------------------------*/

//
//
//  SubtreeInfo
//
//

SubtreeInfo::~SubtreeInfo()
{
  excludedNS.deppendall();
  extensionNS.deppendall();
  excludedCount.deppendall();
  extensionCount.deppendall();
}

void SubtreeInfo::pushNamespaceMarks()
{
  excludedCount.append(excludedNS.number());
  extensionCount.append(extensionNS.number());
}

void SubtreeInfo::popNamespaceMarks()
{
  //excluded
  int limit = excludedCount.number() ? excludedCount.last() : 0;
  int idx;
  for (idx = excludedNS.number() - 1; idx >= limit; idx--)
    excludedNS.deppend();
  if (excludedCount.number()) excludedCount.deppend();
  //extensions
  limit = extensionCount.number() ? extensionCount.last() : 0;
  for (idx = extensionNS.number() - 1; idx >= limit; idx--)
    extensionNS.deppend();
  if (extensionCount.number()) extensionCount.deppend();
}

//
//
//  StylesheetSructure
//
//

eFlag StylesheetStructure::findBestRule(
    Sit S, 
    XSLElement *&ret, 
    Context *c,
    QName *currMode,
    Bool importsOnly)
{
    int i;
    ret = NULL;
    // if we don't process imports only, look at the rules in this
    // structure
    if (!importsOnly)
    {
        int rulesNumber = rulesList.number();
	int bestRule = -1;
	double bestPrio = 0;
	Expression *pattern = NULL;
	QName *thisMode;
	XSLElement *therule;
	for (i = 0; i < rulesNumber; i++)
	{
	    if (
		(bestRule != -1) &&
		(fcomp(rulesList[i] -> priority, bestPrio) == -1)  
		// current < best
		)
		break;
	    therule = rulesList[i] -> rule;
	    Attribute *a = rulesList[i] -> match;
	    if (a) 
		pattern = a -> expr;
	    else 
		continue;

	    thisMode = rulesList[i] -> mode;
	    if ((!thisMode) ^ (!currMode))
		continue;
	    if (thisMode && !((*thisMode) == (*currMode)))
		continue;
	    // else both thisMode and currMode are NULL which is OK
	    if (pattern)
	    {
		Bool result;
		E( pattern -> matchesPattern(S, c, result) );
		if (result)
		{
		    bestRule = i;
		    bestPrio = rulesList[i] -> priority;
		}
	    };
	};
	if (bestRule != -1) 
	    ret = rulesList[bestRule] -> rule; // else remains NULL
    }
    
    // if no match was found, look at the imported stylesheets
    if (!ret)
    {
	int importCount = importChildren.number();
	for (i = 0; i < importCount && !ret; i++)
	{
	    // call findBestRule recursively, setting importsOnly to FALSE
	    E( importChildren[i] -> findBestRule(
		S, ret, c, currMode, FALSE));
	}
    }
    return OK;
};

XSLElement* StylesheetStructure::findRuleByName(Tree &t, QName &q)
{
    XSLElement* ret = rulesList.findByName(t,q);
    if (! ret)
      {
	int importCount = importChildren.number(),
	  i;
	for (i = 0; !ret && i < importCount; i++)
	  ret = importChildren[i] -> findRuleByName(t,q);
	  //ret = importChildren[i] -> rulesList.findByName(t,q);
      }
    return ret;
}

Bool StylesheetStructure::hasAnyStripped()
{
  Bool ret = strippedNamesList.number();
  if (!ret)
    {
      int importCount = importChildren.number();
      for (int i = 0; i < importCount && !ret; i++)
	{
	  ret = importChildren[i] -> hasAnyStripped();
	}
    }
  return ret;
}

Bool StylesheetStructure::hasAnyPreserved()
{
  Bool ret = preservedNamesList.number();
  if (!ret)
    {
      int importCount = importChildren.number();
      for (int i = 0; i < importCount && !ret; i++)
	{
	  ret = importChildren[i] -> hasAnyPreserved();
	}
    }
  return ret;
}

Bool StylesheetStructure::findStrippedName(EQName &ename, int &prec, double &pri)
{
  Bool ret = FALSE;
  if (strippedNamesList.findName(ename, pri)) {
    prec = importPrecedence;
    ret = TRUE;
  }
  else
    {
      int importCount = importChildren.number();
      for (int i = 0; i < importCount && !ret; i++)
	{
	  // call findBestRule recursively, setting importsOnly to FALSE
	  ret = importChildren[i] -> findStrippedName(ename, prec, pri);
	}
    }
  return ret;
}

Bool StylesheetStructure::findPreservedName(EQName &ename, int &prec, double &pri)
{
  Bool ret = FALSE;
  if (preservedNamesList.findName(ename, pri)) {
    prec = importPrecedence;
    ret = TRUE;
  }
  else
    {
      int importCount = importChildren.number();
      for (int i = 0; i < importCount && !ret; i++)
	{
	  // call findBestRule recursively, setting importsOnly to FALSE
	  ret = importChildren[i] -> findPreservedName(ename, prec, pri);
	}
    }
  return ret;
}


//
//
//  SubtreeList
//
//


//
//
//  VarDirectory
//
//

VarDirectory::VarDirectory()
{
}

VarDirectory::~VarDirectory()
{
    freeall(FALSE);
}

XSLElement* VarDirectory::find(QName& name)
{
    int i = findNdx(name);
    return (i == -1)? NULL : (*this)[i] -> getElement();
}

eFlag VarDirectory::insert(Sit S, QName &name, XSLElement* var)
{
    int i = findNdx(name);
    if (i == -1)
	append(new VarDirectoryItem(name, var));
    else
    {
	// if precedences are equal, raise error
	int 
	    oldPrec = (*this)[i] -> getElement() -> getImportPrecedence(),
	    newPrec = var -> getImportPrecedence();
	// due to immediate resolution of xsl:import, new must be <= old
	sabassert(newPrec <= oldPrec);
	(*this)[i] -> setElement(var);
	if (newPrec == oldPrec)
	{
	    Str fullName;
	    var -> getOwner().expandQStr(name, fullName);
	    Err1(S, E1_MULT_ASSIGNMENT, fullName);
	}
    }
    return OK;
}


int VarDirectory::findNdx(const QName& name)
{
    int i;
    for (i = 0; i < number(); i++)
    {
	if ( (*this)[i] -> getName() == name )
	    return i;
    }
    return -1;
}

/*****************************************************************

  Attribute Sets

*****************************************************************/

//
//
//  AttSetMember implementation
//
//

void AttSetMember::set(XSLElement *newAttDef)
{
    int oldPrec = attDef ? 
	attDef -> getImportPrecedence() : -1;
    int newPrec = newAttDef -> getImportPrecedence();
    if (oldPrec == newPrec && !redefinition)
	redefinition = newAttDef;
    if (newPrec <= oldPrec || oldPrec == -1)
	attDef = newAttDef;
    if (newPrec < oldPrec)
	redefinition = NULL;
}

//
//
//  AttSet
//
//

AttSet::AttSet(QName &name_)
: name(name_)
{
};

AttSet::~AttSet()
{
    freeall(FALSE);
    //don't free this list items, these are copies of 
    //pointers stored with xsl:attribute-set element
    //usedSets.freeall(FALSE); 
    usedSets.deppendall(); 
}

eFlag AttSet::checkRedefinitions(Sit S)
{
    for (int i = 0; i < number(); i++)
	if ((*this)[i] -> getRedefinition())
	{
	    XSLElement *redef = (*this)[i] -> getRedefinition(); 
	    Str fullNameAtt, fullNameThis;
	    redef -> getOwner().expandQStr((*this)[i] -> getAttName(), fullNameAtt);
	    redef -> getOwner().expandQStr(getName(), fullNameThis);
	    S.setCurrVDoc(redef);
	    Warn2(S, W2_ATTSET_REDEF, fullNameAtt, fullNameThis);
	}
    return OK;
}

eFlag AttSet::execute(Sit S, Context *c, Tree& sheet, QNameList& history, Bool resolvingGlobals)
{
  if (history.findNdx(name) != -1)
    {
      Str fullName;
      sheet.expandQStr(name, fullName);
      Err1(S, E1_CIRCULAR_ASET_REF, fullName);
    }
  history.append(&name);
  int i;
  for (i = 0; i < usedSets.number(); i++)
    E( sheet.attSets().executeAttSet(S, *(usedSets[i]), c, 
				     sheet, history, resolvingGlobals) );
  history.deppend();

  for (i = 0; i < number(); i++)
    {
      XSLElement *def = (*this)[i] -> getAttributeDef();
      //execute the content
      E( def -> execute(S, c, resolvingGlobals) );
    }
  return OK;
}

int AttSet::findNdx(QName &attName)
{
    for (int i = 0; i < number(); i++)
	if ((*this)[i] -> getAttName() == attName)
	    return i;
    return -1;
}

void AttSet::insertAttributeDef(XSLElement *attDef, QName &attName)
{
    int ndx = findNdx(name);
    if (ndx == -1)
    {
	append(new AttSetMember(attName));
	ndx = number() - 1;
    }
    (*this)[ndx] -> set(attDef);
}

void AttSet::insertUses(QName &usedSet)
{ 
    if (usedSets.findNdx(usedSet) == -1) 
	usedSets.append(&usedSet); 
}


//
//
//  AttSetList
//
//

AttSetList::AttSetList()
:
PList<AttSet*>(LIST_SIZE_LARGE)
{};

AttSetList::~AttSetList()
{
    freeall(FALSE);
}

eFlag AttSetList::checkRedefinitions(Sit S)
{
    for (int i = 0; i < number(); i++)
	E( (*this)[i] -> checkRedefinitions(S) );
    return OK;
}

AttSet* AttSetList::insert(QName &name)
{
    int ndx = findNdx(name);
    AttSet *ptr = NULL;
    if (ndx == -1)
	append(ptr = new AttSet(name));
    else
	ptr = (*this)[ndx];
    return ptr;
}

eFlag AttSetList::executeAttSet(Sit S, QName &name, Context *c, Tree &sheet, QNameList& history, Bool resolvingGlobals)
{
    int ndx = findNdx(name);
    if (ndx == -1)
    {
	Str fullName;
	sheet.expandQStr(name, fullName);
	Err1(S, E1_NONEX_ASET_NAME, fullName);
    }
    E( (*this)[ndx] -> execute(S, c, sheet, history, resolvingGlobals) );
    return OK;
}

int AttSetList::findNdx(const QName &what) const
{
    int count = number();
    for (int i = 0; i < count; i++)
        if ((*this)[i] -> getName() == what)
	    return i;
    return -1;
}

//
//
//  AliasItem implementation
//
//

void AliasItem::set(Phrase newKey, Phrase newValue, Phrase newPrefix,
		    int newPrecedence, XSLElement *source)
{
    sabassert(newPrecedence >= 0);
    if (key == UNDEF_PHRASE) key = newKey;
    if (precedence == newPrecedence && value != newValue && !redefinition)
	redefinition = source;
    if (newPrecedence <= precedence || precedence == -1)
    {
	value = newValue;
	precedence = newPrecedence;
	prefix = newPrefix;
    }
    if (newPrecedence < precedence)
	redefinition = NULL;
}

//
//
//  AliasList implementation
//
//

int AliasList::findNdx(Phrase key_) const
{
    int i, count = number();
    for (i = 0; (i < count) && !(key_ == ((*this)[i] -> getKey())); i++);
    return (i < count) ? i : -1;
}

Phrase AliasList::find(Phrase key_) const
{
    int ndx = findNdx(key_);
    return (ndx != -1) ? (*this)[ndx] -> getValue() : PHRASE_NOT_FOUND;
}

void AliasList::insertAlias(Phrase key, Phrase value, Phrase prefix,
			    int precedence, XSLElement *source)
{
    AliasItem *newItem;
    int ndx = findNdx(key);
    if (ndx == -1)
	append(newItem = new AliasItem);
    else
	newItem = (*this)[ndx];
    newItem -> set(key, value, prefix, precedence, source);
}

eFlag AliasList::checkRedefinitions(Sit S, Tree &sheet)
{
    for (int i = 0; i < number(); i++)
	if ((*this)[i] -> getRedefinition())
	{
	    S.setCurrVDoc((*this)[i] -> getRedefinition());
	    Str fullName;
	    sheet.expand((*this)[i] -> getKey());
	    Warn1(S, W1_ALIAS_REDEF, fullName);
	}
    return OK;
}

/****************************************
T r e e   methods
****************************************/

Tree::Tree(const Str &aname, BOOL aXSLTree)
    : theArena(TREE_ARENA_SIZE), theDictionary(&theArena, TREE_DICT_LOGSIZE),
      structure(0)
{
    // theDictionary.initialize();
    QName &rootname = (QName&) getTheEmptyQName();
    root = new(&theArena) RootNode(*this, rootname);
    XSLTree = aXSLTree;
    stackTop = &getRoot();
    pendingTextNode = NULL;
    getRoot().stamp = 0;
    vcount = 1;
    QName dummyName;
    theDummyElement = new(&theArena) Element(*this, dummyName);
    initDict();

    // append entry for 'the whole tree' to subtrees
    subtrees.push(new SubtreeInfo(
	aname, XSL_NONE, &structure, FALSE));
    getRoot().setSubtreeInfo(subtrees.last());

    excludeStdNamespaces();

    pendingNSList.append(new(&theArena) NSList());

    stripped = aXSLTree;
    hasAnyStripped = hasAnyPreserved = -1;// means 'who knows?'
    importUnique = 0xFFFF;
};

Tree::~Tree()
{
    getRoot().~RootNode();
    // this is just in case there's a processing error:
    //pendingNSList.freeall(FALSE);
    delete theDummyElement;
    // clear subtree information
    subtrees.freeall(FALSE);
    aliasesList.freeall(FALSE);
    unparsedEntities.freeall(FALSE);
    
    pendingNSList.freelast(FALSE);
    //sabassert(!pendingNSList.number());
	//tmpList must be empty before the arena gets down automatically
	tmpList.freeall(FALSE);
}

void Tree::initDict()
{
    theDictionary.initialize();
    theDictionary.insert("", stdPhrases[PHRASE_EMPTY]);
    theDictionary.insert("xsl", stdPhrases[PHRASE_XSL]);
    theDictionary.insert(theXSLTNamespace, stdPhrases[PHRASE_XSL_NAMESPACE]);
    theDictionary.insert(theXMLNamespace, stdPhrases[PHRASE_XML_NAMESPACE]);
    theDictionary.insert(theSabExtNamespace, stdPhrases[PHRASE_SABEXT_NAMESPACE]);
    theDictionary.insert("*", stdPhrases[PHRASE_STAR]);
    theDictionary.insert("xmlns", stdPhrases[PHRASE_XMLNS]);
    theDictionary.insert("lang", stdPhrases[PHRASE_LANG]);
}

void Tree::excludeStdNamespaces()
{
  NZ(getCurrentInfo()) -> 
    getExcludedNS().addUri(stdPhrase(PHRASE_XML_NAMESPACE));
  if (XSLTree)
    {
      NZ(getCurrentInfo()) -> 
	getExcludedNS().addUri(stdPhrase(PHRASE_XSL_NAMESPACE));
    }
}

Element &Tree::dummyElement() const
{
    return *theDummyElement;
}

void Tree::flushPendingText()
{
    if (pendingTextNode)
        pendingTextNode -> cont = pendingText;
    pendingText.empty();
    pendingTextNode = NULL;
}
    
eFlag Tree::appendVertex(Sit S, Vertex *v)
{
    sabassert(stackTop && isDaddy(stackTop));
    sabassert(!isText(v) || !pendingTextNode);
    if (!isText(v))
        flushPendingText();
    E( cast(Daddy*,stackTop) -> newChild(S, v) ); //sets parent too
    if (isDaddy(v))
        stackTop = v;
    v -> stamp = vcount++;
    // set the subtree information for vertex
    v -> setSubtreeInfo(subtrees.getCurrent());
    return OK;
};

void Tree::dropCurrentElement(Vertex *v)
{
    sabassert(stackTop && isElement(stackTop));
    sabassert(stackTop == v);
    sabassert(!pendingTextNode);
    stackTop = v -> parent;
    delete v;
    toE(stackTop) -> contents.deppend();
}

Vertex* Tree::appendText(Sit S, char *string, int len)
{
    Vertex *txt = NULL;
    if (!pendingTextNode)
    {
        // the initializing text does not matter
        txt = new(&getArena()) Text(*this, string, len);
	Processor *proc = S.getProcessor();
	if ( proc && proc->outputter() ) 
	  {
	    OutputDocument *doc=proc->outputter()->getDocumentForLevel(FALSE);
	    txt -> setOutputDocument(doc);
	  }

        appendVertex(S, txt);
        pendingTextNode = toText(txt);
    }
    pendingText.nadd(string,len);
    return txt;
}

Vertex* Tree::popVertex()
{
    Vertex *v = NZ( stackTop );
    stackTop = v -> parent;
    return v;
}

eFlag Tree::parseFinished(Sit S)
{
    flushPendingText();
    return OK;
}

HashTable& Tree::dict() 
{
    return theDictionary;
}

SabArena& Tree::getArena()
{
    return theArena;
}

const QName& Tree::getTheEmptyQName() const
{
    return theEmptyQName;
}


Bool Tree::cmpQNames(const QName &first, const QName &second) const
{
/*
    printf("comparing names (%s,%s,%s) and (%s,%s,%s)\n",
        (char*)(((Tree*)this)->expand(first.getPrefix())),
	    (char*)(((Tree*)this)->expand(first.getUri())),
	    (char*)(((Tree*)this)->expand(first.getLocal())),
        (char*)(((Tree*)this)->expand(second.getPrefix())),
	    (char*)(((Tree*)this)->expand(second.getUri())),
	    (char*)(((Tree*)this)->expand(second.getLocal()))
	);
*/	
    if (first.getLocal() == stdPhrase(PHRASE_STAR))
        return (Bool)(first.getPrefix() == UNDEF_PHRASE || 
            first.getUri() == second.getUri());
    else
        return (Bool) (first.getUri() == second.getUri()
	        && first.getLocal() == second.getLocal());
}

Bool Tree::cmpQNamesForeign(const QName &q, const HashTable& dictForeign, const QName &qForeign)
{
/*
    printf("comparing names (%s,%s,%s) and (%s,%s,%s)\n",
        (char*)(((Tree*)this)->expand(q.getPrefix())),
	    (char*)(((Tree*)this)->expand(q.getUri())),
	    (char*)(((Tree*)this)->expand(q.getLocal())),
        (char*)(dictForeign.getKey(qForeign.getPrefix())),
	    (char*)(dictForeign.getKey(qForeign.getUri())),
	    (char*)(dictForeign.getKey(qForeign.getLocal()))
	);
*/	

    if (q.getLocal() == stdPhrase(PHRASE_STAR))
    {
        return (Bool)(q.getPrefix() == UNDEF_PHRASE || 
            (dict().getKey(q.getUri()) == dictForeign.getKey(qForeign.getUri())));
	}
    else
    {
        return (Bool) 
	        (dict().getKey(q.getUri()) == dictForeign.getKey(qForeign.getUri()) &&
            dict().getKey(q.getLocal()) == dictForeign.getKey(qForeign.getLocal()));		
	}
}

Bool Tree::cmpQNameStrings(const QName &q, const Str& uri, const Str& local)
{
    if (q.getLocal() == stdPhrase(PHRASE_STAR))
        return (Bool)(
	    q.getUri() == UNDEF_PHRASE || dict().getKey(q.getUri()) == uri);
    else
    {
        return (Bool) 
  	        (dict().getKey(q.getUri()) == uri &&
            dict().getKey(q.getLocal()) == local);		
	}
}

void Tree::expandQ(const QName& q, EQName& expanded)
{
    expanded.setLocal(expand(q.getLocal()));
    expanded.setUri(expand(q.getUri()));
    expanded.setPrefix(expand(q.getPrefix()));
}

void Tree::expandQStr(const QName& q, Str& expName)
{
    EQName expanded;
    expandQ(q, expanded);
    expanded.getname(expName);
}

const Str& Tree::expand(Phrase ph)
{
    return dict().getKey(ph);
}

Phrase Tree::unexpand(const Str& strg)
{
    Phrase result;
    dict().insert(strg, result);
    return result;
}

eFlag Tree::serialize(Sit S, char *& result)
{
    OutputterObj out;
    OutputDefinition def;
    GP( DataLine ) targetLine = new DataLine;
    // set default options for output
    EQName xmlMethod;
    xmlMethod.setLocal((char*)"xml");
    E( def.setItemEQName(S, XSLA_METHOD, xmlMethod, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
    E( def.setDefaults(S) ); 
    E( (*targetLine).open(S, (const char*)"arg:/dummy_", DLMODE_WRITE, NULL) );
    out.setOptions(S, targetLine, &def);
    E( getRoot().serialize(S, out) );
    result = (*targetLine).getOutBuffer() -> compactToBuffer();
    E( (*targetLine).close(S) );
    targetLine.del();
    return OK;
}

eFlag Tree::serializeNode(Sit S, Element *v, char *& result)
{
    OutputterObj out;
    OutputDefinition def;
    GP( DataLine ) targetLine = new DataLine;
    // set default options for output
    EQName xmlMethod;
    xmlMethod.setLocal((char*)"xml");
    E( def.setItemEQName(S, XSLA_METHOD, xmlMethod, NULL, OUTPUT_PRECEDENCE_WEAKEST) );
    E( def.setDefaults(S) ); 
    E( (*targetLine).open(S, (const char*)"arg:/dummy_", DLMODE_WRITE, NULL) );
    out.setOptions(S, targetLine, &def);
    E( (*v).serializeSubtree(S, out) );
    result = (*targetLine).getOutBuffer() -> compactToBuffer();
    E( (*targetLine).close(S) );
    targetLine.del();
    return OK;
}

void Tree::makeStamps()
{
    int stamp_ = 0;
    getRoot().makeStamps(stamp_);
    vcount = stamp_;
}

void Tree::updateImportStatus() 
{
  if (! subtrees.getCurrent() -> getStructure() -> getTopLevelFound() )
    {
      StylesheetStructure * stru = subtrees.getCurrent() -> getStructure();
      stru -> setTopLevelFound(true);
      stru -> setImportPrecedence(importUnique);
      importUnique--;
    }
}

/*****************************************************************
Tree::processVertexAfterParse()
Performs any necessary actions on a vertex immediately after it is
parsed into the tree. The vertex is still 'current'. Typically, this
function calls popVertex.
ARGS:
    v   the vertex to be processed
The function operates only on XSL vertices, namely:
xsl:include - replaces the vertex by the newly constructed tree
xsl:output  - files the information into the tree's structures
...
Other vertices are just popped off the stack.
*****************************************************************/

eFlag Tree::processVertexAfterParse(Sit S, Vertex *v, TreeConstructer* tc)
{
  //be careful with this test, it might be moved deeper inside this 
  //function if needed

  if (v -> vt & VT_TOP_FOREIGN) 
    {
      popVertex();
      return OK;
    }

  XSL_OP theOp;
  if (isXSLElement(v))
    {
      XSLElement *x = toX(v);
      theOp = x -> op;

      if (theOp != XSL_IMPORT) updateImportStatus();

      switch(theOp)
        {
	  //catch xsl:use-attribute-sets
	case XSL_ELEMENT:
	case XSL_COPY: 
	  {
	    E( extractUsedSets(S, toE(v)) );
	    popVertex();
	  }; break;
	case XSL_IMPORT:
	  {
	    if (subtrees.getCurrent() -> getStructure() -> getTopLevelFound())
	      {
		Err2(S, E_ELEM_CONTAINS_ELEM, 
		     xslOpNames[XSL_STYLESHEET], 
		     xslOpNames[XSL_IMPORT]);
	      }
	  }; // no break
	case XSL_INCLUDE:
	  {
	    Attribute *a = NZ( x -> atts.find(XSLA_HREF) );
	    GP( Tree ) srcTree; 
	    const Str& base = S.findBaseURI(a -> getSubtreeInfo() ->
					    getBaseURI());		
	    
	    Str absolute;
	    makeAbsoluteURI(S, a -> cont, base, absolute);
	    if (S.getProcessor())
	      {
		E( S.getProcessor() -> readTreeFromURI(S, srcTree, 
						       a -> cont, base, 
						       FALSE) );
		srcTree.keep();
	      }
	    else
	      {
		//Str absolute;
		//makeAbsoluteURI(a -> cont, base, absolute);
		srcTree = new Tree(absolute, FALSE);
		DataLine d;
		E( d.open(S, absolute, DLMODE_READ, /* argList = */ NULL) );
		E( (*srcTree).parse(S, &d) );
		E( d.close(S) );
	      }
	    
	    Element *theSheet=(*srcTree).findStylesheet((*srcTree).getRoot());
	    if (!theSheet)
		Warn1(S, W_NO_STYLESHEET, (char*)(a -> cont));
	    dropCurrentElement(v);
	    if (!theSheet) // to prevent segfault after include/import failure
		break;
	    
	    OutputterObj source;
	    //we start a subtree to record where the nodes come from
	    //when including, we use the old structure
	    //when importing, Tree creates a new one
	    E( startSubtree(S, (*srcTree).getURI(), theOp) );
	    //set extension namespaces for subtree

	    //(*srcTree).speakDebug();

	    //merge it into the current tree
	    E( tc -> parseUsingSAXForAWhile(S, source, absolute, 
					    TRUE, 
					    (Tree*)srcTree,
					    theSheet -> namespaces) );

	    //first we have to deal with ext. and excl. namespaces
	    Attribute *attr;
	    QName q;
	    //exclusions
	    q.setLocal((*srcTree).unexpand("exclude-result-prefixes"));
	    attr = theSheet->atts.find(q);
	    if (attr)
	      E(pushNamespacePrefixes(S, attr->cont, XSLA_EXCL_RES_PREFIXES));
	    //extensions
	    q.setLocal((*srcTree).unexpand("extension-element-prefixes"));
	    attr = theSheet->atts.find(q);
	    if (attr)
	      E(pushNamespacePrefixes(S, attr->cont, XSLA_EXT_ELEM_PREFIXES));

	    if (theSheet)
	      E( theSheet -> contents.copy(S, source) );
	    E( tc -> parseUsingSAXForAWhileDone(S, source, TRUE) );
	    // end the subtree
	    E( endSubtree(S, theOp) );		
	  }; break;
	case XSL_OUTPUT:
	  {
	    int i, attsNumber = x -> atts.number();
	    Attribute *theAtt;
	    for (i = 0; i < attsNumber; i++)
	      {
		theAtt = toA(x -> atts[i]);
		switch(theAtt -> op)
		  {
		  case XSLA_METHOD:
		    {
		      QName q;
		      EQName eq;
		      E( x -> setLogical(S, 
					 q, theAtt -> cont, FALSE) );
		      expandQ(q, eq);
		      E( outputDef.setItemEQName(S, XSLA_METHOD, 
						 eq, v, 
						 v -> getImportPrecedence()) );
		    }; break;
		  case XSLA_CDATA_SECT_ELEMS:
		    {
		      QName q;
		      Bool someRemains;
		      Str listPart;
		      char *p = theAtt -> cont;
		      do
			{
			  someRemains = getWhDelimString(p, listPart);
			  if (someRemains)
			    {
			      E( x -> setLogical(S, 
						 q, listPart, TRUE) );
			      EQName expanded;
			      expandQ(q, expanded);
			      E( outputDef.setItemEQName(S, 
							 XSLA_CDATA_SECT_ELEMS,
							 expanded, v, 
							 v -> getImportPrecedence()) );
			    };
			}
		      while (someRemains);
		    }; break;
		  case XSLA_NONE: //skip other namespaces
		    break;
		  default:
		    {
		      E( outputDef.setItemStr(S, theAtt -> op, theAtt -> cont, 
					      theAtt, 
					      theAtt -> getImportPrecedence()) );
		      
		    };
		  };
	      }
	    popVertex();
	  }; break;
	case XSL_NAMESPACE_ALIAS:
	  {
	    Phrase style, result, sUri, rUri;
	    Attribute *sp = NZ( x -> atts.find(XSLA_STYLESHEET_PREFIX) );
	    Attribute *rp = NZ( x -> atts.find(XSLA_RESULT_PREFIX) );
	    if (sp -> cont == "#default") style = UNDEF_PHRASE;
	    else dict().insert(sp -> cont, style);
	    if (rp -> cont == "#default") result = UNDEF_PHRASE;
	    else dict().insert(rp -> cont, result);

	    int i;
	    i = pendingNS().findNdx(style);
	    if (i != -1)
	      sUri = toNS(pendingNS().operator[](i)) -> uri;
	    else
	      Err1(S, E_EX_NAMESPACE_UNKNOWN, (char*) sp -> cont);

	    i = pendingNS().findNdx(result);
	    if (i != -1)
	      rUri = toNS(pendingNS().operator[](i)) -> uri;
	    else
	      Err1(S, E_EX_NAMESPACE_UNKNOWN, (char*) rp -> cont);

	    aliases().insertAlias(sUri, rUri, result, 
				  v -> getImportPrecedence(), x);
	    popVertex();
	  }; break;
	case XSL_TEMPLATE:
	  {
	    E( insertRule(S, x) );
	    popVertex();
	  }; break;
	case XSL_ATTRIBUTE_SET:
	  {
	    QName name;
	    
	    E( x -> setLogical(S, name,
			       NZ( x -> atts.find(XSLA_NAME)) -> cont, 
			       FALSE) );
	    AttSet *ptr = attSets().insert(name);
	    E( extractUsedSets(S, toE(v)) );
	    if (x -> attSetNames(FALSE))
	      {
		for (int i = 0; i < x -> attSetNames(FALSE) -> number(); i++)
		 ptr -> insertUses(*(x -> attSetNames(FALSE) -> operator[] (i)));
	      }
	    XSLElement *son;
	    for (int i = 0; i < x -> contents.number(); i++)
	      {
		sabassert(isXSLElement(x -> contents[i]) && 
		       toX(x -> contents[i]) -> op == XSL_ATTRIBUTE);
		son = toX(x -> contents[i]);
		E( son -> setLogical(S, name, 
				     NZ( son -> atts.find(XSLA_NAME)) -> cont,
				     FALSE) );
		ptr -> insertAttributeDef(son, name);
	      }
	    popVertex();
	  }; break;
	case XSL_STYLESHEET:
	case XSL_TRANSFORM:
	  {
	    popVertex();
	  }; break;
	case XSL_VARIABLE:
	case XSL_PARAM:
	  {
	    // only look at top-levels
	    Vertex *par = v -> parent;
	    if (par && isXSLElement(par) && 
		(toX(par) -> op == XSL_STYLESHEET || 
		 toX(par) -> op == XSL_TRANSFORM))
	      {
		// is top-level -> insert into directory, 
		//with error if there already is an entry 
		// with the same import precedence
		// find name first
		QName name;
		E( x -> setLogical(S, name, 
				   NZ( x -> atts.find(XSLA_NAME)) -> cont, 
				   FALSE) );
		E( toplevelVars.insert(S, name, x) );
	      }
	    popVertex();
	  }; break;
	case XSL_STRIP_SPACE:
	  {
	    SpaceNameList &foo = 
	      subtrees.getCurrent() -> getStructure() -> strippedNames();
	    E( getSpaceNames(S, *x, NZ(x -> atts.find(XSLA_ELEMENTS)) -> cont, 
			     foo) );
	    popVertex();
	  }; break;
	case XSL_PRESERVE_SPACE:
	  {
	    SpaceNameList &foo = 
	      subtrees.getCurrent() -> getStructure() -> preservedNames();
	    E( getSpaceNames(S, *x, NZ(x -> atts.find(XSLA_ELEMENTS)) -> cont, 
			     foo) );
	    popVertex();
	  }; break;
	default:
	  popVertex();
	}
	//the literal output element may have some xsl features
    }
  else 
    { //isXSLElement
      updateImportStatus();
      if (XSLTree) {
	E( extractUsedSets(S, toE(v)) );
	popVertex();
      }
      else {
	popVertex();
      }
    }
  return OK;
}

eFlag Tree::getSpaceNames(Sit S, Element &e, Str &str, SpaceNameList &where)
{
  char *p, *q;
  q = (char*)str;
  skipWhite(q);
  p = q;
  int i = strcspn(q, theWhitespace);
  while (*q && i) 
    {
      q += i;
      char save = *q;
      *q = 0;
      
      Str token = p;
      QName name;
      E( e.setLogical(S, name, token, FALSE) );
      GP(EQName) ename = new EQName;
      expandQ(name, *ename);
      where.append(ename.keep());

      *q = save;
      skipWhite(q);
      p = q;
      i = strcspn(q, theWhitespace);
    }

  return OK;
}

eFlag Tree::markNamespacePrefixes(Sit S)
{
  if (XSLTree)
    getCurrentInfo() -> pushNamespaceMarks();
  return OK;
}

eFlag Tree::pushNamespacePrefixes(Sit S, Str& prefixes, XSL_ATT att)
{
  if (!XSLTree) return OK;

  PList<Str*> tokens;
  char *p, *q;
  q = (char*) prefixes;
  skipWhite(q);
  p = q;
  int i = strcspn(q, theWhitespace);
  while (*q && i) 
    {
      q += i;
      char save = *q;
      *q = 0;
      
      Str* aux = new Str(p);
      tokens.append(aux);

      *q = save;
      skipWhite(q);
      p = q;
      i = strcspn(q, theWhitespace);
    }
  //add to list
  SubtreeInfo *info = getCurrentInfo();
  for (i = 0; i < tokens.number(); i++)
    {
      Str tok = *(tokens[i]);
      Phrase prefix = 
	tok == (const char*) "#default" ? UNDEF_PHRASE : unexpand(tok);
      int idx = pendingNS().findNdx(prefix);
      if (idx != -1)
	{
	  switch (att) {
	  case XSLA_EXT_ELEM_PREFIXES:
	    {
	      info -> getExtensionNS().append(toNS(pendingNS()[idx]) -> uri);
	    }; //no break
	  case XSLA_EXCL_RES_PREFIXES:
	    {
	      info -> getExcludedNS().append(toNS(pendingNS()[idx]) -> uri);
	    }; break;
	  }
	}
      else
	{
	  Str aux = *(tokens[i]);
	  tokens.freeall(FALSE);
	  Err1(S, E_EX_NAMESPACE_UNKNOWN, (char*) aux);
	}
    }
  tokens.freeall(FALSE);
  //printf("----------------------------\n");
  return OK;
}

eFlag Tree::popNamespacePrefixes(Sit S)
{
  if (XSLTree)
    getCurrentInfo() -> popNamespaceMarks();
  return OK;
}

eFlag Tree::extractUsedSets(Sit S, Element *e) 
{
    Attribute *a = e -> atts.find(XSLA_USE_ATTR_SETS); 
    if (a) 
    {
	QNameList *names = e -> attSetNames(TRUE);
	names -> freeall(FALSE);
	char *p, *q;

	q = (char*) (a -> cont);
	skipWhite(q);
	p = q;
	int i = strcspn(q, theWhitespace);
	while (*q && i) {
	    q += i;
	    char save = *q;
	    *q = 0;

	    Str token = p;
	    GP( QName ) name = new QName;
	    E( e -> setLogical(S, *name, token, FALSE) );

	    names -> append( name.keep() );
	    *q = save;
	    skipWhite(q);
	    p = q;
	    i = strcspn(q, theWhitespace);
	}

    }
    return OK;
}

/*****************************************************************
findStylesheet()
finds a xsl:stylesheet child of the given daddy. Returns NULL 
if not found.
*****************************************************************/

Element* Tree::findStylesheet(Daddy& d)
{
    Vertex *w;
    int dContentsNumber = d.contents.number();
    for (int i = 0; i < dContentsNumber; i++)
    {
        if (isElement(w = d.contents[i]))
        {
            const QName& wName = toE(w) -> name;
	        Tree& owner = w -> getOwner();
		    Str localStr;
//            if (!strcmp(wName.getUri(), theXSLTNamespace) && /* _PH_ */
            if (wName.getUri() == owner.stdPhrase(PHRASE_XSL_NAMESPACE) &&
               ((localStr = owner.expand(wName.getLocal()) == 
	               xslOpNames[XSL_STYLESHEET]) ||
               (localStr == xslOpNames[XSL_TRANSFORM])))
            return toE(w);
        }
    };
    return NULL;
}

double Tree::defaultPriorityLP(Expression *lpath)
{
    sabassert(lpath && lpath -> functor == EXF_LOCPATH);
    sabassert(lpath -> args.number());
    if ((lpath -> args.number() > 1) || lpath -> args[0] -> step -> preds.number())
        return .5;
    else
    {
        switch (lpath -> args[0] -> step -> ntype)
        {
        case EXNODE_COMMENT:
        case EXNODE_TEXT:
        case EXNODE_NODE:
	  return -0.5;
	  break;
        case EXNODE_PI:
	  return lpath->args[0]->step->piname == "" ? -0.5 : 0;
        case EXNODE_NONE:
	  {
	    QName &qn = lpath -> args[0] -> step -> ntest;
	    if (qn.getLocal() != lpath -> getOwnerTree().stdPhrase(PHRASE_STAR))
	      return 0.0;
	    else
	      {   
		if (qn.getPrefix() == UNDEF_PHRASE)
		  return -0.5;
		else
		  return -0.25;
	      };
	  }; break;
        default:
	  return 0.5;
        };
    };
    return 0;   // BCC thinks we don't return enough
}

double Tree::defaultPriority(XSLElement *tmpl)
{
    Expression *e = tmpl -> getAttExpr(XSLA_MATCH);
    if (!e) 
        return PRIORITY_NOMATCH;
    switch(e -> functor)
    {
    case EXF_LOCPATH:
        {
            return defaultPriorityLP(e);
        }; break;
    case EXFO_UNION:
        {
            double max=0, priority;
            BOOL first = TRUE;
            int eArgsNumber = e -> args.number();
            for (int i=0; i < eArgsNumber; i++)
            {
                priority = defaultPriorityLP(e -> args[i]);
                if (first || (priority > max)) 
                    max = priority;
                first = FALSE;
            };
            return max;
        }; break;
    default:
        {
            sabassert(!"expression not a union or LP");
            return 0;   // dummy
        }
    };
    return 0;       // dummy for BCC
}

eFlag Tree::parse(Sit S, DataLine *d)
{
    Log1(S, L1_PARSING, getURI());
    double time_was = getMillisecs();
    TreeConstructer tc(S);
    eFlag retval = tc.parseDataLineUsingExpat(S, this, d);
    if (!retval)
    {
        Log1(S, L1_PARSE_DONE, getMillisecsDiff(time_was));
    }
    return retval;
}

eFlag Tree::getMatchingList(Sit S, Expression& match, Context& result)
{
    E( getRoot().getMatchingList(S, match, result) );
    return OK;
}

Bool Tree::isExtensionUri(Phrase uri) 
{
  return getCurrentInfo() -> getExtensionNS().findNdx(uri) != -1;
}

eFlag Tree::insertRule(Sit S, XSLElement *tmpl)
{
  double prio;
  Attribute *a = tmpl -> atts.find(XSLA_PRIORITY);
  if (!a)
    prio = defaultPriority(tmpl);
  else
    {
      if (a -> cont.toDouble(prio))
	Err(S, ET_BAD_NUMBER);
    };
  QName q; 
  GP( QName ) mode = NULL;
  
  if (!!(a = tmpl -> atts.find(XSLA_NAME)))
    E( tmpl -> setLogical(S, q, a -> cont, FALSE) );
  
  if (q.getLocal() != UNDEF_PHRASE && 
      subtrees.getCurrent() -> getStructure() -> 
      rules().findByName(*this, q))
    {
      Str fullName;
      expandQStr(q, fullName);
      
      Err1(S, ET_DUPLICATE_RULE_NAME, fullName);
    };
  
  if (!!(a = tmpl -> atts.find(XSLA_MODE)))
    E( tmpl -> setLogical(S, *(mode = new QName), 
			  a -> cont, FALSE) );
  
  subtrees.getCurrent() -> getStructure() -> 
    rules().insert(new RuleItem(tmpl,prio,q,mode.keep()));
  return OK;
}

eFlag Tree::insertAttSet(Sit S, XSLElement *tmpl)
{
    QName q; 
    Attribute *a;
    GP( QName ) sets = NULL;
    if (!!(a = tmpl -> atts.find(XSLA_NAME))) 
	E( tmpl -> setLogical(S, q, a -> cont, FALSE) );
    if (q.getLocal() != UNDEF_PHRASE && 
        attSets().findByName(q))
    {
	Str fullName;
	expandQStr(q, fullName);
	Err1(S, ET_DUPLICATE_ASET_NAME, fullName);
    };    
    attSets().append(new AttSet(q));
    return OK;
}



eFlag Tree::startSubtree(Sit S, const Str& baseURI, XSL_OP dependency,
			 Bool isInline /* = FALSE */)
{
    // look if the URI is on the way to root of the include tree
    if (subtrees.findAmongPredecessors(baseURI))
	Err1(S, E1_CIRCULAR_INCLUSION, baseURI);
    StylesheetStructure *structure;
    if (dependency == XSL_IMPORT)
	structure = createStylesheetStructure(S);
    else
	// find structure of current subtree (always defined since Tree::Tree)
	structure = NZ(subtrees.getCurrent()) -> getStructure();

    subtrees.push(
	new SubtreeInfo(
	    baseURI, 
	    dependency,
	    structure,
	    isInline));

    excludeStdNamespaces();
    //set parent tree (closest non-inline)
    if (isInline)
      for (SubtreeInfo *nfo = subtrees.getCurrent();
	   nfo;
	   nfo = nfo -> getParentSubtree())
	{
	  if (!nfo -> isInline())
	    {
	      subtrees.getCurrent() -> setMasterSubtree(nfo);
	      break;
	    }
	}

    return OK;
}

eFlag Tree::endSubtree(Sit S, XSL_OP dependency)
{
  /*
  subtrees.getCurrent()->getStructure()->setImportPrecedence(importUnique);
  if (dependency == XSL_IMPORT)
    {
      importUnique--;
    }
  */

  // move current subtree
  subtrees.pop();
  return OK;
}

StylesheetStructure* Tree::createStylesheetStructure(Sit S)
{
    // current subtree is created in Tree::Tree() so must be NZ
    SubtreeInfo *currSubtree = NZ( subtrees.getCurrent() );
    // there is always a structure for an existing subtree
    StylesheetStructure *currStruct  = NZ( currSubtree -> getStructure() );
    // create new structure with precedence one higher
    //StylesheetStructure *newStruct = new StylesheetStructure(
    //currStruct -> getImportPrecedence() + 1);
    StylesheetStructure *newStruct = new StylesheetStructure(0);

    // file newStruct into existing tree if stylesheet structures
    currStruct -> addImportStructure(S, newStruct);
    return newStruct;    
}

eFlag Tree::findBestRule(
    Sit S, 
    XSLElement *&ret, 
    Context *c,
    QName *currMode,
    Bool importsOnly,
    SubtreeInfo *subtree /* NULL */)
{
  SubtreeInfo *start = (importsOnly && subtree) ? subtree : subtrees[0];
    return NZ(start) -> getStructure() -> findBestRule(
	S, ret, c, currMode, importsOnly);
}

Bool Tree::hasAnyStrippedName() {
  if (hasAnyStripped == -1)
    hasAnyStripped = subtrees[0] -> getStructure() -> hasAnyStripped();
  return hasAnyStripped;
}

Bool Tree::hasAnyPreservedName() {
  if (hasAnyPreserved == -1)
    hasAnyPreserved = subtrees[0] -> getStructure() -> hasAnyPreserved();
  return hasAnyPreserved;
}

Bool Tree::findStrippedName(EQName &name, int &prec, double &prio)
{
  return 
    NZ(subtrees[0]) -> getStructure() -> 
        findStrippedName(name, prec, prio);
}

Bool Tree::findPreservedName(EQName &name, int &prec, double &prio)
{
  return 
    NZ(subtrees[0]) -> getStructure() -> 
        findPreservedName(name, prec, prio);
}

XSLElement* Tree::findRuleByName(QName &q)
{
    return NZ( subtrees[0] ) -> getStructure() -> findRuleByName(*this, q);
}

/*
eFlag Tree::resolveGlobals(Sit S, Context *c, Processor *proc)
{
    sabassert(proc);
    QName temp;
    for (int i = 0; i < toplevelVars.number(); i++)
    {
	// resolve the global identified by pointer
	// the name does not matter
	temp.empty();
	E( proc -> resolveGlobal(S, c, temp, toplevelVars[i] -> getElement()) );
    }
    return OK;
}
*/

void Tree::setUnparsedEntityUri(Str &name, Str &uri)
{
  unparsedEntities.appendConstruct(name, uri);
}

Str* Tree::getUnparsedEntityUri(Str &name)
{
  return unparsedEntities.find(name);
}

eFlag Tree::pushPendingNS(Sit S, Tree* srcTree, NSList &other) 
{
  NSList *lst = new(&theArena) NSList();
  lst -> swallow(S, other, srcTree, this);
  pendingNSList.append(lst);
  return OK;
}
    
eFlag Tree::popPendingNS(Sit S)
{
  pendingNSList.freelast(FALSE);
  return OK;
}

void Tree::speakDebug()
{
  DStr foo;
  getRoot().speak(foo, (SpeakMode)(SM_OFFICIAL | SM_INS_SPACES));
  printf("--------------------\n%s\n--------------------\n", (char*)foo);
}

void Tree::dumpStructure(Sit S)
{
  S.message(MT_LOG, L_SHEET_STRUCTURE, "", "");
  for (int i = 0; i < subtrees.number(); i++)
    {
      //count level
      DStr out = "";
      int level = 0;
      SubtreeInfo * si = subtrees[i];
      while (si)
	{
	  if (level > 0) out += "  ";
	  level++;
	  si = si -> getParentSubtree();
	}
      
      out += subtrees[i] -> getBaseURI();
      S.message(MT_LOG, L_SHEET_ITEM, out, "");
    }
}

//_TH_ v
/****************************************
T m p L i s t   methods
****************************************/

TmpList::TmpList():PList<Vertex *>()
{
#ifdef _TH_DEBUG
  puts("TmpList constructor");
#endif
  //printf("TmpList constructor >%x<\n",this);
}

TmpList::~TmpList()
{
#ifdef _TH_DEBUG
  puts("TmpList destructor");
#endif
  //  dump();
  freeall(FALSE);
}
void TmpList::append(void *what)
{
  //     printf("TmpList append\n");
  ((Vertex *)what)->ordinal=number();
  PList<Vertex *>::append((Vertex *)what);
}

void TmpList::rm(int n)
{
  //     printf("TmpList rm >%d<\n",n);
  PList<Vertex *>::rm(n);
  for(int i=n;i<number();i++)
    (*this)[i]->ordinal=i;
}

void TmpList::freeall(Bool a)
{
#ifdef _TH_DEBUG
  printf("TmpList freeall(Bool)\n");
#endif
  PList<Vertex *>::freeall(a);
}

void TmpList::dump(int p)
{ 
  /*
  Str vname;
  if(p==0) printf("TmpList.dump (%d item(s))\n", nItems);
  else printf("TmpList.dump %d (%d item(s))\n", p, nItems);
  for(int i=0; i<nItems; i++){
    ((*this)[i])->getOwner().expandQStr(toE((*this)[i]) -> getName(),vname);
    printf("%d:>%d< >%x< (%s)\n",i,((*this)[i])->ordinal,((*this)[i]),(char*)vname);
  }
  */
};

int TmpList::findNum(void *p) const
{
  int i;
  for (i = number()-1; (i >= 0) && !((Vertex *)p == (*this)[i]); i--) {};
  return i;
}

void TmpList::rmP(void *p)
{
  rm(((Vertex *)p)->ordinal);
}
//_TH_ ^

Attribute* AttsCache::find(XSL_ATT what)
{
    int i;
    Attribute *a;
    int num = number();
    for (i = 0; i < num; i++)
    {
	// need to use a temporary variable
	// to get around Solaris template problem
        Vertex * pTemp = (*this)[i];
        a = cast(Attribute *, pTemp);
        if (a -> op == what)
            return a;
    };
    return NULL;
}

int AttsCache::findNdx(const QName& attName)
{
    int i;
    Attribute *a;
    int num = number();
    for (i = 0; i < num; i++)
    {
	// need to use a temporary variable
	// to get around Solaris template problem
        Vertex * pTemp = (*this)[i];
        a = toA(pTemp);
        if (attName == a -> getName())
            return i;
    };
    return -1;
}

Attribute* AttsCache::find(const QName& attName)
{
    int ndx = findNdx(attName);
    // need to use a temporary variable
    // to get around Solaris template problem
    if (ndx != -1)
      {
	Vertex * pTemp = (*this)[ndx];
	return toA(pTemp);
      } 
    else
      {
	return NULL;
      }
}
