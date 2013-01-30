/* drivers/misc/mcispi/MciDriver.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciDriver.c
**
**  Description
**      Define the MDD of MCI Driver
**
**  Authur:
**      JongHo Park (jongho0910.park@samsung.com)
**      David Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.12.20  NEW     Created.
**      2011.08.11  MODIFY  Modified for TIKAL project to support 2.6.35 kernel
**/


#include "MciOsalApi.h"
#include "MciDriver.h"

//
// Static functions
//
BOOL            DrvInitializeBuffers(PMCI_DRIVER pDrvCtx, BOOL bAllocateMemory);
PMCI_BUF_DESC   DrvPreprocessMessage(PMCI_DRIVER pDrvCtx, PMCI_BUF_DESC pBufDesc);
ERRORCODE       DrvHandleSpiIrqInterrupt(INT32 irq, pVoid dev_id , struct pt_regs* regs );
ERRORCODE       DrvHandlePendedSpiIrqInterrupt(PMCI_DRIVER pDrvCtx);
Bool            DrvCheckTxRequestAndSend(PMCI_DRIVER pDrvCtx);
BOOL            DrvSetDriverConfiguration(PMCI_DRIVER pDrvCtx, PMCI_DRIVER_CONFIG pDrvConfig);
BOOL            DrvDownloadChannelFirmware(PMCI_DRIVER pDrvCtx, PMCI_FWIMG_BUF_INFO pFwImgBufInfo);
BOOL            DrvDownloadFirmwareForRecovery(PMCI_DRIVER pDrvCtx, PMCI_FWIMG_BUF_INFO pFwImgBufInfo);
Void            DrvSendMessage(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiTransferLen);
BOOL            DrvNotifyDriverStateToNetworkDriver(PMCI_DRIVER pDrvCtx, UInt32 uiParam);
BOOL            DrvNotifyDriverStateToApp(PMCI_DRIVER pDrvCtx, UInt32 uiParam);
BOOL            DrvCreateEventHandles(PMCI_DRIVER pDrvCtx);
BOOL 		    MciBootSuccessEvent(PMCI_DRIVER pDrvCtx);

//#define DBGON 1

ULong volatile jiffies_Start, jiffies_End, diff_jiffies , jiffies_later;
UInt32         uiTickStart; //, uiTickRecv, uiTickQueue, uiTickEvent;

ERRORCODE DrvHandleSpiIrqInterrupt(INT32 irq, pVoid dev_id, struct pt_regs* regs)
{
    PMCI_DRIVER 	pDrvCtx = (PMCI_DRIVER)dev_id;


    ERRORCODE       eRet = EMCI_FAIL;
    Bool            bIsLocked = TRUE;
    MIS_MSG_HDR     tmpMsgHdr;
    UInt32          uiBodyLen;
    PMCI_BUF_DESC   pBufDesc;
    PMIS_MSG_HDR    pMsgHdr;
    UInt32          i;
	Bool			bRequestIsPended = FALSE;

#ifdef _DBG_INTR_TIME
    UInt32          uiTickRecv, uiTickQueue, uiTickEvent;
#endif
    
    jiffies_Start = jiffies;

    do
    {
        
        uiTickStart = MciOsal_GetTickCount();

		if (FALSE == MciHwCheckDmaOperationIsDone(&(pDrvCtx->MciHwCtx), (Bool)(pDrvCtx->pPendedDesc)))
		{
            RETAILMSG(1, (TEXT("########### SPI RX DMA ERROR ###########\r\n")));
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] SPI RX DMA is in progress\r\n")));
            MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pDrvCtx->pPendedDesc, pDrvCtx->hLockStreamDescQueue, NULL);
            pDrvCtx->lRxStarted = 0;
		}

        // If TX operation is in progress, wait for it done
        while(pDrvCtx->lTxStarted) 
        { 
            MciOsal_Sleep(1);        
        }

        // Check TX request queue and send message if request exists
        if (DrvCheckTxRequestAndSend(pDrvCtx))
        {
            RETAILMSG(1, (TEXT("[MCIDRV] Send Message before IRQ ACK\r\n")));
            MciOsal_Sleep(1);
        }
        
        MciOsal_EnterCodeLock(pDrvCtx->hLockHwAccess);
        pDrvCtx->lRxStarted++;
        bIsLocked = TRUE;

        //
        // Receive the header of Message
        //
        MciOsal_memset(&tmpMsgHdr, 0, sizeof(MIS_MSG_HDR));
		eRet = MciHwRecvMessageHeader(&pDrvCtx->MciHwCtx, (pUChar)&tmpMsgHdr, MIS_MSG_HDR_LEN);
        if (EMCI_OK != eRet)
        {
    	    jiffies_Start = jiffies;
			if (eRet == -2)
			{
            	RETAILMSG(1, (TEXT("[ERROR!MCIDRV] DMB_EN = %d, DMB_EN_2.8V = %d, DMB_RST = %d\r\n"), gpio_get_value(S5PV210_MP04(1)), gpio_get_value(S5PV210_MP04(2)), gpio_get_value(S5PV210_GPH3(6))));
				if ((gpio_get_value(S5PV210_MP04(1)) && gpio_get_value(S5PV210_MP04(2)) && gpio_get_value(S5PV210_GPH3(6))) == 0)
				{
            		RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid Power Control Value\r\n")));
            		RETAILMSG(1, (TEXT("               DMB_EN      = %d\r\n"), gpio_get_value(S5PV210_MP04(1))));
            		RETAILMSG(1, (TEXT("               DMB_EN_2.8V = %d\r\n"), gpio_get_value(S5PV210_MP04(2))));
            		RETAILMSG(1, (TEXT("               DMB_RST     = %d\r\n"), gpio_get_value(S5PV210_GPH3(6))));

				}
				DrvNotifyDriverStateToApp(pDrvCtx, (EMCI_TYPE_SPI<<24) + EMCI_EVENT_INVALID_CHIP_STATE);
				break;
			}
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't find STX in Received Message\r\n")));
            break;
        }
        
        if (tmpMsgHdr.usLen & 0x01)
        {
            uiBodyLen = tmpMsgHdr.usLen + 1;   // + ETX
        }
        else
        {
            uiBodyLen = tmpMsgHdr.usLen + 2;   // + ETX + PAD(for half-word align)
        }

        // Get Buffer Descriptor
        if (tmpMsgHdr.ucType == (UInt8)pDrvCtx->uiTypeStreamData)
        {
	RETAILMSG(1, (TEXT("[MCIDRV] DMA Started\r\n")));
            pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QFreeStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL);
        }
        else
        {
            pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QFreeMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);
        }
        
        if (pBufDesc == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't get buffer descriptor for 0x%02X message\r\n"), tmpMsgHdr.ucType));
            break;
        }
        
        pMsgHdr = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc);
        pMsgHdr->ucStx  = tmpMsgHdr.ucStx;
        pMsgHdr->ucType = tmpMsgHdr.ucType;
        pMsgHdr->usLen  = tmpMsgHdr.usLen;

        //
        // Receive the body of message
        //
        if (pMsgHdr->ucType == (UInt8)pDrvCtx->uiTypeStreamData)
        { // Receive Stream
            
            // Check the length in Message header
            if ((uiBodyLen + MIS_MSG_HDR_LEN) > (pDrvCtx->uiMaxStreamBufferSize - MCI_MSG_START_POS_IN_DESC_BUF)
                || (uiBodyLen < 4))
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] #########################################################\r\n")));
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] (%d) The lenth of stream body(%d) is strange!!! MaxSize = %d\r\n"), MciOsal_GetTickCount(), uiBodyLen, pDrvCtx->uiMaxStreamBufferSize));
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV]      STX=%02X Type=%02X Len=%04X\r\n"), pMsgHdr->ucStx, pMsgHdr->ucType, pMsgHdr->usLen));
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] #########################################################\r\n")));
                MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                pBufDesc = NULL;
                break;
            }
            // Receive Stream body
            if (pDrvCtx->MciHwCtx.bSupportDMA == FALSE)
            { 
                eRet = MciHwRecvMessageBody(&pDrvCtx->MciHwCtx, &pMsgHdr->ucData[0], uiBodyLen);
            }
            else
            {	
				
                PMIS_MSG_HDR pPaMsgHdr = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_PA_MSG_BUF(pBufDesc);

                eRet = MciHwRecvMessageBodyWithDMA(&pDrvCtx->MciHwCtx, &pMsgHdr->ucData[0], &pPaMsgHdr->ucData[0], uiBodyLen);

#ifdef _DBG_INTR_TIME
                uiTickRecv = MciOsal_GetTickCount();
#endif
				bRequestIsPended= TRUE;
                          
            }

            // Check an error
            if (bRequestIsPended == FALSE)
            {
	            if (eRet != EMCI_OK)
	            {
	                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] [%d] Occur an error in receiving stream body\r\n"), uiTickStart));
	                MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
	                pBufDesc = NULL;
	                eRet = EMCI_FAIL;
	                break;
	            }
	            else if (pMsgHdr->ucData[pMsgHdr->usLen] != MIS_MSG_ETX)
	            {
	                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] [%u] There's no ETX in received stream body[%d]: %02X %02X %02X %02X %02X %02X\r\n"), uiTickStart, pMsgHdr->usLen, pMsgHdr->ucData[pMsgHdr->usLen-4], pMsgHdr->ucData[pMsgHdr->usLen-3], pMsgHdr->ucData[pMsgHdr->usLen-2], pMsgHdr->ucData[pMsgHdr->usLen-1], pMsgHdr->ucData[pMsgHdr->usLen], pMsgHdr->ucData[pMsgHdr->usLen+1]));
	                MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
	                pBufDesc = NULL;
	                eRet = EMCI_OK;

	                while (MciHwIsInterruptActivated())
	                {
	                    UChar tmpBuf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	                    //RETAILMSG(1, (TEXT("#")));
	                    MciHwSendMessage(&pDrvCtx->MciHwCtx, tmpBuf, 4);
	                    //MciOsal_Sleep(1);
	                }
	                break;
	            }

	            MciQueuePushTail(&pDrvCtx->QReceivedStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
#ifdef _DBG_INTR_TIME
				uiTickQueue = MciOsal_GetTickCount();
#endif
				MciOsal_SetEvent(pDrvCtx->hMciNotifyReceivedStreamEvent);
#ifdef _DBG_INTR_TIME
				uiTickEvent = MciOsal_GetTickCount();
#endif
            }
			else
			{
				pDrvCtx->pPendedDesc = pBufDesc;
			    if (bIsLocked)
			    {
			        MciOsal_LeaveCodeLock(pDrvCtx->hLockHwAccess);
			        bIsLocked = FALSE;
			    }
				return IRQ_HANDLED;
			}
        }
        else
        {// Receive Message
            
            // Check the length in Message header
            if ((uiBodyLen + MIS_MSG_HDR_LEN) > (pDrvCtx->uiMaxMessageBufferSize - MCI_MSG_START_POS_IN_DESC_BUF))
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] #########################################################\r\n")));
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] (%d)The length of message body(%d) is strange!!! MaxSize = %d\r\n"), MciOsal_GetTickCount(), uiBodyLen, pDrvCtx->uiMaxMessageBufferSize));
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV]      STX=%02X Type=%02X Len=%04X\r\n"), pMsgHdr->ucStx, pMsgHdr->ucType, pMsgHdr->usLen));
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] #########################################################\r\n")));
                MciQueuePushTail(&pDrvCtx->QFreeMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                pBufDesc = NULL;
                break;
            }

            // Receive Message body
            if (EMCI_OK != MciHwRecvMessageBody(&pDrvCtx->MciHwCtx, &pMsgHdr->ucData[0], uiBodyLen))
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Occur an error in receiving message body\r\n")));
                MciQueuePushTail(&pDrvCtx->QFreeMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                pBufDesc = NULL;
                break;
            }
            
            if (pMsgHdr->ucData[pMsgHdr->usLen] != MIS_MSG_ETX)
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] There's no ETX in received message body : ")));
                
                for (i = pMsgHdr->usLen+8; i > 0; i--)
                {
                    if ((i%16)==0) RETAILMSG(1, (TEXT("\r\n         ")));
                    RETAILMSG(1, (TEXT("%02X "), pMsgHdr->ucData[pMsgHdr->usLen-i]));
                }
            	RETAILMSG(1, (TEXT("\r\n")));

                MciQueuePushTail(&pDrvCtx->QFreeMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                pBufDesc = NULL;
                break;
            }

            pBufDesc = DrvPreprocessMessage(pDrvCtx, pBufDesc);


            if (pBufDesc)
            {
                MciQueuePushTail(&pDrvCtx->QReceivedMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                
                MciOsal_SetEvent(pDrvCtx->hMciNotifyReceivedMessageEvent);
            }

        }

         eRet = EMCI_OK;

    } while(FALSE);

    if (bIsLocked)
    {
        pDrvCtx->lRxStarted = 0;
        MciOsal_LeaveCodeLock(pDrvCtx->hLockHwAccess);
        bIsLocked = FALSE;
    }
