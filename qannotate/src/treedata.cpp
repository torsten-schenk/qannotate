#include <QApplication>
#include <QStyle>
#include <QInputDialog>
#include <QMenu>
#include <QDebug>
#include <QPushButton>

#include "common/qHexdump.h"
#include "qtfe/qtfe.h"
#include "types.h"
#include "treedata.h"
#include "mainwindow.h"

using namespace spec;

LetterInitDialog::LetterInitDialog(QWidget *parent)
	:	QDialog(parent)
{
	this->setWindowTitle("QAnnotate - Create Letter");
	QVBoxLayout *vLayout = new QVBoxLayout(this);
	QFormLayout *fLayout = new QFormLayout();
	_lineEdit = new QLineEdit(this);
	fLayout->addRow("Number:", _lineEdit);
	_textEdit = new QTextEdit(this);
	fLayout->addRow("Content:", _textEdit);
	vLayout->addLayout(fLayout);
	QHBoxLayout *hLayout = new QHBoxLayout();
	QPushButton *okButton = new QPushButton("OK", this);
	QPushButton *cancelButton = new QPushButton("Cancel", this);

	hLayout->addWidget(okButton);
	hLayout->addWidget(cancelButton);
	vLayout->addLayout(hLayout);
	connect(okButton, SIGNAL(clicked()), this, SLOT(onAccept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void LetterInitDialog::onAccept()
{
	_letterText = _textEdit->toPlainText().split("\n");
	quint64 letterId = _lineEdit->text().toInt();
	if (letterId == 0) {
		letterId = 1;
	}
	_letterId = letterId;
	QDialog::accept();
}

BaseItem::BaseItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	QTreeWidgetItem(parent),
		_context(context)
{}

void BaseItem::dump()
{
	qInfo() << this->text(0);
}

bool BaseItem::operator <(const QTreeWidgetItem &other) const
{
	return _context->sorter->compare(this->text(0), other.text(0)) < 0;
}

ParentItem::ParentItem(TreeItemContext *context, QTreeWidgetItem *parent)
{}

void ParentItem::dump()
{
	BaseItem *baseItem = dynamic_cast<BaseItem *>(this);
	if (baseItem) {
		baseItem->dump();
		for (int i = 0; i < baseItem->childCount(); ++i) {
			ParentItem *parentItem = dynamic_cast<ParentItem *>(baseItem->child(i));
			if (parentItem) {
				parentItem->dump();
			}
			DataItem *dataItem = dynamic_cast<DataItem *>(baseItem->child(i));
			if (dataItem) {
				dataItem->dump();
			}
		}
	}
}

DataItem::DataItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id)
	:	_key(key),
		_id(id)
{}

void DataItem::dump()
{
	qHexdump(_key,"key:");
}

PersonRootItem::PersonRootItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	BaseItem(context, parent),
		ParentItem(context, parent)
{
	this->setText(0,"Persons");

	size_t lb;
	size_t ub;
	try {
		KeyEditor prefixEd(&curspec::meta);
		prefixEd.select(&curspec::personClass);
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			QByteArray key = _context->frontend->key(s);
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			PersonItem *item = new PersonItem(_context, this, key, value->id, true);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Error", "Error loading persons: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
}

void PersonRootItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *addPerson = contextMenu->addAction("Add Person");
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addPerson) {
		this->createChildItem();
	}

	delete contextMenu;
}

void PersonRootItem::createChildItem()
{
	bool ok;
	QString itemName = QInputDialog::getText(this->treeWidget(),"Add Person","Name:", QLineEdit::Normal,"", &ok).simplified();
	if (!ok || itemName.isEmpty()) {
		return;
	}

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::personClass);
		keyEd.stringPut(curspec::PersonClass::ObjectName, itemName);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;
				_context->frontend->modified(keyEd);
				PersonItem *newItem = new PersonItem(_context, this, keyEd, newId);
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage("Person \"" + itemName + "\" already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}
}

PersonItem::PersonItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, bool init)
	:	BaseItem(context, parent),
		DataItem(context, parent, key, id)
{
	this->declKey = &curspec::personObject;
	KeyEditor keyEd(&curspec::meta, key);
	QString text = keyEd.stringGet(curspec::PersonClass::ObjectName);
	QIcon icon = QIcon(":/images/person_icon.svg");
	this->setText(0,text);
	this->setIcon(0,icon);

	_completerItem = new QStandardItem(text);
	_completerItem->setIcon(icon);
	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(context->idCompleter->model());
	if (model){
		model->insertRow(0,_completerItem);
		if (!init) {
			model->sort(0);
		}
	}
}

void PersonItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *renameItem = contextMenu->addAction("Rename Person");
	QAction *deleteItem = contextMenu->addAction("Delete Person");

	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == renameItem) {
		bool ok;
		QString itemName;
		itemName = QInputDialog::getText(this->treeWidget(),"Rename Person","Name:", QLineEdit::Normal,this->text(0), &ok).simplified();
		if (!ok || itemName.isEmpty()) {
			delete contextMenu;
			return;
		} else {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				keyEd.stringPut(curspec::PersonClass::ObjectName, itemName);
				_context->frontend->move(_key,keyEd);
				this->setText(0,itemName);
				this->parent()->sortChildren(0, Qt::AscendingOrder);
				_key = keyEd;
				_context->centralWidget->relabelDataItemForms(this, itemName);
				_completerItem->setText(itemName);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->sort(0);
				}
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error renaming: " +  ex.message(), 4000);
			}
		}
	} else if (selectedAction == deleteItem) {
		if (QMessageBox::question(nullptr, "Delete Person", "Do you really want to delete this Person?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				_context->frontend->erase(_key);
				_context->centralWidget->closeDataItemForms(this);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->removeRow(_completerItem->row());
				}
				delete this;
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error deleting: " +  ex.message(), 4000);
			}
		}
	}

	delete contextMenu;
}

LocationRootItem::LocationRootItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	BaseItem(context, parent),
		ParentItem(context, parent)
{
	this->setText(0,"Locations");

	for (unsigned int i = 0; i < EnumInfo<curspec::LocationType>::N; i++) {
		LocationGroupItem *gItem = new LocationGroupItem(_context, this, i);
		this->addChild(gItem);
	}
}

void LocationRootItem::execContextMenu(const QPoint &point)
{}

void LocationRootItem::createChildItem()
{}

LocationGroupItem::LocationGroupItem(TreeItemContext *context, QTreeWidgetItem *parent, quint8 id)
	:	BaseItem(context, parent),
		ParentItem(context, parent),
		_id(id)
{
	this->setText(0, EnumInfo<curspec::LocationType>::plural(id));

	size_t lb;
	size_t ub;
	try {
		KeyEditor prefixEd(&curspec::meta);
		prefixEd.select(&curspec::locationClass);
		prefixEd.enumPut(curspec::LocationClass::Type, EnumInfo<curspec::LocationType>::fromIndex(id));
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			QByteArray key = _context->frontend->key(s);
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			LocationItem *item = new LocationItem(_context, this, key, value->id, this->objName(), true);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Error", "Error loading locations: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
}

void LocationGroupItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QString objName = EnumInfo<curspec::LocationType>::text(_id);
	QAction *addLocation = contextMenu->addAction("Add " + objName);
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addLocation) {
		this->createChildItem();
	}

	delete contextMenu;
}

void LocationGroupItem::createChildItem()
{
	bool ok;
	QString itemName = QInputDialog::getText(this->treeWidget(),"Add " + this->objName(),"Name:", QLineEdit::Normal,"", &ok).simplified();
	if (!ok || itemName.isEmpty()) {
		return;
	}

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::locationClass);
		keyEd.enumPut(curspec::LocationClass::Type, EnumInfo<curspec::LocationType>::fromIndex(_id));
		keyEd.stringPut(curspec::LocationClass::ObjectName, itemName);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;
				_context->frontend->modified(keyEd);
				LocationItem *newItem = new LocationItem(_context, this, keyEd, newId, this->objName());
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage(this->objName() + " \"" + itemName + "\" already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}
}

