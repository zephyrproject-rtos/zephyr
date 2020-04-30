/*
 * Copyright (c) 2018-2020 O.S.Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>

LOG_MODULE_REGISTER(updatehub);

#include <zephyr.h>

#include <logging/log_ctrl.h>
#include <net/socket.h>
#include <net/net_mgmt.h>
#include <net/net_ip.h>
#include <net/udp.h>
#include <net/coap.h>
#include <net/dns_resolve.h>
#include <drivers/flash.h>
#include <power/reboot.h>
#include <tinycrypt/sha256.h>
#include <data/json.h>

#include "include/updatehub.h"
#include "updatehub_priv.h"
#include "updatehub_firmware.h"
#include "updatehub_device.h"

#if defined(CONFIG_UPDATEHUB_DTLS)
#define CA_CERTIFICATE_TAG 1
#include <net/tls_credentials.h>
#endif



#define NETWORK_TIMEOUT K_SECONDS(2)
#define UPDATEHUB_POLL_INTERVAL K_MINUTES(CONFIG_UPDATEHUB_POLL_INTERVAL)
#define MAX_PATH_SIZE 255
/* MAX_PAYLOAD_SIZE must reflect size COAP_BLOCK_x option */
#define MAX_PAYLOAD_SIZE 1088	//1024
/* MAX_DOWNLOAD_DATA must be equal or bigger than:
 * MAX_PAYLOAD_SIZE + (len + header + options)
 * otherwise download size will be less than real size.
 */
#define MAX_DOWNLOAD_DATA (MAX_PAYLOAD_SIZE + 32)
#define COAP_MAX_RETRY 5			// origionally it is 3
#define MAX_IP_SIZE 30

#if defined(CONFIG_UPDATEHUB_CE)
#define UPDATEHUB_SERVER CONFIG_UPDATEHUB_SERVER
#else
#define UPDATEHUB_SERVER "coap.updatehub.io"
#endif

#define GET_NUM_2(v) ((v) >> 4)

int counter = 0;


static struct updatehub_context {
	struct coap_block_context block;
	struct k_sem semaphore;
	struct flash_img_context flash_ctx;
	struct tc_sha256_state_struct sha256sum;
	enum updatehub_response code_status;
	u8_t uri_path[MAX_PATH_SIZE];
	u8_t payload[MAX_PAYLOAD_SIZE];
	int downloaded_size;
	struct pollfd fds[1];
	int sock;
	int nfds;
} ctx;

static struct update_info {
	char package_uid[TC_SHA256_BLOCK_SIZE + 1];
	char sha256sum_image[TC_SHA256_BLOCK_SIZE + 1];
	int image_size;
} update_info;

static struct k_delayed_work updatehub_work_handle;

static void wait_fds(void)
{
	if (poll(ctx.fds, ctx.nfds, NETWORK_TIMEOUT) < 0) {
		LOG_ERR("Error in poll");
	}
}

static int metadata_hash_get(char *metadata)
{
	struct tc_sha256_state_struct sha256sum;
	unsigned char hash[TC_SHA256_DIGEST_SIZE]= {0};
	char buffer[3];
	int buffer_len = 0;

	if (tc_sha256_init(&sha256sum) == 0) {
		return -1;
	}

	if (tc_sha256_update(&sha256sum, metadata, strlen(metadata)) == 0) {
		return -1;
	}

	if (tc_sha256_final(hash, &sha256sum) == 0) {
		return -1;
	}

	memset(update_info.package_uid, 0, TC_SHA256_BLOCK_SIZE + 1);
	for (int i = 0; i < TC_SHA256_DIGEST_SIZE; i++) {
		snprintk(buffer, sizeof(buffer), "%02x",
			 hash[i]);
		buffer_len = buffer_len + strlen(buffer);
		strncat(&update_info.package_uid[i], buffer,
			MIN(TC_SHA256_BLOCK_SIZE, buffer_len));
	}
	bin2hex(hash, TC_SHA256_DIGEST_SIZE,
		update_info.package_uid, TC_SHA256_BLOCK_SIZE);
	return 0;
}

