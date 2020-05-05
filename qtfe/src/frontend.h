#pragma once

#include <QDir>

#include "common/qException.h"

#include "spec.h"

class RowEditor;

class FrontendException : public Exception {
	public:
		FrontendException(const QString &message) : Exception(message) {}
};

class Frontend {
	public:
		Frontend(const spec::DeclMeta *spec);
		virtual ~Frontend() {}

		const spec::DeclMeta *spec() const {
			return _spec;
		}

	private:
		const spec::DeclMeta *_spec;
};

class SyncFrontend : public Frontend {
	public:
		virtual spec::Value *get(const QByteArray &key, bool create = false) = 0;
		virtual void modified(const QByteArray &key);
		virtual void erase(const QByteArray &key) = 0;
		virtual void move(const QByteArray &oldkey, const QByteArray &newkey) = 0;
		virtual void prefixErase(const QByteArray &prefix) = 0;
		virtual quint64 acquire(const spec::DeclType *type) = 0;
		virtual size_t size() const = 0;
		virtual QByteArray key(size_t pos) = 0;
		virtual spec::Value *value(size_t pos) = 0;
		//returns position of first element, which is >= prefix (inclusive)
		virtual size_t lower(const QByteArray &prefix) = 0;
		//returns position of first element, which is > key (key-based)
		virtual size_t upper(const QByteArray &key) = 0;
		//returns position of first element, which is > prefix (prefix-based)
		virtual size_t prefixUpper(const QByteArray &prefix) = 0;
		virtual bool contains(const QByteArray &key) = 0;
		virtual void foreachKV(const std::function<void(const QByteArray &key, const spec::Value *value)> &fn) = 0;
		virtual void dump() const = 0;

		void gc();

		static void diff(SyncFrontend *a, SyncFrontend *b, const std::function<void(const QByteArray&, spec::Value*, const QByteArray&, spec::Value*)> &cb);

		template<typename T> T *value(size_t pos) {
			T *result = dynamic_cast<T*>(value(pos));
			if(!result)
				throw FrontendException("invalid value type requested");
			return result;
		}

		template<typename T> T *get(const QByteArray &key, bool create = false) {
			spec::Value *v = get(key, create);
			if(!v) {
				return nullptr;
			}
			T *result = dynamic_cast<T*>(v);
			if(!result)
				throw FrontendException("invalid value type requested");
			return result;
		}

		template<typename T> T *tryget(const QByteArray &key, bool create = false) {
			spec::Value *v = get(key, create);
			if(!v) {
				return nullptr;
			}
			return dynamic_cast<T*>(v);
		}

		void range(size_t *l, size_t *u, const QByteArray &key) {
			*l = lower(key);
			*u = upper(key);
		}

		void prefixRange(size_t *l, size_t *u, const QByteArray &prefix) {
			*l = lower(prefix);
			*u = prefixUpper(prefix);
		}

		size_t prefixCount(const QByteArray &prefix) {
			size_t l = lower(prefix);
			size_t u = prefixUpper(prefix);
			return u - l;
		}

	protected:
		SyncFrontend(const spec::DeclMeta *spec);
};

class AsyncFrontend : public Frontend {

};

class MemoryFrontend : public SyncFrontend {
	public:
		MemoryFrontend(const spec::DeclMeta *spec);
		virtual ~MemoryFrontend();

		virtual spec::Value *get(const QByteArray &key, bool create = false) override;
		virtual void erase(const QByteArray &key) override;
		virtual void move(const QByteArray &oldkey, const QByteArray &newkey) override;
		virtual void prefixErase(const QByteArray &prefix) override;
		virtual quint64 acquire(const spec::DeclType *type) override;
		virtual size_t size() const override;
		virtual QByteArray key(size_t pos) override;
		virtual spec::Value *value(size_t pos) override;
		virtual size_t lower(const QByteArray &prefix) override;
		virtual size_t upper(const QByteArray &key) override;
		virtual size_t prefixUpper(const QByteArray &prefix) override;
		virtual bool contains(const QByteArray &key) override;
		virtual void foreachKV(const std::function<void(const QByteArray &key, const spec::Value *value)> &fn) override;

		virtual void dump() const override;

	private:
		AvlTable<1, QByteArray, spec::Value*> _store;
		QVector<quint64> _nextid;
};

class AbstractDirFrontend : public SyncFrontend {
	public:
		AbstractDirFrontend(const spec::DeclMeta *spec, const QDir &dir);
		virtual ~AbstractDirFrontend();

		void load();
		void clear();
		QDir rootdir() const;
		void suspend();
		void resume();

		static QString decodeFilename(const QString &filename);
		static QString encodeFilename(const QString &name);

		virtual spec::Value *get(const QByteArray &key, bool create = false) override;
		virtual void modified(const QByteArray &key) override;
		virtual void erase(const QByteArray &key) override;
		virtual void move(const QByteArray &oldkey, const QByteArray &newkey) override;
		virtual void prefixErase(const QByteArray &prefix) override;
		virtual quint64 acquire(const spec::DeclType *type) override;
		virtual size_t size() const override;
		virtual QByteArray key(size_t pos) override;
		virtual spec::Value *value(size_t pos) override;
		virtual size_t lower(const QByteArray &prefix) override;
		virtual size_t upper(const QByteArray &key) override;
		virtual size_t prefixUpper(const QByteArray &prefix) override;
		virtual bool contains(const QByteArray &key) override;
		virtual void foreachKV(const std::function<void(const QByteArray &key, const spec::Value *value)> &fn) override;

