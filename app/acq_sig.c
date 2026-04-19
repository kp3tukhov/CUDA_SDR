/*
 * GPS Signal Acquisition Application
 * 
 * This application performs satellite signal acquisition from recorded
 * GPS data. It searches for satellites across Doppler and code phase
 * dimensions, detects peaks, and estimates C/N0 ratios.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <complex.h>
#include <time.h>
#include <getopt.h>

#include "core/params.h"
#include "core/types.h"
#include "core/rcv.cuh"
#include "correlator/corr_interface.h"
#include "acquisition/acq_processor.h"
#include "core/utils.h"
#include "io/file_io.h"
#include "io/config_io.h"
#include "code/getcode.h"


#define DEFAULT_THRESHOLD_DB 35.0  // Default detection threshold (C/N0 in dB-Hz)

/* ============================================================================
 * HELPER FUNCTIONS
 * ============================================================================ */

static void print_usage(const char *prog_name) {
    printf("GPS Signal Acquisition\n\n");
    printf("Usage: %s -f <input_file> [options]\n\n", prog_name);
    printf("Required arguments:\n");
    printf("  -f, --file <path>           Input binary file (int8_t samples).\n");
    printf("\nOptional arguments:\n");
    printf("  --sys <system>              Satellite system. (Default: GPS)\n");
    printf("  --sig <signal>              Satellite signal. (Default: L1CA)\n");
    printf("  --dop_min <freq>            Minimum Doppler search frequency in Hz. (Default: %.0f)\n", DEFAULT_DOP_MIN);
    printf("  --dop_max <freq>            Maximum Doppler search frequency in Hz. (Default: %.0f)\n", DEFAULT_DOP_MAX);
    printf("  --dop_step <freq>           Doppler search step in Hz. (Default: %.1f)\n", DEFAULT_DOP_STEP);
    printf("  --tintegration <ms>         Integration time in ms. (Default: %.1f)\n", DEFAULT_T_INT_MS);
    printf("  --device <cpu|gpu>          Execution device (default: cpu).\n");
    printf("                              Note: GPU is only supported for method 3.\n");
    printf("                              If method 1 or 2 is selected with --device gpu,\n");
    printf("                              a warning will be printed and method 3 will be used.\n");
    printf("  --prn <num>                 Search only specified PRN (1-37). If not specified, scan all.\n");
    printf("  --threshold <cn0>           Detection threshold (C/N0 in dB-Hz). (Default: %.1f)\n", DEFAULT_THRESHOLD_DB);
    printf("  -h, --help                  Show this help message.\n");
    printf("\nOutput:\n");
    printf("  Table of detected satellites with parameters:\n");
    printf("  PRN | Status | C/N0 (dB-Hz) | Doppler (Hz) | Code Phase (chips)\n");
    printf("Example:\n");
    printf("  %s -f test_iq.bin --tint=10 --device=gpu --threshold=35 \n", prog_name);
}

/*
 * Print result table header
 */
static void print_result_header(void) {
    printf("\n");
    printf("==================================================================\n");
    printf("                  GPS SIGNAL ACQUISITION RESULTS                  \n");
    printf("==================================================================\n");
    printf(" PRN | Status | C/N0 (dB-Hz) | Doppler (Hz) | Code Offset (ms)   |\n");
    printf("-----+--------+--------------+--------------+--------------------+\n");
}

/*
 * Print result row for a single satellite
 */
static void print_result_row(const satellite_channel_t *result) {

    printf(" %3d | %6s | %10.2f   | %10.2f   | %17.5f\n",
            result->prn,
            result->active ? "FOUND" : "----",
            result->cn0_db_hz,
            result->fdoppler,
            result->code_phase_start_chips/get_code_len(result->sys, result->sig));

}

/*
 * Print summary statistics
 */
static void print_summary(int total_prn, int detected_count, double max_cn0, int best_prn) {
    printf("-----+--------+--------------+--------------+-------------------+\n");
    printf("\nSummary:\n");
    printf("  Total PRN scanned: %d\n", total_prn);
    printf("  Satellites detected: %d\n", detected_count);
    if (detected_count > 0) {
        printf("  Best C/N0: %.2f dB-Hz (PRN %d)\n", max_cn0, best_prn);
    }
    printf("\n");
}


/* ============================================================================
 * MAIN
 * ============================================================================ */

