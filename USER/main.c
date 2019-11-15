/*
	ֱ��ʽ������
		1. ����������ư�λ�ã��ñ�������ⲽ�����λ��
		2. ��λ��ADC���ڸ˽Ƕ�
		3. ������Ϊ����
		4. �ڸ˽ǶȺ�ˮƽλ�������ջ�����
		5. ����ң�ؿ��ư�ˮƽλ��
*/

#include "sys.h"
#include "delay.h"
#include "uart.h"
#include "led.h"
#include "motor.h"
#include "key.h"
#include "adc.h"
#include "pid.h"
#include "math.h"

#define vertical 675		//��ֱʱADC����

u8 Start_Flag=0;       	//�������/������־
u16 Pluse_High=5;     	//����ߵ�ƽ����      
u16 Pluse_Period=200;  	//��������     

extern float adcx;
double Ang_fdb;

//��ȫ���ƽǶ�����----------------------------------------
void safe_ang_limit(void)
{
	Ang_fdb=adcx;
	if(Ang_fdb<(vertical-400) || Ang_fdb>(vertical+400))
		Start_Flag=0;
	
	printf("%f\r\n",Ang_fdb);
}

//����Ƿ�ֱ��������ledָʾ,�ֶ���������-----------------
void Ang_cheak()
{
	if(fabs(Ang_fdb-vertical)<10)
		LED0=0;
	else 
		LED0=1;
}

//���������ȡ--------------------------------------------
u16 Key_Count=0;
void Get_Key()
{
	u8 key;
	
	key=KEY_Scan(0);
	if(key)
	{
		switch(key)
		{				 
			case KEY0_PRES:	//�������
				LED1=0,Start_Flag=1;
				break;
			case KEY1_PRES:	//�������
				LED1=1,Start_Flag=0;
				break;
//			case WKUP_PRES://����	
//				Pluse_Period-=10;
//				break;
//			case KEY2_PRES://����
//				Pluse_Period+=10;
//				break;
		}
	}
}

int Encoder[2]={0};
int hold_point;//�ڸ�ˮƽ�ȶ�λ��
//u8 hold_point_lock=0;//0->������1->�ƶ�
int speed=0;
int mode_flag=0;//0->���㡢1->�ƶ�

//��ȡˮƽ�ٶ�&λ����Ϣ---------------------------------
extern int circle_count;
void Get_Speed(int *Enc,int *Speed)
{
	Enc[0]=Enc[1];
	Enc[1]=TIM_GetCounter(TIM3)+3600*circle_count;
	*Speed = Enc[1]-Enc[0];
}

//��ƽ�ƽ��붨��ģʽ--------------------------------------
void Pos_Lock()
{
	static int Pos_Lock_cnt=0;
	Pos_Lock_cnt++;
	if(Pos_Lock_cnt>5)
	{
		mode_flag=0;
		Pos_Lock_cnt=0;
	}
}

float Speed_ref;
int main(void)
{ 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//����ϵͳ�ж����ȼ�����2	
	My_USART1_Init(115200);													//����ң�ش���
	printf("start");
	delay_init(168);
	LED_Init();			
  Motor_Init();																		//��ʼ���������
	KEY_Init();    				
	Adc_Init();  																		//�ڽǶȵ�λ��
	Encoder_Init();																	//��λ�ñ�����������TIM3��������ģʽ
 	TIM4_Int_Init(5-1,84-1);												//84M/84=1Mhz(10^6)����Ƶ�ʣ�����10��Ϊ5us  (��������)
	TIM2_Int_Init(100-1,84-1);											//0.1ms     (ADC)
	TIM5_Init(0xffffffff,84-1);											//1Mhz ����������
	
	delay_ms(100);
	
	hold_point=TIM_GetCounter(TIM3)+3600*circle_count;	//����λ��ΪĬ��ˮƽ����λ��	

	Encoder[1]=TIM_GetCounter(TIM3)+3600*circle_count;
	while(1)
	{
		if(Key_Count++>200)
		{
			Key_Count=0;
			Get_Key();
		}
		
		safe_ang_limit();
		Ang_cheak();
		
		Get_Speed(Encoder,&speed);
		
		//��λ��ƫ��С��100����pid����
		if(fabs(hold_point-Encoder[1])<100)
			Pos_Lock();
		
		//pid����
		Infantry_Pid_Contrl(vertical,Ang_fdb,hold_point,Encoder[1],speed);
//	Infantry_Pid_Contrl(vertical,Ang_fdb,0,0,0);

		delay_ms(1);
	}
}
