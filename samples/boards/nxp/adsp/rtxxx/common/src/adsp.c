#include "adsp.h"

#include <string.h>
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/drivers/misc/nxp_rtxxx_adsp_ctrl/nxp_rtxxx_adsp_ctrl.h>

static const struct device *adsp = DEVICE_DT_GET(DT_NODELABEL(adsp));

/* ADSP binary image symbols - exported by adspimgs.S */
extern uint32_t adsp_img_reset_start[];
extern uint32_t adsp_img_reset_size;

extern uint32_t adsp_img_text_start[];
extern uint32_t adsp_img_text_size;

extern uint32_t adsp_img_data_start[];
extern uint32_t adsp_img_data_size;

void dspStart(void)
{
    if(!device_is_ready(adsp))
        return;

    nxp_rtxxx_adsp_ctrl_load_section(adsp, adsp_img_reset_start, adsp_img_reset_size, NXP_RTXXX_ADSP_CTRL_SECTION_RESET);
    nxp_rtxxx_adsp_ctrl_load_section(adsp, adsp_img_text_start, adsp_img_text_size, NXP_RTXXX_ADSP_CTRL_SECTION_TEXT);
    nxp_rtxxx_adsp_ctrl_load_section(adsp, adsp_img_data_start, adsp_img_data_size, NXP_RTXXX_ADSP_CTRL_SECTION_DATA);

    nxp_rtxxx_adsp_ctrl_enable(adsp);
}