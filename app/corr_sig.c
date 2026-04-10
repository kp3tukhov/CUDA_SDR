/*
 * GPS Signal Correlator - Correlation Map Generator
 * 
 * This application performs correlation of received GPS signals with
 * locally generated PRN codes. It supports three correlation methods
 * and outputs correlation maps for visualization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <complex.h>

#include <time.h>
#include <getopt.h>


#include "core/params.h"
#include "core/types.h"
#include "core/utils.h"
#include "dsp/code.h"
#include "correlator/corr_interface.h"
#include "correlator/corr_interface_gpu.cuh"
#include "io/file_io.h"
#include "io/config_io.h"

/* ============================================================================
 * MAIN LOGIC
 * ============================================================================ */

void print_usage(const char *prog_name) {
    printf("GPS Signal Correlator - Correlation Map Generator\n\n");
    printf("Usage: %s -f <input_file> --prn <num> [options] -o <output_file>\n\n", prog_name);
    printf("Required arguments:\n");
    printf("  -f, --file <path>       Input binary file (int8_t samples).\n");
    printf("  --prn <num>             PRN number of the satellite (1-37).\n");
    printf("  -o, --output <path>     Output text file for 3D plot data.\n\n");
    printf("Optional arguments:\n");
    printf("  --dop_min <freq>        Minimum Doppler search frequency in Hz. (Default: %.0f)\n", DEFAULT_DOP_MIN);
    printf("  --dop_max <freq>        Maximum Doppler search frequency in Hz. (Default: %.0f)\n", DEFAULT_DOP_MAX);
    printf("  --dop_step <freq>       Doppler search step in Hz. (Default: %.1f)\n", DEFAULT_DOP_STEP);
    printf("  --tintegration <ms>     Integration time in ms. (Default: %.1f)\n", DEFAULT_T_INT_MS);
    printf("  --iq                    Specify if input file contains IQ data (default is I).\n");
    printf("  --method <1|2|3>        Correlation method:\n");
    printf("                          1: Time Domain (Brute Force)\n");
    printf("                          2: Parallel Frequency Search\n");
    printf("                          3: Parallel Code Search (FFT, Default)\n");
    printf("  --device <cpu|gpu>      Execution device (default: cpu).\n");
    printf("                          Note: GPU is only supported for method 3.\n");
    printf("                          If method 1 or 2 is selected with --device gpu,\n");
    printf("                          a warning will be printed and CPU will be used.\n");
    printf("  -h, --help              Show this help message.\n");
    printf("Example:\n");
    printf("  %s -f test_iq.bin --prn=5 --tint=10 --iq --method=3 --device=gpu -o test.dat \n", prog_name);
}


