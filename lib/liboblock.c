/******************************************************************************/
#define _GNU_SOURCE
#include <liboblock.h>
#include <sys/eob_log.h>
#include <arpa/inet.h>
#include <sys/vec.h>
#include <sys/cvec.h>
#include <errno.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <chksum.h>
#include <eoblock_defs.h>
#include <unistd.h>
#include <sys/eob_debug.h>
#include <libudev.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/******************************************************************************/
#undef MODULE
#define MODULE "liboblock: "

#define ALLOC_OBJECT(var) (var = eob_alloc(sizeof(*var)))
#define FREE_OBJECT(var) (var ? (eob_free(var), 0) : 0)

#define EOB_HDR_VERSION 2
#define EOB_HDR_MAGIC 0xe0b10c3003
#define EOB_MAX_VIRT_BLK_SIZE (1 * 1024 * 1024)

/******************************************************************************/
int liboblock_init(void **ctx)
{
    return eob_log_init();
}

/******************************************************************************/
void liboblock_exit(void *ctx)
{
    eob_log_exit();
}


static int eob_header_offset = 0;

/******************************************************************************/
typedef c_vec_t(struct iovec) c_vec_ioblock_t;
typedef vec_t(struct iovec) vec_ioblock_t;

struct eob_block {
    struct eob_header hdr;
    struct eob_device *dev;
    void *data;
    size_t data_len;
    struct iovec *io;
    uint64_t io_count;
};

struct eob_device {
    struct eob_hw_addr addr;
    char path[PATH_MAX];
    int fd;

    struct eob_header hdr;
    struct iovec phys_buff;
    /*struct eob_header hdr;*/
};

struct eob_buff {
    struct eob_device *dev;
    size_t virt_blk_size;
    size_t phys_blk_size;

    struct iovec data;

    /* iovec of iovecs */
    struct iovec phys_blocks;

};

#define EOB_HDR_SIZE sizeof(struct eob_header)

/******************************************************************************/
void *eob_alloc(size_t len)
{
    return calloc(1, len);
}

void eob_free(void *p)
{
    free(p);
}

int eob_iovec_alloc(struct iovec *v)
{
    v->iov_base = malloc(v->iov_len);
    return v->iov_base == NULL ? no_memory_trace() : 0;
}

int eob_iovec_copy(struct iovec *to, const struct iovec *from)
{
    to->iov_len = from->iov_len;

    if(eob_iovec_alloc(to))
        return no_memory_trace();

    memcpy(to->iov_base, from->iov_base, to->iov_len);

    return (0);
}

void *memdup(void *data, size_t len)
{
    void *r = malloc(len);

    if(!r)
        return NULL;

    memcpy(r, data, len);

    return r;
}

void eob_iovec_alloc_wait(struct iovec *v)
{
    EOB_VERIFY(v->iov_len > 0);
    while(NULL == (v->iov_base = malloc(v->iov_len)));
}

void eob_iovec_calloc_wait(struct iovec *v)
{
    EOB_VERIFY(v->iov_len > 0);
    while(NULL == (v->iov_base = calloc(1, v->iov_len)));
}

void eob_iovec_free(struct iovec *v)
{
    free(v->iov_base);
    v->iov_base = NULL;
    v->iov_len = 0;
}

/******************************************************************************/
static int eob_write_header(int fd, const struct eob_header *hdr)
{
    int rv;
    struct iovec buf;

    EOB_VERIFY(fd > 0);
    EOB_VERIFY(hdr->h.phys_blk_size >= sizeof(*hdr));

    buf.iov_len = hdr->h.phys_blk_size;
    eob_iovec_calloc_wait(&buf);

    memcpy(buf.iov_base, hdr, sizeof(*hdr));

    rv = pwritev(fd, &buf, 1, eob_header_offset);

    eob_iovec_free(&buf);

    if (rv < 0)
        return SET_IO_ERR();

    return (0);
}

