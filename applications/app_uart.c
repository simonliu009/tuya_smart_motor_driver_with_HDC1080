/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-25     simonliu       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "drv_common.h"
#include "app_event.h"


#define LOG_TAG     "APP_UART"
#define LOG_LVL     LOG_LVL_INFO
#include <ulog.h>

#define APP_UART_NAME       "uart1"    /* 串口设备名称 */
rt_device_t serial;                /* 串口设备句柄 */
struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;  /* 初始化配置参数 */
/* 用于接收消息的信号量 */
static struct rt_semaphore rx_sem;
//char buffer[256] = {0};

/* 接收数据回调函数 */
static rt_err_t uart_recv_callback(rt_device_t dev, rt_size_t size)
{
    /* 串口接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

//static void wifi_uart_service_thread_entry(void *parameter)
//{
//    wifi_uart_service();
//}

static void serial_thread_entry(void *parameter)
{
    unsigned char buffer[300];

    int i = 0;
    int j = 0;

    unsigned char ch;
    while (1)
    {

        if (rt_device_read(serial, -1, &ch, 1) == 1)
        {
            uart_receive_input(ch);

            //print recieved data
//            buffer[i++]=ch;
//            if(ch == 0x55)
//            {
//                rt_kprintf("uart buffer:\r\n");
//            }

        }
        else
        {

//            for (int j = 0; j < i ; j++)
//            {
//                rt_kprintf("%02x",buffer[j]);
//            }
//            rt_kprintf("\r\n");
//            memset(buffer,0,sizeof(buffer));
//            i = 0;
            /* 阻塞等待接收信号量，等到信号量后再次读取数据 */
            rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        }
    }
}

void tuya_weather_thread_entry()
{
    LOG_D("Start tuya weather thread...");
    rt_uint16_t delay = 15*60; //Check Weather every 15 minutes.
    mcu_open_weather();
    rt_thread_mdelay(5*1000);
    while(1)
    {
        LOG_D("Request Weather...");
        request_weather_serve();
        rt_thread_mdelay(2000);
        if (current_screen == clock_screen) ShowWeatherInfo();
        rt_thread_mdelay(delay*1000);
    }


}

int app_uart1_init(){
    rt_uint32_t ret = RT_EOK;
    /* step1：查找串口设备 */
    serial = rt_device_find(APP_UART_NAME);
    if (!serial)
        {
            LOG_E("find %s failed!\n", APP_UART_NAME);
            return RT_ERROR;
        }
//    LOG_D("Find serial port success:%s", APP_UART_NAME);
    /* step2：修改串口配置参数 */
    config.baud_rate = BAUD_RATE_9600;        //修改波特率为 9600
    config.data_bits = DATA_BITS_8;           //数据位 8
    config.stop_bits = STOP_BITS_1;           //停止位 1
    config.bufsz     = 1024;                   //修改缓冲区 buff size 为 128
    config.parity    = PARITY_NONE;           //无奇偶校验位

    /* step3：控制串口设备。通过控制接口传入命令控制字，与控制参数 */
//    rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config);
    if(RT_EOK != rt_device_control(serial, RT_DEVICE_CTRL_CONFIG, &config))
        {
            LOG_E("Failed to config serial port:%s", APP_UART_NAME);
            return RT_ERROR;
        }
//    LOG_D("Config serial port success:%s", APP_UART_NAME);
    /* 初始化信号量 */
       rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);
    /* step4：打开串口设备。以中断接收及轮询发送模式打开串口设备 */
//    rt_device_open(serial, RT_DEVICE_FLAG_INT_RX);
        if(RT_EOK != rt_device_open(serial, RT_DEVICE_FLAG_INT_RX))
        {
            LOG_E("Failed to open serial port:%s", APP_UART_NAME);
            return RT_ERROR;
        }
  //    LOG_D("Open serial port success:%s", APP_UART_NAME);
    /* step5:设置接收回调函数 */
        rt_device_set_rx_indicate(serial, uart_recv_callback);

    /* 创建 serial 线程 */
        rt_thread_t uart1_thread = rt_thread_create("serial", serial_thread_entry, RT_NULL, 1024, 25, 10);
        /* 创建成功则启动线程 */
        if (uart1_thread != RT_NULL)
        {
            rt_thread_startup(uart1_thread);
        }
        else
        {
            ret = RT_ERROR;
        }

        rt_thread_t tuya_weather_tid = rt_thread_create("tuya_weather", tuya_weather_thread_entry, RT_NULL, 1024, 25, 10);
                /* 创建成功则启动线程 */
        if (tuya_weather_tid != RT_NULL)
        {
            rt_thread_startup(tuya_weather_tid);
        }
        else
        {
            ret = RT_ERROR;
        }
    return RT_EOK;
}



