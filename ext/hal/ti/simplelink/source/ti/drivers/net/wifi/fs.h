/*
 * fs.h - CC31xx/CC32xx Host Driver Implementation
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

#ifndef __FS_H__
#define __FS_H__



#ifdef __cplusplus
extern "C" {
#endif

/*!
    \defgroup FileSystem 
    \short Provides file system capabilities to TI's CC31XX that can be used by both the CC31XX device and the user

*/

/*!

    \addtogroup FileSystem
    @{

*/

/*****************************************************************************/
/* Macro declarations                                                        */
/*****************************************************************************/

/* Create file max size mode  */
#define SL_FS_OPEN_MODE_BIT_MASK    (0xF8000000)
#define SL_NUM_OF_MODE_BIT          (5)

#define SL_FS_OPEN_FLAGS_BIT_MASK   (0x07FE0000)
#define SL_NUM_OF_FLAGS_BIT         (10)

#define SL_FS_OPEN_MAXSIZE_BIT_MASK (0x1FFFF)
#define SL_NUM_OF_MAXSIZE_BIT       (17)


/*
  sl_FsGetInfo and sl_FsGetFileList flags
  ------------------
*/

#define SL_FS_INFO_OPEN_WRITE                    0x1000   /* File is opened for write */
#define SL_FS_INFO_OPEN_READ                     0x800    /* File is opened for read */

#define SL_FS_INFO_MUST_COMMIT                   0x1      /* File is currently open with SL_FS_WRITE_MUST_COMMIT */
#define SL_FS_INFO_BUNDLE_FILE                   0x2      /* File is currently open with SL_FS_WRITE_BUNDLE_FILE */

#define SL_FS_INFO_PENDING_COMMIT                0x4      /* File that was open with SL_FS_WRITE_MUST_COMMIT is closed */
#define SL_FS_INFO_PENDING_BUNDLE_COMMIT         0x8      /* File that was open with SL_FS_WRITE_BUNDLE_FILE is closed */

#define SL_FS_INFO_NOT_FAILSAFE                  0x20     /* File was not created with SL_FS_CREATE_FAILSAFE */
#define SL_FS_INFO_NOT_VALID                     0x100    /* No valid image exists for the file */
#define SL_FS_INFO_SYS_FILE                      0x40     /* File is system file */
#define SL_FS_INFO_SECURE                        0x10     /* File is secured */
#define SL_FS_INFO_NOSIGNATURE                   0x2000   /* File is unsigned, the flag is returns only for sl_FsGetInfo function and not for sl_FsGetFileList */
#define SL_FS_INFO_PUBLIC_WRITE                  0x200    /* File is open for public write */
#define SL_FS_INFO_PUBLIC_READ                   0x400    /* File is open for public read */


/*
  fs_Open flags
  --------------
*/

/* mode */
#define     SL_FS_CREATE                         ((_u32)0x1<<(SL_NUM_OF_MAXSIZE_BIT+SL_NUM_OF_FLAGS_BIT))
#define     SL_FS_WRITE                          ((_u32)0x2<<(SL_NUM_OF_MAXSIZE_BIT+SL_NUM_OF_FLAGS_BIT))
#define     SL_FS_OVERWRITE                      ((_u32)0x4<<(SL_NUM_OF_MAXSIZE_BIT+SL_NUM_OF_FLAGS_BIT))
#define     SL_FS_READ                           ((_u32)0x8<<(SL_NUM_OF_MAXSIZE_BIT+SL_NUM_OF_FLAGS_BIT))
/* creation flags */
#define     SL_FS_CREATE_FAILSAFE                ((_u32)0x1<<SL_NUM_OF_MAXSIZE_BIT)         /* Fail safe */
#define     SL_FS_CREATE_SECURE                  ((_u32)0x2<<SL_NUM_OF_MAXSIZE_BIT)         /* SECURE */
#define     SL_FS_CREATE_NOSIGNATURE             ((_u32)0x4<<SL_NUM_OF_MAXSIZE_BIT)         /* Relevant to secure file only  */
#define     SL_FS_CREATE_STATIC_TOKEN            ((_u32)0x8<<SL_NUM_OF_MAXSIZE_BIT)         /* Relevant to secure file only */
#define     SL_FS_CREATE_VENDOR_TOKEN            ((_u32)0x10<<SL_NUM_OF_MAXSIZE_BIT)        /* Relevant to secure file only */
#define     SL_FS_CREATE_PUBLIC_WRITE            ((_u32)0x20<<SL_NUM_OF_MAXSIZE_BIT)        /* Relevant to secure file only, the file can be opened for write without Token */
#define     SL_FS_CREATE_PUBLIC_READ             ((_u32)0x40<<SL_NUM_OF_MAXSIZE_BIT)        /* Relevant to secure file only, the file can be opened for read without Token  */