static int eob_device_write_header(struct eob_device *dev, const struct eob_header *hdr)
{
    int rv;

    EOB_VERIFY(dev->fd > 0);
    EOB_VERIFY(dev->phys_buff.iov_len == hdr->h.phys_blk_size);

    if (dev->hdr.h.phys_blk_size > sizeof(*hdr))
        memset(dev->phys_buff.iov_base, 0, dev->phys_buff.iov_len);

    memcpy(dev->phys_buff.iov_base, hdr, sizeof(*hdr));
    
    rv = pwritev(dev->fd, &dev->phys_buff, 1, eob_header_offset);

    if (rv < 0)
        return SET_IO_ERR();

    return (0);
}

/******************************************************************************/
static int eob_read_header(int fd, struct eob_header *hdr)
{
    uint64_t off = eob_header_offset;
    ssize_t rv;
    int max_pass = 10;
    int pass = 0;

    EOB_VERIFY(fd > 0);

    do 
    {
        pass++;

        rv = pread(fd, hdr, sizeof(*hdr), off);
        
        if(rv < 0)
            return SET_IO_ERR();

        if(B_TRUE == eob_header_chksum_is_ok(hdr))
        {
            if(EOB_HDR_MAGIC == hdr->h.magic)
            {
                if(EOB_HDR_VERSION != hdr->h.version)
                {
                    return SET_ERR(EOB_VER_MISM);
                }
                else
                    return EOB_OK;
            }
            else
            {
                EOB_LOG(EOB_INFO, "Magic is failed. Attempt to reload header");
                continue;
            }
        } else
        {
            EOB_LOG(EOB_INFO, "Checksum calc is failed, magic is %s. Attempt to reload header", EOB_HDR_MAGIC == hdr->h.magic ? "ok" : "mismatch");
        }

    } while(pass < max_pass);

    EOB_LOG(EOB_ERR, "Checksum calc is finally failed");

    rv = SET_ERR(EOB_CHKSUM);
    
    return rv;
}

static int eob_device_read_header(struct eob_device *dev, struct eob_header *hdr)
{
    uint64_t off = eob_header_offset;
    ssize_t rv;
    int max_pass = 10;
    int pass = 0;

    EOB_VERIFY(dev->fd > 0);

    do 
    {
        pass++;

        rv = pread(dev->fd, hdr, sizeof(*hdr), off);
        
        if(rv < 0)
            return SET_IO_ERR();

        if(B_TRUE == eob_header_chksum_is_ok(hdr))
        {
            if(EOB_HDR_MAGIC == hdr->h.magic)
            {
                if(EOB_HDR_VERSION != hdr->h.version)
                {
                    return SET_ERR(EOB_VER_MISM);
                }
                else
                    return EOB_OK;
            }
            else
            {
                EOB_LOG(EOB_INFO, "Magic is failed. Attempt to reload header");
                continue;
            }
        } else
        {
            EOB_LOG(EOB_INFO, "Checksum calc is failed, magic is %s. Attempt to reload header", EOB_HDR_MAGIC == hdr->h.magic ? "ok" : "mismatch");
        }

    } while(pass < max_pass);

    EOB_LOG(EOB_ERR, "Checksum calc is finally failed");

    rv = SET_ERR(EOB_CHKSUM);
    
    return rv;
}

/******************************************************************************/
static int eob_buff_alloc_phys(struct eob_buff *b)
{
    int i;
    uint64_t io_count = (P2ROUNDUP(b->virt_blk_size, b->phys_blk_size)) / b->phys_blk_size;
    struct iovec *iter;

    b->phys_blocks.iov_len = io_count * sizeof(struct iovec);

    if(eob_iovec_alloc(&b->phys_blocks))
        return SET_ERR(ENOMEM);

    for(i = 0; i < io_count; i++)
    {
        iter = ((struct iovec*) b->phys_blocks.iov_base) + i;
        iter->iov_len = b->phys_blk_size;
        eob_iovec_alloc_wait(iter);
    }
    
    return (0);
}

static int eob_buff_map_phys_to_data(struct eob_buff *b, size_t data_len)
{
    struct iovec *v = b->phys_blocks.iov_base;
    size_t i;
    size_t vec_count = b->phys_blocks.iov_len / sizeof(struct iovec);
    size_t copied = 0;

    EOB_VERIFY(b->data.iov_base == 0);
    EOB_VERIFY(b->data.iov_len == 0);

    b->data.iov_len = data_len;
    eob_iovec_alloc_wait(&b->data);

    for(i = 0; i < vec_count; i++)
    {
        memcpy(((char *) b->data.iov_base) + copied,  v[i].iov_base, v[i].iov_len);
        copied += v[i].iov_len;
        if(copied >= data_len)
            break;
    }

    EOB_VERIFY_EQUAL(copied, data_len);

    return (0);
}

