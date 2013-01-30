/* drivers/misc/mcispi/MciOsalApi_LINUX.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciOsalApi_LINUX.c
**
**  Description
**      Define OS Abstraction Layer APIs for Linux
**
**  Authour:
**      David Kim (hyenakim@samsung.com)
**      Charlie Park (jongho0910.park@samsung.com)
**
**  Update:
**      2009.02.10  NEW         Created.
**/

#include "MciOsalApi.h"

// header file for reserved memory
#include <mach/media.h>
#include <plat/media.h>


#ifdef _MCI_DEVICE_DRIVER_

#define KMALLOC(_x,_y)		vmalloc(_x)
#define KFREE(_x)					vfree(_x)

/*
 * Memory Allocation
 */
pVoid MciOsal_AllocKernelMemory(UInt32 uiSize)
{
    pUInt8 hMem = NULL;

//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsa_AllocKernelMemory\r\n")));

    if (uiSize)
    {
        printk("AllocKernel Memory!!\n");
        hMem = KMALLOC(uiSize, GFP_KERNEL);
        
        if (!hMem)
        {   
             RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory of Kernel!\r\n")));
        }
    }
    return (pVoid)hMem;
}
Void MciOsal_FreeKernelMemory(pVoid pObj)
{
    if (pObj)
    {
        KFREE(pObj);
    }
}

/*
 * DMA(Physical) Memory Allocation
 */

#define _MCI_USE_MEDIA_MEMORY_BANK  1

// This is defined in media.h, if kernel source is updated, this line shall be removed
#define S5P_MDEV_MOBITV         12  
#ifdef S5P_MDEV_MAX
#undef S5P_MDEV_MAX
#define S5P_MDEV_MAX 13
#endif 

H_PHYSMEM MciOsal_AllocPhysicalMemory(UInt32 uiSize, Bool bCacheEnble)
{
    H_PHYSMEM hMem = NULL;

//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsa_AllocPhysicalMemory\r\n")));

    do
    {
        if (uiSize == 0) break;

        hMem = (H_PHYSMEM)KMALLOC( sizeof(MCIOSAL_PHYSMEM), GFP_KERNEL);
        if (hMem == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory of Physical Memory Object\r\n")));
            break;
        }
        MciOsal_memset(hMem, 0, sizeof(MCIOSAL_PHYSMEM));
#if _MCI_USE_MEDIA_MEMORY_BANK
        hMem->pDmaPhysAddr  = s5p_get_media_memory_bank(S5P_MDEV_MOBITV, 1);
        hMem->uiSize        = s5p_get_media_memsize_bank(S5P_MDEV_MOBITV, 1);
        if ( hMem->pDmaPhysAddr == (dma_addr_t)NULL )
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't get reserved physical memory for DMA\r\n")));
            if (hMem)
            {
                KFREE(hMem);
                hMem = NULL;
            }
            break;
        }
        hMem->pDmaVirtAddr  = phys_to_virt( hMem->pDmaPhysAddr);
        RETAILMSG(1, (TEXT("DEBUG> Mem : 0x%08x Size : %x\r\n"),hMem->pDmaPhysAddr,hMem->uiSize));
#else // _MCI_USE_MEDIA_MEMORY_BANK == 0
        hMem->uiSize       = uiSize;
#if 1   // For SMDKC100
#if 0   
#ifdef _ANDROID_
////////////////////////////////////////////////////////////////////////////////
//        hMem->pDmaVirtAddr = ioremap_nocache( 0x55800000, uiSize); 
//        hMem->pDmaPhysAddr = 0x55800000;
        hMem->pDmaVirtAddr = ioremap_nocache( 0x57600000, uiSize); 
        hMem->pDmaPhysAddr = 0x57600000;

///////////////////////////////////////////////////////////////////////////////
#else
        hMem->pDmaVirtAddr = dma_alloc_coherent( NULL, uiSize, &hMem->pDmaPhysAddr, GFP_KERNEL | GFP_DMA ); 
#endif
#endif
        hMem->pDmaVirtAddr = dma_alloc_coherent( NULL, uiSize, &hMem->pDmaPhysAddr, GFP_KERNEL | GFP_DMA ); 
#endif

#if 0   //For SMDK6410
#ifdef _ANDROID_
        ////////////////////////////////////////////////////////////////////////////////
        //        hMem->pDmaVirtAddr = ioremap_nocache( 0x55800000, uiSize); 
        //        hMem->pDmaPhysAddr = 0x55800000;
                
                //SMDKC100
//              hMem->pDmaVirtAddr = ioremap_nocache( 0x57600000, uiSize); 
//              hMem->pDmaPhysAddr = 0x57600000;

                //SMDK6410
                hMem->pDmaVirtAddr = ioremap_nocache( 0x57800000 /*0x57800000 0x56000000 0x57600000*/, uiSize); 
                hMem->pDmaPhysAddr = 0x57800000;//0x56000000; //0x57800000; // 0x56000000; // 0x57600000;//0x56000000 ;//0x57600000;
        
        ///////////////////////////////////////////////////////////////////////////////
#else
                hMem->pDmaVirtAddr = dma_alloc_coherent( NULL, uiSize, &hMem->pDmaPhysAddr, GFP_KERNEL | GFP_DMA ); 
#endif
#endif

        if ( hMem->pDmaVirtAddr == NULL )
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate HAL Common (Physical) Buffer\r\n")));

            if (hMem)
            {
                KFREE(hMem);
                hMem = NULL;
            }

            break;
        }
