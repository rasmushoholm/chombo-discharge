/*!
  @file   air3_mc8.H
  @brief  6-species (3/3 charged/excited) and 8-photon model for air
  @author Robert Marskar
  @date   Feb. 2018
*/

#include "air3_mc8.H"
#include "air3_mc8_species.H"
#include "data_ops.H"
#include "units.H" 

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>

#include <ParmParse.H>
#include <PolyGeom.H>

std::string air3_mc8::s_bolsig_mobility = "E/N (Td)	Mobility *N (1/m/V/s)";
std::string air3_mc8::s_bolsig_diffco   = "E/N (Td)	Diffusion coefficient *N (1/m/s)";
std::string air3_mc8::s_bolsig_alpha    = "E/N (Td)	Total ionization freq. /N (m3/s)";
std::string air3_mc8::s_bolsig_eta      = "E/N (Td)	Total attachment freq. /N (m3/s)";

air3_mc8::air3_mc8() {

  instantiate_species();      

  parse_transport_file(); 
  parse_transport();
  parse_gas_params();
  parse_electron_mobility();
  parse_electron_diffco();
  parse_alpha();
  parse_eta();
  parse_photoi();
  parse_see();
  parse_domain_bc();

  init_rng();                 // Initialize random number generators
  
  parse_initial_particles();  // Parse initial particles
}

air3_mc8::~air3_mc8() {

}

void air3_mc8::read_file_entries(lookup_table& a_table, const std::string a_string){
  Real x, y;
  bool read_line = false;
  std::ifstream infile(m_transport_file);
  std::string line;

  while (std::getline(infile, line)){

    // Right trim string
    line.erase(line.find_last_not_of(" \n\r\t")+1);

    if(line == a_string){ // Begin reading
      read_line = true;
    }
    else if(line == "" & read_line){ // Stop reading
      read_line = false;
    }

    if(read_line){
      std::istringstream iss(line);
      if (!(iss >> x >> y)) {
    	continue;
      }
      a_table.add_entry(x, y);
    }
  }
  infile.close();
}

void air3_mc8::parse_transport_file(){
  ParmParse pp("air3_mc8");
  pp.get("transport_file",  m_transport_file);
  pp.get("uniform_tables",  m_uniform_entries);
  std::ifstream infile(m_transport_file);
  if(!infile.good()){
    MayDay::Abort("air3_mc8::parse_transport_file - could not find transport data");
  }
  else{
    infile.close();
  }
}

void air3_mc8::parse_transport(){
  ParmParse pp("air3_mc8");

  std::string str;

  pp.get("mobile_electrons", str);    m_mobile_electrons    = (str == "true") ? true : false;
  pp.get("diffusive_electrons", str); m_diffusive_electrons = (str == "true") ? true : false;
  pp.get("diffusive_ions", str);      m_diffusive_ions      = (str == "true") ? true : false;
  pp.get("mobile_ions", str);         m_mobile_ions         = (str == "true") ? true : false;
  
  pp.get("ion_mobility", m_ion_mobility);

  m_ion_diffusion = m_ion_mobility*(units::s_kb*m_T)/units::s_Qe;
}

void air3_mc8::parse_gas_params(){
  ParmParse pp("air3_mc8");

  // Pressure form input script
  pp.get("pressure",    m_p);
  pp.get("temperature", m_T);
  pp.get("frac_N2",     m_N2frac);
  pp.get("frac_O2",     m_O2frac);

  m_p  *= units::s_atm2pascal;
  m_N = m_p*units::s_Na/(m_T*units::s_R);
}

void air3_mc8::parse_electron_mobility(){
  ParmParse pp("air3_mc8");

  read_file_entries(m_e_mobility, air3_mc8::s_bolsig_mobility);
  m_e_mobility.scale_x(m_N*units::s_Td);
  m_e_mobility.scale_y(1./m_N); 
  m_e_mobility.make_uniform(m_uniform_entries);
}

void air3_mc8::parse_electron_diffco(){
  ParmParse pp("air3_mc8");
  
  read_file_entries(m_e_diffco, air3_mc8::s_bolsig_diffco);
  m_e_diffco.scale_x(m_N*units::s_Td);
  m_e_diffco.scale_y(1./m_N); 
  m_e_diffco.make_uniform(m_uniform_entries);
}