QString LocationGroupItem::objName()
{
	return EnumInfo<curspec::LocationType>::text(_id);
}

LocationItem::LocationItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, QString groupName, bool init)
	:	BaseItem(context, parent),
		DataItem(context, parent, key, id),
		_groupName(groupName)
{
	this->declKey = &curspec::locationObject;
	KeyEditor keyEd(&curspec::meta, key);
	QString text = keyEd.stringGet(curspec::LocationClass::ObjectName);
	QIcon icon = QIcon(":/images/location_icon.svg");
	this->setText(0,text);
	this->setIcon(0,icon);

	_completerItem = new QStandardItem(text);
	_completerItem->setIcon(icon);
	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(context->idCompleter->model());
	if (model){
		model->insertRow(0,_completerItem);
		if (!init) {
			model->sort(0);
		}
	}
}

void LocationItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *renameItem = contextMenu->addAction("Rename " + _groupName);
	QAction *deleteItem = contextMenu->addAction("Delete " + _groupName);
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == renameItem) {
		bool ok;
		QString itemName;
		itemName = QInputDialog::getText(this->treeWidget(),"Rename " + _groupName,"Name:", QLineEdit::Normal,this->text(0), &ok).simplified();
		if (!ok || itemName.isEmpty()) {
			delete contextMenu;
			return;
		} else {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				keyEd.stringPut(curspec::LocationClass::ObjectName, itemName);
				_context->frontend->move(_key,keyEd);
				this->setText(0,itemName);
				this->parent()->sortChildren(0, Qt::AscendingOrder);
				_key = keyEd;
				_context->centralWidget->relabelDataItemForms(this, itemName);
				_completerItem->setText(itemName);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->sort(0);
				}
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error renaming: " +  ex.message(), 4000);
			}
		}
	} else if (selectedAction == deleteItem) {
		if (QMessageBox::question(nullptr, "Delete " + _groupName, "Do you really want to delete this " + _groupName + "?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				_context->frontend->erase(_key);
				_context->centralWidget->closeDataItemForms(this);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->removeRow(_completerItem->row());
				}
				delete this;
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error deleting: " +  ex.message(), 4000);
			}
		}
	}

	delete contextMenu;
}

BibliographyRootItem::BibliographyRootItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	BaseItem(context, parent),
		ParentItem(context, parent)
{
	this->setText(0,"Bibliography");

	for (unsigned int i = 0; i < EnumInfo<curspec::BibliographyType>::N; i++) {
		BibliographyGroupItem *gItem = new BibliographyGroupItem(_context, this, i);
		this->addChild(gItem);
	}
}

void BibliographyRootItem::execContextMenu(const QPoint &point)
{}

void BibliographyRootItem::createChildItem()
{}

BibliographyGroupItem::BibliographyGroupItem(TreeItemContext *context, QTreeWidgetItem *parent, quint8 id)
	:	BaseItem(context, parent),
		ParentItem(context, parent),
		_id(id)
{
	this->setText(0, EnumInfo<curspec::BibliographyType>::plural(id));

	size_t lb;
	size_t ub;
	try {
		KeyEditor prefixEd(&curspec::meta);
		prefixEd.select(&curspec::bibliographyClass);
		prefixEd.enumPut(curspec::BibliographyClass::Type, EnumInfo<curspec::BibliographyType>::fromIndex(id));
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			QByteArray key = _context->frontend->key(s);
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			BibliographyItem *item = new BibliographyItem(_context, this, key, value->id, this->objName(), true);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Error", "Error loading Bibliography: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
}

void BibliographyGroupItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QString objName = EnumInfo<curspec::BibliographyType>::text(_id);
	QAction *addBibliography = contextMenu->addAction("Add " + objName);
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addBibliography) {
		this->createChildItem();
	}

	delete contextMenu;
}

