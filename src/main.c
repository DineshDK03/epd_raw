#include <device.h>
#include <drivers/spi.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <sys/printk.h>

#include "GUI_Paint.h"
#include "ImageData.h"
#include <stdlib.h> // malloc() free()

#define SPI_DEV         "SPI_1"

#define BUSY_NODE       DT_ALIAS(busy)

#define BUSY_DEV	DT_GPIO_LABEL(BUSY_NODE, gpios)
#define BUSY_PIN	DT_GPIO_PIN(BUSY_NODE, gpios)
#define BUSY_FLAGS	DT_GPIO_FLAGS(BUSY_NODE, gpios)

#define RST_NODE        DT_ALIAS(rst)

#define RST_DEV	        DT_GPIO_LABEL(RST_NODE, gpios)
#define RST_PIN	        DT_GPIO_PIN(RST_NODE, gpios)
#define RST_FLAGS	DT_GPIO_FLAGS(RST_NODE, gpios)

#define DC_NODE         DT_ALIAS(dc)

#define DC_DEV	        DT_GPIO_LABEL(DC_NODE, gpios)
#define DC_PIN	        DT_GPIO_PIN(DC_NODE, gpios)
#define DC_FLAGS	DT_GPIO_FLAGS(DC_NODE, gpios)

#define CS_NODE         DT_ALIAS(cs)

#define CS_DEV	        DT_GPIO_LABEL(CS_NODE, gpios)
#define CS_PIN	        DT_GPIO_PIN(CS_NODE, gpios)
#define CS_FLAGS	DT_GPIO_FLAGS(CS_NODE, gpios)

#define EPD_2IN7_WIDTH       176
#define EPD_2IN7_HEIGHT      264 //46464

static struct spi_config spi_cfg;
static struct spi_cs_control cs_ctrl;

const struct device *spi_dev;
const struct device *dc;
const struct device *rst;
const struct device *busy;
const struct device *cs;

static const unsigned char EPD_2in7_lut_vcom_dc[] = {
    0x00	,0x00,
    0x00	,0x08	,0x00	,0x00	,0x00	,0x02,
    0x60	,0x28	,0x28	,0x00	,0x00	,0x01,
    0x00	,0x14	,0x00	,0x00	,0x00	,0x01,
    0x00	,0x12	,0x12	,0x00	,0x00	,0x01,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00
};
static const unsigned char EPD_2in7_lut_ww[] = {
    0x40	,0x08	,0x00	,0x00	,0x00	,0x02,
    0x90	,0x28	,0x28	,0x00	,0x00	,0x01,
    0x40	,0x14	,0x00	,0x00	,0x00	,0x01,
    0xA0	,0x12	,0x12	,0x00	,0x00	,0x01,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};
static const unsigned char EPD_2in7_lut_bw[] = {
    0x40	,0x08	,0x00	,0x00	,0x00	,0x02,
    0x90	,0x28	,0x28	,0x00	,0x00	,0x01,
    0x40	,0x14	,0x00	,0x00	,0x00	,0x01,
    0xA0	,0x12	,0x12	,0x00	,0x00	,0x01,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};
static const unsigned char EPD_2in7_lut_bb[] = {
    0x80	,0x08	,0x00	,0x00	,0x00	,0x02,
    0x90	,0x28	,0x28	,0x00	,0x00	,0x01,
    0x80	,0x14	,0x00	,0x00	,0x00	,0x01,
    0x50	,0x12	,0x12	,0x00	,0x00	,0x01,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};
static const unsigned char EPD_2in7_lut_wb[] = {
    0x80	,0x08	,0x00	,0x00	,0x00	,0x02,
    0x90	,0x28	,0x28	,0x00	,0x00	,0x01,
    0x80	,0x14	,0x00	,0x00	,0x00	,0x01,
    0x50	,0x12	,0x12	,0x00	,0x00	,0x01,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
    0x00	,0x00	,0x00	,0x00	,0x00	,0x00,
};

