#include <QRegExp>

#include "types.h"
#include "treedata.h"
#include "centralwidget.h"
#include "searchform.h"

using namespace spec;

SearchResultItem::SearchResultItem(QTreeWidgetItem *dataItem, curspec::LanguageId langId, const QString &prop, const QString &val)
	:	QTableWidgetItem(dataItem->text(0)),
		_dataItem(dataItem),
		_langId(langId),
		_langTableItem(new QTableWidgetItem(EnumInfo<curspec::LanguageId>::name(langId))),
		_propTableItem(new QTableWidgetItem(prop)),
		_valTableItem(new QTableWidgetItem(val))
{
	LetterItem *letterItem = dynamic_cast<LetterItem *>(dataItem);
	if (letterItem) {
		QString parentText = letterItem->parent()->text(0);
		QString ownText = this->text();
		this->setText(parentText + ", " + ownText);
	}
	this->setFlags(this->flags() ^ Qt::ItemIsEditable);
	_langTableItem->setFlags(_langTableItem->flags() ^ Qt::ItemIsEditable);
	_propTableItem->setFlags(_propTableItem->flags() ^ Qt::ItemIsEditable);
	_valTableItem->setFlags(_valTableItem->flags() ^ Qt::ItemIsEditable);
}

CheckBoxWithId::CheckBoxWithId(const QString &text, curspec::LanguageId id, QWidget *parent)
	:	QCheckBox(text, parent),
		_id(id)
{
	this->setChecked(true);
}

SearchForm::SearchForm(TreeItemContext *treeItemContext, QWidget *parent)
	:	QWidget(parent),
		_treeItemContext(treeItemContext),
		_resultCount(0),
		_caseSensitivity(Qt::CaseInsensitive),
		_personIcon(":/images/person_icon.svg"),
		_locationIcon(":/images/location_icon.svg"),
		_bibliographyIcon(":/images/bibliography_icon.svg"),
		_philIcon(":/images/phil_icon.svg"),
		_histIcon(":/images/hist_icon.svg"),
		_letterIcon(":/images/letter_icon.svg")
{
	_ui.setupUi(this);
	_statusBar = new QStatusBar(this);
	this->layout()->addWidget(_statusBar);

	this->setWindowFlags(this->windowFlags() | Qt::Window);
	this->setWindowIcon(QIcon(":/images/logo_icon.svg"));
	this->setWindowTitle("QAnnotate - Search");

	QHBoxLayout *hbox = new QHBoxLayout;
	for(unsigned int i = 0; i < EnumInfo<curspec::LanguageId>::N; i++) {
		QLocale lcle(EnumInfo<curspec::LanguageId>::name(i));
		hbox->addWidget(new CheckBoxWithId(lcle.nativeLanguageName(), EnumInfo<curspec::LanguageId>::fromIndex(i), _ui.groupBox));
	}
	hbox->addStretch();
	_ui.groupBox->setLayout(hbox);

	connect(_ui.pushButton, SIGNAL(clicked()), this, SLOT(search()));
	connect(_ui.tableWidget, SIGNAL(itemDoubleClicked(QTableWidgetItem *)), this, SLOT(itemDoubleClicked(QTableWidgetItem *)));
	connect(_ui.lineEdit, SIGNAL(returnPressed()), this, SLOT(search()));
	connect(_ui.checkBoxAll, SIGNAL(clicked(bool)), this, SLOT(checkBoxAllClicked(bool)));
	connect(_ui.checkBoxPersons, SIGNAL(clicked(bool)), this, SLOT(setCheckBoxAllState()));
	connect(_ui.checkBoxLocations, SIGNAL(clicked(bool)), this, SLOT(setCheckBoxAllState()));
	connect(_ui.checkBoxBibliography, SIGNAL(clicked(bool)), this, SLOT(setCheckBoxAllState()));
	connect(_ui.checkBoxPhil, SIGNAL(clicked(bool)), this, SLOT(setCheckBoxAllState()));
	connect(_ui.checkBoxHist, SIGNAL(clicked(bool)), this, SLOT(setCheckBoxAllState()));
	connect(_ui.checkBoxTexts, SIGNAL(clicked(bool)), this, SLOT(setCheckBoxAllState()));
	connect(_ui.checkBoxAnnots, SIGNAL(clicked(bool)), this, SLOT(setCheckBoxAllState()));
}

