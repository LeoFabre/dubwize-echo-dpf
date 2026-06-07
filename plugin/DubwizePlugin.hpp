#pragma once
#include "DistrhoPlugin.hpp"
#include "ParameterMetadata.hpp"
#include "DubwizeEngine.hpp"
#include "TapTempo.hpp"
#include <vector>

START_NAMESPACE_DISTRHO
class DubwizePlugin : public Plugin {
public:
    DubwizePlugin();
protected:
    const char* getLabel() const override { return "Dubwize"; }
    const char* getDescription() const override { return "Dubplex Dubwize Echo (DPF port)"; }
    const char* getMaker() const override { return "Nexus"; }
    const char* getLicense() const override { return "Apache-2.0"; }
    uint32_t getVersion() const override { return d_version(0,1,0); }
    int64_t getUniqueId() const override { return d_cconst('D','b','w','z'); }

    void initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;
    void  activate() override;
    void  run(const float** inputs, float** outputs, uint32_t frames) override;
private:
    dubwize::DubwizeEngine engine_, ppA_, ppB_;
    dubwize::TapTempo tapTempo_;
    float params_[dubwize::kNumControlParams] = {};
    float outParams_[dubwize::kNumOutputParams] = {};
    bool  requiresUpdate_ = true;
    bool  tapEnabledInternal_ = false;
    double frameClockMs_ = 0.0;
    float lastHostBpm_ = 0.0f;
    std::vector<float> dryL_, dryR_, ppAL_, ppAR_, ppBL_, ppBR_;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DubwizePlugin)
};
END_NAMESPACE_DISTRHO
