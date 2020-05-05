#pragma once

#include <QString>
#include <QList>

#include "spec.h"

class MarkupText {
	public:
		enum Emph {
			EmphNone, EmphLesser, EmphGreater
		};

		void newline();
		void append(const QString &text);
		int beginMarkup(spec::Value *value = nullptr);
		void endMarkup(int id);
		int beginEmph(Emph emph);
		void endEmph(int id);

	private:
		struct Markup {
			int begin = 0;
			int end = 0;
			spec::Value *value = nullptr;
		};

		struct Format {
			int begin = 0;
			int end = 0;

		};

		QList<Markup> _markups;
		QStringList _lines;
		bool _overlapping;
};

class TextFragmenter {
	public:
		TextFragmenter(const QString &text);
		QStringList lines() const;
		QString text() const;

		bool word(int p, int w, int *start = nullptr, int *end = nullptr) const; //returns, whether word exists.
		bool word(int pos, int *p = nullptr, int *w = nullptr) const;
		int words() const;
		int paragraphs() const;
//		int line(int pos, int *linepos = nullptr); //linepos: position in line, where 'pos' can be found

	private:
		struct Word {
			//indices: in utf16 encoded
			int begin;
			int end;
			int pidx;
			int widx;
		};

		void update();

		QString _text;
		QStringList _lines;
//		QMap<int, int> _linepos; //[end position] -> [line index]; use _linepos.upperBounds(globalPos) to get iterator to line, where globalPos character can be found.
		QVector<Word> _words;
		int _npar;
//		QMap<int, int> _par
};

class NameListParser {

};

class CommentTextParser {

};

