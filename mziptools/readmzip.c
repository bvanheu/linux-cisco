/**
 * readmzip - read a mzip file a-la readelf
 *
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <endian.h> // FIXME - linux only :(
#include <libgen.h>

#include "mzip.h"

int usage() {
    printf("Usage: readmzip input_file \n");
    return 0;
}

int main(int argc, char *argv[]) {
    mzip_t *mzip;
    mzip_header_t *header;

    if (argc != 2) {
        usage();
        return EXIT_FAILURE;
    }

    mzip = mzip_open(argv[1]);

    header = mzip_get_header(mzip);

    printf("MZIP file: %s\n", basename(argv[1]));
    printf("--\n");
    printf("Magic:\t\t\t\t0x%x (%s)\n", htobe32(header->magic), (char *)&(header->magic));
    printf("Version:\t\t\t0x%x\n", htobe32(header->version));
    printf("Entry point:\t\t\t0x%x\n", htobe32(header->entry_point));
    printf("Unknown 1:\t\t\t0x%x\n", htobe32(header->unknown1));
    printf("Unknown 2:\t\t\t0x%x\n", htobe32(header->unknown2));
    printf("Segment crc16:\t\t\t0x%x\n", htobe16(header->segment_crc16));
    printf("Header crc16:\t\t\t0x%x\n", htobe16(header->header_crc16));
    printf("Load address:\t\t\t0x%x\n", htobe32(header->load_address));
    printf("Segment type:\t\t\t0x%x (%s)\n", htobe32(header->segment_type), (htobe32(header->segment_type) == MZIP_SEGMENT_TYPE_PKZIP ? "PKZIP": "unknown"));
    printf("Segment compressed size:\t0x%x\n", htobe32(header->segment_compressed_size));
    printf("Segment size:\t\t\t0x%x\n", htobe32(header->segment_size));
    printf("Memory image size:\t\t0x%x\n", htobe32(header->memory_image_size));

    mzip_close(mzip);

    return EXIT_SUCCESS;
}
