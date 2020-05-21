/*
The OpenTRV project licenses this file to you
under the Apache Licence, Version 2.0 (the "Licence");
you may not use this file except in compliance
with the Licence. You may obtain a copy of the Licence at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the Licence is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied. See the Licence for the
specific language governing permissions and limitations
under the Licence.

Author(s) / Copyright (s): Damon Hart-Davis 2015--2018
                           Deniz Erbilgin 2017
*/

/*
  Routines for sleeping for various times with particular trade-offs.
  Uses a combination of sleep modes, watchdog timer (WDT), and other techniques.

  Hardware specific.
  */

#ifndef OTV0P2BASE_SLEEP_H
#define OTV0P2BASE_SLEEP_H

#include <stdint.h>

#ifdef ARDUINO_ARCH_AVR
#include <util/atomic.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <util/delay_basic.h>
#endif // ARDUINO_ARCH_AVR

#if defined(EFR32FG1P133F256GM48)
extern "C" {
#include "em_device.h"
#include "em_cmu.h"
}
#endif  // defined(EFR32FG1P133GM48)

#ifdef ARDUINO
#include <Arduino.h>
#endif

#include "OTV0P2BASE_PowerManagement.h"
#include "OTV0P2BASE_RTC.h"

// IF DEFINED: Enable emulated subcycle
#define V0P2BASE_SYSTICK_EMULATED_SUBCYCLE

namespace OTV0P2BASE
{
#ifdef EFR32FG1P133F256GM48
constexpr uint32_t F_CPU = 19000000;  // XXX Where should this go?
#endif

// Normal V0p2 (ATMega328P board) power drain ignoring I/O (typ 0.3mA @ 1MHz CPU, 2V)
// ...delay..() routines burn CPU cycles at full power for accurate small microsecond delays.
// idleCPU() routines put the AVR into idle mode with WDT wake-up (typ 40uA @ 1MHz CPU, 2V; 3x--10x savings);
//   all clocks except CPU run so, for example, Serial should still function.
// nap() routines put the AVR into power-save mode with WTD wake-up (typ 0.8uA+ @1.8V);
//   stops I/O clocks and all timers except timer 2 (for the RTC).
// Sleeping in power save mode as per napXXX() waits for timer 2 or external interrupt (typ 0.8uA+ @1.8V).
//
// It is also possible to save some power by slowing the CPU clock,
// though that may disrupt connected timing for I/O device such as the UART,
// and would possibly cause problems for ISRs invoked while the clock is slow.

// Attempt to sleep accurate-ish small number of microseconds even with our slow (1MHz) CPU clock.
// This does not attempt to adjust clock speeds or sleep.
// Interrupts should probably be disabled around the code that uses this to avoid extra unexpected delays.
#ifdef ARDUINO_ARCH_AVR
    #if defined(__AVR_ATmega328P__)
    static void _delay_NOP(void) __attribute__((always_inline)); // Takes 4n CPU cycles to run, 0 runs for 256 cycles.
    static inline void _delay_NOP(void) { __asm__ volatile ( "nop" "\n\t" ); } // Assumed to take 1us with 1MHz CPU clock.

    static void _delay_x4cycles(uint8_t n) __attribute__((always_inline)); // Takes 4n CPU cycles to run, 0 runs for 256 cycles.
    static inline void _delay_x4cycles(uint8_t n) // Takes 4n CPU cycles to run, 0 runs for 256 cycles.
      {
      __asm__ volatile // Similar to _delay_loop_1() from util/delay_basic.h but multiples of 4 cycles are easier.
         (
          "1: dec  %0" "\n\t"
          "   breq 2f" "\n\t"
          "2: brne 1b"
          : "=r" (n)
          : "0" (n)
        );
      }

