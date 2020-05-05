#include <QFileDialog>
#include <QDebug>
#include <QIcon>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QWidget>
#include <QTextBrowser>
#include <QCollator>

#include "qtfe/qtfe.h"
#include "mainwindow.h"
#include "editform.h"
#include "treedata.h"
#include "searchform.h"
#include "types.h"

using namespace spec;

SaveStateIndicator::SaveStateIndicator(QWidget *parent)
	:	QPushButton(parent),
		_icoUnsaved(QIcon(":/images/unsaved_icon.svg")),
		_icoSaved(QIcon(":/images/saved_icon.svg")),
		_saved(true)
{
	this->setIcon(_icoSaved);
	this->setStatusTip("Indicates if there are unsaved Changes on current tab");
}

void SaveStateIndicator::setUnsaved()
{
	_saved = false;
	this->setIcon(_icoUnsaved);
}

void SaveStateIndicator::setSaved()
{
	_saved = true;
	this->setIcon(_icoSaved);
}

CentralWidget::CentralWidget(QString dirname, MainWindow *parent)
	:	QWidget(parent),
		_lastTabIndex(-1)
{
	_ui.setupUi(this);

	curspec::DirFrontend *dirFrontend = new curspec::DirFrontend(QDir(dirname));
	_treeItemContext.frontend = dirFrontend;
	_treeItemContext.tabWidget = _ui.tabWidget;
	_treeItemContext.langComboBox = _ui.comboBox;
	_treeItemContext.mainWindow = parent;
	_treeItemContext.centralWidget = this;
	_treeItemContext.treeWidget = _ui.treeWidget;

	QCollator *sorter = new QCollator();
	sorter->setNumericMode(true);
	sorter->setCaseSensitivity(Qt::CaseInsensitive);
	_treeItemContext.sorter = sorter;

	SaveStateIndicator *saveState = new SaveStateIndicator(this);
	QHBoxLayout *hLt = new QHBoxLayout();
	hLt->addStretch();
	hLt->addWidget(saveState);
	_ui.formLayout->addRow(hLt);
	_treeItemContext.saveState = saveState;

	QCompleter *completer = new QCompleter(this);
	completer->setModel(new QStandardItemModel(completer));
	completer->setMaxVisibleItems(15);
	completer->setWrapAround(false);
	completer->setModelSorting(QCompleter::CaseSensitivelySortedModel);
	_treeItemContext.idCompleter = completer;

	connect(saveState, SIGNAL(clicked()), this, SLOT(saveCurrentTab()));
	connect(_ui.lineEdit, SIGNAL(returnPressed()), this, SLOT(filterTreeItems()));
	connect(_ui.treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(treeWidgetContextMenu(const QPoint)));
	connect(_ui.treeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(openTreeItem(QTreeWidgetItem *)));
	connect(_ui.treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(_ui.tabWidget, SIGNAL(tabCloseRequested(int)),this,SLOT(tabCloseRequested(int)));
	connect(_ui.tabWidget, SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));
	_ui.tabWidget->tabBar()->installEventFilter(this);
	_ui.treeWidget->viewport()->installEventFilter(this);

	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "annotate/mainwindow");
	if(!_ui.splitter->restoreState(settings.value("treeTabSplitter").toByteArray())) {
		_ui.splitter->setSizes({ 5, 10 });
	}

	dirFrontend->load();

	unsigned int presetLang = settings.value("presetLanguage").toUInt();

	for(unsigned int i = 0; i < EnumInfo<curspec::LanguageId>::N; i++) {
		QLocale lcle(EnumInfo<curspec::LanguageId>::name(i));
		_ui.comboBox->addItem(lcle.nativeLanguageName(), i);
	}
	if (presetLang >= EnumInfo<curspec::LanguageId>::N) {
		presetLang = 0;
	}
	_ui.comboBox->setCurrentIndex(presetLang);

	_treeItemContext.personRoot = new PersonRootItem(&_treeItemContext, _ui.treeWidget->invisibleRootItem());
	_treeItemContext.locationRoot = new LocationRootItem(&_treeItemContext, _ui.treeWidget->invisibleRootItem());
	_treeItemContext.bibliographyRoot = new BibliographyRootItem(&_treeItemContext, _ui.treeWidget->invisibleRootItem());
	_treeItemContext.philRoot = new PhilRootItem(&_treeItemContext, _ui.treeWidget->invisibleRootItem());
	_treeItemContext.histRoot = new HistRootItem(&_treeItemContext, _ui.treeWidget->invisibleRootItem());
	_treeItemContext.textRoot = new TextRootItem(&_treeItemContext, _ui.treeWidget->invisibleRootItem());
	_treeItemContext.introRoot = new IntroRootItem(&_treeItemContext, _ui.treeWidget->invisibleRootItem());
	_ui.treeWidget->sortItems(true, Qt::AscendingOrder);

	completer->model()->sort(0);
}

