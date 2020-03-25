
#include "board_version.h"
#include "os_timer.h"

/**************************************************************************************
主板管脚定义源文件
主板硬件版本:  AirDog V1.33 版本, 2019-06-14
日期: 2019-06-14

**************************************************************************************/

static void PowerIO_Ctrl(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pinx, E_SW_STATE sta)
{
       GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin   = GPIO_Pinx;
       GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;

	if(SW_OPEN == sta)
	{
	       GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
		STM32_GPIO_Init(GPIOx, &GPIO_InitStructure);
		GPIOx->BRR = GPIO_Pinx;  // 低电平导通
	}
	else // 关闭: 输出高电平
	{
	      GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
	      STM32_GPIO_Init(GPIOx, &GPIO_InitStructure);
	}
}

#include "delay.h"
static E_SW_STATE sns_sta = SW_CLOSE;
void SNS_Ctrl_Set(E_SW_STATE sta)
{
        sns_sta = sta;
        if(sta == SW_CLOSE)
        {
                SNS_Power_Ctrl_H(); 
                delay_us(100);
                SNS_Power_Ctrl_L(); 
        }
        else
        {
                 SNS_Power_Ctrl_L();
                 delay_us(100);
                 SNS_Power_Ctrl_H(); 
        }
}

E_SW_STATE SNS_Ctrl_Get(void)
{
      return sns_sta;
}

void LCD_Ctrl_Set(E_SW_STATE sta)
{
	
}
void LCD_BackLight_Ctrl_Set(E_SW_STATE sta)
{
	PowerIO_Ctrl(LCD_BackLight_Ctrl_PORT, LCD_BackLight_Ctrl_Pin , sta);
}

void TFT_Ctrl(E_SW_STATE sta)
{
       LCD_BackLight_Ctrl_Set(sta);
}

os_timer_t tTimerStartGPRS;
void TimerStartGPRS_CallBack(void * arg)
{
           IO_H(GPRS_PWR_KEY_PORT,  GPRS_PWR_KEY_Pin);
}

// 选择网络模块
void NetModuelSelect(E_NET_SEL  sel)
{
        GPIO_InitTypeDef GPIO_InitStructure;
                                                                       
         //WIFI INT 输出, 选择网络模块
        GPIO_InitStructure.GPIO_Pin = WIFI_Power_Ctrl_Pin;
        GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
        GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
        STM32_GPIO_Init(WIFI_Power_Ctrl_PORT, &GPIO_InitStructure);

        GPIO_InitStructure.GPIO_Pin = GPRS_PWR_KEY_Pin;
        STM32_GPIO_Init(GPRS_PWR_KEY_PORT, &GPIO_InitStructure);
        IO_L(GPRS_PWR_KEY_PORT,  GPRS_PWR_KEY_Pin);   
        
	 if(NET_SEL_WIFI == sel)   // 选择WIFI模块
	 {
                WIFI_Power_Ctrl_Open();    
	 }
	 else  if(NET_SEL_GPRS == sel)  // 选择GPRS模块
	 {
               IO_L(GPRS_PWR_KEY_PORT,  GPRS_PWR_KEY_Pin);      
               WIFI_Power_Ctrl_Close();
               os_timer_setfn(&tTimerStartGPRS,  TimerStartGPRS_CallBack,  NULL);
               os_timer_arm(&tTimerStartGPRS,   SEC(2),  0);      
	 }
	 else
	 {
                 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
                  STM32_GPIO_Init(WIFI_Power_Ctrl_PORT, &GPIO_InitStructure);
	 }
}



#include "delay.h"
// 设置数据口为输入或输出
// 板级管脚初始化
void Board_GpioInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

      // 时钟使能
      STM32_RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);


	// 电源保持管脚
	GPIO_InitStructure.GPIO_Pin   = Power_Keep_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
       GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_PP;
	STM32_GPIO_Init(Power_Keep_PORT, &GPIO_InitStructure);
	
	// SENSOR_Power Ctrl
	GPIO_InitStructure.GPIO_Pin   = SNS_Power_Ctrl_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
       GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_PP;
	STM32_GPIO_Init(SNS_Power_Ctrl_PORT, &GPIO_InitStructure);

        // 作为普通IO
	GPIO_InitStructure.GPIO_Pin   =  GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
       GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_PP;
	STM32_GPIO_Init(GPIOC, &GPIO_InitStructure);
	

       // WIFI / GPRS 选择
	NetModuelSelect(NET_SEL_WIFI);
       //delay_us(10);
       
  
    
       // VBUS_Ctrl, USB 数据通信控制IO
	GPIO_InitStructure.GPIO_Pin         = VBUS_Ctrl_Pin;
       GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
       GPIO_InitStructure.GPIO_Mode   = GPIO_Mode_Out_PP;
	STM32_GPIO_Init(VBUS_Ctrl_PORT, &GPIO_InitStructure);
       VBUS_Ctrl_Open();

        // OPEN TFT back light
       LCD_BackLight_Ctrl_Set(SW_OPEN);
       
	// BEEP IO 管脚初始化
       GPIO_InitStructure.GPIO_Pin   = BEEP_IO_Pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
       GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
	STM32_GPIO_Init(BEEP_IO_PORT, &GPIO_InitStructure);
	BEEP_IO_Close();

	 // 充电状态检测管脚
	 GPIO_InitStructure.GPIO_Pin      =  CHG_STAT_Pin;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	 STM32_GPIO_Init(CHG_STAT_PORT, &GPIO_InitStructure);

        // usb detect 管脚
        GPIO_InitStructure.GPIO_Pin      =  USB_Detect_Pin;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
	 STM32_GPIO_Init(USB_Detect_PORT, &GPIO_InitStructure);
        
        // 电源按键
	 GPIO_InitStructure.GPIO_Pin      =  KeyDect_Pin;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	 STM32_GPIO_Init(KeyDect_Port, &GPIO_InitStructure);
}

void Board_OpenSensor(void)
{
       LCD_BackLight_Ctrl_Set(SW_OPEN);
       SNS_Ctrl_Set(SW_OPEN);  
}
void BEEP(uint16_t freq)
{
        u32 i;
        for(i = 0; i < freq; i++)
        {
                 BEEP_IO_Close();
                 delay_ms(100);
                 BEEP_IO_Open();
                 delay_ms(100);
        }
        
}

E_NET_SEL NetModuleRead(void) 
{
       if(WIFI_Power_En_Status())   // IO = high level -> sel WIFI
       {
             return NET_SEL_WIFI;
       }
       else
           return NET_SEL_GPRS;
}



