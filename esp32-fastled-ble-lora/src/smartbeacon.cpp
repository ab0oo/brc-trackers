//
//
// Input: Course in degrees
//        Speed in knots
//
// Output: May force beacons by setting posit_next_time to various
//         values.
//
// Modify: sb_POSIT_rate
//         sb_current_heading
//         sb_last_heading
//         posit_next_time
//
//
// With the defaults compiled into the code, here are the
// turn_thresholds for a few speeds:
//
// Example: sb_turn_min = 20
//          sb_turn_slope = 25
//          sb_high_speed_limit = 60
//
//      > 60mph  20 degrees
//        50mph  25 degrees
//        40mph  26 degrees
//        30mph  28 degrees
//        20mph  33 degrees
//        10mph  45 degrees
//        3mph 103 degrees (we limit it to 80 now)
//        2mph 145 degrees (we limit it to 80 now)
//
// I added a max threshold of 80 degrees into the code.  145 degrees
// is unreasonable to expect except for perhaps switchback or 'U'
// turns.
//
// It'd probably be better to do a linear interpolation of
// turn_threshold based on min/max speed and min/max turns.  That's
// not how the SmartBeaconing(tm) algorithm coders implemented it in
// the HamHud though.
//
#include <Arduino.h>
#include "smartbeacon.h"

int sb_last_heading = -1;

bool compute_smart_beacon(int course, int speed, int secs_since_beacon) {
    int turn_threshold;
    int heading_change_since_beacon;
    int curr_sec = millis() / 1000;
    int sb_POSIT_rate = 0;
    int sb_current_heading = -1;
    int posit_next_time = 0;

    // Check for the low speed threshold, set to slow posit rate if
    // we're going slow.
    if (speed <= SB_LOW_SPEED_LIMIT) {
        // fprintf(stderr,"Slow speed\n");

        // EXPERIMENTAL!!!
        ////////////////////////////////////////////////////////////////////
        // Check to see if we're just crossing the threshold, if so,
        // beacon.  This keeps dead-reckoning working properly on
        // other people's displays.  Be careful for speeds near this
        // threshold though.  We really need a slow-speed rate and a
        // stop rate, with some distance between them, in order to
        // have some hysteresis for these posits.
        //        if (sb_POSIT_rate != (sb_posit_slow * 60) ) { // Previous rate was _not_ the slow rate
        //            beacon_now++; // Force a posit right away
        //            //fprintf(stderr,"Stopping, POSIT!\n");
        //        }
        ////////////////////////////////////////////////////////////////////

        // Set to slow posit rate
        sb_POSIT_rate = SB_POSIT_SLOW * 60;  // Convert to seconds
    } else {                                 // We're moving faster than the low speed limit
        // EXPERIMENTAL!!!
        ////////////////////////////////////////////////////////////////////
        // Check to see if we're just starting to move.  Again, we
        // probably need yet-another-speed-limit here to provide
        // some hysteresis.
        //        if ( (secs_since_beacon > sb_turn_time)    // Haven't beaconed for a bit
        //                && (sb_POSIT_rate == (sb_posit_slow * 60) ) ) { // Last rate was the slow rate
        //            beacon_now++; // Force a posit right away
        //            //fprintf(stderr,"Starting to move, POSIT!\n");
        //        }
        ////////////////////////////////////////////////////////////////////

        // Start with turn_min degrees as the threshold
        turn_threshold = SB_TURN_MIN;

        // Adjust rate according to speed
        if (speed > SB_HIGH_SPEED_LIMIT)  // We're above the high limit
        {
            sb_POSIT_rate = SB_POSIT_FAST;
            // fprintf(stderr,"Setting fast rate\n");
        } else  // We're between the high/low limits.  Set a between rate
        {
            sb_POSIT_rate = (SB_POSIT_FAST * SB_HIGH_SPEED_LIMIT) / speed;
            // fprintf(stderr,"Setting medium rate\n");

            // Adjust turn threshold according to speed
            turn_threshold += (int)((SB_TURN_SLOPE * 10) / speed);
        }

        // Force a maximum turn threshold of 80 degrees (still too
        // high?)
        if (turn_threshold > 80) {
            turn_threshold = 80;
        }

        // Check to see if we've written anything into
        // sb_last_heading variable yet.  If not, write the current
        // course into it.
        if (sb_last_heading == -1) {
            sb_last_heading = course;
        }

        // Corner-pegging.  Note that we don't corner-peg if we're
        // below the low-speed threshold.
        heading_change_since_beacon = abs(course - sb_last_heading);
        if (heading_change_since_beacon > 180) {
            heading_change_since_beacon = 360 - heading_change_since_beacon;
        }

        // fprintf(stderr,"course change:%d\n",heading_change_since_beacon);

        if ((heading_change_since_beacon > turn_threshold) && (secs_since_beacon > SB_TURN_TIME)) {
            return true;  // Force a posit right away

            // fprintf(stderr,"Corner, POSIT!\tOld:%d\tNew:%d\tDifference:%d\tSpeed: %d\tTurn Threshold:%d\n",
            //     sb_last_heading,
            //     course,
            //     heading_change_since_beacon,
            //     speed,
            //     turn_threshold);
        }

        // EXPERIMENTAL
        ////////////////////////////////////////////////////////////////////
        // If we haven't beaconed for a bit (3 * sb_turn_time?), and
        // just completed a turn, check to see if our heading has
        // stabilized yet.  If so, beacon the latest heading.  We'll
        // have to save another variable which says whether the last
        // beacon was caused by corner-pegging.  The net effect is
        // that we'll get an extra posit coming out of a turn that
        // specifies our correct course and probably a more accurate
        // speed until the next posit.  This should make
        // dead-reckoning work even better.
        if (0) {
        }
        ////////////////////////////////////////////////////////////////////
    }

    // Check to see whether we've sped up sufficiently for the
    // posit_next_time variable to be too far out.  If so, shorten
    // that interval to match the current speed.
    if ((posit_next_time - curr_sec) > sb_POSIT_rate) {
        posit_next_time = curr_sec + sb_POSIT_rate;
    }

    // Should we also check for a rate too fast for the current
    // speed?  Probably not.  It'll get modified at the next beacon
    // time, which will happen quickly.

    // Save course for use later.  It gets put into sb_last_heading
    // in UpdateTime() if a beacon occurs.  We then use it above to
    // determine the course deviation since the last time we
    // beaconed.
    sb_current_heading = course;
    return false;
}
