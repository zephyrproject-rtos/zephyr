/*
* Created for ECE595, ASE, Spring 2025
*
*
*/
#ifndef MMU_H_
#define MMU_H_

#define PAGE_SIZE 4096  // 4 KiB page size for Sv32
#define PTE_SIZE 4      // Each PTE (Page Table Entry) is 4 bytes
#define RISCV_MMU_PT_NUM_ENTRIES 1024 //Number of entries  per page table 
#define MAX_L2_TABLES 16

// Define bit flags for PTE entries
#define PTE_VALID    (1 << 0)  // Marks entry as valid
#define PTE_READ     (1 << 1)  // Allows read access
#define PTE_WRITE    (1 << 2)  // Allows write access
#define PTE_EXEC     (1 << 3)  // Allows execute access
#define PTE_USER     (1 << 4)
#define PTE_GLOBAL   (1 << 5)  // Makes the mapping global (not ASID-specific)

#define SV32_PTE_PPN_SHIFT  12  // Physical Page Number (PPN) shift (aligns to 4 KiB)
#define SV32_PTE_PPN_MASK   0xFFFFFC // Mask for PPN extraction
#define SV32_PT_L2_ADDR_MASK   0x3FFFFF // Mask for PPN extraction
#define SV32_PT_L2_ADDR_SHIFT   10 // Mask for PPN extraction
#define SV32_ADDR_TO_PPN_SHIFT    10  // Position of PPN in PTE (Sv32 stores it at bits 31-10)

// Macros to extract page table indices from a virtual address
#define L1_INDEX(va)  (((va) >> 22) & 0x3FF)  // Level 1 (root) index from VPN[1]
#define L2_INDEX(va)  (((va) >> 12) & 0x3FF)  // Level 2 (leaf) index from VPN[0]

// Functions to be used by Zephyr
void z_riscv_mm_init(void);
int riscv_map_page(uintptr_t virt, uintptr_t phys, uint32_t flags);
int arch_page_phys_get(void *virt, uintptr_t *phys);

union riscv_l1_mmu_page_table_entry {
    struct {
        uint32_t v                  : 1;  /* [00]*/
        uint32_t r                  : 1;
        uint32_t w                  : 1;
        uint32_t x                  : 1;
        uint32_t u                  : 1;
        uint32_t g                  : 1;
        uint32_t a                  : 1;
        uint32_t d                  : 1;
        uint32_t rsx                : 2;
        uint32_t ppn_0              : 10;
        uint32_t ppn_1              : 12;  /*[31]*/
    } page_table_entry;
    struct {
        uint32_t v                      : 1;  /* [00]*/
        uint32_t r                      : 1;
        uint32_t w                      : 1;
        uint32_t x                      : 1;
        uint32_t u                      : 1;
        uint32_t g                      : 1;
        uint32_t a                      : 1;
        uint32_t d                      : 1;
        uint32_t rsx                    : 2;
        uint32_t l2_page_table_address  : 22;
    } l2_page_table_ref;
    uint32_t word;
};

struct riscv_mmu_l1_page_table {
	union riscv_l1_mmu_page_table_entry entries[RISCV_MMU_PT_NUM_ENTRIES];
};

union riscv_l2_mmu_page_table_entry {
	struct {
        uint32_t v                      : 1;  /* [00]*/
        uint32_t r                      : 1;
        uint32_t w                      : 1;
        uint32_t x                      : 1;
        uint32_t u                      : 1;
        uint32_t g                      : 1;
        uint32_t a                      : 1;
        uint32_t d                      : 1;
        uint32_t rsx                    : 2;
		uint32_t pa_base		        : 22; /* [31] */
	} l2_page_4k;
    uint32_t word;
};

struct riscv_mmu_l2_page_table {
	union riscv_l2_mmu_page_table_entry entries[RISCV_MMU_PT_NUM_ENTRIES];
};



#endif //MMU_H_