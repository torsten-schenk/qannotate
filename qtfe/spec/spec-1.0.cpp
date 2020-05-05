#include <QRegularExpression>
#include <functional>
#include <QTextStream>
#include <QtEndian>

#include "../src/spec.h"
#include "../src/key.h"

#include "frontend-1.0.h"

namespace {
	void parseProperties(QIODevice *source, const std::function<void(const QString&, const QString&, const QStringList&)> &cb) {
		static QString indent = "\t";
		static QString identifier = "[a-zA-Z-_][a-zA-Z0-9-_]*";
		static QRegularExpression regexProperty("^(" + identifier + ")(?:\\s*\\[\\s*(" + identifier + ")\\s*\\])?\\s*:\\s*$");
		static QRegularExpression regexEmpty("^\\s*$");

		QTextStream stream(source);
		stream.setCodec("UTF-8");
		QRegularExpressionMatch match;
		int nEmpty = 0;
		QStringList text;
		QString language;
		QString property;

		for(;;) {
			QString line = stream.readLine();
			bool indented = line.startsWith(indent);
			bool empty = regexEmpty.match(line).hasMatch();
			auto propertyMatch = regexProperty.match(line);
			if((propertyMatch.hasMatch() || line.isNull()) && !property.isEmpty()) {
				cb(property, language, text);
				text.clear();
				nEmpty = 0;
			}
			if(line.isNull())
				break;
			else if(propertyMatch.hasMatch()) {
				property = propertyMatch.captured(1);
				language = propertyMatch.captured(2);
			}
			else if(empty)
				nEmpty++;
			else if(indented) {
				while(nEmpty--)
					text.append("");
				nEmpty = 0;
				text.append(line.mid(indent.size()));
			}
			else
				throw FrontendException("invalid line in properties file : '" + line + "'");
		}
	}

	void parseText(QIODevice *source, const std::function<void(const QString&, const QStringList &text)> &cb) {
		static QString indent = "\t";
		static QString identifier = "[a-zA-Z-_][a-zA-Z0-9-_]*";
		static QRegularExpression regexHead("^\\[(" + identifier + ")\\]\\s*:\\s*$");
		static QRegularExpression regexEmpty("^\\s*$");

		QTextStream stream(source);
		stream.setCodec("UTF-8");
		QRegularExpressionMatch match;
		QStringList text;
		QString language;
		int nEmpty = 0;

		for(;;) {
			QString line = stream.readLine();
			bool indented = line.startsWith(indent);
			auto headMatch = regexHead.match(line);
			bool empty = regexEmpty.match(line).hasMatch();
			if((headMatch.hasMatch() || line.isNull()) && !text.isEmpty()) {
				cb(language, text);
				text.clear();
				nEmpty = 0;
			}
			if(line.isNull())
				break;
			else if(headMatch.hasMatch())
				language = headMatch.captured(1);
			else if(empty)
				nEmpty++;
			else if(indented) {
				while(nEmpty--)
					text.append("");
				nEmpty = 0;
				text.append(line.mid(indent.size()));
			}
			else
				throw FrontendException("invalid line in text file");
		}
	}

	void parseMarkup(QIODevice *source, const std::function<void(quint64 pbegin, quint64 wbegin, quint64 pend, quint64 wend, spec::v1_0::OptionPunc punc, const QMap<QString, QStringList> &text)> &cb) {
		static QString indent = "\t";
		static QString identifier = "[a-zA-Z-_][a-zA-Z0-9-_]*";
		static QRegularExpression regexMarkup("^@p([0-9]+)w([0-9]+)(?:\\s*-\\s*p([0-9]+)w([0-9]+))?(\\|(\\.)?)?\\s*:\\s*$");
		static QRegularExpression regexLanguage("^" + indent + "\\[\\s*(" + identifier + ")\\s*\\]\\s*$");
		static QRegularExpression regexEmpty("^\\s*$");
		static QRegularExpression regexText("^" + indent + indent + "(.*)");

		QTextStream stream(source);
		stream.setCodec("UTF-8");
		QRegularExpressionMatch match;
		QMap<QString, QStringList> result;
		int nEmpty = -1;
		QStringList text;
		QString language;
		QString markup;
		quint64 pbegin = 0;
		quint64 wbegin = 0;
		quint64 pend = 0;
		quint64 wend = 0;
		spec::v1_0::OptionPunc punc = spec::v1_0::OptionPunc::Unset;

		for(;;) {
			QString line = stream.readLine();
			bool empty = regexEmpty.match(line).hasMatch();
			auto markupMatch = regexMarkup.match(line);
			auto langMatch = regexLanguage.match(line);
			auto textMatch = regexText.match(line);
			if((langMatch.hasMatch() || markupMatch.hasMatch() || line.isNull()) && !language.isEmpty()) {
				result.insert(language, text);
				language = QString();
				text.clear();
			}
			if((markupMatch.hasMatch() || line.isNull()) && nEmpty >= 0) {
				cb(pbegin, wbegin, pend, wend, punc, result);
				result.clear();
				text.clear();
				nEmpty = 0;
			}
			if(line.isNull())
				break;
			else if(markupMatch.hasMatch()) {
				pbegin = markupMatch.captured(1).toULongLong();
				wbegin = markupMatch.captured(2).toULongLong();
				if(!markupMatch.captured(3).isNull()) {
					pend = markupMatch.captured(3).toULongLong();
					wend = markupMatch.captured(4).toULongLong();
				}
				else {
					pend = pbegin;
					wend = wbegin;
				}
				if(markupMatch.captured(5).isNull())
					punc = spec::v1_0::OptionPunc::Unset;
				else if(markupMatch.captured(6).isNull())
					punc = spec::v1_0::OptionPunc::Include;
				else
					punc = spec::v1_0::OptionPunc::Exclude;
			}
			else if(langMatch.hasMatch())
				language = langMatch.captured(1);
			else if(empty)
				nEmpty++;
			else if(textMatch.hasMatch()) {
				while(nEmpty-- > 0)
					text.append("");
				nEmpty = 0;
				text.append(textMatch.captured(1));
			}
			else
				throw FrontendException("invalid line in markup file");
		}
	}
}

