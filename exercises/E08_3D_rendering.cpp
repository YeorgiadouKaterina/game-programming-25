#define TEXTURE_PIXELS_PER_UNIT 128 // how many pixels of textures will be mapped to a single world unit
#define CAMERA_PIXELS_PER_UNIT  32  // how many pixels of windows will be used to render a single world unit

#define ENABLE_DIAGNOSTICS

// rendering framerate
#define TARGET_FRAMERATE_NS     (SECONDS(1) / 60)

// physics timestep
#define PHYSICS_TIMESTEP_NSECS  (SECONDS(1) / 60)
#define PHYSICS_TIMESTEP_SECS   NS_TO_SECONDS(PHYSICS_TIMESTEP_NSECS)
#define PHYSICS_MAX_TIMESTEPS_PER_FRAME 4
#define PHYSICS_MAX_CONTACTS_PER_ENTITY 16

#define WINDOW_W         800
#define WINDOW_H         600

#include <itu_unity_include.hpp>

void ex8_system_update_debug_camera(SDLContext* context, ITU_EntityId* entities, int entities_count)
{
	// TMP only supporting one camera at the moment
	//     probably works if we have multiple and we only do the first,
	//     but it might break if entity order becomes unstable for any reason
	if(entities_count != 1)
		return;


	Transform3D*     transform  = entity_get_data(entities[0], Transform3D);
	CameraComponent* camera     = entity_get_data(entities[0], CameraComponent);
	
	// debug camera controls (should they go somehwere else?)
	{
		glm::vec3 local_camera_pos = glm::vec3(0.0f);
		glm::vec3 local_camera_rot = glm::vec3(0.0f);
		const float SPEED_PAN = 2.0f;
		const float SPEED_MOV = 2.0f;
		const float SPEED_ROT = 0.1f;
		const float THRESHOLD_PAN = 5.0f;
		const float THRESHOLD_ROT = 5.0f;
		bool dirty = false;
		if(context->btn_isdown[BTN_TYPE_UI_EXTRA])
		{
			if (SDL_fabsf(context->mouse_delta.y) > THRESHOLD_ROT) local_camera_rot.x += context->mouse_delta.y * SPEED_ROT;
			if (SDL_fabsf(context->mouse_delta.x) > THRESHOLD_ROT) local_camera_rot.y += context->mouse_delta.x * SPEED_ROT;
			dirty = true;
		}

		if (context->btn_isdown[BTN_TYPE_UP])       { local_camera_pos.z -= SPEED_MOV; dirty = true; }
		if (context->btn_isdown[BTN_TYPE_DOWN])     { local_camera_pos.z += SPEED_MOV; dirty = true; }
		if (context->btn_isdown[BTN_TYPE_LEFT])     { local_camera_pos.x -= SPEED_MOV; dirty = true; }
		if (context->btn_isdown[BTN_TYPE_RIGHT])    { local_camera_pos.x += SPEED_MOV; dirty = true; }
		if (context->btn_isdown[BTN_TYPE_ACTION_0]) { local_camera_pos.y -= SPEED_MOV; dirty = true; }
		if (context->btn_isdown[BTN_TYPE_ACTION_1]) { local_camera_pos.y += SPEED_MOV; dirty = true; }

		if(dirty)
		{
			local_camera_pos *= context->delta;
			local_camera_rot *= context->delta;

			// specific 3D camera movement for editor (mixes local and global transformation)
			glm::mat4 r = transform->local;
			glm::vec4 t = transform->local[3];
			r[3] -= t;

			transform->local = glm::eulerAngleY(local_camera_rot.y) * r * glm::eulerAngleX(local_camera_rot.x);
			transform->local[3] = t;
			transform->local = transform->local * glm::translate(local_camera_pos);
		}
	}
	itu_sys_render3d_set_camera_view(glm::inverse(transform->global));

	// TODO: this is extremely wasteful. Set the projection matrix only if something requires it to change
	glm::mat4 proj;
	switch(camera->type)
	{
		case CAMERA_TYPE_ORTHOGRAPHIC:
		{
			float aspect = context->window_w / context->window_h;
			float z_depth_half = (camera->far - camera->near) / 2.0f;
			proj = glm::ortho(-aspect * camera->zoom, aspect * camera->zoom, -camera->zoom, +camera->zoom, -z_depth_half, z_depth_half);
			break;
		}
		case CAMERA_TYPE_PERSPECTIVE:
		{
			proj = glm::perspective(camera->fow, context->window_w / context->window_h, camera->near, camera->far);
			break;
		}
		default:
		{
			SDL_Log("ERROR invalid camera type!");
			return;
		}
	}
	itu_sys_render3d_set_camera_proj(proj);
}