#ifdef _DBG_INTR_TIME
    RETAILMSG(0, (TEXT("[MCIDRV] [%d-%d] Recv = %d, Queue = %d, Event = %d\n"), uiTickStart,  MciOsal_GetTickCount(), uiTickRecv - uiTickStart, uiTickQueue- uiTickStart, uiTickEvent - uiTickStart));
#endif// Check TX request queue and send message if request exists
    MciOsal_Sleep(1);
    if (DrvCheckTxRequestAndSend(pDrvCtx))
    {
        RETAILMSG(1, (TEXT("[MCIDRV] Send Message after Interrupt\r\n")));
		MciOsal_Sleep(1);
    }
	jiffies_End = jiffies;

    return IRQ_HANDLED;
}
ERRORCODE DrvHandlePendedSpiIrqInterrupt(PMCI_DRIVER pDrvCtx)
{
	ERRORCODE eRet = EMCI_FAIL;
	PMCI_BUF_DESC pBufDesc = NULL;
	PMIS_MSG_HDR  pMsgHdr = NULL;

	do
	{
		if (pDrvCtx == NULL || pDrvCtx->pPendedDesc == NULL)
		{
			break;
		}

		pBufDesc = pDrvCtx->pPendedDesc;
		pMsgHdr  = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc);
		pDrvCtx->pPendedDesc = NULL;

	
		if (pMsgHdr->ucData[pMsgHdr->usLen] != MIS_MSG_ETX)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] [%u] There's no ETX in received stream body[%d]: %02X %02X %02X %02X %02X %02X\r\n"), uiTickStart, pMsgHdr->usLen, pMsgHdr->ucData[pMsgHdr->usLen-4], pMsgHdr->ucData[pMsgHdr->usLen-3], pMsgHdr->ucData[pMsgHdr->usLen-2], pMsgHdr->ucData[pMsgHdr->usLen-1], pMsgHdr->ucData[pMsgHdr->usLen], pMsgHdr->ucData[pMsgHdr->usLen+1]));
            MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
            pBufDesc = NULL;
            eRet = EMCI_OK;

            while (MciHwIsInterruptActivated())
            {
                UChar tmpBuf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
                //RETAILMSG(1, (TEXT("#")));
                MciHwSendMessage(&pDrvCtx->MciHwCtx, tmpBuf, 4);
                //MciOsal_Sleep(1);
            }
            break;
        }

	    MciQueuePushTail(&pDrvCtx->QReceivedStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
		MciOsal_SetEvent(pDrvCtx->hMciNotifyReceivedStreamEvent);
	} while (FALSE);

	
	pDrvCtx->lRxStarted = 0;
        RETAILMSG(1, (TEXT("[MCIDRV] DMA Done: processed=%d\r\n"), (UInt32)(jiffies-jiffies_Start)));
					
	return eRet;
}

PMCI_DRIVER MciDrvCreateInstance()
{
    PMCI_DRIVER pDrvCtx = (PMCI_DRIVER)MciOsal_AllocKernelMemory(sizeof(MCI_DRIVER));
    if (pDrvCtx)
    {
        MciOsal_memset(pDrvCtx, 0, sizeof(MCI_DRIVER));

		//modified by prabha -08/24
        pDrvCtx->bAttached                  = TRUE;//FALSE;   //TRUE;

#ifdef _MCI_DEVICE_DRIVER_ 
        pDrvCtx->bCompleteDrvInit           = FALSE;
#endif
        // Buffer Descriptors for Message
        pDrvCtx->uiMaxStreamBufferSize      = MCI_DRV_MAX_STREAM_BUF_SIZE;
        pDrvCtx->uiMaxMessageBufferSize     = MCI_DRV_MAX_MESSAGE_BUF_SIZE;
        pDrvCtx->uiMessageBufferDescNum     = MCI_DRV_NUM_OF_BUF_DESC;
        
        // Channel Interface IPC Message Type Initialize
        pDrvCtx->uiTypeFreqScan             = 0x33;
        pDrvCtx->uiTypeCreateFilter         = 0x02;
        pDrvCtx->uiTypeDeleteFilter         = 0x03;
        pDrvCtx->uiTypeStatus               = 0x41;
        pDrvCtx->uiTypeStreamData           = 0x44;
        pDrvCtx->uiTypeBootSuccess          = 0x42;
	    pDrvCtx->bIsBootSucess              = FALSE;	
		
    }
    return pDrvCtx;

}


BOOL MciBootSuccessEvent(PMCI_DRIVER pDrvCtx)
{
	MciOsal_SetEvent(pDrvCtx->hMciNotifyReceivedMessageEvent);
	pDrvCtx->bIsBootSucess = TRUE;
    return TRUE;        
	
}

