# Ensemble DFP 

## Modifications to the upstream Alif DFP

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

## Open Items and Missing features
As we work towards the goal to enable Ensemble devices together with DevKit and AppKit boards as preferred platform for
AI/ML workflows (and generic examples) the issues below were found.

The list shall be updated regularly.

- [ ] Driver components that implement CMSIS-Driver APIs ([see CMSIS docs, Covered Interfaces](https://arm-software.github.io/CMSIS_6/latest/Driver/index.html#cmsis_drv_interfaces)) shall be moved to the CMSIS Driver class (Cclass="CMSIS Driver")
  - see [commit](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/commit/c69f2c3b8bd99402dc75e3a92e514d7366c5048d) and an attempt to address this

- [ ] GPIO driver exists with an API that differs from one defined by CMSIS
  - see [here for Alifs Driver_GPIO.h](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/blob/main/Alif_CMSIS/Include/Driver_GPIO.h) and [here for CMSIS defined Driver_GPIO.h](https://github.com/ARM-software/CMSIS_6/blob/main/CMSIS/Driver/Include/Driver_GPIO.h)
  - the GPIO interface was covered by CMSIS with the release of CMSIS 6 while other interfaces were already covered before
  - an CMSIS compliant GPIO implementation [can be found here](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/commit/5b2799267d27ebffa2ba6d96270ecb223fcfe821)

- [ ] Retargeting components shall be updated to support CMSIS-Compiler 2.0.0
  - STDIO components were redefined in CMSIS-Compiler 2.0.0 and are incompatible with 1.0.0
  - see [commit](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/commit/27ae5df7ac086c5dbc9b5023094e0cce06aae24d)

- [ ] The board description for the AppKit board shall be added to the pack description file
  - The [board description was temporarily added here](https://github.com/VladimirUmek/alif_ensemble_appkit-bsp/blob/main/Keil.Ensemble_AppKit-E7_BSP.pdsc#L37) to enable AppKit board in our projects

- [ ] Code download to MRAM does not work when using uVision and UlinkPRO
  - debug stubs were pre-loaded with SETOOLS and device ATOC seems fine according to user guides

- [ ] What is the status of the CMSIS drivers for
  - WiFi
  - USB Device
  - MCI

- [ ] The driver versions do not match component version in the pdsc (pack description file)
  - for example SPI, [version in source is 1.0](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/blob/48b6dd8acdd0324198d73dfbad56ad635035879d/Alif_CMSIS/Source/Driver_SPI.c#L35) and [version in pdsc is 1.1.0](https://github.com/VladimirUmek/alif_ensemble-cmsis-dfp/blob/48b6dd8acdd0324198d73dfbad56ad635035879d/AlifSemiconductor.Ensemble.pdsc#L1373)
  - the same situation is also for other drivers
  - that is not an issue if implementation and component have in fact different versions

