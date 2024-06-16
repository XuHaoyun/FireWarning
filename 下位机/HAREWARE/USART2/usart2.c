#include "usart2.h"
#include "delay.h"
#include "stm32f10x.h"


u8 USART2_RX_BUF[200];     //���ջ���,���USART_REC_LEN���ֽ�.
u8 CO2_RX_BUF[200];
u16 USART2_RX_STA = 0;       //����״̬���	 


void uart2_init( u32 bound )
{
	/* GPIO�˿����� */
	GPIO_InitTypeDef	GPIO_InitStructure;
	USART_InitTypeDef	USART_InitStructure;
	NVIC_InitTypeDef	NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(  RCC_APB2Periph_GPIOA, ENABLE ); 
	RCC_APB1PeriphClockCmd( RCC_APB1Periph_USART2, ENABLE );         /* ʹ��USART2��GPIOAʱ�� */

	/* PA2 TXD2 */
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed	= GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_AF_PP;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	/* PA3 RXD2 */
	GPIO_InitStructure.GPIO_Pin	= GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode	= GPIO_Mode_IN_FLOATING;
	GPIO_Init( GPIOA, &GPIO_InitStructure );

	/* Usart1 NVIC ���� */
	NVIC_InitStructure.NVIC_IRQChannel			= USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority	= 3;               
	NVIC_InitStructure.NVIC_IRQChannelSubPriority		= 2;                     
	NVIC_InitStructure.NVIC_IRQChannelCmd			= ENABLE;                      
	NVIC_Init( &NVIC_InitStructure );                                          

	/* USART ��ʼ������ */
	USART_InitStructure.USART_BaudRate		= bound;                                
	USART_InitStructure.USART_WordLength		= USART_WordLength_8b;                  
	USART_InitStructure.USART_StopBits		= USART_StopBits_1;                    
	USART_InitStructure.USART_Parity		= USART_Parity_No;                     
	USART_InitStructure.USART_HardwareFlowControl	= USART_HardwareFlowControl_None;       
	USART_InitStructure.USART_Mode			= USART_Mode_Rx | USART_Mode_Tx;       

	USART_Init( USART2, &USART_InitStructure );                                             
	USART_ITConfig( USART2, USART_IT_RXNE, ENABLE );  
	USART_ITConfig( USART2, USART_IT_IDLE, ENABLE );         	
	USART_Cmd( USART2, ENABLE );                                                           
}


int Dat_10(int dat){ 
		
 return dat = dat/16*10+dat%16;
}

void USART_SendString(USART_TypeDef* USARTx, char *DataString)
{
	int i = 0;
	USART_ClearFlag(USARTx,USART_FLAG_TC);										//�����ַ�ǰ��ձ�־λ������ȱʧ�ַ����ĵ�һ���ַ���
	while(DataString[i] != '\0')												    //�ַ���������
	{
		USART_SendData(USARTx,DataString[i]);									//ÿ�η����ַ�����һ���ַ�
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC) == 0);					//�ȴ����ݷ��ͳɹ�
		USART_ClearFlag(USARTx,USART_FLAG_TC);									//�����ַ�����ձ�־λ
		i++;
	}
}


int getCo2Density( void ){
	int ic,i1,i2,percentage;
	
	i1 = USART2_RX_BUF[1];
	i2 = USART2_RX_BUF[2];
	ic = i1*256+i2;
	percentage = (int)(((double)ic/1000) * 100);
	
	return percentage;
}

void USART2_IRQHandler( void )                                          
{
	u8 res;
	if ( USART_GetITStatus( USART2, USART_IT_RXNE ) != RESET )     
	{
		if( USART2_RX_STA < 200){
			res = USART_ReceiveData( USART2 ); 
			USART2_RX_BUF[USART2_RX_STA] = res;
			USART2_RX_STA++;
		}
	}
	if( USART_GetITStatus( USART2, USART_IT_IDLE ) != RESET ){
		USART2_RX_STA = 0;
		USART_ClearFlag(USART2,USART_FLAG_IDLE);
		USART2->SR;
		USART2->DR;
	}
}

