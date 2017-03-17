/******************************************************************************/
// #include "eoblock_device.h"
#include "eoblock_sysfs.h"
#define EOB_MODULE_NAME "eob_sysfs: "
#include "eoblock_debug.h"
#include "eoblock_device.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
/*#include <linux/etherdevice.h>*/
/*#include <linux/netdevice.h>*/
#include <linux/version.h>
/*#include <linux/rbtree.h>*/

#include <linux/percpu.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/genhd.h>
#include <linux/bio.h>
#include <linux/bug.h>
#include <linux/list.h>

#include <linux/etherdevice.h>
#include <linux/netdevice.h>

#include <linux/ip.h>
#include <linux/tcp.h>

#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
/*#include <linux/module.h>*/
/*#include <linux/init.h>*/
#include <linux/ctype.h>
/*#include <linux/slab.h>*/
#include <linux/kthread.h>

#include <eoblock_defs.h>
#include <sys/eob_log.h>
#include <eoblock.h>
#include <sys/eob_debug.h>

#define MAX_EOB_DEVICES 32
#define EOBLOCK_PATH

/*struct eob_ctx {*/
    /*struct eob_device *devices[MAX_EOB_DEVICES];*/
    /*spinlock_t lock;*/
/*};*/

#define TRACE_EXIT_RES(X)
#define TRACE_ENTRY()
#define TRACE_EXIT()
#define TRACE_DBG(X, args...)
#define K_VERIFY(X)
#define PRINT_ERROR(X, args...) EOB_PK(KERN_ERR, X, args)

#define STATIC_FN static 
/******************************************************************************/
/*static struct eob_ctx *eob_ctx = NULL;*/

struct eoblock_iface_base
{
    char name[256];
    char device_name[256];
    struct kobject iface_kobj; /* iface driver sysfs entry */

    /*int tgtt_active_sysfs_works_count;*/
    /*struct completion *iface_kobj_release_cmpl;*/
    
    struct eob_device *dev;

    struct list_head iface_list_entry;
};

static LIST_HEAD(eoblock_ifaces);

static int eoblock_setup_id = 0;

/******************************************************************************/
static ssize_t eoblock_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *kobj_attr;

    kobj_attr = container_of(attr, struct kobj_attribute, attr);
    return kobj_attr->show(kobj, kobj_attr, buf);
}

/******************************************************************************/
static ssize_t eoblock_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    struct kobj_attribute *kobj_attr;

    kobj_attr = container_of(attr, struct kobj_attribute, attr);
    if (kobj_attr->store)
        return kobj_attr->store(kobj, kobj_attr, buf, count);
    else
        return -EIO;
}

/******************************************************************************/
const struct sysfs_ops eoblock_sysfs_ops = {
    .show = eoblock_show,
    .store = eoblock_store,
};


static struct kobj_type iface_ktype = {
    .sysfs_ops = &eoblock_sysfs_ops,
    /*.release = eoblock_iface_release,*/
};

/******************************************************************************/
#if 0
static void scst_iface_release(struct kobject *kobj)
{
    struct eoblock_iface *ibase;

    TRACE_ENTRY();

    ibase = container_of(kobj, struct eoblock_iface_base, tgtt_kobj);

    if (ibase->iface_kobj_release_cmpl)
        complete_all(ibase->iface_kobj_release_cmpl);

    TRACE_EXIT();
    return;
}
#endif

static void eoblock_sysfs_root_release(struct kobject *kobj)
{
    /*complete_all(&scst_sysfs_root_release_completion);*/
}

static ssize_t eoblock_setup_id_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int count;

    TRACE_ENTRY();

    count = sprintf(buf, "%d\n", eoblock_setup_id);

    TRACE_EXIT();
    return count;
}

static ssize_t eoblock_setup_id_store(struct kobject *kobj,
    struct kobj_attribute *attr, const char *buf, size_t count)
{
    int res;
    unsigned long val;

    TRACE_ENTRY();

    res = kstrtoul(buf, 0, &val);

    if (res != 0) {
        EOB_PK(KERN_ERR, "kstrtoul() for %s failed: %d ", buf, res);
        goto out;
    }

    eoblock_setup_id = val;
    EOB_PK(KERN_INFO, "Changed scst_setup_id to %x", eoblock_setup_id);

    res = count;

out:
    TRACE_EXIT_RES(res);
    return res;
}

static ssize_t eoblock_iface_mgmt_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    static const char help[] =
        "Usage: echo \"add iface_name <device, partition, file>\" >mgmt\n"
        "       echo \"del iface_name\" >mgmt\n";

    return scnprintf(buf, PAGE_SIZE, "%s", help);
}

/**
 * scst_get_next_lexem() - parse and return next lexem in the string
 *
 * Returns pointer to the next lexem from token_str skipping
 * spaces and '=' character and using them then as a delimeter. Content
 * of token_str is modified by setting '\0' at the delimeter's position.
 */
