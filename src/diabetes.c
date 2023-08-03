

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "utils.c"
#include "filetools.c"
#include "strtools.c"

typedef struct MeasurementHeader {
	uint32_t whatever; // Nice
	uint32_t len;
} MeasurementHeader;

typedef struct ScanHeader {
	uint32_t id;
	uint32_t fileid;
	uint32_t offset;
	uint32_t len;
	char patient_name[64];
	char protocol_name[64];
} ScanHeader;

typedef struct FileHeader {
	uint32_t whatever; // Nice
	uint32_t num;
	ScanHeader entries[64];
} FileHeader;

typedef struct Parameter {
	char type[12];
	char name[24];
	void *content;
} Parameter;

void read_twix_file(FILE* f, char** start, char** stop) {
	// Read name of config
	char name[48]; // is this length the actual max?
	{
		for (int j = 0; j < 48; j++) {
			char c = fgetc(f);
			check_file(f);
			name[j] = c;
			if (c == '\0') break;
		}
	}

	char *str;
	unsigned int len;
	safe_fread(&len, sizeof(unsigned int), f);
	str = malloc(len);
	safe_fread(str, len, f);

	// Check initial signature
	char signature[] = "<XProtocol>";
	size_t signature_length = sizeof(signature)-1;
	if (strncmp(str, signature, signature_length) != 0) {
		printf("Error: parameter string doesn't start as expected:\n");
		debug(str, signature_length);
		printf("\n");
		exit(1);
	}

	// Set new start and stop pointers of string
	*start = find('{', str, str+len-1) + 1;
	*stop = find('}', str+len-1, str) - 1;
	if (*start == NULL || *stop == NULL) {
		printf("Error: could not find enclosing braces of outer scope\n");
		exit(1);
	}
	return;
}

char* parse_parameter_name_type(Parameter* p, char* start, char* stop) {
	char signature[] = "<Param";
	size_t signature_length = sizeof(signature)-1;

	// Strip spaces
	start += strspn(start, " \n\r\t");

	if (stop-start <= signature_length || strncmp(start, signature, signature_length) != 0) {
		debug(start, signature_length);
		printf("Error: invalid parameter signature\n");
		printf("\n");
		exit(1);
	}
	start += signature_length;

	// Get type
	for (int i = 0; i < 12; i++) {
		char c = *start;
		start++;
		if (*start == '\0') {
			printf("Error: parse_parameter(): unexpected null byte read");
			exit(1);
		}
		if (c == '.') {
			p->type[i] = '\0';
			break;
		}
		p->type[i] = c;
	}

	// Get name
	start += 1; // *start might be '\0'
	char* closing_quote = find('\"', start, stop);
	if (closing_quote == NULL) {
		printf("Error: parse_parameter(): closing quote not found\n");
		exit(1);
	}
	size_t diff = closing_quote - start;
	if (diff > 0) {
		strncpy(p->name, start, diff);
		p->name[diff+1] = '\0';
	}
	else {
		p->name[0] = '\0';
	}
	printf("%s\n", p->name);
	return start + 2; // ">
}

// gets {...}
void parse_parameter_content(Parameter* p, char* start, char* stop) {

	start = find('{', start, stop) + 1;
	stop = find('}', stop, start) - 1;
	if (start == NULL || stop == NULL) {
		printf("Error: could not find enclosing braces of parameter scope (%s)\n", p->name);
		exit(1);
	}

	if (p->type == "MAP") {
		char* next_start = start;
		Parameter* list ....;
		need some simple append functionality here
		size_t i = 0;
		while (next_start < start) {
			next_start = find('<', next_start, stop);
			if (next_start == NULL) break;
			next_start = parse_parameter_name_type(&(list[i]), next_start, stop);
			if (next_start == NULL) break;
			append parameter to list
		}
		store list in p contents
	}
	else {
		// Other parameter types
	}
	return;
}



