/* Copyright (C) 2023 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/**************************************************************************//**
 * @file     sd.h
 * @author   Deepak Kumar
 * @email    deepak@alifsemi.com
 * @version  V0.0.1
 * @date     28-Nov-2022
 * @brief    exposed SD Driver variables and APIs.
 * @bug      None.
 * @Note     None
 ******************************************************************************/
#ifndef _SD_H_
#define _SD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "sd_host.h"
#include "sdio.h"

//#define SDMMC_PRINTF_DEBUG
//#define SDMMC_PRINTF_SD_STATE_DEBUG
//#define SDMMC_PRINT_SEC_DATA
#define SDMMC_4BIT_MODE
//#define SDMMC_8BIT_MODE
#define SDMMC_ADMA2_MODE

/**
 * @brief  SD driver status enum definition
 */
typedef enum _SD_DRV_STATUS{
    SD_DRV_STATUS_OK,
    SD_DRV_STATUS_HOST_INIT_ERR,
    SD_DRV_STATUS_CARD_INIT_ERR,
    SD_DRV_STATUS_RD_ERR,
    SD_DRV_STATUS_WR_ERR,
    SD_DRV_STATUS_TIMEOUT_ERR
}SD_DRV_STATUS;

/**
 * @brief  SD Card status enum definition
 */
typedef enum _SD_CARD_STATE{
    SD_CARD_STATE_INIT = -1,
    SD_CARD_STATE_IDLE,
    SD_CARD_STATE_READY,
    SD_CARD_STATE_IDENT,
    SD_CARD_STATE_STBY,
    SD_CARD_STATE_TRAN,
    SD_CARD_STATE_DATA,
    SD_CARD_STATE_RCV,
    SD_CARD_STATE_PRG,
    SD_CARD_STATE_DIS,
    SD_CARD_STATE_RESV
}SD_CARD_STATE;

/**
 * @brief  SD Card Information Structure definition
 */
typedef struct _sd_cardinfo_t{
    uint32_t cardtype;          /*!< Specifies the card Type                        */
    uint32_t cardversion;       /*!< Specifies the card version                     */
    uint32_t relcardadd;        /*!< Specifies the Relative Card Address            */
    uint32_t sectorcount;       /*!< Specifies the Card Capacity in blocks          */
    uint32_t sectorsize;        /*!< Specifies one block size in bytes              */
    uint32_t logblocknbr;       /*!< Specifies the Card logical Capacity in blocks  */
    uint32_t logblocksize;      /*!< Specifies logical block size in bytes          */
    uint32_t busspeed;          /*!< Clock                                          */
    uint16_t card_class;        /*!< Specifies the class of the card class          */
    uint8_t iscardpresent;      /*!< is card present flag                           */
    uint8_t f8flag;             /*!< CMD8 support flag, set after good resp of CMD8 */
    uint8_t sdio_mode;          /*!< sdio only mode flag                            */
    sdio_t sdio;                /*!< sdio card information                          */
}sd_cardinfo_t;

/**
 * @brief  SD command structure definition
 */
typedef struct _sd_cmd_t{
    uint32_t arg;           /*!< SD Command Argument        */
    uint16_t xfer_mode;     /*!< SD Command transfer mode   */
    uint8_t cmdidx;         /*!< SD Command index           */
    uint8_t data_present;   /*!< SD Command uses Data lines */
}sd_cmd_t;

/**
 * @brief  Global SD Handle Information Structure definition
 */
typedef struct _sd_handle_t{
    SDMMC_Type              *regs;          /*!< SD controller registers base address   */
    sd_cmd_t                sd_cmd;         /*!< SD Command info                        */
    sd_cardinfo_t           sd_card;        /*!< SD Card information                    */
    uint32_t                hc_caps;        /*!< Host Controller capabilities           */
    __IO uint32_t           context;        /*!< SD transfer context                    */
    __IO uint32_t           error_code;     /*!< SD Card Error codes                    */
    uint32_t                csd[4];         /*!< SD card specific data table            */
    uint32_t                cid[4];         /*!< SD card identification number table    */
    uint32_t                scr[2];         /*!< SD card Configuration Register         */
    SD_CARD_STATE           state;          /*!< SD card State                          */
    uint16_t                hc_version;     /*!< Host controller version                */
    uint8_t                 bus_width;      /*!< 1Bit, 4Bit, 8Bit Mode                  */
}sd_handle_t;

