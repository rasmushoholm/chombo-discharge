/*!
  @file rk2.cpp
  @brief Implementation of rk2.H
  @author Robert Marskar
  @date   Feb. 2018
*/

#include "rk2.H"
#include "rk2_storage.H"
#include "cdr_iterator.H"
#include "rte_iterator.H"
#include "data_ops.H"
#include "units.H"

#include <ParmParse.H>

typedef rk2::cdr_storage     cdr_storage;
typedef rk2::poisson_storage poisson_storage;
typedef rk2::rte_storage     rte_storage;
typedef rk2::sigma_storage   sigma_storage;

rk2::rk2() : time_stepper() {
  m_alpha = 1.0;

  {
    ParmParse pp("rk2");
    pp.query("alpha", m_alpha);
  }
}

rk2::~rk2(){

}

RefCountedPtr<cdr_storage>& rk2::get_cdr_storage(const cdr_iterator& a_solverit){
  return m_cdr_scratch[a_solverit.get_solver()];
}

RefCountedPtr<rte_storage>& rk2::get_rte_storage(const rte_iterator& a_solverit){
  return m_rte_scratch[a_solverit.get_solver()];
}

void rk2::allocate_cdr_storage(){

  const int ncomp       = 1;
  const int num_species = m_plaskin->get_num_species();
  m_cdr_scratch.resize(num_species);
  
  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    const int idx = solver_it.get_solver();
    m_cdr_scratch[idx] = RefCountedPtr<cdr_storage> (new cdr_storage(m_amr, m_cdr->get_phase(), ncomp));
    m_cdr_scratch[idx]->allocate_storage();
  }
}

void rk2::allocate_poisson_storage(){
  const int ncomp = 1;
  m_poisson_scratch = RefCountedPtr<poisson_storage> (new poisson_storage(m_amr, m_cdr->get_phase(), ncomp));
  m_poisson_scratch->allocate_storage();
}

void rk2::allocate_rte_storage(){
  const int ncomp       = 1;
  const int num_photons = m_plaskin->get_num_photons();
  m_rte_scratch.resize(num_photons);
  
  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    const int idx = solver_it.get_solver();
    m_rte_scratch[idx] = RefCountedPtr<rte_storage> (new rte_storage(m_amr, m_rte->get_phase(), ncomp));
    m_rte_scratch[idx]->allocate_storage();
  }
}

void rk2::allocate_sigma_storage(){
  const int ncomp = 1;
  m_sigma_scratch = RefCountedPtr<sigma_storage> (new sigma_storage(m_amr, m_cdr->get_phase(), ncomp));
  m_sigma_scratch->allocate_storage();
}

void rk2::regrid_internals(){
  CH_TIME("rk2::regrid_internals");
  if(m_verbosity > 5){
    pout() << "rk2::regrid_internals" << endl;
  }
  
  this->allocate_cdr_storage();
  this->allocate_poisson_storage();
  this->allocate_rte_storage();
  this->allocate_sigma_storage();
}

Real rk2::advance(const Real a_dt){
  CH_TIME("rk2::advance");
  if(m_verbosity > 2){
    pout() << "rk2::advance" << endl;
  }

  // Prepare for k1 advance
  this->compute_E_at_start_of_time_step();
  this->compute_cdr_velo_at_start_of_time_step();
  this->compute_cdr_eb_states_at_start_of_time_step();
  this->compute_cdr_diffco_at_start_of_time_step();
  this->compute_cdr_sources_at_start_of_time_step();
  this->compute_cdr_fluxes_at_start_of_time_step();
  this->compute_sigma_flux_at_start_of_time_step();

#if 0
  MayDay::Warning("rk2::advance - debug mode");
  this->solver_dump();
  MayDay::Abort("rk2::advance - stop");
#endif

  // Do k1 advance
  this->advance_cdr_k1(a_dt);
  this->advance_sigma_k1(a_dt);
  this->solve_poisson_k1();
  this->compute_E_after_k1();
  if(m_rte->is_stationary()){
    this->advance_rte_k1_stationary(a_dt);
  }
  else{
    this->advance_rte_k1_transient(a_dt);
  }

  // Recompute things in order to do k2 advance
  this->compute_cdr_eb_states_after_k1();
  this->compute_cdr_velo_after_k1(a_dt);
  this->compute_cdr_diffco_after_k1(a_dt);
  this->compute_cdr_sources_after_k1(a_dt);
  this->compute_cdr_fluxes_after_k1(a_dt);
  this->compute_sigma_flux_after_k1();

  
  // Do k2 advance
  this->advance_cdr_k2(a_dt);
  this->advance_sigma_k2(a_dt);
  this->solve_poisson_k2();
  this->compute_E_after_k2();
  if(m_rte->is_stationary()){
    this->advance_rte_k2_stationary(a_dt);
  }
  else{
    this->advance_rte_k2_transient(a_dt);
  }

  return a_dt;
}

