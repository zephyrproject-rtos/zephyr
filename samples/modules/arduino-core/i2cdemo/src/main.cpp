/*
 * Copyright (c) 2025 John Doe
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <Arduino.h>
#include "Wire.h"

void setup()
{
	printf("\n\nSetup begins\n");
	Wire.begin();
	// initialize the LED pin as an output:

	// initialize the pushbutton pin as an input:
	// pinMode(buttonPin, INPUT);
	// pinMode(ledPin, OUTPUT);
	Wire.beginTransmission(0x53);
	Wire.write(0x2C);
	Wire.write(0x08);
	Wire.endTransmission();

	Wire.beginTransmission(0x53);
	Wire.write(0x31);
	Wire.write(0x08);
	Wire.endTransmission();

	Wire.beginTransmission(0x53);
	Wire.write(0x2D);
	Wire.write(0x08);
	Wire.endTransmission();
	printf("\n\nSetup COMPLETE\n\n\n");
}

void loop()
{
	Wire.beginTransmission(0x53);
Wire.write(0x32);
Wire.endTransmission();
// printf("\n\nrequesting from 53\n\n\n");
Wire.requestFrom(0x53, 1);
byte x0 = Wire.read();

Wire.beginTransmission(0x53);
Wire.write(0x33);
Wire.endTransmission();
Wire.requestFrom(0x53, 1);
byte x1 = Wire.read();
x1 = x1 & 0x03;

uint16_t x = (x1 << 8) + x0;
int16_t xf = x;
if(xf > 511)
{
xf = xf - 1024;
}
float xa = xf * 0.004;
printf("\n\nX = %f\n",static_cast<double>(xa));
// Serial.print("X = ");
// Serial.print(xa);
// Serial.print(" g");
// Serial.println();


Wire.beginTransmission(0x53);
Wire.write(0x34);
Wire.endTransmission();
Wire.requestFrom(0x53, 1);
byte y0 = Wire.read();

Wire.beginTransmission(0x53);
Wire.write(0x35);
Wire.endTransmission();
Wire.requestFrom(0x53, 1);
byte y1 = Wire.read();
y1 = y1 & 0x03;

uint16_t y = (y1 << 8) + y0;
int16_t yf = y;
if(yf > 511)
{
yf = yf - 1024;
}
float ya = yf * 0.004;
// printk("Y = %f\n",ya);
printf("Y = %f\n",static_cast<double>(ya));
// printf("\n\nYa = %f\n\n\n",ya);
// Serial.print("Y = ");
// Serial.print(ya);
// Serial.print(" g");
// Serial.println();

Wire.beginTransmission(0x53);
Wire.write(0x36);
Wire.endTransmission();
Wire.requestFrom(0x53, 1);
byte z0 = Wire.read();

Wire.beginTransmission(0x53);
Wire.write(0x37);
Wire.endTransmission();
Wire.requestFrom(0x53, 1);
byte z1 = Wire.read();
z1 = z1 & 0x03;

uint16_t z = (z1 << 8) + z0;
int16_t zf = z;
if(zf > 511)
{
zf = zf - 1024;
}
float za = zf * 0.004;
printf("Z = %f\n\n",static_cast<double>(za));
// Serial.print("Z = ");
// Serial.print(za);
// Serial.print(" g");
// Serial.println();
// Serial.println();
delay(500);

}
