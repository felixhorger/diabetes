
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <search.h>
#include "utils.c"
#include "filetools.c"
#include "strtools.c"

typedef struct ScanHeader { // Could be reference, actual scan, etc
	uint32_t id;
	uint32_t fileid;
	uint64_t offset;
	uint64_t len;
	char patient_name[64];
	char protocol_name[64];
} ScanHeader;

typedef struct FileHeader { // first thing to read in
	uint32_t version;
	uint32_t num;
	ScanHeader entries[64];
} FileHeader;

typedef struct ProtocolHeader { // Tells how many protocol blocks there are
	uint32_t len;
	uint32_t num;
} ProtocolHeader;

/*
	It is further quite striking that it is quite specific for Cartesian imaging.
	I assume this has historic reasons, and now it is what it is, fair enough.
	However, it is outrageous that the description of scans is so strongly entangled with
	a standard format for raw data. To give a couple of examples, there are flags related to heart beats,
	BLADE sampling, or partial Fourier.
	Since it is difficult to define a fixed standard format that includes all relevant information
	for current and future types of scans, the separation of raw-data and information about the
	applied technique follows logically.
*/
#define EVALINFOMASK_ACQEND			1 << 0  // Last readout in scan
#define EVALINFOMASK_RTFEEDBACK			1 << 1  // Realtime feedback
#define EVALINFOMASK_HPFEEDBACK			1 << 2  // High perfomance feedback
#define EVALINFOMASK_ONLINE			1 << 3  // Online processing for recon?
#define EVALINFOMASK_OFFLINE			1 << 4  // Offline processing (question: doesn't !offline -> online?)
#define EVALINFOMASK_SYNCDATA			1 << 5  // Readout contains synchroneous data (synchroneous?)
#define EVALINFOMASK_UNNAMED6			1 << 6
#define EVALINFOMASK_UNNAMED7			1 << 7
#define EVALINFOMASK_LASTSCANINCONCAT		1 << 8  // Last scan in concatenation (I love the seqloop)
#define EVALINFOMASK_UNNAMED9			1 << 9
#define EVALINFOMASK_RAWDATACORRECTION		1 << 10 // Use raw data correction factor
#define EVALINFOMASK_LASTSCANINMEAS		1 << 11 // Last scan in measurement (nice ordering of flags, well done)
#define EVALINFOMASK_SCANSCALEFACTOR		1 << 12 // Use scan specific additional scale (I guess similar to raw data correction factor)
#define EVALINFOMASK_SECONDHADAMARPULSE		1 << 13 // Second RF exitation of Hadamar pulse
#define EVALINFOMASK_REFPHASESTABSCAN		1 << 14 // Reference phase stabilization scan
#define EVALINFOMASK_PHASESTABSCAN		1 << 15 // Phase stabilization scan
#define EVALINFOMASK_D3FFT			1 << 16 // Execute 3D FFT
#define EVALINFOMASK_SIGNREV			1 << 17 // Sign reversal
#define EVALINFOMASK_PHASEFFT			1 << 18 // Execute phase fft
#define EVALINFOMASK_SWAPPED			1 << 19 // Swapped phase and readout direction
#define EVALINFOMASK_POSTSHAREDLINE		1 << 20 // Shared line
#define EVALINFOMASK_PHASCOR			1 << 21 // Phase correction data
#define EVALINFOMASK_PATREFSCAN			1 << 22 // Additional scan for partial Fourier reference line/partition
#define EVALINFOMASK_PATREFANDIMASCAN		1 << 23 // Partial Fourier reference that is also used as image scan
#define EVALINFOMASK_REFLECT			1 << 24 // Reflect line (along readout or what?)
#define EVALINFOMASK_NOISEADJSCAN		1 << 25 // Noise adjust scan
#define EVALINFOMASK_SHARENOW			1 << 26 // Lines (readouts?) may be shared between e.g. phases
#define EVALINFOMASK_LASTMEASUREDLINE		1 << 27 // Last phase (lines) which is measured, I guess
#define EVALINFOMASK_FIRSTSCANINSLICE		1 << 28 // First scan in slice
#define EVALINFOMASK_LASTSCANINSLICE		1 << 29 // Last scan in slice
#define EVALINFOMASK_TREFFECTIVEBEGIN		1 << 30 // Effective TR begin (triggered from realtime feedback?)
#define EVALINFOMASK_TREFFECTIVEEND		1 << 31 // Effective TR end (also triggered)
#define EVALINFOMASK_REF_POSITION		1 << 32 // Reference position for (patient?) movement
#define EVALINFOMASK_SLC_AVERAGED		1 << 33 // Averaged slice
#define EVALINFOMASK_TAGFLAG1			1 << 34 // Adjust scans
#define EVALINFOMASK_CT_NORMALIZE		1 << 35 // Correction scan for TimCT prescan normalise
#define EVALINFOMASK_SCAN_FIRST			1 << 36 // First scan of a particular map
#define EVALINFOMASK_SCAN_LAST			1 << 37 // Last scan of a particular map
#define EVALINFOMASK_SLICE_ACCEL_REFSCAN        1 << 38 // Single-band reference scan for multi-band
#define EVALINFOMASK_SLICE_ACCEL_PHASCOR	1 << 39 // Additional phase correction in multi-band
#define EVALINFOMASK_FIRST_SCAN_IN_BLADE	1 << 40 // The first line of a blade
#define EVALINFOMASK_LAST_SCAN_IN_BLADE		1 << 41 // The last line of a blade
#define EVALINFOMASK_LAST_BLADE_IN_TR		1 << 42 // Is true for all lines of last BLADE in each TR
#define EVALINFOMASK_UNNAMED43			1 << 43
#define EVALINFOMASK_PACE			1 << 44 // PACE scan
#define EVALINFOMASK_RETRO_LASTPHASE		1 << 45 // Last phase in a heartbeat
#define EVALINFOMASK_RETRO_ENDOFMEAS		1 << 46 // Readout is at end of measurement
#define EVALINFOMASK_RETRO_REPEATTHISHEARTBEAT	1 << 47 // Repeat the current heartbeat
#define EVALINFOMASK_RETRO_REPEATPREVHEARTBEAT	1 << 48 // Repeat the previous heartbeat
#define EVALINFOMASK_RETRO_ABORTSCANNOW		1 << 49 // Abort everything
#define EVALINFOMASK_RETRO_LASTHEARTBEAT	1 << 50 // Readout is from last heartbeat (a dummy)
#define EVALINFOMASK_RETRO_DUMMYSCAN		1 << 51 // Readout is just a dummy scan, throw it away
#define EVALINFOMASK_RETRO_ARRDETDISABLED	1 << 52 // Disable all arrhythmia detection
#define EVALINFOMASK_B1_CONTROLLOOP		1 << 53 // Use readout for B1 Control Loop
#define EVALINFOMASK_SKIP_ONLINE_PHASCOR	1 << 54 // Do not correct phases online
#define EVALINFOMASK_SKIP_REGRIDDING		1 << 55 // Do not regrid
#define EVALINFOMASK_MDH_VOP			1 << 56 // VOP based RF monitoring
#define EVALINFOMASK_UNNAMED57			1 << 57
#define EVALINFOMASK_UNNAMED58			1 << 58
#define EVALINFOMASK_UNNAMED59			1 << 59
#define EVALINFOMASK_UNNAMED60			1 << 60
#define EVALINFOMASK_WIP_1			1 << 61 // WIP application type 1
#define EVALINFOMASK_WIP_2			1 << 62 // WIP application type 2
#define EVALINFOMASK_WIP_3			1 << 63 // WIP application type 3

