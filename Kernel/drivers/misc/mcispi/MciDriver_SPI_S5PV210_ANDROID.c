/* drivers/misc/mcispi/MciDriver_SPI_S5PV210_ANDROID.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciDriver_SPI_S5PV210_ANDROID.c
**
**  Description
**      Define S5PV210 SPI Interface operation and constants
**      (P1 Limo)
**  Authur:
**      Charlie Park (jongho0910.park@samsung.com)
**
**  Update:
**      2009.12.09  NEW     Created.
**/

#include <linux/interrupt.h>    /*request_irq(), enable_irq(), disable_irq(), free_irq()*/
#include <asm/io.h>             /*readl(), writel(), ioremap(), ioremap_nocache(), iounmap()*/
#include <linux/string.h>       /*memset, memcpy*/
#include <linux/module.h>       /*moule_init(), module_exit()*/
#include <linux/fs.h>           /*register_chrdev(), unregister_chrdev()*/
#include <linux/slab.h>         /*kmalloc(), kfree()*/
#include <linux/dma-mapping.h>  /*dma_alloc_coherent(), dma_free_coherent()*/
//#include <asm-arm/semaphore.h>  /*sema_init(), up(), down()*/
#include <linux/semaphore.h>    /*sema_init(), up(), down() ==> for Android BSP 2.6.27 */
#include <linux/wait.h>         /*wake_up(), wait_event_interruptible(), wake_up_interruptible(), down_interruptible(), init_waitqueue_head()*/
#include <linux/kernel.h>       /*container_of(), printk(), sprintf()*/
#include <linux/jiffies.h>      /*get_jiffies_64()*/
#include <linux/delay.h>        /*mdelay(), udelay()*/
//#include <mach/s3c-dma.h>       /*s3c2410_dma_request(), s3c2410_dma_set_buffdone_fn(),s3c2410_dma_set_opfn(),s3c2410_dma_free(),s3c2410_dma_devconfig(),s3c2410_dma_config(),s3c2410_dma_setflags(),s3c2410_dma_enqueue() */
//#include <plat-s5p/dma-pl330.h>       /*s3c2410_dma_request(), s3c2410_dma_set_buffdone_fn(),s3c2410_dma_set_opfn(),s3c2410_dma_free(),s3c2410_dma_devconfig(),s3c2410_dma_config(),s3c2410_dma_setflags(),s3c2410_dma_enqueue() */
//#include <asm/dma.h>
//#include <plat/dma.h>

#include <asm/uaccess.h>        /*copy_to_user(), copy_from_user()*/
#include <linux/irq.h>          /*set_irq_type()*/
#include <linux/spinlock.h>

//#include <plat/regs-gpio.h>
//#include <linux/gpio.h>	// 2010. 12. 15
//#include <mach/regs-gpio.h>	// 2010. 12. 15 - by murphy

//#include <plat/regs-clock.h>
#include <mach/regs-clock.h>	// 2010. 12. 15

#if 0
#include <../drivers/spi/spi_s3c.h>	// 2010. 12. 15 - by murphy
#else
/// modified by tuton for porting Froyo	2010.12.16

#include <mach/dma.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
//#include <plat/spi.h>

// 2011. 6. 7 by murphy
#include <mach/gpio-bank.h>
//#include <mach/gpio-bank-b.h>
//#include <mach/gpio-bank-g2.h>
//#include <mach/gpio-bank-g3.h>

#endif



#include "MciOsalApi.h"
#include "MciCommon.h"
#include "MciDriver_HW.h"
#include "MciDriver.h"

struct s3c2410_dma_client s5pc110spi_dma_client = {
  .name   = "s5pc110-dma",
};

Void __iomem    *gpSpiRegs;
volatile UInt32 m_uiSavedSpiModeRegisterValue;
volatile UInt32 fMciDrvDMA = 1;

UChar  gucRecoveryImage[0x28] = {0x20, 0x00, 0x00, 0x00, 0xaf, 0xbf, 0x2e, 0x2a, 0x18, 0xf0, 0x9f, 0xe5, 0x18, 0xf0, 0x9f, 0xe5,
                                 0x18, 0xf0, 0x9f, 0xe5, 0x18, 0xf0, 0x9f, 0xe5, 0x18, 0xf0, 0x9f, 0xe5, 0xfe, 0xff, 0xff, 0xea, 
                                 0x14, 0xf0, 0x9f, 0xe5, 0x14, 0xf0, 0x9f, 0xe5};

//DECLARE_WAIT_QUEUE_HEAD(MciDMACompleted);

//
// Static functions
// 
Void    MciHwInitializeDeviceCapabilities(PMCI_HW_CONTEXT pHwCtx);
Void    SpiHwInitRegister(void);
Void    SpiHwDmaInterruptDoneHandler(struct s3c2410_dma_chan *dma_ch, void *buf_id, Int32 size, enum s3c2410_dma_buffresult result);
UInt16  SpiHwWriteHWord(PMCI_HW_CONTEXT pHwCtx, UInt16 usData);
UInt16  SpiHwReadHWord(PMCI_HW_CONTEXT pHwCtx);
Void    SpiHwLoopDelay(PMCI_HW_CONTEXT pHwCtx, UInt32 uiDelayCnt);
Void    SpiHwClearTxRxFifo(PMCI_HW_CONTEXT pHwCtx);
Void    SpiHwSetChipSelector(PMCI_HW_CONTEXT pHwCtx, ESPI_CS csValue);
Void    SpiHwSetRxTransferMode(PMCI_HW_CONTEXT pHwCtx, Bool bOn, UInt32 uiTransferLen);
Void    SpiHwSetRxDmaTransferMode(PMCI_HW_CONTEXT pHwCtx, Bool bOn, UInt32 uiTransferLen);
Void    SpiHwSetTxTransferMode(PMCI_HW_CONTEXT pHwCtx, Bool bOn);
Int32 	SpiHwDmaConfig( PMCI_HW_CONTEXT pHwCtx, Int32 mode);
Void    SpiHwRecoveryFromReset(PMCI_HW_CONTEXT pHwCtx);
Void    SpiHwPrepareRecoveryMode(PMCI_HW_CONTEXT pHwCtx);
extern  ERRORCODE DrvHandlePendedSpiIrqInterrupt(PMCI_DRIVER pDrvCtx);


Bool MciHwInitialize(PMCI_HW_CONTEXT pHwCtx, UInt32 uiContext, pVoid lpBusContext)
{
    Bool        bRet = FALSE;
    UInt32      ret;

    do
    {
        MciHwInitializeDeviceCapabilities(pHwCtx);

        /*S3C6400_PA_SPI : 0x7F00C000(SPI PhyAddress)  S3C24XX_SZ_SPI : 0x000010000(4K) */
//        regs = ioremap(S3C6400_PA_SPI, S3C24XX_SZ_SPI + 1);
//        printk("[MCIDRV] S3C6400_PA_SPI =%08X\r\n", S3C6400_PA_SPI);

//        regs = MciOsal_MemoryMapIoSpace(S3C6400_PA_SPI, S3C24XX_SZ_SPI + 1, NULL);

//        regs = MciOsal_MemoryMapIoSpace(S3C6410_PA_SPI, S3C24XX_SZ_SPI + 1, NULL);
//        printk("[MCIDRV] S5PC100_PA_SPI : %08X\n", S5PC100_PA_SPI);

        gpSpiRegs = MciOsal_MemoryMapIoSpace((pVoid)S5PC110_PA_SPI, S5PC110_SIZE_SPI , FALSE);


        RETAILMSG(1, (TEXT("[MCIDRV] Mapped regs =%x\r\n"), (UInt32)gpSpiRegs));
          
        if ( gpSpiRegs == NULL ) 
        {
            RETAILMSG(1, (TEXT("[ERROR-MCIDRV] Can't map IO\n")));
            ret = -ENXIO;
        }

        SpiHwInitRegister();

        MciHwPowerOnChannel(pHwCtx);

        /*Initialize the DMA*/


	/// Modified by tuton	2010.12.15	porting Froyo
        //pHwCtx->gDmaSubChannelRx = DMACH_SPI1_IN; //DMACH_SPI1_IN; DMACH_SPI0_IN
        pHwCtx->gDmaSubChannelRx = DMACH_SPI1_RX;	//DMACH_SPIIN_1; //DMACH_SPI1_IN; DMACH_SPI0_IN

        if (s3c2410_dma_request( pHwCtx->gDmaSubChannelRx, &s5pc110spi_dma_client, NULL)) 
        {
            RETAILMSG(1, (TEXT("[ERROR-MCIDRV] cannot get DMA channel \n")));
            ret = -EBUSY;
        }

        s3c2410_dma_set_buffdone_fn( pHwCtx->gDmaSubChannelRx, SpiHwDmaInterruptDoneHandler);

        //s3c2410_dma_set_opfn( pHwCtx->gDmaSubChannelRx,  NULL);

    bRet = TRUE;

    } while (FALSE);

    return bRet;
}
Void MciHwRemoveInstance(PMCI_HW_CONTEXT pHwCtx)
{

    PMCI_DRIVER pMCI_Drv = container_of(pHwCtx, struct _MCI_DRIVER, MciHwCtx);

    pHwCtx->uiDevState = EDEV_STATE_SHUTDOWN_IN_PROGRESS;
       
    if (pHwCtx->bIRQRegister)
    {

        printk("[MCIDRV] MciHwRegisterInterruptHandle pContext : %08X , MCI_SPI_IRQ : %d\n", (UInt32)pMCI_Drv, MCI_SPI_INT);

        disable_irq(MCI_SPI_INT);
        free_irq(MCI_SPI_INT , pMCI_Drv);
        pHwCtx->bIRQRegister = FALSE;
    }

    s3c2410_dma_free(pHwCtx->gDmaSubChannelRx, &s5pc110spi_dma_client);

    RETAILMSG(1, (TEXT("[MCIDRV] UnMapped regs =%x\r\n"), (UInt32)gpSpiRegs));

    MciOsal_MemoryUnmapIoSpace(gpSpiRegs, 0);
/*
    if (regs !=NULL)
    {
        iounmap(regs);
        regs = NULL;
    }
*/
}


