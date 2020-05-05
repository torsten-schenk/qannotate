#include "text.h"

TextFragmenter::TextFragmenter(const QString &text)
	:	_text(text),
		_npar(0)
{
	update();
}

void TextFragmenter::update()
{
	enum {
		ParBegin, LineBegin, InWord, BetweenWord
	};

	int state = ParBegin;
	int nword = 0;
	int pos = 0;
	int lineno = 0;

	_lines = _text.split("\n");
	_words.clear();
	_lines.clear();
	_npar = 0;

/*	for(auto it : _lines) {
		pos += it.size() + 1; //+1: newline character
		_linepos.insert(pos, lineno++);
	}*/
	//count words
	for(int i = 0; i < _text.size(); i++) {
		ushort c = _text[i].unicode();
		if(c == '\n') {
			if(state == LineBegin)
				state = ParBegin;
			else if(state == InWord || state == BetweenWord)
				state = LineBegin;
		}
		else if(c == ' ' || c == '\t') {
			if(state == InWord)
				state = BetweenWord;
		}
		else {
			if(state == ParBegin)
				state = LineBegin;
			if(state == LineBegin || state == BetweenWord) {
				nword++;
				state = InWord;
			}
		}
	}

	//store words
	_words.resize(nword);
	state = ParBegin;
	nword = 0;
	Word word;
	word.pidx = 0;
	word.widx = 0;
	word.begin = -1;
	word.end = -1;
	for(int i = 0; i < _text.size(); i++) {
		ushort c = _text[i].unicode();
		if(c == '\n') {
			if(state == InWord) {
				word.end = i;
				_words[nword++] = word;
				state = LineBegin;
			}
			else if(state == LineBegin)
				state = ParBegin;
			else if(state == BetweenWord)
				state = LineBegin;
		}
		else if(c == ' ' || c == '\t') {
			if(state == InWord) {
				word.end = i;
				_words[nword++] = word;
				state = BetweenWord;
			}
		}
		else {
			if(state == ParBegin) {
				word.pidx++;
				word.widx = 0;
				state = LineBegin;
			}
			if(state == LineBegin || state == BetweenWord) {
				word.begin = i;
				word.widx++;
				state = InWord;
			}
		}
	}
	if(state == InWord) {
		word.end = _text.size();
		_words[nword++] = word;
	}
	_npar = word.pidx;
}

QString TextFragmenter::text() const
{
	return _text;
}

QStringList TextFragmenter::lines() const
{
	return _lines;
}

int TextFragmenter::words() const
{
	return _words.size();
}

int TextFragmenter::paragraphs() const
{
	return _npar;
}

bool TextFragmenter::word(int p, int w, int *start, int *end) const
{
	for(auto cur : _words) {
		if(cur.pidx == p && cur.widx == w) {
			if(start)
				*start = cur.begin;
			if(end)
				*end = cur.end;
			return true;
		}
	}
	return false;
}

bool TextFragmenter::word(int pos, int *p, int *w) const
{
	if(_words.isEmpty())
		return false;
	for(auto cur : _words) {
		if(cur.end >= pos) {
			if(p)
				*p = cur.pidx;
			if(w)
				*w = cur.widx;
			return true;
		}
	}
	if(p)
		*p = _words.last().pidx;
	if(w)
		*w = _words.last().widx;
	return true;
}

/*int TextFragmenter::line(int pos, int *linepos)
{
	auto it = _linepos.upperBound(pos);
	if(it == _linepos.end())
		return -1;
	if(linepos)
		*linepos = pos - it.key() + _lines.at(it.value()).size() + 1;
	return it.value();
}*/

