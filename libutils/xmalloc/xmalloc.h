/* xmalloc.h - How to allocate RAM without losing. */

#if !defined (_XMALLOC_H_)
#define _XMALLOC_H_

#if defined (__cplusplus)
extern "C"
{
#endif

extern void *xmalloc (unsigned int bytes);
extern void *xrealloc (void *pointer, unsigned int bytes);
extern void *xcalloc (unsigned int count, unsigned int bytes);

#if defined (__cplusplus)
}
#endif

#endif /* !_XMALLOC_H_ */
