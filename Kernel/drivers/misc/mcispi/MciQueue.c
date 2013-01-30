/* drivers/misc/mcispi/MciQueue.c
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciQueue.c
**
**  Description
**      Implement the function of MCI_QUEUE
**
**  Author:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.12.12  NEW     Created.
**/

#include "MciOsalApi.h"
#include "MciQueue.h"

#define _MCI_DBG_CHECK_CODE_LOCK 0

Void MciQueueInit(PMCI_QUEUE pQueue, H_LOCK hLock, pTChar strHelper)
{
    if (pQueue == NULL) return;

    if (hLock) 
    {
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Enter] %s\r\n"), strHelper));
        MciOsal_EnterCodeLock(hLock);
    }

    pQueue->pHead = pQueue->pTail = NULL;
    pQueue->iItemCnt = 0;
    
    if (hLock)
    {
        MciOsal_LeaveCodeLock(hLock);
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Leave] %s\r\n"), strHelper));
    }
    return;

}

Void MciQueuePushTail(PMCI_QUEUE pQueue, PMCI_QITEM pQItem, H_LOCK hLock, pTChar strHelper)
{
    PMCI_QITEM pLastQItem = NULL;
    
    if (pQueue == NULL) return;
    if (pQItem == NULL) return;

    if (hLock) 
    {
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Enter] %s\r\n"), strHelper));
        MciOsal_EnterCodeLock(hLock);
    }
    pLastQItem = pQItem;
    while (MCI_QITEM_GET_NEXT(pLastQItem) != NULL)
    {
        pLastQItem = MCI_QITEM_GET_NEXT(pLastQItem);
        pQueue->iItemCnt++;
    }
    if (pQueue->pHead == NULL) pQueue->pHead = pQItem;
    else                       MCI_QITEM_SET_NEXT(pQueue->pTail, pQItem);
    pQueue->pTail = pLastQItem;
    pQueue->iItemCnt++;
    if (hLock)
    {
        MciOsal_LeaveCodeLock(hLock);
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Leave] %s\r\n"), strHelper));
    }
    return;
}

Void MciQueuePushHead(PMCI_QUEUE pQueue, PMCI_QITEM pQItem, H_LOCK hLock, pTChar strHelper)
{
    PMCI_QITEM pLastQItem = NULL;
    if (pQueue == NULL) return;
    if (pQItem == NULL) return;
    if (hLock) 
    {
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Enter] %s\r\n"), strHelper));
        MciOsal_EnterCodeLock(hLock);
    }
    pLastQItem = pQItem;
    while (MCI_QITEM_GET_NEXT(pLastQItem) != NULL)
    {
        pLastQItem = MCI_QITEM_GET_NEXT(pLastQItem);
        pQueue->iItemCnt++;
    }
    if (pQueue->pHead == NULL)
    {
        pQueue->pHead = pQItem;
        pQueue->pTail = pLastQItem;
    }
    else
    {
        MCI_QITEM_SET_NEXT(pLastQItem, pQueue->pHead);
        pQueue->pHead = pQItem;
    }
    pQueue->iItemCnt++;
    if (hLock) 
    {
        MciOsal_LeaveCodeLock(hLock);
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Leave] %s\r\n"), strHelper));
    }
    return;
}
Void MciQueueRemove(PMCI_QUEUE pQueue, PMCI_QITEM pQItem, H_LOCK hLock, pTChar strHelper)
{
    PMCI_QITEM  pTmpQItem = NULL;

    if (pQueue == NULL) return;
    
    if (hLock) 
    {
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Enter] %s\r\n"), strHelper));
        MciOsal_EnterCodeLock(hLock);
    }

    while (pQItem)
    {
        if (pQueue->pHead == pQItem)
        {
            pQueue->pHead = MCI_QITEM_GET_NEXT(pQueue->pHead);
            pQueue->iItemCnt--;
            if (pQueue->pHead == NULL) pQueue->pTail = NULL;
        }
        else
        {
            pTmpQItem = pQueue->pHead;
            while ((MCI_QITEM_GET_NEXT(pTmpQItem) != NULL) && (MCI_QITEM_GET_NEXT(pTmpQItem) != pQItem))
            {
                pTmpQItem = MCI_QITEM_GET_NEXT(pTmpQItem);
            }
            if (MCI_QITEM_GET_NEXT(pTmpQItem) == pQItem)
            {
                MCI_QITEM_SET_NEXT(pTmpQItem, MCI_QITEM_GET_NEXT(MCI_QITEM_GET_NEXT(pTmpQItem)));
                pQueue->iItemCnt--;
                if (MCI_QITEM_GET_NEXT(pTmpQItem) == NULL) pQueue->pTail = pTmpQItem;
            }
        }
        pQItem = MCI_QITEM_GET_NEXT(pQItem);
    }
    if (hLock) 
    {
        MciOsal_LeaveCodeLock(hLock);
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Leave] %s\r\n"), strHelper));
    }
    return;
}
PMCI_QITEM MciQueuePopHead(PMCI_QUEUE pQueue, H_LOCK hLock, pTChar strHelper)
{
    PMCI_QITEM pQItem = NULL;

    if (pQueue == NULL) return NULL;
  
    if (hLock) 
    {
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Enter] %s\r\n"), strHelper));
        MciOsal_EnterCodeLock(hLock);
    }
    pQItem = pQueue->pHead;
    if (pQItem != NULL)
    {
        pQueue->pHead = MCI_QITEM_GET_NEXT(pQItem); 
        if (pQueue->pHead == NULL) pQueue->pTail = NULL;
        MCI_QITEM_SET_NEXT(pQItem, NULL);
        pQueue->iItemCnt--;
    }

    if (hLock) 
    {
        MciOsal_LeaveCodeLock(hLock);
        if (strHelper) RETAILMSG(_MCI_DBG_CHECK_CODE_LOCK, (TEXT("[Leave] %s\r\n"), strHelper));
    }
//    printk("Queue pop head 222\n");

    return pQItem;
}

