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



bool is_parameter_type(Parameter *p, char *type)
{
	check_pointer(p,    "is_parameter_type(%p, %s)", p, type);
	check_pointer(type, "is_parameter_type(p, %p)",     type);
	if (strncmp(p->type, type, PARAMETER_TYPE_LEN) == 0) return true;
	else return false;
}



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

Parameter* index_parameter_map(Parameter *p, int i) // TODO: this is crap, need more like an iterator?
{
	check_pointer(p, "index_parameter_map(NULL, i)");
	if (!is_parameter_type(p, "ParamMap")) {
		printf("Error: not a ParamMap\n");
		exit(EXIT_FAILURE);
	}
	ParameterList *list = (ParameterList *) p->content;
	if (i == -1) return &(list->p); // Special, because first element is used for type
	if (i < -1) {
		printf("Error: provided index %d is smaller than -1\n", i);
		exit(EXIT_FAILURE);
	}
	// All others follow
	for (int j = -1; j < i; j++) {
		list = list->next;
		if (list == NULL) {
			printf("Error: parameter array index %d out of bounds\n", i);
			exit(EXIT_FAILURE);
		}
	}
	return &(list->p);
}

Parameter* index_parameter_array(Parameter *p, int i)
{
	check_pointer(p, "index_parameter_array(NULL, i)");
	if (strcmp(p->type, "ParamArray") != 0) {
		printf("Error: not a ParamArray\n");
		exit(EXIT_FAILURE);
	}
	Parameter *p_array = (Parameter *) p->content;
	return index_parameter_map(p_array, i);
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
	else if (is_parameter_type(p, "ParamFunctor") || is_parameter_type(p, "PipeService")) {
		printf(" %s", ((Parameter *) p->content)->name);
	}
	printf("\n");

	return;
}



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
			exit(EXIT_FAILURE);
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
		exit(EXIT_FAILURE);
	}

	// Get name
	start += 1;
	char* closing_quote = find('\"', start, stop);
	check_pointer(closing_quote, "parse_parameter(): closing quote not found");
	size_t diff = closing_quote - start;
	if (diff > 0) {
		memcpy(p->name, start, diff);
		p->name[diff] = '\0';
	}
	else {
		p->name[0] = '\0';
	}
	return start + 2; // ">
}



// returns false if parsing list is finished
bool find_container_types(char** p_start, char** p_stop) // container == map or array
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



