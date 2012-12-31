#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <bcm2835.h>

#define CE_PIN RPI_GPIO_P1_15 
#define IRQ_PIN RPI_GPIO_P1_13

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

static const char *device = "/dev/spidev0.0";
static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 1000000;
static uint16_t delay;

static void pabort(const char *s)
{
	perror(s);
	abort();
}


static void transfer(int fd, uint8_t *rxBuf, uint8_t * txBuf, uint32_t len)
{
	int ret;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)txBuf,
		.rx_buf = (unsigned long)rxBuf,
		.len = len,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");

	for (ret = 0; ret < len; ret++) {
		if (!(ret % 6))
			puts("");
		printf("%.2X ", rxBuf[ret]);
	}
	puts("");
}


int main(int argc, char **argv)
{
	int ret = 0;
	int fd;
	
	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	// CE to on
	if (!bcm2835_init())
		return 1;
	
	bcm2835_gpio_fsel(CE_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(IRQ_PIN, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_write(CE_PIN,LOW);	
	for (int i = 0; i < 0xffffff; i++)
	{
		// wait
	}


	uint8_t txBuf[12] = {0};
	uint8_t rxBuf[12] = {0};
	
	
	// set data rate
	txBuf[0] = 0x26;
	txBuf[1] = 0x06;
	transfer(fd, rxBuf, txBuf, 2);
	
	//reset clear
	txBuf[0] = 0x20;
	txBuf[1] = 0x70;
	transfer(fd, rxBuf, txBuf, 2);

	//flush buffers
	txBuf[0] = 0xe1;
	transfer(fd, rxBuf, txBuf,1);

	txBuf[0] = 0xe2;
	transfer(fd, rxBuf, txBuf,1);
	
	// disable shockBurst
	txBuf[0] = 0x21;
	txBuf[1] = 0x00;
	transfer(fd, rxBuf, txBuf, 2);

	

	//set rx address
	txBuf[0] = 0x2B;
	txBuf[1] = 0xE7;
	txBuf[2] = 0xE7;
	txBuf[3] = 0xE7;
	txBuf[4] = 0xE7;
	txBuf[5] = 0xE7;
	transfer(fd, rxBuf, txBuf, 6);
	
	// set payload length
	txBuf[0] = 0x32;
	txBuf[1] = 0x04;
	transfer(fd, rxBuf, txBuf, 2);
	
	// enable pipes 0 and 1
	txBuf[0] = 0x22;
	txBuf[1] = 0x03;
	transfer(fd, rxBuf, txBuf, 2);



	// turn PWR_UP to on
	// PRIM_RX to on
	txBuf[0] = 0x20;
	txBuf[1] = 0x0B;
	transfer(fd, rxBuf,txBuf,2);
	
	//reset clear
	txBuf[0] = 0x27;
	txBuf[1] = 0x70;
	transfer(fd, rxBuf, txBuf, 2);

	//flush buffers
	txBuf[0] = 0xe1;
	transfer(fd, rxBuf, txBuf,1);

	txBuf[0] = 0xe2;
	transfer(fd, rxBuf, txBuf,1);
	


	bcm2835_gpio_write(CE_PIN, HIGH);
	for (int i = 0; i < 0xffffff; i++)
	{
		// wait
	}

	// RADIO Now Configured configured

	close(fd);
	return ret;
}
