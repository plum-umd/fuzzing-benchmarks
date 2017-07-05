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

#ifndef VarsHIncl
#define VarsHIncl

// GP: clean

#include "base.h"
#include "datastr.h"
#include "expr.h"

class VarBindingItem /* : public SabObj */
{
public:
    VarBindingItem()
	: expr(NULL), callLevel(-1), nestLevel(-1), prebinding(FALSE)
	{};
    ~VarBindingItem();
    Expression *expr;
    int callLevel, nestLevel;
    Bool prebinding;
};

class VarBindings /* : public SabObj */
{
public:
    VarBindings(QName& varname_)
	: varname(varname_), isOpenGlobal(FALSE)
    {
    }
    ~VarBindings();
    QName varname;
    // TRUE if this is a global variable currently processed for forward references
    PList <VarBindingItem*> bindings;
    Bool isOpenGlobal;
};

class Tree;

class VarsList : public SList<VarBindings *>
{
public:
    VarsList(Tree&);
    ~VarsList();
    VarBindings* find(QName&);
    virtual int compare(int first, int second, void *data);
    eFlag addBinding(Sit S, QName&, Expression *, Bool force);
    eFlag addPrebinding(Sit S, QName&, Expression *);
    void rmBinding(QName&);
    Expression* getBinding(QName &);
    Expression* getBinding(VarBindings*);
    void startCall();
    void endCall();
    void pushCallLevel(int level);
    void popCallLevel();
    void startNested();
    void endNested();
    void startApplyOne();
    void endApplyOne();
    void rmPrebindings();
    int currCallLevel, currNestLevel;
    // open the global variable (mark as being processed for forward refs)
    // if it doesn't exist, create it; return the pointer to VarBindings* in 'record'
    // if non-NULL record is passed, use it instead of name for performance
    eFlag openGlobal(Sit S, QName& name, VarBindings *&record);
    // close the global variable after having succesfully resolved the forward refs
    eFlag closeGlobal(Sit S, VarBindings *record);
    // see if the var with given 'record' is an open global
    Bool isOpenGlobal(VarBindings *record)
	{ return record -> isOpenGlobal; }
    Tree& sheet;
private:
    List<int> callLevels;
    VarBindings* getOrAdd(QName &);
    void _endCall(Bool rmLastLevelPrebs);
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
};

#endif  // ifndef VarsHIncl