typedef struct ReadoutHeader {
	uint32_t	dma_length_and_flags; // What is dma? First 25 bits are length, last 7 are flags. Conversion via dma_length_and_flags % (1 << 25) I think
	int32_t		meas_uid;
	uint32_t	scan_counter;
	uint32_t	time_stamp;
	uint32_t	pmu_timestamp;
	uint16_t	system_type;
	uint16_t	patient_table_pos_delay;
	int32_t		patient_table_pos_x;
	int32_t		patient_table_pos_y;
	int32_t		patient_table_pos_z;
	int32_t		reserved;
	uint64_t	eval_info_mask;
	uint16_t	num_samples;
	uint16_t	num_channels;
        uint16_t	line;
        uint16_t	average;
        uint16_t	slice;
        uint16_t	partition;
        uint16_t	echo;
        uint16_t	phase;
        uint16_t	repetition;
        uint16_t	set;
        uint16_t	segment;
        uint16_t	a;
        uint16_t	b;
        uint16_t	c;
        uint16_t	d;
        uint16_t	e;
        uint16_t	cutoff[2];
	uint16_t	center_col;
	uint16_t	coil_select;
	float		readout_offcentre;
	uint32_t	time_since_last_rf;
	uint16_t	center_line;
	uint16_t	center_partition;
        float		position[3]; // Patient coordinates, i.e. [sag, cor, tra]
        float		quaternion[4]; // I guess normal and in-plane rotation
	uint16_t	ice[24];
	uint16_t	also_reserved[4];
	uint16_t	application_counter;
	uint16_t	application_mask;
	uint32_t	crc;
} ReadoutHeader;

