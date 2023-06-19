
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#include "adc_context.h"

#define DT_DRV_COMPAT microchip_mcp3425

LOG_MODULE_REGISTER(MCP3425, CONFIG_ADC_LOG_LEVEL);

#define MCP3425_CONFIG_GAIN(x) ((x)&BIT_MASK(2))
#define MCP3425_CONFIG_DR(x) (((x)&BIT_MASK(2)) << 2)
#define MCP3425_CONFIG_CM(x) (((x)&BIT_MASK(1)) << 4)

#define MCP3425_READY_BIT BIT(7)

#define MCP3425_VREF_INTERNAL 2048

enum {
  MCP3425_CONFIG_DR_RATE_240_RES_12 = 0,
  MCP3425_CONFIG_DR_RATE_60_RES_14 = 1,
  MCP3425_CONFIG_DR_RATE_15_RES_16 = 2,
};

enum {
  MCP3425_CONFIG_CM_SINGLE = 0,
  MCP3425_CONFIG_CM_CONTINUOUS = 1,
};

struct mcp3425_config {
  const struct i2c_dt_spec i2c_spec;
  k_tid_t acq_thread_id;
};

struct mcp3425_data {
  const struct device *dev;
  struct adc_context ctx;
  struct k_sem acq_lock;
  int16_t *buffer;
  int16_t *repeat_buffer;

  uint8_t resolution;
  uint8_t gain;
};

static int mcp3425_read_register(const struct device *dev, uint8_t *reg_val) {
  int rc;
  const struct mcp3425_config *config = dev->config;

  uint8_t read_buf[3] = {0x0};

  rc = i2c_read_dt(&config->i2c_spec, read_buf, sizeof(read_buf));

  reg_val[0] = read_buf[0];
  reg_val[1] = read_buf[1];
  reg_val[2] = read_buf[2];

  return rc;
}

static int mcp3425_write_register(const struct device *dev, uint8_t reg) {
  const struct mcp3425_config *config = dev->config;
  uint8_t msg[1] = {reg};

  return i2c_write_dt(&config->i2c_spec, msg, sizeof(msg));
}

static int mcp3425_channel_setup(const struct device *dev,
                                 const struct adc_channel_cfg *cfg) {
  struct mcp3425_data *data = dev->data;

  if (cfg->reference != ADC_REF_INTERNAL) {
    LOG_ERR("Invalid reference '%d", cfg->reference);
    return -EINVAL;
  }

  if (cfg->acquisition_time != ADC_ACQ_TIME_DEFAULT) {
    LOG_ERR("Unsupported acquistion_time '%d'", cfg->acquisition_time);
    return -ENOTSUP;
  }

  if (!cfg->differential) {
    LOG_ERR("Missing the input-negative property, please make sure you add a "
            "\"zephyr,input-negative\" property in the binding \n");
    return -ENOTSUP;
  }

  if ((cfg->gain != ADC_GAIN_1) && (cfg->gain != ADC_GAIN_2) &&
      (cfg->gain != ADC_GAIN_4) && (cfg->gain != ADC_GAIN_8)) {
    LOG_ERR("Unsupported gain selected '%d' \n", cfg->gain);
    return -EINVAL;
  }

  data->gain = cfg->gain;

  return 0;
}

static int mcp3425_start_read(const struct device *dev,
                              const struct adc_sequence *seq) {
  if (seq->channels != BIT(0)) {
    LOG_ERR("Selected channel(s) not supported: %x", seq->channels);
    return -EINVAL;
  }

  if (seq->oversampling) {
    LOG_ERR("Oversampling is not supported");
    return -EINVAL;
  }

  if (seq->calibrate) {
    LOG_ERR("Calibration is not supported");
    return -EINVAL;
  }

  if (!seq->buffer) {
    LOG_ERR("Buffer invalid");
    return -EINVAL;
  }

  const size_t num_extra_samples =
      seq->options ? seq->options->extra_samplings : 0;
  const size_t num_samples = (1 + num_extra_samples) * POPCOUNT(seq->channels);

  if (seq->buffer_size < (num_samples * sizeof(int16_t))) {
    LOG_ERR("buffer size too small");
    return -EINVAL;
  }

  struct mcp3425_data *data = dev->data;

  data->buffer = seq->buffer;

  adc_context_start_read(&data->ctx, seq);

  return adc_context_wait_for_completion(&data->ctx);
}

static int mcp3425_read_async(const struct device *dev,
                              const struct adc_sequence *seq,
                              struct k_poll_signal *async) {
  int ret;

  struct mcp3425_data *data = dev->data;

  adc_context_lock(&data->ctx, async ? true : false, async);
  ret = mcp3425_start_read(dev, seq);
  adc_context_release(&data->ctx, ret);

  return ret;
}

static int mcp3425_read(const struct device *dev,
                        const struct adc_sequence *seq) {
  return mcp3425_read_async(dev, seq, NULL);
}

