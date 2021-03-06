/**
 * io - do basic IO on serial port, mainly to support the klib
 *
 * Copyright (c) 2013
 *  Benjamin Vanheuverzwijn <bvanheu@gmail.com>
 *
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of its contributors may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "../io.h"

int io_putchar(int c) {
    asm("li     %%r4, 0\n\t"
        "lis    %%r4, 0x4000\n\t"
        "loop_transmit_buffer_ready:\n\t"
        "lbz    %%r5, 0(%%r4)\n\t"
        "andi.  %%r5, %%r5, 0x04\n\t"
        "beq    0, loop_transmit_buffer_ready\n\t"
        "stb    %[character], 9(%%r4)\n\t"
        : // output list
        : [character] "r" (c) // input list
        : "r4", "r5" // clobber list
        );

    return c;
}

int io_puts(const char *s) {
    while(*s != '\0') {
        io_putchar(*(s++));
    }

    return 1;
}

int io_getchar(void) {
    // TODO - fill me
    return 0;
}

