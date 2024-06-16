#include "stm32f10x.h"

#include "sys.h"
#include "led.h"
#include "bmp.h"
#include "oled.h"
#include "adc.h"
#include "dth11.h"
#include "timer.h"
#include "delay.h"
#include "usart.h"
#include "GY302.h"
#include "usart2.h"
#include "cJSON.h"

int LED_STATUS = 0;
int FAN_STATUS = 0;

char WIFIName[] = "admin";
char WIFIpwd[] = "88888888";

char userName[] = "";
char password[] = "";
char clientId[] = "ending";
char ip[] = "broker.emqx.io";
char subTopic[] = "/monitorA/post";
char pubTopic[] = "/monitor51/post";

int temp_up = 20,temp_down = 20;  // �¶�������
int hum_up = 50,hum_down = 50;    // ʪ��������
int flame_up = 80,flame_down = 0;    // ����������
int smog_up = 40,smog_down = 0; // ����������

#define BUZZ  PAout(4)     // ������
#define FAN   PAout(5)     // ����
#define LED   PAout(6)     // LED��

#define KEY_EDIT PBin(4)   // ���ð�ť
#define KEY_NEXT PBin(12)  // �л���ť
#define KEY_SWIT PBin(13)  // ��һҳ��ť
#define KEY_ADD  PBin(14)  // ��һ��ť
#define KEY_DEC  PBin(15)  // ��һ��ť

// ��λ�����ݴ���
int handleFlag = 0;        // �ж���λ�����͵�������Ҫ������һ��
int sendFlag = 0;          // ��ʱ��ʱ�䵽�˸�λ��һ��ʾ��������
int initFlag = 0;          // ESP8266��ʼ���ɹ���־λ
int paramFlag = 1;         // �Ƿ����������

int flameIntensity = 0;     // ����ǿ�� 
int smog = 78;   // ����
unsigned char temp = 0,humi = 0;     // �¶�,ʪ��

void paramCheck( void );             // �������Ƿ񳬹�
void handleData( void );             // ��λ�����ݴ���
void DisplayUI( void );              // �̶�ҳ��UI��Ⱦ
void paramEdit( void );              // ��ֵ��������
void editUiDisplay( int pageIndex ); // ����ҳ���ʼ��
void runAlter(int cursor,int count); // ִ�в����޸�
        
extern char *USARTx_RX_BUF;         // �������ݻ���

int main(void)
{
	delay_init();
	LED_Init();
	OLED_Init();
	Adc_Init();
	uart_init(115200);   	     // esp8266���ڳ�ʼ��
	timeInit(4999,7199);       // 72M 0.1ms 500ms
	timeSendInit(14999,19999); // 72m 0.2ms 3s
	OLED_ShowChLength(38,16,47,3);
	ESP8266Init(WIFIName,WIFIpwd,ip,clientId,userName,password,subTopic); //  ESP8266������ƽ̨
	BH1750_Init();            // BH1750��ʼ��
	while(DHT11_Init());      // �ȴ�dth11��ʼ���ɹ�
	OLED_Clear();
	FAN = 0;
	
	while( 1 ){
		
		DisplayUI();
		DHT11_Read_Data(&temp,&humi);
		flameIntensity = getFlameValue();
		smog = getSmogValue();
		
		OLED_ShowNum(45,0,temp,2,16,1);
		OLED_ShowNum(45,16,humi,2,16,1);
		OLED_ShowNum(76,32,smog,2,16,1);
		OLED_ShowNum(76,48,flameIntensity,2,16,1);

		OLED_Refresh();
		// ��������ҳ��
		if (!KEY_EDIT) {
				while (!KEY_EDIT); // ��������
				paramEdit(); // �����������ҳ��
				if (!paramFlag) OLED_ShowChLength(105, 2, 62, 1); // ���������ʾ��ť�رգ�����ʾ��ť
				else OLED_ShowString(105, 2, "  ", 16, 1); // ���������ʾ��ť�򿪣�����հ�ť��ʾ����
		}
		
		// ����Ƿ��¡���һ������
		if (!KEY_NEXT) {
				while (!KEY_NEXT); // ��������
				paramFlag = !paramFlag; // �л������������õ�״̬
				if (!paramFlag) {
						// ���������ʾ��ť�رգ���FAN��LED��BUZZ��ֵ������Ϊ0
						FAN = 0;
						LED = 0;
						BUZZ = 0;
						OLED_ShowChLength(105, 2, 62, 1); // ��ʾ��ť
				}
				else {
						OLED_ShowString(105, 2, "  ", 16, 1); // ��հ�ť��ʾ����
				}
				OLED_Refresh(); // ˢ��OLED��ʾ��
		}

		// ������λ�����ĵ�����
		if ( handleFlag ) {
				handleData();
		}

		// ����Ƿ񵽴��ϴ����ݵ�ʱ��
		if (sendFlag) {
				ESP8266Pub(pubTopic, temp, humi, flameIntensity, smog); // ��������
				sendFlag = 0; // ��sendFlag��Ϊ0
				TIM_SetCounter(TIM4, 0); // ���¼�ʱ3��
		}

		// ��������Χ
		if (paramFlag) {
				paramCheck();
		}
		delay_ms(100); // ��ʱ100����
	}
}

