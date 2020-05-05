#pragma once

#include "editor.h"

class KeyException : public Exception {
	public:
		KeyException(const QString &message) : Exception(message) {}
};

class KeyEditor : public Editor {
	public:
		KeyEditor(const spec::DeclMeta *meta, const QByteArray &key = QByteArray());
		KeyEditor(const spec::DeclKey *decl);

		void load(const QByteArray &key);
		QString name() const; //return name of current key
		void select(const spec::DeclKey *decl, bool keep = false); //initialize a new keytype; keep parent key data if applicable
		size_t fixed() const; //return number of fields, which need to be present. this depends on the currently selected key. to select another key, use select() first. NOTE: field value of fixed fields CAN be changed (except for key fields).
		size_t size() const; //return number of bytes
		size_t fields() const; //return number of set fields
		const void *data() const; //return key data
		bool isValid(bool allowNull = false) const; //checks, whether the current values are within their limits.
		bool hasMissing() const; //check, whether missing field values exist. A missing field is an enum field, which has the value 0. Note that all enums start with 1 (key-internally), so 0 denotes null/missing in any enum case. Note: fields which are not present (i.e. if the current value is not a key, just a prefix) don't count as missing.
		bool isKey() const; //returns true, if current key is a full key. this just checks the number of available fields.
		bool isValidKey() const; //return isKey() && isValid(false)
		bool isAbstract() const; //return whether current key is an abstract key. Abstract keys are ALWAYS prefixes.
		bool isFull() const; //return whether all fields of current key are set.
		bool isNull() const; //return whether no key is currently set (i.e. _decl == nullptr)
		const spec::DeclValue *mapto() const; //return, which type is currently assigned to this key. looks up from specific to general: start looking at last field, search to front. Note: field must be set in order to be checked
		const spec::DeclKey *decl() const;

		void dump(const QString &prefix = QString()) const;

		operator QByteArray() const;

		//aux stuff
		template<typename STRUCT> const STRUCT &auxKey() const {
			return spec::Mapping<spec::DeclKey, STRUCT>::get(_decl);
		}

		template<typename STRUCT> const STRUCT &auxFieldAt(size_t index) const {
			if(!_decl || _set <= index)
				return STRUCT::Default;
			const spec::DeclField *field = _decl->fields + index;
			const STRUCT *value;
			if(field->type->nEnumValues > 0) {
				quint64 intval = uintGetAt(index);
				if(!intval || intval > field->type->nEnumValues)
					return STRUCT::Default;
				value = &spec::EnumMapping<STRUCT>::get(field->type, intval - 1);
				if(value != &STRUCT::Default)
					return *value;
			}
			return spec::TypeMapping<STRUCT>::get(field->type);
		}

		//integer-based API
		size_t offsetAt(size_t field) const; //field doesn't have to be set.
		size_t sizeAt(size_t field) const; //field doesn't have to be set; if field is a flex field and it has not been set, 0 is returned.
		QByteArray cutAt(size_t from, size_t n = 1) const; //extract part of the key. does not change the internal key.
		void pasteAt(size_t field, const QByteArray &data); //try to put 'data' into the key at given field. Can override 'key' fields, so a valid _meta is required. If a 'key' field is overwritten, resulting key will be cut after 'data'. otherwise, just the fields in 'data' will be overridden. key will also be expanded.
		void paste(const QByteArray &data);
		void pasteAppend(const QByteArray &data);
		QString fieldNameAt(size_t field) const;
		void uintPutAt(size_t field, quint64 value);
		void sintPutAt(size_t field, qint64 value);
		void stringPutAt(size_t field, const QString &value);
		void clearAt(size_t field);
		quint64 uintGetAt(size_t field) const;
		qint64 sintGetAt(size_t field) const;
		QString stringGetAt(size_t field) const;
		void truncateAt(size_t field); //truncate key BEFORE given field
		bool isKeyFieldAt(size_t field) const;
		bool isIdFieldAt(size_t field) const;
		const spec::DeclType *typeAt(size_t field) const;

		void fromstringAt(size_t field, const QString &value);
		QString tostringAt(size_t field, bool textFirst = false) const; //convert a field to a string. if field does not exist, return null string

		template<typename E> E enumGetAt(size_t field) {
			static_assert(spec::EnumInfo<E>::IsEnum, "not an enum");
			if(!_decl || field >= _decl->nFields || spec::EnumInfo<E>::DeclObject != _decl->fields[field].type)
				throw KeyException("enum does not belong to given field");
			return spec::EnumInfo<E>::fromInt(uintGetAt(field));
		}

