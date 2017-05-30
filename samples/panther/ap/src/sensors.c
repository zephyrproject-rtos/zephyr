/*
 * sensors.c
 *
 *  Created on: Mar 7, 2017
 *      Author: max
 */


#include <zephyr.h>
#include <stdio.h>
#include <device.h>
#include <sensor.h>
#include <misc/printk.h>

#if 1
#define DBG_PRINT(...) printk(__VA_ARGS__)
#else
#define DBG_PRINT(...)
#endif

/**
 * shield A0 definition structure
 */
typedef struct {
	struct device *devBME280;
	struct device *devADXL362;
	struct device *devADXRS290;
	struct device *devSI1153;
}sShieldA0Def, *psShieldA0Def;

/**
 *
 */
typedef struct {
	struct sensor_value BME280[3];
	struct sensor_value ADXL362[4];
	struct sensor_value ADXRS290[3];
	struct sensor_value SI1153[4];
}sShieldA0Sens;

sShieldA0Def shieldA0Def;
sShieldA0Sens sensors;
/*

	/ * test of sensors on SHIELD0 board  * /

	DBG_PRINT("Initialize Shield A0\n");
	ret = shieldA0Init();
	if (ret) {
		DBG_PRINT("Initialize Shield A0 fail\n");
	} else {
		DBG_PRINT("Initialize Shield A0 OK\n");
	}

	DBG_PRINT("Get sensors\n");
	while (1) {
		shieldA0SensorGet();
		shieldA0SensorPrint();
		k_sleep(MSEC_PER_SEC);
	}


*/

/**
 *
 */
int shieldA0Init(void)
{
	shieldA0Def.devBME280 = device_get_binding("BME280");
	if(!shieldA0Def.devBME280){
		DBG_PRINT("ERROR getting device BME280\n");
	}else{
		DBG_PRINT("dev %p name %s\n", shieldA0Def.devBME280, shieldA0Def.devBME280->config->name);
	}

	shieldA0Def.devADXL362 = device_get_binding("ADXL362");
	if(!shieldA0Def.devADXL362){
		DBG_PRINT("ERROR getting device ADXL362\n");
	}else{
		DBG_PRINT("dev %p name %s\n", shieldA0Def.devADXL362, shieldA0Def.devADXL362->config->name);
	}

	shieldA0Def.devADXRS290 = device_get_binding("ADXRS290");
	if(!shieldA0Def.devADXRS290){
		DBG_PRINT("ERROR getting device ADXRS290\n");
	}else{
		DBG_PRINT("dev %p name %s\n", shieldA0Def.devADXRS290, shieldA0Def.devADXRS290->config->name);
	}

	shieldA0Def.devSI1153 = device_get_binding("SI1153");
	if(!shieldA0Def.devSI1153){
		DBG_PRINT("ERROR getting device SI1153\n");
	}else{
		DBG_PRINT("dev %p name %s\n", shieldA0Def.devSI1153, shieldA0Def.devSI1153->config->name);
	}

	return 0;
}

int shieldA0SensorGet(void)
{
	if(shieldA0Def.devBME280){
		sensor_sample_fetch(shieldA0Def.devBME280);
		sensor_channel_get(shieldA0Def.devBME280, SENSOR_CHAN_TEMP    , &sensors.BME280[0]);
		sensor_channel_get(shieldA0Def.devBME280, SENSOR_CHAN_PRESS   , &sensors.BME280[1]);
		sensor_channel_get(shieldA0Def.devBME280, SENSOR_CHAN_HUMIDITY, &sensors.BME280[2]);
	}
	if(shieldA0Def.devADXL362){
		sensor_sample_fetch(shieldA0Def.devADXL362);
		sensor_channel_get(shieldA0Def.devADXL362, SENSOR_CHAN_ACCEL_X, &sensors.ADXL362[0]);
		sensor_channel_get(shieldA0Def.devADXL362, SENSOR_CHAN_ACCEL_Y, &sensors.ADXL362[1]);
		sensor_channel_get(shieldA0Def.devADXL362, SENSOR_CHAN_ACCEL_Z, &sensors.ADXL362[2]);
		sensor_channel_get(shieldA0Def.devADXL362, SENSOR_CHAN_TEMP, &sensors.ADXL362[3]);
	}
	if(shieldA0Def.devADXRS290){
		sensor_sample_fetch(shieldA0Def.devADXRS290);
		sensor_channel_get(shieldA0Def.devADXRS290, SENSOR_CHAN_GYRO_X, &sensors.ADXRS290[0]);
		sensor_channel_get(shieldA0Def.devADXRS290, SENSOR_CHAN_GYRO_Y, &sensors.ADXRS290[1]);
		sensor_channel_get(shieldA0Def.devADXRS290, SENSOR_CHAN_TEMP, &sensors.ADXRS290[2]);
	}
	if(shieldA0Def.devSI1153){
		sensor_sample_fetch(shieldA0Def.devSI1153);
		sensor_channel_get(shieldA0Def.devSI1153, SENSOR_CHAN_LIGHT, &sensors.SI1153[0]);
		sensor_channel_get(shieldA0Def.devSI1153, SENSOR_CHAN_PROX, &sensors.SI1153[1]);
	}
	//shield0SensorJSON();
	//sens_send(&sensors);
	return 0;
}

void shieldA0SensorPrint(void)
{
	printk("BME280 temp: %d.%06d; press: %d.%06d; humidity: %d.%06d\n",
			sensors.BME280[0].val1, sensors.BME280[0].val2,
			sensors.BME280[1].val1, sensors.BME280[1].val2,
			sensors.BME280[2].val1, sensors.BME280[2].val2);
	printk("ADXL362 Accel X:%d.%06d Y:%d.%06d Z:%d.%06d T:%d.%06d \n",
			sensors.ADXL362[0].val1, sensors.ADXL362[0].val2,
			sensors.ADXL362[1].val1, sensors.ADXL362[1].val2,
			sensors.ADXL362[2].val1, sensors.ADXL362[2].val2,
			sensors.ADXL362[3].val1, sensors.ADXL362[3].val2);
	printk("ADXRS290 Gyro X:%d.%06d Y:%d.%06d T:%d.%06d\n",
			sensors.ADXRS290[0].val1, sensors.ADXRS290[0].val2,
			sensors.ADXRS290[1].val1, sensors.ADXRS290[1].val2,
			sensors.ADXRS290[2].val1, sensors.ADXRS290[2].val2);
	printk("SI1153 amb:%d, prox1:%d, prox2:%d, prox3:%d\n",
			sensors.SI1153[0].val1, sensors.SI1153[1].val1, sensors.SI1153[2].val1, sensors.SI1153[3].val1);
}

