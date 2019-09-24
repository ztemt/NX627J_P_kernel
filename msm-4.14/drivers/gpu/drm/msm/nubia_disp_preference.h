#ifndef _NUBIA_DISP_PREFERENCE_
#define _NUBIA_DISP_PREFERENCE_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include "sde_kms.h"
#include "dsi_panel.h"
#include "dsi_display.h"
#ifdef CONFIG_NUBIA_SWITCH_LCD
#include <linux/msm_drm_notify.h>
#include "sde/sde_connector.h"
#endif

/* ------------------------- General Macro Definition ------------------------*/
#define NUBIA_DISP_COLORTMP_DEBUG        0

enum{
	sRGB_OFF = 23,
	sRGB_ON
};

enum{
	HBM_OFF = 23,
	HBM_ON
};

enum{
	AOD_OFF = 23,
	AOD_ON,
	AOD_BRIGHTNESS_L1,
	AOD_BRIGHTNESS_L2,
	AOD_BRIGHTNESS_L3,
	AOD_BRIGHTNESS_L4,
	AOD_BRIGHTNESS_L5,
	AOD_BRIGHTNESS_L6
};

enum{
	CABC_OFF = 23,
	CABC_LEVEL1,
	CABC_LEVEL2,
	CABC_LEVEL3
};

enum{
	PRIMARY = 1000,
	SECONDARY,
	DUAL_PANEL
};

enum{
	FPS_90 = 90,
	FPS_60 = 60
};

enum{
	MODE_BRIGHT = 0,
	MODE_STANDARD = 1
};
	
#define NUBIA_DISP_LOG_TAG "ZtemtDisp"
#define NUBIA_DISP_LOG_ON

#ifdef  NUBIA_DISP_LOG_ON
#define NUBIA_DISP_ERROR(fmt, args...) printk(KERN_ERR "[%s] [%s: %d] "  fmt, \
	NUBIA_DISP_LOG_TAG, __FUNCTION__, __LINE__, ##args)
#define NUBIA_DISP_INFO(fmt, args...) printk(KERN_ERR "[%s] [%s: %d] "  fmt, \
	NUBIA_DISP_LOG_TAG, __FUNCTION__, __LINE__, ##args)

    #ifdef  NUBIA_DISP_DEBUG_ON
#define  NUBIA_DISP_DEBUG(fmt, args...) printk(KERN_DEBUG "[%s] [%s: %d] "  fmt, \
	NUBIA_DISP_LOG_TAG, __FUNCTION__, __LINE__, ##args)
    #else
#define NUBIA_DISP_DEBUG(fmt, args...)
    #endif
#else
#define NUBIA_DISP_ERROR(fmt, args...)
#define NUBIA_DISP_INFO(fmt, args...)
#define NUBIA_DISP_DEBUG(fmt, args...)
#endif

/* ----------------------------- Structure ----------------------------------*/
struct nubia_disp_type{
  int en_aod_mode;
  int en_cabc;
  unsigned int aod_mode;
  unsigned int cabc;
#ifdef CONFIG_NUBIA_SWITCH_LCD
  int en_sec_aod_mode;
  unsigned int sec_aod_mode;
  int nubia_mode;
  int hbm_mode;
  int srgb_mode;
  int en_sec_cabc;
  unsigned int sec_cabc;
#endif
  int disp_reset;
#ifdef CONFIG_NUBIA_FPS_DYNAMIC
  int en_dynamic_fps;
  unsigned int fps;
  int en_mode;
  unsigned int mode;
#endif
};

#ifdef CONFIG_NUBIA_SWITCH_LCD
enum dsi_panel_state {
	DISPLAY_ONLY_PRIMARY = 1,
	DISPLAY_BOTH_PRIMARY_SECOND,
};
#endif

/* ------------------------- Function Declaration ---------------------------*/
void nubia_disp_preference(void);
void nubia_set_dsi_ctrl(struct dsi_display *display, const char *dsi_type);
#endif
