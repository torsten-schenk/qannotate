#include <QQueue>
#include <QSet>

#include "common/qXml.h"

enum {
	NoMulti,
	ListMulti, //non-indexed list. order is kept, but index is implied by position.
	SetMulti //non-indexed, non-sorted list. order is not kept and index is random (but within 0 to n-1).
};

struct Data {
	virtual ~Data() {}

	template<typename T> const T *to() const {
		return dynamic_cast<const T*>(this);
	}

	template<typename T> T *to() {
		return dynamic_cast<T*>(this);
	}

	virtual int id() const = 0;

	static inline const char *id2name(int id);
};

struct TypeData;

struct NestedData : Data {
	QString name;
	QString fullname;
	size_t index;

};

struct MemberData : Data {
	enum { Id = 1 };
	virtual int id() const override { return Id; };

	QString name;
	TypeData *type;
	QString value;

	size_t index;
};

struct AuxData;

struct StructData : NestedData {
	enum { Id = 2 };
	virtual int id() const override { return Id; };

	~StructData() {
		for(auto it : members)
			delete it;
	}

	QList<MemberData*> members;
	QMap<QString, MemberData*> byname;
	QMap<Data*, AuxData*> aux;
};

struct EnumData : Data {
	enum { Id = 3 };
	virtual int id() const override { return Id; };

	TypeData *owner;
	QString name;
	QString text;
	QString plural;
	size_t index;
};

struct KeyData;
struct FieldData;
struct ValueData;
struct VarData;

struct TypeData : NestedData {
	enum { Id = 4 };
	virtual int id() const override { return Id; };

	enum {
		Null, Boolean, U8, U16, U32, U64, FlexString, KeyReference, TypeReference, StructReference, User
	};

	struct Owner {
		Owner() {}
		Owner(KeyData *key, FieldData *field) : key(key), field(field) {}
		Owner(KeyData *key, ValueData *value, VarData *var) : key(key), value(value), var(var) {}

		KeyData *key;
		FieldData *field = nullptr;
		ValueData *value = nullptr;
		VarData *var = nullptr;
	};

	~TypeData() {
		for(auto it : enumdef)
			delete it;
	}

	int builtin() const {
		const TypeData *cur = this;
		while(cur) {
			if(cur->internal != User)
				return cur->internal;
			cur = cur->super;
		}
		return Null;
	}

	size_t size() const {
		const TypeData *cur = this;
		while(cur) {
			switch(cur->internal) {
				case Boolean: return 1;
				case U8: return 1;
				case U16: return 2;
				case U32: return 4;
				case U64: return 8;
				case User: break;
				default: return 0; //all other types cannot be serialized/are flexible
			}
			cur = cur->super;
		}
		return 0;
	}

	int internal;
	TypeData *super = nullptr;
	int enumType = NoMulti; //one of xMulti: if != NoMulti, the enum is dynamic and its values are stored in the database. specifies the ordering type.
	int multiType = NoMulti; //one of xMulti: multiple instances of this type may be created. specifies ordering type.
	size_t dynamicId = 0;

	QList<EnumData*> enumdef;
	QMap<QString, EnumData*> bynameEnum;
	QList<Owner> owners;
};

struct AuxData : Data {
	enum { Id = 5 };
	virtual int id() const override { return Id; };

	Data *source;
	StructData *strct;
	QMap<QString, QString> values;
};

struct FieldData : Data {
	enum { Id = 6 };
	virtual int id() const override { return Id; };

	QString name;
	TypeData *type;
	size_t index;
	size_t offset;
};

struct KeyData : NestedData {
	enum { Id = 7 };
	virtual int id() const override { return Id; };

	~KeyData() {
		for(auto it : subs)
			delete it;
		for(auto it : fields)
			delete it;
	}

	KeyData *parent = nullptr;
	bool flexible = false;
	size_t offset = 0; //offset in key blob, where this key starts
	size_t size = 0;