void BibliographyGroupItem::createChildItem()
{
	bool ok;
	QString itemName = QInputDialog::getText(this->treeWidget(),"Add " + this->objName(),"Name:", QLineEdit::Normal,"", &ok).simplified();
	if (!ok || itemName.isEmpty()) {
		return;
	}

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::bibliographyClass);
		keyEd.enumPut(curspec::BibliographyClass::Type, EnumInfo<curspec::BibliographyType>::fromIndex(_id));
		keyEd.stringPut(curspec::BibliographyClass::ObjectName, itemName);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;
				_context->frontend->modified(keyEd);
				BibliographyItem *newItem = new BibliographyItem(_context, this, keyEd, newId, this->objName());
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage(this->objName() + " \"" + itemName + "\" already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}
}

QString BibliographyGroupItem::objName()
{
	return EnumInfo<curspec::BibliographyType>::text(_id);
}

BibliographyItem::BibliographyItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, QString groupName, bool init)
	:	BaseItem(context, parent),
		DataItem(context, parent, key, id),
		_groupName(groupName)
{
	this->declKey = &curspec::bibliographyObject;
	KeyEditor keyEd(&curspec::meta, key);
	QString text = keyEd.stringGet(curspec::BibliographyClass::ObjectName);
	QIcon icon = QIcon(":/images/bibliography_icon.svg");
	this->setText(0,text);
	this->setIcon(0,icon);

	_completerItem = new QStandardItem(text);
	_completerItem->setIcon(icon);
	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(context->idCompleter->model());
	if (model){
		model->insertRow(0,_completerItem);
		if (!init) {
			model->sort(0);
		}
	}
}

void BibliographyItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *renameItem = contextMenu->addAction("Rename " + _groupName);
	QAction *deleteItem = contextMenu->addAction("Delete " + _groupName);
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == renameItem) {
		bool ok;
		QString itemName;
		itemName = QInputDialog::getText(this->treeWidget(),"Rename " + _groupName,"Name:", QLineEdit::Normal,this->text(0), &ok).simplified();
		if (!ok || itemName.isEmpty()) {
			delete contextMenu;
			return;
		} else {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				keyEd.stringPut(curspec::BibliographyClass::ObjectName, itemName);
				_context->frontend->move(_key,keyEd);
				this->setText(0,itemName);
				this->parent()->sortChildren(0, Qt::AscendingOrder);
				_key = keyEd;
				_context->centralWidget->relabelDataItemForms(this, itemName);
				_completerItem->setText(itemName);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->sort(0);
				}
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error renaming: " +  ex.message(), 4000);
			}
		}
	} else if (selectedAction == deleteItem) {
		if (QMessageBox::question(nullptr, "Delete " + _groupName, "Do you really want to delete this " + _groupName + "?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				_context->frontend->erase(_key);
				_context->centralWidget->closeDataItemForms(this);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->removeRow(_completerItem->row());
				}
				delete this;
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error deleting: " +  ex.message(), 4000);
			}
		}
	}

	delete contextMenu;
}

PhilRootItem::PhilRootItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	BaseItem(context, parent),
		ParentItem(context, parent)
{
	this->setText(0,"Philological Comments");

	size_t lb;
	size_t ub;
	try {
		KeyEditor prefixEd(&curspec::meta);
		prefixEd.select(&curspec::philCommentClass);
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			QByteArray key = _context->frontend->key(s);
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			PhilItem *item = new PhilItem(_context, this, key, value->id, true);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Warning", "Error loading Philological Comments: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
}

void PhilRootItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *addPhil = contextMenu->addAction("Add Philological Comment");
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addPhil) {
		this->createChildItem();
	}

	delete contextMenu;
}

void PhilRootItem::createChildItem()
{
	bool ok;
	QString itemName = QInputDialog::getText(this->treeWidget(),"Add Philological Comment","Name:", QLineEdit::Normal,"", &ok).simplified();
	if (!ok || itemName.isEmpty()) {
		return;
	}

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::philCommentClass);
		keyEd.stringPut(curspec::PhilCommentClass::ObjectName, itemName);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;
				_context->frontend->modified(keyEd);
				PhilItem *newItem = new PhilItem(_context, this, keyEd, newId);
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage("Philological Comment \"" + itemName + "\" already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}
}

