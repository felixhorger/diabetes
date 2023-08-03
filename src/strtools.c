
void debug(char *str, size_t s) {
	for (int i = 0; i < s; i++) printf("%c", str[i]);
	return;
}

char* find(char c, char *start, char *stop) {
	if (start == NULL || stop == NULL) return NULL;
	int incr = (start < stop) ? 1 : -1;
	while ((size_t)(start) * incr <= (size_t)(stop) * incr && *start != '\0' && *start != c) start += incr;
	if (stop == start || *start != c) return NULL;
	else return start;
}

//char* findfirstnot(int (*notfound)(int), char *str, char *stop, int incr) {
//	if (incr != 1 && incr != -1) return NULL;
//	if (str == NULL || stop == NULL || (size_t)(stop) * incr <= (size_t)(str) * incr) return NULL;
//	while (str != stop && *str != '\0' && notfound(*str)) str += incr;
//	return str;
//}






/*
	Looks for a char c in str, that matches *str in a sense
	that *str and c occur in pairs.
*/
char* find_matching(char* str, char c) {
	char c0 = *str;
	int count = 0;
	do {
		if (*str == c0) count++;
		else if (*str == c) count--;
		str++;
	} while (*str != '\0' && count != 0);
	str--;
	if (*str != c) return NULL;
	else return str;
}



//void find_matching_braces(char *str, size_t n, char **opening, char **closing) {
//	*opening = find('{', str, str+n);
//	if (**opening != '{') {
//		printf("Error: opening brace not found\n");
//		exit(1);
//	}
//	*closing = find_matching(*opening, '}');
//	if (*closing != NULL && **closing != '}') {
//		printf("Error: closing brace not found\n");
//		exit(1);
//	}
//	return;
//}

