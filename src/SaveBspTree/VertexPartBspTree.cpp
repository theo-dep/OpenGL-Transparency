#include "VertexPartBspTree.h"

//--------------------------------------------------------------------------
VertexPartBspTree::VertexPartBspTree() : VertexBspTree()
{
}

//--------------------------------------------------------------------------
VertexPartBspTree::VertexPartBspTree(std::vector<Vertex> && vertices, const std::vector<unsigned int> & indices) : VertexBspTree(std::move(vertices), indices)
{
}

//--------------------------------------------------------------------------
VertexPartBspTree::VertexPartBspTree(std::vector<Vertex> && vertices) : VertexBspTree(std::move(vertices))
{
}

//--------------------------------------------------------------------------
std::shared_ptr<VertexPartBspTree> VertexPartBspTree::unify(const VertexPartBspTree & lhs, const VertexPartBspTree & rhs)
{
    std::vector<Vertex> result;

    lhs.classifyTree(rhs.root_.get(), rhs.vertices_, false, true, result);
    rhs.classifyTree(lhs.root_.get(), lhs.vertices_, false, false, result);

    return std::make_shared<VertexPartBspTree>(std::move(result));
}

//--------------------------------------------------------------------------
void VertexPartBspTree::append(std::vector<Vertex> & v, const Vertex & v1, const Vertex & v2, const Vertex & v3) const
{
    bsp::container_traits<std::vector<Vertex>>::append(v, v1);
    bsp::container_traits<std::vector<Vertex>>::append(v, v2);
    bsp::container_traits<std::vector<Vertex>>::append(v, v3);
}

//--------------------------------------------------------------------------
void VertexPartBspTree::classifyTree(const Node * tree, const std::vector<Vertex> & vertices, bool inside, bool keepEdge, std::vector<Vertex> & out) const
{
    if (tree)
    {
        std::size_t i = 0;
        std::size_t s = bsp::container_traits<std::vector<unsigned int>>::getSize(tree->triangles);
        while (i + 2 < s)
        {
            const Vertex & v1 = get(vertices, get(tree->triangles, i  ));
            const Vertex & v2 = get(vertices, get(tree->triangles, i+1));
            const Vertex & v3 = get(vertices, get(tree->triangles, i+2));

            classifyTriangle(tree, v1, v2, v3, inside, keepEdge, out, false);

            i += 3;
        }

        classifyTree(tree->infront.get(), vertices, inside, keepEdge, out);
        classifyTree(tree->behind.get(), vertices, inside, keepEdge, out);
    }
}

