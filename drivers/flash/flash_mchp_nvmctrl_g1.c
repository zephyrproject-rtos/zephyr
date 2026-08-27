/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file flash_mchp_nvmctrl_g1.c
 * @brief G1 Flash driver for NVMCTRL peripheral.
 *
 * Implements Zephyr Flash API support with basic flash memory
 * operations.
 *
 */

#include <soc.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include <zephyr/drivers/flash/mchp_flash.h>

/*******************************************
 * @brief Devicetree definitions
 *******************************************/
#define DT_DRV_COMPAT microchip_nvmctrl_g1_flash

/*******************************************
 * Const and Macro Defines
 *******************************************/
LOG_MODULE_REGISTER(flash_mchp_nvmctrl_g1);

/* Number of lock regions in the SoC non-volatile flash. */
#define SOC_NV_FLASH_LOCK_REGIONS DT_INST_PROP(0, lock_regions)

/* Size of each lock region in the SoC non-volatile flash. */
#define SOC_NV_FLASH_LOCK_REGION_SIZE ((SOC_NV_FLASH_SIZE) / (SOC_NV_FLASH_LOCK_REGIONS))

/* Device tree node identifier for SoC non-volatile flash instance 0. */
#define SOC_NV_FLASH_NODE DT_INST(0, soc_nv_flash)

/* Size of the SoC non-volatile flash, in bytes. */
#define SOC_NV_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

/* Base address of the SoC non-volatile flash. */
#define SOC_NV_FLASH_BASE_ADDRESS DT_REG_ADDR(SOC_NV_FLASH_NODE)

/* Default size of a flash write block in bytes */
#define FLASH_WRITE_BLOCK_SIZE_DEFAULT 8

/* Write block size of the SoC non-volatile flash, in bytes. */
#define SOC_NV_FLASH_WRITE_BLOCK_SIZE                                                              \
	DT_PROP_OR(SOC_NV_FLASH_NODE, write_block_size, FLASH_WRITE_BLOCK_SIZE_DEFAULT)

/* Default size of a flash erase block in bytes */
#define FLASH_ERASE_BLOCK_SIZE_DEFAULT 8192

/* Erase block size of the SoC non-volatile flash, in bytes. */
#define SOC_NV_FLASH_ERASE_BLOCK_SIZE                                                              \
	DT_PROP_OR(SOC_NV_FLASH_NODE, erase_block_size, FLASH_ERASE_BLOCK_SIZE_DEFAULT)

/* Device tree node identifier for the user row region of SoC non-volatile flash. */
#define SOC_NV_USERROW_NODE DT_INST(1, soc_nv_flash)

/* Size of the userpage region in the SoC non-volatile flash, in bytes. */
#define SOC_NV_USERROW_SIZE DT_REG_SIZE(SOC_NV_USERROW_NODE)

/* Base address of the userpage region in the SoC non-volatile flash. */
#define SOC_NV_USERROW_BASE_ADDR DT_REG_ADDR(SOC_NV_USERROW_NODE)

/* Write block size of the userpage region, in bytes. */
#define SOC_NV_USERROW_WRITE_BLOCK_SIZE DT_PROP(SOC_NV_USERROW_NODE, write_block_size)

/* Erase block size of the userpage region, in bytes. */
#define SOC_NV_USERROW_ERASE_BLOCK_SIZE DT_PROP(SOC_NV_USERROW_NODE, erase_block_size)

/* Number of flash page layouts supported by the MCHP flash driver. */
#define FLASH_MCHP_LAYOUT_SIZE 0x1

/* Size of a double word in bytes for MCHP flash. */
#define FLASH_MCHP_DOUBLE_WORD_SIZE 0x8

/* Size of a quad word in bytes for MCHP flash. */
#define FLASH_MCHP_QUAD_WORD_SIZE 0x10

/* Size of a page in bytes for MCHP flash. */
#define FLASH_MCHP_PAGE_SIZE 0x200

/* Device config */
#define DEV_CFG(dev) ((const struct flash_mchp_dev_config *const)(dev)->config)

/* NVMCTRL Register */
#define NVM_REGS ((const struct flash_mchp_dev_config *)(dev)->config)->regs

/** @brief Default value of flash memory after an erase operation. */
#define FLASH_ERASE_DEFAULT_VALUE 0xFF

/**
 * @def FLASH_MCHP_SUCCESS
 * @brief Macro indicating successful operation.
 */
#define FLASH_MCHP_SUCCESS 0

/**< Encodes the write mode value for the NVMCTRL_CTRLA register. */
#define FLASH_SET_WMODE(mode) ((mode) << NVMCTRL_CTRLA_WMODE_Pos)

/**
 * @brief Calculate the address in flash memory.
 *
 * This macro computes the address in flash memory by adding the specified
 * offset to the base address of the flash memory.
 *
 * @param a Offset to be added to the base address of the flash memory.
 */
#define FLASH_MEMORY(a) ((uint32_t *)((uint8_t *)((a) + SOC_NV_FLASH_BASE_ADDRESS)))

/* Timeout values for WAIT_FOR macro */
#define TIMEOUT_VALUE_US 100000

#define DELAY_US 2

/*******************************************
 * Enums and Structs
 *******************************************/
/**
 * @struct flash_mchp_clock
 * @brief Structure to hold device clock configuration.
 */
struct flash_mchp_clock {

	/* Clock driver */
	const struct device *clock_dev;

	/* Main clock subsystem. */
	clock_control_subsys_t mclk_sys;
};

/**
 * @struct flash_mchp_dev_data
 * @brief Structure to hold flash device data.
 */
struct flash_mchp_dev_data {

	/* Pointer to the Flash device instance. */
	const struct device *dev;

	/* Semaphore lock for flash APIs operations */
	struct k_mutex flash_data_lock;

	/* Stores the current interrupt flag status */
	volatile uint16_t interrupt_flag_status;
};

/**
 * @struct flash_mchp_dev_config
 * @brief Structure to hold flash device configuration.
 */
struct flash_mchp_dev_config {

	/* Pointer to Flash peripheral registers */
	nvmctrl_registers_t *regs;

	/* Flash base address */
	uint32_t base_addr;

	/* Flash clock control */
	struct flash_mchp_clock flash_clock;

	/* Function to configure IRQ */
	void (*irq_config_func)(const struct device *dev);

