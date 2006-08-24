/*
comedi_calibrate/save_cal.c - stuff for saving calibration info to
a file.

Copyright (C) 2003  Frank Mori Hess <fmhess@users.sourceforge.net>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
license, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define _GNU_SOURCE

#include "comedi_calibrate_shared.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <string.h>

static void indent(FILE *file, unsigned numTabs)
{
	unsigned i;
	char *string = malloc(numTabs + 1);
	assert(string);
	for(i = 0; i < numTabs; ++i)
		string[i] = '\t';
	string[i] = '\0';
	fprintf(file, string);
	free(string);
}

static void write_caldac( FILE *file, comedi_caldac_t caldac )
{
	const unsigned baseNumTabs = 4;
	indent(file, baseNumTabs);
	fprintf( file, "{\n" );
	indent(file, baseNumTabs + 1);
	fprintf( file, "subdevice => %i,\n", caldac.subdevice );
	indent(file, baseNumTabs + 1);
	fprintf( file, "channel => %i,\n", caldac.channel );
	indent(file, baseNumTabs + 1);
	fprintf( file, "value => %i,\n", caldac.value );
	indent(file, baseNumTabs);
	fprintf( file, "}" );
}

static void write_polynomial(FILE *file, const comedi_polynomial_t *polynomial)
{
	const unsigned baseNumTabs = 3;
	unsigned i;

	indent(file, baseNumTabs);
	fprintf(file, "{\n");
	indent(file, baseNumTabs + 1);
	fprintf(file, "expansion_origin => %g,\n", polynomial->expansion_origin);
	indent(file, baseNumTabs + 1);
	fprintf(file, "coefficients => [");
	for(i = 0; i <= polynomial->order; ++i)
	{
		assert(i < COMEDI_MAX_NUM_POLYNOMIAL_COEFFICIENTS);
		fprintf(file, "%g,", polynomial->coefficients[i]);
	}
	fprintf(file, "],\n");
	indent(file, baseNumTabs);
	fprintf(file, "}");
}

void write_calibration_setting( FILE *file, comedi_calibration_setting_t setting )
{
	unsigned baseNumTabs = 2;
	int i;

	indent(file, baseNumTabs);
	fprintf( file, "{\n" );
	indent(file, baseNumTabs + 1);
	fprintf( file, "subdevice => %i,\n", setting.subdevice );
	indent(file, baseNumTabs + 1);
	fprintf( file, "channels => [" );
	for( i = 0; i < setting.num_channels; i++ )
		fprintf( file, "%i,", setting.channels[ i ] );
	fprintf( file, "],\n" );
	indent(file, baseNumTabs + 1);
	fprintf( file, "ranges => [" );
	for( i = 0; i < setting.num_ranges; i++ )
		fprintf( file, "%i,", setting.ranges[ i ] );
	fprintf( file, "],\n" );
	indent(file, baseNumTabs + 1);
	fprintf( file, "arefs => [" );
	for( i = 0; i < setting.num_arefs; i++ )
		fprintf( file, "%i,", setting.arefs[ i ] );
	fprintf( file, "],\n" );
	indent(file, baseNumTabs + 1);
	fprintf( file, "caldacs =>\n" );
	indent(file, baseNumTabs + 1);
	fprintf( file, "[\n" );
	for( i = 0; i < setting.num_caldacs; i++ )
	{
		write_caldac( file, setting.caldacs[ i ] );
		fprintf( file, ",\n" );
	}
	indent(file, baseNumTabs + 1);
	fprintf( file, "],\n" );
	indent(file, baseNumTabs + 1);
	if(setting.soft_calibration.to_phys)
	{
		fprintf(file, "softcal_to_phys =>\n");
		write_polynomial(file, setting.soft_calibration.to_phys);
		fprintf( file, ",\n" );
	}
	if(setting.soft_calibration.from_phys)
	{
		indent(file, baseNumTabs + 1);
		fprintf(file, "softcal_from_phys =>\n");
		write_polynomial(file, setting.soft_calibration.from_phys);
	}
	indent(file, baseNumTabs);
	fprintf( file, "}" );
}

int write_calibration_perl_hash( FILE *file, const comedi_calibration_t *calibration )
{
	int i;
	time_t now;

	now = time( NULL );
	fprintf( file, "#calibration file generated by comedi_calibrate\n"
		"#%s\n", ctime( &now ) );
	fprintf( file, "{\n" );
	fprintf( file, "\tdriver_name => \"%s\",\n", calibration->driver_name );
	fprintf( file, "\tboard_name => \"%s\",\n", calibration->board_name );
	fprintf( file, "\tcalibrations =>\n"
		"\t[\n" );
	for( i = 0; i < calibration->num_settings; i++ )
	{
		write_calibration_setting( file, calibration->settings[ i ] );
		fprintf( file, ",\n" );
	}
	fprintf( file, "\t],\n"
		"}\n");

	return 0;
}

int write_calibration_file(const char *file_path, const comedi_calibration_t *calibration)
{
	FILE *file;
	int retval;

	assert(calibration);
	assert(file_path);

	file = fopen(file_path, "w");
	if(file == NULL)
	{
		fprintf( stderr, "failed to open file %s for writing\n",
			file_path );
		perror( "fopen" );
		return -1;
	}

// 	DPRINT( 0, "writing calibration to %s\n", setup->cal_save_file_path );
	retval = write_calibration_perl_hash(file, calibration);

	fclose(file);

	return retval;
}

comedi_calibration_setting_t* sc_alloc_calibration_setting(comedi_calibration_t *calibration)
{
	comedi_calibration_setting_t *temp;

	temp = realloc(calibration->settings,
		(calibration->num_settings + 1) * sizeof(comedi_calibration_setting_t));
	assert(temp);
	calibration->settings = temp;
	memset(&calibration->settings[calibration->num_settings],
		0, sizeof(comedi_calibration_setting_t));

	calibration->num_settings++;

	return &calibration->settings[calibration->num_settings - 1];
}

void sc_push_caldac(comedi_calibration_setting_t *saved_cal, comedi_caldac_t caldac)
{
	int i;

	/* check if caldac is already listed, in which case we just update */
	for( i = 0; i < saved_cal->num_caldacs; i++ )
	{
		if( saved_cal->caldacs[ i ].subdevice != caldac.subdevice ) continue;
		if( saved_cal->caldacs[ i ].channel != caldac.channel ) continue;
		break;
	}
	if( i < saved_cal->num_caldacs )
	{
		saved_cal->caldacs[ i ].value = caldac.value;
		return;
	}
	saved_cal->caldacs = realloc(saved_cal->caldacs,
		(saved_cal->num_caldacs + 1) * sizeof(comedi_caldac_t));
	if(saved_cal->caldacs == NULL)
	{
		fprintf( stderr, "memory allocation failure\n" );
		abort();
	}
	saved_cal->caldacs[ saved_cal->num_caldacs ] = caldac;
	saved_cal->num_caldacs++;
}

