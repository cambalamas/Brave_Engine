#include "Settings.hpp"

#include <fstream>
#include <functional>

#include "Logger.hpp"
#include "FileWatcher.hpp"

namespace brave {

// ========================================================================= //
// ========================================================================= //
// JSON Boilerplate
// ========================================================================= //

// --- C++ Types --- //
#define stdParse(var, defVal) \
  try {                       \
    var = m_json.at(#var);    \
  } catch (json::exception&) { var = defVal; }

// --- GLM Types --- //
#define glmParse(var, defVal)                                        \
  try {                                                              \
    auto v = m_json.at(#var);                                        \
    if (!v.is_array()) {                                             \
      throw json::other_error::create(501, "An array was expected"); \
    }                                                                \
    for (auto i = 0u; i < v.size(); ++i) { var[i] = v.at(i); }       \
  } catch (json::exception&) { var = defVal; }



// ========================================================================= //
// ========================================================================= //
// Private variables definition
// ========================================================================= //

nlohmann::json Settings::m_json;
FileWatcher    Settings::m_filewatcher;
std::string    Settings::m_path{""};
bool           Settings::m_corrupted{false};


// ====================================================================== //
// ====================================================================== //
// Public variables definition
// ====================================================================== //

bool        Settings::initialized{false};
bool        Settings::quiet{true};
int         Settings::openglMajorV{4};
int         Settings::openglMinorV{5};
int         Settings::windowWidth{800};
int         Settings::windowHeight{600};
std::string Settings::windowTitle{"Brave Engine"};
glm::vec3   Settings::clearColor{0.0f};
float       Settings::mouseSensitivity{1.0f};
bool        Settings::pollEvents{false};
glm::vec3   Settings::mainCameraPos{0.0f, 25.0f, 27.5f};
glm::vec2   Settings::mainCameraRot{45.0f, 0.0f};
float       Settings::mainCameraSpeed{0.1f};
glm::vec3   Settings::mainLightPos{0.0f, 5.0f, 0.0f};
glm::vec3   Settings::mainLightColor{1.0f};
float       Settings::mainLightIntensity{500.0f};


// ====================================================================== //
// ====================================================================== //
// Parse process
// ====================================================================== //

void Settings::init(const std::string& filePath) {

  m_path = filePath;
  std::fstream fsSettings(m_path);

  // Define update process
  auto callback = [&](std::fstream& f) {
    retry_pase:
      try {
        m_json      = (!m_path.empty()) ? json::parse(f) : "{}"_json;
        initialized = true;

        stdParse(quiet, true);
        stdParse(openglMajorV, 4);
        stdParse(openglMinorV, 5);
        stdParse(windowWidth, 800);
        stdParse(windowHeight, 600);
        stdParse(windowTitle, "Brave Engine");
        glmParse(clearColor, glm::vec3(0.2, 0.3, 0.3));
        stdParse(mouseSensitivity, 1.0);
        stdParse(pollEvents, false);
        glmParse(mainCameraPos, glm::vec3(0.f));
        glmParse(mainCameraRot, glm::vec2(0.f));
        stdParse(mainCameraSpeed, 0.1f);
        glmParse(mainLightPos, glm::vec3(0, 0, 0));
        glmParse(mainLightColor, glm::vec3(1.0, 0, 1.0));
        stdParse(mainLightIntensity, 1.f);

        m_corrupted = false;
      }

      catch (json::exception& e) {
        m_corrupted = true;
        LOGE("'{}' Bad parsing:\n{}", m_path, e.what());
        goto retry_pase;
      }
  };

  // Load values
  callback(fsSettings);

  // Setup filewatcher to background auto update
  if (!m_path.empty()) m_filewatcher.launch(m_path, callback);
}


// ====================================================================== //
// ====================================================================== //
// Print object values
// ====================================================================== //
#define stdPrint(var) LOG("{} => {}", #var, var);
#define glmPrint(var) LOG("{}: {}", #var, glm::to_string(var));

void Settings::dump() {

  auto status = (m_corrupted) ? " / Corrupted" : "";
  LOG("\nSETTINGS. {}{}\n  ---", m_path, status);
  stdPrint(quiet);
  stdPrint(openglMajorV);
  stdPrint(openglMinorV);
  stdPrint(windowWidth);
  stdPrint(windowHeight);
  stdPrint(windowTitle);
  glmPrint(clearColor);
  stdPrint(mouseSensitivity);
  stdPrint(pollEvents);
  glmPrint(mainCameraPos);
  glmPrint(mainCameraRot);
  stdPrint(mainCameraSpeed);
  glmPrint(mainLightPos);
  glmPrint(mainLightColor);
  stdPrint(mainLightIntensity);
  LOG("");
}


// ========================================================================= //
// ========================================================================= //
// Are settings corrupted?
// ========================================================================= //

bool Settings::corrupted() { return m_corrupted; }

} // namespace brave