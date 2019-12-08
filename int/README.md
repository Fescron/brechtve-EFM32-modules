# INT

## Includes

### MCU-specific

- `stdint`
- `stdbool`
- `em_device`
- `em_cmu`
- `em_gpio`
- `em_rtc`

### Extra modules from this repository

- `debug_dbprint` (see [dbprint.brechtve.be](http://dbprint.brechtve.be))
- `delay`
- `pin_mapping`
- `util`
- `ADXL362`

<br/>

## Implemented methods

### Public

```C
void initGPIOwakeup (void)
bool BTN_getTriggered (uint8_t number)
void BTN_setTriggered (uint8_t number, bool value)
```

### Internal

```C
void GPIO_EVEN_IRQHandler (void)
void GPIO_ODD_IRQHandler (void)
```
