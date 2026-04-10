/*
 * File I/O Operations
 * 
 * This header provides functions for reading signal data from binary files
 * and writing correlation results to text files
 */

#ifndef IO_FILE_IO_H
#define IO_FILE_IO_H

#include <complex.h>
#include "core/types.h"


/*
 * Read signal data from a binary file
 * Supports both I-only and IQ formats (int8_t samples)
 *
 * Parameters:
 *   filename  - Path to input binary file
 *   is_iq     - Non-zero for IQ data, zero for I-only
 *   n_samples - Number of samples to read
 *
 * Returns:
 *   Pointer to complex signal buffer (caller must free), or NULL on error
 */
float complex* read_signal(
    const char *filename,
    int is_iq,
    int n_samples
);

/*
 * Write correlation map data to a text file
 * Output format: tab-separated columns (Doppler_Hz, Code_Sample, Power)
 *
 * Parameters:
 *   filename     - Path to output text file
 *   data         - Correlation power data (row-major: rows x cols)
 *   doppler_min  - Minimum Doppler frequency (Hz)
 *   doppler_step - Doppler frequency step (Hz)
 *   rows         - Number of Doppler bins
 *   cols         - Number of code samples per Doppler bin
 */
void write_corr_table(
    const char *filename,
    double *data,
    double doppler_min,
    double doppler_step,
    int rows,
    int cols
);

#endif /* IO_FILE_IO_H */