typedef struct ChannelHeader {
	uint32_t	type_and_length;
	int32_t		meas_uid;
	uint32_t	scan_counter;
	int32_t		reserved;
	uint32_t	sequence_time;
	uint32_t	unused;
	uint16_t	id;
	uint16_t	also_unused;
	uint32_t	crc;
} ChannelHeader;

#define SCANDATABLOCKLEN 128
typedef struct ScanData {
	ReadoutHeader ***hdrs;
	int32_t block;
	int32_t index;
} ScanData;

size_t get_readout_num_bytes(ReadoutHeader *readout_hdr) {
	return readout_hdr->dma_length_and_flags % (1 << 25);
}



void print_readout_header(ReadoutHeader *hdr)
{
	printf(
		"dma_length_and_flags       %u\n"
		"meas_uid                   %d\n"
		"scan_counter               %u\n"
		"time_stamp                 %u\n"
		"pmu_timestamp              %u\n"
		"system_type                %hu\n"
		"patient_table_pos_delay    %hu\n"
		"patient_table_pos_x        %d\n"
		"patient_table_pos_y        %d\n"
		"patient_table_pos_z        %d\n"
		"reserved                   %d\n"
		"eval_info_mask             %lu\n"
		"num_samples                %hu\n"
		"num_channels               %hu\n"
		"line                       %hu\n"
		"average                    %hu\n"
		"slice                      %hu\n"
		"partition                  %hu\n"
		"echo                       %hu\n"
		"phase                      %hu\n"
		"repetition                 %hu\n"
		"set                        %hu\n"
		"segment                    %hu\n"
		"a                          %hu\n"
		"b                          %hu\n"
		"c                          %hu\n"
		"d                          %hu\n"
		"e                          %hu\n"
		"cutoff[2]                 [%hu, %hu]\n"
		"center_col                 %hu\n"
		"coil_select                %hu\n"
		"readout_offcentre          %f\n"
		"time_since_last_rf         %u\n"
		"center_line                %hu\n"
		"center_partition           %hu\n"
		"position[3]               [%6f, %6f, %6f]\n"
		"quaternion[4]             [%6f, %6f, %6f, %6f]\n"
		"ice[24]                   [%12hu, %12hu, %12hu, %12hu,\n"
		"                           %12hu, %12hu, %12hu, %12hu,\n"
		"                           %12hu, %12hu, %12hu, %12hu,\n"
		"                           %12hu, %12hu, %12hu, %12hu,\n"
		"                           %12hu, %12hu, %12hu, %12hu,\n"
		"                           %12hu, %12hu, %12hu, %12hu]\n"
		"also_reserved[4]          [%hu, %hu, %hu, %hu]\n"
		"application_counter        %hu\n"
		"application_mask           %hu\n"
		"crc                        %u\n",
		hdr->dma_length_and_flags,
		hdr->meas_uid,
		hdr->scan_counter,
		hdr->time_stamp,
		hdr->pmu_timestamp,
		hdr->system_type,
		hdr->patient_table_pos_delay,
		hdr->patient_table_pos_x,
		hdr->patient_table_pos_y,
		hdr->patient_table_pos_z,
		hdr->reserved,
		hdr->eval_info_mask,
		hdr->num_samples,
		hdr->num_channels,
		hdr->line,
		hdr->average,
		hdr->slice,
		hdr->partition,
		hdr->echo,
		hdr->phase,
		hdr->repetition,
		hdr->set,
		hdr->segment,
		hdr->a,
		hdr->b,
		hdr->c,
		hdr->d,
		hdr->e,
		hdr->cutoff[0], hdr->cutoff[1],
		hdr->center_col,
		hdr->coil_select,
		hdr->readout_offcentre,
		hdr->time_since_last_rf,
		hdr->center_line,
		hdr->center_partition,
		hdr->position[0], hdr->position[1], hdr->position[2],
		hdr->quaternion[0], hdr->quaternion[1], hdr->quaternion[2], hdr->quaternion[3],
		hdr->ice[0],  hdr->ice[1],  hdr->ice[2],  hdr->ice[3],
		hdr->ice[4],  hdr->ice[5],  hdr->ice[6],  hdr->ice[7],
		hdr->ice[8],  hdr->ice[9],  hdr->ice[10], hdr->ice[11],
		hdr->ice[12], hdr->ice[13], hdr->ice[14], hdr->ice[15],
		hdr->ice[16], hdr->ice[17], hdr->ice[18], hdr->ice[19],
		hdr->ice[20], hdr->ice[21], hdr->ice[22], hdr->ice[23],
		hdr->also_reserved[0], hdr->also_reserved[1], hdr->also_reserved[2], hdr->also_reserved[3],
		hdr->application_counter,
		hdr->application_mask,
		hdr->crc
	);
}


