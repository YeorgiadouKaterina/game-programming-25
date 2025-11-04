#ifndef ITU_LIB_IMGUI_HPP
#define ITU_LIB_IMGUI_HPP

#ifndef ITU_UNITY_BUILD
#include <itu_lib_engine.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui_impl_sdlrenderer3.h>
#include <imgui/imgui_impl_sdlgpu3.h>
#include <imgui/imgui_impl_sdlgpu3_shaders.h>
#endif

void itu_lib_imgui_setup(SDLContext* context, bool intercept_keyboard);
void itu_lib_imgui_setup(SDL_Window* window, SDLContext* context, bool intercept_keyboard);

// default imgui event handler. Call this before doing your own processing of the SDL event
// NOTE: a return value of `true` is a suggestion to skip processing of the event for the application
bool itu_lib_imgui_process_sdl_event(SDL_Event* event);
void itu_lib_imgui_frame_begin();
void itu_lib_imgui_frame_end(SDLContext* context);

// older function, the renderer is already present in SDLContext
// keeping it for back-compatibility with previous exercises
void itu_lib_imgui_setup(SDL_Window* window, SDLContext* context, bool intercept_keyboard)
{
	// old exercises did not have the window in the context. Patching it here
	context->window = window;
	itu_lib_imgui_setup(context, intercept_keyboard);
}

inline void itu_lib_imgui_setup(SDLContext* context, bool intercept_keyboard)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	io.FontDefault = io.Fonts->AddFontFromFileTTF("data\\Roboto-Medium.ttf", 16);
	//io.FontDefault = io.Fonts->AddFontFromFileTTF("data\\MSGOTHIC.TTC");
	if(intercept_keyboard)
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls

	// defaults to dark style (easier on both eyes and environment)
	ImGui::StyleColorsDark();

	// do some small tweaks
	ImGuiStyle& style = ImGui::GetStyle();
	style.Colors[ImGuiCol_FrameBg] = ImVec4 { 33/255.0f, 33/255.0f, 33/255.0f, 255/255.0f };
	style.FontScaleMain = context->zoom;

	// Setup Platform/Renderer backends
	// temporary logic: if GPU is set up use that, otherwise fallback to renderer

	if(ctx_rendering.device)
	{
		float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

		// Setup scaling
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
		style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)


		ImGui_ImplSDL3_InitForSDLGPU(context->window);
		ImGui_ImplSDLGPU3_InitInfo init_info = {};
		init_info.Device = ctx_rendering.device;
		init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(ctx_rendering.device, context->window);
		init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
		init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
		init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
		ImGui_ImplSDLGPU3_Init(&init_info);
	}
	else if(context->renderer)
	{
		ImGui_ImplSDL3_InitForSDLRenderer(context->window, context->renderer);
		ImGui_ImplSDLRenderer3_Init(context->renderer);
	}
	else
	{
		SDL_Log("cannot init imgui, neither SDL_Renderer nor SDL_GPUDevice are initialized!");
	}
}

inline bool itu_lib_imgui_process_sdl_event(SDL_Event* event)
{
	ImGui_ImplSDL3_ProcessEvent(event);

	// NOTE: if we are interacting with an imgui widget, we don't want the game to do weird stuff in the meantime
	//       (ie, tab to navigate controls would reset the game state, not great)
	if(event->type == SDL_EVENT_KEY_DOWN || event->type == SDL_EVENT_KEY_UP)
	{
		// TMP HACK TO CLOSE DEBUG UI EVEN WHEN IT HAS FOCUS
		if(event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_F1)
			return false;
		ImGuiIO& io = ImGui::GetIO();
		return io.WantCaptureKeyboard;
	}

	return false;
}

inline void itu_lib_imgui_frame_begin()
{
	if(ctx_rendering.device)
		ImGui_ImplSDLGPU3_NewFrame();
	else
		ImGui_ImplSDLRenderer3_NewFrame();

	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
}

inline void itu_lib_imgui_frame_end(SDLContext* context)
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::Render();
	ImDrawData* draw_data = ImGui::GetDrawData();

	if(ctx_rendering.device)
	{
		SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(ctx_rendering.device);

		if(ctx_rendering.color_texture)
		{
			ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

			// Setup and start a render pass
			SDL_GPUColorTargetInfo target_info = {};
			target_info.texture = ctx_rendering.color_texture;
			target_info.load_op = SDL_GPU_LOADOP_LOAD;
			target_info.store_op = SDL_GPU_STOREOP_STORE;
			target_info.mip_level = 0;
			target_info.layer_or_depth_plane = 0;
			target_info.cycle = false;
			SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, NULL);
			
			ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);
			SDL_EndGPURenderPass(render_pass);
		}
		SDL_SubmitGPUCommandBuffer(command_buffer);

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}
	}
	else
	{
		ImGui_ImplSDLRenderer3_RenderDrawData(draw_data, context->renderer);
	}
}
#endif // ITU_LIB_IMGUI_HPP