/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 *
 * @brief This file defines the private data structures for spi flash driver
 */

#ifndef __SPI_FLASH_W25QXXDV_H__
#define __SPI_FLASH_W25QXXDV_H__


struct spi_flash_data {
	struct device *spi;
	uint8_t buf[CONFIG_SPI_FLASH_W25QXXDV_MAX_DATA_LEN +
		    W25QXXDV_LEN_CMD_ADDRESS];
	struct k_sem sem;
};


#endif /* __SPI_FLASH_W25QXXDV_H__ */
