/* object.h: -*- C -*-  */

/*  This file is part of <Meta-HTML>(tm), a system for the rapid
    deployment of Internet and Intranet applications via the use
    of the Meta-HTML language.

    Copyright (c) 1996, 2000, E. B. Gamble (ebg@metahtml.com).

    Meta-HTML is free software; you can redistribute it and/or modify
    it under the terms of the General Public License as published
    by the Free Software Foundation; either version 1, or (at your
    option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    FSF GPL for more details.

    You should have received a copy of the FSF General Public License
    along with this program; if you have not, you may obtain one
    electronically at the following URL:

	 http://www.metahtml.com/COPYING  */

/* Most everything here is temporary and will soon be integrated with the
   real meta-HTML type. */

#include "machine/machine.h"

#define MH_PAGE_DEFAULT_OBJECTS (16 * 4096)

static double
timeval_to_seconds (struct timeval *tm)
{
  return (tm->tv_sec + ((double) tm->tv_usec) / 1e6);
}

/****************************************************************************
 *
 *
 *
 *
 */
typedef struct mh_root *mh_root_t;

struct mh_root
{
  mh_root_t next;
  mh_object_t   *location;
};

#define MH_ROOT_NEXT( root )           ( (root)->next)
#define MH_ROOT_LOCATION( root )       ( (root)->location)
#define MH_ROOT_OBJECT( root )         (*(root)->location)

#define FOR_ROOTS( root, roots )		\
  for (root  = roots; 				\
       root != NULL; 				\
       root  = MH_ROOT_NEXT (root))

static mh_root_t
mh_root_alloc (void)
{
  return xcalloc (1, sizeof (struct mh_root));
}

static void
mh_root_free (mh_root_t root)
{
  memset (root, 0, sizeof (struct mh_root));
  xfree (root);
}

static mh_root_t
mh_root_make (mh_root_t next,
	      mh_object_t   *location)
{
  mh_root_t root =
    mh_root_alloc ();

  MH_ROOT_NEXT     (root) = next;
  MH_ROOT_LOCATION (root) = location;

  return root;
}

static inline boolean_t
mh_root_has_location_p (mh_root_t    root,
			mh_object_t *location)
{ return location == MH_ROOT_LOCATION (root); }

/****************************************************************************
 *
 *
 *
 *
 */
typedef struct mh_drum *mh_drum_t;

struct mh_drum
{
  mh_drum_t   next;
  mh_object_t object;
};

#define MH_DRUM_NEXT( drum )         ((drum)->next)
#define MH_DRUM_OBJECT( drum )       ((drum)->object)

#define FOR_DRUMS( drum, drums )		\
  for (drum  = drums;				\
       drum != NULL;				\
       drum  = MH_DRUM_NEXT (drum))

static mh_drum_t
mh_drum_alloc (void)
{
  return xcalloc (1, sizeof (struct mh_drum));
}

#if defined (NOT_USED)
static void
mh_drum_free (mh_drum_t drum)
{
  memset (drum, 0, sizeof (struct mh_drum));
  xfree (drum);
}
#endif

static mh_drum_t
mh_drum_make (mh_drum_t next,
	      mh_object_t     object)
{
  mh_drum_t drum =
    mh_drum_alloc ();

  MH_DRUM_NEXT   (drum) = next;
  MH_DRUM_OBJECT (drum) = object;

  return drum;
}

/****************************************************************************
 *
 *
 *
 *
 */
typedef struct mh_page *mh_page_t;

struct mh_page
{
  mh_page_t   next;
  mh_object_t bottom;
  mh_object_t limit;
  mh_object_t top;
};

#define MH_PAGE_NEXT( page )     ((page)->next)
#define MH_PAGE_BOTTOM( page )   ((page)->bottom)
#define MH_PAGE_LIMIT( page )    ((page)->limit)
#define MH_PAGE_TOP( page )      ((page)->top)

#define FOR_PAGES( page, pages )		\
  for (page = pages; 				\
       page != NULL; 				\
       page = MH_PAGE_NEXT (page))

#define FOR_PAGE_OBJECTS( object, size, page)	\
  for (object = page->bottom; 			\
       object < page->limit; 			\
       object += size)