char *eoblock_get_next_lexem(char **token_str)
{
    char *p, *q;
    static const char blank = '\0';

    if ((token_str == NULL) || (*token_str == NULL))
        return (char *)&blank;

    for (p = *token_str; (*p != '\0') && (isspace(*p) || (*p == '=')); p++)
        ;

    for (q = p; (*q != '\0') && !isspace(*q) && (*q != '='); q++)
        ;

    if (*q != '\0')
        *q++ = '\0';

    *token_str = q;
    return p;
}

/**
 * scst_get_next_token_str() - parse and return next token
 *
 * This function returns pointer to the next token strings from input_str
 * using '\n', ';' and '\0' as a delimeter. Content of input_str is
 * modified by setting '\0' at the delimeter's position.
 */
char *eoblock_get_next_token_str(char **input_str)
{
    char *p = *input_str;
    int i = 0;

    while ((p[i] != '\n') && (p[i] != ';') && (p[i] != '\0'))
        i++;

    if (i == 0)
        return NULL;

    if (p[i] == '\0')
        *input_str = &p[i];
    else
        *input_str = &p[i+1];

    p[i] = '\0';

    return p;
}

static ssize_t eoblock_iface_dev_name_show(struct kobject *kobj,
    struct kobj_attribute *attr, char *buf)
{
    int pos = 0;
    struct eoblock_iface_base *iface;

    TRACE_ENTRY();

    iface = container_of(kobj, struct eoblock_iface_base, iface_kobj);

    K_VERIFY(strlen(iface->device_name) < PAGE_SIZE);

    pos = sprintf(buf, "%s\n", iface->device_name);

    TRACE_EXIT();
    return pos;
}

static struct kobject *eoblock_ifaces_kobj;
static struct kobj_attribute eoblock_iface_dev_name =
    __ATTR(dev_name, S_IRUGO, eoblock_iface_dev_name_show, NULL);

static int eoblock_add_iface_kobj(
    const char *iface_name,
    const char *device
)
{
    int res;
    struct eoblock_iface_base *iface;

    iface = kcalloc(1, sizeof(struct eoblock_iface_base), GFP_KERNEL);
    
    if (iface == NULL) {
        PRINT_ERROR("Unable to alloc %s.",  "eoblock_iface_base");
        res = -ENOMEM;
        goto do_exit;
    }

    if (strlen(iface_name) >= sizeof(iface->name))
    {
        res = -EINVAL;
        goto do_out_free;
    }

    strcpy(iface->name, iface_name);

    if (strlen(device) > sizeof(iface->name))
    {
        res = -EINVAL;
        goto do_out_free;
    }

    strcpy(iface->device_name, device);

    res = kobject_init_and_add(
        &iface->iface_kobj,
        &iface_ktype, 
        eoblock_ifaces_kobj, "%s", iface->name
    );

    if (res != 0) {
        PRINT_ERROR("Can't add iface %s to sysfs", iface->name);
        goto do_out_put_kobj;
    }

    res = sysfs_create_file(&iface->iface_kobj, &eoblock_iface_dev_name.attr);

    if (res != 0) {
        PRINT_ERROR("Can't add %s file to sysfs", "dev_name");
        goto do_out_del_kobj;
    }

    list_add_tail(&iface->iface_list_entry, &eoblock_ifaces);

    res = eob_device_open(device, iface_name, &iface->dev);

    if (res != 0) {
        PRINT_ERROR("Can't open device %s (dev=%s)", iface->name, iface->device_name);
        goto do_out_del_kobj;
    }

    EOB_PK(KERN_ERR, "Added device: %s from dev %s", iface_name, device);
    return (0);

do_exit:
    return res;

do_out_del_kobj:
    kobject_del(&iface->iface_kobj);
do_out_put_kobj:
    kobject_put(&iface->iface_kobj);

do_out_free:
    kfree(iface);
    goto do_exit;

}

static ssize_t eoblock_add_iface(const char *iface_name, char *params)
{
    int res;
    char *param, *p, *pp;
    const char *dev_name = NULL;
    bool dev_name_set = false;

    TRACE_ENTRY();

    while (1) {
        param = eoblock_get_next_token_str(&params);
        if (param == NULL)
            break;

        p = eoblock_get_next_lexem(&param);
        if (*p == '\0') {
            PRINT_ERROR("Syntax error at %s (iface %s)", param, iface_name);
            res = -EINVAL;
            goto out;
        }

        pp = eoblock_get_next_lexem(&param);
        if (*pp == '\0') {
            PRINT_ERROR("Parameter %s value missed for " "iface %s", p, iface_name);
            res = -EINVAL;
            goto out;
        }

        if (eoblock_get_next_lexem(&param)[0] != '\0') {
            PRINT_ERROR("Too many parameter's %s values " "(iface %s)", p, iface_name);
            res = -EINVAL;
            goto out;
        }

        if (!strcasecmp("dev", p)) {
            dev_name = pp;
            dev_name_set = true;
            continue;
        }

        PRINT_ERROR("Unknown parameter %s (target %s)", p, iface_name);
        res = -EINVAL;
        goto out;
    }

    if (!dev_name_set) {
        PRINT_ERROR("Missing parameter 'dev' " "(iface %s)", iface_name);
        res = -EINVAL;
        goto out;
    }

    res = eoblock_add_iface_kobj(iface_name, dev_name);

out:
    TRACE_EXIT_RES(res);
    return res;
}

