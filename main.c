/*
 * тест библиотеки i2c oled дисплея
 * /

/**********************************************************************
 * Секция include и defines
**********************************************************************/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "si5351.h"
#include "oledi2c.h"
#include "oled_printf.h"
#define MIN_LIMIT 200
#define MAX_LIMIT 170000
/*
11 метров, 25.60 — 26.10 МГц (11,72 — 11,49 метра).
13 метров, 21.45 — 21.85 МГц (13,99 — 13,73 метра).
15 метров, 18.90 — 19.02 МГц (15,87 — 15,77 метра).
16 метров, 17.55 — 18.05 МГц (17,16 — 16,76 метра).
19 метров, 15.10 — 15.60 МГц (19,87 — 18,87 метра).
22 метра, 13.50 — 13.87 МГц (22,22 — 21,63 метра).
25 метров 11.60 — 12.10 МГц (25,86 — 24,79 метра).
31 метр, 9.40 — 9.99 МГц (31,91 — 30,03 метра).
41 метр, 7.20 — 7.50 МГц (41,67 — 39,47 метра).
49 метров, 5.85 — 6.35 МГц (52,36 — 47,66 метра).
60 метров, 4.75 — 5.06 МГц (63,16 — 59,29 метра).
75 метров, 3.90 — 4.00 МГц (76,92 — 75 метров).
90 метров, 3.20 — 3.40 МГц (93,75 — 88,24 метров).
120 метров (средние волны), 2.30 — 2.495 МГц (130,43 — 120,24 метра).
*/
uint16_t band[]={5850,7200,9400,11600,13500,15100,17550};
//#include "si5351.h"
uint32_t encoder=12000;
uint16_t coef=10;

void next_band(){
	static uint8_t i=0;
	uint64_t freq;
	encoder=band[i]*4;//надо переписать значение энкодера
	freq=band[i]*400000ULL;
	if(i<6) i++; else i=0;
	si5351_set_freq(freq, SI5351_CLK0);
	}

void button_setup(){
	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN,
													GPIO3|GPIO4|GPIO5);
	gpio_set(GPIOA,GPIO3|GPIO4|GPIO5);  
}
void led_setup(){
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
	              GPIO_CNF_OUTPUT_PUSHPULL, GPIO8);
	gpio_set(GPIOB,GPIO8);
}


void encoder_init(){
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_AFIO);
	nvic_enable_irq(NVIC_EXTI0_IRQ);
	/* Set GPIO0 (in GPIO port A) to 'input float'. */
	
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT,
													GPIO0|GPIO1|GPIO2);
	
	/* Configure the EXTI subsystem. */
	exti_select_source(EXTI0, GPIOA);
	exti_set_trigger(EXTI0, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI0);
	}

void exti0_isr(void)
{
	static uint8_t n=0;
	n++;
	if(n%2){exti_reset_request(EXTI0);return;}
	//прерывание энкодера
	if(gpio_get(GPIOA,GPIO1)) {if(encoder<MAX_LIMIT-coef) encoder+=coef;}
			else {if(encoder>MIN_LIMIT+coef) encoder-=coef;};
	exti_reset_request(EXTI0);//Флаг прерывания надо сбросить вручную
}



static void i2c_setup(void){
	/* Enable clocks for I2C2 and AFIO. */
	rcc_periph_clock_enable(RCC_I2C1);
	rcc_periph_clock_enable(RCC_AFIO);
	/* Set alternate functions for the SCL and SDA pins of I2C2. */
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ,
		      GPIO_CNF_OUTPUT_ALTFN_OPENDRAIN,
		      GPIO_I2C1_SCL|GPIO_I2C1_SDA);
	//SDA PB7
	//SCL PB6
	/* Disable the I2C before changing any configuration. */
	i2c_peripheral_disable(I2C1);
	//36 is APB1 speed in MHz
	i2c_set_speed(I2C1,i2c_speed_fm_400k,36);
	i2c_peripheral_enable(I2C1);
	}

void set_feq_90(uint64_t freq){
	//Используя si5153 можно непосредственно генерировать два сигнала
	//со сдвигом фаз 90 град.
	uint8_t coef=650000000/freq;
	uint64_t pll_freq=coef*freq;
	// We will output 14.1 MHz on CLK0 and CLK1.
	// A PLLA frequency of 705 MHz was chosen to give an even
	// divisor by 14.1 MHz.
	//unsigned long long freq = 14100000 00ULL;
	//unsigned long long pll_freq = 705000000 00ULL;
	// Set CLK0 and CLK1 to output 14.1 MHz with a fixed PLL frequency
	set_freq_manual(freq*100, pll_freq*100, SI5351_CLK0);
	set_freq_manual(freq*100, pll_freq*100, SI5351_CLK1);
	// Now we can set CLK1 to have a 90 deg phase shift by entering
	// 50 in the CLK1 phase register, since the ratio of the PLL to
	// the clock frequency is 50.
	set_phase(SI5351_CLK0, 0);
	set_phase(SI5351_CLK1, coef);
	// We need to reset the PLL before they will be in phase alignment
	pll_reset(SI5351_PLLA);
	}
	
	
	
	
	
void main(){
	rcc_clock_setup_in_hse_8mhz_out_72mhz();
	led_setup();
//	button_setup();
	i2c_setup();
	oled_init();
	encoder_init();
	gpio_clear(GPIOB,GPIO8);
	oled_clear();
	o_printf("SIS5351 init");
	si5351_init(SI5351_CRYSTAL_LOAD_10PF, 25005500, 0);
	si5351_drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
	//si5351_drive_strength(SI5351_CLK1, SI5351_DRIVE_2MA);
	//si5351_drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);
	//si5351_set_freq(10200000ULL, SI5351_CLK0);
	//si5351_set_freq(90000000*100ULL, SI5351_CLK1);
	//si5351_set_freq(1020000*100ULL, SI5351_CLK2);
	oled_clear();
	uint32_t old_enc=0;
	uint32_t count=0;
	
	while(1){
		if(encoder!=old_enc){
			//set_feq_90(encoder*1000);
			si5351_set_freq(encoder*100000ULL, SI5351_CLK0);
			old_enc=encoder;
			o_printf_at(0,0,1,0,"CNT=%d",count++);
			o_printf_at(0,1,3,0,"%dKHz\n%dKHz",encoder, encoder/4);
			}
		if(!gpio_get(GPIOA,GPIO2)){
			while(!gpio_get(GPIOA,GPIO2));
			coef*=10;
			if(coef>1000)coef=1;
			o_printf_at(0,0,1,0,"coef=>>>%d<<<",coef);
			}
		if(!gpio_get(GPIOA,GPIO3)){
			while(!gpio_get(GPIOA,GPIO3));
			next_band();
			}
		for(uint32_t i=0;i<0xffff;i++)__asm__("nop");
		gpio_toggle(GPIOB,GPIO8);
		}
	}