void ex8_system_mesh_render(SDLContext* context, ITU_EntityId* entities, int entities_count)
{
	for(int i = 0; i < entities_count; ++i)
	{
		ITU_EntityId id = entities[i];
		Transform3D* transform = entity_get_data(id, Transform3D); 
		MeshComponent* mesh = entity_get_data(id, MeshComponent);

		Model3D* model3d = itu_sys_rstorage_model3d_get_ptr(mesh->id_model3d);

		InstanceData data;
		data.transform_global = transform->global;
		itu_sys_render3d_model3d_render(context, model3d, data);
	}
}

static void game_init(SDLContext* context)
{
	itu_sys_rstorage_model3d_load(context, "data/kenney/meshes/vehicle-truck.obj");
	itu_sys_rstorage_model3d_load(context, "data/kenney/meshes/gate.obj");
	itu_sys_rstorage_model3d_load(context, "data/kenney/meshes/floor-small-square.obj");
}

static void game_reset(SDLContext* context)
{
	ITU_EntityId id_parent = ITU_ENTITY_ID_NULL;

	// camera
	{
		ITU_EntityId id = itu_entity_create();
		Transform3D* transform = transform3D_add(id, ITU_ENTITY_ID_NULL);

		transform->local = glm::translate(glm::vec3(0.0f, 5.0f, 0.0f)) * glm::eulerAngleX(-PI_HALF);

		CameraComponent camera = { };
		camera.type = CAMERA_TYPE_PERSPECTIVE;
		camera.fow  = 45.0f;
		camera.zoom =  1.0f;
		camera.near =  0.1f;
		camera.far  = 50.0f;
		entity_add_component(id, CameraComponent, camera);
		itu_entity_set_debug_name(id, "camera");
	}
}

