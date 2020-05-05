#include <QQueue>

class Generator {
	public:
		Generator(Model *model) : _model(model) {}

		Printer *printer() {
			return &_p;
		}

		bool generate(const QString &target) {
			genNames();

			QString ns = "v" + _model->version().replace(".", "_");
			_p.pr("namespace ").pr(ns).pr(" {").pni();
			genProto();
			_p.pr("}").upn();

			genInfo(ns);

			_p.pr("namespace ").pr(ns).pr(" {").pni();

			_p.pr("namespace anon_value {").pni();

			genAnonCell();

			_p.pr("}").upn();

			for(auto it : _model->types()) {
				genDeclType(it);
			}

			QQueue<KeyData*> keys;
			for(auto it : _model->keys())
				genDeclKey(it, keys);
			while(!keys.isEmpty()) {
				KeyData *key = keys.dequeue();
				genDeclKey(key, keys);
			}

			for(auto it : _model->values()) {
				genDeclValue(it);
			}

			for(auto it : _model->structs()) {
				genStruct(it);
			}

			genDeclMeta();
			_p.pr("}").upn();

			for(auto it : _model->structs()) {
				genStructMapping(ns, it);
			}
			
			QFile file(target);
			if(!file.open(QFile::ReadWrite | QFile::Truncate))
				return false;
			IoTarget io(&file);
			_p.stream(&io);
			return true;
		}

	private:
		void genNames() {
			_p.pr("//@maxKeyDepth ").pr(_model->maxKeyDepth()).pn();
			for(auto it : _model->names())
				_p.pr("//@name ").pr(it).pn();
		}

		void genProto() {
			for(auto it : _model->types()) {
//				if(it->enumdef.size() > 0)
				_p.pr("enum class ").pr(uppername(it->fullname)).pr(";").pn();
				_p.pr("extern const DeclType ").pr(lowername(it->fullname)).pr(";").pn();
			}

			QQueue<KeyData*> subs;
			for(auto it : _model->keys())
				subs.enqueue(it);
			while(!subs.isEmpty()) {
				KeyData *cur = subs.dequeue();
				_p.pr("enum class ").pr(uppername(cur->fullname)).pr(";").pn();
				_p.pr("extern const DeclKey ").pr(lowername(cur->fullname)).pr(";").pn();
				for(auto subit : cur->subs)
					subs.enqueue(subit);
			}

			for(auto it : _model->values()) {
				_p.pr("extern const DeclValue ").pr(lowername(it->fullname)).pr(";").pn();
			}
		}

		void genInfo(const QString &ns) {
			for(auto it : _model->types()) {
				if(it->enumdef.size() > 0) {
					//_p.pr("template<> struct EnumInfo<").pr(ns).pr("::").pr(uppername(it->fullname)).pr("> : EnumInfoImpl<").pr(ns).pr("::").pr(uppername(it->fullname)).pr(", ").pr(it->enumdef.size()).pr("> {").pni();
					_p.pr("template<> struct EnumInfo<").pr(ns).pr("::").pr(uppername(it->fullname)).pr("> : EnumInfoImpl<").pr(ns).pr("::").pr(uppername(it->fullname)).pr(", EnumInfo<").pr(ns).pr("::").pr(uppername(it->fullname)).pr(">, ").pr(it->enumdef.size()).pr("> {").pni();
					_p.pr("static constexpr const DeclType *DeclObject = &").pr(ns).pr("::").pr(lowername(it->fullname)).pr(";").pn();
					_p.pr("};").upn();
				}
				_p.pr("template<> struct TypeInfo<").pr(ns).pr("::").pr(uppername(it->fullname)).pr("> : TypeInfoImpl<").pr(ns).pr("::").pr(uppername(it->fullname)).pr(", TypeInfo<").pr(ns).pr("::").pr(uppername(it->fullname)).pr("> > {").pni();
				_p.pr("static constexpr const DeclType *DeclObject = &").pr(ns).pr("::").pr(lowername(it->fullname)).pr(";").pn();
				_p.pr("};").upn();
			}

			QQueue<KeyData*> subs;
			for(auto it : _model->keys())
				subs.enqueue(it);
			while(!subs.isEmpty()) {
				KeyData *cur = subs.dequeue();
				size_t nfield = 0;
				for(KeyData *it = cur; it; it = it->parent)
					nfield += 1 + it->fields.size();
				_p.pr("template<> struct KeyInfo<").pr(ns).pr("::").pr(uppername(cur->fullname)).pr("> : KeyInfoImpl<").pr(ns).pr("::").pr(uppername(cur->fullname)).pr(", ").pr(nfield).pr("> {").pni();
				_p.pr("static constexpr const DeclKey *DeclObject = &").pr(ns).pr("::").pr(lowername(cur->fullname)).pr(";").pn();
				_p.pr("};").upn();
				for(auto subit : cur->subs)
					subs.enqueue(subit);
			}
		}

