#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BUF_CHUNK_SIZE 4096

typedef struct buffer buffer;
struct buffer {
	size_t cap, len;
	char *data;
};


void read_file(FILE *f, buffer *buf) {
	while(1) {
		size_t available = buf->cap - buf->len;
		// Reallocate buffer if no space is available
		if (available <= 1) {
			buf->data = realloc(buf->data, buf->cap + BUF_CHUNK_SIZE);
			for (int i = buf->cap; i < buf->cap + BUF_CHUNK_SIZE; i++) {
				buf->data[i] = 0;
			}
			buf->cap += BUF_CHUNK_SIZE;
		}

		fgets(&buf->data[buf->len], available, f);
		// fgets doesn't return the number of characters read, so strlen is used.
		size_t n = strlen(&buf->data[buf->len]);
		buf->len += n; 
		
		// If the number of bytes read is less than n, that means we either hit
		// a newline or the EOF.
		if (n < available && feof(f)) {
			return; 
		}	
	}
}