/*
 * Copyright (c) 2006-2020, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-02-16     simonliu       the first version
 */



/*

 * oled.h

 *

 *  Created on: Jun 19, 2020

 *      Author: guolunlun

 */


#ifndef __OLED_H__
#define __OLED_H__

//#include "main.h"

#include "stdlib.h"
#include <rtthread.h>
#include "board.h"
#include "app_event.h"


//OLED控制用函数
void OLED_Refresh(void);
void OLED_Init(void);
void OLED_ShowChar(rt_uint8_t x,rt_uint8_t y,rt_uint8_t chr,rt_uint8_t SIZE,rt_uint8_t Is_Reverse);
void OLED_ShowString(rt_uint8_t x,rt_uint8_t y,rt_uint8_t* str,rt_uint8_t SIZE);
void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t no);

#endif /* _OLED_H_ */



