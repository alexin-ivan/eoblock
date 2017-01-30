#ifndef LIBOBLOCK_H_
#define LIBOBLOCK_H_
/******************************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <eoblock.h>
#include <sys/uio.h>
#include <eoblock.h>

struct eob_packet {
    struct eob_packet_head head;
    struct iovec data;
    struct eob_packet_tail tail;
};

/******************************************************************************/
CTASSERT_GLOBAL(sizeof(struct eob_header) == sizeof(char) * 512);

CTASSERT_GLOBAL(sizeof(struct eob_packet_512) == sizeof(char) * 512);
CTASSERT_GLOBAL(sizeof(struct eob_packet_4k) == sizeof(char) * 4096);
CTASSERT_GLOBAL(sizeof(struct eob_packet_8k) == sizeof(char) * 8192);
CTASSERT_GLOBAL(sizeof(struct eob_packet_16k) == sizeof(char) * 16384);


/******************************************************************************/
int eob_device_open_fd(
    struct eob_device **self,
    struct eob_hw_addr *addr,
    int fd,
    const char *name,
    int force
);

/******************************************************************************/
int eob_device_open_file(
    struct eob_device **self,
    const struct eob_hw_addr *addr,
    const char *path,
    int force
);

/******************************************************************************/
char *find_disk_by_path(const char *disk_path);

/******************************************************************************/
void eob_device_close(struct eob_device *dev);

/******************************************************************************/
int eob_hdr_init(
    int fd,
    int nodes_count,
    int virt_blk_size,
    int phys_blk_size,
    int force
);

/******************************************************************************/
int eob_hdr_deinit(
    int fd
);

/******************************************************************************/
int eob_hdr_add_device(
    int fd,
    int node_id,
    struct eob_node_data_phys *data,
    int force
);

/******************************************************************************/
int eob_hdr_rem_device(
    int fd,
    int node_id
);

/******************************************************************************/
int eob_hdr_dump(
    int fd,
    char **pstr
);

/******************************************************************************/
int eob_device_recv_raw(struct eob_device *dev, struct iovec *d);
int eob_device_send_raw(struct eob_device *dev, const  struct eob_hw_addr *dst, const struct iovec *d);

/******************************************************************************/
int eob_device_recv(struct eob_device *dev, struct iovec *d, struct eob_hw_addr *src);
int eob_device_send(struct eob_device *dev, const struct iovec *d, const struct eob_hw_addr *dst);

/******************************************************************************/
/* Receive string message from src_node_id */
int eob_device_recv_str(struct eob_device *dev, char **pstr, struct eob_hw_addr *src);

/* Send string message to dst_node_id */
int eob_device_send_str(struct eob_device *dev, const char *s, struct eob_hw_addr *dst);

/******************************************************************************/
int liboblock_init(void **ctx);
void liboblock_exit(void *ctx);

/******************************************************************************/
#endif /* LIBOBLOCK_H_ */
