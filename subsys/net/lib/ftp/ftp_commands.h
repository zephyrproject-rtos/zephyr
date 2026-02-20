/*
 * Copyright (c) 2020-2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FTP_COMMANDS_H_
#define FTP_COMMANDS_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Reference RFC959 File Transfer Protocol
 */
/* Abort an active file transfer */
#define CMD_ABOR	"ABOR\r\n"
/* Account information */
#define CMD_ACCT	"ACCT %s\r\n"
/* Allocate sufficient disk space to receive a file */
#define CMD_ALLO	"ALLO\r\n"
/* Append (with create) */
#define CMD_APPE	"APPE %s\r\n"
/* Change to Parent Directory */
#define CMD_CDUP	"CDUP\r\n"
/* Change working directory */
#define CMD_CWD		"CWD %s\r\n"
/* Delete file */
#define CMD_DELE	"DELE %s\r\n"
/* Returns usage documentation on a command if specified, else a general help
 * document is returned
 */
#define CMD_HELP	"HELP\r\n"
/* Returns information of a file or directory if specified, else in formation of
 * the current working directory is returned
 */
#define CMD_LIST	"LIST\r\n"
#define CMD_LIST_OPT	"LIST %s\r\n"
#define CMD_LIST_FILE	"LIST %s\r\n"
#define CMD_LIST_OPT_FILE	"LIST %s %s\r\n"
/* Make directory */
#define CMD_MKD		"MKD %s\r\n"
/* Sets the transfer mode (Stream, Block, or Compressed) */
#define CMD_MODE	"MODE\r\n"
/* Returns a list of file names in a specified directory */
#define CMD_NLST	"NLST\r\n"
/* No operation (dummy packet; used mostly on keepalives) */
#define CMD_NOOP	"NOOP\r\n"
/* Select options for a feature (for example OPTS UTF8 ON) */
#define CMD_OPTS	"OPTS %s\r\n"
/* Authentication password */
#define CMD_PASS	"PASS %s\r\n"
/* Enter passive mode */
#define CMD_PASV	"PASV\r\n"
/* Specifies an address and port to which the server should connect */
#define CMD_PORT	"PORT\r\n"
/* Print working directory. Returns the current directory of the host */
#define CMD_PWD		"PWD\r\n"
/* Disconnect */
#define CMD_QUIT	"QUIT\r\n"
/* Re-initializes the connection*/
#define CMD_REIN	"REIN\r\n"
/* Restart transfer from the specified point */
#define CMD_REST	"REST\r\n"
/* Retrieve a copy of the file */
#define CMD_RETR	"RETR %s\r\n"
/* Remove a directory */
#define CMD_RMD		"RMD %s\r\n"
/* Rename from */
#define CMD_RNFR	"RNFR %s\r\n"
/* Rename to */
#define CMD_RNTO	"RNTO %s\r\n"
/* Sends site specific commands to remote server (like SITE IDLE 60 or
 * SITE UMASK 002). Inspect SITE HELP output for complete list of
 * supported commands
 */
#define CMD_SITE	"SITE %s\r\n"
/* Return the size of a file */
#define CMD_SIZE	"SIZE %s\r\n"
/* Mount file structure */
#define CMD_SMNT	"SMNT\r\n"
/* Returns information on the server status, including the status of the
 * current connection
 */
#define CMD_STAT	"STAT\r\n"
/* Accept the data and to store the data as a file at the server site */
#define CMD_STOR	"STOR %s\r\n"
/* Store file uniquely */
#define CMD_STOU	"STOU\r\n"
/* Set file transfer structure */
#define CMD_STRU	"STRU\r\n"
/* Return system type */
#define CMD_SYST	"SYST\r\n"
/* Sets the transfer mode (ASCII/Binary) */
#define CMD_TYPE_A	"TYPE A\r\n"
#define CMD_TYPE_I	"TYPE I\r\n"
/* Authentication username */
#define CMD_USER	"USER %s\r\n"

#ifdef __cplusplus
}
#endif

#endif /* FTP_COMMANDS_H_ */
