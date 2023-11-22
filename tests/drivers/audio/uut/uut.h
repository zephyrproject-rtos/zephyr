#ifndef UUT_H
#define UUT_H

#include <zephyr/audio/codec.h>

#include "drivers/gpio.h"
#include "i2c_mock.h"

int codec_claim_page(const struct device *dev, int page);
int codec_release_page(const struct device *dev, int page);
int codec_read_reg(const struct device *dev, uint16_t reg, uint8_t *val);
int codec_write_reg(const struct device *dev, uint16_t reg, uint8_t mask, uint8_t value);
int codec_init(const struct device *dev);
int codec_soft_reset(const struct device *dev);
int codec_active(const struct device *dev);
int codec_inactive(const struct device *dev);
int codec_mute(const struct device *dev);
int64_t codec_db2dvc(int vol);
int codec_get_output_volume(const struct device *dev);
int codec_set_output_volume_dvc(const struct device *dev, int vol);
int codec_db2gain(int gain);
int codec_set_output_gain_amp(const struct device *dev, int gain);
int codec_set_output_volume(const struct device *dev, int vol);
int codec_set_samplerate(const struct device *dev, int samplerate);
int codec_set_dai_fmt(const struct device *dev, struct audio_codec_cfg *cfg);
int codec_configure(const struct device *dev, struct audio_codec_cfg *cfg);
void codec_start_output(const struct device *dev);
void codec_stop_output(const struct device *dev);
int codec_configure_output(const struct device *dev);
int codec_read_all_regs(const struct device *dev);

struct codec_driver_config {
    struct i2c_dt_spec i2c;
    struct gpio_dt_spec supply_gpio;
};

struct codec_driver_data {
    int volume_lvl;
    struct k_sem page_sem;
};
#endif /* UUT_H */
