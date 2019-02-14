#include <shell/shell.h>

int dhrystone_func(const struct shell *shell, size_t argc, char **argv)
{
	dhrystone_main(argc, argv);
	return 0;
}

SHELL_CMD_REGISTER(dhrystone, NULL, "dhrystone benchmark", dhrystone_func);

