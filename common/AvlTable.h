#pragma once

#include <assert.h>
#include <tuple>
#include <utility>

template<size_t NKEY, typename... COLS> class AvlTable;

namespace detail {
	using id_t = unsigned int;

	template<typename T, typename SEQ> struct TupleFrontImpl;
	template<typename T, size_t... I> struct TupleFrontImpl< T, std::index_sequence<I...> > {
		using TYPE = std::tuple<typename std::tuple_element<I, T>::type...>;

		template<typename TUPLE> static TYPE get(const TUPLE &tuple) {
			return std::make_tuple(std::get<I...>(tuple));
		}
	};
	template<typename T, size_t N> struct TupleFront : TupleFrontImpl< T, std::make_index_sequence<N> > {};

	template<typename T> constexpr T min(T a, T b) {
		return a < b ? a : b;
	}

	template<typename T> constexpr T max(T a, T b) {
		return a > b ? a : b;
	}

	template<size_t I, size_t N, typename A, typename B> struct Compare {
		static int exec(const A &a, const B &b)
		{
			if(std::get<I>(a) < std::get<I>(b))
				return -1;
			else if(std::get<I>(b) < std::get<I>(a))
				return 1;
			else
				return Compare<I + 1, N, A, B>::exec(a, b);
		}
	};

	template<size_t N, typename A, typename B> struct Compare<N, N, A, B> {
		static int exec(const A &a, const B &b)
		{
			return 0;
		}
	};

