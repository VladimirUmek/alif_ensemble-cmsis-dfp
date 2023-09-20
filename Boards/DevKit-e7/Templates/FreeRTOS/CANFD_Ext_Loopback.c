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
 * @file     CANFD_Ext_Loopback.c
 * @author   Shreehari H K
 * @email    shreehari.hk@alifsemi.com
 * @version  V1.0.0
 * @date     05-July-2023
 * @brief    FreeRTOS demo application for CANFD.
 *           - Performs External loopback test.
 * @bug      None
 * @Note     None
 ******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <RTE_Components.h>
#include CMSIS_device_header
#include "pinconf.h"
#include "Driver_CAN.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#if defined(RTE_Compiler_IO_STDOUT)
#include "retarget_stdout.h"
#endif  /* RTE_Compiler_IO_STDOUT */

#include "se_services_port.h"

/* It is recommended to use the bit rate and bit segments
 * as specified in the Hardware reference manual for proper communication.
*/
/*   CANFD_CLK_BITRATE_SETTINGS = 1 -> Nominal bit rate 500kbps, Fast bit rate 2Mbps, 20MHz CANFD clock
 *   CANFD_CLK_BITRATE_SETTINGS = 2 -> Nominal bit rate 250kbps, Fast bit rate 1Mbps, 20MHz CANFD clock
 *   CANFD_CLK_BITRATE_SETTINGS = 3 -> Nominal bit rate 1Mbps, Fast bit rate 4Mbps, 20MHz CANFD clock
*/
#define CANFD_CLK_BITRATE_SETTINGS          1U
#if (CANFD_CLK_BITRATE_SETTINGS == 1U)
    #define CANFD_NOMINAL_BITRATE           500000U
    #define CANFD_BIT_TIME_PROP_SEG         2U
    #define CANFD_BIT_TIME_SEG1             30U
    #define CANFD_BIT_TIME_SEG2             8U
    #define CANFD_BIT_TIME_SJW              8U
    #define CANFD_FAST_BITRATE              2000000U /* 2Mbps */
    #define CANFD_FAST_BIT_TIME_PROP_SEG    1U
    #define CANFD_FAST_BIT_TIME_SEG1        7U
    #define CANFD_FAST_BIT_TIME_SEG2        2U
    #define CANFD_FAST_BIT_TIME_SJW         2U
#elif (CANFD_CLK_BITRATE_SETTINGS == 2U)
    #define CANFD_BITRATE                   250000U
    #define CANFD_BIT_TIME_PROP_SEG         4U
    #define CANFD_BIT_TIME_SEG1             60U
    #define CANFD_BIT_TIME_SEG2             16U
    #define CANFD_BIT_TIME_SJW              16U
    #define CANFD_FAST_BITRATE              1000000U
    #define CANFD_FAST_BIT_TIME_PROP_SEG    2U
    #define CANFD_FAST_BIT_TIME_SEG1        14U
    #define CANFD_FAST_BIT_TIME_SEG2        4U
    #define CANFD_FAST_BIT_TIME_SJW         4U
#elif (CANFD_CLK_BITRATE_SETTINGS == 3U)
    #define CANFD_BITRATE                   1000000U
    #define CANFD_BIT_TIME_PROP_SEG         2U
    #define CANFD_BIT_TIME_SEG1             14U
    #define CANFD_BIT_TIME_SEG2             4U
    #define CANFD_BIT_TIME_SJW              4U
    #define CANFD_FAST_BITRATE              4000000U
    #define CANFD_FAST_BIT_TIME_PROP_SEG    1U
    #define CANFD_FAST_BIT_TIME_SEG1        3U
    #define CANFD_FAST_BIT_TIME_SEG2        1U
    #define CANFD_FAST_BIT_TIME_SJW         1U
#endif

#define CANFD_NOMINAL_BITTIME_SEGMENTS      ((CANFD_BIT_TIME_PROP_SEG << 0U)      | \
                                            (CANFD_BIT_TIME_SEG1 << 8U)           | \
                                            (CANFD_BIT_TIME_SEG2 << 16U)          | \
                                            (CANFD_BIT_TIME_SJW << 24U))

