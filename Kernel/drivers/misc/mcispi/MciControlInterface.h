/* drivers/misc/mcispi/MciControlInterface.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciControlInterface.h
**
**  Description
**      Declare the interface method and types
**
**  Author:
**      Seunghan Kim (hyenakim@samsung.com)
**
 *  Version History
 *      0.1				20080716	Create by shkim, for VisualOn
 *		0.2				20080831	Changed for ISDB-T. Modify some SPI parameters releated with bandwith and
 *									Add query API for Channel Status/Information
 *		0.3				20080918	Add new inferface for setting Configuration
 *		1.0.0			20081027	Modified to support both DVB-T and ISDB-T
 *		1.0.1			20081118	Version of ISDB-T MIS is updated to v0.38
 *		1.1.0			20081128	Version of DVB-T MIS is updated to v0.8
 *		1.2.0			20090325	Support T-DMB APIs
 *		2.0.0			20090430	Support DVB-H APIs
 *      2.1.0           20090824    Support ATSC-M/H APIs (DVB-T, DVB-H, ISDB-T, T-DMB, ATSC-M/H)
 *      3.0.0           20091121    Support CMMB APIs (DVB-T, DVB-H, ISDB-T, T-DMB, CMMB, ATSC-M/H)
 *      3.1.0           20100114    Modify CMMB APIs
 *      4.0.0           20100218    Rename some API function to keep the naming rule.
 *
 */

#ifndef _MCI_CONTROL_INTERFACE_H_
#define _MCI_CONTROL_INTERFACE_H_

#include "MciCommon.h"

/*
 * Define Constants
 */
#define MCI_CONTROL_INTF_MAJOR_VERSION          4
#define MCI_CONTROL_INTF_MINOR_VERSION          0
#define MCI_CONTROL_INTF_RELEASE_VERSION        0

#define MCI_MAX_PID_COUNT                       32
#define MCI_MAX_MFID_COUNT                      40  //for CMMB Multi-Frame

#define MCI_CONTROL_DLL_NAME                    _T("MciControl.dll")
#define MCI_CONTROL_LIB_NAME                    _T("libMciControl.so.0.0.0")
#define MCI_CONTROL_CREATE_FUNC_NAME            _T("MciControlCreateInstance")
#define MCI_CONTROL_GET_INTERFACE_FUNC_NAME     _T("MciControlGetInterfaceFunctions")

/*Set MciControl configuration file(MciConfig.ini) path*/
#define MCI_CONTROL_CONFIGURATION_FILE_PATH     _T("/data/mtvdata/MciConfig.ini")

/*
 * Function type declaration
 */
typedef INT32   (*MCI_CONTROL_CREATE_FUNC) (PVOID* ppHandle, UINT32 uiMtvType, UINT32 uiMajorVersion, UINT32 uiMinorVersion);
typedef INT32   (*MCI_CONTROL_GET_INTERFACE_FUNC)(PUCHAR pFuncTable, UINT32 uiBufSize);

/* 
 * Interface function type declaration
 */
