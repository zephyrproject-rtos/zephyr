#include "lib_export.h"
#include "sensor_api.h"
#include "connection_api.h"
#include "display_indev.h"

static NativeSymbol extended_native_symbol_defs[] = {
#include "runtime_sensor.inl"
#include "connection.inl"
        EXPORT_WASM_API(display_init),
        EXPORT_WASM_API(display_input_read),
        EXPORT_WASM_API(display_flush),
        EXPORT_WASM_API(display_fill),
        EXPORT_WASM_API(display_vdb_write),
        EXPORT_WASM_API(display_map),
        EXPORT_WASM_API(time_get_ms)
};

#include "ext_lib_export.h"