static int eob_buff_map_data_to_phys(struct eob_buff *b)
{
    int i;
    uint64_t io_count = (P2ROUNDUP(b->data.iov_len, b->phys_blk_size)) / b->phys_blk_size;
    struct iovec *iter;

    b->phys_blocks.iov_len = io_count * sizeof(struct iovec);

    if(eob_iovec_alloc(&b->phys_blocks))
        return SET_ERR(ENOMEM);

    for(i = 0; i < io_count; i++)
    {
        iter = ((struct iovec*) b->phys_blocks.iov_base) + i;
        iter->iov_len = b->phys_blk_size;
        eob_iovec_alloc_wait(iter);

        if(i < io_count - 1)
            memcpy(iter->iov_base, ((char *) b->data.iov_base) + (i * b->phys_blk_size), b->phys_blk_size);
        else
            memcpy(iter->iov_base, ((char *) b->data.iov_base) + (i * b->phys_blk_size), b->data.iov_len - (i * b->phys_blk_size));
    }
    
    return (0);
}

/******************************************************************************/
static int eob_buff_write_impl(struct eob_buff *buff, uint64_t virt_offset)
{
    int rv;
    uint64_t offset = eob_header_offset + EOB_HDR_SIZE + (buff->virt_blk_size * virt_offset);

    rv = pwritev(buff->dev->fd, buff->phys_blocks.iov_base, buff->phys_blocks.iov_len / sizeof(struct iovec), offset);

    if (rv < 0)
        return SET_IO_ERR();

    return (0);
}

/******************************************************************************/
int eob_buff_write(struct eob_buff *buff, uint64_t node_id)
{
    int rv;
    
    if(0 != (rv = eob_buff_map_data_to_phys(buff)))
        return rv;

    return eob_buff_write_impl(buff, node_id);
}

/******************************************************************************/
static int eob_buff_read_impl(struct eob_buff *buff)
{
    int rv;
    uint64_t virt_offset = buff->dev->addr.idx.node_id;
    uint64_t offset = eob_header_offset + EOB_HDR_SIZE + (buff->virt_blk_size * virt_offset);
    
    rv = preadv(buff->dev->fd, buff->phys_blocks.iov_base, buff->phys_blocks.iov_len / sizeof(struct iovec), offset);

    if(rv < 0)
        return SET_IO_ERR();

    if(0 != (rv = eob_buff_map_phys_to_data(buff, rv)))
        return rv;

    return (0);
}

/******************************************************************************/
int eob_buff_read(struct eob_buff *buff)
{
    int rv;

    if(0 != (rv = eob_buff_alloc_phys(buff)))
        return rv;
    
    if(0 != (rv = eob_buff_read_impl(buff)))
        return rv;
    
    return (0);
}

/******************************************************************************/
struct eob_buff *eob_buff_create(
    struct eob_device *dev,
    const struct iovec *data
)
{
    struct eob_buff *buff;
    
    if(NULL == ALLOC_OBJECT(buff))
        return NULL;

    if(data)
    {
        if(eob_iovec_copy(&buff->data, data))
        {
            FREE_OBJECT(buff);
            return NULL;
        }
    }

    buff->dev = dev;
    buff->virt_blk_size = dev->hdr.h.virt_blk_size;
    buff->phys_blk_size = dev->hdr.h.phys_blk_size;

    return buff;
}

void eob_buff_destroy(struct eob_buff *buff)
{
    int i;

    EOB_VERIFY_NOT_NULL(buff);
    EOB_VERIFY_NOT_NULL(buff->dev);

    if(buff->data.iov_len)
        eob_iovec_free(&buff->data);

    for(i = 0; i < buff->phys_blocks.iov_len / sizeof(struct iovec); i++)
        eob_iovec_free(((struct iovec *) buff->phys_blocks.iov_base) + i);
}