BOOL   MciDrvInitialize(PMCI_DRIVER pDrvCtx, UInt32 uiContext, pVoid lpBusContext)
{
    bool bRet = FALSE;

    RETAILMSG(1, (TEXT("[MCIDRV] +MciDrvInitialize()\r\n")));

//    return 0;         // die    
    do
    {
        pDrvCtx->uiWaitTimeForStream        = 50;
        pDrvCtx->uiWaitTimeForMessage       = EMCI_EVENT_WAIT_INFINITE;
        pDrvCtx->uiWaitTimeForStateChange   = EMCI_EVENT_WAIT_INFINITE;
        pDrvCtx->hLockHwAccess              = MciOsal_CreateCodeLockObject();
//    return 0;         // die
        if (pDrvCtx->hLockHwAccess == NULL)
        {
            break;
        }
        MciOsal_InitCodeLockObject(pDrvCtx->hLockHwAccess);
//    return 0;     // die 
        if (FALSE == MciHwInitialize(&pDrvCtx->MciHwCtx, uiContext, lpBusContext))
        {
            break;
        }
        pDrvCtx->MciHwCtx.pDrvCtx = (pVoid)pDrvCtx;
        
        pDrvCtx->uiMciType = MciHwGetHwInterfaceType(&pDrvCtx->MciHwCtx);
//    return 0;       // die
		RETAILMSG(1, (TEXT("[MCIDRV] DrvInitializeBuffers++\n")));
        if (FALSE == DrvInitializeBuffers(pDrvCtx, TRUE))
        {
            break;
        }

		RETAILMSG(1, (TEXT("[MCIDRV] DrvCreateEventHandles\n")));

        if (FALSE == DrvCreateEventHandles(pDrvCtx))
        {
            break;
        }

        if (FALSE == MciHwRegisterInterruptHandle(&pDrvCtx->MciHwCtx, (MCI_DRV_INTR_HANDLER_FUNC)DrvHandleSpiIrqInterrupt, (pVoid)pDrvCtx))
        {
            break;
        }


        bRet = TRUE;

        wake_lock_init(&pDrvCtx->wlock, WAKE_LOCK_SUSPEND, "MciSpi");

        pDrvCtx->uiDrvState = EMCI_DRV_INITIALIZED;
        pDrvCtx->bAttached = TRUE;
    } while (FALSE);

    RETAILMSG(1, (TEXT("[MCIDRV] -MciDrvInitialize() = %d\r\n"), (bRet) ? 1 : 0));

#if 0
    printk("[MCIDRV] MCI_IOCTL_SEND_MESSAGE         =0x%08X\r\n", MCI_IOCTL_SEND_MESSAGE);
    printk("[MCIDRV] MCI_IOCTL_GET_MESSAGE          =0x%08X\r\n", MCI_IOCTL_GET_MESSAGE );
    printk("[MCIDRV] MCI_IOCTL_GET_IP_STREAM        =0x%08X\r\n", MCI_IOCTL_GET_IP_STREAM);
    printk("[MCIDRV] MCI_IOCTL_GET_IP_STREAM_DESC   =0x%08X\r\n", MCI_IOCTL_GET_IP_STREAM_DESC);
    printk("[MCIDRV] MCI_IOCTL_GET_TS_STREAM        =0x%08X\r\n", MCI_IOCTL_GET_TS_STREAM);
    printk("[MCIDRV] MCI_IOCTL_GET_DRIVER_STATE     =0x%08X\r\n", MCI_IOCTL_GET_DRIVER_STATE);
    printk("[MCIDRV] MCI_IOCTL_SET_DRIVER_CONFIG    =0x%08X\r\n", MCI_IOCTL_SET_DRIVER_CONFIG);
    printk("[MCIDRV] MCI_IOCTL_SET_STREAM_BUFFER_SIZE =0x%08X\r\n", MCI_IOCTL_SET_STREAM_BUFFER_SIZE);
    printk("[MCIDRV] MCI_IOCTL_CLEAR_STREAM_BUFFER  =0x%08X\r\n", MCI_IOCTL_CLEAR_STREAM_BUFFER);
    printk("[MCIDRV] MCI_IOCTL_CLOSE_MCI_INTERFACE  =0x%08X\r\n", MCI_IOCTL_CLOSE_MCI_INTERFACE);
    printk("[MCIDRV] MCI_IOCTL_FW_DOWNLOAD          =0x%08X\r\n", MCI_IOCTL_FW_DOWNLOAD);
    printk("[MCIDRV] MCI_IOCTL_SW_RESET_CHANNEL     =0x%08X\r\n", MCI_IOCTL_SW_RESET_CHANNEL);
    printk("[MCIDRV] MCI_IOCTL_HW_RESET_CHANNEL     =0x%08X\r\n", MCI_IOCTL_HW_RESET_CHANNEL);
    printk("[MCIDRV] MCI_IOCTL_MCI_DEBUG_CMD        =0x%08X\r\n", MCI_IOCTL_MCI_DEBUG_CMD);
    printk("[MCIDRV] MCI_IOCTL_GET_DRIVER_STATE_CHANGE_INFO =0x%08X\r\n", MCI_IOCTL_GET_DRIVER_STATE_CHANGE_INFO);
    printk("[MCIDRV] MCI_IOCTL_GET_DRIVER_INFO      =0x%08X\r\n", MCI_IOCTL_GET_DRIVER_INFO);
#endif

    return bRet;
}
Void MciDrvRemoveInstance(PMCI_DRIVER pDrvCtx)
{
//    Int32 ret;
    wake_lock_destroy(&pDrvCtx->wlock);

    if (pDrvCtx->uiMtvType == EMTV_TYPE_DVBH)
    {
        DrvNotifyDriverStateToNetworkDriver(pDrvCtx, (EMCI_TYPE_SPI<<24) + EMCI_EVENT_CARD_DETACHED);
    }
    MciHwRemoveInstance(&pDrvCtx->MciHwCtx);

    if (pDrvCtx->hLockHwAccess)
    {
        MciOsal_ReleaseCodeLockObject(pDrvCtx->hLockHwAccess);
        pDrvCtx->hLockHwAccess = NULL;
    }

    if (pDrvCtx->hLockMessageDescQueue)
    {
        MciOsal_ReleaseCodeLockObject(pDrvCtx->hLockMessageDescQueue);
        pDrvCtx->hLockMessageDescQueue= NULL;
    }

    if (pDrvCtx->hLockStreamDescQueue)
    {
        MciOsal_ReleaseCodeLockObject(pDrvCtx->hLockStreamDescQueue);
        pDrvCtx->hLockStreamDescQueue= NULL;
    }

    if (pDrvCtx->hMciNotifyReceivedIpStreamEvent)
    {
        MciOsal_ReleaseEventObject(pDrvCtx->hMciNotifyReceivedIpStreamEvent);
        pDrvCtx->hMciNotifyReceivedIpStreamEvent = NULL;
    }
    if (pDrvCtx->hMciNotifyReceivedTsStreamEvent)
    {
        MciOsal_ReleaseEventObject(pDrvCtx->hMciNotifyReceivedTsStreamEvent);
        pDrvCtx->hMciNotifyReceivedTsStreamEvent = NULL;
    }
    pDrvCtx->hMciNotifyReceivedStreamEvent = NULL;
    
    if (pDrvCtx->hMciNotifyReceivedMessageEvent)
    {
        MciOsal_ReleaseEventObject(pDrvCtx->hMciNotifyReceivedMessageEvent);
        pDrvCtx->hMciNotifyReceivedMessageEvent = NULL;
    }
    
    if (pDrvCtx->hMciNotifyStatusChangeEvent)
    {
        MciOsal_ReleaseEventObject(pDrvCtx->hMciNotifyStatusChangeEvent);
        pDrvCtx->hMciNotifyStatusChangeEvent = NULL;
    }

    if (pDrvCtx->hClosedAllEvent)
    {
        MciOsal_SetEvent(pDrvCtx->hClosedAllEvent);
        MciOsal_ReleaseEventObject(pDrvCtx->hClosedAllEvent);
        pDrvCtx->hClosedAllEvent = NULL;
    }
    
    if (pDrvCtx->lpvFixedBufferMemory)
    {
        if (pDrvCtx->MciHwCtx.bSupportDMA == FALSE)
        {
            MciOsal_FreeKernelMemory((pVoid)pDrvCtx->lpvFixedBufferMemory);
        }
        else
        {
            MciOsal_FreePhysicalMemory(pDrvCtx->hDmaMemory);
        }
        pDrvCtx->lpvFixedBufferMemory = NULL;
    }

    if (pDrvCtx->pImagePath)
    {
        MciOsal_FreeKernelMemory(pDrvCtx->pImagePath);
    }

    if (pDrvCtx)
    {
        MciOsal_FreeKernelMemory(pDrvCtx);
    }

}
BOOL   MciDrvOpen(PMCI_DRIVER pDrvCtx, UInt32 uiAccess, UInt32 uiShare)
{
    pDrvCtx->iOpenedDrvCount++;
    MciOsal_ResetEvent(pDrvCtx->hClosedAllEvent);

    return TRUE;
}
BOOL   MciDrvClose(PMCI_DRIVER pDrvCtx)
{
    pDrvCtx->iOpenedDrvCount--;
    if (pDrvCtx->iOpenedDrvCount <= 0)
    {
        MciOsal_SetEvent(pDrvCtx->hClosedAllEvent);
    }
    return TRUE;
}
UInt32 MciDrvReadStream(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiBufLen)
{
    PMCI_BUF_DESC   pBufDesc = NULL;
    PMIS_MSG_HDR    pMsgBuf = NULL;
    UInt32          uiReadBytes = 0;
    
    RETAILMSG(0, (TEXT("[MCIDRV] MciDrvReadStream PMCI_DRIVER = 0x%08X\r\n"), (UInt32)pDrvCtx));

    RETAILMSG(0, (TEXT("[MCIDRV] + MciDrvReadStream(): pBuffer = 0x%08X\r\n"), (UInt32)pBuffer));

    pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QReceivedStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL);
    if (pBufDesc == NULL && pDrvCtx->bSyncAccessForStream && pDrvCtx->bAttached)
    {
        MciOsal_WaitForEvent(pDrvCtx->hMciNotifyReceivedStreamEvent, pDrvCtx->uiWaitTimeForStream);
        MciOsal_ResetEvent(pDrvCtx->hMciNotifyReceivedStreamEvent);

        pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QReceivedStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL);
    }
    if (pBufDesc != NULL)
    {
        pMsgBuf = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc);

        uiReadBytes = pMsgBuf->usLen;
        if (uiBufLen < uiReadBytes)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Stream Buffer length is too short! Actual %d bytes => buffer %d bytes\r\n"), uiReadBytes, uiBufLen));
            uiReadBytes = 0;
            return uiReadBytes;
        }

        MciOsal_CopyToUser(pBuffer, pMsgBuf->ucData, uiReadBytes);

        RETAILMSG(0, (TEXT("[MCIDRV] ===> MciDrvReadStream(): pBuffer = 0x%08X\r\n"), (UInt32)pBuffer));
    
        MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
        pBufDesc = NULL;
    }
    
    RETAILMSG(0, (TEXT("[MCIDRV] - MciDrvReadStream(): %d bytes\r\n"), uiReadBytes));

    return uiReadBytes;
}
UInt32 MciDrvReadMessage(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiBufLen)
{
    PMCI_BUF_DESC   pBufDesc = NULL;
    PMIS_MSG_HDR    pMsgBuf = NULL;
    UInt32          uiReadBytes = 0;
    UInt32          ret =0;
    
    if (pBuffer == NULL)
    {
        RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid Receive Buffer pointer\r\n")));
        return 0;
    }

    pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QReceivedMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);

  	

    if (pBufDesc == NULL && pDrvCtx->bSyncAccessForMessage && pDrvCtx->bSyncAccessForStream)
    {
        RETAILMSG(_MCI_DBG_MSG_RX, (TEXT("[MCIDRV] Waiting for RecvMsg\r\n")));
        MciOsal_WaitForEvent(pDrvCtx->hMciNotifyReceivedMessageEvent, pDrvCtx->uiWaitTimeForMessage);
        MciOsal_ResetEvent(pDrvCtx->hMciNotifyReceivedMessageEvent);
        pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QReceivedMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);
    }
    
    if (pBufDesc != NULL)
    {

        
        pMsgBuf = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc);
    
        if (pMsgBuf->usLen & 0x01) uiReadBytes = pMsgBuf->usLen + MIS_MSG_HDR_LEN + 1/* ETX */;
        else                       uiReadBytes = pMsgBuf->usLen + MIS_MSG_HDR_LEN + 1/* ETX + PAD */;
        
        if (uiBufLen < uiReadBytes)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Message Buffer length is too short! Actual %d bytes => buffer %d bytes\r\n"), uiReadBytes, uiBufLen));
            return 0;
        }

		ret = MciOsal_CopyToUser(pBuffer, pMsgBuf, uiReadBytes);
		
        MciQueuePushTail(&pDrvCtx->QFreeMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
        pBufDesc = NULL;
    }
    RETAILMSG(_MCI_DBG_MSG_RX, (TEXT("[MCIDRV] -MciDrvReadMessage(): %d bytes\r\n"), uiReadBytes));

    return uiReadBytes;
}