//--------------------------------------------------------------------------
bool VertexPartBspTree::classifyTriangle(const Node * n, const Vertex & v1, const Vertex & v2, const Vertex & v3, bool inside, bool keepEdge, std::vector<Vertex> & out, bool keepNow) const
{
    if (!n)
    {
        if (keepNow)
        {
            append(out, v1, v2, v3);
        }

        return keepNow;
    }

    // this is again a variation of the code in separateTriangles
    // calculate distance of the 3 vertices from the choosen partitioning plane
    std::array<float, 3> dist
    {
        distance(n->plane, v1.Position),
        distance(n->plane, v2.Position),
        distance(n->plane, v3.Position)
    };

    // check on which side of the plane the 3 points are
    std::array<std::size_t, 3> side { sign(dist[0]), sign(dist[1]), sign(dist[2]) };

    std::vector<Vertex> tmp;
    std::array<index_type, 3> A;

    if (side[0] * side[1] == -1)
    {
        A[0] = bsp::container_traits<std::vector<Vertex>>::appendInterpolate(tmp, v1, v2, relation(dist[0], dist[1]));
    }
    if (side[1] * side[2] == -1)
    {
        A[1] = bsp::container_traits<std::vector<Vertex>>::appendInterpolate(tmp, v2, v3, relation(dist[1], dist[2]));
    }
    if (side[2] * side[0] == -1)
    {
        A[2] = bsp::container_traits<std::vector<Vertex>>::appendInterpolate(tmp, v3, v1, relation(dist[2], dist[0]));
    }

    std::size_t preAddSize = bsp::container_traits<std::vector<Vertex>>::getSize(out);
    bool complete = true; // is the triangle completely added with all parts that it is split into
    bool clipped = true;  // is the triangle split at all

    // go over all possible positions of the 3 vertices relative to the plane
    switch (splitType(side[0], side[1], side[2]))
    {
        // all point on one side of the plane (or on the plane)
        // in this case we simply add the complete triangle
        // to the proper halve of the subtree
        case splitType(-1, -1, -1):
        case splitType(-1, -1,  0):
        case splitType(-1,  0, -1):
        case splitType(-1,  0,  0):
        case splitType( 0, -1, -1):
        case splitType( 0, -1,  0):
        case splitType( 0,  0, -1):
            complete = classifyTriangle(n->behind.get(), v1, v2, v3, inside, keepEdge, out, inside);
            clipped = false;
            break;

        case splitType( 0,  0,  1):
        case splitType( 0,  1,  0):
        case splitType( 0,  1,  1):
        case splitType( 1,  0,  0):
        case splitType( 1,  0,  1):
        case splitType( 1,  1,  0):
        case splitType( 1,  1,  1):
            complete = classifyTriangle(n->infront.get(), v1, v2, v3, inside, keepEdge, out, !inside);
            clipped = false;
            break;

        // triangle on the dividing plane
        case splitType( 0,  0,  0):
            if (keepEdge)
            {
                append(out, v1, v2, v3);
                clipped = false;
            }
            break;

        // and now all the ways that the triangle can be cut by the plane
        case splitType( 1, -1,  0):
            complete &= classifyTriangle(n->behind.get(),  v2, v3, tmp[A[0]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v3, v1, tmp[A[0]], inside, keepEdge, out, !inside);
            break;

        case splitType(-1,  0,  1):
            complete &= classifyTriangle(n->behind.get(),  v1, v2, tmp[A[2]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v2, v3, tmp[A[2]], inside, keepEdge, out, !inside);
            break;

        case splitType( 0,  1, -1):
            complete &= classifyTriangle(n->behind.get(),  v3, v1, tmp[A[1]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v1, v2, tmp[A[1]], inside, keepEdge, out, !inside);
            break;

        case splitType(-1,  1,  0):
            complete &= classifyTriangle(n->behind.get(),  v3, v1, tmp[A[0]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v2, v3, tmp[A[0]], inside, keepEdge, out, !inside);
            break;

        case splitType( 1,  0, -1):
            complete &= classifyTriangle(n->behind.get(),  v2, v3, tmp[A[2]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v1, v2, tmp[A[2]], inside, keepEdge, out, !inside);
            break;

        case splitType( 0, -1,  1):
            complete &= classifyTriangle(n->behind.get(),  v1, v2, tmp[A[1]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v3, v1, tmp[A[1]], inside, keepEdge, out, !inside);
            break;

        case splitType( 1, -1, -1):
            complete &= classifyTriangle(n->infront.get(), v1, tmp[A[0]], tmp[A[2]], inside, keepEdge, out, !inside);
            complete &= classifyTriangle(n->behind.get(),  v2, tmp[A[2]], tmp[A[0]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->behind.get(),  v2, v3,        tmp[A[2]], inside, keepEdge, out, inside);
            break;

        case splitType(-1,  1, -1):
            complete &= classifyTriangle(n->infront.get(), v2, tmp[A[1]], tmp[A[0]], inside, keepEdge, out, !inside);
            complete &= classifyTriangle(n->behind.get(),  v3, tmp[A[0]], tmp[A[1]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->behind.get(),  v3, v1,        tmp[A[0]], inside, keepEdge, out, inside);
            break;

        case splitType(-1, -1,  1):
            complete &= classifyTriangle(n->infront.get(), v3, tmp[A[2]], tmp[A[1]], inside, keepEdge, out, !inside);
            complete &= classifyTriangle(n->behind.get(),  v1, tmp[A[1]], tmp[A[2]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->behind.get(),  v1, v2,        tmp[A[1]], inside, keepEdge, out, inside);
            break;

        case splitType(-1,  1,  1):
            complete &= classifyTriangle(n->behind.get(),  v1, tmp[A[0]], tmp[A[2]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v2, tmp[A[2]], tmp[A[0]], inside, keepEdge, out, !inside);
            complete &= classifyTriangle(n->infront.get(), v2, v3,        tmp[A[2]], inside, keepEdge, out, !inside);
            break;

        case splitType( 1, -1,  1):
            complete &= classifyTriangle(n->behind.get(),  v2, tmp[A[1]], tmp[A[0]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v1, tmp[A[0]], tmp[A[1]], inside, keepEdge, out, !inside);
            complete &= classifyTriangle(n->infront.get(), v3, v1,        tmp[A[1]], inside, keepEdge, out, !inside);
            break;

        case splitType( 1,  1, -1):
            complete &= classifyTriangle(n->behind.get(),  v3, tmp[A[2]], tmp[A[1]], inside, keepEdge, out, inside);
            complete &= classifyTriangle(n->infront.get(), v1, tmp[A[1]], tmp[A[2]], inside, keepEdge, out, !inside);
            complete &= classifyTriangle(n->infront.get(), v1, v2,        tmp[A[1]], inside, keepEdge, out, !inside);
            break;

        default:
            complete = false;
            break;
    }

    if (complete && clipped)
    {
        // triangle has been split, but all parts are added to
        // the output buffer, so remove everything added and add
        // the whole triangle as a single unit
        bsp::container_traits<std::vector<Vertex>>::resize(out, preAddSize);
        append(out, v1, v2, v3);
    }

    return complete;
}
