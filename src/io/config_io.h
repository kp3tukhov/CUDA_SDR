/*
 * Configuration File I/O
 * 
 * This header provides functions for reading receiver configuration
 * from text-based configuration files
 */

#ifndef IO_CONFIG_IO_H
#define IO_CONFIG_IO_H

#include "core/types.h"


/*
 * Read receiver configuration from a file
 * Parses key-value pairs (e.g., F_ADC = 8.0e6)
 *
 * Parameters:
 *   filename - Path to configuration file
 *
 * Returns:
 *   receiver_t structure with parsed values (defaults used for missing params)
 */
receiver_t read_receiver_config(const char *filename);

#endif /* IO_CONFIG_IO_H */