    // OTV0P2BASE_busy_spin_delay is a guaranteed CPU-busy-spin delay with no dependency on interrupts,
    // microseconds [4,1023] (<4 will work if a constant).
    #if (F_CPU == 1000000)
    // Delay (busy wait) the specified number of microseconds in the range [4,1023] (<4 will work if a constant).
    // Nominally equivalent to delayMicroseconds() except that 1.0.x version of that is broken for slow CPU clocks.
    // Granularity is 1us if parameter is a compile-time constant, else 4us.
    #define OTV0P2BASE_busy_spin_delay(us) do { \
        if(__builtin_constant_p((us)) && ((us) == 0)) { /* Nothing to do. */ } \
        else { \
          if(__builtin_constant_p((us)) && ((us) & 1)) { ::OTV0P2BASE::_delay_NOP(); } \
          if(__builtin_constant_p((us)) && ((us) & 2)) { ::OTV0P2BASE::_delay_NOP(); ::OTV0P2BASE::_delay_NOP(); } \
          if((us) >= 4) { ::OTV0P2BASE::_delay_x4cycles((us) >> 2); } \
          } } while(false)
    #elif (F_CPU == 16000000) // Arduino UNO runs at 16MHz; make special-case efficient code.
    #define OTV0P2BASE_busy_spin_delay(us) do { \
        if(__builtin_constant_p((us)) && ((us) == 0)) { /* Nothing to do. */ } \
        else { \
          for(uint8_t _usBlocks = ((us) >> 6); _usBlocks-- > 0; ) { ::OTV0P2BASE::_delay_x4cycles(0); } \
          ::OTV0P2BASE::_delay_x4cycles(((us) & 63) << 2); \
          } } while(false)
    #endif
    #endif // defined(__AVR_ATmega328P__)

    #if defined(OTV0P2BASE_busy_spin_delay)
    #define OTV0P2BASE_delay_us(us) OTV0P2BASE_busy_spin_delay(us)
    #else // OTV0P2BASE_busy_spin_delay
    // No definition for OTV0P2BASE_busy_spin_delay().
    #define OTV0P2BASE_delay_us(us) delayMicroseconds(us) // Assume that the built-in routine will behave itself for faster CPU clocks.
    #endif // OTV0P2BASE_busy_spin_delay

    // Delay (busy wait) the specified number of milliseconds in the range [0,255].
    // This may be extended by interrupts, etc, so must not be regarded as very precise.
    inline void delay_ms(uint8_t ms) { while(ms-- > 0) { OTV0P2BASE_delay_us(996); /* Allow for some loop overhead. */ } }
