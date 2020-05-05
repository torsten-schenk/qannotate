#pragma once

#include <functional>
#include <QMap>
#include <QString>
#include <QStack>

#include <stdio.h>

#include "common/qException.h"
#include "common/qHexdump.h"
#include "common/qRoman.h"
#include "common/AvlTable.h"

#define _IN_SPEC_H
class SpecException : Exception {
	public:
		SpecException(const QString &message) : Exception(message) {}
};

namespace spec {
	enum Builtin {
		BuiltinU8, BuiltinU16, BuiltinU32, BuiltinU64, BuiltinBoolean, BuiltinFlexString, BuiltinKeyReference, BuiltinTypeReference, BuiltinStructReference
	};

	enum Ordering {
		NoOrdering, SetOrdering, ListOrdering
	};

	class ValueAccessor;
	template<typename T> struct EnumInfo;

	enum class Name; //defined in generated spec-global.h
	extern const QString names[];
#include "../spec/spec-global.h"

	static inline Name string2name(const QString &str) {
		for(size_t i = 1; i < nNames; i++)
			if(str == names[i])
				return static_cast<Name>(i);
		return Name::null;
	}

	static inline QString name2string(Name name) {
		size_t index = static_cast<size_t>(name);
		if(index >= nNames)
			return QString();
		else
			return names[index];
	}

	static inline quint64 index2enum(quint64 index) {
		return index + 1;
	}

	static inline quint64 enum2index(quint64 value) {
		if(!value)
			throw 1;
		return value - 1;
	}

	template<typename E> static inline const char *enum2name(E value) {
		return EnumInfo<E>::name(value);
	}

	static constexpr quint64 enumNull = 0;


	struct DeclType;
	struct DeclKey;
	struct DeclValue;
	struct DeclVar;
	struct DeclField;

	struct DeclEnum {
		const char *name = nullptr;
		const char *text = nullptr;
		const char *plural = nullptr;
		const DeclValue *mapto = nullptr;

	};

	struct DeclDynid {
		enum Type {
			NullType, FieldType, VarType
		};

		const DeclKey *key;
		Type type;
		union {
			size_t field;
			struct {
				const DeclValue *decl;
				const size_t *varpath;
			} value;
		} u;
	};

	struct DeclType {
		const char *name;
		const char *fullname;
		Builtin builtin;
		size_t size;
		const DeclType *super;
		Ordering enumOrdering;
		Ordering multiOrdering;
		const DeclEnum *enumValues;
		size_t nEnumValues;
		const DeclValue *mapto = nullptr;

		//only used for dynamic id types
		size_t dynamicId = 0; //id to use for acquire() function (0: not a dynamic id)
		const DeclDynid *owners = nullptr;
		size_t nOwners = 0;

		bool is(const DeclType *type) const {
			const DeclType *cur = this;
			while(cur) {
				if(cur == type)
					return true;
				cur = cur->super;
			}
			return false;
		}

		quint64 uintMax() const {
			switch(builtin) {
				case BuiltinU8: return std::numeric_limits<quint8>::max();
				case BuiltinU16: return std::numeric_limits<quint16>::max();
				case BuiltinU32: return std::numeric_limits<quint32>::max();
				case BuiltinU64: return std::numeric_limits<quint64>::max();
				default: return 0;
			}
		}

		ssize_t nameIndexOf(const QString &name) const {
			QByteArray utf8 = name.toUtf8();
			return nameIndexOf(utf8.constData());
		}

		ssize_t nameIndexOf(const char *name) const {
			if(!name)
				return -1;
			for(size_t i = 0; i < nEnumValues; i++)
				if(enumValues[i].name && !strcmp(enumValues[i].name, name))
					return i;
			return -1;
		}

