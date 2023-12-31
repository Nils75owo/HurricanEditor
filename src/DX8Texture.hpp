// DKS - DX8Texture.hpp is a new file I have added.
//      It contains a new texture system class that allows sprites to share textures,
//      where before the game was loading duplicate copies in places. It also
//      abstracts away the details like the alpha textures needed by ETC1 compression.
//      It also unifies the interface that was once different between DX8 and SDL.
#ifndef _DX8TEXTURE_HPP_
#define _DX8TEXTURE_HPP_

#include <map>
#include <string>
#include "SDLPort/SDL_port.hpp"
#include "Globals.hpp"

class TextureHandle {
  public:
    TextureHandle()
        : tex(0),
#if defined(USE_ETC1)
          alphatex(0),  // ETC1 requires a separate texture just for alpha channel
#endif
          instances(0),
          npot_scalex(1.0),
          npot_scaley(1.0) {
    }

    LPDIRECT3DTEXTURE8 tex;
#if defined(USE_ETC1)
    LPDIRECT3DTEXTURE8 alphatex;  // ETC1 compression requires a second texture just for alpha-channel
#endif

    int32_t instances;   // Number of times this texture file has been loaded,
                         //   if the value is 1, it's OK to unload the texture.
    double npot_scalex,  // When a texture is loaded into VRAM, it sometimes
        npot_scaley;     //   must be resized to the nearest-power-of-two, and
                         //   these factors correct for this.
};

class TexturesystemClass {
  public:
    TexturesystemClass() {}
    ~TexturesystemClass() {}
    void Exit();
    int16_t LoadTexture(const std::string &filename);
    void UnloadTexture(const int idx);

    TextureHandle &operator[](int idx) {
#ifndef NDEBUG
        if (idx < 0 || idx >= static_cast<int>(_loaded_textures.size())) {
            Protokoll << "-> Error: Out of bounds index for Texturesystemclass::operator[]: " << std::dec << idx << "\n"
                      << "\tLower bound is 0, Upper bound is " << _loaded_textures.size() - 1 << std::endl;
            GameRunning = false;
        }
#endif
        return _loaded_textures[idx];
    }

  private:
    std::vector<TextureHandle> _loaded_textures;
    std::map<std::string, int16_t> _texture_map;  // Anytime a texture is loaded from a file, its filename is
                                                  //  inserted into this container, mapping it to its index
                                                  //  into the _loaded_textures vector. This way, we can share
                                                  //  textures between sprites.

    // DKS - Hurrican will now look for a file named "scalefactors.txt" in the
    //       directory it is reading textures from. The file is generated
    //       by the texture-utility-script provided with the source code.
    //       This is necessary because Hurrican now supports compressed
    //       texture formats like ETC1 and PVR. Because the images must be
    //       expanded to nearest-power-of-two dimensions on some platforms
    //       before being compressed and stored, Hurrican would otherwise have
    //       no way of knowing how to convert in-game texture coordinates to
    //       coordinates of the textures as they exist on-disk & thus in VRAM.
    //
    //       On each line of the file, there are three fields separated by whitespace:
    //       FILENAME X_SCALE Y_SCALE
    //
    //       * Field FILENAME is the name of the image file, excluding any extension.
    //
    //       * Fields X_SCALE and Y_SCALE are floating-point scale factors that,
    //         when multiplied against an in-game texture coordinate between 0.0-1.0,
    //         will produce a texture coordinate matching the actual
    //         texture in VRAM that might have been expanded to npot dimensions.
    //
    //       *NOTE: if the script did not need to resize an image because it
    //         already had NPOT dimensions, it will not create an entry in
    //         scalefactors.txt
    static const std::string scalefactors_filename;
    std::map<std::string, std::pair<double, double> > _scalefactors_map;
    void ReadScaleFactorsFile(const std::string &fullpath);

    bool LoadTextureFromFile(const std::string &filename, TextureHandle &th);
};

// EXTERNS:
extern TexturesystemClass Textures;

#endif  // _DX8TEXTURE_HPP_
