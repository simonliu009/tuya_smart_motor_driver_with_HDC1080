/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-17     simonliu       the first version
 */



#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_common.h>
#include "oled.h"


#define LOG_TAG     "oled_hwi2c"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>

#define THREAD_PRIORITY         10
#define THREAD_STACK_SIZE       2*1024
#define THREAD_TIMESLICE        5

static rt_thread_t oled_tid = RT_NULL;
static rt_thread_t screen_tid = RT_NULL;
static rt_thread_t opening_screen_tid,closing_screen_tid;

time_t now;

u8 mstr[3] = "";
u8 hstr[3] = "";
u8 sstr[3] = "";
u8 weekstr[4] = "";
u8 monstr[4] = "";
u8 daystr[3] = "";
u8 yearstr[5] = "";

int min = -1, hour = -1, sec = -1, curr_min = 0, curr_hour = 0, curr_sec = 0;

char time_str[32] = "";



void ShowIndoorTempHumi()
{
    /*Indoor*/
    int temp, humi;

    temp = curr_property.IndoorTemperature;
    humi = curr_property.IndoorHumidity;

    if (temp<0)
    {
        OLED_ShowChar(40, 6, '-', 16,0);
        temp = 0 - temp;
        OLED_ShowNum(48, 6, temp, 2, 16,0);
    }
    else {
        OLED_ShowChar(40, 6, ' ', 16,0);
        OLED_ShowNum(48, 6, temp, 2, 16,0);
    }

    OLED_ShowNum(96, 6, humi, 2, 16);
}

void ShowWeatherInfo()
{
    /*室外*/
    int temp, humi;
    temp = curr_property.OutdoorTemperature;

    if (temp<0)
    {
        OLED_ShowChar(40, 4, '-', 16,0);
        temp = 0 - temp;
        OLED_ShowNum(48, 4, temp, 2, 16,0);
    }
    else {
        OLED_ShowChar(40, 4, ' ', 16,0);
        OLED_ShowNum(48, 4, temp, 2, 16,0);
    }

    humi = curr_property.OutdoorHumidity;
    OLED_ShowNum(96, 4, humi, 2, 16);
}


int print_time()
{
    now = time(0);
    localtime_r(&now,&timeinfo);
    strftime(time_str, 31, "%c", &timeinfo); //Sun Aug 19 02:56:02 2012
    LOG_D("Line:%d_%s: time_str:%s , tm_year=%d",__LINE__, __func__,time_str, timeinfo.tm_year);
    return timeinfo.tm_year;
}

void get_date_time()
{
    now = time(0);
    //  LOG_D("now = %ld",now);
    rt_enter_critical();
    localtime_r(&now,&timeinfo);
    //  memcpy(&timeinfo, timeinfo_temp, sizeof(struct tm));
    curr_sec = timeinfo.tm_sec;
    curr_min = timeinfo.tm_min;
    curr_hour =  timeinfo.tm_hour;
    strftime(time_str, 31, "%c", &timeinfo); //Sun Aug 19 02:56:02 2012
//  if (cnt%60 == 0){
//      LOG_D("get_date_time: time_str:%s",time_str);
//  }
    rt_snprintf((char *)weekstr,sizeof(weekstr),"%s",time_str);
    rt_snprintf((char *)monstr,sizeof(monstr),"%s",&time_str[4]);
    rt_snprintf((char *)daystr,sizeof(daystr),"%s",&time_str[8]);
    rt_snprintf((char *)hstr, sizeof(hstr), "%s", &time_str[11]);
    rt_snprintf((char *)mstr, sizeof(mstr), "%s", &time_str[14]);
    rt_snprintf((char *)sstr, sizeof(sstr), "%s", &time_str[17]);
    rt_snprintf((char *)yearstr,sizeof(yearstr),"%s",&time_str[20]);
    rt_exit_critical();

//  LOG_D("__Date__:%s %s %s %s:%s:%s %s ",weekstr, monstr,daystr, hstr, mstr,sstr, yearstr);


}

