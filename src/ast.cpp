//! @file
//! @copyright See <a href="LICENSE.txt">LICENSE.txt</a>.

#include "ast.hpp"

namespace gynjo::ast {
	bool assign::operator==(assign const& that) const noexcept {
		return symbol == that.symbol && *rhs == *that.rhs;
	}

	bool cond::operator==(cond const& that) const noexcept {
		return *test == *that.test && *if_true == *that.if_true && *if_false == *that.if_false;
	}

	bool for_loop::operator==(for_loop const& that) const noexcept {
		return loop_var == that.loop_var && *range == *that.range && *body == *that.body;
	}

	bool and_::operator==(and_ const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool or_::operator==(or_ const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool not_::operator==(not_ const& that) const noexcept {
		return *expr == *that.expr;
	}

	bool eq::operator==(eq const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool neq::operator==(neq const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool lt::operator==(lt const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool leq::operator==(leq const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool gt::operator==(gt const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool geq::operator==(geq const& that) const noexcept {
		return *left == *that.left && *right == *that.right;
	}

	bool add::operator==(add const& that) const noexcept {
		return *addend1 == *that.addend1 && *addend2 == *that.addend2;
	}

	bool sub::operator==(sub const& that) const noexcept {
		return *minuend == *that.minuend && *subtrahend == *that.subtrahend;
	}

	cluster::cluster(std::vector<bool> negations, std::unique_ptr<std::vector<node>> items, std::vector<connector> connectors)
		: negations{std::move(negations)}, items{std::move(items)}, connectors{std::move(connectors)} {}

	cluster::cluster(cluster const& that)
		: negations{that.negations}, items{std::make_unique<std::vector<node>>(*that.items)}, connectors{that.connectors} {}

	cluster& cluster::operator=(cluster const& that) {
		negations = that.negations;
		items = std::make_unique<std::vector<node>>(*that.items);
		connectors = that.connectors;
		return *this;
	}

	bool cluster::operator==(cluster const& that) const noexcept {
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

	bool tup::operator==(tup const& that) const noexcept {
		return *elems == *that.elems;
	}

	list::list() : elems{std::make_unique<std::deque<node>>()} {}
	list::list(std::unique_ptr<std::deque<node>> elems) : elems{std::move(elems)} {}

	list::list(list const& that) {
		elems = std::make_unique<std::deque<node>>(*that.elems);
	}

	list& list::operator=(list const& that) {
		elems = std::make_unique<std::deque<node>>(*that.elems);
		return *this;
	}

	bool list::operator==(list const& that) const noexcept {
		return *elems == *that.elems;
	}

	bool lambda::operator==(lambda const& that) const noexcept {
		return *params == *that.params && body == that.body;
	}

	auto to_string(node const& node) -> std::string {
		using namespace std::string_literals;
		return match(
			node,
			[](nop) { return "no-op"s; },
			[](imp const& imp) { return "import " + imp.filename; },
			[](assign const& assign) { return ast::to_string(assign.symbol) + " = " + to_string(*assign.rhs); },
			[](cond const& cond) {
				return fmt::format(
					"(if {} then {} else {})", to_string(*cond.test), to_string(*cond.if_true), to_string(*cond.if_false));
			},
			[](for_loop const& for_loop) {
				return fmt::format("(for {} in {} do {})",
					tok::to_string(for_loop.loop_var),
					to_string(*for_loop.range),
					to_string(*for_loop.body));
			},
			[](and_ const& and_) { return fmt::format("({} and {})", to_string(*and_.left), to_string(*and_.right)); },
			[](or_ const& or_) { return fmt::format("({} or {})", to_string(*or_.left), to_string(*or_.right)); },
			[](not_ const& not_) { return fmt::format("(not {})", to_string(*not_.expr)); },
			[](eq const& eq) { return fmt::format("({} == {})", to_string(*eq.left), to_string(*eq.right)); },
			[](neq const& neq) { return fmt::format("({} != {})", to_string(*neq.left), to_string(*neq.right)); },
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
					[&](ptr const& body) { return fmt::format("({} -> {})", to_string(*f.params), to_string(*body)); },
					[](intrinsic body) { return name(body); });
			},
			[](intrinsic const& f) { return name(f); },
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
			[](list const& list) {
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
			[](tok::sym const& sym) { return sym.name; });
	}
}
