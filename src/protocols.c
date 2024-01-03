
#define min(a, b) (a) < (b) ? (a) : (b)
#define PARAMETER_NAME_LEN 64
#define PARAMETER_TYPE_LEN 16
#define DEBUG_PARAMETERS


struct Protocol
{
	uint32_t length; // number of bytes of protocols (incl. header)
	uint32_t num; // how many parameter sets
	Parameter *parameters;
};

struct Parameter
{
	char type[PARAMETER_TYPE_LEN];
	char name[PARAMETER_NAME_LEN];
	void *content;
};

struct ParameterList
{
	struct ParameterList* next;
	struct ParameterList* prev;
	Parameter p;
};


enum parse_mode {parse_type = 1, parse_content = 2, copy_type = 4};


//void free
//
//void free_parameter_list()
//{
//
//}


#include "protocol_parameters.c"



void read_protocol_header(FILE* f, Protocol* protocol)
{
	safe_fread(f, protocol, 2 * sizeof(uint32_t)); // read length and num

	protocol->parameters = (Parameter*) malloc(protocol->num * sizeof(Parameter));

	for (int p = 0; p < protocol->num; p++) {

		const char parameter_set_type[] = "ParamSet";
		strcpy(protocol->parameters[p].type, parameter_set_type);

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


void twix_load_protocol(Twix *twix, int scan, char *name) // TODO: keep string to print it if required? Also makes loading the raw protocol easier. Shouldn't take much memory
{
	FILE* f = twix->f;
	fseek(f, twix->file_header->entries[scan].offset, SEEK_SET);

	Protocol *protocol = &(twix->protocols[scan]);
	Parameter *parameters = protocol->parameters;
	int p;
	for (p = 0; p < protocol->num; p++)
		if (strcmp(parameters[p].name, name) == 0) break;

	if (p == protocol->num) {
		printf("Error: protocol %s not found\n", name);
		exit(EXIT_FAILURE);
	}

	read_protocol(f, protocol, p);
	parse_protocol(parameters + p);

	return;
}



char* twix_scanner_protocol(Twix* twix, int scan)
{
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

