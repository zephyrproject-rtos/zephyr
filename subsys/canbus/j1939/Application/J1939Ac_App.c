/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include <J1939Ac.h>
#include <J1939Timer.h>
#include <J1939_Cfg.h>

#ifdef J1939_NAME_MANAGEMENT
#include "J1939Nm.h"
#endif

//lint -esym(522, J1939Ac_App_Init)
//lint -esym(522, J1939Ac_App_ConfigureArbitraryAddressClaimAddresses)

// TODO : Use these defines to configure your J1939 NAME in J1939Ac_App_GenerateName()
/// INDUSTRY_GROUP: (3 bit field)
///          0 - Global
///          1 - On Highway Equipment
///          2 - Agricultural and Forestry Equipment
///          3 - Construction Equipment
///          4 - Marine
///          5 - Industrial-Process Control-Stationary (Gen-Sets)
///          6 - Reserved
///          7 - Reserved
#define INDUSTRY_GROUP (0U)

/// (4 bit field) - Must be a value between zero and 15, inclusive
#define VEHICLE_SYSTEM_INSTANCE (0U)

/// (7 bit field) - See the J1939 top-level document, Appendix B, Table B12 for complete details
#define VEHICLE_SYSTEM (0U) // Non-specific system

/// (8 bit field) - See J1939 top level document, Appendix B, Table B11 for complete details
#define FUNCTION (0U)

/// (5 bit field) - Must be between zero and 31, inclusive
#define FUNCTION_INSTANCE (0U)

/// (3 bit field) - Must be between zero and seven, inclusive
#define ECU_INSTANCE (0U)

/// (11 bit field) - For complete details see the J1939 (top-level document) Appendix B, Table B10
#define MANUFACTURER_CODE (0)

#ifdef J1939AC_SELF_CONFIGURABLE
static void J1939Ac_App_ConfigureArbitraryAddressClaimAddresses(void);
#endif

/**************************************************************************************************/
void J1939Ac_App_Init(void)
{
#ifdef J1939AC_SELF_CONFIGURABLE
   J1939Ac_App_ConfigureArbitraryAddressClaimAddresses();
#endif

   // TODO: Add any application specific code required here
}

/**************************************************************************************************/
void J1939Ac_App_Task(const struct can_frame *message)
{
   (void)message;
   // TODO: Add any application specific code required here
}

/**************************************************************************************************/
void J1939Ac_App_GenerateName(J1939_Node_T node)
{
   J1939_Name_T name;

   // TODO Build up the name for this module if not using USE_DEFINED_NAME

#ifdef J1939_NAME_MANAGEMENT
      J1939Nm_GetCurrentName(node, &name);
#else
      // TODO This number should be pulled from the hardware serial number or other similar
      // information
      name.idNumber = 0;

      name.mfgCode = MANUFACTURER_CODE;
      name.ecuInstance = ECU_INSTANCE;
      name.functionInstance = FUNCTION_INSTANCE;
      name.function = FUNCTION;
      name.vehicleSystem = VEHICLE_SYSTEM;
      name.vehicleSystemInstance = VEHICLE_SYSTEM_INSTANCE;
      name.industryGroup = INDUSTRY_GROUP;
#ifdef J1939AC_SELF_CONFIGURABLE
      name.isSelfConfig = true;
#else
      name.isSelfConfig = false;
#endif
      name.reservedBit = 0;

#endif
      (void)J1939Ac_SetNameConfigNode(node, &name);
}

/**************************************************************************************************/
J1939_SourceAddress_T J1939Ac_App_GetDefaultSourceAddress(J1939_Node_T node)
{
   // This function returns the source address the unit should use to attempt an address claim.
   // It can be hard-coded, stored in EEPROM, determined by strapping pins, etc.  This function
   // MUST be able to determine the default source address prior to the J1939 layer being
   // initialized.

    J1939_SourceAddress_T source = J1939_NULL_ADDRESS;
    source  = (J1939_SourceAddress_T)node->source_address;
    return source;
}

#ifdef J1939AC_SELF_CONFIGURABLE
/**************************************************************************************************/
static void J1939Ac_App_ConfigureArbitraryAddressClaimAddresses(void)
{
   J1939_Counter_T currentIndex = 0;

   for (J1939_Node_T node = 0; node < CAN_NUM_NODES; node++)
   {
      for (J1939_Counter_T listIndex = 0; listIndex < J1939AC_ALT_ADDRESS_LIST_LENGTH; listIndex++)
      {
         // Leave the default address out of the list
         if (J1939_NODE_ADDRESS_0 != (J1939AC_ARBITRARY_MIN_SOURCE_ADDRESS + listIndex))
         {
            J1939Ac_SetArbitraryListEntry(
                node, currentIndex,
                (J1939_SourceAddress_T)(J1939AC_ARBITRARY_MIN_SOURCE_ADDRESS + listIndex));
            currentIndex++;
         }
      }
   }
}
#endif
