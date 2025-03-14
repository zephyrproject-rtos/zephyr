/*
* Created for ECE595, ASE, Spring 2025
* Kallie, Pranav, Alex, & James
*
*/


/*----------------- Includes ---------------*/
#include <zephyr/kernel.h>
#include <zephyr/arch/riscv/csr.h>
#include <zephyr/sys/mem_manage.h>
#include <string.h>

#include <riscv_mmu.h>
#define PTE_ALL (PTE_VALID | PTE_READ | PTE_WRITE | PTE_EXEC | PTE_GLOBAL)

/*---------------- External Variables ----------*/
// Linker Symbols for Kernel Memory Regions
// These are defined in Zephyr's linker script and provide addresses of kernel memory regions
extern uintptr_t _image_ram_start;
extern uintptr_t _image_ram_end;
extern uintptr_t __text_region_start;
extern uintptr_t __text_region_end;

/*---------------- Global Variables -----------*/
typedef uint32_t riscv_pte_t; // A single 32-bit Sv32 Page Table Entry (PTE)
static struct riscv_mmu_l1_page_table l1_page_table __aligned(KB(4)) = {0}; // Pointer to the root page table (Level 1)

/*----------------- PFP ----------------------*/
void riscv_tlb_flush_all(void);
void riscv_tlb_flush(uintptr_t);

/*----------------- Static Functions --------*/

__aligned(PAGE_SIZE)
static struct riscv_mmu_l2_page_table l2_page_table_pool[MAX_L2_TABLES];
static int next_free_l2 = 0;
/**
 * @brief Allocates a 4 KiB-aligned page table
 * @return A pointer to the allocated page table, or NULL on failure
 */
void *allocate_l2_page_table(void)
{
    if (next_free_l2 >= MAX_L2_TABLES) {
        return NULL;
    }
    return &l2_page_table_pool[next_free_l2++];
}



/*----------------- Public Functions ----------*/

/**
 * @brief Initializes the MMU and sets up Sv32 page tables
 */
