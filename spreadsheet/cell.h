#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell();

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    bool IsReferenced() const;

    void AddDependent(Cell* cell);

    void RemoveDependent(Cell* cell);

    const std::unordered_set<Cell*>& GetDependents() const;

    void InvalidateCache();

    bool IsCacheValid() const;

private:

    class Impl {
    public:
        virtual ~Impl() = default;
        virtual Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        Value GetValue() const override { return std::string{}; }
        std::string GetText() const override { return {}; }
        std::vector<Position> GetReferencedCells() const override { return {}; }
    };

    class TextImpl : public Impl {
    public:
        explicit TextImpl(std::string text) : text_(std::move(text)) {}

        Value GetValue() const override {
            if (!text_.empty() && text_.front() == ESCAPE_SIGN) {
                return text_.substr(1);
            }
            return text_;
        }

        std::string GetText() const override { return text_; }
        std::vector<Position> GetReferencedCells() const override { return {}; }

    private:
        std::string text_;
    };

    class FormulaImpl : public Impl {
    public:
        explicit FormulaImpl(std::string expression, Sheet& sheet);

        Value GetValue() const override;
        std::string GetText() const override;
        std::vector<Position> GetReferencedCells() const override;

    private:
        std::unique_ptr<FormulaInterface> formula_;
        Sheet& sheet_;
    };

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    mutable std::optional<Value> cache_;
    std::unordered_set<Cell*> dependents_;
};