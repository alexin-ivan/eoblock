#ifndef CHKSUM_H_
#define CHKSUM_H_
/******************************************************************************/
#include <sys/eob_types.h>
/******************************************************************************/
struct eob_header;
struct eob_chksum;


/******************************************************************************/
boolean_t eob_header_chksum_is_ok(const struct eob_header *hdr);

/******************************************************************************/
void eob_header_chksum_calc(struct eob_header *hdr);

/******************************************************************************/
void eob_chcksum_calc(const void *data, size_t len, struct eob_chksum *out);

/******************************************************************************/
#endif /* CHKSUM_H_ */