#elif defined(__arm__)

    // Cortex M4 instruction set manual states that NOP is not necessarily
    // time consuming and may be removed before the execution stage.
    // Tests suggest that NOP is usually single instruction and where it isn't,
    // other instructions are unpredictable as well.
    // https://www.pabigot.com/arm-cortex-m/the-effect-of-the-arm-cortex-m-nop-instruction/
    static __inline__ void _delay_NOP(void) { __NOP(); }
    // TODO test this is correct.
    static __inline__ void _delay_x4cyclesLong(uint32_t n) // Takes 4n CPU cycles to run, 0 runs for 2^32 cycles.
    {
        __asm__ volatile
        (
            "1: SUBS %0, #1;"   "\n\t"  // 1 cycle
            "   NOP;"           "\n\t"  // 1 cycle
            "   BNE 1b;"        "\n\t"  // 2 cycles
            :"=r"   (n)
            :"0"    (n)
        );
    }
    // For compat with existing AVR version. XXX Portability stuff
    static __inline__ void _delay_x4cycles(uint_fast8_t n) { n = (0U == n) ? 256 : 0; _delay_x4cyclesLong(n); }

    // OTV0P2BASE_busy_spin_delay is a guaranteed CPU-busy-spin delay with no dependency on interrupts,
    // microseconds [4,1023] (<4 will work if a constant).
    // Delay (busy wait) the specified number of microseconds in the range [4,1023] (<4 will work if a constant).
    // Nominally equivalent to delayMicroseconds() except that 1.0.x version of that is broken for slow CPU clocks.
    // Granularity is 1us if parameter is a compile-time constant, else 4us.
    constexpr uint32_t delayLoopTimeInNS = (uint32_t)1e9 / (F_CPU / 4U);
    static __inline__ void OTV0P2BASE_busy_spin_delayLong(uint32_t us)
    {
        do {
            if(__builtin_constant_p((us)) && ((us) == 0)) { /* Nothing to do. */ }
            else {
                // Actually need 9.5 loops per us. Overall error is about -5%.
                constexpr uint32_t delayLoopsPerUS = 1000U / delayLoopTimeInNS;
                _delay_x4cyclesLong(us * delayLoopsPerUS);
            }
        } while(false);
    }
    static __inline__ void OTV0P2BASE_busy_spin_delay(uint_fast8_t us) { OTV0P2BASE_busy_spin_delayLong(us); }
    static __inline__ void OTV0P2BASE_delay_us(uint_fast8_t us) { OTV0P2BASE_busy_spin_delayLong(us); }

    // TODO Replace with low power timer?
    // Delay (busy wait) the specified number of milliseconds in the range [0,255].
    // This may be extended by interrupts, etc, so must not be regarded as very precise.
    inline void delay_ms(uint_fast8_t ms) { while(ms-- > 0) { OTV0P2BASE_busy_spin_delayLong((uint32_t)1e6 / (delayLoopTimeInNS * 9)); /* busy spin takes ~945 us, loop overhead insignificant. */ } }
#else // ARDUINO_ARCH_AVR
    // Stub impl for unit testing.
    // NOTE THAT THIS WILL NOT BEHAVE CORRECTLY UNLESS UNIT TESTING!
    static __inline__ void _delay_x4cycles(uint_fast8_t ) {  }
#endif // ARDUINO_ARCH_AVR


// Sleep with BOD disabled in power-save mode; will wake on any interrupt.
// This particular API is not guaranteed to be maintained:
// please use sleepUntilInt() instead.
void sleepPwrSaveWithBODDisabled();

// Sleep indefinitely in as lower-power mode as possible until a specified watchdog time expires, or another interrupt.
// May be useful to call minimsePowerWithoutSleep() first,
// when not needing any modules left on.
inline void sleepUntilInt() { sleepPwrSaveWithBODDisabled(); }

#ifdef ARDUINO_ARCH_AVR
    // Idle the CPU for specified time but leave everything else running (eg UART), returning on any interrupt or the watchdog timer.
    //   * watchdogSleep is one of the WDTO_XX values from <avr/wdt.h>
    //   * allowPrematureWakeup if true then if woken before watchdog fires return false (default false)
    // Should reduce power consumption vs spinning the CPU more than 3x, though not nearly as much as nap().
    // True iff watchdog timer expired; false if something else woke the CPU.
    // Only use this if not disallowed for board type, eg with ENABLE_USE_OF_AVR_IDLE_MODE.
    // DHD20150920: POSSIBLY NOT RECOMMENDED AS STILL SEEMS TO CAUSE SOME BOARDS TO CRASH.
    #define OTV0P2BASE_IDLE_NOT_RECOMMENDED // IF DEFINED, avoid IDLE mode.
    bool _idleCPU(int_fast8_t watchdogSleep, bool allowPrematureWakeup = false);    // TODO
#endif // ARDUINO_ARCH_AVR

// Sleep briefly in as lower-power mode as possible until the specified (watchdog) time expires.
//   * watchdogSleep is one of the WDTO_XX values from <avr/wdt.h>
// May be useful to call minimsePowerWithoutSleep() first, when not needing any modules left on.
// NOTE: will stop clocks for UART, etc.
void nap(int_fast8_t watchdogSleep);

