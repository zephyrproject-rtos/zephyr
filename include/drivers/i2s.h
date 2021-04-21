/*
 * Copyright (c) 2017 Piotr Mienkowski
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the I2S (Inter-IC Sound) bus drivers.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_I2S_H_
#define ZEPHYR_INCLUDE_DRIVERS_I2S_H_

/**
 * @defgroup i2s_interface I2S Interface
 * @ingroup io_interfaces
 * @brief I2S (Inter-IC Sound) Interface
 *
 * The I2S API provides support for the standard I2S interface standard as well
 * as common non-standard extensions such as PCM Short/Long Frame Sync,
 * Left/Right Justified Data Format.
 * @{
 */

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The following #defines are used to configure the I2S controller.
 */


typedef uint8_t i2s_fmt_t;

/** Data Format bit field position. */
#define I2S_FMT_DATA_FORMAT_SHIFT           0
/** Data Format bit field mask. */
#define I2S_FMT_DATA_FORMAT_MASK            (0x7 << I2S_FMT_DATA_FORMAT_SHIFT)

/** @brief Standard I2S Data Format.
 *
 * Serial data is transmitted in two's complement with the MSB first. Both
 * Word Select (WS) and Serial Data (SD) signals are sampled on the rising edge
 * of the clock signal (SCK). The MSB is always sent one clock period after the
 * WS changes. Left channel data are sent first indicated by WS = 0, followed
 * by right channel data indicated by WS = 1.
 *
 *        -. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-.
 *     SCK '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '
 *        -.                               .-------------------------------.
 *     WS  '-------------------------------'                               '----
 *        -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.
 *     SD  |   |MSB|   |...|   |LSB| x |...| x |MSB|   |...|   |LSB| x |...| x |
 *        -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'
 *             | Left channel                  | Right channel                 |
 */
#define I2S_FMT_DATA_FORMAT_I2S             (0 << I2S_FMT_DATA_FORMAT_SHIFT)

/** @brief PCM Short Frame Sync Data Format.
 *
 * Serial data is transmitted in two's complement with the MSB first. Both
 * Word Select (WS) and Serial Data (SD) signals are sampled on the falling edge
 * of the clock signal (SCK). The falling edge of the frame sync signal (WS)
 * indicates the start of the PCM word. The frame sync is one clock cycle long.
 * An arbitrary number of data words can be sent in one frame.
 *
 *          .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-.
 *     SCK -' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-
 *          .---.                                                       .---.
 *     WS  -'   '-                                                     -'   '-
 *         -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---
 *     SD   |   |MSB|   |...|   |LSB|MSB|   |...|   |LSB|MSB|   |...|   |LSB|
 *         -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---
 *              | Word 1            | Word 2            | Word 3  |  Word n |
 */
#define I2S_FMT_DATA_FORMAT_PCM_SHORT       (1 << I2S_FMT_DATA_FORMAT_SHIFT)

/** @brief PCM Long Frame Sync Data Format.
 *
 * Serial data is transmitted in two's complement with the MSB first. Both
 * Word Select (WS) and Serial Data (SD) signals are sampled on the falling edge
 * of the clock signal (SCK). The rising edge of the frame sync signal (WS)
 * indicates the start of the PCM word. The frame sync has an arbitrary length,
 * however it has to fall before the start of the next frame. An arbitrary
 * number of data words can be sent in one frame.
 *
 *          .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-.
 *     SCK -' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-
 *              .--- ---.    ---.        ---.                               .---
 *     WS      -'       '-      '-          '-                             -'
 *         -.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---
 *     SD   |   |MSB|   |...|   |LSB|MSB|   |...|   |LSB|MSB|   |...|   |LSB|
 *         -'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---
 *              | Word 1            | Word 2            | Word 3  |  Word n |
 */
#define I2S_FMT_DATA_FORMAT_PCM_LONG        (2 << I2S_FMT_DATA_FORMAT_SHIFT)

