/* chombo-discharge
 * Copyright © 2021 SINTEF Energy Research.
 * Please refer to Copyright.txt and LICENSE in the chombo-discharge root directory.
 */

/*!
  @file   CD_Random.H
  @brief  File containing some useful static methods related to random number generation
  @author Robert Marskar
*/

#ifndef CD_Random_H
#define CD_Random_H

// Std includes
#include <random>
#include <memory>
#include <mutex>

// Chombo includes
#include <REAL.H>
#include <RealVect.H>

// Our includes
#include <CD_NamespaceHeader.H>

/*!
  @brief Class for encapsulating random number generation. 
  @details The user can specify a seed 's' where each MPI rank will initialize their RNG with seed 's + procID()'. Note that unless the user specifies 
  Random.seed = <number> in the input script, the RNG will use a seed of '0 + procID()' for the various ranks. If the user specifies a <number> less than 0, 
  a random seed will be used.
*/
class Random
{
public:
  /*!
    @brief Disallowed constructor. 
  */
  Random() = delete;

  /*!
    @brief Deleted copy constructor. Must be impossible because this is a singleton. 
    @param[in] a_other Other RNG engine. 
  */
  Random(const Random& a_other) = delete;

  /*!
    @brief Deleted copy constructor. Must be impossible because this is a singleton. 
    @param[in] a_other Other RNG engine. 
  */
  Random(const Random&& a_other) = delete;

  /*!
    @brief Disallowed copy assignemtn. 
    @param[in] a_other Other RNG engine. 
  */
  Random&
  operator=(const Random& a_other) = delete;

  /*!
    @brief Disallowed move assignment. 
    @param[in] a_other Other RNG engine. 
  */
  Random&
  operator=(const Random&& a_other) = delete;

  /*!
    @brief Get Poisson distributed number
  */
  template <typename T>
  inline static T
  getPoisson(const Real a_mean);

  /*!
    @brief Get a uniform real number on the interval [0,1]
  */
  inline static Real
  getUniformReal01();

  /*!
    @brief Get a uniform real number on the interval [-1,1]
  */
  inline static Real
  getUniformReal11();

  /*!
    @brief Get a number from a normal distribution centered on zero and variance 1
  */
  inline static Real
  getNormal01();

  /*!
    @brief Get a random direction in space.
    @details Uses Marsaglia algorithm. 
  */
  inline static RealVect
  getDirection();

  /*!
    @brief For getting a random number from a user-supplied distribution. T must be a distribution for which we can call T(s_rng)
    @param[in] a_distribution Distribution. Must have object of type
  */
  template <typename T>
  inline static Real
  get(T& a_distribution);

  /*!
    @brief Seed the RNG
  */
  inline static void
  seed();

private:
  /*!
    @brief For checking if RNG has been seeded or not.
  */
  static bool s_seeded;

  /*!
    @brief Random number generator
  */
  static std::mt19937_64 s_rng;

  /*!
    @brief For drawing random number on the interval [0,1]
  */
  static std::uniform_real_distribution<Real> s_uniform01;

  /*!
    @brief For drawing random number on the interval [-1,1]
  */
  static std::uniform_real_distribution<Real> s_uniform11;

  /*!
    @brief Normal distribution centered at zero with standard deviation of one
  */
  static std::normal_distribution<Real> s_normal01;

  /*!
    @brief Set the RNG seed.
    @details If using MPI, the seed is set to a_seed + procID().
    @param[in] a_seed Seed to use for RNG.
  */
  inline static void
  setSeed(const int a_seed);

  /*!
    @brief Set a random RNG seed
  */
  inline static void
  setRandomSeed();
};

#include <CD_NamespaceFooter.H>

#include <CD_RandomImplem.H>

#endif