void z_riscv_mm_init(void)
{
    printk("MMU: Starting early MMU initialization...\n");
    riscv_tlb_flush_all();

    /* 1. Map Kernel Text Section as Read + Execute (RX) */
    uintptr_t text_start = (uintptr_t)&__text_region_start;
    uintptr_t text_end   = (uintptr_t)&__text_region_end;

    text_start &= ~(PAGE_SIZE - 1); // Align start
    text_end   = (text_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // Align end

    // printk("MMU: Mapping .text from PA/VA 0x%08lx to 0x%08lx\n", text_start, text_end);
    

    for (uintptr_t addr = text_start; addr < text_end; addr += PAGE_SIZE) {
        int ret = riscv_map_page(addr, addr, 
            PTE_ALL); // 1:1 mapping
                                //  PTE_VALID | PTE_READ | PTE_EXEC | PTE_GLOBAL);
        
        if (ret) {
            printk("MMU: Failed to map .text page at 0x%lx (ret=%d)\n", addr, ret);
        }
    }

    /* 2. Map Kernel RAM as Read + Write (RW) */
    uintptr_t ram_start = (uintptr_t)&_image_ram_start;
    uintptr_t ram_end   = (uintptr_t)&_image_ram_end;

    ram_start &= ~(PAGE_SIZE - 1); // Align start
    ram_end   = (ram_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1); // Align end

    printk("MMU: Mapping RAM from PA/VA 0x%08lx to 0x%08lx\n", ram_start, ram_end);

    // for (uintptr_t addr = ram_start; addr < ram_end; addr += PAGE_SIZE) {
    //     int ret = riscv_map_page(addr, addr,  // 1:1 mapping
    //         PTE_ALL);//  PTE_VALID | PTE_READ | PTE_WRITE | PTE_GLOBAL);
    //     if (ret) {
    //         printk("MMU: Failed to map RAM page at 0x%lx (ret=%d)\n", addr, ret);
    //     }
    // }
    uintptr_t test_va = 0x81000000;
uintptr_t test_pa = (uintptr_t)&_image_ram_start + 0x1000;  // Just for test
printk("User thread: mapping test_va with flags = 0x%x\n", (PTE_VALID | PTE_READ | PTE_GLOBAL | PTE_USER));
riscv_map_page(test_va, test_pa, PTE_VALID | PTE_READ | PTE_GLOBAL | PTE_USER);  // W=0

    /* 3. Construct SATP Register (Sv32) */
    uintptr_t root_ppn = (uintptr_t)&l1_page_table >> SV32_PTE_PPN_SHIFT;
    uintptr_t satp = (1UL << 31) | root_ppn;  // Mode = Sv32, ASID = 0

    __asm__ volatile("csrw satp, %0" :: "r"(satp));
    printk("MMU: SATP set to 0x%lx\n", satp);

    /* 4. Flush the TLB */
    riscv_tlb_flush_all();
    printk("MMU: TLB flushed (sfence.vma)\n");

    /* 5. Final confirmation */
    printk("MMU: Initialization complete, root PT @ %p\n", &l1_page_table);
}
uint32_t last_mapped_pte = 0;

/**
 * @brief Maps a virtual address to a physical address in the RISC-V Sv32 page table.
 *
 * This function inserts a new mapping into the MMU, translating a virtual address (VA)
 * to a physical address (PA) with the given access permissions. If the required Level 0
 * page table does not exist, it will be allocated dynamically.
 *
 * @param virt  The virtual address to be mapped (must be 4 KiB aligned).
 * @param phys  The physical address to map to (must be 4 KiB aligned).
 * @param flags Access permissions (e.g., PTE_READ | PTE_WRITE | PTE_EXEC).
 *
 * @note This function assumes the root page table is already allocated and initialized.
 *       The function also flushes the TLB entry for the mapped address using `sfence.vma`.
 */
int riscv_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags)
{
    printk("MMU: Mapping VA=0x%lx, PA=0x%lx, flags=0x%x\n", virt, phys, flags);

    uint32_t l1_index = L1_INDEX(virt);
    uint32_t l2_index = L2_INDEX(virt);
    struct riscv_mmu_l2_page_table *l2_page_table;


    /* 1. Allocate Level 2 Table if Necessary */
    if (l1_page_table.entries[l1_index].page_table_entry.v != 1) {
        l2_page_table = (struct riscv_mmu_l2_page_table *)allocate_l2_page_table();
        if (!l2_page_table) {
            printk("MMU: Failed to allocate L2 page table\n");
            return -ENOMEM;
        }

        memset(l2_page_table, 0, PAGE_SIZE);

        // Update Level 1 entry
        l1_page_table.entries[l1_index].l2_page_table_ref.l2_page_table_address =
            ((uintptr_t)l2_page_table >> SV32_PT_L2_ADDR_SHIFT) & SV32_PT_L2_ADDR_MASK;
        l1_page_table.entries[l1_index].page_table_entry.v = 1;
        l1_page_table.entries[l1_index].page_table_entry.u = 1;
    }

    /* 2. Resolve L2 Table from PPN */
    uintptr_t l2_ppn = l1_page_table.entries[l1_index].l2_page_table_ref.l2_page_table_address;
    uintptr_t l2_page_table_phys = l2_ppn << SV32_PT_L2_ADDR_SHIFT;
    l2_page_table = (struct riscv_mmu_l2_page_table *)l2_page_table_phys;

    if (l2_page_table->entries[l2_index].l2_page_4k.v == 1) {
        printk("MMU: Warning, overwriting existing PTE for VA 0x%lx\n", virt);
    }
    /* 3. Set Page Entry with Flags */
    l2_page_table->entries[l2_index].l2_page_4k.pa_base = (phys >> SV32_PTE_PPN_SHIFT) & SV32_PT_L2_ADDR_MASK;
    l2_page_table->entries[l2_index].l2_page_4k.v = 1;
    l2_page_table->entries[l2_index].l2_page_4k.r = (flags & PTE_READ) != 0;
    l2_page_table->entries[l2_index].l2_page_4k.w = (flags & PTE_WRITE) != 0;
    l2_page_table->entries[l2_index].l2_page_4k.x = (flags & PTE_EXEC) != 0;
    l2_page_table->entries[l2_index].l2_page_4k.u = (flags & PTE_USER) != 0;
    l2_page_table->entries[l2_index].l2_page_4k.g = (flags & PTE_GLOBAL) != 0;
    l2_page_table->entries[l2_index].l2_page_4k.a = 1;
    l2_page_table->entries[l2_index].l2_page_4k.d = (flags & PTE_WRITE) ? 1 : 0;

    //test code for now
    if (virt == 0x81000000) {
        uint32_t *pte_ptr = (uint32_t *)&l2_page_table->entries[l2_index];
        last_mapped_pte = *pte_ptr;
    }

    /* 4. Flush TLB */
    riscv_tlb_flush(virt);

    printk("MMU: Mapped VA %p -> PA %p (L1 Index %d, L2 Index %d)\n",
           (void *)virt, (void *)phys, l1_index, l2_index);
    return 0;
}


