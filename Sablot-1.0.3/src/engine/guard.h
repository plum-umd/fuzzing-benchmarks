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

#ifndef GuardHIncl
#define GuardHIncl

// GP: clean

/*
 *
 *  macro definitions for guarded pointers
 *  (i.e. pointers that automatically get deallocated on exit from a function)
 *
 */

/*
 *  DeclGuard(T)    declares a class for use in place of T*
 *  [use DeclArrGuard for pointers to arrays!]
 *
 *  Methods:
 *      keep()      forgets about the allocation so the block is preserved
 *      del()       deallocates the block explicitly
 *      delArray()  deallocates the array explicitly
 *      operator()  returns the pointer, for use in  v() -> item
 *      cast to T*  returns the pointer
 *      operator[]  behaves as if used with the pointer
 *      operator*   behaves as if used with the pointer
 *      operator=   behaves as if used with the pointer
 */

#define DeclGuard( TYPE ) \
class TYPE##_G \
{ \
public: \
    TYPE##_G(TYPE *ptr_ = NULL): ptr(ptr_), kill(TRUE) {} \
    TYPE* keep() {kill = FALSE; return ptr;} \
    TYPE* unkeep() {kill = TRUE; return ptr;} \
    void assign(TYPE* ptr_) {kill = FALSE; ptr = ptr_;} \
    TYPE* operator= (TYPE *ptr_) {kill = ptr_? TRUE : FALSE; return ptr = ptr_;} \
    operator TYPE*&() {return ptr;} \
    TYPE& operator*(void) {sabassert(ptr); return *ptr;} \
    TYPE* operator()(void) {return ptr;} \
    void del(void) {if (ptr) delete ptr; ptr = NULL; kill = FALSE;} \
    void delArray(void) {if (ptr) delete[] ptr; ptr = NULL; kill = FALSE;} \
    ~TYPE##_G() {if (kill) del();} \
protected: \
    TYPE *ptr; \
    Bool kill; \
};

/*
 *  DeclArrGuard(T) - as above, for pointers pointing to an array
 */  

#define DeclArrGuard( TYPE ) \
class TYPE##_ArrG: public TYPE##_G \
{ \
public: \
    TYPE##_ArrG(TYPE *ptr_ = NULL): TYPE##_G(ptr_) {} \
    ~TYPE##_ArrG() {if (kill) delArray();} \
    TYPE& operator [](long nIndex_) { return ptr[nIndex_]; } \
};

/*
 *  DeclDelGuard(T) - for autodeleting lists (automatic freeall(FALSE))
 */  

#define DeclDelGuard( TYPE ) \
class TYPE##_DelG: public TYPE##_G \
{ \
public: \
    TYPE##_DelG(TYPE *ptr_ = NULL): TYPE##_G(ptr_), autoDel(FALSE) {} \
    ~TYPE##_DelG() {if (kill) { if (autoDel && ptr) ptr -> freeall(FALSE); del();}} \
    TYPE* keep() {autoDel = FALSE; return TYPE##_G::keep();} \
    void autodelete() {autoDel = TRUE;}\
private:\
    Bool autoDel;\
};



/*
 *  GP() declares an actual guarded pointer,
 *  e.g. 
 *      GP( Vertex ) v;
 *  replaces
 *      Vertex *v;
 */

#define GP( TYPE ) TYPE##_G 

/*
 *  GPA() declares a guarded pointer pointing to an array
 */

#define GPA( TYPE ) TYPE##_ArrG 

/*
 *  GPD() declares a guarded pointer pointing to an autodeleting list
 */

#define GPD( TYPE ) TYPE##_DelG 

/*
 *
 *  guard declarations
 *
 */

#include "proc.h"
#include "verts.h"
#include "context.h"
#include "tree.h"
#include "parser.h"
#include "encoding.h"

DeclGuard(Vertex);
DeclGuard(Processor);
DeclGuard(Expression);
DeclGuard(SortDef);
DeclGuard(QName);
DeclGuard(EQName);
DeclGuard(Context);
DeclGuard(Str);
typedef const char* ConstCharP;
DeclGuard(ConstCharP);
typedef char GChar; //guarded character
DeclGuard(GChar);
DeclGuard(OutputHistoryItem);
DeclGuard(ExprList);
DeclGuard(DataLine);
DeclGuard(UriList);
DeclGuard(Tree);
DeclGuard(OutputterObj);
DeclGuard(OutputDefinition);
DeclGuard(OutputDocument);
DeclGuard(TreeConstructer);
DeclArrGuard(ConstCharP);
DeclArrGuard(GChar);
DeclDelGuard(ExprList);
DeclGuard(ConvInfo);

#endif // ifndef GuardHIncl