void air3_mc8::parse_alpha(){
  ParmParse pp("air3_mc8");
  read_file_entries(m_e_alpha, air3_mc8::s_bolsig_alpha);
  m_e_alpha.scale_x(m_N*units::s_Td);
  m_e_alpha.scale_y(m_N); 
  m_e_alpha.make_uniform(m_uniform_entries);
}

void air3_mc8::parse_eta(){
  ParmParse pp("air3_mc8");
  read_file_entries(m_e_eta, air3_mc8::s_bolsig_eta);
  m_e_eta.scale_x(m_N*units::s_Td);
  m_e_eta.scale_y(m_N); 
  m_e_eta.make_uniform(m_uniform_entries);
}

void air3_mc8::parse_photoi(){

  ParmParse pp("air3_mc8");

  pp.get("c4v0_exc_eff", m_c4v0_exc_eff);
  pp.get("c4v1_exc_eff", m_c4v1_exc_eff);
  pp.get("b1v1_exc_eff", m_b1v1_exc_eff);

  pp.get("c4v0_X1v0_photoi_eff", m_c4v0_X1v0_photoi_eff);
  pp.get("c4v0_X1v1_photoi_eff", m_c4v0_X1v1_photoi_eff);
  pp.get("c4v1_X1v0_photoi_eff", m_c4v1_X1v0_photoi_eff);
  pp.get("c4v1_X1v1_photoi_eff", m_c4v1_X1v1_photoi_eff);
  pp.get("c4v1_X1v2_photoi_eff", m_c4v1_X1v2_photoi_eff);
  pp.get("c4v1_X1v3_photoi_eff", m_c4v1_X1v3_photoi_eff);
  pp.get("b1v1_X1v0_photoi_eff", m_b1v1_X1v0_photoi_eff);
  pp.get("b1v1_X1v1_photoi_eff", m_b1v1_X1v1_photoi_eff);

  pp.get("quenching_pressure", m_pq);

  m_pq *= units::s_atm2pascal;
}

void air3_mc8::parse_see(){
  ParmParse pp("air3_mc8");
  pp.get("electrode_townsend2",           m_townsend2_electrode);
  pp.get("dielectric_townsend2",          m_townsend2_dielectric);
  pp.get("electrode_quantum_efficiency",  m_electrode_quantum_efficiency);
  pp.get("dielectric_quantum_efficiency", m_dielectric_quantum_efficiency);
}

void air3_mc8::init_rng(){
  ParmParse pp("air3_mc8");
  pp.get("rng_seed", m_rng_seed);

  pp.get("poiss_exp_swap", m_poiss_exp_swap);
  if(m_rng_seed < 0) {
    m_rng_seed = std::chrono::system_clock::now().time_since_epoch().count();
  }
  m_udist01 = new std::uniform_real_distribution<Real>(0.0, 1.0);
  m_udist11 = new std::uniform_real_distribution<Real>(-1.0, 1.0);
  m_rng     = new std::mt19937_64(m_rng_seed);
}

void air3_mc8::instantiate_species(){
  m_num_species = 3;
  m_num_photons = 8;

  m_elec_idx = 0;
  m_plus_idx = 1;
  m_minu_idx = 2;

  m_c4v0_X1v0_idx = 0;
  m_c4v0_X1v1_idx = 1;
  m_c4v1_X1v0_idx = 2;
  m_c4v1_X1v1_idx = 3;
  m_c4v1_X1v2_idx = 4;
  m_c4v1_X1v3_idx = 5;
  m_b1v1_X1v0_idx = 6;
  m_b1v1_X1v1_idx = 7;

  m_species.resize(m_num_species);
  m_species[m_elec_idx]  = RefCountedPtr<species>      (new air3_mc8::electron());
  m_species[m_plus_idx]  = RefCountedPtr<species>      (new air3_mc8::M_plus());
  m_species[m_minu_idx]  = RefCountedPtr<species>      (new air3_mc8::M_minus());

  m_photons.resize(m_num_photons);
  m_photons[m_c4v0_X1v0_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_c4v0_X1v0());
  m_photons[m_c4v0_X1v1_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_c4v0_X1v1());
  m_photons[m_c4v1_X1v0_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_c4v1_X1v0());
  m_photons[m_c4v1_X1v1_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_c4v1_X1v1());
  m_photons[m_c4v1_X1v2_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_c4v1_X1v2());
  m_photons[m_c4v1_X1v3_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_c4v1_X1v3());
  m_photons[m_b1v1_X1v0_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_b1v1_X1v0());
  m_photons[m_b1v1_X1v1_idx] = RefCountedPtr<photon_group> (new air3_mc8::phot_b1v1_X1v1());
}

