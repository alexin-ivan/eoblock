#define _GNU_SOURCE
/******************************************************************************/
#include "iface.h"
#include <stdio.h>
#include <string.h>
#include "iface_test.h"
#include <liboblock.h>
#include <sys/cvec.h>
#include <stdlib.h>
#include <sys/eob_types.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/eob_debug.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/******************************************************************************/
#undef MODULE
#define MODULE "main: "

/******************************************************************************/
#define VERBOSE(X) do { if (iface_verbose == B_TRUE) X; } while(0)

/******************************************************************************/
static boolean_t iface_verbose = B_FALSE;

#define SET_VERBOSE() do { iface_verbose = B_TRUE; eob_log_set_verbose(); } while(0)

/******************************************************************************/
typedef enum {
    HELP_TEST,
    HELP_SEND,
    HELP_RECV,
    HELP_ALL,
    HELP_INIT,
    HELP_DEINIT,
    HELP_ADD,
    HELP_REMOVE,
    HELP_DUMP,
} iface_help_t;

/******************************************************************************/
typedef struct iface_command {
    const char *name;
    int (*fn)(int, char **);
    iface_help_t usage;
} iface_command_t;

/******************************************************************************/
int iface_do_test(int argc, char **argv) {
    return main_test(argc, argv);
}

/******************************************************************************/
static int iface_send(
    const char *filename, 
    const char *devicename,
    boolean_t force,
    int src_node_id,
    int dst_node_id,
    int nodes_count
) {
    int input_fd;
    char input_filename[PATH_MAX];
    struct eob_device *dev;
    int input_flags = O_RDONLY;
    int rv;
    struct eob_hw_addr addr_src = EOB_HW_ADDR_INIT_DEFAULT(src_node_id, nodes_count);
    struct eob_hw_addr addr_dst = EOB_HW_ADDR_INIT_DEFAULT(dst_node_id, nodes_count);
    char *str;

    VERBOSE(EOB_LOG(EOB_DEBUG, "Called: filename=%s, device=%s\n", filename, devicename));

    if(STR_EQ(filename, "-"))
    {
        input_fd = STDIN_FILENO;
        strcpy(input_filename, "stdin");
    }
    else
    {
        input_fd = open(filename, input_flags);
        strcpy(input_filename, filename);

        if(input_fd < 0)
            return SET_IO_ERR();
    }

    if(STR_EQ(devicename, "-"))
    {
        rv = eob_device_open_fd(&dev, &addr_src, STDOUT_FILENO, "stdout", force == B_TRUE);
    }
    else
    {
        rv = eob_device_open_file(&dev, &addr_src, devicename, force == B_TRUE);
    }

    if(rv)
        return SET_ERR(rv);

    EOB_VERIFY_NOT_NULL(dev);

    str = calloc(1, EOB_HW_VIRT_BLK_SIZE(&addr_src));

    rv = read(input_fd, str, EOB_HW_VIRT_BLK_SIZE(&addr_src));

    if(rv < 0)
    {
        if(input_fd != STDIN_FILENO)
            (void) close(input_fd);
        free(str);
        eob_device_close(dev);
        return SET_IO_ERR();
    }
    else
    {
        str[rv] = '\0';
    }

    rv = eob_device_send_str(dev, str, &addr_dst);

    free(str);
    eob_device_close(dev);

    if(input_fd != STDIN_FILENO)
        (void) close(input_fd);

    return (rv);
}