#define PARAMETER_NAME_LEN 64
#define PARAMETER_TYPE_LEN 16

typedef struct Parameter {
	char type[PARAMETER_TYPE_LEN];
	char name[PARAMETER_NAME_LEN];
	void *content;
} Parameter;

struct ParameterList;
typedef struct ParameterList {
	struct ParameterList* next;
	struct ParameterList* prev;
	Parameter p;
} ParameterList;

void read_protocol(FILE* f, char *name, char** start, char** stop) {
	// Read config
	// TODO: what is it useful for?
	for (int j = 0; j < 48; j++) {
		char c = fgetc(f);
		check_file(f);
		name[j] = c;
		if (c == '\0') break;
	}

	// Read protocol into string
	char *str;
	uint32_t len;
	safe_fread(&len, sizeof(uint32_t), f);
	str = malloc(len);
	safe_fread(str, len, f);

	if (strcmp(name, "MeasYaps") == 0) {
		// Check initial signature
		char signature[] = "### ASCCONV BEGIN ";
		size_t signature_length = sizeof(signature)-1;
		if (strncmp(str, signature, signature_length) != 0) {
			printf("Error: parameter string doesn't start as expected:\n");
			debug(str, signature_length);
			printf("\n");
			exit(1);
		}
		*start = strstr(str + signature_length, " ###") + 4;
		*stop = *start + len - sizeof("### ASCCONV END ###") - 132; // Mysterious offset :(
	}
	else {
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
		*start = find('{', str, str+len-1);
		*stop = find('}', str+len-2, str);
		if (*start == NULL || *stop == NULL) {
			printf("Error: could not find enclosing braces of outer scope\n");
			exit(1);
		}
	}
	return;
}

void check_parameter(Parameter *p, char *type) {
	if (p == NULL) {
		printf("Error: parameter is NULL pointer\n");
		exit(1);
	}
	if (strncmp(p->type, type, PARAMETER_TYPE_LEN) != 0) {
		printf("Error: wrong parameter type\n");
		exit(1);
	}
	return;
}
double get_double_parameter(Parameter *p) {
	check_parameter(p, "ParamDouble");
	return *((double *)&(p->content));
}
long get_long_parameter(Parameter *p) {
	check_parameter(p, "ParamLong");
	return *((long *)&(p->content));
}
bool get_bool_parameter(Parameter *p) {
	check_parameter(p, "ParamBool");
	return *((bool *)&(p->content));
}
char *get_string_parameter(Parameter *p) {
	check_parameter(p, "ParamString");
	return (char *) p->content;
}

void print_parameter(Parameter *p) {
	if (p == NULL) {
		printf("Error: NULL pointer given\n");
		exit(1);
	}
	printf("%s %s", p->type, p->name);
	if (strcmp(p->type, "ParamDouble") == 0) printf(" = %lf", get_double_parameter(p));
	else if (strcmp(p->type, "ParamLong") == 0) printf(" = %li", get_long_parameter(p));
	else if (strcmp(p->type, "ParamBool") == 0) printf(" = %c", get_bool_parameter(p));
	else if (strcmp(p->type, "ParamString") == 0) printf(" = %s", get_string_parameter(p));
	printf("\n");
	return;
}

