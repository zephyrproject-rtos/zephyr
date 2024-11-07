#include "i2s_handler.h"

#include <zephyr/kernel.h>
#include <nrfx_clock.h>
#include <audio_i2s.h>
#include <macros_common.h>
#include <hw_codec.h>
#include "data_fifo.h"

/* For the tone test */
#include "pcm_mix.h"
#include <tone.h>
#include "contin_array.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2s_handler, CONFIG_I2S_HANDLER_LOG_LEVEL);

#define FIFO_RX_BLOCK_COUNT (CONFIG_FIFO_FRAME_SPLIT_NUM * CONFIG_FIFO_RX_FRAME_COUNT)

#define BLK_PERIOD_US 1000

/* Number of audio blocks given a duration */
#define NUM_BLKS(d) ((d) / BLK_PERIOD_US)
/* Single audio block size in number of samples (stereo) */
/* clang-format off */
#define BLK_SIZE_SAMPLES(r) (((r)*BLK_PERIOD_US) / 1000000)
#define NEXT_IDX(i) (((i) < (FIFO_NUM_BLKS - 1)) ? ((i) + 1) : 0)
/* Decrement sample FIFO index by one block */
#define PREV_IDX(i) (((i) > 0) ? ((i)-1) : (FIFO_NUM_BLKS - 1))
/* clang-format on */

#define FIFO_SMPL_PERIOD_US (CONFIG_AUDIO_MAX_PRES_DLY_US * 2)
#define FIFO_NUM_BLKS		NUM_BLKS(FIFO_SMPL_PERIOD_US)
#define MAX_FIFO_SIZE		(FIFO_NUM_BLKS * BLK_SIZE_SAMPLES(CONFIG_AUDIO_SAMPLE_RATE_HZ) * 2)

#define BLK_MONO_NUM_SAMPS	 BLK_SIZE_SAMPLES(CONFIG_AUDIO_SAMPLE_RATE_HZ)
#define BLK_STEREO_NUM_SAMPS   (BLK_MONO_NUM_SAMPS * 2)
/* Number of octets in a single audio block */
#define BLK_MONO_SIZE_OCTETS   (BLK_MONO_NUM_SAMPS * CONFIG_AUDIO_BIT_DEPTH_OCTETS)
#define BLK_STEREO_SIZE_OCTETS (BLK_MONO_SIZE_OCTETS * 2)

/* FIFO declaration for rx */
DATA_FIFO_DEFINE(fifo_rx, FIFO_RX_BLOCK_COUNT, WB_UP(BLOCK_SIZE_BYTES));

struct {
#if CONFIG_AUDIO_BIT_DEPTH_16
		int16_t __aligned(sizeof(uint32_t)) fifo[MAX_FIFO_SIZE];
#elif CONFIG_AUDIO_BIT_DEPTH_32
		int32_t __aligned(sizeof(uint32_t)) fifo[MAX_FIFO_SIZE];
#endif
		uint16_t blk_idx; /* Output consumer audio block index */
	} tx_struct;

static uint16_t test_tone_buf[CONFIG_AUDIO_SAMPLE_RATE_HZ / 100];
static size_t test_tone_size;

/* To test sine sound */
static void tone_mix(uint8_t *tx_buf)
{
	int ret;
	int8_t tone_buf_continuous[BLK_MONO_SIZE_OCTETS];
	static uint32_t finite_pos;

	ret = tone_gen(test_tone_buf, &test_tone_size, 1000, CONFIG_AUDIO_SAMPLE_RATE_HZ, 1.0);
	ERR_CHK(ret);

	ret = contin_array_create(tone_buf_continuous, BLK_MONO_SIZE_OCTETS, test_tone_buf,
				  test_tone_size, &finite_pos);
	ERR_CHK(ret);

	ret = pcm_mix(tx_buf, BLK_STEREO_SIZE_OCTETS, tone_buf_continuous, BLK_MONO_SIZE_OCTETS,
			  B_MONO_INTO_A_STEREO_L);
	ERR_CHK(ret);
}

/*
 * This handler function is called every time I2S needs new buffers for
 * TX and RX data.
 *
 * The new TX data buffer is the next consumer block in out.fifo.
 *
 * The new RX data buffer is the first empty slot of in.fifo.
 * New I2S RX data is located in rx_buf_released, and is locked into
 * the in.fifo message queue.
 */
