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

#include "arena.h"
#include "base.h"
#include "proc.h"

// GP: clean (no E())

SabArena::SabArena(int bloxize_)
{
    initialize(bloxize_);
}

void SabArena::initialize(int bloxize_)
{
    totalAsked = totalMalloced = 0;
    bloxize = bloxize_ & ~15;    // qword-align the ends of the blocks
    //    delaying: block0 = blockn = newBlock(bloxize);
    block0 = blockn = NULL;
}

ArenaBlock* SabArena::newBlock(int size)
{
    totalMalloced += size;
    ArenaBlock *b = new ArenaBlock;
    b -> next = NULL;
    b -> data = malloc(b -> freeSpace = size); // guaranteed to be well-aligned
    b -> blockSize = size;
    return b;
}

void SabArena::dispose()
{
    if (!block0) return;
    ArenaBlock *b, *b_was;
    for (b = block0; b; )
    {
        free(b -> data);
        b = (b_was = b) -> next;
        delete b_was;
    };
    // temporarily removing this to avoid the need for a Situation  
    // Log2(proc -> situation, L2_DISP_ARENA, Str(totalAsked), Str(totalMalloced));
    initialize(bloxize);
}

SabArena::~SabArena()
{
    // printf("Arena deleted --- asked %d malloced %d", totalAsked, totalMalloced);
    dispose();
}

void* SabArena::armalloc(int size, int alignment /* = sizeof(void*) */)
{
    totalAsked += size;
    int thisBloxize = bloxize;
    if (!block0)
        block0 = blockn = newBlock(bloxize);
    thisBloxize = blockn -> blockSize;
    if ((blockn -> freeSpace &= ~(alignment - 1)) < size) // align as necessary
    {
        if (size > bloxize)
            thisBloxize = size | 15 + 1;
        blockn = blockn -> next = newBlock(thisBloxize);
    }
    blockn -> freeSpace -= size;
    return ((char*)(blockn -> data)) + (thisBloxize - blockn -> freeSpace - size);
}

void SabArena::arfree(void *p)
{
}

