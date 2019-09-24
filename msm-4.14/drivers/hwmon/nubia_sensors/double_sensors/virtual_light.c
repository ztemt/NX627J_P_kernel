/*
 * This file is part of the virtual_light sensor driver.
 * virtual_light is combined light, and VCSEL.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 *Reversion
 *
 *
 *

when         	who         		Remark : what, where, why          							version
-----------   	------------     	-----------------------------------   					    ------------------
2018/7/11       GaoKuan                 For set_light front or back mode                        v1.0
2018/8/1        Xuxiaohua               Add calibration node                                        v1.1
2018/8/8        GaoKuan                 And volatile type for fb_status and enable_status           v1.2
2018/8/13       GaoKuan                 Change prox front or back status,When LCD switch            V1.3


==========================================================================================
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include "virtual_light.h"

#ifdef CONFIG_NUBIA_SWITCH_LCD
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/msm_drm_notify.h>
#endif

#include "sensor_common.h"

#define VIR_LIGHT_DRV_NAME "light"
#define INPUT_NAME_VIR_LIGHT       "light"

#define TMD2725_ALS_SYS_PATH             "/sys/class/light_back/light_back/"
#define TMD2725_ALS_SYS_ENABLE_PATH      "/sys/class/light_back/light_back/enable"
#define TMD2725_ALS_SYS_FAC_CAL_PATH     "/sys/class/light_back/light_back/fac_calibrate"
#define TMD2725_ALS_SYS_INIT_DEV_PATH    "/sys/class/light_back/light_back/dev_init"
#define TMD2725_ALS_SYS_LIGHT_VALUE_PATH    "/sys/class/light_back/light_back/light_value"
#define TMD2725_ALS_SYS_LIGHT_CALIBRATE_PATH "/sys/class/light_back/light_back/calibrate"
#define TMD2725_ALS_SYS_CHIIP_NAME_PATH "/sys/class/light_back/light_back/chip_name"
#define TMD2725_ALS_SYS_FLUSH_PATH "/sys/class/light_back/light_back/flush"
#define TMD2725_ALS_SYS_DELAY_PATH "/sys/class/light_back/light_back/delay"


#define BH1745_ALS_SYS_PATH            "/sys/class/light_front/light_front/"
#define BH1745_ALS_SYS_ENABLE_PATH     "/sys/class/light_front/light_front/enable"
#define BH1745_ALS_SYS_FAC_CAL_PATH    "/sys/class/light_front/light_front/fac_calibrate"
#define BH1745_ALS_SYS_INIT_DEV_PATH   "/sys/class/light_front/light_front/dev_init"
#define BH1745_ALS_SYS_LIGHT_VALUE_PATH   "/sys/class/light_front/light_front/light_value"
#define BH1745_ALS_SYS_LIGHT_CALIBRATE_PATH "/sys/class/light_front/light_front/calibrate"
#define BH1745_ALS_SYS_TP_CFG_PATH    "/sys/class/light_front/light_front/tp_cfg"
#define BH1745_ALS_SYS_CHIP_NAME_PATH    "/sys/class/light_front/light_front/chip_name"
#define BH1745_ALS_SYS_FLUSH_PATH    "/sys/class/light_front/light_front/flush"
#define BH1745_ALS_SYS_DELAY_PATH    "/sys/class/light_front/light_front/delay"


#define SET_LIGHT_FB_PATH  "/sys/class/light/light/light_fb"

#define VIR_PROX_CHIP_NAME "virtual light"

#define LIGHT_FRONT 1
#define LIGHT_BACK  2
#define BUF_MAX_SIZE  20

static dev_t  virtual_light_dev_t;
static struct class  *virtual_light_class;

static struct virtual_light_data *virtual_als_data;
struct input_dev *virtual_als_input_dev;

/*----------------------------------------------------------------------------*/