PhilItem::PhilItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, bool init)
	:	BaseItem(context, parent),
		DataItem(context, parent, key, id)
{
	this->declKey = &curspec::philCommentObject;
	KeyEditor keyEd(&curspec::meta, key);
	QString text = keyEd.stringGet(curspec::PhilCommentClass::ObjectName);
	QIcon icon = QIcon(":/images/phil_icon.svg");
	this->setText(0,text);
	this->setIcon(0,icon);

	_completerItem = new QStandardItem(text);
	_completerItem->setIcon(icon);
	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(context->idCompleter->model());
	if (model){
		model->insertRow(0,_completerItem);
		if (!init) {
			model->sort(0);
		}
	}
}

void PhilItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *renameItem = contextMenu->addAction("Rename Philological Comment");
	QAction *deleteItem = contextMenu->addAction("Delete Philological Comment");

	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == renameItem) {
		bool ok;
		QString itemName;
		itemName = QInputDialog::getText(this->treeWidget(),"Rename Philological Comment","Name:", QLineEdit::Normal,this->text(0), &ok).simplified();
		if (!ok || itemName.isEmpty()) {
			delete contextMenu;
			return;
		} else {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				keyEd.stringPut(curspec::PhilCommentClass::ObjectName, itemName);
				_context->frontend->move(_key,keyEd);
				this->setText(0,itemName);
				this->parent()->sortChildren(0, Qt::AscendingOrder);
				_key = keyEd;
				_context->centralWidget->relabelDataItemForms(this, itemName);
				_completerItem->setText(itemName);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->sort(0);
				}
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error renaming: " +  ex.message(), 4000);
			}
		}
	} else if (selectedAction == deleteItem) {
		if (QMessageBox::question(nullptr, "Delete Philological Comment", "Do you really want to delete this Philological Comment?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				_context->frontend->erase(_key);
				_context->centralWidget->closeDataItemForms(this);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->removeRow(_completerItem->row());
				}
				delete this;
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error deleting: " +  ex.message(), 4000);
			}
		}
	}

	delete contextMenu;
}

HistRootItem::HistRootItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	BaseItem(context, parent),
		ParentItem(context, parent)
{
	this->setText(0,"Historical Comments");

	for (unsigned int i = 0; i < EnumInfo<curspec::HistCommentType>::N; i++) {
		HistGroupItem *hItem = new HistGroupItem(_context, this, i);
		this->addChild(hItem);
	}
}

void HistRootItem::execContextMenu(const QPoint &point)
{}

void HistRootItem::createChildItem()
{}

HistGroupItem::HistGroupItem(TreeItemContext *context, QTreeWidgetItem *parent, quint8 id)
	:	BaseItem(context, parent),
		ParentItem(context, parent),
		_id(id)
{
	this->setText(0, EnumInfo<curspec::HistCommentType>::plural(id));

	size_t lb;
	size_t ub;
	try {
		KeyEditor prefixEd(&curspec::meta);
		prefixEd.select(&curspec::histCommentClass);
		prefixEd.enumPut(curspec::HistCommentClass::Type, EnumInfo<curspec::HistCommentType>::fromIndex(id));
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			QByteArray key = _context->frontend->key(s);
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			HistItem *item = new HistItem(_context, this, key, value->id, this->objName(), true);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Warning", "Error loading Historical Comments: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
}

void HistGroupItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *addLocation = contextMenu->addAction("Add " + this->objName());
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addLocation) {
		this->createChildItem();
	}

	delete contextMenu;
}

void HistGroupItem::createChildItem()
{
	bool ok;
	QString itemName = QInputDialog::getText(this->treeWidget(),"Add " + this->objName(),"Name:", QLineEdit::Normal,"", &ok).simplified();
	if (!ok || itemName.isEmpty()) {
		return;
	}

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::histCommentClass);
		keyEd.enumPut(curspec::HistCommentClass::Type, EnumInfo<curspec::HistCommentType>::fromIndex(_id));
		keyEd.stringPut(curspec::HistCommentClass::ObjectName, itemName);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;
				_context->frontend->modified(keyEd);
				HistItem *newItem = new HistItem(_context, this, keyEd, newId, this->objName());
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage(this->objName() + " \"" + itemName + "\" already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}
}