	QList<KeyData*> subs;
	QList<FieldData*> fields;
	QMap<QString, FieldData*> bynameField;
};

enum {
	NoRow, NewRow, ContinueRow, EachRow
};

struct RowData;
struct RecordData;

struct CellContainerData;

struct CellData : Data {
	size_t parentid;
	size_t childid;
/*	size_t flatid; //index in DeclValue::vars
	size_t varidx; //index*/

	CellContainerData *parent = nullptr;
	QString name;
	size_t n = 1;

	size_t offset = 0;
	size_t size = 0;
};

struct CellContainerData : CellData {
	size_t elemid; //each cell container with a name will be implemented as a struct. to flatten the resulting structure, each of those containers gets an id > 0

	QList<CellData*> subs; //all immediate children, anonymous and non-anonymous
	QList<CellData*> cells; //all named cells; can be immediate child or (in case child is anonymous) grand-child
	QMap<QString, CellData*> bynameCell;
};

struct VarData : CellData {
	enum { Id = 10 };
	virtual int id() const override { return Id; };

	TypeData *type;
};

struct RecordData : CellContainerData {
	enum { Id = 9 };
	virtual int id() const override { return Id; };

};

struct RowData : CellContainerData {
	enum { Id = 11 };
	virtual int id() const override { return Id; };

	bool flexible = false; //last cell is flexible?
};

struct ValueData : NestedData {
	enum { Id = 8 };
	virtual int id() const override { return Id; };

	bool flexible = false; //last row is flexible?

	CellData *bypathCell(const QStringList &path) {
		QMap<QString, CellData*> *map = &bynameCell;
		CellData *cell = nullptr;
		for(auto it : path) {
			if(!map)
				return nullptr;
			cell = map->value(it);
			if(!cell)
				return nullptr;
			CellContainerData *container = dynamic_cast<CellContainerData*>(cell);
			if(container)
				map = &container->bynameCell;
			else
				map = nullptr;
		}
		return cell;
	}

	QList<CellData*> flat; //all cells, which are x-child of this value
	QList<RowData*> rows; //all rows, anonymous and non-anonymous
	QList<CellData*> cells; //all root cells; by definition, a root cell MUST have a name. so the first cell in the tree with a name is a root cell. this can be a row or some cell which is located deeper in the tree.
	QMap<QString, CellData*> bynameCell;
};

const char *Data::id2name(int id) {
	switch(id) {
		case KeyData::Id: return "key";
		case FieldData::Id: return "field";
		case AuxData::Id: return "aux";
		case TypeData::Id: return "type";
		case EnumData::Id: return "enum";
		case StructData::Id: return "struct";
		case MemberData::Id: return "member";
		default: return "(unknown)";
	}
}

class Model {
	public:
		using Exception = XmlModelException;

		Model() : _nextDynamicId(1), _finished(false) {
			addType(QStringList(), "boolean", TypeData::Boolean);
			addType(QStringList(), "u8", TypeData::U8);
			addType(QStringList(), "u16", TypeData::U16);
			addType(QStringList(), "u32", TypeData::U32);
			addType(QStringList(), "u64", TypeData::U64);
			addType(QStringList(), "flex-string", TypeData::FlexString);
			addType(QStringList(), "key-reference", TypeData::KeyReference);
			addType(QStringList(), "type-reference", TypeData::TypeReference);
			addType(QStringList(), "struct-reference", TypeData::StructReference);
		}

		~Model() {
			for(auto it : _keys)
				delete it;
			for(auto it : _values)
				delete it;
			for(auto it : _types)
				delete it;
			for(auto it : _structs)
				delete it;
			for(auto it : _aux)
				delete it;
		}

