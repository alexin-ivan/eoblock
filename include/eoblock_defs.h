#ifndef EOBLOCK_DEFS_H_
#define EOBLOCK_DEFS_H_
/******************************************************************************/
#  define ATTR_PACKED __attribute__ ((__packed__))
#  define ATTR_UNUSED __attribute__((unused))
#  define ATTR_FORMAT(X__, Y__)  __attribute__ ((format (printf, (X__), (Y__))))
#  define ATTR_VFORMAT(X__, UNUSED__) __attribute__ ((format (printf, (X__), 0)))

/******************************************************************************/

#ifndef EOB_KERNEL
#   ifndef ARRAY_SIZE
#       define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#   endif
#endif
/******************************************************************************/
/*************************** ERRORS *******************************************/
typedef enum {
    EOB_OK = 0,
    EOB_NOMEM = 3000,
    EOB_NODEV,
    EOB_EALREADY,
    EOB_IO,
    EOB_CHKSUM,
    EOB_INVAL,
    EOB_VER_MISM,
    EOB_NALIGN,
    EOB_2BIG,
    EOB_BADUSAGE,
} eob_err_type;

typedef enum {
    EOB_ERR,
    EOB_DEBUG,
    EOB_INFO,
    EOB_CRIT,
    EOB_WARING,
} eob_log_lvl;

/******************************************************************************/
/*************************** KERNEL *******************************************/

#ifndef EOB_KERNEL
#define EOB_LOGGER_WRITE_LOG eoblock_logger_write_log
#define EOB_LOGGER_WRITE_LOG_CONSOLE eoblock_logger_write_log_console
#define eob_strcmp strcmp
#define eob_strlen strlen
#endif

/******************************************************************************/
/*************************** STRINGS ******************************************/
#define STR_EQ(_S1, _S2) (eob_strcmp(_S1, _S2) == 0)
#define STR_IS_EMPTY(_S) (S || (eob_strlen(_S) == 0))

/******************************************************************************/
/*************************** CASSERTIONS **************************************/

/** Compile time assert global */
#define CTASSERT_GLOBAL(x)      _CTASSERT(x, __LINE__)

/** Compile time assert inside code block */
#define CTASSERT(x)         { _CTASSERT(x, __LINE__); }

#define _CTASSERT(x, y)         __CTASSERT(x, y)
#define __CTASSERT(x, y) \
    typedef char ATTR_UNUSED __compile_time_assertion__ ## y[(x) ? 1 : -1]

/******************************************************************************/
/*************************** LOGGING ******************************************/

#ifndef EOB_KERNEL
/* to be forced to define MODULE */
/** MODULE used int logging as prefix. */
#    define MODULE error_MODULE_not_defined

#    define EOB_LOG(_PRIO, ...) EOB_LOGGER_WRITE_LOG(_PRIO, MODULE, __VA_ARGS__)

#    define EOB_MSG(_PRIO, ...) EOB_LOGGER_WRITE_LOG_CONSOLE(_PRIO, MODULE, __VA_ARGS__)

#else

#    define EOB_DEBUG KERN_DEBUG
#    define EOB_INFO  KERN_INFO
#    define EOB_ERR   KERN_ERR

#    define EOB_LOG(_PRIO, fmt, args...) printk(_PRIO "keob: " fmt "\n", ##args)
#    define EOB_MSG(__PRIO, fmt, args...) __undefined_function__

#define NOMEMORY_TRACE(__TAG) \
		printk(KERN_ERR "keob: Alloc failed (%s)\n", __TAG);

#endif
/******************************************************************************/
/*************************** ERROR HANDLING AND LOGGING ***********************/

#ifndef EOB_KERNEL
/* Must bes used only for release versions */
/* This macro disable error code tracing */
/* #define NTRACE_ERRORS */

#ifndef NTRACE_ERRORS
#   define SET_ERR_WITH_ERRNO(errval) \
        (EOB_LOG(EOB_ERR, "Sys_Error in '%s:%s:%d' with code '%d' (%s)", __FILE__, __func__, __LINE__, errno, strerror(errno)), errval)

#   define SET_EOB_ERR(code, __msg) \
        (EOB_LOG(EOB_ERR, "EOB_Error in '%s:%s:%d' with code '%d' (%s) and message: '%s'", __FILE__, __func__, __LINE__, code, eob_strerror(code), __msg), code)
#else
#endif

/** Trace eob error code. */
#define SET_ERR(code) SET_EOB_ERR(code, "")

#define SET_ERR_TRACE(__rv) SET_ERR(__rv)

#define SET_IO_ERR() SET_ERR_WITH_ERRNO(EOB_IO)

/** Trace and return out of memory error code */
#define no_memory_trace() SET_ERR(EOB_NOMEM)
#else

#define SET_CERR(__err) \
        (EOB_LOG(EOB_ERR, "Sys_Error in '%s:%s:%d' with code '%d'", __FILE__, __func__, __LINE__, (int) __err), (int) __err)

#define SET_CERRT(__err, FMT, T) \
        (EOB_LOG(EOB_ERR, "Sys_Error in '%s:%s:%d' with code '" FMT "'", __FILE__, __func__, __LINE__, T __err), T __err)

