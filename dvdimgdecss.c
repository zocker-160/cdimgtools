/* dvdimgdecss.c - CSS descrambling of DVD Video images using libdvdcss/libdvdread
 * Copyrignt © 2012 Géraud Meyer <graud@gmx.com>
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
#   include "config.h"
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dvdread/dvd_reader.h>
#include <dvdread/dvd_udf.h>
#include <dvdcss/dvdcss.h>

#define EX_SUCCESS 0
#define EX_USAGE (~((~0)<<8))
#define EX_OPEN (~((~0)<<7))
#define EX_IO (1<<6)
#define EX_MISMATCH (1<<5)
#define EX_MEM (1<<4)
#define EX_NOP (1<<3)
#ifndef PROGRAM_NAME
#	define PROGRAM_NAME "dvdimgdecss"
#endif
#ifndef PROGRAM_VERSION
#	define PROGRAM_VERSION "0.1"
#endif
const char *progname = PROGRAM_NAME;
const char *progversion = PROGRAM_VERSION;
char verbosity = 1;
char dvdread_check = 0;
char dvdread_decrypt = 0;
#define TITLE_MAX 100

/* Make an array of an enum so as to iterate */
#define DOMAIN_MAX 4
const dvd_read_domain_t dvd_read_domains[DOMAIN_MAX] = {
	DVD_READ_INFO_FILE,
	DVD_READ_MENU_VOBS,
	DVD_READ_TITLE_VOBS,
	DVD_READ_INFO_BACKUP_FILE,
};

static const char *domainname( dvd_read_domain_t domain )
{
	switch( domain ) {
	case DVD_READ_INFO_FILE:
		return "INFO";
	case DVD_READ_MENU_VOBS:
		return "MENU";
	case DVD_READ_TITLE_VOBS:
		return "VOBS";
	case DVD_READ_INFO_BACKUP_FILE:
		return "IBUP";
	default:
		return "Unknown";
	}
}

/* A negative size means inexistent; a zero size means empty */
typedef struct {
	int start;
	int size;
} block_t;

typedef struct {
	block_t ifo, menu, vob, bup;
} titleblocks_t;

block_t *domainblock( titleblocks_t *tblocks, dvd_read_domain_t domain )
{
	if( ! tblocks ) return NULL;
	switch( domain ) {
	case DVD_READ_INFO_FILE:
		return &tblocks->ifo;
	case DVD_READ_MENU_VOBS:
		return &tblocks->menu;
	case DVD_READ_TITLE_VOBS:
		return &tblocks->vob;
	case DVD_READ_INFO_BACKUP_FILE:
		return &tblocks->bup;
	default:
		return NULL;
	}
}

typedef struct blockl {
	block_t  block;
	struct blockl *tail;
} *blockl_t;

static void usage( )
{
	printf( "Usage:\n" );
	printf( "\t%s -V\n", progname );
	printf( "\t%s [-v|-q] [-c] <dvd>\n", progname );
	printf( "\t%s [-v|-q] [-c|-C] <dvd> <out_file>\n", progname );
}

static int  dvdsize        ( const char * );
static int  savetitleblocks( dvd_reader_t *, titleblocks_t (*)[TITLE_MAX] );
static int  fileblock      ( dvd_reader_t *, char *, block_t * );
static int  removetitles   ( blockl_t, titleblocks_t [] );
static int  removeblock    ( blockl_t, const block_t );
static int  decrypttitles  ( dvd_reader_t *, dvdcss_t, int, titleblocks_t [] );
static int  copyblocks     ( dvdcss_t, int, blockl_t );
static int  copyblock      ( dvd_file_t *, dvdcss_t, int, block_t, const char * );
dvd_file_t *openfile       ( dvd_reader_t *, int, dvd_read_domain_t );
static int  progress       ( const int );
static int  printe         ( const char, const char *, ... );