#define     SL_FS_CREATE_MAX_SIZE( MaxFileSize ) ((((_u32)MaxFileSize + 255) / 256 ) & SL_FS_OPEN_MAXSIZE_BIT_MASK )

/* open for write flags */
#define    SL_FS_WRITE_MUST_COMMIT               ((_u32)0x80<<SL_NUM_OF_MAXSIZE_BIT)        /* The file is locked for changes */
#define    SL_FS_WRITE_BUNDLE_FILE               ((_u32)0x100<<SL_NUM_OF_MAXSIZE_BIT)       /* The file is locked for changes as part of Bundle */
#define    SL_FS_WRITE_ENCRYPTED                 ((_u32)0x200<<SL_NUM_OF_MAXSIZE_BIT)       /* This indicates the start of a secured content write session */

/*****************************************************************************/
/* Structure/Enum declarations                                               */
/*****************************************************************************/

typedef enum
{
    SL_FS_TOKEN_MASTER               = 0,
    SL_FS_TOKEN_WRITE_READ           = 1,
    SL_FS_TOKEN_WRITE_ONLY           = 2,
    SL_FS_TOKEN_READ_ONLY            = 3
}SlFsTokenId_e;

typedef struct
{
    _u16  Flags;                    /* see Flags options */
    _u32  Len;                      /* In bytes, The actual size of the copy which is used for read*/
    _u32  MaxSize;                  /* In bytes, The max file size  */
    _u32  Token[4];                 /* see SlFsTokenId_e */
    _u32  StorageSize;              /* In bytes, The total size that the file required on the storage including the mirror */
    _u32  WriteCounter;             /* Number of times in which the file have been written successfully */
}SlFsFileInfo_t;


/*
  sl_FsCtl
  --------
*/
typedef enum
{
  SL_FS_CTL_RESTORE = 0,   /* restores the factory default */
  SL_FS_CTL_ROLLBACK = 1,
  SL_FS_CTL_COMMIT = 2,
  SL_FS_CTL_RENAME = 3,
  SL_FS_CTL_GET_STORAGE_INFO = 5,
  SL_FS_CTL_BUNDLE_ROLLBACK = 6,
  SL_FS_CTL_BUNDLE_COMMIT = 7
}SlFsCtl_e;

typedef enum
{
  SL_FS_BUNDLE_STATE_STOPPED = 0,
  SL_FS_BUNDLE_STATE_STARTED = 1,
  SL_FS_BUNDLE_STATE_PENDING_COMMIT = 3
}SlFsBundleState_e;

typedef struct
{
    _u32  Key[4];/*16 bytes*/
}SlFsKey_t;

typedef struct
{
  _u8   Index;
}SlFsFileNameIndex_t;

typedef union
{
  SlFsFileNameIndex_t  Index;
  _i32                 ErrorNumber;
}SlFsFileNameIndexOrError_t;

/* File control helper structures */

/*SL_FS_CTL_RESTORE*/
typedef enum
{
  SL_FS_FACTORY_RET_TO_IMAGE =  0,/*The system will be back to the production image.*/
  SL_FS_FACTORY_RET_TO_DEFAULT = 2 /*return to factory default*/
}SlFsRetToFactoryOper_e;

typedef struct
{
  _u32 Operation;/*see _SlFsRetToFactoryOper_e*/
}SlFsRetToFactoryCommand_t;

/******************* Input flags end *****************************************/

typedef struct
{
  _u32           IncludeFilters;

}SlFsControl_t;