		void genAnonCell() {
			for(auto it : _model->namedCellContainers()) {
				_p.pr("struct rec_").pr(it->elemid).pr(" {").pni();
				genCellStruct(it->cells);
				_p.pr("};").upn();
			}
			for(auto it : _model->values()) {
				_p.pr("struct value_").pr(uppername(it->fullname)).pr(" : Value {").pni();
				_p.pr("virtual const DeclValue *decl() const override { return &").pr(lowername(it->fullname)).pr("; }").pn();
				genCellStruct(it->cells);
				_p.pr("};").upn();
			}

			for(auto it : _model->namedCellContainers()) {
				_p.pr("struct Rec_").pr(it->elemid).pr(" : ValueAccessor {").pni();
				genCellAccessor(QString("rec_%1").arg(it->elemid), it->cells);
				_p.pr("};").upn();
				_p.pr("extern Rec_").pr(it->elemid).pr(" accessor_rec_").pr(it->elemid).pr(";").pn();
				_p.pr("#ifdef IMPLEMENTATION").opn(0);
				_p.pr("Rec_").pr(it->elemid).pr(" accessor_rec_").pr(it->elemid).pr(";").pn();
				_p.pr("#endif").opn(0);
			}
			for(auto it : _model->values()) {
				_p.pr("void *create_").pr(uppername(it->fullname)).pr("();").pn();
				_p.pr("void destroy_").pr(uppername(it->fullname)).pr("(void*);").pn();
				_p.pr("struct Value_").pr(uppername(it->fullname)).pr(" : ValueAccessor {").pni();
				genCellAccessor("value_" + uppername(it->fullname), it->cells);
				_p.pr("};").upn();
				_p.pr("extern Value_").pr(uppername(it->fullname)).pr(" accessor_value_").pr(uppername(it->fullname)).pr(";").pn();
				_p.pr("#ifdef IMPLEMENTATION").opn(0);
				_p.pr("void *create_").pr(uppername(it->fullname)).pr("() {").pni();
				_p.pr("return new value_").pr(uppername(it->fullname)).pr(";").pn();
				_p.pr("}").upn();
				_p.pr("void destroy_").pr(uppername(it->fullname)).pr("(void *self) {").pni();
				_p.pr("delete reinterpret_cast<value_").pr(uppername(it->fullname)).pr("*>(self);").pn();
				_p.pr("}").upn();
				_p.pr("Value_").pr(uppername(it->fullname)).pr(" accessor_value_").pr(uppername(it->fullname)).pr(";").pn();
				_p.pr("#endif").opn(0);
			}
		}

