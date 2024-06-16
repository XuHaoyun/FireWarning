#include "sys.h"
#include "led.h"
#include "usart.h"	  
#include "delay.h"
#include "oled.h"
#include <string.h>
#include <stdio.h>
#include "usart2.h"

// �������ݵĻ�����
char DATA[32],PHONE[32];
char WIFICon[256],UserName[256],IP[256],SubTopic[256],PubTopic[1024];
extern int LED_STATUS,FAN_STATUS;
char up[2],down[2];
char *USARTx_RX_BUF;

// ���ݴ洢
extern int temp_up,temp_down;
extern int hum_up,hum_down;
extern int sun_up,sun_down;
extern int co2_up,co2_down;
extern int handleFlag;

extern int initFlag;            // esp8266��ʼ��״̬
//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 


 
#if EN_USART1_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	  
void delay(int time){
	while( time ){
		delay_ms(1);
		time--;
	}
}  

void uart_init(u32 bound){
  //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��
  
	//USART1_TX   GPIOA.9
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
  GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.9
   
  //USART1_RX	  GPIOA.10��ʼ��
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;//PA10
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
  GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA.10  

  //Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
   //USART ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ


  USART_Init(USART1, &USART_InitStructure); //��ʼ������1
  USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�������ڽ����ж�
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//�������ڽ����ж�
	
  USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ���1 

}

void USART1_IRQHandler(void){
	u8 res;
	char *res1,*res2,*res3,*res4,*res5,*res6,*res7,*res8;

	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET){
		res = USART_ReceiveData(USART1);	
		if( USART_RX_STA < 400 ){
			TIM_SetCounter(TIM3,0);                   // ��������� 
			if(USART_RX_STA == 0 && !initFlag) 				// ʹ�ܶ�ʱ��3���ж� ��ʼ����Ϻ���Ҫ������ʱ��
			{
				TIM_Cmd(TIM3,ENABLE);                   // ʹ�ܶ�ʱ��3
			}
			USART_RX_BUF[USART_RX_STA++] = res;	      // ��¼���յ���ֵ	 
		}  		 
   } 
	
	 if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET){
		 // ��ʼ����ɺ����ִ������Ĳ���
		 if( initFlag ){  
		   USARTx_RX_BUF = strstr((const char * )USART_RX_BUF,(const char * )"{\"");
		    
		   res1 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"LED\":1");
		   res2 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"LED\":0");
		   res3 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"FAN\":1");
		   res4 = strstr((const char * )USARTx_RX_BUF,(const char * )"\"FAN\":0");
		   res5 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_TEMP");
		   res6 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_HUM");
		   res7 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_SUN");
		   res8 = strstr((const char * )USARTx_RX_BUF,(const char * )"E_SMOG");
		   
		   if( res1 ) handleFlag = 1; 
		   if( res2 ) handleFlag = 2;
			 if( res3 ) handleFlag = 3; 
		   if( res4 ) handleFlag = 4; 			 
		   
		   if( res5 ) handleFlag = 5;  // {"E_TEMP":1,"temp_up":20,"temp_down":20}		
		   if( res6 ) handleFlag = 6;  // {"E_HUM":1,"hum_up":20,"hum_down":20}		
		   if( res7 ) handleFlag = 7;	 // {"E_SUN":1,"sun_up":20,"sun_down":20}		
		   if( res8 ) handleFlag = 8;  // {"E_CO2":1,"res_up":20,"res_down":20}

			 
			 USART_RX_STA = 0;
		 }
		 		
		USART_ClearFlag(USART1, USART_FLAG_IDLE); // ��������жϱ�־λ
		USART1->SR; // ��ȡSR�Ĵ���
		USART1->DR; // ��ȡDR�Ĵ���
	 }	 
	 
} 
#endif	

// ������ڻ���
void clearCache( void ){
	int i;
	for( i=0;i<200;i++ ){
	 USART_RX_BUF[i] = 0;
	}
}


int waitDataStatus(char*atData,char *data,int waitTime){
	char *res;
	USART_RX_STA=0;

	printf(atData);
	while(--waitTime)
	{
		delay_ms(1);
		if(USART_RX_STA & 0X8000)//���յ�һ������
		{
			USART_RX_STA = 0;
			res = strstr((const char * )USART_RX_BUF,(const char * )data);
			if(res) return 1;	
		}
	}
	return 0;
}

void ESP8266Init(char* WIFIname,char* WIFIpwd,char* iP,char *clientID,char* userName,char* password,char* subTopic){
	
	//��ʾ��ʼ������
	OLED_ShowString(50,42,"25%",16,1);OLED_Refresh();
	//��λESP8266
	while(!waitDataStatus("AT+RST\r\n","OK",5000));
	delay_ms(500);
	
	//����ESP8266ΪStationģʽ
	while(!waitDataStatus("AT+CWMODE=1\r\n","OK",5000));
	delay_ms(500);
	OLED_ShowString(50,42,"50%",16,1);OLED_Refresh();
	
	//���ӵ�WiFi
	sprintf(WIFICon,"AT+CWJAP=\"%s\",\"%s\"\r\n",WIFIname,WIFIpwd);
	while(!waitDataStatus(WIFICon,"OK",5000));
	delay_ms(3000);
	
	//����MQTT�û���������
	OLED_ShowString(50,42,"75%",16,1);OLED_Refresh();
	sprintf(UserName,"AT+MQTTUSERCFG=0,1,\"%s\",\"%s\",\"%s\",0,0,\"\"\r\n",clientID,userName,password);
	while(!waitDataStatus(UserName,"OK",5000));
	delay_ms(2000);
	
	sprintf(IP,"AT+MQTTCONN=0,\"%s\",1883,0\r\n",iP);
	while(!waitDataStatus(IP,"OK",15000));
	delay(1000);

	//���ӵ�MQTT������
	OLED_ShowString(46,42,"100%",16,1);OLED_Refresh();
	sprintf(SubTopic,"AT+MQTTSUB=0,\"%s\",0\r\n",subTopic);
	while(!waitDataStatus(SubTopic,"OK",5000));
	
	//����MQTT����
	memset(USART_RX_BUF,0,sizeof(USART_RX_BUF));
	USART_RX_STA = 0; initFlag = 1;   // ��ʼ�����
}

void ESP8266Pub(char *pubTopic,int temperature,int humidity,int Illumination,int smogHumidity){
	// sprintf(PubTopic,"AT+MQTTPUB=0,\"%s\",\"t-%.2f-h-%.2f-l-%d-c-%d\",0,0\r\n",pubTopic,temp,humidity,llumination,co2);
	sprintf(PubTopic,"AT+MQTTPUB=0,\"%s\",\"{\\\"Temperature\\\":%d\\,\\\"Humidity\\\":%d\\,\\\"Illumination\\\":%d\\,\\\"smog\\\":%d}\",1,0\r\n",pubTopic,temperature,humidity,Illumination,smogHumidity);
	// while(!waitDataStatus(PubTopic,"OK",1000));
	printf(PubTopic);
}







