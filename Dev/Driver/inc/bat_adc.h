
#ifndef __BAT_ADC_H__
#define __BAT_ADC_H__

#include "GlobalDef.h"

u16   BatADC_GetVal(void);
void BatADC_Init(void);
uint8_t BatLev_GetPercent(void);

#endif


