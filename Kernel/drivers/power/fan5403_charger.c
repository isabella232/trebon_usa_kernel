/* Switch mode charger to minimize single cell lithium ion charging time from a USB power source.
 *
 * Copyright (C) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/fan5403_charger.h>
#include <linux/fan5403_private.h>

/* Register address */
#define FAN5403_REG_CONTROL0	0x00
#define FAN5403_REG_CONTROL1	0x01
#define FAN5403_REG_OREG	0x02
#define FAN5403_REG_IC_INFO	0x03
#define FAN5403_REG_IBAT	0x04
#define FAN5403_REG_SP_CHARGER	0x05
#define FAN5403_REG_SAFETY	0x06
#define FAN5403_REG_MONITOR	0x10

static struct i2c_client	*fan5403_i2c_client;
static struct work_struct	chg_work;
static struct timer_list	chg_work_timer;
static struct workqueue_struct	*chg_wqueue;

static int fan5403_chg_current = CHG_CURR_NONE;

#define FAN5403_POLLING_INTERVAL	10000	/* 10 sec */

void fan5403_set_charge_current(int curr)
{
	fan5403_chg_current = curr;
}
EXPORT_SYMBOL(fan5403_set_charge_current);

static int fan5403_start_charging(int curr);
static int fan5403_stop_charging(void);

static void _fan5403_print_register(void)
{
	u8 regs[8];
	regs[0] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL0);
	regs[1] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL1);
	regs[2] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_OREG);
	regs[3] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IC_INFO);
	regs[4] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IBAT);
	regs[5] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_SP_CHARGER);
	regs[6] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_SAFETY);
	regs[7] = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_MONITOR);

	pr_info("/FAN5403/ 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n",
		regs[0], regs[1], regs[2], regs[3], regs[4], regs[5], regs[6], regs[7]);
}

static void _fan5403_customize_register_settings(void)
{
	u8 value;
	u8 ic_info;

	ic_info = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IC_INFO);

	if (ic_info == 0x94) {
		/* FAN54013 + 100mohm */
		i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_SAFETY,
			(FAN54013_ISAFE_986	<< FAN5403_SHIFT_ISAFE) |	/* Maximum IOCHARGE */
			(FAN5403_VSAFE_422	<< FAN5403_SHIFT_VSAFE));	/* Maximum OREG */
	} else {
		i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_SAFETY,
			(FAN5403_ISAFE_950	<< FAN5403_SHIFT_ISAFE) |	/* Maximum IOCHARGE */
			(FAN5403_VSAFE_422	<< FAN5403_SHIFT_VSAFE));	/* Maximum OREG */
	}

	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL1);
	value = value & ~(0x3F);	// clear except input limit
	value = value | ((FAN5403_LOWV_34	<< FAN5403_SHIFT_LOWV) |	/* Weak battery voltage 3.4V */
		(FAN5403_TE_DISABLE	<< FAN5403_SHIFT_TE) |
		(FAN5403_CE_ENABLE	<< FAN5403_SHIFT_CE) |
		(FAN5403_HZ_MODE_NO	<< FAN5403_SHIFT_HZ_MODE) |
		(FAN5403_OPA_MODE_CHARGE	<< FAN5403_SHIFT_OPA_MODE));
	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL1, value);

	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_OREG,
		(FAN5403_OREG_422	<< FAN5403_SHIFT_OREG) |	/* Full charging voltage */
		(FAN5403_OTG_PL_HIGH	<< FAN5403_SHIFT_OTG_PL) |
		(FAN5403_OTG_EN_DISABLE	<< FAN5403_SHIFT_OTG_EN));

	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_SP_CHARGER,
		(FAN5403_RESERVED_2	<< FAN5403_SHIFT_RESERVED_2) |
		(FAN5403_DIS_VREG_ON	<< FAN5403_SHIFT_DIS_VREG) |
		(FAN5403_IO_LEVEL_0	<< FAN5403_SHIFT_IO_LEVEL) |	/* Controlled by IOCHARGE bits */
		(FAN5403_SP_RDONLY	<< FAN5403_SHIFT_SP) |
		(FAN5403_EN_LEVEL_RDONLY	<< FAN5403_SHIFT_EN_LEVEL) |
		(FAN5403_VSP_4533	<< FAN5403_SHIFT_VSP));

	if (fan5403_chg_current == CHG_CURR_NONE)
		fan5403_stop_charging();
	else
		fan5403_start_charging(fan5403_chg_current);
}

