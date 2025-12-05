#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/* * TODO: PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

/* * init_pte - Initialize PTE entry
 */
int init_pte(uint32_t *pte,
             int pre,       // present
             addr_t fpn,    // FPN
             int drt,       // dirty
             int swp,       // swap
             int swptyp,    // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0)
  {
    if (swp == 0)
    { // Non swap ~ page online
      if (fpn == 0)
        return -1; // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }
  return 0;
}

/* * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  uint32_t *pte = &caller->krnl->mm->pgd[pgn];

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK); // De cho biet la co data (du o swap)
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/* * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  uint32_t *pte = &caller->krnl->mm->pgd[pgn];

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}

/* * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller, addr_t addr, int pgnum,
                       struct framephy_struct *frames, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *fpit = frames;
  int pgn = PAGING_PGN(addr);

  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING_PAGESZ;

  for (int i = 0; i < pgnum; i++)
  {
    if (fpit == NULL)
      return -1;
    pte_set_fpn(caller, pgn + i, fpit->fpn);
    enlist_pgn_node(&caller->krnl->mm->fifo_pgn, pgn + i);
    fpit = fpit->fp_next;
  }
  return 0;
}

/* * alloc_pages_range - allocate req_pgnum of frame in ram
 */
addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  int fpn;
  struct framephy_struct *newfp_str;
  *frm_lst = NULL;

  for (int i = 0; i < req_pgnum; i++)
  {
    if (MEMPHY_get_freefp(caller->krnl->mram, &fpn) == 0)
    {
      newfp_str = malloc(sizeof(struct framephy_struct));
      newfp_str->fpn = fpn;
      newfp_str->owner = caller->krnl->mm;
      newfp_str->fp_next = *frm_lst;
      *frm_lst = newfp_str;
    }
    else
    {
      return -3000; // Out of memory
    }
  }
  return 0;
}

/* * vm_map_ram - do the mapping all vm are to ram storage device
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  int ret_alloc;

  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc < 0 && ret_alloc != -3000)
    return -1;
  if (ret_alloc == -3000)
    return -3000;

  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);
  return 0;
}

/* * init_mm - Initialize a empty Memory Management instance
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));

  // Cấp phát bảng trang phẳng cho 32-bit (chỉ dùng pgd)
  mm->pgd = malloc(NUM_PAGES * sizeof(uint32_t));
  for (int i = 0; i < NUM_PAGES; i++)
    mm->pgd[i] = 0;

  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  vma0->vm_next = NULL;
  vma0->vm_mm = mm;
  mm->mmap = vma0;

  return 0;
}