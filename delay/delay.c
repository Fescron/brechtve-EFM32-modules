/***************************************************************************//**
 * @file delay.c
 * @brief Delay functionality.
 * @version 3.4
 * @author Brecht Van Eeckhoudt
 *
 * ******************************************************************************
 *
 * @section Versions
 *
 *   @li v1.0: Moved delay functionality from `util.c` to this file.
 *   @li v1.1: Changed global variables to be static (~hidden) and added dbprint `\ n \ r ` fixes.
 *   @li v1.2: Removed EM1 delay method (see note in delayRTCC_EM2).
 *   @li v1.3: Cleaned up includes, added option to select SysTick/EM2 delay functionality
 *             and moved all of the logic in one `delay` method. Renamed sleep method.
 *   @li v1.4: Changed names of static variables, made initRTCcomp static.
 *   @li v1.5: Updated documentation.
 *   @li v1.6: Changed RTCcomp names to RTC.
 *   @li v1.7: Moved IRQ handler of RTC to this file.
 *   @li v1.8: Added ULFRCO logic.
 *   @li v1.9: Updated code with new DEFINE checks.
 *   @li v2.0: Added functionality to enable/disable the clocks only when necessary.
 *   @li v2.1: Added functionality to check if a wakeup was caused by the RTC.
 *   @li v2.2: Removed some clock disabling statements.
 *   @li v2.3: Changed error numbering.
 *   @li v2.4: Moved definitions from header to source file.
 *   @li v2.5: Added functionality to the time spend sleeping.
 *   @li v3.0: Disabled peripheral clock before entering an `error` function, added
 *             functionality to exit methods after `error` call and updated version number.
 *   @li v3.1: Removed `static` before some local variables (not necessary).
 *   @li v3.2: Moved `msTicks` variable and systick handler in `#if` check.
 *   @li v3.3: Added include for the boolean type in the header file, changed `dbwarnInt`
 *             to `dbinfoInt`, added the ability to enable/disable the sleep-announcing.
 *   @li v3.4: Added logic to initialize delay/sleep when calling the methods using `0` as the delay time.
 *
 * ******************************************************************************
 *
 * @todo
 *   **Future improvements:**@n
 *     - Enable/disable clock functionality? (see comments using `//`)
 *
 * ******************************************************************************
 *
 * @section License
 *
 *   **Copyright (C) 2019 - Brecht Van Eeckhoudt**
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the **GNU General Public License** as published by
 *   the Free Software Foundation, either **version 3** of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   *A copy of the GNU General Public License can be found in the `LICENSE`
 *   file along with this source code.*
 *
 *   @n
 *
 *   Some methods use code obtained from examples from [Silicon Labs' GitHub](https://github.com/SiliconLabs/peripheral_examples).
 *   These sections are licensed under the Silabs License Agreement. See the file
 *   "Silabs_License_Agreement.txt" for details. Before using this software for
 *   any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/


#include <stdint.h>        /* (u)intXX_t */
#include <stdbool.h>       /* "bool", "true", "false" */
#include "em_device.h"     /* Include necessary MCU-specific header file */
#include "em_cmu.h"        /* Clock management unit */
#include "em_emu.h"        /* Energy Management Unit */
#include "em_gpio.h"       /* General Purpose IO */
#include "em_rtc.h"        /* Real Time Counter (RTC) */

#include "delay.h"         /* Corresponding header file */
#include "debug_dbprint.h" /* Enable or disable printing to UART */
//#include "util.h"    	   /* Utility functionality (error) */


/* Local definitions (for RTC compare interrupts) */
#define ULFRCOFREQ    1000
#define ULFRCOFREQ_MS 1.000
#define LFXOFREQ      32768
#define LFXOFREQ_MS   32.768


/* Local variables */
/*   -> Volatile because it's modified by an interrupt service routine (@RAM)
 *   -> Static so it's always kept in memory (@data segment, space provided during compile time) */
static volatile bool RTC_sleep_wakeup = false;

#if SYSTICKDELAY == 1 /* SysTick delay selected */
static volatile uint32_t msTicks;
#endif /* SysTick/RTC selection */


bool sleeping = false;
bool RTC_initialized = false;

#if SYSTICKDELAY == 1 /* SysTick delay selected */
bool SysTick_initialized = false;
#endif /* SysTick/RTC selection */


/* Local prototype */
static void initRTC (void);


/**************************************************************************//**
 * @brief
 *   Wait for a certain amount of milliseconds in EM2/3.
 *
 * @details
 *   This method can be called with the argument `0` to force initialization.@n
 *   This method also initializes SysTick/RTC if necessary.
 *
 * @param[in] msDelay
 *   The delay time in **milliseconds**.
 *****************************************************************************/
