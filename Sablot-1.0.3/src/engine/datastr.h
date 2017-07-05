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

//
// datastr.h
//
// most constants (list sizes etc.) are in base.h

// GP: clean

#ifndef DataStrHIncl
#define DataStrHIncl

// error.h will be included later:
#define BaseH_NoErrorH
#include "base.h"
// #include "situa.h"
#undef BaseH_NoErrorH

typedef unsigned long Phrase;
class HashTable;
class Tree;

#define QSORT_THRESHOLD     10      // insertsort is used for smaller lists

// List is a basic structure for ordered lists. 
// If the T's are pointers, you may want to use PList instead, which
// allows you to delete the pointers (in List, they are just taken off the 
// list).

template <class T>
class List /* : public SabObj */
{
public:
    List(int logBlocksize_ = LIST_SIZE_SMALL);
        // append an element to the end of list
    void append(T x);
        // remove last element from list
    void deppend();
        // remove all elements (shrink list to size 0)
    void deppendall();
        // remove element with given index
    void rm(int);
        // test whether list is empty
    Bool isEmpty() const;
        // number of elements in list
    inline int number() const;
        // get element with given index
    T& operator[](int ndx) const;
        // get last element
    inline T& last() const;
        // swap two elements with given indices
    virtual void swap(int,int);
    void insertBefore(T newMember, int refIndex);
    virtual ~List(void);
protected:
    void grow();
    int nItems; 
    T *block;
    int blocksize, origBlocksize;
    virtual T* claimMemory( int nbytes ) const { return (T*) malloc(nbytes); }
    virtual T* reclaimMemory( T *p, int newbytes, int oldbytes ) const 
        { return (T*) realloc(p, newbytes); }
    virtual void returnMemory( T* &p ) const { if (p) free(p); p = NULL; }
};

// PList is for lists whose members are pointers. It adds the possibility
// to delete pointers when removing them from the list.
// The Bool parameters in its methods say whether delete[] or delete should
// be used (TRUE/FALSE, respectively).
//
template <class T>
class PList : public List<T>
{
public:
    PList(int logBlocksize_ = LIST_SIZE_SMALL);
        // free and remove the last pointer
    void freelast(Bool);
        // free and remove all pointers
    void freeall(Bool);
        // free and remove the given pointer
    void freerm(int, Bool);
};


/*****************************************************************
DynBlock

  is a class for "dynamic memory blocks", i.e. blocks that can 
  grow in size.
*****************************************************************/

struct DynBlockItem
{
    char *data;
    int byteCount;
    DynBlockItem *next;
};

class DynBlock
{
    friend class DStr;
public:
    DynBlock();
    ~DynBlock();
    void nadd(const char *data, int bytes);
    Bool isEmpty() const;
    char* getPointer() const;
    char* compactToBuffer() const;
protected:
    int byteCount;
    DynBlockItem *first, *last;
    void remove();
    void compact() const;
    int compactToBuffer_(char* buf, Bool kill_);
};


// block for the dynamic strings
class DynStrBlock : public DynBlock
{
public:
        // compact the string into one chunk,
        // prepending 'firstPart' and appending 0.
    char* compactString_(const char *firstPart, int firstLen);
};


/*****************************************************************

    Str
    static string

*****************************************************************/

class DStr;

class Str
{
public:
    friend class DStr;
        // constructors: default, copy, from char, char* and from
        // dynamic string
    Str();
    Str(const Str& string);
    Str(int num);
    Str(double num);
    Str(char c);
    Str(const char *chars);
    Str(const DStr &dstring);
    
        // assignment: from DStr, Str, char*, char, int, double
    Str& operator=(const DStr& string);
    Str& operator=(const Str& string);
    Str& operator=(const char *chars);
    Str& operator=(char c);
    Str& operator=(int number);
    Str& operator=(double dblnum);
    
        // set self to a string of length 'len' (non-NULL-terminated)
    void nset(const char* chars, int len);
    
        // set self to an empty string
    virtual void empty();
    
        // test whether the string is void
    Bool isEmpty() const;
    