	/* Flash memory parameters */
	struct flash_parameters flash_param;

#ifdef CONFIG_FLASH_PAGE_LAYOUT
	/* Flash pages layouts */
	struct flash_pages_layout flash_layout;
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

/**
 * @enum flash_mchp_write_mode
 * @brief Enumeration for Flash write modes.
 *
 * This enumeration defines the different write modes available for the
 * Flash. Each mode specifies how data is written to the non-volatile memory.
 */
enum flash_mchp_write_mode {
	NVMCTRL_WMODE_MAN, /* Manual Write Mode */
	NVMCTRL_WMODE_ADW, /* Automatic Double Word Write Mode */
	NVMCTRL_WMODE_AQW, /* Automatic Quad Word Write Mode */
	NVMCTRL_WMODE_AP   /* Automatic Page Write Mode */
};

/*******************************************
 * Helper functions
 *******************************************/
/**
 * @brief Check if a given value is aligned to a specified alignment.
 *
 * This function determines whether the provided value is aligned to the
 * specified alignment boundary. Alignment is typically a power of two,
 * and this function checks if the value is a multiple of the alignment.
 *
 * @param value The value to be checked for alignment.
 * @param alignment The alignment boundary to check against. This should
 *                  be a power of two.
 *
 * @return FLASH_MCHP_SUCCESS if the value is aligned to the specified alignment,
 *         -EINVAL otherwise.
 */
static inline int flash_aligned(size_t value, size_t alignment)
{
	return (((value & (alignment - 1)) == 0) ? FLASH_MCHP_SUCCESS : -EINVAL);
}

/**
 * @brief Initializes the NVMCTRL module with automatic wait state generation.
 *
 * This function configures the NVMCTRL_CTRLA register to enable automatic wait
 * state generation by enabling the automatic wait state mask (AUTOWS).
 *
 * @param dev Pointer to the device structure representing the flash controller.
 */
static inline void flash_enable_auto_wait_state(const struct device *dev)
{
	/* Automatic wait state generation */
	NVM_REGS->NVMCTRL_CTRLA = NVMCTRL_CTRLA_AUTOWS_Msk;
}

/**
 * @brief Enable NVMCTRL interrupt.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 *
 */
static inline void flash_interrupt_enable(const struct device *dev)
{
	const uint16_t enable_mask = NVMCTRL_INTENSET_ADDRE_Msk | NVMCTRL_INTENSET_PROGE_Msk |
				     NVMCTRL_INTENSET_LOCKE_Msk | NVMCTRL_INTENSET_NVME_Msk;

	NVM_REGS->NVMCTRL_INTENSET = enable_mask;
}

/**
 * @brief Initializes the flash controller for the specified device.
 *
 * This function enables the automatic wait state and interrupt for the flash
 * controller associated with the given device. It should be called before
 * performing any flash operations to ensure the controller is properly configured.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 */
static void flash_controller_init(const struct device *dev)
{
	flash_enable_auto_wait_state(dev);
	flash_interrupt_enable(dev);
}

/**
 * @brief Set the write mode for the NVMCTRL peripheral.
 *
 * This function configures the write mode of the NVMCTRL (Non-Volatile Memory
 * Controller) by updating the NVMCTRL_CTRLA register with the specified mode.
 * The function ensures that only the write mode bits are modified, preserving
 * the other bits in the register.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 * @param mode Write mode to set for the NVMCTRL.
 */
static inline void flash_set_write_mode(const struct device *dev, enum flash_mchp_write_mode mode)
{
	uint16_t reg = NVM_REGS->NVMCTRL_CTRLA;

	/* Clear the write mode bits and set the new mode */
	reg &= ~NVMCTRL_CTRLA_WMODE_Msk;
	reg |= FLASH_SET_WMODE(mode);

	/* Write back the updated value */
	NVM_REGS->NVMCTRL_CTRLA = reg;
}

/**
 * @brief Retrieve and clear the interrupt flag status of the NVMCTRL
 * peripheral.
 *
 * This function reads the current interrupt flag status from the
 * NVMCTRL_INTFLAG register and then clears the interrupt flags by writing the
 * same value back to the register. This operation is typically used to
 * acknowledge and clear interrupt flags.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 *
 */
static inline void flash_clear_interrupt_flag(const struct device *dev)
{
	struct flash_mchp_dev_data *mchp_flash_data = dev->data;

	mchp_flash_data->interrupt_flag_status = NVM_REGS->NVMCTRL_INTFLAG;

	/* Clear NVMCTRL INTFLAG register */
	NVM_REGS->NVMCTRL_INTFLAG = mchp_flash_data->interrupt_flag_status;
}

/**
 * @brief Retrieve and report the error status of the NVM controller.
 *
 * This function examines the interrupt flag status of the NVMCTRL (Non-Volatile
 * Memory Controller) to determine if any errors have occurred. It checks for
 * address, programming, lock, and NVM errors The function returns a success or
 * failure code based on the presence of errors.
 *
 * @return Returns `FLASH_MCHP_SUCCESS` if no errors are detected, or
 *         `-EIO` if any error flags are set.
 */
static int flash_get_interrupt_status_error(const struct device *dev)
{
	int ret = FLASH_MCHP_SUCCESS;

	struct flash_mchp_dev_data *mchp_flash_data = dev->data;
	uint16_t status = mchp_flash_data->interrupt_flag_status;

	/* Combine all error masks */
	const uint16_t error_mask = NVMCTRL_INTFLAG_ADDRE_Msk | NVMCTRL_INTFLAG_PROGE_Msk |
				    NVMCTRL_INTFLAG_LOCKE_Msk | NVMCTRL_INTFLAG_NVME_Msk;

	if ((status & error_mask) != 0) {
		LOG_ERR("flash operation failed with status 0x%x", status);
		ret = -EIO;
	}

	return ret;
}

/**
 * @brief Block until the NVMCTRL indicates it is ready.
 *
 * This function continuously checks the NVMCTRL status register until the
 * "ready" bit is set, indicating that the NVMCTRL is no longer busy with
 * programming or erasing operations and is ready for a new command.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 *
 * @note This function blocks execution until the NVMCTRL is ready.
 */
static inline void flash_status_ready_wait(const struct device *dev)
{
	/* Wait until the NVM controller is ready */
	if (!WAIT_FOR(((NVM_REGS->NVMCTRL_STATUS & NVMCTRL_STATUS_READY_Msk) ==
		       NVMCTRL_STATUS_READY_Msk),
		      TIMEOUT_VALUE_US, k_busy_wait(DELAY_US))) {
		LOG_ERR("NVMCTRL_STATUS_READY wait timed out");
	}
}

/**
 * @brief Executes a flash memory controller command.
 *
 * Combines the specified command with the required command execution key and writes
 * the result to the NVM controller's control register to initiate the desired flash operation.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 * @param command The flash controller command to execute (e.g., erase, unlock, write).
 */
static inline void flash_process_command(const struct device *dev, uint16_t command)
{
	NVM_REGS->NVMCTRL_CTRLB = command | NVMCTRL_CTRLB_CMDEX_KEY;
}

/**
 * @brief Issue a command to clear the flash page buffer.
 *
 * This function sends the Page Buffer Clear (PBC) command to the flash controller,
 * preparing the page buffer for a new write operation.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 */
static inline void flash_pagebuffer_clear(const struct device *dev)
{
	flash_process_command(dev, NVMCTRL_CTRLB_CMD_PBC);
}

/**
 * @brief Write a double word (64 bits) to flash memory.
 *
 * This function writes a double word (typically 64 bits) to the specified flash memory address.
 * The data is written in 32-bit chunks, as required by the flash controller's page buffer.
 * The function waits for the flash to be ready before writing and checks the status after the
 * operation.
 *
 * @param dev     Pointer to the device structure representing the flash controller.
 * @param data    Pointer to the source data to be written (must be at least 64 bits).
 * @param address Destination address in flash memory where the data will be written.
 *
 * @retval FLASH_MCHP_SUCCESS if the write operation is successful.
 */
static int flash_doubleword_write(const struct device *dev, const void *data, uint32_t address)
{
	int ret = EINVAL;
	uint8_t num_words = FLASH_MCHP_DOUBLE_WORD_SIZE / sizeof(uint32_t);
	const uint32_t *src = (const uint32_t *)data;
	uint32_t *dst = FLASH_MEMORY(address);

	flash_pagebuffer_clear(dev);

	flash_set_write_mode(dev, NVMCTRL_WMODE_ADW);

	/* writing 32-bit data into the given address. Writes to the page buffer must be 32 bits */
	for (uint8_t i = 0U; i < num_words; i++) {
		*dst = *src;
		dst++;
		src++;
	}

	flash_status_ready_wait(dev);

	ret = flash_get_interrupt_status_error(dev);

	return ret;
}

/**
 * @brief Write a quad word (128 bits) to flash memory.
 *
 * This function writes a quad word (typically 128 bits) to the specified flash memory address.
 * The data is written in 32-bit chunks, as required by the flash controller's page buffer.
 * The function waits for the flash to be ready before writing and checks the status after the
 * operation.
 *
 * @param dev     Pointer to the device structure representing the flash controller.
 * @param data    Pointer to the source data to be written (must be at least 128 bits).
 * @param address Destination address in flash memory where the data will be written.
 *
 * @retval FLASH_MCHP_SUCCESS if the write operation is successful.
 */
static int flash_quadword_write(const struct device *dev, const void *data, uint32_t address)
{
	int ret = EINVAL;
	uint8_t num_words = FLASH_MCHP_QUAD_WORD_SIZE / sizeof(uint32_t);
	const uint32_t *src = (const uint32_t *)data;
	uint32_t *dst = FLASH_MEMORY(address);

	flash_pagebuffer_clear(dev);

	flash_set_write_mode(dev, NVMCTRL_WMODE_AQW);

	/* writing 32-bit data into the given address. Writes to the page buffer must be 32 bits */
	for (uint8_t i = 0U; i < num_words; i++) {
		*dst = *src;
		dst++;
		src++;
	}

	flash_status_ready_wait(dev);

	ret = flash_get_interrupt_status_error(dev);

	return ret;
}

/**
 * @brief Erases a memory block in the Microchip NVMCTRL.
 *
 * This function issues a command to erase a block of memory at the specified
 * address in the Non-Volatile Memory Controller (NVMCTRL). It prepares the
 * controller to accept a new command, sets the address, and executes the
 * erase block command. The function then checks the status to ensure the
 * operation was successful.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 * @param address The memory address of the block to be erased.
 *
 * @retval FLASH_MCHP_SUCCESS if the erase operation is successful.
 */
static int flash_erase_block(const struct device *dev, uint32_t address)
{
	int ret = EINVAL;

	/* Set address and command */
	NVM_REGS->NVMCTRL_ADDR = address;
	flash_process_command(dev, NVMCTRL_CTRLB_CMD_EB);

	flash_status_ready_wait(dev);

	ret = flash_get_interrupt_status_error(dev);

	return ret;
}

/**
 * @brief Writes a page of data to flash memory at the specified address.
 *
 * This function writes a block of 32-bit data to the flash memory page starting
 * at the given address. The data is written in 32-bit words, and the write
 * operation is performed in page mode. The function waits until the flash is
 * ready, sets the appropriate write mode, and then writes the data to the page
 * buffer. After writing, it checks the status of the operation.
 *
 * @param dev     Pointer to the device structure representing the flash controller.
 * @param data    Pointer to the source data buffer to be written (must be 32-bit aligned).
 * @param address Destination address in flash memory where the data will be written.
 *
 * @retval FLASH_MCHP_SUCCESS if the write operation is successful.
 */
static int flash_page_write(const struct device *dev, const void *data, uint32_t address)
{
	int ret = EINVAL;
	uint8_t num_words = FLASH_MCHP_PAGE_SIZE / sizeof(uint32_t);
	const uint32_t *src = (const uint32_t *)data;
	uint32_t *dst = FLASH_MEMORY(address);

	flash_pagebuffer_clear(dev);

	flash_set_write_mode(dev, NVMCTRL_WMODE_AP);

	/* Writes to the page buffer must be 32 bits */
	for (uint8_t i = 0U; i < num_words; i++) {
		*dst = *src;
		dst++;
		src++;
	}

	flash_status_ready_wait(dev);

	ret = flash_get_interrupt_status_error(dev);

	return ret;
}

/**
 * @brief Validate the range of a flash memory operation.
 *
 * This function checks whether the specified offset and length are within
 * the valid range of the flash memory. It ensures that the offset is not
 * negative and that the operation does not exceed the total flash size.
 *
 * @param offset The starting offset of the flash memory operation.
 * @param len The length of the flash memory operation.
 *
 * @return 0 if the range is valid, -EINVAL if the range is invalid.
 */
static int flash_valid_range(off_t offset, size_t len)
{
	if (offset < 0) {
		LOG_WRN("0x%lx: before start of flash", (long)offset);
		return -EINVAL;
	}
	if ((offset + len) > SOC_NV_FLASH_SIZE) {
		LOG_WRN("0x%lx: ends past the end of flash", (long)offset);
		return -EINVAL;
	}

	return FLASH_MCHP_SUCCESS;
}

#ifdef CONFIG_FLASH_HAS_UNALIGNED_WRITE
/**
 * @brief Handles unaligned start of a flash write operation.
 *
 * This function performs a read-modify-write for the initial unaligned bytes
 * at the start of a flash write. It updates the offset, buffer pointer, and
 * remaining length to reflect the bytes written, so that the caller can proceed
 * with aligned writes.
 *
 * @param dev     Pointer to the device structure representing the flash controller.
 * @param offset  Pointer to the current offset in flash memory; updated by this function.
 * @param buffer  Pointer to the current data buffer pointer; updated by this function.
 * @param len     Pointer to the remaining length to write; updated by this function.
 *
 * @return FLASH_MCHP_SUCCESS (0) on success, or a negative error code on failure.
 */
static int flash_handle_unaligned_start(const struct device *dev, off_t *offset,
					const uint8_t **buffer, size_t *len)
{
	/* Offset is already aligned, nothing to do */
	if (flash_aligned(*offset, FLASH_MCHP_DOUBLE_WORD_SIZE) == FLASH_MCHP_SUCCESS) {
		return FLASH_MCHP_SUCCESS;
	}

	int ret = -EINVAL;
	uint32_t aligned_addr = *offset & ~(FLASH_MCHP_DOUBLE_WORD_SIZE - 1);
	uint8_t doubleword_buf[FLASH_MCHP_DOUBLE_WORD_SIZE];
	size_t start_offset = (*offset - aligned_addr);
	const uint8_t *src = (const uint8_t *)aligned_addr;
	size_t bytes_to_update = ((*len) < (FLASH_MCHP_DOUBLE_WORD_SIZE - start_offset))
					 ? (*len)
					 : (FLASH_MCHP_DOUBLE_WORD_SIZE - start_offset);

	/*  Read existing data. */
	for (size_t i = 0; i < FLASH_MCHP_DOUBLE_WORD_SIZE; i++) {
		doubleword_buf[i] = src[i];
	}

	/* Overwrite the relevant bytes. */
	for (size_t i = 0; i < bytes_to_update; i++) {
		doubleword_buf[start_offset + i] = (*buffer)[i];
	}

	ret = flash_doubleword_write(dev, doubleword_buf, aligned_addr);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_ERR("double word write failed at 0x%lx", (long)aligned_addr);
		return ret;
	}

