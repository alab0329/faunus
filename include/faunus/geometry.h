#ifndef FAU_GEOMETRY_H
#define FAU_GEOMETRY_H

#include "faunus/common.h"
#include "faunus/point.h"
#include "faunus/slump.h"

namespace Faunus {

  class InputMap;

  /*!
   * \brief Contain geometry related classes for describing simulation cells
   */
  namespace Geometry {

    /*!
     * \brief Polymorphic class for simulation geometries
     * \author Mikael Lund
     *
     * This is an important polymorph class that handles simulation geometries.
     * It contains distance calculation functions, boundary conditions, volume
     * etc.
     *
     * \note This class uses dynamic polymorphism, i.e. virtual functions, which
     *       may have negative impact on performance as function inlining may not be
     *       possible. This is usually a problem only for inner loop distance calculations.
     *       To get optimum performance in inner loops use a derived class directly and do
     *       static compile-time polymorphism.
     * \todo Migrate to NVI (Nonvirtual Interface) pattern -- see "C++ Coding Standards", p.68
     */
    class Geometrybase {
      private:
        virtual string _info(char)=0;
        virtual void _setVolume(double)=0;
        virtual double _getVolume() const=0;
      protected:
        slump slp;
        string name;                                        //!< Name of the geometry
      public:
        enum collisiontype {BOUNDARY,ZONE};                 //!< Types for collision() function
        double getVolume() const;                           //!< Get volume of container
        void setVolume(double);                             //!< Specify new volume
        virtual bool collision(const particle&, collisiontype=BOUNDARY)=0;//!< Check for collision with boundaries, forbidden zones, matter,..
        virtual void randompos(Point &)=0;                  //!< Random point within container
        virtual void boundary(Point &) const=0;             //!< Apply boundary conditions to a point
        virtual void scale(Point&, const double&) const;    //!< Scale point to a new volume - for NPT ensemble
        double dist(const Point&,const Point&);             //!< Distance between two points
        virtual double sqdist(const Point &a, const Point &b) const=0;
        virtual Point vdist(const Point&, const Point&)=0;
        string info(char=20);                               //!< Return info string
        bool save(string);                                  //!< Save container state to disk
        bool load(string,bool=false);                       //!< Load container state from disk
        virtual ~Geometrybase();
    };

    /*! \brief Spherical geometry
     *  \author Mikael Lund
     *  \todo Implement space scaling for isobaric ensemble
     *
     *  This is a spherical simulation container, surrounded by a hard wall.
     */
    class Sphere : public Geometrybase {
      private:
        double r2,diameter;
        void _setVolume(double);
        double _getVolume() const;
      public:
        void setradius(double);
        double r;              //!< Radius
        Sphere(double);
        Sphere(InputMap&);
        string _info(char);
        void randompos(Point &);
        void boundary(Point &) const;
        bool collision(const particle &, collisiontype=BOUNDARY);
        inline double sqdist(const Point &a, const Point &b) const {
          register double dx,dy,dz;
          dx=a.x-b.x;
          dy=a.y-b.y;
          dz=a.z-b.z;
          return dx*dx + dy*dy + dz*dz;
        }
        Point vdist(const Point&, const Point&);
    };

    //---------------------------------------------------------
    /*! \brief Cuboid geometry with periodic boundaries
     *
     *  \author Chris Evers
     *  \date Lund, nov 2010
     *
     *  The Cuboid simulation container has right angles, rectangular faces 
     *  and periodic boundaries. A slice can be introduced to constrain the position
     *  of some of the space to a part of the Cuboid. The function slicecollision
     *  can be used to make sure space are positioned within in the slice.
     */
    class Cuboid : public Geometrybase {
      private:
        string _info(char);                      //!< Return info string
        void _setVolume(double);
        double _getVolume() const;
      protected:
        bool setslice(Point, Point);             //!< Reset slice position
        Point len_inv;                           //!< Inverse sidelengths

      public:
        Cuboid(InputMap&);                       //!< Read input parameters
        bool setlen(Point);                      //!< Reset Cuboid sidelengths
        Point len;                               //!< Sidelengths
        Point len_half;                          //!< Half sidelength
        Point slice_min, slice_max;              //!< Position of slice corners
        Point randompos();                       //!< Get point with random position
        void randompos(Point &);                 //!< Move point to random position
        bool save(string);                       //!< Save container state to disk
        bool load(string,bool=false);            //!< Load container state from disk
        bool collision(const particle&, collisiontype=BOUNDARY);

        inline double sqdist(const Point &a, const Point &b) const {
          double dx=std::abs(a.x-b.x);
          double dy=std::abs(a.y-b.y);
          double dz=std::abs(a.z-b.z);
          if (dx>len_half.x) dx-=len.x;
          if (dy>len_half.y) dy-=len.y;
          if (dz>len_half.z) dz-=len.z;
          return dx*dx + dy*dy + dz*dz;
        }

        inline Point vdist(const Point &a, const Point &b) {       //!< Distance vector
          Point r=a-b;
          if (r.x>len_half.x)
            r.x-=len.x;
          else if (r.x<-len_half.x)
            r.x+=len.x;
          if (r.y>len_half.y)
            r.y-=len.y;
          else if (r.y<-len_half.y)
            r.y+=len.y;
          if (r.z>len_half.z)
            r.z-=len.z;
          else if (r.z<-len_half.z)
            r.z+=len.z;
          return r;
        }

