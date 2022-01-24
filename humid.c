/*****************************************************/
/* by adventuresin9                                  */
/*                                                   */
/* This is a simple pogram to read the temperature   */
/* and humidity from the HTS221 chip found on the    */
/* Raspberry Pi Sense Hat.                           */
/*                                                   */
/* It will mount the i²c device address, initialize  */
/* the chip, read the data, then shutdown and close. */
/*                                                   */
/* Look up the datasheet on the HTS221 for more      */
/* details on the settings and the math needed to    */
/* get readable values.  Linear interpolation        */
/* needs to be done to get Celcius and Relative      */
/* Humidity values.                                  */
/*****************************************************/



#include <u.h>
#include <libc.h>


typedef struct CalTable CalTable;

struct CalTable
{
	int h0rh;
	int h1rh;
	int h0out;
	int h1out;
	int t0degc;
	int t1degc;
	int t0out;
	int t1out;
};


int
mountdev(void)
{
	int fd = -1;

/* HTS221 located at address 5F */

	if(bind("#J5f", "/dev", MBEFORE) < 0)
		sysfatal("bind failed");

	fd = open("/dev/i2c.5f.data", ORDWR);
	if(fd < 0)
		sysfatal("open failed");

	return(fd);
}


void
initchip(int fd)
{

	uchar buf[2];

	/* av_conf reg, 1B=00011011 */
	buf[0] = 0x10;
	buf[1] = 0x1B;
	pwrite(fd, buf, 2, 0);

	/* ctrl_reg1, power up, one-shot */
	buf[0] = 0x20;
	buf[1] = 0x80;
	pwrite(fd, buf, 2, 0);

	/* ctrl_reg2, reboot, enable one-shot*/
	buf[0] = 0x21;
	buf[1] = 0x81;
	pwrite(fd, buf, 2, 0);
}


void
closechip(int fd)
{
	uchar buf[2];

	buf[0] = 0x20;
	buf[1] = 0x00;
	pwrite(fd, buf, 2, 0);

	close(fd);

	unmount("#J5f", "/dev");
}


void
getcal(int fd, CalTable *c)
{

/* This is all to fetch and assemble */
/* the calabration data needed to    */
/* get usable info out of the actual */
/* temperature and humidity outputs  */

	uchar reg[1], buf[1], tmp[1];


	reg[0] = 0x30;
	pwrite(fd, reg, 1, 0);
	pread(fd, buf, 1, 0);
	c->h0rh = buf[0] / 2;

	reg[0] = 0x31;
	pwrite(fd, reg, 1, 0);
	pread(fd , buf, 1, 0);
	c->h1rh = buf[0] / 2;

	reg[0] = 0x35;
	pwrite(fd, reg, 1, 0);
	pread(fd, tmp, 1, 0);

	
	reg[0] = 0x32;
	pwrite(fd, reg, 1, 0);
	pread(fd, buf, 1, 0);
	c->t0degc = buf[0] | ((tmp[0] & 0x3) << 8);
	c->t0degc = c->t0degc / 8;

	reg[0] = 0x33;
	pwrite(fd, reg, 1, 0);
	pread(fd, buf, 1, 0);
	c->t1degc = buf[0] | ((tmp[0] & 0xC) << 6);
	c->t1degc = c->t1degc / 8;

	reg[0] = 0x36;
	pwrite(fd, reg, 1, 0);
	pread(fd, buf, 1, 0);
	reg[0] = 0x37;
	pwrite(fd, reg, 1, 0);
	pread(fd, tmp, 1, 0);
	c->h0out = buf[0] | (tmp[0] << 8);

	reg[0] = 0x3A;
	pwrite(fd, reg, 1, 0);
	pread(fd, buf, 1, 0);
	reg[0] = 0x3B;
	pwrite(fd, reg, 1, 0);
	pread(fd, tmp, 1, 0);
	c->h1out = buf[0] | (tmp[0] << 8);

	reg[0] = 0x3C;
	pwrite(fd, reg, 1, 0);
	pread(fd, buf, 1, 0);
	reg[0] = 0x3D;
	pwrite(fd, reg, 1, 0);
	pread(fd, tmp, 1, 0);
	c->t0out = buf[0] | (tmp[0] << 8);

	reg[0] = 0x3E;
	pwrite(fd, reg, 1, 0);
	pread(fd, buf, 1, 0);
	reg[0] = 0x3F;
	pwrite(fd, reg, 1, 0);
	pread(fd, tmp, 1, 0);
	c->t1out = buf[0] | (tmp[0] << 8);

	if(c->h0out > 32767)
		c->h0out -= 65536;

	if(c->h1out > 32767)
		c->h1out -= 65536;

	if(c->t0out > 32767)
		c->t0out -= 65536;

	if(c->t1out > 32767)
		c->t0out -= 65536;

}


float
gettemp(int fd, CalTable c)
{

/* This gets the current temperature output */
/* smashes it against the calabration data  */
/* and returns temperature in C             */

	uchar reg[1], low[1], high[1];
	int tout;
	float temp;

	reg[0] = 0x2A;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x2B;
	pwrite(fd, reg, 1, 0);
	pread(fd, high, 1, 0);

	tout = low[0] | (high[0] << 8);

	if(tout > 32767)
		tout -= 65536;

	temp = ((tout - c.t0out) * (c.t1degc - c.t0degc)) / (c.t1out - c.t0out) + c.t0degc;

	return temp;
}


float
gethum(int fd, CalTable c)
{

/* This gets the current humidity output */
/* massages it with the calabration data */
/* and returns Relative Humidity         */

	uchar reg[1], low[1], high[1];
	int hout;
	float humi;

	reg[0] = 0x28;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x29;
	pwrite(fd, reg, 1, 0);
	pread(fd, high, 1, 0);

	hout = low[0] | (high[0] << 8);

	if(hout > 32767)
		hout -= 65536;

	humi = ((hout - c.h0out) * (c.h1rh -  c.h0rh)) / (c.h1out - c.h0out) + c.h0rh;

	return humi;
}


void
main(int, char*)
{

	struct CalTable c;

  	int fd;

	fd = mountdev();

	initchip(fd);

	getcal(fd, &c);

	print("Temperature is %.1f °C\n", gettemp(fd, c));
	print("Humidity is %.1f %%\n", gethum(fd, c));

	closechip(fd);

	exits(nil);
}