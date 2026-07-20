/*
 * Copyright (c) 2026 Deere & Company
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/canbus/j1939.h>
#include <J1939Ac.h>
// #include <J1939Ma.h>
#include <J1939Tp.h>

#ifdef J1939DM13_ENABLE
#include <J1939Dm13.h>
#endif

#ifdef CONFIG_J1939_NAME_MANAGEMENT
#include <J1939Nm.h>
#endif


extern struct j1939_dt_node_cfg* j1939_nodes[];

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
const j1939_routing_callback_t J1939_App_RoutingTable[256] = {
    /* PF = 0x0x00(  0)   */ j1939_default_route,
    /* PF = 0x0x01(  1)   */ j1939_default_route,
    /* PF = 0x0x02(  2)   */ j1939_default_route,
    /* PF = 0x0x03(  3)   */ j1939_default_route,
    /* PF = 0x0x04(  4)   */ j1939_default_route,
    /* PF = 0x0x05(  5)   */ j1939_default_route,
    /* PF = 0x0x06(  6)   */ j1939_default_route,
    /* PF = 0x0x07(  7)   */ j1939_default_route,
    /* PF = 0x0x08(  8)   */ j1939_default_route,
    /* PF = 0x0x09(  9)   */ j1939_default_route,
    /* PF = 0x0x0A( 10)   */ j1939_default_route,
    /* PF = 0x0x0B( 11)   */ j1939_default_route,
    /* PF = 0x0x0C( 12)   */ j1939_default_route,
    /* PF = 0x0x0D( 13)   */ j1939_default_route,
    /* PF = 0x0x0E( 14)   */ j1939_default_route,
    /* PF = 0x0x0F( 15)   */ j1939_default_route,
    /* PF = 0x0x10( 16)   */ j1939_default_route,
    /* PF = 0x0x11( 17)   */ j1939_default_route,
    /* PF = 0x0x12( 18)   */ j1939_default_route,
    /* PF = 0x0x13( 19)   */ j1939_default_route,
    /* PF = 0x0x14( 20)   */ j1939_default_route,
    /* PF = 0x0x15( 21)   */ j1939_default_route,
    /* PF = 0x0x16( 22)   */ j1939_default_route,
    /* PF = 0x0x17( 23)   */ j1939_default_route,
    /* PF = 0x0x18( 24)   */ j1939_default_route,
    /* PF = 0x0x19( 25)   */ j1939_default_route,
    /* PF = 0x0x1A( 26)   */ j1939_default_route,
    /* PF = 0x0x1B( 27)   */ j1939_default_route,
    /* PF = 0x0x1C( 28)   */ j1939_default_route,
    /* PF = 0x0x1D( 29)   */ j1939_default_route,
    /* PF = 0x0x1E( 30)   */ j1939_default_route,
    /* PF = 0x0x1F( 31)   */ j1939_default_route,
    /* PF = 0x0x20( 32)   */ j1939_default_route,
    /* PF = 0x0x21( 33)   */ j1939_default_route,
    /* PF = 0x0x22( 34)   */ j1939_default_route,
    /* PF = 0x0x23( 35)   */ j1939_default_route,
    /* PF = 0x0x24( 36)   */ j1939_default_route,
    /* PF = 0x0x25( 37)   */ j1939_default_route,
    /* PF = 0x0x26( 38)   */ j1939_default_route,
    /* PF = 0x0x27( 39)   */ j1939_default_route,
    /* PF = 0x0x28( 40)   */ j1939_default_route,
    /* PF = 0x0x29( 41)   */ j1939_default_route,
    /* PF = 0x0x2A( 42)   */ j1939_default_route,
    /* PF = 0x0x2B( 43)   */ j1939_default_route,
    /* PF = 0x0x2C( 44)   */ j1939_default_route,
    /* PF = 0x0x2D( 45)   */ j1939_default_route,
    /* PF = 0x0x2E( 46)   */ j1939_default_route,
    /* PF = 0x0x2F( 47)   */ j1939_default_route,
    /* PF = 0x0x30( 48)   */ j1939_default_route,
    /* PF = 0x0x31( 49)   */ j1939_default_route,
    /* PF = 0x0x32( 50)   */ j1939_default_route,
    /* PF = 0x0x33( 51)   */ j1939_default_route,
    /* PF = 0x0x34( 52)   */ j1939_default_route,
    /* PF = 0x0x35( 53)   */ j1939_default_route,
    /* PF = 0x0x36( 54)   */ j1939_default_route,
    /* PF = 0x0x37( 55)   */ j1939_default_route,
    /* PF = 0x0x38( 56)   */ j1939_default_route,
    /* PF = 0x0x39( 57)   */ j1939_default_route,
    /* PF = 0x0x3A( 58)   */ j1939_default_route,
    /* PF = 0x0x3B( 59)   */ j1939_default_route,
    /* PF = 0x0x3C( 60)   */ j1939_default_route,
    /* PF = 0x0x3D( 61)   */ j1939_default_route,
    /* PF = 0x0x3E( 62)   */ j1939_default_route,
    /* PF = 0x0x3F( 63)   */ j1939_default_route,
    /* PF = 0x0x40( 64)   */ j1939_default_route,
    /* PF = 0x0x41( 65)   */ j1939_default_route,
    /* PF = 0x0x42( 66)   */ j1939_default_route,
    /* PF = 0x0x43( 67)   */ j1939_default_route,
    /* PF = 0x0x44( 68)   */ j1939_default_route,
    /* PF = 0x0x45( 69)   */ j1939_default_route,
    /* PF = 0x0x46( 70)   */ j1939_default_route,
    /* PF = 0x0x47( 71)   */ j1939_default_route,
    /* PF = 0x0x48( 72)   */ j1939_default_route,
    /* PF = 0x0x49( 73)   */ j1939_default_route,
    /* PF = 0x0x4A( 74)   */ j1939_default_route,
    /* PF = 0x0x4B( 75)   */ j1939_default_route,
    /* PF = 0x0x4C( 76)   */ j1939_default_route,
    /* PF = 0x0x4D( 77)   */ j1939_default_route,
    /* PF = 0x0x4E( 78)   */ j1939_default_route,
    /* PF = 0x0x4F( 79)   */ j1939_default_route,
    /* PF = 0x0x50( 80)   */ j1939_default_route,
    /* PF = 0x0x51( 81)   */ j1939_default_route,
    /* PF = 0x0x52( 82)   */ j1939_default_route,
    /* PF = 0x0x53( 83)   */ j1939_default_route,
    /* PF = 0x0x54( 84)   */ j1939_default_route,
    /* PF = 0x0x55( 85)   */ j1939_default_route,
    /* PF = 0x0x56( 86)   */ j1939_default_route,
    /* PF = 0x0x57( 87)   */ j1939_default_route,
    /* PF = 0x0x58( 88)   */ j1939_default_route,
    /* PF = 0x0x59( 89)   */ j1939_default_route,
    /* PF = 0x0x5A( 90)   */ j1939_default_route,
    /* PF = 0x0x5B( 91)   */ j1939_default_route,
    /* PF = 0x0x5C( 92)   */ j1939_default_route,
    /* PF = 0x0x5D( 93)   */ j1939_default_route,
    /* PF = 0x0x5E( 94)   */ j1939_default_route,
    /* PF = 0x0x5F( 95)   */ j1939_default_route,
    /* PF = 0x0x60( 96)   */ j1939_default_route,
    /* PF = 0x0x61( 97)   */ j1939_default_route,
    /* PF = 0x0x62( 98)   */ j1939_default_route,
    /* PF = 0x0x63( 99)   */ j1939_default_route,
    /* PF = 0x0x64(100)   */ j1939_default_route,
    /* PF = 0x0x65(101)   */ j1939_default_route,
    /* PF = 0x0x66(102)   */ j1939_default_route,
    /* PF = 0x0x67(103)   */ j1939_default_route,
    /* PF = 0x0x68(104)   */ j1939_default_route,
    /* PF = 0x0x69(105)   */ j1939_default_route,
    /* PF = 0x0x6A(106)   */ j1939_default_route,
    /* PF = 0x0x6B(107)   */ j1939_default_route,
    /* PF = 0x0x6C(108)   */ j1939_default_route,
    /* PF = 0x0x6D(109)   */ j1939_default_route,
    /* PF = 0x0x6E(110)   */ j1939_default_route,
    /* PF = 0x0x6F(111)   */ j1939_default_route,
    /* PF = 0x0x70(112)   */ j1939_default_route,
    /* PF = 0x0x71(113)   */ j1939_default_route,
    /* PF = 0x0x72(114)   */ j1939_default_route,
    /* PF = 0x0x73(115)   */ j1939_default_route,
    /* PF = 0x0x74(116)   */ j1939_default_route,
    /* PF = 0x0x75(117)   */ j1939_default_route,
    /* PF = 0x0x76(118)   */ j1939_default_route,
    /* PF = 0x0x77(119)   */ j1939_default_route,
    /* PF = 0x0x78(120)   */ j1939_default_route,
    /* PF = 0x0x79(121)   */ j1939_default_route,
    /* PF = 0x0x7A(122)   */ j1939_default_route,
    /* PF = 0x0x7B(123)   */ j1939_default_route,
    /* PF = 0x0x7C(124)   */ j1939_default_route,
    /* PF = 0x0x7D(125)   */ j1939_default_route,
    /* PF = 0x0x7E(126)   */ j1939_default_route,
    /* PF = 0x0x7F(127)   */ j1939_default_route,
    /* PF = 0x0x80(128)   */ j1939_default_route,
    /* PF = 0x0x81(129)   */ j1939_default_route,
    /* PF = 0x0x82(130)   */ j1939_default_route,
    /* PF = 0x0x83(131)   */ j1939_default_route,
    /* PF = 0x0x84(132)   */ j1939_default_route,
    /* PF = 0x0x85(133)   */ j1939_default_route,
    /* PF = 0x0x86(134)   */ j1939_default_route,
    /* PF = 0x0x87(135)   */ j1939_default_route,
    /* PF = 0x0x88(136)   */ j1939_default_route,
    /* PF = 0x0x89(137)   */ j1939_default_route,
    /* PF = 0x0x8A(138)   */ j1939_default_route,
    /* PF = 0x0x8B(139)   */ j1939_default_route,
    /* PF = 0x0x8C(140)   */ j1939_default_route,
    /* PF = 0x0x8D(141)   */ j1939_default_route,
    /* PF = 0x0x8E(142)   */ j1939_default_route,
    /* PF = 0x0x8F(143)   */ j1939_default_route,
    /* PF = 0x0x90(144)   */ j1939_default_route,
    /* PF = 0x0x91(145)   */ j1939_default_route,
    /* PF = 0x0x92(146)   */ j1939_default_route,
    /* PF = 0x0x93(147)   */ j1939_default_route,
    /* PF = 0x0x94(148)   */ j1939_default_route,
    /* PF = 0x0x95(149)   */ j1939_default_route,
    /* PF = 0x0x96(150)   */ j1939_default_route,
    /* PF = 0x0x97(151)   */ j1939_default_route,
    /* PF = 0x0x98(152)   */ j1939_default_route,
    /* PF = 0x0x99(153)   */ j1939_default_route,
    /* PF = 0x0x9A(154)   */ j1939_default_route,
    /* PF = 0x0x9B(155)   */ j1939_default_route,
    /* PF = 0x0x9C(156)   */ j1939_default_route,
    /* PF = 0x0x9D(157)   */ j1939_default_route,
    /* PF = 0x0x9E(158)   */ j1939_default_route,
    /* PF = 0x0x9F(159)   */ j1939_default_route,
    /* PF = 0x0xA0(160)   */ j1939_default_route,
    /* PF = 0x0xA1(161)   */ j1939_default_route,
    /* PF = 0x0xA2(162)   */ j1939_default_route,
    /* PF = 0x0xA3(163)   */ j1939_default_route,
    /* PF = 0x0xA4(164)   */ j1939_default_route,
    /* PF = 0x0xA5(165)   */ j1939_default_route,
    /* PF = 0x0xA6(166)   */ j1939_default_route,
    /* PF = 0x0xA7(167)   */ j1939_default_route,
    /* PF = 0x0xA8(168)   */ j1939_default_route,
    /* PF = 0x0xA9(169)   */ j1939_default_route,
    /* PF = 0x0xAA(170)   */ j1939_default_route,
    /* PF = 0x0xAB(171)   */ j1939_default_route,
    /* PF = 0x0xAC(172)   */ j1939_default_route,
    /* PF = 0x0xAD(173)   */ j1939_default_route,
    /* PF = 0x0xAE(174)   */ j1939_default_route,
    /* PF = 0x0xAF(175)   */ j1939_default_route,
    /* PF = 0x0xB0(176)   */ j1939_default_route,
    /* PF = 0x0xB1(177)   */ j1939_default_route,
    /* PF = 0x0xB2(178)   */ j1939_default_route,
    /* PF = 0x0xB3(179)   */ j1939_default_route,
    /* PF = 0x0xB4(180)   */ j1939_default_route,
    /* PF = 0x0xB5(181)   */ j1939_default_route,
    /* PF = 0x0xB6(182)   */ j1939_default_route,
    /* PF = 0x0xB7(183)   */ j1939_default_route,
    /* PF = 0x0xB8(184)   */ j1939_default_route,
    /* PF = 0x0xB9(185)   */ j1939_default_route,
    /* PF = 0x0xBA(186)   */ j1939_default_route,
    /* PF = 0x0xBB(187)   */ j1939_default_route,
    /* PF = 0x0xBC(188)   */ j1939_default_route,
    /* PF = 0x0xBD(189)   */ j1939_default_route,
    /* PF = 0x0xBE(190)   */ j1939_default_route,
    /* PF = 0x0xBF(191)   */ j1939_default_route,
    /* PF = 0x0xC0(192)   */ j1939_default_route,
    /* PF = 0x0xC1(193)   */ j1939_default_route,
    /* PF = 0x0xC2(194)   */ j1939_default_route,
    /* PF = 0x0xC3(195)   */ j1939_default_route,
    /* PF = 0x0xC4(196)   */ j1939_default_route,
    /* PF = 0x0xC5(197)   */ j1939_default_route,
    /* PF = 0x0xC6(198)   */ j1939_default_route,
    /* PF = 0x0xC7(199)   */ j1939_default_route,
    /* PF = 0x0xC8(200)   */ j1939_default_route,
    /* PF = 0x0xC9(201)   */ j1939_default_route,
    /* PF = 0x0xCA(202)   */ j1939_default_route,
    /* PF = 0x0xCB(203)   */ j1939_default_route,
    /* PF = 0x0xCC(204)   */ j1939_default_route,
    /* PF = 0x0xCD(205)   */ j1939_default_route,
    /* PF = 0x0xCE(206)   */ j1939_default_route,
    /* PF = 0x0xCF(207)   */ j1939_default_route,
    /* PF = 0x0xD0(208)   */ j1939_default_route,
    /* PF = 0x0xD1(209)   */ j1939_default_route,
    /* PF = 0x0xD2(210)   */ j1939_default_route,
    /* PF = 0x0xD3(211)   */ j1939_default_route,
    /* PF = 0x0xD4(212)   */ j1939_default_route,
    /* PF = 0x0xD5(213)   */ j1939_default_route,
    /* PF = 0x0xD6(214)   */ j1939_default_route,