namespace spec { namespace v1_0 {
	bool commentLessThan(const decltype(spec::v1_0::TextAnnotated::comments)::value_type &a, const decltype(spec::v1_0::TextAnnotated::comments)::value_type &b)
	{
		if(a.pbegin < b.pbegin)
			return true;
		else if(a.pbegin > b.pbegin)
			return false;
		else if(a.wbegin < b.wbegin)
			return true;
		else if(a.wbegin > b.wbegin)
			return false;
		else if(a.pend < b.pend)
			return true;
		else if(a.pend > b.pend)
			return false;
		else
			return a.wend < b.wend;
#if 0
		else if(a.wend < b.wend)
			return true;
		else if(a.wend > b.wend)
			return false;
		else if(a.punc == (quint8)spec::v1_0::OptionPunc::Exclude && (b.punc == (quint8)spec::v1_0::OptionPunc::Include || b.punc == (quint8)spec::v1_0::OptionPunc::Unset))
			return true;
		else
			return false;
#endif
	}

	DirFrontend::DirFrontend(const QDir &root)
		:	AbstractDirFrontend(&meta, root)
	{}

	QDir DirFrontend::categoryDir(const QString &category, bool *exmk)
	{
		QDir dir = rootdir();
		if(!dir.cd(category)) {
			if(!*exmk)
				return dir;
			else if(!dir.mkdir(category))
				throw FrontendException("Error creating category subdir");
			else if(!dir.cd(category))
				throw FrontendException("Error entering category subdir");
		}
		*exmk = true;
		return dir;
	}

	QDir DirFrontend::categoryDir(const QStringList &category, bool *exmk)
	{
		QDir dir = rootdir();
		for(auto it : category) {
			if(!dir.cd(it)) {
				if(!*exmk)
					return dir;
				else if(!dir.mkdir(it))
					throw FrontendException("Error creating category subdir");
				else if(!dir.cd(it))
					throw FrontendException("Error entering category subdir");
			}
		}
		*exmk = true;
		return dir;
	}

	QDir DirFrontend::letterDir(quint64 book, quint64 letter, bool *exmk)
	{
		QDir dir = rootdir();
		QString bookstr = QString("%1").arg(book);
		QString letstr = QString("%1").arg(letter);
		if(!dir.cd("letter")) {
			if(!*exmk)
				return dir;
			else if(!dir.mkdir("letter"))
				throw FrontendException("Error creating letter subdir");
			else if(!dir.cd("letter"))
				throw FrontendException("Error entering letter subdir");
		}
		if(!dir.cd(bookstr)) {
			if(!*exmk)
				return dir;
			else if(!dir.mkdir(bookstr))
				throw FrontendException("Error creating book subdir");
			else if(!dir.cd(bookstr))
				throw FrontendException("Error entering book subdir");
		}
		if(!dir.cd(letstr)) {
			if(!*exmk)
				return dir;
			else if(!dir.mkdir(letstr))
				throw FrontendException("Error creating letter subdir");
			else if(!dir.cd(letstr))
				throw FrontendException("Error entering letter subdir");
		}
		*exmk = true;
		return dir;
	}

	void DirFrontend::loadPersons()
	{
		bool exmk = false;
		QDir dir = categoryDir("person", &exmk);
		if(!exmk)
			return;
		QStringList entries = dir.entryList(QDir::Files);
		KeyEditor clsed(&personClass);
		KeyEditor objed(&personObject);
		for(auto it : entries) {
			QFile file(dir.absoluteFilePath(it));
			if(!file.open(QFile::ReadOnly))
				throw FrontendException("Error opening person file");
			QString name = decodeFilename(it);
			quint64 objid = acquire(&objectId);
			clsed.stringPut(PersonClass::ObjectName, name);
			SyncFrontend::get<ObjectRef>(clsed, true)->id = objid;
			objed.uintPut(PersonObject::ObjectId, objid);
			parseProperties(&file, [&](const QString &name, const QString &language, const QStringList &text){
				quint64 propid = EnumInfo<PersonPropertyId>::findString(name);
				quint64 langid = EnumInfo<LanguageId>::findString(language);
				if(!propid)
					throw FrontendException("No such property for person");
				else if(!language.isNull() && !langid)
					throw FrontendException("No such language for person property");
				objed.uintPut(PersonObject::PropertyId, propid);
				objed.uintPut(PersonObject::LanguageId, langid);
				auto value = get(objed, true);
				auto mapto = objed.mapto();
				if(mapto == &textLine) {
					if(text.size() == 1)
						value->to<TextLine>()->text = text.at(0);
					else if(text.size() > 1)
						throw FrontendException("multiple lines of text for single-line text property for person");
				}
				else if(mapto == &textMultiline)
					value->to<TextMultiline>()->text = text;
				else if(mapto == &personSex) {
					if(text.size() == 1)
						value->to<PersonSex>()->sex = EnumInfo<OptionSex>::findString(text.at(0));
					else if(text.size() > 1)
						throw FrontendException("multiple lines of text for single-line text property for person");
				}
				else
					throw FrontendException("(internal error) value type for person property not handled in spec-1.0.cpp");
			});
		}
	}

	void DirFrontend::loadPhilComments()
	{
		bool exmk = false;
		QDir dir = categoryDir("phil-comment", &exmk);
		if(!exmk)
			return;
		QStringList entries = dir.entryList(QDir::Files);
		KeyEditor clsed(&philCommentClass);
		KeyEditor objed(&philCommentObject);
		for(auto it : entries) {
			QFile file(dir.absoluteFilePath(it));
			if(!file.open(QFile::ReadOnly))
				throw FrontendException("Error opening philological comment file");
			QString name = decodeFilename(it);
			quint64 objid = acquire(&objectId);
			clsed.stringPut(PhilCommentClass::ObjectName, name);
			SyncFrontend::get<ObjectRef>(clsed, true)->id = objid;
			objed.uintPut(PhilCommentObject::ObjectId, objid);
			parseProperties(&file, [&](const QString &name, const QString &language, const QStringList &text){
				quint64 propid = EnumInfo<CommentPropertyId>::findString(name);
				quint64 langid = EnumInfo<LanguageId>::findString(language);
				if(!propid)
					throw FrontendException("No such property for person");
				else if(!language.isNull() && !langid)
					throw FrontendException("No such language for person property");
				objed.uintPut(PhilCommentObject::PropertyId, propid);
				objed.uintPut(PhilCommentObject::LanguageId, langid);
				auto value = get(objed, true);
				auto mapto = objed.mapto();
				if(mapto == &textLine) {
					if(text.size() == 1)
						value->to<TextLine>()->text = text.at(0);
					else if(text.size() > 1)
						throw FrontendException("multiple lines of text for single-line text property for person");
				}
				else if(mapto == &textMultiline)
					value->to<TextMultiline>()->text = text;
				else
					throw FrontendException("(internal error) value type for person property not handled in spec-1.0.cpp");
			});
		}
	}

