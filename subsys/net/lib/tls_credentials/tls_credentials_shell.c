/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tls_credentials_shell, CONFIG_TLS_CREDENTIALS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/base64.h>
#include <zephyr/net/tls_credentials.h>
#include "tls_internal.h"
#include <string.h>
#include <strings.h>
#include <ctype.h>

enum cred_storage_fmt {
	/* Credential is stored as a string and will be passed between the shell and storage
	 * unmodified.
	 */
	CRED_STORAGE_FMT_STRING,

	/* Credential is stored as raw binary, and is parsed from base64 before storage and encoded
	 * back into base64 when retrieved via the shell.
	 */
	CRED_STORAGE_FMT_BINARY,
};

struct cred_type_string {
	char *name;
	enum tls_credential_type type;
};

/* The first entry in each credential type group will be used for human-readable shell
 * output. The last will be used for compact shell output. The rest are accepted synonyms.
 */
static const struct cred_type_string type_strings[] = {
	{"CA_CERT",		TLS_CREDENTIAL_CA_CERTIFICATE},
	{"CA",			TLS_CREDENTIAL_CA_CERTIFICATE},

	{"SERVER_CERT",		TLS_CREDENTIAL_SERVER_CERTIFICATE},
	{"CLIENT_CERT",		TLS_CREDENTIAL_SERVER_CERTIFICATE},
	{"SELF_CERT",		TLS_CREDENTIAL_SERVER_CERTIFICATE},
	{"SELF",		TLS_CREDENTIAL_SERVER_CERTIFICATE},
	{"CLIENT",		TLS_CREDENTIAL_SERVER_CERTIFICATE},
	{"SERV",		TLS_CREDENTIAL_SERVER_CERTIFICATE},

	{"PRIVATE_KEY",		TLS_CREDENTIAL_PRIVATE_KEY},
	{"PK",			TLS_CREDENTIAL_PRIVATE_KEY},

	{"PRE_SHARED_KEY",	TLS_CREDENTIAL_PSK},
	{"PSK",			TLS_CREDENTIAL_PSK},

	{"PRE_SHARED_KEY_ID",	TLS_CREDENTIAL_PSK_ID},
	{"PSK_ID",		TLS_CREDENTIAL_PSK_ID}
};

#define ANY_KEYWORD "any"

/* This is so that we can output base64 in chunks of this length if necessary */
BUILD_ASSERT(
	(CONFIG_TLS_CREDENTIALS_SHELL_CRED_OUTPUT_WIDTH % 4) == 0,
	"CONFIG_TLS_CREDENTIALS_SHELL_CRED_OUTPUT_WIDTH must be a multiple of 4."
);

/* Output buffers used for printing credentials and digests.
 * One extra byte included for NULL termination.
 */
static char cred_out_buf[CONFIG_TLS_CREDENTIALS_SHELL_CRED_OUTPUT_WIDTH + 1];
static char cred_digest_buf[CONFIG_TLS_CREDENTIALS_SHELL_DIGEST_BUF_SIZE + 1];

/* Internal buffer used for storing and retrieving credentials.
 * +1 byte for potential NULL termination.
 */
static char cred_buf[CONFIG_TLS_CREDENTIALS_SHELL_CRED_BUF_SIZE + 1];
static size_t cred_written;

/* Some backends (namely, the volatile backend) store a reference rather than a copy of passed-in
 * credentials. For these backends, we need to copy incoming credentials onto the heap before
 * attempting to store them.
 *
 * Since the backend in use is determined at build time by KConfig, so is this behavior.
 * If multi/dynamic-backend support is ever added, this will need to be updated.
 */
#define COPY_CREDENTIALS_TO_HEAP CONFIG_TLS_CREDENTIALS_BACKEND_VOLATILE

/* Used to track credentials that have been copied permanently to the heap, in case they are
 * ever deleted and need to be freed.
 */
static void *cred_refs[CONFIG_TLS_MAX_CREDENTIALS_NUMBER];

/* Find an empty slot in the cred_refs array, or return -1 if none exists.
 * Pass NULL to find an unused slot.
 */
static int find_ref_slot(const void *const cred)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(cred_refs); i++) {
		if (cred_refs[i] == cred) {
			return i;
		}
	}

	return -1;
}

/* Helpers */

/* Filter out non-printable characters from a passed-in string of known length */
static int filter_nonprint(char *buf, size_t len, char inval)
{
	int i;
	int ret = 0;

	for (i = 0; i < len; i++) {
		if (!isprint((int)buf[i])) {
			buf[i] = inval;
			ret = -EINVAL;
		}
	}

	return ret;
}

