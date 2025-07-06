/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <zephyr/net/net_if.h>
#include <strings.h>
#include <ctype.h>

#include "net_shell_private.h"

#if defined(CONFIG_NET_CONNECTION_MANAGER)

#include "conn_mgr_private.h"
#include <zephyr/net/conn_mgr_connectivity.h>
#include <zephyr/net/conn_mgr_monitor.h>

#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */


#define CM_IF_NAME_NONE "unnamed"

#if defined(CONFIG_NET_INTERFACE_NAME)
#define CM_MAX_IF_NAME MAX(sizeof(CM_IF_NAME_NONE), CONFIG_NET_INTERFACE_NAME_LEN)
#else
#define CM_MAX_IF_NAME sizeof(CM_IF_NAME_NONE)
#endif

#define CM_MAX_IF_INFO (CM_MAX_IF_NAME + 40)


/* Parsing and printing helpers. None of these are used unless CONFIG_NET_CONNECTION_MANAGER
 * is enabled.
 */

#if defined(CONFIG_NET_CONNECTION_MANAGER)

enum cm_type {
	CM_TARG_IFACE,
	CM_TARG_NONE,
	CM_TARG_ALL,
	CM_TARG_INVALID,
};
struct cm_target {
	enum cm_type type;
	struct net_if *iface;
};
enum cm_gs_type {
	CM_GS_GET,
	CM_GS_SET
};
struct cm_flag_string {
	const char *const name;
	enum conn_mgr_if_flag flag;
};
static const struct cm_flag_string flag_strings[] = {
	{"PERSISTENT",		CONN_MGR_IF_PERSISTENT},
	{"NO_AUTO_CONNECT",	CONN_MGR_IF_NO_AUTO_CONNECT},
	{"NO_AUTO_DOWN",	CONN_MGR_IF_NO_AUTO_DOWN},
};

static const char *flag_name(enum conn_mgr_if_flag flag)
{
	/* Scan over predefined flag strings, and return the name
	 * of the first one of matching flag.
	 */
	for (int i = 0; i < ARRAY_SIZE(flag_strings); i++) {
		if (flag_strings[i].flag == flag) {
			return flag_strings[i].name;
		}
	}

	/* No matches found, it's invalid. */
	return "INVALID";
}

