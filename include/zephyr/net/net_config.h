/** @file
 * @brief Routines for network subsystem initialization.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_CONFIG_H_
#define ZEPHYR_INCLUDE_NET_NET_CONFIG_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ssh/keygen.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Network configuration library
 * @defgroup net_config Network Configuration Library
 * @since 1.8
 * @version 0.8.0
 * @ingroup networking
 * @{
 */

/* Flags that tell what kind of functionality is needed by the client. */
/**
 * @brief Application needs routers to be set so that connectivity to remote
 * network is possible. For IPv6 networks, this means that the device should
 * receive IPv6 router advertisement message before continuing.
 */
#define NET_CONFIG_NEED_ROUTER 0x00000001

/**
 * @brief Application needs IPv6 subsystem configured and initialized.
 * Typically this means that the device has IPv6 address set.
 */
#define NET_CONFIG_NEED_IPV6   0x00000002

/**
 * @brief Application needs IPv4 subsystem configured and initialized.
 * Typically this means that the device has IPv4 address set.
 */
#define NET_CONFIG_NEED_IPV4   0x00000004

/**
 * @brief Initialize this network application.
 *
 * @details This will call net_config_init_by_iface() with NULL network
 *          interface.
 *
 * @param app_info String describing this application.
 * @param flags Flags related to services needed by the client.
 * @param timeout How long to wait the network setup before continuing
 * the startup.
 *
 * @return 0 if ok, <0 if error.
 */
int net_config_init(const char *app_info, uint32_t flags, int32_t timeout);

/**
 * @brief Initialize this network application using a specific network
 * interface.
 *
 * @details If network interface is set to NULL, then the default one
 *          is used in the configuration.
 *
 * @param iface Initialize networking using this network interface.
 * @param app_info String describing this application.
 * @param flags Flags related to services needed by the client.
 * @param timeout How long to wait the network setup before continuing
 * the startup.
 *
 * @return 0 if ok, <0 if error.
 */
int net_config_init_by_iface(struct net_if *iface, const char *app_info,
			     uint32_t flags, int32_t timeout);

/**
 * @brief Initialize this network application.
 *
 * @details If CONFIG_NET_CONFIG_AUTO_INIT is set, then this function is called
 *          automatically when the device boots. If that is not desired, unset
 *          the config option and call the function manually when the
 *          application starts.
 *
 * @param dev Network device to use. The function will figure out what
 *        network interface to use based on the device. If the device is NULL,
 *        then default network interface is used by the function.
 * @param app_info String describing this application.
 *
 * @return 0 if ok, <0 if error.
 */
int net_config_init_app(const struct device *dev, const char *app_info);

/**
 * @brief This contains configuration data for password authentication for sshd on the device.
 */
struct net_config_ssh_password_auth {
	/** Username to use for password authentication. If NULL or empty string,
	 * then username is not used. The username cannot contain spaces.
	 */
	const char *username;

	/** Password to use for password authentication. If NULL or empty string,
	 * password authentication will be disabled and only public key
	 * authentication will be allowed.
	 */
	const char *password;
};

/**
 * This contains key configuration data for sshd on the device.
 */
struct net_config_ssh_key {
	/**
	 * SSH key name. This is used when saving or loading keys to identify them.
	 * The key name cannot contain spaces.
	 */
	const char *key_name;

	/**
	 * Temp buffer for loading and saving keys.
	 */
	uint8_t *key_buf;

	/**
	 * Size of the key buffer in bytes. This is used to determine how big the
	 * buffer is for loading and saving keys.
	 */
	size_t key_buf_len;

	/**
	 * Key id
	 */
	int key_id;

	/**
	 * Key type
	 */
	enum ssh_host_key_type key_type;

	/**
	 * Key size in bits.
	 */
	int key_bits;

	/**
	 * Is private key?
	 */
	bool is_private;
};

/**
 * @brief Temporary key buffer needed for loading and saving keys.
 *
 * @param name Name of the SSH server configuration instance. This is used to
 *	  identify the instance and must be unique if multiple instances are defined.
 * @param key_bits Size of the SSH key in bits.
 */
#define NET_CONFIG_SSH_KEY_BUF_DEFINE_STATIC(name, key_bits)		\
	static uint8_t name[key_bits]

/**
 * @brief Statically define a username and password for SSH server configuration.
 *
 * @param name Name of the SSH server configuration instance. This is used to
 *	  identify the instance and must be unique if multiple instances are defined.
 * @param user_name Username to use for password authentication. If NULL or empty string,
 *	  then username is not used. The username cannot contain spaces.
 * @param pwd Password to use for password authentication. If NULL or empty string,
 *	  password authentication will be disabled and only public key authentication will
 *        be allowed.
 */