	(*offset) += bytes_to_update;
	(*buffer) += bytes_to_update;
	(*len) -= bytes_to_update;

	return ret;
}

/**
 * @brief Handles unaligned end of a flash write operation.
 *
 * This function performs a read-modify-write for the final unaligned bytes
 * at the end of a flash write. It does not update the offset, buffer, or length,
 * as it is intended to be called after all aligned writes are complete.
 *
 * @param dev     Pointer to the device structure representing the flash controller.
 * @param offset  Offset in flash memory where the unaligned write should begin.
 * @param buffer  Pointer to the data buffer containing the bytes to write.
 * @param len     Number of bytes to write at the end (less than a double word).
 *
 * @return FLASH_MCHP_SUCCESS (0) on success, or a negative error code on failure.
 */
static int flash_handle_unaligned_end(const struct device *dev, off_t offset, const uint8_t *buffer,
				      size_t len)
{
	int ret = -EINVAL;
	uint32_t aligned_addr = offset;
	uint8_t doubleword_buf[FLASH_MCHP_DOUBLE_WORD_SIZE];
	const uint8_t *src = (const uint8_t *)aligned_addr;

	/* Read existing data */
	for (size_t i = 0; i < FLASH_MCHP_DOUBLE_WORD_SIZE; i++) {
		doubleword_buf[i] = src[i];
	}

	/* Overwrite the relevant bytes. */
	for (size_t i = 0; i < len; i++) {
		doubleword_buf[i] = buffer[i];
	}

	ret = flash_doubleword_write(dev, doubleword_buf, aligned_addr);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_ERR("double word write failed at 0x%lx", (long)aligned_addr);
	}

	return ret;
}
#endif /*CONFIG_FLASH_HAS_UNALIGNED_WRITE*/