/******************************************************************************/
int eob_device_send_raw(struct eob_device *dev, const struct eob_hw_addr *dst, const struct iovec *d)
{
    int rv;
    struct eob_buff *buff;
    
    if(NULL == (buff = eob_buff_create(dev, d)))
        return no_memory_trace();

    rv = eob_buff_write(buff, dst->idx.node_id);

    eob_buff_destroy(buff);

    return rv;
}

int eob_device_recv_raw(struct eob_device *dev, struct iovec *d)
{
    struct eob_buff *buff;
    int rv;

    if(NULL == (buff = eob_buff_create(dev, NULL)))
        return no_memory_trace();

    if(0 != (rv = eob_buff_read(buff)))
    {
        eob_buff_destroy(buff);
        return rv;
    }

    if(eob_iovec_copy(d, &buff->data))
        return no_memory_trace();

    eob_buff_destroy(buff);
    return rv;
}

/******************************************************************************/
int eob_device_recv(struct eob_device *dev, struct iovec *out_data, struct eob_hw_addr *src)
{
    int rv;
    struct iovec d;
    struct eob_packet gpkt;
    struct eob_chksum s;

    if(0 != (rv = eob_device_recv_raw(dev, &d)))
        return rv;
    
    memset(&gpkt, 0, sizeof(gpkt));
 
    switch(dev->hdr.h.virt_blk_size)
    {
        case 512:
            {
                struct eob_packet_512 *pkt = d.iov_base;
                gpkt.data.iov_len = pkt->head.data_len;
                EOB_VERIFY(gpkt.data.iov_len < EOB_MAX_VIRT_BLK_SIZE);
                eob_iovec_alloc_wait(&gpkt.data);
                memcpy(gpkt.data.iov_base, pkt->data, gpkt.data.iov_len);
                gpkt.head = pkt->head;
                gpkt.tail = pkt->tail;
                break;
            }
        case 4096:
            {
                struct eob_packet_4k *pkt = d.iov_base;
                gpkt.data.iov_len = pkt->head.data_len;
                EOB_VERIFY(gpkt.data.iov_len < EOB_MAX_VIRT_BLK_SIZE);
                eob_iovec_alloc_wait(&gpkt.data);
                memcpy(gpkt.data.iov_base, pkt->data, gpkt.data.iov_len);
                gpkt.head = pkt->head;
                gpkt.tail = pkt->tail;
                break;
            }
        case 2 * 4096:
            {
                struct eob_packet_8k *pkt = d.iov_base;
                gpkt.data.iov_len = pkt->head.data_len;
                EOB_VERIFY(gpkt.data.iov_len < EOB_MAX_VIRT_BLK_SIZE);
                eob_iovec_alloc_wait(&gpkt.data);
                memcpy(gpkt.data.iov_base, pkt->data, gpkt.data.iov_len);
                gpkt.head = pkt->head;
                gpkt.tail = pkt->tail;
                break;
            }

        default:
            NOT_IMPLEMENTED();
            return SET_ERR(EOB_INVAL);
    }

    eob_chcksum_calc(gpkt.data.iov_base, gpkt.data.iov_len, &s);

    if(s.value != gpkt.tail.chksum.value)
        return SET_ERR(EOB_CHKSUM);

    *src = gpkt.head.src_addr;
    *out_data = gpkt.data;

    return (0);
}

