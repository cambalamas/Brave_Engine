#include "Motion.hpp"

#include <tuple>
#include <numeric>
#include <fstream>
#include <iostream>

#include "Logger.hpp"
#include "Loader.hpp"
#include "Strings.hpp"
#include "Settings.hpp"
#include "Files.hpp"


namespace brave {

// * Helpers

// Determine the folder for plots and create if not exists
std::string Motion::plotFolder() {
  std::string _folder = Settings::plotDir;

  if (!Files::pathExists(_folder)) {
#if _WIN64
    auto winPath = Files::pathToWin(_folder);
    auto winCmd  = std::string("mkdir" + winPath + ">nul 2>&1");
    system(winCmd.c_str());
#else
    auto nixCmd = std::string("mkdir -p" + _folder);
    system(nixCmd.c_str());
#endif
  }

  if (!Files::pathExists(_folder)) {
    _folder = "";
    LOGE("Couldn't use gived path to store plot data.");
  }

  return _folder;
}



// * Frame methods

// ====================================================================== //
// ====================================================================== //
// Sum rotations of frame
// ====================================================================== //

glm::vec3 Frame::value() const {
  glm::vec3 RT = this->translation * glm::vec3{0.f, 2.f, 0.f};
  glm::vec3 RR = this->rotations.at(0) * glm::vec3{2.f, 0.f, 2.f};

  glm::vec3 JR{0.f};
  for (const auto& rot : this->rotations) JR += rot;

  return RT + RR + (JR - this->rotations.at(0));
}

// ====================================================================== //
// ====================================================================== //
// Lerp frame A to frame B at alpha point
// ====================================================================== //

Frame Frame::lerpOne(const Frame& f2, float alpha) const {
  Frame f;
  f.translation = glm::mix(this->translation, f2.translation, alpha);

  try {
    auto rootFront = Math::rotToVec(this->rotations[0]);
    auto rootY = glm::orientedAngle(Math::unitVecZ, rootFront, Math::unitVecY);

    for (auto i = 0u; i < this->rotations.size(); ++i) {
      auto newRot = glm::mix(this->rotations[i], f2.rotations[i], alpha);
      if (i == 0u) { newRot = rootY * Math::unitVecY; } // Avoid flip over self

      f.rotations.push_back(newRot);
    }
  } catch (std::exception&) {
    LOGE("F1 and F2 doesn't have the same number of Joints?")
  }
  return f;
}

// ====================================================================== //
// ====================================================================== //
// Lerp from frame A to frame B
// ====================================================================== //

std::vector<Frame> Frame::lerpTransition(const Frame& f2, uint steps) const {
  std::vector<Frame> lerpFrames;
  if (steps < 1u) return lerpFrames;
  float alphaStep = 1.0f / glm::clamp((float)steps, 2.f, 100.f);

  for (auto alpha = 0.1f; alpha <= 1.0f; alpha += alphaStep) {
    auto frame = this->lerpOne(f2, alpha);
    lerpFrames.push_back(frame);
  }
  return lerpFrames;
}



// * Motion methods

// ====================================================================== //
// ====================================================================== //
// 'Constructor'
// ====================================================================== //

std::shared_ptr<Motion> Motion::create(const std::string& name,
                                       const std::string& filepath,
                                       loopMode           lm,
                                       uint               steps) {
  auto m       = loader::BVH(filepath);
  m->name      = name;
  m->m_maxStep = 0u;

  //
  float minTX, minTY, minTZ;
  minTX = minTY = minTZ = std::numeric_limits<float>::max();
  //


  // === Iter 1 : Get data and Set undependent ===

  for (auto i = 1u; i < m->frames.size(); ++i) {
    auto ft = m->frames.at(i).translation;
    auto fr = &m->frames.at(i).rotations.at(0);

    // Get Max distance
    if (i < m->frames.size() - 2u) {
      auto step = glm::distance(m->frames.at(i + 1u).translation, ft);
      if (step > m->m_maxStep) m->m_maxStep = step;
    }

    // Get mins positions
    if (ft.x < minTX) minTX = ft.x;
    if (ft.y < minTY) minTY = ft.y;
    if (ft.z < minTZ) minTZ = ft.z;
  }


  // === Iter 2 : Set dependent data ===

  for (auto i = 1u; i < m->frames.size(); ++i) {
    // T back to 0
    m->frames.at(i).translation.x -= minTX;
    m->frames.at(i).translation.y -= minTY;
    m->frames.at(i).translation.z -= minTZ;
  }

  // === CLEAN ===
  m->clean(lm, steps);

  return m;
}

// ====================================================================== //
// ====================================================================== //
// Minor methods
// ====================================================================== //

float Motion::maxStep() const { return m_maxStep; }

bool Motion::isMix() { return Strings::contains(this->name, "_"); }

Frame Motion::linkedFrame(uint frameIdx, float alpha) const {
  Frame f;
  if (!this->linked) return f;
  auto factor = (float)linked->frames.size() / (float)frames.size();
  // Get errors with 'ceil' (get out-of-range values). Using 'floor' instead.
  auto cf = floorf(frameIdx * factor);
  return frames.at(cf).lerpOne(linked->frames.at(cf), alpha);
}

// ====================================================================== //
// ====================================================================== //
// Clean any motion to get a smoother loop
// ====================================================================== //

void Motion::clean(loopMode lm, uint steps) {
  this->frames.erase(this->frames.begin()); // Remove T-Pose
  if (lm == loopMode::none) return;

  // Get low errror frames of animation to generate a loop
  if (lm == loopMode::shortLoop) {
    uint B, E;
    {
      uint  nFrames = frames.size();
      uint  limitA  = nFrames / 2.f;
      uint  limitB  = nFrames - limitA;
      float auxDiff = std::numeric_limits<float>::max();

      for (auto f1 = 0u; f1 < limitA; ++f1) {
        glm::vec3 f1Val = this->frames.at(f1).value();

        for (auto f2 = limitB; f2 < nFrames; ++f2) {
          glm::vec3 f2Val = this->frames.at(f2).value();

          auto diff = glm::compAdd(glm::abs(f1Val - f2Val));
          if (diff < auxDiff) { std::tie(auxDiff, B, E) = {diff, f1, f2}; }
        }
      }
    }

    // Clean based on obtained frames
    std::vector<Frame> auxFrames;
    auxFrames.reserve(E);
    for (auto f = B; f < E; ++f) { auxFrames.push_back(this->frames.at(f)); }
    this->frames = auxFrames; // Store
  }

  // Befor lerp modify the N first frames of motion setting rots to zero, avoiding abrupt changes
  // unsigned int nFrame = 15u;
  // float        alphastep = 1.f / (float)nFrame;
  // auto         fnR       = this->frames.at(nFrame).rotations.at(0);

  // for (auto i = 0u; i < 15u; ++i) {
  //   // auto alpha                         = i * alphastep;
  //   // auto newRot                        = glm::lerp(glm::vec3{0.f}, fnR, alpha);
  //   this->frames.at(i).rotations.at(0) = glm::vec3{0.f};
  // }

  // Lerp
  // this->frames.at(0).rotations.at(0) =
  //     glm::vec3(0.f); // Avoid bad accum xz rots

  auto first = this->frames.front();
  auto last  = this->frames.back();
  for (const auto& nF : last.lerpTransition(first, steps)) {
    this->frames.push_back(nF);
  }

  // Force look forward
  for (auto i = 0u; i < this->frames.size(); ++i) {
    Transform t1;
    t1.rot = this->frames.at(i).rotations.at(0);

    auto currFront = Math::rotToVec(t1.rot);
    auto newY = glm::orientedAngle(t1.front(), Math::unitVecZ, Math::unitVecY);
    //TODO avoid itself rotation because y flip sign in the middle
    Transform t2;
    t2.rot = glm::degrees(newY) * Math::unitVecY;

    float x, y, z;
    glm::extractEulerAngleXYZ(t2.asMatrix() * t1.asMatrix(), x, y, z);

    this->frames.at(i).rotations.at(0) = glm::degrees(glm::vec3{-x, y, z});
    LOG("[{}] - {}", i, glm::to_string(this->frames.at(i).rotations.at(0)));
  }
}

// ====================================================================== //
// ====================================================================== //
// Mix any motion with other and get a new animation
// that conect both smoothly
// ====================================================================== //

Motion::mixMap Motion::mix(const std::shared_ptr<Motion>& m2) {
  auto createTransitionMotion = [&](uint idxF1, uint idxF2) {
    auto mo      = std::make_shared<Motion>();
    mo->joints   = this->joints;
    mo->timeStep = (this->timeStep + m2->timeStep) * 0.5f;

    auto f1 = this->frames.at(idxF1);
    auto f2 = m2->frames.at(idxF2);
    for (const auto& nF : f1.lerpTransition(f2, 10u)) {
      mo->frames.push_back(nF);
    }

    return mo;
  };

  //---

  // for heat map visualization
  auto          _prefix = plotFolder() + this->name + "_" + m2->name;
  std::ofstream heatmap(_prefix + "__heatmap.txt");
  std::ofstream refFrames(_prefix + "__refFrames.txt");

  //---

  mixMap mm;
  {

    uint  auxF2   = 0;
    float auxDiff = std::numeric_limits<float>::infinity();

    for (auto f1 = 0u; f1 < this->frames.size(); f1++) {
      auxF2             = 0;
      auxDiff           = std::numeric_limits<float>::infinity();
      glm::vec3 f1Value = this->frames.at(f1).value();

      for (auto f2 = 0u; f2 < m2->frames.size(); f2++) {
        glm::vec3 f2Value = m2->frames.at(f2).value();

        float diff = glm::compAdd(glm::abs(f1Value - f2Value));
        if (diff < auxDiff) { std::tie(auxDiff, auxF2) = {diff, f2}; }

        // Write to heatmap
        (f2 < m2->frames.size() - 1) ? heatmap << diff << " "
                                     : heatmap << diff << "\n";
      }
      // Write to ref frames. For mark winner frames on heatmap
      refFrames << f1 << " " << auxF2 << "\n";

      // Insert winner frames and its transition on the map
      auto tMo = createTransitionMotion(f1, auxF2);
      mm.insert({f1, {auxF2, tMo}});
    }
  }

  return mm;
}

} // namespace brave
