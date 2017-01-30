#ifndef EOBLOCK_H_
#define EOBLOCK_H_
/******************************************************************************/
#ifndef EOB_KERNEL
#include <assert.h>
#include <sys/eob_log.h>
#include <stdint.h>
#endif

/******************************************************************************/
#define MAX_CLUSTER_SIZE 32
#define EOB_DEFAULT_BLK_SIZE 4096

/******************************************************************************/
struct eob_block;
struct eob_device;
typedef uint64_t eob_address;

/******************************************************************************/
struct eob_hw_addr {
    union {
        uint64_t i_value;
        struct {
            uint8_t mac[6];
            uint8_t __padding[2];
        } mac_value;
        struct {
            uint8_t node_id;
            uint8_t nodes_count;
            uint8_t virt_blk_shift;
            uint8_t phys_blk_shift;
            uint8_t __padding[6];
        } idx;
    };
};

#define EOB_HW_ADDR_INIT(NODE_ID, NODES_COUNT, VSHIFT, PSHIFT)  \
    {                                                           \
        .idx.node_id = NODE_ID,                                 \
        .idx.nodes_count = NODES_COUNT,                         \
        .idx.virt_blk_shift = VSHIFT,                           \
        .idx.phys_blk_shift = PSHIFT,                           \
    }

#define EOB_HW_ADDR_INIT_DEFAULT(NODE_ID, NODES_COUNT) EOB_HW_ADDR_INIT(NODE_ID, NODES_COUNT, 9, 9)
#define EOB_HW_VIRT_BLK_SIZE(hw) (1 << (hw)->idx.virt_blk_shift)

/******************************************************************************/
struct eob_chksum {
    uint64_t value;
} ATTR_PACKED;

typedef enum {
    EOB_NODE_PRESENT = 1,
} eob_node_flags_t;

#define EOB_NODE_IS_PRESENT(n) ((n)->flags & EOB_NODE_PRESENT)

struct eob_node_data_phys {
    uint32_t flags;
    uint32_t ip_addr;
} ATTR_PACKED;

struct eob_header_peamb {
    uint64_t magic;
    uint64_t version;
    uint64_t phys_blk_size;
    uint64_t virt_blk_size;
    uint64_t nodes_count;
    struct eob_node_data_phys nodes[MAX_CLUSTER_SIZE];
} ATTR_PACKED;

struct eob_header {
    struct eob_header_peamb h;
    uint8_t __padding[512 - sizeof(struct eob_chksum) - sizeof(struct eob_header_peamb)];
    struct eob_chksum chksum;
} ATTR_PACKED;

#define EOB_HDR_NODE_AT(__hdr, i) (&(__hdr)->nodes[i])

struct eob_block_virt;

struct eob_packet_head {
    struct eob_hw_addr src_addr;
    uint64_t data_len;
} ATTR_PACKED;

struct eob_packet_tail {
    struct eob_chksum chksum;
} ATTR_PACKED;

#define EOB_PKT_NDATA_LEN (sizeof(struct eob_packet_head) + sizeof(struct eob_packet_tail))

struct eob_packet_512 {
    struct eob_packet_head head;
    uint8_t data[512 - EOB_PKT_NDATA_LEN];
    struct eob_packet_tail tail;
} ATTR_PACKED;

struct eob_packet_4k {
    struct eob_packet_head head;
    uint8_t data[4096 - EOB_PKT_NDATA_LEN];
    struct eob_packet_tail tail;
} ATTR_PACKED;

struct eob_packet_8k {
    struct eob_packet_head head;
    uint8_t data[8192 - EOB_PKT_NDATA_LEN];
    struct eob_packet_tail tail;
} ATTR_PACKED;

struct eob_packet_16k {
    struct eob_packet_head head;
    uint8_t data[16384 - EOB_PKT_NDATA_LEN];
    struct eob_packet_tail tail;
} ATTR_PACKED;





/******************************************************************************/
#endif /* EOBLOCK_H_ */
