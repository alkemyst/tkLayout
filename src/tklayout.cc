#include <boost/program_options.hpp> 
#include <stdlib.h>
#include <iostream>
#include <string>
#include <Squid.hh>
#include "SvnRevision.hh"

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  std::string usage("Usage: ");
  usage += argv[0];
  usage += " <geometry file> [options]";
  int geomtracks, mattracks;
  //std::vector<int> tracksim;
  int verbosity;
  int randseed; 

  std::string basename, optfile, xmldir, htmldir;
  
  po::options_description shown("Analysis options");
  shown.add_options()
    ("help,h", "Display this help message.")
    ("opt-file", po::value<std::string>(&optfile)->implicit_value(""), "Specify an option file to parse program options from, in addition to the command line")
    ("geometry-tracks,n", po::value<int>(&geomtracks)->default_value(100), "N. of tracks for geometry calculations.")
    ("material-tracks,N", po::value<int>(&mattracks)->default_value(100), "N. of tracks for material calculations.")
    ("power,p", "Report irradiation power analysis.")
    ("bandwidth,b", "Report base bandwidth analysis.")
    ("bandwidth-cpu,B", "Report multi-cpu bandwidth analysis.\n\t(implies 'b')")
    ("material,m", "Report materials and weights analyses.")
    ("resolution,r", "Report resolution analysis.")
    ("debug-resolution,R", "Report extended resolution analysis : debug plots for modules parametrized spatial resolution.")
    ("pattern-reco,P", "Report pattern recognition analysis.")
    ("outerCablingMap", "Outer Tracker : Build an optical cabling map, which connects each module to a bundle, cable, DTC + Build a power cabling map. Also provide info on routing of services through channels.")
    ("innerCablingMap", "Inner Tracker: Build an optical cabling map.")
    ("trigger,t", "Report base trigger analysis.")
    ("trigger-ext,T", "Report extended trigger analysis.\n\t(implies 't')")
    ("all,a", "Report all analyses, except extended\ntrigger and debug page. (implies all other relevant\nreport options)")
    ("graph,g", "Build and report neighbour graph.")
    ("xml", po::value<std::string>(&xmldir)->implicit_value(""), "Produce XML output files for materials.\nOptional arg specifies the subdirectory\nof the output directory (chosen via inst\nscript) where to create XML files.\nIf not supplied, the config file name (minus extension)\nwill be used as subdir.")
    ("dump-json", "Dump full tracker structure as JSON to 'tracker.json'")
    ("html-dir", po::value<std::string>(&htmldir), "Override the default html output dir\n(equal to the tracker name in the main\ncfg file) with the one specified.")
    ("verbosity", po::value<int>(&verbosity)->default_value(1), "Levels of details in the program's output (overridden by the option 'quiet').")
    ("quiet", "No output is produced, except the required messages (equivalent to verbosity 0, overrides the option 'verbosity')")
    ("performance", "Outputs the CPU time needed for each computing step (overrides the option 'quiet').")
    ("randseed", po::value<int>(&randseed)->default_value(0xcafebabe), "Set the random seed\nIf explicitly set to 0, seed is random")
    ;
    
  po::options_description trackopt("Track simulation options");
  trackopt.add_options()
    ("tracksim", "Switch to track sim mode, normal analysis disabled")
    ("num-events", po::value<std::string>(), "N. of events to simulate")
    ("num-tracks-ev", po::value<std::string>(), "N. of tracks per event")
    ("event-offset", po::value<std::string>(), "Start the event numbering from an offset value.")
    ("eta", po::value<std::string>(), "Particle eta")
    ("phi0", po::value<std::string>(), "Particle phi0")
    ("z0", po::value<std::string>(), "Particle z0")
    ("pt", po::value<std::string>(), "Particle transverse momentum, alternative to --invPt")
    ("invPt", po::value<std::string>(), "Specify pt in terms of its inverse, alternative to --pt")
    ("charge", po::value<std::string>(), "Particle charge")
    ("instance-id", po::value<std::string>(), "Id of the program instance, to tag the output file with")
    ("tracks-dir", po::value<std::string>(), "Override the default tracksim output dir.\nIf not supplied, the files will be saved in\nthe working dir")
    ;

  po::options_description otheropt("Other options");
  otheropt.add_options()
    ("localAxesLabels", "Prints local axes and labels in the geometry output.\nThis is useful for debugging purposes, makes the output uglier.\nDisabled by default.")
    ("version,v", "Prints software version (SVN revision) and quits.")
    ("webOutput,w", "Prepares the output for web publishing (local running is assumed otherwise).")
    ;

  
  po::options_description hidden;
  hidden.add_options()("base-name", po::value<std::string>(&basename));

  po::positional_options_description posopt;
  posopt.add("base-name", 1); 


  po::options_description mainopt;
  mainopt.add(shown).add(hidden).add(trackopt).add(otheropt);
  
  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(mainopt).positional(posopt).run(), vm);
    po::notify(vm);
    if (!optfile.empty()) { 
      std::ifstream ifs(optfile.c_str());
      if (!ifs) throw po::error(("Options file \"" + optfile + "\" not found").c_str());
      po::store(po::parse_config_file(ifs, mainopt, true), vm);
      ifs.close();
    } 
    po::notify(vm);


    if (geomtracks < 1) throw po::invalid_option_value("geometry-tracks");
    if (mattracks < 1) throw po::invalid_option_value("material-tracks");
    if (!vm.count("base-name") && !vm.count("help") && !vm.count("version")) throw po::error("Missing geometry file"); 

  } catch(po::error& e) {
    std::cerr << "\nERROR: " << e.what() << std::endl << std::endl;
    std::cout << usage << std::endl << shown << trackopt << std::endl;
    return EXIT_FAILURE;
  }

  if (vm.count("help")) {
    std::cout << usage << std::endl << shown << trackopt << otheropt << std::endl;
    return 0;
  }

  if (vm.count("version")) {
    std::cout << "tklayout revision " << SvnRevision::revisionNumber << std::endl;
    return 0;
  }

  insur::Squid squid;
  squid.setCommandLine(argc, argv);
  bool verboseMaterial = false;
  unsigned int verboseWatch = verbosity;
  if (vm.count("quiet")) verboseWatch=0;
  bool performanceWatch = vm.count("performance");
  if (performanceWatch) {
    if (verboseWatch==0) verboseWatch = 1;
  }
  StopWatch::instance()->setVerbosity(verboseWatch, performanceWatch);

  squid.setGeometryFile(basename);
  squid.webOutput = (vm.count("webOutput")!=0);
  if (htmldir != "") squid.setHtmlDir(htmldir);



  // The tracker (and possibly pixel) must be build in any case
  if (!squid.buildTracker()) return EXIT_FAILURE;
  if (!squid.prepareTargetDirectory()) return EXIT_FAILURE;

  // Build cabling map.
  // With option 'all', cabling map is only computed on a specific layout, for which the map is designed.
  // It is also computed for layouts that are marked as compatible with this map
  // The user can also force the computation by using 'cablingMap' option.
  const bool compatibleOTCablingMap = std::any_of(insur::compatible_cabledOTName.begin(), insur::compatible_cabledOTName.end(), [&](std::string s) {
      return (basename.find(s) != std::string::npos); });
  const bool compatibleITCablingMap = std::any_of(insur::compatible_cabledITName.begin(), insur::compatible_cabledITName.end(), [&](std::string s) {
      return (basename.find(s) != std::string::npos); });
  const bool buildOuterCablingMap = ( (vm.count("all") && (basename.find(insur::default_cabledOTName) != std::string::npos || compatibleOTCablingMap)) // Layout on which Outer Tracker cabling map was designed or one that is compatible.
				 || vm.count("outerCablingMap") ); // Forces cabling map computation.
  if (buildOuterCablingMap && !squid.buildOuterCablingMap(vm.count("outerCablingMap")) ) return EXIT_FAILURE;
  const bool buildInnerCablingMap = ( (vm.count("all") && (basename.find(insur::default_cabledITName) != std::string::npos || compatibleITCablingMap)) // Layout on which Inner Tracker cabling map was designed.
				 || vm.count("innerCablingMap") ); // Forces cabling map computation.
  if (buildInnerCablingMap && !squid.buildInnerCablingMap(vm.count("innerCablingMap")) ) return EXIT_FAILURE;
  
  if (!vm.count("tracksim")) {
    // The tracker should pick the types here but in case it does not,
    // we can still write something
    if (!squid.pureAnalyzeGeometry(geomtracks)) return EXIT_FAILURE;

    if (!squid.prepareTargetDirectory()) return EXIT_FAILURE;
    
    if ((vm.count("all") || vm.count("bandwidth") || vm.count("bandwidth-cpu")) && !squid.reportBandwidthSite()) return EXIT_FAILURE;
    if ((vm.count("all") || vm.count("bandwidth-cpu")) && (!squid.reportTriggerProcessorsSite()) ) return EXIT_FAILURE;
    if ((vm.count("all") || vm.count("power")) && (!squid.reportPowerSite()) ) return EXIT_FAILURE;

    // If we need to have the material model, then we build it
    if ( vm.count("all") || vm.count("material") || vm.count("resolution") || vm.count("debug-resolution") || vm.count("pattern-reco") || vm.count("graph") || vm.count("xml") ) {
      if (squid.buildMaterials(verboseMaterial) && squid.createMaterialBudget(verboseMaterial)) {
        if ( vm.count("all") || vm.count("material") || vm.count("resolution") || vm.count("debug-resolution") || vm.count("pattern-reco")) {
          bool triggerMB          = vm.count("all") || vm.count("material");
          bool triggerRes         = vm.count("all") || vm.count("resolution") ||  vm.count("debug-resolution");
          bool triggerPatternReco = vm.count("all") || vm.count("pattern-reco");
          if (!squid.pureAnalyzeMaterialBudget(mattracks, triggerRes, triggerPatternReco, vm.count("debug-resolution"))) return EXIT_FAILURE;
          if (triggerMB  && !squid.reportMaterialBudgetSite()) return EXIT_FAILURE;
          if (triggerRes && !squid.reportResolutionSite()) return EXIT_FAILURE;
          if (triggerPatternReco && !squid.reportPatternRecoSite()) return EXIT_FAILURE;
        }
        if (vm.count("graph") && !squid.reportNeighbourGraphSite()) return EXIT_FAILURE;
        if (vm.count("xml") && !squid.translateFullSystemToXML(xmldir)) return (EXIT_FAILURE);
      }
    }
    
    if (buildOuterCablingMap && !squid.reportOuterCablingMapSite(vm.count("outerCablingMap"), basename)) return EXIT_FAILURE;
    if (buildInnerCablingMap && !squid.reportInnerCablingMapSite(vm.count("innerCablingMap"), basename)) return EXIT_FAILURE;
    if (buildInnerCablingMap && buildOuterCablingMap && !squid.reportInnerAndOuterCablingMapSite(vm.count("innerCablingMap"), vm.count("outerCablingMap"), basename)) return EXIT_FAILURE;

    if ((vm.count("all") || vm.count("trigger") || vm.count("trigger-ext")) &&
        ( !squid.analyzeTriggerEfficiency(mattracks, vm.count("trigger-ext")) || !squid.reportTriggerPerformanceSite(vm.count("trigger-ext"))) ) return EXIT_FAILURE;
   
    if (vm.count("xml")) squid.createAdditionalXmlSite(xmldir);

    bool debugResolution = vm.count("debug-resolution") > 0;
    bool localAxesLabels = vm.count("localAxesLabels") > 0;
    if (!squid.reportGeometrySite(debugResolution, localAxesLabels)) return EXIT_FAILURE;
    if (!squid.additionalInfoSite()) return EXIT_FAILURE;
    if (vm.count("all") || vm.count("dump-json")) squid.dumpJson("tracker.json");
    if (!squid.makeSite()) return EXIT_FAILURE;

  } else {
    //if (tracksim.size() < 1 || tracksim.size > 2) {
    //  std::cerr << "Wrong number of parameters. Syntax: --tracksim <num events> <num tracks/ev>" << std::endl;
    //  std::cerr << "                                    --tracksim parameterfile" << std::endl;
    //  std::cerr << "                                    --tracksim \"key1 = value1; key2 = value2 ...\"" << std::endl;
    //  return EXIT_FAILURE;
   // }
    if (!squid.pureAnalyzeGeometry(geomtracks)) return EXIT_FAILURE;
  
//    if (tracksim.size() == 2) {
//      vmtracks.insert(std::make_pair("num-events", po::variable_value(boost::any(tracksim[0]), false)));
//      vmtracks.insert(std::make_pair("num-tracks", po::variable_value(boost::any(tracksim[1]), false)));
//    }
    squid.simulateTracks(vm, randseed);

    //if (tracksim.size() == 2) { squid.simulateTracks(str2any<long int>(tracksim[0]), str2any<long int>(tracksim[1]), randseed, "", ""); }
    //else if (tracksim.size() == 1 && tracksim[0].at(0)=="\"") { squid.simulateTracks(0, 0, randseed, "", trim(tracksim[0], " \"")); }
    //else { squid.simulateTracks(0, 0, randseed, tracksim[0], ""); }
  }

  return EXIT_SUCCESS;
}




