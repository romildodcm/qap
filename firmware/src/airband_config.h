#ifndef AIRBAND_CONFIG_H
#define AIRBAND_CONFIG_H

#include <stdint.h>

typedef enum {
    STARTUP_LAST = 0,
    STARTUP_SCAN = 1,
} startup_mode_t;

typedef enum {
    OP_MODE_SINGLE = 0,
    OP_MODE_DUAL   = 1,
    OP_MODE_SCAN   = 2,
} op_mode_t;

#define AIRBAND_EEPROM_ADDR  0x0E40
#define AIRBAND_MAGIC        0xAB

typedef struct {
    uint8_t        startup_mode;
    uint8_t        last_op_mode;
    uint8_t        magic;
    uint8_t        reserved;
    uint32_t       last_freq_hz;
} __attribute__((packed)) airband_config_t;

void AIRBAND_LoadConfig(void);
void AIRBAND_SaveConfig(void);
void AIRBAND_SetStartupMode(startup_mode_t mode);
void AIRBAND_SetOpMode(op_mode_t mode);
void AIRBAND_SetLastFreq(uint32_t freq_hz);

extern airband_config_t gAirbandConfig;

#endif
