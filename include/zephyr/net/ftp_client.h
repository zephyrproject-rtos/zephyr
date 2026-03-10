/*
 * Copyright (c) 2020-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 *  @brief FTP client library
 */

#ifndef ZEPHYR_INCLUDE_NET_FTP_CLIENT_H_
#define ZEPHYR_INCLUDE_NET_FTP_CLIENT_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/net/net_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 * @brief FTP client library
 * @details Provide selected FTP client functionality
 * @defgroup ftp_client FTP client library API
 * @since 4.4
 * @version 0.1.0
 * @ingroup networking
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#if defined(CONFIG_FTP_CLIENT)
#define FTP_BUFFER_SIZE	CONFIG_FTP_CLIENT_BUF_SIZE
#else
#define FTP_BUFFER_SIZE	1
#endif
/** @endcond */

/**
 * @brief List of FTP server reply codes
 * Reference RFC959 FTP Transfer Protocol
 */
enum ftp_reply_code {
	/* 100 Series	The requested action is being initiated, expect another
	 * reply before proceeding with a new command
	 */

	/** Restart marker replay. In this case, the text is exact and not left
	 *  to the particular implementation; it must read: MARK yyyy = mmmm
	 *  where yyyy is User-process data stream marker, and mmmm server's
	 *  equivalent marker (note the spaces between markers and "=")
	 */
	FTP_CODE_110_RESTART_MARKER_REPLAY = 110,
	/** Service ready in nnn minutes */
	FTP_CODE_120_SERVICE_READY_IN_NNN_MINUTES = 120,
	/** Data connection already open; transfer starting */
	FTP_CODE_125_DATA_CONN_ALREADY_OPEN = 125,
	/** File status okay; about to open data connection */
	FTP_CODE_150_FILE_STATUS_OK = 150,

	/* 200 Series	The requested action has been successfully completed */

	/** Command OK*/
	FTP_CODE_200_OK = 200,
	/** Command not implemented, superfluous at this site */
	FTP_CODE_202_NOT_IMPLEMENTED = 202,
	/** System status, or system help reply */
	FTP_CODE_211_SYSTEM_STATUS = 211,
	/** Directory status */
	FTP_CODE_212_DIR_STATUS = 212,
	/** File status*/
	FTP_CODE_213_FILE_STATUS = 213,
	/** Help message. Explains how to use the server or the meaning of a
	 *  particular non-standard command. This reply is useful only to the
	 *  human user
	 */
	FTP_CODE_214_HELP_MSG = 214,
	/** NAME system type. Where NAME is an official system name from the
	 *  registry kept by IANA.
	 */
	FTP_CODE_215_NAME_SYSTEM_TYPE = 215,
	/** Service ready for new user */
	FTP_CODE_220_SERVICE_READY = 220,
	/** Service closing control connection */
	FTP_CODE_221_SERVICE_CLOSING_CONN = 221,
	/** Data connection open; no transfer in progress */
	FTP_CODE_225_DATA_CONN_OPEN = 225,
	/** Closing data connection. Requested file action successful (for
	 *  example, file transfer or file abort)
	 */
	FTP_CODE_226_CLOSING_DATA_CONN_SUCCESS = 226,
	/** Entering Passive Mode (h1,h2,h3,h4,p1,p2) */
	FTP_CODE_227_ENTERING_PASSIVE_MODE = 227,
	/** Entering Long Passive Mode (long address, port) */
	FTP_CODE_228_ENTERING_LONG_PASSIVE_MODE = 228,
	/** Entering Extended Passive Mode (|||port|) */
	FTP_CODE_229_ENTERING_EXT_PASSIVE_MODE = 229,
	/** User logged in, proceed. Logged out if appropriate */
	FTP_CODE_230_USER_LOGGED_IN = 230,
	/** User logged out; service terminated */
	FTP_CODE_231_USER_LOGGED_OUT = 231,
	/** Logout command noted, will complete when transfer done */
	FTP_CODE_233_LOGOUT_COMMAND_NOTED = 233,
	/** Specifies that the server accepts the authentication mechanism
	 *  specified by the client, and the exchange of security data is
	 *  complete. A higher level nonstandard code created by Microsoft
	 */
	FTP_CODE_234_SECURITY_ACCEPTED = 234,
	/** Requested file action okay, completed */
	FTP_CODE_250_FILE_ACTION_COMPLETED = 250,
	/** "PATHNAME" created */
	FTP_CODE_257_PATHNAME_CREATED = 257,