CentralWidget::~CentralWidget() {
	QSettings settings(QSettings::IniFormat, QSettings::UserScope, "annotate/mainwindow");
	settings.setValue("treeTabSplitter", _ui.splitter->saveState());

	int i;
	for (i =_ui.tabWidget->count() - 1; _ui.tabWidget->count(); --i) {
		delete _ui.tabWidget->widget(i);
	}

	for (i = _ui.comboBox->count() - 1; _ui.comboBox->count(); --i) {
		_ui.comboBox->removeItem(i);
	}
	for (i = _ui.treeWidget->topLevelItemCount() - 1; _ui.treeWidget->topLevelItemCount(); --i) {
		delete _ui.treeWidget->takeTopLevelItem(i);
	}
	delete _treeItemContext.frontend;
	delete _treeItemContext.saveState;
}

void CentralWidget::dumpTree()
{
	for (int i = 0; i < _ui.treeWidget->topLevelItemCount(); ++i) {
		ParentItem *parentItem = dynamic_cast<ParentItem *>(_ui.treeWidget->topLevelItem(i));
		if (parentItem) {
			parentItem->dump();
		}
	}
}

void CentralWidget::dumpFrontend()
{
	_treeItemContext.frontend->dump();
}

void CentralWidget::filterTreeItems()
{
	QList<QTreeWidgetItem *> foundItems = _ui.treeWidget->findItems(_ui.lineEdit->text(), Qt::MatchRecursive | Qt::MatchContains, 0);
	QTreeWidgetItemIterator it(_ui.treeWidget);
	while (*it) {
		DataItem *dataItem = dynamic_cast<DataItem *>(*it);
		if (dataItem) {
			if (foundItems.contains(*it) || _ui.lineEdit->text().isEmpty()) {
				(*it)->setHidden(false);
			} else {
				(*it)->setHidden(true);
			}
		}
		++it;
	}
}

void CentralWidget::treeWidgetContextMenu(const QPoint &point)
{
	QTreeWidgetItem *clickedItem = _ui.treeWidget->itemAt(point);
	if (clickedItem) {
		BaseItem *item = dynamic_cast<BaseItem *>(clickedItem);
		if (item) {
			item->execContextMenu(_ui.treeWidget->viewport()->mapToGlobal(point));
		}
	}
}

void CentralWidget::openTreeItem(QTreeWidgetItem *item)
{
	curspec::LanguageId langId = EnumInfo<curspec::LanguageId>::fromIndex(_ui.comboBox->currentIndex());
	this->openEditForm(item, langId);
}

void CentralWidget::openEditForm(QTreeWidgetItem *item, curspec::LanguageId langId)
{
	DataItem *itm = dynamic_cast<DataItem *>(item);
	if (itm) {
		EditForm *editForm = _editTabs.value(QPair<quint64, curspec::LanguageId>(itm->id(), langId), nullptr);
		if (!editForm) {
			editForm = new EditForm(&_treeItemContext, itm, langId);
			QString tabTitle;
			LetterItem *letterItem = dynamic_cast<LetterItem *>(item);
			if (letterItem) {
				tabTitle += letterItem->parent()->text(0) + ", ";
			}
			tabTitle += item->text(0) + " - " + EnumInfo<curspec::LanguageId>::name(langId);
			_ui.tabWidget->addTab(editForm,item->icon(0),tabTitle);
			_editTabs.insert(QPair<quint64, curspec::LanguageId>(itm->id(), langId), editForm);
		}
		_treeItemContext.tabWidget->setCurrentWidget(editForm);
	}
}

void CentralWidget::currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous)
{
	DataItem *itm = dynamic_cast<DataItem *>(current);
	if (itm) {
		curspec::LanguageId langId = EnumInfo<curspec::LanguageId>::fromIndex(_ui.comboBox->currentIndex());
		EditForm *editForm = _editTabs.value(QPair<quint64, curspec::LanguageId>(itm->id(), langId), nullptr);
		if (editForm) {
			_treeItemContext.tabWidget->setCurrentWidget(editForm);
		}
	}
}

