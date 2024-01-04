#define min(a, b) (a) < (b) ? (a) : (b)
#define PARAMETER_NAME_LEN 64
#define PARAMETER_TYPE_LEN 16
//#define DEBUG_PARAMETERS

enum parameter_set {twix_config=0, twix_dicom=1, twix_meas=2, twix_measyaps=3, twix_phoenix=4, twix_spice=5};

struct Protocol
{
	uint32_t length; // number of bytes of protocols (incl. header)
	uint32_t num; // how many parameter sets
	Parameter *parameters;
	int sets[6]; // is set to -1 if not parsed
};



#include "protocol_parameters.c"



void read_protocol_header(FILE* f, Protocol* protocol)
{
	safe_fread(f, protocol, 2 * sizeof(uint32_t)); // read length and num

	protocol->parameters = (Parameter*) calloc(1, protocol->num * sizeof(Parameter));

	for (int p = 0; p < protocol->num; p++) {

		strcpy(protocol->parameters[p].type, "NotLoaded");

		char *name = (char*) &(protocol->parameters[p].name);
		for (int j = 0; j < 48; j++) {
			char c = fgetc(f);
			check_file(f);
			name[j] = c;
			if (c == '\0') break;
		}

		char *protocol_info = malloc(sizeof(long) + sizeof(uint32_t));

		uint32_t len;
		safe_fread(f, &len, sizeof(uint32_t));
		memcpy(protocol_info + sizeof(long), &len, sizeof(uint32_t));

		long filepos = ftell(f);
		memcpy(protocol_info, &filepos, sizeof(long));

		protocol->parameters[p].content = (void *) protocol_info;

		fseek(f, len, SEEK_CUR);
	}
	return;
}



void read_protocol(FILE *f, Protocol *protocol, int index)
{
	check_bounds(index, protocol->num, "read_protocol(f, protocol, _index_)");

	strcpy(protocol->parameters[index].type, "ReadParamSet");

	char *protocol_info = (char *) protocol->parameters[index].content;
	long filepos = *((long *)protocol_info);
	uint32_t len = *((uint32_t *)(protocol_info + sizeof(long)));

	fseek(f, filepos, SEEK_SET);

	char *parameters_str = malloc(sizeof(long) + sizeof(uint32_t) + len);
	memcpy(parameters_str, &filepos, sizeof(long));
	memcpy(parameters_str + sizeof(long), &len, sizeof(uint32_t));

	safe_fread(f, parameters_str + sizeof(long) + sizeof(uint32_t), len);

	free(protocol->parameters[index].content);
	protocol->parameters[index].content = (void *) parameters_str;

	return;
}



void parse_protocol(Protocol *protocol, int index) // the top-most container of protocols i.e. Config, Meas, MeasYaps, ...
{
	
	// TODO: is order consistent?
	char *name = protocol->parameters[index].name;
	if      (strcmp(name, "Config")   == 0) protocol->sets[index] = 0;
	else if (strcmp(name, "Dicom")    == 0) protocol->sets[index] = 1;
	else if (strcmp(name, "Meas")     == 0) protocol->sets[index] = 2;
	else if (strcmp(name, "MeasYaps") == 0) protocol->sets[index] = 3;
	else if (strcmp(name, "Phoenix")  == 0) protocol->sets[index] = 4;
	else if (strcmp(name, "Spice")    == 0) protocol->sets[index] = 5;
	else {
		printf("Error: unknown protocol name\n");
		exit(EXIT_FAILURE);
	}

	Parameter *parameter = protocol->parameters + index;
	strcpy(parameter->type, "ParamSet");

	char *str = parameter->content;
	uint32_t len = *((uint32_t *)(str + sizeof(long)));
	char *parameters_str = str + sizeof(long) + sizeof(uint32_t); // advance by filepos and length

	if      (strcmp(parameter->name, "Config")   == 0) parse_config_protocol  (parameter, parameters_str, len);
	else if (strcmp(parameter->name, "Dicom")    == 0) parse_config_protocol  (parameter, parameters_str, len);
	else if (strcmp(parameter->name, "MeasYaps") == 0) parse_measyaps_protocol(parameter, parameters_str, len);
	else if (strcmp(parameter->name, "Meas")     == 0) parse_meas_protocol    (parameter, parameters_str, len);
	else if (strcmp(parameter->name, "Phoenix")  == 0) {
		// TODO: should be similar to "Config"
		// TODO
		printf("Warning: Skipping Phoenix parameter set\n");
	}
	else if (strcmp(parameter->name, "Spice") == 0) {
		// TODO
		printf("Warning: Skipping Spice parameter set\n");
	}
	else {
		printf("Error: unknown parameter set name %s\n", parameter->name);
		exit(EXIT_FAILURE);
	}
	free(str);

	return;
}