		void genCellAccessor(const QString &sname, const QList<CellData*> &cells) {
			size_t index;
			_p.pr("virtual size_t count() const override { return ").pr(cells.size()).pr("; }").pn();

			_p.pr("virtual const void *constChild(const void *self_, size_t id, size_t index) const override {").pni();
			_p.pr("const ").pr(sname).pr(" *self = reinterpret_cast<const ").pr(sname).pr("*>(self_);").pn();
			_p.pr("switch(id) {").pni();
			index = 0;
			for(auto it : cells) {
				size_t n = it->n;
				if(n == 1 && it->parent && it->parent->name.isEmpty())
					n = it->parent->n;
				_p.pr("case ").pr(index).pr(": return ");
				if(n == 1)
					_p.pr("&self->").pr(lowername(it->name));
				else if(n > 1)
					_p.pr("self->").pr(lowername(it->name)).pr(" + index");
				else
					_p.pr("&self->").pr(lowername(it->name)).pr("[index]");
				_p.pr(";").pn();
				index++;
			}
			_p.pr("default: return nullptr;").pn();
			_p.pr("}").upn();
			_p.pr("}").upn();

			_p.pr("virtual void *child(void *self_, size_t id, size_t index) const override {").pni();
			_p.pr(sname).pr(" *self = reinterpret_cast<").pr(sname).pr("*>(self_);").pn();
			_p.pr("switch(id) {").pni();
			index = 0;
			for(auto it : cells) {
				size_t n = it->n;
				if(n == 1 && it->parent && it->parent->name.isEmpty())
					n = it->parent->n;
				_p.pr("case ").pr(index).pr(": return ");
				if(n == 1)
					_p.pr("&self->").pr(lowername(it->name));
				else if(n > 1)
					_p.pr("self->").pr(lowername(it->name)).pr(" + index");
				else
					_p.pr("&self->").pr(lowername(it->name)).pr("[index]");
				_p.pr(";").pn();
				index++;
			}
			_p.pr("default: return nullptr;").pn();
			_p.pr("}").upn();
			_p.pr("}").upn();

			_p.pr("virtual size_t size(const void *self_, size_t id) const override {").pni();
			bool hasDynamic = false;
			for(auto it : cells) {
				size_t n = it->n;
				if(n == 1 && it->parent && it->parent->name.isEmpty())
					n = it->parent->n;
				if(n == 0)
					hasDynamic = true;
			}
			if(hasDynamic)
				_p.pr("const ").pr(sname).pr(" *self = reinterpret_cast<const ").pr(sname).pr("*>(self_);").pn();
			_p.pr("switch(id) {").pni();
			index = 0;
			for(auto it : cells) {
				size_t n = it->n;
				if(n == 1 && it->parent && it->parent->name.isEmpty())
					n = it->parent->n;
				_p.pr("case ").pr(index).pr(": return ");
				if(n > 0)
					_p.pr(n);
				else
					_p.pr("self->").pr(lowername(it->name)).pr(".size()");
				_p.pr(";").pn();
				index++;
			}
			_p.pr("default: return 0;").pn();
			_p.pr("}").upn();
			_p.pr("}").upn();

			if(hasDynamic) {
				_p.pr("virtual bool insert(void *self_, size_t id, size_t index, size_t n) const override {").pni();
				_p.pr(sname).pr(" *self = reinterpret_cast<").pr(sname).pr("*>(self_);").pn();
				_p.pr("switch(id) {").pni();
				index = 0;
				for(auto it : cells) {
					size_t n = it->n;
					if(n == 1 && it->parent && it->parent->name.isEmpty())
						n = it->parent->n;
					if(n == 0) {
						_p.pr("case ").pr(index).pr(":").pni();
						_p.pr("while(n--) self->").pr(lowername(it->name)).pr(".insert(index, decltype(self->").pr(lowername(it->name)).pr(")::value_type());").pn();
						_p.pr("return true;").pnu();
					}
					index++;
				}
				_p.pr("default: return false;").pn();
				_p.pr("}").upn();
				_p.pr("}").upn();

				_p.pr("virtual bool remove(void *self_, size_t id, size_t index, size_t n) const override {").pni();
				_p.pr(sname).pr(" *self = reinterpret_cast<").pr(sname).pr("*>(self_);").pn();
				_p.pr("switch(id) {").pni();
				index = 0;
				for(auto it : cells) {
					size_t n = it->n;
					if(n == 1 && it->parent && it->parent->name.isEmpty())
						n = it->parent->n;
					if(n == 0) {
						_p.pr("case ").pr(index).pr(":").pni();
						_p.pr("while(n--) self->").pr(lowername(it->name)).pr(".removeAt(index);").pn();
						_p.pr("return true;").pnu();
					}
					index++;
				}
				_p.pr("default: return false;").pn();
				_p.pr("}").upn();
				_p.pr("}").upn();
			}
		}
/*		void genValueCheck(ValueData *value) {
			_p.pr("static_assert(ValueInfo<Name::").pr(uppername(value->fullname)).pr(">::IsValue, \"missing value implementation for ").pr(value->fullname).pr("\");").pn();
		}*/

		void genCellStruct(const QList<CellData*> &subs) {
			for(auto it : subs) {
				size_t n = it->n;
				if(n == 1 && it->parent && it->parent->name.isEmpty())
					n = it->parent->n;
				VarData *var = it->to<VarData>();
				if(var) {
					gencppMember(var->type, var->name, n);
					_p.pr(";").pn();
					continue;
				}

				CellContainerData *container = it->to<CellContainerData>();
				if(!container) {
					Q_ASSERT(false);
				}
				gencppMember(QString("rec_%1").arg(container->elemid), container->name, n);
				_p.pr(";").pn();
			}
		}

		void genValue(ValueData *data) {
			for(auto it : data->cells) {
				size_t n = it->n;
				if(n == 1 && it->parent && it->parent->name.isEmpty())
					n = it->parent->n;
				VarData *var = it->to<VarData>();
				if(var) {
					gencppMember(var->type, var->name, n);
					_p.pr(";").pn();
					continue;
				}

				CellContainerData *container = it->to<CellContainerData>();
				if(!container) {
					Q_ASSERT(false);
				}
				gencppMember(QString("rec_%1").arg(container->elemid), container->name, n);
				_p.pr(";").pn();
			}
			_p.pr("};").upn();
		}