/* Main for a command line tool */
int main( int argc, char *argv[] )
{
	char          *dvdfile, *imgfile = NULL;
	dvd_reader_t  *dvd;
	dvdcss_t      dvdcss = NULL;
	int           img;
	titleblocks_t titles[TITLE_MAX];
	struct blockl blocks;
	int           rc, status = EX_SUCCESS;

	setvbuf( stdout, NULL, _IOLBF, BUFSIZ );

	/* Options */
	extern int optind;
	while( (rc = getopt( argc, argv, "qvcCV" )) != -1 )
		switch( (char)rc ) {
		case 'q':
			verbosity--;
			break;
		case 'v':
			verbosity++;
		case 'c':
			dvdread_check = 1;
			break;
		case 'C':
			dvdread_check = 1;
			dvdread_decrypt = 1;
			break;
		case 'V':
			printf( "%s version %s (libdvdcss version %s)\n",
			  progname, progversion, dvdcss_interface_2 );
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
	if( argc < 1 || argc > 2 ) {
		printe( 1, "syntax error\n" );
		usage( );
		exit( EX_USAGE );
	}
	dvdfile = argv[0];
	if( argc == 2 ) imgfile = argv[1];
	if( !imgfile ) verbosity++;

	/* Open the DVD */
	printe( 2, "%s: version %s (libdvdcss version %s)\n",
	  progname, progversion, dvdcss_interface_2 );
	dvdcss = dvdcss_open( dvdfile );
	if( dvdcss == NULL ) {
		printe( 1, "opening of the DVD (%s) with libdvdcss failed\n", dvdfile );
		exit( status | EX_OPEN );
	}
	dvd = DVDOpen( dvdfile );
	if( dvd == NULL ) {
		printe( 1, "opening of the  DVD (%s) failed\n", dvdfile );
		exit( status | EX_OPEN );
	}

	/* Search the DVD for the positions of the title files */
	blocks.tail = NULL;
	blocks.block.start = 0;
	blocks.block.size = dvdsize( dvdfile );
	printe( 3, "%s: DVD end at 0x%08x\n", progname, blocks.block.size );
	status |= savetitleblocks( dvd, &titles );
	if( blocks.block.size < 0 ) {
		printe( 1, "cannot determine the size of the DVD\n" );
		blocks.block.size = 0;
	}
	else
		status |= removetitles( &blocks, titles );

	/* Make libdvdread try to get all the title keys now */
	if( dvdread_check ) openfile( dvd, 0, DVD_READ_MENU_VOBS );

	/* Check & Decrypt & Write */
	if( imgfile ) {
		img = open( imgfile, O_RDWR | O_CREAT,
		  S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH );
		if( img < 0 ) {
			printe( 1, "opening of the image file (%s) failed (%s)\n",
			  imgfile, strerror( errno ) );
			status |= EX_OPEN;
		}
		else {
			printe( 3, "\n" );
			status |= decrypttitles( dvd, dvdcss, img, titles );
			status |= copyblocks( dvdcss, img, &blocks );
			if( close( img ) < 0 ) {
				printe( 1, "closing of the image file failed (%s)\n",
				  strerror( errno ) );
				status |= EX_IO;
			}
		}
	}

	/* Close DVD */
	DVDClose( dvd );
	if( dvdcss_close( dvdcss ) < 0 ) {
		printe( 1, "closing of the DVD with libdvdcss failed\n" );
		status |= EX_IO;
	}
	exit( status );
}

/* Return the size in sectors (whether it is a file or a special device) */
static int dvdsize( const char *dvdfile )
{
	off_t       size;
	struct stat buf;
	int         dvd, rc;

	rc = stat( dvdfile, &buf );
	if( rc < 0 ) {
		printe( 1, "stat DVD (%s) failed (%s)\n", dvdfile, strerror( errno ) );
		return -1;
	}

	if( !buf.st_rdev )
		size = buf.st_size;
	else {
		dvd = open( dvdfile, O_RDONLY );
		if( dvd < 0 ) {
			printe( 1, "opening the DVD (%s) failed (%s)\n",
			  dvdfile, strerror( errno ) );
			return -1;
		}
		size = lseek( dvd, 0, SEEK_END );
		if( size < 0 ) {
			printe( 1, "seeking at the end of the DVD failed (%s)\n",
			  strerror( errno ) );
			return -1;
		}
		if( close( dvd ) < 0 )
			printe( 1, "closing of the DVD failed (%s)\n", strerror( errno ) );
	}

	if( size % DVD_VIDEO_LB_LEN )
		printe( 1, "DVD size is not a block multiple\n" );
	return size / DVD_VIDEO_LB_LEN;
}

/* Save the sector positions of the title/domain files */
static int savetitleblocks( dvd_reader_t *dvd, titleblocks_t (*titles)[TITLE_MAX] )
{
	int           status = EX_SUCCESS;
	char          filename[32]; /* MAX_UDF_FILE_NAME_LEN too much */
	titleblocks_t *tblocks;
	block_t       block;
	int           count = 0, title, i, start;

	/* Video Manager */
	title = 0;
	tblocks = &(*titles)[title];
	sprintf( filename, "/VIDEO_TS/VIDEO_TS.%s", "IFO" );
	fileblock( dvd, filename, &tblocks->ifo )
	|| printf( "%s: WARNING %s not found\n", progname, filename );
	sprintf( filename, "/VIDEO_TS/VIDEO_TS.%s", "VOB" );
	fileblock( dvd, filename, &tblocks->menu );
	fileblock( dvd, "/VIDEO_TS/invalid_name/", &tblocks->vob );
	sprintf( filename, "/VIDEO_TS/VIDEO_TS.%s", "BUP" );
	fileblock( dvd, filename, &tblocks->bup );

	/* Titles */
	for( title = 1; title < TITLE_MAX; title++ ) {
		tblocks = &(*titles)[title];
		sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.%s", title, 0, "IFO" );
		fileblock( dvd, filename, &tblocks->ifo )
		&& count++;
		sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.%s", title, 0, "VOB" );
		fileblock( dvd, filename, &tblocks->menu );
		sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.%s", title, 1, "VOB" );
		fileblock( dvd, filename, &tblocks->vob );
		for( i = 2, start = tblocks->vob.start+tblocks->vob.size; i < 10; i++ ) {
			/* Title VOBs may be split into several files */
			sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.%s", title, i, "VOB" );
			if( fileblock( dvd, filename, &block ) ) {
				tblocks->vob.size += block.size;
				if( block.start != start ) {
					printe( 1, "WARNING whole in title %d before part %d\n",
					  title, i );
					status |= EX_MISMATCH;
				}
				start = block.start+block.size;
			}
		}
		sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.%s", title, 0, "BUP" );
		fileblock( dvd, filename, &tblocks->bup );
	}

	printe( 3, "%s: %d titles found\n", progname, count );
	return status;
}

/* Record the sector range over which a file spans */
static int fileblock( dvd_reader_t *dvd, char *filename, block_t *block )
{
	uint32_t sector, size;
	(*block).start = 0;
	(*block).size = -1;

	sector = UDFFindFile( dvd, filename, &size );
	if( sector ) {
		if( size % DVD_VIDEO_LB_LEN )
			printe( 1, "WARNING size of %s is not a block multiple\n",
			  filename );
		size /= DVD_VIDEO_LB_LEN;
		(*block).start = sector;
		(*block).size = size;
		printe( 3, "%s: %s at 0x%08x-0x%08x\n",
		  progname, filename, sector, sector+size );
		return 1;
	}

	return 0;
}

/* Remove from blocks the block sectors of all title/domains */
static int removetitles( blockl_t blocks, titleblocks_t titles[] )
{
	block_t           *block;
	dvd_read_domain_t domain;
	int               title, i, rc, status = EX_SUCCESS;

	for( title = 0; title < TITLE_MAX; title++ )
		for( i = 0; i < DOMAIN_MAX; i++ ) {
			domain = dvd_read_domains[i];
			block = domainblock( &titles[title], domain );
			rc = removeblock( blocks, *block );
			if( rc == 0 ) {
				printe( 1, "Title %02d %s: block mismatch\n",
				  title, domainname( domain ) );
				status |= EX_MISMATCH;
			}
			if( rc == -1 ) {
				status |= EX_MEM;
				break;
			}
		}

	return status;
}

/* Remove a block from a block list (if it is contained in a block of the list) */
/* Inexistent/invalid blocks are ignored. */
/* If the list is increasing the result is also increasing. */
/* blocks must contain freeable memory (except for the first one). */
static int removeblock( blockl_t blocks, const block_t block )
{
	blockl_t cur, new;

	if( block.size < 0)
		return 1;

	for( cur = blocks; cur != NULL; cur = cur->tail )
		if
			( cur->block.start <= block.start
			  && cur->block.start+cur->block.size >= block.start+block.size )
		{
			/* Allocate a new node */
			new = malloc( sizeof( struct blockl ) );
			if( ! new ) {
				printe( 1, "memory allocation failed\n" );
				return -1;
			}
			/* Make a hole */
			new->block.start = block.start + block.size;
			new->block.size = cur->block.start + cur->block.size - new->block.start;
			cur->block.size = block.start - cur->block.start;
			/* Insert the new block */
			new->tail = cur->tail;
			cur->tail = new;
			/* Remove empty blocks */
			if( new->block.size == 0 ) {
				cur->tail = new->tail;
				free( new );
			}
			if( cur->block.size == 0 ) {
				new = cur->tail;
				*cur = *cur->tail;
				free( new );
			}
			return 1;
		}

	return 0;
}

/* Iterate over titles and domains and check consistency and copy blocks
 * corresponding to a VOB domain at the right position */
static int decrypttitles( dvd_reader_t *dvd, dvdcss_t dvdcss, int img, titleblocks_t titles[] )
{
	dvd_file_t        *file;
	block_t           *block;
	char              blockname[24];
	dvd_read_domain_t domain;
	int               title, i, rc, status = EX_NOP;

	for( title = 0; title < TITLE_MAX; title++ )
		for( i = 0; i < DOMAIN_MAX; i++ ) {
			domain = dvd_read_domains[i];
			block = domainblock( &titles[title], domain );
			if( dvdread_check )
				file = openfile( dvd, title, domain );
			snprintf( blockname, 24, "Title %02d %s", title, domainname( domain ) );

			/* Checks */
			if( dvdread_check && (!!file != !!(block->size >= 0)) ) {
				printe( 1, "ERROR %s: domain mismatch\n", blockname );
				status |= EX_MISMATCH;
			}
			if( dvdread_check && ! file ) continue;
			if( domain == DVD_READ_INFO_FILE )
				printe( 2, "TITLE %02d\n", title );
			if( dvdread_check && (DVDFileSize( file ) != (ssize_t)(block->size)) ) {
				printe( 1, "ERROR %s: size mismatch %zd != %d\n",
				  blockname, DVDFileSize( file ), block->size );
				status |= EX_MISMATCH;
			}

			/* Decrypt VOBs only */
			if( domain != DVD_READ_MENU_VOBS && domain != DVD_READ_TITLE_VOBS )
				rc = copyblock( NULL, dvdcss, img, *block, blockname );
			else
				rc = copyblock( dvdread_check ? file : (void *)1,
				                dvdread_decrypt ? NULL : dvdcss, img, *block, blockname );
			status |= rc;
			status &= ~EX_NOP;
			if( rc != EX_SUCCESS )
				printe( 1, "%s: partial decryption\n", blockname );

			DVDCloseFile( file );
		}

	return status;
}

static int copyblocks( dvdcss_t dvdcss, int img, blockl_t blocks )
{
	char blockname[24];
	int  status = EX_SUCCESS;

	printe( 2, "BLOCKS\n" );
	for( ; blocks != NULL; blocks = blocks->tail ) {
		snprintf( blockname, 24, "Block %08x-%08x",
		  blocks->block.start, blocks->block.start+blocks->block.size );
		status |= copyblock( NULL, dvdcss, img, blocks->block, blockname );
	}

	if( status )
		printe( 1, "error while copying ordinary blocks\n" );
	return status;
}

/* If file is not NULL, copy/decrypt a title/domain, using libdvdcss for
 * reading if dvdcss is not NULL, using libdvdread otherwise. */
/* If file is NULL, copy an ordinary block (ignoring title and domain). */
static int copyblock( dvd_file_t *file, dvdcss_t dvdcss, int img,
                      block_t block, const char *blockname )
{
	int           lb, rc, status = EX_SUCCESS;
	int           seek_flags = file ? DVDCSS_SEEK_KEY : DVDCSS_NOFLAGS;
	int           read_flags = file ? DVDCSS_READ_DECRYPT : DVDCSS_NOFLAGS;
	/* Aligned read buffer */
	static unsigned char data[DVD_VIDEO_LB_LEN * 2];
	unsigned char *buffer =
		data + DVD_VIDEO_LB_LEN
		     - ((long int)data & (DVD_VIDEO_LB_LEN-1));

	if( block.size < 0 ) {
		printe( 2, "%s: inva\n", blockname );
		return status;
	}
	if( file == NULL && dvdcss == NULL ) {
		printe( 2, "%s: skip\n", blockname );
		return status;
	}
	if( block.size == 0 ) {
		printe( 2, "%s: null\n", blockname );
		return status;
	}

	/* Seek in the input */
	if( dvdcss ) {
		rc = dvdcss_seek( dvdcss, block.start, seek_flags );
		if( rc < 0 ) {
			printe( 1, "%s: seeking in the input (dvdcss%s) failed (%s)\n",
			  blockname, seek_flags & DVDCSS_SEEK_KEY ? " key" : "",
			  dvdcss_error( dvdcss ) );
			status |= EX_IO;
		}
	}

	printe( 2, "%s: ", blockname );
	progress( -1 );

	/* Seek to to the right position in the output image */
	rc = ( lseek( img, (off_t)(block.start) * DVD_VIDEO_LB_LEN, SEEK_SET )
	       == (off_t)(-1) );
	if( rc ) {
		progress( 101 );
		printe( 1, "%s: seeking in the image failed (%s)\n",
		  blockname, strerror( errno ) );
		status |= EX_IO;
		return status;
	}

	for( lb = 0, progress( 0 ); lb < block.size; lb++ ) {
		/* Read one sector (possibly decrypted) */
		if( dvdcss )
			rc = ( dvdcss_read( dvdcss, buffer, 1, read_flags ) != 1 );
		else
			rc = ( DVDReadBlocks( file, lb, 1, buffer ) == (ssize_t)(-1) );
		if( rc ) {
			progress( 101 );
			if( file )
				printe( 1, "%s: reading sector %d failed\n", blockname, lb );
			status |= EX_IO;
			return status;
		}

		/* Write the data */
		rc = ( write( img, (void *)buffer, DVD_VIDEO_LB_LEN ) == (ssize_t)(-1) );
		if( rc ) {
			progress( 101 );
			printe( 1, "%s: writing sector %d failed (%s)\n",
			  blockname, lb, strerror( errno ) );
			status |= EX_IO;
			return status;
		}
		progress( (int)(lb*100/block.size) );
	}

	progress( 100 );
	return status;
}

/* Test for file existence before open (to silence libdvdnav) */
dvd_file_t *openfile( dvd_reader_t *dvd, int title, dvd_read_domain_t domain )
{
	static dvd_stat_t stat;
	if( ! DVDFileStat( dvd, title, domain, &stat ) )
		return DVDOpenFile( dvd, title, domain );
	return NULL;
}

/* Keep a percentage indicator at the end of the line */
static int progress( const int perc )
{
	static int last = 0;
	if( perc >= 101 ) { /* abort */
		last = 0;
		printe( 2, "\n" );
		fflush( stdout );
	}
	else if( perc < 0 ) { /* init */
		last = 0;
		if( verbosity > 2 )
			printe( 3, "   %%" );
		fflush( stdout );
	}
	else if( perc == 100 ) { /* finish */
		last = 0;
		if( verbosity > 2 )
			printe( 3, "\b\b\b\b100%%\n" );
		else
			printe( 2, "done\n" );
		fflush( stdout );
	}
	else if( perc != last ) { /* update */
		last = perc;
		if( verbosity > 2 )	{
			printe( 2, "\b\b\b\b% 3d%%", perc );
			fflush( stdout );
			return 1;
		}
	}
	return 0;
}

/* Print on stdout/stderr depending on the verbosity level */
int printe( const char level, const char *format, ... )
{
	va_list arg;
	int     ret;
	FILE   *stream = stdout;
	if( level <= 1 )
		stream = stderr;
	if( level > verbosity )
		return 1;

	va_start( arg, format );
	if( level <= 1 )
		fprintf( stream, "%s: ", progname );
	ret = vfprintf( stream, format, arg );
	va_end( arg );
	return ret;
}
