/*
 * Copyright (c) 2026 Caleb Perkinson
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/canbus/j1939.h>
#include <J1939Ac.h>
// #include <J1939Ma.h>
// #include <J1939Tp.h>
#include <J1939_Cfg.h>

#ifdef J1939DM13_ENABLE
#include <J1939Dm13.h>
#endif

#ifdef J1939_NAME_MANAGEMENT
#include <J1939Nm.h>
#endif


extern struct j1939_dt_node_cfg j1939_nodes[];

static const uint8_t J1939_App_RevPrefix[] = "RV0001,,01.00";
static const uint8_t J1939_App_SwAssemblyPrefix[] = "SW0001";
static const uint8_t J1939_App_ApcPrefix[] = "AC0101";
static const uint8_t J1939_App_AppPrefix[] = "AP0101";

static const uint8_t J1939_App_SwAssemblyPart[] = "?";
static const uint8_t J1939_App_SwAssemblyVersion[] = "?";

static const uint8_t J1939_App_Comma[] = ",";
static const uint8_t J1939_App_Pound[] = "#"; // Not used at this time
static const uint8_t J1939_App_Asterisk[] = "*";
static const uint8_t J1939_App_Dot[] = ".";

/// @brief Copies a string from source to destination, not including a trailing NULL
/// @param source Buffer to copy from
/// @param destination Buffer to copy to
/// @return Pointer into destination where the next character goes
static uint8_t *J1939_App_AddStringNoNull(const uint8_t *source,
                                                 uint8_t *destination);

// Routing table for J1939 Message Routing.  This is global to ease reuse.
const J1939_RoutingCallback_T J1939_App_RoutingTable[256] = {
    /* PF = 0x0x00(  0)   */ J1939_DefaultRoute,
    /* PF = 0x0x01(  1)   */ J1939_DefaultRoute,
    /* PF = 0x0x02(  2)   */ J1939_DefaultRoute,
    /* PF = 0x0x03(  3)   */ J1939_DefaultRoute,
    /* PF = 0x0x04(  4)   */ J1939_DefaultRoute,
    /* PF = 0x0x05(  5)   */ J1939_DefaultRoute,
    /* PF = 0x0x06(  6)   */ J1939_DefaultRoute,
    /* PF = 0x0x07(  7)   */ J1939_DefaultRoute,
    /* PF = 0x0x08(  8)   */ J1939_DefaultRoute,
    /* PF = 0x0x09(  9)   */ J1939_DefaultRoute,
    /* PF = 0x0x0A( 10)   */ J1939_DefaultRoute,
    /* PF = 0x0x0B( 11)   */ J1939_DefaultRoute,
    /* PF = 0x0x0C( 12)   */ J1939_DefaultRoute,
    /* PF = 0x0x0D( 13)   */ J1939_DefaultRoute,
    /* PF = 0x0x0E( 14)   */ J1939_DefaultRoute,
    /* PF = 0x0x0F( 15)   */ J1939_DefaultRoute,
    /* PF = 0x0x10( 16)   */ J1939_DefaultRoute,
    /* PF = 0x0x11( 17)   */ J1939_DefaultRoute,
    /* PF = 0x0x12( 18)   */ J1939_DefaultRoute,
    /* PF = 0x0x13( 19)   */ J1939_DefaultRoute,
    /* PF = 0x0x14( 20)   */ J1939_DefaultRoute,
    /* PF = 0x0x15( 21)   */ J1939_DefaultRoute,
    /* PF = 0x0x16( 22)   */ J1939_DefaultRoute,
    /* PF = 0x0x17( 23)   */ J1939_DefaultRoute,
    /* PF = 0x0x18( 24)   */ J1939_DefaultRoute,
    /* PF = 0x0x19( 25)   */ J1939_DefaultRoute,
    /* PF = 0x0x1A( 26)   */ J1939_DefaultRoute,
    /* PF = 0x0x1B( 27)   */ J1939_DefaultRoute,
    /* PF = 0x0x1C( 28)   */ J1939_DefaultRoute,
    /* PF = 0x0x1D( 29)   */ J1939_DefaultRoute,
    /* PF = 0x0x1E( 30)   */ J1939_DefaultRoute,
    /* PF = 0x0x1F( 31)   */ J1939_DefaultRoute,
    /* PF = 0x0x20( 32)   */ J1939_DefaultRoute,
    /* PF = 0x0x21( 33)   */ J1939_DefaultRoute,
    /* PF = 0x0x22( 34)   */ J1939_DefaultRoute,
    /* PF = 0x0x23( 35)   */ J1939_DefaultRoute,
    /* PF = 0x0x24( 36)   */ J1939_DefaultRoute,
    /* PF = 0x0x25( 37)   */ J1939_DefaultRoute,
    /* PF = 0x0x26( 38)   */ J1939_DefaultRoute,
    /* PF = 0x0x27( 39)   */ J1939_DefaultRoute,
    /* PF = 0x0x28( 40)   */ J1939_DefaultRoute,
    /* PF = 0x0x29( 41)   */ J1939_DefaultRoute,
    /* PF = 0x0x2A( 42)   */ J1939_DefaultRoute,
    /* PF = 0x0x2B( 43)   */ J1939_DefaultRoute,
    /* PF = 0x0x2C( 44)   */ J1939_DefaultRoute,
    /* PF = 0x0x2D( 45)   */ J1939_DefaultRoute,
    /* PF = 0x0x2E( 46)   */ J1939_DefaultRoute,
    /* PF = 0x0x2F( 47)   */ J1939_DefaultRoute,
    /* PF = 0x0x30( 48)   */ J1939_DefaultRoute,
    /* PF = 0x0x31( 49)   */ J1939_DefaultRoute,
    /* PF = 0x0x32( 50)   */ J1939_DefaultRoute,
    /* PF = 0x0x33( 51)   */ J1939_DefaultRoute,
    /* PF = 0x0x34( 52)   */ J1939_DefaultRoute,
    /* PF = 0x0x35( 53)   */ J1939_DefaultRoute,
    /* PF = 0x0x36( 54)   */ J1939_DefaultRoute,
    /* PF = 0x0x37( 55)   */ J1939_DefaultRoute,
    /* PF = 0x0x38( 56)   */ J1939_DefaultRoute,
    /* PF = 0x0x39( 57)   */ J1939_DefaultRoute,
    /* PF = 0x0x3A( 58)   */ J1939_DefaultRoute,
    /* PF = 0x0x3B( 59)   */ J1939_DefaultRoute,
    /* PF = 0x0x3C( 60)   */ J1939_DefaultRoute,
    /* PF = 0x0x3D( 61)   */ J1939_DefaultRoute,
    /* PF = 0x0x3E( 62)   */ J1939_DefaultRoute,
    /* PF = 0x0x3F( 63)   */ J1939_DefaultRoute,
    /* PF = 0x0x40( 64)   */ J1939_DefaultRoute,
    /* PF = 0x0x41( 65)   */ J1939_DefaultRoute,
    /* PF = 0x0x42( 66)   */ J1939_DefaultRoute,
    /* PF = 0x0x43( 67)   */ J1939_DefaultRoute,
    /* PF = 0x0x44( 68)   */ J1939_DefaultRoute,
    /* PF = 0x0x45( 69)   */ J1939_DefaultRoute,
    /* PF = 0x0x46( 70)   */ J1939_DefaultRoute,
    /* PF = 0x0x47( 71)   */ J1939_DefaultRoute,
    /* PF = 0x0x48( 72)   */ J1939_DefaultRoute,
    /* PF = 0x0x49( 73)   */ J1939_DefaultRoute,
    /* PF = 0x0x4A( 74)   */ J1939_DefaultRoute,
    /* PF = 0x0x4B( 75)   */ J1939_DefaultRoute,
    /* PF = 0x0x4C( 76)   */ J1939_DefaultRoute,
    /* PF = 0x0x4D( 77)   */ J1939_DefaultRoute,
    /* PF = 0x0x4E( 78)   */ J1939_DefaultRoute,
    /* PF = 0x0x4F( 79)   */ J1939_DefaultRoute,
    /* PF = 0x0x50( 80)   */ J1939_DefaultRoute,
    /* PF = 0x0x51( 81)   */ J1939_DefaultRoute,
    /* PF = 0x0x52( 82)   */ J1939_DefaultRoute,
    /* PF = 0x0x53( 83)   */ J1939_DefaultRoute,
    /* PF = 0x0x54( 84)   */ J1939_DefaultRoute,
    /* PF = 0x0x55( 85)   */ J1939_DefaultRoute,
    /* PF = 0x0x56( 86)   */ J1939_DefaultRoute,
    /* PF = 0x0x57( 87)   */ J1939_DefaultRoute,
    /* PF = 0x0x58( 88)   */ J1939_DefaultRoute,
    /* PF = 0x0x59( 89)   */ J1939_DefaultRoute,
    /* PF = 0x0x5A( 90)   */ J1939_DefaultRoute,
    /* PF = 0x0x5B( 91)   */ J1939_DefaultRoute,
    /* PF = 0x0x5C( 92)   */ J1939_DefaultRoute,
    /* PF = 0x0x5D( 93)   */ J1939_DefaultRoute,
    /* PF = 0x0x5E( 94)   */ J1939_DefaultRoute,
    /* PF = 0x0x5F( 95)   */ J1939_DefaultRoute,
    /* PF = 0x0x60( 96)   */ J1939_DefaultRoute,
    /* PF = 0x0x61( 97)   */ J1939_DefaultRoute,
    /* PF = 0x0x62( 98)   */ J1939_DefaultRoute,
    /* PF = 0x0x63( 99)   */ J1939_DefaultRoute,
    /* PF = 0x0x64(100)   */ J1939_DefaultRoute,
    /* PF = 0x0x65(101)   */ J1939_DefaultRoute,
    /* PF = 0x0x66(102)   */ J1939_DefaultRoute,
    /* PF = 0x0x67(103)   */ J1939_DefaultRoute,
    /* PF = 0x0x68(104)   */ J1939_DefaultRoute,
    /* PF = 0x0x69(105)   */ J1939_DefaultRoute,
    /* PF = 0x0x6A(106)   */ J1939_DefaultRoute,
    /* PF = 0x0x6B(107)   */ J1939_DefaultRoute,
    /* PF = 0x0x6C(108)   */ J1939_DefaultRoute,
    /* PF = 0x0x6D(109)   */ J1939_DefaultRoute,
    /* PF = 0x0x6E(110)   */ J1939_DefaultRoute,
    /* PF = 0x0x6F(111)   */ J1939_DefaultRoute,
    /* PF = 0x0x70(112)   */ J1939_DefaultRoute,
    /* PF = 0x0x71(113)   */ J1939_DefaultRoute,
    /* PF = 0x0x72(114)   */ J1939_DefaultRoute,
    /* PF = 0x0x73(115)   */ J1939_DefaultRoute,
    /* PF = 0x0x74(116)   */ J1939_DefaultRoute,
    /* PF = 0x0x75(117)   */ J1939_DefaultRoute,
    /* PF = 0x0x76(118)   */ J1939_DefaultRoute,
    /* PF = 0x0x77(119)   */ J1939_DefaultRoute,
    /* PF = 0x0x78(120)   */ J1939_DefaultRoute,
    /* PF = 0x0x79(121)   */ J1939_DefaultRoute,
    /* PF = 0x0x7A(122)   */ J1939_DefaultRoute,
    /* PF = 0x0x7B(123)   */ J1939_DefaultRoute,
    /* PF = 0x0x7C(124)   */ J1939_DefaultRoute,
    /* PF = 0x0x7D(125)   */ J1939_DefaultRoute,
    /* PF = 0x0x7E(126)   */ J1939_DefaultRoute,
    /* PF = 0x0x7F(127)   */ J1939_DefaultRoute,
    /* PF = 0x0x80(128)   */ J1939_DefaultRoute,
    /* PF = 0x0x81(129)   */ J1939_DefaultRoute,
    /* PF = 0x0x82(130)   */ J1939_DefaultRoute,
    /* PF = 0x0x83(131)   */ J1939_DefaultRoute,
    /* PF = 0x0x84(132)   */ J1939_DefaultRoute,
    /* PF = 0x0x85(133)   */ J1939_DefaultRoute,
    /* PF = 0x0x86(134)   */ J1939_DefaultRoute,
    /* PF = 0x0x87(135)   */ J1939_DefaultRoute,
    /* PF = 0x0x88(136)   */ J1939_DefaultRoute,
    /* PF = 0x0x89(137)   */ J1939_DefaultRoute,
    /* PF = 0x0x8A(138)   */ J1939_DefaultRoute,
    /* PF = 0x0x8B(139)   */ J1939_DefaultRoute,
    /* PF = 0x0x8C(140)   */ J1939_DefaultRoute,
    /* PF = 0x0x8D(141)   */ J1939_DefaultRoute,
    /* PF = 0x0x8E(142)   */ J1939_DefaultRoute,
    /* PF = 0x0x8F(143)   */ J1939_DefaultRoute,
    /* PF = 0x0x90(144)   */ J1939_DefaultRoute,
    /* PF = 0x0x91(145)   */ J1939_DefaultRoute,
    /* PF = 0x0x92(146)   */ J1939_DefaultRoute,
    /* PF = 0x0x93(147)   */ J1939_DefaultRoute,
    /* PF = 0x0x94(148)   */ J1939_DefaultRoute,
    /* PF = 0x0x95(149)   */ J1939_DefaultRoute,
    /* PF = 0x0x96(150)   */ J1939_DefaultRoute,
    /* PF = 0x0x97(151)   */ J1939_DefaultRoute,
    /* PF = 0x0x98(152)   */ J1939_DefaultRoute,
    /* PF = 0x0x99(153)   */ J1939_DefaultRoute,
    /* PF = 0x0x9A(154)   */ J1939_DefaultRoute,
    /* PF = 0x0x9B(155)   */ J1939_DefaultRoute,
    /* PF = 0x0x9C(156)   */ J1939_DefaultRoute,
    /* PF = 0x0x9D(157)   */ J1939_DefaultRoute,
    /* PF = 0x0x9E(158)   */ J1939_DefaultRoute,
    /* PF = 0x0x9F(159)   */ J1939_DefaultRoute,
    /* PF = 0x0xA0(160)   */ J1939_DefaultRoute,
    /* PF = 0x0xA1(161)   */ J1939_DefaultRoute,
    /* PF = 0x0xA2(162)   */ J1939_DefaultRoute,
    /* PF = 0x0xA3(163)   */ J1939_DefaultRoute,
    /* PF = 0x0xA4(164)   */ J1939_DefaultRoute,
    /* PF = 0x0xA5(165)   */ J1939_DefaultRoute,
    /* PF = 0x0xA6(166)   */ J1939_DefaultRoute,
    /* PF = 0x0xA7(167)   */ J1939_DefaultRoute,
    /* PF = 0x0xA8(168)   */ J1939_DefaultRoute,
    /* PF = 0x0xA9(169)   */ J1939_DefaultRoute,
    /* PF = 0x0xAA(170)   */ J1939_DefaultRoute,
    /* PF = 0x0xAB(171)   */ J1939_DefaultRoute,
    /* PF = 0x0xAC(172)   */ J1939_DefaultRoute,
    /* PF = 0x0xAD(173)   */ J1939_DefaultRoute,
    /* PF = 0x0xAE(174)   */ J1939_DefaultRoute,
    /* PF = 0x0xAF(175)   */ J1939_DefaultRoute,
    /* PF = 0x0xB0(176)   */ J1939_DefaultRoute,
    /* PF = 0x0xB1(177)   */ J1939_DefaultRoute,
    /* PF = 0x0xB2(178)   */ J1939_DefaultRoute,
    /* PF = 0x0xB3(179)   */ J1939_DefaultRoute,
    /* PF = 0x0xB4(180)   */ J1939_DefaultRoute,
    /* PF = 0x0xB5(181)   */ J1939_DefaultRoute,
    /* PF = 0x0xB6(182)   */ J1939_DefaultRoute,
    /* PF = 0x0xB7(183)   */ J1939_DefaultRoute,
    /* PF = 0x0xB8(184)   */ J1939_DefaultRoute,
    /* PF = 0x0xB9(185)   */ J1939_DefaultRoute,
    /* PF = 0x0xBA(186)   */ J1939_DefaultRoute,
    /* PF = 0x0xBB(187)   */ J1939_DefaultRoute,
    /* PF = 0x0xBC(188)   */ J1939_DefaultRoute,
    /* PF = 0x0xBD(189)   */ J1939_DefaultRoute,
    /* PF = 0x0xBE(190)   */ J1939_DefaultRoute,
    /* PF = 0x0xBF(191)   */ J1939_DefaultRoute,
    /* PF = 0x0xC0(192)   */ J1939_DefaultRoute,
    /* PF = 0x0xC1(193)   */ J1939_DefaultRoute,
    /* PF = 0x0xC2(194)   */ J1939_DefaultRoute,
    /* PF = 0x0xC3(195)   */ J1939_DefaultRoute,
    /* PF = 0x0xC4(196)   */ J1939_DefaultRoute,
    /* PF = 0x0xC5(197)   */ J1939_DefaultRoute,
    /* PF = 0x0xC6(198)   */ J1939_DefaultRoute,
    /* PF = 0x0xC7(199)   */ J1939_DefaultRoute,
    /* PF = 0x0xC8(200)   */ J1939_DefaultRoute,
    /* PF = 0x0xC9(201)   */ J1939_DefaultRoute,
    /* PF = 0x0xCA(202)   */ J1939_DefaultRoute,
    /* PF = 0x0xCB(203)   */ J1939_DefaultRoute,
    /* PF = 0x0xCC(204)   */ J1939_DefaultRoute,
    /* PF = 0x0xCD(205)   */ J1939_DefaultRoute,
    /* PF = 0x0xCE(206)   */ J1939_DefaultRoute,
    /* PF = 0x0xCF(207)   */ J1939_DefaultRoute,
    /* PF = 0x0xD0(208)   */ J1939_DefaultRoute,
    /* PF = 0x0xD1(209)   */ J1939_DefaultRoute,
    /* PF = 0x0xD2(210)   */ J1939_DefaultRoute,
    /* PF = 0x0xD3(211)   */ J1939_DefaultRoute,
    /* PF = 0x0xD4(212)   */ J1939_DefaultRoute,
    /* PF = 0x0xD5(213)   */ J1939_DefaultRoute,
    /* PF = 0x0xD6(214)   */ J1939_DefaultRoute,

