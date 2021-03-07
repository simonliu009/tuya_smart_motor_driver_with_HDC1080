/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-14     simonliu       the first version
 */



#include <rtthread.h>
#include <string.h>
#include "app_event.h"
#define LOG_TAG     "app_event"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>

oled_screen current_screen;
control_state window_state;
thing_property curr_property;

rt_event_t wlan_rdy_event;
rt_event_t ctrl_event;
rt_event_t screen_event;
rt_event_t oled_event_in_progress;
rt_event_t linkkit_event;
struct tm timeinfo = { 0 };
unsigned char smart_weather_str[8] = "";
unsigned char wind_speed_state_str[8] = "";

void thing_property_init()
{
    curr_property.IndoorHumidity = 0;
    curr_property.IndoorTemperature = 0;
    curr_property.OutdoorHumidity = 0;
    curr_property.OutdoorTemperature = 0;
    curr_property.Outdoor_pm25 = 0;
}



int app_event_init(void)
{

    /* 初始化事件对象 */
    wlan_rdy_event = rt_event_create("Internet Ready Event", RT_IPC_FLAG_FIFO);
    ctrl_event     = rt_event_create("Window Control Event", RT_IPC_FLAG_FIFO);
    screen_event   = rt_event_create("OLED Screen Event", RT_IPC_FLAG_FIFO);
    oled_event_in_progress  = rt_event_create("OLED Screen Event2", RT_IPC_FLAG_FIFO);
    linkkit_event  = rt_event_create("Linkkit Event", RT_IPC_FLAG_FIFO);
    window_state = closed;
    thing_property_init();

    if (wlan_rdy_event == NULL)
    {
        LOG_D("wlan_rdy_event init failed.\n");
        return RT_ERROR;
    }
    if (ctrl_event == NULL)
        {
            LOG_D("ctrl_event init failed.\n");
            return RT_ERROR;
        }
    if (screen_event == NULL)
        {
            LOG_D("screen_event init failed.\n");
            return RT_ERROR;
        }
    if (oled_event_in_progress == NULL)
        {
            LOG_D("screen_event2 init failed.\n");
            return RT_ERROR;
            }
    if (linkkit_event == NULL)
            {
                LOG_D("linkkit_event init failed.\n");
                return RT_ERROR;
            }
    return RT_EOK;
}

INIT_APP_EXPORT(app_event_init);
