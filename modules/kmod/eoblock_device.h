#ifndef EOBLOCK_DEVICE_H_
#define EOBLOCK_DEVICE_H_
/******************************************************************************/
/******************************************************************************/
struct eob_device;
typedef struct eob_device edev_t;


int eob_device_open(const char *path, const char *pname, edev_t **pres);
int eob_device_close(edev_t *pres);

#if 0
int add_eob_device_impl(const char *path, int node_id, struct eob_device **pres);
int remove_eob_device_impl(struct eob_device *dev);
#endif
/******************************************************************************/
#endif /* EOBLOCK_DEVICE_H_ */
