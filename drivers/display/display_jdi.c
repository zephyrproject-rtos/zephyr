/*
 * Copyright 2019-22, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_flexio_jdi

#include <zephyr/drivers/display.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/counter/counter_mcux_ctimer.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dma/dma_mcux_lpc.h>

#ifdef CONFIG_HAS_MCUX_CACHE
#include <fsl_cache.h>
#endif
#include <fsl_flexio.h>
#include <fsl_gpio.h>
#include <fsl_ctimer.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(display_jdi, CONFIG_DISPLAY_LOG_LEVEL);

#if !defined(CONFIG_KERNEL_MEM_POOL) || !CONFIG_HEAP_MEM_POOL_SIZE
    #error "jdi driver need config KERNEL_MEM_POOL and CONFIG_HEAP_MEM_POOL_SIZE"
#endif

/* 1 VCK slot contains 1 HST/VCK word and 30 words of RGB data.
 * The first word is used to implement HST/VCK, and the next
 * 30 words transmit RGB data.
 */
#define PIXEL_CLK_25_VCK_LEN_WORD		31
#define PIXEL_CLK_25_VCK_NON_DATA_WORD	1
#define PIXEL_CLK_2_5_MHZ				25

/* The time (tdHST + tsHST + thHST) of 1 HST signal is maintained using 1 word */
#define PIXEL_CLK_25_HST_LEN_WORD		1

/* The number of words be sent by one input trigger */
#define ONCE_TRANS_WORD_NUM             (2)

/* DMA data: 484 VCK slots, 2 * PIXEL_CLK_25_VCK_LEN_WORD 32-bit word (@ 2.5 MHz shift clock) */
/* TODO: 485 ~ 488 slot */
#define IMAGE_DMA_DATA_SIZE_BYTES	    (PIXEL_CLK_25_VCK_LEN_WORD * 2 * 484 * sizeof(uint32_t))
#define IMAGE_DMA_DATA_SIZE_WORDS	    (IMAGE_DMA_DATA_SIZE_BYTES / sizeof(uint32_t))


/* Use SHIFTBUF[0] and SHIFTBUF[1] to cache RGB/VCK/HST data, which will be sent to jdi by shifter */
#define FLEXIO_SHIFTER_PIXEL			0
#define FLEXIO_SHIFTER_PIXEL_ADD		(FLEXIO_SHIFTER_PIXEL + 1)
/* Used to extract even and odd bits in RGB data */
#define FLEXIO_SHIFTER_AUX0				2
#define FLEXIO_SHIFTER_AUX1				3

/* VCK signal is transmitted through FLEXIO_D7.
 * So VCK signal is in the 4th bit in the data
 * that will be transmitted by SHIFTER0.
 * 
 * VCK is low when send Large Pixel Bit(LPB);
 * VCK is high when send Small Pixel Bit(SPB).
 */
#define VCK_PATTERN		    (1<<3)
#define VCK_BYTE0		    (VCK_PATTERN<< 0)
#define VCK_BYTE1		    (VCK_PATTERN<< 8)
#define VCK_BYTE2		    (VCK_PATTERN<<16)
#define VCK_BYTE3		    (VCK_PATTERN<<24)
#define VCK_BYTE0123	    (VCK_BYTE3 | VCK_BYTE2 | VCK_BYTE1 | VCK_BYTE0)
#define VCK_BYTE3210	    VCK_BYTE0123
#define VCK_BYTE123		    (VCK_BYTE3 | VCK_BYTE2 | VCK_BYTE1            )

/* HST signal is transmitted through FLEXIO_D11.
 * So HST signal is in the 8th bit in the data
 * that will be transmitted by SHIFTER0.
 */
#define HST_PATTERN			(1<<7)
#define HST_BYTE0			(HST_PATTERN<<0 )
#define HST_BYTE1			(HST_PATTERN<<8 )
#define HST_BYTE2			(HST_PATTERN<<16)
#define HST_BYTE3			(HST_PATTERN<<24)
#define HST_BYTE23			(            HST_BYTE2 | HST_BYTE3)
#define HST_BYTE123			(HST_BYTE1 | HST_BYTE2 | HST_BYTE3)

/* ctimer enum */
enum {
    PIXEL_DATA_TIMER,
    XRST_VST_DATA_TIMER,
    ENB_DATA_TIMER,
    JDI_MAX_TIMER_NUM,
};

/* flexio timer enum */
enum {
    FLEXIO_TIMER_PIXEL,

    /* HCK related flexio timers */
    FLEXIO_TIMER_HCK_TRIGGER,
    FLEXIO_TIMER_HCK,

    /* ENB related flexio timers */
    FLEXIO_TIMER_GEN,
    FLEXIO_TIMER_ENB_0 = FLEXIO_TIMER_GEN,
    FLEXIO_TIMER_ENB_1,
    FLEXIO_TIMER_ENB_2,
};

/* FLEXIO_D3 used as ENB line. It used as FLEXIO_TIMER_ENB_2 pin output */
#define FLEXIO_TIMER_ENB_PIN 		    3

/* FLEXIO_D4 ~ FLEXIO_D11 used as SHIFTER0 parallel output */
#define FLEXIO_SHIFTER_PIN_B0	    	4
#define FLEXIO_SHIFTER_PIN_G0	    	5
#define FLEXIO_SHIFTER_PIN_R0	    	6
#define FLEXIO_SHIFTER_PIN_VCK			7
#define FLEXIO_SHIFTER_PIN_B1	    	8
#define FLEXIO_SHIFTER_PIN_G1	    	9
#define FLEXIO_SHIFTER_PIN_R1	    	10
#define FLEXIO_SHIFTER_PIN_HST			11	/**/

/* FLEXIO_D12 used as FLEXIO_TIMER_HCK pin output. It also is HCK line of JDI */
#define FLEXIO_TIMER_HCK_PIN			12

/* FLEXIO_D13 used as FLEXIO_TIMER_PIXEL trigger source. It also is SPI5 CLK output */
#define FLEXIO_TIMER_TRIG_PIN			13

/* FLEXIO_D14 is FLEXIO_TIMER_ENB_0 timer output */
#define FLEXIO_TIMER_ENB_0_OUT_PIN		14

/* FLEXIO_D15 used as FLEXIO_TIMER_HCK_TRIGGER pin output. It also is FLEXIO_TIMER_HCK trigger source */
#define FLEXIO_HCK_TRIG_PIN				15

/**
 * @brief Timer match configuration
 * 
 * @param chan_id match channel, rang 0~3
 * @param match_config match config
 */
struct timer_match_config {
    uint8_t chan_id;
    struct counter_alarm_cfg match_config;
};


/**
 * @brief PIXEL_DATA_TIMER config
 * 
 * Use a timer to trigger DMA transfer of DMA data. DMA data contain XRST / VST /
 * VCK / HST / pixel data.
 * 
 * This timer needs to be configured to trigger counting
 * on both rising and falling edges.
 * 
 * Trigger DMA when counter is 1, reset Timer Counter Register when counter is 2.
 * Trigger source come from capture pin, that is @ref input_clock output.
 * 
 * When MR[0] match, send DMA data to SHIFTBUF.
 * When MR[1] match, send 0x80 to SPI5 FIFOWR.
 * When MR[2] match, the timer counter reset.
 * 
 * The detailed execution process is as follows
 *      a. Every time CTIMER0 receives a 0x80, it bursts to send 2 data to SHIFTBUF[0]
 *         and SHIFTBUF[1]
 *      b. When SHIFTER Clock come, SHIFTER0 send data to FLEXIO_D4 ~ FLEXIOD11
 */

void pixel_data_m0_alarm_callback(const struct device *dev,
                     uint8_t chan_id, uint32_t ticks,
                     void *user_data)
{
    printk("\nchan_id %d ticks %d alarm callback\n", chan_id, ticks);	
}
const struct timer_match_config pixel_data_m0_dma_match_config = {
    .chan_id = 0,		/* use match channel 0 to trigger dma */
    .match_config = {
        .callback = NULL/* pixel_data_m0_alarm_callback */,
        .user_data = NULL,
        .ticks = 1,		/* when match value, trigger dma to send DMA data to FLEXIO SHIFTBUF */
        .flags = COUNTER_ALARM_CFG_ABSOLUTE,
    },
};

/* MR1 is not used */
const struct timer_match_config pixel_clock_m1_dma_match_config = {
	.chan_id = 1,		/* use match channel 1 to trigger SPI DMA */
	.match_config = {
		.callback = NULL,
		.user_data = NULL,
		.ticks = 2,		/* when match value, trigger SPI dma to send @ref clkgen_pattern to FIFOWR */
		.flags = COUNTER_ALARM_CFG_ABSOLUTE,
	},
};

const struct timer_match_config pixel_data_reset_match_config = {
    .chan_id = 2,		/* use match channel 2 to reset timer */
    .match_config = {
        .callback = NULL,
        .user_data = NULL,
        .ticks = 2,		/* when match value, reset timer */
        .flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_AUTO_RESET,
    },
};
/**********************************************************************************************
 *                  PIXEL_DATA_TIMER CONFIG END
 *********************************************************************************************/


/**
 * @brief XRST_VST_DATA_TIMER config
 * 
 * XRST / VST of JDI use GPIO Pin to connect. and use a timer to 
 * drive these pins to generate XRST / VST signals.
 * 
 * Trigger DMA to update MR1 when counter value is equal to MR0.
 * Trigger DMA to toggle pin output and reset counter when counter 
 * value is equal to MR1.
 * Reset Timer Counter Register when counter value is equal to MR2
 * 
 * match and pin patterns for XRST/VST DMA driven GPIO control
 * match[] role of MR1:
 * dma_gpio_match[0]: XRST rising edge, coupled with dma_gpio_pin[0]
 * dma_gpio_match[1]: VST rising edge, coupled with dma_gpio_pin[1]
 * dma_gpio_match[2]: VST falling edge, coupled with dma_gpio_pin[2]
 * dma_gpio_match[3]: XRST falling edge, coupled with dma_gpio_pin[3]
 * dma_gpio_match[4]: must be set to satisfy the following: 
 *           match[4]>MR[2]; MR[2]>match[i], i=0,1,2,3
 * 
 * 
 * The detailed process of XRST / VST signal generation is as follows:
 * 
 *       a. The counter counts up until it matches MR0, then trigger
 *          DMA of MR0 to update MR1 value to DMA_GPIO_TOGGLE_XRST_RE.
 *          The counter continues to count up until it matches MR1,
 *          then trigger DMA of MR1 to toggle XRST pin output for
 *          generating XRST rising edge, then counter value reset to zero.
 * 
 *       b. The counter counts up until it matches MR0, then trigger
 *          DMA of MR0 to update MR1 value to DMA_GPIO_TOGGLE_VST_RE.
 *          The counter continues to count up until it matches MR1,
 *          then trigger DMA of MR1 to toggle VST pin output for
 *          generating VST rising edge, then counter value reset to zero.
 * 
 *       c. The counter counts up until it matches MR0, then trigger
 *          DMA of MR0 to update MR1 value to DMA_GPIO_TOGGLE_VST_FE.
 *          The counter continues to count up until it matches MR1,
 *          then trigger DMA of MR1 to toggle VST pin output for
 *          generating VST falling edge, then counter value reset to zero.
 * 
 *       d. The counter counts up until it matches MR0, then trigger
 *          DMA of MR0 to update MR1 value. The match value of the falling
 *          edge of XRST is related to the amount of DMA data pushed.
 *          So @ref dma_gpio_match[3] value need to be dynamically assigned.
 *          The counter continues to count up until it matches MR1,
 *          then trigger DMA of MR1 to toggle XRST pin output for
 *          generating XRST falling edge, then counter value reset to zero.
 * 
 *       e. After XRST falling edge is generated, counter counts up until it
 *          matches MR0, then trigger DMA of MR0 to update MR1 value to @ref 
 *          dma_gpio_match[4]. This dma_gpio_match[4] value TC count is unreachable.
 *          This is to ensure that no XRST signal will be generated during a screen
 *          refresh process. 
 * 
 *       f. After the DMA data is transferred, the timer is still active,
 *          and the TC count is not cleared. Therefore, the TC count needs
 *          to be cleared before the next screen refresh.
 * 
 * Note: 1. After all pixel data is sent, XRST fall to low level. 
 *          So the match[3] value changes dynamically.
 *       2. XRST / VST pins must be on the same port.
 * 
 * 
 */

