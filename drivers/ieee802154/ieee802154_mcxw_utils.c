/* ieee802154_mcxw_utils.h - NXP MCXW7X 802.15.4 driver utils*/

/*
 * Copyright (c) NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <errno.h>

#include "ieee802154_mcxw_utils.h"

// TODO IEEE 802.15.4 MAC Multipurpose frame format
// TODO add function checks

enum offset_fcf_fields
{
    OffsetFrameType                 = 0x00,
    OffsetSecurityEnabled           = 0x03,
    OffsetFramePending              = 0x04,
    OffsetAR                        = 0x05,
    OffsetPanIdCompression          = 0x06,
    OffsetSeqNumberSuppression      = 0x08,
    OffsetIEPresent                 = 0x09,
    OffsetDstAddrMode               = 0x0A,
    OffsetFrameVersion              = 0x0C,
    OffsetSrcAddrMode               = 0x0E,
};

enum mask_fcf_fields
{
    MaskFrameType               = (0x7 << OffsetFrameType),
    MaskSecurityEnabled         = (0x01 << OffsetSecurityEnabled),
    MaskFramePending            = (0x01 << OffsetFramePending),
    MaskAR                      = (0x01 << OffsetAR),
    MaskPanIdCompression        = (0x01 << OffsetPanIdCompression),
    MaskSeqNumberSuppression    = (0x01 << OffsetSeqNumberSuppression),
    MaskIEPresent               = (0x01 << OffsetIEPresent),
    MaskDstAddrMode             = (0x03 << OffsetDstAddrMode),
    MaskFrameVersion            = (0x03 << OffsetFrameVersion),
    MaskSrcAddrMode             = (0x03 << OffsetSrcAddrMode),
};

enum modes_dst_addr
{
    ModeDstAddrNone        = 0x00,
    ModeDstAddrShort       = (0x02 << OffsetDstAddrMode),
    ModeDstAddrExt         = (0x03 << OffsetDstAddrMode),
};

enum version_frame
{
    VersionIeee2003         = 0x00,
    VersionIeee2006         = 0x01,
    VersionIeee2015         = 0x02,
};

enum modes_src_addr
{
    ModeSrcAddrNone        = 0x00,
    ModeSrcAddrShort       = (0x02 << OffsetSrcAddrMode),
    ModeSrcAddrExt         = (0x03 << OffsetSrcAddrMode),
};

enum offset_scf_fields
{
    OffsetSecurityLevel         = 0x00,
    OffsetKeyIdMode             = 0x03,
    OffsetFrameCntSuppression   = 0x05,
    OffsetASNinNonce            = 0x06,
};

enum mask_scf_fields
{
    MaskSecurityLevel       = (0x07 << OffsetSecurityLevel),
    MaskKeyIdMode           = (0x03 << OffsetKeyIdMode),
    MaskFrameCntSuppression = (0x1 << OffsetFrameCntSuppression),
    MaskASNinNonce          = (0x01 << OffsetASNinNonce),
};

static uint16_t GetFrameControlField(uint8_t *pdu, uint16_t lenght)
{
    if ((pdu == NULL) || (lenght < 3))
        return 0x00;

    return (uint16_t)(pdu[0] | (pdu[1] << 8));
}

static bool IsSecurityEnabled(uint16_t fcf)
{
    if (fcf)
        return (bool)(fcf & MaskSecurityEnabled);

    return false;
}

static bool IsIEPresent(uint16_t fcf)
{
    if (fcf)
        return (bool)(fcf & MaskIEPresent);

    return false;
}

static bool IsMultipurpose(uint16_t fcf)
{
    if(fcf)
        return (fcf & 0x5) == 0x5;

    return false;
}

static uint8_t GetFrameVersion(uint16_t fcf)
{
    if (fcf)
        return (uint8_t)((fcf & MaskFrameVersion) >> OffsetFrameVersion);

    return 0xFF;
}

static bool IsVersion2015FCF(uint16_t fcf)
{
    if (fcf)
        return GetFrameVersion(fcf) == VersionIeee2015;

    return false;
}

bool IsVersion2015(uint8_t *pdu, uint16_t lenght)
{
    uint16_t fcf = GetFrameControlField(pdu, lenght);

    if (fcf)
        return GetFrameVersion(fcf) == VersionIeee2015;

    return false;
}

static bool IsSequenceNumberSuppression(uint16_t fcf)
{
    if (fcf)
        return (bool)(fcf & MaskSeqNumberSuppression);

    return false;
}

// TODO multipurpose
static bool IsDstPanIdPresent(uint16_t fcf)
{
    if (!fcf)
        return false;

    bool present;

    if (IsVersion2015FCF(fcf))
    {
        switch(fcf & (MaskDstAddrMode | MaskSrcAddrMode | MaskPanIdCompression))
        {
        case (ModeDstAddrNone  | ModeSrcAddrNone):
        case (ModeDstAddrShort | ModeSrcAddrNone  | MaskPanIdCompression):
        case (ModeDstAddrExt   | ModeSrcAddrNone  | MaskPanIdCompression):
        case (ModeDstAddrNone  | ModeSrcAddrShort):
        case (ModeDstAddrNone  | ModeSrcAddrExt):
        case (ModeDstAddrNone  | ModeSrcAddrShort | MaskPanIdCompression):
        case (ModeDstAddrNone  | ModeSrcAddrExt   | MaskPanIdCompression):
        case (ModeDstAddrExt   | ModeSrcAddrExt   | MaskPanIdCompression):
            present = false;
            break;
        default:
            present = true;
        }
    }
    else
    {
        present  = (bool)(fcf & MaskDstAddrMode);
    }

    return present;

}

// TODO multipurpose
static bool IsSrcPanIdPresent(uint16_t fcf)
{
    if (!fcf)
        return false;

    bool present;

    if (IsVersion2015FCF(fcf))
    {
        switch(fcf & (MaskDstAddrMode | MaskSrcAddrMode | MaskPanIdCompression))
        {
        case (ModeDstAddrNone  | ModeSrcAddrShort):
        case (ModeDstAddrNone  | ModeSrcAddrExt):
        case (ModeDstAddrShort | ModeSrcAddrShort):
        case (ModeDstAddrShort | ModeSrcAddrExt):
        case (ModeDstAddrExt   | ModeSrcAddrShort):
            present = true;
            break;
        default:
            present = false;
        }

    }
    else
    {
        present  = ((fcf & MaskSrcAddrMode) != 0) && ((fcf & MaskPanIdCompression) == 0);
    }

    return present;
}

// TODO multipurpose
static uint8_t CalculateAddrFieldSize(uint16_t fcf)
{
    if(!fcf)
        return 0;

    uint8_t size = 2;

    if (!IsSequenceNumberSuppression(fcf))
        size += 1;
    
    if (IsDstPanIdPresent(fcf))
        size += 2;

    // destination addressing mode
    switch(fcf & MaskDstAddrMode)
    {
        case ModeDstAddrShort:
            size += 2;
            break;
        case ModeDstAddrExt:
            size += 8;
            break;
        default:
            break;
    }

    if(IsSrcPanIdPresent(fcf))
        size += 2;

    // source addressing mode
    switch(fcf & MaskSrcAddrMode)
    {
        case ModeSrcAddrShort:
            size += 2;
            break;
        case ModeSrcAddrExt:
            size += 8;
            break;
        default:
            break;
    }

    return size;
}

static uint8_t GetKeyIdentifierMode(uint8_t *pdu, uint16_t lenght)
{
    uint16_t fcf = GetFrameControlField(pdu, lenght);
    uint8_t ash_start;
    if (IsSecurityEnabled(fcf))
    {
        ash_start = CalculateAddrFieldSize(fcf);
        return (uint8_t)((pdu[ash_start] & MaskKeyIdMode) >> OffsetKeyIdMode);
    }

    return 0xFF;
}

bool IsKeyIdMode1(uint8_t *pdu, uint16_t lenght)
{
    uint8_t key_mode = GetKeyIdentifierMode(pdu, lenght);
    if (key_mode == 0x01)
        return true;

    return false;
}

void SetFrameCounter(uint8_t *pdu, uint16_t lenght, uint32_t fc)
{
    uint16_t fcf = GetFrameControlField(pdu, lenght);
    if (IsSecurityEnabled(fcf))
    {
        uint8_t ash_start = CalculateAddrFieldSize(fcf);
        uint8_t scf = pdu[ash_start];

        // check that Frame Counter Suppression is not set
        if(!(scf & MaskFrameCntSuppression))
        {
            sys_put_le32(fc, &pdu[ash_start+1]);
        }
    }
}

static uint8_t GetASNSize(uint8_t *pdu, uint16_t lenght)
{
    uint16_t fcf = GetFrameControlField(pdu, lenght);
    if(IsSecurityEnabled(fcf))
    {
        uint8_t ash_start = CalculateAddrFieldSize(fcf);
        uint8_t scf = pdu[ash_start];
        // scf size
        uint8_t size = 1;

        // Frame Counter Suppression is not set
        if(!(scf & MaskFrameCntSuppression))
        {
            size += 4;
        }

        uint8_t key_mode = GetKeyIdentifierMode(pdu, lenght);
        switch (key_mode)
        {
            case 0x01:
                size += 1;
                break;
            case 0x02:
                size += 5;
            case 0x03:
                size += 9;
            default:
                break;
        }

        return size;
    }
    return 0;
}

static uint8_t* GetCslIeContentStart(uint8_t *pdu, uint16_t lenght)
{
    uint16_t fcf = GetFrameControlField(pdu, lenght);
    if(IsIEPresent(fcf))
    {
        uint8_t ie_start_idx = CalculateAddrFieldSize(fcf) + GetASNSize(pdu, lenght);
        uint8_t *cur_ie = &pdu[ie_start_idx];

        uint8_t ie_header = (uint16_t)(cur_ie[0] | (cur_ie[1] << 8));
        uint8_t ie_lenght = ie_header & 0x7F;
        uint8_t ie_el_id = ie_header & 0x7F80;

        while((ie_el_id != 0x7e) && (ie_el_id != 0x7f))
        {
            if(ie_el_id == 0x1a)
                return (cur_ie + 2);
            cur_ie += (2 + ie_lenght);
            ie_header = (uint16_t)(cur_ie[0] | (cur_ie[1] << 8));
            ie_lenght = ie_header & 0x7F;
            ie_el_id = ie_header & 0x7F80;
        }
    }

    return NULL;
}

void SetCslIe(uint8_t *pdu, uint16_t lenght, uint16_t period, uint16_t phase)
{
    uint8_t *CslIeContent = GetCslIeContentStart(pdu, lenght);
    if (CslIeContent)
    {
        sys_put_le16(phase, CslIeContent);
        sys_put_le16(period, CslIeContent + 2);
    }
}