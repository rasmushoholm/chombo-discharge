/* chombo-discharge
 * Copyright © 2021 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_ParticleOpsImplem.H
  @brief  Implementation of CD_ParticleOps.H
  @author Robert Marskar
*/

#ifndef CD_ParticleOpsImplem_H
#define CD_ParticleOpsImplem_H

// Chombo includes
#include <CH_Timer.H>
#include <PolyGeom.H>

// Our includes
#include <CD_ParticleOps.H>
#include <CD_PolyUtils.H>
#include <CD_NamespaceHeader.H>

inline bool
ParticleOps::domainIntersection(const RealVect& a_oldPos,
                                const RealVect& a_newPos,
                                const RealVect& a_probLo,
                                const RealVect& a_probHi,
                                Real&           a_s)
{

  // TLDR: This code does a boundary intersection test and returns where on the interval [oldPos, newPos] the intersection
  //       happened. We do this by checking if the particle moves towards a particular domain side and ends up outside of it.

  a_s = std::numeric_limits<Real>::max();

  bool crossedDomainBoundary = false;

  const RealVect path = a_newPos - a_oldPos;

  for (int dir = 0; dir < SpaceDim; dir++) {
    for (SideIterator sit; sit.ok(); ++sit) {
      const Side::LoHiSide side      = sit();
      const RealVect       wallPoint = (side == Side::Lo) ? a_probLo : a_probHi; // A point on the domain side
      const RealVect       n0 =
        sign(side) * RealVect(BASISV(dir)); // Normal vector pointing OUT of the domain on side sit and direction dir.
      const Real normPath = PolyGeom::dot(n0, path); // Component of the path that is normal to the domain edge/face.

      // If normPath > 0 then the particle trajectory points towards the domain edge/face and we can have an intersection.
      if (normPath > 0.0) {

        // s determines the intersection point between the particle path and the plane corresponding to the domain edge/face. Note that
        // we consider the edge/face to be an infinite plane and we just compute the intersection point between each edge/face and select the
        // closest intersection point.
        const Real s = PolyGeom::dot(wallPoint - a_oldPos, n0) / normPath;
        if (s >= 0.0 && s <= 1.0) {
          crossedDomainBoundary = true;
          if (s < a_s) {
            a_s = s;
          }
        }
      }
    }
  }

  return crossedDomainBoundary;
}

inline bool
ParticleOps::ebIntersectionBisect(const RefCountedPtr<BaseIF>& a_impFunc,
                                  const RealVect&              a_oldPos,
                                  const RealVect&              a_newPos,
                                  const Real&                  a_bisectStep,
                                  Real&                        a_s)
{

  // TLDR: We compute the intersection point using a bisection algorithm. We divide the full path into intervals and check if an interval
  //       has a root. If it does, we compute it using Brent's algorithm.

  a_s = std::numeric_limits<Real>::max();

  bool crossedEB = false;

  const Real     pathLen = (a_newPos - a_oldPos).vectorLength(); // Total path len
  const int      nsteps  = ceil(pathLen / a_bisectStep);         // Number of bisection intervals
  const RealVect dxStep  = (a_newPos - a_oldPos) / nsteps;       // Physical length of each bisection interval

  // Check each interval
  RealVect curPos = a_oldPos;
  for (int istep = 0; istep < nsteps; istep++) {
    const Real fa = a_impFunc->value(curPos); // Value of the implicit function at the start of the bisection interval
    const Real fb =
      a_impFunc->value(curPos + dxStep); // Value of the implicit function at the end of the bisection interval

    if (fa * fb <= 0.0) {

      // If this triggered we happen to know that f(pos+dxStep) > 0.0 and f(pos) < 0.0 and so we must have a root on the interval. We now compute the precise location
      // where the particle crossed the EB. For that we use a Brent root finder on the interval [pos, pos+dxStep]. This is a 1D problem.
      const RealVect intersectionPos = PolyUtils::brentRootFinder(a_impFunc, curPos, curPos + dxStep);
      a_s                            = (intersectionPos - a_oldPos).vectorLength() / pathLen;
      crossedEB                      = true;

      break;
    }
    else { // Move to next interval
      curPos += dxStep;
    }
  }

  return crossedEB;
}

inline bool
ParticleOps::ebIntersectionRaycast(const RefCountedPtr<BaseIF>& a_impFunc,
                                   const RealVect&              a_oldPos,
                                   const RealVect&              a_newPos,
                                   const Real&                  a_tolerance,
                                   Real&                        a_s)
{

  a_s = std::numeric_limits<Real>::max();

  bool ret = false;

  // Absolute distance to EB.
  auto dist = [&](const RealVect& x) -> Real { return std::abs(a_impFunc->value(x)); };

  const Real D  = (a_newPos - a_oldPos).vectorLength(); // Total particle path length
  const Real D0 = dist(a_oldPos);                       // Distance to EB from starting position

  // If the distance to the EB from the starting position is smaller than the total path length, we need to check for intersections.
  if (D > D0) {

    const RealVect t = (a_newPos - a_oldPos) / D; // Particle trajectory.

    // Move a_oldPos along +t. If we end up too close to the boundary the particle has intersected the BC. Note that this does NOT check for whether or not
    // the particle moves tangential to the EB surface. The length of each step is the distance to the EB, so if the particle is close to the EB but moves
    // tangentially to it, this routine will be EXTREMELY slow.
    RealVect xa = a_oldPos;
    Real     r  = D;
    Real     d  = dist(xa);

    while (d < r) {

      if (d < a_tolerance) { // We collided.
        a_s = (xa - a_oldPos).vectorLength() / D;
        ret = true;
        break;
      }
      else { // We did not collide.
        xa += t * d;
        r -= d;
        d = dist(xa);
      }
    }
  }

  return ret;
}

inline void
ParticleOps::computeParticleWeights(unsigned long long&      a_weight,
                                    unsigned long long&      a_num,
                                    unsigned long long&      a_remainder,
                                    const unsigned long long a_numPhysicalParticles,
                                    const int                a_ppc)
{
  if (a_numPhysicalParticles <= a_ppc) {
    a_weight    = 1;
    a_remainder = 0;
    a_num       = a_numPhysicalParticles;
  }
  else { // Add superparticles
    a_weight    = a_numPhysicalParticles / a_ppc;
    a_remainder = a_numPhysicalParticles % a_ppc;
    a_num       = (a_remainder == 0) ? a_ppc : a_ppc - 1;
  }
}

template <typename T>
inline T
ParticleOps::partitionParticles(const T a_numParticles)
{
#ifdef CH_MPI
  const T numProcs  = T(numProc());
  const T quotient  = a_numParticles / numProc();
  const T remainder = a_numParticles % numProc();

  Vector<T> particlesPerRank(numProc(), quotient);

  for (int i = 0; i < remainder; i++) {
    particlesPerRank[i]++;
  }

  return particlesPerRank[procID()];
#else
  return a_numParticles;
#endif
}

template <typename P, typename T>
inline void
ParticleOps::drawRandomParticles(List<P>&                         a_particles,
                                 const T                          a_numParticles,
                                 const std::function<RealVect()>& a_distribution)
{

  a_particles.clear();

  const T numParticles = ParticleOps::partitionParticles(a_numParticles);

  for (T t = 0; t < numParticles; t++) {
    P p;
    p.position() = a_distribution();

    a_particles.add(p);
  }
}

#include <CD_NamespaceFooter.H>

#endif
