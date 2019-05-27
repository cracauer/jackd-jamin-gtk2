/*
 *  Copyright (C) 2003 Jan C. Depner, Jack O'Quin, Steve Harris
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id: process.c,v 1.84 2013/02/09 15:47:30 kotau Exp $
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <jack/jack.h>
#include <fftw3.h>
#include <assert.h>

#include "config.h"
#include "state.h"
#include "process.h"
#include "compressor.h"
#include "limiter.h"
#include "limiter-ui.h"
#include "geq.h"
#include "scenes.h"
#include "intrim.h"
#include "io.h"
#include "db.h"
#include "denormal-kill.h"
#include "rms.h"


#define BIQUAD_TYPE double
#include "biquad.h"

#define BUF_MASK   (BINS-1)		/* BINS is a power of two */

#define LERP(f,a,b) ((a) + (f) * ((b) - (a)))

#define IS_DENORMAL(fv) (((*(unsigned int*)&(fv))&0x7f800000)!=0)
typedef FFTW_TYPE fft_data;

static int xo_band_action[XO_NBANDS] = {ACTIVE, ACTIVE, ACTIVE};
static int xo_band_action_pending[XO_NBANDS] = {ACTIVE, ACTIVE, ACTIVE};

/* These values need to be controlled by the UI, somehow */
float xover_fa = 150.0f;
float xover_fb = 1200.0f;
comp_settings compressors[XO_NBANDS];
lim_settings limiter[2];
int limiter_plugin = FAST;
float eq_coefs[BINS]; /* Linear gain of each FFT bin */
float lim_peak[2];

static int iir_xover = 0;
static unsigned int delay_mask;


/*  Low and mid band delays.  Note that there is no high band delay
    (it makes no sense) but there is a slot for it ('cause it was 
    easier to deal with ;-)  */

static float *delay_buf[NCHANNELS][XO_NBANDS];


/*  save_delay saves the actual time setting from preferences.  */

static float save_delay[XO_NBANDS] = {0.0, 0.0, 0.0};


/*  If delay is set to 0 then the band delay push button is off but 
    we will stuff the actual delay (in samples) in here when we want
    to delay).  */

static int delay[XO_NBANDS] = {0, 0, 0};


/*  Set this if we want to set the actual delays on the next pass through.  */

static int delay_pending[XO_NBANDS] = {0, 0, 0};


float in_peak[NCHANNELS], out_peak[NCHANNELS], rms_peak[NCHANNELS];
static rms *r[2] = {NULL, NULL};
static gboolean rms_ready = FALSE;

static float band_f[BANDS];
static float gain_fix[BANDS];
static float bin_peak[BINS];
static int bands[BINS];
static float in_buf[NCHANNELS][BINS];
static float mid_buf[NCHANNELS][BINS];
static float out_buf[NCHANNELS][XO_NBANDS][BINS];
static biquad xo_filt[NCHANNELS][(XO_NBANDS-1) * 2];
static float window[BINS];
static fft_data *real;
static fft_data *comp;
static fft_data *comp_tmp;
static float *out_tmp[NCHANNELS][XO_NBANDS];
static float sw_m_gain[XO_NBANDS];
static float sw_s_gain[XO_NBANDS];
static float sb_l_gain[XO_NBANDS];
static float sb_r_gain[XO_NBANDS];
static float limiter_gain = 1.0f;
static int limiter_plugin_pending = FAST;
static int limiter_plugin_change_pending = FALSE;
static float logscale_pending = -1.0;

static float ws_boost_wet = 0.0f;
static float ws_boost_a = 1.0f;

static unsigned int latcorbuf_pos;
static unsigned int latcorbuf_len;
static float *latcorbuf[BCHANNELS];
static float *latcorbuf_postcomp[BCHANNELS];

static int spectrum_mode = SPEC_POST_EQ;

static int global_bypass = FALSE;
static int eq_bypass_pending = FALSE;
static int eq_bypass = FALSE;
static int limiter_bypass = FALSE;
static int limiter_bypass_pending = FALSE;
static int rms_time_slice;

volatile int global_main_gui = 0;		/* updated from GUI thread */
volatile int global_multiout_gui = 0;		/* updated from GUI thread */

/* Data for plugins */
plugin *comp_plugin, *lim_plugin[2];

/* FFTW data */
fftwf_plan plan_rc = NULL, plan_cr = NULL;

float sample_rate = 0.0f;

/* Desired block size for calling process_signal(). */
const jack_nframes_t dsp_block_size = BINS / OVER_SAMP;

#ifdef FILTER_TUNING
float ft_bias_a_val = 1.0f;
float ft_bias_a_hp_val = 1.0f;
float ft_bias_b_val = 1.0f;
float ft_bias_b_hp_val = 1.0f;
float ft_rez_lp_a_val = 1.2;
float ft_rez_hp_a_val = 1.2;
float ft_rez_lp_b_val = 1.2;
float ft_rez_hp_b_val = 1.2;
#endif

