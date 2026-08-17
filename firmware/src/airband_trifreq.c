#include "airband_trifreq.h"
#include "driver/bk4819.h"
#include "functions.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"

tri_freq_state_t gTriFreq;

void TRIFREQ_Init(void)
{
    gTriFreq.freqs[0] = 11880000;  // 118.800 MHz
    gTriFreq.freqs[1] = 12030000;  // 120.300 MHz
    gTriFreq.freqs[2] = 12150000;  // 121.500 MHz
    gTriFreq.active_slot = 0;
    gTriFreq.tick_count = 0;
    gTriFreq.enabled = false;
    gTriFreq.editing_slot = -1;
    gTriFreq.edit_mem_index = 0;

    for (uint8_t i = 0; i < TRI_FREQ_SLOTS; i++) {
        gTriFreq.rssi[i] = 0;
        gTriFreq.signal_present[i] = false;
    }
}

void TRIFREQ_Enable(bool on)
{
    gTriFreq.enabled = on;
    if (on) {
        gTriFreq.active_slot = 0;
        gTriFreq.tick_count = 0;
        BK4819_SetFrequency(gTriFreq.freqs[gTriFreq.active_slot]);
        BK4819_SetTailDetection(550);
    }
}

void TRIFREQ_SetSlotFreq(uint8_t slot, uint32_t freq)
{
    if (slot < TRI_FREQ_SLOTS)
        gTriFreq.freqs[slot] = freq;
}

void TRIFREQ_Tick10ms(void)
{
    if (!gTriFreq.enabled || gTriFreq.editing_slot >= 0)
        return;

    if (++gTriFreq.tick_count < TRI_FREQ_TICK_INTERVAL)
        return;
    gTriFreq.tick_count = 0;

    gTriFreq.rssi[gTriFreq.active_slot] = BK4819_GetRSSI();
    gTriFreq.signal_present[gTriFreq.active_slot] =
        (gTriFreq.rssi[gTriFreq.active_slot] > 80);

    // If current slot has signal, stay on it
    if (gTriFreq.signal_present[gTriFreq.active_slot])
        return;

    // Check if any other slot might have signal (rotate to find out)
    uint8_t next = (gTriFreq.active_slot + 1) % TRI_FREQ_SLOTS;
    gTriFreq.active_slot = next;

    BK4819_SetFrequency(gTriFreq.freqs[next]);
    BK4819_SetTailDetection(550);
}

void TRIFREQ_StartEdit(uint8_t slot)
{
    if (slot < TRI_FREQ_SLOTS) {
        gTriFreq.editing_slot = slot;
        gTriFreq.edit_mem_index = 0;
    }
}

void TRIFREQ_EditScroll(int8_t direction)
{
    if (gTriFreq.editing_slot < 0)
        return;

    uint8_t max_ch = 29;
    if (direction > 0 && gTriFreq.edit_mem_index < max_ch)
        gTriFreq.edit_mem_index++;
    else if (direction < 0 && gTriFreq.edit_mem_index > 0)
        gTriFreq.edit_mem_index--;

    // Read frequency from memory channel
    VFO_Info_t info;
    RADIO_InitInfo(&info, gTriFreq.edit_mem_index, gTriFreq.freqs[gTriFreq.editing_slot]);

    if (IS_MR_CHANNEL(gTriFreq.edit_mem_index)) {
        RADIO_ConfigureChannel(0, gTriFreq.edit_mem_index);
        gTriFreq.freqs[gTriFreq.editing_slot] = gEeprom.VfoInfo[0].pRX->Frequency;
    }
}

void TRIFREQ_ConfirmEdit(void)
{
    gTriFreq.editing_slot = -1;
    BK4819_SetFrequency(gTriFreq.freqs[gTriFreq.active_slot]);
    BK4819_SetTailDetection(550);
}

void TRIFREQ_CancelEdit(void)
{
    gTriFreq.editing_slot = -1;
}
