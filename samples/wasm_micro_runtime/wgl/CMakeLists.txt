# Copyright (C) 2019 Intel Corporation.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.8.2)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(wamr-lvgl)

enable_language (ASM)

add_definitions (-DNVALGRIND
                 -D__ZEPHYR__
                 -DWASM_ENABLE_BASE_LIB
                 -DWASM_ENABLE_GUI=1
                 -Dattr_container_malloc=bh_malloc
                 -Dattr_container_free=bh_free)

set (WAMR_ROOT $ENV{ZEPHYR_BASE}/../modules/lib/wasm-micro-runtime)
set (IWASM_ROOT ${WAMR_ROOT}/core/iwasm)
set (SHARED_LIB_ROOT ${WAMR_ROOT}/core/shared-lib)
set (APP_MGR_ROOT ${WAMR_ROOT}/core/app-mgr)
set (LVGL_ROOT $ENV{ZEPHYR_BASE}/../modules/lib/gui/lvgl)

include_directories (${IWASM_ROOT}/runtime/include
                     ${IWASM_ROOT}/runtime/platform/zephyr
                     ${IWASM_ROOT}/runtime/platform/include
                     ${IWASM_ROOT}/runtime/utils
                     ${IWASM_ROOT}/runtime/vmcore-wasm
                     ${IWASM_ROOT}/lib/native/base
                     ${IWASM_ROOT}/lib/native/libc
                     ${IWASM_ROOT}/lib/native/extension/sensor
                     ${IWASM_ROOT}/lib/native/extension/connection
                     ${IWASM_ROOT}/lib/native/extension/gui
                     ${IWASM_ROOT}/lib/native-interface
                     ${APP_MGR_ROOT}/app-manager
                     ${APP_MGR_ROOT}/app-mgr-shared
                     ${SHARED_LIB_ROOT}/include
                     ${SHARED_LIB_ROOT}/platform/include
                     ${SHARED_LIB_ROOT}/platform/zephyr
                     ${SHARED_LIB_ROOT}/mem-alloc/ems
                     ${SHARED_LIB_ROOT}/utils
                     ${SHARED_LIB_ROOT}/coap/er-coap
                     ${SHARED_LIB_ROOT}/coap/extension
                     ${IWASM_ROOT}/lib/3rdparty
                     src
                     src/driver
                     src/driver/tmp/tmp/
                     ${LVGL_ROOT}
                     ${LVGL_ROOT}/src
                     ${LVGL_ROOT}/..
                    )

file (GLOB_RECURSE GUI_SRCS
      ${IWASM_ROOT}/lib/native/extension/gui/*.c
      ${LVGL_ROOT}/*.c)

set (IWASM_SRCS
     ${IWASM_ROOT}/runtime/platform/zephyr/wasm_native.c
     ${IWASM_ROOT}/runtime/utils/wasm_dlfcn.c
     ${IWASM_ROOT}/runtime/utils/wasm_hashmap.c
     ${IWASM_ROOT}/runtime/utils/wasm_log.c
     ${IWASM_ROOT}/runtime/utils/wasm_vector.c
     ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_application.c
     ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_interp.c
     ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_loader.c
     ${IWASM_ROOT}/runtime/vmcore-wasm/wasm_runtime.c
     ${IWASM_ROOT}/runtime/vmcore-wasm/invokeNative_general.c
     ${IWASM_ROOT}/lib/native/base/base_lib_export.c
     ${IWASM_ROOT}/lib/native/base/request_response.c
     ${IWASM_ROOT}/lib/native/base/timer_wrapper.c
     ${IWASM_ROOT}/lib/native/libc/libc_wrapper.c
     ${IWASM_ROOT}/lib/native/extension/sensor/runtime_sensor.c
     ${IWASM_ROOT}/lib/native/extension/connection/connection_wrapper.c
     ${IWASM_ROOT}/lib/native/extension/connection/zephyr/connection_lib_impl.c
     ${IWASM_ROOT}/lib/native-interface/attr_container.c
     ${IWASM_ROOT}/lib/native-interface/restful_utils.c
     ${APP_MGR_ROOT}/app-manager/app_manager.c
     ${APP_MGR_ROOT}/app-manager/app_manager_host.c
     ${APP_MGR_ROOT}/app-manager/ble_msg.c
     ${APP_MGR_ROOT}/app-manager/event.c
     ${APP_MGR_ROOT}/app-manager/message.c
     ${APP_MGR_ROOT}/app-manager/module_jeff.c
     ${APP_MGR_ROOT}/app-manager/module_utils.c
     ${APP_MGR_ROOT}/app-manager/module_wasm_app.c
     ${APP_MGR_ROOT}/app-manager/module_wasm_lib.c
     ${APP_MGR_ROOT}/app-manager/resource_reg.c
     ${APP_MGR_ROOT}/app-manager/watchdog.c
     ${APP_MGR_ROOT}/app-manager/platform/zephyr/app_mgr_zephyr.c
     ${SHARED_LIB_ROOT}/platform/zephyr/bh_assert.c
     ${SHARED_LIB_ROOT}/platform/zephyr/bh_definition.c
     ${SHARED_LIB_ROOT}/platform/zephyr/bh_platform.c
     ${SHARED_LIB_ROOT}/platform/zephyr/bh_platform_log.c
     ${SHARED_LIB_ROOT}/platform/zephyr/bh_thread.c
     ${SHARED_LIB_ROOT}/platform/zephyr/bh_time.c
     ${SHARED_LIB_ROOT}/platform/zephyr/bh_math.c
     ${SHARED_LIB_ROOT}/mem-alloc/bh_memory.c
     ${SHARED_LIB_ROOT}/mem-alloc/mem_alloc.c
     ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_alloc.c
     ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_hmu.c
     ${SHARED_LIB_ROOT}/mem-alloc/ems/ems_kfc.c
     ${SHARED_LIB_ROOT}/utils/bh_list.c
     ${SHARED_LIB_ROOT}/utils/bh_log.c
     ${SHARED_LIB_ROOT}/utils/bh_queue.c
     ${SHARED_LIB_ROOT}/utils/runtime_timer.c
     ${SHARED_LIB_ROOT}/coap/er-coap/er-coap.c
     ${SHARED_LIB_ROOT}/coap/extension/coap_conversion.c
     ${SHARED_LIB_ROOT}/coap/extension/coap_over_tcp.c
    )

set (LVGL_DRV_SRCS
     src/driver/display_ili9340_adafruit_1480.c
     src/driver/display_ili9340.c
     src/driver/XPT2046.c
    )

target_sources(app PRIVATE
               ${IWASM_SRCS}  
               ${LVGL_DRV_SRCS}
               ${GUI_SRCS}
               src/main.c
               src/iwasm_main.c
               src/ext_lib_export.c)