void run_eq(unsigned int port, unsigned int in_pos);
void run_eq_iir(unsigned int port, unsigned int in_pos);
void run_width(int xo_band, float *left, float *right, int nframes);
static void k_window_init(int alpha, float *window, int n, int iter);
static void kbd_window_init(int alpha, float *window, int n, int iter);

void process_init(float fs)
{
    float centre = 25.0f;
    unsigned int i, j, band;

    sample_rate = fs;

    for (i = 0; i < BANDS; i++) {
		band_f[i] = centre;
		//printf("band %d = %fHz\n", i, centre);
		centre *= 1.25992105f;		/* up a third of an octave */
		gain_fix[i] = 0.0f;
    }

    band = 0;
    for (i = 0; i < BINS / 2; i++) {
		const float binfreq = sample_rate * 0.5f * (i + 0.5f) / (float) BINS;

		while (binfreq > (band_f[band] + band_f[band + 1]) * 0.5f) {
			band++;
			if (band >= BANDS - 1) {
				band = BANDS - 1;
				break;
			}
		}
		bands[i] = band;
		gain_fix[band]++;
		//printf("bin %d (%f) -> band %d (%f) #%d\n", i, binfreq, band, band_f[band], (int)gain_fix[band]);
	}

	for (i = 0; i < BANDS; i++) {
		if (gain_fix[i] != 0.0f) {
			gain_fix[i] = 1.0f / gain_fix[i];
		} else {
			/* There are no bins for this band, reassign a nearby one */
			for (j = 0; j < BINS / 2; j++) {
				if (bands[j] > i) {
					gain_fix[bands[j]]--;
					bands[j] = i;
					gain_fix[i] = 1.0f;
					break;
				}
			}
		}
	}

    /* Allocate space for FFT data */
    real = fftwf_malloc(sizeof(fft_data) * BINS);
    comp = fftwf_malloc(sizeof(fft_data) * BINS);
    comp_tmp = fftwf_malloc(sizeof(fft_data) * BINS);

    plan_rc = fftwf_plan_r2r_1d(BINS, real, comp, FFTW_R2HC, FFTW_MEASURE);
    plan_cr = fftwf_plan_r2r_1d(BINS, comp_tmp, real, FFTW_HC2R, FFTW_MEASURE);


	/*	Use Kaiser-Bessel window for best results - Code taken from mplayer */
		kbd_window_init(5.0, &window, BINS, 50);
		
    /* Calculate window*/
//   for (i = 0; i < BINS; i++) {

//     window[i] = -0.5f * cosf(2.0f * M_PI * (float) i / (float) BINS) + 0.5f; 
/* root raised cosine window - aparently sounds worse ...
	window[i] = sqrtf(0.5f + -0.5 * cosf(2.0f * M_PI * (float) i /
			  (float) BINS));
*/
//   }

    plugin_init();
    comp_plugin = plugin_load("sc4_1882.so");
    if (comp_plugin == NULL)  {
           fprintf(stderr, "Required plugin sc4_1882.so missing.\n");
           fprintf(stderr, "Please load the SWH plugins.\n");
           exit(1);
    }


    /*  Decide which limiter to use.  Steve Harris' fast_lookahead_limiter or
        Sampo Savolainen's foo_limiter.  */

    lim_plugin[FAST] = plugin_load("fast_lookahead_limiter_1913.so");
    lim_plugin[FOO] = plugin_load("foo_limiter.so");

    if (lim_plugin[limiter_plugin] == NULL) {
      limiter_plugin ^= 1;

      if (lim_plugin[limiter_plugin] == NULL) {
          fprintf(stderr, "Required plugin fast_lookahead_limiter_1913.so and/or foo_limiter.so missing.\n");
          fprintf(stderr, "Please load the SWH plugins and/or Sampo Savolainen's foo_limiter plugin.\n");
          exit(1);
        }
    }


    /* This compressor is specifically stereo, so there are always two
     * channels. */
    for (band = 0; band < XO_NBANDS; band++) {
	out_tmp[CHANNEL_L][band] = calloc(dsp_block_size, sizeof(float));
	out_tmp[CHANNEL_R][band] = calloc(dsp_block_size, sizeof(float));
	compressors[band].handle = plugin_instantiate(comp_plugin, fs);
	comp_connect(comp_plugin, &compressors[band],
		     out_tmp[CHANNEL_L][band], out_tmp[CHANNEL_R][band]);
        delay_buf[CHANNEL_L][band] = calloc(dsp_block_size * 4, sizeof(float));
        delay_buf[CHANNEL_R][band] = calloc(dsp_block_size * 4, sizeof(float));
    }


    /*  Multiply by four so we'll have enough to cover 2 ms at 192KHz.  */

    delay_mask = (dsp_block_size * 4) - 1;


    if (lim_plugin[FAST] != NULL)
      {
        limiter[FAST].handle = plugin_instantiate(lim_plugin[FAST], fs);
        lim_connect(lim_plugin[FAST], &limiter[FAST], NULL, NULL);
      }

    if (lim_plugin[FOO] != NULL)
      {
        limiter[FOO].handle = plugin_instantiate(lim_plugin[FOO], fs);
        lim_connect(lim_plugin[FOO], &limiter[FOO], NULL, NULL);
      }



    /* Allocate at least 1 second of latency correction buffer */
    for (latcorbuf_len = 256; latcorbuf_len < fs * 1.0f; latcorbuf_len *= 2);
    latcorbuf_pos = 0;
    for (i=0; i < BCHANNELS; i++) {
	latcorbuf[i] = calloc(latcorbuf_len, sizeof(float));
	latcorbuf_postcomp[i] = calloc(latcorbuf_len, sizeof(float));
    }

    /* Clear the crossover filters state */
    memset(xo_filt, 0, sizeof(xo_filt));
}


