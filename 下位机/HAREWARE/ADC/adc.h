#ifndef __ADC_H
#define __ADC_H

#include "stm32f10x.h"

void Adc_Init( void );
int getSmogValue( void );
int getFlameValue( void );

#endif