/**
 * @brief Write data to flash memory in blocks as specified by the device tree.
 *
 * This function writes data to flash memory using the block size defined
 * in the device tree (such as 8, 16, or 512 bytes). It checks alignment and
 * remaining data length, and writes each block in turn, updating the offset and
 * buffer pointer as it goes. If a write fails, it stops and returns an error.
 *
 * @param dev    Pointer to the flash controller device.
 * @param offset Pointer to the offset in flash memory to start writing.
 * @param buffer Pointer to the data buffer pointer.
 * @param len    Pointer to the number of bytes left to write.
 *
 * @return FLASH_MCHP_SUCCESS (0) on success, or a negative error code if a write fails.
 */
static int flash_write_aligned_blocks(const struct device *dev, off_t *offset,
				      const uint8_t **buffer, size_t *len)
{
	int ret = -EINVAL;

	/* Writing 0 bytes shall succeed */
	if (*len == 0) {
		return FLASH_MCHP_SUCCESS;
	}

	/* Return error if data length is less than minimum write block size */
	if (*len < FLASH_MCHP_DOUBLE_WORD_SIZE) {
		return ret;
	}

	while (*len >= FLASH_MCHP_DOUBLE_WORD_SIZE) {

		if ((*len >= FLASH_MCHP_PAGE_SIZE) &&
		    (flash_aligned(*offset, FLASH_MCHP_PAGE_SIZE) == FLASH_MCHP_SUCCESS)) {
			ret = flash_page_write(dev, *buffer, *offset);
			if (ret != FLASH_MCHP_SUCCESS) {
				LOG_ERR("page write failed at 0x%lx", (long)*offset);
				break;
			}
			*offset += FLASH_MCHP_PAGE_SIZE;
			*buffer += FLASH_MCHP_PAGE_SIZE;
			*len -= FLASH_MCHP_PAGE_SIZE;
			continue;
		}

		if ((*len >= FLASH_MCHP_QUAD_WORD_SIZE) &&
		    (flash_aligned(*offset, FLASH_MCHP_QUAD_WORD_SIZE) == FLASH_MCHP_SUCCESS)) {
			ret = flash_quadword_write(dev, *buffer, *offset);
			if (ret != FLASH_MCHP_SUCCESS) {
				LOG_ERR("quad word write failed at 0x%lx", (long)*offset);
				break;
			}
			*offset += FLASH_MCHP_QUAD_WORD_SIZE;
			*buffer += FLASH_MCHP_QUAD_WORD_SIZE;
			*len -= FLASH_MCHP_QUAD_WORD_SIZE;
			continue;
		}

		if ((*len >= FLASH_MCHP_DOUBLE_WORD_SIZE) &&
		    (flash_aligned(*offset, FLASH_MCHP_DOUBLE_WORD_SIZE) == FLASH_MCHP_SUCCESS)) {
			ret = flash_doubleword_write(dev, *buffer, *offset);
			if (ret != FLASH_MCHP_SUCCESS) {
				LOG_ERR("double word write failed at 0x%lx", (long)*offset);
				break;
			}
			*offset += FLASH_MCHP_DOUBLE_WORD_SIZE;
			*buffer += FLASH_MCHP_DOUBLE_WORD_SIZE;
			*len -= FLASH_MCHP_DOUBLE_WORD_SIZE;
			continue;
		}

		break;
	}

	return ret;
}

