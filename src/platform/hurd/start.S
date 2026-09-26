/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org> */
.text
.global _start
_start:
    xor %ebp, %ebp
    mov %rsp, %rdi
    and $-16, %rsp
    call agn_hurd_start
    hlt

.section .note.GNU-stack,"",@progbits