		void genDeclMeta() {
			size_t n;
			_p.pr("extern const DeclMeta meta;").pn();

			_p.pr("#ifdef IMPLEMENTATION").opn(0);
			_p.pr("const DeclMeta meta = {").pni();
			
			_p.pr(".name = ").pr(parseText(_model->version())).pr(",").pn();

			_p.pr(".keys = (const DeclKey*[]){").pni();
			QQueue<KeyData*> keysubs;
			for(auto it : _model->keys())
				keysubs.enqueue(it);
			n = 0;
			while(!keysubs.isEmpty()) {
				KeyData *cur = keysubs.dequeue();
				_p.pr("&").pr(lowername(cur->fullname)).pr(",").pn();
				n++;
				for(auto subit : cur->subs)
					keysubs.enqueue(subit);
			}
			_p.pr("},").upn();
			_p.pr(".nKeys = ").pr(n).pr(",").pn();

			_p.pr(".rootKeys = (const DeclKey*[]){").pni();
			for(auto it : _model->keys())
				_p.pr("&").pr(lowername(it->fullname)).pr(",").pn();
			_p.pr("},").upn();
			_p.pr(".nRootKeys = ").pr(_model->keys().size()).pr(",").pn();
			
			_p.pr(".values = (const DeclValue*[]){").pni();
			for(auto it : _model->values())
				_p.pr("&").pr(lowername(it->fullname)).pr(",").pn();
			_p.pr("},").upn();
			_p.pr(".nValues = ").pr(_model->values().size()).pr(",").pn();

				_p.pr(".types = (const DeclType*[]){").pni();
			for(auto it : _model->types())
				_p.pr("&").pr(lowername(it->fullname)).pr(",").pn();
			_p.pr("},").upn();
			_p.pr(".nTypes = ").pr(_model->types().size()).pr(",").pn();

			_p.pr(".idTypes = (const DeclType*[]){").pni();
			n = 0;
			for(auto it : _model->types())
				if(it->dynamicId) {
					_p.pr("&").pr(lowername(it->fullname)).pr(",").pn();
					n++;
				}
			_p.pr("},").upn();
			_p.pr(".nIdTypes = ").pr(n).pr(",").pn();

			_p.pr(".keyType = &keyType").pn();

			_p.pr("};").upn();
			_p.pr("#endif").opn(0);
		}

		int fieldGlobalIndex(const KeyData *key, const FieldData *field) {
			int index = field->index;
			key = key->parent;
			while(key) {
				index += key->fields.size() + 1;
				key = key->parent;
			}
			return index;
		}

		void genDeclType(TypeData *type) {
			QString name = type->fullname;
			if(!type->enumdef.isEmpty()) {
				_p.pr("enum class ").pr(uppername(name)).pr(" {").pni();
				for(auto it : type->enumdef)
					genEnum(it);
				_p.pr("};").upn();
			}
			_p.pr("#ifdef IMPLEMENTATION").opn(0);
			_p.pr("const DeclType ").pr(lowername(name)).pr(" = {").pni();
			_p.pr(".name = ").pr(parseText(type->name)).pr(",").pn();
			_p.pr(".fullname = ").pr(parseText(type->fullname)).pr(",").pn();
			_p.pr(".builtin = ").pr(builtin(type->builtin())).pr(",").pn();
			_p.pr(".size = ").pr(type->size()).pr(",").pn();
			_p.pr(".super = ").pr(type->super ? parseReference(type->super->fullname) : "nullptr").pr(",").pn();
			_p.pr(".enumOrdering = ").pr(ordering(type->enumType)).pr(",").pn();
			_p.pr(".multiOrdering = ").pr(ordering(type->multiType)).pr(",").pn();
			_p.pr(".enumValues = (DeclEnum[]){").pni();
			for(auto it : type->enumdef)
				genDeclEnum(it);
			_p.pr("},").upn();
			_p.pr(".nEnumValues = ").pr(type->enumdef.size()).pr(",").pn();
			ValueData *mapto = _model->mapto(type);
			if(mapto)
				_p.pr(".mapto = &").pr(lowername(mapto->fullname)).pr(",").pn();
			else
				_p.pr(".mapto = nullptr,").pn();
			if(type->dynamicId) {
				_p.pr(".dynamicId = ").pr(type->dynamicId).pr(",").pn();
				_p.pr(".owners = (DeclDynid[]){").pni();
				for(auto it : type->owners) {
					_p.pr("{");
					_p.pr(" .key = &").pr(lowername(it.key->fullname));
					if(it.field)
						_p.pr(", .type = DeclDynid::FieldType");
					else
						_p.pr(", .type = DeclDynid::VarType");
					_p.pr(", .u = {");
					if(it.field) {
						_p.pr(" .field = ").pr(fieldGlobalIndex(it.key, it.field));
					}
					else {
						CellData *cur = it.var;
						QStack<size_t> path;
						while(cur) {
							if(!cur->name.isEmpty())
								path.push(cur->childid);
							cur = cur->parent;
						}

						_p.pr(" .value = { .decl = &").pr(lowername(it.value->fullname)).pr(", .varpath = (size_t[]){ ");
						bool first = true;
						while(!path.isEmpty()) {
							if(first)
								first = false;
							else
								_p.pr(", ");
							_p.pr(path.pop());
						}
						_p.pr(" }");
//						_p.pr(" .var = ").pr(lowername(it.var));
					}
					_p.pr(" } } },").pn();
				}
				_p.pr("},").upn();
				_p.pr(".nOwners = ").pr(type->owners.size()).pn();
			}
			_p.pr("};").upn();
			_p.pr("#endif").opn(0);
		}

		void genEnum(EnumData *enm) {
			_p.pr(uppername(enm->name)).pr(",").pn();
		}