/**
 * @brief Write data to flash memory, supporting both aligned and unaligned writes.
 *
 * This function writes data to flash memory at the given offset. It checks that the
 * write is within a valid memory range and, if needed, handles unaligned start and end
 * regions. For the main part of the write, it uses the write block size specified in
 * the device tree (such as 8, 16, or 512 bytes). The function also locks the flash
 * data structure during the operation to ensure thread safety.
 *
 * @param dev    Flash controller device pointer.
 * @param offset Offset in flash memory to start writing.
 * @param data   Pointer to the data buffer to write.
 * @param len    Number of bytes to write.
 *
 * @return FLASH_MCHP_SUCCESS (0) on success, -EINVAL for alignment errors, or a negative error code
 * on failure.
 */
static int flash_mchp_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	int ret = -EINVAL;
	struct flash_mchp_dev_data *mchp_flash_data = dev->data;
	const uint8_t *buffer = (const uint8_t *)data;

	offset += DEV_CFG(dev)->base_addr;

	ret = flash_valid_range(offset, len);
	if (ret != FLASH_MCHP_SUCCESS) {
		return ret;
	}

#ifndef CONFIG_FLASH_HAS_UNALIGNED_WRITE
	ret = flash_aligned(offset, SOC_NV_FLASH_WRITE_BLOCK_SIZE);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_WRN("0x%lx: not on a write block boundary", (long)offset);
		return ret;
	}

	ret = flash_aligned(len, SOC_NV_FLASH_WRITE_BLOCK_SIZE);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_WRN("%zu: not a integer number of write blocks", len);
		return ret;
	}
#endif /*!CONFIG_FLASH_HAS_UNALIGNED_WRITE*/

	k_mutex_lock(&mchp_flash_data->flash_data_lock, K_MSEC(10));

#ifdef CONFIG_FLASH_HAS_UNALIGNED_WRITE
	/* Handle unaligned start */
	ret = flash_handle_unaligned_start(dev, &offset, &buffer, &len);
	if (ret != FLASH_MCHP_SUCCESS) {
		k_mutex_unlock(&mchp_flash_data->flash_data_lock);
		LOG_ERR("flash unaligned write at start failed: %d", ret);
		return ret;
	}
#endif

	ret = flash_write_aligned_blocks(dev, &offset, &buffer, &len);
	if (ret != FLASH_MCHP_SUCCESS) {
		k_mutex_unlock(&mchp_flash_data->flash_data_lock);
		LOG_ERR("flash aligned write failed: %d", ret);
		return ret;
	}

#ifdef CONFIG_FLASH_HAS_UNALIGNED_WRITE
	/* Handle unaligned end */
	if (len > 0) {
		ret = flash_handle_unaligned_end(dev, offset, buffer, len);
	}
#endif /*CONFIG_FLASH_HAS_UNALIGNED_WRITE*/

	k_mutex_unlock(&mchp_flash_data->flash_data_lock);

	return ret;
}

/**
 * @brief Erases the flash memory block containing the specified address.
 *
 * This function erases the block of flash memory in which the given offset resides.
 * The offset must be aligned to the erase block size, and the size must be a multiple
 * of the erase block size. The function locks the flash data structure during the
 * operation to ensure thread safety, and it checks for valid range and alignment
 * before proceeding. The function waits for the flash to be ready before performing
 * the erase operation.
 *
 * @param dev    Pointer to the device structure representing the flash controller.
 * @param offset Offset in flash memory where the erase should begin (relative to base address).
 * @param size   Number of bytes to erase (must be a multiple of the erase block size).
 *
 * @return FLASH_MCHP_SUCCESS (0) on success,
 *         -EINVAL if alignment requirements are not met,
 *         -EIO if an erase operation fails,
 *         or other error codes as appropriate.
 */
static int flash_mchp_erase(const struct device *dev, off_t offset, size_t size)
{
	int ret = -EINVAL;
	struct flash_mchp_dev_data *mchp_flash_data = dev->data;
	uint32_t page_size = SOC_NV_FLASH_ERASE_BLOCK_SIZE;

	offset += DEV_CFG(dev)->base_addr;

	ret = flash_valid_range(offset, size);
	if (ret != FLASH_MCHP_SUCCESS) {
		return ret;
	}

	ret = flash_aligned(offset, page_size);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_WRN("0x%lx: not on a erase block boundary", (long)offset);
		return ret;
	}

	ret = flash_aligned(size, page_size);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_WRN("%zu: not a integer number of erase blocks", size);
		return ret;
	}

	k_mutex_lock(&mchp_flash_data->flash_data_lock, K_MSEC(10));

	while (size > 0U) {

		/* Erase the block */
		ret = flash_erase_block(dev, offset);
		if (ret != FLASH_MCHP_SUCCESS) {
			LOG_ERR("erase operation failed at 0x%lx", (long)offset);
			ret = -EIO;
			break;
		}

		/* Update size and offset for the next pages */
		size -= page_size;
		offset += page_size;
	}

	k_mutex_unlock(&mchp_flash_data->flash_data_lock);

	return ret;
}

