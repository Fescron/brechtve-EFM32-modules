# DELAY

## Includes

### MCU-specific

- `stdint`
- `stdbool`
- `em_device`
- `em_cmu`
- `em_emu`
- `em_gpio`
- `em_rtc`

### Extra modules from this repository

- `debug_dbprint` (see dbprint.brechtve.be)
- `delay`
- (`util`)

<br/>

## Implemented methods

### Public

```C
void delay (uint32_t msDelay)
void sleep (uint32_t sSleep)
bool RTC_checkWakeup (void)
void RTC_clearWakeup (void)
uint32_t RTC_getPassedSleeptime (void)
```

### Internal

```C
static void initRTC (void)
void SysTick_Handler (void)
void RTC_IRQHandler (void)
```
