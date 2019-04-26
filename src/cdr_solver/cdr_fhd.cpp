/*!
  @file   cdr_fhd.cpp
  @brief  Implementation of cdr_fhd.H
  @author Robert Marskar
  @date   Jan. 2018
*/

#include "cdr_fhd.H"
#include "cdr_fhdF_F.H"
#include "data_ops.H"
#include <time.h>
#include <chrono>

#include <ParmParse.H>

cdr_fhd::cdr_fhd() : cdr_gdnv() {
  m_name = "cdr_fhd";
  m_seed = 0;  
  std::string str;
  ParmParse pp("cdr_fhd");


  pp.get("stochastic_diffusion", str); m_stochastic_diffusion = (str == "true") ? true : false;
  pp.get("stochastic_advection", str); m_stochastic_advection = (str == "true") ? true : false;
  pp.get("limit_slopes", str);         m_slopelim             = (str == "true") ? true : false;
  pp.get("seed", m_seed);

  if(m_seed < 0){
    std::chrono::system_clock::now().time_since_epoch().count();
  }

  this->set_divF_nc(1); // Use covered face stuff for nonconservative divergence

  m_rng = new std::mt19937_64(m_seed);
}

cdr_fhd::~cdr_fhd(){
  this->delete_covered();
}


void cdr_fhd::advance_euler(EBAMRCellData& a_new_state, const EBAMRCellData& a_old_state, const Real a_dt){
  CH_TIME("cdr_fhd::advance_euler");
  if(m_verbosity > 5){
    pout() << m_name + "::advance_euler (no source)" << endl;
  }
  
  if(!m_stochastic_diffusion){
    cdr_tga::advance_euler(a_new_state, a_old_state, a_dt);
  }
  else{
    bool converged = false;

    const int comp         = 0;
    const int ncomp        = 1;
    const int finest_level = m_amr->get_finest_level();

    EBAMRCellData ransource;
    m_amr->allocate(ransource, m_phase, 1);
    this->GWN_diffusion_source(ransource, a_old_state);
      
    // Do the regular aliasing stuff for passing into AMRMultiGrid
    Vector<LevelData<EBCellFAB>* > new_state, old_state, source;
    m_amr->alias(new_state, a_new_state);
    m_amr->alias(old_state, a_old_state);
    m_amr->alias(source,    ransource);
      
    const Real alpha = 0.0;
    const Real beta  = 1.0;
      
    // Multigrid solver
    m_eulersolver->resetAlphaAndBeta(alpha, beta);
    m_eulersolver->oneStep(new_state, old_state, source, a_dt, 0, finest_level, 0.0);
      
    const int status = m_gmg_solver->m_exitStatus;  // 1 => Initial norm sufficiently reduced
    if(status == 1 || status == 8 || status == 9){  // 8 => Norm sufficiently small
      converged = true;
    }
    MayDay::Abort("cdr_fhd::advance_euler - stochastic not implemented");
  }
}

void cdr_fhd::advance_euler(EBAMRCellData&       a_new_state,
			    const EBAMRCellData& a_old_state,
			    const EBAMRCellData& a_source,
			    const Real           a_dt){
  CH_TIME("cdr_fhd::advance_euler");
  if(m_verbosity > 5){
    pout() << m_name + "::advance_euler (with source)" << endl;
  }

  if(m_diffusive){
    if(!m_stochastic_diffusion){
      cdr_tga::advance_euler(a_new_state, a_old_state, a_source, a_dt);
    }
    else{
      bool converged = false;

      const int comp         = 0;
      const int ncomp        = 1;
      const int finest_level = m_amr->get_finest_level();

      EBAMRCellData ransource;
      m_amr->allocate(ransource, m_phase, 1);
      this->GWN_diffusion_source(ransource, a_old_state);
      data_ops::incr(ransource, a_source, 1.0);
      
      // Do the regular aliasing stuff for passing into AMRMultiGrid
      Vector<LevelData<EBCellFAB>* > new_state, old_state, source;
      m_amr->alias(new_state, a_new_state);
      m_amr->alias(old_state, a_old_state);
      m_amr->alias(source,    ransource);
      
      const Real alpha = 0.0;
      const Real beta  = 1.0;
      
      // Multigrid solver
      m_eulersolver->resetAlphaAndBeta(alpha, beta);
      m_eulersolver->oneStep(new_state, old_state, source, a_dt, 0, finest_level, 0.0);
      
      const int status = m_gmg_solver->m_exitStatus;  // 1 => Initial norm sufficiently reduced
      if(status == 1 || status == 8 || status == 9){  // 8 => Norm sufficiently small
	converged = true;
      }
    }
  }
  else{
    data_ops::copy(a_new_state, a_old_state);
  }
}

