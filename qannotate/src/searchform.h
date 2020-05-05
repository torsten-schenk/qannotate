#pragma once

#include <QRegExp>

#include "ui_searchform.h"

class SearchResultItem : public QTableWidgetItem {
	public:
		SearchResultItem(QTreeWidgetItem *dataItem, curspec::LanguageId langId, const QString &prop, const QString &val);

		QTreeWidgetItem *dataItem() {return _dataItem;};
		curspec::LanguageId langId() {return _langId;};
		QTableWidgetItem *langTableItem() {return _langTableItem;};
		QTableWidgetItem *propTableItem() {return _propTableItem;};
		QTableWidgetItem *valTableItem() {return _valTableItem;};

	private:
		QTreeWidgetItem *_dataItem;
		curspec::LanguageId _langId;
		QTableWidgetItem *_langTableItem;
		QTableWidgetItem *_propTableItem;
		QTableWidgetItem *_valTableItem;
};


class CheckBoxWithId : public QCheckBox {
	public:
		CheckBoxWithId(const QString &text, curspec::LanguageId id, QWidget *parent);
		curspec::LanguageId id() {return _id;};

	private:
		curspec::LanguageId _id;
};


class SearchForm : public QWidget {
	Q_OBJECT
	public:
		SearchForm(TreeItemContext *treeItemContext, QWidget *parent = nullptr);

	public slots:
		void checkBoxAllClicked(bool state);
		void setCheckBoxAllState();
		void search();
		void searchInItem(QTreeWidgetItem *treeWidgetItem, bool searchAnnots = false);
		void createSearchResult(const KeyEditor &keyEd, const QIcon &icon, QTreeWidgetItem *treeWidgetItem, curspec::LanguageId langId, const QString &propIdText);
		void itemDoubleClicked(QTableWidgetItem *item);

	private:
		Ui::SearchForm _ui;
		TreeItemContext *_treeItemContext;
		QStatusBar *_statusBar;
		int _resultCount;
		QRegExp _regExp;
		Qt::CaseSensitivity _caseSensitivity;
		QIcon _personIcon;
		QIcon _locationIcon;
		QIcon _bibliographyIcon;
		QIcon _philIcon;
		QIcon _histIcon;
		QIcon _letterIcon;
};