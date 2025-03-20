/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 * last SDA
 * tehn SCL
 */

#include <stdio.h>
#include<zephyr/device.h>
#include<zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include"lcd_driver.h"
#include <stdint.h>
#include<stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>

#  define HUGE_VALF 1e10000f
#  define HUGE_VALL 1e10000L

uint8_t backlight_=1;
uint8_t custom=0;
uint8_t displayfunction;
uint8_t displaycontrol;
uint8_t displaymode;
static double pow(double x, double y)
{
	double result = 1;

	if (y < 0) {
		y = -y;
		while (y--) {
			result /= x;
		}
	} else {
		while (y--) {
			result *= x;
		}
	}

	return result;
}

void transmit_nibble(const struct device *dev,uint8_t slave_address,uint8_t nibble,uint8_t rs,uint8_t en){//nibble always will be in upper four bits
  	uint8_t msg_arr[1];
	msg_arr[0] = nibble|(backlight_<<3)|rs|(en<<2);
	struct i2c_msg msg_obj;
	msg_obj.buf = msg_arr;
	msg_obj.len = 1;
	msg_obj.flags = I2C_MSG_WRITE;
	i2c_transfer(dev,&msg_obj,1,slave_address);
}

void command(const struct device *dev,uint8_t slave_address,uint8_t data){
  uint8_t rs=0;
  transmit_nibble(dev,slave_address,(data)&(0xF0),rs,1);
  transmit_nibble(dev,slave_address,(data)&(0xF0),rs,0);
  transmit_nibble(dev,slave_address,(data)<<4,rs,1);
  transmit_nibble(dev,slave_address,(data)<<4,rs,0);
  //l_delay(1);
  }
  

void data(const struct device *dev,uint8_t slave_address,uint8_t data){
  uint8_t rs=1;
  transmit_nibble(dev,slave_address,(data)&(0xF0),rs,1);
  transmit_nibble(dev,slave_address,(data)&(0xF0),rs,0);
  transmit_nibble(dev,slave_address,(data)<<4,rs,1);
  transmit_nibble(dev,slave_address,(data)<<4,rs,0);
  //l_delay(1);
  }


void lcd_clear(const struct device *dev,uint8_t slave_address){
  command(dev,slave_address,LCD_CLEARDISPLAY);//increment fwd
  //l_delay(1);
}

void lcd_noDisplay(const struct device *dev,uint8_t slave_address){
  displaycontrol &= ~LCD_DISPLAYON;
  command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}
void lcd_display(const struct device *dev,uint8_t slave_address) {
  displaycontrol |= LCD_DISPLAYON;
  command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}

void lcd_noCursor(const struct device *dev,uint8_t slave_address) {
  displaycontrol &= ~LCD_CURSORON;
  command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}
void lcd_cursor(const struct device *dev,uint8_t slave_address) {
  displaycontrol |= LCD_CURSORON;
  command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}


void lcd_noBlink(const struct device *dev,uint8_t slave_address) {
  displaycontrol &= ~LCD_BLINKON;
  command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}
void lcd_blink(const struct device *dev,uint8_t slave_address) {
  displaycontrol |= LCD_BLINKON;
  command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}