#ifdef J1939_MEMORY_ACCESS
    /* PF = 0x0xD7(215)   */ j1939_ma_dm16_process,
#else
    /* PF = 0x0xD7(215)   */ j1939_default_route,
#endif

#if (defined(J1939_MEMORY_ACCESS) && defined(J1939MA_TRANSMIT_READ_WRITE_REQUEST))
    /* PF = 0x0xD8(216)   */ j1939_ma_dm15_response,
#else
    /* PF = 0x0xD8(216)   */ j1939_default_route,
#endif

#ifdef J1939_MEMORY_ACCESS
    /* PF = 0x0xD9(217)   */ j1939_ma_dm14_request,
#else
    /* PF = 0x0xD9(217)   */ j1939_default_route,
#endif

    /* PF = 0x0xDA(218)   */ j1939_default_route,
    /* PF = 0x0xDB(219)   */ j1939_default_route,
    /* PF = 0x0xDC(220)   */ j1939_default_route,
    /* PF = 0x0xDD(221)   */ j1939_default_route,
    /* PF = 0x0xDE(222)   */ j1939_default_route,

#ifdef J1939DM13_ENABLE
    /* PF = 0x0xDF(223)   */ j1939_dm13_receive,
#else
    /* PF = 0x0xDF(223)   */ j1939_default_route,
