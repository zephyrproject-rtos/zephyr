#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

/* SHR Symbol Duration in ns */
#define UWB_PHY_TPSYM_PRF64		IEEE802154_PHY_HRP_UWB_PRF64_TPSYM_SYMBOL_PERIOD_NS
#define UWB_PHY_TPSYM_PRF16		IEEE802154_PHY_HRP_UWB_PRF16_TPSYM_SYMBOL_PERIOD_NS

#define UWB_PHY_NUMOF_SYM_SHR_SFD	8

/* PHR Symbol Duration Tdsym in ns */
#define UWB_PHY_TDSYM_PHR_110K		8205.13
#define UWB_PHY_TDSYM_PHR_850K		1025.64
#define UWB_PHY_TDSYM_PHR_6M8		1025.64

#define UWB_PHY_NUMOF_SYM_PHR		18

/* Data Symbol Duration Tdsym in ns */
#define UWB_PHY_TDSYM_DATA_110K		8205.13
#define UWB_PHY_TDSYM_DATA_850K		1025.64
#define UWB_PHY_TDSYM_DATA_6M8		128.21

#define DWT_WORK_QUEUE_STACK_SIZE	512

#define DWT_FCS_LENGTH			2U
#define DWT_SPI_CSWAKEUP_FREQ		500000U
#define DWT_SPI_SLOW_FREQ		2000000U
#define DWT_SPI_TRANS_MAX_HDR_LEN	3
#define DWT_SPI_TRANS_REG_MAX_RANGE	0x3F
#define DWT_SPI_TRANS_SHORT_MAX_OFFSET	0x7F
#define DWT_SPI_TRANS_WRITE_OP		BIT(7)
#define DWT_SPI_TRANS_SUB_ADDR		BIT(6)
#define DWT_SPI_TRANS_EXTEND_ADDR	BIT(7)

#define DWT_TS_TIME_UNITS_FS		15650U /* DWT_TIME_UNITS in fs */


struct dwt_phy_config {
	uint8_t channel;	/* Channel 1, 2, 3, 4, 5, 7 */
	uint8_t dr;	/* Data rate DWT_BR_110K, DWT_BR_850K, DWT_BR_6M8 */
	uint8_t prf;	/* PRF DWT_PRF_16M or DWT_PRF_64M */

	uint8_t rx_pac_l;		/* DWT_PAC8..DWT_PAC64 */
	uint8_t rx_shr_code;	/* RX SHR preamble code */
	uint8_t rx_ns_sfd;		/* non-standard SFD */
	uint16_t rx_sfd_to;	/* SFD timeout value (in symbols)
				 * (tx_shr_nsync + 1 + SFD_length - rx_pac_l)
				 */

	uint8_t tx_shr_code;	/* TX SHR preamble code */
	uint32_t tx_shr_nsync;	/* PLEN index, e.g. DWT_PLEN_64 */

	float t_shr;
	float t_phr;
	float t_dsym;
};

struct dwt_hi_cfg {
	struct spi_dt_spec bus;
	struct gpio_dt_spec irq_gpio;
	struct gpio_dt_spec rst_gpio;
};

#define DWT_STATE_TX		0
#define DWT_STATE_CCA		1
#define DWT_STATE_RX_DEF_ON	2

struct dwt_context {
	const struct device *dev;
	struct net_if *iface;
	const struct spi_config *spi_cfg;
	struct spi_config spi_cfg_slow;
	struct gpio_callback gpio_cb;
	struct k_sem dev_lock;
	struct k_sem phy_sem;
	struct k_work irq_cb_work;
	struct k_thread thread;
	struct dwt_phy_config rf_cfg;
	atomic_t state;
	bool cca_busy;
	uint16_t sleep_mode;
	uint8_t mac_addr[8];
};


static struct ieee802154_radio_api dwt_radio_api;

/* This struct is used to read all additional RX frame info at one push */
struct dwt_rx_info_regs {
	uint8_t rx_fqual[DWT_RX_FQUAL_LEN];
	uint8_t rx_ttcki[DWT_RX_TTCKI_LEN];
	uint8_t rx_ttcko[DWT_RX_TTCKO_LEN];
	/* RX_TIME without RX_RAWST */
	uint8_t rx_time[DWT_RX_TIME_FP_RAWST_OFFSET];
} _packed;
