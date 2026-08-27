#pragma once

#include "glasscope/lens_config.hpp"
#include "glasscope/lens_renderer.hpp"

#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/render/pass/PassElement.hpp>

namespace Glasscope {

struct LensPassData {
    PHLMONITORREF monitor;
    Vector2D centerLocal;
    Vector2D velocity;
    std::array<Vector2D, 3> trailNodes;
    LensStyle style;
    float reveal = 1.0F;
    float wobble = 0.0F;
    float timeSeconds = 0.0F;
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

} // namespace Glasscope
