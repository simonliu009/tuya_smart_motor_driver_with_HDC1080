/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-09-02     RT-Thread    first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "mcusdk/wifi.h"

#include "mcusdk/mcu_api.h"

#define LOG_TAG     "main.c"
#define LOG_LVL     LOG_LVL_DBG
#include <ulog.h>



extern void app_uart1_init();
extern void wifi_protocol_init(void);
extern void wifi_uart_service(void);

static void rtc_thread_entry()
{

    int delay = 60;
    int cnt = 0;
    LOG_D("rtc thread started.");
//    rt_thread_mdelay(3000);
    mcu_get_system_time();
    rt_thread_mdelay(2000);
    int year = print_time();
    while(1)
    {

        if(cnt%120 == 0 || year < 120)
        {
            mcu_get_system_time();
            year = print_time();
            delay = 1;
         }
        else {
            delay = 60;
        }

//        LOG_D("Delay %d seconds for weather and rtc sync.",delay);
        rt_thread_mdelay(delay*1000);
        cnt++;
    }
}

static void wifi_uart_service_thread_entry(){
    while(1)
    {
        wifi_uart_service();
    }

}




int main(void)
{

    LOG_D("run app_uart1_init()");
    app_uart1_init();
    LOG_D("run wifi_protocol_init()");
    wifi_protocol_init();
//    app_hdc1080_init();
    rt_thread_t rtc_tid = rt_thread_create("rtc_thread", rtc_thread_entry, RT_NULL, 2048, 25, 10);
    if (rtc_tid != RT_NULL)
    {
        rt_thread_startup(rtc_tid);
    }

    rt_thread_t wifi_uart_service_tid = rt_thread_create("wifi_uart_service_thread", wifi_uart_service_thread_entry, RT_NULL, 4096*2, 25, 10);
    if (wifi_uart_service_tid != RT_NULL)
    {
        rt_thread_startup(wifi_uart_service_tid);
    }

    return RT_EOK;
}

#include "stm32h7xx.h"
static int vtor_config(void)
{
    /* Vector Table Relocation in Internal QSPI_FLASH */
    SCB->VTOR = QSPI_BASE;
    return 0;
}
INIT_BOARD_EXPORT(vtor_config);