typedef struct
{
  _u16 DeviceBlockSize;
  _u16 DeviceBlocksCapacity;
  _u16 NumOfAllocatedBlocks;
  _u16 NumOfReservedBlocks;
  _u16 NumOfReservedBlocksForSystemfiles;
  _u16 LargestAllocatedGapInBlocks;
  _u16 NumOfAvailableBlocksForUserFiles;
  _u8  Padding[2];
} SlFsControlDeviceUsage_t;

typedef struct
{
  _u8  MaxFsFiles;
  _u8  IsDevlopmentFormatType;
  _u8  Bundlestate; /*see SlFsBundleState_e*/
  _u8  Reserved;
  _u8  MaxFsFilesReservedForSysFiles;
  _u8  ActualNumOfUserFiles;
  _u8  ActualNumOfSysFiles;
  _u8  Padding;
  _u32 NumOfAlerts;
  _u32 NumOfAlertsThreshold;
  _u16 FATWriteCounter;/*Though it is increased during the programming, the programming and ret to factory takes only 1- write to the FAT, independ of the number of the programming files */
  _u16 Padding2;
}SlFsControlFilesUsage_t;

/*SL_FS_CTL_GET_STORAGE_INFO*/
typedef struct
{
    SlFsControlDeviceUsage_t DeviceUsage;
    SlFsControlFilesUsage_t  FilesUsage;
} SlFsControlGetStorageInfoResponse_t;

typedef struct
{
   _u32   IncludeFilters; /* see SlFsControlFilterCounterFlags_e*/
   _u8    OpenedForWriteCnt;
   _u8    OpeneForReadCnt;
   _u8    ClosedFilesCnt;
   _u8    OpenedForWriteCntWithValidFailSafeImage;
   _u8    OpeneForReadCntWithValidFailSafeImage;
   _u8    ClosedFilesCntWithValidFailSafeImage;
   _u8    padding[2];
} SlFsControlGetCountersResponse_t;

/* GetFileList */
#define SL_FS_MAX_FILE_NAME_LENGTH                       180

typedef enum
{
    SL_FS_GET_FILE_ATTRIBUTES = 0x1
}SlFileListFlags_t;




typedef struct
{
   _u32  FileMaxSize;
   _u32  Properties; /* see SL_FS_INFO_  flags */
   _u32  FileAllocatedBlocks;/*1 block = 4096 bytes*/
}SlFileAttributes_t;
/*!
    \cond DOXYGEN_REMOVE
*/
/*****************************************************************************/
/* external Function prototypes                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Function prototypes                                                       */
/*****************************************************************************/
/*!
    \endcond
*/
/*!
    \brief open file for read or write from/to storage device

    \param[in]      pFileName                  File Name buffer pointer
    \param[in]      AccessModeAndMaxSize       Options: As described below
    \param[in]      pToken                     input Token for read, output Token for write

     AccessModeAndMaxSize possible input                                                                        \n
     SL_FS_READ                                        - Read a file                                                                  \n
     SL_FS_WRITE                                       - Open for write for an existing file (whole file content needs to be rewritten)\n
     SL_FS_CREATE|maxSizeInBytes,accessModeFlags
     SL_FS_CREATE|SL_FS_OVERWRITE|maxSizeInBytes,accessModeFlags  - Open for creating a new file. Max file size is defined in bytes.             \n
                                                                    For optimal FS size, use max size in 4K-512 bytes steps (e.g. 3584,7680,117760)  \n
                                                                    Several access modes bits can be combined together from SlFileOpenFlags_e enum

    \return         File handle on success. Negative error code on fail

    \sa             sl_FsRead sl_FsWrite sl_FsClose
    \note           belongs to \ref basic_api
    \warning
    \par            Example

    - Creating file and writing data to it
    \code
        char*           DeviceFileName = "MyFile.txt";
        unsigned long   MaxSize = 63 * 1024; //62.5K is max file size
        long            DeviceFileHandle = -1;
        _i32            RetVal;        //negative retval is an error
        unsigned long   Offset = 0;
        unsigned char   InputBuffer[100];
        _u32            MasterToken = 0;
 
        // Create a file and write data. The file in this example is secured, without signature and with a fail safe commit
 
        //create a secure file if not exists and open it for write.
        DeviceFileHandle =  sl_FsOpen(unsigned char *)DeviceFileName,
                                      SL_FS_CREATE|SL_FS_OVERWRITE | SL_FS_CREATE_SECURE | SL_FS_CREATE_NOSIGNATURE | SL_FS_CREATE_MAX_SIZE( MaxSize ),
                                      &MasterToken);
 
        Offset = 0;
        //Preferred in secure file that the Offset and the length will be aligned to 16 bytes.
        RetVal = sl_FsWrite( DeviceFileHandle, Offset, (unsigned char *)"HelloWorld", strlen("HelloWorld"));
 
        RetVal = sl_FsClose(DeviceFileHandle, NULL, NULL , 0);
 
        // open the same file for read, using the Token we got from the creation procedure above
        DeviceFileHandle =  sl_FsOpen(unsigned char *)DeviceFileName,
                                      SL_FS_READ,
                                      &MasterToken);
 
        Offset = 0;
        RetVal = sl_FsRead( DeviceFileHandle, Offset, (unsigned char *)InputBuffer, strlen("HelloWorld"));
 
        RetVal = sl_FsClose(DeviceFileHandle, NULL, NULL , 0);
    \endcode
    <br>

    - Create a non secure file if not already exists and open it for write
    \code
        DeviceFileHandle = sl_FsOpen((unsigned char *)DeviceFileName,
                                      SL_FS_CREATE|SL_FS_OVERWRITE| SL_FS_CREATE_MAX_SIZE( MaxSize ),
                                      NULL);
    \endcode
    
    \note             Some of the flags are creation flags and can only be set when the file is created. When opening the file for write the creation flags are ignored. For more information, refer to chapter 8 in the user manual.
              
*/

