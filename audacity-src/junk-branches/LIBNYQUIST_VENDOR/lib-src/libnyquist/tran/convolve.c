#include "stdio.h"
#ifndef mips
#include "stdlib.h"
#endif
#include "xlisp.h"
#include "sound.h"

#include "falloc.h"
#include "cext.h"
#include "convolve.h"

void convolve_free();


typedef struct convolve_susp_struct {
    snd_susp_node susp;
    long terminate_cnt;
    boolean logically_stopped;
    sound_type x_snd;
    long x_snd_cnt;
    sample_block_values_type x_snd_ptr;

    table_type table;
    sample_type *h_buf;
    double length_of_h;
    long h_len;
    long x_buf_len;
    sample_type *x_buffer_pointer;
    sample_type *x_buffer_current;
} convolve_susp_node, *convolve_susp_type;


void h_reverse(sample_type *h, long len)
{
    sample_type temp;
    int i;
    
    for (i = 0; i < len; i++) {
    temp = h[i];
    h[i] = h[len - 1];
    h[len - 1] = temp;
    len--;
    }
}


void convolve_s_fetch(register convolve_susp_type susp, snd_list_type snd_list)
{
    int cnt = 0; /* how many samples computed */
    int togo;
    int n;
    sample_block_type out;
    register sample_block_values_type out_ptr;

    register sample_block_values_type out_ptr_reg;

    register sample_type * h_buf_reg;
    register long h_len_reg;
    register long x_buf_len_reg;
    register sample_type * x_buffer_pointer_reg;
    register sample_type * x_buffer_current_reg;
    register sample_type x_snd_scale_reg = susp->x_snd->scale;
    register sample_block_values_type x_snd_ptr_reg;
    falloc_sample_block(out, "convolve_s_fetch");
    out_ptr = out->samples;
    snd_list->block = out;

    while (cnt < max_sample_block_len) { /* outer loop */
    /* first compute how many samples to generate in inner loop: */
    /* don't overflow the output sample block: */
    togo = max_sample_block_len - cnt;

    /* don't run past the x_snd input sample block: */
    susp_check_term_log_samples(x_snd, x_snd_ptr, x_snd_cnt);
    togo = min(togo, susp->x_snd_cnt);

    /* don't run past terminate time */
    if (susp->terminate_cnt != UNKNOWN &&
        susp->terminate_cnt <= susp->susp.current + cnt + togo) {
        togo = susp->terminate_cnt - (susp->susp.current + cnt);
        if (togo == 0) break;
    }


    /* don't run past logical stop time */
    if (!susp->logically_stopped && susp->susp.log_stop_cnt != UNKNOWN) {
        int to_stop = susp->susp.log_stop_cnt - (susp->susp.current + cnt);
        /* break if to_stop == 0 (we're at the logical stop)
         * AND cnt > 0 (we're not at the beginning of the
         * output block).
         */
        if (to_stop < togo) {
        if (to_stop == 0) {
            if (cnt) {
            togo = 0;
            break;
            } else /* keep togo as is: since cnt == 0, we
                    * can set the logical stop flag on this
                    * output block
                    */
            susp->logically_stopped = true;
        } else /* limit togo so we can start a new
                * block at the LST
                */
            togo = to_stop;
        }
    }

    n = togo;
    h_buf_reg = susp->h_buf;
    h_len_reg = susp->h_len;
    x_buf_len_reg = susp->x_buf_len;
    x_buffer_pointer_reg = susp->x_buffer_pointer;
    x_buffer_current_reg = susp->x_buffer_current;
    x_snd_ptr_reg = susp->x_snd_ptr;
    out_ptr_reg = out_ptr;
    if (n) do { /* the inner sample computation loop */
long i; double sum;
    /* see if we've reached end of x_buffer */
    if ((x_buffer_pointer_reg + x_buf_len_reg) <= (x_buffer_current_reg + h_len_reg)) {
        /* shift x_buffer from current back to base */
        for (i = 1; i < h_len_reg; i++) {
        x_buffer_pointer_reg[i-1] = x_buffer_current_reg[i];
        }    
        /* this will be incremented back to x_buffer_pointer_reg below */
        x_buffer_current_reg = x_buffer_pointer_reg - 1;
    }

    x_buffer_current_reg++;

    x_buffer_current_reg[h_len_reg - 1] = (x_snd_scale_reg * *x_snd_ptr_reg++);

    sum = 0.0;
    for (i = 0; i < h_len_reg; i++) {
        sum += x_buffer_current_reg[i] * h_buf_reg[i];
    }

    *out_ptr_reg++ = (sample_type) sum;
;
    } while (--n); /* inner loop */

    susp->x_buffer_pointer = x_buffer_pointer_reg;
    susp->x_buffer_current = x_buffer_current_reg;
    /* using x_snd_ptr_reg is a bad idea on RS/6000: */
    susp->x_snd_ptr += togo;
    out_ptr += togo;
    susp_took(x_snd_cnt, togo);
    cnt += togo;
    } /* outer loop */

    /* test for termination */
    if (togo == 0 && cnt == 0) {
    snd_list_terminate(snd_list);
    } else {
    snd_list->block_len = cnt;
    susp->susp.current += cnt;
    }
    /* test for logical stop */
    if (susp->logically_stopped) {
    snd_list->logically_stopped = true;
    } else if (susp->susp.log_stop_cnt == susp->susp.current) {
    susp->logically_stopped = true;
    }
} /* convolve_s_fetch */


