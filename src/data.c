
#include "dataflags.c"
#define MEMBLOCKSIZE (256 * sizeof(ReadoutHeader *)) // how many bytes to allocate at once when increasing the size of the structure holding pointers to readout headers
//#define DEBUG_READOUT
		
// TODO:
// Apparently there is another readout registered at the end, no idea what syncdata is (in between meas?)
// SYNCDATA
// ACQEND


// TODO: Make diagram of data structure

struct ScanData
{
	char *buffer; // data read from file
	int n;
	ReadoutHeader **hdrs; // pointers to readout headers
};

struct ReadoutHeader
{
	uint32_t	dma_length_and_flags; // What is dma? First 25 bits are length, last 7 are flags. Conversion via dma_length_and_flags % (1 << 25)
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
        float		quaternion[4]; // I guess normal and in-plane rotation, but not true
	uint16_t	ice[24];
	uint16_t	also_reserved[4];
	uint16_t	application_counter;
	uint16_t	application_mask;
	uint32_t	crc;
};

const int idx_hdr_offset = ( // Where the indices start
	sizeof(uint32_t) +
	sizeof(int32_t) +
	sizeof(uint32_t) +
	sizeof(uint32_t) +
	sizeof(uint32_t) +
	sizeof(uint16_t) +
	sizeof(uint16_t) +
	sizeof(int32_t) +
	sizeof(int32_t) +
	sizeof(int32_t) +
	sizeof(int32_t) +
	sizeof(uint64_t) +
	sizeof(uint16_t) +
	sizeof(uint16_t)
);


struct ChannelHeader
{
	uint32_t	type_and_length;
	int32_t		meas_uid;
	uint32_t	scan_counter;
	int32_t		reserved;
	uint32_t	sequence_time;
	uint32_t	unused;
	uint16_t	id;
	uint16_t	also_unused;
	uint32_t	crc;
};


uint32_t get_readout_num_bytes(ReadoutHeader *readout_hdr)
{
	return readout_hdr->dma_length_and_flags % (1 << 25);
}

//get_channel_header
//get_data

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


// TODO: how to use num_bytes to only read a certain number of readouts? How is e.g. a measurement/average indicated?
// Could say read n readouts, skipping the sync and other special ones
ScanData read_data(FILE *f, size_t num_bytes) // TODO: rename to twix_read_data?
{
	if (num_bytes < sizeof(ReadoutHeader)) {
		printf("Error: input ended prematurely in read_data()");
		exit(EXIT_FAILURE);
	}

	// Read in scan data
	char *scan_buffer = (char *)malloc(num_bytes);
	safe_fread(f, scan_buffer, num_bytes);

	// TODO: rewrite this: use mmap, and also change get_scandata() so that the first call to it
	// finds the readout headers. Otherwise memory is iterated multiple times.
	// Actually, is it necessary to find the readout headers in the first place? Don't think so

	// Set up structure holding the parsed data
	ScanData scan_data;
	scan_data.buffer = scan_buffer;
	scan_data.hdrs = (ReadoutHeader **)lmalloc(1, MEMBLOCKSIZE); // TODO: maybe incorporate the code from lmalloc here to make it more efficient
	scan_data.n = 0;

	// Parse scan data
	char *pos = scan_buffer;
	while (pos < scan_buffer + num_bytes) {
		int n = scan_data.n;
		scan_data.n += 1;
		scan_data.hdrs = (ReadoutHeader **)lrealloc(
			scan_data.hdrs,
			scan_data.n * sizeof(ReadoutHeader *)
		);
		ReadoutHeader *readout_hdr = (ReadoutHeader *)pos;
		#ifdef DEBUG_READOUT
		print_readout_header(readout_hdr);
		#endif
		lset(scan_data.hdrs, n, readout_hdr);
		pos += get_readout_num_bytes(readout_hdr);
	}
	// No idea why this was here, keeping it in case removal introduces bug
	//if (pos != scan_buffer + num_bytes) {
	//	// TODO: Error?
	//	printf("asdasdasd\n");
	//	lfree(scan_data.hdrs);
	//	free(scan_buffer);
	//	exit(EXIT_FAILURE);
	//	return scan_data;
	//}
	// Note: eof is not reached yet, residual bytes are just zeros I think, why?
	return scan_data;
}


