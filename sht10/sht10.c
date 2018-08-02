/*************************************************************
                             \(^o^)/
  Copyright (C), 2013-2020, ZheJiang University of Technology
  File name  : SHT10.c 
  Author     : ziye334    
  Version    : V1.0 
  Data       : 2014/3/10      
  Description: Digital temperature and humidity sensor driver code
  
*************************************************************/
#include "sht10.h"
#include <math.h>
#include <stdint.h>
#include <rtdevice.h>

/* �鿴drv_gpio.c��pin_index pins[]��ȡͨ��GPIO���Ŷ�Ӧ�ı�� */
#define SHT10_DATA_PIN  147     /* ��ӦPD5 */
#define SHT10_SCK_PIN   150     /* ��ӦPD6 */

#define SHT10_DATA_H()  rt_pin_write(SHT10_DATA_PIN,PIN_HIGH)
#define SHT10_DATA_L()  rt_pin_write(SHT10_DATA_PIN,PIN_LOW)
#define SHT10_DATA_R()  rt_pin_read(SHT10_DATA_PIN)

#define SHT10_SCK_H()   rt_pin_write(SHT10_SCK_PIN,PIN_HIGH)
#define SHT10_SCK_L()   rt_pin_write(SHT10_SCK_PIN,PIN_LOW)

/* ��������غ궨�� */
#define     noACK        	0
#define 	ACK             1
                                       //addr      command        r/w
#define STATUS_REG_W       0x06        //000         0011          0          д״̬�Ĵ���
#define STATUS_REG_R       0x07        //000         0011          1          ��״̬�Ĵ���
#define MEASURE_TEMP       0x03        //000         0001          1          �����¶�
#define MEASURE_HUMI       0x05        //000         0010          1          ����ʪ��
#define SOFTRESET          0x1E        //000         1111          0          ��λ

static u16 humi_val, temp_val;
static u8 err, checksum;
static float humi_val_real, temp_val_real;

//�˴������õĳ������ѹ�Ĳ�ͬ���仯
//���ѯ����Ŀ¼�е�doc�ĵ�
const float d1 = -39.7;
const float d2 = +0.01;
const float C1 = -2.0468;
const float C2 = +0.0367;
const float C3 = -0.0000015955;        
const float T1 = +0.01;
const float T2 = +0.00008;

extern void SHT10_ConReset(void);

/*************************************************************
  Function   ��SHT10_Dly  
  Description��SHT10ʱ����Ҫ����ʱ
  Input      : none        
  return     : none    
*************************************************************/
void SHT10_Dly(void)
{
	u16 i;
	for(i = 1000; i > 0; i--);
}

/*************************************************************
  Function   ��SHT10_Config  
  Description����ʼ�� SHT10����
  Input      : none        
  return     : none    
*************************************************************/
void SHT10_Config(void)
{
	rt_pin_mode(SHT10_DATA_PIN,PIN_MODE_OUTPUT);
	rt_pin_mode(SHT10_SCK_PIN, PIN_MODE_OUTPUT);
	
	SHT10_ConReset();        //��λͨѶ
}


/*************************************************************
  Function   ��SHT10_DATAOut
  Description������DATA����Ϊ���
  Input      : none        
  return     : none    
*************************************************************/
void SHT10_DATAOut(void)
{ 
	rt_pin_mode(SHT10_DATA_PIN,PIN_MODE_OUTPUT);
}


/*************************************************************
  Function   ��SHT10_DATAIn  
  Description������DATA����Ϊ����
  Input      : none        
  return     : none    
*************************************************************/
void SHT10_DATAIn(void)
{
	rt_pin_mode(SHT10_DATA_PIN,PIN_MODE_INPUT);
}


/*************************************************************
  Function   ��SHT10_WriteByte  
  Description��д1�ֽ�
  Input      : value:Ҫд����ֽ�        
  return     : err: 0-��ȷ  1-����    
*************************************************************/
u8 SHT10_WriteByte(u8 value)
{
	u8 i, err = 0;
	
	SHT10_DATAOut();           //����DATA������Ϊ���	
	SHT10_Dly();
	for(i=0x80; i>0; i>>=1)  	//д1���ֽ�
	{
		if(i & value)
			SHT10_DATA_H();
		else
			SHT10_DATA_L();
		SHT10_Dly();
		SHT10_SCK_H();
		SHT10_Dly();
		SHT10_SCK_L();
		SHT10_Dly();
	}
	SHT10_DATAIn();            //����DATA������Ϊ����,�ͷ�DATA��
	SHT10_Dly();
	SHT10_SCK_H();
	SHT10_Dly();
	err = SHT10_DATA_R();      //��ȡSHT10��Ӧ��λ
	SHT10_Dly();
	SHT10_SCK_L();
	
	return err;
}

