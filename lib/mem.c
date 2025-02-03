#include <stddef.h>
#include <mpx/vm.h>
#include <memory.h>

#define roundup(s) ((s) & (END|FREE) ? ((s) & ~(END|FREE)) + 4 : (s))

struct mcb *memlist = NULL;

void initialize_heap(size_t sz)
{
	sz = roundup(sz);
	memlist = kmalloc(sz + sizeof(*memlist), 0, NULL);
	memlist->size = sz | FREE | END;
}

void *allocate_memory(size_t sz)
{
	sz = roundup(sz);
	for (struct mcb *mcb = memlist; mcb != NULL; mcb = mcb_next(mcb)) {
		if (mcb_size(mcb) >= sz && mcb_isfree(mcb)) {
			if (mcb_size(mcb) > sz + sizeof(*mcb)) {
				struct mcb *newfree = (void*)(mcb->start + sz);
				newfree->size = mcb_size(mcb) - sz - sizeof(*mcb);
				mcb_setfree(newfree);
				mcb_copyend(newfree, mcb);
				mcb->size = sz;
			}
			mcb_clrfree(mcb);
			return mcb->start;
		}
	}
	return NULL;
}

int free_memory(void *ptr)
{
	struct mcb *prev = NULL;
	struct mcb *mcb = memlist;
	struct mcb *next = mcb_next(mcb);

	while (mcb != NULL) {
		if (mcb->start == ptr && !mcb_isfree(mcb)) {
			mcb_setfree(mcb);
			if (next && mcb_isfree(next)) {
				mcb->size += mcb_size(next) + sizeof(*mcb);
				mcb_copyend(mcb, next);
			}
			if (prev && mcb_isfree(prev)) {
				prev->size += mcb_size(mcb) + sizeof(*mcb);
				mcb_copyend(prev, mcb);
			}
			return 0;
		}

		prev = mcb;
		mcb = next;
		next = mcb_next(mcb);
	}
	return -1;
}

struct mcb *mcb_next(struct mcb *mcb)
{
	return mcb_isend(mcb) ? NULL : (void*)(mcb->start + mcb_size(mcb));
}