	void DirFrontend::loadHistComments()
	{
		for(size_t i = 0; i < EnumInfo<HistCommentType>::N; i++) {
			bool exmk = false;
			QDir dir = categoryDir(QStringList({ "hist-comment", EnumInfo<HistCommentType>::name(i) }), &exmk);
			if(!exmk)
				continue;
			QStringList entries = dir.entryList(QDir::Files);
			KeyEditor clsed(&histCommentClass);
			KeyEditor objed(&histCommentObject);
			clsed.enumPut(HistCommentClass::Type, EnumInfo<HistCommentType>::fromIndex(i));
			for(auto it : entries) {
				QFile file(dir.absoluteFilePath(it));
				if(!file.open(QFile::ReadOnly))
					throw FrontendException("Error opening historical comment file");
				QString name = decodeFilename(it);
				quint64 objid = acquire(&objectId);
				clsed.stringPut(HistCommentClass::ObjectName, name);
				SyncFrontend::get<ObjectRef>(clsed, true)->id = objid;
				objed.uintPut(HistCommentObject::ObjectId, objid);
				parseProperties(&file, [&](const QString &name, const QString &language, const QStringList &text){
					quint64 propid = EnumInfo<CommentPropertyId>::findString(name);
					quint64 langid = EnumInfo<LanguageId>::findString(language);
					if(!propid)
						throw FrontendException("No such property for person");
					else if(!language.isNull() && !langid)
						throw FrontendException("No such language for person property");
					objed.uintPut(HistCommentObject::PropertyId, propid);
					objed.uintPut(HistCommentObject::LanguageId, langid);
					auto value = get(objed, true);
					auto mapto = objed.mapto();
					if(mapto == &textLine) {
						if(text.size() == 1)
							value->to<TextLine>()->text = text.at(0);
						else if(text.size() > 1)
							throw FrontendException("multiple lines of text for single-line text property for person");
					}
					else if(mapto == &textMultiline)
						value->to<TextMultiline>()->text = text;
					else
						throw FrontendException("(internal error) value type for person property not handled in spec-1.0.cpp");
				});
			}
		}
	}

	void DirFrontend::loadLocations()
	{
		for(size_t i = 0; i < EnumInfo<LocationType>::N; i++) {
			bool exmk = false;
			QDir dir = categoryDir(QStringList({ "location", EnumInfo<LocationType>::name(i) }), &exmk);
			if(!exmk)
				continue;
			QStringList entries = dir.entryList(QDir::Files);
			KeyEditor clsed(&locationClass);
			KeyEditor objed(&locationObject);
			clsed.enumPut(LocationClass::Type, EnumInfo<LocationType>::fromIndex(i));
			for(auto it : entries) {
				QFile file(dir.absoluteFilePath(it));
				if(!file.open(QFile::ReadOnly))
					throw FrontendException("Error opening location file");
				QString name = decodeFilename(it);
				quint64 objid = acquire(&objectId);
				clsed.stringPut(LocationClass::ObjectName, name);
				SyncFrontend::get<ObjectRef>(clsed, true)->id = objid;
				objed.uintPut(LocationObject::ObjectId, objid);
				parseProperties(&file, [&](const QString &name, const QString &language, const QStringList &text){
					quint64 propid = EnumInfo<LocationPropertyId>::findString(name);
					quint64 langid = EnumInfo<LanguageId>::findString(language);
					if(!propid)
						throw FrontendException("No such property for location");
					else if(!language.isNull() && !langid)
						throw FrontendException("No such language for location property");
					objed.uintPut(LocationObject::PropertyId, propid);
					objed.uintPut(LocationObject::LanguageId, langid);
					auto value = get(objed, true);
					auto mapto = objed.mapto();
					if(mapto == &textLine) {
						if(text.size() == 1)
							value->to<TextLine>()->text = text.at(0);
						else if(text.size() > 1)
							throw FrontendException("multiple lines of text for single-line text property for location");
					}
					else if(mapto == &textMultiline)
						value->to<TextMultiline>()->text = text;
					else
						throw FrontendException("(internal) value type for location property not handled in spec-1.0.cpp");
				});
			}
		}
	}

	void DirFrontend::loadBibliography()
	{
		for(size_t i = 0; i < EnumInfo<BibliographyType>::N; i++) {
			bool exmk = false;
			QDir dir = categoryDir(QStringList({ "bibliography", EnumInfo<BibliographyType>::name(i) }), &exmk);
			if(!exmk)
				continue;
			QStringList entries = dir.entryList(QDir::Files);
			KeyEditor clsed(&bibliographyClass);
			KeyEditor objed(&bibliographyObject);
			clsed.enumPut(BibliographyClass::Type, EnumInfo<BibliographyType>::fromIndex(i));
			for(auto it : entries) {
				QFile file(dir.absoluteFilePath(it));
				if(!file.open(QFile::ReadOnly))
					throw FrontendException("Error opening bibliography file");
				QString name = decodeFilename(it);
				quint64 objid = acquire(&objectId);
				clsed.stringPut(BibliographyClass::ObjectName, name);
				SyncFrontend::get<ObjectRef>(clsed, true)->id = objid;
				objed.uintPut(BibliographyObject::ObjectId, objid);
				parseProperties(&file, [&](const QString &name, const QString &language, const QStringList &text){
					quint64 propid = EnumInfo<BibliographyPropertyId>::findString(name);
					quint64 langid = EnumInfo<LanguageId>::findString(language);
					if(!propid)
						throw FrontendException("No such property for bibliography");
					else if(!language.isNull() && !langid)
						throw FrontendException("No such language for bibliography property");
					objed.uintPut(BibliographyObject::PropertyId, propid);
					objed.uintPut(BibliographyObject::LanguageId, langid);
					auto value = get(objed, true);
					auto mapto = objed.mapto();
					if(mapto == &textLine) {
						if(text.size() == 1)
							value->to<TextLine>()->text = text.at(0);
						else if(text.size() > 1)
							throw FrontendException("multiple lines of text for single-line text property for bibliography");
					}
					else if(mapto == &textMultiline)
						value->to<TextMultiline>()->text = text;
					else
						throw FrontendException("(internal) value type for bibliography property not handled in spec-1.0.cpp");
				});
			}
		}
	}

