#include <QFormLayout>
#include <QScrollArea>
#include <QTabWidget>
#include <QDebug>
#include <QInputDialog>

#include "qtfe/qtfe.h"
#include "common/qHexdump.h"
#include "editform.h"
#include "editwidgets.h"
#include "treedata.h"
#include "mainwindow.h"
#include "types.h"

using namespace spec;

QSize CustomTabStyle::sizeFromContents(ContentsType type, const QStyleOption *option, const QSize &size, const QWidget *widget) const
{
	QSize s = QProxyStyle::sizeFromContents(type, option, size, widget);
	if (type == QStyle::CT_TabBarTab) {
		s.transpose();
	}
	return s;
}

void CustomTabStyle::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const
{
	if (element == CE_TabBarTabLabel) {
		if (const QStyleOptionTab *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
			QStyleOptionTab opt(*tab);
			opt.shape = QTabBar::RoundedNorth;
			QProxyStyle::drawControl(element, &opt, painter, widget);
			return;
		}
	}
	QProxyStyle::drawControl(element, option, painter, widget);
}

MetadataEditForm::MetadataEditForm(TreeItemContext *context, QByteArray key, EditForm *parent)
	:	QWidget(parent)
{
	QVBoxLayout *vLayout = new QVBoxLayout(this);
	QScrollArea *scrollArea = new QScrollArea(this);
	scrollArea->setWidgetResizable(true);
	QWidget *scrollContent = new QWidget();
	QHBoxLayout *hLayout = new QHBoxLayout(scrollContent);

	QFormLayout *fLayout = new QFormLayout();
	KeyEditor keyEd(&curspec::meta, key);
	for(size_t i = 0; i < EnumInfo<curspec::TextPropertyId>::N; i++) {
		QString propIdText = ((QString) EnumInfo<curspec::TextPropertyId>::text(i)) + ":";
		keyEd.enumPut(curspec::TextMetadata::PropertyId, EnumInfo<curspec::TextPropertyId>::fromIndex(i));
		QByteArray propKey = (QByteArray) keyEd;

		const DeclValue *mapto = keyEd.mapto();
		if (mapto == &curspec::textLine) {
			QHBoxLayout *bBox = new QHBoxLayout();
			LineEdit *lineEdit = new LineEdit(context, parent, keyEd, this);
			bBox->addWidget(lineEdit);
			QPushButton *bButton = new QPushButton(scrollContent);
			bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
			connect(bButton, SIGNAL(clicked()), lineEdit, SLOT(enlarge()));
			bBox->addWidget(bButton);
			curspec::TextLine *value = context->frontend->get<curspec::TextLine>(keyEd);
			if (value) {
				lineEdit->setText((QString) value->text);
			}
			fLayout->addRow(propIdText,bBox);
		} else if (mapto == &curspec::textMultiline) {
			QHBoxLayout *bBox = new QHBoxLayout();
			TextEdit *textEdit = new TextEdit(context, parent, keyEd, this);
			bBox->addWidget(textEdit);
			QPushButton *bButton = new QPushButton(scrollContent);
			bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
			connect(bButton, SIGNAL(clicked()), textEdit, SLOT(enlarge()));
			bBox->addWidget(bButton);
			curspec::TextMultiline *value = context->frontend->get<curspec::TextMultiline>(keyEd);
			if (value) {
				textEdit->blockSignals(true);
				textEdit->setPlainText(value->text.join("\n"));
				textEdit->blockSignals(false);
			}
			fLayout->addRow(propIdText,bBox);
		} else {
			Q_ASSERT(false);
		}
	}
	hLayout->addLayout(fLayout);
	scrollArea->setWidget(scrollContent);
	vLayout->addWidget(scrollArea);
}

TranslationEditForm::TranslationEditForm(TreeItemContext *context, QByteArray key, EditForm *parent)
	:	QWidget(parent)
{
	QVBoxLayout *vLayout = new QVBoxLayout(this);
	KeyEditor keyEd(&curspec::meta, key);

	TextEdit *textEdit = new TextEdit(context, parent, keyEd, this);
	curspec::TextMultiline *value = context->frontend->get<curspec::TextMultiline>(keyEd, true);
	if (value) {
		textEdit->blockSignals(true);
		textEdit->setPlainText(value->text.join("\n"));
		textEdit->blockSignals(false);
	}
	vLayout->addWidget(textEdit);
}

