
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



int main(int argc, char* argv[])
{
	FILE *f;
	if (argc < 2) {
		printf("Error: no filename given\n");
		exit(1);
	}
	f = fopen(argv[1], "rb");

	FileHeader file_header;
	safe_fread(f, &file_header, sizeof(file_header));
	check_file_header(file_header);

	//print_file_header(file_header);


	// TODO: here create arrays holding the protocols and data of every scan for the user to have
	for (int scan = 0; scan < file_header.num_scans; scan++) {

		protocols = parse_protocols();

		---
		ProtocolHeader protocol_header;
		fseek(f, file_header.entries[scan].offset, SEEK_SET);
		safe_fread(f, &protocol_header, sizeof(ProtocolHeader));

		Parameter *protocols = (Parameter*) calloc(protocol_header.num_protocols, sizeof(Parameter));
		for (int p = 0; p < protocol_header.num_protocols; p++) {
			// Read protocol into string
			char *start, *stop;
			char *name = (char*) &(protocols[p].name);
			read_protocol(f, name, &start, &stop);
			// Parse protocol
			if (strcmp(name, "Meas") == 0) {
				start = find('{', start+1, stop);
				start = find_matching(start, '}');
				start = find('<', start, stop);
			}
			else if (strcmp(name, "MeasYaps") == 0) {
				// TODO
				continue;
			}
			else if (strcmp(name, "Phoenix") == 0) {
				// TODO: worth it?
				continue;
			}
			strcpy(protocols[p].type, "ParamMap");
			parse_parameter_content(&(protocols[p]), start, stop, parse_type | parse_content);
		}
		---


		// DATA ----------------------------

		// Read in scan data
		size_t file_pos = file_header.entries[scan].offset + protocol_header.length; // TODO: might not need var
		size_t num_bytes = file_header.entries[scan].len - protocol_header.length;
		//char *asd = malloc(14);
		//safe_fread(f, asd, 14);
		//debug(asd, 14);
		//printf("File pos %ld %ld %ld %ld\n", ftell(f), file_pos, num_bytes, file_header.entries[scan].offset);
		//return 0;
		char *scan_buffer = (char *)malloc(num_bytes);
		fseek(f, file_pos, SEEK_SET);
		safe_fread(f, scan_buffer, num_bytes);
		// Parse scan data
		ScanData scan_data;
		scan_data.block = -1;
		scan_data.index = SCANDATABLOCKLEN;
		char *pos = scan_buffer;
		if (num_bytes < sizeof(ReadoutHeader)) {
			printf("Error: scan %d has too few bytes (%ld < %ld)\n", scan, num_bytes, sizeof(ReadoutHeader));
			return 1;
		}
		while (pos < scan_buffer + num_bytes) {
			if (scan_data.index == SCANDATABLOCKLEN) {
				scan_data.index = 0;
				scan_data.block += 1;
				scan_data.hdrs = (ReadoutHeader ***)reallocarray(
					scan_data.hdrs,
					scan_data.block+1,
					sizeof(ReadoutHeader**)
				);
				scan_data.hdrs[scan_data.block] = (ReadoutHeader **)malloc(
					sizeof(ReadoutHeader*) * SCANDATABLOCKLEN
				);
			}
			ReadoutHeader *readout_hdr = (ReadoutHeader *)pos;
			//print_readout_header(readout_hdr);
			scan_data.hdrs[scan_data.block][scan_data.index] = readout_hdr;
			scan_data.index += 1;
			pos += get_readout_num_bytes(readout_hdr);
		}
		scan_data.hdrs[scan_data.block] = (ReadoutHeader **)reallocarray(
			scan_data.hdrs[scan_data.block],
			scan_data.index,
			sizeof(ReadoutHeader*)
		);
		printf("%d %d\n", scan_data.block, scan_data.index);
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