UInt32 MciDrvSendMessage(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiBufLen)
{
    PMCI_BUF_DESC   pBufDesc = NULL;
    PMIS_MSG_HDR    pMsgBuf = (PMIS_MSG_HDR)pBuffer;
    UInt32          uiTransferLen = 0;

    RETAILMSG(_MCI_DBG_MSG_TX, (TEXT("[MCIDRV] +MciDrvSendMessage(): Lock state = %ld\r\n"), pDrvCtx->lRxStarted));
    if (pBuffer == NULL)
    {
        RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid Send Buffer Pointer\r\n")));
        return 0;
    }

	if (pMsgBuf->ucType==0x1 && pMsgBuf->ucData[0] != 0x4)
	{
        RETAILMSG(1, (TEXT("[MCIDRV] Request CMD = %d\r\n"), pMsgBuf->ucData[0]));
	}

    if (pMsgBuf->usLen & 0x01) uiTransferLen = MIS_MSG_HDR_LEN + pMsgBuf->usLen + 1 /* ETX */;
    else                       uiTransferLen = MIS_MSG_HDR_LEN + pMsgBuf->usLen + 2 /* ETX + PAD */;

    if (pDrvCtx->lRxStarted != 0)  // RX in progress
    {
        pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QFreeMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);
        if (pBufDesc == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] No free message buffer descriptor\r\n")));
            return 0;
        }

        MciOsal_memcpy((pUChar)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc), pBuffer, uiTransferLen);

        MciQueuePushTail(&pDrvCtx->QTxReqMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
        pBufDesc = NULL;
    }
    else
    {
        DrvSendMessage(pDrvCtx, pBuffer, uiTransferLen);
        RETAILMSG(_MCI_DBG_MSG_TX, (TEXT("[MCIDRV] Send MIS Message directly\r\n")));
    }
    
    RETAILMSG(_MCI_DBG_MSG_TX, (TEXT("[MCIDRV] -MciDrvSendMessage()\r\n")));

    return uiTransferLen;
}
Void   MciDrvPowerUp(PMCI_DRIVER pDrvCtx)
{
    RETAILMSG(1, (TEXT("[MCIDRV] POWER-UP\r\n")));
}
Void   MciDrvPowerDown(PMCI_DRIVER pDrvCtx)
{
    RETAILMSG(1, (TEXT("[MCIDRV] POWER-DOWN\r\n")));
}
UInt32 MciDrvSeek(PMCI_DRIVER pDrvCtx, Long lDistance, UInt32 uiuiMoveMethod)
{
    RETAILMSG(1, (TEXT("[MCIDRV] XXX_Seek\r\n")));
	return 0;
}
BOOL MciDrvIoControl(PMCI_DRIVER pDrvCtx, ULong uiCode, pUChar pIn, UInt32 uiInLen, pUChar pOut, UInt32 uiOutLen, pULong puiBytesWritten)//pUInt32 puiBytesWritten)
{
    BOOL            bRet = TRUE;
    PMCI_BUF_DESC   pBufDesc = NULL;
    PMIS_MSG_HDR    pMsgBuf;
    BOOL            bGetBufDesc = FALSE;
    MCI_DRIVER_INFO DrvInfo;


    if (puiBytesWritten) *puiBytesWritten = 0;

    RETAILMSG(_MCI_DBG_TRACE_FUNC, (TEXT("[MCIDRV] +IoControl: 0x%08X\r\n"), (UInt32)uiCode));
    switch (uiCode)
    {
        case MCI_IOCTL_SW_RESET_CHANNEL:
        case MCI_IOCTL_HW_RESET_CHANNEL:
            if (pDrvCtx->bAttached)
            {
                MciHwResetChannelHardware(&pDrvCtx->MciHwCtx);
                MciHwStartChannelHardware(&pDrvCtx->MciHwCtx);
            }
            break;

        case MCI_IOCTL_GET_TS_STREAM:
            bRet = FALSE;
            if (pOut && puiBytesWritten)
            {
                *puiBytesWritten = MciDrvReadStream(pDrvCtx, pOut, uiOutLen);
                if (*puiBytesWritten != 0)
                {
                    bRet = TRUE;
                }
            }
            break;
        case MCI_IOCTL_GET_IP_STREAM_DESC:
            bGetBufDesc = TRUE;
            if (uiInLen == sizeof(pVoid) && pIn)
            {
                pBufDesc = (PMCI_BUF_DESC)pIn;
                MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
            }
            pBufDesc = NULL; 
            // NO BREAK here!!! continue to the next statement: MCI_IOCTL_GET_IP_STREAM. 
        case MCI_IOCTL_GET_IP_STREAM:
            bRet = FALSE;
            do
            {
                UInt32  uiIpLen;
                UInt32  uiIpOffset;
                pByte   pIpData;
                if (pOut == NULL)               break;
                if (puiBytesWritten == NULL)    break;

                if (pDrvCtx->pCurStreamBufDesc == NULL)
                {
                    pDrvCtx->pCurStreamBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QReceivedStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                    if (pDrvCtx->pCurStreamBufDesc == NULL)
                    {
                        break;
                    }
                    MCI_BUF_DESC_SET_PARAM3(pDrvCtx->pCurStreamBufDesc, 0);
                }
                pMsgBuf = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pDrvCtx->pCurStreamBufDesc);

                uiIpOffset = MCI_BUF_DESC_GET_PARAM3(pDrvCtx->pCurStreamBufDesc);
                if (pMsgBuf->usLen < (uiIpOffset + MCI_IPV4_HDR_LEN))
                {
                    MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pDrvCtx->pCurStreamBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                    pDrvCtx->pCurStreamBufDesc = NULL;
                    continue;
                }

                pIpData    = &pMsgBuf->ucData[uiIpOffset];
                if (pIpData[0]>>4 == 0x04)
                {
                    uiIpLen = (pIpData[2] << 8) + pIpData[3];
                }
                else if (pIpData[0]>>4 == 0x06)
                {
                    uiIpLen = (pIpData[4]<<8) + pIpData[5];
                    uiIpLen += MCI_IPV6_HDR_LEN;
                }
                else
                {
                    RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid IP Header:  %02X %02X %02X %02X at Offset(%d)\r\n"), pIpData[0],pIpData[1],pIpData[2],pIpData[3], uiIpOffset));
                    MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pDrvCtx->pCurStreamBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                    pDrvCtx->pCurStreamBufDesc = NULL;
                    continue;
                }
                
                uiIpOffset += uiIpLen;
                if (pMsgBuf->usLen < (uiIpOffset-1))
                {
                    // Invalid IP Packet
                    RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid Packet Data = Too short! MsgLen=%d, BuffOffset=%d\r\n"), pMsgBuf->usLen, uiIpOffset));
                    MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pDrvCtx->pCurStreamBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                    pDrvCtx->pCurStreamBufDesc = NULL;
                    continue;
                }

                if (uiIpOffset & 0x3)
                {
                    uiIpOffset += 4 - (uiIpOffset & 0x03);
                }

                if (bGetBufDesc == FALSE)
                {   // MCI_IOCTL_GET_IP_STREAM
                    if (uiIpLen <= uiOutLen)
                    {
                        MciOsal_CopyToUser(pOut, pIpData, uiIpLen);
                        *puiBytesWritten = uiIpLen;
                    }
                    else
                    {
                        RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Output buffer is too short! Length = %d, Required Length = %d\r\n"), uiOutLen, uiIpLen));
                        break;
                    }
                }
                else
                {
                    pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QFreeStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                    if (pBufDesc != NULL)
                    {
                        PMIS_MSG_HDR pTmpHdr = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc);
                        MCI_BUF_DESC_SET_PARAM3(pBufDesc, pIpData);
                        pTmpHdr->usLen = uiIpLen;

                        MciOsal_CopyToUser((pVoid)pOut, (pVoid)pBufDesc, sizeof(pBufDesc));
                        *puiBytesWritten      = sizeof(PMCI_BUF_DESC);
                    }
                    else
                    {
                        RETAILMSG(1, (TEXT("[ERROR!MCIDRV] There's no free stream buffer descriptor when read IP Desc\r\n")));
                        break;
                    }
                }
                
                MCI_BUF_DESC_SET_PARAM3(pDrvCtx->pCurStreamBufDesc, uiIpOffset);

                bRet = TRUE;
                break;
            } while (TRUE);
            break;
        case MCI_IOCTL_SEND_MESSAGE:
            {
                UInt32 uiTransferLen = 0;

                RETAILMSG(_MCI_DBG_MSG_TX, (TEXT("[MCIDRV] +IOCTL: SendMessage(): Lock state = %ld\r\n"), pDrvCtx->lRxStarted));
                if (pIn == NULL)
                {
                    RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid Send Buffer Pointer\r\n")));
                    bRet = FALSE;
                    break;
                }
             
                pMsgBuf = (PMIS_MSG_HDR)pIn;

				if (pMsgBuf->ucType==0x1 && pMsgBuf->ucData[0] != 0x4)
				{
			        RETAILMSG(1, (TEXT("[MCIDRV] Request CMD = %d\r\n"), pMsgBuf->ucData[0]));
				}

                if (pMsgBuf->usLen & 0x01) uiTransferLen = MIS_MSG_HDR_LEN + pMsgBuf->usLen + 1 /* ETX */;
                else                       uiTransferLen = MIS_MSG_HDR_LEN + pMsgBuf->usLen + 2 /* ETX + PAD */;

                if (pDrvCtx->lRxStarted != 0) // RX in progress
                {
                     
                    pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QFreeMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                    if (pBufDesc == NULL)
                    {
                        RETAILMSG(1, (TEXT("[ERROR!MCIDRV] No free message buffer descriptor\r\n")));
                        bRet = FALSE;
                        break;
                    }

                    MciOsal_memcpy((pUChar)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc), pIn, uiTransferLen);

                    MciQueuePushTail(&pDrvCtx->QTxReqMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                    pBufDesc = NULL;
                }
                else
                {
                    RETAILMSG(_MCI_DBG_MSG_TX, (TEXT("[MCIDRV] IOCTL: Send MIS Message directly\r\n")));
//                    MciOsal_Sleep(1);
                    DrvSendMessage(pDrvCtx, pIn, uiTransferLen);
                }
                RETAILMSG(_MCI_DBG_MSG_TX, (TEXT("[MCIDRV] -IOCTL: SendMessage\r\n")));             
            }
            break;
        case MCI_IOCTL_GET_MESSAGE:
            {
                bRet = FALSE;
                if (pOut && puiBytesWritten)
                {
                    *puiBytesWritten = MciDrvReadMessage(pDrvCtx, (pUChar)pOut, (UInt32)uiOutLen);
                    if (*puiBytesWritten != 0)
                    {
                        bRet = TRUE;
                    }
                }
            }
            break;
        case MCI_IOCTL_GET_DRIVER_STATE:
            if (pOut) 
            {
                MciOsal_PutUser(pDrvCtx->MciHwCtx.uiDevState, (pVoid)pOut);
            }
            if (puiBytesWritten) *puiBytesWritten = sizeof (UInt32);
            break;
        case MCI_IOCTL_FW_DOWNLOAD:
            if (pIn)
            {
                bRet = DrvDownloadChannelFirmware(pDrvCtx, (PMCI_FWIMG_BUF_INFO)pIn);
            }
            else
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Need Firmware Image Buffer\r\n")));
                bRet = FALSE;
            }
            break;
		case MCI_IOCTL_FW_RECOVERY:
			if (pIn)
			{
                bRet = DrvDownloadFirmwareForRecovery(pDrvCtx, (PMCI_FWIMG_BUF_INFO)pIn);
			}
			else
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Need Firmware Image Buffer for Recovery\r\n")));
                bRet = FALSE;
            }
            break;
        case MCI_IOCTL_SET_DRIVER_CONFIG:

            printk("[MCIDRV] +IoControl== SET_DRIVER_CONFIG: 0x%08X\r\n", (UInt32)uiCode);


            if (pIn)
            {
                bRet = DrvSetDriverConfiguration(pDrvCtx, (PMCI_DRIVER_CONFIG)pIn);
            }
            else
            {
                bRet = FALSE;
            }
            break;

        case MCI_IOCTL_CLOSE_MCI_INTERFACE:
            if (MciHwIsStartedChannelLink(&pDrvCtx->MciHwCtx) != FALSE)
            {
                RETAILMSG(1, (TEXT("[MCIDRV] MCI_IOCTL_CLOSE_MCI_INTERFACE111\r\n")));

                MciHwResetChannelHardware(&pDrvCtx->MciHwCtx);
                RETAILMSG(1, (TEXT("[MCIDRV] MCI_IOCTL_CLOSE_MCI_INTERFACE222\r\n")));

                MciHwPowerOffChannel(&pDrvCtx->MciHwCtx);
                RETAILMSG(1, (TEXT("[MCIDRV] MCI_IOCTL_CLOSE_MCI_INTERFACE333\r\n")));

                wake_unlock(&pDrvCtx->wlock);
                RETAILMSG(1, (TEXT("[MCIDRV] Power wake-unlocked --- \r\n")));    
#if 0
				{
					struct file*	fp = NULL;
					mm_segment_t	fs;
	
					fs = get_fs();
					set_fs(KERNEL_DS);
	
					do 
					{
						fp = filp_open(LTE_ALWAYS_ON_FILE, O_RDWR, 0);
						if (fp == NULL)
						{
							RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't open '%s' \r\n"), LTE_ALWAYS_ON_FILE));
							break;
						}
						vfs_write(fp, &pDrvCtx->ucSavedLteWakeonState, 1, &fp->f_pos);
						RETAILMSG(1, (TEXT("[MCIDRV] Store the previous state of LteAlwasyOn = '%c'\r\n"), pDrvCtx->ucSavedLteWakeonState));
	
					} while (FALSE);
					
					if (fp) filp_close(fp, NULL);
					set_fs(fs);
            	}
#endif
			}

            bRet = TRUE;
            break;
        case MCI_IOCTL_CLEAR_STREAM_BUFFER:
            {
                RETAILMSG(1, (TEXT("[MCIDRV] Clear received Stream Buffer\r\n")));
                while (NULL != (pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QReceivedStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL)))
                {
                    MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                }
            }
            bRet = TRUE;
            break;
        case MCI_IOCTL_SET_STREAM_BUFFER_SIZE:
            if (pIn)
            {
                UInt32 uiBufferSize = *(pUInt32)pIn;
                if (pDrvCtx->uiMaxStreamBufferSize != uiBufferSize)
                {
                    RETAILMSG(1, (TEXT("[MCIDRV] The length of Stream buffer is changed: %d bytes => %d bytes\r\n"), pDrvCtx->uiMaxStreamBufferSize, uiBufferSize));

                    pDrvCtx->uiMaxStreamBufferSize = uiBufferSize;

                    MciQueueInit(&pDrvCtx->QFreeMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                    MciQueueInit(&pDrvCtx->QReceivedMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);
                    MciQueueInit(&pDrvCtx->QFreeStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL);
                    MciQueueInit(&pDrvCtx->QReceivedStreamDesc, pDrvCtx->hLockStreamDescQueue, NULL);

                    DrvInitializeBuffers(pDrvCtx, FALSE);
                }
            }
            else
            {
                bRet = FALSE;
            }
            break;
        case MCI_IOCTL_GET_DRIVER_STATE_CHANGE_INFO:
            bRet = FALSE;
			
            if (pDrvCtx->bAttached && pDrvCtx->bSyncAccessForState != FALSE)
            {
                // TODO:
                if (EMCI_EVENT_OBJECT_0 == MciOsal_WaitForEvent(pDrvCtx->hMciNotifyStatusChangeEvent, pDrvCtx->uiWaitTimeForStateChange))
                {
                    if (pOut)
                    {
                        MciOsal_PutUser(pDrvCtx->uiDriverStateChangeInfo, pOut);
                        if (puiBytesWritten)
                        {
                            *puiBytesWritten = sizeof(pDrvCtx->uiDriverStateChangeInfo);
                        }
                        bRet = TRUE;
                        printk("[MCIDRV] MCI_IOCTL_GET_DRIVER_STATE_CHANGE_INFO MciOsa_PutUser success!! bRet = %d\r\n", bRet);

                    }
                }
                MciOsal_ResetEvent(pDrvCtx->hMciNotifyStatusChangeEvent);
                printk("[MCIDRV] Exit MCI_IOCTL_GET_DRIVER_STATE_CHANGE_INFO: 0x%08X\r\n", (UInt32)uiCode);

            }
	     
			
            break;
        case MCI_IOCTL_GET_DRIVER_INFO:
            if (pOut)
            {
//#define WIDEN2(x)   L ## x
#define WIDEN2(x)   x
#define WIDEN(x)    WIDEN2(x)
#define __WDATE__   WIDEN(__DATE__)
#define __WTIME__   WIDEN(__TIME__)
				
                printk("[MCIDRV] Enter MCI_IOCTL_GET_DRIVER_INFO: 0x%08X\r\n", (UInt32)uiCode);

                if (uiOutLen < sizeof(MCI_DRIVER_INFO))
                {
                    bRet = FALSE;
                    break;
                }
				
				DrvInfo.uiMciType				= MciHwGetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_HW_INTF_TYPE);
                
                printk("[MCIDRV] MCI_IOCTL_GET_DRIVER_INFO DrvInfo.uiMciType =%d\r\n", DrvInfo.uiMciType);
				DrvInfo.uiMajorVersion		    = MCI_DRIVER_MAJOR_VERSION;
				DrvInfo.uiMinorVersion		    = MCI_DRIVER_MINOR_VERSION;
				DrvInfo.uiReleaseVersion		= MCI_DRIVER_RELEASE_VERSION;
				DrvInfo.uiIoctlMajorVersion	    = MCI_DRIVER_INTF_MAJOR_VERSION;
				DrvInfo.uiIoctlMinorVersion	    = MCI_DRIVER_INTF_MINOR_VERSION;
				DrvInfo.uiIoctlReleaseVersion	= MCI_DRIVER_INTF_RELEASE_VERSION;
				
				MciOsal_sprintf(DrvInfo.ucCompiledDate, _T("%s"), __WDATE__);
				MciOsal_sprintf(DrvInfo.ucCompiledTime, _T("%s"), __WTIME__);
                MciOsal_PutUser(DrvInfo, pOut);
                if (puiBytesWritten)
                {
                    *puiBytesWritten = sizeof(MCI_DRIVER_INFO);
                }
            }
            else
            {
                bRet = FALSE;
            }
            break;

        case MCI_IOCTL_CHECK_CARD_INSERT:
        	break;
			
        case MCI_IOCTL_DO_SELFTEST:
            bRet = FALSE;
            if (pOut)
            {
				Int32 iResult = -1;
                if (puiBytesWritten)
                {
                    *puiBytesWritten = 0;
                }

				if (EMCI_OK != MciHwDoSelfTest(&pDrvCtx->MciHwCtx, &iResult))
				{
            		RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Failed to execute MciHwDoSelfTest()\r\n")));
					break;
				}
                MciOsal_CopyToUser(pOut, &iResult, sizeof(Int32));
                if (puiBytesWritten)
                {
                    *puiBytesWritten = sizeof(MCI_DRIVER_INFO);
                }
				bRet = TRUE;
            }
        	break;
			

        default:
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Unknown IOCTL Command = 0x%08X\r\n"), uiCode));
            bRet = FALSE;
            break;
    }

    RETAILMSG(_MCI_DBG_TRACE_FUNC, (TEXT("[MCIDRV] -IoControl: %d\r\n"), bRet));

    return bRet;
}