static bool
is_compatible_hardware(struct resp_probe_some_boards *metadata_some_boards)
{
	int i;

	for (i = 0; i < metadata_some_boards->supported_hardware_len; i++) {
		if (strncmp(metadata_some_boards->supported_hardware[i],
			    CONFIG_BOARD, strlen(CONFIG_BOARD)) == 0) {
			return true;
		}
	}
	return false;
}

static void prepare_fds(void)
{
	ctx.fds[ctx.nfds].fd = ctx.sock;
	ctx.fds[ctx.nfds].events = POLLIN;
	ctx.nfds++;
}

static bool start_coap_client(void)
{
	int ret = -1;

#if defined(CONFIG_UPDATEHUB_DTLS)
	int verify = TLS_PEER_VERIFY_NONE;
	sec_tag_t sec_list[] = { CA_CERTIFICATE_TAG };
	int protocol = IPPROTO_DTLS_1_2;
	char port[] = "5684";
#else
	int protocol = IPPROTO_UDP;
	char port[] = "5683";
#endif	
	ctx.sock = socket(AF_INET, SOCK_DGRAM, protocol); // enable IPv4
	if (ctx.sock < 0) {
		LOG_ERR("Failed to create UDP socket");
		return false;
	}

#if defined(CONFIG_UPDATEHUB_DTLS)
	if (setsockopt(ctx.sock, SOL_TLS, TLS_SEC_TAG_LIST,
		       sec_list, sizeof(sec_list)) < 0) {
		LOG_ERR("Failed to set TLS_TAG option");
		return false;
	}

	if (setsockopt(ctx.sock, SOL_TLS, TLS_PEER_VERIFY, &verify, sizeof(int)) < 0) {
		LOG_ERR("Failed to set TLS_PEER_VERIFY option");
		return false;
	}
#endif
	
	
	struct sockaddr ai_addr1;
	
	ai_addr1.data[0] = 22;  // fixed byte for IPv4
	ai_addr1.data[1] = 51;  // fixed byte IPv4
	char string[] = UPDATEHUB_SERVER;
	const char delimiters[] = ".";
	char *token, *cp;
	
	
	cp = strdup (string);                // Make writable copy.  
	token = strtok (cp, delimiters);      // token => "first byte" 
	LOG_INF("%d \n",atoi(token));
	ai_addr1.data[2] = atoi(token); 
	token = strtok (NULL, delimiters);    // token => "2nd byte" 
	LOG_INF("%d \n",atoi(token));
	ai_addr1.data[3] = atoi(token); 
	token = strtok (NULL, delimiters);    // token => "3rd byte" *
	LOG_INF("%d \n",atoi(token));
	ai_addr1.data[4] = atoi(token); 
	token = strtok (NULL, delimiters);    // token => "4th byte" 
	LOG_INF("%d \n",atoi(token));
	ai_addr1.data[5] = atoi(token);   

	ai_addr1.sa_family = AF_INET;

	
	if (connect(ctx.sock, &ai_addr1, 8) < 0) {			//length is valid for IPv4
		LOG_ERR("Cannot connect to UDP remote");
		return false;
	}

	prepare_fds();
	return true;
}

static void cleanup_connection(void)
{
	if (close(ctx.sock) < 0) {
		LOG_ERR("Could not close the socket");
	}

	memset(&ctx.fds[1], 0, sizeof(ctx.fds[1]));
	ctx.nfds = 0;
	ctx.sock = 0;
}


static int send_request(enum coap_msgtype msgtype, enum coap_method method,
			enum updatehub_uri_path type)
{
	struct coap_packet request_packet;
	int ret = -1;
	u8_t content_application_json = 50;
	u8_t data[MAX_PAYLOAD_SIZE];

	ret = coap_packet_init(&request_packet, data, MAX_PAYLOAD_SIZE, 1,
			       COAP_TYPE_CON, 8, coap_next_token(), method,
			       coap_next_id());
	if (ret < 0) {
		LOG_ERR("Could not init packet");
		return ret;
	}

