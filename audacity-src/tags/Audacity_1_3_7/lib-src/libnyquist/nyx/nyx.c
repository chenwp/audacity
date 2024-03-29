/**********************************************************************

  nyx.c

  Nyx: A very simple external interface to Nyquist

  Dominic Mazzoni

**********************************************************************/

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#ifndef WIN32
#include <unistd.h>
#endif

/* nyx includes */
#include "nyx.h"

/* xlisp includes */
#include "switches.h"
#include "xlisp.h"
#include "term.h"
#include "cext.h"

/* nyquist includes */
#include "sound.h"
#include "falloc.h"


/* nyquist externs */
extern LVAL a_sound;
extern snd_list_type zero_snd_list;

/* globals */
int                 nyx_output_pos;
int                 nyx_output_len;
char               *nyx_output_string;
int                 nyx_expr_pos;
int                 nyx_expr_len;
const char         *nyx_expr_string;
LVAL                nyx_result;
XLCONTEXT           nyx_cntxt;
int                 nyx_parse_error_flag;
int                 nyx_first_time = 1;
LVAL                nyx_old_obarray;

typedef struct nyx_susp_struct {
   snd_susp_node       susp;
   nyx_audio_callback  callback;
   void               *userdata;
   long                len;
   int                 channel;
} nyx_susp_node, *nyx_susp_type;

LVAL copylist(LVAL from)
{
   if (from == NULL)
      return NULL;

   return cons(car(from), copylist(cdr(from)));
}

/* Make a copy of the obarray so that we can erase any
   changes the user makes to global variables */
void nyx_save_obarray()
{
   LVAL array, obarrayvec;
   int i;

   xlsave1(array);
   array = newvector(HSIZE);

   obarrayvec = getvalue(obarray);
   for(i=0; i<HSIZE; i++) {
      LVAL from = getelement(obarrayvec, i);
      if (from)
         setelement(array, i, copylist(from));
   }

   nyx_old_obarray = obarray;
   obarray = xlmakesym("*OBARRAY*");
   setvalue(obarray, array);
   xlpop();
}

void nyx_init()
{
   if (nyx_first_time) {
      char *argv[1];
      argv[0] = "nyquist";
      xlisp_main_init(1, argv);

      nyx_output_len = 0;
      nyx_output_pos = 0;
      nyx_output_string = NULL;
      
      nyx_first_time = 0;
   }

   /* keep nyx_result from being garbage-collected */
   xlprot1(nyx_result);

   nyx_save_obarray();
}

void nyx_cleanup()
{
   if (nyx_output_string) {
      free(nyx_output_string);
      nyx_output_string = NULL;
      nyx_output_pos = 0;
      nyx_output_len = 0;
   }

   xlpop(); /* garbage-collect nyx_result */
   gc(); /* run the garbage-collector now */
}

void nyx_susp_fetch(register nyx_susp_type susp, snd_list_type snd_list)
{
   sample_block_type         out;
   sample_block_values_type  out_ptr;
   long                      n;
   int                       err;

   falloc_sample_block(out, "nyx_susp_fetch");
   out_ptr = out->samples;
   snd_list->block = out;

   n = max_sample_block_len;
   if (susp->susp.current + n > susp->len)
      n = susp->len - susp->susp.current;

   err = susp->callback(out_ptr, susp->channel,
                            susp->susp.current, n, 0, susp->userdata);
   if (err)
      longjmp(nyx_cntxt.c_jmpbuf, 1);      

   snd_list->block_len = n;
   susp->susp.current += n;

   if (n == 0) {
      /* we didn't read anything, but can't return length zero, so
         convert snd_list to pointer to zero block */
      snd_list_terminate(snd_list);
   }
   else if (n < max_sample_block_len) {
      /* should free susp */
      snd_list_unref(snd_list->u.next);
      /* if something is in buffer, terminate by pointing to zero block */
      snd_list->u.next = zero_snd_list;
   }
}

void nyx_susp_free(nyx_susp_type susp)
{
   ffree_generic(susp, sizeof(nyx_susp_node), "nyx_susp_free");
}

void nyx_susp_print_tree(nyx_susp_type susp, int n)
{
}

void nyx_capture_output(int max_len)
{
   if (nyx_output_string)
      free(nyx_output_string);

   nyx_output_string = (char *)malloc(max_len);
   nyx_output_len = max_len;
   nyx_output_pos = 0;
}

void nyx_get_captured_output(int *out_len,
                             const char **out_chars)
{
   *out_len = nyx_output_pos;
   *out_chars = (const char *)nyx_output_string;
   nyx_output_pos = 0;
}

