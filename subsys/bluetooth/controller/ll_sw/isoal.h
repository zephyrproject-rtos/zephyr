/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/types.h>
#include <zephyr/toolchain.h>

/** Function return error codes */
typedef uint8_t isoal_status_t;
#define ISOAL_STATUS_OK                   ((isoal_status_t) 0x00) /* No error */
#define ISOAL_STATUS_ERR_SINK_ALLOC       ((isoal_status_t) 0x01) /* Sink pool full */
#define ISOAL_STATUS_ERR_SOURCE_ALLOC     ((isoal_status_t) 0x02) /* Source pool full */
#define ISOAL_STATUS_ERR_SDU_ALLOC        ((isoal_status_t) 0x04) /* SDU allocation */
#define ISOAL_STATUS_ERR_SDU_EMIT         ((isoal_status_t) 0x08) /* SDU emission */
#define ISOAL_STATUS_ERR_PDU_ALLOC        ((isoal_status_t) 0x10) /* PDU allocation */
#define ISOAL_STATUS_ERR_PDU_EMIT         ((isoal_status_t) 0x20) /* PDU emission */
#define ISOAL_STATUS_ERR_UNSPECIFIED      ((isoal_status_t) 0x80) /* Unspecified error */

#define BT_ROLE_BROADCAST (BT_CONN_ROLE_PERIPHERAL + 1)

/** Handle to a registered ISO Sub-System sink */
typedef uint8_t  isoal_sink_handle_t;

/** Handle to a registered ISO Sub-System source */
typedef uint8_t  isoal_source_handle_t;

/** Byte length of an ISO SDU */
typedef uint16_t isoal_sdu_len_t;

/** Byte length of an ISO PDU */
typedef uint8_t isoal_pdu_len_t;

/** Count (ID number) of an ISO SDU */
typedef uint16_t isoal_sdu_cnt_t;

/** Count (ID number) of an ISO PDU */
typedef uint64_t isoal_pdu_cnt_t;

/** Ticks. Used for timestamp */
typedef uint32_t isoal_time_t;

/** SDU status codes */
typedef uint8_t isoal_sdu_status_t;
#define ISOAL_SDU_STATUS_VALID          ((isoal_sdu_status_t) 0x00)
#define ISOAL_SDU_STATUS_ERRORS         ((isoal_sdu_status_t) 0x01)
#define ISOAL_SDU_STATUS_LOST_DATA      ((isoal_sdu_status_t) 0x02)

/** PDU status codes */
typedef uint8_t isoal_pdu_status_t;
#define ISOAL_PDU_STATUS_VALID         ((isoal_pdu_status_t) 0x00)
#define ISOAL_PDU_STATUS_ERRORS        ((isoal_pdu_status_t) 0x01)
#define ISOAL_PDU_STATUS_LOST_DATA     ((isoal_pdu_status_t) 0x02)

/** Production mode */
typedef uint8_t isoal_production_mode_t;
#define ISOAL_PRODUCTION_MODE_DISABLED    ((isoal_production_mode_t) 0x00)
#define ISOAL_PRODUCTION_MODE_ENABLED     ((isoal_production_mode_t) 0x01)


enum isoal_mode {
	ISOAL_MODE_CIS,
	ISOAL_MODE_BIS
};

/**
 * @brief ISO frame SDU buffer - typically an Audio frame buffer
 *
 * This provides the underlying vehicle of ISO SDU interchange through the
 * ISO socket, the contents of which is generally an array of encoded bytes.
 * Access and life time of this should be limited to ISO and the ISO socket.
 * We assume no byte-fractional code words.
 *
 * Decoding code words to samples is the responsibility of the ISO sub system.
 */
struct isoal_sdu_buffer {
	/** Code word buffer
	 *  Type, location and alignment decided by ISO sub system
	 */
	void *dbuf;
	/** Number of bytes accessible behind the dbuf pointer */
	isoal_sdu_len_t  size;
};


/**
 * @brief ISO frame PDU buffer
 */
struct isoal_pdu_buffer {
	/** Additional handle for the provided pdu */
	void             *handle;
	/** PDU contents */
	struct pdu_iso   *pdu;
	/** Maximum size of the data buffer allocated for the payload in the PDU.
	 *  Should be at least Max_PDU_C_To_P / Max_PDU_P_To_C depending on
	 *  the role.
	 */
	isoal_pdu_len_t  size;
};