#if _SL_INCLUDE_FUNC(sl_FsOpen)
_i32 sl_FsOpen(const _u8 *pFileName,const _u32 AccessModeAndMaxSize,_u32 *pToken);
#endif

/*!
    \brief Close file in storage device

    \param[in]      FileHdl               Pointer to the file (assigned from sl_FsOpen)
    \param[in]      pCeritificateFileName Certificate file, or NULL if irrelevant.
    \param[in]      pSignature            The signature is SHA-1, the certificate chain may include SHA-256
    \param[in]      SignatureLen          The signature actual length

    \return         Zero on success, or a negative value if an error occurred
    \sa             sl_FsRead sl_FsWrite sl_FsOpen
    \note           Call the fs_Close  with signature = 'A' signature len = 1 for activating an abort action\n
                    Creating signature : OpenSSL> dgst -binary -sha1 -sign <file-location>\<private_key>.pem -out <file-location>\<output>.sig <file-location>\<input>.txt
    \warning
    \par            Examples
    
    - Closing file: 
    \code
        _i16 RetVal;
        RetVal = sl_FsClose(FileHandle,0,0,0);
    \endcode
    <br>

    - Aborting file:
    \code
        _u8  Signature;
        Signature = 'A';
        sl_FsClose(FileHandle,0,&Signature, 1);
    \endcode
    
    \note            In case the file was opened as not secure file or as secure-not signed, any certificate or signature provided are ignored, those fields should be set to NULL.
*/
#if _SL_INCLUDE_FUNC(sl_FsClose)
_i16 sl_FsClose(const _i32 FileHdl,const _u8* pCeritificateFileName,const _u8* pSignature,const _u32 SignatureLen);
#endif

