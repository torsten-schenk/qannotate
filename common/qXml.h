#pragma once

#include <exception>
#include <functional>
#include <QStack>
#include <QXmlSimpleReader>
#include <QXmlDefaultHandler>

#include "common/qException.h"
#include "common/AvlTable.h"

class XmlModelException : public Exception {
	public:
		XmlModelException(const QString &message) : Exception(message) {
/*			backward::StackTrace st;
			st.load_here(32);
			backward::Printer p;
			p.print(st);*/
		}

};

class XmlModelHandler;

class XmlModelHandlerFrame {
	friend class XmlModelLoader;
	public:
		XmlModelHandlerFrame()
			:	_gap(0),
				_parent(nullptr)
		{}

		virtual ~XmlModelHandlerFrame() {}

		template<typename T> T *to(bool force = true) {
			T *result = dynamic_cast<T*>(this);
			if(!result && force)
				throw XmlModelException("invalid frame type");
			return result;
		}

		template<typename T> const T* to(bool force = true) const {
			const T *result = dynamic_cast<const T*>(this);
			if(!result && force)
				throw XmlModelException("invalid frame type");
			return result;
		}

		template<typename T> T *topmost(bool force = true) {
			T *result = nullptr;
			XmlModelHandlerFrame *cur = this;
			while(cur && !result) {
				result = dynamic_cast<T*>(cur);
				cur = cur->_parent;
			}
			if(!result && force)
				throw XmlModelException("topmost frame not found");
			return result;
		}

		template<typename T> T *parent(bool force = true) {
			T *result = nullptr;
			XmlModelHandlerFrame *cur = _parent;
			while(cur && !result) {
				result = dynamic_cast<T*>(cur);
				cur = cur->_parent;
			}
			if(!result && force)
				throw XmlModelException("topmost frame not found");
			return result;
		}

	private:
		int _gap; //number of null frames between parent and this
		XmlModelHandlerFrame *_parent;
};

namespace detail {
	template<size_t N> struct Binder;

	template<> struct Binder<1> {
		template<typename T, typename R, typename... ARGS> static auto exec(T *self, R (T::*fn)(ARGS...)) {
			return std::bind(fn, self, std::placeholders::_1);
		}
	};

	template<> struct Binder<2> {
		template<typename T, typename R, typename... ARGS> static auto exec(T *self, R (T::*fn)(ARGS...)) {
			return std::bind(fn, self, std::placeholders::_1, std::placeholders::_2);
		}
	};

	template<> struct Binder<3> {
		template<typename T, typename R, typename... ARGS> static auto exec(T *self, R (T::*fn)(ARGS...)) {
			return std::bind(fn, self, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
		}
	};
};

class XmlModelLoader;

class XmlModelHandler {
	friend class XmlModelLoader;
	public:
		using Frame = XmlModelHandlerFrame;
		using Exception = XmlModelException;
		//Frame argument: parent frame. return: own frame or nullptr
		using StartFn0 = void (const QXmlAttributes&);
		using StartFn1 = void (XmlModelHandlerFrame*&, const QXmlAttributes&);
		using StartFn2 = void (const QString&, const QXmlAttributes&);
		using StartFn3 = void (XmlModelHandlerFrame*&, const QString&, const QXmlAttributes&);
		using EndFn0 = void();
		using EndFn1 = void(XmlModelHandlerFrame*);

	protected:
		XmlModelHandler() : _owner(nullptr) {}

		virtual ~XmlModelHandler() {}

		int add() {
			_nodes.append(Node());
			return _nodes.size();
		}

