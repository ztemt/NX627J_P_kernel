#ifndef __CYPRESS8C40141_TOUCHKEY_STRINGIMAGE_H__
#define __CYPRESS8C40141_TOUCHKEY_STRINGIMAGE_H__

#ifdef TARGET_NUBIA_NX629J_V1S
#define CURRENT_NEW_FIRMWARE_VER    0x02

#define LINE_CNT_0 123

const char *stringImage_0[]={
#include "CY8C4014LQI-421_Bootloadable_NX629_V1S_02.cyacd"
};

#else
#define CURRENT_NEW_FIRMWARE_VER    0xF3

#define LINE_CNT_0 119

const char *stringImage_0[]={
#include "CY8C4014LQI-421_Bootloadable_NX629_09.cyacd"
};

#endif

#endif
