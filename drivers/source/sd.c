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
 * @file     sd.c
 * @author   Deepak Kumar
 * @email    deepak@alifsemi.com
 * @version  V0.0.1
 * @date     28-Nov-2022
 * @brief    SD Driver APIs.
 * @bug      None.
 * @Note     None
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "sd.h"
#include "sys_ctrl_sd.h"
#include "string.h"

/* Global SD Driver Callback definitions */
const diskio_t SD_Driver =
{
    sd_init,
    sd_uninit,
    sd_state,
    sd_read,
    sd_write,
};

/* Global SD Handle */
sd_handle_t Hsd;

/**
  \fn           SDErrorHandler
  \brief        SD Driver error Handler
  \param[in]    SD error number
  \return       sd driver state
  */
SD_DRV_STATUS sd_error_handler(){
    return ~SD_DRV_STATUS_OK;
}

/**
  \fn           SD_host_init
  \brief        initialize host controller
  \param[in]    SD Global Handle pointer
  \return       sd driver status
  */
SD_DRV_STATUS sd_host_init(sd_handle_t *pHsd){

    uint8_t powerlevel;

    /* Enable SDMMC Clock */
    enable_sd_periph_clk();

    /* set some default values */
    pHsd->regs      = SDMMC;
    pHsd->state     = SD_CARD_STATE_INIT;

    /* Get the Host Controller version */
    pHsd->hc_version = *((volatile uint16_t *)(SDMMC_HC_VERSION_REG)) & SDMMC_HC_VERSION_REG_Msk;

    /* Get the Host Controller Capabilities */
    hc_get_capabilities(pHsd, &pHsd->hc_caps);

    /* Disable the SD Voltage supply */
    hc_set_bus_power(pHsd, 0x0);

    /* Soft reset Host controller cmd and data lines */
    hc_reset(pHsd, (uint8_t)(SDMMC_SW_RST_ALL_Msk));

    /* Get the host voltage capability */
    if ((pHsd->hc_caps & SDMMC_HOST_SD_CAP_VOLT_3V3_Msk) != 0U) {
        powerlevel = SDMMC_PC_BUS_VSEL_3V3_Msk;
    } else if ((pHsd->hc_caps & SDMMC_HOST_SD_CAP_VOLT_3V0_Msk) != 0U) {
        powerlevel = SDMMC_PC_BUS_VSEL_3V0_Msk;
    } else if ((pHsd->hc_caps & SDMMC_HOST_SD_CAP_VOLT_1V8_Msk) != 0U) {
        powerlevel = SDMMC_PC_BUS_VSEL_1V8_Msk;
    } else {
        powerlevel = 0U;
    }

    hc_set_bus_power(pHsd, (uint8_t)(powerlevel | SDMMC_PC_BUS_PWR_VDD1_Msk));
    hc_set_tout(pHsd, 0xE);

#ifdef SDMMC_ADMA2_MODE
    hc_config_dma(pHsd, (uint8_t)(SDMMC_HOST_CTRL1_ADMA32_MODE_Msk | SDMMC_HOST_CTRL1_DMA_SEL_1BIT_MODE));
#else
    hc_config_dma(pHsd, (uint8_t)(SDMMC_HOST_CTRL1_SDMA_MODE | SDMMC_HOST_CTRL1_DMA_SEL_1BIT_MODE));
#endif

    return SD_DRV_STATUS_OK;
}

/**
  \fn           SD_card_init
  \brief        initialize card
  \param[in]    sd global handle pointer
  \return       sd driver status
  */
