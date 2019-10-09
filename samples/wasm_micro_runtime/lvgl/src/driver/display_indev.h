#ifndef DISPLAY_INDEV_H_
#define DISPLAY_INDEV_H_
#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include "bh_platform.h"
#include "wasm_export.h"

#define USE_MOUSE 1
typedef union {
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t alpha;
    };
    uint32_t full;
} lv_color32_t;

typedef lv_color32_t lv_color_t;
typedef uint8_t lv_indev_state_t;
typedef int16_t lv_coord_t;
typedef uint8_t lv_opa_t;
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

enum {
    LV_INDEV_STATE_REL = 0, LV_INDEV_STATE_PR
};
enum {
    LV_OPA_TRANSP = 0,
    LV_OPA_0 = 0,
    LV_OPA_10 = 25,
    LV_OPA_20 = 51,
    LV_OPA_30 = 76,
    LV_OPA_40 = 102,
    LV_OPA_50 = 127,
    LV_OPA_60 = 153,
    LV_OPA_70 = 178,
    LV_OPA_80 = 204,
    LV_OPA_90 = 229,
    LV_OPA_100 = 255,
    LV_OPA_COVER = 255,
};

extern void xpt2046_init(void);

extern bool touchscreen_read(lv_indev_data_t * data);

extern bool mouse_read(lv_indev_data_t * data);

extern void display_init(wasm_module_inst_t module_inst);

extern void display_deinit(wasm_module_inst_t module_inst);

extern int time_get_ms(wasm_module_inst_t module_inst);

extern void display_flush(wasm_module_inst_t module_inst,
                          int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                          int32 color_p_offset);

extern void display_fill(wasm_module_inst_t module_inst,
                         int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                         lv_color_t color_p);

extern void display_map(wasm_module_inst_t module_inst,
                        int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                        const lv_color_t * color_p);

extern bool display_input_read(wasm_module_inst_t module_inst,
                               int32 data_offset);

void display_vdb_write(wasm_module_inst_t module_inst,
                       int32 buf_offset, lv_coord_t buf_w, lv_coord_t x,
                       lv_coord_t y, int32 color_p_offset, lv_opa_t opa);

#endif

