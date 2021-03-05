#include "driver.H"
#include "poisson_multifluid_gmg.H"
#include "rod_dielectric.H"
#include "poisson_stepper.H"
#include "ParmParse.H"

using namespace physics::poisson;

int main(int argc, char* argv[]){

#ifdef CH_MPI
  MPI_Init(&argc, &argv);
#endif

  // Build class options from input script and command line options
  char* input_file = argv[1];
  ParmParse pp(argc-2, argv+2, NULL, input_file);

  // Set geometry and AMR 
  RefCountedPtr<computational_geometry> compgeom = RefCountedPtr<computational_geometry> (new rod_dielectric());
  RefCountedPtr<amr_mesh> amr                    = RefCountedPtr<amr_mesh> (new amr_mesh());
  RefCountedPtr<geo_coarsener> geocoarsen        = RefCountedPtr<geo_coarsener> (new geo_coarsener());
  RefCountedPtr<cell_tagger> tagger              = RefCountedPtr<cell_tagger> (NULL);

  // Set up basic Poisson, potential = 1 
  auto timestepper = RefCountedPtr<poisson_stepper<poisson_multifluid_gmg> >
     (new poisson_stepper<poisson_multifluid_gmg>());

  // Set up the driver and run it
  RefCountedPtr<driver> engine = RefCountedPtr<driver> (new driver(compgeom, timestepper, amr, tagger, geocoarsen));
  engine->setup_and_run();

#ifdef CH_MPI
  CH_TIMER_REPORT();
  MPI_Finalize();
#endif
}