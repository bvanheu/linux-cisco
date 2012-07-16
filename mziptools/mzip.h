/**
 * Copyright (c) 2012, Benjamin Vanheuverzwijn <bvanheu@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
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