static unsigned int
mh_page_count (mh_page_t page)
{
  unsigned int count = 0;
  FOR_PAGES (page, page)
    count++;
  return count;
}

static inline unsigned int
mh_page_dirty_size (mh_page_t page)
{ return page->limit - page->bottom; }

static inline unsigned int
mh_page_clean_size (mh_page_t page)
{ return page->top - page->limit; }

static inline unsigned int
mh_page_total_size (mh_page_t page)
{ return page->top - page->bottom; }

static mh_page_t
mh_page_alloc (mh_page_t    next,
	       unsigned int objects)
{
  mh_page_t page;

  objects = MAX (objects, MH_PAGE_DEFAULT_OBJECTS);
  
  page         = (mh_page_t)   xcalloc (1,       sizeof (struct mh_page));
  page->bottom = (mh_object_t) xcalloc (objects, sizeof (struct mh_object));

  page->limit  = page->bottom;
  page->top    = page->bottom + objects;
  page->next   = next;

  return page;
}

static inline void
mh_page_clean (mh_page_t page)
{
  memset ((void *) page->bottom,
	  0,
	  (page->top - page->bottom) * sizeof (struct mh_object));
  /* Critical */
  page->limit = page->bottom;

  /* Must not set PAGE->NEXT */
  return;
}

#if defined (NOT_USED)
static void
mh_page_free (mh_page_t page)
{
  mh_page_clean (page);
  xfree (page->bottom);

  memset ((void *) page, 0, sizeof (struct mh_page));
  xfree (page);
}
#endif

static mh_page_t
mh_page_append (mh_page_t pages,
		mh_page_t page)
{
  if (! pages)
    return page;
  else if (! page)
    return pages;
  else
    {
      mh_page_t head = pages;

      while (MH_PAGE_NEXT (pages))
	pages = MH_PAGE_NEXT (pages);
  
      MH_PAGE_NEXT (pages) = page;

      return head;
    }
}

/* Delete PAGE and only PAGE from PAGES.  The new PAGES is returned;
   PAGE itself is not freed. */
static mh_page_t
mh_page_delete (mh_page_t pages,
		mh_page_t page)
{
  if (!pages || !page)
    return pages;
  else if (page == pages)
    return MH_PAGE_NEXT (pages);
  else
    {
      mh_page_t head = pages;
      while (MH_PAGE_NEXT (pages))
	if (page == MH_PAGE_NEXT (pages))
	  {
	    MH_PAGE_NEXT (pages) =
	      MH_PAGE_NEXT (page);
	    break /* while */ ;
	  }
	else
	  pages = MH_PAGE_NEXT (pages);

      return head;
    }
}

/****************************************************************************
 *
 *
 *
 *
 */
typedef struct mh_heap
{
  mh_root_t roots;
  mh_drum_t drums;
  mh_page_t page;
  mh_page_t dirty_pages;
  mh_page_t clean_pages;

  /* GC state flags */
  int       gc_inhibited_p;
  boolean_t gc_pending_p;

  /* Number of GC performed */
  unsigned int   gc_count;

  /* Time spent on all GCs */
  struct timeval gc_time;

  /* Time spent on last GC */
  struct timeval gc_last_time;

  /* Total number of objects retained and discarded */
  unsigned long gc_retained_objects;
  unsigned long gc_discarded_objects;
  
} *mh_heap_t;

/* Forward Declaration */
static void
mh_heap_gc (mh_heap_t heap);

static void
mh_heap_summarize (mh_heap_t heap);

#define FOR_HEAP_OBJECTS( object, size, page, heap )	\
  FOR_PAGES (page, heap->dirty_pages)			\
    FOR_PAGE_OBJECTS (object, size, page)

static inline unsigned int
mh_heap_dirty_pages_count (mh_heap_t heap)
{ return mh_page_count (heap->dirty_pages); }

static inline unsigned int
mh_heap_clean_pages_count (mh_heap_t heap)
{ return mh_page_count (heap->clean_pages); }

