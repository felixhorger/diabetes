
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

Parameter copy_parameter_type(Parameter *dest, Parameter *src) {
	strcpy(dest->name, src->name);
	strcpy(dest->type, src->type);
	if (strcmp(src->type, "ParamMap") == 0) {
		ParameterList *list = (ParameterList*) malloc(sizeof(ParameterList));
		list->prev = NULL;
		dest->content = (void*) list;
		ParameterList *src_list = (ParameterList*) src->content;
		while (src_list != NULL) {
			// Add new element to destination list and advance to it
			list->next = (ParameterList*) malloc(sizeof(ParameterList));
			list->next->prev = list;
			list = list->next;
			// Copy list entry
			copy_parameter_type(&(list->p), &(src_list->p));
			// Advance source list
			src_list = src_list->next;
		}
	}
	else if (strcmp(src->type, "ParamArray") == 0) {
		Parameter *p_array = (Parameter*) malloc(sizeof(Parameter));
		strcpy(p_array->type, "ParamMap");
		p_array->name[0] = '\0';
		dest->content = (void *) p_array;
		copy_parameter_type(p_array, (Parameter*)src->content);
	}
	else {
		dest->content = NULL;
	}
}

enum parse_mode {parse_type = 1, parse_content = 2, copy_type = 4};

// gets {...}
void parse_parameter_content(Parameter* p, char* start, char* stop, enum parse_mode mode) {
	if ((mode & copy_type) == 0) {
		/*	Problem is that with normal ParamMap, there are extra curly braces
			But with ParamMap used for the ParamArray there aren't.
			So, they are only to be removed if not parsing the contents of
			a ParamArray via a ParamMap, i.e. if copy_type is off.
		*/
		start = find('{', start, stop) + 1;
		stop = find('}', stop, start) - 1;
	}
	if (start == NULL || stop == NULL) {
		printf("Error: could not find enclosing braces of parameter scope (%s)\n", p->name);
		exit(1);
	}

	printf("%s %s\n", p->type, p->name);
	if (strcmp(p->type, "ParamMap") == 0) {
		// Set up list of parameters
		ParameterList *list;
		if (mode & parse_type) {
			// Make new list, names and types are to be filled in
			list = (ParameterList*) malloc(sizeof(ParameterList));
			list->prev = NULL;
			p->content = (void*) list;
		}
		else {
			// For ParamArray, names and types have been filled already (first element)
			list = (ParameterList*) p->content;
			if ((mode & copy_type) == 0) list = list->next;
		}
		Parameter* current_p;

		// Append all entries to list
		while (start < stop) {
			if (mode & parse_type) { // Name and type are to be determined
				// Find start of parameter signature
				start = find('<', start, stop);
				if (start == NULL) break;

				// Make new list entry
				list->next = (ParameterList*) malloc(sizeof(ParameterList));
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
			else if (mode & copy_type) {
				list->next = (ParameterList*) malloc(sizeof(ParameterList));
				list->next->prev = list;
				list = list->next;
				current_p = &(list->p);
				copy_parameter_type(current_p, &(list->prev->p));
			}
			else { // Name and type are determined by ParamArray
				list = list->next;
				current_p = &(list->p);
			}

			// Find enclosing scope
			start = find('{', start, stop);
			if (start == NULL) {
				printf("Error: parameter without content\n");
				exit(1);
			}
			char* closing_brace = find_matching(start, '}');

			// Parse content
			// Content of a map is parsed no matter if ParamArray or not
			parse_parameter_content(current_p, start, closing_brace, mode & (~copy_type));
			start = closing_brace + 1;
			start += strspn(start, " \n\r\t");
		}
	}
	else if (strcmp(p->type, "ParamArray") == 0) {
		if (mode & parse_type) {
			// Check signature
			start = find('<', start, stop) + 1;
			char signature[] = "Default> ";
			size_t signature_length = sizeof(signature)-1;
			if (strncmp(start, signature, signature_length) != 0) {
				printf("Error: parameter array with unexpected signature\n");
				debug(start, sizeof(signature));
			}
			start += sizeof(signature)-1;
		}

		char *type_closing_brace = find_matching(start, '}');

		if (mode & parse_type && mode & parse_content) {
			// The elements of the ParamArray will be loaded into a ParamMap, which is set up here

			// Create ParamMap with list in contents
			Parameter *p_array = (Parameter*) malloc(sizeof(Parameter));
			strcpy(p_array->type, "ParamMap");
			p_array->name[0] = '\0'; // No name necessary
			ParameterList *list = (ParameterList *)malloc(sizeof(ParameterList));
			list->prev = NULL;
			list->next = NULL;
			p_array->content = (void *) list;

			// Reference this parameter in contents of p
			p->content = (void *) p_array;

			// Parse array type and put into the ParamMap list 
			Parameter* p_array_element = &(list->p);
			start = parse_parameter_name_type(p_array_element, start, type_closing_brace);
			parse_parameter_content(p_array_element, start, type_closing_brace, parse_type);

			// Advance pointer to after the ParamArray type definition
			start = type_closing_brace + 1;
			// Parse the ParamArray contents
			parse_parameter_content(p_array, start, stop, parse_content | copy_type);
		}
		else if (mode & parse_content) {
			// ParamArray inside a ParamArray, it has been transformed to a ParamMap,
			// just need to follow a reference
			parse_parameter_content((Parameter *)p->content, start, stop, parse_content);
		}
	}
	else if (mode & parse_content) { // Atomic parameters
		if (strcmp(p->type, "ParamString") == 0) {
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
		else if (strcmp(p->type, "ParamLong") == 0) {
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
		else if (strcmp(p->type, "ParamDouble") == 0) {
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
		else if (strcmp(p->type, "ParamBool") == 0) {
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
			bool value;
			if (strcmp(content, "true") == 0) value = true;
			else if (strcmp(content, "false") == 0) value = false;
			else {
				printf("Error: invalid value for ParamBool \"%s\"\n", content);
				exit(1);
			}
			p->content = (void *) value;
			//printf("%s %s %s %d\n", p->name, p->type, (char *) p->content, str_len);
		}
		else {
			// Throw error when finished with all types
		}
	}
	else {
		// TODO
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
		parse_parameter_content(p, start, stop, parse_type | parse_content);
	}

	return 0;
}

