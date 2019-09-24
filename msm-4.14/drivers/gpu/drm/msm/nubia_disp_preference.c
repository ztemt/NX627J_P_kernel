/*
 * nubia_disp_preference.c - nubia lcd display color enhancement and temperature setting
 *	      Linux kernel modules for mdss
 *
 * Copyright (c) 2015 nubia <nubia@nubia.com.cn>
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * Supports NUBIA lcd display color enhancement and color temperature setting
 */

/*------------------------------ header file --------------------------------*/
#include "nubia_disp_preference.h"
#include <linux/delay.h>
#include <linux/msm_drm_notify.h>

#ifdef CONFIG_NUBIA_SWITCH_LCD
#define NUBIA_BACK_LCD_NOT_EXIST 0x88
extern int goodix_ts_exist_register_notifier(struct notifier_block *nb);
extern int goodix_ts_exist_unregister_notifier(struct notifier_block *nb);
unsigned int lcd_states = DISPLAY_BOTH_PRIMARY_SECOND;
#endif
static struct dsi_display *nubia_display = NULL;
static struct dsi_display *nubia_sec_display = NULL;
extern void dsi_panel_notifier(int event, unsigned long data);
/*------------------------------- variables ---------------------------------*/
static struct kobject *enhance_kobj = NULL;
struct nubia_disp_type nubia_disp_val = {
	.en_cabc = 1,
	.cabc = CABC_OFF,
	.en_aod_mode = 1,
	.aod_mode = AOD_OFF,
#ifdef CONFIG_NUBIA_SWITCH_LCD
	.en_sec_aod_mode = 1,
	.sec_aod_mode = AOD_OFF,
	.nubia_mode = 1000,
	.hbm_mode = HBM_OFF,
	.srgb_mode = sRGB_OFF,
	.en_sec_cabc = 1,
	.sec_cabc = CABC_OFF,
#endif
	.disp_reset = 0,
#ifdef CONFIG_NUBIA_FPS_DYNAMIC
	.en_dynamic_fps = 1,
	.fps = FPS_90,
	.en_mode = 1,
	.mode = MODE_BRIGHT,
#endif
};
unsigned int fps_store = 90;
unsigned int mode_store = 0;
#ifdef CONFIG_NUBIA_SWITCH_LCD
static int lcd_exist_nb_handler(struct notifier_block *cb, unsigned long code, void *unused)
{
	NUBIA_DISP_INFO("jbc %s code:%lu\n", __func__, code);
	switch (code) {
	case NUBIA_BACK_LCD_NOT_EXIST:
		lcd_states = DISPLAY_ONLY_PRIMARY;
		NUBIA_DISP_INFO("[test] %s Back lcd and tp is not exist",__func__);
		break;
	default:
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block lcd_exist_nb = {
	.notifier_call = lcd_exist_nb_handler,
};

static ssize_t lcd_state_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	pr_err("[test] %s(jbc): start lcd_states = %d\n", __func__,lcd_states);
	return snprintf(buf, PAGE_SIZE, "%d\n", lcd_states);
}
#endif

static ssize_t cabc_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
		if (nubia_disp_val.en_cabc)
				return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.cabc);
		else
				return snprintf(buf, PAGE_SIZE, "NULL\n");
}

static ssize_t cabc_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;
	if(!nubia_disp_val.en_cabc) {
		NUBIA_DISP_ERROR("no cabc\n");
		return size;
	}
	sscanf(buf, "%d", &val);
	if ((val != CABC_OFF) && (val != CABC_LEVEL1) &&
		(val != CABC_LEVEL2) && (val != CABC_LEVEL3)) {
			NUBIA_DISP_ERROR("invalid cabc val = %d\n", val);
			return size;
		}
	NUBIA_DISP_INFO("cabc value = %d\n", val);

	if(nubia_display == NULL)
		return size;

#ifdef CONFIG_NUBIA_LCD_DISP_PREFERENCE	
	ret = nubia_dsi_panel_cabc(nubia_display->panel, val);
	if (ret == 0) {
		nubia_disp_val.cabc = val;
		NUBIA_DISP_INFO("success to set cabc as = %d\n", val);
	}

#endif
	return size;
}

