#include <QSettings>
#include <QLayout>
#include <QFormLayout>
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

#include "qtfe/qtfe.h"
#include "types.h"
#include "util.h"

#include "settings.h"

using namespace spec;

SettingsDialog::SettingsDialog(QWidget *parent)
	:	QDialog(parent, Qt::Window),
		_settings(QSettings::IniFormat, QSettings::UserScope, "annotate/mainwindow")
{
	this->setWindowTitle("QAnnotate - Settings");
	QVBoxLayout *vLt1 = new QVBoxLayout(this);
	QFormLayout *fLt1 = new QFormLayout();
	QHBoxLayout *hLt1 = new QHBoxLayout();
	QHBoxLayout *hLt2 = new QHBoxLayout();
	QHBoxLayout *hLt3 = new QHBoxLayout();

	_langComboBox = new QComboBox(this);
	fLt1->addRow("Language:", _langComboBox);
	_dirLineEdit = new QLineEdit(this);
	_dirLineEdit->setMinimumWidth(200);
	hLt1->addWidget(_dirLineEdit);
	_browseButton = new QPushButton(QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon), QString(""), this);
	hLt1->addWidget(_browseButton);
	fLt1->addRow("Preset Directory:", hLt1);
	_versionLabel = new QLabel(this);
	hLt3->addWidget(_versionLabel);
	_projectLabel = new QLabel(this);
	hLt3->addWidget(_projectLabel);
	_initButton = new QPushButton("init", this);
	QFont font = _initButton->font();
	font.setPointSize(8);
	_initButton->setFont(font);
	_initButton->setEnabled(false);
	hLt3->addWidget(_initButton);
	fLt1->addRow("Directory Info:", hLt3);
	vLt1->addLayout(fLt1);
	_clearBox = new QCheckBox("Clear settings before", this);
	vLt1->addWidget(_clearBox);
	_okButton = new QPushButton("OK", this);
	hLt2->addWidget(_okButton);
	_cancelButton = new QPushButton("Cancel", this);
	hLt2->addWidget(_cancelButton);
	vLt1->addLayout(hLt2);

	for(unsigned int i = 0; i < EnumInfo<curspec::LanguageId>::N; i++) {
		QLocale lcle(EnumInfo<curspec::LanguageId>::name(i));
		_langComboBox->addItem(lcle.nativeLanguageName(), i);
	}

	unsigned int presetLang = _settings.value("presetLanguage").toUInt();

	if (presetLang >= EnumInfo<curspec::LanguageId>::N) {
		presetLang = 0;
	}
	_langComboBox->setCurrentIndex(presetLang);

	_iniFile = new IniFile(_settings.value("presetDirectory").toString());
	_dirLineEdit->setText(_iniFile->dirName());
	this->updateDirInfo();

	connect(_browseButton, SIGNAL(clicked()), this, SLOT(browseFolder()));
	connect(_initButton, SIGNAL(clicked()), this, SLOT(initFolder()));
	connect(_dirLineEdit, SIGNAL(editingFinished()), this, SLOT(editingFinished()));
	connect(_okButton, SIGNAL(clicked()), this, SLOT(saveSettings()));
	connect(_cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
	connect(_iniFile, SIGNAL(dirInfoChanged()), this, SLOT(updateDirInfo()));
}

void SettingsDialog::editingFinished()
{
	_iniFile->readIniFile(_dirLineEdit->text());
}

void SettingsDialog::updateDirInfo()
{
	_initButton->setEnabled(!_iniFile->fileOk());
	_versionLabel->setText("Version = " + _iniFile->version());
	_projectLabel->setText("Project = " + _iniFile->project());
}

void SettingsDialog::browseFolder()
{
	try {
		QString dirName = QFileDialog::getExistingDirectory(this, "Open folder", _iniFile->dirName(), QFileDialog::ShowDirsOnly);
		if(dirName.isEmpty()) {
			return;
		}
		_iniFile->readIniFile(dirName);
		_dirLineEdit->setText(dirName);
	} catch (const Exception &ex) {
		QMessageBox::warning(this, "QAnnotate - Warning", "Error opening folder:\n" + ex.message());
	}
}

void SettingsDialog::initFolder()
{
	_iniFile->writeIniFile(_dirLineEdit->text());
	_iniFile->readIniFile(_dirLineEdit->text());
}

void SettingsDialog::saveSettings()
{
	QString presetDir = _iniFile->dirName();
	if (_iniFile->fileOk()) {
		_settings.setValue("presetDirectory", presetDir);
	}
	_settings.setValue("presetLanguage", _langComboBox->currentIndex());
	this->accept();
}