#endif // _MCI_USE_MEDIA_MEMORY_BANK
    }while (FALSE);

    return hMem;

}

Void MciOsal_FreePhysicalMemory(H_PHYSMEM pObj)
{
    if (pObj)
    {
        if (pObj->pDmaVirtAddr)
        {

#if 0
#ifdef _ANDROID_    
            iounmap(pObj->pDmaVirtAddr);
            pObj->pDmaPhysAddr = NULL;

#else
            dma_free_coherent( NULL, pObj->uiSize, pObj->pDmaVirtAddr, pObj->pDmaPhysAddr);
            pObj->pDmaPhysAddr = NULL;
#endif
#endif

#if 0   //For SMDK6410
#if 1
#ifdef _ANDROID_
////////////////////////////////////////////////////////
            iounmap(pObj->pDmaVirtAddr);
            pObj->pDmaPhysAddr = NULL;
////////////////////////////////////////////////////////
#else
            dma_free_coherent( NULL, pObj->uiSize, pObj->pDmaVirtAddr, pObj->pDmaPhysAddr);
            pObj->pDmaPhysAddr = NULL;
#endif
#endif
#endif

#if (_MCI_USE_MEDIA_MEMORY_BANK == 0)
#if 1   //For SMDKC100     
        dma_free_coherent( NULL, pObj->uiSize, pObj->pDmaVirtAddr, pObj->pDmaPhysAddr);
        pObj->pDmaPhysAddr = (dma_addr_t)NULL;
#endif

        pObj->pDmaVirtAddr = NULL;
#endif //_MCI_USE_MEDIA_MEMORY_BANK
        }
        KFREE(pObj);
    }

}

pVoid MciOsal_GetVirtualAddress(H_PHYSMEM pObj)
{
    pVoid pAddr = NULL;
    if (pObj && pObj->pDmaVirtAddr)
    {
        pAddr = pObj->pDmaVirtAddr;
    }
    return pAddr;
}

pVoid MciOsal_GetPhysicalAddress(H_PHYSMEM pObj)
{
    pVoid pAddr = NULL;
    if (pObj)
    {
        pAddr = (pVoid)pObj->pDmaPhysAddr;
    }
    return pAddr;
}

/*
 * Alloc/Free Mapped H/W Register
 */