static void virtual_light_enable(int fb_type)
{
    char enable_buf[2] = {'1', '\0'};
    char disable_buf[2] = {'0', '\0'};

    SENSOR_LOG_INFO("fb_type is %d \n",fb_type);
    switch (fb_type)
    {
        case LIGHT_FRONT:
            sensor_write_file(TMD2725_ALS_SYS_ENABLE_PATH, disable_buf, sizeof(disable_buf));
            sensor_write_file(BH1745_ALS_SYS_ENABLE_PATH, enable_buf, sizeof(enable_buf));
            break;

        case LIGHT_BACK:
            sensor_write_file(BH1745_ALS_SYS_ENABLE_PATH, disable_buf, sizeof(disable_buf));
            sensor_write_file(TMD2725_ALS_SYS_ENABLE_PATH, enable_buf, sizeof(enable_buf));
            break;

        default:
			SENSOR_LOG_ERROR("enable fail Prox type error");
			break;
    }
}

static void virtual_light_disable(void)
{
    char disable_buf[2] = {'0','\0'};
    sensor_write_file(BH1745_ALS_SYS_ENABLE_PATH, disable_buf, sizeof(disable_buf));
    sensor_write_file(TMD2725_ALS_SYS_ENABLE_PATH, disable_buf, sizeof(disable_buf));
}

static int  set_light_fb(struct virtual_light_data *data, u8 value)
{
	SENSOR_LOG_INFO("light_fb value = %d\n", value);
	if(data->fb_status != value && (value == LIGHT_FRONT || value == LIGHT_BACK))
	{
		AMS_MUTEX_LOCK(&data->rw_lock);
		data->fb_status = value;
		if(data->enable_status == true)
			virtual_light_enable(data->fb_status);
		AMS_MUTEX_UNLOCK(&data->rw_lock);
	}
	else if (value != LIGHT_FRONT && value != LIGHT_BACK)
	{
		SENSOR_LOG_ERROR("value is not legal \n");
		return -EINVAL;
	}
	return 0;
}

static ssize_t light_fb_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct virtual_light_data *data = dev_get_drvdata(dev);
	u8 value;
	value = simple_strtoul(buf, NULL, 10 );

	if (set_light_fb(data, value) < 0)
		return -EINVAL;
	return count;
}

static ssize_t light_fb_show(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	struct virtual_light_data *data = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", data->fb_status);
}
 static ssize_t virtual_light_enable_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t size)
{
    int ret;
    int enable;
    struct virtual_light_data *data = dev_get_drvdata(dev);

    ret = kstrtoint(buf, 0, &enable);
    if (ret)
        return -EINVAL;
    SENSOR_LOG_INFO(" virtual_light enable is %d \n", enable);
    switch(enable)
    {
        case 0:
            AMS_MUTEX_LOCK(&data->rw_lock);
            virtual_light_disable();
            data->enable_status = false;
            AMS_MUTEX_UNLOCK(&data->rw_lock);
            break;

        case 1:
            AMS_MUTEX_LOCK(&data->rw_lock);
            virtual_light_enable(data->fb_status);
            data->enable_status = true;
            AMS_MUTEX_UNLOCK(&data->rw_lock);
            break;
        default:
            return -EINVAL;
    }
	/*
	If HAL need light front or back status, open it.
	And HAL need add process REL_RX event code
	*/
#ifdef REPORT_FB_STATUS
	virtual_light_report_event(data->fb_status, REL_RX);
#endif
    return size;
}
static ssize_t virtual_light_enable_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    u8 enable;
	struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  LIGHT_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_ENABLE_PATH, &enable,1);
            SENSOR_LOG_INFO("LIGHT_FRONT read enable is %c\n",enable);
            break;

        case LIGHT_BACK:
            // read  LIGHT_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_ENABLE_PATH, &enable, 1);
            SENSOR_LOG_INFO("LIGHT_BACK read enable is %c\n",enable);
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}
    return sprintf(buf, "%c\n", enable);

}

