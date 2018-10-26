//===--------------------------------------------------------------------===//
// scatter.cpp
// Description: This file contains the implementation of the scatter operations
//===--------------------------------------------------------------------===//

#include "common/types/vector_operations.hpp"
#include "common/vector_operations/scatter_loops.hpp"

#include "common/operator/aggregate_operators.hpp"
#include "common/operator/constant_operators.hpp"
#include "common/operator/numeric_binary_operators.hpp"

using namespace duckdb;
using namespace std;

template <class OP>
static void generic_scatter_loop(Vector &source, Vector &dest) {
	if (dest.type != TypeId::POINTER) {
		throw InvalidTypeException(dest.type,
		                           "Cannot scatter to non-pointer type!");
	}
	switch (source.type) {
	case TypeId::TINYINT:
		scatter_templated_loop<int8_t, OP>(source, dest);
		break;
	case TypeId::SMALLINT:
		scatter_templated_loop<int16_t, OP>(source, dest);
		break;
	case TypeId::DATE:
	case TypeId::INTEGER:
		scatter_templated_loop<int32_t, OP>(source, dest);
		break;
	case TypeId::TIMESTAMP:
	case TypeId::BIGINT:
		scatter_templated_loop<int64_t, OP>(source, dest);
		break;
	case TypeId::DECIMAL:
		scatter_templated_loop<double, OP>(source, dest);
		break;
	case TypeId::POINTER:
		scatter_templated_loop<uint64_t, OP>(source, dest);
		break;
	default:
		throw NotImplementedException("Unimplemented type for scatter");
	}
}

void VectorOperations::Scatter::Set(Vector &source, Vector &dest) {
	generic_scatter_loop<operators::PickLeft>(source, dest);
}

void VectorOperations::Scatter::Add(Vector &source, Vector &dest) {
	generic_scatter_loop<operators::Add>(source, dest);
}

void VectorOperations::Scatter::Max(Vector &source, Vector &dest) {
	generic_scatter_loop<operators::Max>(source, dest);
}

void VectorOperations::Scatter::Min(Vector &source, Vector &dest) {
	generic_scatter_loop<operators::Min>(source, dest);
}

void VectorOperations::Scatter::AddOne(Vector &source, Vector &dest) {
	generic_scatter_loop<operators::AddOne>(source, dest);
}

void VectorOperations::Scatter::SetFirst(Vector &source, Vector &dest) {
	generic_scatter_loop<operators::PickRight>(source, dest);
}