pVoid MciOsal_AllocHwRegister(pTChar pHelpText, UInt32 uiBaseAddr, UInt32 uiSize, Bool bIsPhysAddr, Bool bCacheEnable)
{
    pVoid   pRegAddr = NULL;
//    UInt32  uiAddr   = uiBaseAddr;
    do
    {
        if (pHelpText == NULL)
        {
            pHelpText = _T("Register");
        }

        pRegAddr = phys_to_virt(uiBaseAddr);
        if (pRegAddr == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't phys to virt '%s' register memory\r\n"), pHelpText));
            pRegAddr = NULL;
            break;
        }

        RETAILMSG(1, (TEXT("[MCIOSAL] %s: pRegAddr = 0x%08X, pBaseAddr = 0x%08X, size = %d, bIsPhys=%s, bCache=%s\r\n")
                    , pHelpText, (UInt32)pRegAddr, uiBaseAddr, uiSize
                    , (bIsPhysAddr ? _T("TRUE") : _T("FALSE"))
                    , (bCacheEnable ? _T("TRUE") : _T("FALSE"))));

    } while (FALSE);

    return pRegAddr;
}
Void MciOsal_FreeHwRegister(pTChar pHelpText, pVoid pRegAddr)
{
    if (pRegAddr)
    {
        if (pHelpText)
        {
            RETAILMSG(1, (TEXT("[MCIOSAL] %s: Freed\r\n"), pHelpText));
        }
    }

}
pVoid MciOsal_AllocHwBankRegister(pTChar pHelpText, UInt32 uiBaseAddr, UInt32 uiTotalSize, UInt32 uiBankIndex, UInt32 uiBankSize, Bool bIsPhysAddr, Bool bCacheEnable)
{

    pVoid   pRegAddr = NULL;
    do
    {
        pRegAddr = MciOsal_AllocHwRegister(pHelpText, uiBaseAddr, uiTotalSize, bIsPhysAddr, bCacheEnable);
        if (pRegAddr)
        {
            pRegAddr = (pVoid)((UInt32)pRegAddr + uiBankIndex * uiBankSize);
        }
    } while (FALSE);
    return pRegAddr;
}
Void MciOsal_FreeHwBankRegister(pTChar pHelpText, pVoid pRegAddr, UInt32 uiBankIndex, UInt32 uiBankSize)
{
    if (pRegAddr)
    {
        if (pHelpText)
        {
            RETAILMSG(1, (TEXT("[MCIOSAL] %s: Freed\r\n"), pHelpText));
        }
        pRegAddr = (pVoid)((UInt32)pRegAddr - uiBankIndex * uiBankSize);
    }

}
pVoid MciOsal_MemoryMapIoSpace(pVoid pPhysicalAddress, UInt32 uiSize, Bool bCacheEnable)
{
    Void __iomem    *pRegAddr = NULL;

    pRegAddr = ioremap((UInt32)pPhysicalAddress, uiSize);

    if (pRegAddr == NULL)
    {
        RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't map Physical Address of Register\r\n")));
    }
    return pRegAddr;

}
Void MciOsal_MemoryUnmapIoSpace(pVoid pBaseAddr, UInt32 uiSize)
{
    if (pBaseAddr)
    {
        iounmap(pBaseAddr);
        pBaseAddr = NULL;
    }
}

/*
 * Code Lock Object (Criticial Section, Mutex, Semaphore, etc)
 */
H_LOCK MciOsal_CreateCodeLockObject(void)
{
    H_LOCK hLock = NULL;

//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsa_CreateCodeLockObject\r\n")));
    
    do
    {
        hLock = (H_LOCK)KMALLOC( sizeof(MCIOSAL_CODELOCK), GFP_KERNEL);
        
        if (hLock == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory of Code Lock Object\r\n")));
            break;
        }


    } while (FALSE);
    
    return hLock;
}
H_LOCK MciOsal_InitCodeLockObject(H_LOCK hLock)
{

//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsa_InitCodeLockObject\r\n")));

    if (hLock)
    {
        spin_lock_init((spinlock_t*)&(hLock->lock));
    
        return hLock;
    }

    return hLock;
}

Void MciOsal_ReleaseCodeLockObject(H_LOCK hLock)
{
    if (hLock)
    {
        KFREE(hLock);
    }
}
Void MciOsal_EnterCodeLock(H_LOCK hLock)
{

#if 1
    if (hLock)
    {
	    if(spin_is_locked((spinlock_t*)&hLock->lock) == 0)
	    {
     		spin_lock_irqsave((spinlock_t*)&hLock->lock, hLock->flags);
	    }
    }
#endif
}

Void MciOsal_LeaveCodeLock(H_LOCK hLock)
{
#if 1
    if (hLock)
    {
	    if(spin_is_locked(&hLock->lock))
            spin_unlock_irqrestore(&hLock->lock, hLock->flags);

    }
#endif
}

/*
 * Event Object
 */