SD_DRV_STATUS sd_card_init(sd_handle_t *pHsd){

    uint16_t reg;

    /* Default settings */
    pHsd->bus_width          = SDMMC_1_BIT_WIDTH;
    pHsd->sd_card.cardtype   = SDMMC_CARD_SDHC;
    pHsd->sd_card.busspeed   = SDMMC_CLK_400_KHZ;

    reg = SDMMC_CLK_GEN_SEL_Msk | SDMMC_INIT_CLK_DIVSOR_Msk |
          SDMMC_PLL_EN_Msk | SDMMC_CLK_EN_Msk | SDMMC_INTERNAL_CLK_EN_Msk;

    hc_set_clk_freq(pHsd, reg);

    pHsd->state = SD_CARD_STATE_IDLE;

    sys_busy_loop_us(100);

    /* Reset the command structure */
    pHsd->sd_cmd.cmdidx       = 0;
    pHsd->sd_cmd.arg          = 0;
    pHsd->sd_cmd.xfer_mode    = 0;

    /* Check and wait till the card is present and Reset It */
    if(hc_identify_card(pHsd) != SDMMC_HC_STATUS_OK){
        /* Card Not present */
        return SD_DRV_STATUS_TIMEOUT_ERR;
    }

    /* Reset the SD/UHS Cards */
    if(hc_go_idle(pHsd) != SDMMC_HC_STATUS_OK){
        return SD_DRV_STATUS_CARD_INIT_ERR;
    }

    sys_busy_loop_us(100);

    /* Get the card interface condition */
    if(hc_get_card_ifcond(pHsd) != SDMMC_HC_STATUS_OK){
        return SD_DRV_STATUS_CARD_INIT_ERR;
    }

    /* Get the card operating condition */
    if(hc_get_card_opcond(pHsd) != SDMMC_HC_STATUS_OK){
        return SD_DRV_STATUS_CARD_INIT_ERR;
    }

    if(!(pHsd->sd_card.sdio_mode)){

        /* Get the card ID CMD2 */
        if(hc_get_card_cid(pHsd) != SDMMC_HC_STATUS_OK){
            return SD_DRV_STATUS_CARD_INIT_ERR;
        }
    }

    /* Get the card Relative Address CMD3 */
    if(hc_get_rca(pHsd, &pHsd->sd_card.relcardadd) != SDMMC_HC_STATUS_OK){
        return SD_DRV_STATUS_CARD_INIT_ERR;
    }

    if(!(pHsd->sd_card.sdio_mode)){

        /* Get the CSD register */
        if(hc_get_card_csd(pHsd)!= SDMMC_HC_STATUS_OK){
            return SD_DRV_STATUS_CARD_INIT_ERR;
        }

        /* Change the Card State from Identification to Ready */
        pHsd->state = SD_CARD_STATE_STBY;

    }

    /* Select a card */
    if(hc_sel_card(pHsd, pHsd->sd_card.relcardadd)!= SDMMC_HC_STATUS_OK){
        return SD_DRV_STATUS_CARD_INIT_ERR;
    }

    if(!(pHsd->sd_card.sdio_mode)){
#ifdef SDMMC_4BIT_MODE
        if(hc_set_bus_width(pHsd, SDMMC_HOST_CTRL1_4_BIT_WIDTH) != SDMMC_HC_STATUS_OK){
            return SD_DRV_STATUS_CARD_INIT_ERR;
        }
#endif

        if(hc_set_blk_size(pHsd, SDMMC_BLK_SIZE_512_Msk)!= SDMMC_HC_STATUS_OK){
            return SD_DRV_STATUS_CARD_INIT_ERR;
        }
    }

    reg = SDMMC_OP_CLK_DIVSOR_Msk | SDMMC_CLK_GEN_SEL_Msk | SDMMC_PLL_EN_Msk |
          SDMMC_CLK_EN_Msk | SDMMC_INTERNAL_CLK_EN_Msk;
    hc_set_clk_freq(pHsd, reg);

    return SD_DRV_STATUS_OK;
}

/**
  \fn           SD_init
  \brief        main SD initialize function
  \param[in]    device ID
  \return       sd driver status
  */
SD_DRV_STATUS sd_init(uint8_t devId)
{
    ARG_UNUSED(devId);
    SD_DRV_STATUS errcode = SD_DRV_STATUS_OK;

    /* Initialize Host controller */
    errcode = sd_host_init(&Hsd);

    if(errcode != SD_DRV_STATUS_OK){
        return SD_DRV_STATUS_HOST_INIT_ERR;
    }

    /* Initialize SD Memory/Combo Card */
    errcode = sd_card_init(&Hsd);

    if(errcode != SD_DRV_STATUS_OK)
        return SD_DRV_STATUS_CARD_INIT_ERR;

    return SD_DRV_STATUS_OK;
}

/**
  \fn           SD_uninit
  \brief        SD uninitialize function
  \param[in]    device ID
  \return       sd driver status
  */
SD_DRV_STATUS sd_uninit(uint8_t devId)
{
    ARG_UNUSED(devId);

    /* release the card */
    sd_handle_t *pHsd           = &Hsd;
    pHsd->sd_cmd.cmdidx         = CMD7;
    pHsd->sd_cmd.data_present   = 0;
    pHsd->sd_cmd.arg            = 0xFFFF0000; //any other RCA to perform card de-selection

    if(hc_send_cmd(pHsd, &pHsd->sd_cmd) != SDMMC_HC_STATUS_OK){
        sd_error_handler();
    }

    /* turn off power supply to the card */
    hc_set_bus_power(pHsd, (uint8_t)(0));
    return SD_DRV_STATUS_OK;
}