	switch (method) {
	case COAP_METHOD_GET:
		snprintk(ctx.uri_path, MAX_PATH_SIZE,
			 "%s/%s/packages/%s/objects/%s", uri_path(type),
			 CONFIG_UPDATEHUB_PRODUCT_UID, update_info.package_uid,
			 update_info.sha256sum_image);

		ret = coap_packet_append_option(&request_packet,
						COAP_OPTION_URI_PATH,
						ctx.uri_path,
						strlen(ctx.uri_path));
		if (ret < 0) {
			LOG_ERR("Unable add option to request path");

			return ret;
		}

		ret = coap_append_block2_option(&request_packet,
						&ctx.block);
		if (ret < 0) {
			LOG_ERR("Unable coap append block 2");

			return ret;
		}

		ret = coap_packet_append_option(&request_packet, 2048,
						UPDATEHUB_API_HEADER, strlen(UPDATEHUB_API_HEADER));
		if (ret < 0) {
			LOG_ERR("Unable add option to add updatehub header");

			return ret;
		}

		break;

	case COAP_METHOD_POST:
		ret = coap_packet_append_option(&request_packet,
						COAP_OPTION_URI_PATH,
						uri_path(type),
						strlen(uri_path(type)));
		if (ret < 0) {
			LOG_ERR("Unable add option to request path");

			return ret;
		}

		ret = coap_packet_append_option(&request_packet,
						COAP_OPTION_CONTENT_FORMAT,
						&content_application_json,
						sizeof(content_application_json));
		if (ret < 0) {
			LOG_ERR("Unable add option to request format");

			return ret;
		}

		ret = coap_packet_append_option(&request_packet, 2048,
						UPDATEHUB_API_HEADER, strlen(UPDATEHUB_API_HEADER));
		if (ret < 0) {
			LOG_ERR("Unable add option to add updatehub header");

			return ret;
		}

		ret = coap_packet_append_payload_marker(&request_packet);
		if (ret < 0) {
			LOG_ERR("Unable to append payload marker");

			return ret;
		}

		ret = coap_packet_append_payload(&request_packet,
						 ctx.payload,
						 strlen(ctx.payload));
		if (ret < 0) {
			LOG_ERR("Not able to append payload");

			return ret;
		}

		break;

	default:
		LOG_ERR("Invalid method");
		ret = -1;

	}

	ret = send(ctx.sock, request_packet.data, request_packet.offset, 0);
	if (ret < 0) {
		LOG_ERR("Could not send request");

	}

	return ret;
}

static bool install_update_cb_sha256(void)
{
	u8_t image_hash[TC_SHA256_DIGEST_SIZE];
	char buffer[3], sha256_image_dowloaded[TC_SHA256_BLOCK_SIZE + 1];
	int i, buffer_len = 0;

	if (tc_sha256_final(image_hash, &ctx.sha256sum) < 1) {
		LOG_ERR("Could not finish sha256sum");
		return false;
	}

	memset(&sha256_image_dowloaded, 0, TC_SHA256_BLOCK_SIZE + 1);
	for (i = 0; i < TC_SHA256_DIGEST_SIZE; i++) {
		snprintk(buffer, sizeof(buffer), "%02x", image_hash[i]);
		buffer_len = buffer_len + strlen(buffer);
		strncat(&sha256_image_dowloaded[i], buffer,
			MIN(TC_SHA256_BLOCK_SIZE, buffer_len));
	}
	LOG_INF("comparison, sha256_image_dowloaded: %s, update_info.sha256sum_image: %d",sha256_image_dowloaded, strlen(update_info.sha256sum_image));
	if (strncmp(sha256_image_dowloaded,
		update_info.sha256sum_image,
		strlen(update_info.sha256sum_image)) != 0) {
		LOG_ERR("SHA256SUM of image are not the same");
		ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
		return false;
	}

	return true;
}

