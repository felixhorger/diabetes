
typedef struct BlockArray BlockArray;
struct BlockArray
{
	BlockArray *prev;
	BlockArray *next;
	size_t n;
};


void newblock(BlockArray *a, size_t blocksize)
{
	BlockArray *b = (BlockArray *)malloc(sizeof(BlockArray) + blocksize);
	a->next = b;
	b->next = NULL;
	b->prev = a;
	b->size = 0;
	return;
}


void nextelem(BlockArray **a, char **pos, char *(*next)(char *, size_t))
{
	BlockArray *b = *a;
	char *nextpos = (*next)(*pos, b->n);
	if (nextpos >= b->data + BLOCKSIZE) {
		b = b->next;
		if (b == NULL) return;
		*a   = b->next;
		*pos = b->n + 1; // sneaky
	}
	return;
}


bool findelem(BlockArray **a, char **pos, char *val, bool (*match)(char *, char* ), char *(*next)(char *, size_t))
{
	BlockArray *b = *a;
	char *ptr = *pos;
	if (ptr == NULL) ptr = b->data;
	while (true) {
		if (match(val, ptr)) break;
		nextelem(&b, &ptr, next);
		if (b == NULL) break;
	}
	if (b == NULL) return false;
	*a = b;
	*pos = ptr;
	return true;
}


void pushelem(BlockArray **a, char **pos, char *val, char *(*next)(char *, size_t))
{
	char *ptr = *pos;
	if (ptr == NULL) ptr = a->data;

	BlockArray *b = *a;

	while (true) {
	array is full

	}

	while not reached last element

	int i;
	for (i = 0; i < a->n; i++) {
		if (match(val, pos)) break;
		pos = nextelem(a, pos, next);
	}
	if (i == a->n) return NULL
	return pos;
	pos = 
	pos = nextelem(a, pos, next);
}