ERRORCODE MciHwRecvMessageHeader(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, UInt32 uiRecvLen)
{
    ERRORCODE   eRet = EMCI_FAIL;
    UInt32      i;
    UChar       ucData[2];
    Bool        bTimeover = FALSE;
    UInt32      val =0;
    UInt16	uiData;


	if(gpio_get_value(S5PV210_GPH2(5)) != MCI_INTR_ACTIVE)
	{
		RETAILMSG(1, (TEXT("[ERROR!MCISPI:%d] Invalid Interrupt\r\n"), MciOsal_GetTickCount()));
		return eRet;
	}

    // Send MIS ACK Bytes(0x0006)
#if _MCI_CHECK_INTR_IN_RCV_
    SpiHwSetTxTransferMode(pHwCtx, TRUE);
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);

    SpiHwWriteHWord(pHwCtx, MCI_SPI_ACK_BYTES);
    SpiHwLoopDelay(pHwCtx, 200);

    SpiHwSetTxTransferMode(pHwCtx, FALSE);
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);

    SpiHwLoopDelay(pHwCtx, 200);
#else
	i=0;
	do
	{
		unsigned int j = 0;

    	SpiHwSetTxTransferMode(pHwCtx, TRUE);
    	SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
	
    	uiData = SpiHwWriteHWord(pHwCtx, MCI_SPI_ACK_BYTES);
    	SpiHwLoopDelay(pHwCtx, 200);
	
    	SpiHwSetTxTransferMode(pHwCtx, FALSE);
    	SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);

		j=0;
		while (gpio_get_value(S5PV210_GPH2(5)) == MCI_INTR_ACTIVE)
		{
			if (j++ > 50)
			{
				i++;
				break;
			}
		}
		if (gpio_get_value(S5PV210_GPH2(5)) != MCI_INTR_ACTIVE)
		{
			break;
		}
		
		RETAILMSG(1, (TEXT("[ERROR!MCISPI] Read Data after sending ACK bytes: 0x%04X\r\n"), uiData));
		if (i > 1)
		{
			UChar ucTempBuf[10];
			RETAILMSG(1, (TEXT("[ERROR!MCISPI] Couldn't be handled ACK bytes by F/W\r\n")));
			MciHwRecvMessageBody(pHwCtx, ucTempBuf, 10);
			for (i=0; i < 10; i++)
			{
				RETAILMSG(1, (TEXT("%02X "), ucTempBuf[i]));
			}
			RETAILMSG(1, (TEXT("\r\n")));
			return -2;
		}
	} while (1);
#endif

    // Find STX
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);

    for (i=0; i <= 400; i++)
    {
        SpiHwSetRxTransferMode(pHwCtx, TRUE, 2);
        *(pUInt16)ucData = SpiHwReadHWord(pHwCtx);
        SpiHwSetRxTransferMode(pHwCtx, FALSE, 2);
        if (ucData[0] == MIS_MSG_STX && ucData[1] != 0)
        {
            break;
        }
		else if (*(pUInt16)ucData == 0xFFFF || *(pUInt16)ucData == 0x0)
		{
			RETAILMSG(1, (TEXT("[MCISPI] Invalid STX: %02X %02X(Break)\r\n"), ucData[0], ucData[1]));
			SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);
			return -2;
		}
        
		RETAILMSG(1, (TEXT("[MCISPI] Invalid STX: %02X %02X\r\n"), ucData[0], ucData[1]));

#if _MCI_CHECK_INTR_IN_RCV_        
    	if(gpio_get_value(S5PV210_GPH2(5)) != MCI_INTR_ACTIVE)   // CHECK FALLING EGDE
    	{
    		RETAILMSG(1, (TEXT("[ERROR!MCISPI:%d] Invalid Interrupt\r\n"), MciOsal_GetTickCount()));
        	SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);
    		return eRet;
    	}
#endif        

		if(i >= 200)
		{
            bTimeover = TRUE;
            break;
		}
    }

	if(bTimeover) 
	{
		//Cc_Spi_nCS(1);				// nCS To High
		RETAILMSG(1,(TEXT("[ERROR!MCISPI] Can't find the STX of Message. \r\n")));
        SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);
        return eRet;
	}

    pBuffer[0]  = ucData[0];
    pBuffer[1]  = ucData[1];
	
    SpiHwSetRxTransferMode(pHwCtx, TRUE, 2);
    *(pUInt16)&pBuffer[2] = SpiHwReadHWord(pHwCtx);
    SpiHwSetRxTransferMode(pHwCtx, FALSE, 2);
    
    eRet = EMCI_OK;
    
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);
    RETAILMSG(0, (TEXT("[MCISPI] %02X %02X, Len = %d \r\n"), pBuffer[0], pBuffer[1], *(pUInt16)&pBuffer[2]));
    return eRet;
}


ERRORCODE MciHwRecvMessageBody(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, UInt32 uiRecvLen)
{
    UInt32      i;

//    printk("[Charlie] Test!!\n");
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
    SpiHwSetRxTransferMode(pHwCtx, TRUE, uiRecvLen);
    for (i=0; i < uiRecvLen; i = i + 2)
    {
//        barrier();
        *(pUInt16)&pBuffer[i] = SpiHwReadHWord(pHwCtx);
    }
    SpiHwSetRxTransferMode(pHwCtx, FALSE, uiRecvLen);
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);

