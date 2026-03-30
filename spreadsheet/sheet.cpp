#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <variant>

void Sheet::CheckPosition(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid position: " + pos.ToString());
    }
}

Cell* Sheet::GetCellPtr(Position pos) {
    auto it = cells_.find(pos);
    return (it != cells_.end()) ? it->second.get() : nullptr;
}

const Cell* Sheet::GetCellPtr(Position pos) const {
    auto it = cells_.find(pos);
    return (it != cells_.end()) ? it->second.get() : nullptr;
}

Cell& Sheet::GetOrCreateCell(Position pos) {
    auto it = cells_.find(pos);
    if (it == cells_.end()) {
        auto [ins, ok] = cells_.emplace(pos, std::make_unique<Cell>(*this));
        return *ins->second;
    }
    return *it->second;
}

bool Sheet::DFS(Position current, Position target,
                std::unordered_set<Position, PositionHash>& visited) const {
    if (!visited.insert(current).second) {
        return false;
    }
    if (current == target) return true;

    const Cell* cell = GetCellPtr(current);
    if (!cell) return false;

    for (const auto& ref : cell->GetReferencedCells()) {
        if (DFS(ref, target, visited)) return true;
    }
    return false;
}

bool Sheet::HasCircularDependency(Position pos,
                                  const std::vector<Position>& new_refs) const {
    std::unordered_set<Position, PositionHash> visited;
    for (const auto& ref : new_refs) {
        if (DFS(ref, pos, visited)) return true;
    }
    return false;
}

void Sheet::UpdateDependencies(Cell& cell,
                               const std::vector<Position>& old_refs,
                               const std::vector<Position>& new_refs) {
    for (const auto& pos : old_refs) {
        Cell* ref_cell = GetCellPtr(pos);
        if (ref_cell) ref_cell->RemoveDependent(&cell);
    }
    for (const auto& pos : new_refs) {
        Cell& ref_cell = GetOrCreateCell(pos);
        ref_cell.AddDependent(&cell);
    }
}

void Sheet::InvalidateCacheUpwards(Cell& cell) {
    std::queue<Cell*> queue;
    std::unordered_set<Cell*> visited;
    queue.push(&cell);

    while (!queue.empty()) {
        Cell* current = queue.front();
        queue.pop();

        if (!visited.insert(current).second) continue;

        current->InvalidateCache();

        for (Cell* dep : current->GetDependents()) {
            queue.push(dep);
        }
    }
}

void Sheet::SetCell(Position pos, std::string text) {
    CheckPosition(pos);

    std::vector<Position> new_refs;
    if (text.size() > 1 && text.front() == FORMULA_SIGN) {
        auto formula = ParseFormula(text.substr(1));
        new_refs = formula->GetReferencedCells();
    }

    if (HasCircularDependency(pos, new_refs)) {
        throw CircularDependencyException("Circular dependency detected");
    }

    Cell& cell = GetOrCreateCell(pos);

    std::vector<Position> old_refs = cell.GetReferencedCells();

    UpdateDependencies(cell, old_refs, new_refs);

    InvalidateCacheUpwards(cell);

    cell.Set(std::move(text));
}

const CellInterface* Sheet::GetCell(Position pos) const {
    CheckPosition(pos);
    return GetCellPtr(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    CheckPosition(pos);
    return GetCellPtr(pos);
}

void Sheet::ClearCell(Position pos) {
    CheckPosition(pos);

    Cell* cell = GetCellPtr(pos);
    if (!cell) return;

    std::vector<Position> old_refs = cell->GetReferencedCells();
    UpdateDependencies(*cell, old_refs, {});

    InvalidateCacheUpwards(*cell);

    if (cell->IsReferenced()) {
        cell->Clear();
    } else {
        cells_.erase(pos);
    }
}

Size Sheet::GetPrintableSize() const {
    int max_row = 0, max_col = 0;
    for (const auto& [pos, cell] : cells_) {
        if (cell && !cell->GetText().empty()) {
            max_row = std::max(max_row, pos.row + 1);
            max_col = std::max(max_col, pos.col + 1);
        }
    }
    return {max_row, max_col};
}

void Sheet::PrintValues(std::ostream& output) const {
    const auto [rows, cols] = GetPrintableSize();
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c > 0) output << '\t';
            auto it = cells_.find({r, c});
            if (it != cells_.end() && it->second) {
                std::visit([&output](const auto& v) { output << v; },
                           it->second->GetValue());
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    const auto [rows, cols] = GetPrintableSize();
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (c > 0) output << '\t';
            auto it = cells_.find({r, c});
            if (it != cells_.end() && it->second) {
                output << it->second->GetText();
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}