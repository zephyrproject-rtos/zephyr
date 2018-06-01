#ifndef _COMMON_H
#define _COMMON_H

#define  SETB(x,y)   (x|=(1<<y))     //for o/p
#define  CLRB(x,y)   (x&=(~(1<<y)))  //for o/p
#define  TGLB(x,y)   (x^=(1<<y))     //for o/p
#define  CHECKB(x,y) (x&(1<<y))      //for i/p

#include <misc/printk.h>
#include <settings/settings.h>

#include <misc/byteorder.h>
#include <nrf.h>
#include <device.h>
#include <gpio.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/l2cap.h>
#include <bluetooth/hci.h>
#include <bluetooth/mesh.h>
#include <stdio.h>


#include <board.h>

#include "nrf52840.h"
#include "board.h"

#include "ble_mesh.h"
#include "DEVICE_composition.h"
#include "publisher.h"
#include "doer.h"

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

#define CID_INTEL 0x0002

//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

extern struct device *button_device[4];
extern struct device *led_device[4];

void gpio_init(void);

#endif
