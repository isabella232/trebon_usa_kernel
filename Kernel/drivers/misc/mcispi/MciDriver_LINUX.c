/* drivers/misc/mcispi/MciDriver_LINUX.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciDriver_LINUX.c
**
**  Description
**      Implement DllMain function and DDI(Device Driver Interface) functions
**
**  Authour:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.20  NEW     Created.
**/
#include <linux/miscdevice.h>
#include "MciOsalApi.h"
#include "MciDriver.h"

PMCI_DRIVER g_pMciDriver = NULL;

ssize_t MCI_READ(struct file *filp, char *buf , size_t count, loff_t *offset)
{
    PMCI_DRIVER pDriver = (PMCI_DRIVER)filp->private_data;
    //return (ssize_t)MciDrvReadMessage(pDriver, buf, count);
    return (ssize_t)MciDrvReadStream(pDriver, buf, count);
}
ssize_t MCI_WRITE(struct file *filp, char *buf , size_t count, loff_t *offset)
{
    PMCI_DRIVER pDriver = (PMCI_DRIVER)filp->private_data;
    ssize_t ret = 0;
    
    buf = (pChar)MciOsal_MapUserModeBuffer((pVoid)buf, count);
    
    ret = (ssize_t)MciDrvSendMessage(pDriver, buf, count);

    MciOsal_UnmapUserModeBuffer((pVoid)buf);
    return ret;
}


Int32 MCI_IOCTL(struct inode *node, struct file *filp, UInt32 cmd, ULong arg)
{
//    printk("[MCIDRV] MCI_IOCTL cmd =0x%08X\r\n", cmd);

    PMCI_DRIVER     pDriver = (PMCI_DRIVER)filp->private_data;
    PMCI_IOCTL_ARG  pIoCtrlArg;
//    UInt32          uiReturnBytes = 0;
    ULong           uiReturnBytes = 0;
    Bool            bRet = FALSE;
    
//    printk("[MCIDRV] MCI_IOCTL cmd =%d\r\n", cmd);
    pIoCtrlArg = (PMCI_IOCTL_ARG)MciOsal_MapUserModeBuffer((pVoid)arg, sizeof(MCI_IOCTL_ARG));

    if (pIoCtrlArg != NULL)
    {
        pIoCtrlArg->pInBuf = (pUChar)MciOsal_MapUserModeBuffer((pVoid)pIoCtrlArg->pInBuf, pIoCtrlArg->uiInBufLen);
        bRet = (Int32)MciDrvIoControl(pDriver, cmd, pIoCtrlArg->pInBuf
                                                , pIoCtrlArg->uiInBufLen
                                                , pIoCtrlArg->pOutBuf
                                                , pIoCtrlArg->uiOutBufLen
                                                , &uiReturnBytes);

        MciOsal_CopyToUser(pIoCtrlArg->puiWritten, &uiReturnBytes, sizeof(UInt32));

        MciOsal_UnmapUserModeBuffer(pIoCtrlArg->pInBuf);

        MciOsal_UnmapUserModeBuffer(pIoCtrlArg);

        return (Int32)bRet;
    }
    return bRet;
}

Int32 MCI_OPEN(struct inode *node, struct file *filp)
{
    printk("[MCIDRV] MCI_OPEN!!\n");
    filp->private_data = (pVoid)g_pMciDriver;
    MciDrvOpen(g_pMciDriver,0 ,0);
    return 0;
}

Int32 MCI_RELEASE(struct inode *node, struct file *filp)
{
    return MciDrvClose(g_pMciDriver);
}

static struct file_operations MciDrv_fops =
{
    .owner      =   THIS_MODULE,
    .llseek     =   NULL,
    .read       =   MCI_READ,
    .write      =   (Void*)MCI_WRITE,
    .ioctl      =   MCI_IOCTL,
    .open       =   MCI_OPEN,
    .release    =   MCI_RELEASE,
};

#if 1
static struct miscdevice mci_spi_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "MciSpi",
	.fops	= &MciDrv_fops,
};
#endif

static UInt32   MciCdevInitialize(PMCI_DRIVER pDrvCtx)
{
    UInt32 ret = 0;

    RETAILMSG(1,(TEXT("[MCIDRV] Initialize the Character Device..\r\n")));

//    ret = register_chrdev(MCI_SPI_MAJOR, MCI_SPI_DEV_DRV_NAME, &MciDrv_fops);
//  if (ret = register_chrdev(MCI_SPI_MAJOR, MCI_SPI_DEV_DRV_NAME, &MciDrv_fops))
	ret = misc_register(&mci_spi_dev);

    if(ret < 0)
	{
        RETAILMSG(1,(TEXT("[ERROR!MCIDRV] Charater Device Register Failed!\r\n")));
        return ret;
    }

    return ret;
}

static Int32 __init MciDrv_init_module( void )
{

    UInt32 result =0;

    PMCI_DRIVER pDriver = NULL;

    RETAILMSG(1, (TEXT("[MCIDRV] +MCI_Init\r\n")));


    if (g_pMciDriver == NULL)
    {
        pDriver = MciDrvCreateInstance();

        if (pDriver == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] Out of Memory!\r\n")));
            return 0;
        }

        result = MciCdevInitialize(pDriver);
        
        if (result)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIDRV] SPI Cdev Init Failed!!\r\n")));

            if(pDriver)
            {
                MciOsal_FreeKernelMemory(pDriver);
                pDriver = NULL;
            }

            return result;
        }


        if (!MciDrvInitialize(pDriver , 0, NULL))
        {
            MciDrvRemoveInstance(pDriver); 

            if (!result)
            {           
//                unregister_chrdev(MCI_SPI_MAJOR, MCI_SPI_DEV_DRV_NAME);
				misc_deregister(&mci_spi_dev);                
            }

            pDriver = NULL;
        }

        g_pMciDriver = pDriver;

    }
    else
    {
        pDriver = g_pMciDriver;
    }

    pDriver->bCompleteDrvInit = TRUE;
    RETAILMSG(1, (TEXT("[MCIDRV]: -MCI_Init %x\r\n"), (UInt32)pDriver));

    return result;

}

static Void __exit MciDrv_exit_module( void )
{

    RETAILMSG(1,(TEXT("[MCIDRV] +Uninit MCIDRV Module!\r\n")));

    if(g_pMciDriver->iOpenedDrvCount == 0)
    {
        MciDrvRemoveInstance(g_pMciDriver);

        g_pMciDriver = NULL;
    }
    
//    unregister_chrdev(MCI_SPI_MAJOR, MCI_SPI_DEV_DRV_NAME);
	misc_deregister(&mci_spi_dev);

    RETAILMSG(1,(TEXT("[MCIDRV] -Uninit MCIDRV Module!\r\n")));

}

module_init(MciDrv_init_module);
module_exit(MciDrv_exit_module);
//module_param(debug, int, S_IRUGO);

MODULE_AUTHOR("Jong-Ho Park jongho0910.park@samsung.com");
MODULE_DESCRIPTION("MCI Device Driver Module for Mobile TV");
MODULE_LICENSE("GPL");
//DULE_LICENSE("Proprietary");