/* During the data transmission of each line of JDI, XRST is the earliest signal.
 * After the external input clock appears, delay 5 MOSI data (40 2.4MHz SPI clocks,
 * about 14us), then generating XRST rising edge.
 * 
 * Note: The delay time could be any value.
 */
#define	DMA_GPIO_TOGGLE_XRST_RE		(5)

/* There is XRST set-up time (tsXRST) between the rising edge of XRST and the rising edge of VST.
 * The minimum value of tsXRST is 12.8us. The typical value of tsXRST is 17.6us.
 * So use 6 MOSI data (48 2.4MHz SPI clocks, about 19us) to maintain the tsXRST time.
 */
#define	DMA_GPIO_TOGGLE_VST_RE		(6)

/* There is VST set-up time (tsVST, 24us+) and VST hold time (thVST, 24.8us+) after VST rising edge.
 * So use 17 MOSI data (136 2.4MHz SPI clocks, about 57us) to maintain this time.
 */
#define VST_SETUP_TIME              (8)
#define VST_HOLD_TIME               (9)
#define	DMA_GPIO_TOGGLE_VST_FE		(17)

enum {
    XRST_RE_MATCH_ID,
    VST_RE_MATCH_ID,
    VST_FE_MATCH_ID,
    XRST_FE_MATCH_ID,
    MATCH_ID_MAX,
};

/* List of matching values for M1 */
#define DMA_GPIO_MATCH_NUM 	        (5)
static uint32_t dma_gpio_match[DMA_GPIO_MATCH_NUM] = {
    [0] = DMA_GPIO_TOGGLE_XRST_RE,
    [1] = DMA_GPIO_TOGGLE_VST_RE,
    [2] = DMA_GPIO_TOGGLE_VST_FE,
    [3] = 0x80000000,
    [4] = 0x8000000A,
};

#define DISPLAY_VST_PIN	  DT_INST_PHA_BY_NAME(0, gpios, vst, pin)
#define DISPLAY_XRST_PIN  DT_INST_PHA_BY_NAME(0, gpios, xrst, pin)

/* List of values written to the port of XRST / VST pins to toggle pin output. */
#define DMA_GPIO_PIN_NUM 	        (4)
static uint32_t dma_gpio_pin[DMA_GPIO_PIN_NUM] = {
    [0] = 1 << DISPLAY_XRST_PIN,
    [1] = 1 << DISPLAY_VST_PIN,
    [2] = 1 << DISPLAY_VST_PIN,
    [3] = 1 << DISPLAY_XRST_PIN,
};

const struct timer_match_config xrst_vst_m0_dma_match_config = {
    .chan_id = 0,		        /* use match channel 0 to trigger dma */
    .match_config = {
        .callback = NULL,
        .user_data = NULL,
        .ticks = 1,		        /* when match value, trigger dma to update MR1 of XRST_VST_DATA_TIMER */
        .flags = COUNTER_ALARM_CFG_ABSOLUTE,
    },
};

const struct timer_match_config xrst_vst_m1_dma_match_config = {
    .chan_id = 1,		        /* use match channel 1 to trigger dma */
    .match_config = {
        .callback = NULL,
        .user_data = NULL,
        .ticks = 0x80000000,	/* initial value must be >0, when match value, trigger dma to write the port pin NOT register changing the pins of interest */
        .flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_AUTO_RESET,
    },
};

const struct timer_match_config xrst_vst_reset_match_config = {
    .chan_id = 2,		        /* use match channel 2 to reset timer */
    .match_config = {
        .callback = NULL,
        .user_data = NULL,
        .ticks = 0x80000001,	/* initial value must be set as MR[2]>MR[1], when match value, timer stops and resets */
        .flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_AUTO_RESET | COUNTER_ALARM_CFG_AUTO_STOP,
    },
};
/**********************************************************************************************
 *                  XRST_VST_DATA_TIMER CONFIG END
 *********************************************************************************************/

/**
 * @brief ENB_DATA_TIMER config
 * 
 */
#define IOCON_IN_NOPUPD(x)	\
    (IOPCTL_PIO_FSEL(x) 	|	\
     IOPCTL_PIO_PUPDENA(0)	| IOPCTL_PIO_IBENA(1)	| IOPCTL_PIO_FULLDRIVE(0)	| \
     IOPCTL_PIO_AMENA(0)	| IOPCTL_PIO_ODENA(0)	| IOPCTL_PIO_IIENA(0))

/* PIO4_23: display GEN/ENB (timer output), configured as FLEXIO_D3 */
#define DISPLAY_GEN_ENB_PORT			4
#define DISPLAY_GEN_ENB_PIN				23
#define DISPLAY_GEN_ENB_FUNC			8

enum {
    ENB_DISABLE_IN_NONE,                /* initialized status */
    ENB_DISABLE_IN_PARTIAL_UPDATE,      /* ENB disable in partial update mode */
    ENB_DISABLE_IN_ALL_UPDATE,          /* ENB disable in update mode  */
};

static uint32_t dma_jdi_enb_iocon_flexio = IOCON_IN_NOPUPD(DISPLAY_GEN_ENB_FUNC);
static uint32_t dma_jdi_enb_iocon_gpio_0 = IOCON_IN_NOPUPD(0);

static struct timer_match_config enb_m0_dma_match_config = {
    .chan_id = 0,		/* use match channel 0 to trigger dma */
    .match_config = {
        .callback = NULL,
        .user_data = NULL,
        .ticks = 1,		/* when match value, trigger dma to update FLEXIO ENB pin to GPIO */
        .flags = COUNTER_ALARM_CFG_ABSOLUTE,
    },
};

static struct timer_match_config enb_reset_match_config = {
    .chan_id = 1,		/* use match channel 1 to reset timer */
    .match_config = {
        .callback = NULL,
        .user_data = NULL,
        .ticks = 0x80000000,		/* initial value must be > 0, when match value, timer stops and resets */
        .flags = COUNTER_ALARM_CFG_ABSOLUTE | COUNTER_ALARM_CFG_AUTO_RESET | COUNTER_ALARM_CFG_AUTO_STOP,
    },
};
static int jdi_enb_timer_init(const struct device *dev);
/**********************************************************************************************
 *                  ENB_DATA_TIMER CONFIG END
 *********************************************************************************************/

struct display_info {
    uint16_t panel_width;
    uint16_t panel_height;	
};

enum {
    VST_GPIO,
    XRST_GPIO,
    JDI_MAX_GPIO_NUM,
};

struct jdi_gpio_config {
    const struct gpio_dt_spec gpio;
    const GPIO_Type *gpio_base;
    const uint8_t port_no;
};

/**
 * @brief JDI configure structure
 * 
 * @param flexio_base FLEXIO register flexio_base address
 * 
 * @param display_info display information
 * 
 * @param pincfg FLEXIO pin configture
 * 
 * @param vcom_clock Use FLEXIO low frequency output function to generate VCOM/FRP/XFRP (60Hz).
 * 		  So @ref vcom_clock device is 1kHz RTC timer
 * 
 * @param input_clock input clock to flexio for transmit jdi data
 * 
 * @param backlight_gpio backlight gpio pin
 * 
 * @param timer_dev ctimer device pointer
 * 
 * @param ctimer_base ctimer register base address
 * 
 * @param jdi_gpios gpio pin device info of XRST / VST
 */
struct jdi_config {
    FLEXIO_Type *flexio_base;
    struct display_info display_info;
    void (*irq_config_func)(const struct device *dev);
    const struct pinctrl_dev_config *pincfg;
    const struct device *vcom_clock;
    const struct device *input_clock;
    const struct gpio_dt_spec backlight_gpio;
    const struct device *timer_dev[JDI_MAX_TIMER_NUM];
    const CTIMER_Type* ctimer_base[JDI_MAX_TIMER_NUM];
    const struct jdi_gpio_config jdi_gpios[JDI_MAX_GPIO_NUM];
};

/**
 * @brief DMA information during single frame transfer
 * 
 * @param dma_data Contains not only pixel data but also padding data to correspond other XRST/VST-like data
 * 
 * @param dma_data_clock_count The number of SPI output clocks required to transmit dma data
 * 
 * @param dma_data_word_count Transfer dma data number in word
 * 
 * @param rgb_baseline_dma_data_word_count the location of pixel data in @ref dma_data
 * 
 * @param jdi_enb_disable ENB end location
 * 
 * @param jdi_enb_disable_status ENB disable in update or partial-update mode
 */
struct frame_dma_data {
    uint32_t dma_data[IMAGE_DMA_DATA_SIZE_WORDS];
    uint32_t dma_data_clock_count;
    uint32_t dma_data_word_count;
    uint32_t rgb_baseline_dma_data_clock_count;
    uint32_t rgb_baseline_dma_data_word_count;
    uint32_t jdi_enb_disable;
    uint32_t jdi_enb_disable_status;
};

struct jdi_data {
    struct k_sem sem;
    struct display_capabilities cap;
    struct frame_dma_data frame;
};

static void counter_dma_callback(const struct device *dev, void *user_data,
                   uint32_t chan_id, int status)
{
    printk("\nJDI callback, chan_id %d\n", chan_id);
}

/**
 * @brief the configure struct for spi use to generate jdi clock
 * 
 * @param frequency 2.4MHz. Because HCK of jdi minimum cost 400ns (half period),
 * 		  So frequency at most 2.5MHz, and double edge trigger for ctimer.
 * 
 */
static const struct spi_config spi_cfg = {
    .frequency = 2400000,       /* The whole clock cycle is about 420ns. That is the time cost of 1 bit of MOSI data */
    .operation = SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_MODE_CPHA,
    .slave = 0,
};

/* Fixed output value of MOSI for capture by ctimer */
static const uint8_t clkgen_pattern = 0x80;

/**
 * @brief SPI output CLK to FLEXIO to shift data to JDI,
 *        and output MOSI data to CTIMER capture to trigger
 *        prepare JDI signal data.
 * 
 * @param dev is JDI display device
 * @return int 0 on success else negative errno code.
 */
static int jdi_flexio_clock_send(const struct device *dev)
{
    const struct jdi_config *config = dev->config;
    struct jdi_data *data = (struct jdi_data *)dev->data;
    struct frame_dma_data *frame = &data->frame;

    LOG_DBG("\npixel clock count %d\n", frame->dma_data_clock_count);

    uint32_t transfer_bytes = frame->dma_data_clock_count / CHAR_BIT;

    struct spi_buf tx_buf = {
        .buf = (uint8_t *)&clkgen_pattern,
        .len = transfer_bytes,
        .addr_nochange = true,
    };

    struct spi_buf_set tx = {
        .buffers = (const struct spi_buf *)&tx_buf,
        .count = 1,
    };

    int ret = spi_write(config->input_clock, &spi_cfg, &tx);
    if (ret) {
        LOG_ERR("%s fail %d\n", __FUNCTION__, ret);
    }

    return ret;
}

/**
 * @brief When PIXEL_DATA_TIMER capture input signal, transfer RGB/HST/VCK data
 *        to SHIFTBUF via DMA.
 * 
 * Note: 1. PIXEL_DATA_TIMER use MOSI data as trigger source, and transfer data
 *          to SHIFTBUF. SHIFTER parallel shift 8 bits data to JDI from SHIFTBUF,
 *          and SHIFTER trigger source is SPI CLK. 1 MOSI data corresponds 
 *          to 8 clocks, therefore, SHIFTER output 2 words, when PIXEL_DATA_TIMER
 *          MR0 matched.
 * 
 *       2. MR0 matches once, triggering the transmission of 2 words of data.
 * 
 * @param dev is JDI display device
 * @return int 0 on success else negative errno code.
 */