static int _fan5403_set_iocharge(int curr)
{
	u8 value;
	u8 ic_info;
	char *fan5403_curr[] = { "550", "650", "750", "850", "950", "1050", "1150", "1250" };
	char *fan54013_curr[] = { "374", "442", "510", "578", "714", "782", "918", "986" };	/* 100mohm */

	ic_info = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IC_INFO);

	/* Set charge current */
	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IBAT);

	value = value & ~(FAN5403_MASK_RESET);	/* Clear control reset bit */
	value = value & ~(FAN5403_MASK_IOCHARGE);
	value = value | (curr << FAN5403_SHIFT_IOCHARGE);

	if (ic_info == 0x94)
		pr_info("%s: IOCHARGE = %smA\n", __func__, fan54013_curr[curr]);
	else
		pr_info("%s: IOCHARGE = %smA\n", __func__, fan5403_curr[curr]);

	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_IBAT, value);

	/* Set input current limit */
	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL1);

	value = value & ~(FAN5403_MASK_INLIM);

	if (ic_info == 0x94) {
		if (curr <= FAN54013_CURR_510) {
			value = value | (FAN5403_INLIM_500 << FAN5403_SHIFT_INLIM);
			pr_info("%s: INLIM = 500mA\n", __func__);
		} else {
			value = value | (FAN5403_INLIM_NO_LIMIT << FAN5403_SHIFT_INLIM);
			pr_info("%s: INLIM = No limit\n", __func__);
		}
	} else {
		if (curr == FAN5403_CURR_550) {
			value = value | (FAN5403_INLIM_500 << FAN5403_SHIFT_INLIM);
			pr_info("%s: INLIM = 500mA\n", __func__);
		} else {
			value = value | (FAN5403_INLIM_NO_LIMIT << FAN5403_SHIFT_INLIM);
			pr_info("%s: INLIM = No limit\n", __func__);
		}
	}

	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL1, value);

	return curr;
}

static int _fan5403_set_iterm(void)
{
	u8 value;
	u8 ic_info;

	ic_info = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IC_INFO);

	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IBAT);

	value = value & ~(FAN5403_MASK_RESET);	/* Clear control reset bit */
	value = value & ~(FAN5403_MASK_ITERM);

	if (ic_info == 0x94) {
		/* FAN54013 + 100mohm */
		value = value | (FAN54013_ITERM_198 << FAN5403_SHIFT_ITERM);
		pr_info("%s: ITERM 198mA\n", __func__);
	} else {
		value = value | (FAN5403_ITERM_194 << FAN5403_SHIFT_ITERM);
		pr_info("%s: ITERM 194mA\n", __func__);
	}

	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_IBAT, value);

	return 0;
}

static int _fan5403_charging_control(int enable)
{
	u8 value;

	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL1);

	value = value & ~(FAN5403_MASK_CE);
	value = value & ~(FAN5403_MASK_TE);

	/* Set CE and TE bit */
	if (enable)
		value = value |
			(FAN5403_TE_ENABLE << FAN5403_SHIFT_TE) |
			(FAN5403_CE_ENABLE << FAN5403_SHIFT_CE);
	else
		value = value |
			(FAN5403_TE_DISABLE << FAN5403_SHIFT_TE) |
			(FAN5403_CE_DISABLE << FAN5403_SHIFT_CE);

	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL1, value);

	return 0;
}

static void _fan5403_chg_work(struct work_struct *work)
{
	u8 value;
	static int log_cnt = 0;

	/* Reset 32 sec timer while host is alive */
	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL0);

	value = value & ~(FAN5403_MASK_TMR_RST);
	value = value | (FAN5403_TMR_RST_RESET << FAN5403_SHIFT_TMR_RST);

	i2c_smbus_write_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL0, value);

	/* print registers every 10 minutes */
	log_cnt++;
	if (log_cnt == 60) {
		_fan5403_print_register();
		log_cnt = 0;
	}

	pr_debug("fan5403 host is alive\n");

	mod_timer(&chg_work_timer, jiffies + msecs_to_jiffies(FAN5403_POLLING_INTERVAL));
}

static void _fan5403_chg_work_timer_func(unsigned long param)
{
	queue_work(chg_wqueue, &chg_work);
}

