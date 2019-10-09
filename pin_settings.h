#ifndef PIN_SETTINGS_H
#define PIN_SETTINGS_H

#include "periph/gpio.h"
#include "periph/adc.h"

#define RHOMB_ZERO					0
#define SAMD21_XPRO					1
#define ARDUINO_DUE					2



#if (BOARD_NUMBER == RHOMB_ZERO)

#define ATIMER_DEV							TIMER_DEV(0)

//#define EL_DATA_RE1_PIN		 				GPIO_PIN(PA, 17)	// 13
//#define EL_DATA_RE2_PIN		 				GPIO_PIN(PA, 19)	// 12
//#define EL_DATA_RE3_PIN		 				GPIO_PIN(PA, 16)	// 11
#define EL_DATA_RE1_PIN		 				GPIO_PIN(PA, 5)	// 13
#define EL_DATA_RE2_PIN		 				GPIO_PIN(PB, 9)	// 12
#define EL_DATA_RE3_PIN		 				GPIO_PIN(PB, 2)	// 11
#define EL_DATA_I2C_DEV		 				I2C_DEV(0)

#define ANEMO_DAVIS_MUX_OUT					GPIO_PIN(PA, 21)	// 7
#define ANEMO_DAVIS_MUX_C					GPIO_PIN(PA, 20)	// 6
#define ANEMO_DAVIS_MUX_B					GPIO_PIN(PA, 15)	// 5
#define ANEMO_DAVIS_MUX_A					GPIO_PIN(PA, 14)	// 4
#define ANEMO_DAVIS_COUNTER_RST				GPIO_PIN(PA, 9)		// 3
#define ANEMO_DAVIS_COUNTER_N_EN			GPIO_PIN(PA, 8)		// 2
#define ANEMO_DAVIS_ADC_LINE				ADC_LINE(0)			// A0


#elif (BOARD_NUMBER == SAMD21_XPRO)

#define ATIMER_DEV							TIMER_DEV(0)

#define EL_DATA_RE1_PIN		 				GPIO_PIN(PA, 16)
#define EL_DATA_RE2_PIN		 				GPIO_PIN(PA, 17)
#define EL_DATA_RE3_PIN		 				GPIO_PIN(PA, 18)
#define EL_DATA_I2C_DEV		 				I2C_DEV(0)

#define ANEMO_DAVIS_MUX_OUT					GPIO_PIN(PA, 10)
#define ANEMO_DAVIS_MUX_C					GPIO_PIN(PA, 21)
#define ANEMO_DAVIS_MUX_B					GPIO_PIN(PA, 20)
#define ANEMO_DAVIS_MUX_A					GPIO_PIN(PA, 11)
#define ANEMO_DAVIS_COUNTER_RST				GPIO_PIN(PB, 12)
#define ANEMO_DAVIS_COUNTER_N_EN			GPIO_PIN(PB, 13)

#define ANEMO_DAVIS_ADC_LINE				ADC_LINE(0)



#elif (BOARD_NUMBER == ARDUINO_DUE)

#define ATIMER_DEV							TIMER_DEV(1)

#define ANEMO_DAVIS_MUX_OUT					GPIO_PIN(PC, 23)
#define ANEMO_DAVIS_MUX_C					GPIO_PIN(PC, 24)
#define ANEMO_DAVIS_MUX_B					GPIO_PIN(PC, 25)
#define ANEMO_DAVIS_MUX_A					GPIO_PIN(PC, 26)
#define ANEMO_DAVIS_COUNTER_RST				GPIO_PIN(PC, 28)
#define ANEMO_DAVIS_COUNTER_N_EN			GPIO_PIN(PB, 25)
#define ANEMO_DAVIS_ADC_LINE				ADC_LINE(7)



#endif /* BOARD_NUMBER */



#endif /* PIN_SETTINGS */