Bool DrvInitializeBuffers(PMCI_DRIVER pDrvCtx, BOOL bAllocateMemory)
{
    UInt32          uiDescNum = 0, i = 0;
    UInt32          uiCurOffset = 0;
    UInt32          uiDescBufLen;
    PMCI_BUF_DESC   pBufDesc = NULL;

	//printk("+DrvInitializeBuffers\n");
	
    if (bAllocateMemory != FALSE)
    {
        // Initialize Critical Sections
        pDrvCtx->hLockMessageDescQueue    = MciOsal_CreateCodeLockObject();
        pDrvCtx->hLockStreamDescQueue     = MciOsal_CreateCodeLockObject();

        MciOsal_InitCodeLockObject(pDrvCtx->hLockMessageDescQueue);
        MciOsal_InitCodeLockObject(pDrvCtx->hLockStreamDescQueue);

        // allocate fixed hardware memory for common buffer
        if (pDrvCtx->MciHwCtx.bSupportDMA == FALSE)
        {

            pDrvCtx->lpvFixedBufferMemory   = MciOsal_AllocKernelMemory(MCI_DRV_TOTAL_SIZE_OF_BUFFER_MEMORY);
			
            if (pDrvCtx->lpvFixedBufferMemory == NULL)
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Failed to allocate MCI DRV buffer memory\r\n")));
                return FALSE;
            }
            pDrvCtx->lpvPhysFixedBufferMemory = pDrvCtx->lpvFixedBufferMemory;
        }
        else
        {
        	printk("+MciOsal_AllocPhysicalMemory : %d\n",MCI_DRV_TOTAL_SIZE_OF_BUFFER_MEMORY);
            // Allocate non-cashed & DMA memory
            pDrvCtx->hDmaMemory = (H_PHYSMEM)MciOsal_AllocPhysicalMemory(MCI_DRV_TOTAL_SIZE_OF_BUFFER_MEMORY, FALSE);
            if (pDrvCtx->hDmaMemory == NULL)
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Failed to allocate MCI DRV DMA buffer memory\r\n")));
                return FALSE;
            }
			printk("-MciOsal_AllocPhysicalMemory 0x%08X\n", (UInt32)pDrvCtx->hDmaMemory );	

            pDrvCtx->lpvFixedBufferMemory       = (pVoid)MciOsal_GetVirtualAddress(pDrvCtx->hDmaMemory);
            pDrvCtx->lpvPhysFixedBufferMemory   = (pVoid)MciOsal_GetPhysicalAddress(pDrvCtx->hDmaMemory);
        }
    }
 
	
    // Initialize the Message descriptors and buffers
    uiDescBufLen    = ((sizeof(MCI_BUF_DESC) + pDrvCtx->uiMaxMessageBufferSize + 15)/16 + 1) * 16;
    uiDescNum       = pDrvCtx->uiMessageBufferDescNum;

    for (i=0; i < uiDescNum; i++)
    {
        pBufDesc    = (PMCI_BUF_DESC)((ULong)pDrvCtx->lpvFixedBufferMemory + uiCurOffset);
        uiCurOffset+= sizeof(MCI_BUF_DESC);

        MCI_BUF_DESC_INIT(pBufDesc);
        MCI_BUF_DESC_SET_VA_ADDR(pBufDesc, (ULong)pDrvCtx->lpvFixedBufferMemory + uiCurOffset);
        MCI_BUF_DESC_SET_PA_ADDR(pBufDesc, (ULong)pDrvCtx->lpvPhysFixedBufferMemory + uiCurOffset);
        MCI_BUF_DESC_SET_INDEX(pBufDesc, i);

        // Insert the descriptor into Free Message Queue
        MciQueuePushTail(&pDrvCtx->QFreeMessageDesc, (PMCI_QITEM)pBufDesc, NULL, NULL);

        uiCurOffset += (uiDescBufLen - sizeof(MCI_BUF_DESC));
    }

    // Initialize the Stream descriptors and buffers
    uiDescBufLen    = ((sizeof(MCI_BUF_DESC) + pDrvCtx->uiMaxStreamBufferSize + 15)/16 + 1) * 16;
    uiDescNum       = (MCI_DRV_TOTAL_SIZE_OF_BUFFER_MEMORY - uiCurOffset) / uiDescBufLen;

    for (i=0; i < uiDescNum; i++)
    {
        pBufDesc    = (PMCI_BUF_DESC)((ULong)pDrvCtx->lpvFixedBufferMemory + uiCurOffset);
        uiCurOffset+= sizeof(MCI_BUF_DESC);
        if (pDrvCtx->uiMtvType != EMTV_TYPE_DVBH)
        {
            uiCurOffset += 2;
        }
        MCI_BUF_DESC_INIT(pBufDesc);
        MCI_BUF_DESC_SET_VA_ADDR(pBufDesc, (ULong)pDrvCtx->lpvFixedBufferMemory + uiCurOffset);
        MCI_BUF_DESC_SET_PA_ADDR(pBufDesc, (ULong)pDrvCtx->lpvPhysFixedBufferMemory + uiCurOffset);
        MCI_BUF_DESC_SET_INDEX(pBufDesc, i);

        // Insert the descriptor into Free Message Queue
        MciQueuePushTail(&pDrvCtx->QFreeStreamDesc, (PMCI_QITEM)pBufDesc, NULL, NULL);

        uiCurOffset += (uiDescBufLen - sizeof(MCI_BUF_DESC));

        if (pDrvCtx->uiMtvType != EMTV_TYPE_DVBH)
        {
            uiCurOffset -= 2;
        }
    }

    return TRUE;
}

