// TODO: use more
void check_pointer(void *ptr, const char *fmt, ...)
{
	if (ptr != NULL) return;

	printf("Error: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	printf("\n");

	exit(EXIT_FAILURE);
}

void check_bounds(int i, size_t n, char* msg)
{
	if (i > -1 && i < n) return;
	printf("Error: out of bounds access [%d/%lu] in %s\n", i+1, n, msg);
	exit(EXIT_FAILURE);
	return;
}

void check_file(FILE *f)
{
	if (f == NULL) printf("Error: file == NULL");
	else if (feof(f)) printf("Error: read EOF\n");
	else if (ferror(f)) printf("Error: read occurred while reading file\n");
	else return;

	exit(EXIT_FAILURE);
}



void safe_fread(FILE* f, void* ptr, size_t n)
{
	check_file(f);
	size_t m = fread(ptr, 1, n, f);
	if (m == n) return;
	check_file(f);
	return;
}



void debug(char *str, size_t s)
{
	for (int i = 0; i < s; i++) printf("%c", str[i]);

	printf("\n");
	return;
}

char* find(char c, char *start, char *stop)
{
	if (start == NULL || stop == NULL) return NULL;

	int incr = (start < stop) ? 1 : -1;
	while ((size_t)(start) * incr < (size_t)(stop) * incr && *start != '\0' && *start != c) start += incr;

	if (stop == start || *start != c) return NULL;
	else return start;
}


// Looks for a char c in str, that matches *str in a sense that *str and c occur in pairs.
// str* means the character str points to, not whole string
char* find_matching(char *str, char c)
{
	char c0 = *str;
	int count = 1;
	do {
		str++;
		if (*str == c0) count++;
		else if (*str == c) count--;
	} while (*str != '\0' && count != 0);

	if (*str != c) return NULL;
	else return str;
}