#if PRINT_SQ		//201109 by murphy
{
    unsigned int uiIdx=0;
    if(pBuffer[uiIdx]== 0x2e)
    {
		++uiIdx;
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Signal Locked %03d \r\n"), pBuffer[uiIdx++] ));
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t SQ Value %03d \r\n"), pBuffer[uiIdx++] ));
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t First Parade ID %03d \r\n"), pBuffer[uiIdx++] ));
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Second Parade ID %03d \r\n"), pBuffer[uiIdx++] ));
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t SNR %02d.%d \r\n"), pBuffer[uiIdx],pBuffer[uiIdx+1] ));
        uiIdx+=2;
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Primary Ensemble of First parade  (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;

        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Secondary Ensemble of First parade (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;

        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Primary Ensemble of Second parade  (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;

        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Secondary Ensemble of Second parade (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;

        // ´©Àû 
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Coverage for first Parade (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;
        
        RETAILMSG(1,(TEXT("[MCISPI SQ] \t Coverage for Second Parade (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;

        RETAILMSG(1,(TEXT("\n[MCISPI SQ] \t RSSI %02d\r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3])));
        uiIdx+=4;

        RETAILMSG(1,(TEXT("[MCISPI SQ] \t TPC Recv Info (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;

        RETAILMSG(1,(TEXT("[MCISPI SQ] \t FIC Recv Info (%09d/%09d) \r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3]),\
            (pBuffer[uiIdx+4]<<24 | pBuffer[uiIdx+5]<<16 | pBuffer[uiIdx+6]<<8 | pBuffer[uiIdx+7]) ));
        uiIdx +=8;

        RETAILMSG(1,(TEXT("\n[MCISPI SQ] \t BER %02d\r\n"), \
            (pBuffer[uiIdx]<<24 | pBuffer[uiIdx+1]<<16 | pBuffer[uiIdx+2]<<8 | pBuffer[uiIdx+3])));
        uiIdx+=4;

    }else     if(pBuffer[uiIdx]== 0x81)	// debugging message
    {
//		guApCmdRes[0] = DBG_REQ_MH_TP;		// not used : 0x81

//		guApCmdRes[1] = g_MonitoringValue.uiLocked;
//		guApCmdRes[2] = g_MonitoringValue.nRSSI;
//		guApCmdRes[3] = g_MonitoringValue.sSNR/100;
//		guApCmdRes[4] = gPhyFlag.resync;
//		guApCmdRes[5] = gPhyFlag.addingSvc;
//		guApCmdRes[6] = gsPrdInfo_1.enable;
//		guApCmdRes[7] = gsPrdInfo_2.enable;
//		guApCmdRes[8] = g_MonitoringValue.puiTovCounter[0][1];	// parade 0 tov cnt
//		guApCmdRes[9] = g_MonitoringValue.puiTovCounter[1][1];	// parade 1 tov cnt
//		guApCmdRes[10] = g_MonitoringValue.uiBER;
		
		RETAILMSG(1,(TEXT("[MCISPI summary] \t uiLocked %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t nRSSI %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t sSNR %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t addingSvc %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t resync %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t gsPrdInfo_1.enable %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t gsPrdInfo_2.enable %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t puiTovCounter[0][1] %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t puiTovCounter[1][1] %d \r\n"), pBuffer[uiIdx++] ));
		RETAILMSG(1,(TEXT("[MCISPI summary] \t uiBER %d \r\n"), pBuffer[uiIdx++] ));

	}
}    
#endif 
    return EMCI_OK;
}

ERRORCODE MciHwRecvMessageBodyWithDMA(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, pUChar pPaBuffer, UInt32 uiRecvLen)
{
    ERRORCODE   eRet = EMCI_FAIL;

    do
    {
#if _MCI_SUPPORT_DMA
#if _MCI_USE_TEST_TRIGGER    
        UInt32 Gpl7_dat=0;
        Gpl7_dat = readl(S3C_GPLDAT);
        Gpl7_dat |= (0x1<<7);
        writel(Gpl7_dat, S3C_GPLDAT);
        Gpl7_dat &= ~(0x1<<7);
        writel(Gpl7_dat, S3C_GPLDAT);
#endif

        SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);  

        SpiHwSetRxDmaTransferMode(pHwCtx, TRUE, uiRecvLen + 4);


        eRet = SpiHwDmaConfig(pHwCtx, SPI_RX_MODE);    /*initialize the dma*/
        if(eRet !=0)
        {
            RETAILMSG(1, (TEXT("[ERROR-MCIDRV] S5PC110 spi DMA init failed!\n")));
            break;
        }


        eRet = s3c2410_dma_enqueue(pHwCtx->gDmaSubChannelRx, (void*)pHwCtx, (UInt32)pPaBuffer , uiRecvLen + 4); /*start dma queue,may hang on this*/
        if(eRet !=0)
        {
            RETAILMSG(1, (TEXT("[ERROR-MCIDRV] DMA Enqueue failed!\n")));
            break;
        }

        pHwCtx->bDMAOPWait = TRUE;

        eRet = EMCI_OK;

#endif
    }while (FALSE);
    
    return eRet;
}

UInt32 MciHwGetDeviceCapability(PMCI_HW_CONTEXT pHwCtx, EDRV_HW_CAPS capsId)
{
    UInt32 bRet = -1;
    RETAILMSG(1, (TEXT("[MCIDRV] MciHwGetDeviceCapability PMCI_HW_CONTEXT = 0x%08X\r\n"), (UInt32)pHwCtx));

    switch (capsId)
    {
        case EDRV_HW_CAPS_DMA:
            bRet = (UInt32)pHwCtx->bSupportDMA;
            break;
        case EDRV_HW_CAPS_TS:
            bRet = (UInt32)pHwCtx->bSupportTSHW;
            break;
        case EDRV_HW_CAPS_HW_INTF_TYPE:
            bRet = pHwCtx->uiHwIntfType;
            break;
        case EDRV_HW_CAPS_DEVICE_STATE:
            bRet = pHwCtx->uiDevState;
            break;
        default:
            bRet = -1;
            break;
    }

    return bRet;
}

Bool MciHwSetDeviceCapability(PMCI_HW_CONTEXT pHwCtx, EDRV_HW_CAPS capsId, UInt32 uiValue)
{
    Bool bRet = TRUE;
    switch (capsId)
    {
        case EDRV_HW_CAPS_DMA:
            pHwCtx->bSupportDMA     = (Bool)uiValue;
            break;
        case EDRV_HW_CAPS_TS:
            pHwCtx->bSupportTSHW    = (Bool)uiValue;
            break;
        case EDRV_HW_CAPS_SPI_PRESCALE:
            pHwCtx->uispiprescale   = (Bool)uiValue;
        default:
            bRet = FALSE;
            break;
    }

    return bRet;
}

Bool MciHwRegisterInterruptHandle(PMCI_HW_CONTEXT pHwCtx, MCI_DRV_INTR_HANDLER_FUNC fnThread, pVoid pContext)
{

    Int32 ret;
    pHwCtx->bIRQRegister = FALSE;

    ret = set_irq_type(MCI_SPI_INT,IRQ_TYPE_EDGE_RISING);   /* defined in linux/irq.h */     
    if(ret)
    {
        RETAILMSG(1, (TEXT("[ERROR!MCIDRV] IRQ %d set type  error\n"), MCI_SPI_INT));
        return ret;
    }

    printk("[MCIDRV] MciHwRegisterInterruptHandle pContext : %08X , MCI_SPI_IRQ : %d\n", (UInt32)pContext, MCI_SPI_INT);

//    free_irq(MCI_SPI_INT);
    ret = request_threaded_irq(MCI_SPI_INT, NULL, (pVoid)fnThread, IRQF_ONESHOT | IRQ_DISABLED, MCI_SPI_DEV_DRV_NAME, pContext);
//    ret = request_threaded_irq(MCI_SPI_INT, (pVoid)fnThread, NULL, IRQF_SHARED | IRQ_DISABLED, MCI_SPI_DEV_DRV_NAME, pContext);
    if (ret)
    {
		RETAILMSG(1, (TEXT("[ERROR-MCIDRV] ####################################\n")));
        RETAILMSG(1, (TEXT("[ERROR-MCIDRV] IRQ %d request error\n"), MCI_SPI_INT));        
        return false;
    }

    pHwCtx->bIRQRegister = TRUE;
    pHwCtx->bDMAOPWait   = FALSE;


    /*Interrupt Enable*/
    disable_irq(MCI_SPI_INT);
    enable_irq(MCI_SPI_INT);

    ret = TRUE;

    return ret;
}

UInt32 MciHwSendMessage(PMCI_HW_CONTEXT pHwCtx, pUChar pBuffer, UInt32 uiTransferLen)
{

    UInt32 i;
    UInt32 ret = EMCI_OK;

#if _MCI_USE_TEST_TRIGGER
    UInt32 Gpl7_dat=0;
    Gpl7_dat = readl(S3C_GPLDAT);
    Gpl7_dat |= (0x1<<7);
    writel(Gpl7_dat, S3C_GPLDAT);
    Gpl7_dat &= ~(0x1<<7);
    writel(Gpl7_dat, S3C_GPLDAT);
#endif

    SpiHwSetTxTransferMode(pHwCtx, TRUE);
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
    
    for (i=0; i < uiTransferLen; i = i+2)
    {
        SpiHwWriteHWord(pHwCtx, *(pUInt16)&pBuffer[i]);
    }
    
    SpiHwLoopDelay(pHwCtx, 200);
    SpiHwSetTxTransferMode(pHwCtx, FALSE);
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);

    return ret;
}



UInt32 MciHwCheckRxReqIntrLevel(PMCI_HW_CONTEXT pHwCtx)
{
    return 1;
}

Void MciHwResetChannelHardware(PMCI_HW_CONTEXT pHwCtx)
{

    //Mask SPI_IRQ Interrupt
    writel((readl(S5PC11X_EINT2MASK) | (0x01 << 5)), S5PC11X_EINT2MASK);    //XEINT_6
    
    // Reset DMB_RST, DMB_EN , DTV_EN_2.8V signals
    writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 6)), S5PV210_GPH3DAT);     // Set DMB_RST to low
    Sleep(5);

    if (HWREV >= 0x02 && HWREV < 0x07)
    {
        writel((readl(S5PV210_MP04DAT) & ~(0x01 << 1)), S5PV210_MP04DAT);     //Set DMB_EN to low
        Sleep(5);

        writel((readl(S5PV210_MP04DAT) & ~(0x01 << 2)), S5PV210_MP04DAT);     // Set DTV_EN_2.8V to low
    }
    else
    {
        writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 4)), S5PV210_GPH3DAT);     //Set DMB_EN to low
        Sleep(5);

        writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 5)), S5PV210_GPH3DAT);     // Set DTV_EN_2.8V to low
    }
    Sleep(50);

    pHwCtx->uiDevState = EDEV_STATE_RESET;
}

Void MciHwStartChannelHardware(PMCI_HW_CONTEXT pHwCtx)
{
    UInt32 val =0;

    //Mask SPI_IRQ Interrupt
    writel((readl(S5PC11X_EINT2MASK) | (0x01 << 5)), S5PC11X_EINT2MASK);    //XEINT_6

    // Rset SPI FIFOs
    writel((readl(gpSpiRegs + S5PC_CH_CFG) | (0x01 << 5)), gpSpiRegs + S5PC_CH_CFG); 
    writel(0x00, gpSpiRegs + S5PC_CH_CFG); 

    // Set Clock Configuration
    //  SPI_CLKSEL      => b'10 : EPLL clock
    //  ENCLK           => b'1  : Enable
    //  SPI_SCALER      => 66   : 133/(2*(66+1) => 1MHz
    val = (0x0 << 9) | (0x1 << 8) | (MCI_SPI_PRESCALE_VALUE_FW & 0xFF);

    writel(val, gpSpiRegs + S5PC_CLK_CFG); 

    // Set SPI Mode configuration
    //  CH_WIDTH        => b'01 : Half-Word
    //  BUS_WIDTH       => b'01 : Half-Word
    //  RX_RDY_LVL      => b'000001 : 2 Byte
    //  TX_RDY_LVL      => b'000001 : 2 Byte
    val = (0x1 << 29) | (0x1 << 17) | (0x2 << 11) | (0x2 << 5);
    writel(val, gpSpiRegs + S5PC_MODE_CFG); 

    // Set Packet Count Configuration
    writel(0x00, gpSpiRegs + S5PC_PACKET_CNT); 
    writel(0x1F, gpSpiRegs + S5PC_PENDING_CLR);              // all clear pending bit

    // Feedback delay and SWAP configuration
    writel(0x00, gpSpiRegs + S5PC_SWAP_CFG); 



    // Release DTV_EN_2.8V, DMB_EN, DMB_RST signals
    if (HWREV >= 0x02 && HWREV < 0x07)
    {
        writel((readl(S5PV210_MP04DRV) | (0x03 << 4)), S5PV210_MP04DRV);  // Set DTV_EN_2.8V DriverStrength
        writel((readl(S5PV210_MP04DRV) | (0x03 << 2)), S5PV210_MP04DRV);  // Set DMB_EN DriverStrength

        writel((readl(S5PV210_MP04DAT) | (0x01 << 2)), S5PV210_MP04DAT);  // Set DTV_EN_2.8V to high
        writel((readl(S5PV210_MP04DAT) | (0x01 << 1)), S5PV210_MP04DAT);  // Set DMB_EN to high
    }
    else
    {
        writel((readl(S5PV210_GPH3DAT) | (0x01 << 5)), S5PV210_GPH3DAT);    // Set DTV_EN_2.8V to high
        writel((readl(S5PV210_GPH3DAT) | (0x01 << 4)), S5PV210_GPH3DAT);  // Set DMB_EN to high
    }
    Sleep(10);                 

    writel((readl(S5PV210_GPH3DRV) | (0x03 << 12)), S5PV210_GPH3DRV);     // Set DMB_RST DriverStrength
    writel((readl(S5PV210_GPH3DAT) | (0x01 << 6)), S5PV210_GPH3DAT);     // Set DMB_RST to high
    Sleep(20);


    writel((readl(S5PC11X_EINT2PEND) | (0x01 << 5)), S5PC11X_EINT2PEND); // Clearing XEINT_6 Pending bit
    writel(0x0, gpSpiRegs + S5PC_FB_CLK);       // 0nsec additional delay

    return;

}

Bool MciHwIsStartedChannelLink(PMCI_HW_CONTEXT pHwCtx)
{
    return (pHwCtx->uiDevState > EDEV_STATE_RESET);

}

// 4F61 Power On
Void MciHwPowerOnChannel(PMCI_HW_CONTEXT pHwCtx)
{
	// In 4F60/4F65 Module B'd in SMDK6410, there's no power operation
	// [B'd powered]

    MciDrvDisableInterrupt();
    
	pHwCtx->uiDevState = EDEV_STATE_POWER_ON;
	return;

}
// 4F61 Power Off
Void MciHwPowerOffChannel(PMCI_HW_CONTEXT pHwCtx)
{
	pHwCtx->uiDevState = EDEV_STATE_POWER_OFF;
    
    // Release DTV_EN_2.8V, DMB_EN, DMB_RST signals
    if (HWREV >= 0x02 && HWREV < 0x07)
    {
        writel((readl(S5PV210_MP04DAT) & ~(0x01 << 2)), S5PV210_MP04DAT);    // Set DTV_EN_2.8V to high
        writel((readl(S5PV210_MP04DAT) & ~(0x01 << 1)), S5PV210_MP04DAT);  // Set DMB_EN to high
    }
    else
    {
        writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 5)), S5PV210_GPH3DAT);    // Set DTV_EN_2.8V to high
        writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 4)), S5PV210_GPH3DAT);  // Set DMB_EN to high
    }
    
	return;

}
Bool MciHwFirmwareImageDownload(PMCI_HW_CONTEXT pHwCtx, UInt32 uiAddress, pUChar pBuffer, UInt32 uiTransferLen)
{
    UInt32 i;

    RETAILMSG(0, (TEXT("[MCIDRV] +MciHwFirmwareImageDownload\r\n")));

    for (i=0; i < uiTransferLen; i = i+2)
    {
        SpiHwWriteHWord(pHwCtx, *(pUInt16)&pBuffer[i]);
    }

    RETAILMSG(0, (TEXT("[MCIDRV] -MciHwFirmwareImageDownload\r\n")));

    return TRUE;
}