		QString tostring(const void *handle) const {
			if(builtin == BuiltinU8 || builtin == BuiltinU16 || builtin == BuiltinU32 || builtin == BuiltinU64) {
				quint64 value;
				if(builtin == BuiltinU8)
					value = *static_cast<const quint8*>(handle);
				else if(builtin == BuiltinU16)
					value = *static_cast<const quint16*>(handle);
				else if(builtin == BuiltinU32)
					value = *static_cast<const quint32*>(handle);
				else if(builtin == BuiltinU64)
					value = *static_cast<const quint64*>(handle);
				else
					value = 0;
				if(nEnumValues) {
					if(value == 0)
						return "(null)";
					else if(value > nEnumValues)
						return "(out of range)";
					else if(enumValues[value - 1].name)
						return enumValues[value - 1].name;
					else
						return enumValues[value - 1].text;
				}
				else
					return QString::number(value);
			}
			else if(builtin == BuiltinFlexString)
				return *static_cast<const QString*>(handle);
			else
				return QString();
			switch(builtin) {
				case BuiltinFlexString: return *static_cast<const QString*>(handle);
				default: return QString();
			}
		}

		void fromstring(void *handle, const QString &value) const {

		}

		bool stringPut(void *handle, const QString &value) const {
			switch(builtin) {
				case BuiltinFlexString: *static_cast<QString*>(handle) = value; return true;
				default: return false;
			}
		}

		bool uintPut(void *handle, quint64 value) const {
			switch(builtin) {
				case BuiltinU8: *static_cast<quint8*>(handle) = value; return true;
				case BuiltinU16: *static_cast<quint16*>(handle) = value; return true;
				case BuiltinU32: *static_cast<quint32*>(handle) = value; return true;
				case BuiltinU64: *static_cast<quint64*>(handle) = value; return true;
				default: return false;
			}
		}

		bool sintPut(void *handle, qint64 value) const {
			switch(builtin) {
				default: return false;
			}
		}

		quint64 uintGet(const void *handle) const {
			switch(builtin) {
				case BuiltinU8: return *static_cast<const quint8*>(handle);
				case BuiltinU16: return *static_cast<const quint16*>(handle);
				case BuiltinU32: return *static_cast<const quint32*>(handle);
				case BuiltinU64: return *static_cast<const quint64*>(handle);
				default: return 0;
			}
		}

		bool isString() const {
			return builtin == BuiltinFlexString;
		}

		bool isEnum() const {
			return nEnumValues;
		}

		bool isInteger() const {
			return builtin == BuiltinU8 || builtin == BuiltinU16 || builtin == BuiltinU32 || builtin == BuiltinU64;
		}

		bool isUnsigned() const {
			return builtin == BuiltinU8 || builtin == BuiltinU16 || builtin == BuiltinU32 || builtin == BuiltinU64;
		}

		bool isSigned() const {
			return false;
		}

		bool equal(const void *a, const void *b) const {
			switch(builtin) {
				case BuiltinU8:
				case BuiltinU16:
				case BuiltinU32:
				case BuiltinU64:
				case BuiltinBoolean:
					return !memcmp(a, b, size);
				case BuiltinFlexString:
					return *static_cast<const QString*>(a) == *static_cast<const QString*>(b);
				default:
					return false;
			}
		}
	};

	struct DeclField {
		const char *name;
		const DeclType *type;
		const DeclKey *key; //if != null, the field is a fixed value to identify the key
		size_t index; //index of field excluding parent keys
		size_t globalidx; //index of field including all parent keys
		size_t offset;
	};

	struct DeclKey {
		const char *name;
		const char *fullname;
		const DeclKey *parent;
		size_t index; //child index
		size_t offset; //offset in binary key blob, where key starts
		bool abstract; //if key has children, it is abstract. abstract keys ALWAYS denote a prefix.
		bool flexible;
		const DeclField *fields;
		size_t nFields;
		size_t size;
		const DeclKey **subKeys;
		size_t nSubKeys;
		const DeclValue *mapto = nullptr;

		const DeclField *findField(const QString &name) const {
			for(size_t i = 0; i < nFields; i++)
				if(fields[i].name == name)
					return fields + i;
			return nullptr;
		}
	};

