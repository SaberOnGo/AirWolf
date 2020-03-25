
#include "GlobalDef.h"
#include "Application.h"
#include "os_timer.h"
#include "TimerManager.h"
#include "gizwits_port.h"
#include "key_drv.h"
#include "board_version.h"
#include "TFT_API.h"
#include "SnsGUI.h"
#include "os_global.h"

extern void Sensor_SetHCHOCaliBase(void);

int  main(void)
{
      //uint16_t key_result;
       //uint32_t pressed_time;
       //uint32_t key_pwr_time = 0;
       
       AppInit();
       while(1)
	{
		if(flag10ms)
             {
                    flag10ms = 0;
                    key_result = key_read();
		      key_process(key_result);
		      key_pwr_process();
              }
              
              
              if(flag_switch)
              {
                    OS_TimerCheck();	
              }
              else
              {
#if GIZWITS_TYPE
                     gizwits_user_task();
#endif

              }

              if(KEY0_INPUT== 0  && KEY1_INPUT == 0)  
              {
                      App_StartCO2BackgndCali();
                      Sensor_SetHCHOCaliBase();
                      while(!KEY_INPUT);
              }
	}
}