static ssize_t virtual_light_factory_cal_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    char rbuf[BUF_MAX_SIZE];
	struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  LIGHT_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_FAC_CAL_PATH, rbuf,sizeof(rbuf));
            break;

        case LIGHT_BACK:
            // read  LIGHT_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_FAC_CAL_PATH, rbuf,sizeof(rbuf));
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}

	if(ret > BUF_MAX_SIZE)
		ret = BUF_MAX_SIZE;

	memcpy(buf, rbuf, ret);
	return ret;
}


static ssize_t virtual_light_factory_cal_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t size)
{
    int ret;
    struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            ret=sensor_write_file(BH1745_ALS_SYS_FAC_CAL_PATH, buf, size);
            SENSOR_LOG_IF(data->debug_level,"LIGHT_FRONT_FACTORY_CAL\n");
            break;

        case LIGHT_BACK:
            ret=sensor_write_file(TMD2725_ALS_SYS_FAC_CAL_PATH, buf, size);
            SENSOR_LOG_IF(data->debug_level,"LIGHT_BACK_FACTORY_CAL\n");
            break;
        default:
            return -EINVAL;
    }

    if (ret < 0) {
		SENSOR_LOG_ERROR("wirte light factory cal file fail\n");
		return ret;
	}
    return size;
}

static ssize_t virtual_light_dev_init_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    int thre_max;
    char rbuf[BUF_MAX_SIZE];
	struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  LIGHT_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_INIT_DEV_PATH, rbuf,sizeof(rbuf));
            sscanf(rbuf, "%d", &thre_max);
            SENSOR_LOG_IF(data->debug_level,"LIGHT_FRONT read enable is %d\n",thre_max);
            break;

        case LIGHT_BACK:
            // read  LIGHT_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_INIT_DEV_PATH, rbuf,sizeof(rbuf));
            sscanf(rbuf, "%d", &thre_max);
            SENSOR_LOG_IF(data->debug_level,"LIGHT_BACK read enable is %d\n",thre_max);
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}
    return sprintf(buf, "%d\n", thre_max);

}


static ssize_t virtual_light_dev_init_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t size)
{
    int ret;
    int val = 0;
    char init_buf[BUF_MAX_SIZE] = {'1', '\0'};

    ret = kstrtoint(buf, 0, &val);
    if(val != 0){
		SENSOR_LOG_INFO("Double light dev init\n");
		ret=sensor_write_file(BH1745_ALS_SYS_INIT_DEV_PATH, init_buf, sizeof(init_buf));
		ret=sensor_write_file(TMD2725_ALS_SYS_INIT_DEV_PATH, init_buf, sizeof(init_buf));
		if (ret < 0) {
			SENSOR_LOG_ERROR("light dev init fail\n");
			return ret;
		}
	}
    return size;
}

static ssize_t virtual_light_value_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    int thre_max;
    char rbuf[BUF_MAX_SIZE];
	struct virtual_proximity_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  PROX_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_LIGHT_VALUE_PATH, rbuf,sizeof(rbuf));
            sscanf(rbuf, "%d", &thre_max);
            SENSOR_LOG_IF(data->debug_level,"PROX_FRONT read enable is %d\n",thre_max);
            break;

        case LIGHT_BACK:
            // read  PROX_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_LIGHT_VALUE_PATH, rbuf,sizeof(rbuf));
            sscanf(rbuf, "%d", &thre_max);
            SENSOR_LOG_IF(data->debug_level,"PROX_BACK read enable is %d\n",thre_max);
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}
    return sprintf(buf, "%d\n", thre_max);

}

static ssize_t virtual_light_calibrate_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    int thre_max;
    char rbuf[BUF_MAX_SIZE];
	struct virtual_proximity_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  PROX_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_LIGHT_CALIBRATE_PATH, rbuf,sizeof(rbuf));
            sscanf(rbuf, "%d", &thre_max);
            SENSOR_LOG_IF(data->debug_level,"PROX_FRONT read enable is %d\n",thre_max);
            break;

        case LIGHT_BACK:
            // read  PROX_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_LIGHT_CALIBRATE_PATH, rbuf,sizeof(rbuf));
            sscanf(rbuf, "%d", &thre_max);
            SENSOR_LOG_IF(data->debug_level,"PROX_BACK read enable is %d\n",thre_max);
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}
    return sprintf(buf, "%d\n", thre_max);

}