H_EVENT MciOsal_CreateEventObject(pTChar pEventName, Bool bOpenExisting, BOOL bManualReset)
{
    H_EVENT hEvent = NULL;

//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsal_CreateEventObject\r\n")));

    do
    {
        if (bOpenExisting && pEventName == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Missed the Event Name\r\n")));
            break;
        }

        hEvent = (H_EVENT)KMALLOC( sizeof(MCIOSAL_EVENT), GFP_KERNEL);
        if (hEvent == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory of Event Object\r\n")));
            break;
        }

        init_waitqueue_head(&hEvent->WaitQueue);

        hEvent->fMciDrvEvent = 0;

    }while (FALSE);

    return hEvent;

}

Void MciOsal_ReleaseEventObject(H_EVENT hEvent)
{
//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsal_ReleaseEventObject\r\n")));

    if (hEvent)
    {
        KFREE(hEvent);
    } 
}

EMCI_EVENT MciOsal_WaitForEvent(H_EVENT hEvent, UInt32 uiTimeout)
{
    ULong   ulRet = EMCI_EVENT_WAIT_FAILED;
    
    if (hEvent)
    {
        //RETAILMSG(0, (TEXT("[MCIOSAL-Driver] Enter MciOsal_WaitForEvent = 0x%08X\r\n"), hEvent));

//        interruptible_sleep_on(&(hEvent->WaitQueue));

        if (wait_event_interruptible(hEvent->WaitQueue, (hEvent->fMciDrvEvent == 1)))
        {
            RETAILMSG(1, (TEXT("[MCIOSAL-Driver-ERROR] Wait_event_interruptible failed!! hEvent=0x%08X\r\n"), (UInt32)hEvent));
            MciOsal_Sleep(5);
            return ulRet;
        }
        ulRet = EMCI_EVENT_OBJECT_0;

        //RETAILMSG(0, (TEXT("[MCIOSAL-Driver] Waked up !!!!! = 0x%08X\r\n"), hEvent));

//        hEvent->fMciDrvEvent = 0;
        return ulRet;
    }

    return ulRet;

}

Bool MciOsal_SetEvent(H_EVENT hEvent)
{
    Bool bRet = FALSE;
    
//    RETAILMSG(1, (TEXT("[MCIOSAL-Driver] Enter MciOsal_SetEvent\r\n")));

    if (hEvent)
    {
        RETAILMSG(0, (TEXT("[MCIOSAL-Driver] Enter MciOsal_SetEvent == 0x%08X\r\n"), (UINT32)hEvent));
        hEvent->fMciDrvEvent = 1;
        wake_up_interruptible( &(hEvent->WaitQueue));
        bRet = TRUE;
    }
    return bRet;
}
Bool MciOsal_ResetEvent(H_EVENT hEvent)
{
    Bool bRet = FALSE;
    
    if (hEvent)
    {
        hEvent->fMciDrvEvent = 0;
        bRet = TRUE;
    }
    return bRet;
}

Bool MciOsal_SetEventData(H_EVENT hEvent, UInt32 uiData)
{
    Bool bRet = FALSE;

    hEvent->uiEventData = 0;

    if (hEvent)
    {
        hEvent->uiEventData = uiData;
        bRet = TRUE;
    }
    return bRet;


}

UInt32 MciOsal_GetEventData(H_EVENT hEvent)
{
    UInt32 uiData = 0;

    if (hEvent)
    {
        uiData = hEvent->uiEventData;
        hEvent->uiEventData = 0;
    }
    return uiData;


}

Int32 MciOsal_CopyToUser(pVoid to, pVoid from, UInt32 len)
{
    UInt32 ret =0;

//    ret = copy_to_user((void __user *)to, from, len);
    ret = copy_to_user(to, from, len);
        
//    printk("[MCIDRV] Copy_to_User return value = %d\n", ret);

    return ret;
}
Int32 MciOsal_CopyFromUser(pVoid to, pVoid from, UInt32 len)
{

    UInt32 ret =0;

    ret = copy_from_user(to, from, len);
    return ret;
}

