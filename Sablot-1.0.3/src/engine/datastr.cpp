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

#include "datastr.h"
#include "verts.h"
#include "utf8.h"
#include "tree.h"

// GP: clean (only 1 eFlag routine)

// #include <malloc.h> - not needed and deprecated anyway

/* const Str* theEmptyString = new Str(""); */

//     implementations for List are in datastr.h (List is a template)


/*****************************************************************
DynBlock

  is a class for "dynamic memory blocks", i.e. blocks that can 
  grow in size.
*****************************************************************/

DynBlock::DynBlock()
{
    first = last = NULL;
    byteCount = 0;
}

inline void DynBlock::remove()
{
    DynBlockItem *d, *d_was;
    if (first)
    {
        for (d = first; d; )
        {
            d_was = d;
            d = d_was -> next;
            delete [] d_was -> data;
            delete d_was;
        }
    };
    first = last = NULL;
    byteCount = 0;
}

DynBlock::~DynBlock()
{
    remove();
}

inline Bool DynBlock::isEmpty() const
{
    return !byteCount;
}

void DynBlock::nadd(const char *data, int bytes)
{
    DynBlockItem *newitem = new DynBlockItem;
    newitem -> data = new char[bytes];
    memcpy(newitem -> data, data, bytes);
    newitem -> byteCount = bytes;
    newitem -> next = NULL;
    if (last)
        last -> next = newitem;
    else
        first = newitem;
    last = newitem;
    byteCount += bytes;
}

char* DynBlock::getPointer() const
{
    if (!first)
        return NULL;
    if (first -> next)
        compact();
    return (first -> data);
}

// compactToBuffer() joins the parts in a buffer. Returns buf+byteCount.
// if kill_ then the parts will be deallocated

int DynBlock::compactToBuffer_(char* buf, Bool kill_)
{
    int compactedBytes = 0;
    char *p = buf;
    if (first)
    {
        DynBlockItem *d, *d_was;
        for (d = first; d; )
        {
            memcpy(p, d -> data, d -> byteCount);
            p += d -> byteCount;
            compactedBytes += d -> byteCount;
            d = (d_was = d) -> next;
            if (kill_)
            {
                delete[] d_was -> data;
                delete d_was;
            }
        };
        if (kill_)
        {
            first = last = NULL;
            byteCount = 0;
        }
    }
    return compactedBytes;
}


#define thisNoConst (const_cast<DynBlock*>(this))


char* DynBlock::compactToBuffer() const
{
    char *newdata = new char[byteCount+1];
    newdata[thisNoConst -> compactToBuffer_(newdata, FALSE)] = 0;
    return newdata;
}

void DynBlock::compact() const
{
    if (!first || !(first -> next)) return;
    int byteCountWas = byteCount;
    char *newdata = new char[byteCount];
    thisNoConst -> compactToBuffer_(newdata, TRUE);
    thisNoConst -> first = thisNoConst -> last = new DynBlockItem;
    thisNoConst -> first -> data = newdata;
    thisNoConst -> byteCount = first -> byteCount = byteCountWas;
    thisNoConst -> first -> next = NULL;
}



//
//     DynStrBlock
//     a flavour of DynBlock for use in dynamic strings
//
// compactString() allocates a new buffer to hold 'firstPart'
// plus the concat of all the DynStrBlock's parts. Kills the parts
// of the DynStrBlock.
//
char* DynStrBlock::compactString_(const char *firstPart, int firstLen)
{
    int newLength = firstLen + byteCount;
    char *newdata = new char[newLength + 1];
    if (firstLen)
        memcpy(newdata, firstPart, firstLen);
    if (first)
        compactToBuffer_(newdata + firstLen, TRUE);
    newdata[newLength] = 0;
    return newdata;
}




//
//                Str
//                static string
//


Str::Str()
{
    text_ = NULL;
    byteLength_ = 0;
}

Str::Str(const Str& string)
{
    text_ = NULL;
    operator= (string);
}

Str::Str(char c)
{
    text_ = NULL;
    operator= (c);
}

