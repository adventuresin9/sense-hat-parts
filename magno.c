/***********************************************/
/* by adventuresin9                            */
/*                                             */
/* This will read the LSM9DS1 9 axis sensor on */
/* the Raspberry Pi Sense Hat and output the   */
/* values from the magnetometer.               */
/***********************************************/


#include <u.h>
#include <libc.h>


int fd;


int
mountchip(void)
{

/* the LSM9DS1 magnetometer is on I²C address 1C */

	if(bind("#J1c", "/dev", MBEFORE) < 0)
		sysfatal("no J device");

	fd = open("/dev/i2c.1c.data", ORDWR);

	if(fd < 0)
		sysfatal("cant open dev file");

	return(fd);
}


void
initchip(void)
{

	uchar buf[2];

	/* ctrl_reg1_m, high performance, 10Hz */
	buf[0] = 0x20;
	buf[1] = 0x50;
	pwrite(fd, buf, 2, 0);

	/* ctrl_reg2_m, default scale */
	buf[0] = 0x21;
	buf[1] = 0x00;
	pwrite(fd, buf, 2, 0);

	/* ctrl_reg3_m, power on, continuous mode */
	buf[0] = 0x22;
	buf[1] = 0x00;
	pwrite(fd, buf, 2, 0);

	/* ctrl_reg4_m, high performance Z axis */
	buf[0] = 0x23;
	buf[1] = 0x08;
	pwrite(fd, buf, 2, 0);

}


void
closechip(void)
{
	uchar buf[2];

/* send power down command */
	buf[0] = 0x22;
	buf[1] = 0x03;
	pwrite(fd, buf, 2, 0);

	close(fd);

	unmount("#J1c", "/dev");
}


int
getmx(void)
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

	if(o > 32767)
		o -= 65536;

	return(o);
}

int
getmy(void)
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

	if(o > 32767)
		o -= 65536;

	return(o);
}

int
getmz(void)
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

	if(o > 32767)
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
main(int argc, char *argv[])
{
	fd = -1;

	atnotify(handler, 1);

	fd = mountchip();

	initchip();

	for(;;){
		print("mx\t%d\tmy\t%d\tmz\t%d\n", getmx(), getmy(), getmz());
		sleep(100);
	}

	closechip();
	exits(nil);
}