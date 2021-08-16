#include <device.h>
#include <drivers/display.h>
#include <stdio.h>
#include <string.h>
#include <zephyr.h>
#include <sys/printk.h>

#include "image_data.h"

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(app);

#define EPD_2in7_WIDTH		264
#define EPD_2in7_HEIGHT		176
#define EPD_2in7_BUF_SIZE	(EPD_2in7_WIDTH * EPD_2in7_HEIGHT / 8)

void main(void)
{
	const struct device *display_dev;

	display_dev = device_get_binding("EK79652");

	if (display_dev == NULL) {
		LOG_ERR("device not found.  Aborting test.");
		return;
	}

	struct display_buffer_descriptor desc = {
		.buf_size = EPD_2in7_BUF_SIZE,
		.height = EPD_2in7_HEIGHT,
		.width = EPD_2in7_WIDTH,
		.pitch = EPD_2in7_WIDTH
	};

        /*display_write(display_dev, 0, 0, &desc, &imagedata);*/

	display_write(display_dev, 0, 0, &desc, &BitmapExample1);

	/*display_write(display_dev, 0, 0, &desc, &BitmapExample2);*/

	/* display_write(display_dev, 0, 0, &desc, &BitmapExample3); */

	/* display_write(display_dev, 0, 0, &desc, &BitmapExample4); */

	/* display_write(display_dev, 0, 0, &desc, &BitmapExample5); */

	display_blanking_off(display_dev);

	printk("Image printed on display connected to %s\n", CONFIG_BOARD);
}