Parameter* index_parameter_map(Parameter *p, int i) {
	if (p == NULL) {
		printf("Error: NULL pointer given\n");
		exit(1);
	}
	if (strcmp(p->type, "ParamMap") != 0) {
		printf("Error: not a ParamArray\n");
		exit(1);
	}
	ParameterList *list = (ParameterList *) p->content;
	if (i == -1) return &(list->p); // Special, because first element is used for type
	// All others follow
	for (int j = -1; j < i; j++) {
		list = list->next;
		if (list == NULL) {
			printf("Error: parameter array index %d out of bounds\n", i);
			exit(1);
		}
	}
	return &(list->p);
}

Parameter* index_parameter_array(Parameter *p, int i) {
	if (p == NULL) {
		printf("Error: NULL pointer given\n");
		exit(1);
	}
	if (strcmp(p->type, "ParamArray") != 0) {
		printf("Error: not a ParamArray\n");
		exit(1);
	}
	Parameter *p_array = (Parameter *) p->content;
	return index_parameter_map(p_array, i);
}

char* parse_parameter_name_type(Parameter* p, char* start, char* stop) {

	// Strip spaces
	start += strspn(start, " \n\r\t") + 1; // +1 because of <

	// Parse type
	int i;
	//debug(start, stop-start);
	for (i = 0; i < PARAMETER_TYPE_LEN; i++) {
		char c = *start;
		start++;
		if (*start == '\0') {
			printf("Error: unexpected null byte read");
			exit(1);
		}
		if (c == '.') {
			p->type[i] = '\0';
			break;
		}
		p->type[i] = c;
	}
	if (i == PARAMETER_TYPE_LEN) {
		printf("Error: parameter type unexpectedly long:\n");
		debug(p->type, PARAMETER_TYPE_LEN);
		exit(1);
	}

	// Get name
	start += 1;
	char* closing_quote = find('\"', start, stop);
	if (closing_quote == NULL) {
		printf("Error: parse_parameter(): closing quote not found\n");
		exit(1);
	}
	size_t diff = closing_quote - start;
	if (diff > 0) {
		strncpy(p->name, start, diff);
		p->name[diff] = '\0';
	}
	else {
		p->name[0] = '\0';
	}
	return start + 2; // ">
}

void copy_parameter_type(Parameter *dest, Parameter *src) {
	strcpy(dest->name, src->name);
	strcpy(dest->type, src->type);
	if (strcmp(src->type, "ParamMap") == 0) {
		ParameterList *list = (ParameterList*) calloc(1, sizeof(ParameterList));
		dest->content = (void*) list;
		ParameterList *src_list = (ParameterList*) src->content;
		while (true) {
			//print_parameter(&(src_list->p));
			// Copy list entry
			copy_parameter_type(&(list->p), &(src_list->p));
			// Advance source list
			src_list = src_list->next;
			if (src_list == NULL) break;
			// Add new element to destination list and advance to it
			list->next = (ParameterList*) calloc(1, sizeof(ParameterList));
			list->next->prev = list;
			list = list->next;
		}
	}
	else if (strcmp(src->type, "ParamArray") == 0) {
		Parameter *p_array = (Parameter*) calloc(1, sizeof(Parameter));
		strcpy(p_array->type, "ParamMap");
		dest->content = (void *) p_array;
		copy_parameter_type(p_array, (Parameter*)src->content);
	}
	else {
		dest->content = NULL;
	}
}

enum parse_mode {parse_type = 1, parse_content = 2, copy_type = 4};

void parse_parameter_content();

