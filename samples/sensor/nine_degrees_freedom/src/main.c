/*
 * Copyright (c) 2018 Jose Sune Cofreros Jr.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Sample app for nine degrees of freedom sample data acquisition.
 *
 * Sample app for implementing MPU6050 and HMC5883L drivers and dump their
 * sensor data. Verified on Nucleo STM32L476RG and Intel Quark D2000.
 */

#include <zephyr.h>
#include <device.h>
#include <sensor.h>
#include <stdio.h>

void main(void)
{
	struct device *dev1 = device_get_binding("MPU6050");
	struct device *dev2 = device_get_binding("HMC5883L");

	if (!dev1) {
		printk("Cannot find MPU6050!\n");
		return;
	}
	if (!dev2) {
		printk("Cannot find HMC5883L!\n");
		return;
	}

	printk("dev1 %p name %s\n", dev1, dev1->config->name);
	printk("dev2 %p name %s\n", dev2, dev2->config->name);

	unsigned int i = 0;

	while (1) {

		struct sensor_value a_xyz, *a_xyz_ptr, a_x, a_y, a_z;
		struct sensor_value g_xyz, *g_xyz_ptr, g_x, g_y, g_z;
		struct sensor_value m_xyz, *m_xyz_ptr, m_x, m_y, m_z;
		a_xyz_ptr = &a_xyz;
		g_xyz_ptr = &g_xyz;
		m_xyz_ptr = &m_xyz;

		sensor_sample_fetch(dev1);
		sensor_channel_get(dev1, SENSOR_CHAN_ACCEL_XYZ, &a_xyz);
		sensor_channel_get(dev1, SENSOR_CHAN_ACCEL_X, &a_x);
		sensor_channel_get(dev1, SENSOR_CHAN_ACCEL_Y, &a_y);
		sensor_channel_get(dev1, SENSOR_CHAN_ACCEL_Z, &a_z);
		sensor_channel_get(dev1, SENSOR_CHAN_GYRO_XYZ, &g_xyz);
		sensor_channel_get(dev1, SENSOR_CHAN_GYRO_X, &g_x);
		sensor_channel_get(dev1, SENSOR_CHAN_GYRO_Y, &g_y);
		sensor_channel_get(dev1, SENSOR_CHAN_GYRO_Z, &g_z);

		sensor_sample_fetch(dev2);
		sensor_channel_get(dev2, SENSOR_CHAN_MAGN_XYZ, &m_xyz);
		sensor_channel_get(dev2, SENSOR_CHAN_MAGN_X, &m_x);
		sensor_channel_get(dev2, SENSOR_CHAN_MAGN_Y, &m_y);
		sensor_channel_get(dev2, SENSOR_CHAN_MAGN_Z, &m_z);

		printk("\n%06d. MPU6050\n===============\nFull-Axis Accelerometer Capture:\n", i);
		printk("    accel_x: %d.%06d; accel_y: %d.%06d; accel_z: %d.%06d\n", 
			a_xyz_ptr->val1, a_xyz_ptr->val2, 
			(a_xyz_ptr+2)->val1, (a_xyz_ptr+2)->val2, 
			(a_xyz_ptr+3)->val1, (a_xyz_ptr+3)->val2); 
		printk("Single-Axis Accelerometer Capture:\n");
		printk("    accel_x: %d.%06d; accel_y: %d.%06d; accel_z: %d.%06d\n", 
			(unsigned int)a_x.val1, (unsigned int)a_x.val2, 
			(unsigned int)a_y.val1, (unsigned int)a_y.val2, 
			(unsigned int)a_z.val1, (unsigned int)a_z.val2); 

		printk("Full-Axis Gyroscope Capture:\n");
		printk("    gyro_x: %d.%06d; gyro_y: %d.%06d; gyro_z: %d.%06d\n", 
			g_xyz_ptr->val1, g_xyz_ptr->val2, 
			(g_xyz_ptr+2)->val1, (g_xyz_ptr+2)->val2, 
			(g_xyz_ptr+3)->val1, (g_xyz_ptr+3)->val2); 
		printk("Single-Axis Gyroscope Capture:\n");
		printk("    gyro_x: %d.%06d; gyro_y: %d.%06d; gyro_z: %d.%06d\n\n", 
			(unsigned int)g_x.val1, (unsigned int)g_x.val2, 
			(unsigned int)g_y.val1, (unsigned int)g_y.val2, 
			(unsigned int)g_z.val1, (unsigned int)g_z.val2); 

		printk("\n%06d. HMC5883L\n================\nFull-Axis Magnetometer Capture:\n", i);
		printk("    mag_x: %d.%06d; mag_y: %d.%06d; mag_z: %d.%06d\n", 
			m_xyz_ptr->val1, m_xyz_ptr->val2, 
			(m_xyz_ptr+2)->val1, (m_xyz_ptr+2)->val2, 
			(m_xyz_ptr+3)->val1, (m_xyz_ptr+3)->val2); 
		printk("Single-Axis Magnetometer Capture:\n");
		printk("    mag_x: %d.%06d; mag_y: %d.%06d; mag_z: %d.%06d\n\n", 
			(unsigned int)m_x.val1, (unsigned int)m_x.val2, 
			(unsigned int)m_y.val1, (unsigned int)m_y.val2, 
			(unsigned int)m_z.val1, (unsigned int)m_z.val2); 

		k_sleep(1500);
		i++;
	}
}