static void send_command(const struct device *dc, const struct device *cs, uint8_t cmd)
{
        dc = device_get_binding(DC_DEV);
        gpio_pin_configure(dc, DC_PIN,
               GPIO_OUTPUT | DC_FLAGS);

        struct spi_buf buffers = {.buf = &cmd, .len = sizeof(cmd)};
        struct spi_buf_set buf_set = {.buffers = &buffers, .count = 1};

        gpio_pin_set(dc, DC_PIN, 0);
        gpio_pin_set(cs, CS_PIN, 0);
        if (spi_write(spi_dev, &spi_cfg, &buf_set)) {
                printk("Error SPI write");
		return;
	}
        gpio_pin_set(cs, CS_PIN, 1);
}

static void send_data(const struct device *dc, const struct device *cs,uint8_t data)
{
        dc = device_get_binding(DC_DEV);
        gpio_pin_configure(dc, DC_PIN,
               GPIO_OUTPUT | DC_FLAGS);

        struct spi_buf buffers = {.buf = &data, .len = sizeof(data)};
        struct spi_buf_set buf_set = {.buffers = &buffers, .count = 1};

        gpio_pin_set(dc, DC_PIN, 1);
        gpio_pin_set(cs, CS_PIN, 0);
        if (spi_write(spi_dev, &spi_cfg, &buf_set)) {
                printk("Error SPI write");
		return;
	}
        gpio_pin_set(cs, CS_PIN, 1);
}

static void read_busy(const struct device *busy)
{
        printk("e-Paper busy\r\n");
        int pin = gpio_pin_get(busy, BUSY_PIN);

        while (pin > 0) {
            __ASSERT(pin >= 0, "Failed to get pin level");
            printk("wait %u", pin);
            k_sleep(K_MSEC(200));
            pin = gpio_pin_get(busy, BUSY_PIN);
        }
        printk("e-Paper busy release\r\n");
}

static void EPD_2in7_SetLut(const struct device *dc, const struct device *cs)
{
    unsigned int count;
    send_command(dc, cs, 0x20); //vcom
    for(count = 0; count < 44; count++) {
        send_data(dc, cs, EPD_2in7_lut_vcom_dc[count]);
    }

    send_command(dc, cs, 0x21); //ww --
    for(count = 0; count < 42; count++) {
        send_data(dc, cs, EPD_2in7_lut_ww[count]);
    }

    send_command(dc, cs, 0x22); //bw r
    for(count = 0; count < 42; count++) {
        send_data(dc, cs, EPD_2in7_lut_bw[count]);
    }

    send_command(dc, cs, 0x23); //wb w
    for(count = 0; count < 42; count++) {
        send_data(dc, cs, EPD_2in7_lut_bb[count]);
    }

    send_command(dc, cs, 0x24); //bb b
    for(count = 0; count < 42; count++) {
        send_data(dc, cs, EPD_2in7_lut_wb[count]);
    }
}

void EPD_2IN7_Display(const struct device *busy, const struct device *dc, const struct device *cs,const uint8_t *Image)
{
    uint16_t Width, Height;
    Width = (EPD_2IN7_WIDTH % 8 == 0)? (EPD_2IN7_WIDTH / 8 ): (EPD_2IN7_WIDTH / 8 + 1);
    Height = EPD_2IN7_HEIGHT;

    send_command(dc, cs, 0x10);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            send_data(dc, cs, 0XFF);
        }
    }

    send_command(dc, cs, 0x13);
    for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            send_data(dc, cs, Image[i + j * Width]);
        }
    }
    send_command(dc, cs, 0x12);
    k_msleep(2);
    read_busy(busy);
}