typedef UINT32  (*MCI_CONTROL_GET_VERSION_FUNC) (PVOID pHandle);
typedef INT32   (*MCI_CONTROL_REINIT_MCI_CONTROL_FUNC) (PVOID pHandle, UINT32 uiMtvType, UINT32 uiMajorVersion, UINT32 uiMinorVersion);
typedef INT32   (*MCI_CONTROL_REGISTER_CALLBACK_FUNC)(PVOID pHandle, EMCI_CB_FUNC_ID funcID, PVOID fnCallback, PVOID pUserContext);
typedef INT32   (*MCI_CONTROL_SET_CONFIG_FUNC)(PVOID pHandle, EMCI_CONFIG_ID cfgId, PUCHAR pBuf, UINT32 uiBufLen);
typedef INT32   (*MCI_CONTROL_GET_CONFIG_FUNC)(PVOID pHandle, EMCI_CONFIG_ID cfgId, PUCHAR pBuf, UINT32 uiBufLen, PUINT32 puiWritten);
typedef INT32   (*MCI_CONTROL_INITIALIZE_DRIVER_FUNC)(PVOID pHandle, UINT32 uiMciType, pTChar pLinkFwImagePath);
typedef INT32   (*MCI_CONTROL_DEINITIALIZE_DRIVER_FUNC)(PVOID pHandle);
typedef INT32   (*MCI_CONTROL_START_CHANNEL_FUNC)(PVOID pHandle, UINT32 uiFrequency, UINT32 uiBandwidth);
typedef INT32   (*MCI_CONTROL_STOP_CHANNEL_FUNC)(PVOID pHandle);
typedef INT32   (*MCI_CONTROL_ENABLE_TRANSFER_FUNC)(PVOID pHandle, UINT32 uiFrequency);
typedef INT32   (*MCI_CONTROL_DISABLE_TRANSFER_FUNC)(PVOID pHandle, UINT32 uiFrequency);
typedef INT32   (*MCI_CONTROL_CREATE_TS_FILTER_FUNC)(PVOID pHandle, PUINT16 pPIDList, UINT32 uiPIDCount);
typedef INT32   (*MCI_CONTROL_DELETE_TS_FILTER_FUNC)(PVOID pHandle, PUINT16 pPIDList, UINT32 uiPIDCount);
typedef INT32   (*MCI_CONTROL_START_SIGNAL_SCAN_FUNC)(PVOID pHandle, UINT32 uiStartFrequency, UINT32 uiEndFrequency, UINT32 uiBandwidth);
typedef INT32   (*MCI_CONTROL_STOP_SIGNAL_SCAN_FUNC)(PVOID pHandle);
typedef INT32   (*MCI_CONTROL_GET_LOCKED_CHANNEL_LIST_FUNC)(PVOID pHandle, PUINT32 pChannelList, PUINT32 puiLockedChannelCount);
typedef INT32   (*MCI_CONTROL_ENABLE_STATUS_REPORT_FUNC)(PVOID pHandle, UINT32 uiPeriod);
typedef INT32   (*MCI_CONTROL_DISABLE_STATUS_REPORT_FUNC)(PVOID pHandle);
typedef INT32   (*MCI_CONTROL_GET_STRENGTH_FUNC)(PVOID pHandle, PUINT32 puiValue);
typedef INT32   (*MCI_CONTROL_GET_QUALITY_FUNC)(PVOID pHandle, PUINT32 puiValue);
typedef INT32   (*MCI_CONTROL_LINK_SETUP_FUNC)(PVOID pHandle, BOOL bNullPacketDiscard, UINT32 uiTsPacketCount);
typedef INT32   (*MCI_CONTROL_TUNE_CHANNEL_FUNC)(PVOID pHandle, UINT32 uiFrequency, UINT32 uiBandwidth);
typedef INT32   (*MCI_CONTROL_DESTROY_INSTANCE_FUNC)(PVOID pHandle);
typedef INT32   (*MCI_CONTROL_TEST_QUERY_FUNC)(PVOID pHandle, EMCI_TESTID testID, PUCHAR pBuf, UINT32 uiBufLen);
// 20090103: Added for reading stream buffer synchronously
typedef INT32   (*MCI_CONTROL_GET_STREAM_FUNC)(PVOID pContext, PUINT puiStreamType, PUCHAR* ppBuffer, PUINT32 puiBufSize);
typedef INT32   (*MCI_CONTROL_GET_STREAM_WITH_BUFFER_FUNC)(PVOID pContext, PUINT puiStreamType, PUCHAR pBuffer, UINT32 uiBufLen, PUINT32 puiActualSize);

