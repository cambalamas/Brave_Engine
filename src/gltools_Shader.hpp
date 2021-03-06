#pragma once

#include <string>
#include <memory>
#include <unordered_map>

#include "gltools_Math.hpp"
#include "gltools_Camera.hpp"


namespace imog {

class Shader {

private:
  // Get a shared ptr to the shader from the global pool
  // by the concatenation of shaders paths
  static std::shared_ptr<Shader> getFromCache(const std::string& paths);

public:
  // Global pool for shaders
  static std::vector<std::shared_ptr<Shader>>          pool;
  static std::unordered_map<std::string, unsigned int> poolIndices;

  // Get a shared ptr to the shader from the global pool by name
  static std::shared_ptr<Shader> getByName(const std::string& name);

  // Create a new shader if it isn't on the gloabl pool
  static std::shared_ptr<Shader> create(const std::string& name,
                                        const std::string& vertexPath,
                                        const std::string& geomPath,
                                        const std::string& fragPath);

  // Create a new shader by a gived name, searching it in default forlder
  static std::shared_ptr<Shader> createByName(const std::string& name,
                                              bool hasGeometry    = false,
                                              bool hasTesselation = false);

  // Update all shaders of the pool
  static void poolUpdate(const std::shared_ptr<Camera>& camera);


private:
  std::string m_name;

  unsigned int m_program;

  std::unordered_map<std::string, int>  m_uCache;
  std::unordered_map<std::string, bool> m_alertCache; // Alert only once :D

  // Return the OpenGL state machine ID for a filePath
  // shader, if source compilation fails returns 0
  unsigned int loadShader(const std::string& filePath, unsigned int type);


public:
  // Param constructor //! DO NOT CALL THIS DIRECTLY, use Create.
  //
  // 1. Create new program
  // 2. Compile shaders:
  //    - required = vertex, fragment.
  //    - optional = geometry.
  // 3. Apped it to a created program
  // 4. Link program
  // 5. Verify that its linked (if not, delete it and prompt an alert)
  Shader(const std::string& name,
         const std::string& vertexPath,
         const std::string& geomPath,
         const std::string& fragPath);

  // Destructor
  ~Shader();


  // Bind set this program as active and use it to draw
  void bind();

  // Unbind unset this program as active so won't be used to draw
  void unbind();


  // Update upload to the shader camera data
  void update(const std::shared_ptr<Camera>& camera);


  // Returns the ID of the uniform associated to that string,
  // if its cached, return from cache, else request it to OpenGL
  // and store it.
  int uniform(const std::string& uniformName);

  // Upload a mat4 (view, proj, ...)
  void uMat4(const std::string& uniformName, const glm::mat4& mat);

  // Upload a float1 (height, intensity, ...)
  void uFloat1(const std::string& uniformName, float f);

  // Upload a vec3 (lightPos, color, ...)
  void uFloat3(const std::string& uniformName, float f1, float f2, float f3);

  // Upload a vec3 (lightPos, color, ...)
  void uFloat3(const std::string& uniformName, const glm::vec3& floats);

  // Upload a int1 (textures, ...)
  void uInt1(const std::string& uniformName, int i);
};

} // namespace imog