		void dump() const {
			printf("Global: %d\n", _byname.size());
			for(auto it : _byname)
				printf("  %s -> %s [%zu]\n", qPrintable(it->fullname), Data::id2name(it->id()), it->index);

			printf("\nKeys: %d\n", _keys.size());
			for(auto it : _keys)
				dumpKey(it);

			printf("\nValues: %d\n", _values.size());
			for(auto it : _values)
				dumpValue(it);

			printf("\nStructs: %d\n", _structs.size());
			for(auto it : _structs)
				dumpStruct(it);

			printf("\nTypes: %d\n", _types.size());
			for(auto it : _types)
				dumpType(it);

			printf("Aux: %d\n", _aux.size());
			for(auto it : _aux)
				dumpAux(it);
		}

		void setVersion(const QString &version) {
			_version = version;
		}

		QString version() const {
			return _version;
		}

		const QList<TypeData*> &types() const {
			return _types;
		}

		const QList<StructData*> &structs() const {
			return _structs;
		}

		const QList<AuxData*> &aux() const {
			return _aux;
		}

		const QList<KeyData*> &keys() const {
			return _keys;
		}

		const QList<ValueData*> &values() const {
			return _values;
		}

		const QSet<QString> &names() const {
			return _names;
		}

		const QList<CellContainerData*> namedCellContainers() const {
			return _namedCellContainers;
		}

		size_t maxKeyDepth() const {
			return _maxKeyDepth;
		}

		template<typename T> T *get(QStringList ns, const QString &name) const {
			T *result = nullptr;
			QString fullname = ns.join(wordsep);
			if(fullname.isEmpty())
				fullname = name;
			else
				fullname += wordsep + name;
			result = _byname.value(fullname)->to<T>();
			if(!result)
				throw Exception(QString("missing or unexpected '%1'").arg(name));
			return result;
		}

		template<typename T> T *lookup(QStringList ns, const QString &name) const {
			T *result = nullptr;
			while(!ns.isEmpty()) {
				QString fullname = ns.join(wordsep) + wordsep + name;
				result = _byname.value(fullname)->to<T>();
				if(result)
					return result;
				ns.removeLast();
			}
			result = _byname.value(name)->to<T>();
			if(!result)
				throw Exception(QString("missing or unexpected '%1'").arg(name));
			return result;
		}

		ValueData *mapto(Data *source) {
			return _mapto.value(source);
		}

		TypeData *addType(const QStringList &ns, const QString &name, int internal = TypeData::User) {
			TypeData *data = add<TypeData>(ns, name, _types);
			data->internal = internal;
			return data;
		}

		StructData *addStruct(const QStringList &ns, const QString &name) {
			return add<StructData>(ns, name, _structs);
		}

		KeyData *addKey(const QStringList &ns, QString name, KeyData *parent = nullptr) {
			TypeData *pretype = lookup<TypeData>(QStringList(), "key-type");
			if(!pretype)
				throw Exception("type 'key-type' not declared; required before any key can be specified.");
			else if(!pretype->size())
				throw Exception("type 'key-type' must not be flexible.");
			else if(parent && parent->flexible && parent->subs.isEmpty())
				throw Exception("parent key must not be flexible.");

			KeyData *data;
			if(parent) {
				if(name.at(0) == wordsep)
					name = parent->name + name;
				else if(name.at(name.size() - 1) == wordsep)
					name = name + parent->name;
				data = add<KeyData>(ns, name, parent->subs);
				data->offset = parent->size;
				data->size = parent->size;
				parent->flexible = true;
			}
			else
				data = add<KeyData>(ns, name, _keys);
			data->size += pretype->size();
			data->parent = parent;
			return data;
		}

		ValueData *addValue(const QStringList &ns, QString name) {
			ValueData *data = add<ValueData>(ns, name, _values);
			return data;
		}

