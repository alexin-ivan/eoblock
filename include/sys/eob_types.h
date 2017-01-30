#ifndef EOB_TYPES_H_
#define EOB_TYPES_H_
/******************************************************************************/
/******************************************************************************/
#ifndef EOB_KERNEL
#include <sys/types.h>
#include <stdint.h>
#else
#include <linux/types.h>
#endif


typedef ulong ulong_t;

typedef enum {
    B_TRUE,
    B_FALSE,
} boolean_t;
/******************************************************************************/
#endif /* EOB_TYPES_H_ */
