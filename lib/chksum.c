#include <chksum.h>
#include <liboblock.h>
#include <zlib.h>

#undef MODULE
#define MODULE "eob:chksum: "

/******************************************************************************/
boolean_t eob_header_chksum_is_ok(const struct eob_header *hdr)
{
    size_t size = sizeof(hdr->h);
    const void *data = &hdr->h;
    struct eob_chksum s = {0};

    s.value = crc32(0, data, size);

    if (s.value == hdr->chksum.value)
        return (B_TRUE);

    EOB_LOG(EOB_DEBUG, "Checksum mismatch: %lu <> %lu", s.value, hdr->chksum.value);
    return (B_FALSE);
}

/******************************************************************************/
void eob_header_chksum_calc(struct eob_header *hdr)
{
    size_t size = sizeof(hdr->h);
    const void *data = &hdr->h;
    struct eob_chksum s = {0};

    s.value = crc32(0, data, size);

    hdr->chksum.value = s.value;

    EOB_LOG(EOB_DEBUG, "Checksum value: %lu",hdr->chksum.value);
}

/******************************************************************************/
void eob_chcksum_calc(const void *data, size_t len, struct eob_chksum *out)
{
    out->value =  (uint64_t) crc32(0, data, len);
}

/******************************************************************************/