	struct DeclVar {
		size_t parent; //index of first child entry belonging to this parent. therefore: relative child index = index in 'DeclValue::vars' - 'parent'
		Name name;
		size_t n;
		size_t offset; //offset relative to parent
		size_t size; //SINGLE ELEMENT size, NOT multiplied by 'n'
		union {
			const DeclType *type;
			const ValueAccessor *accessor;
		} u;
		size_t child = 0; //if == 0, u.type is valid; else u.accessor is valid; index in DeclValue::vars, where first child is located
		bool isrow = false; //if this declaration results from a named row, 'isrow' is true.
	};

	struct DeclRow {
		size_t offset; //number of rows before this row
		size_t n; //number of rows; == 0: number of rows is flexible
		size_t size; //minimum size of row data
		bool flexible; //is a single row flexible?
//		const DeclVar *vars; //slice of DeclValue::vars; for anonymous rows, vars[0..nVars-1].parent == 0; for named rows, vars[0..nVars-1].parent is child id of row name entry
		size_t vars; //index into enclosing DeclValue::vars array
		size_t nVars;
		bool split = false; //each entry of the referenced var becomes a separate row; nVars must be == 1 and vars->n must be != 1. also 'n' must be == vars->n
	};

	struct DeclValue {
		Name name;
		size_t size; //minimum number of rows
		bool flexible; //last row declaration has n == 0
		const DeclVar *vars;
		size_t nVars;
		const ValueAccessor *accessor;
		void *(*create)();
		void (*destroy)(void*);
		const DeclRow *rows;
		size_t nRows;

		const DeclVar *findVar(size_t parent, Name name) const {
			if(parent >= nVars)
				return nullptr;
			const DeclVar *end = vars + nVars;
			for(const DeclVar *cur = vars + parent; cur != end && cur->parent == parent; cur++)
				if(cur->name == name)
					return cur;
			return nullptr;
		}

		size_t countVar(size_t parent) const {
			if(parent >= nVars)
				return 0;
			const DeclVar *cur;
			const DeclVar *end = vars + nVars;
			for(cur = vars + parent; cur != end && cur->parent == parent; cur++);
			return cur - vars - parent;
		}
	};

	struct DeclMeta {
		const char *name;
		const DeclKey **keys;
		size_t nKeys;
		const DeclKey **rootKeys;
		size_t nRootKeys;
		const DeclValue **values;
		size_t nValues;
		const DeclType **types;
		size_t nTypes;
		const DeclType **idTypes; //this are the types used by database acquire(). each of them has an independent counter.
		size_t nIdTypes;
		const DeclType *keyType; //type to store key id

		const DeclKey *findKey(const QString &fullname) const {
			for(size_t i = 0; i < nKeys; i++)
				if(keys[i]->fullname == fullname)
					return keys[i];
			return nullptr;
		}

		static const DeclMeta *version(const QString &version);
	};


	struct ValueAccessor {
		virtual size_t count() const = 0;
		virtual void *child(void *self, size_t id, size_t index) const = 0;
		virtual const void *constChild(const void *self, size_t id, size_t index) const = 0;
		virtual size_t size(const void *self, size_t id) const = 0;
		virtual bool insert(void *self, size_t id, size_t index, size_t n) const { return false; }
		virtual bool remove(void *self, size_t id, size_t index, size_t n) const { return false; }

		bool append(void *self, size_t id, size_t n = 1) const {
			return insert(self, id, size(self, id), n);
		}

		bool prepend(void *self, size_t id, size_t n = 1) const {
			return insert(self, id, 0, n);
		}
	};


	struct Value {
		struct ConstWalker {
			std::function<void(Name name, size_t n)> down;
			std::function<void()> up;
			std::function<void(size_t index, const ValueAccessor *accessor, const void *handle)> begin; //also called for value itself; 'name' DeclValue::name for root value
			std::function<void()> end;
			std::function<void(size_t index, const DeclType *type, const void *handle)> var;
		};

		struct Walker {
/*			std::function<void(Name name, size_t n)> down;
			std::function<void()> up;
			std::function<void(size_t index, const ValueAccessor *accessor, void *handle)> record; //also called for value itself; 'name' DeclValue::name for root value
			std::function<void(size_t index, const DeclType *type, void *handle)> var;*/
		};

		virtual ~Value() {}

