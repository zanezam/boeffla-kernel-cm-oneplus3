/*
 * Author: andip71
 * 
 * Version 1.2.0
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

/*
 * Change log:
 * 
 * 1.2.1 (28.09.2016)
 *   - Fix the completely broken mic gain sysfs not storing any settings
 *
 * 1.2.0 (26.09.2016)
 *   - Add general mic gain control + avoid speaker volume resets
 *
 * 1.1.1 (16.09.2016)
 *   - Fix speaker control (variable overflow)
 *
 * 1.1.0 (23.08.2016)
 *   - Add speaker control
 *
 * 1.0.0 (15.08.2016)
 *   - Initial version for OnePlus 3
 * 
 */

#include "boeffla_sound.h"


/*****************************************/
// Variables
/*****************************************/

// internal boeffla sound variables
static int boeffla_sound;	// boeffla sound master switch
static int debug;			// debug switch

static int headphone_volume_l;
static int headphone_volume_r;
static int speaker_volume;
static int mic_level_general;

/*****************************************/
// internal helper functions
/*****************************************/

static void reset_boeffla_sound(void)
{
	// set all boeffla sound config settings to defaults
	headphone_volume_l = HEADPHONE_DEFAULT;
	headphone_volume_r = HEADPHONE_DEFAULT;
	speaker_volume = SPEAKER_DEFAULT;
	mic_level_general = MICLEVEL_DEFAULT_GENERAL;

	if (debug)
		printk("Boeffla-sound: boeffla sound reset done\n");
}


static void reset_audio_hub(void)
{
	int tmp;
	
	// reset all audio hub registers back to defaults
	set_headphone_gain_l(headphone_volume_l);
	set_headphone_gain_r(headphone_volume_r);

	tmp = (speaker_volume) << 8;
	tmp += get_speaker_gain() & 0x00FF;
	set_speaker_gain(tmp);

	set_mic_gain_general(mic_level_general);

	if (debug)
		printk("Boeffla-sound: wcd9335 audio hub reset done\n");
}


/*****************************************/
// sysfs interface functions
/*****************************************/

// Boeffla sound master switch

static ssize_t boeffla_sound_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// print current value
	return sprintf(buf, "Boeffla sound status: %d\n", boeffla_sound);
}


static ssize_t boeffla_sound_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	int val;

	// read values from input buffer
	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;
		
	// store if valid data
	if (((val == 0) || (val == 1)))
	{
		// set new status
		boeffla_sound = val;

		// re-initialize settings and audio hub (in any case for both on and off !)
		reset_boeffla_sound();
		reset_audio_hub();

		// print debug info
		if (debug)
			printk("Boeffla-sound: status %d\n", boeffla_sound);
	}

	return count;
}


// Headphone volume

static ssize_t headphone_volume_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val_l;
	int val_r;

	val_l = get_headphone_gain_l();
	val_r = get_headphone_gain_r();

	// convert back into signed integer between -127 and +127)
	if (val_l > 127)
		val_l = val_l - 256;

	if (val_r > 127)
		val_r = val_r - 256;

	// print current values
	return sprintf(buf, "Headphone volume:\nLeft: %d\nRight: %d\n", val_l, val_r);
}


static ssize_t headphone_volume_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	int val_l;
	int val_r;

	// Terminate if boeffla sound is not enabled
	if (!boeffla_sound)
		return count;

	// read values from input buffer
	ret = sscanf(buf, "%d %d", &val_l, &val_r);

	if (ret != 2)
		return -EINVAL;
		
	// check whether values are within the valid ranges and adjust accordingly
	if (val_l > HEADPHONE_MAX)
		val_l = HEADPHONE_MAX;

	if (val_l < HEADPHONE_MIN)
		val_l = HEADPHONE_MIN;

	if (val_r > HEADPHONE_MAX)
		val_r = HEADPHONE_MAX;

	if (val_r < HEADPHONE_MIN)
		val_r = HEADPHONE_MIN;

	// store new values
	headphone_volume_l = val_l;
	headphone_volume_r = val_r;

	// set new values
	set_headphone_gain_l(headphone_volume_l);
	set_headphone_gain_r(headphone_volume_r);

	// print debug info
	if (debug)
		printk("Boeffla-sound: headphone volume L=%d R=%d\n", headphone_volume_l, headphone_volume_r);

	return count;
}

static ssize_t speaker_volume_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val;

	val = get_speaker_gain();	// mono speaker, so we alwas treat L and R the same
	val = (val >> 8) * -1;

	// print current values
	return sprintf(buf, "Speaker volume:\nLeft: %d\nRight: %d\n", val, val);
}


