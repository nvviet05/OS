/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include "common.h"

/* Định nghĩa cấu trúc sc_regs */
struct sc_regs {
        arg_t a1;
        arg_t a2;
        arg_t a3;
        arg_t a4;
        arg_t a5;
        arg_t a6;

        uint32_t orig_ax;
        int32_t flags;
};

/* Khai báo các hàm và bảng hệ thống */
extern const char* sys_call_table[];
extern const int syscall_table_size;

/* libsyscall interface */
int __mm_swap_page(struct pcb_t *, addr_t , addr_t);
int libsyscall(struct pcb_t*, uint32_t, arg_t, arg_t, arg_t);
int syscall(struct krnl_t*, uint32_t, uint32_t, struct sc_regs*);
int __sys_ni_syscall(struct krnl_t*, struct sc_regs*);

#endif /* SYSCALL_H */