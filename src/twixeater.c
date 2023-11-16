
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include "utils.c"
#include "fileheader.c"
#include "protocols.c"
#include "data.c"

// TODO: make function to get protocol in string form from it

int main(int argc, char* argv[])
{
	FILE *f;
	if (argc < 2) {
		printf("Error: no filename given\n");
		exit(1);
	}
	f = fopen(argv[1], "rb");
	// TODO: setvbuf

	FileHeader file_header;
	safe_fread(f, &file_header, sizeof(file_header));
	check_file_header(file_header);

	//print_file_header(file_header);



	Protocol *protocols = (Protocol *) malloc(sizeof(Protocol) * file_header.num_scans);
	ScanData *data = (ScanData *) malloc(sizeof(ScanData) * file_header.num_scans);
	for (int scan = 0; scan < file_header.num_scans; scan++) {

		// Protocols
		fseek(f, file_header.entries[scan].offset, SEEK_SET);
		protocols[scan] = read_protocol(f);
		//parse_protocol(protocols[scan]);

		// Scan data
		/* TODO:
			this is bad because it is like the old mapVBVD, going through the whole memory
			just to get the pointers to the readout headers.

			Better approach for top level usage (all functions must set their position in file, how?):
			twix = twix_open(name); // Just opens the file and reads the header, nothing else
			// twix is a struct holding the file pointer, file_header and *protocols and *data from above
			twix_parse_protocol(twix); // reads the protocol and parses it, because why would you read it without using it? For users that want more control, there are still the low-level functions, how about exporting them? Would need to add twix_ to everyone of them. Also think about which data types need to be exported.
			twix_get_size(twix); // applied to the main twix struct
			twix_get_TE(twix);
			kspace = twix_get_kspace(twix); // reads in all the data, in which form? Check for the special ones not containing data.
			kspace = twix_filter_kspace(twix, filter_func()); // could also filter, filter_func can return 0 for ok, 1 for no, 2 for stop reading. What happens with the pointer structure then, being finished "half way"?
			twix_close(twix); // Frees memory and closes the file, internally can have function twix_free_data, twix_free_protocol etc which the user can also have available

			Better would be not to do this here, 
		*/
		size_t file_pos = file_header.entries[scan].offset + protocols[scan].length;
		fseek(f, file_pos, SEEK_SET);
		size_t num_bytes = file_header.entries[scan].len - protocols[scan].length;
		data[scan] = read_data(f, num_bytes);
		if (data[scan].hdrs == NULL) {
			printf("Error: skipping scan %d, too few bytes (%ld < %ld)\n", scan, num_bytes, sizeof(ReadoutHeader));
		}

		// ptr_scan_data->channel_hdr = (ChannelHeader *)malloc(readout_hdr->num_channels * sizeof(ChannelHeader*));
		// readout_hdr.num_channels, sizeof(ChannelHeader), sizeof(complex float) * readout_hdr.num_samples
		
	    // Apparently there is another readout registered at the end, no idea what syncdata is (in between meas?)
            //if self.is_flag_set('ACQEND') or self.is_flag_set('SYNCDATA'):
            //    return

		//while pos + 128 < scanEnd:  # fail-safe not to miss ACQEND
		//    fid.seek(pos, os.SEEK_SET)
		//    try:
		//	mdb = twixtools.mdb.Mdb(fid, version_is_ve)

		//    if not keep_syncdata_and_acqend:
		//	if mdb.is_flag_set('SYNCDATA'):
		//	    continue
		//	elif mdb.is_flag_set('ACQEND'):
		//	    break

		//    out[-1]['mdb'].append(mdb)

		//    if mdb.is_flag_set('ACQEND'):
		//	break

	}

	// TODO: Functionality to get scan data and sampling indices.

	// TODO: better functions for protocols

	// TODO: functionality to free protocol and other memory 

	// Note: eof is not reached yet, residual bytes are just zeros I think, why?

	return 0;
}

