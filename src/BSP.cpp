// BSP functions    by Alan Baylis 2001
// https://www.alsprogrammingresource.com/bsp.html

#include "BSP.h"

#include <GL/glew.h>

#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

GLuint g_bspEboId = 0;
GLuint g_bspVaoId = 0;

int Abs(int value)
{
    return value >= 0 ? value : -value;
}

int SelectPartitionfromList(POLYGON* nodepolylist, VERTEX* vertices, int numpolys, int* bestfront, int* bestback)
{
    int count = 0, result, absdifference = 1000000000, bestplane = 0, front, back, potentialplane, polytoclassify;
    VECTOR temp;

    // Loop through all the polygons and find the best splitting plane
    for(potentialplane = 0; potentialplane < numpolys; potentialplane++)
    {
        front = back = 0;
        for (polytoclassify = 0; polytoclassify < numpolys; polytoclassify++)
        {
            result = SplitPolygon(nodepolylist[polytoclassify], nodepolylist[potentialplane], NULL, vertices);
            switch (result)
            {
                case Front:
                    front++;
                break;

                case Back:
                    back++;
                break;

                case TwoFrontOneBack:
                    front += 2;
                    back++;
                break;

                case OneFrontTwoBack:
                    front++;
                    back += 2;
                break;

                case OneFrontOneBack:
                    front++;
                    back++;
                break;
            }
        }
        if (Abs(front - back) < absdifference)
        {
            absdifference = Abs(front - back);
            bestplane = potentialplane;
            *bestfront = front;
            *bestback = back;
        }
        if (front == 0 || back == 0)
            count++;
    }
    if (count == numpolys)
        return -1;
    else
        return bestplane;
}

void BuildBSP(BSP_node *node, VERTEX* vertices)
{
    int result, front, back, polytoclassify, partplane;
    POLYGON output[3];

    partplane = SelectPartitionfromList(node->nodepolylist, vertices, node->numpolys, &front, &back);

    if (partplane == -1)
    {
        glGenVertexArrays(1, &g_bspVaoId);
        glGenBuffers(1, &g_bspEboId);

        glBindVertexArray(g_bspVaoId);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_bspEboId);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * sizeof(unsigned int), (GLubyte*)0, GL_STREAM_DRAW);

        node->leaf = true;
        return;
    }

    node->partition = node->nodepolylist[partplane];

    //Allocate memory for a front and back node
    node->frontnode = new BSP_node;
    node->frontnode->leaf = 0;
    node->frontnode->numpolys = front;
    node->frontnode->nodepolylist = new POLYGON[front];

    node->backnode = new BSP_node;
    node->backnode->leaf = 0;
    node->backnode->numpolys = back;
    node->backnode->nodepolylist = new POLYGON[back];

    //Classify each polygon in the current node with respect to the partitioning plane.
    front = back = 0;
    for (polytoclassify = 0; polytoclassify < node->numpolys; polytoclassify++)
    {
        result = SplitPolygon(node->nodepolylist[polytoclassify], node->partition, output, vertices);
        switch (result)
        {
            case Front:
                node->frontnode->nodepolylist[front] = node->nodepolylist[polytoclassify];
                front++;
            break;

            case Back:
                node->backnode->nodepolylist[back] = node->nodepolylist[polytoclassify];
                back++;
            break;

            case TwoFrontOneBack:
                node->frontnode->nodepolylist[front] = output[0];
                node->frontnode->nodepolylist[front + 1] = output[1];
                front += 2;
                node->backnode->nodepolylist[back] = output[2];
                back++;
            break;

            case OneFrontTwoBack:
                node->frontnode->nodepolylist[front] = output[0];
                front++;
                node->backnode->nodepolylist[back] = output[1];
                node->backnode->nodepolylist[back + 1] = output[2];
                back += 2;
            break;

            case OneFrontOneBack:
                node->frontnode->nodepolylist[front] = output[0];
                front++;
                node->backnode->nodepolylist[back] = output[1];
                back++;
            break;
        }
    }

    node->numpolys = 0;
    delete[] node->nodepolylist;

    BuildBSP(node->frontnode, vertices);
    BuildBSP(node->backnode, vertices);
}

