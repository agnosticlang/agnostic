/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org> */
.text
.globl __chkstk
__chkstk:
    push %rcx
    push %rax
    lea 24(%rsp), %rcx
    cmp $0x1000, %rax
    jb 2f
1:
    sub $0x1000, %rcx
    test %rcx, (%rcx)
    sub $0x1000, %rax
    cmp $0x1000, %rax
    ja 1b
2:
    sub %rax, %rcx
    test %rcx, (%rcx)
    pop %rax
    pop %rcx
    ret
