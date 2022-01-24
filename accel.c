/***********************************************/
/* by adventuresin9                            */
/*                                             */
/* This will read the LSM9DS1 9 axis sensor on */
/* the Raspberry Pi Sense Hat and output the   */
/* values from the accelerometer.              */
/***********************************************/



#include <u.h>
#include <libc.h>


int fd;


int
mountchip(void)
{

/* the LSM9DS1 accelerometer and  */
/* gyroscope is on I²C address 6A */

	if(bind("#J6a", "/dev", MBEFORE) < 0)
		sysfatal("no J device");

	fd = open("/dev/i2c.6a.data", ORDWR|ORCLOSE);

	if(fd < 0)
		sysfatal("cant open dev file");

	return(fd);
}


void
initchip(void)
{

	uchar buf[2];

	/* ctrl_reg6_xl, 119Hz refresh */
	buf[0] = 0x20;
	buf[1] = 0x60;
	pwrite(fd, buf, 2, 0);

}


void
closechip(void)
{
	uchar buf[2];

	/* send power down command */
	buf[0] = 0x20;
	buf[1] = 0x00;
	pwrite(fd, buf, 2, 0);

	close(fd);

	unmount("#J6a", "/dev/");
}


int
getax(void)
{

/* X axis */

	uchar reg[1], low[1], high[1];
	int o;

	reg[0] = 0x28;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x29;
	pwrite(fd, reg, 1, 0);
	pread(fd, high, 1, 0);

	o = low[0] | (high[0] << 8);

	if(o > 32757)
		o -= 65536;

	return(o);
}


int
getay(void)
{

/* Y axis */

	uchar reg[1], low[1], high[1];
	int o;

	reg[0] = 0x2A;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x2B;
	pwrite(fd, reg, 1, 0);
	pread(fd, high, 1, 0);

	o = low[0] | (high[0] << 8);

	if(o > 32757)
		o -= 65536;

	return(o);
}


int
getaz(void)
{

/* Z axis */

	uchar reg[1], low[1], high[1];
	int o;

	reg[0] = 0x2C;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x2D;
	pwrite(fd, reg, 1, 0);
	pread(fd, high, 1, 0);

	o = low[0] | (high[0] << 8);

	if(o > 32757)
		o -= 65536;

	return(o);
}


int
handler(void*, char *msg)
{

/* this listens for the DEL key being pressed  */
/* and makes sure the device is closed cleanly */
/* If it is not properly closed, it can lock   */
/* up the I²C interface.                       */

	closechip();
	print("closed %s\n", msg);
	return 0;
}


void
main(int, char*)
{
	fd = -1;

	atnotify(handler, 1);

	fd = mountchip();

	initchip();

	for(;;){
		print("ax\t%d\tay\t%d\taz\t%d\n", getax(), getay(), getaz());
		sleep(100);
	}

	closechip();
	exits(nil);
}