/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_HAS_DTS_I2C)

#ifdef CONFIG_I2C_NRFX

#ifndef DT_MS5837_DEV_NAME

#define DT_MS5837_DEV_NAME \
	DT_NORDIC_NRF_I2C_40004000_MEAS_MS5837_76_LABEL
#define DT_MS5837_I2C_MASTER_DEV_NAME \
	DT_NORDIC_NRF_I2C_40004000_MEAS_MS5837_76_BUS_NAME
#endif

#else

#ifndef DT_MS5837_DEV_NAME
#define DT_MS5837_DEV_NAME            ""
#define DT_MS5837_I2C_MASTER_DEV_NAME ""
#endif

#endif

#endif

