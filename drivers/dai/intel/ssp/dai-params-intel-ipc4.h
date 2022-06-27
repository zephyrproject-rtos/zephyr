/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __DAI_PARAMS_INTEL_IPC4_H__
#define __DAI_PARAMS_INTEL_IPC4_H__

#include <stdint.h>

#define DAI_INTEL_I2S_TDM_MAX_SLOT_MAP_COUNT 8

/**< Type of the gateway. */
enum dai_intel_ipc4_connector_node_id_type {
	/**< HD/A host output (-> DSP). */
	dai_intel_ipc4_hda_host_output_class = 0,
	/**< HD/A host input (<- DSP). */
	dai_intel_ipc4_hda_host_input_class = 1,
	/**< HD/A host input/output (rsvd for future use). */
	dai_intel_ipc4_hda_host_inout_class = 2,

	/**< HD/A link output (DSP ->). */
	dai_intel_ipc4_hda_link_output_class = 8,
	/**< HD/A link input (DSP <-). */
	dai_intel_ipc4_hda_link_input_class = 9,
	/**< HD/A link input/output (rsvd for future use). */
	dai_intel_ipc4_hda_link_inout_class = 10,

	/**< DMIC link input (DSP <-). */
	dai_intel_ipc4_dmic_link_input_class = 11,

	/**< I2S link output (DSP ->). */
	dai_intel_ipc4_i2s_link_output_class = 12,
	/**< I2S link input (DSP <-). */
	dai_intel_ipc4_i2s_link_input_class = 13,

	/**< ALH link output, legacy for SNDW (DSP ->). */
	dai_intel_ipc4_alh_link_output_class = 16,
	/**< ALH link input, legacy for SNDW (DSP <-). */
	dai_intel_ipc4_alh_link_input_class = 17,

	/**< SNDW link output (DSP ->). */
	dai_intel_ipc4_alh_snd_wire_stream_link_output_class = 16,
	/**< SNDW link input (DSP <-). */
	dai_intel_ipc4_alh_snd_wire_stream_link_input_class = 17,

	/**< UAOL link output (DSP ->). */
	dai_intel_ipc4_alh_uaol_stream_link_output_class = 18,
	/**< UAOL link input (DSP <-). */
	dai_intel_ipc4_alh_uaol_stream_link_input_class = 19,

	/**< IPC output (DSP ->). */
	dai_intel_ipc4_ipc_output_class = 20,
	/**< IPC input (DSP <-). */
	dai_intel_ipc4_ipc_input_class = 21,

	/**< I2S Multi gtw output (DSP ->). */
	dai_intel_ipc4_i2s_multi_link_output_class = 22,
	/**< I2S Multi gtw input (DSP <-). */
	dai_intel_ipc4_i2s_multi_link_input_class = 23,
	/**< GPIO */
	dai_intel_ipc4_gpio_class = 24,
	/**< SPI */
	dai_intel_ipc4_spi_output_class = 25,
	dai_intel_ipc4_spi_input_class = 26,
	dai_intel_ipc4_max_connector_node_id_type
};

/**< Base top-level structure of an address of a gateway. */
/*!
 * The virtual index value, presented on the top level as raw 8 bits,
 * is expected to be encoded in a gateway specific way depending on
 * the actual type of gateway.
 */
union dai_intel_ipc4_connector_node_id {

	/**< Raw 32-bit value of node id. */
	uint32_t dw;

	/**< Bit fields */
	struct {
		/**< Index of the virtual DMA at the gateway. */
		uint32_t v_index : 8;

		/**< Type of the gateway, one of ConnectorNodeId::Type values. */
		uint32_t dma_type : 5;

		/**< Rsvd field. */
		uint32_t _rsvd : 19;
	} f; /**<< Bits */
} __packed;

/*!
 * Attributes are usually provided along with the gateway configuration
 * BLOB when the FW is requested to instantiate that gateway.
 *
 * There are flags which requests FW to allocate gateway related data
 * (buffers and other items used while transferring data, like linked list)
 *  to be allocated from a special memory area, e.g low power memory.
 */
union dai_intel_ipc4_gateway_attributes {

	/**< Raw value */
	uint32_t dw;

	/**< Access to the fields */
	struct {
		/**< Gateway data requested in low power memory. */
		uint32_t lp_buffer_alloc : 1;

		/**< Gateway data requested in register file memory. */
		uint32_t alloc_from_reg_file : 1;

