#include <QFileDialog>
#include <QMdiArea>
#include <QDebug>
#include <QIcon>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QLayout>

#include "mainwindow.h"
#include "converter.h"
#include "types.h"
#include "settings.h"
#include "util.h"

MainWindow::MainWindow(QWidget *parent)
	:	QMainWindow(parent),
		_centralWidget(nullptr)
{
	_ui.setupUi(this);
	this->setWindowIcon(QIcon(":/images/logo_icon.svg"));
	this->setWindowTitle("QAnnotate - Cassiodor");

	connect(_ui.action_Open, SIGNAL(triggered(bool)), this, SLOT(handleOpen()));
	connect(_ui.action_Close, SIGNAL(triggered(bool)), this, SLOT(handleClose()));
	connect(_ui.action_Exit, SIGNAL(triggered(bool)), this, SLOT(handleExit()));
	connect(_ui.actionConverter, SIGNAL(triggered(bool)), this, SLOT(handleConverter()));
	connect(_ui.actionSearch, SIGNAL(triggered(bool)), this, SLOT(handleSearch()));
	connect(_ui.action_Settings, SIGNAL(triggered(bool)), this, SLOT(handleSettings()));
	connect(_ui.actionAbout, SIGNAL(triggered(bool)), this, SLOT(handleAbout()));
	connect(_ui.actionDump_Tree_to_Console, SIGNAL(triggered(bool)), this, SLOT(handleDumpTreeToConsole()));
	connect(_ui.actionDump_Backend_to_Console, SIGNAL(triggered(bool)), this, SLOT(handleDumpFrontendToConsole()));

	try {
		QSettings settings(QSettings::IniFormat, QSettings::UserScope, "annotate/mainwindow");
		QString presetDir = settings.value("presetDirectory").toString();
		if (!presetDir.isEmpty()) {
			IniFile iniFile(presetDir);
			if (iniFile.fileOk()) {
				_centralWidget = new CentralWidget(presetDir, this);
				this->setCentralWidget(_centralWidget);
				this->statusBar()->showMessage("Preset directory opened, this can be changed in Settings", 2000);
			} else {
				this->statusBar()->showMessage("Preset directory not valid (maybe wrong project), this can be changed in Settings", 2000);
			}
		}
	} catch (const Exception &ex) {
		this->statusBar()->showMessage("Error opening preset directory: " + ex.message(), 2000);
	}
}

void MainWindow::handleOpen()
{
	try {
		QString dirname = QFileDialog::getExistingDirectory(this, "Open folder", QString(), QFileDialog::ShowDirsOnly);
		if(dirname.isEmpty()) {
			return;
		}
		IniFile iniFile(dirname);
		if (!iniFile.fileOk()) {
			this->statusBar()->showMessage("Directory not valid (maybe wrong project), this can be changed in Settings", 2000);
			return;
		}
		this->handleClose();
		_centralWidget = new CentralWidget(dirname, this);
		this->setCentralWidget(_centralWidget);
	} catch (const Exception &ex) {
		this->statusBar()->showMessage("Error opening folder: " + ex.message(), 2000);
	}
}

void MainWindow::handleClose()
{
	if (_centralWidget) {
		_centralWidget->closeAllTabs();
		delete _centralWidget;
		_centralWidget = nullptr;
	}
}

void MainWindow::handleExit()
{
	this->handleClose();
	this->close();
}

void MainWindow::closeEvent (QCloseEvent *event)
{
	this->handleClose();
	event->accept();
}

void MainWindow::handleDumpTreeToConsole()
{
	if (_centralWidget) {
		_centralWidget->dumpTree();
	}
}

void MainWindow::handleDumpFrontendToConsole()
{
	if (_centralWidget) {
		_centralWidget->dumpFrontend();
	}
}

void MainWindow::handleConverter()
{
	Converter *converter = new Converter(this);
	converter->show();
}

void MainWindow::handleSearch()
{
	if (_centralWidget) {
		_centralWidget->searchItems();
	}
}

void MainWindow::handleSettings()
{
	SettingsDialog *settingsDialog = new SettingsDialog(this);

	settingsDialog->exec();
}

void MainWindow::handleAbout()
{
	QWidget *aboutDialog = new QWidget(this);
	aboutDialog->resize(600,300);
	aboutDialog->setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);
	aboutDialog->setWindowTitle("About QAnnotate");
	aboutDialog->setWindowIcon(QIcon(":/images/logo_icon.svg"));
	QVBoxLayout *vLayout = new QVBoxLayout(aboutDialog);
	QTextBrowser *tb = new QTextBrowser(aboutDialog);
	tb->setMinimumSize(QSize(700, 200));
	tb->setOpenExternalLinks(true);
	QString qtVersion = QT_VERSION_STR;
	tb->setHtml("<center><img src=\"qrc:/images/logo.png\"></center><p style=\"text-align: center; font-weight: bold;\">Version 0.9.8<br>Compiled with Qt " + qtVersion + "<br>Customized for Cassiodor-project</p>"); // TODO fix ugly old style html workaround for non working css (magin: auto;)
	vLayout->addWidget(tb);
	QPushButton *okButton = new QPushButton("Close",aboutDialog);
	aboutDialog->setTabOrder(okButton, tb);
	okButton->setAutoDefault(true);
	vLayout->addWidget(okButton);
	connect(okButton, SIGNAL(clicked()), aboutDialog, SLOT(close()));
	aboutDialog->setLayout(vLayout);
	aboutDialog->show();
}