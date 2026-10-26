// GroupBox.hpp — a reusable titled panel that groups related controls for DPF NanoVG UIs.
//
// It draws a rounded panel with a caption (e.g. "ADSR Envelope") and acts as a container:
// child controls are parented to it, and it lays them out in a row inside its content area
// (below the title strip, within padding). As the synth grows, each module becomes one
// GroupBox — make a box, parent its knobs to it, addControl() each, position + size the box,
// call relayout().
//
// Positions are window-absolute in DGL (a subwidget's coords are not offset by its parent),
// so relayout() places children relative to the box's own absolute position. Mouse events
// still reach the nested controls: the default Widget::onMouse forwards to subwidgets, and
// this box does not override it.

#ifndef STRUCKGAUSSIAN_GROUPBOX_HPP
#define STRUCKGAUSSIAN_GROUPBOX_HPP

#include "NanoVG.hpp"

#include <algorithm>
#include <initializer_list>
#include <string>
#include <vector>

START_NAMESPACE_DGL

class GroupBox : public NanoSubWidget
{
public:
    explicit GroupBox(Widget* parent)
        : NanoSubWidget(parent)
    {
        fHasFont = loadSharedResources(); // DPF's bundled DejaVu Sans
    }

    void setTitle(const std::string& title) { fTitle = title; }
    void setAccentColor(Color color)        { fAccent = color; }
    // Scales font size AND the title-strip/padding geometry together — scaling font alone
    // (without the strip it sits in) is what causes titles to overflow into the content area.
    void setUiScale(float s)
    {
        fUiScale     = s;
        fPadding     = static_cast<int>(10 * s);
        fTitleHeight = static_cast<int>(28 * s);
    }

    // Register a control (already parented to this box) for row layout. Each takes its own cell.
    void addControl(SubWidget* control) { fCells.push_back({ control }); }
    int  controlCount() const
    {
        int n = 0;
        for (const auto& cell : fCells) n += static_cast<int>(cell.size());
        return n;
    }
    // Move `below` into `above`'s cell, under it; the cell's controls share its height. For pairs of
    // short controls such as dropdowns, so they take one column instead of two.
    void stackUnder(SubWidget* above, SubWidget* below)
    {
        for (auto it = fCells.begin(); it != fCells.end(); ++it)
            if (it->size() == 1 && it->front() == below) { fCells.erase(it); break; }
        for (auto& cell : fCells)
            if (std::find(cell.begin(), cell.end(), above) != cell.end()) { cell.push_back(below); return; }
    }
    // Replace the cells with one control each, in this order (row by row once setRows() wraps
    // them); nullptr leaves a cell empty. For a panel whose controls don't read well in param order.
    void arrange(std::initializer_list<SubWidget*> order)
    {
        fCells.clear();
        for (SubWidget* c : order)
            fCells.push_back(c != nullptr ? std::vector<SubWidget*>{ c } : std::vector<SubWidget*>{});
    }
    // Lay the cells out in this many rows (filled left to right, top to bottom); 1 = one row.
    void setRows(int rows) { fRows = rows < 1 ? 1 : rows; }
    int  columnCount() const { return (static_cast<int>(fCells.size()) + fRows - 1) / fRows; }

    // The inner content rectangle (window-absolute), below the title strip and inside padding.
    // For panels that lay their children out themselves rather than via relayout().
    void contentBounds(int& x, int& y, int& w, int& h) const
    {
        x = getAbsoluteX() + fPadding;
        y = getAbsoluteY() + fTitleHeight;
        w = static_cast<int>(getWidth())  - 2 * fPadding;
        h = static_cast<int>(getHeight()) - fTitleHeight - fPadding;
    }

    // Lay the cells out evenly across the content area, in fRows rows.
    void relayout()
    {
        const int n = static_cast<int>(fCells.size());
        if (n == 0)
            return;

        const int contentX = getAbsoluteX() + fPadding;
        const int contentY = getAbsoluteY() + fTitleHeight;
        const int contentW = static_cast<int>(getWidth())  - 2 * fPadding;
        const int contentH = static_cast<int>(getHeight()) - fTitleHeight - fPadding;
        if (contentW <= 0 || contentH <= 0)
            return;

        const int cols  = columnCount();
        const int cellW = contentW / cols;
        const int cellH = contentH / fRows;
        for (int i = 0; i < n; ++i)
        {
            const int m  = static_cast<int>(fCells[i].size());
            if (m == 0) continue;   // an empty cell from arrange()
            const int x  = contentX + (i % cols) * cellW;
            const int y  = contentY + (i / cols) * cellH;
            const int ch = cellH / m;
            for (int j = 0; j < m; ++j)
            {
                fCells[i][j]->setSize(static_cast<uint>(cellW), static_cast<uint>(ch));
                fCells[i][j]->setAbsolutePos(x, y + j * ch);
            }
        }
    }

protected:
    void onNanoDisplay() override
    {
        const float w = getWidth();
        const float h = getHeight();

        // panel fill (slightly raised from the window background)
        beginPath();
        roundedRect(1.5f, 1.5f, w - 3.0f, h - 3.0f, 8.0f);
        fillColor(Color(42, 34, 26));
        fill();

        // border
        beginPath();
        roundedRect(1.5f, 1.5f, w - 3.0f, h - 3.0f, 8.0f);
        strokeColor(Color(74, 62, 48));
        strokeWidth(1.5f);
        stroke();

        // title
        if (fHasFont)
        {
            fontFace(NANOVG_DEJAVU_SANS_TTF);
            textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
            fontSize(13.0f * fUiScale);
            fillColor(fAccent);
            text(12.0f * fUiScale, fTitleHeight * 0.5f + 3.0f, fTitle.c_str(), nullptr);
        }
    }

private:
    std::vector<std::vector<SubWidget*>> fCells;   // each cell's controls, top to bottom
    std::string             fTitle;
    Color                   fAccent { 150, 200, 210 };
    bool                    fHasFont = false;
    float                   fUiScale = 1.0f;
    int                     fRows = 1;

    int fPadding     = 10;
    int fTitleHeight = 28;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GroupBox)
};

END_NAMESPACE_DGL

#endif // STRUCKGAUSSIAN_GROUPBOX_HPP
