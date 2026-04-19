/*
 * Configuration File I/O Implementation
 * 
 * This module provides functions for reading receiver configuration
 * from text-based configuration files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/params.h"
#include "core/types.h"
#include "io/config_io.h"
#include "core/rcv.cuh"


RF_channel_t *read_receiver_config(const char *filename, int buffsize) {
    
    // Default values
    RF_channel_t *rf_ch = new_rf_ch(RECV_F_ADC, RECV_F_BW, RECV_F_LO, 1, buffsize);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening receiver config file");
        return rf_ch;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // Skip comments and empty lines
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == '\0' || *trimmed == '#' || *trimmed == '\n') continue;

        // Parse PAR = VALUE format
        char param[64];
        char value[64];
        if (sscanf(trimmed, "%63[^=]=%63s", param, value) == 2) {
            // Trim whitespace from parameter name
            char *end = param + strlen(param) - 1;
            while (end > param && (*end == ' ' || *end == '\t')) *end-- = '\0';

            double val = strtod(value, NULL);
            if (strcmp(param, "F_ADC") == 0) {
                rf_ch->f_adc = val;
            } else if (strcmp(param, "F_BW") == 0) {
                rf_ch->f_bw = val;
            } else if (strcmp(param, "F_LO") == 0) {
                rf_ch->f_lo = val;
            }
            else if (strcmp(param, "IQ") == 0) {
                rf_ch->iq = (int)val;
            }
        }
    }
    fclose(fp);

    return rf_ch;
}
