/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include <string.h>
#include <stdlib.h>  // abs()
#include "app/chFrScanner.h"
#include "app/dtmf.h"
#include "am_fix.h"
#include "airband_config.h"
#include "bitmaps.h"
#include "board.h"
#include "driver/bk4819.h"
#include "driver/st7565.h"
#include "driver/uart.h"
#include "external/printf/printf.h"
#include "functions.h"
#include "helper/battery.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/graphics.h"
#include "ui/inputbox.h"
#include "ui/main.h"
#include "ui/ui.h"
#include "driver/system.h"

center_line_t center_line = CENTER_LINE_NONE;

bool isMainOnlyInputDTMF = false;

// проверка отображаем экран с одной частотой или двумя
static bool isMainOnly(bool checkGui)
{
    if(( gEeprom.DUAL_WATCH == DUAL_WATCH_OFF && gEeprom.CROSS_BAND_RX_TX == CROSS_BAND_OFF ) || (gSetting_set_gui && checkGui))
        return true;
    else
        return false;
}

const int8_t dBmCorrTable[7] = {
            -15, // band 1
            -25, // band 2
            -20, // band 3
            -4, // band 4
            -7, // band 5
            -6, // band 6
             -1  // band 7
        };

const char *VfoStateStr[] = {
       [VFO_STATE_NORMAL]="",
       [VFO_STATE_BUSY]="BUSY",
       [VFO_STATE_BAT_LOW]="BAT LOW",
       [VFO_STATE_TX_DISABLE]="TX DISABLE",
       [VFO_STATE_TIMEOUT]="TIMEOUT",
       [VFO_STATE_ALARM]="ALARM",
       [VFO_STATE_VOLTAGE_HIGH]="VOLT HIGH"
};

int Rssi2DBm(uint16_t rssi) { return (rssi >> 1) - 160; }

