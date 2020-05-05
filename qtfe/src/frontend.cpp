#include "key.h"
#include "frontend.h"

using namespace spec;

Frontend::Frontend(const DeclMeta *spec)
	:	_spec(spec)
{
	if(!spec)
		throw FrontendException("cannot create frontend: missing spec");
}

void SyncFrontend::modified(const QByteArray &key)
{}

void SyncFrontend::diff(SyncFrontend *a, SyncFrontend *b, const std::function<void(const QByteArray&, Value*, const QByteArray&, Value*)> &cb)
{}

SyncFrontend::SyncFrontend(const DeclMeta *spec)
	:	Frontend(spec)
{}

void SyncFrontend::gc()
{}

MemoryFrontend::MemoryFrontend(const DeclMeta *spec)
	:	SyncFrontend(spec)
{
	_nextid.resize(spec->nIdTypes);
}

MemoryFrontend::~MemoryFrontend()
{
	for(auto it = _store.all(); !it.atEnd(); it.next())
		it.cell<1>()->decl()->destroy(it.cell<1>());
}

Value *MemoryFrontend::get(const QByteArray &key, bool create)
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

void MemoryFrontend::erase(const QByteArray &key)
{
	auto it = _store.single(key);
	if(!it.atEnd()) {
		it.cell<1>()->decl()->destroy(it.cell<1>());
		_store.removeAt(it.index());
	}
}

void MemoryFrontend::move(const QByteArray &oldkey, const QByteArray &newkey)
{
	Value *value = _store.cell<1>(oldkey);
	if(!value)
		throw FrontendException("cannot move value: no such value");
	else if(_store.cell<1>(newkey))
		throw FrontendException("cannot move value: key already exists");
	_store.remove(oldkey);
	_store.put(newkey, value);
}

void MemoryFrontend::prefixErase(const QByteArray &prefix)
{
	size_t l = lower(prefix);
	size_t u = prefixUpper(prefix);
	while(u > l) {
		Value *value = _store.cellAt<1>(l);
		value->decl()->destroy(value);
		_store.removeAt(l);
		u--;
	}
}

quint64 MemoryFrontend::acquire(const spec::DeclType *type)
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

size_t MemoryFrontend::size() const
{
	return _store.size();
}

QByteArray MemoryFrontend::key(size_t pos)
{
	if(pos >= _store.size())
		return QByteArray();
	return _store.cellAt<0>(pos);
}

spec::Value *MemoryFrontend::value(size_t pos)
{
	if(pos >= _store.size())
		return nullptr;
	return _store.cellAt<1>(pos);
}

size_t MemoryFrontend::lower(const QByteArray &prefix)
{
	return _store.lower(prefix).index();
}

size_t MemoryFrontend::upper(const QByteArray &key)
{
	return _store.upper(key).index();
}

size_t MemoryFrontend::prefixUpper(const QByteArray &prefix)
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

bool MemoryFrontend::contains(const QByteArray &key)
{
	return _store.contains(key);
}

void MemoryFrontend::dump() const
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

void MemoryFrontend::foreachKV(const std::function<void(const QByteArray &key, const Value *value)> &fn)
{
	for(auto it = _store.all(); !it.atEnd(); it.next())
		fn(it.cell<0>(), it.cell<1>());
}
#if 0
bool SyncFrontend::put(const QByteArray &key, const Value &value)
{
	return put(key, ValueData(value));
}

bool SyncFrontend::foreachKV(const QByteArray &prefix, const std::function<bool(const QByteArray&, const QByteArray&)> &cb)
{
	size_t l;
	size_t u;
	if(!_backend->lower(l, prefix) || !_backend->prefixUpper(u, prefix))
		return false;
	while(l < u) {
		QByteArray key;
		QByteArray value;
		if(!_backend->key(key, l) || !_backend->value(value, l))
			return false;
		else if(!cb(key, value))
			return true;
		l++;
	}
	return true;
}

bool SyncFrontend::foreachUniqueKV(const QByteArray &prefix, const std::function<bool(const QByteArray&, const QList<QByteArray>&)> &cb)
{
	size_t l;
	size_t u;
	if(!_backend->lower(l, prefix) || !_backend->prefixUpper(u, prefix))
		return false;
	while(l < u) {
		size_t curu;
		QByteArray key;
		QList<QByteArray> value;
		if(!_backend->key(key, l))
			return false;
		else if(!_backend->upper(curu, key))
			return false;
		Q_ASSERT(curu <= u);
		Q_ASSERT(curu > l);
		while(l < curu) {
			QByteArray curvalue;
			if(!_backend->value(curvalue, l))
				return false;
			value.append(curvalue);
			l++;
		}
		if(!cb(key, value))
			return false;
	}
	return true;
}
#endif

