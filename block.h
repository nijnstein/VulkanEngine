#pragma once
using namespace vkengine; 

enum class FaceType
{
	top, bottom, left, right, front, back
};

#define BLOCKTYPE       BYTE
#define BLOCKTYPE_COUNT 11

#define BT_AIR		    0 
#define BT_CLOUD        1
#define BT_STONE	    2 
#define BT_DIRT         3
#define BT_GRASS        4
#define BT_SNOW         5
#define BT_WOOD         6
#define BT_RED          7
#define BT_GREEN        8
#define BT_BLUE         9
#define BT_WHITE        10

static struct BlockInfo
{
    BLOCKTYPE type; 
    COLOR color; 
};

#define BLOCKINFO BlockInfo

static const BLOCKINFO BlockTypes[] =
{
    BLOCKINFO{ BT_AIR,      COLOR(0.0f, 0.0f, 0.0f, 0.0f) },
    BLOCKINFO{ BT_CLOUD,    COLOR(1.0f, 1.0f, 1.0f, 1.0f) },
    BLOCKINFO{ BT_STONE,    COLOR(0.5f, 0.5f, 0.5f, 1.0f) },
    BLOCKINFO{ BT_DIRT,     COLOR(0.4f, 0.3f, 0.2f, 1.0f) },
    BLOCKINFO{ BT_GRASS,    COLOR(0.0f, 0.9f, 0.0f, 1.0f) },
    BLOCKINFO{ BT_SNOW,     COLOR(1.0f, 1.0f, 1.0f, 1.0f) },
    BLOCKINFO{ BT_WOOD,     COLOR(0.9f, 0.2f, 0.0f, 1.0f) },
    BLOCKINFO{ BT_RED,      COLOR(1.0f, 0.0f, 0.0f, 1.0f) },
    BLOCKINFO{ BT_GREEN,    COLOR(0.0f, 1.0f, 0.0f, 1.0f) },
    BLOCKINFO{ BT_BLUE,     COLOR(0.0f, 0.0f, 1.0f, 1.0f) },
    BLOCKINFO{ BT_WHITE,    COLOR(1.0f, 1.0f, 1.0f, 1.0f) }
};

//             
//     f-------g                
//    /.      /|                
//   / .     / |                
//  e-------h  |                   (a) = origin(0,0,0) of mesh
//  |  b . .|. c     -y            (a) = 0,0,0 in blocks 
//  | .     | /       | / +z       (g) = max, max, max in blocks
//  |.      |/        |/        
//  a-------d         +---- +x  
//                              

#define ___a  0.0f, 0.0f, 0.0f 
#define ___b  0.0f, 0.0f, 1.0f 
#define ___c  1.0f, 0.0f, 1.0f
#define ___d  1.0f, 0.0f, 0.0f
 
#define ___e  0.0f, -1.0f, 0.0f 
#define ___f  0.0f, -1.0f, 1.0f 
#define ___g  1.0f, -1.0f, 1.0f
#define ___h  1.0f, -1.0f, 0.0f

