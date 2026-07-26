/* SPDX-License-Identifier: Apache-2.0 */
/* SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org> */
.text
.global _start
_start:
    xor %ebp, %ebp
    and $-16, %rsp
    call main
    mov %eax, %edi
    mov $1, %eax
    syscall
    hlt

.section .note.GNU-stack,"",@progbits
