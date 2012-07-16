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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <zip.h>

#include "mzip.h"
#include "crc16.h"

static inline void mzip_header_host_to_bigendian(mzip_header_t *header) {
    header->magic = htobe32(header->magic);
    header->version = htobe32(header->version);
    header->entry_point = htobe32(header->entry_point);
    header->unknown1 = htobe32(header->unknown1);
    header->unknown2 = htobe32(header->unknown2);
    header->segment_crc16 = htobe16(header->segment_crc16);
    header->header_crc16 = htobe16(header->header_crc16);
    header->header_size = htobe32(header->header_size);
    header->load_address = htobe32(header->load_address);
    header->segment_type = htobe32(header->segment_type);
    header->segment_compressed_size = htobe32(header->segment_compressed_size);
    header->segment_size = htobe32(header->segment_size);
    header->memory_image_size = htobe32(header->memory_image_size);
}

static inline void mzip_header_bigendian_to_host(mzip_header_t *header) {
    header->magic = be32toh(header->magic);
    header->version = be32toh(header->version);
    header->entry_point = be32toh(header->entry_point);
    header->unknown1 = be32toh(header->unknown1);
    header->unknown2 = be32toh(header->unknown2);
    header->segment_crc16 = be16toh(header->segment_crc16);
    header->header_crc16 = be16toh(header->header_crc16);
    header->header_size = be32toh(header->header_size);
    header->load_address = be32toh(header->load_address);
    header->segment_type = be32toh(header->segment_type);
    header->segment_compressed_size = be32toh(header->segment_compressed_size);
    header->segment_size = be32toh(header->segment_size);
    header->memory_image_size = be32toh(header->memory_image_size);
}

//
// MZIP header
//
mzip_header_t *mzip_header_new() {
    mzip_header_t *m = (mzip_header_t *)malloc(sizeof(mzip_header_t));

    memset((void *)m, '\x00', sizeof(mzip_header_t));

    return m;
}

void mzip_header_free(mzip_header_t *m) {
    if (m != NULL) {
        free(m);
    }
}


//
// MZIP file
//
mzip_t *mzip_open(const char *filename) {
    ssize_t bytes_read;
    int fd;

    mzip_t *m = (mzip_t *)malloc(sizeof(mzip_t));
    m->header = (mzip_header_t *)malloc(sizeof(mzip_header_t));

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        // TODO - error
        return NULL;
    }

    bytes_read = read(fd, (void *)m->header, sizeof(mzip_header_t));
    if (bytes_read != sizeof(mzip_header_t)) {
        // TODO - error
    }

    mzip_header_bigendian_to_host(m->header);

    switch (m->header->segment_type) {
        case MZIP_SEGMENT_TYPE_UNKNOWN:
            m->segment_buffer_size = m->header->segment_size;
            break;
        case MZIP_SEGMENT_TYPE_PKZIP:
            m->segment_buffer_size = m->header->segment_compressed_size;
            break;
        default:
            // TODO - error unsupported segment type
            break;
    }

    m->segment = (uint8_t *)malloc(sizeof(m->segment_buffer_size));
    bytes_read = read(fd, (void *)m->segment, m->segment_buffer_size);
    if (bytes_read != m->segment_buffer_size) {
        // TODO - error
    }

    close(fd);

    return m;
}

/**
 * Free some memory
 */
void mzip_close(mzip_t *m) {
    if (m != NULL) {
        if (m->header != NULL) {
            free(m->header);
        }
        if (m->segment != NULL) {
            free(m->segment);
        }
        free(m);
    }
}

/**
 * Create a MZIP file
 */
mzip_t *mzip_create(void) {
    mzip_t *m = (mzip_t *)malloc(sizeof(mzip_t));
    m->header = mzip_header_new();

    m->header->magic = MZIP_HEADER_MAGIC;
    m->header->version = MZIP_HEADER_VERSION_1;
    // next two values must be 0x1 i don't know why.
    m->header->unknown1 = 0x1;
    m->header->unknown2 = 0x1;

    m->header->header_size = sizeof(mzip_header_t);

    return m;
}

mzip_t *mzip_set_entry_point(mzip_t *m, uint32_t entry_point) {
    m->header->entry_point = entry_point;
    return m;
}

/**
 * Add a MZIP segment
 */