void air3_mc8::parse_initial_particles(){



  // Get some parameters from the input script
  Vector<Real> vec;
  Real weight, uniform_pairs, rad_pairs, gaussian_pairs;
  RealVect center_pairs;

  ParmParse pp("air3_mc8");
  pp.get("particle_weight",            weight);
  pp.get("uniform_pairs",              uniform_pairs);
  pp.get("gaussian_pairs",             gaussian_pairs);
  pp.get("gaussian_pairs_radius",      rad_pairs);
  pp.getarr("gaussian_pairs_center",   vec, 0, SpaceDim); center_pairs   = RealVect(D_DECL(vec[0], vec[1], vec[2]));
  
  List<Particle> electron_ion_pairs;

  // Add various types of particles
  add_uniform_particles(electron_ion_pairs,  round(uniform_pairs),    weight);
  add_gaussian_particles(electron_ion_pairs, round(gaussian_pairs),   weight, rad_pairs,   center_pairs);
  
  // Set initial particles
  m_species[m_elec_idx]->get_initial_particles() = electron_ion_pairs;
  m_species[m_plus_idx]->get_initial_particles() = electron_ion_pairs;

  // Set the deposition scheme
  std::string str;
  InterpType deposition;
  pp.get("particle_deposition", str);
  if(str == "cic"){
    deposition = InterpType::CIC;
  }
  else if(str == "ngp"){
    deposition = InterpType::NGP;
  }
  else if(str == "tsc"){
    deposition = InterpType::TSC;
  }
  else{
    MayDay::Abort("air3_mc8::parse_initial_particles - unknown deposition type requested");
  }
  
  for (int i = 0; i < m_num_species; i++){
    m_species[i]->get_deposition() = deposition;
  }
}

void air3_mc8::add_uniform_particles(List<Particle>& a_particles, const int a_num, const Real a_weight){

  // Get Lo/Hi sides of domain
  RealVect lo, hi;
  Vector<Real> vec(SpaceDim);
  ParmParse pp("physical_domain");
  pp.getarr("lo_corner", vec, 0, SpaceDim); lo = RealVect(D_DECL(vec[0], vec[1], vec[2]));
  pp.getarr("hi_corner", vec, 0, SpaceDim); hi = RealVect(D_DECL(vec[0], vec[1], vec[2]));

  auto rngX = std::uniform_real_distribution<Real>(lo[0], hi[0]);
  auto rngY = std::uniform_real_distribution<Real>(lo[1], hi[1]);
#if CH_SPACEDIM==3
  auto rngZ = std::uniform_real_distribution<Real>(lo[2], hi[2]);
#endif

  for (int i = 0; i < a_num; i++){
    const Real x = rngX(*m_rng);
    const Real y = rngX(*m_rng);
#if CH_SPACEDIM==3
    const Real z = rngZ(*m_rng);
#endif
    RealVect pos = RealVect(D_DECL(x, y, z));
    a_particles.add(Particle(a_weight, pos));
  }
}

void air3_mc8::add_gaussian_particles(List<Particle>& a_particles,
				      const int       a_num,
				      const Real      a_weight,
				      const Real      a_rad,
				      const RealVect  a_center){
  m_gauss = std::normal_distribution<Real>(0., a_rad);

  for (int i = 0; i < a_num; i++){
    RealVect pos = a_center + random_gaussian();
    a_particles.add(Particle(a_weight, pos));
  }
}

RealVect air3_mc8::random_gaussian(){

  const Real rad = m_gauss(*m_rng);
  return rad*random_direction();
}

RealVect air3_mc8::random_direction(){
#if CH_SPACEDIM == 2
  return random_direction2D();
#else
  return random_direction3D();
#endif
}