static ssize_t virtual_light_rgb_config_tpinfo_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    char rbuf[BUF_MAX_SIZE];

    ret = sensor_read_file(BH1745_ALS_SYS_TP_CFG_PATH, rbuf,sizeof(rbuf));
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}

	if(ret > BUF_MAX_SIZE)
		ret = BUF_MAX_SIZE;

	memcpy(buf, rbuf, ret);
	return ret;
}


static ssize_t virtual_light_rgb_config_tpinfo_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t size)
{
    int ret;

    ret=sensor_write_file(BH1745_ALS_SYS_TP_CFG_PATH, buf, size);
    if (ret < 0) {
		SENSOR_LOG_ERROR("wirte light factory cal file fail\n");
		return ret;
	}
    return size;
}

static ssize_t virtual_light_rgb_chipid_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    char rbuf[BUF_MAX_SIZE];
	struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  LIGHT_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_CHIP_NAME_PATH, rbuf,sizeof(rbuf));
            break;

        case LIGHT_BACK:
            // read  LIGHT_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_CHIIP_NAME_PATH, rbuf,sizeof(rbuf));
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}

	if(ret > BUF_MAX_SIZE)
		ret = BUF_MAX_SIZE;

	memcpy(buf, rbuf, ret);
	return ret;

}

static ssize_t virtual_light_flush_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    char rbuf[BUF_MAX_SIZE];
	struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  LIGHT_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_FLUSH_PATH, rbuf,sizeof(rbuf));
            break;

        case LIGHT_BACK:
            // read  LIGHT_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_FLUSH_PATH, rbuf,sizeof(rbuf));
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}

	if(ret > BUF_MAX_SIZE)
		ret = BUF_MAX_SIZE;

	memcpy(buf, rbuf, ret);
	return ret;

}

static ssize_t virtual_light_delay_show(struct device *dev,
    struct device_attribute *attr, char *buf)
{
    int ret;
    char rbuf[BUF_MAX_SIZE];
	struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            // read  LIGHT_FRONT enable status
            ret = sensor_read_file(BH1745_ALS_SYS_DELAY_PATH, rbuf,sizeof(rbuf));
            break;

        case LIGHT_BACK:
            // read  LIGHT_BACK enable status
            ret = sensor_read_file(TMD2725_ALS_SYS_DELAY_PATH, rbuf,sizeof(rbuf));
            break;
        default:
            return -EINVAL;

    }
    if (ret < 0) {
		SENSOR_LOG_ERROR("read file fail\n");
		return ret;
	}

	if(ret > BUF_MAX_SIZE)
		ret = BUF_MAX_SIZE;

	memcpy(buf, rbuf, ret);
	return ret;

}

static ssize_t virtual_light_delay_store(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t size)
{
    int ret;
    struct virtual_light_data *data = dev_get_drvdata(dev);

    switch(data->fb_status)
    {
        case LIGHT_FRONT:
            ret=sensor_write_file(BH1745_ALS_SYS_DELAY_PATH, buf, size);
            SENSOR_LOG_IF(data->debug_level,"LIGHT_FRONT_DELAY_STORE\n");
            break;

        case LIGHT_BACK:
            ret=sensor_write_file(TMD2725_ALS_SYS_DELAY_PATH, buf, size);
            SENSOR_LOG_IF(data->debug_level,"LIGHT_BACK_DELAY_STORE\n");
            break;
        default:
            return -EINVAL;
    }

    if (ret < 0) {
		SENSOR_LOG_ERROR("wirte light delay fail\n");
		return ret;
	}
    return size;
}

