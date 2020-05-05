#pragma once

#include "types.h"
#include "spec.h"

class XmlFormat {
	public:
		XmlFormat(SyncFrontend *fe);

		void load(const QString &filename);
		void save(const QString &filename);

	private:
		SyncFrontend *_fe;
};