/*!
    \brief Read block of data from a file in storage device

    \param[in]      FileHdl Pointer to the file (assigned from sl_FsOpen)
    \param[in]      Offset  Offset to specific read block
    \param[out]     pData   Pointer for the received data
    \param[in]      Len     Length of the received data

    \return         Number of read bytes on success, negative error code on failure

    \sa             sl_FsClose sl_FsWrite sl_FsOpen
    \note           belongs to \ref basic_api
    \warning
    \par            Example
    
    - Reading File:
    \code
    Status = sl_FsRead(FileHandle, 0, &readBuff[0], readSize);
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_FsRead)
_i32 sl_FsRead(const _i32 FileHdl,_u32 Offset ,_u8*  pData,_u32 Len);
#endif

/*!
    \brief Write block of data to a file in storage device

    \param[in]      FileHdl  Pointer to the file (assigned from sl_FsOpen)
    \param[in]      Offset   Offset to specific block to be written
    \param[in]      pData    Pointer the transmitted data to the storage device
    \param[in]      Len      Length of the transmitted data

    \return         Number of wireted bytes on success, negative error code on failure

    \sa
    \note           belongs to \ref basic_api
    \warning
    \par            Example
    
    - Writing file:
    \code
    Status = sl_FsWrite(FileHandle, 0, &buff[0], readSize);
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_FsWrite)
_i32 sl_FsWrite(const _i32 FileHdl,_u32 Offset,_u8*  pData,_u32 Len);
#endif

/*!
    \brief Get information of a file

    \param[in]      pFileName    File name
    \param[in]      Token        File token. if irrelevant set to 0.
    \param[out]     pFsFileInfo Returns the File's Information (SlFsFileInfo_t)
                                - Flags
                                - File size 
                                - Allocated size 
                                - Tokens

    \return         Zero on success, negative error code on failure \n
                    When file not exists : SL_ERROR_FS_FILE_NOT_EXISTS
    \note           
                    - If the return value is SL_ERROR_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY or  SL_ERROR_FS_FILE_IS_ALREADY_OPENED information about the file is valid.
                    - Belongs to \ref basic_api

    \sa             sl_FsOpen
    \warning
    \par            Example

    - Getting file info:
    \code
    Status = sl_FsGetInfo("FileName.html",Token,&FsFileInfo);
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_FsGetInfo)
_i16 sl_FsGetInfo(const _u8 *pFileName,const _u32 Token,SlFsFileInfo_t* pFsFileInfo);
#endif

/*!
    \brief Delete specific file from a storage or all files from a storage (format)

    \param[in]      pFileName    File Name
    \param[in]      Token        File token. if irrelevant set to 0
    \return         Zero on success, or a negative value if an error occurred

    \sa
    \note           belongs to \ref basic_api
    \warning
    \par            Example

    - Deleting file:
    \code
        Status = sl_FsDel("FileName.html",Token);
    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_FsDel)
_i16 sl_FsDel(const _u8 *pFileName,const _u32 Token);
#endif



/*!
    \brief Controls various file system operations

   \param[in]  Command , the command to execute, \see SlFsCtl_e
    SL_FS_CTL_RESTORE , Return to factory default, return to factory image , see fs programming
    SL_FS_CTL_ROLLBACK , Roll-back file which was created with 'SL_FS_WRITE_MUST_COMMIT'
    SL_FS_CTL_COMMIT,Commit file which was created with 'SL_FS_WRITE_MUST_COMMIT'
    SL_FS_CTL_RENAME, Rename file
    SL_FS_CTL_GET_STORAGE_INFO, Total size of storage , available size of storage
    SL_FS_CTL_BUNDLE_ROLLBACK, Rollback bundle files
    SL_FS_CTL_BUNDLE_COMMIT, Commit Bundle files
    \param[in]      Token         Set to NULL if not relevant to the command
    \param[in]      pFileName     Set to NULL if not relevant to the command
    \param[in]      pData         The data according the command.
    \param[in]      DataLen       Length of data buffer
    \param[out]     pOutputData   Buffer for the output data
    \param[out]     OutputDataLen Length of the output data buffer
    \param[out]     pNewToken     The new valid file token, if irrelevant can be set to NULL.
    \return         
                    - Zero on success, or a negative value if an error occurred
                    - For SL_FS_CTL_BUNDLE_ROLLBACK, On success bundle the new bundle state is returned (see SlFsBundleState_e) else negative error number
                    - For SL_FS_CTL_BUNDLE_COMMIT, On success the new bundle state is returned (see SlFsBundleState_e) else negative error number
    
    \sa
    \note           belongs to \ref ext_api
    \warning
    \par            Examples

    - SL_FS_CTL_ROLLBACK:
    \code
            FsControl.IncludeFilters = 0;
            slRetVal = sl_FsCtl( (SlFsCtl_e)SL_FS_CTL_FILE_ROLLBACK, Token, NWPfileName ,(_u8 *)&FsControl, sizeof(SlFsControl_t), NULL, 0 , pNewToken);
    \endcode
    <br>

    - SL_FS_CTL_COMMIT:
    \code
            FsControl.IncludeFilters = 0;
            slRetVal = sl_FsCtl(SL_FS_CTL_COMMIT, Token, NWPfileName ,(_u8 *)&FsControl, sizeof(SlFsControl_t), NULL, 0, pNewToken );
    \endcode
    <br>

    - SL_FS_CTL_RENAME:
    \code
            slRetVal = sl_FsCtl(SL_FS_CTL_RENAME, Token, NWPfileName, NewFileName, 0, NULL, 0, NULL );
    \endcode
    <br>

    - SL_FS_CTL_GET_STORAGE_INFO:
    \code
        _i32  GetStorageInfo( SlFsControlGetStorageInfoResponse_t* pSlFsControlGetStorageInfoResponse )
        {
            _i32 slRetVal;

            slRetVal = sl_FsCtl( ( SlFsCtl_e)SL_FS_CTL_GET_STORAGE_INFO, 0, NULL , NULL , 0, (_u8 *)pSlFsControlGetStorageInfoResponse, sizeof(SlFsControlGetStorageInfoResponse_t), NULL );
            return slRetVal;
        }
    \endcode
    <br>

    - SL_FS_CTL_RESTORE:
    \code
        //Return 0 for OK, else Error
        _i32 ProgramRetToImage( )
        {
            _i32 slRetVal;
            SlFsRetToFactoryCommand_t RetToFactoryCommand;
            _i32 RetVal, ExtendedError;

            RetToFactoryCommand.Operation = SL_FS_FACTORY_RET_TO_IMAGE;
            slRetVal = sl_FsCtl( (SlFsCtl_e)SL_FS_CTL_RESTORE, 0, NULL , (_u8 *)&RetToFactoryCommand , sizeof(SlFsRetToFactoryCommand_t), NULL, 0 , NULL );
            if ((_i32)slRetVal < 0)
                {
                    //Pay attention, for this function the slRetVal is composed from Signed RetVal & extended error
                    RetVal = (_i16)slRetVal>> 16;
                    ExtendedError = (_u16)slRetVal& 0xFFFF;
                    printf("\tError SL_FS_FACTORY_RET_TO_IMAGE, 5d, %d\n", RetVal, ExtendedError);
                    return slRetVal;
            }
            //Reset
            sl_Stop(0);
            Sleep(1000);
            sl_Start(NULL, NULL, NULL);

            return slRetVal;
        }
    \endcode
    <br>
    
    - SL_FS_CTL_BUNDLE_ROLLBACK:
    \code    
        //return 0 for O.K else negative
        _i32 BundleRollback()
        {
            _i32 slRetVal = 0;
            SlFsControl_t FsControl;
            FsControl.IncludeFilters = 0; //Use default behaviour
            slRetVal = sl_FsCtl( (SlFsCtl_e)SL_FS_CTL_BUNDLE_ROLLBACK, 0, NULL ,(_u8 *)&FsControl, sizeof(SlFsControl_t), NULL, 0 , NULL);
            return slRetVal;
        }
    \endcode
    <br>

    - SL_FS_CTL_BUNDLE_COMMIT:
    \code
        //return 0 for O.K else negative
        _i32 BundleCommit()
        {
            _i32 slRetVal = 0;
            SlFsControl_t FsControl;
            FsControl.IncludeFilters = 0; //Use default behaviour
            slRetVal = sl_FsCtl( (SlFsCtl_e)SL_FS_CTL_BUNDLE_COMMIT, 0, NULL ,(_u8 *)&FsControl, sizeof(SlFsControl_t), NULL, 0 , NULL);
            return slRetVal;
        }
    \endcode
 */