static void install_update_cb(void)
{
	struct coap_packet response_packet;
	u8_t *data = k_malloc(MAX_DOWNLOAD_DATA);
	int rcvd = -1;

	if (data == NULL) {
		LOG_ERR("Could not alloc data memory");
		ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
		goto cleanup;
	}

	wait_fds();

	rcvd = recv(ctx.sock, data, MAX_DOWNLOAD_DATA, MSG_DONTWAIT);
	if (rcvd <= 0) {
		ctx.code_status = UPDATEHUB_NETWORKING_ERROR;
		LOG_ERR("install_update_cb:(): Could not receive data: %d",ctx.code_status);
		goto cleanup;
	}
	
	if (coap_packet_parse(&response_packet, data, rcvd, NULL, 0) < 0) {
		LOG_ERR("Invalid data received");
		ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
		goto cleanup;
	}
	
	ctx.downloaded_size = ctx.downloaded_size +
			      (response_packet.max_len - response_packet.offset);

	if (tc_sha256_update(&ctx.sha256sum,
			     response_packet.data + response_packet.offset,
			     response_packet.max_len - response_packet.offset) < 1) {
		LOG_ERR("Could not update sha256sum");
		ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
		goto cleanup;
	}

	if (flash_img_buffered_write(&ctx.flash_ctx,
				     response_packet.data + response_packet.offset,
				     response_packet.max_len - response_packet.offset,
				     ctx.downloaded_size == ctx.block.total_size) < 0) {
		LOG_ERR("Error to write on the flash");
		ctx.code_status = UPDATEHUB_INSTALL_ERROR;
		goto cleanup;
	}

	if (coap_update_from_block(&response_packet, &ctx.block) < 0) {
		ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
		goto cleanup;
	}

	if (coap_next_block(&response_packet, &ctx.block) == 0) {
		LOG_INF("install_update_cb(): ctx.downloaded_size: %d and ctx.block.total_size: %d",ctx.downloaded_size,ctx.block.total_size);
		if (ctx.downloaded_size != ctx.block.total_size) {
			LOG_ERR("Could not get the next coap block");
			ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
			goto cleanup;
		}

		LOG_INF("Firmware downloaded successfully");
		if (!install_update_cb_sha256()) {
			LOG_ERR("Firmware validation has failed");
			ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
			goto cleanup;
		}
	}

	ctx.code_status = UPDATEHUB_OK;

cleanup:

	k_free(data);
}


static enum updatehub_response install_update(void)
{
	int verification_download = 0;
	int attempts_download = 0;

	if (boot_erase_img_bank(DT_FLASH_AREA_IMAGE_1_ID) != 0) {
		LOG_ERR("Failed to init flash and erase second slot");
		ctx.code_status = UPDATEHUB_FLASH_INIT_ERROR;
		goto error;
	}

	if (tc_sha256_init(&ctx.sha256sum) < 1) {
		LOG_ERR("Could not start sha256sum");
		ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
		goto error;
	}

	if (!start_coap_client()) {
		ctx.code_status = UPDATEHUB_NETWORKING_ERROR;
		LOG_ERR("install_update(): start_coap_client could not start successfully");
		goto error;
	}

	if (coap_block_transfer_init(&ctx.block, COAP_BLOCK_1024,
				     update_info.image_size) < 0) {
		LOG_ERR("Unable init block transfer");
		ctx.code_status = UPDATEHUB_NETWORKING_ERROR;
		goto cleanup;
	}

	flash_img_init(&ctx.flash_ctx);

	ctx.downloaded_size = 0;

	while (ctx.downloaded_size != ctx.block.total_size) {
		verification_download = ctx.downloaded_size;

		if (send_request(COAP_TYPE_CON, COAP_METHOD_GET,
				 UPDATEHUB_DOWNLOAD) < 0) {
			ctx.code_status = UPDATEHUB_NETWORKING_ERROR;
			goto cleanup;
		}

		install_update_cb();		
	
		if(ctx.code_status == UPDATEHUB_OK){	
			attempts_download = 0;
			goto received_reply;
		}
		if(ctx.code_status == UPDATEHUB_DOWNLOAD_ERROR){	
			LOG_ERR("install_update(): download is not successful \n");
			goto cleanup;
		}

received_reply:
	
		if (verification_download == ctx.downloaded_size) {
			if (attempts_download == COAP_MAX_RETRY) {
				LOG_ERR("Could not get the packet");
				ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
				goto cleanup;
			}
			attempts_download++;
		}
	}

cleanup:
	cleanup_connection();

error:
	ctx.downloaded_size = 0;

