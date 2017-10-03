/*
 * fs.c - CC31xx/CC32xx Host Driver Implementation
 *
 * Copyright (C) 2017 Texas Instruments Incorporated - http://www.ti.com/ 
 * 
 * 
 *  Redistribution and use in source and binary forms, with or without 
 *  modification, are permitted provided that the following conditions 
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the   
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
*/



/*****************************************************************************/
/* Include files                                                             */
/*****************************************************************************/
#include <ti/drivers/net/wifi/simplelink.h>
#include <ti/drivers/net/wifi/source/protocol.h>
#include <ti/drivers/net/wifi/source/driver.h>

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/
#define sl_min(a,b) (((a) < (b)) ? (a) : (b))
#define MAX_NVMEM_CHUNK_SIZE  1456 /*should be 16 bytes align, because of encryption data*/

/*****************************************************************************/
/* Internal functions                                                        */
/*****************************************************************************/

#ifndef SL_TINY
static _u16 _SlFsStrlen(const _u8 *buffer);

static _u32 FsGetCreateFsMode(_u8 Mode, _u32 MaxSizeInBytes,_u32 AccessFlags);

/*****************************************************************************/
/* _SlFsStrlen                                                                */
/*****************************************************************************/
static _u16 _SlFsStrlen(const _u8 *buffer)
{
    _u16 len = 0;
    if( buffer != NULL )
    {
      while(*buffer++) len++;
    }
    return len;
}
#endif
/*****************************************************************************/
/*  _SlFsGetCreateFsMode                                                       */
/*****************************************************************************/



/* Convert the user flag to the file System flag */
#define FS_CONVERT_FLAGS( ModeAndMaxSize )  (((_u32)ModeAndMaxSize & SL_FS_OPEN_FLAGS_BIT_MASK)>>SL_NUM_OF_MAXSIZE_BIT)



typedef enum
{
    FS_MODE_OPEN_READ            = 0,
    FS_MODE_OPEN_WRITE,
    FS_MODE_OPEN_CREATE,
    FS_MODE_OPEN_WRITE_CREATE_IF_NOT_EXIST
}FsFileOpenAccessType_e;

#define FS_MODE_ACCESS_RESERVED_OFFSET                       (27)
#define FS_MODE_ACCESS_RESERVED_MASK                         (0x1F)
#define FS_MODE_ACCESS_FLAGS_OFFSET                          (16)
#define FS_MODE_ACCESS_FLAGS_MASK                            (0x7FF)
#define FS_MODE_ACCESS_OFFSET                                (12)
#define FS_MODE_ACCESS_MASK                                  (0xF)
#define FS_MODE_OPEN_SIZE_GRAN_OFFSET                        (8)
#define FS_MODE_OPEN_SIZE_GRAN_MASK                          (0xF)
#define FS_MODE_OPEN_SIZE_OFFSET                             (0)
#define FS_MODE_OPEN_SIZE_MASK                               (0xFF)
#define FS_MAX_MODE_SIZE                                     (0xFF)

/* SizeGran is up to 4 bit , Size can be up to 8 bit */
#define FS_MODE(Access, SizeGran, Size,Flags)        (_u32)(((_u32)((Access) &FS_MODE_ACCESS_MASK)<<FS_MODE_ACCESS_OFFSET) |  \
                                                            ((_u32)((SizeGran) &FS_MODE_OPEN_SIZE_GRAN_MASK)<<FS_MODE_OPEN_SIZE_GRAN_OFFSET) | \
                                                            ((_u32)((Size) &FS_MODE_OPEN_SIZE_MASK)<<FS_MODE_OPEN_SIZE_OFFSET) | \
                                                            ((_u32)((Flags) &FS_MODE_ACCESS_FLAGS_MASK)<<FS_MODE_ACCESS_FLAGS_OFFSET))


