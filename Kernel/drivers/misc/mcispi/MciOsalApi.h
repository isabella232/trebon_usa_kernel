/* linux/driver/misc/mcispi/MciOsalApi.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciOsalApi.h
**
**  Description
**      Include Header file for OS Abstraction API
**
**  Authur:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.20  NEW     Created.
**/

#ifndef _MCI_OSAL_API_H_
#define _MCI_OSAL_API_H_

#include "MciOsalApi_DEFINE.h"
#include "MciTypeDefs.h"






/*************************************************************************/
/*                   OS Specific Object                                  */
/*************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*
 * Memory Allocation
 */
extern pVoid        MciOsal_AllocMemory(UInt32 uiSize);
extern Void         MciOsal_FreeMemory(pVoid pObj);
extern pVoid        MciOsal_AllocKernelMemory(UInt32 uiSize);
extern Void         MciOsal_FreeKernelMemory(pVoid pObj);
/*
 * DMA(Physical) Memory Allocation
 */
#ifdef _MCI_DEVICE_DRIVER_
extern H_PHYSMEM    MciOsal_AllocPhysicalMemory(UInt32 uiSize, Bool bCacheEnble);
extern Void         MciOsal_FreePhysicalMemory(H_PHYSMEM pObj);
extern pVoid        MciOsal_GetVirtualAddress(H_PHYSMEM pObj);
extern pVoid        MciOsal_GetPhysicalAddress(H_PHYSMEM pObj);
#endif // _MCI_DEVICE_DRIVER_

/*
 * Alloc/Free Mapped H/W Register
 */
#ifdef _MCI_DEVICE_DRIVER_
extern pVoid        MciOsal_AllocHwRegister(pTChar pHelpText, UInt32 uiBaseAddr, UInt32 uiSize, Bool bIsPhysAddr, Bool bCacheEnable);
extern Void         MciOsal_FreeHwRegister(pTChar pHelpText, pVoid pRegAddr);
extern pVoid        MciOsal_AllocHwBankRegister(pTChar pHelpText, UInt32 uiBaseAddr, UInt32 uiTotalSize, UInt32 uiBankIndex, UInt32 uiBankSize, Bool bIsPhysAddr, Bool bCacheEnable);
extern Void         MciOsal_FreeHwBankRegister(pTChar pHelpText, pVoid pRegAddr, UInt32 uiBankIndex, UInt32 uiBankSize);
extern pVoid        MciOsal_MemoryMapIoSpace(pVoid pPhysicalAddress, UInt32 uiSize, Bool bCacheEnable);
extern Void         MciOsal_MemoryUnmapIoSpace(pVoid pBaseAddr, UInt32 uiSize);
#endif // _MCI_DEVICE_DRIVER_


/*
 * Code Lock Object (Criticial Section, Mutex, Semaphore, etc)
 */
extern H_LOCK       MciOsal_CreateCodeLockObject(void);
extern H_LOCK       MciOsal_InitCodeLockObject(H_LOCK hLock);
extern Void         MciOsal_ReleaseCodeLockObject(H_LOCK hLock);
extern Void         MciOsal_EnterCodeLock(H_LOCK hLock);
extern Void         MciOsal_LeaveCodeLock(H_LOCK hLock);

/*
 * Event Object
 */
typedef enum 
{
    EMCI_EVENT_OBJECT_0         = 0,
    EMCI_EVENT_TIMEOUT          = 256,
    EMCI_EVENT_WAIT_FAILED      = 0xFFFFFFFF,
    EMCI_EVENT_WAIT_INFINITE    = 0xFFFFFFFF,
} EMCI_EVENT;

#ifdef _MCI_DEVICE_DRIVER_
extern EMCI_EVENT   MciOsal_WaitForEvent(H_EVENT hEvent, UInt32 uiTimeout);
#else
extern ULong        MciOsal_WaitForEvent(H_EVENT hEvent, UInt32 uiTimeout);
#endif
extern Bool         MciOsal_SetEvent(H_EVENT hEvent);
extern Bool         MciOsal_ResetEvent(H_EVENT hEvent);
extern Bool         MciOsal_SetEventData(H_EVENT hEvent, UInt32 uiData);
extern UInt32       MciOsal_GetEventData(H_EVENT hEvent);
extern H_EVENT      MciOsal_CreateEventObject(pTChar pEventName, Bool bOpenExisting, BOOL bManualReset);
extern Void         MciOsal_ReleaseEventObject(H_EVENT hEvent);


/*
 * Thread Object 
 */