// ���ƹ̶������UI����
void DisplayUI( void ){
	
	// ��ʾ����
	OLED_ShowChLength(0,0,11,2);
	OLED_ShowChLength(0,16,13,2);
	OLED_ShowChLength(0,32,67,4);
	OLED_ShowChLength(0,48,71,4);
	
	// ����ð��
	OLED_ShowString(33,0,":",16,1);
	OLED_ShowString(33,16,":",16,1);
	OLED_ShowString(65,32,":",16,1);
	OLED_ShowString(65,48,":",16,1);

	//�¶�
	OLED_ShowChLength(65,1,21,1);
	//ʪ��
	OLED_ShowString(65,16,"%RH",16,1);

	//����
	OLED_ShowString(100,32,"%",16,1);
	//����
	OLED_ShowString(93,48,"%LX",16,1);
}

// ������ֵ����
void handleData( void ){
	cJSON *data = NULL;
	cJSON *upData = NULL;
	cJSON *downData = NULL;
	
	data = cJSON_Parse( (const char *)USARTx_RX_BUF );
	
	switch( handleFlag ){
		case 1:
			LED = 1;LED_STATUS = 1;
			break;
		case 2:
			LED = 0;LED_STATUS = 0;		
			break;
		case 3:
			FAN = 1;FAN_STATUS = 1;
			break;
		case 4:
			FAN = 0;FAN_STATUS = 0;
			break;
		case 5:	
			upData = cJSON_GetObjectItem(data,"temp_up");
			downData = cJSON_GetObjectItem(data,"temp_down");
			temp_up = upData->valueint;
			temp_down = downData->valueint;
			break;
		case 6:
			upData = cJSON_GetObjectItem(data,"hum_up");
			downData = cJSON_GetObjectItem(data,"hum_down");
			hum_up = upData->valueint;
			hum_down = downData->valueint;
			break;
		case 7:
			upData = cJSON_GetObjectItem(data,"sun_up");
			downData = cJSON_GetObjectItem(data,"sun_down");
			flame_up = upData->valueint;
			flame_down = downData->valueint;
			break;
		case 8:
			upData = cJSON_GetObjectItem(data,"smog_up");
			downData = cJSON_GetObjectItem(data,"smog_down");
			smog_up = upData->valueint;
			smog_down = downData->valueint;
			break;
	}
	// USART_SendString(USART2,(char *)USARTx_RX_BUF);
	clearCache();
	cJSON_Delete(data);
	handleFlag = 0;
	USART_RX_STA = 0;
}

void paramEdit( void ){
	int pageIndex = 0,cursor = 0,alterTip = 0,beforeCursor = 0;
	// ��ʼ��UI
	OLED_Clear();
	OLED_ShowString(112,0,"<",16,1);
	
	while( 1 ){
		editUiDisplay(pageIndex); // UI��ʾ
		if( !KEY_NEXT ){
			while( !KEY_NEXT );
			beforeCursor = (cursor % 4)*16;              // �õ�֮ǰ���޸���ʾλ��
			cursor++;		                                  
			cursor = (cursor % 4) + 4 * pageIndex;   		 // �õ��ڼ�������
			alterTip = (cursor % 4) * 16;            		 // ��ʾ��ť����ʾ��
			OLED_ShowString(112,beforeCursor," ",16,1);  // ��ʾ��ʾ<
			OLED_ShowString(112,alterTip,"<",16,1);      // ��ʾ��ʾ< 
		}
		if( !KEY_SWIT ){                  // ����ҳ���л���ť
			while(!KEY_SWIT);
			pageIndex = (++pageIndex) % 2;  // �л�ҳ��
			cursor = 4 * pageIndex;         // �õ��������α� 
			OLED_Clear();
			OLED_ShowString(112,0,"<",16,1);// ������ʾ��ʾ��ť
			OLED_Refresh();			
		}
		if( !KEY_ADD ){
			while(!KEY_ADD);
			runAlter(cursor,1);
		}
		if( !KEY_DEC ){
			while(!KEY_DEC);
			runAlter(cursor,-1);
		}
 		if( !KEY_EDIT ){                  // �������ð�ť�Ƴ���ǰҳ��
			while( !KEY_EDIT );
			OLED_Clear();
			break;
		}
		handleData();
		OLED_Refresh(); 
	}
}