	template<typename... COLS> struct AvlNode {
		using Tuple = std::tuple<COLS...>;

		AvlNode(const COLS&... cells) :
				b(0),
				size(0),
				p(nullptr),
				l(nullptr),
				r(nullptr),
				cells(cells...)
		{}

		AvlNode() :
				b(0),
				size(0),
				p(nullptr),
				l(nullptr),
				r(nullptr)
		{}

		char b;
		size_t size;
		AvlNode *p;
		AvlNode *l;
		AvlNode *r;
		Tuple cells;

		static void deleteNode(AvlNode *n)
		{
			if(n == nullptr)
				return;
			deleteNode(n->l);
			deleteNode(n->r);
			delete n;
		}

		static void rotateLeft(AvlNode *&root, AvlNode *n)
		{
			auto p = n->p;
			auto l = n->l;
			int h1 = 0; //height of left subtree (initially: p->l)
			int h2 = 0; //height of middle subtree (initially: n->l)
			int h3 = 0; //height of right subtree (initially: n->r)
			size_t s1; //p->l->size
			size_t s2; //n->l->size
			size_t s3; //n->r->size

			//calculate sizes of the three subtrees and set the new size of p and n
			if(n->r != nullptr)
				s3 = n->r->size;
			else
				s3 = 0;
			s2 = n->size - s3 - 1;
			s1 = p->size - s2 - s3 - 2;
			n->size = s1 + s2 + s3 + 2;
			p->size = s1 + s2 + 1;

			//calculate sizes of the three subtrees and set the new size of p and n
			if(n->b <= 0)
				h3 = -n->b;
			else
				h2 = n->b;
			h1 = max(h2, h3) + 1 + p->b;
			p->b = h1 - h2;
			n->b = max(h1, h2) + 1 - h3;

			//perform rotation
			if(p->p == nullptr)
				root = n;
			else if(p->p->l == p)
				p->p->l = n;
			else
				p->p->r = n;
			if(l != nullptr)
				l->p = p;
			n->p = p->p;
			n->l = p;
			p->r = l;
			p->p = n;
		}

		static void rotateRight(AvlNode *&root, AvlNode *n)
		{
			auto p = n->p;
			auto r = n->r;
			int h1 = 0; //height of left subtree (initially: n->l)
			int h2 = 0; //height of middle subtree (initially: n->r)
			int h3 = 0; //height of right subtree (initially: p->r)
			size_t s1; //n->l->size
			size_t s2; //n->r->size
			size_t s3; //p->r->size

			//calculate sizes of the three subtrees and set the new size of p and n
			if(n->l != nullptr)
				s1 = n->l->size;
			else
				s1 = 0;
			s2 = n->size - s1 - 1;
			s3 = p->size - s1 - s2 - 2;
			n->size = s1 + s2 + s3 + 2;
			p->size = s2 + s3 + 1;

			//calculate the virtual height of the three subtrees and set the new balance factor of p and n
			if(n->b <= 0)
				h2 = -n->b;
			else
				h1 = n->b;
			h3 = max(h1, h2) + 1 - p->b;
			p->b = h2 - h3;
			n->b = h1 - max(h2, h3) - 1;

			//perform rotation
			if(p->p == nullptr)
				root = n;
			else if(p->p->l == p)
				p->p->l = n;
			else
				p->p->r = n;
			if(r != nullptr)
				r->p = p;
			n->p = p->p;
			n->r = p;
			p->l = r;
			p->p = n;
		}

		static size_t subtreeSize(AvlNode *n)
		{
			if(n == nullptr)
				return 0;
			else
				return n->size;
		}

		static AvlNode *leftmost(AvlNode *n)
		{
			if(n == nullptr)
				return nullptr;
			while(n->l != nullptr)
				n = n->l;
			return n;
		}

		static AvlNode *rightmost(AvlNode *n)
		{
			if(n == nullptr)
				return nullptr;
			while(n->r != nullptr)
				n = n->r;
			return n;
		}

		static AvlNode *successor(AvlNode *n)
		{
			if(n == nullptr)
				return nullptr;
			else if(n->r != nullptr) {
				n = n->r;
				while(n->l != nullptr)
					n = n->l;
				return n;
			}
			else {
				while(n->p != nullptr && n == n->p->r)
					n = n->p;
				return n->p;
			}
		}

		//returns true if 'index' is <= size of tree. in that case: 'index' == size of tree iff n == null
		//returns false if index is > size of tree. 'n' is null in that case.
		static bool findIndex(AvlNode *&n, size_t index)
		{
			size_t lsize;
			if((n == nullptr && index > 0) || (n != nullptr && index > n->size)) {
				n = nullptr;
				return false;
			}
			else if(n == nullptr && index == 0)
				return true;
			for(lsize = subtreeSize(n->l); n != nullptr && lsize != index; lsize = subtreeSize(n->l)) {
				if(lsize > index)
					n = n->l;
				else if(n->size > index) {
					index -= lsize + 1;
					n = n->r;
					lsize = subtreeSize(n->l);
				}
				else {
					n = nullptr;
					break;
				}
			}
			return true;
		}

		static size_t findUnused(AvlNode *&n, typename std::tuple_element<0, Tuple>::type offset)
		{
			size_t index = subtreeSize(n->l);
			for(;;) {
				if(static_cast<size_t>(std::get<0>(n->cells) + offset) > index) {
					index -= subtreeSize(n->l);
					n = n->l;
				}
				else {
					index++;
					n = n->r;
				}
				if(!n)
					return index;
				index += subtreeSize(n->l);
			}
		}

		template<size_t NKEY, typename... T> static bool findUpper(AvlNode *&n, size_t &index, const std::tuple<T...> &key)
		{
			int cmp;
			bool found = false;
			AvlNode *nodeCandidate = nullptr;
			size_t indexCandidate = subtreeSize(n);

			if(n == nullptr) {
				index = 0;
				return false;
			}

			index = subtreeSize(n->l);
			while(n != nullptr) {
				cmp = Compare<0, min(sizeof...(T), NKEY), typename AvlNode::Tuple, std::tuple<T...>>::exec(n->cells, key);
				if(cmp > 0) {
					nodeCandidate = n;
					indexCandidate = index;
					n = n->l;
					if(n != nullptr)
						index -= subtreeSize(n->r) + 1;
				}
				else {
					if(cmp == 0)
						found = true;
					n = n->r;
					if(n != nullptr)
						index += subtreeSize(n->l) + 1;
				}
			}
			n = nodeCandidate;
			index = indexCandidate;
			return found;
		}

		template<size_t NKEY, typename... T> static bool findLower(AvlNode *&n, size_t &index, const std::tuple<T...> &key)
		{
			int cmp;
			bool found = false;
			AvlNode *nodeCandidate = nullptr;
			size_t indexCandidate = 0;

			if(n == nullptr) {
				index = 0;
				return false;
			}

			index = subtreeSize(n->l);
			while(n != nullptr) {
				cmp = Compare<0, min(sizeof...(T), NKEY), typename AvlNode::Tuple, std::tuple<T...>>::exec(n->cells, key);
				if(cmp >= 0) {
					if(cmp == 0)
						found = true;
					nodeCandidate = n;
					indexCandidate = index;
					n = n->l;
					if(n != nullptr)
						index -= subtreeSize(n->r) + 1;
				}
				else {
					n = n->r;
					if(n != nullptr)
						index += subtreeSize(n->l) + 1;
				}
			}
			n = nodeCandidate;
			if(n != nullptr)
				index = indexCandidate;
			else
				index++;
			return found;
		}

		static void retracingInsert(AvlNode *&root, AvlNode *n)
		{
			AvlNode *p;

			for(p = n; p != nullptr; p = p->p)
				p->size++;

			for(p = n->p; p != nullptr; n = p, p = n->p) {
				if(n == p->l) {
					if(p->b == 1) {
						n->p->b++;
						if(n->b == -1) {
							rotateLeft(root, n->r);
							rotateRight(root, n->p);
						}
						else
							rotateRight(root, n);
						break;
					}
					else if(p->b == -1) {
						p->b = 0;
						break;
					}
					else
						p->b = 1;
				}
				else {
					if(p->b == -1) {
						n->p->b--;
						if(n->b == 1) {
							rotateRight(root, n->l);
							rotateLeft(root, n->p);
						}
						else
							rotateLeft(root, n);
						break;
					}
					else if(p->b == 1) {
						p->b = 0;
						break;
					}
					else
						p->b = -1;
				}
			}
		}

		static void retracingRemove(AvlNode *&root, AvlNode *n, bool r)
		{
			AvlNode *c;

			for(c = n; c != nullptr; c = c->p)
				c->size--;

			while(n != nullptr) {
				if(r) {
					if(n->b == 1) {
						c = n->l;
						n->b++;
						if(c->b == -1) {
							rotateLeft(root, c->r);
							rotateRight(root, c->p);
						}
						else if(c->b == 0) {
							rotateRight(root, c);
							break;
						}
						else
							rotateRight(root, c);
					}
					else if(n->b == 0) {
						n->b = 1;
						break;
					}
					else
						n->b = 0;
				}
				else {
					if(n->b == -1) {
						c = n->r;
						n->b--;
						if(c->b == 1) {
							rotateRight(root, c->l);
							rotateLeft(root, c->p);
						}
						else if(c->b == 0) {
							rotateLeft(root, c);
							break;
						}
						else
							rotateLeft(root, c);
					}
					else if(n->b == 0) {
						n->b = -1;
						break;
					}
					else
						n->b = 0;
				}
				if(n->p != nullptr && n == n->p->r)
					r = true;
				else
					r = false;
				n = n->p;
			}
		}

		static void insertNode(AvlNode *&root, AvlNode *pivot, AvlNode *n)
		{
			AvlNode *cur;
			if(pivot == nullptr) {
				if(root == nullptr) //set root
					root = n;
				else { //insert at end
					pivot = root;
					for(cur = root; cur->r != nullptr; cur = cur->r);
					cur->r = n;
					n->p = cur;
				}
			}
			else if(pivot->l == nullptr) { //insert before pivot: set left child
				pivot->l = n;
				n->p = pivot;
			}
			else { //insert before pivot: at rightmost position of left subtree of pivot
				for(cur = pivot->l; cur->r != nullptr; cur = cur->r);
				cur->r = n;
				n->p = cur;
			}
			retracingInsert(root, n);
		}

		static void removeNode(AvlNode *&root, AvlNode *x)
		{
			AvlNode *y;
			AvlNode *zp;
			bool zr = true;

			if(x->l != nullptr && x->r != nullptr) {
				y = x->l;
				if(y->r != nullptr) {
					//descend to rightmost node
					while(y->r != nullptr)
						y = y->r;
					zp = y->p;
					//put left child of y to y's position
					y->p->r = y->l;
					if(y->l != nullptr)
						y->l->p = y->p;
					//put y to x's position
					y->l = x->l;
					y->l->p = y;
				}
				else {
					zp = y;
					zr = false;
				}
				y->p = x->p;
				y->r = x->r;
				y->b = x->b;
				y->size = x->size;
				if(y->r != nullptr)
					y->r->p = y;
				if(y->p != nullptr) {
					if(y->p->l == x)
						y->p->l = y;
					else
						y->p->r = y;
				}
				else
					root = y;
			}
			else {
				zp = x->p;
				if(zp != nullptr && zp->l == x)
					zr = false;
				if(x->p == nullptr) { //x was the root node, set new root node
					if(x->l != nullptr) {
						root = x->l;
						x->l->p = nullptr;
					}
					else if(x->r != nullptr) {
						root = x->r;
						x->r->p = nullptr;
					}
					else
						root = nullptr;
				}
				else if(x->l != nullptr) { //x has only a left subtree and is not root, attach subtree to parent at x's position
					if(x->p->l == x)
						x->p->l = x->l;
					else
						x->p->r = x->l;
					x->l->p = x->p;
				}
				else if(x->r != nullptr) { //x has only a right subtree and is not root, attach subtree to parent at x's position
					if(x->p->l == x)
						x->p->l = x->r;
					else
						x->p->r = x->r;
					x->r->p = x->p;
				}
				else { //x has no subtree and is not root, remove x from parent
					if(x->p->l == x)
						x->p->l = nullptr;
					else
						x->p->r = nullptr;
				}
			}
			delete x;
			retracingRemove(root, zp, zr);
		}
	};