void cdr_fhd::advance_tga(EBAMRCellData& a_new_state, const EBAMRCellData& a_old_state, const Real a_dt){
  CH_TIME("cdr_fhd::advance_tga");
  if(m_verbosity > 5){
    pout() << m_name + "::advance_tga (no source)" << endl;
  }
  
  if(!m_stochastic_diffusion){
    cdr_tga::advance_tga(a_new_state, a_old_state, a_dt);
  }
  else{
    bool converged = false;

    const int comp         = 0;
    const int ncomp        = 1;
    const int finest_level = m_amr->get_finest_level();

    EBAMRCellData ransource;
    m_amr->allocate(ransource, m_phase, 1);
    this->GWN_diffusion_source(ransource, a_old_state);
      
    // Do the regular aliasing stuff for passing into AMRMultiGrid
    Vector<LevelData<EBCellFAB>* > new_state, old_state, source;
    m_amr->alias(new_state, a_new_state);
    m_amr->alias(old_state, a_old_state);
    m_amr->alias(source,    ransource);
      
    const Real alpha = 0.0;
    const Real beta  = 1.0;
      
    // Multigrid solver
    m_tgasolver->resetAlphaAndBeta(alpha, beta);
    m_tgasolver->oneStep(new_state, old_state, source, a_dt, 0, finest_level, 0.0);
      
    const int status = m_gmg_solver->m_exitStatus;  // 1 => Initial norm sufficiently reduced
    if(status == 1 || status == 8 || status == 9){  // 8 => Norm sufficiently small
      converged = true;
    }
  }
}

void cdr_fhd::advance_tga(EBAMRCellData&       a_new_state,
			  const EBAMRCellData& a_old_state,
			  const EBAMRCellData& a_source,
			  const Real           a_dt){
  CH_TIME("cdr_fhd::advance_tga");
  if(m_verbosity > 5){
    pout() << m_name + "::advance_tga (with source)" << endl;
  }
  
  if(!m_stochastic_diffusion){
    cdr_tga::advance_tga(a_new_state, a_old_state, a_source, a_dt);
  }
  else{
    bool converged = false;

    const int comp         = 0;
    const int ncomp        = 1;
    const int finest_level = m_amr->get_finest_level();

    EBAMRCellData ransource;
    m_amr->allocate(ransource, m_phase, 1);
    this->GWN_diffusion_source(ransource, a_old_state);
    data_ops::incr(ransource, a_source, 1.0);
      
    // Do the regular aliasing stuff for passing into AMRMultiGrid
    Vector<LevelData<EBCellFAB>* > new_state, old_state, source;
    m_amr->alias(new_state, a_new_state);
    m_amr->alias(old_state, a_old_state);
    m_amr->alias(source,    ransource);
      
    const Real alpha = 0.0;
    const Real beta  = 1.0;
      
    // Multigrid solver
    m_tgasolver->resetAlphaAndBeta(alpha, beta);
    m_tgasolver->oneStep(new_state, old_state, source, a_dt, 0, finest_level, 0.0);
      
    const int status = m_gmg_solver->m_exitStatus;  // 1 => Initial norm sufficiently reduced
    if(status == 1 || status == 8 || status == 9){  // 8 => Norm sufficiently small
      converged = true;
    }
  }
}

void cdr_fhd::GWN_diffusion_source(EBAMRCellData& a_ransource, const EBAMRCellData& a_cell_states){
  CH_TIME("cdr_fhd::GWN_diffusion_source");
  if(m_verbosity > 5){
    pout() << m_name + "::GWN_diffusion_source" << endl;
  }

  const int comp  = 0;
  const int ncomp = 1;
  
  EBAMRFluxData ranflux;
  EBAMRFluxData GWN;
  
  m_amr->allocate(ranflux,   m_phase, ncomp);
  m_amr->allocate(GWN,       m_phase, ncomp);
  
  data_ops::set_value(a_ransource, 0.0);
  data_ops::set_value(ranflux,   0.0);
  data_ops::set_value(GWN,       0.0);
      
  this->fill_GWN(GWN, 1.0);                             // Gaussian White Noise
  this->smooth_heaviside_faces(ranflux, a_cell_states); // ranflux = phis
  data_ops::multiply(ranflux, m_diffco);                // ranflux = D*phis
  data_ops::scale(ranflux, 2.0);                        // ranflux = 2*D*phis
  data_ops::square_root(ranflux);                       // ranflux = sqrt(2*D*phis)
  data_ops::multiply(ranflux, GWN);                     // Holds random, cell-centered flux

  // Source term. 
  // I want to re-use conservative_divergence(), but that also computes with the EB fluxes. Back that up
  // first and use the already written and well-tested routine. Then copy back
  EBAMRIVData backup;
  m_amr->allocate(backup, m_phase, 1);
  data_ops::copy(backup, m_ebflux);
  data_ops::set_value(m_ebflux, 0.0);
  conservative_divergence(a_ransource, ranflux); // Compute the conservative divergence. This also refluxes. 
  data_ops::copy(m_ebflux, backup);
}