#if CH_SPACEDIM == 2
RealVect air3_mc8::random_direction2D(){
  const Real EPS = 1.E-8;
  Real x1 = 2.0;
  Real x2 = 2.0;
  Real r  = x1*x1 + x2*x2;
  while(r >= 1.0 || r < EPS){
    x1 = (*m_udist11)(*m_rng);
    x2 = (*m_udist11)(*m_rng);
    r  = x1*x1 + x2*x2;
  }

  return RealVect(x1,x2)/sqrt(r);
}
#endif

#if CH_SPACEDIM==3
RealVect air3_mc8::random_direction3D(){
  const Real EPS = 1.E-8;
  Real x1 = 2.0;
  Real x2 = 2.0;
  Real r  = x1*x1 + x2*x2;
  while(r >= 1.0 || r < EPS){
    x1 = (*m_udist11)(*m_rng);
    x2 = (*m_udist11)(*m_rng);
    r  = x1*x1 + x2*x2;
  }

  const Real x = 2*x1*sqrt(1-r);
  const Real y = 2*x2*sqrt(1-r);
  const Real z = 1 - 2*r;

  return RealVect(x,y,z);
}
#endif

void air3_mc8::parse_domain_bc(){

  ParmParse pp("air3_mc8");
  std::string str;

  m_wallbc.resize(2*SpaceDim, 0); 
  for (int dir = 0; dir < SpaceDim; dir++){
    for (SideIterator sit; sit.ok(); ++sit){
      const Side::LoHiSide side = sit();
	
      std::string str_dir;
      if(dir == 0){
	str_dir = "x";
      }
      else if(dir == 1){
	str_dir = "y";
      }
      else if(dir == 2){
	str_dir = "z";
      }

      // Check for wall BCs
      if(side == Side::Lo){
	std::string type;
	std::string bc_string = "domain_bc_" + str_dir + "_lo";
	if(pp.contains(bc_string.c_str())){
	  pp.get(bc_string.c_str(), type);
	  const int idx = 2*dir;
	  if(type == "wall"){
	    m_wallbc[idx] = 1;
	  }
	}
      }
      else if(side == Side::Hi){
	std::string type;
	std::string bc_string = "domain_bc_" + str_dir + "_hi";
	if(pp.contains(bc_string.c_str())){
	  pp.get(bc_string.c_str(), type);
	  const int idx = 2*dir + 1;
	  if(type == "wall"){
	    m_wallbc[idx] = 1;
	  }
	}
      }
    }
  }
}