/**
 * Generate a Kaiser Window.
 */
static void k_window_init(int alpha, float *window, int n, int iter)
{
    int j, k;
    float a, x;
    a = alpha * M_PI / n;
    a = a*a;
    for(k=0; k<n; k++) {
        x = k * (n - k) * a;
        window[k] = 1.0;
        for(j=iter; j>0; j--) {
            window[k] = (window[k] * x / (j*j)) + 1.0;
        }
    }
}

/**
 * Generate a Kaiser-Bessel Derived Window.
 * @param alpha  determines window shape
 * @param window array to fill with window values
 * @param n      length of the window
 * @param iter   number of iterations to use in BesselI0
 */
static void
kbd_window_init(int alpha, float *window, int n, int iter)
{
    int k, n2;
    float *kwindow;

    n2 = n >> 1;
    kwindow = &window[n2];
    k_window_init(alpha, kwindow, n2, iter);
    window[0] = kwindow[0];
    for(k=1; k<n2; k++) {
        window[k] = window[k-1] + kwindow[k];
    }
    for(k=0; k<n2; k++) {
        window[k] = sqrt(window[k] / (window[n2-1]+1));
        window[n-1-k] = window[k];
    }
}

void run_eq(unsigned int port, unsigned int in_ptr)
{
    const float fix = 2.5f / ((float) BINS * (float) OVER_SAMP);
    float peak;
    unsigned int i, j;
    int targ_bin;
    float *peak_data;

    for (i = 0; i < BINS; i++) {
	real[i] = window[i] * in_buf[port][(in_ptr + i) & BUF_MASK];
    }

    fftwf_execute(plan_rc);

    /* run the EQ + spectrum an. + xover process */

    if (spectrum_mode == SPEC_PRE_EQ) {
	peak_data = comp;
    } else {
	peak_data = comp_tmp;
    }

    memset(comp_tmp, 0, BINS * sizeof(fft_data));
    targ_bin = xover_fa / sample_rate * ((float)BINS + 0.5f);

    comp_tmp[0] = comp[0] * eq_coefs[0];
    if (comp_tmp[0] > bin_peak[0]) bin_peak[0] = comp_tmp[0];
    
    for (i = 1; i < targ_bin && i < BINS / 2 - 1; i++) {
	const float eq_gain = xo_band_action[XO_LOW] == MUTE ? 0.0f :
				(eq_bypass ? 1.0f : eq_coefs[i]);

	comp_tmp[i] = comp[i] * eq_gain;
	comp_tmp[BINS - i] = comp[BINS - i] * eq_gain;

	peak = sqrtf(peak_data[i] * peak_data[i] + peak_data[BINS - i] *
		peak_data[BINS - i]);
	if (peak > bin_peak[i]) {
	    bin_peak[i] = peak;
	}
    }
    fftwf_execute(plan_cr);
    for (j = 0; j < BINS; j++) {
	out_buf[port][XO_LOW][(in_ptr + j) & BUF_MASK] += real[j] * fix *
	    window[j];
    }

    memset(comp_tmp, 0, BINS * sizeof(fft_data));
    targ_bin = xover_fb / sample_rate * ((float)BINS + 0.5f);


    /*  Note that i falls through from the above loop.  */

    for (; i < targ_bin && i < BINS / 2 - 1; i++) {
	const float eq_gain = xo_band_action[XO_MID] == MUTE ? 0.0f :
				(eq_bypass ? 1.0f : eq_coefs[i]);

	comp_tmp[i] = comp[i] * eq_gain;
	comp_tmp[BINS - i] = comp[BINS - i] * eq_gain;
	peak = sqrtf(peak_data[i] * peak_data[i] + peak_data[BINS - i] *
		peak_data[BINS - i]);
	if (peak > bin_peak[i]) {
	    bin_peak[i] = peak;
	}
    }
    fftwf_execute(plan_cr);
    for (j = 0; j < BINS; j++) {
	out_buf[port][XO_MID][(in_ptr + j) & BUF_MASK] += real[j] * fix *
	    window[j];
    }

    memset(comp_tmp, 0, BINS * sizeof(fft_data));


    /*  Again, note that i falls through from the above loop.  */

    for (; i < BINS / 2 - 1; i++) {
	const float eq_gain = xo_band_action[XO_HIGH] == MUTE ? 0.0f :
				(eq_bypass ? 1.0f : eq_coefs[i]);

	comp_tmp[i] = comp[i] * eq_gain;
	comp_tmp[BINS - i] = comp[BINS - i] * eq_gain;
	peak = sqrtf(peak_data[i] * peak_data[i] + peak_data[BINS - i] *
		peak_data[BINS - i]);
	if (peak > bin_peak[i]) {
	    bin_peak[i] = peak;
	}
    }
    fftwf_execute(plan_cr);
    for (j = 0; j < BINS; j++) {
	out_buf[port][XO_HIGH][(in_ptr + j) & BUF_MASK] += real[j] * fix *
	    window[j];
    }
}