#ifdef J1939_MEMORY_ACCESS
    /* PF = 0x0xD7(215)   */ J1939Ma_Dm16Process,
#else
    /* PF = 0x0xD7(215)   */ J1939_DefaultRoute,
#endif

#if (defined(J1939_MEMORY_ACCESS) && defined(J1939MA_TRANSMIT_READ_WRITE_REQUEST))
    /* PF = 0x0xD8(216)   */ J1939Ma_Dm15Response,
#else
    /* PF = 0x0xD8(216)   */ J1939_DefaultRoute,
#endif

#ifdef J1939_MEMORY_ACCESS
    /* PF = 0x0xD9(217)   */ J1939Ma_Dm14Request,
#else
    /* PF = 0x0xD9(217)   */ J1939_DefaultRoute,
#endif

    /* PF = 0x0xDA(218)   */ J1939_DefaultRoute,
    /* PF = 0x0xDB(219)   */ J1939_DefaultRoute,
    /* PF = 0x0xDC(220)   */ J1939_DefaultRoute,
    /* PF = 0x0xDD(221)   */ J1939_DefaultRoute,
    /* PF = 0x0xDE(222)   */ J1939_DefaultRoute,

#ifdef J1939DM13_ENABLE
    /* PF = 0x0xDF(223)   */ J1939Dm13_Receive,