static void audio_i2s_blk_complete(uint32_t frame_start_ts_us, uint32_t *rx_buf_released,
						uint32_t const *tx_buf_released)
{
	int ret;

	/********** I2S TX **********/
	static uint8_t *tx_buf;

	/* Create a fifo that keeps the last message of the fifo.in */
	/* Take the last added message of that secondary fifo and feed it to the tx buffer */

	if (tx_buf_released != NULL) {
		tx_buf = (uint8_t *)&tx_struct.fifo[tx_struct.blk_idx * BLK_STEREO_NUM_SAMPS];
		ERR_CHK(ret);
	}

	/* This work where we can consistently write a sin 1kHz in the buffer */
	/* tone_mix(tx_buf); */

	/********** I2S RX **********/
	static uint32_t *rx_buf;
	static int prev_ret;

	/* Lock last filled buffer into message queue */
	if (rx_buf_released != NULL) {
		ret = data_fifo_block_lock(&fifo_rx, (void **)&rx_buf_released,
						BLOCK_SIZE_BYTES);

		ERR_CHK_MSG(ret, "Unable to lock block RX");
	}

	/* Get new empty buffer to send to I2S HW */
	ret = data_fifo_pointer_first_vacant_get(&fifo_rx, (void **)&rx_buf, K_NO_WAIT);
	if (ret == 0 && prev_ret == -ENOMEM) {
		LOG_WRN("I2S RX continuing stream");
		prev_ret = ret;
	}

	/* Nothing below this should be changed */
	/* this is basically throwing out the oldest package in case the fifo is full */
	/* However that thrown out packet we can use for the echo */
	/* If RX FIFO is filled up */
	if (ret == -ENOMEM) {
		void *data;
		size_t size;

		if (ret != prev_ret) {
			LOG_WRN("I2S RX overrun. Single msg");
			prev_ret = ret;
		}

		/* this is thrown out otherwise */
		/* important to be forever however not possible for a callback */
		/* function so you in a proper solution you need to use a thread */
		ret = data_fifo_pointer_last_filled_get(&fifo_rx, &data, &size,
							K_NO_WAIT);

		ERR_CHK(ret);

		/* hence our ad hoc solution is to update the "fifo" of out */
		/* add to the fifo of tx so next iteration it can be used */
		uint32_t out_blk_idx = tx_struct.blk_idx;

		if (IS_ENABLED(CONFIG_AUDIO_BIT_DEPTH_16)) {
			memcpy(&tx_struct.fifo[out_blk_idx * BLK_STEREO_NUM_SAMPS],
			data, BLK_STEREO_SIZE_OCTETS);
		} else if (IS_ENABLED(CONFIG_AUDIO_BIT_DEPTH_32)) {
			/* TODO */
		}

		tx_struct.blk_idx = NEXT_IDX(out_blk_idx);

		data_fifo_block_free(&fifo_rx, data);

		ret = data_fifo_pointer_first_vacant_get(&fifo_rx, (void **)&rx_buf, K_NO_WAIT);
	}

	/*** Data exchange ***/
	audio_i2s_set_next_buf(tx_buf, rx_buf);

}

void audio_start(void)
{
	int ret;
	uint32_t alloced_cnt;
	uint32_t locked_cnt;

	/* Double buffer I2S */
	uint8_t *tx_buf_one = NULL;
	uint8_t *tx_buf_two = NULL;
	uint32_t *rx_buf_one = NULL;
	uint32_t *rx_buf_two = NULL;

	/* Audio example creates the encoder thread here, but we don't have any bt operations */

	/* Here we enable both direction of the hw codec (out and mic/line in) */
	ret = hw_codec_default_conf_enable();
	ERR_CHK(ret);

	/* for tx we use a struct like in audio_datapath of nordic audio application */
	memset(&tx_struct, 0, sizeof(tx_struct));

	/* link tx buffer with i2s */
	tx_struct.blk_idx = PREV_IDX(tx_struct.blk_idx);
	tx_buf_one = (uint8_t *)&tx_struct.fifo[tx_struct.blk_idx * BLK_STEREO_NUM_SAMPS];
	tx_struct.blk_idx = PREV_IDX(tx_struct.blk_idx);
	tx_buf_two = (uint8_t *)&tx_struct.fifo[tx_struct.blk_idx * BLK_STEREO_NUM_SAMPS];

	/* link rx buffer with i2s */
	ret = data_fifo_num_used_get(&fifo_rx, &alloced_cnt, &locked_cnt);
	if (alloced_cnt || locked_cnt || ret) {
		ERR_CHK_MSG(-ENOMEM, "FIFO is not empty!");
	}

	ret = data_fifo_pointer_first_vacant_get(&fifo_rx, (void **)&rx_buf_one, K_NO_WAIT);
	ERR_CHK_MSG(ret, "RX failed to get block");
	ret = data_fifo_pointer_first_vacant_get(&fifo_rx, (void **)&rx_buf_two, K_NO_WAIT);
	ERR_CHK_MSG(ret, "RX failed to get block");

	/* Start I2S */
	audio_i2s_start(tx_buf_one, rx_buf_one);
	audio_i2s_set_next_buf(tx_buf_two, rx_buf_two);
}

void audio_stop(void)
{
	int ret;

	ret = hw_codec_soft_reset();
	ERR_CHK(ret);

	audio_i2s_stop();

	data_fifo_empty(&fifo_rx);
}

int audio_init(void)
{
	int ret;

	/* Set callback that is called whenever an interrupt happens by I2S */
	audio_i2s_blk_comp_cb_register(audio_i2s_blk_complete);
	audio_i2s_init();

	/* Init hardware codec and soft reset register, luckily done for us by audio application */
	ret = hw_codec_init();
	if (ret) {
		LOG_ERR("Failed to initialize HW codec: %d", ret);
		return ret;
	}

	/* Also creates a thread to control volume */
	hw_codec_volume_set(100);

	/* initialize fifo, only use one for rx */
	/* as we don't need the decoder part fifo_rx = ctrl_blk.in.fifo */
	/* done here because we only want to init the fifo once */
	ret = data_fifo_init(&fifo_rx);
	ERR_CHK_MSG(ret, "Failed to set up rx FIFO");

	return 0;
}
