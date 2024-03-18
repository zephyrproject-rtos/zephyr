#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

*!Serial Peripheral Interface Offsets */
#define SPI0_START 0x00020000 /* Serial Peripheral Interface 0 */
#define SPI1_START 0x00020100 /* Serial Peripheral Interface 1 */
#define SPI2_START 0x00020200 /* Serial Peripheral Interface 2 */
#define SPI3_START 0x00020300 /* Serial Peripheral Interface 3 */

/* Struct to access SSPI registers as 32 bit registers */

#define SSPI_MAX_COUNT  4 /*! Number of Standard SSPI used in the SOC */

#define SSPI0_BASE_ADDRESS  0x00020000 /*! Standard Serial Peripheral Interface 0 Base address*/
#define SSPI0_END_ADDRESS  0x000200FF /*! Standard Serial Peripheral Interface 0 Base address*/

#define SSPI1_BASE_ADDRESS  0x00020100 /*! Standard Serial Peripheral Interface 1 Base address*/
#define SSPI1_END_ADDRESS  0x000201FF /*! Standard Serial Peripheral Interface 1 Base address*/

#define SSPI2_BASE_ADDRESS  0x00020200 /*! Standard Serial Peripheral Interface 2 Base address*/
#define SSPI2_END_ADDRESS  0x000202FF /*! Standard Serial Peripheral Interface 2 Base address*/

#define SSPI3_BASE_ADDRESS  0x00020300 /*! Standard Serial Peripheral Interface 3 Base address*/
#define SSPI3_END_ADDRESS  0x000203FF /*! Standard Serial Peripheral Interface 3 Base address*/

#define SSPI_BASE_OFFSET 0x100


#define SPI_CFG(dev) ((struct spi_shakti_cfg *) ((dev)->config))
#define SPI_DATA(dev) ((struct spi_shakti_data *) ((dev)->data))


struct spi_shakti_data {
	struct spi_context ctx;
};

struct spi_shakti_cfg {
	uint32_t base;
	uint32_t f_sys;
	const struct pinctrl_dev_config *pcfg;
};


#ifdef __cplusplus
}
#endif