void SearchForm::checkBoxAllClicked(bool state)
{
	if (_ui.checkBoxAll->checkState() == Qt::PartiallyChecked) {
		_ui.checkBoxAll->setCheckState(Qt::Checked);
	}
	_ui.checkBoxPersons->setCheckState(_ui.checkBoxAll->checkState());
	_ui.checkBoxLocations->setCheckState(_ui.checkBoxAll->checkState());
	_ui.checkBoxBibliography->setCheckState(_ui.checkBoxAll->checkState());
	_ui.checkBoxPhil->setCheckState(_ui.checkBoxAll->checkState());
	_ui.checkBoxHist->setCheckState(_ui.checkBoxAll->checkState());
	_ui.checkBoxTexts->setCheckState(_ui.checkBoxAll->checkState());
	_ui.checkBoxAnnots->setCheckState(_ui.checkBoxAll->checkState());
}

void SearchForm::setCheckBoxAllState()
{
	int buttonCounter = 0;
	if (_ui.checkBoxPersons->isChecked()) buttonCounter++;
	if (_ui.checkBoxLocations->isChecked()) buttonCounter++;
	if (_ui.checkBoxBibliography->isChecked()) buttonCounter++;
	if (_ui.checkBoxPhil->isChecked()) buttonCounter++;
	if (_ui.checkBoxHist->isChecked()) buttonCounter++;
	if (_ui.checkBoxTexts->isChecked()) buttonCounter++;
	if (_ui.checkBoxAnnots->isChecked()) buttonCounter++;
	if (buttonCounter == 0) {
		_ui.checkBoxAll->setCheckState(Qt::Unchecked);
	} else if (buttonCounter == 7) {
		_ui.checkBoxAll->setCheckState(Qt::Checked);
	} else {
		_ui.checkBoxAll->setCheckState(Qt::PartiallyChecked);
	}
}

void SearchForm::search()
{
	if (_ui.checkBoxCS->isChecked()) {
		_caseSensitivity = Qt::CaseSensitive;
	} else {
		_caseSensitivity = Qt::CaseInsensitive;
	}
	if (_ui.checkBoxRegexp->isChecked()){
		_regExp.setPattern(_ui.lineEdit->text());
		_regExp.setCaseSensitivity(_caseSensitivity);
		if (!_regExp.isValid()) {
			_statusBar->showMessage("Invalid Regular Expression!");
			return;
		}
	}
	_statusBar->showMessage("Searching...");
	_resultCount = 0;
	_ui.tableWidget->clear();
	_ui.tableWidget->setRowCount(0);
	_ui.tableWidget->setColumnCount(4);
	_ui.tableWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Item"));
	_ui.tableWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("Language"));
	_ui.tableWidget->setHorizontalHeaderItem(2, new QTableWidgetItem("Property"));
	_ui.tableWidget->setHorizontalHeaderItem(3, new QTableWidgetItem("Value"));
	if (_ui.checkBoxPersons->isChecked()) {
		for (int i = 0; i < _treeItemContext->personRoot->childCount(); ++i){
			this->searchInItem(_treeItemContext->personRoot->child(i));
		}
	}
	if (_ui.checkBoxLocations->isChecked()) {
		for (int i = 0; i < _treeItemContext->locationRoot->childCount(); ++i){
			for (int j = 0; j < _treeItemContext->locationRoot->child(i)->childCount(); ++j){
				this->searchInItem(_treeItemContext->locationRoot->child(i)->child(j));
			}
		}
	}
	if (_ui.checkBoxBibliography->isChecked()) {
		for (int i = 0; i < _treeItemContext->bibliographyRoot->childCount(); ++i){
			for (int j = 0; j < _treeItemContext->bibliographyRoot->child(i)->childCount(); ++j){
				this->searchInItem(_treeItemContext->bibliographyRoot->child(i)->child(j));
			}
		}
	}
	if (_ui.checkBoxPhil->isChecked()) {
		for (int i = 0; i < _treeItemContext->philRoot->childCount(); ++i){
			this->searchInItem(_treeItemContext->philRoot->child(i));
		}
	}
	if (_ui.checkBoxHist->isChecked()) {
		for (int i = 0; i < _treeItemContext->histRoot->childCount(); ++i){
			for (int j = 0; j < _treeItemContext->histRoot->child(i)->childCount(); ++j){
				this->searchInItem(_treeItemContext->histRoot->child(i)->child(j));
			}
		}
	}
	if (_ui.checkBoxTexts->isChecked()) {
		for (int i = 0; i < _treeItemContext->textRoot->childCount(); ++i){
			for (int j = 0; j < _treeItemContext->textRoot->child(i)->childCount(); ++j){
				this->searchInItem(_treeItemContext->textRoot->child(i)->child(j));
			}
		}
	}
	if (_ui.checkBoxAnnots->isChecked()) {
		for (int i = 0; i < _treeItemContext->textRoot->childCount(); ++i){
			for (int j = 0; j < _treeItemContext->textRoot->child(i)->childCount(); ++j){
				this->searchInItem(_treeItemContext->textRoot->child(i)->child(j), true);
			}
		}
	}
	_statusBar->showMessage(QString("Search finished with %1 result%2").arg(_resultCount).arg(_resultCount == 1? "." : "s."));
}

