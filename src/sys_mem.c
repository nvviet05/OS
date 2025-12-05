/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

#include "os-mm.h"
#include "syscall.h"
#include "libmem.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

#ifdef MM64
#include "mm64.h"
#else
#include "mm.h"
#endif

int __sys_memmap(struct krnl_t *krnl, uint32_t pid, struct sc_regs *regs)
{
    int memop = regs->a1;
    BYTE value;
    struct pcb_t *caller = NULL;

    /* TODO: Traverse proclist to terminate the proc */
    // Tìm PCB của tiến trình đang gọi syscall
    // Vì tiến trình đang thực thi lệnh syscall nên nó phải nằm trong running_list
    struct queue_t *running_q = krnl->running_list;

    if (running_q != NULL)
    {
        for (int i = 0; i < running_q->size; i++)
        {
            if (running_q->proc[i]->pid == pid)
            {
                caller = running_q->proc[i];
                break;
            }
        }
    }

    // Nếu không tìm thấy tiến trình hợp lệ
    if (caller == NULL)
    {
        return -1;
    }

    switch (memop)
    {
    case SYSMEM_MAP_OP:
        // Ánh xạ vùng nhớ (thường dùng cho khởi tạo)
        vmap_pgd_memset(caller, regs->a2, regs->a3);
        break;
    case SYSMEM_INC_OP:
        // Tăng giới hạn vùng nhớ (sbrk)
        inc_vma_limit(caller, regs->a2, regs->a3);
        break;
    case SYSMEM_SWP_OP:
        // Xử lý swap trang
        __mm_swap_page(caller, regs->a2, regs->a3);
        break;
    case SYSMEM_IO_READ:
        // Đọc từ bộ nhớ vật lý
        if (MEMPHY_read(caller->krnl->mram, regs->a2, &value) == 0)
        {
            regs->a3 = (arg_t)value; // Trả giá trị đọc được qua thanh ghi a3
        }
        else
        {
            return -1; // Lỗi đọc
        }
        break;
    case SYSMEM_IO_WRITE:
        // Ghi vào bộ nhớ vật lý
        if (MEMPHY_write(caller->krnl->mram, regs->a2, (BYTE)regs->a3) != 0)
        {
            return -1; // Lỗi ghi
        }
        break;
    default:
        printf("Memop code: %d\n", memop);
        return -1;
    }

    return 0;
}