#pragma once
#include "sdkconfig.h"

#define LCD_HOST (SPI2_HOST)
#define BOARD_POWERON (gpio_num_t)(14)

#define BOARD_SPI_MISO (21)
#define BOARD_SPI_MOSI (19)
#define BOARD_SPI_SCK (18)
#define BOARD_TFT_CS (5)
#define BOARD_TFT_RST (23)
#define BOARD_TFT_DC (16)
#define BOARD_TFT_BL (4)

#define AMOLED_WIDTH (135)
#define AMOLED_HEIGHT (240)

#define BOARD_HAS_TOUCH 0

#define DISPLAY_BUFFER_SIZE (AMOLED_WIDTH * 100)

#define DISPLAY_FULLRESH false
#define LCD_SWAP_XY (false)
#define LCD_MIRROR_X (false)
#define LCD_MIRROR_Y (false)

#if CONFIG_MIKES_WAY
#define X_OFFSET (53)
#define Y_OFFSET (40)
#else
#define X_OFFSET (52)
#define Y_OFFSET (40)
#endif