int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    char *input_file = NULL;
    char *output_file = NULL;
    int prn = -1;
    double dop_min = DEFAULT_DOP_MIN;
    double dop_max = DEFAULT_DOP_MAX;
    double dop_step = DEFAULT_DOP_STEP;
    double t_int = DEFAULT_T_INT_MS;
    int is_iq = 0;
    int method = METHOD_PARALLEL_CODE;
    device_t device = DEVICE_CPU;

    static struct option long_options[] = {
        {"file",         required_argument, 0, 'f'},
        {"prn",          required_argument, 0, 'p'},
        {"output",       required_argument, 0, 'o'},
        {"dop_min",      required_argument, 0, 'm'},
        {"dop_max",      required_argument, 0, 'x'},
        {"dop_step",     required_argument, 0, 's'},
        {"tintegration", required_argument, 0, 't'},
        {"iq",           no_argument,       0, 'q'},
        {"method",       required_argument, 0, 'k'},
        {"device",       required_argument, 0, 'd'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "f:p:o:m:x:s:t:q:k:d:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f': input_file = optarg; break;
            case 'p': prn = atoi(optarg); break;
            case 'o': output_file = optarg; break;
            case 'm': dop_min = atof(optarg); break;
            case 'x': dop_max = atof(optarg); break;
            case 's': dop_step = atof(optarg); break;
            case 't': t_int = atof(optarg); break;
            case 'q': is_iq = 1; break;
            case 'k': method = atoi(optarg); break;
            case 'd':
                if (strcmp(optarg, "gpu") == 0) {
                    device = DEVICE_GPU;
                } else {
                    device = DEVICE_CPU;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (input_file == NULL || output_file == NULL || prn < 1 || prn > 37) {
        fprintf(stderr, "Error: Missing required arguments or invalid PRN.\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Read receiver configuration
    receiver_t recv = read_receiver_config(RECV_CONFIG_FILE);
    recv.iq = is_iq;

    int n_samples_per_period = (int)round(recv.f_adc * (CODE_PERIOD_MS / 1000.0));
    int num_periods = (int)round(t_int / CODE_PERIOD_MS);

    if (num_periods < 1) {
        fprintf(stderr, "Error: Integration time must be at least %.1f ms.\n", CODE_PERIOD_MS);
        return EXIT_FAILURE;
    }

    int n_samples_total = n_samples_per_period * num_periods;

    printf("=== GPS Correlator Configuration ===\n");
    printf("Input file:      %s\n", input_file);
    printf("Output file:     %s\n", output_file);
    printf("PRN:             %d\n", prn);
    printf("Sample Rate:     %.2f MHz\n", recv.f_adc / 1e6);
    printf("IF Frequency:    %.2f MHz\n", recv.f_if / 1e6);
    printf("Integration:     %.1f ms (%d periods)\n", t_int, num_periods);
    printf("Samples/Period:  %d\n", n_samples_per_period);
    printf("Total Samples:   %d\n", n_samples_total);
    printf("Data Format:     %s\n", is_iq ? "IQ (int8_t)" : "I-Only (int8_t)");
    printf("Doppler Range:   [%.1f, %.1f] Hz\n", dop_min, dop_max);
    printf("Doppler Step:    %.1f Hz\n", dop_step);
    printf("Method:          %d ", method);
    switch(method) {
        case METHOD_TIME_DOMAIN: printf("(Time Domain)\n"); break;
        case METHOD_PARALLEL_FREQ: printf("(Parallel Freq)\n"); break;
        case METHOD_PARALLEL_CODE: printf("(Parallel Code)\n"); break;
        default: printf("(Unknown)\n");
    }
    printf("Device:          %s\n", device == DEVICE_GPU ? "GPU" : "CPU");
    printf("=====================================\n");

    double t_start_load = get_time_sec();

    // Read signal data from file
    float complex *signal_buffer = read_signal(input_file, is_iq, n_samples_total);

    if (signal_buffer == NULL) {
        fprintf(stderr, "Error: Failed to load signal data. Exiting.\n");
        return EXIT_FAILURE;
    }

    double t_end_load = get_time_sec();
    printf("[PROFILING] Data loading time: %.4f seconds\n", t_end_load - t_start_load);

    // Generate local PRN code
    int8_t *local_code = (int8_t *)malloc(sizeof(int8_t) * GPS_CODE_LEN);
    if (!local_code) {
        fprintf(stderr, "Error: Failed to allocate code buffer.\n");
        free(signal_buffer);
        return EXIT_FAILURE;
    }

    gen_code_L1CA(prn, local_code, GPS_CODE_LEN, 0);

    // Allocate memory for correlation result
    int n_rows = (int)round((dop_max - dop_min) / dop_step);
    int n_cols = (method == METHOD_PARALLEL_CODE) ? n_samples_per_period : GPS_CODE_LEN;

    double *result_map = (double *)malloc(sizeof(double) * n_rows * n_cols);

    if (!result_map) {
        fprintf(stderr, "Error: Failed to allocate result map.\n");
        free(signal_buffer);
        free(local_code);
        return EXIT_FAILURE;
    }

    // Configure correlator
    correlator_config_t config;
    config.dop_min = dop_min;
    config.dop_step = dop_step;
    config.n_dop = n_rows;
    config.num_periods = num_periods;
    config.method = method;
    config.verbose = 1;
    config.num_prns = 1;
    config.prns[0] = prn;
    for (int i = 1; i < MAX_SATS; i++) {
        config.prns[i] = 0;
    }

    printf("Starting correlation...\n");
    double t_start_corr = get_time_sec();

    // Check if GPU is requested but method is not parallel code
    int use_gpu = (device == DEVICE_GPU && method == METHOD_PARALLEL_CODE);

    int ret;
    if (use_gpu) {
        printf("Using GPU acceleration (method 3 - Parallel Code).\n");
        ret = batch_corr_execute_cuda(
            (const cuFloatComplex*) signal_buffer,
            local_code, &config, &recv,
            &result_map
        );
    } else {
        if (device == DEVICE_GPU) {
            fprintf(stderr, "Warning: GPU is only implemented for method 3 (Parallel Code Search).\n");
            fprintf(stderr, "         Falling back to CPU for method %d.\n", method);
        }
        ret = batch_corr_execute(
            signal_buffer,
            local_code, &config, &recv,
            &result_map
        );
    }

    double t_end_corr = get_time_sec();
    printf("[PROFILING] Correlation execution time: %.4f seconds\n", t_end_corr - t_start_corr);

    if (ret != 0) {
        fprintf(stderr, "Error: corr_execute failed.\n");
        free(signal_buffer);
        free(local_code);
        free(result_map);
        return EXIT_FAILURE;
    }

    printf("Correlation complete. Rows: %d, Cols: %d\n", n_rows, n_cols);

    double t_start_write = get_time_sec();
    write_corr_table(output_file, result_map, dop_min, dop_step, n_rows, n_cols);
    double t_end_write = get_time_sec();

    printf("[PROFILING] File writing time: %.4f seconds\n", t_end_write - t_start_write);
    printf("[PROFILING] TOTAL EXECUTION TIME: %.4f seconds\n", t_end_write - t_start_load);

    free(signal_buffer);
    free(local_code);
    free(result_map);

    return EXIT_SUCCESS;
}