void rk2::compute_E_at_start_of_time_step(){
  CH_TIME("rk2::compute_E_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_E_at_start_of_time_step" << endl;
  }

  EBAMRCellData& E_cell = m_poisson_scratch->get_E_cell();
  EBAMRFluxData& E_face = m_poisson_scratch->get_E_face();
  EBAMRIVData&   E_eb   = m_poisson_scratch->get_E_eb();

  const MFAMRCellData& phi = m_poisson->get_state();
  
  this->compute_E(E_cell, m_cdr->get_phase(), phi);     // Compute cell-centered field
  this->compute_E(E_face, m_cdr->get_phase(), E_cell);  // Compute face-centered field
  this->compute_E(E_eb,   m_cdr->get_phase(), E_cell);  // EB-centered field
}

void rk2::compute_cdr_velo_at_start_of_time_step(){
  CH_TIME("rk2::compute_cdr_velo_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_velo_at_start_of_time_step" << endl;
  }


  Vector<EBAMRCellData*> states     = m_cdr->get_states();
  Vector<EBAMRCellData*> velocities = m_cdr->get_velocities();
  this->compute_cdr_velocities(velocities, states, m_poisson_scratch->get_E_cell(), m_time);
}

void rk2::compute_cdr_eb_states_at_start_of_time_step(){
  CH_TIME("rk2::compute_cdr_eb_states_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_eb_states_at_start_of_time_step" << endl;
  }

  const irreg_amr_stencil<eb_centroid_interp>& stencil = m_amr->get_eb_centroid_interp_stencils(m_cdr->get_phase());
  for (cdr_iterator solver_it = m_cdr->iterator(); solver_it.ok(); ++solver_it){
    const RefCountedPtr<cdr_solver>& solver = solver_it();
    const EBAMRCellData& state              = solver->get_state();

    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    EBAMRIVData& cdr_eb = storage->get_eb_state();

    stencil.apply(cdr_eb, state);
  }

}

void rk2::compute_cdr_diffco_at_start_of_time_step(){
  CH_TIME("rk2::compute_cdr_diffco_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_diffco_at_start_of_time_step" << endl;
  }

  const int num_species = m_plaskin->get_num_species();

  Vector<EBAMRCellData*> cdr_states  = m_cdr->get_states();
  Vector<EBAMRFluxData*> diffco_face = m_cdr->get_diffco_face();
  Vector<EBAMRIVData*> diffco_eb     = m_cdr->get_diffco_eb();

  const EBAMRCellData& E_cell = m_poisson_scratch->get_E_cell();
  const EBAMRIVData& E_eb     = m_poisson_scratch->get_E_eb();

  // Get extrapolated states
  Vector<EBAMRIVData*> eb_states(num_species);
  for (cdr_iterator solver_it = m_cdr->iterator(); solver_it.ok(); ++solver_it){
    const int idx = solver_it.get_solver();
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    eb_states[idx] = &(storage->get_eb_state());
  }
  
  this->compute_cdr_diffco_face(diffco_face, cdr_states, E_cell, m_time);
  this->compute_cdr_diffco_eb(diffco_eb,     eb_states,  E_eb,   m_time);
}

void rk2::compute_cdr_sources_at_start_of_time_step(){
  CH_TIME("rk2::compute_cdr_sources_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_sources_at_start_of_time_step" << endl;
  }
  
  Vector<EBAMRCellData*> cdr_sources = m_cdr->get_sources();
  Vector<EBAMRCellData*> cdr_states  = m_cdr->get_states();
  Vector<EBAMRCellData*> rte_states  = m_rte->get_states();
  EBAMRCellData& E                   = m_poisson_scratch->get_E_cell();

  this->compute_cdr_sources(cdr_sources, cdr_states, rte_states, E, m_time, centering::cell_center);
}

