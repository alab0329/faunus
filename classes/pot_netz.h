#ifndef _POT_NETZ_H
#define _POT_NETZ_H

#include "potentials.h"

/*! \brief Coulomb potential w. an extra empirical PMF
 *  \author Mikael Lund 
 */
class pot_netz : public pot_coulomb {
  private:
    inline double simple(particle::type &id) {
      switch (id) {
        case particle::I:
          return 8.;
          break;
        case particle::CL:
          return .5;
          break;
      }
      return 1.;
    }
    inline double sam(double z, particle::type &id) {
      //double A,B,zn,C1,C2,C3,D1,D2,D3,n;
      //return ( A/pow(z-zn,12)
      //    - B/pow(z-zn,8)
      //    + C1*(z-C2)*exp(-C3*pow(z-C2,2) )
      //    + D1*exp(-D3*pow(z-D2,2) ) ) / f;
      return 0;
    }
    //!< \params z distance from SURFACE.
    //!< \params id particle type
    inline double air(double z, particle::type &id) { // optimize, please
      z=0.1*z; // AA->nm
      if (z>1.5) return 0; // fit not valid beyond 1.5nm
      register double A,B,zn,C1,C2,C3,D1,D2,D3,n;
      switch (id) {
        case particle::I:
          A=.066,B=.977,zn=2.39,C1=-5.1,C2=.7,C3=8.7,D1=-7.32,D2=-.011,D3=2.49,n=1;
          break;
        case particle::CL:
          A=15.16,B=4.13,zn=-.37,C1=C2=C3=0,D1=.68,D2=.7,D3=23.99,n=2;
          break;
        case particle::NA:
          A=14.62,B=4.35,zn=-.084,C1=4.13,C2=1.1,C3=50.,D1=-.37,D2=.7,D3=10.,n=2;
          break;
        default:
          return 0;
          break;
      }
      return (  A * ( pow(  exp(-B*(z-zn)) + pow(-1,n)   ,2) - 1. )
          + C1*(z-C2)*exp( -C3*pow(z-C2,2)  )
          + D1*exp( -D3*pow(z-D2,2)  ) ) / f;
    }
  public:
    pot_netz( pot_setup &pot ) : pot_coulomb( pot ) {
     name+="/Empirial PMF";
     cite="PRL 2007, 99, 226104";
    }
    inline double hypairpot(particle &p1, particle &p2, double r) {
      if (p1.hydrophobic==true)
        return air(r-p1.radius, p2.id); // c2c -> surface2center
      else if (p2.hydrophobic==true)
        return air(r-p2.radius, p1.id);
      return 0;
    }
};
#endif
