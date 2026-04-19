/*
 * Utility Functions for GPS Signal Processing
 * 
 * This header provides general-purpose utility functions for the
 * GPS signal processing library
 */

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

/*
 * Get current time in seconds with high resolution
 * Uses clock() for wall-clock time measurement
 *
 * Returns:
 *   Current time in seconds as a double-precision value
 */
double get_time_sec(void);

#endif /* CORE_UTILS_H */