#define CANFD_FAST_BITTIME_SEGMENTS         ((CANFD_FAST_BIT_TIME_PROP_SEG << 0U) | \
                                            (CANFD_FAST_BIT_TIME_SEG1 << 8U)      | \
                                            (CANFD_FAST_BIT_TIME_SEG2 << 16U)     | \
                                            (CANFD_FAST_BIT_TIME_SJW << 24U))

/* Object filter settings */
#define CANFD_OBJECT_FILTER_CODE_1          0x5A5U
#define CANFD_OBJECT_FILTER_CODE_2          0x01FF5A5AU
#define CANFD_OBJECT_FILTER_MASK            0U

#define CANFD_MAX_MSG_SIZE                  64U

/* Application Message Frame types */
typedef enum _CANFD_FRAME
{
    CANFD_FRAME_STD_ID_CLASSIC_DATA,
    CANFD_FRAME_STD_ID_RTR,
    CANFD_FRAME_STD_ID_FD_DATA,
    CANFD_FRAME_EXT_ID_RTR,
    CANFD_FRAME_EXT_ID_CLASSIC_DATA,
    CANFD_FRAME_EXT_ID_FD_DATA,
    CANFD_FRAME_OVER
}CANFD_FRAME;

/* Define for FreeRTOS notification objects */
#define CANFD_RX_SUCCESS                    0x01
#define CANFD_ERROR                         0x02
#define CANFD_ALL_NOTIFICATIONS             (CANFD_RX_SUCCESS | CANFD_ERROR)

/* Define for FreeRTOS */
#define CANFD_TASK_STACK_SIZE               512U      /* 512 Words */
#define TIMER_SERVICE_TASK_STACK_SIZE       configTIMER_TASK_STACK_DEPTH
#define IDLE_TASK_STACK_SIZE                configMINIMAL_STACK_SIZE

StackType_t IdleStack[2 * IDLE_TASK_STACK_SIZE];
StaticTask_t IdleTcb;
StackType_t TimerStack[2 * TIMER_SERVICE_TASK_STACK_SIZE];
StaticTask_t TimerTcb;

/* Task handle */
TaskHandle_t canfd_xHandle;

/****************************** FreeRTOS functions **********************/

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
      StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize) {
   *ppxIdleTaskTCBBuffer = &IdleTcb;
   *ppxIdleTaskStackBuffer = IdleStack;
   *pulIdleTaskStackSize = IDLE_TASK_STACK_SIZE;
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
   (void) pxTask;

   for (;;);
}

void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
      StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
   *ppxTimerTaskTCBBuffer = &TimerTcb;
   *ppxTimerTaskStackBuffer = TimerStack;
   *pulTimerTaskStackSize = TIMER_SERVICE_TASK_STACK_SIZE;
}

void vApplicationIdleHook(void)
{
   for (;;);
}

/* CANFD instance object */
extern ARM_DRIVER_CAN  Driver_CANFD;
static ARM_DRIVER_CAN* CANFD_instance    = &Driver_CANFD;

/* File Global variables */
volatile bool bus_error                  = false;
volatile bool passive_mode               = false;
volatile bool rx_buf_overrun             = false;
bool          stop_execution             = false;
volatile bool rx_msg_error               = false;
uint8_t       tx_obj_id                  = 255U;
uint8_t       rx_obj_id                  = 255U;
ARM_CAN_MSG_INFO tx_msg_header;
ARM_CAN_MSG_INFO rx_msg_header;
uint8_t          tx_msg_size             = 0U;
volatile uint8_t rx_msg_size             = 0U;
uint8_t tx_data[CANFD_MAX_MSG_SIZE + 1U] = "!!!!!!***** CANFD TESTAPP Message Communication Test *****!!!!!!";
uint8_t rx_data[CANFD_MAX_MSG_SIZE + 1U];

/* A map between Data length code to the payload size */
const uint8_t canfd_len_dlc_map[0x10U] =
              {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 12U, 16U, 20U, 24U, 32U, 48U, 64U};

/* Support functions */
static bool canfd_process_rx_message(void);
static bool canfd_transmit_message(void);
static void canfd_check_error(void);