	return ctx.code_status;
}


static int report(enum updatehub_state state)
{
	struct report report;
	int ret = -1;
	const char *exec = state_name(state);
	char *device_id = k_malloc(DEVICE_ID_BIN_MAX_SIZE);
	char *firmware_version = k_malloc(BOOT_IMG_VER_STRLEN_MAX);
	
	size_t sha256size;	

	if (device_id == NULL || firmware_version == NULL) {
		LOG_ERR("Could not alloc device_id or firmware_version memory");
		goto error;
	}

	if (!updatehub_get_device_identity(device_id, DEVICE_ID_BIN_MAX_SIZE)) {
		goto error;
	}

	if (!updatehub_get_firmware_version(firmware_version, BOOT_IMG_VER_STRLEN_MAX)) {
		goto error;
	}

	memset(&report, 0, sizeof(report));
	report.product_uid = CONFIG_UPDATEHUB_PRODUCT_UID;
	report.device_identity.id = device_id;
	report.version = firmware_version;
	report.hardware = CONFIG_BOARD;
	report.status = exec;
	report.package_uid = update_info.package_uid;

	switch (ctx.code_status) {
	case UPDATEHUB_INSTALL_ERROR:
		report.previous_state =
			state_name(UPDATEHUB_STATE_INSTALLING);
		break;
	case UPDATEHUB_DOWNLOAD_ERROR:
		report.previous_state =
			state_name(UPDATEHUB_STATE_DOWNLOADING);
		break;
	case UPDATEHUB_FLASH_INIT_ERROR:
		report.previous_state =
			state_name(UPDATEHUB_FLASH_INIT_ERROR);
		break;
	default:
		report.previous_state = "";
		break;
	}

	if (strncmp(report.previous_state, "", sizeof("") - 1) != 0) {
		report.error_message = updatehub_response(ctx.code_status);
	} else {
		report.error_message = "";
	}

	memset(&ctx.payload, 0, MAX_PAYLOAD_SIZE);
	ret = json_obj_encode_buf(send_report_descr,
				  ARRAY_SIZE(send_report_descr),
				  &report, ctx.payload,
				  MAX_PAYLOAD_SIZE - 1);
	if (ret < 0) {
		LOG_ERR("Could not encode metadata");
		goto error;
	}

	if (!start_coap_client()) {
		goto error;
	}

	ret = send_request(COAP_TYPE_NON_CON, COAP_METHOD_POST,
			   UPDATEHUB_REPORT);
	if (ret < 0) {
		goto cleanup;
	}

	wait_fds();

cleanup:
	cleanup_connection();

error:
	k_free(firmware_version);
	k_free(device_id);
	return ret;
}

static void probe_cb(char *metadata)
{
	struct coap_packet reply;
	char tmp[MAX_PAYLOAD_SIZE];
	int rcvd = -1;

	wait_fds();

	rcvd = recv(ctx.sock, metadata, MAX_PAYLOAD_SIZE, MSG_DONTWAIT);
	if (rcvd <= 0) {
		LOG_ERR("Could not receive data");
		ctx.code_status = UPDATEHUB_NETWORKING_ERROR;
		return;
	}

	if (coap_packet_parse(&reply, metadata, rcvd, NULL, 0) < 0) {
		LOG_ERR("Invalid data received");
		ctx.code_status = UPDATEHUB_DOWNLOAD_ERROR;
		return;
	}

	if (COAP_RESPONSE_CODE_NOT_FOUND == coap_header_get_code(&reply)) {
		LOG_INF("No update available");
		ctx.code_status = UPDATEHUB_NO_UPDATE;
		return;
	}

	memset(&tmp, 0, MAX_PAYLOAD_SIZE);
	memcpy(tmp, reply.data + reply.offset, reply.max_len - reply.offset);
	memset(metadata, 0, MAX_PAYLOAD_SIZE);
	memcpy(metadata, tmp, strlen(tmp));

	ctx.code_status = UPDATEHUB_OK;

	LOG_INF("Probe metadata received"); 
}


