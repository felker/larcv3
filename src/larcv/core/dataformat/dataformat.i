%module dataformat
%{
/* Put header files here or function declarations like below */

// Basic Types:
#include "core/dataformat/DataFormatTypes.h"
#include "core/dataformat/EventID.h"
#include "core/dataformat/Point.h"
#include "core/dataformat/BBox.h"
#include "core/dataformat/Vertex.h"
#include "core/dataformat/Particle.h"
#include "core/dataformat/ImageMeta.h"
#include "core/dataformat/Image2D.h"
#include "core/dataformat/Voxel.h"

// IO Interface classes:
#include "core/dataformat/EventBase.h"
#include "core/dataformat/EventParticle.h"
#include "core/dataformat/EventImage2D.h"
#include "core/dataformat/DataProductFactory.h"
#include "core/dataformat/IOManager.h"
//#include "core/dataformat/EventVoxel.h"


using namespace larcv;
%}


%include "std_string.i"
%include "std_vector.i"

// Basic Types:
%include "core/dataformat/DataFormatTypes.h"
%include "core/dataformat/EventID.h"
%include "core/dataformat/Point.h"
%include "core/dataformat/BBox.h"
%include "core/dataformat/Vertex.h"
%include "core/dataformat/Particle.h"
%include "core/dataformat/ImageMeta.h"
%include "core/dataformat/Image2D.h"
%include "core/dataformat/Voxel.h"

// IO Interface classes:
%include "core/dataformat/EventBase.h"
%include "core/dataformat/EventParticle.h"
%include "core/dataformat/EventImage2D.h"
%include "core/dataformat/DataProductFactory.h"
%include "core/dataformat/IOManager.h"
//%include "core/dataformat/EventVoxel.h"

// Add templates for the dataproducts that get wrapped vectors:
%template(VectorOfDouble) std::vector<double>;
%template(VectorOfFloat) std::vector<float>;
%template(VectorOfSizet) std::vector<size_t>;
%template(VectorOfVoxel) std::vector<larcv::Voxel>;
%template(VectorOfVoxelSet) std::vector<larcv::VoxelSet>;
// %template(VectorOfSparse) std::vector<larcv::SparseTensor>;
%template(VectorOfParticle) std::vector<larcv::Particle>;

// Define instantiations of LARCV templates:
%template(Point2D) larcv::Point<2>;
%template(Point3D) larcv::Point<3>;
%template(BBox2D) larcv::BBox<2>;
%template(BBox3D) larcv::BBox<3>;
%template(ImageMeta2D) larcv::ImageMeta<2>;
%template(ImageMeta3D) larcv::ImageMeta<3>;




