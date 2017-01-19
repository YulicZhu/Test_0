#include "SD.h"
#include "MPC5604B.h"
#include "includes.h"
#include "ff.h"
#include "WRITE_SD.h";
extern uint8_t BUFFER[];
signed short test_file_system();
FATFS fatfs1;
/*************************************************************/
/*                      ��ʼ��SPIģ��                        */
/*************************************************************/
void SPI_Init(void) 
{

	DSPI_0.MCR.R = 0x80013001;     //����DSPI0Ϊ��ģʽ��CS�źŵ���Ч����ֹFIFO
	DSPI_0.CTAR[0].R = 0x3E0A7727; //����CTAR[0]������Ϊÿ֡����Ϊ8λ����λ��ǰ��������Ϊ100KHz
	DSPI_0.MCR.B.HALT = 0x0;	     //DSPI0��������״̬
}

/*************************************************************/
/*                    ����SPIʱ��Ϊ4MHz                      */
/*************************************************************/
void SPI_4M(void) 
{ 
	DSPI_0.MCR.B.HALT = 0x1;	     //DSPI0ֹͣ����
	DSPI_0.CTAR[0].R = 0x3E087723; //����CTAR[0]������Ϊÿ֡����Ϊ8λ����λ��ǰ��������Ϊ4MHz
	DSPI_0.MCR.B.HALT = 0x0;	     //DSPI0��������״̬
}

/*************************************************************/
/*                        ��ʼ��SD��                         */
/*************************************************************/
void SD_Init(void)
{
	SPI_Init();
	SD_deselect();
	while(SD_Reset()!=0)     //��ʼ��SD��
	{ 	  
		delay();
	}
	SPI_4M(); 
}
void delay(void)
{
	uint32_t j;
	for(j=0;j<1000000;j++)
	{}
}

/*************************************************************/
/*                        ��ջ�����                         */
/*************************************************************/
void clear_buffer(uint8_t buffer[])
{
	uint16_t i;     
	for(i=0;i<512;i++)	
		*(buffer+i)=0;
}

/*************************************************************/
/*                      SPI��дһ���ֽ�                      */
/*************************************************************/
uint8_t SPI_Byte(uint8_t value)
{
	uint8_t input;
	DSPI_0.PUSHR.R = 0x08000000|value;    //��ֵ��Ҫ���͵�����	#0x08EOQλ������	
	while(DSPI_0.SR.B.TCF==0)            //�ȴ��������
	{}
	DSPI_0.SR.B.TCF=1;                   //���������ɱ�־λ
	while(DSPI_0.SR.B.RFDF==0)           //�ȴ����յ�����
	{}
	input=(uint8_t)(DSPI_0.POPR.R);      //��ȡ���յ�������
	DSPI_0.SR.B.RFDF=1;                  //������ձ�־λ

	return(input);      //���ؽ��յ�������
}

/*************************************************************/
/*                       ��SD��д������                      */
/*************************************************************/
uint8_t SD_send_command(uint8_t cmd, uint32_t arg)
{
	uint8_t a,b;
	uint8_t retry=0,crc_default=0x95;
	if(cmd==8)crc_default=0x87;
	else if(cmd==41||cmd==55)crc_default=0x01;
	SPI_Byte(0xff);
	SD_select();

	SPI_Byte(cmd | 0x40);//�ֱ�д������
	SPI_Byte(arg>>24);
	SPI_Byte(arg>>16);
	SPI_Byte(arg>>8);
	SPI_Byte(arg);
	SPI_Byte(crc_default);

	while((a = SPI_Byte(0xff)) == 0xff)//�ȴ���Ӧ��
		if(retry++ > 10) break;//��ʱ�˳�
	if(cmd==8){SPI_Byte(0xff);
	SPI_Byte(0xff);
	SPI_Byte(0xff);
	}
	SD_deselect();

	return a;//����״ֵ̬
}

/*************************************************************/
/*                         ��ʼ��SD��                        */
/*************************************************************/
uint8_t SD_Reset(void)
{
	uint8_t i;
	uint8_t retry;
	uint8_t a = 0x00;

	for(i=0;i<10;i++) SPI_Byte(0xff);	//����74��ʱ�ӣ������!!!

	//SD����λ
	//����CMD0������������ʾ�ɹ�����idle״̬
	for(retry=0; a!=0x01; retry++)
	{
		a = SD_send_command(0, 0);	//��idle����
		if(retry>100)
		{
			return 1;
		}
	}

	SD_send_command(8,0x1aa);


	for(retry=0; a!=0x00&&retry<100; retry++){
		SD_send_command(55,0);
		a = SD_send_command(41,0x40<<24);//����ֵΪ0x01,���ڳ�ʼ�����ظ�����ACMD41��ֱ������ֵR1Ϊ0
		if(retry>100)
		{
			return 2;
		}
	}	
	for(retry=0,a=1; a!=0x00; retry++)
	{
		a = SD_send_command(1, 0);	//��idle����
		if(retry>100)
		{
			return 1;
		}
	}


	a = SD_send_command(59, 0);	//��crc
	a = SD_send_command(16, 512);	//��������С512

	return 0;	//��������
}

