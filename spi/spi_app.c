#include <stdio.h>
#include <unistd.h> /* read, write */
#include <fcntl.h> /*open, close */
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

int lcd_fd = -1;
int fd_dc = -1;
int fd_rst = -1;
int touch_fd = -1;

/* Helper to write '0' or '1' to a GPIO file descriptor */
void gpio_write(int fd, int value)
{
    lseek(fd, 0, SEEK_SET); // RESET THE FILE CURSOR FIRST!
    if (value)
    {
        write(fd, "1", 1);
    }
    else
    {
        write(fd, "0", 1);
    }
}

/* Control function for DC and RST */
void set_dc_command()
{
    gpio_write(fd_dc, 0);
}

void set_dc_data()
{
    gpio_write(fd_dc, 1);
}

void set_rst_low()
{
    gpio_write(fd_rst, 0);
}

void set_rst_high()
{
    gpio_write(fd_rst, 1);
}

/* Transmit a single byte over SPI */
void spi_write_byte(uint8_t byte)
{
    struct spi_ioc_transfer tr = 
    {
        .tx_buf = (unsigned long)&byte,
        .len = 1,
        .speed_hz = 24000000,
        .bits_per_word = 8,
    };
    ioctl(lcd_fd, SPI_IOC_MESSAGE(1), &tr);
}

void lcd_command(uint8_t cmd)
{
    set_dc_command();
    spi_write_byte(cmd);
}

void lcd_data(uint8_t data)
{
    set_dc_data();
    spi_write_byte(data);
}

int lcd_init(const char *device) {
    lcd_fd = open(device, O_RDWR);
    if (lcd_fd < 0) return -1;

    uint8_t mode = SPI_MODE_0;         // CPOL=0, CPHA=0 (Standard for ST7789)
    uint8_t bits = 8;                  // 8 bits per word
    uint32_t speed = 24000000;         // 24 MHz (ST7789 handles up to ~32MHz)

    if (ioctl(lcd_fd, SPI_IOC_WR_MODE, &mode) < 0) return -1;
    if (ioctl(lcd_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) return -1;
    if (ioctl(lcd_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) return -1;

    return 0;
}

int touch_init(const char *device) {
    touch_fd = open(device, O_RDWR);
    if (touch_fd < 0) return -1;

    uint8_t mode = SPI_MODE_0;         // CPOL=0, CPHA=0 (Standard for ST7789)
    uint8_t bits = 8;                  // 8 bits per word
    uint32_t speed = 2000000;         // 24 MHz (ST7789 handles up to ~32MHz)

    if (ioctl(touch_fd, SPI_IOC_WR_MODE, &mode) < 0) return -1;
    if (ioctl(touch_fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) return -1;
    if (ioctl(touch_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) return -1;

    return 0;
}

uint16_t xpt2046_read_channel(uint8_t command_byte) {
    uint8_t tx[3] = { command_byte, 0x00, 0x00 };
    uint8_t rx[3] = { 0x00, 0x00, 0x00 };

    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = 3,
        .speed_hz = 4000000, // XPT2046 is slower than the LCD; keep it around 1-2 MHz
        .bits_per_word = 8,
    };

    // Execute full duplex transfer (Send command, read response simultaneously)
    if (ioctl(touch_fd, SPI_IOC_MESSAGE(1), &tr) < 0) {
        return 0; 
    }

    // The 12-bit result is split across the last two bytes received
    // Format: rx[1] contains the high bits, rx[2] contains the low bits
    uint16_t raw_adc = ((rx[1] << 8) | rx[2]) >> 3; // Shift to align 12-bit structure
    
    return raw_adc & 0x0FFF; // Return clean 12-bit integer (0 - 4095)
}

int main()
{
    /* Open SPI */
    /* Init SPI for LCD */
    lcd_init("/dev/spidev0.1");
    touch_init("/dev/spidev0.0");

    /* Open GPIO */
    fd_dc = open("/sys/class/gpio/gpio49/value", O_WRONLY);
    fd_rst = open("/sys/class/gpio/gpio15/value", O_WRONLY);
    if (lcd_fd < 0 || fd_dc < 0 || fd_rst < 0 || touch_fd < 0)
    {
        perror("Failed to open one or more hardware descriptors");
        return 1;
    }
    printf("Starting ST7789 initialization\n");

    // /*1. Physical hardware Reset */
    // set_rst_high();
    // usleep(10000);      /*sleep for 10 ms*/
    // set_rst_low();
    // usleep(20000);     /*pull down 20 ms*/
    // set_rst_high();
    // usleep(120000);     /*stable delay */

    // /*2. Software Reset */
    // /*set internal registers to their default state*/
    // lcd_command(0x01);
    // usleep(150000);

    // /*3. Wake up*/
    // /*Wake up charge pumps and oscillators*/
    // lcd_command(0x11);
    // usleep(120000);

    // /*4. Interface Pixel Format*/
    // /*0x3A COLMOD*/
    // /*0x55 use RGB565*/
    // lcd_command(0x3A);
    // lcd_data(0x55);

    // /*5. Memory Access Control*/
    // /*MADCTL*/
    // /*Top to bottom, Left to right layout*/
    // lcd_command(0x36);
    // lcd_data(0x00);

    // /*6. Turn display on*/
    // lcd_command(0x20);  /*Normal display mode*/
    // lcd_command(0x29);  /*Main screen ON*/
    // usleep(20000);

    // printf("Initialization completed\n");
    
    // /*7. set data to display RAM*/
    // lcd_command(0x2c);
    
    while(1)
    {
        uint16_t raw_adc = xpt2046_read_channel(0x90);
        printf("ADC of x cordinate: %u\n", raw_adc);
        raw_adc = xpt2046_read_channel(0xD0);
        printf("ADC of y cordinate: %u\n", raw_adc);
        sleep(1);
    }
    
    close(fd_rst);
    close(fd_dc);
    close(lcd_fd);
    printf("Program closed\n");
    return 0;
}