		void genDeclEnum(EnumData *enm) {
			_p.pr("{").pni();
			if(enm->name.isEmpty())
				_p.pr(".name = nullptr,").pn();
			else
				_p.pr(".name = ").pr(parseText(enm->name)).pr(",").pn();
			if(enm->text.isEmpty())
				_p.pr(".text = nullptr,").pn();
			else
				_p.pr(".text = ").pr(parseText(enm->text)).pr(",").pn();
			if(enm->plural.isEmpty())
				_p.pr(".plural = nullptr,").pn();
			else
				_p.pr(".plural = ").pr(parseText(enm->plural)).pr(",").pn();
			ValueData *mapto = _model->mapto(enm);
			if(mapto)
				_p.pr(".mapto = &").pr(lowername(mapto->fullname)).pn();
			_p.pr("},").upn();
		}

		void genDeclKey(KeyData *key, QQueue<KeyData*> &subs) {
			QList<KeyData*> hierarchy;
			for(KeyData *it = key; it; it = it->parent)
				hierarchy.prepend(it);

			QString name = key->fullname;
			_p.pr("enum class ").pr(uppername(name)).pr(" {").pni();
			for(int i = 0; i < hierarchy.size(); i++) {
				KeyData *cur = hierarchy.at(i);
				_p.pr("_").pr(i).pr(",").pn();
				for(auto it : cur->fields)
					_p.pr(uppername(it->name)).pr(",").pn();
			}
			_p.pr("};").upn();

			_p.pr("#ifdef IMPLEMENTATION").opn(0);
			_p.pr("const DeclKey ").pr(lowername(name)).pr(" = {").pni();
			_p.pr(".name = ").pr(parseText(key->name)).pr(",").pn();
			_p.pr(".fullname = ").pr(parseText(key->fullname)).pr(",").pn();
			_p.pr(".parent = ").pr(key->parent ? parseReference(key->parent->fullname) : "nullptr").pr(",").pn();
			_p.pr(".index = ").pr(key->index).pr(",").pn();
			_p.pr(".offset = ").pr(key->offset).pr(",").pn();
			_p.pr(".abstract = ").pr(key->subs.size() > 0 ? "true" : "false").pr(",").pn();
			_p.pr(".flexible = ").pr(key->flexible ? "true" : "false").pr(",").pn();
			_p.pr(".fields = (DeclField[]){").pni();

			size_t nfield = 0;
			for(int i = 0; i < hierarchy.size(); i++) {
				KeyData *cur = hierarchy.at(i);
				genDeclKeyField(cur, i, nfield++);
				for(auto it : cur->fields)
					genDeclField(it, nfield++);
			}
			_p.pr("},").upn();

			_p.pr(".nFields = ").pr(nfield).pr(",").pn();
			_p.pr(".size = ").pr(key->size).pr(",").pn();
			_p.pr(".subKeys = (const DeclKey*[]){").pni();
			for(auto it : key->subs)
				_p.pr("&").pr(lowername(it->fullname)).pr(",").pn();
			_p.pr("},").upn();
			_p.pr(".nSubKeys = ").pr(key->subs.size()).pr(",").pn();
			ValueData *mapto = _model->mapto(key);
			if(mapto)
				_p.pr(".mapto = &").pr(lowername(mapto->fullname)).pn();
			_p.pr("};").upn();
			_p.pr("#endif").opn(0);
			for(auto it : key->subs)
				subs.append(it);
		}

		void genDeclField(FieldData *field, size_t globidx) {
			_p.pr("{").pni();
			_p.pr(".name = ").pr(parseText(field->name)).pr(",").pn();
			_p.pr(".type = ").pr(parseReference(field->type->fullname)).pr(",").pn();
			_p.pr(".key = nullptr,").pn();
			_p.pr(".index = ").pr(field->index).pr(",").pn();
			_p.pr(".globalidx = ").pr(globidx).pr(",").pn();
			_p.pr(".offset = ").pr(field->offset).pn();
			_p.pr("},").upn();
		}

		void genDeclKeyField(KeyData *key, size_t keyidx, size_t globidx) {
			_p.pr("{").pni();
			_p.pr(".name = \"").pr(keyidx).pr("\",").pn();
			_p.pr(".type = &keyType,").pn();
			_p.pr(".key = ").pr(parseReference(key->fullname)).pr(",").pn();
			_p.pr(".index = 0,").pn();
			_p.pr(".globalidx = ").pr(globidx).pr(",").pn();
			_p.pr(".offset = ").pr(key->offset).pn();
			_p.pr("},").upn();
		}

