// Code to compute SmartBeaconing(tm) rates.
//
// SmartBeaconing(tm) was invented by Steve Bragg (KA9MVA) and Tony Arnerich
// (KD7TA).  Its main goal is to change the beacon rate based on speed
// and cornering.  It does speed-variant corner pegging and
// speed-variant posit rate.

// Some tweaks have been added to the generic SmartBeaconing(tm) algorithm,
// but are current labeled as experimental and commented out:  1) We do
// a posit as soon as we first cross below the sb_low_speed_limit, and
// 2) We do a posit as soon as we cross above the sb_low_speed_limit if
// we haven't done a posit for sb_turn_time seconds.  These tweaks are
// intended to help show that the mobile station has stopped (so that
// dead-reckoning doesn't keep it moving across the map on other
// people's displays) and to more quickly show that the station is
// moving again (for the case where they're in stop-and-go traffic
// perhaps).
//
// It's possible that these new tweaks won't work well for the case
// where a station is traveling near the speed of sb_low_speed_limit.
// In this case they'll generate a posit each time they go below it and
// every time they go above it if they haven't done a posit in
// sb_turn_time seconds.  This could result in a lot of posits very
// quickly.  We may need to add yet another limit just above the
// sb_low_speed_limit for hysteresis, and not posit until we cross above
// that new limit.
//
// Several special SmartBeaconing(tm) parameters come into play here:
//
#define SB_TURN_MIN 20
// sb_turn_min          Minimum degrees at which corner pegging will
//                      occur.  The next parameter affects this for
//                      lower speeds.
//
#define SB_TURN_SLOPE 25
// sb_turn_slope        Fudget factor for making turns less sensitive at
//                      lower speeds.  No real units on this one.
//                      It ends up being non-linear over the speed
//                      range the way the original SmartBeaconing(tm)
//                      algorithm works.
//
#define SB_TURN_TIME 5
// sb_turn_time         Dead-time before/after a corner peg beacon.
//                      Units are in seconds.
//
#define SB_POSIT_FAST 90
// sb_posit_fast        Fast posit rate, used if >= sb_high_speed_limit.
//                      Units are in seconds.
//
#define SB_POSIT_SLOW 30
// sb_posit_slow        Slow posit rate, used if <= sb_low_speed_limit.
//                      Units are in minutes.
//
#define SB_LOW_SPEED_LIMIT 2
// sb_low_speed_limit   Low speed limit, units are in Mph.
//
#define SB_HIGH_SPEED_LIMIT 24
// sb_high_speed_limit  High speed limit, units are in Mph.