BOOL MciHwPreFirmwareImageDownload(PMCI_HW_CONTEXT pHwCtx)
{
    Bool    bRet = FALSE;
    UInt32  i, uiLen = 0;
    UChar   ucData[10];

    do
    {
        RETAILMSG(DBG_ON, (TEXT("[MCISPI] +MciHwPreFirmwareImageDownload\r\n")));
        Sleep(1000); //sizkim

        // Find F/W Download Request Message
        SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
        for (i=0; i < 400; i++)
        {
            SpiHwSetRxTransferMode(pHwCtx, TRUE, 2);
            *(pUInt16)ucData = SpiHwReadHWord(pHwCtx);
            SpiHwSetRxTransferMode(pHwCtx, FALSE, 2);
            RETAILMSG(1, (TEXT("[MciSPI Test] ucData[0] = 0x%02X ucData[1] = 0x%02X\r\n"), ucData[0], ucData[1]));            
			//RETAILMSG(1, (TEXT(" => 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n"), ucData[2], ucData[3], ucData[4], ucData[5], ucData[6], ucData[7], ucData[8], ucData[9]));            	

            if (ucData[0] == MIS_MSG_STX && ucData[1] == 0x1C)
            {
                break;
            }
        }

        if (i >= 400)
        {   // Can't find STX & FW_DOWNLOAD_REQUEST type
            RETAILMSG(1, (TEXT("[ERROR!MCISPI] Can't find F/W Downalod request message\r\n")));
            break;
        }
        
        //Read the length of Message for F/W Download request
        SpiHwSetRxTransferMode(pHwCtx, TRUE, 2);
        *(pUInt16)&uiLen = SpiHwReadHWord(pHwCtx);
        SpiHwSetRxTransferMode(pHwCtx, FALSE, 2);
        uiLen += 2; // add ETX and Padding

        // Read the body of Message for F/W Download Request
        SpiHwSetRxTransferMode(pHwCtx, TRUE, uiLen);
        for (i=0; i < uiLen; i=i+2)
        {
            *((pUInt16)&ucData[2+i]) = SpiHwReadHWord(pHwCtx);
        }
        SpiHwSetRxTransferMode(pHwCtx, FALSE, uiLen);

        RETAILMSG(1, (TEXT("[MCISPI] Pre - F/W Download=> %02X %02X, Len = %d\r\n")
                    , ucData[0], ucData[1],  uiLen));
		RETAILMSG(1, (TEXT("[MCIPSI] Data => 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n"), ucData[2], ucData[3], ucData[4], ucData[5], ucData[6], ucData[7]));
        bRet = TRUE;
    } while (FALSE);

    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);
    Sleep(2);

    // External interrupt Enable