typedef enum
{
    FS_MODE_SIZE_GRAN_256B    = 0,   /*  MAX_SIZE = 64K  */
    FS_MODE_SIZE_GRAN_1KB,           /*  MAX_SIZE = 256K */
    FS_MODE_SIZE_GRAN_4KB,           /*  MAX_SZIE = 1M   */
    FS_MODE_SIZE_GRAN_16KB,          /*  MAX_SIZE = 4M   */
    FS_MODE_SIZE_GRAN_64KB,          /*  MAX_SIZE = 16M  */
    FS_MAX_MODE_SIZE_GRAN
}FsFileOpenMaxSizeGran_e;

#ifndef SL_TINY


static _u32 FsGetCreateFsMode(_u8 Mode, _u32 MaxSizeInBytes,_u32 AccessFlags)
{
   _u32 granIdx = 0;
   _u32 granNum = 0;
   _u32 granTable[FS_MAX_MODE_SIZE_GRAN] = {256,1024,4096,16384,65536};

   for(granIdx= FS_MODE_SIZE_GRAN_256B ;granIdx< FS_MAX_MODE_SIZE_GRAN;granIdx++)
   {
       if( granTable[granIdx]*255 >= MaxSizeInBytes )
            break;
   }
   granNum = MaxSizeInBytes/granTable[granIdx];
   if( MaxSizeInBytes % granTable[granIdx] != 0 )
         granNum++;

   return (_u32)FS_MODE( Mode, granIdx, granNum, AccessFlags );

}



#endif

/*****************************************************************************/
/* API functions                                                        */
/*****************************************************************************/

/*****************************************************************************/
/*  sl_FsOpen */
/*****************************************************************************/
typedef union
{
  SlFsOpenCommand_t	    Cmd;
  SlFsOpenResponse_t    Rsp;
}_SlFsOpenMsg_u;


#if _SL_INCLUDE_FUNC(sl_FsOpen)

static const _SlCmdCtrl_t _SlFsOpenCmdCtrl =
{
    SL_OPCODE_NVMEM_FILEOPEN,
    (_SlArgSize_t)sizeof(SlFsOpenCommand_t),
    (_SlArgSize_t)sizeof(SlFsOpenResponse_t)
};

_i32 sl_FsOpen(const _u8 *pFileName,const _u32 ModeAndMaxSize, _u32 *pToken)
{

    _SlFsOpenMsg_u        Msg;
    _SlCmdExt_t           CmdExt;
    _i32		          FileHandle;
    _u32                  MaxSizeInBytes;
    _u32                  OpenMode;
    _u8	                  CreateMode;


    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

	_SlDrvMemZero(&CmdExt, (_u16)sizeof(_SlCmdExt_t));

    if ( _SlFsStrlen(pFileName) >= SL_FS_MAX_FILE_NAME_LENGTH )
    {
        return SL_ERROR_FS_WRONG_FILE_NAME;
    }

    CmdExt.TxPayload1Len = (_u16)((_SlFsStrlen(pFileName)+4) & (~3)); /* add 4: 1 for NULL and the 3 for align */
    CmdExt.pTxPayload1 = (_u8*)pFileName;

	OpenMode = ModeAndMaxSize & SL_FS_OPEN_MODE_BIT_MASK;

    /*convert from the interface flags to the device flags*/
    if( OpenMode == SL_FS_READ )
    {
        Msg.Cmd.Mode = FS_MODE(FS_MODE_OPEN_READ, 0, 0, 0);
    }
    else if (( OpenMode == SL_FS_WRITE ) ||( OpenMode == SL_FS_OVERWRITE))
    {
        Msg.Cmd.Mode = FS_MODE(FS_MODE_OPEN_WRITE, 0, 0, FS_CONVERT_FLAGS ( ModeAndMaxSize));
    }
	/* one of the creation mode */
    else if ( ( OpenMode == (SL_FS_CREATE | SL_FS_OVERWRITE )) || ( OpenMode == SL_FS_CREATE) ||(OpenMode == (SL_FS_CREATE | SL_FS_WRITE )))
    {
       /* test that the size is correct */
       MaxSizeInBytes = (ModeAndMaxSize & SL_FS_OPEN_MAXSIZE_BIT_MASK) * 256;
	   if (MaxSizeInBytes > 0xFF0000 )
	   {
		   return SL_ERROR_FS_FILE_MAX_SIZE_EXCEEDED;
	   }

	   CreateMode = ((OpenMode == (SL_FS_CREATE | SL_FS_OVERWRITE )) ? FS_MODE_OPEN_WRITE_CREATE_IF_NOT_EXIST : FS_MODE_OPEN_CREATE  );

        Msg.Cmd.Mode = FsGetCreateFsMode( CreateMode ,MaxSizeInBytes, FS_CONVERT_FLAGS ( ModeAndMaxSize)  );
    }
    else
    {
        return SL_ERROR_FS_UNVALID_FILE_MODE;
    }

    if(pToken != NULL)
    {
        Msg.Cmd.Token         = *pToken;
    }
    else
    {
        Msg.Cmd.Token         = 0;
    }

    _SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsOpenCmdCtrl, &Msg, &CmdExt);
    FileHandle = (_i32)Msg.Rsp.FileHandle;
    if (pToken != NULL)
    {
        *pToken =      Msg.Rsp.Token;
    }

    /* in case of an error, return the erros file handler as an error code */
    return FileHandle;
}
#endif

