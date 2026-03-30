#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <string>

Cell::FormulaImpl::FormulaImpl(std::string expression, Sheet& sheet)
    : formula_(ParseFormula(std::move(expression)))
    , sheet_(sheet) {}

Cell::Value Cell::FormulaImpl::GetValue() const {
    auto result = formula_->Evaluate(sheet_);
    if (std::holds_alternative<double>(result)) {
        return std::get<double>(result);
    }
    return std::get<FormulaError>(result);
}

std::string Cell::FormulaImpl::GetText() const {
    return std::string(1, FORMULA_SIGN) + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}


Cell::Cell(Sheet& sheet)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet) {}

Cell::~Cell() = default;

void Cell::Set(std::string text) {
    if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        impl_ = std::make_unique<FormulaImpl>(text.substr(1), sheet_);
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
    cache_.reset();
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
    cache_.reset();
}

Cell::Value Cell::GetValue() const {
    if (!cache_) {
        cache_ = impl_->GetValue();
    }
    return *cache_;
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

bool Cell::IsReferenced() const {
    return !dependents_.empty();
}

void Cell::AddDependent(Cell* cell) {
    dependents_.insert(cell);
}

void Cell::RemoveDependent(Cell* cell) {
    dependents_.erase(cell);
}

const std::unordered_set<Cell*>& Cell::GetDependents() const {
    return dependents_;
}

void Cell::InvalidateCache() {
    cache_.reset();
}

bool Cell::IsCacheValid() const {
    return cache_.has_value();
}