/*
 * Copyright (c) 2025 Felipe Neves <ryukokki.felipe@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_kernel.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_hal.h>
#include <libs/vg_lite_driver/VGLiteKernel/vg_lite_hw.h>

/**
 * VG Lite Hal stub functions that are actually no operation for
 * Zephyr but they are called by the LVGL VG-Lite driver so we
 * need to at least define them. I moved to a separated file
 * in order to make the Zephyr HAL file clean as possible.
 *
 * TODO: Remove this file when the LVGL VG-Lite driver gets
 * a proper cleanup.
 */

const char * vg_lite_hal_Status2Name(vg_lite_error_t status)
{
    switch(status) {
        case VG_LITE_SUCCESS:
            return "VG_LITE_SUCCESS";
        case VG_LITE_INVALID_ARGUMENT:
            return "VG_LITE_INVALID_ARGUMENT";
        case VG_LITE_OUT_OF_MEMORY:
            return "VG_LITE_OUT_OF_MEMORY";
        case VG_LITE_NO_CONTEXT:
            return "VG_LITE_NO_CONTEXT";
        case VG_LITE_TIMEOUT:
            return "VG_LITE_TIMEOUT";
        case VG_LITE_OUT_OF_RESOURCES:
            return "VG_LITE_OUT_OF_RESOURCES";
        case VG_LITE_GENERIC_IO:
            return "VG_LITE_GENERIC_IO";
        case VG_LITE_NOT_SUPPORT:
            return "VG_LITE_NOT_SUPPORT";
        case VG_LITE_ALREADY_EXISTS:
            return "VG_LITE_ALREADY_EXISTS";
        case VG_LITE_NOT_ALIGNED:
            return "VG_LITE_NOT_ALIGNED";
        case VG_LITE_FLEXA_TIME_OUT:
            return "VG_LITE_FLEXA_TIME_OUT";
        case VG_LITE_FLEXA_HANDSHAKE_FAIL:
            return "VG_LITE_FLEXA_HANDSHAKE_FAIL";
        case VG_LITE_SYSTEM_CALL_FAIL:
            return "VG_LITE_SYSTEM_CALL_FAIL";
        default:
            return "nil";
    }
}

void vg_lite_hal_barrier(void)
{
}

void vg_lite_hal_initialize(void)
{
}

void vg_lite_hal_deinitialize(void)
{
}

void vg_lite_hal_print(char * format, ...)
{
}

void vg_lite_hal_trace(char * format, ...)
{
}

vg_lite_error_t vg_lite_hal_operation_cache(void * handle, vg_lite_cache_op_t cache_op)
{
    (void) handle;
    (void) cache_op;
    return VG_LITE_SUCCESS;
}

vg_lite_error_t vg_lite_hal_memory_export(int32_t * fd)
{
    return VG_LITE_SUCCESS;
}

void * vg_lite_hal_map(uint32_t flags, uint32_t bytes, void * logical,
                    uint32_t physical, int32_t dma_buf_fd, uint32_t * gpu)
{
    (void) flags;
    (void) bytes;
    (void) logical;
    (void) physical;
    (void) dma_buf_fd;
    (void) gpu;

    return (void *)0;
}

void vg_lite_hal_unmap(void * handle)
{
    (void) handle;
}