void copy_parameter_type(Parameter *dest, Parameter *src) // Required for building parameter arrays
{
	strcpy(dest->name, src->name);
	strcpy(dest->type, src->type);
	if (strcmp(src->type, "ParamMap") == 0) {
		ParameterList *list, *list_head;
		list_head = (ParameterList*) calloc(1, sizeof(ParameterList));
		list = list_head;
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
			list_head->prev = list;
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

	char *str = (char *) malloc(len + 1);
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



ParameterList* append_new_entry(ParameterList* list)
{
	list->next = (ParameterList*) calloc(1, sizeof(ParameterList));
	list->next->prev = list;
	list = list->next;
	return list;
}

void parse_parameter_content(); // Defined later because requires ParamArray and ParaMap functionality

void parse_parameter_list(Parameter *p, char *start, char *stop, enum parse_mode mode)
{
	// Set up list of parameters
	ParameterList *list, *list_head;
	if (mode & parse_type) {
		// For ParamMap (and parsing type of ParamArray), make new list,
		// names and types are to be filled in
		list = (ParameterList*) calloc(1, sizeof(ParameterList));
		p->content = (void*) list;
	}
	else {
		// For ParamArray, names and types have been filled already (first element)
		list = (ParameterList *)p->content;
	}
	list_head = list;
	Parameter* current_p;

	// Append all entries to list
	while (start < stop) {

		// Parse parameter name and type in case of map, or copy type for array
		if (mode & parse_type) {
			// Name and type are to be determined (map, or array type determination)
			if (!find_container_types(&start, &stop)) return;

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
			current_p = &(list->p);
			if (list == NULL) {
				printf("Error: ParamArray type definition inconsistent with value \n");
				exit(EXIT_FAILURE);
			}
		}
		list_head->prev = list;

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

void parse_parameter_array(Parameter *p, char *start, char *stop, enum parse_mode mode)
{
	// TODO: The below line is for cases where a { } is enough to signify an empty array/map instead of
	// { {} {} ...}
	if (strcmp(p->name, "RxCoilSelects") == 0 || strcmp(p->name, "aRxCoilSelectData") == 0) return;

	// Parse the type signature
	if (mode & parse_type) {
		// TODO: this breaks if it isn't last in the list, seems to not happen?
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
			exit(EXIT_FAILURE);
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

		p->content = (void *) p_array;

		// Parse array type and put into the ParamMap list as the first element
		Parameter* p_array_element = &(list->p);
		start = parse_parameter_name_type(p_array_element, start, type_closing_brace);
		parse_parameter_content(p_array_element, start, type_closing_brace, parse_type);
	}

	// Advance pointer to after the ParamArray type definition
	// This is only the case for the parent ParamArray
	if (mode & parse_type && mode & parse_content) start = type_closing_brace + 1;

	// Parse the ParamArray contents but also need to copy type
	// ParamArray inside a ParamArray?
	if (mode & parse_content) parse_parameter_content((Parameter *)p->content, start, stop, parse_content | copy_type);

	return;
}

void parse_parameter_functor(Parameter *p, char *start, char *stop, enum parse_mode mode)
{
	if (mode & parse_type) {

		// Signature
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
				exit(EXIT_FAILURE);
			}
			memcpy(p_functor->name, start, len);
			p_functor->name[len] = '\0';
			start = closing_quote + 1;
		}
	}

	// TODO: can a functor be in a ParamArray? Would this still work?
	if (mode & parse_content) parse_parameter_list((Parameter *) p->content, start, stop, mode);

	return;
}



// [start, stop] = "{...}"
void parse_parameter_content(Parameter* p, char* start, char* stop, enum parse_mode mode)
{
	if (start == NULL || stop == NULL) {
		p->name[PARAMETER_NAME_LEN] = '\0';
		printf("Error: parsing parameter %s: start (%p) or stop (%p) is NULL\n", p->name, start, stop);
		exit(EXIT_FAILURE);
	}

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
			exit(EXIT_FAILURE);
		}
		start += 1;
		stop -= 1;
	}

	if      (is_parameter_type(p, "ParamMap"))     parse_parameter_list(p, start, stop, mode);
	else if (is_parameter_type(p, "Pipe"))         parse_parameter_list(p, start, stop, mode);
	else if (is_parameter_type(p, "ParamArray"))   parse_parameter_array(p, start, stop, mode);
	else if (is_parameter_type(p, "ParamFunctor")) parse_parameter_functor(p, start, stop, mode);
	else if (is_parameter_type(p, "PipeService"))  parse_parameter_functor(p, start, stop, mode);
	else if (mode & parse_content) {
		// Atomic parameters
		if      (strcmp(p->type, "ParamString") == 0) parse_string_parameter(&(p->content), start, stop);
		else if (strcmp(p->type, "ParamLong")   == 0) parse_long_parameter  (&(p->content), start, stop);
		else if (strcmp(p->type, "ParamDouble") == 0) parse_double_parameter(&(p->content), start, stop);
		else if (strcmp(p->type, "ParamBool")   == 0) parse_bool_parameter  (&(p->content), start, stop);
		// else if (strcmp(p->type, "ParamChoice") == 0) TODO, also don't forget corresponding statement in free_parameter():
		else {
			//printf("%s\n", p->type);
			// Throw error when finished with all types?
			return;
		}
	}
	// else: happens only if mode == parse_type and type is atomic

	#ifdef DEBUG_PARAMETERS
	print_parameter(p);
	#endif

	return;
}

void parse_config_protocol(Parameter* parameter, char* str, size_t len)
{
	// Check initial signature
	char signature[] = "<XProtocol>";
	size_t signature_length = sizeof(signature)-1;
	if (strncmp(str, signature, signature_length) != 0) {
		printf("Error: parameter string doesn't start as expected:\n");
		debug(str, signature_length);
		printf("\n");
		exit(EXIT_FAILURE);
	}

	// Find scope of XProtocol
	char *start, *stop;
	start = find('{', str, str+len-1);
	stop = find('}', str+len-2, str);
	if (start == NULL || stop == NULL) {
		printf("Error: could not find enclosing braces of outer scope\n");
		exit(EXIT_FAILURE);
	}
	// Find scope of the empty-named ParamMap
	start = find('{', start+1, stop);
	stop = find_matching(start, '}');

	Parameter *p = (void *) calloc(1, sizeof(Parameter));
	strcpy(p->name, "XProtocol"); // has no name in file ... I decided to remove it from here
	strcpy(p->type, "ParamMap");
	parameter->content = (void *)p;

	parse_parameter_content(p, start, stop, parse_type | parse_content);

	return;
}

