/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     zylx         first version
 */

#include <rtconfig.h>
#include <string.h>
#include <register.h>
#include "../dfu/dfu.h"
#include "bf0_hal.h"
#include "board.h"
#include "boot_flash.h"
#include "sifli_bbm.h"

QSPI_FLASH_CTX_T spi_flash_handle[FLASH_MAX_INSTANCE];
DMA_HandleTypeDef spi_flash_dma_handle[FLASH_MAX_INSTANCE];
flash_read_func g_flash_read;
flash_write_func g_flash_write;
flash_erase_func g_flash_erase;

flash_read_func g_flash_read_mpi5;
flash_write_func g_flash_write_mpi5;
flash_erase_func g_flash_erase_mpi5;

FLASH_HandleTypeDef *boot_handle;
uint32_t g_config_addr;
static uint32_t nand_pagesize = 2048;
static uint32_t nand_blksize = 0x20000;

int port_read_page(int blk, int page, int offset, uint8_t *buff, uint32_t size, uint8_t *spare, uint32_t spare_len)
{
    int res;
    uint32_t addr = (blk * nand_blksize) + (page * nand_pagesize) + offset;

    FLASH_HandleTypeDef *hflash = (FLASH_HandleTypeDef *)&spi_flash_handle[2].handle;
    if ((offset + size) > nand_pagesize)
    {
        HAL_ASSERT(0);
    }

#if (NAND_BUF_CPY_MODE == 1)
    SCB_InvalidateDCache_by_Addr(buff, size);
#endif
    SCB_InvalidateDCache_by_Addr((void *)hflash->base, nand_pagesize + SPI_NAND_MAXOOB_SIZE);
    if (((blk & 1) != 0) && (hflash->wakeup != 0))
        SCB_InvalidateDCache_by_Addr((void *)(hflash->base + (1 << 12)), SPI_NAND_PAGE_SIZE + SPI_NAND_MAXOOB_SIZE);
    res = HAL_NAND_READ_WITHOOB(hflash, addr, buff, size, spare, spare_len);


    return res; //RET_NOERROR;
}

int bbm_get_bb(int blk)
{
    FLASH_HandleTypeDef *hflash = (FLASH_HandleTypeDef *)&spi_flash_handle[2].handle;

    SCB_InvalidateDCache_by_Addr((void *)hflash->base, nand_pagesize + SPI_NAND_MAXOOB_SIZE);
    if (((blk & 1) != 0) && (hflash->wakeup != 0))
        SCB_InvalidateDCache_by_Addr((void *)(hflash->base + (1 << 12)), SPI_NAND_PAGE_SIZE + SPI_NAND_MAXOOB_SIZE);
    int bad = HAL_NAND_GET_BADBLK(hflash, blk);

    return bad;
}

static int read_nand(uint32_t addr, const int8_t *buf, uint32_t size)
{
    int cnt;
    uint32_t fill, offset;

    cnt = 0;
    offset = addr >= boot_handle->base ? addr - boot_handle->base : addr;
    while (size > 0)
    {
        fill = size > nand_pagesize ? nand_pagesize : size;
        if (bbm_read_page(offset / nand_blksize, (offset / nand_pagesize) & (nand_blksize / nand_pagesize - 1), offset & (nand_pagesize - 1), (uint8_t *)(buf + cnt), fill, NULL, 0) != fill)
            break;
        cnt += fill;
        size -= fill;
        offset += fill;
    }
    return cnt;
}

static int read_nor(uint32_t addr, const int8_t *buf, uint32_t size)
{
    memcpy((void *)buf, (uint8_t *)addr, size);
    return size;
}