/* this is like run_eq except that it only uses a FFT to do the EQ, 
   the crossover is handled by IIR filters */

void run_eq_iir(unsigned int port, unsigned int in_ptr)
{
    const float fix = 2.5f / ((float) BINS * (float) OVER_SAMP);
    float peak;
    unsigned int i, j;
    int targ_bin;
    float *peak_data;

    for (i = 0; i < BINS; i++) {
	real[i] = window[i] * in_buf[port][(in_ptr + i) & BUF_MASK];
    }

    fftwf_execute(plan_rc);

    /* run the EQ + spectrum an. + xover process */

    if (spectrum_mode == SPEC_PRE_EQ) {
	peak_data = comp;
    } else {
	peak_data = comp_tmp;
    }

    memset(comp_tmp, 0, BINS * sizeof(fft_data));
    targ_bin = xover_fa / sample_rate * ((float)BINS + 0.5f);

    comp_tmp[0] = comp[0] * eq_coefs[0];
    if (comp_tmp[0] > bin_peak[0]) bin_peak[0] = comp_tmp[0];
    
    for (i = 1; i < BINS / 2 - 1; i++) {
	const float eq_gain = xo_band_action[XO_LOW] == MUTE ? 0.0f :
				(eq_bypass ? 1.0f : eq_coefs[i]);

	comp_tmp[i] = comp[i] * eq_gain;
	comp_tmp[BINS - i] = comp[BINS - i] * eq_gain;

	peak = sqrtf(peak_data[i] * peak_data[i] + peak_data[BINS - i] *
		peak_data[BINS - i]);
	if (peak > bin_peak[i]) {
	    bin_peak[i] = peak;
	}
    }
    fftwf_execute(plan_cr);
    for (j = 0; j < BINS; j++) {
	mid_buf[port][(in_ptr + j) & BUF_MASK] += real[j] * fix * window[j];
    }
}

#define EPSILON 0.0000001f		/* small positive number */
float bin_peak_read_and_clear(int bin)
{
    float ret = bin_peak[bin];
    const float fix = 2.0f / ((float) BINS * (float) OVER_SAMP);

    bin_peak[bin] = EPSILON;		/* don't take log(0.0) */

    return ret * fix;
}

