import os
import sys

# Write an options file. This should be a separate routine
def write_template(args):
    app_dir = args.plasmac_home + "/" + args.base_dir + "/" + args.app_name
    options_filename = app_dir + "/template.inputs"
    optf = open(options_filename, 'w')
    
    # Write plasma kinetics options
    optf.write("# ====================================================================================================\n")
    optf.write('# POTENTIAL CURVE\n')
    optf.write("# ====================================================================================================\n")
    optf.write(args.app_name + ".potential = 1\n")
    optf.write(args.app_name + ".basename  = pout\n")
    optf.write('\n')

    options_files = [args.plasmac_home + "/src/amr_mesh/amr_mesh.options", \
                     args.plasmac_home + "/src/driver/driver.options", \
                     args.plasmac_home + "/src/poisson_solver/" + args.poisson_solver + ".options",\
                     args.plasmac_home + "/src/cdr_solver/" + args.cdr_solver + ".options",\
                     args.plasmac_home + "/src/rte_solver/" + args.rte_solver + ".options",\
                     args.plasmac_home + "/src/geometry/geo_coarsener.options", \
                     args.plasmac_home + "/geometries/" + args.geometry + "/" + args.geometry + ".options", \
                     args.plasmac_home + "/physics/cdr_plasma/time_steppers/" + args.time_stepper + "/" + args.time_stepper + ".options", \
                     args.plasmac_home + "/physics/cdr_plasma/plasma_models/" + args.physics + "/" + args.physics + ".options"]

    if not args.cell_tagger == "none":
        options_files.append(args.plasmac_home + "/physics/cdr_plasma/cell_taggers/" + args.cell_tagger + "/" + args.cell_tagger + ".options")
        
    for opt in options_files:
        if os.path.exists(opt):
            f = open(opt, 'r')
            lines = f.readlines()
            optf.writelines(lines)
            optf.write('\n\n')
            f.close()
        else:
            print 'Could not find options file (this _may_ be normal behavior) ' + opt
    optf.close()