/**
 * @brief Unmaps a virtual address from the RISC-V Sv32 page table.
 *
 * This function removes a virtual-to-physical mapping by clearing the PTE entry.
 * If the Level 0 table exists but becomes empty, it can be freed (optional).
 *
 * @param virt  The virtual address to be unmapped (must be 4 KiB aligned).
 */
void riscv_unmap_page(uintptr_t virt)
{
    uint32_t l1_index = L1_INDEX(virt);
    uint32_t l2_index = L2_INDEX(virt);

    /* 1. Check if the Level 0 Page Table Exists */
    if (l1_page_table.entries[l1_index].page_table_entry.v != 1) {
        printk("MMU: Unmap failed, no L0 table for VA %p\n", (void *)virt);
        return;
    }

    /* 2. Get the L2 Page Table Address */
    uintptr_t l2_page_table_temp = (uintptr_t) (l1_page_table.entries[l1_index].word & (SV32_PT_L2_ADDR_MASK << SV32_PT_L2_ADDR_SHIFT));
    struct riscv_mmu_l2_page_table * l2_page_table = (struct riscv_mmu_l2_page_table *) l2_page_table_temp;
    /* 3. Check if Mapping Exists */
    if (l2_page_table->entries[l2_index].l2_page_4k.v != 1) {
        printk("MMU: Unmap failed, VA %p is not mapped\n", (void *)virt);
        return;
    }

    /* 4. Remove the Mapping */
    l2_page_table->entries[l2_index].l2_page_4k.v = 0;
    printk("MMU: Unmapped VA %p (L1 Index %d, L0 Index %d)\n", (void *)virt, l1_index, l2_index);

    /* 5. Flush TLB for this address */
    riscv_tlb_flush(virt);
}

/**
 * @brief Flushes a specific virtual address from the TLB.
 *
 * Ensures that changes to page table mappings are recognized by the MMU.
 *
 * @param virt The virtual address to flush from the TLB.
 */
void riscv_tlb_flush(uintptr_t virt)
{
    printk("MMU: Flushing TLB for VA %p\n", (void *)virt);
    __asm__ volatile ("sfence.vma %0, x0" :: "r"(virt) : "memory");
    
}

/**
 * @brief Flushes the entire TLB (Translation Lookaside Buffer).
 *
 * This function ensures that all cached virtual-to-physical mappings are invalidated.
 * It is required when switching address spaces or performing global memory updates.
 */
void riscv_tlb_flush_all(void)
{
    printk("MMU: Flushing entire TLB\n");
    __asm__ volatile ("sfence.vma x0, x0" ::: "memory");
}

/**
 * @brief Maps a range of virtual addresses to physical addresses.
 *
 * This function maps a virtual memory range to a corresponding physical memory range
 * with the specified access permissions. It ensures that the mapping is page-aligned
 * and covers the full range requested.
 *
 * @param virt  The starting virtual address (must be page-aligned).
 * @param phys  The starting physical address (must be page-aligned).
 * @param size  The number of bytes to map (must be a multiple of PAGE_SIZE).
 * @param flags Access permissions (e.g., PTE_READ | PTE_WRITE | PTE_EXEC).
 *
 * @return 0 on success, -EINVAL if alignment is incorrect.
 */