		RowData *addRow(ValueData *parent, const QString &name, size_t n) {
			if(parent->flexible)
				throw Exception("cannot append row to flexible value");
			else if(!name.isEmpty() && parent->bynameCell.contains(name))
				throw Exception("cannot append row: duplicate name");
			if(!n)
				parent->flexible = true;
			RowData *data = new RowData();
			data->name = name;
			data->n = n;
			parent->rows.append(data);
			if(!name.isEmpty()) {
				parent->bynameCell.insert(name, data);
				parent->cells.append(data);
				_namedCellContainers.prepend(data);
				data->elemid = _namedCellContainers.size();
			}
			if(!name.isEmpty())
				parent->flat.append(data);
			return data;
		}

		RecordData *addRecord(ValueData *value, RowData *row, CellContainerData *parent, const QString &name, size_t n) {
			if(name.isEmpty())
				throw Exception("invalid record: missing name");
			else if(!n && row != parent)
				throw Exception("flexible record only allowed at topmost level of row");
			else if(row == parent && row->flexible)
				throw Exception("cannot append record to flexible row");
			else if(row == parent && row->name.isEmpty() && row->n != 1) {
				if(row->subs.size() > 0)
					throw Exception("anonymous row with multiplicity != 1 must not have multiple children");
				else if(n != 1)
					throw Exception("anonymous row with multiplicity != 1 must not have children with multiplicity != 1");
			}

			RecordData *data = new RecordData();
			data->name = name;
			data->n = n;
			data->parent = parent;
			data->offset = parent->size;
			parent->subs.append(data);
			if(parent == row && row->name.isEmpty()) {
				if(value->bynameCell.contains(name))
					throw Exception("cannot add record: duplicate name");
				value->cells.append(data);
				value->bynameCell.insert(name, data);
			}
			else if(parent->name.isEmpty()) {
				if(parent->parent->name.isEmpty())
					throw Exception("internal error: multiple consecutive anonymous parents");
				if(parent->parent->bynameCell.contains(name))
					throw Exception("cannot add record: duplicate name");
				parent->parent->cells.append(data);
				parent->parent->bynameCell.insert(name, data);
			}
			else {
				if(parent->bynameCell.contains(name))
					throw Exception("cannot add record: duplicate name");
				parent->cells.append(data);
				parent->bynameCell.insert(name, data);
			}
			_namedCellContainers.prepend(data);
			data->elemid = _namedCellContainers.size();
			if(!n)
				row->flexible = true;
			value->flat.append(data);
			return data;
		}

		VarData *addVar(ValueData *value, RowData *row, CellContainerData *parent, const QString &name, size_t n, TypeData *type) {
			size_t size = type->size();
			if(name.isEmpty())
				throw Exception("var must have a name");
			else if(!n && row != parent)
				throw Exception("flexible var only allowed at topmost level of row");
			else if(!size && row != parent)
				throw Exception("flexible var only allowed at topmost level of row");
			else if(row == parent && row->flexible)
				throw Exception("cannot append var to flexible row");
			else if(row == parent && row->name.isEmpty() && row->n != 1) {
				if(row->subs.size() > 0)
					throw Exception("anonymous row with multiplicity != 1 must not have multiple children");
				else if(n != 1)
					throw Exception("anonymous row with multiplicity != 1 must not have children with multiplicity != 1");
			}
			VarData *data = new VarData();
			data->name = name;
			data->n = n;
			data->parent = parent;
			data->type = type;
			parent->subs.append(data);
			if(parent == row && row->name.isEmpty()) {
				if(value->bynameCell.contains(name))
					throw Exception("cannot add var: duplicate name");
				value->cells.append(data);
				value->bynameCell.insert(name, data);
			}
			else if(parent->name.isEmpty()) {
				if(!parent->parent)
					throw Exception("internal error: null parent parent and row != parent");
				else if(parent->parent->name.isEmpty())
					throw Exception("internal error: multiple consecutive anonymous parents");
				if(parent->parent->bynameCell.contains(name))
					throw Exception("cannot add var: duplicate name");
				parent->parent->cells.append(data);
				parent->parent->bynameCell.insert(name, data);
			}
			else {
				if(parent->bynameCell.contains(name))
					throw Exception("cannot add var: duplicate name");
				parent->cells.append(data);
				parent->bynameCell.insert(name, data);
			}

			if(!n || !size)
				row->flexible = true;
			value->flat.append(data);

			data->offset = parent->size;
			
			CellData *cur = data;
			while(cur) {
				cur->size += size;
				size *= cur->n;
				cur = cur->parent;
			}

			return data;
		}

