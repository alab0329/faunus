#include <faunus/faunus.h>
#include <faunus/ewald.h>
using namespace Faunus;
using namespace Faunus::Potential;

typedef Space<Geometry::Cuboid,PointParticle> Tspace;

int main() {
  InputMap mcp("gctit.json");                   // open user input file
  Tspace spc(mcp);                              // simulation space
  auto pot =
  Energy::Nonbonded<Tspace,CoulombHS>(mcp)      // hamiltonian
  + Energy::EquilibriumEnergy<Tspace>(mcp);


  pot.setSpace(spc);                            // share space w. hamiltonian

  spc.load("state",Tspace::RESIZE);             // load old config. from disk (if any)

  // Two different Widom analysis methods
  double lB = Coulomb(mcp).bjerrumLength();     // get bjerrum length
  Analysis::Widom<PointParticle> widom1;        // widom analysis (I)
  Analysis::WidomScaled<Tspace> widom2(lB,1);   // ...and (II)
  widom1.add(spc.p);
  widom2.add(spc.p);
  Analysis::LineDistribution<> rdf_ab(0.5);      // 0.1 angstrom resolution
  Analysis::VirialPressure virial;

  // Move two first macro molecules to (0,0,+/-25AA)
  vector<Group> g;
  Group g1("protein", 0, 0);
  Group g2("protein", 1, 1);
  g1.setMassCenter(spc), g2.setMassCenter(spc);
  g.push_back(g1), g.push_back(g2);
  Point p(0.0,0.0,25.0);
  g[0].translate(spc,-g[0].cm+p);
  g[1].translate(spc,-g[1].cm-p);
  spc.p=spc.trial;

  Move::Propagator<Tspace> mv(mcp,pot,spc);

  EnergyDrift sys;                              // class for tracking system energy drifts
  sys.init(Energy::systemEnergy(spc,pot,spc.p));// store initial total system energy

  cout << atom.info() + spc.info() + pot.info() + "\n";

  MCLoop loop(mcp);                             // class for handling mc loops
  while ( loop[0] ) {
    while ( loop[1] ) {
      sys+=mv.move();                           // move!
      widom1.sample(spc,pot,1);
      widom2.sample(spc.p,spc.geo);
      if (slump() < 0.10) {
        rdf_ab.sampleMoleculeGroup(spc,g,"protein");
  //      virial.sample(spc,pot);
      }
    }                                           // end of micro loop
    sys.checkDrift(Energy::systemEnergy(spc,pot,spc.p)); // calc. energy drift
    cout << loop.timing();
    rdf_ab.save("rdf.dat");                     // g(r) - not normalized!
  }                                             // end of macro loop

  UnitTest test(mcp);                           // class for unit testing
  //mv.test(test);
  //sys.test(test);
  //widom1.test(test);

  cout << loop.info() + sys.info() + mv.info() + test.info()
    + widom1.info() + widom2.info() + virial.info();

  FormatPQR::save("confout.pqr", spc.p);        // PQR snapshot for VMD etc.
  spc.save("state");                            // final simulation state

  return test.numFailed();
}

/**
  @page example_grand Example: Grand Canonical Salt

  This is an example of a grand canonical salt solution (NmuT)
  with the following MC moves:

  - salt translation
  - salt exchange with virtual bulk - i.e. constant chemical potential

  In addition, two different Widom methods are used to check the chemical
  potential. Note that this analysis is formally valid only in the canonical
  ensemble which may lead to deviations at small system sizes.
  It is, however, easy to turn off GC moves via the input file.

  Run this example from the terminal:

      $ cd src/examples
      $ python grand.py
  
  Input
  =====

  In this example a python script is used to generate the input json file as
  well as run the executable.

  grand.json
  ----------
  @includelineno examples/grand.json  

  Output
  ======
  This is the output generated by `grand.cpp`. Note that the default is to
  use Unicode UTF-16 encoding to print mathematical symbols. If your terminal is unable to
  print this properly, Unicode output can be switched off during compilation.

  @include examples/grand.out

  grand.cpp
  =========
  @includelineno examples/grand.cpp
*/