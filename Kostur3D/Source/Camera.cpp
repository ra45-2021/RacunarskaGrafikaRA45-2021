#include "Header/Camera.h"
#include <cmath>

CameraController gCamera;

void CameraController::OnMouseMove(double xpos, double ypos)
{
    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos;
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(dir);
}

void CameraController::OnScroll(double yoffset)
{
    fov -= (float)yoffset;
    if (fov < 1.0f)  fov = 1.0f;
    if (fov > 45.0f) fov = 45.0f;
}

void CameraController::MovePlanar(float forwardAmt, float rightAmt)
{
    glm::vec3 f = glm::normalize(glm::vec3(front.x, 0.0f, front.z));
    glm::vec3 r = glm::normalize(glm::vec3(front.z, 0.0f, -front.x));

    pos += f * forwardAmt;
    pos += r * rightAmt;
}

glm::mat4 CameraController::View() const
{
    return glm::lookAt(pos, pos + front, up);
}

glm::mat4 CameraController::Projection(float aspect) const
{
    return glm::perspective(glm::radians(fov), aspect, 0.1f, 100.0f);
}

void MouseCallback(GLFWwindow*, double xpos, double ypos)
{
    gCamera.OnMouseMove(xpos, ypos);
}

void ScrollCallback(GLFWwindow*, double, double yoffset)
{
    gCamera.OnScroll(yoffset);
}
