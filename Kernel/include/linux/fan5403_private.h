/*
 *  Copyright (C) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FAN5403_PRIVATE_H_
#define __FAN5403_PRIVATE_H_


/* CONTROL0 */
#define FAN5403_SHIFT_TMR_RST	7
#define FAN5403_SHIFT_EN_STAT	6
#define FAN5403_SHIFT_STAT	4
#define FAN5403_SHIFT_BOOST	3
#define FAN5403_SHIFT_FAULT	0

#define FAN5403_MASK_TMR_RST	(0x1 << FAN5403_SHIFT_TMR_RST)
#define FAN5403_MASK_EN_STAT	(0x1 << FAN5403_SHIFT_EN_STAT)
#define FAN5403_MASK_STAT	(0x3 << FAN5403_SHIFT_STAT)
#define FAN5403_MASK_BOOST	(0x1 << FAN5403_SHIFT_BOOST)
#define FAN5403_MASK_FAULT	(0x7 << FAN5403_SHIFT_FAULT)

#define FAN5403_TMR_RST_RESET	1

#define FAN5403_STAT_READY	0
#define FAN5403_STAT_CHARGING	1
#define FAN5403_STAT_CHG_DONE	2
#define FAN5403_STAT_FAULT	3

#if 0
#define FAN5403_FAULT_NONE	0
#define FAN5403_FAULT_VBUS_OVP	1
#define FAN5403_FAULT_SLEEP_MODE	2
#define FAN5403_FAULT_POOR_INPUT	3
#define FAN5403_FAULT_BATT_OVP	4
#define FAN5403_FAULT_THERMAL	5
#define FAN5403_FAULT_TIMER	6
#define FAN5403_FAULT_NO_BATT	7
#endif

/* CONTROL1 */
#define FAN5403_SHIFT_INLIM	6
#define FAN5403_SHIFT_LOWV	4
#define FAN5403_SHIFT_TE	3
#define FAN5403_SHIFT_CE	2
#define FAN5403_SHIFT_HZ_MODE	1
#define FAN5403_SHIFT_OPA_MODE	0

#define FAN5403_MASK_INLIM	(0x3 << FAN5403_SHIFT_INLIM)
#define FAN5403_MASK_LOWV	(0x3 << FAN5403_SHIFT_LOWV)
#define FAN5403_MASK_TE	(0x1 << FAN5403_SHIFT_TE)
#define FAN5403_MASK_CE	(0x1 << FAN5403_SHIFT_CE)
#define FAN5403_MASK_HZ_MODE	(0x1 << FAN5403_SHIFT_HZ_MODE)
#define FAN5403_MASK_OPA_MODE	(0x1 << FAN5403_SHIFT_OPA_MODE)

#define FAN5403_INLIM_100	0
#define FAN5403_INLIM_500	1
#define FAN5403_INLIM_800	2
#define FAN5403_INLIM_NO_LIMIT	3

#define FAN5403_LOWV_34	0
#define FAN5403_LOWV_35	1
#define FAN5403_LOWV_36	2
#define FAN5403_LOWV_37	3

#define FAN5403_TE_DISABLE	0
#define FAN5403_TE_ENABLE	1

#define FAN5403_CE_ENABLE	0
#define FAN5403_CE_DISABLE	1

#define FAN5403_HZ_MODE_NO	0
#define FAN5403_HZ_MODE_YES	1

#define FAN5403_OPA_MODE_CHARGE	0
#define FAN5403_OPA_MODE_BOOST	1

/* OREG */
#define FAN5403_SHIFT_OREG	2
#define FAN5403_SHIFT_OTG_PL	1
#define FAN5403_SHIFT_OTG_EN	0

#define FAN5403_MASK_OREG	(0x3F << FAN5403_SHIFT_OREG)
#define FAN5403_MASK_OTG_PL	(0x1 << FAN5403_SHIFT_OTG_PL)
#define FAN5403_MASK_OTG_EN	(0x1 << FAN5403_SHIFT_OTG_EN)

