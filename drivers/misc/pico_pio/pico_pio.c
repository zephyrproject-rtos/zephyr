#include <device.h>
#include <drivers/pinctrl.h>

#define DT_DRV_COMPAT raspberrypi_pico_pio

struct pico_pio_config {
	const struct pinctrl_dev_config *pcfg;
};

static int pico_pio_init(const struct device *dev)
{
	int ret;
	const struct pico_pio_config *config = dev->config;

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

#define PICO_PIO_INIT(idx)							\
	PINCTRL_DT_INST_DEFINE(idx);						\
	static const struct pico_pio_config pico_pio##idx##_config = {		\
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),			\
	};									\
	DEVICE_DT_INST_DEFINE(idx, pico_pio_init,				\
			      NULL, NULL,					\
			      &pico_pio##idx##_config, PRE_KERNEL_1,		\
			      CONFIG_SERIAL_INIT_PRIORITY,			\
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PICO_PIO_INIT)
