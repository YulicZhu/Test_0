/*
 * WRITE_SD.c
 *
 *  Created on: Nov 27, 2016
 *      Author: Yulic_zhu
 */
#include "MPC5604B.h"
#include "SD.h"
#include "WRITE_SD.h"
#include "includes.h"
#include "ff.h"
FATFS fatfs2;
uint16_t m;
uint16_t error;
struct buff{
	char clock[11];
	char current_val[5];
	char volt_val[5];
	char mscounter[11];

}*BUFFER;

/************************************************************/
/*                       初始化SIU                          */
/************************************************************/
char * strcpy(char * strDest,const char * strSrc)
{char * strDestCopy = strDest; 
while ((*strDest++=*strSrc++)!='\0'); 
return strDestCopy;
}
/************************************************************/
/*                          主函数                          */
/************************************************************/

void init_SD_FatFs(){
	SD_Init();
	LED=1;
	delay();
	FatFs_Init();
}

int WRITE_SD(uint32_t (*a)[4],int num)
{
	FIL fil1, fil2, fil3;
	TCHAR *tchar = "TESTxm.txt";
	unsigned short br;
	unsigned short wr;
	char Start[3]={0x55,0xAA,0xFF};
	char End[3]={0xFF,0xAA,0x55};
	uint32_t (*p)[4];
	int i,j;
	
	p=a;
	if (FR_OK == f_open(&fil1, tchar, FA_CREATE_ALWAYS))
	{
		if (FR_OK == f_close(&fil1))
		{

		}
		else
		{
			return 2;
		}
	}
	else
	{
		return 1;
	}
	if (FR_OK == f_open(&fil2, tchar, FA_WRITE))
	{
		f_write(&fil2, (const void *)&Start, sizeof(Start), &wr);
		f_lseek(&fil2,f_size(&fil2));
		f_write(&fil2, (const void *)&num, sizeof(num), &wr);
		f_lseek(&fil2,f_size(&fil2));
		for(i=0;i<1024;i++,p++)
			for(j=0;j<4;j++)
		{
			f_write(&fil2, (const void *)&((*p)[j]), sizeof((*p)[j]), &wr);
			f_lseek(&fil2,f_size(&fil2));
		}
		f_write(&fil2, (const void *)&End, sizeof(End), &wr);
		if (FR_OK == f_close(&fil2))
		{
			return 0;
		}
		else
		{
			return 5;
		}
	}
	else
	{
		return 3;
	}
	/*for(i=0;i<1024;i++)
		  for(j=0;j<4;j++)
	{
		strcpy(BUFFER->clock,(unsigned char *)a[i][j]);
		strcpy(BUFFER->current_val,(unsigned char *)a[i][j]);
		strcpy(BUFFER->volt_val,(unsigned char *)a[i][j]);
		strcpy(BUFFER->mscounter,(unsigned char *)a[i][j]);
	}*/

	/*	//写入行列
				f_write(&Cfil2, (const void *)&RowCol1, sizeof(RowCol1), &wr);
				f_lseek(&Cfil2,f_size(&Cfil2));
				f_write(&Cfil2, (const void *)&row, sizeof(row), &wr);
				f_lseek(&Cfil2,f_size(&Cfil2));
				f_write(&Cfil2, (const void *)&column, sizeof(column), &wr);
				f_lseek(&Cfil2,f_size(&Cfil2));
				f_write(&Cfil2, (const void *)&RowCol2, sizeof(RowCol2), &wr);
				f_lseek(&Cfil2,f_size(&Cfil2));*/

			//写入采集数据

}