/**
 * @fn      static int32_t pinmux_config(void)
 * @brief   CANFD Rx and Tx pinmux configuration.
 * @note    none
 * @param   none
 * @retval  execution status.
 */
static int32_t pinmux_config(void)
{
    int32_t ret_val = 0U;

    /* pinmux configurations for CANFD pins */
    ret_val = pinconf_set(PORT_7, PIN_0, PINMUX_ALTERNATE_FUNCTION_7,
                         (PADCTRL_READ_ENABLE | PADCTRL_SCHMITT_TRIGGER_ENABLE |
                          PADCTRL_OUTPUT_DRIVE_STRENGTH_12_MILI_AMPS));
    if(ret_val)
    {
        printf("ERROR: Failed to configure PINMUX for CANFD Rx \r\n");
        return ret_val;
    }

    ret_val = pinconf_set(PORT_7, PIN_1, PINMUX_ALTERNATE_FUNCTION_7,
                         (PADCTRL_OUTPUT_DRIVE_STRENGTH_12_MILI_AMPS |
                          PADCTRL_SCHMITT_TRIGGER_ENABLE));
    if(ret_val)
    {
        printf("ERROR: Failed to configure PINMUX for CANFD Tx \r\n");
        return ret_val;
    }
    return ret_val;
}

/**
 * @fn      void cb_unit_event(uint32_t event)
 * @brief   CANFD Callback function for events
 * @note    none
 * @param   event: CANFD event
 * @retval  none
 */
void cb_unit_event(uint32_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult = pdFALSE;

    if(event == ARM_CAN_EVENT_UNIT_ACTIVE)
    {
        passive_mode = false;
    }
    else if(event == ARM_CAN_EVENT_UNIT_WARNING)
    {
        /* Set bus error flag when bus warning occurred */
        bus_error = true;
    }
    else if(event == ARM_CAN_EVENT_UNIT_PASSIVE)
    {
        /* Set passive mode flag when bus passive error occurred */
        passive_mode = true;
    }
    else if(event == ARM_CAN_EVENT_UNIT_BUS_OFF)
    {
        /* Set bus error flag when bus is OFF */
        bus_error = true;
    }

    if(bus_error || passive_mode)
    {
        /* Communication error occurred */
        xResult = xTaskNotifyFromISR(canfd_xHandle, CANFD_ERROR,
                           eSetBits, &xHigherPriorityTaskWoken);

        if(xResult == pdTRUE)
        {
            portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
        }
    }
}

/**
 * @fn      void cb_object_event(uint32_t obj_idx, uint32_t event)
 * @brief   CANFD Callback function for particular object events
 * @note    none
 * @param   obj_idx : Object ID
 * @param   event   : CANFD event
 * @retval  none
 */
void cb_object_event(uint32_t obj_idx, uint32_t event)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE, xResult = pdFALSE;

    if((event & ARM_CAN_EVENT_RECEIVE) || (event & ARM_CAN_EVENT_RECEIVE_OVERRUN))
    {
        /* Invokes Message Read function if the Receive Object matches */
        if(obj_idx == rx_obj_id)
        {
            /*  Reading arrived CAN Message */
            if(CANFD_instance->MessageRead(obj_idx, &rx_msg_header,
                                rx_data, rx_msg_size) != ARM_DRIVER_OK)
            {
                rx_msg_error = true;
            }

            if(event & ARM_CAN_EVENT_RECEIVE_OVERRUN)
            {
                rx_buf_overrun = true;
            }

            /* Rx Success - Notify the task*/
            xResult = xTaskNotifyFromISR(canfd_xHandle, CANFD_RX_SUCCESS,
                               eSetBits, &xHigherPriorityTaskWoken);

            if(xResult == pdTRUE)
            {
                portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
            }
        }
    }
}

/**
 * @fn      void canfd_lbe_demo_task(void *pvParameters)
 * @brief   CANFD External Loopback Demo
 * @note    none
 * @param   none
 * @retval  none
 */
