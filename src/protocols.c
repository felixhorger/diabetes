struct Protocol
{
	uint32_t length; // number of bytes of protocols (incl. header)
	uint32_t num; // how many parameter sets
	Parameter *parameters;
	int sets[6]; // is set to -1 if not parsed
};

#define min(a, b) (a) < (b) ? (a) : (b)
#define PARAMETER_NAME_LEN 64
#define PARAMETER_TYPE_LEN 16
//#define DEBUG_PARAMETERS

enum parse_mode {parse_type = 1, parse_content = 2, copy_type = 4};

struct Parameter
{
	char type[PARAMETER_TYPE_LEN];
	char name[PARAMETER_NAME_LEN];
	void *content;
};

#include "parse_measyaps.c" // TODO: need to get rid of dependence on struct Parameter globally in twixeater
#include "parse_config_dicom_meas_protocols.c"

// Assumes that the order is predefined
enum parameter_set {twix_config=0, twix_dicom=1, twix_meas=2, twix_measyaps=3, twix_phoenix=4, twix_spice=5};


void read_protocol_header(FILE* f, Protocol* protocol)
{
	safe_fread(f, protocol, 2 * sizeof(uint32_t)); // read length and num

	protocol->parameters = (Parameter*) calloc(1, protocol->num * sizeof(Parameter));

	for (int p = 0; p < protocol->num; p++) {

		strcpy(protocol->parameters[p].type, "NotLoaded");

		char *name = (char*) &(protocol->parameters[p].name);
		for (int j = 0; j < PARAMETER_NAME_LEN; j++) {
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
	
	// TODO: is order consistent? it is assumed here
	char *name = protocol->parameters[index].name;
	if      (strcmp(name, "Config")   == 0) protocol->sets[index] = 0;
	else if (strcmp(name, "Dicom")    == 0) protocol->sets[index] = 1; // these could be anything, just not -1
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


int twix_receive_channels(Twix *twix, int scan)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_receive_channels(twix, _scan_, size)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_measyaps] == -1) {
		printf("Error: MeasYAPS protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Dictionary *dict; 
	dict = (Dictionary *) twix->protocols->parameters[3].content;
	dict = get_measyaps_subdict(dict, "sCoilSelectMeas");
	dict = get_measyaps_subdict(dict, "aRxCoilSelectData[0]");
	dict = get_measyaps_subdict(dict, "asList");
	dict = get_measyaps_subdict(dict, "__attribute__");
	char *num_channels = get_measyaps_entry(dict, "size");

	return strtol(num_channels, NULL, 10);
}


// size[3]
void twix_kspace_size(Twix *twix, int scan, int *size)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_kspace_dims(twix, _scan_, size)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_meas] == -1) {
		printf("Error: Meas protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p, *p_col, *p_lin, *p_par;
	// TODO: the below two lines are the same for every access of meas protocol? Put in function
	p = protocol->parameters + twix_meas; // ParamSet Config
	p = find_in_parameter_map((Parameter *)p->content, "MEAS"); // ParamMap MEAS
	p = find_in_parameter_map(p, "sKSpace"); // ParamMap sKSpace
	p_col = find_in_parameter_map(p, "lBaseResolution");
	p_lin = find_in_parameter_map(p, "lPhaseEncodingLines");
	p_par = find_in_parameter_map(p, "lPartitions");

	if (p_col == NULL || p_lin == NULL || p_par == NULL) {
		printf("Error: could not find k-space dimensions in Config protocol");
		exit(EXIT_FAILURE);
	}

	size[0] = 2 * get_long_parameter(p_col);
	size[1] = get_long_parameter(p_lin);
	size[2] = get_long_parameter(p_par);

	return;
}

// accel[2]
void twix_acceleration_factors(Twix *twix, int scan, int *accel)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_kspace_dims(twix, _scan_, size)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_meas] == -1) {
		printf("Error: Meas protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p, *p_lin, *p_par;
	p = protocol->parameters + twix_meas; // ParamSet Config
	p = find_in_parameter_map((Parameter *)p->content, "MEAS"); // ParamMap MEAS
	p = find_in_parameter_map(p, "sPat");
	p_lin = find_in_parameter_map(p, "lAccelFactPE");
	p_par = find_in_parameter_map(p, "lAccelFact3D");

	accel[0] = get_long_parameter(p_lin);
	accel[1] = get_long_parameter(p_par);

	return;
}

void twix_ref_dims(Twix *twix, int scan, int *size)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_ref_size(twix, _scan_, size)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_meas] == -1) {
		printf("Error: Meas protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p, *p_lin, *p_par;
	p = protocol->parameters + twix_meas; // ParamSet Config
	p = find_in_parameter_map((Parameter *)p->content, "MEAS"); // ParamMap MEAS
	p = find_in_parameter_map(p, "sPat");
	p_lin = find_in_parameter_map(p, "lRefLinesPE");
	p_par = find_in_parameter_map(p, "lRefLines3D");

	size[0] = get_long_parameter(p_lin);
	size[1] = get_long_parameter(p_par);

	return;
}


// in degrees
double twix_flip_angle(Twix *twix, int scan, int i)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_flip_angle(twix, _scan_, i)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_measyaps] == -1) {
		printf("Error: MeasYAPS protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Dictionary *dict; 
	dict = (Dictionary *) twix->protocols->parameters[3].content;

	char key[17+2+2+1]; // adFlipAngleDegree, [], index can be max 2 digits, null byte
	sprintf(key, "adFlipAngleDegree[%d]", i);
	char *fa = get_measyaps_entry(dict, key);

	return strtod(fa, NULL);
}

// in ms
double twix_echo_time(Twix *twix, int scan, int i)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_echo_time(twix, _scan_, i)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_measyaps] == -1) {
		printf("Error: MeasYAPS protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Dictionary *dict; 
	dict = (Dictionary *) twix->protocols->parameters[3].content;

	char key[4+2+3+1]; // alTE, [], index can be max 3 digits, null byte
	sprintf(key, "alTE[%d]", i);
	char *te = get_measyaps_entry(dict, key);

	return 0.001 * strtol(te, NULL, 10);
}

// TODO: boilerplate, how to improve?
// in ms
double twix_repetition_time(Twix *twix, int scan, int i)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_repetition_time(twix, _scan_, i)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_measyaps] == -1) {
		printf("Error: MeasYAPS protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Dictionary *dict; 
	dict = (Dictionary *) twix->protocols->parameters[3].content;

	char key[4+2+3+1]; // alTR, [], index can be max 3 digits, null byte
	sprintf(key, "alTR[%d]", i);
	char *tr = get_measyaps_entry(dict, key);

	return 0.001 * strtol(tr, NULL, 10);
}


// in mus
double twix_dwell_time(Twix *twix, int scan)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_dwell_time(twix, _scan_)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_meas] == -1) {
		printf("Error: Meas protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p;
	p = protocol->parameters + twix_meas; // ParamSet Meas
	p = find_in_parameter_map((Parameter *)p->content, "MEAS"); // ParamMap MEAS
	p = find_in_parameter_map(p, "sRXSPEC");
	p = find_in_parameter_map(p, "alDwellTime");
	// TODO: dwell time is labeled as ParamLong, but is multiple values fffffffff

	return 0.001 * get_long_parameter(p);
}



// orientation = [normal_x, normal_y, normal_z, rotation angle]
// fov[3], shift[3]
void twix_coordinates(Twix *twix, int scan, double *orientation, double *shift, double *fov)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_coordinates(twix, _scan_, orientation, shift, fov)");
	// TODO all this also boilerplate, outsource

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_measyaps] == -1) {
		printf("Error: MeasYAPS protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Dictionary *dict;
	dict = (Dictionary *) twix->protocols->parameters[3].content;
	dict = get_measyaps_subdict(dict, "sSliceArray");
	dict = get_measyaps_subdict(dict, "asSlice[0]");

	fov[0] = strtod(get_measyaps_entry(dict, "dReadoutFOV"), NULL);
	fov[1] = strtod(get_measyaps_entry(dict, "dPhaseFOV"),   NULL);
	fov[2] = 2.0 * strtod(get_measyaps_entry(dict, "dThickness"),  NULL);

	Dictionary *subdict;
	char *entry;
	memset(orientation, 0, 4 * sizeof(double));
	memset(shift, 0, 3 * sizeof(double));

	subdict = get_measyaps_subdict(dict, "sNormal");
	entry = get_measyaps_entry(subdict, "dSag");
	if (entry != NULL) orientation[0] = strtod(entry, NULL);
	entry = get_measyaps_entry(subdict, "dCor");
	if (entry != NULL) orientation[1] = strtod(entry, NULL);
	entry = get_measyaps_entry(subdict, "dTra");
	if (entry != NULL) orientation[2] = strtod(entry, NULL);
	entry = get_measyaps_entry(subdict, "dInPlaneRot");
	if (entry != NULL) orientation[3] = strtod(entry, NULL);

	subdict = get_measyaps_subdict(dict, "sPosition");
	entry = get_measyaps_entry(subdict, "dSag");
	if (entry != NULL) shift[0] = strtod(entry, NULL);
	entry = get_measyaps_entry(subdict, "dCor");
	if (entry != NULL) shift[1] = strtod(entry, NULL);
	entry = get_measyaps_entry(subdict, "dTra");
	if (entry != NULL) shift[2] = strtod(entry, NULL);

	return;
}


// pos[4] three chars plus null byte
void twix_patient_position(Twix *twix, int scan, char *pos)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_patient_position(twix, _scan_, pos)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_config] == -1) {
		printf("Error: Config protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p;
	p = protocol->parameters + twix_config; // ParamSet Config
	p = (Parameter *)p->content; // ParamMap
	p = find_in_parameter_map(p, "PARC"); // ParamMap
	p = find_in_parameter_map(p, "PIPE"); // ParamMap
	p = find_in_parameter_map(p, "acquisition"); // PipeService (behaves like ParamFunctor)
	p = (Parameter *)p->content; // ParamMap
	p = find_in_parameter_map(p, "FeedbackRoot"); // ParamFunctor
	p = (Parameter *)p->content; // ParamMap
	p = find_in_parameter_map(p, "PatientPosition"); // ParamFunctor
	// TODO: simple from other protocols?

	strcpy(pos, get_string_parameter(p));

	return;
}

// 0 = male, 1 = female, 3 = other (I think)
long twix_patient_sex(Twix *twix, int scan)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_patient_position(twix, _scan_, pos)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_config] == -1) {
		printf("Error: Config protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p;
	p = protocol->parameters + twix_config; // ParamSet Config
	p = (Parameter *)p->content; // ParamMap
	p = find_in_parameter_map(p, "PARC"); // ParamMap
	p = find_in_parameter_map(p, "RECOMPOSE"); // ParamMap
	p = find_in_parameter_map(p, "PatientSex");

	return get_long_parameter(p);
}


double twix_ref_voltage(Twix *twix, int scan)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_ref_voltage(twix, _scan_)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_meas] == -1) {
		printf("Error: Meas protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Parameter *p;
	p = protocol->parameters + twix_meas; // ParamSet Meas
	p = find_in_parameter_map((Parameter *)p->content, "MEAS"); // ParamMap MEAS
	p = find_in_parameter_map(p, "sTXSPEC");
	p = find_in_parameter_map(p, "asNucleusInfo");
	p = index_parameter_array(p, 0);
	p = find_in_parameter_map(p, "flReferenceAmplitude");

	return get_double_parameter(p);
}

// param should point to 8 bytes
void twix_wip_param(Twix *twix, int scan, void *param, int i)
{
	check_bounds(scan, twix->file_header->num_scans, "twix_wip_params(twix, _scan_, param, i)");

	Protocol* protocol = twix->protocols + scan;

	if (protocol->sets[twix_measyaps] == -1) {
		printf("Error: MeasYAPS protocol not loaded for scan %d\n", scan);
		exit(EXIT_FAILURE);
	}

	Dictionary *dict;
	dict = (Dictionary *) twix->protocols->parameters[3].content;
	dict = get_measyaps_subdict(dict, "sWipMemBlock");

	char *entry;
	char key[sizeof("alFree[00]")];

	// Check if it's in the long array
	sprintf(key, "alFree[%d]", i);
	entry = get_measyaps_entry(dict, key);
	if (entry != NULL) {
		*(long *)param = strtol(entry, NULL, 10);
		return;
	}

	sprintf(key, "adFree[%d]", i);
	entry = get_measyaps_entry(dict, key);
	if (entry == NULL) {
		printf("Error: WIP parameter at index %d not set\n", i);
		exit(EXIT_FAILURE);
	}
	*(double *)param = strtod(entry, NULL);

	return;
}