/*************************************************************
  Function   ��SHT10_ReadByte  
  Description����1�ֽ�����
  Input      : Ack: 0-��Ӧ��  1-Ӧ��        
  return     : err: 0-��ȷ 1-����    
*************************************************************/
u8 SHT10_ReadByte(u8 Ack)
{
	u8 i, val = 0;
	
	SHT10_DATAIn();          //����DATA������Ϊ����
	SHT10_Dly();
	for(i=0x80; i>0; i>>=1)  //��ȡ1�ֽڵ�����
	{
		SHT10_Dly();
		SHT10_SCK_H();
		SHT10_Dly();
		if(SHT10_DATA_R())	 //��������ƽΪ��
			val = (val | i);
		SHT10_Dly();
		SHT10_SCK_L();
	}
	SHT10_Dly();
	SHT10_DATAOut();         //����DATA������Ϊ���
	SHT10_Dly();
	if(Ack)
	   SHT10_DATA_L();       //Ӧ��������ȥ������ȥ������(У������)
	else
	   SHT10_DATA_H();       //��Ӧ���������˽���
	SHT10_Dly();
	SHT10_SCK_H();
	SHT10_Dly();
	SHT10_SCK_L();
	SHT10_Dly();
	
	return val;              //���ض�����ֵ
}


/*************************************************************
  Function   ��SHT10_TransStart  
  Description����ʼ�����źţ�ʱ�����£�
                     _____         ________
               DATA:      |_______|
                         ___     ___
               SCK : ___|   |___|   |______        
  Input      : none        
  return     : none    
*************************************************************/
void SHT10_TransStart(void)
{
	SHT10_DATAOut();                          //����DATA������Ϊ���
	SHT10_Dly();
	SHT10_DATA_H();
	SHT10_Dly();
	SHT10_SCK_L();
	SHT10_Dly();
	SHT10_SCK_H();
	SHT10_Dly();
	SHT10_DATA_L();
	SHT10_Dly();
	SHT10_SCK_L();
	SHT10_Dly();
	SHT10_SCK_H();
	SHT10_Dly();
	SHT10_DATA_H();
	SHT10_Dly();
	SHT10_SCK_L();

}

/*************************************************************
  Function   ��SHT10_ConReset  
  Description��ͨѶ��λ��ʱ�����£�
                     _____________________________________________________         ________
               DATA:                                                      |_______|
                        _    _    _    _    _    _    _    _    _        ___     ___
               SCK : __| |__| |__| |__| |__| |__| |__| |__| |__| |______|   |___|   |______
  Input      : none        
  return     : none    
*************************************************************/
void SHT10_ConReset(void)
{
	u8 i;
	
	SHT10_DATAOut();
	SHT10_Dly();
	SHT10_DATA_H();
	SHT10_Dly();
	SHT10_SCK_L();	
	SHT10_Dly();
	for(i = 0; i < 9; i++)                  //����SCKʱ��9c��
	{
		SHT10_SCK_H();
		SHT10_Dly();
		SHT10_SCK_L();
		SHT10_Dly();
	}
	SHT10_TransStart();                     //��������
}



/*************************************************************
  Function   ��SHT10_SoftReset  
  Description����λ
  Input      : none        
  return     : err: 0-��ȷ  1-����    
*************************************************************/
u8 SHT10_SoftReset(void)
{
	u8 err = 0;
	
	SHT10_ConReset();                   //ͨѶ��λ
	err += SHT10_WriteByte(SOFTRESET);  //дRESET��λ����	
	return err;
}


/*************************************************************
  Function   ��SHT10_ReadStatusReg  
  Description����״̬�Ĵ���
  Input      : p_value-���������ݣ�p_checksun-������У������       
  return     : err: 0-��ȷ  0-����    
*************************************************************/
u8 SHT10_ReadStatusReg(u8 *p_value, u8 *p_checksum)
{
	u8 err = 0;
	
	SHT10_TransStart();                         //��ʼ����
	err = SHT10_WriteByte(STATUS_REG_R);		//дSTATUS_REG_R��ȡ״̬�Ĵ�������
	*p_value = SHT10_ReadByte(ACK);        	    //��ȡ״̬����
	*p_checksum = SHT10_ReadByte(noACK);		//��ȡ���������	
	return err;
}



/*************************************************************
  Function   ��SHT10_WriteStatusReg  
  Description��д״̬�Ĵ���
  Input      : p_value-Ҫд�������ֵ       
  return     : err: 0-��ȷ  1-����    
*************************************************************/
u8 SHT10_WriteStatusReg(u8 *p_value)
{
	u8 err = 0;
	
	SHT10_TransStart();                         //��ʼ����
	err += SHT10_WriteByte(STATUS_REG_W);		//дSTATUS_REG_Wд״̬�Ĵ�������
	err += SHT10_WriteByte(*p_value);           //д������ֵ	
	return err;
}