int main(int argc, char* argv[]) {
	
	FILE *f;
	if (argc < 2) {
		printf("Error: no filename given\n");
		return 1;
	}
	f = fopen(argv[1], "rb");

	// Get filesize, needed?
	fseek(f, 0, SEEK_END);
	uint64_t filesize = ftell(f);
	fseek(f, 0, SEEK_SET);

	// Check version
	{
		uint32_t v[2];
		safe_fread(&v, sizeof(v), f);
		if (v[0] >= 10000 || v[1] > 64) {
			printf("Error: old unsupported twix version\n");
			return 1;
		}
	}
	fseek(f, 0, SEEK_SET);

	FileHeader file_header;
	safe_fread(&file_header, sizeof(file_header), f);

	for (int i = 0; i < file_header.num; i++) {
		uint64_t position = file_header.entries[i].offset;
		fseek(f, position, SEEK_SET);
		MeasurementHeader measurement_header;
		safe_fread(&measurement_header, sizeof(MeasurementHeader), f);

		char *start, *stop;
		read_twix_file(f, &start, &stop);
		Parameter p;
		start = parse_parameter_name_type(&p, start, stop);
		debug(start, 1);

		//_, n_buffer = np.fromfile(file, dtype=np.uint32, count=2)
		//xprotocol = dict()
		//pattern = br'(\w{4,})\x00(.{4})'
		//pos = file.tell()
		//for _ in range(n_buffer):
		//	tmp = file.read(48)
		//	matches = re.search(pattern, tmp, re.DOTALL)
		//	name = matches.group(1).decode('latin1')
		//	buf_len = struct.unpack('<I', matches.group(2))[0]
		//	pos += len(matches.group())
		//	file.seek(pos)
		//	buf = file.read(buf_len).decode('latin1')
		//	xprotocol[name] = parse_buffer(buf)
		//	xprotocol["{}_raw".format(name)] = buf
		//	pos += buf_len

		//return xprotocol


		//char *asd = malloc(200);
		//safe_fread(asd, 200, f);
		//for (int j = 0; j < 200; j++) {
		//	printf("%c", asd[j]);
		//}
		//exit(1);

		//buf_len = struct.unpack('<I', matches.group(2))[0]
		//pos += len(matches.group())
		//file.seek(pos)
		//buf = file.read(buf_len).decode('latin1')
		//xprotocol[name] = parse_buffer(buf)
		//pos += buf_len

		//char *asd = malloc(20000);
		//fread(asd, 20000, 1, f);



		//xprotocol = dict()
		//pattern = br'(\w{4,})\x00(.{4})'
		//pos = file.tell()
		//for _ in range(n_buffer):
		//	tmp = file.read(48)
		//	matches = re.search(pattern, tmp, re.DOTALL)
		//	name = matches.group(1).decode('latin1')
		//	buf_len = struct.unpack('<I', matches.group(2))[0]
		//	pos += len(matches.group())
		//	file.seek(pos)
		//	buf = file.read(buf_len).decode('latin1')
		//	xprotocol[name] = parse_buffer(buf)
		//	xprotocol["{}_raw".format(name)] = buf
		//	pos += buf_len

		}

	/*

	- for entry in fileheader entries
        seek(offset of scan, from file header)
		read MeasurementHeader

		- parse protocol
            seek(offset of meas from above)
			- twixprot.parse_twix_hdr(fid)
				read two uint32, second is num_buffers
				pattern = br'(\w{4,})\x00(.{4})'
				pos = file.tell()
				for _ in range(n_buffer):
					tmp = file.read(48)
					matches = re.search(pattern, tmp, re.DOTALL)
					name = matches.captures[1]
					buf_len = struct.unpack('<I', matches.group(2))[0]
					pos += len(matches.group())
					file.seek(pos)
					buf = file.read(buf_len).decode('latin1')
					xprotocol[name] = parse_buffer(buf)
					xprotocol["{}_raw".format(name)] = buf
					pos += buf_len

            seek(offset of meas from above)

            out[-1]['hdr_str'] = np.fromfile(fid, dtype="<S1", count=hdr_len)




	*/


    //for s in range(NScans):
    //    if include_scans is not None and s not in include_scans:
    //        # skip scan if it is not requested
    //        continue

    //    if version_is_ve:
    //        out[-1]['raidfile_hdr'] = raidfile_hdr['entry'][s]

    //    if parse_geometry:
    //        out[-1]['geometry'] = twixtools.geometry.Geometry(out[-1])

    //    # if data is not requested (headers only)
    //    if not parse_data:
    //        continue

    //    pos = measOffset[s] + np.uint64(hdr_len)

    //    if verbose:
    //        print('Scan ', s)
    //        progress_bar = tqdm(total=scanEnd - pos, unit='B', unit_scale=True, unit_divisor=1024)
    //    while pos + 128 < scanEnd:  # fail-safe not to miss ACQEND
    //        fid.seek(pos, os.SEEK_SET)
    //        try:
    //            mdb = twixtools.mdb.Mdb(fid, version_is_ve)
    //        except ValueError:
    //            print(f"WARNING: Mdb parsing encountered an error at file position {pos}/{scanEnd}, stopping here.")

    //        # jump to mdh of next scan
    //        pos += mdb.dma_len
    //        if verbose:
    //            progress_bar.update(mdb.dma_len)

    //        if not keep_syncdata_and_acqend:
    //            if mdb.is_flag_set('SYNCDATA'):
    //                continue
    //            elif mdb.is_flag_set('ACQEND'):
    //                break

    //        out[-1]['mdb'].append(mdb)

    //        if mdb.is_flag_set('ACQEND'):
    //            break

    //    if verbose:
    //        progress_bar.close()

    //fid.close()

	return 0;
}