#if 0    
    writel((readl(S5PC11X_EINT2PEND) | (0x01 << 3)), S5PC11X_EINT2PEND); //unmask XEINT_19 Interrupt enable
#else
    writel((readl(S5PC11X_EINT2PEND) | (0x01 << 5)), S5PC11X_EINT2PEND); //unmask XEINT_6 Interrupt enable
#endif

    pHwCtx->uiDevState = EDEV_STATE_FW_DOWNLOAD;

    RETAILMSG(DBG_ON, (TEXT("[MCISPI] -MciHwPreFirmwareImageDownload\r\n")));

    SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
    SpiHwSetTxTransferMode(pHwCtx, TRUE);

    return bRet;
}

Bool MciHwPostFirmwareImageDownload(PMCI_HW_CONTEXT pHwCtx)
{
    UInt32  Spi_Prescale_value =0;

    SpiHwLoopDelay(pHwCtx, 200);
    SpiHwSetTxTransferMode(pHwCtx, FALSE);    
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);

    // Chage Prescaler Value for receiving Message
    writel((readl( gpSpiRegs +S5PC_CLK_CFG) & ~(0x1 << 8)), gpSpiRegs + S5PC_CLK_CFG);   // Clock Disable
    writel((readl( gpSpiRegs +S5PC_CLK_CFG) & ~(0xFF << 0)), gpSpiRegs + S5PC_CLK_CFG);   


	Spi_Prescale_value = 3; //pHwCtx->uispiprescale;    
    writel((readl( gpSpiRegs +S5PC_CLK_CFG) | (Spi_Prescale_value)), gpSpiRegs + S5PC_CLK_CFG);
    printk ("[MCIDRV]S5PC_CLK_CFG = %08X\n", readl( gpSpiRegs + S5PC_CLK_CFG));

    writel((readl( gpSpiRegs +S5PC_CLK_CFG) & ~(0x3 << 9)), gpSpiRegs + S5PC_CLK_CFG);   // PCLK Enable
    writel((readl( gpSpiRegs +S5PC_CLK_CFG) | (0x1 << 8)), gpSpiRegs + S5PC_CLK_CFG);   // Clock Enable

    pHwCtx->uiDevState = EDEV_STATE_READY;

#if 0
    writel((readl(S5PC11X_EINT2MASK) & ~(0x01 << 3)), S5PC11X_EINT2MASK);    // Unmask XEINT_19 Interrupt
#else
    writel((readl(S5PC11X_EINT2MASK) & ~(0x01 << 5)), S5PC11X_EINT2MASK);    // Unmask XEINT_19 Interrupt
#endif

    RETAILMSG(DBG_ON, (TEXT("[MCISPI] Firmware Donwloading Done Wow222!!!!!\r\n")));
    return TRUE;
}

Void MciHwInitializeDeviceCapabilities(PMCI_HW_CONTEXT pHwCtx)
{
    // Use DMA
#if _MCI_SUPPORT_DMA
    pHwCtx->bSupportDMA     = TRUE;
#else
    pHwCtx->bSupportDMA     = FALSE;
#endif

    // Use TS H/W Interface
    pHwCtx->bSupportTSHW    = FALSE;

    // SPI H/W Interface
    pHwCtx->uiHwIntfType    = EMCI_TYPE_SPI;

    pHwCtx->bDMASemaWaked_Outside = FALSE;
}

UInt32 MciHwGetHwInterfaceType(PMCI_HW_CONTEXT pHwCtx)
{
    return (UInt32)pHwCtx->uiHwIntfType;
}

Bool MciHwRegisterCardEventHandle(PMCI_HW_CONTEXT pHwCtx, MCI_DRV_CARD_EVENT_HANDLER_FUNC fnThread, pVoid pContext)
{
    pHwCtx->fnCardEventHandler      = fnThread;
    pHwCtx->pCardEventHandlerContext= pContext;

    return TRUE;
}

Bool MciHwCheckDmaOperationIsDone(PMCI_HW_CONTEXT pHwCtx, Bool bIsPended)
{
    Bool bRet = TRUE;
    if (bIsPended)
    {
    	SpiHwSetRxDmaTransferMode(pHwCtx, FALSE, 0);

        SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);

        bRet = FALSE;
    }
    return bRet;
}

