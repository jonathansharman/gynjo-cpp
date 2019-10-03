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

	cond::cond(ptr test, ptr if_true, ptr if_false)
		: test{std::move(test)}, if_true{std::move(if_true)}, if_false{std::move(if_false)} {}

	cond::cond(cond const& that)
		: test{make_node(*that.test)}, if_true{make_node(*that.if_true)}, if_false{make_node(*that.if_false)} {}

	cond& cond::operator=(cond const& that) {
		test = make_node(*that.test);
		if_true = make_node(*that.if_true);
		if_false = make_node(*that.if_false);
		return *this;
	}

	bool cond::operator==(cond const& that) const {
		return *test == *that.test && *if_true == *that.if_true && *if_false == *that.if_false;
	}

	and_::and_(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	and_::and_(and_ const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	and_& and_::operator=(and_ const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool and_::operator==(and_ const& that) const {
		return *left == *that.left && *right == *that.right;
	}

	or_::or_(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	or_::or_(or_ const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	or_& or_::operator=(or_ const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool or_::operator==(or_ const& that) const {
		return *left == *that.left && *right == *that.right;
	}

	not_::not_(ptr expr) : expr{std::move(expr)} {}

	not_::not_(not_ const& that) : expr{make_node(*that.expr)} {}

	not_& not_::operator=(not_ const& that) {
		expr = make_node(*that.expr);
		return *this;
	}

	bool not_::operator==(not_ const& that) const {
		return *expr == *that.expr;
	}

	eq::eq(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	eq::eq(eq const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	eq& eq::operator=(eq const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool eq::operator==(eq const& that) const {
		return *left == *that.left && *right == *that.right;
	}

	neq::neq(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	neq::neq(neq const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	neq& neq::operator=(neq const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool neq::operator==(neq const& that) const {
		return *left == *that.left && *right == *that.right;
	}

	lt::lt(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	lt::lt(lt const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	lt& lt::operator=(lt const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool lt::operator==(lt const& that) const {
		return *left == *that.left && *right == *that.right;
	}

	leq::leq(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	leq::leq(leq const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	leq& leq::operator=(leq const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool leq::operator==(leq const& that) const {
		return *left == *that.left && *right == *that.right;
	}

	gt::gt(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	gt::gt(gt const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	gt& gt::operator=(gt const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool gt::operator==(gt const& that) const {
		return *left == *that.left && *right == *that.right;
	}

	geq::geq(ptr left, ptr right) : left{std::move(left)}, right{std::move(right)} {}

	geq::geq(geq const& that) : left{make_node(*that.left)}, right{make_node(*that.right)} {}

	geq& geq::operator=(geq const& that) {
		left = make_node(*that.left);
		right = make_node(*that.right);
		return *this;
	}

	bool geq::operator==(geq const& that) const {
		return *left == *that.left && *right == *that.right;
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
			[](tok::boolean const& b) { return ast::to_string(b); },
			[](tok::num const& num) { return num.rep; },
			[](tok::sym const& sym) { return sym.name; });
	}
}