void rk2::compute_cdr_fluxes_at_start_of_time_step(){
  CH_TIME("rk2::compute_cdr_fluxes_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_fluxes_at_start_of_time_step" << endl;
  }
  
  Vector<EBAMRIVData*> cdr_fluxes;
  Vector<EBAMRIVData*> extrap_cdr_fluxes;
  Vector<EBAMRIVData*> extrap_cdr_densities;
  Vector<EBAMRIVData*> extrap_cdr_velocities;
  Vector<EBAMRIVData*> extrap_rte_fluxes;

  cdr_fluxes = m_cdr->get_ebflux();

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);

    EBAMRIVData& dens_eb = storage->get_eb_state();
    EBAMRIVData& velo_eb = storage->get_eb_velo();
    EBAMRIVData& flux_eb = storage->get_eb_flux();

    extrap_cdr_densities.push_back(&dens_eb);
    extrap_cdr_velocities.push_back(&velo_eb);
    extrap_cdr_fluxes.push_back(&flux_eb);
  }

  // Extrapolate densities, velocities, and fluxes
  Vector<EBAMRCellData*> cdr_densities = m_cdr->get_states();
  Vector<EBAMRCellData*> cdr_velocities = m_cdr->get_velocities();
  this->compute_extrapolated_fluxes(extrap_cdr_fluxes, cdr_densities, cdr_velocities, m_cdr->get_phase());
  this->extrapolate_to_eb(extrap_cdr_densities,  m_cdr->get_phase(), cdr_densities); // Already been done, no?
  this->extrapolate_to_eb(extrap_cdr_velocities, m_cdr->get_phase(), cdr_velocities);

  // Compute RTE flux on the boundary
  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    RefCountedPtr<rte_solver>& solver   = solver_it();
    RefCountedPtr<rte_storage>& storage = this->get_rte_storage(solver_it);

    EBAMRIVData& flux_eb = storage->get_eb_flux();
    solver->compute_boundary_flux(flux_eb, solver->get_state());
    extrap_rte_fluxes.push_back(&flux_eb);
  }

  const EBAMRIVData& E = m_poisson_scratch->get_E_eb();

  this->compute_cdr_fluxes(cdr_fluxes,
			   extrap_cdr_fluxes,
			   extrap_cdr_densities,
			   extrap_cdr_velocities,
			   extrap_rte_fluxes,
			   E,
			   m_time);
}

void rk2::compute_sigma_flux_at_start_of_time_step(){
  CH_TIME("rk2::compute_sigma_flux_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_sigma_flux_at_start_of_time_step" << endl;
  }

  EBAMRIVData& flux = m_sigma->get_flux();
  data_ops::set_value(flux, 0.0);

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    const RefCountedPtr<cdr_solver>& solver = solver_it();
    const RefCountedPtr<species>& spec      = solver_it.get_species();
    const EBAMRIVData& solver_flux          = solver->get_ebflux();

    data_ops::incr(flux, solver_flux, spec->get_charge()*units::s_Qe);
  }

  m_sigma->reset_cells(flux);
}

void rk2::advance_cdr_k1(const Real a_dt){
  CH_TIME("rk2::advance_cdr_k1");
  if(m_verbosity > 5){
    pout() << "rk2::advance_cdr_k1" << endl;
  }

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver   = solver_it();
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);

    EBAMRCellData& k1  = storage->get_k1();
    EBAMRCellData& phi = storage->get_phi();

    const EBAMRCellData& state = solver->get_state();

    data_ops::set_value(k1, 0.0);
    solver->compute_rhs(k1, state, a_dt);

    data_ops::set_value(phi, 0.0);
    data_ops::incr(phi, state, 1.0);
    data_ops::incr(phi, k1,    m_alpha*a_dt);

    m_amr->average_down(phi, m_cdr->get_phase());
    m_amr->interp_ghost(phi, m_cdr->get_phase());

    data_ops::floor(phi, 0.0);
  }
}

void rk2::advance_sigma_k1(const Real a_dt){
  CH_TIME("rk2::advance_sigma_k1");
  if(m_verbosity > 5){
    pout() << "rk2::advance_sigma_k1" << endl;
  }

  const EBAMRIVData& state = m_sigma->get_state();
  
  EBAMRIVData& k1  = m_sigma_scratch->get_k1();
  EBAMRIVData& phi = m_sigma_scratch->get_phi();
  m_sigma->compute_rhs(k1);
  data_ops::set_value(phi,   0.0);
  data_ops::incr(phi, state, 1.0);
  data_ops::incr(phi, k1,    m_alpha*a_dt);

  m_amr->average_down(phi, m_cdr->get_phase());
  
  m_sigma->reset_cells(k1);
  m_sigma->reset_cells(phi);
}