void nyx_set_audio_params( double rate )
{
   /* Bind the sample rate to the "*sound-srate*" global */
   setvalue(xlenter("*SOUND-SRATE*"), cvflonum(rate));
}

void nyx_set_input_audio(nyx_audio_callback callback,
                         void *userdata,
                         int num_channels,
                         long len, double rate)
{
   sample_type      scale_factor = 1.0;
   time_type        t0 = 0.0;
   nyx_susp_type   *susp;
   sound_type      *snd;
   double           stretch_len;
   LVAL             warp;
   int              ch;

   susp = (nyx_susp_type *)malloc(num_channels * sizeof(nyx_susp_type));
   snd = (sound_type *)malloc(num_channels * sizeof(sound_type));

   for(ch=0; ch < num_channels; ch++) {
      falloc_generic(susp[ch], nyx_susp_node, "nyx_set_input_audio");

      susp[ch]->callback = callback;
      susp[ch]->userdata = userdata;
      susp[ch]->len = len;
      susp[ch]->channel = ch;

      susp[ch]->susp.sr = rate;
      susp[ch]->susp.t0 = t0;
      susp[ch]->susp.mark = NULL;
      susp[ch]->susp.print_tree = nyx_susp_print_tree;
      susp[ch]->susp.current = 0;
      susp[ch]->susp.fetch = nyx_susp_fetch;
      susp[ch]->susp.free = nyx_susp_free;
      susp[ch]->susp.name = "nyx";
      
      snd[ch] = sound_create((snd_susp_type)susp[ch], t0, 
                             rate, 
                             scale_factor);
   }

   /* Bind the sample rate to the "*sound-srate*" global */
   setvalue(xlenter("*SOUND-SRATE*"), cvflonum(rate));

   /* Bind selection len to "len" global */
   setvalue(xlenter("LEN"), cvflonum(len));

   if (len > 0)
      stretch_len = len / rate;
   else
      stretch_len = 1.0;

   /* Set the "*warp*" global based on the length of the audio */
   xlprot1(warp);
   warp = cons(cvflonum(0),                    /* time offset */
               cons(cvflonum(stretch_len),     /* time stretch */
                    cons(NULL,                 /* cont. time warp */
                         NULL)));
   setvalue(xlenter("*WARP*"), warp);
   xlpop();

   if (num_channels > 1) {
      LVAL array = newvector(num_channels);
      for(ch=0; ch<num_channels; ch++)
         setelement(array, ch, cvsound(snd[ch]));

      setvalue(xlenter("S"), array);
   }
   else {
      LVAL s = cvsound(snd[0]);

      setvalue(xlenter("S"), s);
   }
}

int is_labels(LVAL expr)
{
   /* make sure that we have a list whose first element is a
      list of the form (time "label") */

   LVAL label;
   LVAL first;
   LVAL second;
   LVAL third;

   if (!consp(expr))
      return 0;

   label = car(expr);

   if (!consp(label))
      return 0;

   first = car(label);
   if (!(floatp(first) || fixp(first)))
      return 0;

   if (!consp(cdr(label)))
      return 0;

   second = car(cdr(label));

   if (floatp(second) || fixp(second)) {
      if (!consp(cdr(cdr(label))))
         return 0;
      third = car(cdr(cdr(label)));
      if (!(stringp(third)))
         return 0;
   }
   else
      if (!(stringp(second)))
         return 0;

   /* If this is the end of the list, we're done */

   if (cdr(expr) == NULL)
      return 1;

   /* Otherwise recurse */

   return is_labels(cdr(expr));
}

nyx_rval nyx_get_type(LVAL expr)
{
   if (expr==NULL)
      return nyx_error;

   switch(ntype(expr)) {
   case FIXNUM:
      return nyx_int;
   case FLONUM:
      return nyx_double;
   case STRING:
      return nyx_string;
   case VECTOR: {
      /* make sure it's a vector of sounds */
      int i;
      for(i=0; i<getsize(expr); i++)
         if (!soundp(getelement(expr, i)))
             return nyx_error;
      return nyx_audio;
   }
   case CONS: {
      /* see if it's a list of time/string pairs representing a
         label track */
      if (is_labels(expr))
         return nyx_labels;
      else
         return nyx_error;
   }
   case EXTERN: {
      if (soundp(expr))
         return nyx_audio;
   }
   } /* switch */

   return nyx_error;
}

