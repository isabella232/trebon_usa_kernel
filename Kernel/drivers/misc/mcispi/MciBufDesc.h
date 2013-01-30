/* drivers/misc/mcispi/MciBufDesc.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciBufDesc.h
**
**  Description
**      Declare the type and method for Buffer Descriptor
**
**  Authour:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.20  NEW         Created.
**      2008.12.12  Modified    Support for C-Language programming
**/

#ifndef _MCI_BUF_DESC_H_
#define _MCI_BUF_DESC_H_

#include "MciTypeDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _MCI_BUF_DESC
{
    struct _MCI_BUF_DESC*   pNextDesc;
    pVoid                   pBuffer;
    pVoid                   pPhysBuffer;
    UInt32                  uiType;
    UInt32                  uiIndex;
    UInt32                  uiParam1;
    UInt32                  uiParam2;
    UInt32                  uiParam3;
} MCI_BUF_DESC, *PMCI_BUF_DESC, **PPMCI_BUF_DESC;

//
// Macro functions
//
#define MCI_BUF_DESC_GET_NEXT(x)        ((PMCI_BUF_DESC)(((PMCI_BUF_DESC)(x))->pNextDesc))
#define MCI_BUF_DESC_SET_NEXT(x, y)     ((PMCI_BUF_DESC)(x))->pNextDesc = ((PMCI_BUF_DESC)(y))
#define MCI_BUF_DESC_INIT(x)            (memset((pVoid)(x), 0, sizeof(MCI_BUF_DESC)))
#define MCI_BUF_DESC_GET_VA_ADDR(x)     (((PMCI_BUF_DESC)(x))->pBuffer)
#define MCI_BUF_DESC_SET_VA_ADDR(x,y)   ((PMCI_BUF_DESC)(x))->pBuffer = (pVoid)(y)
#define MCI_BUF_DESC_GET_PA_ADDR(x)     (((PMCI_BUF_DESC)(x))->pPhysBuffer)
#define MCI_BUF_DESC_SET_PA_ADDR(x,y)   (((PMCI_BUF_DESC)(x))->pPhysBuffer = (pVoid)(y))
#define MCI_BUF_DESC_GET_MSG_BUF(x)     ((pVoid)((UInt32)(((PMCI_BUF_DESC)(x))->pBuffer) + MCI_MSG_START_POS_IN_DESC_BUF))
#define MCI_BUF_DESC_GET_PA_MSG_BUF(x)  ((pVoid)((UInt32)(((PMCI_BUF_DESC)(x))->pPhysBuffer) + MCI_MSG_START_POS_IN_DESC_BUF))
#define MCI_BUF_DESC_GET_ETH_HDR(x)     ((pVoid)(((PMCI_BUF_DESC)(x))->pBuffer))
#define MCI_BUF_DESC_GET_IP_HDR(x)      ((pVoid)((UInt32)(((PMCI_BUF_DESC)(x))->pBuffer) + MCI_ETH_HDR_LEN))
#define MCI_BUF_DESC_GET_UDP_HDR(x)     ((pVoid)((UInt32)(((PMCI_BUF_DESC)(x))->pBuffer) + MCI_ETH_HDR_LEN + MCI_IPV4_HDR_LEN))
#define MCI_BUF_DESC_GET_TYPE(x)        (((PMCI_BUF_DESC)(x))->uiType)
#define MCI_BUF_DESC_SET_TYPE(x, y)     (((PMCI_BUF_DESC)(x))->uiType = (UInt32)(y))
#define MCI_BUF_DESC_GET_INDEX(x)       (((PMCI_BUF_DESC)(x))->uiIndex)
#define MCI_BUF_DESC_SET_INDEX(x, y)    (((PMCI_BUF_DESC)(x))->uiIndex = (UInt32)(y))
#define MCI_BUF_DESC_GET_PARAM1(x)      (((PMCI_BUF_DESC)(x))->uiParam1)
#define MCI_BUF_DESC_SET_PARAM1(x, y)   (((PMCI_BUF_DESC)(x))->uiParam1 = (UInt32)(y))
#define MCI_BUF_DESC_GET_PARAM2(x)      (((PMCI_BUF_DESC)(x))->uiParam2)
#define MCI_BUF_DESC_SET_PARAM2(x, y)   (((PMCI_BUF_DESC)(x))->uiParam2 = (UInt32)(y))
#define MCI_BUF_DESC_GET_PARAM3(x)      (((PMCI_BUF_DESC)(x))->uiParam3)
#define MCI_BUF_DESC_SET_PARAM3(x, y)   (((PMCI_BUF_DESC)(x))->uiParam3 = (UInt32)(y))

#ifdef __cplusplus
}
#endif

#endif	// _MCI_BUF_DESC_H_
