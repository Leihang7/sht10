/*************************************************************
                             \(^o^)/
  Copyright (C), 2013-2020, ZheJiang University of Technology
  File name  : SHT10.h 
  Author     : ziye334    
  Version    : V1.0 
  Data       : 2014/3/10      
  Description: Digital temperature and humidity sensor driver code
  
*************************************************************/

#ifndef __SHT10_H__
#define __SHT10_H__
#include <rtthread.h>

enum {TEMP, HUMI};

typedef rt_uint32_t u32;
typedef rt_uint16_t u16;
typedef rt_uint8_t  u8;

void SHT10_Config(void);
void SHT10_ConReset(void);
u8 SHT10_SoftReset(void);
u8 SHT10_Measure(u16 *p_value, u8 *p_checksum, u8 mode);
void SHT10_Calculate(u16 t, u16 rh,float *p_temperature, float *p_humidity);
float SHT10_CalcuDewPoint(float t, float h);
void Air_Measure(void);
#endif
