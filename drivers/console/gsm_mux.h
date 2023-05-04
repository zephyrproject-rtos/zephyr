/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DLCI_CONTROL 0

#if defined(CONFIG_GSM_MUX)
#define DLCI_AT      CONFIG_GSM_MUX_DLCI_AT
#define DLCI_PPP     CONFIG_GSM_MUX_DLCI_PPP
#else
#define DLCI_AT      -1
#define DLCI_PPP     -1
#endif

struct gsm_mux;
struct gsm_dlci;

void gsm_mux_recv_buf(struct gsm_mux *mux, uint8_t *buf, int len);
int gsm_mux_send(struct gsm_mux *mux, uint8_t dlci_address,
		 const uint8_t *buf, size_t size);
struct gsm_mux *gsm_mux_create(const struct device *uart);
int gsm_mux_disconnect(struct gsm_mux *mux, k_timeout_t timeout);
void gsm_mux_init(void);

typedef void (*gsm_mux_dlci_created_cb_t)(struct gsm_dlci *dlci,
					  bool connected,
					  void *user_data);

int gsm_dlci_create(struct gsm_mux *mux,
		    const struct device *uart,
		    int dlci_address,
		    gsm_mux_dlci_created_cb_t dlci_created_cb,
		    void *user_data,
		    struct gsm_dlci **dlci);
int gsm_dlci_send(struct gsm_dlci *dlci, const uint8_t *buf, size_t size);
int gsm_dlci_id(struct gsm_dlci *dlci);
void gsm_mux_detach(struct gsm_mux *mux);