/**
  \fn           sd_state
  \brief        sd state
  \param[in]    Global SD handle pointer
  \return       sd state
  */
SD_CARD_STATE sd_state(sd_handle_t *pHsd){
    uint32_t status;

    hc_get_card_status(pHsd, &status);

    return status;
}

/**
  \fn           SD_DRV_STATUS SD_read(uint32_t sec, uint32_t BlkCnt, volatile unsigned char * DestBuff)
  \brief        read sd sector
  \param[in]    sec - input sector number to read
  \param[in]    BlkCnt - number of block to read
  \param[in]    DestBuff - Destination buffer pointer
  \return       sd driver status
  */
SD_DRV_STATUS sd_read(uint32_t sec, uint16_t BlkCnt, volatile unsigned char *DestBuff){

    sd_handle_t *pHsd =  &Hsd;
    uint32_t timeout_cnt = 2000 * BlkCnt;
    uint8_t retryCnt = 1;

    if(DestBuff == NULL)
        return SD_DRV_STATUS_RD_ERR;

#ifdef SDMMC_PRINTF_DEBUG
    printf("SD READ Dest Buff: 0x%p Sec: %u, Block Count: %u\n",DestBuff,sec,BlkCnt);
#endif

    /* Change the Card State from Tran to Data */
    pHsd->state = SD_CARD_STATE_DATA;

retry:
    hc_read_setup(pHsd, (uint32_t)LocalToGlobal(DestBuff), sec, BlkCnt);

    if(hc_check_xfer_done(pHsd, timeout_cnt) == SDMMC_HC_STATUS_OK)
        RTSS_InvalidateDCache_by_Addr(DestBuff, BlkCnt * SDMMC_BLK_SIZE_512_Msk);
    else{
        /* Soft reset Host controller cmd and data lines */
        hc_reset(pHsd, (uint8_t)(SDMMC_SW_RST_DAT_Msk | SDMMC_SW_RST_CMD_Msk));

        if(!retryCnt--)
            return SD_DRV_STATUS_RD_ERR;
        goto retry;
    }

    /* Change the Card State from Data to Tran */
    pHsd->state = SD_CARD_STATE_TRAN;

#ifdef SDMMC_PRINT_SEC_DATA
    int j = 0;
    int *p = DestBuff;

    while(j<(128*BlkCnt)){
        printf("0x%08x: %08x %08x %08x %08x\n",j*4, p[j+0], p[j+1], p[j+2], p[j+3]);
        j += 4;
    }
#endif

    return SD_DRV_STATUS_OK;
}

/**
  \fn           SD_DRV_STATUS SD_write(uint32_t sec, uint32_t BlkCnt, volatile unsigned char * SrcBuff)
  \brief        Write sd sector
  \param[in]    sector - input sector number to write
  \param[in]    BlkCnt - number of block to write
  \param[in]    SrcBuff - Source buffer pointer
  \return       sd driver status
  */
SD_DRV_STATUS sd_write(uint32_t sector, uint32_t BlkCnt, volatile unsigned char *SrcBuff){

    sd_handle_t *pHsd =  &Hsd;
    int timeout_cnt = 2000 * BlkCnt;
    uint8_t retryCnt=1;
    if(SrcBuff == NULL)
        return SD_DRV_STATUS_WR_ERR;

#ifdef SDMMC_PRINTF_DEBUG
    printf("SD WRITE Src Buff: 0x%p Sec: %d, Block Count: %d\n",SrcBuff,sector,BlkCnt);
#endif

    /* Clean the DCache */
    RTSS_CleanDCache_by_Addr(SrcBuff, BlkCnt * SDMMC_BLK_SIZE_512_Msk);

retry:
    hc_write_setup(pHsd, (uint32_t)LocalToGlobal(SrcBuff), sector, BlkCnt);

    if(hc_check_xfer_done(pHsd, timeout_cnt) != SDMMC_HC_STATUS_OK){
        hc_reset(pHsd, (uint8_t)(SDMMC_SW_RST_DAT_Msk | SDMMC_SW_RST_CMD_Msk));

        if(!retryCnt--)
            return SD_DRV_STATUS_WR_ERR;
        goto retry;
    }

#ifdef SDMMC_PRINT_SEC_DATA
    int j = 0;
    int *p = SrcBuff;

    while(j<(128*BlkCnt)){
        printf("0x%08x: %08x %08x %08x %08x\n",j*4, p[j+0], p[j+1], p[j+2], p[j+3]);
        j += 4;
    }
#endif

    return SD_DRV_STATUS_OK;
}

