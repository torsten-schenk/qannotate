#pragma once

#include <QTabWidget>
#include <QComboBox>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QCollator>

#ifdef DUMP_EXCEPTION_CTOR
#ifdef __linux__
#include <backward.hpp>
static inline void stacktrace(ssize_t skip = 0) {
	backward::StackTrace st;
	backward::Printer p;
	st.load_here(32, skip + 3);
	p.print(st);
}
#else
static inline void stacktrace(ssize_t skip = 0) {}
#endif
#endif

#include "qtfe/qtfe.h"

class AnnotEditWidget;
class AnnotEdit;
class BaseItem;
class BibliographyGroupItem;
class BibliographyItem;
class BibliographyRootItem;
class BookItem;
class CategoryEdit;
class CentralWidget;
class CommentItem;
class ContentEdit;
class Converter;
class CustomTabStyle;
class DataItem;
class EditForm;
class EnlargeDialog;
class HistGroupItem;
class HistItem;
class HistRootItem;
class IniFile;
class IntroItem;
class IntroRootItem;
class LetterInitDialog;
class LetterItem;
class LineEdit;
class LocationGroupItem;
class LocationItem;
class LocationRootItem;
class MainWindow;
class MetadataEdit;
class MetadataEditForm;
class OptionEdit;
class ParentItem;
class PersonItem;
class PersonRootItem;
class PhilItem;
class PhilRootItem;
class SaveStateIndicator;
class SearchForm;
class SearchResultItem;
class SettingsDialog;
class TextEdit;
class TextRootItem;

struct TreeItemContext {
	SyncFrontend *frontend;
	QTabWidget *tabWidget;
	QComboBox *langComboBox;
	MainWindow *mainWindow;
	CentralWidget *centralWidget;
	QTreeWidget *treeWidget;
	PersonRootItem *personRoot;
	LocationRootItem *locationRoot;
	BibliographyRootItem *bibliographyRoot;
	TextRootItem *textRoot;
	PhilRootItem *philRoot;
	HistRootItem *histRoot;
	IntroRootItem *introRoot;
	QStatusBar *statusBar;
	SaveStateIndicator *saveState;
	QCompleter *idCompleter;
	QCollator *sorter;
};

namespace curspec = spec::v1_0;
