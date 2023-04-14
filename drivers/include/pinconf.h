/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/******************************************************************************
 * @file     pinconf.h
 * @author   Silesh C V
 * @email    silesh@alifsemi.com
 * @version  V1.0.0
 * @date     02-Feb-2023
 * @brief    Pin configuration.
 ******************************************************************************/

#ifndef __PINCONF_H__
#define __PINCONF_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus*/

#include <stdint.h>

#define PORT_0                                              0U
#define PORT_1                                              1U
#define PORT_2                                              2U
#define PORT_3                                              3U
#define PORT_4                                              4U
#define PORT_5                                              5U
#define PORT_6                                              6U
#define PORT_7                                              7U
#define PORT_8                                              8U
#define PORT_9                                              9U
#define PORT_10                                             10U
#define PORT_11                                             11U
#define PORT_12                                             12U
#define PORT_13                                             13U
#define PORT_14                                             14U
#define PORT_15                                             15U

#define PIN_0                                               0U
#define PIN_1                                               1U
#define PIN_2                                               2U
#define PIN_3                                               3U
#define PIN_4                                               4U
#define PIN_5                                               5U
#define PIN_6                                               6U
#define PIN_7                                               7U

#define PINMUX_ALTERNATE_FUNCTION_0                         0U
#define PINMUX_ALTERNATE_FUNCTION_1                         1U
#define PINMUX_ALTERNATE_FUNCTION_2                         2U
#define PINMUX_ALTERNATE_FUNCTION_3                         3U
#define PINMUX_ALTERNATE_FUNCTION_4                         4U
#define PINMUX_ALTERNATE_FUNCTION_5                         5U
#define PINMUX_ALTERNATE_FUNCTION_6                         6U
#define PINMUX_ALTERNATE_FUNCTION_7                         7U

#define PADCTRL_READ_ENABLE                                 0x01U
#define PADCTRL_SCHMITT_TRIGGER_ENABLE                      0x02U
#define PADCTRL_SLEW_RATE_FAST                              0x04U
#define PADCTRL_DRIVER_DISABLED_HIGH_Z                      0x00U
#define PADCTRL_DRIVER_DISABLED_PULL_UP                     0x08U
#define PADCTRL_DRIVER_DISABLED_PULL_DOWN                   0x10U
#define PADCTRL_DRIVER_DISABLED_BUS_REPEATER                0x18U
#define PADCTRL_OUTPUT_DRIVE_STRENGTH_02_MILI_AMPS          0x00U
#define PADCTRL_OUTPUT_DRIVE_STRENGTH_04_MILI_AMPS          0x20U
#define PADCTRL_OUTPUT_DRIVE_STRENGTH_08_MILI_AMPS          0x40U
#define PADCTRL_OUTPUT_DRIVE_STRENGTH_12_MILI_AMPS          0x60U
#define PADCTRL_DRIVER_OPEN_DRAIN                           0x80U

/**
  \fn          int32_t pinconf_set(const uint8_t port, const uint8_t pin, const uint8_t alt_func, const uint8_t pad_ctrl)
  \brief       Set pinmux and pad control configuration for a pin.
  \param[in]   port       GPIO port
  \param[in]   pin        GPIO pin
  \param[in]   alt_func   The alternate function to be chosen
  \param[in]   pad_ctrl   Pad control configuration for the pin
  \return      execution_status
*/
int32_t pinconf_set(uint8_t port, uint8_t pin, uint8_t alt_func, uint8_t pad_ctrl);

/**
  \fn          int32_t pinconf_get(const uint8_t port, const uint8_t pin, uint8_t *alt_func, uint8_t *pad_ctrl)
  \brief       Get pinmux and pad control configuration for a pin.
  \param[in]   port       GPIO port
  \param[in]   pin        GPIO pin
  \param[out]  alt_func   Current alternate function selected for the pin
  \param[out]  pad_ctrl   Current pad control configuration for the pin
  \return      execution_status
*/
int32_t pinconf_get(uint8_t port, uint8_t pin, uint8_t *alt_func, uint8_t *pad_ctrl);

#ifdef  __cplusplus
}
#endif /* __cplusplus */
#endif /* __PINCONF_H__ */
