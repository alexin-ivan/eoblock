/******************************************************************************/
#include "eoblock_debug.h"
#include <stdarg.h>
#include <linux/kernel.h>
#include <eoblock_defs.h>
#include <linux/string.h>

/******************************************************************************/

/******************************************************************************/
static int eoblock_vklog(eob_log_lvl prio, const char *module_name, const char *fmt, va_list args)
{
    int rv;
    char buf[256];
    char v;
    const char *p = NULL;

    switch(prio)
    {
        case EOB_DEBUG:
            p = KERN_DEBUG; 
            break;
        case EOB_INFO:
            p = KERN_INFO;
            break;
        case EOB_ERR:
            p = KERN_ERR;
            break;
        case EOB_CRIT:
            p = KERN_CRIT;
            break;
        case EOB_WARNING:
            p = KERN_WARNING;
            break;
    }

    if (!p)
    {
        v = (char) prio;
        p = &v;
    }

    strcpy(buf, p);
    strcat(buf, module_name);
    strcat(buf, fmt);

    rv = vprintk(buf, args);

    return rv;
}

/******************************************************************************/
int eoblock_printk(const char *prio, const char *module_name, const char *fmt, ...)
{
    int rv;
    va_list args;
    va_start(args, fmt);
    rv = eoblock_vklog((int) prio[0], module_name, fmt, args);
    va_end(args);
    return rv;
}

/******************************************************************************/
int eoblock_klog(int prio, const char *module_name, const char *fmt, ...)
{
    int rv;
    va_list args;
    va_start(args, fmt);
    rv = eoblock_vklog(prio, module_name, fmt, args);
    va_end(args);
    return rv;
}

/******************************************************************************/
