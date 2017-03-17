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
#define EOB_MODULE_NAME "eob_device: "
#include "eoblock_debug.h"
#include <linux/socket.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/rbtree.h>
#include <linux/netdevice.h>

#include <linux/etherdevice.h>
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

#include <stddef.h>

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)) &&  (LINUX_VERSION_CODE > KERNEL_VERSION(3,16,0))
#define eob_alloc_netdev alloc_netdev
#elif (LINUX_VERSION_CODE > KERNEL_VERSION(3,19,0)) 
#define eob_alloc_netdev(sizeof_priv, name, setup) \
            alloc_netdev(sizeof_priv, name, NET_NAME_ENUM, setup)
#endif

#define K_ASSERT(X) 
#define TRACE_DBG(X, args...)

/******************************************************************************/
struct eob_device {
    struct net_device *ndev;
    char name[256];
    char path[PATH_MAX];
    struct eob_hw_addr addr;
    struct block_device *bdev;
    struct file *fd;
    uint64_t sectors;
    spinlock_t lock;
};

#if 0
struct eob_device {
    struct net_device *ndev;
    spinlock_t lock;
    char path[PATH_MAX];
    int is_blk;
    struct eob_hw_addr addr;
    struct block_device *bdev;       /* Associated blk device */
    uint64_t sectors;
};
#endif // 0
/******************************************************************************/
#define	VDEV_HOLDER			((void *)0x2401de8)
static void *dev_holder = VDEV_HOLDER;

/******************************************************************************/
int eob_panic(const char *file, const char *func, int line, const char *fmt, ...)
{
    BUG_ON(0);
    return (0);
}

/******************************************************************************/
static int eoblock_o_direct = 1;

/* Returns fd, use IS_ERR(fd) to get error status */
static int eob_open_fd(struct eob_device *dev)
{
	int open_flags = 0;
	struct file *fd;
    const char *name = dev->path;


	open_flags = O_RDWR | O_DSYNC;

	if (eoblock_o_direct)
		open_flags |= O_DIRECT;

	TRACE_DBG("Opening file %s, flags 0x%x", name, open_flags);

	fd = filp_open(name, O_LARGEFILE | open_flags, 0600);

	if (IS_ERR(fd)) {
		if (PTR_ERR(fd) == -EMEDIUMTYPE)
			EOB_LOG(EOB_ERR, "Unable to open %s with EMEDIUMTYPE, DRBD passive?", name);
		else
			EOB_LOG(EOB_ERR, "filp_open(%s) failed: %d", name, (int) PTR_ERR(fd));
        return PTR_ERR(fd);
	}

    dev->fd = fd;

	return (0);
}

__attribute__((unused))
static int eob_open_blk(struct eob_device *dev, const char *path)
{
    fmode_t mode = FMODE_READ | FMODE_WRITE | FMODE_EXCL;
	struct block_device *bdev = ERR_PTR(-ENXIO);
    int count = 0;

	EOB_LOG(EOB_DEBUG, "trying to find disk (%s)", path);

	while (IS_ERR(bdev) && count < 50) {
        bdev = blkdev_get_by_path(path, mode, dev_holder);
		if (unlikely(PTR_ERR(bdev) == -ENOENT)) {
			msleep(10);
			count++;
		} else if (IS_ERR(bdev)) {
			break;
		}
	}

	if (IS_ERR(bdev)) {
		EOB_LOG(EOB_ERR, "failed open dev_path=%s, error=%lu count=%d", path, -PTR_ERR(bdev), count);
		return SET_CERR(-PTR_ERR(bdev));
	}

    if(bdev == NULL) {
        EOB_LOG(EOB_ERR, "%s", "bdev is null -> open failed.");
        return SET_CERR(-EINVAL);
    }

    EOB_LOG(EOB_DEBUG, "Found disk (%s -> %p)", path, bdev);

    if (dev == NULL)
    {
        EOB_LOG(EOB_ERR, "%s", "Priv struct not allocated");
        return SET_CERR(-EINVAL);
    }
    
    EOB_LOG(EOB_DEBUG, "Priv is %s", "ok");

    if(NULL == bdev->bd_disk)
    {
        EOB_LOG(EOB_ERR, "%s", "Device don't have gendisk");
        return SET_CERR(-EINVAL);
    }
    
    EOB_LOG(EOB_DEBUG, "Device is %s", "ok");

    strncpy(dev->path, path, PATH_MAX);
    dev->sectors = get_capacity(bdev->bd_disk);
    dev->bdev = bdev;
    
    EOB_LOG(EOB_DEBUG, "%s", "Successfully inited ndisk");

    return (0);
}

