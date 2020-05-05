#include <QtEndian>

#include "common/qHexdump.h"
#include "key.h"

using namespace spec;

KeyEditor::KeyEditor(const DeclMeta *meta, const QByteArray &key)
	:	_meta(meta),
		_decl(nullptr),
		_fixed(0),
		_set(0)
{
	pasteAt(0, key);
}

KeyEditor::KeyEditor(const DeclKey *decl)
	:	_meta(nullptr),
		_decl(nullptr),
		_fixed(0),
		_set(0)
{
	select(decl);
}

void KeyEditor::load(const QByteArray &key)
{
	pasteAt(0, key);
}

QString KeyEditor::name() const
{
	if(!_decl)
		return QString();
	return _decl->fullname;
}

size_t KeyEditor::fixed() const
{
	return _fixed;
}

size_t KeyEditor::size() const
{
	return _data.size();
}

size_t KeyEditor::fields() const
{
	return _set;
}

const void *KeyEditor::data() const
{
	return _data.constData();
}

bool KeyEditor::isValid(bool allowNull) const
{
	if(!_decl)
		return true;
	for(size_t i = 0; i < _set; i++) {
		const DeclField *cur = _decl->fields + i;
		if(cur->type->nEnumValues > 0) {
			quint64 value = getint(cur->offset, cur->type);
			if(!value && !allowNull)
				return false;
			else if(value > cur->type->nEnumValues)
				return false;
		}
	}
	return true;
}

bool KeyEditor::hasMissing() const
{
	if(!_decl)
		return false;
	for(size_t i = 0; i < _set; i++) {
		const DeclField *cur = _decl->fields + i;
		if(cur->type->nEnumValues > 0 && !getint(cur->offset, cur->type))
			return true;
	}
	return false;
}

bool KeyEditor::isKey() const
{
	return !isAbstract() && isFull();
}

bool KeyEditor::isValidKey() const
{
	return isKey() && isValid(true);
}

bool KeyEditor::isAbstract() const
{
	return !_decl || _decl->abstract;
}

bool KeyEditor::isFull() const
{
	return !_decl || _decl->nFields == _set;
}

bool KeyEditor::isNull() const
{
	return !_decl;
}

KeyEditor::operator QByteArray() const
{
	return _data;
}

const DeclValue *KeyEditor::mapto() const
{
	if(!_decl)
		return nullptr;
	for(size_t i = _set; i > 0; i--) {
		const DeclField *field = _decl->fields + i - 1;
		const DeclType *type = field->type;
		if(type->nEnumValues > 0) {
			quint64 v = getint(field->offset, type);
			if(v > 0 && v <= type->nEnumValues) {
				if(type->enumValues[v - 1].mapto)
					return type->enumValues[v - 1].mapto;
			}
			if(type->mapto)
				return type->mapto;
		}
		if(!i)
			break;
	}
	return _decl->mapto;
}

const spec::DeclKey *KeyEditor::decl() const
{
	return _decl;
}

bool KeyEditor::isKeyFieldAt(size_t field) const
{
	return _decl && field < _decl->nFields && _decl->fields[field].key;
}

bool KeyEditor::isIdFieldAt(size_t field) const
{
	return _decl && field < _decl->nFields && _decl->fields[field].type->dynamicId;
}

const spec::DeclType *KeyEditor::typeAt(size_t field) const
{
	if(!_decl || field >= _decl->nFields)
		throw KeyException("cannot get key fragment offset: no such field");
	return _decl->fields[field].type;
}

size_t KeyEditor::offsetAt(size_t field) const
{
	if(!_decl || field >= _decl->nFields)
		throw KeyException("cannot get key fragment offset: no such field");
	return _decl->fields[field].offset;
}

size_t KeyEditor::sizeAt(size_t field) const
{
	if(!_decl || field >= _decl->nFields)
		throw KeyException("cannot get key fragment size: no such field");
	if(!_decl->fields[field].type->size) {
		Q_ASSERT(field == _decl->nFields - 1); //if a flex field is selected, it MUST be the last field
		if(size_t(_data.size()) < _decl->fields[field].offset)
			return 0;
		else
			return _data.size() - _decl->fields[field].offset;
	}
	else
		return _decl->fields[field].type->size;
}

QByteArray KeyEditor::cutAt(size_t from, size_t n) const
{
	if(!_decl)
		throw KeyException("cannot cut key: key is null");
	else if(from + n > _set)
		throw KeyException("cannot cut key: invalid range");
	else if(!n)
		return QByteArray();
	return _data.mid(offsetAt(from), offsetAt(from + n - 1) + sizeAt(from + n - 1) - offsetAt(from));
}

