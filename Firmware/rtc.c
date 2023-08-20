#include "rtc.h"

//regaddr,seconds,minutes,hours,weekdays,days,months,years
char clock_buffer[]={0x00,0x50,0x59,0x17,0x04,0x05,0x03,0x21};
char *week[]={"Sun","Mon","Tues","Wed","Thur","Fri","Sat"};

void clock_set_time()
{
	char write_buffer[2];

	//set second
	write_buffer[0]=0x00;
	write_buffer[1]=0x50;
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set minute
	write_buffer[0]=0x01;
	write_buffer[1]=0x59;
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set hour
	write_buffer[0]=0x02;
	write_buffer[1]=0x23;
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set weekday
	write_buffer[0]=0x03;
	write_buffer[1]=0x04;
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set day
	write_buffer[0]=0x04;
	write_buffer[1]=0x05;
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set month
	write_buffer[0]=0x05;
	write_buffer[1]=0x03;
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set year
	write_buffer[0]=0x06;
	write_buffer[1]=0x21;
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

}

void clock_read_time() 
{   
	uint8_t start_point = 0x00;  
	i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,&start_point,1,true);
	i2c_read_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,clock_buffer,7,false);
} 

