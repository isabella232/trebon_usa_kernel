/* linux/driver/misc/mcispi/MciOsalApi_LINUX.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciOsalApi_LINUX.h
**
**  Description
**      Define OS Abstraction Layer APIs for Linux
**
**  Authour:
**      Charlie Park (jongho0910.park@samsung.com)
**
**  Update:
**      2009.02.10  NEW         Created.
**/

#ifndef _MCI_OSAL_API_LINUX_H_
#define _MCI_OSAL_API_LINUX_H_

#include "MciTypeDefs.h"

#ifndef _MCI_DEVICE_DRIVER_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>     // memset()
#include <unistd.h>     // sleep()
#include <sys/ioctl.h>  // ioctl()
#include <sys/types.h>
#include <sys/time.h>   // gettimeofday()
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>      // ETIMEOUT

//#include "utils/Log.h"
#else

#include <linux/interrupt.h>    /*request_irq(), enable_irq(), disable_irq(), free_irq()*/
#include <asm/io.h>             /*readl(), writel(), ioremap(), ioremap_nocache(), iounmap()*/
#include <linux/string.h>       /*memset, memcpy*/
#include <linux/module.h>       /*moule_init(), module_exit()*/
#include <linux/fs.h>           /*register_chrdev(), unregister_chrdev()*/
#include <linux/vmalloc.h>           /*register_chrdev(), unregister_chrdev()*/
#include <linux/slab.h>         /*kmalloc(), kfree()*/
#include <linux/dma-mapping.h>  /*dma_alloc_coherent(), dma_free_coherent()*/
//#include <asm-arm/semaphore.h>  /*sema_init(), up(), down()*/
#include <linux/semaphore.h>  /*sema_init(), up(), down() ==> for Android BSP 2.6.27 */
#include <linux/wait.h>         /*wake_up(), wait_event_interruptible(), wake_up_interruptible(), down_interruptible(), init_waitqueue_head()*/
#include <linux/workqueue.h>    /*INIT_WORK(), schedule_work() */
#include <linux/kernel.h>       /*container_of(), printk(), sprintf()*/
#include <linux/jiffies.h>      /*get_jiffies_64()*/
#include <linux/delay.h>        /*mdelay(), udelay()*/
#include <asm/uaccess.h>         /*copy_to_user(), copy_from_user()*/

#include <linux/sched.h>		/// TASK_INTERRUPTIBLE		2010.12.15
#include <mach/media.h>
#include <plat/media.h>
#define CONFIG_HAS_WAKELOCK 1
#define CONFIG_WAKELOCK_STAT 1
#include <linux/wakelock.h>
#include <linux/gpio.h>

#endif // _MCI_DEVICE_DRIVER_

/////////////////////////////////////////////////////////////////////////////
// Define Window-like Data types
/////////////////////////////////////////////////////////////////////////////
typedef char                    INT8;
typedef char *                  PINT8;

typedef unsigned char           UINT8;
typedef unsigned char *         PUINT8;

typedef short int               INT16;
typedef short int *             PINT16;
typedef unsigned short int      UINT16;
typedef unsigned short int *    PUINT16; 

typedef int                     INT32;
typedef int *                   PINT32;
typedef unsigned int            UINT32;
typedef unsigned int *          PUINT32;

typedef void *                  PVOID;
typedef void                    VOID;

typedef unsigned char *         PUCHAR;
typedef unsigned char           UCHAR;

typedef int                     BOOL;
typedef unsigned long           DWORD;


typedef char                    TCHAR;
typedef char*                   PCHAR;
typedef const char*             PCTCHAR;
typedef char                    TChar;
typedef char*                   pTChar;
typedef const char*             pCTChar;

typedef unsigned char*          PUINT;
typedef char*                   PTCHAR;
typedef pTChar                  pTStr;
typedef pVoid                   LPOVERLAPPED;

//typedef unsigned long           HANDLE;     


