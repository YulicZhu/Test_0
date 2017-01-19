#include "MPC5604B.h"

void SYSTEM_Init(void)
{
	short sys_clk_select_status;

	ME.RUNPC[0].B.RUN0 = 1;			//������������0:RUN0����
	ME.MCTL.R = 0x40005AF0;			//����RUN0ģʽ
	ME.MCTL.R = 0x4000A50F;
	while(ME.GS.B.S_MTRANS){}

	sys_clk_select_status = CGM.SC_SS.B.SELSTAT;

	if(sys_clk_select_status!=0){	//assert sysclk is 16MHZ
		while(1);
	}

	
	// SIUL: ѡ�� ME.RUNPC[0] ������  
	ME.PCTL[68].R = 0x00;                 
	// DSPI0: ѡ�� ME.RUNPC[0] ������  
	ME.PCTL[4].R = 0x00;                 
}
void SIU_Init(void)
{
	SIU.PCR[5].B.PA=01;				//Pwm output channel PA5
	SIU.PCR[8].B.PA=01;				//Modulus Counter channel input
	SIU.PCR[71].B.PA=01;			//Timebase channel	PE7

	SIU.PCR[20].B.APC=1;		//ADC-B4
	
	SIU.PCR[76].R = 0x0220;        //����PE[12]Ϊ��©���������D1

	SIU.PCR[13].R = 0x0604;        //����PA[13]ΪSOUT�ӿ�
	SIU.PCR[12].R = 0x0103;        //����PA[12]ΪSIN�ӿ�
	SIU.PCR[14].R = 0x0604;        //����PA[14]ΪSCK�ӿ�
	SIU.PCR[15].R = 0x0223;        //����PA[15]Ϊ��©�������ΪCS��ʹ���ڲ���������
}
void EMIOS_Cnf(){

	/*���ö˿�*/
	EMIOS_0.UCDIS.B.CHDIS23 = 0;	
	EMIOS_0.UCDIS.B.CHDIS5 = 0;	

	/*����EMIOSģ��*/
	EMIOS_0.MCR.B.GPREN= 0;			//Disnable Global Prescale

	//config timebase channel
	EMIOS_0.CH[23].CCR.B.UCPEN =0;	//Disable UC prescaler of timebase channel
	EMIOS_0.CH[23].CCNTR.B.CCNTR =0;
	EMIOS_0.CH[23].CADR.B.CADR = 533;//30kHZ when MCR.GPRE=0
	EMIOS_0.CH[23].CCR.B.MODE =022;	//Set channel mode to MC up mode (internal clock)
	EMIOS_0.CH[23].CCR.B.UCPRE=0;	//channel prescaler pass through
	EMIOS_0.CH[23].CCR.B.UCPEN =1;	//Disable UC prescaler of timebase channel


	//config output channel
	EMIOS_0.CH[5].CCR.B.UCPEN=00;
	EMIOS_0.CH[5].CADR.B.CADR =00;
	EMIOS_0.CH[5].CBDR.B.CBDR =0x100;
	EMIOS_0.CH[5].CCR.B.BSL=00;
	EMIOS_0.CH[5].CCR.B.MODE =0140;//set channel mode to OPWMB(only A1 match FLAG)
	EMIOS_0.CH[5].CCR.B.UCPRE=00;	//channel prescaler pass through
	EMIOS_0.CH[5].CCR.B.UCPEN =1;	//Disable UC prescaler of timebase channel
	//config mc counter
	EMIOS_0.UCDIS.B.CHDIS8 = 0;
	EMIOS_0.CH[8].CCR.B.MODE = 0x13; /* Modulus Counter(MC),����ģʽ ,external clock*/
	EMIOS_0.CH[8].CCR.B.EDPOL=1;
	EMIOS_0.CH[8].CCR.B.EDSEL=0;
	EMIOS_0.CH[8].CADR.B.CADR =0xffff;

	//ʹ��EMIOS_0ȫ��ʱ��
	EMIOS_0.MCR.B.GPRE = 0;	    //Global Prescaler pass through
	EMIOS_0.MCR.B.GPREN = 1;			//Enable Global Prescaler
	EMIOS_0.MCR.B.GTBE = 1;			//Enable Global Timebase

}