int main(void)
{
	bool quit = false;
	SDLContext context = { 0 };

	context.window_w = WINDOW_W;
	context.window_h = WINDOW_H;
	context.zoom = 1;

	context.working_dir = SDL_GetCurrentDirectory();
	context.window = SDL_CreateWindow("E08 - 3D Rendering", WINDOW_W, WINDOW_H, 0);

	itu_sys_estorage_init(512, false);
	enable_component(Transform3D);
	enable_component(MeshComponent);
	enable_component(CameraComponent);
	add_component_debug_ui_render(Transform3D, itu_debug_ui_render_transform3D);
	add_component_debug_ui_render(MeshComponent, itu_debug_ui_render_meshcomponent);
	add_component_debug_ui_render(CameraComponent, itu_debug_ui_render_cameracomponent);
	add_system(ex8_system_update_debug_camera, component_mask(Transform3D)|component_mask(CameraComponent), 0);
	add_system(ex8_system_mesh_render, component_mask(Transform3D)|component_mask(MeshComponent), 0);

	itu_sys_render3d_init(&context);

	itu_lib_imgui_setup(&context, true);
	
	context.camera_default.normalized_screen_size.x = 1.0f;
	context.camera_default.normalized_screen_size.y = 1.0f;
	context.camera_default.normalized_screen_offset.x = 0.0f;
	context.camera_default.zoom = 1;
	context.camera_default.pixels_per_unit = CAMERA_PIXELS_PER_UNIT;
	camera_set_active(&context, &context.camera_default);

	// set degu UI shown by default (new and shiny, let's showcase it)
	context.debug_ui_show = true;

	game_init(&context);
	game_reset(&context);

	SDL_Time walltime_frame_beg;
	SDL_Time walltime_frame_end;
	SDL_Time walltime_work_end;
	SDL_Time elapsed_work = 0;
	SDL_Time elapsed_frame = 0;

	SDL_GetCurrentTime(&walltime_frame_beg);
	walltime_frame_end = walltime_frame_beg;

	sdl_input_set_mapping_keyboard(&context, SDLK_W,     BTN_TYPE_UP);
	sdl_input_set_mapping_keyboard(&context, SDLK_A,     BTN_TYPE_LEFT);
	sdl_input_set_mapping_keyboard(&context, SDLK_S,     BTN_TYPE_DOWN);
	sdl_input_set_mapping_keyboard(&context, SDLK_D,     BTN_TYPE_RIGHT);
	sdl_input_set_mapping_keyboard(&context, SDLK_Q,     BTN_TYPE_ACTION_0);
	sdl_input_set_mapping_keyboard(&context, SDLK_E,     BTN_TYPE_ACTION_1);
	sdl_input_set_mapping_keyboard(&context, SDLK_SPACE, BTN_TYPE_SPACE);

	sdl_input_set_mapping_mouse(&context, 1, BTN_TYPE_UI_SELECT);
	sdl_input_set_mapping_mouse(&context, 3, BTN_TYPE_UI_EXTRA);

	while(!quit)
	{
		quit = sdl_process_events(&context);
		
		itu_lib_imgui_frame_begin();
		itu_sys_render3d_frame_begin(&context);

		// update
		itu_sys_estorage_systems_update(&context);

		// after gameplay is done, update global transforms for rendering
		itu_lib_transform_update_globals();

		// TMP TODO FIXME remove before shipping
		ImGui::Begin("Debug UI", &context.debug_ui_show, 0);
		{
			ImGui::DragFloat("k_ambient", &ctx_rendering.uniform_data_global.k_ambient);
			ImGui::DragFloat("k_diffuse", &ctx_rendering.uniform_data_global.k_diffuse);
			ImGui::DragFloat("k_specular", &ctx_rendering.uniform_data_global.k_specular);
			ImGui::DragFloat("exp_specular", &ctx_rendering.uniform_data_global.exp_specular);
		
			ImGui::DragFloat4("light_direction", &ctx_rendering.uniform_data_global.light_direction.x);
			ImGui::DragFloat4("light_color", &ctx_rendering.uniform_data_global.light_color.x);
			ImGui::DragFloat4("light_color_ambient", &ctx_rendering.uniform_data_global.light_color_ambient.x);
		}
		ImGui::End();


#ifdef ENABLE_DIAGNOSTICS
		{
			if(context.debug_ui_show)
			{
				ImGui::Begin("Debug UI", &context.debug_ui_show, 0);
				{
					ImGui::BeginTabBar("debug_ui_tab");
					if(ImGui::BeginTabItem("Context"))
					{
						ImGui::Text("Timing");
						ImGui::LabelText("work", "%6.3f ms/f", (float)elapsed_work  / (float)MILLIS(1));
						ImGui::LabelText("tot",  "%6.3f ms/f", (float)elapsed_frame / (float)MILLIS(1));
						ImGui::LabelText("physics steps",  "%d", context.physics_steps_count);
		
						ImGui::EndTabItem();
					}
					if(ImGui::BeginTabItem("Entities"))
					{
						itu_sys_estorage_debug_render(&context);
						ImGui::EndTabItem();
					}
					if(ImGui::BeginTabItem("Resources"))
					{
						itu_sys_rstorage_debug_render(&context);
						ImGui::EndTabItem();
					}
		
					ImGui::EndTabBar();
				}
				ImGui::End();
			}
		}
#endif
		
		ctx_rendering.command_buffer_render = SDL_AcquireGPUCommandBuffer(ctx_rendering.device);

		itu_sys_render3d_frame_end(&context);
		itu_lib_imgui_frame_end(&context);

		
		VALIDATE_PANIC(SDL_SubmitGPUCommandBuffer(ctx_rendering.command_buffer_render));

		SDL_GetCurrentTime(&walltime_work_end);
		elapsed_work = walltime_work_end - walltime_frame_beg;

		if(elapsed_work < TARGET_FRAMERATE_NS)
			SDL_DelayNS(TARGET_FRAMERATE_NS - elapsed_work);

		SDL_GetCurrentTime(&walltime_frame_end);
		elapsed_frame = walltime_frame_end - walltime_frame_beg;

		context.delta = (float)elapsed_frame / (float)SECONDS(1);
		context.uptime += context.delta;
		context.elapsed_frame = elapsed_frame;
		walltime_frame_beg = walltime_frame_end;
	}
}
