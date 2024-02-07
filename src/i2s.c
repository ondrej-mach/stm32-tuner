/*
 * Code taken from https://github.com/1Bitsy/1bitsy-examples
 */

#include "i2s.h"

#include <libopencm3/cm3/memorymap.h>
#include <libopencm3/cm3/nvic.h>

#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/spi.h>


#define PLLI2S_N_VALUE 384
#define PLLI2S_R_VALUE 5
#define I2SPR_I2SDIV_VALUE 12
#define I2SPR_ODD_VALUE 1

#define BUFFER_SAMPLES FRAME_SAMPLES
static int32_t dacBuffer[2][BUFFER_SAMPLES] __attribute__((aligned(4)));
static int32_t adcBuffer[2][BUFFER_SAMPLES] __attribute__((aligned(4)));

static AudioProcessor *audioProcFn;

static void init_clocks(void) {
    rcc_periph_clock_enable(RCC_SPI2);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
    rcc_periph_clock_enable(RCC_DMA1);
	rcc_periph_clock_enable(RCC_DMA2);
}

static void init_gpio(void) {
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

    // TODO probably not needed
    // gpio_set_output_options(GPIOB, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO12 | GPIO13 | GPIO15);
	// gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_25MHZ, GPIO6);
}

static void init_i2s(void) {
    rcc_plli2s_config(PLLI2S_N_VALUE, PLLI2S_R_VALUE);

    // Clock prescaler setup
    SPI_I2SPR(SPI2) = I2SPR_ODD_VALUE << 8
        | I2SPR_I2SDIV_VALUE;

    // I2S protocol configuration
	// ADC
    SPI_I2SCFGR(I2S2_EXT_BASE) = SPI_I2SCFGR_I2SMOD
        | SPI_I2SCFGR_I2SCFG_SLAVE_RECEIVE << SPI_I2SCFGR_I2SCFG_LSB
        | SPI_I2SCFGR_I2SSTD_I2S_PHILIPS << SPI_I2SCFGR_I2SSTD_LSB       // TODO
        | SPI_I2SCFGR_CKPOL
        | SPI_I2SCFGR_DATLEN_32BIT << SPI_I2SCFGR_DATLEN_LSB;
	// DAC
    SPI_I2SCFGR(SPI2) = SPI_I2SCFGR_I2SMOD
        | SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT << SPI_I2SCFGR_I2SCFG_LSB
        | SPI_I2SCFGR_I2SSTD_I2S_PHILIPS << SPI_I2SCFGR_I2SSTD_LSB       // TODO
        | SPI_I2SCFGR_CKPOL
        | SPI_I2SCFGR_DATLEN_32BIT << SPI_I2SCFGR_DATLEN_LSB; // TODO



    // Start the I2S PLL.
    RCC_CR |= RCC_CR_PLLI2SON;

    // Enable I2S.
    SPI_I2SCFGR(SPI2) |= SPI_I2SCFGR_I2SE;
    SPI_I2SCFGR(I2S2_EXT_BASE) |= SPI_I2SCFGR_I2SE;
}

static void init_dma(void) {
    // Configure the DMA engine to stream data to the DAC
	dma_stream_reset(DMA1, DMA_STREAM4);
	dma_set_peripheral_address(DMA1, DMA_STREAM4, (intptr_t)&SPI_DR(SPI2));
	dma_set_memory_address(DMA1, DMA_STREAM4, (intptr_t)dacBuffer[0]); // TODO might be completely wrong
	dma_set_memory_address_1(DMA1, DMA_STREAM4, (intptr_t)dacBuffer[1]); // same here
	dma_set_number_of_data(DMA1, DMA_STREAM4, BUFFER_SAMPLES*2);
	dma_channel_select(DMA1, DMA_STREAM4, DMA_SxCR_CHSEL_0);
	dma_set_transfer_mode(DMA1, DMA_STREAM4, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_16BIT);
	dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_16BIT);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
	dma_enable_double_buffer_mode(DMA1, DMA_STREAM4);
	dma_enable_stream(DMA1, DMA_STREAM4);

	// Configure the DMA engine to stream data from the ADC
	dma_stream_reset(DMA1, DMA_STREAM3);
	dma_set_peripheral_address(DMA1, DMA_STREAM3, (intptr_t)&SPI_DR(I2S2_EXT_BASE));
	dma_set_memory_address(DMA1, DMA_STREAM3, (intptr_t)adcBuffer[0]); // TODO
	dma_set_memory_address_1(DMA1, DMA_STREAM3, (intptr_t)adcBuffer[1]);
	dma_set_number_of_data(DMA1, DMA_STREAM3, BUFFER_SAMPLES*2);
	dma_channel_select(DMA1, DMA_STREAM3, DMA_SxCR_CHSEL_3);
	dma_set_transfer_mode(DMA1, DMA_STREAM3, DMA_SxCR_DIR_PERIPHERAL_TO_MEM);
	dma_set_memory_size(DMA1, DMA_STREAM3, DMA_SxCR_MSIZE_16BIT);
	dma_set_peripheral_size(DMA1, DMA_STREAM3, DMA_SxCR_PSIZE_16BIT);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM3);
	dma_enable_double_buffer_mode(DMA1, DMA_STREAM3);
	dma_enable_stream(DMA1, DMA_STREAM3);

    // Interrupt only for input stream
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM3);
	nvic_enable_irq(NVIC_DMA1_STREAM3_IRQ);
	nvic_set_priority(NVIC_DMA1_STREAM3_IRQ, 0x80); // 0 is most urgent

	// Start transmitting data
	spi_enable_rx_dma(I2S2_EXT_BASE);
	spi_enable_tx_dma(SPI2);
}


#define SWAP_HALFWORDS(w) (int32_t)(((uint32_t)(w))<<16 | ((uint32_t)(w))>>16)

void dma1_stream3_isr(void) {
	dma_clear_interrupt_flags(DMA1, DMA_STREAM3, DMA_TCIF);

	int32_t *outBuffer = dacBuffer[!dma_get_target(DMA1, DMA_STREAM4)];
	int32_t *inBuffer = adcBuffer[!dma_get_target(DMA1, DMA_STREAM3)];

	for (int i=0; i<BUFFER_SAMPLES; i++) {
		inBuffer[i] = SWAP_HALFWORDS(inBuffer[i]);
	}
	if (audioProcFn) {
		(*audioProcFn)(inBuffer, outBuffer);
    }
    for (int i=0; i<BUFFER_SAMPLES; i++) {
		outBuffer[i] = SWAP_HALFWORDS(outBuffer[i]);;
	}

}

extern void init_codec(void) {
    init_clocks();
    init_gpio();
    init_i2s();
    init_dma();

}

extern void set_processing_function(AudioProcessor *fn) {
    audioProcFn = fn;
}