int process_signal(jack_nframes_t nframes,
		   int nchannels,
		   int bchannels,
		   jack_default_audio_sample_t *in[],
		   jack_default_audio_sample_t *out[])
{
    unsigned int pos, port, band;
    const unsigned int latency = BINS - dsp_block_size;
    static unsigned int in_ptr = 0, dpos[8] = {0,0,0,0,0,0,0,0};
    static unsigned int n_calc_pt = BINS - (BINS / OVER_SAMP);


    /* The limiters i/o ports potentially change with every call */
    plugin_connect_port(lim_plugin[limiter_plugin], limiter[limiter_plugin].handle, LIM_IN_1, out[CHANNEL_L]);
    plugin_connect_port(lim_plugin[limiter_plugin], limiter[limiter_plugin].handle, LIM_IN_2, out[CHANNEL_R]);
    plugin_connect_port(lim_plugin[limiter_plugin], limiter[limiter_plugin].handle, LIM_OUT_1, out[CHANNEL_L]);
    plugin_connect_port(lim_plugin[limiter_plugin], limiter[limiter_plugin].handle, LIM_OUT_2, out[CHANNEL_R]);

    /* Crossfade parameter values from current to target */
    s_crossfade(nframes);

    if (iir_xover) {
	for (port = 0; port < nchannels; port++) {
#ifdef FILTER_TUNING
	    lp_set_params(&xo_filt[port][0], xover_fa * ft_bias_a_val,
			   ft_rez_lp_a_val, sample_rate);
	    hp_set_params(&xo_filt[port][1], xover_fa * ft_bias_a_hp_val,
			   ft_rez_lp_a_val, sample_rate);
	    lp_set_params(&xo_filt[port][2], xover_fb * ft_bias_b_val,
			   ft_rez_lp_b_val, sample_rate);
	    hp_set_params(&xo_filt[port][3], xover_fb * ft_bias_b_hp_val,
			   ft_rez_lp_b_val, sample_rate);
#else
	    const double bw_a = 1.0/((60.0*(xover_fa/sample_rate))+0.5);
	    const double bw_b = 1.0/((60.0*(xover_fb/sample_rate))+0.5);

	    lp_set_params(&xo_filt[port][0], xover_fa, bw_a, sample_rate);
	    hp_set_params(&xo_filt[port][1], xover_fa, bw_a, sample_rate);
	    lp_set_params(&xo_filt[port][2], xover_fb, bw_b, sample_rate);
	    hp_set_params(&xo_filt[port][3], xover_fb, bw_b, sample_rate);
#endif
	}
    }

    for (pos = 0; pos < nframes; pos++) {
	const unsigned int op = (in_ptr - (global_bypass ? latency : 0)) & BUF_MASK;
	float amp;

	for (port = 0; port < nchannels; port++) {
	    in_buf[port][in_ptr] = in[port][pos] * in_gain[port];
	    denormal_kill(&in_buf[port][in_ptr]);
	    if (in_buf[port][in_ptr] > 100.0f) {
		in_buf[port][in_ptr] = 100.0f;
	    } else if (in_buf[port][in_ptr] < -100.0f) {
		in_buf[port][in_ptr] = -100.0f;
	    }
#if 0
	    if (IS_DENORMAL(in_buf[port][in_ptr])) {
//printf("denormal");
		in_buf[port][in_ptr] = 0.0f;
	    }
	    if (!finite(in_buf[port][in_ptr])) {
printf("WARNING: wierd input: %f\n", in_buf[port][in_ptr]);
		if (isnan(in_buf[port][in_ptr])) {
		    in_buf[port][in_ptr] = 0.0f;
		} else if (in_buf[port][in_ptr] > 0.0f) {
		    in_buf[port][in_ptr] = 1.0f;
		} else {
		    in_buf[port][in_ptr] = -1.0f;
		}
	    }
#endif
	    amp = fabs(in_buf[port][in_ptr]);
	    if (amp > in_peak[port]) {
		in_peak[port] = amp;
	    }

	    if (iir_xover) {
		const float x = mid_buf[port][op];
		const float a = biquad_run(&xo_filt[port][0], x);
		const float y = biquad_run(&xo_filt[port][1], x);
		const float b = biquad_run(&xo_filt[port][2], y);
		const float c = biquad_run(&xo_filt[port][3], y);

		out_tmp[port][XO_LOW][pos] = a;
		out_tmp[port][XO_MID][pos] = b;
		out_tmp[port][XO_HIGH][pos] = c;
		mid_buf[port][op] = 0.0f;
	    } else {
		out_tmp[port][XO_LOW][pos] = out_buf[port][XO_LOW][op];
		out_buf[port][XO_LOW][op] = 0.0f;
		out_tmp[port][XO_MID][pos] = out_buf[port][XO_MID][op];
		out_buf[port][XO_MID][op] = 0.0f;
		out_tmp[port][XO_HIGH][pos] = out_buf[port][XO_HIGH][op];
		out_buf[port][XO_HIGH][op] = 0.0f;
	    }
	}

	in_ptr = (in_ptr + 1) & BUF_MASK;

	if (in_ptr == n_calc_pt) {	/* time to do the FFT? */
	    if (!global_bypass) {
		/* Just so the bypass can't kick in in the middle of
		 * precessing, might do something wierd */
		eq_bypass = eq_bypass_pending;
		limiter_bypass = limiter_bypass_pending;

		if (iir_xover) {
		    run_eq_iir(CHANNEL_L, in_ptr);
		    run_eq_iir(CHANNEL_R, in_ptr);
		} else {
		    run_eq(CHANNEL_L, in_ptr);
		    run_eq(CHANNEL_R, in_ptr);
		}
	    }
	    /* Work out when we can run it again */
	    n_calc_pt = (in_ptr + dsp_block_size) & BUF_MASK;
	}
    }

    /* Handle solo and mute for the IIR crossover case */
    if (iir_xover) {
	for (port = 0; port < nchannels; port++) {
	    for (band = XO_LOW; band < XO_NBANDS; band++) {
		if (xo_band_action[band] == MUTE) {
		    for (pos = 0; pos < nframes; pos++) {
			out_tmp[port][band][pos] = 0.0f;
		    }
		}
	    }
	}
    }

    //printf("rolled fifo's...\n");

    for (band = XO_LOW; band < XO_NBANDS; band++) {
	if (xo_band_action[band] == ACTIVE) {
	    plugin_run(comp_plugin, compressors[band].handle, nframes);
	    run_width(band, out_tmp[CHANNEL_L][band],
			out_tmp[CHANNEL_R][band], nframes);
	}
    }

    //printf("run compressors...\n");

    for (port = 0; port < bchannels; port++) {
	for (pos = 0; pos < nframes; pos++) {

          /*  Original (no delay) code.
	    out[port][pos] =
		out_tmp[port][XO_LOW][pos] + out_tmp[port][XO_MID][pos] +
		out_tmp[port][XO_HIGH][pos];
          */

          
          
		/* original  2 channel output */

		/* copy out_tmp[] to delay_buf[] */
		/*
		  delay_buf[port][XO_LOW][dpos[port] & delay_mask] = out_tmp[port][XO_LOW][pos];
          delay_buf[port][XO_MID][dpos[port] & delay_mask] = out_tmp[port][XO_MID][pos];
          delay_buf[port][XO_HIGH][dpos[port] & delay_mask] = out_tmp[port][XO_HIGH][pos];
		 * 
		  out[port][pos] = delay_buf[port % 2][XO_LOW][(dpos[port % 2] - delay[XO_LOW]) & delay_mask] +
					delay_buf[port % 2][XO_MID][(dpos[port % 2] - delay[XO_MID]) & delay_mask] +
					delay_buf[port % 2][XO_HIGH][(dpos[port % 2] - delay[XO_HIGH]) & delay_mask];
		 * 
		 */		
		

		/* copy out_tmp[] to delay_buf[] */
		delay_buf[port % 2][XO_LOW][dpos[port % 2] & delay_mask] = out_tmp[port % 2][XO_LOW][pos];
		delay_buf[port % 2][XO_MID][dpos[port % 2] & delay_mask] = out_tmp[port % 2][XO_MID][pos];
		delay_buf[port % 2][XO_HIGH][dpos[port % 2] & delay_mask] = out_tmp[port % 2][XO_HIGH][pos];
		
		/* multi channel output */				
		switch (port){
			case 0:
			case 1:
					
				out[port][pos] = delay_buf[port][XO_LOW][(dpos[port] - delay[XO_LOW]) & delay_mask]
					 + delay_buf[port][XO_MID][(dpos[port] - delay[XO_MID]) & delay_mask]
					 + delay_buf[port][XO_HIGH][(dpos[port] - delay[XO_HIGH]) & delay_mask];
			break;	
			case 2:
			case 3:
				out[port][pos] = delay_buf[port % 2][XO_LOW][(dpos[port % 2] - delay[XO_LOW]) & delay_mask];
			break;
			case 4:
			case 5:
				out[port][pos] = delay_buf[port % 2][XO_MID][(dpos[port % 2] - delay[XO_MID]) & delay_mask];
			break;
			case 6:
			case 7:
				out[port][pos] = delay_buf[port % 2][XO_HIGH][(dpos[port % 2] - delay[XO_HIGH]) & delay_mask];
			break;			
		}
	
		
		
          dpos[port]++;

			/* Keep buffer of compressor outputs incase we need it for
			* limiter bypass */
			latcorbuf_postcomp[port][(latcorbuf_pos + pos) & (latcorbuf_len - 1)] = out[port][pos];		
          
	}
    }

    //printf("done something...\n");

    for (pos = 0; pos < nframes; pos++) {
	for (port = 0; port < nchannels; port++) {
	    /* Apply input gain */
	    out[port][pos] *= limiter_gain;

	    /* Check for peaks */
	    if ( port < 2 ){
			if (out[port][pos] > lim_peak[LIM_PEAK_IN]) {
			lim_peak[LIM_PEAK_IN] = out[port][pos];
			}
		}
	}
    }

    for (port = 0; port < nchannels; port++) {
	const float a = ws_boost_a * 0.3;
	const float gain_corr = 1.0 / LERP(ws_boost_wet, 1.0,
				a > M_PI*0.5 ? 1.0 : sinf(1.0 * a));
	for (pos = 0; pos < nframes; pos++) {
	    const float x = out[port][pos] * out_gain;
	    out[port][pos] = LERP(ws_boost_wet, x, sinf(x * a)) * gain_corr;
	}
    }

    plugin_run(lim_plugin[limiter_plugin], limiter[limiter_plugin].handle, nframes);

    /* Keep a buffer of old input data, in case we need it for bypass */
    for (port = 0; port < bchannels; port++) {
	for (pos = 0; pos < nframes; pos++) {
	    latcorbuf[port][(latcorbuf_pos + pos) & (latcorbuf_len - 1)] =
		in[port % 2][pos];
	//	g_print("port %i - %i\n", port, port % 2);
	}
    }

    /* If bypass is on, override all the stuff done by the crossover section,
     * limiter, and so on */
    if (limiter_bypass) {
	const unsigned int limiter_latency = (unsigned int)limiter[limiter_plugin].latency;

	for (port = 0; port < bchannels; port++) {
	    for (pos = 0; pos < nframes; pos++) {
		out[port][pos] = latcorbuf_postcomp[port][(latcorbuf_pos +
			pos - limiter_latency - nframes) & (latcorbuf_len - 1)];
	    }
	}
    }
    if (global_bypass) {
	const unsigned int limiter_latency = (unsigned int)limiter[limiter_plugin].latency;

	for (port = 0; port < bchannels; port++) {
	    for (pos = 0; pos < nframes; pos++) {
		out[port][pos] = latcorbuf[port][(latcorbuf_pos +
			pos - limiter_latency - nframes) & (latcorbuf_len - 1)];
	    }
	}
    }
    latcorbuf_pos += nframes;

    for (pos = 0; pos < nframes; pos++) {
	for (port = 0; port < nchannels; port++) {
	    const float oa = fabs(out[port][pos]);

	    if (oa > lim_peak[LIM_PEAK_OUT]) {
		lim_peak[LIM_PEAK_OUT] = oa;
	    }
	    if (oa > out_peak[port]) {
		out_peak[port] = oa;
	    }
	}
    }


    /*  Don't try to play with the RMS meters until we've initialized the buffers.  */

    if (rms_ready)
      {
        for (port = 0 ; port < nchannels ; port++)
          {
            rms_peak[port] = rms_run_buffer (r[port], out[port], nframes);
          }
      }


    /* We've got to the end of the processing, so update the actions */

    for (band = 0; band < XO_NBANDS; band++) {
	xo_band_action[band] = xo_band_action_pending[band];
    }


    /*  As above, update the limiter.  */

    if (limiter_plugin_change_pending)
      {
        limiter_plugin = limiter_plugin_pending;

        limiter_plugin_change_pending = FALSE;
      }


    if (logscale_pending >= 0.0) 
      {
        limiter[limiter_plugin].logscale = logscale_pending;
        logscale_pending = -1.0;
      }


    /*  Always set these since we turn off delay by setting to 0.  */

    delay[XO_LOW] = delay_pending[XO_LOW];
    delay[XO_MID] = delay_pending[XO_MID];

    return 0;
}

