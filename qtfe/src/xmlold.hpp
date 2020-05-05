using id_t = quint64;

struct ObjectFrame : XmlModelHandlerFrame {
	ObjectFrame(const KeyEditor &key) : key(key) {}

	struct Entry {
		QStringList text;
		const DeclValue *mapto;
	};

	KeyEditor key;
	QMap<QByteArray, Entry> values;
};

class Handler : public XmlModelHandler {
	public:
		Handler(SyncFrontend *fe) : _fe(fe) {
			if(fe->spec() != &v1_0::meta)
				throw XmlModelException("just spec 1.0 target supported by old xml loader");
				
			int root = add();
			int person = add(cb(this, &Handler::startPerson), cb(this, &Handler::endPerson));
			int person_props = add(cb(this, &Handler::runPersonProperty));

			link(0, "annotate-db", root);
			link(root, "person", person);
			link(person, person_props);
		}

	private:
		void startPerson(XmlModelHandlerFrame *&frame, const QXmlAttributes &attr) {
			QString id = attr.value("id");
			if(id.isEmpty())
				throw XmlModelException("missing person id");
			quint32 objid = _fe->acquire(&v1_0::objectId);
			if(!objid)
				throw XmlModelException("error acquiring object id for person");

			KeyEditor key(&v1_0::personObject);
			key.uintPut(v1_0::PersonObject::ObjectId, objid);

			frame = new ObjectFrame(key);

			KeyEditor classkey(&v1_0::personClass);
			classkey.stringPut(v1_0::PersonClass::ObjectName, id);
			auto value = _fe->get<v1_0::ObjectRef>(classkey, true);
			value->id = objid;
		}

		void endPerson(XmlModelHandlerFrame *frame) {
			ObjectFrame *object = frame->to<ObjectFrame>();
			for(auto it = object->values.begin(); it != object->values.end(); ++it) {
				QStringList text = it.value().text;
				if(it.value().mapto == &v1_0::textLine) {
					auto value = _fe->get<v1_0::TextLine>(it.key(), true);
					if(text.size() == 1)
						value->text = text.at(0);
					else if(text.size() == 0)
						value->text = QString();
					else
						throw Exception("multiple lines for single line text");
				}
				else if(it.value().mapto == &v1_0::textMultiline) {
					auto value = _fe->get<v1_0::TextMultiline>(it.key(), true);
					value->text.clear();
					for(auto it : text)
						value->text.append(it);
				}
				else
					throw Exception("cannot load person property: unknown mapto value");
			}
		}

		void runPersonProperty(XmlModelHandlerFrame *frame, const QString &element, const QXmlAttributes &attr) {
			ObjectFrame *object = frame->to<ObjectFrame>();

			ssize_t propid;
			ssize_t langid;

			QString elem = element;
			elem.replace("_", "-");
			propid = v1_0::personPropertyId.nameIndexOf(elem);
			if(propid < 0)
				throw XmlModelException("no such person property '" + elem + "'");

			QString lang = attr.value("language");
			if(lang.isEmpty())
				lang = "en";
			langid = v1_0::languageId.nameIndexOf(lang);
			if(langid < 0)
				throw XmlModelException("no such language '" + lang + "'");
			
			object->key.enumPut(v1_0::PersonObject::PropertyId, EnumInfo<v1_0::PersonPropertyId>::fromIndex(propid));
			object->key.enumPut(v1_0::PersonObject::LanguageId, EnumInfo<v1_0::LanguageId>::fromIndex(langid));
			QByteArray key = object->key;
			ObjectFrame::Entry &entry = object->values[key];
			entry.mapto = object->key.mapto();
			entry.text.append(attr.value("value"));
		}

		SyncFrontend *_fe;
};

