#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "base32.h"

static const unsigned char enc_table[32] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
	'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
	'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
	'Y', 'Z', '2', '3', '4', '5', '6', '7'
};

static const unsigned char dec_table[128] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,  0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16,  0x17, 0x18, 0x19, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static inline uint8_t bits(
		uint8_t value,
		uint8_t srcbit,
		uint8_t destbit,
		int nbit)
{
	int shift = destbit - srcbit;
	value &= ((1 << nbit) - 1) << (srcbit - nbit + 1);
	if(shift < 0)
		return value >> -shift;
	else
		return value << shift;
}

size_t base32_encsize(
		size_t n)
{
	size_t base = n / 5 * 8;
	switch(n % 5) {
		case 0: return base + 0;
		case 1: return base + 2;
		case 2: return base + 4;
		case 3: return base + 5;
		case 4: return base + 7;
	}
	return base;
}

ssize_t base32_decsize(
		size_t n)
{
	ssize_t base = n / 8 * 5;
	switch(n % 8) {
		case 0: return base + 0;
		case 2: return base + 1;
		case 4: return base + 2;
		case 5: return base + 3;
		case 7: return base + 4;
		default: return -1;
	}
}

void base32_enc(
		char *di,
		const void *src,
		size_t n)
{
	unsigned char carry;
	const unsigned char *si = src;

	while(n >= 5) {
		*di++ = enc_table[bits(si[0], 7, 4, 5)];
		*di++ = enc_table[bits(si[0], 2, 4, 3) | bits(si[1], 7, 1, 2)];
		*di++ = enc_table[bits(si[1], 5, 4, 5)];
		*di++ = enc_table[bits(si[1], 0, 4, 1) | bits(si[2], 7, 3, 4)];
		*di++ = enc_table[bits(si[2], 3, 4, 4) | bits(si[3], 7, 0, 1)];
		*di++ = enc_table[bits(si[3], 6, 4, 5)];
		*di++ = enc_table[bits(si[3], 1, 4, 2) | bits(si[4], 7, 2, 3)];
		*di++ = enc_table[bits(si[4], 4, 4, 5)];

		si += 5;
		n -= 5;
	}

	if(n >= 1) {
		*di++ = enc_table[bits(si[0], 7, 4, 5)];
		carry = bits(si[0], 2, 4, 3);
	}
	if(n >= 2) {
		*di++ = enc_table[carry | bits(si[1], 7, 1, 2)];
		*di++ = enc_table[bits(si[1], 5, 4, 5)];
		carry = bits(si[1], 0, 4, 1);
	}
	if(n >= 3) {
		*di++ = enc_table[carry | bits(si[2], 7, 3, 4)];
		carry = bits(si[2], 3, 4, 4);
	}
	if(n >= 4) {
		*di++ = enc_table[carry | bits(si[3], 7, 0, 1)];
		*di++ = enc_table[bits(si[3], 6, 4, 5)];
		carry = bits(si[3], 1, 4, 2);
	}
	if(n)
		*di = enc_table[carry];
}

int base32_dec(
		void *dest,
		const char *si)
{
	char *di = dest;
	unsigned char block[8];
	unsigned char tmp;
	unsigned char carry = 0;
	int bidx = 0;

	while(*si > 0) {
		tmp = dec_table[(int)*si];
		if(tmp == 0xff) {
			errno = EINVAL;
			return -1;
		}

		block[bidx++] = tmp;
		if(bidx == 8) {
			*di++ = bits(block[0], 4, 7, 5) | bits(block[1], 4, 2, 3);
			*di++ = bits(block[1], 1, 7, 2) | bits(block[2], 4, 5, 5) | bits(block[3], 4, 0, 1);
			*di++ = bits(block[3], 3, 7, 4) | bits(block[4], 4, 3, 4);
			*di++ = bits(block[4], 0, 7, 1) | bits(block[5], 4, 6, 5) | bits(block[6], 4, 1, 2);
			*di++ = bits(block[6], 2, 7, 3) | bits(block[7], 4, 4, 5);
			bidx = 0;
		}
		si++;
	}
	if(bidx >= 2) {
		*di++ = bits(block[0], 4, 7, 5) | bits(block[1], 4, 2, 3);
		carry = bits(block[1], 1, 7, 2);
	}
	if(bidx >= 4) {
		*di++ = carry | bits(block[2], 4, 5, 5) | bits(block[3], 4, 0, 1);
		carry = bits(block[3], 3, 7, 4);
	}
	if(bidx >= 5) {
		*di++ = carry | bits(block[4], 4, 3, 4);
		carry = bits(block[4], 0, 7, 1);
	}
	if(bidx >= 7) {
		*di++ = carry | bits(block[5], 4, 6, 5) | bits(block[6], 4, 1, 2);
		carry = bits(block[6], 2, 7, 3);
	}
	if(carry || bidx == 1 || bidx == 3 || bidx == 6) {
		errno = EINVAL;
		return -1;
	}
	else
		return 0;
}

