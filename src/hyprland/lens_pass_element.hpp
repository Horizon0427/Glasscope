#pragma once

#include "glasscope/model/lens_config.hpp"
#include "render/lens_renderer.hpp"

#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/render/pass/PassElement.hpp>

#include <functional>

namespace Glasscope {

struct LensPassData {
    PHLMONITORREF monitor;
    Vector2D centerLocal;
    Vector2D velocity;
    Vector2D pullAxis = {1.0, 0.0};
    std::array<Vector2D, 3> trailNodes;
    LensStyle style;
    float reveal = 1.0F;
    float wobble = 0.0F;
    float interactionWobble = 0.0F;
    bool pinned = false;
    float pullShare = 0.28F;
    float timeSeconds = 0.0F;
    float colorProbeAmount = 0.0F;
    float colorProbeCaptured = 0.0F;
    std::array<float, 3> colorProbeColor = {};
    bool captureColor = false;
    std::function<void(ColorSample)> onColorCaptured;
};

class LensPassElement final : public IPassElement {
  public:
    LensPassElement(LensRenderer& renderer, LensPassData data);

    std::vector<UP<IPassElement>> draw() override;
    bool needsLiveBlur() override;
    bool needsPrecomputeBlur() override;
    std::optional<CBox> boundingBox() override;
    CRegion opaqueRegion() override;
    const char* passName() override;
    ePassElementType type() override;

  private:
    LensRenderer& m_renderer;
    LensPassData m_data;
};

}
