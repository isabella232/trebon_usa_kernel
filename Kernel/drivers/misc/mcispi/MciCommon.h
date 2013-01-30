/* drivers/misc/mcispi/MciCommon.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciCommon.h
**
**  Description
**      Declare the common types and structures
**
**  Author:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.27  NEW     Created.
**/

#ifndef _MCI_COMMON_H_
#define _MCI_COMMON_H_

#include "MciTypeDefs.h"

#define	MIS_MSG_STX					    0x02
#define MIS_MSG_ETX					    0x03
#define MIS_MSG_HDR_LEN				    0x04

#define MCI_ETH_HDR_LEN                 14
#define MCI_IPV4_HDR_LEN                20
#define MCI_IPV6_HDR_LEN                40
#define MCI_MSG_START_POS_IN_DESC_BUF   10
#define MCI_MAX_ETHERNET_PACKET_SIZE	1500

//////////////////////////////////////////////////////////////////////////
// Mobile TV Type
typedef enum _MTV_TYPE_ENUM
{
        EMTV_TYPE_UNKNOWN,
        EMTV_TYPE_DVBH,
        EMTV_TYPE_ISDBT,
        EMTV_TYPE_TDMB,
        EMTV_TYPE_DVBT,
        EMTV_TYPE_ATSC_MH,
        EMTV_TYPE_CMMB,
} EMTV_TYPE;

//////////////////////////////////////////////////////////////////////////
// Link H/W Interface Type
//////////////////////////////////////////////////////////////////////////
typedef enum _MCI_TYPE_ENUM
{
        EMCI_TYPE_UNKNOWN,
        EMCI_TYPE_SPI,
        EMCI_TYPE_SDIO,
        EMCI_TYPE_MEM,
        EMCI_TYPE_USB,
        EMCI_TYPE_BT
} EMCI_TYPE;

//////////////////////////////////////////////////////////////////////////
//      ERROR Code
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_OK                                 = 0,
        EMCI_FAIL                               = -1,
		EMCI_VERSION_MISMATCH                   = -2,
		EMCI_CMD_REQUEST_ERROR                  = -3,
        EMCI_CMD_REQUEST_TIME_OUT               = -4,
        EMCI_OUT_OF_MEMORY                      = -5,
        EMCI_UNLOCKED_SIGNAL                    = -6,
        EMCI_NOT_INITIALIZED                    = -7,
        EMCI_INVALID_PARAMETER                  = -8,
        EMCI_NOT_IMPLEMENTED                    = -9,
        EMCI_MORE_BUFFER_NEEDED                 = -10,
        EMCI_INVALID_CONFIG_ID                  = -11,
        EMCI_INVALID_CALLBACK_FUNC_ID           = -12,
		EMCI_INVALID_REPLY_MESSAGE				= -13,
		EMCI_INVALID_FREQUENCY					= -14,
		EMCI_NOT_SUPPORTED						= -15,
		EMCI_CANNOT_CREATE_DRIVER_OBJECT		= -16,
		EMCI_CANNOT_OPEN_DEVICE_DRIVER		    = -17,
		EMCI_NOT_INITIALIZED_FOR_SYNCH_ACCESS   = -18,
		EMCI_DUPULICATED_IP_ADDRESS				= -19,
		EMCI_NOT_FOUND_FILTER                   = -20,
} EMCI_RETURN;

//////////////////////////////////////////////////////////////////////////
//      STATE CODE
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_EVENT_UNKNOWN                      = 0,
        EMCI_EVENT_DLL_LOADED                   = 1,
        EMCI_EVENT_CARD_ATTACHED                = 2,
        EMCI_EVENT_LINK_DRIVER_OPENED           = 3,
        EMCI_EVENT_BOOT_SUCCESS_RECEIVED        = 4,
        EMCI_EVENT_SIGNAL_SCANED                = 5,
        EMCI_EVENT_SIGNAL_SCAN_DONE             = 6,
        EMCI_EVENT_LINK_DRIVER_REMOVED          = 7,
		EMCI_EVENT_CARD_DETACHED				= 8,
		EMCI_EVENT_ATSCMH_FIC_UPDATED           = 9,
		EMCI_EVENT_INVALID_CHIP_STATE			= 20,
} EMCI_EVENT_TYPE;