int main(int argc, char *argv[]) {

    // Default parameters
    const char *input_file = NULL;
    double dop_min = DEFAULT_DOP_MIN;
    double dop_max = DEFAULT_DOP_MAX;
    double dop_step = DEFAULT_DOP_STEP;
    double t_int = DEFAULT_T_INT_MS;
    device_t device = DEVICE_CPU;
    sys_t sys = SYS_GPS;
    sig_t sig = SIG_L1CA;
    int specific_prn = -1;  // -1 = scan all
    double threshold_db = DEFAULT_THRESHOLD_DB;

    // Parse command line arguments
    static struct option long_options[] = {
        {"file",         required_argument, 0, 'f'},
        {"sys",          required_argument, 0, 'g'},
        {"sig",          required_argument, 0, 'l'},
        {"dop_min",      required_argument, 0, 'd'},
        {"dop_max",      required_argument, 0, 'm'},
        {"dop_step",     required_argument, 0, 's'},
        {"tintegration", required_argument, 0, 't'},
        {"device",       required_argument, 0, 'v'},
        {"prn",          required_argument, 0, 'p'},
        {"threshold",    required_argument, 0, 'T'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "f:g:l:d:m:s:t:v:p:T:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'f':
                input_file = optarg;
                break;
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
            case 'd':
                dop_min = atof(optarg);
                break;
            case 'm':
                dop_max = atof(optarg);
                break;
            case 's':
                dop_step = atof(optarg);
                break;
            case 't':
                t_int = atof(optarg);
                break;
            case 'v':
                if (strcmp(optarg, "gpu") == 0) {
                    device = DEVICE_GPU;
                } else {
                    device = DEVICE_CPU;
                }
                break;
            case 'p':
                specific_prn = atoi(optarg);
                if (specific_prn < 1 || specific_prn > 37) {
                    fprintf(stderr, "Error: PRN must be between 1 and 37.\n");
                    return EXIT_FAILURE;
                }
                break;
            case 'T':
                threshold_db = atof(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // Check required arguments
    if (!input_file) {
        fprintf(stderr, "Error: Input file is required (-f option).\n\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    int t_period_ms = get_period_ms(sys, sig);

    // Read receiver configuration
    RF_channel_t *rf_ch = read_receiver_config(RECV_CONFIG_FILE, 1<<20);

    // Determine signal size
    int n_samples_per_period = (int)round(rf_ch->f_adc * t_period_ms / 1000.0);
    int num_periods = (int)round(t_int / t_period_ms);

    if (num_periods < 1) {
        fprintf(stderr, "Error: Integration time must be at least %.1f ms.\n", (double)t_period_ms);
        return EXIT_FAILURE;
    }

    int n_samples_total = n_samples_per_period * num_periods;
    
    // Read signal in raw (bit-packed) format
    read_raw(input_file, rf_ch->iq, rf_ch->buffer, n_samples_total);

    int n_doppler = (int)round((dop_max - dop_min) / dop_step);

    // Determine PRN range
    int prn_start = (specific_prn > 0) ? specific_prn : 1;
    int prn_end = (specific_prn > 0) ? specific_prn : MAX_SATS;
    int num_prns = prn_end - prn_start + 1;
    

    sys_t syss[MAX_SATS];
    sig_t sigs[MAX_SATS];
    int prns[MAX_SATS];
    for (int i = 0; i < num_prns; i++) {
        syss[i] = sys;
        sigs[i] = sig;
        prns[i] = prn_start + i;
    }


    // Initialize receiver (once)
    GNSSReceiver_t *rcv = new_receiver(rf_ch);
    
    if (!rcv) {
        fprintf(stderr, "Error: Failed to create receiver.\n");
        return EXIT_FAILURE;
    }
    
    correlator_config_t cfg;
    cfg.dop_min = dop_min;
    cfg.dop_step = dop_step;
    cfg.n_dop = n_doppler;
    cfg.n_samples = n_samples_per_period;
    cfg.num_periods = num_periods;
    cfg.method = METHOD_PARALLEL_CODE;
    cfg.device = device; 

    if (add_block_to_receiver(rcv, syss, sigs, prns, num_prns, &cfg) != 0) {
        fprintf(stderr, "Error: Failed to add block to receiver.\n");
        free_receiver(rcv);
        return EXIT_FAILURE;
    }
    
    // Process signal
    // Cycle for checking performance changing
    for(int i = 0; i < 5; i++) {

        double total_start = get_time_sec();

        int ret;

        // Correlate all (device)
        ret = process_receiver(rcv);
        if (ret != 0) {
            fprintf(stderr, "Error: process_receiver failed.\n");
            free_receiver(rcv);
            free_rf_ch(rf_ch);
            return EXIT_FAILURE;
        }
        
        satellite_channel_block_t *blk = rcv->blocks[0];

        // Acquire all (CPU)
        for (int i = 0; i < blk->num_channels; i++) {
            satellite_channel_t *sat = blk->channels[i];

            perform_acquisition(sat, threshold_db);
        }
        
        double total_time = get_time_sec() - total_start;

        // Print results
        int detected_count = 0;
        double max_cn0 = 0.0;
        int best_prn = -1;

        for (int i = 0; i < num_prns; i++) {
            if (blk->channels[i]->active) {
                detected_count++;
                if (blk->channels[i]->cn0_db_hz > max_cn0) {
                    max_cn0 = blk->channels[i]->cn0_db_hz;
                    best_prn = blk->channels[i]->prn;
                }
            }
        }

        print_result_header();
        for (int i = 0; i < num_prns; i++) {
            print_result_row(blk->channels[i]);
        }
        print_summary(num_prns, detected_count, max_cn0, best_prn);

        printf("Total acquisition time: %8.3f ms\n", 1000*total_time);
        if (detected_count > 0) {
            printf("Average time per satellite: %8.3f ms\n", 1000*(total_time / num_prns));
        }

        sleep(1);
    }

    free_receiver(rcv);
    free_rf_ch(rf_ch);

    return EXIT_SUCCESS;
}