void delay (uint32_t msDelay)
{

#if SYSTICKDELAY == 1 /* SysTick delay selected */

	/* Initialize SysTick if not already the case */
	if (!SysTick_initialized)
	{
		/* Initialize and start SysTick
		 * Number of ticks between interrupt = cmuClock_CORE/1000 */
		if (SysTick_Config(CMU_ClockFreqGet(cmuClock_CORE) / 1000)) while (1);

#if DEBUG_DBPRINT == 1 /* DEBUG_DBPRINT */
	dbinfo("SysTick initialized");
#endif /* DEBUG_DBPRINT */

		SysTick_initialized = true;
	}
	else
	{
		/* Only execute if we're not initializing */
		if (msDelay > 0)
		{
			/* Enable SysTick interrupt and counter by setting their bits. */
			SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
		}
	}

	/* Only execute if we're not initializing */
	if (msDelay > 0)
	{
		/* Wait a certain amount of ticks */
		uint32_t curTicks = msTicks;
		while ((msTicks - curTicks) < msDelay);

		/* Disable SysTick interrupt and counter (needs to be done before entering EM2/3) by clearing their bits. */
		SysTick->CTRL &= ~SysTick_CTRL_TICKINT_Msk & ~SysTick_CTRL_ENABLE_Msk;
	}

#else /* EM2/3 RTC delay selected */


	/* Initialize RTC if not already the case */
	if (!RTC_initialized) initRTC();
	else
	{
		/* Only execute if we're not initializing */
		if (msDelay > 0)
		{
			/* Enable necessary oscillator and clocks */

#if ULFRCO == 1 /* ULFRCO selected */

			/* No specific code here */

#else /* LFXO selected */

			/* Enable the low-frequency crystal oscillator for the RTC */
			//CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

#endif /* ULFRCO/LFXO selection */

			/* Enable the clock to the interface of the low energy modules
			 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
			//CMU_ClockEnable(cmuClock_HFLE, true);

			/* Turn on the RTC clock */
			CMU_ClockEnable(cmuClock_RTC, true);
		}
	}

	/* Only execute if we're not initializing */
	if (msDelay > 0)
	{

		/* Set RTC compare value for RTC compare register 0 depending on ULFRCO/LFXO selection */

#if ULFRCO == 1 /* ULFRCO selected */

		if ((ULFRCOFREQ_MS * msDelay) <= 0x00ffffff) RTC_CompareSet(0, (ULFRCOFREQ_MS * msDelay));
		else
		{

#if DEBUG_DBPRINT == 1 /* DEBUG_DBPRINT */
			dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUG_DBPRINT */

			/* Turn off the RTC clock */
			CMU_ClockEnable(cmuClock_RTC, false);

//			error(14);

			/* Exit function */
			return;
		}

#else /* LFXO selected */

		if ((LFXOFREQ_MS * msDelay) <= 0x00ffffff) RTC_CompareSet(0, (LFXOFREQ_MS * msDelay));
		else
		{

#if DEBUG_DBPRINT == 1 /* DEBUG_DBPRINT */
			dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUG_DBPRINT */

			/* Turn off the RTC clock */
			CMU_ClockEnable(cmuClock_RTC, false);

//			error(15);

			/* Exit function */
			return;
		}

#endif /* ULFRCO/LFXO selection */


		/* Start the RTC */
		RTC_Enable(true);

		/* Enter EM2/3 depending on ULFRCO/LFXO selection */

#if ULFRCO == 1 /* ULFRCO selected */
		/* In EM3, high and low frequency clocks are disabled. No oscillator (except the ULFRCO) is running.
		 * Furthermore, all unwanted oscillators are disabled in EM3. This means that nothing needs to be
		 * manually disabled before the statement EMU_EnterEM3(true); */
		EMU_EnterEM3(true); /* "true" - Save and restore oscillators, clocks and voltage scaling */
#else /* LFXO selected */
		EMU_EnterEM2(true); /* "true" - Save and restore oscillators, clocks and voltage scaling */
#endif /* ULFRCO/LFXO selection */


		/* Disable used oscillator and clocks after wake-up */

#if ULFRCO == 1 /* ULFRCO selected */

		/* No specific code here */

#else /* LFXO selected */

		/* Disable the low-frequency crystal oscillator for the RTC */
		//CMU_OscillatorEnable(cmuOsc_LFXO, false, true);

#endif /* ULFRCO/LFXO selection */

		/* Disable the clock to the interface of the low energy modules
		 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
		//CMU_ClockEnable(cmuClock_HFLE, false);

		/* Turn off the RTC clock */
		CMU_ClockEnable(cmuClock_RTC, false);
	}

#endif /* SysTick/RTC selection */

}