void ShowWindowState()
{
    OLED_ShowCHinese(92,0,0);
    OLED_ShowCHinese(108,0,1);
    OLED_ShowCHinese(92,2,4);
    if (window_state == closed)
    {
        OLED_ShowCHinese(108,2,6);
    }
    else {
        OLED_ShowCHinese(108,2,5);
    }
}



void ShowTime(rt_bool_t redraw)
{
    if (current_screen == clock_screen)
    {
        get_date_time();

//        if (redraw == RT_TRUE){
//            if (curr_hour != hour)
//            {
//                OLED_ShowString(0, 0, hstr, 32);
//                hour = curr_hour;
//            }
//            else if(curr_min != min)
//            {
//                OLED_ShowString(48, 0, mstr, 32);
//                min = curr_min;
//            }
//            OLED_Refresh();
//        }

        if (curr_hour != hour || redraw == RT_TRUE)
        {
            OLED_ShowString(0, 0, hstr, 32);
            hour = curr_hour;
            OLED_Refresh();
        }
        OLED_ShowChar(32, 0, ':', 32, 0);
        if (curr_min != min || redraw == RT_TRUE)
        {
            OLED_ShowString(48, 0, mstr, 32);
            min = curr_min;
            OLED_Refresh();
        }

    }
}

void ShowWeatherText()
{
    /*室外*/
    OLED_ShowCHinese(0,4,11);
    OLED_ShowCHinese(16,4,13);
    OLED_ShowCHinese(64, 4, 14);//℃
    OLED_ShowChar(112,4,'%',16,0);//百分号

    /*室内*/
    OLED_ShowCHinese(0,6,11);
    OLED_ShowCHinese(16,6,12);
    OLED_ShowCHinese(64, 6, 14);//℃
    OLED_ShowChar(112,6,'%',16,0);//百分号
}

void ShowOpeningScreen()
//void opening_screen_thread_entry()
{
    rt_uint32_t e;
    current_screen = opening_screen;
    LOG_D("ShowOpeningScreen :current_screen:%d",current_screen);
    OLED_Clear();
    OLED_ShowCHinese(32,2,2);
    OLED_ShowCHinese(48,2,3);
    OLED_ShowCHinese(64,2,5);
    OLED_ShowCHinese(80,2,0);
    OLED_ShowString(16, 6, "<---    --->", 16);
    OLED_Refresh();
    return;

}

void ShowClosingScreen()
{
    current_screen = closing_screen;
    LOG_D("ShowClosingScreen: current_screen:%d",current_screen);
    rt_uint32_t e;
    OLED_Clear();
    OLED_ShowCHinese(32,2,2);
    OLED_ShowCHinese(48,2,3);
    OLED_ShowCHinese(64,2,6);
    OLED_ShowCHinese(80,2,0);
    OLED_ShowString(16, 6, "--->    <---", 16);

    OLED_Refresh();
    return;
}


void ShowLaunchScreen()
{
    current_screen = launch_screen;
    LOG_D("current_screen:%d",current_screen);
    OLED_ShowCHinese(20,0,7);
    OLED_ShowCHinese(44,0,8);
    OLED_ShowCHinese(68,0,0);
    OLED_ShowCHinese(92,0,1);

    OLED_ShowString(24, 2, "Powered By", 16);
    OLED_ShowString(32, 4, "Simon Liu", 16);
    OLED_ShowString(0, 6, "Tuya & RT-Thread", 16);
    OLED_Refresh();
}



void ShowClockScreen()
{
    current_screen = clock_screen;
    LOG_D("current_screen:%d",current_screen);
    OLED_Clear();
    get_date_time();
    ShowWeatherText();
    ShowIndoorTempHumi();
    ShowWeatherInfo();
    ShowWindowState();
    ShowTime(RT_TRUE);
    OLED_Refresh();
}