mzip_t *mzip_add_segment(mzip_t *m, uint32_t load_address, uint8_t *segment, size_t size, size_t memory_image_size) {
    // Update header
    m->header->load_address = load_address;
    m->header->segment_type = MZIP_SEGMENT_TYPE_UNKNOWN;
    m->header->segment_compressed_size = 0x0;
    m->header->segment_size = size;
    m->header->memory_image_size = memory_image_size;
    // Keep track of the buffer
    m->segment = (uint8_t *)malloc(size);
    memcpy(m->segment, segment, size);
    m->segment_buffer_size = size;

    uint16_t crc16 = crc16_ccitt(((uint8_t *)m->header)+0x38, sizeof(mzip_header_t)-0x38, 0x0);
    m->header->segment_crc16 = crc16_ccitt(m->segment, m->segment_buffer_size, crc16);

    return m;
}

/**
 * Compress the segment to save some space
 * XXX - actually you *must* compress the segment to boot it
 */
mzip_t *mzip_compress_segment(mzip_t *m) {
    int fd;
    size_t bytes_read;
    struct stat f_stat;
    struct zip *archive;
    struct zip_source *source_buffer;
    int error;
    char tmp_filename[16];

    //
    // Create temp file
    //
    strncpy(tmp_filename, "/tmp/mzipXXXXXX", 16);
    fd = mkstemp(tmp_filename);

    //
    // Create archive
    //
    archive = zip_open(tmp_filename, ZIP_CREATE, &error);
    if (archive == NULL) {
        // TODO - error
        fprintf(stderr, "zip_open: error\n");
    }

    if ((source_buffer = zip_source_buffer(archive, m->segment, m->segment_buffer_size, 0)) == NULL) {
        // TODO - error
        fprintf(stderr, "zip_source_buffer: error\n");
    }

    if (zip_add(archive, "-", source_buffer) < 0) {
        // TODO _ error
        fprintf(stderr, "zip_add: error\n");
    }

    zip_close(archive);

    if (stat(tmp_filename, &f_stat)) {
        // TODO - error
        fprintf(stderr, "stat: error\n");
    }

    m->segment = (uint8_t *)malloc(f_stat.st_size);
    m->segment_buffer_size = f_stat.st_size;

    close(fd);

    fd = open(tmp_filename, O_RDONLY);
    bytes_read = read(fd, m->segment, m->segment_buffer_size);
    close(fd);

    if (bytes_read != m->segment_buffer_size) {
        // TODO - error
        fprintf(stderr, "read: did not read enough bytes (%d, should be %d)\n", bytes_read, m->segment_buffer_size);
    }

    m->header->segment_type = MZIP_SEGMENT_TYPE_PKZIP;
    m->header->segment_compressed_size = m->segment_buffer_size;

    mzip_header_host_to_bigendian(m->header);

    uint16_t crc16 = crc16_ccitt(((const uint8_t *)(m->header))+0x38, sizeof(mzip_header_t)-0x38, 0x0);
    crc16 = crc16_ccitt((const uint8_t *)(m->segment), m->segment_buffer_size, crc16);

    mzip_header_bigendian_to_host(m->header);

    m->header->segment_crc16 = crc16;

    return m;
}

/**
 * Save a created MZIP file on disk
 */
mzip_t *mzip_save(mzip_t *m, const char *filename) {
    int fd;
    ssize_t bytes_written;

    fd = open(filename, O_CREAT|O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0) {
        fprintf(stderr, "mzip_save: unable to open %s\n", filename);
        return m;
    }

    mzip_header_host_to_bigendian(m->header);

    m->header->header_crc16 = htobe16(crc16_ccitt((const uint8_t *)(m->header), 0x36, 0x0));

    bytes_written = write(fd, (uint8_t *)m->header, sizeof(mzip_header_t));
    if (bytes_written != sizeof(mzip_header_t)) {
        // TODO - error not enough bytes written
        fprintf(stderr, "mzip_save: not enough bytes written (%d, should be %d)\n", bytes_written, sizeof(mzip_header_t));
    }

    bytes_written = write(fd, m->segment, m->segment_buffer_size);
    if (bytes_written != m->segment_buffer_size) {
        // TODO - error
        fprintf(stderr, "mzip_save: not enough bytes written (%d, should be %d)\n", bytes_written, m->segment_buffer_size);
    }

    close(fd);

    mzip_header_bigendian_to_host(m->header);

    return m;
}

/**
 * Destroy a created MZIP file
 */
void mzip_destroy(mzip_t *m) {
    if (m != NULL) {
        if (m->header != NULL) {
            mzip_header_free(m->header);
        }
        if (m->segment != NULL) {
            free(m->segment);
        }
        free(m);
    }
}

/**
 * Get the MZIP file segment
 */
uint8_t *mzip_get_segment(mzip_t *m, uint8_t *buffer, size_t size) {
    memcpy(buffer, m->segment, size);
    return buffer;
}

/**
 * Get the MZIP file header
 */
mzip_header_t *mzip_get_header(mzip_t *m) {
    return m->header;
}