#ifdef CONFIG_NUBIA_SWITCH_LCD
static ssize_t sec_cabc_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	if (nubia_disp_val.en_sec_cabc)
		return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.sec_cabc);
	else
		return snprintf(buf, PAGE_SIZE, "NULL\n");
}

static ssize_t sec_cabc_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;

	if(!nubia_disp_val.en_sec_cabc) {
		NUBIA_DISP_ERROR("no secondary cabc\n");
		return size;
	}

	sscanf(buf, "%d", &val);
	if ((val != CABC_OFF) && (val != CABC_LEVEL1) &&
		(val != CABC_LEVEL2) && (val != CABC_LEVEL3)) {
		NUBIA_DISP_ERROR("invalid secondary cabc val = %d\n", val);
		return size;
	}

	NUBIA_DISP_INFO("secondary cabc value = %d\n", val);

	if(nubia_sec_display == NULL)
		return size;

	ret = nubia_dsi_panel_cabc(nubia_sec_display->panel, val);
	if (ret == 0) {
		nubia_disp_val.sec_cabc = val;
		NUBIA_DISP_INFO("success to set secondary cabc as = %d\n", val);
	}
	return size;
}

static ssize_t aod_mode_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	if (nubia_disp_val.en_aod_mode)
		return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.aod_mode);
	else
		return snprintf(buf, PAGE_SIZE, "NULL\n");

}

static ssize_t aod_mode_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;

	if(!nubia_disp_val.en_aod_mode) {
		NUBIA_DISP_ERROR("no aod mode\n");
		return size;
	}

	sscanf(buf, "%d", &val);

	if ((val < AOD_OFF) || (val > AOD_BRIGHTNESS_L6)) {
		NUBIA_DISP_ERROR("invalid AOD mode val = %d\n", val);
		return size;
	}

	NUBIA_DISP_INFO("aod mode value = %d\n", val);

	if(nubia_display == NULL)
		return size;

	if (val >= AOD_BRIGHTNESS_L1) {
		ret = nubia_set_aod_brightness(nubia_display->panel, val);
		if (!ret) {
			nubia_disp_val.aod_mode = val;
			NUBIA_DISP_INFO("success to set aod brightness as = %d\n", val);
		}
	} else {
		ret = nubia_dsi_display_aod(nubia_display, val);
		if (!ret) {
			nubia_disp_val.aod_mode = val;
			NUBIA_DISP_INFO("success to set aod mode as = %d\n", val);
		}
	}

	/* when exit aod mode,should update panel brightness*/
	if (val == AOD_OFF) {
		mdelay(32);//delay 2 frames time make sure exit aod success
		ret = nubia_update_brightness(nubia_display->drm_conn);
		if (ret)
			NUBIA_DISP_INFO("nubia update brightness failed, ret = %d\n", ret);
	}

	return size;
}

static ssize_t sec_aod_mode_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	if (nubia_disp_val.en_sec_aod_mode)
		return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.sec_aod_mode);
	else
		return snprintf(buf, PAGE_SIZE, "NULL\n");

}

static ssize_t sec_aod_mode_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;

	if(!nubia_disp_val.en_sec_aod_mode) {
		NUBIA_DISP_ERROR("no sec aod mode\n");
		return size;
	}

	sscanf(buf, "%d", &val);

	if ((val < AOD_OFF) || (val > AOD_BRIGHTNESS_L6)) {
		NUBIA_DISP_ERROR("invalid sec AOD mode val = %d\n", val);
		return size;
	}

	NUBIA_DISP_INFO("sec aod mode value = %d\n", val);

	if(nubia_sec_display == NULL)
		return size;

	if (val >= AOD_BRIGHTNESS_L1) {
		ret = nubia_set_aod_brightness(nubia_sec_display->panel, val);
		if (!ret) {
			nubia_disp_val.sec_aod_mode = val;
			NUBIA_DISP_INFO("success to set sec aod brightness as = %d\n", val);
		}
	} else {
		ret = nubia_dsi_display_aod(nubia_sec_display, val);
		if (!ret) {
			nubia_disp_val.sec_aod_mode = val;
			NUBIA_DISP_INFO("success to set sec aod mode as = %d\n", val);
		}
	}

	/* when exit aod mode,should update panel brightness*/
	if (val == AOD_OFF) {
		mdelay(32);//delay 2 frames time make sure exit aod success
		ret = nubia_update_brightness(nubia_sec_display->drm_conn);
		if (ret)
			NUBIA_DISP_INFO("nubia update sec brightness failed, ret = %d\n", ret);
	}

	return size;
}