static void adc_context_start_sampling(struct adc_context *ctx) {
  int ret;

  struct mcp3425_data *data = CONTAINER_OF(ctx, struct mcp3425_data, ctx);
  const struct device *dev = data->dev;

  uint8_t setup_config_reg = 0;

  setup_config_reg |= (MCP3425_CONFIG_GAIN(data->gain));
  if (data->ctx.sequence.resolution) {
    switch (data->ctx.sequence.resolution) {
    case 12U:
      setup_config_reg |= MCP3425_CONFIG_DR(MCP3425_CONFIG_DR_RATE_240_RES_12);
      break;
    case 14U:
      setup_config_reg |= MCP3425_CONFIG_DR(MCP3425_CONFIG_DR_RATE_60_RES_14);
      break;
    case 16U:
      setup_config_reg |= MCP3425_CONFIG_DR(MCP3425_CONFIG_DR_RATE_15_RES_16);
      break;
    default:
      LOG_ERR("Unsupported resolution: '%d'", data->ctx.sequence.resolution);
      return;
    }
  }

  setup_config_reg |= (MCP3425_CONFIG_CM(MCP3425_CONFIG_CM_SINGLE));
  setup_config_reg |= (MCP3425_READY_BIT); // Set ready bit to start
  ret = mcp3425_write_register(dev, setup_config_reg);
  if (ret) {
    LOG_WRN("Failed to start conversion");
  }

  data->repeat_buffer = data->buffer;

  k_sem_give(&data->acq_lock);
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx,
                                              bool repeat_sampling) {
  struct mcp3425_data *data = CONTAINER_OF(ctx, struct mcp3425_data, ctx);

  if (repeat_sampling) {
    data->buffer = data->repeat_buffer;
  }
}

static void mcp3425_acq_thread_fn(void *p1, void *p2, void *p3) {
  int ret;

  struct mcp3425_data *data = p1;
  const struct device *dev = data->dev;

  while (true) {
    k_sem_take(&data->acq_lock, K_FOREVER);

    uint8_t read_buffer[3] = {0x0};

    /*
     * Wait until sampling is done
     */
    do {
      k_sleep(K_USEC(100));

      ret = mcp3425_read_register(dev, read_buffer);
      if (ret < 0) {
        adc_context_complete(&data->ctx, ret);
      }
    } while (!(read_buffer[2] & MCP3425_READY_BIT));

    adc_context_complete(&data->ctx, ret);

    *data->buffer = sys_get_be16(read_buffer);

    data->buffer++;

    adc_context_on_sampling_done(&data->ctx, data->dev);
  }
}

static int mcp3425_init(const struct device *dev) {
  const struct mcp3425_config *config = dev->config;
  struct mcp3425_data *data = dev->data;

  k_sem_init(&data->acq_lock, 0, 1);

  if (!i2c_is_ready_dt(&config->i2c_spec)) {
    LOG_ERR("Bus not ready \n");
    return -EINVAL;
  }

  adc_context_unlock_unconditionally(&data->ctx);

  return 0;
}

static const struct adc_driver_api mcp3425_driver_api = {
    .channel_setup = mcp3425_channel_setup,
    .read = mcp3425_read,
    .ref_internal = MCP3425_VREF_INTERNAL,
#ifdef CONFIG_ADC_ASYNC
    .read_async = mcp3425_read_async,
#endif
};

#define MCP3425_INIT(n)                                                        \
  static const struct mcp3425_config inst_##n##_config;                        \
  static struct mcp3425_data inst_##n##_data;                                  \
  K_THREAD_DEFINE(inst_##n##_thread,                                           \
                  CONFIG_ADC_MCP3425_ACQUISITION_THREAD_STACK_SIZE,            \
                  mcp3425_acq_thread_fn, &inst_##n##_data, NULL, NULL,         \
                  CONFIG_ADC_MCP3425_INIT_PRIORITY, 0, 0);                     \
  static const struct mcp3425_config inst_##n##_config = {                     \
      .i2c_spec = I2C_DT_SPEC_INST_GET(n),                                     \
      .acq_thread_id = inst_##n##_thread,                                      \
  };                                                                           \
  static struct mcp3425_data inst_##n##_data = {                               \
      .dev = DEVICE_DT_INST_GET(n),                                            \
      ADC_CONTEXT_INIT_LOCK(inst_##n##_data, ctx),                             \
      ADC_CONTEXT_INIT_TIMER(inst_##n##_data, ctx),                            \
      ADC_CONTEXT_INIT_SYNC(inst_##n##_data, ctx)                              \
                                                                               \
  };                                                                           \
  DEVICE_DT_INST_DEFINE(                                                       \
      n, &mcp3425_init, NULL, &inst_##n##_data, &inst_##n##_config,            \
      POST_KERNEL, CONFIG_ADC_MCP3425_INIT_PRIORITY, &mcp3425_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MCP3425_INIT)

BUILD_ASSERT(CONFIG_I2C_INIT_PRIORITY < CONFIG_ADC_MCP3425_INIT_PRIORITY);