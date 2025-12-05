#include "string.h"
#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

struct vm_rg_struct *get_vm_area_node_at_brk(struct pcb_t *caller, int vmaid, addr_t size, addr_t alignedsz)
{
  struct vm_rg_struct *newrg;
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);

  newrg = malloc(sizeof(struct vm_rg_struct));

  newrg->rg_start = cur_vma->sbrk;
  newrg->rg_end = cur_vma->sbrk + alignedsz;
  newrg->rg_next = NULL;

  return newrg;
}

int validate_overlap_vm_area(struct pcb_t *caller, int vmaid, addr_t vmastart, addr_t vmaend)
{
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  if (caller->krnl->mm->mmap == NULL)
    return -1;

  struct vm_area_struct *vma = caller->krnl->mm->mmap;
  while (vma != NULL)
  {
    if (vma != cur_vma)
    {
      // Kiểm tra chồng lấn: start1 < end2 && start2 < end1
      if (vmastart < vma->vm_end && vma->vm_start < vmaend)
        return -1;
    }
    vma = vma->vm_next;
  }
  return 0;
}

int inc_vma_limit(struct pcb_t *caller, int vmaid, addr_t inc_sz)
{
  struct vm_rg_struct *newrg = malloc(sizeof(struct vm_rg_struct));
  int inc_amt = (int)inc_sz;
  int incnumpage = inc_amt / PAGING_PAGESZ;
  struct vm_rg_struct *area = get_vm_area_node_at_brk(caller, vmaid, inc_sz, inc_amt);
  struct vm_area_struct *cur_vma = get_vma_by_num(caller->krnl->mm, vmaid);
  int old_end = cur_vma->vm_end;

  if (validate_overlap_vm_area(caller, vmaid, area->rg_start, area->rg_end) < 0)
  {
    free(newrg);
    return -1;
  }

  if (vm_map_ram(caller, area->rg_start, area->rg_end,
                 old_end, incnumpage, newrg) < 0)
  {
    free(newrg);
    return -1;
  }

  cur_vma->sbrk += inc_sz;
  cur_vma->vm_end += inc_sz;
  free(area);

  return 0;
}

int __mm_swap_page(struct pcb_t *caller, addr_t vicfpn, addr_t swpfpn)
{
  __swap_cp_page(caller->krnl->mram, vicfpn, caller->krnl->active_mswp, swpfpn);
  return 0;
}