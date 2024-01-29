/*
 * Code taken from https://github.com/1Bitsy/1bitsy-examples
 */

#include "i2s.h"

#include <assert.h>
#include <stdio.h>

#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>


static i2s_sample_callback *spi2_callback;
static i2s_channel spi2_left_right;

static i2s_sample_callback *spi3_callback;
static i2s_channel spi3_left_right;


#define PLLI2S_N_VALUE 258
#define PLLI2S_R_VALUE 3
#define I2SPR_I2SDIV_VALUE 3
#define I2SPR_ODD_VALUE SPI_I2SPR_ODD 1


extern void init_i2s(const i2s_config *cfg, const i2s_instance *inst,
                     i2s_sample_callback *callback)
{
    (void)cfg;
    uint32_t base = inst->i2si_base_address;
    
    // Enable clock to needed peripheries
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_DMA1); // TODO
	rcc_periph_clock_enable(RCC_DMA2);

    // TODO
    spi2_callback = callback;
    spi2_left_right = I2SC_LEFT;

    // Enable GPIO pins
    // PB9 -> WS (LR clock)
    // PB10 -> Bus CLK
    // PB14 -> ext_SD (INPUT)
    // PB15 -> SD (OUTPUT)
    // PC6 -> Master CLK
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO9|GPIO10|GPIO15);
	gpio_set_af(GPIOB, GPIO_AF5, GPIO9|GPIO10|GPIO15);
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO14);
	gpio_set_af(GPIOB, GPIO_AF6, GPIO14);
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
	gpio_set_af(GPIOC, GPIO_AF5, GPIO6);

    // Enable interrupt.
    nvic_enable_irq(NVIC_SPI2_IRQ);

    SPI2_I2SCFGR  = 0;

    RCC_PLLI2SCFGR     = PLLI2S_N_VALUE << RCC_PLLI2SCFGR_PLLI2SN_SHIFT
                       | PLLI2S_R_VALUE << RCC_PLLI2SCFGR_PLLI2SR_SHIFT;

    // Clock prescaler setup
    SPI2_I2SPR    = SPI_I2SPR_MCKOE             // Output master CLK
                       | I2SPR_ODD_VALUE
                       | I2SPR_I2SDIV_VALUE;
    
    // I2S protocol configuration
    SPI2_I2SCFGR  = SPI_I2SCFGR_I2SMOD
                       | SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT
                         << SPI_I2SCFGR_I2SCFG_LSB
                       | SPI_I2SCFGR_I2SSTD_I2S_PHILIPS        // TODO
                         << SPI_I2SCFGR_I2SSTD_LSB             // TODO
                       | SPI_I2SCFGR_CKPOL
                       | SPI_I2SCFGR_DATLEN_24BIT
                         << SPI_I2SCFGR_DATLEN_LSB;

    // DMA and interrupts
    SPI2_CR2    |= SPI_CR2_TXEIE;

    // Start the I2S PLL.
    RCC_CR |= RCC_CR_PLLI2SON;

    // Enable I2S.
    SPI2_I2SCFGR |= SPI_I2SCFGR_I2SE;
}

void spi2_isr()
{
    int16_t next_sample = 0;
    if (spi2_callback)
        next_sample = (*spi2_callback)(spi2_left_right);
    spi2_left_right = !spi2_left_right;
    SPI2_DR = SPI2_DR; //next_sample;
    uint16_t sr = SPI2_SR;
    (void)sr;
    // if (sr & ~(SPI_SR_BSY | SPI_SR_CHSIDE))
    //     fprintf(stderr, " ISR: SPI2_SR = %#x\n", sr);
}
