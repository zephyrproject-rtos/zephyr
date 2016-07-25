subdir-ccflags-y +=-I$(srctree)/ext/lib/crypto/tinycrypt/include

obj-$(CONFIG_TINYCRYPT) := source/utils.o
obj-$(CONFIG_TINYCRYPT_ECC_DH) += source/ecc_dh.o source/ecc.o
obj-$(CONFIG_TINYCRYPT_ECC_DSA) += source/ecc_dsa.o source/ecc.o
obj-$(CONFIG_TINYCRYPT_AES) += source/aes_decrypt.o source/aes_encrypt.o
obj-$(CONFIG_TINYCRYPT_AES_CBC) += source/cbc_mode.o
obj-$(CONFIG_TINYCRYPT_AES_CTR) += source/ctr_mode.o
obj-$(CONFIG_TINYCRYPT_AES_CCM) += source/ccm_mode.o
obj-$(CONFIG_TINYCRYPT_AES_CMAC) += source/cmac_mode.o
obj-$(CONFIG_TINYCRYPT_SHA256) += source/sha256.o
obj-$(CONFIG_TINYCRYPT_SHA256_HMAC) += source/hmac.o
obj-$(CONFIG_TINYCRYPT_SHA256_HMAC_PRNG) += source/hmac_prng.o
obj-$(CONFIG_TINYCRYPT_CTR_PRNG) += source/ctr_prng.o