int jdi_pixel_data_m0_dma_config(const struct device *dev)
{
    const struct jdi_config *config = dev->config;
    struct jdi_data *dev_data = dev->data;

    // dev_data->frame.dma_data_word_count = 1024;

    const struct mcux_counter_dma_cfg pixel_data_m0_priv_dma_cfg = {
        .mcux_dma_cfg = {
            .channel_trigger = {
                .type = kDMA_RisingEdgeTrigger,			// hw trigger, rising edge.
                .burst = kDMA_EdgeBurstTransfer2,		// burst transfer, burst size. Burst transfor 2 * dest_data_size bytes at a time. Trigger to transmit all data at once
                                                        // Assign value from end address to start address
                .wrap = kDMA_DstWrap,					// destination burst wrap.  the destination address range for each burst will be the same
            },
            .disable_int = false,
        },
    };

    const struct counter_dma_cfg pixel_data_m0_dma_config = {
        .channel_direction = MEMORY_TO_MEMORY,
        .channel_priority = 1, 					// priority = 1
        .source_data_size = sizeof(uint32_t), 	// 32-bit transfers
        .dest_data_size = sizeof(uint32_t),		// 32-bit transfers
        .source_burst_length = 0,				// burst size = 0
        .dest_burst_length = 2,					// burst size = 2
        .src_addr = (uint32_t)dev_data->frame.dma_data,
        .dest_addr = (uint32_t)&config->flexio_base->SHIFTBUF[FLEXIO_SHIFTER_PIXEL], /* Use SHIFTBUF[0] and SHIFTBUF[1] to cache RGB/ENB/VCK data */
        .length = dev_data->frame.dma_data_word_count * sizeof(uint32_t),  // bytes num. So need to multiply by source_data_size
        .source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
        .dest_addr_adj = DMA_ADDR_ADJ_INCREMENT,
        .callback = counter_dma_callback,
        .user_data = dev->data,
        .priv_config = (void *)&(pixel_data_m0_priv_dma_cfg),
    };
    printk("dma data size %d\n", dev_data->frame.dma_data_word_count);
    // printk("rgb data %08x %08x %08x %08x %08x %08x %08x %08x\n", dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count], 
    //                         dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count + 1],
    //                         dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count + 2],
    //                         dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count + 3],
    //                         dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count + 4],
    //                         dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count + 5],
    //                         dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count + 6],
    //                         dev_data->frame.dma_data[dev_data->frame.rgb_baseline_dma_data_word_count + 7]);
    return counter_set_dma_cfg(config->timer_dev[PIXEL_DATA_TIMER], pixel_data_m0_dma_match_config.chan_id, (struct counter_dma_cfg *)&pixel_data_m0_dma_config);

}

/**
 * @brief DMA configure of XRST_VST_DATA_TIMER MR0
 * 
 * When MR0 matched, update MR1
 * 
 * @param dev is JDI display device
 * @return int 0 on success else negative errno code.
 */
int jdi_xrst_vst_m0_dma_config(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    const struct mcux_counter_dma_cfg xrst_vst_m0_priv_dma_cfg = {
        .mcux_dma_cfg = {
            .channel_trigger = {
                .type = kDMA_RisingEdgeTrigger,			// hw trigger, rising edge.
                .burst = kDMA_EdgeBurstTransfer1,		// burst transfer, burst size. Burst transfor 1 * dest_data_size bytes at a time. Trigger to transmit all data at once
                                                        // Assign value from end address to start address
                .wrap = kDMA_NoWrap,					// no burst wrap.
            },
            .desc_loop = true,
            .disable_int = true,
        },
    };

    struct counter_dma_cfg xrst_vst_m0_dma_config = {
        .channel_direction = MEMORY_TO_MEMORY,
        .channel_priority = 0, 					// priority = 0
        .source_data_size = sizeof(uint32_t), 	// 32-bit transfers
        .dest_data_size = sizeof(uint32_t),		// 32-bit transfers
        .source_burst_length = 0,				//
        .dest_burst_length = 1,					//
        .src_addr = (uint32_t)&dma_gpio_match[0],
        .dest_addr = (uint32_t)&config->ctimer_base[XRST_VST_DATA_TIMER]->MR[1],
        .length = DMA_GPIO_MATCH_NUM * sizeof(uint32_t),			// bytes num
        .source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
        .dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
        .callback = NULL,
        .user_data = dev->data,
        .priv_config = (void *)&(xrst_vst_m0_priv_dma_cfg),
    };

    return counter_set_dma_cfg(config->timer_dev[XRST_VST_DATA_TIMER], xrst_vst_m0_dma_match_config.chan_id, &xrst_vst_m0_dma_config);

}

/**
 * @brief DMA configure of XRST_VST_DATA_TIMER MR1
 * 
 * When MR1 matched, generate XRST / VST signal.
 * 
 * @param dev is JDI display device
 * @return int 0 on success else negative errno code.
 */
int jdi_xrst_vst_m1_dma_config(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    uint32_t port_no = config->jdi_gpios[VST_GPIO].port_no;
    uint32_t dest_addr = (uint32_t)&(config->jdi_gpios[VST_GPIO].gpio_base->NOT[port_no]);

    const struct mcux_counter_dma_cfg xrst_vst_m1_priv_dma_cfg = {
        .mcux_dma_cfg = {
            .channel_trigger = {
                .type = kDMA_RisingEdgeTrigger,			// hw trigger, rising edge.
                .burst = kDMA_EdgeBurstTransfer1,		// burst transfer, burst size. Burst transfor 1 * dest_data_size bytes at a time. Trigger to transmit all data at once
                                                        // Assign value from end address to start address
                .wrap = kDMA_NoWrap,					// no burst wrap.
            },
            .desc_loop = true,
            .disable_int = true,
        },
    };

    struct counter_dma_cfg xrst_vst_m1_dma_config = {
        .channel_direction = MEMORY_TO_MEMORY,
        .channel_priority = 0, 					// priority = 0
        .source_data_size = sizeof(uint32_t), 	// 32-bit transfers
        .dest_data_size = sizeof(uint32_t),		// 32-bit transfers
        .source_burst_length = 0,				//
        .dest_burst_length = 1,					//
        .src_addr = (uint32_t)&dma_gpio_pin[0],
        .dest_addr = dest_addr,
        .length = DMA_GPIO_PIN_NUM * sizeof(uint32_t),			// bytes num
        .source_addr_adj = DMA_ADDR_ADJ_INCREMENT,
        .dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
        .callback = NULL,
        .user_data = dev->data,
        .priv_config = (void *)&(xrst_vst_m1_priv_dma_cfg),
    };

    return counter_set_dma_cfg(config->timer_dev[XRST_VST_DATA_TIMER], xrst_vst_m1_dma_match_config.chan_id, &xrst_vst_m1_dma_config);

}

/**
 * @brief DMA configure of ENB_DATA_TIMER MR0
 * 
 * @param dev is JDI display device
 * @return int 0 on success else negative errno code.
 */
int jdi_enb_m0_dma_config(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    // TODO
    dma_jdi_enb_iocon_flexio = IOCON_IN_NOPUPD(DISPLAY_GEN_ENB_FUNC);
    GPIO->DIRSET[DISPLAY_GEN_ENB_PORT] = 1<<DISPLAY_GEN_ENB_PIN;
    GPIO->CLR[DISPLAY_GEN_ENB_PORT] = 1<<DISPLAY_GEN_ENB_PIN;
    dma_jdi_enb_iocon_gpio_0  = IOCON_IN_NOPUPD(0);

    const struct mcux_counter_dma_cfg enb_m0_priv_dma_cfg = {
        .mcux_dma_cfg = {
            .channel_trigger = {
                .type = kDMA_RisingEdgeTrigger,			// hw trigger, rising edge.
                .burst = kDMA_EdgeBurstTransfer1,		// burst transfer, burst size. Burst transfor 1 * dest_data_size bytes at a time. Trigger to transmit all data at once
                                                        // Assign value from end address to start address
                .wrap = kDMA_NoWrap,					// no burst wrap.
            },
            .desc_loop = true,
            .disable_int = false,
        },
    };

    struct counter_dma_cfg enb_m0_dma_config = {
        .channel_direction = MEMORY_TO_MEMORY,
        .channel_priority = 0, 					// priority = 0
        .source_data_size = sizeof(uint32_t), 	// 32-bit transfers
        .dest_data_size = sizeof(uint32_t),		// 32-bit transfers
        .source_burst_length = 0,				//
        .dest_burst_length = 0,					//
        .src_addr = (uint32_t)&dma_jdi_enb_iocon_gpio_0,
        .dest_addr = (uint32_t)&IOPCTL->PIO[DISPLAY_GEN_ENB_PORT][DISPLAY_GEN_ENB_PIN],
        .length = 1024 * sizeof(uint32_t),			// bytes num
        .source_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
        .dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE,
        .callback = NULL,
        .user_data = dev->data,
        .priv_config = (void *)&(enb_m0_priv_dma_cfg),
    };

    return counter_set_dma_cfg(config->timer_dev[ENB_DATA_TIMER], enb_m0_dma_match_config.chan_id, &enb_m0_dma_config);

}

/**
 * @brief generate VCOM / FRP / XFRP to control JDI on/off
 * 
 * Generate VCOM/FRP/XFRP using flexio's low frequency output function
 * 
 * @param dev is JDI display device
 * @return int 0 on success else negative errno code.
 */
static void jdi_vcom_control(const struct device *dev, bool on_off)
{
    const struct jdi_config *config = dev->config;
    if (config->vcom_clock == NULL) {
        return;
    }

    if (on_off) {
        counter_start(config->vcom_clock);
    }
    else {
        counter_stop(config->vcom_clock);
    }
}

/**
 * @brief reset clock count, word count, enb status
 * 
 * @param frame jdi frame info
 */
static void jdi_dma_data_reset(struct frame_dma_data *frame)
{
	// reset all counters
	frame->dma_data_clock_count = 0;
	frame->dma_data_word_count = 0;
	frame->rgb_baseline_dma_data_clock_count = 0;
	frame->rgb_baseline_dma_data_word_count  = 0;

	frame->jdi_enb_disable_status = 0;

	return;    
}

/**
 * @brief Fill 2 VCK slots without RGB data but with HST, and update ENB status
 * 
 * @param frame Frame for DMA data
 * 
 * @param jdi_enb_disable_status enb status will be set
 */
void jdi_dma_data_add_jdi_last_enb(struct frame_dma_data *frame, uint32_t jdi_enb_disable_status)
{
    uint32_t i_word_loc;

    uint32_t *pnt_data_dma_lpb;
    uint32_t *pnt_data_dma_spb;
    // uint32_t *pnt_image_pixel0123;
    // uint32_t *pnt_image_pixel4567;

    uint32_t dma_word_count_loc = frame->dma_data_word_count;

    /* initialize dma data pointers */
    pnt_data_dma_lpb = (uint32_t *)&(frame->dma_data[dma_word_count_loc + PIXEL_CLK_25_HST_LEN_WORD]);
    pnt_data_dma_spb = pnt_data_dma_lpb + PIXEL_CLK_25_VCK_LEN_WORD;

    /* add VCK with zero data + VCK without horizontal control */
    /* ======================================================== */
    for (i_word_loc = 0; i_word_loc != (PIXEL_CLK_25_VCK_LEN_WORD - PIXEL_CLK_25_HST_LEN_WORD); i_word_loc++)
    {
        *pnt_data_dma_lpb = 0;
        *pnt_data_dma_spb = VCK_BYTE0123;

        pnt_data_dma_lpb++;
        pnt_data_dma_spb++;
    }

    /* JDI: prepare HST & VCK
     * LPB 0: HST
     * SPB 0: HST + VCK
     */
    frame->dma_data[dma_word_count_loc + 0                            ]  = HST_BYTE23;
    frame->dma_data[dma_word_count_loc + 0 + PIXEL_CLK_25_VCK_LEN_WORD]  = VCK_BYTE0123;

    /* TODO: */
    frame->jdi_enb_disable = (dma_word_count_loc + 0 + PIXEL_CLK_25_VCK_LEN_WORD + 2) >> 1;

    pnt_data_dma_lpb += PIXEL_CLK_25_VCK_LEN_WORD + PIXEL_CLK_25_VCK_NON_DATA_WORD;
    pnt_data_dma_spb += PIXEL_CLK_25_VCK_LEN_WORD + PIXEL_CLK_25_VCK_NON_DATA_WORD;

    dma_word_count_loc += 2 * PIXEL_CLK_25_VCK_LEN_WORD;

    /* update parameters */
    frame->dma_data_word_count = dma_word_count_loc;

    /* update JDI ENB disable status */
    frame->jdi_enb_disable_status = jdi_enb_disable_status;

    return;
}