EditForm::EditForm(TreeItemContext *context, DataItem *item, curspec::LanguageId langId)
	:	QWidget(context->tabWidget),
		_item(item),
		_langId(langId),
		_context(context),
		_contentEdit(nullptr),
		_changesMade(false)
{
	this->resize(916, 644);
	QVBoxLayout *vLayout = new QVBoxLayout(this);

	KeyEditor keyEd(&curspec::meta);
	if (item->declKey == &curspec::personObject) {
		QScrollArea *scrollArea = new QScrollArea(this);
		scrollArea->setWidgetResizable(true);
		QWidget *scrollContent = new QWidget();
		scrollContent->setGeometry(QRect(0, 0, 896, 624));
		QHBoxLayout *hLayout = new QHBoxLayout(scrollContent);
		QFormLayout *fLayout = new QFormLayout();

		keyEd.select(&curspec::personObject);
		keyEd.uintPut(curspec::PersonObject::ObjectId, item->id());
		keyEd.enumPut(curspec::PersonObject::LanguageId,langId);
		for(size_t i = 0; i < EnumInfo<curspec::PersonPropertyId>::N; i++) {
			QString propIdText = ((QString) EnumInfo<curspec::PersonPropertyId>::text(i)) + ":";
			keyEd.enumPut(curspec::PersonObject::PropertyId, EnumInfo<curspec::PersonPropertyId>::fromIndex(i));
			const DeclValue *mapto = keyEd.mapto();
			if (mapto == &curspec::textLine) {
				QHBoxLayout *bBox = new QHBoxLayout();
				LineEdit *lineEdit = new LineEdit(_context, this, keyEd, this);
				bBox->addWidget(lineEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), lineEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextLine *value = _context->frontend->get<curspec::TextLine>(keyEd);
				if (value) {
					lineEdit->setText((QString) value->text);
				}
				fLayout->addRow(propIdText,bBox);
			} else if (mapto == &curspec::textMultiline) {
				QHBoxLayout *bBox = new QHBoxLayout();
				TextEdit *textEdit = new TextEdit(_context, this, keyEd, this);
				bBox->addWidget(textEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), textEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd);
				if (value) {
					textEdit->blockSignals(true);
					textEdit->setPlainText(value->text.join("\n"));
					textEdit->blockSignals(false);
				}
				fLayout->addRow(propIdText,bBox);
			} else if (curspec::optionValueType(mapto)) {
				KeyEditor optEd(&curspec::meta, keyEd);
				optEd.uintPut(curspec::PersonObject::LanguageId, enumNull);
				OptionEdit *optionEdit = new OptionEdit(_context, this, optEd, curspec::optionValueType(mapto), this);
				fLayout->addRow(propIdText,optionEdit);
			}
		}
		hLayout->addLayout(fLayout);
		scrollArea->setWidget(scrollContent);
		vLayout->addWidget(scrollArea);
	} else if (item->declKey == &curspec::locationObject) {
		QScrollArea *scrollArea = new QScrollArea(this);
		scrollArea->setWidgetResizable(true);
		QWidget *scrollContent = new QWidget();
		scrollContent->setGeometry(QRect(0, 0, 896, 624));
		QHBoxLayout *hLayout = new QHBoxLayout(scrollContent);
		QFormLayout *fLayout = new QFormLayout();

		keyEd.select(&curspec::locationObject);
		keyEd.uintPut(curspec::LocationObject::ObjectId, item->id());
		keyEd.enumPut(curspec::LocationObject::LanguageId,langId);
		for(size_t i = 0; i < EnumInfo<curspec::LocationPropertyId>::N; i++) {
			QString propIdText = ((QString) EnumInfo<curspec::LocationPropertyId>::text(i)) + ":";
			keyEd.enumPut(curspec::LocationObject::PropertyId, EnumInfo<curspec::LocationPropertyId>::fromIndex(i));

			const DeclValue *mapto = keyEd.mapto();
			if (mapto == &curspec::textLine) {
				QHBoxLayout *bBox = new QHBoxLayout();
				LineEdit *lineEdit = new LineEdit(_context, this, keyEd, this);
				bBox->addWidget(lineEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), lineEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextLine *value = _context->frontend->get<curspec::TextLine>(keyEd);
				if (value) {
					lineEdit->setText((QString) value->text);
				}
				fLayout->addRow(propIdText,bBox);
			} else if (mapto == &curspec::textMultiline) {
				QHBoxLayout *bBox = new QHBoxLayout();
				TextEdit *textEdit = new TextEdit(_context, this, keyEd, this);
				bBox->addWidget(textEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), textEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd);
				if (value) {
					textEdit->blockSignals(true);
					textEdit->setPlainText(value->text.join("\n"));
					textEdit->blockSignals(false);
				}
				fLayout->addRow(propIdText,bBox);
			} else {
				Q_ASSERT(false);
			}
		}
		hLayout->addLayout(fLayout);
		scrollArea->setWidget(scrollContent);
		vLayout->addWidget(scrollArea);
	} else if (item->declKey == &curspec::bibliographyObject) {
		QScrollArea *scrollArea = new QScrollArea(this);
		scrollArea->setWidgetResizable(true);
		QWidget *scrollContent = new QWidget();
		scrollContent->setGeometry(QRect(0, 0, 896, 624));
		QHBoxLayout *hLayout = new QHBoxLayout(scrollContent);
		QFormLayout *fLayout = new QFormLayout();

		keyEd.select(&curspec::bibliographyObject);
		keyEd.uintPut(curspec::BibliographyObject::ObjectId, item->id());
		keyEd.enumPut(curspec::BibliographyObject::LanguageId,langId);
		for(size_t i = 0; i < EnumInfo<curspec::BibliographyPropertyId>::N; i++) {
			QString propIdText = ((QString) EnumInfo<curspec::BibliographyPropertyId>::text(i)) + ":";
			keyEd.enumPut(curspec::BibliographyObject::PropertyId, EnumInfo<curspec::BibliographyPropertyId>::fromIndex(i));

			const DeclValue *mapto = keyEd.mapto();
			if (mapto == &curspec::textLine) {
				QHBoxLayout *bBox = new QHBoxLayout();
				LineEdit *lineEdit = new LineEdit(_context, this, keyEd, this);
				bBox->addWidget(lineEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), lineEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextLine *value = _context->frontend->get<curspec::TextLine>(keyEd);
				if (value) {
					lineEdit->setText((QString) value->text);
				}
				fLayout->addRow(propIdText,bBox);
			} else if (mapto == &curspec::textMultiline) {
				QHBoxLayout *bBox = new QHBoxLayout();
				TextEdit *textEdit = new TextEdit(_context, this, keyEd, this);
				bBox->addWidget(textEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), textEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd);
				if (value) {
					textEdit->blockSignals(true);
					textEdit->setPlainText(value->text.join("\n"));
					textEdit->blockSignals(false);
				}
				fLayout->addRow(propIdText,bBox);
			} else {
				Q_ASSERT(false);
			}
		}
		hLayout->addLayout(fLayout);
		scrollArea->setWidget(scrollContent);
		vLayout->addWidget(scrollArea);
	} else if (item->declKey == &curspec::philCommentObject) {
		QScrollArea *scrollArea = new QScrollArea(this);
		scrollArea->setWidgetResizable(true);
		QWidget *scrollContent = new QWidget();
		scrollContent->setGeometry(QRect(0, 0, 896, 624));
		QHBoxLayout *hLayout = new QHBoxLayout(scrollContent);
		QFormLayout *fLayout = new QFormLayout();

		keyEd.select(&curspec::philCommentObject);
		keyEd.uintPut(curspec::PhilCommentObject::ObjectId, item->id());
		keyEd.enumPut(curspec::PhilCommentObject::LanguageId,langId);
		for(size_t i = 0; i < EnumInfo<curspec::CommentPropertyId>::N; i++) {
			QString propIdText = ((QString) EnumInfo<curspec::CommentPropertyId>::text(i)) + ":";
			keyEd.enumPut(curspec::PhilCommentObject::PropertyId, EnumInfo<curspec::CommentPropertyId>::fromIndex(i));

			const DeclValue *mapto = keyEd.mapto();
			if (mapto == &curspec::textLine) {
				QHBoxLayout *bBox = new QHBoxLayout();
				LineEdit *lineEdit = new LineEdit(_context, this, keyEd, this);
				bBox->addWidget(lineEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), lineEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextLine *value = _context->frontend->get<curspec::TextLine>(keyEd);
				if (value) {
					lineEdit->setText((QString) value->text);
				}
				fLayout->addRow(propIdText,bBox);
			} else if (mapto == &curspec::textMultiline) {
				QHBoxLayout *bBox = new QHBoxLayout();
				TextEdit *textEdit = new TextEdit(_context, this, keyEd, this);
				bBox->addWidget(textEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), textEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd);
				if (value) {
					textEdit->blockSignals(true);
					textEdit->setPlainText(value->text.join("\n"));
					textEdit->blockSignals(false);
				}
				fLayout->addRow(propIdText,bBox);
			} else {
				Q_ASSERT(false);
			}
		}
		hLayout->addLayout(fLayout);
		scrollArea->setWidget(scrollContent);
		vLayout->addWidget(scrollArea);
	} else if (item->declKey == &curspec::histCommentObject) {
		QScrollArea *scrollArea = new QScrollArea(this);
		scrollArea->setWidgetResizable(true);
		QWidget *scrollContent = new QWidget();
		scrollContent->setGeometry(QRect(0, 0, 896, 624));
		QHBoxLayout *hLayout = new QHBoxLayout(scrollContent);
		QFormLayout *fLayout = new QFormLayout();

		keyEd.select(&curspec::histCommentObject);
		keyEd.uintPut(curspec::HistCommentObject::ObjectId, item->id());
		keyEd.enumPut(curspec::HistCommentObject::LanguageId,langId);
		for(size_t i = 0; i < EnumInfo<curspec::CommentPropertyId>::N; i++) {
			QString propIdText = ((QString) EnumInfo<curspec::CommentPropertyId>::text(i)) + ":";
			keyEd.enumPut(curspec::HistCommentObject::PropertyId, EnumInfo<curspec::CommentPropertyId>::fromIndex(i));

			const DeclValue *mapto = keyEd.mapto();
			if (mapto == &curspec::textLine) {
				QHBoxLayout *bBox = new QHBoxLayout();
				LineEdit *lineEdit = new LineEdit(_context, this, keyEd, this);
				bBox->addWidget(lineEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), lineEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextLine *value = _context->frontend->get<curspec::TextLine>(keyEd);
				if (value) {
					lineEdit->setText((QString) value->text);
				}
				fLayout->addRow(propIdText,bBox);
			} else if (mapto == &curspec::textMultiline) {
				QHBoxLayout *bBox = new QHBoxLayout();
				TextEdit *textEdit = new TextEdit(_context, this, keyEd, this);
				bBox->addWidget(textEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), textEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd);
				if (value) {
					textEdit->blockSignals(true);
					textEdit->setPlainText(value->text.join("\n"));
					textEdit->blockSignals(false);
				}
				fLayout->addRow(propIdText,bBox);
			} else {
				Q_ASSERT(false);
			}
		}
		hLayout->addLayout(fLayout);
		scrollArea->setWidget(scrollContent);
		vLayout->addWidget(scrollArea);
	} else if (item->declKey == &curspec::textLetter) {
		keyEd.select(&curspec::textContent);
		keyEd.uintPut(curspec::TextContent::LetterId, item->id());
		curspec::TextAnnotated *value = _context->frontend->get<curspec::TextAnnotated>(keyEd);
		if (value) {
			QSplitter *splitter = new QSplitter();
			vLayout->addWidget(splitter);
			_contentEdit = new ContentEdit(_context, keyEd, value, this);
			_contentEdit->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(_contentEdit,SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(annContextMenu(const QPoint &)));
			splitter->addWidget(_contentEdit);
			_tabWidget = new QTabWidget(this);
			_tabWidget->setTabPosition(QTabWidget::West);
			_tabWidget->tabBar()->setStyle(new CustomTabStyle);
			_tabWidget->tabBar()->setStyleSheet("QTabBar::tab { font-size: 13px; height: 170%; width: 30px;} QTabBar::tab:selected { font-weight: bold; }");
			splitter->addWidget(_tabWidget);
			splitter->setOrientation(Qt::Vertical);

			keyEd.select(&curspec::textTranslation);
			keyEd.enumPut(curspec::TextTranslation::LanguageId, langId);
			keyEd.uintPut(curspec::TextTranslation::LetterId, item->id());
			_translationEditForm = new TranslationEditForm(_context, keyEd, this);
			_tabWidget->addTab(_translationEditForm,"Translation");

			keyEd.select(&curspec::textMetadata);
			keyEd.enumPut(curspec::TextMetadata::LanguageId,langId);
			keyEd.uintPut(curspec::TextMetadata::LetterId, item->id());
			_metadataEditForm = new MetadataEditForm(_context, keyEd, this);
			_tabWidget->addTab(_metadataEditForm,"Metadata");

			for (unsigned int i = 0; i < EnumInfo<curspec::AnnotatedType>::N; i++) {
				CategoryEdit *cEdit = new CategoryEdit(_context, this, _contentEdit, value, index2enum(i), keyEd);
				_tabWidget->addTab(cEdit,EnumInfo<curspec::AnnotatedType>::text(i));
			}
			connect(_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
		}
	} else if (item->declKey == &curspec::introObject) {
		QScrollArea *scrollArea = new QScrollArea(this);
		scrollArea->setWidgetResizable(true);
		QWidget *scrollContent = new QWidget();
		scrollContent->setGeometry(QRect(0, 0, 896, 624));
		QHBoxLayout *hLayout = new QHBoxLayout(scrollContent);
		QFormLayout *fLayout = new QFormLayout();

		keyEd.select(&curspec::introObject);
		keyEd.uintPut(curspec::IntroObject::ObjectId, item->id());
		keyEd.enumPut(curspec::IntroObject::LanguageId,langId);
		for(size_t i = 0; i < EnumInfo<curspec::IntroPropertyId>::N; i++) {
			QString propIdText = ((QString) EnumInfo<curspec::IntroPropertyId>::text(i)) + ":";
			keyEd.enumPut(curspec::IntroObject::PropertyId, EnumInfo<curspec::IntroPropertyId>::fromIndex(i));

			const DeclValue *mapto = keyEd.mapto();
			if (mapto == &curspec::textLine) {
				QHBoxLayout *bBox = new QHBoxLayout();
				LineEdit *lineEdit = new LineEdit(_context, this, keyEd, this);
				bBox->addWidget(lineEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), lineEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextLine *value = _context->frontend->get<curspec::TextLine>(keyEd);
				if (value) {
					lineEdit->setText((QString) value->text);
				}
				fLayout->addRow(propIdText,bBox);
			} else if (mapto == &curspec::textMultiline) {
				QHBoxLayout *bBox = new QHBoxLayout();
				TextEdit *textEdit = new TextEdit(_context, this, keyEd, this);
				bBox->addWidget(textEdit);
				QPushButton *bButton = new QPushButton(scrollContent);
				bButton->setIcon(QIcon(":/images/magnifier_icon.svg"));
				connect(bButton, SIGNAL(clicked()), textEdit, SLOT(enlarge()));
				bBox->addWidget(bButton);
				curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd);
				if (value) {
					textEdit->blockSignals(true);
					textEdit->setPlainText(value->text.join("\n"));
					textEdit->blockSignals(false);
				}
				fLayout->addRow(propIdText,bBox);
			} else {
				Q_ASSERT(false);
			}
		}
		hLayout->addLayout(fLayout);
		scrollArea->setWidget(scrollContent);
		vLayout->addWidget(scrollArea);
	}
}

void EditForm::changesMade()
{
	_changesMade = true;
	_context->saveState->setUnsaved();
}

void EditForm::commitChanges()
{
	if (_changesMade) {
		try {
			KeyEditor itemKey(&curspec::meta, _item->key());
			_context->frontend->modified(_item->key());
			_changesMade = false;
			_context->saveState->setSaved();
		} catch (const Exception &ex) {
			qInfo() << "exception:" << ex.message();
		}
	}
}

void EditForm::tabChanged(int index)
{
	CategoryEdit *ce = dynamic_cast<CategoryEdit *>(_tabWidget->currentWidget());
	if (ce) {
		ce->markInit();
	} else {
		if (_contentEdit) {
			_contentEdit->reload();
		}
	}
}

void EditForm::annContextMenu(const QPoint &point)
{
	CategoryEdit *ce = dynamic_cast<CategoryEdit *>(_tabWidget->currentWidget());
	if (ce) {
		ce->contextMenu(point);
	} else {
		ContentEdit *sender = qobject_cast<ContentEdit *>(QObject::sender());
		if (sender) {
			QMenu *contextMenu = sender->createStandardContextMenu();
			contextMenu->exec(sender->mapToGlobal(point));
		}
	}
}