	/* 300 Series	The command has been accepted, but the requested action
	 *is on hold, pending receipt of further information
	 */

	/** User name okay, need password */
	FTP_CODE_331_USERNAME_OK_NEED_PASSWORD = 331,
	/** Need account for login */
	FTP_CODE_332_NEED_ACCOUNT = 332,
	/** Requested file action pending further information */
	FTP_CODE_350_FILE_ACTION_PENDING = 350,

	/* 400 Series	The command was not accepted and the requested action
	 * did not take place, but the error condition is temporary and the
	 * action may be requested again
	 */

	/** Service not available, closing control connection. This may be a
	 *  reply to any command if the service knows it must shut down
	 */
	FTP_CODE_421_SERVICE_UNAVAILABLE = 421,
	/** Cannot open data connection */
	FTP_CODE_425_CANNOT_OPEN_DATA_CONN = 425,
	/** Connection closed; transfer aborted */
	FTP_CODE_426_CONN_CLOSED = 426,
	/** Invalid username or password */
	FTP_CODE_430_INVALID_USERNAME_OR_PASSWORD = 430,
	/** Requested host unavailable */
	FTP_CODE_434_HOST_UNAVAILABLE = 434,
	/** Requested file action not taken */
	FTP_CODE_450_FILE_ACTION_NOT_TAKEN = 450,
	/** Requested action aborted. Local error in processing */
	FTP_CODE_451_ACTION_ABORTED = 451,
	/** Requested action not taken. Insufficient storage space in system.
	 *  File unavailable (for example, file busy)
	 */
	FTP_CODE_452_ACTION_NOT_TAKEN = 452,

	/* 500 Series	Syntax error, command unrecognized and the requested
	 * action did not take place. This may include errors such as command
	 * line too long
	 */

	/** General error */
	FTP_CODE_500_GENERAL_ERROR = 500,
	/** Syntax error in parameters or arguments */
	FTP_CODE_501_SYNTAX_ERROR = 501,
	/** Command not implemented */
	FTP_CODE_502_COMMAND_NOT_COMPLETED = 502,
	/** Bad sequence of commands */
	FTP_CODE_503_BAD_SEQUENCE_OF_COMMANDS = 503,
	/** Command not implemented for that parameter */
	FTP_CODE_504_COMMAND_NOT_IMPLEMENTED = 504,
	/** Not logged in */
	FTP_CODE_530_NOT_LOGGED_IN = 530,
	/** Need account for storing files */
	FTP_CODE_532_NEED_ACCOUNT = 532,
	/** Could Not Connect to Server - Policy Requires SSL */
	FTP_CODE_534_CANNOT_CONNECT_SSL_REQUIRED = 534,
	/** Requested action not taken. File unavailable (for example, file not
	 *  found, no access)
	 */
	FTP_CODE_550_FILE_UNAVAILABLE = 550,
	/** Requested action aborted. Page type unknown */
	FTP_CODE_551_PAGE_TYPE_UNKNOWN = 551,
	/** Requested file action aborted. Exceeded storage allocation (for
	 *  current directory or dataset)
	 */
	FTP_CODE_552_FILE_EXCEEDED_STORAGE_LOCATION = 552,
	/** Requested action not taken. File name not allowed */
	FTP_CODE_553_FILE_NAME_NOT_ALLOWED = 553,

	/* Replies regarding confidentiality and integrity */

