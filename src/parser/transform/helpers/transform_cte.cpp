#include <duckdb/common/enums/set_operation_type.hpp>
#include "duckdb/common/exception.hpp"
#include "duckdb/parser/statement/select_statement.hpp"
#include "duckdb/parser/transformer.hpp"
#include "duckdb/parser/query_node/set_operation_node.hpp"
#include "duckdb/parser/expression/star_expression.hpp"
#include "duckdb/parser/query_node/recursive_cte_node.hpp"

using namespace duckdb;
using namespace std;

void Transformer::TransformCTE(PGWithClause *de_with_clause, SelectStatement &select) {
	// TODO: might need to update in case of future lawsuit
	assert(de_with_clause);

//	if (de_with_clause->recursive) {
//		throw NotImplementedException("Recursive CTEs not supported");
//	}
	assert(de_with_clause->ctes);
	for (auto cte_ele = de_with_clause->ctes->head; cte_ele != NULL; cte_ele = cte_ele->next) {
		auto cte = reinterpret_cast<PGCommonTableExpr *>(cte_ele->data.ptr_value);
		// lets throw some errors on unsupported features early
//		if (cte->cterecursive) {
//			throw NotImplementedException("Recursive CTEs not supported");
//		}
		if (cte->aliascolnames) {
			throw NotImplementedException("Column name aliases not supported in CTEs");
		}
		if (cte->ctecolnames) {
			throw NotImplementedException("Column name setting not supported in CTEs");
		}
		if (cte->ctecoltypes) {
			throw NotImplementedException("Column type setting not supported in CTEs");
		}
		if (cte->ctecoltypmods) {
			throw NotImplementedException("Column type modification not supported in CTEs");
		}
		if (cte->ctecolcollations) {
			throw NotImplementedException("CTE collations not supported");
		}
		// we need a query
		if (!cte->ctequery || cte->ctequery->type != T_PGSelectStmt) {
			throw Exception("A CTE needs a SELECT");
		}

//		auto cte_select = TransformSelectNode((PGSelectStmt *)cte->ctequery);
        unique_ptr<QueryNode> cte_select;

		if(cte->cterecursive || de_with_clause->recursive) {
		    cte_select = TransformRecursiveCTE(cte);
		} else {
		    cte_select = TransformSelectNode((PGSelectStmt *)cte->ctequery);
		}

		if (!cte_select) {
			throw Exception("A CTE needs a SELECT");
		}
		auto cte_name = string(cte->ctename);

		auto it = select.cte_map.find(cte_name);
		if (it != select.cte_map.end()) {
			// can't have two CTEs with same name
			throw Exception("A CTE needs an unique name");
		}
		select.cte_map[cte_name] = move(cte_select);
	}
}

unique_ptr<QueryNode> Transformer::TransformRecursiveCTE(PGCommonTableExpr *cte) {
    auto stmt = (PGSelectStmt *)cte->ctequery;

    if(!stmt->all) {
        throw Exception("UNION semantics for recursive CTEs is not implemented.");
    }

    unique_ptr<QueryNode> node;
    switch (stmt->op) {
        case PG_SETOP_UNION:
        case PG_SETOP_EXCEPT:
        case PG_SETOP_INTERSECT: {
            node = make_unique<RecursiveCTENode>();
            auto result = (RecursiveCTENode *)node.get();
            result->ctename = string(cte->ctename);
            result->union_all = stmt->all;
            result->left = TransformSelectNode(stmt->larg);
            result->right = TransformSelectNode(stmt->rarg);

            if (!result->left || !result->right) {
                throw Exception("Failed to transform setop children.");
            }

            result->select_distinct = true;
            switch (stmt->op) {
                case PG_SETOP_UNION:
                    result->select_distinct = !stmt->all;
                    break;
                default:
                    throw Exception("Unexpected setop type");
            }
            // if we compute the distinct result here, we do not have to do this in
            // the children. This saves a bunch of unnecessary DISTINCTs.
            if (result->select_distinct) {
                result->left->select_distinct = false;
                result->right->select_distinct = false;
            }
            break;
        }
        default:
            throw NotImplementedException("Statement type %d not implemented!", stmt->op);
    }
    // transform the common properties
    // both the set operations and the regular select can have an ORDER BY/LIMIT attached to them
    TransformOrderBy(stmt->sortClause, node->orders);
    if (stmt->limitCount) {
        node->limit = TransformExpression(stmt->limitCount);
    }
    if (stmt->limitOffset) {
        node->offset = TransformExpression(stmt->limitOffset);
    }
    return node;
}