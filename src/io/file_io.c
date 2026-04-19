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


void read_raw(
    const char *filename,
    int is_iq,
    uint8_t *buffer,
    int n_samples
) {
    size_t read_size = n_samples * (is_iq ? 2 : 1);

    int8_t *data = (int8_t*)malloc(read_size);
    if (!data) {
        fprintf(stderr, "Error: Failed to allocate memory for input buffer.\n");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening input file");
        free(data);
        return;
    }

    fread(data, 1, read_size, fp);
    fclose(fp);

    if (is_iq) {
        for (int i = 0; i < n_samples; i++) {
            buffer[i] = (uint8_t)((data[2*i + 1] << 4) | (data[2*i]& 0xF));
        }
    } else {
        for (int i = 0; i < n_samples; i++) {
            buffer[i] = (uint8_t)(data[i] & 0xF);
        }
    }

    free(data);
}


void read_signal(
    const char *filename,
    int is_iq,
    float complex *buffer,
    int n_samples
) {
    size_t read_size = n_samples * (is_iq ? 2 : 1);

    int8_t *data = (int8_t*)malloc(read_size);
    if (!data) {
        fprintf(stderr, "Error: Failed to allocate memory for input buffer.\n");
        return;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening input file");
        free(data);
        return;
    }

    fread(data, 1, read_size, fp);
    fclose(fp);

    if (is_iq) {
        for (int i = 0; i < n_samples; i++) {
            buffer[i] = (float)data[2*i] + I * (float)data[2*i + 1];
        }
    } else {
        for (int i = 0; i < n_samples; i++) {
            buffer[i] = (float)data[i] + I * 0.0f;
        }
    }

    free(data);
}


void write_corr_table(
    const char *filename,
    float *data,
    double dop_min,
    double dop_step,
    int rows,
    int cols
) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        perror("Error opening output file");
        return;
    }

    fprintf(fp, "# Doppler_Hz\tCode_Sample\tPower\n");

    for (int r = 0; r < rows; r++) {
        double current_doppler = dop_min + r * dop_step;
        for (int c = 0; c < cols; c++) {
            double power = data[r * cols + c];
            fprintf(fp, "%.2f\t%d\t%.6f\n", current_doppler, c, power);
        }
    }

    fclose(fp);
}
