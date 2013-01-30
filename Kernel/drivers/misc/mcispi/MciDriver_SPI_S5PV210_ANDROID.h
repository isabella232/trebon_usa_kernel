/* linux/driver/misc/mcispi/MciDriver_SPI_S5PV210_ANDROID.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciDriver_SPI_S5PV210_ANDROID.h
**
**  Description
**      Header file for the H/W SPI Interface Control for SAMSUNG S5PV210 Android/Linux
**      (P1 Limo)
**  Authur:
**      Charlie Park (jongho0910.park@samsung.com)
**
**  Update:
**      2009.12.09  NEW     Created.
**/

#ifndef _MCI_DRIVER_SPI_S5PV210_ANDROID_H_
#define _MCI_DRIVER_SPI_S5PV210_ANDROID_H_

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int HWREV;	// For run-time checking of H/W version

#define DBG_ON                  0
#define _MCI_DBG_CHECK_RX_INTR  1
#define _MCI_USE_TEST_TRIGGER   0   //Use GPL7 as Test Trigger Port

#define MCI_SPI_DEV_DRV_NAME    "MciSpi"
#define MCI_SPI_MAJOR           155     /*Device Major number*/
#define MCI_SPI_MINOR           0       /*Device Minor number*/ 

// Test용으로 SQ를 출력 시, DMA는 사용하지 않는다.201109 by murphy
// 다음 값이 1이면 test mode이다.
#define PRINT_SQ                0

#if     PRINT_SQ
#define _MCI_SUPPORT_DMA        0
#else
#define _MCI_SUPPORT_DMA        1
#endif

#define MCI_SPI_STX             0x02
#define MCI_SPI_ETX             0x03
#define MCI_SPI_ACK_MSB         0x06
#define MCI_SPI_ACK_LSB         0x00

#define SIZE_OF_IPC_HEAD        (5)

#define TIMEOUT_VALUE           5000
#define MAXIMUM_PACKET          (65535)


#define SPI_CHANNEL 1
#define SPI_TX_MODE 1
#define SPI_RX_MODE 0

#define MCI_INTR_ACTIVE			1
#define _MCI_CHECK_INTR_IN_RCV_	0

//////////////////////////////////////////////////////////////////////////////////
/////   H/W Access Definition                                                /////
//////////////////////////////////////////////////////////////////////////////////

/*
 *  Channel Chip(S3C4F61) Access Information in P1 Limo Project with S5PV210 AP Chip
 *  
 *  SPI_IRQ                -   GPH2CON[3] XEINT_19
 *  DTV_EN_2.8V            -   GPH3CON[2] XEINT_26
 *  DMB_EN                 -   GPH3CON[3] XEINT_27  
 *  DMB_RST                -   GPD0CON[1] XPWMTOUT_1
 *
 *  <SPI[0] Access>
 *  SPI_0_CLK(Pin42)       -   GPBCON[0]           => SPI Clock
 *  SPI_0_MOSI(Pin44)      -   GPBCON[3]           => Master-Out, Slave-In
 *  SPI_0_MISO(Pin46)      -   GPBCON[2]           => Master-In, Slave-Out
 *  SPI_0_nSS(Pin48)       -   GPBCON[1]           => Chip Selector
 *
 *  <SPI[1] Access>
 *  SPI_1_CLK(Pin42)       -   GPBCON[4]           => SPI Clock
 *  SPI_1_MOSI(Pin44)      -   GPBCON[7]           => Master-Out, Slave-In
 *  SPI_1_MISO(Pin46)      -   GPBCON[6]           => Master-In, Slave-Out
 *  SPI_1_nSS(Pin48)       -   GPBCON[5]           => Chip Selector
 *
 *  <SPI[2] Access>
 *  SPI_2_CLK(Pin42)       -   GPG2CON[0]           => SPI Clock
 *  SPI_2_MOSI(Pin44)      -   GPG2CON[3]           => Master-Out, Slave-In
 *  SPI_2_MISO(Pin46)      -   GPG2CON[2]           => Master-In, Slave-Out
 *  SPI_2_nSS(Pin48)       -   GPG2CON[1]           => Chip Selector
 */

