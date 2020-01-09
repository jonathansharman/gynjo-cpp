//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "expr.hpp"

#include "stmt.hpp"

namespace gynjo {
	block::block() : stmts{std::make_unique<std::vector<stmt>>()} {}

	block::block(block const& that) : stmts{std::make_unique<std::vector<stmt>>(*that.stmts)} {}

	block::~block() = default;

	auto block::operator=(block const& that) -> block& {
		stmts = std::make_unique<std::vector<stmt>>(*that.stmts);
		return *this;
	}

	auto block::operator==(block const& that) const noexcept -> bool {
		return *stmts == *that.stmts;
	}

	auto cond::operator==(cond const& that) const noexcept -> bool {
		return *test == *that.test && *true_expr == *that.true_expr && *false_expr == *that.false_expr;
	}

	auto and_::operator==(and_ const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto or_::operator==(or_ const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto not_::operator==(not_ const& that) const noexcept -> bool {
		return *expr == *that.expr;
	}

	auto eq::operator==(eq const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto neq::operator==(neq const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto approx::operator==(approx const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto lt::operator==(lt const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto leq::operator==(leq const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto gt::operator==(gt const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto geq::operator==(geq const& that) const noexcept -> bool {
		return *left == *that.left && *right == *that.right;
	}

	auto add::operator==(add const& that) const noexcept -> bool {
		return *addend1 == *that.addend1 && *addend2 == *that.addend2;
	}

	auto sub::operator==(sub const& that) const noexcept -> bool {
		return *minuend == *that.minuend && *subtrahend == *that.subtrahend;
	}

	cluster::cluster(std::vector<bool> negations, std::unique_ptr<std::vector<expr>> items, std::vector<connector> connectors)
		: negations{std::move(negations)}, items{std::move(items)}, connectors{std::move(connectors)} {}

	cluster::cluster(cluster const& that)
		: negations{that.negations}, items{std::make_unique<std::vector<expr>>(*that.items)}, connectors{that.connectors} {}

	auto cluster::operator=(cluster const& that) -> cluster& {
		negations = that.negations;
		items = std::make_unique<std::vector<expr>>(*that.items);
		connectors = that.connectors;
		return *this;
	}

	auto cluster::operator==(cluster const& that) const noexcept -> bool {
		return items == that.items;
	}

	tup_expr::tup_expr() : elems{std::make_unique<std::vector<expr>>()} {}
	tup_expr::tup_expr(std::unique_ptr<std::vector<expr>> elems) : elems{std::move(elems)} {}

	tup_expr::tup_expr(tup_expr const& that) {
		elems = std::make_unique<std::vector<expr>>(*that.elems);
	}

	auto tup_expr::operator=(tup_expr const& that) -> tup_expr& {
		elems = std::make_unique<std::vector<expr>>(*that.elems);
		return *this;
	}

	auto tup_expr::operator==(tup_expr const& that) const noexcept -> bool {
		return *elems == *that.elems;
	}

	list_expr::list_expr() : elems{std::make_unique<std::deque<expr>>()} {}
	list_expr::list_expr(std::unique_ptr<std::deque<expr>> elems) : elems{std::move(elems)} {}

	list_expr::list_expr(list_expr const& that) {
		elems = std::make_unique<std::deque<expr>>(*that.elems);
	}

	auto list_expr::operator=(list_expr const& that) -> list_expr& {
		elems = std::make_unique<std::deque<expr>>(*that.elems);
		return *this;
	}

	auto list_expr::operator==(list_expr const& that) const noexcept -> bool {
		return *elems == *that.elems;
	}

	auto lambda::operator==(lambda const& that) const noexcept -> bool {
		return *params == *that.params && body == that.body;
	}

	auto to_string(expr const& expr) -> std::string {
		using namespace std::string_literals;
		return match(
			expr.value,
			[](cond const& cond) {
				return fmt::format(
					"({} ? {} : {})", to_string(*cond.test), to_string(*cond.true_expr), to_string(*cond.false_expr));
			},
			[](block const& block) {
				std::string result = "{ ";
				if (!block.stmts->empty()) {
					result += to_string(block.stmts->front());
					for (auto it = block.stmts->begin() + 1; it != block.stmts->end(); ++it) {
						result += "; " + to_string(*it);
					}
				}
				result += " }";
				return result;
			},
			[](and_ const& and_) { return fmt::format("({} and {})", to_string(*and_.left), to_string(*and_.right)); },
			[](or_ const& or_) { return fmt::format("({} or {})", to_string(*or_.left), to_string(*or_.right)); },
			[](not_ const& not_) { return fmt::format("(not {})", to_string(*not_.expr)); },
			[](eq const& eq) { return fmt::format("({} == {})", to_string(*eq.left), to_string(*eq.right)); },
			[](neq const& neq) { return fmt::format("({} != {})", to_string(*neq.left), to_string(*neq.right)); },
			[](approx const& approx) {
				return fmt::format("({} ~ {})", to_string(*approx.left), to_string(*approx.right));
			},
			[](lt const& lt) { return fmt::format("({} < {})", to_string(*lt.left), to_string(*lt.right)); },
			[](leq const& leq) { return fmt::format("({} <= {})", to_string(*leq.left), to_string(*leq.right)); },
			[](gt const& gt) { return fmt::format("({} > {})", to_string(*gt.left), to_string(*gt.right)); },
			[](geq const& geq) { return fmt::format("({} >= {})", to_string(*geq.left), to_string(*geq.right)); },
			[](add const& add) { return fmt::format("({} + {})", to_string(*add.addend1), to_string(*add.addend2)); },
			[](sub const& sub) { return fmt::format("({} - {})", to_string(*sub.minuend), to_string(*sub.subtrahend)); },
			[](cluster const& cluster) {
				std::string result = "(";
				if (!cluster.items->empty()) {
					result += (cluster.negations.front() ? "-" : "") + to_string(cluster.items->front());
				}
				for (std::size_t i = 0; i < cluster.connectors.size(); ++i) {
					auto item_string = (cluster.negations[i + 1] ? "-" : "") + to_string((*cluster.items)[i + 1]);
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
			[](lambda const& f) {
				return match(
					f.body,
					[&](expr_ptr const& body) {
						return fmt::format("({} -> {})", to_string(*f.params), to_string(*body));
					},
					[](intrinsic body) { return name(body); });
			},
			[](intrinsic const& f) { return name(f); },
			[](tup_expr const& tup) {
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
			[](list_expr const& list) {
				std::string result = "[";
				if (!list.elems->empty()) {
					result += to_string(list.elems->front());
					for (auto it = list.elems->begin() + 1; it != list.elems->end(); ++it) {
						result += ", " + to_string(*it);
					}
				}
				result += "]";
				return result;
			},
			[](tok::boolean const& b) { return tok::to_string(b); },
			[](tok::num const& num) { return num.rep; },
			[](std::string const& str) { return tok::to_string(str); },
			[](tok::sym const& sym) { return sym.name; });
	}
}