static int fan5403_start_charging(int curr)
{
	u8 ic_info;
	int iocharge;

	if (!fan5403_i2c_client)
		return 0;

	fan5403_chg_current = curr;

	ic_info = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IC_INFO);

	if (ic_info == 0x94) {
		/* FAN54013 with 100mohm */
		switch (curr) {
		case CHG_CURR_TA:
			iocharge = FAN54013_CURR_986;
			break;
		case CHG_CURR_TA_EVENT:
			iocharge = FAN54013_CURR_782;
			break;
		case CHG_CURR_USB:
			iocharge = FAN54013_CURR_510;
			break;
		default:
			iocharge = -1;
			break;
		}
	} else {
		/* FAN5403 with 68mohm */
		switch (curr) {
		case CHG_CURR_TA:
			iocharge = FAN5403_CURR_950;
			break;
		case CHG_CURR_TA_EVENT:
			iocharge = FAN5403_CURR_750;
			break;
		case CHG_CURR_USB:
			iocharge = FAN5403_CURR_550;
			break;
		default:
			iocharge = -1;
			break;
		}
	}

	if (iocharge != (-1)) {
		_fan5403_set_iocharge(iocharge);
		_fan5403_set_iterm();
		_fan5403_charging_control(1);
	} else {
		pr_err("%s: Error (curr = %d)\n", __func__, curr);
		_fan5403_charging_control(0);
	}

	_fan5403_print_register();
	return 0;
}

static int fan5403_stop_charging(void)
{
	if (!fan5403_i2c_client)
		return 0;

	fan5403_chg_current = CHG_CURR_NONE;
	_fan5403_charging_control(0);

	pr_info("%s\n", __func__);

	_fan5403_print_register();
	return 0;
}

static int fan5403_get_vbus_status(void)
{
	u8 value;

	if (!fan5403_i2c_client)
		return 0;

	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_MONITOR);

	if (value & FAN5403_MASK_VBUS_VALID)
		return 1;
	else
		return 0;
}

static int fan5403_set_host_status(bool isAlive)
{
	if (!fan5403_i2c_client)
		return 0;

	if (isAlive) {
		/* Reset 32 sec timer every 10 sec to ensure the host is alive */
		queue_work(chg_wqueue, &chg_work);
	} else
		del_timer_sync(&chg_work_timer);

	return isAlive;
}

static int _fan5403_get_charging_fault(void)
{
	u8 value;

	if (!fan5403_i2c_client)
		return 0;

	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL0);

	switch (value & FAN5403_MASK_FAULT) {
	case FAN5403_FAULT_NONE:
		pr_info("%s: none\n", __func__);
		break;
	case FAN5403_FAULT_VBUS_OVP:
		pr_info("%s: vbus ovp\n", __func__);
		break;
	case FAN5403_FAULT_SLEEP_MODE:
		pr_info("%s: sleep mode\n", __func__);
		break;
	case FAN5403_FAULT_POOR_INPUT:
		pr_info("%s: poor input source\n", __func__);
		break;
	case FAN5403_FAULT_BATT_OVP:
		pr_info("%s: battery ovp\n", __func__);
		break;
	case FAN5403_FAULT_THERMAL:
		pr_info("%s: thermal shutdown\n", __func__);
		break;
	case FAN5403_FAULT_TIMER:
		pr_info("%s: timer fault\n", __func__);
		break;
	case FAN5403_FAULT_NO_BATT:
		pr_info("%s: no battery\n", __func__);
		break;
	}

	return (value & FAN5403_MASK_FAULT);
}

static int _fan5403_get_charging_status(void)
{
	u8 value;

	value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_CONTROL0);
	value = value & FAN5403_MASK_STAT;

	if (value == (FAN5403_STAT_READY << FAN5403_SHIFT_STAT))
		return FAN5403_STAT_READY;
	else if (value == (FAN5403_STAT_CHARGING << FAN5403_SHIFT_STAT))
		return FAN5403_STAT_CHARGING;
	else if (value == (FAN5403_STAT_CHG_DONE << FAN5403_SHIFT_STAT))
		return FAN5403_STAT_CHG_DONE;
	else
		return FAN5403_STAT_FAULT;
}

