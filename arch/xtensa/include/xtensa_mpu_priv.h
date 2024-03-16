/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_XTENSA_XTENSA_MPU_PRIV_H_
#define ZEPHYR_ARCH_XTENSA_XTENSA_MPU_PRIV_H_

#include <stdint.h>

#include <zephyr/toolchain.h>
#include <zephyr/arch/xtensa/mpu.h>
#include <zephyr/sys/util_macro.h>

#include <xtensa/config/core-isa.h>

/**
 * @defgroup xtensa_mpu_internal_apis Xtensa Memory Protection Unit (MPU) Internal APIs
 * @ingroup xtensa_mpu_apis
 * @{
 */

/**
 * @name Bit shifts and masks for MPU entry registers.
 *
 * @{
 */

/**
 * Number of bits to shift for start address in MPU entry register.
 *
 * This is only used for aligning the value to the MPU entry register,
 * and is different than the hardware alignment requirement.
 */
#define XTENSA_MPU_ENTRY_REG_START_ADDR_SHIFT		5U

/**
 * Bit mask of start address in MPU entry register.
 *
 * This is only used for aligning the value to the MPU entry register,
 * and is different than the hardware alignment requirement.
 */
#define XTENSA_MPU_ENTRY_REG_START_ADDR_MASK		0xFFFFFFE0U

/** Number of bits to shift for enable bit in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_ENABLE_SHIFT		0U

/** Bit mask of enable bit in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_ENABLE_MASK		BIT(XTENSA_MPU_ENTRY_ENABLE_SHIFT)

/** Number of bits to shift for lock bit in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_LOCK_SHIFT			1U

/** Bit mask of lock bit in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_LOCK_MASK			BIT(XTENSA_MPU_ENTRY_LOCK_SHIFT)

/** Number of bits to shift for access rights in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_ACCESS_RIGHTS_SHIFT	8U

/** Bit mask of access rights in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_ACCESS_RIGHTS_MASK		\
	(0xFU << XTENSA_MPU_ENTRY_REG_ACCESS_RIGHTS_SHIFT)

/** Number of bits to shift for memory type in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_MEMORY_TYPE_SHIFT		12U

/** Bit mask of memory type in MPU entry register. */
#define XTENSA_MPU_ENTRY_REG_MEMORY_TYPE_MASK		\
	(0x1FFU << XTENSA_MPU_ENTRY_REG_MEMORY_TYPE_SHIFT)

/**
 * @}
 */

/**
 * Define one MPU entry of type struct xtensa_mpu_entry.
 *
 * @note This needs a comma at the end if used in array declaration.
 *
 * @param saddr Start address.
 * @param en Enable bit
 * @param rights Access rights.
 * @param memtype Memory type.
 */
#define XTENSA_MPU_ENTRY(saddr, en, rights, memtype) \
	{ \
		.as.p.enable = en, \
		.as.p.lock = 0, \
		.as.p.mbz = 0, \
		.as.p.start_addr = (saddr >> XTENSA_MPU_ENTRY_START_ADDR_SHIFT), \
		.at.p.segment = 0, \
		.at.p.mbz1 = 0, \
		.at.p.access_rights = rights, \
		.at.p.memory_type = memtype, \
		.at.p.mbz2 = 0, \
	}

/**
 * @brief Read MPUCFG register.
 *
 * This returns the bitmask of enabled MPU entries (foreground segments).
 *
 * @return Value of MPUCFG register.
 */
static ALWAYS_INLINE uint32_t xtensa_mpu_mpucfg_read(void)
{
	uint32_t mpucfg;

	__asm__ __volatile__("rsr.mpucfg %0" : "=a" (mpucfg));

	return mpucfg;
}

/**
 * @brief Read MPUENB register.
 *
 * This returns the enable bits for MPU entries.
 *
 * @return Value of MPUENB register.
 */
static ALWAYS_INLINE uint32_t xtensa_mpu_mpuenb_read(void)
{
	uint32_t mpuenb;

	__asm__ __volatile__("rsr.mpuenb %0" : "=a" (mpuenb));

	return mpuenb;
}

