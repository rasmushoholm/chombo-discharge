/*!
  @file   cdr_layout.cpp
  @brief  Implementation of cdr_layout.H
  @author Robert Marskar
  @date   Jan. 2018
*/

#include "cdr_layout.H"
#include "cdr_iterator.H"
#include "units.H"
#include "data_ops.H"

#include <ParmParse.H>

cdr_layout::cdr_layout(){

}

cdr_layout::cdr_layout(const Vector<RefCountedPtr<cdr_species> >& a_species){
  m_species = a_species;
  m_solvers.resize(0);
}

cdr_layout::~cdr_layout(){

}

phase::which_phase cdr_layout::get_phase() const{
  return m_phase;
}

cdr_iterator cdr_layout::iterator() {
  return cdr_iterator(*this);
}

void cdr_layout::add_solver(RefCountedPtr<cdr_solver> a_solver){
  m_solvers.push_back(a_solver);
}

void cdr_layout::parse_options(){
  CH_TIME("cdr_layout::parse_options");
  if(m_verbosity > 6){
    pout() << "cdr_layout::parse_options" << endl;
  }

  for (cdr_iterator solver_it = this->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->parse_options();
  }
}

void cdr_layout::allocate_internals(){
  CH_TIME("cdr_layout::allocate_internals");
  if(m_verbosity > 6){
    pout() << "cdr_layout::allocate_internals" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->allocate_internals();
  }
}

void cdr_layout::deallocate_internals(){
  CH_TIME("cdr_layout::deallocate_internals");
  if(m_verbosity > 6){
    pout() << "cdr_layout::deallocate_internals" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->deallocate_internals();
  }
}

void cdr_layout::cache_states(){
  CH_TIME("cdr_layout::cache_states");
  if(m_verbosity > 6){
    pout() << "cdr_layout::cache_states" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->cache_state();
  }
}

void cdr_layout::initial_data(){
  CH_TIME("cdr_layout::initial_data");
  if(m_verbosity > 6){
    pout() << "cdr_layout::initial_data" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->initial_data();
  }
}

void cdr_layout::regrid(const int a_lmin, const int a_old_finest_level, const int a_new_finest_level){
  CH_TIME("cdr_layout::regrid");
  if(m_verbosity > 5){
    pout() << "cdr_layout::regrid" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->regrid(a_lmin, a_old_finest_level, a_new_finest_level);
  }
}

void cdr_layout::set_amr(const RefCountedPtr<amr_mesh>& a_amr){
  CH_TIME("cdr_layout::set_amr");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_amr" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_amr(a_amr);
  }
}

void cdr_layout::set_computational_geometry(const RefCountedPtr<computational_geometry>& a_compgeom){
  CH_TIME("cdr_layout::set_computational_geometry");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_computational_geometry" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_computational_geometry(a_compgeom);
  }
}

void cdr_layout::set_phase(phase::which_phase a_phase){
  CH_TIME("cdr_layout::set_phase");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_phase" << endl;
  }

  m_phase = a_phase;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_phase(a_phase);
  }
}

void cdr_layout::set_verbosity(const int a_verbosity){
  CH_TIME("cdr_layout::set_verbosity");
  m_verbosity = a_verbosity;
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_verbosity" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_verbosity(a_verbosity);
  }
}

void cdr_layout::set_bc(cdr_bc::which_bc a_bc){
  CH_TIME("cdr_layout::set_bc");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_bc" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_domain_bc(a_bc);
  }
}

void cdr_layout::sanity_check(){
  CH_TIME("cdr_layout::sanity_check");
  if(m_verbosity > 5){
    pout() << "cdr_layout::sanity_check" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->sanity_check();
  }
}

void cdr_layout::set_mass_redist(const bool a_mass_redist){
  CH_TIME("cdr_layout::set_mass_redist");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_mass_redist" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_mass_redist(a_mass_redist);
  }
}

void cdr_layout::set_time(const int a_step, const Real a_time, const Real a_dt) {
  CH_TIME("cdr_layout::set_time");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_time" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_time(a_step, a_time, a_dt);
  }
}

void cdr_layout::set_velocity(const EBAMRCellData& a_velo){
  CH_TIME("cdr_layout::set_velocity(ebamrcell)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_velocity(ebamrcell)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_velocity(a_velo);
  }
}

void cdr_layout::set_velocity(const RealVect a_velo){
  CH_TIME("cdr_layout::set_velocity(constant)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_velocity(constant)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_velocity(a_velo);
  }
}

void cdr_layout::set_diffco(const EBAMRFluxData& a_diffco, const EBAMRIVData& a_diffco_eb){
  CH_TIME("cdr_layout::set_diffco(ebamrflux)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_diffco(ebamrflux)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_diffco(a_diffco, a_diffco_eb);
  }
}

void cdr_layout::set_diffco(const Real a_diffco){
  CH_TIME("cdr_layout::set_diffco(constant)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_diffco(constant)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_diffco(a_diffco);
  }
}

void cdr_layout::set_source(const EBAMRCellData& a_source){
  CH_TIME("cdr_layout::set_source(ebamrcell)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_source(ebamrcell)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_source(a_source);
  }
}

void cdr_layout::set_source(const Real a_source){
  CH_TIME("cdr_layout::set_source(constant)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_source(constant)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_source(a_source);
  }
}

void cdr_layout::set_ebflux(const EBAMRIVData& a_ebflux){
  CH_TIME("cdr_layout::set_source(ebamriv)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_source(ebamriv)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_ebflux(a_ebflux);
  }
}

void cdr_layout::set_ebflux(const Real a_ebflux){
  CH_TIME("cdr_layout::set_source(constant)");
  if(m_verbosity > 5){
    pout() << "cdr_layout::set_source(constant)" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->set_ebflux(a_ebflux);
  }
}