static PACKED_VERTEX unitcube[6 * 6] =
{
    // BACK  +z  fgc/cbf             okish
    // xyz    v       r   g   b   nx       u   v   ny  nz
    {{ ___f,  0},   { 1,  1,  1,  0 },   { 0,  0,  0,  1 }},
    {{ ___g,  0},   { 1,  1,  1,  0 },   { 0,  0,  0,  1 }},
    {{ ___c,  0},   { 1,  1,  1,  0 },   { 0,  0,  0,  1 }},
    {{ ___c,  0},   { 1,  1,  1,  0 },   { 0,  0,  0,  1 }},
    {{ ___b,  0},   { 1,  1,  1,  0 },   { 0,  0,  0,  1 }},
    {{ ___f,  0},   { 1,  1,  1,  0 },   { 0,  0,  0,  1 }},
           
    // FRONT -z  adh/hea             ok
    // xyz    v       r   g   b   nx       u   v   ny  nz
    {{ ___a,  0},   { 1,  1,  1,  0 },   { 0,  0,  0, -1 }},
    {{ ___d,  0},   { 1,  1,  1,  0 },   { 0,  0,  0, -1 }},
    {{ ___h,  0},   { 1,  1,  1,  0 },   { 0,  0,  0, -1 }},
    {{ ___h,  0},   { 1,  1,  1,  0 },   { 0,  0,  0, -1 }},
    {{ ___e,  0},   { 1,  1,  1,  0 },   { 0,  0,  0, -1 }},
    {{ ___a,  0},   { 1,  1,  1,  0 },   { 0,  0,  0, -1 }},

    // LEFT -x   aeb/efb
    // xyz    v       r   g   b   nx       u   v   ny  nz
    {{ ___a,  0},   { 1,  1,  1, -1 },   { 0,  0,  0,  0 }},
    {{ ___e,  0},   { 1,  1,  1, -1 },   { 0,  0,  0,  0 }},
    {{ ___b,  0},   { 1,  1,  1, -1 },   { 0,  0,  0,  0 }},                                   
    {{ ___e,  0},   { 1,  1,  1, -1 },   { 0,  0,  0,  0 }},
    {{ ___f,  0},   { 1,  1,  1, -1 },   { 0,  0,  0,  0 }},
    {{ ___b,  0},   { 1,  1,  1, -1 },   { 0,  0,  0,  0 }}, 

    // RIGHT +x  dch/cgh
    // xyz    v       r   g   b   nx       u   v   ny  nz
    {{ ___d,  0},   { 1,  1,  1,  1 },   { 0,  0,  0,  0 }},
    {{ ___c,  0},   { 1,  1,  1,  1 },   { 0,  0,  0,  0 }},
    {{ ___h,  0},   { 1,  1,  1,  1 },   { 0,  0,  0,  0 }},                                   
    {{ ___c,  0},   { 1,  1,  1,  1 },   { 0,  0,  0,  0 }},
    {{ ___g,  0},   { 1,  1,  1,  1 },   { 0,  0,  0,  0 }},
    {{ ___h,  0},   { 1,  1,  1,  1 },   { 0,  0,  0,  0 }}, 


    // TOP  -y   ehg/gfe             ok
    // xyz   v        r   g   b   nx       u   v   ny  nz
    {{ ___e,  0},   { 1,  1,  1,  0 },   { 0,  0, -1,  0 }},
    {{ ___h,  0},   { 1,  1,  1,  0 },   { 0,  0, -1,  0 }},
    {{ ___g,  0},   { 1,  1,  1,  0 },   { 0,  0, -1,  0 }},
    {{ ___g,  0},   { 1,  1,  1,  0 },   { 0,  0, -1,  0 }},
    {{ ___f,  0},   { 1,  1,  1,  0 },   { 0,  0, -1,  0 }},
    {{ ___e,  0},   { 1,  1,  1,  0 },   { 0,  0, -1,  0 }}, 

    // BOTTOM +y  cda/abc            ok
    // xyz   v        r   g   b   nx       u   v   ny  nz
    {{ ___c,  0},   { 1,  1,  1,  0 },   { 0,  0,  1,  0 }},
    {{ ___d,  0},   { 1,  1,  1,  0 },   { 0,  0,  1,  0 }},
    {{ ___a,  0},   { 1,  1,  1,  0 },   { 0,  0,  1,  0 }},
    {{ ___a,  0},   { 1,  1,  1,  0 },   { 0,  0,  1,  0 }},
    {{ ___b,  0},   { 1,  1,  1,  0 },   { 0,  0,  1,  0 }},
    {{ ___c,  0},   { 1,  1,  1,  0 },   { 0,  0,  1,  0 }},
};

#define ___SET_FACE(___face, ___blocktype, ___offset_v3)               \
    PACKED_VERTEX* ___u = &unitcube[___face * 6];                      \
    VEC4 ___color = BlockTypes[___blocktype % BLOCKTYPE_COUNT].color;  \
    PACKED_VERTEX ___v;                                                \
    VEC4 ___pos_offset = VEC4(___offset_v3, 0);    

