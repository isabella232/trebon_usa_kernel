/*
 *  Copyright (C) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FAN5403_CHARGER_H_
#define __FAN5403_CHARGER_H_

/* Charge current */
/* Argument value of "set_charge_current" */
enum fan5403_iocharge_t {
	FAN5403_CURR_550 = 0,
	FAN5403_CURR_650,
	FAN5403_CURR_750,
	FAN5403_CURR_850,
	FAN5403_CURR_950,
	FAN5403_CURR_1050,
	FAN5403_CURR_1150,
	FAN5403_CURR_1250,
};	/* FAN5403 + 68mohm */

enum fan54013_100m_iocharge_t {
	FAN54013_CURR_374 = 0,
	FAN54013_CURR_442,
	FAN54013_CURR_510,
	FAN54013_CURR_578,
	FAN54013_CURR_714,
	FAN54013_CURR_782,
	FAN54013_CURR_918,
	FAN54013_CURR_986,
};	/* FAN54013 + 100mohm */

/* See "fan5403_start_charging()" in fan5403_charger.c */
#define CHG_CURR_NONE		(-1)
#define CHG_CURR_TA		0
#define CHG_CURR_TA_EVENT	1
#define CHG_CURR_USB		2

/* IC Information */
/* Return value of "get_ic_info" */
#define CHARGER_IC_FAN5403	0
#define CHARGER_IC_FAN54013	1

/* Charging fault */
/* Argument value of "tx_charge_fault" */
enum fan5403_fault_t {
	FAN5403_FAULT_NONE = 0,
	FAN5403_FAULT_VBUS_OVP,
	FAN5403_FAULT_SLEEP_MODE,
	FAN5403_FAULT_POOR_INPUT,
	FAN5403_FAULT_BATT_OVP,
	FAN5403_FAULT_THERMAL,
	FAN5403_FAULT_TIMER,
	FAN5403_FAULT_NO_BATT,
};

struct fan5403_platform_data {
	/* Rx functions from host */
	int (*start_charging)(int curr);
	int (*stop_charging)(void);
	int (*get_vbus_status)(void);
	int (*set_host_status)(bool isAlive);
	int (*get_monitor_bits)(void);	/* support battery test mode (*#0228#) */
	int (*get_fault)(void);
	int (*get_ic_info)(void);

	/* Tx functions to host */
	void (*tx_charge_done)(void);
	void (*tx_charge_fault)(enum fan5403_fault_t reason);
};

/*

 Host(AP) --> Client(Charger)	: "Rx" from host
 Host(AP) <-- Client(Charger)	: "Tx" to host

*/

#endif