enum updatehub_response updatehub_probe(void)
{
	struct probe request;
	struct resp_probe_some_boards metadata_some_boards;
	struct resp_probe_any_boards metadata_any_boards;

	char *metadata = k_malloc(MAX_PAYLOAD_SIZE);
	char *metadata_copy = k_malloc(MAX_PAYLOAD_SIZE);
	char *device_id = k_malloc(DEVICE_ID_BIN_MAX_SIZE);
	char *firmware_version = k_malloc(BOOT_IMG_VER_STRLEN_MAX);

	if (device_id == NULL || firmware_version == NULL ||
	    metadata == NULL || metadata_copy == NULL) {
		LOG_ERR("Could not alloc probe memory");
		ctx.code_status = UPDATEHUB_METADATA_ERROR;
		goto error;
	}

	if (!boot_is_img_confirmed()) {
		LOG_ERR("The current image is not confirmed");
		ctx.code_status = UPDATEHUB_UNCONFIRMED_IMAGE;
		goto error;
	}

	if (!updatehub_get_firmware_version(firmware_version, BOOT_IMG_VER_STRLEN_MAX)) {
		ctx.code_status = UPDATEHUB_METADATA_ERROR;
		goto error;
	}

	if (!updatehub_get_device_identity(device_id, DEVICE_ID_BIN_MAX_SIZE)) {
		ctx.code_status = UPDATEHUB_METADATA_ERROR;
		goto error;
	}

	memset(&request, 0, sizeof(request));
	request.product_uid = CONFIG_UPDATEHUB_PRODUCT_UID;
	request.device_identity.id = device_id;
	request.version = firmware_version;
	request.hardware = CONFIG_BOARD;

	memset(&ctx.payload, 0, MAX_PAYLOAD_SIZE);
	if (json_obj_encode_buf(send_probe_descr,
				ARRAY_SIZE(send_probe_descr),
				&request, ctx.payload,
				MAX_PAYLOAD_SIZE - 1) < 0) {
		LOG_ERR("Could not encode metadata");
		ctx.code_status = UPDATEHUB_METADATA_ERROR;
		goto error;
	}

	if (!start_coap_client()) {
		ctx.code_status = UPDATEHUB_NETWORKING_ERROR;
		goto error;
	}
	uint8_t i = 0;
	bool while_condition = true;

	while(while_condition && i < COAP_MAX_RETRY){	

		if (send_request(COAP_TYPE_CON, COAP_METHOD_POST, UPDATEHUB_PROBE) < 0) {
			ctx.code_status = UPDATEHUB_NETWORKING_ERROR;
			while_condition = false; 
			break;
		}

		memset(metadata, 0, MAX_PAYLOAD_SIZE);
		probe_cb(metadata);
	
		if (ctx.code_status == UPDATEHUB_OK) {
			LOG_INF("received packet# %d and update found",i);	
			while_condition = false;
			break;
		}
		
		i++;
	}


	if (ctx.code_status != UPDATEHUB_OK) {
		goto cleanup;
	}


	
	memset(&update_info, 0, sizeof(update_info));
	if (metadata_hash_get(metadata) < 0) {
		LOG_ERR("Could not get metadata hash");
		ctx.code_status = UPDATEHUB_METADATA_ERROR;
		goto cleanup;
	}

