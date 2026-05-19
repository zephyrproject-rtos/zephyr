/*
 * Copyright (C) 2026 Metratec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <inttypes.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(modem_simcom_sim7080_tls, CONFIG_MODEM_LOG_LEVEL);

#include <zephyr/drivers/modem/simcom-sim7080.h>
#include "sim7080.h"

enum sim7080_cert_type {
	SIM7080_CERT_TYPE_ROOT_CA,
	SIM7080_CERT_TYPE_CLIENT_CERT,
	SIM7080_CERT_TYPE_PSK_TABLE,
};

enum sim7080_tls_proto {
	SIM7080_TLS_PROTO_TLS = 1,
	SIM7080_TLS_PROTO_DTLS = 2,
};

static const char * const cert_type_lut[] = {"CACERT", "CERT", "PSKTABLE"};

static int sim7080_set_tls_hostname(struct modem_socket *sock, const char *hostname,
				    net_socklen_t len)
{
	char send_buf[sizeof("AT+CSSLCFG=\"SNI\",#,") + MDM_SSL_SNI_MAX_LEN] = {0};
	struct sim7080_socket_data *sock_data = sock->data;

	if (hostname == NULL) {
		return 0;
	}

	if (len > MDM_SSL_SNI_MAX_LEN) {
		LOG_WRN("Hostname too long");
		return -EINVAL;
	}

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CSSLCFG=\"SNI\",%u,",
			   sock_data->ssl_ctx_idx);
	if (ret < 0) {
		return ret;
	}

	memcpy(send_buf + ret, hostname, len);
	send_buf[ret + len] = '\0';

	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

static int sim7080_set_tls_peer_verify(struct modem_socket *sock, uint32_t val)
{
	struct sim7080_socket_data *sock_data = sock->data;

	if (val == ZSOCK_TLS_PEER_VERIFY_NONE) {
		sock_data->peer_verify = false;
		return 0;
	} else if (val == ZSOCK_TLS_PEER_VERIFY_REQUIRED) {
		sock_data->peer_verify = true;
		return 0;
	}

	return -EINVAL;
}

int sim7080_tls_setsockopt(struct modem_socket *sock, int optname, const void *optval,
			   net_socklen_t optlen)
{
	switch (optname) {
	case ZSOCK_TLS_HOSTNAME:
		return sim7080_set_tls_hostname(sock, optval, optlen);
	case ZSOCK_TLS_PEER_VERIFY:
		if (optlen != sizeof(uint32_t)) {
			return -EINVAL;
		}
		return sim7080_set_tls_peer_verify(sock, *(const uint32_t *)optval);
	default:
		return -ENOTSUP;
	}
}

static uint8_t sim7080_translate_ssl_version(int proto)
{
	switch (proto) {
	case NET_IPPROTO_TLS_1_0:
		return 1;
	case NET_IPPROTO_TLS_1_1:
		return 2;
	case NET_IPPROTO_TLS_1_2:
		return 3;
	case NET_IPPROTO_DTLS_1_0:
		return 4;
	case NET_IPPROTO_DTLS_1_2:
		return 5;
	default:
		return 0;
	}
}

static int sim7080_sock_set_ssl(struct modem_socket *sock, bool on)
{
	char send_buf[sizeof("AT+CASSLCFG=##,\"SSL\",#")] = {0};

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CASSLCFG=%d,\"SSL\",%u", sock->id, on);

	if (ret < 0) {
		return ret;
	}

	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

static int sim7080_sock_set_ssl_version(struct modem_socket *sock, uint8_t version)
{
	char send_buf[sizeof("AT+CSSLCFG=\"SSLVERSION\",#,#")] = {0};

	struct sim7080_socket_data *sock_data = sock->data;

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CSSLCFG=\"SSLVERSION\",%u,%u",
			   sock_data->ssl_ctx_idx, version);
	if (ret < 0) {
		return ret;
	}

	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

static int sim7080_sock_set_ssl_proto(struct modem_socket *sock, enum sim7080_tls_proto proto)
{
	char send_buf[sizeof("AT+CSSLCFG=\"PROTOCOL\",#,#")] = {0};

	struct sim7080_socket_data *sock_data = sock->data;

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CSSLCFG=\"PROTOCOL\",%u,%u",
			   sock_data->ssl_ctx_idx, proto);
	if (ret < 0) {
		return ret;
	}

	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

static int sim7080_sock_set_ssl_context(struct modem_socket *sock)
{
	char send_buf[sizeof("AT+CASSLCFG=#,\"CRINDEX\",#")] = {0};

	struct sim7080_socket_data *sock_data = sock->data;

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CASSLCFG=%d,\"CRINDEX\",%u", sock->id,
			   sock_data->ssl_ctx_idx);
	if (ret < 0) {
		return ret;
	}

	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

static int sim7080_sock_set_tls_cert(struct modem_socket *sock, enum sim7080_cert_type type,
				     const char *name)
{
	char send_buf[sizeof("AT+CASSLCFG=##,\"########\",\"\"") + MDM_SSL_CERT_NAME_MAX_LEN] = {0};

	if (type > SIM7080_CERT_TYPE_PSK_TABLE) {
		return -EINVAL;
	}

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CASSLCFG=%d,\"%s\",\"%s\"", sock->id,
			   cert_type_lut[type], name);
	if (ret < 0) {
		return ret;
	}

	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

int sim7080_handle_ca_tls(struct modem_socket *sock)
{
	uint8_t proto = sim7080_translate_ssl_version(sock->ip_proto);

	if (proto == 0) {
		return sim7080_sock_set_ssl(sock, false);
	}

	struct sim7080_socket_data *sock_data = sock->data;

	if (strlen(sock_data->root_ca_dtls) == 0) {
		LOG_ERR("No root certificate given");
		return -EINVAL;
	}

	if (sock_data->peer_verify && strlen(sock_data->client_cert) == 0) {
		LOG_ERR("No client certificate given");
		return -EINVAL;
	}

	int ret = sim7080_sock_set_ssl_version(sock, proto);

	if (ret != 0) {
		LOG_ERR("Failed to set ssl version");
		return ret;
	}

	ret = sim7080_sock_set_ssl(sock, true);
	if (ret != 0) {
		LOG_ERR("Failed to enable ssl for socket");
		return ret;
	}

	ret = sim7080_sock_set_ssl_context(sock);
	if (ret != 0) {
		LOG_ERR("Failed to set ssl context");
		return ret;
	}

	/* Perform DTLS sequence */
	if (sock->ip_proto == NET_IPPROTO_DTLS_1_0 || sock->ip_proto == NET_IPPROTO_DTLS_1_2) {
		ret = sim7080_sock_set_ssl_proto(sock, SIM7080_TLS_PROTO_DTLS);
		if (ret != 0) {
			LOG_ERR("Failed to set DTLS protocol");
			return ret;
		}

		return sim7080_sock_set_tls_cert(sock, SIM7080_CERT_TYPE_PSK_TABLE,
						 sock_data->root_ca_dtls);
	}

	/* Perform TLS sequence */
	ret = sim7080_sock_set_ssl_proto(sock, SIM7080_TLS_PROTO_TLS);
	if (ret != 0) {
		LOG_ERR("Failed to set TLS protocol");
		return ret;
	}

	ret = sim7080_sock_set_tls_cert(sock, SIM7080_CERT_TYPE_ROOT_CA, sock_data->root_ca_dtls);
	if (ret != 0) {
		LOG_ERR("Failed to set root certificate name");
		return ret;
	}

	if (sock_data->peer_verify) {
		return sim7080_sock_set_tls_cert(sock, SIM7080_CERT_TYPE_CLIENT_CERT,
						 sock_data->client_cert);
	}

	return 0;
}

