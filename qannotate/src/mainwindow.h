#pragma once

#include <QMainWindow>
#include <QTreeView>

#include "types.h"
#include "centralwidget.h"
#include "ui_mainwindow.h"

class MainWindow : public QMainWindow {
	Q_OBJECT
	public:
		MainWindow(QWidget *parent = nullptr);

	signals:
		void requestOpen(const QString &filename);
		void requestSave(const QString &filename);

	private slots:
		void handleOpen();
		void handleClose();
		void handleExit();
		void handleDumpTreeToConsole();
		void handleDumpFrontendToConsole();
		void handleConverter();
		void handleSearch();
		void handleSettings();
		void handleAbout();
		void closeEvent (QCloseEvent *event);

	private:
		Ui::MainWindow _ui;
		CentralWidget *_centralWidget;
};