float eval_comp(float thresh, float ratio, float knee, float in)
{
    /* Below knee */
    if (in <= thresh - knee) {
	return in;
    }

    /* In knee */
    if (in < thresh + knee) {
	const float x = -(thresh - knee - in) / knee;
	return in - knee * x * x * 0.25f * (ratio - 1.0f) / ratio;
    }

    /* Above knee */
    return in + (thresh - in) * (ratio - 1.0f) / ratio;
}

void process_set_spec_mode(int mode)
{
    spectrum_mode = mode;

    set_scene_warning_button ();
}

int process_get_spec_mode()
{
  return (spectrum_mode);
}

void process_set_limiter_plugin(int id)
{
  int pid = limiter_plugin;

  limiter_plugin_pending = id;


  if (lim_plugin[id] == NULL) return;


  /*  Copy the previous settings to the current plugin.  */

  limiter[id].ingain = limiter[pid].ingain;
  limiter[id].limit = limiter[pid].limit;
  limiter[id].release = limiter[pid].release;
  limiter[id].attenuation = limiter[pid].attenuation;
  limiter[id].latency = limiter[pid].latency;
  limiter[id].logscale = limiter[pid].logscale;


  limiter_plugin_change_pending = TRUE;


  /*  Turn the logscale on or off depending on whether we're using FAST or FOO.  */

  if (limiter_plugin_pending == FOO)
    {
      limiter_logscale_set_state (TRUE);
    }
  else
    {
      limiter_logscale_set_state (FALSE);
    }

  limiter_set_label (limiter_plugin_pending);
}

