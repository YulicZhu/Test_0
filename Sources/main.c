#include "MPC5604B.h"
#include "WRITE_SD.h"
#include "MODULE_Init.h"
int cap_num=0;
void Single_Collection();
uint32_t volt_capture(void);
long Read_Stm();
int main(void) {
	SYSTEM_Init();
	SIU_Init();
	EMIOS_Cnf();
	init_SD_FatFs();
	for(;;){
		Single_Collection();
		//		emios_0_modual_counter=EMIOS_0.CH[8].CCNTR.B.CCNTR;
	}
}
uint32_t volt_capture(void){
	ADC.NCMR[0].B.CH0 = 1;

	ADC.MCR.B.NSTART =1;

	while(ADC.CDR[0].B.VALID!=1){}

	return (uint32_t)ADC.CDR[0].B.CDATA*5000/0x3FF;//volt_val

}
long Read_Stm(){
	//STM.CH[0].CCR.B.CEN = 1;//enable
	STM.CR.B.CPS=0xf;//1MHz;
	STM.CR.B.TEN = 1;//enable
	return STM.CNT.R;
}
void Single_Collection(){
	unsigned long sys_clock_base,t,i,FLAG=0;
	unsigned long emios_0_modual_counter=0;
	
	uint32_t a[1024][4];

	EMIOS_0.CH[8].CCR.B.MODE = 0x13; // Modulus Counter(MC),输入模式 ,external clock*/，启动计数器
	sys_clock_base=Read_Stm();

	for(t=0;t<1024;t++)
	{	a[t][0]=(uint32_t)Read_Stm()-sys_clock_base;
		a[t][1]=t;//volt_capture();//current_capture();
		a[t][2]=2*t;//volt_capture();
		a[t][3]=(uint32_t)EMIOS_0.CH[8].CCNTR.B.CCNTR;
		LED=(FLAG^=1);
	for(i=0;i<50;i++){};//延时一小会--改成查询stm
	}

	EMIOS_0.CH[8].CCR.B.MODE = 00;//MS计数器清零
	WRITE_SD(a,cap_num);
	cap_num++;//1024组数据，每组4*4字节，每次发送32*16=512字节，需分32次发送，
}
