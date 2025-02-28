/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <stdlib.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arm64/lib_helpers.h>
#include <zephyr/sys/printk.h>
#include <zephyr/shell/shell.h>
#include <zephyr/logging/log.h>
#include <zephyr/zvm/zvm.h>
#include <zephyr/zvm/os.h>
#include <zephyr/zvm/vm.h>
#include <zephyr/zvm/vm_device.h>
#include <zephyr/zvm/vm_manager.h>
#include <zephyr/zvm/vdev/vserial.h>


LOG_MODULE_DECLARE(ZVM_MODULE_NAME);

#define SHELL_HELP_ZVM "ZVM manager command. " \
	"Some subcommand you can choice as below:\n"
#define SHELL_HELP_CREATE_NEW_VM "Create a vm.\n" \
	"You can use 'zvm new -t zephyr' or 'linux' to create a new vm.\n"
#define SHELL_HELP_RUN_VM "Run a created vm.\n" \
	"You can use 'zvm run -n 0' to run vm with vmid equal to 0.\n"
#define SHELL_HELP_LIST_VM "List all vm info.\n" \
	"You can use 'zvm info' to list all vm info.\n" \
	"You can use 'zvm info -n 0' to list vm info with vmid equal to 0.\n"
#define SHELL_HELP_PAUSE_VM "Pause a vm.\n" \
	"You can use 'zvm pause -n 0' to pause vm with vmid equal to 0.\n"
#define SHELL_HELP_DELETE_VM "Delete a vm.\n" \
	"You can use 'zvm delete -n 0' to delete vm with vmid equal to 0.\n"
#define SHELL_HELP_UPDATE_VM "Update vm.\n" \
	"vm update is not supported now.\n"
#define SHELL_HELP_CONNECT_VIRTUAL_SERIAL "Switch virtual serial.\n" \
	"You can use 'zvm look 0' to connect available virtual serial.\n"

static struct k_spinlock shell_vmops_lock;

static int cmd_zvm_new(const struct shell *zvm_shell, size_t argc, char **argv)
{
	int ret = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&shell_vmops_lock);
	shell_fprintf(zvm_shell, SHELL_NORMAL, "Ready to create a new vm...\n");

	ret = zvm_new_guest(argc, argv);
	if (ret) {
		shell_fprintf(zvm_shell, SHELL_NORMAL,
			"Create vm failured, please follow the message and try again!\n");
		k_spin_unlock(&shell_vmops_lock, key);
		return ret;
	}
	k_spin_unlock(&shell_vmops_lock, key);

	return ret;
}


static int cmd_zvm_run(const struct shell *zvm_shell, size_t argc, char **argv)
{
	/* Run vm code. */
	int ret = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&shell_vmops_lock);

	ret = zvm_run_guest(argc, argv);
	if (ret) {
		shell_fprintf(zvm_shell, SHELL_NORMAL,
			"Start vm failured, please follow the message and try again!\n");
		k_spin_unlock(&shell_vmops_lock, key);
		return ret;
	}

	k_spin_unlock(&shell_vmops_lock, key);

	return ret;
}


static int cmd_zvm_pause(const struct shell *zvm_shell, size_t argc, char **argv)
{
	int ret = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&shell_vmops_lock);
	ret = zvm_pause_guest(argc, argv);
	if (ret) {
		shell_fprintf(zvm_shell, SHELL_NORMAL,
			"Pause vm failured, please follow the message and try again!\n");
		k_spin_unlock(&shell_vmops_lock, key);
		return ret;
	}

	k_spin_unlock(&shell_vmops_lock, key);

	return ret;
}


static int cmd_zvm_delete(const struct shell *zvm_shell, size_t argc, char **argv)
{
	int ret = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&shell_vmops_lock);

	/* Delete vm code. */
	ret = zvm_delete_guest(argc, argv);
	if (ret) {
		shell_fprintf(zvm_shell, SHELL_NORMAL,
			"Delete vm failured, please follow the message and try again!\n");
		k_spin_unlock(&shell_vmops_lock, key);
		return ret;
	}
	k_spin_unlock(&shell_vmops_lock, key);

	return ret;
}


static int cmd_zvm_info(const struct shell *zvm_shell, size_t argc, char **argv)
{
	int ret = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&shell_vmops_lock);

	/* Delete vm code. */
	ret = zvm_info_guest(argc, argv);
	if (ret) {
		shell_fprintf(zvm_shell, SHELL_NORMAL,
			"List vm failured.\n There may no vm in the system!\n");
		k_spin_unlock(&shell_vmops_lock, key);
		return ret;
	}
	k_spin_unlock(&shell_vmops_lock, key);

	return 0;
}


static int cmd_zvm_update(const struct shell *zvm_shell, size_t argc, char **argv)
{
	/* Update vm code. */
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(zvm_shell, SHELL_NORMAL,
			"Update vm is not support now, Please try other command.\n");
	return 0;
}

/* Add subcommand for Root0 command zvm. */
SHELL_STATIC_SUBCMD_SET_CREATE(m_sub_zvm,
	SHELL_CMD(new, NULL, SHELL_HELP_CREATE_NEW_VM, cmd_zvm_new),
	SHELL_CMD(run, NULL, SHELL_HELP_RUN_VM, cmd_zvm_run),
	SHELL_CMD(pause, NULL, SHELL_HELP_PAUSE_VM, cmd_zvm_pause),
	SHELL_CMD(delete, NULL, SHELL_HELP_DELETE_VM, cmd_zvm_delete),
	SHELL_CMD(info, NULL, SHELL_HELP_LIST_VM, cmd_zvm_info),
	SHELL_CMD(update, NULL, SHELL_HELP_UPDATE_VM, cmd_zvm_update),
#ifdef CONFIG_ENABLE_VM_VSERIAL
	SHELL_CMD(look, NULL, SHELL_HELP_CONNECT_VIRTUAL_SERIAL, switch_virtual_serial_handler),
#endif
	SHELL_SUBCMD_SET_END
);

/* Add command for hypervisor. */
SHELL_CMD_REGISTER(zvm, &m_sub_zvm, SHELL_HELP_ZVM, NULL);