/** @brief Produced ISO SDU frame with associated meta data */
struct isoal_sdu_produced {
	/** Status of contents, if valid or SDU was lost.
	 * This maps directly to the HCI ISO Data packet Packet_Status_Flag.
	 * BT Core V5.3 : Vol 4 HCI I/F : Part G HCI Func. Spec.:
	 * 5.4.5 HCI ISO Data packets : Table 5.2 :
	 * Packet_Status_Flag (in packets sent by the Controller)
	 */
	isoal_sdu_status_t      status;
	/** Regardless of status, we always have timing */
	isoal_time_t            timestamp;
	/** Sequence number of SDU */
	isoal_sdu_cnt_t         seqn;
	/** Contents and length can only be trusted if status is valid */
	struct isoal_sdu_buffer contents;
	/** Optional context to be carried from PDU at alloc-time */
	void                    *ctx;
};


/** @brief Produced ISO PDU encapsulation */
struct isoal_pdu_produced {
	/** Contents information provided at PDU allocation */
	struct isoal_pdu_buffer contents;
};


/** @brief Received ISO PDU with associated meta data */
struct isoal_pdu_rx {
	/** Meta */
	struct node_rx_iso_meta  *meta;
	/** PDU contents and length can only be trusted if status is valid */
	struct pdu_iso           *pdu;
};

/** @brief Received ISO SDU with associated meta data */
struct isoal_sdu_tx {
	/** Code word buffer
	 *  Type, location and alignment decided by ISO sub system
	 */
	void *dbuf;
	/** Number of bytes accessible behind the dbuf pointer */
	isoal_sdu_len_t  size;
	/** SDU packet boundary flags from HCI indicating the fragment type
	 *  can be directly assigned to the SDU state
	 */
	uint8_t sdu_state;
	/** Packet sequence number from HCI ISO Data Header (ISO Data Load
	 *  Field)
	 */
	uint16_t packet_sn;
	/** ISO SDU length from HCI ISO Data Header (ISO Data Load Field) */
	uint16_t iso_sdu_length;
	/** Time stamp from HCI or vendor specific path (us) */
	uint32_t time_stamp;
	/** CIG Reference of target event (us, compensated for drift) */
	uint32_t cig_ref_point;
	/** Target Event of SDU */
	uint64_t target_event:39;
};



/* Forward declaration */
struct isoal_sink;
struct node_tx_iso;

/**
 * @brief  Callback: Request memory for a new ISO SDU buffer
 *
 * Proprietary ISO sub systems may have
 * specific requirements or opinions on where to locate ISO SDUs; some
 * memories may be faster, may be dynamically mapped in, etc.
 *
 * @return ISOAL_STATUS_ERR_ALLOC if size_request could not be fulfilled, otherwise
 *         ISOAL_STATUS_OK.
 */
typedef isoal_status_t (*isoal_sink_sdu_alloc_cb)(
	/*!< [in]  Sink context */
	const struct isoal_sink    *sink_ctx,
	/*!< [in]  Received PDU */
	const struct isoal_pdu_rx  *valid_pdu,
	/*!< [out] Struct is modified. Must not be NULL */
	struct isoal_sdu_buffer    *sdu_buffer
);

/**
 * @brief  Callback: Push an ISO SDU into an ISO sink
 *
 * Call also handing back buffer ownership
 */
typedef isoal_status_t (*isoal_sink_sdu_emit_cb)(
	/*!< [in]  Sink context */
	const struct isoal_sink         *sink_ctx,
	/*!< [in]  Filled valid SDU to be pushed */
	const struct isoal_sdu_produced *valid_sdu
);

/**
 * @brief  Callback: Write a number of bytes to SDU buffer
 */
typedef isoal_status_t (*isoal_sink_sdu_write_cb)(
	/*!< [in]  Destination buffer */
	void          *dbuf,
	/*!< [in]  Source data */
	const uint8_t *pdu_payload,
	/*!< [in]  Number of bytes to be copied */
	const size_t consume_len
);


struct isoal_sink_config {
	enum isoal_mode mode;
	/* TODO add SDU and PDU max length etc. */
};

