#include <QMimeData>
#include <QLineEdit>
#include <QDebug>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QSlider>
#include <QScrollArea>
#include <QtGui>
#include <QtWidgets>

#include "qtfe/qtfe.h"
#include "mainwindow.h"
#include "editwidgets.h"
#include "treedata.h"

using namespace spec;

static void trimList(QStringList &strLst)
{
	for (auto &str : strLst) {
		str = str.trimmed();
	}
}

EnlargeDialog::EnlargeDialog(const QString &title, const QString &content, QWidget *parent)
	:	QDialog(parent, Qt::Window),
		_text(new QPlainTextEdit(content))
{
	this->setWindowTitle(title);
	QVBoxLayout *vBox = new QVBoxLayout(this);
	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

	vBox->addWidget(_text);
	vBox->addWidget(buttonBox);
	this->setLayout(vBox);

	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString EnlargeDialog::text()
{
	return _text->toPlainText();
}

LineEdit::LineEdit(TreeItemContext *context, EditForm *editForm, QWidget *parent)
	:	QLineEdit(parent),
		_context(context),
		_editForm(editForm),
		_propertyKey(QByteArray()),
		_parent(parent)
{
	connect(_context->idCompleter, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
}

LineEdit::LineEdit(TreeItemContext *context, EditForm *editForm, QByteArray key, QWidget *parent)
	:	QLineEdit(parent),
		_context(context),
		_editForm(editForm),
		_propertyKey(key),
		_parent(parent)
{
	connect(_context->idCompleter, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
	connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(edited(const QString&)));
}

void LineEdit::insertCompletion(const QString& completion)
{
	if (_context->idCompleter->widget() != this) {
		return;
	}
	this->deselect();
	this->cursorWordForward(false);
	int extra = completion.length() - _context->idCompleter->completionPrefix().length();
	this->insert(completion.right(extra));
}

void LineEdit::focusInEvent(QFocusEvent *e)
{
	_context->idCompleter->setWidget(this);
	QLineEdit::focusInEvent(e);
}

void LineEdit::keyPressEvent(QKeyEvent *e)
{
	if (_context->idCompleter && _context->idCompleter->popup()->isVisible()) {
		switch (e->key()) {
			case Qt::Key_Enter:
			case Qt::Key_Return:
			case Qt::Key_Escape:
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				e->ignore();
				return;
			default:
				break;
		}
	}

	bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);
	if (isShortcut) {
		int endPos = this->cursorPosition();
		this->cursorWordBackward(false);
		QString completionPrefix = this->text().mid(this->cursorPosition(), endPos);
		this->setCursorPosition(endPos);

		if (completionPrefix.length()) {
			if (completionPrefix != _context->idCompleter->completionPrefix()) {
				_context->idCompleter->setCompletionPrefix(completionPrefix);
				_context->idCompleter->popup()->setCurrentIndex(_context->idCompleter->completionModel()->index(0, 0));
			}
			QRect cr = cursorRect();
			cr.setWidth(_context->idCompleter->popup()->sizeHintForColumn(0) + _context->idCompleter->popup()->verticalScrollBar()->sizeHint().width());
			_context->idCompleter->complete(cr);
		}
	} else {
		_context->idCompleter->popup()->hide();
		QLineEdit::keyPressEvent(e);
	}
}

void LineEdit::edited(const QString& string)
{
	curspec::TextLine *value = _context->frontend->get<curspec::TextLine>(_propertyKey, true);
	if (value) {
		value->text = this->text().simplified();
		_editForm->changesMade();
	}
}

void LineEdit::enlarge()
{
	KeyEditor ed(&curspec::meta, _propertyKey);
	QString name = "Input";
	const DeclField *field = ed.decl()->findField("property-id");
	if (field) {
		name = ed.tostringAt(field->index, true);
	}
	name += ":";
	EnlargeDialog *enlargeDialog = new EnlargeDialog(name, this->text(), this);
	if (enlargeDialog->exec() == QDialog::Accepted) {
		QString text = enlargeDialog->text();
		text.replace("\n", " ");
		this->setText(text.simplified());
		this->edited(text);
	}
}

TextEdit::TextEdit(TreeItemContext *context, EditForm *editForm, QWidget *parent)
	:	QPlainTextEdit(parent),
		_context(context),
		_editForm(editForm),
		_propertyKey(QByteArray()),
		_parent(parent)
{
	this->setStyleSheet(
		"TextEdit{ border: 2px solid black;}"
	);
	connect(_context->idCompleter, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
}

TextEdit::TextEdit(TreeItemContext *context, EditForm *editForm, QByteArray key, QWidget *parent)
	:	QPlainTextEdit(parent),
		_context(context),
		_editForm(editForm),
		_propertyKey(key),
		_parent(parent)
{
	this->setStyleSheet(
		"TextEdit{ border: 2px solid black;}"
	);
	connect(_context->idCompleter, SIGNAL(activated(QString)), this, SLOT(insertCompletion(QString)));
	connect(this, SIGNAL(textChanged()), this, SLOT(finished()));
}

void TextEdit::insertCompletion(const QString& completion)
{
	if (_context->idCompleter->widget() != this) {
		return;
	}
	QTextCursor tc = this->textCursor();
	int extra = completion.length() - _context->idCompleter->completionPrefix().length();
	tc.movePosition(QTextCursor::Left);
	tc.movePosition(QTextCursor::EndOfWord);
	tc.insertText(completion.right(extra));
	this->setTextCursor(tc);
}

void TextEdit::focusInEvent(QFocusEvent *e)
{
	_context->idCompleter->setWidget(this);
	QPlainTextEdit::focusInEvent(e);
}

void TextEdit::wheelEvent(QWheelEvent *e)
{
	if(e->modifiers() & Qt::ControlModifier) {
		int delta = e->delta();
		if(delta < 0) {
			this->zoomOut(2);
		} else {
			this->zoomIn(2);
		}
		return;
	}
	QPlainTextEdit::wheelEvent(e);
}

void TextEdit::keyPressEvent(QKeyEvent *e)
{
	if (_context->idCompleter && _context->idCompleter->popup()->isVisible()) {
		switch (e->key()) {
			case Qt::Key_Enter:
			case Qt::Key_Return:
			case Qt::Key_Escape:
			case Qt::Key_Tab:
			case Qt::Key_Backtab:
				e->ignore();
				return;
			default:
				break;
		}
	}

	bool isShortcut = ((e->modifiers() & Qt::ControlModifier) && e->key() == Qt::Key_Space);
	if (isShortcut) {
		QTextCursor tc = this->textCursor();
		tc.select(QTextCursor::WordUnderCursor);
		QString completionPrefix = tc.selectedText();

		if (completionPrefix.length()) {
			if (completionPrefix != _context->idCompleter->completionPrefix()) {
				_context->idCompleter->setCompletionPrefix(completionPrefix);
				_context->idCompleter->popup()->setCurrentIndex(_context->idCompleter->completionModel()->index(0, 0));
			}
			QRect cr = this->cursorRect();
			cr.setWidth(_context->idCompleter->popup()->sizeHintForColumn(0) + _context->idCompleter->popup()->verticalScrollBar()->sizeHint().width());
			_context->idCompleter->complete(cr);
		}
	} else {
		QPlainTextEdit::keyPressEvent(e);
		_context->idCompleter->popup()->hide();
	}
}

void TextEdit::finished()
{
	curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(_propertyKey, true);
	if (value) {
		QStringList plainText = this->toPlainText().split("\n");
		trimList(plainText);
		value->text = plainText;
		_editForm->changesMade();
	}
}

void TextEdit::enlarge()
{
	KeyEditor ed(&curspec::meta, _propertyKey);
	QString name = "Input";
	const DeclField *field = ed.decl()->findField("property-id");
	if (field) {
		name = ed.tostringAt(field->index, true);
	}
	name += ":";
	EnlargeDialog *enlargeDialog = new EnlargeDialog(name, this->toPlainText(), this);
	if (enlargeDialog->exec() == QDialog::Accepted) {
		QString text = enlargeDialog->text();
		this->blockSignals(true);
		this->setPlainText(text);
		this->finished();
		this->blockSignals(false);
		_editForm->changesMade();
	}
}

OptionEdit::OptionEdit(TreeItemContext *context, EditForm *editForm, QByteArray key, const DeclType *optionValueType, QWidget *parent)
	:	QWidget(parent),
		_context(context),
		_editForm(editForm),
		_propertyKey(key),
		_parent(parent)
{
	QHBoxLayout *hLayout = new QHBoxLayout(this);
	hLayout->setSpacing(0);
	hLayout->setMargin(0);
	_checkBox = new QCheckBox(this);
	_comboBox = new QComboBox(this);
	hLayout->addWidget(_checkBox);
	hLayout->addWidget(_comboBox);
	hLayout->addStretch();
	this->setLayout(hLayout);

	for(unsigned int i = 0; i < optionValueType->nEnumValues; i++) {
		_comboBox->addItem(optionValueType->enumValues[i].text);
	}

	curspec::PersonSex *value = _context->frontend->get<curspec::PersonSex>(_propertyKey, true);
	if (!value) {
		throw ("something very strange happened");
	}
	if (value->sex == enumNull) {
		_checkBox->setChecked(false);
		_comboBox->setEnabled(false);
	} else {
		_checkBox->setChecked(true);
		_comboBox->setEnabled(true);
		_comboBox->setCurrentIndex(enum2index(value->sex));
	}

	connect(_checkBox, SIGNAL(stateChanged(int)), this, SLOT(checkboxChanged(int)));
	connect(_comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(comboChanged(int)));
}

void OptionEdit::checkboxChanged(int state)
{
	curspec::PersonSex *value = _context->frontend->get<curspec::PersonSex>(_propertyKey);
	if (value) {
		if (state == Qt::Checked) {
			if (value->sex == enumNull) {
				value->sex = index2enum(_comboBox->currentIndex());
			} else {
				_comboBox->setCurrentIndex(enum2index(value->sex));
			}
			_comboBox->setEnabled(true);
		} else {
			_comboBox->setEnabled(false);
			value->sex = enumNull;
		}
		_editForm->changesMade();
	}
}

void OptionEdit::comboChanged(int index)
{
	curspec::PersonSex *value = _context->frontend->get<curspec::PersonSex>(_propertyKey);
	if (value) {
		value->sex = index2enum(_comboBox->currentIndex());
		_editForm->changesMade();
	}
}

int AnchorNoFocus::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
	if (hint == SH_TextControl_FocusIndicatorTextCharFormat)
	return false;
	return QProxyStyle::styleHint(hint, option, widget, returnData);
}

ContentEdit::ContentEdit(TreeItemContext *context, QByteArray key, curspec::TextAnnotated *value, QWidget *parent)
	:	QTextBrowser(parent),
		_context(context),
		_propertyKey(key),
		_parent(parent)
{
	this->setStyle(new AnchorNoFocus);
	_styleNormal.setForeground(Qt::black);
	_styleNormal.setFontWeight(QFont::Normal);
	_styleNormal.setFontItalic(false);
	textFragmenter = new TextFragmenter(value->text.join("\n"));
	this->setPlainText(textFragmenter->text());
	installEventFilter(this);
	this->setOpenLinks(false);
	setReadOnly(true);
}

void ContentEdit::unmark()
{
	QTextCursor cursor(this->document());
	cursor.movePosition(QTextCursor::Start, QTextCursor::MoveAnchor);
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
	cursor.setCharFormat(_styleNormal);
}

void ContentEdit::reload()
{
	this->unmark();
	this->setPlainText(textFragmenter->text());
}

bool ContentEdit::eventFilter(QObject *watched, QEvent *ev)
{
	if (ev->type() == QEvent::FocusOut) {
		QFocusEvent *focusEvent = static_cast<QFocusEvent*>(ev);
		if (focusEvent->reason() != Qt::ActiveWindowFocusReason && focusEvent->reason() != Qt::PopupFocusReason) {
			QTextCursor cursor = this->textCursor();
			cursor.clearSelection();
			this->setTextCursor(cursor);
		}
		finished();
	}
	return false;
}

void ContentEdit::finished()
{
	curspec::TextAnnotated *value = _context->frontend->get<curspec::TextAnnotated>(_propertyKey, true);
	if (value) {
		QStringList plainText = this->toPlainText().split("\n");
		 trimList(plainText);
		 value->text = plainText;
	}
}

CategoryEdit::CategoryEdit(TreeItemContext *context, EditForm *editForm, ContentEdit *annTextEdit, curspec::TextAnnotated *value, quint8 annotatedType, QByteArray key)
	:	_context(context),
		_editForm(editForm),
		_key(key),
		_contentEdit(annTextEdit),
		_value(value),
		_annotatedType(annotatedType)
{
	_styleHilite1.setForeground(Qt::red);
	_styleHilite1.setFontWeight(QFont::Normal);
	_styleHilite1.setFontItalic(true);
	_styleHilite2.setForeground(Qt::red);
	_styleHilite2.setFontWeight(QFont::Bold);

	for (auto comment : value->comments) {
		if (comment.type == annotatedType) {
			CommentItem *commentItem = new CommentItem(annTextEdit, comment.pbegin, comment.wbegin, comment.pend, comment.wend, comment.punc, comment.id);
			_comments.append(commentItem);
		}
	}
	std::sort(_comments.begin(), _comments.end(), [](const CommentItem *a, const CommentItem *b) { return *a < *b; });
	QVBoxLayout *vLayout = new QVBoxLayout;
	_slider = new QSlider(Qt::Horizontal, this);
	_slider->setMinimum(0);
	_slider->setMaximum(_comments.size() - 1);
	_slider->setTickPosition(QSlider::TicksBelow);
	_slider->setTickInterval(1);
	_slider->setStyleSheet("");
	if (_comments.size() < 2) {
		_slider->setVisible(false);
	}
	vLayout->addWidget(_slider);
	connect(_slider, SIGNAL(valueChanged(int)), this, SLOT(valueChanged(int)));
	QHBoxLayout *hLayout = new QHBoxLayout;
	_puncOptionBox = new QCheckBox("Include Punctuation", this);
	_puncOptionBox->setTristate(true);
#ifdef _WIN32
	// ugly solution for state not shown correctly in Windows
	_puncOptionBox->setStyleSheet("QCheckBox::indicator {width: 14px; height: 14px;} QCheckBox::indicator:indeterminate {image: url(:/images/checkbox_indeterminate.png);}");
#endif
	hLayout->addWidget(_puncOptionBox);
	connect(_puncOptionBox, SIGNAL(stateChanged(int)), this, SLOT(puncOptionChanged(int)));
	_infoText = new QLabel();
	hLayout->addWidget(_infoText);
	hLayout->addStretch();
	_deleteButton = new QPushButton();
	_deleteButton->setText(QString("delete"));
	hLayout->addWidget(_deleteButton);
	connect(_deleteButton, SIGNAL(clicked(bool)), this, SLOT(deleteComment()));
	vLayout->addLayout(hLayout);
	_mapto = EnumInfo<curspec::AnnotatedType>::DeclObject->enumValues[annotatedType - 1].mapto;
	if (! _mapto) {
		 _mapto = EnumInfo<curspec::AnnotatedType>::DeclObject->mapto;
	}
	if (_mapto == &curspec::textLine) {
		_annot = new LineEdit(_context, _editForm, this);
		connect(_annot, SIGNAL(textEdited(const QString&)), this, SLOT(annotChanged()));
		vLayout->addWidget(_annot);
		vLayout->addStretch();
	} else {
		_annot = new TextEdit(_context, _editForm, this);
		connect(_annot, SIGNAL(textChanged()), this, SLOT(annotChanged()));
		vLayout->addWidget(_annot);
	}
	_annot->setEnabled(false);
	this->setLayout(vLayout);
	connect(_contentEdit, SIGNAL(anchorClicked(const QUrl &)), this, SLOT(anchorClicked(const QUrl &)));
}

void CategoryEdit::puncOptionChanged(int state)
{
	CommentItem *commentItem = _comments.value(_slider->value(), nullptr);
	if (commentItem) {
		for (auto &comment : _value->comments) {
			if (comment.id == commentItem->id()) {
				switch(state) {
					case Qt::Unchecked:
						comment.punc = (quint8)curspec::OptionPunc::Exclude;
						commentItem->updatePunc((quint8)curspec::OptionPunc::Exclude);
						break;
					case Qt::PartiallyChecked:
						comment.punc = (quint8)curspec::OptionPunc::Unset;
						commentItem->updatePunc((quint8)curspec::OptionPunc::Unset);
						break;
					case Qt::Checked:
						comment.punc = (quint8)curspec::OptionPunc::Include;
						commentItem->updatePunc((quint8)curspec::OptionPunc::Include);
						break;
				}
				_editForm->changesMade();
			}
		}
	}
}

void CategoryEdit::markInit()
{
	_slider->setValue(0);
	this->valueChanged(0);
}

void CategoryEdit::anchorClicked(const QUrl &link)
{
	QStringList linkParts = link.path().split(";");
	if (linkParts.at(0).toInt() == _annotatedType) {
		_slider->setValue(linkParts.at(1).toInt());
	}
}

void CategoryEdit::valueChanged(int value)
{
	_contentEdit->unmark();

	for (int i = 0; i < _comments.size(); ++i) {
		auto comment = _comments.at(i);
		QTextCharFormat anchoredFormat = _styleHilite1;
		anchoredFormat.setAnchorHref(QString::number(_annotatedType) + ";" + QString::number(i));
		anchoredFormat.setAnchor(true);
		comment->mark(anchoredFormat);
	}

	if (_comments.size() > 1) {
		_puncOptionBox->setVisible(true);
		_deleteButton->setVisible(true);
		_slider->setVisible(true);
	} else {
		_puncOptionBox->setVisible(false);
		_deleteButton->setVisible(false);
		_slider->setVisible(false);
	}

	CommentItem *comment = _comments.value(value, nullptr);
	if (comment) {
		_annot->setEnabled(true);
		comment->mark(_styleHilite2);
		KeyEditor keyEd1(&curspec::meta,_key);
		KeyEditor keyEd2(&curspec::meta);
		keyEd2.select(&curspec::textComment);
		keyEd2.uintPut(curspec::TextComment::LetterId, keyEd1.uintGet(curspec::TextMetadata::LetterId));
		keyEd2.uintPut(curspec::TextComment::ObjectId, comment->id());
		keyEd2.uintPut(curspec::TextComment::LanguageId, keyEd1.uintGet(curspec::TextMetadata::LanguageId));
		curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd2);
		_annot->blockSignals(true);
		_puncOptionBox->blockSignals(true);
		switch(comment->punc()) {
			case (quint8)curspec::OptionPunc::Exclude:
				_puncOptionBox->setCheckState(Qt::Unchecked);
				break;
			case (quint8)curspec::OptionPunc::Unset:
				_puncOptionBox->setCheckState(Qt::PartiallyChecked);
				break;
			case (quint8)curspec::OptionPunc::Include:
				_puncOptionBox->setCheckState(Qt::Checked);
				break;
		}
		QString textValue("");
		if (value) {
			textValue = value->text.join("\n");
		}
		if (_mapto == &curspec::textLine) {
			LineEdit *le = dynamic_cast<LineEdit*>(_annot);
			if (le) {
				le->setText(textValue);
			}
		} else {
			TextEdit *te = dynamic_cast<TextEdit*>(_annot);
			if (te) {
				te->setPlainText(textValue);
			}
		}
		_annot->blockSignals(false);
		_puncOptionBox->blockSignals(false);
	} else {
		_annot->setEnabled(false);
	}
}

