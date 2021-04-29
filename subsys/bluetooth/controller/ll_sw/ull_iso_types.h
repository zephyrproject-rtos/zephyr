/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct ll_iso_datapath {
	uint8_t  path_dir;
	uint8_t  path_id;
	uint8_t  coding_format;
	uint16_t company_id;
	isoal_sink_handle_t sink_hdl;
};