typedef enum
{
	    EMCI_STATE_UNKNOWN	        = 0,
	    EMCI_STATE_DRIVER_ATTACHED	= EMCI_EVENT_LINK_DRIVER_OPENED,
	    EMCI_STATE_SIGNAL_SCAN_DONE = EMCI_EVENT_SIGNAL_SCAN_DONE,
	    EMCI_STATE_DRIVER_DETACHED	= EMCI_EVENT_CARD_DETACHED,
		EMCI_STATE_DVBH_SET_PLATFORM_DONE,
		EMCI_STATE_DVBH_GET_NETWORK_CLOCK_DONE,
		EMCI_STATE_ATSC_FIC_UPDATED,
	    EMCI_STATE_SHUTDOWN_IN_PROGRESS,
		EMCI_STATE_CONTROL_READY
} EMCI_STATE;

//////////////////////////////////////////////////////////////////////////
// Callback Function ID
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_CBID_NOTIFY_STATE = 0,
        EMCI_CBID_RECEIVE_TS_PACKET,
        EMCI_CBID_RECEIVE_TDMB_VIDEO_STREAM,
        EMCI_CBID_RECEIVE_TDMB_AUDIO_STREAM,
        EMCI_CBID_RECEIVE_TDMB_DATA_STREAM,
        EMCI_CBID_RECEIVE_TDMB_FIC_STREAM,
        EMCI_CBID_RECEIVE_CMMB_STREAM,
        EMCI_CBID_RECEIVE_SIGNAL_STATUS,
        EMCI_CBID_RECEIVE_SCAN_PROGRESS,
        EMCI_CBID_RECEIVE_TEST_QUERY,
        EMCI_CBID_MESSAGE_PRINT,
		EMCI_CBID_RECEIVE_IP_PACKET,
		EMCI_CBID_RECEIVE_PLATFORM_STATUS,
		EMCI_CBID_RECEIVE_TDMB_SCAN_PROGRESS,
		EMCI_CBID_RECEIVE_ATSC_FIC_CHUNK,
        EMCI_CBID_RECEIVE_CMMB_UAM_REPLY,
        EMCI_CBID_RECEIVE_ATSC_SQ_INFO,
        EMCI_CBID_RECEIVE_ATSC_TPC_INFO,
} EMCI_CB_FUNC_ID;

//////////////////////////////////////////////////////////////////////////
// Configuration ID
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_CFGID_TS_PACKET_COUNT = 0,
        EMCI_CFGID_TS_TIMEOUT,
        EMCI_CFGID_FREQUENCY_START,
        EMCI_CFGID_FREQUENCY_END,
        EMCI_CFGID_EWS_ENABLE,
        EMCI_CFGID_3SEG_ENABLE,
        EMCI_CFGID_TS_CNT_FOR_BER_MEASURE,
        EMCI_CFGID_TDMB_TUNER_BAND,
        EMCI_CFGID_TDMB_TS_FORMAT,
        EMCI_CFGID_TDMB_SCAN_OPTION_USE_FIC_DB,
		EMCI_CFGID_ENABLE_NETWORK_DRIVER,
} EMCI_CONFIG_ID;

//////////////////////////////////////////////////////////////////////////
// Frequency Bandwidth enumeration
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_FREQ_BW_8MHZ                       = 0,
        EMCI_FREQ_BW_7MHZ                       = 1,
        EMCI_FREQ_BW_6MHZ                       = 2,
        EMCI_FREQ_BW_5MHZ                       = 3,
        EMCI_FREQ_BW_2MHZ                       = 4,
} EMCI_FREQ_BW;


//////////////////////////////////////////////////////////////////////////
// Frequency Bandwidth enumeration
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_STREAM_TYPE_TS                     = 0,
        EMCI_STREAM_TYPE_TDMB_FIC               = 1,
        EMCI_STREAM_TYPE_TDMB_VIDEO             = 2,
        EMCI_STREAM_TYPE_TDMB_AUDIO             = 3,
        EMCI_STREAM_TYPE_TDMB_DATA              = 4,
        EMCI_STREAM_TYPE_CMMB_MF                = 5,
} EMCI_STREAM_TYPE;

//////////////////////////////////////////////////////////////////////////
// T-DMB Related enumeration
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_TDMB_TB_BANDIII_KOREA              = 0,
        EMCI_TDMB_TB_BANDIII_EUROPE             = 1,
        EMCI_TMDB_TB_LBAND_EUROPE               = 2,
		EMCI_TDMB_TB_MAX						= 3,
} EMCI_TDMB_TUNER_BAND;
typedef enum
{
        EMCI_TDMB_SCAN_OPT_USE_FIC_DB           = 1,
        EMCI_TDMB_SCAN_OPT_NO_USE_FIC_DB        = 2,
} EMCI_TDMB_SCAN_OPTION;
typedef enum
{
        EMCI_TDMB_BT_FIC                        = 1,
        EMCI_TDMB_BT_STATUS                     = 2,
        EMCI_TDMB_BT_MSC                        = 3,
} EMCI_TDMB_BLOCK_TYPE;
typedef enum
{
        EMCI_TDMB_TF_204                        = 1,
        EMCI_TDMB_TF_188                        = 2,
} EMCI_TDMB_TS_FORMAT;
typedef enum
{	
		EMCI_TDMB_FIC_MODE_UNKNOWN				= 0,
		EMCI_TDMB_FIC_MODE_I					= 1,
		EMCI_TDMB_FIC_MODE_II					= 2,
		EMCI_TDMB_FIC_MODE_III					= 3,
		EMCI_TDMB_FIC_MODE_IV					= 4,
} EMCI_TDMB_FIC_MODE;