#else
    /* PF = 0x0xDF(223)   */ J1939_DefaultRoute,
#endif

    /* PF = 0x0xE0(224)   */ J1939_DefaultRoute,
    /* PF = 0x0xE1(225)   */ J1939_DefaultRoute,
    /* PF = 0x0xE2(226)   */ J1939_DefaultRoute,
    /* PF = 0x0xE3(227)   */ J1939_DefaultRoute,
    /* PF = 0x0xE4(228)   */ J1939_DefaultRoute,
    /* PF = 0x0xE5(229)   */ J1939_DefaultRoute,
    /* PF = 0x0xE6(230)   */ J1939_DefaultRoute,
    /* PF = 0x0xE7(231)   */ J1939_DefaultRoute,
    /* PF = 0x0xE8(232)   */ J1939_DefaultRoute,
    /* PF = 0x0xE9(233)   */ J1939_DefaultRoute,
    /* PF = 0x0xEA(234)   */ J1939_RequestPgn,
#ifdef J1939_TRANSPORT_PROTOCOL
    /* PF = 0x0xEB(235)   */ J1939Tp_DataTransfer,
    /* PF = 0x0xEC(236)   */ J1939Tp_ConnectionManagement,
#else
    /* PF = 0x0xEB(235)   */ J1939_DefaultRoute,
    /* PF = 0x0xEC(236)   */ J1939_DefaultRoute,
