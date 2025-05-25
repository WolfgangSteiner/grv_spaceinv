#include "grv/grv_ringbuffer.h"
#include <assert.h>
#include <stdio.h>


int main(int, char**) {

	grv_ringbuffer_t buffer;
	i64 buffer_size = 16;
	grv_ringbuffer_init(&buffer, buffer_size);

	i32 prev = -1;
	i32 curr = 0;
	for (;;) {
		grv_ringbuffer_write(&buffer, (u8*)&curr, sizeof(curr));
		prev = curr;
		curr++;

		i32 read;
		i64 bytes_read = grv_ringbuffer_read(&buffer, (u8*)&read, sizeof(read));

		for (i32 i = 0; i < buffer_size/sizeof(curr); i++) {
			printf("%d  ", ((i32*)buffer.data)[i]);
		}
		printf("\n");

		assert(bytes_read == sizeof(read));
		assert(read == prev);
	}
}