static void oled_thread_entry(void *parameter)
{
    int count = 0;

    MX_I2C4_Init();
    rt_thread_mdelay(500);
    OLED_Init();          //初始化OLED
    OLED_Clear();
    ShowLaunchScreen();
    rt_thread_mdelay(10*1000);
    OLED_Clear();
    rt_thread_mdelay(20);
    ShowClockScreen();
    while(1)
    {
        ShowTime(RT_FALSE);
        rt_thread_mdelay(1000);
        count ++;
    }
}

void start_opening_screen_thread()
{
    opening_screen_tid = rt_thread_create("opening_screen_thread", ShowOpeningScreen, RT_NULL, 4096, THREAD_PRIORITY + 1, 10);
   if(opening_screen_tid != RT_NULL)
   {
       rt_thread_startup(opening_screen_tid);
   }
}

void start_closing_screen_thread()
{
    closing_screen_tid = rt_thread_create("closing_screen_thread", ShowClosingScreen, RT_NULL, 4096, THREAD_PRIORITY + 1, 10);
   if(closing_screen_tid != RT_NULL)
   {
       rt_thread_startup(closing_screen_tid);
   }
}

static void screen_switch_thread_entry()
{
//    rt_err_t result = RT_EOK;
    rt_uint32_t e;
    LOG_D("Screen switch thread started...");
    while(1)
    {


        if (rt_event_recv(screen_event, (OPEN_EVT_FLG|CLOSE_EVT_FLG|PAUSE_EVT_FLG|STOP_EVT_FLG|RESUME_EVT_FLG),
                             RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR,
                             RT_WAITING_FOREVER, &e) == RT_EOK)
           {
               LOG_D("screen_switch_thread_entry: Recv event 0x%02X, switch screen...", e);
               if (e == OPEN_EVT_FLG){
                   LOG_D("start opening screen thread");

                   start_opening_screen_thread();
               }
               else if (e == CLOSE_EVT_FLG){
                   LOG_D("start closing screen thread");


                   start_closing_screen_thread();
               }
               else if (e == RESUME_EVT_FLG){
                   LOG_D("window_state=%d",window_state);
                   if (window_state == paused_closing){
                       LOG_D("window_state is paused_closing, start closing screen thread");
                       start_closing_screen_thread();
                   }
                   if (window_state == paused_opening){
                       LOG_D("window_state is paused_opening, start opening screen thread");
                       start_opening_screen_thread();
                   }
               }

               else {
                   if (e == PAUSE_EVT_FLG)
                   {
                       rt_event_send(oled_event_in_progress, PAUSE_EVT_FLG);
                   }
                   else {
                       rt_event_send(oled_event_in_progress, STOP_EVT_FLG);
                }
                ShowClockScreen();
               }
           }
    }

}

/* 创建线程 */
int app_oled_start(void)
{
    LOG_D("Auto start oled display");
    rt_err_t result = RT_EOK;
    /* 初始化mailbox */

    /* 创建线程*/
    oled_tid = rt_thread_create("oled_display",
                            oled_thread_entry, RT_NULL,
                            THREAD_STACK_SIZE,
                            THREAD_PRIORITY, 20);

    /* 如果获得线程控制块，启动这个线程 */
    if (oled_tid != RT_NULL){
        rt_thread_startup(oled_tid);

    }
    else {
        result = RT_ERROR;
    }

    screen_tid = rt_thread_create("screen_switch_thread",
            screen_switch_thread_entry, RT_NULL,
            THREAD_STACK_SIZE,
            THREAD_PRIORITY, THREAD_TIMESLICE);

    if (screen_tid != RT_NULL)
    {
        rt_thread_startup(screen_tid);
    }
    else {
        result = RT_ERROR;
    }

    return result;

}


MSH_CMD_EXPORT(print_time, print current date time);
INIT_APP_EXPORT(app_oled_start);
