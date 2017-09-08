ifdef CONFIG_TINYCRYPT
ZEPHYRINCLUDE += -I$(srctree)/ext/lib/crypto/tinycrypt/include
endif

ifdef CONFIG_MBEDTLS_BUILTIN
ZEPHYRINCLUDE += -I$(srctree)/ext/lib/crypto/mbedtls/include
ZEPHYRINCLUDE += -I$(srctree)/ext/lib/crypto/mbedtls/configs
endif

ifdef CONFIG_MBEDTLS_LIBRARY
include $(srctree)/ext/lib/crypto/mbedtls/Makefile
endif

ifdef CONFIG_TINYCBOR
ZEPHYRINCLUDE += -I$(srctree)/ext/lib/tinycbor/include
endif

ifdef CONFIG_CBORATTR
ZEPHYRINCLUDE += -I$(srctree)/ext/lib/cborattr/include
endif