pVoid MciOsal_MapUserModeBuffer(pVoid pBuf, UInt32 len)
{
    
    pUChar pKernelBuf = NULL;
    int    iRet;
    if (pBuf != NULL)
    {
        pKernelBuf = (pUChar)KMALLOC(len, GFP_ATOMIC);
        if (pKernelBuf != NULL)
        {
            iRet = copy_from_user((pVoid)pKernelBuf, pBuf, len);
            return (pVoid)pKernelBuf;
        }
        
        if (pKernelBuf == NULL)
        {
           RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory of MapUserModeBuffer!\r\n")));
        }

    }
    return (pVoid)pKernelBuf;
}
Void MciOsal_UnmapUserModeBuffer(pVoid pBuf)
{
    if (pBuf != NULL)
    {
        KFREE(pBuf);
        pBuf = NULL;
    }
    return;
}

UInt32 MciOsal_GetTickCount(void)
{
//    return (UInt32)get_jiffies_64();
    return (UInt32)jiffies;

}

UInt32 MciOsal_GetLastError(void)
{
  return -1;
}

Void  MciOsal_Sleep(UInt32 uiMillisecond)
{

    msleep(uiMillisecond*100);

}

#else
pVoid MciOsal_AllocMemory(UInt32 uiSize)
{
    pVoid   hMem;
    if (uiSize)
    {
        hMem = malloc(uiSize);
        
        if ( !hMem )
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory ==>MciOsal_AllocMemory!\r\n")));
 
        }
    }
    return hMem;
}

Void MciOsal_FreeMemory(pVoid pObj)
{
    if (pObj)
    {
        free(pObj);
    }
}

/*
 * Event Object
 */

H_EVENT MciOsal_CreateEventObject(pTChar pEventName, Bool bOpenExisting, BOOL bManualReset)
{
    H_EVENT hEvent = NULL;

    do
    {
    	RETAILMSG(1, (TEXT("[MCIOSAL] Create EventObject, pEventName=%s\r\n"), (pEventName) ? pEventName : _T("NULL")));
        if (bOpenExisting && pEventName == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Missed the Event Name\r\n")));
            break;
        }

        hEvent = (H_EVENT)malloc( sizeof(MCIOSAL_EVENT));
        if (hEvent == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory ==> MciOsal_CreateEventObject!!\r\n")));
            break;
        }
		//RETAILMSG(1, (TEXT("[CHECK-CreateEventObject-1]\r\n")));
        pthread_cond_init((pthread_cond_t*)&hEvent->g_hWaitEvent, NULL);
		//RETAILMSG(1, (TEXT("[CHECK-CreateEventObject-2]\r\n")));
        pthread_mutex_init((pthread_mutex_t*)&hEvent->g_mutex, NULL);
		//RETAILMSG(1, (TEXT("[CHECK-CreateEventObject-3]\r\n")));

    }while (FALSE);

    return hEvent;

}

Void MciOsal_ReleaseEventObject(H_EVENT hEvent)
{
    if (hEvent)
    {
        pthread_cond_destroy((pthread_cond_t*)&hEvent->g_hWaitEvent);
        free(hEvent);
    } 
}

ULong MciOsal_WaitForEvent(H_EVENT hEvent, UInt32 uiTimeout)
{
    INT32   ulRet = FALSE;
    struct timeval now;
    struct timespec timeout;

//    pthread_mutex_lock(&hEvent->g_mutex);

    gettimeofday(&now, NULL);
    timeout.tv_sec = now.tv_sec + (uiTimeout/1000);
    timeout.tv_nsec = now.tv_usec * 1000;
    
    if (hEvent)
    {
//        RETAILMSG(1, (TEXT("[MCIOSAL] +++pthread_cond_timedwait  start +++\n")));

        pthread_mutex_lock((pthread_mutex_t*)&hEvent->g_mutex);

        ulRet = pthread_cond_timedwait((pthread_cond_t*)&hEvent->g_hWaitEvent, (pthread_mutex_t*)&hEvent->g_mutex, &timeout);

        if (ulRet == EINVAL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] pthread_cond_timedwait Invalid argument:%d\r\n"),ulRet));
            pthread_mutex_unlock((pthread_mutex_t*)&hEvent->g_mutex);
            return ulRet;
        }
        if (ulRet == ETIMEDOUT)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] pthread_cond_timedwait TimeOut!!! :%d\r\n"),ulRet));

            pthread_mutex_unlock((pthread_mutex_t*)&hEvent->g_mutex);
            ulRet = EMCI_EVENT_TIMEOUT;
            return ulRet;
        }
        
        pthread_mutex_unlock((pthread_mutex_t*)&hEvent->g_mutex);

