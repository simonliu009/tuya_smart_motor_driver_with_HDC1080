/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-28     simonliu       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "app_event.h"
#include "mcusdk/wifi.h"

#define LOG_TAG     "app_gpio.c"
#define LOG_LVL     LOG_LVL_ERROR

#include <ulog.h>


#define LED_PIN GET_PIN(I, 8)
#define WIFI_PIN GET_PIN(H,9)

unsigned char app_wifi_work_state;

/*
 *
 * @brief  MCU主动获取当前wifi工作状态
 * @param  Null
 * @return wifi work state
 * -          SMART_CONFIG_STATE: smartconfig配置状态
 * -          AP_STATE: AP配置状态
 * -          WIFI_NOT_CONNECTED: WIFI配置成功但未连上路由器
 * -          WIFI_CONNECTED: WIFI配置成功且连上路由器
 * -          WIFI_CONN_CLOUD: WIFI已经连接上云服务器
 * -          WIFI_LOW_POWER: WIFI处于低功耗模式
 * -          SMART_AND_AP_STATE: WIFI smartconfig&AP 模式
 * @note   如果为模块自处理模式,MCU无须调用该函数
 * unsigned char mcu_get_wifi_work_state(void)
    {
        return wifi_work_state;
    }
 */

void wifi_pin_cb()
{
    app_wifi_work_state = mcu_get_wifi_work_state();
    if (app_wifi_work_state == SMART_CONFIG_STATE)
    {
        mcu_set_wifi_mode(AP_CONFIG);
        LOG_D("Set wifi to AP_CONFIG mode...");
    }
    else {
        mcu_set_wifi_mode(SMART_CONFIG);
        LOG_D("Set wifi to SMART_CONFIG mode...");
    }

}

void wifi_state_thread_entry()
{
    static rt_uint32_t delay = 8000;
    rt_uint32_t cnt = 0;
    LOG_D("WIFI State thread started...");
    while(1)
    {
        app_wifi_work_state = mcu_get_wifi_work_state();
        if (app_wifi_work_state == WIFI_CONNECTED || app_wifi_work_state == WIFI_CONN_CLOUD)
        {
            if (cnt%10==0)
            {
                LOG_D("wifi work state is WIFI_CONNECTED or WIFI_CONN_CLOUD");
            }
            rt_pin_write(LED_PIN, PIN_LOW);//Turn ON LED
            rt_thread_mdelay(200);
            cnt++;
            continue;
        }
        else if (app_wifi_work_state == WIFI_NOT_CONNECTED || app_wifi_work_state == WIFI_LOW_POWER)
        {
            if (cnt%60==0)
            {
                LOG_D("wifi work state is WIFI_NOT_CONNECTED or WIFI_LOW_POWER");
            }
            rt_pin_write(LED_PIN, PIN_HIGH);//Turn OFF LED
            rt_thread_mdelay(200);
            cnt++;
            continue;
        }
        else if (app_wifi_work_state == SMART_CONFIG_STATE)
        {
            if (cnt%60==0)
            {
                LOG_D("wifi work state is SMART_CONFIG_STATE");
            }
            delay = 200;
        }
        else if (app_wifi_work_state == AP_STATE)
        {
            if (cnt%60==0)
            {
                LOG_D("wifi work state is AP_STATE");
            }
            delay = 500;
        }
        else{
            LOG_D("wifi work state is unknown:%d",wifi_work_state);
        }
        rt_thread_mdelay(delay);
        rt_pin_write(LED_PIN, PIN_HIGH);
        rt_thread_mdelay(delay);
        rt_pin_write(LED_PIN, PIN_LOW);
        cnt++;

    }
}

int app_gpio_init()
{
    rt_err_t result = RT_EOK;

    rt_pin_mode(LED_PIN, PIN_MODE_OUTPUT);

    rt_pin_mode(WIFI_PIN, PIN_MODE_INPUT);
    rt_pin_attach_irq(WIFI_PIN, PIN_IRQ_MODE_FALLING, wifi_pin_cb, RT_NULL);
    rt_pin_irq_enable(WIFI_PIN, PIN_IRQ_ENABLE);


   rt_thread_t wifi_state_tid = rt_thread_create("wifi_state_thread", wifi_state_thread_entry, RT_NULL, 512, 15, 10);
   if (wifi_state_tid != RT_NULL)
   {
           rt_thread_startup(wifi_state_tid);
   }
   else
   {
       result = RT_ERROR;
   }

   return result;
}


INIT_APP_EXPORT(app_gpio_init);