		virtual const DeclValue *decl() const = 0;
//		virtual Value *clone() const = 0;
		template<typename T> T *to() {
			return dynamic_cast<T*>(this);
		}
		template<typename T> const T *to() const {
			return dynamic_cast<const T*>(this);
		}


		void constWalk(const ConstWalker &walker) const;
		void walk(const Walker &walker);

		bool operator==(const Value &other) const;
		bool operator!=(const Value &other) const;

		void dump() const;
	};

	struct Struct {
		template<typename T> static const T &KeyAux(const DeclKey *key) {
			return T::Default;
		}
	};

/*	class Cursor {
		public:
			Cursor(Value *value);
			void down(Name name, size_t index);
			size_t count(Name name);
			void stringPut(const QString &value);
			void uintPut(quint64 value);

		private:
			Value *_value;
	};*/

	//valid api assumption: value == 0 -> Enum::null (no index); index == 0 -> value == 1 (index = value - 1)
	template<typename T> struct EnumInfo {
		static constexpr size_t N = 0;
		static constexpr bool IsEnum = false;
		static constexpr const DeclType *DeclObject = nullptr;
	};

	template<typename T> struct TypeInfo {
		static constexpr bool IsType = false;
		static constexpr const DeclType *DeclObject = nullptr;
	};

	template<typename DECL, typename STRUCT> struct Mapping;
	template<typename DECL, typename STRUCT> struct MappedValue;
	template<typename T, typename STRUCT, size_t SIZE> struct MappingImpl;

	template<typename STRUCT> struct MappingImpl<DeclEnum, STRUCT, 0> {
		static constexpr size_t N = 0;

		static const STRUCT &get(const DeclType *type, quint64 value) {
			return STRUCT::Default;
		}

		template<typename ENUM> static const STRUCT &get(ENUM value) {
			return STRUCT::Default;
		}
	};

	template<typename STRUCT> struct MappingImpl<DeclType, STRUCT, 0> {
		static constexpr size_t N = 0;

		static const STRUCT &get(const DeclType *type) {
			return STRUCT::Default;
		}
	};

	template<typename STRUCT> struct MappingImpl<DeclKey, STRUCT, 0> {
		static constexpr size_t N = 0;

		static const STRUCT &get(const DeclKey *key) {
			return STRUCT::Default;
		}
	};


	template<typename STRUCT, size_t SIZE> struct MappingImpl<DeclEnum, STRUCT, SIZE> {
		static constexpr size_t N = SIZE;
		static const MappedValue<DeclEnum, STRUCT> mapping[N];

		static const STRUCT &get(const DeclType *type, quint64 value) {
			for(size_t i = 0; i < N; i++)
				if(mapping[i].type == type && mapping[i].value == value)
					return *mapping[i].aux;
			return STRUCT::Default;
		}

		template<typename ENUM> static const STRUCT &get(ENUM value) {
			return get(EnumInfo<ENUM>::DeclObject, EnumInfo<ENUM>::toIndex(value));
		}
	};

	template<typename STRUCT, size_t SIZE> struct MappingImpl<DeclType, STRUCT, SIZE> {
		static constexpr size_t N = SIZE;
		static const MappedValue<DeclType, STRUCT> mapping[N];

		static const STRUCT &get(const DeclType *type) {
			for(size_t i = 0; i < N; i++)
				if(mapping[i].type == type)
					return *mapping[i].aux;
			return STRUCT::Default;
		}
	};

	template<typename STRUCT, size_t SIZE> struct MappingImpl<DeclKey, STRUCT, SIZE> {
		static constexpr size_t N = SIZE;
		static const MappedValue<DeclKey, STRUCT> mapping[N];

		static const STRUCT &get(const DeclKey *key) {
			for(size_t i = 0; i < N; i++)
				if(mapping[i].key == key)
					return *mapping[i].aux;
			return STRUCT::Default;
		}
	};

	template<typename STRUCT> using EnumMapping = Mapping<DeclEnum, STRUCT>;
	template<typename STRUCT> using TypeMapping = Mapping<DeclType, STRUCT>;
	template<typename STRUCT> using KeyMapping = Mapping<DeclKey, STRUCT>;

