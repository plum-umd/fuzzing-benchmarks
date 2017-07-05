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

#ifndef ArenaHIncl
#define ArenaHIncl

// GP: clean

#include "base.h"

struct ArenaBlock
{
  void *data;
  ArenaBlock *next;
  int freeSpace;
  int blockSize;
};

class SabArena
{
public:
    SabArena(int bloxize_);
    /*size of by Tim Crook release 0.51 _PH_ */
    void* armalloc(int size, int alignment = sizeof (void*)); 
    void initialize(int bloxize_);
    void arfree(void *p);
    void dispose();
    ~SabArena();
private:
    ArenaBlock* newBlock(int size);
    int bloxize;
    ArenaBlock *block0, *blockn;
    int totalAsked, totalMalloced;
};

class SabArenaMember
{
public:
    /* VC++ 4.2 doesn't take the following:
    void operator delete(void *ptr, SabArena *arena_) {};
    so far, VC++6 throws warnings
    */

    void *operator new(size_t size, SabArena *arena_ = NULL)
    {
        // shouldn't be called without an arena specified
//        sabassert(arena_);
        return arena_? arena_ -> armalloc(size) : ::operator new(size);
    };

    void operator delete(void *ptr, size_t size)
    {
        // do nothing
        // ::operator delete(ptr);
    }
};

class SabArenaStr : public SabArenaMember, public Str
{
public:
    SabArenaStr(SabArena *arena__)
        : arena_(arena__) 
    {};
    SabArenaStr(Str& string, SabArena *arena__)
        : arena_(arena__)
    {
        operator =(string);
    };
/*
    SabArenaStr(int num, SabArena *arena__)
        : arena_(arena__)
    {
        operator =(num);
    };

    SabArenaStr(double num, SabArena *arena__)
        : arena_(arena__)
    {
        operator =(num);
    };

    SabArenaStr(char c, SabArena *arena__)
        : arena_(arena__)
    {
        operator =(c);
    };
*/
    SabArenaStr(char *chars, SabArena *arena__)
        : arena_(arena__)
    {
        operator =(chars);
    };
    SabArenaStr(DStr &dstring, SabArena *arena__)
        : arena_(arena__)
    {
        operator =(dstring);
    };

    const SabArenaStr& operator= (const Str& string)
    { nset((char*)string, string.length()); return *this; }

    const SabArenaStr& operator= (const DStr& dstring)
    { nset((char*)dstring, dstring.length()); return *this; }

    const SabArenaStr& operator= (const char *s)
    { nset(s, strlen(s)); return *this; }

    ~SabArenaStr()
    {
        remove_();
    }

protected:
    virtual char* claimMemory( int nbytes ) const 
    { 
        // ask for memory with alignment 1 if from arena
        return arena_ ? (char*)(arena_ -> armalloc(nbytes,1)) : Str::claimMemory(nbytes); 
    }
    virtual void returnMemory( char *&p ) const 
    {
        if (arena_)
            arena_ -> arfree(p);
        else
            if (p) delete[] p; 
        p = NULL; 
    }
    SabArena *arena_;
};

#endif // ArenaHIncl
