//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "ast.hpp"

namespace gynjo::ast {
	lambda::lambda(ptr params, ptr body) : params{std::move(params)}, body{std::move(body)} {}

	lambda::lambda(lambda const& that) : params(clone(*that.params)), body(clone(*that.body)) {}

	lambda& lambda::operator=(lambda const& that) {
		params = clone(*that.params);
		body = clone(*that.body);
		return *this;
	}

	auto to_string(node const& node) -> std::string {
		return match(
			node,
			[](assign const& assign) {
				return "(" + ast::to_string(assign.symbol) + " = " + to_string(*assign.rhs) + ")";
			},
			[](add const& add) { return fmt::format("({} + {})", to_string(*add.addend1), to_string(*add.addend2)); },
			[](neg const& neg) { return fmt::format("(-{})", to_string(*neg.expr)); },
			[](sub const& sub) { return fmt::format("({} - {})", to_string(*sub.minuend), to_string(*sub.subtrahend)); },
			[](cluster const& cluster) {
				std::string result = "(";
				if (!cluster.items.empty()) { result += to_string(*cluster.items.front()); }
				for (std::size_t i = 0; i < cluster.connectors.size(); ++i) {
					auto item_string = to_string(*cluster.items[i + 1]);
					switch (cluster.connectors[i]) {
						case cluster::connector::adj_paren:
							result += " (" + item_string + ")";
							break;
						case cluster::connector::adj_nonparen:
							result += " " + item_string;
							break;
						case cluster::connector::mul:
							result += " * " + item_string;
							break;
						case cluster::connector::div:
							result += " / " + item_string;
							break;
						case cluster::connector::exp:
							result += " ^ " + item_string;
							break;
					}
					result += ")";
				}
				return result;
			},
			[](lambda const& f) { return fmt::format("({} -> {})", to_string(*f.params), to_string(*f.body)); },
			[](tup const& tup) {
				std::string result = "(";
				if (!tup.elems.empty()) {
					result += to_string(*tup.elems.front());
					for (auto it = tup.elems.begin() + 1; it != tup.elems.end(); ++it) {
						result += ", " + to_string(**it);
					}
				}
				result += ")";
				return result;
			},
			[](tok::num const& num) { return num.rep; },
			[](tok::sym const& sym) { return sym.name; });
	}

	auto clone(ast::node const& node) -> ast::ptr {
		return match(
			node,
			[](assign const& assign) {
				return make_node(ast::assign{assign.symbol, clone(*assign.rhs)});
			},
			[](add const& add) {
				return make_node(ast::add{clone(*add.addend1), clone(*add.addend2)});
			},
			[](neg const& neg) { return make_node(ast::neg{clone(*neg.expr)}); },
			[](sub const& sub) {
				return make_node(ast::sub{clone(*sub.minuend), clone(*sub.subtrahend)});
			},
			[](cluster const& c) {
				std::vector<ptr> items;
				for (auto const& item : c.items) {
					items.push_back(clone(*item));
				}
				return make_node(cluster{std::move(items), c.connectors});
			},
			[](lambda const& f) {
				return make_node(ast::lambda{clone(*f.params), clone(*f.body)});
			},
			[](tup const& tup) {
				ast::tup result;
				for (auto const& elem : tup.elems) {
					result.elems.push_back(clone(*elem));
				}
				return make_node(std::move(result));
			},
			[](tok::num const& num) { return make_node(num); },
			[](tok::sym const& sym) { return make_node(sym); });
	}
}
