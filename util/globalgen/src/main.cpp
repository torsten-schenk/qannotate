#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QCommandLineParser>
#include <QCoreApplication>

#include "common/printer.hpp"

static constexpr char wordsep = '-';

namespace {
	QString uppername(const QString &name) {
		QStringList words = name.split(wordsep);
		for(int i = 0; i < words.size(); i++)
			words[i][0] = words[i][0].toUpper();
		return words.join("");
	}
}


int main(int argn, char **argv)
{
	QCoreApplication app(argn, argv);
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addPositionalArgument("output", "Output header file with global definitions");
	parser.addPositionalArgument("headers", "Input header files", "[headers...]");
	parser.process(app);

	if(parser.positionalArguments().size() < 1) {
		printf("Invalid arguments\n");
		parser.showHelp();
		return 1;
	}

	QSet<QString> names;
	size_t maxKeyDepth = 0;
	
	QRegExp regexName("^//@name\\s+([a-zA-Z_-][a-zA-Z0-9_-]*)\\s*$");
	QRegExp regexMaxKeyDepth("^//@maxKeyDepth\\s+([0-9]+)\\s*$");
	for(int i = 1; i < parser.positionalArguments().size(); i++) {
		QString input = parser.positionalArguments().at(i);
		QFile file(input);
		if(!file.open(QIODevice::ReadOnly)) {
			printf("Error opening input file %s\n", qPrintable(input));
			return 1;
		}
		QTextStream stream(&file);
		while(!stream.atEnd()) {
			QString line = stream.readLine();
			if(stream.status() != QTextStream::Ok) {
				printf("Error reading from file %s\n", qPrintable(input));
				return 1;
			}
			if(regexName.exactMatch(line))
				names.insert(regexName.cap(1));
			else if(regexMaxKeyDepth.exactMatch(line)) {
				size_t n = regexMaxKeyDepth.cap(1).toUInt();
				if(n > maxKeyDepth)
					maxKeyDepth = n;
			}
		}
	}

	QString output = parser.positionalArguments().at(0);
	QFile file(output);
	if(!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
		printf("Error opening output file %s\n", qPrintable(output));
		return 1;
	}
	Printer p;
	QList<QString> namesList = names.toList();
	qSort(namesList);

	p.pr("enum class Name {").pni();
	p.pr("null,").pn();
	for(size_t i = 0; i < maxKeyDepth; i++)
		p.pr("_").pr(i).pr(",").pn();

	for(auto it : namesList)
		p.pr(uppername(it)).pr(",").pn();
	p.pr("};").upn();
	p.pr("static constexpr const size_t nNames = ").pr(namesList.size() + maxKeyDepth + 1).pr(";").pn();

	p.pr("#ifdef IMPLEMENTATION").pn();
	p.pr("const QString names[] = {").pni();
	p.pr("nullptr,").pn();
	for(size_t i = 0; i < maxKeyDepth; i++)
		p.pr("\"").pr(i).pr("\",").pn();
	for(auto it : namesList)
		p.pr("\"").pr(it).pr("\",").pn();
	p.pr("};").upn();
	p.pr("#endif").pn();

	IoTarget target(&file);
	p.stream(&target);
	return 0;
}

