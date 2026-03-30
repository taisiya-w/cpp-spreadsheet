#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

namespace {

class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression)
        try : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception& e) {
        throw FormulaException(e.what());
    }

    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute([&sheet](Position pos) -> double {
                if (!pos.IsValid()) {
                    throw FormulaError(FormulaError::Category::Ref);
                }

                const CellInterface* cell = sheet.GetCell(pos);
                if (!cell) return 0.0;

                const auto val = cell->GetValue();

                if (std::holds_alternative<double>(val)) {
                    return std::get<double>(val);
                }

                if (std::holds_alternative<FormulaError>(val)) {
                    throw std::get<FormulaError>(val);
                }

                const auto& str = std::get<std::string>(val);
                if (str.empty()) return 0.0;

                try {
                    std::size_t end_pos = 0;
                    double result = std::stod(str, &end_pos);
                    if (end_pos != str.size()) {
                        throw FormulaError(FormulaError::Category::Value);
                    }
                    return result;
                } catch (const FormulaError&) {
                    throw;
                } catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            });
        } catch (const FormulaError& fe) {
            return fe;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> result;
        for (const auto& pos : ast_.GetCells()) {
            if (result.empty() || !(result.back() == pos)) {
                result.push_back(pos);
            }
        }
        return result;
    }

private:
    FormulaAST ast_;
};

}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}