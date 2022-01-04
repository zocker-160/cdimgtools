/* cssdec.c - simple css descrambling program using libdvdcss
 * Copyright © 2012 Géraud Meyer <graud@gmx.com>
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful, but
 *   WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *   for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
#	include "config.h"
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <limits.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>

#include <dvdcss/dvdcss.h>
#include <dvdcss/version.h>

#define EX_SUCCESS 0
#define EX_USAGE (~((~0)<<8))
#define EX_OPEN (~((~0)<<7))
#define EX_IO (1<<6)
#define EX_KEY (1<<5)
#define EX_UNDECRYPTED (1<<0)
#ifndef PROGRAM_NAME
#	define PROGRAM_NAME "cssdec"
#endif
#ifndef PROGRAM_VERSION
#	define PROGRAM_VERSION "0.1"
#endif
const char *progname = PROGRAM_NAME;
const char *progversion = PROGRAM_VERSION;
char verbosity = 1;

/* readsector() return flags */
#define READ_ERROR 1<<0
#define READ_EOF 1<<1
#define SCRAMBLED 1<<2
#define DECRYPTED 1<<3
#define FAILED_DECRYPTION 1<<5

static int  readsector ( dvdcss_t, unsigned char *, const int );
static int  isscrambled( const unsigned char * );
static int  dumpsector ( unsigned char *, FILE * );
static int  printe     ( const char, const char *, ... );

static void usage( )
{
	fprintf( stderr, "Usage:\n" );
	fprintf( stderr, "\t%s -V\n", progname );
	fprintf( stderr, "\t%s [-v|-q] [-e] [-o <out_file> [-a]] <file> [<start_sect> [<end_sect>]]\n",
	  progname );
	fprintf( stderr, "\t%s [-v|-q] -k <file> [<start_sect>]\n", progname );
}

int main( int argc, char *argv[] )
{
	int            status = EX_SUCCESS;
	const char    *dvdfile, *outfile = NULL;
	dvdcss_t       dvdcss;
	FILE          *out = stdout;
	const char    *outfile_mode = "w+";
	unsigned char  data[ DVDCSS_BLOCK_SIZE * 2 ];
	unsigned char *buffer;
	unsigned int   sector = 0, end = INT_MAX;
	int            n_processed = 0, n_scrambled = 0, n_undecrypted = 0;
	int            rc;

	/* Options */
	char b_noeof = 0, b_keyonly = 0;
	extern int optind;
	extern char *optarg;
	while( (rc = getopt( argc, argv, "qveo:akV" )) != -1 )
		switch( (char)rc )
		{
		case 'q':
			verbosity--;
			break;
		case 'v':
			verbosity++;
			break;
		case 'e':
			b_noeof = 1;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'a':
			outfile_mode = "a+";
			break;
		case 'k':
			b_keyonly = 1;
			break;
		case 'V':
			printf( "%s version %s (libdvdcss version %s)\n", progname, progversion, DVDCSS_VERSION_STRING);
			exit( EX_SUCCESS );
			break;
		case '?':
		default:
			usage( );
			exit( EX_USAGE );
		}
	argc -= optind;
	argv += optind;

	/* Command line args */
	if( argc < 1 || argc > 3 || (b_keyonly && argc > 2) )
	{
		printe( 1, "syntax error" );
		usage( );
		exit( EX_USAGE );
	}
	dvdfile = argv[0];
	if( argc >= 2 ) sector = (int)strtol( argv[1], (char **)NULL, 0 );
	if( argc >= 3 ) end = (int)strtol( argv[2], (char **)NULL, 0 );

	/* Initialize libdvdcss */
	printe( 2, "%s version %s (libdvdcss version %s)", progname, progversion, DVDCSS_VERSION_STRING);
	dvdcss = dvdcss_open( (char *)dvdfile );
	if( dvdcss == NULL )
	{
		printe( 1, "opening of the DVD (%s) failed", dvdfile );
		exit( status | EX_OPEN );
	}

	/* Try to get a key */
	printe( 2, "trying to obtain the title key at sector %d", sector );
	rc = dvdcss_seek( dvdcss, sector, DVDCSS_SEEK_KEY );
	if( rc < 0 )
	{
		printe( 1, "getting the title key failed (%s)",
		  dvdcss_error( dvdcss ) );
		exit( status | EX_KEY );
	}
	if( b_keyonly ) goto CLOSEDVD_EXIT;

	/* Open the output file */
	if( outfile )
	{
		out = fopen( outfile, outfile_mode );
		if( out == NULL )
		{
			printe( 1, "opening of the output file (%s) failed (%s)",
			  outfile, strerror( errno ) );
			exit( status | EX_OPEN );
		}
	}

	/* Align our read buffer */
	buffer = data + DVDCSS_BLOCK_SIZE
	              - ((long int)data & (DVDCSS_BLOCK_SIZE-1));

	for( ; sector < end; sector++ )
	{
		/* Read decrypted */
		rc = readsector( dvdcss, buffer, sector );

		/* Check & Count */
		if( rc & READ_EOF )
		{
			printe( 2, "stop reading before sector %d", sector );
			break;
		}
		if( rc & READ_ERROR )
		{
			printe( 1, "sect %d: read error; aborting", sector );
			status |= EX_IO;
			break;
		}
		n_processed++;
		if( rc & SCRAMBLED )
		{
			n_scrambled++;
			if( ! (rc & DECRYPTED) )
				n_undecrypted++;
		}

		/* Process the sector */
		if( ! dumpsector( buffer, out ) ) {
			printe( 1, "sect %d: writing failed; aborting", sector );
			status |= EX_IO;
			break;
		};
	}

	/* Summary & Return status */
	printe( 2, "summary of processed sectors:" );
	printe( 2, "%d undecrypted scrambled sectors", n_undecrypted );
	printe( 2, "%d scrambled sectors", n_scrambled );
	printe( 2, "%d processed sectors", n_processed );
	if( status & EX_IO )
		printe( 2, "partial processing because of an I/O error" );
	if( b_noeof && sector < end ) status |= EX_IO;
	if( n_undecrypted > 0 ) status |= EX_UNDECRYPTED;

	/* Close & Exit */
	if( fflush( out ) != 0 )
	{
		printe( 1, "flushing of the output failed (%s)", strerror( errno ) );
		status |= EX_IO;
	}
	if( outfile )
		if( fclose( out ) == EOF )
			printe( 1, "closing of the ouput file failed (%s)",
			  strerror( errno ) );
CLOSEDVD_EXIT:
	rc = dvdcss_close( dvdcss );
	if( rc < 0 ) printe( 1, "closing of the DVD failed" );
	exit( status );
}