void KeyEditor::pasteAt(size_t field, const QByteArray &data)
{
	if(!_meta)
		throw KeyException("cannot paste key without a valid meta spec");
	else if(_decl && field >= _decl->nFields && !_decl->abstract)
		throw KeyException("cannot paste key: no such field");
	else if(_decl && field > _decl->nFields)
		throw KeyException("cannot paste key: no such (inferrable) field");
	else if(!_decl && field > 0)
		throw KeyException("cannot paste key: no such (inferrable) field");
	const DeclKey **subs;
	size_t nsub;
	size_t end; //where does our current key end?
	size_t cur = field; //current field
	size_t srcoff = 0;
	size_t tgtoff;
	if(!field) {
		subs = _meta->rootKeys;
		nsub = _meta->nRootKeys;
		tgtoff = 0;
		end = 0;
		_set = field;
	}
	else {
		const DeclField *last = _decl->fields + _decl->nFields - 1;
		subs = _decl->subKeys;
		nsub = _decl->nSubKeys;
		if(field == _decl->nFields)
			tgtoff = last->offset + last->type->size;
		else
			tgtoff = _decl->fields[field].offset;
		end = _decl->nFields;
	}

	while(srcoff < size_t(data.size())) {
//		printf("ITERATION: %zu %zu  %llu %llu\n", srcoff, tgtoff, cur, end);
		size_t cursz;
		if(cur == end)
			cursz = _meta->keyType->size;
		else if(!_decl->fields[cur].type->size) { //final flex field 
			cursz = size_t(data.size()) - srcoff;
			resize(tgtoff + cursz);
		}
		else
			cursz = _decl->fields[cur].type->size;
		Q_ASSERT(cursz);
		if(size_t(data.size()) < srcoff + cursz)
			throw KeyException("cannot paste key: corrupted data");

		if(cur == end) {
			quint64 keyid = getint(data, srcoff, _meta->keyType) - 1;
//			printf("NEW KEY: %zu %zu\n", keyid, srcoff);
			_decl = nullptr;
			for(size_t i = 0; i < nsub; i++)
				if(subs[i]->index == keyid) {
					_decl = subs[i];
					break;
				}
			if(!_decl)
				throw KeyException("cannot paste key: no such key");
			subs = _decl->subKeys;
			nsub = _decl->nSubKeys;
			end = _decl->nFields;
			_set = cur;
			Q_ASSERT(end > cur);
		}
		resizeUp(tgtoff + cursz);
//		printf("PUT DATA: %zu %llu\n", tgtoff, cursz);
		memcpy(_data.data() + tgtoff, data.constData() + srcoff, cursz);
		srcoff += cursz;
		tgtoff += cursz;
		cur++;
	}
	if(_set < cur)
		_set = cur;
}

void KeyEditor::paste(const QByteArray &data)
{
	pasteAt(0, data);
}

void KeyEditor::pasteAppend(const QByteArray &data)
{
	pasteAt(_set, data);
}

QString KeyEditor::fieldNameAt(size_t index) const
{
	if(!_decl || index >= _decl->nFields)
		return QString();
	else
		return _decl->fields[index].name;
}

void KeyEditor::uintPutAt(size_t index, quint64 value)
{
	if(!_decl || _decl->nFields <= index)
		throw KeyException("cannot set uint: no such field");
	const DeclField *field = _decl->fields + index;
	if(field->key)
		throw KeyException("cannot set uint: field is constant for key");
	else if(field->type->nEnumValues && value > field->type->nEnumValues)
		throw KeyException("cannot set uint: given value out of range (enum)");
	putint(field->offset, field->type, value);
	if(index >= _set)
		_set = index + 1;
}

void KeyEditor::sintPutAt(size_t index, qint64 value)
{
	if(!_decl || _decl->nFields <= index)
		throw KeyException("cannot set sint: no such field");
	const DeclField *field = _decl->fields + index;
	if(field->key)
		throw KeyException("cannot set sint: field is constant for key");
	else if(field->type->nEnumValues && quint64(value) > field->type->nEnumValues)
		throw KeyException("cannot set sint: given value out of range (enum)");
	putint(field->offset, field->type, value);
	if(index >= _set)
		_set = index + 1;
}

void KeyEditor::stringPutAt(size_t index, const QString &value)
{
	if(!_decl || _decl->nFields <= index)
		throw KeyException("cannot set string: no such field");
	const DeclField *field = _decl->fields + index;
	if(field->key)
		throw KeyException("cannot set string: field is constant for key");
	else if(field->type->nEnumValues)
		throw KeyException("cannot set string: given field is an enum (unimplemented yet)");
	putstr(field->offset, field->type, value);
	if(index >= _set)
		_set = index + 1;
}

void KeyEditor::clearAt(size_t index)
{
	if(!_decl || _decl->nFields <= index)
		throw KeyException("cannot set string: no such field");
	const DeclField *field = _decl->fields + index;
	if(field->key)
		throw KeyException("cannot set string: field is constant for key");
	putnull(field->offset, field->type);
	if(index >= _set)
		_set = index + 1;
}

