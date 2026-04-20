
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <string>

// vector math, just what we need for the parametric surface and normals
struct Vec3 {
  double x, y, z;
  Vec3(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
  Vec3 operator+(const Vec3& o) const { return Vec3(x+o.x, y+o.y, z+o.z); }
  Vec3 operator-(const Vec3& o) const { return Vec3(x-o.x, y-o.y, z-o.z); }
  Vec3 operator*(double t)      const { return Vec3(x*t, y*t, z*t); }
};

static Vec3 cross(Vec3 a, Vec3 b) {
  return Vec3(a.y*b.z - a.z*b.y,
              a.z*b.x - a.x*b.z,
              a.x*b.y - a.y*b.x);
}
static Vec3 normalize(Vec3 v) {
  double l = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
  if (l < 1e-12) return Vec3(0, 0, 1);
  return Vec3(v.x/l, v.y/l, v.z/l);
}
// the parametric lips surface is defined by two functions, one for the upper lip and one for the lower lip.
// World axes (same convention as the CBspheres examples read by ColladaParser):
//   +X right, +Y up, +Z toward the camera (pucker direction).
//
// Both lips meet along the mouth line y = -0.015 * (1 - u^2).

typedef Vec3 (*LipFunc)(double, double);

// u in [-1, 1] : left corner -> right corner of the mouth
// v in [ 0, 1] : mouth line  -> top of upper lip
static Vec3 upper_lip(double u, double v) {
  const double half_width = 0.80;
  const double top_height = 0.32;   
  const double pucker     = 0.20;   

  double s = std::max(0.0, 1.0 - u*u);   // lip-corner taper factor
  double x = u * half_width;

  // Top edge (skin border of the upper lip).
  // Smooth elliptical arch + a sharp Gaussian dip at u=0 -> the Cupid's bow.
  double arc   = top_height * std::pow(s, 0.55);
  double cupid = 0.08 * std::exp(-u*u / 0.008);
  double y_top = arc - cupid;

  // Bottom edge = the mouth line.  Slight downward curl so the two lips
  // meet on a gentle smile.
  double y_bot = -0.015 * s;

  double y = y_bot + v * (y_top - y_bot);

  // Forward pucker: max at the lip centerline (u=0) and mid-height (v=0.5),
  // zero at the corners and along both edges.
  double horiz = std::pow(s, 0.45);
  double vert  = 4.0 * v * (1.0 - v);
  double z     = pucker * horiz * vert;

  return Vec3(x, y, z);
}

// u in [-1, 1] : left corner -> right corner of the mouth
// v in [ 0, 1] : bottom of lower lip -> mouth line   (chosen so normals face +Z)
static Vec3 lower_lip(double u, double v) {
  const double half_width = 0.80;
  const double bot_height = 0.42;  
  const double pucker     = 0.26;   

  double s = std::max(0.0, 1.0 - u*u);
  double x = u * half_width;

  // Bottom edge: wide, fuller arc below the mouth line.
  double y_bot_edge = -bot_height * std::pow(s, 0.75);
  // Top edge: matches upper_lip's bottom edge exactly so the lips meet.
  double y_top_edge = -0.015 * s;

  double y = y_bot_edge + v * (y_top_edge - y_bot_edge);

  double horiz = std::pow(s, 0.40);
  double vert  = 4.0 * v * (1.0 - v);
  double z     = pucker * horiz * vert;

  return Vec3(x, y, z);
}

static Vec3 surface_normal(LipFunc f, double u, double v) {
  const double eps = 1e-3;
  double up = std::min( 0.999, u + eps);
  double um = std::max(-0.999, u - eps);
  double vp = std::min( 1.0,   v + eps);
  double vm = std::max( 0.0,   v - eps);

  Vec3 du = f(up, v) - f(um, v);
  Vec3 dv = f(u, vp) - f(u, vm);
  Vec3 n  = cross(du, dv);

  if (n.z < 0) n = n * -1.0;   
  return normalize(n);
}

//  tessellation 
// Samples (nu+1) x (nv+1) vertices and emits 2 * nu * nv triangles.
// CCW winding order when viewed from +Z, so the visible side is the
// camera-facing side.
static void tesselate(LipFunc f,
                      std::vector<Vec3>& verts,
                      std::vector<Vec3>& norms,
                      std::vector<int>&  tris,
                      int nu, int nv) {
  int base = (int)verts.size();

  const double u_min = -0.99, u_max = 0.99;

  for (int i = 0; i <= nu; i++) {
    double u = u_min + (u_max - u_min) * double(i) / nu;
    for (int j = 0; j <= nv; j++) {
      double v = double(j) / nv;
      verts.push_back(f(u, v));
      norms.push_back(surface_normal(f, u, v));
    }
  }

  const int stride = nv + 1;
  for (int i = 0; i < nu; i++) {
    for (int j = 0; j < nv; j++) {
      int p00 = base + (i  ) * stride + (j  );
      int p01 = base + (i  ) * stride + (j+1);
      int p10 = base + (i+1) * stride + (j  );
      int p11 = base + (i+1) * stride + (j+1);
      // Two CCW triangles per quad.
      tris.push_back(p00); tris.push_back(p10); tris.push_back(p01);
      tris.push_back(p01); tris.push_back(p10); tris.push_back(p11);
    }
  }
}

// the  main 
int main(int argc, char** argv) {
  std::string outpath = "dae/final-project/lips1.dae";
  if (argc > 1) outpath = argv[1];

  std::vector<Vec3> verts, norms;
  std::vector<int>  tris;

  const int NU = 60;   // samples across the width
  const int NV = 24;   // samples from edge to mouth line
  tesselate(upper_lip, verts, norms, tris, NU, NV);
  tesselate(lower_lip, verts, norms, tris, NU, NV);

  std::ofstream out(outpath);
  if (!out) {
    std::cerr << "ERROR: could not open " << outpath << " for writing\n";
    return 1;
  }
  out << std::fixed << std::setprecision(6);

  // scene header, camera, two lights, lip-gloss material 
  out <<
R"(<?xml version="1.0" encoding="utf-8"?>
<COLLADA xmlns="http://www.collada.org/2005/11/COLLADASchema" version="1.4.1">
  <asset>
    <unit name="meter" meter="1"/>
    <up_axis>Y_UP</up_axis>
  </asset>
  <library_cameras>
    <camera id="Camera-camera" name="Camera">
      <optics><technique_common><perspective>
        <xfov sid="xfov">40</xfov>
        <aspect_ratio>1.333333</aspect_ratio>
        <znear sid="znear">0.1</znear>
        <zfar  sid="zfar">100</zfar>
      </perspective></technique_common></optics>
    </camera>
  </library_cameras>
  <library_lights>
    <light id="KeyLight-light" name="KeyLight">
      <technique_common><point>
        <color sid="color">18 18 18</color>
        <constant_attenuation>1</constant_attenuation>
        <linear_attenuation>0</linear_attenuation>
        <quadratic_attenuation>0.25</quadratic_attenuation>
      </point></technique_common>
    </light>
    <light id="FillLight-light" name="FillLight">
      <technique_common><point>
        <color sid="color">6 6 7</color>
        <constant_attenuation>1</constant_attenuation>
        <linear_attenuation>0</linear_attenuation>
        <quadratic_attenuation>0.25</quadratic_attenuation>
      </point></technique_common>
    </light>
  </library_lights>
  <library_effects>
    <effect id="lips-effect">
      <profile_COMMON>
        <technique sid="common">
          <phong>
            <emission><color sid="emission">0 0 0 1</color></emission>
            <ambient ><color sid="ambient" >0 0 0 1</color></ambient>
            <diffuse ><color sid="diffuse" >0.82 0.22 0.26 1</color></diffuse>
            <specular><color sid="specular">0.1 0.1 0.1 1</color></specular>
            <shininess><float sid="shininess">1</float></shininess>
            <index_of_refraction><float sid="index_of_refraction">1</float></index_of_refraction>
          </phong>
        </technique>
      </profile_COMMON>
      <extra>
        <technique profile="CGL">
          <layered>
            <roughness>0.08</roughness>
            <thickness>0.6</thickness>
            <base_color>0.82 0.22 0.26</base_color>
            <saturation>1.2</saturation>
            <ior>1.5</ior>
          </layered>
        </technique>
      </extra>
    </effect>
  </library_effects>
  <library_materials>
    <material id="lips-material" name="lips_material">
      <instance_effect url="#lips-effect"/>
    </material>
  </library_materials>
  <library_geometries>
    <geometry id="lips-mesh" name="lips">
      <mesh>
)";