/**
 * @brief Read data from the flash memory.
 *
 * This function reads a specified number of bytes from the flash memory
 * at a given offset and copies it into the provided buffer.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 * @param offset Offset in the flash memory from which to start reading.
 * @param data Pointer to the buffer where the read data will be stored.
 * @param len Number of bytes to read from the flash memory.
 *
 * @return Returns NVMCTRL_MCHP_SUCCESS upon successful completion, or a negative error code
 * if the operation fails.
 */
static int flash_mchp_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	int ret = -EINVAL;
	struct flash_mchp_dev_data *mchp_flash_data = dev->data;
	uint32_t flash_base_addr = DEV_CFG(dev)->base_addr;

	ret = flash_valid_range(offset, len);
	if (ret != FLASH_MCHP_SUCCESS) {
		return ret;
	}

	uint8_t *dst = (uint8_t *)data;
	const uint8_t *src = (const uint8_t *)flash_base_addr + offset;

	k_mutex_lock(&mchp_flash_data->flash_data_lock, K_MSEC(10));

	for (size_t i = 0; i < len; i++) {
		dst[i] = src[i];
	}

	k_mutex_unlock(&mchp_flash_data->flash_data_lock);

	return ret;
}

/**
 * @brief Retrieve the flash parameters for a given device.
 *
 * This function provides access to the flash parameters associated with
 * a specific flash device. The parameters include details such as the
 * minimal write alignment and size, device capabilities, and the value
 * used to fill erased areas of the flash memory.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 *
 * @return Pointer to a `flash_parameters` structure containing the flash
 *         device's parameters. The returned structure includes:
 *         - `write_block_size`: The minimal write alignment and size.
 *         - `caps.no_explicit_erase`: Indicates whether the device requires
 *           explicit erase operations or not.
 *         - `erase_value`: The value used to fill erased areas of the flash memory.
 */
static const struct flash_parameters *flash_mchp_get_parameters(const struct device *dev)
{
	return (&DEV_CFG(dev)->flash_param);
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
/**
 * @brief Retrieve the flash page layout for a Microchip NVM controller.
 *
 * This function provides the layout of flash pages for the specified device.
 * It retrieves the page layout and the size of the layout from the device's
 * configuration and assigns them to the provided pointers.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 * @param layout Pointer to store the address of the flash pages layout array.
 * @param layout_size Pointer to store the size of the flash pages layout array.
 *
 */
static void flash_mchp_page_layout(const struct device *dev,
				   const struct flash_pages_layout **layout, size_t *layout_size)
{
	*layout = &(DEV_CFG(dev)->flash_layout);
	*layout_size = FLASH_MCHP_LAYOUT_SIZE;
}
#endif /*CONFIG_FLASH_PAGE_LAYOUT*/

#ifdef CONFIG_FLASH_EX_OP_ENABLED
/**
 * @brief Determines if a given address is outside the user row range in NVMCTRL.
 *
 * This function checks whether the specified address falls outside the
 * defined user row range in the NVMCTRL memory. If the address is outside
 * the range, it returns a failure code; otherwise, it returns a success code.
 *
 * @param address The address to be checked.
 * @return Returns FLASH_MCHP_SUCCESS if the address is within the user row range,
 *         otherwise returns -EINVAL if the address is outside the range.
 */
static int flash_check_offset_user_range(uint32_t address)
{
	int ret = FLASH_MCHP_SUCCESS;

	/* Check if the address is outside the user row range */
	if ((address < SOC_NV_USERROW_BASE_ADDR) ||
	    (address > (SOC_NV_USERROW_BASE_ADDR + SOC_NV_USERROW_SIZE))) {
		ret = -EINVAL;
	}

	return ret;
}

/**
 * @brief Erases a user row in the Microchip NVMCTRL.
 *
 * This function issues a command to erase a user row at the specified address
 * in the Non-Volatile Memory Controller (NVMCTRL). It prepares the controller
 * to accept a new command, sets the address, and executes the erase row
 * command. The function then checks the status to ensure the operation was
 * successful.
 *
 * @param[in] dev Pointer to the device structure representing the NVMCTRL hardware instance.
 * @param address The memory address of the user row to be erased.
 *
 * @retval FLASH_MCHP_SUCCESS if the erase operation is successful.
 */
static int flash_user_row_erase(const struct device *dev, uint32_t address)
{
	int ret = -EINVAL;

	/* Set address and command */
	NVM_REGS->NVMCTRL_ADDR = address;
	flash_process_command(dev, NVMCTRL_CTRLB_CMD_EP);

	flash_status_ready_wait(dev);

	ret = flash_get_interrupt_status_error(dev);

	return ret;
}

/**
 * @brief Writes data to the user row area of flash memory.
 *
 * This function writes a buffer of data to the user row section of flash memory,
 * starting at the specified offset. The address and data length must be aligned
 * to the user row write block size. The function checks for valid address range
 * and alignment before performing the write operation. Data is written in
 * quad-word (typically 16-byte) blocks.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 * @param in  Pointer to a flash_mchp_ex_op_userrow_data_t structure containing
 *            the offset, data pointer, and data length to be written.
 * @param out Unused output pointer (reserved for future use).
 *
 * @return FLASH_MCHP_SUCCESS (0) on success,
 *         -EINVAL if alignment requirements are not met,
 *         or other error codes as appropriate.
 */
static int flash_ex_op_user_row_write(const struct device *dev, const uintptr_t in, void *out)
{
	ARG_UNUSED(out);
	int ret = -EINVAL;
	struct flash_mchp_dev_data *mchp_flash_data = dev->data;

	const flash_mchp_ex_op_userrow_data_t *userrow_data =
		(const flash_mchp_ex_op_userrow_data_t *)in;

	const uint8_t *buffer = (const uint8_t *)userrow_data->data;
	uint32_t address = userrow_data->offset + SOC_NV_USERROW_BASE_ADDR;
	size_t len = userrow_data->data_len;

	ret = flash_aligned(address, SOC_NV_USERROW_WRITE_BLOCK_SIZE);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_WRN("0x%lx: not on a write block boundary", (long)address);
		return ret;
	}

	ret = flash_aligned(len, SOC_NV_USERROW_WRITE_BLOCK_SIZE);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_WRN("%zu: not a integer number of write blocks", len);
		return ret;
	}

	ret = flash_check_offset_user_range(address);
	if (ret != FLASH_MCHP_SUCCESS) {
		return ret;
	}

	uint32_t num_quad_words = (len / SOC_NV_USERROW_WRITE_BLOCK_SIZE);

	k_mutex_lock(&mchp_flash_data->flash_data_lock, K_MSEC(10));

	for (uint32_t write_count = 0U; write_count < num_quad_words; write_count++) {
		ret = flash_quadword_write(dev, buffer, address);
		if (ret != FLASH_MCHP_SUCCESS) {
			break;
		}

		buffer += SOC_NV_USERROW_WRITE_BLOCK_SIZE;
		address += SOC_NV_USERROW_WRITE_BLOCK_SIZE;
	}

	k_mutex_unlock(&mchp_flash_data->flash_data_lock);

	return ret;
}