        // get byte with the given index
        // NEEDS TO INCLUDE pack_()
    char operator[](int index) const;
    
        // convert to const char* (just return the internal pointer)
        // NEEDS TO INCLUDE pack_()
    virtual operator char*() const;
    
        // test for < / == / > against a Str, char*
        // NEEDS TO INCLUDE pack_()
    int compare(const Str &other) const;
    int compare(const char *otherChars) const;
    
        // test for being lex-smaller than other Str, char*
        // NEEDS TO INCLUDE pack_()
    Bool operator< (const Str &);
    Bool operator< (const char*);
    
        // test for equality: to a Str, char*
        // NEEDS TO INCLUDE pack_()
    Bool operator== (const Str& other) const;
    Bool operator== (const char* otherChars) const;
    Bool operator== (char* otherChars) const;
    
        // non-case-sensitive test for equality: to a Str, char*
        // NEEDS TO INCLUDE pack_()
    Bool eqNoCase(const Str& other) const;
    Bool eqNoCase(const char* otherChars) const;
    
        // get the byte length of the string
    virtual int length() const;
    
        // addition operator: with char*, Str and int
        // NEEDS TO INCLUDE pack_()
    DStr operator+ (const Str& other) const;
    DStr operator+ (const char *otherChars) const;
    DStr operator+ (int otherNum) const;
    
        // convert *this to double, return FALSE if OK
    Bool toDouble(double& retval) const;
    
    DStr& appendSelf(DStr& other);


    // THE FOLLOWING 3 FUNCTIONS WOULD BE BEST REMOVED
        // output *this to another string with whitespace trimmed
    void speakTerse(DStr &);

    char* cloneData();

    virtual ~Str();
protected:
        // deallocate all memory used
    virtual void remove_() { returnMemory(text_); }
    
        // pack all parts (in derived classes) to form a valid Str
    virtual void pack_() const;

    // claimMemory and returnMemory are to facilitate allocating the character data in an arena
    virtual char* claimMemory( int nbytes ) const { return new char[nbytes]; }

    virtual void returnMemory( char *&p ) const { if (p) delete[] p; p = NULL; }
    
    char* text_;
    int byteLength_;
};


/*****************************************************************
    DStr
    dynamic string
*****************************************************************/

class DStr : public Str
{
public:
    DStr();
    DStr(char c);
    DStr(const char *chars);
    DStr(const Str& string);
    DStr(const DStr& dstring);
    
    DStr& operator=(const DStr& other);
    
        // append a string or whatever
    DStr& operator+= (const DStr& other);
    DStr& operator+= (const Str& other);
    DStr& operator+= (const char* otherChars);
    DStr& operator+= (char c);
    DStr& operator+= (int num);

    virtual DStr& appendSelf(DStr& other);
    
        // nadd adds a non-zero-terminated string
    DStr& nadd(const char *,int);
    
    virtual operator char*() const;
    virtual int length() const;

        // consume is just like += but it steals the char*'s from 'other'.
/*
    DStr& operator consume(Str &other);
    DStr& operator consume(DStr &other);
*/
    
    virtual ~DStr();
private:
        // deallocate all memory used
    virtual void remove_();    
        // pack all parts (in derived classes) to form a valid Str
    virtual void pack_() const;
    DynStrBlock blocks;
};


//
//
//  string constants and functions
//
//

void escapeChars(DStr& result, const Str& what, 
                 const char* toEscape, const char** substitutes);

/* extern const Str* theEmptyString; */

/*****************************************************************
NamespaceStack
*****************************************************************/

class NamespaceStackObj /* : public SabObj */
{
public:
   Str prefix, uri;
   Bool hidden;
};

class NamespaceStack : public PList<NamespaceStackObj*>
{
public:
        // find the index of the element with the given key
    int findNum(const Str &_key) const;
        // find the value of the element with the given key
    const Str* getUri(const Str &) const;
    Bool isHidden(const Str &) const;
    void appendConstruct(const Str &prefix, const Str& uri,
			 Bool _hidden);
};