void rk2::solve_poisson_k1(){
  CH_TIME("rk2::solve_poisson_k1");
  if(m_verbosity > 5){
    pout() << "rk2::solve_poisson_k1" << endl;
  }

  MFAMRCellData& scratch_pot = m_poisson_scratch->get_phi();
  EBAMRIVData& sigma         = m_sigma_scratch->get_phi();
  Vector<EBAMRCellData*> cdr_densities;
  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    cdr_densities.push_back(&(storage->get_phi()));
  }

  data_ops::set_value(scratch_pot, 0.0);
  data_ops::incr(scratch_pot, m_poisson->get_state(), 1.0);

  if((m_step + 1) % m_fast_poisson == 0){
    bool converged = this->solve_poisson(scratch_pot, m_poisson->get_source(), cdr_densities, sigma, centering::cell_center);
    if(!converged){
      pout() << "rk2::solve_poisson_k1 - solver did not converge at step = " << m_step << endl;
    }
  }
}

void rk2::compute_E_after_k1(){
  CH_TIME("rk2::compute_E_after_k1");
  if(m_verbosity > 5){
    pout() << "rk2::compute_E_after_k1" << endl;
  }
  
  EBAMRCellData& E_cell = m_poisson_scratch->get_E_cell();
  EBAMRFluxData& E_face = m_poisson_scratch->get_E_face();
  EBAMRIVData&   E_eb   = m_poisson_scratch->get_E_eb();

  const MFAMRCellData& phi = m_poisson_scratch->get_phi();
  
  this->compute_E(E_cell, m_cdr->get_phase(), phi);     // Compute cell-centered field
  this->compute_E(E_face, m_cdr->get_phase(), E_cell);  // Compute face-centered field
  this->compute_E(E_eb,   m_cdr->get_phase(), E_cell);  // EB-centered field
}

void rk2::advance_rte_k1_stationary(const Real a_dt){
  CH_TIME("rk2::compute_rte_k1_stationary");
  if(m_verbosity > 5){
    pout() << "rk2::compute_k1_stationary" << endl;
  }

  const Real time = m_time + m_alpha*a_dt;

  Vector<EBAMRCellData*> rte_states;
  Vector<EBAMRCellData*> rte_sources;
  Vector<EBAMRCellData*> cdr_states;

  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    RefCountedPtr<rte_solver>& solver   = solver_it();
    RefCountedPtr<rte_storage>& storage = this->get_rte_storage(solver_it);
    
    EBAMRCellData& phi    = storage->get_phi();
    EBAMRCellData& state  = solver->get_state();
    EBAMRCellData& source = solver->get_source();

    data_ops::set_value(phi, 0.0);
    data_ops::incr(phi, state, 1.0);

    rte_states.push_back(&(phi));
    rte_sources.push_back(&(source));
  }

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    cdr_states.push_back(&(storage->get_phi()));
  }

  EBAMRCellData& E = m_poisson_scratch->get_E_cell();

  if((m_step + 1) % m_fast_rte == 0){
    const Real dummy_dt = 0.0;
    this->solve_rte(rte_states, rte_sources, cdr_states, E, time, dummy_dt, centering::cell_center);
  }
}