static int sim7080_fs_init(void)
{
	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, "AT+CFSINIT",
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

static int sim7080_fs_upload(const char *name, sim7080_tls_cert_read_func read_func, size_t len,
			     uint32_t timeout)
{
	char send_buf[sizeof("AT+CFSWFILE=#,,#,######,######") + MDM_SSL_CERT_NAME_MAX_LEN] = {0};

	if (strlen(name) > MDM_SSL_CERT_NAME_MAX_LEN) {
		LOG_ERR("Cert Name too long");
		return -EINVAL;
	}

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CFSWFILE=3,%s,0,%u,%u", name, len,
			   timeout);
	if (ret < 0) {
		LOG_ERR("Could not format cert name");
		return ret;
	}

	ret = modem_cmd_send_nolock(&mctx.iface, &mctx.cmd_handler, NULL, 0U, send_buf, NULL,
				    K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to send write command");
		return ret;
	}

	/* Wait for the DOWNLOAD urc */
	ret = k_sem_take(&mdata.fs_sem, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("No DOWNLOAD received");
		return ret;
	}

	k_sem_reset(&mdata.sem_response);

	size_t written = 0;

	while (written < len) {
		size_t to_write = len - written;

		if (to_write > sizeof(mdata.tls_cert_buf)) {
			to_write = sizeof(mdata.tls_cert_buf);
		}

		int n = read_func(mdata.tls_cert_buf, to_write, written);

		if (n < 0) {
			LOG_ERR("Read function returned error");
			return ret;
		}

		LOG_DBG("req=%zu,prov=%d", to_write, n);

		ret = modem_cmd_send_data_nolock(&mctx.iface, mdata.tls_cert_buf, n);
		if (ret < 0) {
			return ret;
		}

		written += n;
	}

	/* Wait for the OK */
	ret = k_sem_take(&mdata.sem_response, MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("No status received");
		return ret;
	}

	if (modem_cmd_handler_get_error(&mdata.cmd_handler_data) != 0) {
		LOG_ERR("Upload failed with error");
		return ret;
	}

	return 0;
}