#if _SL_INCLUDE_FUNC(sl_FsCtl)
_i32   sl_FsCtl(  SlFsCtl_e Command,  _u32 Token,  _u8 *pFileName, const _u8 *pData, _u16 DataLen, _u8 *pOutputData, _u16 OutputDataLen,_u32 *pNewToken );
#endif
/*!
    \brief Enables to format and configure the device with pre-prepared configuration

    \param[in]      Flags          For future use
    \param[in]      pKey        In case the ucf is encrypted the encryption key, otherwise NULL
    \param[in]      pData       The file is download in data chunks, the chunk size should be aligned to 16 bytes, if no data Set to NULL
    \param[in]      Len         The length of pData in bytes
    \return         The return value is:
                    - On error < 0 , contains the error number and extended error number
                    - On success > 0, represent the number of bytes received
                    - On successful end == 0 , when all file chunks are download
    \sa
    \note           belongs to \ref ext_api
    \warning
    \par            Example 
    
    - FS programming:
    \code

        //Return 0 for OK, else Error
        _i32 ProgramImage( char* UcfFileName, char * KeyFileName )
        {
            #define PROGRAMMING_CHUNK_SIZE 4096
            _i32 slRetVal = 0;
            SlFsKey_t Key;
            FILE *hostFileHandle = NULL;
            _u16 bytesRead;
            _u8 DataBuf[PROGRAMMING_CHUNK_SIZE];
            FILE *KeyFileHandle = NULL;
            short ErrorNum;
            unsigned short ExtendedErrorNum;
            time_t start,end;
            double dif;
            _u8* pKey = NULL;
            errno_t err;

            if (KeyFileName != "")
            {
                //Read key
                err   = fopen_s( &KeyFileHandle, KeyFileName, "rb");
                if (err != 0)
                {
                    return __LINE__;//error
                }
                fread((_u8*)&Key, 1, sizeof(SlFsKey_t), KeyFileHandle);
                fclose(KeyFileHandle);
                pKey = (_u8*)&Key;
            }

            // Downlaoding the Data with the key, the key can be set only in the first chunk,no need to download it with each chunk
            if (UcfFileName != "")
            {
                //Read data
                 err   = fopen_s( &hostFileHandle, UcfFileName, "rb");
                if (err != 0)
                {
                    return __LINE__;//error
                }

                time (&start);

                bytesRead = fread(DataBuf, 1, PROGRAMMING_CHUNK_SIZE, hostFileHandle);

                while ( bytesRead  )
                {
                    slRetVal =  sl_FsProgram( DataBuf , bytesRead , (_u8*)pKey,  0 );
                    if(slRetVal ==  SL_API_ABORTED)//timeout
                    {
                        return( slRetVal );
                    }
                    else if (slRetVal < 0 )//error
                    {
                        ErrorNum = (long)slRetVal >> 16;
                        ExtendedErrorNum = (_u16)(slRetVal & 0xFFFF);
                        printf("\tError sl_FsProgram = %d , %d \n", ErrorNum, ExtendedErrorNum);
                        fclose(hostFileHandle);
                        return( ErrorNum );
                    }
                    if(slRetVal == 0)//finished succesfully
                        break;
                    pKey = NULL;//no need to download the key with each chunk;
                    bytesRead = fread(DataBuf, 1, PROGRAMMING_CHUNK_SIZE, hostFileHandle);
                }


                time (&end);
                dif = difftime (end,start);
            #ifdef PRINT
                printf ("\tProgramming took %.2lf seconds to run.\n", dif );
            #endif
                //The file was downloaded but it was not detected by the programming as the EOF.
                if((bytesRead == 0 ) && (slRetVal > 0 ))
                {
                    return __LINE__;//error
                }


                fclose(hostFileHandle);
            }//if (UcfFileName != "")

            //this scenario is in case the image was already "burned" to the SFLASH by external tool and only the key is downloaded
            else if (KeyFileName != "")
            {
                slRetVal =  sl_FsProgram(NULL , 0 , (_u8*)pKey,  0 );
                if (slRetVal < 0)//error
                {
                    ErrorNum = (long)slRetVal >> 16;
                    ExtendedErrorNum = (_u16)slRetVal && 0xFF;;
                    printf("\tError sl_FsProgram = %d , %d \n", ErrorNum, ExtendedErrorNum);
                    fclose(hostFileHandle);
                    return( ErrorNum );
                }
            }

            if( slRetVal == 0 )
            {
                //Reset the nWP
                sl_Stop(100);
                Sleep(1000);
                sl_Start(NULL, NULL, NULL);
                Sleep(2000);
             }

            return slRetVal;

        }

    \endcode
*/

