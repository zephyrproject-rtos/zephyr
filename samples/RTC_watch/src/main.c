/*
 * Copyright (c) 2018 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>

#include "../generated/gui_guider.h"
#include "../generated/events_init.h"
#include <zephyr/drivers/i2c.h>
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);

#define INCLUDE_RTC_LIB

#ifdef INCLUDE_RTC_LIB
#include"RTC.h"
#define DELAY_FREQ_BASE 40000000
uint8_t bitSet(uint8_t data,uint8_t shift){
	return (data|(1<<shift));
}
uint8_t bitClear(uint8_t data,uint8_t shift){
	return (data&(~(1<<shift)));
}
uint8_t bitRead(uint8_t data,uint8_t shift){
	return ((data&(1<<shift))>>shift);
}

uint8_t bitWrite(uint8_t data,uint8_t shift,uint8_t bit){
	return ( (data & ~(1<<shift)) | (bit<<shift) );
}

static void delayms(long delay)
{
  for(int i=0;i<(3334*delay*(DELAY_FREQ_BASE/40000000));i++){
    __asm__ volatile("NOP");
  }
}

void DS3231_setHourMode(const struct device *dev,uint8_t slave_address,uint8_t h_mode)
{
	uint8_t data;

	if(h_mode == CLOCK_H12 || h_mode == CLOCK_H24)
	{
    uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x02;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		data = msg_arr[0];
		data=bitWrite(data, 6, h_mode);
    msg_obj.len = 2;
	  msg_arr[0] = 0x02;
    msg_arr[1] = data;
	  msg_obj.flags = I2C_MSG_WRITE;
    i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

uint8_t DS3231_getHourMode(const struct device *dev,uint8_t slave_address)
{
	bool h_mode;
	uint8_t data;

    uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x02;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		data = msg_arr[0];
	  h_mode = bitRead(data, 6);

	return (h_mode);

}

/*-----------------------------------------------------------
setMeridiem(uint8_t meridiem)

-----------------------------------------------------------*/

void DS3231_setMeridiem(const struct device *dev,uint8_t slave_address,uint8_t meridiem)
{
	uint8_t data;
	if(meridiem == HOUR_AM || meridiem == HOUR_PM)
	{
		if (DS3231_getHourMode(dev,slave_address) == CLOCK_H12)
		{
    uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x02;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
    data = msg_arr[0];
		data=bitWrite(data, 5, meridiem);
    msg_obj.len = 2;
	  msg_arr[0] = 0x02;
    msg_arr[1] = data;
	  msg_obj.flags = I2C_MSG_WRITE;
    i2c_transfer(dev,&msg_obj,1,slave_address);
		}
	}
}

uint8_t DS3231_getMeridiem(const struct device *dev,uint8_t slave_address)
{
	bool flag;
	uint8_t data;
	if (DS3231_getHourMode(dev,slave_address) == CLOCK_H12)
	{
    uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x02;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		data = msg_arr[0];
		flag = bitRead(data, 5);
		return (flag);
	}
	else
		return (HOUR_24);
}

uint8_t DS3231_getSeconds(const struct device *dev,uint8_t slave_address)
{
	uint8_t seconds;
    uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x00;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		seconds = (msg_obj.buf)[0];
    return (bcd2bin(seconds));
}