static irqreturn_t _fan5403_handle_stat(int irq, void *data)
{
	struct fan5403_platform_data *pdata = data;
	int stat;
	enum fan5403_fault_t fault_reason;

	msleep(500);

	stat = _fan5403_get_charging_status();

	switch (stat) {
	case FAN5403_STAT_READY:
		pr_info("%s: ready\n", __func__);
		break;
	case FAN5403_STAT_CHARGING:
		pr_info("%s: charging\n", __func__);
		_fan5403_customize_register_settings();
		break;
	case FAN5403_STAT_CHG_DONE:
		pdata->tx_charge_done();
		pr_info("%s: chg done\n", __func__);
		break;
	case FAN5403_STAT_FAULT:
		_fan5403_print_register();
		fault_reason = _fan5403_get_charging_fault();
		pdata->tx_charge_fault(fault_reason);
		_fan5403_customize_register_settings();
		break;
	}

	return IRQ_HANDLED;
}

/* Support battery test mode (*#0228#) */
static int fan5403_get_monitor_bits(void)
{
	/*
		VICHG not supported with FAN5403...

		0:	Discharging (no charger)
		1:	CC charging
		2:	CV charging
		3:	Not charging (charge termination) include fault
	*/

	u8 value = 0;

	if (fan5403_get_vbus_status()) {
		if (_fan5403_get_charging_status() == FAN5403_STAT_CHARGING) {
			/* Check CV bit */
			value = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_MONITOR);
			if (value & FAN5403_MASK_CV)
				return 2;
			else
				return 1;
		}
		return 3;
	}
	return 0;
}

static int fan5403_get_fault(void)
{
	enum fan5403_fault_t fault_reason = FAN5403_FAULT_NONE;

	if (_fan5403_get_charging_status() == FAN5403_STAT_FAULT)
		fault_reason = _fan5403_get_charging_fault();

	return fault_reason;
}

static int fan5403_get_ic_info(void)
{
	u8 ic_info;

	if (!fan5403_i2c_client) {
		pr_err("%s: i2c not initialized!\n", __func__);
		return -1;
	}

	ic_info = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IC_INFO);

	if (ic_info == 0x94)
		return CHARGER_IC_FAN54013;
	else
		return CHARGER_IC_FAN5403;
}

static int __devinit fan5403_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct fan5403_platform_data *pdata = client->dev.platform_data;
	int ret;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -EIO;

	pr_info("+++++++++++++++ %s +++++++++++++++\n", __func__);

	fan5403_i2c_client = client;

	/* Init Rx functions from host */
	pdata->start_charging = fan5403_start_charging;
	pdata->stop_charging = fan5403_stop_charging;
	pdata->get_vbus_status = fan5403_get_vbus_status;
	pdata->set_host_status = fan5403_set_host_status;
	pdata->get_monitor_bits = fan5403_get_monitor_bits;
	pdata->get_fault = fan5403_get_fault;
	pdata->get_ic_info = fan5403_get_ic_info;

	INIT_WORK(&chg_work, _fan5403_chg_work);
	setup_timer(&chg_work_timer, _fan5403_chg_work_timer_func, (unsigned long)pdata);
	chg_wqueue = create_freezeable_workqueue("fan5403_wqueue");

	ret = request_threaded_irq(client->irq, NULL, _fan5403_handle_stat,
				   (IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING),
				   "fan5403-stat", pdata);

	_fan5403_customize_register_settings();
	_fan5403_print_register();

	return 0;
}

static int __devexit fan5403_remove(struct i2c_client *client)
{
        return 0;
}

static int fan5403_resume(struct i2c_client *client)
{
	struct fan5403_platform_data *pdata = client->dev.platform_data;
	u8 ic_info;

	ic_info = i2c_smbus_read_byte_data(fan5403_i2c_client, FAN5403_REG_IC_INFO);

	if (ic_info == 0x94) {
		if (_fan5403_get_charging_status() == FAN5403_STAT_CHG_DONE) {
			pr_info("%s: chg done\n", __func__);
			pdata->tx_charge_done();
		}
	}

	return 0;
}

static const struct i2c_device_id fan5403_id[] = {
	{"fan5403", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, fan5403_id);

static struct i2c_driver fan5403_i2c_driver = {
	.driver = {
		.name = "fan5403",
	},
	.probe = fan5403_probe,
	.remove = __devexit_p(fan5403_remove),
	.resume = fan5403_resume,
	.id_table = fan5403_id,
};

static int __init fan5403_init(void)
{
	return i2c_add_driver(&fan5403_i2c_driver);
}
module_init(fan5403_init);

static void __exit fan5403_exit(void)
{
	i2c_del_driver(&fan5403_i2c_driver);
}
module_exit(fan5403_exit);

