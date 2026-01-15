#include "Header/MeshBuilders.h"
#include <cmath>

static void Push(std::vector<float>& out,
                 const glm::vec3& p,
                 const glm::vec4& c,
                 const glm::vec2& uv,
                 const glm::vec3& n)
{
    out.push_back(p.x); out.push_back(p.y); out.push_back(p.z);
    out.push_back(c.r); out.push_back(c.g); out.push_back(c.b); out.push_back(c.a);
    out.push_back(uv.x); out.push_back(uv.y);
    out.push_back(n.x); out.push_back(n.y); out.push_back(n.z);
}

void BuildCylinder(std::vector<float>& out,
                   float radius, float height, int seg,
                   const glm::vec4& color,
                   bool withBottom)
{
    out.clear();
    const float PI = 3.1415926f;

    float y0 = -height * 0.5f;
    float y1 =  height * 0.5f;

    for (int i = 0; i < seg; i++) {
        float a0 = (float)i / seg * 2.0f * PI;
        float a1 = (float)(i + 1) / seg * 2.0f * PI;

        glm::vec3 p00(radius * cos(a0), y0, radius * sin(a0));
        glm::vec3 p01(radius * cos(a1), y0, radius * sin(a1));
        glm::vec3 p10(radius * cos(a0), y1, radius * sin(a0));
        glm::vec3 p11(radius * cos(a1), y1, radius * sin(a1));

        glm::vec3 n0 = glm::normalize(glm::vec3(cos(a0), 0, sin(a0)));
        glm::vec3 n1 = glm::normalize(glm::vec3(cos(a1), 0, sin(a1)));

        Push(out, p00, color, {0,0}, n0);
        Push(out, p10, color, {0,1}, n0);
        Push(out, p11, color, {1,1}, n1);

        Push(out, p00, color, {0,0}, n0);
        Push(out, p11, color, {1,1}, n1);
        Push(out, p01, color, {1,0}, n1);
    }

    if (withBottom) {
        glm::vec3 n(0, -1, 0);
        glm::vec3 center(0, y0, 0);

        for (int i = 0; i < seg; i++) {
            float a0 = (float)i / seg * 2.0f * PI;
            float a1 = (float)(i + 1) / seg * 2.0f * PI;

            glm::vec3 p0(radius * cos(a0), y0, radius * sin(a0));
            glm::vec3 p1(radius * cos(a1), y0, radius * sin(a1));

            Push(out, center, color, {0.5f,0.5f}, n);
            Push(out, p1,     color, {0.5f,0.5f}, n);
            Push(out, p0,     color, {0.5f,0.5f}, n);
        }
    }
}

void BuildSphere(std::vector<float>& out,
                 float radius, int seg, int rings,
                 const glm::vec4& color)
{
    out.clear();
    const float PI = 3.1415926f;

    for (int y = 0; y < rings; y++) {
        float v0 = (float)y / rings;
        float v1 = (float)(y + 1) / rings;
        float th0 = v0 * PI;
        float th1 = v1 * PI;

        for (int x = 0; x < seg; x++) {
            float u0 = (float)x / seg;
            float u1 = (float)(x + 1) / seg;
            float ph0 = u0 * 2.0f * PI;
            float ph1 = u1 * 2.0f * PI;

            glm::vec3 p00(radius * sin(th0)*cos(ph0), radius * cos(th0), radius * sin(th0)*sin(ph0));
            glm::vec3 p10(radius * sin(th1)*cos(ph0), radius * cos(th1), radius * sin(th1)*sin(ph0));
            glm::vec3 p11(radius * sin(th1)*cos(ph1), radius * cos(th1), radius * sin(th1)*sin(ph1));
            glm::vec3 p01(radius * sin(th0)*cos(ph1), radius * cos(th0), radius * sin(th0)*sin(ph1));

            glm::vec3 n00 = glm::normalize(p00);
            glm::vec3 n10 = glm::normalize(p10);
            glm::vec3 n11 = glm::normalize(p11);
            glm::vec3 n01 = glm::normalize(p01);

            Push(out, p00, color, {u0, v0}, n00);
            Push(out, p10, color, {u0, v1}, n10);
            Push(out, p11, color, {u1, v1}, n11);

            Push(out, p00, color, {u0, v0}, n00);
            Push(out, p11, color, {u1, v1}, n11);
            Push(out, p01, color, {u1, v0}, n01);
        }
    }
}