/**
 * @brief Left Justified Data Format.
 *
 * Serial data is transmitted in two's complement with the MSB first. Both
 * Word Select (WS) and Serial Data (SD) signals are sampled on the rising edge
 * of the clock signal (SCK). The bits within the data word are left justified
 * such that the MSB is always sent in the clock period following the WS
 * transition. Left channel data are sent first indicated by WS = 1, followed
 * by right channel data indicated by WS = 0.
 *
 *          .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-.
 *     SCK -' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-
 *            .-------------------------------.                               .-
 *     WS  ---'                               '-------------------------------'
 *         ---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.-
 *     SD     |MSB|   |...|   |LSB| x |...| x |MSB|   |...|   |LSB| x |...| x |
 *         ---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'-
 *            | Left channel                  | Right channel                 |
 */
#define I2S_FMT_DATA_FORMAT_LEFT_JUSTIFIED  (3 << I2S_FMT_DATA_FORMAT_SHIFT)

/**
 * @brief Right Justified Data Format.
 *
 * Serial data is transmitted in two's complement with the MSB first. Both
 * Word Select (WS) and Serial Data (SD) signals are sampled on the rising edge
 * of the clock signal (SCK). The bits within the data word are right justified
 * such that the LSB is always sent in the clock period preceding the WS
 * transition. Left channel data are sent first indicated by WS = 1, followed
 * by right channel data indicated by WS = 0.
 *
 *          .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-. .-.
 *     SCK -' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-' '-
 *            .-------------------------------.                               .-
 *     WS  ---'                               '-------------------------------'
 *         ---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.---.-
 *     SD     | x |...| x |MSB|   |...|   |LSB| x |...| x |MSB|   |...|   |LSB|
 *         ---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'---'-
 *            | Left channel                  | Right channel                 |
 */
#define I2S_FMT_DATA_FORMAT_RIGHT_JUSTIFIED (4 << I2S_FMT_DATA_FORMAT_SHIFT)

/** Send MSB first */
#define I2S_FMT_DATA_ORDER_MSB              (0 << 3)
/** Send LSB first */
#define I2S_FMT_DATA_ORDER_LSB              BIT(3)
/** Invert bit ordering, send LSB first */
#define I2S_FMT_DATA_ORDER_INV              I2S_FMT_DATA_ORDER_LSB

/** Data Format bit field position. */
#define I2S_FMT_CLK_FORMAT_SHIFT           4
/** Data Format bit field mask. */
#define I2S_FMT_CLK_FORMAT_MASK            (0x3 << I2S_FMT_CLK_FORMAT_SHIFT)

/** Invert bit clock */
#define I2S_FMT_BIT_CLK_INV                 BIT(4)
/** Invert frame clock */
#define I2S_FMT_FRAME_CLK_INV               BIT(5)

/** NF represents "Normal Frame" whereas IF represents "Inverted Frame"
 *  NB represents "Normal Bit Clk" whereas IB represents "Inverted Bit clk"
 */
#define I2S_FMT_CLK_NF_NB		(0 << I2S_FMT_CLK_FORMAT_SHIFT)
#define I2S_FMT_CLK_NF_IB		(1 << I2S_FMT_CLK_FORMAT_SHIFT)
#define I2S_FMT_CLK_IF_NB		(2 << I2S_FMT_CLK_FORMAT_SHIFT)
#define I2S_FMT_CLK_IF_IB		(3 << I2S_FMT_CLK_FORMAT_SHIFT)

typedef uint8_t i2s_opt_t;

/** Run bit clock continuously */
#define I2S_OPT_BIT_CLK_CONT                (0 << 0)
/** Run bit clock when sending data only */
#define I2S_OPT_BIT_CLK_GATED               BIT(0)
/** I2S driver is bit clock master */
#define I2S_OPT_BIT_CLK_MASTER              (0 << 1)
/** I2S driver is bit clock slave */
#define I2S_OPT_BIT_CLK_SLAVE               BIT(1)
/** I2S driver is frame clock master */
#define I2S_OPT_FRAME_CLK_MASTER            (0 << 2)
/** I2S driver is frame clock slave */
#define I2S_OPT_FRAME_CLK_SLAVE             BIT(2)

