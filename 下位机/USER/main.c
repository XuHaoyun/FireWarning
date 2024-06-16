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

int temp_up = 20,temp_down = 20;  // 温度上下限
int hum_up = 50,hum_down = 50;    // 湿度上下限
int flame_up = 80,flame_down = 0;    // 光照上下限
int smog_up = 40,smog_down = 0; // 烟雾上下限

#define BUZZ  PAout(4)     // 蜂鸣器
#define FAN   PAout(5)     // 风扇
#define LED   PAout(6)     // LED灯

#define KEY_EDIT PBin(4)   // 设置按钮
#define KEY_NEXT PBin(12)  // 切换按钮
#define KEY_SWIT PBin(13)  // 下一页按钮
#define KEY_ADD  PBin(14)  // 加一按钮
#define KEY_DEC  PBin(15)  // 减一按钮

// 上位机数据处理
int handleFlag = 0;        // 判断上位机发送的数据需要处理哪一个
int sendFlag = 0;          // 定时器时间到了该位置一表示发送数据
int initFlag = 0;          // ESP8266初始化成功标志位
int paramFlag = 1;         // 是否开启参数检查

int flameIntensity = 0;     // 火焰强度 
int smog = 78;   // 光照
unsigned char temp = 0,humi = 0;     // 温度,湿度

void paramCheck( void );             // 检查参数是否超过
void handleData( void );             // 上位机数据处理
void DisplayUI( void );              // 固定页面UI渲染
void paramEdit( void );              // 阈值参数设置
void editUiDisplay( int pageIndex ); // 设置页面初始化
void runAlter(int cursor,int count); // 执行参数修改
        
extern char *USARTx_RX_BUF;         // 串口数据缓存

int main(void)
{
	delay_init();
	LED_Init();
	OLED_Init();
	Adc_Init();
	uart_init(115200);   	     // esp8266串口初始化
	timeInit(4999,7199);       // 72M 0.1ms 500ms
	timeSendInit(14999,19999); // 72m 0.2ms 3s
	OLED_ShowChLength(38,16,47,3);
	ESP8266Init(WIFIName,WIFIpwd,ip,clientId,userName,password,subTopic); //  ESP8266接入云平台
	BH1750_Init();            // BH1750初始化
	while(DHT11_Init());      // 等待dth11初始化成功
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
		// 进入设置页面
		if (!KEY_EDIT) {
				while (!KEY_EDIT); // 防抖处理
				paramEdit(); // 进入参数设置页面
				if (!paramFlag) OLED_ShowChLength(105, 2, 62, 1); // 如果参数提示按钮关闭，则显示按钮
				else OLED_ShowString(105, 2, "  ", 16, 1); // 如果参数提示按钮打开，则清空按钮显示内容
		}
		
		// 检测是否按下“下一个”键
		if (!KEY_NEXT) {
				while (!KEY_NEXT); // 防抖处理
				paramFlag = !paramFlag; // 切换参数提醒设置的状态
				if (!paramFlag) {
						// 如果参数提示按钮关闭，则将FAN、LED、BUZZ的值都设置为0
						FAN = 0;
						LED = 0;
						BUZZ = 0;
						OLED_ShowChLength(105, 2, 62, 1); // 显示按钮
				}
				else {
						OLED_ShowString(105, 2, "  ", 16, 1); // 清空按钮显示内容
				}
				OLED_Refresh(); // 刷新OLED显示屏
		}

		// 处理上位机更改的数据
		if ( handleFlag ) {
				handleData();
		}

		// 检测是否到达上传数据的时机
		if (sendFlag) {
				ESP8266Pub(pubTopic, temp, humi, flameIntensity, smog); // 发送数据
				sendFlag = 0; // 将sendFlag置为0
				TIM_SetCounter(TIM4, 0); // 重新计时3秒
		}

		// 检查参数范围
		if (paramFlag) {
				paramCheck();
		}
		delay_ms(100); // 延时100毫秒
	}
}

// 绘制固定不变的UI画面
void DisplayUI( void ){
	
	// 显示汉字
	OLED_ShowChLength(0,0,11,2);
	OLED_ShowChLength(0,16,13,2);
	OLED_ShowChLength(0,32,67,4);
	OLED_ShowChLength(0,48,71,4);
	
	// 汉字冒号
	OLED_ShowString(33,0,":",16,1);
	OLED_ShowString(33,16,":",16,1);
	OLED_ShowString(65,32,":",16,1);
	OLED_ShowString(65,48,":",16,1);

	//温度
	OLED_ShowChLength(65,1,21,1);
	//湿度
	OLED_ShowString(65,16,"%RH",16,1);

	//烟雾
	OLED_ShowString(100,32,"%",16,1);
	//光照
	OLED_ShowString(93,48,"%LX",16,1);
}