  // the positions 
  out << "        <source id=\"lips-positions\">\n"
      << "          <float_array id=\"lips-positions-array\" count=\""
      << verts.size()*3 << "\">";
  for (auto& v : verts) out << " " << v.x << " " << v.y << " " << v.z;
  out << "</float_array>\n"
      << "          <technique_common><accessor source=\"#lips-positions-array\" count=\""
      << verts.size() << "\" stride=\"3\">\n"
      << "            <param name=\"X\" type=\"float\"/>"
         "<param name=\"Y\" type=\"float\"/>"
         "<param name=\"Z\" type=\"float\"/>\n"
      << "          </accessor></technique_common>\n"
      << "        </source>\n";

  // normal lips 
  out << "        <source id=\"lips-normals\">\n"
      << "          <float_array id=\"lips-normals-array\" count=\""
      << norms.size()*3 << "\">";
  for (auto& n : norms) out << " " << n.x << " " << n.y << " " << n.z;
  out << "</float_array>\n"
      << "          <technique_common><accessor source=\"#lips-normals-array\" count=\""
      << norms.size() << "\" stride=\"3\">\n"
      << "            <param name=\"X\" type=\"float\"/>"
         "<param name=\"Y\" type=\"float\"/>"
         "<param name=\"Z\" type=\"float\"/>\n"
      << "          </accessor></technique_common>\n"
      << "        </source>\n";