int process_get_limiter_plugin()
{
  /*  This is a startup fixer.  If we specified the plugin on the command line
      we want to return pending until these two are the same.  */

  if (limiter_plugin_pending != limiter_plugin)
    {
      return (limiter_plugin_pending);
    }
  else
    {
      return (limiter_plugin);
    }
}

void process_set_stereo_width(int xo_band, float width)
{
    assert(xo_band >= 0 && xo_band < XO_NBANDS);

    /* Scale width to be pi/4 - pi/2, the sqrt(2) factor saves us some cycles
     * later */
    sw_m_gain[xo_band] = cosf((width + 1.0f) * 0.78539815f) * 0.7071067811f;
    sw_s_gain[xo_band] = sinf((width + 1.0f) * 0.78539815f) * 0.7071067811f;
}


/*  This is a holdover from when we had per band balance.  */

void process_set_stereo_balance(int xo_band, float bias)
{
    assert(xo_band >= 0 && xo_band < XO_NBANDS);

    sb_l_gain[xo_band] = db2lin(bias * -0.5f);
    sb_r_gain[xo_band] = db2lin(bias * 0.5f);
}

void run_width(int xo_band, float *left, float *right, int nframes)
{
    unsigned int pos;

    for (pos = 0; pos < nframes; pos++) {
	const float mid = (left[pos] + right[pos]) * sw_m_gain[xo_band];
	const float side = (left[pos] - right[pos]) * sw_s_gain[xo_band];

	left[pos] = (mid + side) * sb_l_gain[xo_band];
	right[pos] = (mid - side) * sb_r_gain[xo_band];
    }
}