static struct device_attribute attrs_als_device[] = {
	__ATTR(enable, 0664, virtual_light_enable_show, virtual_light_enable_store),
    __ATTR(light_fb,0660,light_fb_show,light_fb_store),
	__ATTR(dev_init,0664, virtual_light_dev_init_show, virtual_light_dev_init_store),
    __ATTR(fac_calibrate,0664, virtual_light_factory_cal_show, virtual_light_factory_cal_store),
    __ATTR(light_value,0444, virtual_light_value_show, NULL),
    __ATTR(calibrate,0444, virtual_light_calibrate_show, NULL),
	__ATTR(tp_cfg, 0664, virtual_light_rgb_config_tpinfo_show, virtual_light_rgb_config_tpinfo_store),
	__ATTR(chip_name, 0440, virtual_light_rgb_chipid_show, NULL),
	__ATTR(delay, 0664, virtual_light_delay_show, virtual_light_delay_store),
	__ATTR(flush, 0444, virtual_light_flush_show, NULL),
};

#ifdef NUBIA_LIGHT_NOTIFY_LCD
int light_fb_notifier_callback(struct notifier_block *self,
                         unsigned long event, void *data)
{
	struct virtual_light_data *light_data =
			container_of(self, struct virtual_light_data, light_fb_notify);
		struct fb_event *fb_event = data;
		int *blank;
		char front_buf[2] = {'1', '\0'};
		char back_buf[2] = {'2', '\0'};
		 if (fb_event && fb_event->data && light_data)
		{
			if(event == MSM_DRM_SWITCH_EVENT_BLANK){
				blank = fb_event->data;
				switch (*blank)
				{
					case MSM_DRM_MAJOR_BLANK_UNBLANK:
						light_data->lcd_stat |= LCD_FRONT_BIT;
						SENSOR_LOG_INFO("LCD STAT: FRONT ON");
						break;
					case MSM_DRM_MAJOR_POWERDOWN:
						light_data->lcd_stat &=~ LCD_FRONT_BIT;
						SENSOR_LOG_INFO("Clear LCD FRONT STAT");
						break;
					case MSM_DRM_SLAVE_BLANK_UNBLANK:
						light_data->lcd_stat |= LCD_BACK_BIT;
						SENSOR_LOG_INFO("LCD STAT: BACK ON");
						break;
					case MSM_DRM_SLAVE_POWERDOWN:
						light_data->lcd_stat &=~ LCD_BACK_BIT;
						SENSOR_LOG_INFO("Clear LCD BACK STAT");
						break;
					default:
						SENSOR_LOG_ERROR("No have state to process\n");
				}

				SENSOR_LOG_INFO("lcd_stat: %d",light_data->lcd_stat);
				switch (light_data->lcd_stat)
				{
					case LCD_FRONT_BIT:
					case LCD_FRONT_BACK_BIT:
						SENSOR_LOG_INFO("set light is front(lcd_stat is 2 or 6)");
						sensor_write_file(SET_LIGHT_FB_PATH, front_buf, sizeof(front_buf));
						break;
					case LCD_BACK_BIT:
						SENSOR_LOG_INFO("set light is back(lcd_stat is 4)");
						sensor_write_file(SET_LIGHT_FB_PATH, back_buf, sizeof(back_buf));
						break;
					default:
						SENSOR_LOG_ERROR("No have event to process\n");
				}
			}
		}

    return NOTIFY_DONE;
}


static void light_fb_notifier_register(void){
    int ret = 0;
	virtual_als_data->light_fb_notify.notifier_call = light_fb_notifier_callback;
    ret = msm_drm_panel_register_client(&virtual_als_data->light_fb_notify);
    if (ret < 0){
        SENSOR_LOG_ERROR("Failed to register light notify cilent\n");
    }
}
#endif


