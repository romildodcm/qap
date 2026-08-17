#ifndef AIRBAND_CHANNELS_H
#define AIRBAND_CHANNELS_H

#include <stdint.h>

#define AIRBAND_MAX_CHANNELS 30
#define AIRBAND_DEFAULT_COUNT 9

typedef struct {
    uint32_t freq_hz;
    uint8_t  priority; // 0=normal, 1=alta
    char     label[8];
} airband_channel_t;

extern const airband_channel_t airband_defaults[AIRBAND_DEFAULT_COUNT];

void AIRBAND_LoadDefaults(void);

#endif