BOOL DrvCreateEventHandles(PMCI_DRIVER pDrvCtx)
{
    BOOL    bRet = FALSE;

    RETAILMSG(_MCI_DBG_TRACE_FUNC, (TEXT("[MCIDRV] +DrvCreateEventHandles\r\n")));

    do
    {
        pDrvCtx->hMciNotifyReceivedTsStreamEvent = MciOsal_CreateEventObject(MCI_EVENT_NAME_TS_STREAM_RECEIVE, TRUE, TRUE);
        if (pDrvCtx->hMciNotifyReceivedTsStreamEvent == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't create the event for TS Stream Recevie Notification\r\n")));
            break;
        }

        pDrvCtx->hMciNotifyReceivedIpStreamEvent = MciOsal_CreateEventObject(MCI_EVENT_NAME_IP_STREAM_RECEIVE, TRUE, TRUE);
        if (pDrvCtx->hMciNotifyReceivedIpStreamEvent == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't create the event for IP Stream Recevie Notification\r\n")));
            break;
        }
        // Set default event for stream notification
        if (pDrvCtx->uiMtvType == EMTV_TYPE_DVBH)
        {
            pDrvCtx->hMciNotifyReceivedStreamEvent = pDrvCtx->hMciNotifyReceivedIpStreamEvent;
        }
        else
        {
            pDrvCtx->hMciNotifyReceivedStreamEvent = pDrvCtx->hMciNotifyReceivedTsStreamEvent;
        }

        pDrvCtx->hMciNotifyReceivedMessageEvent = MciOsal_CreateEventObject(MCI_EVENT_NAME_MIS_MESSAGE_RECEIVE, TRUE, TRUE);
        if (pDrvCtx->hMciNotifyReceivedMessageEvent == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't create the event for MIS Message Recevie Notification\r\n")));
            break;
        }

        pDrvCtx->hMciNotifyStatusChangeEvent = MciOsal_CreateEventObject(MCI_EVENT_NAME_DRIVER_STATE_CHANGE, TRUE, TRUE);
        if (pDrvCtx->hMciNotifyStatusChangeEvent == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't create the event for Driver State Notification\r\n")));
            break;
        }

        pDrvCtx->hClosedAllEvent = MciOsal_CreateEventObject(NULL, FALSE, TRUE);
        if (pDrvCtx->hClosedAllEvent== NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't create the event for Closed all driver Event\r\n")));
            break;
        }

        bRet = TRUE;
    } while (FALSE);

#if 1
    RETAILMSG(1, (TEXT("[MCIDRV] hMciNotifyReceivedTsStreamEvent = 0x%08X\r\n"), (UInt32)pDrvCtx->hMciNotifyReceivedTsStreamEvent));
    RETAILMSG(1, (TEXT("[MCIDRV] hMciNotifyReceivedIpStreamEvent = 0x%08X\r\n"), (UInt32)pDrvCtx->hMciNotifyReceivedIpStreamEvent));
    RETAILMSG(1, (TEXT("[MCIDRV] hMciNotifyReceivedMessageEvent = 0x%08X\r\n"), (UInt32)pDrvCtx->hMciNotifyReceivedMessageEvent));
    RETAILMSG(1, (TEXT("[MCIDRV] hMciNotifyStatusChangeEvent = 0x%08X\r\n"), (UInt32)pDrvCtx->hMciNotifyStatusChangeEvent));
    RETAILMSG(1, (TEXT("[MCIDRV] hClosedAllEvent = 0x%08X\r\n"), (UInt32)pDrvCtx->hClosedAllEvent));
#endif

    return bRet;
}

BOOL DrvNotifyDriverStateToApp(PMCI_DRIVER pDrvCtx, UInt32 uiParam)
{
    BOOL    bRet = FALSE;

    RETAILMSG(1, (TEXT("[MCIDRV] Notify Driver State to App: 0x%08X(0x%08X)\r\n"), (UInt32)pDrvCtx->hMciNotifyStatusChangeEvent, uiParam));
    if (pDrvCtx->hMciNotifyStatusChangeEvent)
    {
        pDrvCtx->uiDriverStateChangeInfo = uiParam;
        MciOsal_SetEventData(pDrvCtx->hMciNotifyStatusChangeEvent, (DWORD)uiParam);
        MciOsal_SetEvent(pDrvCtx->hMciNotifyStatusChangeEvent);

        bRet = TRUE;

        //Added by prabha - 08/24
//	    pDrvCtx->bAttached = TRUE;	
    }
    	
    RETAILMSG(1, (TEXT("[MCIDRV] Done notifying Driver State: %d\r\n"), bRet));

    return bRet;
 
}

