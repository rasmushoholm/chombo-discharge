/* chombo-discharge
 * Copyright © 2021 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_ItoPlasmaAir3LFA.H
  @brief  Ito plasma three-species model for air
  @author Robert Marskar
*/

#ifndef CD_ItoPlasmaAir3LFA_H
#define CD_ItoPlasmaAir3LFA_H

// Our includes
#include <CD_ItoPlasmaPhysicsLFA.H>
#include <CD_LookupTable.H>
#include <CD_NamespaceHeader.H>

namespace Physics {
  namespace ItoPlasma {

    /*!
      @brief Abstract clas for specifying plasma kinetics
    */
    class ItoPlasmaAir3LFA : public ItoPlasmaPhysicsLFA
    {
    public:
      class Electron;
      class Positive;
      class Negative;
      class PhotonZ;

      ItoPlasmaAir3LFA();
      ~ItoPlasmaAir3LFA();

      Real
      computeDt(const RealVect a_E, const RealVect a_pos, const Vector<Real> a_cdr_densities) const override;

      Real
      computeAlpha(const RealVect a_E) const override;

      Real
      computeEta(const RealVect a_E) const;

      void
      updateReactionRatesLFA(const RealVect a_E, const Real a_dx, const Real a_kappa) const override;

      Vector<Real>
      computeItoMobilitiesLFA(const Real a_time, const RealVect a_pos, const RealVect a_E) const override;

      Vector<Real>
      computeItoDiffusionLFA(const Real         a_time,
                             const RealVect     a_pos,
                             const RealVect     a_E,
                             const Vector<Real> a_cdr_densities) const override;

    protected:
      // for dt
      Real m_deltaX;

      // Stuff for initial conditions
      Real     m_blob_radius;
      Real     m_particle_weight;
      int      m_num_particles;
      RealVect m_blob_center;

      // Indices for species
      int m_ElectronIdx;
      int m_PositiveIdx;
      int m_NegativeIdx;
      int m_PhotonZ_idx;

      // Mobile ions or not
      bool m_isMobile_ions;

      // Ion mobility
      Real m_ion_mu;
      Real m_ion_D;

      // Pressure, neutral number density etc
      Real m_N;
      Real m_p;
      Real m_pq;
      Real m_T;
      Real m_O2frac;
      Real m_N2frac;
      Real m_pO2;
      Real m_photoi_factor;

      // Functions for drawing initial particles
      void
      drawInitialParticles();

      // Zheleznyak photoionization rates
      Real
      excitationRates(const Real a_E) const;
      Real
      sergeyFactor(const Real a_O2frac) const;
      Real
      photoionizationRate(const Real a_E) const;

      // Electron velocity
      RealVect
      computeElectronDriftVelocity(const RealVect a_E) const;

      // Read file into table
      void
      readTables();
    };

    class ItoPlasmaAir3LFA::Electron : public ItoSpecies
    {
    public:
      Electron();
      ~Electron();
    };

    class ItoPlasmaAir3LFA::Positive : public ItoSpecies
    {
    public:
      Positive();
      ~Positive();
    };

    class ItoPlasmaAir3LFA::Negative : public ItoSpecies
    {
    public:
      Negative();
      ~Negative();
    };

    class ItoPlasmaAir3LFA::PhotonZ : public RtSpecies
    {
    public:
      PhotonZ();
      ~PhotonZ();

      Real
      getAbsorptionCoefficient(const RealVect a_pos) const override;

    protected:
      Real m_pO2;
      Real m_f1;
      Real m_f2;
      Real m_K1;
      Real m_K2;

      Real                                  m_seed;
      std::mt19937_64*                      m_rng;
      std::uniform_real_distribution<Real>* m_udist01;
    };
  } // namespace ItoPlasma
} // namespace Physics

#include <CD_NamespaceFooter.H>

#endif
