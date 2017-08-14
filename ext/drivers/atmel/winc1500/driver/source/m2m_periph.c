/**
 *
 * \file
 *
 * \brief NMC1500 Peripherials Application Interface.
 *
 * Copyright (c) 2016 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */


/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
INCLUDES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

#include "driver/include/m2m_periph.h"
#include "driver/source/nmasic.h"
#include "m2m_hif.h"

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
MACROS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
#define GPIO_OP_DIR     0
#define GPIO_OP_SET     1
#define GPIO_OP_GET     2
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
DATA TYPES
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/

/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
STATIC FUNCTIONS
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/
static sint8 get_gpio_idx(uint8 u8GpioNum)
{
	if(u8GpioNum >= M2M_PERIPH_GPIO_MAX) return -1;
	if(u8GpioNum == M2M_PERIPH_GPIO15) { return 15;
	} else if(u8GpioNum == M2M_PERIPH_GPIO16) { return 16;
	} else if(u8GpioNum == M2M_PERIPH_GPIO18) { return 18;
	} else if(u8GpioNum == M2M_PERIPH_GPIO3) { return 3;
	} else if(u8GpioNum == M2M_PERIPH_GPIO4) { return 4;
	} else if(u8GpioNum == M2M_PERIPH_GPIO5) { return 5;
	} else if(u8GpioNum == M2M_PERIPH_GPIO6) { return 6;
	} else {
		return -2;
	}
}
/*
 * GPIO read/write skeleton with wakeup/sleep capability.
 */
static sint8 gpio_ioctl(uint8 op, uint8 u8GpioNum, uint8 u8InVal, uint8 * pu8OutVal)
{
	sint8 ret, gpio;

	ret = hif_chip_wake();
	if(ret != M2M_SUCCESS) goto _EXIT;

	gpio = get_gpio_idx(u8GpioNum);
	if(gpio < 0) goto _EXIT1;

	if(op == GPIO_OP_DIR) {
		ret = set_gpio_dir((uint8)gpio, u8InVal);
	} else if(op == GPIO_OP_SET) {
		ret = set_gpio_val((uint8)gpio, u8InVal);
	} else if(op == GPIO_OP_GET) {
		ret = get_gpio_val((uint8)gpio, pu8OutVal);
	}
	if(ret != M2M_SUCCESS) goto _EXIT1;

_EXIT1:
	ret = hif_chip_sleep();
_EXIT:
	return ret;
}
/*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*
FUNCTION IMPLEMENTATION
*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*/


sint8 m2m_periph_init(tstrPerphInitParam * param)
{
	return M2M_SUCCESS;
}

sint8 m2m_periph_gpio_set_dir(uint8 u8GpioNum, uint8 u8GpioDir)
{
	return gpio_ioctl(GPIO_OP_DIR, u8GpioNum, u8GpioDir, NULL);
}

sint8 m2m_periph_gpio_set_val(uint8 u8GpioNum, uint8 u8GpioVal)
{
	return gpio_ioctl(GPIO_OP_SET, u8GpioNum, u8GpioVal, NULL);
}

sint8 m2m_periph_gpio_get_val(uint8 u8GpioNum, uint8 * pu8GpioVal)
{
	return gpio_ioctl(GPIO_OP_GET, u8GpioNum, 0, pu8GpioVal);
}

sint8 m2m_periph_gpio_pullup_ctrl(uint8 u8GpioNum, uint8 u8PullupEn)
{
	/* TBD */
	return M2M_SUCCESS;
}

sint8 m2m_periph_i2c_master_init(tstrI2cMasterInitParam * param)
{
	/* TBD */
	return M2M_SUCCESS;
}

sint8 m2m_periph_i2c_master_write(uint8 u8SlaveAddr, uint8 * pu8Buf, uint16 u16BufLen, uint8 flags)
{
	/* TBD */
	return M2M_SUCCESS;
}

sint8 m2m_periph_i2c_master_read(uint8 u8SlaveAddr, uint8 * pu8Buf, uint16 u16BufLen, uint16 * pu16ReadLen, uint8 flags)
{
	/* TBD */
	return M2M_SUCCESS;
}


sint8 m2m_periph_pullup_ctrl(uint32 pinmask, uint8 enable)
{
	return pullup_ctrl(pinmask, enable);
}
