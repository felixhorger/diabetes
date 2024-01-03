// Get parameter value
double get_double_parameter(Parameter *p)
{
	check_pointer(p, "get_double_parameter(NULL)");
	return *((double *)&(p->content));
}

long get_long_parameter(Parameter *p)
{
	check_pointer(p, "get_long_parameter(NULL)");
	return *((long *)&(p->content));
}

bool get_bool_parameter(Parameter *p)
{
	check_pointer(p, "get_bool_parameter(NULL)");
	return *((bool *)&(p->content));
}

char *get_string_parameter(Parameter *p)
{
	check_pointer(p, "get_string_parameter(NULL)");
	return (char *) p->content;
}

Parameter* index_parameter_map(Parameter *p, int i)
{
	check_pointer(p, "index_parameter_map(NULL, i)");
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

Parameter* index_parameter_array(Parameter *p, int i)
{
	check_pointer(p, "index_parameter_array(NULL, i)");
	if (strcmp(p->type, "ParamArray") != 0) {
		printf("Error: not a ParamArray\n");
		exit(1);
	}
	Parameter *p_array = (Parameter *) p->content;
	return index_parameter_map(p_array, i);
}



// Debug parameter
bool is_parameter_type(Parameter *p, char *type)
{
	check_pointer(p,    "is_parameter_type(%p, type)", p);
	check_pointer(type, "is_parameter_type(p, %p)",    type);
	if (strncmp(p->type, type, PARAMETER_TYPE_LEN) == 0) return 1;
	else return 0;
}

void print_parameter(Parameter *p)
{
	check_pointer(p, "print_parameter(NULL)");

	printf("%s %s", p->type, p->name);
	if      (is_parameter_type(p, "ParamDouble")) printf(" = %lf", get_double_parameter(p));
	else if (is_parameter_type(p, "ParamLong"))   printf(" = %li", get_long_parameter(p));
	else if (is_parameter_type(p, "ParamBool"))   printf(" = %d",  get_bool_parameter(p));
	else if (is_parameter_type(p, "ParamString")) {
		char* str = get_string_parameter(p);
		if (str == NULL) printf(" = \"\"");
		else             printf(" = \"%s\"", str);
	}
	printf("\n");

	return;
}



// Parse parameter name, type and values from string
char* parse_parameter_name_type(Parameter* p, char* start, char* stop)
{
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
	check_pointer(closing_quote, "parse_parameter(): closing quote not found");
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



// container == map or array
// returns false if parsing list is finished
bool find_container_type(char** p_start, char** p_stop)
{
	char* start = *p_start;
	char* stop  = *p_stop;

	bool not_found = true;
	while (start < stop) {

		// Find start of parameter signature
		char *next_brace = find('{', start, stop);
		char *next_type = find('<', start, stop);
		if (next_brace != NULL && next_type > next_brace) {
			printf("Error: did not find a valid type\n");
			debug(start, min(128, next_brace-start-1));
			exit(EXIT_FAILURE);
		}
		start = next_type;
		if (start == NULL) return false;

		// Skip if type is on blacklist
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
			continue;
		}

		start += 1;
	}

	*p_start = start;
	*p_stop  = stop;

	return !not_found;
}


// Required for building parameter arrays
void copy_parameter_type(Parameter *dest, Parameter *src)
{
	strcpy(dest->name, src->name);
	strcpy(dest->type, src->type);
	if (strcmp(src->type, "ParamMap") == 0) {
		ParameterList *list = (ParameterList*) calloc(1, sizeof(ParameterList));
		dest->content = (void*) list;
		ParameterList *src_list = (ParameterList*) src->content;
		while (true) {
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



void parse_string_parameter(void** content, char* start, char* stop)
{
	char *str_start = find('"', start, stop);
	if (str_start == NULL) return;
	str_start += 1;

	char *str_stop = find('"', str_start, stop);
	size_t len = str_stop - str_start;

	char *str = (char *) malloc(len + 1);
	memcpy(str, str_start, len);
	str[len] = '\0';

	*content = (void *) str;

	return;
}

void parse_long_parameter(void** content, char* start, char* stop)
{
	start += strspn(start, " \n\r\t");
	stop = start + strspn(start, "-0123456789");
	if (start == stop) return;

	int64_t *ptr = (int64_t *) content;
	*ptr = strtol(start, NULL, 10);

	return;
}

void parse_double_parameter(void** content, char* start, char* stop)
{
	start += strspn(start, " \n\r\t");
	stop = start + strspn(start, "-.0123456789");
	if (start == stop) return;

	double *ptr = (double *) content;
	*ptr = strtod(start, NULL);

	return;
}

void parse_bool_parameter(void** content, char* start, char* stop)
{
	char *definitions = find('<', start, stop);
	char *str_start = find('"', start, stop);

	if (str_start == NULL || definitions != NULL) return;

	str_start += 1;
	char *str_stop = find('"', str_start, stop);
	size_t len = str_stop - str_start;

	char *str = (char *) malloc(len+1);
	memcpy(str, str_start, len);
	str[len] = '\0';

	bool *ptr = (bool *) content;
	if (strcmp(str, "true") == 0) *ptr = true;
	else if (strcmp(str, "false") == 0) *ptr = false;
	else {
		printf("Error: invalid value for ParamBool \"%s\"\n", str);
		exit(EXIT_FAILURE);
	}

	free(str);

	return;
}

void parse_parameter_content(); // Defined later because requires ParamArray and ParaMap functionality

ParameterList* append_new_entry(ParameterList* list)
{
	list->next = (ParameterList*) calloc(1, sizeof(ParameterList));
	list->next->prev = list;
	list = list->next;
	return list;
}

void parse_parameter_list(Parameter *p, char *start, char *stop, enum parse_mode mode)
{
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
		if (mode & parse_type) {
			// Name and type are to be determined (map, or array type determination)
			if (!find_container_type(&start, &stop)) return;

			list = append_new_entry(list);
			current_p = &(list->p);

			start = parse_parameter_name_type(current_p, start, stop);
			check_pointer(start, "parse_parameter_list(): parameter with unexpected name or type");
		}
		else if (mode & copy_type) {
			// Array, need to copy type before parsing content
			list = append_new_entry(list);
			current_p = &(list->p);
			copy_parameter_type(current_p, &(list->prev->p));
		}
		else {
			// Map, advance by first list element
			list = list->next;
			if (list == NULL) {
				printf("Error: ParamArray type definition inconsistent with value \n");
				exit(EXIT_FAILURE);
			}
		}

		// Find enclosing scope
		start = find('{', start, stop);
		if (start == NULL) {
			if (mode & parse_type) {
				printf("Error: parameter without content\n");
				exit(EXIT_FAILURE);
			}
			else return;
		}
		char* closing_brace = find_matching(start, '}');

		// Parse content
		// Content of a map is parsed no matter if ParamArray or not
		parse_parameter_content(current_p, start, closing_brace, mode & (~copy_type));
		start = closing_brace + 1;
		start += strspn(start, " \n\r\t");
	}

	return;
}



// [start, stop] = "{...}"
void parse_parameter_content(Parameter* p, char* start, char* stop, enum parse_mode mode)
{
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

	if (strcmp(p->type, "ParamMap") == 0 || strcmp(p->type, "Pipe") == 0) {
		parse_parameter_list(p, start, stop, mode);
		#ifdef DEBUG_PARAMETERS
		printf("%s %s\n", p->name, p->type);
		#endif
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
		#ifdef DEBUG_PARAMETERS
		printf("%s %s\n", p->name, p->type);
		#endif
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
		#ifdef DEBUG_PARAMETERS
		printf("%s %s %s\n", p->name, p->type, ((Parameter *) p->content)->name);
		#endif
	}
	else if (mode & parse_content) { // Atomic parameters
		if      (strcmp(p->type, "ParamString") == 0) parse_string_parameter(&(p->content), start, stop);
		else if (strcmp(p->type, "ParamLong")   == 0) parse_long_parameter  (&(p->content), start, stop);
		else if (strcmp(p->type, "ParamDouble") == 0) parse_double_parameter(&(p->content), start, stop);
		else if (strcmp(p->type, "ParamBool")   == 0) parse_bool_parameter  (&(p->content), start, stop);
		else {
			//printf("%s\n", p->type);
			// Throw error when finished with all types?
			return;
		}
		#ifdef DEBUG_PARAMETERS
		print_parameter(p);
		#endif
	}
	else {
		// TODO: when does this happen?
	}
	return;
}

// the top-most container of protocols i.e. Config, Meas, MeasYaps, ...
void parse_protocol(Parameter *parameter)
{
	char *str = parameter->content;
	uint32_t len = *((uint32_t *)(str + sizeof(long)));
	char *parameters_str = str + sizeof(long) + sizeof(uint32_t); // advance by filepos and length
	char *start, *stop;

	if (strcmp(parameter->name, "Config") == 0) {
		// Check initial signature
		char signature[] = "<XProtocol>";
		size_t signature_length = sizeof(signature)-1;
		if (strncmp(parameters_str, signature, signature_length) != 0) {
			printf("Error: parameter string doesn't start as expected:\n");
			debug(parameters_str, signature_length);
			printf("\n");
			exit(1);
		}
		// Set new start and stop pointers of string
		// TODO: this could be done better?
		start = find('{', parameters_str, parameters_str+len-1);
		stop = find('}', parameters_str+len-2, parameters_str);
		if (start == NULL || stop == NULL) {
			printf("Error: could not find enclosing braces of outer scope\n");
			exit(1);
		}
		start = find('{', start+1, stop);
		stop = find_matching(start, '}');
		strcpy(parameter->type, "ParamMap");
		parse_parameter_content(parameter, start, stop, parse_type | parse_content);
	}
	else if (strcmp(parameter->name, "MeasYaps") == 0) {
		// Check initial signature
		char signature[] = "### ASCCONV BEGIN ";
		size_t signature_length = sizeof(signature)-1;
		if (strncmp(parameters_str, signature, signature_length) != 0) {
			printf("Error: parameter string doesn't start as expected:\n");
			debug(parameters_str, signature_length);
			printf("\n");
			exit(1);
		}
		start = strstr(parameters_str + signature_length, " ###") + 4;
		stop = start + len - sizeof("### ASCCONV END ###") - 132; // Mysterious offset :(
		// TODO
		printf("Warning: Skipping MeasYaps parameter set\n");
		return;
	}
	else if (strcmp(parameter->name, "Meas") == 0) {
		// TODO: should be similar to "Config"
		printf("Warning: Skipping Meas parameter set\n");
		return;
	}
	else if (strcmp(parameter->name, "Phoenix") == 0) {
		// TODO
		printf("Warning: Skipping Phoenix parameter set\n");
		return;
	}
	else if (strcmp(parameter->name, "Dicom") == 0) {
		// TODO
		printf("Warning: Skipping Dicom parameter set\n");
		return;
	}
	else if (strcmp(parameter->name, "Spice") == 0) {
		// TODO
		printf("Warning: Skipping Spice parameter set\n");
		return;
	}
	else {
		printf("Error: unknown parameter set name %s\n", parameter->name);
		exit(1);
	}
	free(str);

	return;
}


