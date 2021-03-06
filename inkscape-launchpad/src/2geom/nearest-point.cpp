/*
 * nearest point routines for D2<SBasis> and Piecewise<D2<SBasis>>
 *
 * Authors:
 *
 * 		Marco Cecchetti <mrcekets at gmail.com>
 *
 * Copyright 2007-2008  authors
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 */


#include <2geom/nearest-point.h>
#include <algorithm>


namespace Geom
{

////////////////////////////////////////////////////////////////////////////////
// D2<SBasis> versions

/*
 * Return the parameter t of a nearest point on the portion of the curve "c",
 * related to the interval [from, to], to the point "p".
 * The needed curve derivative "dc" is passed as parameter.
 * The function return the first nearest point to "p" that is found.
 */

double nearest_point( Point const& p,
                      D2<SBasis> const& c,
                      D2<SBasis> const& dc,
                      double from, double to )
{
    if ( from > to ) std::swap(from, to);
    if ( from < 0 || to > 1 )
    {
        THROW_RANGEERROR("[from,to] interval out of bounds");
    }
    if (c.isConstant()) return from;
    SBasis dd = dot(c - p, dc);
    //std::cout << dd << std::endl;
    std::vector<double> zeros = Geom::roots(dd);

    double closest = from;
    double min_dist_sq = L2sq(c(from) - p);
    for ( size_t i = 0; i < zeros.size(); ++i )
    {
        double distsq = L2sq(c(zeros[i]) - p);
        if ( min_dist_sq > L2sq(c(zeros[i]) - p) )
        {
            closest = zeros[i];
            min_dist_sq = distsq;
        }
    }
    if ( min_dist_sq > L2sq( c(to) - p ) )
        closest = to;
    return closest;

}

/*
 * Return the parameters t of all the nearest points on the portion of
 * the curve "c", related to the interval [from, to], to the point "p".
 * The needed curve derivative "dc" is passed as parameter.
 */

std::vector<double>
all_nearest_points( Point const& p,
            D2<SBasis> const& c,
            D2<SBasis> const& dc,
            double from, double to )
{
    std::swap(from, to);
    if ( from > to ) std::swap(from, to);
    if ( from < 0 || to > 1 )
    {
        THROW_RANGEERROR("[from,to] interval out of bounds");
    }

    std::vector<double> result;
    if (c.isConstant())
    {
        result.push_back(from);
        return result;
    }
    SBasis dd = dot(c - p, dc);

    std::vector<double> zeros = Geom::roots(dd);
    std::vector<double> candidates;
    candidates.push_back(from);
    candidates.insert(candidates.end(), zeros.begin(), zeros.end());
    candidates.push_back(to);
    std::vector<double> distsq;
    distsq.reserve(candidates.size());
    for ( unsigned int i = 0; i < candidates.size(); ++i )
    {
        distsq.push_back( L2sq(c(candidates[i]) - p) );
    }
    unsigned int closest = 0;
    double dsq = distsq[0];
    for ( unsigned int i = 1; i < candidates.size(); ++i )
    {
        if ( dsq > distsq[i] )
        {
            closest = i;
            dsq = distsq[i];
        }
    }
    for ( unsigned int i = 0; i < candidates.size(); ++i )
    {
        if( distsq[closest] == distsq[i] )
        {
            result.push_back(candidates[i]);
        }
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////
// Piecewise< D2<SBasis> > versions


double nearest_point( Point const& p,
                      Piecewise< D2<SBasis> > const& c,
                      double from, double to )
{
    if ( from > to ) std::swap(from, to);
    if ( from < c.cuts[0] || to > c.cuts[c.size()] )
    {
        THROW_RANGEERROR("[from,to] interval out of bounds");
    }

    unsigned int si = c.segN(from);
    unsigned int ei = c.segN(to);
    if ( si == ei )
    {
        double nearest=
            nearest_point(p, c[si], c.segT(from, si), c.segT(to, si));
        return c.mapToDomain(nearest, si);
    }
    double t;
    double nearest = nearest_point(p, c[si], c.segT(from, si));
    unsigned int ni = si;
    double dsq;
    double mindistsq = distanceSq(p, c[si](nearest));
    Rect bb(Geom::Point(0,0),Geom::Point(0,0));
    for ( unsigned int i = si + 1; i < ei; ++i )
    {
        bb = *bounds_fast(c[i]);
        dsq = distanceSq(p, bb);
        if ( mindistsq <= dsq ) continue;
        t = nearest_point(p, c[i]);
        dsq = distanceSq(p, c[i](t));
        if ( mindistsq > dsq )
        {
            nearest = t;
            ni = i;
            mindistsq = dsq;
        }
    }
    bb = *bounds_fast(c[ei]);
    dsq = distanceSq(p, bb);
    if ( mindistsq > dsq )
    {
        t = nearest_point(p, c[ei], 0, c.segT(to, ei));
        dsq = distanceSq(p, c[ei](t));
        if ( mindistsq > dsq )
        {
            nearest = t;
            ni = ei;
        }
    }
    return c.mapToDomain(nearest, ni);
}

std::vector<double>
all_nearest_points( Point const& p,
                    Piecewise< D2<SBasis> > const& c,
                    double from, double to )
{
    if ( from > to ) std::swap(from, to);
    if ( from < c.cuts[0] || to > c.cuts[c.size()] )
    {
        THROW_RANGEERROR("[from,to] interval out of bounds");
    }

    unsigned int si = c.segN(from);
    unsigned int ei = c.segN(to);
    if ( si == ei )
    {
        std::vector<double>	all_nearest =
            all_nearest_points(p, c[si], c.segT(from, si), c.segT(to, si));
        for ( unsigned int i = 0; i < all_nearest.size(); ++i )
        {
            all_nearest[i] = c.mapToDomain(all_nearest[i], si);
        }
        return all_nearest;
    }
    std::vector<double> all_t;
    std::vector< std::vector<double> > all_np;
    all_np.push_back( all_nearest_points(p, c[si], c.segT(from, si)) );
    std::vector<unsigned int> ni;
    ni.push_back(si);
    double dsq;
    double mindistsq = distanceSq( p, c[si](all_np.front().front()) );
    Rect bb(Geom::Point(0,0),Geom::Point(0,0));
    for ( unsigned int i = si + 1; i < ei; ++i )
    {
        bb = *bounds_fast(c[i]);
        dsq = distanceSq(p, bb);
        if ( mindistsq < dsq ) continue;
        all_t = all_nearest_points(p, c[i]);
        dsq = distanceSq( p, c[i](all_t.front()) );
        if ( mindistsq > dsq )
        {
            all_np.clear();
            all_np.push_back(all_t);
            ni.clear();
            ni.push_back(i);
            mindistsq = dsq;
        }
        else if ( mindistsq == dsq )
        {
            all_np.push_back(all_t);
            ni.push_back(i);
        }
    }
    bb = *bounds_fast(c[ei]);
    dsq = distanceSq(p, bb);
    if ( mindistsq >= dsq )
    {
        all_t = all_nearest_points(p, c[ei], 0, c.segT(to, ei));
        dsq = distanceSq( p, c[ei](all_t.front()) );
        if ( mindistsq > dsq )
        {
            for ( unsigned int i = 0; i < all_t.size(); ++i )
            {
                all_t[i] = c.mapToDomain(all_t[i], ei);
            }
            return all_t;
        }
        else if ( mindistsq == dsq )
        {
            all_np.push_back(all_t);
            ni.push_back(ei);
        }
    }
    std::vector<double> all_nearest;
    for ( unsigned int i = 0; i < all_np.size(); ++i )
    {
        for ( unsigned int j = 0; j < all_np[i].size(); ++j )
        {
            all_nearest.push_back( c.mapToDomain(all_np[i][j], ni[i]) );
        }
    }
    all_nearest.erase(std::unique(all_nearest.begin(), all_nearest.end()),
                      all_nearest.end());
    return all_nearest;
}

} // end namespace Geom


