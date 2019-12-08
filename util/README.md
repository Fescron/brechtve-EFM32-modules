# UTIL

## Includes

### MCU-specific

- `stdint`
- `stdbool`
- `em_device`
- `em_cmu`
- `em_gpio`

### Extra modules from this repository

- `debug_dbprint` (see [dbprint.brechtve.be](http://dbprint.brechtve.be))
- `delay`
- `pin_mapping`
- `util`
- (`lora_wrappers`)

<br/>

## Implemented methods

### Public

```C
void led (bool enabled)
void error (uint8_t number)
```

### Internal

```C
static void initLED (void)
```
