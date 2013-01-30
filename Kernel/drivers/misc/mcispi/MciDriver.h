/* drivers/misc/mcispi/MciDriver.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciDriver.h
**
**  Description
**      Header file for the base operation of MCI Drivers
**
**  Authur:
**      Seunghan Kim (hyenakim@samsung.com)
**      Jongho Park (jongho0910.park@samsung.com)
**
**  Update:
**      2009.1.20  NEW     Created.
**/

#ifndef _MCI_DRIVER_H_
#define _MCI_DRIVER_H_

#include "MciTypeDefs.h"
#include "MciIoctl.h"
#include "MciCommon.h"
#include "MciBufDesc.h"
#include "MciQueue.h"
#include "MciDriver_HW.h"




#define MCI_DRIVER_MAJOR_VERSION            2
#define MCI_DRIVER_MINOR_VERSION            0
#define MCI_DRIVER_RELEASE_VERSION          0

#define DBGON                               0

//////////////////////////////////////////////////////////////////////////
/////   Compiler Directives
/////////////////////////////////////////////////////////////////////////
// Debug Related Compile Directives
#define _MCI_DBG_MSG_TX                     0
#define _MCI_DBG_MSG_RX                     0
#define _MCI_DBG_TRACE_FUNC                 0


#if 1	// M900 

/*
 *  MciSpi's Buffer Definition
 */
#define MCI_DRV_NUM_OF_BUF_DESC                     5           // # of Message Buffer Descriptors and buffers
#define MCI_DRV_MAX_STREAM_BUF_SIZE                 0xA000/*0x1E000*/       // max size of Stream Desc and Buffer in Burst Transfer Mode
#define MCI_DRV_MAX_MESSAGE_BUF_SIZE                600           // max size of Message Desc and Buffer

/*
 *  Definition for Channel MIS Message
 */
#define MCI_DRV_TOTAL_SIZE_OF_BUFFER_MEMORY       0x2A000//0x200000 //0x10000// 0x200000//0x200000         // size: 2Mbytes

#else
	/*
	 *  MciSpi's Buffer Definition
	 */
	#define MCI_DRV_NUM_OF_BUF_DESC                     100           // # of Message Buffer Descriptors and buffers
	#define MCI_DRV_MAX_STREAM_BUF_SIZE                 0x10000       // max size of Stream Desc and Buffer in Burst Transfer Mode
	#define MCI_DRV_MAX_MESSAGE_BUF_SIZE                400           // max size of Message Desc and Buffer

	/*
	 *  Definition for Channel MIS Message
	 */
	#define MCI_DRV_TOTAL_SIZE_OF_BUFFER_MEMORY       0x200000 //0x40000//0x10000// 0x200000//0x200000         // size: 2Mbytes
#endif

#define LTE_ALWAYS_ON_FILE             "/data/factory/Lte.AlwaysOn"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct _MCI_DRIVER
{
    // Count for the opened drivers
    UInt32              iOpenedDrvCount;
    BOOL                bAttached;

    MCI_HW_CONTEXT      MciHwCtx;
    UInt32              uiDrvState;

    H_LOCK              hLockHwAccess;

    // Buffer Descriptors for Stream
    UInt32              uiMaxStreamBufferSize;
    UInt32              uiStreamBufferDescNum;
    MCI_QUEUE           QReceivedStreamDesc;
    MCI_QUEUE           QFreeStreamDesc;
    H_LOCK              hLockStreamDescQueue;

    // Buffer Descriptors for Message
    UInt32              uiMaxMessageBufferSize;
    UInt32              uiMessageBufferDescNum;
    MCI_QUEUE           QReceivedMessageDesc;
    MCI_QUEUE           QFreeMessageDesc;
    MCI_QUEUE           QTxReqMessageDesc;
    H_LOCK              hLockMessageDescQueue;

    volatile ULong      lRxStarted;
    volatile ULong      lTxStarted;

	// For HalAllocateCommonBuffer
	H_PHYSMEM           hDmaMemory;
	pVoid				lpvFixedBufferMemory;
	pVoid				lpvPhysFixedBufferMemory;

	// Burst Stream Transfer Mode (only in DVB-H IP Transfer)
    PMCI_BUF_DESC       pCurStreamBufDesc;
	PMCI_BUF_DESC		pPendedDesc;
    
    H_EVENT				hMciNotifyReceivedTsStreamEvent;
    H_EVENT             hMciNotifyReceivedIpStreamEvent;
    H_EVENT             hMciNotifyReceivedStreamEvent;
    H_EVENT             hMciNotifyReceivedMessageEvent;
    H_EVENT             hMciNotifyStatusChangeEvent;


    BOOL                bCompleteDrvInit;

    // Timeout value
    UInt32              uiWaitTimeForStream;
    UInt32              uiWaitTimeForMessage;
    UInt32              uiWaitTimeForStateChange;
    // State Change Info
    UInt32              uiDriverStateChangeInfo;

    pTStr               pImagePath;
    UInt32              uiBufferRetryCount;
    UInt32              uiBufferRetryInterval;
    UInt32              uiMtvType;
    UInt32              uiMciType;

    // message type
    UInt32              uiTypeFreqScan;
    UInt32              uiTypeCreateFilter;
    UInt32              uiTypeDeleteFilter;
    UInt32              uiTypeStatus;
    UInt32              uiTypeStreamData;
	UInt32				uiTypeBootSuccess;
    
    UInt32              uiStreamCntPerSec;
    
    //
    Bool                bSyncAccessForStream;
    Bool                bSyncAccessForMessage;
    Bool                bSyncAccessForState;

    H_EVENT             hClosedAllEvent;
    Bool                bLoadNetworkAdpater;
    UChar		        ucBootSuccessBuffer[1024];
    UInt32			    uiBootMsgLength;	
    Bool		        bIsBootSucess;	

    struct wake_lock    wlock;
    long                wkae_time;
	UChar				ucSavedLteWakeonState;
} MCI_DRIVER, *PMCI_DRIVER, **PPMCI_DRIVER;

extern PMCI_DRIVER g_pMciDriver;

//
// external functions
//
extern PMCI_DRIVER  MciDrvCreateInstance(void);
extern BOOL         MciDrvInitialize(PMCI_DRIVER pDrvCtx, UInt32 uiContext, pVoid lpBusContext);
extern Void         MciDrvRemoveInstance(PMCI_DRIVER pDrvCtx);
extern BOOL         MciDrvOpen(PMCI_DRIVER pDrvCtx, UInt32 uiAccess, UInt32 uiShare);
extern BOOL         MciDrvClose(PMCI_DRIVER pDrvCtx);
extern UInt32       MciDrvReadStream(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiBufLen);
extern UInt32       MciDrvReadMessage(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiBufLen);
extern UInt32       MciDrvSendMessage(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiBufLen);
extern BOOL         MciDrvIoControl(PMCI_DRIVER pDrvCtx, ULong uiCode, pUChar pIn, UInt32 uInLen, pUChar pOut, UInt32 uiOutLen, pULong puiBytesWritten);//pUInt32 puiBytesWritten);
extern Void         MciDrvPowerUp(PMCI_DRIVER pDrvCtx);
extern Void         MciDrvPowerDown(PMCI_DRIVER pDrvCtx);
extern UInt32       MciDrvSeek(PMCI_DRIVER pDrvCtx, long lDistance, UInt32 uiuiMoveMethod);

#ifdef __cplusplus
}
#endif

#endif
