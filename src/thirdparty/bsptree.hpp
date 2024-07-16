#pragma once

#include <cstdint>
#include <tuple>
#include <memory>
#include <limits>
#include <cmath>
#include <array>

#include <algorithm>
#include <ranges>

#ifdef PRINT_LOG
#include <iostream>
#include <chrono>
#endif

#ifdef PARALLEL
#include <execution>
#define EXECUTION_PAR std::execution::par,
#else
#define EXECUTION_PAR
#endif

namespace bsp {

/// specialize this template for your point type
/// you must provide the following value_type and two functions:
/// coordinate_type, which provides the type of one coordinate of a point
/// cross(const P & a, const P & b), which returns the cross product of both points
/// dot(const P a &, const P a &), which returns the dot product of both points
template <class P> struct point_traits;

/// specialize this template for your vertex type
/// you must provide the following position_type and two functions:
/// position_type, which provides the type of one position of the vertex
/// getPosition(const V & v), which returns a point_traits handled vector of the position
/// getInterpolatedVertex(v1, v2, f) which returns an interpolated vertex between
///      the two given vertices
template <class V> struct vertex_traits;

/// specialize this template for the container that
/// you want to use, the required fields are shown in this
/// default version that works for normal std containers with random access (e.g. vector und deque)
template<class V> struct container_traits
{
  typedef std::size_t size_type;
  typedef typename V::value_type value_type;
  static auto get(const V & v, size_type i) { return v[i]; }
  template <class coord_type>
  static size_type appendInterpolate(V & v, const value_type & a, const value_type & b, coord_type f)
  {
    size_type res = v.size();
    v.emplace_back(vertex_traits<value_type>::getInterpolatedVertex(a, b, f));
    return res;
  }
  static void append(V & v, const value_type & val)
  {
    v.push_back(val);
  }
  static void append(V & v, const V & v2)
  {
    v.insert(v.end(), v2.begin(), v2.end());
  }
  static size_type getSize(const V & v) noexcept
  {
    return v.size();
  }
  static void resize(V & v, size_type i)
  {
    v.resize(i);
  }
  static void reserve(V & v, size_type i)
  {
    v.reserve(i);
  }
};

/// A class for a bsp-Tree. The tree is meant for OpenGL usage. You input container of vertices and
/// a container of indices into the vertex container and you get a bsp tree that can order your
/// polygons from back to front.
/// Limitations:
///   - triangles only
///   - coplanar triangles must not overlap, drawing order will be undefined in this case
/// The class tries to minimize the number of triangles that need to be cut... only then does it
/// try to balance the tree because traversing the tree will always take the same amount of time
/// because the number of triangles in there is always the same
//
/// \tparam C the vertex container type, you need to specialize container_traits for this, if it
///         is not a vector or deque, you also need to specialize vertex_traits for the
///         contained vertices
/// \tparam I the index container type, the contained integer types need to be big enough to index
///         all vertices
/// \tparam E exponent of the epsilon value used to find out if a vertex is on the plane of a triangle.
///         Specify according to the scale of your object, E=0 mans 1, E=1 0.1, E=2 0.01 and so on, if
///         the vertex is less than that amount away from a plane it will be considered on the plane
template <class C, class I, container_traits<C>::size_type E = 5>
class BspTree
{
  protected:

    // the vertex type and the type for indexing the vertex container
    using size_type = typename container_traits<C>::size_type;
    using vertex_type = typename container_traits<C>::value_type;
    using index_type = typename container_traits<I>::value_type;

    // the point type of a vertex type
    using point_type = typename vertex_traits<vertex_type>::position_type;
    using coord_type = typename point_traits<point_type>::coordinate_type;

    typedef std::tuple<point_type, coord_type> Plane; // plane type

    // type for the node of the bsp-tree
    typedef struct Node {
      Plane plane; // the plane that intersects the space
      I triangles; // the triangles that are on this plane
      std::unique_ptr<struct Node> behind; // all that is behind the plane (relative to normal of plane)
      std::unique_ptr<struct Node> infront; // all that is in front of the plane
    } Node;

    // pointer to root of bsp-tree
    std::unique_ptr<Node> root_;

    // the vertices of all triangles within the tree
    C vertices_;

