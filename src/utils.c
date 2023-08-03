
int min(int a, int b) {
	return a < b ? a : b;
}

void catch_segfault(void *ptr1, void *ptr2, char *msg) {
	if (ptr1 != NULL && ptr1 < ptr2) return;
	printf("Error: caught segmentation fault: %s\n", msg);
	exit(1);
	return;
}