/****************************************
Q N a m e
****************************************/
// container for a QName (e.g. "books:isbn") consisting
// of the "namespace prefix", "namespace URI" and the "local part"

class QName
{
    friend class TreeConstructer;
public:
    QName();
    QName(const QName &other);
        // set the QName from the string received from expat (using NS_SEP)
    QName& operator =(const QName& other);
    Bool isEmpty() const;
    void empty();

        // set just the prefix
    void setPrefix(Phrase);
    void setLocal(Phrase);
    void setUri(Phrase);

        // return the full name
    // Str getname(Tree& t) const;
        // return the local part
    Phrase getLocal() const;
        // return the URI
    Phrase getUri() const;
        // return the prefix
    Phrase getPrefix() const;
        // return true if a prefix is defined
    Bool hasPrefix() const;

        // check for equality to another qname
    Bool operator==(const QName &) const;

        // output to a string
    // void speak(DStr&, SpeakMode) const;
        // check if our own prefix is bound to the given namespace
    // Bool prefixIsNS(const Str &s) const;
        // get the associated dictionary
private:
    Phrase
        prefix,
        uri,
        local;
};

/*****************************************************************
Q N a m e L i s t
*****************************************************************/

class QNameList : public PList<QName *>
{
public:
    // finds item with equal local-part and uri
    int findNdx(const QName &what) const;
};

/****************************************
E Q N a m e
****************************************/

class EQName
{
//    friend class TreeConstructer;
public:
    EQName();
    EQName(const EQName&);
    Bool isEmpty() const;
    void empty();

    void set(const QName&, const HashTable&);
    void setPrefix(const Str&);
    void setLocal(const Str&);
    void setUri(const Str&);

        // return the full name
    void getname(Str& fullName) const;
        // return the local part
    const Str& getLocal() const;
        // return the URI
    const Str& getUri() const;
        // return the prefix
    const Str& getPrefix() const;
        // return true if a prefix is defined
    Bool hasPrefix() const;

        // check for equality to another qname
    Bool operator==(const EQName &) const;

private:
    Str
        prefix,
        uri,
        local;
};



/*****************************************************************
    S t r S t r L i s t
*****************************************************************/

class StrStr /* : public SabObj */
{
public:
    Str 
        key,
        value;
};

class StrStrList : public PList<StrStr*>
{
public:
        // find the index of the element with the given key
    int findNum(const Str &_key) const;
        // find the value of the element with the given key
    Str* find(const Str &) const;
    void appendConstruct(const Str &key, const Str& value);
};


/*****************************************************************
    E Q N a m e L i s t
*****************************************************************/

class EQNameList : public PList<EQName *>
{
public:
    const EQName* find(const EQName &what) const;
};

/*****************************************************************
    E Q N a m e S t r L i s t
*****************************************************************/
struct EQNameStr
{
    EQNameStr(const EQName& key_, const Str& value_) : key(key_), value(value_)
    {
    };
    EQName key;
    Str value;
};

class EQNameStrList : public PList<EQNameStr *>
{
public:
    EQNameStrList()
    {
    };
    int findNdx(const EQName &what) const;
    const Str* find(const EQName &what) const;
    void appendConstruct(const EQName &key, const Str& value);
};

/*****************************************************************

    S L i s t

*****************************************************************/

template <class T>
class SList: public PList<T>
{
public:
    SList(int logBlocksize_);
        // comparison of two elements
    virtual int compare(int, int, void *data)
    { sabassert(!"abstract compare"); return 0; }
        // insertion of an element
    void insert(T, void *data = NULL);
    void sort(int from = 0, int to = - 1, void *data = NULL);  // to == -1 means "nItems-1"
private:
    void quicksort(int bottom, int top, void *data);
    void qsPartition(int& i, int& j, void *data);
    void insertsort(int bottom, int top, void *data);
};

/***********

  SortDef

***********/

class Expression;

