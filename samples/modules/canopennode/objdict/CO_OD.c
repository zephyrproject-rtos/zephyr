/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*******************************************************************************

   File - CO_OD.c/CO_OD.h
   CANopen Object Dictionary.

   This file was automatically generated with libedssharp Object
   Dictionary Editor v0.8-7-g2e53bbc   DON'T EDIT THIS FILE MANUALLY !!!!
*******************************************************************************/


#include "CO_driver.h"
#include "CO_OD.h"
#include "CO_SDO.h"

/*******************************************************************************
   DEFINITION AND INITIALIZATION OF OBJECT DICTIONARY VARIABLES
*******************************************************************************/


/***** Definition for ROM variables ********************************************/
struct sCO_OD_ROM CO_OD_ROM = {
	CO_OD_FIRST_LAST_WORD,

/*1000*/ 0x0000L,
/*1005*/ 0x0080L,
/*1006*/ 0x0000L,
/*1007*/ 0x0000L,
/*1008*/ { 'Z', 'e', 'p', 'h', 'y', 'r', ' ', 'R', 'T', 'O', 'S', '/', 'C', 'A', 'N', 'o', 'p', 'e', 'n', 'N', 'o', 'd', 'e' },
/*1009*/ { '3', '.', '0', '0' },
/*100a*/ { '3', '.', '0', '0' },
/*1012*/ 0x0000L,
/*1014*/ 0x0080L,
/*1015*/ 0x64,
/*1016*/ { 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
/*1017*/ 0x3e8,
/*1018*/ { 0x4L, 0x0000L, 0x0000L, 0x0000L, 0x0000L },
/*1019*/ 0x0L,
/*1029*/ { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00 },
/*1200*/ { { 0x2L, 0x0600L, 0x0580L } },
/*1400*/ { { 0x2L, 0x0200L, 0xfeL },
/*1401*/ { 0x2L, 0x0300L, 0xfeL },
/*1402*/ { 0x2L, 0x0400L, 0xfeL },
/*1403*/ { 0x2L, 0x0500L, 0xfeL } },
/*1600*/ { { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L },
/*1601*/ { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L },
/*1602*/ { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L },
/*1603*/ { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L } },
/*1800*/ { { 0x6L, 0x0180L, 0xfeL, 0x00, 0x0L, 0x00, 0x0L },
/*1801*/ { 0x6L, 0x0280L, 0xfeL, 0x00, 0x0L, 0x00, 0x0L },
/*1802*/ { 0x6L, 0x0380L, 0xfeL, 0x00, 0x0L, 0x00, 0x0L },
/*1803*/ { 0x6L, 0x0480L, 0xfeL, 0x00, 0x0L, 0x00, 0x0L } },
/*1a00*/ { { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L },
/*1a01*/ { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L },
/*1a02*/ { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L },
/*1a03*/ { 0x0L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L, 0x0000L } },
/*1f80*/ 0x0000L,

	CO_OD_FIRST_LAST_WORD,
};


/***** Definition for RAM variables ********************************************/
struct sCO_OD_RAM CO_OD_RAM = {
	CO_OD_FIRST_LAST_WORD,

/*1001*/ 0x0L,
/*1002*/ 0x0000L,
/*1003*/ { 0, 0, 0, 0, 0, 0, 0, 0 },
/*1010*/ { 0x00000003 },
/*1011*/ { 0x00000001 },
/*1f50*/ { 0 },
/*1f51*/ { 0x0L },
/*1f56*/ { 0x0000L },
/*1f57*/ { 0x0000L },
/*2100*/ { 0x0L, 0x0L, 0x0L, 0x0L, 0x0L, 0x0L, 0x0L, 0x0L, 0x0L, 0x0L },
/*2102*/ 0x0000L,

	CO_OD_FIRST_LAST_WORD,
};


/***** Definition for EEPROM variables ********************************************/
struct sCO_OD_EEPROM CO_OD_EEPROM = {
	CO_OD_FIRST_LAST_WORD,

/*2101*/ 0x0000L,

	CO_OD_FIRST_LAST_WORD,
};




/*******************************************************************************
   STRUCTURES FOR RECORD TYPE OBJECTS
*******************************************************************************/


/*0x1018*/ const CO_OD_entryRecord_t OD_record1018[5] = {
	{ (void *)&CO_OD_ROM.identity.maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.identity.vendorID, 0x85, 0x4 },
	{ (void *)&CO_OD_ROM.identity.productCode, 0x85, 0x4 },
	{ (void *)&CO_OD_ROM.identity.revisionNumber, 0x85, 0x4 },
	{ (void *)&CO_OD_ROM.identity.serialNumber, 0x85, 0x4 },
};

/*0x1200*/ const CO_OD_entryRecord_t OD_record1200[3] = {
	{ (void *)&CO_OD_ROM.SDOServerParameter[0].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.SDOServerParameter[0].COB_IDClientToServer, 0x85, 0x4 },
	{ (void *)&CO_OD_ROM.SDOServerParameter[0].COB_IDServerToClient, 0x85, 0x4 },
};

/*0x1400*/ const CO_OD_entryRecord_t OD_record1400[3] = {
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[0].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[0].COB_IDUsedByRPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[0].transmissionType, 0x0d, 0x1 },
};