static unsigned long
mh_heap_dirty_size (mh_heap_t heap)
{
  unsigned long count = 0;
  mh_page_t page;

  FOR_PAGES (page, heap->dirty_pages)
    count += (page == heap->page
	      ? mh_page_dirty_size (page)
	      : mh_page_total_size (page));

  return count;
}

static unsigned long
mh_heap_clean_size (mh_heap_t heap)
{
  unsigned long count = 0;
  mh_page_t page;

  FOR_PAGES (page, heap->clean_pages)
    count += mh_page_total_size (page);

  return count;
}

static inline struct timeval
mh_heap_gc_time (mh_heap_t heap)
{ return heap->gc_time; }

static inline struct timeval
mh_heap_gc_last_time (mh_heap_t heap)
{ return heap->gc_last_time; }

static struct timeval
mh_heap_gc_average_time (mh_heap_t heap)
{
  struct timeval result;
  timerclear (&result);

  if (heap->gc_count != 0)
    {
      result.tv_sec  = 
	(heap->gc_time.tv_sec  / heap->gc_count);

      result.tv_usec = 
	(heap->gc_time.tv_usec / heap->gc_count) +
	(heap->gc_time.tv_sec  % heap->gc_count) * 100000;
    }

  return result;
}

extern boolean_t
mh_heap_has_root_location_p (mh_heap_t    heap,
			     mh_object_t *location)
{
  mh_root_t root;
  FOR_ROOTS (root, heap->roots)
    if (mh_root_has_location_p (root, location))
      return true;
  return false;
}
     
static void
mh_heap_add_clean_pages (mh_heap_t heap,
			 mh_page_t pages)
{
  mh_page_t page;

  heap->clean_pages =
    (NULL == heap->clean_pages
     ? pages
     : mh_page_append (heap->clean_pages, pages));

  FOR_PAGES (page, pages)
    mh_page_clean (page);

  return;
}

static void
mh_heap_find_clean_page (mh_heap_t    heap,
			 unsigned int objects)
{
  mh_page_t page;

  /* Look through current CLEAN_PAGES */
  FOR_PAGES (page, heap->clean_pages)
    if (page->limit + objects < page->top)
      {
	/* Move from CLEAN to DIRTY; set PAGE */

	heap->clean_pages =
	  mh_page_delete (heap->clean_pages, page);

	page->next = NULL;

	heap->dirty_pages =
	  mh_page_append (heap->dirty_pages, page);

	heap->page = page;

	return;
      }

  /* Allocate a new PAGE for OBJECTS */
  heap->page = mh_page_alloc (NULL, objects);
  heap->dirty_pages =
    mh_page_append (heap->dirty_pages, heap->page);

  return;
}

static mh_page_t
mh_heap_find_page (mh_heap_t    heap,
		   unsigned int objects)
{
  mh_page_t page = heap->page;

  if (! heap->page)
    {
      mh_heap_find_clean_page (heap, 0);
      page = heap->page;
    }

  /* Find a page from which OBJECTS can be allocated */
  if (page->limit + objects < page->top)
    return page;

  /* PAGE is full do a GC */
  heap->gc_pending_p = true;  
  mh_heap_gc (heap);

  /* Try again for PAGE */
  page = heap->page;
  if (page->limit + objects < page->top)
    return page;
  
  /* Get a page from the CLEAN_PAGES */
  mh_heap_find_clean_page (heap, objects);

  /* If NULL, out of memory */
  return heap->page;
}

static mh_object_t
mh_heap_alloc (mh_heap_t    heap,
	       unsigned int objects)
{
  mh_page_t   page;
  mh_object_t object;

  if (heap->gc_pending_p && 0 == heap->gc_inhibited_p)
    mh_heap_gc (heap);

  /* Get this is be a properly aligned */
  objects = OBJECT_ALIGNMENT_SIZE (objects);

  /* Find the PAGE where OBJECTS can be allocated */
  page = mh_heap_find_page (heap, objects);

  /* Get the OBJECT itself */
  object = page->limit;

  /* Adjust the PAGE limit */
  page->limit += objects;

  return object;
}