Str::Str(int num)
{
    text_ = NULL;
    operator= (num);
}

Str::Str(double num)
{
    text_ = NULL;
    operator= (num);
}

Str::Str(const char *chars)
{
    if (!chars) 
        chars="";
    text_ = NULL;
    operator= (chars);
}

Str::Str(const DStr &dstring)
{
    text_ = NULL;
    operator= (dstring);
}

Str& Str::operator=(const Str& string)
{
    remove_();
    int hisByteLen = string.length();
    byteLength_ = hisByteLen;
    text_ = claimMemory(hisByteLen + 1);
    memcpy(operator char*(), string.operator char*(), hisByteLen+1);
    return *this;
}

Str& Str::operator=(const DStr& dynamic)
{
    remove_();
    nset((char*) dynamic, dynamic.length());
    return *this;
}

Str& Str::operator=(const char* chars)
{
    nset(NZ(chars), strlen(chars));
    return *this;
}

Str& Str::operator= (char c)
{
    remove_();
    char *p;
    *(p = claimMemory(2)) = c;
    p[1] = 0;
    byteLength_ = 1;
    text_ = p;
    return *this;
}

Str& Str::operator= (int num)
{
    remove_();
    char buf[20];
    sprintf(buf,"%d",num);
    operator=(buf);
    return *this;
}

Str& Str::operator= (double num)
{
    remove_();
    char buf[20];
    sprintf(buf,"%.13g",num);
    operator=(buf);
    return *this;
}

void Str::nset(const char* chars, int len)
{
    sabassert(chars);
    remove_();
    byteLength_ = len;
    text_ = claimMemory(len + 1);
    memcpy(text_, chars, len);
    text_[len] = 0;
}

char Str::operator[] (int index) const
{
    sabassert(index >= 0 && index <= length());
    pack_();
    return text_[index];
}

Bool Str::isEmpty() const
{
    return !length(); // this is virtual
}

int Str::length() const
{
    return byteLength_;
}

Str::operator char*() const
{
    if (!text_)
        const_cast<Str*>(this) -> empty();
    return text_;
}

void Str::empty()
{
    byteLength_ = 0;
    remove_();
    *(text_ = claimMemory(1)) = 0;
}

int Str::compare(const char* otherChars) const
{
    int cmp = strcmp(operator char*(), otherChars);
    if (!cmp)
	return 0;
    else return (cmp > 0 ? 1 : -1);
}

int Str::compare(const Str& other) const
{
    return compare((const char*) other);
}

Bool Str::operator== (const Str& other) const
{
    return (Bool) !strcmp(operator char*(), (char*) other);
}

Bool Str::operator== (const char* otherChars) const
{
    return (Bool) !strcmp(operator char*(), otherChars);
}

Bool Str::operator== (char* otherChars) const
{
    return (Bool) !strcmp(operator char*(), otherChars);
}

Bool Str::eqNoCase (const Str& other) const
{
    return (strEqNoCase(operator char*(), (char*) other) ? TRUE : FALSE);
}

Bool Str::eqNoCase (const char* other) const
{
    return (strEqNoCase(operator char*(), other) ? TRUE : FALSE);
}

Bool Str::operator< (const char* other)
{
    return compare(other) < 0;
}

Bool Str::operator< (const Str &other)
{
  return operator<((const char*) other);
}

Bool Str::toDouble(double &d) const
{
    char *stopper;
    d = strtod(operator char*(),&stopper);
    return (!!*stopper);
}

Str::~Str()
{
    remove_();
}

DStr& Str::appendSelf(DStr& other)
{
    other.nadd(operator char*(), length());
    remove_();
    return other;
}

DStr Str::operator+ (const Str& other) const
{
    pack_();
    other.pack_();
    DStr temp(*this);
    temp += other;
    return temp;
}

DStr Str::operator+ (const char* otherChars) const
{
    pack_();
    DStr temp(*this);
    temp += otherChars;
    return temp;
}

