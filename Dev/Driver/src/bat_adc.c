
#include "bat_adc.h"
#include "os_global.h"
#include "board_version.h"
#include "delay.h"

#if BAT_DEBUG_EN
#define BAT_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
//#define  BAT_DEBUG   dbg_print_detail
#else
#define BAT_DEBUG(...)
#endif


// 电池电量对应的电量百分比
typedef struct
{
    uint8_t  percent;  // 剩余电量百分比, 如: 15 表示: 15%
	uint16_t volt;    // 电压: 如 3345 表示: 3.345V, 即单位: mV
}T_BAT_LEVEL_MAP;

static const T_BAT_LEVEL_MAP BatLevMap[] = 
{
     {2,  3300},   // 0%, 3.300 V: 表示: <= 3.300 V时, 电池电量为 2%
     {5,  3380},
     {10, 3450},   // 10 %, 3.45 V,
     {25, 3500},   // 25 %, 3.50 V
     {30, 3550},   // 30 %, 3.55 V
     {40, 3600},   // 40 %, 3.60 V
     {50, 3650},   // 50 %, 3.65 V
     {60, 3700},   // 60 %, 3.70 V
     {70, 3800},    // 70 %,
     {80, 3850},   // 80 %, 3.85 V
     {85, 3900},
     {90, 3950},   // 90 %, 3.95 V
     {95, 4100},  
     {98, 4130},
     {100, 4150}, // 100 %, >= 4.15 V
};

#define BAT_LEVELS   (sizeof(BatLevMap) / sizeof(BatLevMap[0]))



uint8_t bat_lev_percent = 0;  // 电池电量

uint8_t BatLev_GetPercent(void)
{
      return bat_lev_percent;
}

// 将电池电压转换成百分比
//static 
// 参数: uint16_t bat_volt, 电池电压, 单位: mV
void BatLev_VoltToPercent(uint16_t bat_volt)
{
      uint8_t i;

     if(bat_volt <= BatLevMap[0].volt){ bat_lev_percent  = 0; }
     else if(bat_volt >= BatLevMap[BAT_LEVELS - 1].volt )
    { 
            bat_lev_percent  = 100; 
     }
     else
    {
          for(i = 1; i < BAT_LEVELS; i++)
         {
                if(BatLevMap[i - 1].volt <= bat_volt && bat_volt < BatLevMap[i].volt)
                {
                   bat_lev_percent = BatLevMap[i - 1].percent;
                   //BAT_DEBUG("bat lev = %d, bat_volt = %d.%03d V \r\n", i, bat_volt / 1000, bat_volt % 1000);
      			 break;
                }
          }
     }
      BAT_DEBUG("bat_volt = %d.%03d V, bat=%02d, %d%, t = %ld\n", bat_volt / 1000, bat_volt % 1000, 
   	           bat_lev_percent, os_get_tick());
      BAT_DEBUG("bat percent = %d \n", bat_lev_percent);  
}


/************************************
获取bat ADC的转换值

************************************/
u16 BatADC_GetVal(void)
{
	 u16 AD_Value;
	 u8  AD_i;
	 u32 AD_temp = 0;
        //double volt;
        
	 for(AD_i = 0; AD_i < 5; AD_i++)
	 {	 	
			STM32_ADC_SoftwareStartConvCmd(BAT_ADC_x, ENABLE);    /* Start ADC1 Software Conversion */ 
			while(! (READ_REG_32_BIT(BAT_ADC_x->SR, ADC_FLAG_EOC)));  // 等待转换结束
			AD_temp	+= (BAT_ADC_x->DR&0XFFF);
	 }
	 //volt =((uint32_t)( (AD_temp  / 5.0) * 3300) ) >> 12;   // 测得的电阻电压
	 
	 //os_printf("ad = %ld, Vr = %ld\r\n",   AD_temp,  (uint32_t)volt);
	 //volt = volt * 635 / 470;
	 //os_printf("bat v = %ld \r\n", (uint32_t)volt);
	 AD_Value =(((AD_temp/5)*330*635 /470)>>12);//获取电池电压值*100,

	 BatLev_VoltToPercent(AD_Value * 10);
	 
	 return AD_Value;
}




/*****************************
ADC1的0(PC0)

单次转换

*****************************/
void BatADC_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	STM32_RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC| RCC_APB2Periph_BAT_ADC, ENABLE );	  //使能ADC1通道时钟

       // PCLK2 = 48MHz, ADC时钟需要分频
       // ADCCLK = PCLK2 / 6 = 72 MHz / 6 = 12M
	STM32_RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //设置ADC分频因子6, 72 M /6 = 12, ADC最大时间不能超过14M
	
	GPIO_InitStructure.GPIO_Pin = BAT_ADC_Pin;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//模拟输入引脚

        STM32_GPIO_Init(BAT_ADC_PORT, &GPIO_InitStructure);	
	STM32_ADC_DeInit_ADC1();
	
	STM32_ADC_Init(BAT_ADC_x, 
		             ADC_Mode_Independent,        //ADC工作模式:ADC1和ADC2工作在独立模式
		             DISABLE,   	                //模数转换工作在单通道模式
		             DISABLE,	                    //模数转换工作在单次转换模式
		             ADC_ExternalTrigConv_None, 	//转换由软件而不是外部触发启动
		             ADC_DataAlign_Right,        //ADC数据右对齐
		             1);                            //顺序进行规则转换的ADC通道的数目

        /* ADC1 regular channel configuration */ 
       STM32_ADC_RegularChannelConfig(BAT_ADC_x,  BAT_ADC_Channel, 1, ADC_SampleTime_55Cycles5);

	STM32_ADC_Cmd(BAT_ADC_x, ENABLE);	//使能指定的ADCx
	delay_us(30);
	
	STM32_ADC_ResetCalibration(BAT_ADC_x);	//使能复位校准  
	 
	while(STM32_ADC_GetResetCalibrationStatus(BAT_ADC_x));	//等待复位校准结束
	
	STM32_ADC_StartCalibration(BAT_ADC_x);	 //开启AD校准
 
	while(STM32_ADC_GetCalibrationStatus(BAT_ADC_x));	 //等待校准结束

	//STM32_NVICInit(ADC1_2_IRQn,NVIC_GROUP,1,1);	//设置ADC1中断优先级

}