/**
 * @brief Prepare dummy data which inserted before pixel data and will be sent to SHIFTBUF
 * 
 * Before sending the real pixel data, VST/XRST/VCK-like data needs to be generated.
 * These data and pixel data are triggered by the same input (SPI MOSI / CLK),
 * so when these data are generated, DMA data transfer is also triggered. Therefore
 * some dummy data needs to be filled before the pixel data.
 * 
 * Note: 1 MOSI data will trigger 2 words to be sent, so the number of padding bytes
 * needs to be multiplied by 2
 * 
 * Note：generate the first VCK slot without any RGB data in this function.
 * 
 * @param frame Frame for DMA data
 */
void jdi_dma_data_add_head(struct frame_dma_data *frame)
{
    uint32_t i_loc;
    uint32_t dma_word_count_loc;

    dma_word_count_loc = frame->dma_data_word_count;

    /* VST is handled by the CTIMER1 & DMA */
    //====================================

    /* + 10 us, prepare to generate XRST rising edge */
    for (i_loc = 0; i_loc != (DMA_GPIO_TOGGLE_XRST_RE - 1) * 2; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0;
    }
    dma_word_count_loc += (DMA_GPIO_TOGGLE_XRST_RE - 1) * 2;

    /* add slot for XRST rising edge */
    for (i_loc = 0; i_loc != 1 * 2; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0x00000000;
    }
    dma_word_count_loc += 1 * 2;

    /* tsXRST (XRST set-up time, min value is 12.8us) 12.8+ us before VST rising edge */
    for (i_loc = 0; i_loc != DMA_GPIO_TOGGLE_VST_RE * 2; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0x00000000;
    }
    dma_word_count_loc += DMA_GPIO_TOGGLE_VST_RE * 2;

    /* 24+ us after VST rising edge <=> 7.75 x 3.2 us windows => implement 9 (8 + 1 for VST update) */
    for (i_loc = 0; i_loc != VST_SETUP_TIME * 2; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0x00000000;
    }
    dma_word_count_loc += VST_SETUP_TIME * 2;

    /* VCK 1(twVCKH = tsVST + thVST): 24.8 + 24 us = 48.8 us <=> 15.25 x 3.2 us => implement 17 (16 + 1 for VST update) */
    for (i_loc = 0; i_loc != DMA_GPIO_TOGGLE_VST_FE * 2; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = VCK_BYTE0123;
    }
    dma_word_count_loc += DMA_GPIO_TOGGLE_VST_FE * 2;

    /* update parameters */
    frame->dma_data_word_count = dma_word_count_loc;

    /* add_head prepares foundation for RGB data, save rgb_baseline */
    frame->rgb_baseline_dma_data_word_count = dma_word_count_loc;

    return;
}

/**
 * @brief Prepare dma data for pixel data attached with VCK / HST 
 * 
 * In upodate mode, after the jdi_dma_data_add_head function is processed, prepare to generate
 * the second and subsequent VCK.
 * 
 * At the beginning of the VCK slot, there are tdHST (HST delay time, 400ns), tsHST (HST set-up
 * time, 200ns) and thHST (HST hold time, 200ns). Here, use 1 word to accomplish, the value of
 * this word is HST_BYTE23, meaning tdHST is about 800ns and tsHST + thHST is about 800ns.
 * 
 * Start sending RGB data after HST. The current screen line is 240 pixels.
 * 1 line of RGB data is transmitted in 2 times: LPB(Large Pixel Bit) and SPB(Small Pixel Bit).
 * So 2 VCK slots are required to complete a line of RGB data transmission. 1 VCK slot contains
 * 120 HCK slots with data and 2 dummy HCK slots. 
 * 
 * According to the timing requirements of JDI data transmission, adjust the RGB data format,
 * store and transmit high-order bit data and low-order bit data separately.
 * Use SHIFTBUFOES / SHIFTBUFEOS of SHIFTBUF to accomplish this function.
 * 
 * After the high-order bit and the low-order bit of the same color are isolated, one byte of
 * DMA data contains 2 bits of Reg, 2 bits of Blue and 2 bits of Green. These bit data will be
 * sent to the R1 / R2 / B1 / B2 / G1 / G2 lines of JDI.
 * 
 * 1 LPB or SPB RGB data needs 30 words of space to save, corresponding to 120 HCK slots.
 * 
 * 
 * While transmitting RGB data, it also comes with VCK and HST. the first word store HST and VCK data
 * When transmitting LPB data, VCK value is 0; When transmitting SPB data, VCK value is 1;
 * 
 * 
 * @param frame Frame for DMA data
 * 
 * @param pixel_data pixel data
 * 
 * @param number_of_lines lines number of pixel data
 */ 
void jdi_prepare_image_dma_data(struct frame_dma_data *frame, uint8_t *pixel_data, uint32_t number_of_lines)
{
    uint32_t i_line_loc, i_word_loc;

    uint32_t *pnt_data_dma_lpb;
    uint32_t *pnt_data_dma_spb;
    uint32_t *pnt_image_pixel0123;
    uint32_t *pnt_image_pixel4567;

    uint32_t dma_word_count_loc = frame->dma_data_word_count;

    /* initialize pixel offsets 0, 1, 2, 3 */
    pnt_image_pixel0123 = (uint32_t *)pixel_data;
    pnt_image_pixel4567 = pnt_image_pixel0123 + 1;

    /* initialize dma data pointers */
    /* LPB data, the first word store HST and VCK data, so need to skip 1 */
    pnt_data_dma_lpb = (uint32_t *)&(frame->dma_data[dma_word_count_loc + PIXEL_CLK_25_HST_LEN_WORD]);

    /* SPB data, skip 30 words lpb data,
     * then the next word store HST and VCK data, so need to skip 1.
     */
    pnt_data_dma_spb = pnt_data_dma_lpb + PIXEL_CLK_25_VCK_LEN_WORD;

    for (i_line_loc = 0; i_line_loc != number_of_lines; i_line_loc++)
    {
        /* add line data, Separate high-order bits and low-order bits */
        /* ============== */
        for (i_word_loc = 0; i_word_loc != (PIXEL_CLK_25_VCK_LEN_WORD - PIXEL_CLK_25_HST_LEN_WORD); i_word_loc++)
        {
            FLEXIO0->SHIFTBUF[FLEXIO_SHIFTER_AUX0] = *pnt_image_pixel0123;
            FLEXIO0->SHIFTBUF[FLEXIO_SHIFTER_AUX1] = *pnt_image_pixel4567;

            *pnt_data_dma_lpb =
                (FLEXIO0->SHIFTBUFOES[FLEXIO_SHIFTER_AUX1] & 0xFFFF0000) |
                (FLEXIO0->SHIFTBUFEOS[FLEXIO_SHIFTER_AUX0] & 0x0000FFFF);

            *pnt_data_dma_spb =
                (FLEXIO0->SHIFTBUFEOS[FLEXIO_SHIFTER_AUX1] & 0xFFFF0000) |
                (FLEXIO0->SHIFTBUFOES[FLEXIO_SHIFTER_AUX0] & 0x0000FFFF) |
                VCK_BYTE0123;

            pnt_image_pixel0123 += 2;
            pnt_image_pixel4567 += 2;
            pnt_data_dma_lpb++;
            pnt_data_dma_spb++;
        }

        /* JDI: prepare HST & VCK
         * LPB first word 0: HST
         * SPB first word 0: HST + VCK
         */
        frame->dma_data[dma_word_count_loc + 0                            ]  = HST_BYTE23;
        frame->dma_data[dma_word_count_loc + 0 + PIXEL_CLK_25_VCK_LEN_WORD]  = HST_BYTE23 | VCK_BYTE0123;

        /* Skip to next LPB / SPB position */
        pnt_data_dma_lpb += PIXEL_CLK_25_VCK_LEN_WORD + PIXEL_CLK_25_VCK_NON_DATA_WORD;
        pnt_data_dma_spb += PIXEL_CLK_25_VCK_LEN_WORD + PIXEL_CLK_25_VCK_NON_DATA_WORD;

        dma_word_count_loc += 2 * PIXEL_CLK_25_VCK_LEN_WORD;
    }

    /* update parameters */
    frame->dma_data_word_count = dma_word_count_loc;

    return;
}

/**
 * @brief After filling the RGB data, add dma data corresponding to the remaining VCK/XRST data
 * 
 * In update mode, RGB data ends on VCK 481 slot, and ENB signal ends on VCK 482 slot.
 * 
 * In partial-update mode, The end position of RGB data is determined by how many lines
 * of RGB data are transferred, and ENB signal ends on the next VCK slot following RGB
 * data.
 * 
 * Fill VCK 484 ~ 486 slots, tfXRST (XRST falling time) and VCK 487.
 * VCK 488 slot is low level, use delay for a while instead.
 * 
 * @param frame Frame for DMA data
 */
void jdi_dma_data_add_tail(struct frame_dma_data * frame)
{
    uint32_t i_loc;
    uint32_t dma_word_count_loc;

    if (frame->jdi_enb_disable_status == 0)
    {	
        /* Here in update mode, last ENB not added, do it now - add VCK 482/483 */
        jdi_dma_data_add_jdi_last_enb(frame, 2);

        dma_word_count_loc = frame->dma_data_word_count;
    }
    else
    {
        dma_word_count_loc = frame->dma_data_word_count;

        if (frame->jdi_enb_disable_status == 1)
        {
            /* last ENB added before the tail, implement empty VCK 482/483 */

            /* VCK_482 low */
            for (i_loc = 0; i_loc != PIXEL_CLK_25_VCK_LEN_WORD; i_loc++)
            {
                frame->dma_data[dma_word_count_loc + i_loc] = 0;
            }
            dma_word_count_loc += PIXEL_CLK_25_VCK_LEN_WORD;

            /* VCK_483 high */
            for (i_loc = 0; i_loc != PIXEL_CLK_25_VCK_LEN_WORD; i_loc++)
            {
                frame->dma_data[dma_word_count_loc + i_loc] = VCK_BYTE0123;
            }
            dma_word_count_loc += PIXEL_CLK_25_VCK_LEN_WORD;
        }
    }

    /* VCK_484 low */
    for (i_loc = 0; i_loc != PIXEL_CLK_25_VCK_LEN_WORD; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0;
    }
    dma_word_count_loc += PIXEL_CLK_25_VCK_LEN_WORD;

    /* VCK_485 high */
    for (i_loc = 0; i_loc != PIXEL_CLK_25_VCK_LEN_WORD; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = VCK_BYTE0123;
    }
    dma_word_count_loc += PIXEL_CLK_25_VCK_LEN_WORD;

    /* add slot for XRST falling edge */
    for (i_loc = 0; i_loc != 1 * 2; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0x00000000;
    }
    dma_word_count_loc += 1 * 2;

    /* VCK_486 low */
    for (i_loc = 0; i_loc != PIXEL_CLK_25_VCK_LEN_WORD; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0;
    }
    dma_word_count_loc += PIXEL_CLK_25_VCK_LEN_WORD;

    /* VCK_487 high */
    for (i_loc = 0; i_loc != PIXEL_CLK_25_VCK_LEN_WORD; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = VCK_BYTE0123;
    }
    dma_word_count_loc += PIXEL_CLK_25_VCK_LEN_WORD;

    /* + 10 us */
    for (i_loc = 0; i_loc != (DMA_GPIO_TOGGLE_XRST_RE - 1) * 2; i_loc++)
    {
        frame->dma_data[dma_word_count_loc + i_loc] = 0;
    }
    dma_word_count_loc += (DMA_GPIO_TOGGLE_XRST_RE - 1) * 2;

    /* update parameters */
    frame->dma_data_word_count  = dma_word_count_loc;
    /* 1 MOSI data corresponds to 8 clocks, triggering the output of 2 words at the same time */
    frame->dma_data_clock_count = 4 * dma_word_count_loc;

    return;
}

