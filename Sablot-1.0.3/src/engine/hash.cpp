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
 *      Tom Moog <tmoog@polhode.com>
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

#include "hash.h"
#include "proc.h"
#include "arena.h"

// GP: clean (only 1 M() macro)

// the unsigned long is split into two parts: a masked hash in the lower bits, and 
// age in the upper;
// HASH_CODE_BITS determines the max width of the hash code
#define HASH_CODE_BITS       24

#define hashMask( ID, BITS ) ( (ID) & ( ( 1L << (BITS) ) - 1 ) )
#define hashIdCode( ID ) hashMask( (ID), HASH_CODE_BITS ) 
#define hashIdAge( ID ) ( (ID) >> HASH_CODE_BITS )
#define hashIdCompose( CODE, AGE ) ( ( ((HashId) AGE) << HASH_CODE_BITS )\
            | hashMask( (CODE), HASH_CODE_BITS ) )

oolong hash(const Str& k);

//
//
//      HashTable
//
//

void HashTable::initialize()
{
    // initialize buckets to NULLs
    int i, bucketsCount = TWO_TO( logSize );
    for (i = 0; i < bucketsCount; i++)
        buckets.append( NULL );
    visitedBuckets = 0;
    itemsCount = 0;
}

HashTable::HashTable(SabArena *arena_, int logSize_)
: buckets( logSize_ ), theArena( arena_ ), logSize( logSize_ ), 
  _theEmptyKey(NULL)
{
    _theEmptyKey = new Str;
    //    initialize();
    visitedBuckets = 0;
    itemsCount = -1; // not initialized yet
}

void HashTable::destroy(Sit S)
{
    Log2(S, L2_DISP_HASH, Str(itemsCount), Str(buckets.number()));
    // only kill the HashItems
    if (!theArena)
    {
        HashItem *p, *old_p;
        for (int i = 0; i < buckets.number(); i++)
            for (p = buckets[i]; (old_p = p) != NULL; p = old_p -> next) 
                delete p;
    }
    else
        ;   // do nothing as the ListItems were allocated in an arena
    buckets.deppendall();
    (*this).HashTable::~HashTable();
}

HashTable::~HashTable()
{
    // destroy();
    cdelete(_theEmptyKey);
}

inline oolong HashTable::codeToIndex(oolong code) const
{ 
    return hashMask(code, logSize); 
}

//
//  for internal use only. Returns true if the desired item was found. Sets p
//  to this item. If not found then to its predecessor. If that's not found too then
//  to NULL.
//

Bool HashTable::lookupOrPreceding(const Str& key, oolong hashed, HashItem *&p) const
{
    sabassert(itemsCount != -1);     // assert hash table initialized
    for ( p = buckets[hashMask(hashed, logSize)]; p; p = p -> next )
    {
        if (p -> key == key)
            return TRUE;
        else if (!p -> next)
            return FALSE;
    }
    // empty bucket, p is NULL
    return FALSE;
}

void HashTable::insert(const Str& key, HashId& id, const void *data /* = NULL */)
{
    sabassert(itemsCount != -1);     // assert hash table initialized
    oolong hashed = hash( key );
    HashItem *p, *q;
    // try to find our item, or at least its predecessor
    if (!lookupOrPreceding( key, hashed, p ))
    {   
        // NOT FOUND. create and insert a new hash item
        // if buckets grow, need to update p
        if ( buckets.number() <= itemsCount )
            p = expandWatching( hashed );
        ++itemsCount;
        q = new( theArena )
            HashItem( key, hashed, data, p ? p -> age + 1 : 0, theArena );
	    // sorry, can't check for the allocation - no situation to report to
        //M( S, q );  // GP: OK (only fails if nothing to dealloc)
        if (p)
            p = p -> next = q;
        else
        {
            p = buckets[codeToIndex(hashed)] = q;
            ++visitedBuckets;
        };
    }
    id = hashIdCompose( hashed, p -> age );
}

HashId HashTable::insert(const Str& key)
{
    HashId id;
    insert(key, id, NULL);
    return id;
}

HashId HashTable::lookup(const Str& key, const void **data /* = NULL */) const
{
    sabassert(itemsCount != -1);     // assert hash table initialized
    oolong hashed = hash( key );
    HashItem *p;
    if (!lookupOrPreceding( key, hashed, p ))
    {
        if (data) *data = NULL;
        return ITEM_NOT_FOUND;
    }
    if (data) *data = p -> stuff;
    return hashIdCompose( hashed, p -> age );
}

