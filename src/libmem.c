#include "libmem.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "syscall.h"

static pthread_mutex_t mmvm_lock = PTHREAD_MUTEX_INITIALIZER;

int __alloc(struct pcb_t *caller, int vmaid, int rgid, addr_t size, addr_t *alloc_addr)
{
  pthread_mutex_lock(&mmvm_lock);
  struct vm_rg_struct rgnode;

  if (get_free_vmrg_area(caller, vmaid, size, &rgnode) == 0)
  {
    caller->krnl->mm->symrgtbl[rgid].rg_start = rgnode.rg_start;
    caller->krnl->mm->symrgtbl[rgid].rg_end = rgnode.rg_end;
    *alloc_addr = rgnode.rg_start;
    pthread_mutex_unlock(&mmvm_lock);
    return 0;
  }

  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  int inc_sz = PAGING_PAGE_ALIGNSZ(size);
  int old_sbrk = cur_vma->sbrk;

  struct sc_regs regs;
  regs.a1 = SYSMEM_INC_OP;
  regs.a2 = vmaid;
  regs.a3 = inc_sz;

  if (syscall(caller->krnl, caller->pid, 17, &regs) == -1)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  caller->krnl->mm->symrgtbl[rgid].rg_start = old_sbrk;
  caller->krnl->mm->symrgtbl[rgid].rg_end = old_sbrk + size;
  *alloc_addr = old_sbrk;

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

int __free(struct pcb_t *caller, int vmaid, int rgid)
{
  pthread_mutex_lock(&mmvm_lock);

  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  struct vm_rg_struct *rgnode = get_symrg_byid(caller->krnl->mm, rgid);
  if (rgnode->rg_start == 0 && rgnode->rg_end == 0)
  {
    pthread_mutex_unlock(&mmvm_lock);
    return -1;
  }

  struct vm_rg_struct *free_rg = malloc(sizeof(struct vm_rg_struct));
  free_rg->rg_start = rgnode->rg_start;
  free_rg->rg_end = rgnode->rg_end;
  free_rg->rg_next = NULL;

  enlist_vm_freerg_list(caller->krnl->mm, free_rg);

  rgnode->rg_start = 0;
  rgnode->rg_end = 0;

  pthread_mutex_unlock(&mmvm_lock);
  return 0;
}

int pg_getpage(struct mm_struct *mm, int pgn, int *fpn, struct pcb_t *caller)
{
  uint32_t pte = pte_get_entry(caller, pgn);

  if (!PAGING_PAGE_PRESENT(pte))
  {
    addr_t vicpgn, swpfpn, vicfpn;

    if (find_victim_page(caller->krnl->mm, &vicpgn) == -1)
      return -1;

    uint32_t vic_pte = pte_get_entry(caller, vicpgn);
    vicfpn = PAGING_FPN(vic_pte);

    if (MEMPHY_get_freefp(caller->krnl->active_mswp, &swpfpn) == -1)
      return -1;

    __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);

    pte_set_swap(caller, vicpgn, 0, swpfpn);

    pte_set_fpn(caller, pgn, vicfpn);

    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn);
  }

  *fpn = PAGING_FPN(pte_get_entry(caller, pgn));
  return 0;
}

int pg_getval(struct mm_struct *mm, int addr, BYTE *data, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
  MEMPHY_read(caller->krnl->mram, phyaddr, data);
  return 0;
}

int pg_setval(struct mm_struct *mm, int addr, BYTE value, struct pcb_t *caller)
{
  int pgn = PAGING_PGN(addr);
  int off = PAGING_OFFST(addr);
  int fpn;

  if (pg_getpage(mm, pgn, &fpn, caller) != 0)
    return -1;

  int phyaddr = (fpn << PAGING_ADDR_FPN_LOBIT) + off;
  MEMPHY_write(caller->krnl->mram, phyaddr, value);
  return 0;
}

int enlist_vm_freerg_list(struct mm_struct *mm, struct vm_rg_struct *rg_elmt)
{
  struct vm_rg_struct *rg_node = mm->mmap->vm_freerg_list;
  if (rg_elmt->rg_start >= rg_elmt->rg_end)
    return -1;
  if (rg_node != NULL)
    rg_elmt->rg_next = rg_node;
  mm->mmap->vm_freerg_list = rg_elmt;
  return 0;
}

struct vm_rg_struct *get_symrg_byid(struct mm_struct *mm, int rgid)
{
  if (rgid < 0 || rgid > PAGING_MAX_SYMTBL_SZ)
    return NULL;
  return &mm->symrgtbl[rgid];
}

int liballoc(struct pcb_t *proc, addr_t size, uint32_t reg_index)
{
  addr_t addr;
  int val = __alloc(proc, 0, reg_index, size, &addr);
  if (val == -1)
    return -1;
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
#endif
  return val;
}

int libfree(struct pcb_t *proc, uint32_t reg_index)
{
  int val = __free(proc, 0, reg_index);
  if (val == -1)
    return -1;
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
#endif
  return val;
}

int libread(struct pcb_t *proc, uint32_t source, addr_t offset, uint32_t *destination)
{
  BYTE data;
  // Sửa lỗi: dùng proc->krnl->mm thay vì proc->mm
  int val = pg_getval(proc->krnl->mm, proc->krnl->mm->symrgtbl[source].rg_start + offset, &data, proc);
  *destination = (uint32_t)data;
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
#endif
  return val;
}

int libwrite(struct pcb_t *proc, BYTE data, uint32_t destination, addr_t offset)
{
  // Sửa lỗi: dùng proc->krnl->mm thay vì proc->mm
  int val = pg_setval(proc->krnl->mm, proc->krnl->mm->symrgtbl[destination].rg_start + offset, data, proc);
  if (val == -1)
    return -1;
#ifdef IODUMP
#ifdef PAGETBL_DUMP
  print_pgtbl(proc, 0, -1);
#endif
  MEMPHY_dump(proc->krnl->mram);
#endif
  return val;
}

int find_victim_page(struct mm_struct *mm, addr_t *retpgn)
{
  struct pgn_t *pg = mm->fifo_pgn;
  if (!pg)
    return -1;

  struct pgn_t *prev = NULL;
  while (pg->pg_next)
  {
    prev = pg;
    pg = pg->pg_next;
  }

  *retpgn = pg->pgn;

  if (prev)
    prev->pg_next = NULL;
  else
    mm->fifo_pgn = NULL;

  free(pg);
  return 0;
}

int get_free_vmrg_area(struct pcb_t *caller, int vmaid, int size, struct vm_rg_struct *newrg)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  struct vm_rg_struct *rgit = cur_vma->vm_freerg_list;
  struct vm_rg_struct *prev = NULL;

  if (rgit == NULL)
    return -1;

  while (rgit != NULL)
  {
    if (rgit->rg_start + size <= rgit->rg_end)
    {
      newrg->rg_start = rgit->rg_start;
      newrg->rg_end = rgit->rg_start + size;

      if (rgit->rg_start + size < rgit->rg_end)
      {
        rgit->rg_start += size;
      }
      else
      {
        if (prev)
          prev->rg_next = rgit->rg_next;
        else
          cur_vma->vm_freerg_list = rgit->rg_next;
        free(rgit);
      }
      return 0;
    }
    prev = rgit;
    rgit = rgit->rg_next;
  }
  return -1;
}