		void genDeclValue(ValueData *value) {
			size_t total;
			_p.pr("using ").pr(uppername(value->fullname)).pr(" = anon_value::value_").pr(uppername(value->fullname)).pr(";").pn();
			_p.pr("#ifdef IMPLEMENTATION").opn(0);

			_p.pr("const DeclValue ").pr(lowername(value->fullname)).pr(" = {").pni();
			_p.pr(".name = Name::").pr(uppername(value->fullname)).pr(",").pn();

			total = 0;
			for(auto it : value->rows)
				total += it->n;
			_p.pr(".size = ").pr(total).pr(",").pn();
			if(value->rows.isEmpty())
				_p.pr(".flexible = false,").pn();
			else
				_p.pr(".flexible = ").pr(value->rows.last()->n == 0 ? "true" : "false").pr(",").pn();

			_p.pr(".vars = (const DeclVar[]){").pni();


			genDeclVar(0, value->cells);
			_p.pr("},").upn();
			_p.pr(".nVars = ").pr(value->flat.size()).pr(",").pn();
			_p.pr(".accessor = &anon_value::accessor_value_").pr(uppername(value->fullname)).pr(",").pn();
			_p.pr(".create = anon_value::create_").pr(uppername(value->fullname)).pr(",").pn();
			_p.pr(".destroy = anon_value::destroy_").pr(uppername(value->fullname)).pr(",").pn();
			_p.pr(".rows = (const DeclRow[]){").pni();
			size_t offset = 0;
			for(auto it : value->rows) {
				if(it->name.isEmpty())
					genDeclRow(value, offset, value->cells.indexOf(it->subs.first()), it);
				else
					genDeclRow(value, offset, it->parentid, it);
			}
			_p.pr("},").upn();
			_p.pr(".nRows = ").pr(value->rows.size()).pr(",").pn();
			_p.pr("};").upn();
			_p.pr("#endif").opn(0);

		}

		void genDeclVar(size_t parent, const QList<CellData*> &cells) {
			for(auto it : cells) {
				size_t n = it->n;
				if(n == 1 && it->parent && it->parent->name.isEmpty())
					n = it->parent->n;
				_p.pr("{ .parent = ").pr(parent).pr(", .name = Name::").pr(uppername(it->name));
				_p.pr(", .n = ").pr(n).pr(", .offset = ").pr(it->offset).pr(", .size = ").pr(it->size);
				VarData *var = it->to<VarData>();
				if(var)
					_p.pr(", .u = { .type = &").pr(lowername(var->type->fullname)).pr(" }");
				else {
					CellContainerData *container = it->to<CellContainerData>();
					_p.pr(", .u = { .accessor = &anon_value::accessor_rec_").pr(container->elemid).pr(" }");
					_p.pr(", .child = ").pr(it->parentid);
				}
				if(!it->parent)
					_p.pr(", .isrow = true");
				_p.pr(" },").pn();
			}
			for(auto it : cells) {
				CellContainerData *container = it->to<CellContainerData>();
				if(container)
					genDeclVar(it->parentid, container->cells);
			}
		}

		void genDeclRow(ValueData *value, size_t &offset, size_t varidx, RowData *row) {
			_p.pr("{").pni();
			_p.pr(".offset = ").pr(offset).pr(",").pn();
			_p.pr(".n = ").pr(row->n).pr(",").pn();
			offset += row->n;
			_p.pr(".size = ").pr(row->size).pr(",").pn();
			_p.pr(".flexible = ").pr(row->flexible ? "true" : "false").pr(",").pn();
			_p.pr(".vars = ").pr(varidx).pr(",").pn();
//			_p.pr(".vars = ").pr(lowername(value->fullname)).pr(".vars + ").pr(varidx).pr(",").pn();
			_p.pr(".nVars = ").pr(row->subs.size()).pr(",").pn();
/*			_p.pr(".index = ").pr(row->index).pr(",").pn();
			_p.pr(".offset = ").pr(row->offset).pr(",").pn();*/
			if(row->name.isEmpty() && row->n != 1)
				_p.pr(".split = true").pn();
/*			_p.pr(".cells = (const DeclCell[]){").pni();
			size_t total = 0;
			genDeclCells(total, 0, row->subs);
			_p.pr("},").upn();
			_p.pr(".nCells = ").pr(total).pn();*/
			_p.pr("},").upn();
		}

		void genStruct(StructData *strct) {
			QString name = strct->fullname;
			_p.pr("struct ").pr(uppername(name)).pr(" {").pni();
			for(auto it : strct->members)
				genStructMember(it);
			_p.pr("static const ").pr(uppername(name)).pr(" Default;").pn();
			_p.pr("static constexpr size_t NAux = ").pr(strct->aux.size()).pr(";").pn();
			_p.pr("static const ").pr(uppername(name)).pr(" Aux[NAux];").pn();
			_p.pr("};").upn();

			_p.pr("#ifdef IMPLEMENTATION").opn(0);
			_p.pr("const ").pr(uppername(name)).pr(" ").pr(uppername(name)).pr("::Default;").pn();
			_p.pr("const ").pr(uppername(name)).pr(" ").pr(uppername(name)).pr("::Aux[NAux] = {").pni();
			for(auto it : strct->aux) {
				_p.pr("{").pni();
				genAuxValue(it);
				_p.pr("},").upn();
			}
			_p.pr("};").upn();
			_p.pr("#endif").opn(0);
		}

		void genStructMember(MemberData *member) {
			_p.pr(cppvar(member->type)).pr(lowername(member->name));
			if(!member->value.isEmpty()) {
				_p.pr(" = ").pr(cppvalue(member->type, member->value));
			}
			else
				_p.pr(" = ").pr(cppnull(member->type));
			_p.pr(";").pn();
		}

