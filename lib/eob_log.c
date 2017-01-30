/******************************************************************************/
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/eob_log.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdlib.h>
#include <eoblock.h>
#include <errno.h>
#include <sys/eob_debug.h>

static int eob_log_verbose = 0;

/******************************************************************************/
int eob_log_init()
{
    openlog("eob", LOG_PID, LOG_LOCAL0);
    return (0);
}

void eob_log_exit()
{
    
}

void eob_log_set_verbose()
{
    eob_log_verbose = 1;
}

const char *eob_log_lvl_to_str(eob_log_lvl lvl)
{
    switch(lvl)
    {
        case EOB_CRIT:
            return "CRIT";
        case EOB_INFO:
            return "INFO";
        case EOB_DEBUG:
            return "DEBUG";
        case EOB_WARING:
            return "WARNING";
        case EOB_ERR:
            return "ERROR";
    }

    UNREACHABLE_CODE();

    return NULL;
}

/******************************************************************************/
static int v_eob_log_write_console(int lvl, const char *module, const char *fmt, va_list va)
{
    char *s;
    char *sn;
    int rv;
    
    rv = vasprintf(&s, fmt, va);
    if(rv <= 0)
        abort();

    rv = asprintf(&sn, "%s: %s%s", eob_log_lvl_to_str(lvl), module, s);
    if(rv <= 0)
        abort();
 
    if(eob_log_verbose)
        fprintf(stderr, "%s\n", sn);

    syslog(LOG_DEBUG, "%s", sn);

    free(sn);
    free(s);

    return (0);
}

int eob_log_write(int lvl, const char *module, const char *fmt, ...)
{
    int rv;
    va_list args;
    va_start(args, fmt);
    rv = v_eob_log_write_console(lvl, module, fmt, args);
    va_end(args);
    return rv;
}
int eob_log_write_console(int lvl, const char *module, const char *fmt, ...)
{
    int rv;
    va_list args;
    va_start(args, fmt);
    rv = v_eob_log_write_console(lvl, module, fmt, args);
    va_end(args);
    return rv;
}

/******************************************************************************/
const char *eob_strerror(int err)
{
    eob_err_type e = err;

    switch(e)
    {
        case EOB_OK:
            return "success";
        case EOB_NODEV:
            return "nodevice";
        case EOB_NOMEM:
            return "oom";
        case EOB_IO:
            return "io error";
        case EOB_INVAL:
            return "invalid argument";
        case EOB_EALREADY:
            return "already";
        case EOB_CHKSUM:
            return "checksumm mismatch";
        case EOB_VER_MISM:
            return "version mismatch";
        case EOB_NALIGN:
            return "wrong align";
        case EOB_2BIG:
            return "to big message";
        case EOB_BADUSAGE:
            return "invalid cmdline";
    }

    eob_log_write(EOB_ERR, "eob:log: ", "undefined error: %d", err);
    UNREACHABLE_CODE();
    return NULL;
}