/*****************************************************************************/
/* sl_FsClose */
/*****************************************************************************/
typedef union
{
  SlFsCloseCommand_t    Cmd;
  _BasicResponse_t	    Rsp;
}_SlFsCloseMsg_u;


#if _SL_INCLUDE_FUNC(sl_FsClose)

static const _SlCmdCtrl_t _SlFsCloseCmdCtrl =
{
    SL_OPCODE_NVMEM_FILECLOSE,
    (_SlArgSize_t)sizeof(SlFsCloseCommand_t),
    (_SlArgSize_t)sizeof(SlFsCloseResponse_t)
};

_i16 sl_FsClose(const _i32 FileHdl, const _u8*  pCeritificateFileName,const _u8*  pSignature ,const _u32 SignatureLen)
{
    _SlFsCloseMsg_u    Msg;
    _SlCmdExt_t        ExtCtrl;

    _SlDrvMemZero(&Msg, (_u16)sizeof(SlFsCloseCommand_t));

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);


    Msg.Cmd.FileHandle             = (_u32)FileHdl;
    if( pCeritificateFileName != NULL )
    {
        Msg.Cmd.CertificFileNameLength = (_u32)((_SlFsStrlen(pCeritificateFileName)+4) & (~3)); /* add 4: 1 for NULL and the 3 for align */
    }
    Msg.Cmd.SignatureLen           = SignatureLen;

	_SlDrvMemZero(&ExtCtrl, (_u16)sizeof(_SlCmdExt_t));

    ExtCtrl.TxPayload1Len = (_u16)(((SignatureLen+3) & (~3))); /* align */
    ExtCtrl.pTxPayload1   = (_u8*)pSignature;
    ExtCtrl.RxPayloadLen = (_i16)Msg.Cmd.CertificFileNameLength;
    ExtCtrl.pRxPayload   = (_u8*)pCeritificateFileName; /* Add signature */

    if(ExtCtrl.pRxPayload != NULL &&  ExtCtrl.RxPayloadLen != 0)
    {
       ExtCtrl.RxPayloadLen = ExtCtrl.RxPayloadLen * (-1);
    }

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsCloseCmdCtrl, &Msg, &ExtCtrl));

    return (_i16)((_i16)Msg.Rsp.status);
}
#endif


/*****************************************************************************/
/* sl_FsRead */
/*****************************************************************************/
typedef union
{
  SlFsReadCommand_t	    Cmd;
  SlFsReadResponse_t	Rsp;
}_SlFsReadMsg_u;

#if _SL_INCLUDE_FUNC(sl_FsRead)