typedef enum
{
		EMCI_DVBH_SCAN_ACTION_START				= 0,
		EMCI_DVBH_SCAN_ACTION_STOP				= 1,
		EMCI_DVBH_SCAN_ACTION_NIT_SCAN			= 2,
		EMCI_DVBH_SCAN_ACTION_INT_SCAN			= 3,
} EMCI_DVBH_SCAN_ACTION;

typedef enum
{
		EMCI_DVBH_GET_NC_ACTION_REQUEST			= 0,
		EMCI_DVBH_GET_NC_ACTION_CANCEL			= 1,
} EMCI_DVBH_GET_NETWORK_CLOCK_ACTION;

typedef enum
{
		EMCI_IP_VERSION_V4						= 2, /*AF_INET*/
		EMCI_IP_VERSION_V6						= 23, /*AF_INET6*/
} EMCI_IP_VERSION;

//////////////////////////////////////////////////////////////////////////
// Test Query Function ID
//////////////////////////////////////////////////////////////////////////
typedef enum
{
        EMCI_TESTID_SELF_TEST = 0,
        EMCI_TESTID_GPIO_TEST = 1,
        EMCI_TESTID_PWM_TEST  = 2,
        EMCI_TESTID_CW_TEST   = 3,
        EMCI_TESTID_THROUGHPUT_TEST = 255
} EMCI_TESTID;


//////////////////////////////////////////////////////////////////////////
// MIS Message Header
//////////////////////////////////////////////////////////////////////////
typedef struct _MIS_MSG_HDR
{
        UInt8       ucStx;          // Start Code
        UInt8       ucType;         // IPC message type
        UInt16      usLen;
        UInt8       ucData[1];
} MIS_MSG_HDR, *PMIS_MSG_HDR;



//////////////////////////////////////////////////////////////////////////
// Channel Status Structure
//////////////////////////////////////////////////////////////////////////
typedef struct _TISDBT_CHANNEL_STATUS
{
        UInt16  usCNR;                          // In 10^(-2) decibels, e.g. 0x07D0 (2000) is 20.00 dB
        UInt16  usBER;                          // Integer(8 bit) and negative power of 10(8 bit), e.g. 0x0307 is 7x10^-3
        UInt16  usPER;                          // See BER
        UInt8   ucRSSI;                         // (-)dBm unit
        UInt8   ucESR;                          // Erroneous Second Ratio 5%
} TISDBT_CHANNEL_STATUS, *PTISDBT_CHANNEL_STATUS;

typedef struct _TISDBT_CHANNEL_STATUS   TDVBT_CHANNEL_STATUS, *PTDVBT_CHANNEL_STATUS;

typedef struct _TTDMB_CHANNEL_STATUS
{
		UInt8	ucRSSI;
		UInt16	usVBER;
		UInt16	usBER;
		UInt16	usPER;
		UInt16	usESR;
		UInt16	usFFER;
		UInt16	usMMFER;
} TTDMB_CHANNEL_STATUS, *PTTDMB_CHANNEL_STATUS;

typedef struct _TDVBH_CHANNEL_STATUS
{
		UInt32	Frequency;
		UInt32	TimeSlicingInfo;
		UInt16	TransportStreamID;
		UInt16	OriginalNetworkID;
		UInt16	NetworkID;
		UInt16	CellID;
		UInt16  PER;
		UInt16  BER;
		UInt16	VBER;
		UInt8 	RSSI;
		UInt8	FER;
		UInt8	MFER;
		UInt8	SignalQuality;
		UInt16	SyncTime;
} TDVBH_CHANNEL_STATUS, *PTDVBH_CHANNEL_STATUS;

typedef struct _TCMMB_CHANNEL_STATUS
{
		UChar	ucMfId;
		UInt32  uiFrequency;
		UChar	ucRSSI;
		UInt32  uiLBER;
		UInt32  uiBER;
		UInt32  uiBLER;
		UInt16  usFER;
		UInt16  usMFER;
		UInt16  usSyncTime;
		UInt32  uiPER;
		UInt32  uiRSPER;
		UInt32  uiSBER;
} TCMMB_CHANNEL_STATUS, *PTCMMB_CHANNEL_STATUS;