#endif

    /* PF = 0x0xE0(224)   */ j1939_default_route,
    /* PF = 0x0xE1(225)   */ j1939_default_route,
    /* PF = 0x0xE2(226)   */ j1939_default_route,
    /* PF = 0x0xE3(227)   */ j1939_default_route,
    /* PF = 0x0xE4(228)   */ j1939_default_route,
    /* PF = 0x0xE5(229)   */ j1939_default_route,
    /* PF = 0x0xE6(230)   */ j1939_default_route,
    /* PF = 0x0xE7(231)   */ j1939_default_route,
    /* PF = 0x0xE8(232)   */ j1939_default_route,
    /* PF = 0x0xE9(233)   */ j1939_default_route,
    /* PF = 0x0xEA(234)   */ j1939_request_pgn,
#ifdef CONFIG_J1939_TRANSPORT_PROTOCOL
    /* PF = 0x0xEB(235)   */ j1939_tp_data_transfer,
    /* PF = 0x0xEC(236)   */ j1939_tp_connection_management,
#else
    /* PF = 0x0xEB(235)   */ j1939_default_route,
    /* PF = 0x0xEC(236)   */ j1939_default_route,
#endif

    /* PF = 0x0xED(237)   */ j1939_default_route,
    /* PF = 0x0xEE(238)   */ j1939_ac_is_received,
    /* PF = 0x0xEF(239)   */ j1939_default_route,
    /* PF = 0x0xF0(240)   */ j1939_default_route,
    /* PF = 0x0xF1(241)   */ j1939_default_route,
    /* PF = 0x0xF2(242)   */ j1939_default_route,
    /* PF = 0x0xF3(243)   */ j1939_default_route,
    /* PF = 0x0xF4(244)   */ j1939_default_route,
    /* PF = 0x0xF5(245)   */ j1939_default_route,
    /* PF = 0x0xF6(246)   */ j1939_default_route,
    /* PF = 0x0xF7(247)   */ j1939_default_route,
    /* PF = 0x0xF8(248)   */ j1939_default_route,
    /* PF = 0x0xF9(249)   */ j1939_default_route,
    /* PF = 0x0xFA(250)   */ j1939_default_route,
    /* PF = 0x0xFB(251)   */ j1939_default_route,
    /* PF = 0x0xFC(252)   */ j1939_default_route,
    /* PF = 0x0xFD(253)   */ j1939_default_route,
    /* PF = 0x0xFE(254)   */ j1939_pdu_f254_process,
    /* PF = 0x0xFF(255)   */ j1939_default_route};

