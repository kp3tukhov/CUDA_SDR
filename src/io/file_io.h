/*
 * File I/O Operations
 * 
 * This header provides functions for reading signal data from binary files
 * and writing correlation results to text files
 */

#ifndef IO_FILE_IO_H
#define IO_FILE_IO_H

/*
 * Read signal data from a binary file into raw format
 * Supports both I-only and IQ formats (int8_t samples)
 *
 * Parameters:
 *   filename  - Path to input binary file
 *   is_iq     - Non-zero for IQ data, zero for I-only
 *   buffer    - Raw bit-packed 8(4+4) signal buffer
 *   n_samples - Number of samples to read
 */
void read_raw(
    const char *filename,
    int is_iq,
    uint8_t *buffer,
    int n_samples
);

/*
 * Read signal data from a binary file
 * Supports both I-only and IQ formats (int8_t samples)
 *
 * Parameters:
 *   filename  - Path to input binary file
 *   is_iq     - Non-zero for IQ data, zero for I-only
 *   buffer    - Complex signal buffer
 *   n_samples - Number of samples to read
 */
void read_signal(
    const char *filename,
    int is_iq,
    float complex *buffer,
    int n_samples
);

/*
 * Write correlation map data to a text file
 * Output format: tab-separated columns (Doppler_Hz, Code_Sample, Power)
 *
 * Parameters:
 *   filename     - Path to output text file
 *   data         - Correlation power data (row-major: rows x cols)
 *   dop_min      - Minimum Doppler frequency (Hz)
 *   dop_step     - Doppler frequency step (Hz)
 *   rows         - Number of Doppler bins
 *   cols         - Number of code samples per Doppler bin
 */
void write_corr_table(
    const char *filename,
    float *data,
    double dop_min,
    double dop_step,
    int rows,
    int cols
);

#endif /* IO_FILE_IO_H */
