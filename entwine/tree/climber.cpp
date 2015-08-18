/******************************************************************************
* Copyright (c) 2016, Connor Manning (connor@hobu.co)
*
* Entwine -- Point cloud indexing
*
* Entwine is available under the terms of the LGPL2 license. See COPYING
* for specific license text and more information.
*
******************************************************************************/

#include <entwine/tree/climber.hpp>

#include <entwine/types/point.hpp>

namespace entwine
{

Climber::Climber(const BBox& bbox, const Structure& structure)
    : m_structure(structure)
    , m_dimensions(structure.dimensions())
    , m_factor(structure.factor())
    , m_index(0)
    , m_levelIndex(0)
    , m_chunkId(structure.nominalChunkIndex())
    , m_depth(0)
    , m_sparseDepthBegin(
            structure.dynamicChunks() ? structure.sparseDepthBegin() : 0)
    , m_depthChunks(1)
    , m_chunkNum(0)
    , m_chunkPoints(structure.baseChunkPoints())
    , m_bbox(bbox)
{ }

void Climber::magnify(const Point& point)
{
    const Point& mid(m_bbox.mid());

    // Up: +4, Down: +0.
    const int z(m_dimensions == 3 && point.z >= mid.z ? 4 : 0);

    // North: +2, South: +0.
    const int y(point.y >= mid.y ? 2 : 0);

    // East: +1, West: +0.
    const int x(point.x >= mid.x ? 1 : 0);

    switch (x + y + z)
    {
        case Dir::swd: goSwd(); break;
        case Dir::sed: goSed(); break;
        case Dir::nwd: goNwd(); break;
        case Dir::ned: goNed(); break;
        case Dir::swu: goSwu(); break;
        case Dir::seu: goSeu(); break;
        case Dir::nwu: goNwu(); break;
        case Dir::neu: goNeu(); break;
    }
}

void Climber::climb(const Dir dir)
{
    if (++m_depth > m_structure.nominalChunkDepth())
    {
        if (!m_sparseDepthBegin || m_depth <= m_sparseDepthBegin)
        {
            const std::size_t chunkRatio(
                    (m_index - m_chunkId).getSimple() /
                    (m_chunkPoints / m_factor));

            assert(chunkRatio < m_factor);

            m_chunkId <<= m_dimensions;
            m_chunkId.incSimple();
            m_chunkId += chunkRatio * m_chunkPoints;

            if (m_depth >= m_structure.coldDepthBegin())
            {
                m_chunkNum =
                    ((m_chunkId - m_structure.coldIndexBegin()) / m_chunkPoints)
                    .getSimple();
            }

            m_depthChunks *= m_factor;
        }
        else
        {
            m_chunkNum += m_depthChunks;

            m_chunkId <<= m_dimensions;
            m_chunkId.incSimple();

            m_chunkPoints *= m_factor;
        }
    }

    m_index <<= m_dimensions;
    m_index.incSimple();
    m_index += dir;

    m_levelIndex <<= m_dimensions;
    m_levelIndex.incSimple();
}

bool SplitClimber::next(bool terminate)
{
    if (terminate || (m_depthEnd && depth() + 1 == m_depthEnd))
    {
        while (
                depth() &&
                static_cast<unsigned>(++m_traversal.back()) ==
                    m_structure.factor())
        {
            m_traversal.pop_back();
            m_splits /= 2;

            m_index = (m_index >> m_structure.dimensions()) - 1;

            m_xPos /= 2;
            m_yPos /= 2;
            m_zPos /= 2;
        }

        if (depth())
        {
            const auto current(m_traversal.back());
            ++m_index;

            if (current % 2)
            {
                // Odd numbers: W->E.
                ++m_xPos;
            }
            if (current == 2 || current == 6)
            {
                // 2 or 6: E->W, N->S.
                --m_xPos;
                ++m_yPos;
            }
            else if (current == 4)
            {
                // 4: E->W, S->N, D->U.
                --m_xPos;
                --m_yPos;
                ++m_zPos;
            }
        }
    }
    else
    {
        m_traversal.push_back(0);
        m_splits *= 2;

        m_index = (m_index << m_structure.dimensions()) + 1;

        m_xPos *= 2;
        m_yPos *= 2;
        m_zPos *= 2;
    }

    if (depth())
    {
        if (depth() < m_depthBegin) return next();
        else if (overlaps()) return true;
        else return next(true);
    }
    else
    {
        return false;
    }
}

} // namespace entwine