static const _SlCmdCtrl_t _SlFsReadCmdCtrl =
{
    SL_OPCODE_NVMEM_FILEREADCOMMAND,
    (_SlArgSize_t)sizeof(SlFsReadCommand_t),
    (_SlArgSize_t)sizeof(SlFsReadResponse_t)
};

_i32 sl_FsRead(const _i32 FileHdl,_u32 Offset, _u8*  pData,_u32 Len)
{
    _SlFsReadMsg_u      Msg;
    _SlCmdExt_t         ExtCtrl;
    _u16      ChunkLen;
    _SlReturnVal_t      RetVal =0;
    _i32                RetCount = 0;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

    _SlDrvMemZero(&ExtCtrl, (_u16)sizeof(_SlCmdExt_t));

    ChunkLen = (_u16)sl_min(MAX_NVMEM_CHUNK_SIZE,Len);
    ExtCtrl.RxPayloadLen = (_i16)ChunkLen;
    ExtCtrl.pRxPayload   = (_u8 *)(pData);
    Msg.Cmd.Offset       = Offset;
    Msg.Cmd.Len          = ChunkLen;
    Msg.Cmd.FileHandle   = (_u32)FileHdl;
    do
    {
        RetVal = _SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsReadCmdCtrl, &Msg, &ExtCtrl);
        if(SL_OS_RET_CODE_OK == RetVal)
        {
            if( Msg.Rsp.status < 0)
            {
                if( RetCount > 0)
                {
                   return RetCount;
                }
                else
                {
                   return Msg.Rsp.status;
                }
            }
            RetCount += (_i32)Msg.Rsp.status;
            Len -= ChunkLen;
            Offset += ChunkLen;
            Msg.Cmd.Offset      = Offset;
            ExtCtrl.pRxPayload   += ChunkLen;
            ChunkLen = (_u16)sl_min(MAX_NVMEM_CHUNK_SIZE,Len);
            ExtCtrl.RxPayloadLen  = (_i16)ChunkLen;
            Msg.Cmd.Len           = ChunkLen;
            Msg.Cmd.FileHandle  = (_u32)FileHdl;
        }
        else
        {
            return RetVal;
        }
    }while(ChunkLen > 0);

    return (_i32)RetCount;
}
#endif

/*****************************************************************************/
/* sl_FsWrite */
/*****************************************************************************/
typedef union
{
  SlFsWriteCommand_t	    Cmd;
  SlFsWriteResponse_t	    Rsp;
}_SlFsWriteMsg_u;


#if _SL_INCLUDE_FUNC(sl_FsWrite)

static const _SlCmdCtrl_t _SlFsWriteCmdCtrl =
{
    SL_OPCODE_NVMEM_FILEWRITECOMMAND,
    (_SlArgSize_t)sizeof(SlFsWriteCommand_t),
    (_SlArgSize_t)sizeof(SlFsWriteResponse_t)
};

_i32 sl_FsWrite(const _i32 FileHdl,_u32 Offset, _u8*  pData,_u32 Len)
{
    _SlFsWriteMsg_u     Msg;
    _SlCmdExt_t         ExtCtrl;
    _u16      ChunkLen;
    _SlReturnVal_t      RetVal;
    _i32                RetCount = 0;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

	_SlDrvMemZero(&ExtCtrl, (_u16)sizeof(_SlCmdExt_t));

    ChunkLen = (_u16)sl_min(MAX_NVMEM_CHUNK_SIZE,Len);
    ExtCtrl.TxPayload1Len = ChunkLen;
    ExtCtrl.pTxPayload1   = (_u8 *)(pData);
    Msg.Cmd.Offset      = Offset;
    Msg.Cmd.Len          = ChunkLen;
    Msg.Cmd.FileHandle  = (_u32)FileHdl;

    do
    {

        RetVal = _SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsWriteCmdCtrl, &Msg, &ExtCtrl);
        if(SL_OS_RET_CODE_OK == RetVal)
        {
            if( Msg.Rsp.status < 0)
            {
                if( RetCount > 0)
                {
                   return RetCount;
                }
                else
                {
                   return Msg.Rsp.status;
                }
            }

            RetCount += (_i32)Msg.Rsp.status;
            Len -= ChunkLen;
            Offset += ChunkLen;
            Msg.Cmd.Offset        = Offset;
            ExtCtrl.pTxPayload1   += ChunkLen;
            ChunkLen = (_u16)sl_min(MAX_NVMEM_CHUNK_SIZE,Len);
            ExtCtrl.TxPayload1Len  = ChunkLen;
            Msg.Cmd.Len           = ChunkLen;
            Msg.Cmd.FileHandle  = (_u32)FileHdl;
        }
        else
        {
            return RetVal;
        }
    }while(ChunkLen > 0);

    return (_i32)RetCount;
}
#endif

