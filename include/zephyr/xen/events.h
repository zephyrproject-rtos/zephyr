/*
 * Copyright (c) 2021-2025 EPAM Systems
 * Copyright (c) 2022 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Xen event channel services.
 * @ingroup xen_event_channels
 */

#ifndef __XEN_EVENTS_H__
#define __XEN_EVENTS_H__

#include <zephyr/xen/public/event_channel.h>

#include <zephyr/kernel.h>

/**
 * @defgroup xen_event_channels Xen event channels
 * @ingroup xen_support
 * @brief Allocate, bind, and service Xen event channels from Zephyr.
 * @{
 */

/**
 * @brief Event-channel callback signature.
 *
 * The callback runs from the Xen event interrupt handler.
 *
 * @param priv User data pointer that was registered with the event channel.
 */
typedef void (*evtchn_cb_t)(void *priv);

/** @cond INTERNAL_HIDDEN */
/* Internal event-channel callback slot. */
struct event_channel_handle {
	evtchn_cb_t cb;
	void *priv;
};
typedef struct event_channel_handle evtchn_handle_t;
/** @endcond */

/**
 * @brief Query the status of an event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * The caller provides the input domain and port in @p status, and Xen fills the
 * remaining fields with the current channel state.
 *
 * @param[in,out] status Event-channel status request and response buffer.
 *
 * @return 0 on success, negative errno value on failure.
 */
int evtchn_status(evtchn_status_t *status);

/**
 * @brief Close a local event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port to close.
 *
 * @return 0 on success, negative errno value on failure.
 */
int evtchn_close(evtchn_port_t port);

/**
 * @brief Set the Xen priority assigned to an event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port to update.
 * @param priority Xen event priority value.
 *
 * @return 0 on success, negative errno value on failure.
 */
int evtchn_set_priority(evtchn_port_t port, uint32_t priority);

/**
 * @brief Send a notification over an event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port to notify.
 *
 * @return 0 on success, negative errno value on failure.
 */
int notify_evtchn(evtchn_port_t port);

/**
 * @brief Allocate an unbound event channel for the calling domain.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param remote_dom Domain that may bind to the allocated channel.
 *
 * @return Local event-channel port on success, negative errno value on failure.
 */
int alloc_unbound_event_channel(domid_t remote_dom);

/**
 * @brief Allocate an unbound event channel between two domains.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS,CONFIG_XEN_DOM0}
 *
 * @param dom Domain that receives the new local port. May be ``DOMID_SELF``.
 * @param remote_dom Peer domain that may bind to the port.
 *
 * @return Local event-channel port on success, negative errno value on failure.
 */
int alloc_unbound_event_channel_dom0(domid_t dom, domid_t remote_dom);

/**
 * @brief Bind to a remote event channel and register a callback.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param remote_dom Peer domain that already owns @p remote_port.
 * @param remote_port Event-channel port in @p remote_dom.
 * @param cb Callback invoked when the local channel fires.
 * @param data User data pointer passed to @p cb.
 *
 * @return Local event-channel port on success, negative errno value on failure.
 */
int bind_interdomain_event_channel(domid_t remote_dom, evtchn_port_t remote_port,
				   evtchn_cb_t cb, void *data);

/**
 * @brief Attach a callback to an already allocated local event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port number.
 * @param cb Callback to invoke when the port is signaled.
 * @param data User data pointer passed to @p cb.
 *
 * @retval 0 on success.
 */
int bind_event_channel(evtchn_port_t port, evtchn_cb_t cb, void *data);

/**
 * @brief Remove the callback bound to an event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * The channel remains allocated, but subsequent notifications are recorded as
 * missed events until another callback is registered.
 *
 * @param port Local event-channel port number.
 *
 * @retval 0 on success.
 */
int unbind_event_channel(evtchn_port_t port);

/**
 * @brief Get and clear the missed-event state for an unbound channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port number.
 *
 * @retval 1 At least one event was recorded while no callback was bound.
 * @retval 0 Otherwise.
 */
int get_missed_events(evtchn_port_t port);

/**
 * @brief Mask local delivery for an event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port number.
 *
 * @retval 0 Always returned after updating the shared-info mask bit.
 */
int mask_event_channel(evtchn_port_t port);

/**
 * @brief Unmask local delivery for an event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port number.
 *
 * @retval 0 Always returned after updating the shared-info mask bit.
 */
int unmask_event_channel(evtchn_port_t port);

/**
 * @brief Clear the pending bit for an event channel.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * @param port Local event-channel port number.
 */
void clear_event_channel(evtchn_port_t port);

/**
 * @brief Initialize Xen event handling for the current guest.
 *
 * @kconfig_dep{CONFIG_XEN_EVENTS}
 *
 * This function prepares the per-port callback table and connects the Zephyr
 * ISR to the Xen event interrupt described by devicetree.
 *
 * @return 0 on success, negative errno value on failure.
 * @retval -EINVAL Xen shared-info page is not mapped.
 */
int xen_events_init(void);

/** @} */

#endif /* __XEN_EVENTS_H__ */
