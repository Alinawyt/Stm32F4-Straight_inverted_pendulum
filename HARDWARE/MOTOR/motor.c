/*
	�������ˣ����ָ���ģ��������дһ���ˣ��Ժ�Ӧ�ò𿪵�����ļ��� =_=
*/
#include "motor.h"
#include "key.h"
#include "led.h"
#include "pid.h"
#include "adc.h"
#include "uart.h"	

extern u16 Pluse_High;
extern u16 Pluse_Period;
extern u8 Start_Flag;


/*=======================================================�����ʼ�����===============================================================*/
//����������I/O��ʼ��
void Motor_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);//ʹ��GPIOEʱ��
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ��GPIOBʱ��
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6; //DRIVER_DIR DRIVER_OE��Ӧ����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOE, &GPIO_InitStructure);//��ʼ��GPIOE5,6
	
	dirction=1;//PE5����� ˳ʱ�뷽��  DRIVER_DIR
	motor_enable=0;//PE6����� ʹ�����  DRIVER_OE
//---------------------------------------------------------------------------	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; //STEP_PULSE ��Ӧ����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//��ͨ���ģʽ
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//�ٶ�100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//����
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��PB8
}

//���������I/O��ʼ�� PA6(CH1),PA7(CH2) ��Ϊ������A��B��
void Encoder_Init(void)
{
	TIM3_Int_Init();
	
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource6,GPIO_AF_TIM3);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource7,GPIO_AF_TIM3);
	
	GPIO_Init(GPIOA, &GPIO_InitStructure); 

}


//������ٶȿ��ƺ���
extern float Ang_OUT;
extern float Pos_OUT;

extern int mode_flag;//0->���㡢1->�ƶ�
void motor_control(float a,float p)
{
	static float temp_x=1;
	static u8 first_flag=1;
	static float last_p=0;

	if(mode_flag==0)
	{
		first_flag=1;
		temp_x+=a;
		temp_x-=p;
	
	}	
	else
	{
		if(first_flag)
			first_flag=0,temp_x=-p,last_p=-p;
		else
			temp_x+=(p-last_p);
		temp_x+=a;
	}
	
	if(temp_x>0)
		dirction=1,Pluse_Period=10000/temp_x;
	else
		dirction=0,Pluse_Period=-10000/temp_x;
	
	if(Pluse_Period<25)
		Pluse_Period=25;
}



/*=======================================================���ֶ�ʱ������===============================================================*/
//TIM4 �������ɶ�ʱ��
void TIM4_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);  ///ʹ��TIM4ʱ��
  TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; //����ʱ�ӷָ�
	
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseInitStructure);//��ʼ��TIM4
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE); //����ʱ��4�����ж�

	NVIC_InitStructure.NVIC_IRQChannel=TIM4_IRQn; //��ʱ��4�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03; //�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_Cmd(TIM4,ENABLE); //ʹ�ܶ�ʱ��4
}

//TIM3 ������ģʽ�����ڰ�λ�ñ�����
void TIM3_Int_Init() 
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_ICInitTypeDef TIM_ICInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///ʹ��TIM3ʱ��

//��ʱ������-------------------------------------------------------------	
  TIM_TimeBaseInitStructure.TIM_Period = 3600; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=0x0;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; //����ʱ�ӷָ�
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//��ʼ��TIM3
//������ģʽ--------------------------------------------------------------	
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);//???????3
  TIM_ICStructInit(&TIM_ICInitStructure); 
  TIM_ICInitStructure.TIM_ICFilter = 10;
  TIM_ICInit(TIM3, &TIM_ICInitStructure);
//�ж�����--------------------------------------------------------------	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�

	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn; //��ʱ��3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01; //�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
  
	//Reset counter
  TIM_SetCounter(TIM3,0); //TIM3->CNT=0
  TIM_Cmd(TIM3, ENABLE); 
}

//TIM2 ���ɰڽǶȵ�λ����ADC��������
void TIM2_Int_Init(u16 arr,u16 psc) 
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  ///ʹ��TIM3ʱ��
	
  TIM_TimeBaseInitStructure.TIM_Period = arr; 	//�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; //����ʱ�ӷָ�
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//��ʼ��TIM3
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�

	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn; //��ʱ��3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x01; //��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x02; //�����ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_Cmd(TIM2,ENABLE); //ʹ�ܶ�ʱ��3
}

//TIM5 ����PID��������
void TIM5_Init(uint32_t prd,uint32_t psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);  
	
	TIM_TimeBaseInitStructure.TIM_Period = prd; 
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; 
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseInitStructure);
	
	
	TIM_ARRPreloadConfig(TIM5,ENABLE);
	TIM_Cmd(TIM5,ENABLE); 
}

/*===============================================================���ֶ�ʱ���жϷ�����===============================================================*/
//��ʱ���ж�2  ADCɨ��
float adcx;					//ָʾ�ڸ˽Ƕ�
u16 adc_buff_cnt=0;	//������ֵ�˲�
u16 adc_buff[20];		//������ֵ�˲�
void TIM2_IRQHandler(void)//0.1ms
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET) 	//����ж�
	{		
		static u16 ADC_Count=0;											//ADC��������
		
		ADC_Count++;
		if(ADC_Count>3)															//0.2ms
		{
			ADC_Count=0;
			
			//��������们���˲�����
			if(adc_buff_cnt<20)
			{
				adc_buff[adc_buff_cnt]=Get_Adc(ADC_Channel_5);
				adc_buff_cnt++;
			}
			//�����ϣ����л����˲�
			else
			{
				for(int i=0;i<19;i++)
					adc_buff[i]=adc_buff[i+1],adcx+=adc_buff[i];
				
				adc_buff[19]=Get_Adc(ADC_Channel_5);
				adcx+=adc_buff[19];
				
				adcx/=20;
			}
		}
	}
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  //����жϱ�־λ
}



//��ʱ���ж�3 ����������
int circle_count=0;
void TIM3_IRQHandler(void)//100ms
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //����ж�
	{		
		if((TIM3->CR1>>4 & 0x01)==0) //DIRλ==0
			circle_count++;
		else if((TIM3->CR1>>4 & 0x01)==1)//DIRλ==1
			circle_count--;
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //����жϱ�־λ
}


//��ʱ��4�жϷ����� �������
void TIM4_IRQHandler(void)//5us
{
	if(TIM_GetITStatus(TIM4,TIM_IT_Update)==SET) //����ж�
	{	
		static u32 TimeCount=0;
		static u32 AccCount=0;
		
		if(Start_Flag==0)
			motor_enable=1;
		else if(Start_Flag==1)
		{
			motor_enable=0;
			TimeCount++;
			AccCount++;
			
			if(TimeCount<Pluse_High)//����ߵ�ƽ����10us
				Pluse=1;
			else if(TimeCount>Pluse_High)
				Pluse=0;
			if(TimeCount>Pluse_Period)//����
				TimeCount=0;
			
		}
		
		if(AccCount>5)	//0.1ms
		{
			AccCount=0;
			motor_control(Ang_OUT/1000,Pos_OUT/1000);
		}	
	}
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);  //����жϱ�־λ
}