/**************************************************************************************************/
void j1939_app_init(void)
{
   for (uint16_t index = 0; index < CONFIG_J1939_NODES_COUNT; index++)
   {
      j1939_node_t node = j1939_nodes[index];
      // Register the PGN requests we need to handle
      (void)j1939_register_request_pgn(J1939_SOFTWARE_ID_PGN, node);
      (void)j1939_register_request_pgn(J1939_ECU_ID_INFO_PGN, node);
   }
}

/**************************************************************************************************/
void j1939_app_task(void)
{
   j1939_source_address_t requestedSource;

   for (uint16_t index = 0; index < CONFIG_J1939_NODES_COUNT; index++)
   {
      j1939_node_t node = j1939_nodes[index];

      if (j1939_is_pgn_requested(J1939_ECU_ID_INFO_PGN, &requestedSource, node))
      {
         j1939_app_transmit_ecu_id((j1939_destination_address_t)requestedSource, node);
      }
      if (j1939_is_pgn_requested(J1939_SOFTWARE_ID_PGN, &requestedSource, node))
      {
         j1939_app_transmit_software_id((j1939_destination_address_t)requestedSource, node);
      }
#ifdef CONFIG_J1939_NAME_MANAGEMENT
      if (j1939_is_pgn_requested(J1939NM_PGN, &requestedSource, node))
      {
         j1939_nm_pgn_request_handler((j1939_destination_address_t)requestedSource, node);
      }
#endif
   }
}