#define FAN5403_OREG_420	35
#define FAN5403_OREG_422	36
#define FAN5403_OREG_424	37
#define FAN5403_OREG_426	38
#define FAN5403_OREG_428	39
#define FAN5403_OREG_430	40

#define FAN5403_OTG_PL_LOW	0
#define FAN5403_OTG_PL_HIGH	1

#define FAN5403_OTG_EN_DISABLE	0
#define FAN5403_OTG_EN_ENABLE	1

/* IC_INFO */
#define FAN5403_SHIFT_VENDOR_CODE	5
#define FAN5403_SHIFT_PN	3
#define FAN5403_SHIFT_REV	0

#define FAN5403_MASK_VENDOR_CODE	(0x7 << FAN5403_SHIFT_VENDOR_CODE)
#define FAN5403_MASK_PN	(0x3 << FAN5403_SHIFT_PN)
#define FAN5403_MASK_REV	(0x7 << FAN5403_SHIFT_REV)

/* IBAT */
#define FAN5403_SHIFT_RESET	7
#define FAN5403_SHIFT_IOCHARGE	4
#define FAN5403_SHIFT_RESERVED_1	3
#define FAN5403_SHIFT_ITERM	0

#define FAN5403_MASK_RESET	(0x1 << FAN5403_SHIFT_RESET)
#define FAN5403_MASK_IOCHARGE	(0x7 << FAN5403_SHIFT_IOCHARGE)
#define FAN5403_MASK_RESERVED_1	(0x1 << FAN5403_SHIFT_RESERVED_1)
#define FAN5403_MASK_ITERM	(0x7 << FAN5403_SHIFT_ITERM)

#define FAN5403_IOCHARGE_550	0
#define FAN5403_IOCHARGE_650	1
#define FAN5403_IOCHARGE_750	2
#define FAN5403_IOCHARGE_850	3
#define FAN5403_IOCHARGE_950	4
#define FAN5403_IOCHARGE_1050	5
#define FAN5403_IOCHARGE_1150	6
#define FAN5403_IOCHARGE_1250	7

#define FAN5403_ITERM_49	0
#define FAN5403_ITERM_97	1
#define FAN5403_ITERM_146	2
#define FAN5403_ITERM_194	3
#define FAN5403_ITERM_243	4
#define FAN5403_ITERM_291	5
#define FAN5403_ITERM_340	6
#define FAN5403_ITERM_388	7

/* FAN54013 + 100mohm */
#define FAN54013_ITERM_33	0
#define FAN54013_ITERM_66	1
#define FAN54013_ITERM_99	2
#define FAN54013_ITERM_132	3
#define FAN54013_ITERM_165	4
#define FAN54013_ITERM_198	5
#define FAN54013_ITERM_231	6
#define FAN54013_ITERM_265	7

/* SP_CHARGER */
#define FAN5403_SHIFT_RESERVED_2	7
#define FAN5403_SHIFT_DIS_VREG	6
#define FAN5403_SHIFT_IO_LEVEL	5
#define FAN5403_SHIFT_SP	4
#define FAN5403_SHIFT_EN_LEVEL	3
#define FAN5403_SHIFT_VSP	0

#define FAN5403_MASK_RESERVED_2	(0x1 << FAN5403_SHIFT_RESERVED_2)
#define FAN5403_MASK_DIS_VREG	(0x1 << FAN5403_SHIFT_DIS_VREG)
#define FAN5403_MASK_IO_LEVEL	(0x1 << FAN5403_SHIFT_IO_LEVEL)
#define FAN5403_MASK_SP	(0x1 << FAN5403_SHIFT_SP)
#define FAN5403_MASK_EN_LEVEL	(0x1 << FAN5403_SHIFT_EN_LEVEL)
#define FAN5403_MASK_VSP	(0x7 << FAN5403_SHIFT_VSP)

#define FAN5403_RESERVED_2	1

