#ifndef srgb_from_lambda_h
#define srgb_from_lambda_h

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Convert a wave length given in nm to a RGB triplet where each value
 * is in the range [0,1]. It is suggested that lambda_nm is in the range
 * [425, 650]. The output array needs to be allocated before the call.
 * This implementation uses an approximation of the CIE 1931 data.
 */
void srgb_from_lambda(double lambda_nm, double * output_RGB);

#endif
