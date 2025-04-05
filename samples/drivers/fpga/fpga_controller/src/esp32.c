#include <esp_spi_flash.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>

static int esp32_init(void)
{
	const void *vaddr = NULL;
	__unused esp_err_t esp_err;
	spi_flash_mmap_handle_t handle;
	size_t paddr = FIXED_PARTITION_OFFSET(scratch_partition);
	size_t size = FIXED_PARTITION_SIZE(scratch_partition);

	esp_err = spi_flash_mmap(paddr, size, SPI_FLASH_MMAP_DATA, &vaddr, &handle);
	if (esp_err != ESP_OK) {
		printk("failed to mmap %zx\n", paddr);
		return -EIO;
	}

	printk("esp32: mapped %zu@%zx to %p\n", size, paddr, vaddr);

	return 0;
}

SYS_INIT(esp32_init, POST_KERNEL, 42);