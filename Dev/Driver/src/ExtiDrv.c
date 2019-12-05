
#include "ExtiDrv.h"
#include "os_global.h"
#include "os_timer.h"
#include "board_version.h"
//#include "BatteryLevel.h"
//#include "PowerCtrl.h"
#include "FatFs_Demo.h"
#include "Application.h"
#include "RTCDrv.h"
#include "ADC_Drv.h"
#include "SnsGUI.h"
#include "cfg_variable.h"


#if EXTI_DEBUG_EN 
#define EXT_DEBUG(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define EXT_DEBUG(...)
#endif

uint8_t battery_is_charging = 0; // 电池是否正在充电, 高4位表示: USB是否插入; 低 4位表示: 电池是否在充电


static os_timer_t tTimerCapKeyTrigger;
static os_timer_t tTimerCapKeyTotal;

static volatile uint16_t vibration_cnt = 0;  // 震动次数
static os_timer_t tTimerTFTDispCntDown;
static uint16_t  tft_left_disp_sec; // TFT 屏幕剩余显示时间
static uint16_t sns_left_sec = 0;  // 传感器剩余运行时间


//static volatile uint16_t chg_stat_ttl = 3;  // 检测充电管脚的上升下降沿的生存时间
//static E_CHG_STAT last_chg_stat = CHG_NONE; // 上一次的充电状态

// 电池充电状态
E_CHG_STAT BatChargStat(void)
{
      if(USB_Detect_Read())   // 高电平
      {
                if(CHG_STAT_Read())return CHG_DONE;
                else return CHG_ON;
      }
       else
            return CHG_NONE;
}

extern void DisplayBatteryPower(void);

static void TimerTFTDispCntDown_CallBack(void * arg)
{
         if(tft_left_disp_sec)
         {
               SnsGUI_DisplayTime(&calendar);
               tft_left_disp_sec--;
               if(tft_left_disp_sec == 0)
               {
                        // 关闭 TFT 屏幕
                        TFT_Ctrl(SW_CLOSE);
                        // 默认 5 min 后关闭传感器, 可在 config.txt 文件修改此值
                        sns_left_sec  = cfgVar_SnsRemainRunTime;  
                        EXT_DEBUG("close tft power, sec = %d,  sns_left = %d sec \r\n", 
                                                       os_get_tick() / 100,   sns_left_sec);
                        SET_REG_32_BIT(EXTI->IMR, EXTI_Line_CAPKEY);  // 使能外部中断
               }
               //DbgInfo_FreeDisk();
         }
         else
         {
                if(sns_left_sec < 0xFFFF)
                {
                      sns_left_sec--;
                      if(sns_left_sec == 0)
                      {
                             EXT_DEBUG("close sns power, sec = %d \r\n",  os_get_tick() / 100);
                             SNS_Ctrl_Set(SW_CLOSE);
                      }
                }
         }

         /*
         if(chg_stat_ttl)chg_stat_ttl--;

         do
         {
                 E_CHG_STAT cur_chg_stat = BatChargStat();
                 
                 if(last_chg_stat != cur_chg_stat)
                 {
                         EXT_DEBUG("chg stat changed: last = %d, cur = %d \r\n",  last_chg_stat, cur_chg_stat);
                         last_chg_stat = cur_chg_stat;
                 }
         }while(0);
         */
         DisplayBatteryPower();
         os_timer_arm(&tTimerTFTDispCntDown,   SEC(1),   0);
}

#include "TFT_API.h"
#include "QDTFT_Demo.h"
#include "TFT_Demo.h"
#include "UGUI_Demo.h"
#include "SnsGUI.h"



static void TimerCapKeyTrigger_CallBack(void * arg)
{
      //EXT_DEBUG("capkey cnt = %d \r\n", vibration_cnt);
      if(vibration_cnt > 30)
      {
            if(tft_left_disp_sec == 0)
            {
                    if(!SNS_Power_Is_Open())
                    {
                          #if 0 // tofix: 硬件问题: 重新打开传感器电源会导致重启
                          SNS_Ctrl_Set(SW_OPEN);
                          LCD_Ctrl_Set(SW_OPEN);
                          UserGUI_Init();
                          SnsGUI_DisplayNormal();
                          ADCDrv_DrawBatCapacity(1);
                          #else
                          JumpToBootloader();
                          #endif
                    }
                    LCD_BackLight_Ctrl_Set(SW_OPEN);
                    
            }
            tft_left_disp_sec = cfgVar_LcdBackLightSec;  // TFT 屏幕背光持续时间
           // EXT_DEBUG("set tft disp sec = %d sec \r\n", tft_left_disp_sec);
      }
}

static void TimerCapKeyTotal_CallBack(void * arg)
{
         vibration_cnt = 0;
}




