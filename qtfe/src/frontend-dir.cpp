#include <common/base32.h>

#include "key.h"
#include "frontend.h"
 
using namespace spec;

QString AbstractDirFrontend::decodeFilename(const QString &filename)
{
	QByteArray data = filename.toUpper().toLocal8Bit();
	QByteArray utf8;
	ssize_t len = base32_decsize(data.size());
	if(len < 0)
		return QString();
	utf8.resize(len);
	base32_dec(utf8.data(), data.constData());
	return QString::fromUtf8(utf8);
}

QString AbstractDirFrontend::encodeFilename(const QString &name)
{
	QByteArray data;
	QByteArray utf8 = name.toUtf8();
	data.resize(base32_encsize(utf8.size()));
	base32_enc(data.data(), utf8.constData(), utf8.size());
	return QString::fromLocal8Bit(data).toLower();
}

AbstractDirFrontend::AbstractDirFrontend(const spec::DeclMeta *spec, const QDir &dir)
	:	SyncFrontend(spec),
		_suspended(false),
		_rootdir(dir)
{
	if(!dir.exists())
		throw FrontendException("No such frontend directory");
	_nextid.resize(spec->nIdTypes);
}

AbstractDirFrontend::~AbstractDirFrontend()
{
	for(auto it = _store.all(); !it.atEnd(); it.next())
		it.cell<1>()->decl()->destroy(it.cell<1>());
}

void AbstractDirFrontend::load()
{
	bool suspended = _suspended;
	_suspended = true;
	actionLoad();
	_suspended = suspended;
}

void AbstractDirFrontend::clear()
{
}

QDir AbstractDirFrontend::rootdir() const
{
	return _rootdir;
}

void AbstractDirFrontend::suspend()
{
	_suspended = true;
}

void AbstractDirFrontend::resume()
{
	if(_suspended) {
		_suspended = false;
		actionSave();
	}
}

void AbstractDirFrontend::modified(const QByteArray &key)
{
	if(_suspended)
		return;
	Value *value = _store.cell<1>(key);
	if(!value)
		return;
	actionModify(key, value);
}


Value *AbstractDirFrontend::get(const QByteArray &key, bool create)
{
	Value *v = _store.cell<1>(key);
	if(!v && create) {
		KeyEditor keyed(spec(), key);
		if(!keyed.isValid(true))
			throw FrontendException("error creating value: invalid key");
		const DeclValue *mapto = keyed.mapto();
		if(!mapto)
			throw FrontendException("error creating value: key does not map to a value");
		v = static_cast<Value*>(mapto->create());
		_store.insert(key, v);
	}
	return v;
}

void AbstractDirFrontend::erase(const QByteArray &key)
{
	auto it = _store.single(key);
	if(!it.atEnd()) {
		if(!_suspended)
			actionErase(key, it.cell<1>());
		it.cell<1>()->decl()->destroy(it.cell<1>());
		_store.removeAt(it.index());
	}
}

void AbstractDirFrontend::move(const QByteArray &oldkey, const QByteArray &newkey)
{
	Value *value = _store.cell<1>(oldkey);
	if(!value)
		throw FrontendException("cannot move value: no such value");
	else if(_store.cell<1>(newkey))
		throw FrontendException("cannot move value: key already exists");
	_store.remove(oldkey);
	_store.put(newkey, value);
	if(!_suspended) {
		actionErase(oldkey, value);
		actionModify(newkey, value);
	}
}

void AbstractDirFrontend::prefixErase(const QByteArray &prefix)
{
	size_t l = lower(prefix);
	size_t u = prefixUpper(prefix);
	while(u > l) {
		Value *value = _store.cellAt<1>(l);
		if(!_suspended)
			actionErase(_store.cellAt<0>(l), value);
		value->decl()->destroy(value);
		_store.removeAt(l);
		u--;
	}
}

quint64 AbstractDirFrontend::acquire(const DeclType *type)
{
	if(!type->dynamicId || type->dynamicId > size_t(_nextid.size()))
		throw FrontendException("type is not used as dynamic id");
	quint64 id = ++_nextid[type->dynamicId - 1];
	if(!id || id > type->uintMax()) {
		_nextid[type->dynamicId - 1]--;
		throw FrontendException("cannot acquire id: id too big for datatype");
	}
	return id;
}

size_t AbstractDirFrontend::size() const
{
	return _store.size();
}

QByteArray AbstractDirFrontend::key(size_t pos)
{
	if(pos >= _store.size())
		return QByteArray();
	return _store.cellAt<0>(pos);
}

Value *AbstractDirFrontend::value(size_t pos)
{
	if(pos >= _store.size())
		return nullptr;
	return _store.cellAt<1>(pos);
}

size_t AbstractDirFrontend::lower(const QByteArray &prefix)
{
	return _store.lower(prefix).index();
}

size_t AbstractDirFrontend::upper(const QByteArray &key)
{
	return _store.upper(key).index();
}
	
size_t AbstractDirFrontend::prefixUpper(const QByteArray &prefix)
{
	QByteArray tmp = prefix;
	while(!tmp.isEmpty()) {
		uint8_t last = tmp.at(tmp.size() - 1);
		if(last < 0xff) {
			last++;
			tmp[tmp.size() - 1] = last;
			return _store.lower(tmp).index();
		}
		else
			tmp.chop(1);
	}
	return _store.size();
}

bool AbstractDirFrontend::contains(const QByteArray &key)
{
	return _store.contains(key);
}

void AbstractDirFrontend::foreachKV(const std::function<void(const QByteArray &key, const Value *value)> &fn)
{
	for(auto it = _store.all(); !it.atEnd(); it.next())
		fn(it.cell<0>(), it.cell<1>());
}

void AbstractDirFrontend::dump() const
{
	Value::ConstWalker walker;
	QStack<Name> names;
	walker.down = [&](Name name, size_t n) {
		names.push(name);
	};
	walker.up = [&](){ names.pop(); };
	walker.begin = [&](size_t index, const ValueAccessor *accessor, const void *handle) {
		for(size_t i = 0; i < size_t(names.size() + 1); i++)
			printf("  ");
		printf("%s[%zu]\n", qPrintable(name2string(names.top())), index);
	};
	walker.var = [&](size_t index, const DeclType *type, const void *handle) {
		for(size_t i = 0; i < size_t(names.size() + 1); i++)
			printf("  ");
		const char *quot = type->isString() ? "'" : "";
		printf("%s[%zu] = %s%s%s\n", qPrintable(name2string(names.top())), index, quot, qPrintable(type->tostring(handle)), quot);
	};
	printf("MemoryFrontend dump\n");
	printf("  store: %zu\n", _store.size());
	for(auto it = _store.all(); !it.atEnd(); it.next()) {
		qHexdump(it.cell<0>(), "    ");
		it.cell<1>()->constWalk(walker);
		printf("\n");
	}
	printf("  dynamic ids: %d\n", _nextid.size());
	for(int i = 0; i < _nextid.size(); i++)
		printf("    %s: %llu\n", spec()->idTypes[i]->fullname, _nextid[i]);
}

void AbstractDirFrontend::put(const QByteArray &key, Value *value)
{
	Value *cur = _store.cell<1>(key);
	if(cur)
		delete cur;
	_store.put(key, value);
}

