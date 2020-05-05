#pragma once

#include <QTreeView>
#include <QLabel>

#include "qtfe/qtfe.h"
#include "types.h"
#include "ui_centralwidget.h"
#include "mainwindow.h"
#include "editform.h"

using namespace spec;

class SaveStateIndicator : public QPushButton {
	public:
		SaveStateIndicator(QWidget *parent = nullptr);

		void setUnsaved();
		void setSaved();

	private:
		QIcon _icoUnsaved;
		QIcon _icoSaved;
		bool _saved;
};

class CentralWidget : public QWidget {
	Q_OBJECT
	public:
		CentralWidget(QString dirname, MainWindow *parent = nullptr);
		~CentralWidget();

		void triggerChanged();
		void dumpTree();
		void dumpFrontend();
		void openEditForm(QTreeWidgetItem *item, curspec::LanguageId langId);

	public slots:
		void currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
		void relabelDataItemForms(DataItem *item, QString name);
		void closeDataItemForms(DataItem *item);
		void treeWidgetContextMenu(const QPoint &point);
		void openTreeItem(QTreeWidgetItem *item);
		void tabCloseRequested(int index);
		void tabChanged(int index);
		void searchItems();
		void saveCurrentTab();
		void closeAllTabs();

	private:
		Ui::CentralWidget _ui;
		TreeItemContext _treeItemContext;
		QMap<QPair<quint64, curspec::LanguageId>, EditForm*> _editTabs;
		int _lastTabIndex;

		bool eventFilter(QObject *obj, QEvent *ev) override;

	private slots:
		void filterTreeItems();
};