/* Verify that a provided string consists only of the characters 0-9*/
static bool check_numeric(char *str)
{
	int i;
	int len = strlen(str);

	for (i = 0; i < len; i++) {
		if (!isdigit((int)str[i])) {
			return false;
		}
	}

	return true;
}

/* Clear the credential write buffer, returns true if anything was actually cleared. */
static bool cred_buf_clear(void)
{
	bool cleared = cred_written != 0;

	(void)memset(cred_buf, 0, sizeof(cred_buf));
	cred_written = 0;

	return cleared;
}

/* Parse a (possibly incomplete) chunk into the credential buffer */
static int cred_buf_write(char *chunk)
{
	char *writehead = cred_buf + cred_written;
	size_t chunk_len = strlen(chunk);

	/* Verify that there is room for the incoming chunk */
	if ((writehead + chunk_len) >= (cred_buf + sizeof(cred_buf) - 1)) {
		return -ENOMEM;
	}

	/* Append chunk to the credential buffer.
	 * Deliberately do not copy NULL terminator.
	 */
	memcpy(writehead, chunk, chunk_len);
	cred_written += chunk_len;

	return chunk_len;
}

/* Get the human-readable name of a TLS credential type */
static const char *cred_type_name(enum tls_credential_type type)
{
	/* Scan over predefined type strings, and return the name
	 * of the first one of matching type.
	 */
	for (int i = 0; i < ARRAY_SIZE(type_strings); i++) {
		if (type_strings[i].type == type) {
			return type_strings[i].name;
		}
	}

	/* No matches found, it's invalid. */
	return "INVALID";
}

/* Get the compact name of a TLS credential type*/
static const char *cred_type_name_compact(enum tls_credential_type type)
{
	/* Scan over predefined type strings, and return the name
	 * of the last one of matching type.
	 */
	for (int i = ARRAY_SIZE(type_strings) - 1; i >= 0; i--) {
		if (type_strings[i].type == type) {
			return type_strings[i].name;
		}
	}

	/* No matches found, it's invalid. */
	return "INV";
}

/* Shell interface routines */

/* Attempt to parse a command line argument into a sectag.
 * TLS_SEC_TAG_NONE is returned if ANY_KEYWORD is provided.
 */
static int shell_parse_cred_sectag(const struct shell *sh, char *arg, sec_tag_t *out,
				   bool allow_any)
{
	unsigned long sectag_value;
	int err = 0;

	/* Check for "ANY" special keyword if desired. */
	if (allow_any && strcasecmp(arg, ANY_KEYWORD) == 0) {
		*out = TLS_SEC_TAG_NONE;
		return 0;
	}

	/* Otherwise, verify that the sectag is purely numeric */
	if (!check_numeric(arg)) {
		err = -EINVAL;
		goto error;
	}

	/* Use strtoul because it has nicer validation features than atoi */
	sectag_value = shell_strtoul(arg, 10, &err);

	if (!err) {
		*out = (sec_tag_t)sectag_value;
		return 0;
	}

error:
	shell_fprintf(sh, SHELL_ERROR, "%s is not a valid sectag.\n", arg);
	return err;
}

/* Attempt to parse a command line argument into a credential type.
 * TLS_CREDENTIAL_NONE is returned if ANY_KEYWORD is provided.
 */
static int shell_parse_cred_type(const struct shell *sh, char *arg, enum tls_credential_type *out,
				 bool allow_any)
{
	/* Check for "ANY" special keyword if desired. */
	if (allow_any && strcasecmp(arg, ANY_KEYWORD) == 0) {
		*out = TLS_CREDENTIAL_NONE;
		return 0;
	}

	/* Otherwise, scan over predefined type strings, and return the corresponding
	 * credential type if one is found
	 */
	for (int i = 0; i < ARRAY_SIZE(type_strings); i++) {
		if (strcasecmp(arg, type_strings[i].name) == 0) {
			*out = type_strings[i].type;
			return 0;
		}
	}

	/* No matches found, it's invalid. */
	shell_fprintf(sh, SHELL_ERROR, "%s is not a valid credential type.\n", arg);

	return -EINVAL;
}

/* Parse a backend specifier argument
 * Right now, only a single backend is supported, so this is serving simply as a reserved argument.
 * As such, the only valid input is "default"
 */