typedef struct _TCHANNEL_STATUS
{
        UInt32  uiLength;
        union
        {
                TISDBT_CHANNEL_STATUS   IsdbtChannelStatus;
                TDVBT_CHANNEL_STATUS    DvbtChannelStatus;
				TDVBH_CHANNEL_STATUS	DvbhChannelStatus;
				TCMMB_CHANNEL_STATUS	CmmbChannelStatus;
				TTDMB_CHANNEL_STATUS	TdmbChannelStatus;
        } u;
} TCHANNEL_STATUS, *PTCHANNEL_STATUS;

//////////////////////////////////////////////////////////////////////////
// Channel Information Structures
//////////////////////////////////////////////////////////////////////////
typedef struct _TISDBT_CHANNEL_INFO
{
        UInt8   ucRSSI;                         // (-)dBm unit
        UInt8   ucLock;                         // 0x00: Not locked, 0x01: Locked
        UInt8   ucFFTMode;                      // 0x00: 2K, 0x01: 8K, 0x02: 4K
        UInt8   ucGuardInterval;                // 0x00: 1/32, 0x01: 1/16, 0x02: 1/8, 0x03: 1/4
        UInt8   ucConstellation;                // 0x00: QPSK, 0x01: 16 QAM, 0x02: 64 QAM
        UInt8   ucCodeRate;                     // 0x00: 1/2, 0x01: 2/3, 0x02: 3/4, 0x03: 5/6, 0x04: 7/8
        UInt8   ucPartialReception;             // 0x00: no partial reception, 0x01: partial ceception available
		UInt32  ucFrequency;
} TISDBT_CHANNEL_INFO, *PTISDBT_CHANNEL_INFO;

typedef struct _TISDBT_CHANNEL_INFO             TDVBT_CHANNEL_INFO, *PTDVBT_CHANNEL_INFO;

typedef struct _TCHANNEL_INFO
{
        UInt32  uiLength;
        union
        {
                TISDBT_CHANNEL_INFO             IsdbtChannelInfo;
                TDVBT_CHANNEL_INFO              DvbtChannelInfo;
        } u;
} TCHANNEL_INFO, *PTCHANNEL_INFO;

typedef struct _ETHERNET_HEADER 
{
	UInt16	wDestMAC[3];
	UInt16  wSrcMAC[3];
	UInt16  wFrameType;
} ETHERNET_HEADER, *PETHERNET_HEADER;

typedef struct _IPV4_HEADER
{
	UInt8	bVersionLength;
	UInt8	bTypeOfService;
	UInt16	wTotalLength;
	UInt16	wIdentification;
	UInt16	wFragment;
	UInt8	bTimeToLive;
	UInt8	bProtocol;
	UInt16	wCRC;
	UInt16	dwSrcIP1;
	UInt16	dwSrcIP2;
	UInt16	dwDestIP1;
	UInt16	dwDestIP2;
	// Options can go in here
	// Then comes the data
} IPV4_HEADER, *PIPV4_HEADER;

typedef struct _UDP_HEADER
{
	UInt16	wSrcPort;
	UInt16	wDestPort;
	UInt16  wTotalUDPLength;
	UInt16	wCRC;
} UDP_HEADER, *PUDP_HEADER;

typedef struct _UDP_PSEUDO_HEADER
{
	ULong	dwSrcIP;
	ULong	dwDestIP;
	UInt8	bZero;
	UInt8	bProtocol;
	UInt16  wTotalUDPLength;
} UDP_PSEUDO_HEADER, *PUDP_PSEUDO_HEADER;

typedef struct _MCI_DVBH_PLATFORM_INFO
{
	UInt32	uiPlatformID;
	UInt16	uiNetworkID;
	UInt32	uiEsgRootIpVersion;
	UChar	ucEsgRootIpAddress[16];
	UInt32  uiStatus;
} MCI_DVBH_PLATFORM_INFO, *PMCI_DVBH_PLATFORM_INFO, **PPMCI_DVBH_PLATFORM_INFO;

typedef struct _MCI_DVBH_NETWORK_CLOCK
{
	UChar	ucYear;
	UChar	ucMonth;
	UChar	ucDay;
	UChar	ucHour;
	UChar	ucMinute;
	UChar	ucSecond;
} MCI_DVBH_NETWORK_CLOCK, *PMCI_DVBH_NETWORK_CLOCK, **PPMCI_DVBH_NETWORK_CLOCK;