Void SpiHwInitRegister(void)
{

    RETAILMSG(1, (TEXT("[MCISPI] SPI DRIVER INITIALIZING...\n")));

    // Use MPLL because it seems to be better than EPLL.
    // EPLL can be used by anyone else, so changing the configuration of EPLL is very dangerous.
    if(SPI_CHANNEL) //HSSPI[1]
    {
        RETAILMSG(1, (TEXT("[MCISPI] SPI DRIVER INITIALIZING SPI_CHANNEL = %d...\n"), SPI_CHANNEL));

        writel((readl(S5P_CLK_SRC5) | (0x07 << 4)), S5P_CLK_SRC5);   // Select MOUTepll for HSSPI[1]
        writel((readl(S5P_CLK_DIV5) & ~(0xF << 4)), S5P_CLK_DIV5);   // Set CLKSPI1 to CLKSPI1in

        // Get GPIO for HSSPI[1] MISO, MOSI, CLK, nSS
        writel((readl(S5PV210_GPBPUD) & ~(0xFF << 8)), S5PV210_GPBPUD);      // Set pull-up/down disable
        writel((readl(S5PV210_GPBCON) & ~(0xFFFF << 16)), S5PV210_GPBCON);     
        writel((readl(S5PV210_GPBCON) | (0x02 << 28) | (0x02 << 24) | (0x02 << 20) | (0x02 << 16)), S5PV210_GPBCON);
        writel((readl(S5PV210_GPBDRV) | (0xFF << 8)), S5PV210_GPBDRV);      // Set DriverStrength
        writel((readl(S5PV210_GPBCONPDN) | (0xFF << 8)), S5PV210_GPBCONPDN);      // Set DriverStrength
    }
    else        // HSSPI[0]
    {
        RETAILMSG(1, (TEXT("[MCISPI] SPI DRIVER INITIALIZING SPI_CHANNEL = %d...\n"), SPI_CHANNEL));

        writel((readl(S5P_CLK_SRC5) | (0x07)), S5P_CLK_SRC5);   // Select MOUTepll for HSSPI[0]
        writel((readl(S5P_CLK_DIV5) & ~(0xF)), S5P_CLK_DIV5);;   // Set CLKSPI0 to CLKSPI0in

        // Get GPIO for HSSPI[0] MISO, MOSI, CLK, nSS
        writel((readl(S5PV210_GPBPUD) & ~(0xFF << 0)), S5PV210_GPBPUD);      // Set pull-up/down disable
        writel((readl(S5PV210_GPBCON) & ~(0xFFFF << 0)), S5PV210_GPBCON); 
        writel((readl(S5PV210_GPBCON) | (0x02 << 12) | (0x02 << 8) | (0x02 << 4) | (0x02 << 0)), S5PV210_GPBCON);

        // shkim: Change Drive Strength
        // [2n+1:2n], n=0~7: strength, [n+16], n=0~7: 0=> Fast Slew, 1=> Slow Slew
        writel(((readl(S5PV210_GPBDRV)&~0x00d07300)|0x00007300), S5PV210_GPBDRV);

    }

	/// Modified for CES2011 Galaxy Tab		2010.12.17	tuton
	// SPI_IRQ Pin Initialization: GPH0CON[6]/XEINT[6]
	writel((readl(S5PV210_GPH2CON) & ~(0xF << 20)), S5PV210_GPH2CON);
	writel((readl(S5PV210_GPH2CON) | (0xF << 20)), S5PV210_GPH2CON);              // GPH0CON[6], EXT_INT[6]

	/// [Modified by tuton]	2010-12-27
	//writel((readl(S5PV210_GPH0PUD) & ~ (0x03 << 6)), S5PV210_GPH0PUD); 
	//writel((readl(S5PV210_GPH0PUD) | (0x01 << 6)), S5PV210_GPH0PUD);                // Set Pull-down enable
	writel((readl(S5PV210_GPH2PUD) & ~ (0x03 << 10)), S5PV210_GPH2PUD);             
	writel((readl(S5PV210_GPH2PUD) | (0x01 << 10)), S5PV210_GPH2PUD);                // Set Pull-down enable

	//writel((readl(S5PC11X_EINT0CON) & ~(0x7 << 24)), S5PC11X_EINT0CON); 
	//writel((readl(S5PC11X_EINT0CON) | (0x03 << 24)), S5PC11X_EINT0CON);          // Set Rising edge-triggerer
	writel((readl(S5PC11X_EINT2CON) & ~(0x7 << 20)), S5PC11X_EINT2CON); 
	writel((readl(S5PC11X_EINT2CON) | (0x02 << 20)), S5PC11X_EINT2CON);          // Set Faliing edge-triggerer

	/// [Modified by tuton]	2010-12-27
	//writel((readl(S5PC11X_EINT0FLTCON0) & ~(0xFF << 24)), S5PC11X_EINT0FLTCON0);
	//writel((readl(S5PC11X_EINT0FLTCON0) | (0x01 << 31) | (0x01 << 30) | (0x09 << 24) ), S5PC11X_EINT0FLTCON0);    // Set Filter Enable
	writel((readl(S5PC11X_EINT2FLTCON1) & ~(0xFF << 8)), S5PC11X_EINT2FLTCON1);
	writel((readl(S5PC11X_EINT2FLTCON1) | (0x01 << 15) | (0x01 << 14) | (0x09 << 8) ), S5PC11X_EINT2FLTCON1);    // Set Filter Enable

	/// Modified for CES2011 Galaxy Tab		2010.12.17	tuton
	// DMB_RST Pin Initialization: GPD0[2]/XpwmTOUT[2]

	/// [Modified by tuton]	2010-12-27
	//writel((readl(S5PV210_GPD0PUD) & ~(0x03 << 2)), S5PV210_GPD0PUD);      // Set Pull-up/down disable
	writel((readl(S5PV210_GPH3PUD) & ~(0x03 << 12)), S5PV210_GPH3PUD);       // Set Pull-up/down disable
	/// [end of code]
	
	writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 6)), S5PV210_GPH3DAT);     // Set to low
	writel((readl(S5PV210_GPH3CON) & ~(0x0F << 24)), S5PV210_GPH3CON);  
	writel((readl(S5PV210_GPH3CON) | (0x01 << 24)), S5PV210_GPH3CON);    // GPD0CON[1], Output

        RETAILMSG(1, (TEXT("[MCIDRV] HW Platform version is '%02d'\r\n"), HWREV));

        if (HWREV >= 0x02 && HWREV < 0x07)
        {
	    // DMB_EN Pin 
	    writel((readl(S5PV210_MP04PUD) & ~(0x03 << 2)), S5PV210_MP04PUD);      // external pull-down register
	    writel((readl(S5PV210_MP04DAT) & ~(0x01 << 1)), S5PV210_MP04DAT);     // Set to low
	    writel((readl(S5PV210_MP04CON) & ~(0x0F << 4)), S5PV210_MP04CON);  
	    writel((readl(S5PV210_MP04CON) |  (0x01 << 4)), S5PV210_MP04CON);    // GPH3CON[3], Output
    
	    // DMB_EN_2.8V Pin
	    writel((readl(S5PV210_MP04PUD) & ~(0x03 << 4)), S5PV210_MP04PUD);       // external Pull-down register
	    writel((readl(S5PV210_MP04DAT) & ~(0x01 << 2)), S5PV210_MP04DAT);     // Set to low
	    writel((readl(S5PV210_MP04CON) & ~(0x0F << 8)), S5PV210_MP04CON);  
	    writel((readl(S5PV210_MP04CON) |  (0x01 << 8)), S5PV210_MP04CON);    // GPJ3CON[2], Output
        }
        else
        {
	    // DMB_EN Pin 
	    writel((readl(S5PV210_GPH3PUD) & ~(0x03 << 8)), S5PV210_GPH3PUD);      // Set Pull-up/down disable
	    writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 4)), S5PV210_GPH3DAT);     // Set to low
	    writel((readl(S5PV210_GPH3CON) & ~(0x0F << 16)), S5PV210_GPH3CON);  
	    writel((readl(S5PV210_GPH3CON) | (0x01 << 16)), S5PV210_GPH3CON);    // GPH3CON[3], Output
    
	    // DMB_EN_2.8V Pin
	    writel((readl(S5PV210_GPH3PUD) & ~(0x03 << 10)), S5PV210_GPH3PUD);       // Set Pull-up/down disable
	    writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 5)), S5PV210_GPH3DAT);     // Set to low
	    writel((readl(S5PV210_GPH3CON) & ~(0x0F << 20)), S5PV210_GPH3CON);  
	    writel((readl(S5PV210_GPH3CON) | (0x01 << 20)), S5PV210_GPH3CON);    // GPJ3CON[2], Output
        }


    // HSSPI Clock On
    if(SPI_CHANNEL)
    {
        writel((readl(S5P_CLKGATE_IP3) | (1 << 13)), S5P_CLKGATE_IP3);  // For HSSPI[1], PCLK_SPI1
    }
    else
    {
        writel((readl(S5P_CLKGATE_IP3) | (1 << 12)), S5P_CLKGATE_IP3);  // For HSSPI[0], PCLK_SPI0
    }
}


Void SpiHwDmaInterruptDoneHandler(struct s3c2410_dma_chan *dma_ch, void *buf_id, Int32 size, enum s3c2410_dma_buffresult result)
{
	PMCI_HW_CONTEXT pHwCtx = (PMCI_HW_CONTEXT)buf_id;

	SpiHwSetRxDmaTransferMode(pHwCtx, FALSE, 0);

    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);
	
	RETAILMSG(0, (TEXT("[MCIDRV] DMA Done: status=0x%08X\r\n"), (UInt32)result));

	DrvHandlePendedSpiIrqInterrupt((PMCI_DRIVER)pHwCtx->pDrvCtx);
}