static ssize_t nubia_mode_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.nubia_mode);
}

static ssize_t nubia_mode_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;

	sscanf(buf, "%d", &val);

	if ((val > 2100) || (val < 1000)) {
		NUBIA_DISP_ERROR("invalid nubia mode, val = %d\n", val);
		return size;
	}

	NUBIA_DISP_INFO("nubia mode value = %d\n", val);

	nubia_disp_val.nubia_mode = val;
	if (2002 == val) {
		pr_err("nubia mode value 2002val\n");
		//dsi_panel_notifier(MSM_DRM_SWITCH_EARLY_EVENT_BLANK,MSM_DRM_MAJOR_BLANK_UNBLANK);
	}else if (2001 == val) {
		pr_err("nubia mode value 2001val\n");
		dsi_panel_notifier(MSM_DRM_SWITCH_EARLY_EVENT_BLANK,PANEL_READY_STATE);
	}else if (2005 == val) {
		pr_err("nubia mode value 2005val\n");
		dsi_panel_notifier(MSM_DRM_SWITCH_EARLY_EVENT_BLANK,BACK_SCREEN_EFFECT);
	}else if (2000 == val) {
		pr_err("nubia mode value 2000val\n");
		dsi_panel_notifier(MSM_DRM_SWITCH_EARLY_EVENT_BLANK,PANEL_READY_STATE);
	}
	return size;
}

int get_nubia_mode()
{
	return nubia_disp_val.nubia_mode;
}
static ssize_t hbm_mode_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	if (nubia_disp_val.hbm_mode)
		return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.hbm_mode);
	else
		return snprintf(buf, PAGE_SIZE, "NULL\n");

}

static ssize_t hbm_mode_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;

	if(!nubia_disp_val.hbm_mode) {
		NUBIA_DISP_ERROR("no hbm mode\n");
		return size;
	}

	sscanf(buf, "%d", &val);

	if ((val != HBM_OFF) && (val != HBM_ON)) {
		NUBIA_DISP_ERROR("invalid HBM mode val = %d\n", val);
		return size;
	}

	NUBIA_DISP_INFO("HBM mode value = %d\n", val);

	if(nubia_display == NULL)
		return size;

	ret = nubia_dsi_panel_hbm(nubia_display->panel, val);
	if (ret == 0) {
		nubia_disp_val.hbm_mode = val;
		NUBIA_DISP_INFO("success to set hbm mode as = %d\n", val);
	}

	return size;
}

static ssize_t srgb_mode_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	if (nubia_disp_val.srgb_mode)
		return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.srgb_mode);
	else
		return snprintf(buf, PAGE_SIZE, "NULL\n");

}

static ssize_t srgb_mode_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;

	if(!nubia_disp_val.srgb_mode) {
		NUBIA_DISP_ERROR("no sRGB mode\n");
		return size;
	}

	sscanf(buf, "%d", &val);

	if ((val != sRGB_OFF) && (val != sRGB_ON)) {
		NUBIA_DISP_ERROR("invalid sRGB mode val = %d\n", val);
		return size;
	}

	NUBIA_DISP_INFO("sRGB mode value = %d\n", val);

	if(nubia_display == NULL)
		return size;

	ret = nubia_dsi_panel_srgb(nubia_display->panel, val);
	if (ret == 0) {
		nubia_disp_val.srgb_mode = val;
		NUBIA_DISP_INFO("success to set sRGB mode as = %d\n", val);
	}

	return size;
}
#endif

static ssize_t disp_reset_show(struct kobject *kobj,
        struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.disp_reset);
}

static ssize_t disp_reset_store(struct kobject *kobj,
        struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	sscanf(buf, "%d", &val);
	nubia_disp_val.disp_reset = val;
	NUBIA_DISP_INFO("disp_reset = %d\n", val);
	return size;
}

int get_disp_reset()
{
	return nubia_disp_val.disp_reset;
}