void CentralWidget::relabelDataItemForms(DataItem *item, QString name)
{
	for(unsigned int i = 0; i < EnumInfo<curspec::LanguageId>::N; i++) {
		curspec::LanguageId langId = EnumInfo<curspec::LanguageId>::fromIndex(i);
		EditForm *editForm = _editTabs.value(QPair<quint64, curspec::LanguageId>(item->id(), langId), nullptr);
		if (editForm) {
			int index = _ui.tabWidget->indexOf(editForm);
			if (index > -1) {
				QString tabText = _ui.tabWidget->tabText(index);
				_ui.tabWidget->setTabText(index, name + " - " + EnumInfo<curspec::LanguageId>::name(langId));
			}
		}
	}
}

void CentralWidget::closeDataItemForms(DataItem *item)
{
	for(unsigned int i = 0; i < EnumInfo<curspec::LanguageId>::N; i++) {
		curspec::LanguageId langId = EnumInfo<curspec::LanguageId>::fromIndex(i);
		EditForm *editForm = _editTabs.take(QPair<quint64, curspec::LanguageId>(item->id(), langId));
		if (editForm) {
			editForm->commitChanges();
			delete editForm;
		}
	}
}

void CentralWidget::tabCloseRequested(int index)
{
	QWidget *widget = _ui.tabWidget->widget(index);
	EditForm *editForm = qobject_cast<EditForm *>(widget);

	if (editForm) {
		editForm->commitChanges();
		_editTabs.remove(QPair<quint64, curspec::LanguageId>(editForm->item()->id(), editForm->langId()));
	}
	delete widget;
}

void CentralWidget::tabChanged(int index)
{
	EditForm *editForm = qobject_cast<EditForm *>(_ui.tabWidget->widget(_lastTabIndex));
	if (editForm) {
		editForm->commitChanges();
	}
	_lastTabIndex = index;
}

void CentralWidget::searchItems()
{
	SearchForm *searchForm = new SearchForm(&_treeItemContext, this);
	searchForm->show();
}

void CentralWidget::saveCurrentTab()
{
	EditForm *editForm = qobject_cast<EditForm *>(_ui.tabWidget->currentWidget());
	if (editForm) {
		editForm->commitChanges();
	}
}

void CentralWidget::closeAllTabs()
{
	while (_ui.tabWidget->count()) {
		EditForm *editForm = qobject_cast<EditForm *>(_ui.tabWidget->widget(0));
		if (editForm) {
			editForm->commitChanges();
			_editTabs.remove(QPair<quint64, curspec::LanguageId>(editForm->item()->id(), editForm->langId()));
		}
		delete _ui.tabWidget->widget(0);
	}
}

bool CentralWidget::eventFilter(QObject *obj, QEvent *event)
{
	//qInfo() << event->type();
	if (event->type() == QEvent::MouseButtonPress) {
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
		QPoint point = mouseEvent->pos();
		if (obj == _ui.tabWidget->tabBar() && mouseEvent->button() == Qt::RightButton) {
			int tabIndex = _ui.tabWidget->tabBar()->tabAt(point);
			QMenu *contextMenu = new QMenu;
			QAction *closeOtherTabs = contextMenu->addAction("Close other tabs");
			if (_ui.tabWidget->count() < 2) {
				closeOtherTabs->setEnabled(false);
			} else {
				closeOtherTabs->setEnabled(true);
			}
			QAction *selectedAction = contextMenu->exec(_ui.tabWidget->mapToGlobal(point));
			if (selectedAction == closeOtherTabs) {
				int i = _ui.tabWidget->count() - 1;
				while (_ui.tabWidget->count() > 1) {
					if (i != tabIndex) {
						EditForm *form = dynamic_cast<EditForm *>(_ui.tabWidget->widget(i));
						if (form) {
							_editTabs.remove(QPair<quint64, curspec::LanguageId>(form->item()->id(), form->langId()));
						}
						delete _ui.tabWidget->widget(i);
					}
					--i;
				}
			}
			delete contextMenu;
			return false; // prevent event from being sent on to other objects
		} else if (obj == _ui.treeWidget->viewport() && mouseEvent->button() == Qt::LeftButton) {
			QTreeWidgetItem *item = _ui.treeWidget->itemAt(point);
			DataItem *dataItem = dynamic_cast<DataItem *>(item);
			if (dataItem) {
				QDrag *drag = new QDrag(this);
				QMimeData *mimeData = new QMimeData;
				mimeData->setText(item->text(0));
				drag->setMimeData(mimeData);
				drag->exec();
				return false;
			} else {
				return QWidget::eventFilter(obj, event);
			}
		} else {
			return QWidget::eventFilter(obj, event);
		}
	} else {
		return QWidget::eventFilter(obj, event);
	}
}
