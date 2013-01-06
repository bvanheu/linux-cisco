# What is it?
    This is an attemp to build an ELF loader to make it easier to run 3rd party
    software on your hardware. This is somewhat a second stage bootloader.

# How to build?
    You will need a powerpc crosscompiler ready.
    Adjust the path to your powerpc crosscompiler in build/toolchain-powerpc.cmake
    and then:
    $ cd build
    $ cmake -DCMAKE_TOOLCHAIN_FILE=./toolchain-powerpc.cmake ../

# License?
    3-clause BSD license