typedef enum 
{ 
        EMCI_FREQ_POINT                         = 0, 
        EMCI_FREQ_NUM                           = 1
} EMCI_CMMB_FREQ_UNIT;

typedef enum 
{ 
        EMCI_CMMB_BW_8MHZ                       = 0, 
        EMCI_CMMB_BW_2MHZ                       = 1
} EMCI_CMMB_BANDWIDTH;

typedef enum 
{ 
        EMCI_ONLY_CMMB_SIGNAL_DETECT            = 0, 
        EMCI_TS0_RECEPTION                      = 1
} EMCI_CMMB_TUNE_MODE;

typedef struct _MCI_CMMB_TUNE_MODE
{
    EMCI_CMMB_FREQ_UNIT     FreqUnit;
    EMCI_CMMB_BANDWIDTH	    BandWidth;
    EMCI_CMMB_TUNE_MODE     TuneMode;
} MCI_CMMB_TUNE_MODE, *PMCI_CMMB_TUNE_MODE, **PPMCI_CMMB_TUNE_MODE;

typedef enum 
{ 
        EMCI_CMMB_RS_240_240                    = 0, 
        EMCI_CMMB_RS_240_224                    = 1, 
        EMCI_CMMB_RS_240_192                    = 2, 
        EMCI_CMMB_RS_240_176                    = 3
} EMCI_CMMB_RS_CODE_RATE;

typedef enum 
{ 
        EMCI_CMMB_BI_MODE1                      = 1, 
        EMCI_CMMB_BI_MODE2                      = 2, 
        EMCI_CMMB_BI_MODE3                      = 3, 
} EMCI_CMMB_BYTE_INT;

typedef enum 
{ 
        EMCI_CMMB_LDPC_1_2                      = 0, 
        EMCI_CMMB_LDPC_3_4                      = 1
} EMCI_CMMB_LDPC_CODE_RATE;

typedef enum 
{ 
        EMCI_CMMB_MOD_BPSK                      = 0, 
        EMCI_CMMB_MOD_QPSK                      = 1,
        EMCI_CMMB_MOD_16QAM                     = 2
} EMCI_CMMB_MODULATION;

typedef enum 
{ 
        EMCI_CMMB_SCRAMB_MODE0                  = 0, 
        EMCI_CMMB_SCRAMB_MODE1                  = 1
} EMCI_CMMB_SCRAMBLE_MODE;

typedef struct _MCI_CMMB_FILTER_PARAM
{
    EMCI_CMMB_RS_CODE_RATE      RsCodeRate;
    EMCI_CMMB_BYTE_INT	        ByteInt;
    EMCI_CMMB_LDPC_CODE_RATE    LdpcCodeRate;
    EMCI_CMMB_MODULATION        Modulation;
    EMCI_CMMB_SCRAMBLE_MODE     ScrambMode;
    UChar                       ucNumOfTimeSlot;
    pUChar                      pucTimeSlotNum;
} MCI_CMMB_FILTER_PARAM, *PMCI_CMMB_FILTER_PARAM, **PPMCI_CMMB_FILTER_PARAM;

typedef struct _MCI_CMMB_CA_INFO
{
    UInt16                      usCaSystemId;
    UChar                       ucEmmDataType;
    UChar                       ucEcmDataType;
    UChar                       ucEcmTransportType;
} MCI_CMMB_CA_INFO, *PMCI_CMMB_CA_INFO, **PPMCI_CMMB_CA_INFO;

//virtual INT32           MciCmmbSetMuxConfig(UChar ucQtyMuxFrames, EMCI_CMMB_SCRAMBLE_MODE ScrambMode, UChar ucNumOfTimeSlot, pUChar pucTimeSlotNum) = 0;

typedef struct _MCI_CMMB_MUX_CONFIG_PARAM
{
    EMCI_CMMB_SCRAMBLE_MODE     ScrambMode;
    UChar                       ucNumOfTimeSlot;
    UChar                       pucTimeSlotNum[40];
} MCI_CMMB_MUX_CONFIG_PARAM, *PMCI_CMMB_MUX_CONFIG_PARAM, **PPMCI_CMMB_MUX_CONFIG_PARAM;

typedef struct _MCI_CMMB_MUX_CONFIG
{
    UChar                       ucQtyMuxFrames;
    MCI_CMMB_MUX_CONFIG_PARAM   MuxConfigParam[40];
} MCI_CMMB_MUX_CONFIG, *PMCI_CMMB_MUX_CONFIG, **PPMCI_CMMB_MUX_CONFIG;

#endif // _MCI_COMMON_H_