/**
 * @brief Generate SPI MOSI and CLK to trigger send JDI frame.
 * 
 * a. Reset XRST_VST_DATA_TIMER TC and restart it
 * b. Update match value (dma_gpio_match[3]) of XRST falling edge
 * c. Change ENB pin to FLEXIO function
 * d. Set MR0 and MR1 of ENB_DATA_TIMER, and restart it
 * e. Generate SPI MOSI and CLK
 * 
 * @param dev is JDI display device
 */
void jdi_send_frame(const struct device *dev)
{
    const struct jdi_config *config = dev->config;
    struct jdi_data *dev_data = dev->data;
    struct frame_dma_data *frame = &dev_data->frame;

    counter_reset(config->timer_dev[XRST_VST_DATA_TIMER]);

	/* rules for setting up MR[2] and dma_gpio_match[3], dma_gpio_match[4]
	 *
	 * dma_gpio_match[3] is set to control the XRST falling edge
	 * MR[2] > dma_gpio_match[3]
	 * dma_gpio_match[4] > MR[2]
     *
     * dma_gpio_match[3] value = The total number of DMAs to be sent - DMA header data number - DMA tail data number
     * NOTE: MR value counter up with MOSI data, and 1 MOSI data corresponds to 2 words of DMA data, So need to divide by 2
     * DMA header data number: refer to the jdi_dma_data_add_head function
     * DMA tail data number: VCK 486 ~ 488 slot
     */
	dma_gpio_match[XRST_FE_MATCH_ID] = (frame->dma_data_word_count / 2) -
                        (DMA_GPIO_TOGGLE_XRST_RE + DMA_GPIO_TOGGLE_VST_RE + DMA_GPIO_TOGGLE_VST_FE) -
                        PIXEL_CLK_25_VCK_LEN_WORD - (DMA_GPIO_TOGGLE_XRST_RE - 1);

	/* let XRST_VST_DATA_TIMER run */
	counter_start(config->timer_dev[XRST_VST_DATA_TIMER]);

	// enable FLEXIO @ ENB
	IOPCTL->PIO[DISPLAY_GEN_ENB_PORT][DISPLAY_GEN_ENB_PIN] = dma_jdi_enb_iocon_flexio;

	/* let CTIMER2 run */
    enb_m0_dma_match_config.match_config.ticks = frame->jdi_enb_disable;
    enb_reset_match_config.match_config.ticks = frame->jdi_enb_disable+1;
    jdi_enb_timer_init(dev);

	counter_start(config->timer_dev[ENB_DATA_TIMER]);

    jdi_flexio_clock_send(dev);
	return;
}

/**
 * @brief Fill line data before or after valid data in partial update mode
 * 
 * The 1st line is the 2st VCK slot or the next slot to vaild data, which
 * contain 1 HST data + 30 empty pixel data.
 * 
 * In invalid data area, the VCK slot maintain minimum 1us.
 * According to SPI CLK, 1 bit 400ns, and shifter 8 lines are transmitted in
 * parallel, that is, the VCK represented by 1 word is maintained at 1.6us,
 * 
 * @param frame Frame for DMA data
 * @param number_of_lines number of lines to be filled
 */
void jdi_dma_data_add_ffwd_line(struct frame_dma_data * frame, uint32_t number_of_lines)
{
	uint32_t i_loc;
	uint32_t dma_word_count_loc;

	dma_word_count_loc = frame->dma_data_word_count;

	if (number_of_lines != 0)
	{
		/* The 1st line is the 2st VCK slot, which contain 1 HST data + 30 pixel data */
		uint32_t *pnt_data_dma_lpb = (uint32_t *)&(frame->dma_data[dma_word_count_loc + 1]);

		for (i_loc = 0; i_loc != (PIXEL_CLK_25_VCK_LEN_WORD - 1); i_loc++)
		{
			*pnt_data_dma_lpb = 0;

			pnt_data_dma_lpb++;
		}

		/* JDI: prepare HST & VCK
		 * LPB 0: no HST
		 * SPB 0: no HST + VCK
         */
		frame->dma_data[dma_word_count_loc + 0                            ]  = 0;
        /* twVCKH (VCK High width) */
		frame->dma_data[dma_word_count_loc + 0 + PIXEL_CLK_25_VCK_LEN_WORD]  = VCK_BYTE0123;

		dma_word_count_loc += 1 * PIXEL_CLK_25_VCK_LEN_WORD + 1;

		/* the rest of ffwed lines (if any) are mins (VCK 1us) */
		if (number_of_lines > 1)
		{
            /* Use 1 word (1.6us) to represent the VCK corresponding to invalid pixel data */
			for (i_loc = 1; i_loc != number_of_lines; i_loc++)
			{
                /* twVCKL (VCK Low width) + no pixel */
				frame->dma_data[dma_word_count_loc + 0] = 0;

                /* twVCKH (VCK High width) + no pixel */
				frame->dma_data[dma_word_count_loc + 1] = VCK_BYTE0123;

				dma_word_count_loc += 1 * 2;
			}
		}
	}

	/* update parameters */
	frame->dma_data_word_count = dma_word_count_loc;

	return;
}


static int jdi_write(const struct device *dev, const uint16_t x,
                 const uint16_t y,
                 const struct display_buffer_descriptor *desc,
                 const void *buf)
{
    const struct jdi_config *config = dev->config;
    struct jdi_data *dev_data = dev->data;
    uint16_t panel_width = config->display_info.panel_width;
    uint16_t panel_height = config->display_info.panel_height;

    /* Only supports transferring the entire line of the screen at a time */
    if (panel_width != desc->width || panel_width != desc->pitch) {
        return -EINVAL;
    }

    k_sem_take(&dev_data->sem, K_FOREVER);

    jdi_dma_data_reset(&dev_data->frame);

    /* Generate timing diagrams corresponding to all DMA data */
    jdi_dma_data_add_head(&dev_data->frame);

    if (desc->height == panel_height) {
        /* Update mode */
        jdi_prepare_image_dma_data(&dev_data->frame, (uint8_t *)buf, desc->height);
    } else {
        /* Partial update mode */
        /* Fill line data before pixel data */
        jdi_dma_data_add_ffwd_line(&dev_data->frame, y);

        /* Convert pixel data to DMA data */
        jdi_prepare_image_dma_data(&dev_data->frame, (uint8_t *)buf, desc->height);

        /* Fill 1 line data and disable ENB output in partial update mode */
        jdi_dma_data_add_jdi_last_enb(&dev_data->frame, 1);

        if (panel_height > y + desc->height + 1) {
            /* Fill line data after pixel data */
            jdi_dma_data_add_ffwd_line(&dev_data->frame, panel_height - (y + desc->height + 1));
        }

    }
    jdi_dma_data_add_tail(&dev_data->frame);

    /* prepare frame's dma descriptors */
    jdi_pixel_data_m0_dma_config(dev);

    jdi_send_frame(dev);

    k_sem_give(&dev_data->sem);
    uint8_t channel = 34;
    LOG_DBG("DMA channel %d CFG %08x\n", channel, DMA0->CHANNEL[channel].CFG);
    LOG_DBG("DMA channel %d XFERCFG %08x\n", channel, DMA0->CHANNEL[channel].XFERCFG);
    
    // printk("\n\n\n\n\n");
    return 0;
}

static int jdi_read(const struct device *dev, const uint16_t x,
                const uint16_t y,
                const struct display_buffer_descriptor *desc,
                void *buf)
{
    LOG_ERR("Read not implemented");
    return -ENOTSUP;
}

static void *jdi_get_framebuffer(const struct device *dev)
{
    LOG_ERR("Direct framebuffer access not implemented");
    return NULL;
}

static int jdi_display_blanking_off(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    jdi_vcom_control(dev, false);

    if (config->backlight_gpio.port != NULL) {
        return gpio_pin_set_dt(&config->backlight_gpio, 1);
    }
    return -ENOTSUP;
}

static int jdi_display_blanking_on(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    jdi_vcom_control(dev, true);

    if (config->backlight_gpio.port != NULL) {
        return gpio_pin_set_dt(&config->backlight_gpio, 0);
    }
    return -ENOTSUP;
}

static int jdi_set_brightness(const struct device *dev,
                      const uint8_t brightness)
{
    LOG_WRN("Set brightness not implemented");
    return -ENOTSUP;
}

static int jdi_set_contrast(const struct device *dev,
                    const uint8_t contrast)
{
    LOG_ERR("Set contrast not implemented");
    return -ENOTSUP;
}

static int jdi_set_pixel_format(const struct device *dev,
                    const enum display_pixel_format
                    pixel_format)
{
    struct jdi_data *dev_data = dev->data;
    ARG_UNUSED(dev_data);

    LOG_ERR("Pixel format change not implemented");
    return -ENOTSUP;
}

static int jdi_set_orientation(const struct device *dev,
        const enum display_orientation orientation)
{
    if (orientation == DISPLAY_ORIENTATION_NORMAL) {
        return 0;
    }
    LOG_ERR("Changing display orientation not implemented");
    return -ENOTSUP;
}

static void jdi_get_capabilities(const struct device *dev,
        struct display_capabilities *capabilities)
{
    const struct jdi_config *config = dev->config;
    struct jdi_data *dev_data = dev->data;
    ARG_UNUSED(dev_data);
    
    if (capabilities == NULL) {
        return;
    }

    capabilities->x_resolution = config->display_info.panel_width;
    capabilities->y_resolution = config->display_info.panel_height;
    capabilities->supported_pixel_formats = PIXEL_FORMAT_RGB_222;
    return ;
}

/**
 * @brief init a ctimer used for send DMA data to SHIFTBUF
 * 
 * @param dev JDI display device
 * @return int 0 on success else negative errno code.
 */
static int jdi_pixel_data_timer_init(const struct device *dev)
{
    const struct jdi_config *config = dev->config;
    int ret;

    if (config->timer_dev[PIXEL_DATA_TIMER] == NULL) {
        return -EINVAL;
    }

    /* Use a timer to trigger DMA transfer of pixel data.
     * This timer needs to be configured to trigger counting
     * on rising and falling edges.
     * Trigger DMA when counter is 1, reset counter when counter is 2.
     * Trigger source come from capture pin, that is @ref input_clock output
     */

    ret = counter_set_channel_alarm(config->timer_dev[PIXEL_DATA_TIMER], 
                pixel_data_m0_dma_match_config.chan_id, &pixel_data_m0_dma_match_config.match_config);
    if (ret) {
        LOG_ERR("set timer %d chanenl %d fail %d", PIXEL_DATA_TIMER, pixel_data_m0_dma_match_config.chan_id, ret);
        return ret;
    }

    ret = counter_set_channel_alarm(config->timer_dev[PIXEL_DATA_TIMER],
    			pixel_clock_m1_dma_match_config.chan_id, &pixel_clock_m1_dma_match_config.match_config);
    if (ret) {
    	LOG_ERR("set timer %d chanenl %d fail %d", PIXEL_DATA_TIMER, pixel_clock_m1_dma_match_config.chan_id, ret);
    	return ret;
    }


    ret = counter_set_channel_alarm(config->timer_dev[PIXEL_DATA_TIMER], 
                pixel_data_reset_match_config.chan_id, &pixel_data_reset_match_config.match_config);
    if (ret) {
        LOG_ERR("set timer %d chanenl %d fail %d", PIXEL_DATA_TIMER, pixel_data_reset_match_config.chan_id, ret);
        return ret;
    }

    return 0;
}