BOOL DrvNotifyDriverStateToNetworkDriver(PMCI_DRIVER pDrvCtx, UInt32 uiParam)
{

    BOOL    bRet = FALSE;

    do
    {
        if (bRet == FALSE)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't notify the state of Driver to Network Driver: 0x%08X\r\n"), MciOsal_GetLastError()));
            break;
        }

        bRet = TRUE;
            
    } while (FALSE);

    return bRet;

}

Void DrvSendMessage(PMCI_DRIVER pDrvCtx, pUChar pBuffer, UInt32 uiTransferLen)
{
    PMCI_HW_CONTEXT pHwCtx = &pDrvCtx->MciHwCtx;

    MciOsal_EnterCodeLock(pDrvCtx->hLockHwAccess);
	
    pDrvCtx->lTxStarted++;

    MciHwSendMessage(pHwCtx, pBuffer, uiTransferLen);
    
    //InterlockedDecrement((LPLONG)&pDrvCtx->lTxStarted);
    pDrvCtx->lTxStarted--;
    MciOsal_LeaveCodeLock(pDrvCtx->hLockHwAccess);

        RETAILMSG(1, (TEXT("[MCIDRV] TX Message: [%d]\r\n"), pBuffer[4]));
#if 0

		RETAILMSG(1, (TEXT("               DMB_EN      = %d\r\n"), gpio_get_value(S5PV210_MP04(1))));
        RETAILMSG(1, (TEXT("               DMB_EN_2.8V = %d\r\n"), gpio_get_value(S5PV210_MP04(2))));
        RETAILMSG(1, (TEXT("               DMB_RST     = %d\r\n"), gpio_get_value(S5PV210_GPH3(6))));
        RETAILMSG(1, (TEXT("               SPI_IRQ     = %d\r\n"), MciHwIsInterruptActivated()));
#endif
#if _MCI_DBG_MSG_TX
    {
        UInt32 i;
        RETAILMSG(1, (TEXT("[MCIDRV] TX Message: ")));
        for (i=0; i < uiTransferLen; i++)
        {
            if ((i & 0xF) == 0)             RETAILMSG(1, (TEXT("\r\n\t")));
            RETAILMSG(1, (TEXT(" %02X"), pBuffer[i]));
        }
        RETAILMSG(1, (TEXT("\r\n")));
    }
#endif

}

BOOL DrvDownloadChannelFirmware(PMCI_DRIVER pDrvCtx, PMCI_FWIMG_BUF_INFO pFwImgBufInfo)
{
    BOOL    bRet = FALSE;

    do
    {
        RETAILMSG(DBGON, (TEXT("[MCIDRV] +DrvDownloadChannelFirmware\r\n")));
        if (pFwImgBufInfo == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid F/W Image Information\r\n")));
            break;
        }
        RETAILMSG(DBGON, (TEXT("[MCIDRV] F/W Image Buffer (%d/%d), %d bytes\"\r\n")
                    , pFwImgBufInfo->uiFragmentIndex
                    , pFwImgBufInfo->uiTotalFragmentCount
                    , pFwImgBufInfo->uiBufSize));

        if (pFwImgBufInfo->uiFragmentIndex == 0)
        {
            // Reset if channel is already running.
         	MciHwResetChannelHardware(&pDrvCtx->MciHwCtx);
            MciHwStartChannelHardware(&pDrvCtx->MciHwCtx);

            RETAILMSG(1, (TEXT("[MCIDRV] Power wake-locked +++ \r\n")));            
            wake_lock(&pDrvCtx->wlock);

#if 0
			{
				UChar			ucNewState = '1';
				struct file*	fp = NULL;
				mm_segment_t	fs;

				fs = get_fs();
				set_fs(KERNEL_DS);

				do 
				{
					fp = filp_open(LTE_ALWAYS_ON_FILE, O_RDWR, 0);
					if (fp == NULL)
					{
						RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Can't open '%s' \r\n"), LTE_ALWAYS_ON_FILE));
						break;
					}
					vfs_read(fp, &pDrvCtx->ucSavedLteWakeonState, 1, &fp->f_pos);
					filp_close(fp, NULL);

					fp = filp_open(LTE_ALWAYS_ON_FILE, O_RDWR, 0);
					vfs_write(fp, &ucNewState, 1, &fp->f_pos);

					RETAILMSG(1, (TEXT("[MCIDRV] Make LteAlwasyOn state to '1' (prev = '%c')\r\n"), pDrvCtx->ucSavedLteWakeonState));
				} while (FALSE);
				
				if (fp) filp_close(fp, NULL);
				set_fs(fs);
			}
#endif
        
			if (FALSE == MciHwPreFirmwareImageDownload(&pDrvCtx->MciHwCtx))
            {
                break;
            }
        }

	    RETAILMSG(DBGON, (TEXT("[MCIDRV] F/W Image Buffer (%d), %d bytes\"\r\n")
                    , pFwImgBufInfo->uiAddrOffset
                    , pFwImgBufInfo->uiBufSize
                  ));
        bRet = MciHwFirmwareImageDownload( &pDrvCtx->MciHwCtx
                                         , pFwImgBufInfo->uiAddrOffset
                                         , &pFwImgBufInfo->ucBuffer[0]
                                         , pFwImgBufInfo->uiBufSize);

        if (bRet == FALSE)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] F/W Download Failed(%d/%d) failed\r\n"), pFwImgBufInfo->uiFragmentIndex, pFwImgBufInfo->uiTotalFragmentCount));
            break;
        }
        if (pFwImgBufInfo->uiFragmentIndex == (pFwImgBufInfo->uiTotalFragmentCount - 1))
        {
            MciHwPostFirmwareImageDownload(&pDrvCtx->MciHwCtx);

            pDrvCtx->uiDrvState = EMCI_DRV_READY;
        }
        bRet = TRUE;
    } while (FALSE);


    RETAILMSG(DBGON, (TEXT("[MCIDRV] -DrvDownloadChannelFirmware\r\n")));
    
    return bRet;
}

BOOL DrvDownloadFirmwareForRecovery(PMCI_DRIVER pDrvCtx, PMCI_FWIMG_BUF_INFO pFwImgBufInfo)
{
    BOOL    bRet = FALSE;

    do
    {
        RETAILMSG(DBGON, (TEXT("[MCIDRV] +DrvDownloadFirmwareForRecovery\r\n")));
        if (pFwImgBufInfo == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Invalid F/W Image Information\r\n")));
            break;
        }
        RETAILMSG(DBGON, (TEXT("[MCIDRV] F/W Image Buffer (%d/%d), %d bytes\"\r\n")
                    , pFwImgBufInfo->uiFragmentIndex
                    , pFwImgBufInfo->uiTotalFragmentCount
                    , pFwImgBufInfo->uiBufSize));

        if (pFwImgBufInfo->uiFragmentIndex == 0)
        {
			extern Void SpiHwPrepareRecoveryMode(PMCI_HW_CONTEXT pHwCtx);
			SpiHwPrepareRecoveryMode(&pDrvCtx->MciHwCtx);
        }

        RETAILMSG(DBGON, (TEXT("[MCIDRV] F/W Image Buffer (%d), %d bytes\"\r\n")
                    , pFwImgBufInfo->uiAddrOffset
                    , pFwImgBufInfo->uiBufSize
                  ));
        bRet = MciHwFirmwareImageDownload( &pDrvCtx->MciHwCtx
                                         , pFwImgBufInfo->uiAddrOffset
                                         , &pFwImgBufInfo->ucBuffer[0]
                                         , pFwImgBufInfo->uiBufSize);

        if (bRet == FALSE)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] F/W Download Failed(%d/%d) failed\r\n"), pFwImgBufInfo->uiFragmentIndex, pFwImgBufInfo->uiTotalFragmentCount));
            break;
        }
        if (pFwImgBufInfo->uiFragmentIndex == (pFwImgBufInfo->uiTotalFragmentCount - 1))
        {
            MciHwPostFirmwareImageDownload(&pDrvCtx->MciHwCtx);

            pDrvCtx->uiDrvState = EMCI_DRV_READY;
        }
        bRet = TRUE;
    } while (FALSE);

    RETAILMSG(DBGON, (TEXT("[MCIDRV] -DrvDownloadChannelFirmware\r\n")));
    
    return bRet;
}