static int shell_parse_cred_backend(const struct shell *sh, char *arg)
{
	if (strcasecmp(arg, "default") == 0) {
		return 0;
	}

	shell_fprintf(sh, SHELL_ERROR, "%s is not a valid backend.\n", arg);

	return -EINVAL;
}

/* Parse an input type specifier */
static int shell_parse_cred_storage_format(const struct shell *sh, char *arg,
					   enum cred_storage_fmt *out, bool *terminated)
{
	if (strcasecmp(arg, "bin") == 0) {
		*out = CRED_STORAGE_FMT_BINARY;
		*terminated = false;
		return 0;
	}

	if (strcasecmp(arg, "bint") == 0) {
		*out = CRED_STORAGE_FMT_BINARY;
		*terminated = true;
		return 0;
	}

	if (strcasecmp(arg, "str") == 0) {
		*out = CRED_STORAGE_FMT_STRING;
		*terminated = false;
		return 0;
	}

	if (strcasecmp(arg, "strt") == 0) {
		*out = CRED_STORAGE_FMT_STRING;
		*terminated = true;
		return 0;
	}

	shell_fprintf(sh, SHELL_ERROR, "%s is not a valid storage format.\n", arg);

	return -EINVAL;
}

/* Clear credential buffer, with shell feedback */
static void shell_clear_cred_buf(const struct shell *sh)
{
	/* We will only print a message if some data was actually wiped. */
	if (cred_buf_clear()) {
		shell_fprintf(sh, SHELL_NORMAL, "Credential buffer cleared.\n");
	}
}

