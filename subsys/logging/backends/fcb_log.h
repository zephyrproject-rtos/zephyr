/**
 * @file fcb_log.h
 * @brief This module enables FCB logging
 *
 */
#ifndef __FCB_LOG_H__
#define __FCB_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/device.h>
#include <zephyr/fs/fcb.h>
//#if CONFIG_DFU_TARGET_MCUBOOT
//#include <pm_config.h> /* access to PM_FCB_STORAGE_SIZE */
//#endif

#if  ( DT_HAS_CHOSEN(nordic_pm_ext_flash) )
#if  ( DT_CHOSEN(nordic_pm_ext_flash) == mx25r64)
#define FCB_SPI_FLASH_SECTOR_SIZE 4096 
#elif ( DT_CHOSEN(nordic_pm_ext_flash) == en25qh32b)
#define FCB_SPI_FLASH_SECTOR_SIZE DT_PROP( DT_CHOSEN(nordic_pm_ext_flash), erase_sector_size)
#elif ( DT_CHOSEN(nordic_pm_ext_flash) == at45db641)
#define FCB_SPI_FLASH_SECTOR_SIZE DT_PROP( DT_CHOSEN(nordic_pm_ext_flash), page_size)
#elif ( DT_CHOSEN(nordic_pm_ext_flash) == gd25wb256)
//#define FCB_SPI_FLASH_SECTOR_SIZE DT_PROP( DT_CHOSEN(nordic_pm_ext_flash), erase_sector_size)
#define FCB_SPI_FLASH_SECTOR_SIZE 4096
#else
#define FCB_SPI_FLASH_SECTOR_SIZE -1 
#endif
#else // DT_HAS_CHOSEN(nordic_pm_ext_flash)
#define FCB_SPI_FLASH_SECTOR_SIZE -1  /* currently support only nordic_pm_ext_flash  */
#endif

//size_t storage_size = FIXED_PARTITION_SIZE(fcb_storage);

#if FCB_SPI_FLASH_SECTOR_SIZE > 0
//#define FCB_SECTOR_COUNT (PM_FCB_STORAGE_SIZE /FCB_SPI_FLASH_SECTOR_SIZE)
#define FCB_SECTOR_COUNT (PARTITION_SIZE(fcb_storage) /FCB_SPI_FLASH_SECTOR_SIZE)
#else
#define FCB_SECTOR_COUNT -1  /* run time detect of flash sector size */
#endif

typedef struct log_entry_s {
	uint8_t type;       /**< log entry type */
	uint64_t timestamp; /**< log entry timestamp */
    uint8_t len;        /**< log entry data length */
    char * data;         /**< log entry data */
	/**< Flash area where the entry is placed */
}log_entry_t;

/******************************************************************************/
/* Global Function Prototypes                                                 */
/******************************************************************************/
/**
 * @brief
 * @param id
 *
 */
int fcb_init_instance(void);
int fcb_append_log_entry(uint8_t * log_data, size_t len);
void fcb_print_Sector(uint16_t sector_no);
struct fcb * get_fcb_instance(void);
bool fcb_log_ready(void);struct 
fcb * get_fcb_instance(void);
size_t fcb_log_write_align(void);

#ifdef __cplusplus
}
#endif

#endif /* __FCB_LOG_H__ */
