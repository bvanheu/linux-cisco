/**
 * readmzip - read a mzip file a-la readelf
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