/*****************************************************************************/
/* sl_FsGetInfo */
/*****************************************************************************/
typedef union
{
  SlFsGetInfoCommand_t	    Cmd;
  SlFsGetInfoResponse_t    Rsp;
}_SlFsGetInfoMsg_u;


#if _SL_INCLUDE_FUNC(sl_FsGetInfo)


static const _SlCmdCtrl_t _SlFsGetInfoCmdCtrl =
{
    SL_OPCODE_NVMEM_FILEGETINFOCOMMAND,
    (_SlArgSize_t)sizeof(SlFsGetInfoCommand_t),
    (_SlArgSize_t)sizeof(SlFsGetInfoResponse_t)
};



const _u16 FlagsTranslate[] =
{
        SL_FS_INFO_OPEN_WRITE,
        SL_FS_INFO_OPEN_READ,
        SL_FS_INFO_NOT_FAILSAFE,
        SL_FS_INFO_NOT_VALID,
        SL_FS_INFO_SYS_FILE,
        SL_FS_INFO_MUST_COMMIT,
        SL_FS_INFO_BUNDLE_FILE,
        SL_FS_INFO_PENDING_COMMIT,
        SL_FS_INFO_PENDING_BUNDLE_COMMIT,
        0,
        SL_FS_INFO_SECURE,
        SL_FS_INFO_NOSIGNATURE,
        SL_FS_INFO_PUBLIC_WRITE,
        SL_FS_INFO_PUBLIC_READ,
        0,
        0
};



_i16 sl_FsGetInfo(const _u8 *pFileName,const _u32 Token,SlFsFileInfo_t* pFsFileInfo)
{
    _SlFsGetInfoMsg_u    Msg;
    _SlCmdExt_t          CmdExt;
    _u16                 BitNum;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

	_SlDrvMemZero(&CmdExt, (_u16)sizeof(_SlCmdExt_t));

    if ( _SlFsStrlen(pFileName) >= SL_FS_MAX_FILE_NAME_LENGTH )
	{
		return SL_ERROR_FS_WRONG_FILE_NAME;
	}


    CmdExt.TxPayload1Len = (_u16)((_SlFsStrlen(pFileName)+4) & (~3)); /* add 4: 1 for NULL and the 3 for align  */
    CmdExt.pTxPayload1   = (_u8*)pFileName;

    Msg.Cmd.Token       = Token;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsGetInfoCmdCtrl, &Msg, &CmdExt));


    /* convert flags */
    pFsFileInfo->Flags = 0;
    for (BitNum = 0; BitNum < 16; BitNum++ )
    {
        if (( Msg.Rsp.Flags >> BitNum) & 0x1 )
        {
            pFsFileInfo->Flags |= FlagsTranslate[BitNum];
        }
    }

    pFsFileInfo->Len          = Msg.Rsp.FileLen;
	pFsFileInfo->MaxSize      = Msg.Rsp.AllocatedLen;
    pFsFileInfo->Token[0]     = Msg.Rsp.Token[0];
    pFsFileInfo->Token[1]     = Msg.Rsp.Token[1];
    pFsFileInfo->Token[2]     = Msg.Rsp.Token[2];
    pFsFileInfo->Token[3]     = Msg.Rsp.Token[3];
    pFsFileInfo->StorageSize  = Msg.Rsp.FileStorageSize;
    pFsFileInfo->WriteCounter = Msg.Rsp.FileWriteCounter;
    return  (_i16)((_i16)Msg.Rsp.Status);
}
#endif