DStr Str::operator+ (int num) const
{
    pack_();
    DStr temp(*this);
    temp += num;
    return temp;
}


// pack_() is overridden in DStr to put the pieces of the dynamic
// string together
//
void Str::pack_() const
{
    if (!text_)
        const_cast<Str*>(this) -> empty();
};


// a ridiculous old function (almost dead)
// FIXME: do away with it and namechar() too
//
void Str::speakTerse(DStr &ret)
{
    int i;
    char c;
    pack_();
    for (i = 0; i < length(); i++)
    {
        switch(c = (*this)[i])
        {
        case '\n': 
            ret += "&#10;"; break;
        case '\t': 
            ret += "&#9;"; break;
        default:
            ret += c;
        };
    };
}

char* Str::cloneData()
{
  pack_();
  char *ret = new char[length() + 1];
  if 
    (text_) strcpy(ret, text_);
  else 
    *ret = 0;
  return ret;
}


//
//
//                  DStr
//                  dynamic string
//
//


DStr::DStr()
{
};

DStr::DStr(char c)
{
    // was: text = NULL 
    // which leaks memory since Str::Str() is called
    returnMemory(text_);
    operator+=(c);
};
DStr::DStr(const char *chars)
{
    // was: text = NULL 
    // which leaks memory since Str::Str() is called
    returnMemory(text_);
    operator+=(chars);
};

DStr::DStr(const Str& string)
{
    // was: text = NULL 
    // which leaks memory since Str::Str() is called
    returnMemory(text_);
    operator+=(string);
};

DStr::DStr(const DStr& dstring)
  : Str()
{
    // was: text = NULL 
    // which leaks memory since Str::Str() is called
    returnMemory(text_);
    operator+=(dstring);
};

DStr& DStr::operator=(const DStr& other)
{
    remove_();
    operator+=(other);
    return *this;
}


DStr& DStr::nadd(const char *adding, int len)
{
    sabassert(adding);
    if (text_)
        blocks.nadd(adding, len);
    else
        nset(adding,len);
    return *this;
}


DStr& DStr::operator+= (const char *adding)
{
    if (!text_ || *adding)
        nadd(adding, strlen(adding));
    return *this;
}

DStr& DStr::operator+= (const Str& addingStr)
{
    nadd((char*) addingStr, addingStr.length());
    return *this;
}

DStr& DStr::operator+=(const DStr& other)
{
    if (!other.text_) return *this;
    nadd(other.text_, other.byteLength_);
    DynBlockItem *b;
    for (b = other.blocks.first; b; b = b -> next)
        nadd(b -> data, b -> byteCount);
    return *this;
}

DStr& DStr::operator+= (int addingnum)
{
    Str temp = addingnum;
    return operator +=(temp);
}

DStr& DStr::operator+= (char addingc)
{
    Str temp = addingc;
    return operator +=(temp);
}

DStr& DStr::appendSelf(DStr& other)
{
    other.nadd(text_, byteLength_);
    DynBlockItem *b;
    for (b = blocks.first; b; b = b -> next)
        other.nadd(b -> data, b -> byteCount);
    remove_();
    byteLength_ = 0;
    return other;
}

DStr::~DStr()
{
    remove_();
}

void DStr::remove_()
{
    returnMemory(text_);
    blocks.remove();   
}

void DStr::pack_() const
{
    // if not needed leave asap
    if (blocks.isEmpty()) return;
    int blocksLen = blocks.byteCount;
    // concat text_ with the contents of all blocks
    char *oldText_ = const_cast<DStr*>(this) -> text_;
    const_cast<DStr*>(this) -> text_ =
        const_cast<DynStrBlock*>(&blocks) -> 
        compactString_(text_, byteLength_);
    returnMemory(oldText_);
    const_cast<DStr*>(this) -> byteLength_ += blocksLen;
    // the length stays the same
}

int DStr::length() const
{
    return byteLength_ + blocks.byteCount; 
}

DStr::operator char* () const
{
    pack_();
    return Str::operator char*();
}



//
//
//  string functions
//
//