int virtual_light_register(void)
{
    int err = 0;

	SENSOR_LOG_INFO("virtual_light_register_sys start\n");

    virtual_als_data = kzalloc(sizeof(struct virtual_light_data), GFP_KERNEL);
	if (!virtual_als_data) {
		SENSOR_LOG_ERROR("kzalloc virtual_als_data failed\n");
		err = -ENOMEM;
		goto exit;
	}
	mutex_init(&virtual_als_data->rw_lock);
	virtual_als_data->flag_prox_debug = false;
	virtual_als_data->fb_status = LIGHT_FRONT;
#ifdef NUBIA_LIGHT_NOTIFY_LCD
	virtual_als_data->lcd_stat = LCD_FRONT_BIT;
#endif
    virtual_als_data->report_fb_status = 0;
    virtual_als_data->report_value = 0;
    virtual_als_data->enable_status = false;

    virtual_light_class = class_create(THIS_MODULE, VIR_LIGHT_DRV_NAME);
    alloc_chrdev_region(&virtual_light_dev_t, 0, 1, VIR_LIGHT_DRV_NAME);
    virtual_als_data->als_dev = device_create(virtual_light_class, 0,
        virtual_light_dev_t, 0, VIR_LIGHT_DRV_NAME);
	if (IS_ERR(virtual_als_data->als_dev)) {
		SENSOR_LOG_ERROR("device_create light failed\n");
		goto create_als_dev_failed;
	}

	dev_set_drvdata(virtual_als_data->als_dev, virtual_als_data);

	err = sensor_create_sysfs_interfaces(virtual_als_data->als_dev, attrs_als_device, ARRAY_SIZE(attrs_als_device));
	if (err < 0) {
		SENSOR_LOG_ERROR("create sysfs interfaces failed\n");
		goto create_sysfs_interface_failed;
	}

	/* allocate light input_device */

    virtual_als_input_dev = input_allocate_device();
    if (IS_ERR_OR_NULL(virtual_als_input_dev)) {
		err = -ENOMEM;
		SENSOR_LOG_ERROR("could not allocate virtual input device\n");
		goto allocate_input_device_failed;
	}
	input_set_drvdata(virtual_als_input_dev, virtual_als_data);
	virtual_als_input_dev->name = INPUT_NAME_VIR_LIGHT;
	virtual_als_input_dev->id.bustype = BUS_I2C;

	set_bit(EV_REL, virtual_als_input_dev->evbit);
	set_bit(REL_X, virtual_als_input_dev->relbit);
	set_bit(REL_Y, virtual_als_input_dev->relbit);

	err = input_register_device(virtual_als_input_dev);
	if (err < 0) {
		SENSOR_LOG_ERROR("could not register virtual_als input device\n");
		err = -ENOMEM;
		goto register_input_device_failed;
	}

    virtual_als_data->als_input_dev = virtual_als_input_dev;

#ifdef NUBIA_LIGHT_NOTIFY_LCD
	light_fb_notifier_register();
#endif
	SENSOR_LOG_INFO("virtual_light_register ok.\n");

	return 0;
register_input_device_failed:
	input_unregister_device(virtual_als_input_dev);
allocate_input_device_failed:
	input_free_device(virtual_als_input_dev);
create_sysfs_interface_failed:
	sensor_remove_sysfs_interfaces(virtual_als_data->als_dev, attrs_als_device, ARRAY_SIZE(attrs_als_device));
create_als_dev_failed:
	virtual_als_data->als_dev = NULL;
	device_destroy(virtual_light_class, virtual_light_dev_t);
	class_destroy(virtual_light_class);
exit:
	return err;
}


void virtual_light_unregister(void)
{
	struct device *dev = virtual_als_data->als_dev;

	input_unregister_device(virtual_als_data->als_input_dev);
	input_free_device(virtual_als_data->als_input_dev);
	sensor_remove_sysfs_interfaces(dev, attrs_als_device, ARRAY_SIZE(attrs_als_device));
	mutex_destroy(&virtual_als_data->rw_lock);
	kfree(virtual_als_data);
}