	/** Integrity protected reply */
	FTP_CODE_631_INTEGRITY_PROTECTED_REPLY = 631,
	/** Confidentiality and integrity protected reply */
	FTP_CODE_632_INT_AND_CONF_PROTECTED_REPLY = 632,
	/** Confidentiality protected reply */
	FTP_CODE_633_CONFIDENTIALITY_PROTECTED_REPLY = 633,

	/* Proprietary reply codes */

	/** DUMMY */
	FTP_CODE_900_UNKNOWN_ERROR = 900,

	/** Fatal errors */
	/** Disconnected by remote server */
	FTP_CODE_901_DISCONNECTED_BY_REMOTE = 901,
	/** Connection aborted */
	FTP_CODE_902_CONNECTION_ABORTED = 902,
	/** Socket poll error */
	FTP_CODE_903_SOCKET_POLL_ERROR = 903,
	/** Unexpected poll event */
	FTP_CODE_904_UNEXPECTED_POLL_EVENT = 904,
	/** Network down */
	FTP_CODE_905_NETWORK_DOWN = 905,
	/** Unexpected error */
	FTP_CODE_909_UNEXPECTED_ERROR = 909,

	/* Non-fatal errors */
	/** Data transfer timeout */
	FTP_CODE_910_DATA_TRANSFER_TIMEOUT = 910,

	/* 10000 Series Common Winsock Error Codes[2] (These are not FTP
	 * return codes)
	 */

	/** Connection reset by peer. The connection was forcibly closed by the
	 *  remote host
	 */
	FTP_CODE_10054_CONNECTION_RESET_BY_PEER = 10054,
	/** Cannot connect to remote server */
	FTP_CODE_10060_CANNOT_CONNECT = 10060,
	/** Cannot connect to remote server. The connection is actively refused
	 *  by the server
	 */
	FTP_CODE_10061_CONNECTION_REFUSED = 10061,
	/** Directory not empty */
	FTP_CODE_10066_DIRECTORY_NOT_EMPTY = 10066,
	/** Too many users, server is full */
	FTP_CODE_10068_TOO_MANY_USERS = 10068
};

/** @cond INTERNAL_HIDDEN */
#define FTP_PRELIMINARY_POS(code)	((code) >= 100 && (code) < 200)
#define FTP_COMPLETION_POS(code)	((code) >= 200 && (code) < 300)
#define FTP_INTERMEDIATE_POS(code)	((code) >= 300 && (code) < 400)
#define FTP_TRANSIENT_NEG(code)		((code) >= 400 && (code) < 500)
#define FTP_COMPLETION_NEG(code)	((code) >= 500 && (code) < 600)
#define FTP_PROTECTED(code)		((code) >= 600 && (code) < 700)
#define FTP_PROPRIETARY(code)		((code) >= 900 && (code) < 1000)
#define FTP_WINSOCK_ERR(code)		((code) >= 10000)
/** @endcond */

/** FTP transfer mode. */
enum ftp_transfer_type {
	FTP_TYPE_ASCII, /**< ASCII transfer */
	FTP_TYPE_BINARY, /**< Binary transfer */
};

/** FTP file write mode. */
enum ftp_put_type {
	FTP_PUT_NORMAL, /**< Overwrite a file */
	FTP_PUT_UNIQUE, /**< Write to a file with a unique file name */
	FTP_PUT_APPEND, /**< Append a file */
};

/**
 * @brief FTP asynchronous callback function.
 *
 * @param msg FTP client data received, or local message
 * @param len length of message
 */
typedef void (*ftp_client_callback_t)(const uint8_t *msg, uint16_t len);

