/*

MIT License

Copyright (c) Avnet Corporation. All rights reserved.
Author: Rickey Costillo

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef OLED_H
#define OLED_H

typedef enum { SHOW_LOGO = 0, SHOW_BBQ_STATUS, SHOW_OTA_UPDATE_PENDING } Oled_States;

#include <stdio.h>
#include <stdint.h>
#include "sd1306.h"
#include "applibs_versions.h"
#include "dx_i2c.h"

#define OLED_NUM_SCREEN 4 - 1

#define OLED_TITLE_X 0
#define OLED_TITLE_Y 0
#define OLED_RECT_TITLE_X 0
#define OLED_RECT_TITLE_Y 0
#define OLED_RECT_TITLE_W 127
#define OLED_RECT_TITLE_H 18

#define OLED_LINE_1_X 0
#define OLED_LINE_1_Y 16

#define OLED_LINE_2_X 0
#define OLED_LINE_2_Y 26

#define OLED_LINE_3_X 0
#define OLED_LINE_3_Y 36

#define OLED_LINE_4_X 0
#define OLED_LINE_4_Y 46

#define FONT_SIZE_TITLE 2
#define FONT_SIZE_LINE 1

#define SSID_MAX_LEGTH 15

uint8_t oled_init(DX_I2C_BINDING *oledBinding);
void oled_update(DX_I2C_BINDING *oledBinding, float targetTemp, float bbqTemp, Oled_States newState);
void oled_draw_logo(DX_I2C_BINDING *oledBinding);

#endif