		EnumData *addEnum(TypeData *type, const QString &name) {
			if(type->bynameEnum.contains(name))
				throw Exception("enum name already defined for type");
			EnumData *data = new EnumData();
			data->owner = type;
			data->name = name;
			data->index = type->enumdef.size();
			type->enumdef.append(data);
			type->bynameEnum.insert(name, data);
			return data;
		}

		void addMapto(Data *source, const QStringList &ns, const QString &target) {
			if(!target.isEmpty()) {
				ValueData *mapto = lookup<ValueData>(ns, target);
				if(!mapto)
					throw Exception(QString("no such value '%1'").arg(target));
				_mapto.insert(source, mapto);
			}
		}

		AuxData *addAux(Data *source, StructData *strct) {
			if(strct->aux.contains(source))
				throw Exception("aux already defined for object");
			AuxData *data = new AuxData();
			data->source = source;
			data->strct = strct;
			_aux.append(data);
			strct->aux.insert(source, data);
			return data;
		}

		FieldData *addField(KeyData *key, const QString &name, TypeData *type) {
			if(key->flexible)
				throw Exception("no more fields allowed after flexible field");
			KeyData *cur = key;
			while(cur) {
				if(cur->bynameField.contains(name))
					throw Exception("duplicate field name in key");
				cur = cur->parent;
			}
			FieldData *data = new FieldData();
			size_t size = type->size();
			data->name = name;
			data->index = key->fields.size() + 1;
			data->offset = key->size;
			data->type = type;
			key->fields.append(data);
			key->bynameField.insert(name, data);
			key->size += size;
			key->flexible = !size;
			return data;
		}

		size_t acquireDynamicId() {
			return _nextDynamicId++;
		}

		void finish() {
			if(_finished)
				return;

			_maxKeyDepth = 0;
			_names.clear();

			for(auto it : _structs) {
				_names.insert(it->name);
				_names.insert(it->fullname);
				for(auto mit : it->members)
					_names.insert(mit->name);
			}

			for(auto it : _types) {
				_names.insert(it->name);
				_names.insert(it->fullname);
				for(auto eit : it->enumdef)
					_names.insert(eit->name);
			}

			for(auto it : _values) {
				_names.insert(it->name);
				_names.insert(it->fullname);
				for(auto subit : it->flat)
					_names.insert(subit->name);
			}

			QQueue<KeyData*> keys;
			for(auto it : _keys)
				keys.enqueue(it);
			while(!keys.isEmpty()) {
				KeyData *cur = keys.dequeue();
				size_t depth = 0;
				for(KeyData *it = cur; it; it = it->parent)
					depth++;
				if(depth > _maxKeyDepth)
					_maxKeyDepth = depth;

				_names.insert(cur->name);
				_names.insert(cur->fullname);
				for(auto it : cur->fields)
					_names.insert(it->name);
				for(auto it : cur->subs)
					keys.enqueue(it);
			}

/*			for(auto it : _keys) {
				it->index = 0;
				mkidxDeclField(1, it);
			}*/

			for(auto it : _values) {
				size_t total = 0;
				mkidDeclVar(total, it->cells);
			}

			_finished = true;
		}

	private:
		void mkidDeclVar(size_t &total, const QList<CellData*> &cells) {
			if(cells.isEmpty())
				return;
			if(total > 0) //total == 0: we are at the root node (i.e. the value). the root node always has id 0, so nothing to collect here.
				cells.first()->parent->parentid = total;
			total += cells.size();
			size_t index = 0;
			for(auto it : cells) {
				it->childid = index++;
				CellContainerData *container = it->to<CellContainerData>();
				if(container)
					mkidDeclVar(total, container->cells);
			}
		}

