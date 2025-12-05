/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "syscall.h"
#include "common.h"

// Định nghĩa thủ công các prototype thay vì dùng macro phức tạp
extern int __sys_listsyscall(struct krnl_t *, uint32_t, struct sc_regs *);
extern int __sys_memmap(struct krnl_t *, uint32_t, struct sc_regs *);

/*
 * The sys_call_table[] is used for system calls, but to know the system
 * call address.
 */
const char *sys_call_table[] = {
    "0-sys_listsyscall",
    "17-sys_memmap",
};

const int syscall_table_size = sizeof(sys_call_table) / sizeof(char *);

int __sys_ni_syscall(struct krnl_t *krnl, struct sc_regs *regs)
{
   /*
    * DUMMY systemcall
    */
   return 0;
}

int syscall(struct krnl_t *krnl, uint32_t pid, uint32_t nr, struct sc_regs *regs)
{
   // Chuyển đổi thủ công thay vì dùng file .lst để tránh lỗi include
   switch (nr)
   {
   case 0:
      return __sys_listsyscall(krnl, pid, regs);
   case 17:
      return __sys_memmap(krnl, pid, regs);
   default:
      return __sys_ni_syscall(krnl, regs);
   }
};