#!/bin/bash

powerpc-elf-as -m403 -Wall  helloworld.s -o helloworld.o
powerpc-elf-ld --omagic --strip-all --discard-all --entry=_start -Ttext=3000 -o helloworld.elf helloworld.o
elf2mzip helloworld.elf helloworld.bin
