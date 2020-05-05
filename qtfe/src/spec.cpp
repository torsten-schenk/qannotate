#include <QtEndian>

#include "common/qHexdump.h"

#define IMPLEMENTATION
#include "spec.h"
#undef IMPLEMENTATION

namespace spec {
	namespace {
		void dumpRecursive(const DeclValue *decl, size_t varidx, size_t index, size_t total, const void *handle, size_t indent)
		{
			const DeclVar *var = decl->vars + varidx;
			for(size_t i = 0; i < indent; i++)
				printf("  ");
			printf("%s", qPrintable(name2string(var->name)));
			if(total > 1)
				printf(" [%zu]", index);
			if(var->child) {
				printf("\n");
				const void *child = var->u.accessor->constChild(handle, varidx, index);
				size_t nvar = decl->countVar(var->child);
				for(size_t i = 0; i < nvar; i++) {
					const DeclVar *subvar = decl->vars + var->child + i;
					for(size_t k = 0; k < subvar->n; k++) {
						dumpRecursive(decl, var->child + i, k, subvar->n, child, indent + 1);
					}
				}
			}
			else {
				printf(" = ");
				switch(var->u.type->builtin) {
					case BuiltinFlexString: printf("'%s'", qPrintable(*reinterpret_cast<const QString*>(handle))); break;
					case BuiltinU8: printf("%u", *reinterpret_cast<const quint8*>(handle)); break;
					case BuiltinU16: printf("%u", *reinterpret_cast<const quint16*>(handle)); break;
					case BuiltinU32: printf("%u", *reinterpret_cast<const quint32*>(handle)); break;
					case BuiltinU64: printf("%llu", *reinterpret_cast<const quint64*>(handle)); break;
					case BuiltinBoolean: printf("%s", *reinterpret_cast<const bool*>(handle) ? "true" : "false"); break;
					default: Q_ASSERT(false); break;
				}

				printf("\n");
			}
		}
	}

	void Value::dump() const
	{
		printf("value dump [%s]:\n", qPrintable(name2string(decl()->name)));
		size_t nvar = decl()->countVar(0);
		for(size_t i = 0; i < nvar; i++) {
			const DeclVar *var = decl()->vars + i;
			size_t n = var->n;
			if(!n)
				n = decl()->accessor->size(this, i);
			for(size_t k = 0; k < n; k++) {
				const void *child = decl()->accessor->constChild(this, i, k);
				dumpRecursive(decl(), i, k, n, child, 2);
			}
		}
	}

	namespace {
		void constWalkRecursive(const DeclValue *decl, const ValueAccessor *accessor, const void *handle, const DeclVar *var, size_t nvar, const Value::ConstWalker &walker)
		{
			for(size_t i = 0; i < nvar; i++) {
				size_t n = var->n;
				if(!n)
					n = accessor->size(handle, i);
				if(walker.down)
					walker.down(var->name, n);
				for(size_t k = 0; k < n; k++) {
					const void *child = accessor->constChild(handle, i, k);
					if(var->child) {
						if(walker.begin)
							walker.begin(k, var->u.accessor, child);
						constWalkRecursive(decl, var->u.accessor, child, decl->vars + var->child, decl->countVar(var->child), walker);
						if(walker.end)
							walker.end();
					}
					else {
						if(walker.var)
							walker.var(k, var->u.type, child);
					}
				}
				if(walker.up)
					walker.up();
				var++;
			}
		}
	}

	void Value::constWalk(const ConstWalker &walker) const
	{
		size_t nvar = decl()->countVar(0);
		if(walker.down)
			walker.down(decl()->name, 1);
		if(walker.begin)
			walker.begin(0, decl()->accessor, this);
		constWalkRecursive(decl(), decl()->accessor, this, decl()->vars, nvar, walker);
		if(walker.end)
			walker.end();
		if(walker.up)
			walker.up();
	}

	void Value::walk(const Walker &walker)
	{}

	namespace {
		bool eqRecursive(const DeclValue *decl, const ValueAccessor *accessor, const void *a, const void *b, const DeclVar *var, size_t nvar)
		{
			for(size_t i = 0; i < nvar; i++) {
				size_t n = accessor->size(a, i);
				if(accessor->size(b, i) != n)
					return false;
				for(size_t k = 0; k < n; k++) {
					if(var->child) {
						if(!eqRecursive(decl, var->u.accessor, accessor->constChild(a, i, k), accessor->constChild(b, i, k), decl->vars + var->child, decl->countVar(var->child)))
							return false;
					}
					else {
						if(!var->u.type->equal(accessor->constChild(a, i, k), accessor->constChild(b, i, k)))
							return false;
					}
				}
				var++;
			}
			return true;
		}
	}

	bool Value::operator==(const Value &other) const
	{
		if(decl() != other.decl())
			return false;
		size_t nvar = decl()->countVar(0);
		return eqRecursive(decl(), decl()->accessor, this, &other, decl()->vars, nvar);
	}

	bool Value::operator!=(const Value &other) const
	{
		if(decl() != other.decl())
			return true;
		size_t nvar = decl()->countVar(0);
		return !eqRecursive(decl(), decl()->accessor, this, &other, decl()->vars, nvar);
	}

	const DeclMeta *DeclMeta::version(const QString &version)
	{
		if(version == "1.0")
			return &v1_0::meta;
		else
			return nullptr;
	}
}

