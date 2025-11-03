#ifndef ITU_RESOURCE_STORAGE_HPP
#define ITU_RESOURCE_STORAGE_HPP

#ifndef ITU_UNITY_BUILD
#include <itu_engine.hpp>
#endif

struct Model3D;

// TODO find appropriate place for this
struct RawTextureData
{
	void* texture;
	int width;
	int height;
	int channels;
};

typedef Uint32 ITU_IdTexture;
typedef Uint32 ITU_IdRawTexture;
typedef Uint32 ITU_IdAudio;
typedef Uint32 ITU_IdFont;
typedef Uint32 ITU_IdModel3D;

ITU_IdTexture itu_sys_rstorage_texture_load(SDLContext* context, const char* path, SDL_ScaleMode mode);
ITU_IdTexture itu_sys_rstorage_texture_add(SDL_Texture* texture);
ITU_IdTexture itu_sys_rstorage_texture_from_ptr(SDL_Texture* texture);
SDL_Texture*  itu_sys_rstorage_texture_get_ptr(ITU_IdTexture id);
void          itu_sys_rstorage_texture_set_debug_name(ITU_IdTexture id, const char* debug_name);
const char*   itu_sys_rstorage_texture_get_debug_name(ITU_IdTexture id);

ITU_IdRawTexture itu_sys_rstorage_raw_texture_load(SDLContext* context, const char* path);
ITU_IdRawTexture itu_sys_rstorage_raw_texture_add(void* pixel_data, int width, int height, int channels);
ITU_IdRawTexture itu_sys_rstorage_raw_texture_from_ptr(void* pixel_data);
RawTextureData*  itu_sys_rstorage_raw_texture_get_ptr(ITU_IdRawTexture id);
void             itu_sys_rstorage_raw_texture_set_debug_name(ITU_IdRawTexture id, const char* debug_name);
const char*      itu_sys_rstorage_raw_texture_get_debug_name(ITU_IdRawTexture id);

ITU_IdFont  itu_sys_rstorage_font_load(SDLContext* context, const char* path, float size);
ITU_IdFont  itu_sys_rstorage_font_add(TTF_Font* font);
ITU_IdFont  itu_sys_rstorage_font_from_ptr(TTF_Font* font);
TTF_Font*   itu_sys_rstorage_font_get_ptr(ITU_IdFont id);
void        itu_sys_rstorage_font_set_debug_name(ITU_IdFont id, const char* debug_name);
const char* itu_sys_rstorage_font_get_debug_name(ITU_IdFont id);

ITU_IdModel3D  itu_sys_rstorage_model3d_load(SDLContext* context, const char* path);
ITU_IdModel3D  itu_sys_rstorage_model3d_add(Model3D* data);
ITU_IdModel3D  itu_sys_rstorage_model3d_from_ptr(Model3D* data);
Model3D*   itu_sys_rstorage_model3d_get_ptr(ITU_IdModel3D id);
void        itu_sys_rstorage_model3d_set_debug_name(ITU_IdModel3D id, const char* debug_name);
const char* itu_sys_rstorage_model3d_get_debug_name(ITU_IdModel3D id);

void itu_sys_rstorage_debug_render(SDLContext* context);
bool itu_sys_rstorage_debug_render_font(TTF_Font* font, TTF_Font** new_font);
bool itu_sys_rstorage_debug_render_texture(SDL_Texture* texture, SDL_Texture** new_texture, SDL_FRect* rect);
bool itu_sys_rstorage_debug_render_raw_texture(void* raw_texture, void** new_raw_texture);
bool itu_sys_rstorage_debug_render_model3d(Model3D* mesh, Model3D** new_mesh);

#endif // ITU_RESOURCE_STORAGE_HPP