// These commands scroll the display without changing the RAM
void lcd_scrollDisplayLeft(const struct device *dev,uint8_t slave_address) {
  command(dev,slave_address,LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void lcd_scrollDisplayRight(const struct device *dev,uint8_t slave_address){
  command(dev,slave_address,LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void lcd_leftToRight(const struct device *dev,uint8_t slave_address) {
  displaymode |= LCD_ENTRYLEFT;
  command(dev,slave_address,LCD_ENTRYMODESET | displaymode);
}

// This is for text that flows Right to Left
void lcd_rightToLeft(const struct device *dev,uint8_t slave_address) {
  displaymode &= ~LCD_ENTRYLEFT;
  command(dev,slave_address,LCD_ENTRYMODESET | displaymode);
}

// This will 'right justify' text from the cursor
void lcd_autoscroll(const struct device *dev,uint8_t slave_address) {
  displaymode |= LCD_ENTRYSHIFTINCREMENT;
  command(dev,slave_address,LCD_ENTRYMODESET | displaymode);
}

// This will 'left justify' text from the cursor
void lcd_noAutoscroll(const struct device *dev,uint8_t slave_address){
  displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
  command(dev,slave_address,LCD_ENTRYMODESET | displaymode);
}

void lcd_home(const struct device *dev,uint8_t slave_address)
{
  command(dev,slave_address,LCD_RETURNHOME);  // set cursor position to zero
}

void lcd_init(const struct device *dev,uint8_t slave_address,uint8_t lines,uint8_t dots){
  command(dev,slave_address,0x02);//4 bit interface
  if(lines==2) displayfunction|=LCD_2LINE;
  if(dots==1) displayfunction|=LCD_5x10DOTS;
  command(dev,slave_address,displayfunction|LCD_FUNCTIONSET);//16x2 5x7
  lcd_display(dev,slave_address);
  lcd_leftToRight(dev,slave_address);
  lcd_clear(dev,slave_address);
}

void lcd_setCursor(const struct device *dev,uint8_t slave_address,uint8_t col, uint8_t row)
{
if(row==0)
command(dev,slave_address,0x80+col);
else
command(dev,slave_address,0xC0+col);
}



/*
void lcd_createChar(const struct device *dev,uint8_t slave_address, uint8_t charmap[]) {
  command(dev,slave_address,(LCD_SETCGRAMADDR | ((custom) << 3)));
  for (int i=0; i<8; i++) {
    data(dev,slave_address,charmap[i]);
  }
  lcd_data(dev,slave_address,custom);
}
*/



void lcd_backlight(const struct device *dev,uint8_t slave_address){
backlight_=1;
command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}





void lcd_noBacklight(const struct device *dev,uint8_t slave_address){
backlight_=0;
command(dev,slave_address,LCD_DISPLAYCONTROL | displaycontrol);
}





void lcd_printnumber(const struct device *dev,uint8_t slave_address,uint32_t num){
uint32_t copy=num;
uint32_t digit=1;
while(copy){
copy/=10;
digit*=10;
}
digit/=10;
while(digit){
data(dev,slave_address,(((uint8_t)(num/digit))+48));
num%=digit;
digit/=10;
}
  }
  
  

void lcd_string(const struct device *dev,uint8_t slave_address,uint8_t *x)
{
  while(*x)
  data(dev,slave_address,*(x++));
}



void lcd_char(const struct device *dev,uint8_t slave_address,uint8_t x)
{
  data(dev,slave_address,x);
}


void lcd_backspace(const struct device *dev,uint8_t slave_address){

}

static inline void lcd_itoa (const struct device *dev,uint8_t slave_address,unsigned long long int number, unsigned base, long int total_number_of_digits)
{
    uint8_t variable;
	int i = 0;
	unsigned int intermediate = 0;
	unsigned int digits[sizeof(number)*8];

	while (1) {
		digits[i] = number % base;

		if (number < base){
			i++;
			break;
		}

		number /= base;
		i++;
	}
	if(total_number_of_digits != -1){
		for(long int prepend_index = 0; prepend_index < total_number_of_digits-i; prepend_index++){
			putchar(48);
            data(dev,slave_address,48);
		}
	}
	
	while (i-- > 0)
		{
			if (digits[i] >= 10)
				intermediate = 'a' - 10;
			else
				intermediate = '0';
            variable = digits[i] + intermediate;
			putchar(variable);
            data(dev,slave_address,variable);
		}
}

#define MAX_PRECISION	(10)
static const double rounders[MAX_PRECISION + 1] =
{
	0.5,				// 0
	0.05,				// 1
	0.005,				// 2
	0.0005,				// 3
	0.00005,			// 4
	0.000005,			// 5
	0.0000005,			// 6
	0.00000005,			// 7
	0.000000005,		// 8
	0.0000000005,		// 9
	0.00000000005		// 10
};

/**
 * @fn char * ftoa1(double f, char* buf, int precision)
 * @brief Converts a floating-point number to a string
 * @param number The floating-point number to be converted
 * @param result The buffer in which the converted string will be stored
 * @param afterpoint The number of digits to be printed after the decimal point
 * @return A pointer to the converted string
 */
char * lcd_ftoa1(double f, char* buf, int precision)
{
	char * ptr = buf;
	char * p = ptr;
	char * p1;
	char c;
	long intPart;

	// check precision bounds
	if (precision > MAX_PRECISION)
		precision = MAX_PRECISION;

	// sign stuff
	if (f < 0)
	{
		f = -f;
		*ptr++ = '-';
	}

	if (precision < 0)  // negative precision == automatic precision guess
	{
		if (f < 1.0) precision = 6;
		else if (f < 10.0) precision = 5;
		else if (f < 100.0) precision = 4;
		else if (f < 1000.0) precision = 3;
		else if (f < 10000.0) precision = 2;
		else if (f < 100000.0) precision = 1;
		else precision = 0;
	}

	// round value according the precision
	if (precision)
		f += rounders[precision];

	// integer part...
	intPart = f;
	f -= intPart;

	if (!intPart)
		*ptr++ = '0';
	else
	{
		// save start pointer
		p = ptr;

		// convert (reverse order)
		while (intPart)
		{
			*p++ = '0' + intPart % 10;
			intPart /= 10;
		}

		// save end pos
		p1 = p;

		// reverse result
		while (p > ptr)
		{
			c = *--p;
			*p = *ptr;
			*ptr++ = c;
		}

		// restore end pos
		ptr = p1;
	}

	// decimal part
	if (precision)
	{
		// place decimal point
		*ptr++ = '.';

		// convert
		while (precision--)
		{
			f *= 10.0;
			c = f;
			*ptr++ = '0' + c;
			f -= c;
		}
	}

	// terminating zero
	*ptr = 0;

	return buf;
}

/** @fn void lcd_printf_(const char *fmt, va_list ap)
 * @brief Handles the input stream of characters to print on screen
 * @details Identifies the type of format string, number of arguments and prints the right characer on screen
 * @param const char * fmt - formatting strings
 * @param const va_list ap - arg list
 */
void lcd_printf_(const struct device *dev,uint8_t slave_address,const char *fmt, va_list ap)
{
    uint8_t variable;
	register const char* p;
	const char* last_fmt;
	register int ch;
	unsigned long long num;
	int base, lflag, i;
	float float_num = 0;
	char float_arr[30] = {'\0'};
	int backtothebeginning;

	for (;;) {
		for (;(ch = *(unsigned char *) fmt) != '%'; fmt++) {
			if (ch == '\0')
				return;
			putchar(ch);
            data(dev,slave_address,ch);
		}
		fmt++;

		// Process a %-escape sequence
		last_fmt = fmt;
		lflag = 0;
		long int total_number_of_digits = -1;
		

		backtothebeginning = 0;
		for (;;) {

			switch (ch = *(unsigned char *) fmt++) {

				// long flag (doubled for long long)
				case 'l':{
					lflag++;
					backtothebeginning = 1;
        }
          break;

				// character
				case 'c':{
                    variable = va_arg(ap, int);
					putchar(variable);
                    data(dev,slave_address,variable);
        }
					break;

				// string
				case 's':{
					if ((p = va_arg(ap, char *)) == NULL)
						p = "(null)";
					for (; (ch = *p) != '\0' ;) {
						putchar(ch);
                        data(dev,slave_address,ch);
						p++;
          }
					}
					break;

				// (signed) decimal
				case 'd':{
					base = 10;

					if (lflag >= 2)
						num = va_arg(ap, long long);
					else if (lflag ==1)
						num = va_arg(ap, long);
					else
						num = va_arg(ap, int);

					if ((long long) num < 0) {
						putchar('-');
                        data(dev,slave_address,'-');
						num = -(long long) num;
					}

					lcd_itoa( dev,slave_address,num, base, total_number_of_digits);
                    total_number_of_digits = -1;
        }
					break;
				
				// float
				case 'f':{
					char *float_str;
					float value = va_arg(ap, double);

					lcd_ftoa1(value, float_str, 4);
					
					for(int i=0; float_str[i] !='\0'; i++)
					{ 
						putchar(float_str[i]);
                        data(dev,slave_address,float_str[i]);
					}
        }
					break;
				
				// (unsigned) decimal
				case 'u':{
					base = 10;

					if (lflag >= 2)
						num = va_arg(ap, unsigned long long);
					else if (lflag)
						num = va_arg(ap, unsigned long);
					else
						num = va_arg(ap, unsigned int);

					lcd_itoa( dev,slave_address,num, base, total_number_of_digits);
                    total_number_of_digits = -1;
        }
					break;

				// (unsigned) octal
				case 'o':{
					// should do something with padding so it's always 3 octets
					base = 8;

					if (lflag >= 2)
						num = va_arg(ap, unsigned long long);
					else if (lflag)
						num = va_arg(ap, unsigned long);
					else
						num = va_arg(ap, unsigned int);

					lcd_itoa(dev,slave_address, num, base, total_number_of_digits);
                    total_number_of_digits = -1;
        }
					break;

				// hexadecimal
				case 'x':
				{
        	base = 16;

					if (lflag >= 2)
						num = va_arg(ap, unsigned long long);
					else if (lflag)
						num = va_arg(ap, unsigned long);
					else
						num = va_arg(ap, unsigned int);

					lcd_itoa( dev,slave_address,num, base, total_number_of_digits);
                    total_number_of_digits = -1;
        }
					break;

				// escaped '%' character
				case '%':
				{
        	putchar(ch);
                    data(dev,slave_address,ch);
					break;

				// prepend zeros
				case '0':
					ch = *(unsigned char *) fmt++;
					total_number_of_digits = 0;
					int index = 0;
					while((ch>=48) && (ch<=57)){
						total_number_of_digits = total_number_of_digits + (ch-48) * pow(10, index);	
						ch = *(unsigned char *) fmt++;
						index += 1;
					}
					fmt--;
					backtothebeginning = 1;
        }
        	break;


				// unrecognized escape sequence - just print it literally
				default:
				{
        	putchar('%');
                    data(dev,slave_address,'%');
					fmt = last_fmt;
        }
      		break;
			}

			if (backtothebeginning)
			{
				backtothebeginning = 0;
				continue;
			}
			else
				break;
    }
	}
}

/** @fn int printf(const char* fmt, ...)
 * @brief function to print characters on file
 * @details prints the characters on terminal
 * @param const char*
 * @return int
 */
int lcd_printf(const struct device *dev,uint8_t slave_address,const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	lcd_printf_(dev,slave_address,fmt, ap);

	va_end(ap);
	return 0;// incorrect return value, but who cares, anyway?
}


//scanf.c functions


/***************************************************************************
* Project           		    :  Custom C Library
* Name of the file	     	    :  scan.c
* Brief Description of file     :  Implements a custom version of the scanf() function, including support for backspace.
* Name of Author    	        :  Kapil Shyam. M
* Email ID                      :  kapilshyamm@gmail.com

Copyright (C) 2023 Mindgrove Technologies Pvt Ltd. All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

***************************************************************************/


/**
 * @fn float strtof1(const char* str, char** endptr)
 * @brief Converts a string to a float
 * @param nptr A pointer to the string to be converted
 * @param endptr If not NULL, stores the address of the first invalid character in nptr
 * @return The converted float
 */
float lcd_strtof1(const char* str, char** endptr)
{
	float result = 0.0;
	int sign = 1;

	// Checking the integer whether +ve or -ve
	if(*str == '-')
	{
		sign = -1;
		str++;
	}
	else if (*str == '+')
	{
		str++;
	}

	// Calculate the integer part
	while(*str >= '0' && *str <= '9')
	{
		result = result*10 + (*str - '0');
		str++;
	}

	// Calculate the floating point part
	if (*str == '.')
	{
		str++;
		float base = 0.1;
		while(*str >= '0' && *str <= '9')
		{
			result += base*(*str - '0');
			base /= 10.0;
			str++;
		}
	}

        // Returning the value with sign
	*endptr = (char*)&str;

	return result*sign;
}

/**
 * @fn long strtol1(const char *nptr, char **endptr, int base)
 * @brief Converts a string to a long integer
 * @param nptr A pointer to the string to be converted
 * @param endptr If not NULL, stores the address of the first invalid character in nptr
 * @param base The base of the number represented in nptr
 * @return The converted long int
 */
long lcd_strtol1(const char *nptr, char **endptr, int base)
{
  long val = 0;
  int c, neg = 0;
  const char *p = nptr;

  /* skip leading white space */
  while (isspace((unsigned char)*p))
    p++;

  /* check for a sign */
  if (*p == '-' || *p == '+') {
    neg = (*p == '-');
    p++;
  }

  /* check for a base specifier */
  if (base == 0) {
    if (*p == '0') {
      if (*++p == 'x' || *p == 'X') {
        p++;
        base = 16;
      } else {
        base = 8;
      }
    } else {
      base = 10;
    }
  } else if (base == 16) {
    if (*p == '0' && (*++p == 'x' || *p == 'X'))
      p++;
  }

  /* convert the string to an integer */
  while ((c = *p) != '\0') {
    int digit;
    if (isdigit((unsigned char)c))
      digit = c - '0';
    else if (isalpha((unsigned char)c))
      digit = toupper((unsigned char)c) - 'A' + 10;
    else
      break;
    if (digit >= base)
      break;
    val = val * base + digit;
    p++;
  }

  /* store the end pointer */
  if (endptr != NULL)
    *endptr = (char *)p;

  /* return the result */
  return neg ? -val : val;
}

/**
 * @brief * @fn int _scan_(const char* format, va_list)
 * @brief Reads formatted input from the console
 * @param format A string that specifies the expected format of the input
 * @param ... Additional arguments, as required by the format string
 * @return The number of successfully read and stored items
 */

int lcd_scan_(const struct device *dev,uint8_t slave_address,const char * format, va_list vl)
{
    int i = 0, j = 0, ret = 0;
    char c;
    char *out_loc;
    i = 0;

    while (format && format[i])
    {
        if (format[i] == '%') 
        {
            i++;
            switch (format[i]) 
            {

                case 's': 
                {
                    c = getchar();
                    char* buff = va_arg(vl, char*);
                    int k = 0;
                    while (c != '\r')
                    {
                        if(c == '\b'){
                            if(k > 0){
                                k--;
                                putchar('\b');
                                putchar(' ');
                                putchar('\b');
                                putchar(' ');
                                lcd_backspace(dev,slave_address);
                                buff[k] = 0;
                            }
                        }
                        else
                        {
                          buff[k++] = c;
                        }
                        lcd_printf(dev,slave_address,"%c", c);
                        c = getchar();
                    }
                    buff[k]='\0';
                    ret++;
                    break;
                }
                case 'c': 
                {   
                    int k=0;
                    c = getchar();
                    lcd_printf(dev,slave_address,"%c", c);
                    if(c == '\b'){
                            if(k > 0){
                                k--;
                                putchar('\b');
                                putchar(' ');
                                putchar('\b');
                                putchar(' ');
                               lcd_backspace(dev,slave_address);
                            }
                        }
                    *(char *)va_arg(vl, char*) = c;
                    ret++;
                    break;
                }
                case 'd': 
                {
                    int* d = va_arg(vl, int*);
                    c = getchar();
                    char buff[100] = {0};
                    int k = 0;
                    int result=0;
                    int digit;
                    while ((c != '\r') && (c != ' '))
                    {
                        if(c == '\b'){
                            if(k > 0){
                                k--;
                                putchar('\b');
                                putchar(' ');
                                putchar('\b');
                                putchar(' ');
                               lcd_backspace(dev,slave_address);
                                buff[k] = 0;
                            }
                        }
                        else
                        {
                          buff[k++] = c;
                        }
                        lcd_printf(dev,slave_address,"%c", c);
                        // buff[k] = c;
                        c = getchar();
                    }
                    buff[k]='\0';
                    for(int i=0;i<k; i++)
                    {
                        if (buff[i] >= '0' && buff[i] <= '9')
                        {
                            digit = buff[i] - '0';
                            result = result*10 + digit;
                        }
                        else
                        {
                            break;
                        }
                    }
                    
                    *d = result;
                    if((errno == ERANGE && (*d == LONG_MAX || *d == LONG_MIN)) || (errno != 0 && *d == 0))
                    {
                        perror("strtol");
                        return EOF;
                    }
                    if(out_loc == buff)
                    {
                        fprintf(stderr, "Not a Number. No Digits Found!\n");
                        return EOF;
                    }
                    ret++;
                    break;
                 }
                 case 'f':
            		 {
                    float* f = (float *) va_arg(vl, double*);
                    errno = 0;
                    int decimal_point = -1;
                    c = getchar();
                    char buff[100] = {0};
                    int k = 0;
                    while ((c != '\r') && (c != ' '))
                    {
                        if (c == '\b') {
                            if (k > 0) {
                                if (decimal_point != -1 && k > decimal_point) {
                                    decimal_point--;
                                } else if (buff[k - 1] == '.') {
                                    decimal_point = -1;
                                }
                                k--;
                                putchar('\b');
                                putchar(' ');
                                putchar('\b');
                                putchar(' ');
                                buff[k] = 0;
                               lcd_backspace(dev,slave_address);
                            }
                        }
                        else {
                            if (c == '.') {
                                if (decimal_point == -1) {
                                    decimal_point = k;
                                } else {
                                    fprintf(stderr, "Invalid input: multiple decimal points\n");
                                    return EOF;
                                }
                            }
                            buff[k++] = c;
                        }
                        lcd_printf(dev,slave_address,"%c", c, k);
                        // buff[k] = c;
                        c = getchar();
                    }
                    buff[k]='\0';
                    *f = lcd_strtof1(buff, &out_loc);
                    if((errno == ERANGE && (*f == HUGE_VALF || *f == -HUGE_VALF)) || (errno != 0 && *f == 0))
                    {
                        perror("strtof");
                        return EOF;
                    }
                    if(out_loc == buff)
                    {
                        fprintf(stderr, "Not a Number. No Digits Found!\n");
                        return EOF;
                    }
                    ret++;
                    break;
                }
                 case 'x':
                 {
                    int* x = va_arg(vl, int*);
                    c = getchar();
                    char buff[100] = {0};
                    int k = 0;
                    while ((c != '\r') && (c != ' '))
                    {
                        if(c == '\b'){
                            if(k > 0){
                                k--;
                                putchar('\b');
                                putchar(' ');
                                putchar('\b');
                                putchar(' ');
                               lcd_backspace(dev,slave_address);
                                buff[k] = 0;
                            }
                        }
                        else
                        {
                          buff[k++] = c;
                        }
                        lcd_printf(dev,slave_address,"%c", c);
                        // buff[k] = c;
                        c = getchar();
                    }
                    buff[k]='\0';
                    *x = lcd_strtol1(buff, &out_loc, 16);
                    if((errno == ERANGE && (*x == LONG_MAX || *x == LONG_MIN)) || (errno != 0 && *x == 0))
                    {
                        perror("strtol");
                        return EOF;
                    }
                    if(out_loc == buff)
                    {
                        fprintf(stderr, "Not a Number. No Digits Found!\n");
                        return EOF;
                    }
                    ret++;
                    break;
                 }
                  case 'o':
                  {
                      int* o = va_arg(vl, int*);
                      c = getchar();
                      char buff[100] = {0};
                      int k = 0;
                      while ((c != '\r') && (c != ' '))
                      {
                        if(c == '\b'){
                            if(k > 0){
                                k--;
                                putchar('\b');
                                putchar(' ');
                                putchar('\b');
                                putchar(' ');
                                lcd_backspace(dev,slave_address);
                                buff[k] = 0;
                            }
                        }
                        else
                        {
                          buff[k++] = c;
                        }
                        lcd_printf(dev,slave_address,"%c", c);
                        // buff[k] = c;
                        c = getchar();
                      }
                        buff[k]='\0';
                      *o = lcd_strtol1(buff, &out_loc, 8);
                      if((errno == ERANGE && (*o == LONG_MAX || *o == LONG_MIN)) || (errno != 0 && *o == 0))
                      {
                          perror("strtol");
                          return EOF;
                      }
                      if(out_loc == buff)
                      {
                          fprintf(stderr, "Not a Number. No Digits Found!\n");
                          return EOF;
                      }
                      ret++;
                      break;
                  }
                  case 'u':
                  {
                      int* u = va_arg(vl, unsigned int*);
                      c = getchar();
                      char buff[100] = {0};
                      int k = 0;
                      while ((c != '\r') && (c != ' '))
                      {
                        if(c == '\b'){
                            if(k > 0){
                                k--;
                                putchar('\b');
                                putchar(' ');
                                putchar('\b');
                                putchar(' ');
                                buff[k] = 0;
                                lcd_backspace(dev,slave_address);
                            }
                        }
                        else
                        {
                          buff[k++] = c;
                        }
                        lcd_printf(dev,slave_address,"%c", c);
                        // buff[k] = c;
                        c = getchar();
                      }
                      buff[k]='\0';
                      *u = lcd_strtol1(buff, &out_loc, 10);
                      if((errno == ERANGE && (*u == LONG_MAX || *u == LONG_MIN)) || (errno != 0 && *u == 0))
                      {
                          perror("strtol");
                          return EOF;
                      }
                      if(out_loc == buff)
                      {
                          fprintf(stderr, "Not a Number. No Digits Found!\n");
                          return EOF;
                      }
                      ret++;
                      break;
                  }
            }
        } 
        else 
        {
            c = getchar();
            if (c != format[i])
            {
                ret = -1;
                break;
            }
        }
        i++;
    }
    return ret;
}

/**
* @fn int scan(const char* format, ...)
* @brief reads formatted input from the standard input stream
* @details the format parameter specifies the format of the input, consisting of conversion specifications and ordinary characters.
* @param const char* format - string that specifies the format of the input
* @return int - number of input items successfully read and assigned. If an error occurs, or if the end of the input is reached before the first conversion, EOF is returned.
**/
int lcd_scan(const struct device *dev,uint8_t slave_address,const char* format, ...)
{
	va_list vl;
    int done;
 	va_start( vl, format);
  lcd_cursor(dev,slave_address);
	done = lcd_scan_(dev,slave_address,format, vl);
  lcd_noCursor(dev,slave_address);
va_end(vl);
    return done;
}

void lcd_run(){
  const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
  printf("Hello world");
  lcd_init(dev,0x27,2,1);
  lcd_setCursor(dev,0x27,0,0);
  lcd_printf(dev,0x27,"Hello world");
while(1);
 }

K_THREAD_DEFINE(my_tid, 512,
                lcd_run, NULL, NULL, NULL,
                1, 0, 0);




