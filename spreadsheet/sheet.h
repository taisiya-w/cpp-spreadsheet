#pragma once

#include "cell.h"
#include "common.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct PositionHash {
    std::size_t operator()(Position pos) const noexcept {
        return std::hash<int>{}(pos.row) ^ (std::hash<int>{}(pos.col) << 16);
    }
};

class Sheet : public SheetInterface {
public:
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHash> cells_;

    void CheckPosition(Position pos) const;

    Cell* GetCellPtr(Position pos);
    const Cell* GetCellPtr(Position pos) const;

    Cell& GetOrCreateCell(Position pos);

    bool HasCircularDependency(
        Position pos,
        const std::vector<Position>& new_refs) const;

    bool DFS(Position current,
             Position target,
             std::unordered_set<Position, PositionHash>& visited) const;

    void UpdateDependencies(Cell& cell,
                            const std::vector<Position>& old_refs,
                            const std::vector<Position>& new_refs);
    void InvalidateCacheUpwards(Cell& cell);
};