void epd_init(const struct device *busy, const struct device *dc, const struct device *cs,const struct device *rst)
{
        gpio_pin_set(rst, RST_PIN, 1);
        k_sleep(K_MSEC(10));
        gpio_pin_set(rst, RST_PIN, 0);
        k_sleep(K_MSEC(10));
        read_busy(busy);
        printk("Reset\n");

        send_command(dc, cs, 0x01); // POWER_SETTING
        send_data(dc, cs, 0x03); // VDS_EN, VDG_EN
        send_data(dc, cs, 0x00); // VCOM_HV, VGHL_LV[1], VGHL_LV[0]
        send_data(dc, cs, 0x2b); // VDH
        send_data(dc, cs, 0x2b); // VDL
        send_data(dc, cs, 0x09); // VDHR

        send_command(dc, cs, 0x06);  // BOOSTER_SOFT_START
        send_data(dc, cs, 0x07);
        send_data(dc, cs, 0x07);
        send_data(dc, cs, 0x17);

        // read_busy(busy);
        // // Power optimization
        // send_command(dc, cs, 0xF8);
        // send_data(dc, cs, 0x60);
        // send_data(dc, cs, 0xA5);
        //
        // // Power optimization
        // send_command(dc, cs, 0xF8);
        // send_data(dc, cs, 0x89);
        // send_data(dc, cs, 0xA5);
        //
        // // Power optimization
        // send_command(dc, cs, 0xF8);
        // send_data(dc, cs, 0x90);
        // send_data(dc, cs, 0x00);
        //
        // // Power optimization
        // send_command(dc, cs, 0xF8);
        // send_data(dc, cs, 0x93);
        // send_data(dc, cs, 0x2A);
        //
        // // Power optimization
        // send_command(dc, cs, 0xF8);
        // send_data(dc, cs, 0xA0);
        // send_data(dc, cs, 0xA5);
        //
        // // Power optimization
        // send_command(dc, cs, 0xF8);
        // send_data(dc, cs, 0xA1);
        // send_data(dc, cs, 0x00);
        //
        // // Power optimization
        // send_command(dc, cs, 0xF8);
        // send_data(dc, cs, 0x73);
        // send_data(dc, cs, 0x41);


        send_command(dc, cs, 0x16); // PARTIAL_DISPLAY_REFRESH
        send_data(dc, cs, 0x00);

        send_command(dc, cs, 0x04); // POWER_ON
        read_busy(busy);

        send_command(dc, cs, 0x00); // PANEL_SETTING
        send_data(dc, cs, 0xBF); // KW-BF   KWR-AF    BWROTP 0f

        send_command(dc, cs, 0x61);
        send_data(dc, cs, 0x00);
        send_data(dc, cs, 0xb0);       //176
        send_data(dc, cs, 0x01);
        send_data(dc, cs, 0x08);		//264

        send_command(dc, cs, 0x82);  // VCM_DC_SETTING_REGISTER
        send_data(dc, cs, 0x28);
        read_busy(busy);

        send_command(dc, cs, 0x30); // PLL_CONTROL
        send_data(dc, cs, 0x3A); // 3A 100HZ   29 150Hz 39 200HZ    31 171HZ

        send_command(dc, cs, 0x50);  // VCM_DC_SETTING_REGISTER
        send_data(dc, cs, 0x47);

        EPD_2in7_SetLut(dc, cs);
        printk("EPD init done\n");
}

void epd_clear(const struct device *busy, const struct device *dc, const struct device *cs)
{
        uint16_t Width, Height;
        Width = (EPD_2IN7_WIDTH % 8 == 0)? (EPD_2IN7_WIDTH / 8 ): (EPD_2IN7_WIDTH / 8 + 1);
        Height = EPD_2IN7_HEIGHT;

        send_command(dc, cs, 0x10);
        for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            send_data(dc, cs, 0XFF);
        }
        }

        send_command(dc, cs, 0x13);
        for (uint16_t j = 0; j < Height; j++) {
        for (uint16_t i = 0; i < Width; i++) {
            send_data(dc, cs, 0XFF);
        }
        }

        send_command(dc, cs, 0x12);
        k_msleep(2);
        read_busy(busy);
}

void epd_sleep(const struct device *dc, const struct device *cs)
{
    send_command(dc, cs, 0X50);
    send_data(dc, cs, 0xf7);
    send_command(dc, cs, 0X02);  	//power off
    send_command(dc, cs, 0X07);  	//deep sleep
    send_data(dc, cs, 0xA5);
}