  protected:

    // Some internal helper functions

    static constexpr const point_type& normal(const Plane & plane) noexcept { return std::get<0>(plane); }
    static constexpr const coord_type& offset(const Plane & plane) noexcept { return std::get<1>(plane); }

    // a function that is used to contract the numbers between -1 and 1 into one, used
    // for the categorisation of a triangles relation to the cutting plane
    static constexpr size_type splitType(size_type a, size_type b, size_type c) noexcept
    {
      return (a+1)*16 + (b+1)*4 + (c+1);
    }

    // calculate distance of a point from a plane
    static coord_type distance(const Plane & plane, const point_type & t) noexcept
    {
      return (point_traits<point_type>::dot(normal(plane), t) - offset(plane));
    }

    // calculate the pow number of e
    static constexpr coord_type pow(coord_type i, size_type e) noexcept
    {
      if (e == 0) return 1;
      return i * pow(i, e - 1);
    }

    // calculate the sign of a number,
    static constexpr size_type sign(coord_type i) noexcept
    {
      // the epsilon value for deciding if a point is on a plane
      constexpr coord_type epsilon = pow(0.1, E);

      if (i >  epsilon) return 1;
      if (i < -epsilon) return -1;
      return 0;
    }

    // calculate the absolute number
    template <typename T>
    static constexpr T abs(T i) noexcept
    {
      return (i > 0 ? i : -i);
    }

    // calculate relative position of a point along a line which is a
    // units from one and b units from the other
    // assumes that either a or b are not zero
    static constexpr coord_type relation(coord_type a, coord_type b) noexcept
    {
      return abs(a) / (abs(a) + abs(b));
    }

    // calculate the plane in hessian normal form for the triangle with the indices given in the triple p
    Plane calculatePlane(size_type a, size_type b, size_type c) const noexcept
    {
      auto p1 = vertex_traits<vertex_type>::getPosition(get(vertices_, a));
      auto p2 = vertex_traits<vertex_type>::getPosition(get(vertices_, b));
      auto p3 = vertex_traits<vertex_type>::getPosition(get(vertices_, c));

      auto norm = point_traits<point_type>::cross(p2 - p1, p3 - p1);
      auto p = point_traits<point_type>::dot(norm, p1);

      return std::make_tuple(norm, p);
    }

    // append indices for a triangle to the index container
    void append(I & v, index_type v1, index_type v2, index_type v3) const
    {
      container_traits<I>::append(v, v1);
      container_traits<I>::append(v, v2);
      container_traits<I>::append(v, v3);
    }

    // helper function to get element from container using the traits
    // this is used so often that it is worth it here
    template <class T>
    auto get(const T & container, size_type i) const noexcept { return container_traits<T>::get(container, i); }

    // get the vertex that is pointed to by the i-th index in the index container
    vertex_type getVertIndex(size_type i, const I & indices) const noexcept
    {
      return get(vertices_, get(indices, i));
    }

