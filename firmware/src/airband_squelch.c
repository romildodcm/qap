#include "airband_squelch.h"
#include "driver/bk4819.h"

static int16_t noise_samples[NOISE_FLOOR_SAMPLES];
static uint8_t sample_index;
static int16_t noise_floor;
static bool    squelch_open;

void AIRBAND_SquelchInit(void)
{
    for (uint8_t i = 0; i < NOISE_FLOOR_SAMPLES; i++)
        noise_samples[i] = 320;  // -160 + 320/2 = 0 dBm equivalent raw
    sample_index = 0;
    noise_floor = 320;
    squelch_open = false;
}

void AIRBAND_SquelchUpdate(uint16_t rssi)
{
    int16_t val = (int16_t)rssi;

    // Track noise floor: keep the lowest samples
    if (val < noise_samples[sample_index] || !squelch_open)
        noise_samples[sample_index] = val;

    sample_index = (sample_index + 1) % NOISE_FLOOR_SAMPLES;

    // Compute noise floor as average of stored samples
    int32_t sum = 0;
    for (uint8_t i = 0; i < NOISE_FLOOR_SAMPLES; i++)
        sum += noise_samples[i];
    noise_floor = (int16_t)(sum / NOISE_FLOOR_SAMPLES);

    // SNR in half-dB units (raw RSSI is in half-dB)
    int16_t snr_half_db = val - noise_floor;
    int16_t snr_db = snr_half_db / 2;

    if (squelch_open) {
        if (snr_db < SNR_CLOSE_DB)
            squelch_open = false;
    } else {
        if (snr_db >= SNR_OPEN_DB)
            squelch_open = true;
    }
}

bool AIRBAND_SquelchCheck(void)
{
    return squelch_open;
}

int16_t AIRBAND_SquelchGetSNR(void)
{
    return ((int16_t)BK4819_GetRSSI() - noise_floor) / 2;
}

int16_t AIRBAND_SquelchGetNoiseFloor(void)
{
    return (noise_floor / 2) - 160;
}