// SPI Test Define
#define S5PC_CH_CFG             (0x00)      //SPI configuration
#define S5PC_CLK_CFG            (0x04)      //Clock configuration
#define S5PC_MODE_CFG 	        (0x08)      //SPI FIFO control
#define S5PC_SLAVE_SEL          (0x0C)      //Slave selection
#define S5PC_SPI_INT_EN         (0x10)      //SPI interrupt enable
#define S5PC_SPI_STATUS         (0x14)      //SPI status
#define S5PC_SPI_TX_DATA        (0x18)      //SPI TX data
#define S5PC_SPI_RX_DATA        (0x1C)      //SPI RX data
#define S5PC_PACKET_CNT         (0x20)      //count how many data master gets
#define S5PC_PENDING_CLR        (0x24)      //Pending clear
#define S5PC_SWAP_CFG 	        (0x28)      //SWAP config register
#define S5PC_FB_CLK             (0x2c)      //SWAP FB config register

// fixed by murphy 
#define S5PC11X_GPIOREG(x)		    (S5P_VA_GPIO + (x))

#define S5PC11X_EINT0CON		    S5PC11X_GPIOREG(0xE00)		/* EINT0  ~ EINT7  */
#define S5PC11X_EINT1CON		    S5PC11X_GPIOREG(0xE04)		/* EINT8  ~ EINT15 */
#define S5PC11X_EINT2CON		    S5PC11X_GPIOREG(0xE08)		/* EINT16 ~ EINT23 */
#define S5PC11X_EINT3CON		    S5PC11X_GPIOREG(0xE0C)		/* EINT24 ~ EINT31 */
//#define S5PC11X_EINTCON(x)		    (S5PC11X_EINT30CON+x*0x4)	/* EINT0  ~ EINT31  */

#define S5PC11X_EINT0FLTCON0		S5PC11X_GPIOREG(0xE80)		/* EINT0  ~ EINT3  */
#define S5PC11X_EINT0FLTCON1		S5PC11X_GPIOREG(0xE84)
#define S5PC11X_EINT1FLTCON0		S5PC11X_GPIOREG(0xE88)		/* EINT8 ~  EINT11 */
#define S5PC11X_EINT1FLTCON1		S5PC11X_GPIOREG(0xE8C)
#define S5PC11X_EINT2FLTCON0		S5PC11X_GPIOREG(0xE90)
#define S5PC11X_EINT2FLTCON1		S5PC11X_GPIOREG(0xE94)
#define S5PC11X_EINT3FLTCON0		S5PC11X_GPIOREG(0xE98)
#define S5PC11X_EINT3FLTCON1		S5PC11X_GPIOREG(0xE9C)
//#define S5PC11X_EINTFLTCON(x)		(S5PC11X_EINT30FLTCON0+x*0x4)	/* EINT0  ~ EINT31 */

#define S5PC11X_EINT0MASK		    S5PC11X_GPIOREG(0xF00)		/* EINT30[0] ~  EINT30[7]  */
#define S5PC11X_EINT1MASK		    S5PC11X_GPIOREG(0xF04)		/* EINT31[0] ~  EINT31[7] */
#define S5PC11X_EINT2MASK		    S5PC11X_GPIOREG(0xF08)		/* EINT32[0] ~  EINT32[7] */
#define S5PC11X_EINT3MASK		    S5PC11X_GPIOREG(0xF0C)		/* EINT33[0] ~  EINT33[7] */
//#define S5PC11X_EINTMASK(x)		    (S5PC11X_EINT30MASK+x*0x4)	/* EINT0 ~  EINT31  */

#define S5PC11X_EINT0PEND		    S5PC11X_GPIOREG(0xF40)		/* EINT30[0] ~  EINT30[7]  */
#define S5PC11X_EINT1PEND		    S5PC11X_GPIOREG(0xF44)		/* EINT31[0] ~  EINT31[7] */
#define S5PC11X_EINT2PEND		    S5PC11X_GPIOREG(0xF48)		/* EINT32[0] ~  EINT32[7] */
#define S5PC11X_EINT3PEND		    S5PC11X_GPIOREG(0xF4C)		/* EINT33[0] ~  EINT33[7] */
//#define S5PC11X_EINTPEND(x)		    (S5PC11X_EINT30PEND+x*04)	/* EINT0 ~  EINT31  */