// Sleep briefly in as lower-power mode as possible until the specified (watchdog) time expires, or another interrupt.
//   * watchdogSleep is one of the WDTO_XX values from <avr/wdt.h>
//   * allowPrematureWakeup if true then if woken before watchdog fires return false
// Returns false if the watchdog timer did not go off, true if it did.
// May be useful to call minimsePowerWithoutSleep() first, when not needing any modules left on.
// NOTE: will stop clocks for UART, etc.
bool nap(int_fast8_t watchdogSleep, bool allowPrematureWakeup);

#ifdef ARDUINO_ARCH_AVR
    // If CPU clock is 1MHz then *assume* that it is the 8MHz internal RC clock prescaled by 8 unless DEFAULT_CPU_PRESCALE is defined.
    #if F_CPU == 1000000L
    static const uint8_t DEFAULT_CPU_PRESCALE = 3;
    #else // F_CPU == 1000000L
    static const uint8_t DEFAULT_CPU_PRESCALE = 1;
    #endif // F_CPU == 1000000L

    static const clock_div_t MAX_CPU_PRESCALE = clock_div_256; // At least for the ATmega328P...
    // Minimum scaled CPU clock speed; expected to be 31250Hz when driven from 8MHz RC clock.
    #if F_CPU > 16000000L
    static const uint32_t MIN_CPU_HZ = ((F_CPU) >> (((int) MAX_CPU_PRESCALE) - (DEFAULT_CPU_PRESCALE)));
    #else // F_CPU > 16000000L
    static const uint16_t MIN_CPU_HZ = ((F_CPU) >> (((int) MAX_CPU_PRESCALE) - (DEFAULT_CPU_PRESCALE)));
    #endif // F_CPU > 16000000L
#endif // ARDUINO_ARCH_AVR

#ifdef ARDUINO_ARCH_AVR
    // Sleep for specified number of _delay_loop2() loops at minimum available CPU speed.
    // Each loop takes 4 cycles at that minimum speed, but entry and exit overheads may take the equivalent of a loop or two.
    // Note: inlining is prevented so as to avoid migrating anything into the section where the CPU is running slowly.
    // Not recommended as-is as may interact badly with interrupts if used naively (eg ISR code runs very slowly).
    // This may only be safe to use with interrupts disabled.
    void _sleepLowPowerLoopsMinCPUSpeed(uint16_t loops) __attribute__ ((noinline));
    // Sleep/spin for approx specified strictly-positive number of milliseconds, in as low-power mode as possible.
    // This may be achieved in part by dynamically slowing the CPU clock if possible.
    // Macro to allow some constant folding at compile time where the sleep-time argument is constant.
    // Should be good for values up to at least 1000, ie 1 second.
    // Assumes MIN_CPU_HZ >> 4000.
    // TODO: break out to non-inlined routine where arg is not constant (__builtin_constant_p).
    // Not recommended as-is as may interact badly with interrupts if used naively (eg ISR code runs very slowly).
    void inline _sleepLowPowerMs(const uint16_t ms) { _sleepLowPowerLoopsMinCPUSpeed((((MIN_CPU_HZ * (ms)) + 2000) / 4000) - ((MIN_CPU_HZ>=12000)?2:((MIN_CPU_HZ>=8000)?1:0))); }
    // Sleep/spin for (typically a little less than) strictly-positive specified number of milliseconds, in as low-power mode as possible.
    // This may be achieved in part by dynamically slowing the CPU clock if possible.
    // Macro to allow some constant folding at compile time where the sleep-time argument is constant.
    // Should be good for values up to at least 1000, ie 1 second.
    // Uses formulation likely to be quicker than _sleepLowPowerMs() for non-constant argument values,
    // and that results in a somewhat shorter sleep than _sleepLowPowerMs(ms).
    // Assumes MIN_CPU_HZ >> 4000.
    // TODO: break out to non-inlined routine where arg is not constant (__builtin_constant_p).
    // Not recommended as-is as may interact badly with interrupts if used naively (eg ISR code runs very slowly).
    void inline _sleepLowPowerLessThanMs(const uint16_t ms) { _sleepLowPowerLoopsMinCPUSpeed(((MIN_CPU_HZ/4000) * (ms)) - ((MIN_CPU_HZ>=12000)?2:((MIN_CPU_HZ>=8000)?1:0))); }
    // Sleep/spin for approx specified strictly-positive number of milliseconds, in as low-power mode as possible.
    // Nap() may be more efficient for intervals of longer than 15ms.
    // Interrupts are blocked for about 1ms at a time.
    // Should be good for the full range of values and should take no time where 0ms is specified.
    void inline sleepLowPowerMs(uint16_t ms) { while(ms-- > 0) { ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { _sleepLowPowerMs(1); } } }
    // Sleep/spin for (typically a little less than) strictly-positive specified number of milliseconds, in as low-power mode as possible.
    // Nap() may be more efficient for intervals of longer than 15ms.
    // Interrupts are blocked for about 1ms at a time.
    // Should be good for the full range of values and should take no time where 0ms is specified.
    static void inline sleepLowPowerLessThanMs(uint16_t ms) { while(ms-- > 0) { ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { _sleepLowPowerLessThanMs(1); } } }
