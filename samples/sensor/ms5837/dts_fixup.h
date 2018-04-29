/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_HAS_DTS_I2C)

#ifdef CONFIG_I2C_NRFX

#ifndef CONFIG_MS5837_DEV_NAME

#define CONFIG_MS5837_DEV_NAME \
	NORDIC_NRF_I2C_40004000_MEAS_MS5837_76_LABEL
#define CONFIG_MS5837_I2C_MASTER_DEV_NAME \
	NORDIC_NRF_I2C_40004000_MEAS_MS5837_76_BUS_NAME
#endif

#else

#ifndef CONFIG_MS5837_DEV_NAME
#define CONFIG_MS5837_DEV_NAME            ""
#define CONFIG_MS5837_I2C_MASTER_DEV_NAME ""
#endif

#endif

#endif