/**
 * @brief  Disk IO Driver structure definition
 */
typedef struct _diskio_t{
    SD_DRV_STATUS (*disk_initialize)    (uint8_t);                                  /*!< Initialize Disk Drive      */
    SD_DRV_STATUS (*disk_uninitialize)  (uint8_t);                                  /*!< Un-initialize Disk Drive   */
    SD_CARD_STATE (*disk_status)        (sd_handle_t *);                            /*!< Get Disk Status            */
    SD_DRV_STATUS (*disk_read)          (uint32_t, uint16_t, volatile uint8_t *);   /*!< Read Sector(s)             */
    SD_DRV_STATUS (*disk_write)         (uint32_t, uint32_t, volatile uint8_t *);   /*!< Write Sector(s)            */
}diskio_t;

extern const diskio_t SD_Driver;

/* SD Driver function forward declaration */
SD_CARD_STATE sd_state(sd_handle_t *);
SD_DRV_STATUS sd_init(uint8_t);
SD_DRV_STATUS sd_uninit(uint8_t);
SD_DRV_STATUS sd_io_init(sd_handle_t *);
SD_DRV_STATUS sd_host_init(sd_handle_t *);
SD_DRV_STATUS sd_card_init(sd_handle_t *);
SD_DRV_STATUS sd_write(uint32_t, uint32_t, volatile unsigned char *);
SD_DRV_STATUS sd_read(uint32_t, uint16_t, volatile unsigned char *);
SD_DRV_STATUS sd_error_handler();
SDMMC_HC_STATUS hc_send_cmd(sd_handle_t *, sd_cmd_t *);
SDMMC_HC_STATUS hc_reset(sd_handle_t *, uint8_t);
void hc_set_bus_power(sd_handle_t *, uint8_t);
SDMMC_HC_STATUS hc_set_clk_freq(sd_handle_t *, uint16_t);
void hc_set_tout(sd_handle_t *, uint8_t);
SDMMC_HC_STATUS hc_identify_card(sd_handle_t *);
SDMMC_HC_STATUS hc_get_card_ifcond(sd_handle_t *);
SDMMC_HC_STATUS hc_get_card_opcond(sd_handle_t *);
SDMMC_HC_STATUS hc_get_card_cid(sd_handle_t *);
SDMMC_HC_STATUS hc_get_card_csd(sd_handle_t *);
SDMMC_HC_STATUS hc_config_dma(sd_handle_t *, uint8_t);
SDMMC_HC_STATUS hc_set_bus_width(sd_handle_t *, uint8_t);
SDMMC_HC_STATUS hc_switch_1v8(sd_handle_t *);
SDMMC_HC_STATUS hc_go_idle(sd_handle_t *);
SDMMC_HC_STATUS hc_sel_card(sd_handle_t *, uint32_t);
SDMMC_HC_STATUS hc_set_blk_size(sd_handle_t *, uint32_t);
SDMMC_HC_STATUS hc_set_blk_cnt(sd_handle_t *, uint32_t);
SDMMC_HC_STATUS hc_read_setup(sd_handle_t *, uint32_t , uint32_t , uint16_t);
SDMMC_HC_STATUS hc_write_setup(sd_handle_t *, uint32_t , uint32_t , uint16_t);
SDMMC_HC_STATUS hc_check_xfer_done(sd_handle_t *, uint32_t);
SDMMC_HC_STATUS hc_get_rca(sd_handle_t *, uint32_t *);
SDMMC_HC_STATUS hc_get_card_status(sd_handle_t *pHsd, uint32_t *);
SDMMC_HC_STATUS hc_get_capabilities(sd_handle_t *, uint32_t *);
uint32_t hc_get_response1(sd_handle_t *);
uint32_t hc_get_response2(sd_handle_t *);
uint32_t hc_get_response3(sd_handle_t *);
uint32_t hc_get_response4(sd_handle_t *);
SDMMC_HC_STATUS hc_io_reset(sd_handle_t *);
SDMMC_HC_STATUS hc_check_bus_idle(sd_handle_t *);
SDMMC_HC_STATUS hc_get_io_opcond(sd_handle_t *, uint32_t, uint32_t *);
SDMMC_HC_STATUS hc_io_rw_direct(sd_handle_t *pHsd, uint32_t rwFlag, uint32_t fn, \
         uint32_t addr, uint8_t writeData, uint8_t *readPtr);

#ifdef __cplusplus
}
#endif

#endif