Void SpiHwSetChipSelector(PMCI_HW_CONTEXT pHwCtx, ESPI_CS csValue)
{
	switch(csValue)
	{
		case ESPI_CS_ENABLE:
            writel((readl(gpSpiRegs + S5PC_SLAVE_SEL) & ~(0x1)), gpSpiRegs + S5PC_SLAVE_SEL);
			break;
		case ESPI_CS_DISABLE:
		default:
            writel((readl(gpSpiRegs + S5PC_SLAVE_SEL) | (0x1)), gpSpiRegs + S5PC_SLAVE_SEL);
		break;
	}
    SpiHwLoopDelay(pHwCtx, 3);
	return;
}
Void SpiHwSetRxTransferMode(PMCI_HW_CONTEXT pHwCtx, Bool bOn, UInt32 uiTransferLen)
{
    UInt32 val =0;
    if (bOn)    // Start
    {
        writel(MCI_SPI_FIFO_RESET, gpSpiRegs + S5PC_CH_CFG);
        writel(0x0, gpSpiRegs + S5PC_CH_CFG);
        pHwCtx->m_uiSavedSpiModeRegisterValue = readl( gpSpiRegs + S5PC_MODE_CFG);
        m_uiSavedSpiModeRegisterValue = pHwCtx->m_uiSavedSpiModeRegisterValue ;

        val = (0x1<<29) | (0x1<<17)| (0x2 << 11) | (0x2 << 5);
        writel(val, gpSpiRegs + S5PC_MODE_CFG);
        writel(0x0, gpSpiRegs + S5PC_PACKET_CNT);
        writel(0x1F, gpSpiRegs + S5PC_PENDING_CLR);
        
        if (uiTransferLen)
        {
            uiTransferLen                       = uiTransferLen / 2;  // For Half-word operation
            val = (0x1<<16) | (uiTransferLen & 0xFFFF);
            writel(val, gpSpiRegs + S5PC_PACKET_CNT);
        }
        writel(MCI_SPI_MASTER_RX_ON, gpSpiRegs + S5PC_CH_CFG);
    }
    else        // Stop
    {
        writel(MCI_SPI_MASTER_RX_OFF, gpSpiRegs + S5PC_CH_CFG);

        val = pHwCtx->m_uiSavedSpiModeRegisterValue;        
//        m_uiSavedSpiModeRegisterValue = val;
        writel(val, gpSpiRegs + S5PC_MODE_CFG);
    }
    return;
}


Void SpiHwSetTxTransferMode(PMCI_HW_CONTEXT pHwCtx, Bool bOn)
{
    UInt32 val =0;
    if (bOn)    // Start
    {
        pHwCtx->m_uiSavedSpiModeRegisterValue = readl( gpSpiRegs + S5PC_MODE_CFG);

        m_uiSavedSpiModeRegisterValue = pHwCtx->m_uiSavedSpiModeRegisterValue;
        val = (0x1<<29) | (0x1<<17)| (0x2 << 11) | (0x2 << 5);
        writel(val, gpSpiRegs + S5PC_MODE_CFG);
        writel(0x0, gpSpiRegs + S5PC_PACKET_CNT);
        writel(0x1F, gpSpiRegs + S5PC_PENDING_CLR);

        writel(MCI_SPI_FIFO_RESET, gpSpiRegs + S5PC_CH_CFG);
        writel(0x0, gpSpiRegs + S5PC_CH_CFG);
        writel(MCI_SPI_MASTER_TX_ON | MCI_SPI_MASTER_RX_ON, gpSpiRegs + S5PC_CH_CFG);

    }
    else        // Stop
    {
        // Wait for TX Done
        while ((readl(gpSpiRegs + S5PC_SPI_STATUS) & (0x1 << 25)) == 0);

        writel(MCI_SPI_MASTER_TX_OFF, gpSpiRegs + S5PC_CH_CFG);

        val = pHwCtx->m_uiSavedSpiModeRegisterValue;
//        m_uiSavedSpiModeRegisterValue = val;
        writel(val , gpSpiRegs + S5PC_MODE_CFG);
    }
    return;
}


Void SpiHwSetRxDmaTransferMode(PMCI_HW_CONTEXT pHwCtx, Bool bOn, UInt32 uiTransferLen)
{

    UInt32 val =0;

    if (bOn)    // Start
    {
        writel(MCI_SPI_FIFO_RESET, gpSpiRegs + S5PC_CH_CFG);
        writel(0x00, gpSpiRegs + S5PC_CH_CFG);
        pHwCtx->m_uiSavedSpiModeRegisterValue   = readl( gpSpiRegs + S5PC_MODE_CFG);

        m_uiSavedSpiModeRegisterValue = pHwCtx->m_uiSavedSpiModeRegisterValue;

        val = (0x1<<29) | (0x1<<17) | (0x1<<2) | (0x0) | (0x3FF << 19);

        writel(val, gpSpiRegs + S5PC_MODE_CFG);
        writel(0x00, gpSpiRegs + S5PC_PACKET_CNT);
        writel(0x1F, gpSpiRegs + S5PC_PENDING_CLR);

        if (uiTransferLen)
        {
            uiTransferLen = uiTransferLen / 2;  // For Half-word operation
            val = (0x1<<16) | (uiTransferLen & 0x7FFF);
            writel(val, gpSpiRegs + S5PC_PACKET_CNT);
        }
        writel(MCI_SPI_MASTER_RX_ON, gpSpiRegs + S5PC_CH_CFG);

    }
    else        // Stop
    {
        writel(MCI_SPI_MASTER_RX_OFF, gpSpiRegs + S5PC_CH_CFG);

        val = pHwCtx->m_uiSavedSpiModeRegisterValue;
//        m_uiSavedSpiModeRegisterValue = val;
        writel(val , gpSpiRegs + S5PC_MODE_CFG);
    }
    return;
}

UInt16 SpiHwWriteHWord(PMCI_HW_CONTEXT pHwCtx, UInt16 usData)
{
    
	while((((readl( gpSpiRegs + S5PC_SPI_STATUS)) >> 6) & 0x1FF) >= 0x20 /* FIFO full */);

    writel( usData, gpSpiRegs + S5PC_SPI_TX_DATA);
    
    while((readl( gpSpiRegs + S5PC_SPI_STATUS) & (0x1 << 25)) == 0);  /*TX_DONE*/

    return (UInt16)readl( gpSpiRegs + S5PC_SPI_RX_DATA);
}

UInt16 SpiHwReadHWord(PMCI_HW_CONTEXT pHwCtx)
{
	UInt16  usData = 0xFFFF;
	UInt32  j=0;
        
    while(((readl( gpSpiRegs + S5PC_SPI_STATUS) >> 15) & 0x1FF)==0 /* FIFO empty */)
	{
		if(j++ > 90000)
		{
			RETAILMSG(1,(TEXT("[Error] No Readable Data !! \r\n")));
			return -1;
		}
	};

    usData = (UInt16)readl( gpSpiRegs + S5PC_SPI_RX_DATA);
    
	return usData;
}

Void SpiHwClearTxRxFifo(PMCI_HW_CONTEXT pHwCtx)
{
    writel(0x1F, gpSpiRegs + S5PC_PENDING_CLR);
    
    writel((readl( gpSpiRegs + S5PC_CH_CFG) | (0x1 << 5)), gpSpiRegs + S5PC_CH_CFG);  // active SPI software reset
    writel((readl( gpSpiRegs + S5PC_CH_CFG) & ~(0x1 << 5)), gpSpiRegs + S5PC_CH_CFG);  // inative SPI software reset

    return;
}

Void SpiHwLoopDelay(PMCI_HW_CONTEXT pHwCtx, UInt32 uiMicroSecond)
{
    UInt32 i;

    volatile UInt32 uiTmpValue;

    for (i=0; i < (uiMicroSecond * 2); i++)
    {
        uiTmpValue = readl(S5PV210_GPH2DAT); // SPI_IRQ must be set to 0    in case of Falling Edge
    }

}

Int32 SpiHwDmaConfig( PMCI_HW_CONTEXT pHwCtx, Int32 mode)
{

    Int32 ret;

	do
	{
	    if (mode == 0) 
	    { // RX
			ret = s3c2410_dma_devconfig(pHwCtx->gDmaSubChannelRx, S3C2410_DMASRC_HW, S5PC_SPI_RX_DATA_REG);

	        if(ret !=0)
	        {
	            RETAILMSG(1, (TEXT("[ERROR-MCIDRV] DMA RX Config failed!\n")));
	            break;
	        }

	    }
		else
	    { // TX
			ret = s3c2410_dma_devconfig(pHwCtx->gDmaSubChannelRx, S3C2410_DMASRC_MEM, S5PC_SPI_TX_DATA_REG);
	        if(ret !=0)
	        {
	            RETAILMSG(1, (TEXT("[ERROR-MCIDRV] DMA TX Config failed!\n")));
	            break;
	        }

	    }
	    
		ret = s3c2410_dma_config(pHwCtx->gDmaSubChannelRx, S5PC_DMA_XFER_HWORD);
	    if(ret !=0)
	    {
	        RETAILMSG(1, (TEXT("[ERROR-MCIDRV] DMA Transfer mode Config failed!\n")));
	        break;
	    }
		
	    ret = s3c2410_dma_setflags(pHwCtx->gDmaSubChannelRx, S3C2410_DMAF_AUTOSTART);
	    if(ret !=0)
	    {        
	        RETAILMSG(1, (TEXT("[ERROR-MCIDRV] DMA Setflag Config failed!\n")));
	        break;
	    }
	} while (FALSE);

    return ret;
}

