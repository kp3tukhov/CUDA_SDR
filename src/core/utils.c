/*
 * Utility Functions for GPS Signal Processing Implementation
 * 
 * This module provides general-purpose utility functions for the
 * GPS signal processing library
 */

#include <time.h>
#include "utils.h"


double get_time_sec(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}
