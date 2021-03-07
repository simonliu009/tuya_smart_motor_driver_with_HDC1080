/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-14     simonliu       the first version
 */
#ifndef __APP_EVENT_H__
#define __APP_EVENT_H__

#define WIFI_RDY_EVT_FLG 1<<0
#define OPEN_EVT_FLG 1<<1
#define CLOSE_EVT_FLG 1<<2
#define PAUSE_EVT_FLG 1<<3
#define RESUME_EVT_FLG 1<<4
#define STOP_EVT_FLG 1<<5
#define POST_PRO_EVT_FLG 1<<6


typedef rt_uint32_t  u32;
typedef rt_uint16_t u16;
typedef rt_uint8_t  u8;

typedef enum{
    launch_screen,
    clock_screen,
    opening_screen,
    closing_screen
} oled_screen;

extern oled_screen current_screen;

typedef enum{
    opened,
    closed,
    opening,
    closing,
//    paused,
    paused_opening,
    paused_closing

} control_state;

typedef struct{
    rt_bool_t powerstate;
    int windowControl;
    int Outdoor_pm25;
    int32_t OutdoorTemperature;
    int32_t IndoorTemperature;
    int32_t OutdoorHumidity;
    int32_t IndoorHumidity;
} thing_property;

extern thing_property curr_property;
extern control_state window_state;
extern rt_event_t wlan_rdy_event;
extern rt_event_t ctrl_event;
extern rt_event_t screen_event;
extern rt_event_t oled_event_in_progress;
extern rt_event_t linkkit_event;
extern struct tm timeinfo;
extern unsigned char smart_weather_str[8];
extern unsigned char wind_speed_state_str[8];




//int app_event_init(void);

#endif /* APPLICATIONS_APP_EVENT_H_ */