int RenderBSP(BSP_node *node, VERTEX* vertices, const glm::mat4& modelViewProjectionMatrix)
{
    int Side;
    VECTOR Position, edge1, edge2, planeNormal, temp;

    //The current position of the player/viewpoint
    Position.x = glm::column(modelViewProjectionMatrix, 3).x;
    Position.y = glm::column(modelViewProjectionMatrix, 3).y;
    Position.z = glm::column(modelViewProjectionMatrix, 3).z;

    if (!node->leaf)
    {
        // get the partitioning planes normal
        edge1.x = vertices[node->partition.mIndices[1]].x - vertices[node->partition.mIndices[0]].x;
        edge1.y = vertices[node->partition.mIndices[1]].y - vertices[node->partition.mIndices[0]].y;
        edge1.z = vertices[node->partition.mIndices[1]].z - vertices[node->partition.mIndices[0]].z;
        edge2.x = vertices[node->partition.mIndices[2]].x - vertices[node->partition.mIndices[0]].x;
        edge2.y = vertices[node->partition.mIndices[2]].y - vertices[node->partition.mIndices[0]].y;
        edge2.z = vertices[node->partition.mIndices[2]].z - vertices[node->partition.mIndices[0]].z;
        planeNormal = glm::cross(edge1, edge2);
        temp.x = vertices[node->partition.mIndices[0]].x;
        temp.y = vertices[node->partition.mIndices[0]].y;
        temp.z = vertices[node->partition.mIndices[0]].z;
        Side = ClassifyPoint(Position, temp, planeNormal);

        if (Side == -1)
        {
            RenderBSP(node->frontnode, vertices, modelViewProjectionMatrix);
            RenderBSP(node->backnode, vertices, modelViewProjectionMatrix);
        }
        else
        {
            RenderBSP(node->backnode, vertices, modelViewProjectionMatrix);
            RenderBSP(node->frontnode, vertices, modelViewProjectionMatrix);
        }
    }

    if (node->leaf)
    {
        //Draw polygons that are in the leaf
        for (int loop = 0; loop < node->numpolys; loop++)
        {
            glBindVertexArray(g_bspVaoId);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_bspEboId);
            unsigned int indices[3] = { node->nodepolylist[loop].mIndices[0], node->nodepolylist[loop].mIndices[2], node->nodepolylist[loop].mIndices[2] };
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 3 * sizeof(unsigned int), indices);

            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);
        }
    }
    return 1;
}

void DeleteBSP(BSP_node *node)
{
    if (node->leaf == true)
    {
        glDeleteBuffers(1, &g_bspEboId);
        glDeleteVertexArrays(1, &g_bspVaoId);

        delete[] node->nodepolylist;
        return;
    }

    DeleteBSP(node->frontnode);
    delete node->frontnode;
    DeleteBSP(node->backnode);
    delete node->backnode;
}

