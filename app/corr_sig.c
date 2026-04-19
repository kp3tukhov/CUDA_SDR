/*
 * GPS Signal Correlator - Correlation Map Generator
 * 
 * This application performs correlation of received GPS signals with
 * locally generated PRN codes. It supports three correlation methods
 * and outputs correlation maps for visualization.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <complex.h>
#include <time.h>
#include <getopt.h>


#include "core/params.h"
#include "core/types.h"
#include "core/rcv.cuh"
#include "correlator/corr_interface.h"
#include "core/utils.h"
#include "io/file_io.h"
#include "io/config_io.h"
#include "code/getcode.h"


void print_usage(const char *prog_name) {
    printf("GPS Signal Correlator - Correlation Map Generator\n\n");
    printf("Usage: %s -f <input_file> --prn <num> [options] -o <output_file>\n\n", prog_name);
    printf("Required arguments:\n");
    printf("  -f, --file <path>       Input binary file (int8_t samples).\n");
    printf("  --prn <num>             PRN number of the satellite (1-37).\n");
    printf("  -o, --output <path>     Output text file for 3D plot data.\n\n");
    printf("Optional arguments:\n");
    printf("  --sys <system>          Satellite system. (Default: GPS)\n");
    printf("  --sig <signal>          Satellite signal. (Default: L1CA)\n");
    printf("  --dop_min <freq>        Minimum Doppler search frequency in Hz. (Default: %.0f)\n", DEFAULT_DOP_MIN);
    printf("  --dop_max <freq>        Maximum Doppler search frequency in Hz. (Default: %.0f)\n", DEFAULT_DOP_MAX);
    printf("  --dop_step <freq>       Doppler search step in Hz. (Default: %.1f)\n", DEFAULT_DOP_STEP);
    printf("  --tintegration <ms>     Integration time in ms. (Default: %.1f)\n", DEFAULT_T_INT_MS);
    printf("  --verbose               Print detailed information.\n");
    printf("  --method <1|2|3>        Correlation method:\n");
    printf("                          1: Time Domain (Brute Force)\n");
    printf("                          2: Parallel Frequency Search\n");
    printf("                          3: Parallel Code Search (FFT, Default)\n");
    printf("  --device <cpu|gpu>      Execution device (default: cpu).\n");
    printf("                          Note: GPU is only supported for method 3.\n");
    printf("                          If method 1 or 2 is selected with --device gpu,\n");
    printf("                          a warning will be printed and method 3 will be used.\n");
    printf("  -h, --help              Show this help message.\n");
    printf("Example:\n");
    printf("  %s -f test_iq.bin --prn=5 --tint=10 --verbose --method=3 --device=gpu -o test.dat \n", prog_name);
}

/* ============================================================================
 * MAIN LOGIC
 * ============================================================================ */

