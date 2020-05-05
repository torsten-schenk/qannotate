#include <QDir>
#include <QFile>
#include <QTextStream>

#include "util.h"

IniFile::IniFile(QString dirName)
	:	_dir(dirName),
		_version(""),
		_project("")
{
	this->readIniFile(dirName);
}

bool IniFile::fileOk()
{
	return _version == _targetVersion && _project == _targetProject;
}

void IniFile::readIniFile(QString dirName)
{
	_dir.setPath(dirName);
	bool inSctnGnrl = false;
	_version = "unset";
	_project = "unset";

	QFile iniFile(dirName + QDir::separator() + "qannotate.ini");
	if (iniFile.open(QIODevice::ReadOnly)) {
		QTextStream readStream(&iniFile);
		while (!readStream.atEnd()) {
			QString line = readStream.readLine();
			line = line.simplified();
			if (!inSctnGnrl) {
				if (QString::compare(line, "[general]", Qt::CaseInsensitive) == 0) {
					inSctnGnrl = true;
				}
			} else {
				if (line.at(0) == '[') {
					inSctnGnrl = false;
				} else {
					QStringList splittedLine = line.split("=");
					if (splittedLine.size() == 2) {
						QString entry = splittedLine.at(0).simplified();
						QString value = splittedLine.at(1).simplified();
						if (entry == "version") {
							_version = value;
						} else if (entry == "project") {
							_project = value;
						}
					}
				}
			}
		}
		iniFile.close();
	}
	emit dirInfoChanged();
}

void IniFile::writeIniFile(QString dirName)
{
	_dir.setPath(dirName);

	QFile iniFile(dirName + QDir::separator() + "qannotate.ini");
	if (iniFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
		QTextStream writeStream(&iniFile);
		writeStream << "[general]\nversion = " << _targetVersion << "\nproject = " << _targetProject;
		_version = _targetVersion;
		_project = _targetProject;
	}
}
