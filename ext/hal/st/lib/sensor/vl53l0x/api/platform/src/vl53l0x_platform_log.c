/*
 * COPYRIGHT (C) STMicroelectronics 2015. All rights reserved.
 *
 * This software is the confidential and proprietary information of
 * STMicroelectronics ("Confidential Information").  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with STMicroelectronics
 *
 * Programming Golden Rule: Keep it Simple!
 *
 */

/*!
 * \file   VL53L0X_platform_log.c
 * \brief  Code function defintions for Ewok Platform Layer
 *
 */


#include <windows.h>
#include <stdio.h>    // sprintf(), vsnprintf(), printf()

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_def.h"
#include "vl53l0x_platform_log.h"

#define trace_print(level, ...) trace_print_module_function(TRACE_MODULE_PLATFORM, level, TRACE_FUNCTION_NONE, ##__VA_ARGS__)
#define trace_i2c(...) trace_print_module_function(TRACE_MODULE_NONE, TRACE_LEVEL_NONE, TRACE_FUNCTION_I2C, ##__VA_ARGS__)

char  debug_string[VL53L0X_MAX_STRING_LENGTH_PLT];


char * _trace_filename = NULL;
FILE *_tracefile = NULL;

uint32_t _trace_level = TRACE_LEVEL_WARNING;
uint32_t _trace_modules = TRACE_MODULE_NONE;
uint32_t _trace_functions = TRACE_FUNCTION_NONE;


int32_t VL53L0X_trace_config(char *filename, uint32_t modules, uint32_t level, uint32_t functions)
{
    int STATUS = 0;

    if ((_trace_filename != NULL) && (_trace_filename != filename))
    {
        if ( _tracefile != NULL )
        {
            fclose(_tracefile);
            _tracefile = NULL;
        }
        free(_trace_filename);
        _trace_filename = NULL;
    }

    if ((filename != NULL) && (_tracefile == NULL))
    {
        _tracefile = fopen(filename, "w+");

        //TODO: Add time and header banner to the log file to indicate we've just opened a new log session

        if ( _tracefile != NULL )
        {
            _trace_filename = (char*)malloc((strlen(filename) + 1) * sizeof(char));
            strcpy(_trace_filename, filename);
        } else
            STATUS = 1;
    }

    _trace_functions = functions;
    _trace_level = level;
    _trace_modules = modules;

    return STATUS;
}

void trace_print_module_function(uint32_t module, uint32_t level, uint32_t function, const char *format, ...)
{
    if ( ((level <=_trace_level) && ((module & _trace_modules) > 0))
        || ((function & _trace_functions) > 0) )
    {
        va_list arg_list;
        char message[VL53L0X_MAX_STRING_LENGTH_PLT];

        va_start(arg_list, format);
        vsnprintf(message, VL53L0X_MAX_STRING_LENGTH_PLT, format, arg_list);
        va_end(arg_list);

        if (_tracefile != NULL)
            fprintf(_tracefile, message);
        else
            printf(message);
    }
}