BOOL DrvSetDriverConfiguration(PMCI_DRIVER pDrvCtx, PMCI_DRIVER_CONFIG pDrvConfig)
{
	Bool bRet = FALSE;

    pDrvCtx->uiMtvType              = pDrvConfig->uiMtvType;
    pDrvCtx->uiTypeFreqScan         = pDrvConfig->uiMsgFreqScanType;
    pDrvCtx->uiTypeCreateFilter     = pDrvConfig->uiMsgCreateFilterType;
    pDrvCtx->uiTypeDeleteFilter     = pDrvConfig->uiMsgDeleteFilterType;
    pDrvCtx->uiTypeStatus           = pDrvConfig->uiMsgStatusType;
    pDrvCtx->uiTypeBootSuccess      = pDrvConfig->uiMsgBootSuccessType;
    pDrvCtx->uiTypeStreamData       = pDrvConfig->uiMsgStreamDataType;
    pDrvCtx->uiBufferRetryCount     = pDrvConfig->uiBufferRetryCount;
    pDrvCtx->bSyncAccessForMessage  = pDrvConfig->bSyncAccessForMessage;
    pDrvCtx->bSyncAccessForState    = pDrvConfig->bSyncAccessForState;
    pDrvCtx->bSyncAccessForStream   = pDrvConfig->bSyncAccessForStream;
    pDrvCtx->uiWaitTimeForStream    = pDrvConfig->uiWaitTimeForStream;
    pDrvCtx->uiWaitTimeForMessage   = pDrvConfig->uiWaitTimeForMessage;
    pDrvCtx->uiWaitTimeForStateChange = pDrvConfig->uiWaitTimeForStateChange;

    pDrvCtx->uiMaxMessageBufferSize = (pDrvConfig->uiMaxMessageBufferSize != 0) ? pDrvConfig->uiMaxMessageBufferSize : MCI_DRV_MAX_MESSAGE_BUF_SIZE;
    pDrvCtx->uiMaxStreamBufferSize  = (pDrvConfig->uiMaxStreamBufferSize != 0) ? pDrvConfig->uiMaxStreamBufferSize : MCI_DRV_MAX_STREAM_BUF_SIZE;
    pDrvCtx->uiMessageBufferDescNum = (pDrvConfig->uiMessageBufferCount != 0) ? pDrvConfig->uiMessageBufferCount : MCI_DRV_NUM_OF_BUF_DESC;

    if (pDrvConfig->uiInterruptHandlerThreadPriority == 0)
    {
        MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_RECV_THREAD_PRIORITY, 101);
    }
    else
    {
        MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_RECV_THREAD_PRIORITY, pDrvConfig->uiInterruptHandlerThreadPriority);
    }

    if (pDrvCtx->uiMciType == EMCI_TYPE_SDIO)
    {
        
        RETAILMSG(1, (TEXT("[!MCIDRV] Rxbase address = 0x%08X, TX base address = 0x%08X\r\n"),pDrvConfig->uiSdioRxBaseAddress,pDrvConfig->uiSdioTxBaseAddress));
        MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SDIO_RX_ADDR, pDrvConfig->uiSdioRxBaseAddress);
        MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SDIO_TX_ADDR, pDrvConfig->uiSdioTxBaseAddress);
        MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SDIO_ENABLE_MULTI_BLK_TRANSFER, pDrvConfig->bSdioEnableMultiBlockTransfer);
    }

    /*Added by Charlie for the set the SPI Prescale Value*/
    // Set Clock Configuration
    // SPI_CLKSEL      => b'10 : EPLL clock
    // ENCLK           => b'1  : Enable
    // SPI_SCALER      => 32   : 66/(2*(32+1) => 1MHz    

	switch (pDrvCtx->uiMtvType)
	{
        case EMTV_TYPE_DVBH:
    	    MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 3); 
            MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_DMA, FALSE);  //DVB-H not using DMA 
            break;

    	case EMTV_TYPE_DVBT:
    		 MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 2); break;

    	case EMTV_TYPE_TDMB:
    		MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 4); break;

    	case EMTV_TYPE_ISDBT:
    		MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 3); break;

    	case EMTV_TYPE_ATSC_MH:
			 MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 3); break;	/// origin code
    		 //MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 4); break;	/// test fix code by 김승한 책임  2010.03.15
             /// SPI Clock을 작게 만드려면 여기서 prescale값을 크게 만들어야 한다...    2010-12-28    

    	case EMTV_TYPE_CMMB: //sizkim In case of CMMB Chip 1 => 16.6Mhz
    		MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 1); break; //In case of CMMB FPGA Test(5Mhz) 10 => 6Mhz

    	default:
    		MciHwSetDeviceCapability(&pDrvCtx->MciHwCtx, EDRV_HW_CAPS_SPI_PRESCALE, 4); break;
	}

#if 0    
    printk("[MCIDRV] pDrvCtx->uiMtvType             : %d\n", pDrvCtx->uiMtvType);
    printk("[MCIDRV] pDrvCtx->uiTypeFreqScan        : %d\n", pDrvCtx->uiTypeFreqScan);
    printk("[MCIDRV] pDrvCtx->uiTypeCreateFilter    : %d\n", pDrvCtx->uiTypeCreateFilter);
    printk("[MCIDRV] pDrvCtx->uiTypeDeleteFilter    : %d\n", pDrvCtx->uiTypeDeleteFilter);
    printk("[MCIDRV] pDrvCtx->uiTypeStatus          : %d\n", pDrvCtx->uiTypeStatus);
    printk("[MCIDRV] pDrvCtx->uiTypeBootSuccess     : %d\n", pDrvCtx->uiTypeBootSuccess);
    printk("[MCIDRV] pDrvCtx->uiTypeStreamData      : %d\n", pDrvCtx->uiTypeStreamData);
    printk("[MCIDRV] pDrvCtx->uiBufferRetryCount    : %d\n", pDrvCtx->uiBufferRetryCount);
    printk("[MCIDRV] pDrvCtx->bSyncAccessForMessage : %d\n", pDrvCtx->bSyncAccessForMessage);
    printk("[MCIDRV] pDrvCtx->bSyncAccessForState   : %d\n", pDrvCtx->bSyncAccessForState);
    printk("[MCIDRV] pDrvCtx->bSyncAccessForStream  : %d\n", pDrvCtx->bSyncAccessForStream);
    printk("[MCIDRV] pDrvCtx->uiMaxMessageBufferSize  : %d\n", pDrvCtx->uiMaxMessageBufferSize);
    printk("[MCIDRV] pDrvCtx->uiMaxStreamBufferSize  : %d\n", pDrvCtx->uiMaxStreamBufferSize);
    printk("[MCIDRV] pDrvCtx->uiMessageBufferDescNum  : %d\n", pDrvCtx->uiMessageBufferDescNum);
    printk("[MCIDRV] pDrvCtx->MciHwCtx.uispiprescale : %d\n", pDrvCtx->MciHwCtx.uispiprescale);                
#endif
    do
    {
        pDrvCtx->lRxStarted = 0;
        pDrvCtx->lTxStarted = 0;
    
        if (pDrvCtx->uiMtvType == EMTV_TYPE_DVBH)
        {
            pDrvCtx->hMciNotifyReceivedStreamEvent = pDrvCtx->hMciNotifyReceivedIpStreamEvent;
        }
        else
        {
            pDrvCtx->hMciNotifyReceivedStreamEvent = pDrvCtx->hMciNotifyReceivedTsStreamEvent;
        }
    
        MCI_QUEUE_INIT(&pDrvCtx->QFreeMessageDesc);
        MCI_QUEUE_INIT(&pDrvCtx->QReceivedMessageDesc);
        MCI_QUEUE_INIT(&pDrvCtx->QFreeStreamDesc);
        MCI_QUEUE_INIT(&pDrvCtx->QReceivedStreamDesc);
        MCI_QUEUE_INIT(&pDrvCtx->QTxReqMessageDesc);

        DrvInitializeBuffers(pDrvCtx, FALSE);

        if (pDrvConfig->bNotifyAttachEvent)
        {
            printk("[MCIDRV]  Entering DrvNotifyDriverStateToApp############\r\n");

            if ( MciHwGetHwInterfaceType(&pDrvCtx->MciHwCtx) == EMCI_TYPE_SPI)
            {
                 if (FALSE == DrvNotifyDriverStateToApp(pDrvCtx, (EMCI_TYPE_SPI<<24) + EMCI_EVENT_CARD_ATTACHED))
                 {
                     break;
                 }
            }
            
        }
        bRet = TRUE;
    } while(FALSE);
    
	return bRet;
}

Bool DrvCheckTxRequestAndSend(PMCI_DRIVER pDrvCtx)
{
    Bool            bRet = FALSE;
    PMCI_BUF_DESC   pBufDesc = NULL;
    PMIS_MSG_HDR    pMsgBuf = NULL;
    UInt32          uiTransferLen = 0;

    pBufDesc = (PMCI_BUF_DESC)MciQueuePopHead(&pDrvCtx->QTxReqMessageDesc, pDrvCtx->hLockMessageDescQueue, NULL);
    if (pBufDesc)
    {
        pMsgBuf = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc);
        uiTransferLen = pMsgBuf->usLen + MIS_MSG_HDR_LEN + 1;
        if (uiTransferLen & 0x1)
        {   // align to half-word
            uiTransferLen++;
        }
        RETAILMSG(_MCI_DBG_MSG_TX, (TEXT("[MCIDRV] Send Request Message in IRQ\r\n")));
        DrvSendMessage(pDrvCtx, (pUChar)pMsgBuf, uiTransferLen);
        MciQueuePushTail(&pDrvCtx->QFreeMessageDesc, (PMCI_QITEM)pBufDesc, pDrvCtx->hLockMessageDescQueue, NULL);
        bRet = TRUE;
    }

    return bRet;
}



PMCI_BUF_DESC DrvPreprocessMessage(PMCI_DRIVER pDrvCtx, PMCI_BUF_DESC pBufDesc)
{
    PMIS_MSG_HDR    pMsgHdr;

    if (pBufDesc == NULL)
    {
        return pBufDesc;
    }

    pMsgHdr = (PMIS_MSG_HDR)MCI_BUF_DESC_GET_MSG_BUF(pBufDesc);

    if (pMsgHdr->ucType == (UInt8)pDrvCtx->uiTypeBootSuccess)
    {
#if 1
        RETAILMSG(1, (TEXT("[MCIDRV] Receive the message for Boot Success from Link F/W\r\n")));

        DrvNotifyDriverStateToApp(pDrvCtx, ((pDrvCtx->uiMciType)<<24) + EMCI_EVENT_BOOT_SUCCESS_RECEIVED);
#else
	extern Void SpiHwRecoveryFromReset(PMCI_HW_CONTEXT pHwCtx);
	SpiHwRecoveryFromReset(&pDrvCtx->MciHwCtx);
#endif

    }

    {

        RETAILMSG(1, (TEXT("[MCIDRV] Received Msg Type=%02X, Status=%02X\r\n"), pMsgHdr->ucType, pMsgHdr->ucData[0]));
#if _MCI_DBG_MSG_RX		
        UInt32 i;

        RETAILMSG(1, (TEXT("[MCIDRV] Rx Message Info\r\n")));
        RETAILMSG(1, (TEXT("         Type = %02X\r\n"), pMsgHdr->ucType));
        RETAILMSG(1, (TEXT("         Length = %d"), pMsgHdr->usLen));
        for (i=0; i <= pMsgHdr->usLen; i++)
        {
            if ((i%16)==0) RETAILMSG(1, (TEXT("\r\n         ")));
            RETAILMSG(1, (TEXT("%02X "), pMsgHdr->ucData[i]));
        }
        RETAILMSG(1, (TEXT("\r\n")));
#endif

		if (pMsgHdr->ucType==0x1 && pMsgHdr->ucData[0]!=0x2e && pMsgHdr->ucData[0]!=0x2f)
		{
        	RETAILMSG(1, (TEXT("[MCIDRV] Notified CMD = %d\r\n"), pMsgHdr->ucData[0]));
		}
    }
    return pBufDesc;
}