//        RETAILMSG(1, (TEXT("[MCIOSAL] ---pthread_cond_timedwait  end ---\n")));

        ulRet = TRUE;
        return ulRet;
    }

    return ulRet;
}

Bool MciOsal_SetEvent(H_EVENT hEvent)
{
    Bool bRet = FALSE;
    
    if (hEvent)
    {
        usleep(80000);
//        printf("++++++++++++Signal hWaitForReplyEvent++++++++++++++++++\n");
//        sleep(2);
        pthread_cond_signal(&hEvent->g_hWaitEvent);

//        usleep(3000);

        bRet = TRUE;
        return bRet;
    }
    return bRet;
}

Bool MciOsal_SetEventData(H_EVENT hEvent, UInt32 uiData)
{
    Bool bRet = FALSE;

    if (hEvent)
    {
        bRet = TRUE;
    }
    return bRet;


}

Bool MciOsal_ResetEvent(H_EVENT hEvent)
{
    Bool bRet = FALSE;
    
    if (hEvent)
    {
        bRet = TRUE;
    }
    return bRet;
}


/*
 * Thread Object 
 */
H_THREAD MciOsal_CreateTaskObject(pTChar pTaskName, LPTASK_START_ROUTINE fnTaskProc, pVoid pTaskArg, pVoid pStack, UInt32 uiStackSize, Int32 iPriority)
{
    H_THREAD hThread = NULL;
    RETAILMSG(1, (TEXT("[MCICONTROL] 22222\r\n")));

    do
    {
        hThread = (H_THREAD)malloc(sizeof(MCIOSAL_THREAD));
        if (hThread == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory ==> MciOsal_CreateTaskObject!!\r\n")));
            break;
        }
        RETAILMSG(1, (TEXT("[MCICONTROL] 22222-1\r\n")));

        hThread->pStack         = pStack;
        hThread->uiStackSize    = uiStackSize;
        hThread->iPriority      = iPriority;

        Int32   err;
//        struct sched_param schedparam;

//        schedparam.sched_priority = 30;
        
        pthread_attr_t thread_attr;
        pthread_attr_init(&thread_attr);
        RETAILMSG(1, (TEXT("[MCICONTROL] 22222-2\r\n")));
//        pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
//        pthread_attr_setschedparam(&thread_attr, &schedparam);
        pthread_attr_setdetachstate(&thread_attr , PTHREAD_CREATE_DETACHED);
        RETAILMSG(1, (TEXT("[MCICONTROL] 22222-3\r\n")));
        
//        RETAILMSG(1, (TEXT("[MCICONTROL] 22222-2\r\n")));
        err = pthread_create((pthread_t*)&hThread->hHandle, &thread_attr, fnTaskProc , pTaskArg);
		
        RETAILMSG(1, (TEXT("[MCICONTROL] 22222-4\r\n")));
		
        if (err !=0)
        {
            pthread_attr_destroy(&thread_attr);
        }
        else
            pthread_attr_destroy(&thread_attr);        

    }while(FALSE);
    
    return hThread;
}
Void MciOsal_ReleaseTaskObject(H_THREAD hThread)
{
    if (hThread)
    {
        free(hThread);
    }
}

Void MciOsal_TerminateTask(H_THREAD hThread, UInt32 uiExitCode)
{
    //sizkim    pthread_exit(0);
    if (hThread && hThread->hHandle)
    {
	    pthread_detach(hThread->hHandle);
    }
}

Bool MciOsal_GetExitCodeTask(H_THREAD hThread, pUInt32 puiExitCode)
{
    if (hThread && hThread->hHandle)
    {
        return pthread_join(hThread->hHandle, (Void *)puiExitCode);
    }
    return FALSE;
}
Int32 MciOsal_GetTaskPriority(H_THREAD hThread)
{
    return (hThread->iPriority);
}

