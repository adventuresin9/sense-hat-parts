/***********************************************/
/* by adventuresin9                            */
/*                                             */
/* This will read the LSM9DS1 9 axis sensor on */
/* the Raspberry Pi Sense Hat and output the   */
/* values from the gyroscope.                  */
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

	/* ctrl_reg4, 119Hz, 500pdi, default BW */
	buf[0] = 0x10;
	buf[1] = 0x68;
	pwrite(fd, buf, 2, 0);

	/* ctrl_reg4, enable gyro x, y, z */
	buf[0] = 0x1E;
	buf[1] = 0x38;
	pwrite(fd, buf, 2, 0);

}


void
closechip(void)
{
	uchar buf[2];

	/* send power down command */
	buf[0] = 0x10;
	buf[1] = 0x00;
	pwrite(fd, buf, 2, 0);

	close(fd);

	unmount("#J6a", "/dev/");
}


int
getgx(void)
{

/* X axis */

	uchar reg[1], low[1], high[1];
	int o;

	reg[0] = 0x18;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x19;
	pwrite(fd, reg, 1, 0);
	pread(fd, high, 1, 0);

	o = low[0] | (high[0] << 8);

	if(o > 32757)
		o -= 65536;

	return(o);
}


int
getgy(void)
{

/* Y axis */

	uchar reg[1], low[1], high[1];
	int o;

	reg[0] = 0x1A;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x1B;
	pwrite(fd, reg, 1, 0);
	pread(fd, high, 1, 0);

	o = low[0] | (high[0] << 8);

	if(o > 32757)
		o -= 65536;

	return(o);
}


int
getgz(void)
{

/* Z axis */

	uchar reg[1], low[1], high[1];
	int o;

	reg[0] = 0x1C;
	pwrite(fd, reg, 1, 0);
	pread(fd, low, 1, 0);

	reg[0] = 0x1D;
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
		print("gx\t%d\tgy\t%d\tgz\t%d\n", getgx(), getgy(), getgz());
		sleep(100);
	}

	closechip();
	exits(nil);
}