static int iface_recv(
    const char *filename, 
    const char *devicename,
    boolean_t force,
    int src_node_id,
    int nodes_count
) {
    int output_fd;
    char output_filename[PATH_MAX];
    struct eob_device *dev;
    int output_flags = O_WRONLY;
    int rv;
    struct eob_hw_addr addr_src = EOB_HW_ADDR_INIT_DEFAULT(src_node_id, nodes_count);
    struct eob_hw_addr addr_dst = EOB_HW_ADDR_INIT_DEFAULT(0, 0);
    char *str;

    VERBOSE(EOB_LOG(EOB_DEBUG, "Called: filename=%s, device=%s\n", filename, devicename));

    if(STR_EQ(filename, "-")) {
        output_fd = STDOUT_FILENO;
        strcpy(output_filename, "stdout");
    } else {
        output_fd = open(filename, output_flags);
        strcpy(output_filename, filename);

        if(output_fd < 0)
            return SET_IO_ERR();
    }

    if(STR_EQ(devicename, "-")) {
        rv = eob_device_open_fd(&dev, &addr_src, STDIN_FILENO, "stdin", force == B_TRUE);
    } else {
        rv = eob_device_open_file(&dev, &addr_src, devicename, force == B_TRUE);
    }

    if(rv)
        return SET_ERR(rv);

    EOB_VERIFY_NOT_NULL(dev);

    rv = eob_device_recv_str(dev, &str, &addr_dst);

    if (rv < 0) {
        if(output_fd != STDOUT_FILENO)
            (void) close(output_fd);
        eob_device_close(dev);
        return SET_ERR(rv);
    }

    if(iface_verbose == B_TRUE)
    {
        if(output_fd == STDOUT_FILENO)
            (void) fprintf(stdout, "%d: %s\n", addr_dst.idx.node_id, str);
        else
        {
            rv = write(output_fd, str, strlen(str) + 1);
            EOB_VERIFY(rv > 0);
        }
    }
    else
    {
        if(output_fd == STDOUT_FILENO)
            (void) fprintf(stdout, "%s\n", str);
        else
        {
            rv = write(output_fd, str, strlen(str) + 1);
            EOB_VERIFY(rv > 0);
        }
    }

    free(str);
    eob_device_close(dev);

    if(output_fd != STDOUT_FILENO)
        (void) close(output_fd);

    return (rv);
}