QString HistGroupItem::objName()
{
	return EnumInfo<curspec::HistCommentType>::text(_id);
}

HistItem::HistItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, QString groupName, bool init)
	:	BaseItem(context, parent),
		DataItem(context, parent, key, id),
		_groupName(groupName)
{
	this->declKey = &curspec::histCommentObject;
	KeyEditor keyEd(&curspec::meta, key);
	QString text = keyEd.stringGet(curspec::HistCommentClass::ObjectName);
	QIcon icon = QIcon(":/images/hist_icon.svg");
	this->setText(0,text);
	this->setIcon(0,icon);

	_completerItem = new QStandardItem(text);
	_completerItem->setIcon(icon);
	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(context->idCompleter->model());
	if (model){
		model->insertRow(0,_completerItem);
		if (!init) {
			model->sort(0);
		}
	}
}

void HistItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *renameItem = contextMenu->addAction("Rename " + _groupName);
	QAction *deleteItem = contextMenu->addAction("Delete " + _groupName);
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == renameItem) {
		bool ok;
		QString itemName;
		itemName = QInputDialog::getText(this->treeWidget(),"Rename " + _groupName,"Name:", QLineEdit::Normal,this->text(0), &ok).simplified();
		if (!ok || itemName.isEmpty()) {
			delete contextMenu;
			return;
		} else {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				keyEd.stringPut(curspec::HistCommentClass::ObjectName, itemName);
				_context->frontend->move(_key,keyEd);
				this->setText(0,itemName);
				this->parent()->sortChildren(0, Qt::AscendingOrder);
				_key = keyEd;
				_context->centralWidget->relabelDataItemForms(this, itemName);
				_completerItem->setText(itemName);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->sort(0);
				}
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error renaming: " +  ex.message(), 4000);
			}
		}
	} else if (selectedAction == deleteItem) {
		if (QMessageBox::question(nullptr, "Delete " + _groupName, "Do you really want to delete this " + _groupName + "?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				_context->frontend->erase(_key);
				_context->centralWidget->closeDataItemForms(this);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->removeRow(_completerItem->row());
				}
				delete this;
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error deleting: " +  ex.message(), 4000);
			}
		}
	}

	delete contextMenu;
}

TextRootItem::TextRootItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	BaseItem(context, parent),
		ParentItem(context, parent)
{
	this->setText(0,"Texts");

	size_t lb;
	size_t ub;
	try {
		KeyEditor prefixEd(&curspec::meta);
		prefixEd.select(&curspec::textBook);
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			QByteArray key = _context->frontend->key(s);
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			BookItem *item = new BookItem(_context, this, key, value->id);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Warning", "Error loading Texts: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
}

void TextRootItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *addText = contextMenu->addAction("Add Book");
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addText) {
		this->createChildItem();
	}

	delete contextMenu;
}

void TextRootItem::createChildItem()
{
	bool ok;
	quint64 itemNumber;

	itemNumber = QInputDialog::getInt(this->treeWidget(), "Add Book", "Number:", 1, 1, 2147483647, 1, &ok);
	if (!ok) {
		return;
	}

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::textBook);
		keyEd.uintPut(curspec::TextBook::ObjectNumber, itemNumber);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;
				_context->frontend->modified(keyEd);
				BookItem *newItem = new BookItem(_context, this, keyEd, newId);
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage("Book Number \"" + QString::number(itemNumber) + "\" already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}
}

BookItem::BookItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id)
	:	BaseItem(context, parent),
		ParentItem(context, parent),
		_key(key),
		_id(id)
{
	KeyEditor prefixEd(&curspec::meta);
	prefixEd.select(&curspec::textLetter);
	prefixEd.uintPut(curspec::TextLetter::BookId, id);
	KeyEditor keyEd(&curspec::meta, key);

	unsigned int bookNumber = keyEd.uintGet(curspec::TextBook::ObjectNumber);
	this->setText(0, curspec::bookToName(bookNumber));
	this->setData(0, Qt::UserRole, bookNumber);

	size_t lb;
	size_t ub;
	try {
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			LetterItem *item = new LetterItem(_context, this, _context->frontend->key(s), value->id);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Warning", "Error loading Book Items: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
	this->setIcon(0,QIcon(":/images/book_icon.svg"));
}

void BookItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *addLetter = contextMenu->addAction("Add Letter");
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addLetter) {
		this->createChildItem();
	}
}