	template<size_t NKEY, typename... COLS> class Iterator {
		using RowTuple = std::tuple<COLS...>;
		using KeyTuple = typename TupleFront<RowTuple, NKEY>::TYPE;
		public:
			Iterator() :
					_begin(nullptr),
					_beginidx(-1),
					_end(nullptr),
					_endidx(-1),
					_cur(nullptr),
					_curidx(-1),
					_root(nullptr)
			{}

			Iterator(AvlNode<COLS...> *root, AvlNode<COLS...> *begin, size_t beginidx) :
					_begin(begin),
					_beginidx(beginidx),
					_end(nullptr),
					_endidx(AvlNode<COLS...>::subtreeSize(root)),
					_cur(begin),
					_curidx(beginidx),
					_root(root)
			{}

			Iterator(AvlNode<COLS...> *root, AvlNode<COLS...> *begin, size_t beginidx, AvlNode<COLS...> *end, size_t endidx) :
					_begin(begin),
					_beginidx(beginidx),
					_end(end),
					_endidx(endidx),
					_cur(begin),
					_curidx(beginidx),
					_root(root)
			{}

			void dump() const
			{

			}

			size_t index() const
			{
				return _curidx;
			}

			size_t count() const
			{
				return _endidx - _beginidx;
			}

			bool atEnd() const
			{
				return _cur == _end;
			}

			bool atBegin() const
			{
				return _cur == _begin;
			}

			bool atFront() const
			{
				return _cur == AvlNode<COLS...>::leftmost(_root);
			}

			bool atBack() const {
				return _cur == nullptr;
			}

			bool isNull() const
			{
				return !_cur;
			}

			void next()
			{
				assert(!atBack());
				next(_cur, _curidx);
			}

			void prev()
			{
				assert(!atFront());
				prev(_cur, _curidx);
			}

			void operator+=(ssize_t inc) {
				AvlNode<COLS...> *n = _root;
				if(AvlNode<COLS...>::findIndex(n, _curidx + inc))
					_curidx += inc;
				else
					_curidx = AvlNode<COLS...>::subtreeSize(_root);
				_cur = n;
			}

			void operator-=(ssize_t inc) {
				AvlNode<COLS...> *n = _root;
				if(AvlNode<COLS...>::findIndex(n, _curidx - inc))
					_curidx -= inc;
				else
					_curidx = AvlNode<COLS...>::subtreeSize(_root);
				_cur = n;
			}

/*			template<size_t N> void upper()
			{
				static_assert(N <= NKEY, "too many columns given");
				auto key = TupleFront<RowTuple, N>::get(_cur->cells);
				AvlNode<COLS...>::template findUpper<NKEY>(_cur, _curidx, key);
			}*/

			template<size_t COL> typename std::tuple_element<COL, RowTuple>::type cell()
			{
				static_assert(COL < sizeof...(COLS), "no such column");
				return std::get<COL>(_cur->cells);
			}

		private:
			void next(AvlNode<COLS...> *&n, size_t &index)
			{
				index++;
				if(n->r != nullptr) {
					n = n->r;
					while(n->l != nullptr)
						n = n->l;
				}
				else {
					while(n->p != nullptr && n == n->p->r)
						n = n->p;
					n = n->p;
				}
			}

			void prev(AvlNode<COLS...> *&n, size_t &index)
			{
				index--;
				if(n) {
					if(n->l != nullptr) {
						n = n->l;
						while(n->r != nullptr)
							n = n->r;
					}
					else {
						while(n->p != nullptr && n == n->p->l)
							n = n->p;
						n = n->p;
					}
				}
				else if(_root) {
					n = _root;
					while(n->r != nullptr)
						n = n->r;
				}
			}

			AvlNode<COLS...> *_begin;
			size_t _beginidx;
			AvlNode<COLS...> *_end;
			size_t _endidx;

			AvlNode<COLS...> *_cur;
			size_t _curidx;
			AvlNode<COLS...> *_root;
	};
}

