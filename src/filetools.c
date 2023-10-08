
void check_file(FILE *f) {
	if (feof(f)) printf("Error: read EOF\n");
	else if (ferror(f)) printf("Error: read occurred while reading file\n");
	else return;
	exit(1);
}

void safe_fread(void* ptr, size_t n, FILE* f) { // TODO: change this, put f first arg
	size_t m = fread(ptr, 1, n, f);
	if (m == n) return;
	check_file(f);
	return;
}
