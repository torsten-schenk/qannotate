#pragma once

#include <QByteArray>

static inline void qHexdump(const QByteArray &array, const char *prefix = nullptr) {
	size_t cur;
	size_t i;
	size_t address = 0;
	const unsigned char *base = (const unsigned char*)array.data();
	size_t bytes = array.size();
	while(bytes > 0) {
		cur = qMin(bytes, size_t(16));
		if(prefix)
			printf("%s", prefix);
		printf("%08x  ", (unsigned int)address);
		for(i = 0; i < cur; i++) {
			if(i == 8)
				printf(" ");
			printf("%02x ", base[i]);
		}
		for(; i < 16; i++) {
			if(i == 8)
				printf(" ");
			printf("   ");
		}
		printf(" |");
		for(i = 0; i < cur; i++)
			if(base[i] <= 0x20 || base[i] >= 0x7f)
				printf(".");
			else
				printf("%c", base[i]);
		printf("|\n");
		bytes -= cur;
		address += cur;
		base += cur;
	}
}

static inline void qHexdump(const void *data, size_t bytes, const char *prefix = nullptr) {
	size_t cur;
	size_t i;
	size_t address = 0;
	const unsigned char *base = (const unsigned char*)data;
	while(bytes > 0) {
		cur = qMin(bytes, size_t(16));
		if(prefix)
			printf("%s", prefix);
		printf("%08x  ", (unsigned int)address);
		for(i = 0; i < cur; i++) {
			if(i == 8)
				printf(" ");
			printf("%02x ", base[i]);
		}
		for(; i < 16; i++) {
			if(i == 8)
				printf(" ");
			printf("   ");
		}
		printf(" |");
		for(i = 0; i < cur; i++)
			if(base[i] <= 0x20 || base[i] >= 0x7f)
				printf(".");
			else
				printf("%c", base[i]);
		printf("|\n");
		bytes -= cur;
		address += cur;
		base += cur;
	}
}