static int sim7080_fs_delete(const char *name)
{
	char send_buf[sizeof("AT+CFSDFILE=#,") + MDM_SSL_CERT_NAME_MAX_LEN] = {0};

	if (strlen(name) > MDM_SSL_CERT_NAME_MAX_LEN) {
		return -EINVAL;
	}

	int ret = snprintk(send_buf, sizeof(send_buf), "AT+CFSDFILE=3,%s", name);

	if (ret < 0) {
		return ret;
	}

	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf,
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

static int sim7080_fs_close(void)
{
	return modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, "AT+CFSTERM",
			      &mdata.sem_response, MDM_CMD_TIMEOUT);
}

int mdm_sim7080_import_root_ca(const char *cert_name, sim7080_tls_cert_read_func read_func,
			       size_t cert_len)
{
	char send_buf[sizeof("AT+CSSLCFG=\"CONVERT\",#,") + MDM_SSL_CERT_NAME_MAX_LEN] = {0};

	if (!cert_name || !read_func || cert_len == 0) {
		return -EINVAL;
	}

	int ret = sim7080_fs_init();

	if (ret != 0) {
		LOG_ERR("Failed to init file system");
		return ret;
	}

	ret = sim7080_fs_upload(cert_name, read_func, cert_len, 10000);
	if (ret != 0) {
		LOG_ERR("Root CA upload failed");
		(void)sim7080_fs_close();
		return ret;
	}

	ret = snprintk(send_buf, sizeof(send_buf), "AT+CSSLCFG=\"CONVERT\",2,%s", cert_name);
	if (ret < 0) {
		(void)sim7080_fs_delete(cert_name);
		(void)sim7080_fs_close();
		return ret;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Root CA conversion failed");
		(void)sim7080_fs_delete(cert_name);
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_delete(cert_name);
	if (ret != 0) {
		LOG_ERR("Failed to delete root ca file");
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_close();
	if (ret != 0) {
		LOG_ERR("Failed to close file system");
		return ret;
	}

	return 0;
}

int mdm_sim7080_import_client_cert(const char *cert_name, sim7080_tls_cert_read_func cert_read_func,
				   size_t cert_len, const char *key_name,
				   sim7080_tls_cert_read_func key_read_func, size_t key_len,
				   const char *passwd)
{
	char send_buf[sizeof("AT+CSSLCFG=\"CONVERT\",#,,,\"\"") + 3 * MDM_SSL_CERT_NAME_MAX_LEN] = {
		0};

	if (!cert_name || !cert_read_func || cert_len == 0 || !key_name || !key_read_func ||
	    key_len == 0) {
		return -EINVAL;
	}

	int ret = sim7080_fs_init();

	if (ret != 0) {
		LOG_ERR("Failed to init file system");
		return ret;
	}

	ret = sim7080_fs_upload(cert_name, cert_read_func, cert_len, 10000);
	if (ret != 0) {
		LOG_ERR("Client cert upload failed");
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_upload(key_name, key_read_func, key_len, 10000);
	if (ret != 0) {
		LOG_ERR("Client key upload failed");
		(void)sim7080_fs_close();
		return ret;
	}

	if (passwd) {
		ret = snprintk(send_buf, sizeof(send_buf), "AT+CSSLCFG=\"CONVERT\",1,%s,%s,\"%s\"",
			       cert_name, key_name, passwd);
	} else {
		ret = snprintk(send_buf, sizeof(send_buf), "AT+CSSLCFG=\"CONVERT\",1,%s,%s",
			       cert_name, key_name);
	}

	if (ret < 0) {
		(void)sim7080_fs_delete(cert_name);
		(void)sim7080_fs_close();
		return ret;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("Client certificate conversion failed");
		(void)sim7080_fs_delete(cert_name);
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_delete(cert_name);
	if (ret != 0) {
		LOG_ERR("Failed to delete client certificate file");
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_delete(key_name);
	if (ret != 0) {
		LOG_ERR("Failed to delete client key file");
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_close();
	if (ret != 0) {
		LOG_ERR("Failed to close file system");
		return ret;
	}

	return 0;
}

int mdm_sim7080_import_dtls_psk(const char *psk_name, sim7080_tls_cert_read_func read_func,
				size_t psk_len)
{
	char send_buf[sizeof("AT+CSSLCFG=\"CONVERT\",#,") + MDM_SSL_CERT_NAME_MAX_LEN] = {0};

	if (!psk_name || !read_func || psk_len == 0) {
		return -EINVAL;
	}

	int ret = sim7080_fs_init();

	if (ret != 0) {
		LOG_ERR("Failed to init file system");
		return ret;
	}

	ret = sim7080_fs_upload(psk_name, read_func, psk_len, 10000);
	if (ret != 0) {
		LOG_ERR("PSK upload failed");
		(void)sim7080_fs_close();
		return ret;
	}

	ret = snprintk(send_buf, sizeof(send_buf), "AT+CSSLCFG=\"CONVERT\",3,%s", psk_name);
	if (ret < 0) {
		(void)sim7080_fs_delete(psk_name);
		(void)sim7080_fs_close();
		return ret;
	}

	ret = modem_cmd_send(&mctx.iface, &mctx.cmd_handler, NULL, 0, send_buf, &mdata.sem_response,
			     MDM_CMD_TIMEOUT);
	if (ret < 0) {
		LOG_ERR("PSK conversion failed");
		(void)sim7080_fs_delete(psk_name);
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_delete(psk_name);
	if (ret != 0) {
		LOG_ERR("Failed to delete psk file");
		(void)sim7080_fs_close();
		return ret;
	}

	ret = sim7080_fs_close();
	if (ret != 0) {
		LOG_ERR("Failed to close file system");
		return ret;
	}

	return 0;
}

int mdm_sim7080_configure_tls_certs(int fd, const char *root_ca, const char *client_cert)
{
	struct modem_socket *sock = modem_socket_from_fd(&mdata.socket_config, fd);

	if (!sock) {
		LOG_ERR("No socket allocated with fd %d", fd);
		return -ENOENT;
	}

	if (sock->type != NET_SOCK_STREAM) {
		LOG_ERR("Cannot configure tls certs for non tcp sockets");
		return -EINVAL;
	}

	if (root_ca == NULL) {
		LOG_ERR("No root ca name provided");
		return -EINVAL;
	}

	if (strlen(root_ca) > MDM_SSL_CERT_NAME_MAX_LEN) {
		LOG_ERR("Root ca name exceeds maximum length");
		return -EINVAL;
	}

	if (client_cert && strlen(client_cert) > MDM_SSL_CERT_NAME_MAX_LEN) {
		LOG_ERR("Client cert name exceeds maximum length");
		return -EINVAL;
	}

	struct sim7080_socket_data *data = sock->data;

	memset(data->root_ca_dtls, 0, sizeof(data->root_ca_dtls));
	memset(data->client_cert, 0, sizeof(data->client_cert));

	strncpy(data->root_ca_dtls, root_ca, sizeof(data->root_ca_dtls));

	if (client_cert) {
		strncpy(data->client_cert, client_cert, sizeof(data->client_cert));
	}

	return 0;
}

int mdm_sim7080_configure_dtls_psktable(int fd, const char *psktable)
{
	struct modem_socket *sock = modem_socket_from_fd(&mdata.socket_config, fd);

	if (!sock) {
		LOG_ERR("No socket allocated with fd %d", fd);
		return -ENOENT;
	}

	if (sock->type != NET_SOCK_DGRAM) {
		LOG_ERR("Cannot configure tls certs for non tcp sockets");
		return -EINVAL;
	}

	if (psktable == NULL) {
		LOG_ERR("No root ca name provided");
		return -EINVAL;
	}

	if (strlen(psktable) > MDM_SSL_CERT_NAME_MAX_LEN) {
		LOG_ERR("Root ca name exceeds maximum length");
		return -EINVAL;
	}

	struct sim7080_socket_data *data = sock->data;

	memset(data->root_ca_dtls, 0, sizeof(data->root_ca_dtls));

	strncpy(data->root_ca_dtls, psktable, sizeof(data->root_ca_dtls));

	return 0;
}
