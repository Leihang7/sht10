/*
 * File      : sht10_app.c
 * This file is part of RT-Thread RTOS
 * COPYRIGHT (C) 2006 - 2018, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-04-19     misonyo      the first version
 */
#include <rthw.h>
#include <rtthread.h>
#include "sht10.h"

static char recording_state = 0;

void sht10_thread_entry(void *parameter)
{
    SHT10_Config();
    while(1)
    {
        if(recording_state)
        Air_Measure();
        rt_thread_delay(rt_tick_from_millisecond(1000));  
    }
}

rt_err_t app_init(void)
{ 
    rt_thread_t tid;
    
    tid = rt_thread_create("sht10", 
                            sht10_thread_entry,
                            RT_NULL,
                            2048,
                            24,
                            20);
    if (tid == RT_NULL)
    {
        return -RT_ERROR;
    }
    
    rt_thread_startup(tid);
    
    return RT_EOK;
}

static void recording_sensor_data(int argc, char **argv)
{
    char *start_state = "start";
    char *stop_state = "stop";
    char *state;
    
    state = argv[1]; 
    if(argc < 2)
    {
        rt_kprintf("Usage: recording_sensor_data \n");
        rt_kprintf("Like: recording_sensor_data start/stop \n");
        return ;
    }
    
    if(strcmp(start_state, state) == 0)
    {
        recording_state = 1;
    }
    else if(strcmp(stop_state, state) == 0)
    {
        recording_state = 0;
    }
}
/* 导出到 msh 命令列表中*/
MSH_CMD_EXPORT(recording_sensor_data, Start or Stop recording sensor data);