nyx_rval nyx_eval_expression(const char *expr_string)
{
   LVAL expr = NULL;

   nyx_expr_string = expr_string;
   nyx_expr_len = strlen(nyx_expr_string);
   nyx_expr_pos = 0;

   nyx_result = NULL;
   nyx_parse_error_flag = 0;

   xlprot1(expr);

   /* setup the error return */
   xlbegin(&nyx_cntxt,CF_TOPLEVEL|CF_CLEANUP|CF_BRKLEVEL,(LVAL)1);
   if (setjmp(nyx_cntxt.c_jmpbuf))
      goto finish;

   while(nyx_expr_pos < nyx_expr_len) {
      expr = NULL;

      /* read an expression */
      if (!xlread(getvalue(s_stdin), &expr, FALSE))
         break;

      #if 0
      /* save the input expression (so the user can refer to it
         as +, ++, or +++) */
      xlrdsave(expr);
      #endif
      
      /* evaluate the expression */
      nyx_result = xleval(expr);
   }

   xlflush();

 finish:
   xlend(&nyx_cntxt);

   xlpop(); /* unprotect expr */

   /* reset the globals to their initial state */
   obarray = nyx_old_obarray;
   setvalue(xlenter("S"), NULL);
   gc();

   return nyx_get_type(nyx_result);
}

int nyx_get_audio_num_channels()
{
   if (nyx_get_type(nyx_result) != nyx_audio)
      return 0;

   if (vectorp(nyx_result))
      return getsize(nyx_result);
   else
      return 1;
}

int nyx_get_audio(nyx_audio_callback callback, void *userdata)
{
   sample_block_type block;
   sound_type snd;
   sound_type *snds;
   float *buffer;
   long bufferlen;
   long *totals;
   long *lens;
   long cnt;
   int result = 0;
   int num_channels;
   int ch, i;
   int success = FALSE;

   if (nyx_get_type(nyx_result) != nyx_audio)
      return;

   num_channels = nyx_get_audio_num_channels();
   snds = (sound_type *)malloc(num_channels * sizeof(sound_type));
   totals = (long *)malloc(num_channels * sizeof(long));
   lens = (long *)malloc(num_channels * sizeof(long));

   /* setup the error return */
   xlbegin(&nyx_cntxt,CF_TOPLEVEL|CF_CLEANUP|CF_BRKLEVEL,(LVAL)1);
   if (setjmp(nyx_cntxt.c_jmpbuf))
      goto finish;

   for(ch=0; ch<num_channels; ch++) {
      if (num_channels == 1)
         snd = getsound(nyx_result);
      else
         snd = getsound(getelement(nyx_result, ch));
      snds[ch] = snd;
      totals[ch] = 0;
      lens[ch] = snd_length(snd, snd->stop);
   }

   buffer = NULL;
   bufferlen = 0;

   while(result==0) {
      for(ch=0; ch<num_channels; ch++) {
         snd = snds[ch];
         cnt = 0;
         block = snd->get_next(snd, &cnt);
         if (block == zero_block || cnt == 0) {
            result = -1;
            break;
         }

         /* copy the data to a temporary buffer and scale it
            by the appropriate scale factor */

         if (cnt > bufferlen) {
            if (buffer)
               free(buffer);
            buffer = (float *)malloc(cnt * sizeof(float));
            bufferlen = cnt;
         }

         memcpy(buffer, block->samples, cnt * sizeof(float));

         for(i=0; i<cnt; i++)
            buffer[i] *= snd->scale;

         result = callback(buffer, ch,
                           totals[ch], cnt, lens[ch], userdata);

         if (result != 0) {
            goto finish;
         }

         totals[ch] += cnt;
      }
   }

   success = TRUE;

 finish:
   if (buffer)
      free(buffer);
   free(snds);
   free(totals);
   free(lens);

   xlend(&nyx_cntxt);

   return success;
}

int nyx_get_int()
{
   if (nyx_get_type(nyx_result) != nyx_int)
      return -1;

   return getfixnum(nyx_result);
}

double nyx_get_double()
{
   if (nyx_get_type(nyx_result) != nyx_double)
      return -1.0;

   return getflonum(nyx_result);
}

const char *nyx_get_string()
{
   if (nyx_get_type(nyx_result) != nyx_string)
      return NULL;

   return (const char *)getstring(nyx_result);

}

int nyx_get_num_labels()
{
   LVAL s = nyx_result;
   int count = 0;

   if (nyx_get_type(nyx_result) != nyx_labels)
      return 0;

   while(s) {
      count++;
      s = cdr(s);
   }

   return count;
}