void parse_meas_protocol(Parameter* parameter, char* str, size_t len)
{
	// TODO: overlap with config protocol, outsource
	char signature[] = "<XProtocol>";
	size_t signature_length = sizeof(signature)-1;
	if (strncmp(str, signature, signature_length) != 0) {
		printf("Error: parameter string doesn't start as expected:\n");
		debug(str, signature_length);
		printf("\n");
		exit(EXIT_FAILURE);
	}

	// Find scope of XProtocol
	char *start, *stop;
	start = find('{', str, str+len-1);
	stop = find('}', str+len-2, str);
	if (start == NULL || stop == NULL) {
		printf("Error: could not find enclosing braces of outer scope\n");
		exit(EXIT_FAILURE);
	}

	// Find scope of the empty-named ParamMap
	// Skip Name, ID, Userversion, EVAStringTable
	// This is simplistic, might fail if "name" is weird
	const char needle[] = "EVAStringTable";
	do start = find('<', start+1, stop);
	while (start != NULL && strncmp(start+1, needle, sizeof(needle)-1) != 0);
	if (start == NULL) {
		printf("Error: could not find EVAStringTable signature in Meas protocol\n");
		exit(EXIT_FAILURE);
	}

	start = find('{', start+1, stop);
	start = find_matching(start, '}'); // End of EVAStringTable block
	start = find('{', start+1, stop); // Start of scope of encapsulating ParamMap with no name
	stop = find_matching(start, '}');

	Parameter *p = (void *) calloc(1, sizeof(Parameter));
	strcpy(p->name, "XProtocol"); // has no name in file ... I decided to remove it from here
	strcpy(p->type, "ParamMap");
	parameter->content = (void *)p;

	parse_parameter_content(p, start, stop, parse_type | parse_content);

	// Rest in Meas is ignored since could find anything useful in there
	return;
}

void parse_measyaps_protocol(Parameter* parameter, char* str, size_t len)
{
	char signature[] = "### ASCCONV BEGIN ";
	size_t signature_length = sizeof(signature)-1;
	if (strncmp(str, signature, signature_length) != 0) {
		printf("Error: parameter string doesn't start as expected:\n");
		debug(str, signature_length);
		printf("\n");
		exit(EXIT_FAILURE);
	}

	char *start, *stop;
	start = strstr(str + signature_length, " ###") + 4;
	stop = start + len - sizeof("### ASCCONV END ###") - 132; // Mysterious offset :(
	// TODO
	printf("Warning: Skipping MeasYaps parameter set (bogus %s)\n", stop);
	return;
}



void free_parameter(Parameter* p)
{
	//#ifdef DEBUG_PARAMETERS
	//printf("To free: %s %s\n", p->name, p->type);
	//#endif
	if (
		is_parameter_type(p, "ParamBool")   ||
		is_parameter_type(p, "ParamLong")   ||
		is_parameter_type(p, "ParamDouble")
	) return;
	else if (is_parameter_type(p, "ParamString")) free(p->content);
	else if (is_parameter_type(p, "ParamMap") || is_parameter_type(p, "Pipe")) {
		ParameterList *list, *list_head, *prev;
		list_head = (ParameterList *)p->content;
		list = list_head->prev;
		if (list_head->p.type[0] != '\0') free_parameter(&(list_head->p));
		free(list_head);
		if (list == NULL) return;
		while (list != list_head) {
			//printf("pointers in parameter list %p %p %p\n", list, list->prev, list->next);
			//printf("Current param in map %s %s\n", list->p.name, list->p.type);
			free_parameter(&(list->p));
			prev = list->prev;
			free(list);
			list = prev;
		}
		// Note: p->content is freed via list_head
	}
	else if (is_parameter_type(p, "ParamArray")) {
		if (p->content == NULL) return; // This can happen ...
		free_parameter((Parameter *)p->content);
		free(p->content);
	}
	else if (is_parameter_type(p, "ParamFunctor") || is_parameter_type(p, "PipeService")) {
		free_parameter((Parameter *)p->content);
		free(p->content);
	}
	else if (is_parameter_type(p, "NotLoaded"))    free(p->content);
	else if (is_parameter_type(p, "ReadParamSet")) free(p->content);
	else if (is_parameter_type(p, "ParamSet")) {
		free_parameter((Parameter *)p->content);
		free(p->content);
	}
	else if (is_parameter_type(p, "ParamChoice")) { /* TODO: Not implemented */ }
	else {
		printf("Error: Could not free parameter \"%s\" of type \"%s\"\n", p->name, p->type);
		exit(EXIT_FAILURE);
	}

	return;
}

