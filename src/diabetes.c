
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

struct ParameterList;
typedef struct ParameterList {
	struct ParameterList* next;
	struct ParameterList* prev;
	Parameter p;
} ParameterList;

void read_protocol(FILE* f, char** start, char** stop) {
	// Read config
	// TODO: what is it useful for?
	char name[48]; // is this length the actual max?
	{
		for (int j = 0; j < 48; j++) {
			char c = fgetc(f);
			check_file(f);
			name[j] = c;
			if (c == '\0') break;
		}
	}

	// Read protocol into string
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
	*start = find('{', str, str+len-1);
	*stop = find('}', str+len-1, str);
	if (*start == NULL || *stop == NULL) {
		printf("Error: could not find enclosing braces of outer scope\n");
		exit(1);
	}
	return;
}

char* parse_parameter_name_type(Parameter* p, char* start, char* stop) {

	// Strip spaces
	start += strspn(start, " \n\r\t") + 1; // +1 because of <

	// Parse type
	for (int i = 0; i < sizeof(p->type); i++) {
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
	if (p->type[sizeof(p->type)] != '\0') {
		printf("Error: parameter type unexpectedly long");
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
		p->name[diff+1] = '\0';
	}
	else {
		p->name[0] = '\0';
	}
	return start + 2; // ">
}


// gets {...}
void parse_parameter_content(Parameter* p, char* start, char* stop, bool istype) {

	start = find('{', start, stop) + 1;
	stop = find('}', stop, start) - 1;
	if (start == NULL || stop == NULL) {
		printf("Error: could not find enclosing braces of parameter scope (%s)\n", p->name);
		exit(1);
	}

	//printf("%s %s\n", p->type, p->name);
	if (strcmp(p->type, "ParamMap") == 0) {
		char* next_start = start;
		ParameterList *list = (ParameterList*) malloc(sizeof(ParameterList));
		list->prev = NULL;
		p->content = (void*) list;
		ParameterList* current_list = list;
		Parameter* current_p;
		while (next_start < stop) {
			// Find start of parameter signature
			next_start = find('<', next_start, stop);
			if (next_start == NULL) break;

			// Make new list entry
			current_list->next = (ParameterList*) malloc(sizeof(ParameterList));
			current_list->next->prev = current_list;
			current_list = current_list->next;
			current_p = &(current_list->p);

			// Parse name
			next_start = parse_parameter_name_type(current_p, next_start, stop);
			if (next_start == NULL) {
				printf("Error: parameter with unexpected name or type\n");
				exit(1);
			}

			// Find enclosing scope
			next_start = find('{', next_start, stop);
			if (next_start == NULL) {
				printf("Error: parameter without content\n");
				exit(1);
			}
			char* closing_brace = find_matching(next_start, '}');

			// Parse content
			parse_parameter_content(current_p, next_start, closing_brace, istype);
			next_start = closing_brace + 1;
		}
	}
	else if (strcmp(p->type, "ParamArray") == 0) {
		// Check signature
		start = find('<', start, stop) + 1;
		char signature[] = "Default> ";
		size_t signature_length = sizeof(signature)-1;
		if (strncmp(start, signature, signature_length) != 0) {
			printf("Error: parameter array with unexpected signature\n");
			debug(start, sizeof(signature));
		}
		start += sizeof(signature)-1;

		char *type_closing_brace = find_matching(start, '}');

		// Parse name and type
		Parameter* p_array_element = (Parameter*)malloc(sizeof(Parameter));
		start = parse_parameter_name_type(p_array_element, start, type_closing_brace);
		parse_parameter_content(p_array_element, start, type_closing_brace, true);
		//printf("%s\n", p_array_element->type);

		// Parse content
		// Copy p_array_element for next element
		// finally put into p->content
	}
	else if (strcmp(p->type, "ParamString") == 0) {
		char *str_start = find('"', start, stop);
		if (str_start == NULL) {
			p->content = NULL;
			return;
		}
		str_start += 1;
		char *str_stop = find('"', str_start, stop);
		size_t str_len = str_stop - str_start;
		char *content = (char *) malloc(str_len+1);
		memcpy(content, str_start, str_len);
		content[str_len] = '\0';
		p->content = (void *) content;
		//printf("%s %s %s %d\n", p->name, p->type, (char *) p->content, str_len);
	}
	else if (!istype && strcmp(p->type, "ParamLong") == 0) {
		start += strspn(start, " \n\r\t");
		stop = start + strspn(start, "-0123456789");
		if (start == stop) {
			p->content = NULL;
			return;
		}
		int64_t *content = (int64_t *) &(p->content);
		*content = strtol(start, NULL, 10);
		//printf("%s %s %li\n", p->name, p->type, (int64_t) p->content);
	}
	else if (!istype && strcmp(p->type, "ParamDouble") == 0) {
		start += strspn(start, " \n\r\t");
		stop = start + strspn(start, "-.0123456789");
		if (start == stop) {
			p->content = NULL;
			return;
		}
		double *content = (double *) &(p->content);
		*content = strtod(start, NULL);
		//printf("%s %s %lf\n", p->name, p->type, (double) *content);
	}
	else {
		// Throw error when finished with all types
	}
	return;
}



int main(int argc, char* argv[]) {
	
	// Open file
	FILE *f;
	if (argc < 2) {
		printf("Error: no filename given\n");
		return 1;
	}
	f = fopen(argv[1], "rb");

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

	// Get file header
	FileHeader file_header;
	safe_fread(&file_header, sizeof(file_header), f);

	// Iterate measurements
	for (int i = 0; i < file_header.num; i++) {
		// Read measurement header
		uint64_t position = file_header.entries[i].offset;
		fseek(f, position, SEEK_SET);
		MeasurementHeader measurement_header;
		safe_fread(&measurement_header, sizeof(MeasurementHeader), f);

		// Read protocol into string
		char *start, *stop;
		read_protocol(f, &start, &stop);

		// Parse protocol
		Parameter *p = (Parameter *) malloc(sizeof(Parameter));
		strcpy(p->name, "XProtocol");
		strcpy(p->type, "ParamMap");
		parse_parameter_content(p, start, stop, false);
	}

	return 0;
}