void canfd_lbe_demo_task(void *pvParameters)
{
    int32_t ret_val                 = ARM_DRIVER_OK;
    ARM_CAN_CAPABILITIES              can_capabilities;
    ARM_CAN_OBJ_CAPABILITIES          can_obj_capabilities;
    uint8_t iter                    = 0U;
    bool cur_sts                    = true;
    uint32_t task_notified_value    = 0U;
    uint32_t error_code             = 0U;
    uint32_t service_error_code     = 0U;

    /* Initialize the SE services */
    se_services_port_init();

    /* Enables the HFOSC clock */
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle,
                                              CLKEN_HFOSC,
                                              true,
                                              &service_error_code);
    if(error_code)
        printf("SE Error: HFOSC clk enable = %d\n", error_code);

    /* Enables the 160MHz clock */
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle,
                                              CLKEN_CLK_160M,
                                              true,
                                              &service_error_code);
    if(error_code)
        printf("SE Error: 160 MHz clk enable = %d\n", error_code);

    printf("*** CANFD External Loopback Demo app is starting ***\n");

    ret_val = pinmux_config();
    if(ret_val != ARM_DRIVER_OK)
    {
        printf("Error in pin-mux configuration\n");
        return;
    }

    /* Get CANFD capabilities */
    can_capabilities = CANFD_instance->GetCapabilities();
    printf("Num of objects supported: %d\r\n", can_capabilities.num_objects);

    /* Initializing CANFD Access struct */
    ret_val = CANFD_instance->Initialize(cb_unit_event, cb_object_event);
    if(ret_val != ARM_DRIVER_OK)
    {
        printf("ERROR: Failed to initialize the CANFD \n");
        return;
    }

    /* Powering up CANFD */
    ret_val = CANFD_instance->PowerControl(ARM_POWER_FULL);
    if(ret_val != ARM_DRIVER_OK)
    {
        printf("ERROR: Failed to Power up the CANFD \n");
        goto uninitialise_canfd;
    }

    /* Setting CANFD to FD mode */
    if(can_capabilities.fd_mode == 1U)
    {
        CANFD_instance->Control(ARM_CAN_SET_FD_MODE, ENABLE);
        if(ret_val != ARM_DRIVER_OK)
        {
           printf("ERROR: CANFD Enabling FD mode failed\r\n");
           goto power_off_canfd;
        }
    }

    /* Initializing up CANFD module */
    ret_val = CANFD_instance->SetMode(ARM_CAN_MODE_INITIALIZATION);
    if(ret_val != ARM_DRIVER_OK)
    {
        printf("ERROR: Failed to set CANFD to INIT mode \r\n");
        goto power_off_canfd;
    }
    /* Setting bit rate for CANFD */
    ret_val = CANFD_instance->SetBitrate(ARM_CAN_BITRATE_NOMINAL,
              CANFD_NOMINAL_BITRATE, CANFD_NOMINAL_BITTIME_SEGMENTS);
    if(ret_val != ARM_DRIVER_OK)
    {
       printf("ERROR: Failed to set CANFD Nominal Bitrate\r\n");
       goto power_off_canfd;
    }
    /* Setting bit rate for CANFD */
    if(can_capabilities.fd_mode == 1U)
    {
        ret_val = CANFD_instance->SetBitrate(ARM_CAN_BITRATE_FD_DATA,
                  CANFD_FAST_BITRATE, CANFD_FAST_BITTIME_SEGMENTS);
        if(ret_val != ARM_DRIVER_OK)
        {
           printf("ERROR: Failed to set CANFD Fast Bitrate\r\n");
           goto power_off_canfd;
        }
    }
    /* Assign IDs to Tx and Rx objects */
    for(iter = 0U; iter < can_capabilities.num_objects; iter++)
    {
        can_obj_capabilities = CANFD_instance->ObjectGetCapabilities(iter);
        if((can_obj_capabilities.tx == 1U) && (tx_obj_id == 255U))
        {
            tx_obj_id = iter;
        }
        else if((can_obj_capabilities.rx == 1U) && (rx_obj_id == 255U))
        {
            rx_obj_id = iter;
        }
    }

    ret_val = CANFD_instance->ObjectConfigure(tx_obj_id, ARM_CAN_OBJ_TX);
    if(ret_val != ARM_DRIVER_OK)
    {
       printf("ERROR: Object Tx configuration failed\r\n");
       goto power_off_canfd;
    }

    ret_val = CANFD_instance->ObjectConfigure(rx_obj_id, ARM_CAN_OBJ_RX);
    if(ret_val != ARM_DRIVER_OK)
    {
       printf("ERROR: Object Rx configuration failed\r\n");
       goto power_off_canfd;
    }
    /* Setting Object filter of CANFD */
    ret_val = CANFD_instance->ObjectSetFilter(rx_obj_id, ARM_CAN_FILTER_ID_EXACT_ADD,
                                CANFD_OBJECT_FILTER_CODE_1, CANFD_OBJECT_FILTER_MASK);
    if(ret_val == ARM_DRIVER_ERROR_SPECIFIC)
    {
       printf("ERROR: No free Filter available\r\n");
    }
    else if(ret_val != ARM_DRIVER_OK)
    {
       printf("ERROR: Failed to set CANFD Object filter\r\n");
       goto power_off_canfd;
    }

    /* Setting Object filter of CANFD */
    ret_val = CANFD_instance->ObjectSetFilter(rx_obj_id, ARM_CAN_FILTER_ID_EXACT_ADD,
                                CANFD_OBJECT_FILTER_CODE_2, CANFD_OBJECT_FILTER_MASK);
    if(ret_val == ARM_DRIVER_ERROR_SPECIFIC)
    {
       printf("ERROR: No free Filter available\r\n");
    }
    else if(ret_val != ARM_DRIVER_OK)
    {
       printf("ERROR: Failed to set CANFD Object filter\r\n");
       goto power_off_canfd;
    }

    /* Setting CANFD to Normal mode */
    ret_val = CANFD_instance->SetMode(ARM_CAN_MODE_LOOPBACK_EXTERNAL);
    if(ret_val != ARM_DRIVER_OK)
    {
       printf("ERROR: Failed to set CANFD to External Loopback mode\r\n");
       goto power_off_canfd;
    }

    while(!(stop_execution))
    {
        /* Checks current status is Ok*/
        if(cur_sts)
        {
            cur_sts = false;
            /* Invoke the below function to prepare and send a message */
            if(canfd_transmit_message() != false)
            {
                /* wait for receive/error callback. */
                if(xTaskNotifyWait(NULL, CANFD_ALL_NOTIFICATIONS,
                   &task_notified_value, portMAX_DELAY) != pdFALSE)
                {
                    /* Checks if both callbacks are successful */
                    if(task_notified_value & CANFD_RX_SUCCESS)
                    {
                        /* Invokes received message process function */
                        cur_sts = canfd_process_rx_message();
                    }
                    else if(task_notified_value & CANFD_ERROR)
                    {
                        /* Invoke the below function to check on errors */
                        canfd_check_error();
                        /* Sets the flag to stop the execution */
                        stop_execution = true;
                    }
                }
            }
        }
    }

