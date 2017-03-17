/******************************************************************************/
/* XXX: for YCMe and CCode */
#ifdef __clang__

#ifndef EOB_KERNEL
#   define EOB_KERNEL
#endif

#ifndef __KERNEL__
#   define __KERNEL__
#endif

#ifndef CONFIG_BLOCK
#   define CONFIG_BLOCK
#endif

#ifndef CONFIG_X86_64
#   define CONFIG_X86_64
#endif

#ifndef CONFIG_FLAT_MEM
#   define CONFIG_FLATMEM
#endif

#ifndef CONFIG_X86_L1_CACHE_SHIFT
#define CONFIG_X86_L1_CACHE_SHIFT 6
#endif

/*typedef unsigned fmode_t;*/

//int eob_log_dummy(char prio, const char *fmt, ...);
//#define EOB_LOG(X, FMT, args...) eob_log_dummy(X, FMT, args)

int eob_log_init();
void eob_log_deinit();

typedef _Bool bool;

/*struct sockaddr {};*/

#endif
/******************************************************************************/
#include "eoblock_device.h"
#include "eoblock_sysfs.h"
#define EOB_MODULE_NAME "eob_main: "
#include "eoblock_debug.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/version.h>
#include <linux/rbtree.h>

#include <linux/percpu.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/genhd.h>
#include <linux/bio.h>
#include <linux/bug.h>

#include <linux/etherdevice.h>
#include <linux/netdevice.h>

#include <linux/ip.h>
#include <linux/tcp.h>

#include <eoblock_defs.h>
#include <sys/eob_log.h>
#include <eoblock.h>
#include <sys/eob_debug.h>

#define MAX_EOB_DEVICES 32

struct eob_ctx {
    struct eob_device *devices[MAX_EOB_DEVICES];
    spinlock_t lock;
};

/******************************************************************************/
static struct eob_ctx *eob_ctx = NULL;

char *df_dev_path = "/dev/sdcxxx";
module_param(df_dev_path, charp, 0644);

int df_node_id = 0;
module_param(df_node_id, int, 0);


/******************************************************************************/
int remove_eob_device(int node_id)
{
    struct eob_device *dev;
    dev = eob_ctx->devices[node_id];
    spin_lock(&eob_ctx->lock);
    remove_eob_device_impl(dev);
    spin_unlock(&eob_ctx->lock);
    return (0);
}

/******************************************************************************/
int add_eob_device(const char *path, int node_id)
{
    struct eob_device *dev;
    int rv;
    spin_lock(&eob_ctx->lock);
    rv = add_eob_device_impl(path, node_id, &dev);
    if(!rv)
        eob_ctx->devices[node_id] = dev;
    spin_unlock(&eob_ctx->lock);
    return (rv);
}

/******************************************************************************/
static int __init oeblock_init(void)
{
    int rv;

    eob_log_init();
    eob_ctx = kcalloc(1, sizeof(*eob_ctx), GFP_KERNEL);
    
    if(!eob_ctx)
        return SET_CERR(-ENOMEM);

    spin_lock_init(&eob_ctx->lock);

    EOB_PK(KERN_INFO, "%s", "Module loaded");

	rv = eoblock_sysfs_init();
    
	if (rv != 0)
        return rv;

    /*rv = add_eob_device(df_dev_path, df_node_id);*/
    
    return rv;
}

/******************************************************************************/
static void __exit oeblock_exit(void)
{
    /*remove_eob_device(df_node_id);*/
    eoblock_sysfs_cleanup();
    EOB_LOG(EOB_INFO, "%s", "Module unloaded");
    kfree(eob_ctx);
    eob_log_exit();
}

/******************************************************************************/
int eob_log_init(void)
{
    return (0);
}

/******************************************************************************/
void eob_log_exit(void)
{

}

/******************************************************************************/
module_init(oeblock_init);
module_exit(oeblock_exit);

MODULE_AUTHOR("Ivan Alechin");
MODULE_DESCRIPTION("Ethernet over block device");
MODULE_LICENSE("GPL");
