// Filename: textureReference.h
// Created by:  drose (28Nov00)
// 
////////////////////////////////////////////////////////////////////

#ifndef TEXTUREREFERENCE_H
#define TEXTUREREFERENCE_H

#include <pandatoolbase.h>

#include "textureProperties.h"

#include <luse.h>
#include <typedWriteable.h>

class TextureImage;
class SourceTextureImage;
class Filename;
class EggFile;
class EggData;
class EggTexture;
class EggGroupNode;
class EggPrimitive;
class TexturePlacement;

////////////////////////////////////////////////////////////////////
// 	 Class : TextureReference
// Description : This is the particular reference of a texture
//               filename by an egg file.  It also includes
//               information about the way in which the egg file uses
//               the texture; e.g. does it repeat.
////////////////////////////////////////////////////////////////////
class TextureReference : public TypedWriteable {
public:
  TextureReference();
  ~TextureReference();

  void from_egg(EggFile *egg_file, EggData *data, EggTexture *egg_tex);

  EggFile *get_egg_file() const;
  SourceTextureImage *get_source() const;
  TextureImage *get_texture() const;

  bool has_uvs() const;
  const TexCoordd &get_min_uv() const;
  const TexCoordd &get_max_uv() const;

  EggTexture::WrapMode get_wrap_u() const;
  EggTexture::WrapMode get_wrap_v() const;

  void set_placement(TexturePlacement *placement);
  void clear_placement();
  TexturePlacement *get_placement() const;

  void mark_egg_stale();
  void update_egg();

  void output(ostream &out) const;
  void write(ostream &out, int indent_level = 0) const;

private:
  void get_uv_range(EggGroupNode *group);
  void update_uv_range(EggGroupNode *group);
  
  bool get_geom_uvs(EggPrimitive *geom,
		    TexCoordd &geom_min_uv, TexCoordd &geom_max_uv);
  void translate_geom_uvs(EggPrimitive *geom, const TexCoordd &trans) const;
  static void collect_uv(bool &any_uvs, TexCoordd &min_uv, TexCoordd &max_uv,
			 const TexCoordd &got_min_uv,
			 const TexCoordd &got_max_uv);
  static LVector2d translate_uv(const TexCoordd &min_uv,
				const TexCoordd &max_uv);

  EggFile *_egg_file;
  EggTexture *_egg_tex;
  EggData *_egg_data;

  LMatrix3d _tex_mat, _inv_tex_mat;
  SourceTextureImage *_source_texture;
  TexturePlacement *_placement;

  bool _uses_alpha;

  bool _any_uvs;
  TexCoordd _min_uv, _max_uv;
  EggTexture::WrapMode _wrap_u, _wrap_v;

  TextureProperties _properties;

  // The TypedWriteable interface follows.
public:
  static void register_with_read_factory();
  virtual void write_datagram(BamWriter *writer, Datagram &datagram); 
  virtual int complete_pointers(vector_typedWriteable &plist, 
				BamReader *manager);

protected:
  static TypedWriteable *make_TextureReference(const FactoryParams &params);
  void fillin(DatagramIterator &scan, BamReader *manager);

public:
  static TypeHandle get_class_type() {
    return _type_handle;
  }
  static void init_type() {
    TypedWriteable::init_type();
    register_type(_type_handle, "TextureReference",
		  TypedWriteable::get_class_type());
  }
  virtual TypeHandle get_type() const {
    return get_class_type();
  }

private:
  static TypeHandle _type_handle;
};

INLINE ostream &
operator << (ostream &out, const TextureReference &ref) {
  ref.output(out);
  return out;
}

#endif