VECTOR GetEdgeIntersection(VECTOR point0, VECTOR point1, POLYGON planePolygon, VERTEX* vertices)
{
    VECTOR edge1, edge2, planeNormal, pointOnPlane, intersection, temp;
    float numerator, denominator, t;

    // get a point on the plane
    pointOnPlane.x = vertices[planePolygon.mIndices[0]].x;
    pointOnPlane.y = vertices[planePolygon.mIndices[0]].y;
    pointOnPlane.z = vertices[planePolygon.mIndices[0]].z;

    // get the splitting planes normal
    edge1.x = vertices[planePolygon.mIndices[1]].x - vertices[planePolygon.mIndices[0]].x;
    edge1.y = vertices[planePolygon.mIndices[1]].y - vertices[planePolygon.mIndices[0]].y;
    edge1.z = vertices[planePolygon.mIndices[1]].z - vertices[planePolygon.mIndices[0]].z;
    edge2.x = vertices[planePolygon.mIndices[2]].x - vertices[planePolygon.mIndices[0]].x;
    edge2.y = vertices[planePolygon.mIndices[2]].y - vertices[planePolygon.mIndices[0]].y;
    edge2.z = vertices[planePolygon.mIndices[2]].z - vertices[planePolygon.mIndices[0]].z;
    planeNormal = glm::cross(edge1, edge2);

// find edge intersection:
// intersection = p0 + (p1 - p0) * t
// where t = (planeNormal . (pointOnPlane - p0)) / (planeNormal . (p1 - p0))

    //planeNormal . (pointOnPlane - point0)
    temp.x = pointOnPlane.x - point0.x;
    temp.y = pointOnPlane.y - point0.y;
    temp.z = pointOnPlane.z - point0.z;
    numerator = glm::dot(planeNormal, temp);

    //planeNormal . (point1 - point0)
    temp.x = point1.x - point0.x;
    temp.y = point1.y - point0.y;
    temp.z = point1.z - point0.z;
    denominator = glm::dot(planeNormal, temp);

    if (denominator)
        t = numerator / denominator;
    else
        t = 0.0;

    intersection.x = point0.x + temp.x * t;
    intersection.y = point0.y + temp.y * t;
    intersection.z = point0.z + temp.z * t;

    return intersection;
}

int ClassifyPoint(VECTOR point, VECTOR pO, VECTOR pN)
{
    VECTOR TempVect;
    TempVect.x = pO.x - point.x;
    TempVect.y = pO.y - point.y;
    TempVect.z = pO.z - point.z;
    VECTOR dir = TempVect;
    float d = glm::dot(dir, pN);

    if (d < -0.001f)
        return 1;
    else
        if (d > 0.001f)
            return -1;
        return 0;
}

