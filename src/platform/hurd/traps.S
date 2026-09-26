/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org> */
.text

.macro mach_trap name, number
.global \name
\name:
    mov $\number, %rax
    mov %rcx, %r10
    syscall
    ret
.endm

mach_trap agn_mach_msg_trap, -25
mach_trap agn_mach_reply_port, -26
mach_trap agn_mach_task_self, -28
mach_trap agn_vm_allocate, -65
mach_trap agn_vm_deallocate, -66
mach_trap agn_task_terminate, -69
mach_trap agn_mach_port_deallocate, -73

.section .note.GNU-stack,"",@progbits