/*0x1401*/ const CO_OD_entryRecord_t OD_record1401[3] = {
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[1].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[1].COB_IDUsedByRPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[1].transmissionType, 0x0d, 0x1 },
};

/*0x1402*/ const CO_OD_entryRecord_t OD_record1402[3] = {
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[2].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[2].COB_IDUsedByRPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[2].transmissionType, 0x0d, 0x1 },
};

/*0x1403*/ const CO_OD_entryRecord_t OD_record1403[3] = {
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[3].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[3].COB_IDUsedByRPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOCommunicationParameter[3].transmissionType, 0x0d, 0x1 },
};

/*0x1600*/ const CO_OD_entryRecord_t OD_record1600[9] = {
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[0].mappedObject8, 0x8d, 0x4 },
};

/*0x1601*/ const CO_OD_entryRecord_t OD_record1601[9] = {
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[1].mappedObject8, 0x8d, 0x4 },
};

/*0x1602*/ const CO_OD_entryRecord_t OD_record1602[9] = {
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[2].mappedObject8, 0x8d, 0x4 },
};

/*0x1603*/ const CO_OD_entryRecord_t OD_record1603[9] = {
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.RPDOMappingParameter[3].mappedObject8, 0x8d, 0x4 },
};

/*0x1800*/ const CO_OD_entryRecord_t OD_record1800[7] = {
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[0].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[0].COB_IDUsedByTPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[0].transmissionType, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[0].inhibitTime, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[0].compatibilityEntry, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[0].eventTimer, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[0].SYNCStartValue, 0x0d, 0x1 },
};

/*0x1801*/ const CO_OD_entryRecord_t OD_record1801[7] = {
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[1].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[1].COB_IDUsedByTPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[1].transmissionType, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[1].inhibitTime, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[1].compatibilityEntry, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[1].eventTimer, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[1].SYNCStartValue, 0x0d, 0x1 },
};

/*0x1802*/ const CO_OD_entryRecord_t OD_record1802[7] = {
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[2].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[2].COB_IDUsedByTPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[2].transmissionType, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[2].inhibitTime, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[2].compatibilityEntry, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[2].eventTimer, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[2].SYNCStartValue, 0x0d, 0x1 },
};

/*0x1803*/ const CO_OD_entryRecord_t OD_record1803[7] = {
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[3].maxSubIndex, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[3].COB_IDUsedByTPDO, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[3].transmissionType, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[3].inhibitTime, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[3].compatibilityEntry, 0x05, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[3].eventTimer, 0x8d, 0x2 },
	{ (void *)&CO_OD_ROM.TPDOCommunicationParameter[3].SYNCStartValue, 0x0d, 0x1 },
};

/*0x1a00*/ const CO_OD_entryRecord_t OD_record1a00[9] = {
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[0].mappedObject8, 0x8d, 0x4 },
};

/*0x1a01*/ const CO_OD_entryRecord_t OD_record1a01[9] = {
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[1].mappedObject8, 0x8d, 0x4 },
};

/*0x1a02*/ const CO_OD_entryRecord_t OD_record1a02[9] = {
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[2].mappedObject8, 0x8d, 0x4 },
};

/*0x1a03*/ const CO_OD_entryRecord_t OD_record1a03[9] = {
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].numberOfMappedObjects, 0x0d, 0x1 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject1, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject2, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject3, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject4, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject5, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject6, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject7, 0x8d, 0x4 },
	{ (void *)&CO_OD_ROM.TPDOMappingParameter[3].mappedObject8, 0x8d, 0x4 },
};