/******************************************************************************/
int eob_device_send(struct eob_device *dev, const struct iovec *data, const struct eob_hw_addr *dst)
{
    int rv;
    struct eob_packet gpkt;
    struct iovec vpkt;

    if(data->iov_len > dev->hdr.h.virt_blk_size)
        return SET_ERR(EOB_2BIG);

    gpkt.data = *data;
    gpkt.head.src_addr = dev->addr;
    gpkt.head.data_len = data->iov_len;
    eob_chcksum_calc(data->iov_base, data->iov_len, &gpkt.tail.chksum);

    switch(dev->hdr.h.virt_blk_size)
    {
        case 512:
            {
                struct eob_packet_512 pkt;
                pkt.head = gpkt.head;
                pkt.tail = gpkt.tail;
                memset(pkt.data, 0, sizeof(pkt.data));
                memcpy(pkt.data, gpkt.data.iov_base, gpkt.data.iov_len);
                vpkt.iov_len = sizeof(pkt);
                vpkt.iov_base = memdup(&pkt, sizeof(pkt));
                break;
            }

        case 4096:
            {
                struct eob_packet_4k pkt;
                pkt.head = gpkt.head;
                pkt.tail = gpkt.tail;
                memset(pkt.data, 0, sizeof(pkt.data));
                memcpy(pkt.data, gpkt.data.iov_base, gpkt.data.iov_len);
                vpkt.iov_len = sizeof(pkt);
                vpkt.iov_base = memdup(&pkt, sizeof(pkt));
                break;
            }

        default:
            NOT_IMPLEMENTED();
            return SET_ERR(EOB_INVAL);
    }
 
    /* TODO: cleanup on error */

    if(0 != (rv = eob_device_send_raw(dev, dst, &vpkt)))
    {
        eob_iovec_free(&vpkt);
        return rv;
    }
    
    eob_iovec_free(&vpkt);
    return rv;
}

/******************************************************************************/
int eob_device_init_hdr(
    struct eob_device *dev,
    int force
)
{
    struct eob_header hdr = {{0}};
    int rv;
    boolean_t rewrite = force ? B_TRUE : B_FALSE;

    if (0 != (rv = eob_device_read_header(dev, &hdr)))
    {
        if (rv != EOB_CHKSUM && rv != EOB_VER_MISM)
            return rv;
        else
            rewrite = B_TRUE;
    }

    if (rewrite == B_FALSE)
    {
        if (hdr.h.phys_blk_size != 1 << (dev->addr.idx.phys_blk_shift))
        {
            EOB_LOG(EOB_ERR, "Invalid header: phys block size");
            return SET_ERR(EOB_INVAL);
        }

        if (hdr.h.virt_blk_size != 1 << (dev->addr.idx.virt_blk_shift))
        {
            EOB_LOG(EOB_ERR, "Invalid header: virt block size");
            return SET_ERR(EOB_INVAL);
        }

        if (hdr.h.virt_blk_size < hdr.h.phys_blk_size)
        {
            EOB_LOG(EOB_ERR, "Invalid header: virt block < phys block");
            return SET_ERR(EOB_INVAL);
        }

        dev->hdr = hdr;
        return (0);
    }

    hdr.h.version = EOB_HDR_VERSION;
    hdr.h.magic = EOB_HDR_MAGIC;
    hdr.h.nodes_count = dev->addr.idx.nodes_count;
    hdr.h.virt_blk_size = 1 << (dev->addr.idx.virt_blk_shift);
    hdr.h.phys_blk_size = 1 << (dev->addr.idx.phys_blk_shift);

    eob_header_chksum_calc(&hdr);

    if(0 != (rv = eob_device_write_header(dev, &hdr)))
        return rv;

    dev->hdr = hdr;


    EOB_LOG(EOB_DEBUG, "Header initialized");

    return (0);
}

/******************************************************************************/
static char *part_name_to_disk_name(const char *name)
{
    int i;
    int len = strlen(name);

    for (i = 0; i < len; i++)
    {
        if (isdigit(name[i]))
            return strndup(name, i);
    }

    return NULL;
}