/**************************************************************************//**
 * @brief
 *   Sleep for a certain amount of seconds in EM2/3.
 *
 * @details
 *   This method also initializes the RTC if necessary.
 *
 * @param[in] sSleep
 *   The sleep time in **seconds**.
 *****************************************************************************/
void sleep (uint32_t sSleep)
{
	/* Initialize RTC if not already the case */
	if (!RTC_initialized) initRTC();
	else
	{
		/* Only execute if we're not initializing */
		if (sSleep > 0)
		{

			/* Enable necessary oscillator and clocks */

#if ULFRCO == 1 /* ULFRCO selected */

			/* No specific code here */

#else /* LFXO selected */

			/* Enable the low-frequency crystal oscillator for the RTC */
			//CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

#endif /* ULFRCO/LFXO selection */

			/* Enable the clock to the interface of the low energy modules
			 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
			//CMU_ClockEnable(cmuClock_HFLE, true);

			/* Turn on the RTC clock */
			CMU_ClockEnable(cmuClock_RTC, true);
		}
	}

	/* Only execute if we're not initializing */
	if (sSleep > 0)
	{

#if DEBUG_DBPRINT == 1 /* DEBUG_DBPRINT */
#if ANNOUNCE_SLEEPING == 1 /* Announce sleeping enabled */
#if ULFRCO == 1 /* ULFRCO selected */
		dbinfoInt("Sleeping in EM3 for ", sSleep, " s\n\r\n\r");
#else /* LFXO selected */
		dbinfoInt("Sleeping in EM2 for ", sSleep, " s\n\r\n\r");
#endif /* ULFRCO/LFXO selection */
#endif /* Announce sleeping */
#endif /* DEBUG_DBPRINT */

		/* Set RTC compare value for RTC compare register 0 depending on ULFRCO/LFXO selection */

#if ULFRCO == 1 /* ULFRCO selected */

		if ((ULFRCOFREQ * sSleep) <= 0x00ffffff) RTC_CompareSet(0, (ULFRCOFREQ * sSleep));
		else
		{

#if DEBUG_DBPRINT == 1 /* DEBUG_DBPRINT */
			dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUG_DBPRINT */

			/* Turn off the RTC clock */
			CMU_ClockEnable(cmuClock_RTC, false);

//			error(16);

			/* Exit function */
			return;
		}

#else /* LFXO selected */

		if ((LFXOFREQ * sSleep) <= 0x00ffffff) RTC_CompareSet(0, (LFXOFREQ * sSleep));
		else
		{

#if DEBUG_DBPRINT == 1 /* DEBUG_DBPRINT */
			dbcrit("Delay too long, can't fit in the field!");
#endif /* DEBUG_DBPRINT */

			/* Turn off the RTC clock */
			CMU_ClockEnable(cmuClock_RTC, false);

//			error(17);

			/* Exit function */
			return;
		}

#endif /* ULFRCO/LFXO selection */

		/* Indicate that we're using the sleep method */
		sleeping = true;


		/* Start the RTC */
		RTC_Enable(true);

		/* Enter EM2/3 depending on ULFRCO/LFXO selection */

#if ULFRCO == 1 /* ULFRCO selected */
		/* In EM3, high and low frequency clocks are disabled. No oscillator (except the ULFRCO) is running.
		 * Furthermore, all unwanted oscillators are disabled in EM3. This means that nothing needs to be
		 * manually disabled before the statement EMU_EnterEM3(true); */
		EMU_EnterEM3(true); /* "true" - Save and restore oscillators, clocks and voltage scaling */
#else /* LFXO selected */
		EMU_EnterEM2(true); /* "true" - Save and restore oscillators, clocks and voltage scaling */
#endif /* ULFRCO/LFXO selection */

		/* Indicate that we're no longer sleeping */
		sleeping = false;

		/* Disable used oscillator and clocks after wake-up */

#if ULFRCO == 1 /* ULFRCO selected */

		/* No specific code here */

#else /* LFXO selected */

		/* Disable the low-frequency crystal oscillator for the RTC */
		//CMU_OscillatorEnable(cmuOsc_LFXO, false, true);

#endif /* ULFRCO/LFXO selection */

		/* Disable the clock to the interface of the low energy modules
		 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
		//CMU_ClockEnable(cmuClock_HFLE, false);

		/* Turn off the RTC clock */
		CMU_ClockEnable(cmuClock_RTC, false);
	}
}


