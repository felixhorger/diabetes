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

#include "fileheader.c"
#include "protocols.c"
#include "data.c"



/* TODO:
Loading data:
Better approach for top level usage (all functions must set their position in file, how?):
twix = twix_open(name); // Just opens the file and reads the header, nothing else
// twix is a struct holding the file pointer, file_header and *protocols and *data from above
twix_load_protocol(twix); // reads the protocol and parses it, because why would you read it without using it? For users that want more control, there are still the low-level functions, how about exporting them? Would need to add twix_ to everyone of them. Also think about which data types need to be exported.
twix_get_size(twix); // applied to the main twix struct
twix_get_TE(twix);
kspace = twix_get_kspace(twix); // reads in all the data, in which form? Check for the special ones not containing data.
kspace = twix_filter_kspace(twix, filter_func()); // could also filter, filter_func can return 0 for ok, 1 for no, 2 for stop reading. What happens with the pointer structure then, being finished "half way"?
twix_close(twix); // Frees memory and closes the file, internally can have function twix_free_data, twix_free_protocol etc which the user can also have available

*/

// TODO: Functionality to get scan data and sampling indices.
// TODO: better functions for protocols
// TODO: functionality to free protocol and other memory 
// TODO: load all scans: for (int scan = 0; scan < file_header.num_scans; scan++) {






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

	Twix* twix = (Twix *) malloc(sizeof(Twix));
	twix->f = f;
	twix->file_header = file_header;

	twix->protocols = (Protocol *) malloc(sizeof(Protocol) * file_header->num_scans);
	for (int scan = 0; scan < file_header->num_scans; scan++) {
		fseek(f, file_header->entries[scan].offset, SEEK_SET);
		read_protocol_header(f, twix->protocols + scan);
	}

	return twix;
}


void twix_close(Twix* twix)
{
	fclose(twix->f);
	free(twix->file_header);

	// TODO: these two need freeing of all substructures
	free(twix->protocols);
	free(twix->data);
	free(twix);
	//
	return;
}



int main(int argc, char* argv[])
{
	if (argc < 2) {
		printf("Error: no filename given\n");
		exit(1);
	}

	Twix* twix = twix_open(argv[1]);
	//twix_save_scanner_protocol(twix, 0, "scanner_protocol.pro");
	twix_load_protocol(twix, 0, "Config");
	//twix_load_data(twix, 0);
	twix_close(twix);

	return 0;
}

