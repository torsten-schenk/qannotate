#pragma once

#include "common/qException.h"

class UtfException : public Exception {
	public:
		UtfException(const QString &message) : Exception(message) {}
};

class UtfIndexer {
	public:
		static int utf16to8(const QString &string, int index16 = -1) {
			int index8 = 0;
			int i = 0;
			while(i < string.size()) {
				if(i >= index16 && index16 >= 0)
					return index8;
				uint utf32 = next32(string, i);
				index8 += utf8size(utf32);
			}
			return index8;
		}

		static int utf8to16(const QString &string, int index8 = -1) {
			int index16 = 0;
			while(index16 < string.size()) {
				int prev = index16;
				uint utf32 = next32(string, index16);
				int size = utf8size(utf32);
				if(index8 >= 0 && index8 < size)
					return prev;
				index8 -= size;
			}
			return index16;
		}

		static int utf8to32(const QString &string, int index8);
		static int utf16to32(const QString &string, int index16);
		static int utf32to8(const QString &string, int index32);
		static int utf32to16(const QString &string, int index32);

	private:
		static uint next32(const QString &string, int &index) {
			uint utf32;
			QChar c = string.at(index++);
			if(c.isHighSurrogate()) {
				if(index == string.size())
					throw UtfException("incomplete utf16 string");
				QChar l = string.at(index++);
				if(!l.isLowSurrogate())
					throw UtfException("invalid utf16 character sequence");
				utf32 = QChar::surrogateToUcs4(c, l);
			}
			else if(c.isLowSurrogate()) {
				throw UtfException("invalid utf16 character sequence");
			}
			else
				utf32 = c.unicode();
			if(utf32 >= 0x110000)
				throw UtfException("invalid character");
			return utf32;
		}

		int utf8size(uint utf32) {
			int size = 1;
			if(utf32 >= 0x80)
				size++;
			if(utf32 >= 0x800)
				size++;
			if(utf32 >= 0x10000)
				size++;
			return size;
		}
};

