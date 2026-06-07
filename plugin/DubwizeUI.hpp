#pragma once
#include "DistrhoUI.hpp"
#include "ParameterMetadata.hpp"

START_NAMESPACE_DISTRHO

class DubwizeUI : public UI
{
public:
    DubwizeUI();

protected:
    void onNanoDisplay() override;
    bool onMouse(const MouseEvent& ev) override;
    bool onMotion(const MotionEvent& ev) override;
    void parameterChanged(uint32_t index, float value) override;

private:
    // Grid layout for the 45 control params.
    static constexpr int   kCols       = 8;
    static constexpr float kCellW      = 124.f;
    static constexpr float kCellH      = 92.f;
    static constexpr float kMarginX    = 12.f;
    static constexpr float kMarginTop  = 44.f;
    static constexpr int   kRows       = (dubwize::kNumControlParams + kCols - 1) / kCols;
    static constexpr float kWidth      = kMarginX * 2.f + kCols * kCellW;          // 1024
    static constexpr float kHeight     = kMarginTop + kRows * kCellH + kMarginX;   // 44 + 6*92 + 12 = 608

    // Geometry of one control's cell (top-left corner).
    void cellOrigin(int i, float& cx, float& cy) const;
    // Hit test: returns the control index under (mx,my), or -1.
    int  hitTest(float mx, float my) const;

    float values_[dubwize::kNumControlParams];
    int   draggingIdx_ = -1;
    float dragStartY_  = 0.f;
    float dragStartN_  = 0.f;   // normalized value at drag start

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DubwizeUI)
};

END_NAMESPACE_DISTRHO
