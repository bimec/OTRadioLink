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

Author(s) / Copyright (s): Damon Hart-Davis 2015--2016
*/

#include "OTRadValve_BoilerDriver.h"


namespace OTRadValve
    {


//// Set thresholds for per-value and minimum-aggregate percentages to fire the boiler.
//// Coerces values to be valid:
//// minIndividual in range [1,100] and minAggregate in range [minIndividual,100].
//void OnOffBoilerDriverLogic::setThresholds(const uint8_t minIndividual, const uint8_t minAggregate)
//  {
//  minIndividualPC = constrain(minIndividual, 1, 100);
//  minAggregatePC = constrain(minAggregate, minIndividual, 100);
//  }
//
//// Called upon incoming notification of status or call for heat from given (valid) ID.
//// ISR-/thread- safe to allow for interrupt-driven comms, and as quick as possible.
//// Returns false if the signal is rejected, eg from an unauthorised ID.
//// The basic behaviour is that a signal with sufficient percent open
//// is good for 2 minutes (120s, 60 ticks) unless explicitly cancelled earlier,
//// for all valve types including FS20/FHT8V-style.
//// That may be slightly adjusted for IDs that indicate FS20 housecodes, etc.
////   * id  is the two-byte ID or house code; 0xffffu is never valid
////   * percentOpen  percentage open that the remote valve is reporting
//bool OnOffBoilerDriverLogic::receiveSignal(const uint16_t id, const uint8_t percentOpen)
//  {
//  if((badID == id) || (percentOpen > 100)) { return(false); } // Reject bad args.
//
//  bool accepted = false;
//
//  // Under lock to be ISR-safe.
//  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
//    {
//#if defined(BOILER_RESPOND_TO_SPECIFIED_IDS_ONLY)
//    // Reject unrecognised IDs if any in the auth list with false return.
//    if(badID != authedIDs[0])
//      {
//        // TODO
//      }
//#endif
//
//
//// Find current entry in list if present and update,
//// else extend list if possible,
//// or replace 0 entry if available to make space,
//// or replace lower/lowest entry if this passed 'individual' threshold,
//// else reject update.
//
//    // Find entry for current ID if present or create one at end if space,
//    // but note in passing lowest % lower than current signal in case above not possible.
//
//
//    }
//
//  return(accepted);
//  }
//
//#if defined(OnOffBoilerDriverLogic_CLEANUP)
//// Do some incremental clean-up to speed up future operations.
//// Aim to free up at least one status slot if possible.
//void OnOffBoilerDriverLogic::cleanup()
//  {
//  // Swap more-recently-heard-from items towards lower indexes.
//  // Kill off trailing old entries.
//  // Don't necessarily run on every tick:
//  // possibly only run when something actually expires or when out of space.
//  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
//    {
//    if(badID != status[0].id)
//      {
//      // TODO
//      }
//    }
//  }
//#endif
//
//// Fetches statuses of valves recently heard from and returns the count; 0 if none.
//// Optionally filters to return only those still live and apparently calling for heat.
////   * valves  array to copy status to the start of; never null
////   * size  size of valves[] in entries (not bytes), no more entries than that are used,
////     and no more than maxRadiators entries are ever needed
////   * onlyLiveAndCallingForHeat  if true retrieves only current entries
////     'calling for heat' by percentage
//uint8_t OnOffBoilerDriverLogic::valvesStatus(PerIDStatus valves[], const uint8_t size, const bool onlyLiveAndCallingForHeat) const
//  {
//  uint8_t result = 0;
//  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
//    {
//    for(volatile const PerIDStatus *p = status; (p < status+maxRadiators) && (badID != p->id); ++p)
//      {
//      // Stop if retun array full.
//      if(result >= size) { break; }
//      // Skip if filtering and current item not of interest.
//      if(onlyLiveAndCallingForHeat && ((p->ticksUntilOff < 0) || (p->percentOpen < minIndividualPC))) { continue; }
//      // Copy data into result array and increment count.
//      valves[result++] = *(PerIDStatus *)p;
//      }
//    }
//  return(result);
//  }
//
//// Poll every 2 seconds in real/virtual time to update state in particular the callForHeat value.
//// Not to be called from ISRs,
//// in part because this may perform occasional expensive-ish operations
//// such as incremental clean-up.
//// Because this does not assume a tick is in real time
//// this remains entirely unit testable,
//// and no use of wall-clack time is made within this or sibling class methods.
//void OnOffBoilerDriverLogic::tick2s()
//  {
//  bool doCleanup = false;
//
//  // If individual and aggregate limits are passed by set of IDs (and minimumTicksUntilOn is zero)
//  // then nominally turn boiler on else nominally turn it off.
//  // Such change of state may be prevented/delayed by duty-cycle limits.
//  //
//  // Adjust all expiry timers too.
//  //
//  // Be careful not to lock out interrupts (ie hold the lock) too long.
//
//  // Set true if at least one valve has met/passed the individual % threshold to be considered calling for heat.
//  bool atLeastOneValceCallingForHeat = false;
//  // Partial cumulative percent open (stops accumulating once threshold has been passed).
//  uint8_t partialCumulativePC = 0;
//  ATOMIC_BLOCK (ATOMIC_RESTORESTATE)
//    {
//    for(volatile PerIDStatus *p = status; (p < status+maxRadiators) && (badID != p->id); ++p)
//      {
//      // Decrement time-until-expiry until lower limit is reached, at which point call for a cleanup!
//      volatile int8_t &t = p->ticksUntilOff;
//      if(t > -128) { --t; } else { doCleanup = true; continue; }
//      // Ignore stale entries for boiler-state calculation.
//      if(t < 0) { continue; }
//      // Check if at least one valve is really open.
//      if(!atLeastOneValceCallingForHeat && (p->percentOpen >= minIndividualPC)) { atLeastOneValceCallingForHeat = true; }
//      // Check if aggregate limits are being reached.
//      if(partialCumulativePC < minAggregatePC) { partialCumulativePC += p->percentOpen; }
//      }
//    }
//  // Compute desired boiler state unconstrained by duty-cycle limits.
//  // Boiler should be on if both individual and aggregate limits are met.
//  const bool desiredBoilerState = atLeastOneValceCallingForHeat && (partialCumulativePC >= minAggregatePC);
//
//#if defined(OnOffBoilerDriverLogic_CLEANUP)
//  if(doCleanup) { cleanup(); }
//#endif
//
//  // Note passage of a tick in current state.
//  if(ticksInCurrentState < 0xff) { ++ticksInCurrentState; }
//
//  // If already in the correct state then nothing to do.
//  if(desiredBoilerState == callForHeat) { return; }
//
//  // If not enough ticks have passed to change state then don't.
//  if(ticksInCurrentState < minTicksInEitherState) { return; }
//
//  // Change boiler state and reset counter.
//  callForHeat = desiredBoilerState;
//  ticksInCurrentState = 0;
//  }
//
//uint8_t BoilerDriver::read()
//   {
//   logic.tick2s();
//   value = (logic.isCallingForHeat()) ? 0 : 100;
//   return(value);
//   }


    }
