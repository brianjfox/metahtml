/* Memory allocator `malloc'.
   Copyright 1990, 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.
		  Written May 1989 by Mike Haertel.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.

   The author may be reached (Email) at the address mike@ai.mit.edu,
   or (US mail) as Mike Haertel c/o Free Software Foundation.  */

#ifndef	_MALLOC_INTERNAL
#define _MALLOC_INTERNAL
#include "malloc.h"
#endif
#include <errno.h>

/* How to really get more memory.  */
__ptr_t (*__morecore) __P ((ptrdiff_t __size)) = __default_morecore;

/* Debugging hook for `malloc'.  */
__ptr_t (*__malloc_hook) __P ((__malloc_size_t __size));

/* Pointer to the base of the first block.  */
char *_heapbase;

/* Block information table.  Allocated with align/__free (not malloc/free).  */
malloc_info *_heapinfo;

/* Number of info entries.  */
static __malloc_size_t heapsize;

/* Search index in the info table.  */
__malloc_size_t _heapindex;

/* Limit of valid info table indices.  */
__malloc_size_t _heaplimit;

/* Free lists for each fragment size.  */
struct list _fraghead[BLOCKLOG];

/* Instrumentation.  */
__malloc_size_t _chunks_used;
__malloc_size_t _bytes_used;
__malloc_size_t _chunks_free;
__malloc_size_t _bytes_free;

/* Are you experienced?  */
int __malloc_initialized;

__malloc_size_t __malloc_extra_blocks;

void (*__malloc_initialize_hook) __P ((void));
void (*__after_morecore_hook) __P ((void));


/* Aligned allocation.  */
static __ptr_t align __P ((__malloc_size_t));
static __ptr_t
align (size)
     __malloc_size_t size;
{
  __ptr_t result;
  unsigned long int adj;

  result = (*__morecore) (size);
  adj = (unsigned long int) ((unsigned long int) ((char *) result -
						  (char *) NULL)) % BLOCKSIZE;
  if (adj != 0)
    {
      __ptr_t new;
      adj = BLOCKSIZE - adj;
      new = (*__morecore) (adj);
      result = (char *) result + adj;
    }

  if (__after_morecore_hook)
    (*__after_morecore_hook) ();

  return result;
}

/* Get SIZE bytes, if we can get them starting at END.
   Return the address of the space we got.
   If we cannot get space at END, fail and return -1.  */
static __ptr_t get_contiguous_space __P ((__malloc_ptrdiff_t, __ptr_t));
static __ptr_t
get_contiguous_space (size, position)
     __malloc_ptrdiff_t size;
     __ptr_t position;
{
  __ptr_t before;
  __ptr_t after;

  before = (*__morecore) (0);
  /* If we can tell in advance that the break is at the wrong place,
     fail now.  */
  if (before != position)
    return 0;

  /* Allocate SIZE bytes and get the address of them.  */
  after = (*__morecore) (size);
  if (!after)
    return 0;

  /* It was not contiguous--reject it.  */
  if (after != position)
    {
      (*__morecore) (- size);
      return 0;
    }

  return after;
}


/* This is called when `_heapinfo' and `heapsize' have just
   been set to describe a new info table.  Set up the table
   to describe itself and account for it in the statistics.  */
static void register_heapinfo __P ((void));
#ifdef __GNUC__
__inline__
#endif
static void
register_heapinfo ()
{
  __malloc_size_t block, blocks;

  block = BLOCK (_heapinfo);
  blocks = BLOCKIFY (heapsize * sizeof (malloc_info));

  /* Account for the _heapinfo block itself in the statistics.  */
  _bytes_used += blocks * BLOCKSIZE;
  ++_chunks_used;

  /* Describe the heapinfo block itself in the heapinfo.  */
  _heapinfo[block].busy.type = 0;
  _heapinfo[block].busy.info.size = blocks;
  /* Leave back-pointers for malloc_find_address.  */
  while (--blocks > 0)
    _heapinfo[block + blocks].busy.info.size = -blocks;
}