/******************************************************************************/
void eob_close_blk(struct eob_device *dev)
{
    const fmode_t mode = FMODE_READ | FMODE_WRITE | FMODE_EXCL;
    blkdev_put(dev->bdev, mode);
}

void eob_close_fd(struct eob_device *dev)
{
	filp_close(dev->fd, NULL);
}

/******************************************************************************/
int eob_net_device_open(struct net_device *ndev)
{
    struct eob_device *dev = netdev_priv(ndev);
    int result;

    if (0 != (result = eob_open_fd(dev))) {
        EOB_LOG(EOB_ERR, "Unable to open file");
        return SET_CERR(-ENODEV);
    }

    /*if((result = eob_open_blk(dev, dev->path))) {*/
        /*EOB_LOG(EOB_ERR, "%s", "Unable to open blk device");*/
        /*return SET_CERR(-ENODEV);*/
    /*}*/

    return (0);
}

/******************************************************************************/
int eob_device_release(struct net_device *ndev)
{
    struct eob_device *dev = netdev_priv(ndev);
    eob_close_fd(dev);
    /*eob_close_blk(dev);*/
    return (0);
}

/******************************************************************************/
static const struct net_device_ops eob_device_ops = {
	.ndo_open            = eob_net_device_open,
	.ndo_stop            = eob_device_release,
    /*.ndo_start_xmit      = ioscsi_tx,*/
	/*.ndo_do_ioctl        = ioscsi_ioctl,*/
	/*.ndo_set_config      = ioscsi_config,*/
	/*.ndo_get_stats       = ioscsi_stats,*/
	/*.ndo_change_mtu      = ioscsi_change_mtu,*/
	/*.ndo_tx_timeout      = ioscsi_tx_timeout*/
};

/******************************************************************************/
void eob_device_ctor(struct net_device *dev)
{
    struct eob_device *priv;
    /*int i;*/

    ether_setup(dev); /* assign some of the fields */
    /*dev->watchdog_timeo = eoblock_timeout;*/

    dev->netdev_ops = &eob_device_ops;

    priv = netdev_priv(dev);
    memset(priv, 0, sizeof(struct eob_device));

    priv->ndev = dev;
    // netif_napi_add(dev, &priv->napi, snull_poll,2);
    spin_lock_init(&priv->lock);
    /*ioscsi_rx_ints(dev, 1);		[> enable receive interrupts <]*/
}

/******************************************************************************/
void eob_device_dtor(struct net_device *dev);

/******************************************************************************/
int eob_device_open(const char *path, const char *pname, edev_t **pres)
{
    int result;
    struct net_device *ndev;
    struct eob_device *dev;
    
    ndev = eob_alloc_netdev(sizeof(struct eob_device), pname, eob_device_ctor);

    if (!ndev) {
        NOMEMORY_TRACE("Allocate net device.");
        return SET_CERR(-ENODEV);
    }

    dev = netdev_priv(ndev);

    strcpy(dev->path, path);
    strcpy(dev->name, pname);

    if( (result = register_netdev(ndev))) {
        free_netdev(ndev);
        EOB_LOG(EOB_ERR, "%s", "Unable to register eoblock device");
        return SET_CERR(-ENODEV);
    }

    EOB_LOG(EOB_INFO, "eoblock device %s (%s) initialized", ndev->name, dev->path);

    *pres = dev;

    return (0);
}

int eob_device_close(edev_t *dev)
{
        
}

/******************************************************************************/
int add_eob_device_impl(const char *path, int node_id, struct eob_device **pres)
{
    int result;
    struct net_device *ndev;
    struct eob_device *dev;
    
    ndev = eob_alloc_netdev(sizeof(struct eob_device), "eob%d", eob_device_ctor);

    if (!ndev) {
        NOMEMORY_TRACE("Allocate net device.");
        return SET_CERR(-ENODEV);
    }

    dev = netdev_priv(ndev);

    strcpy(dev->path, path);

    if( (result = register_netdev(ndev))) {
        free_netdev(ndev);
        EOB_LOG(EOB_ERR, "%s", "Unable to register eoblock device");
        return SET_CERR(-ENODEV);
    }

    EOB_LOG(EOB_INFO, "eoblock device %s (%s) initialized", ndev->name, dev->path);

    *pres = dev;

    return (0);
}

/******************************************************************************/
int remove_eob_device_impl(struct eob_device *dev)
{
    unregister_netdev(dev->ndev);
    free_netdev(dev->ndev);
    return (0);
}

EXPORT_SYMBOL(add_eob_device_impl);
EXPORT_SYMBOL(remove_eob_device_impl);
