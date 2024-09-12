/*
 * Copyright (c) 2024 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/atomic.h>
#include "util/memq.h"
#include "util/dbuf.h"

#include <zephyr/drivers/bluetooth/hci_driver.h>

#include "comdef.h"
#include "hci_api.h"

//#include "common/assert.h"
#include "hal_assert.h"
#include "hal_types.h"

#define LOG_LEVEL CONFIG_BT_HCI_DRIVER_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_ctlr_hci_driver);

/*******************************************************************************
 * TYPEDEFS
 */

/*******************************************************************************
 * CONSTANTS
 */
#define BT_HCI_SET_BD_ADDR           0xfc0c


/*******************************************************************************
 * LOCAL FUNCTIONS PROTOTYPES
 */
static int hci_driver_ll_send_to_host_cb(uint8 *pHciPkt, uint16 pktLen);
static void vs_set_bd_addr();

/*******************************************************************************
 * EXTERNS
 */
extern void bleStack_Init();
extern int HCI_HostToController(uint8_t *pHciPkt, uint16_t pktLen);

typedef int_fast16_t ICall_Errno;
typedef uint_least8_t ICall_EntityID;
typedef void *ICall_SyncHandle;
ICall_Errno ICall_registerApp(ICall_EntityID *entity,
                                            ICall_SyncHandle *msgSyncHdl);
extern void RegisterAssertCback(assertCback_t appAssertHandler);

/*******************************************************************************
 * GLOBAL VARIABLES
 */

hci_c2h_cbs_t cbs;

/*******************************************************************************
 * API FUNCTIONS
 */

/*******************************************************************************
 * @fn          AssertHandler
 *
 * @brief       This is the Application's callback handler for asserts raised
 *              in the stack.  When EXT_HAL_ASSERT is defined in the Stack Wrapper
 *              project this function will be called when an assert is raised,
 *              and can be used to observe or trap a violation from expected
 *              behavior.
 *
 *              As an example, for Heap allocation failures the Stack will raise
 *              HAL_ASSERT_CAUSE_OUT_OF_MEMORY as the assertCause and
 *              HAL_ASSERT_SUBCAUSE_NONE as the assertSubcause.  An application
 *              developer could trap any malloc failure on the stack by calling
 *              HAL_ASSERT_SPINLOCK under the matching case.
 *
 *              An application developer is encouraged to extend this function
 *              for use by their own application.  To do this, add hal_assert.c
 *              to your project workspace, the path to hal_assert.h (this can
 *              be found on the stack side). Asserts are raised by including
 *              hal_assert.h and using macro HAL_ASSERT(cause) to raise an
 *              assert with argument assertCause.  the assertSubcause may be
 *              optionally set by macro HAL_ASSERT_SET_SUBCAUSE(subCause) prior
 *              to asserting the cause it describes. More information is
 *              available in hal_assert.h.
 *
 * input parameters
 *
 * @param       assertCause    - Assert cause as defined in hal_assert.h.
 * @param       assertSubCause - Optional assert subcause (see hal_assert.h).
 *
 * output parameters
 *
 * @param       None.
 *
 * @return      None.
 */
void AssertHandler(uint8 assertCause, uint8 assertSubCause)
{
  // check the assert cause
  switch (assertCause) {
    // This assert is raised from the BLE Stack when a malloc failure occurs.
    case HAL_ASSERT_CAUSE_OUT_OF_MEMORY:
      // ERROR: OUT OF MEMORY
      HAL_ASSERT_SPINLOCK;
      break;

    case HAL_ASSERT_CAUSE_INTERNAL_ERROR:
      // check the subcause
      if (assertSubCause == HAL_ASSERT_SUBCAUSE_FW_INERNAL_ERROR) {
        // ERROR: INTERNAL FW ERROR
        HAL_ASSERT_SPINLOCK;
      } else {
        // ERROR: INTERNAL ERROR
        HAL_ASSERT_SPINLOCK;
      }
      break;

    // An assert originating from an ICall failure.
    case HAL_ASSERT_CAUSE_ICALL_ABORT:
      // ERROR: ICALL ABORT
      HAL_ASSERT_SPINLOCK;
      break;

    default:
     // ERROR: DEFAULT SPINLOCK
      HAL_ASSERT_SPINLOCK;
      break;
  }

  return;
}

#define HCI_TYPE_INVALID                             (0xFF)

static const enum bt_buf_type translation_array_in[6] = { HCI_TYPE_INVALID,                 /* BT_HCI_H4_NONE */ /* NOT SUPPORTED */
                                                     HCI_TYPE_INVALID,                 /* BT_HCI_H4_CMD  */ /* NOT SUPPORTED */
                                                     BT_BUF_ACL_IN,                    /* BT_HCI_H4_ACL  */
                                                     HCI_TYPE_INVALID,                 /* BT_HCI_H4_SCO  */ /* NOT SUPPORTED */
                                                     BT_BUF_EVT,                       /* BT_HCI_H4_EVT  */
                                                     HCI_TYPE_INVALID                  /* BT_HCI_H4_ISO  */ /* NOT SUPPORTED */
};