void sc_push_channel( comedi_calibration_setting_t *saved_cal, int channel )
{
	if( channel == SC_ALL_CHANNELS )
	{
		saved_cal->num_channels = 0;
		if( saved_cal->channels )
		{
			free( saved_cal->channels );
			saved_cal->channels = NULL;
		}
	}else
	{
		saved_cal->channels = realloc( saved_cal->channels,
			( saved_cal->num_channels + 1 ) * sizeof( int ) );
		if( saved_cal->channels == NULL )
		{
			fprintf( stderr, "memory allocation failure\n" );
			abort();
		}
		saved_cal->channels[ saved_cal->num_channels++ ] = channel;
	}
}

void sc_push_range( comedi_calibration_setting_t *saved_cal, int range )
{
	if( range == SC_ALL_RANGES )
	{
		saved_cal->num_ranges = 0;
		if( saved_cal->ranges )
		{
			free( saved_cal->ranges );
			saved_cal->ranges = NULL;
		}
	}
	else
	{
		saved_cal->ranges = realloc( saved_cal->ranges,
			( saved_cal->num_ranges + 1 ) * sizeof( int ) );
		if( saved_cal->ranges == NULL )
		{
			fprintf( stderr, "memory allocation failure\n" );
			abort();
		}
		saved_cal->ranges[ saved_cal->num_ranges++ ] = range;
	}

}

void sc_push_aref( comedi_calibration_setting_t *saved_cal, int aref )
{
	assert( saved_cal->num_arefs < CS_MAX_AREFS_LENGTH );

	if( aref == SC_ALL_AREFS )
		saved_cal->num_arefs = 0;
	else
		saved_cal->arefs[ saved_cal->num_arefs++ ] = aref;
}
