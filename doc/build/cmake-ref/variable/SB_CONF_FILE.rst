SB_CONF_FILE
############

Sysbuild Kconfig configuration file, or list of files.

Defaults to :file:`sysbuild.conf` in :cmake:variable:`APPLICATION_CONFIG_DIR`, if that file
exists. Sysbuild is an opt-in feature, so the file is optional.

Settings in these files override the defaults of sysbuild's own Kconfig tree. Paths are resolved
relative to :cmake:variable:`APP_DIR`.

See :cmake:module:`sysbuild_kconfig`.
