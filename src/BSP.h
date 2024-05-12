// BSP functions    by Alan Baylis 2001
// https://www.alsprogrammingresource.com/bsp.html

#pragma once

#include <assimp/mesh.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct POLYGON
{
    unsigned int mIndices[3];
};
//typedef aiFace POLYGON;
typedef aiVector3D VERTEX;
typedef glm::vec3 VECTOR;

enum {Front, Back, TwoFrontOneBack, OneFrontTwoBack, OneFrontOneBack};

typedef struct BSP_node
{
    int nodeid;             //unique identification number given to the node
    POLYGON partition;      //well use this to store the partitioning plane that divides our nodes up
    BSP_node *backnode;     //pointer to the node that is on the back side of our partitioning plane
    BSP_node *frontnode;    //pointer to the node that is on the front side of our partitioning plane
    bool leaf;              //true or false value for a node saying that this node has no children and only contains polygons
    int numpolys;           //number of polygons in the node, this will be 0 if the node is not a leaf
    POLYGON *nodepolylist;  //pointer to the list of polygons in the leaf node, again this list will be empty if the node is not a leaf
} BSP_node;

int SelectPartitionfromList(POLYGON* nodepolylist, VERTEX* vertices, int numpolys, int* bestfront, int* bestback);
void BuildBSP(BSP_node *node, VERTEX* vertices);
int RenderBSP(BSP_node *node, VERTEX* vertices, const glm::mat4& modelViewProjectionMatrix);
void DeleteBSP(BSP_node *node);
VECTOR GetEdgeIntersection(VECTOR point0, VECTOR point1, POLYGON planePolygon, VERTEX* vertices);
int ClassifyPoint(VECTOR point, VECTOR pO, VECTOR pN);
int SplitPolygon(POLYGON polygonToSplit, POLYGON planePolygon, POLYGON* polygons, VERTEX* vertices);
