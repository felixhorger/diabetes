#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "utils.c"

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
	FileHeader *file_header;
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
	free(twix->data);

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
	twix_close(twix);
	return 0;
	twix_load_protocol(twix, 0, twix_config);
	twix_load_protocol(twix, 0, twix_dicom);
	twix_load_protocol(twix, 0, twix_meas);
	twix_load_protocol(twix, 0, twix_measyaps);
	int size[3];
	twix_kspace_dims(twix, 0, size);
	printf("kspace dims: %d %d %d\n", size[0], size[1], size[2]);
	//twix_load_data(twix, 0);
	twix_close(twix);

	return 0;
}

