/*
 * linux/drivers/power/s3c6410_battery.h
 *
 * Battery measurement code for S3C6410 platform.
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define DRIVER_NAME	"sec-battery"

enum {
	BATT_VOL = 0,
	BATT_VOL_ADC,
	BATT_TEMP,
	BATT_TEMP_ADC,
	BATT_CHARGING_SOURCE,
	BATT_FG_SOC,
	BATT_RESET_SOC,
	CHARGING_MODE_BOOTING,
	BATT_TEMP_CHECK,
	BATT_FULL_CHECK,
	BATT_TYPE,
	AUTH_BATTERY,
#ifdef CONFIG_WIRELESS_CHARGING
	WC_STATUS,	/* Wireless charging */
	WC_ADC,
#endif
	BATT_CHG_CURRENT,	/* Charging current ADC */
	BATT_USE_CALL,	/* battery use */
	BATT_USE_VIDEO,
	BATT_USE_MUSIC,
	BATT_USE_BROWSER,
	BATT_USE_HOTSPOT,
	BATT_USE_CAMERA,
	BATT_USE_DATA_CALL,
	BATT_USE_LTE,
	BATT_USE_GPS,
	BATT_USE,	/* flags */
	CONTROL_TEMP,
};

#ifdef CONFIG_MACH_TREBON
#define TOTAL_CHARGING_TIME	(8*60*60)	/* 8 hours */
#else
#define TOTAL_CHARGING_TIME	(6*60*60)	/* 6 hours */
#endif

#define TOTAL_RECHARGING_TIME	(2*60*60)	/* 2 hours */

#if defined(CONFIG_TIKAL_MPCS)
#define RECHARGE_COND_VOLTAGE		4140000	/* 4.14V */
#elif defined CONFIG_MACH_TREBON
#define RECHARGE_COND_VOLTAGE       4300000 /* 4.30V */
#elif defined(CONFIG_AEGIS_USCC)
#define RECHARGE_COND_VOLTAGE		4135000	/* 4.135V */
#else
#define RECHARGE_COND_VOLTAGE		4130000	/* 4.13V */
#endif

