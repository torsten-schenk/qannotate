#pragma once

#include "common/qException.h"

#include "spec.h"

class Editor {
	public:
		static quint64 getint(const QByteArray &data, size_t offset, const spec::DeclType *type);
		static QString getstr(const QByteArray &data, size_t offset, const spec::DeclType *type);
		static void print(const QByteArray &data, size_t offset, const spec::DeclType *type);

	protected:
		void putint(size_t offset, const spec::DeclType *type, quint64 value);
		void putstr(size_t offset, const spec::DeclType *type, const QString &value);
		void putnull(size_t offset, const spec::DeclType *type);
		quint64 getint(size_t offset, const spec::DeclType *type) const { return getint(_data, offset, type); }
		QString getstr(size_t offset, const spec::DeclType *type) const { return getstr(_data, offset, type); }
		void resize(size_t size);
		void resizeUp(size_t size);
		void resizeDown(size_t size);

		QByteArray _data;
};

