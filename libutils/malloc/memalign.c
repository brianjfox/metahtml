/* Copyright (C) 1991, 1992, 1993, 1994, 1995 Free Software Foundation, Inc.

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
Cambridge, MA 02139, USA.  */

#ifndef	_MALLOC_INTERNAL
#define _MALLOC_INTERNAL
#include <malloc.h>
#endif

__ptr_t (*__memalign_hook) __P ((size_t __size, size_t __alignment));

__ptr_t
memalign (alignment, size)
     __malloc_size_t alignment;
     __malloc_size_t size;
{
  __ptr_t result;
  unsigned long int adj, lastadj;

  if (__memalign_hook)
    return (*__memalign_hook) (alignment, size);

  /* Allocate a block with enough extra space to pad the block with up to
     (ALIGNMENT - 1) bytes if necessary.  */
  result = malloc (size + alignment - 1);
  if (result == NULL)
    return NULL;

  /* Figure out how much we will need to pad this particular block
     to achieve the required alignment.  */
  adj = (unsigned long int) ((char *) result - (char *) NULL) % alignment;

  do
    {
      /* Reallocate the block with only as much excess as it needs.  */
      free (result);
      result = malloc (adj + size);
      if (result == NULL)	/* Impossible unless interrupted.  */
	return NULL;

      lastadj = adj;
      adj = (unsigned long int) ((char *) result - (char *) NULL) % alignment;
      /* It's conceivable we might have been so unlucky as to get a
	 different block with weaker alignment.  If so, this block is too
	 short to contain SIZE after alignment correction.  So we must
	 try again and get another block, slightly larger.  */
    } while (adj > lastadj);

  if (adj != 0)
    {
      /* Record this block in the list of aligned blocks, so that `free'
	 can identify the pointer it is passed, which will be in the middle
	 of an allocated block.  */

      struct alignlist *l;
      for (l = _aligned_blocks; l != NULL; l = l->next)
	if (l->aligned == NULL)
	  /* This slot is free.  Use it.  */
	  break;
      if (l == NULL)
	{
	  l = (struct alignlist *) malloc (sizeof (struct alignlist));
	  if (l == NULL)
	    {
	      free (result);
	      return NULL;
	    }
	  l->next = _aligned_blocks;
	  _aligned_blocks = l;
	}
      l->exact = result;
      result = l->aligned = (char *) result + alignment - adj;
    }

  return result;
}
