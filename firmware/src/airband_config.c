#include "airband_config.h"
#include "airband_channels.h"
#include "driver/eeprom.h"
#include "misc.h"
#include "driver/st7565.h"
#include "settings.h"

airband_config_t gAirbandConfig;

void AIRBAND_LoadConfig(void)
{
    EEPROM_ReadBuffer(AIRBAND_EEPROM_ADDR, &gAirbandConfig, sizeof(gAirbandConfig));

    if (gAirbandConfig.magic != AIRBAND_MAGIC) {
        gAirbandConfig.startup_mode = STARTUP_SCAN;
        gAirbandConfig.last_op_mode = OP_MODE_SCAN;
        gAirbandConfig.last_freq_hz = 11880000; // 118.800 MHz
        gAirbandConfig.magic = AIRBAND_MAGIC;
        gAirbandConfig.reserved = 0xFF;
        AIRBAND_SaveConfig();
        AIRBAND_LoadDefaults();

        // Force normal display (light background)
        gSetting_set_inv = false;
        ST7565_ContrastAndInv();
    }

    // Dedicated scanner: always scan every MR channel
    gEeprom.SCAN_LIST_DEFAULT = 5;  // "ALL"
}

void AIRBAND_SaveConfig(void)
{
    gAirbandConfig.magic = AIRBAND_MAGIC;
    EEPROM_WriteBuffer(AIRBAND_EEPROM_ADDR, &gAirbandConfig);
}

void AIRBAND_SetStartupMode(startup_mode_t mode)
{
    gAirbandConfig.startup_mode = mode;
    AIRBAND_SaveConfig();
}

void AIRBAND_SetOpMode(op_mode_t mode)
{
    gAirbandConfig.last_op_mode = mode;
    AIRBAND_SaveConfig();
}

void AIRBAND_SetLastFreq(uint32_t freq_hz)
{
    gAirbandConfig.last_freq_hz = freq_hz;
    AIRBAND_SaveConfig();
}