void SearchForm::searchInItem(QTreeWidgetItem *treeWidgetItem, bool searchAnnots)
{
	KeyEditor keyEd(&curspec::meta);
	DataItem *dataItem = dynamic_cast<DataItem *>(treeWidgetItem);
	if (dataItem) {
		if (dataItem->declKey == &curspec::personObject){
			keyEd.select(&curspec::personObject);
			keyEd.uintPut(curspec::PersonObject::ObjectId, dataItem->id());
			for(size_t i = 0; i < EnumInfo<curspec::PersonPropertyId>::N; i++) {
				QString propIdText = ((QString) EnumInfo<curspec::PersonPropertyId>::text(i)) + ":";
				keyEd.enumPut(curspec::PersonObject::PropertyId, EnumInfo<curspec::PersonPropertyId>::fromIndex(i));
				for (auto groupBoxChild : _ui.groupBox->children()) {
					CheckBoxWithId *checkBox = dynamic_cast<CheckBoxWithId *>(groupBoxChild);
					if (checkBox && checkBox->isChecked()) {
						keyEd.enumPut(curspec::PersonObject::LanguageId,checkBox->id());
						this->createSearchResult(keyEd, _personIcon, treeWidgetItem, checkBox->id(), propIdText);
					}
				}
			}
		} else if (dataItem->declKey == &curspec::locationObject) {
			keyEd.select(&curspec::locationObject);
			keyEd.uintPut(curspec::LocationObject::ObjectId, dataItem->id());
			for(size_t i = 0; i < EnumInfo<curspec::LocationPropertyId>::N; i++) {
				QString propIdText = ((QString) EnumInfo<curspec::LocationPropertyId>::text(i)) + ":";
				keyEd.enumPut(curspec::LocationObject::PropertyId, EnumInfo<curspec::LocationPropertyId>::fromIndex(i));
				for (auto groupBoxChild : _ui.groupBox->children()) {
					CheckBoxWithId *checkBox = dynamic_cast<CheckBoxWithId *>(groupBoxChild);
					if (checkBox && checkBox->isChecked()) {
						keyEd.enumPut(curspec::LocationObject::LanguageId,checkBox->id());
						this->createSearchResult(keyEd, _locationIcon, treeWidgetItem, checkBox->id(), propIdText);
					}
				}
			}
		} else if (dataItem->declKey == &curspec::bibliographyObject) {
			keyEd.select(&curspec::bibliographyObject);
			keyEd.uintPut(curspec::BibliographyObject::ObjectId, dataItem->id());
			for(size_t i = 0; i < EnumInfo<curspec::BibliographyPropertyId>::N; i++) {
				QString propIdText = ((QString) EnumInfo<curspec::BibliographyPropertyId>::text(i)) + ":";
				keyEd.enumPut(curspec::BibliographyObject::PropertyId, EnumInfo<curspec::BibliographyPropertyId>::fromIndex(i));
				for (auto groupBoxChild : _ui.groupBox->children()) {
					CheckBoxWithId *checkBox = dynamic_cast<CheckBoxWithId *>(groupBoxChild);
					if (checkBox && checkBox->isChecked()) {
						keyEd.enumPut(curspec::BibliographyObject::LanguageId,checkBox->id());
						this->createSearchResult(keyEd, _bibliographyIcon, treeWidgetItem, checkBox->id(), propIdText);
					}
				}
			}
		} else if (dataItem->declKey == &curspec::philCommentObject) {
			keyEd.select(&curspec::philCommentObject);
			keyEd.uintPut(curspec::PhilCommentObject::ObjectId, dataItem->id());
			for(size_t i = 0; i < EnumInfo<curspec::CommentPropertyId>::N; i++) {
				QString propIdText = ((QString) EnumInfo<curspec::CommentPropertyId>::text(i)) + ":";
				keyEd.enumPut(curspec::PhilCommentObject::PropertyId, EnumInfo<curspec::CommentPropertyId>::fromIndex(i));
				for (auto groupBoxChild : _ui.groupBox->children()) {
					CheckBoxWithId *checkBox = dynamic_cast<CheckBoxWithId *>(groupBoxChild);
					if (checkBox && checkBox->isChecked()) {
						keyEd.enumPut(curspec::PhilCommentObject::LanguageId,checkBox->id());
						this->createSearchResult(keyEd, _philIcon, treeWidgetItem, checkBox->id(), propIdText);
					}
				}
			}
		} else if (dataItem->declKey == &curspec::histCommentObject) {
			keyEd.select(&curspec::histCommentObject);
			keyEd.uintPut(curspec::HistCommentObject::ObjectId, dataItem->id());
			for(size_t i = 0; i < EnumInfo<curspec::CommentPropertyId>::N; i++) {
				QString propIdText = ((QString) EnumInfo<curspec::CommentPropertyId>::text(i)) + ":";
				keyEd.enumPut(curspec::HistCommentObject::PropertyId, EnumInfo<curspec::CommentPropertyId>::fromIndex(i));
				for (auto groupBoxChild : _ui.groupBox->children()) {
					CheckBoxWithId *checkBox = dynamic_cast<CheckBoxWithId *>(groupBoxChild);
					if (checkBox && checkBox->isChecked()) {
						keyEd.enumPut(curspec::HistCommentObject::LanguageId,checkBox->id());
						this->createSearchResult(keyEd, _histIcon, treeWidgetItem, checkBox->id(), propIdText);
					}
				}
			}
		} else if (dataItem->declKey == &curspec::textLetter) {
			if (searchAnnots) {
				KeyEditor commentKeyEd(&curspec::meta);
				keyEd.select(&curspec::textContent);
				keyEd.uintPut(curspec::TextContent::LetterId, dataItem->id());
				curspec::TextAnnotated *value = _treeItemContext->frontend->get<curspec::TextAnnotated>(keyEd);
				if(value) {
					for (auto comment : value->comments) {
						QString propIdText = ((QString) EnumInfo<curspec::AnnotatedType>::text(enum2index(comment.type))) + ":";
						for (auto groupBoxChild : _ui.groupBox->children()) {
							CheckBoxWithId *checkBox = dynamic_cast<CheckBoxWithId *>(groupBoxChild);
							if (checkBox && checkBox->isChecked()) {
								commentKeyEd.select(&curspec::textComment);
								commentKeyEd.uintPut(curspec::TextComment::LetterId, dataItem->id());
								commentKeyEd.uintPut(curspec::TextComment::ObjectId, comment.id);
								commentKeyEd.enumPut(curspec::TextComment::LanguageId, checkBox->id());
								this->createSearchResult(commentKeyEd, _letterIcon, treeWidgetItem, checkBox->id(), propIdText);
							}
						}
					}
				}
			} else {
				keyEd.select(&curspec::textMetadata);
				keyEd.uintPut(curspec::TextMetadata::LetterId, dataItem->id());
				for(size_t i = 0; i < EnumInfo<curspec::TextPropertyId>::N; i++) {
					QString propIdText = ((QString) EnumInfo<curspec::TextPropertyId>::text(i)) + ":";
					keyEd.enumPut(curspec::TextMetadata::PropertyId, EnumInfo<curspec::TextPropertyId>::fromIndex(i));
					for (auto groupBoxChild : _ui.groupBox->children()) {
						CheckBoxWithId *checkBox = dynamic_cast<CheckBoxWithId *>(groupBoxChild);
						if (checkBox && checkBox->isChecked()) {
							keyEd.enumPut(curspec::TextMetadata::LanguageId, checkBox->id());
							this->createSearchResult(keyEd, _letterIcon, treeWidgetItem, checkBox->id(), propIdText);
						}
					}
				}
			}
		}
	}
}
/*

		curspec::TextMultiline *value = _context->frontend->get<curspec::TextMultiline>(keyEd2);
		_annot->blockSignals(true);
		if (value) {
			_annot->setPlainText(value->text.join("\n"));


			for (unsigned int i = 0; i < EnumInfo<curspec::AnnotatedType>::N; i++) {
				CategoryEdit *cEdit = new CategoryEdit(_context, this, _contentEdit, value, index2enum(i), keyEd);
				_tabWidget->addTab(cEdit,EnumInfo<curspec::AnnotatedType>::text(i));
			}*/

