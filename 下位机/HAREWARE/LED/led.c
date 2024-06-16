#include "led.h"
#include "stm32f10x.h"

void LED_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//GPIOA
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,ENABLE);//GPIOB

	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);    
	
	// 初始化四个按钮
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	
	// 初始化继电器
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
	GPIO_ResetBits(GPIOB, GPIO_Pin_3);
	
	// 初始化蜂鸣器和继电器
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	GPIO_ResetBits(GPIOA, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6);
}

int key_int(char *data,int len){
	int i,num = 0,sum[2];
	
	for(i=0;i<len;i++){
		switch(data[i]){
			case '0':
				sum[i] = 0;
				break;
			case '1':
				sum[i] = 1;
				break;	
			case '2':
				sum[i] = 2;
				break;
			case '3':
				sum[i] = 3;
				break;
			case '4':
				sum[i] = 4;
				break;	
			case '5':
				sum[i] = 5;
				break;			
			case '6':
				sum[i] = 6;
				break;
			case '7':
				sum[i] = 7;
				break;	
			case '8':
				sum[i] = 8;
				break;
			case '9':
				sum[i] = 9;
				break;
			default:
				sum[i] = 1;
				break;
		}
		num = sum[0]*10 + sum[1];
	}
	return num;
}