static ssize_t speaker_volume_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	int val;
	int val_unused;
	int tmp;

	// Terminate if boeffla sound is not enabled
	if (!boeffla_sound)
		return count;

	// read values from input buffer
	ret = sscanf(buf, "%d %d", &val, &val_unused);	// mono speaker, so we only consider first value

	if (ret != 2)
		return -EINVAL;
		
	// check whether values are within the valid ranges and adjust accordingly
	if (val > SPEAKER_MAX)
		val = SPEAKER_MAX;

	if (val < SPEAKER_MIN)
		val = SPEAKER_MIN;

	// store new value
	speaker_volume = val;

	// set new values
	tmp = (val * -1);
	tmp = (tmp << 8) + (get_speaker_gain() & 0x00FF);
	set_speaker_gain(tmp);

	// print debug info
	if (debug)
		printk("Boeffla-sound: speaker volume L=%d R=%d\n", speaker_volume, speaker_volume);

	return count;
}


// Microphone level general

static ssize_t mic_level_general_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int val;

	val = get_mic_gain_general();

	// convert byte to signed int
	if (val > 127)
		val = (256 - val) * -1;

	return sprintf(buf, "Mic level general %d\n", val);
}


static ssize_t mic_level_general_store(struct device *dev, struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	int val;

	// Terminate if boeffla sound is not enabled
	if (!boeffla_sound)
		return count;

	// read value for mic level from input buffer
	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;

	// check whether values are within the valid ranges and adjust accordingly
	if (val > MICLEVEL_MAX_GENERAL)
		val = MICLEVEL_MAX_GENERAL;

	if (val < MICLEVEL_MIN_GENERAL)
		val = MICLEVEL_MIN_GENERAL;

	// store new value
	mic_level_general = val;

	// set new value
	set_mic_gain_general(mic_level_general);

	// print debug info
	if (debug)
		printk("Boeffla-sound: Mic level general %d\n", mic_level_general);

	return count;
}


// Debug status

static ssize_t debug_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// return current debug status
	return sprintf(buf, "Debug status: %d\n", debug);
}

static ssize_t debug_store(struct device *dev, struct device_attribute *attr,
						const char *buf, size_t count)
{
	unsigned int ret = -EINVAL;
	unsigned int val;

	// check data and store if valid
	ret = sscanf(buf, "%d", &val);

	if (ret != 1)
		return -EINVAL;
		
	if ((val == 0) || (val == 1))
		debug = val;

	return count;
}


// Version information

static ssize_t version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	// return version information
	return sprintf(buf, "%s\n", BOEFFLA_SOUND_VERSION);
}


/*****************************************/
// Initialize boeffla sound sysfs folder
/*****************************************/

// define objects
static DEVICE_ATTR(boeffla_sound, 0664, boeffla_sound_show, boeffla_sound_store);
static DEVICE_ATTR(headphone_volume, 0664, headphone_volume_show, headphone_volume_store);
static DEVICE_ATTR(speaker_volume, 0664, speaker_volume_show, speaker_volume_store);
static DEVICE_ATTR(mic_level_general, 0664, mic_level_general_show, mic_level_general_store);
static DEVICE_ATTR(debug, 0664, debug_show, debug_store);
static DEVICE_ATTR(version, 0664, version_show, NULL);

// define attributes
static struct attribute *boeffla_sound_attributes[] = {
	&dev_attr_boeffla_sound.attr,
	&dev_attr_headphone_volume.attr,
	&dev_attr_speaker_volume.attr,
	&dev_attr_mic_level_general.attr,
	&dev_attr_debug.attr,
	&dev_attr_version.attr,
	NULL
};

// define attribute group
static struct attribute_group boeffla_sound_control_group = {
	.attrs = boeffla_sound_attributes,
};

// define control device
static struct miscdevice boeffla_sound_control_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "boeffla_sound",
};


/*****************************************/
// Driver init and exit functions
/*****************************************/

static int boeffla_sound_init(void)
{
	// register boeffla sound control device
	misc_register(&boeffla_sound_control_device);
	if (sysfs_create_group(&boeffla_sound_control_device.this_device->kobj,
				&boeffla_sound_control_group) < 0) {
		printk("Boeffla-sound: failed to create sys fs object.\n");
		return 0;
	}

	// Initialize variables
	boeffla_sound = BOEFFLA_SOUND_DEFAULT;
	debug = DEBUG_DEFAULT;
	
	// Reset boeffla sound settings
	reset_boeffla_sound();

	// Print debug info
	printk("Boeffla-sound: engine version %s started\n", BOEFFLA_SOUND_VERSION);

	return 0;
}


static void boeffla_sound_exit(void)
{
	// remove boeffla sound control device
	sysfs_remove_group(&boeffla_sound_control_device.this_device->kobj,
                           &boeffla_sound_control_group);

	// Print debug info
	printk("Boeffla-sound: engine stopped\n");
}


/* define driver entry points */

module_init(boeffla_sound_init);
module_exit(boeffla_sound_exit);