/*
 Although this function is called SplitPolygon, it really only splits triangles, the
 function inputs the triangle to be split and another triangle to be used for the splitting
 plane. The parameter 'polygons' is a pointer to 3 triangles for output.
 The function splits the input triangle into either 0, 2 or 3 new triangles with
 recalculated texture coordinates.
 If the polygons pointer happens to be NULL then the function will just set the outputFlag and return.

 The return value will be either Front, Back, TwoFrontOneBack, OneFrontTwoBack or OneFrontOneBack.
 Front means that all points are infront of the plane or the polygon lies on the plane and faces the front.
 Back means that all points are behind of the plane or the polygon lies on the plane and faces the back.
 TwoFrontOneBack means that polygons 1 and 2 are infront of the plane and polygon 3 is behind.
 OneFrontTwoBack means that polygon 1 is infront of the plane and polygons 2 and 3 are behind.
 OneFrontOneBack means that polygon 1 is infront of the plane and polygon 2 is behind, polygon 3 is not used.
*/
int SplitPolygon(POLYGON polygonToSplit, POLYGON planePolygon, POLYGON* polygons, VERTEX* vertices)
{
    VECTOR planeNormal, polysNormal, pointOnPlane, edge1, edge2, temp;
    VERTEX ptA, ptB, outpts[4], inpts[4], intersection;
    int count = 0, out_c = 0, in_c = 0, sideA, sideB, outputFlag;
    float x1, x2, y1, y2, z1, z2; //, u1, u2, v1, v2, scale; // texture calculation variables

    // get a point on the plane
    pointOnPlane.x = vertices[planePolygon.mIndices[0]].x;
    pointOnPlane.y = vertices[planePolygon.mIndices[0]].y;
    pointOnPlane.z = vertices[planePolygon.mIndices[0]].z;

    // get the splitting planes normal
    edge1.x = vertices[planePolygon.mIndices[1]].x - vertices[planePolygon.mIndices[0]].x;
    edge1.y = vertices[planePolygon.mIndices[1]].y - vertices[planePolygon.mIndices[0]].y;
    edge1.z = vertices[planePolygon.mIndices[1]].z - vertices[planePolygon.mIndices[0]].z;
    edge2.x = vertices[planePolygon.mIndices[2]].x - vertices[planePolygon.mIndices[0]].x;
    edge2.y = vertices[planePolygon.mIndices[2]].y - vertices[planePolygon.mIndices[0]].y;
    edge2.z = vertices[planePolygon.mIndices[2]].z - vertices[planePolygon.mIndices[0]].z;
    planeNormal = glm::cross(edge1, edge2);

    // get the normal of the polygon to split
    edge1.x = vertices[polygonToSplit.mIndices[1]].x - vertices[polygonToSplit.mIndices[0]].x;
    edge1.y = vertices[polygonToSplit.mIndices[1]].y - vertices[polygonToSplit.mIndices[0]].y;
    edge1.z = vertices[polygonToSplit.mIndices[1]].z - vertices[polygonToSplit.mIndices[0]].z;
    edge2.x = vertices[polygonToSplit.mIndices[2]].x - vertices[polygonToSplit.mIndices[0]].x;
    edge2.y = vertices[polygonToSplit.mIndices[2]].y - vertices[polygonToSplit.mIndices[0]].y;
    edge2.z = vertices[polygonToSplit.mIndices[2]].z - vertices[polygonToSplit.mIndices[0]].z;
    polysNormal = glm::cross(edge1, edge2);

    // check if the polygon lies on the plane
    for (int loop = 0; loop < 3; loop++)
    {
        temp.x = vertices[polygonToSplit.mIndices[loop]].x;
        temp.y = vertices[polygonToSplit.mIndices[loop]].y;
        temp.z = vertices[polygonToSplit.mIndices[loop]].z;
        if (ClassifyPoint(temp, pointOnPlane, planeNormal) == 0)
            count++;
        else
            break;
    }
    if (count == 3)
    {
        if (ClassifyPoint(polysNormal, pointOnPlane, planeNormal) == 1)
            return Front;
        if (ClassifyPoint(polysNormal, pointOnPlane, planeNormal) == -1)
            return Back;
    }

    // try to split the polygon
    ptA = vertices[polygonToSplit.mIndices[2]];
    temp.x = ptA.x;
    temp.y = ptA.y;
    temp.z = ptA.z;
    sideA = ClassifyPoint(temp, pointOnPlane, planeNormal);
    for (int i = -1; ++i < 3;)
    {
        ptB = vertices[polygonToSplit.mIndices[i]];
        temp.x = ptB.x;
        temp.y = ptB.y;
        temp.z = ptB.z;
        sideB = ClassifyPoint(temp, pointOnPlane, planeNormal);
        if (sideB > 0)
        {
            if (sideA < 0)
            {
                // find intersection
                edge1.x = ptA.x;
                edge1.y = ptA.y;
                edge1.z = ptA.z;
                edge2.x = ptB.x;
                edge2.y = ptB.y;
                edge2.z = ptB.z;

                temp = GetEdgeIntersection(edge1, edge2, planePolygon, vertices);
                intersection.x = temp.x;
                intersection.y = temp.y;
                intersection.z = temp.z;

                // find the new texture coordinates
                x1 = ptB.x - ptA.x;
                y1 = ptB.y - ptA.y;
                z1 = ptB.z - ptA.z;
                x2 = intersection.x - ptA.x;
                y2 = intersection.y - ptA.y;
                z2 = intersection.z - ptA.z;
                //u1 = ptA.u;
                //u2 = ptB.u;
                //v1 = ptA.v;
                //v2 = ptB.v;
                //scale = sqrt(x2*x2+y2*y2+z2*z2)/sqrt(x1*x1+y1*y1+z1*z1);
                //intersection.u = u1 + (u2-u1) * scale;
                //intersection.v = v1 + (v2-v1) * scale;

                outpts[out_c++] = inpts[in_c++] = intersection;
            }
            inpts[in_c++] = ptB;
        }
        else if (sideB < 0)
        {
            if (sideA > 0)
            {
                // find intersection
                edge1.x = ptA.x;
                edge1.y = ptA.y;
                edge1.z = ptA.z;
                edge2.x = ptB.x;
                edge2.y = ptB.y;
                edge2.z = ptB.z;

                temp = GetEdgeIntersection(edge1, edge2, planePolygon, vertices);
                intersection.x = temp.x;
                intersection.y = temp.y;
                intersection.z = temp.z;

                // find the new texture coordinates
                x1 = ptB.x - ptA.x;
                y1 = ptB.y - ptA.y;
                z1 = ptB.z - ptA.z;
                x2 = intersection.x - ptA.x;
                y2 = intersection.y - ptA.y;
                z2 = intersection.z - ptA.z;
                //u1 = ptA.u;
                //u2 = ptB.u;
                //v1 = ptA.v;
                //v2 = ptB.v;
                //scale = sqrt(x2*x2+y2*y2+z2*z2)/sqrt(x1*x1+y1*y1+z1*z1);
                //intersection.u = u1 + (u2-u1) * scale;
                //intersection.v = v1 + (v2-v1) * scale;

                outpts[out_c++] = inpts[in_c++] = intersection;
            }
            outpts[out_c++] = ptB;
        }
        else
            outpts[out_c++] = inpts[in_c++] = ptB;
            ptA = ptB;
            sideA = sideB;
    }

    if (in_c == 4)          // two polygons are infront, one behind
    {
        outputFlag = TwoFrontOneBack;
        if (polygons)
        {
            vertices[polygons[0].mIndices[0]] = inpts[0];
            vertices[polygons[0].mIndices[1]] = inpts[1];
            vertices[polygons[0].mIndices[2]] = inpts[2];
            vertices[polygons[1].mIndices[0]] = inpts[0];
            vertices[polygons[1].mIndices[1]] = inpts[2];
            vertices[polygons[1].mIndices[2]] = inpts[3];
            vertices[polygons[2].mIndices[0]] = outpts[0];
            vertices[polygons[2].mIndices[1]] = outpts[1];
            vertices[polygons[2].mIndices[2]] = outpts[2];
        }
    }
    else if (out_c == 4)    // one polygon is infront, two behind
    {
        outputFlag = OneFrontTwoBack;
        if (polygons)
        {
            vertices[polygons[0].mIndices[0]] = inpts[0];
            vertices[polygons[0].mIndices[1]] = inpts[1];
            vertices[polygons[0].mIndices[2]] = inpts[2];
            vertices[polygons[1].mIndices[0]] = outpts[0];
            vertices[polygons[1].mIndices[1]] = outpts[1];
            vertices[polygons[1].mIndices[2]] = outpts[2];
            vertices[polygons[2].mIndices[0]] = outpts[0];
            vertices[polygons[2].mIndices[1]] = outpts[2];
            vertices[polygons[2].mIndices[2]] = outpts[3];
        }
    }
    else if (in_c == 3 && out_c == 3)  // plane bisects the polygon
    {
        outputFlag = OneFrontOneBack;
        if (polygons)
        {
            vertices[polygons[0].mIndices[0]] = inpts[0];
            vertices[polygons[0].mIndices[1]] = inpts[1];
            vertices[polygons[0].mIndices[2]] = inpts[2];
            vertices[polygons[1].mIndices[0]] = outpts[0];
            vertices[polygons[1].mIndices[1]] = outpts[1];
            vertices[polygons[1].mIndices[2]] = outpts[2];
        }
    }
    else // then polygon must be totally infront of or behind the plane
    {
        int side;

        for (int loop = 0; loop < 3; loop++)
        {
            temp.x = vertices[polygonToSplit.mIndices[loop]].x;
            temp.y = vertices[polygonToSplit.mIndices[loop]].y;
            temp.z = vertices[polygonToSplit.mIndices[loop]].z;
            side = ClassifyPoint(temp, pointOnPlane, planeNormal);
            if (side == 1)
            {
                outputFlag = Front;
                break;
            }
            else if (side == -1)
            {
                outputFlag = Back;
                break;
            }
        }
    }

    return outputFlag;
}