char *find_disk_by_path(const char *disk_path)
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    const char *path;
    struct udev_device *dev;
    char *disk = NULL;

    udev = udev_new();

    if(!udev)
    {
        EOB_LOG(EOB_ERR, "Can not init udev ctx");
        return NULL;
    }

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices) {
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

        if(STR_EQ(udev_device_get_property_value(dev, "DEVNAME"), disk_path))
        {
            if(STR_EQ(udev_device_get_property_value(dev, "DEVTYPE"), "partition"))
                disk = part_name_to_disk_name(disk_path);
            if(STR_EQ(udev_device_get_property_value(dev, "DEVTYPE"), "disk"))
                disk = strdup(disk_path);
        }

        /*strcat(path, udev_device_get_property_value(dev, "DEVPATH"));*/
        /*strcpy(disk_item->name, udev_device_get_property_value(dev, "ID_SERIAL"));*/
        /*strcpy(disk_item->dev_name, udev_device_get_property_value(dev, "DEVNAME"));*/
        /*strcpy(disk_item->model, udev_device_get_property_value(dev, "ID_MODEL"));*/
        /*strcpy(disk_item->bus, udev_device_get_property_value(dev, "ID_BUS"));*/
        /*disk_item->rotational = atoi(udev_device_get_sysattr_value(dev, "queue/rotational"));*/
        /*disk_item->hw_sector_size = atoi(udev_device_get_sysattr_value(dev, "queue/hw_sector_size"));*/
        /*disk_item->physical_block_size = atoi(udev_device_get_sysattr_value(dev, "queue/physical_block_size"));*/
        /*SP_SIZE_VALUE(&(disk_item->size)) = size_in_sectors = (atoll(udev_device_get_sysattr_value(dev, "size")) * DISK_BLOCK_SIZE);*/
        /*size_in_sectors = size_in_sectors / disk_item->hw_sector_size;*/

        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);

    return disk;
}



static int udev_get_phys_blk_size(const char *disk_path, uint64_t *pphys_blk_size)
{
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;
    const char *path;

    udev = udev_new();

    if(!udev)
    {
        EOB_LOG(EOB_ERR, "Can not init udev ctx");
        return SET_ERR(EOB_INVAL);
    }

    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);

    udev_list_entry_foreach(dev_list_entry, devices) {
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);

        if(STR_EQ(udev_device_get_property_value(dev, "DEVNAME"), disk_path))
        {
            EOB_VERIFY(STR_EQ(udev_device_get_property_value(dev, "DEVTYPE"), "disk"));
            *pphys_blk_size = atoi(udev_device_get_sysattr_value(dev, "queue/physical_block_size"));
        }

        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);

    return (0);
}

static int eob_device_open_disk(
    struct eob_device **pself,
    int node_id,
    int nodes_count,
    const char *path,
    int force
)
{
    uint64_t phys_blk_size;
    int rv;
    char *path_blk;
    struct eob_hw_addr addr;
    int fd;
    int flags = O_RDWR | O_SYNC | O_DIRECT;

    EOB_LOG(EOB_DEBUG, "Attempt to open: %s", path);

    path_blk = find_disk_by_path(path);

    if(!path_blk)
        return SET_ERR(EOB_NODEV);

    rv = udev_get_phys_blk_size(path, &phys_blk_size);
    EOB_LOG(EOB_DEBUG, "Found disk: %s", path_blk);

    free(path_blk);

    if (rv)
        return SET_ERR_TRACE(rv);

    fd = open(path, flags);

    if(fd < 0)
        return SET_IO_ERR();

    addr.idx.phys_blk_shift = highbit64(phys_blk_size);
    addr.idx.virt_blk_shift = (uint8_t) highbit64(4 * 4096);
    addr.idx.node_id = node_id;
    addr.idx.nodes_count = nodes_count;

    EOB_LOG(EOB_DEBUG, "Phys blk size: %d", 1 << addr.idx.phys_blk_shift);

    return eob_device_open_fd(pself, &addr, fd, path, force);
}

int eob_device_open(
    struct eob_device **pself,
    int node_id,
    int nodes_count,
    const char *path,
    int force
)
{
    struct stat64 statbuf;
    const struct eob_hw_addr addr = EOB_HW_ADDR_INIT_DEFAULT(node_id, nodes_count);
    int rv;

    if(!access(path, F_OK))
        return SET_ERR(EOB_NODEV);

    rv = stat64(path, &statbuf);

    if(rv < 0)
        return SET_IO_ERR();

    if(S_ISBLK(statbuf.st_mode))
        return eob_device_open_disk(pself, node_id, nodes_count, path, force);

    if((S_ISREG(statbuf.st_mode)))
        return eob_device_open_file(pself, &addr, path, force);

    EOB_LOG(EOB_ERR, "Invalid device (need disk or file)");
    return SET_ERR(EOB_INVAL);
}