#if _SL_INCLUDE_FUNC(sl_FsProgram)
_i32   sl_FsProgram(const _u8*  pData , _u16 Len , const _u8 * pKey ,  _u32 Flags );
#endif
/*!
    \brief The list of file names, the files are retrieve in chunks

    \param[in, out] pIndex      The first chunk should start with value of  -1, afterwards the Index from the previous call should be set as input\n
                                Returns current chunk intex, start the next chunk from that number
    \param[in]      Count       Number of entries to retrieve
    \param[in]      MaxEntryLen The total size of the buffer is Count * MaxEntryLen
    \param[out]     pBuff       The buffer contains list of SlFileAttributes_t + file name
    \param[in]      Flags       Is to retrieve file attributes see SlFileAttributes_t.
    \return         The actual number of entries which are contained in the buffer. On error negative number which contains the error number.
    \sa
    \note           belongs to \ref ext_api
    \warning
    \par            Example

    - Getting file list
    \code
        typedef struct
        {
            SlFileAttributes_t attribute;
            char fileName[SL_FS_MAX_FILE_NAME_LENGTH];
        }slGetfileList_t;
    
        #define COUNT 5
    
        void PrintFileListProperty(_u16 prop);
    
        INT32 GetFileList()
        {
            _i32 NumOfEntriesOrError = 1;
            _i32 Index = -1;
            slGetfileList_t File[COUNT];
            _i32  i;
            _i32 RetVal = 0;
    
            printf("%\n");
            while( NumOfEntriesOrError > 0 )
            {
                NumOfEntriesOrError = sl_FsGetFileList( &Index, COUNT, (_u8)(SL_FS_MAX_FILE_NAME_LENGTH + sizeof(SlFileAttributes_t)), (unsigned char*)File, SL_FS_GET_FILE_ATTRIBUTES);
                if (NumOfEntriesOrError < 0)
                {
                    RetVal = NumOfEntriesOrError;//error
                    break;
                }
                for (i = 0; i < NumOfEntriesOrError; i++)
                {
                   printf("Name: %s\n", File[i].fileName);
                   printf("AllocatedBlocks: %5d ",File[i].attribute.FileAllocatedBlocks);
                   printf("MaxSize(byte): %5d \n", File[i].attribute.FileMaxSize);
                   PrintFileListProperty((_u16)File[i].attribute.Properties);
                   printf("%\n\n");
               }
           }
           printf("%\n");
           return RetVal;//0 means O.K
       }
   
       void PrintFileListProperty(_u16 prop)
       {
           printf("Flags : ");
           if (prop & SL_FS_INFO_MUST_COMMIT)
               printf("Open file commit,");
           if (prop & SL_FS_INFO_BUNDLE_FILE)
               printf("Open bundle commit,");
           if (prop & SL_FS_INFO_PENDING_COMMIT)
               printf("Pending file commit,");
           if (prop & SL_FS_INFO_PENDING_BUNDLE_COMMIT)
                printf("Pending bundle commit,");
            if (prop & SL_FS_INFO_SECURE)
                printf("Secure,");
            if (prop & SL_FS_INFO_NOT_FAILSAFE)
                printf("File safe,");
            if (prop & SL_FS_INFO_SYS_FILE)
                printf("System,");
            if (prop & SL_FS_INFO_NOT_VALID)
                printf("No valid copy,");
            if (prop & SL_FS_INFO_PUBLIC_WRITE)
                printf("Public write,");
            if (prop & SL_FS_INFO_PUBLIC_READ)
                printf("Public read,");
        }

    \endcode
*/
#if _SL_INCLUDE_FUNC(sl_FsGetFileList)
_i32  sl_FsGetFileList(_i32* pIndex, _u8 Count, _u8 MaxEntryLen , _u8* pBuff, SlFileListFlags_t Flags );
#endif

/*!

 Close the Doxygen group.
 @}

 */

#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif /*  __FS_H__ */

