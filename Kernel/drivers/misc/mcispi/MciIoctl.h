/* linux/driver/misc/mcispi/MciIoctl.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciIoctl.h
**
**  Description
**      Define I/O Control Code
**
**  Authur:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.20  NEW     Created.
**      2008.12.15  Modify  Add MCI_CARD_INFO structure for comunicating with Network Driver in WindowCE
**/

#ifndef _MCI_IOCTL_H_
#define _MCI_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MCI_DRIVER_INTF_MAJOR_VERSION           1
#define MCI_DRIVER_INTF_MINOR_VERSION           1
#define MCI_DRIVER_INTF_RELEASE_VERSION         1

/**
 *      Constance Definition for Mobile TV Solution
 */
#define MCI_EVENT_NAME_DRIVER_STATE_CHANGE      _T("OEM/MciDriverStateChangeEvent")
#define MCI_EVENT_NAME_MIS_MESSAGE_RECEIVE      _T("OEM/MciMisMessageReceiveEvent")
#define MCI_EVENT_NAME_TS_STREAM_RECEIVE        _T("OEM/MciTsStreamReceiveEvent")
#define MCI_EVENT_NAME_IP_STREAM_RECEIVE        _T("OEM/MciIpStreamReceiveEvent")

#define MCI_CTL_CODE( DeviceType, Function, Method, Access) (                   \
        ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method)  \
        )
#define MCI_IOCTL_SERIAL_PORT					0x0000001b
#define MCI_METHOD_BUFFERED                     0
#define MCI_IOCTL_WRITE							0x0002
#define MCI_IOCTL_READ							0x0001
#define MCI_IOCTL_ANY							0

#define OID_MCI_SET_CARD_INFO                   0xFF000000
#define OID_MCI_RECEIVE_BUF_DESC                0xFF000001
#define OID_MCI_PLATFORM_ADDRESS                0xFF000002

// IOCTL Commands
#define MCI_IOCTL_SEND_MESSAGE                  MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 0, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_GET_MESSAGE                   MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 1, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_GET_IP_STREAM                 MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 2, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_GET_IP_STREAM_DESC            MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 3, MCI_METHOD_BUFFERED, MCI_IOCTL_ANY)
#define MCI_IOCTL_GET_TS_STREAM                 MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 4, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_GET_DRIVER_STATE              MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 5, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_SET_DRIVER_CONFIG             MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 6, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_SET_STREAM_BUFFER_SIZE        MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 7, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_CLEAR_STREAM_BUFFER           MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 8, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_CLOSE_MCI_INTERFACE           MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT, 9, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_FW_DOWNLOAD                   MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,10, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_SW_RESET_CHANNEL              MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,11, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_HW_RESET_CHANNEL              MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,12, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_MCI_DEBUG_CMD                 MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,13, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_GET_DRIVER_STATE_CHANGE_INFO  MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,14, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_ENABLE_NETWORK_DRIVER			MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,15, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)
#define MCI_IOCTL_CHECK_CARD_INSERT		        MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,16, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_GET_DRIVER_INFO               MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,50, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_DO_SELFTEST		            MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,51, MCI_METHOD_BUFFERED, MCI_IOCTL_READ)
#define MCI_IOCTL_FW_RECOVERY		            MCI_CTL_CODE(MCI_IOCTL_SERIAL_PORT,52, MCI_METHOD_BUFFERED, MCI_IOCTL_WRITE)



typedef struct _MCI_DRIVER_CONFIG
{
    UInt32      uiMtvType;
    UInt32      uiInterruptHandlerThreadPriority;
    UInt32      uiBufferRetryCount;
    UInt32      uiMaxStreamBufferSize;
    UInt32      uiMaxMessageBufferSize;
    UInt32      uiMessageBufferCount;
    BOOL        bEnableTsHwInterface;
    UInt32      uiMsgFreqScanType;
    UInt32      uiMsgCreateFilterType;
    UInt32      uiMsgDeleteFilterType;
    UInt32      uiMsgStatusType;
    UInt32      uiMsgStreamDataType;
    UInt32      uiMsgBootSuccessType;
    BOOL        bNotifyAttachEvent;
    UInt32      uiSdioRxBaseAddress;
    UInt32      uiSdioTxBaseAddress;
    Bool        bSyncAccessForStream;
    Bool        bSyncAccessForMessage;
    Bool        bSyncAccessForState;
    UInt32      uiWaitTimeForStream;
    UInt32      uiWaitTimeForMessage;
    UInt32      uiWaitTimeForStateChange;
    UInt32      bSdioEnableMultiBlockTransfer;
} MCI_DRIVER_CONFIG, *PMCI_DRIVER_CONFIG;

typedef struct _MCI_DRIVER_INFO
{
    UInt32      uiMciType;
    UInt32      uiMajorVersion;
    UInt32      uiMinorVersion;
    UInt32      uiReleaseVersion;
    UInt32      uiIoctlMajorVersion;
    UInt32      uiIoctlMinorVersion;
    UInt32      uiIoctlReleaseVersion;
    TChar       ucCompiledDate[20];
    TChar       ucCompiledTime[20];
} MCI_DRIVER_INFO, *PMCI_DRIVER_INFO;

typedef struct _MCI_CARD_INFO
{
    UInt32      uiCardType;
    UInt32      uiDriverEventType;
    BOOL        bCheckAvSeqNumInDvbh;
} MCI_CARD_INFO, *PMCI_CARD_INFO, **PPMCI_CARD_INFO;

typedef enum _MCI_DRV_STATE
{
    EMCI_DRV_UNKNOWN,
    EMCI_DRV_INITIALIZED,
    EMCI_DRV_READY,
    EMCI_DRV_SHUTDOWN_IN_PROGRESS
} MCI_DRV_STATE;

typedef struct _MCI_FWIMG_BUF_INFO
{
    UInt32      uiTotalLength;
    UInt32      uiAddrOffset;
    UInt32      uiFragmentIndex;
    UInt32      uiTotalFragmentCount;
    UInt32      uiBufSize;
    UChar       ucBuffer[1];
} MCI_FWIMG_BUF_INFO, *PMCI_FWIMG_BUF_INFO, **PPMCI_FWIMG_BUF_INFO;


#ifdef __cplusplus
}
#endif

#endif // _MCI_IOCTL_H_