power_off_canfd:
/* Powering OFF CANFD module */
    if(CANFD_instance->PowerControl(ARM_POWER_OFF) != ARM_DRIVER_OK)
    {
       printf("ERROR in CANFD power off\r\n");
    }

uninitialise_canfd:
    /*  Un-initialising CANFD module */
    if(CANFD_instance->Uninitialize() != ARM_DRIVER_OK)
    {
        printf("ERROR in CANFD un-initialization\r\n");
    }

    /* Disables the HFOSC clock */
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle,
                                              CLKEN_HFOSC,
                                              false,
                                              &service_error_code);
    if(error_code)
        printf("SE Error: HFOSC clk disable = %d\n", error_code);

    /* Disables the 160MHz clock */
    error_code = SERVICES_clocks_enable_clock(se_services_s_handle,
                                              CLKEN_CLK_160M,
                                              false,
                                              &service_error_code);
    if(error_code)
        printf("SE Error: 160 MHz clk disable = %d\n", error_code);

    printf("*** CANFD External Loopback Demo is ended ***\r\n");

    /* Task delete */
    vTaskDelete( NULL );
}

/**
 * @fn      int main()
 * @brief   main function of the CANFD Application
 * @note    none
 * @param   none
 * @retval  software execution status
 */
int main()
{
    /* System Initialization */
    SystemCoreClockUpdate();

 #if defined(RTE_Compiler_IO_STDOUT_User)
    int32_t ret;
    ret = stdout_init();
    if(ret != ARM_DRIVER_OK)
    {
        while(1)
        {
        }
    }
 #endif

   /* Create application main thread */
   BaseType_t xReturned = xTaskCreate(canfd_lbe_demo_task, "CANFD_LBEM_Task",
                                      CANFD_TASK_STACK_SIZE, NULL,
                                      (configMAX_PRIORITIES - 1U), &canfd_xHandle);
   if(xReturned != pdPASS)
   {
      vTaskDelete(canfd_xHandle);
      return -1;
   }

   /* Start thread execution */
   vTaskStartScheduler();

   return 0;
}