void DS3231_setSeconds(const struct device *dev,uint8_t slave_address,uint8_t seconds)
{
	if (seconds >= 00 && seconds <= 59)
	{
    uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 2;
	  msg_arr[0] = 0x00;
    msg_arr[1] = bin2bcd(seconds);
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

/*-----------------------------------------------------------
getMinutes
-----------------------------------------------------------*/
uint8_t DS3231_getMinutes(const struct device *dev,uint8_t slave_address)
{
	uint8_t minutes;
uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x01;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		minutes = msg_arr[0];
	return (bcd2bin(minutes));
}

void DS3231_setMinutes(const struct device *dev,uint8_t slave_address,uint8_t minutes)
{
	if (minutes >= 00 && minutes <= 59)
	{
    uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 2;
	  msg_arr[0] = 0x01;
    msg_arr[1] = bin2bcd(minutes);
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

/*-----------------------------------------------------------
getHour
-----------------------------------------------------------*/
uint8_t DS3231_getHours(const struct device *dev,uint8_t slave_address)
{
	uint8_t hours;
	bool h_mode;
	h_mode = DS3231_getHourMode(dev,slave_address);

    uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x02;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		hours = msg_arr[0];
    if (h_mode == CLOCK_H24)
	{
		return (bcd2bin(hours));
	}
	else if (h_mode == CLOCK_H12)
	{
		hours=bitClear(hours, 5);
		hours=bitClear(hours, 6);
		return (bcd2bin(hours));
	}
}

void  DS3231_setHours(const struct device *dev,uint8_t slave_address,uint8_t hours)
{
	bool h_mode;
	if (hours >= 00 && hours <= 23)
	{
		h_mode = DS3231_getHourMode(dev,slave_address);
		if (h_mode == CLOCK_H24)
		{
			hours = bin2bcd(hours);
		}
		else if (h_mode == CLOCK_H12)
		{
			if (hours > 12)
			{
				hours = hours % 12;
				hours = bin2bcd(hours);
				hours=bitSet(hours, 6);
				hours=bitSet(hours, 5);
			}
			else
			{
				hours = bin2bcd(hours);
				hours=bitSet(hours, 6);
				hours=bitClear(hours, 5);
			}
		}
		printf("Hr set as:%#x\n",hours);
      uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 2;
	  msg_arr[0] = 0x02;
      msg_arr[1] = hours;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

/*-----------------------------------------------------------
getWeek
-----------------------------------------------------------*/
uint8_t DS3231_getWeek(const struct device *dev,uint8_t slave_address)
{
	uint8_t week;
uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x03;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		week = msg_arr[0];
	return week;
}

void DS3231_setWeek(const struct device *dev,uint8_t slave_address,uint8_t week)
{
	if (week >= 1 && week <= 7)
	{
    uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 2;
	  msg_arr[0] = 0x03;
    msg_arr[1] = week;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

void DS3231_updateWeek(const struct device *dev,uint8_t slave_address)
{
	uint16_t y;
	uint8_t m, d, weekday;
	uint8_t msg_arr[2];
	struct i2c_msg msg_obj;
	y=DS3231_getYear(dev,slave_address);
	m=DS3231_getMonth(dev,slave_address);
	d=DS3231_getDay(dev,slave_address);
	
	weekday  = (d += m < 3 ? y-- : y - 2, 23*m/9 + d + 4 + y/4- y/100 + y/400)%7;
	
	if (weekday >= 1 && weekday <= 7)
	{
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 2;
	  msg_arr[0] = 0x03;
    msg_arr[1] = weekday;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

/*-----------------------------------------------------------
getDay
-----------------------------------------------------------*/
uint8_t DS3231_getDay(const struct device *dev,uint8_t slave_address)
{
	uint8_t day;
uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x04;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		day = msg_arr[0];
	return (bcd2bin(day));
}

void DS3231_setDay(const struct device *dev,uint8_t slave_address,uint8_t day)
{
	if (day >= 1 && day <= 31)
	{
    uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 2;
	  msg_arr[0] = 0x04;
    msg_arr[1] = bin2bcd(day);
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

/*-----------------------------------------------------------
getMonth ()

-----------------------------------------------------------*/
uint8_t DS3231_getMonth(const struct device *dev,uint8_t slave_address)
{
	uint8_t month;

uint8_t msg_arr[1];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x04;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
		i2c_transfer(dev,&msg_obj,1,slave_address);
		month = msg_arr[0];
	month=bitClear(month,7);		//Clear Century Bit;
	return (bcd2bin(month));
}
/*-----------------------------------------------------------
setMonth()
-----------------------------------------------------------*/

void DS3231_setMonth(const struct device *dev,uint8_t slave_address,uint8_t month)
{
	uint8_t data, century_bit;
	if (month >= 1 && month <= 12)
	{
    uint8_t msg_arr[2];
	  struct i2c_msg msg_obj;
	  msg_obj.buf = msg_arr;
	  msg_obj.len = 1;
	  msg_arr[0] = 0x05;
	  msg_obj.flags = I2C_MSG_WRITE;
	  i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	data = msg_arr[0];
	//Read Century bit and return it safe
	century_bit = bitRead(data, 7);
	month = bin2bcd(month);
	data=bitWrite(month,7,century_bit);
	msg_obj.len = 2;
	msg_arr[0] = 0x05;
    msg_arr[1] = month;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

/*-----------------------------------------------------------
getYear (Completed)

Century Bit
1 = 1900s
0 = 2000s
-----------------------------------------------------------*/
uint16_t DS3231_getYear(const struct device *dev,uint8_t slave_address)
{
	uint8_t century_bit,data;
	uint16_t century,year;
	uint8_t msg_arr[1];
	struct i2c_msg msg_obj;
	msg_obj.buf = msg_arr;
	msg_obj.len = 1;
	msg_arr[0] = 0x05;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	data = msg_arr[0];
	century_bit = bitRead(data, 7);
	if(century_bit == 1)
	{
		century = 1900;
	}
	else if(century_bit == 0)
	{
		century = 2000;
	}
	msg_arr[0] = 0x06;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	year = msg_arr[0];
	year = bcd2bin(year) + century;
	return (year);
}

void DS3231_setYear(const struct device *dev,uint8_t slave_address,uint16_t year)
{
	uint8_t century,data;

	if((year >= 00 && year <= 99) || (year >= 1900 && year <= 2099))
	{
		// If year is 2 digits set to 2000s.
		if(year >= 00 && year <= 99)
			year = year + 2000;

		//Century Calculation
		if(year >= 1900 && year <= 1999)
			century = 1;
		else if (year >= 2000 && year <= 2099)
			century = 0;

		// Find Century 
		year = year % 100;		//Converting to 2 Digit
		
		// Read Century bit from Month Register(0x05)
	uint8_t msg_arr[2];
	struct i2c_msg msg_obj;
	msg_obj.buf = msg_arr;
	msg_obj.len = 1;
	msg_arr[0] = 0x05;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
    msg_obj.flags = I2C_MSG_READ;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	data = msg_arr[0];
		
		// Set century bit to 1 for year > 2000;
		if(century == 1)
			data=bitSet(data,7);
		else
			data=bitClear(data,7);

		// Write Century bit to Month Register(0x05)
	msg_obj.len = 2;
	msg_arr[0] = 0x05;
    msg_arr[1] = data;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	msg_arr[0] = 0x06;
    msg_arr[1] = bin2bcd(year);
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}


void DS3231_setTime(const struct device *dev,uint8_t slave_address,uint8_t hours, uint8_t minutes, uint8_t seconds)
{
	if (hours >= 00 && hours <= 23 && minutes >= 00 && minutes <= 59 && seconds >= 00 && seconds <= 59)
	{
		bool h_mode;
		h_mode = DS3231_getHourMode(dev,slave_address);
		if (h_mode == CLOCK_H24)
		{
			hours = bin2bcd(hours);	// 0x02
		}
		else if (h_mode == CLOCK_H12)
		{
			if (hours > 12)
			{
				hours = hours % 12;
				hours = bin2bcd(hours);
				hours=bitSet(hours, 6);
				hours=bitSet(hours, 5);
			}
			else
			{
				hours = bin2bcd(hours);
				hours=bitSet(hours, 6);
				hours=bitClear(hours, 5);
			}
		}
		uint8_t msg_arr[4];
		struct i2c_msg msg_obj;
		msg_obj.buf = msg_arr;
		msg_obj.len = 4;
		msg_arr[0] = 0x00;
		msg_arr[1] = bin2bcd(seconds);
		msg_arr[2] = bin2bcd(minutes);
		msg_arr[3] = hours;
		msg_obj.flags = I2C_MSG_WRITE;
		i2c_transfer(dev,&msg_obj,1,slave_address);
	}
}

void DS3231_setDate(const struct device *dev,uint8_t slave_address,uint8_t day, uint8_t month, uint16_t year)
{
	year = year % 100; //Convert year to 2 Digit

		uint8_t msg_arr[4];
		struct i2c_msg msg_obj;
		msg_obj.buf = msg_arr;
		msg_obj.len = 4;
		msg_arr[0] = 0x04;
		msg_arr[1] = bin2bcd(day);
		msg_arr[2] = bin2bcd(month);
		msg_arr[3] = bin2bcd(year);
		msg_obj.flags = I2C_MSG_WRITE;
		i2c_transfer(dev,&msg_obj,1,slave_address);
}

void DS3231_setDateTime(const struct device *dev,uint8_t slave_address,char* date, char* time)
{
	uint8_t day, month, hour, minute, second;
	uint16_t year;
	// sample input: date = "Dec 26 2009", time = "12:34:56"
	year = atoi(date + 9);
	DS3231_setYear(dev,slave_address,year);
	// Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
	switch (date[0]) {
		case 'J': month = (date[1] == 'a') ? 1 : ((date[2] == 'n') ? 6 : 7); break;
		case 'F': month = 2; break;
		case 'A': month = date[2] == 'r' ? 4 : 8; break;
		case 'M': month = date[2] == 'r' ? 3 : 5; break;
		case 'S': month = 9; break;
		case 'O': month = 10; break;
		case 'N': month = 11; break;
		case 'D': month = 12; break;
		}
	DS3231_setMonth(dev,slave_address,month);
	day = atoi(date + 4);
	DS3231_setDay(dev,slave_address,day);
	hour = atoi(time);
	DS3231_setHours(dev,slave_address,hour);
	minute = atoi(time + 3);
	DS3231_setMinutes(dev,slave_address,minute);
	second = atoi(time + 6);
	DS3231_setSeconds(dev,slave_address,second);
}
#endif







lv_ui guider_ui;

// int main(void)
// {
// 	const struct device *display_dev;
// 	uint8_t hr,min;
// 	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
//     const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
//     //   DS3231_setSeconds(dev,DS3231_ADDR,00);
//     //   DS3231_setHours(dev,DS3231_ADDR,11);
//     //   DS3231_setMinutes(dev,DS3231_ADDR,45);
// 	//   DS3231_setHourMode(dev,DS3231_ADDR,CLOCK_H12);
// 	if (!device_is_ready(display_dev)) {
// 		LOG_ERR("Device not ready, aborting test");
// 		return 0;
// 	}

// 	printk("Lvgl has started\n");
// 	//setup_ui(&guider_ui,hr,min);
//    	// events_init(&guider_ui);

// 	// lv_task_handler();
// 	display_blanking_off(display_dev);
// 	while (1) {
// 		hr=DS3231_getHours(dev,DS3231_ADDR);
//      	min=DS3231_getMinutes(dev,DS3231_ADDR);
// 		setup_ui(&guider_ui,hr,min);
// 		lv_task_handler();
// 		printk("Lvgl is running\n");
// 		// k_sleep(K_MSEC(10));
// 	}
// }
uint8_t hr,min;
void main_task_handler(void)
{
	const struct device *display_dev;

	display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
 
    //   DS3231_setSeconds(dev,DS3231_ADDR,00);
    //   DS3231_setHours(dev,DS3231_ADDR,11);
    //   DS3231_setMinutes(dev,DS3231_ADDR,45);
	//   DS3231_setHourMode(dev,DS3231_ADDR,CLOCK_H12);
	if (!device_is_ready(display_dev)) {
		LOG_ERR("Device not ready, aborting test");
		return 0;
	}

	printk("Lvgl has started\n");
	//setup_ui(&guider_ui,hr,min);
   	// events_init(&guider_ui);

	// lv_task_handler();
	display_blanking_off(display_dev);
	while (1) {
		setup_ui(&guider_ui,hr,min);
		lv_task_handler();
		printk("Lvgl is running\n");
		// k_sleep(K_MSEC(10));
		k_yield();
	}
}
void rtc_task_handler(void){
   const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
   while(1){
		hr=DS3231_getHours(dev,DS3231_ADDR);
     	min=DS3231_getMinutes(dev,DS3231_ADDR);
		k_yield();
   }
}


#define MY_STACK_SIZE 2048
#define MY_PRIORITY 1
K_THREAD_DEFINE(display_task, MY_STACK_SIZE,
                main_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
K_THREAD_DEFINE(rtc_task, MY_STACK_SIZE,
                rtc_task_handler, NULL, NULL, NULL,
                MY_PRIORITY, 0, 0);
