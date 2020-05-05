#include <QFile>
#include <QXmlStreamWriter>

#include "common/qXml.h"

#include "key.h"
#include "frontend.h"

#include "xml.h"

using namespace spec;

namespace { namespace xmlold {
#include "xmlold.hpp"
}}

namespace {
	struct ValueFrame : XmlModelHandlerFrame {
		ValueFrame(const DeclValue *decl, const DeclVar *vars, size_t nvar, const ValueAccessor *accessor, void *handle) : decl(decl), vars(vars), nvar(nvar), accessor(accessor), handle(handle) {}

		const DeclValue *decl;
		const DeclVar *vars;
		size_t nvar;
		const ValueAccessor *accessor;
		void *handle;

		QMap<const DeclVar*, size_t> n;
	};

#if 0
	struct Element : XmlModelHandlerFrame {
		QMap<Name, size_t> ncell;
	};
#endif

	class Handler : public XmlModelHandler {
		public:
			Handler(SyncFrontend *fe) : _fe(fe), _spec(nullptr) {
				int root = add(cb(this, &Handler::start));
				int key = add(cb(this, &Handler::keyStart));
				int value = add(cb(this, &Handler::valueStart));

				link(0, "annotate-db", root);
				link(root, key);
				link(key, value);
				link(value, value);
			}

		private:
			void start(const QXmlAttributes &attr) {
				QString attrSpec = attr.value("spec");
				QString attrXml = attr.value("xml");
				if(attrSpec.isEmpty())
					delegate(new xmlold::Handler(_fe));
				else if(attrXml != "1.0")
					throw XmlModelException("cannot load xml: unimplemented xml version");
				else {
					_spec = DeclMeta::version(attrSpec);
					if(!_spec)
						throw XmlModelException("invalid spec version");
					else if(_spec != _fe->spec())
						throw XmlModelException("spec version does not match frontend version");
				}
			}

			void keyStart(XmlModelHandlerFrame *&frame, const QString &name, const QXmlAttributes &attr) {
				const DeclKey *key = _spec->findKey(name);
				if(!key)
					throw XmlModelException("no such key");
				KeyEditor keyed(key);
				for(int i = 0; i < attr.length(); i++) {
					const DeclField *field = key->findField(attr.qName(i));
					if(!field)
						throw XmlModelException("no such field for key");
					if(field->type->dynamicId) {
						QString xmlid = attr.value(i);
						quint64 dbid = _idmap.value(xmlid);
						if(!dbid) {
							dbid = _fe->acquire(field->type);
							_idmap.insert(xmlid, dbid);
						}
						keyed.uintPutAt(field->index, dbid);
					}
					else
						keyed.fromstringAt(field->index, attr.value(i));
				}
				const DeclValue *mapto = keyed.mapto();
				if(!mapto)
					throw XmlModelException("invalid key: does not map to a value");
				Value *value = _fe->get(keyed, true);
				if(value->decl() != mapto)
					throw XmlModelException("internal error: keyed.mapto() doesn't match returned value in qtfe/xml.cpp");
				frame = new ValueFrame(mapto, mapto->vars, mapto->countVar(0), mapto->accessor, value);
			}