/**
 * @fn      void canfd_check_error(void)
 * @brief   Checks for the errors in CANFD
 * @note    none
 * @param   none
 * @retval  none
 */
void canfd_check_error(void)
{
    ARM_CAN_STATUS cur_sts;

    if(bus_error)
    {
        /* Getting the current CANFD status */
        cur_sts = CANFD_instance->GetStatus();

        if(cur_sts.unit_state == ARM_CAN_UNIT_STATE_BUS_OFF)
        {
            printf("Error: CANFD Bus OFF:\r\n");
        }
        printf("Error in CANFD-->Error Code:%d\r\n", cur_sts.last_error_code);

        bus_error = false;
    }
    /* If canfd is in passive mode then raise an error */
    if(passive_mode)
    {
        printf("Error: CANFD In Error Passive mode:\r\n");
    }
}

/**
 * @fn      bool canfd_process_rx_message(void)
 * @brief   Processes the received messages
 * @note    none
 * @param   none
 * @retval  none
 */
bool canfd_process_rx_message(void)
{
    uint8_t iter = 0U;

    if(rx_msg_error)
    {
        printf("Error in reading message \r\n");
        rx_msg_error = false; /* Discard the message */
        stop_execution = true;
        return false;
    }
    else
    {
        /* Checking if a new message is received. If yes
         * performs the below operations */
        if(rx_msg_header.rtr == 1U)
        {
            printf("Rx msg:\r\n    Type:Remote frame, Id:%lu",
                   (rx_msg_header.id & (~ARM_CAN_ID_IDE_Msk)));
        }
        else
        {
            printf("Rx msg:\r\n    Type:Data frame, ");

            /* Checks if expected Rx msg length is equal to actual length */
            if(rx_msg_size == canfd_len_dlc_map[rx_msg_header.dlc])
            {
                /* If transmitted message is matching with received message:
                 * performs the following operation */
                if(memcmp(tx_data, rx_data, rx_msg_size) == 0U)
                {
                    /* If any error is present in the Rx message */
                    if(rx_msg_header.esi)
                    {
                        printf("\r\n    Error Occurred in Rx message \r\n");
                        stop_execution = true;
                        return false;
                    }

                    printf("Id:%lu, Len:%d:\r\n    Data:",
                           (rx_msg_header.id & (~ARM_CAN_ID_IDE_Msk)), rx_msg_size);
                    for(iter = 0; iter < rx_msg_size; iter++)
                    {
                        printf("%c", rx_data[iter]);
                    }
                }
                else
                {
                    printf("\r\n    Error: Tx and Rx message mismatch \r\n");
                    stop_execution = true;
                    return false;
                }
            }
            else
            {
                printf("\r\n    Error: Rx msg length is not as expected\r\n");
                stop_execution = true;
                return false;
            }
        }
        printf("\r\n");

        if(rx_buf_overrun)
        {
            printf("Error: Receiver buffer overrun \r\n");
            rx_buf_overrun = false;
        }
    }
    return true;
}

/**
 * @fn      bool canfd_transmit_message(void)
 * @brief   Prepares and sends message
 * @note    none
 * @param   none
 * @retval  none
 */