#define SET_CERRU(__err) SET_CERRT(__err, "%ul", (unsigned long))
#endif
/******************************************************************************/
/*************************** ASSERTIONS ***************************************/
#define EOB_ASSERT(x) ASSERT(x)
#define EOB_ASSERT_NOT_NULL(x)  EOB_ASSERT( (x) != NULL )
#define EOB_ASSERT_NULL(x)  EOB_ASSERT( (x) == NULL )
#define EOB_ASSERT_EQUAL(A, B) ASSERT3U( (A), ==, (B) )
#define EOB_ASSERT_NOT_EQUAL(A, B) EOB_ASSERT( (A) != (B) )
#define EOB_ASSERT_NO_LOG(x) assert(x)

/******************************************************************************/
/*************************** VERIFY *******************************************/
/** In debug versions 'verfiy' will be equal to assertions.
 * However, in release versions, 'verify' will be logged message and aborted programms.
 * Common difference between verify and assert is behaviour in NDEBUG condition:
 * verify always checks condition and abort() if conditions is false.
 * assert checks condition only if !NDEBUG.
 */
#define EOB_VERIFY(X) VERIFY(X)
#define EOB_VERIFY_EQUAL(A, B) VERIFY3U( (A), ==, (B) )
#define EOB_VERIFY_NOT_EQUAL(A, B) VERIFY3U( (A), !=, (B) )
#define EOB_VERIFY_NOT_EQUAL_S(A, B, __s) EOB_VERIFY( ((A) != (B)) && __s )
#define EOB_VERIFY_NOT_NULL(A) VERIFY3P( (A), !=, NULL )

#define TODO_EOB_VERIFY(EXPR) EOB_VERIFY(EXPR)

#define	EOB_VERIFY_IMPLY(A, B) EOB_VERIFY((!(A)) || (B))

/******************************************************************************/
/*************************** CODE SEQUENCE ************************************/

/** For code line, which can not be reached and must be implemented in further. */
#define NOT_IMPLEMENTED() EOB_VERIFY(0 && "Not implemented")

/** For code line, which can not be reached. */
#define UNREACHABLE_CODE() EOB_ASSERT(0 && "Unreacheble code")

/******************************************************************************/
/*************************** CC ATTRIBUTES ************************************/
#ifndef ATTR_CLEANUP
#define ATTR_CLEANUP(_cleanup_) __attribute__((cleanup( _cleanup_ )))
#endif

/******************************************************************************/
/* RAII */

#define with_raii(var_, _get, _put) \
    for(\
            var_ ATTR_CLEANUP(_put) = (_get) , *__it##__LINE__ = (void*) (intptr_t) 1;  \
            ((int) (intptr_t) __it##__LINE__) < 2; \
            __it##__LINE__ = (void*) (intptr_t) (((int) (intptr_t) __it##__LINE__) + 1) \
    )


/******************************************************************************/
/******************************************************************************/
/*
 * Compatibility macros/typedefs needed for Solaris -> Linux port
 */
#define P2ALIGN(x, align)	((x) & -(align))
#define P2CROSS(x, y, align)	(((x) ^ (y)) > (align) - 1)
#define P2ROUNDUP(x, align)	((((x) - 1) | ((align) - 1)) + 1)
#define P2PHASE(x, align)	((x) & ((align) - 1))
#define P2NPHASE(x, align)	(-(x) & ((align) - 1))
#define ISP2(x)			(((x) & ((x) - 1)) == 0)
#define IS_P2ALIGNED(v, a)	((((uintptr_t)(v)) & ((uintptr_t)(a) - 1))==0)
#define P2BOUNDARY(off, len, align) \
				(((off) ^ ((off) + (len) - 1)) > (align) - 1)

/*
 * Typed version of the P2* macros.  These macros should be used to ensure
 * that the result is correctly calculated based on the data type of (x),
 * which is passed in as the last argument, regardless of the data
 * type of the alignment.  For example, if (x) is of type uint64_t,
 * and we want to round it up to a page boundary using "PAGESIZE" as
 * the alignment, we can do either
 *
 * P2ROUNDUP(x, (uint64_t)PAGESIZE)
 * or
 * P2ROUNDUP_TYPED(x, PAGESIZE, uint64_t)
 */
#define P2ALIGN_TYPED(x, align, type)   \
        ((type)(x) & -(type)(align))
#define P2PHASE_TYPED(x, align, type)   \
        ((type)(x) & ((type)(align) - 1))
#define P2NPHASE_TYPED(x, align, type)  \
        (-(type)(x) & ((type)(align) - 1))
#define P2ROUNDUP_TYPED(x, align, type) \
        ((((type)(x) - 1) | ((type)(align) - 1)) + 1)
#define P2END_TYPED(x, align, type)     \
        (-(~(type)(x) & -(type)(align)))
#define P2PHASEUP_TYPED(x, align, phase, type)  \
        ((type)(phase) - (((type)(phase) - (type)(x)) & -(type)(align)))
#define P2CROSS_TYPED(x, y, align, type)        \
        (((type)(x) ^ (type)(y)) > (type)(align) - 1)
#define P2SAMEHIGHBIT_TYPED(x, y, type) \
        (((type)(x) ^ (type)(y)) < ((type)(x) & (type)(y)))


/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 */
#define eob_fls64(x) \
    (x ? (sizeof(x) * 8) - __builtin_clzll(x) : 0)

#define highbit64(x) (x ? eob_fls64(x) - 1 : 0)

/******************************************************************************/
/*************************** POST INCLUDES ************************************/



/******************************************************************************/
#endif /* EOBLOCK_DEFS_H_ */
