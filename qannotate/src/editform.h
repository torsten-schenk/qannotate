#pragma once

#include <QWidget>
#include <QByteArray>
#include <QLabel>
#include <QVBoxLayout>
#include <QtGui>
#include <QtWidgets>

#include "qtfe/qtfe.h"
#include "treedata.h"
#include "types.h"
#include "editwidgets.h"

using namespace spec;

class CustomTabStyle : public QProxyStyle
{
	public:
		QSize sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const;
		void drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const;
};

class MetadataEditForm : public QWidget {
	Q_OBJECT
	public:
		MetadataEditForm(TreeItemContext *context, QByteArray key, EditForm *parent = nullptr);
};

class TranslationEditForm : public QWidget {
	Q_OBJECT
	public:
		TranslationEditForm(TreeItemContext *context, QByteArray key, EditForm *parent = nullptr);
};

class EditForm : public QWidget {
	Q_OBJECT
	public:
		EditForm(TreeItemContext *context, DataItem *item, curspec::LanguageId langId);

		DataItem *item() {return _item;}
		curspec::LanguageId langId() {return _langId;}
		void changesMade();
		void commitChanges();

	public slots:
		void tabChanged(int index);
		void annContextMenu(const QPoint &);

	private:
		DataItem *_item;
		curspec::LanguageId _langId;
		TreeItemContext *_context;
		MetadataEditForm *_metadataEditForm;
		TranslationEditForm *_translationEditForm;
		ContentEdit *_contentEdit;
		QTabWidget *_tabWidget;
		bool _changesMade;
};
