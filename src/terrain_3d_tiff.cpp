#ifdef HAVE_LIBTIFF
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include "terrain_3d_tiff.h"

// Define class name for LOG macro
static const char* __class__ = "TiffIO";

Error TiffIO::save_to_file(const String &file_name, Ref<Image> img) {
    if (img.is_null() || img->is_empty()) {
        UtilityFunctions::push_error("TiffIO: Invalid image provided for TIFF export");
        return ERR_INVALID_DATA;
    }
    
    const int width = img->get_width();
    const int height = img->get_height();
    
    if (width <= 0 || height <= 0) {
        UtilityFunctions::push_error("TiffIO: Invalid image dimensions: ", width, "x", height);
        return ERR_INVALID_DATA;
    }

    // Convert Godot String to C string
    CharString file_path = file_name.utf8();
    
    // Try to open TIFF file - this will fail if libtiff DLL is missing
    TIFF* tiff = TIFFOpen(file_path.get_data(), "w");
    if (!tiff) {
        UtilityFunctions::push_error("TiffIO: Failed to open TIFF file for writing: ", file_name);
        return ERR_CANT_OPEN;
    }

    // Get image data
    PackedByteArray data = img->get_data();
    const float* pixel_data = reinterpret_cast<const float*>(data.ptr());

    // Set basic TIFF tags
    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 1);
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 32);
    TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_IEEEFP);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, height); // Single strip
    TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);

    // Write image data row by row
    const int bytes_per_row = width * sizeof(float);
    for (int y = 0; y < height; y++) {
        const void* row_data = &pixel_data[y * width];
        if (TIFFWriteScanline(tiff, const_cast<void*>(row_data), y) < 0) {
            UtilityFunctions::push_error("TiffIO: Failed to write TIFF scanline ", y);
            TIFFClose(tiff);
            return ERR_CANT_CREATE;
        }
    }

    // Close the TIFF file
    TIFFClose(tiff);
    
    UtilityFunctions::print("TiffIO: Successfully exported ", width, "x", height, " TIFF to: ", file_name);
    return OK;
}

Ref<Image> TiffIO::load_from_file(const String &file_name) {
    CharString file_path = file_name.utf8();
    TIFF* tiff = TIFFOpen(file_path.get_data(), "r");
    if (!tiff) {
        UtilityFunctions::push_error("TiffIO: Failed to open TIFF for reading: ", file_name);
        return Ref<Image>();
    }

    uint32 width = 0, height = 0;
    uint16 samples_per_pixel = 0;
    uint16 bits_per_sample = 0;
    uint16 sample_format = SAMPLEFORMAT_UINT; // default
    TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height);
    TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);
    TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    TIFFGetField(tiff, TIFFTAG_SAMPLEFORMAT, &sample_format);

    if (width == 0 || height == 0) {
        UtilityFunctions::push_error("TiffIO: Invalid TIFF dimensions (libtiff)");
        TIFFClose(tiff);
        return Ref<Image>();
    }
    if (samples_per_pixel != 1 || bits_per_sample != 32 || sample_format != SAMPLEFORMAT_IEEEFP) {
        UtilityFunctions::push_error("TiffIO: Unsupported TIFF format (expecting 32-bit float, 1 channel)");
        TIFFClose(tiff);
        return Ref<Image>();
    }

    const size_t pixels = size_t(width) * size_t(height);
    PackedByteArray bytes;
    bytes.resize(pixels * sizeof(float));
    float *dst = reinterpret_cast<float*>(bytes.ptrw());

    // Read scanlines
    for (uint32 y = 0; y < height; ++y) {
        void *row_ptr = &dst[y * width];
        if (TIFFReadScanline(tiff, row_ptr, y) < 0) {
            UtilityFunctions::push_error("TiffIO: Failed reading TIFF scanline ", (int)y);
            TIFFClose(tiff);
            return Ref<Image>();
        }
    }
    TIFFClose(tiff);

    Ref<Image> image;
    image.instantiate();
    // Godot expects RF to be 32-bit float in red channel. We'll pack each float into R and set A=1.
    // But Image::create expects packed pixel bytes. Simpler: create FORMAT_RF directly.
    image->set_data(width, height, false, Image::FORMAT_RF, bytes);
    return image;
}
#endif
