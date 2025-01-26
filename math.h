#pragma once

namespace vkengine
{

   namespace math
   {
        Sphere calculateBoundingSphere(std::span<glm::vec3> positions);

        glm::vec3 smoothDamp(
            const glm::vec3& current,
            glm::vec3 target,
            glm::vec3& currentVelocity,
            float smoothTime,
            float dt,
            float maxSpeed);

        template<typename T> glm::vec<2, T> getIntersectionDepth(const Rect<T>& rectA, const Rect<T>& rectB);

        // if rect has negative width or height, left-top corner is changed
        // and w, h become positive in returned rect
        template<typename T> Rect<T> getAbsoluteRect(const Rect<T>& rect);
    }
}