static inline void
mh_object_to_broken_heart (mh_object_t old,
			   mh_object_t new)
{
  mh_broken_heart_t heart = MH_AS_BROKEN_HEART (old);

  heart->tag = MH_BROKEN_HEART_TAG;
  MH_BROKEN_HEART_OBJECT (heart) = new;
  return;
}

static mh_object_t
mh_heap_transport_move (mh_heap_t     heap,
			mh_object_t   object)
{
  /* Determine the SIZE of OBJECT - for us and others */
  unsigned int size = mh_object_size (object);

  /* Allocate a new object from HEAP */
  mh_object_t result = mh_heap_alloc (heap, size);

  /* Blindly fill RESULT with OBJECT */ 
  memcpy ((void *) result,
	  (const void *) object,
	  size * sizeof (mh_object_t));

  /* Change OBJECT into a BROKEN_HEART for future redirection. */
  mh_object_to_broken_heart (object, result);

  return result;
}

static inline mh_object_t
mh_heap_transport (mh_heap_t     heap,
		   mh_object_t   object)
{
  return MH_BROKEN_HEART_P (object)
    ? MH_BROKEN_HEART_OBJECT (MH_AS_BROKEN_HEART (object))
    : mh_heap_transport_move (heap, object);
}

static inline void
mh_heap_scan (mh_heap_t     heap, 
	      mh_object_t   object,
	      unsigned int *size)
{
  *size = OBJECT_ALIGNMENT_SIZE (mh_object_size (object));
  
  /* Scan OBJECT based on its type and transport its components */
  switch (MH_OBJECT_TAG (object))
    {
    case MH_STRING_TAG:
    case MH_NUMBER_TAG:
      /* Nothing */
      break;

    case MH_FUNCTION_TAG:
      {
	mh_tag_t tag = MH_AS_TAG (object);

	MH_TAG_CONSTANTS_VECTOR (tag) =
	  mh_heap_transport (heap, MH_TAG_CONSTANTS_VECTOR (tag));
      }
      break;

    case MH_VECTOR_TAG:
      {
	mh_vector_t vector    = MH_AS_VECTOR (object);
	unsigned int count    = 0;
	unsigned int length   = MH_VECTOR_LENGTH (vector);
	mh_object_t *objects  = MH_VECTOR_VALUES (vector);

	for (; count < length; count++)
	  objects[count] =
	    mh_heap_transport (heap, objects[count]);
      }
      break;

    case MH_NIL_TAG:
      /* Nothing */
      break;

    case MH_ALIST_TAG:
      {
	mh_alist_t alist = MH_AS_ALIST (object);

	MH_ALIST_NAME (alist) = 
	  mh_heap_transport (heap, MH_ALIST_NAME  (alist));
	MH_ALIST_VALUE (alist) = 
	  mh_heap_transport (heap, MH_ALIST_VALUE (alist));
	MH_ALIST_TAIL  (alist) = 
	  MH_AS_ALIST (mh_heap_transport
		       (heap, 
			MH_AS_OBJECT (MH_ALIST_TAIL  (alist))));
      }
      break;
      
    default:
      break;
    }
}

static void
mh_object_clean (mh_object_t object)
{
}