/*************************************************************/
/*                     ��SD����ȡһ������                    */
/*************************************************************/
uint8_t read_block(uint32_t sector, uint8_t* buffer)
{
	uint8_t a;          
	uint16_t i;
	a = SD_send_command(17, sector);  //������ 	
	if(a != 0x00) return a;

	SD_select();
	//�����ݵĿ�ʼ
	while(SPI_Byte(0xff) != 0xfe)
	{ }

	for(i=0; i<512; i++)              //��512������
	{
		*buffer++ = SPI_Byte(0xff);		//���ÿն�
	}

	SPI_Byte(0xff);              
	SPI_Byte(0xff);  	
	SD_deselect();
	SPI_Byte(0xff);              
	return 0;
} 

/*************************************************************/
/*                     ��SD��д��һ������                    */
/*************************************************************/
uint8_t write_block(uint32_t sector,uint8_t* buffer)
{
	uint8_t a;
	uint16_t i;
	if(sector<1) return 0xff;     //Ϊ�˱���SD������������������
	a = SD_send_command(24, sector);//д����
	if(a != 0x00) return a;

	SD_select();

	SPI_Byte(0xff);
	SPI_Byte(0xff);
	SPI_Byte(0xff);

	SPI_Byte(0xfe);//����ʼ��

	for(i=0; i<512; i++)//��512�ֽ�����
	{
		SPI_Byte(*buffer++);
	}

	SPI_Byte(0xff);
	SPI_Byte(0xff);

	a = SPI_Byte(0xff); 	
	if( (a&0x1f) != 0x05)
	{
		SD_deselect();
		return a;
	}
	//�ȴ�������
	while(!SPI_Byte(0xff))
	{}

	SD_deselect();

	return 0;
} 
/*************************************************************/
/*                     ��SD��д��������                    */
/*************************************************************/


void FatFs_Init(){
	char g_sdcard_status=0;
	TCHAR *path = "0:";
	/* ����TF���ļ�ϵͳ */
	if (FR_OK == f_mount(&fatfs1, path, 1))
	{g_sdcard_status=1;

	/* �ļ���д���� */
	//g_sdcard_status=WRITE_SD();
	g_sdcard_status=test_file_system();
	}
}

signed short test_file_system()
{
	FIL fil1, fil2, fil3;
	TCHAR *tchar = "TEST.txt";
	unsigned short br;
	unsigned short wr;
	unsigned char test_write_to_TFCard[15] = "Hello world!";
	unsigned char test_read_from_TFCard[15] = "";
//	disk_initialize(0);

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
		if (FR_OK == f_write(&fil2, (const void *)&test_write_to_TFCard, sizeof(test_write_to_TFCard), &wr))
		{
			if (FR_OK == f_close(&fil2))
			{
				
			}
			else
			{
				return 5;
			}
		}
		else
		{
			return 4;
		}
	}
	else
	{
		return 3;
	}

	if (FR_OK == f_open(&fil3, tchar, FA_READ))
	{
		if (FR_OK == f_read(&fil3, (void *)&test_read_from_TFCard, sizeof(test_read_from_TFCard), &br))
		{
			if (FR_OK == f_close(&fil3))
			{
				
			}
			else
			{
				return 8;
			}
		}
		else
		{
			return 7;
		}
	}
	else
	{
		return 6;
	}
	
	if (test_write_to_TFCard == test_read_from_TFCard)
	{
		return 0;
	}
	else
	{
		return 7;
	}
}

uint8_t SD_write_multiple_block(uint32_t sector, uint8_t n, const uint8_t buffer[][512])
{
uint8_t a;
uint16_t i;
uint16_t j;
if(sector<1) return 0xff;     //Ϊ�˱���SD������������������
a = SD_send_command(25, sector);//д����
if(a != 0x00) return a;
SD_select();
SPI_Byte(0xff);
SPI_Byte(0xff);
for(i=0;i<n;i++)
{
	SPI_Byte(0xfc);
	for(j=0; j<512; j++)//��512�ֽ�����
	{
		SPI_Byte(buffer[i][j]);
	}

	SPI_Byte(0xff);
	SPI_Byte(0xff);

	a = SPI_Byte(0xff); 	
	if( (a&0x1f) != 0x05)
	{
		SD_deselect();
		return a;
	}
	//�ȴ�������
	while(!SPI_Byte(0xff))
	{}
	SPI_Byte(0xfb);
	while(!SPI_Byte(0xFF)){}
	SD_deselect();

	return 0;
} 
}