#define FAN5403_DIS_VREG_ON	0
#define FAN5403_DIS_VREG_OFF	1

#define FAN5403_IO_LEVEL_0	0
#define FAN5403_IO_LEVEL_1	1

#define FAN5403_SP_RDONLY	1

#define FAN5403_EN_LEVEL_RDONLY	1

#define FAN5403_VSP_4213	0
#define FAN5403_VSP_4293	1
#define FAN5403_VSP_4373	2
#define FAN5403_VSP_4453	3
#define FAN5403_VSP_4533	4
#define FAN5403_VSP_4613	5
#define FAN5403_VSP_4693	6
#define FAN5403_VSP_4773	7

/* SAFETY */
#define FAN5403_SHIFT_RESERVED_3	7
#define FAN5403_SHIFT_ISAFE	4
#define FAN5403_SHIFT_VSAFE	0

#define FAN5403_MASK_RESERVED_3	(0x1 << FAN5403_SHIFT_RESERVED_3)
#define FAN5403_MASK_ISAFE	(0x7 << FAN5403_SHIFT_ISAFE)
#define FAN5403_MASK_VSAFE	(0xF << FAN5403_SHIFT_VSAFE)

#define FAN5403_ISAFE_550	0
#define FAN5403_ISAFE_650	1
#define FAN5403_ISAFE_750	2
#define FAN5403_ISAFE_850	3
#define FAN5403_ISAFE_950	4
#define FAN5403_ISAFE_1050	5
#define FAN5403_ISAFE_1150	6
#define FAN5403_ISAFE_1250	7

/* FAN54013 + 100mohm */
#define FAN54013_ISAFE_374	0
#define FAN54013_ISAFE_442	1
#define FAN54013_ISAFE_510	2
#define FAN54013_ISAFE_578	3
#define FAN54013_ISAFE_714	4
#define FAN54013_ISAFE_782	5
#define FAN54013_ISAFE_918	6
#define FAN54013_ISAFE_986	7

#define FAN5403_VSAFE_420	0
#define FAN5403_VSAFE_422	1
#define FAN5403_VSAFE_424	2
#define FAN5403_VSAFE_426	3
#define FAN5403_VSAFE_428	4
#define FAN5403_VSAFE_430	5
#define FAN5403_VSAFE_432	6
#define FAN5403_VSAFE_434	7
#define FAN5403_VSAFE_436	8
#define FAN5403_VSAFE_438	9
#define FAN5403_VSAFE_440	10
#define FAN5403_VSAFE_442	11
#define FAN5403_VSAFE_444	12

/* MONITOR */
#define FAN5403_SHIFT_ITERM_CMP	7
#define FAN5403_SHIFT_VBAT_CMP	6
#define FAN5403_SHIFT_LINCHG	5
#define FAN5403_SHIFT_T_120	4
#define FAN5403_SHIFT_ICHG	3
#define FAN5403_SHIFT_IBUS	2
#define FAN5403_SHIFT_VBUS_VALID	1
#define FAN5403_SHIFT_CV	0

#define FAN5403_MASK_ITERM_CMP	(0x1 << FAN5403_SHIFT_ITERM_CMP)
#define FAN5403_MASK_VBAT_CMP	(0x1 << FAN5403_SHIFT_VBAT_CMP)
#define FAN5403_MASK_LINCHG	(0x1 << FAN5403_SHIFT_LINCHG)
#define FAN5403_MASK_T_120	(0x1 << FAN5403_SHIFT_T_120)
#define FAN5403_MASK_ICHG	(0x1 << FAN5403_SHIFT_ICHG)
#define FAN5403_MASK_IBUS	(0x1 << FAN5403_SHIFT_IBUS)
#define FAN5403_MASK_VBUS_VALID	(0x1 << FAN5403_SHIFT_VBUS_VALID)
#define FAN5403_MASK_CV	(0x1 << FAN5403_SHIFT_CV)

#endif	/* !__FAN5403_PRIVATE_H_ */