			void valueStart(XmlModelHandlerFrame *&frame, const QString &namestr, const QXmlAttributes &attr) {
				ValueFrame *value = frame->topmost<ValueFrame>();
				Name name = string2name(namestr);
				QString attrString = attr.value("string");
				QString attrEnum = attr.value("enum");
				QString attrUint = attr.value("uint");
				QString attrSint = attr.value("sint");
				QString attrId = attr.value("id");
				if(name == Name::null)
					throw XmlModelException("no such name");
//				printf("VALUE: %s %zu\n", qPrintable(name2string(value->decl->name)), value->vars - value->decl->vars);
				const DeclVar *child = value->decl->findVar(value->vars - value->decl->vars, name);
				if(!child)
					throw XmlModelException(QString("no such value entry '%1'").arg(name2string(name)));
				size_t index = value->n[child]++;
				if(child->n) {
					if(index >= child->n)
						throw XmlModelException("too many values for field");
				}
				else {
					if(!value->accessor->append(value->handle, child - value->vars))
						throw XmlModelException("error adding field to value");
				}
				void *handle = value->accessor->child(value->handle, child - value->vars, index);
				if(child->child) {
					frame = new ValueFrame(value->decl, value->decl->vars + child->child, value->decl->countVar(child->child), child->u.accessor, handle);
				}
				else {
					const DeclType *type = child->u.type;
					if(type->isString()) {
						if(attrString.isNull())
							throw XmlModelException("missing 'string' attribute for string entry");
						type->stringPut(handle, attrString);
					}
					else if(type->dynamicId) {
						if(attrId.isEmpty())
							throw XmlModelException("missing 'id' attribute for id entry");
						quint64 dbid = _idmap.value(attrId);
						if(!dbid) {
							dbid = _fe->acquire(type);
							_idmap.insert(attrId, dbid);
						}
						type->uintPut(handle, dbid);
					}
					else if(type->isEnum()) {
						if(attrEnum.isEmpty())
							throw XmlModelException("missin 'enum' attribute for enum entry");
						//TODO unify this with fromstring in DeclType and key.cpp
						QByteArray utf8 = attrEnum.toUtf8();
						ssize_t enumidx = type->nameIndexOf(utf8.constData());
						if(enumidx < 0)
							type->uintPut(handle, 0);
						else
							type->uintPut(handle, enumidx + 1);
					}
					else if(type->isUnsigned()) {
						if(attrUint.isEmpty())
							throw XmlModelException("missing 'uint' attribute for uint entry");
						quint64 value = attrUint.toULongLong();
						//TODO check, if value is ok
						type->uintPut(handle, value);
					}
					else if(type->isSigned()) {
						if(attrSint.isEmpty())
							throw XmlModelException("missing 'sint' attribute for uint entry");
						qint64 value = attrSint.toLongLong();
						//TODO check, if value is ok
						type->sintPut(handle, value);
					}
				}
			}

			SyncFrontend *_fe;
			const DeclMeta *_spec;

			QMap<QString, quint64> _idmap;
	};
}

XmlFormat::XmlFormat(SyncFrontend *fe)
	:	_fe(fe)
{
}

void XmlFormat::load(const QString &filename)
{
	XmlModelLoader loader;
	if(!loader.parse(filename, new Handler(_fe)))
		throw XmlModelException("Error parsing xml file");
}

void XmlFormat::save(const QString &filename)
{
	Value::ConstWalker walker;
	QStack<Name> names;

	QFile file(filename);
	if(!file.open(QFile::ReadWrite | QFile::Truncate))
		throw XmlModelException("error opening xml output file");
	QByteArray prefix;
	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);

	writer.writeStartElement("annotate-db");
	writer.writeAttribute("xml", "1.0");
	writer.writeAttribute("spec", _fe->spec()->name);

	walker.down = [&](Name name, size_t n) {
		names.push(name);
	};

	walker.up = [&]() {
		names.pop();
	};

	walker.begin = [&](size_t index, const ValueAccessor *accessor, const void *handle) {
		if(names.size() > 1) //don't write value type
			writer.writeStartElement(name2string(names.top()));
	};

	walker.end = [&]() {
		if(names.size() > 1) //don't write value type
			writer.writeEndElement();
	};

	walker.var = [&](size_t index, const DeclType *type, const void *handle) {
		writer.writeStartElement(name2string(names.top()));
		if(type->isString())
			writer.writeAttribute("string", type->tostring(handle));
		else if(type->dynamicId)
			writer.writeAttribute("id", QString("%1:%2").arg(type->fullname).arg(type->tostring(handle)));
		else if(type->isEnum())
			writer.writeAttribute("enum", type->tostring(handle));
		else if(type->isSigned())
			writer.writeAttribute("sint", type->tostring(handle));
		else if(type->isUnsigned())
			writer.writeAttribute("uint", type->tostring(handle));
		else
			throw XmlModelException("cannot write value: missing implementation");
		writer.writeEndElement();
	};


	_fe->foreachKV([&](const QByteArray &key, const Value *value) {
		KeyEditor keyed(_fe->spec(), key);
		if(!keyed.isValidKey())
			throw XmlModelException("cannot save xml: database contains invalid keys");
		writer.writeStartElement(keyed.name());

		for(size_t i = 0; i < keyed.fields(); i++)
			if(!keyed.isKeyFieldAt(i)) {
				if(keyed.typeAt(i)->dynamicId)
					writer.writeAttribute(keyed.fieldNameAt(i), QString("%1:%2").arg(keyed.typeAt(i)->fullname).arg(keyed.tostringAt(i)));
				else
					writer.writeAttribute(keyed.fieldNameAt(i), keyed.tostringAt(i));
			}

		value->constWalk(walker);

		writer.writeEndElement();
		return true;
	});

	writer.writeEndElement();
}

