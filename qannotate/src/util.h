#pragma once

class IniFile : public QObject {
	Q_OBJECT
	public:
		IniFile(QString dirName);

		bool fileOk();
		void readIniFile(QString dirName);
		void writeIniFile(QString dirName);
		bool dirOk(){return _dir.exists();};
		QString dirName(){return _dir.path();}
		QString version(){return _version;}
		QString project(){return _project;}

	signals:
		void dirInfoChanged();

	private:
		const QString _targetVersion = "1";
		const QString _targetProject = "cassiodor";
		QDir _dir;
		QString _version;
		QString _project;
};