/*****************************************************************************/
/* sl_FsDel */
/*****************************************************************************/
typedef union
{
  SlFsDeleteCommand_t   	    Cmd;
  SlFsDeleteResponse_t	        Rsp;
}_SlFsDeleteMsg_u;


#if _SL_INCLUDE_FUNC(sl_FsDel)

static const _SlCmdCtrl_t _SlFsDeleteCmdCtrl =
{
    SL_OPCODE_NVMEM_FILEDELCOMMAND,
    (_SlArgSize_t)sizeof(SlFsDeleteCommand_t),
    (_SlArgSize_t)sizeof(SlFsDeleteResponse_t)
};

_i16 sl_FsDel(const _u8 *pFileName,const _u32 Token)
{
    _SlFsDeleteMsg_u Msg;
    _SlCmdExt_t          CmdExt;


    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

    if ( _SlFsStrlen(pFileName) >= SL_FS_MAX_FILE_NAME_LENGTH )
	{
		return SL_ERROR_FS_WRONG_FILE_NAME;
	}


	_SlDrvMemZero(&CmdExt, (_u16)sizeof(_SlCmdExt_t));

    CmdExt.TxPayload1Len = (_u16)((_SlFsStrlen(pFileName)+4) & (~3)); /* add 4: 1 for NULL and the 3 for align */
    CmdExt.pTxPayload1   = (_u8*)pFileName;
    Msg.Cmd.Token       = Token;


    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsDeleteCmdCtrl, &Msg, &CmdExt));

    return  (_i16)((_i16)Msg.Rsp.status);
}
#endif

/*****************************************************************************/
/* sl_FsCtl */
/*****************************************************************************/
typedef union
{
  SlFsFileSysControlCommand_t	    Cmd;
  SlFsFileSysControlResponse_t	    Rsp;
}_SlFsFileSysControlMsg_u;

#if _SL_INCLUDE_FUNC(sl_FsCtl)


const _SlCmdCtrl_t _SlFsFileSysControlCmdCtrl =
{
  SL_OPCODE_NVMEM_NVMEMFILESYSTEMCONTROLCOMMAND,
    sizeof(SlFsFileSysControlCommand_t),
    sizeof(SlFsFileSysControlResponse_t)
};