class SortDef
{
public:
    Expression *sortExpr;
    Str lang;
    Bool asText, ascend, upper1st;
    SortDef() 
      : sortExpr(NULL), asText(TRUE), ascend(TRUE), upper1st(FALSE)
        {};
    SortDef(Expression *sortExpr_, Str lang_, 
        Bool asText_, Bool ascend_, Bool upper1st_)
        : sortExpr(sortExpr_), lang(lang_),
          asText(asText_), ascend(ascend_), upper1st(upper1st_)
        {};
};

/***********

  SortDefList

***********/

class SortDefList : public PList<SortDef*>
{
    void appendConstruct(Expression *sortExpr_, Str lang_, 
        Bool asText_, Bool ascend_, Bool upper1st_)
    {
        append(new SortDef(sortExpr_, lang_,
            asText_, ascend_, upper1st_));
    };
};

/***************************************
U r i  L i s t
****************************************/

/****************************************/
/* exclusion prefix list                */

class UriList : public PList<Phrase>
{
public:
  //UriList();
  void addUri(Phrase);
  //void removeUri(Str & uri);
  int findNdx(Phrase);
};

/*****************************************************************

t e m p l a t e   m e t h o d s

*****************************************************************/

#include "situa.h"
//#include "error.h"


/*================================================================
List methods
================================================================*/

template <class T>
List<T>::List(int logBlocksize_) 
:
origBlocksize(TWO_TO(logBlocksize_))
{
    nItems = 0;
    blocksize = 0;
    block = NULL;
};


template <class T>
void List<T>::append(T what)
{
    if (nItems >= blocksize)
    {
        if (block)
            grow();
        else
        {
            blocksize = origBlocksize;
            block = (T*) claimMemory(blocksize * sizeof(T));
            // FIXME: asserting memory request ok
            sabassert(block);
        }
    }
    block[nItems++] = what;
};

template <class T>
void List<T>::deppend()
{
    sabassert (nItems > 0);
    --nItems;
    if (!(nItems & (nItems - 1)) && (nItems >= origBlocksize))   // realloc at powers of 2
    {
        int oldblocksize = blocksize;
        blocksize = nItems;
        if (blocksize)
        {
            block = (T*) reclaimMemory(block, blocksize * sizeof(T),
                oldblocksize * sizeof(T));
            // FIXME: asserting memory request ok
            sabassert(block);
        }
        else
            returnMemory(block);
    }
};

// double the block size

template <class T>
void List<T>::grow()
{
    if (!block) return;
    blocksize = blocksize << 1;
    int nbytes = blocksize * sizeof(T);
    block = (T*) reclaimMemory(block, nbytes, nbytes >> 1);
    // FIXME: asserting memory request ok
    sabassert(block);
}

template <class T>
void List<T>::rm(int n)
{
    sabassert((n >= 0) && (n < nItems));
    memmove(block + n, block + n + 1, (nItems - n - 1) * sizeof(T));
    deppend();
}

template <class T>
void List<T>::swap(int i, int j)
{
    sabassert((i >= 0) && (i < nItems));
    sabassert((j >= 0) && (j < nItems));
    T temp = block[i];
    block[i] = block[j];
    block[j] = temp;
}

template <class T>
void List<T>::deppendall() 
{
    nItems = 0;
    blocksize = 0;
    returnMemory(block);
/*    if (block)
        free(block);
        */
//    block = NULL;
}

template <class T>
inline int List<T>::number() const 
{
    return nItems;
}

template <class T>
List<T>::~List()
{
    deppendall();
};

template <class T>
inline T& List<T>::operator[](int ndx) const
{
    sabassert((ndx < nItems) && (ndx >= 0));
    return block[ndx];
};

template <class T>
inline T& List<T>::last() const
{
    sabassert(nItems);
    return block[nItems - 1];
}

template <class T>
Bool List<T>::isEmpty() const
{
    return (Bool) (!nItems);
}

template <class T>
void List<T>::insertBefore(T newMember, int refIndex)
{
    append(newMember);
    memmove(block + refIndex + 1, block + refIndex, 
        (number()-refIndex-1) * sizeof(T));
	block[refIndex] = newMember;
}