/**
 * @brief use CTIMER to generate XRST/VST line
 * 
 * T1_DMAREQ_M0 drives MATCH update, T1_DMAREQ_M1 drives PIN update.
 * 
 * MR[0] is set to 1 so that when CTIMER1 starts counting and reaches
 * count of 1 this triggers a DMA request that updates MR[1] (MR[1] > MR[0])
 * 
 * MR[1] is set to match the point in time when a display line (XRST or VST)
 * needs update. when CTIMER1 reaches count of MR[1] a DMA trigger is generated
 * and a pattern is written into the port pin NOT register changing the pins
 * of interest; at the same time when CTIMER1 count reaches MR[1] this resets
 * CTIMER1 count, too, letting the CTIMER1 go back to 0 and counting up again,
 * if CTIMER1 reaches MR[2] the timer stops and resets.
 * 
 * in reality when the XRST falling edge is generated (using match/pin arrays'
 * index 3 entries) CTIMER1 resets and goes back to counting from 1 and the
 * MR[1] will get updated with match[4]; soon after this the frame will end,
 * the FLEXCOMM5 isr will execute and CTIMER1 will be stopped and reset in sw;
 * if for whatever reason FLEXCOMM5 isr does not run soon after the frame ends,
 * CTIMER1 will reach MR[2] and its hw will do the same thing on its own
 * 
 * the last MR[1] update must be made so that MR[1]>MR[2] guaranteeing that
 * CTIMER1 driven DMA based pin updates will not make any port changes after
 * the XRST falling edge is generated
 * 
 * @param dev JDI display device
 * @return int 0 on success else negative errno code.
 */
int jdi_xrst_vst_timer_init(const struct device *dev)
{
    const struct jdi_config *config = dev->config;
    int ret;

    if (config->timer_dev[XRST_VST_DATA_TIMER] == NULL) {
        return -EINVAL;
    }

    ret = counter_set_channel_alarm(config->timer_dev[XRST_VST_DATA_TIMER], 
                xrst_vst_m0_dma_match_config.chan_id, &xrst_vst_m0_dma_match_config.match_config);
    if (ret) {
        LOG_ERR("set timer %d chanenl %d fail %d", XRST_VST_DATA_TIMER, xrst_vst_m0_dma_match_config.chan_id, ret);
        return ret;
    }

    counter_set_channel_alarm(config->timer_dev[XRST_VST_DATA_TIMER], 
                xrst_vst_m1_dma_match_config.chan_id, &xrst_vst_m1_dma_match_config.match_config);
    if (ret) {
        LOG_ERR("set timer %d chanenl %d fail %d", XRST_VST_DATA_TIMER, xrst_vst_m1_dma_match_config.chan_id, ret);
        return ret;
    }

    counter_set_channel_alarm(config->timer_dev[XRST_VST_DATA_TIMER], 
                xrst_vst_reset_match_config.chan_id, &xrst_vst_reset_match_config.match_config);
    if (ret) {
        LOG_ERR("set timer %d chanenl %d fail %d", XRST_VST_DATA_TIMER, xrst_vst_reset_match_config.chan_id, ret);
        return ret;
    }

    return 0;
}

/**
 * @brief use CTIMER to generate ENB disable signal
 * 
 * use CTIMER2 to disable the ENB output following the last HST generated
 * The FLEXIO is set to generate HST's matching ENB in the next half-line
 * 
 * @param dev JDI display device
 * @return int 0 on success else negative errno code.
 */
static int jdi_enb_timer_init(const struct device *dev)
{
    const struct jdi_config *config = dev->config;
    int ret;

    if (config->timer_dev[ENB_DATA_TIMER] == NULL) {
        return -EINVAL;
    }

    ret = counter_set_channel_alarm(config->timer_dev[ENB_DATA_TIMER], 
                enb_m0_dma_match_config.chan_id, &enb_m0_dma_match_config.match_config);
    if (ret) {
        LOG_ERR("set timer %d chanenl %d fail %d", ENB_DATA_TIMER, enb_m0_dma_match_config.chan_id, ret);
        return ret;
    }

    ret = counter_set_channel_alarm(config->timer_dev[ENB_DATA_TIMER], 
                enb_reset_match_config.chan_id, &enb_reset_match_config.match_config);
    if (ret) {
        LOG_ERR("set timer %d chanenl %d fail %d", ENB_DATA_TIMER, enb_reset_match_config.chan_id, ret);
        return ret;
    }

    return 0;
}

/**
 * @brief FLEXIO SHIFTER configure
 * 
 * Use SHIFTER0 to load RGB/VCK/HST data from SHIFTBUF0, and send to [FLEXIOD4, FLEXIOD11].
 * The FLEXIO timer which SHIFTER used is external trigger source from SPI CLK
 * SPI CLK also is shifter clock, and the cycle time of the clock is exactly equal to the
 * time of RGB 1bit. Therefore in 1 MOSI data (8 SPI CLK), 8 bytes of pixel data are sent. 
 * That is 1 MOSI data trigger the DMA of CTIMER0 MR0 to send 2 words of pixel data to SHIFTBUF.
 * So FLEXIO need use 2 SHIFTBUF to receive data. It need to initialize 2 shifters
 * 
 * @param dev is JDI display device
 */
static void jdi_pixel_data_flexio_shifter_config(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    /* SHIFTER0 support parallel transmit, so we use SHIFTER0 to send pixel data */
    const flexio_shifter_config_t pixel_data_shifter0_cfg = {
        .timerSelect = FLEXIO_TIMER_PIXEL,		/* FLEXIO_TIMER_PIXEL is used for controlling the logic/shift register 
                                                   and generating the Shift clock */
        .timerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive, /*  Shift on posedge of Shift clock */
        .pinConfig = kFLEXIO_PinConfigOutput,	/* Shifter pin output */
        .pinSelect = 4,							/* FXIO_D4:FXIO_D[4 + PWIDTH] pin is used for SHIFTER0 output */
        .parallelWidth = 7,						/* Parallel transmission 8 bit, there are RGB/VCK/HST data */
        .pinPolarity = kFLEXIO_PinActiveHigh,	/* Pin is active high */
        .shifterMode = kFLEXIO_ShifterModeTransmit, /* Transmit mode */
        .inputSource = kFLEXIO_ShifterInputFromNextShifterOutput, /* Input Source: Shifter N+1 Output */
        .shifterStop = kFLEXIO_ShifterStopBitDisable, 	/* Disable shifter stop bit */
        .shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable,	/* Disable shifter start bit */
    };

    FLEXIO_SetShifterConfig(config->flexio_base, FLEXIO_SHIFTER_PIXEL, &pixel_data_shifter0_cfg);
    LOG_DBG("\nshifter0 SHIFTCFG %08x, SHIFTCTL %08x\n", config->flexio_base->SHIFTCFG[FLEXIO_SHIFTER_PIXEL], 
                            config->flexio_base->SHIFTCTL[FLEXIO_SHIFTER_PIXEL]);

    const flexio_shifter_config_t pixel_data_shifter1_cfg = {
        .timerSelect = FLEXIO_TIMER_PIXEL,		/* FLEXIO_TIMER_PIXEL is used for controlling the logic/shift register 
                                                   and generating the Shift clock */
        .timerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive, /*  Shift on posedge of Shift clock */
        .pinConfig = kFLEXIO_PinConfigOutputDisabled,	/* Shifter pin output disabled */
        .pinSelect = 0,							/* pin selected: NA */
        .parallelWidth = 7,						/* Parallel transmission 8 bit, there are RGB/VCK/HST data */
        .pinPolarity = kFLEXIO_PinActiveHigh,	/* Pin is active high */
        .shifterMode = kFLEXIO_ShifterModeTransmit, /* Transmit mode */
        .inputSource = kFLEXIO_ShifterInputFromNextShifterOutput, /* Input Source: Shifter N+1 Output */
        .shifterStop = kFLEXIO_ShifterStopBitDisable, 	/* Disable shifter stop bit */
        .shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable,	/* Disable shifter start bit */
    };

    FLEXIO_SetShifterConfig(config->flexio_base, FLEXIO_SHIFTER_PIXEL_ADD, &pixel_data_shifter1_cfg);
    LOG_DBG("\nshifter1 SHIFTCFG %08x, SHIFTCTL %08x\n", config->flexio_base->SHIFTCFG[FLEXIO_SHIFTER_PIXEL_ADD], 
                            config->flexio_base->SHIFTCTL[FLEXIO_SHIFTER_PIXEL_ADD]);

    /* make sure auxiliary shifters are not configured. Use this to separate odd and even bits in preparation for LPB and SPB */
    FLEXIO0->SHIFTCTL[FLEXIO_SHIFTER_AUX0] = 0;
    FLEXIO0->SHIFTCTL[FLEXIO_SHIFTER_AUX1] = 0;
}

/**
 * @brief Shifter timer configure
 * 
 * The timer used by SHIFTER use external trigger source from SPI CLK.
 * Therefore make sure that the SPI CLK is connected to FLEXIO_D13 on the hardware wiring.
 * According to JDI timing requirements, the period of SPI CLK must be above 400ns.
 * 
 * @param dev is JDI display device
 */
static void jdi_pixel_data_flexio_timer_config(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    /* flexio timer init, which is clock as pixel data shifter */
    const flexio_timer_config_t pixel_data_timer_cfg = {
        /* Trigger. */
        .triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(FLEXIO_TIMER_TRIG_PIN), /* FLEXIO_D13 used as FLEXIO_TIMER_PIXEL trigger */
        .triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh,				   /* Trigger active high */
        .triggerSource = kFLEXIO_TimerTriggerSourceInternal,					   /* Internal trigger selected */
        /* Pin. */
        .pinConfig = kFLEXIO_PinConfigOutputDisabled,							   /* Timer pin output disabled */
        .pinSelect = 0,															   /* Timer Pin Select - NA */
        .pinPolarity = kFLEXIO_PinActiveHigh,									   /* Pin is active - NA */
        /* Timer. */
        .timerMode = kFLEXIO_TimerModeSingle16Bit,								   /* Single 16-bit counter mode */
        .timerOutput = kFLEXIO_TimerOutputZeroNotAffectedByReset,				   /* Timer output is logic zero when enabled and is not affected by timer reset */
        .timerDecrement = 7,													   /* Decrement counter on Trigger input (rising edge), Shift clock equals Trigger input */
        .timerReset = kFLEXIO_TimerResetNever,									   /* Timer never reset */
        .timerDisable = kFLEXIO_TimerDisableNever,						           /* Timer never disabled */
        .timerEnable = kFLEXIO_TimerEnabledAlways,								   /* Timer always enabled */
        .timerStop = kFLEXIO_TimerStopBitDisabled,								   /* Stop bit disabled */
        .timerStart = kFLEXIO_TimerStartBitDisabled,							   /* Start bit disabled */
        .timerCompare = (8 - 1),												   /* reload shifter control: 4-line parallel interface <=> 32/4 = 8. 
                                                                                    * When the shift clock source is a pin or trigger input, 
                                                                                    * the compare register is used to set the number of bits in each
                                                                                    * word equal to (CMP[15:0] + 1) / 2.
                                                                                    */
    };

    FLEXIO_SetTimerConfig(config->flexio_base, FLEXIO_TIMER_PIXEL, &pixel_data_timer_cfg);
    LOG_DBG("\nflexio pixel data timer TIMCFG %08x, TIMCTL %08x, TIMCMP %08x\n", config->flexio_base->TIMCFG[FLEXIO_TIMER_PIXEL], 
                            config->flexio_base->TIMCTL[FLEXIO_TIMER_PIXEL], config->flexio_base->TIMCMP[FLEXIO_TIMER_PIXEL]);

}

