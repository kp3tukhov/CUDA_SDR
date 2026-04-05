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


receiver_t read_receiver_config(const char *filename) {
    receiver_t cfg;
    // Default values
    cfg.f_adc = RECV_F_ADC;
    cfg.f_bw = RECV_F_BW;
    cfg.f_lo = RECV_F_LO;
    cfg.f_if = F_GPS_L1 - cfg.f_lo;

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Error opening receiver config file");
        return cfg;
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
                cfg.f_adc = val;
            } else if (strcmp(param, "F_BW") == 0) {
                cfg.f_bw = val;
            } else if (strcmp(param, "F_LO") == 0) {
                cfg.f_lo = val;
            }
            else if (strcmp(param, "IQ") == 0) {
                cfg.iq = (int)val;
            }
        }
    }
    fclose(fp);

    cfg.f_if = F_GPS_L1 - cfg.f_lo;
    return cfg;
}
