#ifndef TERRAIN3D_TIFF_H
#define TERRAIN3D_TIFF_H

#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/classes/image.hpp>

#include <tiffio.h>

using namespace godot;

class TiffIO {
public:
    static Error save_to_file(const String &file_name, Ref<Image> img);
    static Ref<Image> load_from_file(const String &file_name);
};

#endif // TERRAIN3D_TIFF_H
