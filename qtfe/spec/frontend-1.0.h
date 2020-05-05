#pragma once

#include "../src/frontend.h"

namespace spec { namespace v1_0 {
	class DirFrontend : public AbstractDirFrontend {
		public:
			DirFrontend(const QDir &root);

		protected:
			virtual void actionLoad() override;
			virtual void actionSave() override;
			virtual void actionErase(const QByteArray &key, Value *value) override;
			virtual void actionModify(const QByteArray &key, Value *value) override;

		private:
			void loadPersons();
			void loadLocations();
			void loadBibliography();
			void loadPhilComments();
			void loadHistComments();
			void loadLetters();
			void loadIntro();
			void storePerson(const QString &name, quint64 id, bool create = true);
			void storeLocation(LocationType type, const QString &name, quint64 id, bool create = true);
			void storeBibliography(BibliographyType type, const QString &name, quint64 id, bool create = true);
			void storePhilComment(const QString &name, quint64 id, bool create = true);
			void storeHistComment(HistCommentType type, const QString &name, quint64 id, bool create = true);
			void storeLetter(quint64 book, quint64 letter, quint64 id, bool create = true);
			void storeIntro(const QString &name, quint64 id, bool create = true);

			QDir categoryDir(const QString &category, bool *exist); //exist: input: if true, dir will be created; output: if input is false, output indicates, whether dir exists
			QDir categoryDir(const QStringList &category, bool *exist); //exist: input: if true, dir will be created; output: if input is false, output indicates, whether dir exists
			QDir letterDir(quint64 book, quint64 letter, bool *exist);

			QMap<quint64, quint64> _id2number;
	};
}}

