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

#include "key.h"
#include "tree.h"
#include "guard.h"
#include "domprovider.h"

//
//  KList
//

KList::KList()
    : doc(NULL)
{
}

KList::~KList()
{
}

int KList::findNdx(const Str& keyValue) const
{
    int bottom = 0,
        top = number() - 1,
	    middle = 0;
	Bool found = FALSE;
	while (top >= bottom && !found)
	{
	    middle = (top + bottom)/2;
	    //switch(values[middle] -> compare(keyValue))
	    switch(keyValue.compare(values[middle]))
	    {
	    case 1:
	      bottom = middle + 1; break;
	    case -1:
	      top = middle - 1; break;
	    case 0:
	      found = TRUE; 
	    }
	}
	if (!found) return -1;
	// return first match
	while (middle > 0 && keyValue == values[middle-1])
	    middle--;
	return middle;
}

eFlag KList::getNodes(Sit S, const Str& keyValue, Context& result) const
{
    int ndx = findNdx(keyValue);
    if (ndx != -1)
    {
        do
	    {
            result.append((*this)[ndx]);
	    }
	    while (++ndx < number() && keyValue == values[ndx]);
    }    
    return OK;
}

eFlag KList::makeValues(Sit S, Expression& use)
{
    Context c(NULL);  //_cn_ no current in top level keys
    Expression result(use.getOwnerElement());
    NodeHandle thisVertex;
    DStr strg;
    for (int i = 0; i < number(); i++)
    {
      thisVertex = (*this)[i];
      c.set(thisVertex);
      //and use the node as the current context node
      c.setCurrentNode(thisVertex);
      E( use.eval(S, result, &c) );
      c.deppend();
      //values.append(new Str);
      switch(result.type)
	{    
	case EX_NODESET:
	  {
	    const Context& set = result.tonodesetRef();
	    if (set.isVoid()) 
	      { //add an empty key
		char *foo = new char[1]; *foo = '\0';
		values.append(foo);
		break;
	      }
	    S.dom().constructStringValue(set[0], strg);
	    //*(values.last()) = strg;
	    values.append(strg.cloneData());
	    strg.empty();
	    for (int j = 1; j < set.getSize(); j++)
	      {
		insertBefore(thisVertex, ++i);
		//values.append(new Str);
		S.dom().constructStringValue(set[j], strg);
		//*(values.last()) = strg;
		values.append(strg.cloneData());
		strg.empty();
	      }
	  }; break;
	default:
	  {
	    //E( result.tostring(S, *(values.last())) );
	    strg.empty();
	    E( result.tostring(S, strg) );
	    values.append(strg.cloneData());
	  }
	}
    }
    return OK;
}

int KList::compare(int i, int j, void *data)
{
  //int cmp = strcmp((char*)*(values[i]), (char*)*(values[j]));
  //always non wcsxfrm'ed ????
  int cmp = strcmp(values[i], values[j]);
  if (!cmp) return 0;
  return (cmp > 0) ? 1 : -1;
}

void KList::swap(int i, int j)
{
    SList<NodeHandle>::swap(i, j);
    values.swap(i, j);
}

void KList::sort(Sit S)
{
    SList<NodeHandle>::sort(0, number() - 1, &(S.dom()));
}


//
//  Key
//

Key::Key(const EQName& ename_, Expression& match_, Expression& use_)
    : ename(ename_), match(match_), use(use_)
{
}

eFlag Key::create(Sit S, SXP_Document doc)
{
    GP( Context ) c = new Context(NULL, /* isForKey = */ TRUE);
    if (find(doc))
	// subkey exists, return
	return OK;
    // create a context for 'match'
    //E( toTree(doc) -> getMatchingList(S, match, *c) );
    S.dom().getMatchingList(S, doc, match, *c);
    // only keep the array of nodes
    KList *array = (*c).getKeyArray();
    array -> incRefCount();
    array -> setSourceDoc(doc);
    subkeys.append(array);
    // create values
    E( array -> makeValues(S, use) );
    array -> sort(S);
    Str fullname;
    ename.getname(fullname);
    Log2(S, L2_KEY_ADDED, Str(array -> number()), fullname);
    // #tom#
    // list();
    //
    // c dies
    return OK;
}

const EQName& Key::getName()
{
    return ename;
}

eFlag Key::getNodes(Sit S, const Str& keyValue, Context& result, SXP_Document doc) const
{
    KList *array = find(doc);
    // if the key was declared later then the document was loaded (eg.
    // in the variable binding) we need to create the key right now
    if (! array) 
      {
	Processor *proc = S.getProcessor();
	E( proc -> makeKeysForDoc(S, doc) );
	array = find(doc);
      }
    //now get nodes
    E( NZ(array) -> getNodes(S, keyValue, result) );
    return OK;
}

Key::~Key()
{
    subkeys.freeall(FALSE);
}

void Key::report(Sit S, MsgType type, MsgCode code, 
    const Str &arg1, const Str &arg2) const
{
    S.message(type, code, arg1, arg2);
}

void Key::list()
{
    Str full;
    ename.getname(full);
    printf("// KEY %s\n", (char*)full);
    for (int i = 0; i < subkeys.number(); i++)
    {
	printf("//   \"doc %p\"\n",subkeys[i] -> getSourceDoc());
	for (int j = 0; j < subkeys[i] -> number(); j++)
	    printf("//     (%p) '%s'\n", subkeys[i] -> operator[](j), 
		   //(char*)*(subkeys[i] -> values[j]));
		   subkeys[i] -> values[j]);
	putchar('\n');
    }
}

KList* Key::find(SXP_Document doc) const
{
    int i;
    for (i = 0; i < subkeys.number(); i++)
	if (subkeys[i] -> getSourceDoc() == doc)
	    return subkeys[i];
    return NULL;
}

//
//  KeySet
//

#define KeyErr1(situa, code, ename) {\
    Str fullname; ename.getname(fullname); Err1(S, code, fullname); }

KeySet::KeySet()
: PList<Key*>(LIST_SIZE_SMALL)
{
}

eFlag KeySet::addKey(Sit S, const EQName& ename, SXP_Document doc,
    Expression& match, Expression& use)
{
    if (findKey(ename))
        KeyErr1(S, E1_DUPLICIT_KEY, ename);
    Key *newKey;
    append(newKey = new Key(ename, match, use));
    E( newKey -> create(S, doc) );
    return OK;
}

eFlag KeySet::makeKeysForDoc(Sit S, SXP_Document doc)
{
    for (int i = 0; i < number(); i++)
    {
	Key* k = (*this)[i];
	if (k -> find(doc)) break;
	E( k -> create(S, doc) );
    }
    return OK;
}

eFlag KeySet::getNodes(Sit S, const EQName& ename, SXP_Document doc,
    const Str& value, Context& result)
{    
    Key* theKey = findKey(ename);
    if (!theKey)
        KeyErr1(S, E1_KEY_NOT_FOUND, ename);
    E( theKey -> getNodes(S, value, result, doc) );
    return OK;
}

KeySet::~KeySet()
{
    freeall(FALSE);
}

Key* KeySet::findKey(const EQName& ename)
{
    for (int i = 0; i < number(); i++)
    {
        if (ename == (*this)[i] -> getName())
	    return (*this)[i];
    }
    return NULL;
}

void KeySet::report(Sit S, MsgType type, MsgCode code, 
    const Str &arg1, const Str &arg2) const
{
    S.message(type, code, arg1, arg2);
}
