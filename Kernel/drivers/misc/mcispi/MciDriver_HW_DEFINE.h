/* drivers/misc/mcispi/MciDriver_HW_DEFINE.h
 *
 * Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *      http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*!
**  MciDriver_HW_DEFINE.h
**
**  Description
**      Include the actual PDD header file
**
**  Authur:
**      Seunghan Kim (hyenakim@samsung.com)
**
**  Update:
**      2008.10.20  NEW     Created.
**/

#ifndef _MCI_DRIVER_HW_DEFINE_H_
#define _MCI_DRIVER_HW_DEFINE_H_

#ifdef _ANDROID_
#include "MciDriver_SPI_S5PV210_ANDROID.h"
#endif

#ifdef _LINUX_
#include "MciDriver_SPI_S5PV210_ANDROID.h"
#endif

#endif //_MCI_DRIVER_HW_DEFINE_H_

