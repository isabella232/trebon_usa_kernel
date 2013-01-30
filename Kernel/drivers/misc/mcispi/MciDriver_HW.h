/* drivers/misc/mcispi/MciDriver_HW.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciDriver_HW.h
**
**  Description
**      Header file for defining the interface of H/W control
**
**  Authur:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.20  NEW     Created.
**/

#ifndef _MCI_DRIVER_HW_H_
#define _MCI_DRIVER_HW_H_

#include "MciTypeDefs.h"

typedef Int32 (*MCI_DRV_INTR_HANDLER_FUNC)(pVoid pContext);
typedef Int32 (*MCI_DRV_CARD_EVENT_HANDLER_FUNC)(pVoid pContext, UInt32 uiEventType);
typedef Int32 (*MCI_DRV_CARD_STATE_TO_APP_EVENT_FUNC)(pVoid pContext, UInt32 uiParam);
typedef Int32 (*MCI_DRV_BOOT_SUCCESS_APP_EVENT_FUNC)(pVoid pContext);

typedef enum {
    EDEV_STATE_POWER_OFF,
    EDEV_STATE_DETACHED = EDEV_STATE_POWER_OFF,
    EDEV_STATE_POWER_ON,
    EDEV_STATE_ATTACHED = EDEV_STATE_POWER_ON,
    EDEV_STATE_RESET,
    EDEV_STATE_FW_DOWNLOAD,
    EDEV_STATE_READY,
    EDEV_STATE_SHUTDOWN_IN_PROGRESS
} EDRV_STATE;

typedef enum {
    EDEV_CARD_EVENT_ATTACHED,
    EDEV_CARD_EVENT_EJECTED,
} EDEV_CARD_EVENT;

#include "MciDriver_HW_DEFINE.h"

extern Bool         MciHwInitialize(PMCI_HW_CONTEXT pHwCtx, UInt32 uiContext, pVoid lpBusContext);
extern Void			MciHwRemoveInstance(PMCI_HW_CONTEXT pHwCtx);
extern Bool         MciHwRegisterInterruptHandle(PMCI_HW_CONTEXT pHwCtx, MCI_DRV_INTR_HANDLER_FUNC fnThread, pVoid pContext);
extern Bool         MciHwRegisterCardEventHandle(PMCI_HW_CONTEXT pHwCtx, MCI_DRV_CARD_EVENT_HANDLER_FUNC fnThread, pVoid pContext);
extern UInt32       MciHwSendMessage(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, UInt32 uiTransferLen);
extern Bool         MciHwPreFirmwareImageDownload(PMCI_HW_CONTEXT pHwCtx);
extern Bool         MciHwFirmwareImageDownload(PMCI_HW_CONTEXT pHwCtx, UInt32 uiAddress, pUChar pBuffer, UInt32 uiTransferLen);
extern Bool         MciHwPostFirmwareImageDownload(PMCI_HW_CONTEXT pHwCtx);
extern ERRORCODE    MciHwRecvMessageHeader(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, UInt32 uiRecvLen);
extern ERRORCODE    MciHwRecvMessageBody(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, UInt32 uiRecvLen);
extern ERRORCODE    MciHwRecvMessageBodyWithDMA(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, pUChar pPaBuffer, UInt32 uiRecvLen);
extern Void         MciHwResetChannelHardware(PMCI_HW_CONTEXT pHwCtx);
extern Void         MciHwStartChannelHardware(PMCI_HW_CONTEXT pHwCtx);
extern Bool         MciHwIsStartedChannelLink(PMCI_HW_CONTEXT pHwCtx);
extern Void         MciHwPowerOnChannel(PMCI_HW_CONTEXT pHwCtx);
extern Void         MciHwPowerOffChannel(PMCI_HW_CONTEXT pHwCtx);
extern UInt32       MciHwGetHwInterfaceType(PMCI_HW_CONTEXT pHwCtx);
extern Void         MciDrvDisableInterrupt(void);
extern Void         MciDrvEnableInterrupt(void);
extern Bool 		MciHwCheckDmaOperationIsDone(PMCI_HW_CONTEXT pHwCtx, Bool bIsPended);
extern Int32		MciHwDoSelfTest(PMCI_HW_CONTEXT pHwCtx, pInt32 pResult);

typedef enum {
    EDRV_HW_CAPS_DMA,                // Support DMA
    EDRV_HW_CAPS_TS,                 // Support TS H/W Interface
    EDRV_HW_CAPS_HW_INTF_TYPE,       // H/W Interface Type
    EDRV_HW_CAPS_DEVICE_STATE,       // Device State
    EDRV_HW_CAPS_SDIO_RX_ADDR,
    EDRV_HW_CAPS_SDIO_TX_ADDR,
    EDRV_HW_CAPS_RECV_THREAD_PRIORITY,
    EDRV_HW_CAPS_SDIO_ENABLE_MULTI_BLK_TRANSFER,
    EDRV_HW_CAPS_SPI_PRESCALE,       // SPI Prescale Value   
} EDRV_HW_CAPS;
extern Void         MciHwInitializeDeviceCapabilities(PMCI_HW_CONTEXT pHwCtx);
extern UInt32       MciHwGetDeviceCapability(PMCI_HW_CONTEXT pHwCtx, EDRV_HW_CAPS capsId);
extern Bool         MciHwSetDeviceCapability(PMCI_HW_CONTEXT pHwCtx, EDRV_HW_CAPS capsId, UInt32 uiValue);
extern Bool         MciHwIsInterruptActivated(void);
#endif // _MCI_DRIVER_HW_H_