/** @brief Loop back mode.
 *
 * In loop back mode RX input will be connected internally to TX output.
 * This is used primarily for testing.
 */
#define I2S_OPT_LOOPBACK                    BIT(7)

/** @brief Ping pong mode
 *
 * In ping pong mode TX output will keep alternating between a ping buffer
 * and a pong buffer. This is normally used in audio streams when one buffer
 * is being populated while the other is being played (DMAed) and vice versa.
 * So, in this mode, 2 sets of buffers fixed in size are used. Static Arrays
 * are used to achieve this and hence they are never freed.
 */
#define I2S_OPT_PINGPONG                    BIT(6)

/**
 * @brief I2C Direction
 */
enum i2s_dir {
	/** Receive data */
	I2S_DIR_RX,
	/** Transmit data */
	I2S_DIR_TX,
};

/** Interface state */
enum i2s_state {
	/** @brief The interface is not ready.
	 *
	 * The interface was initialized but is not yet ready to receive /
	 * transmit data. Call i2s_configure() to configure interface and change
	 * its state to READY.
	 */
	I2S_STATE_NOT_READY,
	/** The interface is ready to receive / transmit data. */
	I2S_STATE_READY,
	/** The interface is receiving / transmitting data. */
	I2S_STATE_RUNNING,
	/** The interface is draining its transmit queue. */
	I2S_STATE_STOPPING,
	/** TX buffer underrun or RX buffer overrun has occurred. */
	I2S_STATE_ERROR,
};

/** Trigger command */
enum i2s_trigger_cmd {
	/** @brief Start the transmission / reception of data.
	 *
	 * If I2S_DIR_TX is set some data has to be queued for transmission by
	 * the i2s_write() function. This trigger can be used in READY state
	 * only and changes the interface state to RUNNING.
	 */
	I2S_TRIGGER_START,
	/** @brief Stop the transmission / reception of data.
	 *
	 * Stop the transmission / reception of data at the end of the current
	 * memory block. This trigger can be used in RUNNING state only and at
	 * first changes the interface state to STOPPING. When the current TX /
	 * RX block is transmitted / received the state is changed to READY.
	 * Subsequent START trigger will resume transmission / reception where
	 * it stopped.
	 */
	I2S_TRIGGER_STOP,
	/** @brief Empty the transmit queue.
	 *
	 * Send all data in the transmit queue and stop the transmission.
	 * If the trigger is applied to the RX queue it has the same effect as
	 * I2S_TRIGGER_STOP. This trigger can be used in RUNNING state only and
	 * at first changes the interface state to STOPPING. When all TX blocks
	 * are transmitted the state is changed to READY.
	 */
	I2S_TRIGGER_DRAIN,
	/** @brief Discard the transmit / receive queue.
	 *
	 * Stop the transmission / reception immediately and discard the
	 * contents of the respective queue. This trigger can be used in any
	 * state other than NOT_READY and changes the interface state to READY.
	 */
	I2S_TRIGGER_DROP,
	/** @brief Prepare the queues after underrun/overrun error has occurred.
	 *
	 * This trigger can be used in ERROR state only and changes the
	 * interface state to READY.
	 */
	I2S_TRIGGER_PREPARE,
};

/** @struct i2s_config
 * @brief Interface configuration options.
 *
 * Memory slab pointed to by the mem_slab field has to be defined and
 * initialized by the user. For I2S driver to function correctly number of
 * memory blocks in a slab has to be at least 2 per queue. Size of the memory
 * block should be multiple of frame_size where frame_size = (channels *
 * word_size_bytes). As an example 16 bit word will occupy 2 bytes, 24 or 32
 * bit word will occupy 4 bytes.
 *
 * Please check Zephyr Kernel Primer for more information on memory slabs.
 *
 * @remark When I2S data format is selected parameter channels is ignored,
 * number of words in a frame is always 2.
 *
 * @param word_size Number of bits representing one data word.
 * @param channels Number of words per frame.
 * @param format Data stream format as defined by I2S_FMT_* constants.
 * @param options Configuration options as defined by I2S_OPT_* constants.
 * @param frame_clk_freq Frame clock (WS) frequency, this is sampling rate.
 * @param mem_slab memory slab to store RX/TX data.
 * @param block_size Size of one RX/TX memory block (buffer) in bytes.
 * @param timeout Read/Write timeout. Number of milliseconds to wait in case TX
 *        queue is full or RX queue is empty, or 0, or SYS_FOREVER_MS.
 */
