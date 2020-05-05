#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QProxyStyle>

#include "types.h"

#include "qtfe/qtfe.h"

using namespace spec;

class EnlargeDialog : public QDialog
{
	public:
		EnlargeDialog(const QString &title, const QString &content, QWidget *parent);

		QString text();
	private:
		QPlainTextEdit *_text;
};

class AnnotEdit {};

class LineEdit : public QLineEdit {
	Q_OBJECT
	public:
		LineEdit(TreeItemContext *context, EditForm *editForm, QWidget *parent = nullptr);
		LineEdit(TreeItemContext *context, EditForm *editForm, QByteArray key, QWidget *parent = nullptr);

	public slots:
		void enlarge();

	protected:
		void keyPressEvent(QKeyEvent *e) override;
		void focusInEvent(QFocusEvent *e) override;

	private:
		TreeItemContext *_context;
		EditForm *_editForm;
		QByteArray _propertyKey;
		QWidget *_parent;

	private slots:
		void insertCompletion(const QString &completion);
		void edited(const QString& string);
};

class TextEdit : public QPlainTextEdit {
	Q_OBJECT
	public:
		TextEdit(TreeItemContext *context, EditForm *editForm, QWidget *parent = nullptr);
		TextEdit(TreeItemContext *context, EditForm *editForm, QByteArray key, QWidget *parent = nullptr);

	public slots:
		void enlarge();

	protected:
		void keyPressEvent(QKeyEvent *e) override;
		void focusInEvent(QFocusEvent *e) override;
		virtual void wheelEvent(QWheelEvent *e) override;

	private:
		TreeItemContext *_context;
		EditForm *_editForm;
		QByteArray _propertyKey;
		QWidget *_parent;

	private slots:
		void insertCompletion(const QString &completion);
		void finished();
};

class OptionEdit : public QWidget {
	Q_OBJECT
	public:
		OptionEdit(TreeItemContext *context, EditForm *editForm, QByteArray key, const DeclType *optionValueType, QWidget *parent);

	private slots:
		void checkboxChanged(int state);
		void comboChanged(int index);

	private:
		TreeItemContext *_context;
		EditForm *_editForm;
		QByteArray _propertyKey;
		QComboBox *_comboBox;
		QCheckBox *_checkBox;
		QWidget *_parent;
};

class AnchorNoFocus : public QProxyStyle
{
	public:
		int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const;
};

class ContentEdit : public QTextBrowser {
	Q_OBJECT
	public:
		ContentEdit(TreeItemContext *context, QByteArray key, curspec::TextAnnotated *value, QWidget *parent = nullptr);

		TextFragmenter *textFragmenter;

		void unmark();
		void reload();
		QByteArray propertyKey() {return _propertyKey;}

	private:
		TreeItemContext *_context;
		QByteArray _propertyKey;
		QTextCharFormat _styleNormal;
		QWidget *_parent;

		bool eventFilter(QObject *watched, QEvent *ev);

	private slots:
		void finished();
};

class CategoryEdit : public QWidget {
	Q_OBJECT
	public:
		CategoryEdit(TreeItemContext *context, EditForm *editForm, ContentEdit *annTextEdit, curspec::TextAnnotated *value, quint8 annotatedType, QByteArray key);

		void markInit();
		void contextMenu(const QPoint &point);

	public slots:
		void anchorClicked(const QUrl &link);
		void valueChanged(int value);
		void annotChanged();
		void deleteComment();
		void puncOptionChanged(int state);

	private:
		TreeItemContext *_context;
		EditForm *_editForm;
		QByteArray _key;
		QVector <CommentItem*> _comments;
		ContentEdit *_contentEdit;
		curspec::TextAnnotated *_value;
		quint8 _annotatedType;
		const DeclValue *_mapto;
		QWidget *_annot;
		QSlider *_slider;
		QLabel *_infoText;
		QCheckBox *_puncOptionBox;
		QPushButton *_deleteButton;

		QTextCharFormat _styleHilite1;
		QTextCharFormat _styleHilite2;
};

class CommentItem : public QWidget {
	Q_OBJECT
	public:
		CommentItem(ContentEdit *annTextEdit, int pbegin, int wbegin, int pend, int wend, quint8 punc, quint32 id);

		bool operator<(const CommentItem &other) const;
		void updatePos(int pbegin, int wbegin, int pend, int eWord);
		void updatePunc(quint8 punc);
		void mark(QTextCharFormat style);
		quint32 id() {return _id;}
		quint8 punc() {return _punc;}

	private:
		ContentEdit *_contentEdit;
		int _pbegin;
		int _wbegin;
		int _pend;
		int _wend;
		quint8 _punc;
		quint32 _id;
};
