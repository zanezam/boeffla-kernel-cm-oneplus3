/*
 * Powerkey helper driver
 *
 * Author: andip71, 20.06.2016
 *
 * Version 1.0.0
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/boeffla_powerkey_helper.h>


/*****************************************/
/* Variables, structures and pointers */
/*****************************************/

#define	PWRKEY_DURATION_MS		60


/*****************************************/
/* Module/driver data */
/*****************************************/

#define DRIVER_AUTHOR "andip71 (Lord Boeffla)"
#define DRIVER_DESCRIPTION "Powerkey helper driver"
#define DRIVER_VERSION "1.1.0"
#define LOGTAG "Boeffla powerkey helper: "

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESCRIPTION);
MODULE_VERSION(DRIVER_VERSION);
MODULE_LICENSE("GPLv2");

static struct input_dev * pwrdev;
static DEFINE_MUTEX(boeffla_pwrkeyworklock);


/*****************************************/
// Internal functions
/*****************************************/

/* PowerKey internal work function */
static void boeffla_presspwr_dowork(struct work_struct * boeffla_presspwr_work)
{
	if (!mutex_trylock(&boeffla_pwrkeyworklock))
		return;

	input_event(pwrdev, EV_KEY, KEY_POWER, 1);
	input_event(pwrdev, EV_SYN, 0, 0);
	msleep(PWRKEY_DURATION_MS);

	input_event(pwrdev, EV_KEY, KEY_POWER, 0);
	input_event(pwrdev, EV_SYN, 0, 0);
	msleep(PWRKEY_DURATION_MS);
    
    mutex_unlock(&boeffla_pwrkeyworklock);
	return;
}
static DECLARE_WORK(boeffla_presspwr_work, boeffla_presspwr_dowork);


/*****************************************/
// External functions
/*****************************************/

/* PowerKey external function */
void boeffla_press_powerkey(void)
{
	schedule_work(&boeffla_presspwr_work);
    return;
}


/*****************************************/
// Driver init and exit functions
/*****************************************/

static int __init boeffla_powerkey_helper_init(void)
{
	int rc = 0;

	pwrdev = input_allocate_device();
	if (!pwrdev)
	{
		pr_err(LOGTAG"Can't allocate suspend autotest power button\n");
		return -EFAULT;
	}

	input_set_capability(pwrdev, EV_KEY, KEY_POWER);
	pwrdev->name = "bpkh_pwrkey";
	pwrdev->phys = "bpkh_pwrkey/input0";
	rc = input_register_device(pwrdev);
	if (rc) {
		pr_err(LOGTAG"%s: input_register_device err=%d\n", __func__, rc);
		goto err1;
	}

	return 0;

err1:
	input_free_device(pwrdev);
	return -EFAULT;
}


static void __exit boeffla_powerkey_helper_exit(void)
{
	input_unregister_device(pwrdev);
	input_free_device(pwrdev);

	return;
}


late_initcall(boeffla_powerkey_helper_init);
module_exit(boeffla_powerkey_helper_exit);
