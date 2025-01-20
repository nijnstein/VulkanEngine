#pragma once

namespace vkengine
{
    struct AABB
    {
        VEC3 min = VEC3(INF);
        VEC3 max = VEC3(-INF);
        VEC3 size;
        VEC3 halfSize;
        VEC3 center;

        void FromVertices(VERTEX* vertices, SIZE size)
        {
            min = VEC3(INF);
            max = -min;

            for (int i = 0; i < size; i++)
            {
                VEC3 vertex = vertices[i].pos;
                min = MIN(vertex, min);
                max = MAX(vertex, max);
            }

            FromMinMax();
        }


        void FromVertices(PACKED_VERTEX* vertices, SIZE size)
        {
            min = VEC3(INF);
            max = -min;

            for (int i = 0; i < size; i++)
            {
                VEC3 vertex = VEC3(vertices[i].posAndValue);
                min = MIN(vertex, min);
                max = MAX(vertex, max);
            }

            FromMinMax();
        }


        void FromMinMax()
        {
            size = max - min;
            halfSize = size / 2.0f;
            center = min + halfSize;
        }

        static AABB fromBoxMinMax(VEC3 min, VEC3 max)
        {
            AABB aabb = {
                .min = min,
                .max = max
            };
            aabb.FromMinMax();
            return aabb; 
        }
    };
}