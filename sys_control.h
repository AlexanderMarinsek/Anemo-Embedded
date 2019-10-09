#ifndef SYS_CONTROL_H
#define SYS_CONTROL_H


/* Configure system and set data formater based on the modules in use */
/*#define SYS_CONFING (						\
		SYS_SERIAL_DATA_MASK | \
		SYS_WIND_DATA_MASK | \
		SYS_ENV_DATA_MASK | \
		SYS_EL_DATA_MASK \
		)*/
#define SYS_CONFING	(					\
		SYS_SERIAL_DATA_MASK | \
		SYS_WIND_DATA_MASK | \
		SYS_ENV_DATA_MASK | \
		SYS_EL_DATA_MASK \
		)


/* Dont forget to change formater (no spaces)! */
//#define DATA_FORMATER "{%s}"			// wind
//#define DATA_FORMATER "{%s,%s}"		// wind, env
#define DATA_FORMATER "{%s,%s,%s}"	// wind, env, el
//#define DATA_FORMATER "{%s,%s,%s,%s}"	// wind, env, el, dv

/* Direction offset from north in degrees * 10e1 */
#define NORTH_OFFSET_10E1			1575U

/* Set up timer */
#define DATA_SEND_PERIOD_MIN		1U


/******************************************************************************/

/* Macros related to system configuration */
#define SYS_WIND_DATA_MASK				(1U << 8)		// 0x0100
#define SYS_ENV_DATA_MASK				(1U << 9)		// 0x0200
#define SYS_EL_DATA_MASK				(1U << 10)		// 0x0400
#define SYS_DV_DATA_MASK				(1U << 11)		// 0x0800

#define SYS_SERIAL_DATA_MASK			(1U << 12)		// 0x1000
#define SYS_DATA_STORAGE_MASK			(1U << 13)		// 0x2000
#define SYS_LORA_DATA_MASK				(1U << 14)		// 0x4000


/* Macros related to timer setup */
#if !defined (US_PER_SEC)
	#define US_PER_SEC					1000000
#endif

#define ATIMER_PERIOD_S			(3U)
#define ATIMER_FREQ				(15625U)
#define COOKIE            	  	(100U)		/* Dummy argument for callback */
#define ATIMER_CALIBRATION		170
//#define ATIMER_PERIOD_TICKS     ((ATIMER_FREQ * ATIMER_PERIOD_S) - 1)
#define ATIMER_PERIOD_TICKS     ((ATIMER_FREQ * ATIMER_PERIOD_S) - ATIMER_CALIBRATION)

#define TIMER_PERIOD_S				3U
#define TIMER_FREQ					1000000U
#define COOKIE              		100U
#define TIMER_PERIOD_US     		TIMER_PERIOD_S * US_PER_SEC

#define TICKS_PER_PERIOD			\
		DATA_SEND_PERIOD_MIN * SEC_PER_MIN / TIMER_PERIOD_S


#endif
