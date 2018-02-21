/*!
  @file   air_11eed_species.cpp
  @brief  Implementation of air_11eed_species.H
  @author Robert Marskar
  @date   Feb. 2018
  @todo   Could definitely see ways to cut down on the typic 
*/

#include "air_11eed_species.H"
#include "units.H" 

#include <ParmParse.H>

air_11eed::eed::eed(){
  m_name = "electron energy density";
  m_unit = "eVm-3";
  m_charge = 0;

  // Get gas parameters
  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);

  { // Get initial energy
    Real init_ionization;
    ParmParse pp("air_11eed");
    pp.get("initial_electron_energy", m_init_energy);
    pp.get("initial_ionization", init_ionization);

    m_init_energy *= init_ionization;
  }
}

air_11eed::eed::~eed(){

}

air_11eed::electron::electron(){
  m_name   = "electron density";
  m_unit   = "m-3";
  m_charge = -1;

  {// Get initial parameter
    ParmParse pp("air_11eed");
    pp.get("initial_ionization", m_initial_ionization);
  }
}

air_11eed::electron::~electron(){

}

air_11eed::O2::O2(){
  m_name   = "O2";
  m_unit   = "m-3";
  m_charge = 0;

  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);
  m_initial_density = N*O2frac;
}

air_11eed::O2::~O2(){

}

air_11eed::N2::N2(){
  m_name   = "N2";
  m_unit   = "m-3";
  m_charge = 0.;

  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);
  m_initial_density = N*N2frac;
}

air_11eed::N2::~N2(){

}

air_11eed::N2plus::N2plus() {
  m_name   = "N2plus";
  m_unit   = "m-3";
  m_charge = 1;

  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);
  ParmParse pp("air_11eed");
  pp.get("initial_ionization", m_initial_ionization);
  m_initial_ionization *= N2frac;
}

air_11eed::N2plus::~N2plus(){

}

air_11eed::N4plus::N4plus(){
  m_name   = "N4plus";
  m_unit   = "m-3";
  m_charge = 1;
}

air_11eed::N4plus::~N4plus(){

}

air_11eed::O2plus::O2plus(){
  m_name   = "O2plus";
  m_unit   = "m-3";
  m_charge = 1;

  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);
  ParmParse pp("air_11eed");
  pp.get("initial_ionization", m_initial_ionization);
  m_initial_ionization *= O2frac;
}

air_11eed::O2plus::~O2plus(){

}

air_11eed::O4plus::O4plus(){
  m_name   = "O4plus";
  m_unit   = "m-3";
  m_charge = 1;
}

air_11eed::O4plus::~O4plus(){

}

air_11eed::O2plusN2::O2plusN2() {
  m_name   = "O2plusN2";
  m_unit   = "m-3";
  m_charge = 1;
}

air_11eed::O2plusN2::~O2plusN2(){

}

air_11eed::O2minus::O2minus(){
  m_name   = "O2minus";
  m_unit   = "m-3";
  m_charge = -1;
}

air_11eed::O2minus::~O2minus(){

}

air_11eed::Ominus::Ominus(){
  m_name   = "Ominus";
  m_unit   = "m-3";
  m_charge = -1;
}

air_11eed::Ominus::~Ominus(){

}

air_11eed::O::O(){
  m_name   = "O";
  m_unit   = "m-3";
  m_charge = 0;
}

air_11eed::O::~O(){

}

air_11eed::photon_one::photon_one(){
  m_name   = "photon_one";
  m_A      = 1.12E-4; // Default parameters
  m_lambda = 4.15E-2;

  { // Override from input script
    ParmParse pp("air_11eed");
    pp.query("photon1_A_coeff",      m_A);
    pp.query("photon1_lambda_coeff", m_lambda);
  }

  // Get gas stuff from input script
  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);
  m_pO2 = p*O2frac;
}

air_11eed::photon_one::~photon_one(){

}

air_11eed::photon_two::photon_two(){
  m_name   = "photon_two";
  m_A      = 2.88E-3; // Default parameters
  m_lambda = 1.09E-1;

  { // Override from input script
    ParmParse pp("air_11eed");
    pp.query("photon2_A_coeff",      m_A);
    pp.query("photon2_lambda_coeff", m_lambda);
  }

  // Get gas stuff from input script
  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);
  m_pO2 = p*O2frac;
}

air_11eed::photon_two::~photon_two(){

}

air_11eed::photon_three::photon_three(){
  m_name   = "photon_three";
  m_A      = 2.76E-1;
  m_lambda = 6.69E-1;

  { // Override from input script
    ParmParse pp("air_11eed");
    pp.query("photon3_A_coeff",      m_A);
    pp.query("photon3_lambda_coeff", m_lambda);
  }

  // Get gas stuff from input script
  Real Tg, p, N, O2frac, N2frac;
  air_11eed::get_gas_parameters(Tg, p, N, O2frac, N2frac);
  m_pO2 = p*O2frac;
}

air_11eed::photon_three::~photon_three(){

}

Real air_11eed::eed::initial_data(const RealVect a_pos, const Real a_time) const{
  return m_init_energy;
}

Real air_11eed::electron::initial_data(const RealVect a_pos, const Real a_time) const {
  return m_initial_ionization;
}

Real air_11eed::O2::initial_data(const RealVect a_pos, const Real a_time) const {
  return m_initial_density;
}

Real air_11eed::N2::initial_data(const RealVect a_pos, const Real a_time) const {
  return m_initial_density;
}

Real air_11eed::N2plus::initial_data(const RealVect a_pos, const Real a_time) const {
  return m_initial_ionization;
}

Real air_11eed::N4plus::initial_data(const RealVect a_pos, const Real a_time) const {
  return 0.0;
}

Real air_11eed::O2plus::initial_data(const RealVect a_pos, const Real a_time) const{
  return m_initial_ionization;
}

Real air_11eed::O4plus::initial_data(const RealVect a_pos, const Real a_time) const {
  return 0.0;
}

Real air_11eed::O2plusN2::initial_data(const RealVect a_pos, const Real a_time) const {
  return 0.0;
}

Real air_11eed::O2minus::initial_data(const RealVect a_pos, const Real a_time) const {
  return 0.0;
}

Real air_11eed::Ominus::initial_data(const RealVect a_pos, const Real a_time) const {
  return 0.0;
}

Real air_11eed::O::initial_data(const RealVect a_pos, const Real a_time) const {
  return 0.0;
}

Real air_11eed::photon_one::get_kappa(const RealVect a_pos) const {
  return m_lambda*m_pO2/(sqrt(3.0));
}

Real air_11eed::photon_one::get_lambda() const {
  return m_lambda;
}

Real air_11eed::photon_one::get_A() const {
  return m_A;
}

Real air_11eed::photon_one::get_pO2() const {
  return m_pO2;
}

Real air_11eed::photon_two::get_kappa(const RealVect a_pos) const {
  return m_lambda*m_pO2/(sqrt(3.0));
}

Real air_11eed::photon_two::get_lambda() const {
  return m_lambda;
}

Real air_11eed::photon_two::get_A() const {
  return m_A;
}

Real air_11eed::photon_two::get_pO2() const {
  return m_pO2;
}

Real air_11eed::photon_three::get_kappa(const RealVect a_pos) const{
  return m_lambda*m_pO2/(sqrt(3.0));
}

Real air_11eed::photon_three::get_lambda() const {
  return m_lambda;
}

Real air_11eed::photon_three::get_A() const {
  return m_A;
}

Real air_11eed::photon_three::get_pO2() const {
  return m_pO2;
}

