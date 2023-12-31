#include "stm32f10x.h"
#include "USART.h"
#include "TIM.h"
#include "LED.h"
#include "mpu6050.h"
#include "imu.h"
#include "ANO_DT.h"
#include "Remote.h"
#include "control.h"
#include "delay.h"

#undef SUCCESS
#define SUCCESS 0
#undef FAILED
#define FAILED 1

volatile uint32_t SysTick_count; // 系统时间计数
_st_Mpu MPU6050;				 // MPU6050原始数据
_st_Mag AK8975;
_st_AngE Angle;		   // 当前角度姿态值
_st_Remote Remote;	   // 遥控通道值
_st_ALL_flag ALL_flag; // 系统标志位，包含解锁标志位等

PidObject pidRateX; // 内环PID数据
PidObject pidRateY;
PidObject pidRateZ;

PidObject pidPitch; // 外环PID数据
PidObject pidRoll;
PidObject pidYaw;

PidObject pidHeightRate;
PidObject pidHeightHigh;

void pid_param_Init(void); // PID控制参数初始化，改写PID并不会保存数据，请调试完成后直接在程序里更改 再烧录到飞控

void TIM3_IRQHandler(void) // TIM3中断 3ms
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET) // 检查指定的TIM中断发生与否:TIM 中断源
	{
		SysTick_count++;
		
		MpuGetData();// 获取MPU6050数据
		GetAngle(&MPU6050, &Angle, 0.00626f);// 获取姿态角度
		RC_Analy();// 解析遥控器数据
		FlightPidControl(0.003f);// 飞行控制PID计算
		MotorControl();// 电机控制
		
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update); // 清除TIMx的中断待处理位:TIM 中断源
	}
}

int main(void)
{
	SystemInit();									// System init.
	delay_init();									// delay init.
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); // 2个bit的抢占优先级，2个bit的子优先级

	// 系统初始化
	IIC_Init();		  // I2C初始化
	pid_param_Init(); // PID参数初始化
	LEDInit();		  // LED闪灯初始化
	MpuInit();		  // MPU6050初始化
	USART1_Config(115200);
	USART3_Config(115200);
	TIM2_PWM_Config(); // 4路PWM初始化
	TIM3_Config();	   // 系统工作周期初始化

	while (1)
	{
		// ANTO_polling(); // 匿名上位机
		PilotLED(); // LED刷新
	}
}

void pid_param_Init(void)
{
	pidRateX.kp = 2.0f;
	pidRateY.kp = 2.0f;
	pidRateZ.kp = 4.0f;

	pidRateX.ki = 0.0f;
	pidRateY.ki = 0.0f;
	pidRateZ.ki = 0.0f;

	pidRateX.kd = 0.28f;
	pidRateY.kd = 0.28f;
	pidRateZ.kd = 0.4f;
//	pidRateX.kd = 0.08f;
//	pidRateY.kd = 0.08f;
//	pidRateZ.kd = 0.5f;

	pidPitch.kp = 7.0f;
	pidRoll.kp = 7.0f;
	pidYaw.kp = 7.0f;
}