static void cm_print_flags(const struct shell *sh)
{
	PR("Valid flag keywords are:\n");
	for (int i = 0; i < ARRAY_SIZE(flag_strings); i++) {
		PR("\t%s,\n", flag_strings[i].name);
	}
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

static void cm_target_help(const struct shell *sh)
{
	PR("Valid target specifiers are 'ifi [index]', 'if [name]', or '[index]'.\n");
}

/* These parsers treat argv as a tokenstream, and increment *argidx by the number of
 * tokens parsed.
 */
static int parse_ifi_target(const struct shell *sh, size_t argc, char *argv[], int *argidx,
			    struct cm_target *target)
{
	char *arg;
	int err = 0;
	unsigned long iface_index;

	/* At least one remaining argument is required to specify a target index */
	if (*argidx >= argc) {
		PR_ERROR("Please specify the target iface index.\n");
		goto error;
	}

	arg = argv[*argidx];

	iface_index = shell_strtoul(arg, 10, &err);

	if (err) {
		PR_ERROR("\"%s\" is not a valid iface index.\n", arg);
		goto error;
	}

	target->iface = net_if_get_by_index(iface_index);

	if (target->iface == NULL) {
		PR_ERROR("iface with index \"%s\" does not exist.\n", arg);
		goto error;
	}

	*argidx += 1;
	target->type = CM_TARG_IFACE;
	return 0;

error:
	target->type = CM_TARG_INVALID;
	return -EINVAL;
}

static int parse_if_target(const struct shell *sh, size_t argc, char *argv[], int *argidx,
			   struct cm_target *target)
{
#if defined(CONFIG_NET_INTERFACE_NAME)
	char *arg;
	/* At least one remaining argument is required to specify a target name */
	if (*argidx >= argc) {
		PR_ERROR("Please specify the target iface name.\n");
		goto error;
	}

	arg = argv[*argidx];

	target->iface = net_if_get_by_index(net_if_get_by_name(arg));

	if (target->iface == NULL) {
		PR_ERROR("iface with name \"%s\" does not exist.\n", arg);
		goto error;
	}

	*argidx += 1;
	target->type = CM_TARG_IFACE;
	return 0;
#else
	PR_ERROR("iface name lookup requires CONFIG_NET_INTERFACE_NAME.\n");
	goto error;
#endif

error:
	target->type = CM_TARG_INVALID;
	return -EINVAL;
}

/* parse `if [iface name]`, `ifi [iface index]`, `[iface index]`, or `all` */
static int parse_target(const struct shell *sh, size_t argc, char *argv[], int *argidx,
			struct cm_target *target)
{
	char *arg;

	/* At least one argument is required to specify a target */
	if (*argidx >= argc) {
		target->type = CM_TARG_NONE;
		return 0;
	}

	arg = argv[*argidx];

	/* At least one argument provided. Is it "all" or "none"? */
	if (strcasecmp(arg, "all") == 0) {
		*argidx += 1;
		target->type = CM_TARG_ALL;
		return 0;
	}

	if (strcasecmp(arg, "none") == 0) {
		*argidx += 1;
		target->type = CM_TARG_NONE;
		return 0;
	}

	/* If not, interpret as an iface index if it is also numeric */
	if (check_numeric(arg)) {
		return parse_ifi_target(sh, argc, argv, argidx, target);
	}

	/* Otherwise, arg must be a target type specifier */
	if (strcasecmp(arg, "if") == 0) {
		*argidx += 1;
		return parse_if_target(sh, argc, argv, argidx, target);
	}

	if (strcasecmp(arg, "ifi") == 0) {
		*argidx += 1;
		return parse_ifi_target(sh, argc, argv, argidx, target);
	}

	PR_ERROR("%s is not a valid target type or target specifier.\n", arg);
	cm_target_help(sh);
	target->type = CM_TARG_INVALID;

	return -EINVAL;
}

static int parse_getset(const struct shell *sh, size_t argc, char *argv[], int *argidx,
			enum cm_gs_type *result)
{
	char *arg;

	/* At least one argument is required to specify get or set */
	if (*argidx >= argc) {
		goto error;
	}

	arg = argv[*argidx];

	if (strcasecmp(arg, "get") == 0) {
		*argidx += 1;
		*result = CM_GS_GET;
		return 0;
	}

	if (strcasecmp(arg, "set") == 0) {
		*argidx += 1;
		*result = CM_GS_SET;
		return 0;
	}

error:
	PR_ERROR("Please specify get or set\n");
	return -EINVAL;
}

static int parse_flag(const struct shell *sh, size_t argc, char *argv[], int *argidx,
		      enum conn_mgr_if_flag *result)
{
	char *arg;

	/* At least one argument is required to specify get or set */
	if (*argidx >= argc) {
		PR_ERROR("Please specify a flag.\n");
		cm_print_flags(sh);

		return -EINVAL;
	}

	arg = argv[*argidx];

	for (int i = 0; i < ARRAY_SIZE(flag_strings); i++) {
		if (strcasecmp(arg, flag_strings[i].name) == 0) {
			*argidx += 1;
			*result = flag_strings[i].flag;
			return 0;
		}
	}

	PR_ERROR("%s is not a valid flag.\n", arg);
	return -EINVAL;
}

static int parse_bool(const struct shell *sh, size_t argc, char *argv[], int *argidx, bool *result)
{
	char *arg;

	/* At least one argument is required to specify a boolean */
	if (*argidx >= argc) {
		goto error;
	}

	arg = argv[*argidx];

	if (strcasecmp(arg, "yes") == 0 ||
	    strcasecmp(arg, "y") == 0 ||
	    strcasecmp(arg, "1") == 0 ||
	    strcasecmp(arg, "true") == 0) {
		*argidx += 1;
		*result = true;
		return 0;
	}

	if (strcasecmp(arg, "no") == 0 ||
	    strcasecmp(arg, "n") == 0 ||
	    strcasecmp(arg, "0") == 0 ||
	    strcasecmp(arg, "false") == 0) {
		*argidx += 1;
		*result = false;
		return 0;
	}

error:
	PR_ERROR("Please specify true or false.\n");
	return -EINVAL;
}

static int parse_timeout(const struct shell *sh, size_t argc, char *argv[], int *argidx,
			 int *result)
{
	char *arg;
	int err = 0;
	unsigned long value;

	/* At least one argument is required to specify a timeout */
	if (*argidx >= argc) {
		PR_ERROR("Please specify a timeout (in seconds).\n");
		return -EINVAL;
	}

	arg = argv[*argidx];

	/* Check for special keyword "none" */
	if (strcasecmp(arg, "none") == 0) {
		*argidx += 1;
		*result = CONN_MGR_IF_NO_TIMEOUT;
		return 0;
	}

	/* Otherwise, try to parse integer timeout (seconds). */
	if (!check_numeric(arg)) {
		PR_ERROR("%s is not a valid timeout.\n", arg);
		return -EINVAL;
	}

	value = shell_strtoul(arg, 10, &err);
	if (err) {
		PR_ERROR("%s is not a valid timeout.\n", arg);
		return -EINVAL;
	}

	*argidx += 1;
	*result = value;
	return 0;
}

static void cm_get_iface_info(struct net_if *iface, char *buf, size_t len)
{
#if  defined(CONFIG_NET_INTERFACE_NAME)
	char name[CM_MAX_IF_NAME];

	if (net_if_get_name(iface, name, sizeof(name))) {
		strcpy(name, CM_IF_NAME_NONE);
	}

	snprintk(buf, len, "%d (%p - %s - %s)", net_if_get_by_iface(iface), iface, name,
						iface2str(iface, NULL));
#else
	snprintk(buf, len, "%d (%p - %s)", net_if_get_by_iface(iface), iface,
					   iface2str(iface, NULL));
#endif
}

/* bulk iface actions */
static void cm_iface_status(struct net_if *iface, void *user_data)
{
	const struct shell *sh = user_data;
	uint16_t state = conn_mgr_if_state(iface);
	bool ignored;
	bool bound;
	bool admin_up;
	bool oper_up;
	bool has_ipv4;
	bool has_ipv6;
	bool connected;
	char iface_info[CM_MAX_IF_INFO];
	char *ip_state;

	cm_get_iface_info(iface, iface_info, sizeof(iface_info));

	if (state == CONN_MGR_IF_STATE_INVALID) {
		PR("iface %s not tracked.\n", iface_info);
	} else {
		ignored = state & CONN_MGR_IF_IGNORED;
		bound = conn_mgr_if_is_bound(iface);
		admin_up = net_if_is_admin_up(iface);
		oper_up = state & CONN_MGR_IF_UP;
		has_ipv4 = state & CONN_MGR_IF_IPV4_SET;
		has_ipv6 = state & CONN_MGR_IF_IPV6_SET;
		connected = state & CONN_MGR_IF_READY;

		if (has_ipv4 && has_ipv6) {
			ip_state = "IPv4 + IPv6";
		} else if (has_ipv4) {
			ip_state = "IPv4";
		} else if (has_ipv6) {
			ip_state = "IPv6";
		} else {
			ip_state = "no IP";
		}

		PR("iface %s status: %s, %s, %s, %s, %s, %s.\n", iface_info,
		   ignored ?	"ignored"	: "watched",
		   bound ?	"bound"		: "not bound",
		   admin_up ?	"admin-up"	: "admin-down",
		   oper_up ?	"oper-up"	: "oper-down",
		   ip_state,
		   connected ?	"connected"	: "not connected");
	}
}

static void cm_iface_ignore(struct net_if *iface, void *user_data)
{
	const struct shell *sh = user_data;
	char iface_info[CM_MAX_IF_INFO];

	cm_get_iface_info(iface, iface_info, sizeof(iface_info));

	conn_mgr_ignore_iface(iface);
	PR("iface %s now ignored.\n", iface_info);
}

static void cm_iface_watch(struct net_if *iface, void *user_data)
{
	const struct shell *sh = user_data;
	char iface_info[CM_MAX_IF_INFO];

	cm_get_iface_info(iface, iface_info, sizeof(iface_info));

	conn_mgr_watch_iface(iface);
	PR("iface %s now watched.\n", iface_info);
}

#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */

static void not_available(const struct shell *sh)
{
	PR_INFO("This command is not available unless CONFIG_NET_CONNECTION_MANAGER is enabled.\n");
}

#endif /* !defined(CONFIG_NET_CONNECTION_MANAGER) */


/* Commands */

static int cmd_net_cm_status(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		/* no need to print anything, parse_target already explained the issue */
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	if (target.type == CM_TARG_NONE || target.type == CM_TARG_ALL) {
		net_if_foreach(cm_iface_status, (void *)sh);
		return 0;
	}

	if (target.type == CM_TARG_IFACE) {
		cm_iface_status(target.iface, (void *)sh);
		return 0;
	}

	PR_ERROR("Invalid target selected.\n");
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	PR_INFO("conn_mgr is not enabled. Enable by setting CONFIG_NET_CONNECTION_MANAGER=y.\n");
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_ignore(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		/* no need to print anything, parse_target already explained the issue */
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		cm_target_help(sh);
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR("Ignoring all ifaces.\n");
		net_if_foreach(cm_iface_ignore, (void *)sh);
		return 0;
	}

	if (target.type == CM_TARG_IFACE) {
		cm_iface_ignore(target.iface, (void *)sh);
		return 0;
	}

	PR_ERROR("Invalid target selected.\n");
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_watch(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		/* no need to print anything, parse_target already explained the issue */
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		cm_target_help(sh);
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR("Watching all ifaces.\n");
		net_if_foreach(cm_iface_watch, (void *)sh);
		return 0;
	}

	if (target.type == CM_TARG_IFACE) {
		cm_iface_watch(target.iface, (void *)sh);
		return 0;
	}

	PR_ERROR("Invalid target selected.\n");
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_connect(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};
	char iface_info[CM_MAX_IF_INFO];

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		/* no need to print anything, parse_target already explained the issue */
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		cm_target_help(sh);
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR("Instructing all non-ignored ifaces to connect.\n");
		conn_mgr_all_if_connect(true);
		return 0;
	}

	if (target.type == CM_TARG_IFACE) {
		cm_get_iface_info(target.iface, iface_info, sizeof(iface_info));

		if (!conn_mgr_if_is_bound(target.iface)) {
			PR_ERROR("iface %s is not bound to a connectivity implementation, cannot "
				 "connect.\n", iface_info);
			return 0;
		}

		PR("Instructing iface %s to connect.\n", iface_info);
		conn_mgr_if_connect(target.iface);
		return 0;
	}

	PR_ERROR("Invalid target selected.\n");
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};
	char iface_info[CM_MAX_IF_INFO];

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		/* no need to print anything, parse_target already explained the issue */
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		cm_target_help(sh);
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR("Instructing all non-ignored ifaces to disconnect.\n");
		conn_mgr_all_if_disconnect(true);
		return 0;
	}

	if (target.type == CM_TARG_IFACE) {
		cm_get_iface_info(target.iface, iface_info, sizeof(iface_info));

		if (!conn_mgr_if_is_bound(target.iface)) {
			PR_ERROR("iface %s is not bound to a connectivity implementation, cannot "
				 "disconnect.\n", iface_info);
			return 0;
		}

		PR("Instructing iface %s to disonnect.\n", iface_info);
		conn_mgr_if_disconnect(target.iface);
		return 0;
	}

	PR_ERROR("Invalid target selected.\n");
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_up(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};
	char iface_info[CM_MAX_IF_INFO];

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		/* no need to print anything, parse_target already explained the issue */
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR("Taking all non-ignored ifaces admin-up.\n");
		conn_mgr_all_if_up(true);
		return 0;
	}

	if (target.type == CM_TARG_IFACE) {
		cm_get_iface_info(target.iface, iface_info, sizeof(iface_info));
		PR("Taking iface %s admin-up.\n", iface_info);
		PR_WARNING("This command duplicates 'net iface up' if [target] != all.\n");

		net_if_up(target.iface);
		return 0;
	}

	PR_ERROR("Invalid target selected.\n");
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_down(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};
	char iface_info[CM_MAX_IF_INFO];

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		/* no need to print anything, parse_target already explained the issue */
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		cm_target_help(sh);
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR("Taking all non-ignored ifaces admin-down.\n");
		conn_mgr_all_if_down(true);
		return 0;
	}

	if (target.type == CM_TARG_IFACE) {
		cm_get_iface_info(target.iface, iface_info, sizeof(iface_info));
		PR("Taking iface %s admin-down.\n", iface_info);
		PR_WARNING("This command duplicates 'net iface down' if [target] != all.\n");

		net_if_down(target.iface);
		return 0;
	}

	PR_ERROR("Invalid target selected.\n");
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_flag(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	enum cm_gs_type getset = CM_GS_GET;
	enum conn_mgr_if_flag flag = CONN_MGR_IF_PERSISTENT;
	bool value = false;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};
	char iface_info[CM_MAX_IF_INFO];

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		cm_target_help(sh);
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR_ERROR("Cannot get/set flags for all ifaces.\n");
		return 0;
	}

	if (target.type != CM_TARG_IFACE) {
		PR_ERROR("Invalid target selected.\n");
		return 0;
	}

	if (parse_getset(sh, argc, argv, &argidx, &getset)) {
		return 0;
	}

	if (parse_flag(sh, argc, argv, &argidx, &flag)) {
		return 0;
	}

	/* If we are in set mode, expect the value to be provided. */
	if (getset == CM_GS_SET && parse_bool(sh, argc, argv, &argidx, &value)) {
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	cm_get_iface_info(target.iface, iface_info, sizeof(iface_info));

	if (!conn_mgr_if_is_bound(target.iface)) {
		PR_ERROR("iface %s is not bound to a connectivity implementation, cannot "
			 "get/set connectivity flag.\n", iface_info);
		return 0;
	}

	if (getset == CM_GS_SET) {
		(void)conn_mgr_if_set_flag(target.iface, flag, value);
		PR("Set the connectivity %s flag to %s on iface %s.\n", flag_name(flag),
		   value?"y":"n", iface_info);
	} else {
		value = conn_mgr_if_get_flag(target.iface, flag);
		PR("The current value of the %s connectivity flag on iface %s is %s.\n",
		   flag_name(flag), iface_info, value?"y":"n");
	}
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

static int cmd_net_cm_timeout(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONNECTION_MANAGER)
	int argidx = 1;
	enum cm_gs_type getset = CM_GS_GET;
	int value = CONN_MGR_IF_NO_TIMEOUT;
	struct cm_target target = {
		.type = CM_TARG_INVALID
	};
	char iface_info[CM_MAX_IF_INFO];

	if (parse_target(sh, argc, argv, &argidx, &target)) {
		return 0;
	}

	if (target.type == CM_TARG_NONE) {
		PR_ERROR("Please specify a target.\n");
		cm_target_help(sh);
		return 0;
	}

	if (target.type == CM_TARG_ALL) {
		PR_ERROR("Cannot get/set timeout for all ifaces.\n");
		return 0;
	}

	if (target.type != CM_TARG_IFACE) {
		PR_ERROR("Invalid target selected.\n");
		return 0;
	}

	if (parse_getset(sh, argc, argv, &argidx, &getset)) {
		return 0;
	}

	/* If we are in set mode, expect the value to be provided. */
	if (getset == CM_GS_SET && parse_timeout(sh, argc, argv, &argidx, &value)) {
		return 0;
	}

	if (argidx != argc) {
		PR_ERROR("Too many args.\n");
		return 0;
	}

	cm_get_iface_info(target.iface, iface_info, sizeof(iface_info));

	if (!conn_mgr_if_is_bound(target.iface)) {
		PR_ERROR("iface %s is not bound to a connectivity implementation, cannot "
			 "get/set connectivity timeout.\n", iface_info);
		return 0;
	}

	if (getset == CM_GS_SET) {
		(void)conn_mgr_if_set_timeout(target.iface, value);
		PR("Set the connectivity timeout for iface %s to %d%s.\n", iface_info, value,
		   value == 0 ? " (no timeout)":" seconds");
	} else {
		value = conn_mgr_if_get_timeout(target.iface);
		PR("The connectivity timeout for iface %s is %d%s.\n", iface_info, value,
		   value == 0 ? " (no timeout)":" seconds");
	}
#else /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	not_available(sh);
#endif /* defined(CONFIG_NET_CONNECTION_MANAGER) */
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_cm,
	SHELL_CMD_ARG(status, NULL,
		  "'net cm status [target]' shows the connectivity status of the specified "
		  "iface(s).",
		  cmd_net_cm_status, 1, 2),
	SHELL_CMD_ARG(ignore, NULL,
		  "'net cm ignore [target]' ignores the specified iface(s).",
		  cmd_net_cm_ignore, 1, 2),
	SHELL_CMD_ARG(watch, NULL,
		  "'net cm watch [target]' watches the specified iface(s).",
		  cmd_net_cm_watch, 1, 2),
	SHELL_CMD_ARG(connect, NULL,
		  "'net cm connect [target]' connects the specified iface(s).",
		  cmd_net_cm_connect, 1, 2),
	SHELL_CMD_ARG(disconnect, NULL,
		  "'net cm disconnect [target]' disconnects the specified iface(s).",
		  cmd_net_cm_disconnect, 1, 2),
	SHELL_CMD_ARG(up, NULL,
		  "'net cm up [target]' takes the specified iface(s) admin-up.",
		  cmd_net_cm_up, 1, 2),
	SHELL_CMD_ARG(down, NULL,
		  "'net cm down [target]' takes the specified iface(s) admin-down.",
		  cmd_net_cm_down, 1, 2),
	SHELL_CMD_ARG(flag, NULL,
		  "'net cm flag [target] [get/set] [flag] [value]' gets or sets a flag "
		  "for the specified iface.",
		  cmd_net_cm_flag, 1, 5),
	SHELL_CMD_ARG(timeout, NULL,
		  "'net cm timeout [target] [get/set] [value]' gets or sets the timeout "
		  "for the specified iface.",
		  cmd_net_cm_timeout, 1, 4),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), cm, &net_cmd_cm, "Control conn_mgr.", NULL, 1, 0);
