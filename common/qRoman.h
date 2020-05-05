#pragma once

static inline void _digitRoman(int &value, int mul, char one, char five, char ten, char *seq) {
	int digit = value / mul;
	value -= digit * mul;
	switch(digit) {
		case 1: seq[0] = one; seq[1] = 0; break;
		case 2: seq[0] = one; seq[1] = one; seq[2] = 0; break;
		case 3: seq[0] = one; seq[1] = one; seq[2] = one; seq[3] = 0; break;
		case 4: seq[0] = one; seq[1] = five; seq[2] = 0; break;
		case 5: seq[0] = five; seq[1] = 0; break;
		case 6: seq[0] = five; seq[1] = one; seq[2] = 0; break;
		case 7: seq[0] = five; seq[1] = one; seq[2] = one; seq[3] = 0; break;
		case 8: seq[0] = five; seq[1] = one; seq[2] = one; seq[3] = one; seq[4] = 0; break;
		case 9: seq[0] = one; seq[1] = ten; seq[2] = 0; break;
		default: seq[0] = 0; break;
	}
}

static inline QString toRomanNumeral(int value) {
	char tmp[5];
	QString roman;
	while(value >= 1000) {
		roman.append('M');
		value -= 1000;
	}
	_digitRoman(value, 100, 'C', 'D', 'M', tmp);
	roman.append(tmp);
	_digitRoman(value, 10, 'X', 'L', 'C', tmp);
	roman.append(tmp);
	_digitRoman(value, 1, 'I', 'V', 'X', tmp);
	roman.append(tmp);
	return roman;
}