int eob_device_open_file(
    struct eob_device **pself,
    const struct eob_hw_addr *addr,
    const char *path,
    int force
)
{
    struct eob_device *self;
    int rv;
    int flags = O_RDWR | O_SYNC;
    /*int flags = O_RDWR | O_NONBLOCK | O_DIRECT | O_SYNC;*/
    
    if (NULL == ALLOC_OBJECT(self))
        return SET_ERR(EOB_NOMEM);

    self->phys_buff.iov_len = (1 << addr->idx.phys_blk_shift);

    if (eob_iovec_alloc(&self->phys_buff))
        return SET_ERR(EOB_NOMEM);

    self->addr = *addr;
    strncpy(self->path, path, PATH_MAX);
    rv = open(path, flags, 0);

    if(rv < 0)
    {
        rv = SET_IO_ERR();
        eob_iovec_free(&self->phys_buff);
        FREE_OBJECT(self);
        return SET_ERR(EOB_NODEV);
    }

    self->fd = rv;

    if(0 != (rv = eob_device_init_hdr(self, force)))
    {
        if(rv != EOB_EALREADY)
        {
            eob_iovec_free(&self->phys_buff);
            FREE_OBJECT(self);
            return rv;
        }
        else
        {
            EOB_LOG(EOB_INFO, "eob label already inited for device %s", path);
        }
    }

 
    *pself = self;

    return (0);
}

/******************************************************************************/
int eob_device_open_fd(
    struct eob_device **pself,
    struct eob_hw_addr *addr,
    int fd,
    const char *name,
    int force
)
{
    struct eob_device *self;
    int rv;
    
    if (NULL == ALLOC_OBJECT(self))
        return SET_ERR(EOB_NOMEM);

    self->phys_buff.iov_len = (1 << addr->idx.phys_blk_shift);

    if (eob_iovec_alloc(&self->phys_buff))
        return SET_ERR(EOB_NOMEM);

    self->addr = *addr;
    strncpy(self->path, name, PATH_MAX);

    self->fd = fd;

    if(0 != (rv = eob_device_init_hdr(self, force)))
    {
        if(rv != EOB_EALREADY)
        {
            eob_iovec_free(&self->phys_buff);
            FREE_OBJECT(self);
            return rv;
        }
        else
        {
            EOB_LOG(EOB_INFO, "eob label already inited for device %s", name);
        }
    }

 
    *pself = self;

    return (0);
}


/******************************************************************************/
void eob_device_close(struct eob_device *self)
{
    if(self->fd && self->fd != STDIN_FILENO && self->fd != STDOUT_FILENO)
        (void) close(self->fd);

    eob_iovec_free(&self->phys_buff);
    FREE_OBJECT(self);
}

/******************************************************************************/
int eob_device_recv_str(struct eob_device *dev, char **pstr, struct eob_hw_addr *src)
{
    int rv;
    struct iovec v;

    rv = eob_device_recv(dev, &v, src);

    if(rv)
        return rv;

    if(v.iov_len == 0)
    {
        *pstr = NULL;
        return (0);
    }

    EOB_VERIFY(strlen(v.iov_base) + 1 == v.iov_len);

    *pstr = v.iov_base;
        
    return (0);
}

/******************************************************************************/
int eob_device_send_str(struct eob_device *dev, const char *s, struct eob_hw_addr *dst)
{
    struct iovec v;
    v.iov_base = (void *) s;
    v.iov_len = strlen(s) + 1;

    return eob_device_send(dev, &v, dst);
}

/******************************************************************************/
int eob_hdr_init(
    int fd,
    int nodes_count,
    int virt_blk_size,
    int phys_blk_size,
    int force
)
{
    int rv;
    struct eob_header hdr = {{0}};

    if(nodes_count == 0)
        nodes_count = MAX_CLUSTER_SIZE;

    if(virt_blk_size == 0)
        virt_blk_size = EOB_DEFAULT_BLK_SIZE;
    
    if(phys_blk_size == 0)
        phys_blk_size = EOB_DEFAULT_BLK_SIZE;

    /* TODO: return */
    VERIFY3U(phys_blk_size, <=, virt_blk_size);
    EOB_VERIFY_EQUAL(0, P2PHASE(virt_blk_size, phys_blk_size));

    hdr.h.nodes_count = nodes_count;
    hdr.h.virt_blk_size = virt_blk_size;
    hdr.h.phys_blk_size = phys_blk_size;
    hdr.h.version = EOB_HDR_VERSION;
    hdr.h.magic = EOB_HDR_MAGIC;

    eob_header_chksum_calc(&hdr);

    if(0 != (rv = eob_write_header(fd, &hdr)))
        return SET_ERR_TRACE(rv);

    return (0);
}