void process_set_ws_boost(float val)
{
    if (val < 1.0f) {
	ws_boost_wet = val;
	ws_boost_a = 1.0f;
    } else {
	ws_boost_wet = 1.0f;
	ws_boost_a = val;
    }
}

void process_set_xo_band_action(int band, int action)
{
    assert(action == ACTIVE || action == MUTE || action == BYPASS);

    xo_band_action_pending[band] = action;
}

void process_set_eq_bypass(int bypass)
{
    eq_bypass_pending = bypass;
}

void process_set_crossover_type(int type)
{
    iir_xover = type;
}

int process_get_crossover_type()
{
    return (iir_xover);
}

void process_set_limiter_bypass(int bypass)
{
    limiter_bypass_pending = bypass;
}

void process_set_low2mid_xover (float freq)
{
    xover_fa = freq;
}

void process_set_mid2high_xover (float freq)
{
    xover_fb = freq;
}

float process_get_low2mid_xover ()
{
    return (xover_fa);
}

float process_get_mid2high_xover ()
{
    return (xover_fb);
}

void process_get_bypass_states (int *eq, int *comp, int *limit, int *global)
{
  int i;

  *global = global_bypass;
  *eq = eq_bypass;

  for (i = 0 ; i < XO_NBANDS ; i++) comp[i] = xo_band_action[i];

  *limit = limiter_bypass;
  *global = global_bypass;
}

int process_get_bypass_state (int bypass_type)
{
  switch (bypass_type)
    {
    case EQ_BYPASS:
      return (eq_bypass);
      break;

    case LOW_COMP_BYPASS:
      return (xo_band_action[0]);
      break;

    case MID_COMP_BYPASS:
      return (xo_band_action[1]);
      break;

    case HIGH_COMP_BYPASS:
      return (xo_band_action[2]);
      break;

    case LIMITER_BYPASS:
      return (limiter_bypass);
      break;

    case GLOBAL_BYPASS:
      return (global_bypass);
      break;

    default:
      return (-1);
      break;
    }
}

float process_get_sample_rate ()
{
  return (sample_rate);
}

int process_get_rms_time_slice ()
{
  return (rms_time_slice);
}

void process_set_rms_time_slice (int milliseconds)
{
  rms_time_slice = milliseconds;


  if (r[0]) rms_free (r[0]);
  if (r[1]) rms_free (r[1]);

  float ts = (float) rms_time_slice / 1000.0;

  r[0] = rms_new (sample_rate, ts);
  r[1] = rms_new (sample_rate, ts);


  rms_ready = TRUE;
}

void process_set_global_bypass (int state)
{
  global_bypass = state;
}


int process_limiter_plugins_available ()
{
  if (lim_plugin[FAST] == NULL || lim_plugin[FOO] == NULL) return (1);
  return (2);
}


/*  This actually returns the number of samples for the delay but 
    it really doesn't matter.  Anyway, we might want to use that 
    sometime in the future.  */

int process_get_xo_delay_state (int band)
{
  return (delay[band]);
}

void process_set_xo_delay_state (int band, int state)
{
  if (state)
    {
      delay_pending[band] = NINT ((sample_rate / 1000.0) * save_delay[band]);
    }
  else
    {
      delay_pending[band] = 0;
    }
}

float process_get_xo_delay_time (int band)
{
  return (save_delay[band]);
}

void process_set_xo_delay_time (int band, float ms)
{
  save_delay[band] = ms;
}

void process_set_limiter_logscale (float value)
{
  logscale_pending = value;

  set_scene_warning_button ();
}

/* vi:set ts=8 sts=4 sw=4: */
