/*
 * Configuration File I/O
 * 
 * This header provides functions for reading receiver configuration
 * from text-based configuration files
 */

#ifndef IO_CONFIG_IO_H
#define IO_CONFIG_IO_H

/*
 * Read RF channel configuration from a file
 * Parses key-value pairs (e.g., F_ADC = 8.0e6)
 *
 * Parameters:
 *   filename - Path to configuration file
 *   buffsize - Size of a buffer to be allocated for channel data
 *
 * Returns:
 *   RF_channel_t* pointer to the allocated structure with parsed values (defaults used for missing params)
 */
RF_channel_t *read_receiver_config(const char *filename, int buffsize);

#endif /* IO_CONFIG_IO_H */
