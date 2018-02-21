/*!
  @file species.cpp
  @brief Implementation of species.H
  @author Robert Marskar
  @date Jan. 2018
*/

#include "species.H"

species::species(){
  m_name         = "default_species";
  m_unit         = "default_unit";
  m_charge       = 0;
  m_diffusive    = true;
  m_force_output = false;
}

species::species(const std::string a_name, const int a_charge, const bool a_diffusive){
  m_name      = a_name;
  m_charge    = a_charge;
  m_diffusive = a_diffusive;
}

species::~species(){

}

Real species::initial_data(const RealVect a_pos, const Real a_time) const{
  return 0.;
}

std::string species::get_name() const {
  return m_name;
}

std::string species::get_unit() const {
  return m_unit;
}

int species::get_charge() const {
  return m_charge;
}

bool species::is_diffusive() const {
  return m_diffusive;
}

bool species::force_output() const {
  return m_force_output;
}