		/**< Reserved field */
		uint32_t _rsvd : 30;
	} bits; /**<< Bits */
} __packed;

/**< Configuration for the IPC Gateway */
struct dai_intel_ipc4_gateway_config_blob {

	/**< Size of the gateway buffer, specified in bytes */
	uint32_t buffer_size;

	/**< Flags */
	union flags {
		struct bits {
			/**< Activates high threshold notification */
			/*!
			 * Indicates whether notification should be sent to the host
			 * when the size of data in the buffer reaches the high threshold
			 * specified by threshold_high parameter.
			 */
			uint32_t notif_high : 1;

			/**< Activates low threshold notification */
			/*!
			 * Indicates whether notification should be sent to the host
			 * when the size of data in the buffer reaches the low threshold
			 * specified by threshold_low parameter.
			 */
			uint32_t notif_low : 1;

			/**< Reserved field */
			uint32_t rsvd : 30;
		} f; /**<< Bits */
		/**< Raw value of flags */
		uint32_t flags_raw;
	} u; /**<< Flags */

	/**< High threshold */
	/*!
	 * Specifies the high threshold (in bytes) for notifying the host
	 * about the buffered data level.
	 */
	uint32_t threshold_high;

	/**< Low threshold */
	/*!
	 * Specifies the low threshold (in bytes) for notifying the host
	 * about the buffered data level.
	 */
	uint32_t threshold_low;
} __packed;

/* i2s Configuration BLOB building blocks */

/* i2s registers for i2s Configuration */
struct dai_intel_ipc4_ssp_config {
	uint32_t ssc0;
	uint32_t ssc1;
	uint32_t sscto;
	uint32_t sspsp;
	uint32_t sstsa;
	uint32_t ssrsa;
	uint32_t ssc2;
	uint32_t sspsp2;
	uint32_t ssc3;
	uint32_t ssioc;
} __packed;

struct dai_intel_ipc4_ssp_mclk_config {
	/* master clock divider control register */
	uint32_t mdivc;

	/* master clock divider ratio register */
	uint32_t mdivr;
} __packed;

struct dai_intel_ipc4_ssp_driver_config {
	struct dai_intel_ipc4_ssp_config i2s_config;
	struct dai_intel_ipc4_ssp_mclk_config mclk_config;
} __packed;

struct dai_intel_ipc4_ssp_start_control {
	/* delay in msec between enabling interface (moment when
	 * Copier instance is being attached to the interface) and actual
	 * interface start. Value of 0 means no delay.
	 */
	uint32_t clock_warm_up    : 16;

	/* specifies if parameters target MCLK (1) or SCLK (0) */
	uint32_t mclk             : 1;

	/* value of 1 means that clock should be started immediately
	 * even if no Copier instance is currently attached to the interface.
	 */
	uint32_t warm_up_ovr      : 1;
	uint32_t rsvd0            : 14;
} __packed;

struct dai_intel_ipc4_ssp_stop_control {
	/* delay in msec between stopping the interface
	 * (moment when Copier instance is being detached from the interface)
	 * and interface clock stop. Value of 0 means no delay.
	 */
	uint32_t clock_stop_delay : 16;

	/* value of 1 means that clock should be kept running (infinite
	 * stop delay) after Copier instance detaches from the interface.
	 */
	uint32_t keep_running     : 1;

	/* value of 1 means that clock should be stopped immediately */
	uint32_t clock_stop_ovr   : 1;
	uint32_t rsvd1            : 14;
} __packed;

union dai_intel_ipc4_ssp_dma_control {
	struct dai_intel_ipc4_ssp_control {
		struct dai_intel_ipc4_ssp_start_control start_control;
		struct dai_intel_ipc4_ssp_stop_control stop_control;
	} control_data;

	struct dai_intel_ipc4_mn_div_config {
		uint32_t mval;
		uint32_t nval;
	} mndiv_control_data;
} __packed;

struct dai_intel_ipc4_ssp_configuration_blob {
	union dai_intel_ipc4_gateway_attributes gw_attr;

	/* TDM time slot mappings */
	uint32_t tdm_ts_group[DAI_INTEL_I2S_TDM_MAX_SLOT_MAP_COUNT];

	/* i2s port configuration */
	struct dai_intel_ipc4_ssp_driver_config i2s_driver_config;

	/* optional configuration parameters */
	union dai_intel_ipc4_ssp_dma_control i2s_dma_control[0];
} __packed;

#endif
