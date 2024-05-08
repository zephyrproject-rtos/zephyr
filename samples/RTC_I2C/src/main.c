/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
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



void main(){
  const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
  uint8_t hr,min,secs;
      DS3231_setSeconds(dev,DS3231_ADDR,56);
      DS3231_setHours(dev,DS3231_ADDR,18);
      DS3231_setMinutes(dev,DS3231_ADDR,30);
	  DS3231_setHourMode(dev,DS3231_ADDR,CLOCK_H12);
    while(1){
        hr=DS3231_getHours(dev,DS3231_ADDR);
     	min=DS3231_getMinutes(dev,DS3231_ADDR);
      	secs=DS3231_getSeconds(dev,DS3231_ADDR);
       	printk("\n %d:%d:%d PM",hr,min,secs); 
		delayms(1000);
 	}
 }

// K_THREAD_DEFINE(my_tid, 512,
//                 rtc_run, NULL, NULL, NULL,
//                 1, 0, 0);