void rk2::advance_rte_k1_transient(const Real a_dt){
  CH_TIME("rk2::compute_rte_k1_transient");
  if(m_verbosity > 5){
    pout() << "rk2::compute_k1_transient" << endl;
  }

  const Real time = m_time + 0.5*m_alpha*a_dt; // Source terms are now centered on the half time step

  Vector<EBAMRCellData*> rte_states;
  Vector<EBAMRCellData*> rte_sources;
  Vector<EBAMRCellData*> cdr_states;

  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    RefCountedPtr<rte_solver>& solver   = solver_it();
    RefCountedPtr<rte_storage>& storage = this->get_rte_storage(solver_it);
    
    EBAMRCellData& phi    = storage->get_phi();
    EBAMRCellData& state  = solver->get_state();
    EBAMRCellData& source = solver->get_source();

    data_ops::set_value(phi, 0.0);
    data_ops::incr(phi, state, 1.0);
    
    rte_states.push_back(&(phi));
    rte_sources.push_back(&(source));
  }

  // Source term must be centered between time tn and the intermediate time
  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver   = solver_it();
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    
    const EBAMRCellData& state = solver->get_state();
    const EBAMRCellData& phi   = storage->get_phi();

    EBAMRCellData& scratch = storage->get_scratch();

    data_ops::set_value(scratch, 0.0);
    data_ops::incr(scratch, state, 0.5);
    data_ops::incr(scratch, phi,   0.5);

    cdr_states.push_back(&(scratch));
  }


  if((m_step + 1) % m_fast_rte == 0){ // Actual solve
    const MFAMRCellData& state = m_poisson->get_state();
    const MFAMRCellData& phi   = m_poisson_scratch->get_phi();
  
    MFAMRCellData& scratch_phi = m_poisson_scratch->get_scratch_phi();
    EBAMRCellData& scratch_E   = m_poisson_scratch->get_scratch_E();

    data_ops::set_value(scratch_phi, 0.0);
    data_ops::incr(scratch_phi, state, 0.5);
    data_ops::incr(scratch_phi, phi, 0.5);

    m_amr->average_down(scratch_phi);
    m_amr->interp_ghost(scratch_phi);

    this->compute_E(scratch_E, m_cdr->get_phase(), scratch_phi);
  

    this->solve_rte(rte_states, rte_sources, cdr_states, scratch_E, time, m_alpha*a_dt, centering::cell_center);
  }
}

void rk2::compute_cdr_eb_states_after_k1(){
  CH_TIME("rk2::compute_cdr_eb_states_after_k1");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_eb_states_after_k1" << endl;
  }

  const irreg_amr_stencil<eb_centroid_interp>& stencil = m_amr->get_eb_centroid_interp_stencils(m_cdr->get_phase());
  for (cdr_iterator solver_it = m_cdr->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    EBAMRCellData& cdr_state = storage->get_phi();
    EBAMRIVData& cdr_eb      = storage->get_eb_state();

    stencil.apply(cdr_eb, cdr_state);
  }
}

void rk2::compute_cdr_velo_after_k1(const Real a_dt){
  CH_TIME("rk2::compute_cdr_velo_after_k1");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_velo_after_k1";
  }
  
  const int num_species = m_plaskin->get_num_species();
  
  const Real time = m_time + m_alpha*a_dt;
  
  Vector<EBAMRCellData*> states(num_species);
  Vector<EBAMRCellData*> velocities = m_cdr->get_velocities();

  for (cdr_iterator solver_it = m_cdr->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    const int idx = solver_it.get_solver();
    states[idx] = &(storage->get_phi());
  }
  
  this->compute_cdr_velocities(velocities, states, m_poisson_scratch->get_E_cell(), time);
}

void rk2::compute_cdr_diffco_after_k1(const Real a_dt){
  CH_TIME("rk2::compute_cdr_diffco_after_k1");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_diffco_after_k1" << endl;
  }

  const int num_species = m_plaskin->get_num_species();

  const Real time = m_time + m_alpha*a_dt;
  
  Vector<EBAMRFluxData*> diffco_face = m_cdr->get_diffco_face();
  Vector<EBAMRIVData*> diffco_eb     = m_cdr->get_diffco_eb();

  const EBAMRCellData& E_cell = m_poisson_scratch->get_E_cell();
  const EBAMRIVData& E_eb     = m_poisson_scratch->get_E_eb();

  Vector<EBAMRCellData*> states(num_species);
  Vector<EBAMRIVData*> eb_states(num_species);
  for (cdr_iterator solver_it = m_cdr->iterator(); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    const int idx  = solver_it.get_solver();
    states[idx]    = &(storage->get_phi());
    eb_states[idx] = &(storage->get_eb_state());
  }
  
  this->compute_cdr_diffco_face(diffco_face, states,    E_cell, time);
  this->compute_cdr_diffco_eb(diffco_eb,     eb_states, E_eb,   time);
}

