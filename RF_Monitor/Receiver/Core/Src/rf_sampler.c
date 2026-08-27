#include "rf_sampler.h"

#include "stm32f4xx_hal.h"

#define RF_SAMPLER_TIMER_CLOCK_HZ 1000000U
#define RF_SAMPLER_TIMER_PRIORITY 1U
#define RF_SAMPLER_SYSTICK_PRIORITY 15U

static volatile uint8_t sample_request_pending;
static volatile uint32_t overrun_count;

static uint32_t RF_Sampler_EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void RF_Sampler_ExitCritical(uint32_t primask)
{
    if (primask == 0U)
    {
        __enable_irq();
    }
}

bool RF_Sampler_Init(void)
{
    uint32_t timer_clock_hz = HAL_RCC_GetPCLK1Freq();

    /* APB timers run at twice PCLK when the APB prescaler is not 1. */
    if ((RCC->CFGR & RCC_CFGR_PPRE1) != 0U)
    {
        timer_clock_hz *= 2U;
    }

    if (timer_clock_hz == 0U ||
        (timer_clock_hz % RF_SAMPLER_TIMER_CLOCK_HZ) != 0U)
    {
        return false;
    }

    uint32_t prescaler = timer_clock_hz / RF_SAMPLER_TIMER_CLOCK_HZ;
    if (prescaler == 0U || prescaler > 65536U)
    {
        return false;
    }

    __HAL_RCC_TIM2_CLK_ENABLE();

    TIM2->CR1 = TIM_CR1_URS;
    TIM2->CR2 = 0U;
    TIM2->SMCR = 0U;
    TIM2->DIER = 0U;
    TIM2->PSC = prescaler - 1U;
    TIM2->ARR = (RF_SAMPLER_TIMER_CLOCK_HZ / RF_SAMPLER_RATE_HZ) - 1U;
    TIM2->CNT = 0U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;

    /* Fault handlers remain above TIM2; SysTick remains below it. */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(TIM2_IRQn, RF_SAMPLER_TIMER_PRIORITY, 0U);
    HAL_NVIC_SetPriority(SysTick_IRQn, RF_SAMPLER_SYSTICK_PRIORITY, 0U);
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    HAL_NVIC_EnableIRQ(TIM2_IRQn);

    sample_request_pending = 0U;
    overrun_count = 0U;
    return true;
}

void RF_Sampler_Start(void)
{
    uint32_t primask = RF_Sampler_EnterCritical();

    sample_request_pending = 0U;
    overrun_count = 0U;
    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->CNT = 0U;
    TIM2->SR = 0U;
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1 |= TIM_CR1_CEN;

    RF_Sampler_ExitCritical(primask);
}

void RF_Sampler_Stop(void)
{
    uint32_t primask = RF_Sampler_EnterCritical();

    TIM2->CR1 &= ~TIM_CR1_CEN;
    TIM2->DIER &= ~TIM_DIER_UIE;
    TIM2->SR = 0U;
    HAL_NVIC_ClearPendingIRQ(TIM2_IRQn);
    sample_request_pending = 0U;

    RF_Sampler_ExitCritical(primask);
}

bool RF_Sampler_TakeRequest(void)
{
    bool request_available = false;
    uint32_t primask = RF_Sampler_EnterCritical();

    if (sample_request_pending != 0U)
    {
        sample_request_pending = 0U;
        request_available = true;
    }

    RF_Sampler_ExitCritical(primask);
    return request_available;
}

uint32_t RF_Sampler_GetOverrunCount(void)
{
    uint32_t primask = RF_Sampler_EnterCritical();
    uint32_t count = overrun_count;
    RF_Sampler_ExitCritical(primask);
    return count;
}

void RF_Sampler_TimerIRQHandler(void)
{
    if ((TIM2->SR & TIM_SR_UIF) == 0U)
    {
        return;
    }

    TIM2->SR &= ~TIM_SR_UIF;

    if (sample_request_pending != 0U)
    {
        overrun_count++;
    }
    else
    {
        sample_request_pending = 1U;
    }
}