_i32 sl_FsCtl( SlFsCtl_e Command, _u32 Token,   _u8 *pFileName, const _u8 *pData, _u16 DataLen, _u8 *pOutputData, _u16 OutputDataLen,_u32 *pNewToken )
{
    _SlFsFileSysControlMsg_u     Msg;
    _SlCmdExt_t                  CmdExt;

    /* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

    Msg.Cmd.Token = Token;
    Msg.Cmd.Operation = (_u8)Command;

	_SlDrvMemZero(&CmdExt, (_u16)sizeof(_SlCmdExt_t));

    if ((SL_FS_CTL_ROLLBACK == Command) || (SL_FS_CTL_COMMIT == Command ))
    {
        Msg.Cmd.FileNameLength = _SlFsStrlen(pFileName) + 1 ;

		if ( _SlFsStrlen(pFileName) >= SL_FS_MAX_FILE_NAME_LENGTH )
		{
			return SL_ERROR_FS_WRONG_FILE_NAME;
		}


        /*the data is aligned*/
        CmdExt.RxPayloadLen = DataLen;
        CmdExt.pRxPayload = (_u8 *)(pData);

        CmdExt.TxPayload1Len = (_SlFsStrlen(pFileName) + 4) & (~3);
        CmdExt.pTxPayload1   = pFileName;

        Msg.Cmd.BufferLength =  CmdExt.RxPayloadLen + CmdExt.TxPayload1Len;

       if(CmdExt.pRxPayload != NULL &&  CmdExt.RxPayloadLen != 0)
       {
           CmdExt.RxPayloadLen = CmdExt.RxPayloadLen * (-1);
       }


    }
    else if( SL_FS_CTL_RENAME == Command )
    {
		if ( _SlFsStrlen(pFileName) >= SL_FS_MAX_FILE_NAME_LENGTH )
		{
			return SL_ERROR_FS_WRONG_FILE_NAME;
		}

      Msg.Cmd.FileNameLength = (_SlFsStrlen(pFileName) + 4) & (~3);

        /*current file name*/
        CmdExt.RxPayloadLen = (_u16)Msg.Cmd.FileNameLength;
        CmdExt.pRxPayload   = pFileName;


      /*New file name*/
        CmdExt.TxPayload1Len = (_SlFsStrlen(pData) + 4) & (~3);;
        CmdExt.pTxPayload1 = (_u8 *)(pData);

        Msg.Cmd.BufferLength =  CmdExt.RxPayloadLen + CmdExt.TxPayload1Len;

       if(CmdExt.pRxPayload != NULL &&  CmdExt.RxPayloadLen != 0)
       {
           CmdExt.RxPayloadLen = CmdExt.RxPayloadLen * (-1);
       }

    }
    else
    {
    Msg.Cmd.FileNameLength = 0;

    CmdExt.TxPayload1Len = (DataLen + 3) & (~3);
    CmdExt.pTxPayload1 = (_u8 *)(pData);

    CmdExt.RxPayloadLen = OutputDataLen;
    CmdExt.pRxPayload = pOutputData;

    Msg.Cmd.BufferLength =  CmdExt.TxPayload1Len;
    }



    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsFileSysControlCmdCtrl, &Msg, &CmdExt));

  if( pNewToken != NULL )
  {
      *pNewToken = Msg.Rsp.Token;
  }

    return  (_i32)((_i32)Msg.Rsp.Status);
}
#endif


/*****************************************************************************/
/* sl_FsProgram */
/*****************************************************************************/
typedef union
{
  SlFsProgramCommand_t	    Cmd;
  SlFsProgramResponse_t	    Rsp;
}_SlFsProgrammingMsg_u;

#if _SL_INCLUDE_FUNC(sl_FsProgram)


const _SlCmdCtrl_t _SlFsProgrammingCmdCtrl =
{
  SL_OPCODE_NVMEM_NVMEMFSPROGRAMMINGCOMMAND,
    sizeof(SlFsProgramCommand_t),
    sizeof(SlFsProgramResponse_t)
};

