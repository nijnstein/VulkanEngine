#include "defines.h"

namespace vkengine
{
	namespace math
	{
        inline static TriangleIntersection intersectTriangle(VEC3 rayOrigin, VEC3 rayDir, VEC3 A, VEC3 B, VEC3 C)
        {
            VEC3 E1 = B - A;
            VEC3 E2 = C - A;
            VEC3 N = CROSS(E1, E2);
            FLOAT det = -DOT(rayDir, N);
            FLOAT invdet = 1.0 / det;
            VEC3 AO = rayOrigin - A;
            VEC3 DAO = CROSS(AO, rayDir);
            FLOAT u = DOT(E2, DAO) * invdet;
            FLOAT v = -DOT(E1, DAO) * invdet;
            FLOAT t = DOT(AO, N) * invdet;
            return
            {
                .point = rayOrigin + t * rayDir,
                .uvi = {u, v, 1 - u - v},
                .intersects = (det >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u + v) <= 1.0)
            };
            // https://stackoverflow.com/questions/42740765/intersection-between-line-and-triangle-in-3d
            // If you want the segment version instead of the ray version, just add a check at the end that t
            // is less than the length of the segment and convert the segment to a ray where Dir is a unit vector.
            // And if you want the line version, get rid of the t >= 0 check altogethe
        }

        /// <summary>
        /// Returns the determinant of the 2x2 matrix defined as
        /// <list>
        /// <item>| x1 x2 |</item>
        /// <item>| y1 y2 |</item>
        /// </list>
        /// </summary>
        inline static FLOAT Det2(FLOAT x1, FLOAT x2, FLOAT y1, FLOAT y2)
        {
            return (x1 * y2 - y1 * x2);
        }

        inline static bool intersectLine(VEC2 v1, VEC2 v2, VEC2 v3, VEC2 v4, VEC2& intersection)
        {
            const FLOAT const tolerance = 0.000001f;

            FLOAT a = Det2(v1.x - v2.x, v1.y - v2.y, v3.x - v4.x, v3.y - v4.y);
            if (ABS(a) < 0.00000000001f)
            {
                // parallel
                return false;
            }

            FLOAT d1 = Det2(v1.x, v1.y, v2.x, v2.y);
            FLOAT d2 = Det2(v3.x, v3.y, v4.x, v4.y);
            FLOAT x = Det2(d1, v1.x - v2.x, d2, v3.x - v4.x) / a;
            FLOAT y = Det2(d1, v1.y - v2.y, d2, v3.y - v4.y) / a;

            if (x < MIN(v1.x, v2.x) - tolerance || x > MAX(v1.x, v2.x) + tolerance) return false;
            if (y < MIN(v1.y, v2.y) - tolerance || y > MAX(v1.y, v2.y) + tolerance) return false;
            if (x < MIN(v3.x, v4.x) - tolerance || x > MAX(v3.x, v4.x) + tolerance) return false;
            if (y < MIN(v3.y, v4.y) - tolerance || y > MAX(v3.y, v4.y) + tolerance) return false;

            intersection = { x, y };
            return true;
        }

        inline static bool intersectPlane(VEC3 rayDirection, VEC3 rayOrigin, VEC3 planeNormal, VEC3 coord, VEC3& intersection)
        {
            FLOAT dnr = DOT(planeNormal, rayDirection);
            if (dnr == 0)
            {
                // No intersection, the line is parallel to the plane
                return false;
            }

            // get d value
            FLOAT d = DOT(planeNormal, coord);

            // Compute the 'length' for the directed line ray intersecting the plane
            FLOAT t = (d - DOT(planeNormal, rayOrigin)) / dnr;

            // output contact point
            intersection = rayOrigin + NORM(rayDirection) * t;
            return true;
        }
	}
}