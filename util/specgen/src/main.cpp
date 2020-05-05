#include <QCommandLineParser>
#include <QCoreApplication>

static constexpr char wordsep = '-';

#include "common/printer.hpp"

#include "model.hpp"
#include "loader.hpp"
#include "generator.hpp"

int main(int argn, char **argv)
{
	QCoreApplication app(argn, argv);
	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addPositionalArgument("source", "XML source file containing the spec");
	parser.addPositionalArgument("target", "Output header file");
	parser.process(app);

	if(parser.positionalArguments().size() != 2) {
		printf("Invalid arguments\n");
		parser.showHelp();
		return 1;
	}
	
	QString source = parser.positionalArguments().at(0);
	QString target = parser.positionalArguments().at(1);
	XmlModelLoader loader;
	Model model;
	if(!loader.parse(source, new DirectLoader(&model))) {
		printf("Error parsing xml file\n");
		return 1;
	}
	Generator generator(&model);
	if(!generator.generate(target)) {
		printf("Error generating output\n");
		return 1;
	}
	return 0;
}

