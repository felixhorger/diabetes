
#include "dataflags.c"
#define SCANDATABLOCKLEN 128 // How many readout headers are in one block in ScanData

// TODO: Make diagram of data structure
typedef struct ScanData ScanData; // Pointers into buffer holding scan data are stored here
typedef struct ReadoutHeader ReadoutHeader; // Formerly called measurement data header (MDH), information for single readout
typedef struct ChannelHeader ChannelHeader; // Information of a single channel, placed just after the readout header and before the readout of every channel

struct ScanData
{
	ReadoutHeader ***hdrs;
	int32_t block;
	int32_t index;
};

struct ReadoutHeader
{
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
};

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


size_t get_readout_num_bytes(ReadoutHeader *readout_hdr)
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