		int add(const std::function<StartFn0> &start, const std::function<EndFn0> &end) {
			Node node;
			node.start0 = start;
			node.end0 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn1> &start, const std::function<EndFn0> &end) {
			Node node;
			node.start1 = start;
			node.end0 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn2> &start, const std::function<EndFn0> &end) {
			Node node;
			node.start2 = start;
			node.end0 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn3> &start, const std::function<EndFn0> &end) {
			Node node;
			node.start3 = start;
			node.end0 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn0> &start, const std::function<EndFn1> &end) {
			Node node;
			node.start0 = start;
			node.end1 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn1> &start, const std::function<EndFn1> &end) {
			Node node;
			node.start1 = start;
			node.end1 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn2> &start, const std::function<EndFn1> &end) {
			Node node;
			node.start2 = start;
			node.end1 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn3> &start, const std::function<EndFn1> &end) {
			Node node;
			node.start3 = start;
			node.end1 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn0> &start) {
			Node node;
			node.start0 = start;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn1> &start) {
			Node node;
			node.start1 = start;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn2> &start) {
			Node node;
			node.start2 = start;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<StartFn3> &start) {
			Node node;
			node.start3 = start;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<EndFn0> &end) {
			Node node;
			node.end0 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		int add(const std::function<EndFn1> &end) {
			Node node;
			node.end1 = end;
			_nodes.append(node);
			return _nodes.size();
		}

		void link(int from, const QString &element, int to) {
			_links.insert(from, element, to);
		}

		void link(int from, int to) {
			_links.insert(from, QString(), to);
		}

		template<typename T, typename R, typename... ARGS> auto cb(T *self, R (T::*fn)(ARGS...)) {
			return detail::Binder<sizeof...(ARGS)>::exec(self, fn);
		}

		void delegate(XmlModelHandler *child); //will be free'd on pop()

/*		template<class FN, class SELF> auto startfn(FN &&fn, SELF &&self) {
			return std::bind(fn, self, std::placeholders::_1, std::placeholders::_2);
		}

		template<class FN, class SELF> auto endfn(FN &&fn, SELF &&self) {
			return std::bind(fn, self, std::placeholders::_1);
		}*/

	private:
		struct Node {
			Node() {}
/*			Node(const std::function<StartFn> &start, const std::function<EndFn> &end) : start(start), end(end) {}
			Node(const std::function<EndFn> &end) : end(end) {}
			Node(const std::function<StartFn> &start) : start(start) {}*/

			std::function<StartFn0> start0;
			std::function<StartFn1> start1;
			std::function<StartFn2> start2;
			std::function<StartFn3> start3;
			std::function<EndFn0> end0;
			std::function<EndFn1> end1;
		};

		XmlModelLoader *_owner;
		QList<Node> _nodes;
		AvlTable<2, int, QString, int> _links;
};

class XmlModelLoader : public QXmlDefaultHandler {
	friend class XmlModelHandler;
	public:
		using Exception = XmlModelException;
		XmlModelLoader() : _userStack(nullptr), _ignore(0), _delegate(nullptr) {}

		~XmlModelLoader() {
			while(!_execStack.isEmpty()) {
				const ExecFrame &top = _execStack.pop();
				if(!top.node)
					delete top.handler;
			}
			while(_userStack) {
				XmlModelHandlerFrame *prev = _userStack;
				_userStack = _userStack->_parent;
				delete prev;
			}
			delete _delegate;
		}

		bool parse(const QString &filename, XmlModelHandler *handler, XmlModelHandlerFrame *root = nullptr) {
			QFile file(filename);
			if(!file.open(QFile::ReadOnly)) {
				printf("error opening file\n");
				return false;
			}
			QXmlInputSource source(&file);
			return parse(&source, handler, root);
		}

		bool parse(const QXmlInputSource *input, XmlModelHandler *handler, XmlModelHandlerFrame *root = nullptr)
		{
			if(handler->_owner) {
				printf("handler already owned\n");
				return false;
			}
			handler->_owner = this;
			clear();
			QXmlSimpleReader reader;
			reader.setContentHandler(this);
			reader.setErrorHandler(this);
			if(root)
				_userStack = root;
			else
				_userStack = new XmlModelHandlerFrame();
			_execStack.push(ExecFrame(handler, 0));
			if(reader.parse(input))
				return true;
			printf("ERROR: %s\n", reader.errorHandler()->errorString().toUtf8().constData());
			return false;
		}

		void clear() {
			while(!_execStack.isEmpty()) {
				const ExecFrame &top = _execStack.pop();
				if(!top.node)
					delete top.handler;
			}
			while(_userStack) {
				XmlModelHandlerFrame *prev = _userStack;
				_userStack = _userStack->_parent;
				delete prev;
			}
			_ignore = 0;
			delete _delegate;
			_delegate = nullptr;
		}

	private:
		using Node = XmlModelHandler::Node;

		struct ExecFrame {
			ExecFrame(XmlModelHandler *handler = nullptr, int node = 0) : handler(handler), node(node), wildcard(0) {}

