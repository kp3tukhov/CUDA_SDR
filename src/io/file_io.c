/*
 * File I/O Operations Implementation
 * 
 * This module provides functions for reading signal data from binary files
 * and writing correlation results to text files
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <complex.h>
#include "io/file_io.h"


float complex* read_signal(
    const char *filename,
    int is_iq,
    int n_samples
) {
    size_t buffer_size = n_samples * (is_iq ? 2 : 1);

    int8_t *raw_data = (int8_t*)malloc(buffer_size);
    if (!raw_data) {
        fprintf(stderr, "Error: Failed to allocate memory for input buffer.\n");
        return NULL;
    }


    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening input file");
        free(raw_data);
        return NULL;
    }

    fread(raw_data, 1, buffer_size, fp);
    fclose(fp);

    // Allocate complex signal buffer
    float complex *signal_buffer = (float complex*)malloc(sizeof(float complex) * n_samples);
    if (!signal_buffer) {
        fprintf(stderr, "Error: Failed to allocate signal buffer.\n");
        free(raw_data);
        return NULL;
    }

    if (is_iq) {
        for (int i = 0; i < n_samples; i++) {
            signal_buffer[i] = (float)raw_data[2*i] + I * (float)raw_data[2*i + 1];
        }
    } else {
        for (int i = 0; i < n_samples; i++) {
            signal_buffer[i] = (float)raw_data[i] + I * 0.0f;
        }
    }

    free(raw_data);

    return signal_buffer;
}


void write_corr_table(
    const char *filename,
    double *data,
    double doppler_min,
    double doppler_step,
    int rows,
    int cols
) {
    // Write correlation results to file
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Error opening output file");
        return;
    }

    fprintf(fp, "# Doppler_Hz\tCode_Sample\tPower\n");

    for (int r = 0; r < rows; r++) {
        double current_doppler = doppler_min + r * doppler_step;
        for (int c = 0; c < cols; c++) {
            double power = data[r * cols + c];
            fprintf(fp, "%.2f\t%d\t%.6f\n", current_doppler, c, power);
        }
    }

    fclose(fp);
    printf("Results saved to %s\n", filename);
}