void CategoryEdit::annotChanged()
{
	CommentItem *comment = _comments.value(_slider->value(), nullptr);
	if (comment) {
		KeyEditor keyEd1(&curspec::meta,_key);
		KeyEditor keyEd2(&curspec::meta);
		keyEd2.select(&curspec::textComment);
		keyEd2.uintPut(curspec::TextComment::LetterId, keyEd1.uintGet(curspec::TextMetadata::LetterId));
		keyEd2.uintPut(curspec::TextComment::ObjectId, comment->id());
		keyEd2.uintPut(curspec::TextComment::LanguageId, keyEd1.uintGet(curspec::TextMetadata::LanguageId));
		curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd2, true);
		if (value) {
			QStringList plainText;
			if (_mapto == &curspec::textLine) {
				LineEdit *le = dynamic_cast<LineEdit*>(_annot);
				if (le) {
					plainText = le->text().split("\n");
				}
			} else {
				TextEdit *te = dynamic_cast<TextEdit*>(_annot);
				if (te) {
					plainText = te->toPlainText().split("\n");
				}
			}
			trimList(plainText);
			value->text = plainText;
			_editForm->changesMade();
		}
	}
}

void CategoryEdit::deleteComment()
{
	CommentItem *commentItem = _comments.value(_slider->value(), nullptr);
	if (commentItem) {
		for (int i = 0; i < _value->comments.size(); ++i) {
			if (_value->comments.value(i).id == commentItem->id()) {
				if (_mapto == &curspec::textLine) {
					LineEdit *le = dynamic_cast<LineEdit*>(_annot);
					if (le) {
						le->setText(QString(""));
					}
				} else {
					TextEdit *te = dynamic_cast<TextEdit*>(_annot);
					if (te) {
						te->setPlainText(QString(""));
					}
				}
				_value->comments.removeAt(i);
				_comments.removeAt(_slider->value());
				_slider->setMaximum(_comments.size() - 1);
				std::sort(_comments.begin(), _comments.end(), [](const CommentItem *a, const CommentItem *b) { return *a < *b; });
				int val = _slider->value();
				val--;
				if (val < 0) {
					val = 0;
				}
				if (_slider->value() == val) {
					this->valueChanged(val);
				} else {
					_slider->setValue(val);
				}
				_editForm->changesMade();
				break;
			}
		}
	}
}

