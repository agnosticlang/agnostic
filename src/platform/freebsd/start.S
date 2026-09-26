/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org> */
.text
.global _start
_start:
    xor %ebp, %ebp
    mov %rsp, %r12
    and $-16, %rsp
    mov (%r12), %rdi
    lea 8(%r12), %rsi
    mov %rdi, agn_argc(%rip)
    mov %rsi, agn_argv(%rip)
    call main
    mov %eax, %edi
    mov $1, %eax
    syscall
    hlt

.data
.global agn_argc
agn_argc: .quad 0
.global agn_argv
agn_argv: .quad 0

.set NT_FREEBSD_ABI_TAG, 1
.set FREEBSD_ABI_OSREL, 1400000

.section .note.tag,"a",@note
.p2align 2
.long 2f - 1f
.long 4f - 3f
.long NT_FREEBSD_ABI_TAG
1:  .asciz "FreeBSD"
2:  .p2align 2
3:  .long FREEBSD_ABI_OSREL
4:

.section .note.GNU-stack,"",@progbits