void nyx_get_label(int index,
                   double *start_time,
                   double *end_time,
                   const char **label)
{
   LVAL s = nyx_result;
   LVAL label_expr;
   LVAL t0_expr;
   LVAL t1_expr;
   LVAL str_expr;

   if (nyx_get_type(nyx_result) != nyx_labels)
      return;

   if (index < 0 || index >= nyx_get_num_labels())
      return;

   while(index) {
      index--;
      s = cdr(s);
   }

   /* We either have (t0 "label") or (t0 t1 "label") */

   label_expr = car(s);
   t0_expr = car(label_expr);
   t1_expr = car(cdr(label_expr));
   if (stringp(t1_expr)) {
      str_expr = t1_expr;
      t1_expr = t0_expr;
   }
   else
      str_expr = car(cdr(cdr(label_expr)));

   if (floatp(t0_expr))
      *start_time = getflonum(t0_expr);
   else if (fixp(t0_expr))
      *start_time = (double)getfixnum(t0_expr);

   if (floatp(t1_expr))
      *end_time = getflonum(t1_expr);
   else if (fixp(t1_expr))
      *end_time = (double)getfixnum(t1_expr);

   *label = (const char *)getstring(str_expr);
}

const char *nyx_get_error_str()
{
   return NULL;
}

int ostgetc()
{
   if (nyx_expr_pos < nyx_expr_len) {
      fflush(stdout);
      return (nyx_expr_string[nyx_expr_pos++]);
   }
   else if (nyx_expr_pos == nyx_expr_len) {
      /* Add whitespace at the end so that the parser
         knows that this is the end of the expression */
      nyx_expr_pos++;
      return '\n';
   }
   else
      return EOF;
}

/* osinit - initialize */
void osinit(char *banner)
{
}

/* osfinish - clean up before returning to the operating system */
void osfinish(void) 
{
}

/* oserror - print an error message */
void oserror(char *msg)
{
   printf("nyx error: %s\n", msg);
}

long osrand(long n)
{
   return (((int) rand()) % n);
}

/* osaopen - open an ascii file */
FILE *osaopen(name,mode) char *name,*mode;
{
   FILE *fp;
   fp = fopen(name,mode);
   return fp;
}

/* osbopen - open a binary file */
FILE *osbopen(char *name, char *mode)
{
   char bmode[10];
   FILE *fp;
   
   strncpy(bmode, mode, 8);
   strcat(bmode,"b");
   fp = fopen(name,bmode);
   return fp;
}

/* osclose - close a file */
int osclose(FILE *fp)
{
   return (fclose(fp));
}

/* osagetc - get a character from an ascii file */
int osagetc(FILE *fp)
{
   return (getc(fp));
}

/* osaputc - put a character to an ascii file */
int osaputc(int ch, FILE *fp)
{
   return (putc(ch,fp));
}

extern int dbgflg;

/* osbgetc - get a character from a binary file */
/* int osbgetc(fp) FILE *fp; {return (getc(fp));} */
#ifndef WIN32  // duplicated in winfun.c, per James Crook, 7/4/2003
	int osbgetc(FILE *fp)
	{
		return (getc(fp));
	}
#endif

/* osbputc - put a character to a binary file */
int osbputc(int ch, FILE *fp)
{
   return (putc(ch,fp));
}

/* ostputc - put a character to the terminal */
void ostputc(int ch)
{     
   oscheck();		/* check for control characters */
   
   if (nyx_output_pos < nyx_output_len)
      nyx_output_string[nyx_output_pos++] = (char)ch;
   else
      putchar(((char) ch));
}

/* osflush - flush the terminal input buffer */
void osflush(void)
{
}

/* oscheck - check for control characters during execution */
void oscheck(void)
{
   /* if they hit control-c:
      xflush(); xltoplevel(); return;
   */
}

/* xsystem - execute a system command */
#ifndef WIN32  // duplicated in winfun.c, per James Crook, 7/4/2003
LVAL xsystem()
{
   if (moreargs()) {
      unsigned char *cmd;
      cmd = (unsigned char *)getstring(xlgastring());
      fprintf(stderr, "Will not execute system command: %s\n", cmd);
   }
   return s_true;
}
#endif

/* xsetdir -- set current directory of the process */
LVAL xsetdir()
{
   char *dir = (char *)getstring(xlgastring());
   int result;
   xllastarg();
   result = chdir(dir);
   if (result) {
      perror("SETDIR");
   }
   return NULL;
}

/* xgetkey - get a key from the keyboard */
#ifndef WIN32  // duplicated in winfun.c, per James Crook, 7/4/2003
	LVAL xgetkey() {xllastarg(); return (cvfixnum((FIXTYPE)getchar()));}
#endif

/* ossymbols - enter os specific symbols */
#ifndef WIN32  // duplicated in winfun.c, per James Crook, 7/4/2003
	void ossymbols(void) {}
#endif

/* xsetupconsole -- used to configure window in Win32 version */
#ifndef WIN32  // duplicated in winfun.c, per James Crook, 7/4/2003
	LVAL xsetupconsole() { return NULL; }
#endif

/* control-C handling */
void ctcinit()	{}