static uint32_t nand_cache[(4096 + 128) / 4];
static uint32_t bbm_cache_buf[(4096 + 128) / 4];
static uint32_t init_mpi3(int nand)
{
    qspi_configure_t flash_cfg = FLASH3_CONFIG;
    struct dma_config flash_dma = FLASH3_DMA_CONFIG;

    board_pinmux_mpi3(BOOT_FROM_QFN());
    flash_cfg.line = HAL_FLASH_QMODE ;
    g_flash_read = nand ? read_nand : read_nor;
    if (nand)
    {
        flash_cfg.base += HPSYS_MPI_MEM_CBUS_2_SBUS_OFFSET;
        flash_cfg.SpiMode = SPI_MODE_NAND;
        spi_flash_handle[2].handle.data_buf = (uint8_t *)nand_cache;
    }
    HAL_Delay_us(0);
    spi_flash_handle[2].dual_mode = 1;
    spi_flash_handle[2].flash_mode = nand ? 1 : 0;
    spi_flash_handle[2].handle.freq = 8000000;// 48M / 6
    HAL_StatusTypeDef res = HAL_FLASH_Init(&(spi_flash_handle[2]), &flash_cfg, &spi_flash_dma_handle[2], &flash_dma, BSP_GetFlash3DIV());
    if ((res == HAL_OK) && (nand != 0))
    {
        nand_pagesize = HAL_NAND_PAGE_SIZE(&spi_flash_handle[2].handle);
        nand_blksize = HAL_NAND_BLOCK_SIZE(&spi_flash_handle[2].handle);

        spi_flash_handle[2].handle.buf_mode = 1;    // default set to buffer mode for nand
        HAL_NAND_CONF_ECC(&spi_flash_handle[2].handle, 1); // default enable ECC if support !
        bbm_set_page_size(nand_pagesize);
        bbm_set_blk_size(nand_blksize);
        sif_bbm_init(spi_flash_handle[2].total_size, (uint8_t *)bbm_cache_buf);
    }
    return (spi_flash_handle[2].base_addr);
}

static int write_nor(uint32_t addr, const int8_t *buf, uint32_t size)
{
    FLASH_HandleTypeDef *hflash = &(spi_flash_handle[4].handle);
    uint32_t taddr, start, remain, fill;
    uint8_t *tbuf;
    int res;

    if ((addr < hflash->base) || (addr > (hflash->base + hflash->size)))
        return 0;

    taddr = addr - hflash->base;
    tbuf = (uint8_t *)buf;
    remain = size;

    start = taddr & (QSPI_NOR_PAGE_SIZE - 1);
    if (start > 0)    // start address not page aligned
    {
        fill = QSPI_NOR_PAGE_SIZE - start;   // remained size in one page
        if (fill > size)    // not over one page
        {
            fill = size;
        }
        res = HAL_QSPIEX_WRITE_PAGE(hflash, taddr, tbuf, fill);
        if (res != fill)
            return 0;
        taddr += fill;
        tbuf += fill;
        remain -= fill;
    }
    while (remain > 0)
    {
        fill = remain > QSPI_NOR_PAGE_SIZE ? QSPI_NOR_PAGE_SIZE : remain;
        res = HAL_QSPIEX_WRITE_PAGE(hflash, taddr, tbuf, fill);
        if (res != fill)
            return 0;
        taddr += fill;
        tbuf += fill;
        remain -= fill;
    }

    return size;
}

static int erase_nor(uint32_t addr, uint32_t size)
{
    FLASH_HandleTypeDef *hflash = &(spi_flash_handle[4].handle);
    uint32_t taddr, remain;
    int res;

    if ((addr < hflash->base) || (addr > (hflash->base + hflash->size)))
        return 1;

    taddr = addr - hflash->base;
    remain = size;

    if ((taddr & (QSPI_NOR_SECT_SIZE - 1)) != 0)
        return -1;
    if ((remain & (QSPI_NOR_SECT_SIZE - 1)) != 0)
        return -2;

    while (remain > 0)
    {
        res = HAL_QSPIEX_SECT_ERASE(hflash, taddr);
        if (res != 0)
            return 1;

        remain -= QSPI_NOR_SECT_SIZE;
        taddr += QSPI_NOR_SECT_SIZE;
    }

    return 0;
}


