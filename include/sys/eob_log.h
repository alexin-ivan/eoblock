#ifndef EOB_LOG_H_
#define EOB_LOG_H_
/******************************************************************************/
#include <eoblock_defs.h>

/******************************************************************************/
int eob_log_init(void);
void eob_log_exit(void);

#ifndef EOB_KERNEL
    int eob_log_write(int lvl, const char *module, const char *fmt, ...)
        ATTR_FORMAT(3,4);

    int eob_log_write_console(int lvl, const char *module, const char *fmt, ...)
        ATTR_FORMAT(3,4);

#   define eoblock_logger_write_log eob_log_write
#   define eoblock_logger_write_log_console eob_log_write_console

    /******************************************************************************/
    const char *eob_strerror(int err);

    /******************************************************************************/
    void eob_log_set_verbose(void);
#endif


/******************************************************************************/
#endif /* EOB_LOG_H_ */
