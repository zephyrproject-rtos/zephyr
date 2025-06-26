#include <zephyr/device.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_data_types.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/pmci/mctp/mctp.h>
#include <zephyr/pmci/mctp/mctp_i2c_gpio_target.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/timeutil.h>
#include <zenoh-pico.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sensor_hub_pub);

#define POLL_PERIOD_MS 1000

const struct device *const bme680 = DEVICE_DT_GET(DT_NODELABEL(bme680));

#define CHANS 4
#define BME680_CHANS(X, SEP) \
	X(0, "Ambient Temp (C)", SENSOR_CHAN_AMBIENT_TEMP, 0) SEP() \
	X(1, "Pressure (kP)", SENSOR_CHAN_PRESS, 0) SEP() \
	X(2, "Relative Humidity (%)", SENSOR_CHAN_HUMIDITY, 0)

#define COMMA_SEP() ,
#define SEMI_SEP() ;
#define CHAN_SPEC_X(_idx, _name, chan_type, chan_idx) {chan_type, chan_idx}
#define IODEV_CHANS BME680_CHANS(CHAN_SPEC_X, COMMA_SEP)

SENSOR_DT_READ_IODEV(bme680_io, DT_NODELABEL(bme680), IODEV_CHANS);

RTIO_DEFINE(r, 1, 1);

MCTP_I2C_GPIO_TARGET_DT_DEFINE(mctp_i2c_ctrl, DT_NODELABEL(mctp_i2c));

static uint8_t read_buf[128];

/* Readings into an array */
struct sensor_chan_spec chan_specs[CHANS] = {
	BME680_CHANS(CHAN_SPEC_X, COMMA_SEP)
};
struct sensor_q31_data readings[CHANS];


int poll_sensors()
{
	const struct sensor_decoder_api *decoder;
	int rc;

	rc = sensor_read(&bme680_io, &r, read_buf, ARRAY_SIZE(read_buf));
		rc = sensor_get_decoder(bme680, &decoder);

	if (rc != 0) {
		LOG_INF("%s: sensor_get_decode() failed: %d\n", bme680->name, rc);
		return rc;
	}

	for (int i = 0; i < CHANS; i++) {
		int frame_idx = 0;

		decoder->decode(read_buf, chan_specs[i],
			&frame_idx, 1, &readings[i]);
	}

	LOG_INF("Readings:\n");

#define CHAN_PRINT_X(idx, desc, chan_type, chan_idx) \
	printk("  %d: %s %s%d.%d\n", idx, desc, PRIq_arg(readings[idx].readings[0].value, 2, readings[idx].shift));

	BME680_CHANS(CHAN_PRINT_X, SEMI_SEP);

	return 0;
}

int pub_sensors()
{
	return 0;
}

#define MODE "peer"
#define LOCATOR "mctp/20"

#define KEYEXPR "demo/example/zenoh-pico-pub"
#define VALUE "Pub from Zenoh-Pico!"

#if Z_FEATURE_PUBLICATION == 1
int main(int argc, char **argv)
{
	LOG_INF("Sensor Pub, MCTP EID %d on %s\n", mctp_i2c_ctrl.endpoint_id, CONFIG_BOARD_TARGET);
	zephyr_mctp_register_bus(&mctp_i2c_ctrl.binding);

	// Initialize Zenoh Session and other parameters
	z_owned_config_t config;
	z_config_default(&config);
	zp_config_insert(z_loan_mut(config), Z_CONFIG_MODE_KEY, MODE);
	if (strcmp(LOCATOR, "") != 0) {
		zp_config_insert(z_loan_mut(config), Z_CONFIG_CONNECT_KEY, LOCATOR);
	}

	// Open Zenoh session
	LOG_INF("Opening Zenoh Session...");
	z_owned_session_t s;
	while (z_open(&s, z_move(config), NULL) < 0) {
		LOG_INF("Unable to open session, retrying in a few");
		k_msleep(5000);
	}


	// Start the receive and the session lease loop for zenoh-pico
	LOG_INF("Starting read+lease tasks");
	zp_start_read_task(z_loan_mut(s), NULL);
	zp_start_lease_task(z_loan_mut(s), NULL);

	LOG_INF("Declaring publisher for '%s'...", KEYEXPR);
	z_view_keyexpr_t ke;
	z_view_keyexpr_from_str_unchecked(&ke, KEYEXPR);
	z_owned_publisher_t pub;
	if (z_declare_publisher(z_loan(s), &pub, z_loan(ke), NULL) < 0) {
		LOG_ERR("Unable to declare publisher for key expression!");
		return -1;
	}


	LOG_INF("Polling sensors");
    	char buf[256];
    	uint32_t i = 0;
    	while (1) {
		k_msleep(POLL_PERIOD_MS);
		poll_sensors();

		/* pub_sensors();*/
		z_owned_bytes_t payload;

		sprintf(buf, "Temp (C): %s%d.%d, Pressure (kP): %s%d.%d, Relative Humidity (%%): %s%d.%d",
		        PRIq_arg(readings[0].readings[0].value, 2, readings[0].shift),
		        PRIq_arg(readings[1].readings[0].value, 2, readings[1].shift),
		        PRIq_arg(readings[2].readings[0].value, 2, readings[2].shift)
		);

        	/*sprintf(buf, "[%4d] %s", i, VALUE); */
	        z_bytes_copy_from_str(&payload, buf);
		/*z_bytes_from_buf(&payload, read_buf, ARRAY_SIZE(read_buf), NULL, NULL); */

		z_publisher_put(z_loan(pub), z_move(payload), NULL);
		i += 1;
	}

	LOG_INF("Closing Zenoh Session...");
	z_drop(z_move(pub));
	z_drop(z_move(s));
	LOG_INF("Done");
	return 0;
}
#else
int main(void) {
    printf("ERROR: Zenoh pico was compiled without Z_FEATURE_PUBLICATION but this example requires it.\n");
    return -2;
}
#endif

