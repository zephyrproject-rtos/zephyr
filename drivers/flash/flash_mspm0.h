#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_MSPM0_H
#define ZEPHYR_DRIVERS_FLASH_FLASH_MSPM0_H_


#include <zephyr/drivers/flash.h>
#include <zephyr/devicetree.h>
#include <driverlib/dl_flashctl.h>

#define FLASH_SIZE (CONFIG_FLASH_SIZE * 1024)
#define FLASH_PAGE_SIZE          (CONFIG_FLASH_MSPM0_LAYOUT_PAGE_SIZE)
#define FLASH_MSPM0_BASE_ADDRESS DT_REG_ADDR(DT_INST(0, soc_nv_flash))

#if DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#define FLASH_MSPM0_WRITE_BLOCK_SIZE \
	DT_PROP(DT_INST(0, soc_nv_flash), write_block_size)
#else
#error Flash write block size not available
	/* Flash Write block size is extracted from device tree */
	/* as flash node property 'write-block-size' */
#endif

/* Single program flash word size 64-bit */
#define FLASH_MSPM0_FLASH_WRITE_SIZE	(8)

struct flash_mspm0_priv {
	FLASHCTL_Regs *regs;
	struct k_sem sem;
};

#define FLASH_MSPM0_PRIV(dev) ((struct flash_mspm0_priv *)((dev)->config))

#define FLASH_MSPM0_REGS(dev) (FLASH_MSPM0_PRIV(dev)->regs)

// enum mspm0_ex_ops {
//     FLASH_MSPM0_EX_OP_SECTOR_WP = FLASH_EX_OP_VENDOR_BASE,

// 	FLASH_MSPM0_EX_OP_RDP,

// 	FLASH_MSPM0
// };


#ifdef CONFIG_FLASH_PAGE_LAYOUT
static inline bool flash_mspm0_range_exists(const struct device *dev,
					    off_t offset,
					    uint32_t len)
{
	struct flash_pages_info info;

	return !(flash_get_page_info_by_offs(dev, offset, &info) ||
		 flash_get_page_info_by_offs(dev, offset + len - 1, &info));
}
#else
#error Error! Flash Page layout not defined
#endif	/* CONFIG_FLASH_PAGE_LAYOUT */

#ifdef CONFIG_FLASH_PAGE_LAYOUT
void flash_mspm0_page_layout(const struct device *dev,
			     const struct flash_pages_layout **layout,
			     size_t *layout_size);
#endif

int flash_mspm0_write_range(const struct device *dev, unsigned int offset, const void *data, unsigned int len);

int flash_mspm0_block_erase_loop(const struct device *dev, unsigned int offset, unsigned int len);

int flash_mspm0_wait_flash_idle(const struct device *dev);

static int flash_mspm0_erase(const struct device *dev, off_t offset, size_t len);
int flash_mspm0_block_erase_loop(const struct device *dev,
				 unsigned int offset,
				 unsigned int len);
#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_MSPM0_H_ */