/* Read a sector; read decrypted again if it seems crypted */
static int readsector( dvdcss_t dvdcss, unsigned char *buffer, const int sector )
{
	int rc, flags = 0;

	/* Seek at sector sector and read one sector */
	rc = dvdcss_seek( dvdcss, sector, DVDCSS_NOFLAGS );
	if( rc < 0 )
	{
		printe( 1, "sect %d: seek failed (%s)", sector, dvdcss_error( dvdcss ) );
		return flags | READ_ERROR;
	}
	rc = dvdcss_read( dvdcss, buffer, 1, DVDCSS_NOFLAGS );
	if( rc < 0 )
	{
		printe( 1, "sect %d: read failed (%s)", sector, dvdcss_error( dvdcss ) );
		return flags | READ_ERROR;
	}
	if( rc == 0 )
	{
		printe( 1, "sect %d: EOF", sector );
		return flags | READ_EOF;
	}

	if( ! isscrambled( buffer ) /* Check if sector is encrypted */ )
		printe( 3, "sect %d: not crypted", sector );
	else
	{
		printe( 3, "sect %d: crypted", sector );
		flags |= SCRAMBLED;

		/* Seek at sector sector and try to decrypt sector */
		rc = dvdcss_seek( dvdcss, sector, DVDCSS_NOFLAGS );
		if( rc < 0 )
		{
			printe( 1, "sect %d: seek failed (%s)",
			  sector, dvdcss_error( dvdcss ) );
			return flags;
		}
		rc = dvdcss_read( dvdcss, buffer, 1, DVDCSS_READ_DECRYPT );
		  /* Warning: A failure to decrypt is not considered an error in
		   * libdvdcss 1.2.12 */
		if( rc != 1 )
		{
			printe( 2, "sect %d: read (decrypted) failed (%s)",
			  sector, dvdcss_error( dvdcss ) );
			return flags;
		}

		if( isscrambled( buffer ) /* Check if the decryption really succeeded */ )
		{
			/* Probably a bug in libdvdcss not to have given an error earlier */
			printe( 1, "sect %d: still apparently crypted after decryption",
			  sector );
			flags |= FAILED_DECRYPTION;
		}
		else
		{
			printe( 3, "sect %d: decrypted", sector );
			flags |= DECRYPTED;
		}
	}

	return flags;
}

/* Check if a sector is scrambled */
static int isscrambled( const unsigned char *buffer )
{
	return buffer[ 0x14 ] & 0x30;
}

/* Dump the sector on stdout */
static int dumpsector( unsigned char *buffer, FILE *out )
{
	size_t n;
	n = fwrite( (void *)buffer, DVDCSS_BLOCK_SIZE, 1, out );
	if( ferror( out ) )
	{
		printe( 1, "write error (%s)", strerror( errno ) );
		clearerr( out );
		return 0;
	}
	return (n == 1);
}

/* Print a line on stderr preceded by the program name */
int printe( const char level, const char *format, ... )
{
	va_list arg;
	int rc;
	if( level > verbosity ) return 1;

	va_start( arg, format );
	fprintf( stderr, "%s: ", progname );
	rc = vfprintf( stderr, format, arg );
	va_end( arg );
	fprintf( stderr, "\n" );
	return rc;
}
