#include <QTextStream>
#include <QDataStream>
#include <QBuffer>
#include <QStack>

class PrinterTarget {
	public:
		PrinterTarget() : _indent(0), _overrideIndent(-1) {}

		virtual void printIndent(int n) = 0;
		virtual void printText(const QString &text) = 0;
		virtual void printNewline() = 0;

		void indent(int amount) {
			_indent += amount;
		}

		void overrideIndent(int amount) {
			_overrideIndent = amount;
		}

		void text(const QString &text) {
			_line += text;
		}

		void newline() {
			if(!_line.isEmpty()) {
				if(_overrideIndent >= 0)
					printIndent(_overrideIndent);
				else
					printIndent(_indent < _anchor.top() ? _anchor.top() : _indent);
				printText(_line);
				_line = QString();
				_overrideIndent = -1;
			}
			printNewline();
		}

		void pushAnchor() {
			_anchor.push(_indent);
		}

		void popAnchor() {
			_indent = _anchor.pop();
		}

	private:
		int _indent;
		int _overrideIndent;
		QString _line;

		QStack<int> _anchor;
};

class IoTarget : public PrinterTarget {
	public:
		IoTarget(QIODevice *io, QChar indent = '\t', QChar newline = '\n') : _stream(io), _indent(indent), _newline(newline) {}

		virtual void printIndent(int n) override {
			while(n--)
				_stream << _indent;
		}

		virtual void printText(const QString &text) override {
			_stream << text;
		}

		virtual void printNewline() override {
			_stream << _newline;
		}

	private:
		QTextStream _stream;
		QChar _indent;
		QChar _newline;
};

class TextTarget : public PrinterTarget {
	public:
		TextTarget(QTextStream &stream, QChar indent = '\t', QChar newline = '\n') : _stream(stream), _indent(indent), _newline(newline) {}

		virtual void printIndent(int n) override {
			while(n--)
				_stream << _indent;
		}

		virtual void printText(const QString &text) override {
			_stream << text;
		}

		virtual void printNewline() override {
			_stream << _newline;
		}

	private:
		QTextStream &_stream;
		QChar _indent;
		QChar _newline;
};

class PrinterSection {
	public:
		enum {
			FragText, FragSub, FragNewline, FragIndent, FragOverrideIndent
		};

		PrinterSection() {
			_buffer.open(QIODevice::ReadWrite);
		}

		~PrinterSection() {
			for(auto it : _subs)
				delete it;
		}

		PrinterSection *get(const QString &name) {
			PrinterSection *sub = _bynameSub.value(name);
			if(!sub) {
				sub = new PrinterSection();
				_subs.append(sub);
				_bynameSub.insert(name, sub);
			}
			return sub;
		}

		void newline() {
			QDataStream stream(&_buffer);
			stream << quint8(FragNewline);
		}

		void indent(int amount) {
			QDataStream stream(&_buffer);
			stream << quint8(FragIndent) << amount;
		}

		void overrideIndent(int amount) {
			QDataStream stream(&_buffer);
			stream << quint8(FragOverrideIndent) << amount;
		}

		void embed(PrinterSection *sub) {
			int id = _subs.indexOf(sub);
			Q_ASSERT(id >= 0);
			QDataStream stream(&_buffer);
			stream << quint8(FragSub) << id;
		}

		void text(const QString &text) {
			if(!text.isEmpty()) {
				QDataStream stream(&_buffer);
				stream << quint8(FragText) << text;
			}
		}

		template<typename T> void number(T num) {
			QDataStream stream(&_buffer);
			stream << quint8(FragText) << QString::number(num);
		}

		void stream(PrinterTarget *target) {
			_buffer.seek(0);
			QDataStream stream(&_buffer);
			target->pushAnchor();
			while(!stream.atEnd()) {
				quint8 type;
				stream >> type;
				if(type == FragText) {
					QString text;
					stream >> text;
					target->text(text);
				}
				else if(type == FragNewline)
					target->newline();
				else if(type == FragIndent) {
					int amount;
					stream >> amount;
					target->indent(amount);
				}
				else if(type == FragOverrideIndent) {
					int amount;
					stream >> amount;
					target->overrideIndent(amount);
				}
				else if(type == FragSub) {
					int id;
					stream >> id;
					_subs.at(id)->stream(target);
				}
				else {
					Q_ASSERT(false);
				}
			}
			target->popAnchor();
		}

	private:
		QList<PrinterSection*> _subs;
		QMap<QString, PrinterSection*> _bynameSub;
		QBuffer _buffer;
};

class Printer {
	public:
		Printer() {
			_stack.push(new PrinterSection());
		}

		Printer &embed(const QString &name) {
			PrinterSection *cur = _stack.top();
			PrinterSection *sub = cur->get(name);
			cur->embed(sub);
			return *this;
		}

		Printer &pn() {
			PrinterSection *cur = _stack.top();
			cur->newline();
			return *this;
		}

		Printer &pni() {
			PrinterSection *cur = _stack.top();
			cur->newline();
			cur->indent(1);
			return *this;
		}

		Printer &pno(int indent = 0) {
			PrinterSection *cur = _stack.top();
			cur->newline();
			cur->overrideIndent(indent);
			return *this;
		}

		Printer &pnu() {
			PrinterSection *cur = _stack.top();
			cur->newline();
			cur->indent(-1);
			return *this;
		}

		Printer &ipn() {
			PrinterSection *cur = _stack.top();
			cur->indent(1);
			cur->newline();
			return *this;
		}

		Printer &opn(int indent = 0) {
			PrinterSection *cur = _stack.top();
			cur->overrideIndent(indent);
			cur->newline();
			return *this;
		}

		Printer &upn() {
			PrinterSection *cur = _stack.top();
			cur->indent(-1);
			cur->newline();
			return *this;
		}

		Printer &indent(int amount = 1) {
			PrinterSection *cur = _stack.top();
			cur->indent(amount);
			return *this;
		}

		Printer &unindent(int amount = 1) {
			PrinterSection *cur = _stack.top();
			cur->indent(-amount);
			return *this;
		}

		Printer &pr(const QString &string) {
			PrinterSection *cur = _stack.top();
			cur->text(string);
			return *this;
		}

		Printer &pr(const char *string) {
			PrinterSection *cur = _stack.top();
			cur->text(string);
			return *this;
		}

		template<typename T> Printer &pr(T num) {
			PrinterSection *cur = _stack.top();
			cur->number(num);
			return *this;
		}

		void select(const QString &section) {
			PrinterSection *cur = _stack.top();
			PrinterSection *sub = cur->get(section);
			_stack.push(sub);
		}

		void unselect() {
			Q_ASSERT(_stack.size() > 1);
			_stack.pop();
		}

		void stream(PrinterTarget *target) {
			_stack.first()->stream(target);
			target->newline();
		}

	private:
		QStack<PrinterSection*> _stack;
};