void convolve_toss_fetch(susp, snd_list)
  register convolve_susp_type susp;
  snd_list_type snd_list;
{
    long final_count = susp->susp.toss_cnt;
    time_type final_time = susp->susp.t0;
    long n;

    /* fetch samples from x_snd up to final_time for this block of zeros */
    while ((round((final_time - susp->x_snd->t0) * susp->x_snd->sr)) >=
       susp->x_snd->current)
    susp_get_samples(x_snd, x_snd_ptr, x_snd_cnt);
    /* convert to normal processing when we hit final_count */
    /* we want each signal positioned at final_time */
    n = round((final_time - susp->x_snd->t0) * susp->x_snd->sr -
         (susp->x_snd->current - susp->x_snd_cnt));
    susp->x_snd_ptr += n;
    susp_took(x_snd_cnt, n);
    susp->susp.fetch = susp->susp.keep_fetch;
    (*(susp->susp.fetch))(susp, snd_list);
}


void convolve_mark(convolve_susp_type susp)
{
    sound_xlmark(susp->x_snd);
}


void convolve_free(convolve_susp_type susp)
{
    table_unref(susp->table); 
    free(susp->x_buffer_pointer);    sound_unref(susp->x_snd);
    ffree_generic(susp, sizeof(convolve_susp_node), "convolve_free");
}


void convolve_print_tree(convolve_susp_type susp, int n)
{
    indent(n);
    stdputstr("x_snd:");
    sound_print_tree_1(susp->x_snd, n);
}


sound_type snd_make_convolve(sound_type x_snd, sound_type h_snd)
{
    register convolve_susp_type susp;
    rate_type sr = x_snd->sr;
    time_type t0 = 0.0;
    int interp_desc = 0;
    sample_type scale_factor = 1.0F;
    time_type t0_min = t0;
    falloc_generic(susp, convolve_susp_node, "snd_make_convolve");
    susp->table = sound_to_table(h_snd);
    susp->h_buf = susp->table->samples;
    susp->length_of_h = susp->table->length;
    susp->h_len = (long) susp->length_of_h;
     h_reverse(susp->h_buf, susp->h_len);
    susp->x_buf_len = 2 * susp->h_len;
    susp->x_buffer_pointer = calloc((2 * (susp->h_len)), sizeof(float));
    susp->x_buffer_current = susp->x_buffer_pointer;
    susp->susp.fetch = convolve_s_fetch;
    susp->terminate_cnt = UNKNOWN;
    /* handle unequal start times, if any */
    if (t0 < x_snd->t0) sound_prepend_zeros(x_snd, t0);
    /* minimum start time over all inputs: */
    t0_min = min(x_snd->t0, t0);
    /* how many samples to toss before t0: */
    susp->susp.toss_cnt = (long) ((t0 - t0_min) * sr + 0.5);
    if (susp->susp.toss_cnt > 0) {
    susp->susp.keep_fetch = susp->susp.fetch;
    susp->susp.fetch = convolve_toss_fetch;
    }

    /* initialize susp state */
    susp->susp.free = convolve_free;
    susp->susp.sr = sr;
    susp->susp.t0 = t0;
    susp->susp.mark = convolve_mark;
    susp->susp.print_tree = convolve_print_tree;
    susp->susp.name = "convolve";
    susp->logically_stopped = false;
    susp->susp.log_stop_cnt = logical_stop_cnt_cvt(x_snd);
    susp->susp.current = 0;
    susp->x_snd = x_snd;
    susp->x_snd_cnt = 0;
    return sound_create((snd_susp_type)susp, t0, sr, scale_factor);
}


sound_type snd_convolve(sound_type x_snd, sound_type h_snd)
{
    sound_type x_snd_copy = sound_copy(x_snd);
    return snd_make_convolve(x_snd_copy, h_snd);
}
