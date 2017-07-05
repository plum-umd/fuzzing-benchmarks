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
 *      Bob Jenkins <bob_jenkins@burtleburtle.net>
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

/*
 * based on hash function and code by Bob Jenkins, <bob_jenkins@burtleburtle.net>, 1996.
 */

#if !defined(HashHIncl)
#define HashHIncl

// GP: clean

#include "base.h"
#include "datastr.h"
#include "arena.h"

class Processor;
class SabArena;

//
//      HashItem
//      an item in the hash table. Contains part of the unique ID, the key and associated stuff (a void*).
//

class HashItem : public SabArenaMember
{
public:
    HashItem(const char *key_, oolong code_, const void *stuff_, int age_, SabArena *arena_)
        : key( (char*)key_, arena_ ), code( code_ ), stuff( stuff_ ), 
          age( age_ ), next( NULL )
    {};
    const SabArenaStr key;
    oolong code;
    const void *stuff;
    int age;
    HashItem *next;
};

//
//      HashItemList
//      used for the list of buckets
//

class HashItemList : public PList<HashItem*> 
{
    friend class HashTable;
    HashItemList( int logSize_ )
        : PList<HashItem*>( logSize_ )
    {}
};

//
//      HashTable
//      Simple hash table. It can grow, the items have a unique ID which allows quick lookup
//      of the key as well as the associated value.
//

class HashTable
{
public:
    // initialize the hash
    HashTable(SabArena *arena_, int logSize_);

     // destroy the hash
    ~HashTable();

    // insert an item, returning its id (if found as well as if not)
    void insert(const Str& key, HashId& id, const void *data = NULL );

    void insert(const char *key, HashId& id, const void *data = NULL )
    {
        insert(Str(key), id, data);
    };
    
    HashId insert(const Str& key);
    
    HashId insert(const char *key, const void *data = NULL )
    {
        return insert(Str(key));
    };
    


    // look up an item, returning ITEM_NOT_FOUND and data==NULL if not found
    HashId lookup(const Str& key, const void **data = NULL ) const;

    // return the key for a given id, asserting it exists
    const Str& getKey(HashId id) const;

    void initialize();
    void destroy(Sit S);

#ifdef _DEBUG
    void dump() const;
#endif

protected:
    oolong codeToIndex(oolong code) const;
    HashItem* expandWatching(oolong hashed);
    Bool lookupOrPreceding(const Str& key, oolong hashed, HashItem *&p) const;
    HashItemList buckets;   // the list of buckets
    SabArena *theArena;            // the arena to be used for hash items
    Processor *proc;
    int visitedBuckets,
        itemsCount,
        logSize;
    void report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2);
private:
    Str *_theEmptyKey; /* __PH__ */
};

#endif //HashHIncl