/* Set everything up and remember that we have.  */
int
__malloc_initialize ()
{
  if (__malloc_initialized)
    return 0;

  if (__malloc_initialize_hook)
    (*__malloc_initialize_hook) ();

  heapsize = HEAP / BLOCKSIZE;
  _heapinfo = (malloc_info *) align (heapsize * sizeof (malloc_info));
  if (_heapinfo == NULL)
    return 0;
  memset (_heapinfo, 0, heapsize * sizeof (malloc_info));
  _heapinfo[0].free.size = 0;
  _heapinfo[0].free.next = _heapinfo[0].free.prev = 0;
  _heapindex = 0;
  _heapbase = (char *) _heapinfo;
  _heaplimit = BLOCK (_heapbase + heapsize * sizeof (malloc_info));

  register_heapinfo ();

  __malloc_initialized = 1;
  return 1;
}

static int morecore_recursing;

/* Get neatly aligned memory, initializing or
   growing the heap info table as necessary. */
static __ptr_t morecore __P ((__malloc_size_t));
static __ptr_t
morecore (size)
     __malloc_size_t size;
{
  __ptr_t result;
  malloc_info *newinfo, *oldinfo;
  __malloc_size_t newsize;

  if (morecore_recursing)
    /* Avoid recursion.  The caller will know how to handle a null return.  */
    return NULL;

  result = align (size);
  if (result == NULL)
    return NULL;

  /* Check if we need to grow the info table.  */
  if ((__malloc_size_t) BLOCK ((char *) result + size) > heapsize)
    {
      /* Calculate the new _heapinfo table size.  We do not account for the
	 added blocks in the table itself, as we hope to place them in
	 existing free space, which is already covered by part of the
	 existing table.  */
      newsize = heapsize;
      do
	newsize *= 2;
      while ((__malloc_size_t) BLOCK ((char *) result + size) > newsize);

      /* First try to allocate the new info table in core we already have,
	 in the usual way using realloc.  If realloc cannot extend it in
	 place or relocate it to existing sufficient core, we will get
	 called again, and the code above will notice the
	 `morecore_recursing' flag and return null.  */
      {
	int save = errno;	/* Don't want to clobber errno with ENOMEM.  */
	morecore_recursing = 1;
	newinfo = (malloc_info *) _realloc_internal
	  (_heapinfo, newsize * sizeof (malloc_info));
	morecore_recursing = 0;
	if (newinfo == NULL)
	  errno = save;
	else
	  {
	    /* We found some space in core, and realloc has put the old
	       table's blocks on the free list.  Now zero the new part
	       of the table and install the new table location.  */
	    memset (&newinfo[heapsize], 0,
		    (newsize - heapsize) * sizeof (malloc_info));
	    _heapinfo = newinfo;
	    heapsize = newsize;
	    goto got_heap;
	  }
      }

      /* Allocate new space for the malloc info table.  */
      while (1)
  	{
 	  newinfo = (malloc_info *) align (newsize * sizeof (malloc_info));

 	  /* Did it fail?  */
 	  if (newinfo == NULL)
 	    {
 	      (*__morecore) (-size);
 	      return NULL;
 	    }

 	  /* Is it big enough to record status for its own space?
 	     If so, we win.  */
 	  if ((__malloc_size_t) BLOCK ((char *) newinfo
 				       + newsize * sizeof (malloc_info))
 	      < newsize)
 	    break;

 	  /* Must try again.  First give back most of what we just got.  */
 	  (*__morecore) (- newsize * sizeof (malloc_info));
 	  newsize *= 2;
  	}

      /* Copy the old table to the beginning of the new,
	 and zero the rest of the new table.  */
      memcpy (newinfo, _heapinfo, heapsize * sizeof (malloc_info));
      memset (&newinfo[heapsize], 0,
	      (newsize - heapsize) * sizeof (malloc_info));
      oldinfo = _heapinfo;
      _heapinfo = newinfo;
      heapsize = newsize;

      register_heapinfo ();

      /* Reset _heaplimit so _free_internal never decides
	 it can relocate or resize the info table.  */
      _heaplimit = 0;
      _free_internal (oldinfo);

      /* The new heap limit includes the new table just allocated.  */
      _heaplimit = BLOCK ((char *) newinfo + heapsize * sizeof (malloc_info));
      return result;
    }

 got_heap:
  _heaplimit = BLOCK ((char *) result + size);
  return result;
}

