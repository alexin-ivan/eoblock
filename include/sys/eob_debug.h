#ifndef EOB_DEBUG_H_
#define EOB_DEBUG_H_
/******************************************************************************/
/******************************************************************************/
#include <eoblock_defs.h>

#ifndef EOB_KERNEL
#include <stdio.h>
#include <stdlib.h>
#endif

#ifndef EOB_KERNEL
static inline int
eob_assert_impl(const char *buf, const char *file, const char *func, int line)
{
    fprintf(stderr, "%s\n", buf);
    fprintf(stderr, "ASSERT at %s:%d:%s()", file, line, func);
    abort();
}

static inline int
eob_verify_impl(const char *buf, const char *file, const char *func, int line)
{
    fprintf(stderr, "%s\n", buf);
    fprintf(stderr, "VERIFY at %s:%d:%s()", file, line, func);
    abort();
}
#endif

#ifndef EOB_KERNEL
#define eob_assert(x) eob_assert_impl(x, #x)

#define VERIFY(cond)                            \
    (void) ((!(cond)) &&                        \
        eob_assert_impl(#cond, __FILE__, __FUNCTION__, __LINE__))

#define __VERIFY_BUF_LEN 256
#define VERIFY3_IMPL(LEFT, OP, RIGHT, TYPE)             \
do {                                    \
    const TYPE __left = (TYPE)(LEFT);               \
    const TYPE __right = (TYPE)(RIGHT);             \
    if (!(__left OP __right)) {                 \
        char *__buf = alloca(__VERIFY_BUF_LEN);              \
        (void) snprintf(__buf, __VERIFY_BUF_LEN,                \
            "%s %s %s (0x%llx %s 0x%llx)", #LEFT, #OP, #RIGHT,  \
            (unsigned long long)__left, #OP, (unsigned long long)__right);  \
        eob_assert_impl(__buf, __FILE__, __FUNCTION__, __LINE__);   \
    }                               \
} while (0)

#define VERIFY3S(x, y, z)   VERIFY3_IMPL(x, y, z, int64_t)
#define VERIFY3U(x, y, z)   VERIFY3_IMPL(x, y, z, uint64_t)
#define VERIFY3P(x, y, z)   VERIFY3_IMPL(x, y, z, uintptr_t)
#define VERIFY0(x)          VERIFY3_IMPL(x, ==, 0, uint64_t)


#else /* EOB_KERNEL */

#define	VERIFY3_IMPL(LEFT, OP, RIGHT, TYPE, FMT, CAST)			\
	(void)((!((TYPE)(LEFT) OP (TYPE)(RIGHT))) &&			\
	    eob_panic(__FILE__, __FUNCTION__, __LINE__,			\
	    "VERIFY3(" #LEFT " " #OP " " #RIGHT ") "			\
	    "failed (" FMT " " #OP " " FMT ")\n",			\
	    CAST (LEFT), CAST (RIGHT)))

#define	VERIFY(cond)							\
	(void)(unlikely(!(cond)) &&					\
	    eob_panic(__FILE__, __FUNCTION__, __LINE__,			\
	    "%s", "VERIFY(" #cond ") failed\n"))

int eob_panic(const char *file, const char *func, int line, const char *fmt, ...);


#define	VERIFY3S(x,y,z)	VERIFY3_IMPL(x, y, z, int64_t, "%lld", (long long))
#define	VERIFY3U(x,y,z)	VERIFY3_IMPL(x, y, z, uint64_t, "%llu",		\
				    (unsigned long long))
#define	VERIFY3P(x,y,z)	VERIFY3_IMPL(x, y, z, uintptr_t, "%p", (void *))
#define	VERIFY0(x)	VERIFY3_IMPL(0, ==, x, int64_t, "%lld",	(long long))

#endif




#ifdef NDEBUG
#define ASSERT(X) ((void *)0)
#define ASSERT3U(X, op, Y) ((void *) 0)
#define ASSERT3P(__X, op, __Y) ((void *) 0)
#else
#define ASSERT(X) VERIFY(X)
#define ASSERT3U(__X, __op, __Y) VERIFY3U(__X, __op, __Y)
#define ASSERT3P(__X, __op, __Y) VERIFY3P(__X, __op, __Y)
#endif


/******************************************************************************/
#endif /* EOB_DEBUG_H_ */