  // vertices + triangles (normals go on the <triangles> input, not here) 
  out << "        <vertices id=\"lips-vertices\">\n"
      << "          <input semantic=\"POSITION\" source=\"#lips-positions\"/>\n"
      << "        </vertices>\n";

  out << "        <triangles count=\"" << tris.size()/3
      << "\" material=\"lips-material\">\n"
      << "          <input semantic=\"VERTEX\" source=\"#lips-vertices\" offset=\"0\"/>\n"
      << "          <input semantic=\"NORMAL\" source=\"#lips-normals\"  offset=\"1\"/>\n"
      << "          <p>";
  // Vertex-index and normal-index happen to coincide (built 1:1 above).
  for (int idx : tris) out << " " << idx << " " << idx;
  out << "</p>\n"
      << "        </triangles>\n";

  //  scene graph 
  out <<
R"(      </mesh>
    </geometry>
  </library_geometries>
  <library_visual_scenes>
    <visual_scene id="Scene" name="Scene">
      <node id="Camera" name="Camera" type="NODE">
        <matrix sid="transform">1 0 0 0  0 1 0 -0.05  0 0 1 2.2  0 0 0 1</matrix>
        <instance_camera url="#Camera-camera"/>
      </node>
      <node id="KeyLight" name="KeyLight" type="NODE">
        <matrix sid="transform">1 0 0 1.2  0 1 0 1.3  0 0 1 2.0  0 0 0 1</matrix>
        <instance_light url="#KeyLight-light"/>
      </node>
      <node id="FillLight" name="FillLight" type="NODE">
        <matrix sid="transform">1 0 0 -1.5  0 1 0 0.3  0 0 1 1.8  0 0 0 1</matrix>
        <instance_light url="#FillLight-light"/>
      </node>
      <node id="Lips" name="Lips" type="NODE">
        <matrix sid="transform">1 0 0 0  0 1 0 0  0 0 1 0  0 0 0 1</matrix>
        <instance_geometry url="#lips-mesh">
          <bind_material><technique_common>
            <instance_material symbol="lips-material" target="#lips-material"/>
          </technique_common></bind_material>
        </instance_geometry>
      </node>
    </visual_scene>
  </library_visual_scenes>
  <scene><instance_visual_scene url="#Scene"/></scene>
</COLLADA>
)";

  out.close();
  std::cout << "Wrote " << outpath << ": "
            << verts.size() << " verts, "
            << tris.size()/3 << " triangles\n";
  return 0;
}