#include <QtEndian>

#include "editor.h"

using namespace spec;

void Editor::print(const QByteArray &data, size_t offset, const spec::DeclType *type)
{
	if(type->builtin == BuiltinU8 || type->builtin == BuiltinU16 || type->builtin == BuiltinU32 || type->builtin == BuiltinU64) {
		quint64 value = getint(data, offset, type);
		printf("%llu (0x%llx)", value, value);
		if(type->nEnumValues > 0) {
			if(!value)
				printf(" (enum: null)");
			else if(value > type->nEnumValues)
				printf(" (enum: out of range");
			else
				printf(" (enum=%s)", type->enumValues[value - 1].name);
		}
	}
	else if(type->builtin == BuiltinFlexString)
		printf("'%s'", qPrintable(getstr(data, offset, type)));
	else
		printf("(unknown type)");
}

void Editor::putint(size_t offset, const DeclType *type, quint64 value)
{
	resizeUp(offset + type->size);
	if(type->builtin == BuiltinU8) {
		quint8 v = value;
		Q_ASSERT(sizeof(v) == type->size);
		_data.data()[offset] = v;
	}
	else if(type->builtin == BuiltinU16) {
		quint16 v = value;
		Q_ASSERT(sizeof(v) == type->size);
		v = qToBigEndian(v);
		memcpy(_data.data() + offset, &v, sizeof(v));
	}
	else if(type->builtin == BuiltinU32) {
		quint32 v = value;
		Q_ASSERT(sizeof(v) == type->size);
		v = qToBigEndian(v);
		memcpy(_data.data() + offset, &v, sizeof(v));
	}
	else if(type->builtin == BuiltinU64) {
		quint64 v = value;
		Q_ASSERT(sizeof(v) == type->size);
		v = qToBigEndian(v);
		memcpy(_data.data() + offset, &v, sizeof(v));
	}
	else
		throw Exception("cannot set integer value on non-integer field");
}

void Editor::putstr(size_t offset, const DeclType *type, const QString &value)
{
	QByteArray utf8 = value.toUtf8();
	if(type->builtin == BuiltinFlexString) {
		resize(offset);
		_data.append(utf8);
	}
	else
		throw Exception("cannot set string value on non-string field");
}

void Editor::putnull(size_t offset, const DeclType *type)
{
	if(type->builtin == BuiltinU8 || type->builtin == BuiltinU16 || type->builtin == BuiltinU32 || type->builtin == BuiltinU64)
		putint(offset, type, 0);
	else if(type->builtin == BuiltinFlexString)
		putstr(offset, type, QString());
	else
		throw Exception("cannot set clear value for unknown field type");
}

quint64 Editor::getint(const QByteArray &data, size_t offset, const DeclType *type)
{
	if(offset + type->size > size_t(data.size()))
		throw Exception("cannot get integer value: key too small");
	if(type->builtin == BuiltinU8) {
		quint8 v;
		Q_ASSERT(sizeof(v) == type->size);
		v = data.constData()[offset];
		return v;
	}
	else if(type->builtin == BuiltinU16) {
		quint16 v;
		Q_ASSERT(sizeof(v) == type->size);
		memcpy(&v, data.data() + offset, sizeof(v));
		return qFromBigEndian(v);
	}
	else if(type->builtin == BuiltinU32) {
		quint32 v;
		Q_ASSERT(sizeof(v) == type->size);
		memcpy(&v, data.data() + offset, sizeof(v));
		return qFromBigEndian(v);
	}
	else if(type->builtin == BuiltinU64) {
		quint64 v;
		Q_ASSERT(sizeof(v) == type->size);
		memcpy(&v, data.data() + offset, sizeof(v));
		return qFromBigEndian(v);
	}
	else
		throw Exception("cannot get integer value from non-integer field");
}

QString Editor::getstr(const QByteArray &data, size_t offset, const DeclType *type)
{
	if(offset > size_t(data.size()))
		throw Exception("cannot get string value: key too small");
	if(type->builtin == BuiltinFlexString)
		return QString::fromUtf8(data.mid(offset));
	else
		throw Exception("cannot get string value on non-string field");
}

void Editor::resize(size_t size)
{
	size_t old = _data.size();
	_data.resize(size);
	if(old < size)
		memset(_data.data() + old, 0, size - old);
}

void Editor::resizeUp(size_t size)
{
	size_t old = _data.size();
	if(old < size) {
		_data.resize(size);
		memset(_data.data() + old, 0, size - old);
	}
}

void Editor::resizeDown(size_t size)
{
	size_t old = _data.size();
	if(old > size)
		_data.resize(size);
}