void escapeChars(DStr& result, const Str& what, 
                 const char* toEscape, const char** substitutes)
{
    char *pStart = what, *p = pStart;
    int chunkLength;
    while (pStart)
    {
        p = strpbrk(pStart, toEscape);
        if (p)
        {
            if ((chunkLength = (int)(p - pStart)))
                result.nadd(pStart, chunkLength);
            result += substitutes[(int) (NZ(strchr(toEscape, *p)) - toEscape)];
            pStart = ++p;
        }
        else 
        {
            result += pStart;
            pStart = NULL;
        }
    }
}


//
//
//                  QName
//                  holds a qname as in the XML spec
//
//

QName::QName()
{
    local = UNDEF_PHRASE;
    uri = UNDEF_PHRASE;
    prefix = UNDEF_PHRASE;
}

QName::QName(const QName &other)
{
    local = other.local;
    uri = other.uri;
    prefix = other.prefix;
}

QName& QName::operator =(const QName& other)
{
    local = other.local;
    uri = other.uri;
    prefix = other.prefix;
    return *this;
}

Bool QName::isEmpty() const
{
    return (uri == UNDEF_PHRASE && local == UNDEF_PHRASE);
};

void QName::empty()
{
    prefix = UNDEF_PHRASE;
    local = UNDEF_PHRASE;
    uri = UNDEF_PHRASE;
}

/*
void QName::speak(DStr &s, SpeakMode mode) const
{
    if (prefix != UNDEF_PHRASE)
    {
        s += dict().getKey(prefix);
        s += ":";
    };
    s += dict().getKey(local);
}
*/

/*
Bool QName::prefixIsNS(const Str &uri2) const
{
    Phrase uri2Id;
    dict().insert(uri2, uri2Id);
    return (uri == uri2Id);
} 
*/

void QName::setPrefix(Phrase prefix_)
{
    prefix = prefix_;
//    dict().insert(pref, prefix);
}


void QName::setLocal(Phrase local_)
{
    local = local_;
//    dict().insert(loc, local);
}

void QName::setUri(Phrase uri_)
{
    uri = uri_;
//    dict().insert(u, uri);
}


Bool QName::operator== (const QName &other) const
{
    return local == other.local && uri == other.uri;
}

/*
Str QName::getname() const
{
    DStr s;
    speak(s,SM_NAME);
    return s;
}
*/

Phrase QName::getLocal() const
{
    return local;
//  return dict().getKey(local);
}

Phrase QName::getUri() const
{
    return uri;
//  return dict().getKey(uri);
}

Phrase QName::getPrefix() const
{
    return prefix;
//  return dict().getKey(prefix);
}

Bool QName::hasPrefix() const
{
   return (prefix != UNDEF_PHRASE);
}


//
//
//  QNameList
//
//

int QNameList::findNdx(const QName &what) const
{
    int count = number();
    for (int i = 0; i < count; i++)
		// checking for equality of local and uri
		if ((*this)[i] -> operator==(what))
		    return i;
    return -1;
}

//
//    EQName
//

EQName::EQName()
{
};

EQName::EQName(const EQName& other)
{
    prefix = other.getPrefix();
    uri = other.getUri();
    local = other.getLocal();
};

Bool EQName::isEmpty() const
{
    return prefix.isEmpty() && uri.isEmpty() && local.isEmpty();
}

void EQName::empty()
{
    prefix.empty();
    uri.empty();
    local.empty();	    
}

void EQName::set(const QName& q, const HashTable& dict)
{
    prefix = dict.getKey(q.getPrefix());
    uri = dict.getKey(q.getUri());
    local = dict.getKey(q.getLocal());
}

void EQName::setPrefix(const Str& prefix_)
{
    prefix = prefix_;
}

void EQName::setLocal(const Str& local_)
{
    local = local_;
}

void EQName::setUri(const Str& uri_)
{
    uri = uri_;
}

void EQName::getname(Str& fullName) const
{
    DStr strg;
    if (!prefix.isEmpty())
    {
        strg += prefix;
        strg += ":";
    };
    strg += local;
    fullName = strg;
}