Bool MciOsal_SetTaskPriority(H_THREAD hThread, Int32 iPriority)
{
#if 0    
    UInt32  ret;

    struct sched_param schedparam;

    schedparam.sched_priority = iPriority;

    ret = sched_setscheduler(hThread->ulThreadId,  SCHED_RR, &schedparam);

    if ( ret )
    {
         RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] sched_setscheduler failed!! ret : %d, hThread->ulThread : %d\r\n"),ret, hThread->ulThreadId));
         return FALSE;
    }
    RETAILMSG(1, (TEXT("[MCIOSAL] sched_setscheduler success!!\r\n")));
#endif
    return TRUE;
}


/*
 * Device Driver Operations
 */
//H_DRIVER MciOsal_CreateLinuxDriverObject(pTChar pDriverKey, pTChar pPrefix, UInt32 uiIndex)
H_DRIVER MciOsal_CreateDriverObject(pTChar pDriverKey, pTChar pPrefix, UInt32 uiIndex)
{

//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsal_CreateDriverObject\r\n")));

    H_DRIVER    hDriver = NULL;
    Bool        bRet = FALSE;
    do
    {
        if (pPrefix == NULL)
        {
            break;
        }

        hDriver = (H_DRIVER)malloc(sizeof(MCIOSAL_DEVICE));
        if (hDriver == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory ==> MciOsal_CreateDriverObject!!\r\n")));
            break;
        }
        hDriver->FW_fd      = 0;
        hDriver->uiIndex = uiIndex;

        bRet = TRUE;
//        RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsal_CreateDriverObject11111111111111\r\n")));


    }while (FALSE);

    if (bRet == FALSE && hDriver)
    {
//        RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsal_CreateDriverObject2222\r\n")));
        free(hDriver);
        hDriver = NULL;
    }
//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsal_CreateDriverObject3333\r\n")));

    return hDriver;

}
Void MciOsal_ReleaseDriverObject(H_DRIVER hDriver)
{
    if (hDriver)
    {
        if (hDriver->FW_fd)
        {
            hDriver->FW_fd = 0;
        }

        free(hDriver);
        hDriver = NULL;
    }
}
Void MciOsal_EnterCodeLock(H_LOCK hLock)
{
    if (hLock)
    {
        pthread_mutex_lock(&hLock->g_mutex);
    }
}

Void MciOsal_LeaveCodeLock(H_LOCK hLock)
{
    if (hLock)
    {
        pthread_mutex_unlock(&hLock->g_mutex);
    }
}

H_LOCK MciOsal_CreateCodeLockObject(void)
{
    H_LOCK hLock = NULL;

//    RETAILMSG(1, (TEXT("[MCIOSAL] Enter MciOsa_CreateCodeLockObject\r\n")));
    
    do
    {
        hLock = (H_LOCK)malloc( sizeof(MCIOSAL_CODELOCK));
        
        if (hLock == NULL)
        {
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Can't allocate the memory of Code Lock Object\r\n")));
            break;
        }
        pthread_mutex_init(&hLock->g_mutex, NULL);

    } while (FALSE);
    
    return hLock;
}

Void MciOsal_ReleaseCodeLockObject(H_LOCK hLock)
{
    if (hLock)
    {
	    pthread_mutex_destroy(&hLock->g_mutex);
        free(hLock);
    }
}

#define MCI_SPI_DEV_NAME      "/dev/MciSpi"

