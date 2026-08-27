#ifndef RF_SAMPLER_H
#define RF_SAMPLER_H

#include <stdbool.h>
#include <stdint.h>

#define RF_SAMPLER_RATE_HZ 1000U
#define RF_SAMPLER_WINDOW_SIZE 512U

bool RF_Sampler_Init(void);
void RF_Sampler_Start(void);
void RF_Sampler_Stop(void);
bool RF_Sampler_TakeRequest(void);
uint32_t RF_Sampler_GetOverrunCount(void);
void RF_Sampler_TimerIRQHandler(void);

#endif
