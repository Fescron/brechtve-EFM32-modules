# ADC

## Includes

### MCU-specific

- `stdint`
- `stdbool`
- `em_device`
- `em_cmu`
- `em_adc`

### Extra modules from this repository

- `debug_dbprint` (see [dbprint.brechtve.be](http://dbprint.brechtve.be))
- `util`

<br/>

## Implemented methods

### Public

```C
static float32_t convertToCelsius (int32_t adcSample)
void ADC0_IRQHandler (void)
```

### Internal

```C
static float32_t convertToCelsius (int32_t adcSample)
void ADC0_IRQHandler (void)
```

<br/>

## Implemented types

```C
/** Enum type for the ADC */
typedef enum adc_measurements
{
	BATTERY_VOLTAGE,
	INTERNAL_TEMPERATURE
} ADC_Measurement_t;
```