/* Allocate memory from the heap.  */
__ptr_t
_malloc_internal (size)
     __malloc_size_t size;
{
  __ptr_t result;
  __malloc_size_t block, blocks, lastblocks, start;
  register __malloc_size_t i;
  struct list *next;

  /* ANSI C allows `malloc (0)' to either return NULL, or to return a
     valid address you can realloc and free (though not dereference).

     It turns out that some extant code (sunrpc, at least Ultrix's version)
     expects `malloc (0)' to return non-NULL and breaks otherwise.
     Be compatible.  */

#if	0
  if (size == 0)
    return NULL;
#endif

  if (size < sizeof (struct list))
    size = sizeof (struct list);

#ifdef SUNOS_LOCALTIME_BUG
  if (size < 16)
    size = 16;
#endif

  /* Determine the allocation policy based on the request size.  */
  if (size <= BLOCKSIZE / 2)
    {
      /* Small allocation to receive a fragment of a block.
	 Determine the logarithm to base two of the fragment size. */
      register __malloc_size_t log = 1;
      --size;
      while ((size /= 2) != 0)
	++log;

      /* Look in the fragment lists for a
	 free fragment of the desired size. */
      next = _fraghead[log].next;
      if (next != NULL)
	{
	  /* There are free fragments of this size.
	     Pop a fragment out of the fragment list and return it.
	     Update the block's nfree and first counters. */
	  result = (__ptr_t) next;
	  next->prev->next = next->next;
	  if (next->next != NULL)
	    next->next->prev = next->prev;
	  block = BLOCK (result);
	  if (--_heapinfo[block].busy.info.frag.nfree != 0)
	    _heapinfo[block].busy.info.frag.first = (unsigned long int)
	      ((unsigned long int) ((char *) next->next - (char *) NULL)
	       % BLOCKSIZE) >> log;

	  /* Update the statistics.  */
	  ++_chunks_used;
	  _bytes_used += 1 << log;
	  --_chunks_free;
	  _bytes_free -= 1 << log;
	}
      else
	{
	  /* No free fragments of the desired size, so get a new block
	     and break it into fragments, returning the first.  */
	  result = malloc (BLOCKSIZE);
	  if (result == NULL)
	    return NULL;

	  /* Link all fragments but the first into the free list.  */
	  next = (struct list *) ((char *) result + (1 << log));
	  next->next = NULL;
	  next->prev = &_fraghead[log];
	  _fraghead[log].next = next;

	  for (i = 2; i < (__malloc_size_t) (BLOCKSIZE >> log); ++i)
	    {
	      next = (struct list *) ((char *) result + (i << log));
	      next->next = _fraghead[log].next;
	      next->prev = &_fraghead[log];
	      next->prev->next = next;
	      next->next->prev = next;
	    }

	  /* Initialize the nfree and first counters for this block.  */
	  block = BLOCK (result);
	  _heapinfo[block].busy.type = log;
	  _heapinfo[block].busy.info.frag.nfree = i - 1;
	  _heapinfo[block].busy.info.frag.first = i - 1;

	  _chunks_free += (BLOCKSIZE >> log) - 1;
	  _bytes_free += BLOCKSIZE - (1 << log);
	  _bytes_used -= BLOCKSIZE - (1 << log);
	}
    }
  else
    {
      /* Large allocation to receive one or more blocks.
	 Search the free list in a circle starting at the last place visited.
	 If we loop completely around without finding a large enough
	 space we will have to get more memory from the system.  */
      blocks = BLOCKIFY (size);
      start = block = _heapindex;
      while (_heapinfo[block].free.size < blocks)
	{
	  block = _heapinfo[block].free.next;
	  if (block == start)
	    {
	      /* Need to get more from the system.  Get a little extra.  */
	      __malloc_size_t wantblocks = blocks + __malloc_extra_blocks;
	      block = _heapinfo[0].free.prev;
	      lastblocks = _heapinfo[block].free.size;
	      /* Check to see if the new core will be contiguous with the
		 final free block; if so we don't need to get as much.  */
	      if (_heaplimit != 0 && block + lastblocks == _heaplimit &&
		  /* We can't do this if we will have to make the heap info
                     table bigger to accomodate the new space.  */
		  block + wantblocks <= heapsize &&
		  get_contiguous_space ((wantblocks - lastblocks) * BLOCKSIZE,
					ADDRESS (block + lastblocks)))
		{
 		  /* We got it contiguously.  Which block we are extending
		     (the `final free block' referred to above) might have
		     changed, if it got combined with a freed info table.  */
 		  block = _heapinfo[0].free.prev;
  		  _heapinfo[block].free.size += (wantblocks - lastblocks);
		  _bytes_free += (wantblocks - lastblocks) * BLOCKSIZE;
 		  _heaplimit += wantblocks - lastblocks;
		  continue;
		}
	      result = morecore (wantblocks * BLOCKSIZE);
	      if (result == NULL)
		return NULL;
	      block = BLOCK (result);
	      /* Put the new block at the end of the free list.  */
	      _heapinfo[block].free.size = wantblocks;
	      _heapinfo[block].free.prev = _heapinfo[0].free.prev;
	      _heapinfo[block].free.next = 0;
	      _heapinfo[0].free.prev = block;
	      _heapinfo[_heapinfo[block].free.prev].free.next = block;
	      ++_chunks_free;
	      /* Now loop to use some of that block for this allocation.  */
	    }
	}

      /* At this point we have found a suitable free list entry.
	 Figure out how to remove what we need from the list. */
      result = ADDRESS (block);
      if (_heapinfo[block].free.size > blocks)
	{
	  /* The block we found has a bit left over,
	     so relink the tail end back into the free list. */
	  _heapinfo[block + blocks].free.size
	    = _heapinfo[block].free.size - blocks;
	  _heapinfo[block + blocks].free.next
	    = _heapinfo[block].free.next;
	  _heapinfo[block + blocks].free.prev
	    = _heapinfo[block].free.prev;
	  _heapinfo[_heapinfo[block].free.prev].free.next
	    = _heapinfo[_heapinfo[block].free.next].free.prev
	    = _heapindex = block + blocks;
	}
      else
	{
	  /* The block exactly matches our requirements,
	     so just remove it from the list. */
	  _heapinfo[_heapinfo[block].free.next].free.prev
	    = _heapinfo[block].free.prev;
	  _heapinfo[_heapinfo[block].free.prev].free.next
	    = _heapindex = _heapinfo[block].free.next;
	  --_chunks_free;
	}

      _heapinfo[block].busy.type = 0;
      _heapinfo[block].busy.info.size = blocks;
      ++_chunks_used;
      _bytes_used += blocks * BLOCKSIZE;
      _bytes_free -= blocks * BLOCKSIZE;

      /* Mark all the blocks of the object just allocated except for the
	 first with a negative number so you can find the first block by
	 adding that adjustment.  */
      while (--blocks > 0)
	_heapinfo[block + blocks].busy.info.size = -blocks;
    }

  return result;
}

__ptr_t
malloc (size)
     __malloc_size_t size;
{
  if (!__malloc_initialized && !__malloc_initialize ())
    return NULL;

  return (__malloc_hook != NULL ? *__malloc_hook : _malloc_internal) (size);
}

#ifndef _LIBC

/* On some ANSI C systems, some libc functions call _malloc, _free
   and _realloc.  Make them use the GNU functions.  */

__ptr_t
_malloc (__malloc_size_t size)
{
  return malloc (size);
}

void
_free (__ptr_t ptr)
{
  free (ptr);
}

__ptr_t
_realloc (__ptr_t ptr, __malloc_size_t size)
{
  return realloc (ptr, size);
}

#endif