struct i2s_config {
	uint8_t word_size;
	uint8_t channels;
	i2s_fmt_t format;
	i2s_opt_t options;
	uint32_t frame_clk_freq;
	struct k_mem_slab *mem_slab;
	size_t block_size;
	int32_t timeout;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * For internal use only, skip these in public documentation.
 */
__subsystem struct i2s_driver_api {
	int (*configure)(const struct device *dev, enum i2s_dir dir,
			 struct i2s_config *cfg);
	struct i2s_config *(*config_get)(const struct device *dev,
					 enum i2s_dir dir);
	int (*read)(const struct device *dev, void **mem_block, size_t *size);
	int (*write)(const struct device *dev, void *mem_block, size_t size);
	int (*trigger)(const struct device *dev, enum i2s_dir dir,
		       enum i2s_trigger_cmd cmd);
};
/**
 * @endcond
 */

/**
 * @brief Configure operation of a host I2S controller.
 *
 * The dir parameter specifies if Transmit (TX) or Receive (RX) direction
 * will be configured by data provided via cfg parameter.
 *
 * The function can be called in NOT_READY or READY state only. If executed
 * successfully the function will change the interface state to READY.
 *
 * If the function is called with the parameter cfg->frame_clk_freq set to 0
 * the interface state will be changed to NOT_READY.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dir Stream direction: RX or TX as defined by I2S_DIR_*
 * @param cfg Pointer to the structure containing configuration parameters.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid argument.
 */
__syscall int i2s_configure(const struct device *dev, enum i2s_dir dir,
			    struct i2s_config *cfg);

static inline int z_impl_i2s_configure(const struct device *dev,
				       enum i2s_dir dir,
				       struct i2s_config *cfg)
{
	const struct i2s_driver_api *api =
		(const struct i2s_driver_api *)dev->api;

	return api->configure(dev, dir, cfg);
}

/**
 * @brief Fetch configuration information of a host I2S controller
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dir Stream direction: RX or TX as defined by I2S_DIR_*
 * @retval Pointer to the structure containing configuration parameters,
 *         or NULL if un-configured
 */
static inline struct i2s_config *i2s_config_get(const struct device *dev,
						enum i2s_dir dir)
{
	const struct i2s_driver_api *api =
		(const struct i2s_driver_api *)dev->api;

