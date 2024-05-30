#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>

LOG_MODULE_REGISTER(MAIN);

struct pmsX003_measurments
{
    struct sensor_value pm_1_0_cf;
    struct sensor_value pm_2_5_cf;
    struct sensor_value pm_10_cf;
    struct sensor_value pm_1_0_atm;
    struct sensor_value pm_2_5_atm;
    struct sensor_value pm_10_0_atm;
    struct sensor_value pm_0_3_count;
    struct sensor_value pm_0_5_count;
    struct sensor_value pm_1_0_count;
    struct sensor_value pm_2_5_count;
    struct sensor_value pm_5_0_count;
    struct sensor_value pm_10_0_count;
};

int main(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(pmsx003));

	if(!dev)
	{
		LOG_ERR("Error: no device found.");
		return 0;
	}

	if(!device_is_ready(dev))
	{
            LOG_ERR("\"%s\" is not configured properly. ", dev->name);
			return 0;
	}


	struct pmsX003_measurments data;
	while(1)
	{
        sensor_sample_fetch(dev);
		sensor_channel_get(dev,SENSOR_CHAN_PM_1_0_CF, &data.pm_1_0_cf) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_2_5_CF, &data.pm_2_5_cf) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_10_0_CF, &data.pm_10_cf) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_1_0_ATM, &data.pm_1_0_atm) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_2_5_ATM, &data.pm_2_5_atm) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_10_0_ATM, &data.pm_10_0_atm) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_0_3_COUNT, &data.pm_0_3_count) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_0_5_COUNT, &data.pm_0_5_count) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_1_0_COUNT, &data.pm_1_0_count) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_2_5_COUNT, &data.pm_2_5_count) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_5_0_COUNT, &data.pm_5_0_count) ;
        sensor_channel_get(dev,SENSOR_CHAN_PM_10_0_COUNT, &data.pm_10_0_count);
        
        LOG_INF("pm1.0_cf = %d.%02d µg/m³", data.pm_1_0_cf.val1, data.pm_1_0_cf.val2);
        LOG_INF("pm2.5_cf = %d.%02d µg/m³", data.pm_2_5_cf.val1, data.pm_2_5_cf.val2);
        LOG_INF("pm10_cf = %d.%02d µg/m³", data.pm_10_cf.val1, data.pm_10_cf.val2);
        LOG_INF("pm1.0_atm = %d.%02d µg/m³", data.pm_1_0_atm.val1, data.pm_1_0_atm.val2);
        LOG_INF("pm2.5_atm = %d.%02d µg/m³", data.pm_2_5_atm.val1, data.pm_2_5_atm.val2);
        LOG_INF("pm10_atm = %d.%02d µg/m³", data.pm_10_0_atm.val1, data.pm_10_0_atm.val2);
        LOG_INF("pm0.3_count = %d particles/0.1L", data.pm_0_3_count.val1);
        LOG_INF("pm0.5_count = %d particles/0.1L", data.pm_0_5_count.val1);
        LOG_INF("pm1.0_count = %d particles/0.1L", data.pm_1_0_count.val1);
        LOG_INF("pm2.5_count = %d particles/0.1L", data.pm_2_5_count.val1);
        LOG_INF("pm5.0_count = %d particles/0.1L", data.pm_5_0_count.val1);
        LOG_INF("pm10_count = %d particles/0.1L", data.pm_10_0_count.val1);

		k_sleep(K_SECONDS(10));
	}
	return 0;
}

