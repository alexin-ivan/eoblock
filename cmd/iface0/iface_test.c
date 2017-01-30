/******************************************************************************/
#include "iface_test.h"
#include <eoblock.h>
#include <sys/avl.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <liboblock.h>
#include <stdlib.h>
#include <sys/eob_debug.h>

/******************************************************************************/
#undef MODULE
#define MODULE "test: "

/******************************************************************************/
const char *dev_name = NULL;

/******************************************************************************/
struct avl_test_node{
    avl_node_t node;
    int value;
    const char *tag;
};

/******************************************************************************/
int avl_test_node_cmp(const void * a, const void *b)
{
    return AVL_CMP(((const struct avl_test_node*) a)->value, ((const struct avl_test_node*) b)->value);
}

/******************************************************************************/
int avl_test()
{
    avl_tree_t tree;
    struct avl_test_node nsearch = {{{0}}};
    avl_index_t hint;
    struct avl_test_node *idx;

    struct avl_test_node nodes[] = {
        { .value = 1, .tag = "1", },
        { .value = 2, .tag = "2", },
        { .value = 3, .tag = "3", },
        { .value = 1, .tag = "4", },
    };

    avl_create(&tree, avl_test_node_cmp, sizeof(struct avl_test_node), offsetof(struct avl_test_node, node));

    for (int i = 0; i < ARRAY_SIZE(nodes); i++)
    {
        if(NULL == (idx = avl_find(&tree, &nodes[i], &hint)))
        {
            avl_add(&tree, &nodes[i]);
        }
        else
        {
            idx->tag = nodes[i].tag;
        }
    }

    nsearch.value = 1;
    idx = avl_find(&tree, &nsearch, &hint);

    EOB_VERIFY(idx != NULL);

    EOB_VERIFY(STR_EQ(idx->tag, "4"));

    return (0);
}

int bit_hacks_test()
{
    uint64_t blk_sz = 8;
    uint64_t i;
    uint64_t r;

    uint64_t value_512 = 512;
    uint64_t value_4096 = 4096;

    for(i = 0; i < 50; i++)
    {
        EOB_MSG(EOB_INFO, "VALUE=%lu", i);

        r = P2ALIGN(i, blk_sz);
        EOB_MSG(EOB_INFO, "P2ALIGN=%lu", r);
        
        r = P2ROUNDUP(i, blk_sz);
        EOB_MSG(EOB_INFO, "P2ROUNDUP=%lu", r);
        
        r = P2PHASE(i, blk_sz);
        EOB_MSG(EOB_INFO, "P2PHASE=%lu", r);

    }
    
    EOB_VERIFY_EQUAL(9, highbit64(value_512));
    EOB_VERIFY_EQUAL(9, (uint8_t) highbit64(value_512));
    EOB_VERIFY_EQUAL(12, highbit64(value_4096));
    EOB_VERIFY_EQUAL(14, highbit64( (uint64_t) 4 * 4096));

    return (0);
}

int test_chksum()
{
    struct ttt {
        char data[16];
        uint32_t chksum;
    };

    return (0);
}

int test_eob_send_recv()
{
    char *str_data = "Hello! Humans! We are here!";
    char *dst_data = NULL;

    struct eob_device *dev_src;
    struct eob_device *dev_dst;

    struct eob_hw_addr addr_src = EOB_HW_ADDR_INIT_DEFAULT(0, 2);
    struct eob_hw_addr addr_dst = EOB_HW_ADDR_INIT_DEFAULT(1, 2);
    struct eob_hw_addr addr_from = {{0}};

    EOB_VERIFY(0 == eob_device_open_file(&dev_src, &addr_src, dev_name, 0));
    EOB_VERIFY(0 == eob_device_send_str(dev_src, str_data, &addr_dst));
    eob_device_close(dev_src);

    EOB_VERIFY(0 == eob_device_open_file(&dev_dst, &addr_dst, dev_name, 0));
    EOB_VERIFY(0 == eob_device_recv_str(dev_dst, &dst_data, &addr_from));
    eob_device_close(dev_src);

    EOB_VERIFY(0 == strcmp(str_data, dst_data));
    EOB_VERIFY(addr_src.idx.node_id == addr_from.idx.node_id);

    free(dst_data);

    return (0);
}

int test_eob_open_alloc_close()
{
    struct eob_device *dev;
    struct eob_hw_addr addr;
    struct eob_hw_addr another;
    struct iovec io_data;
    char *str_data = "Hello! Humans! We are here!";
    size_t data_len = strlen(str_data) + 1;
    int my_node_id = 1;

    addr.idx.node_id = my_node_id;
    addr.idx.nodes_count = 2;
    addr.idx.virt_blk_shift = 9;
    addr.idx.phys_blk_shift = 9;

    another.idx.node_id = my_node_id;
    another.idx.nodes_count = 2;
    another.idx.virt_blk_shift = 9;
    another.idx.phys_blk_shift = 9;

    io_data.iov_base = str_data;
    io_data.iov_len = data_len;

    EOB_VERIFY(0 == eob_device_open_file(&dev, &addr, dev_name, 0));
    EOB_VERIFY(0 == eob_device_send(dev, &io_data, &another));
    eob_device_close(dev);
    another.i_value = 0;

    EOB_VERIFY(0 == eob_device_open_file(&dev, &addr, dev_name, 0));
    EOB_VERIFY(0 == eob_device_recv(dev, &io_data, &another));
    eob_device_close(dev);

    EOB_VERIFY(0 == strcmp(io_data.iov_base, str_data));
    EOB_VERIFY(another.idx.node_id == my_node_id);

    return (0);
}

int test_find_dev_by_path()
{
    char *path;
    
    path = find_disk_by_path(dev_name);

    EOB_VERIFY_NOT_NULL(path);

    return (0);
}

int main_test(int argc, char **argv)
{
    int rv;
    void *ctx;

    dev_name = argv[2];

    EOB_VERIFY_NOT_NULL(dev_name);


    


    rv = test_find_dev_by_path();
    EOB_VERIFY_EQUAL(rv, 0);

    rv = liboblock_init(&ctx);
    EOB_VERIFY(rv == 0);

    /*rv =  avl_test();*/
    /*EOB_VERIFY(rv == 0);*/
    
    /*rv =  bit_hacks_test();*/
    /*EOB_VERIFY(rv == 0);*/

    /*rv =  test_chksum();*/
    /*EOB_VERIFY(rv == 0);*/

    rv =  test_eob_open_alloc_close();
    EOB_VERIFY(rv == 0);

    rv =  test_eob_send_recv();
    EOB_VERIFY(rv == 0);

    liboblock_exit(ctx);

    return rv;
}