/**
 * @brief Erases the user row area of flash memory.
 *
 * This function erases the entire user row section of flash memory, starting at
 * the base address defined by SOC_NV_USERROW_BASE_ADDR. It waits for the flash
 * to be ready before performing the erase operation. The input and output
 * parameters are currently unused.
 *
 * @param dev Pointer to the device structure representing the flash controller.
 * @param in  Unused input parameter (reserved for future use).
 * @param out Unused output parameter (reserved for future use).
 *
 * @return FLASH_MCHP_SUCCESS (0) on success,
 *         -EIO if the erase operation fails.
 */
static int flash_ex_op_user_row_erase(const struct device *dev, const uintptr_t in, void *out)
{
	ARG_UNUSED(in);
	ARG_UNUSED(out);

	int ret = -EINVAL;
	struct flash_mchp_dev_data *mchp_flash_data = dev->data;

	k_mutex_lock(&mchp_flash_data->flash_data_lock, K_MSEC(10));

	/* Erase the user page */
	ret = flash_user_row_erase(dev, SOC_NV_USERROW_BASE_ADDR);
	if (ret != FLASH_MCHP_SUCCESS) {
		LOG_ERR("User page erase failed");
		ret = -EIO;
	}

	k_mutex_unlock(&mchp_flash_data->flash_data_lock);

	return ret;
}

/**
 * @brief Lock all regions of the SoC non-volatile flash.
 *
 * This function iterates over all lock regions of the SoC non-volatile flash and issues
 * a lock command for each region. It waits for the flash to be ready before issuing each command.
 *
 * @param dev Pointer to the flash device structure.
 * @param in  Unused input parameter (reserved for future use or interface compatibility).
 * @param out Unused output parameter (reserved for future use or interface compatibility).
 *
 * @retval FLASH_MCHP_SUCCESS if all regions are successfully locked.
 */
static int flash_ex_op_region_lock(const struct device *dev, const uintptr_t in, void *out)
{
	ARG_UNUSED(in);
	ARG_UNUSED(out);
	int ret = -EINVAL;

	for (off_t offset = 0; offset < SOC_NV_FLASH_SIZE;
	     offset += SOC_NV_FLASH_LOCK_REGION_SIZE) {

		/* Set address and command */
		NVM_REGS->NVMCTRL_ADDR = offset + SOC_NV_FLASH_BASE_ADDRESS;
		flash_process_command(dev, NVMCTRL_CTRLB_CMD_LR);

		ret = flash_get_interrupt_status_error(dev);
		if (ret != FLASH_MCHP_SUCCESS) {
			break;
		}
	}

	return ret;
}

/**
 * @brief Unlock all regions of the SoC non-volatile flash.
 *
 * This function iterates over all lock regions of the SoC non-volatile flash and issues
 * an unlock command for each region. It waits for the flash to be ready before issuing each
 * command.
 *
 * @param dev Pointer to the flash device structure.
 * @param in  Unused input parameter (reserved for future use or interface compatibility).
 * @param out Unused output parameter (reserved for future use or interface compatibility).
 *
 * @retval FLASH_MCHP_SUCCESS if all regions are successfully unlocked.
 */
static int flash_ex_op_region_unlock(const struct device *dev, const uintptr_t in, void *out)
{
	ARG_UNUSED(in);
	ARG_UNUSED(out);
	int ret = -EINVAL;

	for (off_t offset = 0; offset < SOC_NV_FLASH_SIZE;
	     offset += SOC_NV_FLASH_LOCK_REGION_SIZE) {

		/* Set address and command */
		NVM_REGS->NVMCTRL_ADDR = offset + SOC_NV_FLASH_BASE_ADDRESS;
		flash_process_command(dev, NVMCTRL_CTRLB_CMD_UR);

		ret = flash_get_interrupt_status_error(dev);
		if (ret != FLASH_MCHP_SUCCESS) {
			break;
		}
	}

	return ret;
}

/**
 * @brief Executes an extended flash operation based on the provided operation code.
 *
 * This function acts as a dispatcher for various extended flash operations, such as
 * erasing or writing the user row, and locking or unlocking flash regions. The specific
 * operation to perform is determined by the @p code parameter.
 *
 * @param dev  Pointer to the device structure representing the flash controller.
 * @param code Operation code specifying which extended operation to perform.
 * @param in   Pointer to input data required by the operation (usage depends on operation).
 * @param out  Pointer to output data buffer (usage depends on operation).
 *
 * @return FLASH_MCHP_SUCCESS (0) on success,
 *         -EINVAL if the operation code is invalid,
 *         or other error codes as returned by the respective functions.
 */