/******************************************************************************/
int eob_hdr_deinit(
    int fd
)
{
    int rv;
    struct eob_header hdr = {{0}};

    rv = pwrite(fd, &hdr, sizeof(hdr), eob_header_offset);

    if(rv < 0)
        return SET_IO_ERR();

    return (0);
}

/******************************************************************************/
char *eob_dump_nodes(struct eob_node_data_phys *nodes, size_t count)
{
    char line[256];
    int rv;
    char *buf;
    struct eob_node_data_phys *iter;
    struct in_addr addr;

    buf = calloc(1, sizeof(line) * count);
    EOB_VERIFY_NOT_NULL(buf);

    for(size_t i = 0; i < count; i++)
    {
        iter = nodes + i;
        addr.s_addr = iter->ip_addr;
        rv = snprintf(line, 256, 
            "\tNode %lu: ip=%s; flags=%s;\n", 
            i, inet_ntoa(addr), EOB_NODE_IS_PRESENT(iter) ? "P" : "nP"
        );
        EOB_VERIFY(rv > 0);
        strcat(buf, line);
    }

    return buf;
}

int eob_hdr_dump(
    int fd,
    char **pstr
)
{
    struct eob_header hdr = {{0}};
    int rv;
    char *nodes;

    rv = eob_read_header(fd, &hdr);

    if(rv)
        return SET_ERR_TRACE(rv);

    nodes = eob_dump_nodes(hdr.h.nodes, ARRAY_SIZE(hdr.h.nodes));

    rv = asprintf(pstr,
        "version: %lu\n"
        "magic: %lu\n"
        "nodes_count: %lu\n"
        "phys_blk_size: %lu\n"
        "virt_blk_size: %lu\n"
        "header_size: %lu\n"
        "%s\n"
        ,
        hdr.h.version,
        hdr.h.magic,
        hdr.h.nodes_count,
        hdr.h.phys_blk_size,
        hdr.h.virt_blk_size,
        sizeof(hdr),
        nodes
    );

    free(nodes);

    if(rv < 0)
        return SET_IO_ERR();

    return (0);
}

/******************************************************************************/
int eob_hdr_add_device(
    int fd,
    int node_id,
    struct eob_node_data_phys *data,
    int force
)
{
    struct eob_header hdr = {{0}};
    int rv;

    rv = eob_read_header(fd, &hdr);

    if(rv)
        return SET_ERR_TRACE(rv);

    if(node_id >= hdr.h.nodes_count)
    {
        EOB_LOG(EOB_ERR, "Invalid node id (more that max value).");
        return SET_ERR(EOB_INVAL);
    }
    
    if(!force && EOB_NODE_IS_PRESENT(&hdr.h.nodes[node_id]))
    {
        return SET_ERR(EOB_INVAL);
    }
    else 
    {
        hdr.h.nodes[node_id] = *data;
    }

    eob_header_chksum_calc(&hdr);

    if(0 != (rv = eob_write_header(fd, &hdr)))
        return SET_ERR_TRACE(rv);

    return (0);
}

/******************************************************************************/
int eob_hdr_rem_device(
    int fd,
    int node_id
)
{
    struct eob_header hdr = {{0}};
    struct eob_node_data_phys phys = {0};
    int rv;

    rv = eob_read_header(fd, &hdr);

    if(rv)
        return SET_ERR_TRACE(rv);
    
    if(!EOB_NODE_IS_PRESENT(&hdr.h.nodes[node_id]))
        return (0);

    hdr.h.nodes[node_id] = phys;

    eob_header_chksum_calc(&hdr);

    if(0 != (rv = eob_write_header(fd, &hdr)))
        return SET_ERR_TRACE(rv);

    return (0);
}

/******************************************************************************/
