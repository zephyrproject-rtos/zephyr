#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul_usbc_vbus.h>
#include <zephyr/drivers/usb_c/emul_fusb302b.h>

#define DT_DRV_COMPAT fcs_fusb302b_vbus

struct fusb_vbus_emul_data {
};
struct fusb_vbus_emul_cfg {
	const struct emul *fusb_emul;
};

int fusb_vbus_emul_set_voltage(const struct emul *emul, int mV)
{
	const struct fusb_vbus_emul_cfg *cfg = emul->cfg;
	set_vbus(cfg->fusb_emul, mV);
	return 0;
}

int fusb_vbus_emul_init(const struct emul *emul, const struct device *parent)
{
	return 0;
}

static struct usbc_vbus_emul_driver_api fusb_vbus_emul_api = {
	.set_vbus_voltage = fusb_vbus_emul_set_voltage,
};

static struct i2c_emul_api fusb_vbus_i2c_api = {
	// Not really using SPI, but using fusb302b
};

#define FUSB_VBUS_EMUL_DATA(n) static struct fusb_vbus_emul_data fusb_vbus_emul_data_##n;

#define FUSB_VBUS_EMUL_DEFINE(n)                                                                   \
	EMUL_DT_INST_DEFINE(n, fusb_vbus_emul_init, &fusb_vbus_emul_data_##n,                      \
			    &fusb_vbus_emul_cfg_##n, &fusb_vbus_i2c_api, &fusb_vbus_emul_api)

#define FUSB_VBUS_EMUL(n)                                                                          \
	FUSB_VBUS_EMUL_DATA(n)                                                                     \
	static const struct fusb_vbus_emul_cfg fusb_vbus_emul_cfg_##n = {                          \
		.fusb_emul = EMUL_DT_GET(DT_INST_PHANDLE(n, fusb302b))};                           \
	FUSB_VBUS_EMUL_DEFINE(n)

DT_INST_FOREACH_STATUS_OKAY(FUSB_VBUS_EMUL)
