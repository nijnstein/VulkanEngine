#pragma once 
#include <glm/glm.hpp>

namespace vkengine
{
    namespace math
    {
        template<typename T> struct Rect;

        template<typename T> struct Rect
        {
            template<typename U>
            explicit Rect(const Rect<U>& r);

            template<typename T>
            Rect() : left(0), top(0), width(0), height(0)
            {
            }

            template<typename T>
            Rect(T left, T top, T width, T height) : left(left), top(top), width(width), height(height)
            {
            }
 
            template<typename T>
            Rect(const glm::vec<2, T>& position, const glm::vec<2, T>& size) : Rect(position.x, position.y, size.x, size.y)
            {
            }
                                
            template<typename T>
            bool contains(const glm::vec<2, T>& point) const
            {
                return (point.x >= left) && (point.x <= left + width) && (point.y >= top) && (point.y <= top + height);
            }

            template<typename T>
            bool intersects(const Rect<T>& rect) const
            {
                return getIntersectionDepth(*this, rect) != glm::vec<2, T>{T(0)};
            }

            template<typename T>
            glm::vec<2, T> getPosition() const
            {
                return { left, top };
            }

            template<typename T>
            void setPosition(const glm::vec<2, T>& position)
            {
                left = position.x;
                top = position.y;
            }

            template<typename T>
            glm::vec<2, T> getSize() const
            {
                return { width, height };
            }

            template<typename T>
            void setSize(const glm::vec<2, T>& size)
            {
                width = size.x;
                height = size.y;
            }


            template<typename T>
            glm::vec<2, T> getCenter() const
            {
                return { left + width / T(2), top + height / T(2) };
            }

            template<typename T>
            void move(const glm::vec<2, T>& offset)
            {
                setPosition(getPosition() + offset);
            }

            template<typename T>
            glm::vec<2, T> getTopLeftCorner() const
            {
                return { left, top };
            }

            template<typename T>
            glm::vec<2, T> getTopRightCorner() const
            {
                return { left + width, top };
            }

            template<typename T>
            glm::vec<2, T> getBottomLeftCorner() const
            {
                return { left, top + height };
            }

            template<typename T>
            glm::vec<2, T> getBottomRightCorner() const
            {
                return { left + width, top + height };
            }

            glm::vec<2, T> getCenter() const;

            void move(const glm::vec<2, T>& offset);

            // data members
            T left, top, width, height;
        };

        // equality operators
        template<typename T>
        bool operator==(const Rect<T>& a, const Rect<T>& b)
        {
            return a.left == b.left && a.top == b.top && a.width == b.width && a.height == b.height;
        }

        template<typename T>
        bool operator!=(const Rect<T>& a, const Rect<T>& b)
        {
            return !(a == b);
        }
    }
}
