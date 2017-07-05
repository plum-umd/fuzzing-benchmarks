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

#ifndef ContextHIncl
#define ContextHIncl

// GP: clean

/*****************************************************************

    c   o   n   t   e   x   t   .   h

*****************************************************************/

#include "expr.h"
#include "verts.h"

class SortDefList;

class CList : public SList<NodeHandle>
{
public:
    friend class Key;
    CList();
    ~CList();
    virtual int compare(int i, int j, void *data);
    virtual void swap(int i, int j);
    void incRefCount();
    int decRefCount();
    eFlag sort(Sit S, XSLElement *caller, Context*, SortDefList* sortDefs_ = NULL);
protected:
    Bool wcsValues; //says whether values are wcsxfrm'ed
    PList<char*> values;
    List<int> tags; //used for the nested sort
private:
    int refCount;
    SortDefList *sortDefs;
    eFlag makeValues(Sit S, int from, int to, int level, 
        XSLElement *caller, Context *ctxt);
    int currLevel;
    int compareWithoutDocOrd(int i, int j);
    Bool tagChanged(int i, int j);
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);    
};


/*****************************************************************

    C   o   n   t   e   x   t

  expression evaluation context

*****************************************************************/

class KList;

class Context 
{
    friend class CList;
public:
    Context(NodeHandle current, Bool isForKey_ = FALSE);
    ~Context();
    int 
        getPosition() const;
    int 
        getSize() const;
    NodeHandle 
        current() const;
    NodeHandle
        getCurrentNode() const;
    void
        setCurrentNode(NodeHandle);
    void    
        reset();
    Bool 
        isFinished() const;
    Bool
        isVoid() const;
    NodeHandle 
        shift();
    void    
        set(NodeHandle);
    void 
        setVirtual(NodeHandle v, int virtualPosition_, int virtualSize_);
    NodeHandle 
        operator[] (int) const;
    void
        deppendall();
    void
        append(NodeHandle);
    void
        deppend();
    Context
        *copy();
    Context 
        *swallow(Sit S, Context *);
    void
        swap(int, int);
    Bool
        contains(NodeHandle v) const;

    KList* getKeyArray();
    
    eFlag sort(Sit S, XSLElement *caller = NULL, SortDefList* sortDefs_ = NULL)
    {
        sabassert(caller || !sortDefs_);
        E( array -> sort(S, caller, this, sortDefs_) );
        return OK;
    }
    void uniquize();
    CList* getArrayForDOM();
protected:
    CList 
        *array;
    NodeHandle 
        currentNode;
    void setPosition(int pos)
    {
        position = pos;
    };
    int
        position,
        virtualPosition,
        virtualSize;
	Bool isForKey;
};

#endif  //ifndef ContextHIncl