void cdr_fhd::smooth_heaviside_faces(EBAMRFluxData& a_face_states, const EBAMRCellData& a_cell_states){
  CH_TIME("cdr_fhd::smooth_heaviside_faces");
  if(m_verbosity > 5){
    pout() << m_name + "::smooth_heaviside_faces" << endl;
  }

  const int comp  = 0;
  const int ncomp = 1;
  const int finest_level = m_amr->get_finest_level();

  for (int lvl = 0; lvl <= finest_level; lvl++){
    const DisjointBoxLayout& dbl = m_amr->get_grids()[lvl];
    const EBISLayout& ebisl      = m_amr->get_ebisl(m_phase)[lvl];
    const Real dx                = m_amr->get_dx()[lvl];
    const Real vol               = pow(dx, SpaceDim);
    
    for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){
      const Box& box = dbl.get(dit());
      const EBISBox& ebisbox = ebisl[dit()];
      const EBGraph& ebgraph = ebisbox.getEBGraph();

      for (int dir = 0; dir < SpaceDim; dir++){
	EBFaceFAB& face_states       = (*a_face_states[lvl])[dit()][dir];
	const EBCellFAB& cell_states = (*a_cell_states[lvl])[dit()];

	BaseFab<Real>& reg_face       = face_states.getSingleValuedFAB();
	const BaseFab<Real>& reg_cell = cell_states.getSingleValuedFAB();

	Box facebox = box;
	facebox.grow(dir,1);
	facebox.surroundingNodes(dir);

	face_states.setVal(0.0);
	FORT_HEAVISIDE_MEAN(CHF_FRA1(reg_face, comp),
			    CHF_CONST_FRA1(reg_cell, comp),
			    CHF_CONST_INT(dir),
			    CHF_CONST_REAL(dx),
			    CHF_BOX(facebox));

	// Fix up irregular cell faces
	const IntVectSet& irreg = ebisbox.getIrregIVS(box);
	const FaceStop::WhichFaces stopcrit = FaceStop::SurroundingNoBoundary;
	for (FaceIterator faceit(irreg, ebgraph, dir, stopcrit); faceit.ok(); ++faceit){
	  const FaceIndex& face = faceit();


	  const VolIndex lovof = face.getVoF(Side::Lo);
	  const VolIndex hivof = face.getVoF(Side::Hi);

	  const Real loval = cell_states(lovof, comp);
	  const Real hival = cell_states(hivof, comp);


	  Real Hlo;
	  if(loval*vol <= 0.0){
	    Hlo = 0.0;
	  }
	  else if(loval*vol >= 1.0){
	    Hlo = 1.0;
	  }
	  else{
	    Hlo = loval*vol;
	  }

	  Real Hhi;
	  if(hival*vol <= 0.0){
	    Hhi = 0.0;
	  }
	  else if(hival*vol >= 1.0){
	    Hhi = 1.0;
	  }
	  else{
	    Hhi = hival*vol;
	  }

	  face_states(face, comp) = 0.5*(hival + loval)*Hlo*Hhi;
	}

	// No random flux on domain faces. 
	for (SideIterator sit; sit.ok(); ++sit){
	  
	}
      }
    }

    // Covered is bogus
    EBLevelDataOps::setCoveredVal(*a_face_states[lvl], 0.0);
  }
}