#endif

    /* PF = 0x0xED(237)   */ J1939_DefaultRoute,
    /* PF = 0x0xEE(238)   */ J1939Ac_IsReceived,
    /* PF = 0x0xEF(239)   */ J1939_DefaultRoute,
    /* PF = 0x0xF0(240)   */ J1939_DefaultRoute,
    /* PF = 0x0xF1(241)   */ J1939_DefaultRoute,
    /* PF = 0x0xF2(242)   */ J1939_DefaultRoute,
    /* PF = 0x0xF3(243)   */ J1939_DefaultRoute,
    /* PF = 0x0xF4(244)   */ J1939_DefaultRoute,
    /* PF = 0x0xF5(245)   */ J1939_DefaultRoute,
    /* PF = 0x0xF6(246)   */ J1939_DefaultRoute,
    /* PF = 0x0xF7(247)   */ J1939_DefaultRoute,
    /* PF = 0x0xF8(248)   */ J1939_DefaultRoute,
    /* PF = 0x0xF9(249)   */ J1939_DefaultRoute,
    /* PF = 0x0xFA(250)   */ J1939_DefaultRoute,
    /* PF = 0x0xFB(251)   */ J1939_DefaultRoute,
    /* PF = 0x0xFC(252)   */ J1939_DefaultRoute,
    /* PF = 0x0xFD(253)   */ J1939_DefaultRoute,
    /* PF = 0x0xFE(254)   */ J1939_PduF254Process,
    /* PF = 0x0xFF(255)   */ J1939_DefaultRoute};