//  color ansi code for debug message
#define CLRDBLK     "\x1b[30m"
#define CLRDRED     "\x1b[31m"
#define CLRDGRN     "\x1b[32m"
#define CLRDYLW     "\x1b[33m"
#define CLRDBLU     "\x1b[34m"
#define CLRDMGT     "\x1b[35m"
#define CLRDCYN     "\x1b[36m"
#define CLRDWHT     "\x1b[37m"
#define CLRHBLK     "\x1b[1;30m"
#define CLRHRED     "\x1b[1;31m"
#define CLRHGRN     "\x1b[1;32m"
#define CLRHYLW     "\x1b[1;33m"
#define CLRHBLU     "\x1b[1;34m"
#define CLRHMGT     "\x1b[1;35m"
#define CLRHCYN     "\x1b[1;36m"
#define CLRHWHT     "\x1b[1;37m"
#define CLRBBLK     "\x1b[40m"
#define CLRBRED     "\x1b[41m"
#define CLRBGRN     "\x1b[42m"
#define CLRBYLW     "\x1b[43m"
#define CLRBBLU     "\x1b[44m"
#define CLRBMGT     "\x1b[45m"
#define CLRBCYN     "\x1b[46m"
#define CLRBWHT     "\x1b[47m"
#define CLRRST      "\x1b[0m"


/////////////////////////////////////////////////////////////////////////////
// Define Window-like Macros
/////////////////////////////////////////////////////////////////////////////
#ifndef RETAILMSG
    #ifdef _MCI_DEVICE_DRIVER_
        #define RETAILMSG(cond , Print_exp)  ((cond) ? (printk Print_exp), 1 : 0)
    #else
//        #define LOG_NDEBUG 0
         #define RETAILMSG(cond , Print_exp)  ((cond) ? (printf Print_exp), 1 : 0)
    #endif
#endif // RETAILMSG

//#define _T(x)                   ((char*)x)
#define _T(x)                   x
#define TEXT(x)                 x

#define S_OK                    0
#define E_FAIL                  -1
#define INVALID_HANDLE_VALUE    (HANDLE)-1
#define DELAY_TIME_MSEC                             (1*HZ)

#ifdef _MCI_DEVICE_DRIVER_
    #define Sleep(x)                mdelay(x)
#else
    #define Sleep(x)                sleep(x)
#endif
#define INFINITE                0xFFFF;

#ifdef _MCI_DEVICE_DRIVER_

typedef struct _MCIOSAL_CODELOCK
{
    spinlock_t   lock;
//	struct mutex lock;
    ULong	flags;
} MCIOSAL_CODELOCK, *PMCIOSAL_CODELOCK, **PPMCIOSAL_CODELOCK;

typedef struct _MCIOSAL_EVENT
{
    wait_queue_head_t   WaitQueue;
    UInt32              fMciDrvEvent;
    UInt32              uiEventData;
//    pTChar               pEventName;
} MCIOSAL_EVENT, *PMCIOSAL_EVENT, **PPMCIOSAL_EVENT;


typedef struct _MCIOSAL_PHYSMEM
{
    UInt32      uiSize;
    dma_addr_t  pDmaPhysAddr;
    pVoid       pDmaVirtAddr;
} MCIOSAL_PHYSMEM, *PMCIOSAL_PHYSMEM, **PPMCIOSAL_PHYSMEM;

#else

typedef struct _MCIOSAL_EVENT
{
    pthread_cond_t      g_hWaitEvent;
    pthread_mutex_t     g_mutex;
    struct timespec     timeout;
} MCIOSAL_EVENT, *PMCIOSAL_EVENT, **PPMCIOSAL_EVENT;

typedef struct _MCIOSAL_CODELOCK
{
    pthread_mutex_t     g_mutex;
} MCIOSAL_CODELOCK, *PMCIOSAL_CODELOCK, **PPMCIOSAL_CODELOCK;

typedef struct _MCIOSAL_THREAD
{
    pthread_t           hHandle;
    ULong               ulThreadId;
    pVoid               pStack;
    UInt32              uiStackSize;
    Int32               iPriority;
} MCIOSAL_THREAD, *PMCIOSAL_THREAD, **PPMCIOSAL_THREAD;

typedef struct _MCIOSAL_DEVICE
{
    UInt32              FW_fd;
    UInt32              uiIndex;
} MCIOSAL_DEVICE, *PMCIOSAL_DEVICE, **PPMCIOSAL_DEVICE;

typedef struct _MCIOSAL_FILE
{
//    HANDLE              hHandle;
} MCIOSAL_FILE, *PMCIOSAL_FILE, **PPMCIOSAL_FILE;
#endif

typedef void* H_DEVDLL;

#endif // _MCI_OSAL_API_LINUX_H_