#define ___GEN_VERTEX(___vertices)                                     \
    ___v = *(___u++);                                                  \
    ___v.posAndValue += ___pos_offset;                                 \
    ___v.colorAndNormal.x = ___color.x;                                \
    ___v.colorAndNormal.y = ___color.y;                                \
    ___v.colorAndNormal.z = ___color.z;                                \
    ___vertices[0] = ___v;                                             \
    ___vertices++;                                                 
                                                                   
#define ___GEN_FACE(___face, ___vertices, ___blocktype, ___offset_v3)  \
    {                                                                  \
        ___SET_FACE(___face, ___blocktype, ___offset_v3)               \
        ___GEN_VERTEX(___vertices)                                     \
        ___GEN_VERTEX(___vertices)                                     \
        ___GEN_VERTEX(___vertices)                                     \
        ___GEN_VERTEX(___vertices)                                     \
        ___GEN_VERTEX(___vertices)                                     \
        ___GEN_VERTEX(___vertices)                                     \
    }  

#define ___GEN_VERTEX_JOIN(___vertices)                                \
    ___v = *(___u++);                                                  \
    ___v.posAndValue.x += ___v.posAndValue.x * ___join.x;              \
    ___v.posAndValue.y += ___v.posAndValue.y * ___join.y;              \
    ___v.posAndValue.z += ___v.posAndValue.z * ___join.z;              \
    ___v.posAndValue += ___pos_offset;                                 \
    ___v.colorAndNormal.x = ___color.x;                                \
    ___v.colorAndNormal.y = ___color.y;                                \
    ___v.colorAndNormal.z = ___color.z;                                \
    ___vertices[0] = ___v;                                             \
    ___vertices++;                                                 

#define ___GEN_FACE_JOIN(___face, ___vertices, ___blocktype, ___offset_v3_1, ___join_v3)     \
    {                                                                                        \
        PACKED_VERTEX* ___u = &unitcube[___face * 6];                                        \
        VEC4 ___color = BlockTypes[___blocktype % BLOCKTYPE_COUNT].color;                    \
        PACKED_VERTEX ___v;                                                                  \
        VEC4 ___pos_offset = VEC4(___offset_v3_1, 0);                                        \
        VEC3 ___join = VEC3(                                                                 \
            ___join_v3.x == 0 ? 0 : ___join_v3.x - ___offset_v3_1.x,                         \
            ___join_v3.y == 0 ? 0 : ___join_v3.y + ___offset_v3_1.y,                         \
            ___join_v3.z == 0 ? 0 : ___join_v3.z - ___offset_v3_1.z);                        \
        ___GEN_VERTEX_JOIN(___vertices)                                                      \
        ___GEN_VERTEX_JOIN(___vertices)                                                      \
        ___GEN_VERTEX_JOIN(___vertices)                                                      \
        ___GEN_VERTEX_JOIN(___vertices)                                                      \
        ___GEN_VERTEX_JOIN(___vertices)                                                      \
        ___GEN_VERTEX_JOIN(___vertices)                                                      \
    }



#define BACK_FACE   0
#define FRONT_FACE  1
#define LEFT_FACE   2
#define RIGHT_FACE  3
#define TOP_FACE    4
#define BOTTOM_FACE 5

#define ___GEN_CUBE(___vertices, ___blocktype, ___offset_v3)                \
        ___GEN_FACE(BACK_FACE,   ___vertices, ___blocktype, ___offset_v3)   \
        ___GEN_FACE(FRONT_FACE,  ___vertices, ___blocktype, ___offset_v3)   \
        ___GEN_FACE(LEFT_FACE,   ___vertices, ___blocktype, ___offset_v3)   \
        ___GEN_FACE(RIGHT_FACE,  ___vertices, ___blocktype, ___offset_v3)   \
        ___GEN_FACE(TOP_FACE,    ___vertices, ___blocktype, ___offset_v3)   \
        ___GEN_FACE(BOTTOM_FACE, ___vertices, ___blocktype, ___offset_v3)
        

 
 