        inline int anint(double x) const {
          return int(x>0. ? x+.5 : x-.5);
        }

        //! Apply periodic boundary conditions
        inline void boundary(Point &a) const {
          if (std::abs(a.x)>len_half.x) a.x-=len.x*anint(a.x*len_inv.x);
          if (std::abs(a.y)>len_half.y) a.y-=len.y*anint(a.y*len_inv.y);
          if (std::abs(a.z)>len_half.z) a.z-=len.z*anint(a.z*len_inv.z);
        }
        virtual void scale(Point&, const double&) const; //!< Scale point to a new volume - for NPT ensemble
    };

    /*!
     * \brief Cuboidslit: cubuid without periodic boundary in the z direction
     * \author Chris Evers
     * \date Lund, nov 2010
     */
    class Cuboidslit : public Cuboid {
      public:
        Cuboidslit(InputMap &);

        //! Calculate distance using the minimum image convention
        inline double sqdist(const Point &a, const Point &b) const {   //!< Squared distance 
          double dx=std::abs(a.x-b.x);
          double dy=std::abs(a.y-b.y);
          double dz=a.z-b.z;
          if (dx>len_half.x) dx-=len.x;
          if (dy>len_half.y) dy-=len.y;                                      
          return dx*dx + dy*dy + dz*dz;
        }   

        inline Point vdist(const Point &a, const Point &b) {       //!< Distance vector
          Point r=a-b;
          if (r.x>len_half.x)
            r.x-=len.x;
          else if (r.x<-len_half.x)
            r.x+=len.x;
          if (r.y>len_half.y)
            r.y-=len.y;
          else if (r.y<-len_half.y)
            r.y+=len.y;
          return r;
        }

        //! Apply periodic boundary conditions
        inline void boundary(Point &a) const {
          if (std::abs(a.x)>len_half.x) a.x-=len.x*anint(a.x*len_inv.x);
          if (std::abs(a.y)>len_half.y) a.y-=len.y*anint(a.y*len_inv.y);
        }
    };

    /*! \brief Cylindrical simulation container
     *  \author Mikael Lund & Bjoern Persson
     *  \todo Needs some testing
     *
     *  This is a Cylinder container where all walls
     *  are HARD. The origin is in the middle of the
     *  Cylinder.
     */
    class Cylinder : public Geometrybase {
      private:
        string _info(char); //!< Cylinder info
        void _setVolume(double);
        double _getVolume() const;
        double halflen;
        double r2;    //!< Cylinder radius squared
        void init(double,double);
      public:
        double len;   //!< Cylinder length
        double r;     //!< Cylinder radius
        double diameter;
        Cylinder(double, double);
        Cylinder(InputMap &);
        void randompos(Point &);
        bool collision(const particle&, collisiontype=BOUNDARY);
        inline double sqdist(const Point &a, const Point &b) const {
          register double dx,dy,dz;
          dx=a.x-b.x;
          dy=a.y-b.y;
          dz=a.z-b.z;
          return dx*dx + dy*dy + dz*dz;
        }
    };

#ifdef HYPERSPHERE
    /*! \brief HyperSphere simulation container
     *  \author Martin Trulsson
     *  \date Lund, 2009
     */
    class hyperSphere : public Sphere {
      private:
        const double pi;
        string _info(char);
      public:
        hyperSphere(InputMap&);
        void randompos(point &);
        bool collision(const particle &, collisiontype=BOUNDARY);

        double dist(const point &a, const point &b) {
          return r*a.geodesic(b); // CHECK!!! (virtual=slow!)
        }

        inline double sqdist(const point &a, const point &b) const {
          return pow(dist(a,b),2); // !! SHOULD BE REAL DISTANCE CHECK!! (virtual=slow!)
        }

        bool overlap(const particle &a) {
          for (int i=0; i<p.size(); i++)
            if (hyperoverlap(a,p[i])==true)
              return true;
          return false;
        }

        inline bool hyperoverlap(const particle &a, const particle &b) {
          return (r*a.geodesic(b)<a.radius+b.radius);
        }
    };

#endif

    /*!
     * \brief Vector rotation routines
     * \note Boundary condition are respected.
     * \author Mikael Lund
     * \date Canberra, 2009
     */
    class VectorRotate {
      private:
        Point origin, u;
        double cosang, sinang;
        double e1mcox, e1mcoy, e1mcoz;
        Geometrybase* geoPtr;
      public:
        void setAxis(Geometrybase&, const Point&, const Point&, double);  //!< Set axis of rotation and degrees to rotate
        Point rotate(const Geometrybase&, Point) const;                   //!< Rotate point around axis
    };

  }//namespace Geometry

  template<typename Tbase>
    class BasePointerList {
      private:
        vector<Tbase*> base;
      public:
        void insert(Tbase &g) {
          if (&g!=nullptr)
            if ( std::find(base.begin(), base.end(), &g)==base.end() )
              base.push_back(&g);
        }

        template<typename Tderived> void sync(const Tbase& source) {
          assert(&source!=nullptr);
          const Tderived* src=dynamic_cast<const Tderived*>(&source); //get dereved class pointer to source
          for (auto b : base) {
            Tderived* target=dynamic_cast<Tderived*>(b); // derived class pointer to target
            if ( src != target )
              (*target)=(*src);                    // copy data from source to target
          }
        }
    };

  namespace Geometry {
    typedef BasePointerList<Geometrybase> GeometryList;
  }

}//namespace
#endif
