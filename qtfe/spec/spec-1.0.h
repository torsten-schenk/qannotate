//NOT meant to be included directly! src/spec.h includes this file at the correct location
#ifndef _IN_SPEC_H
#error "Don't include this file directly"
#endif

#include "spec-1.0.gen.h"

namespace v1_0 {
	static inline QString letterToName(int number) {
		return QString("Epistula %1").arg(number);
	}

	static inline QString bookToName(int number) {
		return QString("Liber %1").arg(toRomanNumeral(number));
	}

	static inline const DeclType *optionValueType(const DeclValue *decl) {
		if(decl->nVars == 1 && !decl->vars[0].child && decl->vars[0].u.type->isEnum())
			return decl->vars[0].u.type;
		else
			return nullptr;
	}

	static inline void setOptionValue(Value *value, size_t enmval) {
		const DeclValue *decl = value->decl();
		const DeclType *type = optionValueType(decl);
		if(!type || enmval > type->nEnumValues)
			throw 1; //TODO
		void *handle = decl->accessor->child(value, 0, 0);
		type->uintPut(handle, enmval);
	}

	static inline size_t optionValue(const Value *value) {
		const DeclValue *decl = value->decl();
		const DeclType *type = optionValueType(decl);
		if(!type)
			throw 1; //TODO
		const void *handle = decl->accessor->constChild(value, 0, 0);
		type->uintGet(handle);
	}
}