#ifdef CONFIG_NUBIA_FPS_DYNAMIC
static ssize_t preference_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	if (nubia_disp_val.en_mode)
		return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.mode);
	else
		return snprintf(buf, PAGE_SIZE, "NULL\n");

}

static ssize_t preference_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;
	if(!nubia_disp_val.en_mode) {
			NUBIA_DISP_ERROR("no preference mode\n");
			return size;
	}

	sscanf(buf, "%d", &val);
	mode_store = val;
	if ((mode_store != MODE_BRIGHT) && (mode_store != MODE_STANDARD)) {
		NUBIA_DISP_ERROR("invalid preference mode = %d\n", mode_store);
		return size;
	}

	NUBIA_DISP_INFO("preference mode value = %d\n", mode_store);
	if(nubia_display==NULL)
		return size;
	ret = nubia_dsi_panel_preference(nubia_display->panel, mode_store);
	if (ret == 0) {
		nubia_disp_val.mode = mode_store;
		NUBIA_DISP_INFO("success to set preference mode as = %d\n", mode_store);
	}
	return size;
}

static ssize_t dynamic_fps_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	if (nubia_disp_val.en_dynamic_fps)
		return snprintf(buf, PAGE_SIZE, "%d\n", nubia_disp_val.fps);
	else
		return snprintf(buf, PAGE_SIZE, "NULL\n");

}
static ssize_t dynamic_fps_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t size)
{
	uint32_t val = 0;
	int ret = 0;
	uint32_t temp;
	if(!nubia_disp_val.en_dynamic_fps) {
			NUBIA_DISP_ERROR("no dynamic fps mode\n");
			return size;
	}

	sscanf(buf, "%d", &val);
	temp = val;
	if(fps_store!=temp)
		fps_store = temp;
	else
		return size;
	if ((fps_store != FPS_90) && (fps_store != FPS_60)) {
		NUBIA_DISP_ERROR("invalid dynamic fps val = %d\n", fps_store);
		return size;
	}

	NUBIA_DISP_INFO("dynamic fps mode value = %d\n", fps_store);
	if(nubia_display==NULL)
		return size;
	ret = nubia_dsi_panel_dynamic_fps(nubia_display->panel, fps_store);
	if (ret == 0) {
		nubia_disp_val.fps = val;
		NUBIA_DISP_INFO("success to set dynamic fps mode as = %d\n", fps_store);
	}
	return size;
}
#endif

#ifdef CONFIG_NUBIA_DEBUG_LCD_REG

u8 rx_buf[1024] = {0};
u16 rx_len = 0;

u8 hex_to_char(u8 bChar)
{
	if((bChar >= 0x30) && (bChar <= 0x39))
		bChar -= 0x30;
	else if((bChar >= 0x41) && (bChar <= 0x46))
		bChar -= 0x37;
	else if((bChar >= 0x61) && (bChar <= 0x66))
		bChar -= 0x57;
	return bChar;
}

unsigned char char_to_hex(unsigned char bChar)
{
		if((bChar >=0) && (bChar<=9))
			bChar += 0x30;
		else if((bChar >=10) && (bChar<=15))
			bChar += 0x37;
		else bChar = 0xFF;
		return bChar;
}

static void kernel_str_int(const char *buf, unsigned char *tx_buf)
{
	int i = 0, j = 0, k = 0; 
	char *p;
	p = (char *)buf;
	for(i=0; *p != '\n'; i++){
		if(*p == ' '){
			j++;
			k = 0;
		}else{
			tx_buf[j] = (tx_buf[j] << 4) | hex_to_char(*p);
			k++;
		}
		p++;
	}
}

static ssize_t lcd_reg_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	int i = 0, j = 0;
	unsigned char val = 0;
	for(i=0; i< strlen(rx_buf) / sizeof(char); i++){
		val = (rx_buf[i] & 0xF0) >> 4;
		buf[j++] = char_to_hex(val);
		val = rx_buf[i] & 0x0F;
		buf[j++] = char_to_hex(val);
		buf[j++] = ' ';
	}
	buf[j] = '\n';
	//for(i=0; i<j-1; i++)
	//	printk("%c", buf[i]);
	return j;
}

