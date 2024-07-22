# Ensemble DFP 

## Modifications to the original Alif DFP

### - Updated DFP to support CMSIS-Compiler 2.0.0 (also branch: compiler_2_0_0)
  - replaces version that supported CMSIS-Compiler 1.0.0
  - the main difference is in how STDIO components are represented
  - see [commit](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/commit/27ae5df7ac086c5dbc9b5023094e0cce06aae24d)

### - Changed component class for drivers that have CMSIS defined APIs (also branch: cmsis_driver)
  - change from Cclass="Device" Cgroup="SOC Peripherals" to Cclass="CMSIS Driver"
  - deleted header files for driver APIs that reside in CMSIS pack
  - change from Csub to Cgroup
      Csub         | Cgroup
      -------------|--------
      USART        | USART
      I2S          | SAI
      I2C          | I2C
      LPI2C        | I2C:LPI2C
      Ethernet MAC | Ethernet MAC
      SPI          | SPI
  - see [commit](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/commit/c69f2c3b8bd99402dc75e3a92e514d7366c5048d)

### - Added GPIO driver using CMSIS Driver API specification (also branch: cmsis_driver_gpio)
  - GPIO driver using Alif defined API was removed
  - extension that would cover functionality gap between CMSIS and Alif API is not yet implemented
  - see [commit](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/commit/5b2799267d27ebffa2ba6d96270ecb223fcfe821)