void parse_parameter_list(Parameter *p, char *start, char *stop, enum parse_mode mode) {
	// Set up list of parameters
	ParameterList *list;
	if (mode & parse_type) {
		// Make new list, names and types are to be filled in
		list = (ParameterList*) calloc(1, sizeof(ParameterList));
		p->content = (void*) list;
	}
	else {
		// For ParamArray, names and types have been filled already (first element)
		list = (ParameterList*) p->content;
	}
	Parameter* current_p;

	// Append all entries to list
	while (start < stop) {
		// Parse parameter name and type in case of map, or copy type for array
		if (mode & parse_type) { // Name and type are to be determined (map, or array type determination)
			// Find start of parameter signature
			bool not_found = true;
			while (start < stop) {
				char *next_brace = find('{', start, stop);
				char *next_type = find('<', start, stop);
				if (next_brace != NULL && next_type > next_brace) {
					printf("Error: did not find a valid type\n");
					debug(start, min(128, next_brace-start-1));
					exit(1);
				}
				start = next_type;
				if (start == NULL) return;
				if (strncmp(start+1, "Param", 5) == 0 || strncmp(start+1, "Pipe", 4) == 0) {
					not_found = false;
					break;
				}
				else if (
					strncmp(start+1, "Event", 5) == 0 ||
					strncmp(start+1, "Connection", 10) == 0 ||
					strncmp(start+1, "Method", 6) == 0 ||
					strncmp(start+1, "Dependency", 10) == 0 ||
					strncmp(start+1, "ProtocolComposer", 16) == 0
				) {
					start = find_matching(next_brace, '}') + 1;
					continue;
				}
				else if (strncmp(start+1, "Visible", 7) == 0) {
					// TODO: is the necessary?
					start = find('\n', start, stop) + 1;
					//debug(start, 20);
					continue;
				}
				start += 1;
			}
			if (not_found) return;

			// Make new list entry
			list->next = (ParameterList*) calloc(1, sizeof(ParameterList));
			list->next->prev = list;
			list = list->next;
			current_p = &(list->p);

			// Parse name
			start = parse_parameter_name_type(current_p, start, stop);
			if (start == NULL) {
				printf("Error: parameter with unexpected name or type\n");
				exit(1);
			}
		}
		else if (mode & copy_type) { // Array, need to copy type before parsing content
			list->next = (ParameterList*) calloc(1, sizeof(ParameterList));
			list->next->prev = list;
			list = list->next;
			current_p = &(list->p);
			copy_parameter_type(current_p, &(list->prev->p));
			//printf("copied %s\n", current_p->type);
			//print_parameter(index_parameter_array(current_p, 0));
		}
		else { // Map, advance by first list element
			list = list->next;
			if (list == NULL) {
				printf("Error: ParamArray type definition inconsistent with value \n");
				exit(1);
			}
			current_p = &(list->p);
		}

		// Find enclosing scope
		start = find('{', start, stop);
		if (start == NULL) {
			if (mode & parse_type) {
				printf("Error: parameter without content\n");
				exit(1);
			}
			else return;
		}
		char* closing_brace = find_matching(start, '}');

		// Parse content
		// Content of a map is parsed no matter if ParamArray or not
		parse_parameter_content(current_p, start, closing_brace, mode & (~copy_type));
		//printf("content parsed %s\n", current_p->type);
		start = closing_brace + 1;
		start += strspn(start, " \n\r\t");
	}
	return;
}