void air3_mc8::advance_reaction_network(Vector<Real>&          a_particle_sources,
					Vector<Real>&          a_photon_sources,
					const Vector<Real>     a_particle_densities,
					const Vector<RealVect> a_particle_gradients,
					const Vector<Real>     a_photon_densities,
					const RealVect         a_E,
					const RealVect         a_pos,
					const Real             a_dx,
					const Real             a_dt,
					const Real             a_time,
					const Real             a_kappa) const{
  // R1: e + M -> e + e + M+  alpha*Xe
  // R2: e + M -> M-+         eta*Xe
  // R3: e + M -> c4v0        alpha*Xe*exc_eff(c4v0)
  // R4: e + M -> c4v1        alpha*Xe*exc_eff(c4v1)
  // R5: e + M -> b1v1        alpha*Xe*exc_eff(b1v1)
  const Real volume = pow(a_dx, SpaceDim);

  // Ionization and attachment coefficients
  const Real alpha  = m_e_alpha.get_entry(a_E.vectorLength());
  const Real eta    = m_e_eta.get_entry(a_E.vectorLength());
  const Real ve     = (a_E*m_e_mobility.get_entry(a_E.vectorLength())).vectorLength();

  const Real R1 = alpha*a_particle_densities[m_elec_idx];
  const Real R2 = eta*a_particle_densities[m_elec_idx];

  Real& Se = a_particle_sources[m_elec_idx];
  Real& Sp = a_particle_sources[m_plus_idx];
  Real& Sm = a_particle_sources[m_minu_idx];

  Se = 0.0;
  Sp = 0.0;
  Sm = 0.0;

  // e + M => e + e + M+
  Se += R1;
  Sp += R1;

  // e + M => M-
  Se -= R2;
  Sm += R2;

  // Photoionization, M + y => e + M+
  for (int i = 0; i < a_photon_densities.size(); i++){
    Se += a_photon_densities[i]/a_dt;
    Sp += a_photon_densities[i]/a_dt;
  }

  // Propensity functions for photon emission
  const Real quench         = m_pq/(m_pq+m_p);
  const Real prop_c4v0_X1v0 = quench*m_c4v0_X1v0_photoi_eff*m_c4v0_exc_eff*R1*volume;
  const Real prop_c4v0_X1v1 = quench*m_c4v0_X1v1_photoi_eff*m_c4v0_exc_eff*R1*volume;
  const Real prop_c4v1_X1v0 = quench*m_c4v1_X1v0_photoi_eff*m_c4v1_exc_eff*R1*volume;
  const Real prop_c4v1_X1v1 = quench*m_c4v1_X1v1_photoi_eff*m_c4v1_exc_eff*R1*volume;
  const Real prop_c4v1_X1v2 = quench*m_c4v1_X1v2_photoi_eff*m_c4v1_exc_eff*R1*volume;
  const Real prop_c4v1_X1v3 = quench*m_c4v1_X1v3_photoi_eff*m_c4v1_exc_eff*R1*volume;
  const Real prop_b1v1_X1v0 = quench*m_b1v1_X1v0_photoi_eff*m_b1v1_exc_eff*R1*volume;
  const Real prop_b1v1_X1v1 = quench*m_b1v1_X1v1_photoi_eff*m_b1v1_exc_eff*R1*volume;
  
  a_photon_sources[m_c4v0_X1v0_idx] = 1.0*poisson_reaction(prop_c4v0_X1v0, a_dt);
  a_photon_sources[m_c4v0_X1v1_idx] = 1.0*poisson_reaction(prop_c4v0_X1v1, a_dt);
  a_photon_sources[m_c4v1_X1v0_idx] = 1.0*poisson_reaction(prop_c4v1_X1v0, a_dt);
  a_photon_sources[m_c4v1_X1v1_idx] = 1.0*poisson_reaction(prop_c4v1_X1v1, a_dt);
  a_photon_sources[m_c4v1_X1v2_idx] = 1.0*poisson_reaction(prop_c4v1_X1v2, a_dt);
  a_photon_sources[m_c4v1_X1v3_idx] = 1.0*poisson_reaction(prop_c4v1_X1v3, a_dt);
  a_photon_sources[m_b1v1_X1v0_idx] = 1.0*poisson_reaction(prop_b1v1_X1v0, a_dt);
  a_photon_sources[m_b1v1_X1v1_idx] = 1.0*poisson_reaction(prop_b1v1_X1v1, a_dt);

  return;
}

int air3_mc8::poisson_reaction(const Real a_propensity, const Real a_dt) const{
  int value = 0;
  const Real mean = a_propensity*a_dt;

  if(mean < m_poiss_exp_swap){
    std::poisson_distribution<int> dist(mean);
    value = dist(*m_rng);
  }
  else{
    std::normal_distribution<double> dist(mean, sqrt(mean));
    value = dist(*m_rng);
  }

  return value;
}


Vector<Real> air3_mc8::compute_cdr_diffusion_coefficients(const Real         a_time,
							  const RealVect     a_pos,
							  const RealVect     a_E,
							  const Vector<Real> a_cdr_densities) const {

  Vector<Real> dco(m_num_species, 0.0);
  dco[m_elec_idx] = m_e_diffco.get_entry(a_E.vectorLength());
  dco[m_plus_idx] = m_ion_diffusion;
  dco[m_minu_idx] = m_ion_diffusion;
  
  return dco;

}
  
Vector<RealVect> air3_mc8::compute_cdr_velocities(const Real         a_time,
						  const RealVect     a_pos,
						  const RealVect     a_E,
						  const Vector<Real> a_cdr_densities) const{
  Vector<RealVect> vel(m_num_species, RealVect::Zero);

  vel[m_elec_idx] = -a_E*m_e_mobility.get_entry(a_E.vectorLength());
  vel[m_plus_idx] =  a_E*m_ion_mobility;
  vel[m_minu_idx] = -a_E*m_ion_mobility;
  
  return vel;
}
  