/* Write data into the credential buffer, with shell feedback. */
static int shell_write_cred_buf(const struct shell *sh, char *chunk)
{
	int res = cred_buf_write(chunk);

	/* Report results. */

	if (res == -ENOMEM) {
		shell_fprintf(sh, SHELL_ERROR, "Not enough room in credential buffer for "
					       "provided data. Increase "
					       "CONFIG_TLS_CREDENTIALS_SHELL_CRED_BUF_SIZE.\n");
		shell_clear_cred_buf(sh);
		return -ENOMEM;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Stored %d bytes.\n", res);

	return 0;
}

/* Adds a credential to the credential store */
static int tls_cred_cmd_add(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	sec_tag_t sectag;
	enum cred_storage_fmt format;
	bool terminated;
	enum tls_credential_type type;
	void *cred_copy = NULL;
	void *cred_chosen = NULL;
	bool keep_copy = false;
	int ref_slot = -1;

	/* Lock credentials so that we can interact with them directly.
	 * Mainly this is required by credential_get.
	 */

	credentials_lock();

	err = shell_parse_cred_sectag(sh, argv[1], &sectag, false);
	if (err) {
		goto cleanup;
	}

	err = shell_parse_cred_type(sh, argv[2], &type, false);
	if (err) {
		goto cleanup;
	}

	err = shell_parse_cred_backend(sh, argv[3]);
	if (err) {
		goto cleanup;
	}

	err = shell_parse_cred_storage_format(sh, argv[4], &format, &terminated);
	if (err) {
		goto cleanup;
	}

	if (argc == 6) {
		/* Credential was passed, clear credential buffer and then use the passed-in
		 * credential.
		 */
		shell_clear_cred_buf(sh);
		err = shell_write_cred_buf(sh, argv[5]);
		if (err) {
			goto cleanup;
		}
	}

	/* Make sure the credential buffer isn't empty. */
	if (cred_written == 0) {
		shell_fprintf(sh, SHELL_ERROR, "Please provide a credential to add.\n");
		err = -ENOENT;
		goto cleanup;
	}

	/* Check whether a credential of this type and sectag already exists. */
	if (credential_get(sectag, type)) {
		shell_fprintf(sh, SHELL_ERROR, "TLS credential with sectag %d and type %s "
					       "already exists.\n", sectag, cred_type_name(type));
		err = -EEXIST;
		goto cleanup;
	}

	/* If binary format was specified, decode from base64. */
	if (format == CRED_STORAGE_FMT_BINARY) {
		/* base64_decode can handle in-place operation.
		 * Pass &cred_written as olen so that it is updated to match the size of the base64
		 * encoding.
		 *
		 * We use sizeof(cred_buf) - 1 since we want to keep room fors a NULL terminator.
		 * Though, technically, this is not actually needed since the output of
		 * base64_decode is always shorter than its input.
		 */
		err = base64_decode(cred_buf, sizeof(cred_buf) - 1, &cred_written,
				    cred_buf, cred_written);
		if (err) {
			shell_fprintf(sh, SHELL_ERROR, "Could not decode input from base64, "
						       "error: %d\n", err);
			err = -EINVAL;
			goto cleanup;
		}
	}

	/* If NULL termination was requested, append one.
	 * We are always guaranteed to have room in the buffer for this.
	 */
	if (terminated) {
		cred_buf[cred_written] = 0;
		cred_written += 1;
	}

	/* Default to using cred_buf directly. */
	cred_chosen = cred_buf;

	/* If the currently active TLS Credentials backend stores credentials by reference,
	 * copy the incoming credentials off to the heap, and then use this copy instead.
	 */
	if (IS_ENABLED(COPY_CREDENTIALS_TO_HEAP)) {
		/* Before copying the credential to heap, make sure we are able to store a
		 * reference to it so that it can be freed if the credential is ever deleted.
		 */

		ref_slot = find_ref_slot(NULL);

		if (ref_slot < 0) {
			shell_fprintf(sh, SHELL_ERROR, "No reference slot available, cannot copy "
						       "credential to heap. Credential will not be "
						       "stored\n");
			err = -ENOMEM;
			goto cleanup;
		}

		cred_copy = k_malloc(cred_written);
		if (!cred_copy) {
			shell_fprintf(sh, SHELL_ERROR, "Not enough heap for TLS credential of "
						       "size %d.\n", cred_written);
			err = -ENOMEM;
			goto cleanup;
		}

		memset(cred_copy, 0, cred_written);
		memcpy(cred_copy, cred_buf, cred_written);

		shell_fprintf(sh, SHELL_WARNING, "Credential has been copied to heap. Memory will "
						 "be leaked if this credential is deleted without "
						 "using the shell.\n");

		cred_chosen = cred_copy;
	}

	/* Finally, store the credential in whatever credentials backend is active. */
	err = tls_credential_add(sectag, type, cred_chosen, cred_written);
	if (err) {
		shell_fprintf(sh, SHELL_ERROR, "Failed to add TLS credential with sectag %d and "
					       "type %s. Error: %d\n.", sectag,
					       cred_type_name(type), err);
		goto cleanup;
	}

	/* Do not free the copied key during cleanup, since it was successfully written. */
	keep_copy = true;

	shell_fprintf(sh, SHELL_NORMAL, "Added TLS credential of type %s, sectag %d, and length %d "
					"bytes.\n", cred_type_name(type), sectag, cred_written);

cleanup:
	/* Unlock credentials since we are done interacting with internal state. */
	credentials_unlock();

	/* We are also done with the credentials buffer, so clear it for good measure. */
	shell_clear_cred_buf(sh);

	/* If we copied the credential, make sure it is eventually freed. */
	if (cred_copy) {
		if (keep_copy) {
			/* If the credential was successfully stored, keep a reference to it in case
			 * it is ever deleted and needs to be freed.
			 */
			cred_refs[ref_slot] = cred_copy;
		} else {
			/* Otherwise, clear and free it immediately */
			memset(cred_copy, 0, cred_written);
			(void)k_free(cred_copy);
		}
	}

	return err;
}

/* Buffers credential data into the credential buffer. */
static int tls_cred_cmd_buf(const struct shell *sh, size_t argc, char *argv[])
{
	/* If the "clear" keyword is provided, clear the buffer rather than write to it. */
	if (strcmp(argv[1], "clear") == 0) {
		shell_clear_cred_buf(sh);
		return 0;
	}

	/* Otherwise, assume provided arg is base64 and attempt to write it into the credential
	 * buffer.
	 */
	return shell_write_cred_buf(sh, argv[1]);
}

/* Deletes a credential from the credential store */
static int tls_cred_cmd_del(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	sec_tag_t sectag;
	enum tls_credential_type type;
	struct tls_credential *cred = NULL;
	int ref_slot = -1;

	/* Lock credentials so that we can safely use internal access functions. */
	credentials_lock();

	err = shell_parse_cred_sectag(sh, argv[1], &sectag, false);
	if (err) {
		goto cleanup;
	}

	err = shell_parse_cred_type(sh, argv[2], &type, false);
	if (err) {
		goto cleanup;
	}

	/* Check whether a credential of this type and sectag actually exists. */
	cred = credential_get(sectag, type);
	if (!cred) {
		shell_fprintf(sh, SHELL_ERROR, "There is no TLS credential with sectag %d and "
					       "type %s.\n", sectag, cred_type_name(type));
		err = -ENOENT;
		goto cleanup;
	}

	ref_slot = find_ref_slot(cred->buf);
	if (ref_slot >= 0) {
		/* This was a credential we copied to heap. Clear and free it. */
		memset((void *)cred_buf, 0, cred->len);
		k_free((void *)cred_buf);
		cred->buf = NULL;

		/* Clear the reference slot so it can be used again. */
		cred_refs[ref_slot] = NULL;

		shell_fprintf(sh, SHELL_NORMAL, "Stored credential freed.\n");
	}

	/* Attempt to delete. */
	err = tls_credential_delete(sectag, type);
	if (err) {
		shell_fprintf(sh, SHELL_ERROR, "Deleting TLS credential with sectag %d and "
					       "type %s failed with error: %d.\n", sectag,
					       cred_type_name(type), err);
		goto cleanup;
	}

	shell_fprintf(sh, SHELL_NORMAL, "Deleted TLS credential with sectag %d and type %s.\n",
					sectag, cred_type_name(type));

cleanup:
	/* Unlock credentials since we are done interacting with internal state. */
	credentials_unlock();

	return err;
}

/* Retrieves credential data from credential store. */
static int tls_cred_cmd_get(const struct shell *sh, size_t argc, char *argv[])
{
	int i;
	int remaining;
	int written;
	int err = 0;
	size_t cred_len;
	sec_tag_t sectag;
	enum tls_credential_type type;
	enum cred_storage_fmt format;
	bool terminated;

	size_t line_length;

	/* Lock credentials so that we can safely use internal access functions. */
	credentials_lock();

	err = shell_parse_cred_sectag(sh, argv[1], &sectag, false);
	if (err) {
		goto cleanup;
	}

	err = shell_parse_cred_type(sh, argv[2], &type, false);
	if (err) {
		goto cleanup;
	}

	err = shell_parse_cred_storage_format(sh, argv[3], &format, &terminated);
	if (err) {
		goto cleanup;
	}

	line_length = CONFIG_TLS_CREDENTIALS_SHELL_CRED_OUTPUT_WIDTH;

	/* If the credential is stored as binary, adjust line length so that the output
	 * base64 has width CONFIG_TLS_CREDENTIALS_SHELL_CRED_OUTPUT_WIDTH
	 */
	if (format == CRED_STORAGE_FMT_BINARY) {
		line_length = CONFIG_TLS_CREDENTIALS_SHELL_CRED_OUTPUT_WIDTH / 4 * 3;
	}

	/* Check whether a credential of this type and sectag actually exists. */
	if (!credential_get(sectag, type)) {
		shell_fprintf(sh, SHELL_ERROR, "There is no TLS credential with sectag %d and "
					       "type %s.\n", sectag, cred_type_name(type));
		err = -ENOENT;
		goto cleanup;
	}

	/* Clear the credential buffer before use. */
	shell_clear_cred_buf(sh);

	/* Load the credential into the credential buffer */
	cred_len = sizeof(cred_buf);
	err = tls_credential_get(sectag, type, cred_buf, &cred_len);
	if (err == -EFBIG) {
		shell_fprintf(sh, SHELL_ERROR, "Not enough room in the credential buffer to "
					       "retrieve credential with sectag %d and type %s. "
					       "Increase TLS_CREDENTIALS_SHELL_MAX_CRED_LEN.\n",
					       sectag, cred_type_name(type));
		err = -ENOMEM;
		goto cleanup;
	} else if (err) {
		shell_fprintf(sh, SHELL_ERROR, "Could not retrieve TLS credential with sectag %d "
					       "and type %s due to error: %d.\n", sectag,
					       cred_type_name(type), err);
		goto cleanup;
	}

	/* Update the credential buffer writehead.
	 * Keeping this accurate ensures that a "Buffer Cleared" message is eventually printed.
	 */
	cred_written = cred_len;

	/* If the stored credential is NULL-terminated, do not include NULL termination in output */
	if (terminated) {
		if (cred_buf[cred_written - 1] != 0) {
			shell_fprintf(sh, SHELL_ERROR, "The stored credential isn't "
						       "NULL-terminated, but a NULL-terminated "
						       "format was specified.\n");

			err = -EINVAL;
			goto cleanup;
		}
		cred_written -= 1;
	}

	/* Print the credential out in lines. */
	for (i = 0; i < cred_written; i += line_length) {
		/* Print either a full line, or however much credential data is left. */
		remaining = MIN(line_length, cred_written - i);

		/* Read out a line of data. */
		memset(cred_out_buf, 0, sizeof(cred_out_buf));
		if (format == CRED_STORAGE_FMT_BINARY) {
			(void)base64_encode(cred_out_buf, sizeof(cred_out_buf),
					    &written, &cred_buf[i], remaining);
		} else if (format == CRED_STORAGE_FMT_STRING) {
			memcpy(cred_out_buf, &cred_buf[i], remaining);
			if (filter_nonprint(cred_out_buf, remaining, '?')) {
				err = -EBADF;
			}
		}

		/* Print the line. */
		shell_fprintf(sh, SHELL_NORMAL, "%s\n", cred_out_buf);
	}

	if (err) {
		shell_fprintf(sh, SHELL_WARNING, "Non-printable characters were included in the "
						 "output and filtered. Have you selected the "
						 "correct storage format?\n");
	}

cleanup:
	/* Unlock credentials since we are done interacting with internal state. */
	credentials_unlock();

	/* Clear buffers when done. */
	memset(cred_out_buf, 0, sizeof(cred_out_buf));
	shell_clear_cred_buf(sh);

	return err;
}

/* Lists credentials in credential store. */
static int tls_cred_cmd_list(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	size_t digest_size;
	sec_tag_t sectag = TLS_SEC_TAG_NONE;
	struct tls_credential *cred;
	int count = 0;

	sec_tag_t sectag_filter = TLS_SEC_TAG_NONE;
	enum tls_credential_type type_filter = TLS_CREDENTIAL_NONE;

	/* Lock credentials so that we can safely use internal access functions. */
	credentials_lock();

	/* Sectag filter was provided, parse it. */
	if (argc >= 2) {
		err = shell_parse_cred_sectag(sh, argv[1], &sectag_filter, true);
		if (err) {
			goto cleanup;
		}
	}

	/* Credential type filter was provided, parse it. */
	if (argc >= 3) {
		err = shell_parse_cred_type(sh, argv[2], &type_filter, true);
		if (err) {
			goto cleanup;
		}
	}

	/* Scan through all occupied sectags */
	while ((sectag = credential_next_tag_get(sectag)) != TLS_SEC_TAG_NONE) {
		/* Filter by sectag if requested. */
		if (sectag_filter != TLS_SEC_TAG_NONE && sectag != sectag_filter) {
			continue;
		}

		cred = NULL;
		/* Scan through all credentials within each sectag */
		while ((cred = credential_next_get(sectag, cred)) != NULL) {
			/* Filter by credential type if requested. */
			if (type_filter != TLS_CREDENTIAL_NONE && cred->type != type_filter) {
				continue;
			}
			count++;

			/* Generate a digest of the credential */
			memset(cred_digest_buf, 0, sizeof(cred_digest_buf));
			strcpy(cred_digest_buf, "N/A");
			digest_size = sizeof(cred_digest_buf);
			err = credential_digest(cred, cred_digest_buf, &digest_size);

			/* Print digest and sectag/type info */
			shell_fprintf(sh, err ? SHELL_ERROR : SHELL_NORMAL, "%d,%s,%s,%d\n",
				      sectag, cred_type_name_compact(cred->type),
				      err ? "ERROR" : cred_digest_buf, err);

			err = 0;
		}
	};

	shell_fprintf(sh, SHELL_NORMAL, "%d credentials found.\n", count);

cleanup:
	/* Unlock credentials since we are done interacting with internal state. */
	credentials_unlock();

	/* Clear digest buffer afterwards for good measure. */
	memset(cred_digest_buf, 0, sizeof(cred_digest_buf));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(tls_cred_cmds,
	SHELL_CMD_ARG(buf, NULL, "Buffer in credential data so it can be added.",
		      tls_cred_cmd_buf, 2, 0),
	SHELL_CMD_ARG(add, NULL, "Add a TLS credential.",
		      tls_cred_cmd_add, 5, 1),
	SHELL_CMD_ARG(del, NULL, "Delete a TLS credential.",
		      tls_cred_cmd_del, 3, 0),
	SHELL_CMD_ARG(get, NULL, "Retrieve the contents of a TLS credential",
		      tls_cred_cmd_get, 4, 0),
	SHELL_CMD_ARG(list, NULL, "List stored TLS credentials, optionally filtering by type "
				  "or sectag.",
		      tls_cred_cmd_list, 1, 2),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(cred, &tls_cred_cmds, "TLS Credentials Commands", NULL);
