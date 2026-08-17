#ifndef AIRBAND_TRIFREQ_H
#define AIRBAND_TRIFREQ_H

#include <stdint.h>
#include <stdbool.h>

#define TRI_FREQ_SLOTS 3
#define TRI_FREQ_TICK_INTERVAL 3  // 3 × 10ms = 30ms per slot

typedef struct {
    uint32_t freqs[TRI_FREQ_SLOTS];
    int16_t  rssi[TRI_FREQ_SLOTS];
    bool     signal_present[TRI_FREQ_SLOTS];
    uint8_t  active_slot;
    uint8_t  tick_count;
    bool     enabled;
    int8_t   editing_slot;  // -1 = not editing, 0–2 = editing that slot
    uint8_t  edit_mem_index; // which memory channel is selected during edit
} tri_freq_state_t;

extern tri_freq_state_t gTriFreq;

void TRIFREQ_Init(void);
void TRIFREQ_Tick10ms(void);
void TRIFREQ_Enable(bool on);
void TRIFREQ_SetSlotFreq(uint8_t slot, uint32_t freq);
void TRIFREQ_StartEdit(uint8_t slot);
void TRIFREQ_EditScroll(int8_t direction);
void TRIFREQ_ConfirmEdit(void);
void TRIFREQ_CancelEdit(void);

#endif
