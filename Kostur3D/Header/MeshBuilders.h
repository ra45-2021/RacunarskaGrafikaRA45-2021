#pragma once
#include <vector>
#include <glm/glm.hpp>

void BuildCylinder(std::vector<float>& out,
                   float radius, float height, int seg,
                   const glm::vec4& color,
                   bool withBottom);

void BuildSphere(std::vector<float>& out,
                 float radius, int seg, int rings,
                 const glm::vec4& color);