/**
 * @brief Write MPUENB register.
 *
 * This writes the enable bits for MPU entries.
 *
 * @param mpuenb Value to be written.
 */
static ALWAYS_INLINE void xtensa_mpu_mpuenb_write(uint32_t mpuenb)
{
	__asm__ __volatile__("wsr.mpuenb %0" : : "a"(mpuenb));
}

/**
 * @name MPU entry internal helper functions.
 *
 * @{
 */

/**
 * @brief Return the start address encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 *
 * @return Start address.
 */
static ALWAYS_INLINE
uintptr_t xtensa_mpu_entry_start_address_get(const struct xtensa_mpu_entry *entry)
{
	return (entry->as.p.start_addr << XTENSA_MPU_ENTRY_REG_START_ADDR_SHIFT);
}

/**
 * @brief Set the start address encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 * @param addr Start address.
 */
static ALWAYS_INLINE
void xtensa_mpu_entry_start_address_set(struct xtensa_mpu_entry *entry, uintptr_t addr)
{
	entry->as.p.start_addr = addr >> XTENSA_MPU_ENTRY_REG_START_ADDR_SHIFT;
}

/**
 * @brief Return the lock bit encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 *
 * @retval True Lock bit is set.
 * @retval False Lock bit is not set.
 */
static ALWAYS_INLINE
bool xtensa_mpu_entry_lock_get(const struct xtensa_mpu_entry *entry)
{
	return entry->as.p.lock != 0;
}

/**
 * @brief Set the lock bit encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 * @param lock True if to lock the MPU entry.
 */
static ALWAYS_INLINE
void xtensa_mpu_entry_lock_set(struct xtensa_mpu_entry *entry, bool lock)
{
	entry->as.p.lock = lock ? 1 : 0;
}

/**
 * @brief Return the enable bit encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 *
 * @retval True Enable bit is set.
 * @retval False Enable bit is not set.
 */
static ALWAYS_INLINE
bool xtensa_mpu_entry_enable_get(const struct xtensa_mpu_entry *entry)
{
	return entry->as.p.enable != 0;
}

/**
 * @brief Set the enable bit encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 * @param en True if to enable the MPU entry.
 */
static ALWAYS_INLINE
void xtensa_mpu_entry_enable_set(struct xtensa_mpu_entry *entry, bool en)
{
	entry->as.p.enable = en ? 1 : 0;
}

/**
 * @brief Return the access rights encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 *
 * @return Access right value.
 */
static ALWAYS_INLINE
uint8_t xtensa_mpu_entry_access_rights_get(const struct xtensa_mpu_entry *entry)
{
	return entry->at.p.access_rights;
}

/**
 * @brief Set the lock bit encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 * @param access_rights Access rights to be set.
 */
static ALWAYS_INLINE
void xtensa_mpu_entry_access_rights_set(struct xtensa_mpu_entry *entry, uint8_t access_rights)
{
	entry->at.p.access_rights = access_rights;
}

/**
 * @brief Return the memory type encoded in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 *
 * @return Memory type value.
 */
static ALWAYS_INLINE
uint16_t xtensa_mpu_entry_memory_type_get(const struct xtensa_mpu_entry *entry)
{
	return entry->at.p.memory_type;
}

/**
 * @brief Set the memory type in the MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 * @param memory_type Memory type to be set.
 */
static ALWAYS_INLINE
void xtensa_mpu_entry_memory_type_set(struct xtensa_mpu_entry *entry, uint16_t memory_type)
{
	entry->at.p.memory_type = memory_type;
}

/**
 * @brief Set both access rights and memory type of a MPU entry.
 *
 * @param entry Pointer to the MPU entry.
 * @param access_rights Access rights value.
 * @param memory_type Memory type value.
 */