#ifdef SD_BL_MODE
/*****************************SD functions*************************************/
#include "sd_emmc_drv.h"
// todo, as jlink driver, SD1 base used 0xa0000000, and backup sd2 for 0x64000000 as flash 3
#define BOOT_SD_MEM_BASE        (0Xa0000000)
#define BOOT_SD_MEM_OFFSET      (0X1000)

static uint8_t sd_cache[512];

static int read_sdemmc(uint32_t addr, const int8_t *buf, uint32_t size)
{
    // TODO: Add SD read.
    uint32_t offset = addr - BOOT_SD_MEM_BASE;
    uint32_t remain = size;
    uint8_t *data = (uint8_t *)buf;

    while (remain >= 512)
    {
        sd_read_data(offset, data, 512);
        remain -= 512;
        offset += 512;
        data += 512;
    }
    if (remain > 0)
    {
        sd_read_data(offset, sd_cache, 512);
        memcpy(data, sd_cache, remain);
    }
    return size;
}

uint32_t init_sdnand()
{
    int res = 0;
#if 0
    // initial clock
    HAL_HPAON_EnableXT48();
    HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_SYS, RCC_SYSCLK_HXT48);
    HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_HP_PERI, RCC_CLK_PERI_HXT48);
    HAL_RCC_HCPU_SetDiv(1, 1, 5);
#else
    HAL_RCC_HCPU_ClockSelect(RCC_CLK_MOD_SDMMC, RCC_CLK_FLASH_DLL2);
#endif
    HAL_RCC_HCPU_enable2(HPSYS_RCC_ENR2_SDMMC1, 1);

    // set pinmux and europa for power
    board_pinmux_sd1();
    board_sd1_power_on();

    // initial controller and chip
    res = sdmmc_init();
    if (res != 0)
    {
        HAL_ASSERT(0);
    }
    g_flash_read = read_sdemmc;
    return BOOT_SD_MEM_BASE + BOOT_SD_MEM_OFFSET;
}
#endif

static uint32_t init_mpi5()
{
    qspi_configure_t flash_cfg = FLASH5_CONFIG;
    struct dma_config flash_dma = FLASH5_DMA_CONFIG;
    uint16_t div;

    flash_cfg.line = HAL_FLASH_NOR_MODE;
    div = 4; //BSP_GetFlash5DIV();
    spi_flash_handle[4].dual_mode = 1;
    spi_flash_handle[4].flash_mode =   0;
    int res = HAL_FLASH_Init(&(spi_flash_handle[4]), &flash_cfg, &spi_flash_dma_handle[4], &flash_dma, div);
    if (res != 0)
    {
        HAL_ASSERT(0);
    }

    g_flash_read_mpi5 = read_nor;
    g_flash_write_mpi5 = write_nor;
    g_flash_erase_mpi5 = erase_nor;

    return (spi_flash_handle[4].base_addr);
}

/******************************************************************************/

void dfu_flash_init()
{
    g_config_addr = MPI5_MEM_BASE;
    g_flash_read = read_nor;

    init_mpi5();

    if (g_flash_read_mpi5)
    {
        while (1)
        {
            g_flash_read_mpi5(g_config_addr, (const int8_t *)&sec_config_cache, sizeof(struct sec_configuration));
            if (sec_config_cache.magic == SEC_CONFIG_MAGIC)
                break;
            HAL_Delay_us(100000);
        }
    }

#ifndef SD_BL_MODE
    if (BOOT_FROM_NAND())
        board_boot_src = BOOT_FROM_MPI3_NAND;
    else
        board_boot_src = BOOT_FROM_MPI3_NOR;
    BSP_SetFlash3DIV(6);
    init_mpi3(BOOT_FROM_NAND());
    boot_handle = (FLASH_HandleTypeDef *)&spi_flash_handle[2].handle;
#else
//    init_sdnand();
    boot_handle = NULL;
#endif
}


void sec_flash_func_init()
{
    g_flash_read = NULL;
}