typedef ULong (*LPTASK_START_ROUTINE)(pVoid pArg);
extern H_THREAD     MciOsal_CreateTaskObject(pTChar pTaskName, LPTASK_START_ROUTINE fnTaskProc, pVoid pTaskArg, pVoid pStack, UInt32 uiStackSize, Int32 iPriority);
extern Void         MciOsal_ReleaseTaskObject(H_THREAD hThread);
extern Void         MciOsal_TerminateTask(H_THREAD hThread, UInt32 uiExitCode);
extern Bool         MciOsal_GetExitCodeTask(H_THREAD hThread, pUInt32 puiExitCode);
extern Int32        MciOsal_GetTaskPriority(H_THREAD hThread);
extern Bool         MciOsal_SetTaskPriority(H_THREAD hThread, Int32 iPriority);

/*
 * I/O Control Operation
 */
typedef struct _MCI_IOCTL_ARG
{
//    pUChar      pInBuf;
    pVoid       pInBuf;
//    UInt32      uiInBufLen;
    ULong       uiInBufLen;

//    pUChar      pOutBuf;
    pVoid      pOutBuf;

//    UInt32      uiOutBufLen;
    ULong      uiOutBufLen;

//    pUInt32     puiWritten;
    pULong     puiWritten;

} MCI_IOCTL_ARG, *PMCI_IOCTL_ARG, **PPMCI_IOCTL_ARG;
#ifdef _MCI_OSAL_LINUX_
//extern H_DRIVER     MciOsal_CreateLinuxDriverObject(pTChar pDriverKey, pTChar pPrefix, UInt32 uiIndex);
extern H_DRIVER     MciOsal_CreateDriverObject(pTChar pDriverKey, pTChar pPrefix, UInt32 uiIndex);
#endif
extern Void         MciOsal_ReleaseDriverObject(H_DRIVER hDriver);
extern Bool         MciOsal_DevOpen(H_DRIVER hDriver, Bool bLoadDLL);
extern Bool         MciOsal_DevClose(H_DRIVER hDriver);
extern Bool         MciOsal_DevRead(H_DRIVER hDriver, pVoid pBuffer, UInt32 uiBufLen, pUInt32 puiBytesRead);
extern Bool         MciOsal_DevWrite(H_DRIVER hDriver, pVoid pBuffer, UInt32 uiBufLen, pUInt32 puiBytesWritten);
extern Bool         MciOsal_DevIoControl(H_DRIVER hDriver, ULong uiControlCode, pVoid pIn, ULong uiInLen, pVoid pOut, ULong uiOutLen, pULong puiBytesReturn);
extern Bool         MciOsal_IsValidDevHandle(H_DRIVER hDriver);

/*
 * String/Memory Operations
 */
#ifndef _MCI_OSAL_API_WINCE_
#define _tcscpy                             strcpy
#define _stprintf                           sprintf
#define _tcslen                             strlen
#endif
#define MciOsal_memcpy                      memcpy
#define MciOsal_memset                      memset
#define MciOsal_strcpy                      _tcscpy
#define MciOsal_sprintf                     _stprintf
#define MciOsal_strlen                      _tcslen

#ifdef _MCI_DEVICE_DRIVER_
extern Int32        MciOsal_CopyToUser(pVoid to, pVoid from, UInt32 len);
extern Int32        MciOsal_CopyFromUser(pVoid to, pVoid from, UInt32 len);
extern pVoid        MciOsal_MapUserModeBuffer(pVoid pBuf, UInt32 len);
extern Void         MciOsal_UnmapUserModeBuffer(pVoid pBuf);
extern UInt32       MciOsal_GetTickCount(void);
extern UInt32       MciOsal_GetLastError(void);
extern Void         MciOsal_Sleep(UInt32 uiMillisecond);


/* macro function for Copy(To/From)User operation */
#define MciOsal_GetUser(kern_var, user_ptr) MciOsal_CopyFromUser((pVoid)&(kern_var), (pVoid)user_ptr, (UInt32)sizeof(kern_var))
#define MciOsal_PutUser(kern_var, user_ptr) MciOsal_CopyToUser((pVoid)user_ptr, (pVoid)&(kern_var), (UInt32)sizeof(kern_var))

#else //MCI_DEVICE_DRIVER_

extern UInt32       MciOsal_GetTickCount();
extern UInt32       MciOsal_GetLastError();
extern Void         MciOsal_Sleep(UInt32 uiMillisecond);
#endif

#ifdef __cplusplus
}
#endif

#endif // _MCI_OSAL_API_H_