/**
 * @brief FELXIO initialization related to pixel data
 * 
 * @param dev is JDI display device
 */
static void jdi_pixel_data_flexio_config(const struct device *dev)
{
    jdi_pixel_data_flexio_shifter_config(dev);
    jdi_pixel_data_flexio_timer_config(dev);
}

/**
 * @brief HCK FLEXIO timer configure
 * 
 * When the HST signal occurs and continues tsHST (HST set-up time), the HCK signal needs
 * to be generated.
 * 
 * HCK trigger timer is triggered by HST rising edge. According to the current code, the
 * HST signal remains high for 800ns+, so config lower 8-bits of TIMCMP as 56 (based on FLEXIO
 * CLOCK about 600ns). When the lower 8-bits equal zero, toggle FLEXIO_D15 output and disable
 * decrement until next HST rising edge trigger.
 * 
 * HCK timer is triggered by FLEXIO_D15, the output of HCK trigger timer and output HCK signal,
 * There are 122 HCK slots, so upper 8-bits of TIMCMP is 122.
 *
 * 
 * @param dev is JDI display device
 */
static void jdi_hck_flexio_timer_config(const struct device *dev)
{
    uint32_t main_clk_freq = CLOCK_GetMainClkFreq();
    uint32_t flexio_clk_freq = CLOCK_GetFlexioClkFreq();
    LOG_DBG("\nMain Clock Freq %d, FLEXIO Clock Freq %d\n", main_clk_freq, flexio_clk_freq);

    /* TIMCMP value of HCK trigger timer is based on 96M FLEXIO clock frequency */
    assert(flexio_clk_freq == 96000000);

    const struct jdi_config *config = dev->config;

    /* HCK trigger timer init */
    const flexio_timer_config_t hck_trigger_timer_cfg = {
        /* Trigger. */
        .triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(FLEXIO_SHIFTER_PIN_HST), /* FLEXIO_D11(HST) used as FLEXIO_TIMER_HCK_TRIGGER trigger */
        .triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh,				   /* Trigger active high */
        .triggerSource = kFLEXIO_TimerTriggerSourceInternal,					   /* Internal trigger selected */
        /* Pin. */
        .pinConfig = kFLEXIO_PinConfigOutput,							   		   /* Timer pin output */
        .pinSelect = FLEXIO_HCK_TRIG_PIN,										   /* Timer Pin Select - FLEXIO_HCK_TRIG_PIN (FLEXIO_D15) */
        .pinPolarity = kFLEXIO_PinActiveHigh,									   /* Pin is active high */
        /* Timer. */
        .timerMode = kFLEXIO_TimerModeDual8BitBaudBit,							   /* 8-bit baud counter mode */
        .timerOutput = kFLEXIO_TimerOutputZeroAffectedByReset,					   /* Timer output is logic zero when enabled and on timer reset */
        .timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput,		   /* Decrement counter on FlexIO clock, Shift clock equals Timer output */
        .timerReset = kFLEXIO_TimerResetNever,									   /* Timer never reset */
        .timerDisable = kFLEXIO_TimerDisableOnTimerCompare,						   /* Timer disabled on Timer compare (upper 8-bits match and decrement) */
        .timerEnable = kFLEXIO_TimerEnableOnTriggerRisingEdge,					   /* Timer enabled on Trigger rising edge */
        .timerStop = kFLEXIO_TimerStopBitDisabled,								   /* Stop bit disabled */
        .timerStart = kFLEXIO_TimerStartBitDisabled,							   /* Start bit disabled */
        .timerCompare = ((2-1)<<8 | (56-1)<<0),									   /* 8-bit baud counter mode <=> number of bits + clock divider/delay.
                                                                                    * When the lower 8-bits decrement to zero, the timer output is toggled
                                                                                    * and the lower 8-bits reload from the compare register. The upper 8-bits
                                                                                    * decrement when the lower 8-bits equal zero and decrement.
                                                                                    */
    };

    FLEXIO_SetTimerConfig(config->flexio_base, FLEXIO_TIMER_HCK_TRIGGER, &hck_trigger_timer_cfg);
    LOG_DBG("\nflexio HCK trigger timer TIMCFG %08x, TIMCTL %08x, TIMCMP %08x\n", config->flexio_base->TIMCFG[FLEXIO_TIMER_HCK_TRIGGER], 
                            config->flexio_base->TIMCTL[FLEXIO_TIMER_HCK_TRIGGER], config->flexio_base->TIMCMP[FLEXIO_TIMER_HCK_TRIGGER]);

    /* HCK timer init */
    const flexio_timer_config_t hck_timer_cfg = {
        /* Trigger. */
        .triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(FLEXIO_HCK_TRIG_PIN), /* FLEXIO_D15(HCK trigger timer output) used as FLEXIO_TIMER_HCK trigger */
        .triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh,				   /* Trigger active high */
        .triggerSource = kFLEXIO_TimerTriggerSourceInternal,					   /* Internal trigger selected */
        /* Pin. */
        .pinConfig = kFLEXIO_PinConfigOutput,							   		   /* Timer pin output */
        .pinSelect = FLEXIO_TIMER_HCK_PIN,										   /* Timer Pin Select - FLEXIO_TIMER_HCK_PIN (FLEXIO_D12) */
        .pinPolarity = kFLEXIO_PinActiveHigh,									   /* Pin is active high */
        /* Timer. */
        .timerMode = kFLEXIO_TimerModeDual8BitBaudBit,							   /* 8-bit baud counter mode */
        .timerOutput = kFLEXIO_TimerOutputOneAffectedByReset,					   /* Timer output is logic one when enabled and on timer reset */
        .timerDecrement = kFLEXIO_TimerDecSrcOnPinInputShiftPinInput,		 	   /* Decrement counter on Pin input (both edges), Shift clock equals Pin input */
        .timerReset = kFLEXIO_TimerResetNever,									   /* Timer never reset */
        .timerDisable = kFLEXIO_TimerDisableOnTimerCompare,						   /* Timer disabled on Timer compare (upper 8-bits match and decrement) */
        .timerEnable = kFLEXIO_TimerEnableOnTriggerRisingEdge,					   /* Timer enabled on Trigger rising edge */
        .timerStop = kFLEXIO_TimerStopBitDisabled,								   /* Stop bit disabled */
        .timerStart = kFLEXIO_TimerStartBitDisabled,							   /* Start bit disabled */
        .timerCompare = ((122-1)<<8 | (2-1)<<0),								   /* 8-bit baud counter mode <=> number of bits + clock divider */
    };

    FLEXIO_SetTimerConfig(config->flexio_base, FLEXIO_TIMER_HCK, &hck_timer_cfg);
    /* the timer input pin is a different pin from the timer output pin. PINSEL must select an even
     * numbered pin when this bit is set, so the output pin is even numbered and input pin is odd numbered.
     * Timer pin input is selected by PINSEL+1 (FLEXIO_D13, also is SPI5 CLK)
     */
    FLEXIO0->TIMCTL[FLEXIO_TIMER_HCK] |= FLEXIO_TIMCTL_PININS(1);
    LOG_DBG("\nflexio HCK timer TIMCFG %08x, TIMCTL %08x, TIMCMP %08x\n", config->flexio_base->TIMCFG[FLEXIO_TIMER_HCK], 
                            config->flexio_base->TIMCTL[FLEXIO_TIMER_HCK], config->flexio_base->TIMCMP[FLEXIO_TIMER_HCK]);

}

/**
 * @brief FELXIO initialization related to HCK
 * 
 * @param dev is JDI display device
 */
static void jdi_hck_flexio_config(const struct device *dev)
{
    jdi_hck_flexio_timer_config(dev);
}