/**************************************************************************************************/
void J1939_App_Init(void)
{
   for (uint16_t index = 0; index < J1939_DT_NUM_NODES; index++)
   {
      J1939_Node_T node = &(j1939_nodes[index]);
      // Register the PGN requests we need to handle
      (void)J1939_RegisterRequestPgn(J1939_SOFTWARE_ID_PGN, node);
      (void)J1939_RegisterRequestPgn(J1939_ECU_ID_INFO_PGN, node);
   }
}

/**************************************************************************************************/
void J1939_App_Task(void)
{
   J1939_SourceAddress_T requestedSource;

   for (uint16_t index = 0; index < J1939_DT_NUM_NODES; index++)
   {
      J1939_Node_T node = &(j1939_nodes[index]);

    //   if (J1939_IsPgnRequested(J1939_ECU_ID_INFO_PGN, &requestedSource, node))
    //   {
    //      J1939_App_TransmitEcuId((J1939_DestinationAddress_T)requestedSource, node);
    //   }
    //   if (J1939_IsPgnRequested(J1939_SOFTWARE_ID_PGN, &requestedSource, node))
    //   {
    //      J1939_App_TransmitSoftwareId((J1939_DestinationAddress_T)requestedSource, node);
    //   }
#ifdef J1939_NAME_MANAGEMENT
      if (J1939_IsPgnRequested(J1939NM_PGN, &requestedSource, node))
      {
         J1939Nm_PgnRequestHandler((J1939_DestinationAddress_T)requestedSource, node);
      }
#endif
   }
}

