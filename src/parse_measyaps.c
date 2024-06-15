typedef struct Dictionary Dictionary;
struct Dictionary
{
	size_t n;
	size_t keyslen;
	size_t keysalloc;
	size_t valsalloc;
	char *keys; // TODO: this could be bmalloc array?
	void **vals; // TODO: this could also be like keys? Problem is if element is changed a different space required
	// TODO: need to find a better solution to the type-indicator byte, maybe separate array in here?
};

#define DICT_SEARCH (0)
#define DICT_SETVAL (1)
#define DICT_CREATE (2)
// mode == 0: ignore value, just search
// mode == 1: set value, create if necessary
// mode == 2: search and create if doesn't exist, but don't set
// key must be null-terminated

void ** measyaps_entry(Dictionary *dict, char *key, void *value, int mode)
{
	const size_t keyblocksize = 4096;
	const size_t valblocksize = 32;

	//printf("search %s %p %d\n", key, dict->vals, dict->n == 0);

	char *dictkeys = dict->keys;
	int i = 0;

	while (i < dict->n) {
		bool found = true;
		char *k = key;
		while (*dictkeys != '\0' && *k != '\0') {
			if (*dictkeys != *k) {
				found = false;
				break;
			}
			dictkeys++;
			k++;
		}
		if (found && *dictkeys == '\0' && *k == '\0') {
			if (mode == DICT_SETVAL) dict->vals[i] = value;
			//printf("found %s %p %d\n", key, dict->vals, i);
			return dict->vals + i;
		}
		while (*dictkeys != '\0') dictkeys++; // Move to next key
		dictkeys++;
		i++;
	}
	// Did not find a matching entry

	if (mode == DICT_SEARCH) return NULL;

	size_t keylen = strlen(key) + 1;
	int64_t to_allocate = dict->keyslen + keylen - dict->keysalloc;
	if (to_allocate > 0) {
		size_t l = keyblocksize;
		while (l < to_allocate) l += keyblocksize;
		dict->keysalloc += l;
		dict->keys = realloc(dict->keys, dict->keysalloc);
	}
	strcpy(dict->keys + dict->keyslen, key);
	dict->keyslen += keylen;

	i = dict->n;

	dict->n += 1;
	if (dict->n > dict->valsalloc) {
		dict->valsalloc += valblocksize;
		dict->vals = realloc(dict->vals, sizeof(void *) * dict->valsalloc); // TODO: reallocarray
	}

	dict->vals[i] = value;

	//printf("create %s %p %d\n", key, dict->vals, i);

	return dict->vals + i;
}

void parse_measyaps_line(Dictionary *dict, char *start, char *equalsign, char *eol)
{
	char *dot = find('.', start, equalsign);

	if (dot == NULL) {
		// Last key in this line, parse also value

		char *keystop = find('\t', start+1, eol);
		*keystop = '\0';

		equalsign++;
		char *valstart = equalsign + strspn(equalsign, " \t");
		size_t len = eol - valstart;
		char *val = malloc(1 + len + 1);
		val[0] = 0; // indicate it's a value and not subdict
		strncpy(val+1, valstart, len);
		val[len+1] = '\0';

		//printf("adding %s | %s\n", start, val);
		measyaps_entry(dict, start, (void *)val, DICT_SETVAL);
	}
	else {
		// Not the last key in this line, add key to current dict if necessary and create subdict

		*dot = '\0';

		Dictionary *subdict;

		void **val = measyaps_entry(dict, start, NULL, DICT_CREATE);
		if (*val == NULL) {
			char *ptr = calloc(1, 1 + sizeof(Dictionary));
			// TODO: that's nasty, any other way?
			*ptr = 1;
			*val = (void *)ptr;
			subdict = (Dictionary *)(ptr + 1);
		}
		else subdict = (Dictionary *)(((char *)*val) + 1);

		parse_measyaps_line(subdict, dot+1, equalsign, eol);
	}

	return;
}


void parse_measyaps_protocol(Parameter* parameter, char* str, size_t len)
{
	strcpy(parameter->type, "MeasYaps"); // TODO: this must be adjusted if Dictionary is used instead of ParameterList

	char signature[] = "### ASCCONV BEGIN ";
	size_t signature_length = sizeof(signature)-1;
	if (strncmp(str, signature, signature_length) != 0) {
		printf("Error: parameter string doesn't start as expected:\n");
		debug(str, signature_length);
		printf("\n");
		exit(EXIT_FAILURE);
	}

	char *start, *stop;
	start = strstr(str + signature_length, " ###") + 5;
	stop = start + len - sizeof("### ASCCONV END ###") - 133; // Mysterious offset :(

	Dictionary *dict = (Dictionary *)calloc(1, sizeof(Dictionary));

	while (start < stop) {

		char *eol = find('\n', start, stop);
		if (eol == NULL) break;

		char *equalsign = find('=', start, eol);
		if (equalsign == NULL) {
			printf("Error: could find '=' in MeasYaps line\n");
			exit(EXIT_FAILURE);
		}

		parse_measyaps_line(dict, start, equalsign, eol);

		start = eol + 1;
	}

	parameter->content = (void *)dict;

	return;
}


void free_measyaps(Dictionary *dict)
{
	free(dict->keys);

	void **vals = dict->vals;


	for (int i = 0; i < dict->n; i++) {
		char *val = (char *)vals[i];
		if (*val == 1) free_measyaps((Dictionary *)(val + 1));
		free(vals[i]);
	}

	free(vals);

	return;
}