static inline
void xtensa_mpu_entry_attributes_set(struct xtensa_mpu_entry *entry,
				     uint8_t access_rights, uint16_t memory_type)
{
	xtensa_mpu_entry_access_rights_set(entry, access_rights);
	xtensa_mpu_entry_memory_type_set(entry, memory_type);
}

/**
 * @brief Set fields in MPU entry so it will be functional.
 *
 * This sets the starting address, enable bit, access rights and memory type
 * of an entry.
 *
 * Note that this preserves the valud of the segment field.
 *
 * @param entry Pointer to the entry to be manipulated.
 * @param start_address Start address to be set.
 * @param enable Whether this entry should be enabled.
 * @param access_rights Access rights for the entry.
 * @param memory_type Memory type for the entry.
 */
static inline
void xtensa_mpu_entry_set(struct xtensa_mpu_entry *entry, uintptr_t start_address,
			  bool enable, uint8_t access_rights, uint16_t memory_type)
{
	uint8_t segment = entry->at.p.segment;

	/* Clear out the fields, and make sure MBZ fields are zero. */
	entry->as.raw = 0;
	entry->at.raw = 0;

	xtensa_mpu_entry_start_address_set(entry, start_address);
	xtensa_mpu_entry_enable_set(entry, enable);
	xtensa_mpu_entry_access_rights_set(entry, access_rights);
	xtensa_mpu_entry_memory_type_set(entry, memory_type);

	entry->at.p.segment = segment;
}

/**
 * @brief Test if two MPU entries have same access rights.
 *
 * @param entry1 MPU entry #1
 * @param entry2 MPU entry #2.
 *
 * @return True if access rights are the same, false otherwise.
 */
static inline
bool xtensa_mpu_entries_has_same_access_rights(const struct xtensa_mpu_entry *entry1,
					       const struct xtensa_mpu_entry *entry2)
{
	return entry1->at.p.access_rights == entry2->at.p.access_rights;
}

/**
 * @brief Test if two MPU entries have same memory types.
 *
 * @param entry1 MPU entry #1.
 * @param entry2 MPU entry #2.
 *
 * @return True if memory types are the same, false otherwise.
 */
static inline
bool xtensa_mpu_entries_has_same_memory_type(const struct xtensa_mpu_entry *entry1,
					     const struct xtensa_mpu_entry *entry2)
{
	return entry1->at.p.memory_type == entry2->at.p.memory_type;
}

/**
 * @brief Test if two MPU entries have same access rights and memory types.
 *
 * @param entry1 MPU entry #1.
 * @param entry2 MPU entry #2.
 *
 * @return True if access rights and memory types are the same, false otherwise.
 */
static inline
bool xtensa_mpu_entries_has_same_attributes(const struct xtensa_mpu_entry *entry1,
					    const struct xtensa_mpu_entry *entry2)
{
	return xtensa_mpu_entries_has_same_access_rights(entry1, entry2) &&
	       xtensa_mpu_entries_has_same_memory_type(entry1, entry2);
}

/**
 * @brief Test if two entries has the same addresses.
 *
 * @param entry1 MPU entry #1.
 * @param entry2 MPU entry #2.
 *
 * @return True if they have the same address, false otherwise.
 */
static inline
bool xtensa_mpu_entries_has_same_address(const struct xtensa_mpu_entry *entry1,
					 const struct xtensa_mpu_entry *entry2)
{
	return xtensa_mpu_entry_start_address_get(entry1)
	       == xtensa_mpu_entry_start_address_get(entry2);
}

/**
 * @}
 */

/**
 * @name MPU access rights helper functions.
 *
 * @{
 */

/**
 * @brief Test if the access rights is valid.
 *
 * @param access_rights Access rights value.
 *
 * @return True if access rights is valid, false otherwise.
 */
static ALWAYS_INLINE bool xtensa_mpu_access_rights_is_valid(uint8_t access_rights)
{
	return (access_rights != 1) && (access_rights <= 15);
}

/**
 * @}
 */

/**
 * @}
 */

#endif /* ZEPHYR_ARCH_XTENSA_XTENSA_MPU_PRIV_H_ */
