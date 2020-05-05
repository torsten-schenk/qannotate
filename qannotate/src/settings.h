#pragma once

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>

class SettingsDialog : public QDialog {
	Q_OBJECT
	public:
		SettingsDialog(QWidget *parent = nullptr);

	public slots:
		void updateDirInfo();
		void editingFinished();
		void initFolder();
		void saveSettings();
		void browseFolder();

	private:
		QSettings _settings;
		QLabel *_langLabel;
		QComboBox *_langComboBox;
		QLineEdit *_dirLineEdit;
		QPushButton *_browseButton;
		QPushButton *_initButton;
		QLabel *_versionLabel;
		QLabel *_projectLabel;
		QPushButton *_okButton;
		QPushButton *_cancelButton;
		QCheckBox *_clearBox;
		IniFile *_iniFile;
};