template<size_t NKEY, typename... COLS> class AvlTable {
	static_assert(sizeof...(COLS) >= NKEY, "number of key columns exceeds number of total columns");

	public:
		using AvlNode = detail::AvlNode<COLS...>;
		using Iterator = detail::Iterator<NKEY, COLS...>;
		using RowTuple = std::tuple<COLS...>;
		using KeyTuple = typename detail::TupleFront<RowTuple, NKEY>::TYPE;

		AvlTable(const AvlTable &other) = delete;
		AvlTable &operator=(const AvlTable &other) = delete;

		AvlTable() :
				_root(nullptr)
		{}

		~AvlTable()
		{
			AvlNode::deleteNode(_root);
		}

		void clear()
		{
			AvlNode::deleteNode(_root);
			_root = nullptr;
		}

		template<size_t COL> typename std::tuple_element<COL, RowTuple>::type cellAt(size_t index) const
		{
			static_assert(COL < sizeof...(COLS), "no such column");
			AvlNode *n = _root;
			AvlNode::findIndex(n, index);
			if(n == NULL)
				throw 1;
			return std::get<COL>(n->cells);
		}

		template<size_t COL, typename... K> typename std::tuple_element<COL, RowTuple>::type cell(K... argkey) const
		{
			static_assert(COL < sizeof...(COLS), "no such column");
			AvlNode *n = _root;
			size_t index;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			if(!AvlNode::template findLower<NKEY>(n, index, key))
				return typename std::tuple_element<COL, RowTuple>::type();
			else if(n == NULL)
				return typename std::tuple_element<COL, RowTuple>::type();
			return std::get<COL>(n->cells);
		}

#if 0
		//TODO we need to disable this method, if a) NKEY==0 b) the first column is not an integral type
		typename std::tuple_element<0, KeyTuple>::type unused(typename std::tuple_element<0, KeyTuple>::type offset = typename std::tuple_element<0, KeyTuple>::type())
		{
			AvlNode *n = _root;
			size_t index = AvlNode::findUnused(n, offset);
			return index + offset;
		}
#endif

#if 0
		typename std::tuple_element<0, KeyTuple>::type acquire( typename std::tuple_element<0, KeyTuple>::type offset = typename std::tuple_element<0, KeyTuple>::type())
		{
			AvlNode *pivot = _root;
			size_t index = AvlNode::findUnused(n);
			RowTuple row(index + offset, cols...);
			AvlNode *n = new AvlNode(index + offset);
			AvlNode::insertNode(pivot, n);
		}
#endif

		template<typename... K> bool contains(K... argkey) {
			static_assert(sizeof...(K) <= NKEY, "given key exceeds number of key columns");
			AvlNode *n = _root;
			size_t index;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			return AvlNode::template findLower<NKEY>(n, index, key);
		}

		size_t insertAt(size_t index, const COLS&... cols)
		{
			AvlNode *pivot = _root;

			if(!AvlNode::findIndex(pivot, index))
				throw 1; //index out of bounds TODO don't throw
			
			AvlNode *n = new AvlNode(cols...);
			AvlNode::insertNode(_root, pivot, n);
			return index;
		}

		size_t append(const COLS&...cols)
		{
			AvlNode *pivot = _root;
			size_t index;
			RowTuple row(cols...);

			AvlNode::template findUpper<NKEY>(pivot, index, row);
			AvlNode *n = new AvlNode(cols...);
			AvlNode::insertNode(_root, pivot, n);
			return index;
		}

		size_t prepend(const COLS&...cols)
		{
			AvlNode *pivot = _root;
			size_t index;
			RowTuple row(cols...);

			AvlNode::template findLower<NKEY>(pivot, index, row);
			AvlNode *n = new AvlNode(cols...);
			AvlNode::insertNode(_root, pivot, n);
			return index;
		}

		size_t insert(const COLS&...cols)
		{
			AvlNode *pivot = _root;
			size_t index;
			RowTuple row(cols...);

			AvlNode::template findUpper<NKEY>(pivot, index, row);
			AvlNode *n = new AvlNode(cols...);
			AvlNode::insertNode(_root, pivot, n);
			return index;
		}

		void putAt(size_t index, const COLS&... cols)
		{
			AvlNode *pivot = _root;
			RowTuple row(cols...);
			if(!AvlNode::findIndex(pivot, index))
				throw 1; //TODO don't throw; what to do? idea would be to return bool, but see also other xAt functions
			pivot->cells = row;
		}

		size_t put(const COLS&... cols)
		{
			AvlNode *pivot = _root;
			size_t index;
			RowTuple row(cols...);
			if(AvlNode::template findLower<NKEY>(pivot, index, row))
				pivot->cells = row;
			else {
				AvlNode *n = new AvlNode(cols...);
				AvlNode::insertNode(_root, pivot, n);
			}
			return index;
		}

		size_t removeAt(size_t index, size_t n = 1)
		{
			for(size_t i = 0; i < n; i++) { //TODO this is optimizable... idea: we should get the new node at position 'index' from removeNode(), so we just need to delete that node until there is no further node or n has reached 0.
				AvlNode *pivot = _root;
				if(!AvlNode::findIndex(pivot, index))
					return i;
				AvlNode::removeNode(_root, pivot);
			}
			return n;
		}

		template<typename... K> size_t remove(K... argkey)
		{
			AvlNode *bn = _root;
			AvlNode *en = _root;
			size_t bi;
			size_t ei;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			AvlNode::template findLower<NKEY>(bn, bi, key);
			AvlNode::template findUpper<NKEY>(en, ei, key);
			return removeAt(bi, ei - bi);
		}

		Iterator at(size_t index) const
		{
			AvlNode *n = _root;
			if(!AvlNode::findIndex(n, index))
				throw 1;
			return Iterator(_root, n, index);
		}

		template<typename... K> Iterator single(K... argkey) const
		{
			static_assert(sizeof...(K) <= NKEY, "given key exceeds number of key columns");
			AvlNode *n = _root;
			size_t index;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			if(!(AvlNode::template findLower<NKEY>(n, index, key)))
				return Iterator();
			return Iterator(_root, n, index, AvlNode::successor(n), index + 1);
		}

		template<typename... K> Iterator all(K... argkey) const
		{
			static_assert(sizeof...(K) <= NKEY, "given key exceeds number of key columns");
			AvlNode *bn = _root;
			AvlNode *en = _root;
			size_t bi;
			size_t ei;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			AvlNode::template findLower<NKEY>(bn, bi, key);
			AvlNode::template findUpper<NKEY>(en, ei, key);
			return Iterator(_root, bn, bi, en, ei);

		}

		template<typename... K> Iterator lower(K... argkey) const
		{
			static_assert(sizeof...(K) <= NKEY, "given key exceeds number of key columns");
			AvlNode *n = _root;
			size_t index;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			AvlNode::template findLower<NKEY>(n, index, key); //TODO is this correct? thoughts: remove if() clause
			return Iterator(_root, n, index);
		}

		template<typename... K> Iterator upper(K... argkey) const
		{
			static_assert(sizeof...(K) <= NKEY, "given key exceeds number of key columns");
			AvlNode *n = _root;
			size_t index;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			AvlNode::template findUpper<NKEY>(n, index, key);
			return Iterator(_root, n, index);
		}

		template<typename... K> size_t count(K... argkey) const
		{
			static_assert(sizeof...(K) <= NKEY, "given key exceeds number of key columns");
			AvlNode *bn = _root;
			AvlNode *en = _root;
			size_t bi;
			size_t ei;
			typename detail::TupleFront<RowTuple, sizeof...(K)>::TYPE key(argkey...);
			if(!(AvlNode::template findLower<NKEY>(bn, bi, key)))
				return 0;
			AvlNode::template findUpper<NKEY>(en, ei, key);
			return ei - bi;
		}

		Iterator begin() {
			AvlNode *n = _root;
			AvlNode *p = nullptr;
			while(n != nullptr) {
				p = n;
				n = n->l;
			}
			return Iterator(_root, p, 0, nullptr, subtreeSize(_root));
		}

		size_t size() const {
			return AvlNode::subtreeSize(_root);
		}

	protected:

		AvlNode *_root;
};

/*
template<typename... COLS> class IdTable : public AvlTable<1, detail::id_t, COLS...> {
	public:
		static constexpr const detail::id_t FirstId = 1;
		static constexpr const detail::id_t NullId = 0;

		detail::id_t insertNew(const COLS&... cols)
		{
			AvlNode *n = rightmost(_root);
			size_t count = size();

			if(n == nullptr) {
				insert(FirstId, cols...);
				return FirstId;
			}
			else if(std::get<0>(n->cells) == FirstId + count - 1) {
				insert(FirstId + count, cols...);
				return FirstId + count;
			}

			size_t min = 0;
			size_t max = count - 1;
			while(true) {
				size_t idx = (min + max) / 2;
				n = findIndex(_root, idx);
				if(std::get<0>(n->cells) == FirstId + idx - 1)
					min = idx;

			}
		}
};
*/
