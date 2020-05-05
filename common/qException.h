#pragma once

#include <QString>
#ifdef DUMP_EXCEPTION_CTOR
#ifdef __linux__
#include "3rdparty/backward-cpp/backward.hpp"
#endif
#endif

class Exception {
	public:
		Exception(const QString &message) : _message(message) {
#ifdef DUMP_EXCEPTION_CTOR
#ifdef __linux__
			backward::StackTrace st;
			st.load_here(32, 3);
			backward::Printer p;
			p.print(st);
#endif
#endif
		}

		QString message() const {
			return _message;
		}

	private:
		QString _message;
};