_i32   sl_FsProgram(const _u8*  pData , _u16 DataLen ,const _u8 * pKey ,  _u32 Flags )
{
    _SlFsProgrammingMsg_u     Msg;
    _SlCmdExt_t                CmdExt;
    _u16                       ChunkLen;

    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

    Msg.Cmd.Flags = (_u32)Flags;

   _SlDrvResetCmdExt(&CmdExt);

    /* no data and no key, called only for extracting the image */
    if( (DataLen == 0) && (pKey == NULL) )
    {
        Msg.Cmd.ChunkLen = 0;
        Msg.Cmd.KeyLen = 0;
        Msg.Cmd.Flags = Flags;
        VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsProgrammingCmdCtrl, &Msg, &CmdExt));
    }
    else if( (DataLen> 0)  && ( pData == NULL))
    {
       return( ((_i32)SL_ERROR_FS_WRONG_INPUT_SIZE) << 16 );
    }
    else if( (DataLen == 0) && (pKey != NULL) )
    {
        Msg.Cmd.ChunkLen = 0;
        Msg.Cmd.KeyLen = sizeof(SlFsKey_t);;
        Msg.Cmd.Flags = Flags;
        CmdExt.pTxPayload1 = (_u8*)pKey;
        CmdExt.TxPayload1Len  = sizeof(SlFsKey_t);
        VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsProgrammingCmdCtrl, &Msg, &CmdExt));
    }
    else /* DataLen > 0 */
   {
        if( (DataLen & 0xF) > 0)
        {
            return( ((_i32)SL_ERROR_FS_NOT_16_ALIGNED) << 16 );
        }
        Msg.Cmd.Flags = Flags;

        CmdExt.pTxPayload1   = (_u8 *)pData;
        ChunkLen = (_u16)sl_min(MAX_NVMEM_CHUNK_SIZE, DataLen);

        while(ChunkLen > 0)
        {
            Msg.Cmd.ChunkLen  = ChunkLen;
            CmdExt.TxPayload1Len = ChunkLen;
            if( pKey != NULL )
            {
                Msg.Cmd.KeyLen = sizeof(SlFsKey_t);
                CmdExt.RxPayloadLen = sizeof(SlFsKey_t);
                CmdExt.pRxPayload   = (_u8 *)pKey;

                if(CmdExt.pRxPayload != NULL &&  CmdExt.RxPayloadLen != 0)
                {
                   CmdExt.RxPayloadLen = CmdExt.RxPayloadLen * (-1);
                }
            }
            else /* No key */
            {
                Msg.Cmd.KeyLen = 0;
                CmdExt.RxPayloadLen = 0;
                CmdExt.pRxPayload   = NULL;
            }

            VERIFY_RET_OK( _SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsProgrammingCmdCtrl, &Msg, &CmdExt));

            if( Msg.Rsp.Status <= 0 ) /* Error or finished */
            {
                return (_i32)(Msg.Rsp.Status);
            }

            DataLen -= ChunkLen;
            CmdExt.pTxPayload1 += ChunkLen;

            ChunkLen = (_u16)sl_min(MAX_NVMEM_CHUNK_SIZE, DataLen);
        }
    }
    return  (_i32)(Msg.Rsp.Status);
}
#endif

/*****************************************************************************/
/* sl_FsGetFileList */
/*****************************************************************************/
typedef union
{
  SlFsGetFileListCommand_t	    Cmd;
  SlFsGetFileListResponse_t	    Rsp;
}_SlFsGetFileListMsg_u;

#if _SL_INCLUDE_FUNC(sl_FsGetFileList)


const _SlCmdCtrl_t _SlFsGetFileListCmdCtrl =
{
  SL_OPCODE_NVMEM_NVMEMGETFILELISTCOMMAND,
    sizeof(SlFsGetFileListCommand_t),
    sizeof(SlFsGetFileListResponse_t)
};




_i32  sl_FsGetFileList(_i32* pIndex, _u8 Count, _u8 MaxEntryLen , _u8* pBuff, SlFileListFlags_t Flags )
{
  _SlFsGetFileListMsg_u     Msg;
    _SlCmdExt_t                CmdExt;
    _u16                       OutputBufferSize;


	/* verify that this api is allowed. if not allowed then
    ignore the API execution and return immediately with an error */
    VERIFY_API_ALLOWED(SL_OPCODE_SILO_FS);

    _SlDrvResetCmdExt(&CmdExt);

    Msg.Cmd.Index = *pIndex;
    Msg.Cmd.MaxEntryLen = MaxEntryLen  & (~3); /* round to modulu 4 */
    Msg.Cmd.Count = Count;
    Msg.Cmd.Flags = (_u8)Flags;

    OutputBufferSize = Msg.Cmd.Count * Msg.Cmd.MaxEntryLen;
    if( OutputBufferSize > MAX_NVMEM_CHUNK_SIZE )
    {
      return SL_ERROR_FS_WRONG_INPUT_SIZE;
    }

    CmdExt.RxPayloadLen = OutputBufferSize;
    CmdExt.pRxPayload   = pBuff;

    VERIFY_RET_OK(_SlDrvCmdOp((_SlCmdCtrl_t *)&_SlFsGetFileListCmdCtrl, &Msg, &CmdExt));


    *pIndex = Msg.Rsp.Index;

    return  (_i32)((_i32)Msg.Rsp.NumOfEntriesOrError);
}
#endif

