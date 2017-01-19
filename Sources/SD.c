#include "SD.h"
#include "MPC5604B.h"
#include "includes.h"
#include "ff.h"
#include "WRITE_SD.h";
extern uint8_t BUFFER[];
signed short test_file_system();
FATFS fatfs1;
/*************************************************************/
/*                      初始化SPI模块                        */
/*************************************************************/
void SPI_Init(void) 
{

	DSPI_0.MCR.R = 0x80013001;     //设置DSPI0为主模式，CS信号低有效，禁止FIFO
	DSPI_0.CTAR[0].R = 0x3E0A7727; //配置CTAR[0]，设置为每帧数据为8位，高位在前，波特率为100KHz
	DSPI_0.MCR.B.HALT = 0x0;	     //DSPI0处于运行状态
}

/*************************************************************/
/*                    设置SPI时钟为4MHz                      */
/*************************************************************/
void SPI_4M(void) 
{ 
	DSPI_0.MCR.B.HALT = 0x1;	     //DSPI0停止运行
	DSPI_0.CTAR[0].R = 0x3E087723; //配置CTAR[0]，设置为每帧数据为8位，高位在前，波特率为4MHz
	DSPI_0.MCR.B.HALT = 0x0;	     //DSPI0处于运行状态
}

/*************************************************************/
/*                        初始化SD卡                         */
/*************************************************************/
void SD_Init(void)
{
	SPI_Init();
	SD_deselect();
	while(SD_Reset()!=0)     //初始化SD卡
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
/*                        清空缓冲区                         */
/*************************************************************/
void clear_buffer(uint8_t buffer[])
{
	uint16_t i;     
	for(i=0;i<512;i++)	
		*(buffer+i)=0;
}

/*************************************************************/
/*                      SPI读写一个字节                      */
/*************************************************************/
uint8_t SPI_Byte(uint8_t value)
{
	uint8_t input;
	DSPI_0.PUSHR.R = 0x08000000|value;    //赋值需要发送的数据	#0x08EOQ位，存疑	
	while(DSPI_0.SR.B.TCF==0)            //等待发送完成
	{}
	DSPI_0.SR.B.TCF=1;                   //清除发送完成标志位
	while(DSPI_0.SR.B.RFDF==0)           //等待接收到数据
	{}
	input=(uint8_t)(DSPI_0.POPR.R);      //读取接收到的数据
	DSPI_0.SR.B.RFDF=1;                  //清除接收标志位

	return(input);      //返回接收到的数据
}

/*************************************************************/
/*                       向SD卡写入命令                      */
/*************************************************************/
uint8_t SD_send_command(uint8_t cmd, uint32_t arg)
{
	uint8_t a,b;
	uint8_t retry=0,crc_default=0x95;
	if(cmd==8)crc_default=0x87;
	else if(cmd==41||cmd==55)crc_default=0x01;
	SPI_Byte(0xff);
	SD_select();

	SPI_Byte(cmd | 0x40);//分别写入命令
	SPI_Byte(arg>>24);
	SPI_Byte(arg>>16);
	SPI_Byte(arg>>8);
	SPI_Byte(arg);
	SPI_Byte(crc_default);

	while((a = SPI_Byte(0xff)) == 0xff)//等待响应，
		if(retry++ > 10) break;//超时退出
	if(cmd==8){SPI_Byte(0xff);
	SPI_Byte(0xff);
	SPI_Byte(0xff);
	}
	SD_deselect();

	return a;//返回状态值
}

/*************************************************************/
/*                         初始化SD卡                        */
/*************************************************************/
uint8_t SD_Reset(void)
{
	uint8_t i;
	uint8_t retry;
	uint8_t a = 0x00;

	for(i=0;i<10;i++) SPI_Byte(0xff);	//至少74个时钟，必须的!!!

	//SD卡复位
	//发送CMD0，正常跳出表示成功进入idle状态
	for(retry=0; a!=0x01; retry++)
	{
		a = SD_send_command(0, 0);	//发idle命令
		if(retry>100)
		{
			return 1;
		}
	}

	SD_send_command(8,0x1aa);


	for(retry=0; a!=0x00&&retry<100; retry++){
		SD_send_command(55,0);
		a = SD_send_command(41,0x40<<24);//返回值为0x01,正在初始化，重复发送ACMD41，直到返回值R1为0
		if(retry>100)
		{
			return 2;
		}
	}	
	for(retry=0,a=1; a!=0x00; retry++)
	{
		a = SD_send_command(1, 0);	//发idle命令
		if(retry>100)
		{
			return 1;
		}
	}


	a = SD_send_command(59, 0);	//关crc
	a = SD_send_command(16, 512);	//设扇区大小512

	return 0;	//正常返回
}

/*************************************************************/
/*                     由SD卡读取一个扇区                    */
/*************************************************************/
uint8_t read_block(uint32_t sector, uint8_t* buffer)
{
	uint8_t a;          
	uint16_t i;
	a = SD_send_command(17, sector);  //读命令 	
	if(a != 0x00) return a;

	SD_select();
	//等数据的开始
	while(SPI_Byte(0xff) != 0xfe)
	{ }

	for(i=0; i<512; i++)              //读512个数据
	{
		*buffer++ = SPI_Byte(0xff);		//不用空读
	}

	SPI_Byte(0xff);              
	SPI_Byte(0xff);  	
	SD_deselect();
	SPI_Byte(0xff);              
	return 0;
} 

/*************************************************************/
/*                     向SD卡写入一个扇区                    */
/*************************************************************/
uint8_t write_block(uint32_t sector,uint8_t* buffer)
{
	uint8_t a;
	uint16_t i;
	if(sector<1) return 0xff;     //为了保护SD卡引导区，跳过该区
	a = SD_send_command(24, sector);//写命令
	if(a != 0x00) return a;

	SD_select();

	SPI_Byte(0xff);
	SPI_Byte(0xff);
	SPI_Byte(0xff);

	SPI_Byte(0xfe);//发开始符

	for(i=0; i<512; i++)//送512字节数据
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
	//等待操作完
	while(!SPI_Byte(0xff))
	{}

	SD_deselect();

	return 0;
} 
/*************************************************************/
/*                     向SD卡写入多个扇区                    */
/*************************************************************/


void FatFs_Init(){
	char g_sdcard_status=0;
	TCHAR *path = "0:";
	/* 挂载TF卡文件系统 */
	if (FR_OK == f_mount(&fatfs1, path, 1))
	{g_sdcard_status=1;

	/* 文件读写测试 */
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
if(sector<1) return 0xff;     //为了保护SD卡引导区，跳过该区
a = SD_send_command(25, sector);//写命令
if(a != 0x00) return a;
SD_select();
SPI_Byte(0xff);
SPI_Byte(0xff);
for(i=0;i<n;i++)
{
	SPI_Byte(0xfc);
	for(j=0; j<512; j++)//送512字节数据
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
	//等待操作完
	while(!SPI_Byte(0xff))
	{}
	SPI_Byte(0xfb);
	while(!SPI_Byte(0xFF)){}
	SD_deselect();

	return 0;
} 
}
