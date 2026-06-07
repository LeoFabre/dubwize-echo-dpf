#include "DubwizeUI.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Color;
using dubwize::Param;
using dubwize::ParamInfo;
using dubwize::paramInfo;
using dubwize::kNumControlParams;

DubwizeUI::DubwizeUI() : UI(kWidth, kHeight)
{
    setSize(kWidth, kHeight);
    setGeometryConstraints(kWidth, kHeight, false);

    // Seed UI values from plugin defaults; the host pushes real values via
    // parameterChanged().
    for (int i = 0; i < kNumControlParams; ++i)
        values_[i] = paramInfo(static_cast<Param>(i)).def;
}

void DubwizeUI::parameterChanged(uint32_t index, float value)
{
    if (index < static_cast<uint32_t>(kNumControlParams)) {
        values_[index] = value;
        repaint();
    }
    // index >= 45 are read-only output params: ignored.
}

void DubwizeUI::cellOrigin(int i, float& cx, float& cy) const
{
    const int col = i % kCols;
    const int row = i / kCols;
    cx = kMarginX + col * kCellW;
    cy = kMarginTop + row * kCellH;
}

int DubwizeUI::hitTest(float mx, float my) const
{
    for (int i = 0; i < kNumControlParams; ++i) {
        float cx, cy;
        cellOrigin(i, cx, cy);
        if (mx >= cx && mx < cx + kCellW && my >= cy && my < cy + kCellH)
            return i;
    }
    return -1;
}

void DubwizeUI::onNanoDisplay()
{
    // Background.
    beginPath();
    rect(0.f, 0.f, kWidth, kHeight);
    fillColor(Color(26, 26, 31));
    fill();

    // Title.
    fontSize(18.f);
    textAlign(ALIGN_LEFT | ALIGN_BASELINE);
    fillColor(Color(217, 217, 217));
    text(12.f, 28.f, "Dubwize — debug UI", nullptr);

    char buf[64];

    for (int i = 0; i < kNumControlParams; ++i) {
        const ParamInfo& pi = paramInfo(static_cast<Param>(i));
        const float v = values_[i];

        float cx, cy;
        cellOrigin(i, cx, cy);

        // Cell centre and control box geometry.
        const float boxX = cx + 14.f;
        const float boxY = cy + 8.f;
        const float boxW = kCellW - 28.f;
        const float boxH = 44.f;
        const float midX = cx + kCellW * 0.5f;

        if (pi.isBool) {
            // Toggle rectangle, filled when on.
            const bool on = v > 0.5f;
            beginPath();
            rect(boxX, boxY, boxW, boxH);
            fillColor(on ? Color(102, 204, 102) : Color(64, 64, 71));
            fill();
            strokeColor(Color(153, 153, 153));
            stroke();

            fontSize(11.f);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(on ? Color(20, 30, 20) : Color(200, 200, 200));
            text(midX, boxY + boxH * 0.5f, on ? "ON" : "OFF", nullptr);
        }
        else if (pi.numChoices > 0) {
            // Choice box showing the current option string.
            int idx = static_cast<int>(v + 0.5f);
            idx = std::clamp(idx, 0, pi.numChoices - 1);

            beginPath();
            rect(boxX, boxY, boxW, boxH);
            fillColor(Color(46, 46, 56));
            fill();
            strokeColor(Color(153, 153, 153));
            stroke();

            fontSize(10.f);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(Color(217, 217, 217));
            const char* choice = (pi.choices != nullptr) ? pi.choices[idx] : "?";
            text(midX, boxY + boxH * 0.5f, choice, nullptr);
        }
        else {
            // Float: arc/knob.
            const float range = std::max(1e-9f, pi.max - pi.min);
            const float n = std::clamp((v - pi.min) / range, 0.f, 1.f);

            const float r  = 18.f;
            const float kx = midX;
            const float ky = boxY + r + 2.f;

            // Knob body.
            beginPath();
            circle(kx, ky, r);
            fillColor(Color(46, 46, 56));
            fill();
            strokeColor(Color(153, 153, 153));
            stroke();

            // Value arc (270 degree sweep starting bottom-left).
            const float a0 = -2.356f;            // -135 deg
            const float a1 = a0 + n * 4.712f;    // up to +135 deg
            beginPath();
            arc(kx, ky, r - 3.f, a0, a1, CW);
            strokeColor(Color(102, 178, 102));
            strokeWidth(3.f);
            stroke();
            strokeWidth(1.f);

            // Pointer line.
            beginPath();
            moveTo(kx, ky);
            lineTo(kx + std::cos(a1) * (r - 4.f),
                   ky + std::sin(a1) * (r - 4.f));
            strokeColor(Color(242, 242, 242));
            strokeWidth(2.f);
            stroke();
            strokeWidth(1.f);

            // Numeric value.
            std::snprintf(buf, sizeof(buf), "%.3g", static_cast<double>(v));
            fontSize(10.f);
            textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
            fillColor(Color(180, 200, 220));
            text(midX, boxY + boxH + 4.f, buf, nullptr);
        }

        // Parameter name label (always rendered, at the bottom of the cell).
        fontSize(10.f);
        textAlign(ALIGN_CENTER | ALIGN_BASELINE);
        fillColor(Color(200, 200, 200));
        text(midX, cy + kCellH - 6.f, pi.name, nullptr);
    }
}

bool DubwizeUI::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press) {
        const int i = hitTest(ev.pos.getX(), ev.pos.getY());
        if (i < 0)
            return false;

        const ParamInfo& pi = paramInfo(static_cast<Param>(i));

        if (pi.isBool) {
            // Toggle on click.
            const float nv = (values_[i] > 0.5f) ? 0.f : 1.f;
            values_[i] = nv;
            setParameterValue(i, nv);
            repaint();
            return true;
        }
        if (pi.numChoices > 0) {
            // Advance to next choice (wrap around) on click.
            int idx = static_cast<int>(values_[i] + 0.5f);
            idx = (idx + 1) % pi.numChoices;
            const float nv = static_cast<float>(idx);
            values_[i] = nv;
            setParameterValue(i, nv);
            repaint();
            return true;
        }

        // Float: begin vertical drag.
        const float range = std::max(1e-9f, pi.max - pi.min);
        draggingIdx_ = i;
        dragStartY_  = ev.pos.getY();
        dragStartN_  = std::clamp((values_[i] - pi.min) / range, 0.f, 1.f);
        return true;
    }

    draggingIdx_ = -1;
    return true;
}

bool DubwizeUI::onMotion(const MotionEvent& ev)
{
    if (draggingIdx_ < 0)
        return false;

    const int i = draggingIdx_;
    const ParamInfo& pi = paramInfo(static_cast<Param>(i));
    const float range = pi.max - pi.min;

    // Drag up increases value. 200px of travel = full range.
    const float dy = dragStartY_ - ev.pos.getY();
    float n = std::clamp(dragStartN_ + dy / 200.f, 0.f, 1.f);
    float v = pi.min + n * range;
    v = std::clamp(v, std::min(pi.min, pi.max), std::max(pi.min, pi.max));

    values_[i] = v;
    setParameterValue(i, v);
    repaint();
    return true;
}

UI* createUI() { return new DubwizeUI(); }

END_NAMESPACE_DISTRHO