		virtual void dump() const override;


	protected:
		virtual void actionLoad() = 0;
		virtual void actionSave() = 0;
		virtual void actionErase(const QByteArray &key, spec::Value *value) = 0;
		virtual void actionModify(const QByteArray &key, spec::Value *value) = 0;

		void put(const QByteArray &key, spec::Value *value);

	private:
		bool _suspended;
		QDir _rootdir;
		AvlTable<1, QByteArray, spec::Value*> _store;
		QVector<quint64> _nextid;
};

class CacheFrontend : public SyncFrontend {

};

class CachingFrontend : public AsyncFrontend {
	public:
		CachingFrontend(CacheFrontend *cache);

};

#if 0
class SyncFrontend public Frontend {
	public:
		using Exception = spec::Exception; //TODO ugly...

		enum Mode {
			NoLock, ReadLock, WriteLock
		};

		SyncFrontend(const spec::DeclMeta *spec, anndb::SyncBackend *backend);

		bool put(const QByteArray &key, const spec::Value &value);

		//if value is empty and key does not exist, NOP.
		//if value is empty and (at least one) key exists, remove first entry.
		//if value is not empty and key does not exist, insert entry.
		//if value is not empty and key exists, replace first entry.
		bool put(const QByteArray &key, const QByteArray &value) {
			return _backend->put(key, value);
		}

		bool put(const QByteArray &key, const QList<QByteArray> &value) {
			if(!_backend->clear(key))
				return false;
			for(auto it : value)
				if(!_backend->append(key, it))
					return false;
			return true;
		}

		bool append(const QByteArray &key, const QByteArray &value) {
			return _backend->append(key, value);
		}

		bool prepend(const QByteArray &key, const QByteArray &value) {
			return _backend->prepend(key, value);
		}

		bool insert(const QByteArray &key, size_t index, const QByteArray &value) {
			return _backend->insert(key, index, value);
		}

		bool replace(const QByteArray &key, size_t index, const QByteArray &value) {
			return _backend->replace(key, index, value);
		}

		bool remove(const QByteArray &key, size_t index = 0, size_t n = 1) {
			return _backend->remove(key, index, n);
		}

		bool prefixClear(const QByteArray &prefix = QByteArray()) {
			return _backend->prefixClear(prefix);
		}

		bool clear(const QByteArray &key) {
			return _backend->clear(key);
		}

		bool get(QByteArray *value, const QByteArray &key, size_t index = 0) {
			return _backend->get(*value, key, index);
		}

		//acquires a new id from the backend
		//if 0 has been returned, an error has occured.
		quint64 acquire(const spec::DeclType *type) {
			if(!type->dynamicId)
				throw Exception("type is not used as dynamic id");
			quint64 id = _backend->acquire(type->dynamicId);
			if(!id || id > type->uintMax())
				throw Exception("cannot acquire id: id too big for datatype");
			return id;
		}

		//returns the current username set in backend
		QString username() const {
			return _backend->username();
		}

		//write a line with given indentation into log specified by 'out'. 'out' is backend-dependent.
		bool log(int out, int indent, const QString &line) {
			return _backend->log(out, indent, line);
		}

		//returns number of values in backend
		size_t size() const {
			return _backend->size();
		}

		bool key(QByteArray *key, size_t pos) {
			return _backend->key(*key, pos);
		}

		bool value(QByteArray *value, size_t pos) {
			return _backend->value(*value, pos);
		}

		//returns position of first element, which is >= prefix (inclusive)
		bool lower(size_t *l, const QByteArray &prefix) {
			return _backend->lower(*l, prefix);
		}

		//returns position of first element, which is > key (key-based)
		bool upper(size_t *u, const QByteArray &key) {
			return _backend->upper(*u, key);
		}

		bool range(size_t *l, size_t *u, const QByteArray &key) {
			return _backend->lower(*l, key) && _backend->upper(*u, key);
		}

		//returns position of first element, which is > prefix (prefix-based)
		bool prefixUpper(size_t *u, const QByteArray &prefix) {
			return _backend->prefixUpper(*u, prefix);
		}

		bool prefixRange(size_t *l, size_t *u, const QByteArray &prefix) {
			return _backend->lower(*l, prefix) && _backend->prefixUpper(*u, prefix);
		}

		bool foreachKV(const std::function<bool(const QByteArray&, const QByteArray&)> &cb) {
			return foreachKV(QByteArray(), cb);
		}

		bool foreachUniqueKV(const std::function<bool(const QByteArray&, const QList<QByteArray>&)> &cb) {
			return foreachUniqueKV(QByteArray(), cb);
		}

		bool prefixCount(size_t *n, const QByteArray &prefix) {
			size_t l;
			size_t u;
			if(!_backend->lower(l, prefix) || !_backend->prefixUpper(u, prefix))
				return false;
			*n = u - l;
			return true;
		}

		bool foreachKV(const QByteArray &prefix, const std::function<bool(const QByteArray&, const QByteArray&)> &cb);
		bool foreachUniqueKV(const QByteArray &prefix, const std::function<bool(const QByteArray&, const QList<QByteArray>&)> &cb);

	private:
		anndb::SyncBackend *_backend;
};
/*
class AsyncFrontend : public Frontend {
	public:
		AsyncFrontend(AsyncBackend *backend);
};
*/
#endif