#endif // ARDUINO_ARCH_AVR

#ifdef ARDUINO_ARCH_AVR  // TODO Non emulated version.
    //#if defined(WAKEUP_32768HZ_XTAL) || 1 // FIXME: avoid getSubCycleTime() where slow clock NOT available.
    //// Get fraction of the way through the basic cycle in range [0,255].
    //// This can be used for precision timing during the cycle,
    //// or to avoid overrunning a cycle with tasks of variable timing.
    //// Only valid if running the slow (32768Hz) clock.
    //#define getSubCycleTime() ((uint8_t)TCNT2)
    //// Approximation which is allowed to be zero if true value not available.
    //#define _getSubCycleTime() (getSubCycleTime())
    //#else
    //// Approximation which is allowed to be zero if true value not available.
    //#define _getSubCycleTime() (0)
    inline uint8_t getSubCycleTime() { return (TCNT2); }
    inline uint8_t _getSubCycleTime() { return (TCNT2); }
// #endif
#elif defined(EFR32FG1P133F256GM48) && defined(V0P2BASE_SYSTICK_EMULATED_SUBCYCLE)
    namespace {
        volatile uint_fast8_t subCycleTime = 0U;
        uint32_t calcSysTickTicks(uint32_t period) {
            const auto clockFreq = CMU_ClockFreqGet(cmuClock_CORE);  // TODO reference F_CPU instead?
            // ticks = freq * period in s
            return (clockFreq * period) / 1000;
        }
    }
    /**
     * @brief   Sets up the systick timer to fire every 2/256 seconds.
     * @retval  False on success, else true.
     */
    inline bool setupEmulated2sSubCycle()
    {
        return SysTick_Config(calcSysTickTicks(2000U/256U));
    }

    /**
     * @brief   Increment SubCycle time when SysTick handler called.
     * 
     * Should be placed in SysTick_Handler()
     */
    inline void tickSubCycle(void)
    {
        subCycleTime += 1;
        subCycleTime = (256U == subCycleTime) ? 0 : subCycleTime;
    }

    //// Get fraction of the way through the basic cycle in range [0,255].
    //// This can be used for precision timing during the cycle,
    //// or to avoid overrunning a cycle with tasks of variable timing.
    inline uint_fast8_t getSubCycleTime() { return (subCycleTime); }
    inline uint_fast8_t _getSubCycleTime() { return (subCycleTime); }