#define MCI_SPI_INT                 IRQ_EINT(21) //IRQ_EINT(6) //IRQ_EINT(33)    //IRQ_EINT(28)     //IRQ_SPI1

    /* SPI CHANNEL 1 */
#if (SPI_CHANNEL)
	/// Modified by tuton	2010.12.15	porting Froyo
    //#define S5PC110_PA_SPI          S5PC11X_PA_SPI1
	#define S5PC110_PA_SPI          S5PV210_PA_SPI1		//S3C_SPI_PA_SPI1

	#define S5PC110_SIZE_SPI        0x00001000      //0x00100000
    #define S5PC_SPI_TX_DATA_REG    0xE1400018     //SPI TX data
    #define S5PC_SPI_RX_DATA_REG    0xE140001C     //SPI RX data
#else
    /* SPI CHANNEL 0 */
/// Modified by tuton	2010.12.15	porting Froyo
    //#define S5PC110_PA_SPI          S5PC11X_PA_SPI0
    #define S5PC110_PA_SPI          S5PV210_PA_SPI0
	
    #define S5PC110_SIZE_SPI        0x00001000      //0x00100000
    #define S5PC_SPI_TX_DATA_REG    0xE1300018  //SPI TX data
    #define S5PC_SPI_RX_DATA_REG    0xE130001C  //SPI RX data
#endif

/* DMA transfer unit (byte). */
#define S5PC_DMA_XFER_BYTE       1
#define S5PC_DMA_XFER_HWORD      2
#define S5PC_DMA_XFER_WORD       4 

#define S3C2410_DCON_HANDSHAKE  (1<<31)
#define S3C2410_DCON_SYNC_PCLK  (0<<30)


#define MCI_SPI_ACK_BYTES               0x0006  // ACK bytes
#define MCI_SPI_PRESCALE_VALUE_FW       66//35
#define MCI_SPI_PRESCALE_VALUE_MSG      3
#define MCI_SPI_FIFO_RESET              0x0020  // 
#define MCI_SPI_MASTER_RX_ON            0x0002  //
#define MCI_SPI_MASTER_RX_OFF           0x0000  //
#define MCI_SPI_MASTER_TX_ON            0x0001  //
#define MCI_SPI_MASTER_TX_OFF           0x0000  //

typedef enum {
    ESPI_CS_ENABLE      = 0,
    ESPI_CS_DISABLE     = 1
} ESPI_CS;

typedef struct _MCI_HW_CONTEXT
{

    /*
     * Common (Mandotory) members
     */
    // Device Capabilities
    Bool                        bSupportDMA;
    Bool                        bSupportTSHW;
    Bool                        bIRQRegister;
    Bool                        bDMAOPWait;
    UInt32                      uiHwIntfType;

    UInt32                      uiDevState;    
    pVoid						pDrvCtx;
    UInt32                      uispiprescale;
	
    //////////////////////////////////////////////////////////////////////////
    /////   Phy Memory to Virtual Memory map Definitions                 /////
    //////////////////////////////////////////////////////////////////////////
    struct resource             *ioarea;

    UInt32                      gDmaSubChannelRx;
    dma_addr_t                  DmaAllocatedPhysicalAddress;

    pVoid                       lpvFixedBufferMemory;

    pVoid                       lpvPhysFixedBufferMemory;

    MCI_DRV_CARD_EVENT_HANDLER_FUNC fnCardEventHandler;
    pVoid                           pCardEventHandlerContext;

    UInt32                      m_uiSavedSpiModeRegisterValue;
//    volatile    UInt32              fMciDrvRcvStream;
    BOOL                        bDMASemaWaked_Outside;
    BOOL                        bIsCardInserted;
} MCI_HW_CONTEXT, *PMCI_HW_CONTEXT, **PPMCI_HW_CONTEXT;

#ifdef __cplusplus
}
#endif

#endif // _MCI_DRIVER_SPI_S5PV210_ANDROID_H_