    // separate the triangles within indices into the 3 lists of triangles that are behind, infront and on the
    // plane of the triangle given in pivot
    // when needed triangles are split and the smaller triangles are added to the proper lists
    // return the plane
    Plane separateTriangles(size_type pivot, const I & indices, I & behind, I & infront, I & onPlane)
    {
      // get the plane of the pivot triangle
      const Plane plane = calculatePlane(
        get(indices, pivot  ),
        get(indices, pivot+1),
        get(indices, pivot+2)
      );

      // go over all triangles and separate them
      for (size_type i = 0; i < container_traits<I>::getSize(indices); i+=3)
      {
        // calculate distance of the 3 vertices from the choosen partitioning plane
        std::array<coord_type, 3> dist
        {
          distance(plane, vertex_traits<vertex_type>::getPosition(getVertIndex(i  , indices))),
          distance(plane, vertex_traits<vertex_type>::getPosition(getVertIndex(i+1, indices))),
          distance(plane, vertex_traits<vertex_type>::getPosition(getVertIndex(i+2, indices)))
        };

        // check on which side of the plane the 3 points are
        std::array<size_type, 3> side { sign(dist[0]), sign(dist[1]), sign(dist[2]) };

        // if necessary create intermediate points for triangle
        // edges that cross the plane
        // the new points will be on the plane and will be new
        // vertices for new triangles
        // we only need to calculate the intermediate points for an
        // edge, when one vertex of the edge is on one side of the plan
        // and the other one on the other side, so when the product of
        // the signs is negative
        std::array<index_type, 3> A;

        if (side[0] * side[1] == -1)
        {
          A[0] = container_traits<C>::appendInterpolate(vertices_, getVertIndex(i  , indices), getVertIndex(i+1, indices), relation(dist[0], dist[1]));
        }
        if (side[1] * side[2] == -1)
        {
          A[1] = container_traits<C>::appendInterpolate(vertices_, getVertIndex(i+1, indices), getVertIndex(i+2, indices), relation(dist[1], dist[2]));
        }
        if (side[2] * side[0] == -1)
        {
          A[2] = container_traits<C>::appendInterpolate(vertices_, getVertIndex(i+2, indices), getVertIndex(i  , indices), relation(dist[2], dist[0]));
        }

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
            append(behind, get(indices, i), get(indices, i+1), get(indices, i+2));
            break;

          case splitType( 0,  0,  1):
          case splitType( 0,  1,  0):
          case splitType( 0,  1,  1):
          case splitType( 1,  0,  0):
          case splitType( 1,  0,  1):
          case splitType( 1,  1,  0):
          case splitType( 1,  1,  1):
            append(infront, get(indices, i  ), get(indices, i+1), get(indices, i+2));
            break;

          // triangle on the dividing plane
          default:
          case splitType( 0,  0,  0):
            append(onPlane, get(indices, i  ), get(indices, i+1), get(indices, i+2));
            break;

          // and now all the ways that the triangle can be cut by the plane
          case splitType( 1, -1,  0):
            append(behind,  get(indices, i+1), get(indices, i+2), A[0]);
            append(infront, get(indices, i+2), get(indices, i+0), A[0]);
            break;

          case splitType(-1,  0,  1):
            append(behind,  get(indices, i+0), get(indices, i+1), A[2]);
            append(infront, get(indices, i+1), get(indices, i+2), A[2]);
            break;

          case splitType( 0,  1, -1):
            append(behind,  get(indices, i+2), get(indices, i+0), A[1]);
            append(infront, get(indices, i+0), get(indices, i+1), A[1]);
            break;

          case splitType(-1,  1,  0):
            append(behind,  get(indices, i+2), get(indices, i+0), A[0]);
            append(infront, get(indices, i+1), get(indices, i+2), A[0]);
            break;

          case splitType( 1,  0, -1):
            append(behind,  get(indices, i+1), get(indices, i+2), A[2]);
            append(infront, get(indices, i+0), get(indices, i+1), A[2]);
            break;

          case splitType( 0, -1,  1):
            append(behind,  get(indices, i+0), get(indices, i+1), A[1]);
            append(infront, get(indices, i+2), get(indices, i+0), A[1]);
            break;

          case splitType( 1, -1, -1):
            append(infront, get(indices, i+0), A[0],              A[2]);
            append(behind,  get(indices, i+1), A[2],              A[0]);
            append(behind,  get(indices, i+1), get(indices, i+2), A[2]);
            break;

          case splitType(-1,  1, -1):
            append(infront, get(indices, i+1), A[1],              A[0]);
            append(behind,  get(indices, i+2), A[0],              A[1]);
            append(behind,  get(indices, i+2), get(indices, i+0), A[0]);
            break;

          case splitType(-1, -1,  1):
            append(infront, get(indices, i+2), A[2],              A[1]);
            append(behind,  get(indices, i+0), A[1],              A[2]);
            append(behind,  get(indices, i+0), get(indices, i+1), A[1]);
            break;

          case splitType(-1,  1,  1):
            append(behind,  get(indices, i+0), A[0],              A[2]);
            append(infront, get(indices, i+1), A[2],              A[0]);
            append(infront, get(indices, i+1), get(indices, i+2), A[2]);
            break;

          case splitType( 1, -1,  1):
            append(behind,  get(indices, i+1), A[1],              A[0]);
            append(infront, get(indices, i+0), A[0],              A[1]);
            append(infront, get(indices, i+2), get(indices, i+0), A[1]);
            break;

          case splitType( 1,  1, -1):
            append(behind,  get(indices, i+2), A[2],              A[1]);
            append(infront, get(indices, i+0), A[1],              A[2]);
            append(infront, get(indices, i+0), get(indices, i+1), A[1]);
            break;

        }
      }

