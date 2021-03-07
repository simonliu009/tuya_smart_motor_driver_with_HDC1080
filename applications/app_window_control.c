/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-20     simonliu       the first version
 */


#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "app_event.h"
#include <time.h>
#include "mcusdk/mcu_api.h"

#define LOG_TAG              "Window_Control"
#define LOG_LVL              LOG_LVL_DBG
#include <ulog.h>

#define OPEN_PIN GET_PIN(H,2)
#define CLOSE_PIN GET_PIN(A,6)
//#define WIFI_PIN GET_PIN(H,9)

#define IN1_PIN GET_PIN(H,15)
#define IN2_PIN GET_PIN(H,13)

#define DPID_STATUS 1
#define DPID_CONTROL 2

#define CTRL_TIME_TOTAL  5 * 1000; //  systick is 1000Hz

static rt_thread_t window_control_tid;
static rt_timer_t ctrl_timer;

rt_tick_t ctrl_time_start = 0;

rt_tick_t ctrl_time_for_close = 0;
rt_tick_t ctrl_time_for_open = CTRL_TIME_TOTAL;

void show_ctrl_time()
{
    LOG_D("ctrl_time_for_close=%d",ctrl_time_for_close);
    LOG_D("ctrl_time_for_open=%d",ctrl_time_for_open);
}


void open_ioset()
{
    rt_pin_write(IN1_PIN, PIN_HIGH);
    rt_pin_write(IN2_PIN, PIN_LOW);

}

void close_ioset()
{
    rt_pin_write(IN2_PIN, PIN_HIGH);
    rt_pin_write(IN1_PIN, PIN_LOW);
}

//用11态刹车，00态低功耗
void stop_ioset()
{
    rt_pin_write(IN1_PIN, PIN_HIGH);
    rt_pin_write(IN2_PIN, PIN_HIGH);
}

static void stop_operating_cb(void *parameter)
{
    LOG_D("stop_operating_cb  executed...");
    stop_ioset();
    if (window_state == opening)
    {
        window_state = opened;
        ctrl_time_for_open = 0;
        ctrl_time_for_close = CTRL_TIME_TOTAL;
    }
    if (window_state == closing)
    {
        window_state = closed;
        ctrl_time_for_open = CTRL_TIME_TOTAL;
        ctrl_time_for_close = 0;
    }


    if (ctrl_timer != RT_NULL){
        rt_timer_delete(ctrl_timer);
    }

    rt_event_send(screen_event, STOP_EVT_FLG);
    show_ctrl_time();
    mcu_dp_enum_update(DPID_STATUS, window_state );

}




void open_window()
{
    LOG_D("open_window()...");
    window_state = opening;
    ctrl_time_start = rt_tick_get();
    open_ioset();

}

void close_window()
{
    LOG_D("close_window()...");
    window_state = closing;
    ctrl_time_start = rt_tick_get();
    close_ioset();

}

void pause_window()
{
    LOG_D("pause_window()...");
    if (window_state == opening)
    {
        window_state = paused_opening;
        ctrl_time_for_close = ctrl_time_for_close + (rt_tick_get()- ctrl_time_start);
    }
    if (window_state == closing)
    {
        window_state = paused_closing;
        ctrl_time_for_close = ctrl_time_for_close - (rt_tick_get()- ctrl_time_start);
    }
    LOG_D("window_state=%d",window_state);
    ctrl_time_for_open = CTRL_TIME_TOTAL - ctrl_time_for_close;
    stop_ioset();
//    window_state = paused;

}

void open_button_cb()
{
    rt_uint32_t re = RT_EOK;
    LOG_D("Open button is pushed...");
    if (window_state == closed || window_state == paused_opening || window_state == paused_closing)
    {
        re = rt_event_send(screen_event, OPEN_EVT_FLG);
        LOG_D("Send OPEN_EVT_FLG to screen_event:0x%02X, result : %d",OPEN_EVT_FLG, re );
        re = rt_event_send(ctrl_event, OPEN_EVT_FLG);
        LOG_D("Send OPEN_EVT_FLG to ctrl_event:0x%02X, result : %d",OPEN_EVT_FLG, re );
}

    if (window_state == opening)
    {
        re = rt_event_send(screen_event, PAUSE_EVT_FLG);
        LOG_D("Send PAUSE EVENT, result : %d", re );
        re = rt_event_send(ctrl_event, PAUSE_EVT_FLG);
        LOG_D("Send PAUSE EVENT, result : %d", re );
    }

}

void close_button_cb()
{
    rt_uint32_t re = RT_EOK;
    LOG_D("Close button is pushed...");
    if (window_state == opened || window_state == paused_opening || window_state == paused_closing)
    {
        re = rt_event_send(screen_event, CLOSE_EVT_FLG);
        LOG_D("Send CLOSE_EVT_FLG to screen_event:0x%02X, result : %d",CLOSE_EVT_FLG, re );
        re = rt_event_send(ctrl_event, CLOSE_EVT_FLG);
        LOG_D("Send CLOSE_EVT_FLG to ctrl_event:0x%02X, result : %d",CLOSE_EVT_FLG, re );
    }

    if (window_state == closing)
    {
        re = rt_event_send(screen_event, PAUSE_EVT_FLG);
        LOG_D("Send PAUSE EVENT, result : %d", re );
        re = rt_event_send(ctrl_event, PAUSE_EVT_FLG);
        LOG_D("Send PAUSE EVENT, result : %d", re );
    }

}



