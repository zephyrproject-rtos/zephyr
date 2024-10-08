
#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>

#define _DDR_NODE          DT_CHOSEN(zephyr_sram)
#define _LAYOUT_DDR_LOC	   DT_REG_ADDR(_DDR_NODE)
#define _LAYOUT_DDR_SIZE   DT_REG_SIZE(_DDR_NODE)

/* Current offset from HEAP_BASE of unused memory */
__attribute__((section(".bss"), used)) static size_t heap_sz;

#define HEAP_BASE ((uintptr_t) (&_end))
#define MAX_HEAP_SIZE (_LAYOUT_DDR_LOC + _LAYOUT_DDR_SIZE - HEAP_BASE)

/* Implementation stolen from newlib/libc-hooks.c */
void *_sbrk(intptr_t count)
{
	void *ret, *ptr;

	ptr = ((char *)HEAP_BASE) + heap_sz;

	if ((heap_sz + count) < MAX_HEAP_SIZE) {
		heap_sz += count;
		ret = ptr;

	} else {
		ret = (void *)-1;
	}

	return ret;
}
__weak FUNC_ALIAS(_sbrk, sbrk, void *);