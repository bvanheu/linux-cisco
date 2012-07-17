.data
helloworld:
    .ascii  "Hello Cisco!\r\n"

.text
.global _start

#
# MAIN
# r3: boot_flags
# r4: filename
#
_start:
    mflr    %r0
    stw     %r0, 8(%r1)

    lis     %r10, 0x0
    li      %r10, 0x0
    lis     %r10, 0x4000

    lis     %r3,helloworld@ha
    addi    %r3,%r3,helloworld@l
    li      %r4, 14
    bl      send_string

    # ret
    # cannot ret to the bootloader

#
# SEND STRING
# r3: string
# r4: length
#
send_string:
    mflr    %r0,
    stw     %r0, 8(%r1)

    # init var
    mr      %r6,%r3 # string
    mr      %r7,%r4 # length
    li      %r8,0   # counter

    # send loop
    loop:
    lbzx    %r3, %r6, %r8
    bl      send_char
    addi    %r8,%r8,1
    cmp     0,%r8,%r7
    blt     loop

    # ret
    end:
    lwz     %r0, 8(%r1)
    mtlr    %r0
    blr

#
# SEND CHAR
# r3: character
#
send_char:
    mflr    %r0
    stw     %r0, 8(%r1)

    # wait for transmit buffer ready
    loop_transmit_buffer_ready:
    lbz     %r11, 0(%r10)
    andi.   %r11, %r11, 0x04
    beq     0,loop_transmit_buffer_ready

    # put  char in transmission buffer
    transmit_char:
    stb     %r3, 9(%r10)

    # ret
    ret:
    lwz     %r0, 8(%r1)
    mtlr    %r0
    blr