void rk2::compute_cdr_sources_after_k1(const Real a_dt){
  CH_TIME("rk2::compute_cdr_sources_after_k1");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_sources_after_k1" << endl;
  }

  const Real time = m_time + m_alpha*a_dt;

  Vector<EBAMRCellData*> cdr_sources;
  Vector<EBAMRCellData*> cdr_states;
  Vector<EBAMRCellData*> rte_states;

  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    RefCountedPtr<rte_storage>& storage = this->get_rte_storage(solver_it);
    rte_states.push_back(&(storage->get_phi()));
  }

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver   = solver_it();
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    
    cdr_states.push_back(&(storage->get_phi()));
    cdr_sources.push_back(&(solver->get_source()));
  }

  EBAMRCellData& E = m_poisson_scratch->get_E_cell();

  this->compute_cdr_sources(cdr_sources, cdr_states, rte_states, E, time, centering::cell_center);
}

void rk2::compute_cdr_fluxes_after_k1(const Real a_dt){
  CH_TIME("rk2::compute_cdr_fluxes_after_k1");
  if(m_verbosity > 5){
    pout() << "rk2::compute_cdr_fluxes_after_k1" << endl;
  }

  const Real time = m_time + m_alpha*a_dt;
  
  Vector<EBAMRIVData*> cdr_fluxes;
  Vector<EBAMRIVData*> extrap_cdr_fluxes;
  Vector<EBAMRIVData*> extrap_cdr_densities;
  Vector<EBAMRIVData*> extrap_cdr_velocities;
  Vector<EBAMRIVData*> extrap_rte_fluxes;
  
  Vector<EBAMRCellData*> cdr_densities;
  Vector<EBAMRCellData*> rte_densities;

  cdr_fluxes = m_cdr->get_ebflux();

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);

    EBAMRCellData& dens  = storage->get_phi();
    EBAMRIVData& dens_eb = storage->get_eb_state();
    EBAMRIVData& velo_eb = storage->get_eb_velo();
    EBAMRIVData& flux_eb = storage->get_eb_flux();

    cdr_densities.push_back(&dens);
    extrap_cdr_densities.push_back(&dens_eb);
    extrap_cdr_velocities.push_back(&velo_eb);
    extrap_cdr_fluxes.push_back(&flux_eb);
  }

  // Extrapolate densities, velocities, and fluxes
  Vector<EBAMRCellData*> cdr_velocities = m_cdr->get_velocities();
  this->compute_extrapolated_fluxes(extrap_cdr_fluxes, cdr_densities, cdr_velocities, m_cdr->get_phase());
  this->extrapolate_to_eb(extrap_cdr_densities,  m_cdr->get_phase(), cdr_densities); // This has already been done, no?
  this->extrapolate_to_eb(extrap_cdr_velocities, m_cdr->get_phase(), cdr_velocities);

  // Compute RTE flux on the boundary
  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    RefCountedPtr<rte_solver>& solver   = solver_it();
    RefCountedPtr<rte_storage>& storage = this->get_rte_storage(solver_it);

    EBAMRIVData& flux_eb = storage->get_eb_flux();
    solver->compute_boundary_flux(flux_eb, storage->get_phi());
    extrap_rte_fluxes.push_back(&flux_eb);
  }

  const EBAMRIVData& E = m_poisson_scratch->get_E_eb();

  this->compute_cdr_fluxes(cdr_fluxes,
			   extrap_cdr_fluxes,
			   extrap_cdr_densities,
			   extrap_cdr_velocities,
			   extrap_rte_fluxes,
			   E,
			   time);
}

void rk2::compute_sigma_flux_after_k1(){
  CH_TIME("rk2::compute_sigma_flux_after_k1");
  if(m_verbosity > 5){
    pout() << "rk2::compute_sigma_flux_after_k1" << endl;
  }

  EBAMRIVData& flux = m_sigma->get_flux();
  data_ops::set_value(flux, 0.0);

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    const RefCountedPtr<cdr_solver>& solver = solver_it();
    const RefCountedPtr<species>& spec      = solver_it.get_species();
    const EBAMRIVData& solver_flux          = solver->get_ebflux();

    data_ops::incr(flux, solver_flux, spec->get_charge()*units::s_Qe);
  }

  m_sigma->reset_cells(flux);
}