      return plane;
    }

    typedef std::tuple<size_type, size_type> Pivot; // pivot type (number behind, number infront)
    // helper to find the good pivot point
    struct PivotCompare
    {
      // new pivot is better, if
      // the total number of triangles is lower (less division)
      // or equal and the triangles more equally distributed between left and right
      //bool operator()(const Pivot & lhs, const Pivot & rhs) const
      bool operator()(const std::pair<Pivot, size_type> & lhs, const std::pair<Pivot, size_type> & rhs) const
      {
        const auto [nb, nf]{ lhs.first };
        size_type ns = nb+nf;

        const auto [bb, bf]{ rhs.first };
        size_type bs = bb+bf;

        return ((ns < bs) || ((ns == bs) && (abs(nb-nf) < abs(bb-bf))));
      }
    };

    // check what would happen if the plane of pivot is used as a cutting plane for the triangles in indices
    // returns the number of triangles that would end up on the plane of pivot, behind it or in front of it
    Pivot evaluatePivot(size_type pivot, const I & indices) const noexcept
    {
      size_type behind = 0;
      size_type infront = 0;

      // count how many triangles would need to be cut, would lie behind and in front of the plane
      const Plane plane = calculatePlane(
        get(indices, pivot  ),
        get(indices, pivot+1),
        get(indices, pivot+2)
      );

      // this is a simplification of the algorithm above to just count the numbers of triangles
      for (size_type i = 0; i < container_traits<I>::getSize(indices); i+=3)
      {
        std::array<size_type, 3> side
        {
          sign(distance(plane, vertex_traits<vertex_type>::getPosition(getVertIndex(i  , indices)))),
          sign(distance(plane, vertex_traits<vertex_type>::getPosition(getVertIndex(i+1, indices)))),
          sign(distance(plane, vertex_traits<vertex_type>::getPosition(getVertIndex(i+2, indices))))
        };

        switch (splitType(side[0], side[1], side[2]))
        {
          case splitType(-1, -1, -1):
          case splitType(-1, -1,  0):
          case splitType(-1,  0, -1):
          case splitType(-1,  0,  0):
          case splitType( 0, -1, -1):
          case splitType( 0, -1,  0):
          case splitType( 0,  0, -1):
            behind++;
            break;

          case splitType( 0,  0,  1):
          case splitType( 0,  1,  0):
          case splitType( 0,  1,  1):
          case splitType( 1,  0,  0):
          case splitType( 1,  0,  1):
          case splitType( 1,  1,  0):
          case splitType( 1,  1,  1):
            infront++;
            break;

          default:
          case splitType( 0,  0,  0):
            break;

          case splitType(-1, -1,  1):
          case splitType(-1,  1, -1):
          case splitType( 1, -1, -1):
            behind += 2;
            infront++;
            break;

          case splitType(-1,  0,  1):
          case splitType( 1,  0, -1):
          case splitType(-1,  1,  0):
          case splitType( 1, -1,  0):
          case splitType( 0, -1,  1):
          case splitType( 0,  1, -1):
            behind++;
            infront++;
            break;

          case splitType(-1,  1,  1):
          case splitType( 1, -1,  1):
          case splitType( 1,  1, -1):
            behind++;
            infront += 2;
            break;
        }
      }

      return std::make_tuple(behind, infront);
    }