Bool MciOsal_DevOpen(H_DRIVER hDriver, Bool bLoadDLL)
{
    Bool  bRet = FALSE;
//    TChar DevName[20];

    do
    {
        if (hDriver == NULL)
        {
            break;
        }
        if (bLoadDLL)
        {

    		hDriver->FW_fd = open( MCI_SPI_DEV_NAME, O_RDWR);        
//    		hDriver->FW_fd = open( "/dev/MciSpi", O_RDWR);        


            if ( (hDriver->FW_fd) <=0)
            {
                RETAILMSG(1, (TEXT("[MCIOSAL] /dev/MciSpi open Error!\r\n")));
                hDriver->FW_fd = 0;
                break;
            }
            RETAILMSG(1,(TEXT("[MCIOSAL] /dev/MciSpi Open Success!! FW_fd = %d\r\n"),hDriver->FW_fd));

            bRet = TRUE;
        }
    }while (FALSE);

    if (bRet == FALSE && hDriver)
    {
        if (hDriver->FW_fd)
        {
            hDriver->FW_fd = 0;
        }
    }

    return bRet;
}
Bool MciOsal_DevClose(H_DRIVER hDriver)
{
    Int32   ret = FALSE;
    if (hDriver)
    {
		RETAILMSG(1, (TEXT("[MciOsal] DevClose hDriver->FW_fd = 0x%08X\r\n"), hDriver->FW_fd));
        if (hDriver->FW_fd)
        {
            UInt32 tmpFW_fd = hDriver->FW_fd;
            hDriver->FW_fd = 0;
            ret = close(tmpFW_fd);
            
            if (ret)
            {
                RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Device Driver File Close Failed!!\r\n")));
                return FALSE;
            }
        }
    }
    return TRUE;

}
Bool MciOsal_DevRead(H_DRIVER hDriver, pVoid pBuffer, UInt32 uiBufLen, pUInt32 puiBytesRead)
{
    Bool bRet = FALSE;
    if (hDriver && hDriver->FW_fd)
    {
        *puiBytesRead = read(hDriver->FW_fd, pBuffer, uiBufLen );
        bRet = TRUE;
    }
   
    return bRet;
}
Bool MciOsal_DevWrite(H_DRIVER hDriver, pVoid pBuffer, UInt32 uiBufLen, pUInt32 puiBytesWritten)
{
    Bool bRet = FALSE;
    if (hDriver && hDriver->FW_fd)
    {
        *puiBytesWritten = write(hDriver->FW_fd, pBuffer, uiBufLen);
        bRet = TRUE;
    }
    return bRet;
}
//MCI_IOCTL_ARG   Mci_ioctl_arg;
Bool MciOsal_DevIoControl(H_DRIVER hDriver, ULong uiCmd, pVoid pIn, ULong uiInLen, pVoid pOut, ULong uiOutLen, pULong puiBytesReturn)
{
    Bool bRet = FALSE;

    MCI_IOCTL_ARG   Mci_ioctl_arg;

    Mci_ioctl_arg.pInBuf        = pIn;
    Mci_ioctl_arg.pOutBuf       = pOut;
    Mci_ioctl_arg.uiInBufLen    = uiInLen;
    Mci_ioctl_arg.uiOutBufLen   = uiOutLen;
    Mci_ioctl_arg.puiWritten    = puiBytesReturn;
    
    if (hDriver && hDriver->FW_fd)
    {
//        printf("[MCIOSAL] Send Ioctl ulIoControlCode =%d,   FW_fd =%d\n", uiCmd, hDriver->FW_fd);

        bRet = ioctl(hDriver->FW_fd, uiCmd, &Mci_ioctl_arg);

        if (!bRet)
        {            
            RETAILMSG(1, (TEXT("[ERROR!MCIOSAL] Ioctl Request Failed!! bRet = %d\r\n"), bRet));
            return bRet;
        }
        else
        {
//            RETAILMSG(0, (TEXT("[MCIOSAL] Ioctl Request !! bRet = %d\r\n"), bRet));
                        
//            RETAILMSG(1, (TEXT("[MCIOSAL] Ioctl Request success Mci_ioctl_arg.pInBuf = 0x%08X\r\n"), *(Mci_ioctl_arg.pInBuf)));

#if 0
            pIn      = Mci_ioctl_arg.pInBuf;
            pOut     = Mci_ioctl_arg.pOutBuf;
            uiInLen  = Mci_ioctl_arg.uiInBufLen;
            uiOutLen = Mci_ioctl_arg.uiOutBufLen;
            puiBytesReturn = Mci_ioctl_arg.puiWritten;
#endif
        }
        bRet = TRUE;
    }

    return bRet;
}
Bool MciOsal_IsValidDevHandle(H_DRIVER hDriver)
{
	Bool	bRet = FALSE;
	do 
	{
		if (hDriver == NULL)
		{
			break;
		}
		if (hDriver->FW_fd<0)
		{
			break;
		}
		bRet = TRUE;
	} while(FALSE);

	return bRet;

}
UInt32 MciOsal_GetTickCount(void)
{
//    return (UInt32)get_jiffies_64();
    return (UInt32)0;

}
UInt32 MciOsal_GetLastError(void)
{
  return E_FAIL;
}

Void  MciOsal_Sleep(UInt32 uiMillisecond)
{
  sleep(uiMillisecond);

}

#endif
