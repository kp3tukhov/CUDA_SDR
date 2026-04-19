/*
 * GNSS spreading-code access (dispatch layer)
 *
 * This module implements get_code and nominal signal parameters by system/signal
 */

#include <math.h>
#include <stdint.h>

#include "core/params.h"
#include "core/types.h"
#include "code/code_gps.h"
#include "code/getcode.h"


void get_code(sys_t sys, sig_t sig, int prn, int8_t *buff) {

    switch (sys) {
        case SYS_GPS:
            switch (sig) {
                case SIG_L1CA:
                        gen_code_L1CA(prn, buff, G_L1CA_CLEN, 0);
                        break;
            }
    }
    
}

int get_code_len(sys_t sys, sig_t sig) {

    switch (sys) {
        case SYS_GPS:
            switch (sig) {
                case SIG_L1CA:
                        return G_L1CA_CLEN;
            }
    }

    return 0;

}

double get_carrier(sys_t sys, sig_t sig) {

    switch (sys) {
        case SYS_GPS:
            switch (sig) {
                case SIG_L1CA:
                        return G_L1_CARR;
            }
    }

    return 0;

}

double get_chiprate(sys_t sys, sig_t sig) {

    switch (sys) {
        case SYS_GPS:
            switch (sig) {
                case SIG_L1CA:
                        return G_L1CA_CRATE;
            }
    }

    return 0;

}

int get_period_ms(sys_t sys, sig_t sig) {

    int code_len = get_code_len(sys, sig);
    int chiprate = get_chiprate(sys, sig);

    return 1000.0 * (double) code_len / (double)chiprate;

}