struct isoal_sink_session {
	isoal_sink_sdu_alloc_cb  sdu_alloc;
	isoal_sink_sdu_emit_cb   sdu_emit;
	isoal_sink_sdu_write_cb  sdu_write;
	struct isoal_sink_config param;
	isoal_sdu_cnt_t          seqn;
	uint16_t                 handle;
	uint8_t                  pdus_per_sdu;
	uint8_t                  framed;
	uint32_t                 latency_unframed;
	uint32_t                 latency_framed;
};

struct isoal_sdu_production {
	/* Permit atomic enable/disable of SDU production */
	volatile isoal_production_mode_t  mode;
	/* We are constructing an SDU from {<1 or =1 or >1} PDUs */
	struct isoal_sdu_produced  sdu;
	/* Bookkeeping */
	isoal_pdu_cnt_t  prev_pdu_id : 39;
	/* Assumes that isoal_pdu_cnt_t is a uint64_t bit field */
	uint64_t prev_pdu_is_end:1;
	uint64_t prev_pdu_is_padding:1;
	uint64_t sdu_allocated:1;
	enum {
		ISOAL_START,
		ISOAL_CONTINUE,
		ISOAL_ERR_SPOOL
	} fsm;
	uint8_t pdu_cnt;
	uint8_t sdu_state;
	isoal_sdu_len_t sdu_written;
	isoal_sdu_len_t sdu_available;
	isoal_sdu_status_t sdu_status;
};

struct isoal_sink {
	/* Session-constant */
	struct isoal_sink_session session;

	/* State for SDU production */
	struct isoal_sdu_production sdu_production;
};

/* Forward declaration */
struct isoal_source;

/**
 * @brief  Callback: Request memory for a new ISO PDU buffer
 *
 * Proprietary ISO sub systems may have
 * specific requirements or opinions on where to locate ISO PDUs; some
 * memories may be faster, may be dynamically mapped in, etc.
 *
 * @return ISOAL_STATUS_ERR_ALLOC if size_request could not be fulfilled, otherwise
 *         ISOAL_STATUS_OK.
 */
typedef isoal_status_t (*isoal_source_pdu_alloc_cb)(
	/*!< [out] Struct is modified. Must not be NULL */
	struct isoal_pdu_buffer *pdu_buffer
);

/**
 * @brief  Callback: Release an ISO PDU
 */
typedef isoal_status_t (*isoal_source_pdu_release_cb)(
	/*!< [in]  PDU to be released */
	struct node_tx_iso *node_tx,
	/*!< [in]  CIS/BIS handle */
	const uint16_t handle,
	/*!< [in]  Cause of release. ISOAL_STATUS_OK means PDU was enqueued towards
	 *         LLL and sent or flushed, and may need further handling before
	 *         memory is released, e.g. forwarded to HCI thread for complete-
	 *         counting. Any other status indicates cause of error during SDU
	 *         fragmentation/segmentation, in which case the PDU will not be
	 *         produced.
	 */
	const isoal_status_t status
);

/**
 * @brief  Callback: Write a number of bytes to PDU buffer
 */
typedef isoal_status_t (*isoal_source_pdu_write_cb)(
	/*!< [out]  PDU under production */
	struct isoal_pdu_buffer *pdu_buffer,
	/*!< [in]  Offset within PDU buffer */
	const size_t offset,
	/*!< [in]  Source data */
	const uint8_t *sdu_payload,
	/*!< [in]  Number of bytes to be copied */
	const size_t consume_len
);

/**
 * @brief  Callback: Enqueue an ISO PDU
 */
typedef isoal_status_t (*isoal_source_pdu_emit_cb)(
	/*!< [in]  PDU to be enqueued */
	struct node_tx_iso *node_tx,
	/*!< [in]  CIS/BIS handle */
	const uint16_t handle
);

struct isoal_source_config {
	enum isoal_mode mode;
	/* TODO add SDU and PDU max length etc. */
};

struct isoal_source_session {
	isoal_source_pdu_alloc_cb   pdu_alloc;
	isoal_source_pdu_write_cb   pdu_write;
	isoal_source_pdu_emit_cb    pdu_emit;
	isoal_source_pdu_release_cb pdu_release;

