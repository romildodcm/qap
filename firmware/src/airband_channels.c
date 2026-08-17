#include "airband_channels.h"
#include "misc.h"
#include "settings.h"
#include "frequencies.h"
#include "driver/eeprom.h"
#include "radio.h"
#include <string.h>

const airband_channel_t airband_defaults[AIRBAND_DEFAULT_COUNT] = {
    {11880000, 1, "TWR FOZ"},
    {12030000, 1, "APP FOZ"},
    {11915000, 1, "APP FOZ"},
    {12910000, 0, "APP FOZ"},
    {12345000, 0, "UNICOM"},
    {12340000, 0, "LIVRE"},
    {12490000, 0, "ACC CWB"},
    {13380000, 0, "ACC CWB"},
    {12150000, 1, "EMERG"},
};

void AIRBAND_LoadDefaults(void)
{
    for (uint8_t i = 0; i < AIRBAND_DEFAULT_COUNT && i < AIRBAND_MAX_CHANNELS; i++) {
        const airband_channel_t *ch = &airband_defaults[i];

        uint32_t freq = ch->freq_hz;
        uint16_t addr = 0x0000 + (i * 16);
        uint8_t buf[16];

        memset(buf, 0, sizeof(buf));

        // freq RX (bytes 0-3) and TX (bytes 4-7)
        memcpy(&buf[0], &freq, 4);
        memcpy(&buf[4], &freq, 4);

        // byte 0x0B: modulation(upper nibble)=AM | offsetDir(lower nibble)=0
        buf[0x0B] = (MODULATION_AM << 4);

        // byte 0x0C: bandwidth=wide(0), power=low(0)
        buf[0x0C] = 0;

        // byte 0x0E: step = STEP_8_33kHz
        buf[0x0E] = STEP_8_33kHz;

        EEPROM_WriteBuffer(addr, buf);
        EEPROM_WriteBuffer(addr + 8, buf + 8);

        // Channel name
        uint16_t name_addr = 0x0F50 + (i * 16);
        uint8_t name_buf[16];
        memset(name_buf, 0, sizeof(name_buf));
        strncpy((char *)name_buf, ch->label, 7);
        EEPROM_WriteBuffer(name_addr, name_buf);
        EEPROM_WriteBuffer(name_addr + 8, name_buf + 8);

        // Channel attribute: band=1 (108MHz), scanlist enabled
        gMR_ChannelAttributes[i].__val = 0;
        gMR_ChannelAttributes[i].band = BAND2_108MHz;
        gMR_ChannelAttributes[i].scanlist1 = 1;
    }

    // Save channel attributes to EEPROM
    EEPROM_WriteBuffer(0x0D60, (const void *)gMR_ChannelAttributes);
    EEPROM_WriteBuffer(0x0D68, (const void *)((const uint8_t *)gMR_ChannelAttributes + 8));

    // Force MR mode: screen shows channel 0 (TWR FOZ)
    uint8_t screen_cfg[8];
    EEPROM_ReadBuffer(0x0E80, screen_cfg, 8);
    screen_cfg[0] = MR_CHANNEL_FIRST;     // ScreenChannel[0] = MR channel 0
    screen_cfg[1] = MR_CHANNEL_FIRST;     // MrChannel[0] = 0
    screen_cfg[3] = MR_CHANNEL_FIRST;     // ScreenChannel[1] = MR channel 0
    screen_cfg[4] = MR_CHANNEL_FIRST;     // MrChannel[1] = 0
    EEPROM_WriteBuffer(0x0E80, screen_cfg);

    // Update RAM settings
    gEeprom.ScreenChannel[0] = MR_CHANNEL_FIRST;
    gEeprom.ScreenChannel[1] = MR_CHANNEL_FIRST;
    gEeprom.MrChannel[0] = MR_CHANNEL_FIRST;
    gEeprom.MrChannel[1] = MR_CHANNEL_FIRST;
}