/**************************************************************************//**
 * @brief
 *   Method to check if the wakeup was caused by the RTC.
 *
 * @return
 *   The value of `RTC_sleep_wakeup`.
 *****************************************************************************/
bool RTC_checkWakeup (void)
{
	return (RTC_sleep_wakeup);
}


/**************************************************************************//**
 * @brief
 *   Method to clear `RTC_sleep_wakeup`.
 *****************************************************************************/
void RTC_clearWakeup (void)
{
	RTC_sleep_wakeup = false;
}


/**************************************************************************//**
 * @brief
 *   Method to get the time spend sleeping (in seconds) in the case
 *   of GPIO wake-up.
 *
 * @return
 *   The time spend sleeping in **seconds**.
 *****************************************************************************/
uint32_t RTC_getPassedSleeptime (void)
{
	uint32_t sSleep = RTC_CounterGet();

	/* Disable the counter */
	RTC_Enable(false);

#if ULFRCO == 1 /* ULFRCO selected */
		sSleep /= ULFRCOFREQ;
#else /* LFXO selected */
		sSleep /= LFXOFREQ;
#endif /* ULFRCO/LFXO selection */

	return (sSleep);
}


/**************************************************************************//**
 * @brief
 *   RTC initialization.
 *
 * @note
 *   This is a static method because it's only internally used in this file
 *   and called by other methods if necessary.
 *****************************************************************************/
static void initRTC (void)
{

#if ULFRCO == 1 /* ULFRCO selected */

	/* Enable the ultra low-frequency RC oscillator for the RTC */
	//CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true); /* The ULFRCO is always on */

	/* Enable the clock to the interface of the low energy modules
	 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
	CMU_ClockEnable(cmuClock_HFLE, true);

	/* Route the ULFRCO clock to the RTC */
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);

#else /* LFXO selected */

	/* Enable the low-frequency crystal oscillator for the RTC */
	CMU_OscillatorEnable(cmuOsc_LFXO, true, true);

	/* Enable the clock to the interface of the low energy modules
	 * cmuClock_CORELE = cmuClock_HFLE (deprecated) */
	CMU_ClockEnable(cmuClock_HFLE, true);

	/* Route the LFXO clock to the RTC */
	CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_LFXO);

#endif /* ULFRCO/LFXO selection */

	/* Turn on the RTC clock */
	CMU_ClockEnable(cmuClock_RTC, true);

	/* Allow channel 0 to cause an interrupt */
	RTC_IntEnable(RTC_IEN_COMP0);
	RTC_IntClear(RTC_IFC_COMP0); /* This statement was in the ULFRCO but not in the LFXO example. It's kept here just in case. */
	NVIC_ClearPendingIRQ(RTC_IRQn);
	NVIC_EnableIRQ(RTC_IRQn);

	/* Configure the RTC settings */
	RTC_Init_TypeDef rtc = RTC_INIT_DEFAULT;
	rtc.enable = false; /* Don't start counting when initialization is done */

	/* Initialize RTC with pre-defined settings */
	RTC_Init(&rtc);

#if DEBUG_DBPRINT == 1 /* DEBUG_DBPRINT */
#if ULFRCO == 1 /* ULFRCO selected */
	dbinfo("RTC initialized with ULFRCO");
#else /* LFXO selected */
	dbinfo("RTC initialized with LFXO");
#endif /* ULFRCO/LFXO selection */
#endif /* DEBUG_DBPRINT */

	RTC_initialized = true;
}


#if SYSTICKDELAY == 1 /* SysTick delay selected */
/**************************************************************************//**
 * @brief
 *   Interrupt Service Routine for system tick counter.
 *****************************************************************************/
void SysTick_Handler (void)
{
	msTicks++; /* Increment counter necessary by SysTick delay functionality */
}
#endif /* SysTick/RTC selection */


/**************************************************************************//**
 * @brief
 *   Interrupt Service Routine for the RTC.
 *
 * @note
 *   The *weak* definition for this method is located in `system_efm32hg.h`.
 *****************************************************************************/
void RTC_IRQHandler (void)
{
	/* Disable the counter */
	RTC_Enable(false);

	/* Clear the interrupt source */
	RTC_IntClear(RTC_IFC_COMP0);

	/* If the wakeup was caused by "sleeping" (not a delay), act accordingly */
	if (sleeping) RTC_sleep_wakeup = true;
}