/**************************************************************************************************/
bool J1939_App_Pf254Process(const struct can_frame *message, J1939_Node_T node)
{
   // If we don't handle the message, we should let the default handler handle it.
   return J1939_DefaultRoute(message, node);
}

/**************************************************************************************************/
void J1939_App_TransmitEcuId(J1939_DestinationAddress_T destination, J1939_Node_T node)
{
   // To minimize stack usage we allocate a buffer from the Transport Layer
   // The code below does not perform any range checking.  The application developer is responsible
   // for allocating a buffer large enough to hold the largest possible string.
   uint8_t *data =
       J1939Tp_GetBuffer(150);      // Pointer to the beginning of the transport buffer
   uint8_t *tempData = data; // Temporary pointer used to build up the string

   char hardwarePart[16];
   char hardwareSerial[16];
   char hardwareRevision[16];

   // Check the pointers are not NULL.
   // Pointers are to the same location, so if one is NULL, both are NULL.
   // If only one pointer is checked, then Lint complains.
   // If the compiler is detecting this is always true, silence the warning
#ifdef __TASKING__
#pragma warning 549
#endif
   if (tempData && data)
#ifdef __TASKING__
#pragma warning restore
#endif
   {
      // TODO: To prepare ECU_ID_PGN data, fill the transport buffer
      // TODO: Get hardware part number
      // TODO: Get hardware revision number
      // TODO: Get hardware serial number
      tempData = J1939_App_AddStringNoNull((uint8_t *)hardwarePart, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Comma, tempData);
      tempData = J1939_App_AddStringNoNull((uint8_t *)hardwareRevision, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Asterisk, tempData);
      tempData = J1939_App_AddStringNoNull((uint8_t *)hardwareSerial, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Asterisk, tempData);
      // ECU Location
      tempData = J1939_App_AddStringNoNull(J1939_App_Asterisk, tempData);
      // ECU Type
      tempData = J1939_App_AddStringNoNull(J1939_App_Asterisk, tempData);
      // ECU Manufacturer Name
      tempData = J1939_App_AddStringNoNull(J1939_App_Asterisk, tempData);
      // ECU Hardware ID
      tempData = J1939_App_AddStringNoNull(J1939_App_Asterisk, tempData);

      // Note that if J1939_TransmitPgn() succeeds, the transport layer will take care of freeing
      // the buffer we allocated when the transport session is completed.  If it fails, we need
      // to take care of it ourselves
      if (!(J1939_TransmitPgn(
              J1939_Priority_6, J1939_ECU_ID_INFO_PGN, destination, data,
              //lint --e(613) if tempData or data are null, this function will not be called
              (J1939_Counter_T)(tempData - data), node)))
      {
         J1939Tp_FreeBuffer(data);
      }
   }
} //lint !e438

