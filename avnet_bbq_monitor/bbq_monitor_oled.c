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

#include "bbq_monitor_oled.h"
#include <math.h>

// Forward declarations
void oled_draw_bbq_status(DX_I2C_BINDING *oledBinding, int targetTemp, float bbqTemp);

// AVNET logo
const unsigned char Image_avnet_bmp[BUFFER_SIZE] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   128, 240, 240, 240, 240, 48,  0,   0,   112, 240, 240, 240, 224, 0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   112, 240, 240, 240, 192, 0,   0,   0,   0,   0,   0,   0,   0,   0,   224, 240, 240, 240, 16,  0,   0,   0,   0,   0,   0,   0,   0,   240,
    240, 240, 240, 224, 128, 0,   0,   0,   0,   0,   0,   0,   0,   240, 240, 240, 240, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   240, 240, 240, 240, 112, 112, 112, 112,
    112, 112, 112, 112, 112, 112, 112, 0,   0,   0,   0,   0,   0,   0,   0,   112, 112, 112, 112, 112, 112, 112, 240, 240, 240, 240, 112, 112, 112, 112, 112, 112, 0,   0,   0,
    0,   0,   0,   0,   224, 252, 255, 255, 255, 15,  1,   0,   0,   0,   0,   3,   15,  127, 255, 255, 248, 128, 0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   31,  255, 255,
    254, 240, 0,   0,   0,   0,   224, 248, 255, 255, 127, 7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   255, 255, 255, 255, 15,  31,  127, 252, 248, 224, 224, 128, 0,
    0,   255, 255, 255, 255, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   255, 255, 255, 255, 224, 224, 224, 224, 224, 224, 224, 224, 224, 224, 0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   255, 255, 255, 255, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   128, 240, 254, 255, 127, 15,  1,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   3,   31,  255, 255, 252, 224, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   63,  255, 255, 248, 240, 254, 255, 255, 31,  3,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   255, 255, 255, 255, 0,   0,   0,   1,   3,   15,  15,  63,  126, 252, 255, 255, 255, 255, 0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   255, 255, 255, 255, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 128, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   255, 255,
    255, 255, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   7,   7,   3,   0,   0,   0,   12,  14,  14,  14,  14,  14,  14,  14,  14,  12,  0,   0,   0,   7,   7,   7,
    7,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   7,   7,   7,   7,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   7,
    7,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   3,   7,   7,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   7,   7,   7,   7,   7,   7,   7,   7,
    7,   7,   7,   7,   7,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   7,   7,   7,   7,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0};

/**
 * @brief  OLED initialization.
 * @param  None.
 * @retval Positive if was unsuccefully, zero if was succefully.
 */
uint8_t oled_init(DX_I2C_BINDING *oledBinding)
{
    return sd1306_init(oledBinding);
}

// State machine to change the OLED status
void oled_update(DX_I2C_BINDING *oledBinding, int targetTemp, float bbqTemp, Oled_States currentState)
{
    switch (currentState) {
    case SHOW_LOGO:
        oled_draw_logo(oledBinding);
        break;

    case SHOW_BBQ_STATUS:
        oled_draw_bbq_status(oledBinding, targetTemp, bbqTemp);
        break;
    case SHOW_OTA_UPDATE_PENDING:
        // TODO

        break;

    default:
        break;
    }
}

/**
 * @brief  Display the Avnet Logo
 * @param  None.
 * @retval None.
 */
void oled_draw_logo(DX_I2C_BINDING *oledBinding)
{
    // Copy image_avnet to OLED buffer
    sd1306_draw_img(Image_avnet_bmp);

    // Send the buffer to OLED RAM
    sd1306_refresh(oledBinding);
}

/**
 * @brief  Display current temperature and targets
 * @param  None
 * @retval None.
 */
void oled_draw_bbq_status(DX_I2C_BINDING *oledBinding, int targetTemp, float bbqTemp)

{
    // Strings for labels
    uint8_t str_title[] = "BBQ Temps";
    uint8_t str_targetTemp[] = "Target: ";
    uint8_t str_currentTemp[] = "Current:";
    uint8_t aux_float_data_str[] = "%.2f";

    // Clear oled buffer
    clear_oled_buffer();

    // Draw the title
    sd1306_draw_string(OLED_TITLE_X, OLED_TITLE_Y, str_title, FONT_SIZE_TITLE, white_pixel);

    // Draw a label at line 2
    sd1306_draw_string(OLED_LINE_2_X, OLED_LINE_2_Y, str_targetTemp, FONT_SIZE_LINE, white_pixel);

    // Draw target temperature
    // Convert temperature value to string
    ftoa((float)targetTemp, aux_float_data_str, 2);
    // Draw the temperature string
    sd1306_draw_string(get_str_size(str_targetTemp) * 6, OLED_LINE_2_Y, aux_float_data_str, FONT_SIZE_LINE, white_pixel);
    // Draw the units
    sd1306_draw_string(get_str_size(str_targetTemp) * 6 + (get_str_size(aux_float_data_str) + 1) * 6, OLED_LINE_2_Y, "F", FONT_SIZE_LINE, white_pixel);

    // Draw a label at line 3
    sd1306_draw_string(OLED_LINE_3_X, OLED_LINE_3_Y, str_currentTemp, FONT_SIZE_LINE, white_pixel);

    // Convert temperature value to string
    ftoa(bbqTemp, aux_float_data_str, 2);

    // Draw BBQ temperature value
    sd1306_draw_string(get_str_size(str_currentTemp) * 6, OLED_LINE_3_Y, aux_float_data_str, FONT_SIZE_LINE, white_pixel);
    // Draw the units
    sd1306_draw_string(get_str_size(str_currentTemp) * 6 + (get_str_size(aux_float_data_str) + 1) * 6, OLED_LINE_3_Y, "F", FONT_SIZE_LINE, white_pixel);

    // Send the buffer to OLED RAM
    sd1306_refresh(oledBinding);
}
