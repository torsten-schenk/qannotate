#pragma once

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t base32_encsize(
		size_t n);

ssize_t base32_decsize(
		size_t n);

int base32_check(
		const char *base32);

void base32_enc(
		char *dest,
		const void *src,
		size_t n);

int base32_dec(
		void *dest,
		const char *si);

#ifdef __cplusplus
}
#endif