/*************************************************************
  Function   ��SHT10_Measure  
  Description������ʪ�ȴ�������ȡ��ʪ��
  Input      : p_value-������ֵ��p_checksum-������У����        
  return     : err: 0-��ȷ 1������    
*************************************************************/
u8 SHT10_Measure(u16 *p_value, u8 *p_checksum, u8 mode)
{
	u8 err=0, value_H=0, value_L=0;
	u32 i;
	
	SHT10_TransStart();                         	//��ʼ����
	switch(mode)                                                         
	{
	case TEMP:                                  	//�����¶�
		err += SHT10_WriteByte(MEASURE_TEMP);		//дMEASURE_TEMP�����¶�����
		break;
	case HUMI:
		err += SHT10_WriteByte(MEASURE_HUMI);		//дMEASURE_HUMI����ʪ������
		break;
	default:
	   break;
	}
	SHT10_DATAIn();
	for(i = 0; i < 24000000; i++)               	//�ȴ�DATA�źű�����
	{
	   if(SHT10_DATA_R() == 0) break;       		//��⵽DATA�������ˣ�����ѭ��
	}
	if(SHT10_DATA_R() == 1)                     	//����ȴ���ʱ��
	   err += 1;
	value_H = SHT10_ReadByte(ACK);
	value_L = SHT10_ReadByte(ACK);
	*p_checksum = SHT10_ReadByte(noACK);        	//��ȡУ������
	*p_value = ((u16)value_H << 8) | value_L;
	return err;
}


/*************************************************************
  Function   ��SHT10_Calculate  
  Description��������ʪ�ȵ�ֵ
  Input      : Temp-�Ӵ������������¶�ֵ��Humi-�Ӵ�����������ʪ��ֵ
               p_humidity-�������ʵ�ʵ�ʪ��ֵ��p_temperature-�������ʵ���¶�ֵ        
  return     : none    
*************************************************************/
void SHT10_Calculate(u16 t, u16 rh, float *p_temperature, float *p_humidity)
{	
	float RH_Lin;             //RH����ֵ        
	float RH_Ture;            //RH��ʵֵ
	float temp_C;
	
	temp_C = d1 + d2 * t;                                     //�����¶�ֵ        
	RH_Lin = C1 + C2 * rh + C3 * rh * rh;                     //����ʪ��ֵ
	RH_Ture = (temp_C -25) * (T1 + T2 * rh) + RH_Lin;         //ʪ�ȵ��¶Ȳ���������ʵ�ʵ�ʪ��ֵ
	
	if(RH_Ture > 100)                                         //����ʪ��ֵ����
		RH_Ture = 100;
	if(RH_Ture < 0.1)
		RH_Ture = 0.1;                                        //����ʪ��ֵ����
	
	if(temp_C > 120)
		temp_C = 120;
	if(temp_C < -39.9)
		temp_C = -39.9;
	
	*p_humidity = RH_Ture;
	*p_temperature = temp_C;
}


/*************************************************************
  Function   ��SHT10_CalcuDewPoint  
  Description������¶��
  Input      : h-ʵ�ʵ�ʪ�ȣ�t-ʵ�ʵ��¶�        
  return     : dew_point-¶��    
*************************************************************/
float SHT10_CalcuDewPoint(float t, float h)
{
	float logEx, dew_point;
	
	logEx = 0.66077 + 7.5 * t / (237.3 + t) + (log10(h) - 2);
	dew_point = ((0.66077 - logEx) * 237.3) / (logEx - 8.16077);
	
	return dew_point; 
}

void Air_Measure(void)
{	
    int fd;
    char *new_line="\n";
    static char input_data[50]={"     Temp=  ,     Humi=  \n"};
    static int input_length = 0;
    int ret;
    struct stat buf;
    time_t now;
    char *time_now;
    
    now = time(RT_NULL);
    time_now = ctime(&now);
	
    err += SHT10_Measure(&temp_val, &checksum, TEMP);                  //��ȡ�¶Ȳ���ֵ
    err += SHT10_Measure(&humi_val, &checksum, HUMI);                  //��ȡʪ�Ȳ���ֵ
    if(err != 0)
    {
       SHT10_ConReset();
    }
    else
    {
        SHT10_Calculate(temp_val, humi_val, &temp_val_real, &humi_val_real);     //����ʵ�ʵ���ʪ��ֵ
        //dew_point = SHT10_CalcuDewPoint(temp_val_real, humi_val_real);         //����¶���¶�

        input_data[10] = (int)(temp_val_real)/10 + 48;
        input_data[11] = (int)(temp_val_real)%10 + 48;
        input_data[23] = (int)(humi_val_real)/10 + 48;
        input_data[24] = (int)(humi_val_real)%10 + 48;
        
        fd = open("/spi/sensor_data.txt", O_WRONLY | O_CREAT | O_APPEND );
        if (fd >= 0)
        {
            /*write data*/	
            write(fd, time_now, 24);
            write(fd, input_data, 25);
            write(fd, new_line, 1);
            /*close file*/  
            close(fd);
        } 
        else
        { 
            rt_kprintf("open /spi/sensor_data.txt failed!\n");
        }
       
      // rt_kprintf("Temp=%d,     Humi=%d\n", (int)(temp_val_real), (int)(humi_val_real));
    } 			   
}

