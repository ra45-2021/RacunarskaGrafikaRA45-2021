#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

class CameraController {
public:
    glm::vec3 pos {0.0f, 0.3f, 2.8f};
    glm::vec3 front {0.0f, 0.0f, -1.0f};
    glm::vec3 up {0.0f, 1.0f, 0.0f};

    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 45.0f;

    bool firstMouse = true;
    float lastX = 0.0f;
    float lastY = 0.0f;

    void OnMouseMove(double xpos, double ypos);
    void OnScroll(double yoffset);

    void MovePlanar(float forward, float right); 

    glm::mat4 View() const;
    glm::mat4 Projection(float aspect) const;
};

extern CameraController gCamera;

void MouseCallback(GLFWwindow* window, double xpos, double ypos);
void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