void cdr_layout::write_plot_file(){
  CH_TIME("cdr_layout::write_plot_file");
  if(m_verbosity > 5){
    pout() << "cdr_layout::write_plot_file" << endl;
  }

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    solver->write_plot_file();
  }
}

Real cdr_layout::compute_cfl_dt(){
  CH_TIME("cdr_layout::compute_cfl_dt");
  if(m_verbosity > 5){
    pout() << "cdr_layout::compute_cfl_dt" << endl;
  }

  Real dt = 1.E99;
  
  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    const Real this_dt = solver->compute_cfl_dt();

    dt = Min(dt, this_dt);
  }

  return dt;
}

Real cdr_layout::compute_diffusive_dt(){
  CH_TIME("cdr_layout::compute_diffusive_dt");
  if(m_verbosity > 5){
    pout() << "cdr_layout::compute_diffusive_dt" << endl;
  }

  Real dt = 1.E99;
  
  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    const Real this_dt = solver->compute_diffusive_dt();

    dt = Min(dt, this_dt);
  }

  return dt;
}

Real cdr_layout::compute_source_dt(const Real a_tolerance, const bool a_elec_only){
  CH_TIME("cdr_layout::compute_source_dt");
  if(m_verbosity > 5){
    pout() << "cdr_layout::compute_source_dt" << endl;
  }

  const int comp = 0;
  
  Real max = 0.0;
  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    Real cur_max, cur_min;
    RefCountedPtr<cdr_solver>& solver = solver_it();
    data_ops::get_max_min(cur_max, cur_min, solver->get_state(), comp);
    max = Max(max, cur_max);
    if(a_elec_only){
      break;
    }
  }

  Real dt = 1.E99;
  
  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    const Real this_dt = solver->compute_source_dt(max, a_tolerance);

    dt = Min(dt, this_dt);
    if(a_elec_only){
      break;
    }
  }

  return dt;
}

Real cdr_layout::compute_total_charge(){
  CH_TIME("cdr_layout::compute_total_charge");
  if(m_verbosity > 5){
    pout() << "cdr_layout::compute_total_charge" << endl;
  }

  Real Q = 0.;
  
  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    const Real N = solver->compute_mass();

    Q += N*units::s_Qe;
  }

  return Q;
}

Vector<Real> cdr_layout::compute_mass(){
  CH_TIME("cdr_layout::compute_mass");
  if(m_verbosity > 5){
    pout() << "cdr_layout::compute_mass" << endl;
  }

  Vector<Real> mass;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    mass.push_back(solver->compute_mass());
  }

  return mass;
}

Vector<Real> cdr_layout::compute_charge(){
  CH_TIME("cdr_layout::compute_charge");
  if(m_verbosity > 5){
    pout() << "cdr_layout::compute_charge" << endl;
  }

  Vector<Real> charge;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    charge.push_back(solver->compute_charge()*units::s_Qe);
  }

  return charge;
}

Vector<std::string> cdr_layout::get_names() {
  CH_TIME("cdr_layout::get_names");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_names" << endl;
  }

  Vector<std::string> names;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    names.push_back(solver->get_name());
  }

  return names;
}

Vector<RefCountedPtr<cdr_solver> >& cdr_layout::get_solvers(){
  return m_solvers;
}

Vector<RefCountedPtr<cdr_species> >& cdr_layout::get_species(){
  return m_species;
}

Vector<EBAMRCellData*> cdr_layout::get_states(){
  CH_TIME("cdr_layout::get_states");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_states" << endl;
  }

  Vector<EBAMRCellData*> states;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    states.push_back(&(solver->get_state()));
  }

  return states;
}

Vector<EBAMRCellData*> cdr_layout::get_sources(){
  CH_TIME("cdr_layout::get_sources");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_sources" << endl;
  }

  Vector<EBAMRCellData*> sources;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    sources.push_back(&(solver->get_source()));
  }

  return sources;
}

Vector<EBAMRCellData*> cdr_layout::get_velocities(){
  CH_TIME("cdr_layout::get_velocities");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_velocities" << endl;
  }

  Vector<EBAMRCellData*> velocities;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    velocities.push_back(&(solver->get_velo_cell()));
  }

  return velocities;
}

Vector<EBAMRFluxData*> cdr_layout::get_diffco_face(){
  CH_TIME("cdr_layout::get_diffco_face");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_diffco_face" << endl;
  }

  Vector<EBAMRFluxData*> diffco;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    diffco.push_back(&(solver->get_diffco_face()));
  }

  return diffco;
}

Vector<EBAMRIVData*> cdr_layout::get_diffco_eb(){
  CH_TIME("cdr_layout::get_diffco_eb");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_diffco_eb" << endl;
  }

  Vector<EBAMRIVData*> diffco_eb;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    diffco_eb.push_back(&(solver->get_diffco_eb()));
  }

  return diffco_eb;
}

Vector<EBAMRIVData*> cdr_layout::get_ebflux(){
  CH_TIME("cdr_layout::get_ebflux");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_ebflux" << endl;
  }

  Vector<EBAMRIVData*> ebflux;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    ebflux.push_back(&(solver->get_ebflux()));
  }

  return ebflux;
}

Vector<EBAMRIFData*> cdr_layout::get_domainflux(){
  CH_TIME("cdr_layout::get_domainflux");
  if(m_verbosity > 5){
    pout() << "cdr_layout::get_domainflux" << endl;
  }

  Vector<EBAMRIFData*> domainflux;

  for (cdr_iterator solver_it(*this); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    domainflux.push_back(&(solver->get_domainflux()));
  }

  return domainflux;
}


bool cdr_layout::get_mass_redist(){
  return m_mass_redist;
}