/*================================================================
PList methods
================================================================*/

#define deleteScalarOrVector(PTR, VECT) \
    {if (VECT) delete[] PTR; else delete PTR;}

template <class T>
PList<T>::PList(int logBlocksize_)
: List<T>(logBlocksize_)
{
}

template <class T>
void PList<T>::freelast(Bool asArray)
{
    deleteScalarOrVector(this->last(), asArray);
    this->deppend();
}

template <class T>
void PList<T>::freeall(Bool asArray)
{
    for (int i = 0; i < this->nItems; i++)
        deleteScalarOrVector(this->block[i], asArray);
    this->deppendall();
}

template <class T>
void PList<T>::freerm(int n, Bool asArray)
{
    sabassert((n >= 0) && (n < this->nItems));
    deleteScalarOrVector(this->block[n], asArray);
    this->rm(n);
}

/*================================================================
SList methods    
================================================================*/

template <class T>
SList<T>::SList(int logBlocksize_) 
    : PList<T>(logBlocksize_)
{
};

template <class T>
void SList<T>::insert(T what, void *data /* = NULL */)
{
    this->append(what);
    int whatndx = this->number() - 1;
    int i, j;
    for (i=0; i < whatndx; i++)
        if (compare(whatndx, i, data) == -1) break;
    if (i < whatndx)
    {
        for (j = whatndx; j > i; j--)
            this->operator[](j) = this->operator[](j-1);
        this->operator[](i) = what;
    };
};

template <class T>
void SList<T>::insertsort(int bottom, int top, void *data)
{
    int ceiling, k;
    //T curr;
    for (ceiling = bottom + 1; ceiling <= top; ceiling++)
    {
      //curr = this->block[ceiling];
        /* was:
        for (k = ceiling - 1; k >= bottom && compare(k, ceiling) > 0; k--)
            this->block[k+1] = this->block[k];
        this->block[k+1] = curr;
         * but need to use swap() to drag extra pointers along
         */
        for (k = ceiling - 1; k >= bottom && compare(k, k + 1, data) > 0; k--)
            this->swap(k, k+1); 
    }
}

template <class T>
void SList<T>::qsPartition(int& bottom, int& top, void *data)
{
    int middle = (bottom + top) >> 1;
    if (compare(bottom, top, data) > 0) this->swap(bottom, top);
    if (compare(middle, top, data) > 0) this->swap(middle, top);
    if (compare(bottom, middle, data) > 0) this->swap(bottom, middle);
    // T pivot = this->block[middle];
    while (bottom <= top)
    {
        // boundary conditions fixed by Tim
        while ((++bottom <= top) && compare(bottom, middle, data) <= 0);
        while ((--top >= bottom) && (compare(top, middle, data) >= 0));

        if (bottom < top) 
        {
            if (middle == bottom)
                middle = top;
            else if (middle == top)
                middle = bottom;
            this->swap(bottom, top);
        }
    }
}

template <class T>
void SList<T>::quicksort(int bottom, int top, void *data)
{
    sabassert(QSORT_THRESHOLD >= 3);
    int i = bottom, j = top;
    if (top - bottom < QSORT_THRESHOLD)
        insertsort(bottom, top, data);
    else
    {
        qsPartition(i, j, data);
        quicksort(bottom, j, data);
        quicksort(i, top, data);
    }
}

template <class T>
void SList<T>::sort(int from /* = 0 */, int to /* = - 1 */, void *data /* = NULL */)
{
    /* cannot initialize 'to' to nItems-1 directly as g++ reports error */ 
    if (to == -1)
        to = this->nItems - 1;
    if (this->nItems < 2)
        return;
    quicksort(from, to, data);
}

/******************* expression list moved here ***************/
typedef PList<Expression *> ExprList;

/******************* sorted string list ***********************/
class SortedStringList: public SList<Str*> {
 public:
  SortedStringList() : SList<Str*>(LIST_SIZE_MEDIUM) {};
  virtual int compare(int, int, void* data);
  int findIdx(Str & val);
};

#endif //ifndef DataStrHIncl

