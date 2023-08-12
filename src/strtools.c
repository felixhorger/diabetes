
void debug(char *str, size_t s) {
	for (int i = 0; i < s; i++) printf("%c", str[i]);
	printf("\n");
	return;
}

char* find(char c, char *start, char *stop) {
	if (start == NULL || stop == NULL) return NULL;
	int incr = (start < stop) ? 1 : -1;
	while ((size_t)(start) * incr <= (size_t)(stop) * incr && *start != '\0' && *start != c) start += incr;
	if (stop == start || *start != c) return NULL;
	else return start;
}


/*
	Looks for a char c in str, that matches *str in a sense
	that *str and c occur in pairs.
*/
char* find_matching(char* str, char c) {
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