void cdr_fhd::fill_GWN(EBAMRFluxData& a_noise, const Real a_sigma){
  CH_TIME("cdr_fhd::fill_GWN");
  if(m_verbosity > 5){
    pout() << m_name + "::fill_GWN" << endl;
  }

  const int comp  = 0;
  const int ncomp = 1;
  const int finest_level = m_amr->get_finest_level();

  std::normal_distribution<double> GWN(0.0, a_sigma);

  for (int lvl = 0; lvl <= finest_level; lvl++){
    const DisjointBoxLayout& dbl = m_amr->get_grids()[lvl];
    const EBISLayout& ebisl      = m_amr->get_ebisl(m_phase)[lvl];
    const Real dx                = m_amr->get_dx()[lvl];
    const Real vol               = pow(dx, SpaceDim);
    const Real ivol              = sqrt(1./vol);
    
    for (DataIterator dit = dbl.dataIterator(); dit.ok(); ++dit){
      const Box& box = dbl.get(dit());
      const EBISBox& ebisbox = ebisl[dit()];
      const EBGraph& ebgraph = ebisbox.getEBGraph();

      for (int dir = 0; dir < SpaceDim; dir++){
	EBFaceFAB& noise = (*a_noise[lvl])[dit()][dir];

	Box facebox = box;
	facebox.surroundingNodes(dir);

	noise.setVal(0.0);

	// Regular faces
	BaseFab<Real>& noise_reg = noise.getSingleValuedFAB();
	for (BoxIterator bit(facebox); bit.ok(); ++bit){
	  const IntVect iv = bit();
	  noise_reg(iv, comp) = GWN(*m_rng)*ivol;
	}

	// Irregular faces
	const IntVectSet& irreg = ebisbox.getIrregIVS(box);
	const FaceStop::WhichFaces stopcrit = FaceStop::SurroundingNoBoundary;
	for (FaceIterator faceit(irreg, ebgraph, dir, stopcrit); faceit.ok(); ++faceit){
	  const FaceIndex& face = faceit();

	  noise(face, comp) = GWN(*m_rng)*ivol;
	}
      }
    }
  }
}

void cdr_fhd::compute_divF(EBAMRCellData& a_divF, const EBAMRCellData& a_state, const Real a_extrap_dt, const bool a_redist){
  CH_TIME("cdr_fhd::compute_divF(divF, state)");
  if(m_verbosity > 5){
    pout() << m_name + "::compute_divF(divF, state)" << endl;
  }

  if(m_mobile){
    cdr_tga::compute_divF(a_divF, a_state, a_extrap_dt, a_redist); // Deterministic flux
    if(m_stochastic_advection){ // Stochastic flux
      EBAMRCellData ransource;
      m_amr->allocate(ransource, m_phase, 1);
      this->GWN_advection_source(ransource, a_state);
      data_ops::incr(a_divF, ransource, 1.0);
    }
  }
  else{
    data_ops::set_value(a_divF, 0.0);
  }
}

void cdr_fhd::GWN_advection_source(EBAMRCellData& a_ransource, const EBAMRCellData& a_cell_states){
  CH_TIME("cdr_fhd::GWN_advection_source");
  if(m_verbosity > 5){
    pout() << m_name + "::GWN_advection_source" << endl;
  }

  const int comp  = 0;
  const int ncomp = 1;
  
  EBAMRFluxData ranflux;
  EBAMRFluxData GWN;
  
  m_amr->allocate(ranflux,   m_phase, ncomp);
  m_amr->allocate(GWN,       m_phase, ncomp);
  
  data_ops::set_value(a_ransource, 0.0);
  data_ops::set_value(ranflux,   0.0);
  data_ops::set_value(GWN,       0.0);
      
  this->fill_GWN(GWN, 1.0);                             // Gaussian White Noise
  this->smooth_heaviside_faces(ranflux, a_cell_states); // ranflux = phis
  data_ops::square_root(ranflux);                       // ranflux = sqrt(phis)
  data_ops::multiply(ranflux, GWN);                     // ranflux = sqrt(phis)*W/sqrt(dV)
  data_ops::multiply(ranflux, m_velo_face);             // ranflux = v*sqrt(phis)*W/sqrt(dV)

  // Source term. 
  // I want to re-use conservative_divergence(), but that also computes with the EB fluxes. Back that up
  // first and use the already written and well-tested routine. Then copy back.
  EBAMRIVData backup;
  m_amr->allocate(backup, m_phase, 1);
  data_ops::copy(backup, m_ebflux);
  data_ops::set_value(m_ebflux, 0.0);
  conservative_divergence(a_ransource, ranflux); // Compute the conservative divergence. This also refluxes. 
  data_ops::copy(m_ebflux, backup);
}