/** FTP client context. */
struct ftp_client {
	struct net_sockaddr remote; /**< Server address */
	bool connected; /**< Server connected flag */
	int ctrl_sock; /**< Control socket */
	int data_sock; /**< Data socket */
	int sec_tag; /**< Secure tag */
	uint8_t ctrl_buf[FTP_BUFFER_SIZE]; /**< Control buffer */
	size_t ctrl_len; /**< Length of data in the control buffer */
	uint8_t data_buf[FTP_BUFFER_SIZE]; /**< Data buffer */
	ftp_client_callback_t ctrl_callback; /**< Control callback */
	ftp_client_callback_t data_callback; /**< Data callback */
	/** @cond INTERNAL_HIDDEN */
	struct k_mutex lock;
	struct k_work_delayable keepalive_work;
	/** @endcond */
};

/**@brief Initialize the FTP client context.
 *
 * @param client FTP client context
 * @param ctrl_callback Callback for FTP command result.
 * @param data_callback Callback for FTP received data.
 *
 * @return 0 on success. A negative errno code in case of a failure.
 */
int ftp_init(struct ftp_client *client, ftp_client_callback_t ctrl_callback,
	     ftp_client_callback_t data_callback);

/**@brief Uninitialize the FTP client context.
 *
 * @param client FTP client context
 *
 * @return 0 on success. A negative errno code in case of a failure.
 */
int ftp_uninit(struct ftp_client *client);

/**@brief Open FTP connection.
 *
 * @param client FTP client context
 * @param hostname FTP server name or IP address
 * @param port FTP service port on server
 * @param sec_tag If FTP over TLS is required (-1 means no TLS)
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_open(struct ftp_client *client, const char *hostname, uint16_t port,
	     int sec_tag);

/**@brief FTP server login.
 *
 * @param client FTP client context
 * @param username user name
 * @param password The password
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_login(struct ftp_client *client, const char *username,
	      const char *password);

/**@brief Close FTP connection.
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_close(struct ftp_client *client);

/**@brief Get FTP server and connection status
 * Also returns server system type
 *
 * @param client FTP client context
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_status(struct ftp_client *client);

/**@brief Set FTP transfer type
 *
 * @param client FTP client context
 * @param type transfer type
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_type(struct ftp_client *client, enum ftp_transfer_type type);

/**@brief Print working directory
 *
 * @param client FTP client context
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_pwd(struct ftp_client *client);

/**@brief List information of folder or file
 *
 * @param client FTP client context
 * @param options List options, refer to Linux "man ls"
 * @param target file or directory to list. If not specified, list current folder
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_list(struct ftp_client *client, const char *options, const char *target);

/**@brief Change working directory
 *
 * @param client FTP client context
 * @param folder Target folder
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_cwd(struct ftp_client *client, const char *folder);

/**@brief Make directory
 *
 * @param client FTP client context
 * @param folder New folder name
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_mkd(struct ftp_client *client, const char *folder);

/**@brief Remove directory
 *
 * @param client FTP client context
 * @param folder Target folder name
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_rmd(struct ftp_client *client, const char *folder);

/**@brief Rename a file
 *
 * @param client FTP client context
 * @param old_name Old file name
 * @param new_name New file name
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_rename(struct ftp_client *client, const char *old_name,
	       const char *new_name);

/**@brief Delete a file
 *
 * @param client FTP client context
 * @param file Target file name
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_delete(struct ftp_client *client, const char *file);

/**@brief Get a file
 *
 * @param client FTP client context
 * @param file Target file name
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_get(struct ftp_client *client, const char *file);

/**@brief Put data to a file
 * If file does not exist, create the file
 *
 * @param client FTP client context
 * @param file Target file name
 * @param data Data to be stored
 * @param length Length of data to be stored
 * @param type specify FTP put types, see enum ftp_reply_code
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_put(struct ftp_client *client, const char *file, const uint8_t *data,
	    uint16_t length, int type);

/**@brief Exchange keep-alive commands with the server.
 *
 * @param client FTP client context
 *
 * @return 0 on success.
 *         A negative errno code in case of a failure.
 *         A positive FTP status code in case of an unexpected server response.
 */
int ftp_keepalive(struct ftp_client *client);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_FTP_CLIENT_H_ */

/**@} */
