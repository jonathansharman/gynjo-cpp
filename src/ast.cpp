//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "ast.hpp"

namespace gynjo::ast {
	assign::assign(tok::sym symbol, ptr rhs) : symbol{symbol}, rhs{std::move(rhs)} {}

	assign::assign(assign const& that) : symbol{that.symbol}, rhs{make_node(*that.rhs)} {}

	assign& assign::operator=(assign const& that) {
		symbol = that.symbol;
		rhs = make_node(*that.rhs);
		return *this;
	}

	bool assign::operator==(assign const& that) const {
		return symbol == that.symbol && *rhs == *that.rhs;
	}

	add::add(ptr addend1, ptr addend2) : addend1{std::move(addend1)}, addend2{std::move(addend2)} {}

	add::add(add const& that) : addend1{make_node(*that.addend1)}, addend2{make_node(*that.addend2)} {}

	add& add::operator=(add const& that) {
		addend1 = make_node(*that.addend1);
		return *this;
	}

	bool add::operator==(add const& that) const {
		return *addend1 == *that.addend1 && *addend2 == *that.addend2;
	}

	neg::neg(ptr expr) : expr{std::move(expr)} {}

	neg::neg(neg const& that) : expr{make_node(*that.expr)} {}

	neg& neg::operator=(neg const& that) {
		expr = make_node(*that.expr);
		return *this;
	}

	bool neg::operator==(neg const& that) const {
		return *expr == *that.expr;
	}

	sub::sub(ptr minuend, ptr subtrahend) : minuend{std::move(minuend)}, subtrahend{std::move(subtrahend)} {}

	sub::sub(sub const& that) : minuend{make_node(*that.minuend)}, subtrahend{make_node(*that.subtrahend)} {}

	sub& sub::operator=(sub const& that) {
		minuend = make_node(*that.minuend);
		subtrahend = make_node(*that.subtrahend);
		return *this;
	}

	bool sub::operator==(sub const& that) const {
		return *minuend == *that.minuend && *subtrahend == *that.subtrahend;
	}

	cluster::cluster(std::unique_ptr<std::vector<node>> items, std::vector<connector> connectors)
		: items{std::move(items)}, connectors{std::move(connectors)} {}

	cluster::cluster(cluster const& that)
		: items{std::make_unique<std::vector<node>>(*that.items)}, connectors{that.connectors} {}

	cluster& cluster::operator=(cluster const& that) {
		items = std::make_unique<std::vector<node>>(*that.items);
		connectors = that.connectors;
		return *this;
	}

	bool cluster::operator==(cluster const& that) const {
		return items == that.items;
	}

	tup::tup() : elems{std::make_unique<std::vector<node>>()} {}
	tup::tup(std::unique_ptr<std::vector<node>> elems) : elems{std::move(elems)} {}

	tup::tup(tup const& that) {
		elems = std::make_unique<std::vector<node>>(*that.elems);
	}

	tup& tup::operator=(tup const& that) {
		elems = std::make_unique<std::vector<node>>(*that.elems);
		return *this;
	}

	bool tup::operator==(tup const& that) const {
		return *elems == *that.elems;
	}

	lambda::lambda(ptr params, ptr body) : params{std::move(params)}, body{std::move(body)} {}

	lambda::lambda(lambda const& that) : params{make_node(*that.params)}, body{make_node(*that.body)} {}

	lambda& lambda::operator=(lambda const& that) {
		params = make_node(*that.params);
		body = make_node(*that.body);
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
				if (!cluster.items->empty()) { result += to_string(cluster.items->front()); }
				for (std::size_t i = 0; i < cluster.connectors.size(); ++i) {
					auto item_string = to_string((*cluster.items)[i + 1]);
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
				if (!tup.elems->empty()) {
					result += to_string(tup.elems->front());
					for (auto it = tup.elems->begin() + 1; it != tup.elems->end(); ++it) {
						result += ", " + to_string(*it);
					}
				}
				result += ")";
				return result;
			},
			[](tok::num const& num) { return num.rep; },
			[](tok::sym const& sym) { return sym.name; });
	}
}