// gets {...}
void parse_parameter_content(Parameter* p, char* start, char* stop, enum parse_mode mode) {
	if (start == NULL || stop == NULL) {
		p->name[PARAMETER_NAME_LEN] = '\0';
		printf("Error: parsing parameter %s: start (%p) or stop (%p) is NULL\n", p->name, start, stop);
		exit(1);
	}

	//printf("%s %s %d\n", p->name, p->type, mode);
	if ((mode & copy_type) == 0) { // is copy_type off?
		/*	Problem is that with normal ParamMap, there are extra curly braces
			But with ParamMap used for the ParamArray there aren't.
			So, they are only to be removed if not parsing the contents of
			a ParamArray via a ParamMap, i.e. if copy_type is off.
		*/
		start = find('{', start, stop);
		stop = find('}', stop, start);
		//debug(start, stop-start);
		if (start == NULL || stop == NULL) {
			printf("Error: parsing %s: could not find enclosing braces of parameter scope\n", p->name);
			exit(1);
		}
		start += 1;
		stop -= 1;
	}

	//printf("%s %s\n", p->name, p->type);
	if (strcmp(p->type, "ParamMap") == 0 || strcmp(p->type, "Pipe") == 0) {
		parse_parameter_list(p, start, stop, mode);
		//printf("%s %s\n", p->name, p->type);
	}
	else if (strcmp(p->type, "ParamArray") == 0) {
		// TODO: The below line is for cases where a { } is enough to signify an empty array/map instead of { {} {} ...}
		if (strcmp(p->name, "RxCoilSelects") == 0 || strcmp(p->name, "aRxCoilSelectData") == 0) return;
		// Parse the type signature
		if (mode & parse_type) {
			// Find type signature
			// TODO: this breaks if it isn't last in the list
			char signature[] = "Default> ";
			size_t signature_length = sizeof(signature)-1;
			bool not_found = true;
			while (start < stop) {
				start = find('<', start, stop);
				// TODO check for NULL
				if (strncmp(start+1, signature, signature_length) == 0) {
					not_found = false;
					break;
				}
				start += sizeof(signature);
			}
			if (not_found) {
				printf("Error: parameter array with unexpected signature\n");
				exit(1);
			}
			start += sizeof(signature);
		}

		// Find matching closing brace
		char *type_opening_brace = find('{', start+1, stop);
		char *type_closing_brace = find_matching(type_opening_brace, '}');

		// Parsing type of ParamArray
		if (mode & parse_type) {
			// The elements of the ParamArray will be loaded into a ParamMap, which is set up here

			// Create ParamMap with list in contents
			Parameter *p_array = (Parameter*) calloc(1, sizeof(Parameter));
			strcpy(p_array->type, "ParamMap");
			p_array->name[0] = '\0'; // No name necessary
			ParameterList *list = (ParameterList *) calloc(1, sizeof(ParameterList));
			p_array->content = (void *) list;

			// Reference this parameter in contents of p
			p->content = (void *) p_array;

			// Parse array type and put into the ParamMap list
			Parameter* p_array_element = &(list->p);
			start = parse_parameter_name_type(p_array_element, start, type_closing_brace);
			parse_parameter_content(p_array_element, start, type_closing_brace, parse_type);
		}

		// Advance pointer to after the ParamArray type definition
		// This is only the case for the parent ParamArray
		if (mode & parse_type && mode & parse_content) start = type_closing_brace + 1;

		// Parse content
		if (mode & parse_content) { // ParamArray inside a ParamArray
			// Parse the ParamArray contents, but also need to copy type
			parse_parameter_content((Parameter *)p->content, start, stop, parse_content | copy_type);
		}
		//printf("%s %s\n", p->name, p->type);
	}
	else if (strcmp(p->type, "ParamFunctor") == 0 || strcmp(p->type, "PipeService") == 0) {
		if (mode & parse_type) {
			// Check signature
			start = find('<', start, stop) + 1;
			char signature[] = "Class> ";
			size_t signature_length = sizeof(signature)-1;
			if (strncmp(start, signature, signature_length) != 0) {
				printf("Error: functor with unexpected signature\n");
				debug(start, sizeof(signature));
			}
			start += sizeof(signature)-1;

			// Create ParamMap with list in contents
			Parameter *p_functor = (Parameter*) calloc(1, sizeof(Parameter));
			strcpy(p_functor->type, "ParamMap");
			// Reference this parameter in contents of p
			p->content = (void *) p_functor;

			// Parse functor name
			{
				start = find('"', start, stop);
				start += 1;
				char *closing_quote = find('"', start, stop);
				// TODO: check for null start and closing
				size_t len = closing_quote-start;
				if (len > PARAMETER_NAME_LEN) {
					printf("Error: parameter name too long\n");
					debug(start, len);
					exit(1);
				}
				strncpy(p_functor->name, start, len);
				p_functor->name[len+1] = '\0';
				start = closing_quote + 1;
			}
		}

		// TODO: can a functor be in a ParamArray?
		if (mode & parse_content) parse_parameter_list((Parameter *) p->content, start, stop, mode);
		//printf("%s %s %s\n", p->name, p->type, ((Parameter *) p->content)->name);
	}
	else if (mode & parse_content) { // Atomic parameters
		if (strcmp(p->type, "ParamString") == 0) {
			char *str_start = find('"', start, stop);
			if (str_start == NULL) return;
			str_start += 1;
			char *str_stop = find('"', str_start, stop);
			size_t str_len = str_stop - str_start;
			char *content = (char *) malloc(str_len+1);
			memcpy(content, str_start, str_len);
			content[str_len] = '\0';
			p->content = (void *) content;
			//printf("%s %s %s %d\n", p->name, p->type, (char *) p->content, str_len);
		}
		else if (strcmp(p->type, "ParamLong") == 0) {
			start += strspn(start, " \n\r\t");
			stop = start + strspn(start, "-0123456789");
			if (start == stop) return;
			int64_t *content = (int64_t *) &(p->content);
			*content = strtol(start, NULL, 10);
			//printf("%s %s %li\n", p->name, p->type, (int64_t) p->content);
		}
		else if (strcmp(p->type, "ParamDouble") == 0) {
			start += strspn(start, " \n\r\t");
			stop = start + strspn(start, "-.0123456789");
			if (start == stop) return;
			double *content = (double *) &(p->content);
			*content = strtod(start, NULL);
			//printf("%s %s %lf\n", p->name, p->type, (double) *content);
		}
		else if (strcmp(p->type, "ParamBool") == 0) {
			char *definitions = find('<', start, stop); // yeah not parsing these
			char *str_start = find('"', start, stop);
			if (str_start == NULL || definitions != NULL) return;
			str_start += 1;
			char *str_stop = find('"', str_start, stop);
			size_t str_len = str_stop - str_start;
			char *content = (char *) malloc(str_len+1);
			memcpy(content, str_start, str_len);
			content[str_len] = '\0';
			bool value;
			if (strcmp(content, "true") == 0) value = true;
			else if (strcmp(content, "false") == 0) value = false;
			else {
				printf("Error: invalid value for ParamBool \"%s\"\n", content);
				exit(1);
			}
			p->content = (void *) value;
			//printf("%s %s %d\n", p->name, p->type, (size_t) p->content);
		}
		else {
			//printf("%s\n", p->type);
			// Throw error when finished with all types
		}
	}
	else {
		// TODO
	}
	return;
}





