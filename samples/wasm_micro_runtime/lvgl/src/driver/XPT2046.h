/**
 * @file XPT2046.h
 *
 */

#ifndef XPT2046_H
#define XPT2046_H

#define USE_XPT2046 1
#ifndef LV_CONF_INCLUDE_SIMPLE
#define LV_CONF_INCLUDE_SIMPLE
#endif

#  define XPT2046_HOR_RES     320
#  define XPT2046_VER_RES     240
#  define XPT2046_X_MIN       200
#  define XPT2046_Y_MIN       200
#  define XPT2046_X_MAX       3800
#  define XPT2046_Y_MAX       3800
#  define XPT2046_AVG         4
#  define XPT2046_INV         0

#define CMD_X_READ  0b10010000
#define CMD_Y_READ  0b11010000

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#ifdef LV_CONF_INCLUDE_SIMPLE
//#include "lv_drv_conf.h"
#else
//#include "../../lv_drv_conf.h"
#endif

#if USE_XPT2046
#include <autoconf.h>
#include <stdint.h>
#include <stdbool.h>
//#include "lvgl/lv_hal/lv_hal_indev.h"
#include "device.h"
#include "gpio.h"
#if 1
enum {
    LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR
};
typedef uint8_t lv_indev_state_t;
typedef int16_t lv_coord_t;
typedef struct {
    lv_coord_t x;
    lv_coord_t y;
} lv_point_t;

typedef struct {
    union {
        lv_point_t point; /*For LV_INDEV_TYPE_POINTER the currently pressed point*/
        uint32_t key; /*For LV_INDEV_TYPE_KEYPAD the currently pressed key*/
        uint32_t btn; /*For LV_INDEV_TYPE_BUTTON the currently pressed button*/
        int16_t enc_diff; /*For LV_INDEV_TYPE_ENCODER number of steps since the previous read*/
    };
    void *user_data; /*'lv_indev_drv_t.priv' for this driver*/
    lv_indev_state_t state; /*LV_INDEV_STATE_REL or LV_INDEV_STATE_PR*/
} lv_indev_data_t;
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void xpt2046_init(void);
bool xpt2046_read(lv_indev_data_t * data);

/**********************
 *      MACROS
 **********************/

#endif /* USE_XPT2046 */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* XPT2046_H */