void twix_load_protocol(Twix *twix, int scan, enum parameter_set ps) // TODO: keep string to print it if required? Also makes loading the raw protocol easier. Shouldn't take much memory
{
	check_bounds(scan, twix->file_header->num_scans, "twix_load_protocol(twix, _scan_)");

	FILE* f = twix->f;
	fseek(f, twix->file_header->entries[scan].offset, SEEK_SET);

	Protocol *protocol = &(twix->protocols[scan]);

	read_protocol(f, protocol, ps);
	parse_protocol(protocol, ps);

	return;
}

char* twix_scanner_protocol(Twix* twix, int scan)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_scanner_protocol(twix, _scan_)");

	Protocol* protocol = twix->protocols + scan;
	Parameter* parameters = protocol->parameters;
	int p;
	const char name[] = "MeasYaps";
	for (p = 0; p < protocol->num; p++)
		if (strcmp(parameters[p].name, name) == 0) break;

	if (p == protocol->num) {
		printf("Error: could not find MeasYaps, raw data broken?");
		exit(EXIT_FAILURE);
	}

	FILE* f = twix->f;
	long filepos = *((long*) parameters[p].content);
	uint32_t len = *((uint32_t*)((char*) parameters[p].content + sizeof(long))); // TODO: make struct to do this more elegantly?

	char *protocol_str = (char*) malloc(len);

	fseek(f, filepos, SEEK_SET);
	safe_fread(f, protocol_str, len);

	return protocol_str;
}

void twix_save_scanner_protocol(Twix* twix, int scan, char* filename)
{
	char *scanner_protocol = twix_scanner_protocol(twix, scan);
	FILE *f = fopen(filename, "w");
	// TODO: check pointer
	fprintf(f, "%s", scanner_protocol);
	fclose(f);
	free(scanner_protocol);

	return;
}


// size[3]
void twix_kspace_dims(Twix *twix, int scan, int *size)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_kspace_dims(twix, _scan_, size)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_meas] == -1) {
		printf("Error: Meas protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p, *p_col, *p_lin, *p_par;
	p = protocol->parameters + twix_meas; // ParamSet
	p = index_parameter_map((Parameter *)p->content, 4); // ParamMap MEAS
	p = index_parameter_map(p, 38); // ParamMap sKSpace
	p_col = index_parameter_map(p, 0);
	p_lin = index_parameter_map(p, 1);
	p_par = index_parameter_map(p, 5);

	size[0] = (long) p_col->content;
	size[0] *= 2;
	size[1] = (long) p_lin->content;
	size[2] = (long) p_par->content;

	return;
}

//void twix_kspace_dims(Twix *twix, int scan, int *size)
//{
//	check_bounds(scan, twix->file_header->num_scans, "twix_kspace_dims(twix, _scan_)");
//
//	size[0] = twix_obj["NCol"])
//
//	# Read other dimensions
//	#=
//		Would be too easy if one could use
//
//		num_lines	= convert(Int, twix_obj["NLin"])
//		num_partitions	= convert(Int, twix_obj["NPar"])
//		num_channels	= convert(Int, twix_obj["NCha"])
//
//		but it happens that this doesn't match with the FOV that was selected, i.e.
//		this gives you the sampled k-space size, not necessarily matching the reconstructed one.
//	=#
//	bogus = twix["hdr"]["Meas"]
//	num_lines	= convert(Int, bogus["lPhaseEncodingLines"])
//	num_partitions	= convert(Int, bogus["lPartitions"])
//	num_channels	= convert(Int, twix_obj["NCha"])
//	return num_columns, num_lines, num_partitions, num_channels
//	end
//	size(raw::SiemensRawData, name::Symbol; key::String="image") = convert(Int, raw.data[key]["N" * String(name)])
//	return;
//}