static const uint8_t     translation_array_out[6] = { BT_HCI_H4_CMD,                            /* BT_BUF_CMD      */
                                                      HCI_TYPE_INVALID,                         /* BT_BUF_EVT      */ /* NOT SUPPORTED */
                                                      BT_HCI_H4_ACL,                            /* BT_BUF_ACL_OUT  */
                                                      HCI_TYPE_INVALID,                         /* BT_BUF_ISO_OUT  */ /* NOT SUPPORTED */
                                                      HCI_TYPE_INVALID,                         /* BT_BUF_ISO_IN   */ /* NOT SUPPORTED */
                                                      HCI_TYPE_INVALID                          /* BT_BUF_H4       */ /* NOT SUPPORTED */
};

static uint8_t hci_driver_add_pkt_type(struct net_buf *buf)
{
    /* Read the nut_buf buffer paket type */
    enum bt_buf_type type = bt_buf_get_type(buf);

    /* Translate the nut_buf buffer paket type into the HCI Paket type */
	uint8_t h4_type = translation_array_out[type];

	if (h4_type == HCI_TYPE_INVALID) {
        LOG_ERR("Received Invalid pkt type from the Host: %u", type);
		return FAILURE;
	}

    /* Set the HCI Paket type into the payload */
    net_buf_simple_push_mem(&buf->b, &h4_type, sizeof(h4_type));

	return SUCCESS;
}

static int hci_driver_send(struct net_buf *buf)
{
    static int first_entry = TRUE;
	int err = SUCCESS;

    if (TRUE == first_entry) {
        /* Register the calling context in the icall */
        ICall_SyncHandle syncEvent_dummy;
        ICall_EntityID icall_entity_dummy;
        ICall_registerApp(&icall_entity_dummy, &syncEvent_dummy);
        first_entry = FALSE;

        /* In case bt_ctlr_set_public_addr was called, we need to set the BD_ADDR before executing any other commands */
        vs_set_bd_addr();
    }

	LOG_DBG("enter");

	if (!buf->len) {
		LOG_ERR("Empty HCI packet");
		return -EINVAL;
	}

	err = hci_driver_add_pkt_type(buf);
	if (SUCCESS != err) {
		net_buf_unref(buf);
		return -EINVAL;
	}

	err = HCI_HostToController(buf->data, buf->len);
	if (SUCCESS != err) {
		net_buf_unref(buf);
		return -EINVAL;
	}
    net_buf_unref(buf);

	LOG_DBG("exit: %d", err);

	return err;
}

struct net_buf *hci_evt_create(uint8 *pHciPkt, uint16 pktLen)
{
	struct net_buf *buf;

	enum bt_buf_type buf_type = translation_array_in[pHciPkt[0]/* pktType */];

    if (buf_type == HCI_TYPE_INVALID) {
        LOG_ERR("Received Invalid pkt type from the Controller: %u", pHciPkt[0]);
        return NULL;
    }

	buf = bt_buf_get_rx(buf_type, K_NO_WAIT);

	if (NULL != buf) {
        /* Skip the native H4_EVT OpCode as
         * - HCI_RAW adds it in bt_recv
         * - HCI_CORE doesn't expect it at all
         * */
        pHciPkt++;
        pktLen--;
        net_buf_add_mem(buf, pHciPkt, pktLen);
	}
	return buf;
}

static int hci_driver_ll_send_to_host_cb(uint8 *pHciPkt, uint16 pktLen)
{
    struct net_buf *buf = NULL;
    buf = hci_evt_create(pHciPkt, pktLen);

    if (buf) {
	bt_recv(buf);
	return SUCCESS;
    }

	return FAILURE;
}

static int hci_driver_open(void)
{
	int status = SUCCESS;

	/* Register Application callback to trap asserts raised in the Stack */
	RegisterAssertCback(AssertHandler);

	/* Init HCI Driver callbacks interface structure */
	cbs.send      = hci_driver_ll_send_to_host_cb;

	/* Register HCI Driver callbacks to provide hci_driver interface to the LL */
	status = HCI_ControllerToHostRegisterCb(&cbs);
	if (SUCCESS == status) {
		bleStack_Init();
        LOG_DBG("Success.");
	}

	return status;
}

static int hci_driver_close(void)
{
	return 0;
}

static const struct bt_hci_driver drv = {
	.name	= "TI HCI Controller",
	.bus	= BT_HCI_DRIVER_BUS_VIRTUAL,
	.quirks = BT_QUIRK_NO_AUTO_DLE,
	.open	= hci_driver_open,
	.close	= hci_driver_close,
	.send	= hci_driver_send,
};

static int hci_driver_init(void)
{

	bt_hci_driver_register(&drv);

	return 0;
}

SYS_INIT(hci_driver_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static bt_addr_t public_addr = {0};
uint8_t set_bd_addr = FALSE;

void bt_ctlr_set_public_addr(const uint8_t *addr)
{
    memcpy(&public_addr, addr, sizeof(public_addr));
    set_bd_addr = TRUE;
}

static void vs_set_bd_addr()
{
#ifdef CONFIG_HCI_HOST
    if (set_bd_addr) {
        struct net_buf *buf;
        bt_addr_t *bd_addr;

        buf = bt_hci_cmd_create(BT_HCI_SET_BD_ADDR, sizeof(*bd_addr));
        if (!buf) {
            return;
        }

        bd_addr = net_buf_add(buf, sizeof(*bd_addr));
        bt_addr_copy(bd_addr, &public_addr);

        bt_buf_set_type(buf, BT_BUF_CMD);

        hci_driver_add_pkt_type(buf);

        hci_driver_send(buf);

    }
#endif
}