	void DirFrontend::loadLetters()
	{
		bool exmk = false;
		bool ok;
		QDir dir = categoryDir("letter", &exmk);
		if(!exmk)
			return;
		QStringList bookent = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
		KeyEditor booked(&textBook);
		KeyEditor lettered(&textLetter);
		KeyEditor contented(&textContent);
		KeyEditor metaed(&textMetadata);
		KeyEditor transed(&textTranslation);
		KeyEditor commented(&textComment);
		for(auto bit : bookent) {
			quint64 booknum = bit.toULongLong(&ok, 10);
			if(!ok)
				throw FrontendException("invalid book: directory is not a valid number");
			QDir bdir = dir;
			if(!bdir.cd(bit))
				throw FrontendException("error descending into book directory");

			quint64 bookid = acquire(&objectId);
			_id2number.insert(bookid, booknum);
			booked.uintPut(TextBook::ObjectNumber, booknum);
			lettered.uintPut(TextLetter::BookId, bookid);
			SyncFrontend::get<ObjectRef>(booked, true)->id = bookid;
			QStringList letterent = bdir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
			for(auto lit : letterent) {
				quint64 letnum = lit.toULongLong(&ok, 10);
				if(!ok)
					throw FrontendException("invalid letter: directory is not a valid number");
				quint64 letid = acquire(&objectId);
				_id2number.insert(letid, letnum);
				QDir ldir = bdir;
				if(!ldir.cd(lit))
					throw FrontendException("error descending into letter directory");
				lettered.uintPut(TextLetter::ObjectNumber, letnum);
				SyncFrontend::get<ObjectRef>(lettered, true)->id = letid;
				contented.uintPut(TextContent::LetterId, letid);
				transed.uintPut(TextTranslation::LetterId, letid);
				metaed.uintPut(TextMetadata::LetterId, letid);
				commented.uintPut(TextComment::LetterId, letid);
				
				{
					QFile file(ldir.absoluteFilePath("metadata"));
					if(file.open(QFile::ReadOnly)) {
						parseProperties(&file, [&](const QString &name, const QString &language, const QStringList &text){
							quint64 propid = EnumInfo<TextPropertyId>::findString(name);
							quint64 langid = EnumInfo<LanguageId>::findString(language);
							if(!propid)
								throw FrontendException("No such property for letter");
							else if(!language.isNull() && !langid)
								throw FrontendException("No such language for letter property");
							metaed.uintPut(TextMetadata::PropertyId, propid);
							metaed.uintPut(TextMetadata::LanguageId, langid);
							auto value = get(metaed, true);
							auto mapto = metaed.mapto();
							if(mapto == &textLine) {
								if(text.size() == 1)
									value->to<TextLine>()->text = text.at(0);
								else if(text.size() > 1)
									throw FrontendException("multiple lines of text for single-line text property for letter");
							}
							else if(mapto == &textMultiline)
								value->to<TextMultiline>()->text = text;
							else
								throw FrontendException("(internal error) value type for person property not handled in spec-1.0.cpp");
						});
					}
				}

				{
					QFile file(ldir.absoluteFilePath("translation"));
					if(file.open(QFile::ReadOnly)) {
						parseText(&file, [&](const QString &language, const QStringList &text){
							quint64 langid = EnumInfo<LanguageId>::findString(language);
							if(!language.isNull() && !langid)
								throw FrontendException("No such language for letter translation");
							transed.uintPut(TextTranslation::LanguageId, langid);
							auto value = get(transed, true);
							auto mapto = transed.mapto();
							if(mapto == &textLine) {
								if(text.size() == 1)
									value->to<TextLine>()->text = text.at(0);
								else if(text.size() > 1)
									throw FrontendException("multiple lines of text for single-line text translation for letter");
							}
							else if(mapto == &textMultiline)
								value->to<TextMultiline>()->text = text;
							else
								throw FrontendException("(internal error) value type for letter translation not handled in spec-1.0.cpp");
						});
					}
				}

				auto content = SyncFrontend::get<TextAnnotated>(contented, true);

				{
					QFile file(ldir.absoluteFilePath("content"));
					if(file.open(QFile::ReadOnly)) {
						QTextStream stream(&file);
						stream.setCodec("UTF-8");
						for(;;) {
							QString line = stream.readLine();
							if(line.isNull())
								break;
							content->text.append(line);
						}
					}
				}

				for(size_t i = 0; i < annotatedType.nEnumValues; i++) {
					QFile file(ldir.absoluteFilePath(annotatedType.enumValues[i].name));
					if(file.open(QFile::ReadOnly)) {
						parseMarkup(&file, [&](quint64 pbegin, quint64 wbegin, quint64 pend, quint64 wend, spec::v1_0::OptionPunc punc, const QMap<QString, QStringList> &text){
							decltype(content->comments)::value_type markup;
							markup.pbegin = pbegin;
							markup.wbegin = wbegin;
							markup.pend = pend;
							markup.wend = wend;
							markup.id = acquire(&objectId);
							markup.punc = (quint8)punc;
							markup.type = index2enum(i);
							content->comments.append(markup);
							for(auto it = text.begin(); it != text.end(); ++it) {
								quint64 langid = EnumInfo<LanguageId>::findString(it.key());
								if(!langid)
									throw FrontendException("No such language for person property");
								commented.uintPut(TextComment::ObjectId, markup.id);
								commented.uintPut(TextComment::LanguageId, langid);
								SyncFrontend::get<TextMultiline>(commented, true)->text = it.value();
							}
						});
					}
				}
				qSort(content->comments.begin(), content->comments.end(), commentLessThan);
			}
		}
		QStringList entries = dir.entryList(QDir::Files);
	}