/**
** read and write lcd register
** buf[0] = 1 read;
** 		buf[1] = which register do you want to read
** 		buf[2] = how many num do you want to get
** buf[0] = 0 write
**		buf[1] = which register do you want to write
** 		buf[2] = how many reg num do you want to
**		buf[3] ....  the data to write
**/
static ssize_t lcd_reg_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t size)
{
	int rc = 0, i=0;
	u8 tx_buf[100] = {0};
	u8 *data;
	struct mipi_dsi_device *dsi = &nubia_display->panel->mipi_device;
	if(!dsi){
		pr_err("%s: lcd reg store error, dsi is NULL\n", __func__);
		return 0;
	}

	kernel_str_int(buf, tx_buf);

	//printk("read tx_buf[0] = %d, tx_buf[1] = %d, tx_buf[2] = %d", tx_buf[0] , tx_buf[1], tx_buf[2] );
	if(tx_buf[0] == 1){
		rx_len = tx_buf[2];
		data = (u8*)kzalloc(rx_len, GFP_KERNEL);

		printk("read tx_buf[0] = %d, tx_buf[1] = %d, tx_buf[2] = %d", tx_buf[0] , tx_buf[1], tx_buf[2] );
		rc = dsi_panel_read_data(dsi, tx_buf[1], data, tx_buf[2]);
		printk(">>>>>>> rc = %d \n", rc);
		for(i=0; i<rc; i++){
			printk("%d = %x \n",i, data[i]);
		}
		if(rc > 0)
			memcpy(rx_buf, data, rc);

		//printk("rx_buf[0] =%d, rx_buf[1] = %d \n", data[0], data[1]);
		//printk("rx_buf[0] =%d, rx_buf[1] = %d \n", rx_buf[0], rx_buf[1]);
		kfree(data);
		data = NULL;
		if(rc<0){
			pr_err("%s: read panel data error \n", __func__);
			return 0;
		}
	}else{
		//printk("read tx_buf[0] = %d, tx_buf[1] = %d, tx_buf[2] = %d, tx_buf[3] = %d", tx_buf[0] , tx_buf[1], tx_buf[2], tx_buf[3] );
		rc = dsi_panel_write_data(dsi, tx_buf[1], &tx_buf[3], tx_buf[2]);
	}
	if(rc<0){
		pr_err("%s: write panel data error \n", __func__);
		return 0;
	}
	return size;
}

static ssize_t sec_lcd_reg_store(struct kobject *kobj,
	struct kobj_attribute *attr, const char *buf, size_t size)
{
	int rc = 0, i=0;
	u8 tx_buf[10] = {0};
	u8 *data;
	struct mipi_dsi_device *dsi = &nubia_sec_display->panel->mipi_device;
	if(!dsi){
		pr_err("%s: lcd reg store error, dsi is NULL\n", __func__);
		return 0;
	}

	kernel_str_int(buf, tx_buf);

	//printk("read tx_buf[0] = %d, tx_buf[1] = %d, tx_buf[2] = %d", tx_buf[0] , tx_buf[1], tx_buf[2] );
	if(tx_buf[0] == 1){
		rx_len = tx_buf[2];
		data = (u8*)kzalloc(rx_len, GFP_KERNEL);

		printk("read tx_buf[0] = %d, tx_buf[1] = %d, tx_buf[2] = %d", tx_buf[0] , tx_buf[1], tx_buf[2] );
		rc = dsi_panel_read_data(dsi, tx_buf[1], data, tx_buf[2]);
		printk(">>>>>>> rc = %d \n", rc);
		for(i=0; i<rc; i++){
			printk("%d = %x \n",i, data[i]);
		}
		if(rc > 0)
			memcpy(rx_buf, data, rc);

		//printk("rx_buf[0] =%d, rx_buf[1] = %d \n", data[0], data[1]);
		//printk("rx_buf[0] =%d, rx_buf[1] = %d \n", rx_buf[0], rx_buf[1]);
		kfree(data);
		data = NULL;
		if(rc<0){
			pr_err("%s: read panel data error \n", __func__);
			return 0;
		}
	}else{
		//printk("read tx_buf[0] = %d, tx_buf[1] = %d, tx_buf[2] = %d, tx_buf[3] = %d", tx_buf[0] , tx_buf[1], tx_buf[2], tx_buf[3] );
		rc = dsi_panel_write_data(dsi, tx_buf[1], &tx_buf[3], tx_buf[2]);
	}
	if(rc<0){
		pr_err("%s: write panel data error \n", __func__);
		return 0;
	}
	return size;
}

