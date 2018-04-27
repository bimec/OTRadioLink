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

Author(s) / Copyright (s): Damon Hart-Davis 2018
*/

/*
 * Sample data set.  ONLY INCLUDE WHERE SPECIFICALLY NEEDED!
 */


#ifndef PUT_OTRADIOLINK_AMBIENTLIGHTOCCUPANCYDETECTIONTEST_sample1gBriefLightOn_H
#define PUT_OTRADIOLINK_AMBIENTLIGHTOCCUPANCYDETECTIONTEST_sample1gBriefLightOn_H

#include "AmbientLightOccupancyDetectionTest.h"


namespace OTV0P2BASE {
namespace PortableUnitTest {
namespace DATA {


// Sample with brief light on in middle of night that should not trigger heating.
// Late December in London.
static const ALDataSample sample1gBriefLightOn[] =
    {
{20,0,3,2},
{20,0,7,2},
{20,0,11,2},
{20,0,15,2},
{20,0,19,2},
{20,0,23,2},
{20,0,27,2},
{20,0,31,2},
{20,0,35,2},
{20,0,39,2},
{20,0,43,2},
{20,0,47,2},
{20,0,51,2},
{20,0,55,2},
{20,0,59,2},
{20,1,3,2},
{20,1,7,2},
{20,1,11,2},
{20,1,15,2},
{20,1,19,2},
{20,1,23,2},
{20,1,27,2},
{20,1,31,2},
{20,1,35,2},
{20,1,39,2},
{20,1,43,2},
{20,1,47,2},
{20,1,51,2},
{20,1,55,2},
{20,1,59,2},
{20,2,3,2},
{20,2,7,2},
{20,2,11,2},
{20,2,15,2},
{20,2,19,2},
{20,2,23,2},
{20,2,27,2},
{20,2,31,2},
{20,2,35,2},
{20,2,39,2},
{20,2,43,2},
{20,2,47,2},
{20,2,51,2},
{20,2,55,2},
{20,3,0,2},
{20,3,3,2},
{20,3,7,2},
{20,3,11,2},
{20,3,15,2},
{20,3,19,2},
{20,3,23,2},
{20,3,27,2},
{20,3,31,2},
{20,3,35,2},
{20,3,39,2},
{20,3,43,2},
{20,3,47,2},
{20,3,51,2},
{20,3,55,2},
{20,3,59,2},
{20,4,3,2},
{20,4,7,2},
{20,4,11,2},
{20,4,15,2},
{20,4,19,2},
{20,4,27,2},
{20,4,31,2},
{20,4,35,2},
{20,4,39,2},
{20,4,43,2},
{20,4,47,2},
{20,4,51,2},
{20,4,55,2},
{20,4,59,2},
{20,5,3,2},
{20,5,7,2},
{20,5,11,2},
{20,5,15,2},
{20,5,19,2},
{20,5,23,2},
{20,5,27,2},
{20,5,31,2},
{20,5,35,2},
{20,5,39,2},
{20,5,43,2},
{20,5,47,2},
{20,5,51,2},
{20,5,55,2},
{20,5,59,2},
{20,6,3,2},
{20,6,7,2},
{20,6,11,2},
{20,6,15,2},
{20,6,19,2},
{20,6,23,2},
{20,6,27,2},
{20,6,31,2},
{20,6,35,2},
{20,6,40,2},
{20,6,44,2},
{20,6,48,2},
{20,6,52,2},
{20,6,56,2},
{20,7,0,2},
{20,7,4,2},
{20,7,8,2},
{20,7,12,2},
{20,7,17,2, occType::OCC_NONE, true, false, ALDataSample::SB_MINMAX}, // Possible minimal setback, anticipating occupancy.
{20,7,21,139},
{20,7,25,139},
{20,7,29,112},
{20,7,32,145, ALDataSample::NO_OCC_EXPECTATION, false, true, ALDataSample::SB_NONE}, // Should have detected occupancy by no, so no setback.
{20,7,37,2},
{20,7,38,2},
{20,7,42,2},
{20,7,46,3},
{20,7,50,4},
{20,7,54,5},
{20,7,58,46},
{20,8,0,52},
{20,8,2,48},
{20,8,7,49},
{20,8,8,11},
{20,8,11,12},
{20,8,15,15},
{20,8,16,16},
{20,8,18,17},
{20,8,20,18},
{20,8,22,19},
{20,8,24,20},
{20,8,25,21},
{20,8,26,22},
{20,8,29,25},
{20,8,30,26},
{20,8,31,27},
{20,8,34,30},
{20,8,38,35},
{20,8,42,38},
{20,8,43,40},
{20,8,46,42},
{20,8,48,43},
{20,8,50,45},
{20,8,51,46},
{20,8,55,48},
{20,8,59,51},
{20,9,0,52},
{20,9,1,53},
{20,9,4,57},
{20,9,5,58},
{20,9,9,59},
{20,9,10,59},
{20,9,14,60},
{20,9,16,61},
{20,9,19,62},
{20,9,23,64},
{20,9,24,65},
{20,9,27,69},
{20,9,28,71},
{20,9,29,73},
{20,9,32,80},
{20,9,33,81},
{20,9,34,82},
{20,9,37,86},
{20,9,38,86},
{20,9,39,85},
{20,9,41,83},
{20,9,43,82},
{20,9,46,82},
{20,9,47,83},
{20,9,51,85},
{20,9,52,86},
{20,9,56,88},
{20,9,57,89},
{20,9,59,91},
{20,10,1,92},
{20,10,4,93},
{20,10,5,91},
{20,10,6,90},
{20,10,7,93},
{20,10,9,95},
{20,10,10,98},
{20,10,13,103},
{20,10,15,100},
{20,10,18,102},
{20,10,19,103},
{20,10,22,102},
{20,10,23,103},
{20,10,25,104},
{20,10,27,105},
{20,10,30,106},
{20,10,32,108},
{20,10,34,110},
{20,10,35,112},
{20,10,38,114},
{20,10,39,116},
{20,10,41,119},
{20,10,43,118},
{20,10,46,119},
{20,10,47,121},
{20,10,49,140},
{20,10,51,148},
{20,10,54,139},
{20,10,56,133},
{20,10,57,129},
{20,10,59,128},
{20,11,1,127},
{20,11,3,129},
{20,11,6,133},
{20,11,7,135},
{20,11,9,141},
{20,11,11,133},
{20,11,15,138},
{20,11,18,142},
{20,11,19,140},
{20,11,21,133},
{20,11,23,138},
{20,11,25,144},
{20,11,27,127},
{20,11,29,124},
{20,11,31,110},
{20,11,34,92},
{20,11,38,73},
{20,11,39,71},
{20,11,41,65},
{20,11,43,63},
{20,11,45,57},
{20,11,47,53},
{20,11,50,49},
{20,11,51,47},
{20,11,53,44},
{20,11,55,44},
{20,11,58,48},
{20,11,59,49},
{20,12,1,56},
{20,12,3,63},
{20,12,5,113},
{20,12,7,165},
{20,12,10,176},
{20,12,11,172},
{20,12,13,140},
{20,12,15,131},
{20,12,17,125},
{20,12,19,115},
{20,12,21,119},
{20,12,23,120},
{20,12,25,135},
{20,12,27,116},
{20,12,29,132},
{20,12,31,142},
{20,12,34,113},
{20,12,35,138},
{20,12,36,140},
{20,12,37,146},
{20,12,39,118},
{20,12,40,136},
{20,12,41,142},
{20,12,42,133},
{20,12,43,147},
{20,12,45,163},
{20,12,48,168},
{20,12,49,170},
{20,12,52,170},
{20,12,55,164},
{20,12,57,156},
{20,12,59,126},
{20,13,1,131},
{20,13,3,172},
{20,13,5,171},
{20,13,8,175},
{20,13,9,174},
{20,13,10,172},
{20,13,11,170},
{20,13,13,170},
{20,13,14,147},
{20,13,15,113},
{20,13,16,109},
{20,13,17,104},
{20,13,19,120},
{20,13,21,105},
{20,13,23,86},
{20,13,25,72},
{20,13,27,70},
{20,13,29,63},
{20,13,31,67},
{20,13,34,79},
{20,13,36,77},
{20,13,37,76},
{20,13,39,77},
{20,13,41,73},
{20,13,43,76},
{20,13,45,77},
{20,13,47,73},
{20,13,49,74},
{20,13,51,67},
{20,13,53,68},
{20,13,55,71},
{20,13,57,76},
{20,13,59,81},
{20,14,2,69},
{20,14,3,65},
{20,14,5,58},
{20,14,7,50},
{20,14,9,47},
{20,14,11,50},
{20,14,13,113},
{20,14,15,179},
{20,14,17,180},
{20,14,19,180},
{20,14,21,178},
{20,14,23,134},
{20,14,25,112},
{20,14,27,93},
{20,14,30,79},
{20,14,31,76},
{20,14,32,74},
{20,14,35,67},
{20,14,36,67},
{20,14,37,68},
{20,14,38,70},
{20,14,39,73},
{20,14,41,78},
{20,14,43,75},
{20,14,45,73},
{20,14,47,76},
{20,14,49,74},
{20,14,51,66},
{20,14,53,57},
{20,14,55,51},
{20,14,57,46},
{20,14,59,42},
{20,15,1,38},
{20,15,4,33},
{20,15,5,32},
{20,15,7,30},
{20,15,9,29},
{20,15,11,28},
{20,15,13,28},
{20,15,16,28},
{20,15,17,30},
{20,15,19,34},
{20,15,21,40},
{20,15,23,42},
{20,15,25,42},
{20,15,27,41},
{20,15,29,34},
{20,15,31,27},
{20,15,33,24},
{20,15,35,22},
{20,15,37,20},
{20,15,40,16},
{20,15,42,15},
{20,15,44,13},
{20,15,45,12},
{20,15,47,10},
{20,15,49,9},
{20,15,52,7},
{20,15,55,6},
{20,15,57,6},
{20,16,1,5},
{20,16,2,4},
{20,16,6,4},
{20,16,7,3},
{20,16,10,3},
{20,16,14,3},
{20,16,18,3},
{20,16,22,3},
{20,16,26,3},
{20,16,29,2},
{20,16,31,2},
{20,16,34,2},
{20,16,38,2},
{20,16,42,2},
{20,16,46,2},
{20,16,50,2},
{20,16,55,2},
{20,16,59,2},
{20,17,3,2},
{20,17,7,2},
{20,17,11,2},
{20,17,15,2},
{20,17,19,2},
{20,17,23,2},
{20,17,27,2},
{20,17,31,2},
{20,17,35,2},
{20,17,39,2},
{20,17,43,2},
{20,17,47,2},
{20,17,51,2},
{20,17,54,2},
{20,17,59,2},
{20,18,3,2},
{20,18,7,2},
{20,18,10,2},
{20,18,15,2},
{20,18,19,2},
{20,18,23,2},
{20,18,26,2},
{20,18,30,2},
{20,18,34,2},
{20,18,38,2},
{20,18,42,2},
{20,18,47,2},
{20,18,50,2},
{20,18,55,2},
{20,18,58,2},
{20,19,2,2},
{20,19,7,2},
{20,19,11,2},
{20,19,16,3},
{20,19,17,2},
{20,19,20,2},
{20,19,25,2},
{20,19,29,2},
{20,19,33,2},
{20,19,37,2},
{20,19,41,2},
{20,19,45,2},
{20,19,49,2},
{20,19,53,2},
{20,19,57,2},
{20,20,1,2},
{20,20,5,2},
{20,20,9,2},
{20,20,13,2},
{20,20,16,2},
{20,20,20,2},
{20,20,26,2},
{20,20,27,3},
{20,20,30,3, occType::OCC_NONE, true, false, ALDataSample::SB_MINMAX}, // Setback possibly reducing in anticipation of occupancy.
{20,20,34,3},
{20,20,38,3},
{20,20,40,145, occType::OCC_PROBABLE, false, true, ALDataSample::SB_NONE}, // Occupancy.
{20,20,42,151},
{20,20,43,103},
{20,20,44,134},
{20,20,47,127},
{20,20,48,110},
{20,20,49,98},
{20,20,52,141},
{20,20,53,145},
{20,20,57,113},
{20,20,58,131},
{20,21,2,150},
{20,21,4,136},
{20,21,7,144},
{20,21,8,161},
{20,21,12,148},
{20,21,16,119},
{20,21,20,2},
{20,21,24,2},
{20,21,28,2},
{20,21,32,2},
{20,21,36,2},
{20,21,40,2},
{20,21,44,2},
{20,21,48,2},
{20,21,52,2},
{20,21,56,2},
{20,22,0,2},
{20,22,4,2},
{20,22,8,2},
{20,22,12,2},
{20,22,17,2},
{20,22,21,2},
{20,22,25,2},
{20,22,29,2},
{20,22,33,2},
{20,22,41,2},
{20,22,45,2},
{20,22,49,2},
{20,22,53,2},
{20,22,57,2},
{20,23,1,2},
{20,23,5,2},
{20,23,9,2},
{20,23,13,2},
{20,23,17,2},
{20,23,21,2},
{20,23,25,2},
{20,23,29,2},
{20,23,34,2},
{20,23,37,2},
{20,23,41,2},
{20,23,45,2},
{20,23,49,2},
{20,23,53,2},
{20,23,57,2},
{21,0,1,2},
{21,0,5,2},
{21,0,9,2},
{21,0,13,2},
{21,0,17,2},
{21,0,21,2},
{21,0,25,2},
{21,0,30,2},
{21,0,33,2},
{21,0,37,2},
{21,0,41,2},
{21,0,45,2},
{21,0,49,2},
{21,0,53,2},
{21,0,57,2},
{21,1,1,2},
{21,1,5,2},
{21,1,9,2},
{21,1,13,2},
{21,1,17,2},
{21,1,21,2},
{21,1,25,2},
{21,1,29,2},
{21,1,33,2},
{21,1,37,2},
{21,1,41,2},
{21,1,45,2},
{21,1,49,2},
{21,1,53,2},
{21,1,57,2},
{21,2,1,2},
{21,2,5,2},
{21,2,9,2},
{21,2,13,2, occType::OCC_NONE, true, false, ALDataSample::SB_MAX}, // Dark for a while; full setback.
{21,2,14,99, ALDataSample::NO_OCC_EXPECTATION, false, false, ALDataSample::SB_ECOMAX}, // Brief light on; good setback maintained.
{21,2,15,2, occType::OCC_NONE, true, false, ALDataSample::SB_ECOMAX}, // Light off almost immediately; good setback still in place.
{21,2,17,2},
{21,2,20,2},
{21,2,24,2},
{21,2,29,2},
{21,2,34,2},
{21,2,38,2},
{21,2,42,2},
{21,2,46,2},
{21,2,50,2},
{21,2,54,2},
{21,2,58,2},
{21,3,2,2},
{21,3,6,2},
{21,3,10,2},
{21,3,14,2},
{21,3,18,2},
{21,3,22,2},
{21,3,26,2},
{21,3,31,2},
{21,3,36,2},
{21,3,39,2},
{21,3,44,2},
{21,3,47,2},
{21,3,52,2},
{21,3,56,2},
{21,4,0,2},
{21,4,4,2},
{21,4,8,2},
{21,4,12,2},
{21,4,16,2},
{21,4,20,2},
{21,4,24,2},
{21,4,28,2},
{21,4,32,2},
{21,4,36,2},
{21,4,40,2},
{21,4,44,2},
{21,4,48,2},
{21,4,52,2},
{21,4,56,2},
{21,5,0,2, occType::OCC_NONE, true, false, ALDataSample::SB_MAX}, // Should be back to full setback.
{21,5,4,2},
{21,5,8,2},
{21,5,12,2},
{21,5,16,2},
{21,5,20,2},
{21,5,24,2},
{21,5,28,2},
{21,5,32,2},
{21,5,36,2},
{21,5,41,2},
{21,5,45,2},
{21,5,49,2},
{21,5,53,2},
{21,5,57,2},
{21,6,1,2},
{21,6,5,2},
{21,6,13,2},
{21,6,17,2},
{21,6,21,2},
{21,6,26,2},
{21,6,29,2},
{21,6,33,2},
{21,6,37,2},
{21,6,41,2},
{21,6,45,2},
{21,6,49,2},
{21,6,53,2},
{21,6,57,2},
{21,7,1,2},
{21,7,5,2},
{21,7,9,2},
{21,7,13,2},
{21,7,17,2, occType::OCC_NONE, true, false, ALDataSample::SB_MINMAX}, // May have minimal setback, anticipating occupancy.
{21,7,20,128},
{21,7,24,143},
{21,7,28,138},
{21,7,33,2},
{21,7,34,2},
{21,7,38,2},
{21,7,42,2},
{21,7,46,2},
{21,7,50,3},
{21,7,54,42},
{21,7,56,44},
{21,7,58,47},
{21,7,59,48},
{21,8,3,49},
{21,8,4,50},
{21,8,5,51},
{21,8,8,6},
{21,8,10,7},
{21,8,13,8},
{21,8,14,9},
{21,8,17,10},
{21,8,21,12},
{21,8,22,13},
{21,8,23,14},
{21,8,26,12},
{21,8,27,13},
{21,8,31,16},
{21,8,35,24},
{21,8,37,27},
{21,8,39,26},
{21,8,40,27},
{21,8,41,26},
{21,8,44,18},
{21,8,45,17},
{21,8,49,31},
{21,8,50,36},
{21,8,52,33},
{21,8,54,36},
{21,8,55,30},
{21,8,59,33},
{21,9,0,30},
{21,9,2,28},
{21,9,4,27},
{21,9,5,28},
{21,9,6,29},
{21,9,7,30},
{21,9,9,28},
{21,9,10,27},
{21,9,11,26},
{21,9,14,34},
{21,9,18,42},
{21,9,19,40},
{21,9,20,41},
{21,9,23,42},
{21,9,24,44},
{21,9,28,36},
{21,9,29,33},
{21,9,32,31},
{21,9,33,32},
{21,9,36,40},
{21,9,37,42},
{21,9,40,47},
{21,9,41,42},
{21,9,42,38},
{21,9,43,34},
{21,9,45,26},
{21,9,46,25},
{21,9,47,23},
{21,9,50,23},
{21,9,51,24},
{21,9,53,26},
{21,9,55,27},
{21,9,58,27},
{21,9,59,30},
{21,10,2,40},
{21,10,3,50},
{21,10,5,52},
{21,10,7,50},
{21,10,10,63},
{21,10,11,65},
{21,10,14,67},
{21,10,15,62},
{21,10,17,71},
{21,10,19,66},
{21,10,22,59},
{21,10,23,50},
{21,10,26,41},
{ }
    };


} } }


#endif
