#pragma once

#include <QTreeWidgetItem>
#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>

#include "qtfe/qtfe.h"
#include "types.h"
#include "editform.h"

using namespace spec;

class LetterInitDialog : public QDialog
{
	Q_OBJECT
	public:
		LetterInitDialog(QWidget *parent = nullptr);

		quint64 letterId() {return _letterId;}
		QStringList letterText() {return _letterText;}

	public slots:
		void onAccept();

	private:
		QTextEdit *_textEdit;
		QLineEdit *_lineEdit;
		quint64 _letterId;
		QStringList _letterText;
};

class BaseItem : public QTreeWidgetItem {
	public:
		BaseItem(TreeItemContext *context, QTreeWidgetItem *parent);

		virtual void execContextMenu(const QPoint &point) = 0;
		void dump();
		bool operator <(const QTreeWidgetItem &other) const;

	protected:
		TreeItemContext *_context;
};

class ParentItem { // Item for that children can be created
	public:
		ParentItem(TreeItemContext *context, QTreeWidgetItem *parent);

		virtual void createChildItem() = 0;
		void dump();
};

class DataItem { // Item for that an EditForm can be opened
	public:
		DataItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id);

		const spec::DeclKey *declKey;

		quint64 id(){return _id;}
		void dump();

		QByteArray key() {return _key;}

	protected:
		QByteArray _key;
		quint64 _id;
};

class PersonRootItem : public BaseItem, public ParentItem {
	public:
		PersonRootItem(TreeItemContext *context, QTreeWidgetItem *parent);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
};

class PersonItem : public BaseItem, public DataItem {
	public:
		PersonItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, bool init = false);

		void execContextMenu(const QPoint &point) override;

	private:
		QStandardItem *_completerItem;
};

class LocationRootItem : public BaseItem, public ParentItem {
	public:
		LocationRootItem(TreeItemContext *context, QTreeWidgetItem *parent);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
};

class LocationGroupItem : public BaseItem, public ParentItem {
	public:
		LocationGroupItem(TreeItemContext *context, QTreeWidgetItem *parent, quint8 id);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
		QString objName();

	private:
		quint8 _id;
};

class LocationItem : public BaseItem, public DataItem {
	public:
		LocationItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, QString groupName, bool init = false);

		void execContextMenu(const QPoint &point) override;

	private:
		QString _groupName;
		QStandardItem *_completerItem;
};

class BibliographyRootItem : public BaseItem, public ParentItem {
	public:
		BibliographyRootItem(TreeItemContext *context, QTreeWidgetItem *parent);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
};

class BibliographyGroupItem : public BaseItem, public ParentItem {
	public:
		BibliographyGroupItem(TreeItemContext *context, QTreeWidgetItem *parent, quint8 id);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
		QString objName();

	private:
		quint8 _id;
};

class BibliographyItem : public BaseItem, public DataItem {
	public:
		BibliographyItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, QString groupName, bool init = false);

		void execContextMenu(const QPoint &point) override;

	private:
		QString _groupName;
		QStandardItem *_completerItem;
};

class PhilRootItem : public BaseItem, public ParentItem {
	public:
		PhilRootItem(TreeItemContext *context, QTreeWidgetItem *parent);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
};

class PhilItem : public BaseItem, public DataItem {
	public:
		PhilItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, bool init = false);

		void execContextMenu(const QPoint &point) override;

	private:
		QStandardItem *_completerItem;
};

class HistRootItem : public BaseItem, public ParentItem {
	public:
		HistRootItem(TreeItemContext *context, QTreeWidgetItem *parent);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
};

class HistGroupItem : public BaseItem, public ParentItem {
	public:
		HistGroupItem(TreeItemContext *context, QTreeWidgetItem *parent, quint8 id);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
		QString objName();

	private:
		quint8 _id;
};

class HistItem : public BaseItem, public DataItem {
	public:
		HistItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, QString groupName, bool init = false);

		void execContextMenu(const QPoint &point) override;

	private:
		QString _groupName;
		QStandardItem *_completerItem;
};

class TextRootItem : public BaseItem, public ParentItem {
	public:
		TextRootItem(TreeItemContext *context, QTreeWidgetItem *parent);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
};

class BookItem : public BaseItem, public ParentItem {
	public:
		BookItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
		bool operator<(const QTreeWidgetItem &other) const;
		QByteArray key() {return _key;}

	private:
		QByteArray _key;
		quint64 _id;
};

class LetterItem : public BaseItem, public ParentItem, public DataItem {
	public:
		LetterItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
		void initTextContent();
		bool operator<(const QTreeWidgetItem &other) const;

	private:
		quint64 _bookId;
};

class IntroRootItem : public BaseItem, public ParentItem {
	public:
		IntroRootItem(TreeItemContext *context, QTreeWidgetItem *parent);

		void execContextMenu(const QPoint &point) override;
		void createChildItem() override;
};

class IntroItem : public BaseItem, public DataItem {
	public:
		IntroItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, bool init = false);

		void execContextMenu(const QPoint &point) override;

	private:
		QStandardItem *_completerItem;
};