static void
mh_heap_gc (mh_heap_t heap)
{
  mh_root_t root;
  mh_drum_t drum;
  mh_page_t page;
  mh_page_t dirty_pages;
  unsigned int size;
  mh_object_t  object;

  struct timeval  begin_time, end_time;

  unsigned long initial_dirty_size;
  unsigned long final_dirty_size;

  if (0 != heap->gc_inhibited_p)
    return;

#if 0
  /* */
  printf ("\nGC Start\n");
  mh_heap_summarize (heap);
#endif

  /* Surely inhibit GC */
  heap->gc_inhibited_p++;

  /* Gather some statistics */
  gettimeofday (&begin_time, NULL);
  initial_dirty_size = mh_heap_dirty_size (heap);

  /* Setup 'FROM and TO' spaces */

  /* 1: Keep a pointer to the current DIRTY_PAGES.  These will become
     clean pages once all objects have been scanned */
  /* Set up FROM/TO spaces */
  dirty_pages = heap->dirty_pages;

  /* 2: Empty the HEAP's dirty_pages and page pointer.  This is a
     setup for finding a new clean page. */
  heap->page = heap->dirty_pages = NULL;

  /* 3: Get a clean page - either from CLEAN_PAGES or newly allocated */
  mh_heap_find_clean_page (heap, 0);


  /* Scan roots - allow, temporarily, for NULL roots */
  FOR_ROOTS (root, heap->roots)
    if (MH_ROOT_OBJECT (root) > (mh_object_t) 0x00001000)
      MH_ROOT_OBJECT (root) =
	mh_heap_transport (heap, MH_ROOT_OBJECT (root));

  /* SCAN FROM pages - where ROOTS were just transported */
  FOR_HEAP_OBJECTS (object, size, page, heap)
    mh_heap_scan (heap, object, &size);

  /* Scan drums. */
  FOR_DRUMS (drum, heap->drums)
    /* Any drum that is not a broken heart needs to be cleaned. */ 
    if (! MH_BROKEN_HEART_P (drum->object))
      mh_object_clean (drum->object);

  /* All DIRTY_PAGES are now CLEAN_PAGES */
  mh_heap_add_clean_pages (heap, dirty_pages);

  final_dirty_size = mh_heap_dirty_size (heap);
  heap->gc_retained_objects  += final_dirty_size;
  heap->gc_discarded_objects += 
    (initial_dirty_size - final_dirty_size);

  gettimeofday (&end_time, NULL);
  timersub (&end_time, &begin_time, &heap->gc_last_time);
  timeradd (&heap->gc_last_time,
	    &heap->gc_time,
	    &heap->gc_time);

  heap->gc_pending_p = false;
  heap->gc_inhibited_p--;
  heap->gc_count++;

#if 0
  /* */
  printf ("\nGC End\n");
  mh_heap_summarize (heap);
#endif

  printf ("GC: %f (msec)\n", 
	  1000 * timeval_to_seconds (&heap->gc_last_time));
}

static void
mh_heap_type_data (mh_heap_t     heap,
		   mh_type_t     type,
		   unsigned long long *count,
		   unsigned long long *memory)
{
  mh_page_t   page;
  mh_object_t object;
  unsigned int size;

  *count  = 0;
  *memory = 0;
  FOR_HEAP_OBJECTS (object, size, page, heap)
    {
      size = OBJECT_ALIGNMENT_SIZE (mh_object_size (object));
      if (type == MH_OBJECT_TAG (object))
	{
	  *count  += 1;
	  *memory += size;
	}
    }
}

static void
mh_heap_summarize (mh_heap_t heap)
{
  mh_root_t root;

  struct timeval gc_ave_time = mh_heap_gc_average_time (heap);

  unsigned long long data [10];

  mh_heap_type_data (heap, MH_STRING_TAG,   &data[0], &data[1]);
  mh_heap_type_data (heap, MH_NUMBER_TAG,   &data[2], &data[3]);
  mh_heap_type_data (heap, MH_VECTOR_TAG,   &data[4], &data[5]);
  mh_heap_type_data (heap, MH_ALIST_TAG,    &data[6], &data[7]);
  mh_heap_type_data (heap, MH_FUNCTION_TAG, &data[8], &data[9]);

  printf ("\n\
HEAP: %p\n\
  Dirty Pages\n\
    Count: %u\n\
    Allocated: %lu (objs)\n\
  Clean Pages\n\
    Count: %u\n\
    Available: %lu (objs)\n\
  Collections\n\
    Pending: %s\n\
    Enabled: %s (%d)\n\
    Count  : %d\n\
    Total   Time: %f (sec)\n\
    Average Time: %f (sec)\n\
    Objects Retained : %6lu (average)\n\
    Objects Discarded: %6lu (average)\n\
  Object Types:\n\
    String: %6Lu, %10Lu\n\
    Number: %6Lu, %10Lu\n\
    Vector: %6Lu, %10Lu\n\
    Alist : %6Lu, %10Lu\n\
    Tag   : %6Lu, %10Lu\n\
  Roots",
	  heap,
	  mh_heap_dirty_pages_count (heap),
	  mh_heap_dirty_size (heap),
	  mh_heap_clean_pages_count (heap),
	  mh_heap_clean_size (heap),
	  (heap->gc_pending_p   ? "yes" : "no"),
	  (0 == heap->gc_inhibited_p ? "yes" : "no"),
	  heap->gc_inhibited_p,
	  heap->gc_count,
	  timeval_to_seconds (&heap->gc_time),
	  timeval_to_seconds (&gc_ave_time),
	  (heap->gc_count
	   ? (heap->gc_retained_objects / heap->gc_count) 
	   : 0l),
	  (heap->gc_count
	   ? (heap->gc_discarded_objects / heap->gc_count) 
	   : 0l),
	  data[0], data[1],
	  data[2], data[3],
	  data[4], data[5],
	  data[6], data[7],
	  data[8], data[9]);
	  
  FOR_ROOTS (root, heap->roots)
    {
      mh_object_t *location = MH_ROOT_LOCATION (root);
#if 0
      printf ("\n    %#.8x: %#.8x ",
	      (unsigned int)location, (unsigned int)*location);
#endif
#if 1
      printf ("\n    %#.8x:  ", (unsigned int)location);
      if (*location > (mh_object_t) 0x08000000 &&
	  *location < (mh_object_t) 0x09000000)
	mh_object_to_file (*location, true, stdout);
      else if (*location)
	printf ("*********");
#endif
    }
  printf ("\n");
  /* Print a summary of the heap */

}

