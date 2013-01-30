/* linux/driver/misc/mcispi/MciQueue.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciQueue.h
**
**  Description
**      Declare the type and method for Queue operation
**
**  Authour:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.20  NEW         Created.
**      2008.12.12  Modified    Support for C-language Programming
**/
#ifndef _MCI_QUEUE_H_
#define _MCI_QUEUE_H_

#include "MciTypeDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MCI_QITEM 
{
    struct _MCI_QITEM*  pNext;
} MCI_QITEM, *PMCI_QITEM, **PPMCI_QITEM;

//
// Macro functions for MCI_QITEM
//
#define MCI_QITEM_INIT(x)           (memset((pVoid)(x), 0, sizeof(MCI_QITEM)))
#define MCI_QITEM_GET_NEXT(x)       (((PMCI_QITEM)(x))->pNext)
#define MCI_QITEM_SET_NEXT(x, y)    (((PMCI_QITEM)(x))->pNext = (PMCI_QITEM)(y))

typedef struct _MCI_QUEUE
{
    PMCI_QITEM      pHead;
    PMCI_QITEM      pTail;
    Int32           iItemCnt;
} MCI_QUEUE, *PMCI_QUEUE, **PPMCI_QUEUE;

//
// Macro function for MCI_QUEUE structure
//
#define MCI_QUEUE_INIT(x)           (memset((pVoid)(x), 0, sizeof(MCI_QUEUE)))
#define MCI_QUEUE_INC_COUNT(x)      (++(((PMCI_QUEUE)(x))->iItemCnt))
#define MCI_QUEUE_DEC_COUNT(x)      (--(((PMCI_QUEUE)(x))->iItemCnt))
#define MCI_QUEUE_GET_COUNT(x)      (((PMCI_QUEUE)(x))->iItemCnt)
#define MCI_QUEUE_GET_HEAD(x)       (((PMCI_QUEUE)(x))->pHead)
#define MCI_QUEUE_SET_HEAD(x,y)     (((PMCI_QUEUE)(x))->pHead = (PMCI_QITEM)(y))
#define MCI_QUEUE_GET_TAIL(x)       (((PMCI_QUEUE)(x))->pTail)
#define MCI_QUEUE_SET_TAIL(x,y)     (((PMCI_QUEUE)(x))->pTail = (PMCI_QITEM)(y))

//
// external function for MCI_QUEUE structure
//
extern Void         MciQueueInit(PMCI_QUEUE pQueue, H_LOCK hLock, pTChar strHelper);
extern Void         MciQueuePushTail(PMCI_QUEUE pQueue, PMCI_QITEM pQItem, H_LOCK hLock, pTChar strHelper);
extern Void         MciQueuePushHead(PMCI_QUEUE pQueue, PMCI_QITEM pQItem, H_LOCK hLock, pTChar strHelper);
extern Void         MciQueueRemove(PMCI_QUEUE pQueue, PMCI_QITEM pQItem, H_LOCK hLock, pTChar strHelper);
extern PMCI_QITEM   MciQueuePopHead(PMCI_QUEUE pQueue, H_LOCK hLock, pTChar strHelper);

#ifdef __cplusplus
}
#endif

#endif //_MCI_QUEUE_H_
