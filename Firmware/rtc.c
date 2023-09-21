#include "rtc.h"

//regaddr,seconds,minutes,hours,weekdays,days,months,years
char clock_buffer[]={0x00,0x50,0x59,0x17,0x04,0x05,0x03,0x21};
char *week[]={"Sun","Mon","Tues","Wed","Thur","Fri","Sat"};
 
// Function to convert an integer to BCD
unsigned char int_to_bcd(int n) {
    unsigned char bcd = 0;
    int shift = 0;

    while (n > 0) {
        int digit = n % 10;
        bcd |= (digit << shift);
        shift += 4;
        n /= 10;
    }

    return bcd;
}

void clock_set_time(unsigned char second, unsigned char minute, unsigned char hour, unsigned char day, unsigned char month, unsigned char year)
{
	char write_buffer[2];

	//set second
	write_buffer[0]=0x00;
	write_buffer[1]=second;
    i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);


	//set minute
	write_buffer[0]=0x01;
	write_buffer[1]=minute;
    i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false)  ;


	//set hour
	write_buffer[0]=0x02;
	write_buffer[1]=hour;
    i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set weekday
	write_buffer[0]=0x03;
	write_buffer[1]=0x01;
    i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);


	//set day
	write_buffer[0]=0x04;
	write_buffer[1]=day;
    i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set month
	write_buffer[0]=0x05;
	write_buffer[1]=month;
    i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);

	//set year
	write_buffer[0]=0x06;
	write_buffer[1]=year;
    i2c_write_blocking(CLOCK_I2C_LINE,CLOCK_ADDR,write_buffer,2,false);
}

void clock_read_time() 
{   
	uint8_t start_point = 0x00;  
	i2c_write_timeout_us(CLOCK_I2C_LINE,CLOCK_ADDR,&start_point,1,true, 1000);
	i2c_read_timeout_us(CLOCK_I2C_LINE,CLOCK_ADDR,clock_buffer,7,false, 1000);
} 