void CapKey_Init(void)
{
       EXTI_InitTypeDef   EXTI_InitStructure;
       GPIO_InitTypeDef   GPIO_InitStructure;


	 os_timer_setfn(&tTimerCapKeyTrigger,      TimerCapKeyTrigger_CallBack,       NULL);
	 os_timer_setfn(&tTimerCapKeyTotal,           TimerCapKeyTotal_CallBack,             NULL);
        os_timer_setfn(&tTimerTFTDispCntDown, TimerTFTDispCntDown_CallBack,  NULL);
	 os_timer_arm(&tTimerTFTDispCntDown,   SEC(1),   0);

	 tft_left_disp_sec = cfgVar_FirstLcdBackLightSec;
	 
	 // 使能 IO 时钟
	 // add code here
	 
	 GPIO_InitStructure.GPIO_Pin  = CAPKEY_Pin;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	 STM32_GPIO_Init(CAPKEY_PORT, &GPIO_InitStructure);
	 
	 /* Enable AFIO clock */
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
   
	 /* Connect EXTIn 外部中断线到 IO 管脚  */
	 GPIO_EXTILineConfig(CAPKEY_PortSource ,  CAPKEY_PinSource );
        STM32_EXTI_ClearITPendingBit(EXTI_Line_CAPKEY);  // 清除中断标志位

	 
	 /* Configure EXTIx line */
	 EXTI_InitStructure.EXTI_Line = EXTI_Line_CAPKEY;
	 EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	 EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;  
	 EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	 EXTI_Init(&EXTI_InitStructure);
   
	 /* Enable and set EXTI0 Interrupt to the lowest priority */
	 STM32_NVICInit(EXTI_CAPKEY_IRQn, 4,  6, 0);	 // 第4组优先级, 4位抢占优先级, 0 位响应优先级
}

// 充电管脚中断初始化
void CHG_STAT_ExtiInit(void)
{
       EXTI_InitTypeDef   EXTI_InitStructure;
       GPIO_InitTypeDef   GPIO_InitStructure;

	 GPIO_InitStructure.GPIO_Pin  = CHG_STAT_Pin;
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;

	 STM32_GPIO_Init(CHG_STAT_PORT, &GPIO_InitStructure);
	 
	 /* Enable AFIO clock */
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
   
	 /* Connect EXTIn 外部中断线到 IO 管脚  */
	 GPIO_EXTILineConfig(CHG_STAT_PortSource ,  CHG_STAT_PinSource );
        STM32_EXTI_ClearITPendingBit(EXTI_Line_CHG_STAT);  // 清除中断标志位

	 
	 /* Configure EXTIx line */
	 EXTI_InitStructure.EXTI_Line = EXTI_Line_CHG_STAT;
	 EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	 EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;  
	 EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	 EXTI_Init(&EXTI_InitStructure);
   
	 /* Enable and set EXTI0 Interrupt to the lowest priority */
	 STM32_NVICInit(EXTI_CAPKEY_IRQn, 4,  6, 0);	 // 第4组优先级, 4位抢占优先级, 0 位响应优先级
}

void ExtiDrv_Init(void)
{
       if(! App_GetRunMode())
       {
              CapKey_Init();         //  振动检测 中断初始化
        }
        //CHG_STAT_ExtiInit();
}


void EXTI_CAPKEY_IRQHandler(void)
{

    CLEAR_REG_32_BIT(EXTI->IMR, EXTI_Line_CAPKEY);  // 禁止外部中断
    if(READ_REG_32_BIT(EXTI->PR, EXTI_Line_CAPKEY))  // 读取管脚的中断标志
    {
		EXTI->PR = EXTI_Line_CAPKEY;   // 往挂起位写 1 清中断标志
		vibration_cnt++;
		if(vibration_cnt > 30)
		     os_timer_arm(&tTimerCapKeyTrigger,  2, 0);  // 延时 10 ms  
              if(vibration_cnt > 30 )
              {
                    os_timer_arm(&tTimerCapKeyTotal,  SEC(3),  0);
              }

                       
    }
    SET_REG_32_BIT(EXTI->IMR, EXTI_Line_CAPKEY);
}

/*
void EXTI_CHG_STAT_IRQHandler(void)
{
     CLEAR_REG_32_BIT(EXTI->IMR, EXTI_Line_CHG_STAT);  // 禁止外部中断
    if(READ_REG_32_BIT(EXTI->PR, EXTI_Line_CHG_STAT))  // 读取管脚的中断挂起位
    {
		EXTI->PR = EXTI_Line_CHG_STAT;   // 往挂起位写 1 清中断标志
		chg_stat_ttl = 3;                    
    }
    SET_REG_32_BIT(EXTI->IMR, EXTI_Line_CHG_STAT);
}
*/