// 20081228: Added for T-DMB operation
typedef INT32   (*MCI_TDMB_CREATE_FIC_FILTER_FUNC)(PVOID pHandleU, UINT32 uiFrequency);
typedef INT32   (*MCI_TDMB_CREATE_STATUS_FILTER_FUNC)(PVOID pHandle, UINT32 uiFrequency);
typedef INT32   (*MCI_TDMB_CREATE_MSC_FILTER_FUNC)(PVOID pHandle, UINT32 uiFrequency, UINT32 Sid, UCHAR SCIdS, UCHAR SubChId, UCHAR DataFormat);
typedef INT32   (*MCI_TDMB_DELETE_FIC_FILTER_FUNC)(PVOID pHandle, UINT32 uiFrequency);
typedef INT32   (*MCI_TDMB_DELETE_STATUS_FILTER_FUNC)(PVOID pHandle, UINT32 uiFrequency);
typedef INT32   (*MCI_TDMB_DELETE_MSC_FILTER_FUNC)(PVOID pHandle, UINT32 uiFrequency, UINT32 Sid, UCHAR SCIdS, UCHAR SubChId, UCHAR DataFormat);
// 20081228: Added for T-DMB operation
typedef INT32   (*MCI_TDMB_TRANSFER_FIC_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32   (*MCI_TDMB_TRANSFER_MSC_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32   (*MCI_TDMB_TRANSFER_STATUS_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
////android
typedef INT32   (*MCI_TDMB_RECEIVE_SCAN_PROGRESS)(PVOID pContext, UInt32 uiFrequency,UInt32 uiSId, UInt8 uiSCIds,UInt8 uiSubChId);


/*
 * Callback function type declaration
 */
typedef INT32 (*MCI_CONTROL_NOTIFY_STATE_FUNC)(PVOID pContext, UINT32 uiState, UINT32 uiParam);
typedef INT32 (*MCI_CONTROL_RECEIVE_STREAM_FUNC)(PVOID pContext, UINT32 uiStreamType, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_CONTROL_RECEIVE_SIGNAL_STATUS_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_CONTROL_RECEIVE_SCAN_PROGRESS_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_CONTROL_RECEIVE_TEST_QUERY_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef VOID  (*MCI_CONTROL_MESSAGE_PRINT_FUNC)(PVOID pContext, PTCHAR pFormat, ...);
typedef INT32 (*MCI_CONTROL_RECEIVE_DVBH_HANDOVER_ATCTIVITY_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_CONTROL_RECEIVE_IP_PACKET_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_CONTROL_RECEIVE_PLATFORM_STATUS)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);


// 20090420: Added for DVB-H Operation
typedef INT32 (*MCI_DVBH_SET_PLATFORM_FUNC)(PVOID pContext, PMCI_DVBH_PLATFORM_INFO pPlatformLists, UInt32 uiPlatformCount, UInt32 uiAction, Bool bSynchronous);
typedef INT32 (*MCI_DVBH_CREATE_FILTER_FUNC)(PVOID pContext, UInt32 uiPlatformID, UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum, MCI_CONTROL_RECEIVE_IP_PACKET_FUNC fnCallback, pVoid pCallbackContext);
typedef INT32 (*MCI_DVBH_DELETE_FILTER_FUNC)(PVOID pContext, UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum);
typedef INT32 (*MCI_DVBH_GET_NETWORK_CLOCK_FUNC)(PVOID pContext, UInt32 uiAction, PMCI_DVBH_NETWORK_CLOCK pClock);
typedef INT32 (*MCI_DVBH_RESET_FUNC)(PVOID pContext);
typedef INT32 (*MCI_DVBH_GSM_BAND_FUNC)(PVOID pContext, UInt32 uiGSMBand);
/*
* Callback fucntion type declaration for ATSC-M/H
*/ 
typedef INT32 (*MCI_ATSC_MH_RECEIVE_FIC_CHUNK_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_ATSC_MH_RECEIVE_SQ_INFO_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_ATSC_MH_RECEIVE_TPC_INFO_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);
typedef INT32 (*MCI_ATSC_MH_RECEIVE_IP_PACKET_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);

// 20090813: Added for ATSC-M/H Operation
typedef INT32 (*MCI_ATSC_START_CHANNEL_FUNC)(PVOID pContext, UINT32 uiFrequency);
typedef INT32 (*MCI_ATSC_STOP_CHANNEL_FUNC)(PVOID pContext);
typedef INT32 (*MCI_ATSC_GET_FIC_DATA_FUNC)(PVOID pContext, PUCHAR pBuf, PUINT32 puiBufLen);
typedef INT32 (*MCI_ATSC_START_ONE_FREQUENCY_SCAN_FUNC)(PVOID pContext, UINT32 uiFrequency);
typedef INT32 (*MCI_ATSC_STOP_CHANNEL_SCAN_FUNC)(PVOID pContext);
typedef INT32 (*MCI_ATSC_CREATE_IP_FILTER_FUNC)(PVOID pContext, UInt32 uiEnsembleID, UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum, MCI_ATSC_MH_RECEIVE_IP_PACKET_FUNC fnCallback, pVoid pCallbackContext);
typedef INT32 (*MCI_ATSC_DELETE_IP_FILTER_FUNC)(PVOID pContext, UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum);
typedef INT32 (*MCI_ATSC_DELETE_ALL_IP_FILTERS_FUNC)(PVOID pContext);
typedef INT32 (*MCI_ATSC_GET_SIGNAL_QUALITY_FUNC)(PVOID pContext, PUINT32 puiSignalQuality);
typedef INT32 (*MCI_ATSC_GET_TPC_INFO_FUNC)(PVOID pContext);

    // 20091221: Added for CMMB MIS Rev 0.7
typedef INT32 (*MCI_CMMB_TUNE_FUNC)(PVOID pContext, UInt32 uiFrequency, PMCI_CMMB_TUNE_MODE pTuneMode);
typedef INT32 (*MCI_CMMB_START_SCAN_FUNC)(PVOID pContext, UInt32 uiStartFrequency, UInt32 uiEndFrequency, PMCI_CMMB_TUNE_MODE pTuneMode);
typedef INT32 (*MCI_CMMB_STOP_SCAN_FUNC)(PVOID pContext);
typedef INT32 (*MCI_CMMB_CREATE_FILTER_FUNC)(PVOID pContext, UInt8 Mfid, PMCI_CMMB_FILTER_PARAM pFilterParam);
typedef INT32 (*MCI_CMMB_DELETE_FILTER_FUNC)(PVOID pContext, UInt8 Mfid);
typedef INT32 (*MCI_CMMB_CA_ACTIVATE_FUNC)(PVOID pContext, UInt8 Mfid, UInt8 SubFrameNum, UInt16 usServiceId, PMCI_CMMB_CA_INFO pCaInfo);
typedef INT32 (*MCI_CMMB_CA_DEACTIVATE_FUNC)(PVOID pContext, UInt16 usServiceId);
typedef INT32 (*MCI_CMMB_UAM_MESSAGE_FUNC)(PVOID pContext, pUChar pUamRequestMsg, UInt32 uiMsgLen);
typedef INT32 (*MCI_CMMB_SET_MUX_CONFIG_FUNC)(PVOID pContext, PMCI_CMMB_MUX_CONFIG pMuxConfig);

/*
* Callback fucntion type declaration for CMMB
*/ 
typedef INT32 (*MCI_CMMB_RECEIVE_UAM_REPLY_FUNC)(PVOID pContext, PUCHAR pBuffer, UINT32 uiBufSize);

/*
// 20090616: Added for CMMB Operation
typedef INT32 (*MCI_CONTROL_CREATE_CMMB_FILTER_FUNC)(PVOID pContext, UCHAR FilterCount, UCHAR Mfid, UCHAR RSCodeRate, UCHAR ModeOfByteInterleaving, UCHAR LDPCCodeRate, UCHAR ModeOfModulation, UCHAR ModeOfScrambling, UCHAR NumOfTimeSlots,  PUINT8 pTimeSlotNumber);
typedef INT32 (*MCI_CONTROL_DELETE_CMMB_FILTER_FUNC)(PVOID pContext, UCHAR FilterCount, UCHAR Mfid);
*/

typedef struct _MCI_INTF_FUNC_TABLE
{
        MCI_CONTROL_GET_VERSION_FUNC                    fnMciControlGetMajorVersion;
        MCI_CONTROL_GET_VERSION_FUNC                    fnMciControlGetMinorVersion;
        MCI_CONTROL_GET_VERSION_FUNC                    fnMciControlGetReleaseVersion;
        MCI_CONTROL_REINIT_MCI_CONTROL_FUNC             fnMciControlReinitializeMciControl;
        MCI_CONTROL_REGISTER_CALLBACK_FUNC              fnMciControlRegisterCallback;
        MCI_CONTROL_SET_CONFIG_FUNC                     fnMciControlSetConfig;
        MCI_CONTROL_GET_CONFIG_FUNC                     fnMciControlGetConfig;
        MCI_CONTROL_INITIALIZE_DRIVER_FUNC              fnMciControlInitializeDriver;
        MCI_CONTROL_DEINITIALIZE_DRIVER_FUNC            fnMciControlDeinitializeDriver;
        MCI_CONTROL_START_CHANNEL_FUNC                  fnMciControlStartChannel;
        MCI_CONTROL_STOP_CHANNEL_FUNC                   fnMciControlStopChannel;
        MCI_CONTROL_ENABLE_TRANSFER_FUNC                fnMciControlEnableTransfer;
        MCI_CONTROL_DISABLE_TRANSFER_FUNC               fnMciControlDisableTransfer;
        MCI_CONTROL_CREATE_TS_FILTER_FUNC               fnMciControlCreateTsFilter;
        MCI_CONTROL_DELETE_TS_FILTER_FUNC               fnMciControlDeleteTsFilter;
        MCI_CONTROL_START_SIGNAL_SCAN_FUNC              fnMciControlStartSignalScan;
        MCI_CONTROL_STOP_SIGNAL_SCAN_FUNC               fnMciControlStopSignalScan;
        MCI_CONTROL_GET_LOCKED_CHANNEL_LIST_FUNC        fnMciControlGetLockedChannelList;
        MCI_CONTROL_ENABLE_STATUS_REPORT_FUNC           fnMciControlEnableStatusReport;
        MCI_CONTROL_DISABLE_STATUS_REPORT_FUNC          fnMciControlDisableStatusReport;
        MCI_CONTROL_GET_STRENGTH_FUNC                   fnMciControlGetStrength;        
        MCI_CONTROL_GET_QUALITY_FUNC                    fnMciControlGetQuality;
        MCI_CONTROL_LINK_SETUP_FUNC                     fnMciControlLinkSetup;
        MCI_CONTROL_TUNE_CHANNEL_FUNC                   fnMciControlTuneChannel;
        MCI_CONTROL_DESTROY_INSTANCE_FUNC               fnMciControlDestroyInstance;
        MCI_CONTROL_TEST_QUERY_FUNC                     fnMciControlTestQuery;
        MCI_CONTROL_GET_STREAM_FUNC                     fnMciControlGetStream;
		MCI_CONTROL_GET_STREAM_WITH_BUFFER_FUNC         fnMciControlGetStreamWithBuffer;
        
        MCI_TDMB_CREATE_FIC_FILTER_FUNC                 fnMciTdmbCreateFicFilter;
        MCI_TDMB_CREATE_STATUS_FILTER_FUNC              fnMciTdmbCreateStatusFilter;
        MCI_TDMB_CREATE_MSC_FILTER_FUNC                 fnMciTdmbCreateMscFilter;
        MCI_TDMB_DELETE_FIC_FILTER_FUNC                 fnMciTdmbDeleteFicFilter;
        MCI_TDMB_DELETE_STATUS_FILTER_FUNC              fnMciTdmbDeleteStatusFilter;
        MCI_TDMB_DELETE_MSC_FILTER_FUNC                 fnMciTdmbDeleteMscFilter;
		MCI_DVBH_SET_PLATFORM_FUNC				        fnMciDvbhSetPlatform;
		MCI_DVBH_CREATE_FILTER_FUNC				        fnMciDvbhCreateFilter;
		MCI_DVBH_DELETE_FILTER_FUNC				        fnMciDvbhDeleteFilter;
		MCI_DVBH_GET_NETWORK_CLOCK_FUNC			        fnMciDvbhGetNetworkClock;
		MCI_DVBH_RESET_FUNC						        fnMciDvbhReset;
		MCI_DVBH_GSM_BAND_FUNC					        fnMciDvbhSetGsmBand;
        
        MCI_ATSC_START_CHANNEL_FUNC                     fnMciAtscStartChannel;
        MCI_ATSC_STOP_CHANNEL_FUNC                      fnMciAtscStopChannel;
        MCI_ATSC_START_ONE_FREQUENCY_SCAN_FUNC          fnMciAtscStartOneFrequencyScan;
        MCI_ATSC_STOP_CHANNEL_SCAN_FUNC                 fnMciAtscStopFrequencyScan;
        MCI_ATSC_GET_FIC_DATA_FUNC                      fnMciAtscGetFicData;
        MCI_ATSC_CREATE_IP_FILTER_FUNC                  fnMciAtscCreateIpFilter;
        MCI_ATSC_DELETE_IP_FILTER_FUNC                  fnMciAtscDeleteIpFilter;
        MCI_ATSC_DELETE_ALL_IP_FILTERS_FUNC             fnMciAtscDeleteAllIpFilters;
        MCI_ATSC_GET_SIGNAL_QUALITY_FUNC                fnMciAtscGetSignalQuality;
        MCI_ATSC_GET_TPC_INFO_FUNC                      fnMciAtscGetTpcInfo;

		MCI_CMMB_TUNE_FUNC						        fnMciCmmbTune;
        MCI_CMMB_START_SCAN_FUNC                        fnMciCmmbStartScan;
        MCI_CMMB_STOP_SCAN_FUNC                         fnMciCmmbStopScan;
        MCI_CMMB_CREATE_FILTER_FUNC                     fnMciCmmbCreateFilter;
        MCI_CMMB_DELETE_FILTER_FUNC                     fnMciCmmbDeleteFilter;
        MCI_CMMB_CA_ACTIVATE_FUNC                       fnMciCmmbCaActivate;
        MCI_CMMB_CA_DEACTIVATE_FUNC                     fnMciCmmbCaDeactivate;
        MCI_CMMB_UAM_MESSAGE_FUNC                       fnMciCmmbUamMessage;
        MCI_CMMB_SET_MUX_CONFIG_FUNC                    fnMciCmmbSetMuxConfig;

} MCI_INTF_FUNC_TABLE, *PMCI_INTF_FUNC_TABLE;


/*
 * External function declaration
 */
extern "C" INT32  MciControlCreateInstance(PVOID* ppHandle, UINT32 uiMtvType, UINT32 uiMajorVersion, UINT32 uiMinorVersion);
extern "C" INT32  MciControlGetInterfaceFunctions(PMCI_INTF_FUNC_TABLE pFuncTable);

/*
 * Define MciControl Module Interface classes
 */
#ifdef __cplusplus
class IMciControl
{
public:
        virtual UINT32          MciControlGetMajorVersion() = 0;
        virtual UINT32          MciControlGetMinorVersion() = 0;
        virtual UINT32          MciControlGetReleaseVersion() = 0;
        virtual INT32           MciControlReinitializeMciControl( UINT32 uiMtvType, UINT32 uiMajorVeraion, UINT32 uiMinorVersion ) = 0;
        virtual INT32           MciControlRegisterCallback(EMCI_CB_FUNC_ID funcID, PVOID fnCallback, PVOID pUserContext) = 0;
        virtual INT32           MciControlSetConfig(EMCI_CONFIG_ID cfgId, PUCHAR pBuf, UINT32 uiBufLen) = 0;
        virtual INT32           MciControlGetConfig(EMCI_CONFIG_ID cfgId, PUCHAR pBuf, UINT32 uiBufLen, PUINT32 puiWritten = NULL) = 0;
        virtual INT32           MciControlInitializeDriver(UINT32 uiMciType, pTChar pLinkFwImagePath) = 0;
        virtual INT32           MciControlDeinitializeDriver() = 0;
        virtual INT32           MciControlDestroyInstance() = 0;

        virtual INT32           MciControlStartChannel(UINT32 uiFrequency, UINT32 uiBandwidth) = 0;
        virtual INT32           MciControlStopChannel() = 0;
        virtual INT32           MciControlEnableTransfer(UINT32 uiFrequency) = 0;
        virtual INT32           MciControlDisableTransfer(UINT32 uiFrequency) = 0;
        virtual INT32           MciControlCreateTsFilter(PUINT16 pPIDList, UINT32 uiPIDCount) = 0;
        virtual INT32           MciControlDeleteTsFilter(PUINT16 pPIDList, UINT32 uiPIDCount) = 0;
        virtual INT32           MciControlStartSignalScan(UINT32 uiStartFrequency, UINT32 uiEndFrequency, UINT32 uiBandwidth = 0) = 0;
        virtual INT32           MciControlStopSignalScan() = 0;
        virtual INT32           MciControlGetLockedChannelList(PUINT32 pChannelList, PUINT32 puiLockedChannelCount) = 0;
        virtual INT32           MciControlEnableStatusReport(UINT32 uiPeriod) = 0;
        virtual INT32           MciControlDisableStatusReport() = 0;
        virtual INT32           MciControlGetStrength(PUINT32 puiValue) = 0;
        virtual INT32           MciControlGetQuality(PUINT32 puiValue) = 0;
        virtual INT32           MciControlLinkSetup(BOOL bNullPacketDiscard, UINT32 uiTsPacketCount) = 0;
        virtual INT32           MciControlTuneChannel(UINT32 uiFrequency, UINT32 uiBandwidth) = 0;
        virtual INT32           MciControlTestQuery(EMCI_TESTID testID, PUCHAR pBuf, UINT32 uiBufLen) = 0;
        // 20090103: Added for reading stream buffer synchronously
        virtual INT32           MciControlGetStream(PUINT puiStreamType, PUCHAR* ppBuf, PUINT32 puiBufLen) = 0;
        virtual INT32           MciControlGetStreamWithBuffer(PUINT puiStreamType, PUCHAR pBuffer, UINT32 uiBufLen, PUINT32 puiActualLen) = 0;

        // 20081228: Added for T-DMB
        virtual INT32           MciTdmbCreateFicFilter(UINT32 uiFrequency) = 0;
        virtual INT32           MciTdmbDeleteFicFilter(UINT32 uiFrequency) = 0;
        virtual INT32           MciTdmbCreateStatusFilter(UINT32 uiFrequency) = 0;
        virtual INT32           MciTdmbDeleteStatusFilter(UINT32 uiFrequency) = 0;
        virtual INT32           MciTdmbCreateMscFilter(UINT32 uiFrequency, UINT32 SId, UCHAR SCIdS, UCHAR SubChId, UCHAR DataFormat) = 0;
        virtual INT32           MciTdmbDeleteMscFilter(UINT32 uiFrequency, UINT32 SId, UCHAR SCIdS, UCHAR SubChId, UCHAR DataFormat) = 0;
		// 20090420
		virtual INT32			MciDvbhSetPlatform(PMCI_DVBH_PLATFORM_INFO pPlatformLists, UInt32 uiPlatformCount, UInt32 uiAction = 0, Bool bSynchronous = FALSE) = 0;
		virtual INT32			MciDvbhCreateFilter(UInt32 uiPlatformID, UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum, MCI_CONTROL_RECEIVE_IP_PACKET_FUNC fnCallback, pVoid pCallbackContext) = 0;
		virtual INT32			MciDvbhDeleteFilter(UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum) = 0;
		virtual INT32			MciDvbhGetNetworkClock(UInt32 uiAction, PMCI_DVBH_NETWORK_CLOCK pClock = NULL) = 0;
		virtual INT32			MciDvbhReset() = 0;
		virtual INT32			MciDvbhSetGsmBand(UInt32 uiGSMBand) = 0;

        // 20090815: Added for ATSC-M/H
        virtual INT32           MciAtscStartChannel(UINT32 uiFrequency) = 0;
        virtual INT32           MciAtscStopChannel() = 0;
        virtual INT32           MciAtscStartOneFrequencyScan(UINT32 uiFrequency) = 0;
        virtual INT32           MciAtscStopFrequencyScan() = 0;
        virtual INT32           MciAtscGetFicData(pUChar pBuf, pUInt32 puiBufLen) = 0;
        virtual INT32           MciAtscCreateIpFilter(UInt32 uiEnsembleID, UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum, MCI_ATSC_MH_RECEIVE_IP_PACKET_FUNC fnCallback, pVoid pCallbackContext) = 0;
        virtual INT32           MciAtscDeleteIpFilter(UInt32 uiIpVersion, pUChar pIpAddress, UInt32 uiPortNum) = 0;
        virtual INT32           MciAtscDeleteAllIpFilters() = 0;
        virtual INT32           MciAtscGetSignalQuality(PUINT32 puiSignalQuality)= 0;
        virtual INT32           MciAtscGetTpcInfo() = 0;

            // 20091221: Added for CMMB MIS Rev 0.7
        virtual INT32           MciCmmbTune(UInt32 uiFrequency, PMCI_CMMB_TUNE_MODE pTuneMode) = 0;
        virtual INT32           MciCmmbStartScan(UInt32 uiStartFrequency, UInt32 uiEndFrequency, PMCI_CMMB_TUNE_MODE pTuneMode) = 0;
        virtual INT32           MciCmmbStopScan() = 0;
        virtual INT32           MciCmmbCreateFilter(UInt8 Mfid, PMCI_CMMB_FILTER_PARAM pFilterParam) = 0;
        virtual INT32           MciCmmbDeleteFilter(UInt8 Mfid) = 0;
        virtual INT32           MciCmmbCaActivate(UInt8 Mfid, UInt8 SubFrameNum, UInt16 usServiceId, PMCI_CMMB_CA_INFO pCaInfo) = 0;
        virtual INT32           MciCmmbCaDeactivate(UInt16 usServiceId) = 0;
        virtual INT32           MciCmmbUamMessage(pUChar pUamRequestMsg, UInt32 uiMsgLen) = 0;
        virtual INT32           MciCmmbSetMuxConfig(PMCI_CMMB_MUX_CONFIG pMuxConfig) = 0;

};
#endif //__cplusplus


#endif // _MCI_CONTROL_INTERFACE_H_