	void DirFrontend::loadIntro()
	{
		bool exmk = false;
		QDir dir = categoryDir("intro", &exmk);
		if(!exmk)
			return;
		QStringList entries = dir.entryList(QDir::Files);
		KeyEditor clsed(&introClass);
		KeyEditor objed(&introObject);
		for(auto it : entries) {
			QFile file(dir.absoluteFilePath(it));
			if(!file.open(QFile::ReadOnly))
				throw FrontendException("Error opening intro file");
			QString name = decodeFilename(it);
			quint64 objid = acquire(&objectId);
			clsed.stringPut(IntroClass::ObjectName, name);
			SyncFrontend::get<ObjectRef>(clsed, true)->id = objid;
			objed.uintPut(IntroObject::ObjectId, objid);
			parseProperties(&file, [&](const QString &name, const QString &language, const QStringList &text){
				quint64 propid = EnumInfo<IntroPropertyId>::findString(name);
				quint64 langid = EnumInfo<LanguageId>::findString(language);
				if(!propid)
					throw FrontendException("No such property for intro");
				else if(!language.isNull() && !langid)
					throw FrontendException("No such language for intro property");
				objed.uintPut(IntroObject::PropertyId, propid);
				objed.uintPut(IntroObject::LanguageId, langid);
				auto value = get(objed, true);
				auto mapto = objed.mapto();
				if(mapto == &textLine) {
					if(text.size() == 1)
						value->to<TextLine>()->text = text.at(0);
					else if(text.size() > 1)
						throw FrontendException("multiple lines of text for single-line text property for intro");
				}
				else if(mapto == &textMultiline)
					value->to<TextMultiline>()->text = text;
				else
					throw FrontendException("(internal error) value type for intro property not handled in spec-1.0.cpp");
			});
		}
	}


	void DirFrontend::storePerson(const QString &name, quint64 id, bool create)
	{
		bool exmk = true;
		QDir dir = categoryDir("person", &exmk);
		QString filename = encodeFilename(name);
		QFile file(dir.absoluteFilePath(filename));
		if(!create && !file.exists())
			return;
		else if(!file.open(QFile::WriteOnly))
			throw FrontendException("Error opening object file");
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		KeyEditor ed(&meta);
		ed.select(&personObject);
		ed.uintPut(PersonObject::ObjectId, id);
		size_t l = lower(ed);
		size_t u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = enum2name(ed.enumGet<PersonPropertyId>(PersonObject::PropertyId));
			const DeclValue *mapto = ed.mapto();
			if(ed.uintGet(PersonObject::LanguageId)) {
				QString language;
				language = enum2name(ed.enumGet<LanguageId>(PersonObject::LanguageId));
				stream << name << " [" << language << "]:\n";
			}
			else
				stream << name << ":\n";

			if(mapto == &textLine) {
				auto v = SyncFrontend::value<TextLine>(l);
				stream << "\t" << v->text << "\n";
			}
			else if(mapto == &textMultiline) {
				auto v = SyncFrontend::value<TextMultiline>(l);
				for(auto it : v->text)
					stream << "\t" << it << "\n";
			}
			else if(mapto == &personSex) {
				auto v = SyncFrontend::value<PersonSex>(l);
				if(v->sex) {
					QString name = enum2name(EnumInfo<OptionSex>::fromInt(v->sex));
					stream << "\t" << name << "\n";
				}
			}
			else {
				Q_ASSERT(false);
				throw FrontendException("(internal) missing person property handling");
			}
		}
	}

