/*
** Copyright (C) 1999-2002 Erik de Castro Lopo <erikd@zip.com.au>
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	<stdio.h>

#include	"sndfile.h"
#include	"config.h"
#include	"common.h"

/*-static sf_count_t	raw_seek   (SF_PRIVATE *psf, sf_count_t offset, int whence) ;-*/

/*------------------------------------------------------------------------------
** Public function.
*/

int 	
raw_open	(SF_PRIVATE *psf)
{	int	subformat, error = SFE_NO_ERROR ;
		
	subformat = psf->sf.format & SF_FORMAT_SUBMASK ;

	psf->endian = psf->sf.format & SF_FORMAT_ENDMASK ;

	if (CPU_IS_BIG_ENDIAN && (psf->endian == 0 || psf->endian == SF_ENDIAN_CPU))
		psf->endian = SF_ENDIAN_BIG ;
	else if (CPU_IS_LITTLE_ENDIAN && (psf->endian == 0 || psf->endian == SF_ENDIAN_CPU))
		psf->endian = SF_ENDIAN_LITTLE ;

	psf->blockwidth = psf->bytewidth * psf->sf.channels ;
	psf->dataoffset = 0 ;
	psf->datalength = psf->filelength ;

	switch (subformat)
	{	case SF_FORMAT_PCM_S8 :
				psf->chars = SF_CHARS_SIGNED ;
				error = pcm_init (psf) ;
				break ;

		case SF_FORMAT_PCM_U8 :
				psf->chars = SF_CHARS_UNSIGNED ;
				error = pcm_init (psf) ;
				break ;
				
		case SF_FORMAT_PCM_16 :
		case SF_FORMAT_PCM_24 :
		case SF_FORMAT_PCM_32 :
				error = pcm_init (psf) ;
				break ;
		
		case SF_FORMAT_ULAW :
				error = ulaw_init (psf) ;
				break ;
		
		case SF_FORMAT_ALAW :
				error = alaw_init (psf) ;
				break ;
		
		case SF_FORMAT_FLOAT :
				error = float32_init (psf) ;
				break ;

		case SF_FORMAT_DOUBLE :
				error = double64_init (psf) ;
				break ;

		case SF_FORMAT_DWVW_12 :
				error = dwvw_init (psf, 12) ;
				break ;

		case SF_FORMAT_DWVW_16 :
				error = dwvw_init (psf, 16) ;
				break ;

		case SF_FORMAT_DWVW_24 :
				error = dwvw_init (psf, 24) ;
				break ;

		case SF_FORMAT_GSM610 :
				error = gsm610_init (psf) ;
				break ;

		default : return SFE_BAD_OPEN_FORMAT ;
		} ;

	return error ;
} /* raw_open */
