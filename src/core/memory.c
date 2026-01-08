#include "memory.h"

#include <string.h>

void memory_zero(void *pointer, usize size) {
	memset(pointer, 0, size);
}