	memcpy(metadata_copy, metadata, strlen(metadata));
	if (json_obj_parse(metadata, strlen(metadata),
			   recv_probe_sh_array_descr,
			   ARRAY_SIZE(recv_probe_sh_array_descr),
			   &metadata_some_boards) < 0) {

		if (json_obj_parse(metadata_copy, strlen(metadata_copy),
				   recv_probe_sh_string_descr,
				
   ARRAY_SIZE(recv_probe_sh_string_descr),
				   &metadata_any_boards) < 0) {
			LOG_ERR("Could not parse json");
			ctx.code_status = UPDATEHUB_METADATA_ERROR;
			goto cleanup;
		}
		LOG_ERR("sha256sum: %s image size: %d",metadata_any_boards.objects[1].objects.sha256sum,metadata_any_boards.objects[1].objects.size);
		memcpy(update_info.sha256sum_image,
		       metadata_any_boards.objects[1].objects.sha256sum,
		       strlen(metadata_any_boards.objects[1].objects.sha256sum));
		update_info.image_size = metadata_any_boards.objects[1].objects.size;
	} else {
		if (!is_compatible_hardware(&metadata_some_boards)) {
			LOG_ERR("Incompatible hardware");
			ctx.code_status =
				UPDATEHUB_INCOMPATIBLE_HARDWARE;
			goto cleanup;
		}
		memcpy(update_info.sha256sum_image,
		       metadata_some_boards.objects[1].objects.sha256sum,
		       strlen(metadata_some_boards.objects[1]
			      .objects.sha256sum));
		update_info.image_size =
			metadata_some_boards.objects[1].objects.size;
	}

	ctx.code_status = UPDATEHUB_HAS_UPDATE;

cleanup:
	cleanup_connection();

error:
	k_free(metadata);
	k_free(metadata_copy);
	k_free(firmware_version);
	k_free(device_id);

	return ctx.code_status; 
}

enum updatehub_response updatehub_update(void)
{
	if (report(UPDATEHUB_STATE_DOWNLOADING) < 0) {
		LOG_ERR("Could not reporting downloading state");
		goto error;
	}

	if (report(UPDATEHUB_STATE_INSTALLING) < 0) {
		LOG_ERR("Could not reporting installing state");
		goto error;
	}

	if (install_update() != UPDATEHUB_OK) {
		goto error;
	}

	if (report(UPDATEHUB_STATE_DOWNLOADED) < 0) {
		LOG_ERR("Could not reporting downloaded state");
		goto error;
	}

	if (boot_request_upgrade(BOOT_UPGRADE_TEST)) {
		LOG_ERR("Could not reporting downloaded state");
		ctx.code_status = UPDATEHUB_INSTALL_ERROR;
		goto error;
	}

	if (report(UPDATEHUB_STATE_INSTALLED) < 0) {
		LOG_ERR("Could not reporting installed state");
		goto error;
	}

	if (report(UPDATEHUB_STATE_REBOOTING) < 0) {
		LOG_ERR("Could not reporting rebooting state");
		goto error;
	}

	LOG_INF("Image flashed successfully, you can reboot now");

	return ctx.code_status;

error:
	if (ctx.code_status != UPDATEHUB_NETWORKING_ERROR) {
		if (report(UPDATEHUB_STATE_ERROR) < 0) {
			LOG_ERR("Could not reporting error state");
		}
	}

	return ctx.code_status;
}

static void autohandler(struct k_work *work)
{
	switch (updatehub_probe()) {
	case UPDATEHUB_UNCONFIRMED_IMAGE:
		LOG_ERR("Image is unconfirmed. Rebooting to revert back to previous"
			"confirmed image.");

		sys_reboot(SYS_REBOOT_WARM);
		break;

	case UPDATEHUB_HAS_UPDATE:
		switch (updatehub_update()) {
		case UPDATEHUB_OK:
			sys_reboot(SYS_REBOOT_WARM);
			break;

		default:
			break;
		}

		break;

	case UPDATEHUB_NO_UPDATE:
		break;

	default:
		break;
	}

	k_delayed_work_submit(&updatehub_work_handle, UPDATEHUB_POLL_INTERVAL);
}
void updatehub_autohandler(void)
{
	k_delayed_work_init(&updatehub_work_handle, autohandler);
	k_delayed_work_submit(&updatehub_work_handle, K_NO_WAIT);
}