	void DirFrontend::storeLocation(LocationType type, const QString &name, quint64 id, bool create)
	{
		bool exmk = true;
		QDir dir = categoryDir(QStringList({ "location", EnumInfo<LocationType>::name(type) }), &exmk);
		QString filename = encodeFilename(name);
		QFile file(dir.absoluteFilePath(filename));
		if(!create && !file.exists())
			return;
		else if(!file.open(QFile::WriteOnly))
			throw FrontendException("Error opening object file");
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		KeyEditor ed(&meta);
		ed.select(&locationObject);
		ed.uintPut(LocationObject::ObjectId, id);
		size_t l = lower(ed);
		size_t u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = enum2name(ed.enumGet<LocationPropertyId>(LocationObject::PropertyId));
			const DeclValue *mapto = ed.mapto();
			if(ed.uintGet(LocationObject::LanguageId)) {
				QString language;
				language = enum2name(ed.enumGet<LanguageId>(LocationObject::LanguageId));
				stream << name << " [" << language << "]:\n";
			}
			else
				stream << name << ":\n";

			if(mapto == &textLine) {
				auto v = SyncFrontend::value<TextLine>(l);
				stream << "\t" << v->text << "\n";
			}
			else if(mapto == &textMultiline) {
				auto v = SyncFrontend::value<TextMultiline>(l);
				for(auto it : v->text)
					stream << "\t" << it << "\n";
			}
			else {
				Q_ASSERT(false);
				throw FrontendException("(internal) missing location property handling");
			}
		}
	}

	void DirFrontend::storeBibliography(BibliographyType type, const QString &name, quint64 id, bool create)
	{
		bool exmk = true;
		QDir dir = categoryDir(QStringList({ "bibliography", EnumInfo<BibliographyType>::name(type) }), &exmk);
		QString filename = encodeFilename(name);
		QFile file(dir.absoluteFilePath(filename));
		if(!create && !file.exists())
			return;
		else if(!file.open(QFile::WriteOnly))
			throw FrontendException("Error opening object file");
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		KeyEditor ed(&meta);
		ed.select(&bibliographyObject);
		ed.uintPut(BibliographyObject::ObjectId, id);
		size_t l = lower(ed);
		size_t u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = enum2name(ed.enumGet<BibliographyPropertyId>(BibliographyObject::PropertyId));
			const DeclValue *mapto = ed.mapto();
			if(ed.uintGet(BibliographyObject::LanguageId)) {
				QString language;
				language = enum2name(ed.enumGet<LanguageId>(BibliographyObject::LanguageId));
				stream << name << " [" << language << "]:\n";
			}
			else
				stream << name << ":\n";

			if(mapto == &textLine) {
				auto v = SyncFrontend::value<TextLine>(l);
				stream << "\t" << v->text << "\n";
			}
			else if(mapto == &textMultiline) {
				auto v = SyncFrontend::value<TextMultiline>(l);
				for(auto it : v->text)
					stream << "\t" << it << "\n";
			}
			else {
				Q_ASSERT(false);
				throw FrontendException("(internal) missing bibliography property handling");
			}
		}
	}

	void DirFrontend::storePhilComment(const QString &name, quint64 id, bool create)
	{
		bool exmk = true;
		QDir dir = categoryDir("phil-comment", &exmk);
		QString filename = encodeFilename(name);
		QFile file(dir.absoluteFilePath(filename));
		if(!create && !file.exists())
			return;
		else if(!file.open(QFile::WriteOnly))
			throw FrontendException("Error opening object file");
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		KeyEditor ed(&meta);
		ed.select(&philCommentObject);
		ed.uintPut(PhilCommentObject::ObjectId, id);
		size_t l = lower(ed);
		size_t u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = enum2name(ed.enumGet<CommentPropertyId>(PhilCommentObject::PropertyId));
			const DeclValue *mapto = ed.mapto();
			if(ed.uintGet(PhilCommentObject::LanguageId)) {
				QString language;
				language = enum2name(ed.enumGet<LanguageId>(PhilCommentObject::LanguageId));
				stream << name << " [" << language << "]:\n";
			}
			else
				stream << name << ":\n";

			if(mapto == &textLine) {
				auto v = SyncFrontend::value<TextLine>(l);
				stream << "\t" << v->text << "\n";
			}
			else if(mapto == &textMultiline) {
				auto v = SyncFrontend::value<TextMultiline>(l);
				for(auto it : v->text)
					stream << "\t" << it << "\n";
			}
			else {
				Q_ASSERT(false);
				throw FrontendException("(internal) missing phil comment property handling");
			}
		}
	}

	void DirFrontend::storeHistComment(HistCommentType type, const QString &name, quint64 id, bool create)
	{
		bool exmk = true;
		QDir dir = categoryDir(QStringList({ "hist-comment", EnumInfo<HistCommentType>::name(type) }), &exmk);
		QString filename = encodeFilename(name);
		QFile file(dir.absoluteFilePath(filename));
		if(!create && !file.exists())
			return;
		else if(!file.open(QFile::WriteOnly))
			throw FrontendException("Error opening object file");
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		KeyEditor ed(&meta);
		ed.select(&histCommentObject);
		ed.uintPut(HistCommentObject::ObjectId, id);
		size_t l = lower(ed);
		size_t u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = enum2name(ed.enumGet<CommentPropertyId>(HistCommentObject::PropertyId));
			const DeclValue *mapto = ed.mapto();
			if(ed.uintGet(HistCommentObject::LanguageId)) {
				QString language;
				language = enum2name(ed.enumGet<LanguageId>(HistCommentObject::LanguageId));
				stream << name << " [" << language << "]:\n";
			}
			else
				stream << name << ":\n";

			if(mapto == &textLine) {
				auto v = SyncFrontend::value<TextLine>(l);
				stream << "\t" << v->text << "\n";
			}
			else if(mapto == &textMultiline) {
				auto v = SyncFrontend::value<TextMultiline>(l);
				for(auto it : v->text)
					stream << "\t" << it << "\n";
			}
			else {
				Q_ASSERT(false);
				throw FrontendException("(internal) missing hist comment property handling");
			}
		}
	}

	void DirFrontend::storeLetter(quint64 book, quint64 letter, quint64 id, bool create)
	{
		bool exmk = true;
		QDir dir = letterDir(book, letter, &exmk);

		KeyEditor ed(&meta);
		ed.select(&textContent);
		ed.uintPut(TextContent::LetterId, id);
		auto content = SyncFrontend::get<TextAnnotated>(ed);
		if(!content)
			return;

		qSort(content->comments.begin(), content->comments.end(), commentLessThan);

		{
			QFile file(dir.absoluteFilePath("content"));
			if(file.exists() || create) {
				if(!file.open(QFile::WriteOnly))
					throw FrontendException("Error opening letter content file");
				QTextStream stream(&file);
				stream.setCodec("UTF-8");
				for(auto it : content->text)
					stream << it << "\n";
			}
		}

		{
			QFile file(dir.absoluteFilePath("metadata"));
			if(file.exists() || create) {
				if(!file.open(QFile::WriteOnly))
					throw FrontendException("Error opening letter metadata file");
				QTextStream stream(&file);
				stream.setCodec("UTF-8");
				ed.select(&textMetadata);
				ed.uintPut(TextMetadata::LetterId, id);
				size_t l = lower(ed);
				size_t u = prefixUpper(ed);
				for(; l < u; l++) {
					ed.load(key(l));
					QString name = enum2name(ed.enumGet<TextPropertyId>(TextMetadata::PropertyId));
					const DeclValue *mapto = ed.mapto();
					if(ed.uintGet(TextMetadata::LanguageId)) {
						QString language;
						language = enum2name(ed.enumGet<LanguageId>(TextMetadata::LanguageId));
						stream << name << " [" << language << "]:\n";
					}
					else
						stream << name << ":\n";

					if(mapto == &textLine) {
						auto v = SyncFrontend::value<TextLine>(l);
						stream << "\t" << v->text << "\n";
					}
					else if(mapto == &textMultiline) {
						auto v = SyncFrontend::value<TextMultiline>(l);
						for(auto it : v->text)
							stream << "\t" << it << "\n";
					}
					else {
						Q_ASSERT(false);
						throw FrontendException("(internal) missing letter property handling");
					}
				}
			}
		}

		{
			QFile file(dir.absoluteFilePath("translation"));
			if(file.exists() || create) {
				if(!file.open(QFile::WriteOnly))
					throw FrontendException("Error opening letter translation file");
				QTextStream stream(&file);
				stream.setCodec("UTF-8");
				ed.select(&textTranslation);
				ed.uintPut(TextTranslation::LetterId, id);
				size_t l = lower(ed);
				size_t u = prefixUpper(ed);
				for(; l < u; l++) {
					ed.load(key(l));
					const DeclValue *mapto = ed.mapto();
					if(ed.uintGet(TextTranslation::LanguageId)) {
						QString language;
						language = enum2name(ed.enumGet<LanguageId>(TextTranslation::LanguageId));
						stream << "[" << language << "]:\n";
					}
					else
						throw FrontendException("Invalid or missing language in translation");

					if(mapto == &textLine) {
						auto v = SyncFrontend::value<TextLine>(l);
						stream << "\t" << v->text << "\n";
					}
					else if(mapto == &textMultiline) {
						auto v = SyncFrontend::value<TextMultiline>(l);
						for(auto it : v->text)
							stream << "\t" << it << "\n";
					}
					else {
						Q_ASSERT(false);
						throw FrontendException("(internal) missing letter translation handling");
					}
				}
			}
		}

		KeyEditor ced(&meta);
		for(size_t i = 0; i < annotatedType.nEnumValues; i++) {
			QFile file(dir.absoluteFilePath(annotatedType.enumValues[i].name));
			if(file.exists() || create) {
				if(!file.open(QFile::WriteOnly))
					throw FrontendException("Error opening letter comment file");
				QTextStream stream(&file);
				stream.setCodec("UTF-8");
				for(auto it : content->comments) {
					if(EnumInfo<AnnotatedType>::fromInt(it.type) == EnumInfo<AnnotatedType>::fromIndex(i)) {
						ced.select(&textComment);
						ced.uintPut(TextComment::LetterId, id);
						//ced.truncate(TextComment::LanguageId);
						ced.uintPut(TextComment::ObjectId, it.id);
						size_t cl = lower(ced);
						size_t cu = prefixUpper(ced);
						if(it.pbegin == it.pend && it.wbegin == it.wend)
							stream << "@p" << it.pbegin << "w" << it.wbegin;
						else
							stream << "@p" << it.pbegin << "w" << it.wbegin << "-p" << it.pend << "w" << it.wend;
						if(it.punc == (quint8)spec::v1_0::OptionPunc::Include)
							stream << "|";
						else if(it.punc == (quint8)spec::v1_0::OptionPunc::Exclude)
							stream << "|.";
						stream << ":\n";
						for(; cl < cu; cl++) {
							ced.load(key(cl));
							QString language = enum2name(ced.enumGet<LanguageId>(TextComment::LanguageId));
							stream << "\t[" << language << "]\n";
							auto comment = SyncFrontend::value<TextMultiline>(cl);
							for(auto it : comment->text)
								stream << "\t\t" << it << "\n";
						}
					}
				}
			}
		}
	}

	void DirFrontend::storeIntro(const QString &name, quint64 id, bool create)
	{
		bool exmk = true;
		QDir dir = categoryDir("intro", &exmk);
		QString filename = encodeFilename(name);
		QFile file(dir.absoluteFilePath(filename));
		if(!create && !file.exists())
			return;
		else if(!file.open(QFile::WriteOnly))
			throw FrontendException("Error opening object file");
		QTextStream stream(&file);
		stream.setCodec("UTF-8");
		KeyEditor ed(&meta);
		ed.select(&introObject);
		ed.uintPut(IntroObject::ObjectId, id);
		size_t l = lower(ed);
		size_t u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = enum2name(ed.enumGet<IntroPropertyId>(IntroObject::PropertyId));
			const DeclValue *mapto = ed.mapto();
			if(ed.uintGet(IntroObject::LanguageId)) {
				QString language;
				language = enum2name(ed.enumGet<LanguageId>(IntroObject::LanguageId));
				stream << name << " [" << language << "]:\n";
			}
			else
				stream << name << ":\n";

			if(mapto == &textLine) {
				auto v = SyncFrontend::value<TextLine>(l);
				stream << "\t" << v->text << "\n";
			}
			else if(mapto == &textMultiline) {
				auto v = SyncFrontend::value<TextMultiline>(l);
				for(auto it : v->text)
					stream << "\t" << it << "\n";
			}
			else {
				Q_ASSERT(false);
				throw FrontendException("(internal) missing intro property handling");
			}
		}
	}

	void DirFrontend::actionLoad()
	{
		loadPersons();
		loadLocations();
		loadBibliography();
		loadLetters();
		loadPhilComments();
		loadHistComments();
		loadIntro();
	}

	void DirFrontend::actionSave()
	{
		size_t l;
		size_t u;

		KeyEditor ed(&meta);
		ed.select(&personClass);
		l = lower(ed);
		u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = ed.stringGet(PersonClass::ObjectName);
			quint64 id = SyncFrontend::value<ObjectRef>(l)->id;
			storePerson(name, id);
		}

		ed.select(&locationClass);
		l = lower(ed);
		u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = ed.stringGet(LocationClass::ObjectName);
			LocationType type = ed.enumGet<LocationType>(LocationClass::Type);
			quint64 id = SyncFrontend::value<ObjectRef>(l)->id;
			storeLocation(type, name, id);
		}

		ed.select(&bibliographyClass);
		l = lower(ed);
		u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = ed.stringGet(BibliographyClass::ObjectName);
			BibliographyType type = ed.enumGet<BibliographyType>(BibliographyClass::Type);
			quint64 id = SyncFrontend::value<ObjectRef>(l)->id;
			storeBibliography(type, name, id);
		}

		ed.select(&philCommentClass);
		l = lower(ed);
		u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = ed.stringGet(PhilCommentClass::ObjectName);
			quint64 id = SyncFrontend::value<ObjectRef>(l)->id;
			storePhilComment(name, id);
		}

		ed.select(&histCommentClass);
		l = lower(ed);
		u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = ed.stringGet(HistCommentClass::ObjectName);
			HistCommentType type = ed.enumGet<HistCommentType>(HistCommentClass::Type);
			quint64 id = SyncFrontend::value<ObjectRef>(l)->id;
			storeHistComment(type, name, id);
		}

		ed.select(&textBook);
		l = lower(ed);
		u = prefixUpper(ed);
		for(; l < u; l++) {
			KeyEditor led(&meta);
			led.select(&textLetter);
			ed.load(key(l));
			quint64 booknum = ed.uintGet(TextBook::ObjectNumber);
			quint64 bookid = SyncFrontend::value<ObjectRef>(l)->id;
			led.uintPut(TextLetter::BookId, bookid);
			size_t bl = lower(led);
			size_t bu = prefixUpper(led);
			for(; bl < bu; bl++) {
				led.load(key(bl));
				quint64 letnum = led.uintGet(TextLetter::ObjectNumber);
				quint64 letid = SyncFrontend::value<ObjectRef>(bl)->id;
				storeLetter(booknum, letnum, letid);
			}
		}

		ed.select(&introClass);
		l = lower(ed);
		u = prefixUpper(ed);
		for(; l < u; l++) {
			ed.load(key(l));
			QString name = ed.stringGet(IntroClass::ObjectName);
			quint64 id = SyncFrontend::value<ObjectRef>(l)->id;
			storeIntro(name, id);
		}
	}

	void DirFrontend::actionErase(const QByteArray &key, Value *value)
	{
		KeyEditor keyed(&meta, key);
		bool exmk = false;

		if(keyed.decl() == &personClass) {
			QDir dir = categoryDir("person", &exmk);
			QString name = keyed.stringGet(PersonClass::ObjectName);
			if(exmk)
				QFile(dir.absoluteFilePath(encodeFilename(name))).remove();
		}
		else if(keyed.decl() == &locationClass) {
			auto type = keyed.enumGet<LocationType>(LocationClass::Type);
			QDir dir = categoryDir(QStringList({ "location", EnumInfo<LocationType>::name(type) }), &exmk);
			QString name = keyed.stringGet(LocationClass::ObjectName);
			if(exmk)
				QFile(dir.absoluteFilePath(encodeFilename(name))).remove();
		}
		else if(keyed.decl() == &bibliographyClass) {
			auto type = keyed.enumGet<BibliographyType>(BibliographyClass::Type);
			QDir dir = categoryDir(QStringList({ "bibliography", EnumInfo<BibliographyType>::name(type) }), &exmk);
			QString name = keyed.stringGet(BibliographyClass::ObjectName);
			if(exmk)
				QFile(dir.absoluteFilePath(encodeFilename(name))).remove();
		}
		else if(keyed.decl() == &philCommentClass) {
			QDir dir = categoryDir("phil-comment", &exmk);
			QString name = keyed.stringGet(PhilCommentClass::ObjectName);
			if(exmk)
				QFile(dir.absoluteFilePath(encodeFilename(name))).remove();
		}
		else if(keyed.decl() == &histCommentClass) {
			HistCommentType type = keyed.enumGet<HistCommentType>(HistCommentClass::Type);
			QDir dir = categoryDir(QStringList({ "hist-comment", EnumInfo<HistCommentType>::name(type) }), &exmk);
			QString name = keyed.stringGet(HistCommentClass::ObjectName);
			if(exmk)
				QFile(dir.absoluteFilePath(encodeFilename(name))).remove();
		}
		else if(keyed.decl() == &textBook) {
		}
		else if(keyed.decl() == &textLetter) {
			quint64 bookid = keyed.uintGet(TextLetter::BookId);
			quint64 letid = value->to<ObjectRef>()->id;
			quint64 letnum = _id2number.value(letid);
			quint64 booknum = _id2number.value(bookid);
			_id2number.remove(letid);
			QDir dir = letterDir(booknum, letnum, &exmk);
			if(exmk) {
				QString dirname = dir.dirName();
				QFile(dir.absoluteFilePath("content")).remove();
				QFile(dir.absoluteFilePath("metadata")).remove();
				QFile(dir.absoluteFilePath("translation")).remove();
				for(size_t i = 0; i < annotatedType.nEnumValues; i++)
					QFile(dir.absoluteFilePath(annotatedType.enumValues[i].name)).remove();
				if(dir.cdUp())
					dir.rmdir(dirname);
			}
		}
		else if(keyed.decl() == &introClass) {
			QDir dir = categoryDir("intro", &exmk);
			QString name = keyed.stringGet(IntroClass::ObjectName);
			if(exmk)
				QFile(dir.absoluteFilePath(encodeFilename(name))).remove();
		}
	}

	void DirFrontend::actionModify(const QByteArray &key, Value *value)
	{
		KeyEditor ed(&meta, key);
		if(ed.decl() == &personClass) {
			QString name = ed.stringGet(PersonClass::ObjectName);
			auto v = value->to<ObjectRef>();
			if(v->id)
				storePerson(name, v->id);
			else
				throw 1;
		}
		else if(ed.decl() == &locationClass) {
			LocationType type = ed.enumGet<LocationType>(LocationClass::Type);
			QString name = ed.stringGet(LocationClass::ObjectName);
			auto v = value->to<ObjectRef>();
			if(v->id)
				storeLocation(type, name, v->id);
			else
				throw 1;
		}
		else if(ed.decl() == &bibliographyClass) {
			BibliographyType type = ed.enumGet<BibliographyType>(BibliographyClass::Type);
			QString name = ed.stringGet(BibliographyClass::ObjectName);
			auto v = value->to<ObjectRef>();
			if(v->id)
				storeBibliography(type, name, v->id);
			else
				throw 1;
		}
		else if(ed.decl() == &philCommentClass) {
			QString name = ed.stringGet(PhilCommentClass::ObjectName);
			auto v = value->to<ObjectRef>();
			if(v->id)
				storePhilComment(name, v->id);
			else
				throw 1;
		}
		else if(ed.decl() == &histCommentClass) {
			HistCommentType type = ed.enumGet<HistCommentType>(HistCommentClass::Type);
			QString name = ed.stringGet(HistCommentClass::ObjectName);
			auto v = value->to<ObjectRef>();
			if(v->id)
				storeHistComment(type, name, v->id);
			else
				throw 1;
		}
		else if(ed.decl() == &textBook) {
			quint64 id = value->to<ObjectRef>()->id;
			quint64 num = ed.uintGet(TextBook::ObjectNumber);
			if(id)
				_id2number.insert(id, num);
			else
				throw 1;
		}
		else if(ed.decl() == &textLetter) {
			quint64 letnum = ed.uintGet(TextLetter::ObjectNumber);
			quint64 letid = value->to<ObjectRef>()->id;
			quint64 bookid = ed.uintGet(TextLetter::BookId);
			quint64 booknum = _id2number.value(bookid);
			if(letid && bookid) {
				storeLetter(booknum, letnum, letid);
				_id2number.insert(letid, letnum);
			}
			else
				throw 1;
		}
		else if(ed.decl() == &introClass) {
			QString name = ed.stringGet(IntroClass::ObjectName);
			auto v = value->to<ObjectRef>();
			if(v->id)
				storeIntro(name, v->id);
			else
				throw 1;
		}
	}
}}