extern void
mh_heap_add_root (mh_heap_t    heap,
		  mh_object_t *location)
{
#if defined (MH_HEAP_CAREFUL_ROOTS)
  if (mh_heap_has_root_location_p (heap, location))
    printf ("\nADD_ROOT: exists @ %p\n", location);
#endif
  heap->roots =
    mh_root_make (heap->roots, location);

  return;
}

extern void
mh_heap_rem_root (mh_heap_t    heap,
		  mh_object_t *location)
{
  mh_root_t next;
  mh_root_t root = heap->roots;

  if (! root) return;

  if (location == MH_ROOT_LOCATION (root))
    {
      heap->roots = MH_ROOT_NEXT (root);
      mh_root_free (root);
      return;
    }

  FOR_ROOTS (root, heap->roots)
    if (NULL     != (next = MH_ROOT_NEXT (root)) &&
	location == MH_ROOT_LOCATION (next))
      {
	MH_ROOT_NEXT (root) = MH_ROOT_NEXT (next);
	mh_root_free (next);
#if defined (MH_HEAP_CAREFUL_ROOTS)
	if (mh_heap_has_root_location_p (heap, location))
	  printf ("\nREM_ROOT: remains @ %p\n", location);
#endif
	return;
      }
}

extern void
mh_heap_add_drum (mh_heap_t   heap,
		  mh_object_t object)
{
  heap->drums =
    mh_drum_make (heap->drums, object);

  return;
}

    
/************************************************************************
 *
 *
 *
 *
 */
static struct mh_heap mh_heap_record =
{
  NULL,				/* roots */
  NULL,				/* drums */
  NULL,				/* pages */
  NULL,				/* dirty_pages */
  NULL,				/* clean_pages */
  1,				/* gc_inhibited_p */
  false,			/* gc_pending_p */
  0,				/* gc_count */
  { 0, 0 },			/* gc_time */
  { 0, 0 },			/* gc_last_time */
  0L,				/* gc_transported_objects */
  0L				/* gc_discarded_objects */
};

static mh_heap_t mh_heap = & mh_heap_record;

extern void
mh_memory_gc (void)
{
  mh_heap_t heap = mh_heap;

  /* User doesn't get to call for a GC at arbitrary times */ 
  if (heap->gc_pending_p && 0 == heap->gc_inhibited_p)
    mh_heap_gc (heap);
}

extern void
mh_memory_gc_force (void)
{
  mh_heap_t heap = mh_heap;
  int inhibited  = heap->gc_inhibited_p;

  heap->gc_inhibited_p = 0;
  mh_heap_gc (heap);
  heap->gc_inhibited_p = inhibited;
}

extern void
mh_memory_gc_disable (void)
{
  mh_heap->gc_inhibited_p++;
}