// ͨ��cursor����ȷ��������Ҫ����ֵ
void runAlter(int cursor,int count){
	int countResult;
	
	switch( cursor ){
		case 0:
			countResult = temp_up + count;
		  temp_up = countResult>0?countResult:0;
			break;
		case 1:
			countResult = temp_down + count;
		  temp_down = countResult>0 ? countResult : 0;
			break;
		case 2:
			countResult = hum_up + count;
		  hum_up = countResult>0 ? countResult : 0;
			break;
		case 3:
			countResult = hum_down + count;
		  hum_down = countResult>0 ? countResult : 0;
			break;
		case 4:
			countResult = flame_up + count;
		  flame_up = countResult>0 ? countResult : 0;
			break;
		case 5:
			countResult = flame_down + count;
		  flame_down = countResult>0 ? countResult : 0;
			break;
		case 6:
			countResult = smog_up + count;
		  smog_up = countResult>0 ? countResult : 0;
			break;
		case 7:
			countResult = smog_down + count;
		  smog_down = countResult>0 ? countResult : 0;
			break;	
	}
}


// ���ó�ʼ��UI
void editUiDisplay( int pageIndex ){
	// ����ǵ�һҳ��ʾ��һҳ
	if( pageIndex == 0){
		// ��ʾ����
		OLED_ShowChLength(0,0,23,4);
		OLED_ShowChLength(0,16,27,4);
		OLED_ShowChLength(0,32,31,4);
		OLED_ShowChLength(0,48,35,4);
		
		// ��ʾð��
		OLED_ShowString(65,0,":",16,1);
		OLED_ShowString(65,16,":",16,1);
		OLED_ShowString(65,32,":",16,1);
		OLED_ShowString(65,48,":",16,1);
		
		// ��ʾ����ֵ
		OLED_ShowNum(76,0,temp_up,2,16,1);
		OLED_ShowNum(76,16,temp_down,2,16,1);
		OLED_ShowNum(76,32,hum_up,2,16,1);
		OLED_ShowNum(76,48,hum_down,2,16,1);
		return;
	}
	if( pageIndex == 1 ){
		// ��ʾ�ڶ�ҳ
		OLED_ShowChLength(0,0,75,4);
		OLED_ShowChLength(0,16,79,4);
		OLED_ShowChLength(0,32,50,4);
		OLED_ShowChLength(0,48,54,4);
			
		// ��ʾð��
		OLED_ShowString(65,0,":",16,1);
		OLED_ShowString(65,16,":",16,1);
		OLED_ShowString(65,32,":",16,1);
		OLED_ShowString(65,48,":",16,1);
		
		// ��ʾ����ֵ
		OLED_ShowNum(76,0,flame_up,2,16,1);
		OLED_ShowNum(76,16,flame_down,2,16,1);
		OLED_ShowNum(76,32,smog_up,2,16,1);
		OLED_ShowNum(76,48,smog_down,2,16,1);
	}
	
	if( pageIndex == 2 ){
		// ��ʾ����ҳ
		OLED_ShowChLength(0,0,50,4);
		OLED_ShowChLength(0,16,54,4);
		OLED_ShowChLength(0,32,58,4);
		OLED_ShowChLength(0,48,58,4);

		// ��ʾð��
		OLED_ShowString(65,0,":",16,1);
		OLED_ShowString(65,16,":",16,1);
		OLED_ShowString(65,32,":",16,1);
		OLED_ShowString(65,48,":",16,1);
		
		// ��ʾ����ֵ
		OLED_ShowNum(76,0,0,2,16,1);
		OLED_ShowNum(76,16,0,2,16,1);
		OLED_ShowNum(76,32,0,2,16,1);
		OLED_ShowNum(76,48,0,2,16,1);
	}
	OLED_Refresh();
}

void paramCheck( void ){
	//������
	int buzzFlag;
	int ledFlag;
	int fanFlag;

	ledFlag = 0;
	fanFlag = 0;
	buzzFlag = 0;
	
	
	if( !LED_STATUS ){
		if(  temp >= temp_up && humi <= hum_down){
			ledFlag = 1;
		}	
		
		if( ledFlag ){
			LED = 1;
		}else{
			LED = 0;
		}
	}	
	
	// ���ա��¶ȡ�����Ũ�ȡ�����ʪ��ͬʱ����������������
	if( flameIntensity >= flame_up ){
		fanFlag = 1;
		buzzFlag = 1;
	}
	
	if( !FAN_STATUS ){
		if( smog >= smog_up ){
			fanFlag = 1;
			buzzFlag = 1;
		}	
		
		if( fanFlag ){
			FAN = 1;
		}else{
			FAN = 0;
		}
  }
	
	if( buzzFlag ){
		BUZZ = 1;
	}else{
		BUZZ = 0;
	}		
}




