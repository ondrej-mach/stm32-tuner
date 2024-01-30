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


#define PLLI2S_N_VALUE 258
#define PLLI2S_R_VALUE 3
#define I2SPR_I2SDIV_VALUE 3
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
    spi_reset(SPI2); // probably not needed

    RCC_PLLI2SCFGR = PLLI2S_N_VALUE << RCC_PLLI2SCFGR_PLLI2SN_SHIFT
        | PLLI2S_R_VALUE << RCC_PLLI2SCFGR_PLLI2SR_SHIFT;

    // Clock prescaler setup
    SPI_I2SPR(SPI2) = SPI_I2SPR_MCKOE  // Output master CLK
        | I2SPR_ODD_VALUE
        | I2SPR_I2SDIV_VALUE;

    // I2S protocol configuration
    SPI_I2SCFGR(SPI2) = SPI_I2SCFGR_I2SMOD
        | SPI_I2SCFGR_I2SCFG_MASTER_TRANSMIT << SPI_I2SCFGR_I2SCFG_LSB
        | SPI_I2SCFGR_I2SSTD_I2S_PHILIPS << SPI_I2SCFGR_I2SSTD_LSB       // TODO
        | SPI_I2SCFGR_CKPOL
        | SPI_I2SCFGR_DATLEN_24BIT << SPI_I2SCFGR_DATLEN_LSB; // TODO

    SPI_I2SCFGR(I2S2_EXT_BASE) = SPI_I2SCFGR_I2SMOD
        | SPI_I2SCFGR_I2SCFG_SLAVE_RECEIVE << SPI_I2SCFGR_I2SCFG_LSB;

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
	dma_set_number_of_data(DMA1, DMA_STREAM4, BUFFER_SAMPLES);
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
	dma_set_number_of_data(DMA1, DMA_STREAM3, BUFFER_SAMPLES);
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


void dma1_stream3_isr(void) {
	dma_clear_interrupt_flags(DMA1, DMA_STREAM3, DMA_TCIF);

	int32_t *outBuffer = dma_get_target(DMA1, DMA_STREAM4) ?
	                         (void*)dacBuffer[0] :
	                         (void*)dacBuffer[1];
	const int32_t *inBuffer = dma_get_target(DMA1, DMA_STREAM3) ?
	                        (void*)adcBuffer[0] :
	                        (void*)adcBuffer[1];

	if (audioProcFn) {
		(*audioProcFn)(inBuffer, outBuffer);
    }

	// samplecounter += CODEC_SAMPLES_PER_FRAME;
	// int32_t framePeakOut = INT16_MIN;
	// int32_t framePeakIn = INT16_MIN;

	// for (unsigned s=0; s<BUFFER_SAMPLES; s++) {
	// 	if (outBuffer->m[s] > framePeakOut) {
	// 		framePeakOut = outBuffer->m[s];
	// 	}
	// 	if (inBuffer->m[s] > framePeakIn) {
	// 		framePeakIn = inBuffer->m[s];
	// 	}
	// }
	// if (framePeakOut > peakOut) {
	// 	peakOut = framePeakOut;
	// }
	// if (framePeakIn > peakIn) {
	// 	peakIn = framePeakIn;
	// }
    //
	// platformFrameFinishedCB();
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

