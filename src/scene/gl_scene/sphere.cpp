#include "sphere.h"

#include "scene/object.h"
#include "util/sphere_drawing.h"

#include "application/visual_debugger.h"

#include "pathtracer/bsdf.h"

namespace CGL { namespace GLScene {

namespace {

std::string make_material_key(const Collada::MaterialInfo* material) {
  if (!material) return "";
  if (!material->id.empty()) return material->id;
  return material->name;
}

std::string make_material_label(const Collada::MaterialInfo* material) {
  if (!material) return "";
  if (!material->name.empty() && !material->id.empty()) {
    return material->name + " (" + material->id + ")";
  }
  if (!material->name.empty()) return material->name;
  return material->id;
}

} // namespace

Sphere::Sphere(const Collada::SphereInfo& info, 
               const Vector3D position, const double scale) : 
  p(position), r(info.radius * scale) { 
  if (info.material) {
    bsdf = info.material->bsdf;
    material_key = make_material_key(info.material);
    material_label = make_material_label(info.material);
  } else {
    bsdf = new DiffuseBSDF(Vector3D(0.5f,0.5f,0.5f));    
  }
}

void Sphere::set_draw_styles(DrawStyle *defaultStyle, DrawStyle *hoveredStyle,
                             DrawStyle *selectedStyle) {
  style = defaultStyle;
}

void Sphere::render_in_opengl() const {
  Misc::draw_sphere_opengl(p, r);
}

void Sphere::render_debugger_node()
{
  if (ImGui::TreeNode(this, "Sphere"))
  {
    DragDouble("Radius", &r, 0.005);
    DragDouble3("Position", &p[0], 0.005);

    if (bsdf) bsdf->render_debugger_node();

    ImGui::TreePop();
  }
}

BBox Sphere::get_bbox() {
  return BBox(p.x - r, p.y - r, p.z - r, p.x + r, p.y + r, p.z + r);
}

BSDF* Sphere::get_bsdf() {
  return bsdf;
}

void Sphere::set_bsdf(BSDF* next_bsdf) {
  bsdf = next_bsdf;
}

std::string Sphere::get_material_key() const {
  return material_key;
}

std::string Sphere::get_material_label() const {
  return material_label;
}

SceneObjects::SceneObject *Sphere::get_static_object() {
  return new SceneObjects::SphereObject(p, r, bsdf);
}

} // namespace GLScene
} // namespace CGL