#endif  // ARDUINO_ARCH_AVR
#if defined(ARDUINO_ARCH_AVR) || defined(__arm__)
    //// Maximum value for OTV0P2BASE::getSubCycleTime(); full cycle length is this + 1.
    //// So ~4ms per count for a 1s cycle time, ~8ms per count for a 2s cycle time.
    //#define GSCT_MAX 255
    static constexpr uint8_t GSCT_MAX = 255;
    //
    // Basic cycle length in milliseconds; strictly positive. FIXME only 2 tick cycle support
    static constexpr uint16_t BASIC_CYCLE_MS = 2000;
    static constexpr uint8_t SUB_CYCLE_TICKS_PER_S = (uint8_t)((1 + (int)GSCT_MAX)/2); // Careful of overflow.
    //#if defined(V0P2BASE_TWO_S_TICK_RTC_SUPPORT)
    //#define BASIC_CYCLE_MS 2000
    //#define SUB_CYCLE_TICKS_PER_S ((GSCT_MAX+1)/2) // Sub-cycle ticks per second.
    //#else
    //#define BASIC_CYCLE_MS 1000
    //#define SUB_CYCLE_TICKS_PER_S (GSCT_MAX+1) // Sub-cycle ticks per second.
    //#endif
    //// Approx (rounded down) milliseconds per tick of OTV0P2BASE::getSubCycleTime(); strictly positive.
    //#define SUBCYCLE_TICK_MS_RD (BASIC_CYCLE_MS / (GSCT_MAX+1))
    static constexpr uint8_t SUBCYCLE_TICK_MS_RD = (BASIC_CYCLE_MS / (GSCT_MAX+1));
    //// Approx (rounded to nearest) milliseconds per tick of OTV0P2BASE::getSubCycleTime(); strictly positive and no less than SUBCYCLE_TICK_MS_R
    //#define SUBCYCLE_TICK_MS_RN ((BASIC_CYCLE_MS + ((GSCT_MAX+1)/2)) / (GSCT_MAX+1))
    static constexpr uint8_t SUBCYCLE_TICK_MS_RN = ((BASIC_CYCLE_MS + ((GSCT_MAX+1)/2)) / (GSCT_MAX+1));
    //// Returns (rounded-down) approx milliseconds until end of current basic cycle; non-negative.
    //// Upper limit is set by length of basic cycle, thus 1000 or 2000 typically.
    //#define msRemainingThisBasicCycle() (SUBCYCLE_TICK_MS_RD * (GSCT_MAX-OTV0P2BASE::getSubCycleTime()))
    inline uint16_t msRemainingThisBasicCycle() { return (SUBCYCLE_TICK_MS_RD * (GSCT_MAX-getSubCycleTime() ) ); }
#endif // defined(ARDUINO_ARCH_AVR) || defined(EFR32FG1P133GM48)

#ifdef ARDUINO_ARCH_AVR   // TODO
    // Return some approximate/fast measure of CPU cycles elapsed.  Will not count when (eg) CPU/TIMER0 not running.
    // Rather depends on Arduino/wiring setup for micros()/millis().
    #ifndef DONT_USE_TIMER0
    #if defined(TCNT0)
    inline uint8_t getCPUCycleCount() { return((uint8_t)TCNT0); }
    #elif defined(TCNT0L)
    inline uint8_t getCPUCycleCount() { return((uint8_t)TCNT0L); }
    #else
    #error TIMER0 not defined
    #endif
    #else
    #define getCPUCycleCount() ((uint8_t)0) // Fixed result if TIMER0 is not used (for normal Arduino purposes).
    #endif
#endif // ARDUINO_ARCH_AVR


#ifdef ARDUINO_ARCH_AVR   // TODO
    // Sleep in reasonably low-power mode until specified target subcycle time.
    // Returns true if OK, false if specified time already passed or significantly missed (eg by more than one tick).
    // May use a combination of techniques to hit the required time.
    // Requesting a sleep until at or near the end of the cycle risks overrun and may be unwise.
    // Using this to sleep less then 2 ticks may prove unreliable as the RTC rolls on underneath...
    // This is NOT intended to be used to sleep over the end of a minor cycle.
    bool sleepUntilSubCycleTime(uint8_t sleepUntil);
