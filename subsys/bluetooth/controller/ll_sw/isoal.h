/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/types.h>
#include <toolchain.h>

/** Function return error codes */
typedef uint8_t isoal_status_t;
#define ISOAL_STATUS_OK                   ((isoal_status_t) 0x00) /* No error */
#define ISOAL_STATUS_ERR_SINK_ALLOC       ((isoal_status_t) 0x01) /* Sink pool full */
#define ISOAL_STATUS_ERR_SOURCE_ALLOC     ((isoal_status_t) 0x02) /* Source pool full */
#define ISOAL_STATUS_ERR_SDU_ALLOC        ((isoal_status_t) 0x04) /* SDU allocation */
#define ISOAL_STATUS_ERR_SDU_EMIT         ((isoal_status_t) 0x08) /* SDU emission */
#define ISOAL_STATUS_ERR_UNSPECIFIED      ((isoal_status_t) 0x10) /* Unspecified error */

#define BT_ROLE_BROADCAST (BT_CONN_ROLE_PERIPHERAL + 1)

/** Handle to a registered ISO Sub-System sink */
typedef uint8_t  isoal_sink_handle_t;

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
#define ISOAL_PDU_STATUS_LOST_DATA     ((isoal_pdu_status_t) 0x01)
#define ISOAL_PDU_STATUS_ERRORS        ((isoal_pdu_status_t) 0x02)

/** Production mode */
typedef uint8_t isoal_production_mode_t;
#define ISOAL_PRODUCTION_MODE_DISABLED    ((isoal_production_mode_t) 0x00)
#define ISOAL_PRODUCTION_MODE_ENABLED     ((isoal_production_mode_t) 0x01)


/**
 *  Origin of {PDU or SDU}.
 */
struct isoal_rx_origin {
	/** Originating subsystem */
	enum __packed {
		ISOAL_SUBSYS_BT_LLL   = 0x01,    /*!< Bluetooth LLL */
		ISOAL_SUBSYS_BT_HCI   = 0x02,    /*!< Bluetooth HCI */
		ISOAL_SUBSYS_VS       = 0xff     /*!< Vendor specific */
	} subsys;

	/** Subsystem instance */
	union {
		struct {
			uint16_t  handle;       /*!< BT connection handle */
		} bt_lll;
	} inst;
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

/** @brief Produced ISO SDU frame with associated meta data */
struct isoal_sdu_produced {
	/** Status of contents, if valid or SDU was lost */
	isoal_sdu_status_t      status;
	/** Regardless of status, we always have timing */
	isoal_time_t            timestamp;
	/** Sequence number of SDU */
	isoal_sdu_cnt_t         seqn;
	/** Regardless of status, we always know where the PDUs that produced
	 *  this SDU, came from
	 */
	struct isoal_rx_origin  origin;
	/** Contents and length can only be trusted if status is valid */
	struct isoal_sdu_buffer contents;
	/** Optional context to be carried from PDU at alloc-time */
	void                    *ctx;
};


/** @brief Received ISO PDU with associated meta data */
struct isoal_pdu_rx {
	/** Meta */
	struct node_rx_iso_meta  *meta;
	/** PDU contents and length can only be trusted if status is valid */
	struct pdu_iso           *pdu;
};


/* Forward declaration */
struct isoal_sink;

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
	enum {
		ISOAL_MODE_CIS,
		ISOAL_MODE_BIS
	} mode;
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


isoal_status_t isoal_init(void);

isoal_status_t isoal_reset(void);

isoal_status_t isoal_sink_create(uint16_t handle,
				 uint8_t  role,
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