int main(int argc, char *argv[]) {

    // Default parameters
    char *input_file = NULL;
    char *output_file = NULL;
    int prn = -1;
    double dop_min = DEFAULT_DOP_MIN;
    double dop_max = DEFAULT_DOP_MAX;
    double dop_step = DEFAULT_DOP_STEP;
    double t_int = DEFAULT_T_INT_MS;
    int verbose = 0;
    int method = METHOD_PARALLEL_CODE;
    device_t device = DEVICE_CPU;
    sys_t sys = SYS_GPS;
    sig_t sig = SIG_L1CA;

    static struct option long_options[] = {
        {"file",         required_argument, 0, 'f'},
        {"prn",          required_argument, 0, 'p'},
        {"output",       required_argument, 0, 'o'},
        {"sys",          required_argument, 0, 'g'},
        {"sig",          required_argument, 0, 'l'},
        {"dop_min",      required_argument, 0, 'm'},
        {"dop_max",      required_argument, 0, 'x'},
        {"dop_step",     required_argument, 0, 's'},
        {"tintegration", required_argument, 0, 't'},
        {"verbose",      no_argument,       0, 'v'},
        {"method",       required_argument, 0, 'k'},
        {"device",       required_argument, 0, 'd'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "f:p:o:g:l:m:x:s:t:v:k:d:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f': input_file = optarg; break;
            case 'p': prn = atoi(optarg); break;
            case 'o': output_file = optarg; break;
            case 'g':
                if (strcmp(optarg, "G") == 0) {
                    sys = SYS_GPS;
                }
                else {
                    sys = SYS_GPS;
                }
                break;
            case 'l':
                if (strcmp(optarg, "L1CA") == 0) {
                    sig = SIG_L1CA;
                }
                else {
                    sig = SIG_L1CA;
                }
                break;           
            case 'm': dop_min = atof(optarg); break;
            case 'x': dop_max = atof(optarg); break;
            case 's': dop_step = atof(optarg); break;
            case 't': t_int = atof(optarg); break;
            case 'v': verbose = 1; break;
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

    if (!input_file || !output_file || prn < 1 || prn > 37) {
        fprintf(stderr, "Error: Missing required arguments or invalid PRN.\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int t_period_ms = get_period_ms(sys, sig);
    int code_len = get_code_len(sys, sig);

    // Read receiver configuration
    RF_channel_t *rf_ch = read_receiver_config(RECV_CONFIG_FILE, 1<<20);

    // Determine signal size
    int n_samples_per_period = (int)round(rf_ch->f_adc * (t_period_ms / 1000.0));
    int num_periods = (int)round(t_int / t_period_ms);


    int n_rows = (int)round((dop_max - dop_min) / dop_step);
    int n_cols = (method == METHOD_PARALLEL_CODE) ? n_samples_per_period : code_len;
    sys_t syss[1];
    sig_t sigs[1];
    int prns[1];
    int num_prns = 1;
    syss[0] = sys;
    sigs[0] = sig;
    prns[0] = prn;


    if (num_periods < 1) {
        fprintf(stderr, "Error: Integration time must be at least %.1f ms.\n", (double)t_period_ms);
        return EXIT_FAILURE;
    }

    int n_samples_total = n_samples_per_period * num_periods;

    if (verbose) {
        printf("\n");
        printf("=== GNSS Correlator Configuration ===\n");
        printf("Input file:      %s\n", input_file);
        printf("Output file:     %s\n", output_file);
        printf("PRN:             %d\n", prn);
        printf("Sample Rate:     %.2f MHz\n", rf_ch->f_adc / 1e6);
        printf("IF Frequency:    %.2f MHz\n", (get_carrier(sys, sig) - rf_ch->f_lo) / 1e6);
        printf("Integration:     %.1f ms (%d periods)\n", t_int, num_periods);
        printf("Total Samples:   %d\n", n_samples_total);
        printf("Data Format:     %s\n", rf_ch->iq ? "IQ (2 x int8_t)" : "I-Only (int8_t)");
        printf("Doppler Range:   [%.1f, %.1f] Hz\n", dop_min, dop_max);
        printf("Doppler Step:    %.1f Hz\n", dop_step);
        printf("Method:          %d ", method);
        switch(method) {
            case METHOD_TIME_DOMAIN: printf("(Time Domain)\n"); break;
            case METHOD_PARALLEL_FREQ: printf("(Parallel Freq)\n"); break;
            case METHOD_PARALLEL_CODE:
            default: printf("(Parallel Code)\n"); break;
        }
        printf("Device:          %s\n", device == DEVICE_GPU ? "GPU" : "CPU");
        printf("=====================================\n\n");
    }

    // Read signal in raw (bit-packed) format
    read_raw(input_file, rf_ch->iq, rf_ch->buffer, n_samples_total);

    // Initialize receiver (once)
    double t_start = get_time_sec();

    GNSSReceiver_t *rcv = new_receiver(rf_ch);
    if (!rcv) {
        fprintf(stderr, "Error: Failed to create receiver.\n");
        return EXIT_FAILURE;
    }

    correlator_config_t cfg;
    cfg.dop_min = dop_min;
    cfg.dop_step = dop_step;
    cfg.n_dop = n_rows;
    cfg.n_samples = n_samples_per_period;
    cfg.num_periods = num_periods;
    cfg.method = method;
    cfg.device = device;

    if (add_block_to_receiver(rcv, syss, sigs, prns, num_prns, &cfg) != 0) {
        fprintf(stderr, "Error: Failed to add block to receiver.\n");
        free_receiver(rcv);
        return EXIT_FAILURE;
    }

    printf("Starting correlation...\n");

    // Process signal
    double t_start_corr = get_time_sec();

    int ret;

    ret = process_receiver(rcv);
    if (ret != 0) {
        fprintf(stderr, "Error: process_receiver failed.\n");
        free_receiver(rcv);
        free_rf_ch(rf_ch);
        return EXIT_FAILURE;
    }

    double t_end_corr = get_time_sec();
    
    // Write results
    printf("Correlation complete. Rows: %d, Cols: %d\n", n_rows, n_cols);
    printf("Resource preparation time: %8.3f ms\n", 1000*(t_start_corr - t_start));
    printf("Correlation time: %8.3f ms\n", 1000*(t_end_corr - t_start_corr));

    write_corr_table(
        output_file,
        rcv->blocks[0]->channels[0]->acq->corr_map,
        dop_min, dop_step,
        n_rows, n_cols
    );
    printf("Results saved to %s\n", output_file);

    free_receiver(rcv);
    free_rf_ch(rf_ch);

    return EXIT_SUCCESS;
}