void SearchForm::createSearchResult(const KeyEditor &keyEd, const QIcon &icon, QTreeWidgetItem *treeWidgetItem, curspec::LanguageId langId, const QString &propIdText)
{
	const DeclValue *mapto = keyEd.mapto();
	if (mapto == &curspec::textLine) {
		curspec::TextLine *value = _treeItemContext->frontend->get<curspec::TextLine>(keyEd);
		if (value) {
			QString text = (QString) value->text;
			if ((_ui.checkBoxRegexp->isChecked() && text.contains(_regExp)) || (!_ui.checkBoxRegexp->isChecked() && text.contains(_ui.lineEdit->text(), _caseSensitivity))) {
				SearchResultItem *searchResultItem = new SearchResultItem(treeWidgetItem, langId, propIdText, text);
				searchResultItem->setIcon(icon);
				_ui.tableWidget->insertRow(_ui.tableWidget->rowCount());
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 0, searchResultItem);
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 1, searchResultItem->langTableItem());
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 2, searchResultItem->propTableItem());
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 3, searchResultItem->valTableItem());
				_resultCount++;
			}
		}
	} else if (mapto == &curspec::textMultiline) {
		curspec::TextMultiline *value = _treeItemContext->frontend->get<curspec::TextMultiline>(keyEd);
		if (value) {
			QString text = (QString) value->text.join("\n");
			if ((_ui.checkBoxRegexp->isChecked() && text.contains(_regExp)) || (!_ui.checkBoxRegexp->isChecked() && text.contains(_ui.lineEdit->text(), _caseSensitivity))) {
				SearchResultItem *searchResultItem = new SearchResultItem(treeWidgetItem, langId, propIdText, text);
				searchResultItem->setIcon(icon);
				_ui.tableWidget->insertRow(_ui.tableWidget->rowCount());
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 0, searchResultItem);
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 1, searchResultItem->langTableItem());
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 2, searchResultItem->propTableItem());
				_ui.tableWidget->setItem(_ui.tableWidget->rowCount() -1, 3, searchResultItem->valTableItem());
				_resultCount++;
			}
		}
	}
}

void SearchForm::itemDoubleClicked(QTableWidgetItem *item)
{
	SearchResultItem *searchResultItem = dynamic_cast<SearchResultItem *>(item);
	if (searchResultItem) {
		_treeItemContext->centralWidget->openEditForm(searchResultItem->dataItem(), searchResultItem->langId());
	}
}
