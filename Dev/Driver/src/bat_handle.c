
#include "bat_handle.h"
#include "bat_adc.h"
#include "board_version.h"
#include "tft_graph.h"
#include "FONT_API.h"
#include "SnsGUI.h"
#include "icon_battery.h"
#include "os_global.h"


extern E_CHG_STAT BatChargStat(void);

//显示1格电量
void DispOneGridPower(u16 color)
{
	LCD_fillRect(BAT_X_OFFSET, BAT_Y_OFFSET,  32, 16, BLACK);
	LCD_fillRect(BAT_X_OFFSET + 24, BAT_Y_OFFSET,  32, 16,  color);
}

//显示2格电量
void DispTwoGridPower(u16 color)
{
       LCD_fillRect(BAT_X_OFFSET, BAT_Y_OFFSET,  32, 16, BLACK);
	LCD_fillRect(BAT_X_OFFSET + 16, BAT_Y_OFFSET,  32, 16,  color);
}

//显示3格电量
void DispThreeGridPower(u16 color)
{
	LCD_fillRect(BAT_X_OFFSET, BAT_Y_OFFSET,  32, 16, BLACK);
	LCD_fillRect(BAT_X_OFFSET + 8, BAT_Y_OFFSET,  32, 16,  color);
}

//显示4格电量
void DispFourGridPower(u16 color)
{
        LCD_fillRect(BAT_X_OFFSET, BAT_Y_OFFSET,  32, 16,  color);
}

#include "TFT_API.h"
// power shut off when low battery capacity
void PowerShutOff(void)
{
          UserGUI_LCDClear(C_BLACK);
          UG_FontSelect(&FONT_10X16);  
          UG_SetBackcolor(C_BLACK);
          UG_SetForecolor(C_WHITE);
          UG_PutString(125, 100,       "Closing");
          UG_PutString(60,      130,       "Low Battery Capacity");
          delay_ms(2000);
         // LCD_BackLight_Ctrl_Set(SW_CLOSE);
          Power_Keep_Close();
          NVIC_SystemReset();//电源键按下，系统复位关机
}

//显示电量
void DispBatPower(void)
{
	       u16 BatVal;
		u8  BatGrid;
		u16 DispColor;
	
		BatVal = BatADC_GetVal();
		
		if(BatChargStat() >= CHG_ON)  // 充电中，改善充电时电池浮压
		{
			if(BatVal   <= 380)
				BatVal   -= 10;
			if((BatVal <= 400) && (BatVal > 380))
				BatVal   -= 7;
			if((BatVal <= 405) && (BatVal > 400))
				BatVal   -= 4;
		}
		
		if(BatVal>=395)
			BatGrid = 4;
		else if(BatVal >= 383)
			BatGrid = 3;
		else if(BatVal >= 370)
			BatGrid = 2;
		else if(BatVal >= 358)
			BatGrid = 1;
		else
			BatGrid = 0;
		
		if(BatChargStat() >= CHG_ON) //充电中
				DispColor = GREEN; // 绿色
		else
				DispColor = WHITE;
		
		switch(BatGrid)
		{
			case 0:
			{
					if(BatChargStat() < CHG_ON)  //非充电，电量不足关机
					{
					         if(os_get_sec() > 30)
					         {      
								os_printf("low battery,ready to shut down\r\n");
								PowerShutOff();   // shut off system
					         }
					}
					break;
			}
			case 1:DispOneGridPower(DispColor);break;
			case 2:DispTwoGridPower(DispColor);break;
			case 3:DispThreeGridPower(DispColor);break;
			case 4:DispFourGridPower(DispColor);break;
		}
}



void DisplayBatteryPower(void)
{
        #if 1
        u8 is_charging = (BatChargStat() >= CHG_ON ?  1 : 0);
        u8 percent;
        u16 ad;
        
        ad = BatADC_GetVal() * 10;
        percent = BatLev_GetPercent();
        os_printf("percent = %d, chg = %d, %d.%03d V\r\n",  
                                 percent,  is_charging,  ad / 1000,  ad % 1000);
        os_printf("usb lev = %d\r\n",  (!USB_Detect_Read()));
        if(percent < 10 && (! USB_Detect_Read() || !is_charging ))   
         //if(percent < 50 && (! USB_Detect_Read()))
        {
               os_printf("bat percent = %d, system power off \r\n",  percent);
               PowerShutOff();   // shut off system
        }
        ICON_SetBatPercent(percent,  is_charging,  0);
        #endif
}