#endif

static struct kobj_attribute lcd_disp_attrs[] = {
	__ATTR(cabc,        0664, cabc_show,       cabc_store),
#ifdef CONFIG_NUBIA_SWITCH_LCD
	__ATTR(aod_mode, 		 0664, aod_mode_show,		  aod_mode_store),
	__ATTR(sec_aod_mode, 		 0664, sec_aod_mode_show,		  sec_aod_mode_store),
	__ATTR(nubia_mode, 		 0664, nubia_mode_show,		  nubia_mode_store),
	__ATTR(hbm_mode, 		 0664, hbm_mode_show,		  hbm_mode_store),
	__ATTR(srgb_mode, 		 0664, srgb_mode_show,		  srgb_mode_store),
	__ATTR(sec_cabc,        0664, sec_cabc_show,       sec_cabc_store),
	__ATTR(lcd_state,        0664, lcd_state_show,       NULL),
#endif
	__ATTR(disp_reset,		 0664, disp_reset_show, 	  disp_reset_store),
#ifdef CONFIG_NUBIA_FPS_DYNAMIC
	__ATTR(dfps,     0664,dynamic_fps_show, dynamic_fps_store),
	__ATTR(preference, 0664,preference_show,preference_store),
#endif
#ifdef CONFIG_NUBIA_DEBUG_LCD_REG
	__ATTR(lcd_reg,     0664, lcd_reg_show, lcd_reg_store),
	__ATTR(sec_lcd_reg,     0664, lcd_reg_show, sec_lcd_reg_store),
#endif
};

void nubia_set_dsi_ctrl(struct dsi_display *display, const char *dsi_type)
{
	NUBIA_DISP_INFO("start\n");

	if (!strcmp(dsi_type, "primary"))
		nubia_display = display;
	else
		nubia_sec_display = display;
}

static int __init nubia_disp_preference_init(void)
{
	int retval = 0;
	int attr_count = 0;

	NUBIA_DISP_INFO("start\n");

	enhance_kobj = kobject_create_and_add("lcd_enhance", kernel_kobj);

	if (!enhance_kobj) {
		NUBIA_DISP_ERROR("failed to create and add kobject\n");
		return -ENOMEM;
	}

	/* Create attribute files associated with this kobject */
	for (attr_count = 0; attr_count < ARRAY_SIZE(lcd_disp_attrs); attr_count++) {
		retval = sysfs_create_file(enhance_kobj, &lcd_disp_attrs[attr_count].attr);
		if (retval < 0) {
			NUBIA_DISP_ERROR("failed to create sysfs attributes\n");
			goto err_sys_creat;
		}
	}
#ifdef CONFIG_NUBIA_SWITCH_LCD
	goodix_ts_exist_register_notifier(&lcd_exist_nb);
#endif
	NUBIA_DISP_INFO("success\n");

	return retval;

err_sys_creat:
//#ifdef CONFIG_NUBIA_SWITCH_LCD
	//cancel_delayed_work_sync(&nubia_disp_val.lcd_states_work);
//#endif
	for (--attr_count; attr_count >= 0; attr_count--)
		sysfs_remove_file(enhance_kobj, &lcd_disp_attrs[attr_count].attr);

	kobject_put(enhance_kobj);
	return retval;
}

static void __exit nubia_disp_preference_exit(void)
{
	int attr_count = 0;

	for (attr_count = 0; attr_count < ARRAY_SIZE(lcd_disp_attrs); attr_count++)
		sysfs_remove_file(enhance_kobj, &lcd_disp_attrs[attr_count].attr);

	kobject_put(enhance_kobj);
#ifdef CONFIG_NUBIA_SWITCH_LCD
	goodix_ts_exist_unregister_notifier(&lcd_exist_nb);
#endif
}

MODULE_AUTHOR("NUBIA LCD Driver Team Software");
MODULE_DESCRIPTION("NUBIA LCD DISPLAY Color Saturation and Temperature Setting");
MODULE_LICENSE("GPL");
module_init(nubia_disp_preference_init);
module_exit(nubia_disp_preference_exit);
