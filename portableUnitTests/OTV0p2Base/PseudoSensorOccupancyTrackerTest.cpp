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

Author(s) / Copyright (s): Damon Hart-Davis 2016
*/

/*
 * Driver for PseudoSensorOccupancyTracker tests.
 */

#include <stdint.h>
#include <gtest/gtest.h>
#include <OTV0p2Base.h>
#include "OTV0P2BASE_SensorAmbientLightOccupancy.h"


// Basic operation (duration of occupancy from trigger), etc.
TEST(PseudoSensorOccupancyTracker,basics)
{
    // Set up default occupancy tracker.
    OTV0P2BASE::PseudoSensorOccupancyTracker o1;
    ASSERT_FALSE(o1.isLikelyOccupied());
    o1.markAsOccupied();
    ASSERT_TRUE(o1.isLikelyOccupied());
    // Run for half the nominal time and ensure still marked as occupied.
    for(int i = 0; i < o1.OCCUPATION_TIMEOUT_M/2; ++i) { o1.read(); ASSERT_TRUE(o1.isLikelyOccupied()); }
    // Run again for about half the nominal time and ensure now not occupied.
    for(int i = 0; i < o1.OCCUPATION_TIMEOUT_M/2 + 1; ++i) { o1.read(); }
    ASSERT_FALSE(o1.isLikelyOccupied());
}