/**************************************************************************************************/
void J1939_App_TransmitSoftwareId(J1939_DestinationAddress_T destination, J1939_Node_T node)
{
   // To minimize stack usage we allocate a buffer from the Transport Layer
   // The code below does not perform any range checking.  The application developer is responsible
   // for allocating a buffer large enough to hold the largest possible string.
   uint8_t *data =
       J1939Tp_GetBuffer(150);      // Pointer to the beginning of the transport buffer
   uint8_t *tempData = data; // Temporary pointer used to build up the string

   char softwarePart[16];
   char softwareVersion[16];

   // Check the pointers are not NULL.
   // Pointers are to the same location, so if one is NULL, both are NULL.
   // If only one pointer is checked, then Lint complains.
   // If the compiler is detecting this is always true, silence the warning
#ifdef __TASKING__
#pragma warning 549
#endif
   if (tempData && data)
#ifdef __TASKING__
#pragma warning restore
#endif
   {
      // TODO: Set up your Software ID information in this array
      // Note there are a number of projects that have built these up properly from PCD information
      // You may want to look at those implementations for examples
      *tempData++ = 1;
      // RV: version of s/w id that this Controller uses
      // Note that this matches the current JDOS/JDCBB format, but not the VNSM database
      tempData = J1939_App_AddStringNoNull(J1939_App_RevPrefix, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Pound, tempData);
      // SW: Software assembly 0001
      tempData = J1939_App_AddStringNoNull(J1939_App_SwAssemblyPrefix, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Comma, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_SwAssemblyPart, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Comma, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_SwAssemblyVersion, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Pound, tempData);
      // AP: Application 0101 (Application SW)
      tempData = J1939_App_AddStringNoNull(J1939_App_AppPrefix, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Comma, tempData);
      // TODO: Get software part number
      tempData = J1939_App_AddStringNoNull((uint8_t *)softwarePart, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Comma, tempData);
      // TODO: Get software version number
      tempData = J1939_App_AddStringNoNull((uint8_t *)softwareVersion, tempData);
      tempData = J1939_App_AddStringNoNull(J1939_App_Pound, tempData);
      // AC: Application Config 0101 (Application SW Details)
      // tempData = J1939_App_AddStringNoNull(J1939_App_ApcPrefix, tempData);
      // tempData = J1939_App_AddStringNoNull(J1939_App_Comma, tempData);
      // tempData = J1939_App_AddStringNoNull(AppBuildData, tempData);
      // tempData = J1939_App_AddStringNoNull(J1939_App_Comma, tempData);
      // tempData = J1939_App_AddStringNoNull(AppVersionNumber, tempData);
      // tempData = J1939_App_AddStringNoNull(J1939_App_Pound, tempData);
      // '*' at the end of the entire set of s/w assembly
      tempData = J1939_App_AddStringNoNull(J1939_App_Asterisk, tempData);

      // Note that if J1939_TransmitPgn() succeeds, the transport layer will take care of freeing
      // the buffer we allocated when the transport session is completed.  If it fails, we need
      // to take care of it ourselves
      if (!(J1939_TransmitPgn(
              J1939_Priority_6,      // Priority.
              J1939_SOFTWARE_ID_PGN, // PGN.
              destination,           // Source Address to reply to
              data,                  // Data pointer.
              //lint --e(613) if tempData or data are null, this function will not be called
              (J1939_Counter_T)(tempData - data), // Byte counter for pointer.
              node)))                             // CAN Node.
      {
         J1939Tp_FreeBuffer(data);
      }
   }
} //lint !e438

/**************************************************************************************************/
J1939_Node_T J1939_App_GetJ1939NodeFromSourceAddress(J1939_SourceAddress_T address)
{
   J1939_Node_T node;

//    TODO: probably could return a pointer to the can dev?
//    for (J1939_Node_T index = 0; index < J1939_NUM_NODES; index++)
//    {
//       if (address == J1939Ac_GetSourceAddress(index))
//       {
//          node = index;
//          break;
//       }
//       else if (index == J1939_NUM_NODES - 1)
//       {
//          node = J1939_NUM_NODES + 1;
//          break;
//       }
//    }

   return node;
}

/**************************************************************************************************/
static uint8_t *J1939_App_AddStringNoNull(const uint8_t *source,
                                                 uint8_t *destination)
{
   // We will copy from the source, to the destination, up to, but not including the NULL pointer
   while (*source != 0)
   {
      *destination++ = *source++;
   }

   return destination;
}
