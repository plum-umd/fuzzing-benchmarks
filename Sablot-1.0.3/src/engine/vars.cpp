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

#include "vars.h"
#include "situa.h"
#include "tree.h"

// GP: clean

#define iterate(VAR, LST) for(int VAR = 0; VAR < (LST).number(); VAR++)

void varDump(VarsList *which, char *text)
{
    printf("'%s': variable dump at level %d/%d\n", text, which -> currCallLevel, 
        which -> currNestLevel);
    int i, 
        whichNumber = which -> number();
    for (i = 0; i < whichNumber; i++)
    {
        VarBindings *record = (*which)[i];
        printf("%s ",(char *)(which -> sheet.expand(record -> varname.getLocal())));
        for (int j = 0; j < record -> bindings.number(); j++)
        {
            printf("%s%d/%d ", (record -> bindings[j] -> prebinding ? "P":""),
                record -> bindings[j] -> callLevel, 
                record -> bindings[j] -> nestLevel);
        }
        puts("");
    }
    puts("");
}

/*****************************************************************
    V a r B i n d i n g I t e m
*****************************************************************/

VarBindingItem::~VarBindingItem()
{
    delete expr;
}

/*****************************************************************
    V a r B i n d i n g s
*****************************************************************/

VarBindings::~VarBindings()
{
    bindings.freeall(FALSE);
}

/*****************************************************************
    V a r s L i s t
*****************************************************************/

VarsList::VarsList(Tree& sheet_)
: SList<VarBindings*>(1), sheet(sheet_)
{
    currCallLevel = currNestLevel = 0;
}

VarsList::~VarsList()
{
  freeall(FALSE);
}

VarBindings* VarsList::find(QName &name)
{
    for (int i = 0; i < number(); i++)
        if (sheet.cmpQNames((*this)[i] -> varname,name))
            return (*this)[i];
    return NULL;
}

int VarsList::compare(int first, int second, void *data)
{
    const QName 
        &q1 = (*this)[first] -> varname,
        &q2 = (*this)[second] -> varname;
    int u = sheet.expand(q1.getUri()).compare(sheet.expand(q2.getUri()));
    if (u) return u;
    u = sheet.expand(q1.getLocal()).compare(sheet.expand(q2.getLocal()));
    return u;
}

VarBindings* VarsList::getOrAdd(QName &q)
{
    VarBindings *record = find(q);
    if (!record)
    {
        record = new VarBindings(q);
        insert(record);
    };
    return record;
}

eFlag VarsList::addBinding(Sit S, QName& q, Expression *e, Bool force)
{
    VarBindings *record = getOrAdd(q);
    VarBindingItem *lastbind = NULL;

    if (!record -> bindings.isEmpty())
    {
        lastbind = record -> bindings.last();
        if (lastbind -> callLevel == currCallLevel) // && !lastbind -> prebinding)
            Err1(S, E1_MULT_ASSIGNMENT, (char*)sheet.expand(q.getLocal()));
    }

    /*
        construct the new binding item. if there is a pre-binding that applies,
        just change it to a binding and drop the current expression. otherwise,
        add the new item.
    */

    VarBindingItem *it;
    record -> bindings.append(it = new VarBindingItem);
    it -> callLevel = currCallLevel;
    it -> nestLevel = currNestLevel;
    it -> prebinding = FALSE;
    if (lastbind && lastbind -> prebinding && 
        lastbind -> callLevel == (currCallLevel - 1) && 
        lastbind -> nestLevel == currNestLevel &&
        !force)
    {
        it -> expr = new Expression(e -> getOwnerElement()); // GP: OK
        delete e; // moved above the following line
        E( lastbind -> expr -> eval(S, *(it -> expr), NULL) );
    }
    else
        it -> expr = e;
    return OK;
}