Vector<Real> air3_mc8::compute_cdr_domain_fluxes(const Real           a_time,
						 const RealVect       a_pos,
						 const int            a_dir,
						 const Side::LoHiSide a_side,
						 const RealVect       a_E,
						 const Vector<Real>   a_cdr_densities,
						 const Vector<Real>   a_cdr_velocities,
						 const Vector<Real>   a_cdr_gradients,
						 const Vector<Real>   a_rte_fluxes,
						 const Vector<Real>   a_extrap_cdr_fluxes) const{
  Vector<Real> fluxes(m_num_species, 0.0);

  int idx, sgn;
  if(a_side == Side::Lo){
    sgn = -1;
    idx = 2*a_dir;
  }
  else{
    sgn = 1;
    idx = 2*a_dir + 1;
  }

  if(m_wallbc[idx] == 0){ // Inflow/outflow
    for (int i = 0; i < fluxes.size(); i++){
      fluxes[i] = a_extrap_cdr_fluxes[i];
    }
  }
  else if(m_wallbc[idx] == 1){ // wall
    for (int i = 0; i < fluxes.size(); i++){
      fluxes[i] = 0.0;
    }
  }
  else{
    MayDay::Abort("morrow_jiang::compute_cdr_domain_fluxes - uknown domain bc requested");
  }

  
  return fluxes;
}
  
Vector<Real> air3_mc8::compute_cdr_electrode_fluxes(const Real         a_time,
						    const RealVect     a_pos,
						    const RealVect     a_normal,
						    const RealVect     a_E,
						    const Vector<Real> a_cdr_densities,
						    const Vector<Real> a_cdr_velocities,
						    const Vector<Real> a_cdr_gradients,
						    const Vector<Real> a_rte_fluxes,
						    const Vector<Real> a_extrap_cdr_fluxes) const{
  return compute_cdr_fluxes(a_time, a_pos, a_normal, a_E, a_cdr_densities, a_cdr_velocities, a_cdr_gradients, a_rte_fluxes,
			    a_extrap_cdr_fluxes, m_townsend2_electrode, m_electrode_quantum_efficiency);
}

Vector<Real> air3_mc8::compute_cdr_dielectric_fluxes(const Real         a_time,
						     const RealVect     a_pos,
						     const RealVect     a_normal,
						     const RealVect     a_E,
						     const Vector<Real> a_cdr_densities,
						     const Vector<Real> a_cdr_velocities,
						     const Vector<Real> a_cdr_gradients,
						     const Vector<Real> a_rte_fluxes,
						     const Vector<Real> a_extrap_cdr_fluxes) const{
  return compute_cdr_fluxes(a_time, a_pos, a_normal, a_E, a_cdr_densities, a_cdr_velocities, a_cdr_gradients, a_rte_fluxes,
			    a_extrap_cdr_fluxes, m_townsend2_dielectric, m_dielectric_quantum_efficiency);
}

Vector<Real> air3_mc8::compute_cdr_fluxes(const Real         a_time,
					  const RealVect     a_pos,
					  const RealVect     a_normal,
					  const RealVect     a_E,
					  const Vector<Real> a_cdr_densities,
					  const Vector<Real> a_cdr_velocities,
					  const Vector<Real> a_cdr_gradients,
					  const Vector<Real> a_rte_fluxes,
					  const Vector<Real> a_extrap_cdr_fluxes,
					  const Real         a_townsend2,
					  const Real         a_quantum_efficiency) const{
  Vector<Real> fluxes(m_num_species, 0.0);

  const bool cathode = PolyGeom::dot(a_E, a_normal) < 0.0;
  const bool anode   = PolyGeom::dot(a_E, a_normal) > 0.0;

  // Switch for setting drift flux to zero for charge species
  Vector<Real> aj(m_num_species, 0.0);
  for (int i = 0; i < m_num_species; i++){
    if(data_ops::sgn(m_species[i]->get_charge())*PolyGeom::dot(a_E, a_normal) < 0){
      aj[i] = 1.0;
    }
    else {
      aj[i] = 0.0;
    }
  }

  // Drift outflow for now
  for (int i = 0; i < m_num_species; i++){
    fluxes[i] = aj[i]*a_extrap_cdr_fluxes[i];
  }

  return fluxes;
}

Real air3_mc8::initial_sigma(const Real a_time, const RealVect a_pos) const {
  return 0.0;
}