	return api->config_get(dev, dir);
}

/**
 * @brief Read data from the RX queue.
 *
 * Data received by the I2S interface is stored in the RX queue consisting of
 * memory blocks preallocated by this function from rx_mem_slab (as defined by
 * i2s_configure). Ownership of the RX memory block is passed on to the user
 * application which has to release it.
 *
 * The data is read in chunks equal to the size of the memory block. If the
 * interface is in READY state the number of bytes read can be smaller.
 *
 * If there is no data in the RX queue the function will block waiting for
 * the next RX memory block to fill in. This operation can timeout as defined
 * by i2s_configure. If the timeout value is set to K_NO_WAIT the function
 * is non-blocking.
 *
 * Reading from the RX queue is possible in any state other than NOT_READY.
 * If the interface is in the ERROR state it is still possible to read all the
 * valid data stored in RX queue. Afterwards the function will return -EIO
 * error.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mem_block Pointer to the RX memory block containing received data.
 * @param size Pointer to the variable storing the number of bytes read.
 *
 * @retval 0 If successful.
 * @retval -EIO The interface is in NOT_READY or ERROR state and there are no
 *         more data blocks in the RX queue.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
static inline int i2s_read(const struct device *dev, void **mem_block,
				 size_t *size)
{
	const struct i2s_driver_api *api =
		(const struct i2s_driver_api *)dev->api;

	return api->read(dev, mem_block, size);
}

/**
 * @brief Read data from the RX queue into a provided buffer
 *
 * Data received by the I2S interface is stored in the RX queue consisting of
 * memory blocks preallocated by this function from rx_mem_slab (as defined by
 * i2s_configure). Calling this function removes one block from the queue
 * which is copied into the provided buffer and then freed.
 *
 * The provided buffer must be large enough to contain a full memory block
 * of data, which is parameterized for the channel via i2s_configure().
 *
 * This function is otherwise equivalent to i2s_read().
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Destination buffer for read data, which must be at least the
 *            as large as the configured memory block size for the RX channel.
 * @param size Pointer to the variable storing the number of bytes read.
 *
 * @retval 0 If successful.
 * @retval -EIO The interface is in NOT_READY or ERROR state and there are no
 *         more data blocks in the RX queue.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
__syscall int i2s_buf_read(const struct device *dev, void *buf, size_t *size);

/**
 * @brief Write data to the TX queue.
 *
 * Data to be sent by the I2S interface is stored first in the TX queue. TX
 * queue consists of memory blocks preallocated by the user from tx_mem_slab
 * (as defined by i2s_configure). This function takes ownership of the memory
 * block and will release it when all data are transmitted.
 *
 * If there are no free slots in the TX queue the function will block waiting
 * for the next TX memory block to be send and removed from the queue. This
 * operation can timeout as defined by i2s_configure. If the timeout value is
 * set to K_NO_WAIT the function is non-blocking.
 *
 * Writing to the TX queue is only possible if the interface is in READY or
 * RUNNING state.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param mem_block Pointer to the TX memory block containing data to be sent.
 * @param size Number of bytes to write. This value has to be equal or smaller
 *        than the size of the memory block.
 *
 * @retval 0 If successful.
 * @retval -EIO The interface is not in READY or RUNNING state.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
static inline int i2s_write(const struct device *dev, void *mem_block,
			    size_t size)
{
	const struct i2s_driver_api *api =
		(const struct i2s_driver_api *)dev->api;

	return api->write(dev, mem_block, size);
}

/**
 * @brief Write data to the TX queue from a provided buffer
 *
 * This function acquires a memory block from the I2S channel TX queue
 * and copies the provided data buffer into it. It is otherwise equivalent
 * to i2s_write().
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Pointer to a buffer containing the data to transmit.
 * @param size Number of bytes to write. This value has to be equal or smaller
 *        than the size of the channel's TX memory block configuration.
 *
 * @retval 0 If successful.
 * @retval -EIO The interface is not in READY or RUNNING state.
 * @retval -EBUSY Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -ENOMEM No memory in TX slab queue.
 * @retval -EINVAL Size parameter larger than TX queue memory block.
 */
__syscall int i2s_buf_write(const struct device *dev, void *buf, size_t size);

/**
 * @brief Send a trigger command.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param dir Stream direction: RX or TX.
 * @param cmd Trigger command.
 *
 * @retval 0 If successful.
 * @retval -EINVAL Invalid argument.
 * @retval -EIO The trigger cannot be executed in the current state or a DMA
 *         channel cannot be allocated.
 * @retval -ENOMEM RX/TX memory block not available.
 */
__syscall int i2s_trigger(const struct device *dev, enum i2s_dir dir,
			  enum i2s_trigger_cmd cmd);

static inline int z_impl_i2s_trigger(const struct device *dev,
				     enum i2s_dir dir,
				     enum i2s_trigger_cmd cmd)
{
	const struct i2s_driver_api *api =
		(const struct i2s_driver_api *)dev->api;

	return api->trigger(dev, dir, cmd);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/i2s.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_I2S_H_ */
