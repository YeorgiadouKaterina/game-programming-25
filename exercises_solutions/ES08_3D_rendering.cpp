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
enum ES8_Tag
{
	TAG_CAMERA_ACTIVE,
	TAG_CAMERA_DEBUG,
	TAG_ENTITY_CONTROLLED
};

struct ES8_PlayerData
{
	float speed_mov = 2.0f;
	float speed_rot = 0.1f;
	float threshold_rot = 5.0f;

	float camera_speed = 0.1f;
	float camera_rot_x_min = -PI_HALF;
	float camera_rot_x_max = +PI_HALF;
	glm::vec3 camera_rot;
	ITU_EntityId camera_ref;
};
register_component(ES8_PlayerData);

void ex8_system_update_active_camera(SDLContext* context, ITU_EntityId* entities, int entities_count)
{
	if(entities_count != 1)
		return;

	Transform3D*     transform  = entity_get_data(entities[0], Transform3D);
	CameraComponent* camera     = entity_get_data(entities[0], CameraComponent);

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

void ex8_system_update_player(SDLContext* context, ITU_EntityId* entities, int entities_count)
{
	// debug camera controls (should they go somehwere else?)
	for(int i = 0; i < entities_count; ++i)
	{
		ITU_EntityId id = entities[i];
		Transform3D*     transform        = entity_get_data(id, Transform3D);
		ES8_PlayerData*  data             = entity_get_data(id, ES8_PlayerData);
		Transform3D*     transform_camera = entity_get_data(data->camera_ref, Transform3D);

		float player_rot_x = 0;
		float player_rot_y = 0;
		glm::vec3 player_mov = glm::vec3(0.0f);
		
		// character movement
		if (context->btn_isdown[BTN_TYPE_UP])    
			player_mov.z -= data->speed_mov;
		if (context->btn_isdown[BTN_TYPE_DOWN])  
			player_mov.z += data->speed_mov;
		if (context->btn_isdown[BTN_TYPE_LEFT])  
			player_mov.x -= data->speed_mov;
		if (context->btn_isdown[BTN_TYPE_RIGHT]) 
			player_mov.x += data->speed_mov;

		// camera control
		if (SDL_fabsf(context->mouse_delta.y) > data->threshold_rot) player_rot_x -= context->mouse_delta.y * data->camera_speed;
		if (SDL_fabsf(context->mouse_delta.x) > data->threshold_rot) player_rot_y -= context->mouse_delta.x * data->camera_speed;

		{
			player_mov *= context->delta;
			player_rot_y *= context->delta;
			player_rot_x *= context->delta;

			// update player
			transform->local = transform->local  * glm::eulerAngleY(player_rot_y)* glm::translate(player_mov);

			// update camera
			data->camera_rot.x += player_rot_x;
			data->camera_rot.x = SDL_clamp(data->camera_rot.x, data->camera_rot_x_min, data->camera_rot_x_max);
			transform_camera->local = glm::translate((glm::vec3)transform_camera->local[3]) * glm::eulerAngleX(data->camera_rot.x);
		}
	}
}

void ex8_system_update_controls_debug(SDLContext* context, ITU_EntityId* entities, int entities_count)
{
	for(int i = 0; i < entities_count; ++i)
	{
		ITU_EntityId id = entities[i];
		Transform3D* transform  = entity_get_data(id, Transform3D);

		glm::vec3 local_camera_pos = glm::vec3(0.0f);
		glm::vec3 local_camera_rot = glm::vec3(0.0f);
		const float SPEED_MOV = 2.0f;
		const float SPEED_ROT = 0.1f;
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
}

static ITU_EntityId ref_player;
static ITU_EntityId ref_camera_gameplay;
static ITU_EntityId ref_camera_debug;

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

	set_tag_debug_name(TAG_CAMERA_ACTIVE    , "camera_active");
	set_tag_debug_name(TAG_CAMERA_DEBUG     , "camera_debug");
	set_tag_debug_name(TAG_ENTITY_CONTROLLED, "entity_controlled");
}

static void game_reset(SDLContext* context)
{
	ITU_EntityId id_parent = ITU_ENTITY_ID_NULL;

	// camera (debug)
	{
		ITU_EntityId id = itu_entity_create();
		ref_camera_debug = id;
		Transform3D* transform = transform3D_add(id, ITU_ENTITY_ID_NULL);

		transform->local = glm::translate(glm::vec3(0.0f, 5.0f, 0.0f)) * glm::eulerAngleX(-PI_HALF);

		CameraComponent camera = { };
		camera.type = CAMERA_TYPE_PERSPECTIVE;
		camera.fow  = 45.0f;
		camera.zoom =  1.0f;
		camera.near =  0.1f;
		camera.far  = 50.0f;
		entity_add_component(id, CameraComponent, camera);

		itu_entity_tag_add(id, TAG_CAMERA_DEBUG);
		itu_entity_set_debug_name(id, "camera_debug");
	}

	// camera
	{
		ITU_EntityId id = itu_entity_create();
		ref_camera_gameplay = id;
		Transform3D* transform = transform3D_add(id, ITU_ENTITY_ID_NULL);
		
		transform->local = glm::translate(glm::vec3(0.0f, 1.0f, 0.0f));

		CameraComponent camera = { };
		camera.type = CAMERA_TYPE_PERSPECTIVE;
		camera.fow  = 45.0f;
		camera.zoom =  1.0f;
		camera.near =  0.1f;
		camera.far  = 50.0f;
		entity_add_component(id, CameraComponent, camera);
		itu_entity_set_debug_name(id, "camera_gameplay");
	}

	// player
	{
		ITU_EntityId id = itu_entity_create();
		ref_player = id;
		Transform3D* transform = transform3D_add(id, ITU_ENTITY_ID_NULL);
		ES8_PlayerData data;
		data.camera_ref = ref_camera_gameplay;
		entity_add_component(id, ES8_PlayerData, data);

		itu_entity_set_debug_name(id, "player");
	}
	transform3D_reparent(ref_camera_gameplay, ref_player);

	// floor
	int grid_side = 4;
	int grid_side_half = grid_side / 2;
	int tiles_num = grid_side*grid_side;
	for(int i =0; i < tiles_num; ++i)
	{
		ITU_EntityId id = itu_entity_create();
		Transform3D* transform = transform3D_add(id, ITU_ENTITY_ID_NULL);

		int x = i % grid_side - grid_side_half;
		int z = i / grid_side - grid_side_half;
		transform->local = glm::translate(glm::vec3(x, 0.0f, z));

		MeshComponent mesh = { 2 };
		entity_add_component(id, MeshComponent, mesh);
		itu_entity_set_debug_name(id, "floor");
	}
}

void set_control_mode(SDLContext* context)
{
	SDL_SetWindowRelativeMouseMode(context->window, !context->debug_ui_show);
	if(context->debug_ui_show)
	{
		itu_entity_tag_remove(ref_player, TAG_ENTITY_CONTROLLED);
		itu_entity_tag_remove(ref_camera_gameplay, TAG_CAMERA_ACTIVE);
		itu_entity_tag_add(ref_camera_debug, TAG_CAMERA_ACTIVE);
	}
	else
	{
		itu_entity_tag_add(ref_player, TAG_ENTITY_CONTROLLED);
		itu_entity_tag_add(ref_camera_gameplay, TAG_CAMERA_ACTIVE);
		itu_entity_tag_remove(ref_camera_debug, TAG_CAMERA_ACTIVE);
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
	context.window = SDL_CreateWindow("ES08 - 3D Rendering", WINDOW_W, WINDOW_H, 0);

	itu_sys_estorage_init(512, false);
	enable_component(Transform3D);
	enable_component(MeshComponent);
	enable_component(CameraComponent);
	enable_component(ES8_PlayerData);
	add_component_debug_ui_render(Transform3D, itu_debug_ui_render_transform3D);
	add_component_debug_ui_render(MeshComponent, itu_debug_ui_render_meshcomponent);
	add_component_debug_ui_render(CameraComponent, itu_debug_ui_render_cameracomponent);
	add_system(ex8_system_update_controls_debug, component_mask(Transform3D)|component_mask(CameraComponent), tag_mask(TAG_CAMERA_ACTIVE)|tag_mask(TAG_CAMERA_DEBUG));
	add_system(ex8_system_update_player        , component_mask(Transform3D)|component_mask(ES8_PlayerData) , tag_mask(TAG_ENTITY_CONTROLLED));
	add_system(ex8_system_update_active_camera , component_mask(Transform3D)|component_mask(CameraComponent), tag_mask(TAG_CAMERA_ACTIVE));
	add_system(ex8_system_mesh_render          , component_mask(Transform3D)|component_mask(MeshComponent)  , 0);

	itu_sys_render3d_init(&context);

	itu_lib_imgui_setup(&context, true);
	
	context.camera_default.normalized_screen_size.x = 1.0f;
	context.camera_default.normalized_screen_size.y = 1.0f;
	context.camera_default.normalized_screen_offset.x = 0.0f;
	context.camera_default.zoom = 1;
	context.camera_default.pixels_per_unit = CAMERA_PIXELS_PER_UNIT;
	camera_set_active(&context, &context.camera_default);

	// set degu UI shown by default (new and shiny, let's showcase it)
	// NOTE: leaving it here for now because showing/hiding debug UI creates some poblems when mouse is in "first person" mode
	//       (grabbed by window + relative motion + hidden)
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

	sdl_input_set_mapping_keyboard(&context, SDLK_F1, BTN_TYPE_DEBUG_F1);
	sdl_input_set_mapping_keyboard(&context, SDLK_F2, BTN_TYPE_DEBUG_F2);
	sdl_input_set_mapping_keyboard(&context, SDLK_F3, BTN_TYPE_DEBUG_F3);

	
	set_control_mode(&context);
	while(!quit)
	{
		quit = sdl_process_events(&context);
		
		itu_sys_render3d_frame_begin(&context);
		itu_lib_imgui_frame_begin();

		// update
		itu_sys_estorage_systems_update(&context);

		// after gameplay is done, update global transforms for rendering
		itu_lib_transform_update_globals();

		if(context.btn_isjustpressed[BTN_TYPE_DEBUG_F1])
			set_control_mode(&context);

#ifdef ENABLE_DIAGNOSTICS
		{
			if(context.debug_ui_show)
			{
				ImGui::Begin("Debug UI", &context.debug_ui_show, 0);
				{
					ImGui::BeginTabBar("debug_ui_tab");
					if(ImGui::BeginTabItem("Entities", NULL))
					{
						itu_sys_estorage_debug_render(&context);
						ImGui::EndTabItem();
					}
					if(ImGui::BeginTabItem("Context"))
					{
						ImGui::Text("Timing");
						ImGui::LabelText("work", "%6.3f ms/f", (float)elapsed_work  / (float)MILLIS(1));
						ImGui::LabelText("tot",  "%6.3f ms/f", (float)elapsed_frame / (float)MILLIS(1));
						ImGui::LabelText("physics steps",  "%d", context.physics_steps_count);
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
		// NOTE: ordef of operations
		// 1. `itu_sys_render3d_frame_begin()`
		// 2. <update entities>
		// 3. `itu_sys_render3d_frame_end()1
		// 4. <other rendering-related end-of-frame operations>
		// 5. `itu_sys_render3d_frame_submit()`
		//
		itu_sys_render3d_frame_end(&context);
		itu_lib_imgui_frame_end(&context);
		itu_sys_render3d_frame_submit(&context);

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
