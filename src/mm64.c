#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#if defined(MM64)

int init_pte(uint64_t *pte,
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

int get_pd_from_address(addr_t addr, uint64_t *pgd, uint64_t *p4d, uint64_t *pud, uint64_t *pmd, uint64_t *pt)
{
  /* Trích xuất chỉ số từ địa chỉ 64-bit */
  *pgd = PAGING64_ADDR_PGD(addr);
  *p4d = PAGING64_ADDR_P4D(addr);
  *pud = PAGING64_ADDR_PUD(addr);
  *pmd = PAGING64_ADDR_PMD(addr);
  *pt = PAGING64_ADDR_PT(addr);
  return 0;
}

int get_pd_from_pagenum(addr_t pgn, uint64_t *pgd, uint64_t *p4d, uint64_t *pud, uint64_t *pmd, uint64_t *pt)
{
  addr_t addr = pgn << PAGING64_ADDR_PT_LOBIT; // Chuyển PGN sang địa chỉ (giả định offset = 0)
  return get_pd_from_address(addr, pgd, p4d, pud, pmd, pt);
}

/* * Helper function: Allocate next level table if not present
 */
uint64_t *get_next_level(uint64_t *current_entry)
{
  if (*current_entry == 0)
  {
    // Chưa có bảng con, cấp phát mới (512 entries cho mỗi level 9-bit)
    uint64_t *new_table = malloc(512 * sizeof(uint64_t));
    for (int i = 0; i < 512; i++)
      new_table[i] = 0;
    // Lưu địa chỉ bảng con vào entry hiện tại (ép kiểu)
    *current_entry = (uint64_t)new_table;
  }
  return (uint64_t *)(*current_entry);
}

int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  uint64_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

  // Traversal and allocation on demand
  uint64_t *pgd_table = caller->krnl->mm->pgd;
  uint64_t *p4d_table = get_next_level(&pgd_table[pgd_idx]);
  uint64_t *pud_table = get_next_level(&p4d_table[p4d_idx]);
  uint64_t *pmd_table = get_next_level(&pud_table[pud_idx]);
  uint64_t *pt_table = get_next_level(&pmd_table[pmd_idx]);

  uint64_t *pte = &pt_table[pt_idx];

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

int pte_set_fpn(struct pcb_t *caller, addr_t pgn, addr_t fpn)
{
  uint64_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

  uint64_t *pgd_table = caller->krnl->mm->pgd;
  uint64_t *p4d_table = get_next_level(&pgd_table[pgd_idx]);
  uint64_t *pud_table = get_next_level(&p4d_table[p4d_idx]);
  uint64_t *pmd_table = get_next_level(&pud_table[pud_idx]);
  uint64_t *pt_table = get_next_level(&pmd_table[pmd_idx]);

  uint64_t *pte = &pt_table[pt_idx];

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}

uint64_t pte_get_entry(struct pcb_t *caller, addr_t pgn)
{
  uint64_t pgd_idx, p4d_idx, pud_idx, pmd_idx, pt_idx;
  get_pd_from_pagenum(pgn, &pgd_idx, &p4d_idx, &pud_idx, &pmd_idx, &pt_idx);

  uint64_t *pgd_table = caller->krnl->mm->pgd;
  if (!pgd_table[pgd_idx])
    return 0;

  uint64_t *p4d_table = (uint64_t *)pgd_table[pgd_idx];
  if (!p4d_table[p4d_idx])
    return 0;

  uint64_t *pud_table = (uint64_t *)p4d_table[p4d_idx];
  if (!pud_table[pud_idx])
    return 0;

  uint64_t *pmd_table = (uint64_t *)pud_table[pud_idx];
  if (!pmd_table[pmd_idx])
    return 0;

  uint64_t *pt_table = (uint64_t *)pmd_table[pmd_idx];
  return pt_table[pt_idx];
}

int vmap_page_range(struct pcb_t *caller, addr_t addr, int pgnum,
                    struct framephy_struct *frames, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *fpit = frames;
  int pgn = PAGING_PGN(addr); // Macro PAGING_PGN có thể cần chỉnh lại cho 64-bit nếu chưa đúng

  /* Cập nhật trong mm64.h: #define PAGING_PGN(x) GETVAL(x,PAGING_PGN_MASK,PAGING_ADDR_PGN_LOBIT)
     Nhưng PGN ở đây là số trang logic liên tiếp */

  ret_rg->rg_start = addr;
  ret_rg->rg_end = addr + pgnum * PAGING64_PAGESZ;

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
      return -3000;
    }
  }
  return 0;
}

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

int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));

  // Cấp phát bảng PGD (Level 5) - 512 entries
  mm->pgd = malloc(512 * sizeof(uint64_t));
  for (int i = 0; i < 512; i++)
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

// Các hàm khác như print_pgtbl cần được cập nhật để duyệt cây 5 cấp (tùy chọn)
int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
  // Traverse and print valid entries (simplified)
  return 0;
}

#endif