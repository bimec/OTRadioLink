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
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());
    o1.markAsOccupied();
    ASSERT_TRUE(o1.isLikelyRecentlyOccupied());
    ASSERT_TRUE(o1.isLikelyOccupied());
    ASSERT_FALSE(o1.isLikelyUnoccupied());
    // Run for half the nominal time and ensure still marked as occupied.
    for(int i = 0; i < o1.OCCUPATION_TIMEOUT_M/2; ++i) { o1.read(); ASSERT_TRUE(o1.isLikelyOccupied()); }
    // Run again for about half the nominal time and ensure now not occupied.
    for(int i = 0; i < o1.OCCUPATION_TIMEOUT_M/2 + 1; ++i) { o1.read(); }
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());

    // Run down from max and show that various (short-term) views are consistent.
    // Full consistency may only be enforced directly after read().
    o1.markAsOccupied();
    for(int i = 0; i < o1.OCCUPATION_TIMEOUT_M + 1; ++i)
        {
        const uint8_t v = o1.read();
        ASSERT_EQ(v, o1.get());
        EXPECT_EQ((0 != v), o1.isLikelyOccupied());
        if(o1.isLikelyRecentlyOccupied())
            {
            EXPECT_TRUE(o1.isLikelyOccupied());
            EXPECT_FALSE(o1.isLikelyUnoccupied());
            EXPECT_LT(0, o1.get());
            EXPECT_EQ(3, o1.twoBitOccupancyValue());
            }
        if(o1.isLikelyOccupied())
            {
            EXPECT_FALSE(o1.isLikelyUnoccupied());
            EXPECT_LT(0, o1.get());
            EXPECT_LE(2, o1.twoBitOccupancyValue());
            }
        if(o1.isLikelyUnoccupied())
            {
            EXPECT_EQ(0, o1.get());
            EXPECT_EQ(1, o1.twoBitOccupancyValue());
            }
        }
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());

    // Put in holiday mode; show marked very vacant.
    o1.setHolidayMode();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());
    ASSERT_LT(0, o1.getVacancyH());
    ASSERT_TRUE(o1.longVacant());
    ASSERT_TRUE(o1.longLongVacant());

    // Put in holiday mode; show that markAsOccupied()
    // brings status back to occupied.
    o1.setHolidayMode();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());
    o1.markAsOccupied();
    ASSERT_TRUE(o1.isLikelyRecentlyOccupied());
    ASSERT_TRUE(o1.isLikelyOccupied());
    ASSERT_FALSE(o1.isLikelyUnoccupied());
    ASSERT_TRUE(o1.reportedNewOccupancyRecently());
    EXPECT_EQ(3, o1.twoBitOccupancyValue());

    // Put in holiday mode; show that markAsPossiblyOccupied()
    // brings status back to occupied.
    o1.setHolidayMode();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());
    o1.markAsPossiblyOccupied();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_TRUE(o1.isLikelyOccupied());
    ASSERT_FALSE(o1.isLikelyUnoccupied());
    ASSERT_TRUE(o1.reportedNewOccupancyRecently());
    EXPECT_EQ(2, o1.twoBitOccupancyValue());

    // Put in holiday mode; show that markAsJustPossiblyOccupied()
    // DOES NOT move status to occupied.
    o1.setHolidayMode();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());
    o1.markAsJustPossiblyOccupied();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());
    ASSERT_FALSE(o1.reportedNewOccupancyRecently());
    EXPECT_EQ(1, o1.twoBitOccupancyValue());

    // Show that markAsJustPossiblyOccupied()
    // does indicate some occupancy when system not very torpid.
    o1.reset();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_FALSE(o1.isLikelyOccupied());
    ASSERT_TRUE(o1.isLikelyUnoccupied());
    o1.markAsJustPossiblyOccupied();
    ASSERT_FALSE(o1.isLikelyRecentlyOccupied());
    ASSERT_TRUE(o1.isLikelyOccupied());
    ASSERT_FALSE(o1.isLikelyUnoccupied());
    ASSERT_FALSE(o1.reportedNewOccupancyRecently());
    EXPECT_EQ(2, o1.twoBitOccupancyValue());
}