static int16_t map(int16_t x, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

uint8_t DBm2S(int dbm) {
    if(dbm < -141) dbm = -141;
    if(dbm > -53) dbm = -53;

    uint8_t s_level = 0;

    if(dbm <= -93) {
        s_level = map(dbm, -141, -93, 1, 9);
    } else {
        s_level = map(dbm, -93, -53, 9, 49);
    }
    return s_level;
}

// отрисовка строки с rssi баром
void UI_RSSIBar(uint16_t rssi, uint8_t y) {
    if (rssi == 0)
        return;
    const uint8_t BAR_LEFT_MARGIN = 45;
    const uint8_t BAR_BASE = y + 7;

    int dBm = Rssi2DBm(rssi)
        + ((gSetting_AM_fix && gRxVfo->Modulation == MODULATION_AM) ? AM_fix_get_gain_diff() : 0)
        + dBmCorrTable[gRxVfo->Band];

    uint8_t s = DBm2S(dBm);

    FillRect(0, y, LCD_WIDTH, 8, C_CLEAR);

    PrintMedium(0, BAR_BASE, "%d", dBm);
    if (s < 10) {
        PrintMedium(26, BAR_BASE, "S%u", s);
    } else {
        PrintMedium(26, BAR_BASE, "+%u", s - 9);
    }
    const uint8_t bar = (s<=9)? s : (9+(s-9)/10);
    for (uint8_t i = 0; i < bar; ++i) {
        FillRect(BAR_LEFT_MARGIN + i * 6, y + 1, 5, 6, C_FILL);
    }

}

void UI_MAIN_TimeSlice500ms(void)
{
    if (FUNCTION_IsRx()) {
        UI_RSSIBar(BK4819_GetRSSI(), LCD_HEIGHT - 16);
    }

    ST7565_BlitFullScreen();
}

static void PrintDCS(uint8_t x, uint8_t y, TextPos posLCR,  const FREQ_Config_t *pConfig) {
    switch((int)pConfig->CodeType)
    {
        case CODE_TYPE_CONTINUOUS_TONE:
        PrintSmallEx(x, y, posLCR, C_FILL, "CT%u.%u", CTCSS_Options[pConfig->Code] / 10, CTCSS_Options[pConfig->Code] % 10);
        break;

        case CODE_TYPE_DIGITAL:
        PrintSmallEx(x, y, posLCR, C_FILL, "DC%03oN", DCS_Options[pConfig->Code]);
        break;

        case CODE_TYPE_REVERSE_DIGITAL:
        PrintSmallEx(x, y, posLCR, C_FILL,"DC%03oI", DCS_Options[pConfig->Code]);
        break;
    }
}

static void DisplaySingleFreq(uint8_t activeVFO) {
    const VFO_Info_t *vfoInfo = &gEeprom.VfoInfo[activeVFO];
    const uint8_t screenChannel = gEeprom.ScreenChannel[activeVFO];
    const FREQ_Config_t *pConfig = FUNCTION_IsRx() ? vfoInfo->pRX : vfoInfo->pRX;
    uint32_t frequency = pConfig->Frequency;

    // Line 1: badge + RX indicator — only in non-scan single mode.
    // During scan the status line already shows "SCAN", so no badge is needed.
    if (gScanStateDir == SCAN_OFF) {
        FillRect(0, 0, 27, 9, C_FILL);
        if (IS_MR_CHANNEL(screenChannel)) {
            PrintMediumEx(1, 7, POS_L, C_INVERT, "M%03u", screenChannel + 1);
        } else {
            PrintMediumEx(1, 7, POS_L, C_INVERT, "VFO");
        }

        if (FUNCTION_IsRx()) {
            FillRect(29, 0, 14, 9, C_FILL);
            PrintMediumEx(30, 7, POS_L, C_INVERT, "RX");
        }
    }

    // Line 2: channel name (large, centered). Baseline y=22 keeps the 7px
    // glyphs inside page 2 (y 16-22), clear of the frequency (starts y=25).
    char chName[22] = "";
    if (IS_MR_CHANNEL(screenChannel))
        SETTINGS_FetchChannelName(chName, screenChannel);
    if (chName[0] != 0) {
        PrintMediumBoldEx(LCD_WIDTH / 2, 22, POS_C, C_FILL, "%s", chName);
    }

    // Line 3: frequency (biggest font, centered, 3 decimals)
    PrintBiggestDigitsEx(LCD_WIDTH / 2, 38, POS_C, C_FILL,
        "%u.%03u", frequency / 100000, frequency / 100 % 1000);

    // Line 4: S-meter bar
    if (FUNCTION_IsRx()) {
        UI_RSSIBar(BK4819_GetRSSI(), LCD_HEIGHT - 16);
    }

    // Bottom: AM + SQL (AGC lives on the status line, page 0)
    DrawHLine(0, LCD_HEIGHT - 8, LCD_WIDTH, C_FILL);
    PrintSmall(1, LCD_HEIGHT - 2, "%s", gModulationStr[vfoInfo->Modulation]);
    PrintSmall(20, LCD_HEIGHT - 2, "SQL%d", gEeprom.SQUELCH_LEVEL);
}

static void DisplayDualVfo(uint8_t activeVFO) {
    const VFO_Info_t *vfoInfo = &gEeprom.VfoInfo[activeVFO];
    const uint8_t screenChannel = gEeprom.ScreenChannel[activeVFO];
    const ModulationMode_t mod = vfoInfo->Modulation;
    const enum Vfo_txtr_mode mode = FUNCTION_IsTx() ? VFO_MODE_TX : ( FUNCTION_IsRx() ? VFO_MODE_RX : VFO_MODE_NONE );
    const FREQ_Config_t *pConfig = (mode == VFO_MODE_TX) ? vfoInfo->pTX : vfoInfo->pRX;
    uint32_t frequency = pConfig->Frequency;

    // Row 1 (y=8): badge + RX + step
    FillRect(0, 0, 27, 9, C_FILL);
    if (gScanStateDir != SCAN_OFF) {
        PrintMediumEx(1, 7, POS_L, C_INVERT, FUNCTION_IsRx() ? "SC\x7f" : "SC>");
    } else if (IS_MR_CHANNEL(screenChannel)) {
        const bool inputting = gInputBoxIndex != 0 && gEeprom.TX_VFO == activeVFO;
        if (!inputting)
            PrintMediumEx(1, 7, POS_L, C_INVERT, "M%03u", screenChannel + 1);
        else
            PrintMediumEx(1, 7, POS_L, C_INVERT, "M%.3s", INPUTBOX_GetAscii());
    } else if (IS_FREQ_CHANNEL(screenChannel)) {
        const uint8_t fchannel = 1 + screenChannel - FREQ_CHANNEL_FIRST;
        const char *vfoIndexNames[] = {"A", "B"};
        PrintMediumEx(1, 7, POS_L, C_INVERT, "V%s%u", vfoIndexNames[activeVFO], fchannel);
    }

    if (mode == VFO_MODE_RX || mode == VFO_MODE_TX) {
        FillRect(29, 0, 14, 9, C_FILL);
        PrintMediumEx(30, 7, POS_L, C_INVERT, mode == VFO_MODE_TX ? "TX" : "RX");
    }
    PrintMediumEx(LCD_WIDTH - 1, 7, POS_R, C_FILL, "%d.%02u", vfoInfo->StepFrequency / 100, vfoInfo->StepFrequency % 100);

    // Row 2 (y=16): SQL only (AGC lives in the bottom bar, page 7).
    // Hidden during airband scan so it can't overlap the SCAN badge.
    if (gScanStateDir == SCAN_OFF) {
        if (gMonitor) {
            PrintMediumEx(1, 16, POS_L, C_FILL, "MONI");
        } else {
            PrintMediumEx(1, 16, POS_L, C_FILL, "SQL%d", gEeprom.SQUELCH_LEVEL);
        }
    }

    // Row 3: Channel name (if MR). 8px label during scan so it never
    // overlaps the big frequency digits drawn below at y=38.
    if (IS_MR_CHANNEL(screenChannel)) {
        char string[22];
        SETTINGS_FetchChannelName(string, screenChannel);
        if (string[0] != 0) {
            if (gScanStateDir != SCAN_OFF)
                PrintSmallEx(LCD_WIDTH / 2, 24, POS_C, C_FILL, "%s", string);
            else
                PrintMediumBoldEx(LCD_WIDTH / 2, 26, POS_C, C_FILL, "%s", string);
        }
    }

    // Row 4 (y=38): Frequency (big, centered)
    if (gInputBoxIndex > 0 && IS_FREQ_CHANNEL(screenChannel)) {
        const char * ascii = INPUTBOX_GetAscii();
        bool isGigaF = frequency >= _1GHz_in_KHz;
        PrintBiggestDigitsEx(LCD_WIDTH / 2, 38, POS_C, C_FILL,
            "%.*s.%.3s", 3 + isGigaF, ascii, ascii + 3 + isGigaF);
    } else {
        PrintBiggestDigitsEx(LCD_WIDTH / 2, 38, POS_C, C_FILL,
            "%u.%03u", frequency / 100000, frequency / 100 % 1000);
    }

    // Row 5 (y=48): S-meter
    if (FUNCTION_IsRx()) {
        UI_RSSIBar(BK4819_GetRSSI(), LCD_HEIGHT - 16);
    }

    // Bottom bar (page 7): AM + BW + SQL (AGC is on the status line, page 0)
    DrawHLine(0, LCD_HEIGHT - 8, LCD_WIDTH, C_FILL);
    PrintSmall(1, LCD_HEIGHT - 2, "%s", gModulationStr[mod]);
    const char *bandWidthNames[] = {"W", "N"};
    PrintSmall(20, LCD_HEIGHT - 2, bandWidthNames[vfoInfo->CHANNEL_BANDWIDTH]);
    PrintSmall(30, LCD_HEIGHT - 2, "SQL%d", gEeprom.SQUELCH_LEVEL);

}

static void printVfoBlock(uint8_t base, uint8_t vfoNum, bool isActive) {
    const VFO_Info_t *vfoInfo = &gEeprom.VfoInfo[vfoNum];
    const uint8_t screenChannel = gEeprom.ScreenChannel[vfoNum];
    const ModulationMode_t mod = vfoInfo->Modulation;
    const ChannelAttributes_t att = gMR_ChannelAttributes[screenChannel];
    const enum Vfo_txtr_mode mode = FUNCTION_IsTx() ? VFO_MODE_TX : ( FUNCTION_IsRx() ? VFO_MODE_RX : VFO_MODE_NONE );
    const FREQ_Config_t *pConfig = (mode == VFO_MODE_TX) ? vfoInfo->pTX : vfoInfo->pRX;
    uint32_t frequency = pConfig->Frequency;

    // Show widget MemoryChan or Frequency Band
    if(isActive) {
        FillRect(0, base, 27, 9, C_FILL);
    }

    if (IS_MR_CHANNEL(screenChannel)) {   // channel mode
        const bool inputting = gInputBoxIndex != 0 && isActive;
        if (!inputting)
            PrintMediumEx(1, base+7, POS_L, C_INVERT, "M%03u", screenChannel + 1);
        else
            PrintMediumEx(1, base+7, POS_L, C_INVERT, "M%.3s", INPUTBOX_GetAscii());
    } else if (IS_FREQ_CHANNEL(screenChannel)) {   // frequency mode
        // show the frequency band number
        const char *buf = vfoInfo->pRX->Frequency < _1GHz_in_KHz ? "" : "+";
        const uint8_t fchannel = 1 + screenChannel - FREQ_CHANNEL_FIRST;
        const char *vfoIndexNames[] = {"A", "B"};
        PrintMediumEx(1, base+7, POS_L, C_INVERT, "V%s%u%s", vfoIndexNames[vfoNum], fchannel, buf);
    }

    if(isActive && (mode == VFO_MODE_TX || mode == VFO_MODE_RX) ) {
        FillRect(0, base+8, 14, 9, C_FILL);
        PrintMediumEx(1, base+15, POS_L, C_INVERT, mode == VFO_MODE_TX?"TX":"RX");
    }

    if(vfoNum == gEeprom.TX_VFO)
    {
        if(gMonitor) {
            PrintMediumEx(1, base + 23, POS_L, C_FILL, "MONI");
        } else {
            PrintMediumEx(1, base + 23, POS_L, C_FILL, "SQL%d", gEeprom.SQUELCH_LEVEL);
        }
    }


    const uint8_t startRightBlock = 32;

    // Frequency
    if (IS_FREQ_CHANNEL(screenChannel)) {
        if (gInputBoxIndex > 0) {   // user entering a frequency
            const char * ascii = INPUTBOX_GetAscii();
            bool isGigaF = frequency>=_1GHz_in_KHz;
            PrintBiggestDigitsEx(LCD_WIDTH - 14, base + 15, POS_R, C_FILL,
                "%.*s.%.3s", 3 + isGigaF, ascii, ascii + 3 + isGigaF);
        } else {
            PrintBiggestDigitsEx(LCD_WIDTH - 1, base + 15, POS_R, C_FILL,
                "%u.%03u", (frequency / 100000), (frequency / 100 % 1000));
        }
    } else {
        char string[22];
        const uint8_t centerRightBlock = startRightBlock + (LCD_WIDTH - startRightBlock) / 2;
        switch (gEeprom.CHANNEL_DISPLAY_MODE) {
            case MDF_FREQUENCY:
                PrintBiggestDigitsEx(LCD_WIDTH - 1, base + 15, POS_R, C_FILL,
                    "%u.%03u", (frequency / 100000), (frequency / 100 % 1000));
                break;

            case MDF_CHANNEL:   // show the channel number
                PrintMediumBoldEx(centerRightBlock, base + 15, POS_C, C_FILL, "CH-%03u", screenChannel + 1);
                break;

            case MDF_NAME:      // show the channel name
                SETTINGS_FetchChannelName(string, screenChannel);
                if (string[0] != 0) {
                    PrintMediumBoldEx(centerRightBlock, base + 15, POS_C, C_FILL, "%10s", string);
                } else {
                    PrintMediumBoldEx(centerRightBlock, base + 15, POS_C, C_FILL, "CH-%03u", screenChannel + 1);
                }

                break;
            case MDF_NAME_FREQ:
                SETTINGS_FetchChannelName(string, screenChannel);
                if (string[0] != 0) {
                    PrintMediumBoldEx(centerRightBlock, base + 7, POS_C, C_FILL, "%10s", string);
                }
                PrintMediumBoldEx(centerRightBlock, base + 15, POS_C, C_FILL,
                    "%u.%03u", (frequency / 100000), (frequency / 100 % 1000));
        }
    }

    // third line
    PrintSmall(startRightBlock, base + 23, gModulationStr[mod]);

    // show the audio scramble symbol
    if (vfoInfo->SCRAMBLING_TYPE > 0 && gSetting_ScrambleEnable)
        PrintSmall(startRightBlock+10, base + 23, "$");

    if (att.compander){
        PrintSmall(startRightBlock + 15, base + 23, "C");
    }

    const char *bandWidthNames[] = {"W", "N"};
    PrintSmall(startRightBlock + 20, base + 23, bandWidthNames[vfoInfo->CHANNEL_BANDWIDTH]);

    uint8_t currentPower = vfoInfo->OUTPUT_POWER % 8;
    const char pwr_short[][3] = {"L1", "L2", "L3", "L4", "L5", "M", "H"};
    PrintSmall(startRightBlock + 30, base + 23, pwr_short[currentPower]);

    if (vfoInfo->freq_config_RX.Frequency != vfoInfo->freq_config_TX.Frequency) {
        // show the TX offset symbol
        const char dir_list[][2] = {"", "+", "-"};
        PrintSmall(startRightBlock + 50, base + 23, dir_list[vfoInfo->TX_OFFSET_FREQUENCY_DIRECTION % 3]);
    }

    // show the TX/RX reverse symbol
    if (vfoInfo->FrequencyReverse) {
        PrintSmall(startRightBlock + 60, base + 23, "R");
    }

    PrintDCS(startRightBlock + 70, base + 23, POS_L, pConfig);
}

static void DisplayTwoVfo(uint8_t activeVFO) {
    printVfoBlock(9, 0, activeVFO == 0);
    if (FUNCTION_IsRx()) {
        UI_RSSIBar(BK4819_GetRSSI(), 28);
    }
    printVfoBlock(37, 1, activeVFO == 1);
}

void UI_DisplayMain(void)
{
    UI_ClearScreen();

    if(gLowBattery && !gLowBatteryConfirmed) {
        UI_DisplayPopup("LOW BATTERY");
        ST7565_BlitFullScreen();
        return;
    }

    uint8_t activeVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;

    // Airband scan uses the same layout as single mode (clean top, big
    // channel name). The DualVfo/TwoVfo paths are only for the legacy modes.
    if (gAirbandConfig.last_op_mode == OP_MODE_SINGLE || gScanStateDir != SCAN_OFF) {
        DisplaySingleFreq(activeVFO);
    } else if (gAirbandConfig.last_op_mode == OP_MODE_DUAL || isMainOnly(false)) {
        DisplayDualVfo(activeVFO);
    } else {
        DisplayTwoVfo(activeVFO);
    }

    ST7565_BlitFullScreen();
}
