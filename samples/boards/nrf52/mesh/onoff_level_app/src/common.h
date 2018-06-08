#ifndef _COMMON_H
#define _COMMON_H

#define  SETB(x, y)   (x |= (1<<y))     /* for o/p */
#define  CLRB(x, y)   (x &= (~(1<<y)))  /* for o/p */
#define  TGLB(x, y)   (x ^= (1<<y))     /* for o/p */
#define  CHECKB(x, y) (x &  (1<<y))     /* for i/p */

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
#include "device_composition.h"
#include "publisher.h"
//#include "process_message.h"

/* -------------------- COMPANY ID -------------------- */

#define CID_INTEL 0x0002

/* -------------------- GPIO -------------------- */

extern struct device *button_device[4];
extern struct device *led_device[4];

void gpio_init(void);

/* -------------------- NVS -------------------- */

#include <nvs/nvs.h>

#define NVS_LED_STATE_ID 1

/* hex for "MESH" */
#define STORAGE_MAGIC 0x4d455348

/* Multiple of FLASH_PAGE_SIZE */
#define NVS_SECTOR_SIZE 4096

/* At least 2 sectors */
#define NVS_SECTOR_COUNT 2

/* Start address of the filesystem in flash */
#define NVS_STORAGE_OFFSET (FLASH_AREA_STORAGE_OFFSET - 8192)

/* Largest item that can be stored */
#define NVS_MAX_ELEM_SIZE 256

extern struct nvs_fs fs;

/* -------------------- Others -------------------- */

#define LEVEL_0   -32768
#define LEVEL_25  -16384
#define LEVEL_50  0
#define LEVEL_75  16384
#define LEVEL_100 32767

struct light_state_t {

	uint8_t OnOff;
	int16_t power;
};

extern struct light_state_t light_state_current;

void update_light_state(void);
void nvs_light_state_save(void);

#endif