static void window_control_thread_entry()
{
    rt_uint32_t event_recv;
    rt_err_t re = RT_EOK;

    LOG_D("window_control_thread started. Waiting for events....");

    while(1)
    {
        if (rt_event_recv(ctrl_event, OPEN_EVT_FLG|CLOSE_EVT_FLG|PAUSE_EVT_FLG|RESUME_EVT_FLG,
                          RT_EVENT_FLAG_OR|RT_EVENT_FLAG_CLEAR,
                          RT_WAITING_FOREVER, &event_recv) == RT_EOK)
        {
//            LOG_D("Recv event: 0x%02x", event_recv);
            if (event_recv == OPEN_EVT_FLG)
            {
                if (window_state == closed || window_state == paused_opening || window_state == paused_closing)
                {
                    open_window();
//                    mcu_dp_enum_update(DPID_CONTROL, 0);
                    ctrl_timer = rt_timer_create("timer_win_ctrl", stop_operating_cb,//回调函数timeout2
                                                 RT_NULL,  ctrl_time_for_open,
                                                 RT_TIMER_FLAG_ONE_SHOT);//执行一次回调函数
                    if (ctrl_timer != RT_NULL){
                        re = rt_timer_start(ctrl_timer);
                        if (re == RT_EOK){
                            LOG_D("Start timer success...");
                        }
                        else {
                            LOG_D("Start timer failed:%d",re);
                        }

                    }
                }
            }

            if (event_recv == CLOSE_EVT_FLG)
            {
                if (window_state == opened || window_state == paused_opening || window_state == paused_closing)
                {
                    close_window();
//                    mcu_dp_enum_update(DPID_CONTROL, 1);
                    ctrl_timer = rt_timer_create("timer_win_ctrl", stop_operating_cb,//回调函数timeout2
                                                 RT_NULL,  ctrl_time_for_close,
                                                 RT_TIMER_FLAG_ONE_SHOT);//执行一次回调函数
                    if (ctrl_timer != RT_NULL){
                        re = rt_timer_start(ctrl_timer);
                        if (re == RT_EOK){
                            LOG_D("Start timer success...");
                        }
                        else {
                            LOG_D("Start timer failed:%d",re);
                        }
                    }
                }
            }

            if (event_recv == PAUSE_EVT_FLG)
            {
                pause_window();
//                mcu_dp_enum_update(DPID_CONTROL, 2);
                if (ctrl_timer != RT_NULL){
                    re = rt_timer_delete(ctrl_timer);
                    if (re == RT_EOK){
                        LOG_D("Delete timer success...");
                    }
                    else {
                        LOG_D("Delete timer failed:%d",re);
                    }
                }
            }

            if (event_recv == RESUME_EVT_FLG)
            {
//                mcu_dp_enum_update(DPID_CONTROL, 3);
                if (window_state == paused_opening)
                {
                    open_window();
                    ctrl_timer = rt_timer_create("timer_win_ctrl", stop_operating_cb,//回调函数timeout2
                                                 RT_NULL,  ctrl_time_for_open,
                                                 RT_TIMER_FLAG_ONE_SHOT);//执行一次回调函数
                    if (ctrl_timer != RT_NULL){
                        re = rt_timer_start(ctrl_timer);
                        if (re == RT_EOK){
                            LOG_D("Start timer success...");
                        }
                        else {
                            LOG_D("Start timer failed:%d",re);
                        }
                    }
                }

                if (window_state == paused_closing )
                {
                    close_window();
                    ctrl_timer = rt_timer_create("timer_win_ctrl", stop_operating_cb,//回调函数timeout2
                                                 RT_NULL,  ctrl_time_for_close,
                                                 RT_TIMER_FLAG_ONE_SHOT);//执行一次回调函数
                    if (ctrl_timer != RT_NULL){
                        re = rt_timer_start(ctrl_timer);
                        if (re == RT_EOK){
                            LOG_D("Start timer success...");
                        }
                        else {
                            LOG_D("Start timer failed:%d",re);
                        }
                    }
                }
            }

        }
    }


}

int app_window_control_init()
{
    rt_err_t result = RT_EOK;

    rt_pin_mode(OPEN_PIN, PIN_MODE_INPUT);
    rt_pin_attach_irq(OPEN_PIN, PIN_IRQ_MODE_FALLING, open_button_cb, RT_NULL);
    rt_pin_irq_enable(OPEN_PIN, PIN_IRQ_ENABLE);

    rt_pin_mode(CLOSE_PIN, PIN_MODE_INPUT);
    rt_pin_attach_irq(CLOSE_PIN, PIN_IRQ_MODE_FALLING, close_button_cb, RT_NULL);
    rt_pin_irq_enable(CLOSE_PIN, PIN_IRQ_ENABLE);

//    rt_pin_mode(WIFI_PIN, PIN_MODE_INPUT);
//    rt_pin_attach_irq(WIFI_PIN, PIN_IRQ_MODE_FALLING, wifi_pin_cb, RT_NULL);
//    rt_pin_irq_enable(WIFI_PIN, PIN_IRQ_ENABLE);

    rt_pin_mode(IN1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(IN2_PIN, PIN_MODE_OUTPUT);


   window_control_tid = rt_thread_create("window_control_thread", window_control_thread_entry, RT_NULL, 4096*2, 15, 10);
   if (window_control_tid != RT_NULL)
   {
           rt_thread_startup(window_control_tid);
   }
   else
   {
       result = RT_ERROR;
   }




   return result;
}

INIT_APP_EXPORT(app_window_control_init);