void CategoryEdit::contextMenu(const QPoint &point)
{
	QMenu *contextMenu = _contentEdit->createStandardContextMenu(point);
	contextMenu->addSeparator();
	QAction *addAnnot = contextMenu->addAction("Add Annotation");
	QAction *changeAnnot = contextMenu->addAction("Change Annotation");

	QAction *selectedAction = contextMenu->exec(_contentEdit->mapToGlobal(point));

	if (selectedAction == addAnnot || selectedAction == changeAnnot) {
		QTextCursor cursor(_contentEdit->textCursor());
		if(cursor.hasSelection()) {
			int pbegin;
			int wbegin;
			int pend;
			int wend;
			quint8 punc;
			if (_contentEdit->textFragmenter->word(cursor.selectionStart(), &pbegin, &wbegin) && _contentEdit->textFragmenter->word(cursor.selectionEnd(), &pend, &wend)) {
				if (selectedAction == addAnnot) {
					try {
						_puncOptionBox->setCheckState(Qt::Unchecked);
						punc = (quint8)curspec::OptionPunc::Unset;
						quint64 newId = _context->frontend->acquire(&curspec::objectId);
						if(newId) {
							KeyEditor keyEd1(&curspec::meta,_key);
							KeyEditor keyEd2(&curspec::meta);
							keyEd2.select(&curspec::textComment);
							keyEd2.uintPut(curspec::TextComment::ObjectId, newId);
							keyEd2.uintPut(curspec::TextComment::LetterId, keyEd1.uintGet(curspec::TextMetadata::LetterId));
							keyEd2.uintPut(curspec::TextComment::LanguageId, keyEd1.uintGet(curspec::TextMetadata::LanguageId));
							decltype(_value->comments)::value_type comment;
							comment.pbegin = pbegin;
							comment.wbegin = wbegin;
							comment.pend = pend;
							comment.wend = wend;
							comment.punc = punc;
							comment.id = newId;
							comment.type = _annotatedType;
							_value->comments.append(comment);
							CommentItem *commentItem = new CommentItem(_contentEdit, pbegin, wbegin, pend, wend, punc, newId);
							_comments.append(commentItem);
							std::sort(_comments.begin(), _comments.end(), [](const CommentItem *a, const CommentItem *b) { return *a < *b; });
							_slider->setMinimum(0);
							_slider->setMaximum(_comments.size() - 1);
							for (int i = 0; i < _comments.size(); ++i) {
								if (_comments.at(i) == commentItem) {
									if (_slider->value() == i) {
										this->valueChanged(i);
									} else {
										_slider->setValue(i);
									}
								}
							}
							_editForm->changesMade();
						}
					} catch (const Exception &ex) {
						qInfo() << "exception:" << ex.message();
					}
				} else { // changeAnnot
					CommentItem *commentItem = _comments.value(_slider->value(), nullptr);
					if (commentItem) {
						for (auto &comment : _value->comments) {
							if (comment.id == commentItem->id()) {
								comment.pbegin = pbegin;
								comment.wbegin = wbegin;
								comment.pend = pend;
								comment.wend = wend;
								commentItem->updatePos(pbegin, wbegin, pend, wend);
								std::sort(_comments.begin(), _comments.end(), [](const CommentItem *a, const CommentItem *b) { return *a < *b; });
								for (int i = 0; i < _comments.size(); ++i) {
									if (_comments.at(i) == commentItem) {
										if (_slider->value() == i) {
											this->valueChanged(i);
										} else {
											_slider->setValue(i);
										}
									}
								}
								_editForm->changesMade();
							}
						}
					}
				}
			}
		} else {
			_context->mainWindow->statusBar()->showMessage("Select some text to markup first", 2000);
		}
	}
	delete contextMenu;
}