/**************************************************************************************************/
bool j1939_app_pf254_process(const struct can_frame *message, j1939_node_t node)
{
   // If we don't handle the message, we should let the default handler handle it.
   return j1939_default_route(message, node);
}

/**************************************************************************************************/
void j1939_app_transmit_ecu_id(j1939_destination_address_t destination, j1939_node_t node)
{
   // To minimize stack usage we allocate a buffer from the Transport Layer
   // The code below does not perform any range checking.  The application developer is responsible
   // for allocating a buffer large enough to hold the largest possible string.
   uint8_t *data =
       j1939_tp_get_buffer(150);      // Pointer to the beginning of the transport buffer
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

      // Note that if j1939_transmit_pgn() succeeds, the transport layer will take care of freeing
      // the buffer we allocated when the transport session is completed.  If it fails, we need
      // to take care of it ourselves
      if (!(j1939_transmit_pgn(
              J1939_Priority_6, J1939_ECU_ID_INFO_PGN, destination, data,
              //lint --e(613) if tempData or data are null, this function will not be called
              (j1939_counter_t)(tempData - data), node)))
      {
         j1939_tp_free_buffer(data);
      }
   }
} //lint !e438

/**************************************************************************************************/
void j1939_app_transmit_software_id(j1939_destination_address_t destination, j1939_node_t node)
{
   // To minimize stack usage we allocate a buffer from the Transport Layer
   // The code below does not perform any range checking.  The application developer is responsible
   // for allocating a buffer large enough to hold the largest possible string.
   uint8_t *data =
       j1939_tp_get_buffer(150);      // Pointer to the beginning of the transport buffer
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

      // Note that if j1939_transmit_pgn() succeeds, the transport layer will take care of freeing
      // the buffer we allocated when the transport session is completed.  If it fails, we need
      // to take care of it ourselves
      if (!(j1939_transmit_pgn(
              J1939_Priority_6,      // Priority.
              J1939_SOFTWARE_ID_PGN, // PGN.
              destination,           // Source Address to reply to
              data,                  // Data pointer.
              //lint --e(613) if tempData or data are null, this function will not be called
              (j1939_counter_t)(tempData - data), // Byte counter for pointer.
              node)))                             // CAN Node.
      {
         j1939_tp_free_buffer(data);
      }
   }
} //lint !e438

/**************************************************************************************************/
j1939_node_t j1939_app_get_j1939_node_from_source_address(j1939_source_address_t address)
{
   j1939_node_t node;

//    TODO: probably could return a pointer to the can dev?
//    for (j1939_node_t index = 0; index < CONFIG_J1939_NODES_COUNT; index++)
//    {
//       if (address == j1939_ac_get_source_address(index))
//       {
//          node = index;
//          break;
//       }
//       else if (index == CONFIG_J1939_NODES_COUNT - 1)
//       {
//          node = CONFIG_J1939_NODES_COUNT + 1;
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