const Str& EQName::getLocal() const
{
    return local;
}

const Str& EQName::getUri() const
{
    return uri;
}

const Str& EQName::getPrefix() const
{
    return prefix;
}

Bool EQName::hasPrefix() const
{
    return !prefix.isEmpty();
}

Bool EQName::operator==(const EQName &other) const
{
    return (uri == other.getUri() 
        && local == other.getLocal() 
	    && uri == other.getUri());
}



//
//
//                  StrStrList
//
//



int StrStrList::findNum(const Str &key_) const
{
    int i, count = number();
    for (i = 0; (i < count) && !(key_ == ((*this)[i] -> key)); i++) {};
    return (i < count) ? i : -1;
}

Str* StrStrList::find(const Str &_key) const
{
    int ndx = findNum(_key);
    return (ndx != -1) ? &((*this)[ndx] -> value) : NULL;
}

void StrStrList::appendConstruct(const Str &key, const Str& value)
{
    StrStr* newItem = new StrStr;
    newItem -> key  = key;
    newItem -> value = value;
    append(newItem);
}

//
//
//                  NamespaceStack
//
//

int NamespaceStack::findNum(const Str &prefix_) const
{
    int i, count = number();
    for (i = count-1; (i >= 0) && !(prefix_ == ((*this)[i] -> prefix)); i--) {};
    return i;
}

const Str* NamespaceStack::getUri(const Str &_prefix) const
{
    int ndx = findNum(_prefix);
    return (ndx != -1) ? &((*this)[ndx] -> uri) : NULL;
}


Bool NamespaceStack::isHidden(const Str &_prefix) const
{
    int ndx = findNum(_prefix);
    return (ndx != -1) ? ((*this)[ndx] -> hidden) : TRUE;
}

void NamespaceStack::appendConstruct(const Str &prefix, const Str& uri,
				     Bool _hidden)
{
    NamespaceStackObj* newItem = new NamespaceStackObj;
    newItem -> prefix  = prefix;
    newItem -> uri = uri;
    newItem -> hidden = _hidden;
    append(newItem);
}


//
//
//                  EQNameList
//
//


const EQName* EQNameList::find(const EQName &what) const
{
    int i, count = number();
    for (i = 0; (i < count) && !(what == (*operator[](i))); i++) {};
    return (i < count) ? operator[](i) : NULL;
}



//
//
//                  EQNameStrList
//
//

// returns -1 if the EQName was not found
int EQNameStrList::findNdx(const EQName &what) const
{
    int i, count = number();
    for (i = 0; i < count && !(what == operator[](i) -> key); i++) {};
    return (i < count) ? i : -1;
}

const Str* EQNameStrList::find(const EQName &what) const
{
    int i = findNdx(what);
    return (i != -1 ? &(operator[](i) -> value) : NULL);
}

void EQNameStrList::appendConstruct(const EQName &key, const Str& value)
{
    EQNameStr* newItem = new EQNameStr(key, value);
    append(newItem);
}



/************************************/
/* PrefixList */
int UriList::findNdx(Phrase what) {
  for (int i = 0; i < number(); i++) {
    Phrase foo = (*this)[i];
    if (foo == what) return i;
  }
  return -1;
}

void UriList::addUri(Phrase what) {
  int i = findNdx(what);
  if (i == -1) append(what);
}

/************************************/
/* SortedStringList */

int SortedStringList::compare(int i, int j, void * data)
{
  return ((*this)[i] -> compare(*((*this)[j])));
}


int SortedStringList::findIdx(Str & val)
{
  int i, j, cmp, result;
  i = 0; 
  j = nItems - 1;
  result = -1;

  while (i <= j) {
    int x = (int)((i + j) / 2);
    cmp = (*this)[x] -> compare(val);
    if (cmp < 0) {
      i = x + 1;
    } else if (cmp > 0) {
      j = x - 1;
    } else {
      result = x;
      break;
    }
  }

  return result;
}