		template<typename E> void enumPutAt(size_t field, E value) {
			static_assert(spec::EnumInfo<E>::IsEnum, "not an enum");
			if(!_decl || field >= _decl->nFields || spec::EnumInfo<E>::DeclObject != _decl->fields[field].type)
				throw KeyException("enum does not belong to given field");
			uintPutAt(field, spec::EnumInfo<E>::toInt(value));
		}

		//enum-based API
		template<typename T> QByteArray cutBefore(T field) const { //cut from beginning of key up to (excluding) 'field'
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			return cutAt(0, spec::KeyInfo<T>::toInt(field));
		}

		template<typename T> QByteArray cutAfter(T field) const { //cut from beginning of key up to (including) 'field'
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			return cutAt(0, spec::KeyInfo<T>::toInt(field) + 1);
		}

		template<typename T> QByteArray cut(T from, size_t n = 1) const { //cut given range
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			return cutAt(spec::KeyInfo<T>::toInt(from), n);
		}

		template<typename T> QByteArray cut(T from, T to, bool inclusive = true) const { //cut given range ('inclusive' specifies, whether 'to' is inclusive or exclusive)
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			size_t begin = spec::KeyInfo<T>::toInt(from);
			size_t end = spec::KeyInfo<T>::toInt(to);
			if(end < begin)
				throw KeyException("cannot cut key: invalid range");
			return cutAt(begin, end - begin + (inclusive ? 1 : 0));
		}

		template<typename T> void paste(T field, const QByteArray &data) {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			pasteAt(spec::KeyInfo<T>::toInt(field), data);
		}

		template<typename T> void truncate(T field) {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			truncateAt(spec::KeyInfo<T>::toInt(field));
		}

		template<typename T> void clear(T field) {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			clearAt(spec::KeyInfo<T>::toInt(field));
		}

		template<typename F, typename E> void enumPut(F field, E value) {
			static_assert(spec::KeyInfo<F>::IsKey, "not a field");
			static_assert(spec::EnumInfo<E>::IsEnum, "not an enum");
			if(spec::KeyInfo<F>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			else if(spec::EnumInfo<E>::DeclObject != _decl->fields[spec::KeyInfo<F>::toInt(field)].type)
				throw KeyException("enum does not belong to given field");
			uintPutAt(spec::KeyInfo<F>::toInt(field), spec::EnumInfo<E>::toInt(value));
		}

		template<typename T> void uintPut(T field, quint64 value) {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			uintPutAt(spec::KeyInfo<T>::toInt(field), value);
		}

		template<typename T> void sintPut(T field, qint64 value) {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			sintPutAt(spec::KeyInfo<T>::toInt(field), value);
		}

		template<typename T> void stringPut(T field, const QString &value) {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			stringPutAt(spec::KeyInfo<T>::toInt(field), value);
		}

		template<typename E, typename F> E enumGet(F field) {
			static_assert(spec::KeyInfo<F>::IsKey, "not a field");
			static_assert(spec::EnumInfo<E>::IsEnum, "not an enum");
			if(spec::KeyInfo<F>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			else if(spec::EnumInfo<E>::DeclObject != _decl->fields[spec::KeyInfo<F>::toInt(field)].type)
				throw KeyException("enum does not belong to given field");
			return spec::EnumInfo<E>::fromInt(uintGetAt(spec::KeyInfo<F>::toInt(field)));
		}

		template<typename T> quint64 uintGet(T field) const {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			return uintGetAt(spec::KeyInfo<T>::toInt(field));
		}

		template<typename T> qint64 sintGet(T field) const {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			return sintGetAt(spec::KeyInfo<T>::toInt(field));
		}

		template<typename T> QString stringGet(T field) const {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			return stringGetAt(spec::KeyInfo<T>::toInt(field));
		}

		template<typename T> bool isKeyField(T field) const {
			static_assert(spec::KeyInfo<T>::IsKey, "not a field");
			if(spec::KeyInfo<T>::DeclObject != _decl)
				throw KeyException("field does not belong to current key");
			return isKeyFieldAt(spec::KeyInfo<T>::toInt(field));
		}

	private:
		const spec::DeclMeta *_meta;
		const spec::DeclKey *_decl;

		size_t _fixed; //number of fields, which cannot be removed for the current key
		size_t _set; //number of fields which have been set
};