static int eoblock_remove_iface(const char *name)
{
    struct eoblock_iface_base *iface;
    int found = 0;

    TRACE_ENTRY();

    list_for_each_entry(iface, &eoblock_ifaces, iface_list_entry) {
        if (strcmp(iface->name, name) == 0) {
            found = 1;
            break;
        }
    }

    if (!found) {
        PRINT_ERROR("Interface %s isn't registered", name);
        return -ENOENT;
    }

    eob_device_close(iface->dev);
    list_del(&iface->iface_list_entry);
    kobject_del(&iface->iface_kobj);
    kobject_put(&iface->iface_kobj);
    kfree(iface);
    EOB_PK(KERN_ERR, "REMOVE_IFACE: %s", name);
    return (0);
}

static int eoblock_iface_mgmt_cb(char *buffer)
{
    int res = 0;
    char *p, *pp, *iface_name;

    TRACE_ENTRY();

    TRACE_DBG("buffer %s", buffer);

    pp = buffer;
    p = eoblock_get_next_lexem(&pp);

    if (strcasecmp("add", p) == 0) {
        iface_name = eoblock_get_next_lexem(&pp);
        if (*iface_name == '\0') {
            EOB_PK(KERN_ERR, "%s", "Target name required");
            res = -EINVAL;
            goto out;
        }
        res = eoblock_add_iface(iface_name, pp);
    } else if (strcasecmp("del", p) == 0) {
        iface_name = eoblock_get_next_lexem(&pp);
        if (*iface_name == '\0') {
            EOB_PK(KERN_ERR, "%s", "Target name required");
            res = -EINVAL;
            goto out;
        }

        p = eoblock_get_next_lexem(&pp);
        if (*p != '\0')
            goto out_syntax_err;
        res = eoblock_remove_iface(iface_name);
    } else {
        EOB_PK(KERN_ERR, "Unknown action \"%s\"", p);
        res = -EINVAL;
        goto out;
    }

out:
    TRACE_EXIT_RES(res);
    return res;

out_syntax_err:
    EOB_PK(KERN_ERR, "Syntax error on \"%s\"", p);
    res = -EINVAL;
    goto out;
}

static ssize_t eoblock_iface_mgmt_store(struct kobject *kobj,
    struct kobj_attribute *attr, const char *buf, size_t count)
{
    int res;
    char *buffer;
    
    TRACE_ENTRY();

    buffer = kasprintf(GFP_KERNEL, "%.*s", (int)count, buf);

    if (buffer == NULL) {
        res = -ENOMEM;
        goto out;
    }

    res = eoblock_iface_mgmt_cb(buffer);
    
    if (res == 0)
        res = count;

    goto out_free;

out:
    TRACE_EXIT_RES(res);
    return res;

out_free:
    kfree(buffer);
    goto out;
}


static struct kobj_attribute eoblock_iface_mgmt =
    __ATTR(mgmt, S_IRUGO | S_IWUSR, eoblock_iface_mgmt_show,
           eoblock_iface_mgmt_store);

static struct kobj_attribute eoblock_setup_id_attr =
    __ATTR(setup_id, S_IRUGO | S_IWUSR, eoblock_setup_id_show,
           eoblock_setup_id_store);

static struct attribute *eoblock_sysfs_root_default_attrs[] = {
    &eoblock_setup_id_attr.attr,
    &eoblock_iface_mgmt.attr,
    NULL,
};
/******************************************************************************/
static struct kobj_type eoblock_sysfs_root_ktype = {
    .sysfs_ops = &eoblock_sysfs_ops,
    .release = eoblock_sysfs_root_release,
    .default_attrs = eoblock_sysfs_root_default_attrs,
};

/******************************************************************************/
static struct kobject eoblock_sysfs_root_kobj;

/******************************************************************************/
int __init eoblock_sysfs_init(void)
{
    int res;

    res = kobject_init_and_add(
        &eoblock_sysfs_root_kobj,
        &eoblock_sysfs_root_ktype, 
        kernel_kobj, "%s", "eoblock"
    );
    
    if (res != 0)
        goto eoblock_root_add_error;

    eoblock_ifaces_kobj = kobject_create_and_add("ifaces", &eoblock_sysfs_root_kobj);
    
    if (eoblock_ifaces_kobj == NULL)
        goto ifaces_kobj_error;

out:
    TRACE_EXIT_RES(res);
    return res;

ifaces_kobj_error:
    kobject_del(&eoblock_sysfs_root_kobj);

eoblock_root_add_error:
    kobject_put(&eoblock_sysfs_root_kobj);

    /*kthread_stop(sysfs_work_thread);*/

    if (res == 0)
        res = -EINVAL;

    goto out;

}

/******************************************************************************/
void __exit eoblock_sysfs_cleanup(void)
{
    kobject_del(eoblock_ifaces_kobj);
    kobject_put(eoblock_ifaces_kobj);

    kobject_del(&eoblock_sysfs_root_kobj);
    kobject_put(&eoblock_sysfs_root_kobj);
}

/******************************************************************************/