void BookItem::createChildItem()
{
	LetterInitDialog *letterInitDialog = new LetterInitDialog;
	if (letterInitDialog->exec() != QDialog::Accepted) {
		return;
	}

	quint64 itemNumber = letterInitDialog->letterId();
	QStringList itemText = letterInitDialog->letterText();

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::textLetter);
		keyEd.uintPut(curspec::TextLetter::ObjectNumber, itemNumber);
		keyEd.uintPut(curspec::TextLetter::BookId, _id);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;

				KeyEditor keyEd2(&curspec::meta);
				keyEd2.select(&curspec::textContent);
				keyEd2.uintPut(curspec::TextContent::LetterId, newId);
				curspec::TextAnnotated *value2 = _context->frontend->get<curspec::TextAnnotated>(keyEd2, true);
				if (value2) {
					value2->text = itemText;
				}
				_context->frontend->modified(keyEd);
				LetterItem *newItem = new LetterItem(_context, this, keyEd, newId);
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage("Letter already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}

	delete letterInitDialog;
}

bool BookItem::operator<(const QTreeWidgetItem &other) const
{
	return this->data(0, Qt::UserRole).toUInt() < other.data(0, Qt::UserRole).toUInt();
}


LetterItem::LetterItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id)
	:	BaseItem(context, parent),
		ParentItem(context, parent),
		DataItem(context, parent, key, id)
{
	this->declKey = &curspec::textLetter;
	KeyEditor keyEd(&curspec::meta, key);
	KeyEditor objectKeyEd(&curspec::meta, key);
	unsigned int letterNumber = keyEd.uintGet(curspec::TextLetter::ObjectNumber);
	this->setText(0,curspec::letterToName(letterNumber));
	this->setData(0, Qt::UserRole, letterNumber);
	_bookId = keyEd.uintGet(curspec::TextLetter::BookId);
	this->setIcon(0,QIcon(":/images/letter_icon.svg"));
}

void LetterItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *deleteItem = contextMenu->addAction("Delete Letter");

	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == deleteItem) {
		if (QMessageBox::question(nullptr, "Delete Letter", "Do you really want to delete this Letter?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
			try {
				_context->centralWidget->closeDataItemForms(this);
				_context->frontend->prefixErase(_key);
				delete this;
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error deleting: " +  ex.message(), 4000);
			}
		}
	}
}

void LetterItem::createChildItem()
{}

bool LetterItem::operator<(const QTreeWidgetItem &other) const
{
	return this->data(0, Qt::UserRole).toUInt() < other.data(0, Qt::UserRole).toUInt();
}

IntroRootItem::IntroRootItem(TreeItemContext *context, QTreeWidgetItem *parent)
	:	BaseItem(context, parent),
		ParentItem(context, parent)
{
	this->setText(0,"Intro");

	size_t lb;
	size_t ub;
	try {
		KeyEditor prefixEd(&curspec::meta);
		prefixEd.select(&curspec::introClass);
		_context->frontend->prefixRange(&lb, &ub, prefixEd);
		for (size_t s = lb; s < ub; s++) {
			QByteArray key = _context->frontend->key(s);
			curspec::ObjectRef *value = _context->frontend->value<curspec::ObjectRef>(s);
			IntroItem *item = new IntroItem(_context, this, key, value->id, true);
			this->addChild(item);
		}
		this->sortChildren(0, Qt::AscendingOrder);
	} catch (const Exception &ex) {
		QMessageBox msgBox(QMessageBox::Warning, "Warning", "Error loading Intro: " + ex.message(), QMessageBox::Ok);
		msgBox.exec();
	}
}

void IntroRootItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *addIntro = contextMenu->addAction("Add Section");
	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == addIntro) {
		this->createChildItem();
	}

	delete contextMenu;
}

void IntroRootItem::createChildItem()
{
	bool ok;
	QString itemName = QInputDialog::getText(this->treeWidget(),"Add Section","Name:", QLineEdit::Normal,"", &ok).simplified();
	if (!ok || itemName.isEmpty()) {
		return;
	}

	try {
		KeyEditor keyEd(&curspec::meta);
		keyEd.select(&curspec::introClass);
		keyEd.stringPut(curspec::IntroClass::ObjectName, itemName);
		if (!_context->frontend->contains(keyEd)) {
			quint64 newId = _context->frontend->acquire(&curspec::objectId);
			if(newId) {
				curspec::ObjectRef *value = _context->frontend->get<curspec::ObjectRef>(keyEd, true);
				value->id = newId;
				_context->frontend->modified(keyEd);
				IntroItem *newItem = new IntroItem(_context, this, keyEd, newId);
				this->addChild(newItem);
				this->sortChildren(0, Qt::AscendingOrder);
			}
		} else {
			_context->mainWindow->statusBar()->showMessage("Section \"" + itemName + "\" already exists", 4000);
		}
	} catch (const Exception &ex) {
		_context->mainWindow->statusBar()->showMessage("Error creating: " +  ex.message(), 4000);
	}
}

IntroItem::IntroItem(TreeItemContext *context, QTreeWidgetItem *parent, const QByteArray &key, quint64 id, bool init)
	:	BaseItem(context, parent),
		DataItem(context, parent, key, id)
{
	this->declKey = &curspec::introObject;
	KeyEditor keyEd(&curspec::meta, key);
	QString text = keyEd.stringGet(curspec::IntroClass::ObjectName);
	QIcon icon = QIcon(":/images/intro_icon.svg");
	this->setText(0,text);
	this->setIcon(0,icon);

	_completerItem = new QStandardItem(text);
	_completerItem->setIcon(icon);
	QStandardItemModel *model = qobject_cast<QStandardItemModel *>(context->idCompleter->model());
	if (model){
		model->insertRow(0,_completerItem);
		if (!init) {
			model->sort(0);
		}
	}
}

void IntroItem::execContextMenu(const QPoint &point)
{
	QMenu *contextMenu = new QMenu;
	QAction *renameItem = contextMenu->addAction("Rename Section");
	QAction *deleteItem = contextMenu->addAction("Delete Section");

	QAction *selectedAction = contextMenu->exec(point);

	if (selectedAction == renameItem) {
		bool ok;
		QString itemName;
		itemName = QInputDialog::getText(this->treeWidget(),"Rename Section","Name:", QLineEdit::Normal,this->text(0), &ok).simplified();
		if (!ok || itemName.isEmpty()) {
			delete contextMenu;
			return;
		} else {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				keyEd.stringPut(curspec::IntroClass::ObjectName, itemName);
				_context->frontend->move(_key,keyEd);
				this->setText(0,itemName);
				this->parent()->sortChildren(0, Qt::AscendingOrder);
				_key = keyEd;
				_context->centralWidget->relabelDataItemForms(this, itemName);
				_completerItem->setText(itemName);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->sort(0);
				}
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error renaming: " +  ex.message(), 4000);
			}
		}
	} else if (selectedAction == deleteItem) {
		if (QMessageBox::question(nullptr, "Delete Section", "Do you really want to delete this Section?", QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Ok) {
			try {
				KeyEditor keyEd(&curspec::meta, _key);
				_context->frontend->erase(_key);
				_context->centralWidget->closeDataItemForms(this);
				QStandardItemModel *model = qobject_cast<QStandardItemModel *>(_context->idCompleter->model());
				if (model){
					model->removeRow(_completerItem->row());
				}
				delete this;
			} catch (const Exception &ex) {
				_context->mainWindow->statusBar()->showMessage("Error deleting: " +  ex.message(), 4000);
			}
		}
	}

	delete contextMenu;
}
