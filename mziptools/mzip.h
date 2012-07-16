/**
 * Copyright benjamin vanheuverzwijn (c) 2012
 *
 * TODO
 * ~~~~
 * - My guess is that a MZIP file may contains more than one segment but the
 *   bootloader (rommon) doesn't support it. I only have one file to test the
 *   MZIP format so it's hard to figure out.
 */

#ifndef __MZIP_H
#define __MZIP_H

#include <stdint.h>

//
// MZIP object
//
struct mzip {
    int fd;
    const char *filename;
    struct mzip_header *header;
    uint8_t *segment;
    size_t segment_buffer_size;
};

typedef struct mzip mzip_t;

//
// MZIP header
//
struct mzip_header {
    uint32_t magic;
    uint32_t version;
    uint32_t entry_point;
    uint32_t unknown1; // should be something like number of segments
    uint32_t unknown2; // ...
    uint32_t delimiter[8];
    uint16_t segment_crc16;
    uint16_t header_crc16;
    uint32_t header_size;
    uint32_t load_address;
    uint32_t segment_type;
    uint32_t segment_compressed_size;
    uint32_t segment_size;
    uint32_t memory_image_size;
    uint32_t delimiter2[8];
};

typedef struct mzip_header mzip_header_t;

// Little-endian
#define MZIP_HEADER_MAGIC       0x4D5A4950
#define MZIP_HEADER_VERSION_1   0x1

#define MZIP_SEGMENT_TYPE_UNKNOWN   0x0
#define MZIP_SEGMENT_TYPE_PKZIP     0x1

//
// MZIP header
//
mzip_header_t *mzip_header_new();

void mzip_header_free(mzip_header_t *m);


//
// MZIP file
//

// Read a mzip file
mzip_t *mzip_open(const char *filename);

void mzip_close(mzip_t *m);

mzip_header_t *mzip_get_header(mzip_t *m);

uint8_t *mzip_get_segment(mzip_t *m, uint8_t *buffer, size_t size);

// Create a new mzip file
mzip_t *mzip_create(void);

mzip_t *mzip_set_entry_point(mzip_t *m, uint32_t entry_point);

mzip_t *mzip_add_segment(mzip_t *m, uint32_t load_address, uint8_t *segment, size_t segment_size, size_t memory_image_size);

mzip_t *mzip_compress_segment(mzip_t *m);

mzip_t *mzip_save(mzip_t *m, const char *filename);

void mzip_destroy(mzip_t *m);

#endif

