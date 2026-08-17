#ifndef AIRBAND_SQUELCH_H
#define AIRBAND_SQUELCH_H

#include <stdbool.h>
#include <stdint.h>

#define SNR_OPEN_DB    10
#define SNR_CLOSE_DB    7
#define NOISE_FLOOR_SAMPLES 16

void    AIRBAND_SquelchInit(void);
void    AIRBAND_SquelchUpdate(uint16_t rssi);
bool    AIRBAND_SquelchCheck(void);
int16_t AIRBAND_SquelchGetSNR(void);
int16_t AIRBAND_SquelchGetNoiseFloor(void);

#endif