			XmlModelHandler *handler;
			int node;
			size_t wildcard;
		};

		bool fatalError(const QXmlParseException &exception) override {
			printf("fatal error at line %d, column %d: %s\n", exception.lineNumber(), exception.columnNumber(), qPrintable(exception.message()));
			return false;
		}

		bool error(const QXmlParseException &exception) override {
			printf("error at line %d, column %d: %s\n", exception.lineNumber(), exception.columnNumber(), qPrintable(exception.message()));
			return false;
		}

		bool start(XmlModelHandler *delegate) {
			ExecFrame from = _execStack.top();
			ExecFrame to;
			if(delegate) {
				if(delegate->_owner) {
					printf("DELEGATE ERROR\n");
					return false;
				}
				delegate->_owner = this;
				from = ExecFrame(delegate, 0);
				_execStack.push(from);
				_userStack->_gap++;
			}
//			printf("START: %s %d %d %d\n", localName.toUtf8().constData(), from, to, _nodes.size());
			to = from;
			if(to.wildcard)
				to.wildcard++;
			else {
				to.node = from.handler->_links.cell<2>(from.node, _curName);
				if(!to.node) {
					to.node = from.handler->_links.cell<2>(from.node, QString());
					if(!to.node) {
						printf("CHECK HERE\n");
						return false;
					}
//					to.wildcard = 1; //TODO what exactly specifies a RECURSIVE wildcard? this one should set to.wildcard = 1
				}
			}
			try {
				const Node &node = from.handler->_nodes.at(to.node - 1);
				XmlModelHandlerFrame *user = _userStack;
				if(node.start0)
					node.start0(_curAtts);
				else if(node.start1)
					node.start1(user, _curAtts);
				else if(node.start2)
					node.start2(_curName, _curAtts);
				else if(node.start3)
					node.start3(user, _curName, _curAtts);
				if(user == nullptr) { //skip subtree; also don't call 'end' callback of this element
					_ignore = 1;
					return true;
				}
				else if(user != _userStack) {
					if(user->_parent)
						throw Exception("user stack frame cannot be reused");
					user->_parent = _userStack;
					_userStack = user;
				}
				else
					user->_gap++;
				_execStack.push(to);
				return true;
			}
			catch(XmlModelException &e) {
				printf("ERROR IN START: %s\n", e.message().toUtf8().constData());
				return false;
			}
		}

		bool end() {
/*			printf("END: %d\n", _execStack.size());
			for(int i = 0; i < _execStack.size(); i++)
				printf("  [%d] handler=%p node=%d\n", i, _execStack.at(i).handler, _execStack.at(i).node);*/
			ExecFrame cur = _execStack.pop();

			try {
				XmlModelHandlerFrame *user = _userStack;
				const Node &node = cur.handler->_nodes.at(cur.node - 1);
				if(node.end0)
					node.end0();
				else if(node.end1)
					node.end1(user);
				if(user->_gap)
					user->_gap--;
				else {
					_userStack = user->_parent;
					delete user;
				}
				return true;
			}
			catch(XmlModelException &e) {
				printf("ERROR IN END: %s\n", e.message().toUtf8().constData());
				return false;
			}
		}

		virtual bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts) override {
			if(_ignore) {
				_ignore++;
				return true;
			}
			_curName = qName;
			_curAtts = atts;
			if(!start(nullptr)) {

				return false;
			}
			while(_delegate) {
				XmlModelHandler *delegate = _delegate;
				_delegate = nullptr;
				if(!start(delegate))
					return false;
			}
			return true;
		}

		virtual bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName) override {
			if(_ignore) {
				_ignore--;
				return true;
			}
			if(!end())
				return false;
			while(!_execStack.top().node) {
				delete _execStack.pop().handler;
				_userStack->_gap--;
				if(_execStack.isEmpty())
					return true;
				else if(!end())
					return false;
			}
			return true;
		}

		XmlModelHandlerFrame *_userStack;
		QStack<ExecFrame> _execStack;
		unsigned int _ignore;
		XmlModelHandler *_delegate;

		QString _curName;
		QXmlAttributes _curAtts;
};

inline void XmlModelHandler::delegate(XmlModelHandler *child) {
	if(_owner->_delegate)
		throw Exception("delegate handle already used");
	_owner->_delegate = child;
}

