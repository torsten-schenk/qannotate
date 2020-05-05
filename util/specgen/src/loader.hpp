class Loader : public XmlModelHandler {
	public:
		Loader(Model *model) : _model(model) {
			int root = add(cb(this, &Loader::root), cb(this, &Loader::finish));
			int type = add(cb(this, &Loader::type));
			int ns = add(cb(this, &Loader::ns));
			int dynid = add(cb(this, &Loader::dynid));
			int owner = add(cb(this, &Loader::owner));
			int ref = add(cb(this, &Loader::ref));
			int strct = add(cb(this, &Loader::strct));
			int aux = add(cb(this, &Loader::aux));
			int enm = add(cb(this, &Loader::enm));
			int key = add(cb(this, &Loader::key));
			int value = add(cb(this, &Loader::value));
			int row = add(cb(this, &Loader::row));
			int record = add(cb(this, &Loader::record));
			int var = add(cb(this, &Loader::var));
			int field = add(cb(this, &Loader::field));
			int member = add(cb(this, &Loader::member));

			link(0, "db-spec", root);
			link(root, "key", key);
			link(root, "value", value);
			link(root, "namespace", ns);
			link(root, "type", type);
			link(root, "dynid", dynid);
			link(root, "struct", strct);
			link(ns, "key", key);
			link(ns, "value", value);
			link(ns, "namespace", ns);
			link(ns, "type", type);
			link(ns, "dynid", dynid);
			link(ns, "struct", strct);
			link(type, "enum", enm);
			link(key, "field", field);
			link(key, "key", key);
			link(dynid, "owner", owner);
			link(owner, "ref", ref);
			link(value, "row", row);
			link(row, "record", record);
			link(record, "record", record);
			link(record, "var", var);
			link(row, "var", var);
			link(type, "aux", aux);
			link(enm, "aux", aux);
			link(strct, "member", member);
		}

		Model *model() const {
			return _model;
		}

	protected:
		struct Namespace : Frame {
			QString name;
			QString fullname;
			QStringList path;
		};

		struct Type : Frame {
			Type(TypeData *data) : data(data) {}

			TypeData *data;
		};

		struct Struct : Frame {
			Struct(StructData *data) : data(data) {}

			StructData *data;
		};

		struct Key : Frame {
			Key(KeyData *data) : data(data) {}

			KeyData *data;
		};

		struct Value : Frame {
			Value(ValueData *data) : data(data) {}

			ValueData *data;
		};

		struct Row : Frame {
			Row(RowData *data) : data(data) {}

			RowData *data;
		};

		struct Record : Frame {
			Record(RecordData *data) : data(data) {}

			RecordData *data;
		};

		struct Enum : Frame {
			Enum(EnumData *data) : data(data) {}
			EnumData *data;
		};

		virtual void root(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void finish(XmlModelHandlerFrame *frame) = 0;
		virtual void ns(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void type(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void enm(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void member(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) = 0;
		virtual void field(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) = 0;
		virtual void aux(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) = 0;
		virtual void key(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void dynid(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void owner(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void ref(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) = 0;
		virtual void value(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void row(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void record(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;
		virtual void var(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) = 0;
		virtual void strct(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) = 0;

		static int parseMulti(const QString &name) {
			if(name.isEmpty() || name == "none")
				return NoMulti;
			else if(name == "list")
				return ListMulti;
			else if(name == "set")
				return SetMulti;
			else
				throw Exception(QString("invalid value for multiplicity ordering: '%1'").arg(name));
		}

		Model *_model;
};

class DirectLoader : public Loader {
	public:
		DirectLoader(Model *model) : Loader(model) {}

	protected:
		void root(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			QString attrVersion = attr.value("version");
			if(attrVersion.isEmpty())
				throw Exception("missing 'version' attribute");
			frame = new Namespace();
			model()->setVersion(attrVersion);
		}

		void finish(XmlModelHandlerFrame *frame) {
			model()->finish();
		}

		void ns(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *parent = frame->topmost<Namespace>();
			Namespace *child = new Namespace();
			QString attrName = attr.value("name");
			if(attrName.isEmpty())
				throw Exception("missing 'name' attribute");
			child->name = attrName;
			if(parent->fullname.isEmpty())
				child->fullname = attrName;
			else
				child->fullname = parent->fullname + wordsep + attrName;
			child->path = parent->path;
			child->path.append(attrName);
			frame = child;
		}

		void type(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			TypeData *super = nullptr;
			QString attrName = attr.value("name");
			QString attrExtend = attr.value("extend");
			QString attrMapto = attr.value("mapto");
			int multiType = parseMulti(attr.value("multi"));
			int enumType = parseMulti(attr.value("enum"));
			if(attrName.isEmpty())
				throw Exception("missing 'name' attribute");
			else if(attrExtend.isEmpty())
				throw Exception("missing 'extend' attribute");
			super = model()->lookup<TypeData>(ns->path, attrExtend);
			if(!super)
				throw Exception(QString("no such type '%1' in namespace '%2'").arg(attrExtend).arg(ns->fullname));
			TypeData *data = model()->addType(ns->path, attrName);
			data->super = super;
			data->enumType = enumType;
			data->multiType = multiType;

			frame = new Type(data);
			model()->addMapto(data, ns->path, attrMapto);
		}

		void enm(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			Type *type = frame->topmost<Type>();
			QString attrName = attr.value("name");
			QString attrText = attr.value("text");
			QString attrPlural = attr.value("plural");
			QString attrMapto = attr.value("mapto");
			if(attrName.isEmpty())
				throw Exception("missing 'name' attribute");
			EnumData *data = model()->addEnum(type->data, attrName);
			data->text = attrText;
			data->plural = attrPlural;
			model()->addMapto(data, ns->path, attrMapto);
			frame = new Enum(data);
		}

		void member(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			Struct *strct = frame->topmost<Struct>();
			QString attrName = attr.value("name");
			QString attrType = attr.value("type");
			QString attrDefault = attr.value("default");
			if(attrName.isEmpty())
				throw Exception("missing 'name' attribute");
			else if(strct->data->byname.contains(attrName))
				throw Exception(QString("struct already contains an member named '%1'").arg(attrName));
			TypeData *type = model()->lookup<TypeData>(ns->path, attrType);
			if(!type)
				throw Exception(QString("no such type '%1' in namespace '%2'").arg(attrType).arg(ns->fullname));
			MemberData *data = new MemberData();
			data->name = attrName;
			data->value = attrDefault;
			data->type = type;
			data->index = strct->data->members.size();
			strct->data->byname.insert(attrName, data);
			strct->data->members.append(data);
		}

		void field(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			Key *key = frame->topmost<Key>();
			QString attrName = attr.value("name");
			QString attrType = attr.value("type");
			if(attrName.isEmpty())
				attrName = attrType;
			else if(attrType.isEmpty())
				attrType = attrName;
			if(attrName.isEmpty())
				throw Exception("missing one of 'name' or 'type' attribute");
			TypeData *type = model()->lookup<TypeData>(ns->path, attrType);
			if(!type)
				throw Exception(QString("no such type '%1' in namespace '%2'").arg(attrType).arg(ns->fullname));

			model()->addField(key->data, attrName, type);
		}

		void aux(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			QString attrStruct = attr.value("struct");
			if(attrStruct.isEmpty())
				throw Exception("missing 'struct' attribute");
			Data *source;
			if(frame->to<Enum>(false))
				source = frame->to<Enum>()->data;
			else if(frame->to<Type>(false))
				source = frame->to<Type>()->data;
			else
				throw Exception("error in aux(): source data type not handled");

			StructData *strct = model()->lookup<StructData>(ns->path, attrStruct);
			if(!strct)
				throw Exception(QString("no such struct '%1' in namespace '%2'").arg(attrStruct).arg(ns->fullname));

			AuxData *data = model()->addAux(source, strct);
			for(int i = 0; i < attr.count(); i++) {
				QString name = attr.qName(i);
				if(name == "struct")
					continue;
				QString value = attr.value(i);
				data->values.insert(name, value);
			}
		}

		void key(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			Key *parent = frame->topmost<Key>(false);
			QString attrMapto = attr.value("mapto");
			QString attrName = attr.value("name");
			if(attrName.isEmpty())
				throw Exception("missing 'name' attribute");
			KeyData *data = model()->addKey(ns->path, attrName, parent ? parent->data : nullptr);
			frame = new Key(data);
			model()->addMapto(data, ns->path, attrMapto);
		}

		void dynid(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			QString attrType = attr.value("type");
			if(attrType.isEmpty())
				throw Exception("missing 'type' attribute");
			TypeData *type = model()->lookup<TypeData>(ns->path, attrType);
			if(type->dynamicId)
				throw Exception("type already declared as dynamic");
			type->dynamicId = model()->acquireDynamicId();
			frame = new Type(type);
		}

		void owner(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			Type *type = frame->topmost<Type>();
			QString attrKey = attr.value("key");
			QString attrField = attr.value("field");
			QString attrValue = attr.value("value");
			QString attrVar = attr.value("var");
			if(attrKey.isEmpty())
				throw Exception("missing 'key' attribute");
			else if(!attrField.isEmpty() && (!attrValue.isEmpty() || !attrVar.isEmpty()))
				throw Exception("expected either 'field' attribute of both 'value' and 'var' attributes");
			else if(attrField.isEmpty() && (attrValue.isEmpty() || attrVar.isEmpty()))
				throw Exception("expected either 'field' attribute of both 'value' and 'var' attributes");
			KeyData *key = model()->lookup<KeyData>(ns->path, attrKey);
			if(!attrField.isEmpty()) {
				FieldData *field = key->bynameField.value(attrField);
				if(!field)
					throw Exception("no such field for given key");
				type->data->owners.append(TypeData::Owner(key, field));
			}
			else {
				ValueData *value = model()->lookup<ValueData>(ns->path, attrValue);
				VarData *var = value->bypathCell(attrVar.split("."))->to<VarData>();
				if(!var)
					throw Exception("no such var for given value");
				else if(var->type != type->data)
					throw Exception("given var does not have expected type");
				type->data->owners.append(TypeData::Owner(key, value, var));
			}
		}

		void ref(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) {
		}

		void value(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			QString attrName = attr.value("name");
			if(attrName.isEmpty())
				throw Exception("missing 'name' attribute");
			ValueData *value = model()->addValue(ns->path, attrName);
			frame = new Value(value);
		}

		void row(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Value *value = frame->topmost<Value>();
			QString attrN = attr.value("n");
			QString attrName = attr.value("name");
			size_t n;
			if(attrN.isEmpty())
				n = 1;
			else
				n = attrN.toULongLong();
			RowData *data = model()->addRow(value->data, attrName, n);
			frame = new Row(data);
		}

		void record(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Value *value = frame->topmost<Value>();
			Row *row = frame->topmost<Row>();
			Record *record = frame->topmost<Record>(false);
			QString attrN = attr.value("n");
			QString attrName = attr.value("name");
			size_t n;
			if(attrN.isEmpty())
				n = 1;
			else
				n = attrN.toULongLong();
			CellContainerData *parent;
			if(record)
				parent = record->data;
			else
				parent = row->data;
			RecordData *data = model()->addRecord(value->data, row->data, parent, attrName, n);
			frame = new Record(data);
		}

		void var(XmlModelHandlerFrame *frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			Value *value = frame->topmost<Value>();
			Row *row = frame->topmost<Row>();
			Record *record = frame->topmost<Record>(false);
			QString attrN = attr.value("n");
			QString attrName = attr.value("name");
			QString attrType = attr.value("type");
			size_t n;
			if(attrN.isEmpty())
				n = 1;
			else
				n = attrN.toULongLong();
			CellContainerData *parent;
			if(record)
				parent = record->data;
			else
				parent = row->data;
			TypeData *type = model()->lookup<TypeData>(ns->path, attrType);
			if(!type)
				throw Exception("no such type");
			model()->addVar(value->data, row->data, parent, attrName, n, type);
		}

		void strct(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			Namespace *ns = frame->topmost<Namespace>();
			QString attrName = attr.value("name");
			if(attrName.isEmpty())
				throw Exception("missing 'name' attribute");
			StructData *data = model()->addStruct(ns->path, attrName);
			frame = new Struct(data);
		}

};