static int flash_mchp_ex_op(const struct device *dev, uint16_t code, const uintptr_t in, void *out)
{
	int ret = -EINVAL;

	switch (code) {
	case FLASH_EX_OP_USER_ROW_ERASE:
		ret = flash_ex_op_user_row_erase(dev, in, out);
		break;
	case FLASH_EX_OP_USER_ROW_WRITE:
		ret = flash_ex_op_user_row_write(dev, in, out);
		break;
	case FLASH_EX_OP_REGION_LOCK:
		ret = flash_ex_op_region_lock(dev, in, out);
		break;
	case FLASH_EX_OP_REGION_UNLOCK:
		ret = flash_ex_op_region_unlock(dev, in, out);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
#endif /*CONFIG_FLASH_EX_OP_ENABLED*/

/**
 * @brief Interrupt Service Routine for the Microchip NVMCTRL peripheral.
 *
 * This function handles interrupts from the Microchip NVMCTRL peripheral.
 * It clears the interrupt flag to acknowledge the interrupt and releases
 * a semaphore to allow other operations to proceed.
 *
 * @param dev Pointer to the device structure for the flash controller.
 */
static void flash_mchp_isr(const struct device *dev)
{
	flash_clear_interrupt_flag(dev);
}

/**
 * @brief Initializes the Microchip NVMCTRL peripheral.
 *
 * This function sets up the necessary resources and configurations for the
 * Microchip flash memory controller to operate. It initializes mutexes and
 * semaphores, enables the clock for the controller, configures interrupts,
 * and performs any necessary hardware initialization.
 *
 * @param dev Pointer to the device structure for the flash controller.
 *
 * @return Returns 0 upon successful initialization.
 *
 */
static int flash_mchp_init(const struct device *dev)
{
	int ret = -EINVAL;

	const struct flash_mchp_dev_config *const mchp_flash_cfg = DEV_CFG(dev);
	struct flash_mchp_dev_data *mchp_flash_data = dev->data;

	ret = clock_control_on(mchp_flash_cfg->flash_clock.clock_dev,
			       mchp_flash_cfg->flash_clock.mclk_sys);

	if ((ret == FLASH_MCHP_SUCCESS) || (ret == -EALREADY)) {

		k_mutex_init(&(mchp_flash_data->flash_data_lock));

		mchp_flash_cfg->irq_config_func(dev);

		flash_controller_init(dev);

		ret = FLASH_MCHP_SUCCESS;
	}

	return ret;
}

/**
 * @brief NVMCTRL driver API structure.
 */
static DEVICE_API(flash, flash_mchp_api) = {
	.write = flash_mchp_write,
	.read = flash_mchp_read,
	.erase = flash_mchp_erase,
	.get_parameters = flash_mchp_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_mchp_page_layout,
#endif /*CONFIG_FLASH_PAGE_LAYOUT*/
#ifdef CONFIG_FLASH_EX_OP_ENABLED
	.ex_op = flash_mchp_ex_op,
#endif /*CONFIG_FLASH_EX_OP_ENABLED*/
};

/**
 * @brief Declare the FLASH IRQ handler.
 *
 * @param n Instance number.
 */
#define FLASH_MCHP_IRQ_HANDLER_DECL(n)                                                             \
	static void flash_mchp_irq_config_##n(const struct device *dev)

/**
 * @brief Define and connect the FLASH IRQ handler for a given instance.
 *
 * This macro defines the IRQ configuration function for the specified FLASH instance,
 * connects the FLASH interrupt to its handler, and enables the IRQ.
 *
 * @param n Instance number.
 */
#define FLASH_MCHP_IRQ_HANDLER(n)                                                                  \
	static void flash_mchp_irq_config_##n(const struct device *dev)                            \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    flash_mchp_isr, DEVICE_DT_INST_GET(n), 0);                             \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
	}

/**
 * @brief Configures the flash memory page layout for the FLASH.
 *
 */
#ifdef CONFIG_FLASH_PAGE_LAYOUT
#define FLASH_LAYOUT                                                                               \
	.flash_layout = {.pages_count = SOC_NV_FLASH_SIZE / SOC_NV_FLASH_ERASE_BLOCK_SIZE,         \
			 .pages_size = SOC_NV_FLASH_ERASE_BLOCK_SIZE},
#else
#define FLASH_LAYOUT
#endif /*CONFIG_FLASH_PAGE_LAYOUT*/

/*
 * @brief Define the FLASH configuration.
 *
 * @param n Instance number.
 */
#define FLASH_MCHP_CONFIG_DEFN(n)                                                                  \
	static const struct flash_mchp_dev_config flash_mchp_config_##n = {                        \
		.regs = (nvmctrl_registers_t *)DT_INST_REG_ADDR(n),                                \
		.base_addr = SOC_NV_FLASH_BASE_ADDRESS,                                            \
		.flash_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                       \
		.flash_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)), \
		.irq_config_func = flash_mchp_irq_config_##n,                                      \
		.flash_param = {.write_block_size = SOC_NV_FLASH_WRITE_BLOCK_SIZE,                 \
				.caps = {.no_explicit_erase = false},                              \
				.erase_value = FLASH_ERASE_DEFAULT_VALUE},                         \
		FLASH_LAYOUT}

/**
 * @brief Macro to define the flash data structure for a specific instance.
 *
 * This macro defines the flash data structure for a specific instance of the Microchip flash
 * device.
 *
 * @param n Instance number.
 */
#define FLASH_MCHP_DATA_DEFN(n) static struct flash_mchp_dev_data flash_mchp_data_##n

/**
 * @brief Macro to define the device structure for a specific instance of the flash device.
 *
 * This macro defines the device structure for a specific instance of the Microchip flash device.
 * It uses the DEVICE_DT_INST_DEFINE macro to create the device instance with the specified
 * initialization function, data structure, configuration structure, and driver API.
 *
 * @param n Instance number.
 */
#define FLASH_MCHP_DEVICE_DT_DEFN(n)                                                               \
	DEVICE_DT_INST_DEFINE(n, flash_mchp_init, NULL, &flash_mchp_data_##n,                      \
			      &flash_mchp_config_##n, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_mchp_api)

/**
 * @brief Initialize the FLASH device.
 *
 * @param n Instance number.
 */
#define FLASH_MCHP_DEVICE_INIT(n)                                                                  \
	FLASH_MCHP_IRQ_HANDLER_DECL(n);                                                            \
	FLASH_MCHP_CONFIG_DEFN(n);                                                                 \
	FLASH_MCHP_DATA_DEFN(n);                                                                   \
	FLASH_MCHP_DEVICE_DT_DEFN(n);                                                              \
	FLASH_MCHP_IRQ_HANDLER(n);

/**
 * @brief Initialize all FLASH instances.
 */
DT_INST_FOREACH_STATUS_OKAY(FLASH_MCHP_DEVICE_INIT)