/*******************************************************************************
   OBJECT DICTIONARY
*******************************************************************************/
const CO_OD_entry_t CO_OD[45] = {

	{ 0x1000, 0x00, 0x85, 4, (void *)&CO_OD_ROM.deviceType },
	{ 0x1001, 0x00, 0x26, 1, (void *)&CO_OD_RAM.errorRegister },
	{ 0x1002, 0x00, 0xa6, 4, (void *)&CO_OD_RAM.manufacturerStatusRegister },
	{ 0x1003, 0x08, 0x8e, 4, (void *)&CO_OD_RAM.preDefinedErrorField[0] },
	{ 0x1005, 0x00, 0x8d, 4, (void *)&CO_OD_ROM.COB_ID_SYNCMessage },
	{ 0x1006, 0x00, 0x8d, 4, (void *)&CO_OD_ROM.communicationCyclePeriod },
	{ 0x1007, 0x00, 0x8d, 4, (void *)&CO_OD_ROM.synchronousWindowLength },
	{ 0x1008, 0x00, 0x05, 23, (void *)&CO_OD_ROM.manufacturerDeviceName },
	{ 0x1009, 0x00, 0x05, 4, (void *)&CO_OD_ROM.manufacturerHardwareVersion },
	{ 0x100a, 0x00, 0x05, 4, (void *)&CO_OD_ROM.manufacturerSoftwareVersion },
	{ 0x1010, 0x01, 0x8e, 4, (void *)&CO_OD_RAM.storeParameters[0] },
	{ 0x1011, 0x01, 0x8e, 4, (void *)&CO_OD_RAM.restoreDefaultParameters[0] },
	{ 0x1012, 0x00, 0x85, 4, (void *)&CO_OD_ROM.COB_ID_TIME },
	{ 0x1014, 0x00, 0x85, 4, (void *)&CO_OD_ROM.COB_ID_EMCY },
	{ 0x1015, 0x00, 0x8d, 2, (void *)&CO_OD_ROM.inhibitTimeEMCY },
	{ 0x1016, 0x04, 0x8d, 4, (void *)&CO_OD_ROM.consumerHeartbeatTime[0] },
	{ 0x1017, 0x00, 0x8d, 2, (void *)&CO_OD_ROM.producerHeartbeatTime },
	{ 0x1018, 0x04, 0x00, 0, (void *)&OD_record1018 },
	{ 0x1019, 0x00, 0x0d, 1, (void *)&CO_OD_ROM.synchronousCounterOverflowValue },
	{ 0x1029, 0x06, 0x0d, 1, (void *)&CO_OD_ROM.errorBehavior[0] },
	{ 0x1200, 0x02, 0x00, 0, (void *)&OD_record1200 },
	{ 0x1400, 0x02, 0x00, 0, (void *)&OD_record1400 },
	{ 0x1401, 0x02, 0x00, 0, (void *)&OD_record1401 },
	{ 0x1402, 0x02, 0x00, 0, (void *)&OD_record1402 },
	{ 0x1403, 0x02, 0x00, 0, (void *)&OD_record1403 },
	{ 0x1600, 0x08, 0x00, 0, (void *)&OD_record1600 },
	{ 0x1601, 0x08, 0x00, 4, (void *)&OD_record1601 },
	{ 0x1602, 0x08, 0x00, 4, (void *)&OD_record1602 },
	{ 0x1603, 0x08, 0x00, 4, (void *)&OD_record1603 },
	{ 0x1800, 0x06, 0x00, 0, (void *)&OD_record1800 },
	{ 0x1801, 0x06, 0x00, 4, (void *)&OD_record1801 },
	{ 0x1802, 0x06, 0x00, 4, (void *)&OD_record1802 },
	{ 0x1803, 0x06, 0x00, 4, (void *)&OD_record1803 },
	{ 0x1a00, 0x08, 0x00, 0, (void *)&OD_record1a00 },
	{ 0x1a01, 0x08, 0x00, 4, (void *)&OD_record1a01 },
	{ 0x1a02, 0x08, 0x00, 0, (void *)&OD_record1a02 },
	{ 0x1a03, 0x08, 0x00, 0, (void *)&OD_record1a03 },
	{ 0x1f50, 0x01, 0x0a, 0, (void *)0 },
	{ 0x1f51, 0x01, 0x0e, 1, (void *)&CO_OD_RAM.programControl[0] },
	{ 0x1f56, 0x01, 0x86, 4, (void *)&CO_OD_RAM.programSoftwareIdentification[0] },
	{ 0x1f57, 0x01, 0x86, 4, (void *)&CO_OD_RAM.flashStatusIdentification[0] },
	{ 0x1f80, 0x00, 0x8d, 4, (void *)&CO_OD_ROM.NMTStartup },
	{ 0x2100, 0x00, 0x26, 10, (void *)&CO_OD_RAM.errorStatusBits },
	{ 0x2101, 0x00, 0xa7, 4, (void *)&CO_OD_EEPROM.powerOnCounter },
	{ 0x2102, 0x00, 0xfe, 4, (void *)&CO_OD_RAM.buttonPressCounter },
};
