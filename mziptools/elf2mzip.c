/**
 * elf2mzip - build a mzip file from elf a-la elf2flt
 */
#define _BSD_SOURCE

#include <endian.h> // FIXME - linux only :(
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <elf.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "mzip.h"

static inline void elf_header_to_littleendian(Elf32_Ehdr *elf) {
    elf->e_type = be16toh(elf->e_type);
    elf->e_machine = be16toh(elf->e_machine);
    elf->e_version = be32toh(elf->e_version);
    elf->e_entry = be32toh(elf->e_entry);
    elf->e_phoff = be32toh(elf->e_phoff);
    elf->e_shoff = be32toh(elf->e_shoff);
    elf->e_flags = be32toh(elf->e_flags);
    elf->e_ehsize = be16toh(elf->e_ehsize);
    elf->e_phentsize = be16toh(elf->e_phentsize);
    elf->e_phnum = be16toh(elf->e_phnum);
    elf->e_shentsize = be16toh(elf->e_shentsize);
    elf->e_shnum = be16toh(elf->e_shnum);
    elf->e_shstrndx = be16toh(elf->e_shstrndx);
}

static inline void elf_program_header_to_littleendian(Elf32_Phdr *ph) {
    ph->p_type = be32toh(ph->p_type);
    ph->p_flags = be32toh(ph->p_flags);
    ph->p_offset = be32toh(ph->p_offset);
    ph->p_vaddr = be32toh(ph->p_vaddr);
    ph->p_filesz = be32toh(ph->p_filesz);
    ph->p_memsz = be32toh(ph->p_memsz);
    ph->p_flags = be32toh(ph->p_flags);
    ph->p_align = be32toh(ph->p_align);
}

int usage() {
    printf("Usage: elf2mzip input_file output_file\n");
    return 0;
}

int main(int argc, char *argv[]) {
    Elf32_Ehdr elf;
    size_t bytes_read;
    uint8_t i;
    int fd;
    Elf32_Phdr *program_header_table;
    mzip_t *mzip = mzip_create();
    bool elf_in_bigendian = false;

    if (argc != 3) {
        usage();
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    bytes_read = read(fd, (uint8_t *)&elf, sizeof(Elf32_Ehdr));

    // Validate ELF file
    if (elf.e_ident[EI_MAG0] != ELFMAG0 || elf.e_ident[EI_MAG1] != ELFMAG1 || elf.e_ident[EI_MAG2] != ELFMAG2 || elf.e_ident[EI_MAG3] != ELFMAG3) {
        // TODO - error
        fprintf(stderr, "error - not an ELF file\n");
        return EXIT_FAILURE;
    }

    // Validate endianess
    if (elf.e_ident[EI_DATA] == ELFDATA2MSB) {
        elf_in_bigendian = true;
        elf_header_to_littleendian(&elf);
    }

    // Validate entry point
    if (elf.e_entry < 0x3000) {
        // TODO - error
        fprintf(stderr, "error - invalid entry point: %x (to be valid: 0x3000 < entry_point < 0x10000)", elf.e_entry);
        return EXIT_FAILURE;
    }
    printf("\tEntry point: 0x%x\n", elf.e_entry);

    // Validate at least one program header
    if (!elf.e_phnum) {
        // TODO - error
        fprintf(stderr, "error - no program header: %d\n", elf.e_phnum);
        return EXIT_FAILURE;
    }

    printf("\tProgram header segment: %d\n", elf.e_phnum);

    // Seek to program header tables
    if (lseek(fd, elf.e_phoff, SEEK_SET) < 0) {
        // TODO - error
        fprintf(stderr, "error - unable to seek to program header table\n");
        return EXIT_FAILURE;
    }

    program_header_table = (Elf32_Phdr *)malloc(elf.e_phnum * elf.e_phentsize);

    bytes_read = read(fd, program_header_table, elf.e_phnum * elf.e_phentsize);
    if (bytes_read != elf.e_phnum * elf.e_phentsize) {
        // TODO - error;
        fprintf(stderr, "error - unable to read program header table\n");
        return EXIT_FAILURE;
    }

    uint8_t *program_section;
    for (i=0; i < elf.e_phnum; i++) {
        if (elf_in_bigendian) {
            elf_program_header_to_littleendian(&(program_header_table[i]));
        }

        printf("\t\tPROGRAM SEGMENT %d\n", i);

        // Keep only LOAD program segment
        if (program_header_table[i].p_type != PT_LOAD) {
            printf("\t\tSkipping non PT_LOAD segment\n");
            continue;
        }

        printf("\t\tSegment type:\t\tPT_LOAD\n");
        printf("\t\tSegment load address:\t0x%x\n", program_header_table[i].p_vaddr);
        printf("\t\tSegment size:\t\t0x%x\n", program_header_table[i].p_filesz);
        printf("\t\tMemory image size:\t0x%x\n", program_header_table[i].p_memsz);

        if (lseek(fd, program_header_table[i].p_offset, SEEK_SET) < 0) {
            // TODO - error
            fprintf(stderr, "error - unable to seek to program_header_table\n");
            continue;
        }

        program_section = (uint8_t *)malloc(program_header_table[i].p_filesz);

        bytes_read = read(fd, program_section, program_header_table[i].p_filesz);
        if (bytes_read != program_header_table[i].p_filesz) {
            // TODO - error
            fprintf(stderr, "error - unable to read enough bytes\n");
            continue;
        }


        // XXX - For now, we only keep the first LOAD program segment
        break;
    }

    if (program_section == NULL) {
        // TODO - error
        fprintf(stderr, "error - no program segment PT_LOAD found\n");
        return EXIT_FAILURE;
    }

    mzip_add_segment(mzip, program_header_table[i].p_vaddr, program_section, program_header_table[i].p_filesz, program_header_table[i].p_memsz);
    mzip_compress_segment(mzip);
    mzip_set_entry_point(mzip, elf.e_entry);
    mzip_save(mzip, argv[2]);
    mzip_destroy(mzip);

    close(fd);

    return EXIT_SUCCESS;
}