eFlag VarsList::addPrebinding(Sit S, QName &q, Expression *e)
{
    VarBindings *record = getOrAdd(q);
    VarBindingItem *lastbind = NULL;

    if (!record -> bindings.isEmpty())
    {
        lastbind = record -> bindings.last();
        if (lastbind -> nestLevel == currNestLevel && 
            lastbind -> callLevel == currCallLevel &&    // new
            lastbind -> prebinding)
            Err1(S, E1_MULT_ASSIGNMENT, (char*)sheet.expand(q.getLocal()));
    }

    VarBindingItem *it = new VarBindingItem;
    it -> callLevel = currCallLevel; 
    it -> nestLevel = currNestLevel;
    it -> expr = e;
    it -> prebinding = TRUE;
    record -> bindings.append(it);

    return OK;
}

void VarsList::rmBinding(QName &q)
{
    VarBindings *record = find(q);
    sabassert(record && record -> bindings.number());
    record -> bindings.freelast(FALSE);
}

Expression* VarsList::getBinding(QName &q)
{
    VarBindings *record = find(q);
    if (!record)
	return NULL;
    return getBinding(record);
}

Expression* VarsList::getBinding(VarBindings* record)
{
    if (!record || record -> bindings.isEmpty()) 
        return NULL;

    for (int i = record -> bindings.number() - 1; 
        (i >= 0) && (record -> bindings[i] -> callLevel == currCallLevel);
        i--)
        {
            if (!record -> bindings[i] -> prebinding)
                return record -> bindings[i] -> expr;
        };

    // try to return the global value
//    if (!record -> bindings[0] -> callLevel && !record -> bindings[0] -> prebinding)
    if (record -> bindings[0] -> callLevel == 1)
        return record -> bindings[0] -> expr;
    else 
    {
        if ((record -> bindings.number() > 1) && 
        (record -> bindings[1] -> callLevel == 1))
            return record -> bindings[1] -> expr;
        else
            return NULL;
    }
}

void VarsList::startCall()
{
    currCallLevel++;
//    varDump(this, "startCall");
}

/*================================================================
endCall
================================================================*/

void VarsList::_endCall(Bool rmLastLevelPrebs)
{
//    varDump(this, "_endCall before");
    VarBindingItem* lastbind;
    for (int i = 0; i < number(); i++)
    {
        VarBindings *record = (*this)[i];
        if (!record -> bindings.isEmpty())
        {
            while ( !record -> bindings.isEmpty() &&
                    (((lastbind=record -> bindings.last()) -> 
                            callLevel == currCallLevel) ||
                        (rmLastLevelPrebs &&
                            lastbind -> callLevel == currCallLevel - 1 &&
                            lastbind -> prebinding)) && 
                    lastbind -> nestLevel >= currNestLevel) // was ==
            {
                record -> bindings.freelast(FALSE);
            }
        }
    }
//    varDump(this, "_endCall after");
    currCallLevel--;
}

void VarsList::endCall()
{
    _endCall(TRUE);
}

void VarsList::startNested()
{
    currNestLevel++;
}

void VarsList::endNested()
{
    currNestLevel--;
}

void VarsList::startApplyOne()
{
    startCall();
}

void VarsList::endApplyOne()
{
    _endCall(FALSE);
}

void VarsList::rmPrebindings()
{
    VarBindingItem* lastbind;
    for (int i = 0; i < number(); i++)
    {
        VarBindings *record = (*this)[i];
        if (!record -> bindings.isEmpty() && 
            (lastbind = record -> bindings.last()) -> callLevel == currCallLevel &&
            lastbind -> prebinding && 
            lastbind -> nestLevel >= currNestLevel)
        {
            record -> bindings.freelast(FALSE);
        }
    }
}

eFlag VarsList::openGlobal(Sit S, QName& name, VarBindings *&record)
{
    if (!record)
	record = find(name);
    if (!record)
	insert(record = new VarBindings(name));
    record -> isOpenGlobal = TRUE;
    return OK;
}

eFlag VarsList::closeGlobal(Sit S, VarBindings *record)
{ 
    record -> isOpenGlobal = FALSE; 
    return OK;
}

void VarsList::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type,code,arg1,arg2);
}

void VarsList::pushCallLevel(int level)
{
  callLevels.append(currCallLevel);
  currCallLevel = level;
}

void VarsList::popCallLevel()
{
  currCallLevel = callLevels[callLevels.number() - 1];
  callLevels.deppend();
}