#endif // ARDUINO_ARCH_AVR


// Forced MCU reset/restart as near full cold-reset as possible.
// Turns off interrupts, sets the watchdog, and busy-spins until the watchdog forces a reset.
// The watchdog timeout is long enough that a watchdog-oblivious bootloader
// can successfully drop through to the main code which can then stop a further reset,
// else the main line code may never be reached.
// Inline to give the compiler full visibility for efficient use in ISR code (eg avoid full register save).
//
// Background on implementation and issues, see for example:
//   http://forum.arduino.cc/index.php?topic=12874.0
//   http://playground.arduino.cc/Main/ArduinoReset
// This suggests that a timeout of > 2s may be OK with the optiboot loader:
//   https://tushev.org/articles/arduino/5/arduino-and-watchdog-timer
#ifdef ARDUINO_ARCH_AVR
    #if defined(__AVR_ATmega328P__)
    inline void forceReset()
        {
        cli();
        wdt_enable(WDTO_4S); // Must be long enough for bootloader to pass control to main code.
        for( ; ; ) { }
        }
    #endif // defined(__AVR_ATmega328P__)
#elif defined(EFR32FG1P133F256GM48)  // ARDUINO_ARCH_AVR
    inline void forceReset() { NVIC_SystemReset(); }
#elif defined(__GNUC__)
    inline void forceReset();// __attribute__ ((noreturn));  // FIXME commented to allow compilation of unit tests on clang compiler (DE20170510)
#endif  // ARDUINO_ARCH_AVR
#ifdef ARDUINO_ARCH_AVR
/**
 * @brief   - Sleep in low-power mode (waiting for interrupts) until seconds roll.
 *          - Will call the function passed in repeatedly until it returns
 *          false, then sleep OR exit the loop of the seconds roll.
 * @param   preSleepFn_ptr: Function to call in loop. Defaults to nullptr, in which case
 *          it will go straight to sleep.
 * @param   preventLongSleep:  If false, AVR sleeps until interrupt.
 *          If true, does a series of short naps instead.
 * @retval  current time in seconds, in range [0,59]
 * @note    Should be placed at the top of loop to minimise timing jitter/delay
 *          from Arduino background activity after loop() returns.
 * @note    DHD20130425: waking up from sleep and getting to start processing
 *          below this block may take >10ms.
 *
 */
template<bool (*preSleepFn_ptr)()=nullptr>
uint_fast8_t sleepUntilNewCycle(const uint_fast8_t oldTimeLSD,
                                const bool preventLongSleep = false)
{
    // Ensure that serial I/O is off while sleeping.
    powerDownSerial();
    // Power down most stuff (except radio for hub RX).
    minimisePowerWithoutSleep();
    uint_fast8_t newTLSD;
    while(oldTimeLSD == (newTLSD = getSecondsLT())) {
        if(nullptr != preSleepFn_ptr) {
            if(preSleepFn_ptr()) { continue; }
        }
        if(!preventLongSleep) {
            // Normal long minimal-power sleep until wake-up interrupt.
            // Rely on interrupt to force quick loop round to I/O poll.
            sleepUntilInt();
        } else {
            // If there is no hardware interrupt wakeup support
            // on receipt/RX of a frame by the radio (etc),
            // then this can only sleep for a short time between
            // explicit poll()s.
            //
            // Allow wake on interrupt while nap()ping
            // to minimise loop timing jitter
            // when the slow RTC 'end of sleep' tick arrives.
            OTV0P2BASE::nap(WDTO_15MS, true);
        }
    }
    return (newTLSD);
}

#endif  // ARDUINO_ARCH_AVR

} // OTV0P2BASE


#endif // OTV0P2BASE_SLEEP_H