		void genStructMapping(const QString &ns, StructData *strct) {
			size_t nEnumMap = 0;
			size_t nTypeMap = 0;
			size_t nKeyMap = 0;
			for(auto it : strct->aux)
				if(it->source->id() == EnumData::Id)
					nEnumMap++;
				else if(it->source->id() == TypeData::Id)
					nTypeMap++;
				else if(it->source->id() == KeyData::Id)
					nKeyMap++;
				else
					printf("missing aux mapping handler for data source %d\n", it->source->id());

			_p.pr("template<> struct Mapping<DeclEnum, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("> : MappingImpl<DeclEnum, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr(", ").pr(nEnumMap).pr("> {};").pn();
			if(nEnumMap > 0) {
				size_t index = 0;

				_p.pr("#ifdef IMPLEMENTATION").opn(0);
				_p.pr("template<> const MappedValue<DeclEnum, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("> MappingImpl<DeclEnum, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr(", ").pr(nEnumMap).pr(">::mapping[N] = {").pni();
				for(auto it : strct->aux) {
					if(it->source->id() == EnumData::Id) {
						EnumData *data = it->source->to<EnumData>();
						_p.pr("{").pni();
						_p.pr(".type = &").pr(ns).pr("::").pr(lowername(data->owner->fullname)).pr(",").pn();
						_p.pr(".value = ").pr(data->index).pr(",").pn();
						_p.pr(".aux = ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("::Aux + ").pr(index).pr(",").pn();
						_p.pr("},").upn();
					}
					index++;
				}
				_p.pr("};").upn();
				_p.pr("#endif").opn(0);
			}

			_p.pr("template<> struct Mapping<DeclType, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("> : MappingImpl<DeclType, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr(", ").pr(nTypeMap).pr("> {};").pn();
			if(nTypeMap > 0) {
				size_t index = 0;

				_p.pr("#ifdef IMPLEMENTATION").opn(0);
				_p.pr("template<> const MappedValue<DeclType, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("> MappingImpl<DeclType, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr(", ").pr(nTypeMap).pr(">::mapping[N] = {").pni();
				for(auto it : strct->aux) {
					if(it->source->id() == TypeData::Id) {
						TypeData *data = it->source->to<TypeData>();
						_p.pr("{").pni();
						_p.pr(".type = &").pr(ns).pr("::").pr(lowername(data->fullname)).pr(",").pn();
						_p.pr(".aux = ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("::Aux + ").pr(index).pr(",").pn();
						_p.pr("},").upn();
					}
					index++;
				}
				_p.pr("};").upn();
				_p.pr("#endif").opn(0);
			}

			_p.pr("template<> struct Mapping<DeclKey, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("> : MappingImpl<DeclKey, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr(", ").pr(nKeyMap).pr("> {};").pn();
			if(nKeyMap > 0) {
				size_t index = 0;

				_p.pr("#ifdef IMPLEMENTATION").opn(0);
				_p.pr("template<> const MappedValue<DeclKey, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("> MappingImpl<DeclKey, ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr(", ").pr(nKeyMap).pr(">::mapping[N] = {").pni();
				for(auto it : strct->aux) {
					if(it->source->id() == KeyData::Id) {
						KeyData *data = it->source->to<KeyData>();
						_p.pr("{").pni();
						_p.pr(".key = &").pr(ns).pr("::").pr(lowername(data->fullname)).pr(",").pn();
						_p.pr(".aux = ").pr(ns).pr("::").pr(uppername(strct->fullname)).pr("::Aux + ").pr(index).pr(",").pn();
						_p.pr("},").upn();
					}
					index++;
				}
				_p.pr("};").upn();
				_p.pr("#endif").opn(0);
			}
		}

		void genAuxValue(AuxData *aux) {
			for(auto it : aux->strct->members) {
				QString value = aux->values.value(it->name);
				if(!value.isEmpty()) {
					_p.pr(".").pr(lowername(it->name)).pr(" = ").pr(cppvalue(it->type, value)).pr(",").pn();
				}
			}
		}

		void gencppMember(TypeData *type, const QString &name, size_t n) {
			int internal = type->builtin();
			if(n == 0) {
				switch(internal) {
					case TypeData::FlexString: _p.pr("QStringList ").pr(lowername(name)); break;
					default: _p.pr("QList<").pr(cpptype(type)).pr("> ").pr(lowername(name)); break;
				}
			}
			else if(n > 1)
				_p.pr(cppvar(type)).pr(lowername(name)).pr("[").pr(n).pr("]").pr(" = { ").pr(cppnull(type)).pr(" }");
			else
				_p.pr(cppvar(type)).pr(lowername(name)).pr(" = ").pr(cppnull(type));
		}

		void gencppMember(const QString &type, const QString &name, size_t n) {
			if(n == 0)
				_p.pr("QList<").pr(type).pr("> ").pr(lowername(name));
			else if(n > 1)
				_p.pr(type).pr(" ").pr(lowername(name)).pr("[").pr(n).pr("]");
			else
				_p.pr(type).pr(" ").pr(lowername(name));
		}

		QString cppvar(TypeData *type) {
			int internal = type->builtin();
			switch(internal) {
				case TypeData::U8: return "quint8 ";
				case TypeData::U16: return "quint16 ";
				case TypeData::U32: return "quint32 ";
				case TypeData::U64: return "quint64 ";
				case TypeData::Boolean: return "bool ";
				case TypeData::FlexString: return "QString ";
				case TypeData::KeyReference: return "const DeclKey *";
				case TypeData::TypeReference: return "const DeclType *";
				case TypeData::StructReference: return "const DeclStruct *";
				default: throw 1;
			}
		}

		QString cpptype(TypeData *type) {
			int internal = type->builtin();
			switch(internal) {
				case TypeData::U8: return "quint8";
				case TypeData::U16: return "quint16";
				case TypeData::U32: return "quint32";
				case TypeData::U64: return "quint64";
				case TypeData::Boolean: return "bool";
				case TypeData::FlexString: return "QString";
				case TypeData::KeyReference: return "const DeclKey*";
				case TypeData::TypeReference: return "const DeclType*";
				case TypeData::StructReference: return "const DeclStruct*";
				default: throw 1;
			}
		}

		QString cppvalue(TypeData *type, const QString &text) {
			int internal = type->builtin();
			switch(internal) {
				case TypeData::U8: return parseU8(text);
				case TypeData::U16: return parseU16(text);
				case TypeData::U32: return parseU32(text);
				case TypeData::U64: return parseU64(text);
				case TypeData::Boolean: return parseBoolean(text);
				case TypeData::FlexString: return parseText(text);
				case TypeData::KeyReference: return parseReference(text);
				case TypeData::TypeReference: return parseReference(text);
				case TypeData::StructReference: return parseReference(text);
				default: throw 1;
			}
		}

		QString cppnull(TypeData *type) {
			int internal = type->builtin();
			switch(internal) {
				case TypeData::U8: return "0";
				case TypeData::U16: return "0";
				case TypeData::U32: return "0";
				case TypeData::U64: return "0";
				case TypeData::Boolean: return "false";
				case TypeData::FlexString: return "nullptr";
				case TypeData::KeyReference: return "nullptr";
				case TypeData::TypeReference: return "nullptr";
				case TypeData::StructReference: return "nullptr";
				default: throw 1;
			}
		}

		QString ordering(int value) {
			switch(value) {
				case NoMulti: return "NoOrdering";
				case ListMulti: return "ListOrdering";
				case SetMulti: return "SetOrdering";
				default: throw 1;
			}
		}

		QString builtin(int value) {
			switch(value) {
				case TypeData::U8: return "BuiltinU8";
				case TypeData::U16: return "BuiltinU16";
				case TypeData::U32: return "BuiltinU32";
				case TypeData::U64: return "BuiltinU64";
				case TypeData::Boolean: return "BuiltinBoolean";
				case TypeData::FlexString: return "BuiltinFlexString";
				case TypeData::KeyReference: return "BuiltinKeyReference";
				case TypeData::TypeReference: return "BuiltinTypeReference";
				case TypeData::StructReference: return "BuiltinStructReference";
				default: throw 1;
			}
		}

		QString parseU8(const QString &text) {
			return text;
		}

		QString parseU16(const QString &text) {
			return text;
		}

		QString parseU32(const QString &text) {
			return text;
		}

		QString parseU64(const QString &text) {
			return text;
		}

		QString parseText(const QString &text) {
			QVector<uint> utf32 = text.toUcs4();
			QString result = "\"";
			for(auto it : utf32) {
				if(it < 32)
					result += QString::asprintf("\\x%02x", it);
				else if(it == '"')
					result += "\\\"";
				else if(it == '?')
					result += "\\\?";
				else if(it == '\\')
					result += "\\\\";
				else if(it < 127)
					result += QChar(it);
				else if(it < 0x100)
					result += QString::asprintf("\\x%02x", it);
				else if(it < 0x10000)
					result += QString::asprintf("\\u%04x", it);
				else
					result += QString::asprintf("\\U%08x", it);
			}
			return result + "\"";
		}

		QString parseBoolean(const QString &text) {
			return text;
		}

		QString parseReference(const QString &text) {
			return "&" + lowername(text);
		}

		QString uppername(const QString &name) {
			QStringList words = name.split(wordsep);
			for(int i = 0; i < words.size(); i++)
				words[i][0] = words[i][0].toUpper();
			return words.join("");
		}

		QString lowername(const QString &name) {
			QStringList words = name.split(wordsep);
			for(int i = 1; i < words.size(); i++)
				words[i][0] = words[i][0].toUpper();
			return words.join("");
		}

		Model *_model;

		Printer _p;
};

