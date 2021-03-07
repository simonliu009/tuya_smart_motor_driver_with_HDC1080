/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-28     simonliu       the first version
 */



/*
 * File: /Users/simonliu/Documents/ART-Pi推窗器/hdc1080.c
 * Project: /Users/simonliu/Documents/ART-Pi推窗器
 * Created Date: 2021-02-28 18:59:14
 * Author: Simon Liu
 * -----
 * Last Modified: 2021-02-28 23:21:37
 * Modified By: Simon Liu
 * -----
 * Copyright (c) 2021 SimonLiu Inc.
 *
 * May the force be with you.
 * -----
 * HISTORY:
 * Date         By  Comments
 * ----------   --- ----------------------------------------------------------
 */



#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drv_common.h>
#include "board.h"
#include "app_event.h"
#include "mcusdk/mcu_api.h"

#define LOG_TAG     "HDC1080"
#define LOG_LVL     LOG_LVL_INFO
#include <ulog.h>


#define IIC_WRITE_BIT 0
#define IIC_READ_BIT 1


#define HDC1080_SENSOR_ADDR                 0x40
#define HDC1080_TEMPERATURE_ADDR            0x00
#define HDC1080_HUMIDITY_ADDR               0x01
#define HDC1080_CONFIGURATION_ADDR          0x02

#define DPID_INDOOR_TEMP 23
#define DPID_INDOOR_HUMI 101

rt_uint8_t reg_add = 0x00;
uint8_t hdc1080_init_data[]={0x90,0x00};

static void hdc1080_init()
{

    HAL_StatusTypeDef ret = HAL_I2C_Mem_Write(&hi2c4, HDC1080_SENSOR_ADDR << 1 | IIC_WRITE_BIT, HDC1080_CONFIGURATION_ADDR, I2C_MEMADD_SIZE_8BIT, hdc1080_init_data, 2, HAL_MAX_DELAY);
    if (ret != HAL_OK)  {
    LOG_E("HC1080 init error");             //I2C 故障处理
    HAL_I2C_DeInit(&hi2c4); // 释放 IO 口为 GPIO ，复位句柄状态标志
    HAL_I2C_Init(&hi2c4); // 这句重新初始化 I2C 控制器
    }
    else {
        LOG_D("HC1080 init ok");
    }
}

static void hdc1080_read_temp_humi()
{
    rt_uint8_t buff[4] = {0};
    HAL_StatusTypeDef ret;

    ret = HAL_I2C_Master_Transmit(&hi2c4, HDC1080_SENSOR_ADDR << 1 | IIC_WRITE_BIT, &reg_add,1,HAL_MAX_DELAY);


    if (ret != HAL_OK)  { //I2C 故障处理
    HAL_I2C_DeInit(&hi2c4); // 释放 IO 口为 GPIO ，复位句柄状态标志
    HAL_I2C_Init(&hi2c4); // 这句重新初始化 I2C 控制器
    }
    rt_thread_mdelay(200);
    ret = HAL_I2C_Master_Receive(&hi2c4,HDC1080_SENSOR_ADDR << 1 | IIC_READ_BIT, buff, 4, 1000);
    if (ret != HAL_OK)  { //I2C 故障处理
    HAL_I2C_DeInit(&hi2c4); // 释放 IO 口为 GPIO ，复位句柄状态标志
    HAL_I2C_Init(&hi2c4); // 这句重新初始化 I2C 控制器
    }

    float temp = ((float)(buff[0]<<8|buff[1])/65536.00)*165.00 - 40;
    float humi = ((float)(buff[2]<<8|buff[3])/65536.00) * 100;
//    LOG_D("temp=%.2f",temp);
//    LOG_D("humi=%.2f",humi);
    rt_enter_critical();
    curr_property.IndoorTemperature = roundf(temp);
    curr_property.IndoorHumidity = roundf(humi);
    rt_exit_critical();
}

static void hdc1080_thread_entry()
{
    rt_uint32_t delay = 10;
    LOG_D("HDC1080 Thread Started...");
    while(1)
    {
        hdc1080_read_temp_humi();

        rt_thread_mdelay(delay*1000);
        LOG_D("Update Indoor Temperature:%d", curr_property.IndoorTemperature);
        LOG_D("Update Indoor Humidity:%d", curr_property.IndoorHumidity);
        mcu_dp_value_update(DPID_INDOOR_TEMP,curr_property.IndoorTemperature);
        mcu_dp_value_update(DPID_INDOOR_HUMI,curr_property.IndoorHumidity);
    }
}

int app_hdc1080_init()
{
    rt_err_t result = RT_EOK;
    rt_thread_mdelay(3000);

    hdc1080_init();

    rt_thread_t hdc1080_tid = rt_thread_create("hdc1080_thread", hdc1080_thread_entry, RT_NULL, 4096, 15, 10);
    if (hdc1080_tid != RT_NULL)
    {
        LOG_D("Start hdc1080 thread");
        rt_thread_startup(hdc1080_tid);
   }
   else
   {
       LOG_E("Failed to start hdc1080 thread");
       result = RT_ERROR;
   }

   return result;
}


INIT_APP_EXPORT(app_hdc1080_init);