	struct isoal_source_config param;
	isoal_sdu_cnt_t            seqn;
	uint16_t                   handle;
	uint16_t                   iso_interval;
	uint8_t                    framed;
	uint8_t                    burst_number;
	uint8_t                    pdus_per_sdu;
	uint8_t                    max_pdu_size;
	int32_t                    latency_unframed;
	int32_t                    latency_framed;
};

struct isoal_pdu_production {
	/* Permit atomic enable/disable of PDU production */
	volatile isoal_production_mode_t  mode;
	/* We are constructing a PDU from {<1 or =1 or >1} SDUs */
	struct isoal_pdu_produced pdu;
	uint8_t                   pdu_state;
	/* PDUs produced for current SDU */
	uint8_t                   pdu_cnt;
	uint64_t                  payload_number:39;
	uint64_t                  seg_hdr_sc:1;
	uint64_t                  seg_hdr_length:8;
	uint64_t                  sdu_fragments:8;
	isoal_pdu_len_t           pdu_written;
	isoal_pdu_len_t           pdu_available;
	/* Location (byte index) of last segmentation header */
	isoal_pdu_len_t           last_seg_hdr_loc;
};

struct isoal_source {
	/* Session-constant */
	struct isoal_source_session session;

	/* State for PDU production */
	struct isoal_pdu_production pdu_production;
};

isoal_status_t isoal_init(void);

isoal_status_t isoal_reset(void);

isoal_status_t isoal_sink_create(uint16_t handle,
				 uint8_t  role,
				 uint8_t  framed,
				 uint8_t  burst_number,
				 uint8_t  flush_timeout,
				 uint32_t sdu_interval,
				 uint16_t iso_interval,
				 uint32_t stream_sync_delay,
				 uint32_t group_sync_delay,
				 isoal_sink_sdu_alloc_cb sdu_alloc,
				 isoal_sink_sdu_emit_cb  sdu_emit,
				 isoal_sink_sdu_write_cb  sdu_write,
				 isoal_sink_handle_t *hdl);

struct isoal_sink_config *isoal_get_sink_param_ref(isoal_sink_handle_t hdl);

void isoal_sink_enable(isoal_sink_handle_t hdl);

void isoal_sink_disable(isoal_sink_handle_t hdl);

void isoal_sink_destroy(isoal_sink_handle_t hdl);

isoal_status_t isoal_rx_pdu_recombine(isoal_sink_handle_t sink_hdl,
				      const struct isoal_pdu_rx *pdu_meta);

/* SDU Call backs for HCI interface  */
isoal_status_t sink_sdu_alloc_hci(const struct isoal_sink    *sink_ctx,
				  const struct isoal_pdu_rx  *valid_pdu,
				  struct isoal_sdu_buffer    *sdu_buffer);
isoal_status_t sink_sdu_emit_hci(const struct isoal_sink         *sink_ctx,
				 const struct isoal_sdu_produced *valid_sdu);
isoal_status_t sink_sdu_write_hci(void *dbuf,
				  const uint8_t *pdu_payload,
				  const size_t consume_len);

isoal_status_t isoal_source_create(uint16_t handle,
				   uint8_t  role,
				   uint8_t  framed,
				   uint8_t  burst_number,
				   uint8_t  flush_timeout,
				   uint8_t  max_octets,
				   uint32_t sdu_interval,
				   uint16_t iso_interval,
				   uint32_t stream_sync_delay,
				   uint32_t group_sync_delay,
				   isoal_source_pdu_alloc_cb pdu_alloc,
				   isoal_source_pdu_write_cb pdu_write,
				   isoal_source_pdu_emit_cb pdu_emit,
				   isoal_source_pdu_release_cb pdu_release,
				   isoal_source_handle_t *hdl);

struct isoal_source_config *isoal_get_source_param_ref(isoal_source_handle_t hdl);

void isoal_source_enable(isoal_source_handle_t hdl);

void isoal_source_disable(isoal_source_handle_t hdl);

void isoal_source_destroy(isoal_source_handle_t hdl);

isoal_status_t isoal_tx_sdu_fragment(isoal_source_handle_t source_hdl,
				     struct isoal_sdu_tx *tx_sdu);

void isoal_tx_pdu_release(isoal_source_handle_t source_hdl,
			  struct node_tx_iso *node_tx);