int arch_mem_map(void *virt, uintptr_t phys, size_t size, uint32_t flags)
{
    printk("MMU: arch_mem_map() called - VA %p -> PA %p, size: %lu bytes, flags: 0x%x\n",
           virt, (void *)phys, size, flags);

    // Ensure addresses and size are page-aligned
    if (((uintptr_t)virt & (PAGE_SIZE - 1)) || (phys & (PAGE_SIZE - 1)) || (size & (PAGE_SIZE - 1))) {
        printk("MMU: arch_mem_map() failed - addresses must be page-aligned\n");
        return -EINVAL;
    }

    uintptr_t va = (uintptr_t)virt;
    uintptr_t pa = phys;

    // Iterate through each 4 KiB page and map it
    while (size > 0) {
        printk("MMU: Mapping VA %p -> PA %p with flags 0x%x\n", (void *)va, (void *)pa, flags);
        riscv_map_page(va, pa, flags);
        va += PAGE_SIZE;
        pa += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    printk("MMU: arch_mem_map() completed successfully.\n");
    return 0;
}


/**
 * @brief Unmaps a range of virtual addresses.
 *
 * This function removes virtual memory mappings in the specified range.
 *
 * @param virt  The starting virtual address (must be page-aligned).
 * @param size  The number of bytes to unmap (must be a multiple of PAGE_SIZE).
 *
 * @return 0 on success, -EINVAL if alignment is incorrect.
 */
int arch_mem_unmap(void *virt, size_t size)
{
    printk("MMU: arch_mem_unmap() called - VA %p, size: %lu bytes\n", virt, size);

    // Ensure virtual address and size are page-aligned
    if (((uintptr_t)virt & (PAGE_SIZE - 1)) || (size & (PAGE_SIZE - 1))) {
        printk("MMU: arch_mem_unmap() failed - addresses must be page-aligned\n");
        return -EINVAL;
    }

    uintptr_t va = (uintptr_t)virt;

    // Iterate through each 4 KiB page and unmap it
    while (size > 0) {
        printk("MMU: Unmapping VA %p\n", (void *)va);
        riscv_unmap_page(va);
        va += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    printk("MMU: arch_mem_unmap() completed successfully.\n");
    return 0;
}

/**
 * @brief Retrieves the physical address mapped to a given virtual address.
 *
 * This function looks up the MMU page tables to find the physical address (PA)
 * that corresponds to a given virtual address (VA).
 *
 * @param virt  The virtual address to look up.
 * @param phys  Pointer to store the retrieved physical address.
 *
 * @return 0 on success, -EINVAL if the virtual address is not mapped.
 */
int arch_page_phys_get(void *virt, uintptr_t *phys)
{
    uintptr_t va = (uintptr_t)virt;
    uint32_t l1_index = L1_INDEX(va);
    uint32_t l2_index = L2_INDEX(va);

    // 1. Check if the L1 (top-level) entry is valid
    if (l1_page_table.entries[l1_index].page_table_entry.v != 1) {
        printk("MMU: arch_page_phys_get() failed - No L0 table for VA %p\n", virt);
        return -EINVAL;
    }

    // 2. Recover L2 page table address from L1 entry
    uintptr_t l2_pt_ppn = l1_page_table.entries[l1_index].l2_page_table_ref.l2_page_table_address;
    uintptr_t l2_page_table_addr = l2_pt_ppn << SV32_PT_L2_ADDR_SHIFT;

    struct riscv_mmu_l2_page_table *l2_page_table = (struct riscv_mmu_l2_page_table *)l2_page_table_addr;

    // 3. Check if this L2 entry is valid
    if (l2_page_table->entries[l2_index].l2_page_4k.v != 1) {
        printk("MMU: arch_page_phys_get() failed - VA %p is not mapped\n", virt);
        return -EINVAL;
    }

    // 4. Extract and return the physical address
    uintptr_t pa_ppn = l2_page_table->entries[l2_index].l2_page_4k.pa_base;
    *phys = pa_ppn << SV32_PTE_PPN_SHIFT;

    // printk("MMU: arch_page_phys_get(): L2[%d].pa_base = 0x%x -> PA 0x%lx\n",
    //    l2_index,
    //    l2_page_table->entries[l2_index].l2_page_4k.pa_base,
    //    *phys);

    printk("MMU: arch_page_phys_get() - VA %p -> PA %p\n", virt, (void *)*phys);

    return 0;
}

/**
 * @brief Handles a page fault by allocating and mapping a new page.
 *
 * This function is called when an unmapped virtual address is accessed. It
 * allocates a new physical page, maps it, and flushes the TLB entry.
 *
 * @param fault_addr The virtual address that caused the page fault.
 *
 * @return 0 on success, -ENOMEM if memory allocation fails, -EINVAL if fault_addr is misaligned.
 */
int riscv_handle_page_fault(uintptr_t fault_addr)
{
    // Ensure the faulting address is page-aligned
    if (fault_addr & (PAGE_SIZE - 1)) {
        printk("MMU: Page fault handler failed - misaligned address %p\n", (void *)fault_addr);
        return -EINVAL;
    }

    printk("MMU: Handling page fault at VA %p\n", (void *)fault_addr);

    // Step 2: Allocate a new physical page (for now, just get a dummy page)
    void *new_page_ptr = k_malloc(PAGE_SIZE);
    if (!new_page_ptr) {
        printk("MMU: Page fault handler failed - Out of memory\n");
        return -ENOMEM;
    }

    // Ensure the physical page is cleared
    memset(new_page_ptr, 0, PAGE_SIZE);
    uintptr_t new_phys_page = (uintptr_t)new_page_ptr;
    printk("MMU: Allocated new page at PA %p for VA %p\n", (void *)new_phys_page, (void *)fault_addr);

    // Step 3: Map the new page into the MMU
    riscv_map_page(fault_addr, new_phys_page, PTE_VALID | PTE_READ | PTE_WRITE | PTE_EXEC);

    // Step 4: Flush the TLB entry for this address
    riscv_tlb_flush(fault_addr);

    printk("MMU: Page fault resolved - VA %p -> PA %p\n", (void *)fault_addr, (void *)new_phys_page);

    return 0;
}