Void MciDrvDisableInterrupt(void)
{
#if 0
    writel((readl(S5PC11X_EINT2MASK) | (0x01 << 3)), S5PC11X_EINT2MASK);    //mask GPH2CON[3] XEINT_19

    //sizkim    
    writel((readl(S5PC11X_EINT2PEND) | (0x01 << 3)), S5PC11X_EINT2PEND); // Clearing XEINT_19 Pending bit
#else
    writel((readl(S5PC11X_EINT2MASK) | (0x01 << 5)), S5PC11X_EINT2MASK);    //mask GPH2CON[3] XEINT_6

    //sizkim    
    writel((readl(S5PC11X_EINT2PEND) | (0x01 << 5)), S5PC11X_EINT2PEND); // Clearing XEINT_6 Pending bit
#endif

}

Void MciDrvEnableInterrupt(void)
{
    //sizkim    writel((readl(S5PC1XX_EINT2PEND) | (0x01 << 4)), S5PC1XX_EINT2PEND);
#if 0    
    writel((readl(S5PC11X_EINT2PEND) | (0x01 << 3)), S5PC11X_EINT2PEND); // Clearing XEINT_19 Pending bit
    writel((readl(S5PC11X_EINT2MASK) & ~(0x01 << 3)), S5PC11X_EINT2MASK);    // Unmask XEINT_19 Interrupt
#else
    writel((readl(S5PC11X_EINT2PEND) | (0x01 << 5)), S5PC11X_EINT2PEND); // Clearing XEINT_19 Pending bit
    writel((readl(S5PC11X_EINT2MASK) & ~(0x01 << 5)), S5PC11X_EINT2MASK);    // Unmask XEINT_19 Interrupt
#endif

}

Bool MciHwIsInterruptActivated(void)
{
    return (gpio_get_value(S5PV210_GPH2(5)) == MCI_INTR_ACTIVE);
}

Int32 MciHwDoSelfTest(PMCI_HW_CONTEXT pHwCtx, pInt32 pResult)
{
	Int32	iRet;
	RETAILMSG(1,(TEXT("[MCIDRV] + DoSelfTest ========================\r\n")));
	do
	{
		UChar buffer[20];
		UInt16 msgLen = 0;
		if (pResult == NULL)
		{
			break;
		}
		RETAILMSG(1, (TEXT("[MCIDRV] Before Reset : SPI_IRQ = '%s'\r\n"), MciHwIsInterruptActivated() ? "Low" : "High"));
		MciHwPowerOffChannel(pHwCtx);
		MciHwStartChannelHardware(pHwCtx);
		MciHwPowerOnChannel(pHwCtx);
		RETAILMSG(1, (TEXT("[MCIDRV] After Reset  : SPI_IRQ = '%s'\r\n"), MciHwIsInterruptActivated() ? "Low" : "High"));
		if (TRUE == MciHwIsInterruptActivated())
		{
			*pResult = 1; // Interrupt is not High
			break;
		}

    		// Chage Prescaler Value for receiving Message
    		writel((readl( gpSpiRegs +S5PC_CLK_CFG) & ~(0x1 << 8)), gpSpiRegs + S5PC_CLK_CFG);   // Clock Disable
    		writel((readl( gpSpiRegs +S5PC_CLK_CFG) & ~(0xFF << 0)), gpSpiRegs + S5PC_CLK_CFG);   
	
    		writel((readl( gpSpiRegs +S5PC_CLK_CFG) | (3)), gpSpiRegs + S5PC_CLK_CFG);

    		writel((readl( gpSpiRegs +S5PC_CLK_CFG) & ~(0x3 << 9)), gpSpiRegs + S5PC_CLK_CFG);   // PCLK Enable
    		writel((readl( gpSpiRegs +S5PC_CLK_CFG) | (0x1 << 8)), gpSpiRegs + S5PC_CLK_CFG);   // Clock Enable

		// Read Message Header
		SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
		SpiHwSetRxTransferMode(pHwCtx, TRUE, 2);
		*((pUInt16)(buffer + 0)) = SpiHwReadHWord(pHwCtx);
		SpiHwSetRxTransferMode(pHwCtx, FALSE, 2);
		if (buffer[0] != 0x02)
		{
			*pResult = 2; // Invalid STX
			break;
		}
		if (buffer[1] != 0x1C)
		{
			*pResult = 3; // Invalid MsgType of FwDonwlaodReq
			break;
		}

		// Read Message Length
		SpiHwSetRxTransferMode(pHwCtx, TRUE, 2);
		msgLen = SpiHwReadHWord(pHwCtx);
		SpiHwSetRxTransferMode(pHwCtx, FALSE, 2);
		*((pUInt16)(buffer + 2)) = msgLen;
		SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);
		if (msgLen != 4)
		{
			*pResult = 4; // Invalid MsgLen of FwDownloadReq
			break;
		}

		// Read Message Body
		if (EMCI_OK == MciHwRecvMessageBody(pHwCtx, &buffer[4], 6))
        {
            RETAILMSG(1, (TEXT("[MCITEST] F/W DownReq(BODY): %02X %02X %02X %02X %02X %02X\r\n"), buffer[4], buffer[5], buffer[6], buffer[7], buffer[8], buffer[9]));
        }
		else
		{
			*pResult = 5; // Can't read MsgBody of FwDownloadReq
			break;
		}

		if ((buffer[4] != 0x61) && (buffer[5] != 0x4F) && (buffer[6] != 0x00) && (buffer[7] != 0x00))
		{
			*pResult = 6; // Invalid MsgBody of FwDownloadReq
			break;
		}
		if (buffer[8] != 0x03)
		{
			*pResult = 7; // Invalid ETX
			break;
		}

		// Check Interrupt
		RETAILMSG(1, (TEXT("[MCIDRV] After Reading : SPI_IRQ = '%s'\r\n"), MciHwIsInterruptActivated() ? "Low" : "High"));
		if (FALSE == MciHwIsInterruptActivated())
		{
			*pResult = 8; // Interrupt is not Low
			break;
		}

		MciHwPowerOffChannel(pHwCtx);

		*pResult = 0;
		iRet = EMCI_OK;
	} while (FALSE);
	RETAILMSG(1,(TEXT("[MCIDRV] - DoSelfTest: ret=%d, result=%d\r\n"), iRet, *pResult));
	return iRet;
}

Void SpiHwPrepareRecoveryMode(PMCI_HW_CONTEXT pHwCtx)
{
    UChar  ucData[10];
    UInt32 i;

    MciDrvDisableInterrupt(); // Disable interrupt.
    writel((readl(S5PV210_GPH3DAT) & ~(0x01 << 6)), S5PV210_GPH3DAT);     // Set DMB_RST to Low

    MciOsal_Sleep(5);
    writel((readl(S5PV210_GPH3DAT) | (0x01 << 6)), S5PV210_GPH3DAT);     // Set DMB_RST to high    

    MciOsal_Sleep(5);

    // Read the body of Message for F/W Download Request
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
    SpiHwSetRxTransferMode(pHwCtx, TRUE, 10);
    for (i=0; i < 10; i=i+2)
    {
        *((pUInt16)&ucData[i]) = SpiHwReadHWord(pHwCtx);
    }
    SpiHwSetRxTransferMode(pHwCtx, FALSE, 10);
    SpiHwSetChipSelector(pHwCtx, ESPI_CS_DISABLE);

    RETAILMSG(1, (TEXT("[MCIPSI] F/W DonwReq => 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\r\n"), ucData[4], ucData[5], ucData[6], ucData[7], ucData[8], ucData[9]));

    MciDrvEnableInterrupt();  // Enable interrupt.
    MciOsal_Sleep(5);

    SpiHwSetChipSelector(pHwCtx, ESPI_CS_ENABLE);
    SpiHwSetTxTransferMode(pHwCtx, TRUE);
}