int iface_do_send(int argc, char **argv) {
    const char *src_filename;
    const char *device;
    int c;
    boolean_t force = B_FALSE;
    const char *src = NULL;
    const char *dst = NULL;
    const char *nodes_count = "0";

	/* check options */
	while ((c = getopt(argc, argv, "vFf:s:d:c:")) != -1) {
		switch (c) {
        case 'v':
            SET_VERBOSE();
            break;
        case 'F':
            force = B_TRUE;
            break;
        case 's':
            src = optarg;
            break;
        case 'd':
            dst = optarg;
            break;
		case 'f':
			src_filename = optarg;
			break;
        case 'c':
            nodes_count = optarg;
            break;
		case ':':
			(void) fprintf(stderr, ("missing argument for '%c' option\n"), optopt);
			goto badusage;
		case '?':
			(void) fprintf(stderr, "invalid option '%c'\n", optopt);
			goto badusage;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2) {
		(void) fprintf(stderr, "missing device name argument\n");
		goto badusage;
	}

    if(!src)
    {
        (void) fprintf(stderr, "missing source node id\n");
        goto badusage;
    }
    
    if(!dst)
    {
        (void) fprintf(stderr, "missing destination node id\n");
        goto badusage;
    }

	device = argv[1];

    return iface_send(src_filename, device, force, atoi(src), atoi(dst), atoi(nodes_count));

badusage:
    return SET_ERR(EOB_BADUSAGE);
}

int iface_hdr_dump(const char *device) {
    int fd;
    int rv;
    char *str = NULL;
    int flags = O_RDWR | O_SYNC;

    fd = open(device, flags);

    if(fd < 0)
        return SET_IO_ERR();

    rv = eob_hdr_dump(fd, &str);

    (void) close(fd);

    if(rv)
        return SET_ERR_TRACE(rv);

    fprintf(stdout, "Dump:\n%s\n", str);
    free(str);

    return (0);
}

int iface_do_dump(int argc, char **argv)
{
    int c;
    const char *device;
    boolean_t json_dump = B_FALSE;

	while ((c = getopt(argc, argv, "vj")) != -1) {
		switch (c) {
        case 'v':
            SET_VERBOSE();
            break;
        case 'j':
            json_dump = B_TRUE;
            break;
		case ':':
			(void) fprintf(stderr, "missing argument for '%c' option\n", optopt);
			goto badusage;
		case '?':
			(void) fprintf(stderr, "invalid option '%c'\n", optopt);
			goto badusage;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2) {
		(void) fprintf(stderr, "missing device name argument\n");
		goto badusage;
	}

	
    device = argv[1];

    if(json_dump == B_TRUE)
        NOT_IMPLEMENTED();

    c = iface_hdr_dump(device);

    if(c)
        return SET_ERR_TRACE(c);

    return (0);

badusage:
    return SET_ERR(EOB_BADUSAGE);
}

int iface_hdr_add(const char *device, int node_id, const char *ip, boolean_t force) {
    int fd;
    int rv;
    int flags = O_RDWR | O_SYNC;
    struct in_addr addr;
    struct eob_node_data_phys phys = {0};

    fd = open(device, flags);

    if(fd < 0)
        return SET_IO_ERR();

    rv = inet_aton(ip, &addr);

    if(rv < 0)
        return SET_IO_ERR();

    phys.ip_addr = addr.s_addr;
    phys.flags = 1;

    rv = eob_hdr_add_device(fd, node_id, &phys, force == B_TRUE);

    (void) close(fd);

    if(rv)
        return SET_ERR_TRACE(rv);

    return (0);
}

int iface_do_add(int argc, char **argv)
{
    int c;
    boolean_t force = B_FALSE;
    const char *ip = NULL;
    const char *node_id_str = NULL;
    const char *device = NULL;

	while ((c = getopt(argc, argv, "vFi:n:")) != -1) {
		switch (c) {
        case 'v':
            SET_VERBOSE();
            break;
        case 'F':
            force = B_TRUE;
            break;
        case 'n':
            node_id_str = optarg;
            break;
        case 'i':
            ip = optarg;
            break;
		case ':':
			(void) fprintf(stderr, "missing argument for '%c' option\n", optopt);
			goto badusage;
		case '?':
			(void) fprintf(stderr, "invalid option '%c'\n", optopt);
			goto badusage;
		}
	}

	argc -= optind;
	argv += optind;

    if(!ip) {
		(void) fprintf(stderr, "missing ip addr argument\n");
		goto badusage;
    }
    
    if(!node_id_str) {
		(void) fprintf(stderr, "missing <node-id> argument\n");
		goto badusage;
    }


	if (argc < 2) {
		(void) fprintf(stderr, "missing device name argument\n");
		goto badusage;
	}

	
    device = argv[1];

    c = iface_hdr_add(device, atoi(node_id_str), ip, force);

    if(c)
        return SET_ERR_TRACE(c);

    return (0);

badusage:
    return SET_ERR(EOB_BADUSAGE);
}

int iface_hdr_rem(const char *device, int node_id) {
    int fd;
    int rv;
    int flags = O_RDWR | O_SYNC;

    fd = open(device, flags);

    if(fd < 0)
        return SET_IO_ERR();

    rv = eob_hdr_rem_device(fd, node_id);

    (void) close(fd);

    if(rv)
        return SET_ERR_TRACE(rv);

    return (0);
}

int iface_do_remove(int argc, char **argv)
{
    int c;
    const char *node_id_str = NULL;
    const char *device = NULL;

	while ((c = getopt(argc, argv, "vn:")) != -1) {
		switch (c) {
        case 'v':
            SET_VERBOSE();
            break;
        case 'n':
            node_id_str = optarg;
            break;
		case ':':
			(void) fprintf(stderr, "missing argument for '%c' option\n", optopt);
			goto badusage;
		case '?':
			(void) fprintf(stderr, "invalid option '%c'\n", optopt);
			goto badusage;
		}
	}

	argc -= optind;
	argv += optind;

    if(!node_id_str) {
		(void) fprintf(stderr, "missing <node-id> argument\n");
		goto badusage;
    }


	if (argc < 2) {
		(void) fprintf(stderr, "missing device name argument\n");
		goto badusage;
	}

	
    device = argv[1];

    c = iface_hdr_rem(device, atoi(node_id_str));

    if(c)
        return SET_ERR_TRACE(c);

    return (0);

badusage:
    return SET_ERR(EOB_BADUSAGE);
}

int iface_hdr_deinit(const char *device) {
    int fd;
    int rv;
    int flags = O_RDWR | O_SYNC;

    fd = open(device, flags);

    if(fd < 0)
        return SET_IO_ERR();

    rv = eob_hdr_deinit(fd);

    (void) close(fd);

    if(rv)
        return SET_ERR_TRACE(rv);

    return (0);
}

int iface_hdr_init(const char *device, int nodes_count, int virt_blk_size, int phys_blk_size, boolean_t force) {
    int fd;
    int rv;
    int flags = O_RDWR | O_SYNC;

    fd = open(device, flags);

    if(fd < 0)
        return SET_IO_ERR();

    rv = eob_hdr_init(fd, nodes_count, virt_blk_size, phys_blk_size, force == B_TRUE);

    (void) close(fd);

    if(rv)
        return SET_ERR_TRACE(rv);

    return (0);
}

int iface_do_init(int argc, char **argv)
{
    int c;
    boolean_t force = B_FALSE;
    const char *virt = NULL;
    const char *phys = NULL;
    const char *nodes = NULL;
    const char *device = NULL;

    int virt_blk_size = 0;
    int phys_blk_size = 0;
    int nodes_count = 0;

	while ((c = getopt(argc, argv, "vF:V:P:c:")) != -1) {
		switch (c) {
        case 'v':
            SET_VERBOSE();
            break;
        case 'F':
            force = B_TRUE;
            break;
        case 'V':
            virt = optarg;
            break;
        case 'P':
            phys = optarg;
            break;
        case 'c':
            nodes = optarg;
            break;
		case ':':
			(void) fprintf(stderr, "missing argument for '%c' option\n", optopt);
			goto badusage;
		case '?':
			(void) fprintf(stderr, "invalid option '%c'\n", optopt);
			goto badusage;
		}
	}

	argc -= optind;
	argv += optind;

    if(virt)
        virt_blk_size = atoi(virt);
    
    if(phys)
        phys_blk_size = atoi(phys);

    if(nodes)
        nodes_count = atoi(nodes);

	if (argc < 2) {
		(void) fprintf(stderr, "missing device name argument\n");
		goto badusage;
	}

	
    device = argv[1];

    c = iface_hdr_init(device, nodes_count, virt_blk_size, phys_blk_size, force);

    if(c)
        return SET_ERR_TRACE(c);

    return (0);

badusage:
    return SET_ERR(EOB_BADUSAGE);
}

int iface_do_deinit(int argc, char **argv)
{
    int c;
    const char *device = NULL;

	while ((c = getopt(argc, argv, "v")) != -1) {
		switch (c) {
        case 'v':
            SET_VERBOSE();
            break;
		case ':':
			(void) fprintf(stderr, "missing argument for '%c' option\n", optopt);
			goto badusage;
		case '?':
			(void) fprintf(stderr, "invalid option '%c'\n", optopt);
			goto badusage;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2) {
		(void) fprintf(stderr, "missing device name argument\n");
		goto badusage;
	}

    device = argv[1];

    c = iface_hdr_deinit(device);

    if(c)
        return SET_ERR_TRACE(c);

    return (0);

badusage:
    return SET_ERR(EOB_BADUSAGE);
}

int iface_do_recv(int argc, char **argv)
{
    const char *dst_filename;
    const char *device;
    int c;
    boolean_t force = B_FALSE;
    const char *src = NULL;
    const char *nodes_count = "0";

	/* check options */
	while ((c = getopt(argc, argv, "vFf:s:c:")) != -1) {
		switch (c) {
        case 'v':
            SET_VERBOSE();
            break;
        case 'F':
            force = B_TRUE;
            break;
        case 's':
            src = optarg;
            break;
		case 'f':
			dst_filename = optarg;
			break;
        case 'c':
            nodes_count = optarg;
            break;
		case ':':
			(void) fprintf(stderr, ("missing argument for '%c' option\n"), optopt);
			goto badusage;
		case '?':
			(void) fprintf(stderr, "invalid option '%c'\n", optopt);
			goto badusage;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2) {
		(void) fprintf(stderr, "missing device name argument\n");
		goto badusage;
	}

    if(!src)
    {
        (void) fprintf(stderr, "missing source node id\n");
        goto badusage;
    }

	device = argv[1];

    return iface_recv(dst_filename, device, force, atoi(src), atoi(nodes_count));

badusage:
    return SET_ERR(EOB_BADUSAGE);
}

typedef c_vec_t(iface_command_t) cvec_iface_command_t;
static iface_command_t __iface_command_table[] = {
    {"test", iface_do_test, HELP_TEST},
    {"init", iface_do_init, HELP_INIT},
    {"deinit", iface_do_deinit, HELP_DEINIT},
    {"add",  iface_do_add,  HELP_ADD},
    {"remove", iface_do_remove, HELP_REMOVE},
    {"send", iface_do_send, HELP_SEND},
    {"recv", iface_do_recv, HELP_RECV},
    {"dump", iface_do_dump, HELP_DUMP},
};

cvec_iface_command_t iface_command_table = c_vec_from_static(__iface_command_table);

const char *get_usage(iface_help_t idx) {
    switch (idx) {
        case HELP_TEST:
            return "\ttest <device>\n";
        case HELP_SEND:
            return "\tsend [-v] [-F] [-c <nodes-count>] -s <node-id> -d <node-id> -f <file> <device>\n";
        case HELP_RECV:
            return "\trecv [-v] [-F] [-c <nodes-count>] -s <node-id>  -f <file> <device>\n";
        case HELP_INIT:
            return "\tinit [-v] [-F] [-c <nodes-count>] [-V <size> ] [-P <size>] <device>\n";
        case HELP_DEINIT:
            return "\tdeinit [-v] <device>\n";
        case HELP_ADD:
            return "\tadd [-v] [-F] -n <node-id> -i <ip>  <device>\n";
        case HELP_REMOVE:
            return "\tremove [-v] -n <node-id> <device>\n";
        case HELP_DUMP:
            return "\tdump [-v] [-j] <device>\n";
        case HELP_ALL:
            break;
    }

    UNREACHABLE_CODE();
    return NULL;
}

void usage(boolean_t requested, iface_command_t *cmd) {
	FILE *fp = requested ? stdout : stderr;
    iface_command_t *iter;

    if(!cmd) {
        fprintf(fp, "Avaiable commands: \n");
        c_vec_foreach_ptr(&iface_command_table, iter)
        {
            fprintf(fp, "%s", get_usage(iter->usage));
        }
        fprintf(fp, "\n");
    } else {
        fprintf(fp, "Usage: \n");
        fprintf(fp, "%s", get_usage(cmd->usage));
    }

	exit(requested ? 0 : 2);
}

int main(int argc, char **argv)
{
    iface_command_t *cmd;
    const char *cmd_name;
    int rv;
    void *ctx;
    
    if (argc == 1)
        usage(B_FALSE, NULL);

    cmd_name = argv[1];

	if ((strcmp(cmd_name, "-?") == 0) || strcmp(cmd_name, "--help") == 0 || strcmp(cmd_name, "-h") == 0)
		usage(B_TRUE, NULL);

    c_vec_find_by_str_field(&iface_command_table, cmd_name, cmd, name);

    if (c_vec_is_end(&iface_command_table, cmd)) {
        fprintf(stderr, "Command %s not found\n", cmd_name);
        usage(B_FALSE, NULL);
    }

    if(STR_EQ(cmd->name, "test"))
        return cmd->fn(argc, argv);

    rv = liboblock_init(&ctx);

    if(rv) {
        fprintf(stderr, "Unable to init lib ctx (%s)\n", eob_strerror(rv));
        rv = EXIT_FAILURE;
        return (rv);
    }

    rv = cmd->fn(argc, argv);

    liboblock_exit(ctx);

    if(rv == EOB_BADUSAGE)
    {
        usage(B_FALSE, cmd);
    } else if(rv) {
        fprintf(stderr, "Error: %s\n", eob_strerror(rv));
        rv = EXIT_FAILURE;
        return (rv);
    }

    return (rv);
}