#define NET_CONFIG_SSH_PASSWORD_AUTH_DEFINE_STATIC(name, user_name, pwd) \
	static struct net_config_ssh_password_auth name = {		\
		.username = user_name,					\
		.password = pwd,					\
	}

/**
 * @brief Statically define a private key for SSH server configuration.
 *
 * @param name Name of the SSH server configuration instance. This is used to
 *        identify the instance and must be unique if multiple instances are defined.
 * @param _key_buf_name Name of the temporary key buffer defined with
 *        NET_CONFIG_SSH_KEY_BUF_DEFINE_STATIC() macro.
 *        This buffer is needed for loading and saving keys.
 *        The buffer can be shared between private and public key configurations if needed,
 *        but it must be big enough to hold the key data for both configurations.
 * @param _key_name SSH key name. This is used when saving or loading keys to
 *        identify them. The key name cannot contain spaces.
 * @param _key_id Key id. This is used to identify the key.
 * @param _key_type Type of the SSH key. For example RSA key etc.
 * @param _key_bits Size of the SSH key in bits. This is used to determine the
 *        size of the buffer needed for loading and saving keys.
 */
#define NET_CONFIG_SSH_PRIV_KEY_DEFINE_STATIC(name,			\
					      _key_buf_name,		\
					      _key_buf_len,		\
					      _key_name,		\
					      _key_id,			\
					      _key_type,		\
					      _key_bits)		\
	static struct net_config_ssh_key name = {			\
		.key_buf = _key_buf_name,				\
		.key_buf_len = _key_buf_len,				\
		.key_name = _key_name,					\
		.key_id = _key_id,					\
		.key_type = _key_type,					\
		.key_bits = _key_bits,					\
		.is_private = true,					\
	}

/**
 * @brief Statically define a public key for SSH server configuration.
 *
 * @param name Name of the SSH server configuration instance. This is used to
 *        identify the instance and must be unique if multiple instances are defined.
 * @param _key_buf_name Name of the temporary key buffer defined with
 *        NET_CONFIG_SSH_KEY_BUF_DEFINE_STATIC() macro.
 *        This buffer is needed for loading and saving keys.
 *        The buffer can be shared between private and public key configurations if needed,
 *        but it must be big enough to hold the key data for both configurations.
 * @param _key_name SSH key name. This is used when saving or loading keys to
 *        identify them. The key name cannot contain spaces.
 * @param _key_id Key id. This is used to identify the key.
 */
#define NET_CONFIG_SSH_PUB_KEY_DEFINE_STATIC(name,			\
					     _key_buf_name,		\
					     _key_buf_len,		\
					     _key_name,			\
					     _key_id)			\
	static struct net_config_ssh_key name = {			\
		.key_buf = _key_buf_name,				\
		.key_buf_len = _key_buf_len,				\
		.key_name = _key_name,					\
		.key_id = _key_id,					\
		.key_bits = 0,						\
		.is_private = false,					\
	}

/** @brief Initialize SSH server on the device.
 *
 * @param iface Network interface to use for the SSH server. If NULL, the SSH
 *        server will bind to the wildcard address and be reachable through all network
 *        interfaces.
 * @param priv_key_config Private key configuration for the SSH server.
 * @param pub_key_config Public key configuration for the SSH server.
 * @param password_auth_config Password authentication configuration for the SSH server.
 *        If using public key authetication, then this can be left as NULL or
 *        with NULL username and password. The public key authentication will have higher
 *        priority than password authentication, so if both are configured, then password
 *        authentication will only be used if public key authentication fails.
 *        If you want to specify a username for public key authentication, then set the username
 *        in the password_auth_config struct, even if password authentication is not used.
 *        This is because the SSH server needs to know the username to associate it with
 *        the public key.
 * @param sshd_instance SSH server instance number. This is used to identify the SSH
 *        server instance and must be unique if multiple instances are defined.
 *
 * @return 0 if the SSH server was initialized successfully, <0 if there was
 * an error.
 */
#if defined(CONFIG_SSH_SERVER)
int net_config_init_sshd(struct net_if *iface,
			 const struct net_config_ssh_key *priv_key_config,
			 const struct net_config_ssh_key *pub_key_config,
			 const struct net_config_ssh_password_auth *password_auth_config,
			 int sshd_instance);
#else
static inline int net_config_init_sshd(struct net_if *iface,
			const struct net_config_ssh_key *priv_key_config,
			const struct net_config_ssh_key *pub_key_config,
			const struct net_config_ssh_password_auth *password_auth_config,
			int sshd_instance)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(priv_key_config);
	ARG_UNUSED(pub_key_config);
	ARG_UNUSED(password_auth_config);
	ARG_UNUSED(sshd_instance);

	return -ENOTSUP;
}
#endif /* CONFIG_SSH_SERVER */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_NET_CONFIG_H_ */