// 处理阈值参数
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
	// 初始化UI
	OLED_Clear();
	OLED_ShowString(112,0,"<",16,1);
	
	while( 1 ){
		editUiDisplay(pageIndex); // UI显示
		if( !KEY_NEXT ){
			while( !KEY_NEXT );
			beforeCursor = (cursor % 4)*16;              // 拿到之前的修改显示位置
			cursor++;		                                  
			cursor = (cursor % 4) + 4 * pageIndex;   		 // 得到第几个数据
			alterTip = (cursor % 4) * 16;            		 // 提示按钮的显示行
			OLED_ShowString(112,beforeCursor," ",16,1);  // 显示提示<
			OLED_ShowString(112,alterTip,"<",16,1);      // 显示提示< 
		}
		if( !KEY_SWIT ){                  // 按下页面切换按钮
			while(!KEY_SWIT);
			pageIndex = (++pageIndex) % 2;  // 切换页面
			cursor = 4 * pageIndex;         // 拿到参数的游标 
			OLED_Clear();
			OLED_ShowString(112,0,"<",16,1);// 重新显示提示按钮
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
 		if( !KEY_EDIT ){                  // 按下设置按钮推出当前页面
			while( !KEY_EDIT );
			OLED_Clear();
			break;
		}
		handleData();
		OLED_Refresh(); 
	}
}

// 通过cursor，来确定哪里需要更改值
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


// 设置初始化UI
void editUiDisplay( int pageIndex ){
	// 如果是第一页显示第一页
	if( pageIndex == 0){
		// 显示汉字
		OLED_ShowChLength(0,0,23,4);
		OLED_ShowChLength(0,16,27,4);
		OLED_ShowChLength(0,32,31,4);
		OLED_ShowChLength(0,48,35,4);
		
		// 显示冒号
		OLED_ShowString(65,0,":",16,1);
		OLED_ShowString(65,16,":",16,1);
		OLED_ShowString(65,32,":",16,1);
		OLED_ShowString(65,48,":",16,1);
		
		// 显示测量值
		OLED_ShowNum(76,0,temp_up,2,16,1);
		OLED_ShowNum(76,16,temp_down,2,16,1);
		OLED_ShowNum(76,32,hum_up,2,16,1);
		OLED_ShowNum(76,48,hum_down,2,16,1);
		return;
	}
	if( pageIndex == 1 ){
		// 显示第二页
		OLED_ShowChLength(0,0,75,4);
		OLED_ShowChLength(0,16,79,4);
		OLED_ShowChLength(0,32,50,4);
		OLED_ShowChLength(0,48,54,4);
			
		// 显示冒号
		OLED_ShowString(65,0,":",16,1);
		OLED_ShowString(65,16,":",16,1);
		OLED_ShowString(65,32,":",16,1);
		OLED_ShowString(65,48,":",16,1);
		
		// 显示测量值
		OLED_ShowNum(76,0,flame_up,2,16,1);
		OLED_ShowNum(76,16,flame_down,2,16,1);
		OLED_ShowNum(76,32,smog_up,2,16,1);
		OLED_ShowNum(76,48,smog_down,2,16,1);
	}
	
	if( pageIndex == 2 ){
		// 显示第三页
		OLED_ShowChLength(0,0,50,4);
		OLED_ShowChLength(0,16,54,4);
		OLED_ShowChLength(0,32,58,4);
		OLED_ShowChLength(0,48,58,4);

		// 显示冒号
		OLED_ShowString(65,0,":",16,1);
		OLED_ShowString(65,16,":",16,1);
		OLED_ShowString(65,32,":",16,1);
		OLED_ShowString(65,48,":",16,1);
		
		// 显示测量值
		OLED_ShowNum(76,0,0,2,16,1);
		OLED_ShowNum(76,16,0,2,16,1);
		OLED_ShowNum(76,32,0,2,16,1);
		OLED_ShowNum(76,48,0,2,16,1);
	}
	OLED_Refresh();
}

void paramCheck( void ){
	//蜂鸣器
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
	
	// 光照、温度、烟雾浓度、空气湿度同时满足条件蜂鸣器响
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




