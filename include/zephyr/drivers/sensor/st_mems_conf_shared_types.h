/*
 * Copyright (c) 2025 STMicroelectronics.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>

/*
 * This file contain the MEMS Configuration Shared Types v2.0 that are used inside
 * the various algo configuration files.
 */

#ifndef MEMS_CONF_SHARED_TYPES
#define MEMS_CONF_SHARED_TYPES

#define MEMS_CONF_ARRAY_LEN(x) (sizeof(x) / sizeof(x[0]))

/*
 * MEMS_CONF_SHARED_TYPES format supports the following operations:
 * - MEMS_CONF_OP_TYPE_TYPE_READ: read the register at the location specified
 *   by the "address" field ("data" field is ignored)
 * - MEMS_CONF_OP_TYPE_TYPE_WRITE: write the value specified by the "data"
 *   field at the location specified by the "address" field
 * - MEMS_CONF_OP_TYPE_TYPE_DELAY: wait the number of milliseconds specified by
 *   the "data" field ("address" field is ignored)
 * - MEMS_CONF_OP_TYPE_TYPE_POLL_SET: poll the register at the location
 *   specified by the "address" field until all the bits identified by the mask
 *   specified by the "data" field are set to 1
 * - MEMS_CONF_OP_TYPE_TYPE_POLL_RESET: poll the register at the location
 *   specified by the "address" field until all the bits identified by the mask
 *   specified by the "data" field are reset to 0
 */

struct mems_conf_name_list {
 const char *const *list;
 uint16_t len;
};

enum {
 MEMS_CONF_OP_TYPE_READ = 1,
 MEMS_CONF_OP_TYPE_WRITE = 2,
 MEMS_CONF_OP_TYPE_DELAY = 3,
 MEMS_CONF_OP_TYPE_POLL_SET = 4,
 MEMS_CONF_OP_TYPE_POLL_RESET = 5
};

struct mems_conf_op {
 uint8_t type;
 uint8_t address;
 uint8_t data;
};

struct mems_conf_op_list {
 const struct mems_conf_op *list;
 uint32_t len;
};

#endif /* MEMS_CONF_SHARED_TYPES */

#ifndef MEMS_CONF_METADATA_SHARED_TYPES
#define MEMS_CONF_METADATA_SHARED_TYPES

struct mems_conf_application {
 char *name;
 char *version;
};

struct mems_conf_result {
 uint8_t code;
 char *label;
};

enum {
 MEMS_CONF_OUTPUT_CORE_HW = 1,
 MEMS_CONF_OUTPUT_CORE_EMB = 2,
 MEMS_CONF_OUTPUT_CORE_FSM = 3,
 MEMS_CONF_OUTPUT_CORE_MLC = 4,
 MEMS_CONF_OUTPUT_CORE_ISPU = 5
};

enum {
 MEMS_CONF_OUTPUT_TYPE_UINT8_T = 1,
 MEMS_CONF_OUTPUT_TYPE_INT8_T = 2,
 MEMS_CONF_OUTPUT_TYPE_CHAR = 3,
 MEMS_CONF_OUTPUT_TYPE_UINT16_T = 4,
 MEMS_CONF_OUTPUT_TYPE_INT16_T = 5,
 MEMS_CONF_OUTPUT_TYPE_UINT32_T = 6,
 MEMS_CONF_OUTPUT_TYPE_INT32_T = 7,
 MEMS_CONF_OUTPUT_TYPE_UINT64_T = 8,
 MEMS_CONF_OUTPUT_TYPE_INT64_T = 9,
 MEMS_CONF_OUTPUT_TYPE_HALF = 10,
 MEMS_CONF_OUTPUT_TYPE_FLOAT = 11,
 MEMS_CONF_OUTPUT_TYPE_DOUBLE = 12
};

struct mems_conf_output {
 char *name;
 uint8_t core;
 uint8_t type;
 uint16_t len;
 uint8_t reg_addr;
 char *reg_name;
 uint8_t num_results;
 const struct mems_conf_result *results;
};

struct mems_conf_output_list {
 const struct mems_conf_output *list;
 uint16_t len;
};

struct mems_conf_mlc_identifier {
 uint8_t fifo_tag;
 uint16_t id;
 char *label;
};

struct mems_conf_mlc_identifier_list {
 const struct mems_conf_mlc_identifier *list;
 uint16_t len;
};

#endif /* MEMS_CONF_METADATA_SHARED_TYPES */