CommentItem::CommentItem(ContentEdit *annTextEdit, int pbegin, int wbegin, int pend, int wend, quint8 punc, quint32 id)
	:	_contentEdit(annTextEdit),
		_pbegin(pbegin),
		_wbegin(wbegin),
		_pend(pend),
		_wend(wend),
		_punc(punc),
		_id(id)
{}

bool CommentItem::operator<(const CommentItem &other) const
{
	if (this->_pbegin < other._pbegin) return true;
	else if (this->_pbegin > other._pbegin) return false;
	else if (this->_wbegin < other._wbegin) return true;
	else if (this->_wbegin > other._wbegin) return false;
	else if (this->_pend < other._pend) return true;
	else if (this->_pend > other._pend) return false;
	else return (this->_wend < other._wend);
}

void CommentItem::updatePos(int pbegin, int wbegin, int pend, int wend)
{
	_pbegin = pbegin;
	_wbegin = wbegin;
	_pend = pend;
	_wend = wend;
}

void CommentItem::updatePunc(quint8 punc)
{
	_punc = punc;
}

void CommentItem::mark(QTextCharFormat style)
{
	QTextCursor cursor(_contentEdit->document());

	int start;
	int end;
	if (_contentEdit->textFragmenter->word(_pbegin, _wbegin, &start, &end)){
		cursor.setPosition(start, QTextCursor::MoveAnchor);
		if (_contentEdit->textFragmenter->word(_pend, _wend, &start, &end)){
			cursor.setPosition(end, QTextCursor::KeepAnchor);
			cursor.mergeCharFormat(style);
		} else {
			printf("No such word %d in paragraph %d\n", _wbegin, _pbegin);
		}
	} else {
		printf("No such word %d in paragraph %d\n", _wbegin, _pbegin);
	}
}