#define chain( NEWPTR, TAIL, ROOT_NDX )\
{ if (TAIL) TAIL = TAIL -> next = NEWPTR;\
else { TAIL = buckets[ROOT_NDX] = NEWPTR; visitedBuckets++; }}

// this function fixed by Tom Moog

HashItem* HashTable::expandWatching( oolong hashed )
{
    sabassert(itemsCount != -1);     // assert hash table initialized
    oolong i, oldBucketCount = buckets.number();
    for (i = 0; i < oldBucketCount; i++)
        buckets.append(NULL);
    oolong distinguish = 1L << logSize;

    HashItem *p,
        *upperTail,
        *lowerTail,
        *retval = NULL;

    visitedBuckets = 0;

    for (i = 0; i < oldBucketCount; i++)    
    {
        upperTail = lowerTail = NULL;
        for (p = buckets[i]; p; p = p -> next)
        {
            if (p -> code & distinguish)
                chain(p, upperTail, i + oldBucketCount)
            else
                chain(p, lowerTail, i);
        };
        if (buckets[i])
            ++visitedBuckets;
        if (lowerTail)
            lowerTail -> next = NULL;
        else
            buckets[i] = NULL;
        if (upperTail)
            upperTail -> next = NULL;
        // do the watching
        if ( codeToIndex(hashed) == i )
            retval = (hashed & distinguish) ? upperTail : lowerTail;
    }
    
    ++logSize;  // can't do this before because codeToIndex() depends on it
    sabassert( logSize <= HASH_CODE_BITS );
    return retval;
}

const Str& HashTable::getKey(HashId id) const
{
  sabassert(itemsCount != -1);     // assert hash table initialized
  if (id == UNDEF_PHRASE) 
    {
      return *_theEmptyKey; /* __PH__ */
    }
  int age = hashIdAge(id);
  HashItem *p;
  for (p = buckets[codeToIndex(hashIdCode(id))]; 
       p && p -> age != age; p = p -> next);
  // DROPME
#ifdef _DEBUG
  if (!p)
    {
      dump();
      printf("\n HASH FAULT: id=%08lx ", id);
    }
#endif
    return NZ(p) -> key;
}

#ifdef _DEBUG
void HashTable::dump() const
    {
        printf("----- %d elements  %d buckets  %d visited -----\n", 
            itemsCount, buckets.number(), visitedBuckets);
        for (int i = 0; i < buckets.number(); i++)
        {
            if (buckets[i])
            {
                printf("[%04x] ", i);
                for (HashItem *j = buckets[i]; j; j = j -> next)
                {
                    printf("%s(%08x) ", (char*)(j -> key), j -> code );
                };
                puts("");
            }
        }
        puts("--------------------------------------------------\n\n");
    }
#endif

void HashTable::report(Sit S, MsgType type, MsgCode code, const Str& arg1, const Str& arg2)
{
    S.message(type, code, arg1, arg2);
}




//
//  the real internals
//

//
// 3-number mixer
//

#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

//
// the hash function
//

oolong hash(const Str& key)
{
   register oolong a, b, c, len;
   register const char *k = (const char*) key;

   /* Set up the internal state */
   len = key.length();
   a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
   c = 0;   // should have been either the previous hash value or any number :-)

   /*---------------------------------------- handle most of the key */
   while (len >= 12)
   {
      a += (k[0] +((oolong)k[1]<<8) +((oolong)k[2]<<16) +((oolong)k[3]<<24));
      b += (k[4] +((oolong)k[5]<<8) +((oolong)k[6]<<16) +((oolong)k[7]<<24));
      c += (k[8] +((oolong)k[9]<<8) +((oolong)k[10]<<16)+((oolong)k[11]<<24));
      mix(a,b,c);
      k += 12; len -= 12;
   }

   /*------------------------------------- handle the last 11 bytes */
   c += key.length();
   switch(len)              /* all the case statements fall through */
   {
   case 11: c+=((oolong)k[10]<<24);
   case 10: c+=((oolong)k[9]<<16);
   case 9 : c+=((oolong)k[8]<<8);
      /* the first byte of c is reserved for the length */
   case 8 : b+=((oolong)k[7]<<24);
   case 7 : b+=((oolong)k[6]<<16);
   case 6 : b+=((oolong)k[5]<<8);
   case 5 : b+=k[4];
   case 4 : a+=((oolong)k[3]<<24);
   case 3 : a+=((oolong)k[2]<<16);
   case 2 : a+=((oolong)k[1]<<8);
   case 1 : a+=k[0];
     /* case 0: nothing left to add */
   }
   mix(a,b,c);
   /*-------------------------------------------- report the result */
   return c;
}
