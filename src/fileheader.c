
struct ScanHeader
{
	uint32_t id;
	uint32_t fileid;
	uint64_t offset;
	uint64_t len;
	char patient_name[64]; // Why is this in here and not FileHeader?
	char protocol_name[64];
};

struct FileHeader
{
	uint32_t version;
	uint32_t num_scans;
	ScanHeader entries[64];
};



void check_file_header(FileHeader *file_header)
{
	if (file_header->version >= 10000 || file_header->num_scans > 64) {
		printf("Error: unsupported twix version\n");
		exit(1);
	}
	return;
}


void print_file_header(FileHeader *file_header)
{
	printf(
		"File header:\n"
		"Version         %d\n"
		"Number of scans %d\n",
		file_header->version,
		file_header->num_scans
	);

	for (int s = 0; s < file_header->num_scans; s++) {
		printf(
			"-----\n"
			"Scan:     %d\n"
			"ID:       %d\n"
			"File ID:  %d\n"
			"Offset:   %ld\n"
			"Length:   %ld\n"
			"Patient:  %s\n"
			"Protocol: %s\n-----",
			s,
			file_header->entries[s].id,
			file_header->entries[s].fileid,
			file_header->entries[s].offset,
			file_header->entries[s].len,
			file_header->entries[s].patient_name,
			file_header->entries[s].protocol_name
		);
	}
	return;
}