	template<typename STRUCT> struct MappedValue<DeclEnum, STRUCT> {
		const DeclType *type;
		quint64 value;
		const STRUCT *aux;
	};

	template<typename STRUCT> struct MappedValue<DeclType, STRUCT> {
		const DeclType *type;
		const STRUCT *aux;
	};

	template<typename STRUCT> struct MappedValue<DeclKey, STRUCT> {
		const DeclKey *key;
		const STRUCT aux;
	};

	template<typename T, typename INFO, size_t Size> struct EnumInfoImpl {
		using Class = T;
		static constexpr size_t N = Size;
		static constexpr bool IsEnum = true;

		static constexpr quint64 toIndex(T value) {
			return static_cast<quint64>(value);
		}

		static T fromString(const QString &name) {
			QByteArray utf8 = name.toUtf8();
			for(size_t i = 0; i < N; i++)
				if(!strcmp(INFO::DeclObject->enumValues[i].name, utf8.constData()))
					return static_cast<T>(i);
			throw SpecException("enum value name not found");
		}

		//returns a VALUE
		static quint64 findString(const QString &name) {
			QByteArray utf8 = name.toUtf8();
			for(size_t i = 0; i < N; i++)
				if(!strcmp(INFO::DeclObject->enumValues[i].name, utf8.constData()))
					return i + 1;
			return 0;
		}

		static constexpr T fromIndex(quint64 value) {
			if(value >= N)
				throw SpecException("value too big for enum");
			return static_cast<T>(value);
		}

		static constexpr quint64 toInt(T value) {
			return static_cast<quint64>(value) + 1;
		}

		static constexpr T fromInt(quint64 value) {
			if(value == 0)
				throw SpecException("cannot convert null value to enum value");
			else if(value > N)
				throw SpecException("value too big for enum");
			return static_cast<T>(value - 1);
		}

		//can be used for iteration: 0 <= value < N
		static const char *name(quint64 index) {
			if(index >= N)
				throw SpecException("no such value for enum");
			return INFO::DeclObject->enumValues[index].name;
		}

		static const char *name(T value) {
			return name(toIndex(value));
		}

		//can be used for iteration: 0 <= value < N
		static const char *text(quint64 index) {
			if(index >= N)
				throw SpecException("no such value for enum");
			return INFO::DeclObject->enumValues[index].text;
		}

		static const char *text(T value) {
			return text(toIndex(value));
		}

		//can be used for iteration: 0 <= value < N
		//returns plural, if it exists, or text otherwise
		static const char *plural(quint64 index) {
			if(index >= N)
				throw SpecException("no such value for enum");
			if(INFO::DeclObject->enumValues[index].plural)
				return INFO::DeclObject->enumValues[index].plural;
			else
				return INFO::DeclObject->enumValues[index].text;
		}

		static const char *plural(T value) {
			return plural(toIndex(value));
		}
	};

	template<typename T, typename INFO> struct TypeInfoImpl {
		using Class = T;
		static constexpr bool IsType = true;
	};

	template<typename T> struct KeyInfo {
		static constexpr size_t N = 0;
		static constexpr bool IsKey = false;
		static constexpr const DeclKey *DeclObject = nullptr;
	};

	template<typename T, size_t Size> struct KeyInfoImpl {
		using Decl = T;
		static constexpr size_t N = Size;
		static constexpr bool IsKey = true;

		static constexpr quint64 toInt(T value) {
			return static_cast<quint64>(value);
		}

		static constexpr T fromInt(quint64 value) {
			if(value >= N)
				throw SpecException("value too big for enum");
			return static_cast<T>(value);
		}
	};

	template<typename T> static inline void setEnumNull(T *enm) {
		*enm = static_cast<T>(0);
	}

	template<typename T> static inline void setEnumByIndex(size_t index, T *enm) {
		*enm = static_cast<T>(index + 1);
	}

	template<typename T> static inline void setEnumByValue(size_t value, T *enm) {
		*enm = static_cast<T>(value);
	}

#include "../spec/spec-1.0.h"
}
#undef _IN_SPEC_H