		void mkidxDeclField(size_t offset, KeyData *key) {
			for(auto it : key->fields)
				it->index = offset++;
			for(auto it : key->subs) {
				it->index = offset;
				mkidxDeclField(offset + 1, it);
			}
		}

		template<typename T> T *add(const QStringList &ns, const QString &name, QList<T*> &list) {
			QString fullname;
			fullname = ns.join(wordsep);
			if(fullname.isEmpty())
				fullname = name;
			else
				fullname += wordsep + name;
			if(_byname.contains(fullname))
				throw Exception(QString("object named '%1' already exists").arg(fullname));

			T *data = new T();
			data->name = name;
			data->fullname = fullname;

			data->index = list.size();
			_byname.insert(data->fullname, data);
			list.append(data);
			return data;
		}

		void dumpStruct(StructData *strct, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");
			printf("%s\n", qPrintable(strct->fullname));
			for(auto it : strct->members)
				dumpMember(it, indent);
		}

		void dumpKey(KeyData *key, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");
			printf("%s: size=%zu flexible=%c\n", qPrintable(key->fullname), key->size, key->flexible ? 'y' : 'n');
			for(auto it : key->fields)
				dumpField(it, indent);
			for(auto it : key->subs)
				dumpKey(it, indent + 1);
		}

		void dumpValue(ValueData *value, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");

		}

		void dumpField(FieldData *field, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");
			printf("- %s: type=%s offset=%zu\n", qPrintable(field->name), qPrintable(field->type->fullname), field->offset);
		}

		void dumpMember(MemberData *member, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");
			printf("- %s: type=%s value=%s\n", qPrintable(member->name), qPrintable(member->type->fullname), qPrintable(member->value));
		}

		void dumpType(TypeData *type, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");
			printf("%s: super=%s internal=%d enum=%d multi=%d\n", qPrintable(type->fullname), type->super ? qPrintable(type->super->fullname) : "(none)", type->internal, type->enumType, type->multiType);
			for(auto it : type->enumdef)
				dumpEnum(it, indent);
		}

		void dumpEnum(EnumData *enm, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");
			printf("- %s\n", qPrintable(enm->name));
		}

		void dumpAux(AuxData *aux, int indent = 1) const {
			for(int i = 0; i < indent; i++)
				printf("  ");
			printf("source=");
			printData(aux->source);
			printf(" struct=%s\n", qPrintable(aux->strct->fullname));
			for(auto it = aux->values.begin(); it != aux->values.end(); ++it) {
				for(int i = 0; i < indent; i++)
					printf("  ");
				printf("- %s -> %s\n", qPrintable(it.key()), qPrintable(it.value()));
			}
		}

		void printData(Data *data) const {
			switch(data->id()) {
				case TypeData::Id: printf("[type %s]", qPrintable(data->to<TypeData>()->fullname)); return;
				case StructData::Id: printf("[struct %s]", qPrintable(data->to<StructData>()->fullname)); return;
				case KeyData::Id: printf("[key %s]", qPrintable(data->to<KeyData>()->fullname)); return;
				case AuxData::Id: printf("[aux %p]", data); return;
				case EnumData::Id: printf("[enum %p]", data); return;
				case MemberData::Id: printf("[member %p]", data); return;
				case FieldData::Id: printf("[field %p]", data); return;
			}
		}

		QString _version;

		QList<StructData*> _structs;
		QList<TypeData*> _types;
		QList<KeyData*> _keys;
		QList<ValueData*> _values;
		QList<AuxData*> _aux;
		QMap<QString, NestedData*> _byname;
		QMap<Data*, ValueData*> _mapto;

		QList<CellContainerData*> _namedCellContainers;

		QSet<QString> _names;
		size_t _maxKeyDepth;

		size_t _nextDynamicId;
		bool _finished;
};

