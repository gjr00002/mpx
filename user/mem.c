#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

enum show { ALL, ALLOCATED, FREEBLOCKS };

static int mem_show(enum show which)
{
	for (struct mcb *mcb = memlist; mcb != NULL; mcb = mcb_next(mcb)) {
		if (which == ALL) {
			printf("%s ", mcb_isfree(mcb) ? "FREE " : "ALLOC");
		}
		if (which == ALL || (which == ALLOCATED && !mcb_isfree(mcb)) || (which == FREEBLOCKS && mcb_isfree(mcb))) {
			printf("%p: %zd\n", (void*)mcb->start, mcb_size(mcb));
		}
	}
	return 0;
}

static int mem_alloc(int argc, char *argv[])
{
	for (int i = 2; i < argc; i++) {
		size_t sz = atoi(argv[i]);
		void *ptr = allocate_memory(sz);
		if (ptr == NULL) {
			printf("Allocation of %zd bytes FAILED!\n", sz);
			return 1;
		}
		printf("Allocated %zd at %p\n", sz, ptr);
	}
	return 0;
}

static int mem_free(int argc, char *argv[])
{
	for (int i = 2; i < argc; i++) {
		void *ptr = (void*)htoi(argv[i]);
		int ret = free_memory(ptr);
		if (ret == -1) {
			printf("Deallocating %p FAILED!\n", ptr);
			return 1;
		}
	}
	return 0;
}

int mem(int argc, char *argv[])
{
	if (argc < 2) {
		return mem_show(ALL);
	} else if (!strcmp(argv[1], "-a") && argc == 2) {
		return mem_show(ALLOCATED);
	} else if (!strcmp(argv[1], "-f") && argc == 2) {
		return mem_show(FREEBLOCKS);
	} else if (!strcmp(argv[1], "-A")) {
		return mem_alloc(argc, argv);
	} else if (!strcmp(argv[1], "-F")) {
		return mem_free(argc, argv);
	}

	printf("\tmem               Show all memory blocks\n");
	printf("\tmem -a            Show allocated memory\n");
	printf("\tmem -f            Show free memory\n");
	printf("\tmem -A size...    Allocate size bytes of memory\n");
	printf("\tmem -F address... Free memory at address\n");
	return 1;
}