void rk2::advance_cdr_k2(const Real a_dt){
  CH_TIME("rk2::advance_cdr_k2");
  if(m_verbosity > 5){
    pout() << "rk2::advance_cdr_k2" << endl;
  }

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver   = solver_it();
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);

    EBAMRCellData& state   = solver->get_state();
    EBAMRCellData& k1      = storage->get_k1();
    EBAMRCellData& k2      = storage->get_k2();
    EBAMRCellData& phi     = storage->get_phi();
    EBAMRCellData& scratch = storage->get_scratch();

    solver->compute_rhs(k2, phi, a_dt);

    // For transient RTE solvers I need the source term at half time steps. Since the internal state
    // inside the solver will be overwritten, I take a backup into rk2_storage.scratch
    if(!m_rte->is_stationary()){
      data_ops::set_value(scratch,   0.0);
      data_ops::incr(scratch, state, 1.0);
    }

    
    data_ops::incr(state, k1, a_dt*(1 - 1./(2.*m_alpha)));
    data_ops::incr(state, k2, a_dt*1./(2.*m_alpha));

    m_amr->average_down(state, m_cdr->get_phase());
    m_amr->interp_ghost(state, m_cdr->get_phase());

    data_ops::floor(state, 0.0);
  }
}

void rk2::advance_sigma_k2(const Real a_dt){
  CH_TIME("rk2::advance_sigma_k1");
  if(m_verbosity > 5){
    pout() << "rk2::advance_sigma_k1" << endl;
  }
  
  EBAMRIVData& k1  = m_sigma_scratch->get_k1();
  EBAMRIVData& k2  = m_sigma_scratch->get_k2();
  m_sigma->compute_rhs(k2);

  EBAMRIVData& state = m_sigma->get_state();
  data_ops::incr(state, k1, a_dt*(1 - 1./(2.*m_alpha)));
  data_ops::incr(state, k2, a_dt*1./(2.*m_alpha));

  m_sigma->reset_cells(state);
}

void rk2::solve_poisson_k2(){
  CH_TIME("rk2::solve_poisson_k2");
  if(m_verbosity > 5){
    pout() << "rk2::solve_poisson_k2" << endl;
  }

  // We computed the intermediate potential at time t_k + alpha*dt. Linearly extrapolate that result to the end of
  // the time step. The result for this is y_extrap = y_alpha/alpha - y_0*(1-alpha)/alpha. This (usually) brings the
  // initial guess closer to the true solution.
  MFAMRCellData& pot     = m_poisson->get_state();
  MFAMRCellData& phi     = m_poisson_scratch->get_phi();
  MFAMRCellData& scratch = m_poisson_scratch->get_scratch_phi();

  // For transient RTE solvers I need the source term at half time steps. Since the internal state
  // inside the solver will be overwritten, I take a backup into poisson_storage.scratch_phi
  if(!m_rte->is_stationary()){
    data_ops::set_value(scratch, 0.0);
    data_ops::incr(scratch, pot, 1.0); 
  }

  if((m_step + 1) % m_fast_poisson == 0){
    data_ops::scale(pot, -(1.0 - m_alpha)/m_alpha);
    data_ops::incr(pot, phi, 1./m_alpha);

    const bool converged = this->solve_poisson(pot,
					       m_poisson->get_source(),
					       m_cdr->get_states(),
					       m_sigma->get_state(),
					       centering::cell_center);
    if(!converged){
      pout() << "rk2::solve_poisson_k2 - solver did not converge at step = " << m_step << endl;
    }
  }
  else{
    data_ops::set_value(pot, 0.0);
    data_ops::incr(pot, phi, 1.0);
  }
}

void rk2::compute_E_after_k2(){
  CH_TIME("rk2::compute_E_at_start_of_time_step");
  if(m_verbosity > 5){
    pout() << "rk2::compute_E_at_start_of_time_step" << endl;
  }
  
  EBAMRCellData& E_cell = m_poisson_scratch->get_E_cell();
  EBAMRFluxData& E_face = m_poisson_scratch->get_E_face();
  EBAMRIVData&   E_eb   = m_poisson_scratch->get_E_eb();

  const MFAMRCellData& phi = m_poisson->get_state();
  
  this->compute_E(E_cell, m_cdr->get_phase(), phi);     // Compute cell-centered field
  this->compute_E(E_face, m_cdr->get_phase(), E_cell);  // Compute face-centered field
  this->compute_E(E_eb,   m_cdr->get_phase(), E_cell);  // EB-centered field
}