void main(void)
{
	printk("EPD Example\n");
        spi_dev = device_get_binding(SPI_DEV);

        if (spi_dev == NULL) {
                printk("Could not get %s device\n", SPI_DEV);
                return;
        }
        spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8);
        spi_cfg.frequency = 4000000;
        spi_cfg.slave = 0;
        spi_cfg.cs = NULL;

        printk("SPI Initialized\n");

        busy = device_get_binding(BUSY_DEV);
        if (busy == NULL) {
                return;
        }

        rst = device_get_binding(RST_DEV);
        if (rst == NULL) {
                return;
        }

        dc = device_get_binding(DC_DEV);
        if (dc == NULL) {
                return;
        }

        cs = device_get_binding(CS_DEV);
        if (cs == NULL) {
                return;
        }

        gpio_pin_configure(rst, RST_PIN,
               GPIO_OUTPUT_INACTIVE | RST_FLAGS);

        gpio_pin_configure(dc, DC_PIN,
               GPIO_OUTPUT_INACTIVE | DC_FLAGS);

        gpio_pin_configure(busy, BUSY_PIN,
               GPIO_INPUT | BUSY_FLAGS);

        cs_ctrl.gpio_dev = cs;
        if (!cs_ctrl.gpio_dev) {
          printk("Unable to get SPI GPIO CS device");
          return;
        }

        cs_ctrl.gpio_pin = CS_PIN;
        cs_ctrl.gpio_dt_flags = CS_FLAGS;
        cs_ctrl.delay = 0U;
        spi_cfg.cs = &cs_ctrl;
        printk("SPI Setup done\n");

        printk("e-Paper Init and Clear...\r\n");
        epd_init(busy,dc,cs,rst);

        epd_clear(busy,dc,cs);
        k_msleep(500);

        //Create a new image cache
        uint8_t *BlackImage;
        /* you have to edit the startup_stm32fxxx.s file and set a big enough heap size */
        uint16_t Imagesize = ((EPD_2IN7_WIDTH % 8 == 0)? (EPD_2IN7_WIDTH / 8 ): (EPD_2IN7_WIDTH / 8 + 1)) * EPD_2IN7_HEIGHT;
        if((BlackImage = (uint8_t *)k_malloc(Imagesize)) == NULL) {
            printk("Failed to apply for black memory...\r\n");
            return;
        }
        printk("Paint_NewImage\r\n");
        Paint_NewImage(BlackImage, EPD_2IN7_WIDTH, EPD_2IN7_HEIGHT, 270, WHITE);

        printk("show image for array\r\n");
        Paint_SelectImage(BlackImage);
        Paint_Clear(WHITE);
        Paint_DrawBitMap(gImage_2in7);
        EPD_2IN7_Display(busy,dc,cs,BlackImage);
        k_msleep(500);

        //1.Select Image
        printk("SelectImage:BlackImage\r\n");
        Paint_SelectImage(BlackImage);
        Paint_Clear(WHITE);

        // 2.Drawing on the image
        printk("Drawing:BlackImage\r\n");
        Paint_DrawPoint(10, 80, BLACK, DOT_PIXEL_1X1, DOT_STYLE_DFT);
        Paint_DrawPoint(10, 90, BLACK, DOT_PIXEL_2X2, DOT_STYLE_DFT);
        Paint_DrawPoint(10, 100, BLACK, DOT_PIXEL_3X3, DOT_STYLE_DFT);
        Paint_DrawLine(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawLine(70, 70, 20, 120, BLACK, DOT_PIXEL_1X1, LINE_STYLE_SOLID);
        Paint_DrawRectangle(20, 70, 70, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        Paint_DrawRectangle(80, 70, 130, 120, BLACK, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawCircle(45, 95, 20, BLACK, DOT_PIXEL_1X1, DRAW_FILL_EMPTY);
        Paint_DrawCircle(105, 95, 20, WHITE, DOT_PIXEL_1X1, DRAW_FILL_FULL);
        Paint_DrawLine(85, 95, 125, 95, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
        Paint_DrawLine(105, 75, 105, 115, BLACK, DOT_PIXEL_1X1, LINE_STYLE_DOTTED);
        Paint_DrawString_EN(10, 0, "waveshare", &Font16, BLACK, WHITE);
        Paint_DrawString_EN(10, 20, "hello world", &Font12, WHITE, BLACK);
        Paint_DrawNum(10, 33, 123456789, &Font12, BLACK, WHITE);
        Paint_DrawNum(10, 50, 987654321, &Font16, WHITE, BLACK);
        Paint_DrawString_CN(130, 0,"����abc", &Font12CN, BLACK, WHITE);
        Paint_DrawString_CN(130, 20, "΢ѩ����", &Font24CN, WHITE, BLACK);

        printk("EPD_Display\r\n");
        EPD_2IN7_Display(busy,dc,cs,BlackImage);
        k_msleep(2000);

        printk("Clear...\r\n");
        epd_clear(busy,dc,cs);

        printk("Goto Sleep...\r\n");
        epd_sleep(dc,cs);

        k_free(BlackImage);
        BlackImage = NULL;

}