void twix_load_data(Twix* twix, int scan)
{
	FILE* f = twix->f;
	size_t protocol_length = twix->protocols[scan].length;
	size_t file_pos  = twix->file_header->entries[scan].offset + protocol_length;
	size_t num_bytes = twix->file_header->entries[scan].len    - protocol_length;
	fseek(f, file_pos, SEEK_SET);

	ScanData data = read_data(f, num_bytes);

	// TODO could be better, maybe remove
	if (data.hdrs == NULL)
		printf("Error: skipping scan %d, too few bytes (%ld < %ld)\n", scan, num_bytes, sizeof(ReadoutHeader));

	if (twix->data == NULL) twix->data = (ScanData *) malloc(sizeof(ScanData) * twix->file_header->num_scans);
	twix->data[scan] = data;

	return;
}

// TODO: scan and also change twix struct to make data an array of scandatas
size_t twix_get_scandata(Twix *twix, float **kspace, uint16_t **idx, uint8_t *which_idx, uint8_t num_idx)
{
	// TODO: check data == NULL
	int num_readouts = twix->data->n;
	ReadoutHeader *hdr = lget(twix->data->hdrs, 0);

	uint16_t num_samples = hdr->num_samples;
	uint16_t num_channels = hdr->num_channels;
	size_t bytes_per_channel = sizeof(float) * 2 * num_samples; // and per readout
	size_t bytes_per_readout = bytes_per_channel * num_channels;
	// 2 is for complex

	float *kspace_mem = (float *)malloc(bytes_per_readout * (size_t)num_readouts);
	uint16_t *idx_mem = (uint16_t *)malloc(sizeof(uint16_t) * num_idx * (size_t)num_readouts);
	if (kspace_mem == NULL || idx_mem == NULL) {
		printf("Error: could not allocate memory for k-space or scan indices\n");
		exit(EXIT_FAILURE);
	}

	uint16_t idx_offset[14]; // max number of indices in header
	for (int i = 0; i < num_idx; i++) idx_offset[i] = idx_hdr_offset + sizeof(uint16_t) * which_idx[i];

	uint16_t *ptr_idx = idx_mem;
	void *ptr_kspace = kspace_mem;
	uint32_t expected_bytes_per_channel = sizeof(ChannelHeader) + bytes_per_channel;
	uint32_t expected_bytes_per_readout = bytes_per_readout + num_channels * sizeof(ChannelHeader) + sizeof(ReadoutHeader);
	// TODO: clarify in comments, "expected" means different things here 

	int num_actual_readouts = 0;
	for (int i = 0; i < num_readouts; i++) {
		hdr = lget(twix->data->hdrs, i);
		//printf("%d %d %d\n", i, num_readouts, num_actual_readouts);

		// Check if it's an actual readout
		uint64_t flags = hdr->eval_info_mask;
		if (
			flags & NO_IMAGE_SCAN ||
			(flags & EVALINFOMASK_PATREFSCAN && !(flags & EVALINFOMASK_PATREFANDIMASCAN))
		) continue;

		void *pos = (void *)hdr;
		
		for (int j = 0; j < num_idx; j++) ptr_idx[j] = *(uint16_t *)(pos + idx_offset[j]);
		ptr_idx += num_idx;

		uint32_t bytes_this_readout = get_readout_num_bytes(hdr);
		if (expected_bytes_per_readout != bytes_this_readout) {
			printf("Error: unexpected number of bytes in readout (exp %d, got %d)\n", expected_bytes_per_readout, bytes_this_readout);
			exit(EXIT_FAILURE);
		}

		pos += sizeof(ReadoutHeader) + sizeof(ChannelHeader);

		for (int c = 0; c < num_channels; c++) {
			memcpy(ptr_kspace, pos, bytes_per_channel);
			pos += expected_bytes_per_channel;
			ptr_kspace += bytes_per_channel;
		}

		num_actual_readouts++;
	}


	// TODO: realloc meaningful? Man koennte beim einlesen der daten die eigentliche anzahl readouts speichern, allerdings geht das wenn man zum ersten mal durchgeht, was am optimalsten waere
	// TODO: apparently reallocarray would be better, it stops safely if the below multiplication overflows
	// TODO: how can you ensure that memory is not moved for performance?
	ptr_kspace = kspace_mem; //realloc(kspace_mem, bytes_per_readout * (size_t)num_actual_readouts);
	ptr_idx = idx_mem; //realloc(idx_mem, sizeof(uint16_t) * (num_idx * (size_t)num_actual_readouts));
	if (ptr_kspace == NULL || ptr_idx == NULL) {
		free(kspace_mem);
		free(idx_mem);
		printf("Error: realloc in twix_get_scandata() failed.\n");
		exit(EXIT_FAILURE);
	}

	*kspace = ptr_kspace;
	*idx = ptr_idx;

	return num_actual_readouts;
}