int main(int argc, char* argv[])
{
	// Open file
	FILE *f;
	if (argc < 2) {
		printf("Error: no filename given\n");
		return 1;
	}
	f = fopen(argv[1], "rb");

	// Get file header
	FileHeader file_header;
	safe_fread(&file_header, sizeof(file_header), f);
	// Check version
	if (file_header.version >= 10000 || file_header.num > 64) {
		printf("Error: unsupported twix version\n");
		return 1;
	}

	// TODO put in function
	//printf("File header: 1 whatever %d\n 2 num %d\n 3 id %d\n 4 fileid %d\n 5 offset %d\n 6 len %d\n name %s\n prot %s\n",
	//	file_header.whatever,
	//	file_header.num,
	//	file_header.entries[0].id,
	//	file_header.entries[0].fileid,
	//	file_header.entries[0].offset,
	//	file_header.entries[0].len,
	//	file_header.entries[0].patient_name,
	//	file_header.entries[0].protocol_name
	//);

	// Iterate measurements
	// TODO: here create arrays holding the protocols and data of every scan
	for (int scan = 0; scan < file_header.num; scan++) {
		// Read measurement header
		fseek(f, file_header.entries[scan].offset, SEEK_SET);
		ProtocolHeader protocol_header;
		safe_fread(&protocol_header, sizeof(ProtocolHeader), f);

		// Parse headers
		Parameter *headers = (Parameter*) calloc(protocol_header.num, sizeof(Parameter));
		for (int header = 0; header < protocol_header.num; header++) {
			// Read protocol into string
			Parameter *p = &(headers[header]);
			char *start, *stop;
			char *name = (char*) &(p->name);
			read_protocol(f, name, &start, &stop);
			//printf("%s\n", name);
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
			strcpy(p->type, "ParamMap");
			parse_parameter_content(p, start, stop, parse_type | parse_content);
		}

		// Read in scan data
		size_t file_pos = file_header.entries[scan].offset + protocol_header.len; // TODO: might not need var
		size_t num_bytes = file_header.entries[scan].len - protocol_header.len;
		//char *asd = malloc(14);
		//safe_fread(asd, 14, f);
		//debug(asd, 14);
		//printf("File pos %ld %ld %ld %ld\n", ftell(f), file_pos, num_bytes, file_header.entries[scan].offset);
		//return 0;
		char *scan_buffer = (char *)malloc(num_bytes);
		fseek(f, file_pos, SEEK_SET);
		safe_fread(scan_buffer, num_bytes, f);
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

