#ifndef EOBLOCK_DEBUG_H_
#define EOBLOCK_DEBUG_H_
/******************************************************************************/
/******************************************************************************/
int eoblock_printk(const char *prio, const char *module_name, const char *fmt, ...)
    __attribute__((format(printf, 3,4)));

int eoblock_klog(int prio, const char *module_name, const char *fmt, ...)
    __attribute__((format(printf, 3,4)));

/******************************************************************************/
#endif /* EOBLOCK_DEBUG_H_ */
