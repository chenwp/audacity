/*
** Copyright (C) 2001-2002 Erik de Castro Lopo <erikd@zip.com.au>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/



#include	<stdio.h>
#include	<string.h>
#include	<unistd.h>
#include	<math.h>

#include	<sndfile.h>

#include	"utils.h"

#define	BUFFER_LEN		(1<<10)
#define LOG_BUFFER_SIZE	1024

static void	test_update_header (char *filename, int typemajor) ;

/* Force the start of this buffer to be double aligned. Sparc-solaris will
** choke if its not.
*/
static	short	data_out [BUFFER_LEN] ;
static	short	data_in  [BUFFER_LEN] ;

int		
main (int argc, char *argv[])
{	int		bDoAll = 0 ;
	int		nTests = 0 ;

	if (argc != 2)
	{	printf ("Usage : %s <test>\n", argv [0]) ;
		printf ("    Where <test> is one of the following:\n") ;
		printf ("           wav  - test WAV file peak chunk\n") ;
		printf ("           aiff - test AIFF file PEAK chunk\n") ;
		printf ("           all  - perform all tests\n") ;
		exit (1) ;
		} ;

	bDoAll=!strcmp (argv [1], "all");
		
	if (bDoAll || ! strcmp (argv [1], "wav"))
	{	test_update_header ("header.wav", SF_FORMAT_WAV) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "aiff"))
	{	test_update_header ("header.aiff", SF_FORMAT_AIFF) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "au"))
	{	test_update_header ("header.au", SF_FORMAT_AU) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "svx"))
	{	test_update_header ("header.svx", SF_FORMAT_SVX) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "nist"))
	{	test_update_header ("header.nist", SF_FORMAT_NIST) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "ircam"))
	{	test_update_header ("header.ircam", SF_FORMAT_IRCAM) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "voc"))
	{	test_update_header ("header.voc", SF_FORMAT_VOC) ;
		nTests++ ;
		} ;

	if (bDoAll || ! strcmp (argv [1], "w64"))
	{	test_update_header ("header.w64", SF_FORMAT_W64) ;
		nTests++ ;
		} ;

	if (nTests == 0)
	{	printf ("Mono : ************************************\n") ;
		printf ("Mono : *  No '%s' test defined.\n", argv [1]) ;
		printf ("Mono : ************************************\n") ;
		return 1 ;
		} ;

	return 0;
} /* main */


/*============================================================================================
**	Helper functions and macros.
*/ 

#define PUT_DOTS(k)					\
			{	while (k--)			\
					putchar ('.') ;	\
				putchar (' ') ;		\
				}


/*============================================================================================
**	Here are the test functions.
*/ 

static void	
test_update_header (char *filename, int typemajor)
{	SNDFILE		*outfile, *infile ;
	SF_INFO		sfinfo ;
	int			k, frames ;

	printf ("    test_update_header  : %s ", filename) ;
	fflush (stdout) ;
	
	k = abs (18 - strlen (filename)) ;
	PUT_DOTS (k) ;
	
	sfinfo.samplerate  = 44100 ;
	sfinfo.format 	   = (typemajor | SF_FORMAT_PCM_16) ;
	sfinfo.channels    = 1 ;
	sfinfo.frames     = 0 ;

	frames = BUFFER_LEN / sfinfo.channels ;

	outfile = test_open_file_or_die (filename, SFM_WRITE, &sfinfo, __LINE__) ;

	for (k = 0 ; k < BUFFER_LEN ; k++)
		data_out [k] = k + 1 ;
	test_write_short_or_die (outfile, 0, data_out, BUFFER_LEN, __LINE__) ;

	infile = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;
	sf_close (infile) ;

	sf_command (outfile, SFC_UPDATE_HEADER_NOW, NULL, 0) ;

	/* Open file and check log buffer for an error. If header update failed
	** the the log buffer will contain errors. 
	*/
	infile = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;
	check_log_buffer_or_die (infile) ;

	if (abs (BUFFER_LEN - sfinfo.frames) > 1)
	{	printf ("\n\nLine %d : Incorrect sample count (%ld should be %d)\n", __LINE__, SF_COUNT_TO_LONG (sfinfo.frames), BUFFER_LEN) ;
		dump_log_buffer (infile) ;
		exit (1) ;
		} ;

	test_read_short_or_die (infile, 0, data_in, BUFFER_LEN, __LINE__) ;
	for (k = 0 ; k < BUFFER_LEN ; k++)
		if (data_out [k] != k + 1)
			printf ("Error : line %d\n", __LINE__) ;

	sf_close (infile) ;
	
	/* Set auto update on. */
	sf_command (outfile, SFC_SET_UPDATE_HEADER_AUTO, NULL, SF_TRUE) ;

	/* Write more data_out. */
	for (k = 0 ; k < BUFFER_LEN ; k++)
		data_out [k] = k + 2 ;
	test_write_short_or_die (outfile, 0, data_out, BUFFER_LEN, __LINE__) ;

	/* Open file again and make sure no errors in log buffer. */
	infile = test_open_file_or_die (filename, SFM_READ, &sfinfo, __LINE__) ;
	check_log_buffer_or_die (infile) ;
	
	if (abs (2 * BUFFER_LEN - sfinfo.frames) > 1)
	{	printf ("\n\nLine %d : Incorrect sample count (%ld should be %d)\n", __LINE__, SF_COUNT_TO_LONG (sfinfo.frames), 2 * BUFFER_LEN) ;
		dump_log_buffer (infile) ;
		exit (1) ;
		} ;

	sf_close (infile) ;
	
	sf_close (outfile) ;

	unlink (filename) ;
	puts ("ok") ;
} /* test_update_header */

