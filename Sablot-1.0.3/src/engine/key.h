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

#ifndef KeyHIncl
#define KeyHIncl

#include "base.h"
#include "context.h"
#include "sxpath.h"

class KList : public CList
{
public:
    KList();
    ~KList();
    eFlag makeValues(Sit S, Expression& use);
    eFlag getNodes(Sit S, const Str& keyValue, Context& result) const;
    virtual int compare(int i, int j, void *data);
    virtual void swap(int i, int j);    
    void sort(Sit S);
    void setSourceDoc(SXP_Document doc_) 
	{doc = doc_;}
    SXP_Document getSourceDoc() 
	{return doc;}
private:
    int findNdx(const Str& keyValue) const;
    SXP_Document doc;
};

class Key
{
public:
    Key(const EQName& ename_, Expression& match_, Expression& use_);
    eFlag create(Sit S, SXP_Document doc);
    eFlag getNodes(Sit S, const Str& keyValue, Context& result, SXP_Document doc) const;
    const EQName& getName();
    ~Key();
    void report(Sit S, MsgType type, MsgCode code, 
        const Str &arg1, const Str &arg2) const;
    KList* find(SXP_Document doc) const;
private:
    const EQName ename;
    PList<KList*> subkeys;
    Expression &match,
        &use;    
    void list();	
};

class KeySet: public PList<Key*>
{
public:
    KeySet();
    eFlag addKey(Sit S, const EQName& ename, SXP_Document doc,
		 Expression& match, Expression& use);
    eFlag makeKeysForDoc(Sit S, SXP_Document doc);
    eFlag getNodes(Sit S, const EQName& ename, SXP_Document doc,
        const Str& value, Context& result);
    ~KeySet();
    void report(Sit S, MsgType type, MsgCode code, 
		const Str &arg1, const Str &arg2) const;		
private:
    Key* findKey(const EQName& ename);
};

#endif // KeyHIncl