quint64 KeyEditor::uintGetAt(size_t index) const
{
	if(!_decl || _decl->nFields <= index)
		throw KeyException("cannot get uint: no such field");
	const DeclField *field = _decl->fields + index;
	return getint(field->offset, field->type);
}

qint64 KeyEditor::sintGetAt(size_t index) const
{
	if(!_decl || _decl->nFields <= index)
		throw KeyException("cannot get sint: no such field");
	const DeclField *field = _decl->fields + index;
	return getint(field->offset, field->type);
}

QString KeyEditor::stringGetAt(size_t index) const
{
	if(!_decl || _decl->nFields <= index)
		throw KeyException("cannot get string: no such field");
	const DeclField *field = _decl->fields + index;
	if(field->type->nEnumValues)
		throw KeyException("cannot get string: given field is an enum (unimplemented yet)");
	return getstr(field->offset, field->type);
}

void KeyEditor::truncateAt(size_t field)
{
	if(field < _fixed)
		throw KeyException("cannot truncate fixed field");
	else if(!_decl || field >= _decl->nFields)
		throw KeyException("cannot truncate: no such field");
	resizeDown(_decl->fields[field].offset);
	if(_set > field)
		_set = field;
}

void KeyEditor::fromstringAt(size_t index, const QString &value)
{
	if(!_decl || _decl->nFields <= index)
		return;
	const DeclField *field = _decl->fields + index;
	const DeclType *type = field->type;
	if(type->builtin == BuiltinU8 || type->builtin == BuiltinU16 || type->builtin == BuiltinU32 || type->builtin == BuiltinU64) {
		bool ok;
		qlonglong iv = value.toLongLong(&ok);
		if(ok)
			putint(field->offset, type, iv);
		else if(type->nEnumValues > 0) {
			QByteArray utf8 = value.toUtf8();
			iv = type->nameIndexOf(utf8.constData());
			if(iv < 0)
				putint(field->offset, type, 0);
			else
				putint(field->offset, type, iv + 1);
		}
		else
			putint(field->offset, type, 0);
		if(index >= _set)
			_set = index + 1;
	}
	else if(type->builtin == BuiltinFlexString) {
		putstr(field->offset, type, value);
		if(index >= _set)
			_set = index + 1;
	}
}

QString KeyEditor::tostringAt(size_t index, bool textFirst) const
{
	if(!_decl || _set <= index)
		return QString();
	const DeclField *field = _decl->fields + index;
	const DeclType *type = field->type;
	if(type->builtin == BuiltinU8 || type->builtin == BuiltinU16 || type->builtin == BuiltinU32 || type->builtin == BuiltinU64) {
		quint64 value = getint(field->offset, type);
		if(type->nEnumValues > 0) {
			if(!value)
				return "(null)";
			else if(value > type->nEnumValues)
				return "(out of range)";
			else if((textFirst && type->enumValues[value - 1].text) || !type->enumValues[value - 1].name)
				return type->enumValues[value - 1].text;
			else
				return type->enumValues[value - 1].name;
		}
		else
			return QString::number(value);
	}
	else if(type->builtin == BuiltinFlexString)
		return getstr(field->offset, type);
	else
		return QString();
}

void KeyEditor::select(const DeclKey *decl, bool keep) {
	keep = keep && _decl;
	if(!keep)
		_data = QByteArray();
	_fixed = 0;
	for(size_t i = 0; i < decl->nFields; i++) {
		if(decl->fields[i].key) {
			size_t offset = decl->fields[i].offset;
			size_t size = decl->fields[i].type->size;
			_fixed = i + 1;
			if(keep && (_decl->nFields <= i || decl->fields[i].key != _decl->fields[i].key)) {
				putint(offset, decl->fields[i].type, decl->fields[i].key->index + 1);
				resizeDown(offset + size);
				keep = false;
			}
			else if(!keep)
				putint(offset, decl->fields[i].type, decl->fields[i].key->index + 1);
		}
	}
	_decl = decl;
	_set = _fixed;
}

void KeyEditor::dump(const QString &prefix) const
{
	QByteArray utf8 = prefix.toUtf8();
	if(!_decl) {
		printf("%sNull key\n", utf8.constData());
		return;
	}
	printf("%sKey %s: set=%zu fixed=%zu full-fields=%zu size=%d full-size=%zu\n", utf8.constData(), _decl->fullname, _set, _fixed, _decl->nFields, _data.size(), _decl->size);
	size_t offset = 0;
	for(size_t i = 0; i < _set; i++) {
		const DeclField *field = _decl->fields + i;
		const DeclType *type = field->type;
		printf("%s  [%4zu] %s%s=", utf8.constData(), i, field->key ? "key_" : "", field->name);
		Editor::print(_data, offset, type);
		printf("\n");
		offset += type->size;
	}
	printf("%sHexdump:\n", utf8.constData());
	qHexdump(_data, utf8.constData());
}