static void jdi_enb_flexio_timer_config(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    /* ENB timer init */
    /* FLEXIO_TIMER_ENB_0: generate a pulse (@HCK 115, 116,117) */
    const flexio_timer_config_t enb0_timer_cfg = {
        /* Trigger. */
        .triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(FLEXIO_TIMER_TRIG_PIN), /* FLEXIO_D13(SPI CLK) used as FLEXIO_TIMER_ENB_0 trigger */
        .triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh,				   /* Trigger active high */
        .triggerSource = kFLEXIO_TimerTriggerSourceInternal,					   /* Internal trigger selected */
        /* Pin. */
        .pinConfig = kFLEXIO_PinConfigOutput,							   		   /* Timer pin output */
        .pinSelect = FLEXIO_TIMER_ENB_0_OUT_PIN,								   /* Timer Pin Select - FLEXIO_TIMER_ENB_0_OUT_PIN (FLEXIO_D14) */
        .pinPolarity = kFLEXIO_PinActiveHigh,									   /* Pin is active high */
        /* Timer. */
        .timerMode = 6,							   								   /* Dual 8-bit counters PWM low mode */
        .timerOutput = kFLEXIO_TimerOutputZeroNotAffectedByReset,				   /* Timer output is logic zero when enabled and is not affected by timer reset */
        .timerDecrement = kFLEXIO_TimerDecSrcOnTriggerInputShiftTriggerInput,	   /* Decrement counter on Trigger input (both edges), Shift clock equals Trigger input */
        .timerReset = kFLEXIO_TimerResetNever,									   /* Timer never reset */
        .timerDisable = kFLEXIO_TimerDisableOnTimerCompare,						   /* Timer disabled on Timer compare (upper 8-bits match and decrement) */
        .timerEnable = kFLEXIO_TimerEnableOnPrevTimerEnable,					   /* Timer enabled on Timer N-1 enable */
        .timerStop = kFLEXIO_TimerStopBitDisabled,								   /* Stop bit disabled */
        .timerStart = kFLEXIO_TimerStartBitDisabled,							   /* Start bit disabled */
        .timerCompare = ((6-1)<<8 | (228-1)<<0),								   /* Dual 8-bit counters PWM high mode: HIGH + LOW */
    };

    FLEXIO_SetTimerConfig(config->flexio_base, FLEXIO_TIMER_ENB_0, &enb0_timer_cfg);
    LOG_DBG("\nflexio ENB 0 timer TIMCFG %08x, TIMCTL %08x, TIMCMP %08x\n", config->flexio_base->TIMCFG[FLEXIO_TIMER_ENB_0], 
                            config->flexio_base->TIMCTL[FLEXIO_TIMER_ENB_0], config->flexio_base->TIMCMP[FLEXIO_TIMER_ENB_0]);

    /* FLEXIO_TIMER_ENB_1: enabled by pin FLEXIO_TIMER_ENB_0_OUT_PIN */
    const flexio_timer_config_t enb1_timer_cfg = {
        /* Trigger. */
        .triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(FLEXIO_TIMER_TRIG_PIN), /* FLEXIO_D13(SPI CLK) used as FLEXIO_TIMER_ENB_0 trigger */
        .triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh,				   /* Trigger active high */
        .triggerSource = kFLEXIO_TimerTriggerSourceInternal,					   /* Internal trigger selected */
        /* Pin. */
        .pinConfig = kFLEXIO_PinConfigOutputDisabled,					 		   /* Timer pin output disabled */
        .pinSelect = FLEXIO_TIMER_ENB_0_OUT_PIN,								   /* Timer Pin Select - FLEXIO_TIMER_ENB_0_OUT_PIN (FLEXIO_D14) */
        .pinPolarity = kFLEXIO_PinActiveHigh,									   /* Pin is active high */
        /* Timer. */
        .timerMode = 6,							   								   /* Dual 8-bit counters PWM low mode */
        .timerOutput = kFLEXIO_TimerOutputZeroNotAffectedByReset,				   /* Timer output is logic zero when enabled and is not affected by timer reset */
        .timerDecrement = kFLEXIO_TimerDecSrcOnTriggerInputShiftTriggerInput,	   /* Decrement counter on Trigger input (both edges), Shift clock equals Trigger input */
        .timerReset = kFLEXIO_TimerResetNever,									   /* Timer never reset */
        .timerDisable = kFLEXIO_TimerDisableOnTimerCompare,						   /* Timer disabled on Timer compare (upper 8-bits match and decrement) */
        .timerEnable = kFLEXIO_TimerEnableOnPinRisingEdge,						   /* Timer enabled on Pin rising edge */
        .timerStop = kFLEXIO_TimerStopBitDisabled,								   /* Stop bit disabled */
        .timerStart = kFLEXIO_TimerStartBitDisabled,							   /* Start bit disabled */
        .timerCompare = ((2-1)<<8 | (2-1)<<0),								   	   /* Dual 8-bit counters PWM high mode: HIGH + LOW */
    };

    FLEXIO_SetTimerConfig(config->flexio_base, FLEXIO_TIMER_ENB_1, &enb1_timer_cfg);
    LOG_DBG("\nflexio ENB 1 timer TIMCFG %08x, TIMCTL %08x, TIMCMP %08x\n", config->flexio_base->TIMCFG[FLEXIO_TIMER_ENB_1], 
                            config->flexio_base->TIMCTL[FLEXIO_TIMER_ENB_1], config->flexio_base->TIMCMP[FLEXIO_TIMER_ENB_1]);

    /* FLEXIO_TIMER_ENB_2: generate a pulse half-a-line later... */
    const flexio_timer_config_t enb2_timer_cfg = {
        /* Trigger. */
        .triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(FLEXIO_TIMER_TRIG_PIN), /* FLEXIO_D13(SPI CLK) used as FLEXIO_TIMER_ENB_0 trigger */
        .triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh,				   /* Trigger active high */
        .triggerSource = kFLEXIO_TimerTriggerSourceInternal,					   /* Internal trigger selected */
        /* Pin. */
        .pinConfig = kFLEXIO_PinConfigOutput,					 		   		   /* Timer pin output */
        .pinSelect = FLEXIO_TIMER_ENB_PIN,								   	   	   /* Timer Pin Select - FLEXIO_TIMER_ENB_PIN (FLEXIO_D3) */
        .pinPolarity = kFLEXIO_PinActiveHigh,									   /* Pin is active high */
        /* Timer. */
        .timerMode = 6,							   								   /* Dual 8-bit counters PWM low mode */
        .timerOutput = kFLEXIO_TimerOutputZeroNotAffectedByReset,				   /* Timer output is logic zero when enabled and is not affected by timer reset */
        .timerDecrement = kFLEXIO_TimerDecSrcOnTriggerInputShiftTriggerInput,	   /* Decrement counter on Trigger input (both edges), Shift clock equals Trigger input */
        .timerReset = kFLEXIO_TimerResetNever,									   /* Timer never reset */
        .timerDisable = kFLEXIO_TimerDisableOnTimerCompare,						   /* Timer disabled on Timer compare (upper 8-bits match and decrement) */
        .timerEnable = kFLEXIO_TimerEnableOnPrevTimerEnable,					   /* Timer enabled on Timer N-1 enable */
        .timerStop = kFLEXIO_TimerStopBitDisabled,								   /* Stop bit disabled */
        .timerStart = kFLEXIO_TimerStartBitDisabled,							   /* Start bit disabled */
        .timerCompare = ((124-1)<<8 | (83-1)<<0),							   	   /* Dual 8-bit counters PWM high mode: HIGH + LOW */
    };

    FLEXIO_SetTimerConfig(config->flexio_base, FLEXIO_TIMER_ENB_2, &enb2_timer_cfg);
    LOG_DBG("\nflexio ENB 2 timer TIMCFG %08x, TIMCTL %08x, TIMCMP %08x\n", config->flexio_base->TIMCFG[FLEXIO_TIMER_ENB_2], 
                            config->flexio_base->TIMCTL[FLEXIO_TIMER_ENB_2], config->flexio_base->TIMCMP[FLEXIO_TIMER_ENB_2]);
}

/**
 * @brief FELXIO initialization related to ENB
 * 
 * @param dev is JDI display device
 */
static void jdi_enb_flexio_config(const struct device *dev)
{
    jdi_enb_flexio_timer_config(dev);
}

static int jdi_flexio_setup(const struct device *dev)
{
    const struct jdi_config *config = dev->config;

    uint32_t main_clk_freq = CLOCK_GetMainClkFreq();
    uint32_t flexio_clk_freq = CLOCK_GetFlexioClkFreq();
    LOG_DBG("\nMain Clock Freq %d, FLEXIO Clock Freq %d\n", main_clk_freq, flexio_clk_freq);

    /* init flexio */
    flexio_config_t flexio_config;
    FLEXIO_GetDefaultConfig(&flexio_config);

    FLEXIO_Init(config->flexio_base, &flexio_config);

    jdi_pixel_data_flexio_config(dev);

    jdi_hck_flexio_config(dev);
    
    jdi_enb_flexio_config(dev);

    return 0;
}

static int jdi_init(const struct device *dev)
{
    const struct jdi_config *config = dev->config;
    struct jdi_data *dev_data = dev->data;
    int err;

    err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
    if (err) {
        return err;
    }

    /* init VST / XRST GPIO pin */
    for (uint8_t i = 0; i < JDI_MAX_GPIO_NUM; i++) {
        err = gpio_pin_configure_dt(&config->jdi_gpios[i].gpio, GPIO_OUTPUT_INACTIVE);
        if (err) {
            return err;
        }
    }

    if (config->backlight_gpio.port != NULL) {
        err = gpio_pin_configure_dt(&config->backlight_gpio, GPIO_OUTPUT_ACTIVE);
        if (err) {
            return err;
        }
    }

    jdi_flexio_setup(dev);

    err = jdi_pixel_data_timer_init(dev);
    if (err) {
        LOG_ERR("pixel data timer init fail %d", err);
        return err;
    }

    err = jdi_xrst_vst_timer_init(dev);
    if (err) {
        LOG_ERR("xrst/vst timer init fail %d", err);
        return err;
    }

    err = jdi_enb_timer_init(dev);
    if (err) {
        LOG_ERR("enb timer init fail %d", err);
        return err;
    }

    /* Set fixed DMA configuration */
    err = jdi_xrst_vst_m0_dma_config(dev);
    if (err) {
        LOG_ERR("xrst/vst timer match0 dma config fail %d", err);
        return err;
    }
    err = jdi_xrst_vst_m1_dma_config(dev);
    if (err) {
        LOG_ERR("xrst/vst timer match1 dma config fail %d", err);
        return err;
    }

    err = jdi_enb_m0_dma_config(dev);
    if (err) {
        LOG_ERR("enb timer match0 dma config fail %d", err);
        return err;
    }

    k_sem_init(&dev_data->sem, 0, 1);
    k_sem_give(&dev_data->sem);

    return 0;
}

static const struct display_driver_api flexio_jdi_api = {
    .blanking_on = jdi_display_blanking_on,
    .blanking_off = jdi_display_blanking_off,
    .write = jdi_write,
    .read = jdi_read,
    .get_framebuffer = jdi_get_framebuffer,
    .set_brightness = jdi_set_brightness,
    .set_contrast = jdi_set_contrast,
    .get_capabilities = jdi_get_capabilities,
    .set_pixel_format = jdi_set_pixel_format,
    .set_orientation = jdi_set_orientation,
};

#define BACKLIGHT_GPIO_INIT(id)				\
        COND_CODE_1(DT_NODE_HAS_PROP(DT_DRV_INST(id), backlight_gpios),	\
        (GPIO_DT_SPEC_INST_GET(id, backlight_gpios)), \
        ({							\
            .port = NULL,			\
            .pin = 0,				\
            .dt_flags = 0,			\
        }))

#define JDI_CLOCK_CONFIG(id, name)				\
        COND_CODE_1(DT_INST_CLOCKS_HAS_NAME(id, name), 		\
            (DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(id, name))),	\
            (NULL))

#define JDI_TIMER_DEV_CONFIG(id, name)				\
        COND_CODE_1(DT_INST_PROP_HAS_NAME(id, timers, name), 		\
            (DEVICE_DT_GET(DT_INST_PHANDLE_BY_NAME(id, timers, name))),	\
            (NULL))

#define JDI_TIMER_REG_ADDR_CONFIG(id, name)				\
        COND_CODE_1(DT_INST_PROP_HAS_NAME(id, timers, name), 		\
            (DT_REG_ADDR_BY_IDX(DT_INST_PHANDLE_BY_NAME(id, timers, name), 0)),	\
            (0))

#define JDI_TIMERS_CONFIG(id)						\
    .timer_dev	= {									\
        JDI_TIMER_DEV_CONFIG(id, pixel),			\
        JDI_TIMER_DEV_CONFIG(id, xrst),				\
        JDI_TIMER_DEV_CONFIG(id, enb),				\
    },												\
    .ctimer_base = {									\
        (const CTIMER_Type*)JDI_TIMER_REG_ADDR_CONFIG(id, pixel),			\
        (const CTIMER_Type*)JDI_TIMER_REG_ADDR_CONFIG(id, xrst),			\
        (const CTIMER_Type*)JDI_TIMER_REG_ADDR_CONFIG(id, enb),				\
    },

#define JDI_GPIO_CONFIG(id, name)                   \
        {                                           \
            .gpio = {                               \
                .port = DEVICE_DT_GET(DT_INST_PHANDLE_BY_NAME(id, gpios, name)),     \
                .pin = DT_INST_PHA_BY_NAME(id, gpios, name, pin),            \
                .dt_flags = DT_INST_PHA_BY_NAME(id, gpios, name, flags),     \
            },                                      \
            .gpio_base = (const GPIO_Type *)DT_REG_ADDR_BY_IDX(DT_INST_PHANDLE_BY_NAME(id, gpios, name), 0), \
            .port_no = DT_PROP(DT_INST_PHANDLE_BY_NAME(id, gpios, name), port),           \
        }


#define JDI_GPIOS_CONFIG(id)                        \
    .jdi_gpios = {                                  \
        [VST_GPIO] = JDI_GPIO_CONFIG(id, vst),      \
        [XRST_GPIO] = JDI_GPIO_CONFIG(id, xrst),    \
    },

#define FLEXIO_JDI_DEVICE(id)							\
    PINCTRL_DT_INST_DEFINE(id);							\
    /* static void flexio_jdi_config_func_##id(const struct device *dev); */	\
    static const struct jdi_config jdi_config_##id = {	\
        .flexio_base = (FLEXIO_Type *) DT_INST_REG_ADDR(id),			\
        .display_info = {							\
            .panel_width = DT_INST_PROP(id, width),			\
            .panel_height = DT_INST_PROP(id, height),		\
        },								\
        .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(id),			\
        .vcom_clock = JDI_CLOCK_CONFIG(id, vcom),	\
        .input_clock = JDI_CLOCK_CONFIG(id, input_clock),	\
        .backlight_gpio = BACKLIGHT_GPIO_INIT(id),	\
        JDI_TIMERS_CONFIG(id)				        \
        JDI_GPIOS_CONFIG(id)                        \
    };									\
    static struct jdi_data flexio_jdi_data_##id;			\
    DEVICE_DT_INST_DEFINE(id,						\
                &jdi_init,					\
                NULL,						\
                &flexio_jdi_data_##id,				\
                &jdi_config_##id,				\
                POST_KERNEL,					\
                CONFIG_DISPLAY_INIT_PRIORITY,			\
                &flexio_jdi_api);


DT_INST_FOREACH_STATUS_OKAY(FLEXIO_JDI_DEVICE)
