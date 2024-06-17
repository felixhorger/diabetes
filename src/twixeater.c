#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "utils.c"
#include "../../bmalloc/include/bmalloc.h"

typedef struct FileHeader FileHeader; // first to be read in
typedef struct ScanHeader ScanHeader; // one for e.g. reference scan, actual scan, ...
typedef struct Protocol Protocol;
typedef struct Parameter Parameter;
typedef struct ParameterList ParameterList;
typedef struct ScanData ScanData; // Pointers into buffer holding scan data are stored here
typedef struct ReadoutHeader ReadoutHeader; // Formerly called measurement data header (MDH), information for single readout
typedef struct ChannelHeader ChannelHeader; // Information of a single channel, placed just after the readout header and before the readout of every channel
typedef struct Twix Twix;

struct Twix
{
	FILE* f;
	FileHeader *file_header; // TODO: Why are these all pointers? at least for FileHeader and ScanData could have it in here directly
	Protocol *protocols;
	ScanData *data;
};
// TODO: add array with fixed order of protocols (Config, Meas, MeasYAPS, Dicom, ...)



#include "fileheader.c"
#include "protocols.c"
#include "data.c"



/* TODO:
twix_get_size(twix); // applied to the main twix struct
Functionality to get scan data and sampling indices.
better functions for protocols
twix_get_TE(twix);
kspace = twix_get_kspace(twix); // reads in all the data, in which form? Check for the special ones not containing data.
? kspace = twix_filter_kspace(twix, filter_func()); // could also filter, filter_func can return 0 for ok, 1 for no, 2 for stop reading. What happens with the pointer structure then, being finished "half way"?
search protocol functions
*/


Twix* twix_open(char *filename)
{
	FILE* f;

	f = fopen(filename, "rb");
	check_pointer(f, "Could not open file %s\n", filename);
	// TODO: setvbuf?

	FileHeader *file_header = (FileHeader *) malloc(sizeof(FileHeader));
	safe_fread(f, file_header, sizeof(FileHeader));
	check_file_header(file_header);
	//print_file_header(file_header);

	Twix* twix = (Twix *) calloc(1, sizeof(Twix));
	twix->f = f;
	twix->file_header = file_header;

	twix->protocols = (Protocol *) calloc(1, sizeof(Protocol) * file_header->num_scans);
	for (int scan = 0; scan < file_header->num_scans; scan++) {
		fseek(f, file_header->entries[scan].offset, SEEK_SET);
		read_protocol_header(f, twix->protocols + scan);
		memset(twix->protocols[scan].sets, -1, sizeof(twix->protocols[scan].sets));
	}

	return twix;
}


void twix_close(Twix* twix)
{
	for (int scan = 0; scan < twix->file_header->num_scans; scan++) {
		Protocol *protocol = twix->protocols + scan;
		Parameter *params = protocol->parameters;
		for (int p = 0; p < protocol->num; p++) free_parameter(params + p);
		free(params);
	}
	free(twix->protocols);

	// TODO: needs freeing of all substructures
	if (twix->data != NULL) {
		free(twix->data->buffer);
		lfree(twix->data->hdrs);
		free(twix->data);
	}

	free(twix->file_header);
	fclose(twix->f);
	free(twix);

	return;
}



int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Error: no filename given\n");
		exit(EXIT_FAILURE);
	}

	Twix* twix = twix_open(argv[1]);
	twix_save_scanner_protocol(twix, 0, "scanner_protocol.pro");
	twix_load_protocol(twix, 0, twix_config);
	twix_load_protocol(twix, 0, twix_meas);
	twix_load_protocol(twix, 0, twix_dicom);
	twix_load_protocol(twix, 0, twix_measyaps);

	printf("Number of Rx channels %d\n", twix_receive_channels(twix, 0));

	int size[3];
	twix_kspace_size(twix, 0, size);
	printf("kspace size: %d %d %d\n", size[0], size[1], size[2]);

	int accel[2];
	twix_acceleration_factors(twix, 0, accel);
	printf("acceleration: %d %d\n", accel[0], accel[1]);

	printf("flip angle %fdeg\n", twix_flip_angle(twix, 0, 0));
	printf("echo time %fms\n", twix_echo_time(twix, 0, 0));
	printf("repetition time %fms\n", twix_repetition_time(twix, 0, 0));
	printf("dwell time %fmus\n", twix_dwell_time(twix, 0));

	twix_ref_dims(twix, 0, size);
	printf("ref size: %d %d \n", size[0], size[1]);

	double orientation[4], shift[3], fov[3];
	twix_coordinates(twix, 0, orientation, shift, fov);
	printf(
		"Normal: (%f, %f, %f)\nIn-plane rotation: %fdeg\nShift: (%f, %f, %f)mm\nField of view: (%f, %f, %f)mm\n",
		orientation[0], orientation[1], orientation[2],
		orientation[3],
		shift[0], shift[1], shift[2],
		fov[0], fov[1], fov[2]
	);


	char pos[4];
	twix_patient_position(twix, 0, pos);
	printf("Patient position %s\n", pos);

	printf("Patient sex %ld\n", twix_patient_sex(twix, 0));

	double doubleparam;
	long longparam;
	twix_wip_param(twix, 0, &doubleparam, 2);
	twix_wip_param(twix, 0, &longparam, 10);
	printf("WIP Param at index 2 (long) %ld, at index 10 (double) %f\n", longparam, doubleparam);

	printf("Reference voltage %fV\n", twix_ref_voltage(twix, 0));

	twix_load_data(twix, 0);
	printf("Number of readouts %ld\n", twix->data->n);
	//print_readout_header(lget(twix->data->hdrs, 0));
	//print_readout_header(lget(twix->data->hdrs, 10));

	printf("Echoes %d %d %d %d %d %d %d %d %d\n",
		lget(twix->data->hdrs, 0)->echo,
		lget(twix->data->hdrs, 1)->echo,
		lget(twix->data->hdrs, 2)->echo,
		lget(twix->data->hdrs, 3)->echo,
		lget(twix->data->hdrs, 4)->echo,
		lget(twix->data->hdrs, 5)->echo,
		lget(twix->data->hdrs, 6)->echo,
		lget(twix->data->hdrs, 7)->echo,
		lget(twix->data->hdrs, 8)->echo
	);

	printf("Header expected size %d, measured %ld\n", get_readout_num_bytes(lget(twix->data->hdrs, 0)), (size_t)lget(twix->data->hdrs, 1) - (size_t)lget(twix->data->hdrs, 0));

	printf("Scan counter %d %d\n", lget(twix->data->hdrs, 0)->scan_counter, lget(twix->data->hdrs, 1)->scan_counter);

	//ChannelHeader *p = (ChannelHeader *)(lget(twix->data->hdrs, 0)+1);
	//printf("%d\n",     p->id);
	//printf("%d\n",
	//	(
	//		(ChannelHeader *)(
	//			(void *)p + 32*(sizeof(ChannelHeader) + 2 * 4 * 96)
	//		)
	//	)->id
	//);


	float *kspace;
	uint16_t *idx;
	uint8_t which_idx[] = {0, 3};
	twix_get_scandata(twix, &kspace, &idx, which_idx, 2);
	free(kspace);
	free(idx);


	twix_close(twix);

	return 0;
}