bool canfd_transmit_message()
{
    static CANFD_FRAME msg_type = CANFD_FRAME_STD_ID_CLASSIC_DATA;
    uint32_t status = ARM_DRIVER_OK;
    uint8_t iter;

    /* If the previous message is successfully sent 7 seconds ago,
     * then prepaer and transmit next message */
    switch(msg_type)
    {
        case CANFD_FRAME_STD_ID_CLASSIC_DATA:
            /* Sending Classic CAN DATA message of
             * length 5 bytes with Message Id 0x5A5 */
            tx_msg_header.brs = 0x0U;
            tx_msg_header.dlc = 0x5U;
            tx_msg_header.id  = 0x5A5U;
            tx_msg_header.rtr = 0x0U;
            tx_msg_header.edl = 0x0U;
            tx_msg_size       = 0x5U;
            break;
        case CANFD_FRAME_STD_ID_RTR:
            /* Sending Classic CAN Remote request message
             * with Message Id 0x5A5 */
            tx_msg_header.brs = 0x0U;
            tx_msg_header.dlc = 0x0U;
            tx_msg_header.id  = 0x5A5U;
            tx_msg_header.rtr = 0x1U;
            tx_msg_header.edl = 0x0U;
            tx_msg_size       = 0x0U;
            break;
        case CANFD_FRAME_STD_ID_FD_DATA:
            /* Sending FD CAN DATA message of
             * length 64 bytes with Message Id 0x5A5 */
            tx_msg_header.brs = 0x1U;
            tx_msg_header.dlc = 0xFU;
            tx_msg_header.id  = 0x5A5U;
            tx_msg_header.rtr = 0x0U;
            tx_msg_header.edl = 0x1U;
            tx_msg_size       = 0x40U;
            break;
        case CANFD_FRAME_EXT_ID_RTR:
            /* Sending Classic CAN Remote request message
             * with Extended Message Id 0x1FF5A5AU */
            tx_msg_header.brs = 0x0U;
            tx_msg_header.dlc = 0x0U;
            tx_msg_header.id  = 0x81FF5A5AU;
            tx_msg_header.rtr = 0x1U;
            tx_msg_header.edl = 0x0U;
            tx_msg_size       = 0x0U;
            break;
        case CANFD_FRAME_EXT_ID_CLASSIC_DATA:
            /* Sending Classic CAN data message of
             * length 8 bytes with Extended Message Id 0x1FF5A5AU */
            tx_msg_header.brs = 0x0U;
            tx_msg_header.dlc = 0x8U;
            tx_msg_header.id  = 0x81FF5A5AU;
            tx_msg_header.rtr = 0x0U;
            tx_msg_header.edl = 0x0U;
            tx_msg_size       = 0x8U;
            break;
        case CANFD_FRAME_EXT_ID_FD_DATA:
            /* Sending FD CAN message of length 16 bytes
             * with Extended Message Id 0x1FF5A5AU */
            tx_msg_header.brs = 0x1U;
            tx_msg_header.dlc = 0xAU;
            tx_msg_header.id  = 0x81FF5A5AU;
            tx_msg_header.rtr = 0x0U;
            tx_msg_header.edl = 0x1U;
            tx_msg_size       = 0x10U;
            break;

        case CANFD_FRAME_OVER:
        default:
            stop_execution = true;
            return false;
    }
    /* Sends the message to CAN HAL Driver */
    status = CANFD_instance->MessageSend(tx_obj_id, &tx_msg_header,
                                         tx_data, tx_msg_size);
    if(status == ARM_DRIVER_OK)
    {
        rx_msg_size = tx_msg_size;
        printf("Tx Msg:\r\n    Id:%lu, Len:%d: \r\n    Data:",
                (tx_msg_header.id & (~ARM_CAN_ID_IDE_Msk)), tx_msg_size);
        for(iter = 0; iter < tx_msg_size; iter++)
        {
            printf("%c", tx_data[iter]);
        }
        printf("\r\n");
        msg_type++;
    }
    else
    {
        printf("Error: Failed to send message \n");
        stop_execution = true;
        return false;
    }
    return true;
}