    // create the bsp tree for the triangles given in the indices vector, it returns the
    // pointer to the root of the tree
    // the function chooses a cutting plane and recursively recursively calls itself with
    // the lists of triangles that are behind and in front of the choosen plane
    std::unique_ptr<Node> makeTree(const I & indices)
    {
      if (container_traits<I>::getSize(indices) > 3)
      {
        size_type bestIndex = 0;
        Pivot bestPivot;

        { // find a good pivot element
#ifdef PRINT_LOG
          std::cout << "Start: " << std::format("{:%d/%m/%Y %H:%M:%S}", std::chrono::system_clock::now()) << std::endl;
#endif

          std::vector<std::pair<Pivot, size_type>> pivots(container_traits<I>::getSize(indices) / 3);

          { // parallelize all pivot evaluations
            auto stridedIndices = std::views::iota(size_type(0), container_traits<I>::getSize(indices)) | std::views::stride(3);
            std::transform(EXECUTION_PAR stridedIndices.cbegin(), stridedIndices.cend(), pivots.begin(),
              [this, &indices](size_type i) -> decltype(pivots)::value_type
              {
                return std::make_pair(evaluatePivot(i, indices), i);
              }
            );

            std::sort(EXECUTION_PAR pivots.begin(), pivots.end(), PivotCompare{});
          }

          bestPivot = pivots.begin()->first;
          bestIndex = pivots.begin()->second;
        }

        // create the node for this part of the tree
        auto node = std::make_unique<Node>();

        // container for the triangle indices for the triangles in front and behind the plane
        I behind, infront;
        container_traits<I>::reserve(behind, std::get<0>(bestPivot));
        container_traits<I>::reserve(infront, std::get<1>(bestPivot));

        // sort the triangles into the 3 containers
        node->plane = separateTriangles(bestIndex, indices, behind, infront, node->triangles);

#ifdef PRINT_LOG
        std::cout << "End: " << std::format("{:%d/%m/%Y %H:%M:%S}", std::chrono::system_clock::now())
          << ", best " << bestIndex
          << ", from " << indices.size()
          << ", remaining " << (behind.size() + infront.size())
          << ", on " << node->triangles.size()
          << ", behind " << behind.size()
          << ", infront " << infront.size()
          << std::endl;
#endif

        node->behind = makeTree(behind);
        node->infront = makeTree(infront);

        return node;
      }
      else if (container_traits<I>::getSize(indices) == 3)
      {
        // create the last node for this part of the tree
        // special case if epsilon is too small
        auto node = std::make_unique<Node>();
        node->plane = separateTriangles(0, indices, node->triangles, node->triangles, node->triangles);
        return node;
      }
      else
      {
        // this tree is empty, return nullptr
        return std::unique_ptr<Node>();
      }
    }

    // sort the triangles in the tree into the out container so that triangles far from p are
    // in front of the output vector
    void sortBackToFront(const point_type & p, const Node * n, I & out) const
    {
      if (!n) return;

      if (distance(n->plane, p) < 0)
      {
        sortBackToFront(p, n->infront.get(), out);
        container_traits<I>::append(out, n->triangles);
        sortBackToFront(p, n->behind.get(), out);
      }
      else
      {
        sortBackToFront(p, n->behind.get(), out);
        container_traits<I>::append(out, n->triangles);
        sortBackToFront(p, n->infront.get(), out);
      }
    }

  public:

    /// construct the tree, vertices are taken over, indices not
    /// the tree is constructed in such a way that the least number
    /// of triangles need to be split, if there is a way to build this
    /// tree without splitting, it will be found
    /// \param vertices, container with vertices, will be taken over and
    ///        new vertices appended, when necessary
    /// \param indices, container with indices into the vertices, each group of
    ///        3 corresponds to one triangle
    BspTree(C && vertices, const I & indices) : vertices_(std::move(vertices))
    {
      root_ = makeTree(indices);
    }

    /// another constructor that assumes that the vertices given are grouped in
    /// triples that each represents one triangle
    BspTree(C && vertices) : vertices_(std::move(vertices))
    {
      I indices;

      for (size_type i = 0; i < container_traits<C>::getSize(vertices_); i++)
      {
        container_traits<I>::append(indices, i);
      }

      root_ = makeTree(indices);
    }

    /// get the vertex container
    const C & getVertices() const noexcept { return vertices_; }

    /// get a container of indices for triangles so that the triangles are sorted
    /// from back to front when viewed from the given position
    /// \param p the point from where to look
    /// \return container of indices into the vertex container
    I sort(const point_type & p) const
    {
      I out;

      // TODO do we want to check, if the container elements are big enough?
      sortBackToFront(p, root_.get(), out);

      return out;
    }

};

}
