/*
 * I2S Audio Sample for LPC54S018
 *
 * This sample demonstrates I2S audio interface using FLEXCOMM6/7.
 * It generates a simple sine wave tone and outputs it via I2S.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <math.h>

LOG_MODULE_REGISTER(i2s_sample, LOG_LEVEL_INF);

/* Status LED */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* I2S Device - using FLEXCOMM6 as I2S0 */
#define I2S_TX_NODE DT_NODELABEL(i2s0)
static const struct device *i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);

/* Audio configuration */
#define SAMPLE_RATE     44100
#define CHANNELS        2
#define BITS_PER_SAMPLE 16
#define BYTES_PER_SAMPLE (BITS_PER_SAMPLE / 8)
#define BLOCK_SIZE      192
#define BLOCK_COUNT     4
#define TONE_FREQUENCY  1000  /* 1kHz test tone */

/* Audio buffers */
static int16_t audio_buffers[BLOCK_COUNT][BLOCK_SIZE * CHANNELS];
static struct k_mem_slab audio_mem_slab;
static char audio_mem_slab_buffer[BLOCK_COUNT * sizeof(void *)];

/* Generate sine wave samples */
static void generate_sine_wave(int16_t *buffer, size_t num_samples, 
                              uint32_t frequency, uint32_t sample_rate)
{
    static uint32_t phase = 0;
    const float amplitude = 0.5f * 32767;  /* 50% amplitude */
    
    for (size_t i = 0; i < num_samples; i++) {
        float sample = amplitude * sinf(2.0f * 3.14159f * phase / sample_rate);
        
        /* Stereo: same signal on both channels */
        buffer[i * CHANNELS] = (int16_t)sample;
        buffer[i * CHANNELS + 1] = (int16_t)sample;
        
        phase += frequency;
        if (phase >= sample_rate) {
            phase -= sample_rate;
        }
    }
}

/* I2S configuration */
static struct i2s_config i2s_cfg = {
    .word_size = BITS_PER_SAMPLE,
    .channels = CHANNELS,
    .format = I2S_FMT_DATA_FORMAT_I2S,
    .options = I2S_OPT_FRAME_CLK_MASTER | I2S_OPT_BIT_CLK_MASTER,
    .frame_clk_freq = SAMPLE_RATE,
    .mem_slab = &audio_mem_slab,
    .block_size = BLOCK_SIZE * CHANNELS * BYTES_PER_SAMPLE,
    .timeout = 1000,
};

/* Optional: Configure audio codec via I2C */
static void configure_audio_codec(void)
{
    const struct device *i2c_dev = DEVICE_DT_GET(DT_ALIAS(arduino_i2c));
    
    if (!device_is_ready(i2c_dev)) {
        LOG_WRN("I2C not ready for codec configuration");
        return;
    }
    
    /* Example: Configure a generic I2C audio codec */
    /* Replace with actual codec configuration for your hardware */
    uint8_t codec_addr = 0x1A;  /* Example codec address */
    uint8_t config_data[][2] = {
        {0x00, 0x00},  /* Reset codec */
        {0x01, 0x17},  /* Power up */
        {0x02, 0x00},  /* Configure I2S mode */
        /* Add more codec-specific configuration */
    };
    
    for (int i = 0; i < ARRAY_SIZE(config_data); i++) {
        int ret = i2c_write(i2c_dev, config_data[i], 2, codec_addr);
        if (ret < 0) {
            LOG_DBG("No codec found at 0x%02X", codec_addr);
            break;
        }
        k_sleep(K_MSEC(10));
    }
}

/* I2S transmit thread */
static void i2s_tx_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    void *tx_block;
    size_t tx_size;
    int ret;
    int block_idx = 0;
    
    LOG_INF("Starting I2S transmission");
    
    /* Prepare initial blocks */
    for (int i = 0; i < BLOCK_COUNT; i++) {
        ret = k_mem_slab_alloc(&audio_mem_slab, &tx_block, K_NO_WAIT);
        if (ret < 0) {
            LOG_ERR("Failed to allocate audio block: %d", ret);
            return;
        }
        
        /* Generate audio data */
        generate_sine_wave((int16_t *)tx_block, BLOCK_SIZE, 
                          TONE_FREQUENCY, SAMPLE_RATE);
        
        /* Queue block for transmission */
        ret = i2s_write(i2s_dev_tx, tx_block, i2s_cfg.block_size);
        if (ret < 0) {
            LOG_ERR("Failed to write I2S block: %d", ret);
            k_mem_slab_free(&audio_mem_slab, tx_block);
            return;
        }
    }
    
    /* Start transmission */
    ret = i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_START);
    if (ret < 0) {
        LOG_ERR("Failed to start I2S: %d", ret);
        return;
    }
    
    /* Continuous transmission loop */
    while (1) {
        /* Wait for a block to be transmitted */
        ret = i2s_read(i2s_dev_tx, &tx_block, &tx_size);
        if (ret < 0) {
            LOG_ERR("I2S read error: %d", ret);
            break;
        }
        
        /* Generate new audio data */
        generate_sine_wave((int16_t *)tx_block, BLOCK_SIZE, 
                          TONE_FREQUENCY, SAMPLE_RATE);
        
        /* Re-queue the block */
        ret = i2s_write(i2s_dev_tx, tx_block, tx_size);
        if (ret < 0) {
            LOG_ERR("Failed to write I2S block: %d", ret);
            break;
        }
        
        /* Toggle LED to show activity */
        if (device_is_ready(led.port)) {
            gpio_pin_toggle_dt(&led);
        }
    }
    
    /* Stop transmission on error */
    i2s_trigger(i2s_dev_tx, I2S_DIR_TX, I2S_TRIGGER_STOP);
}

K_THREAD_DEFINE(i2s_tx, 2048, i2s_tx_thread, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
    int ret;
    
    LOG_INF("I2S Audio Sample for LPC54S018");
    LOG_INF("Generating %d Hz sine wave", TONE_FREQUENCY);
    LOG_INF("Sample rate: %d Hz, %d channels, %d bits",
            SAMPLE_RATE, CHANNELS, BITS_PER_SAMPLE);
    
    /* Configure LED */
    if (device_is_ready(led.port)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED: %d", ret);
        }
    }
    
    /* Check if I2S device is ready */
    if (!device_is_ready(i2s_dev_tx)) {
        LOG_ERR("I2S device not ready");
        return -1;
    }
    
    /* Initialize memory slab for audio buffers */
    k_mem_slab_init(&audio_mem_slab, audio_mem_slab_buffer,
                    i2s_cfg.block_size, BLOCK_COUNT);
    
    /* Configure I2S */
    ret = i2s_configure(i2s_dev_tx, I2S_DIR_TX, &i2s_cfg);
    if (ret < 0) {
        LOG_ERR("Failed to configure I2S: %d", ret);
        return -1;
    }
    
    /* Optional: Configure audio codec */
    configure_audio_codec();
    
    LOG_INF("I2S configured successfully");
    LOG_INF("Connect an I2S DAC or amplifier to hear the tone");
    LOG_INF("Pins:");
    LOG_INF("  SCLK: P1.22 (FC6_SCK)");
    LOG_INF("  WS:   P2.30 (FC6_WS)");
    LOG_INF("  DATA: P2.28 (FC6_TXD)");
    LOG_INF("  MCLK: P1.31 (if needed)");
    
    /* Main loop - just sleep */
    while (1) {
        k_sleep(K_SECONDS(1));
    }
    
    return 0;
}