extern void
mh_memory_gc_enable (void)
{
  if (mh_heap->gc_inhibited_p > 0)
    mh_heap->gc_inhibited_p--;
}

extern mh_object_t
mh_memory_alloc (unsigned int objects)
{
  return mh_heap_alloc (mh_heap, objects);
}

extern void
mh_memory_summarize (void)
{
  mh_heap_summarize (mh_heap);
}

extern void
mh_memory_add_root (mh_object_t *location)
{
  mh_heap_add_root (mh_heap, location);
}

extern void
mh_memory_rem_root (mh_object_t *location)
{
  mh_heap_rem_root (mh_heap, location);
}

extern void
mh_memory_add_drum (mh_object_t object)
{
  mh_heap_add_drum (mh_heap, object);
}


#if defined (HEAP_TEST)

extern int
main (int   argc,
      char *argv[])
{
  mh_object_t object;
  mh_vector_t vector;

  printf ("\
OBJECT_SIZE_IN_BYTES: %d\n\
OBJECT_ALIGNMENT_IN_BYTES: %d\n\
OBJECT_ALIGNMENT_IN_OBJECTS: %d\n\
OBJECT_ALIGNMENT_SIZE (1-5): %d, %d, %d, %d, %d\n\
OBJECT_SIZE (1-11/2): %d, %d, %d, %d, %d, %d\n\
OBJECT_ALIGNMENT_SIZE (bytes): %d, %d, %d, %d, %d, %d\n\
",
	  OBJECT_SIZE_IN_BYTES,
	  OBJECT_ALIGNMENT_IN_BYTES,
	  OBJECT_ALIGNMENT_IN_OBJECTS,
	  OBJECT_ALIGNMENT_SIZE (1),
	  OBJECT_ALIGNMENT_SIZE (2),
	  OBJECT_ALIGNMENT_SIZE (3),
	  OBJECT_ALIGNMENT_SIZE (4),
	  OBJECT_ALIGNMENT_SIZE (5),
	  OBJECT_SIZE (1),
	  OBJECT_SIZE (3),
	  OBJECT_SIZE (5),
	  OBJECT_SIZE (7),
	  OBJECT_SIZE (9),
	  OBJECT_SIZE (11),
	  OBJECT_ALIGNMENT_SIZE_FROM_BYTES (1),
	  OBJECT_ALIGNMENT_SIZE_FROM_BYTES (3),
	  OBJECT_ALIGNMENT_SIZE_FROM_BYTES (5),
	  OBJECT_ALIGNMENT_SIZE_FROM_BYTES (7),
	  OBJECT_ALIGNMENT_SIZE_FROM_BYTES (9),
	  OBJECT_ALIGNMENT_SIZE_FROM_BYTES (11));

  mh_object_init ();

  object = MH_AS_OBJECT
    (mh_cons_new (MH_AS_OBJECT (mh_string_buffer ("abc", 10)),
		  MH_AS_OBJECT (mh_number_new (1.2))));

  mh_number_new (1.2);

  vector = mh_vector_new (2);
  MH_VECTOR_REF (vector, 0) = object;
  MH_VECTOR_REF (vector, 1) = object;

  object = MH_AS_OBJECT (vector);
  mh_memory_add_root (&object);

  mh_memory_summarize ();

  mh_heap_gc (mh_heap);

  mh_memory_summarize ();

  mh_memory_gc_disable ();
  {
    int count = 1000;
    while (count--)
      {
	mh_string_buffer ("", (int) ((100 * random()) /	RAND_MAX));
	mh_number_new (random());
	mh_vector_new (100);
      }
  }

  mh_memory_summarize ();
  mh_memory_gc_enable ();
  mh_heap_gc (mh_heap);
  mh_memory_summarize ();
  printf ("Starting the second loop\n");
#if 1
  {
    int count = 10000;
    while (count--)
      {
	mh_string_buffer ("", (int) ((100 * random()) /	RAND_MAX));
	mh_number_new (random());
	mh_vector_new (100);
      }
  }
#endif
  mh_heap_gc (mh_heap);
  mh_memory_summarize ();

  return 0;
}

#endif /* defined (HEAP_TEST) */