void rk2::advance_rte_k2_stationary(const Real a_dt){
  CH_TIME("rk2::compute_rte_k2_stationary");
  if(m_verbosity > 5){
    pout() << "rk2::compute_rte_k2_stationary" << endl;
  }

  const Real time = m_time + a_dt; // Source terms centered on the end of the time step

  Vector<EBAMRCellData*> rte_states;
  Vector<EBAMRCellData*> rte_sources;
  Vector<EBAMRCellData*> cdr_states;

  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    RefCountedPtr<rte_solver>& solver   = solver_it();
    RefCountedPtr<rte_storage>& storage = this->get_rte_storage(solver_it);
    
    EBAMRCellData& phi    = storage->get_phi();
    EBAMRCellData& state  = solver->get_state();
    EBAMRCellData& source = solver->get_source();

    
    // We computed the intermediate potential at time t_k + alpha*dt. Linearly extrapolate that result to the end of
    // the time step. The result for this is y_extrap = y_alpha/alpha - y_0*(1-alpha)/alpha. This (usually) brings the
    // initial guess closer to the true solution.
    if((m_step + 1) % m_fast_rte == 0){
      data_ops::scale(state, -(1.0 - m_alpha)/(m_alpha));
      data_ops::incr(state, phi, 1.0/m_alpha);
    }
    else {
      data_ops::set_value(state, 0.0);
      data_ops::incr(state, phi, 1.0);
    }
    
    rte_states.push_back(&(state));
    rte_sources.push_back(&(source));
  }

  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver = solver_it();
    cdr_states.push_back(&(solver->get_state()));
  }

  EBAMRCellData& E = m_poisson_scratch->get_E_cell();

  if((m_step + 1) % m_fast_rte == 0){
    const Real dummy_dt = 0.0;
    this->solve_rte(rte_states, rte_sources, cdr_states, E, time, dummy_dt, centering::cell_center);
  }
}

void rk2::advance_rte_k2_transient(const Real a_dt){
  CH_TIME("rk2::compute_rte_k1_transient");
  if(m_verbosity > 5){
    pout() << "rk2::compute_k1_transient" << endl;
  }

  const Real time = m_time + 0.5*a_dt; // Source terms centered on the half time step

  // If we made it here, the old potential lies in m_poisson_scratch->scratch and the old cdr solutions
  // lie in cdr_storage->m_scratch. The internal state inside the solver is unaffected by anything done previously

  Vector<EBAMRCellData*> rte_states;
  Vector<EBAMRCellData*> rte_sources;
  Vector<EBAMRCellData*> cdr_states;

  for (rte_iterator solver_it(*m_rte); solver_it.ok(); ++solver_it){
    RefCountedPtr<rte_solver>& solver   = solver_it();
    RefCountedPtr<rte_storage>& storage = this->get_rte_storage(solver_it);
    
    EBAMRCellData& state  = solver->get_state();  // This has been unaffected so far because we solved onto rte_storage.phi
    EBAMRCellData& source = solver->get_source(); // in the k1-stage. 

    rte_states.push_back(&(state));
    rte_sources.push_back(&(source));
  }

    
  // Source term must be centered between time tn and the intermediate time
  for (cdr_iterator solver_it(*m_cdr); solver_it.ok(); ++solver_it){
    RefCountedPtr<cdr_solver>& solver   = solver_it();
    RefCountedPtr<cdr_storage>& storage = this->get_cdr_storage(solver_it);
    
    const EBAMRCellData& state   = solver->get_state();
    const EBAMRCellData& scratch = storage->get_scratch();

    EBAMRCellData& phi = storage->get_phi();

    data_ops::set_value(phi, 0.0);
    data_ops::incr(phi, scratch, 0.5);
    data_ops::incr(phi, phi,     0.5);

    cdr_states.push_back(&(phi));
  }
  
  if((m_step + 1) % m_fast_rte == 0){ // Actual solve
    const MFAMRCellData& state       = m_poisson->get_state();
    const MFAMRCellData& scratch_phi = m_poisson_scratch->get_scratch_phi();
    
    MFAMRCellData& phi   = m_poisson_scratch->get_phi();
    EBAMRCellData& scratch_E   = m_poisson_scratch->get_scratch_E();
    
    data_ops::set_value(phi, 0.0);
    data_ops::incr(phi, state, 0.5);
    data_ops::incr(phi, scratch_phi, 0.5);
    
    m_amr->average_down(phi);
    m_amr->interp_ghost(phi);
    
    this->compute_E(scratch_E, m_cdr->get_phase(), phi);
    
    this->solve_rte(rte_states, rte_sources, cdr_states, scratch_E, time, a_dt, centering::cell_center);
  }
}

Real rk2::restrict_dt(){
  return 1.E99;
}
